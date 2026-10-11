#include "menudraw.hpp"

PORT_OVERRIDE void SetMenuScissor(mgRect<int> rect) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void ResetMenuScissor() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void DrawOneItem(mgCDrawPrim *prim, mgRect<float> rect, int item, int mode, MENU_PARTS_EFFECT_STRUCT1 *effect, u8 *rgba, int item_flag) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void MenuItemBrdDraw(float *pos, mgRect<int> clip_rect, int &tex_block, int a, int r, int g, int b) {
    PS2_UNIMPLEMENTED();
}
