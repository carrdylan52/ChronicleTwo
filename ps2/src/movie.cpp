#include "common.h"
#include "movie.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include <eekernel.h>
#include <libdma.h>
#include <libgraph.h>
#include <libsdr.h>
#include <sifdev.h>
#include <sifdma.h>
#include <sifrpc.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <strings.h>

/**
 * Whether movie playback has started.
 */
static bool isStarted;
/**
 * Movie bytes remaining to demultiplex.
 */
static int writerest;
/**
 * Whether both fields of the current frame have been transferred.
 */
#ifdef NONMATCHING
static u8 isFrameEnd;
#endif
/**
 * Vertical blanks elapsed since display started.
 */
static u32 frd;
/**
 * Whether the vertical-blank handler submits movie fields.
 */
static u8 isCountVblank;
/**
 * Field parity of the next movie transfer.
 */
#ifdef NONMATCHING
static int Cb;
#endif
/**
 * Movie picture width.
 */
static int MpegW;
/**
 * Movie picture height.
 */
static int MpegH;
/**
 * Movie video decoder.
 */
static VideoDec videoDec;
/**
 * Open movie stream.
 */
static StrFile infile;
/**
 * Texture name supplied to the movie player.
 */
static char * TexName;
/**
 * Movie input ring.
 */
#ifdef NONMATCHING
static ReadBuf * readBuf;
#endif
/**
 * Movie bytes remaining to read.
 */
#ifdef NONMATCHING
static int readrest;
#endif
/**
 * Whether the movie stream repeats.
 */
#ifdef NONMATCHING
static u8 Loop;
#endif
/**
 * Whether the movie includes PCM playback.
 */
static u8 isWithAudio;
/**
 * Reader-thread completion flag.
 */
static volatile int stepMainStatus;
/**
 * Reader-thread exit request.
 */
static int stepMainExitFlag;
/**
 * Decoded-frame ring.
 */
static VoBuf voBuf;
/**
 * PCM audio streamer.
 */
static AudioDec audioDec;
/**
 * Zero-filled PCM block for paused playback.
 */
static u8 _0_buf[2048];
/**
 * Whether the disc has been made ready for movie streaming.
 */
static u8 isStrFileInit;

static int viBufReset(ViBuf *buf);
static int cpy2area(u8 *dst1, int size1, u8 *dst2, int size2, u8 *src1, int len1, u8 *src2, int len2);
static int audioDecEndPut(AudioDec *dec, int count);
static void setImageTag(u32 *tag, void *data, int a, int width, int height);
static int videoDecPutTs(VideoDec *dec, long pts, long dts, u8 *area, int size);
static int viBufModifyPts(ViBuf *buf, TimeStamp *range);
static int viBufRestartDMA(ViBuf *buf);
static int videoCallback(sceMpeg *mpeg, sceMpegCbDataStr *str, void *user);
static int audioDecBeginPut(AudioDec *dec, u8 **area1, int *size1, u8 **area2, int *size2);
static int viBufGetTs(ViBuf *buf, TimeStamp *ts);
static int defMain(void *);
static void videoDecMain(void *arg);
static void stepMain(void *arg);
static s32 mpegError(sceMpeg *mpeg, sceMpegCbDataError *error, void *user);
static int mpegNodata(sceMpeg *mpeg, sceMpegCbData *data, void *user);
static int mpegStopDMA(sceMpeg *mpeg, sceMpegCbData *data, void *user);
static int mpegRestartDMA(sceMpeg *mpeg, sceMpegCbData *data, void *user);
static int mpegTS(sceMpeg *mpeg, sceMpegCbDataTimeStamp *data, void *user);
static int pcmCallback(sceMpeg *mpeg, sceMpegCbDataStr *str, void *user);
static int vblankHandler(int irq);
static int handler_endimage(int irq);
static void voBufCreate(VoBuf *buf, VoData *data, VoTag *tags, int count);
static void voBufReset(VoBuf *buffer);
static s32 voBufIsFull(VoBuf *buffer);
static void voBufIncCount(VoBuf *buf);
static u8 *voBufGetData(VoBuf *buf);
static s32 voBufIsEmpty(VoBuf *buffer);
static VoTag *voBufGetTag(VoBuf *buf);
static void voBufDecCount(VoBuf *buf);
static u32 getFIFOindex(ViBuf *buf, void *addr);
static void setD3_CHCR(u32 chcr);
static void setD4_CHCR(u32 chcr);
static void scTag2(QWORD *tag, void *addr, u32 id, u32 count);
static int viBufCreate(ViBuf *buf, u_long128 *data, u_long128 *tags, int sectors, TimeStamp *ts, int ts_count);
static void viBufBeginPut(ViBuf *buf, u8 **area1, int *size1, u8 **area2, int *size2);
static void viBufEndPut(ViBuf *buf, int count);
static int viBufAddDMA(ViBuf *buf);
static int viBufStopDMA(ViBuf *buf);
static int viBufDelete(ViBuf *buf);
static void viBufFlush(ViBuf *buf);
static int viBufPutTs(ViBuf *buf, TimeStamp *ts);
static int strFileOpen(StrFile *file, char *path);
static void strFileSeek(StrFile *file);
static int strFileClose(StrFile *file);
static int strFileRead(StrFile *file, void *buf, int size);
static void readBufCreate(ReadBuf *buf);
static int readBufBeginPut(ReadBuf *buf, u8 **out);
static int readBufEndPut(ReadBuf *buf, int count);
static int readBufBeginGet(ReadBuf *buf, u8 **out);
static int readBufEndGet(ReadBuf *buf, int count);
static int audioDecCreate(AudioDec *dec, u8 *ring_buf, int ring_size, int iop_size);
static int audioDecDelete(AudioDec *dec);
static void audioDecPause(AudioDec *dec);
static void audioDecResume(AudioDec *dec);
static void audioDecStart(AudioDec *dec);
static void audioDecReset(AudioDec *dec);
static s32 audioDecIsPreset(AudioDec *decoder);
static int audioDecSendToIOP(AudioDec *dec);
static void iopGetArea(int *addr1, int *size1, int *addr2, int *size2, AudioDec *dec, int wanted);
static int sendToIOP2area(int dest1, int size1, int dest2, int size2, u8 *src1, int len1, u8 *src2,
                   int len2);
static int sendToIOP(int iop_addr, u8 *src, int size);
static void changeMasterVolume(u32 volume);
static void changeInputVolume(u32 volume);
static void startDisplay(int field);
static int switchThread(void);
static u32 videoDecSetState(VideoDec *dec, u32 state);
static s32 videoDecGetState(VideoDec *decoder);
static int decBs0(VideoDec *dec);
static void videoDecBeginPut(VideoDec *dec, u8 **area1, int *size1, u8 **area2, int *size2);
static void videoDecEndPut(VideoDec *dec, int count);
static int isAudioOK(void);

// Check divisors before ring-index modulo operations.
#pragma divbyzerocheck on

// Code (.text)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", Load__6CMovieFPcPP9mgCMemoryiibbb);
void CMovie::Load(char *name, mgCMemory *memory, int width, int height, bool with_audio, bool loop) {
    mgCMemory *pools[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    pools[0] = memory;
    pools[1] = memory;
    pools[2] = memory;
    pools[3] = memory;
    pools[4] = memory;
    pools[5] = memory;
    Load(name, pools, width, height, with_audio, loop, true);
}

void CMovie::Load(char *name, mgCMemory *memory, int width, int height, bool with_audio, bool loop, bool init_sound) {
    mgCMemory *pools[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
    pools[0] = memory;
    pools[1] = memory;
    pools[2] = memory;
    pools[3] = memory;
    pools[4] = memory;
    pools[5] = memory;
    Load(name, pools, width, height, with_audio, loop, init_sound);
}

void CMovie::Play(char *path) {
    ThreadParam param;
    if (is_playing == 0) {
        TexName = path;
        param.entry = (void (*)(void *))defMain;
        param.stack = def_stack;
        param.stackSize = 0x800;
        param.initPriority = 10;
        param.gpReg = &_gp;
        param.option = 0;
        def_thread = CreateThread(&param);
        StartThread(def_thread, 0);
        param.entry = videoDecMain;
        param.stack = video_stack;
        param.stackSize = 0x4000;
        param.initPriority = 10;
        param.gpReg = &_gp;
        param.option = 0;
        video_thread = CreateThread(&param);
        StartThread(video_thread, &videoDec);
        stepMainExitFlag = 0;
        param.entry = (void (*)(void *))stepMain;
        param.stack = step_stack;
        param.stackSize = 0x4000;
        param.initPriority = 10;
        param.gpReg = &_gp;
        param.option = 0;
        step_thread = CreateThread(&param);
        StartThread(step_thread, 0);
        videoDec.hid_vblank = AddIntcHandler(2, vblankHandler, 0);
        EnableIntc(2);
        videoDec.hid_endimage = AddDmacHandler(2, handler_endimage, 0);
        EnableDmac(2);
        is_playing = 1;
    }
}

void CMovie::SwitchThread() {
    int turn;

    turn = 0;
    if (is_playing != 0) {
        for (; turn < 4; turn++) {
            switchThread();
        }
    }
}

void CMovie::Term() {
    if (is_playing != 0) {
        videoDecFlush(&videoDec);
        switchThread();
        stepMainExitFlag = 1;
        while (stepMainStatus == 0) {
            switchThread();
        }
        TerminateThread(step_thread);
        DeleteThread(step_thread);
        TerminateThread(video_thread);
        DeleteThread(video_thread);
        TerminateThread(def_thread);
        DeleteThread(def_thread);
        DisableDmac(2);
        RemoveDmacHandler(2, videoDec.hid_endimage);
        RemoveIntcHandler(2, videoDec.hid_vblank);
        is_playing = 0;
    }
    videoDecDelete(&videoDec);
    audioDecReset(&audioDec);
    audioDecDelete(&audioDec);
    strFileClose(&infile);
}

int CMovie::EndCheck() {
    if (writerest >= 5) {
        return videoDecGetState(&videoDec) == VIDEO_DEC_STATE_END ? 1 : 0;
    }
    return 1;
}

int CMovie::IsStarted() {
    return isStarted;
}

int CMovie::GetVoBufDataSize() { return 0x1C0000; }

int CMovie::GetViBufDataSize() { return 0x80000; }

s32 CMovie::GetViBufTagSize(void) {
    return 0x1010;
}

s32 CMovie::GetMpegWorkSize(s32 width, s32 height) {
    s32 half_work_units;
    s32 pixel_work_units;

    pixel_work_units = width * height * 9;
    half_work_units = pixel_work_units >> 1;
    if (pixel_work_units < 0) {
        half_work_units = (s32) (pixel_work_units + 1) >> 1;
    }
    return half_work_units + 0x1768;
}

int CMovie::GetReadBufSize() { return 0x50050; }

s32 CMovie::GetTagProgSize(s32 width, s32 height) {
    s32 macroblocks;
    s32 tag_pages;
    s32 tag_bytes_rounded;
    s32 macroblock_columns;
    s32 macroblock_groups;

    macroblock_columns = width >> 4;
    if (width < 0) {
        macroblock_columns = (s32) (width + 0xF) >> 4;
    }
    macroblocks = macroblock_columns * height;
    macroblock_groups = macroblocks >> 4;
    if (macroblocks < 0) {
        macroblock_groups = (s32) (macroblocks + 0xF) >> 4;
    }
    tag_bytes_rounded = (((macroblock_groups * 6) + 0x6E) * 4) + 0x3F;
    tag_pages = tag_bytes_rounded >> 6;
    if (tag_bytes_rounded < 0) {
        tag_pages = (s32) (tag_bytes_rounded + 0x3F) >> 6;
    }
    return tag_pages << 8;
}

int CMovie::videoDecCreate(VideoDec *vd, u8 *work, int work_size, u_long128 *data, u_long128 *tag,
                           int n, TimeStamp *ts, int n_ts) {
    sceMpegCreate(&vd->mpeg, work, work_size);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbError, (sceMpegCallback)mpegError, 0);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbNodata, mpegNodata, 0);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbStopDMA, mpegStopDMA, 0);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbRestartDMA, mpegRestartDMA, 0);
    sceMpegAddCallback(&vd->mpeg, sceMpegCbTimeStamp, (sceMpegCallback)mpegTS, 0);
    vd->state = VIDEO_DEC_STATE_NORMAL;
    viBufCreate(&vd->vibuf, data, tag, n, ts, n_ts);
    return 1;
}

int CMovie::videoDecSetStream(VideoDec *vd, int str_type, int ch, sceMpegCallback callback, void *data) {
    sceMpegAddStrCallback(&vd->mpeg, (sceMpegStrType)(str_type & 0xFF), ch, callback, data);
    return 1;
}

int CMovie::videoDecDelete(VideoDec *vd) {
    viBufDelete(&vd->vibuf);
    sceMpegDelete(&vd->mpeg);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", videoDecFlush__6CMovieFP8VideoDec);
/**
 * Yields to the other movie threads.
 */
static int defMain(void *) {
    for (;;) {
        switchThread();
    }
}

/**
 * Decodes video frames until the stream ends and all output is displayed.
 */
static void videoDecMain(void *arg) {
    VideoDec *dec = (VideoDec *)arg;

    viBufReset(&dec->vibuf);
    voBufReset(&voBuf);
    decBs0(dec);
    while (voBuf.count != 0) {
    }
    videoDecSetState(dec, VIDEO_DEC_STATE_END);
}

/**
 * Reads movie data, demultiplexes the stream, and starts buffered playback.
 */
#ifdef NONMATCHING
static void stepMain(void *arg) {
    u8 *put_area;
    u8 *get_area;
    VideoDec *dec = &videoDec;
    ReadBuf *ring = readBuf;
    StrFile *file = &infile;
    static int cnt = 0;
    stepMainStatus = 0;
    do {
        if (Loop != 0 && readrest < 0x50001) {
            strFileSeek(file);
            readrest = infile.size;
        }
        int room = readBufBeginPut(ring, &put_area);
        if (readrest > 0 && room >= 0x10000) {
            int bytes_read = strFileRead(file, put_area, 0x10000);
            readBufEndPut(ring, bytes_read);
            readrest -= bytes_read;
        }
        switchThread();
        int available = readBufBeginGet(ring, &get_area);
        if (available > 0) {
            int consumed = sceMpegDemuxPssRing(&dec->mpeg, get_area, available, ring->data, ring->size);
            readBufEndGet(ring, consumed);
            writerest -= consumed;
            if (writerest <= 0) {
                writerest = infile.size;
            }
        }
        audioDecSendToIOP(&audioDec);
        if (isStarted == 0 && voBufIsFull(&voBuf) != 0 && isAudioOK() != 0) {
            startDisplay(1);
            if (isWithAudio != 0) {
                audioDecStart(&audioDec);
            }
            isStarted = 1;
        }
    } while (stepMainExitFlag == 0);
    stepMainStatus = 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", stepMain__FPv);
#endif

/**
 * Acknowledges an MPEG decoder error.
 */
static s32 mpegError(sceMpeg *mpeg, sceMpegCbDataError *error, void *user) {
    return 1;
}

/**
 * Queues further bit-stream DMA while yielding to the reader.
 */
static int mpegNodata(sceMpeg *mpeg, sceMpegCbData *data, void *user) {
    switchThread();
    viBufAddDMA(&videoDec.vibuf);
    return 1;
}

/**
 * Stops the video bit-stream DMA.
 */
static int mpegStopDMA(sceMpeg *mpeg, sceMpegCbData *data, void *user) {
    viBufStopDMA(&videoDec.vibuf);
    return 1;
}

/**
 * Restores the video bit-stream DMA.
 */
static int mpegRestartDMA(sceMpeg *mpeg, sceMpegCbData *data, void *user) {
    viBufRestartDMA(&videoDec.vibuf);
    return 1;
}

/**
 * Supplies the next presentation and decoding timestamps.
 */
static int mpegTS(sceMpeg *mpeg, sceMpegCbDataTimeStamp *data, void *user) {
    TimeStamp ts;
    viBufGetTs(&videoDec.vibuf, &ts);
    data->pts = ts.pts;
    data->dts = ts.dts;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", videoCallback__FP7sceMpegP16sceMpegCbDataStrPv);
/**
 * Copies demultiplexed PCM data into the audio ring.
 */
static int pcmCallback(sceMpeg *mpeg, sceMpegCbDataStr *str, void *user) {
    u8 *area1;
    u8 *area2;
    int size1;
    int size2;
    u8 *src;
    int total;
    int first;
    int ring_size;
    u8 *ring_end;
    src = str->data + 4;
    ring_size = ((ReadBuf *)user)->size;
    ring_end = (u8 *)user + ring_size;
    if (src >= ring_end) {
        src -= ring_size;
    }
    first = ring_end - src;
    total = str->len - 4;
    first = (total < first) ? total : first;
    audioDecBeginPut(&audioDec, &area1, &size1, &area2, &size2);
    int copied = cpy2area(area1, size1, area2, size2, src, first, (u8 *)user, total - first);
    audioDecEndPut(&audioDec, copied);
    int result = 0;
    if (copied > 0) {
        result = 1;
    }
    return result;
}

/**
 * Submits the next field of a decoded frame at vertical blank.
 */
#ifdef NONMATCHING
static int vblankHandler(int irq) {
    if (isCountVblank) {
        VoTag *tag = voBufGetTag(&voBuf);
        if (tag == 0) {
            frd++;
            EIntr();
            return 0;
        }
        if (Cb == 0 && tag->status == VO_TAG_STATUS_READY) {
            sceDmaSend(DmaCH2, tag->v[0]);
            tag->status = VO_TAG_STATUS_FIRST;
        } else if (Cb == 1 && tag->status == VO_TAG_STATUS_FIRST) {
            sceDmaSend(DmaCH2, tag->v[1]);
            tag->status = VO_TAG_STATUS_FREE;
            isFrameEnd = 1;
        }
        Cb ^= 1;
    }
    EIntr();
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", vblankHandler__Fi);
#endif

/**
 * Releases a frame after its second field transfer.
 */
#ifdef NONMATCHING
static int handler_endimage(int irq) {
    if (isFrameEnd) {
        voBufDecCount(&voBuf);
        isFrameEnd = 0;
    }
    EIntr();
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", handler_endimage__Fi);
#endif

/**
 * Initializes the decoded-frame ring.
 */
static void voBufCreate(VoBuf *buf, VoData *data, VoTag *tags, int count) {
    buf->data = data;
    buf->tag = tags;
    buf->ring_tag = tags;
    buf->size = count;
    buf->count = 0;
    buf->write = 0;
    for (int i = 0; i < count; i++) {
        buf->tag[i].status = VO_TAG_STATUS_FREE;
    }
}

/**
 * Empties the decoded-frame ring.
 */
static void voBufReset(VoBuf *buffer) {
    buffer->write = 0;
    buffer->count = 0;
}

/**
 * Reports whether every output frame slot is occupied.
 */
static s32 voBufIsFull(VoBuf *buffer) {
    return buffer->count == buffer->size;
}

/**
 * Queues the newly decoded frame for display.
 */
static void voBufIncCount(VoBuf *buf) {
    DIntr();
    buf->ring_tag[buf->write].status = VO_TAG_STATUS_READY;
    buf->count++;
    buf->write = (buf->write + 1) % buf->size;
    EIntr();
}

/**
 * Returns the next free decoded-frame buffer.
 */
static u8 *voBufGetData(VoBuf *buf) {
    if (voBufIsFull(buf)) {
        return 0;
    }
    return (u8 *)&buf->data[buf->write];
}

/**
 * Reports whether the decoded-frame ring is empty.
 */
static s32 voBufIsEmpty(VoBuf *buffer) {
    return buffer->count == 0;
}

/**
 * Returns the display record for the oldest queued frame.
 */
static VoTag *voBufGetTag(VoBuf *buf) {
    if (voBufIsEmpty(buf)) {
        return 0;
    }
    return &buf->ring_tag[(buf->write - buf->count + buf->size) % buf->size];
}

/**
 * Releases the oldest queued frame.
 */
static void voBufDecCount(VoBuf *buf) {
    if (buf->count > 0) {
        buf->count = buf->count - 1;
    }
}

/**
 * Converts a DMA address to its bit-stream block index.
 */
static u32 getFIFOindex(ViBuf *buf, void *addr) {
    if (addr == (void *)(((u32)&buf->tag[buf->n + 1]) & 0xFFFFFFF)) {
        return 0;
    }
    return (u32)((u8 *)addr - (u8 *)buf->data) >> 11;
}

/**
 * Writes the IPU output DMA channel control while masking DMA interrupts.
 */
static void setD3_CHCR(u32 chcr) {
    DIntr();
    int mask = *(int *)0x1000F520;
    *(int *)0x1000F590 = mask | 0x10000;
    *(u32 *)0x1000B000 = chcr;
    *(int *)0x1000F590 = *(int *)0x1000F520 & 0xFFFEFFFF;
    EIntr();
}

/**
 * Writes the IPU input DMA channel control while masking DMA interrupts.
 */
static void setD4_CHCR(u32 chcr) {
    DIntr();
    int mask = *(int *)0x1000F520;
    *(int *)0x1000F590 = mask | 0x10000;
    *(u32 *)0x1000B400 = chcr;
    *(int *)0x1000F590 = *(int *)0x1000F520 & 0xFFFEFFFF;
    EIntr();
}

/**
 * Builds the lower doubleword of a DMA tag.
 */
static void scTag2(QWORD *tag, void *addr, u32 id, u32 count) {
    tag->l[0] = ((u64)(u32)addr << 32) | ((u64)id << 28) | (u64)count;
}

/**
 * Initializes the bit-stream ring and its semaphore.
 */
static int viBufCreate(ViBuf *buf, u_long128 *data, u_long128 *tags, int sectors, TimeStamp *ts, int ts_count) {
    buf->data = data;
    buf->tag = (u_long128 *)(((u32)tags & 0xFFFFFFF) | 0x20000000);
    buf->n = sectors;
    buf->buff_size = sectors << 11;
    buf->ts = ts;
    buf->n_ts = ts_count;
    SemaParam sema;
    sema.initCount = 1;
    sema.maxCount = 1;
    buf->sema = CreateSema(&sema);
    viBufReset(buf);
    buf->total_bytes = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", viBufReset__FP5ViBuf);
/**
 * Returns the writable portions of the bit-stream ring.
 */
static void viBufBeginPut(ViBuf *buf, u8 **area1, int *size1, u8 **area2, int *size2) {
    WaitSema(buf->sema);
    int write_pos;
    int queued = buf->read_bytes;
    int used = buf->dma_n;
    int size = buf->buff_size;
    int end_bytes = (buf->dma_start + used) << 11;
    write_pos = (end_bytes + queued) % size;
    int free = (((buf->n - 2) - used) << 11) - queued;
    if (size - write_pos >= free) {
        *area1 = (u8 *)buf->data + write_pos;
        *size1 = free;
        *area2 = NULL;
        *size2 = 0;
    } else {
        *area1 = (u8 *)buf->data + write_pos;
        *size1 = buf->buff_size - write_pos;
        *area2 = (u8 *)buf->data;
        *size2 = free - (buf->buff_size - write_pos);
    }
    SignalSema(buf->sema);
}

/**
 * Records bytes written into the bit-stream ring.
 */
static void viBufEndPut(ViBuf *buf, int count) {
    WaitSema(buf->sema);
    buf->read_bytes += count;
    buf->total_bytes += count;
    SignalSema(buf->sema);
}

/**
 * Extends the DMA chain with newly buffered bit-stream blocks.
 */
static int viBufAddDMA(ViBuf *buf) {
    int chained = 0;
    WaitSema(buf->sema);
    if (buf->is_active == 0) {
        printf("DMA ADD not active\n");
        return 0;
    }
    setD4_CHCR(5);
    u32 chcr = *(u32 *)0x1000B400;
    u32 madr = *(u32 *)0x1000B410;
    int fifo_index = getFIFOindex(buf, (void *)madr);
    int consumed = (fifo_index + buf->n - buf->dma_start) % buf->n;
    buf->dma_start = (buf->dma_start + consumed) % buf->n;
    buf->dma_n -= consumed;
    int ready;
    int tail = (buf->dma_start + buf->dma_n) % buf->n;
    ready = buf->read_bytes / 0x800;
    buf->read_bytes %= 0x800;
    if (ready > 0) {
        int last = (buf->dma_start + buf->dma_n - 1 + buf->n) % buf->n;
        scTag2((QWORD *)(buf->tag + last), (void *)((u8 *)buf->data + last * 0x800), 3,
               0x80);
        chained = 1;
    }
    for (int i = 0; i < ready; i++) {
        scTag2((QWORD *)(buf->tag + tail), (void *)((u8 *)buf->data + tail * 0x800),
               i == ready - 1 ? 0 : 3, 0x80);
        tail = (tail + 1) % buf->n;
    }
    buf->dma_n += ready;
    if (buf->dma_n != 0) {
        if (chained) {
            chcr = (chcr & 0xFFFFFFF) | 0x30000000;
        }
        setD4_CHCR(chcr | 0x100);
    }
    SignalSema(buf->sema);
    return 1;
}

// Optimize the DMA stop routine as one region.
#pragma optimization_level 4
/**
 * Saves the DMA and IPU state while stopping bit-stream transfers.
 */
static int viBufStopDMA(ViBuf *buf) {
    WaitSema(buf->sema);
    buf->is_active = 0;
    setD4_CHCR(5);
    buf->env.d4madr = *(u32 *)0x1000B410;
    buf->env.d4tadr = *(u32 *)0x1000B430;
    buf->env.d4qwc = *(u32 *)0x1000B420;
    buf->env.d4chcr = *(u32 *)0x1000B400;
    volatile u32 *ipu_ctrl = (volatile u32 *)0x10002010;
    while ((*ipu_ctrl & 0xF0) != 0) {
    }
    setD3_CHCR(0);
    buf->env.d3madr = *(u32 *)0x1000B010;
    buf->env.d3qwc = *(u32 *)0x1000B020;
    buf->env.d3chcr = *(u32 *)0x1000B000;
    buf->env.ipubp = *(u32 *)0x10002020;
    buf->env.ipuctrl = *(u32 *)0x10002010;
    SignalSema(buf->sema);
    return 1;
}

#pragma optimization_level reset
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", viBufRestartDMA__FP5ViBuf);
/**
 * Clears the input DMA channel and deletes the ring semaphore.
 */
static int viBufDelete(ViBuf *buf) {
    setD4_CHCR(5U);
    *(int *)0x1000B420 = 0;
    *(int *)0x1000B410 = 0;
    *(int *)0x1000B430 = 0;
    DeleteSema(buf->sema);
    return 1;
}

/**
 * Rounds the buffered byte count up to a complete bit-stream block.
 */
static void viBufFlush(ViBuf *buf) {
    WaitSema(buf->sema);
    buf->read_bytes = (buf->read_bytes + 0x7FF) / 0x800 * 0x800;
    SignalSema(buf->sema);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", viBufModifyPts__FP5ViBufP9TimeStamp);
/**
 * Appends a timestamp when the timestamp ring has room.
 */
static int viBufPutTs(ViBuf *buf, TimeStamp *ts) {
    int had_room = 0;
    WaitSema(buf->sema);
    if (buf->count_ts < buf->n_ts) {
        viBufModifyPts(buf, ts);
        if (ts->pts >= 0 || ts->dts >= 0) {
            buf->ts[buf->wt_ts].pts = ts->pts;
            buf->ts[buf->wt_ts].dts = ts->dts;
            buf->ts[buf->wt_ts].pos = ts->pos;
            buf->ts[buf->wt_ts].len = ts->len;
            buf->count_ts++;
            buf->wt_ts = (buf->wt_ts + 1) % buf->n_ts;
        }
        had_room = 1;
    }
    SignalSema(buf->sema);
    return had_room;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", viBufGetTs__FP5ViBufP9TimeStamp);
/**
 * Opens the movie file and initializes its streaming state.
 */
static int strFileOpen(StrFile *file, char *path) {
    char full_path[0x100];
    char device[0x4C];
    sceCdRMode cd_mode;
    char *suffix;
    char *colon = index(path, ':');
    if (colon != NULL) {
        int device_len = colon - path;
        strncpy(device, path, device_len);
        device[device_len] = 0;
        if (strcmp(device, "cdrom0") == 0) {
            int i;
            int length = strlen(&colon[1]);
            i = 0;
            file->is_on_cd = 1;
            while (i < length) {
                if (colon[i + 1] == '/') {
                    colon[i + 1] = '\\';
                }
                colon[i + 1] = toupper(colon[i + 1]);
                i++;
            }
            sprintf(full_path, "%s%s;1", colon + 1, suffix);
        } else {
            file->is_on_cd = 0;
            sprintf(full_path, "%s:%s", device, colon + 1);
        }
    } else {
        strcpy(device, "host0");
        file->is_on_cd = 0;
        sprintf(full_path, "%s:%s", device, path);
    }
    file->is_on_cd = 1;
    strcpy(full_path, "\\MOVIE\\");
    strcat(full_path, path);
    strcat(full_path, ";1");
    printf("file:[%d] %s\n", file->is_on_cd, full_path);
    if (file->is_on_cd != 0) {
        if (isStrFileInit == 0) {
            sceCdDiskReady(0);
            isStrFileInit = 1;
        }
        file->iop_buf = iop_bd_addr;
        sceCdStInit(0x50, 5, (void *)(((u32)file->iop_buf + 0xF) & ~0xF));
        if (sceCdSearchFile(&file->fp, full_path) == 0) {
            printf("Cannot open '%s'(sceCdSearchFile)\n", full_path);
            for (;;) {
            }
        }
        file->size = file->fp.size;
        cd_mode.trycount = 0;
        cd_mode.spindlctrl = 0;
        cd_mode.datapattern = 0;
        sceCdStStart(file->fp.lsn, &cd_mode);
    } else {
        file->fd = sceOpen(full_path, 1);
        if (file->fd < 0) {
            printf("Cannot open '%s'(sceOpen)\n", full_path);
            return 0;
        }
        file->size = sceLseek(file->fd, 0, 2);
        if (file->size < 0) {
            printf("sceLseek() fails (%s): %d\n", full_path, file->size);
            sceClose(file->fd);
            return 0;
        }
        if (sceLseek(file->fd, 0, 0) < 0) {
            printf("sceLseek() fails (%s)\n", full_path);
            sceClose(file->fd);
            return 0;
        }
    }
    return 1;
}

/**
 * Rewinds the movie stream.
 */
static void strFileSeek(StrFile *file) {
    if (file->is_on_cd != 0) {
        sceCdStSeekF(file->fp.lsn);
        return;
    }
    sceLseek(file->fd, 0, 0);
}

/**
 * Closes the movie stream.
 */
static int strFileClose(StrFile *file) {
    if (file->is_on_cd != 0) {
        sceCdStStop();
    } else {
        sceClose(file->fd);
    }
    return 1;
}

/**
 * Reads bytes from the movie stream.
 */
static int strFileRead(StrFile *file, void *buf, int size) {
    u32 err;
    if (file->is_on_cd != 0) {
        return sceCdStRead(size >> 11, buf, 1, &err) << 11;
    }
    return sceRead(file->fd, buf, size);
}

/**
 * Initializes the movie input ring.
 */
static void readBufCreate(ReadBuf *buf) {
    buf->count = 0;
    buf->put = 0;
    buf->size = 0x50000;
}

/**
 * Returns the next writable input-ring area.
 */
static int readBufBeginPut(ReadBuf *buf, u8 **out) {
    int room = buf->size - buf->count;
    if (room != 0) {
        *out = buf->data + buf->put;
    }
    return room;
}

/**
 * Records bytes written into the input ring.
 */
static int readBufEndPut(ReadBuf *buf, int count) {
    int room = buf->size - buf->count;
    int stored = (count < room) ? count : room;
    buf->put = (buf->put + stored) % buf->size;
    buf->count += stored;
    return stored;
}

/**
 * Returns the oldest buffered movie input.
 */
static int readBufBeginGet(ReadBuf *buf, u8 **out) {
    if (buf->count != 0) {
        *out = buf->data + (buf->put - buf->count + buf->size) % buf->size;
    }
    return buf->count;
}

/**
 * Releases consumed movie input bytes.
 */
static int readBufEndGet(ReadBuf *buf, int count) {
    int avail = buf->count;
    int taken = (count < avail) ? count : avail;
    buf->count -= taken;
    return taken;
}

/**
 * Initializes PCM streaming buffers in EE and IOP memory.
 */
static int audioDecCreate(AudioDec *dec, u8 *ring_buf, int ring_size, int iop_size) {
    int iop_buf;
    int iop_stub;

    dec->state = AUDIO_DEC_STATE_HEADER;
    dec->hdr_count = 0;
    dec->data = ring_buf;
    dec->put = 0;
    dec->count = 0;
    dec->size = ring_size;
    dec->total_bytes = 0;
    dec->total_bytes_sent = 0;
    dec->iop_buff_size = iop_size;
    dec->iop_last_pos = 0;
    dec->iop_pause_pos = 0;
    dec->iop_buff = (int)sceSifAllocIopHeap(iop_size);
    iop_buf = (int)(dec->iop_buff);
    if (iop_buf < 0) {
        printf("Cannot allocate IOP memory\n", iop_buf);
        return 0;
    }
    printf("IOP memory 0x%08x(size:%d) is allocated\n", iop_buf, iop_size);
    dec->iop_zero = (int)sceSifAllocIopHeap(0x800);
    iop_stub = (int)(dec->iop_zero);
    if (iop_stub < 0) {
        printf("Cannot allocate IOP memory\n", iop_stub);
        return 0;
    }
    printf("IOP memory 0x%08x(size:%d) is allocated\n", iop_stub, 0x800);
    memset(&_0_buf, 0, 0x800);
    sendToIOP(dec->iop_zero, &_0_buf[0], 0x800);
    changeMasterVolume(0x3FFFU);
    sndSetMasterVol(0, 1.0f);
    sndSetMasterVol(1, 1.0f);
    return 1;
}

/**
 * Frees the IOP audio buffers.
 */
static int audioDecDelete(AudioDec *dec) {
    sceSifFreeIopHeap((void *)dec->iop_buff);
    sceSifFreeIopHeap((void *)dec->iop_zero);
    return 1;
}

/**
 * Mutes audio and records the IOP playback position.
 */
static void audioDecPause(AudioDec *dec) {
    dec->state = AUDIO_DEC_STATE_PAUSE;
    changeInputVolume(0);
    dec->iop_pause_pos = (sceSdRemote(1, 0x80E0, 1, 2, 0, 0) & 0xFFFFFF) - dec->iop_buff;
    sceSdRemote(1, 0x80D0, 1, 0, dec->iop_zero, 0x4000, 0x800);
}

/**
 * Restarts PCM playback from the paused position.
 */
static void audioDecResume(AudioDec *dec) {
    changeInputVolume(0x7FFF);
    int size = dec->iop_buff_size;
    int base = dec->iop_buff;
    sceSdRemote(1, 0x80E0, 1, 0x13, base, size / 0x400 * 0x400, base + dec->iop_pause_pos);
    dec->state = AUDIO_DEC_STATE_PLAY;
}

/**
 * Starts PCM playback.
 */
static void audioDecStart(AudioDec *dec) {
    audioDecResume(dec);
}

/**
 * Pauses playback and empties the audio ring.
 */
static void audioDecReset(AudioDec *dec) {
    audioDecPause(dec);
    dec->state = AUDIO_DEC_STATE_HEADER;
    dec->hdr_count = 0;
    dec->put = 0;
    dec->count = 0;
    dec->total_bytes = 0;
    dec->total_bytes_sent = 0;
    dec->iop_last_pos = 0;
    dec->iop_pause_pos = 0;
}

/**
 * Reports whether enough audio has been sent to fill the IOP ring.
 */
static s32 audioDecIsPreset(AudioDec *decoder) {
    return decoder->total_bytes_sent >= decoder->iop_buff_size;
}

/**
 * Transfers complete PCM blocks into free IOP ring space.
 */
static int audioDecSendToIOP(AudioDec *dec) {
    int iop_addr1;
    int iop_addr2;
    int iop_size1;
    int iop_size2;
    int sent = 0;
    switch (dec->state) {
        case AUDIO_DEC_STATE_HEADER:
            return 0;
        case AUDIO_DEC_STATE_PRESET:
            iop_addr1 = dec->iop_buff + dec->total_bytes_sent % dec->iop_buff_size;
            iop_size1 = dec->iop_buff_size - dec->total_bytes_sent;
            iop_size2 = 0;
            iop_addr2 = 0;
            break;
        case AUDIO_DEC_STATE_PLAY:
            iopGetArea(&iop_addr1, &iop_size1, &iop_addr2, &iop_size2, dec,
                       (sceSdRemote(1, 0x8100, 1) & 0xFFFFFF) - dec->iop_buff);
            break;
        case AUDIO_DEC_STATE_PAUSE:
            return 0;
    }
    int ring_size = dec->size;
    u8 *read_pos = &dec->data[(ring_size + (dec->put - dec->count)) % ring_size];
    int used = dec->count;
    u8 *ring = dec->data;
    int blocks = used / 0x400;
    int whole = blocks * 0x400;
    int first = &ring[ring_size] - read_pos;
    if (whole < first) {
        first = whole;
    }
    int second = whole - first;
    if (iop_size1 + iop_size2 >= 0x400 && first + second >= 0x400) {
        sent = sendToIOP2area(iop_addr1, iop_size1, iop_addr2, iop_size2, read_pos, first, ring, second);
    }
    dec->count -= sent;
    dec->total_bytes_sent += sent;
    dec->iop_last_pos = (dec->iop_last_pos + sent) % dec->iop_buff_size;
    return sent;
}

/**
 * Returns writable portions of the IOP audio ring.
 */
static void iopGetArea(int *addr1, int *size1, int *addr2, int *size2, AudioDec *dec, int wanted) {
    int room = (wanted + dec->iop_buff_size - dec->iop_last_pos - 0x400) % dec->iop_buff_size;
    int len = room / 0x400 * 0x400;
    if (dec->iop_buff_size - dec->iop_last_pos >= len) {
        *addr1 = dec->iop_buff + dec->iop_last_pos;
        *size1 = len;
        *size2 = 0;
        *addr2 = 0;
    } else {
        *addr1 = dec->iop_buff + dec->iop_last_pos;
        *size1 = dec->iop_buff_size - dec->iop_last_pos;
        *addr2 = dec->iop_buff;
        *size2 = len - (dec->iop_buff_size - dec->iop_last_pos);
    }
}

/**
 * Transfers wrapped EE audio into wrapped IOP ring space.
 */
static int sendToIOP2area(int dest1, int size1, int dest2, int size2, u8 *src1, int len1, u8 *src2,
                   int len2) {
    int end1 = size1 + size2;
    int end2 = len1 + len2;
    if (end1 < end2) {
        int excess = end2 - end1;
        if (excess >= len2) {
            len1 -= excess - len2;
            len2 = 0;
        } else {
            len2 -= excess;
        }
    }
    int rest = size1 - len1;
    if (len1 >= size1) {
        sendToIOP(dest1, src1, size1);
        sendToIOP(dest2, &src1[size1], len1 - size1);
        sendToIOP(dest2 + len1 - size1, src2, len2);
    } else if (len2 >= rest) {
        sendToIOP(dest1, src1, len1);
        sendToIOP(dest1 + len1, src2, rest);
        sendToIOP(dest2, &src2[size1] - len1, len2 - rest);
    } else {
        sendToIOP(dest1, src1, len1);
        sendToIOP(dest1 + len1, src2, len2);
    }
    return len1 + len2;
}

/**
 * Transfers bytes from EE memory to IOP memory through SIF DMA.
 */
static int sendToIOP(int iop_addr, u8 *src, int size) {
    if (size <= 0) {
        return 0;
    }
    sceSifDmaData dma;
    dma.data = src;
    dma.addr = (void *)iop_addr;
    dma.size = size;
    dma.mode = 0;
    FlushCache(0);
    int id = sceSifSetDma(&dma, 1);
    while (sceSifDmaStat(id) >= 0) {
    }
    return size;
}

/**
 * Sets master volume on both SPU cores.
 */
static void changeMasterVolume(u32 volume) {
    for (int core = 0; core < 2; core++) {
        sceSdRemote(1, rSdSetParam, core | SD_P_MVOLL, volume);
        sceSdRemote(1, rSdSetParam, core | SD_P_MVOLR, volume);
    }
}

/**
 * Sets the PCM input volume.
 */
static void changeInputVolume(u32 volume) {
    sceSdRemote(1, rSdSetParam, 0xF81, volume);
    sceSdRemote(1, rSdSetParam, 0x1081, volume);
}

/**
 * Waits for a field boundary and enables movie display.
 */
static void startDisplay(int field) {
    while (field == sceGsSyncV(0)) {
    }
    frd = 0;
    isCountVblank = 1;
}

/**
 * Yields within the movie thread priority.
 */
static int switchThread(void) {
    return RotateThreadReadyQueue(10);
}

/**
 * Changes decoder progress and returns the preceding state.
 */
static u32 videoDecSetState(VideoDec *dec, u32 state) {
    u32 old = dec->state;
    dec->state = state;
    return old;
}

/**
 * Returns decoder progress.
 */
static s32 videoDecGetState(VideoDec *decoder) {
    return decoder->state;
}

/**
 * Decodes pictures and queues their frame upload chains.
 */
static int decBs0(VideoDec *dec) {
    if (sceMpegIsEnd(&dec->mpeg) == 0) {
        do {
            u8 *picture = voBufGetData(&voBuf);
            if (picture == 0) {
                do {
                    switchThread();
                    picture = voBufGetData(&voBuf);
                } while (picture == 0);
            }
            int mb_width = MpegW / 16;
            int mb_count = mb_width * MpegH / 16;
            if (sceMpegGetPicture(&dec->mpeg, (sceIpuRGB32 *)picture, mb_count) < 0) {
                printf("sceMpegGetPicture() decode error");
            }
            int i = 0;
            if (dec->mpeg.frameCount == 0) {
                for (; i < voBuf.size; i++) {
                    setImageTag(voBuf.ring_tag[i].v[0],
                                &voBuf.data[i], 0, dec->mpeg.width, dec->mpeg.height);
                    setImageTag(voBuf.ring_tag[i].v[1],
                                &voBuf.data[i], 0, dec->mpeg.width, dec->mpeg.height);
                }
            }
            voBufIncCount(&voBuf);
            switchThread();
        } while (sceMpegIsEnd(&dec->mpeg) == 0);
    }
    sceMpegReset(&dec->mpeg);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", setImageTag__FPUiPviii);
/**
 * Returns writable video bit-stream areas.
 */
static void videoDecBeginPut(VideoDec *dec, u8 **area1, int *size1, u8 **area2, int *size2) {
    viBufBeginPut(&dec->vibuf, area1, size1, area2, size2);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", videoDecPutTs__FP8VideoDecllPUci);
/**
 * Records video bit-stream bytes received.
 */
static void videoDecEndPut(VideoDec *dec, int count) {
    viBufEndPut(&dec->vibuf, count);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", cpy2area__FPUciPUciPUciPUci);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", audioDecBeginPut__FP8AudioDecPPUcPiPPUcPi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/movie", audioDecEndPut__FP8AudioDeci);
/**
 * Reports whether audio buffering permits playback to start.
 */
static int isAudioOK(void) {
    return isWithAudio != 0 ? audioDecIsPreset(&audioDec) : 1;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1276__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1287__2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_318__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_319__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_320__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_321__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_322__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_323__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_584__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_810__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1028__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1029__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1030__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1031__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1032__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1033__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1034__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1035__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1036__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1037__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1038__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1109__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1110__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_1270__3__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/movie", at_468__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(frd, 0x4);
INCLUDE_BSS(TexName, 0x4);
INCLUDE_BSS(readBuf, 0x4);
INCLUDE_BSS(writerest, 0x4);
INCLUDE_BSS(readrest, 0x4);
INCLUDE_BSS(isWithAudio, 0x4);
INCLUDE_BSS(isStarted, 0x4);
INCLUDE_BSS(isStrFileInit, 0x4);
INCLUDE_BSS(Loop, 0x4);
INCLUDE_BSS(MpegW, 0x4);
INCLUDE_BSS(MpegH, 0x4);
INCLUDE_BSS(isCountVblank, 0x4);
INCLUDE_BSS(isFrameEnd, 0x4);
INCLUDE_BSS(Cb, 0x4);
INCLUDE_BSS(stepMainStatus, 0x4);
INCLUDE_BSS(stepMainExitFlag, 0x4);
INCLUDE_BSS(cnt_513, 0x4);
INCLUDE_BSS(init_514, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(videoDec, 0xC0);
INCLUDE_BSS(audioDec, 0x60);
INCLUDE_BSS(voBuf, 0x20);
INCLUDE_BSS(infile, 0x40);
INCLUDE_BSS(_0_buf, 0x800);
INCLUDE_BSS(at_344, 0x20);
INCLUDE_BSS(at_349, 0x20);
