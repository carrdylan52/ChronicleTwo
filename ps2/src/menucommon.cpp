#include "common.h"
#include "menucommon.hpp"
#include "character.hpp"
#include "dataread.hpp"
#include "gamedata.hpp"
#include "mainloop.hpp"
#include "menuaqua.hpp"
#include "menucls1.hpp"
#include "menudraw.hpp"
#include "menumain.hpp"
#include "menusys.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "nd_meswin.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "sysmes.hpp"
#include "userdata.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * Number of item kinds in the sorting order.
 */
static const int sort_type_count = 0x24;
/**
 * Default number of entries allocated for a menu information table.
 */
static const int default_etc_count = 0x60;
/**
 * Current sorting rank of each item kind.
 */
static s8 sort_table[sort_type_count] = {
    36, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 0,
};
/**
 * First item kind in the next sorting order.
 */
static s16 sort_top_type = 1;
/**
 * First texture entry selected by the script.
 */
static u16 MenuTexPosNo;
/**
 * Texture entry offset within the current script definition.
 */
static u16 MenuTexPosNo_local;
/**
 * Texture name copied by subsequent texture-rectangle definitions.
 */
static char MenuSpiTextureName[32];
/**
 * Form currently being defined by the script.
 */
static CMenuPosDataForm *menu_formPt;
/**
 * Next named action entry to fill.
 */
static MENU_FORM_ACTION *menu_spi_form_action_info;
/**
 * Next part effect entry to fill.
 */
static MENU_PARTS_EFFECT_STRUCT1 *menu_parts_effect_ptr;
/**
 * Part currently being defined by the script.
 */
static MENUFORMPARTS_TYPE *menu_form_part;
/**
 * Index of the next part in the selected form.
 */
static int menu_form_partsno;
/**
 * Non-zero while the script executes the requested command.
 */
static u8 SpiMenuExeCommandFlag;
/**
 * Texture block selected for subsequent texture definitions.
 */
static s16 menu_analyze_texblock;
/**
 * Base form number selected by the script.
 */
static s16 menu_analyze_formno;
/**
 * Relative index of the next form to define.
 */
static s16 menu_analyze_formno_offset;
/**
 * First information entry selected by the script.
 */
static s16 Menu_Target_No;
/**
 * Information entry offset within the current script definition.
 */
static s16 Menu_Target_No_local;
/**
 * Saved object-sound port volume.
 */
static float SndPortVol_Ob;
/**
 * Saved base-map sound port volume.
 */
static float SndPortVol_Base;
/**
 * Saved event-sound port volume.
 */
static float SndPortVol_Event;
/**
 * Saved environmental background-music volume.
 */
static float SndPortVol_Env;
/**
 * Non-zero when the event-sound port was muted.
 */
static int SndPortCheck_EventPort;

static int CompGameData(int item_a, int item_b);
static int SeitonItemBoardSub(CGameDataUsed *items, int count);
static int _ETCINFO_MALLOC(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO_OFFSET(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO_CLEAR(SPI_STACK *stack, int arg_count);
static int _ETCINFO2_MALLOC(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO2_OFFSET(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO2(SPI_STACK *stack, int arg_count);
static int _MENU_ETCINFO2_CLEAR(SPI_STACK *stack, int arg_count);
static int _MENU_RESET_TEXINFO(SPI_STACK *stack, int arg_count);
static int _MENU_INIT_DRAWLIST(SPI_STACK *stack, int arg_count);
static int _MENU_TEXDATA_CLEAR(SPI_STACK *stack, int argc);
static int _MENU_FORM_CLEAR(SPI_STACK *stack, int argc);
static int _MENU_TEXDATA_MALLOC(SPI_STACK *stack, int argc);
static int _MENU_TEXNAME(SPI_STACK *stack, int argc);
static int _MENU_TEXDATA_OFFSET(SPI_STACK *stack, int argc);
static int _MENU_TEXDATA(SPI_STACK *stack, int argc);
static int _MENU_FORM_MALLOC(SPI_STACK *stack, int argc);
static int _MENU_FORM_OFFSET_NO(SPI_STACK *stack, int argc);
static int _MENU_FORM_SET(SPI_STACK *stack, int argc);
static int _MENU_FORM_PARTNUM(SPI_STACK *stack, int argc);
static void menu_texdata_to_formpart_copy(MENUFORMPARTS_TYPE *part);
static int _MENU_FORM_DTYPE(SPI_STACK *stack, int argc);
static int _MENU_FORM_MTYPE(SPI_STACK *stack, int argc);
static int _MENU_FORM_DRAWFLG(SPI_STACK *stack, int argc);
static int _MENU_FORM_VIBECNT(SPI_STACK *stack, int argc);
static int _MENU_FORM_SETEND(SPI_STACK *stack, int argc);
static int _MENU_FORM_MOVERATE(SPI_STACK *stack, int argc);
static int _MENU_FORM_PUTXY(SPI_STACK *stack, int argc);
static int _MENU_FORM_RGBA(SPI_STACK *stack, int argc);
static int _MENU_ACTION_TABLE_NUM(SPI_STACK *stack, int argc);
static int _MENU_ACTION_DEF(SPI_STACK *stack, int argc);
static int _MENU_ACTION_SETACTION(SPI_STACK *stack, int argc);
static int _MENU_PARTVIBECNT(SPI_STACK *stack, int argc);
static int _MENU_PARTVIBER(SPI_STACK *stack, int argc);
static int _MENU_SHADOW_ONOFF(SPI_STACK *stack, int argc);
static int _CLIP_WH(SPI_STACK *stack, int argc);
static int _MENU_PARTRGBA(SPI_STACK *stack, int argc);
static int _MENU_PART_ALPHA_BLEND(SPI_STACK *stack, int argc);
static int _MENU_PART_ETCINFO(SPI_STACK *stack, int argc);
static int _MENU_PART_BILINEAR(SPI_STACK *stack, int argc);
static void MakePartsName(SPI_STACK *stack, MENUFORMPARTS_TYPE *part);
static int _MENU_PART_DTYPE(SPI_STACK *stack, int argc);
static int _MENU_NORMAL(SPI_STACK *stack, int argc);
static int _MENU_NORMAL2(SPI_STACK *stack, int argc);
static int _MENU_CURSOR(SPI_STACK *stack, int argc);
static int _MENU_FUNCINFO(SPI_STACK *stack, int argc);
static int _MENU_NUMBER1(SPI_STACK *stack, int argc);
static int _MENU_NUMBER2(SPI_STACK *stack, int argc);
static int _MENU_FRMIMG(SPI_STACK *stack, int argc);
static int _MENU_FORM(SPI_STACK *stack, int argc);
static int _MENU_ITEM(SPI_STACK *stack, int argc);
static int _MENU_ITEM_CHECKMARK(SPI_STACK *stack, int argc);
static int _MENU_FILLBOXINFO(SPI_STACK *stack, int argc);
static int _MENU_WAKU_RECT(SPI_STACK *stack, int argc);
static int _MENU_WAKU_CIRCLE(SPI_STACK *stack, int argc);
static int _MENU_PARTS_EFF_NUM(SPI_STACK *stack, int argc);
static int _MENU_PARTS_EFFECT(SPI_STACK *stack, int argc);
static int _MENU_EXE_COMMAND_NAME(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_DRAWFLAG(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_RGBA(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_CALCRGBAPARAM(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_FADE(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_SETPOS(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_SETACTION(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_PARTSONOFF(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_PARTSONOFF_GRP(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_SWAP(SPI_STACK *stack, int argc);
static int _MENU_EXE_FORM_GROUP_SWAP(SPI_STACK *stack, int argc);
static int _MENU_EXE_MSGENV(SPI_STACK *stack, int argc);
static int _MENU_EXE_MAKEMSG(SPI_STACK *stack, int argc);
static int _MENU_EXE_SETABSPOS(SPI_STACK *stack, int argc);
static int _MENU_EXE_MSGSETSYSTEMBUFF(SPI_STACK *stack, int argc);
static int _MENU_EXE_MSGSETBUFF(SPI_STACK *stack, int argc);
static int _MENU_EXE_MSGSETFUCHI(SPI_STACK *stack, int argc);
static int _MENU_EXE_MSGSETCURSOR(SPI_STACK *stack, int argc);
static int _MENU_SET_QUESTIONGYOU(SPI_STACK *stack, int argc);
static int _MENU_SET_OPENSPEED(SPI_STACK *stack, int argc);
static int _MENU_INPUT_KEY(SPI_STACK *stack, int argc);
static int _MENU_CURSOR_ONOFF(SPI_STACK *stack, int argc);
static int _MENU_CURSOR_FADE(SPI_STACK *stack, int argc);
static int _MENU_WAKUTYPE(SPI_STACK *stack, int argc);
static int _MENU_SCENE_FADE(SPI_STACK *stack, int argc);
static int _MENU_SE_PLAY(SPI_STACK *stack, int argc);
static int _MENU_EXE_INIT_DRAWLIST(SPI_STACK *stack, int argc);
static int _MENU_EXE_RESET_TEXINFO(SPI_STACK *stack, int argc);
static int _MENU_DEBUG_PRINTF(SPI_STACK *stack, int argc);
static int menu_dtype_init(CMenuPosDataForm *form, SPI_STACK *stack, int argc);
static int _MENU_FORM_RGBA_BIT(SPI_STACK *stack, int argc);
static int _MENU_FILLBOX(SPI_STACK *stack, int argc);

/**
 * Menu layout tags and their handlers.
 */
static SPI_TAG_PARAM menu_analyze_tag[] = {
    {"ETCINFO_MALLOC", _ETCINFO_MALLOC},
    {"ETCINFO_OFFSET", _MENU_ETCINFO_OFFSET},
    {"ETCINFO", _MENU_ETCINFO},
    {"ETCINFO_CLEAR", _MENU_ETCINFO_CLEAR},
    {"ETCINFO2_MALLOC", _ETCINFO2_MALLOC},
    {"ETCINFO2_OFFSET", _MENU_ETCINFO2_OFFSET},
    {"ETCINFO2", _MENU_ETCINFO2},
    {"ETCINFO2_CLEAR", _MENU_ETCINFO2_CLEAR},
    {"RESET_TEXINFO", _MENU_RESET_TEXINFO},
    {"INIT_DRAWLIST", _MENU_INIT_DRAWLIST},
    {"TEXDATA_CLEAR", _MENU_TEXDATA_CLEAR},
    {"TEXDATA_MALLOC", _MENU_TEXDATA_MALLOC},
    {"TEXNAME", _MENU_TEXNAME},
    {"TEXDATA_OFFSET", _MENU_TEXDATA_OFFSET},
    {"TEXDATA", _MENU_TEXDATA},
    {"TD", _MENU_TEXDATA},
    {"FORM_CLEAR", _MENU_FORM_CLEAR},
    {"FORM_MALLOC", _MENU_FORM_MALLOC},
    {"FORM_OFFSET_NO", _MENU_FORM_OFFSET_NO},
    {"FORM_SET", _MENU_FORM_SET},
    {"FORM_PARTNUM", _MENU_FORM_PARTNUM},
    {"FORM_DTYPE", _MENU_FORM_DTYPE},
    {"FORM_MTYPE", _MENU_FORM_MTYPE},
    {"FORM_DRAWFLG", _MENU_FORM_DRAWFLG},
    {"FORM_VIBECNT", _MENU_FORM_VIBECNT},
    {"FORM_MOVERATE", _MENU_FORM_MOVERATE},
    {"FORM_PUTXY", _MENU_FORM_PUTXY},
    {"FORM_RGBA", _MENU_FORM_RGBA},
    {"FORM_RGBABIT", _MENU_FORM_RGBA_BIT},
    {"FORM_ACTTBL", _MENU_ACTION_TABLE_NUM},
    {"FORM_ACTDEF", _MENU_ACTION_DEF},
    {"FORM_SETACT", _MENU_ACTION_SETACTION},
    {"PARTVIBECNT", _MENU_PARTVIBECNT},
    {"PARTVIBER", _MENU_PARTVIBER},
    {"SHADOW", _MENU_SHADOW_ONOFF},
    {"CLIP", _CLIP_WH},
    {"PARTRGBA", _MENU_PARTRGBA},
    {"PARTALP_BLEND", _MENU_PART_ALPHA_BLEND},
    {"PART_ETCINFO", _MENU_PART_ETCINFO},
    {"PART_BILINEAR", _MENU_PART_BILINEAR},
    {"PART_DTYPE", _MENU_PART_DTYPE},
    {"NORMAL", _MENU_NORMAL},
    {"NRL", _MENU_NORMAL},
    {"NORMAL2", _MENU_NORMAL2},
    {"CURSOR", _MENU_CURSOR},
    {"FUNCINFO", _MENU_FUNCINFO},
    {"NUMBER1", _MENU_NUMBER1},
    {"NUMBER2", _MENU_NUMBER2},
    {"FRMIMG", _MENU_FRMIMG},
    {"FORM", _MENU_FORM},
    {"FORM_SETEND", _MENU_FORM_SETEND},
    {"ITEM", _MENU_ITEM},
    {"ITEM_MARK", _MENU_ITEM_CHECKMARK},
    {"FILLBOX", _MENU_FILLBOX},
    {"FILLBOXINFO", _MENU_FILLBOXINFO},
    {"WAKU_RECT", _MENU_WAKU_RECT},
    {"WAKU_CIRCLE", _MENU_WAKU_CIRCLE},
    {"PARTS_EFF_NUM", _MENU_PARTS_EFF_NUM},
    {"PARTS_EFFECT", _MENU_PARTS_EFFECT},
    {NULL, NULL},
};

/**
 * Menu execution tags and their handlers.
 */
static SPI_TAG_PARAM menu_execommand_analyze_tag[] = {
    {"EXE_NAME", _MENU_EXE_COMMAND_NAME},
    {"FRM_DRAWFLG", _MENU_EXE_FORM_DRAWFLAG},
    {"FRM_RGBA", _MENU_EXE_FORM_RGBA},
    {"FRM_CALCRGBA", _MENU_EXE_FORM_CALCRGBAPARAM},
    {"FRM_FADE", _MENU_EXE_FORM_FADE},
    {"FRM_SETPOS", _MENU_EXE_FORM_SETPOS},
    {"FRM_SETACTION", _MENU_EXE_FORM_SETACTION},
    {"FRM_PARTSONOFF", _MENU_EXE_FORM_PARTSONOFF},
    {"FRM_PARTSONOFFG", _MENU_EXE_FORM_PARTSONOFF_GRP},
    {"FRM_PTSONOFF", _MENU_EXE_FORM_PARTSONOFF},
    {"FRM_PTSONOFFG", _MENU_EXE_FORM_PARTSONOFF_GRP},
    {"FRM_SWAP", _MENU_EXE_FORM_SWAP},
    {"FRM_SWAP_G", _MENU_EXE_FORM_GROUP_SWAP},
    {"INIT_DRAWLIST", _MENU_EXE_INIT_DRAWLIST},
    {"RESET_TEXINFO", _MENU_EXE_RESET_TEXINFO},
    {"MSG_ENV", _MENU_EXE_MSGENV},
    {"MSG_MAKEMSG", _MENU_EXE_MAKEMSG},
    {"MSG_ABSPOS", _MENU_EXE_SETABSPOS},
    {"MSG_SYSBUF", _MENU_EXE_MSGSETSYSTEMBUFF},
    {"MSG_BUF", _MENU_EXE_MSGSETBUFF},
    {"MSG_FUCHI", _MENU_EXE_MSGSETFUCHI},
    {"MSG_SETCUR", _MENU_EXE_MSGSETCURSOR},
    {"MSG_SETGYOU", _MENU_SET_QUESTIONGYOU},
    {"MSG_SPD", _MENU_SET_OPENSPEED},
    {"IN_KEY", _MENU_INPUT_KEY},
    {"CURSOR_ONOFF", _MENU_CURSOR_ONOFF},
    {"CURSOR_FADE", _MENU_CURSOR_FADE},
    {"SET_WAKU", _MENU_WAKUTYPE},
    {"SCN_FADE", _MENU_SCENE_FADE},
    {"SE_ON", _MENU_SE_PLAY},
    {"PRINT", _MENU_DEBUG_PRINTF},
    {NULL, NULL},
};

/**
 * Gives the number of 16-byte blocks needed to hold a byte count.
 */
static inline unsigned int align16_blocks(unsigned int bytes) {
    if (bytes & 0xF) {
        return (bytes >> 4) + 1;
    }
    return bytes >> 4;
}

// Trap on division by zero for variable integer divisors.
#pragma divbyzerocheck on

// Code (.text)
int GetRandI(int range) {
    return rand() % range;
}

float GetRandF(float range) {
    return range * mgRnd();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", ReCalcBox__FP9mgVu0FBOX9mgVu0FBOX);
float MenuAdjustPolygonScale(mgCFrame *frame, float size) {
    mgVu0FBOX box;
    if (frame == NULL) {
        return 1.0f;
    }
    frame->GetWorldBBox(&box);
    return MenuAdjustPolygonScale(box, size);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", MenuAdjustPolygonScale__F9mgVu0FBOXf);
void MenuAdjustPolygonScale(CCharacter2 *chara, float size) {
    if (chara != NULL) {
        float magnitude;
        float height = chara->body_height;
        float scale = 1.0f;
        magnitude = height;
        if (height < 0.0f) {
            magnitude = -height;
        }
        if (magnitude > 1.0f) {
            scale = size / height;
        }
        chara->SetScale(scale, scale, scale);
    }
}

void AddRotationCharaY(CCharacter2 *chara, float angle) {
    sceVu0FVECTOR rotation;
    if (chara != NULL) {
        chara->GetRotation(rotation);
        float *rotation_y = &rotation[1];
        *rotation_y += angle;
        *rotation_y = mgAngleLimit(*rotation_y);
        chara->SetRotation(rotation);
    }
}

void MenuSePlay(int sound_no) {
    if (sound_no >= 0) {
        MenuSePlay(SystemSND_ID, sound_no);
    }
}

void MenuSePlay(unsigned int handle, int sound_no) {
    if (sound_no >= 0) {
        sndSePlay(handle, sound_no, 0);
    }
}

void MenuSePlay(int sound_no, unsigned int *bank, mgCMemory *memory) {
    if (bank == NULL || memory == NULL) {
        return;
    }
    memory->stReset();
    sndInitPort(SND_PORT_MENU);
    sndSePlay(sndLoadSound(SND_PORT_MENU, bank, memory), sound_no, 0);
    MenuSePlayUsedFlag = 1;
}

void StopEnvSoundMenu(int event_port) {
    SndPortVol_Ob = sndGetPortVol(SND_PORT_OB);
    SndPortVol_Base = sndGetPortVol(SND_PORT_BASE);
    sndSetPortVol(SND_PORT_OB, 0.0f);
    sndSetPortVol(SND_PORT_BASE, 0.0f);
    SndPortCheck_EventPort = event_port;
    if (event_port != 0) {
        SndPortVol_Event = sndGetPortVol(SND_PORT_EVENT);
        sndSetPortVol(SND_PORT_EVENT, 0.0f);
    }
    SndPortCheck_EventPort = event_port;
    SndPortVol_Env = GetMainScene()->GetEnvBGMVol();
    GetMainScene()->SetEnvBGMVol(0.0f);
}

void ReStartEnvSoundMenu() {
    sndSetPortVol(SND_PORT_OB, SndPortVol_Ob);
    sndSetPortVol(SND_PORT_BASE, SndPortVol_Base);
    if (SndPortCheck_EventPort != 0) {
        sndSetPortVol(SND_PORT_EVENT, SndPortVol_Event);
    }
    GetMainScene()->SetEnvBGMVol(SndPortVol_Env);
}

/**
 * Compares two items by their current type order, then by item number.
 */
static int CompGameData(int item_a, int item_b) {
    CGameData *game_data;
    CDataCommon *record_a;
    CDataCommon *record_b;
    int rank_a;
    int rank_b;

    game_data = GetGameDataPt();
    record_a = game_data->GetCommonData(item_a);
    record_b = game_data->GetCommonData(item_b);
    rank_b = 0;
    rank_a = 0;
    if (record_a != NULL) {
        rank_a = sort_table[record_a->type];
    }
    if (record_b != NULL) {
        rank_b = sort_table[record_b->type];
    }
    if (item_a <= 0) {
        rank_a = sort_type_count;
    }
    if (item_b <= 0) {
        rank_b = sort_type_count;
    }
    if (rank_b < rank_a) {
        return 1;
    }
    if (rank_a < rank_b) {
        return -1;
    }
    if (item_b < item_a) {
        return 1;
    }
    if (item_a < item_b) {
        return -1;
    }
    return 0;
}

/**
 * Sorts item slots in the current type order and reports whether any were exchanged.
 */
static int SeitonItemBoardSub(CGameDataUsed *items, int count) {
    CGameDataUsed *board;
    int i;
    int sort_type;
    int j;
    u8 swapped;

    board = items;
    sort_type = sort_top_type;
    for (i = 0; i < sort_type_count; i++) {
        sort_table[sort_type] = i;
        sort_type++;
        if (sort_type >= sort_type_count) {
            sort_type = 0;
        }
    }
    sort_table[0] = sort_type_count;
    swapped = 0;
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (CompGameData(board[i].item_no, board[j].item_no) > 0) {
                GameDataSwap(&board[i], &board[j], 1);
                swapped = 1;
            }
        }
    }
    return swapped;
}

int MenuSeiton(CGameDataUsed *items, int count) {
    CGameDataUsed *board = items;
    int i;
    int j;
    int attempt;
    CGameDataUsed *second;
    int room;
    int moved;
    CGameDataUsed *first;

    if (board == NULL) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        first = &board[i];
        if (first->CheckTypeEnableStack() != 0) {
            for (j = i + 1; j < count; j++) {
                second = &board[j];
                if (first->item_no == second->item_no) {
                    room = first->CheckStackRemain();
                    if (room <= 0) {
                        break;
                    }
                    moved = second->GetNum();
                    if (room < moved) {
                        moved = room;
                    }
                    first->AddNum(moved, 1);
                    second->DeleteNum(moved);
                }
            }
        }
    }
    for (attempt = 0; attempt < sort_type_count; attempt++) {
        if (SeitonItemBoardSub(board, count) != 0) {
            break;
        }
        sort_top_type += 1;
        if (sort_top_type >= sort_type_count) {
            sort_top_type = 1;
        }
    }
    return 1;
}

int GetSameAdrressUserData(CGameDataUsed *item, int kind) {
    CGameDataUsed *entry;
    int bag_max;
    int i;

    if (kind == 0) {
        entry = MenuUserParam.used_data;
        bag_max = GetNowBagMax(1);
        for (i = 0; i < bag_max; i++, entry++) {
            if (entry == item) {
                return i;
            }
        }
    }
    return -1;
}

void local_sort1(int &cursor, int *count, int *list) {
    int i;

    for (i = cursor; i < *count; i++) {
        list[i] = list[i + 1];
    }
    *count -= 1;
    if (cursor > 0) {
        cursor -= 1;
    }
}

int GetNowChapter(CSaveData *save) {
    int progress;

    if (save == NULL) {
        return -1;
    }
    progress = save->game_progress;
    if (progress < 2) {
        return 0;
    }
    if (progress == 2 || progress == 3) {
        return 1;
    }
    if (progress >= 4) {
        return progress - 2;
    }

    if (progress >= 100) {
        return 1;
    }
    return 1;
}

u_long128 *MenuCalcBufAlignment(u_long128 *buffer) {
    int aligned;
    int blocks;
    int size = (int)buffer;

    aligned = size;
    if ((size % 64) != 0) {
        blocks = size >> 6;
        if (size < 0) {
            blocks = (size + 0x3F) >> 6;
        }
        aligned = (blocks + 1) << 6;
    }
    return (u_long128 *)aligned;
}

int LoadFileMenu(char *name, u_long128 *buffer, int mode) {
    static char *language_dirs[] = {
        "0/", "1/", "2/", "3/", "4/", "5/", "1/", NULL,
    };

    char path[0x8C];
    int size;

    if (name == NULL || buffer == NULL) {
        return -1;
    }
    strcpy(path, "menu/");
    strcat(path, language_dirs[LanguageCode]);
    strcat(path, name);
    if (mode == 0) {
        LoadFileBG(path, buffer, &size);
    }
    if (mode == 1) {
        LoadFile2(path, buffer, &size, 0);
    }
    return size;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", ConvertFontCode__FPcPc);
int CheckNowEurope() {
    if (LanguageCode > 0 && LanguageCode < 6) {
        return 1;
    }
    return 0;
}

int MenuCommonReadData(mgCMemory *memory, char **names, int mode) {
    int total;
    int i;
    int size;

    StartReadBG();
    total = 0;
    memory->Align64();

    i = 0;
    while (names[i] != NULL) {
        size = LoadFileMenu(names[i], &memory->stack[memory->stack_used], mode);
        memory->Alloc(align16_blocks(size));
        total += size;
        i++;
        memory->Align64();
    }
    return total;
}

void MenuDeleteTextureBlock(int *blocks) {
    mgCTextureManager *manager = &mgTexManager;
    int i = 0;
    int block;

    while ((block = blocks[i]) >= 0 && i < 16) {
        manager->DeleteBlock(block);
        i++;
    }
}

void MenuWorkTextureEnter(int id, char *name, int width, int height, int format) {
    mgCTextureManager *manager = &mgTexManager;
    int width_rest = width % 64;
    if (width_rest != 0) {
        width += 64 - width_rest;
    }
    int height_rest = height % 64;
    if (height_rest != 0) {
        height += 64 - height_rest;
    }
    manager->EnterTexture(id, name, NULL, width, height, format, NULL, 0, 0);
}

void MenuEnterIMG(int size, u8 *data, char *name) {
    mgCTextureManager *manager = &mgTexManager;

    if (name == NULL) {
        manager->name_suffix[0] = 0;
    } else {
        strcpy(manager->name_suffix, name);
    }
    manager->EnterIMGFile(data, size, NULL, NULL);
    manager->name_suffix[0] = 0;
}

BG_READ_INFO *GetReadBGInfo(char *name) {
    char path[0x80];
    GetCurrentDir(path);
    strcat(path, name);
    return GetReadBGFile(path);
}

void CalcMenu1(float target, float *value, float divisor, float snap_range, int snap) {
    *value += (target - *value) / divisor;
    if (snap != 0 || (float)abs((int)(target - *value)) < snap_range) {
        *value = target;
    }
}

void CalcMenu1(int target, int *value, int divisor, int snap_range, int snap) {
    *value += (target - *value) / divisor;
    if (snap != 0 || abs(target - *value) < snap_range) {
        *value = target;
    }
}

int CalcMenuAdd(int *cursor, int step, int limit) {
    if (cursor == NULL) {
        return -1;
    }
    *cursor += step;
    if ((step < 0 && *cursor < limit) || (step > 0 && *cursor > limit)) {
        *cursor = limit;
        return 1;
    }
    return 0;
}

int CalcMenuAdd(float *cursor, float step, float limit) {
    if (cursor == NULL) {
        return -1;
    }
    *cursor += step;
    if ((step < 0.0f && *cursor < limit) || (step > 0.0f && *cursor > limit)) {
        *cursor = limit;
        return 1;
    }
    return 0;
}

s32 CalcMenuAdd2(s32 *value, s32 delta, s32 limit) {
    if (value == NULL) {
        return -1;
    }
    if ((delta < 0) && ((*value + delta) < limit)) {
        return 1;
    }
    if ((delta > 0) && (limit < (*value + delta))) {
        return 1;
    }
    *value += delta;
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", GetNumberKeta__Fi);
int GetDispVolumeForFloat(float volume) {
    int whole;

    whole = (int)volume;
    if ((volume - (float)whole) < 0.00005f) {
        return whole;
    }
    return whole + 1;
}

float GetFloatCommaValue(float value) {
    return value - (float)(int)value;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", CalcScrlBarPutPos__Fifif);
void Trans3DPosTo2DPos(mgCCamera *camera, mgCFrame *frame, int *out) {
    sceVu0FMATRIX view;
    sceVu0FVECTOR camera_pos;
    sceVu0FVECTOR frame_pos;
    if (camera == NULL || frame == NULL) {
        return;
    }
    camera->GetCameraMatrix(view);
    camera->GetPos(camera_pos);
    mgSetViewMatrix(view, camera_pos);
    frame->GetPosition(frame_pos);
    mgTransWorldScreen(out, frame_pos);
}

/**
 * Allocates and clears the integer menu-information table.
 */
static int _ETCINFO_MALLOC(SPI_STACK *stack, int arg_count) {
    int count;
    CPosDataManage *data;
    MENU_ETCINFO *table;

    count = default_etc_count;
    if (arg_count > 0) {
        count = spiGetStackInt(stack);
    }
    table = (MENU_ETCINFO *)MenuSpiStack->Alloc(
        align16_blocks(count * sizeof(MENU_ETCINFO)));
    data = MenuPosData;
    data->etc_tbl = table;
    data->etc_tbl_num = count;
    MenuPosData->EtcTblClear(0, count);
    return 1;
}

/**
 * Sets the first integer-information entry and clears its relative index.
 */
static int _MENU_ETCINFO_OFFSET(SPI_STACK *stack, int arg_count) {
    Menu_Target_No = spiGetStackInt(stack);
    Menu_Target_No_local = 0;
    return 1;
}

/**
 * Names and fills the next integer-information entry.
 */
static int _MENU_ETCINFO(SPI_STACK *stack, int arg_count) {
    int index;
    char *name;
    int i;
    MENU_ETCINFO *entry;

    index = Menu_Target_No + Menu_Target_No_local;
    if (index >= MenuPosData->etc_tbl_num) {
        return 1;
    }
    entry = &MenuPosData->etc_tbl[index];
    name = spiGetStackString(stack++);
    if (entry->name != NULL) {

        for (i = 0; i < index; i++) {
            if (strcmp(entry->name, name) == 0) {
                return 1;
            }
        }
        return 1;
    }
    entry->name = mgCopyString(name, MenuSpiStack);
    entry->value[0] = spiGetStackInt(stack++);
    entry->value[1] = spiGetStackInt(stack);
    Menu_Target_No_local = Menu_Target_No_local + 1;
    return 1;
}

/**
 * Clears a range of integer-information entries.
 */
static int _MENU_ETCINFO_CLEAR(SPI_STACK *stack, int arg_count) {
    int from;
    int to;

    from = spiGetStackInt(stack++);
    to = 1000;
    if (arg_count > 1) {
        to = spiGetStackInt(stack);
    }
    if (arg_count <= 1 || MenuPosData->etc_tbl_num < to) {
        to = MenuPosData->etc_tbl_num;
    }
    MenuPosData->EtcTblClear(from, to);
    return 1;
}

/**
 * Allocates and clears the floating-point menu-information table.
 */
static int _ETCINFO2_MALLOC(SPI_STACK *stack, int arg_count) {
    int count;
    CPosDataManage *data;
    MENU_ETCINFO2 *table;

    count = default_etc_count;
    if (arg_count > 0) {
        count = spiGetStackInt(stack);
    }
    table = (MENU_ETCINFO2 *)MenuSpiStack->Alloc(
        align16_blocks(count * sizeof(MENU_ETCINFO2)));
    data = MenuPosData;
    data->etc_tbl2 = table;
    data->etc_tbl2_num = count;
    MenuPosData->EtcTbl2Clear(0, count);
    return 1;
}

/**
 * Sets the first floating-point-information entry and clears its relative index.
 */
static int _MENU_ETCINFO2_OFFSET(SPI_STACK *stack, int arg_count) {
    Menu_Target_No = spiGetStackInt(stack);
    Menu_Target_No_local = 0;
    return 1;
}

/**
 * Names and fills the next floating-point-information entry.
 */
static int _MENU_ETCINFO2(SPI_STACK *stack, int arg_count) {
    int index;
    char *name;
    int i;
    int j;
    MENU_ETCINFO2 *entry;

    index = Menu_Target_No + Menu_Target_No_local;
    if (index >= MenuPosData->etc_tbl2_num) {
        return 1;
    }
    entry = &MenuPosData->etc_tbl2[index];
    name = spiGetStackString(stack++);
    if (entry->name != NULL) {
        for (i = 0; i < index; i++) {
            if (strcmp(entry->name, name) == 0) {
                return 1;
            }
        }
    }
    entry->name = mgCopyString(name, MenuSpiStack);
    for (j = 0; j < arg_count - 1; j++) {
        entry->value[j] = spiGetStackFloat(stack++);
    }
    Menu_Target_No_local = Menu_Target_No_local + 1;
    return 1;
}

/**
 * Clears a range of floating-point-information entries.
 */
static int _MENU_ETCINFO2_CLEAR(SPI_STACK *stack, int arg_count) {
    CPosDataManage *data;
    int from;
    int to;
    int count;

    from = spiGetStackInt(stack++);
    to = 1000;
    if (arg_count > 1) {
        to = spiGetStackInt(stack);
    }
    data = MenuPosData;
    count = data->etc_tbl2_num;
    if (arg_count <= 1 || count < to) {
        to = count;
    }
    data->EtcTbl2Clear(from, to);
    return 1;
}

/**
 * Resets every texture block in the menu texture table.
 */
static int _MENU_RESET_TEXINFO(SPI_STACK *stack, int arg_count) {
    MenuPosData->ResetTextureInfoAll();
    return 1;
}

/**
 * Links the named menu forms into their drawing order.
 */
static int _MENU_INIT_DRAWLIST(SPI_STACK *stack, int arg_count) {
    MenuPosData->InitDrawList();
    return 1;
}

/**
 * Clears a range of menu texture entries.
 */
static int _MENU_TEXDATA_CLEAR(SPI_STACK *stack, int argc) {
    int from = spiGetStackInt(stack++);
    int to = 500;
    if (argc > 1) {
        to = spiGetStackInt(stack);
    }
    CPosDataManage *pos_data = MenuPosData;
    int count = pos_data->tex_info_num;
    if (argc <= 1 || count < to) {
        to = count;
    }
    pos_data->TexGetInfoClear(from, to);
    return 1;
}

/**
 * Clears a range of forms and rebuilds their drawing order.
 */
static int _MENU_FORM_CLEAR(SPI_STACK *stack, int argc) {
    int from = 0;
    int to = 100;
    if (argc == 2) {
        from = spiGetStackInt(stack++);
        to = spiGetStackInt(stack);
    }
    MenuPosData->FormInfoClear(from, to);
    MenuPosData->InitDrawList();
    return 1;
}

/**
 * Allocates the menu texture-rectangle table.
 */
static int _MENU_TEXDATA_MALLOC(SPI_STACK *stack, int argc) {
    int count = 256;
    unsigned int bytes;
    unsigned int blocks;
    if (argc > 0) {
        count = spiGetStackInt(stack);
    }
    bytes = count * sizeof(MENU_BASETEXINFO);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    MENU_BASETEXINFO *table = (MENU_BASETEXINFO *)MenuSpiStack->Alloc(blocks);
    CPosDataManage *pos_data = MenuPosData;
    pos_data->tex_info = table;
    pos_data->tex_info_num = count;
    return 1;
}

/**
 * Selects the texture name and the menu texture block.
 */
static int _MENU_TEXNAME(SPI_STACK *stack, int argc) {
    SPI_STACK *block_arg = &stack[1];
    char *name = spiGetStackString(stack);
    if (name != NULL) {
        strcpy(MenuSpiTextureName, name);
    }
    int index = spiGetStackInt(block_arg);
    menu_analyze_texblock = MenuCommonInfo->tex_block[index];
    return 1;
}

/**
 * Sets the first texture entry and clears its relative index.
 */
static int _MENU_TEXDATA_OFFSET(SPI_STACK *stack, int argc) {
    MenuTexPosNo = spiGetStackInt(stack);
    MenuTexPosNo_local = 0;
    return 1;
}

/**
 * Registers a named texture rectangle in the next texture entry.
 */
static int _MENU_TEXDATA(SPI_STACK *stack, int argc) {
    char *name;
    int x;
    int y;
    int w;
    int h;
    int no;
    MENU_BASETEXINFO *slot;

    name = spiGetStackString(stack++);
    x = spiGetStackInt(stack++);
    y = spiGetStackInt(stack++);
    w = spiGetStackInt(stack++);
    h = spiGetStackInt(stack);
    no = MenuTexPosNo + MenuTexPosNo_local;
    slot = MenuPosData->GetTexGetInfo(no);
    if (slot == NULL) {
        return 0;
    }
    if (MenuPosData->GetTexGetInfo(name) != 0) {
        return 0;
    }
    slot->tex_name = mgCopyString(MenuSpiTextureName, MenuSpiStack);
    slot->tex_block = menu_analyze_texblock;
    slot->tbl_no = no;
    slot->name = mgCopyString(name, MenuSpiStack);
    slot->rect.Set(x, y, w, h);
    MenuTexPosNo_local += 1;
    return 1;
}

/**
 * Allocates the menu form table.
 */
static int _MENU_FORM_MALLOC(SPI_STACK *stack, int argc) {
    int count = spiGetStackInt(stack);
    unsigned int bytes = count * sizeof(CMenuPosDataForm);
    unsigned int blocks;
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    u_long128 *block = (u_long128 *)MenuSpiStack->Alloc(blocks + 2);
    CMenuPosDataForm *table = (CMenuPosDataForm *)operator new[](bytes, block);
    CPosDataManage *pos_data = MenuPosData;
    pos_data->form = table;
    pos_data->form_num = count;
    MenuPosData->FormInfoClear(0, count);
    return 1;
}

/**
 * Sets the first form number used by subsequent form definitions.
 */
static int _MENU_FORM_OFFSET_NO(SPI_STACK *stack, int argc) {
    menu_analyze_formno = spiGetStackInt(stack);
    menu_analyze_formno_offset = 0;
    return 1;
}

/**
 * Selects and names the next form.
 */
static int _MENU_FORM_SET(SPI_STACK *stack, int argc) {
    char *name = spiGetStackString(stack++);
    int form_no;
    SPI_STACK *next_slot = stack;
    form_no = menu_analyze_formno + menu_analyze_formno_offset;
    if (argc > 2) {
        form_no = menu_analyze_formno + spiGetStackInt(next_slot);
    }
    menu_formPt = NULL;
    if (MenuPosData->GetFormInfo(name) != 0) {
        return 0;
    }
    menu_formPt = MenuPosData->GetFormInfo(form_no);
    if (menu_formPt != NULL) {
        menu_formPt->Initialize();
        menu_formPt->name = mgCopyString(name, MenuSpiStack);
    }
    menu_formPt->active = 1;
    menu_formPt->draw_flag = 1;
    menu_form_partsno = 0;
    menu_analyze_formno_offset += 1;
    return 1;
}

/**
 * Allocates and initializes the selected form's parts.
 */
static int _MENU_FORM_PARTNUM(SPI_STACK *stack, int argc) {
    int i;
    unsigned int bytes;
    unsigned int blocks;
    if (menu_formPt == NULL) {
        return 0;
    }
    menu_formPt->parts_num = spiGetStackInt(stack);
    bytes = menu_formPt->parts_num * sizeof(MENUFORMPARTS_TYPE);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    menu_formPt->parts = (MENUFORMPARTS_TYPE *)MenuSpiStack->Alloc(blocks);
    for (i = 0; i < menu_formPt->parts_num; i++) {
        MenuPosDataTypeInit(&menu_formPt->parts[i]);
    }
    return 1;
}

/**
 * Copies a named texture rectangle into the current part size.
 */
static void menu_texdata_to_formpart_copy(MENUFORMPARTS_TYPE *part) {
    MENU_BASETEXINFO *tex = MenuPosData->GetTexGetInfo(part->tex_info_no);
    part->w = 0;
    part->h = 0;
    if (tex != NULL) {
        part->w = tex->rect.right;
        part->h = tex->rect.bottom;
    }
}

int menu_spi_analyze_func_strcut1(MENU_SPI_ANALYZE_STRUCT1 *table, char *name) {
    int i = 0;
    while (1) {
        char *entry_name = table[i].name;
        if (entry_name == NULL) {
            break;
        }
        if (strcmp(entry_name, name) == 0) {

            return table[i].value;
        }
        i++;
    }
    return -1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", menu_dtype_init__FP16CMenuPosDataFormP9SPI_STACKi);
/**
 * Sets the selected form's drawing kind and its parameters.
 */
static int _MENU_FORM_DTYPE(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 form_types[] = {
        {"normal", MENUFORM_DTYPE_NORMAL},
        {"itembrd", MENUFORM_DTYPE_ITEMBRD},
        {"giftview", MENUFORM_DTYPE_GIFTVIEW},
        {"msgform", MENUFORM_DTYPE_MSGFORM},
        {"poly", MENUFORM_DTYPE_POLY},
        {"mappart", MENUFORM_DTYPE_MAPPART},
        {"combrd", MENUFORM_DTYPE_COMBRD},
        {"createbrd", MENUFORM_DTYPE_CREATEBRD},
        {"list", MENUFORM_DTYPE_LIST},
        {"bg_tile", MENUFORM_DTYPE_BG_TILE},
        {"dload", MENUFORM_DTYPE_DLOAD},
        {"mainfrm", MENUFORM_DTYPE_MAINFRM},
        {"mainimg", MENUFORM_DTYPE_MAINIMG},
        {"chrstar", MENUFORM_DTYPE_CHRSTAR},
        {"inv_card", MENUFORM_DTYPE_INV_CARD},
        {"geolist", MENUFORM_DTYPE_GEOLIST},
        {"geotitle", MENUFORM_DTYPE_GEOTITLE},
        {"geoana", MENUFORM_DTYPE_GEOANA},
        {"shoplist", MENUFORM_DTYPE_SHOPLIST},
        {"clip", MENUFORM_DTYPE_CLIP},
        {"wmap", MENUFORM_DTYPE_WMAP},
        {"buildup", MENUFORM_DTYPE_BUILDUP},
        {"mosbaji", MENUFORM_DTYPE_MOSBAJI},
        {"savelist", MENUFORM_DTYPE_SAVELIST},
        {"infocur", MENUFORM_DTYPE_INFOCUR},
        {"house", MENUFORM_DTYPE_HOUSE},
        {NULL, -1},
    };

    char *name;
    if (menu_formPt == NULL) {
        return 0;
    }
    name = spiGetStackString(stack++);
    if (name == NULL) {
        return 0;
    }
    menu_formPt->dtype = menu_spi_analyze_func_strcut1(form_types, name);
    menu_dtype_init(menu_formPt, stack, argc);
    return 1;
}

/**
 * Sets the selected form's movement kind.
 */
static int _MENU_FORM_MTYPE(SPI_STACK *stack, int argc) {
    static char *move_types[] = {
        "n",
        "d",
        "l",
        "i",
        "ir",
        NULL,
    };

    char *name = spiGetStackString(stack);
    int type = MENUFORM_MTYPE_N;
    int i = 0;
    char *entry;
    while ((entry = move_types[i]) != 0) {
        if (strcmp(name, entry) == 0) {
            type = i - 1;
            break;
        }
        i++;
    }
    if (type < MENUFORM_MTYPE_N) {
        type = MENUFORM_MTYPE_N;
    }
    menu_formPt->mtype = type;
    return 1;
}

/**
 * Sets whether a selected or named form is drawn.
 */
static int _MENU_FORM_DRAWFLG(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    if (menu_formPt == NULL) {
        return 0;
    }
    if (argc == 1) {
        menu_formPt->draw_flag = (spiGetStackInt(stack++) != 0);
    }
    if (argc == 2) {
        form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
        int value = spiGetStackInt(stack);
        if (form != NULL) {
            form->draw_flag = (value != 0);
        }
    }
    return 1;
}

/**
 * Sets the selected form's vibration counters.
 */
static int _MENU_FORM_VIBECNT(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot;

    next_slot = &stack[1];
    if (menu_formPt == NULL) {
        return 0;
    }
    menu_formPt->vibe_cnt[0] = spiGetStackInt(stack);
    menu_formPt->vibe_cnt[1] = spiGetStackInt(next_slot);
    return 1;
}

/**
 * Completes the current form definition.
 */
static int _MENU_FORM_SETEND(SPI_STACK *stack, int argc) {
    return 1;
}

/**
 * Sets the selected form's movement rates.
 */
static int _MENU_FORM_MOVERATE(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];

    if (menu_formPt == NULL) {
        return 0;
    }
    menu_formPt->rate_x = spiGetStackFloat(stack);
    menu_formPt->rate_y = spiGetStackFloat(next_slot);
    return 1;
}

/**
 * Sets the selected form's drawing position.
 */
static int _MENU_FORM_PUTXY(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];

    if (menu_formPt == NULL) {
        return 0;
    }
    menu_formPt->x = (float)spiGetStackInt(stack);
    menu_formPt->y = (float)spiGetStackInt(next_slot);
    return 1;
}

/**
 * Sets the selected form's color and resets its color transitions.
 */
static int _MENU_FORM_RGBA(SPI_STACK *stack, int argc) {
    int values[4];
    int i;
    int step;
    CMenuPosDataForm *form;
    if (menu_formPt == NULL) {
        return 0;
    }
    for (i = 0; i < argc; i++) {
        values[i] = spiGetStackInt(stack++);
    }
    if (argc == 3) {
        form = menu_formPt;
        form->rgba[0] = values[0];
        form->rgba[1] = values[1];
        form->rgba[2] = values[2];
        form->rgba[3] = 0x80;
        for (step = 0; step < 4; step++) {
            form->SetRGBACalcParam(step, 0, 0x80);
        }
    }
    if (argc == 4) {
        form = menu_formPt;
        form->rgba[0] = values[0];
        form->rgba[1] = values[1];
        form->rgba[2] = values[2];
        form->rgba[3] = values[3];
        for (i = 0; i < 4; i++) {
            form->SetRGBACalcParam(i, 0, 0x80);
        }
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", _MENU_FORM_RGBA_BIT__FP9SPI_STACKi);
/**
 * Allocates the selected form's named-action table.
 */
static int _MENU_ACTION_TABLE_NUM(SPI_STACK *stack, int argc) {
    int count;
    MENU_FORM_ACTION *table;
    unsigned int bytes;
    unsigned int blocks;

    if (menu_formPt == NULL) {
        return 0;
    }
    count = spiGetStackInt(stack);
    bytes = count * sizeof(MENU_FORM_ACTION);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    table = (MENU_FORM_ACTION *)MenuSpiStack->Alloc(blocks);
    menu_formPt->action = table;
    menu_formPt->action_num = count;
    menu_spi_form_action_info = table;
    return 1;
}

/**
 * Fills the next named movement action.
 */
static int _MENU_ACTION_DEF(SPI_STACK *stack, int argc) {
    MENU_FORM_ACTION_MOVE *action;
    strcpy(menu_spi_form_action_info->name, spiGetStackString(stack++));
    menu_spi_form_action_info->move = (MENU_FORM_ACTION_MOVE *)MenuSpiStack->Alloc(2);
    action = menu_spi_form_action_info->move;
    action->mtype = spiGetStackInt(stack++);
    action->x = spiGetStackFloat(stack++);
    action->y = spiGetStackFloat(stack++);
    action->rate_x = spiGetStackFloat(stack++);
    action->rate_y = spiGetStackFloat(stack);
    menu_spi_form_action_info++;
    return 1;
}

/**
 * Starts a named action on the selected form.
 */
static int _MENU_ACTION_SETACTION(SPI_STACK *stack, int argc) {
    if (menu_formPt == NULL) {
        return 0;
    }
    menu_formPt->SetAction(spiGetStackString(stack));
    return 1;
}

/**
 * Sets the current part's vibration counters.
 */
static int _MENU_PARTVIBECNT(SPI_STACK *stack, int argc) {
    menu_form_part->vibe_cnt[0] = spiGetStackInt(stack++);
    menu_form_part->vibe_cnt[1] = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the current part's vibration radii.
 */
static int _MENU_PARTVIBER(SPI_STACK *stack, int argc) {
    menu_form_part->viber[0] = spiGetStackInt(stack++);
    menu_form_part->viber[1] = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the current part's shadow flag and offset.
 */
static int _MENU_SHADOW_ONOFF(SPI_STACK *stack, int argc) {
    menu_form_part->shadow = 1;
    menu_form_part->shadow_offset = 4;
    if (argc > 0) {
        menu_form_part->shadow = (spiGetStackInt(stack++) != 0);
    }
    if (argc == 2) {
        menu_form_part->shadow_offset = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the selected form's clipping width and height.
 */
static int _CLIP_WH(SPI_STACK *stack, int argc) {
    int width;
    int height;
    if (menu_formPt == NULL) {
        return 0;
    }
    width = mgScreenWidth - 1;
    height = mgScreenHeight - 1;
    if (argc == 2) {
        width = spiGetStackInt(stack++);
        height = spiGetStackInt(stack);
    }
    menu_formPt->clip_w = width;
    menu_formPt->clip_h = height;
    return 1;
}

/**
 * Sets the current part's color.
 */
static int _MENU_PARTRGBA(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part;
    int i;

    part = menu_form_part;
    if (part == NULL) {
        return 0;
    }
    if (argc == 1) {
        part->rgba[3] = spiGetStackInt(stack);
        part->rgba[2] = 0x80;
        part->rgba[1] = 0x80;
        part->rgba[0] = 0x80;
    } else if (argc == 3) {
        part->rgba[0] = spiGetStackInt(stack++);
        part->rgba[1] = spiGetStackInt(stack++);
        part->rgba[2] = spiGetStackInt(stack);
        part->rgba[3] = 0x80;
    } else if (argc == 4) {
        for (i = 0; i < 4; i++) {
            part->rgba[i] = spiGetStackInt(stack++);
        }
    }
    return 1;
}

/**
 * Sets the current part's alpha-blending mode.
 */
static int _MENU_PART_ALPHA_BLEND(SPI_STACK *stack, int argc) {
    if (menu_form_part == NULL) {
        return 0;
    }
    menu_form_part->alpha_blend = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets indexed parameters on the current part.
 */
static int _MENU_PART_ETCINFO(SPI_STACK *stack, int argc) {
    int pairs;
    int i;
    int index;
    if (menu_form_part == NULL) {
        return 0;
    }
    pairs = argc / 2;
    for (i = 0; i < pairs; i++) {
        index = spiGetStackInt(stack++);

        menu_form_part->etc_info[index] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Enables bilinear sampling on the current part.
 */
static int _MENU_PART_BILINEAR(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_form_part;
    u8 *field = &part->bilinear;
    u8 value;

    if (part == NULL) {
        return 0;
    }
    value = part->bilinear;
    if (value == 0) {
        *field = 1;
    } else {
        *field = value | 1;
    }
    return 1;
}

/**
 * Copies the script argument into the part name.
 */
static void MakePartsName(SPI_STACK *stack, MENUFORMPARTS_TYPE *part) {
    part->name = mgCopyString(spiGetStackString(stack), MenuSpiStack);
}

/**
 * Creates a part with the named drawing kind and its parameters.
 */
static int _MENU_PART_DTYPE(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 part_types[] = {
        {"clut_reload", MENUFORMPARTS_DTYPE_CLUT_RELOAD},
        {"\224\255\226\276\203l\203^", MENUFORMPARTS_DTYPE_IDEA_BOARD},
        {"\203A\203\213\203o\203\200", MENUFORMPARTS_DTYPE_ALBUM},
        {"\202\262\202\277\202\341\220\374", MENUFORMPARTS_DTYPE_RANDOM_LINE},
        {"font", MENUFORMPARTS_DTYPE_FONT},
        {"\203l\203^\222\240", MENUFORMPARTS_DTYPE_IDEA_MEMO},
        {NULL, -1},
    };

    MENUFORMPARTS_TYPE *part;
    int i;
    if (menu_formPt == NULL) {
        return 0;
    }
    part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->dtype = menu_spi_analyze_func_strcut1(part_types, spiGetStackString(stack++));
    MakePartsName(stack++, part);
    if (part->dtype == MENUFORMPARTS_DTYPE_CLUT_RELOAD) {
        for (i = 0; i < argc - 2; i++) {
            part->etc_info[i] = spiGetStackInt(stack++);
        }
    } else if (part->dtype == MENUFORMPARTS_DTYPE_RANDOM_LINE) {
        part->x = spiGetStackInt(stack++);
        part->y = spiGetStackInt(stack++);
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    }
    part->draw_flag = 1;
    part->active = 1;
    return 1;
}

/**
 * Creates a textured part with the first normal drawing kind.
 */
static int _MENU_NORMAL(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part;
    if (menu_formPt == NULL) {
        return 0;
    }
    part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    if (argc > 4) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    } else {
        menu_texdata_to_formpart_copy(part);
    }
    part->dtype = MENUFORMPARTS_DTYPE_NORMAL;
    part->active = 1;
    return 1;
}

/**
 * Creates a textured part with the second normal drawing kind.
 */
static int _MENU_NORMAL2(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    if (argc > 4) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    } else {
        menu_texdata_to_formpart_copy(part);
    }
    part->dtype = MENUFORMPARTS_DTYPE_NORMAL2;
    part->active = 1;
    return 1;
}

/**
 * Creates a cursor part.
 */
static int _MENU_CURSOR(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack);
    part->dtype = MENUFORMPARTS_DTYPE_CURSOR;
    part->active = 1;
    part->draw_flag = 1;
    return 1;
}

/**
 * Creates a function-information part.
 */
static int _MENU_FUNCINFO(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack);
    part->dtype = MENUFORMPARTS_DTYPE_FUNCINFO;
    part->active = 1;
    part->draw_flag = 0;
    return 1;
}

/**
 * Creates a part that draws a number with the first number style.
 */
static int _MENU_NUMBER1(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->etc_info[0] = spiGetStackInt(stack++);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    if (argc > 5) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    }
    part->dtype = MENUFORMPARTS_DTYPE_NUMBER1;
    part->active = 1;
    return 1;
}

/**
 * Creates a part that draws a number with the second number style.
 */
static int _MENU_NUMBER2(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->etc_info[0] = spiGetStackInt(stack++);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    if (argc > 5) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    }
    part->etc_info[1] = 0;
    part->dtype = MENUFORMPARTS_DTYPE_NUMBER2;
    part->active = 1;
    return 1;
}

/**
 * Creates a frame-image part with the named image kind.
 */
static int _MENU_FRMIMG(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 image_types[] = {
        {"bg", MENUFORMPARTS_DTYPE_BG},
        {"beta", MENUFORMPARTS_DTYPE_BETA},
        {NULL, -1},
    };

    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->dtype = menu_spi_analyze_func_strcut1(image_types, spiGetStackString(stack++));
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    part->w = spiGetStackInt(stack++);
    part->h = spiGetStackInt(stack);
    if (part->bilinear != 0) {
        part->bilinear |= 1;
    } else {
        part->bilinear = 1;
    }
    part->active = 1;
    return 1;
}

/**
 * Creates a part that draws another named form.
 */
static int _MENU_FORM(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = spiGetStackInt(stack++);
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack);
    part->dtype = MENUFORMPARTS_DTYPE_FORM;
    part->active = 1;
    return 1;
}

/**
 * Creates an item part with the named item drawing kind.
 */
static int _MENU_ITEM(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 item_types[] = {
        {"trs", MENUFORMPARTS_DTYPE_TRS},
        {"neta", MENUFORMPARTS_DTYPE_NETA},
        {NULL, -1},
    };

    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->dtype = menu_spi_analyze_func_strcut1(item_types, spiGetStackString(stack++));
    part->etc_info[1] = 0;
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    part->w = 32.0f;
    part->h = 40.0f;
    if (argc > 4) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    }
    part->active = 1;
    part->draw_flag = 1;
    return 1;
}

/**
 * Creates an item check-mark part.
 */
static int _MENU_ITEM_CHECKMARK(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->dtype = MENUFORMPARTS_DTYPE_CHECKMARK;
    part->tex_info_no = MenuCommonInfo->tex_block[0];
    MakePartsName(stack++, part);
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    if (argc > 3) {
        part->w = spiGetStackInt(stack++);
        part->h = spiGetStackInt(stack);
    } else {
        menu_texdata_to_formpart_copy(part);
    }
    part->active = 1;
    part->draw_flag = 1;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucommon", _MENU_FILLBOX__FP9SPI_STACKi);
/**
 * Stores four parameters in the next part effect and advances the effect entry.
 */
static int _MENU_FILLBOXINFO(SPI_STACK *stack, int argc) {
    menu_parts_effect_ptr->type = MENU_PARTS_EFFECT_UNK_1;
    for (int i = 0; i < 4; i++) {
        menu_parts_effect_ptr->param[i] = spiGetStackInt(stack++);
    }
    menu_parts_effect_ptr++;
    return 1;
}

/**
 * Creates a rectangular frame part.
 */
static int _MENU_WAKU_RECT(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->dtype = MENUFORMPARTS_DTYPE_WAKU_RECT;
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    part->w = spiGetStackInt(stack++);
    part->h = spiGetStackInt(stack);
    part->active = 1;
    part->draw_flag = 1;
    return 1;
}

/**
 * Creates a circular frame part.
 */
static int _MENU_WAKU_CIRCLE(SPI_STACK *stack, int argc) {
    MENUFORMPARTS_TYPE *part = menu_formPt->GetEnableEnterPart();
    menu_form_part = part;
    part->tex_info_no = MenuPosData->GetTexGetInfoTblNo(spiGetStackString(stack++));
    MakePartsName(stack++, part);
    part->dtype = MENUFORMPARTS_DTYPE_WAKU_CIRCLE;
    part->x = spiGetStackInt(stack++);
    part->y = spiGetStackInt(stack++);
    part->w = spiGetStackInt(stack++);
    part->h = spiGetStackInt(stack);
    part->active = 1;
    part->draw_flag = 1;
    return 1;
}

/**
 * Allocates the current part's effect entries.
 */
static int _MENU_PARTS_EFF_NUM(SPI_STACK *stack, int argc) {
    int count = spiGetStackInt(stack);
    unsigned int bytes;
    unsigned int blocks;
    if (menu_form_part == NULL || count <= 0) {
        return 0;
    }
    menu_form_part->effect_num = count;
    bytes = count * sizeof(MENU_PARTS_EFFECT_STRUCT1);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    menu_form_part->effect = (MENU_PARTS_EFFECT_STRUCT1 *)MenuSpiStack->Alloc(blocks);
    menu_parts_effect_ptr = menu_form_part->effect;
    return 1;
}

/**
 * Sets the next part effect's kind and parameters.
 */
static int _MENU_PARTS_EFFECT(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 effect_types[] = {
        {"blink", MENU_PARTS_EFFECT_BLINK},
        {"rot", MENU_PARTS_EFFECT_ROT},
        {"huriko", MENU_PARTS_EFFECT_HURIKO},
        {"stretch", MENU_PARTS_EFFECT_STRETCH},
        {"stretch_rep", MENU_PARTS_EFFECT_STRETCH_REP},
        {"stretch_sin", MENU_PARTS_EFFECT_STRETCH_SIN},
        {NULL, 0},
    };

    MENU_PARTS_EFFECT_STRUCT1 *effect;
    char *name;
    int i;
    name = spiGetStackString(stack++);
    effect = menu_parts_effect_ptr;
    if (effect == NULL || name == 0) {
        return 0;
    }
    effect->type = menu_spi_analyze_func_strcut1(effect_types, name);
    effect->active = 1;
    effect->repeat = 1;
    for (i = 0; i < argc - 1; i++) {
        effect->param[i] = spiGetStackInt(stack++);
    }
    menu_parts_effect_ptr++;
    return 1;
}

int MenuDataAnalyze(char *script, int size, mgCMemory *memory) {
    if (script == NULL) {
        return 0;
    }
    MenuSpiStack = memory;
    CScriptInterpreter interpreter;
    interpreter.SetTag(menu_analyze_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
    return 1;
}

/**
 * Enables the commands belonging to the requested command name.
 */
static int _MENU_EXE_COMMAND_NAME(SPI_STACK *stack, int argc) {
    char *name;

    name = spiGetStackString(stack);
    SpiMenuExeCommandFlag = 0;
    if (strcmp(MenuCommandAnalyzeInfo.command_name, name) == 0) {
        SpiMenuExeCommandFlag = 1;
    }
    return 1;
}

/**
 * Sets a named form's drawing flag for the enabled command.
 */
static int _MENU_EXE_FORM_DRAWFLAG(SPI_STACK *stack, int argc) {
    int draw;
    int count;
    int i;
    char *name;
    CMenuPosDataForm *form;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    draw = spiGetStackInt(stack++);
    count = argc - 1;
    for (i = 0; i < count; i++) {
        name = spiGetStackString(stack++);
        form = MenuPosData->GetFormInfo(name);
        if (form != NULL) {
            form->draw_flag = (draw != 0);
        } else {
            printf("%s not found\n", name);
        }
    }
    return 1;
}

/**
 * Sets a named form's color for the enabled command.
 */
static int _MENU_EXE_FORM_RGBA(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    int i;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
    if (form == NULL) {
        return 1;
    }
    if (argc == 1) {
        form->rgba[0] = 0x80;
        form->rgba[1] = 0x80;
        form->rgba[2] = 0x80;
        form->rgba[3] = 0x80;
        for (i = 0; i < 4; i++) {
            form->SetRGBACalcParam(i, 0, 0x80);
        }
    } else {
        int r = spiGetStackInt(stack++);
        int g = spiGetStackInt(stack++);
        int b = spiGetStackInt(stack++);
        int a = spiGetStackInt(stack);
        form->rgba[0] = r;
        form->rgba[1] = g;
        form->rgba[2] = b;
        form->rgba[3] = a;
        for (i = 0; i < 4; i++) {
            form->SetRGBACalcParam(i, 0, 0x80);
        }
    }
    return 1;
}

/**
 * Sets a named form's color transition for the enabled command.
 */
static int _MENU_EXE_FORM_CALCRGBAPARAM(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    int mode;
    int param1;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
    if (form == NULL) {
        return 1;
    }
    mode = spiGetStackInt(stack++);
    param1 = spiGetStackInt(stack++);
    form->SetRGBACalcParam(mode, param1, spiGetStackInt(stack));
    return 1;
}

/**
 * Starts a named form's fade for the enabled command.
 */
static int _MENU_EXE_FORM_FADE(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    int mode;
    int frames;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
    if (form == NULL) {
        return 1;
    }
    mode = spiGetStackInt(stack++);
    frames = spiGetStackInt(stack);
    if (mode == 0) {
        form->FormFadeIn(frames, 1);
    }
    if (mode == 1) {
        form->FormFadeOut(frames, 1);
    }
    return 1;
}

/**
 * Sets a named form's position for the selected language.
 */
static int _MENU_EXE_FORM_SETPOS(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    CMenuPosDataForm *form = MenuPosData->GetFormInfo(
        spiGetStackString(stack++));
    if (form == NULL) {
        return 1;
    }
    int x = spiGetStackInt(stack++);
    int y = spiGetStackInt(stack++);
    int language = LanguageCode;
    if (argc >= 4) {
        language = spiGetStackInt(stack);
    }
    if (language != LanguageCode) {
        return 0;
    }
    form->x = x;
    form->y = y;
    return 1;
}

/**
 * Starts a named form's action for the enabled command.
 */
static int _MENU_EXE_FORM_SETACTION(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    SPI_STACK *next_slot = &stack[1];
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack));
    if (form == NULL) {
        return 1;
    }
    form->SetAction(spiGetStackString(next_slot));
    return 1;
}

/**
 * Sets a named part's drawing flag for the enabled command.
 */
static int _MENU_EXE_FORM_PARTSONOFF(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    MENUFORMPARTS_TYPE *part;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
    if (form == NULL) {
        return 0;
    }
    part = form->GetPartInfo(spiGetStackString(stack++));
    if (part == NULL) {
        return 0;
    }
    part->draw_flag = (spiGetStackInt(stack) != 0);
    return 1;
}

/**
 * Sets drawing flags on a list of named parts.
 */
static int _MENU_EXE_FORM_PARTSONOFF_GRP(SPI_STACK *stack, int argc) {
    CMenuPosDataForm *form;
    int on;
    int count;
    int i;
    MENUFORMPARTS_TYPE *part;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = MenuPosData->GetFormInfo(spiGetStackString(stack++));
    if (form == NULL) {
        return 0;
    }
    on = spiGetStackInt(stack++);
    count = argc - 2;
    for (i = 0; i < count; i++) {
        part = form->GetPartInfo(spiGetStackString(stack++));
        if (part != NULL) {
            part->draw_flag = (on != 0);
        }
    }
    return 1;
}

/**
 * Exchanges two named forms in the drawing order.
 */
static int _MENU_EXE_FORM_SWAP(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    char *form;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = spiGetStackString(stack);
    MenuPosData->FormReLink(form, spiGetStackString(next_slot));
    return 1;
}

/**
 * Exchanges two runs of named forms in the drawing order.
 */
static int _MENU_EXE_FORM_GROUP_SWAP(SPI_STACK *stack, int argc) {
    char *form;
    char *target;
    char *third;
    char *fourth;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    form = spiGetStackString(stack++);
    target = spiGetStackString(stack++);
    third = spiGetStackString(stack++);
    fourth = spiGetStackString(stack);
    MenuPosData->FormReLink2(form, target, third, fourth);
    return 1;
}

/**
 * Selects the named preset for a message window.
 */
static int _MENU_EXE_MSGENV(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 message_types[] = {
        {"default", 0},
        {"default_black", 1},
        {"no_win", 2},
        {"system", 3},
        {"name", 4},
        {"name_black", 5},
        {"itemcmd", 6},
        {"invent", 7},
        {"geo", 8},
        {"msgdic", 9},
        {"general", 10},
        {"general_2", 18},
        {"yesno", 11},
        {"brd3", 12},
        {"helpwin", 13},
        {"makebrd", 14},
        {"itemmsg", 15},
        {"itemmsg_defaultfuchi", 16},
        {"volmsg", 17},
        {"talk", 19},
        {NULL, 0},
    };

    SPI_STACK *next_slot = &stack[1];
    int msg_no;
    int preset;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    msg_no = spiGetStackInt(stack);
    preset = menu_spi_analyze_func_strcut1(message_types, spiGetStackString(next_slot));
    MenuDCMsg[msg_no]->MsgPreset(preset, LanguageCode);
    return 1;
}

/**
 * Creates the selected message in a message window.
 */
static int _MENU_EXE_MAKEMSG(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    int message;
    int id;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    message = spiGetStackInt(stack);
    id = spiGetStackInt(next_slot);
    MenuDCMsg[message]->MakeMsg(id);
    return 1;
}

/**
 * Sets whether a message window uses absolute positioning.
 */
static int _MENU_EXE_SETABSPOS(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    int index;
    int value;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    index = spiGetStackInt(stack);
    value = spiGetStackInt(next_slot);
    MenuDCMsg[index]->SetAbsPos(value);
    return 1;
}

/**
 * Selects the system-message buffer of a message window.
 */
static int _MENU_EXE_MSGSETSYSTEMBUFF(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    int message;
    short *buffer;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    message = spiGetStackInt(stack);
    buffer = MenuCommandAnalyzeInfo.system_mes_buff[spiGetStackInt(next_slot)];
    if (buffer == NULL) {
        buffer = GetSystemMesBuffer();
    }
    MenuDCMsg[message]->SetBuff_system(buffer);
    return 1;
}

/**
 * Selects the message buffer of a message window.
 */
static int _MENU_EXE_MSGSETBUFF(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    int message_index = spiGetStackInt(stack);
    int buffer_index = spiGetStackInt(next_slot);
    MenuDCMsg[message_index]->SetBuff(MenuCommandAnalyzeInfo.mes_buff[buffer_index]);
    return 1;
}

/**
 * Sets the named border style of a message window.
 */
static int _MENU_EXE_MSGSETFUCHI(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 border_types[] = {
        {"default", 5},
        {"none", 0},
        {"ol2", 8},
        {NULL, 0},
    };

    SPI_STACK *next_slot = &stack[1];
    int message;
    char *text;
    int type;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    message = spiGetStackInt(stack);
    text = spiGetStackString(next_slot);
    type = menu_spi_analyze_func_strcut1(border_types, text);
    MenuDCMsg[message]->fuchi = type;
    return 1;
}

/**
 * Sets the cursor of a message window.
 */
static int _MENU_EXE_MSGSETCURSOR(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    int index;
    int cursor;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    index = spiGetStackInt(stack);
    cursor = spiGetStackInt(next_slot);
    MenuDCMsg[index]->SetMsgCursor(cursor);
    return 1;
}

/**
 * Sets the first selection line of a message window.
 */
static int _MENU_SET_QUESTIONGYOU(SPI_STACK *stack, int argc) {
    SPI_STACK *next_slot = &stack[1];
    int msg_no;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    msg_no = spiGetStackInt(stack);
    MenuDCMsg[msg_no]->select_top = spiGetStackInt(next_slot);
    return 1;
}

/**
 * Sets the opening speed of a message window.
 */
static int _MENU_SET_OPENSPEED(SPI_STACK *stack, int argc) {
    int message;
    SPI_STACK *next_slot = &stack[1];

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    message = spiGetStackInt(stack);
    MenuDCMsg[message]->fade_speed = spiGetStackFloat(next_slot);
    return 1;
}

/**
 * Sets whether the menu accepts key input.
 */
static int _MENU_INPUT_KEY(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    MenuCommonInfo->key_enable = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets whether the menu cursor form is drawn.
 */
static int _MENU_CURSOR_ONOFF(SPI_STACK *stack, int argc) {
    int value;
    CMenuPosDataForm *cursor;

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    value = spiGetStackInt(stack);
    cursor = MenuCommonInfo->cursor_form;
    if (cursor != NULL) {
        cursor->draw_flag = (value != 0);
    }
    return 1;
}

/**
 * Starts a fade on the menu cursor form.
 */
static int _MENU_CURSOR_FADE(SPI_STACK *stack, int argc) {
    int fade_in;
    int speed;
    int steps;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    fade_in = spiGetStackInt(stack++);
    speed = 10;
    if (argc == 2) {
        speed = spiGetStackInt(stack++);
    }
    steps = 1;
    if (argc == 3) {
        steps = spiGetStackInt(stack);
    }
    if (fade_in != 0) {
        MenuCommonInfo->CursorFadeIn(speed, steps);
    } else {
        MenuCommonInfo->CursorFadeOut(speed, steps);
    }
    return 1;
}

/**
 * Sets the menu cursor's frame kind.
 */
static int _MENU_WAKUTYPE(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    MenuCommonInfo->SetWakuType(spiGetStackInt(stack));
    return 1;
}

/**
 * Starts and steps the menu scene's fade.
 */
static int _MENU_SCENE_FADE(SPI_STACK *stack, int argc) {
    int fade_in;
    int frames;
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    fade_in = spiGetStackInt(stack++);
    frames = 40;
    if (argc == 2) {
        frames = spiGetStackInt(stack);
    }
    if (fade_in != 0) {
        MenuMainScene->fade.FadeIn(frames);
    } else {
        MenuMainScene->fade.FadeOut(frames, 0.0f, 0.0f, 0.0f);
    }
    MenuMainScene->fade.FadeStep();
    return 1;
}

/**
 * Plays the menu sound selected by its name.
 */
static int _MENU_SE_PLAY(SPI_STACK *stack, int argc) {
    static MENU_SPI_ANALYZE_STRUCT1 sounds[] = {
        {"OK", 1},
        {"CANCEL", 5},
        {NULL, -1},
    };

    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    MenuSePlay(menu_spi_analyze_func_strcut1(sounds, spiGetStackString(stack)));
    return 1;
}

/**
 * Rebuilds the form drawing order for the enabled command.
 */
static int _MENU_EXE_INIT_DRAWLIST(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    MenuPosData->InitDrawList();
    return 1;
}

/**
 * Resets texture blocks for the enabled command.
 */
static int _MENU_EXE_RESET_TEXINFO(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    MenuPosData->ResetTextureInfoAll();
    return 1;
}

/**
 * Prints the supplied message followed by a newline.
 */
static int _MENU_DEBUG_PRINTF(SPI_STACK *stack, int argc) {
    if (SpiMenuExeCommandFlag == 0) {
        return 1;
    }
    printf(spiGetStackString(stack));
    printf("\n");
    return 1;
}

void MenuCommandAnalyze(char *script, int size, char *command_name) {
    if ((script != NULL) && (command_name != NULL)) {
        strcpy(MenuCommandAnalyzeInfo.command_name, command_name);
        CScriptInterpreter interpreter;
        interpreter.SetTag(menu_execommand_analyze_tag);
        interpreter.SetScript(script, size);
        interpreter.Run();
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", sort_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", langdirpathTable_1161__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", mes_cord_conv_1193__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_1728__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_1759__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_1994__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2060__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2074__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2090__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2144__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", menu_analyze_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2369__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2422__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", tbl_2516__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", menu_execommand_analyze_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1162__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1163__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1164__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1165__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1166__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1167__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1173__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1729__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1730__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1731__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1732__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1733__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1734__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1735__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1736__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1737__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1738__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1739__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1740__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1741__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1742__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1743__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1744__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1745__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1746__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1747__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1748__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1749__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1750__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1751__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1752__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1753__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1754__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1760__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1761__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1762__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1763__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1764__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1995__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1996__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1997__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1998__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_1999__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2000__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2061__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2062__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2075__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2076__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2091__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2145__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2146__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2147__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2148__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2149__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2150__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2161__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2162__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2163__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2164__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2165__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2166__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2167__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2168__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2169__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2170__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2171__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2172__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2173__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2174__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2175__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2176__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2177__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2178__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2179__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2180__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2181__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2182__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2183__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2184__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2185__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2186__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2187__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2188__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2189__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2190__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2191__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2192__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2193__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2194__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2195__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2196__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2197__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2198__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2199__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2200__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2201__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2202__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2203__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2204__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2205__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2206__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2207__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2208__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2209__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2210__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2211__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2212__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2213__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2214__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2215__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2216__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2217__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2218__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2219__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2253__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2370__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2371__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2372__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2373__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2374__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2375__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2376__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2377__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2378__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2379__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2380__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2381__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2382__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2383__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2384__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2385__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2386__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2387__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2388__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2389__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2423__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2424__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2517__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2518__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2538__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2539__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2540__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2541__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2542__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2543__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2544__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2545__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2546__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2547__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2548__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2549__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2550__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2551__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2552__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2553__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2554__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2555__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2556__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2557__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2558__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2559__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2560__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2561__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2562__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2563__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2564__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2565__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2566__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2567__2__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", sort_top_type__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucommon", at_2092__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(MenuSePlayUsedFlag, 0x4);
INCLUDE_BSS(SndPortVol_Ob, 0x4);
INCLUDE_BSS(SndPortVol_Base, 0x4);
INCLUDE_BSS(SndPortVol_Event, 0x4);
INCLUDE_BSS(SndPortCheck_EventPort, 0x4);
INCLUDE_BSS(SndPortVol_Env, 0x4);
INCLUDE_BSS(MenuTexPosNo, 0x4);
INCLUDE_BSS(MenuTexPosNo_local, 0x4);
INCLUDE_BSS(menu_analyze_texblock, 0x4);
INCLUDE_BSS(MenuSpiStack, 0x4);
INCLUDE_BSS(Menu_Target_No, 0x4);
INCLUDE_BSS(Menu_Target_No_local, 0x4);
INCLUDE_BSS(menu_formPt, 0x4);
INCLUDE_BSS(menu_form_part, 0x4);
INCLUDE_BSS(menu_parts_effect_ptr, 0x4);
INCLUDE_BSS(menu_analyze_formno, 0x4);
INCLUDE_BSS(menu_analyze_formno_offset, 0x4);
INCLUDE_BSS(menu_form_partsno, 0x4);
INCLUDE_BSS(menu_spi_form_action_info, 0x4);
INCLUDE_BSS(SpiMenuExeCommandFlag, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(MenuSpiTextureName, 0x20);
INCLUDE_BSS(MenuCommandAnalyzeInfo, 0x70);
