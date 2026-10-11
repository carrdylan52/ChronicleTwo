#include "mg_drawprim.hpp"

PORT_OVERRIDE void mgCDrawPrim::BeginDma() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCDrawPrim::Begin2() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCDrawPrim::End2() {
    PS2_UNIMPLEMENTED();
}

void mgCDrawPrim::Data0(float *data) {
    PS2_UNIMPLEMENTED();
}

void mgCDrawPrim::Data4(float *data) {
    PS2_UNIMPLEMENTED();
}

void mgCDrawPrim::Vertex(float *pos) {
    PS2_UNIMPLEMENTED();
}

void mgCDrawPrim::Color(float *color) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCDrawPrim::TextureCrd4(int u, int v) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgCDrawPrim::Texture(mgCTexture *source) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCDrawManager::ReloadTexture(int group, sceVif1Packet *vif_packet) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCDrawManager::Draw(int group, sceVif1Packet *vif_packet) {
    PS2_UNIMPLEMENTED();
}
