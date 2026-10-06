#include "common.h"
#include "drawwin.hpp"
#include "nd_meswin.hpp"
#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "menudraw.hpp"

// These expansions keep the geometry next to each retail function's draw calls.
#define DrawWindowPart(prim, part, x, y, width, height, color) \
    do { \
        mgRect<int> screen; \
        mgRect<int> texture(data[part][0], data[part][1], data[part][2], data[part][3]); \
        screen.Set(x, y, width, height); \
        set2DSprite(prim, screen, texture, color); \
    } while (0)

#define DrawWindowTile(prim, x, y, width, height, tex_x, tex_y, tex_width, tex_height, color) \
    do { \
        mgRect<int> screen; \
        mgRect<int> texture(tex_x, tex_y, tex_width, tex_height); \
        screen.Set(x, y, width, height); \
        set2DSprite(prim, screen, texture, color); \
    } while (0)

#define DrawWindowRow(prim, part, win, y, height, color) \
    do { \
        DrawWindowPart(prim, part, win.x, y, 0x17, height, color); \
        DrawWindowPart(prim, part + 1, win.x + 0x17, y, win.width - 0x2E, height, color); \
        DrawWindowPart(prim, part + 2, win.x + win.width - 0x17, y, 0x17, height, color); \
    } while (0)

#define WindowFillAlpha(alpha, opaque) ((opaque) ? 0x80 : (alpha) * 0x36 / 128)

// Code (.text)
void CalcSelectCursorPos(RECT win, int *cursor_pos) {
    int left = win.x + 0x17;
    int inner_width = win.width - 0x2E;
    cursor_pos[0] = left + inner_width * 5 / 20 - 0x1E;
    cursor_pos[1] = win.y + win.height - 0x29;
    cursor_pos[2] = left + inner_width * 15 / 20 - 0x1E;
    cursor_pos[3] = cursor_pos[1];
}

void OffsetYesNoWin(RECT *win, RECT *shadow) {
    if (win->width < 0xA6) {
        win->width = 0xA6;
        shadow->width = 0xA6;
    }
}

void DrawVersatileWin_yesno(mgCDrawPrim *prim, RECT win, RGBAQ_TYPE *color, int alpha, int opaque) {
    MySetPrim(prim, 1, 0);
    int inside_width = win.width - 0x2E;
    int inside_x = win.x + 0x17;
    int right_x = win.x + win.width - 0x17;
    int top_y = win.y;
    int side_y = win.y + 0x19;
    int side_height = win.height - 0x50;
    int band_y = win.y + win.height - 0x37;
    int lower_y = win.y + win.height - 0x29;
    int bottom_y = win.y + win.height - 0x19;
    DrawWindowPart(prim, VWIN_TOP_L, win.x, top_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_TOP_C, inside_x, top_y, inside_width, 0x19, color);
    DrawWindowPart(prim, VWIN_TOP_R, right_x, top_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_SIDE_L, win.x, side_y, 0x17, side_height, color);
    int fill_alpha = 0x80;
    if (opaque == 0) {
        fill_alpha = alpha * 0x36 / 128;
    }
    FillRect(inside_x - 0xA, side_y - 9, inside_width + 0x16, side_height + 0xD,
             0, 0, 0, fill_alpha);
    DrawWindowPart(prim, VWIN_SIDE_R, right_x, side_y, 0x17, side_height, color);
    DrawWindowPart(prim, VWIN_BAND_L, win.x, band_y, 0x17, 0xE, color);
    DrawWindowPart(prim, VWIN_BAND_C, inside_x, band_y, inside_width, 0xE, color);
    DrawWindowPart(prim, VWIN_BAND_R, right_x, band_y, 0x17, 0xE, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_L, win.x, lower_y, 0x17, 0x10, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_C, inside_x, lower_y, inside_width, 0x10, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_R, right_x, lower_y, 0x17, 0x10, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_L, win.x, bottom_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_C, inside_x, bottom_y, inside_width, 0x19, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_R, right_x, bottom_y, 0x17, 0x19, color);
}

void MyMenuHelpWinDraw(mgCDrawPrim *prim, RECT win, int alpha) {
    mgRect<int> top_left_screen;
    mgRect<int> top_left_texture;
    mgRect<int> top_screen;
    mgRect<int> top_texture;
    mgRect<int> top_right_screen;
    mgRect<int> top_right_texture;
    mgRect<int> left_screen;
    mgRect<int> left_texture;
    mgRect<int> center_screen;
    mgRect<int> center_texture;
    mgRect<int> right_screen;
    mgRect<int> right_texture;
    mgRect<int> bottom_left_screen;
    mgRect<int> bottom_left_texture;
    mgRect<int> bottom_screen;
    mgRect<int> bottom_texture;
    mgRect<int> bottom_right_screen;
    mgRect<int> bottom_right_texture;
    RGBAQ_TYPE color;
    color.b = 0x80;
    color.g = 0x80;
    color.r = 0x80;
    color.a = alpha;
    int left = win.x + 0x18;
    int right = win.x + win.width - 0x18;
    int top = win.y + 0x16;
    int bottom = win.y + win.height - 0x16;
    int inner_width = win.width - 0x30;
    int inner_height = win.height - 0x2C;
    top_left_texture.Set(0xC0, 0xA6, 0x18, 0x16);
    top_left_screen.Set(win.x, win.y, 0x18, 0x16);
    set2DSprite(prim, top_left_screen, top_left_texture, &color);
    top_texture.Set(0xD8, 0xA6, 0x10, 0x16);
    top_screen.Set(left, win.y, inner_width, 0x16);
    set2DSprite(prim, top_screen, top_texture, &color);
    top_right_texture.Set(0xE8, 0xA6, 0x18, 0x16);
    top_right_screen.Set(right, win.y, 0x18, 0x16);
    set2DSprite(prim, top_right_screen, top_right_texture, &color);
    left_texture.Set(0xC0, 0xBC, 0x18, 0x14);
    left_screen.Set(win.x, top, 0x18, inner_height);
    set2DSprite(prim, left_screen, left_texture, &color);
    center_texture.Set(0xD8, 0xBC, 0x10, 0x14);
    center_screen.Set(left, top, inner_width, inner_height);
    set2DSprite(prim, center_screen, center_texture, &color);
    right_texture.Set(0xE8, 0xBC, 0x18, 0x14);
    right_screen.Set(right, top, 0x18, inner_height);
    set2DSprite(prim, right_screen, right_texture, &color);
    bottom_left_texture.Set(0xC0, 0xD0, 0x18, 0x16);
    bottom_left_screen.Set(win.x, bottom, 0x18, 0x16);
    set2DSprite(prim, bottom_left_screen, bottom_left_texture, &color);
    bottom_texture.Set(0xD8, 0xD0, 0x10, 0x16);
    bottom_screen.Set(left, bottom, inner_width, 0x16);
    set2DSprite(prim, bottom_screen, bottom_texture, &color);
    bottom_right_texture.Set(0xE8, 0xD0, 0x18, 0x16);
    bottom_right_screen.Set(right, bottom, 0x18, 0x16);
    set2DSprite(prim, bottom_right_screen, bottom_right_texture, &color);
}

#ifdef NONMATCHING
void MyMenuFloatingWinDraw(mgCDrawPrim *prim, RECT win, int point_x, int point_y,
                           RGBAQ_TYPE *frame_color, RGBAQ_TYPE *fill_color) {
    MySetPrim(prim, 1, 0);
    const int sx[3] = {win.x, win.x + 7, win.x + win.width - 7};
    const int sy[3] = {win.y, win.y + 9, win.y + win.height - 9};
    const int sw[3] = {7, win.width - 14, 7};
    const int sh[3] = {9, win.height - 18, 9};
    const int outer_tx[3] = {0xA0, 0xA7, 0xC9};
    const int inner_tx[3] = {0x70, 0x77, 0x99};
    const int ty[3] = {0, 9, 0x27};
    const int tw[3] = {7, 0x22, 7};
    const int th[3] = {9, 0x1E, 9};
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            DrawWindowTile(prim, sx[col], sy[row], sw[col], sh[row],
                           outer_tx[col], ty[row], tw[col], th[row], fill_color);
    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col) {
            if (row == 1 && col == 1) continue;
            DrawWindowTile(prim, sx[col], sy[row], sw[col], sh[row],
                           inner_tx[col], ty[row], tw[col], th[row], frame_color);
        }
    MySetPrim(prim, 4, 0);
    int x, y, tx, ty_outer, ty_inner;
    if (point_x < win.x) {
        x = win.x - 13; y = point_y - 10; tx = 0xA6; ty_outer = 0x45; ty_inner = 0x30;
    } else if (point_x > win.x + win.width) {
        x = win.x + win.width - 8; y = point_y - 10; tx = 0xBB; ty_outer = 0x45; ty_inner = 0x30;
    } else if (point_y < win.y) {
        x = point_x - 10; y = win.y - 13; tx = 0x7C; ty_outer = 0x45; ty_inner = 0x30;
    } else if (point_y > win.y + win.height) {
        x = point_x - 10; y = win.y + win.height - 8; tx = 0x91; ty_outer = 0x45; ty_inner = 0x30;
    } else return;
    DrawWindowTile(prim, x, y, 0x15, 0x15, tx, ty_outer, 0x15, 0x15, fill_color);
    DrawWindowTile(prim, x, y, 0x15, 0x15, tx, ty_inner, 0x15, 0x15, frame_color);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/drawwin", MyMenuFloatingWinDraw__FP11mgCDrawPrim4RECTiiP10RGBAQ_TYPEP10RGBAQ_TYPE);
#endif
void DrawVersatileWin_1(mgCDrawPrim *prim, RECT win, RGBAQ_TYPE *color, int alpha, int opaque) {
    mgRect<int> top_left_screen;
    mgRect<int> top_left_texture;
    mgRect<int> top_screen;
    mgRect<int> top_texture;
    mgRect<int> top_right_screen;
    mgRect<int> top_right_texture;
    mgRect<int> left_screen;
    mgRect<int> left_texture;
    mgRect<int> right_screen;
    mgRect<int> right_texture;
    mgRect<int> bottom_left_screen;
    mgRect<int> bottom_left_texture;
    mgRect<int> bottom_screen;
    mgRect<int> bottom_texture;
    mgRect<int> bottom_right_screen;
    mgRect<int> bottom_right_texture;
    MySetPrim(prim, 1, 0);
    int left = win.x + 0x17;
    int right = win.x + win.width - 0x17;
    int inner_width = win.width - 0x2E;
    int top = win.y + 0x19;
    int inner_height = win.height - 0x32;
    int bottom = win.y + win.height - 0x19;
    top_left_texture.Set(data[VWIN_TOP_L][0], data[VWIN_TOP_L][1], data[VWIN_TOP_L][2], data[VWIN_TOP_L][3]);
    top_left_screen.Set(win.x, win.y, 0x17, 0x19);
    set2DSprite(prim, top_left_screen, top_left_texture, color);
    top_texture.Set(data[VWIN_TOP_C][0], data[VWIN_TOP_C][1], data[VWIN_TOP_C][2], data[VWIN_TOP_C][3]);
    top_screen.Set(left, win.y, inner_width, 0x19);
    set2DSprite(prim, top_screen, top_texture, color);
    top_right_texture.Set(data[VWIN_TOP_R][0], data[VWIN_TOP_R][1], data[VWIN_TOP_R][2], data[VWIN_TOP_R][3]);
    top_right_screen.Set(right, win.y, 0x17, 0x19);
    set2DSprite(prim, top_right_screen, top_right_texture, color);
    left_texture.Set(data[VWIN_SIDE_L][0], data[VWIN_SIDE_L][1], data[VWIN_SIDE_L][2], data[VWIN_SIDE_L][3]);
    left_screen.Set(win.x, top, 0x17, inner_height);
    set2DSprite(prim, left_screen, left_texture, color);
    int fill_alpha = 0x80;
    if (opaque == 0) {
        fill_alpha = alpha * 0x36 / 128;
    }
    FillRect(left - 10, top - 9, inner_width + 0x16, inner_height + 0x18, 0, 0, 0,
                        fill_alpha);
    right_texture.Set(data[VWIN_SIDE_R][0], data[VWIN_SIDE_R][1], data[VWIN_SIDE_R][2], data[VWIN_SIDE_R][3]);
    right_screen.Set(right, top, 0x17, inner_height);
    set2DSprite(prim, right_screen, right_texture, color);
    bottom_left_texture.Set(data[VWIN_BOTTOM_L][0], data[VWIN_BOTTOM_L][1], data[VWIN_BOTTOM_L][2], data[VWIN_BOTTOM_L][3]);
    bottom_left_screen.Set(win.x, bottom, 0x17, 0x19);
    set2DSprite(prim, bottom_left_screen, bottom_left_texture, color);
    bottom_texture.Set(data[VWIN_BOTTOM_C][0], data[VWIN_BOTTOM_C][1], data[VWIN_BOTTOM_C][2], data[VWIN_BOTTOM_C][3]);
    bottom_screen.Set(left, bottom, inner_width, 0x19);
    set2DSprite(prim, bottom_screen, bottom_texture, color);
    bottom_right_texture.Set(data[VWIN_BOTTOM_R][0], data[VWIN_BOTTOM_R][1], data[VWIN_BOTTOM_R][2], data[VWIN_BOTTOM_R][3]);
    bottom_right_screen.Set(right, bottom, 0x17, 0x19);
    set2DSprite(prim, bottom_right_screen, bottom_right_texture, color);
}

void DrawVersatileWin_1(mgCDrawPrim *prim, RECT win, RGBAQ_TYPE *color, int alpha) {
    DrawVersatileWin_1(prim, win, color, alpha, 0);
}

void DrawVersatileWin_3(mgCDrawPrim *prim, RECT win, int select_y, RGBAQ_TYPE *color, int alpha, int opaque) {
    MySetPrim(prim, 1, 0);
    int band_top = select_y - 7;
    int band_bottom = select_y + 7;
    int inside_x = win.x + 0x17;
    int right_x = win.x + win.width - 0x17;
    int inside_width = win.width - 0x2E;
    int top_y = win.y;
    int side_y = win.y + 0x19;
    int bottom_y = win.y + win.height - 0x19;
    int side_height = band_top - (win.height + 0x19);
    int lower_height = bottom_y - band_bottom;
    DrawWindowPart(prim, VWIN_TOP_L, win.x, top_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_TOP_C, inside_x, top_y, inside_width, 0x19, color);
    DrawWindowPart(prim, VWIN_TOP_R, right_x, top_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_SIDE_L, win.x, side_y, 0x17, side_height, color);
    int fill_alpha = 0x80;
    if (opaque == 0) {
        fill_alpha = alpha * 0x36 / 128;
    }
    FillRect(inside_x - 0xA, side_y - 9, inside_width + 0x16, side_height + 0xD,
             0, 0, 0, fill_alpha);
    DrawWindowPart(prim, VWIN_SIDE_R, right_x, side_y, 0x17, side_height, color);
    DrawWindowPart(prim, VWIN_BAND_L, win.x, band_top, 0x17, 0xE, color);
    DrawWindowPart(prim, VWIN_BAND_C, inside_x, band_top, inside_width, 0xE, color);
    DrawWindowPart(prim, VWIN_BAND_R, right_x, band_top, 0x17, 0xE, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_L, win.x, band_bottom, 0x17, lower_height, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_C, inside_x, band_bottom, inside_width, lower_height, color);
    DrawWindowPart(prim, VWIN_LOWER_SIDE_R, right_x, band_bottom, 0x17, lower_height, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_L, win.x, bottom_y, 0x17, 0x19, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_C, inside_x, bottom_y, inside_width, 0x19, color);
    DrawWindowPart(prim, VWIN_LOWER_BOTTOM_R, right_x, bottom_y, 0x17, 0x19, color);
}

void DrawVersatileWin_4(mgCDrawPrim *prim, RECT win, RGBAQ_TYPE *color, int alpha, int opaque) {
    mgRect<int> top_left_screen;
    mgRect<int> top_left_texture;
    mgRect<int> top_screen;
    mgRect<int> top_texture;
    mgRect<int> top_right_screen;
    mgRect<int> top_right_texture;
    mgRect<int> left_screen;
    mgRect<int> left_texture;
    mgRect<int> right_screen;
    mgRect<int> right_texture;
    mgRect<int> bottom_left_screen;
    mgRect<int> bottom_left_texture;
    mgRect<int> bottom_screen;
    mgRect<int> bottom_texture;
    mgRect<int> bottom_right_screen;
    mgRect<int> bottom_right_texture;
    MySetPrim(prim, 1, 0);
    int left = win.x + 0x17;
    int right = win.x + win.width - 0x17;
    int inner_width = win.width - 0x2E;
    int top = win.y + 0x19;
    int inner_height = win.height - 0x32;
    int bottom = win.y + win.height - 0x19;
    top_left_texture.Set(data[VWIN_TOP4_L][0], data[VWIN_TOP4_L][1], data[VWIN_TOP4_L][2], data[VWIN_TOP4_L][3]);
    top_left_screen.Set(win.x, win.y, 0x17, 0x19);
    set2DSprite(prim, top_left_screen, top_left_texture, color);
    top_texture.Set(data[VWIN_TOP4_C][0], data[VWIN_TOP4_C][1], data[VWIN_TOP4_C][2], data[VWIN_TOP4_C][3]);
    top_screen.Set(left, win.y, inner_width, 0x19);
    set2DSprite(prim, top_screen, top_texture, color);
    top_right_texture.Set(data[VWIN_TOP4_R][0], data[VWIN_TOP4_R][1], data[VWIN_TOP4_R][2], data[VWIN_TOP4_R][3]);
    top_right_screen.Set(right, win.y, 0x17, 0x19);
    set2DSprite(prim, top_right_screen, top_right_texture, color);
    left_texture.Set(data[VWIN_SIDE_L][0], data[VWIN_SIDE_L][1], data[VWIN_SIDE_L][2], data[VWIN_SIDE_L][3]);
    left_screen.Set(win.x, top, 0x17, inner_height);
    set2DSprite(prim, left_screen, left_texture, color);
    int fill_alpha = 0x80;
    if (opaque == 0) {
        fill_alpha = alpha * 0x36 / 128;
    }
    FillRect(left - 10, top - 0xD, inner_width + 0x16, inner_height + 0x1C, 0, 0, 0,
                        fill_alpha);
    right_texture.Set(data[VWIN_SIDE_R][0], data[VWIN_SIDE_R][1], data[VWIN_SIDE_R][2], data[VWIN_SIDE_R][3]);
    right_screen.Set(right, top, 0x17, inner_height);
    set2DSprite(prim, right_screen, right_texture, color);
    bottom_left_texture.Set(data[VWIN_BOTTOM_L][0], data[VWIN_BOTTOM_L][1], data[VWIN_BOTTOM_L][2], data[VWIN_BOTTOM_L][3]);
    bottom_left_screen.Set(win.x, bottom, 0x17, 0x19);
    set2DSprite(prim, bottom_left_screen, bottom_left_texture, color);
    bottom_texture.Set(data[VWIN_BOTTOM_C][0], data[VWIN_BOTTOM_C][1], data[VWIN_BOTTOM_C][2], data[VWIN_BOTTOM_C][3]);
    bottom_screen.Set(left, bottom, inner_width, 0x19);
    set2DSprite(prim, bottom_screen, bottom_texture, color);
    bottom_right_texture.Set(data[VWIN_BOTTOM_R][0], data[VWIN_BOTTOM_R][1], data[VWIN_BOTTOM_R][2], data[VWIN_BOTTOM_R][3]);
    bottom_right_screen.Set(right, bottom, 0x17, 0x19);
    set2DSprite(prim, bottom_right_screen, bottom_right_texture, color);
}

void DrawVersatileWin_4(mgCDrawPrim *prim, RECT win, RGBAQ_TYPE *color, int alpha) {
    DrawVersatileWin_4(prim, win, color, alpha, 0);
}

void DrawDQFukidashi(mgCDrawPrim *prim, RECT win, int tail_x, int tail_y,
                     RGBAQ_TYPE *color, int tail_on, int mode) {
    int inside_x = win.x + 0x10;
    int inside_width = win.width - 0x20;
    int right_x = win.x + win.width - 0x10;
    int side_y = win.y + 0x10;
    int bottom_y = win.y + win.height - 0x10;
    int side_height = win.height - 0x20;
    MySetPrim(prim, 1, 0);
    DrawWindowTile(prim, win.x, win.y, 0x10, 0x10, 0, 0xD0, 0x10, 0x10, color);
    if (tail_on) {
        int end_x = inside_x + inside_width;
        if (tail_x - 8 < inside_x) {
            tail_x = inside_x + 8;
        }
        if (tail_x + 8 > end_x) {
            tail_x = end_x - 8;
        }
        DrawWindowTile(prim, inside_x, win.y, tail_x - 8 - inside_x, 0x10,
                       0x10, 0xD0, 0x10, 0x10, color);
        DrawWindowTile(prim, tail_x - 8, win.y - 0x10, 0x10, 0x20,
                       0x30, 0xD0, 0x10, 0x20, color);
        DrawWindowTile(prim, tail_x + 8, win.y, inside_x + inside_width - (tail_x + 8),
                       0x10, 0x10, 0xD0, 0x10, 0x10, color);
    } else {
        DrawWindowTile(prim, inside_x, win.y, inside_width, 0x10,
                       0x10, 0xD0, 0x10, 0x10, color);
    }
    DrawWindowTile(prim, right_x, win.y, 0x10, 0x10, 0x20, 0xD0, 0x10, 0x10, color);
    DrawWindowTile(prim, win.x, side_y, 0x10, side_height,
                   0, 0xE0, 0x10, 0x10, color);
    DrawWindowTile(prim, inside_x, side_y, inside_width, side_height,
                   0x10, 0xE0, 0x10, 0x10, color);
    DrawWindowTile(prim, right_x, side_y, 0x10, side_height,
                   0x20, 0xE0, 0x10, 0x10, color);
    DrawWindowTile(prim, win.x, bottom_y, 0x10, 0x10, 0, 0xF0, 0x10, 0x10, color);
    DrawWindowTile(prim, inside_x, bottom_y, inside_width, 0x10,
                   0x10, 0xF0, 0x10, 0x10, color);
    DrawWindowTile(prim, right_x, bottom_y, 0x10, 0x10, 0x20, 0xF0, 0x10, 0x10, color);
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/drawwin", data__DATA);
