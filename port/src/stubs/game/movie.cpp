#include "movie.hpp"

PORT_OVERRIDE void CMovie::Load(char *name, mgCMemory **memory, int width, int height, bool with_audio, bool loop, bool init_sound) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CMovie::Play(char *path) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CMovie::Term() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CMovie::videoDecCreate(VideoDec *dec, u8 *mpeg_buffer, int mpeg_size, u_long128 *data, u_long128 *tags, int sectors, TimeStamp *ts, int ts_count) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CMovie::videoDecSetStream(VideoDec *dec, int id, int param, sceMpegCallback callback, void *user) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CMovie::videoDecDelete(VideoDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CMovie::videoDecFlush(VideoDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void stepMain(void *arg) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int videoCallback(sceMpeg *mpeg, sceMpegCbDataStr *str, void *user) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int vblankHandler(int irq) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void voBufIncCount(VoBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void setD3_CHCR(u32 chcr) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void setD4_CHCR(u32 chcr) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufCreate(ViBuf *buf, u_long128 *data, u_long128 *tags, int sectors, TimeStamp *ts, int ts_count) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufReset(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void viBufBeginPut(ViBuf *buf, u8 **area1, int *size1, u8 **area2, int *size2) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void viBufEndPut(ViBuf *buf, int count) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufAddDMA(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufStopDMA(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufRestartDMA(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufDelete(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void viBufFlush(ViBuf *buf) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufPutTs(ViBuf *buf, TimeStamp *ts) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int viBufGetTs(ViBuf *buf, TimeStamp *ts) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int strFileOpen(StrFile *file, char *path) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void strFileSeek(StrFile *file) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int strFileClose(StrFile *file) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int strFileRead(StrFile *file, void *buf, int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int audioDecCreate(AudioDec *dec, u8 *ring_buf, int ring_size, int iop_size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int audioDecDelete(AudioDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void audioDecPause(AudioDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void audioDecResume(AudioDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int audioDecSendToIOP(AudioDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int sendToIOP(int iop_addr, u8 *src, int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void changeMasterVolume(u32 volume) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void changeInputVolume(u32 volume) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void startDisplay(int field) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int switchThread() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int decBs0(VideoDec *dec) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void setImageTag(u32 *tag, void *data, int a, int width, int height) {
    PS2_UNIMPLEMENTED();
}
