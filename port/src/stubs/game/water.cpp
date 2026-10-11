#include "water.hpp"

PORT_OVERRIDE void CFireRaster::Draw(sceVu0FVECTOR position, float *scale) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CWater::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CWater::Draw(u_int *tag, float (*matrix)[4], mgCDrawManager *draw_manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_int CWater::CreatePacket(mgCDrawManager *draw_manager) {
    PS2_UNIMPLEMENTED();
}
