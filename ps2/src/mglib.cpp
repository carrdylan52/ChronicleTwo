#include "common.h"
#include "mglib.hpp"

#include <cstdio>
#include <cstring>
#include <eekernel.h>
#include <libdev.h>
#include <sifdev.h>

#include "mg_drawprim.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_tanime.hpp"
#include "mg_texture.hpp"
#include "mg_visual.hpp"

static int             draw_performance_meter;                                /**< Non-zero draws the frame timing meter. */
static int             rot_priority = -1;                                     /**< Ready queue rotated while waiting for vertical sync. */
static int             vcount;                                                /**< Number of vertical syncs received. */
static int (*VSyncCallBack2)(int); /**< Additional vertical sync handler. */
static int             font_cons = -1;                                        /**< Handle of the development console. */
static int             font_draw_flag;                                        /**< Non-zero displays the development console. */
static int             mgChangeLight;                                         /**< Non-zero requests lighting state to be sent. */
static int             mgDBuffID;                                             /**< Display buffer selected for the current frame. */
static int             mgDataID;                                              /**< Packet and data buffers selected for the current frame. */
#ifdef NONMATCHING
static int             old_vcount;                                            /**< Vertical sync count at the preceding buffer swap. */
#endif
#ifdef NONMATCHING
static int             over_vsync;                                            /**< Vertical syncs beyond the requested frame interval. */
#endif
static int             call_back_active;                                      /**< Non-zero while the vertical sync handler runs. */
static int             h_count;                                               /**< Root counter value at the preceding buffer swap. */
#ifdef NONMATCHING
static int             capture_on;                                            /**< Non-zero requests the next frame capture. */
#endif
#ifdef NONMATCHING
static int             cap_ture_cnt;                                          /**< Alternating capture phase at the single-sync frame rate. */
#endif
#ifdef NONMATCHING
static int             frame_buf0;                                            /**< First frame buffer address in GS pages. */
#endif
#ifdef NONMATCHING
static int             frame_buf1;                                            /**< Second frame buffer address in GS pages. */
#endif
static int             packet_size;                                           /**< Capacity of a VIF packet buffer. */
static u_int          *packetbuf[2];                                          /**< Storage of the two VIF packets. */
static sceVif1Packet   vifpacket[2];                                          /**< Alternating VIF packet builders. */
mgRENDER_INFO mgRenderInfo;
mgCTextureManager mgTexManager;
mgCDrawManager mgDrawManager;
static mgCMemory       packet_buf[2];                                         /**< Alternating packet memory managers. */
static mgCMemory       data_buf[2];                                           /**< Alternating drawing data memory managers. */
static mgCTexture      frame_tex;                                             /**< Texture covering the current frame buffer. */
static mgCTexture      fixz_tex[2];                                           /**< Textures covering the depth buffer. */
#ifdef NONMATCHING
static sceGsDimx       mgDIMX;                                                /**< Packed GS dither matrix. */
#endif
#ifdef NONMATCHING
static sceGsStoreImage gs_simage;                                             /**< GS image read-back packet. */
#endif

static int VSyncCallBack(int cause);
static void StoreImage(int front_buffer);
static int             now_prog_id = -1;                                      /**< Identifier of the last uploaded VU program. */
static u_long128     **user_prog_adr;                                         /**< Registered user VU upload packets. */
static int             user_prog_num;                                         /**< Number of registered user VU programs. */
static u_long128      *prog_adr[3] = { Vu_prog0, Vu_prog_sdw, Vu_prog_3dsp }; /**< Built-in VU upload packets. */

// Code (.text)
void mgPerformanceMeter(int enable) {
    draw_performance_meter = enable;
}

int mgGetPerformanceMeterFlag() {
    return draw_performance_meter;
}

/**
 * Updates the display field and frame counter on a vertical sync interrupt.
 */
// Preserve the interrupt handler's instruction scheduling.
#pragma global_optimizer off
static int VSyncCallBack(int cause) {
    call_back_active = 1;

    VSyncField = (u_char)((((*(u_long *)0x12001000 >> 13) & 1) != 0) ^ 1);
    if (VSyncCallBack2 != NULL) {
        VSyncCallBack2(cause);
    }
    vcount++;
    if (vcount < 0) {
        vcount = 0;
    }
    call_back_active = 0;
    asm {
        sync
        ei
    }
    return 0;
}

#pragma global_optimizer reset

void mgInitVSyncCallBack(int (*callback)(int)) {
    VSyncCallBack2 = callback;
}

void mgSetRotateThread(int priority) {
    rot_priority = priority;
}

/**
 * Waits for a number of vertical syncs, sharing the ready queue while waiting.
 */
static void WaitVSync(int start, int count) {
    for (;;) {
        if (mgGetVSyncCount() - start >= count) {
            return;
        }
        if (rot_priority > 0) {
            RotateThreadReadyQueue(rot_priority);
        }
    }
}

int mgGetVSyncCount() {
    return vcount;
}

/**
 * Selects the screen dimensions and the bounds relative to their centre.
 */
static int GetScreenSize(int mode, int *width, int *height, int *left, int *top, int *right, int *bottom) {
    switch (mode) {
        case MG_SCREEN_MODE_640X448:
            *width = 640;
            *height = 448;
            break;
        case MG_SCREEN_MODE_512X480:
            *width = 512;
            *height = 480;
            break;
        case MG_SCREEN_MODE_512X416:
            *width = 512;
            *height = 416;
            break;
        case MG_SCREEN_MODE_512X448:
        default:
            mode = MG_SCREEN_MODE_512X448;
            *width = 512;
            *height = 448;
            break;
    }
    *left = -(*width >> 1);
    *top = -(*height >> 1);
    *right = *width + *left;
    *bottom = *height + *top;
    return mode;
}

#ifdef NONMATCHING
void mgInit(int screen_mode, int video_mode) {
    static signed char dimx[16] = { 10, 4, 6, 8, 12, 0, 2, 14, 7, 9, 11, 5, 3, 15, 13, 1 };
    sceDmaEnv          dma_env;
    u_long128          clear_pixels[8192];
    sceGsLoadImage     load_image;
    u_long             packed_dimx;
    int                aligned_height;
    int                buffer;
    int                i;

    font_cons = -1;
    font_draw_flag = 0;
    mgChangeLight = 1;
    sceDmaReset(1);
    sceDmaGetEnv(&dma_env);
    dma_env.notify = 0x100;
    sceDmaPutEnv(&dma_env);
    sceGsResetPath();
    mgAntialiasing = 1;
    mgDBuffID = 0;
    mgDataID = 0;
    memset(&mgRenderInfo, 0, sizeof(mgRenderInfo));
    mgVif1Packet = vifpacket;
    DmaCH1 = sceDmaGetChan(1);
    DmaCH2 = sceDmaGetChan(2);
    DmaCH8 = sceDmaGetChan(8);
    DmaCH1->chcr.TTE = 1;
    *(u_long128 *)&mgGiftagAD = 0;
    mgGiftagAD.EOP = 1;
    mgGiftagAD.NREG = 1;
    mgGiftagAD.REGS0 = SCE_GIF_PACKED_AD;
    mgScreenMode = GetScreenSize(screen_mode, &mgScreenWidth, &mgScreenHeight, &mgScreenNX, &mgScreenNY, &mgScreenMX, &mgScreenMY);
    aligned_height = mgScreenHeight;
    if (aligned_height % 32 != 0) {
        aligned_height += 32 - aligned_height % 32;
    }
    mgScreenOffx = 0x800 - mgScreenWidth / 2;
    mgScreenOffy = 0x800 - mgScreenHeight / 2;
    mgScreenDepth = 32;
    mgScreenZDepth = 32;
    mgScreenLeft = mgScreenOffx;
    mgScreenRight = mgScreenOffx + mgScreenWidth;
    mgScreenTop = mgScreenOffy;
    mgScreenBottom = mgScreenOffy + mgScreenHeight;
    sceGsResetGraph(0, SCE_GS_INTERLACE, video_mode, 0);
    for (i = 0; i < 8192; i++) {
        clear_pixels[i] = 0;
    }
    for (buffer = 0; buffer < 32; buffer++) {
        sceGsSetDefLoadImage(&load_image, buffer * 0x200, 2, SCE_GS_PSMCT32, 0, 0, 128, 256);
        FlushCache(0);
        sceGsExecLoadImage(&load_image, clear_pixels);
    }
    sceGsSetDefDBuff(&mgDBuff, SCE_GS_PSMCT32, (short)mgScreenWidth, (short)mgScreenHeight, SCE_GS_ZGEQUAL, SCE_GS_PSMZ24, 0);
    frame_buf0 = 0;
    frame_buf1 = mgScreenDepth * (mgScreenWidth * aligned_height / 2048) / 32;
    mgBackColor[0] = 0.0f;
    mgBackColor[1] = 0.0f;
    mgBackColor[2] = 0.0f;
    mgBackColor[3] = 128.0f;
    mgClearBackFlag = 1;
    mgDBuff.draw0.frame1.FBP = frame_buf1;
    mgDBuff.draw1.frame1.FBP = frame_buf0;
    mgDBuff.draw1.zbuf1.bits.zbp = frame_buf1 * 2;
    mgDBuff.draw0.zbuf1.bits.zbp = mgDBuff.draw1.zbuf1.bits.zbp;
    mgDBuff.clear0.rgbaq.bytes.red = (int)mgBackColor[0];
    mgDBuff.clear0.rgbaq.bytes.green = (int)mgBackColor[1];
    mgDBuff.clear0.rgbaq.bytes.blue = (int)mgBackColor[2];
    mgDBuff.clear0.rgbaq.bytes.alpha = (int)mgBackColor[3];
    mgDBuff.clear1.rgbaq.bytes.red = mgDBuff.clear0.rgbaq.bytes.red;
    mgDBuff.clear1.rgbaq.bytes.green = mgDBuff.clear0.rgbaq.bytes.green;
    mgDBuff.clear1.rgbaq.bytes.blue = mgDBuff.clear0.rgbaq.bytes.blue;
    mgDBuff.clear1.rgbaq.bytes.alpha = mgDBuff.clear0.rgbaq.bytes.alpha;
    *(u_long *)&mgTEX1_1 = 0x261;
    mgTEX1_2 = mgTEX1_1;
    *(u_long *)&mgTEST_1 = 0x5000B;
    mgTEST_2 = mgTEST_1;
    mgZBUF_1 = mgDBuff.draw0.zbuf1;
    mgZBUF_2 = mgDBuff.draw0.zbuf1;
    *(u_long *)&mgALPHA_1 = 0x44;
    mgALPHA_2 = mgALPHA_1;
    *(u_long *)&mgTEXA_1 = 0x100400000ULL;
    mgTEXA_2 = mgTEXA_1;
    mgDrawManager.texture_manager = &mgTexManager;
    mgDrawManager.render_info = &mgRenderInfo;
    mgRenderInfo.Initialize();
    mgRenderInfo.draw_env[0].zbuf = mgZBUF_1;
    mgRenderInfo.draw_env[1].zbuf = mgZBUF_2;
    FlushCache(0);
    sceDmaSend(DmaCH1, My_dma_start0);
    sceGsSyncPath(0, 0);
    FlushCache(0);
    sceDmaSend(DmaCH1, Vu_progmain);
    sceGsSyncPath(0, 0);
    {
        u_long128 *program = mgGetVuProgPacket(MG_VU_PROG_MAIN);

        FlushCache(0);
        sceDmaSend(DmaCH1, program);
        sceGsSyncPath(0, 0);
    }
    *(volatile u_int *)0x10000010 = 0x83;
    mgFrameRate = 2;
    vcount = 0;
    over_vsync = 0;
    sceGsSyncVCallback(VSyncCallBack);
    VSyncCallBack2 = NULL;
    call_back_active = 0;
    FlushCache(0);
    sceGsSwapDBuff(&mgDBuff, 0);
    sceDmaSync(DmaCH2, 0, 0);
    mgCreateSinTable();
    for (i = 0; i < 16; i++) {
        dimx[i] = dimx[i] / 2 - 4;
    }
    packed_dimx = 0;
    for (i = 0; i < 16; i++) {
        packed_dimx |= (u_long)(dimx[i] & 0x7) << (i * 4);
    }
    mgDIMX.value = packed_dimx;
    mgDIMX.bits.dm00 = dimx[0];
    mgDIMX.bits.dm01 = dimx[1];
    mgDIMX.bits.dm02 = dimx[2];
    mgDIMX.bits.dm03 = dimx[3];
    mgDIMX.bits.dm10 = dimx[4];
    mgDIMX.bits.dm11 = dimx[5];
    mgDIMX.bits.dm12 = dimx[6];
    mgDIMX.bits.dm13 = dimx[7];
    mgDIMX.bits.dm20 = dimx[8];
    mgDIMX.bits.dm21 = dimx[9];
    mgDIMX.bits.dm22 = dimx[10];
    mgDIMX.bits.dm23 = dimx[11];
    mgDIMX.bits.dm30 = dimx[12];
    mgDIMX.bits.dm31 = dimx[13];
    mgDIMX.bits.dm32 = dimx[14];
    mgDIMX.bits.dm33 = dimx[15];
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgInit__Fii);
#endif

void mgInitVif1Packet(u_long128 *buffer0, u_long128 *buffer1, int size) {
    int remainder;

    packetbuf[0] = (u_int *)buffer0;
    packetbuf[1] = (u_int *)buffer1;
    remainder = (int)packetbuf[0] % 4;
    if (remainder != 0) {
        packetbuf[0] += 4 - remainder;
    }
    remainder = (int)packetbuf[1] % 4;
    if (remainder != 0) {
        packetbuf[1] += 4 - remainder;
    }
    sceVif1PkInit(&vifpacket[0], packetbuf[0]);
    sceVif1PkInit(&vifpacket[1], packetbuf[1]);
    sceVif1PkReset(&vifpacket[0]);
    sceVif1PkReset(&vifpacket[1]);
    packet_size = size;
}

void mgSetPacketBuffer(mgCMemory *memory0, mgCMemory *memory1) {
    packet_buf[0] = *memory0;
    packet_buf[1] = *memory1;
    packet_buf[0].stack_used = 0;
    packet_buf[0].lock = 0;
    packet_buf[1].stack_used = 0;
    packet_buf[1].lock = 0;
}

void mgSetDataBuffer(mgCMemory *memory0, mgCMemory *memory1, int skip_head) {
    u_long128 *buffer;
    int        size;

    buffer = memory0->stAllocTest(1);
    size = memory0->stack_size - memory0->stack_used;
    if (skip_head != 0) {
        buffer += 1024;
        size -= 2048;
    }
    data_buf[0].stSetBuffer(buffer, size);
    buffer = memory1->stAllocTest(1);
    size = memory1->stack_size - memory1->stack_used;
    if (skip_head != 0) {
        buffer += 1024;
        size -= 2048;
    }
    data_buf[1].stSetBuffer(buffer, size);
    data_buf[0].stack_used = 0;
    data_buf[0].lock = 0;
    data_buf[1].stack_used = 0;
    data_buf[1].lock = 0;
}

mgCMemory *mgGetDataBuffer() {
    return &data_buf[mgDataID];
}

int mgGetTopVRAMAddress() {
    int height;
    int pixels;

    height = mgScreenHeight;
    if (height % 32 != 0) {
        height += 32 - height % 32;
    }
    pixels = mgScreenWidth * height;
    int blocks = mgScreenDepth * pixels * 2 / 256 / 8;
    blocks += mgScreenZDepth * pixels / 256 / 8;
    return blocks;
}

float mgGetNowFrameRate(void) {
    return (float)mgFrameRate;
}

void mgBeginFrame(mgCDrawManager *manager) {
    u_char alpha;
    int red;
    if (manager == NULL) {
        mgDrawManager.texture_manager = &mgTexManager;
        manager = &mgDrawManager;
        mgDrawManager.render_info = &mgRenderInfo;
    }

    *(volatile u_int *)0x10000000 = 0;
    h_count = *(volatile u_int *)0x10000000;

    red = (int)mgBackColor[0];
    mgDBuff.clear0.rgbaq.bytes.red = red;
    int green = (int)mgBackColor[1];
    mgDBuff.clear0.rgbaq.bytes.green = green;
    int blue = (int)mgBackColor[2];
    mgDBuff.clear0.rgbaq.bytes.blue = blue;
    alpha = (int)mgBackColor[3];
    mgDBuff.clear1.rgbaq.bytes.red = red;
    mgDBuff.clear1.rgbaq.bytes.green = green;
    mgDBuff.clear1.rgbaq.bytes.blue = blue;
    mgDBuff.clear0.rgbaq.bytes.alpha = alpha;
    mgDBuff.clear1.rgbaq.bytes.alpha = alpha;
    mgBeginPacket(manager);
    *(u_long128 *)&mgGiftagAD = 0;
    mgGiftagAD.EOP = 1;
    mgGiftagAD.NREG = 1;
    mgGiftagAD.REGS0 = SCE_GIF_PACKED_AD;
    sceVif1PkCnt(mgVif1Packet, 0);
    sceVif1PkOpenDirectCode(mgVif1Packet, 0);
    sceVif1PkOpenGifTag(mgVif1Packet, *(u_long128 *)&mgGiftagAD);
    sceVif1PkAddGsAD(mgVif1Packet, SCE_GS_SCANMSK, 0);
    sceVif1PkAddGsAD(mgVif1Packet, SCE_GS_TEXFLUSH, 0);
    sceVif1PkCloseGifTag(mgVif1Packet);
    sceVif1PkCloseDirectCode(mgVif1Packet);
    mgSetPkTextureRepeat(1);
    sceGsFrame *frame;
    if (mgDBuffID != 0) {
        frame = &mgDBuff.draw0.frame1;
    } else {
        frame = &mgDBuff.draw1.frame1;
    }
    mgFRAME_1 = *frame;
    mgSetPkFrameBuffer(-1, -1, -1, -1);
    mgSetPkClearScreen(
        mgDBuff.clear0.rgbaq.bytes.red, mgDBuff.clear0.rgbaq.bytes.green,
        mgDBuff.clear0.rgbaq.bytes.blue, mgDBuff.clear0.rgbaq.bytes.alpha);
    mgFlushRenderInfo();
}

void mgBeginPacket(mgCDrawManager *manager) {
    mgCMemory *packet_memory;
    mgCMemory *data_memory;

    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    mgVif1Packet = &vifpacket[mgDataID];
    sceVif1PkReset(mgVif1Packet);
    packet_memory = &packet_buf[mgDataID];
    packet_memory->stack_used = 0;
    packet_memory->lock = 0;
    data_memory = &data_buf[mgDataID];
    data_memory->stack_used = 0;
    data_memory->lock = 0;
    manager->packet_memory = &packet_buf[mgDataID];
    manager->data_memory = &data_buf[mgDataID];
    manager->SetSortTable(1);
}

void mgBeginDraw(mgCMemory *memory, int *block_list, mgCDrawManager *manager) {
    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    manager->BeginDraw(memory, block_list);
}

void mgEndDraw(mgCDrawManager *manager) {
    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    manager->EndDraw(mgVif1Packet);
}

void mgPreEndDraw(mgCDrawManager *manager) {
    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    manager->PreEndDraw();
}

int mgEndDrawReloadTexture(int block, mgCDrawManager *manager) {
    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    return manager->ReloadTexture(block, mgVif1Packet);
}

void mgEndDraw(int block, mgCDrawManager *manager) {
    if (manager == NULL) {
        manager = &mgDrawManager;
    }
    manager->Draw(block, mgVif1Packet);
}

void mgStoreFrameImage() {
    StoreImage(0);
}

#ifdef NONMATCHING
void mgEndFrame(mgCDrawManager *manager) {
    static int       count = 1;
    static float     cpu_ratio = 0.0f;
    static float     free_ratio = 0.0f;
    static u_long128 store_data[256];
    sceGsStoreImage  store_image;
    sceGsDispEnv    *display;
    sceGsFrame      *frame;
    float            frame_ticks;
    float            packet_free;
    float            data_free;
    int              wait_start;
    int              sample;
    int              pixel;
    int              depth;
    int              top;
    int              magnification;
    u_long           display_base;
    u_long           display_position;

    frame_ticks = (float)(mgFrameRate * 262);
    cpu_ratio = 100.0f * ((u_int)(*(volatile u_int *)0x10000000 - h_count) / frame_ticks);
    mgWaitFrame();
    wait_start = *(volatile u_int *)0x10000000;
    if (draw_performance_meter != 0) {
        packet_free = 100.0f * (float)(mgDrawManager.packet_memory->stack_size - mgDrawManager.packet_memory->stack_used) / (float)mgDrawManager.packet_memory->stack_size;
        data_free = 100.0f * (float)(mgDrawManager.data_memory->stack_size - mgDrawManager.data_memory->stack_used) / (float)mgDrawManager.data_memory->stack_size;

        mgCDrawPrim prim;

        prim.Initialize(NULL, NULL);
        prim.DepthTestEnable(0);
        prim.TextureMapEnable(0);
        prim.AlphaBlendEnable(1);
        prim.ZMask(-1);
        prim.Begin(SCE_GS_PRIM_SPRITE);
        prim.Color(128, 128, 128, 128);
        top = mgScreenHeight - 40;
        if (free_ratio <= 0.0f) {
            prim.Color(255, 0, 0, 64);
        } else {
            prim.Color(128, 128, 128, 64);
        }
        prim.Vertex(mgScreenWidth, top, 0);
        prim.Vertex(mgScreenWidth - 100, top + 8, 0);
        prim.Vertex(mgScreenWidth, top + 12, 0);
        prim.Vertex(mgScreenWidth - 100, top + 16, 0);
        if (cpu_ratio > 100.0f) {
            prim.Color(255, 0, 0, 64);
        } else if (cpu_ratio > 50.0f) {
            prim.Color(0, 64, 64, 64);
        } else {
            prim.Color(0, 0, 128, 64);
        }
        prim.Vertex(mgScreenWidth, top, 0);
        prim.Vertex((float)mgScreenWidth - cpu_ratio, (float)(top + 8), 0.0f);
        prim.Vertex((float)mgScreenWidth - (100.0f - free_ratio), (float)(top + 12), 0.0f);
        prim.Vertex(mgScreenWidth - 100, top + 16, 0);
        prim.Color(0, 128, 0, 64);
        prim.Vertex(mgScreenWidth, top + 20, 0);
        prim.Vertex((float)mgScreenWidth - packet_free, (float)(top + 24), 0.0f);
        prim.Vertex(mgScreenWidth, top + 28, 0);
        prim.Vertex((float)mgScreenWidth - data_free, (float)(top + 32), 0.0f);
        prim.End();
    }
    mgEndPacket(NULL);
    for (sample = 0; sample < 4; sample++) {
        if (mgPickZBuff[sample].enable != 0) {
            if (mgPickZBuff[sample].x < 4 || mgScreenWidth - 4 < mgPickZBuff[sample].x || mgPickZBuff[sample].y < 4 || mgScreenHeight - 4 < mgPickZBuff[sample].y) {
                mgPickZBuff[sample].z = -1;
            } else {
                sceGsSetDefStoreImage(&store_image, mgZBUF_1.bits.zbp * 2048 / 64, mgScreenWidth / 64, 0x30, mgPickZBuff[sample].x - 4, mgPickZBuff[sample].y - 4, 8, 8);
                FlushCache(0);
                sceGsExecStoreImage(&store_image, store_data);
                sceGsSyncPath(0, 0);
                depth = ((u_int *)store_data)[0] & 0xFFFFFF;
                for (pixel = 0; pixel < 64; pixel++) {
                    if ((int)(((u_int *)store_data)[pixel] & 0xFFFFFF) < depth) {
                        depth = ((u_int *)store_data)[pixel] & 0xFFFFFF;
                    }
                }
                mgPickZBuff[sample].z = depth;
            }
        }
    }
    over_vsync = 0;
    if (vcount - old_vcount >= mgFrameRate) {
        over_vsync = 1;
    }
    WaitVSync(old_vcount, mgFrameRate);
    old_vcount = vcount;
    if (capture_on != 0) {
        if (mgFrameRate == 1) {
            if (cap_ture_cnt % 2 != 0) {
                StoreImage(0);
            }
        } else {
            StoreImage(0);
        }
        cap_ture_cnt++;
    }
    capture_on = 0;
    if (font_draw_flag != 0 && font_cons >= 0) {
        if (font_cons >= 0) {
            sceDevConsAttribute(font_cons, 7);
        }
        sceDevConsDraw(font_cons);
    }
    font_draw_flag = 0;
    display = mgDBuffID != 0 ? &mgDBuff.disp1 : &mgDBuff.disp0;
    if (mgAntialiasing != 0) {
        display->pmode = 0x7F23;
    } else {
        *(volatile u_long *)0x12000000 = 0xFF23;
        display->pmode = 0xFF23;
    }
    display->bgcolor = 0;
    display->smode2 = 1;
    frame = mgDBuffID != 0 ? &mgDBuff.draw1.frame1 : &mgDBuff.draw0.frame1;
    magnification = 3;
    if (mgScreenWidth == 512) {
        magnification = 4;
    }
    display_base = frame->FBP | (frame->FBW << 9) | (frame->PSM << 15);
    display_position = 0x290 | ((u_long)((524 - mgScreenHeight) / 2 + 72) << 12) | ((u_long)magnification << 23);
    display->dispfb = display_base;
    *(u_long *)&display->display = display_position | ((u_long)(mgScreenWidth * (magnification + 1) - 1) << 32) | ((u_long)(mgScreenHeight - 1) << 44);
    FlushCache(0);
    sceGsSwapDBuff(&mgDBuff, mgDBuffID);
    sceDmaSync(DmaCH2, 0, 0);
    display_base = frame->FBP | (frame->FBW << 9) | (frame->PSM << 15);
    *(volatile u_long *)0x12000070 = display_base | ((u_long)0x800 << 32);
    *(volatile u_long *)0x12000080 = display_position | ((u_long)(mgScreenWidth * (magnification + 1) - 1) << 32) | ((u_long)(mgScreenHeight - 2) << 44);
    *(volatile u_long *)0x12000090 = display_base;
    *(volatile u_long *)0x120000A0 = display_position | ((u_long)(mgScreenWidth * (magnification + 1) - 1) << 32) | ((u_long)(mgScreenHeight - 2) << 44);
    mgNowFrameRate = (u_int)(*(volatile u_int *)0x10000000 - h_count) / 262.0f;
    if (mgNowFrameRate - (float)mgFrameRate > 1.0f) {
        free_ratio = 0.0f;
        mgNowFrameRate = 1.0f + (float)mgFrameRate;
    } else {
        free_ratio = 100.0f * ((u_int)(*(volatile u_int *)0x10000000 - wait_start) / frame_ticks);
    }
    count++;
    if (60 / mgFrameRate < count) {
        count = 0;
    }
    mgSendPacket(NULL);
    mgDBuffID = !mgDBuffID;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgEndFrame__FP14mgCDrawManager);
#endif

void mgSendPacket(mgCDrawManager *manager) {
    DmaCH1 = sceDmaGetChan(1);
    DmaCH1->chcr.TTE = 1;
    FlushCache(0);
    sceDmaSend(DmaCH1, mgVif1Packet->pBase);
    mgDataID = !mgDataID;
}

void mgEndPacket(mgCDrawManager *manager) {
    sceVif1PkEnd(mgVif1Packet, 0);
    sceVif1PkTerminate(mgVif1Packet);
}

void mgWaitFrame() {
    if (sceGsSyncPath(0, 0) < 0) {
        printf("******\n");
        printf("base = %x,cuur = %x\n", mgVif1Packet->pCurrent);
        Exit(-1);
    }
}

int mgDraw(mgCFrame *frame) {
    if (frame != NULL) {
        return frame->Draw();
    }
    return 0;
}

int mgDrawDirect(mgCFrame *frame) {
    int size;

    if (frame == NULL) {
        return 0;
    }
    sceVif1PkTerminate(mgVif1Packet);
    size = frame->Draw(mgVif1Packet->pCurrent);
    sceVif1PkReserve(mgVif1Packet, size * 4);
    return size;
}

int mgDrawDirect(mgCVisual *visual, float (*matrix)[4]) {
    int size;

    if (visual == NULL) {
        return 0;
    }
    sceVif1PkTerminate(mgVif1Packet);
    size = visual->Draw(mgVif1Packet->pCurrent, matrix, 0);
    sceVif1PkReserve(mgVif1Packet, size * 4);
    return size;
}

void mgDrawDirectStart(void) {
    sceVif1PkTerminate(mgVif1Packet);
    ddraw_size = 0;
}

int mgDrawDirect2(mgCFrame *frame) {
    int size;

    if (frame == NULL) {
        return 0;
    }
    size = frame->Draw(mgVif1Packet->pCurrent + ddraw_size * 4);
    ddraw_size += size;
    return size;
}

void mgDrawDirectEnd(void) {
    if ((s32)ddraw_size > 0) {
        sceVif1PkReserve(mgVif1Packet, ddraw_size * 4);
    }
}

int mgGetDrawRect(mgCFrame *frame, mgVu0FBOX *rect) {
    if (frame != NULL) {
        return frame->GetDrawRect(rect, NULL);
    }
    return 0;
}

void mgBeginDrawShadow(mgCTexture *shadow, mgCTexture *unused) {
    int width;
    int height;

    if (shadow != NULL) {
        width = shadow->width;
        height = shadow->height;
        if (width % 64 != 0) {
            width += 64 - width % 64;
        }
        if (height % 64 != 0) {
            height += 64 - height % 64;
        }
        mgSetPkFrameBuffer(shadow->tex0.TBP0 / 32, width, height, shadow->tex0.PSM);

        mgCDrawPrim prim;

        prim.Initialize(NULL, NULL);
        prim.DepthTestEnable(0);
        prim.AlphaTestEnable(0);
        prim.ZMask(-1);
        prim.TextureMapEnable(0);
        prim.Begin(SCE_GS_PRIM_SPRITE);
        prim.Color(0, 0, 0, 0);
        prim.Vertex(0, 0, 0);
        prim.Vertex(shadow->width, shadow->height, 0);
        prim.End();
    }
}

#ifdef NONMATCHING
void mgEndDrawShadow(mgCTexture *shadow, mgCTexture *unused) {
    if (shadow != NULL) {
        mgCTexture texture = *shadow;
        sceGsAlpha alpha;
        sceGsTexa  texa;

        texture.tex0.PSM = SCE_GS_PSMCT24;
        mgSetPkFrameBuffer(-1, -1, -1, -1);

        mgCDrawPrim prim;

        prim.Initialize(NULL, NULL);
        prim.DepthTestEnable(0);
        prim.AlphaTestEnable(1);
        prim.AlphaTest(SCE_GS_GEQUAL, 1);
        prim.AlphaBlendEnable(1);
        prim.ZMask(-1);
        prim.TextureMapEnable(1);
        prim.Bilinear(1);
        prim.Begin(SCE_GS_PRIM_SPRITE);
        prim.Color(128, 128, 128, 128);
        prim.Texture(&texture);
        alpha.bits.fix = 64;
        alpha.bits.a = SCE_GS_ALPHA_ZERO;
        alpha.bits.b = SCE_GS_ALPHA_CD;
        alpha.bits.c = SCE_GS_ALPHA_AS;
        alpha.bits.d = SCE_GS_ALPHA_CD;
        prim.Direct(SCE_GS_ALPHA_1, alpha.value);
        texa.TA0 = 48;
        texa.TA1 = 128;
        texa.AEM = 1;
        prim.Direct(SCE_GS_TEXA, *(u_long *)&texa);
        prim.TextureCrd(1, 1);
        prim.Vertex(0, 0, 0);
        prim.TextureCrd(texture.width - 1, texture.height - 1);
        prim.Vertex(mgScreenWidth, mgScreenHeight, 0);
        texa.TA0 = 0;
        texa.TA1 = 128;
        texa.AEM = 1;
        prim.Direct(SCE_GS_TEXFLUSH, 0);
        prim.Direct(SCE_GS_TEXA, *(u_long *)&texa);
        prim.End();
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgEndDrawShadow__FP10mgCTextureP10mgCTexture);
#endif

void mgSetRenderInfo(float projection, float near_z, float far_z) {
    mgRenderInfo.SetRenderInfo(projection, mgScreenWidth, mgScreenHeight, near_z, far_z, mgScreenZDepth, 2096.0f / (3.0f * (float)mgScreenWidth));
}

void mgSetProjection(float projection) {
    mgSetRenderInfo(projection, mgRenderInfo.clip_min[2], mgRenderInfo.clip_max[2]);
    mgSetViewMatrix(mgRenderInfo.view, mgRenderInfo.camera_pos);
}

float mgGetProjection() {
    return mgRenderInfo.projection;
}

void mgSetBackGround(float *color) {
    sceVu0CopyVector(mgBackColor, color);
}

void mgSetBackGround(float r, float g, float b, float a) {
    sceVu0FVECTOR color = {};

    color[0] = r;
    color[1] = g;
    color[2] = b;
    color[3] = a;
    mgSetBackGround(color);
}

void mgInitLighting() {
    mgRenderInfo.InitLighting();
    mgChangeLight = 1;
}

void mgInitActiveLighting() {
    mgRenderInfo.InitActiveLighting();
    mgChangeLight = 1;
}

int mgActiveLighting(int set, int copy) {
    mgChangeLight = 1;
    return mgRenderInfo.ActiveLighting(set, copy);
}

void mgSetLight(float (*direction)[4], float (*color)[4]) {
    mgChangeLight = 1;
    mgRenderInfo.SetLight(direction, color);
}

void mgGetLight(float (*direction)[4], float (*color)[4]) {
    mgRenderInfo.GetLight(direction, color);
}

void mgSetLight(int light, float *direction, float *color) {
    mgChangeLight = 1;
    mgRenderInfo.SetLight(light, direction, color);
}

void mgSetAmbient(float *ambient) {
    mgChangeLight = 1;
    mgRenderInfo.SetAmbient(ambient);
}

void mgGetAmbient(float *ambient) {
    mgRenderInfo.GetAmbient(ambient);
}

void mgSetPlight(int light, float *position, float *color, float intensity, float range) {
    mgChangeLight = 1;
    mgRenderInfo.SetPlight(light, position, color, intensity, range);
}

void mgSetPlight(int light, mgPOINT_LIGHT *point_light) {
    mgChangeLight = 1;
    mgRenderInfo.SetPlight(light, point_light);
}

void mgGetPlight(int light, mgPOINT_LIGHT *point_light) {
    mgRenderInfo.GetPlight(light, point_light);
}

void mgResetPlight() {
    int light;

    mgChangeLight = 1;
    for (light = 0; light < 4; light++) {
        mgRenderInfo.SetPlight(light, (mgPOINT_LIGHT *)NULL);
    }
}

void mgSetViewMatrix(float (*view)[4], float *position) {
    mgRenderInfo.SetViewMatrix(view, position);
}

void mgSetDropShadowMatrix(float *light, float *position, float *normal) {
    mgRenderInfo.SetDropShadowMatrix(light, position, normal);
}

void mgFogEnable(s32 enabled) {
    mgRenderInfo.FogEnable(enabled);
}

s32 mgGetFogEnable(void) {
    return mgRenderInfo.GetFogEnable();
}

void mgPlightEnable(s32 enabled) {
    mgRenderInfo.PlightEnable(enabled);
}

s32 mgGetPlightEnable(void) {
    return mgRenderInfo.GetPlightEnable();
}

void mgSetFogParam(float near_dist, float far_dist, u8 r, u8 g, u8 b, float far_value, float near_value) {
    mgRenderInfo.SetFogParam(near_dist, far_dist, r, g, b, far_value, near_value);
}

void mgSetFogParam(mgFOG_PARAM *fog) {
    mgRenderInfo.SetFogParam(fog->near_dist, fog->far_dist, fog->r, fog->g, fog->b,
                             fog->far_value, fog->near_value);
}

#ifdef NONMATCHING
void mgGetFogParam(mgFOG_PARAM *fog) {
    fog->near_dist = mgRenderInfo.fog.near_dist;
    fog->far_dist = mgRenderInfo.fog.far_dist;
    fog->r = mgRenderInfo.fog.r;
    fog->g = mgRenderInfo.fog.g;
    fog->b = mgRenderInfo.fog.b;
    fog->unk_b = mgRenderInfo.fog.unk_b;
    fog->offset = mgRenderInfo.fog.offset;
    fog->far_value = mgRenderInfo.fog.far_value;
    fog->near_value = mgRenderInfo.fog.near_value;
    fog->scale = mgRenderInfo.fog.scale;
    fog->coef[0] = mgRenderInfo.fog.coef[0];
    fog->coef[1] = mgRenderInfo.fog.coef[1];
    fog->coef[2] = mgRenderInfo.fog.coef[2];
    fog->coef[3] = mgRenderInfo.fog.coef[3];
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgGetFogParam__FP11mgFOG_PARAM);
#endif

void mgSetAllScissorFlag(int flag) {
    mgRenderInfo.all_scissor = flag;
}

void mgFlushRenderInfo() {
    sceVif1Packet *vif;
    u_int *packet;

    vif = mgVif1Packet;
    sceVif1PkTerminate(vif);
    packet = vif->pCurrent;

    packet[0] = MG_DMA_CNT | 4;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = MG_VIF_UNPACK_V4_32 | (1 << MG_VIF_NUM_SHIFT) | 0x3B;
    packet[4] = *(u_int *)&mgRenderInfo.fog.offset;
    packet[5] = *(u_int *)&mgRenderInfo.fog.near_value;
    packet[6] = *(u_int *)&mgRenderInfo.fog.far_value;
    packet[7] = *(u_int *)&mgRenderInfo.fog.scale;
    packet[8] = 0;
    packet[9] = 0;
    packet[10] = 0;

    packet[11] = MG_VIF_UNPACK_V4_32 | (2 << MG_VIF_NUM_SHIFT) | 0x39;
    *(u_long128 *)&packet[12] = *(u_long128 *)mgRenderInfo.guard_max;
    *(u_long128 *)&packet[16] = *(u_long128 *)mgRenderInfo.guard_min;
    sceVif1PkReserve(vif, 20);
}

void mgSetPkTextureRepeat(int repeat) {
    sceGsClamp clamp;

    memset(&clamp, 0, sizeof(clamp));
    if (repeat == 0) {
        clamp.WMS = 1;
        clamp.WMT = 1;
    }
    mgSetPkTextureRepeat(clamp);
}

void mgSetPkTextureRepeat(sceGsClamp clamp) {
    sceVif1Packet *vif;

    vif = mgVif1Packet;
    sceVif1PkCnt(vif, 0);
    sceVif1PkOpenDirectCode(vif, 0);
    sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
    sceVif1PkAddGsAD(vif, SCE_GS_CLAMP_1, *(u_long *)&clamp);
    sceVif1PkCloseGifTag(vif);
    sceVif1PkCloseDirectCode(vif);
}

void mgSetPkFrameBuffer(mgCTexture *texture) {
    if (texture == NULL) {
        mgSetPkFrameBuffer(-1, -1, -1, -1);
        return;
    }
    mgSetPkFrameBuffer(texture->tex0.TBP0 / 32, texture->tex0.TBW * 64, texture->height, texture->tex0.PSM);
}

#ifdef NONMATCHING
void mgSetPkFrameBuffer(int fbp, int width, int height, int psm) {
    sceGsFrame     frame;
    sceGsFrame    *default_frame;
    sceGsXyOffset  offset;
    sceGsScissor   scissor;
    sceVif1Packet *vif;
    u_int         *packet;
    u_long        *registers;
    int            screen_width;
    int            screen_height;
    int            aligned_width;
    int            width_shift;
    int            height_shift;
    int            size;
    short          bpp;
    int            bit;

    default_frame = mgDBuffID != 0 ? &mgDBuff.draw0.frame1 : &mgDBuff.draw1.frame1;
    if (fbp < 0) {
        fbp = default_frame->FBP;
    }
    if (psm < 0) {
        psm = default_frame->PSM;
    }
    GetScreenSize(mgScreenMode, &screen_width, &screen_height, &mgScreenNX, &mgScreenNY, &mgScreenMX, &mgScreenMY);
    if (width < 0) {
        width = screen_width;
    }
    if (height < 0) {
        height = screen_height;
    }
    mgScreenWidth = width;
    mgScreenHeight = height;
    mgScreenNX = -width / 2;
    mgScreenNY = -height / 2;
    mgScreenMX = mgScreenNX + width;
    mgScreenMY = mgScreenNY + height;
    aligned_width = width;
    if (aligned_width % 64 != 0) {
        aligned_width += 64 - aligned_width % 64;
    }
    frame = *default_frame;
    frame.FBP = fbp;
    frame.FBW = aligned_width / 64;
    frame.PSM = psm;
    frame.bits.fbmsk = 0;
    mgFRAME_1 = frame;
    mgScreenOffx = 0x800 - width / 2;
    mgScreenOffy = 0x800 - height / 2;
    offset.OFX = (short)mgScreenOffx * 16;
    offset.OFY = (short)mgScreenOffy * 16;
    scissor.SCAX0 = 0;
    scissor.SCAX1 = width - 1;
    scissor.SCAY0 = 0;
    scissor.SCAY1 = height - 1;
    vif = mgVif1Packet;
    sceVif1PkTerminate(vif);
    packet = vif->pCurrent;
    packet[0] = MG_DMA_CNT | 7;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = MG_VIF_DIRECT | 7;
    packet[4] = MG_GIFTAG_EOP | 6;
    packet[5] = 1 << MG_GIFTAG_NREG_SHIFT;
    packet[6] = SCE_GIF_PACKED_AD;
    packet[7] = 0;
    registers = (u_long *)&packet[8];
    registers[0] = 0;
    registers[1] = SCE_GS_TEXFLUSH;
    registers[2] = frame.value;
    registers[3] = SCE_GS_FRAME_1;
    registers[4] = *(u_long *)&offset;
    registers[5] = SCE_GS_XYOFFSET_1;
    registers[6] = *(u_long *)&scissor;
    registers[7] = SCE_GS_SCISSOR_1;
    registers[8] = 0;
    registers[9] = SCE_GS_SCANMSK;
    registers[10] = 0;
    registers[11] = SCE_GS_TEXFLUSH;
    sceVif1PkReserve(vif, 32);
    bpp = 0;
    switch (psm) {
        case SCE_GS_PSMCT32:
            bpp = 32;
            break;
        case SCE_GS_PSMCT24:
            bpp = 24;
            break;
        case SCE_GS_PSMCT16:
        case SCE_GS_PSMCT16S:
            bpp = 16;
            break;
    }
    frame_tex.Initialize();
    frame_tex.width = width;
    frame_tex.height = height;
    frame_tex.bpp = bpp;
    frame_tex.vram_size = bpp * (width * height) / 8 / 256;
    frame_tex.clut_size = 0;
    frame_tex.image_blocks = frame_tex.vram_size;
    frame_tex.tex0.value = 0;
    frame_tex.tex0.TBP0 = fbp << 5;
    frame_tex.tex0.TBW = width / 64;
    frame_tex.tex0.PSM = psm;
    width_shift = 0;
    for (size = width; size >= 2; size >>= 1) {
        width_shift++;
    }
    size = 1;
    for (bit = 0; bit < width_shift; bit++) {
        size *= 2;
    }
    if (width != size) {
        width_shift++;
    }
    height_shift = 0;
    for (size = height; size >= 2; size >>= 1) {
        height_shift++;
    }
    size = 1;
    for (bit = 0; bit < height_shift; bit++) {
        size *= 2;
    }
    if (height != size) {
        height_shift++;
    }
    frame_tex.tex0.bits.tw = width_shift;
    frame_tex.tex0.bits.th = height_shift;
    frame_tex.tex0.bits.tcc = 1;
    frame_tex.tex0.bits.tfx = 0;
    *(u_long *)&frame_tex.tex1 = 0x261;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgSetPkFrameBuffer__Fiiii);
#endif

#ifdef NONMATCHING
void mgGetFrameBuffer(mgCTexture *texture) {
    *texture = frame_tex;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgGetFrameBuffer__FP10mgCTexture);
#endif

#ifdef NONMATCHING
void mgGetFrameBackBuffer(mgCTexture *texture) {
    sceGsFrame *frame;

    frame = mgDBuffID != 0 ? &mgDBuff.draw1.frame1 : &mgDBuff.draw0.frame1;
    *texture = frame_tex;
    texture->tex0.TBP0 = frame->FBP << 5;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgGetFrameBackBuffer__FP10mgCTexture);
#endif

mgCDrawEnv *mgGetpDrawEnv(int index) {
    return &mgRenderInfo.draw_env[index != 0];
}

void mgSetPkMoveImage(mgCTexture *src, mgRect<int> src_rect, mgCTexture *dst, int dst_x, int dst_y, int direction) {
    if (src == NULL || dst == NULL) {
        return;
    }
    mgSetPkMoveImage(&src->tex0, src_rect, &dst->tex0, dst_x, dst_y, direction);
}

#ifdef NONMATCHING
void mgSetPkMoveImage(sceGsTex0 *src, mgRect<int> src_rect, sceGsTex0 *dst, int dst_x, int dst_y, int direction) {
    sceVif1Packet *vif;
    int            src_width;
    int            dst_width;
    int            left;
    int            top;
    int            right;
    int            bottom;

    vif = mgVif1Packet;
    dst_width = dst->TBW * 64;
    src_width = src->TBW * 64;
    if (dst_width < 64) {
        dst_width = 64;
    }
    if (src_width < 64) {
        src_width = 64;
    }
    if (src_rect.right - src_rect.left + 1 > 0 && src_rect.bottom - src_rect.top + 1 > 0) {
        left = src_rect.left / 16;
        top = src_rect.top / 16;
        right = src_rect.right / 16;
        bottom = src_rect.bottom / 16;
        sceVif1PkCnt(vif, 0);
        sceVif1PkOpenDirectCode(vif, 0);
        sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
        sceVif1PkAddGsAD(vif, SCE_GS_BITBLTBUF, SCE_GS_SET_BITBLTBUF(src->TBP0, src_width / 64, src->PSM, dst->TBP0, dst_width / 64, dst->PSM));
        sceVif1PkAddGsAD(vif, SCE_GS_TRXPOS, SCE_GS_SET_TRXPOS(left, top, dst_x >> 4, dst_y >> 4, direction));
        sceVif1PkAddGsAD(vif, SCE_GS_TRXREG, SCE_GS_SET_TRXREG(right - left + 1, bottom - top + 1));
        sceVif1PkAddGsAD(vif, SCE_GS_TRXDIR, SCE_GS_LOCAL_LOCAL);
        sceVif1PkCloseGifTag(vif);
        sceVif1PkCloseDirectCode(vif);
        sceVif1PkCnt(vif, 0);
        sceVif1PkOpenDirectCode(vif, 0);
        sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
        sceVif1PkAddGsAD(vif, SCE_GS_TEXFLUSH, 0);
        sceVif1PkCloseGifTag(vif);
        sceVif1PkCloseDirectCode(vif);
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgSetPkMoveImage__FP9sceGsTex09mgRect_i_P9sceGsTex0iii);
#endif

void mgSetPkMoveImage(mgCTexture *src, mgRect<int> src_rect, mgCTexture *dst, mgRect<int> dst_rect, mgCDrawEnv *env) {
    if (src == NULL || dst == NULL) {
        return;
    }
    mgSetPkMoveImage(&src->tex0, src_rect, &dst->tex0, dst->height,
                     dst_rect, env);
}

#ifdef NONMATCHING
void mgSetPkMoveImage(sceGsTex0 *src, mgRect<int> src_rect, sceGsTex0 *dst, int dst_height, mgRect<int> dst_rect, mgCDrawEnv *env) {
    sceVif1Packet *vif;
    sceGsTest      test;
    sceGsZbuf      zbuf;
    sceGsAlpha     alpha;
    int            width;

    width = dst->TBW * 64;
    if (width % 64 != 0) {
        width += 64 - width % 64;
    }
    if (dst_height % 64 != 0) {
        dst_height += 64 - dst_height % 64;
    }
    mgSetPkFrameBuffer(dst->TBP0 / 32, width, dst_height, dst->PSM);
    vif = mgVif1Packet;
    sceVif1PkCnt(vif, 0);
    sceVif1PkOpenDirectCode(vif, 0);
    sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
    sceVif1PkAddGsAD(vif, SCE_GS_TEXFLUSH, 0);
    if (env != NULL) {
        sceVif1PkAddGsAD(vif, SCE_GS_TEST_1, env->test.value);
        zbuf = mgZBUF_1;
        zbuf.bits.zmsk = env->zbuf.bits.zmsk;
        sceVif1PkAddGsAD(vif, SCE_GS_ZBUF_1, *(u_long *)&zbuf);
        sceVif1PkAddGsAD(vif, SCE_GS_ALPHA_1, env->alpha.value);
    } else {
        test = mgTEST_1;
        test.bits.ate = 0;
        test.bits.zte = 1;
        test.bits.ztst = SCE_GS_ALWAYS;
        test.bits.date = 0;
        sceVif1PkAddGsAD(vif, SCE_GS_TEST_1, *(u_long *)&test);
        zbuf = mgZBUF_1;
        zbuf.bits.zmsk = 1;
        sceVif1PkAddGsAD(vif, SCE_GS_ZBUF_1, *(u_long *)&zbuf);
        alpha = mgALPHA_1;
        alpha.bits.a = SCE_GS_ALPHA_ZERO;
        alpha.bits.b = SCE_GS_ALPHA_ZERO;
        alpha.bits.c = SCE_GS_ALPHA_FIX;
        alpha.bits.d = SCE_GS_ALPHA_CS;
        sceVif1PkAddGsAD(vif, SCE_GS_ALPHA_1, *(u_long *)&alpha);
    }
    sceVif1PkAddGsAD(vif, SCE_GS_TEX1_1, 0x25);
    sceVif1PkAddGsAD(vif, SCE_GS_TEX0_1, *(u_long *)src);
    sceVif1PkAddGsAD(vif, SCE_GS_PRMODECONT, 1);
    sceVif1PkAddGsAD(vif, SCE_GS_PRIM, 0x116);
    sceVif1PkAddGsAD(vif, SCE_GS_RGBAQ, 0x80808080);
    sceVif1PkAddGsAD(vif, SCE_GS_UV, (u_long)src_rect.left | ((u_long)src_rect.top << 16));
    sceVif1PkAddGsAD(vif, SCE_GS_XYZF2, (u_long)(mgScreenOffx * 16 + dst_rect.left) | ((u_long)(mgScreenOffy * 16 + dst_rect.top) << 16));
    sceVif1PkAddGsAD(vif, SCE_GS_UV, (u_long)src_rect.right | ((u_long)src_rect.bottom << 16));
    sceVif1PkAddGsAD(vif, SCE_GS_XYZF2, (u_long)(mgScreenOffx * 16 + dst_rect.right) | ((u_long)(mgScreenOffy * 16 + dst_rect.bottom) << 16));
    sceVif1PkAddGsAD(vif, SCE_GS_TEXFLUSH, 0);
    sceVif1PkCloseGifTag(vif);
    sceVif1PkCloseDirectCode(vif);
    mgSetPkFrameBuffer(-1, -1, -1, -1);
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgSetPkMoveImage__FP9sceGsTex09mgRect_i_P9sceGsTex0i9mgRect_i_P10mgCDrawEnv);
#endif

void mgSetPkClearScreen(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    int x;
    sceVif1Packet *vif;
    vif = mgVif1Packet;
    sceVif1PkCnt(vif, 0);
    sceVif1PkOpenDirectCode(vif, 0);
    sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
    sceVif1PkAddGsAD(vif, SCE_GS_TEXFLUSH, 0);
    sceVif1PkCloseGifTag(vif);
    sceVif1PkCloseDirectCode(vif);
    sceVif1PkCnt(vif, 0);
    sceVif1PkOpenDirectCode(vif, 0);
    sceVif1PkOpenGifTag(vif, *(u_long128 *)&mgGiftagAD);
    sceGsTest test = mgTEST_1;
    test.bits.ate = 0;
    test.bits.zte = 1;
    test.bits.ztst = SCE_GS_ALWAYS;
    test.bits.date = 0;
    sceVif1PkAddGsAD(vif, SCE_GS_TEST_1, *(u_long *)&test);
    sceGsZbuf zbuf = mgZBUF_1;
    zbuf.bits.zmsk = 0;
    sceVif1PkAddGsAD(vif, SCE_GS_ZBUF_1, *(u_long *)&zbuf);
    sceGsAlpha alpha = mgALPHA_1;
    alpha.bits.a = SCE_GS_ALPHA_ZERO;
    alpha.bits.b = SCE_GS_ALPHA_ZERO;
    alpha.bits.c = SCE_GS_ALPHA_FIX;
    alpha.bits.d = SCE_GS_ALPHA_CS;
    sceVif1PkAddGsAD(vif, SCE_GS_ALPHA_1, *(u_long *)&alpha);
    sceVif1PkAddGsAD(vif, SCE_GS_TEX1_1, 1);
    sceVif1PkAddGsAD(vif, SCE_GS_PRIM, 0x146);
    sceVif1PkAddGsAD(vif, SCE_GS_RGBAQ,
                     (u_long)r | ((u_long)g << 8) | ((u_long)b << 16) |
                         ((u_long)a << 24));
    for (x = 0; x < mgScreenWidth * 16; x += 0x200) {
        int left = (mgScreenOffx << 4) + x;
        sceVif1PkAddGsAD(vif, SCE_GS_XYZF2,
                         ((u_long)(mgScreenOffy << 4) << 16) | (u_long)left);
        int right = (mgScreenOffx << 4) + x + 0x200;
        sceVif1PkAddGsAD(vif, SCE_GS_XYZF2,
                         ((u_long)((mgScreenOffy + mgScreenHeight) << 4) << 16) |
                             (u_long)right);
    }
    sceVif1PkAddGsAD(vif, SCE_GS_TEXFLUSH, 0);
    sceVif1PkCloseGifTag(vif);
    sceVif1PkCloseDirectCode(vif);
}

int mgStoreImage(mgCTexture *texture, u_long128 *buffer) {
    sceGsStoreImage image;
    int             width;

    if (texture == NULL || buffer == NULL) {
        return 0;
    }
    mgWaitFrame();
    width = texture->width / 64;
    if (width == 0) {
        width = 1;
    }
    sceGsSetDefStoreImage(&image, texture->tex0.TBP0, width, texture->tex0.PSM, 0, 0, texture->width, texture->height);
    FlushCache(0);
    sceGsExecStoreImage(&image, buffer);
    sceGsSyncPath(0, 0);
    return texture->bpp * (texture->width * texture->height);
}

#ifdef NONMATCHING
int mgStoreZBuffImage(mgRect<int> &rect, u_long128 *buffer) {
    sceGsStoreImage image;
    int             width;
    int             height;
    int             pixel;
    u_int          *depth;

    width = rect.right - rect.left + 1;
    height = rect.bottom - rect.top + 1;
    if (width % 8 != 0) {
        width = width / 8 * 8;
    }
    if (height % 8 != 0) {
        height = height / 8 * 8;
    }
    if (width == 0 || height == 0) {
        return 0;
    }
    sceGsSetDefStoreImage(&image, mgZBUF_1.bits.zbp * 2048 / 64, mgScreenWidth / 64, 0x30, rect.left, rect.top, width, height);
    FlushCache(0);
    sceGsExecStoreImage(&image, buffer);
    sceGsSyncPath(0, 0);
    depth = (u_int *)buffer;
    for (pixel = 0; pixel < width * height; pixel++) {
        depth[pixel] &= 0xFFFFFF;
    }
    return width * height / 4;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgStoreZBuffImage__FR9mgRect_i_P1);
#endif

float mgConvZBuffToDist(unsigned int z) {
    return mgRenderInfo.view_screen[3][2] / ((float)z - mgRenderInfo.view_screen[2][2]);
}

mgCTexture *mgGetTextureZ(int index) {
    if (index < 0 || index > 1) {
        return NULL;
    }
    return &fixz_tex[index];
}

/**
 * Tests GS coordinates and view depth against the drawing range and clip planes.
 */
static int prim_clip_check(float *position) {
    mgRENDER_INFO *info = &mgRenderInfo;
    if (position[0] < 0.0f || position[0] > 4095.0f) {
        return 0;
    }
    if (position[1] < 0.0f || position[1] > 4095.0f) {
        return 0;
    }
    float z = position[3];
    if (z < info->clip_min[2] || z > info->clip_max[2]) {
        return 0;
    }
    return 1;
}

int mgTransWorldPrim(int *prim, float *position) {
    sceVu0FVECTOR projected;
    float         reciprocal;

    sceVu0ApplyMatrix(projected, mgRenderInfo.world_screen, position);
    reciprocal = 1.0f / projected[3];
    projected[0] *= reciprocal;
    projected[1] *= reciprocal;
    projected[2] *= reciprocal;
    prim[0] = (int)(16.0f * projected[0]);
    prim[1] = (int)(16.0f * projected[1]);
    prim[2] = (int)projected[2];
    prim[3] = 0;
    return prim_clip_check(projected);
}

int mgTransWorldScreen(int *screen, float *position) {
    int visible;

    visible = mgTransWorldPrim(screen, position);
    screen[0] -= mgScreenOffx * 16;
    screen[1] -= mgScreenOffy * 16;
    return visible;
}

int mgTransViewPrim(int *prim, float *position) {
    sceVu0FVECTOR projected;
    float         reciprocal;

    sceVu0ApplyMatrix(projected, mgRenderInfo.view_screen, position);
    reciprocal = 1.0f / projected[3];
    projected[0] *= reciprocal;
    projected[1] *= reciprocal;
    projected[2] *= reciprocal;
    prim[0] = (int)(16.0f * projected[0]);
    prim[1] = (int)(16.0f * projected[1]);
    prim[2] = (int)projected[2];
    prim[3] = 0;
    return prim_clip_check(projected);
}

void mgTransWorldView(float *view, float *position) {
    sceVu0ApplyMatrix(view, mgRenderInfo.view, position);
}

int mgTransZPrim(float z) {
    sceVu0FVECTOR position = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0IVECTOR prim;

    position[2] = z;
    mgTransViewPrim(prim, position);
    return prim[2];
}

float mgGetDistFromCamera(float *position) {
    return mgDistVector(position, mgRenderInfo.camera_pos);
}

void mgGetDirFromCamera(float *direction, float *position) {
    sceVu0SubVector(direction, position, mgRenderInfo.camera_pos);
}

void mgGetCameraPos(float *position) {
    *(u_long128 *)position = *(u_long128 *)mgRenderInfo.camera_pos;
}

void mgGetCameraPose(float (*pose)[4]) {
    *(u_long128 *)pose[0] = *(u_long128 *)mgRenderInfo.camera_pose[0];
    *(u_long128 *)pose[1] = *(u_long128 *)mgRenderInfo.camera_pose[1];
    *(u_long128 *)pose[2] = *(u_long128 *)mgRenderInfo.camera_pose[2];
    *(u_long128 *)pose[3] = *(u_long128 *)mgRenderInfo.camera_pose[3];
}

#ifdef NONMATCHING
int mgTransWorldPrim3DSprite(int *top_left, int *bottom_right, float *position, float width, float height, int unused) {
    sceVu0FVECTOR projected;
    sceVu0FVECTOR first;
    sceVu0FVECTOR second;
    float         scaled_width;
    float         scaled_height;
    float         reciprocal;
    float         half_width;
    float         half_height;
    int           visible;

    scaled_width = width * mgRenderInfo.view_screen[0][0];
    scaled_height = height * mgRenderInfo.view_screen[1][1];
    sceVu0ApplyMatrix(projected, mgRenderInfo.world_screen, position);
    if (projected[3] < 1.0f) {
        return 0;
    }
    reciprocal = 1.0f / projected[3];
    projected[0] *= reciprocal;
    projected[1] *= reciprocal;
    projected[2] *= reciprocal;
    sceVu0CopyVector(first, projected);
    sceVu0CopyVector(second, projected);
    half_width = 0.5f * (scaled_width * reciprocal);
    first[0] -= half_width;
    half_height = 0.5f * (scaled_height * reciprocal);
    first[1] -= half_height;
    second[0] += half_width;
    second[1] += half_height;
    top_left[0] = (int)(16.0f * first[0]);
    top_left[1] = (int)(16.0f * first[1]);
    top_left[2] = (int)first[2];
    top_left[3] = 0;
    bottom_right[0] = (int)(16.0f * second[0]);
    bottom_right[1] = (int)(16.0f * second[1]);
    bottom_right[2] = (int)second[2];
    bottom_right[3] = 0;
    visible = prim_clip_check(first);
    return visible & prim_clip_check(second);
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", mgTransWorldPrim3DSprite__FPiPiPfffi);
#endif

/**
 * Tests whether an identifier selects an available built-in or user microprogram.
 */
static int CheckVuProgID(int id) {
    if (id < MG_VU_PROG_USER) {
        if (id <= -1) {
            return 0;
        }
        if (id >= 3) {
            return 0;
        }
    } else {
        if (id < MG_VU_PROG_USER) {
            return 0;
        }
        if (id >= user_prog_num + MG_VU_PROG_USER) {
            return 0;
        }
        if (user_prog_adr == NULL) {
            return 0;
        }
        if (user_prog_adr[id - MG_VU_PROG_USER] == NULL) {
            return 0;
        }
    }
    return 1;
}

u_long128 *mgGetVuProgPacket(int id) {
    if (CheckVuProgID(id) == 0) {
        return NULL;
    }
    if (id < MG_VU_PROG_USER) {
        return prog_adr[id];
    }
    return user_prog_adr[id - MG_VU_PROG_USER];
}

int mgSendVuProg(unsigned int *packet, int id) {
    u_long128 *program;
    int        size;

    if (CheckVuProgID(id) == 0) {
        now_prog_id = -1;
        return 0;
    }
    size = 0;
    if (id != now_prog_id) {
        program = mgGetVuProgPacket(id);
        packet[0] = MG_DMA_CALL;
        packet[1] = (u_int)program;
        packet[2] = 0;
        packet[3] = 0;
        size = 4;
        now_prog_id = id;
        return 4;
    }
    return size;
}

void mgSetUserVuProg(u_long128 **table, int count) {
    user_prog_adr = table;
    user_prog_num = count;
}

void mgSetUserVuProgAdr(int index, u_long128 *packet) {
    if (index < 0 || index >= user_prog_num) {
        return;
    }
    user_prog_adr[index] = packet;
}

#ifdef NONMATCHING
/**
 * Saves a selected frame buffer as a numbered 24-bit TGA on the host device.
 */
static void StoreImage(int front_buffer) {
    static int image_num = 0;
    u_char     tga[18] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0 };
    char       device[128];
    char       filename[128];
    char      *character;
    u_char    *row;
    u_char     red;
    int        file;
    int        y;
    int        pixel;
    int        output;

    tga[12] = mgScreenWidth;
    tga[14] = mgScreenHeight;
    tga[13] = mgScreenWidth >> 8;
    tga[15] = mgScreenHeight >> 8;
    strcpy(device, "host0:");
    sprintf(filename, "%si%5d.tga", device, image_num++);
    for (character = filename; *character != '\0'; character++) {
        if (*character == ' ') {
            *character = '0';
        }
    }
    file = sceOpen(filename, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);

    mgCTexture texture;

    if (front_buffer == 0) {
        mgGetFrameBackBuffer(&texture);
    } else {
        mgGetFrameBuffer(&texture);
    }
    sceGsSetDefStoreImage(&gs_simage, texture.tex0.TBP0, mgScreenWidth / 64, SCE_GS_PSMCT32, 0, 0, mgScreenWidth, mgScreenHeight);
    FlushCache(0);
    sceGsExecStoreImage(&gs_simage, (u_long128 *)0x2100000);
    sceGsSyncPath(0, 0);
    sceWrite(file, tga, 18);
    for (y = 0; y < mgScreenHeight; y++) {
        row = (u_char *)((u_long128 *)0x2100000 + mgScreenWidth * (mgScreenHeight - y - 1) / 4);
        for (pixel = 0, output = 0; pixel < mgScreenWidth * 4; pixel += 4, output += 3) {
            red = row[pixel];
            row[pixel] = row[pixel + 2];
            row[pixel + 2] = red;
            row[output] = row[pixel];
            row[output + 1] = row[pixel + 1];
            row[output + 2] = row[pixel + 2];
        }
        row = (u_char *)((u_long128 *)0x2100000 + mgScreenWidth * (mgScreenHeight - y - 1) / 4);
        sceWrite(file, row, mgScreenWidth * 3);
    }
    sceClose(file);
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mglib", StoreImage__Fi);
#endif

int mgInitFont() {
    sceDevConsInit();
    font_cons = sceDevConsOpen((mgScreenOffx + 8) * 16, (mgScreenOffy + 8) * 16, 40, 24);
    return font_cons;
}

void mgCloseFont() {
    if (font_cons >= 0) {
        sceDevConsClose(font_cons);
    }
    font_draw_flag = 0;
}


// Static initialiser (.init)
// Produced by mgRenderInfo, mgTexManager, mgDrawManager, packet_buf, data_buf, frame_tex and fixz_tex.


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", dimx_281__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_1389__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", prog_adr__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_1538__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_715__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_716__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_1568__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", at_1569__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", font_cons__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", rot_priority__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mglib", now_prog_id__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(mgAntialiasing, 0x4);
INCLUDE_BSS(mgFrameRate, 0x4);
INCLUDE_BSS(mgNowFrameRate, 0x4);
INCLUDE_BSS(DmaCH1, 0x4);
INCLUDE_BSS(DmaCH2, 0x4);
INCLUDE_BSS(DmaCH8, 0x4);
INCLUDE_BSS(mgVif1Packet, 0x4);
INCLUDE_BSS(mgClearBackFlag, 0x4);
INCLUDE_BSS(mgScreenMode, 0x4);
INCLUDE_BSS(mgScreenWidth, 0x4);
INCLUDE_BSS(mgScreenHeight, 0x4);
INCLUDE_BSS(mgScreenNX, 0x4);
INCLUDE_BSS(mgScreenNY, 0x4);
INCLUDE_BSS(mgScreenMX, 0x4);
INCLUDE_BSS(mgScreenMY, 0x4);
INCLUDE_BSS(mgScreenOffx, 0x4);
INCLUDE_BSS(mgScreenOffy, 0x4);
INCLUDE_BSS(mgScreenDepth, 0x4);
INCLUDE_BSS(mgScreenZDepth, 0x4);
INCLUDE_BSS(mgScreenLeft, 0x4);
INCLUDE_BSS(mgScreenRight, 0x4);
INCLUDE_BSS(mgScreenTop, 0x4);
INCLUDE_BSS(mgScreenBottom, 0x4);
INCLUDE_BSS(VSyncField, 0x8);
INCLUDE_BSS(mgTEX1_1, 0x8);
INCLUDE_BSS(mgTEX1_2, 0x8);
INCLUDE_BSS(mgTEST_1, 0x8);
INCLUDE_BSS(mgTEST_2, 0x8);
INCLUDE_BSS(mgZBUF_1, 0x8);
INCLUDE_BSS(mgZBUF_2, 0x8);
INCLUDE_BSS(mgALPHA_1, 0x8);
INCLUDE_BSS(mgALPHA_2, 0x8);
INCLUDE_BSS(mgTEXA_1, 0x8);
INCLUDE_BSS(mgTEXA_2, 0x8);
INCLUDE_BSS(mgFRAME_1, 0x8);
INCLUDE_BSS(mgDBuffID, 0x4);
INCLUDE_BSS(mgDataID, 0x4);
INCLUDE_BSS(mgChangeLight, 0x8);
INCLUDE_BSS(packetbuf, 0x8);
INCLUDE_BSS(packet_size, 0x4);
INCLUDE_BSS(frame_buf0, 0x4);
INCLUDE_BSS(frame_buf1, 0x4);
INCLUDE_BSS(font_draw_flag, 0x4);
INCLUDE_BSS(draw_performance_meter, 0x8);
INCLUDE_BSS(mgDIMX, 0x8);
INCLUDE_BSS(vcount, 0x4);
INCLUDE_BSS(old_vcount, 0x4);
INCLUDE_BSS(over_vsync, 0x4);
INCLUDE_BSS(VSyncCallBack2, 0x4);
INCLUDE_BSS(call_back_active, 0x4);
INCLUDE_BSS(h_count, 0x4);
INCLUDE_BSS(capture_on, 0x4);
INCLUDE_BSS(cap_ture_cnt, 0x4);
INCLUDE_BSS(count_580, 0x4);
INCLUDE_BSS(init_581, 0x4);
INCLUDE_BSS(cpu_ratio_583, 0x4);
INCLUDE_BSS(init_584, 0x4);
INCLUDE_BSS(free_ratio_586, 0x4);
INCLUDE_BSS(init_587, 0x4);
INCLUDE_BSS(ddraw_size, 0x4);
INCLUDE_BSS(user_prog_adr, 0x4);
INCLUDE_BSS(user_prog_num, 0x4);
INCLUDE_BSS(image_num_1535, 0x4);
INCLUDE_BSS(init_1536, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(mgGiftagAD, 0x10);
INCLUDE_BSS(mgRenderInfo, 0x1020);
INCLUDE_BSS(mgBackColor, 0x10);
INCLUDE_BSS(mgTexManager, 0x220);
INCLUDE_BSS(mgDrawManager, 0x80);
INCLUDE_BSS(mgDBuff, 0x230);
INCLUDE_BSS(mgPickZBuff, 0x40);
INCLUDE_BSS(vifpacket, 0x40);
INCLUDE_BSS(packet_buf, 0x60);
INCLUDE_BSS(data_buf, 0x60);
INCLUDE_BSS(frame_tex, 0x70);
INCLUDE_BSS(store_data_614, 0x1000);
INCLUDE_BSS(at_863, 0x10);
INCLUDE_BSS(fixz_tex, 0xE0);
INCLUDE_BSS(gs_simage, 0xA0);
