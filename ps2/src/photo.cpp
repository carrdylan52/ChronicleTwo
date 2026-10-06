#include "common.h"
#include "photo.hpp"
#include "mglib.hpp"
#include "menucommon.hpp"
#include "padcontrol.hpp"
#include "font.hpp"
#include "userdata.hpp"
#include "mg_texture.hpp"
#include "mainloop.hpp"
#include <cstring>
#include <cstdio>

static char *mes_txt[LANG_SPANISH + 1][PHOTO_MES_NUM] = {
    {
        "(A):\216\312\220^\202\360\212m\224F",
        "(#):\216\312\220^\202\360\202\306\202\351",
        "\203\206\203\212\203X\202\314\216\312\220^\211\306\203"
        "\214\203x\203\213\202\252\217\343\202\252\202\301\202\275",
        "(R):\203Y\201[\203\200"
    },
    {
        "(A):Confirm Picture",
        "(#):Take Picture",
        "Max's photography level increased!",
        "(R):zoom (X):Back"
    },
    {
        "(A) : confirmer photo",
        "(#) : prendre photo",
        "Niveau de photographie de Max a augment[UNI00e9] !",
        "(R) : zoom (X) : retour"
    },
    {
        "(A):Foto best[UNI00e4]tigen",
        "(#):Fotografieren",
        "Max' Fotografen-Level ist gestiegen!",
        "(R):Zoom (X):Zur[UNI00fc]ck"
    },
    {
        "(A):Conferma Foto",
        "(#):Scatta foto",
        "Il livello foto di Max [UNI00e8] aumentato!",
        "(R):zoom (X):Indietro"
    },
    {
        "(A):Confirmar foto",
        "(#):Hacer foto",
        "[UNI00a1]Ha aumentado el nivel de fotograf[UNI00ed]a de Max!",
        "(R):zoom (X):Volver"
    },
};
static char *null_txt = "";

static int TakePhotoMode;
static float AddProj;
static int CameraTexb;
static mgCTexture *WorkTex;
static int ShutterAnmCnt;
static int ShowTakePhotoCnt;
static int OpenMenu;
static int ShowTitleCnt;
static int ShowLevelUpCnt;
static CFont Font;
static char PhotoTitle[128];

// Code (.text)
/**
 * Gives a camera-mode message in the current language, or an empty
 * string when the message or language is out of range.
 *
 * @mangled GetMesTxt__Fi
 * @address 0x313880
 * @size 0x60
 */
static char *GetMesTxt(int message) {
    if (message < 0 || message >= PHOTO_MES_NUM) {
        return null_txt;
    }
    if (LanguageCode < 0 || LanguageCode >= LANG_SPANISH + 1) {
        return null_txt;
    }
    return mes_txt[LanguageCode][message];
}

float PhotoAddProjection() {
    if (NowTakePhoto()) {
        return AddProj;
    }
    return 0.0f;
}

/**
 * Clears the displayed photo title.
 *
 * @mangled InitPhotoTitle__Fv
 * @address 0x313910
 * @size 0x10
 */
static void InitPhotoTitle() {
    ShowTitleCnt = 0;
    PhotoTitle[0] = 0;
}

void InitTakePhoto() {
    TakePhotoMode = TAKE_PHOTO_OFF;
    AddProj = 0.0f;
    InitPhotoTitle();
    ShowTakePhotoCnt = 0;
    CameraTexb = -1;
    ShutterAnmCnt = 0;
    OpenMenu = 0;
    ShowLevelUpCnt = 0;
}

void LoadTakePhoto(int camera_texb, mgCMemory *memory, u_long128 *buffer) {
    WorkTex = mgTexManager.EnterTexture(MG_TEXTURE_BLOCK_FIX, "fix_work", NULL,
                                       64, 64, 16, NULL, 0, 0);
    CameraTexb = camera_texb;
    Font.Init();
    Font.Preset(FONT_PRESET_SHADOWED);
    Font.SetFuchi(FUCHI_OUTLINE);
    Font.SetClearance(0xF, 0x18);
}

void StartTakePhoto() {
    InitTakePhoto();
    TakePhotoMode = TAKE_PHOTO_AIM;
}

void EndTakePhoto() {
    InitTakePhoto();
}

int NowTakePhoto() {
    return TakePhotoMode > TAKE_PHOTO_OFF;
}

int IsEnablePhotoMenu() {
    return TakePhotoMode == TAKE_PHOTO_AIM;
}

void HidePhoto() {
    ShowTakePhotoCnt = 0;
}

int GhostPhotoTiming() {
    switch (TakePhotoMode) {
        case TAKE_PHOTO_SHUTTER:
        case TAKE_PHOTO_STORE:
            return 1;
        default:
            return 0;
    }
}

void LoopTakePhoto(CPadControl *pad, CInventUserData *user_data) {
    if (user_data != NULL) {
        if (TakePhotoMode == TAKE_PHOTO_AIM) {
            AddProj += 10.0f * -pad->Analog(3);
            if (AddProj > 200.0f) {
                AddProj = 200.0f;
            }
            if (AddProj < -200.0f) {
                AddProj = -200.0f;
            }
            if (user_data->IsPhotoSpace(NULL) != 0 && pad->Btn(0x33) != 0) {
                TakePhotoMode = TAKE_PHOTO_SHUTTER;
            }
            --ShowTakePhotoCnt;
            if (ShowTakePhotoCnt < 0) {
                ShowTakePhotoCnt = 0;
            }
        }
        if (TakePhotoMode == TAKE_PHOTO_AFTERSHOT) {
            --ShutterAnmCnt;
            if (ShutterAnmCnt < 0) {
                ShutterAnmCnt = 0;
                TakePhotoMode = TAKE_PHOTO_AIM;
            }
        }
        if (TakePhotoMode == TAKE_PHOTO_OPEN_MENU) {
            OpenMenu = 1;
            TakePhotoMode = TAKE_PHOTO_AIM;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/photo", DrawTakePhoto__FP17USER_PICTURE_INFOPf);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/photo", SetTookPhotoData__FP17USER_PICTURE_INFO);

void DrawTakePhotoSystem(int texb, CInventUserData *user_data) {
    char title[256];
    char count_text[40];
    int counts[2];
    int y;
    int char_width;
    mgTexManager.ReloadTexture(texb, (sceVif1Packet *)NULL);
    Font.SetColor(0xFF, 0xFF, 0xFF, 0x80);
    if (ShowTitleCnt > 0 && PhotoTitle[0] != 0) {
        y = mgScreenHeight - 0x24;
        Font.SetStr(PhotoTitle);
        Font.SetPos(0x14, y);
        Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
        --ShowTitleCnt;
        if (ShowTitleCnt <= 0) {
            InitPhotoTitle();
        }
    } else {
        y = mgScreenHeight - 0x29;
        Font.SetStr(GetMesTxt(PHOTO_MES_CONFIRM));
        Font.SetPos(0x28, y);
        Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
        y = mgScreenHeight - 0x29;
        Font.SetStr(GetMesTxt(PHOTO_MES_ZOOM));
        Font.SetPos(0xF0, y);
        Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
        y = mgScreenHeight - 0x15;
        Font.SetStr(GetMesTxt(PHOTO_MES_TAKE));
        Font.SetPos(0x28, y);
        Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
    }
    ConvertFontCode(GetMesTxt(PHOTO_MES_LEVEL_UP), title);
    char_width = Font.draw_w;
    y = 0xF6;
    y -= (int)((char_width * strlen(title)) >> 1) / 2;
    if (ShowLevelUpCnt > 0) {
        Font.SetStr(GetMesTxt(PHOTO_MES_LEVEL_UP));
        Font.SetPos(y, 0x140);
        Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
        --ShowLevelUpCnt;
    }
    user_data->GetPictureNum(counts);
    sprintf(count_text, "%d/%d", counts[0], counts[1]);
    if (counts[0] >= counts[1]) {
        Font.SetColor(0xFF, 0x20, 0x10, 0x80);
    }
    y = mgScreenHeight - 0x2E;
    Font.SetStr(count_text);
    Font.SetPos(0x1B8, y);
    Font.DrawDirect(Font.str, Font.pos_x, Font.pos_y);
}

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", at_936__6__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", at_817__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", at_852__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", at_997__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", at_1055__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/photo", null_txt__DATA);

// Small uninitialised data (.sbss)

// Uninitialised data (.bss)
