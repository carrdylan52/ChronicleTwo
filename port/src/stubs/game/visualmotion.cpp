#include "visualmotion.hpp"

PORT_OVERRIDE int mgCVisualMotionMDT::CreateFaceMotionPacket(u_int *packet, mgCFace *face, mgCVMotionData *motion) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int mgCVisualMotionMDT::CreateExtRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *render_info) {
    PS2_UNIMPLEMENTED();
}
