#include "common.h"
#include "font.hpp"

#include <cstdio>
#include <cstring>

#include "dataread.hpp"
#include "mainloop.hpp"
#include "mg_drawprim.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "nd_meswin.hpp"

/**
 * Font-number lookup table loaded for the current language.
 */
static char FontTblBinBuff[FONT_TBL_BIN_SIZE];

// Code (.text)
int GetGaijiW(int code) {
    if (code >= GAIJI_CODE_TOP && code < GAIJI_CODE_END) {
        unsigned int index = code - GAIJI_CODE_TOP;
        return GaijiDataTbl[index].w;
    }
    return 0;
}

int GetGaijiH(int code) {
    if (code >= GAIJI_CODE_TOP && code < GAIJI_CODE_END) {
        unsigned int index = code - GAIJI_CODE_TOP;
        return GaijiDataTbl[index].h;
    }
    return 0;
}
RECT GetRectFontTex(int font_no, int *tex_no) {
    int font = font_no;
    if (font_no >= FONT_GAIJI_CODE_TOP && font_no < FONT_GAIJI_CODE_END) {
        if ((LanguageCode == LANG_FRENCH || LanguageCode == LANG_GERMAN || LanguageCode == LANG_ITALIAN) || LanguageCode == LANG_SPANISH) {
            font = GetFontNoFromFontGaijiCode((unsigned short)font_no);
        }
    }
    RECT rect = {0, 0, 0, 0};
    if (font < 0) {
        return rect;
    }
    if (font < FONT_TEX_PAGE_CHARS) {
        *tex_no = 0;
    } else if (font < 2 * FONT_TEX_PAGE_CHARS) {
        *tex_no = 1;
        font -= FONT_TEX_PAGE_CHARS;
    } else if (font < 3 * FONT_TEX_PAGE_CHARS) {
        *tex_no = 2;
        font -= 2 * FONT_TEX_PAGE_CHARS;
    } else if (font < 4 * FONT_TEX_PAGE_CHARS) {
        *tex_no = 3;
        font -= 3 * FONT_TEX_PAGE_CHARS;
    } else {
        return rect;
    }
    rect.x = font % FONT_TEX_COLUMNS;
    rect.y = font / FONT_TEX_COLUMNS;
    rect.x *= FONT_TEX_CHAR_W;
    rect.y *= FONT_TEX_CHAR_H;
    rect.width = FONT_TEX_CHAR_W;
    rect.height = FONT_TEX_CHAR_H;
    return rect;
}

void MySetTexMini(int tex_no, mgCDrawPrim *prim) {
    mgCTextureManager *texManager = &mgTexManager;
    if (tex_no == 0) {
        prim->Texture(texManager->GetTexture("FontTex_s_0", -1));
    } else {
        prim->Texture(texManager->GetTexture("FontTex_s_1", -1));
    }
}
RECT GetRectFontTexMini(int font_no, int *tex_no) {
    RECT rect = {0, 0, 0, 0};
    return rect;
}
// Preserve the order of character copies.
#pragma global_optimizer off
char *My_strncpy(char *dst, const char *src, unsigned int n) {
    char *out = dst;
    const char *in = src;
    u32 i = 0;
    char c;
    while (i < n) {
        if (*in == '[') {
            while ((c = *in) != ']') {
                *out = c;
                out++;
                in++;
            }
        }
        i++;
        *out = *in;
        in++;
        out++;
    }
    return dst;
}
#pragma global_optimizer reset
u8 *GetYoyakuTblTop() {
    return (u8 *)((FONT_TBL_BIN *) FontTblBinBuff)->yoyaku_tbl;
}

int LoadFontTblBin() {
    int size;
    if (LanguageCode == LANG_ENGLISH) {
        LoadFile("meswin/fonttbl_1.bin", FontTblBinBuff, &size);
    } else {
        LoadFile("meswin/fonttbl_2.bin", FontTblBinBuff, &size);
    }
    if (size > FONT_TBL_BIN_SIZE) {
        printf("****ERR\tFontTblBinBuff OVER!!!****\n");
        return 0;
    }
    return 1;
}

int GetYoyakuTblNum() {
    return ((FONT_TBL_BIN *) FontTblBinBuff)->yoyaku_num;
}

int GetKanjiTopNo() {
    return ((FONT_TBL_BIN *) FontTblBinBuff)->kanji_top_no;
}

int GetHalfFontNum() {
    return ((FONT_TBL_BIN *)FontTblBinBuff)->half_font_num;
}

int CFont::CheckKanjiFont(int font_no) {
    if (LanguageCode == LANG_CHINESE) {
        return 0;
    }
    if (GetKanjiTopNo() == 0) {
        return 0;
    }
    if (font_no < GetKanjiTopNo()) {
        return 0;
    }
    return (font_no >= GetYoyakuTblNum()) ^ 1;
}

int CFont::CheckHalfFont(int font_no) {
    if (font_no == FONT_NO_HALF_SPACE) {
        return 1;
    }
    if (LanguageCode != LANG_ENGLISH) {
        if (font_no >= FONT_NO_ALPHABETICAL_TOP && font_no < FONT_NO_FONT_GAIJI_TOP) {
            return 1;
        }
        if (font_no >= FONT_NO_FONT_GAIJI_TOP && font_no < FONT_NO_FONT_GAIJI_END) {
            return 0;
        }
    }
    if (font_no < 0) {
        return 0;
    }
    return (font_no >= GetHalfFontNum()) ^ 1;
}

void CFont::SetDrawSize(s32 width, s32 height) {
    draw_w = width;
    draw_h = height;
}

void CFont::SetClearance(s32 width, s32 height) {
    clearance_w = width;
    clearance_h = height;
}

void CFont::SetPos(s32 x, s32 y) {
    pos_x = x;
    pos_y = y;
}

void CFont::SetColor(s32 r, s32 g, s32 b, s32 a) {
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}

void CFont::SetColor(RGBAQ_TYPE color) {
    this->color.r = color.r;
    this->color.g = color.g;
    this->color.b = color.b;
    this->color.a = color.a;
}

void CFont::SetColor(u32 packed_color) {
    RGBAQ_TYPE color = RgbqToUint(packed_color);
    SetColor(color);
}

void CFont::SetFuchi(s32 style) {
    fuchi = style;
}

void CFont::SetStr(char *str) {
    memset(this->str, 0, sizeof(this->str));
    if (strlen(str) >= sizeof(this->str)) {
        printf("ERR:\x95\xB6\x8E\x9A\x90\x94\x82\xAA\x91\xBD\x82\xB7\x82\xAC\x82\xDC\x82\xB7\x81\x42\n");
        return;
    }
    strcpy(this->str, str);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", GetGaijiFontNo__FPc);
int GetGaijiLen(unsigned short code) {
    int found = FONT_NO_NONE;
    int i;
    for (i = 0; i < FCONV_CODE_NUM; i++) {
        if (code == FconvCodeTbl[i].code) {
            found = i;
            break;
        }
    }
    if (found == FONT_NO_NONE) {
        return 0;
    }
    return FconvCodeTbl[found].len;
}
unsigned short GetAlphabeticalFontNo_uc(unsigned char c) {
    u8 table[ALPHABETICAL_CHARA_NUM] = {
        0xA1, 0xAA, 0xB0, 0xBA, 0xBF, 0xC0, 0xC1, 0xC2,
        0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
        0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD1, 0xD2, 0xD3,
        0xD4, 0xD5, 0xD6, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD,
        0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
        0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE,
        0xEF, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF9,
        0xFA, 0xFB, 0xFC, 0xFD, 0xFF, 0xBD, 0xBE,
    };
    int i;
    u8 key;
    if (LanguageCode == LANG_ENGLISH) {
        return 0;
    }
    if (c == 0x9C) {
        key = 0xBE;
    } else {
        key = c;
    }
    for (i = 0; i < ALPHABETICAL_CHARA_NUM; i++) {
        if (key == table[i]) {
            return i + FONT_NO_ALPHABETICAL_TOP;
        }
    }
    return 0;
}
unsigned short GetAlphabeticalFontNo_cp(char *str) {
    char code[12];
    int i;
    if (LanguageCode == LANG_ENGLISH) {
        return 0;
    }
    if (strncmp(str, "[UNI0", 5) != 0) {
        return 0;
    }
    strncpy(code, str + 5, 5);
    if (strncmp(code, "11d]", 4) == 0) {
        strncpy(code, "0e8]", 4);
    }
    if (strncmp(code, "129]", 4) == 0) {
        strncpy(code, "0ea]", 4);
    }
    if (strncmp(code, "155]", 4) == 0) {
        strncpy(code, "0e0]", 4);
    }
    if (strncmp(code, "171]", 4) == 0) {
        strncpy(code, "0fb]", 4);
    }
    if (strncmp(code, "17f]", 4) == 0) {
        strncpy(code, "0f9]", 4);
    }
    for (i = 0; i < ALPHABETICAL_CHARA_NUM; i++) {
        if (strncmp(code, alphabetical_chara_tbl[i], 4) == 0) {
            return i + FONT_NO_ALPHABETICAL_TOP;
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", GetFontGaijiFontNo__FPc);
unsigned short GetAlphabeticalFontNo_us(unsigned short code) {
    u16 table[ALPHABETICAL_CHARA_NUM] = {
        0xFFA1, 0xFFAA, 0xFFB0, 0xFFBA, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2,
        0xFFC3, 0xFFC4, 0xFFC5, 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA,
        0xFFCB, 0xFFCC, 0xFFCD, 0xFFCE, 0xFFCF, 0xFFD1, 0xFFD2, 0xFFD3,
        0xFFD4, 0xFFD5, 0xFFD6, 0xFFD9, 0xFFDA, 0xFFDB, 0xFFDC, 0xFFDD,
        0xFFDF, 0xFFE0, 0xFFE1, 0xFFE2, 0xFFE3, 0xFFE4, 0xFFE5, 0xFFE6,
        0xFFE7, 0xFFE8, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFEC, 0xFFED, 0xFFEE,
        0xFFEF, 0xFFF1, 0xFFF2, 0xFFF3, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF9,
        0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFF, 0xFFBD, 0xFFBE,
    };
    int i;
    if (LanguageCode == LANG_ENGLISH) {
        return 0;
    }
    for (i = 0; i < ALPHABETICAL_CHARA_NUM; i++) {
        if (code == table[i]) {
            return i + FONT_NO_ALPHABETICAL_TOP;
        }
    }
    return 0;
}
unsigned short GetFontNoFromFontGaijiCode(unsigned short code) {
    u16 table[FONT_GAIJI_CONV_NUM] = {
        0xFDE0, 0xFDE1, 0xFDE2, 0xFDE3, 0xFDE4, 0xFDE5, 0xFDE6, 0xFDE7,
        0xFDE8, 0xFDE9, 0xFDEA, 0xFDEB, 0xFDEC, 0xFDED, 0xFDEE, 0xFDEF,
        0xFDF0, 0xFDF1, 0xFDF2, 0xFDF3, 0xFDF4, 0xFDF5, 0xFDF6, 0xFDF7,
    };
    int i;
    if (LanguageCode == LANG_ENGLISH) {
        return 0;
    }
    for (i = 0; i < FONT_GAIJI_CONV_NUM; i++) {
        if (code == table[i]) {
            return i + FONT_NO_FONT_GAIJI_TOP;
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", GetFontGaijiHankaku__FUs);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", GetFontNo__FPc);
int GetHalfFontNo(char c) {
    char buf[8];
    u16 no = GetAlphabeticalFontNo_uc(c);
    if (no != 0) {
        return no & 0xFFFF;
    }
    buf[1] = 0x20;
    buf[0] = c;
    return GetFontNo(buf);
}

int CFont::GetDigitNo(int font_no) {
    if (font_no == GetFontNo("\202P")) {
        return 1;
    }
    if (font_no == GetFontNo("\202Q")) {
        return 2;
    }
    if (font_no == GetFontNo("\202R")) {
        return 3;
    }
    if (font_no == GetFontNo("\202S")) {
        return 4;
    }
    if (font_no == GetFontNo("\202T")) {
        return 5;
    }
    if (font_no == GetFontNo("\202U")) {
        return 6;
    }
    if (font_no == GetFontNo("\202V")) {
        return 7;
    }
    if (font_no == GetFontNo("\202W")) {
        return 8;
    }
    if (font_no == GetFontNo("\202X")) {
        return 9;
    }
    if (font_no == GetFontNo("\202O")) {
        return 0;
    }
    return -1;
}

void set2DSpriteEasyFont(mgCDrawPrim *prim, mgRect<int> xy, mgRect<int> uv, RGBAQ_TYPE *color) {
    uv.right += uv.left;
    uv.bottom += uv.top;
    uv.right += 1;
    uv.bottom += 1;
    xy.right += xy.left;
    xy.bottom += xy.top;
    prim->Color(color->r, color->g, color->b, color->a);
    prim->TextureCrd(uv.left, uv.top);
    prim->Vertex(xy.left, xy.top, 0);
    prim->TextureCrd(uv.right, uv.bottom);
    prim->Vertex(xy.right, xy.bottom, 0);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", set2DSprite_Fuchi__FP11mgCDrawPrim4RECT4RECTii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/font", DrawChar__5CFontFP11mgCDrawPrimiiii10RGBAQ_TYPEUc);
void CFont::DrawChar(mgCDrawPrim *prim, char *str, int x, int y) {

    DrawChar(prim, GetFontNo(str), x, y, 1, color, (int)alpha);
}

void MySetTex(char *texture_name, mgCDrawPrim *prim) {
    prim->Texture(mgTexManager.GetTexture(texture_name, -1));
}

void MySetTex(int font_page, mgCDrawPrim *prim) {
    if ((font_page == 0) || (font_page == 1)) {
        prim->Texture(GetFontTexture(font_page));
    }
}

void DrawGaiji_sub(mgCDrawPrim *prim, int code, int x, int y, RGBAQ_TYPE color, int line_h) {
    mgRect<int> dst;
    mgRect<int> src;
    int index = code - GAIJI_CODE_TOP;
    if (LanguageCode != LANG_JAPANESE) {
        if (index + GAIJI_CODE_TOP == 0xFD06) {
            index = 8;
        } else if (index + GAIJI_CODE_TOP == 0xFD08) {
            index = 6;
        }
    }
    int width = GaijiDataTbl[index].w;
    int height = GaijiDataTbl[index].h;
    int offset_x = GaijiDataTbl[index].off_x;
    int offset_y = GaijiDataTbl[index].off_y;
    src.Set(GaijiDataTbl[index].u, GaijiDataTbl[index].v, width, height);
    dst.Set(x + offset_x, y + offset_y + (line_h - height) / 2, width, height);
    set2DSpriteEasy(prim, dst, src, &color);
}

void CFont::DrawGaiji(mgCDrawPrim *prim, int code, int x, int y) {
    RGBAQ_TYPE neutral;
    MySetTex("gaiji", prim);
    neutral.a = 0x80;
    neutral.b = 0x80;
    neutral.g = 0x80;
    neutral.r = 0x80;
    DrawGaiji_sub(prim, code, x, y, neutral, clearance_h);
}

void UpDateWH(s32 *width, s32 *height, s32 new_width, s32 new_height) {
    if (*width < new_width) {
        *width = new_width;
    }
    if (*height < new_height) {
        *height = new_height;
    }
}

void CFont::CalcDrawWH(char *str, int *w, int *h) {
    int *height_out = h;
    int len = strlen(str);
    int max_width = 0;
    int max_height = 0;
    int pen_x = 0;
    int pen_y = 0;
    int pos = 0;
    char *cursor;
    u16 gaiji_no;
    u16 gaiji;
    int half;
    while (pos < len) {
        cursor = &str[pos];
        if (0 < GetAlphabeticalFontNo_cp(cursor)) {
            pen_x += clearance_w / 2;
            pos += 9;
            UpDateWH(&max_width, &max_height, pen_x, pen_y + clearance_h);
        } else {
            gaiji = GetFontGaijiFontNo(cursor);
            if (gaiji != 0) {
                if (GetFontGaijiHankaku(gaiji) != 0) {
                    pen_x += clearance_w / 2;
                } else {
                    pen_x += clearance_w;
                }
                pos += 2;
                UpDateWH(&max_width, &max_height, pen_x, pen_y + clearance_h);
            } else {
                gaiji_no = GetGaijiFontNo(cursor);
                if (gaiji_no >= GAIJI_CODE_TOP && gaiji_no < GAIJI_CODE_END) {
                    pen_x += GetGaijiW(gaiji_no);
                    pos += GetGaijiLen(gaiji_no);
                    UpDateWH(&max_width, &max_height, pen_x, pen_y + GetGaijiH(gaiji_no));
                } else {
                    half = GetHalfFontNo(*cursor);
                    if (half == FONT_NO_NEWLINE) {
                        pen_x = 0;
                        pos += 1;
                        pen_y += clearance_h;
                    } else if (CheckHalfFont(half) != 0) {
                        pen_x += clearance_w / 2;
                        pos += 1;
                        UpDateWH(&max_width, &max_height, pen_x, pen_y + clearance_h);
                    } else {
                        if (CheckKanjiFont(GetFontNo(cursor)) != 0) {
                            pen_x += clearance_w;
                        } else if (CheckKanjiFont(GetFontNo(&cursor[2])) != 0) {
                            pen_x += clearance_w;
                        } else {
                            pen_x += clearance_w;
                        }
                        pos += 2;
                        UpDateWH(&max_width, &max_height, pen_x, pen_y + clearance_h);
                    }
                }
            }
        }
    }
    *w = max_width;
    *height_out = max_height;
}

void CFont::DrawDirect(char *str, int x, int y) {
    SetPos(x, y);
    mgCDrawPrim prim;
    MySetPrim(&prim, 1, 0);

    int height = (int)unk_b4;
    prim.offset_x = (int)unk_b0 * 16;
    prim.offset_y = height * 16;
    prim.Begin(6);
    int len = strlen(str);
    int pen_x = 0;
    int pen_y = 0;
    int pos = 0;
    u16 gaiji_no;
    u16 gaiji;
    int font_no;
    int half;
    char *cursor;
    while (pos < len) {
        cursor = &str[pos];
        font_no = GetAlphabeticalFontNo_cp(cursor);
        if (0 < font_no) {
            DrawChar(&prim, font_no, pos_x + pen_x, pos_y + pen_y, 1, color, (int)alpha);
            pen_x += clearance_w / 2;
            pos += 9;
        } else {
            gaiji = GetFontGaijiFontNo(cursor);
            if (gaiji != 0) {
                DrawChar(&prim, gaiji & 0xFFFF, pos_x + pen_x, pos_y + pen_y, 1, color,
                         (int)alpha);
                if (GetFontGaijiHankaku(gaiji) != 0) {
                    pen_x += clearance_w / 2;
                } else {
                    pen_x += clearance_w;
                }
                pos += 2;
            } else {
                gaiji_no = GetGaijiFontNo(cursor);
                if (gaiji_no >= GAIJI_CODE_TOP && gaiji_no < GAIJI_CODE_END) {
                    DrawGaiji(&prim, gaiji_no, pos_x + pen_x, pos_y + pen_y);
                    pen_x += GetGaijiW(gaiji_no);
                    pos += GetGaijiLen(gaiji_no);
                } else {
                    half = GetHalfFontNo(*cursor);
                    if (half == FONT_NO_NEWLINE) {
                        pen_x = 0;
                        pos += 1;
                        pen_y += clearance_h;
                    } else if (CheckHalfFont(half) != 0) {
                        DrawChar(&prim, half, pos_x + pen_x, pos_y + pen_y, 1, color,
                                 (int)alpha);
                        pen_x += clearance_w / 2;
                        pos += 1;
                    } else {
                        DrawChar(&prim, cursor, pos_x + pen_x, pos_y + pen_y);
                        if (CheckKanjiFont(GetFontNo(cursor)) != 0) {
                            pen_x += clearance_w;
                        } else if (CheckKanjiFont(GetFontNo(&cursor[2])) != 0) {
                            pen_x += clearance_w;
                        } else {
                            pen_x += clearance_w;
                        }
                        pos += 2;
                    }
                }
            }
        }
    }
    prim.End();
}

void CFont::Preset(s32 preset) {
    switch (preset) {
    case 0:
    case 1:
        this->SetColor(0x80202020U);
        SetFuchi(FUCHI_SHADOW_BLACK);
        break;
    case 2:
    case 3:
        this->SetColor(0x80686A6BU);
        SetFuchi(FUCHI_OUTLINE_THICK);
        break;
    case 4:
        this->SetColor(0x80686A6BU);
        SetFuchi(FUCHI_SHADOW_BLACK_WIDE);
        break;
    }
}

void CFont::Init(void) {
    memset(str, 0, sizeof(str));
    SetFuchi(FUCHI_OUTLINE);
    color.a = 0x80;
    color.b = 0x80;
    color.g = 0x80;
    color.r = 0x80;
    alpha = 0x80;
    pos_y = 0;
    pos_x = 0;
    clearance_w = 15;
    clearance_h = 24;
    draw_w = 16;
    draw_h = 20;
    mini = 0;
    unk_b0 = 0.0f;
    unk_b4 = 0.0f;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", GaijiDataTbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", FconvCodeTbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", FontGaijiConvTbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", alphabetical_chara_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1041__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1120__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1137__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1255__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1264__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1272__2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_812__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_813__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_848__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_849__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_850__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_936__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_938__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_939__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_940__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_941__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_942__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_943__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_944__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_945__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_946__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_947__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_948__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_949__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_950__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_951__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_952__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_953__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_954__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_955__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_956__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_957__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_958__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_959__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_960__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_961__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_962__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_963__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_964__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_965__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_966__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_967__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_968__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_969__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_970__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_971__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_972__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_973__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_974__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_975__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_976__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_977__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_978__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_979__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_980__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_981__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_982__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_983__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_984__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_985__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_986__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_987__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_988__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_989__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_990__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_991__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_992__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_993__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_994__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_995__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_996__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_997__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_998__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_999__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1000__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1001__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1002__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1003__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1004__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1005__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1089__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1090__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1091__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1092__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1093__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1094__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1095__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1096__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1097__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1098__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1099__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1448__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/font", at_1543__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(FontTblBinBuff, 0x1000);
INCLUDE_BSS(at_784__2, 0x10);
INCLUDE_BSS(at_817__4, 0x10);
INCLUDE_BSS(at_1466__6, 0x10);
