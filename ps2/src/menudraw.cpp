#include "common.h"
#include "menudraw.hpp"
#include "dataread.hpp"
#include "dng_effect.hpp"
#include "font.hpp"
#include "gamedata.hpp"
#include "mainloop.hpp"
#include "mapload.hpp"
#include "menucls1.hpp"
#include "menuchr.hpp"
#include "menucommon.hpp"
#include "menumain.hpp"
#include "menusys.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "scenesnd.hpp"
#include "userdata.hpp"
#include <libgraph.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static s8                MenuDrawNumberKeta;
static u8                MenuMainFrame_ActionEndFlag;
static short             use_trans_rect;
static float             use_item_enable_alpha_angle;
static int               use_item_enable_alpha;
static float            *spectol_raster_xtbl;
static mgCTexture       *Tex_MenuDl;
static int               MenuDl_TotalSize;
static int               MenuDl_ProcessSize;
static float             MenuMainFrame_LeftTop_Pos[2];
static short             MenuMainFrame_Display_Mode;
static float             MenuMainFrame_Display_Mode_Cnt;
static float             MenuMainFrame_Display_Mode_Cnt_Rate;
static float             MenuMainFrame_Lenze_Pos[2];
static float             MenuMainFrame_MoveRate[2];
static float             MenuMainFrame_MoveRate_Cnt;
mgRect<int>              GiftBoxWindowPutPos(0, 0, 0, 0);
mgRect<int>              menu_long_hand(0x3E, 1, 0x28, 0x18);
static mgRect<int>       MenuMainFrame_PutRect(0, 0, 0, 0);
static mgRect<int>       MenuMainIMG_PutRect(0, 0, 0, 0);
static mgRect<int>       star_light(0, 0x20, 8, 8);
static mgRect<int>       MenuItemBrdKomaRect(0x20, 0x20, 0x28, 0x32);
static mgRect<int>       ItemBoardScrlBar1(0x60, 0x80, 0x16, 0xB);
static mgRect<int>       ItemBoardScrlBar2(0x60, 0x8A, 0x16, 4);
static mgRect<int>       ItemBoardScrlBar3(0x60, 0x8C, 0x16, 0xC);
static mgRect<int>       ItemBoardCursor(0x76, 0x80, 8, 0x1E);
static int               MenuItemBrdMaxLine;
static int               MenuItemBrdViewLine;
static float             MenuItemBrdScrlCurLen;
static float             DrawItemCounter;
static s8                DrawItemDefCounter;
static float             MenuItemBrdScrlBarY;
static mgCTexture       *MenuVerticalLineTex;
static float             MenuVerticalLineUpLimmit;
static CEffVerticalLine *MenuVerticalLine;
static int               MenuVerticalLineNum;
static CCharacter2      *MenuVerticalLineChara;
static float             MenuVerticalRange;
static sceVu0FVECTOR     MenuVerticalLineCharaPos;
static sceVu0FVECTOR     MenuVerticalLineCharaPos2;
static float             l_levelup_pos[32][3];
static float             l_levelup_vec[32][3];
static s8                l_levelup_counter[32];
static s8                l_levelup_generate_counter[32];
static short             fish_boiled_runflag;
static short             fish_boiled_count;
static float             fish_boiled_positin[8][2];
static float             fish_boiled_amp_count[8];
static float             fish_boiled_streatch_rate[8];
static float             fish_boiled_alpha[8];
static mgCTexture       *fish_boiled_effect_tex;

static void ConvMGIRECTtoINTtbl(mgRect<int> rect, int *corners);
static void SetPartEffectInfoRandFunc(MENU_PARTS_EFFECT_STRUCT1 *effect);
static void SetPartEffectInfoRandFunc(MENU_PARTS_EFFECT_STRUCT1 *effect, short *param, int num);
static void PushPrimRepeat(mgCDrawPrim *prim, float *pos, int *tex_pos, int num);
static void MenuWindowHelp(mgCDrawPrim *prim, mgCTexture *tex, float x, float y, float w, float h, short *tex_tbl);
static void SetMenuDrawNumberKeta(char keta);
static void DrawRandamLine(mgCDrawPrim *prim, int *points, int division, int num, u8 *rgba);
static void MENU_BASETEXINFO_Init(MENU_BASETEXINFO *info);
static void InitInitBuildUpInfoEffectPos();

// Trap on division by zero for variable integer divisors.
#pragma divbyzerocheck on

// Code (.text)
void AttachMessageForm() {
    char name[32];

    for (int i = 0; i < 9; i++) {
        sprintf(name, "msg%d", i);
        MenuMesForm[i] = MenuPosData->GetFormInfo(name);
    }
}

void Init_MENUFORM_MAKEBRD_INFO(MENUFORM_MAKEBRD_INFO *board) {
    memset(board, 0, sizeof(*board));
}

void GetMenuItemIconTexGetXY(int item, mgRect<int> &out_rect) {
    int icon_no = GetItemIconNo(item);

    // The night version of this item's icon follows its day version.
    if (item == 0x38 && MenuMainScene != NULL && GetTimeBand(MenuMainScene->time) == 2) {
        icon_no++;
    }
    out_rect.right = out_rect.bottom = 32;
    out_rect.left = (icon_no % 8) * out_rect.right;
    out_rect.top = (icon_no / 8) * out_rect.bottom;
}

mgCTexture *GetMenuItemIconTexInfo(int item, int kind) {
    use_trans_rect = -1;
    CDataCommon *common = GameItemDataManage.GetCommonData(item);
    if (common != NULL) {
        use_trans_rect = common->unk_20;
        if (0 <= use_trans_rect) {
            mgCTexture *tex[4] = {
                MenuPosData->item_icon_tex[0][use_trans_rect],
                MenuPosData->item_icon_tex[1][use_trans_rect],
                MenuPosData->item_icon_tex[2][use_trans_rect],
                MenuPosData->item_icon_tex[3][use_trans_rect],
            };
            return tex[kind];
        }
    }
    return NULL;
}

/**
 *
 * Writes the four corners of a rectangle as x and y pairs.
 *
 */
static void ConvMGIRECTtoINTtbl(mgRect<int> rect, int *corners) {
    corners[0] = rect.left;
    corners[1] = rect.top;
    corners[2] = rect.left + rect.right;
    corners[3] = rect.top;
    corners[4] = rect.left;
    corners[5] = rect.top + rect.bottom;
    corners[6] = corners[2];
    corners[7] = corners[5];
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", ConvMGFRECTtoFLOATtbl__F9mgRect_f_Pf);

/**
 *
 * Gives a part effect random parameters for its kind.
 *
 */
static void SetPartEffectInfoRandFunc(MENU_PARTS_EFFECT_STRUCT1 *effect) {
    if (effect != NULL) {
        int rand_a = rand();
        int rand_b = rand();
        switch (effect->type) {
            case MENU_PARTS_EFFECT_UNK_9:
                effect->param[0] = 0.0f;
                effect->param[1] = 34.0f + (float)(rand_a % 20);
                effect->param[2] = 2.0f + (float)(rand_a % 30);
                effect->param[3] = (float)(rand_b % 34 - 1);
                effect->param[4] = (float)(rand_a % 3);
                effect->param[5] = 1.0f + 0.2f * (float)(rand_b % 4);
                effect->param[6] = (float)(rand_a % 9);
                effect->param[7] = 3.0f + (float)(rand_b % 6);
                return;
            case MENU_PARTS_EFFECT_STRETCH:
                effect->param[0] = 0.0f;
                effect->param[1] = 320.0f;
                effect->param[2] = 16.0f;
                effect->param[3] = 20.0f;
                effect->param[4] = 0.0f;
                effect->param[5] = 1000.0f;
                break;
        }
    }
}

/**
 *
 * Copies parameters into a part effect.
 *
 */
static void SetPartEffectInfoRandFunc(MENU_PARTS_EFFECT_STRUCT1 *effect, short *param, int num) {
    for (int i = 0; i < num; i++) {
        effect->param[i] = param[i];
    }
}

void SetSpriteEnv(mgCDrawPrim *prim, int mode) {
    if (prim != NULL) {
        prim->Initialize(NULL, NULL);
        prim->Coord(0);
        switch (mode) {
            case 0:
            case 4:
            case 6:
                prim->AlphaBlendEnable(1);
                prim->Bilinear(0);
                if (mode == 4) {
                    prim->AlphaBlend(2);
                    prim->Bilinear(1);
                } else {
                    prim->AlphaBlend(1);
                }
                prim->AlphaTestEnable(1);
                prim->AlphaTest(1, 0);
                prim->DepthTestEnable(0);
                prim->ZMask(-1);
                prim->Shading(0);
                prim->TextureMapEnable(1);
                prim->AntiAliasing(0);
                if (mode == 6) {
                    prim->Shading(1);
                    prim->Bilinear(1);
                    return;
                }
                break;
            case 1:
            case 2:
                prim->AlphaTestEnable(1);
                prim->AlphaTest(1, 0);
                prim->AlphaBlendEnable(1);
                prim->AlphaBlend(1);
                prim->TextureMapEnable(0);
                prim->DepthTestEnable(1);
                if (mode == 2) {
                    prim->DepthTestEnable(0);
                    prim->AntiAliasing(1);
                }
                prim->ZMask(-1);
                prim->Shading(1);
                return;
            case 5:
                prim->AlphaTestEnable(0);
                prim->AlphaTest(1, 0);
                prim->AlphaBlendEnable(0);
                prim->AlphaBlend(4);
                prim->DepthTestEnable(0);
                prim->ZMask(-1);
                prim->Shading(0);
                prim->TextureMapEnable(1);
                prim->Bilinear(1);
                return;
            case 3:
                prim->AlphaTestEnable(1);
                prim->AlphaTest(1, 0);
                prim->AlphaBlendEnable(1);
                prim->AlphaBlend(1);
                prim->DepthTestEnable(0);
                prim->ZMask(-1);
                prim->Shading(1);
                prim->TextureMapEnable(0);
                break;
            default:
                break;
        }
    }
}

/**
 *
 * Writes a run of textured vertices.
 *
 */
static void PushPrimRepeat(mgCDrawPrim *prim, float *pos, int *tex_pos, int num) {
    for (int i = 0; i < num; i++) {
        prim->TextureCrd(tex_pos[i * 2], tex_pos[i * 2 + 1]);
        prim->Vertex(pos[i * 2], pos[i * 2 + 1], 0.0f);
    }
}

void PrimQuad(mgCDrawPrim *prim, float x, float y, mgRect<int> tex_rect) {
    prim->TextureCrd(tex_rect.left, tex_rect.top);
    prim->Vertex(x, y, 0.0f);
    prim->TextureCrd(tex_rect.left + tex_rect.right, tex_rect.top + tex_rect.bottom);
    prim->Vertex(x + tex_rect.right, y + tex_rect.bottom, 0.0f);
}

void PrimQuad(mgCTexture *tex, float x, float y, mgRect<int> tex_rect, int a, int r, int g, int b) {
    mgCDrawPrim prim;

    SetSpriteEnv(&prim, 0);
    prim.Begin(6);
    prim.Texture(tex);
    prim.Color(r, g, b, a);
    PrimQuad(&prim, x, y, tex_rect);
    prim.End();
}

void PrimQuad(mgCDrawPrim *prim, mgCTexture *tex, float x, float y, mgRect<int> tex_rect, int a, int r, int g,
              int b) {
    SetSpriteEnv(prim, 0);
    prim->Begin(6);
    prim->Texture(tex);
    prim->Color(r, g, b, a);
    PrimQuad(prim, x, y, tex_rect);
    prim->End();
}

void PrimQuad(mgCTexture *tex, mgRect<int> put_rect, mgRect<int> tex_rect, int a, int r, int g, int b) {
    mgCDrawPrim prim;

    SetSpriteEnv(&prim, 0);
    prim.Begin(6);
    prim.Texture(tex);
    prim.Color(r, g, b, a);
    PrimQuad(&prim, put_rect, tex_rect);
    prim.End();
}

void PrimQuad(mgCDrawPrim *prim, mgCTexture *tex, mgRect<int> put_rect, mgRect<int> tex_rect, int a, int r, int g,
              int b) {
    SetSpriteEnv(prim, 0);
    prim->Begin(6);
    prim->Texture(tex);
    prim->Color(r, g, b, a);
    PrimQuad(prim, put_rect, tex_rect);
    prim->End();
}

void MenuClipRectCheck(mgRect<int> &rect) {
    if (rect.left < 0) {
        rect.left = 0;
    }
    if (rect.top < 0) {
        rect.top = 0;
    }
    int max_x = mgScreenWidth - 1;
    if (rect.right > max_x) {
        rect.right = max_x;
    }
    int max_y = mgScreenHeight - 1;
    if (rect.bottom > max_y) {
        rect.bottom = max_y;
    }
}

void SetMenuScissor(mgRect<int> rect) {
    mgCDrawPrim *prim = GetMenuPrim();
    prim->Initialize(NULL, NULL);
    prim->Begin(0);
    prim->Direct(SCE_GS_SCISSOR_1, rect.left | ((u_long)rect.right << 16) | ((u_long)rect.top << 32) | ((u_long)rect.bottom << 48));
    prim->End();
}

void ResetMenuScissor() {
    mgCDrawPrim *prim = GetMenuPrim();
    prim->Initialize(NULL, NULL);
    prim->Begin(0);
    prim->Direct(SCE_GS_SCISSOR_1, ((u_long)(mgScreenWidth - 1) << 16) | ((u_long)(mgScreenHeight - 1) << 48));
    prim->End();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", SetModeMenuDrawItemBoard__Fi);

void EnableUseItemAlphaStep() {
    use_item_enable_alpha_angle += 0.052359879f;
    if (use_item_enable_alpha_angle >= 3.1415927f) {
        use_item_enable_alpha_angle -= 6.2831855f;
    }
    use_item_enable_alpha = (int)(48.0f + 32.0f * sinf(use_item_enable_alpha_angle));
}

void InitSpectolRasterTable(mgCMemory *stack) {
    int row;
    int column;
    int offset;
    float angle;

    spectol_raster_xtbl = (float *)stack->Alloc(0x4E0);
    for (row = 0, offset = 0; row < 0x9C; row++) {
        angle = 0.0418879f * (float)row;
        while (3.1415927f < angle) {
            angle -= 6.2831855f;
        }
        for (column = 0; column < 0x20; column++) {
            spectol_raster_xtbl[offset + column] = 3.0f * sinf(angle);
            angle += 0.15707964f;
        }
        offset += 0x20;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawOneItem__FP11mgCDrawPrim9mgRect_f_iiP25MENU_PARTS_EFFECT_STRUCT1PUci);

/**
 *
 * Draws a help window three texture rows high, stretching its middle row.
 *
 */
static void MenuWindowHelp(mgCDrawPrim *prim, mgCTexture *tex, float x, float y, float w, float h, short *tex_tbl) {
    static short MenuWindowHelpTable[36] = {
        0, 0,  22, 24, 23, 0,  2, 24, 24, 0,  22, 24, 0, 25, 22, 1,  22, 25,
        2, 1,  24, 25, 22, 1,  0, 68, 22, 24, 22, 68, 2, 24, 24, 68, 22, 24,
    };

    if (tex != NULL) {
        mgRect<int> top;
        mgRect<int> middle;
        mgRect<int> bottom;
        if (tex_tbl == NULL) {
            tex_tbl = MenuWindowHelpTable;
        }
        SetSpriteEnv(prim, 0);
        prim->Begin(6);
        prim->Texture(tex);
        prim->Color(128, 128, 128, 128);
        top.Set((int)x, (int)y, (int)w, 24);
        Menu3DivideTextureDraw(prim, top, tex_tbl, 1);
        middle.Set((int)x, (int)(24.0f + y), (int)w, (int)h);
        Menu3DivideTextureDraw(prim, middle, &tex_tbl[12], 1);
        bottom.Set((int)x, (int)(24.0f + y + h), (int)w, 24);
        Menu3DivideTextureDraw(prim, bottom, &tex_tbl[24], 1);
        prim->End();
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuPresentBoxView__FiiRiP10mgCTextureP10mgCTexture);

/**
 *
 * Sets the number of digits the menu numbers are drawn with.
 *
 */
static void SetMenuDrawNumberKeta(char keta) {
    MenuDrawNumberKeta = keta;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawMenuNumber__FP11mgCDrawPrimii9mgRect_i_9mgRect_i_ii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", PrimDrawNumber__FP11mgCDrawPrimiiii9mgRect_i_ii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", PrimDrawNumber2__FP11mgCDrawPrimiiii9mgRect_i_ii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", PrimFillRect4__FP11mgCDrawPrim9mgRect_f_PfPfPfPf);

void MenuReloadTexture(int &tex_block, int new_tex_block) {
    mgCTextureManager *manager = &mgTexManager;

    if (tex_block != new_tex_block) {
        tex_block = new_tex_block;
        manager->ReloadTexture(tex_block, (sceVif1Packet *)NULL);
    }
}

void MenuReloadCLUT(int no) {
    mgCTexture *tex[2] = {MenuCharaChangeCLUT_Tex, MenuCharaChangeBase_Tex};

    if (tex[no] != NULL) {
        mgTexManager.ReloadCLUT(tex[no], (sceVif1Packet *)NULL);
    }
}

void DrawMenuFillBox(int a, int r, int g, int b) {
    DrawMenuFillBox(0.0f, 0.0f, (float)mgScreenWidth, (float)mgScreenHeight, a, r, g, b);
}

void DrawMenuFillBox(float x, float y, float w, float h, int a, int r, int g, int b) {
    mgCDrawPrim *prim;

    prim = GetMenuPrim();
    SetSpriteEnv(prim, 1);
    prim->DepthTestEnable(0);
    prim->Begin(6);
    prim->Color(r, g, b, a);
    prim->Vertex(x, y, 0.0f);
    prim->Vertex(x + w, y + h, 0.0f);
    prim->End();
}

void DrawMenuFillBox(mgCDrawPrim *prim, float x, float y, float w, float h, int a, int r, int g, int b) {
    SetSpriteEnv(prim, 1);
    prim->DepthTestEnable(0);
    prim->Begin(6);
    prim->Color(r, g, b, a);
    prim->Vertex(x, y, 0.0f);
    prim->Vertex(x + w, y + h, 0.0f);
    prim->End();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GenarateRandamLine__FPiiiPiii);

/**
 *
 * Draws a line strip through points smoothed into a curve.
 *
 */
static void DrawRandamLine(mgCDrawPrim *prim, int *points, int division, int num, u8 *rgba) {
    sceVu0FVECTOR ring[1000];
    sceVu0FVECTOR path[2000];
    int i;
    int total;

    if (prim == NULL || points == NULL) {
        return;
    }
    for (i = 0; i < num; i++) {
        ring[i][0] = (float)points[i * 2];
        ring[i][1] = (float)points[i * 2 + 1];
        ring[i][2] = 0;
    }
    SetSpriteEnv(prim, 3);
    prim->Begin(2);
    CreatSmoothPass(path, ring, num, division, 0, num);
    prim->Color(rgba[0], rgba[1], rgba[2], rgba[3]);
    prim->Vertex(path[0][0], path[0][1], 0.0f);
    total = (num - 1) * (division - 1);
    for (i = 0; i < total; i++) {
        prim->Vertex(path[i][0], path[i][1], 0.0f);
    }
    prim->End();
}

mgCTexture *GetMenuDlTexture() {
    return mgTexManager.GetTexture("menudl", -1);
}

void InitMenuDl(mgCTexture *tex, int total_size) {
    Tex_MenuDl = tex;
    MenuDl_TotalSize = total_size;
    MenuDl_ProcessSize = 0;
}

int StepMenuDl(int add_size) {
    if (MenuDl_TotalSize <= 0) {
        return 1;
    }
    MenuDl_ProcessSize += add_size;
    if (MenuDl_TotalSize <= MenuDl_ProcessSize) {
        MenuDl_ProcessSize = MenuDl_TotalSize;
        return 1;
    }
    return 0;
}

int StepMenuDl2(int size) {
    if (MenuDl_TotalSize <= 0) {
        return 1;
    }
    MenuDl_ProcessSize = size;
    if (MenuDl_TotalSize <= size) {
        MenuDl_ProcessSize = MenuDl_TotalSize;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawMenuDl__FRiiiii);

void DrawMenuDl(int alpha) {
    static char *tbl[7][2] = {
        {(char *)" ", (char *)" "},
        {(char *)"Downloading Geostone...", (char *)"Geostone downloading complete."},
        {(char *)"T[UNI00e9]l[UNI00e9]chargement de la g[UNI00e9]opierre...",
         (char *)"T[UNI00e9]l[UNI00e9]chargement de la g[UNI00e9]opierre fini"},
        {(char *)"Geostein-Download ...", (char *)"Geostein-Download beendet."},
        {(char *)"Scaricamento Geopietra in corso...", (char *)"Scaricamento Geopietra completato."},
        {(char *)"Descargando Geopiedra...", (char *)"Descarga de Geopiedra finalizada."},
        {(char *)"Downloading Geostone...", (char *)"Geostone downloading complete."},
    };
    char text[0x80];
    int tex_block;
    int h;
    int w;

    if (Tex_MenuDl != NULL) {
        if (alpha < 0) {
            alpha = 0;
        }
        tex_block = -1;
        int panel_w = 0x10E;
        if (LanguageCode == 2 || LanguageCode == 4 || LanguageCode == 5) {
            panel_w = 0x13A;
        }
        DrawMenuDl(tex_block, 0, 0x72, panel_w, alpha);
        int language = LanguageCode;
        mgCTexture *tex = mgTexManager.GetTexture("gaiji", -1);
        if (tex != NULL) {
            MenuReloadTexture(tex_block, tex->block);
            int done = StepMenuDl(0);
            memset(text, 0, sizeof(text));
            ConvertFontCode(tbl[language][done], text);
            CMenuFont font;
            font.alpha = alpha;
            font.SetStr(text);
            font.CalcDrawWH(font.str, &w, &h);
            font.SetPos((0x200 - w) >> 1, 0x86);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", CalcCommonBrdDrawInfo__FPfP21MENUFORM_MAKEBRD_INFOP6ClsMes);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", CommonBoardDraw__FPfRi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuCursorDraw__FP10mgCTexturePffiif);

void MenuCursorDraw(mgCTexture *tex, float *pos, float rot, int alpha) {
    MenuCursorDraw(tex, pos, rot, 0, alpha, 1.0f);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawMenuTilePattern__FP11mgCDrawPrimP10mgCTextureff9mgRect_i_iPUc);

void DrawMenuMainFrmImg(int &tex_block, mgRect<int> put_rect, mgRect<int> tex_rect, int r, int g, int b, int a,
                        int unk) {
    mgCTexture *tex = MenuPosData->common_tex;
    if (tex != NULL) {
        MenuReloadTexture(tex_block, tex->block);
        mgCDrawPrim *prim = GetMenuPrim();
        SetSpriteEnv(prim, 5);
        prim->Begin(6);
        prim->Texture(tex);
        prim->Color(r, g, b, a);
        PrimQuad(prim, put_rect, tex_rect);
        prim->End();
    }
}

int GetMenuMainFrameEndFlag() {
    return MenuMainFrame_ActionEndFlag;
}

float *GetMenuMainFrameLeftTopPos(int unk) {
    return MenuMainFrame_LeftTop_Pos;
}

float GetMenuMainFrameCount() {
    static float tbl[8] = {10.0f, 14.0f, 24.0f, 27.0f, 24.0f, 24.0f, 24.0f, 24.0f};

    return tbl[MenuMainFrame_Display_Mode / 2];
}

void MenuMainFrameModeSet(int mode, int reset) {
    MenuMainFrame_Display_Mode = mode;
    MenuMainFrame_ActionEndFlag = 0;
    MenuMainFrame_MoveRate[0] = 1.0f;
    if (reset != 0) {
        switch (MenuMainFrame_Display_Mode) {
            case 0:
                MenuMainFrame_Display_Mode_Cnt = 0.0f;
                MenuMainFrame_Display_Mode_Cnt_Rate = 0.7f;
                break;
            case 2:
            case 4:
            case 6:
            case 8:
                MenuMainFrame_Display_Mode_Cnt = 10.0f;
                MenuMainFrame_Lenze_Pos[0] = 350.0f;
                MenuMainFrame_Lenze_Pos[1] = 240.0f;
                break;
        }
    }
    MenuMainFrame_MoveRate[0] = GetMenuMainFrameCount();
    MenuMainFrame_MoveRate[1] = GetMenuMainFrameCount();
    MenuMainFrame_MoveRate_Cnt = 0.0f;
    if (MenuMainFrame_Display_Mode >= 4) {
        MenuMainFrame_MoveRate_Cnt = 0.2617994f;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuMainFrameStep__Fv);

void MenuMainFrameDraw(int &tex_block, int alpha) {
    mgRect<int> screen_rect;
    float fade;
    float radius_x;
    float radius_y;
    float turn;
    float scale;
    float x0;
    float y0;
    float x1;
    float y1;
    float x3;
    float y3;
    float x2;
    float y2;
    float x4;
    float size;
    float y4;
    float angle;
    mgCTextureManager *manager = &mgTexManager;
    mgCTexture *background = manager->GetTexture("bg16", -1);
    if (background == NULL) {
        return;
    }
    screen_rect.Set(0, 0, 0x2C0, 0x1A0);
    radius_x = 160.0f;
    fade = 128.0f;
    float progress = MenuMainFrame_Display_Mode_Cnt / 10.0f;
    scale = 1.5f - 0.5f * progress;
    switch (MenuMainFrame_Display_Mode) {
        case 0:
        case 1:
            fade = 128.0f * progress;
            break;
    }
    turn = 0.7853982f * progress;
    angle = -0.5235988f + turn;
    radius_x *= scale;
    radius_y = 120.0f * scale;
    x0 = MenuMainFrame_Lenze_Pos[0] - radius_x * cosf(angle);
    y0 = MenuMainFrame_Lenze_Pos[1] - radius_x * sinf(angle);
    x1 = MenuMainFrame_Lenze_Pos[0] - radius_y * cosf(angle - 0.2617994f);
    y1 = MenuMainFrame_Lenze_Pos[1] - radius_y * sinf(angle - 0.2617994f);
    angle += 0.7853982f;
    x3 = MenuMainFrame_Lenze_Pos[0] - radius_x * cosf(angle);
    y3 = MenuMainFrame_Lenze_Pos[1] - radius_x * sinf(angle);
    angle = 0.24166098f + angle;
    x2 = MenuMainFrame_Lenze_Pos[0] - radius_y * cosf(angle);
    y2 = MenuMainFrame_Lenze_Pos[1] - radius_y * sinf(angle);
    turn = -0.83775806f + turn;
    x4 = MenuMainFrame_Lenze_Pos[0] - radius_x * cosf(turn);
    y4 = MenuMainFrame_Lenze_Pos[1] - radius_x * sinf(turn);
    size = 48.0 * scale;
    MenuReloadTexture(tex_block, background->block);
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Bilinear(1);
    prim->Begin(6);
    prim->Texture(background);
    int fade_alpha;
    prim->Color(0x80, 0x80, 0x80, fade_alpha = (int)fade);
    PrimQuad(prim, MenuMainFrame_PutRect, screen_rect);
    prim->End();
    mgCTexture *ornament = manager->GetTexture("mnmain", -1);
    prim->AlphaBlend(2);
    prim->Shading(1);
    prim->Begin(5);
    prim->Texture(ornament);
    prim->Color(0x80, 0x80, 0x80, fade_alpha);
    prim->TextureCrd(0x1A, 8);
    prim->Vertex(x0, y0, 0.0f);
    prim->TextureCrd(0x1A, 0x1A);
    prim->Vertex(x1, y1, 0.0f);
    prim->TextureCrd(0x38, 0x1A);
    prim->Vertex(x2, y2, 0.0f);
    prim->TextureCrd(0x38, 8);
    prim->Vertex(x3, y3, 0.0f);
    prim->End();
    prim->Begin(6);
    prim->TextureCrd(4, 0xA);
    prim->Vertex(x4, y4, 0.0f);
    prim->TextureCrd(0x12, 0x18);
    prim->Vertex((int)(x4 + size), (int)(y4 + 1.25f * size), 0);
    prim->End();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuMainFrameImgDraw__FRi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawMenuWakuStep__Fv);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawMenuWakuRect__FP10mgCTexture9mgRect_f_9mgRect_i_iiii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawWakuCircle__FP11mgCDrawPrimP10mgCTexture9mgRect_f_9mgRect_i_ffiiii);

/**
 *
 * Clears a texture rectangle entry.
 *
 */
static void MENU_BASETEXINFO_Init(MENU_BASETEXINFO *info) {
    info->name = NULL;
    info->tex_name = NULL;
    info->rect.Set(0, 0, 0, 0);
    info->tex_block = 0;
}

void MenuPosDataTypeInit(MENUFORMPARTS_TYPE *part) {
    part->name = NULL;
    part->active = 0;
    part->draw_flag = 1;
    part->dtype = MENUFORMPARTS_DTYPE_NORMAL;
    part->vibe_cnt[1] = 0;
    part->vibe_cnt[0] = 0;
    part->viber[0] = 10;
    part->viber[1] = 8;
    part->tex_info_no = 0;
    part->h = 0;
    part->w = 0;
    part->y = 0;
    part->x = 0;
    part->rgba[3] = 0x80;
    part->rgba[2] = 0x80;
    part->rgba[1] = 0x80;
    part->rgba[0] = 0x80;
    part->etc_info[2] = 0;
    part->etc_info[1] = 0;
    part->etc_info[0] = 0;
    part->unk_2c = 1.0f;
    part->effect_num = 0;
    part->effect = NULL;
    part->alpha_blend = 1;
    part->bilinear = 0;
    part->item_flag = 0;
    part->tex = NULL;
    part->shadow = 0;
}

void MenuFormPartsPresetItem(MENUFORMPARTS_TYPE *part, int draw, int item, int sub_item) {
    if (part != NULL) {
        part->draw_flag = draw != 0;
        part->etc_info[0] = 0;
        part->etc_info[1] = item;
        part->etc_info[2] = sub_item;
        Func_MenuItemIconSetEffectOne(part);
    }
}

void CMenuPosDataForm::Initialize() {
    int i;

    name = NULL;
    active = 0;
    draw_flag = 1;
    y = 0;
    x = 0;
    dtype = MENUFORM_DTYPE_NORMAL;
    vibe_cnt[1] = 0;
    vibe_cnt[0] = 0;
    clip_h = -1;
    clip_w = -1;
    next_y = 0;
    next_x = 0;
    rate_y = 1.1f;
    rate_x = 1.1f;
    mtype = -1;
    rgba_bit = 0;
    for (i = 0; i < 4; i++) {
        rgba_add[i] = 0;
        rgba[i] = 0x80;
        rgba_target[i] = 0x80;
    }
    counter = 0;
    parts_num = 0;
    parts = NULL;
    chara = NULL;
    chara_tex_block = 0;
    unk_36 = 0;
    step_stop = 0;
    action_no = -1;
    action_state = -1;
    action_num = 0;
    action = NULL;
    sub_no = 0;
    prev = NULL;
    next = NULL;
}

MENUFORMPARTS_TYPE *CMenuPosDataForm::GetPartInfo(char *part_name) {
    for (int i = 0; i < parts_num; i++) {
        if (strcmp(parts[i].name, part_name) == 0) {
            return &parts[i];
        }
    }
    return NULL;
}

void CMenuPosDataForm::SetPartDrawFlag(char *part_name, bool draw) {
    MENUFORMPARTS_TYPE *part = GetPartInfo(part_name);
    if (part != NULL) {
        part->draw_flag = draw;
    }
}

void Func_MallocPartEffectInfo(MENUFORMPARTS_TYPE *part, mgCMemory *stack, int num) {
    unsigned int size;
    unsigned int blocks;

    if (part != NULL) {
        size = num * sizeof(MENU_PARTS_EFFECT_STRUCT1);
        part->effect_num = num;
        if (size & 0xF) {
            blocks = (size >> 4) + 1;
        } else {
            blocks = size >> 4;
        }
        part->effect = (MENU_PARTS_EFFECT_STRUCT1 *)stack->Alloc(blocks);
    }
}

void Func_SetPartEffectInfo(MENU_PARTS_EFFECT_STRUCT1 *effect, unsigned int type, short *param) {
    int i;

    if (effect == NULL) {
        return;
    }
    effect->type = type;
    for (i = 0; i < 8; i++) {
        effect->param[i] = 0;
        if (param != NULL) {
            effect->param[i] = param[i];
        }
    }
}

void CMenuPosDataForm::SetActionCharaPtr(CActionChara *character, s32 texture_block, s32 secondary_block) {
    chara = character;
    chara_tex_block = texture_block;
    unk_36 = secondary_block;
}

void CMenuPosDataForm::SetRGBACalcParam(int channel, int add, int target) {
    if (channel < 0 || channel > 3) {
        return;
    }
    rgba_add[channel] = add;
    rgba_target[channel] = target;
}

void CMenuPosDataForm::FormFadeIn(int frames, int reset) {
    if (reset != 0) {
        rgba[0] = 0x80;
        rgba[1] = 0x80;
        rgba[2] = 0x80;
        rgba[3] = 0;
        for (int i = 0; i < 4; i++) {
            SetRGBACalcParam(i, 0, 0x80);
        }
    }
    SetRGBACalcParam(3, 0x80 / frames, 0x80);
}

void CMenuPosDataForm::FormFadeOut(int frames, int reset) {
    if (reset != 0) {
        rgba[0] = 0x80;
        rgba[1] = 0x80;
        rgba[2] = 0x80;
        rgba[3] = 0x80;
        for (int i = 0; i < 4; i++) {
            SetRGBACalcParam(i, 0, 0x80);
        }
    }
    SetRGBACalcParam(3, -0x80 / frames, 0);
}

void CMenuPosDataForm::SetNumber(s8 *part_name, s32 number) {
    MENUFORMPARTS_TYPE *part = GetPartInfo(part_name);
    if (part != NULL) {
        part->etc_info[1] = number;
    }
}

void CMenuPosDataForm::SetPartRGBA(char *part_name, int r, int g, int b, int a) {
    MENUFORMPARTS_TYPE *part = GetPartInfo(part_name);
    if (part != NULL) {
        part->rgba[0] = r;
        part->rgba[1] = g;
        part->rgba[2] = b;
        part->rgba[3] = a;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GetPutPosXY__16CMenuPosDataFormFPcRiRi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GetPutPosXY__16CMenuPosDataFormFPcRfRf);

MENUFORMPARTS_TYPE *CMenuPosDataForm::GetEnableEnterPart() {
    for (int i = 0; i < parts_num; i++) {
        if (parts[i].name == NULL && parts[i].active == 0) {
            return &parts[i];
        }
    }
    return NULL;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GetNowPosRGBA__16CMenuPosDataFormFP18MENUFORMPARTS_TYPEP16MENU_BASETEXINFOPfPUc);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuPartsStep__16CMenuPosDataFormFv);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", DrawItemIconEffect2__FP11mgCDrawPrimP10mgCTextureP18MENUFORMPARTS_TYPE9mgRect_f_);

void MenuItemBrdSetInfo(int unk, int top_line, int max_line, int view_line) {
    float hidden_lines;

    MenuItemBrdMaxLine = max_line;
    MenuItemBrdViewLine = view_line;
    hidden_lines = (float)(max_line - view_line);
    if (hidden_lines < 1.0f) {
        hidden_lines = 1.0f;
    }
    MenuItemBrdScrlCurLen = 256.0f / hidden_lines;
    MenuItemBrdCalcManner = 1;
    Func_MenuItemBrdPosStep(top_line);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuItemBrdFrameDraw__FiiRiiiii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuItemBrdDraw__FPf9mgRect_i_Riiiii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuItemModeItemDraw__FRi9mgRect_i_PfP18MENUFORMPARTS_TYPEP10mgCTexture9mgRect_i_i);

int CMenuPosDataForm::MenuFormStep() {
    int ended = 0;
    int pos[2];
    int value;
    int i;

    if (step_stop != 0) {
        return 0;
    }
    GetNextMovePos(pos);
    counter += 1;
    if (counter > 100000) {
        counter = 0;
    }
    if (dtype == MENUFORM_DTYPE_POLY || dtype == MENUFORM_DTYPE_MAPPART) {
        if (counter > 15) {
            counter = 15;
        }
    }
    if (dtype == MENUFORM_DTYPE_BG_TILE) {
        // The tiled background scrolls by its part's etc_info every other frame and wraps after one tile.
        MENUFORMPARTS_TYPE *part = parts;
        MENU_BASETEXINFO *tex_info = MenuPosData->GetTexGetInfo(part->tex_info_no);
        if (counter % 2 != 0) {
            pos[0] += part->etc_info[0];
            pos[1] += part->etc_info[1];
        }
        if (part->etc_info[0] < 0) {
            if (pos[0] <= -tex_info->rect.right) {
                pos[0] = 0;
            }
        } else if (part->etc_info[0] > 0 && pos[0] >= 0) {
            pos[0] = -tex_info->rect.right;
        }
        if (part->etc_info[1] < 0) {
            if (pos[1] <= -tex_info->rect.bottom) {
                pos[1] = 0;
            }
        } else if (part->etc_info[1] > 0 && pos[1] >= 0) {
            pos[1] = -tex_info->rect.bottom;
        }
    } else if (dtype != MENUFORM_DTYPE_MSGFORM) {
        MenuPartsStep();
    }
    if (CheckMoveEnd(pos[0], pos[1]) != 0) {
        ended = 1;
        action_state = 4;
    }
    for (i = 0; i < 4; i++) {
        s8 add = rgba_add[i];
        if (add != 0) {
            value = rgba[i];
            if (CalcMenuAdd(&value, add, rgba_target[i]) != 0) {
                rgba_add[i] = 0;
            }
            rgba[i] = value;
        }
    }
    x = (float)pos[0];
    y = (float)pos[1];
    return ended;
}

s32 CMenuPosDataForm::CheckMoveEnd(s32 target_x, s32 target_y) {
    s32 finished = 0;
    if (x == (float) target_x) {
        finished = 1;
        if (y != (float) target_y) {
            finished = 0;
        }
    }
    return finished;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", CheckMoveEnd__16CMenuPosDataFormFv);

void CMenuPosDataForm::SetAction(char *action_name) {
    for (int i = 0; i < action_num; i++) {
        if (strcmp(action_name, action[i].name) == 0) {
            action_no = i;
            action_state = 1;
            return;
        }
    }
    action_no = -1;
}

void CMenuPosDataForm::SetNextMovePos(s32 *position, s32 move_type) {
    mtype = move_type;
    next_x = position[0];
    next_y = position[1];
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GetNextMovePos__16CMenuPosDataFormFPi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Menu3DivideTextureDraw__FP11mgCDrawPrim9mgRect_i_Psi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuFormDrawNormal__16CMenuPosDataFormFiiffRi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuFormDraw__16CMenuPosDataFormFiiRi);

void CMenuPosDataForm::MenuFormDraw(int &tex_block) {
    MenuFormDraw((int)x, (int)y, tex_block);
}

void CPosDataManage::Initialize(void) {
    etc_tbl = NULL;
    etc_tbl_num = 0;
    tex_info = NULL;
    tex_info_num = 0;
    form = NULL;
    form_num = 0;
    step_stop = 0;
}

MENU_BASETEXINFO *CPosDataManage::GetTexGetInfo(int no) {
    if (no < 0 || tex_info_num <= no) {
        return NULL;
    }
    return &tex_info[no];
}

MENU_BASETEXINFO *CPosDataManage::GetTexGetInfo(char *info_name) {
    int i;

    if (info_name == NULL) {
        return NULL;
    }
    for (i = 0; i < tex_info_num; i++) {
        if (tex_info[i].name != NULL && strcmp(info_name, tex_info[i].name) == 0) {
            return &tex_info[i];
        }
    }
    return NULL;
}

int CPosDataManage::GetTexGetInfoTblNo(char *info_name) {
    int i;

    if (info_name != NULL) {
        for (i = 0; i < tex_info_num; i++) {
            if (tex_info[i].name != NULL && strcmp(tex_info[i].name, info_name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

void CPosDataManage::TexGetInfoClear(int start, int end) {
    int i;

    if (end > tex_info_num) {
        end = tex_info_num;
    }
    for (i = start; i < end; i++) {
        MENU_BASETEXINFO_Init(&tex_info[i]);
    }
}

void CPosDataManage::ResetTextureBlockNo(char *tex_name, int tex_block) {
    MENU_BASETEXINFO *info;
    int i;

    info = GetTexGetInfo(0);
    for (i = 0; i < tex_info_num; i++, info++) {
        if (info->tex_name != NULL && strcmp(info->tex_name, tex_name) == 0) {
            info->tex_block = tex_block;
        }
    }
}

void CPosDataManage::ResetTextureInfoAll() {
    mgCTextureManager *manager = &mgTexManager;
    CMenuPosDataForm *form;
    MENUFORMPARTS_TYPE *part;
    MENU_BASETEXINFO *info;
    int i;

    form = GetDrawTopList();
    while (form != NULL) {
        part = form->parts;
        for (i = 0; i < form->parts_num && part != NULL; i++, part++) {
            info = GetTexGetInfo(part->tex_info_no);
            if (info != NULL) {
                part->tex = manager->GetTexture(info->tex_name, info->tex_block);
            }
        }
        form = form->next;
    }
}

void CPosDataManage::EtcTblClear(int start, int end) {
    MENU_ETCINFO *info;
    int i;

    if (end > etc_tbl_num) {
        end = etc_tbl_num;
    }
    info = &etc_tbl[start];
    for (i = 0; i < end - start; i++, info++) {
        info->name = NULL;
    }
}

MENU_ETCINFO *CPosDataManage::GetEtcTbl(char *info_name) {
    MENU_ETCINFO *info;
    int i;
    int num;

    if (info_name == NULL) {
        return NULL;
    }
    num = etc_tbl_num;
    info = etc_tbl;
    for (i = 0; i < num; i++, info++) {
        if (info->name != NULL && strcmp(info->name, info_name) == 0) {
            return info;
        }
    }
    return NULL;
}

void CPosDataManage::GetEtcTblValue(char *info_name, int &out_value0, int &out_value1) {
    MENU_ETCINFO *info = GetEtcTbl(info_name);

    if (info == NULL) {
        if (&out_value0 != NULL) {
            out_value0 = 0;
        }
        if (&out_value1 != NULL) {
            out_value1 = 0;
        }
        return;
    }
    if (&out_value0 != NULL) {
        out_value0 = info->value[0];
    }
    if (&out_value1 != NULL) {
        out_value1 = info->value[1];
    }
}

MENU_ETCINFO2 *CPosDataManage::GetEtcTbl2(char *info_name) {
    MENU_ETCINFO2 *info;
    int i;
    int num;

    if (info_name == NULL) {
        return NULL;
    }
    num = etc_tbl2_num;
    info = etc_tbl2;
    for (i = 0; i < num; i++, info++) {
        if (info->name != NULL && strcmp(info->name, info_name) == 0) {
            return info;
        }
    }
    return NULL;
}

void CPosDataManage::GetEtcTbl2Value(char *info_name, float *out_values, int num) {
    MENU_ETCINFO2 *info = GetEtcTbl2(info_name);
    int i;

    if (info == NULL) {
        return;
    }
    for (i = 0; i < num; i++) {
        out_values[i] = info->value[i];
    }
}

void CPosDataManage::EtcTbl2Clear(int start, int end) {
    MENU_ETCINFO2 *info;
    int num;
    int i;

    if (end > etc_tbl2_num) {
        end = etc_tbl2_num;
    }
    num = end - start;
    info = &etc_tbl2[start];
    for (i = 0; i < num; i++, info++) {
        info->name = NULL;
    }
}

char *GetMenuMainIconChar(int no) {
    static char icon_name[32];

    sprintf(icon_name, "mi%d", no - 2);
    return icon_name;
}

CMenuPosDataForm *CPosDataManage::GetFormInfo(char *form_name) {
    CMenuPosDataForm *form;
    int i;

    if (form_name == NULL) {
        return NULL;
    }
    form = GetDrawTopList();
    for (i = 0; form != NULL && i < form_num; i++) {
        if (form->name == NULL) {
            break;
        }
        if (strcmp(form->name, form_name) == 0) {
            return form;
        }
        form = form->next;
    }
    return NULL;
}

CMenuPosDataForm *CPosDataManage::GetFormInfo(int no) {
    if (no < 0 || form_num <= no) {
        return NULL;
    }
    return &form[no];
}

void CPosDataManage::FormInfoClear(int start, int end) {
    int i;

    if (start < 0) {
        start = 0;
    }
    if (end >= form_num) {
        end = form_num;
    }
    for (i = start; i < end; i++) {
        form[i].Initialize();
    }
}

void CPosDataManage::SetFormPos(char *form_name, int *pos) {
    CMenuPosDataForm *form = GetFormInfo(form_name);
    if (form != NULL) {
        form->x = (float)pos[0];
        form->y = (float)pos[1];
    }
}

void CPosDataManage::InitDrawList() {
    CMenuPosDataForm *form = this->form;
    CMenuPosDataForm *prev = NULL;
    CMenuPosDataForm *next;
    int i = 0;
    int j;

    // Links the named forms in table order, skipping the unused slots.
    while (i < form_num) {
        form->prev = prev;
        next = NULL;
        for (j = 1; j < form_num - i; j++) {
            if (form[j].name != NULL) {
                i += j;
                next = &form[j];
                break;
            }
        }
        form->next = next;
        prev = form;
        form = next;
        if (form == NULL) {
            break;
        }
    }
}

CMenuPosDataForm *CPosDataManage::GetDrawTopList() {
    CMenuPosDataForm *form = GetFormInfo(0);
    CMenuPosDataForm *prev;

    if (form != NULL) {
        do {
            prev = form->prev;
            if (prev == NULL) {
                return form;
            }
            form = prev;
        } while (prev != NULL);
    }
    return NULL;
}

void CPosDataManage::FormReLink(char *form_name0, char *form_name1) {
    CMenuPosDataForm *form0 = GetFormInfo(form_name0);
    CMenuPosDataForm *form1 = GetFormInfo(form_name1);
    CMenuPosDataForm *prev;
    CMenuPosDataForm *next;

    if (form0 == NULL || form1 == NULL) {
        return;
    }
    prev = form0->prev;
    next = form0->next;
    form0->prev = form1->prev;
    form0->next = form1->next;
    form1->prev = prev;
    form1->next = next;
    if (form1->prev != NULL) {
        form1->prev->next = form1;
    }
    if (form1->next != NULL) {
        form1->next->prev = form1;
    }
    if (form0->next != NULL) {
        form0->next->prev = form0;
    }
    if (form0->prev != NULL) {
        form0->prev->next = form0;
    }
}

void CPosDataManage::FormReLink2(char *first0, char *last0, char *first1, char *last1) {
    CMenuPosDataForm *head0 = GetFormInfo(first0);
    CMenuPosDataForm *head1 = GetFormInfo(first1);
    CMenuPosDataForm *tail0 = GetFormInfo(last0);
    CMenuPosDataForm *tail1 = GetFormInfo(last1);
    CMenuPosDataForm *after0;
    CMenuPosDataForm *after1;
    CMenuPosDataForm *before1;
    CMenuPosDataForm *before0;

    // Swaps the runs head0..tail0 and head1..tail1 in the draw list.
    if (head0 == NULL || head1 == NULL) {
        return;
    }
    before0 = head0->prev;
    after0 = NULL;
    before1 = head1->prev;
    after1 = NULL;
    if (tail0 != NULL) {
        after0 = tail0->next;
    }
    if (tail1 != NULL) {
        after1 = tail1->next;
    }
    if (tail0 == before1) {
        head1->prev = before0;
        before0->next = head1;
        head0->prev = tail1;
        tail1->next = head0;
        tail0->next = after1;
        after1->prev = tail0;
    } else if (tail1 == before0) {
        head0->prev = before0;
        before1->next = head0;
        head1->prev = tail0;
        tail0->next = head1;
        tail1->next = after0;
        after0->prev = tail1;
    } else {
        head1->prev = before0;
        if (before0 != NULL) {
            before0->next = head1;
        }
        if (after0 != NULL) {
            tail1->next = after0;
            after0->prev = tail1;
        }
        head0->prev = before1;
        if (before1 != NULL) {
            before1->next = head0;
        }
        if (after1 != NULL) {
            tail0->next = after1;
            after1->prev = tail0;
        }
    }
}

void CPosDataManage::FormStep() {
    CMenuPosDataForm *form = GetDrawTopList();
    int stopped;

    while (form != NULL) {
        stopped = form->step_stop;
        if (step_stop != 0) {
            form->step_stop = 1;
        }
        form->MenuFormStep();
        if (stopped == 0) {
            form->step_stop = 0;
        }
        form = form->next;
        if (form == NULL) {
            break;
        }
    }
}

void MenuDrawParamStep() {
    DrawMenuWakuStep();
    EnableUseItemAlphaStep();
    DrawItemCounter += 0.25f;
    if (DrawItemCounter >= 10.0f) {
        DrawItemCounter = 0.0f;
    }
    DrawItemDefCounter += 1;
    if (DrawItemDefCounter >= 0x50) {
        DrawItemDefCounter = 0;
    }
}

void CPosDataManage::FormDraw() {
    int tex_block = -1;
    CMenuPosDataForm *form = GetDrawTopList();

    while (form != NULL) {
        form->MenuFormDraw(tex_block);
        form = form->next;
        if (form == NULL) {
            break;
        }
    }
}

void CPosDataManage::ClearPos(void) {
    EtcTblClear(0, etc_tbl_num);
    TexGetInfoClear(0, tex_info_num);
    FormInfoClear(0, form_num);
}

void CMenuPosDataManage::AttachCommonTexInfo() {
    common_tex = mgTexManager.GetTexture("menuwork2", -1);
    unk_40 = NULL;
    icon_effect_tex = mgTexManager.GetTexture("menueff0", -1);
    effect_tex = mgTexManager.GetTexture("spectre", -1);
    item_icon_tex[0][0] = mgTexManager.GetTexture("wepicon", -1);
    item_icon_tex[0][1] = mgTexManager.GetTexture("itemicon", -1);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", StepMainMenuIconMove__18CMenuPosDataManageFPiii);

int CheckItemUseVariable(CGameDataUsed *item, CItemUseTarget *target) {
    int state;
    int result;

    if (item == NULL || target == NULL) {
        return 0;
    }
    state = CheckNowStateUseThisItem(item, target);
    result = state;
    if (CheckBuildUp((CGameDataUsed *)target->target.data, NULL, NULL, NULL) != 0) {
        result = state | 2;
    }
    return result;
}

void Func_MenuItemBrdPrepare(MENUFORMPARTS_TYPE *parts, CGameDataUsed *items, CGameDataUsed *target, int num) {
    int count;
    int i;

    if (parts != NULL) {
        CItemUseTarget use_target;
        count = GetNowBagMax(1);
        for (i = 0; i < count; i++, parts++) {
            use_target.SetPtr(num, &items[i]);
            parts->item_flag = CheckItemUseVariable(target, &use_target);
        }
    }
}

void Func_MenuItemBrdPrepare2(MENUFORMPARTS_TYPE *parts, CGameDataUsed *items, CGameDataUsed *target) {
    int count;
    int i;
    int item_no;

    if (parts == NULL) {
        return;
    }
    if (target == NULL) {
        return;
    }
    count = GetNowBagMax(1);
    item_no = target->item_no;
    for (i = 0; i < count; i++, parts++) {
        if (item_no == 0x17D) {
            parts->item_flag = 0;
        } else {
            CItemUseTarget use_target;
            use_target.SetPtr(1, &items[i]);
            parts->item_flag = CheckItemUseVariable(target, &use_target);
        }
    }
}

int NowUseNeedItemCheck(CUserDataManager *user) {
    int needs;
    int in_battle;
    int limit;
    int party;
    int active_chara;
    CHARA_DATA *charas[2];
    ROBO_DATA *robo;
    CHARA_DATA *chara;
    CGameDataUsed *weapon;

    if (user == NULL) {
        return 0;
    }
    needs = 0;
    in_battle = 0;
    if ((GetMainScene()->battle_area.floor_status & 4) != 0) {
        in_battle = 1;
    }
    active_chara = user->active_chr_no;
    party = user->GetNowPartyMember();
    charas[0] = user->GetCharaDataPtr(USER_CHARA_MAX);
    charas[1] = user->GetCharaDataPtr(USER_CHARA_MONICA);
    if ((party & 4) != 0) {
        robo = &user->robo_data;
        if (robo != NULL) {
            if (robo->AddPoint(0.0f) < 0.2f) {
                needs |= 0x80;
            }
        }
        limit = GetShiledKitLimmit(user->CheckRobotCore());
        if (robo->shield_kit_num < limit) {
            needs |= 0x10000;
        }
    }
    if (active_chara == USER_CHARA_MAX || active_chara == USER_CHARA_MONICA) {
        chara = charas[active_chara];
        if (active_chara == USER_CHARA_MAX ||
            (active_chara == USER_CHARA_MONICA && (party & 2) != 0)) {
            if (chara->hp.GetRate() < 0.2f) {
                needs |= 1;
            }
            if ((chara->status_attr & CHARA_STATUS_POISON) != 0) {
                needs |= 0x100;
            }
            if ((chara->status_attr & CHARA_STATUS_UNK_2) != 0) {
                needs |= 0x200;
            }
            if ((chara->status_attr & CHARA_STATUS_UNK_4) != 0) {
                needs |= 0x400;
            }
            if ((chara->status_attr & CHARA_STATUS_UNK_8) != 0) {
                needs |= 0x800;
            }
            if ((chara->status_attr & CHARA_STATUS_POWER) != 0) {
                needs |= 0x1000;
            }
            if ((chara->status_attr & CHARA_STATUS_UNK_20) != 0) {
                needs |= 0x2000;
            }
            if ((chara->status_attr & CHARA_STATUS_UNK_40) != 0) {
                needs |= 0x4000;
            }
        }
        weapon = &chara->equip[0];
        if (weapon->GetWHp(NULL) < 0.2f) {
            needs |= 2;
        }
        if (chara->equip[1].GetWHp(NULL) < 0.2f) {
            if (active_chara == USER_CHARA_MAX) {
                needs |= 4;
            }
            if (active_chara == USER_CHARA_MONICA) {
                needs |= 8;
            }
        }
    } else if (active_chara == USER_CHARA_ROBO) {
        robo = &user->robo_data;
        if (robo->parts[0].GetWHp(NULL) < 0.2f) {
            needs |= 0x8000;
        }
    } else if (active_chara == USER_CHARA_MONSTER) {
        if (charas[1]->hp.GetRate() < 0.2f) {
            needs |= 1;
        }
    }
    needs |= 0x30;
    if ((party & 2) == 0) {
        needs &= ~0x20;
    }
    if (in_battle != 0) {
        needs &= ~0x6F01;
    }
    return needs | 0x40;
}

void Func_MenuIconDrawPrepare(MENUFORMPARTS_TYPE *part, CGameDataUsed *item, int unk) {
    int item_no;
    CDataCommon *common;
    CDataItem *info;
    u32 flags;

    if (part != NULL && item != NULL) {
        part->item_flag = 0;
        item_no = item->item_no;
        if (item_no >= 0x10C) {
            common = GetCommonItemData(item_no);
            if (common != NULL && (common->attribute & 0x20) != 0) {
                info = GetItemInfoData(item_no);
                if (info != NULL) {
                    flags = info->use_flags;
                    if ((flags & 0x100) != 0 && (unk & 0x1) != 0) {
                        part->item_flag |= 1;
                    } else if (((flags & 0x20000) != 0 && (unk & 0x100) != 0) ||
                               ((flags & 0x80000) != 0 && (unk & 0x400) != 0) ||
                               ((flags & 0x8000) != 0 && (unk & 0x800) != 0) ||
                               ((flags & 0x200000) != 0 && (unk & 0x200) != 0) ||
                               ((flags & 0x4000000) != 0 && (unk & 0x2000) != 0) ||
                               ((flags & 0x10000000) != 0 && (unk & 0x4000) != 0)) {
                        part->item_flag |= 1;
                    } else if ((flags & 0x400) != 0 &&
                               ((item_no == 0x126 && ((unk & 0x2) != 0 || (unk & 0x8000) != 0)) ||
                                (item_no == 0x12A && (unk & 0x4) != 0) ||
                                (item_no == 0x160 && (unk & 0x8) != 0) ||
                                (item_no == 0x17D && (unk & 0x80) != 0))) {
                        part->item_flag |= 1;
                    } else if (item_no == 0x1A7 && (unk & 0x10000) != 0) {
                        part->item_flag |= 1;
                    } else if (item_no == 0x128 || item_no == 0x184) {
                        part->item_flag |= 1;
                    } else if (item_no == 0x185 && (unk & 0x20) != 0) {
                        part->item_flag |= 1;
                    }
                }
            }
        }
    }
}

void CheckItemBoardFunc_MenuIconDrawPrepare(CUserDataManager *user, MENUFORMPARTS_TYPE *parts) {
    int need_item;
    CGameDataUsed *item;
    int count;
    int i;

    need_item = NowUseNeedItemCheck(user);
    item = user->GetUsedDataPtr(0);
    count = GetNowBagMax(1);
    for (i = 0; i < count; i++) {
        Func_MenuIconDrawPrepare(&parts[i], item, need_item);
        if (CheckBuildUp(item, NULL, NULL, NULL) != 0) {
            parts[i].item_flag |= 2;
        }
        item++;
    }
}

void MenuItemBrdScrlBarStep(int top_line, int y, int manner) {
    int pos;

    pos = CalcScrlBarPutPos(y, 252.0f, top_line, (float)(MenuItemBrdMaxLine - MenuItemBrdViewLine));
    if (manner == 0) {
        CalcMenu1((float)pos, &MenuItemBrdScrlBarY, 4.0f, 4.0f, 0);
    }
    if (manner == 1) {
        MenuItemBrdScrlBarY = (float)pos;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Func_MenuItemBrdPosStep__Fi);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GetPosMenuItemBrdKoma__18CMenuPosDataManageFPiii);

void CMenuPosDataManage::GetPosMenuItemOnItemBrd(s32 *pos, s32 item_no, s32 clip) {
    this->GetPosMenuItemBrdKoma(pos, item_no, clip);
    pos[0] += 4;
    pos[1] += 4;
}

void CMenuPosDataManage::GetPosMenuItemBrdForEffect(s32 *pos, s32 item_no, s32 clip) {
    this->GetPosMenuItemOnItemBrd(pos, item_no, clip);
    pos[0] += 0x12;
    pos[1] += 0x15;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Func_MenuItemIconSetEffectOne__FP18MENUFORMPARTS_TYPE);

void MenuItemBrdItemIconEffectMalloc(mgCMemory *stack, MENUFORMPARTS_TYPE *parts, int num) {
    int i;

    for (i = 0; i < num; i++) {
        MenuPosDataTypeInit(&parts[i]);
        parts[i].active = 1;
        Func_MallocPartEffectInfo(&parts[i], stack, 8);
        Func_MenuItemIconSetEffectOne(&parts[i]);
    }
    parts->name = (char *)stack->Alloc(1);
    strcpy(parts->name, "icon");
    parts->w = 32.0f;
    parts->h = 40.0f;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MallocPallet__18CMenuPosDataManageFP9mgCMemory);

void CMenuPosDataManage::SearchTransPalletNo() {
    u8 *color;
    int i;
    int j;

    // Finds the first fully transparent entry of each item icon palette.
    for (i = 0; i < 2; i++) {
        if (item_icon_tex[0][i] != NULL) {
            color = (u8 *)item_icon_tex[0][i]->clut;
            for (j = 0; j < 256; j++) {
                if (color[3] == 0) {
                    trans_pallet_no[i] = j;
                    break;
                }
                color += 4;
            }
        }
    }
}

void CMenuPosDataManage::InitializeCMenuPosDataManage() {
    Initialize();
    common_tex = NULL;
    unk_40 = NULL;
    unk_44 = NULL;
    unk_48 = NULL;
    icon_effect_tex = NULL;
    effect_tex = NULL;
    item_icon_tex[0][0] = NULL;
    item_icon_tex[0][1] = NULL;
    item_icon_tex[1][0] = NULL;
    item_icon_tex[1][1] = NULL;
    pallet[2][0] = NULL;
    pallet[2][1] = NULL;
    item_icon_tex[3][0] = NULL;
    item_icon_tex[3][1] = NULL;
    memset(unk_74, 0, sizeof(unk_74));
    memset(unk_2cc, 0, sizeof(unk_2cc));
    memset(unk_524, 2, sizeof(unk_524));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuCapture__FiP9mgCMemoryi);

void SetBGFrameForMenu(int tex_block, char *tex_name) {
    mgTexManager.ReloadTexture(tex_block, (sceVif1Packet *)NULL);
    mgCTexture frame;
    mgRect<int> src_rect;
    mgRect<int> half_rect;
    mgCDrawPrim *prim;
    mgCTexture *background;

    mgGetFrameBuffer(&frame);
    src_rect.Set(0, 0, mgScreenWidth << 4, mgScreenHeight << 4);
    half_rect.Set(0, 0, mgScreenWidth << 3, mgScreenHeight << 3);
    prim = GetMenuPrim();
    prim->Initialize(NULL, NULL);
    prim->AlphaTestEnable(0);
    prim->ZMask(-1);
    prim->TextureMapEnable(1);
    background = mgTexManager.GetTexture(tex_name, -1);
    background->Bilinear(1);
    mgSetPkMoveImage(&frame, src_rect, background, 0, 0, 0);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", MenuFrameImageDraw__FP11mgCDrawPrimP10mgCTexture9mgRect_f_9mgRect_i_iii);

void CRepairEffect::Initialize(void) {
    active = 0;
    particle = NULL;
    unk_1c = NULL;
    tex = NULL;
    particle_num = 0;
}

void CRepairEffect::Generate(mgCMemory *stack, int num) {
    unsigned int size;
    unsigned int blocks;
    int i;
    REPAIR_EFFECT_PARTICLE *spark;
    int side;

    alpha = 0x80;
    counter = 0;
    active = 1;
    particle_num = num;
    size = particle_num * sizeof(REPAIR_EFFECT_PARTICLE);
    if ((size & 0xF) != 0) {
        blocks = (size >> 4) + 1;
    } else {
        blocks = size >> 4;
    }
    particle = new (stack->Alloc(blocks + 2)) REPAIR_EFFECT_PARTICLE[particle_num];
    for (i = 0; i < particle_num; i++) {
        spark = &particle[i];
        spark->active = 1;
        spark->unk_0 = 128.0f;
        spark->unk_4 = 128.0f;
        spark->unk_8 = 64.0f;
        spark->alpha = 90.0f + GetRandF(20.0f);
        spark->y = (float)y + GetRandF(40.0f) - 22.0f;
        side = GetRandI(34);
        spark->x = (float)(x + side);
        spark->vx = GetRandF(0.16f);
        if (side < 19) {
            spark->vx = -spark->vx;
        }
        spark->unk_14 = 0;
        spark->counter = 0;
    }
}

void CRepairEffect::Step() {
    REPAIR_EFFECT_PARTICLE *spark;
    int alive;
    int i;

    if (active != 0) {
        alive = 0;
        alpha -= 2;
        if (alpha < 0) {
            alpha = 0;
        }
        for (i = 0; i < particle_num; i++) {
            spark = &particle[i];
            if (spark->active != 0) {
                spark->counter++;
                spark->x += spark->vx;
                spark->y += 0.5f;
                spark->alpha -= 1.7f;
                if (spark->alpha <= 0.0f) {
                    spark->active = 0;
                }
                alive++;
            }
        }
        if (alive == 0) {
            active = 0;
        }
        counter++;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Draw__13CRepairEffectFv);

void CRepairManager::Initialize() {
    int i;

    data_ready = 0;
    tex_block = -1;
    unk_1a4 = NULL;
    tex = NULL;
    model = NULL;
    data = NULL;
    for (i = 0; i < 8; i++) {
        effect[i] = NULL;
        effect_stack[i].stSetBuffer(NULL, 0);
    }
    keep = 0;
}

void CRepairManager::SetStack(mgCMemory *stack, int mode) {
    int i;
    u_long128 *top = &stack->stack[stack->stack_used] + (stack->stack_size - stack->stack_used);

    // Carves eight effect stacks and the model stack off the top of the free stack space, or takes
    // the effect stacks from its bottom.
    for (i = 0; i < 8; i++) {
        if (mode == 0) {
            top -= 0x500;
            effect_stack[i].stSetBuffer(top - 0x500, 0x500);
        }
        if (mode == 1) {
            top = stack->stGetTop();
            effect_stack[i].stSetBuffer(top, 0x500);
            stack->Alloc(0x501);
        }
    }
    if (mode == 0) {
        model_stack.stSetBuffer(top - 0x1800, 0x1800);
    }
}

void CRepairManager::Clear(void) {
    this->Initialize();
}

void CRepairManager::LoadDataBG(mgCMemory *stack) {
    int size;
    unsigned int blocks;

    bg_load = 0;
    if (data_ready == 0 || data == NULL) {
        stack->stReset();
        stack->Align64();
        size = 0;
        data = (u32 *)stack->stGetTop();
        StartReadBG();
        LoadFileBG("menu/eff/repair.chr", (u_long128 *)data, &size);
        if (((unsigned int)size & 0xF) != 0) {
            blocks = ((unsigned int)size >> 4) + 1;
        } else {
            blocks = (unsigned int)size >> 4;
        }
        stack->Alloc(blocks);
        SetStack(stack, 0);
        bg_load = 1;
    }
}

void CRepairManager::CheckDataBG(int new_tex_block) {
    int size;

    if (data_ready == 0) {
        mgTexManager.DeleteBlock(new_tex_block);
        tex_block = new_tex_block;
        MenuEnterIMG(new_tex_block, (u8 *)GetPackFile(data, "repair_powder2d.img", &size), "_2");
        tex = mgTexManager.GetTexture("menueff0_2", -1);
        data_ready = 1;
        bg_load = 0;
    }
}

void CRepairManager::SetRepairData(mgCMemory *stack, int new_tex_block, u32 *new_data) {
    int size;

    tex_block = new_tex_block;
    data = new_data;
    MenuEnterIMG(new_tex_block, (u8 *)GetPackFile(new_data, "repair_powder2d.img", &size), "_2");
    tex = mgTexManager.GetTexture("menueff0_2", -1);
    data_ready = 1;
    bg_load = 0;
    SetStack(stack, 1);
    keep = 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", GeneratePoly__14CRepairManagerFPfi);

void CRepairManager::Generate(int x, int y) {
    int i;
    int slot;
    CRepairEffect *repair;
    mgCMemory *stack;

    slot = -1;
    for (i = 0; i < 8; i++) {
        if (effect[i] == NULL) {
            slot = i;
            break;
        }
    }
    if (0 <= slot) {
        effect_stack[slot].stReset();
        stack = &effect_stack[slot];
        effect[slot] = new (stack->Alloc(5)) CRepairEffect;
        if ((repair = effect[slot]) != NULL) {
            repair->Initialize();
            repair->x = x;
            repair->y = y;
            repair->unk_1c = unk_1a4;
            repair->tex = tex;
            repair->Generate(stack, 0x20);
        }
    }
}

s32 CRepairManager::IsRunModel(void) {
    return model != NULL;
}

s32 CRepairManager::IsRun(void) {
    s32 i;
    s32 running;

    running = 0;
    for (i = 0; i < 8; i++) {
        if (effect[i] != NULL) {
            running = 1;
        }
    }
    if (this->IsRunModel() != 0) {
        running = 1;
    }
    return running;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Step__14CRepairManagerFv);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Draw__14CRepairManagerFv);

void CLevelUpEffect::Initialize(void) {
    active = 0;
    chara = NULL;
    tex = NULL;
}

void CLevelUpEffect::Generate(mgCTexture *new_tex, int new_kind, int x, int y) {
    tex = new_tex;
    kind = new_kind;
    pos[0] = (float)(x - 0x10);
    pos[1] = (float)y;
    counter = 0;
    chara = NULL;
    active = 1;
}

void CLevelUpEffect::Generate(mgCTexture *new_tex, int new_kind, CCharacter2 *new_chara) {
    int i;

    active = 1;
    kind = new_kind;
    chara = new_chara;
    tex = new_tex;
    chara->GetPosition(pos);
    for (i = 0; i < 0x20; i++) {
        l_levelup_pos[i][0] = (pos[0] + GetRandF(14.0f)) - 7.0f;
        l_levelup_pos[i][1] = (pos[1] - 3.0f) + GetRandF(3.0f);
        l_levelup_pos[i][2] = (pos[2] + GetRandF(14.0f)) - 7.0f;
        l_levelup_vec[i][0] = GetRandF(0.3f) - 0.15f;
        l_levelup_vec[i][1] = GetRandF(0.5f);
        l_levelup_vec[i][2] = GetRandF(0.3f) - 0.15f;
        l_levelup_counter[i] = GetRandI(0x15);
        l_levelup_generate_counter[i] = GetRandI(3) + 2;
    }
}

int CLevelUpEffect::IsRun(void) {
    return active;
}

#ifdef NONMATCHING
void CLevelUpEffect::Step() {
    int i;
    float *spark_pos;
    float *spark_vec;
    s8 *spark_counter;
    s8 *generate_counter;
    int alive;

    if (active != 0) {
        if (chara != NULL) {
            // Each spark rises and is born again at the character a few times before it dies.
            alive = 0;
            for (i = 0; i < 32; i++) {
                generate_counter = &l_levelup_generate_counter[i];
                if (*generate_counter > 0) {
                    spark_pos = l_levelup_pos[i];
                    spark_vec = l_levelup_vec[i];
                    spark_counter = &l_levelup_counter[i];
                    spark_pos[0] += spark_vec[0];
                    spark_pos[1] += spark_vec[1];
                    spark_pos[2] += spark_vec[2];
                    (*spark_counter)--;
                    if (*spark_counter < 0) {
                        spark_pos[0] = pos[0] + GetRandF(14.0f) - 7.0f;
                        spark_pos[1] = pos[1] - 3.0f + GetRandF(3.0f);
                        spark_pos[2] = pos[2] + GetRandF(14.0f) - 7.0f;
                        spark_vec[0] = GetRandF(0.3f) - 0.15f;
                        spark_vec[1] = GetRandF(0.4f);
                        spark_vec[2] = GetRandF(0.3f) - 0.15f;
                        *spark_counter = GetRandI(6) + 16;
                        (*generate_counter)--;
                    }
                    alive++;
                }
            }
            if (alive <= 0) {
                active = 0;
                chara = NULL;
            }
        } else {
            counter++;
            if (counter > 30) {
                active = 0;
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Step__14CLevelUpEffectFv);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Draw__14CLevelUpEffectFv);

void CLevelUpEffectManager::Initialize() {
    int i;

    for (i = 0; i < 8; i++) {
        effect[i].Initialize();
    }
    label_tex = NULL;
}

int CLevelUpEffectManager::IsRun() {
    int i;

    for (i = 0; i < 8; i++) {
        if (effect[i].IsRun() != 0) {
            return 1;
        }
    }
    return 0;
}

void CLevelUpEffectManager::Generate(int kind, int x, int y) {
    int i;

    for (i = 0; i < 8; i++) {
        if (effect[i].IsRun() == 0) {
            effect[i].Generate(label_tex, kind, x, y);
            break;
        }
    }
}

void CLevelUpEffectManager::Generate(int kind, CCharacter2 *chara) {
    int i;

    for (i = 0; i < 8; i++) {
        if (effect[i].IsRun() == 0) {
            effect[i].Generate(spark_tex, kind, chara);
            break;
        }
    }
}

void CLevelUpEffectManager::Step() {
    int i;

    for (i = 0; i < 8; i++) {
        effect[i].Step();
    }
}

void CLevelUpEffectManager::Draw() {
    int i;

    for (i = 0; i < 8; i++) {
        effect[i].Draw();
    }
}

void CStarDust::Generate(int new_x, int new_y, int base_life, int rand_life) {
    x = (float)new_x;
    y = (float)new_y;
    life = base_life + GetRandI(rand_life);
    active = 1;
}

void CStarDust::Step() {
    if (active != 0) {
        life -= 1;
        if (life < 0) {
            active = 0;
        }
    }
}

void CStarDust::Draw(mgCTexture *tex, int u, int v) {
    short remaining;
    int alpha;
    mgCDrawPrim *prim;

    if (active != 0 && tex != NULL) {
        remaining = life;
        alpha = 0x80;
        if (remaining < 8) {
            alpha = remaining * 0x10;
        }
        prim = GetMenuPrim();
        SetSpriteEnv(prim, 4);
        prim->Begin(6);
        prim->Texture(tex);
        prim->Color(0x80, 0x80, 0x80, alpha);
        prim->TextureCrd(u, v);
        prim->Vertex(x, y, 0.0f);
        prim->TextureCrd(u + 8, v + 8);
        prim->Vertex(8.0f + x, 8.0f + y, 0.0f);
        prim->End();
    }
}

CStarDust *CheckNotRunStarDust(CStarDust *star, int num) {
    int i;

    if (star == NULL || num <= 0) {
        return NULL;
    }
    for (i = 0; i < num; i++) {
        if (star[i].active == 0) {
            return &star[i];
        }
    }
    return NULL;
}

int CheckRunStarDust(CStarDust *star, int num) {
    int i;

    if (star == NULL || num <= 0) {
        return 0;
    }
    for (i = 0; i < num; i++) {
        if (star[i].active != 0) {
            return 1;
        }
    }
    return 0;
}

void CEffVerticalLine::Generate(float *center, float range, float unk) {
    float half_range;

    speed = 0.005f + GetRandF(0.1f);
    half_range = range / 2.0f;
    pos[0] = (center[0] + GetRandF(1.9f * range)) - 1.9f * half_range;
    pos[1] = center[1] - 0.76f * range;
    pos[2] = (center[2] + GetRandF(1.7f * range)) - 1.7f * half_range;
    pos[3] = center[3];
    w = 1.0f + GetRandF(0.4f);
    h = 0.7f + GetRandF(0.5f);
    r = GetRandF(26.0f);
    g = 72.0f + GetRandF(26.0f);
    b = 96.0f + GetRandF(26.0f);
    alpha = 138.0f + GetRandF(32.0f);
    angle = GetRandF(0.029637668f);
    angle_add = 0.059275337f;
}

void CEffVerticalLine::Step() {
    pos[1] += speed;
    h += 1.5f * speed;
    if (speed < 0.26f) {
        speed += 0.005f;
    } else {
        speed += 0.009f;
    }
    angle += angle_add;
    if (3.1415927f <= angle) {
        angle = 3.1415927f;
    }
}

void CEffVerticalLine::Draw() {
    int screen0[4];
    int screen1[4];
    mgCDrawPrim prim;
    float fade;

    SetSpriteEnv(&prim, 4);
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.Begin(6);
    prim.Texture(MenuVerticalLineTex);
    // A thin line and a faint wide glow around it.
    if (mgTransWorldPrim3DSprite(screen0, screen1, pos, w, h, 0) != 0) {
        fade = alpha * sinf(angle);
        if (fade <= 0.0f) {
            fade = 0.0f;
        }
        prim.Color((int)r, (int)g, (int)b, (int)fade);
        prim.TextureCrd(0, 0x62);
        prim.Vertex4(screen0);
        prim.TextureCrd(0xA, 0x80);
        prim.Vertex4(screen1);
    }
    if (mgTransWorldPrim3DSprite(screen0, screen1, pos, 3.6f * w, 1.5f * h, 0) != 0) {
        fade = alpha * sinf(angle);
        if (fade <= 0.0f) {
            fade = 0.0f;
        }
        fade *= 0.2f;
        prim.Color((int)r, (int)g, (int)b, (int)fade);
        prim.TextureCrd(0, 0x62);
        prim.Vertex4(screen0);
        prim.TextureCrd(0xA, 0x80);
        prim.Vertex4(screen1);
    }
    prim.End();
}

/**
 *
 * Scatters the build-up lines around the character at random heights and phases.
 *
 */
static void InitInitBuildUpInfoEffectPos() {
    int i;

    for (i = 0; i < MenuVerticalLineNum; i++) {
        MenuVerticalLine[i].Generate(MenuVerticalLineCharaPos, MenuVerticalRange, 20.0f);
        MenuVerticalLine[i].pos[1] = MenuVerticalLineCharaPos[1] + GetRandF(8.0f);
        MenuVerticalLine[i].angle = GetRandF(3.1415927f);
    }
}

void InitBuildUpInfoEffect(mgCMemory *stack, mgCTexture *tex, int num, float up_limit) {
    unsigned int size;
    unsigned int blocks;

    MenuVerticalLineTex = tex;
    MenuVerticalLineUpLimmit = up_limit;
    MenuVerticalLine = NULL;
    MenuVerticalLineNum = num;
    MenuVerticalLineChara = NULL;
    if (stack != NULL) {
        size = num * sizeof(CEffVerticalLine);
        blocks = (size & 0xF) != 0 ? (size >> 4) + 1 : size >> 4;
        MenuVerticalLine = new (stack->Alloc(blocks + 2)) CEffVerticalLine[num];
        InitInitBuildUpInfoEffectPos();
    }
}

void SetBuildUpInfoChara(CCharacter2 *chara, float range) {
    int same;

    same = 1;
    if (MenuVerticalLineChara != chara) {
        same = 0;
    }
    MenuVerticalRange = range;
    MenuVerticalLineChara = chara;
    if (chara != NULL) {
        chara->GetPosition(MenuVerticalLineCharaPos);
        MenuVerticalLineCharaPos2[1] = MenuVerticalLineCharaPos[1] - 3.0f;
        if (same != 0) {
            InitInitBuildUpInfoEffectPos();
        }
    }
}

void StepBuildUpInfoEffect() {
    int i;

    if (MenuVerticalLine != NULL) {
        for (i = 0; i < MenuVerticalLineNum; i++) {
            MenuVerticalLine[i].Step();
            if (3.1415927f <= MenuVerticalLine[i].angle || 19.0f <= MenuVerticalLine[i].pos[1]) {
                MenuVerticalLine[i].Generate(MenuVerticalLineCharaPos, MenuVerticalRange, 20.0f);
            }
        }
    }
}

void DrawBuildUpInfoEffect() {
    int i;

    if (MenuVerticalLineChara == NULL || MenuVerticalLine == NULL) {
        return;
    }
    if (MenuVerticalLineTex != NULL) {
        mgTexManager.ReloadTexture(MenuVerticalLineTex->block, (sceVif1Packet *)NULL);
        for (i = 0; i < MenuVerticalLineNum; i++) {
            MenuVerticalLine[i].Draw();
        }
    }
}

void InitFishBoiledEffect(int *pos, mgCTexture *tex) {
    int i;

    fish_boiled_effect_tex = tex;
    fish_boiled_runflag = 0;
    if (pos != NULL) {
        for (i = 0; i < 8; i++) {
            fish_boiled_positin[i][0] = (24.0f + (float)pos[0]) - GetRandF(32.0f);
            fish_boiled_positin[i][1] = (24.0f + (float)pos[1] + GetRandF(12.0f)) - 6.0f;
            fish_boiled_alpha[i] = 132.0f - GetRandF(24.0f);
            fish_boiled_streatch_rate[i] = 0.4f + GetRandF(0.4f);
        }
        fish_boiled_runflag = 1;
    }
    fish_boiled_count = 0;
}

int StepFishBoiledEffect() {
    int ended;
    int i;

    ended = 0;
    if (fish_boiled_runflag == 0) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        fish_boiled_positin[i][1] -= 0.5f;
        fish_boiled_amp_count[i] = mgAngleLimit(0.052359879f + fish_boiled_amp_count[i]);
        float *alpha = &fish_boiled_alpha[i];
        if (CalcMenuAdd(alpha, -2.0f, 0.0f) != 0) {
            ended++;
        }
    }
    if (ended >= 8) {
        fish_boiled_runflag = 0;
        fish_boiled_effect_tex = NULL;
        return 0;
    }
    return 1;
}

void DrawFishBoiledEffect() {
    mgCDrawPrim *prim;
    int i;
    float size;
    float x;

    if (fish_boiled_runflag == 0) {
        return;
    }
    if (fish_boiled_effect_tex == NULL) {
        return;
    }
    prim = GetMenuPrim();
    SetSpriteEnv(prim, 4);
    prim->Begin(6);
    prim->Texture(fish_boiled_effect_tex);
    for (i = 0; i < 8; i++) {
        size = 32.0f * fish_boiled_streatch_rate[i];
        x = fish_boiled_positin[i][0] + 2.0f * sinf(fish_boiled_amp_count[i]);
        prim->Color(0x80, 0x80, 0x80, (int)fish_boiled_alpha[i]);
        prim->TextureCrd(0x20, 0);
        prim->Vertex(x, fish_boiled_positin[i][1], 0.0f);
        prim->TextureCrd(0x40, 0x20);
        prim->Vertex(x + size, size + fish_boiled_positin[i][1], 0.0f);
    }
    prim->End();
}

void SetEffectSpectolBreak(mgCMemory *stack, CMenuEffect *effect, int item) {
    stack->stReset();
    int base[10] = {120, 175, 0, 0, 100, 100, 100, 100, 0, 0};
    mgRect<int> rect;

    rect.Set(0, 0, 0, 0);
    GetMenuItemIconTexGetXY(item, rect);
    mgCTexture *icon_tex = GetMenuItemIconTexInfo(item, 0);
    base[2] = rect.left;
    base[3] = rect.top;
    base[4] = use_trans_rect;
    effect->PresetEffect(stack, icon_tex, 0x13, base);
    effect->EffectStart();
    MenuSePlay(-1);
}

void SetEffectSpectolFusion(mgCMemory *stack, CMenuEffect **effect, CGameDataUsed *item, int unk) {
    int base[10] = {120, 175, 100, 100, 0, 0, 0, 0, 0, 0};
    mgCTexture *icon_tex;

    trans_spectol_pos = GetSameAdrressUserData(item, 0);
    trans_spectol_cnt = 0;
    if (unk != 0) {
        base[4] = 1;
    }
    if (trans_spectol_pos >= 0) {
        itemmenu_chr_rotflag = 0;
    }
    stack->stReset();
    effect[0]->PresetEffect(stack, MenuPosData->effect_tex, 10, base);
    icon_tex = GetMenuItemIconTexInfo(item->item_no, 0);
    base[2] = item->item_no;
    effect[1]->PresetEffect(stack, icon_tex, 0x15, base);
    effect[0]->EffectStart();
    effect[1]->EffectStart();
}

void CMenuEffect::Initialize(void) {
    tex_block = 0;
    tex = NULL;
    type = -1;
    run = 0;
    info_num = 0;
    info = NULL;
    alpha = 128;
}

void CMenuEffect::PresetEffect(mgCMemory *stack, mgCTexture *new_tex, int new_type, int *base) {
    int *menu_tex_block;

    menu_tex_block = MenuCommonInfo->tex_block;
    type = new_type;
    switch (new_type) {
        case 10:
            SetTexInfo(new_tex, menu_tex_block);
            info_num = 0x60;
            SetMemory(stack);
            SetBaseInfo(base, 1, 1, 4);
            break;
        case 0:
            SetTexInfo(new_tex, menu_tex_block);
            info_num = 0x80;
            SetMemory(stack);
            SetBaseInfo(base, 1, 1, 4);
            break;
        case 4:
            SetTexInfo(new_tex, menu_tex_block);
            info_num = 1;
            SetMemory(stack);
            SetBaseInfo(base, 1, 1, 4);
            break;
        case 19:
            SetTexInfo(new_tex, menu_tex_block);
            info_num = 0x70;
            SetMemory(stack);
            SetBaseInfo(base, 1, 1, 7);
            break;
        case 21:
            SetTexInfo(new_tex, menu_tex_block);
            info_num = 1;
            SetMemory(stack);
            SetBaseInfo(base, 1, 1, 4);
            break;
    }
}

void CMenuEffect::SetMemory(mgCMemory *stack) {
    unsigned int size;
    unsigned int blocks;

    size = info_num * sizeof(MENU_EFFECT_INFO);
    if (size & 0xF) {
        blocks = (size >> 4) + 1;
    } else {
        blocks = size >> 4;
    }
    info = (MENU_EFFECT_INFO *)stack->Alloc(blocks);
}

void CMenuEffect::SetTexInfo(mgCTexture *new_tex, int *new_tex_block) {
    tex = new_tex;
    if (new_tex_block != NULL) {
        tex_block = *new_tex_block;
    }
}

void CMenuEffect::SetBaseInfo(int *base, int preset, int mode, int num) {
    int i;

    for (i = 0; i < num; i++) {
        base_info[i] = base[i];
    }
    if (preset != 0) {
        PresetInfoAll(mode);
    }
}

void CMenuEffect::EffectStart(void) {
    run = 1;
    counter = 0;
}

void CMenuEffect::PresetInfoAll(int mode) {
    int i;

    for (i = 0; i < info_num; i++) {
        PresetInfo(&info[i], i, mode);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", PresetInfo__11CMenuEffectFP16MENU_EFFECT_INFOii);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Step__11CMenuEffectFv);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menudraw", Draw__11CMenuEffectFv);

template <class T>
void PrimQuad(mgCDrawPrim *prim, mgRect<T> put_rect, mgRect<int> tex_rect) {
    if (prim != NULL) {
        prim->TextureCrd(tex_rect.left, tex_rect.top);
        prim->Vertex(put_rect.left, put_rect.top, (T)0);
        prim->TextureCrd(tex_rect.left + tex_rect.right, tex_rect.top + tex_rect.bottom);
        prim->Vertex(put_rect.left + put_rect.right, put_rect.top + put_rect.bottom, (T)0);
    }
}

template void PrimQuad(mgCDrawPrim *prim, mgRect<float> put_rect, mgRect<int> tex_rect);

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", spectol_break_pos__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", spectol_break_angle__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", item_transtbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", paint_color_table_1234__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", spectol_y_addtbl_1245__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", MenuWindowHelpTable_1346__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", table_1650__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", tbl_1689__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", get_onoffbrdtbl_1789__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1790__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1791__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1796__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1803__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1814__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1999__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", tbl_2072__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2265__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", star_color_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", frmtbl0_2922__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", frmtbl1_2938__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2949__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2950__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2951__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", rottbl_3145__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", baseposoffset_tbl_4194__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", actposoffsettbl1_4195__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4494__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4495__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", l_levelup_color__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_5441__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_5450__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_5901__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_873__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_975__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1622__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1690__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1691__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1692__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1693__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1694__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1695__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1696__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1697__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1698__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1699__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1700__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1711__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1780__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2209__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2237__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_2238__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_3054__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_3721__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_3927__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4182__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4183__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4184__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4185__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4186__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4453__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4522__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4877__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4888__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4889__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4890__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4933__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4934__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4935__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", rgbatbl_1379__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1788__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", get_btntbl_1810__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_1998__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", MenuWakuPutXY__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", static_rgba_table_3128__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", menu_prim_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_3658__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", basepos_4190__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", farleft_4191__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", xyoffset_4192__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", actpos_4193__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menudraw", at_4442__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(use_trans_rect, 0x4);
INCLUDE_BSS(item_board_counter, 0x4);
INCLUDE_BSS(MenuDrawItemInfoNum, 0x4);
INCLUDE_BSS(use_item_enable_alpha_angle, 0x4);
INCLUDE_BSS(use_item_enable_alpha, 0x4);
INCLUDE_BSS(spectol_raster_xtbl, 0x4);
INCLUDE_BSS(DrawItemCounter, 0x4);
INCLUDE_BSS(DrawItemDefCounter, 0x4);
INCLUDE_BSS(NowGiftBoxPtr, 0x4);
INCLUDE_BSS(GiftBoxViewForm, 0x4);
INCLUDE_BSS(NowGiftBoxSelect, 0x4);
INCLUDE_BSS(GiftBoxViewFlag, 0x4);
INCLUDE_BSS(curpos_1393, 0x4);
INCLUDE_BSS(init_1394, 0x4);
INCLUDE_BSS(at_1400__2, 0x8);
INCLUDE_BSS(MenuDrawNumberKeta, 0x8);
INCLUDE_BSS(at_1521__2, 0x8);
INCLUDE_BSS(menu_randam_line_draw_postbl, 0x4);
INCLUDE_BSS(Tex_MenuDl, 0x4);
INCLUDE_BSS(MenuDl_TotalSize, 0x4);
INCLUDE_BSS(MenuDl_ProcessSize, 0x4);
INCLUDE_BSS(Tex_CommonBoard, 0x4);
INCLUDE_BSS(make_object_husoku_number_blink, 0x4);
INCLUDE_BSS(MenuCursorReverseFlag, 0x4);
INCLUDE_BSS(menu_cursor_rotation_angle, 0x4);
INCLUDE_BSS(MenuMainFrame_ActionEndFlag, 0x4);
INCLUDE_BSS(MenuMainFrame_Display_Mode, 0x4);
INCLUDE_BSS(MenuMainFrame_Display_Mode_Cnt, 0x4);
INCLUDE_BSS(MenuMainFrame_Display_Mode_Cnt_Rate, 0x4);
INCLUDE_BSS(MenuMainFrame_Lenze_Pos, 0x8);
INCLUDE_BSS(MenuMainFrame_MoveRate, 0x8);
INCLUDE_BSS(MenuMainFrame_MoveRate_Cnt, 0x8);
INCLUDE_BSS(MenuMainFrame_LeftTop_Pos, 0x8);
INCLUDE_BSS(MainFrameStepFlag_2092, 0x4);
INCLUDE_BSS(init_2093, 0x4);
INCLUDE_BSS(MenuWakuRotCnt, 0x8);
INCLUDE_BSS(at_2596__2, 0x8);
INCLUDE_BSS(MenuItemBrdCalcManner, 0x4);
INCLUDE_BSS(MenuItemBrdMaxLine, 0x4);
INCLUDE_BSS(MenuItemBrdViewLine, 0x4);
INCLUDE_BSS(MenuItemBrdScrlCurLen, 0x4);
INCLUDE_BSS(MenuItemBrdUnderBrdPosY_Next, 0x8);
INCLUDE_BSS(MenuItemBrdUnderBrdPosXY, 0x8);
INCLUDE_BSS(MenuItemBrdScrlBarY, 0x4);
INCLUDE_BSS(localrgba_3166, 0x4);
INCLUDE_BSS(at_3325, 0x8);
INCLUDE_BSS(at_3428, 0x8);
INCLUDE_BSS(at_3527, 0x8);
INCLUDE_BSS(at_3531, 0x8);
INCLUDE_BSS(at_3612, 0x8);
INCLUDE_BSS(at_3651, 0x8);
INCLUDE_BSS(MenuPosData, 0x8);
INCLUDE_BSS(at_4205, 0x8);
INCLUDE_BSS(at_4526, 0x8);
INCLUDE_BSS(MenuFrameTex, 0x4);
INCLUDE_BSS(at_4727, 0x4);
INCLUDE_BSS(at_4728, 0x4);
INCLUDE_BSS(at_4729, 0x4);
INCLUDE_BSS(at_4730, 0x4);
INCLUDE_BSS(MenuVerticalLineTex, 0x4);
INCLUDE_BSS(MenuVerticalLine, 0x4);
INCLUDE_BSS(MenuVerticalLineNum, 0x4);
INCLUDE_BSS(MenuVerticalLineUpLimmit, 0x4);
INCLUDE_BSS(MenuVerticalLineChara, 0x4);
INCLUDE_BSS(MenuVerticalRange, 0x4);
INCLUDE_BSS(fish_boiled_count, 0x4);
INCLUDE_BSS(fish_boiled_runflag, 0x4);
INCLUDE_BSS(fish_boiled_effect_tex, 0x4);
INCLUDE_BSS(at_5917, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(menu_limmit_displayflag, 0xA0);
INCLUDE_BSS(MenuMesForm, 0x30);
INCLUDE_BSS(at_900__4, 0x10);
INCLUDE_BSS(MenuDrawItemInfo, 0x260);
INCLUDE_BSS(GiftBoxWindowPutPos, 0x10);
INCLUDE_BSS(Pos_ItemInGiftBox, 0x10);
INCLUDE_BSS(MakeBoardDrawInfo, 0x20);
INCLUDE_BSS(CommonBoardDrawInfo, 0x30);
INCLUDE_BSS(at_1720, 0x10);
INCLUDE_BSS(menu_long_hand, 0x10);
INCLUDE_BSS(MenuMainFrame_PutRect, 0x10);
INCLUDE_BSS(MenuMainIMG_PutRect, 0x10);
INCLUDE_BSS(at_2292, 0x10);
INCLUDE_BSS(at_2303, 0x10);
INCLUDE_BSS(at_2395__4, 0x20);
INCLUDE_BSS(star_light, 0x10);
INCLUDE_BSS(MenuItemBrdKomaRect, 0x10);
INCLUDE_BSS(ItemBoardScrlBar1, 0x10);
INCLUDE_BSS(ItemBoardScrlBar2, 0x10);
INCLUDE_BSS(ItemBoardScrlBar3, 0x10);
INCLUDE_BSS(ItemBoardCursor, 0x10);
INCLUDE_BSS(at_2919, 0x30);
INCLUDE_BSS(at_3384, 0x20);
INCLUDE_BSS(putpostbl_3410, 0x20);
INCLUDE_BSS(getpostbl_3411, 0x20);
INCLUDE_BSS(temp_3925, 0x20);
INCLUDE_BSS(at_4496, 0x20);
INCLUDE_BSS(l_levelup_pos, 0x180);
INCLUDE_BSS(l_levelup_vec, 0x180);
INCLUDE_BSS(l_levelup_counter, 0x20);
INCLUDE_BSS(l_levelup_generate_counter, 0x20);
INCLUDE_BSS(MenuVerticalLineCharaPos, 0x10);
INCLUDE_BSS(MenuVerticalLineCharaPos2, 0x10);
INCLUDE_BSS(fish_boiled_positin, 0x40);
INCLUDE_BSS(fish_boiled_amp_count, 0x20);
INCLUDE_BSS(fish_boiled_streatch_rate, 0x20);
INCLUDE_BSS(fish_boiled_alpha, 0x20);
