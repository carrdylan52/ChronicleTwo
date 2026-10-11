#include "mg_texture.hpp"

PORT_OVERRIDE void mgCTextureManager::Initialize(int start, int end) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCTextureManager::GetRemainVRAM(int block) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE mgCTexture *mgCTextureManager::EnterTexture(int block, char *name, u_long128 **image, int width, int height, int bpp, u_long128 *clut, u_long tex1, int swizzled) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgLoadImage(u_int *packet, int base, int format, int width, u_long128 *image, int quadwords, int pos_x, int pos_y, int size_w, int size_h) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCTextureManager::ReloadTexture(int index, sceVif1Packet *packet) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCTextureManager::ReloadTexture(int block, u_int *packet) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCTextureManager::ReloadCLUT(mgCTexture *texture, sceVif1Packet *packet) {
    PS2_UNIMPLEMENTED();
}
