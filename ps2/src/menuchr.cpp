#include "common.h"
#include "menuchr.hpp"
#include "mapselect.hpp"
#include "monster.hpp"
#include "npccfg.hpp"
#include "charasetup.hpp"
#include "dynamicanime.hpp"
#include "sound.hpp"
#include "dataread.hpp"
#include "prespr.hpp"
#include "mg_drawprim.hpp"
#include <cstdio>
#include <cstring>
#include "effscript.hpp"
#include "map.hpp"
#include "font.hpp"
#include "sysmes.hpp"
#include "scenesnd.hpp"
#include "savedata.hpp"
#include "userdata.hpp"
#include "gamedata.hpp"
#include "scriptinterpreter.hpp"
#include "mg_math.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "mainloop.hpp"
#include "menucls1.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "menusys.hpp"
#include "menumain.hpp"
#include "memcard.hpp"
#include "menuop.hpp"
#include "mapload.hpp"
#include "menuaqua.hpp"

mgCMemory MenuActionCharaBuffer[MENU_CHARA_LOAD_MAX];
static mgCMemory MenuChangeMemory;
mgCMemory MenuChangeNpcMemory;
static mgCMemory ChrChangeInitTextureStack;
static mgCMemory MenuMonChangeLoadStack;
static mgCMemory MenuMosBuildStack;
static mgCMemory MenuMosLoadStack;
mgCMemory SwordEffectStack;
static mgCMemory MosBookStack;

static inline unsigned int blocks_for(unsigned int size) {
    return (size & 0xF) != 0 ? (size >> 4) + 1 : size >> 4;
}

/**
 * Whether the party NPC model was read successfully.
 */
static s8 MenuNPCLoadFlag;
/**
 * Number of monster transformation effects read.
 */
static int mos_effect_read_num;

/**
 * Transformation effect parameters of the selected monster.
 */
static MOS_HENGE_PARAM *mos_effect_henge_param;
/**
 * Script file buffers of the transformation effects.
 */
static u8 *mos_effect_readbuff1[4];
/**
 * Byte sizes of the transformation effect scripts.
 */
static int mos_effect_readbuff1_size[4];
/**
 * Model pack buffers of the transformation effects.
 */
static u8 *mos_effect_readbuff2[4];
/**
 * Byte sizes of the transformation effect model packs.
 */
static int mos_effect_readbuff2_size[4];
/**
 * Number of selections in the monster badge menu.
 */
static int max_3170 = 4;
/**
 * Number of badge selections visible at once.
 */
static int viewnum_3171 = 4;
static CMenuMosSelect *MenuMosSelectPtr;
/**
 * Character whose menu sound bank is loaded.
 */
static int MenuSoundCharaNo = -1;
/**
 * Texture sliding across the main character preview.
 */
static mgCTexture *NowMainCharaChngTex;
/**
 * Background image of the main character preview.
 */
static mgCTexture *NowMainCharaFrameImage;
/**
 * Flags controlling the preview image and message.
 */
static short NowMainCharaChngStatusBit;
/**
 * Phase of the sliding character image.
 */
static int NowMainCharaChngTexMovePhase;
/**
 * Horizontal position of the sliding character image.
 */
static int NowMainCharaChngTexMoveX;
/**
 * Character whose main preview image is being read.
 */
static short NowReadMainCharaNo = -1;
/**
 * Buffer containing the party NPC model pack.
 */
static u8 *MenuPartyNPCModelReadBuffer;
/**
 * Phase of loading the selected costume.
 */
static short MenuCosutumeLoadPhase;
/**
 * Costume selection menu currently open.
 */
static CMenuCostumeSel *MenuCosPtr;
/**
 * Monster book menu currently open.
 */
static CMosBookMenu *MenuMosBookPtr;
/**
 * Monster book records in the save data.
 */
static u8 *MonsterBookPtr;
/**
 * Mode the monster book was opened in.
 */
static short MonsterBookBootMode;
/**
 * Base texture of the monster book.
 */
static mgCTexture *Tex_MBase;
/**
 * Page texture of the monster book.
 */
static mgCTexture *Tex_MBook;
/**
 * Background texture of the monster book.
 */
static mgCTexture *Tex_MBg;
/**
 * Display bit corresponding to each elemental resistance.
 */
static u32 stand_bit_5472[16] = { 1, 4, 16, 8, 2, 32, 64, 64, 64, 64, 0, 0, 0, 0, 0, 0 };
/**
 * Monster type names for each language.
 */
static char *monster_type_name[7][12] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "???", "Beast", "Windup", "Aquatic",
    "Flora", "Magical Creature", "Darkling", "Reptile Family", "Spirit", "Undead", "Card",
    "???", "???", "Animal", "Robotis[UNI00e9]", "Aquatique", "V[UNI00e9]g[UNI00e9]tal",
    "Cr[UNI00e9]ature magique", "Cr[UNI00e9]ature obscure", "Reptile", "Esprit", "Mort-vivant",
    "Carte", "???", "???", "Tier", "Aufzieh-Figur", "Wassertier", "Flora", "Zauberwesen",
    "D[UNI00fc]sterling", "Reptil", "Geist", "Untoter", "Karte", "???", "???", "Bestia",
    "Robot", "Acquatico", "Flora", "Creatura magica", "Oscuro", "Rettile", "Spirito",
    "Nonmorto", "Carta", "???", "???", "Bestia", "Broma", "Acu[UNI00e1]tico", "Flora",
    "Criatura m[UNI00e1]gica", "Misterioso", "Familia de reptiles", "Esp[UNI00ed]ritu",
    "Muerto Viviente", "Carta", "???", "???", "Beast", "Windup", "Aquatic", "Flora",
    "Magical Creature", "Darkling", "Reptile Family", "Spirit", "Undead", "Card", "???"
};
/**
 * Monster weakness names for each language.
 */
static char *monster_jyakuten[7][8] = {
    "", "", "", "", "", "", "", "", "Fl.", "Ch.", "Li.", "Cy.", "Sm.", "Ex.", "Be.", "Sc.",
    "Fe.", "Fr.", "[UNI00c9]c.", "Cy.", "Ma.", "Ex.", "An.", "[UNI00c9]c.", "Hi.",
    "K[UNI00e4].", "Bl.", "To.", "Ze.", "Ex.", "Ti.", "Sc.", "Fu.", "Gh.", "Li.", "Ci.", "Di.",
    "Es.", "Be.", "Sq.", "Ll.", "Fr.", "Ra.", "Ci.", "Ap.", "Ex.", "Co.", "De.", "Fl.", "Ch.",
    "Li.", "Cy.", "Sm.", "Ex.", "Be.", "Sc."
};
/**
 * Configuration read for menu character models.
 */
static char menu_infocfgname[] = "info.cfg";
/**
 * Character model parts loaded by the monster book.
 */
static int tbl_5848[8] = { 1, 1, 1, 1, 1, 1, 1, 0 };
/**
 * Form parts that display the current and maximum health gauges.
 */
static char *partt_2332[6] = { "hp_now0", "hp_max0", "hp_now1", "hp_max1", "hp_now2", "hp_max2" };
/**
 * Party change menu currently open.
 */
static CMenuChrCngMenu *ChrChangMenuPt;
/**
 * Cursor direction for each party selection.
 */
static u8 cursor_revtbl_2237[5] = { 1, 0, 0, 0, 0 };
/**
 * Whether the party joining sound has been played.
 */
static u8 MenuGetPartySeFlag;

static int MenuMemoryDivide(mgCMemory *memory, mgCMemory **list, int chara);

static int CheckBattleLoop();
static void EditCharaPrepare();
static void GetBajjiPosition(CMenuPosDataForm *form, int slot, int unused, int *pos);
static s32 CosutmeSelDefaultSet(s32 costume_id, s16 *costume_list);
static BASE_MONSTER_TBL *GetMonsterBaseInfoForMonsterMemoIndex(int memo_index);

/**
 * Configuration buffer used by the party change menu.
 */
static int MenuCharaChangePosDataCfgBuffer;

inline CMenuChrCngMenu::CMenuChrCngMenu() {
    change_phase = 0;
    change_chara = -1;
    change_ready = 0;
    MenuCharaChangePosDataCfgBuffer = 0;
    unk_118 = 0;
    select = 0;
    last_select = 0;
    star_fade = 0;
    enable_change = 0;
    party_member = 0;
    item_brd_select = 0;
    item_brd_pos = 0;
    open_wait = -1;
    cursor_wave = 0;
    form = NULL;
    npc_sub_form2 = NULL;
    npc_chara_form = NULL;
    npc_sub_form = NULL;
    npc_mes_form = NULL;
    chara_pos[0] = NULL;
    chara_pos[1] = NULL;
    chara_pos[2] = NULL;
    chara_pos[3] = NULL;
    chara_pos[4] = NULL;
    npc_cmd_mes[0] = 0;
    npc_cmd_mes[1] = 0;
    npc_cmd_mes[2] = 0;
    npc_cmd_mes[3] = 0;
    unk_23C = 0;
    cmd_part[0] = NULL;
    cmd_part[1] = NULL;
    cmd_part[2] = NULL;
    cmd_part[3] = NULL;
    point_gauge_part = NULL;
    set_cursor = 0;
    gauge_part[0] = NULL;
    gauge_part[1] = NULL;
    gauge_part[2] = NULL;
    gauge[0] = NULL;
    gauge[1] = NULL;
    gauge[2] = NULL;
    item_brd_arrived = 0;
    party_info = 0;
    npc_data = 0;
    mes_data = 0;
    sys_mes = NULL;
    npc_no = 0;
    sub_menu = -1;
    sub_menu_next = -1;
    face_state = -1;
    face_chara = -1;
    face_loaded = 0;
    face_img = NULL;
    npc_chara = NULL;
    npc_loading = 0;
    npc_loaded = 0;
    npc_wait = 0;
    npc_show = 0;
    npc_y = 0;
    InitStarInfo();
    key_arg_no = 0;
    close_on_end = 0;
    got_item = 0;
    gift_item = 0;
    gift_num = 0;
    memset(clut, 0, sizeof(clut));
    memset(unk_1E80, 0, sizeof(unk_1E80));
    npc_model_stack.stSetBuffer(NULL, 0);
    npc_build_stack.stSetBuffer(NULL, 0);
}

// Code (.text)
void InitMenuBGReadInfo2(MENU_BGREAD_INFO2 *info) {
    info->reading = 0;
    info->chara = NULL;
    info->name[0] = 0;
    info->path[0] = 0;
}

int MenuLoadFileCheck(MENU_BGREAD_INFO2 **info) {
    int found = 0;
    for (int i = 0; i < 7; i++) {
        if (info[i] != NULL && info[i]->reading != 0) {
            found = 1;
        }
    }
    return found;
}

void MenuBGReadInfo2Malloc(mgCMemory *stack, int *use_tbl) {
    for (int i = 0; i < 7; i++) {
        if (use_tbl[i] != 0) {
            MenuCharaBuild2[i] = (MENU_BGREAD_INFO2 *)stack->Alloc(8);
            InitMenuBGReadInfo2(MenuCharaBuild2[i]);
        } else {
            MenuCharaBuild2[i] = NULL;
        }
    }
}
s16 ConvertCharaLoadDataPhase(int chara_no, int part) {
    static short tbl[20] = { 2, 3, 4, 5, 1, 2, 3, 4, 5, 1, 1, 2, 4, 0, 0, 0, 0, 0, 0, 0 };

    return tbl[part + chara_no * 5];
}

/**
 * Tells whether the current menu allows the battle scene to keep running.
 */
static int CheckBattleLoop() {
    if (MenuCommonInfo == NULL) {
        return 1;
    }
    short menu_type = MenuCommonInfo->open_type;
    if (menu_type == 1 || menu_type == 17 || menu_type == 14 || menu_type == 21) {
        return 1;
    }
    return 0;
}

void SetMenuLoadItemNo(int chara_no) {
    int count = 0;
    CUserDataManager *user_data = GetUserDataMan();
    if (user_data == NULL) {
        return;
    }
    switch (chara_no) {
        case 0:
        case 1: {
            CHARA_DATA *chara = user_data->GetCharaDataPtr(chara_no);
            do {
                MenuLoadItemNo[count] = chara->equip[count].item_no;
                count++;
            } while (count < 5);
            break;
        }
        case 2: {
            ROBO_DATA *robo = &user_data->robo_data;
            MenuLoadItemNo[0] = robo->parts[3].item_no;
            MenuLoadItemNo[1] = robo->parts[0].item_no;
            MenuLoadItemNo[2] = robo->parts[1].item_no;
            MenuLoadItemNo[3] = 0;
            MenuLoadItemNo[4] = robo->parts[2].item_no;
            count = 5;
            break;
        }
    }
    while (count < 12) {
        MenuLoadItemNo[count] = 0;
        count++;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuMemoryDivide__FP9mgCMemoryPP9mgCMemoryi);
void MenuMemoryAdjust(mgCMemory *src, mgCMemory *rest, mgCMemory *buffer, int mode) {
    int free_blocks = src->stack_size - src->stack_used;
    mgCMemory *list[7] = { NULL };
    list[0] = buffer;
    list[1] = &buffer[1];
    list[2] = &buffer[2];
    list[3] = &buffer[3];
    list[4] = &buffer[4];
    list[5] = &buffer[5];
    list[6] = &buffer[6];
    int used = MenuMemoryDivide(src, list, mode);
    rest->stSetBuffer(buffer->stack + buffer->stack_used + used, free_blocks - used);
    if (strlen("LOAD STACK") < 16) {
        strcpy(rest->name, "LOAD STACK");
    }
    rest->stack_used = 0;
    rest->lock = 0;
}

void DeleteMonsterEffect(void) {
    if (FxScriptMan != NULL) {
        FxScriptMan->ClearEffectFromChrid(0);
        FxScriptMan->level = 2;
        FxScriptMan->ClearBaseFromLevel(2, NULL, -1);
    }
    mgTexManager.DeleteBlock(0xAA);
}

void SetMessagePositionNPCForm(CMenuPosDataForm *form, CDC2Mes *mes) {
    if (form == NULL || mes == NULL) {
        return;
    }
    int pos[10];
    int *point;
    form->GetPutPosXY("pname", pos[0], pos[1]);
    point = &pos[2];
    form->GetPutPosXY("setu0", point[0], point[1]);
    point = &pos[4];
    form->GetPutPosXY("setu1", point[0], point[1]);
    point = &pos[6];
    form->GetPutPosXY("setu2", point[0], point[1]);
    point = &pos[8];
    form->GetPutPosXY("setu3", point[0], point[1]);
    mes->SetMsgItemPos(pos, 5);
    mes->SetMovePosCenteringGyou(0, pos[0], pos[1]);
}

void AdjustNPCTalk(CDC2Mes *mes, CCharacter2 *chara) {
    ClsMes *bubble = mes;
    if (bubble == NULL || chara == NULL) {
        return;
    }
    int screen_pos[4];
    bubble->fukidashi_pos = 5;
    bubble->tail_on = 1;
    GetScrPosFromChar(chara, screen_pos);
    screen_pos[2] = 0;
    screen_pos[3] = 0xBE;
    bubble->AutoSet(screen_pos);
}

void CMenuChrCngMenu::AttachForm() {
    char name[0x20];
    int i;
    int j;

    form = MenuPosData->GetFormInfo("chr_bg");
    if (form != NULL) {
        gauge_part[0] = form->GetPartInfo("hpfill0");
        gauge_part[1] = form->GetPartInfo("hpfill1");
        gauge_part[2] = form->GetPartInfo("hpfill2");
    }
    npc_mes_form = MenuPosData->GetFormInfo("polywin");
    npc_sub_form = MenuPosData->GetFormInfo("polywin_bg");
    npc_chara_form = MenuPosData->GetFormInfo("polychr");
    npc_sub_form2 = MenuPosData->GetFormInfo("polywin_chrhide");
    npc_chara_form->SetActionCharaPtr(NULL, 0, -1);
    for (i = 0; i < 5; i++) {
        sprintf(name, "namep%d", i);
        chara_pos[i] = MenuPosData->GetEtcTbl(name);
    }
    point_gauge_part = form->GetPartInfo("stbar");
    for (j = 0; j < 4; j++) {
        sprintf(name, "cmd%d", j);
        cmd_part[j] = form->GetPartInfo(name);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", EnterDataMenu__15CMenuChrCngMenuFPUc);
void CMenuChrCngMenu::LoadNPCFaceData(mgCMemory *stack, int load_now) {
    char path[0x40];
    unsigned int size;

    stack->stack_used = 0;
    stack->lock = 0;
    face_loaded = load_now;
    face_img = NULL;
    face_chara = MenuUserDataManPtr->NowPartyCharaID();
    if (face_chara < 0) {
        face_loaded = 1;
        face_state = -1;
    } else {
        face_state = 0;
        if (face_chara <= 0) {
            face_chara = 1;
        }
        if (face_chara > 26) {
            face_chara = 26;
        }
        sprintf(path, "npcface/%d.img", face_chara);
        stack->Align64();
        face_img = (u8*)(stack->stack + stack->stack_used);
        size = LoadFileMenu(path, (u_long128 *)face_img, load_now);
        stack->Alloc((size & 0xF) ? (size >> 4) + 1 : size >> 4);
    }
}

void CMenuChrCngMenu::EnterNPCFaceData() {
    s8 load_state;

    if ((face_state == 0) && ((load_state = face_loaded, (load_state == 1)) ||
                              ((load_state == 0) && (ReadBGSync() == 0)))) {
        mgTexManager.EnterIMGFile(face_img, tex_block[0], NULL, NULL);
        face_state = 1;
        ExeScript("NPCFACEEND");
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", LoadBGNPCModel__15CMenuChrCngMenuFi);
int CMenuChrCngMenu::CheckBGNPCModel() {
    int load_result;

    load_result = 0;
    if (ReadBGSync() == 0) {
        load_result = MenuNPCLoadCheck(npc_chara, &npc_build_stack, tex_block[1]);
    }
    if (npc_chara == NULL) {
        return 0;
    }
    if (key_arg_no == 3) {
        npc_y = npc_y + ((-2.8f - npc_y) / 6.0f);
    } else {
        npc_y = npc_y + ((-16.6f - npc_y) / 6.0f);
    }
    sceVu0FVECTOR position = { 14.0f, 0.0f, 0.0f, 1.0f };
    position[1] = npc_y;
    if (load_result == 1) {
        npc_loaded = 1;
        npc_wait = 0;
        ((CCharacter2 *)npc_chara)->SetRotation(0.0f, -0.07853982f, 0.0f);
        npc_chara_form->counter = 0;
    }
    if (npc_loaded != 0) {
        ((CCharacter2 *)npc_chara)->SetScale(1.0f, 1.0f, 1.0f);
        if (npc_no == 9) {
            MenuAdjustPolygonScale((CCharacter2 *)npc_chara, 5.655f);
        } else {
            MenuAdjustPolygonScale((CCharacter2 *)npc_chara, 6.96f);
        }
        ((CCharacter2 *)npc_chara)->SetPosition(position);
        ((CCharacter2 *)npc_chara)->Step();
        if (npc_chara_form->counter >= 0xF && npc_sub_form->y < 60.0f) {
            ExeScript("CHRFADEIN");
        }
        if (npc_wait < 0x15) {
            npc_wait = npc_wait + 1;
        } else {
            npc_show = 1;
        }
        if (npc_show != 0) {
            npc_chara_form->SetActionCharaPtr(npc_chara, tex_block[1], -1);
        }
        if (key_arg_no != 3) {
            npc_show = 0;
        }
        if (-166.0f < form->y) {
            npc_show = 0;
        }
        if (select != 4) {
            npc_show = 0;
        }
        if (36.0f < npc_chara_form->y) {
            npc_show = 0;
        }
        npc_chara_form->draw_flag = npc_show != 0;
    }
    return load_result;
}

/**
 * Clears the two spare character slots before the party is changed.
 */
static void EditCharaPrepare() {
    CActionChara *chara = (CActionChara *)MenuMainScene->GetCharacter(1);
    if (chara != NULL) {
        chara->Initialize(NULL);
    }
    chara = (CActionChara *)MenuMainScene->GetCharacter(2);
    if (chara != NULL) {
        chara->Initialize(NULL);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", KeyChangeMain__15CMenuChrCngMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", CalcTex__15CMenuChrCngMenuFv);
int CMenuChrCngMenu::CheckChrChange() {
    int result = 0;
    int read_busy = ReadBGSync();
    mgCMemory *stack = &MenuCharaLoadStack;
    CActionChara *chara;

    switch (change_phase) {
        case 0:
            break;
        case 1:
            if (change_ready != 0 && read_busy == 0) {
                switch (change_chara) {
                    case 0:
                    case 1:
                        MenuItemCharaDataLoadEndCheck(MenuCharaBuild2, stack, MenuActionChara,
                                                      change_chara, -1, MenuArg.chara_tex_block);
                        break;
                    case 2:
                        break;
                }
                MenuCharaSoundLoad(stack, change_chara, 1);
                change_phase = 2;
            }
            break;
        case 2:
            if (change_ready != 0 && read_busy == 0) {
                switch (change_chara) {
                    case 0:
                    case 1:
                        MenuItemCharaDataLoadEndCheck(MenuCharaBuild2, stack, MenuActionChara,
                                                      change_chara, -1, MenuArg.chara_tex_block);
                        break;
                    case 2:
                        MenuItemRoboDataLoadEndCheck(MenuCharaBuild2, stack, MenuActionChara, -1,
                                                     MenuArg.chara_tex_block);
                        break;
                }
                chara = (CActionChara *)MenuMainScene->GetCharacter(0);
                if (GetMenuLoopType() == 1) {
                    chara->effect_man = FxScriptMan;
                }
                if (chara != NULL && MenuLoadInfo.unk_1 == 0) {
                    chara->InitScript();
                }
                MenuCharaSoundEnter(MenuMainScene, chara, 1);
                CopyActiveItemAndWeapon(change_chara, -1);
                result = 2;
                change_phase += 1;
            }
            break;
        case 3:
            result = 2;
            break;
    }
    return result;
}

int CMenuChrCngMenu::MenuLocalLoop() {
    char name[0x20];
    int cursor[2];
    int result;
    int icon_mode;
    int fade_done;
    int frame_end;
    int message_id;
    int item_no;
    CGameDataUsed *item;
    CDC2Mes *mes;
    int name_x;
    int name_y;
    int width;

    KeyChangeMain();
    int step[2] = { 0, 0 };
    if (npc_no == 1 && this->step == 0x14) {
        MenuPosData->GetPosMenuItemOnItemBrd(cursor, item_brd_select, 1);
        cursor[0] -= 8;
        cursor[1] -= 10;
        step[0] = -0x2E;
        step[1] = 0x12;
    } else {
        sprintf(name, "cur%d", select);
        form->GetPutPosXY(name, cursor[0], cursor[1]);
        MenuCursorReverseFlag = cursor_revtbl_2237[select];
    }
    MenuCommonInfo->MenuPosStep(cursor, step);
    if (set_cursor != 0) {
        MenuCommonInfo->MenuSetPos(cursor[0], cursor[1]);
        set_cursor = 0;
    }
    icon_mode = 2;
    int *mode_id = (int *)GetCommonMenuModeID();
    if (mode == 2) {
        icon_mode = 0;
    }
    MenuPosData->StepMainMenuIconMove(mode_id, 4, icon_mode);
    fade_done = 1;
    if (MenuCommonInfo->open_type == 4 || close_on_end == 1 || MenuCommonInfo->open_type == 0xE) {
        fade_done = FadeCheckMenu();
    }
    frame_end = GetMenuMainFrameEndFlag();
    result = CheckChrChange();
    switch (mode) {
        case 1:
            EnterNPCFaceData();
            if (fade_done != 0 && face_state != 0 && ReadBGSync() == 0) {
                short key_mode = MenuCommonInfo->open_type;
                if ((key_mode != 4 && frame_end != 0) || key_mode == 4) {
                    mode = 0;
                    star_fade = 0;
                    star_spawn = 1;
                    open_wait = 0;
                    set_cursor = 1;
                    ExeScript("INI0");
                    key_mode = MenuCommonInfo->open_type;
                    if (key_mode == 4) {
                        ExeScript("INIT1");
                    } else if (key_mode == 0xE) {
                        ExeScript("INIT2");
                    }
                    LoadBGNPCModel(1);
                }
            }
            break;
        case 2:
            if (fade_done != 0 && frame_end != 0) {
                DeleteTexBlock();
                MenuCommonInfo->cursor = 1;
                MenuCursorReverseFlag = 0;
                ExeScript("\217I\227\271\217\210\227\235");
                result = 1;
                if (close_on_end != 0) {
                    result = 2;
                }
            }
            break;
        default:
            if (MenuCommonInfo->open_type == 4 && MenuGetPartySeFlag == 0 && fade_done != 0) {
                MenuGetPartySeFlag = 1;
                MenuSePlay(0x11);
            }
            CheckBGNPCModel();
            break;
    }
    MenuPosData->FormStep();
    CalcTex();
    message_id = select + 0x190;
    if (select == 4) {
        message_id = npc_mes_talk;
    }
    if (select < 4) {
        if (IsCheckParty(select) == 0) {
            message_id = 0x194;
        }
        if (CheckBitFlagMenu(0x36) == 0 && select == 1 && OmakeFlag == 0) {
            message_id = 0x199;
        }
    }
    MenuDCMsg[0]->MakeMsg(message_id);
    if (npc_no == 1) {
        item_no = item_brd_select;
        if (0 <= item_no && item_no < GetNowBagMax(0)) {
            char *item_names[2] = { " ", "" };
            int item_volumes[2] = { 0, 0 };
            item = MenuDrawItemInfo[item_brd_select];
            mes = MenuDCMsg[7];
            if (item != NULL) {
                item_names[0] = item->GetName(0);
                item->GetWHp(item_volumes);
            }
            mes->SetMsgItemNo(item_names, 1);
            mes->SetMsgVolumeNo(item_volumes, 2);
            mes->value_width[0] = 6;
            name_x = (int)(MenuMesForm[7]->x + (float)(mes->abs_win.width >> 1));
            width = mes->GetStrWidth(0);
            name_x -= width / 2;
            name_y = (int)(14.0f + MenuMesForm[7]->y);
            mes->line_pos[0][0] = name_x;
            mes->line_pos[0][1] = name_y;
            mes->line_pos_on[0] = 1;
            mes->MakeMsg(0x1C3);
            mes->StepMsg();
        }
    }
    return result;
}

void CMenuChrCngMenu::InitStarInfo() {
    int i;

    star_fade = -1;
    star_spawn = 0;
    star_y = 0.0f;
    star_x = 0.0f;
    unk_260 = 0;
    star_size = 0.0f;
    star_angle = 0.0f;
    star_alpha = 0.0f;
    for (i = 0; i < 256; i++) {
        star[i].alpha = 0.0f;
    }
    star_stop_wait = 0;
    star_fade_out = 0;
    star_pulse = 0.0f;
}

void CMenuChrCngMenu::UpdataLife() {
    int i;

    i = 0;
    gauge[0] = &MenuUserParam.chara[0]->hp;
    gauge[1] = &MenuUserParam.chara[1]->hp;
    gauge[2] = &MenuUserParam.robo->hp;
    do {
        form->SetNumber(partt_2332[i * 2], GetDispVolumeForFloat(gauge[i]->now));
        form->SetNumber(partt_2332[i * 2 + 1], (int)(gauge[i]->max));
        if (gauge_part[i] != NULL && gauge[i] != NULL) {
            gauge_part[i]->w = GetDispVolumeForFloat(66.0f * gauge[i]->GetRate());
        }
        i++;
    } while (i < 3);
    if (!(party_member & 1)) {
        ExeScript("YURIS_OFFLIFE");
    }
    if (!(party_member & 2)) {
        ExeScript("MONICA_OFFLIFE");
    }
    if (!(party_member & 4)) {
        ExeScript("ROBO_OFFLIFE");
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuCharaChangeStarDraw__Fv);
#ifdef NONMATCHING
int MenuCharaChangeInit(mgCMemory *stack, int *tex_block, int mode) {
    static int tbl[8] = { 1, 1, 1, 1, 1, 1, 1, 0 };
    u8 *buffer;
    int size;
    unsigned int file_size;
    CMenuChrCngMenu *menu;
    CRepairManager *repair;
    int i;
    CCharacter2 *chara;
    short party_chara;

    buffer = (u8*)stack->stack;
    if (mode == 4 || mode == 0xE) {
        file_size = LoadFileMenu("chrchg0.pac", (u_long128 *)buffer, 1);
        stack->Alloc((file_size & 0xF) ? (file_size >> 4) + 1 : file_size >> 4);
        stack->Align64();
    }
    ChrChangeInitTextureStack.stSetBuffer((u_long128 *)buffer, stack->stack_used);
    size = stack->stGetRest();
    MenuChangeMemory.stSetBuffer(stack->stGetTop(), size);
    MenuChangeMemory.Alloc(0x100);
    menu = new (MenuChangeMemory.Alloc(0x1FA)) CMenuChrCngMenu;
    ChrChangMenuPt = menu;
    menu->SetTexBlock(tex_block);
    party_chara = MenuUserDataManPtr->active_chr_no;
    ChrChangMenuPt->last_select = party_chara;
    ChrChangMenuPt->select = party_chara;
    repair = new (MenuChangeMemory.Alloc(0x21)) CRepairManager;
    MenuRepairMan = repair;
    repair->Initialize();
    if (mode == 4) {
        ChrChangMenuPt->key_arg_no = 2;
        ChrChangMenuPt->last_select = 4;
        ChrChangMenuPt->select = 4;
    }
    for (i = 0; i < MENU_CHARA_LOAD_MAX; i++) {
        chara = MenuMainScene->GetCharacter(i);
        MenuActionChara[i] = (CActionChara *)chara;
    }
    MenuBGReadInfo2Malloc(&MenuChangeMemory, tbl);
    MenuCharaChangeCLUT_Tex = 0;
    MenuMainFrameModeSet(4, 1);
    ChrChangMenuPt->EnterDataMenu((u8*)stack->stack);
    MenuChangeMemory.Align64();
    size = MenuChangeMemory.stGetRest();
    MenuChangeNpcMemory.stSetBuffer(MenuChangeMemory.stGetTop(), size);
    ChrChangMenuPt->LoadNPCFaceData(&MenuChangeNpcMemory, 0);
    MenuChangeNpcMemory.Align64();
    size = MenuChangeNpcMemory.stGetRest();
    MenuCharaLoadStack.stSetBuffer(MenuChangeNpcMemory.stGetTop(), size);
    MenuLoadInfo.unk_1 = 0;
    MenuCharaLoadStack.stack_used = 0;
    MenuCharaLoadStack.lock = 0;
    switch (mode) {
        case 0:
            MenuLoadInfo.unk_1 = 1;
            break;
    }
    MenuLoadInfo.mode = 2;
    if (ChrChangMenuPt->key_arg_no == 2 || mode == 0xE) {
        while (GetMenuMainFrameEndFlag() == 0) {
            MenuMainFrameStep();
        }
        ChrChangMenuPt->FadeOutMenu(0x1E, 0.0f);
        ChrChangMenuPt->ExeScript("JOININIT");
    } else {
        MenuMainScene->fade.FadeIn(1);
        MenuMainScene->fade.FadeStep();
    }
    MenuCommonInfo->key_enable = 0;
    MenuCommonInfo->cursor = 0;
    MenuGetPartySeFlag = 0;
    MenuDCMsg[0]->MsgPreset(3);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuCharaChangeInit__FP9mgCMemoryPii);
#endif
int MenuCharaChangeKey(void) {
    int result;
    int fade_done;
    int box_result;
    int phase;
    int mode;
    int i;
    mgCMemory *stack;
    u8 *texture;
    CMenuPosDataForm *form;
    CMenuPosDataForm *cursor_form;

    int pos[2];

    result = 0;
    fade_done = ChrChangMenuPt->FadeCheckMenu();
    mode = 1;
    phase = ChrChangMenuPt->sub_menu;
    switch (phase) {
        case -1:
            if (ChrChangMenuPt->sub_menu_next == 1) {
                if (fade_done != 0) {
                    ChrChangMenuPt->sub_menu = ChrChangMenuPt->sub_menu_next;
                    MenuCharaLoadStack.stack_used = 0;
                    MenuCharaLoadStack.lock = 0;
                    if (MenuLoadInfo.unk_1 == 1) {
                        mode = 0;
                    }
                    MenuMonsterBoxInit(&MenuCharaLoadStack, ChrChangMenuPt->tex_block, mode);
                }
            } else if (ChrChangMenuPt->sub_menu_next == -1 && fade_done != 0) {
                result = ChrChangMenuPt->MenuLocalLoop();
            }
            break;
        case 1:
            if (ChrChangMenuPt->sub_menu_next == -1) {
                if (fade_done != 0) {
                    ChrChangMenuPt->sub_menu = ChrChangMenuPt->sub_menu_next;
                }
            } else if (phase == 1) {
                box_result = MenuMonsterBoxKey();
                if (box_result != 0) {
                    if (box_result == 1) {
                        ChrChangMenuPt->sub_menu_next = -1;
                        ChrChangMenuPt->sub_menu = -1;
                        MenuCharaLoadStack.stack_used = 0;
                        MenuCharaLoadStack.lock = 0;
                        texture = (u8*)ChrChangeInitTextureStack.stack;
                        char file_name[0x40] = "chrchg0.pac";
                        LoadFileMenu(file_name, (u_long128 *)texture, 1);
                        ChrChangMenuPt->EnterDataMenu(texture);
                        ChrChangMenuPt->LoadNPCFaceData(&MenuChangeNpcMemory, 1);
                        ChrChangMenuPt->EnterNPCFaceData();
                        ChrChangMenuPt->LoadBGNPCModel(0);
                        ChrChangMenuPt->CheckBGNPCModel();
                        MenuDCMsg[0]->SetBuff(ChrChangMenuPt->sys_mes);
                        MenuDCMsg[0]->MakeMsg(ChrChangMenuPt->select + 0x190);
                        form = MenuMesForm[7];
                        form->rgba[0] = 0x80;
                        form->rgba[1] = 0x80;
                        form->rgba[2] = 0x80;
                        form->rgba[3] = 0;
                        i = 0;
                        do {
                            form->SetRGBACalcParam(i, 0, 0x80);
                            i++;
                        } while (i < 4);
                        ChrChangMenuPt->InitStarInfo();
                        ChrChangMenuPt->star_fade = 0;
                        ChrChangMenuPt->set_cursor = 1;
                        ChrChangMenuPt->form->GetPutPosXY("cur3", pos[0], pos[1]);
                        MenuCommonInfo->MenuSetPos(pos[0], pos[1]);
                        ChrChangMenuPt->form->GetPutPosXY("mi2", pos[0], pos[1]);
                        cursor_form = MenuFormMI2;
                        cursor_form->x = (float)pos[0];
                        cursor_form->y = (float)pos[1];
                        ChrChangMenuPt->FadeInMenu(0x28, 0.0f);
                        MenuCamInit(1.0f);
                    } else if (box_result == 2) {
                        SetupUnitMan(MenuMainScene, MenuUserDataManPtr, 3, NULL);
                        result = 2;
                        MenuArg.result[0] = 3;
                    }
                }
            }
            break;
    }
    return result;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuCharaChangeDraw__Fv);
char *GetMonsterName(int monster_no) {
    BASE_MONSTER_TBL *record = GetMonsterTable(monster_no);
    if (record != NULL) {
        return record->name;
    }
    return NULL;
}

int get_gajji_id_from_monster_progress_table(int monster_no, int *level) {
    int row;
    int column;

    for (row = 0; row < 19; row++) {
        for (column = 1; column < 5; column++) {
            if (monster_no == monster_progress_tbl[row][column]) {
                if (level) {
                    *level = column - 1;
                }
                return monster_progress_tbl[row][0];
            }
        }
    }
    return -1;
}

int GetMonsterProgressTableNo(int level, int monster_no) {
    int row = 0;
    do {
        if (monster_no == monster_progress_tbl[row][level + 1]) {
            return row;
        }
        row++;
    } while (row < 19);
    return -1;
}

int get_monster_tbl_bajjilevel(int *out, int bajji_no, int monster_no, int level) {
    int row;
    int count;
    int entry;
    int j;

    if (level < 0 || level > 3) {
        return 0;
    }
    count = 0;
    row = 0;
    for (; row < 19; row++) {
        if (monster_no < 0 ||
            (0 <= monster_no && level > 0 && monster_no == monster_progress_tbl[row][level])) {
            if (bajji_no == monster_progress_tbl[row][0]) {
                out[count] = monster_progress_tbl[row][level + 1];
                count++;
            }
        }
    }
    row = 0;
    for (; row < count; row++) {
        entry = out[row];
        for (j = row + 1; j < count; j++) {
            if (entry == out[j]) {
                local_sort1(row, &count, out);
            }
        }
    }
    return count;
}

int get_default_monster_progresstbl(int bajji_no) {
    int row;
    for (row = 0; row < 19; row++) {
        if (bajji_no == monster_progress_tbl[row][0]) {
            return row;
        }
    }
    return 0;
}

int GetMonsterModelFile(int monster_no, int kind, char *path) {
    char suffix[0x20];
    BASE_MONSTER_TBL *monster;
    char *base_name;
    int number;
    MOS_HENGE_PARAM *henge_param;

    if (path == NULL) {
        return 0;
    }
    monster = GetMonsterTable(monster_no);
    base_name = monster->model;
    if (monster == NULL) {
        return 0;
    }
    if ((int)strlen(base_name) <= 0) {
        return 0;
    }
    strcpy(path, base_name);
    if (kind == 0) {
        strcat(path, ".chr");
    }
    if (kind == 1) {
        number = monster->sound_no;
        if (number < 0) {
            return 0;
        }
        strcpy(path, "snd2/mon/");
        if (number < 10) {
            sprintf(suffix, "EN_00%d.snd", number);
        } else if (number < 100) {
            sprintf(suffix, "EN_0%d.snd", number);
        } else {
            sprintf(suffix, "EN_%d.snd", number);
        }
        strcat(path, suffix);
    }
    if (kind == 2) {
        henge_param = GetMonsterHengeParam(monster_no);
        if (henge_param != NULL) {
            sprintf(path, "%s.stb", henge_param->sound_bank);
        }
    }
    if (kind == 3) {
        strcpy(path, monster->model);
        strcat(path, ".cfg");
    }
    return 1;
}

void CMenuMosSelect::AttachForm() {
    char name[0x20];
    MOS_CHANGE_PARAM *base;
    int i;

    badge_form = MenuPosData->GetFormInfo("\203\202\203\223\203X\203^\201[\203o\203b\203W\224\240");
    model_form = MenuPosData->GetFormInfo("\203\202\203\223\203X\203^\201[\203|\203\212");
    info_form = MenuPosData->GetFormInfo("\203\202\203\223\203X\203^\201[\203X\203e\201[\203^\203X");
    Tex_BuildUpBoard = mgTexManager.GetTexture("itembrd", -1);
    GetUserDataMan();
    if (badge_form != NULL) {
        base = badge;
        i = 0;
        do {
            MENUFORMPARTS_TYPE *part;
            sprintf(name, "B%d", i);
            part = badge_form->GetPartInfo(name);
            if (part != NULL) {
                part->draw_flag = base[i].enable != 0;
            }
            i += 1;
        } while (i < 0xC);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MonsterScaleCheck__FP11CCharacter2);
int MonsterEffectRead(mgCMemory *stack, int monster_no, int background) {
    char script_path[0x80];
    char pack_path[0x80];
    int i;

    mos_effect_henge_param = GetMonsterHengeParam(monster_no);
    mos_effect_read_num = 0;
    if (mos_effect_henge_param != NULL && stack != NULL) {
        if (FxScriptMan != NULL) {
            for (i = 0; i < 4; i++) {
                char *effect_name = mos_effect_henge_param->effect_name[i];
                if (effect_name == NULL) {
                    continue;
                }
                FxScriptMan->GetNeedFilePath(effect_name, script_path, pack_path);
                stack->Align64();
                mos_effect_readbuff1[i] = (u8 *)stack->stGetTop();
                if (background != 0) {
                    if (LoadFileBG(script_path, (u_long128 *)mos_effect_readbuff1[i],
                                   &mos_effect_readbuff1_size[i]) == 0) {
                        continue;
                    }
                } else if (LoadFile2(script_path, mos_effect_readbuff1[i],
                                     &mos_effect_readbuff1_size[i], 0) == 0) {
                    continue;
                }
                unsigned int total = mos_effect_readbuff1_size[i] + 0x800;
                stack->Alloc((total & 0xF) ? (total >> 4) + 1 : total >> 4);
                stack->Align64();
                mos_effect_readbuff2[i] = (u8 *)stack->stGetTop();
                if (background != 0) {
                    if (LoadFileBG(pack_path, (u_long128 *)mos_effect_readbuff2[i],
                                   &mos_effect_readbuff2_size[i]) == 0) {
                        continue;
                    }
                } else if (LoadFile2(pack_path, mos_effect_readbuff2[i],
                                     &mos_effect_readbuff2_size[i], 0) == 0) {
                    continue;
                }
                total = mos_effect_readbuff2_size[i] + 0x800;
                stack->Alloc((total & 0xF) ? (total >> 4) + 1 : total >> 4);
                mos_effect_read_num += 1;
            }
        }
    }
    return mos_effect_read_num;
}

int MonsterEffectEnter(CScene *scene, u_long128 *buffer, int tex_block) {
    int i;
    u_long128 *saved_buffer;

    if (mos_effect_henge_param != NULL && 0 < mos_effect_read_num && FxScriptMan != NULL) {
        mgTexManager.DeleteBlock(tex_block);
        saved_buffer = scene->read_buff;
        i = 0;
        FxScriptMan->load_buffer = (u_long128*)buffer;
        for (; i < mos_effect_read_num; i++) {
            char *effect_name = mos_effect_henge_param->effect_name[i];
            if (effect_name != NULL) {
                FxScriptMan->BuildBase(effect_name, (u_long128 *)mos_effect_readbuff1[i],
                                       mos_effect_readbuff1_size[i],
                                       (u_long128 *)mos_effect_readbuff2[i],
                                       mos_effect_readbuff2_size[i], MorattaStack, tex_block);
            }
        }
        FxScriptMan->load_buffer = saved_buffer;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", CheckLoadBGMonster__14CMenuMosSelectFv);
/**
 * Finds the position of a monster badge on its form.
 */
static void GetBajjiPosition(CMenuPosDataForm *form, int slot, int unused, int *pos) {
    char name[0x20];
    if (form != NULL) {
        sprintf(name, "B%d", slot);
        form->GetPutPosXY(name, pos[0], pos[1]);
    }
}

void CMenuMosSelect::CalcCursorPosition() {
    int pos[2];
    if (BuildUpWeaponInfo.mode == 1) {
        pos[0] = BuildUpNameXY[BuildUpWeaponInfo.select_no][0] - 0x14;
        pos[1] = BuildUpNameXY[BuildUpWeaponInfo.select_no][1];
    } else {
        GetBajjiPosition(badge_form, select, top, pos);
        pos[0] -= 0x1E;
        pos[1] += 0xE;
    }
    MenuCommonInfo->MenuPosStep(pos, NULL);
    if (set_cursor != 0) {
        MenuCommonInfo->MenuSetPos(pos[0], pos[1]);
        set_cursor = 0;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", CalcTex__14CMenuMosSelectFv);
int CMenuMosSelect::KeyNormalMode(int select_key, int lr_key, int push_button) {
    static int overcode[4] = { 0, 0, 0, 0 };

    int old_cursor = select;
    MenuGlidKeyCheck(select_key, &select, &top, &max_3170, &viewnum_3171, overcode, 0xC);
    if (old_cursor != select) {
        MenuLoadInfo.unk_6[1] = 0;
        view_monster = -1;
        if (select < 0xA) {
            MOS_CHANGE_PARAM *entry = badge + select;
            if (entry != NULL) {
                if (entry->enable != 0) {
                    view_monster = entry->monster_id;
                }
            }
        }
        MenuSePlay(0);
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuMonsterBoxInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", KeyStep__14CMenuMosSelectFv);
int MenuMonsterBoxKey(void) {
    return MenuMosSelectPtr->KeyStep();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuMonsterBoxDraw__Fv);
void MenuTimeStepEnvFunc(CScene *scene, CActionChara *chara, int item_no) {
    mgCFrame *sun;
    mgCFrame *moon;
    if (scene == NULL || chara == NULL) {
        return;
    }
    if (item_no != 0x38) {
        return;
    }
    sun = chara->SearchObject("w15a");
    moon = chara->SearchObject("w15b");
    if (sun == NULL || moon == NULL) {
        return;
    }
    if (GetTimeBand(scene->time) != MAP_TIME_BAND_NIGHT) {
        sun->SetAttrParamDraw(1, 0);
        moon->SetAttrParamDraw(0, 0);
    } else {
        sun->SetAttrParamDraw(0, 0);
        moon->SetAttrParamDraw(1, 0);
    }
}

void MenuWeaponRealStepEnvFunc(CActionChara *chara, int item_no) {
    mgCFrame *object;
    float rotation[4];
    if (item_no == 0x58) {
        object = chara->SearchObject("parts01");
        if (object != NULL) {
            object->GetRotation(rotation);
            rotation[1] += 0.13962634f;
            rotation[1] = mgAngleLimit(rotation[1]);
            object->SetRotation(rotation);
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemCharaDataLoad__FP9mgCMemoryiPP17MENU_BGREAD_INFO2i);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemCharaDataLoadPack__FiP12CActionCharaP12CActionCharaiPUiP9mgCMemoryii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemCharaDataLoadEndCheck__FPP17MENU_BGREAD_INFO2P9mgCMemoryPP12CActionCharaiii);
u32 MenuCharaSoundLoad(mgCMemory *stack, int chara_no, int background) {
    char path[0x60];
    int size;
    unsigned int blocks;
    CharaSndBuffer = NULL;
    MenuSoundCharaNo = chara_no;
    size = 0;
    stack->Align64();
    CharaSndBuffer = (u32*)(stack->stack + stack->stack_used);
    if (chara_no < 3) {
        GetCharacterSnd(GetUserDataMan(), chara_no, path);
    } else {
        GetMonsterModelFile(GetUserDataMan()->monster_id, 1, path);
    }
    if (background != 0) {
        LoadFileBG(path, (u_long128 *)CharaSndBuffer, &size);
    } else {
        LoadFile2(path, CharaSndBuffer, &size, 0);
    }
    blocks = (size & 0xF) ? ((unsigned int)size >> 4) + 1 : (unsigned int)size >> 4;
    stack->Alloc(blocks);
    return size;
}

void MenuCharaSoundEnter(CScene *scene, CActionChara *chara, int init_port) {
    if (scene != NULL && chara != NULL) {
        chara->foot_se_bank = scene->se_base_id;
        chara->foot_sound_id = -1;
        if (init_port != 0) {
            sndInitPort(7);
        }
        u32 *buffer = CharaSndBuffer;
        if (buffer != NULL) {
            int index;
            signed char ids[4] = { 3, 3, 1, 0 };
            index = MenuSoundCharaNo;
            if (index < 0) {
                index = 0;
            }
            chara->se_bank =
                sndLoadSound(7, buffer, MorattaStack + ids[index]);
        }
        chara->se_bank_2 = scene->se_battle_id;
        chara->SetSoundInfoCopy();
    }
}

u32 MenuItemChrLoad(mgCMemory *stack, int item_no, int kind, MENU_BGREAD_INFO2 *info,
                    int restart_read) {
    unsigned int size;
    unsigned int blocks;
    if (restart_read != 0) {
        BreakReadBG();
        StartReadBG();
    }
    GetGameDataPt();
    info->reading = 1;
    info->chara = NULL;
    strcpy(info->name, GetItemFileName(item_no, 0));
    if (kind == 1) {
        strcat(info->name, "_item.chr");
    }
    strcpy(info->path, GetItemFilePath(item_no, 1));
    size = 0;
    stack->Align64();
    if (LoadFileBG((char *)&info->path, (u_long128 *)(stack->stack + stack->stack_used),
                   (int *)&size) == 0) {
        info->reading = 0;
    } else {
        blocks = (size & 0xF) ? (size >> 4) + 1 : size >> 4;
        stack->Alloc(blocks);
        stack->Align64();
    }
    return size;
}

int MenuItemChrLoadEndCheck(MENU_BGREAD_INFO2 *info, CActionChara *chara, mgCMemory *stack,
                            int tex_block) {
    if (info->reading != 0) {
        BG_READ_INFO *loaded = GetReadBGInfo(info->path);
        mgCTextureManager *tex_manager = &mgTexManager;
        u_long128 *model_buffer;
        tex_manager->DeleteBlock(tex_block);
        model_buffer = loaded->buffer;
        stack->stack_used = 0;
        stack->lock = 0;
        info->chara = chara;
        if (chara != NULL) {
            strcpy(tex_manager->name_suffix, "_menu");
            chara->Initialize(NULL);
            chara->LoadPack((u_int *)model_buffer, menu_infocfgname, stack, stack, stack, tex_block,
                             0);
            tex_manager->name_suffix[0] = 0;
        }
        info->reading = 0;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemRoboDataLoad__FP9mgCMemoryPP17MENU_BGREAD_INFO2i);
void DeleteOutLineMenu(CActionChara *chara, int sub) {
    char name[0x20];
    if (chara != NULL) {
        mgCTextureManager *tex_manager = &mgTexManager;
        sprintf(name, "out_line2%d", chara->outline_tex_no);
        if (sub != 0) {
            strcat(name, "_menu");
        }
        tex_manager->DeleteTexture(name, -1);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemRoboDataLoadEndCheck__FPP17MENU_BGREAD_INFO2P9mgCMemoryPP12CActionCharaii);
void MenuRoboPartsLightOff(mgCFrame *frame) {
    mgCFrame *part;

    if (frame != NULL) {
        part = frame->SearchFrame("light");
        if (part != NULL) {
            part->attr->draw = 0;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuMonsterLoadBG__FP9mgCMemoryPP17MENU_BGREAD_INFO2ii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuMonsterLoadBGCheck__FPP17MENU_BGREAD_INFO2PP12CActionCharaii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuItemCharaDataLoadEndCheckAfter__FPP17MENU_BGREAD_INFO2i);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", InitMainCharaBG__FiP9mgCMemoryi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", ReadMainCharaBG__Fv);
int KeyMainCharaBG(void) {
    int read_state;

    read_state = ReadMainCharaBG();
    if (MenuLoadInfo.unk_1 == 1) {
        NowMainCharaChngTexMoveX = NowMainCharaChngTexMoveX + 0x12;
    } else {
        NowMainCharaChngTexMoveX += 0xC;
        if (NowReadMainCharaNo >= 2) {
            NowMainCharaChngTexMoveX += 8;
        }
        if (NowMainCharaChngTexMovePhase > 0) {
            NowMainCharaChngTexMoveX = NowMainCharaChngTexMoveX + 0xC;
        }
    }
    if (NowMainCharaChngTexMoveX > 0x208) {
        NowMainCharaChngTexMoveX = 0x208;
    }
    switch (NowMainCharaChngTexMovePhase) {
        case 0:
            if (NowMainCharaChngTexMoveX > 0x80) {
                NowMainCharaChngTexMoveX = 0x80;
            }
            break;
        default:
            break;
    }
    if (read_state == 2) {
        if (mgScreenWidth <= NowMainCharaChngTexMoveX) {
            NowMainCharaChngTex = NULL;
            NowReadMainCharaNo = -1;
            return 1;
        }
    }
    return 0;
}

void DrawMainCharaBG(void) {
    mgRect<int> frame_rect;
    mgRect<int> slide_rect;
    int loaded_tex;

    loaded_tex = -1;
    if (NowMainCharaFrameImage != NULL) {
        MenuReloadTexture(loaded_tex, NowMainCharaFrameImage->block);
        frame_rect.Set(0, 0, 0x200, mgScreenHeight);
        PrimQuad(NowMainCharaFrameImage, 0.0f, 0.0f, frame_rect, 0x80, 0x80, 0x80, 0x80);
    }
    if (!(NowMainCharaChngStatusBit & 2)) {
        MenuReloadTexture(loaded_tex, MenuArg.mes_tex_block);
        MenuDCMsg[0]->StepMsg();
        MenuDCMsg[0]->DrawMsg();
        return;
    }
    if (NowMainCharaChngTex != NULL) {
        MenuReloadTexture(loaded_tex, NowMainCharaChngTex->block);
        slide_rect.Set(0, NowReadMainCharaNo << 6, 0x100, 0x40);
        PrimQuad(NowMainCharaChngTex, (float)NowMainCharaChngTexMoveX, 180.0f, slide_rect, 0x80, 0x80,
                 0x80, 0x80);
    }
}

int MenuNPCModelLoad(mgCMemory *stack, int chara_no, int background) {
    int size;
    u8 *buffer;
    char *name;

    MenuNPCLoadFlag = 0;
    stack->Align64();
    name = GetPartyCharaModelName(chara_no, 3);
    buffer = (u8 *)stack->stGetTop();
    MenuPartyNPCModelReadBuffer = buffer;
    if (name == NULL) {
        return 0;
    }
    if (background != 0) {
        LoadFileBG(name, (u_long128 *)buffer, &size);
    } else {
        LoadFile2(name, buffer, &size, 0);
    }
    if (size > 0) {
        MenuNPCLoadFlag = 1;
    }
    stack->Alloc(blocks_for(size));
    return MenuNPCLoadFlag;
}

#ifdef NONMATCHING
int MenuNPCLoadCheck(CActionChara *chara, mgCMemory *stack, int tex_block) {
    mgCTextureManager *tex_manager = &mgTexManager;

    if (MenuNPCLoadFlag == 1) {
        if (chara != NULL) {
            stack->stack_used = 0;
            stack->lock = 0;
            tex_manager->DeleteBlock(tex_block);
            strcpy(tex_manager->name_suffix, "_mn");
            chara->Initialize(0);
            chara->LoadPack((u_int*)MenuPartyNPCModelReadBuffer, menu_infocfgname, stack, stack, stack,
                             tex_block, 0);
            tex_manager->name_suffix[0] = 0;
            MenuNPCLoadFlag = 0;
            return 1;
        }
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuNPCLoadCheck__FP12CActionCharaP9mgCMemoryi);
#endif
void CMenuCostumeSel::UpdateCostumeList(int chara_no, unsigned long attr) {
    CHARA_DATA *chara_data;
    int kind;
    int index;

    chara_data = GetUserDataMan()->GetCharaDataPtr(0);
    if (chara_no == 0) {
        this->costume_num[0] = GetCostumeList(attr, 6, this->costume_list[1]);
        this->costume_num[1] = GetCostumeList(attr, 5, this->costume_list[0]);
        this->costume_num[2] = GetCostumeList(attr, 7, this->costume_list[2]);
    }
    if (chara_no == 1) {
        chara_data = GetUserDataMan()->GetCharaDataPtr(1);
        this->costume_num[0] = GetCostumeList(attr, 9, this->costume_list[1]);
        this->costume_num[1] = GetCostumeList(attr, 8, this->costume_list[0]);
        this->costume_num[2] = GetCostumeList(attr, 10, this->costume_list[2]);
    }
    if (chara_data == NULL) {
        return;
    }
    int worn[4] = { 0, 0, 0, -1 };
    worn[0] = chara_data->equip[2].item_no;
    worn[1] = chara_data->equip[4].item_no;
    worn[2] = chara_data->equip[3].item_no;
    for (kind = 0; kind < 3; kind++) {
        this->costume_select[kind] = 0;
        for (index = 0; index < this->costume_num[kind]; index++) {
            if (worn[kind] == this->list[kind][index]) {
                this->costume_select[kind] = index;
            }
        }
    }
}

/**
 * Finds a costume in the first five choices, or selects the first choice.
 */
static s32 CosutmeSelDefaultSet(s32 costume_id, s16 *costume_list) {
    for (int index = 0; index < 5; index++) {
        if (costume_id == costume_list[index]) {
            return index;
        }
    }
    return 0;
}

#ifdef NONMATCHING
void CMenuCostumeSel::LoadMenuData(mgCMemory *stack, int *tex_block) {
    static int tbl[8] = { 1, 1, 0, 0, 1, 1, 1, 0 };
    int i;
    mgCTextureManager *tex_manager;
    u8 *buffer;
    unsigned int size;
    u8 *icons;
    short *system_mes;
    int free_size;

    this->SetTexBlock(tex_block);
    for (i = 0; i < 7; i++) {
        MenuActionChara[i] = new (stack->Alloc(0x105)) CActionChara;
        MenuActionChara[i]->Initialize(0);
    }
    tex_manager = &mgTexManager;
    buffer = (u8 *)stack->stGetTop();
    size = LoadFileMenu("fukusel.img", (u_long128 *)buffer, 1);
    stack->Alloc((int)size / 16 + 0x10);
    stack->Align64();
    mgTexManager.EnterIMGFile(buffer, *tex_block, NULL, NULL);
    this->tile_tex = mgTexManager.GetTexture("fukusen", -1);
    icons = (u8*)GetMenuMainIMGPtr();
    if (icons != NULL) {
        tex_manager->EnterIMGFile(icons, *tex_block, NULL, NULL);
    }
    this->cursor_tex = tex_manager->GetTexture("mnmain", -1);
    this->cursor_x = 0;
    this->cursor_y = 0;
    this->cursor_wave = 0;
    this->unk_2BC = 0;
    AttachMessageForm();
    system_mes = GetSystemMesBuffer();
    MenuDCMsg[0]->SetMessData(system_mes, GetMenuMainMessageBuffer());
    MenuDCMsg[0]->MsgPreset(0xA);
    MenuDCMsg[0]->SetAbsPos(8);
    system_mes = GetSystemMesBuffer();
    MenuDCMsg[7]->SetMessData(system_mes, GetMenuMainMessageBuffer());
    MenuDCMsg[7]->MsgPreset(0xB);
    MenuDCMsg[7]->SetAbsPos(8);
    MenuDrawEnv->speed = 2.0f;
    MenuBGReadInfo2Malloc(stack, tbl);
    MenuLoadInfo.mode = 3;
    MenuLoadInfo.unk_1 = 1;
    MenuLoadInfo.unk_2 = 1;
    MenuLoadInfo.unk_5 = 0;
    MenuLoadInfo.unk_4 = -1;
    MenuLoadInfo.unk_3 = 0;
    MenuLoadInfo.unk_6[1] = 0;
    MenuLoadInfo.unk_6[0] = 1;
    free_size = stack->stGetRest();
    this->stack.stSetBuffer(stack->stGetTop(), free_size);
    MenuMemoryAdjust(&this->stack, &MenuCharaLoadStack, MenuActionCharaBuffer, 0);
    SetMenuLoadItemNo(0);
    MenuItemCharaDataLoad(&MenuCharaLoadStack, 0, MenuCharaBuild2, 1);
    do {
    } while (ReadBGSync() == 0);
    this->load_wait = 0;
    MenuCosutumeLoadPhase = 2;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", LoadMenuData__15CMenuCostumeSelFP9mgCMemoryPi);
#endif
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", KeyStep__15CMenuCostumeSelFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", Draw__15CMenuCostumeSelFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MenuCostumeInit__FP9mgCMemoryPii);
int MenuCostumeKey() {
    return MenuCosPtr->KeyStep();
}

void MenuCostumeDraw(void) {
    MenuCosPtr->Draw();
}

/**
 * Finds the monster recorded at an index in the monster book.
 */
static BASE_MONSTER_TBL *GetMonsterBaseInfoForMonsterMemoIndex(int memoIndex) {
    BASE_MONSTER_TBL *monster;
    int i;

    monster = GetMonsterBaseInfo(0);
    for (i = 0; i < 0x14A; i++) {
        if (monster->unk_b2 == memoIndex) {
            return monster;
        }
        monster++;
    }
    return NULL;
}

void CMosBookMenu::InitMonsterInfo(void) {
    area_name[0] = 0;
    name[0] = 0;
    type_name[0] = 0;
    hp = 0;
    abs = 0;
    kill_num = 0;
    strong_bit = 0;
    weak_bit = 0;
    drop_item[0][0] = 0;
    drop_item[1][0] = 0;
    drop_item[2][0] = 0;
    weak_name[0] = 0;
}

void CMosBookMenu::SetMonsterInfo(BASE_MONSTER_TBL *info) {
    char *area;
    char **type_names;
    char *message;
    int i;
    int item_count;
    int j;
    int k;
    int weak_count;
    int n;

    this->InitMonsterInfo();
    if (info != NULL) {
        strcpy(this->name, info->name);
        area = NULL;
        if (0 <= info->unk_b0) {
            area = GetMapTitle(GetDngMapNo(info->unk_b0));
        }

        char local_area_names[2][32] = { "Rainbow Butterfly Wd.", "Bois Pap. arc-en-ciel" };
        if (LanguageCode == 1) {
            if (info->unk_b0 == 1) {
                area = local_area_names[0];
            }
        }
        if (LanguageCode == 2 && info->unk_b0 == 1) {
            area = local_area_names[1];
        }
        if (area != NULL) {
            strcpy(this->area_name, area);
        }
        type_names = monster_type_name[LanguageCode];
        if (type_names[0] != NULL) {
            strcpy(this->type_name, type_names[info->user_mons_id]);
        }
        this->hp = info->unk_56;
        this->abs = info->unk_58;
        this->kill_num = KillMonsterCount(info->id, 0);
        item_count = 0;
        for (i = 0; i < 3; i++) {
            if (0 < info->drop_items[i]) {
                message = GetItemMessage(info->drop_items[i]);
                if (message != NULL) {
                    strcpy(this->drop_item[item_count], message);
                    item_count++;
                }
            }
        }
        for (j = 0; j < 12; j++) {
            if (info->ext_param[j] >= 101) {
                this->strong_bit = this->strong_bit | stand_bit_5472[j];
            }
            if (info->ext_param[j] < 51) {
                this->weak_bit = this->weak_bit | stand_bit_5472[j];
            }
        }
        weak_count = 0;
        int weak_list[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        for (k = 0; k < 8; k++) {
            if (info->unk_6c[k] >= 50) {
                weak_list[weak_count] = k;
                weak_count++;
            }
        }
        if (weak_count > 0) {
            if (monster_jyakuten[LanguageCode][0] != NULL) {
                strcpy(this->weak_name, monster_jyakuten[LanguageCode][weak_list[0]]);
                for (n = 1; n < weak_count; n++) {
                    strcat(this->weak_name, monster_jyakuten[LanguageCode][weak_list[n]]);
                }
            }
        }
    }
}

void CMosBookMenu::InitEnd(void) {
    u8 *buffer;
    unsigned int size;
    int free_size;
    int i;
    BASE_MONSTER_TBL *entry;

    buffer = (u8 *)MosBookStack.stGetTop();
    size = LoadFileMenu("memomos.pac", (u_long128 *)buffer, 1);
    MosBookStack.Alloc(blocks_for(size));
    mgTexManager.EnterIMGFile((u8*)GetPackFile((unsigned int *)buffer, "out.img", NULL),
                              this->tex_block[0], NULL, NULL);
    Tex_MBase = mgTexManager.GetTexture("mosbox", -1);
    Tex_MBook = mgTexManager.GetTexture("memomos", -1);
    Tex_MBg = mgTexManager.GetTexture("mosbg", -1);
    this->tex_block_no = this->tex_block[4];
    mgCMemory scratch;
    free_size = MosBookStack.stGetRest();
    scratch.stSetBuffer(MosBookStack.stGetTop(), free_size);
    MenuMemoryAdjust(&scratch, &this->stack, MenuActionCharaBuffer, 3);
    this->list_num = 0;
    for (i = 0; i < MOS_BOOK_MEMO_NUM; i++) {
        entry = GetMonsterBaseInfoForMonsterMemoIndex(i);
        if (entry != NULL && 0 < KillMonsterCount(entry->id, 0)) {
            this->list[this->list_num] = entry->id;
            this->list_num++;
        }
    }
    for (i = this->list_num; i < MOS_BOOK_MEMO_NUM; i++) {
        this->list[i] = -1;
    }
    this->monster_info = GetMonsterBaseInfo(this->list[this->select]);
    this->SetMonsterInfo(this->monster_info);
    this->FadeInMenu(0x32, 0.0f);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", Draw__12CMosBookMenuFv);
#ifdef NONMATCHING
int CMosBookMenu::KeyStep(void) {
    int select;
    int lr;
    int push;
    int fade;
    int cmd;
    int i;
    float scroll;
    float half;

    select = MenuCommonInfo->CheckSelectKey();
    lr = MenuCommonInfo->CheckLRKey();
    push = MenuCommonInfo->CheckPushButton();
    fade = this->FadeCheckMenu();
    cmd = -1;
    switch (this->mode) {
        case MOS_BOOK_MODE_FADING_IN:
            if (fade != 0) {
                this->mode = MOS_BOOK_MODE_BROWSING;
                MenuCommonInfo->key_enable = 1;
                this->load_phase = 1;
            }
            break;
        case MOS_BOOK_MODE_FADING_OUT:
            if (fade != 0) {
                this->DeleteTexBlock();
                if (MonsterBookBootMode == 1) {
                    return 2;
                }
                return 1;
            }
            break;
        case MOS_BOOK_MODE_BROWSING:
            if ((lr & 0x10) || (lr & 0x20) || (select & 4) || (select & 8)) {
                cmd = 0x64;
                if (select & 4) {
                    this->select -= 1;
                }
                if (select & 8) {
                    this->select += 1;
                }
                if ((lr & 0x40) || (lr & 0x10)) {
                    this->select -= 10;
                }
                if ((lr & 0x80) || (lr & 0x20)) {
                    this->select += 10;
                }
                if (this->select < 0) {
                    this->select = this->list_num - 1;
                }
                if (this->list_num <= this->select) {
                    this->select = 0;
                }
                if (this->select < 0) {
                    this->select = 0;
                }
                this->monster_info = GetMonsterBaseInfo(this->list[this->select]);
                this->SetMonsterInfo(this->monster_info);
            } else if (push & 2) {
                cmd = 0xA;
                MenuSePlay(5);
            } else if (menu_debug_flag != 0) {
                if (push & 4) {
                    for (i = 0; i < MOS_BOOK_MEMO_NUM; i++) {
                        if (i != 0x30 && i != 0x44) {
                            KillMonsterCount(i, 1);
                        }
                    }
                    MenuSePlay(1);
                }
            }
            break;
    }
    if (0 <= cmd) {
        switch (cmd) {
            case 0xA:
                this->mode = MOS_BOOK_MODE_FADING_OUT;
                this->FadeOutMenu(0x3C, 0.0f);
                break;
            case 0x64:
                this->load_phase = 1;
                break;
        }
    }
    switch (this->load_phase) {
        case 0:
            break;
        case 1:
            this->load_wait = 0;
            this->load_phase += 1;
            break;
        case 2:
            this->load_wait += 1;
            if (this->load_wait >= 20) {
                StartReadBG();

                this->stack.stack_used = 0;
                this->stack.lock = 0;
                MenuMonsterLoadBG(&this->stack, MenuCharaBuild2, this->list[this->select], 1);
                this->monster = NULL;
                this->load_phase += 1;
                if (MenuCharaBuild2[0]->reading == 0) {
                    this->load_phase = 0;
                }
            }
            break;
        case 3:
            if (ReadBGSync() == 0) {
                this->load_phase += 1;
                this->show_wait = 0;
                this->monster = new (this->stack.Alloc(0x105)) CActionChara;
                this->monster->Initialize(0);
                MenuMonsterLoadBGCheck(MenuCharaBuild2, &this->monster, this->tex_block_no, -1);
                float x = -12.8f;
                float y = -6.6f;
                float z = 0.0f;
                this->monster->SetPosition(x, y, z);
                this->monster->SetMotion("\227\247\202\277", 0, 1);
                MonsterScaleCheck((CCharacter2 *)this->monster);
            }
            break;
        case 4:
            this->skip_draw ^= 1;
            if ((s8)this->skip_draw != 0) {
                this->monster->Step();
            }
            this->show_wait += 1;
            if (this->show_wait > 20) {
                this->show_wait = 20;
            }
            break;
    }
    half = 0.5f;
    scroll = this->bg_scroll + half;
    this->bg_scroll = scroll;
    if (0.0f <= scroll) {
        this->bg_scroll = scroll - 256.0f;
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", KeyStep__12CMosBookMenuFv);
#endif
void MonsterBookInit(mgCMemory *stack, int *tex_block, int mode) {
    CMosBookMenu *book;
    int i;
    int size;

    size = stack->stGetRest();
    MosBookStack.stSetBuffer(stack->stGetTop(), size);
    book = new (MosBookStack.Alloc(0x9A)) CMosBookMenu;
    MenuMosBookPtr = book;
    book->SetTexBlock(tex_block);
    MonsterBookPtr = (u8*)&GetSaveData()->monster_book;
    MonsterBookBootMode = mode;
    MenuBGReadInfo2Malloc(&MosBookStack, tbl_5848);
    MenuLoadInfo.mode = 4;
    MenuLoadInfo.unk_2 = 1;
    MenuLoadInfo.unk_6[1] = 0;
    MenuLoadInfo.unk_1 = 0;
    MenuLoadInfo.unk_4 = -1;
    MenuLoadInfo.unk_5 = 0;
    MosBookStack.Align64();
    MenuMosBookPtr->InitEnd();
}

int MonsterBookKey() {
    return MenuMosBookPtr->KeyStep();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuchr", MonsterBookDraw__Fv);
void mgRect<short>::Set(short new_left, short new_top, short new_right, short new_bottom) {
    left = new_left;
    top = new_top;
    right = new_right;
    bottom = new_bottom;
}

// Static initialiser (.init)


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", monster_progress_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_992__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_robo_memorytbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_chr_memorytbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_1233__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1372__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", nextIDtbl_1594__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", partt_2332__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_2483__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2629__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2691__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2696__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", overcode_3172__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_3186__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", posdef_3194__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", refdef_3195__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_3196__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", convert_table_3430__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", ghobitbl_3437__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3481__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", get_stringtbl_3557__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_3725__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_load_chrpathtbl_3811__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_infocfgname__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4327__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", MonsterDataPath__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4586__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", convtbl_4621__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4728__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4967__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_5016__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", infomsg_5256__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", putw_5262__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", monster_type_name__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", monster_jyakuten__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", monstere_file_template__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5452__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", stand_bit_5472__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tiletbl_5573__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", under_brdtbl_5576__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", put_under_offset_5577__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", ic_5580__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", line_5595__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", wakutbl_5600__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tbl_5848__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1078__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1104__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1131__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1132__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1133__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1134__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1135__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1171__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1172__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1173__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1174__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1175__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1176__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1177__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1178__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1179__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1180__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1181__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1234__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1235__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1236__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1237__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1276__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1277__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1278__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1279__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1280__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1281__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1282__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1283__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1284__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1285__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1304__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1319__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1361__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_1402__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2003__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2004__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2005__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2006__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2007__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2008__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2009__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2010__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2011__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2012__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2013__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2014__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2015__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2016__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2017__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2018__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2019__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2020__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2021__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2022__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2023__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2191__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2192__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2193__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2194__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2195__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2196__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2197__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2286__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2287__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2303__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2304__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2305__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2306__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2307__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2333__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2334__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2335__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2336__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2337__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2338__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2363__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2364__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2365__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2595__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2596__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2662__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2770__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2771__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2772__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2773__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2774__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2775__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2776__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2777__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2778__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2779__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2780__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2781__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2782__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2783__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2784__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2785__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2912__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2913__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2914__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2915__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2916__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2917__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2918__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2940__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2941__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2942__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2943__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2944__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3160__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3161__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3162__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3163__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3164__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3165__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3166__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3197__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3198__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3199__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3200__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3201__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3202__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3269__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3270__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3271__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3272__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3273__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3274__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3275__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3276__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3558__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3559__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3560__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3561__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3685__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3686__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3687__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3688__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3689__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3690__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3691__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3692__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3693__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3694__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3695__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3696__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3697__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3698__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3699__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3700__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3701__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3702__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3703__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3704__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3705__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3706__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3707__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3708__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3726__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3727__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3728__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3729__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3730__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3731__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3732__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3733__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3734__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3735__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3736__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3737__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3762__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3779__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3780__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3790__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3791__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3792__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3793__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3794__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3812__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3813__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3814__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3913__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3969__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3970__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4123__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4186__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4276__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4277__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4278__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4296__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4517__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4518__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4519__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4520__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4548__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4789__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4790__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4791__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4868__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4950__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5051__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5052__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5053__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5197__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5257__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5258__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5259__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5260__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5261__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5356__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5357__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5358__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5359__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5360__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5361__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5362__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5363__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5364__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5365__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5366__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5367__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5368__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5369__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5370__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5371__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5372__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5373__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5374__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5375__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5376__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5377__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5378__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5379__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5380__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5381__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5382__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5383__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5384__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5385__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5386__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5387__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5388__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5389__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5390__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5391__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5392__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5393__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5394__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5395__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5396__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5397__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5398__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5399__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5400__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5401__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5402__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5403__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5404__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5405__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5406__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5407__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5408__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5409__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5410__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5411__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5412__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5413__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5414__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5415__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5416__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5417__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5418__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5419__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5420__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5421__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5422__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5423__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5424__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5425__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5426__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5427__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5428__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5429__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5430__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5431__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5432__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5433__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5434__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5435__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5558__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5559__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5560__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5561__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5839__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_5893__DATA);

// Static initialiser table (.ctor)


// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", __vt__12CMosBookMenu__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", __vt__15CMenuCostumeSel__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", __vt__14CMenuMosSelect__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", __vt__15CMenuChrCngMenu__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", MenuSoundCharaNo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", msgtbl1_1732__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", se_sndtbl_1749__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", cursor_revtbl_2237__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_2288__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", max_3170__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", viewnum_3171__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_chara_chrtbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", menu_chara_cfg_chrtbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_3810__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", pathtbl_3836__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", at_4158__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", convItoPhase_4229__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", monster_load_id__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", NowReadMainCharaNo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", NowReadMainCharaMonsterNo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", phasetbl_5119__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", tilergba_5203__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuchr", convtbl_5238__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(MorattaStack, 0x4);
INCLUDE_BSS(MenuLoadInfo, 0x8);
INCLUDE_BSS(MenuCharaChangeBase_Tex, 0x4);
INCLUDE_BSS(MenuCharaChangeCLUT_Tex, 0x4);
INCLUDE_BSS(MenuCharaChangeStar_Tex, 0x4);
INCLUDE_BSS(MenuCharaChangeCLUT, 0x4);

INCLUDE_BSS(menu_debug_npcselect, 0x4);
INCLUDE_BSS(menu_debug_npc_decide, 0x4);
INCLUDE_BSS(MenuDebugChangeSelectMode, 0x4);
INCLUDE_BSS(MenuDebugCharaChangeSelect, 0x4);
INCLUDE_BSS(SelectedCmdNo_1415, 0x4);
INCLUDE_BSS(init_1416, 0x4);
INCLUDE_BSS(at_1650__2, 0x4);
INCLUDE_BSS(at_1684__2, 0x8);
INCLUDE_BSS(MenuGetPartySeFlag, 0x8);
INCLUDE_BSS(at_2232, 0x8);
INCLUDE_BSS(at_2289__2, 0x8);
INCLUDE_BSS(ChrChangMenuPt, 0x8);
INCLUDE_BSS(at_2371__4, 0x8);
INCLUDE_BSS(MenuMosTexture, 0x4);
INCLUDE_BSS(CharaSndBuffer, 0x4);
INCLUDE_BSS(mos_effect_henge_param, 0x4);
INCLUDE_BSS(mos_effect_read_num, 0x4);
INCLUDE_BSS(MenuMosSelectPtr, 0x4);
INCLUDE_BSS(menu_debug_select__2, 0x4);
INCLUDE_BSS(select_monster_save_3371, 0x4);
INCLUDE_BSS(init_3372__2, 0x4);
INCLUDE_BSS(at_3412, 0x4);
INCLUDE_BSS(at_3440, 0x4);
INCLUDE_BSS(NowReadMainCharaPhase, 0x4);
INCLUDE_BSS(NowReadMainChara, 0x4);
INCLUDE_BSS(NowMainCharaChngTex, 0x4);
INCLUDE_BSS(NowMainCharaChngTexMoveX, 0x4);
INCLUDE_BSS(NowMainCharaChngTexMovePhase, 0x4);
INCLUDE_BSS(NowMainCharaFrameImage, 0x4);
INCLUDE_BSS(NowMainCharaChngStatusBit, 0x4);
INCLUDE_BSS(MenuNPCLoadFlag, 0x4);
INCLUDE_BSS(MenuPartyNPCModelReadBuffer, 0x4);

INCLUDE_BSS(CostumeAttr, 0x8);
INCLUDE_BSS(MenuCosPtr, 0x4);
INCLUDE_BSS(MonsterBookPtr, 0x4);
INCLUDE_BSS(Tex_MBook, 0x4);
INCLUDE_BSS(Tex_MBase, 0x4);
INCLUDE_BSS(Tex_MBg, 0x4);
INCLUDE_BSS(MonsterBookBootMode, 0x4);
INCLUDE_BSS(MenuMosBookPtr, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(MenuCharaBuild2, 0x1C);
INCLUDE_BSS(D_01F3C7FC, 0x4);
INCLUDE_BSS(MenuActionChara, 0x20);
INCLUDE_BSS(MenuActionCharaBuffer, 0x150);
INCLUDE_BSS(MenuLoadItemNo, 0x20);
INCLUDE_BSS(at_1083__2, 0x20);
INCLUDE_BSS(MenuChangeMemory, 0x30);
INCLUDE_BSS(MenuChangeNpcMemory, 0x30);
INCLUDE_BSS(ChrChangeInitTextureStack, 0x30);
INCLUDE_BSS(at_1806__2, 0x20);
INCLUDE_BSS(at_2372__4, 0x20);
INCLUDE_BSS(at_2674, 0x80);
INCLUDE_BSS(at_2675, 0x80);
INCLUDE_BSS(at_2676, 0x80);
INCLUDE_BSS(MenuMonChangeLoadStack, 0x30);
INCLUDE_BSS(MenuMosBuildStack, 0x30);
INCLUDE_BSS(MenuMosLoadStack, 0x30);
INCLUDE_BSS(MenuMonsterBGInfo, 0x20);
INCLUDE_BSS(mos_effect_readbuff1, 0x10);
INCLUDE_BSS(mos_effect_readbuff2, 0x10);
INCLUDE_BSS(mos_effect_readbuff1_size, 0x10);
INCLUDE_BSS(mos_effect_readbuff2_size, 0x10);
INCLUDE_BSS(at_3054__2, 0x20);
INCLUDE_BSS(at_3511, 0x20);
INCLUDE_BSS(at_3529, 0x20);
INCLUDE_BSS(at_3554, 0x20);
INCLUDE_BSS(SwordEffectStack, 0x30);
INCLUDE_BSS(at_3974, 0x20);
INCLUDE_BSS(at_3975, 0x20);
INCLUDE_BSS(at_3993, 0x20);
INCLUDE_BSS(at_4300__2, 0x20);
INCLUDE_BSS(at_4328, 0x20);
INCLUDE_BSS(at_4329, 0x20);
INCLUDE_BSS(script_file_name, 0x20);
INCLUDE_BSS(at_4565, 0x20);
INCLUDE_BSS(at_4585, 0x10);
INCLUDE_BSS(NowMainReadPosition, 0x10);
INCLUDE_BSS(NowMainReadRotation, 0x10);
INCLUDE_BSS(MosBookStack, 0x30);
INCLUDE_BSS(at_5482, 0x20);
