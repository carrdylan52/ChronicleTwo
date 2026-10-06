#include "common.h"
#include "menumain.hpp"
#include "charasetup.hpp"
#include "dynamicanime.hpp"
#include "sound.hpp"
#include "dataread.hpp"
#include "prespr.hpp"
#include "mg_drawprim.hpp"
#include <cstdio>
#include <cstring>
#include "dngfloor.hpp"
#include "map.hpp"
#include "menushop.hpp"
#include "menusystemdata.hpp"
#include "menuop.hpp"
#include "editmenu.hpp"
#include "dngmenu.hpp"
#include "menuchr.hpp"
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
#include "inventmn.hpp"
#include "menuaqua.hpp"
#include "menumap.hpp"
#include "menucapt.hpp"
#include "nameregi.hpp"
#include "nowload.hpp"
#include "event_func.hpp"
#include "sce/libvu0.h"

static void MenuWorldTrans();
static void MenuPolygonSetEnv();
static void MenuPolygonEnvReset();
static void MenuDebugModeDraw();
static short CheckEventDay(int *day);
static void MakeMenuTopic();
static int MenuInternSelectKey();
static void MenuInternSelectDraw();

static mgCMemory MenuMainStack;

static mgCMemory MenuMainStack_Next;

static mgCDrawPrim MenuPrimFix;

mgCMemory MenuMainTextureReadBuf;

mgCMemory MenuSoundBuffer;

static CMenuFont TopicFont;

static int MenuEtcSpecialCode;

static s8 MenuLoopType;

static mgCDrawPrim *MenuPrim = &MenuPrimFix;

static int CommonMenuModeID2[8];

static CMenuInter *CMenuInterPt;

static u8 MenuDoubleDrawCheck;

static s16 MenuTopicAlphaCalc;

static int old_light_menu;

static float SndPortVol_Enemy;

static sceVu0FVECTOR menu_old_chara_position;

static sceVu0FVECTOR menu_old_chara_rotation;

static int MenuBGMVolume_Save;

static CMenuPosDataForm *MenuAreaBrdForm;

static CMenuPosDataForm *MenuTimeBrdForm;

static char *MenuAreaName;

static s16 MenuTopicType;

static int MenuTopicAlpha = 0x80;

static s16 MenuTopicLength;

static int TopicFontX;

static int (*menu_keyfunctbl[30])() = {
    MenuInternSelectKey,
    MenuInternSelectKey,
    MenuItemKey,
    MenuGeoramaKey,
    MenuCharaChangeKey,
    MenuInventKey,
    WorldMoveKey,
    MenuOptionKey,
    MenuManualKey,
    NULL,
    MenuMonsterBoxKey,
    DngTreeMapKey,
    MenuShopKey,
    MenuSaveKey,
    MenuSaveKey,
    MenuItemSelectKey,
    MenuInventKey,
    MenuChapterKey,
    MenuAquaKey,
    MenuRemovalKey,
    NameRegistKey,
    MenuGyoraceFishSelKey,
    MenuNPCQuestViewKey,
    MenuCostumeKey,
    KeyMainCharaBG,
    GyoraceMenuKey,
    SphidaMenuKey,
    MonsterBookKey,
    SubGameSaveKey,
    SphidaScoreViewKey
};

static void (*menu_drawfunctbl[30])() = {
    MenuInternSelectDraw,
    MenuInternSelectDraw,
    MenuItemDraw,
    MenuGeoramaDraw,
    MenuCharaChangeDraw,
    MenuInventDraw,
    WorldMoveDraw,
    MenuOptionDraw,
    MenuManualDraw,
    NULL,
    MenuMonsterBoxDraw,
    DngTreeMapDraw,
    MenuShopDraw,
    MenuSaveDraw,
    MenuSaveDraw,
    MenuItemSelectDraw,
    MenuInventDraw,
    MenuChapterDraw,
    MenuAquaDraw,
    MenuRemovalDraw,
    NameRegistDraw,
    MenuGyoraceFishSelDraw,
    MenuNPCQuestViewDraw,
    MenuCostumeDraw,
    DrawMainCharaBG,
    GyoraceMenuDraw,
    SphidaMenuDraw,
    MonsterBookDraw,
    SubGameSaveDraw,
    SphidaScoreViewDraw
};

static sceVu0FVECTOR menu_basedgRef = {0.0f, 0.0f, -100.0f, 1.0f};

static sceVu0FVECTOR menu_basedgCamPos = {0.0f, 0.0f, 100.0f, 1.0f};

static int CommonMenuModeID[2][8] = {
    2, 4, 5, 6, 7, 8, -1, 0, 2, 4, 5, 11, 7, 8, -1, 0
};

struct MonsterTableEntry {
    s16 name_no;    /**< Monster whose name the bookshelf shows. */
    s16 message_no; /**< Item message shown with that monster. */
};

static MonsterTableEntry monster_table[10] = {
    {0, 269},
    {220, 280},
    {8, 318},
    {0, 0},
    {164, 211},
    {72, 225},
    {124, 234},
    {44, 289},
    {176, 276},
    {236, 189}
};

// Code (.text)
void MenuScreenBlackBeltSet(s32 enable) {
}
int GetMenuLoopType() {
    return MenuLoopType;
}
int CheckTrushMenu() {
    short mode = MenuCommonInfo->open_type;
    if (mode == 0x10 || mode == 0x11) {
        return 1;
    }
    return 0;
}
CSaveDataDungeon *menu_GetSaveDataDungeon(void) {
    CSaveData *save_data;

    save_data = GetSaveData();
    if (save_data != NULL) {
        return &save_data->save_dungeon;
    }
    return NULL;
}
void *menu_GetBattleAreaScene(void) {
    CScene *scene;

    scene = GetMainScene();
    if (scene != NULL) {
        return &scene->battle_area;
    }
    return NULL;
}
CMenuSystemData *GetMenuSysData(void) {
    CSaveData *save_data;

    save_data = GetSaveData();
    if (save_data != NULL) {
        return &save_data->menu_system_data;
    }
    return NULL;
}
int CheckBitFlagMenu(int flag_no) {
    CSaveData *save_data;

    save_data = GetSaveData();
    if (save_data != NULL) {
        return save_data->GetBitFlag(flag_no);
    }
    return 0;
}
int CheckShortFlagMenu(int flag_no) {
    CSaveData *save_data;

    save_data = GetSaveData();
    if (save_data != NULL) {
        return save_data->GetShortFlag(flag_no);
    }
    return 0;
}
int CheckStartChapter8(CSaveData *save) {
    if (save == NULL) {
        return 0;
    }
    if (save->GetBitFlag(0x2E0) == 1) {
        if (!save->GetBitFlag(0x320)) {
            return 1;
        }
    }
    return 0;
}
void InitMenuEtcSpecialFlag() {
    MenuEtcSpecialCode = 0;
}
int SetMenuEtcFlag(int flag) {
    MenuEtcSpecialCode |= flag;
    return MenuEtcSpecialCode;
}
int GetMenuEtcFlag() {
    return MenuEtcSpecialCode;
}
mgCDrawPrim *GetMenuPrim(void) {
    return MenuPrim;
}
void MenuMainImageDataEnter(int tex_block) {
    u8 *image;

    image = (u8*)GetMenuMainIMGPtr();
    if (image != NULL) {
        mgTexManager.EnterIMGFile(image, tex_block, NULL, NULL);
        MenuPosData->ResetTextureBlockNo("mnmain", tex_block);
    }
}
void SetMenuFrameRate(int rate) {
    mgFrameRate = rate;
}
void SetMenuKeyCtrlEnv(int env) {
    GamePad.AutoRepeatOff();
    GamePad.MenuModeOff();
    if (env == 0) {
        GamePad.SetAutoRepeat(0xF000, 15, 4);
        GamePad.MenuModeOn(120);
    } else if (env != 1 && env == 2) {
        GamePad.SetAutoRepeat(0xF00C, 15, 4);
        GamePad.MenuModeOn(120);
    }
}
/** Enables or disables the pad reset gesture while a dungeon menu is open. */
static void DisablePadReset(int disable) {
    DNG_BATTLE_AREA *scene;

    if (GetNowLoopNo() == 2) {
        scene = (DNG_BATTLE_AREA*)menu_GetBattleAreaScene();
        if (scene != NULL) {
            if (disable != 0) {
                scene->pause_flag |= 0x8000;
            } else {
                scene->pause_flag &= ~0x8000;
            }
        }
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", MenuMainInit__FP13MENU_INIT_ARG);
int MenuMainExit() {
    CCharacter2 *chara;
    int i;
    int active_chara;
    int is_fishing_menu;
    CScene *scene;
    mgCCamera *camera;
    sceVu0FMATRIX view_matrix;
    sceVu0FVECTOR pos;
    sceVu0FMATRIX world_matrix;
    sceVu0FMATRIX identity;

    mgActiveLighting(old_light_menu, 0);
    MenuDeleteTextureBlock(MenuCommonInfo->tex_block);
    PauseEnable(1);
    DisablePadReset(0);
    MenuPrimFix.Initialize(NULL, NULL);
    MenuPrimFix.offset_x = 0;
    MenuPrimFix.offset_y = 0;
    if (MenuSePlayUsedFlag != 0) {
        sndInitPort(8);
    }
    sndSetPortVol(5, SndPortVol_Enemy);
    active_chara = MenuUserDataManPtr->active_chr_no;
    GetBattleCharaInfo();
    if (MenuArg.end_code == 1) {
        active_chara = MenuArg.result[0];
        SetMenuEtcFlag(1);
    }
    MenuCommonInfo->user_data->SetActiveChrNo(active_chara);
    if (MenuMainScene != NULL) {
        chara = (CCharacter2 *)MenuMainScene->GetCharacter(0);
        if (chara != NULL) {
            chara->SetPosition(menu_old_chara_position);
            chara->SetRotation(menu_old_chara_rotation);
        }
        if ((MenuArg.end_code == 1 || MenuArg.end_code == 21) && chara != NULL) {
            chara->UpdatePosition();
            i = 0;
            if (chara->dynamic_anime_num != 0) {
                while (i < chara->dynamic_anime_num) {
                    chara->dynamic_anime[i].ResetPosition();
                    i++;
                }
            }
            MenuMainScene->fade.FadeOut(-1, 0.0f, 0.0f, 0.0f);
        }
        MenuMainScene->SetVolBGM(MenuBGMVolume_Save);
    }
    is_fishing_menu = (MenuArg.end_code == 5) | (MenuArg.end_code == 6);
    if (active_chara == 0) {
        if (MenuUserParam.chara[0]->equip[0].IsFishingRod() != 0) {
            if (is_fishing_menu == 0) {
                MenuArg.end_code = 11;
                MenuArg.result[0] = MenuUserDataManPtr->GetFishingRodNo();
                MenuArg.result[1] = MenuUserDataManPtr->GetFishBait();
                if (GetMenuLoopType() == 1) {
                    chara = (CCharacter2 *)MenuMainScene->GetCharacter(1);
                    if (chara != NULL) {
                        chara->DeleteImage();
                        ((CActionChara *)chara)->Initialize(NULL);
                    }
                    SetupUnitMan(MenuMainScene, MenuUserDataManPtr, active_chara, NULL);
                }
            } else {
                ReEquipFishingGameWeapon();
            }
        }
    }
    MenuPrevEndCode = -1;
    SetMenuKeyCtrlEnv(1);
    MenuPosData->ClearPos();
    MenuMainStack.stack_used = 0;
    MenuPosData = NULL;
    MenuMainStack.lock = 0;
    SetDngTreeFlag(0);
    SetMenuFrameRate(2);
    CMenuInterPt = NULL;
    menu_debug_flag = 0;
    EdEventMenuExit();
    mgSetProjection(MenuDrawEnv->old_projection);
    scene = GetMainScene();
    camera = scene->GetCamera(scene->active_camera);
    if (camera != NULL) {
        camera->GetCameraMatrix(view_matrix);
        camera->GetPos(pos);
        sceVu0UnitMatrix(identity);
        sceVu0MulMatrix(world_matrix, identity, view_matrix);
        mgSetViewMatrix(world_matrix, pos);
    }
    return 1;
}
s32 MenuMainLoop(void) {
    s32 next_mode = MenuMainKey();
    MenuMainDraw();
    return next_mode;
}
int MenuMainKey() {
    static s8 refresh_cnt;
    static s8 init;
    int result;
    short page;
    int menu;


    MenuWorldTrans();
    MenuPolygonSetEnv();
    ReadBG();
    MenuCommonInfo->SelDataInit();
    MenuMainFrameStep();
    MenuAreaBoardNameStep();
    if (DebugFlag != 0 && GamePad.Down(0x400) != 0) {
        menu_debug_flag ^= 1;
    }
    result = menu_keyfunctbl[MenuCommonInfo->now_mode]();
    switch (result) {
        case 1:
            page = MenuCommonInfo->open_type;
            switch (page) {
                case 0:
                case 1: {
                    menu = MenuCommonInfo->now_mode;
                    if (menu >= 2 && menu < 12) {
                        int page_table[2] = {0, 1};
                        MenuCommonInfo->now_mode = page_table[page];
                        MenuCommonInfo->key_enable = 1;
                        if (MenuCommonInfo->cursor_form != NULL) {
                            MenuCommonInfo->cursor_form->draw_flag = 1;
                        }
                        MenuCommonInfo->CursorFadeIn(10.0f, 1);
                        MenuCommonInfo->SetVibeCnt(60, 30);
                        CMenuInterPt->cursor_jump = 1;
                        MenuPosData->FormInfoClear(10, 80);
                        MenuPosData->InitDrawList();
                        result = 0;
                        CMenuInterPt->step = 0;
                        CMenuInterPt->help_update = 1;
                        MenuCommonInfo->cursor = CMenuInterPt->select_no;
                        if (menu == 11 || menu == 6) {
                            MenuMainScene->fade.FadeIn(40);
                            ReturnMenuIntern(0);
                        }
                        MenuTopicAlphaCalc = 0;
                    }
                } break;
            }
            break;
        case 2:
            if (MenuCommonInfo->now_mode == 4) {
                MenuMainScene->fade.FadeIn(40);
            }
            break;
    }
    MenuItemCommandCounter++;
    if (MenuItemCommandCounter > 10000000) {
        MenuItemCommandCounter = 0;
    }
    MenuCommonInfo->unk_0 ^= 1;
    MenuCommonInfo->StepMenuBGM();
    MenuDrawParamStep();
    if (init == 0) {
        refresh_cnt = 0;
        init = 1;
    }
    refresh_cnt++;
    if (refresh_cnt >= 25) {
        refresh_cnt = 0;
        UserDataRefresh();
    }
    MenuDoubleDrawCheck = 0;
    return result;
}
void MenuMainDraw() {
    mgCDrawPrim *prim;
    int next_page;

    if (MenuDoubleDrawCheck == 0) {
        prim = GetMenuPrim();
        prim->offset_x = 0;
        prim->offset_y = 0;
        DrawMenuFillBox(0x80, 0, 0, 0);
        prim->offset_x = 0;
        prim->offset_y = 0;
        menu_drawfunctbl[MenuCommonInfo->now_mode]();
        if (menu_debug_flag != 0) {
            MenuDebugModeDraw();
        }
        MenuPolygonEnvReset();
        MenuMainScene->fade.Draw();
        next_page = MenuCommonInfo->next_mode;
        if (next_page >= 0) {
            if (MenuCommonInfo->now_mode != next_page) {
                MenuCommonInfo->now_mode = next_page;
                MenuCommonInfo->next_mode = -1;
            }
        }
        MenuDoubleDrawCheck = 1;
    }
}
int NextMenuInit(int mode, mgCMemory *stack, int *tex_blocks) {
    int page;
    int known;
    int dungeon_mode;
    char *map_name;

    page = MenuCommonInfo->open_type;
    known = 1;
    switch (mode) {
        case 2:
            MenuItemInit(stack, tex_blocks, page);
            break;
        case 4:
            MenuCharaChangeInit(stack, tex_blocks, page);
            break;
        case 5:
        case 16:
            MenuInventInit(stack, tex_blocks, page);
            break;
        case 11:
            dungeon_mode = 0;
            if (MenuMainScene != NULL) {
                dungeon_mode = MenuSaveDataDungeonPtr->stage_id;
            }
            if (TreeMapCallDungeonSubMap != 0) {
                map_name = MenuMainScene->GetMapName(MenuMainScene->active_map);
                if (strcmp(map_name, "s05") == 0) {
                    dungeon_mode = 1;
                }
                if (strcmp(map_name, "d04b01") == 0) {
                    dungeon_mode = 3;
                }
            }
            DngTreeMapInit(stack, tex_blocks, page, dungeon_mode);
            break;
        case 18:
            MenuAquaInit(stack, tex_blocks, page);
            break;
        case 6:
            WorldMoveInit(stack, tex_blocks, page);
            break;
        case 8:
            MenuManualInit(stack, tex_blocks, page);
            break;
        case 7:
            MenuOptionInit(stack, tex_blocks, page);
            break;
        default:
            known = 0;
            break;
    }
    if (known != 0) {
        MenuCommonInfo->next_mode = mode;
    } else {
        MenuCommonInfo->next_mode = -1;
    }
    return known;
}
void MenuCamInit(float speed) {
    *(u_long128 *)MenuDrawEnv->ref = *(u_long128 *)menu_basedgRef;
    *(u_long128 *)MenuDrawEnv->pos = *(u_long128 *)menu_basedgCamPos;
    MenuDrawEnv->speed = speed;
    MenuDrawEnv->camera.Resume();
}
/** Sets the menu camera and installs its view matrix. */
static void MenuWorldTrans(void) {
    mgCCamera *camera;
    sceVu0FMATRIX view_matrix;
    sceVu0FVECTOR pos;
    sceVu0FMATRIX world_matrix;
    sceVu0FMATRIX identity;

    mgSetProjection(MenuDrawEnv->projection);
    camera = &MenuDrawEnv->camera;
    MenuDrawEnv->camera.GetCameraMatrix(view_matrix);
    camera->GetPos(pos);
    MENU_DRAW_ENV *env = MenuDrawEnv;
    float rate = -1.0f;
    camera->SetSpeed(env->speed, rate);
    camera->SetNextRef(MenuDrawEnv->ref);
    camera->SetNextPos(MenuDrawEnv->pos);
    camera->Step(1);
    sceVu0UnitMatrix(identity);
    sceVu0MulMatrix(world_matrix, identity, view_matrix);
    mgSetViewMatrix(world_matrix, pos);
}
/** Installs the ambient light used by the menu models. */
static void MenuPolygonSetEnv(void) {
    mgGetAmbient(MenuDrawEnv->old_ambient);
    mgSetAmbient(MenuDrawEnv->ambient);
}
/** Restores the scene ambient light after drawing the menu. */
static void MenuPolygonEnvReset(void) {
    mgSetAmbient(MenuDrawEnv->old_ambient);
}
char *GetMenuCfgFileName(int cfg_no, int unused) {
    static char *menu_main_cfgname[2] = {
        "men0.pac",
        "men0.pac"
    };
    static char workchr[0x60];
    int local;

    sprintf(workchr, "menu/%d/", LanguageCode);
    strcat(workchr, menu_main_cfgname[cfg_no]);
    printf("menu_stack ptr : %p\n", &local);
    return workchr;
}
short *GetMenuMainMessageBuffer(void) {
    int size;

    return (short *)GetPackFile(MenuArg.pack, "allmenu.mes", &size);
}
u_int *GetMenuMainIMGPtr(void) {
    return GetPackFile(MenuArg.pack, "frametex.img", 0);
}
u_int *GetMenuMainPosCfgBuffer(int *size) {
    return GetPackFile(MenuArg.pack, "menu0.cfg", size);
}
void SetCommonMenuModeID() {
    int table;
    int bit_ctrl;
    int i;

    table = MenuCommonInfo->open_type;
    bit_ctrl = GetSaveData()->GetBitCtrl();
    if (table == 0x10) {
        table = 0;
    }
    if (table == 0x11) {
        table = 1;
    }
    if ((unsigned int)table < 2) {
        for (i = 0; i < 8; i++) {
            CommonMenuModeID2[i] = CommonMenuModeID[table][i];
            if (CommonMenuModeID2[i] == 6 && (bit_ctrl & 0x10)) {
                CommonMenuModeID2[i] = 11;
                TreeMapCallDungeonSubMap = 1;
            }
        }
    }
}
int *GetCommonMenuModeID(void) {
    return CommonMenuModeID2;
}
bool CursorSaveOptionState() {
    CSaveData *save_data = GetSaveData();
    bool remember_cursor = false;
    if (save_data != NULL) {
        SV_CONFIG_OPTION *config = save_data->GetConfig();
        remember_cursor = !config->cursor_save;
    }
    return remember_cursor;
}
void ReturnMenuIntern(int out) {
    static char *acttbl[2] = {
        "\222\206\202\326",
        "\212O\202\326"
    };
    char *action = acttbl[out];

    if (MenuAreaBrdForm != NULL) {
        MenuAreaBrdForm->SetAction(action);
    }
    if (MenuTimeBrdForm != NULL) {
        MenuTimeBrdForm->SetAction(action);
    }
}
void MenuAreaBoardNameStep() {
    CDC2Mes *message;
    float hours;
    float minutes;
    int day;

    if (MenuAreaBrdForm != NULL) {
        char *names[2] = {NULL, NULL};
        names[0] = MenuAreaName;
        message = MenuDCMsg[1];
        message->MakeMsg(0x32);
        message->SetMsgItemNo(names, 1);
        message->GetStrWidth(names[0]);
        int position[2] = {0, 0};
        MenuAreaBrdForm->GetNextMovePos(position);
        int widths[9] = {98, 122, 122, 122, 122, 122, 122, 122, 122};
        message->SetMovePosCenteringGyou(0, position[0] + widths[LanguageCode],
                                         position[1] + 7);
        if (MenuTimeBrdForm != NULL) {
            hours = (int)MenuNowTime;
            minutes = (int)(60.0f * (MenuNowTime - hours));
            if (minutes < 0.0f) {
                minutes = 0.0f;
            }
            if (59.0f < minutes) {
                minutes = 59.0f;
            }
            day = MenuActiveSaveData->day + 1;
            if (day > 9999) {
                day = 9999;
            }
            MenuTimeBrdForm->SetNumber("\223\372\212\324", day);
            if (MenuNowMapType == 5 || MenuNowMapType == 6) {
                MenuTimeBrdForm->SetPartDrawFlag("AM", false);
                MenuTimeBrdForm->SetPartDrawFlag("PM", false);
                MenuTimeBrdForm->SetPartDrawFlag("\216\236", false);
                MenuTimeBrdForm->SetPartDrawFlag("\225\252", false);
                MenuTimeBrdForm->SetPartDrawFlag(";", false);
            } else {
                MenuTimeBrdForm->SetPartDrawFlag("\226\242\227\210", false);
                if (LanguageCode == 3) {
                    MenuTimeBrdForm->SetPartDrawFlag("AM", false);
                    MenuTimeBrdForm->SetPartDrawFlag("PM", false);
                } else if (12.0f <= hours) {
                    MenuTimeBrdForm->SetPartDrawFlag("AM", false);
                    MenuTimeBrdForm->SetPartDrawFlag("PM", true);
                    hours -= 12.0f;
                } else {
                    MenuTimeBrdForm->SetPartDrawFlag("AM", true);
                    MenuTimeBrdForm->SetPartDrawFlag("PM", false);
                }
            }
            if (LanguageCode != 3 && LanguageCode > 0 && hours == 0.0f) {
                hours = 12.0f;
            }
            MenuTimeBrdForm->SetNumber("\216\236", (int)hours);
            MenuTimeBrdForm->SetNumber("\225\252", (int)minutes);
        }
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", CheckEventDay__FPi);
/** Prepares the localized topic for the current event day. */
static void MakeMenuTopic(void) {
    static char *topic_tbl[7][3] = {
        "",
        " ",
        " ",
        "",
        "Fishing Contest: %d hr(s). to go",
        "Finny Frenzy: %d hr(s). to go",
        "",
        "Tournoi de p[UNI00ea]che : encore %d h(s)",
        "Meill. nageoires : encore %d h(s)",
        "",
        "Angelturnier: Noch %d Std.",
        "Fl.-Fieber: Noch %d Std.",
        "",
        "Torneo di Pesca: ancora %d ora/e",
        "Pinna Sprint: ancora %d ora/e",
        "",
        "Concurso Pesca: %d h. para salir",
        "Finny Frenzy: %d h. para salir",
        "",
        "Concurso Pesca: %d h. para salir",
        "Finny Frenzy: %d h. para salir"
    };
    char text[0x80];
    int day;
    int height;
    int width;

    MenuTopicType = 0;
    MenuTopicAlpha = 0;
    MenuTopicAlphaCalc = 0;
    day = 0;
    MenuTopicType = CheckEventDay(&day);
    sprintf(text, topic_tbl[LanguageCode][MenuTopicType], day);
    TopicFont.SetStr(text);
    TopicFont.CalcDrawWH(TopicFont.str, &width, &height);
    MenuTopicLength = width;
    TopicFontX = 30;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", DrawMenuTopic__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", MenuInternInit__FP9mgCMemoryii);
void CMenuInter::Initialize(s32 unused) {
    step = 1;
    select_no = 0;
    select_num = 6;
    mode_list = NULL;
    bg_read_step = 0;
    bg_read_wait = 30;
    cursor_jump = 1;
    next_mode = -1;
    help_update = 0;
}
void MenuCommonBaseDataEnter(mgCMemory *stack, u_int *pack, int pack_size, int tex_block) {
    mgCTextureManager *tex = &mgTexManager;
    int size;
    MenuMainTextureReadBuf.stSetBuffer((u_long128 *)pack, pack_size / 16);
    mgTexManager.EnterIMGFile((u8*)GetPackFile(pack, "edmenu.img", 0), tex_block, 0, 0);
    mgTexManager.EnterIMGFile((u8*)GetPackFile(pack, "allitem.img", &size), tex_block, 0, 0);
    u8 *image = (u8*)GetPackFile(pack, "spectre.img", 0);
    if (image) {
        tex->EnterIMGFile(image, tex_block, 0, 0);
    }
    MenuPosData->AttachCommonTexInfo();
    MenuItemIconTextureBlock = tex_block;
    MenuPosData->MallocPallet(stack);
    MenuPosData->SearchTransPalletNo();
    MenuPosData->ResetTextureInfoAll();
}
void MenuBaseTextureReEnter() {
    mgCTextureManager *tex = &mgTexManager;
    int block = MenuCommonInfo->tex_block[1];
    tex->DeleteBlock(block);
    MenuMainImageDataEnter(block);
    unsigned int *pack = (unsigned int *)MenuMainTextureReadBuf.stack;
    int size;
    tex->EnterIMGFile((u8*)GetPackFile(pack, "edmenu.img", 0), block, 0, 0);
    tex->EnterIMGFile((u8*)GetPackFile(pack, "allitem.img", &size), block, 0, 0);
    u8 *image = (u8*)GetPackFile(pack, "spectre.img", 0);
    if (image) {
        tex->EnterIMGFile(image, block, 0, 0);
    }
    MenuPosData->AttachCommonTexInfo();
    MenuPosData->ResetTextureInfoAll();
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", InitEnd__10CMenuInterFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", PushOk__10CMenuInterFv);
int CMenuInter::ReadBGTexture(int mode, int restart) {
    static char *filetbl[17] = {
        "itemmn0.pac",
        "",
        "chrchg0.pac",
        "inv2_bg.pac",
        "",
        "op1.pac",
        "manual1.pac",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    };
    char name[0x80];
    char **entry;
    if (restart != 0) {
        bg_read_wait = 30;
        bg_read_step = 0;
    }
    if (0 < bg_read_wait) {
        bg_read_wait -= 1;
    }
    if (bg_read_step == 0 && bg_read_wait <= 0) {
        BreakReadBG();
        StartReadBG();
        MenuMainStack_Next.stack_used = 0;
        MenuMainStack_Next.lock = 0;
        entry = &filetbl[mode - 2];
        if (strlen(*entry) != 0) {
            unsigned int size;
            strcpy(name, *entry);
            size = LoadFileMenu(
                name, (u_long128 *)(MenuMainStack_Next.stack + MenuMainStack_Next.stack_used), 0);
            MenuMainStack_Next.Alloc((size & 15) ? (size >> 4) + 1 : size >> 4);
            bg_read_step = 1;
        } else {
            MenuMainStack_Next.stack_used = 0;
            MenuMainStack_Next.lock = 0;
            bg_read_step = 2;
        }
    } else if (bg_read_step == 1) {
        ReadBG();
        if (ReadBGSync() == 0) {
            bg_read_step = 2;
        }
    }
    return bg_read_step == 2;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", MenuInternSelectKey__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", MenuInternSelectDraw__Fv);
void CopyActiveItemAndWeapon(int chara_no, int unused) {
    mgCTexture *textures[2];

    mgCTextureManager *manager = &mgTexManager;
    textures[0] = manager->GetTexture("icon_dmy1", -1);
    textures[1] = manager->GetTexture("icon_dmy2", -1);
    if (textures[0] == NULL || textures[1] == NULL) {
        return;
    }
    CopyActiveIconTexture(textures, chara_no, 0);
    manager->ReloadTexture(-1, (sceVif1Packet *)NULL);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumain", CopyActiveIconTexture__FPP10mgCTextureiPUi);
/** Draws the debug indicator over the menu. */
static void MenuDebugModeDraw() {
    float margin = 6.0f, width = 110.0f, height = 24.0f;
    mgTexManager.ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)NULL);
    DrawMenuFillBox(margin, margin, width, height, 0x5C, 0, 0, 0);
    CMenuFont menu_font;
    menu_font.SetStr("MenuDebugMode");
    menu_font.SetPos(6, 6);
    menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
}
void BookshelfMessageMake(ClsMes *mes, int mes_offset, int item_no, int monster_no) {
    int photos[5];
    char name[0x80];
    int window_no;
    short chara;
    if (mes != NULL) {
        window_no = 0;
        chara = GetUserDataMan()->active_chr_no;
        if (chara == 0) {
            window_no = 0xBBB;
            if (0 < item_no) {
                CheckItemTable(item_no, photos);
                GetPhotoNameStr(photos[0], name);
                strcpy(mes->name[0], name);
                GetPhotoNameStr(photos[1], name);
                strcpy(mes->name[1], name);
                GetPhotoNameStr(photos[2], name);
                strcpy(mes->name[2], name);
                window_no = mes_offset + 0xBB8;
            }
        }
        if (chara == 1) {
            window_no = mes_offset + 0xBC5;
            if (0 <= monster_no) {
                char *monster_name;
                char *monster_text;
                if (monster_no > 9) {
                    monster_no = 0;
                }
                int message_no = monster_table[monster_no].message_no;
                monster_name = GetMonsterName(monster_table[monster_no].name_no);
                monster_text = GetItemMessage(message_no);
                if (monster_name != NULL) {
                    strcpy(mes->name[0], monster_name);
                }
                if (monster_text != NULL) {
                    strcpy(mes->name[1], monster_text);
                }
                window_no = mes_offset + 0xBC2;
                if (monster_no == 8 || monster_no == 1) {
                    window_no += 10;
                }
            }
        }
        mes->MakeMesWin(window_no);
    }
}

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", light_1062__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", lightcolor_1063__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_keyfunctbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_drawfunctbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_basedgRef__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_basedgCamPos__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", CommonMenuModeID__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1699__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_maintopic_colortbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_maintopic_colortbl_shadow__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", topic_tbl_1777__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", filetbl_2141__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", monster_table__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1028__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1440__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1598__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1599__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1621__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1624__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1625__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1630__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1635__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1640__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1683__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1684__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1736__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1737__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1738__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1739__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1740__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1741__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1742__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1778__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1779__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1780__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1781__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1782__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1783__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1784__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1785__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1786__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1787__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1788__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1789__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1859__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1930__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1931__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1932__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1933__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1934__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1935__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1936__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1937__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1938__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1956__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1957__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1958__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2003__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2004__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2142__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2143__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2144__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2145__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2146__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2329__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2330__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2331__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2332__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2333__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2334__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2335__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2344__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2345__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2439__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2440__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_2450__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", MenuPrim__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", MenuPrevEndCode__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", MenuBGTextureBlock__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", MenuItemIconTextureBlock__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1514__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", menu_main_cfgname_1620__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", acttbl_1682__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", MenuTopicAlpha__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", fname_1858__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1865__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1866__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", at_1867__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumain", loopnumtbl_2360__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(MenuMainScene, 0x4);
INCLUDE_BSS(MenuActiveSaveData, 0x4);
INCLUDE_BSS(MenuUserDataManPtr, 0x4);
INCLUDE_BSS(MenuSystemDataPtr, 0x4);
INCLUDE_BSS(MenuConfigPtr, 0x4);
INCLUDE_BSS(MenuSaveDataDungeonPtr, 0x4);
INCLUDE_BSS(MenuFishAquarium, 0x4);
INCLUDE_BSS(MenuNowMapNo, 0x4);
INCLUDE_BSS(MenuNowMapType, 0x4);
INCLUDE_BSS(MenuMainSubDataPackAdr, 0x4);
INCLUDE_BSS(CMenuInterPt, 0x4);
INCLUDE_BSS(MenuInterMes, 0x4);
INCLUDE_BSS(MenuInterMesDrawFlag, 0x4);
INCLUDE_BSS(MenuAreaBrdForm, 0x4);
INCLUDE_BSS(MenuTimeBrdForm, 0x4);
INCLUDE_BSS(MenuAreaName, 0x4);
INCLUDE_BSS(MenuNowTime, 0x4);
INCLUDE_BSS(SndPortVol_Enemy, 0x4);
INCLUDE_BSS(ItemOverFlowCheckFlag, 0x4);
INCLUDE_BSS(MenuDrawEnv, 0x4);
INCLUDE_BSS(MenuCommonInfo, 0x4);
INCLUDE_BSS(MenuLoopType, 0x4);
INCLUDE_BSS(MenuEtcSpecialCode, 0x4);
INCLUDE_BSS(MenuEtcInfo, 0x8);
INCLUDE_BSS(MenuMoveItemPtr, 0x4);
INCLUDE_BSS(MenuBGMVolume_Save, 0x4);
INCLUDE_BSS(MenuFormMI2, 0x4);
INCLUDE_BSS(MenuItemCommandCounter, 0x4);
INCLUDE_BSS(menu_debug_flag, 0x4);
INCLUDE_BSS(MenuTopicAlphaCalc, 0x4);
INCLUDE_BSS(TopicTex, 0x4);
INCLUDE_BSS(old_light_menu, 0x4);
INCLUDE_BSS(HatumeiMenuOkFlag, 0x4);
INCLUDE_BSS(WorldMapOkFlag, 0x4);
INCLUDE_BSS(ManualMenuOkFlag, 0x4);
INCLUDE_BSS(DngMoveMenuOkFlag, 0x4);
INCLUDE_BSS(MenuDoubleDrawCheck, 0x4);
INCLUDE_BSS(refresh_cnt_1523, 0x4);
INCLUDE_BSS(init_1524, 0x8);
INCLUDE_BSS(at_1697__2, 0x8);
INCLUDE_BSS(at_1698__2, 0x8);
INCLUDE_BSS(MenuTopicType, 0x4);
INCLUDE_BSS(MenuTopicLength, 0x4);
INCLUDE_BSS(TopicFontX, 0x8);
INCLUDE_BSS(at_1976, 0x8);
INCLUDE_BSS(at_2209__3, 0x8);

// Uninitialised data (.bss)
INCLUDE_BSS(MenuMainStack, 0x30);
INCLUDE_BSS(MenuMainStack_Next, 0x30);
INCLUDE_BSS(CMenuInterStatic, 0x20);
INCLUDE_BSS(MenuPrimFix, 0x120);
INCLUDE_BSS(MenuItemUse, 0x20);
INCLUDE_BSS(MenuMainTextureReadBuf, 0x30);
INCLUDE_BSS(MenuSoundBuffer, 0x30);
INCLUDE_BSS(MenuArg, 0xA0);
INCLUDE_BSS(menu_old_chara_position, 0x10);
INCLUDE_BSS(menu_old_chara_rotation, 0x10);
INCLUDE_BSS(workchr_1622, 0x60);
INCLUDE_BSS(CommonMenuModeID2, 0x20);
INCLUDE_BSS(TopicFont, 0xC0);
INCLUDE_BSS(at_2351, 0x20);
