#include "mg_shadow.hpp"

PORT_OVERRIDE int mgCShadowMDT::CreateFacePacket(u_int *packet, mgCFace *face) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_int mgCShadowMDT::CreatePacket(mgCDrawManager *draw_manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCShadowMDT::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    PS2_UNIMPLEMENTED();
}
