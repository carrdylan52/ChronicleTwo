#include "mg_sprite.hpp"

PORT_OVERRIDE int mgC3DSprite::CreateRenderInfoPacket(u_int *dest, float (*matrix)[4], mgRENDER_INFO *render_info) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgC3DSprite::Draw(u_int *tag, float (*matrix)[4], mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgC3DSprite::BeginCreatePacket(int mode, mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgC3DSprite::CPSetDrawEnv(mgCDrawEnv *draw_env) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgC3DSprite::EndCPSprite() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE u_int mgCSprite::CreatePacket(mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCSprite::Draw(u_int *tag, float (*matrix)[4], mgCDrawManager *manager) {
    PS2_UNIMPLEMENTED();
}
