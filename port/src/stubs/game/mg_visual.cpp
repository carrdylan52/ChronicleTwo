#include "mg_visual.hpp"

PORT_OVERRIDE u_int *GetScrPad() {
    PS2_UNIMPLEMENTED();
}

void SendDMA(void *packet, int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int SetPointLight(u_int *packet, float (*first)[4], float (*second)[4]) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualMDT::SetMaterialRef(u_long128 *packet, mgMaterial *material, int flags) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualMDT::Draw(u_int *tag, float (*matrix)[4], mgCDrawManager *draw_manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_int mgCVisualMDT::CreatePacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_int mgCVisualFixMDT::CreatePacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualMDT::CreateFacePacket(u_int *packet, mgCFace *face) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualMDT::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualPrim::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    PS2_UNIMPLEMENTED();
}
