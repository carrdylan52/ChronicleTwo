#include "mglib.hpp"

PORT_OVERRIDE int VSyncCallBack(int field) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void WaitVSync(int start, int frames) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgInit(int screen_mode, int video_mode) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgInitVif1Packet(u_long128 *buffer_a, u_long128 *buffer_b, int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgGetTopVRAMAddress() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgBeginFrame(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgBeginPacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgEndFrame(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSendPacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgEndPacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgWaitFrame() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgDrawDirect(mgCFrame *frame) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgDrawDirect(mgCVisual *visual, float (*matrix)[4]) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgDrawDirectStart() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgDrawDirectEnd() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgEndDrawShadow(mgCTexture *shadow, mgCTexture *unused) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgFlushRenderInfo() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSetPkTextureRepeat(sceGsClamp clamp) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSetPkFrameBuffer(int fbp, int width, int height, int psm) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSetPkMoveImage(sceGsTex0 *src, mgRect<int> src_rect, sceGsTex0 *dst, int dst_x, int dst_y, int direction) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSetPkMoveImage(sceGsTex0 *src, mgRect<int> src_rect, sceGsTex0 *dst, int dst_height, mgRect<int> dst_rect, mgCDrawEnv *env) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgSetPkClearScreen(u_char red, u_char green, u_char blue, u_char alpha) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgTransWorldPrim(int *out, float *pos) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgTransWorldScreen(int *out, float *pos) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgTransViewPrim(int *out, float *pos) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgTransWorldPrim3DSprite(int *top_left, int *bottom_right, float *position, float width, float height, int unused) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_long128 *mgGetVuProgPacket(int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void StoreImage(int front_buffer) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgInitFont() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCloseFont() {
    PS2_UNIMPLEMENTED();
}
