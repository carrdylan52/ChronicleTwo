#include "common.h"
#include "menuop.hpp"
#include "sound.hpp"
#include "prespr.hpp"
#include "mg_drawprim.hpp"
#include <cstdio>
#include "font.hpp"
#include "scenesnd.hpp"
#include "savedata.hpp"
#include "userdata.hpp"
#include "gamedata.hpp"
#include "scriptinterpreter.hpp"
#include "mg_math.hpp"
#include "menusys.hpp"
#include "menucls1.hpp"
#include "menudraw.hpp"
#include "memcard.hpp"
#include "mainloop.hpp"
#include "mglib.hpp"
#include "menucommon.hpp"
#include "mapselect.hpp"
#include "dataread.hpp"
#include "menumain.hpp"
#include "sysmes.hpp"
#include "mg_texture.hpp"
#include <cstring>
#include "menuaqua.hpp"

static void InitMnOnePictTex();
static void SetMCIconData(u_int *pack, int slot);
static void SubGameCFGAnalyze(char *command);

static mgCMemory StaticMenuLocalStack;

static mgCMemory StaticMenuLocalStack2;

static mgCMemory SaveMenuStack;

static CMenuPosDataForm *LocalMenuBGForm;

static CMenuPosDataForm *LocalMenuClipForm;

static CMenuPosDataForm *OptionButtonForm;

static CDC2Mes *MenuReturnMsg;

static u8 MenuReturnMsgDrawFlag;

static int MnOnePictTex[8];

static CManualMenu *CManualPtr;

static CMenuOption *CMenuOptionPtr;

static CSaveMenuClass *SaveMenuPtr;

static CMemoryCardManager *MemoryCardPtr;

static CDC2Mes *SaveFileList[13];

static mgCTexture *Tex_SaveFile;

static s16 SubGameSaveBlock;

static float SubSaveTileXY;

static char *SubGameSaveCFGBuffer;

static int SubGameSaveCFGBufferSize;

static s16 MenuMapInfoSave_DngNo;

static s8 manual_list_mesclstbl[5] = {2, 4, 5, 6, 8};

static float config_option_num_i = 16.0f;

static float config_option_num_f = 16.0f;

/** Returns the dungeon record in the save data. */
static inline CSaveDataDungeon *GetDungeonSaveData(CSaveData *data) {
    return &data->save_dungeon;
}

// Code (.text)
void InitMenuReturnMsg(mgCMemory *stack) {
    CDC2Mes *window;
    short *system_mes;
    ClsMes *mes;
    int width;
    int height;

    MenuReturnMsg = NULL;
    if (LanguageCode > 0) {
        window = new (stack->Alloc(0x2A7)) CDC2Mes;
        MenuReturnMsg = window;
        MenuReturnMsg->texture_block = MenuArg.mes_tex_block;
        MenuReturnMsg->buff = NULL;
        system_mes = GetSystemMesBuffer();
        MenuReturnMsg->SetMessData(system_mes, GetMenuMainMessageBuffer());
        MenuReturnMsg->MsgPreset(3);
        MenuReturnMsg->ClsMes::mes_no = -1;
        MenuReturnMsg->MakeMsg(0x5A);
        MenuReturnMsg->StepMsg();
        mes = MenuReturnMsg;
        width = mes->text_w;
        width += 0x20;
        height = mes->text_h;
        height += 0x1A;
        MenuReturnMsg->SetPutPos(0x100 - (width >> 1), (mgScreenHeight - height) - 0x18, width,
                                 height);
    }
    MenuReturnMsgDrawFlag = 0;
}

void SetMenuReturnMsgCtrl(int on) {
    MenuReturnMsgDrawFlag = on != 0;
    if (LanguageCode == 0) {
        MenuReturnMsgDrawFlag = 0;
    }
    if (MenuReturnMsg == NULL) {
        MenuReturnMsgDrawFlag = 0;
    }
}

void DrawMenuReturnMsg() {
    if ((MenuReturnMsgDrawFlag != 0) && (MenuReturnMsg != NULL)) {
        mgTexManager.ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)NULL);
        MenuReturnMsg->StepMsg();
        MenuReturnMsg->DrawMsg();
    }
}

int CheckOmakeVtuto(int no) {
    if (OmakeFlag == 1) {
        if (GetNowLoopNo() == 2) {
            if (no == 0x23 || no == 0x24) {
                return 1;
            }
        }
    }
    return 0;
}

/** Clears the texture slots used by manual picture pages. */
static void InitMnOnePictTex() {
    MnOnePictTex[0] = 0;
    MnOnePictTex[1] = 0;
    MnOnePictTex[2] = 0;
    MnOnePictTex[3] = 0;
    MnOnePictTex[4] = 0;
    MnOnePictTex[5] = 0;
    MnOnePictTex[6] = 0;
    MnOnePictTex[7] = 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", MenuManualInit__FP9mgCMemoryPii);
int MenuManualKey() {
    return CManualPtr->KeyStep();
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", MenuManualDraw__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", KeyStep__11CManualMenuFv);
void CManualMenu::CalcTex(void) {
    CMenuPosDataForm *bg_form;
    int slide_in;
    int slide_in_flag;
    float *left_top;
    int xy[2];
    float speed;
    float target;
    int column_shift;
    int shifted;
    s8 *list_tbl;
    int list;
    CDC2Mes *mes;
    CMenuPosDataForm *panel;
    int item_pos[10][2];
    int item;
    int first_y;
    CMenuPosDataForm *form;
    MENUFORMPARTS_TYPE *bar[3];
    int scroll_range[2];
    int visible_top;

    bg_form = LocalMenuBGForm;
    if (bg_form == NULL) {
        return;
    }
    slide_in = 0;
    slide_in_flag = 0;
    left_top = GetMenuMainFrameLeftTopPos(0);
    xy[0] = (int)(left_top[0]);
    xy[1] = (int)(480.0f + left_top[1]);
    if (GetMenuMainFrameEndFlag() == 0) {
        slide_in = 1;
        slide_in_flag = 1;
        bg_form->x = (float)xy[0];
        bg_form->y = (float)xy[1];
    } else {
        bg_form->x = (float)xy[0];
    }
    bg_form->GetPutPosXY("base_msg", xy[0], xy[1]);
    speed = 3.5f;
    if (mode == 2 || mode == 1) {
        speed = 1.0f;
    }
    target = (float)(xy[1] - top * 24);
    CalcMenu1(target, &list_y, speed, speed, 0);
    if ((float)xy[1] < list_y) {
        list_y = target;
    }
    xy[1] = (int)(list_y);
    column_shift = 0;
    shifted = 0;
    list_tbl = manual_list_mesclstbl;
    for (list = 0; list < 5; list++) {
        first_y = xy[1];
        mes = MenuDCMsg[list_tbl[list]];
        panel = MenuMesForm[list_tbl[list]];
        for (item = 0; item < 10; item++) {
            item_pos[item][0] = xy[0] + column_shift;
            item_pos[item][1] = xy[1];
            xy[1] += 24;
            if (CheckNowEurope() != 0 && shifted == 0 && item >= 8) {
                column_shift = 8;
                shifted = 1;
            }
        }
        mes->SetMsgItemPos(&item_pos[0][0], 10);
        if (xy[1] < 0x48 || first_y > 0x186) {
            panel->draw_flag = 0;
        } else {
            panel->draw_flag = 1;
        }
    }
    bg_form->GetPutPosXY("info_msg", xy[0], xy[1]);
    form = MenuMesForm[3];
    if (form != NULL) {
        form->x = (float)xy[0];
        form->y = (float)xy[1];
    }
    bar[0] = bg_form->GetPartInfo("bar0");
    bar[1] = bg_form->GetPartInfo("bar1");
    bar[2] = bg_form->GetPartInfo("bar2");
    bg_form->GetPutPosXY("scrlbase", item_pos[0][0], item_pos[0][1]);
    MenuPosData->GetEtcTblValue("manualbarwh", scroll_range[0], scroll_range[1]);
    if (slide_in_flag == 0) {
        visible_top = top;
        LocalFunc_AdjustScrlBar(bar, &item_pos[0][0], scroll_range, visible_top, 46.0f, 10.0f,
                                slide_in);
    }
    if (LocalMenuClipForm != NULL) {
        LocalMenuBGForm->GetPutPosXY("msg_clip", xy[0], xy[1]);
        form = LocalMenuClipForm;
        form->x = (float)xy[0];
        form->y = (float)xy[1];
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", CalcCursorPosition__11CManualMenuFv);
int CMenuOption::KeyStep(void) {
    int finished;
    int move_type;
    int frame_end;
    int changed;
    int step;
    int keys;
    int pushed;
    int prev_choice;

    finished = 0;
    move_type = 2;
    if (mode == 2) {
        move_type = 0;
    }
    if (MenuArg.open_type == MENU_OPEN_OPTION) {
        move_type = 2;
    }
    MenuPosData->StepMainMenuIconMove(GetCommonMenuModeID(), 7, move_type);
    frame_end = GetMenuMainFrameEndFlag();
    switch (mode) {
        case 1:
            if (this->step == 0 && ReadBGSync() == 0 && frame_end != 0) {
                GamePad.KeyLock(0);
                MenuCommonInfo->SetWakuType(0);
                MenuCommonInfo->SetMoveMethod(2);
                cursor_jump = 1;
                mode = 0;
                ExeScript("INITEND");
                if (MenuArg.open_type == MENU_OPEN_OPTION) {
                    ExeScript("TITLE_INITEND");
                    this->step = 1;
                    mode = 1;
                }
            }
            if (this->step == 1 && FadeCheckMenu() != 0) {
                this->step = 0;
                mode = 0;
            }
            break;
        case 2:
            if (MenuArg.open_type == MENU_OPEN_OPTION) {
                if (FadeCheckMenu() != 0) {
                    return 1;
                }
                break;
            }
            if (frame_end != 0) {
                ExeScript("\217I\227\271\217\210\227\235");
                if (MenuConfigPtr != NULL) {
                    printf("cam_ctrl[0]  : %d\n", MenuConfigPtr->eye_reverse);
                    printf("cam_ctrl[1]  : %d\n", MenuConfigPtr->unk_37);
                    if (MenuConfigPtr->sound_mode != 0) {
                        CSnd.SetStereoMode(0);
                    } else {
                        CSnd.SetStereoMode(1);
                    }
                }
                MenuCommonInfo->SetMoveMethod(2);
                finished = 1;
            }
            break;
        case 0:
            changed = 0;
            MenuCommonInfo->CheckSelectKey();
            pushed = MenuCommonInfo->CheckPushButton();
            keys = MenuCommonInfo->CheckLRKey();
            step = MenuListSelectKeyCheck(keys, 9);
            if (MenuKeySelectCheck(step, &select, &top, 0, (int)config_option_num_i, 9, 0) != 0) {
                changed = 1;
            }
            prev_choice = choice;
            if (keys & 4) {
                choice = prev_choice - 1;
            }
            if (keys & 8) {
                choice += 1;
            }
            if (choice < 0) {
                choice = 0;
            }
            if (choice_num[select] <= choice) {
                choice = choice_num[select] - 1;
            }
            if (prev_choice != choice) {
                changed = 1;
            }
            if (changed != 0) {
                MenuSePlay(0);
            }
            switch (ConvertCheckPushButton(pushed)) {
                case 1:
                    if (select == 12) {
                        config.caption_off = choice;
                    } else if (select == 13) {
                        config.unk_35 = choice;
                    } else if (select == 14) {
                        config.eye_reverse = choice;
                    } else if (select == 15) {
                        config.unk_37 = choice;
                    } else if (select != 8 || config.enemy_hp != 1) {
                        *value[select] = choice;
                        if (select == 7 && choice == 1) {
                            *value[8] = 1;
                        }
                    }
                    UpdateOptionForm();
                    MenuSePlay(1);
                    break;
                case 8:
                    MenuSePlay(1);
                    InitSV_CONFIG_OPTION(&config);
                    UpdateOptionForm();
                    break;
                case 2:
                    mode = 2;
                    memcpy(MenuConfigPtr, &config, sizeof(SV_CONFIG_OPTION));
                    MenuCommonInfo->SetMoveMethod(-1);
                    MenuCommonInfo->SetWakuType(-1);
                    if (MenuArg.open_type == MENU_OPEN_OPTION) {
                        ExeScript("PREEND_T");
                    } else {
                        MenuMainFrameModeSet(9, 0);
                        ReturnMenuIntern(0);
                        MenuCommonInfo->SetWakuType(-1);
                        ExeScript("PREEND");
                    }
                    break;
            }
            break;
    }
    CalcTex();
    MenuPosData->FormStep();
    {
        int pos[2] = {0, 0};
        int size[2] = {0, 0};
        int velocity[2] = {-48, 0};
        int limit[2];
        char name[0x20];
        int row_no;
        int choice_no;

        switch (mode) {
            case 1:
                if (MenuArg.open_type != MENU_OPEN_OPTION) {
                    break;
                }
            case 0:
                row_no = select;
                choice_no = choice;
                if (row_no < 10) {
                    sprintf(name, "INDEX0%d%d", row_no, choice_no);
                } else {
                    sprintf(name, "INDEX%d%d", row_no, choice_no);
                }
                OptionButtonForm->GetPutPosXY(name, pos[0], pos[1]);
                pos[0] -= 2;
                pos[1] -= 3;
                sprintf(name, "op_waku%d", select);
                MenuPosData->GetEtcTblValue(name, size[0], size[1]);
                MenuCommonInfo->SetWakuWH(0, size[0], size[1]);
                MenuPosData->GetEtcTblValue("op_cursorlimmit", limit[0], limit[1]);
                if (pos[1] < limit[0]) {
                    pos[1] = limit[0];
                }
                if (limit[1] < pos[1]) {
                    pos[1] = limit[1];
                }
                MenuCommonInfo->MenuPosStep(&pos[0], &velocity[0]);
                break;
        }
        if (cursor_jump != 0) {
            MenuCommonInfo->MenuSetPos(pos[0], pos[1]);
            cursor_jump = 0;
        }
    }
    return finished;
}

void CMenuOption::CalcTex(void) {
    CMenuPosDataForm *bg_form;
    int slide_in;
    int slide_in_flag;
    float *left_top;
    int xy[2];
    float speed;
    float target;
    s8 *list_tbl;
    int list;
    CMenuPosDataForm *mes_form;
    CMenuPosDataForm *clip_form;
    int item_pos[10][2];
    int item;
    CMenuPosDataForm *form;
    MENUFORMPARTS_TYPE *bar[3];
    int scroll_range[2];

    bg_form = LocalMenuBGForm;
    if (bg_form == NULL) {
        return;
    }
    slide_in = 0;
    slide_in_flag = 0;
    left_top = GetMenuMainFrameLeftTopPos(0);
    xy[0] = (int)(left_top[0]);
    xy[1] = (int)(480.0f + left_top[1]);
    if (GetMenuMainFrameEndFlag() == 0) {
        slide_in = 1;
        slide_in_flag = 1;
        bg_form->x = (float)xy[0];
        bg_form->y = (float)xy[1];
    } else {
        bg_form->x = (float)xy[0];
    }
    bg_form->GetPutPosXY("base_msg", xy[0], xy[1]);
    speed = 3.5f;
    if (mode == 2) {
        speed = 2.0f;
    }
    target = (float)(xy[1] - top * 24);
    CalcMenu1(target, &list_y, speed, speed, 0);
    if ((float)xy[1] < list_y) {
        list_y = target;
    }
    xy[1] = (int)(list_y);
    list_tbl = manual_list_mesclstbl;
    for (list = 0; list < 3; list++) {
        for (item = 0; item < 10; item++) {
            item_pos[item][0] = xy[0];
            item_pos[item][1] = xy[1];
            xy[1] += 24;
        }
        MenuDCMsg[list_tbl[list]]->SetMsgItemPos(&item_pos[0][0], 10);
    }
    form = OptionButtonForm;
    if (form != NULL) {
        xy[1] = (int)(list_y - 3.0f);
        form->x = (float)xy[0];
        form->y = (float)xy[1];
    }
    bg_form->GetPutPosXY("info_msg", xy[0], xy[1]);
    mes_form = MenuMesForm[3];
    if (mes_form != NULL) {
        mes_form->x = (float)xy[0];
        mes_form->y = (float)xy[1];
    }
    bar[0] = bg_form->GetPartInfo("bar0");
    bar[1] = bg_form->GetPartInfo("bar1");
    bar[2] = bg_form->GetPartInfo("bar2");
    bg_form->GetPutPosXY("scrlbase", item_pos[0][0], item_pos[0][1]);
    MenuPosData->GetEtcTblValue("manualbarwh", scroll_range[0], scroll_range[1]);
    if (slide_in_flag == 0) {
        LocalFunc_AdjustScrlBar(
            bar, &item_pos[0][0], scroll_range, top, config_option_num_f, 9.0f, slide_in);
    }
    if (LocalMenuClipForm != NULL) {
        bg_form->GetPutPosXY("msg_clip", xy[0], xy[1]);
        clip_form = LocalMenuClipForm;
        clip_form->x = (float)xy[0];
        clip_form->y = (float)xy[1];
    }
}

void CMenuOption::DefaultButton(MENUFORMPARTS_TYPE **buttons) {
    int i;
    for (i = 0; i < 3; i++) {
        if (buttons[i] != NULL) {
            buttons[i]->rgba[0] = 0x40;
            buttons[i]->rgba[1] = 0x40;
            buttons[i]->rgba[2] = 0x40;
        }
    }
}

void CMenuOption::EnableButton(MENUFORMPARTS_TYPE *button) {
    button->rgba[0] = 0x80;
    button->rgba[1] = 0x80;
    button->rgba[2] = 0x80;
}
void CMenuOption::UpdateOptionForm(void) {
    SV_CONFIG_OPTION *config;
    MENUFORMPARTS_TYPE **row;
    int i;

    if (OptionButtonForm != NULL) {
        config = &this->config;
        if (config != NULL) {
            row = this->button[0];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->cursor_save]);
            }
            row = this->button[1];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->vibration]);
            }
            row = this->button[2];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->message_speed]);
            }
            row = this->button[3];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->sound_mode]);
            }
            row = this->button[4];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->fast_time]);
            }
            row = this->button[5];
            if (row != NULL) {
                DefaultButton(row);
                for (i = 0; i < OPTION_BUTTON_NUM; i++) {
                    if (config->map == i) {
                        EnableButton(row[i]);
                    }
                }
            }
            row = this->button[6];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->damage_off]);
            }
            row = this->button[7];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->enemy_hp]);
            }
            row = this->button[8];
            if (row != NULL) {
                DefaultButton(row);
                if (config->enemy_hp == 0) {
                    EnableButton(row[config->anger_counter]);
                }
            }
            row = this->button[9];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->unk_24]);
            }
            row = this->button[10];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->monster_name]);
            }
            row = this->button[11];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->dof_off]);
            }
            row = this->button[12];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->caption_off]);
            }
            row = this->button[13];
            if (row != NULL) {
                DefaultButton(row);
                EnableButton(row[config->unk_35]);
            }
            row = this->button[14];
            if (LanguageCode > 0) {
                if (row != NULL) {
                    DefaultButton(row);
                    EnableButton(row[config->eye_reverse]);
                }
                row = this->button[15];
                if (row != NULL) {
                    DefaultButton(row);
                    EnableButton(row[config->unk_37]);
                }
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", MenuOptionInit__FP9mgCMemoryPii);
int MenuOptionKey() {
    return CMenuOptionPtr->KeyStep();
}

void MenuOptionDraw() {
    MenuPosData->FormDraw();
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", LocalFunc_AdjustScrlBar__FPP18MENUFORMPARTS_TYPEPiPiiffi);
void CSaveMenuClass::SetDlInfoMsg(int load, int show) {
    int message_no = 0xC08;
    if (load == 1) {
        message_no = 0xC09;
    }
    MenuDCMsg[7]->MakeMsg(message_no);
    MenuDCMsg[7]->StepMsg();
    CMenuPosDataForm *form;
    (form = MenuMesForm[7])->x = (mgScreenWidth - MenuDCMsg[7]->line_w[0]) >> 1;
    form->y = 168.0f;
    MenuMesForm[7]->draw_flag = show != 0;
}

void CSaveMenuClass::EnvSetSave(int kind) {
    CDC2Mes *save_mes;
    int func_no;
    int size_kind;
    int data_size;

    dl_base = 0;
    if (kind == 0) {
        func_no = 6;
        phase = 2;
        size_kind = 1;
    } else {
        func_no = 3;
        phase = 0xA;
        size_kind = 0;
    }
    MemoryCardPtr->file_no = select;
    MemoryCardPtr->SetFuncNo(func_no);
    data_size = MemoryCardPtr->GetSaveDataSize(size_kind);
    if (kind == 1) {
        data_size -= 0x1000;
    }
    save_mes = MenuDCMsg[2];
    save_mes->MsgPreset(0xA, LanguageCode);
    save_mes->push_button = 0;
    save_mes->SetAbsPos(8);
    save_mes->MakeMsg(0xBBF);
    save_mes->SetMsgVolumeNoOne(slot + 1);
    if (cursor_form != NULL) {
        cursor_form->draw_flag = 0;
    }
    InitMenuDl(dl_tex, data_size);
    SetDlInfoMsg(0, 1);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", KeyStep__14CSaveMenuClassFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", SaveFileListDraw__FRiPfi);
/** Loads the three save icons and gives them to the memory card manager. */
static void SetMCIconData(u_int *pack, int slot) {
    MC_ICON_DATA icons[3] = {
        {"dc2_ic.ico", NULL, 0},
        {"dc2_ic_c.ico", NULL, 0},
        {"dc2_ic_d.ico", NULL, 0}
    };
    for (int i = 0; i < 3; i++) {
        MC_ICON_DATA *icon = &icons[i];
        icon->data = GetPackFile(pack, icon->name, &icon->size);
    }
    MemoryCardPtr->SetIconData(icons, slot);
}

int GetDngMapNo(int dng_no) {
    static char *dngmap[7] = {
        "d01f01",
        "d02f01",
        "d03f01",
        "d04f01",
        "d05f01",
        "d06f01",
        "d07f01"
    };
    if (dng_no < 0) {
        return 0;
    }
    if (dng_no > 6) {
        return 0;
    }
    return SearchMapNo(dngmap[dng_no]);
}

void SaveMapInfo(int dng_no) {
    memcpy(MenuMapInfoSave, &GetSaveData()->map_no, sizeof(MenuMapInfoSave));
    int saved_dungeon = GetDungeonSaveData(GetSaveData())->stage_id;
    MenuMapInfoSave_DngNo = saved_dungeon;
    if (0 <= dng_no) {
        short *map_info = &GetSaveData()->map_no;
        CSaveData *save_data = GetSaveData();
        short previous = *map_info;
        save_data->prev_map_no = previous;
        map_info = &GetSaveData()->map_no;
        *map_info = GetDngMapNo(dng_no);
        saved_dungeon = GetDungeonSaveData(GetSaveData())->stage_id;
        MenuMapInfoSave_DngNo = saved_dungeon;
        GetDungeonSaveData(GetSaveData())->stage_id = dng_no;
    }
}

void ResetMapInfo() {
    memcpy(&GetSaveData()->map_no, MenuMapInfoSave, sizeof(MenuMapInfoSave));
    int dungeon_no = MenuMapInfoSave_DngNo;
    GetDungeonSaveData(GetSaveData())->stage_id = dungeon_no;
}
#ifdef NONMATCHING
void MenuSaveInit(mgCMemory *stack, int *tex_block, int open_type) {
    static char *b[3] = {
        "bar0",
        "bar1",
        "bar2"
    };
    CSaveMenuClass *menu;
    CMemoryCardManager *card;
    u8 *pack;
    unsigned int script_size;
    unsigned int blocks;
    short *main_messages;
    CDC2Mes *window;
    int mes_layout;
    int i;
    int part;
    int data_size;
    int free_size;

    free_size = stack->stGetRest();
    SaveMenuStack.stSetBuffer(stack->stGetTop(), free_size);
    menu = new (SaveMenuStack.Alloc(0x1D)) CSaveMenuClass;
    SaveMenuPtr = menu;
    card = new (SaveMenuStack.Alloc(0x112)) CMemoryCardManager;
    MemoryCardPtr = card;
    InitMenuReturnMsg(&SaveMenuStack);
    SetMenuReturnMsgCtrl(1);
    MemoryCardPtr->Initialize(&SaveMenuStack);
    MemoryCardPtr->SetBuff_Album(NULL);
    if (MemoryCardPtr->InitForMC() == 0) {
        SaveMenuStack.stAlloc64(0x800);
        SaveMenuPtr->SetTexBlock(tex_block);
        if (open_type == 7) {
            SaveMenuPtr->mode = 0;
        }
        if (open_type == 8) {
            SaveMenuPtr->mode = 1;
        }
        if (open_type == 0x1E) {
            SaveMenuPtr->mode = 2;
        }
        SaveMenuStack.Align64();
        pack = (u8 *)(SaveMenuStack.stack + SaveMenuStack.stack_used);
        script_size = LoadFileMenu("save.pac", (u_long128 *)pack, 1);
        blocks = (script_size & 0xF) != 0 ? (script_size >> 4) + 1 : script_size >> 4;
        SaveMenuStack.Alloc(blocks);
        mgTexManager.EnterIMGFile((u_char *)GetPackFile((unsigned int *)pack, "img.img", NULL),
                                  SaveMenuPtr->tex_block[1],
                                  NULL, NULL);
        mgTexManager.EnterIMGFile((u_char *)GetPackFile((unsigned int *)pack, "frametex.img", NULL),
                                  SaveMenuPtr->tex_block[1],
                                  NULL, NULL);
        Tex_SaveFile = mgTexManager.GetTexture("save", -1);
        SaveMenuPtr->dl_tex = GetMenuDlTexture();
        InitMenuDl(NULL, 0);
        main_messages = GetMenuMainMessageBuffer();
        window = MenuDCMsg[2];
        window->SetMessData(GetSystemMesBuffer(), main_messages);
        window = MenuDCMsg[3];
        window->SetMessData(main_messages, main_messages);
        window->MsgPreset(0x10, LanguageCode);
        window->MakeMsg(0xC1C);
        window = MenuDCMsg[4];
        window->SetMessData(main_messages, main_messages);
        window->MsgPreset(0x10, LanguageCode);
        window->MakeMsg(0xC1F);
        window = MenuDCMsg[5];
        window->SetMessData(main_messages, main_messages);
        window->MsgPreset(0x10, LanguageCode);
        window->MakeMsg(0xC20);
        window = MenuDCMsg[6];
        window->SetMessData(main_messages, main_messages);
        window->MsgPreset(0x10, LanguageCode);
        if (SaveMenuPtr->mode == 0) {
            window->MakeMsg(0xC3B);
        } else {
            window->MakeMsg(0xC3A);
        }
        MenuDCMsg[7]->SetMessData(main_messages, main_messages);
        MenuDCMsg[7]->MsgPreset(0x10);
        mes_layout = -4;
        if (CheckNowEurope() != 0) {
            mes_layout = 0;
        }
        for (i = 0; i < 13; i++) {
            window = new (SaveMenuStack.Alloc(0x2A7)) CDC2Mes;
            SaveFileList[i] = window;
            SaveFileList[i]->SetMessData(main_messages, main_messages);
            SaveFileList[i]->MsgPreset(0x10);
            SaveFileList[i]->value_zero = 1;
            SaveFileList[i]->value_space = mes_layout;
        }
        SetMCIconData((unsigned int *)pack, 2);
        char *data = (char *)GetPackFile((unsigned int *)pack, "save.cfg", &data_size);
        MenuDataAnalyze(data, data_size, &SaveMenuStack);
        SaveMenuPtr->script =
            (char *)GetPackFile((unsigned int *)pack, "save_com.cfg", &SaveMenuPtr->script_size);
        MenuPosData->AttachCommonTexInfo();
        MenuPosData->ResetTextureBlockNo("save", SaveMenuPtr->tex_block[1]);
        MenuPosData->ResetTextureInfoAll();
        SaveMenuPtr->title_form = MenuPosData->GetFormInfo("TITLE");
        SaveMenuPtr->slot_form[0] = MenuPosData->GetFormInfo("SLOT1");
        SaveMenuPtr->slot_form[1] = MenuPosData->GetFormInfo("SLOT2");
        SaveMenuPtr->cursor_form = MenuPosData->GetFormInfo("CURSOR");
        SaveMenuPtr->list_form = MenuPosData->GetFormInfo("LIST");
        SaveMenuPtr->scrlbar_form = MenuPosData->GetFormInfo("SCRLBAR");
        part = 0;
        if (SaveMenuPtr->scrlbar_form != NULL) {
            for (; part < 3; part++) {
                SaveMenuPtr->scrlbar_parts[part] =
                    SaveMenuPtr->scrlbar_form->GetPartInfo(b[part]);
            }
        }
        AttachMessageForm();
        SaveMenuStack.Align64();
        if (SaveMenuPtr->mode == 0) {
            StopEnvSoundMenu(1);
            MenuMainScene->GetActiveBgmStatus(&SaveMenuPtr->bgm_status);
            MenuMainScene->StopBGM(0);
            MenuMainScene->LoadBGM(0x30,
            (u_long128 *)(SaveMenuStack.stack + SaveMenuStack.stack_used));
            MenuMainScene->PlayBGM(0, -1, 1.0f);
        }
        SaveMenuPtr->FadeInMenu(0x3C, 0.0f);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", MenuSaveInit__FP9mgCMemoryPii);
#endif
int MenuSaveKey() {
    return SaveMenuPtr->KeyStep();
}

void MenuSaveDraw() {
    char text[0x200];
    MenuPosData->FormDraw();
    DrawMenuReturnMsg();
    if (DebugFlag != 0 && menu_debug_flag != 0) {
        CMenuFont menu_font;
        int mode = SaveMenuPtr->mode;
        if (mode == 1 || mode == 2) {
            menu_font.SetStr("MODE STATE : LOAD");
            menu_font.SetPos(0x14, 0x28);
            menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        } else {
            menu_font.SetStr("MODE STATE : SAVE");
            menu_font.SetPos(0x14, 0x28);
            menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        }
        sprintf(text, "SAVEFILE TOTALSIZE : %d K\n", MemoryCardPtr->GetSaveDataSize(0) / 1024);
        menu_font.SetStr(text);
        menu_font.SetPos(0x14, 0x3C);
        menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        sprintf(text, "          SAVEDATA : %d K\n", 0x196);
        menu_font.SetStr(text);
        menu_font.SetPos(0x14, 0x50);
        menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        sprintf(text, "         ALBUMDATA : %d K\n", MemoryCardPtr->GetSaveDataSize(5));
        menu_font.SetStr(text);
        menu_font.SetPos(0x14, 0x64);
        menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        sprintf(text, "          ICONDATA : %d K\n", MemoryCardPtr->GetIconDataSize() / 1024);
        menu_font.SetStr(text);
        menu_font.SetPos(0x14, 0x78);
        menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
        MC_CARD_INFO *card;
        int port = MemoryCardPtr->port;
        if (port == 0 || port == 1) {
            card = &MemoryCardPtr->card[port];
        } else {
            card = NULL;
        }
        sprintf(text, "Slot 0\nType:%d\nformat:%d\nclusta%d\n", card->type,
            card->formatted, card->free_size);
        menu_font.SetStr(text);
        menu_font.SetPos(0x14, 0x8C);
        menu_font.DrawDirect(menu_font.str, menu_font.pos_x, menu_font.pos_y);
    }
}

/** Runs a command in the mini-game save layout. */
static void SubGameCFGAnalyze(char *command) {
    MenuCommandAnalyze(SubGameSaveCFGBuffer, SubGameSaveCFGBufferSize, command);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", SubGameSaveInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuop", SubGameSaveKey__Fv);
void SubGameSaveDraw() {
    mgCTextureManager *textures = &mgTexManager;
    textures->ReloadTexture(SubGameSaveBlock, (sceVif1Packet *)NULL);
    if (Tex_SaveFile != NULL) {
        mgCDrawPrim *prim = GetMenuPrim();
        mgRect<int> rect;
        rect.Set(0x100, 0x100, 0x100, 0x100);
        DrawMenuTilePattern(prim, Tex_SaveFile, SubSaveTileXY, SubSaveTileXY, rect, 0, NULL);
    }
    textures->ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)NULL);
    MenuDCMsg[0]->DrawMsg();
}

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", manual_boot_event_no__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", submap_table_1022__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", fillw_1125__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1315__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", tp_2083__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", conv_2316__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2609__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", dngmap_2627__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", b_2715__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1023__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1024__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1025__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1026__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1027__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1028__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1029__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1030__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1031__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1032__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1033__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1034__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1035__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1036__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1037__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1038__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1039__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1040__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1041__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1042__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1043__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1102__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1103__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1104__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1105__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1106__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1107__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1108__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1109__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1237__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1238__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1428__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1429__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1430__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1431__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1432__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1433__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1434__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1435__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1436__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1437__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1438__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1439__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1440__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1441__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1512__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1513__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1514__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1515__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1516__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1517__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1518__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1519__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1648__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1649__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1650__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1651__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1652__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1653__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1654__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1655__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1656__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1900__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1901__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1902__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1903__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1904__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1905__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1906__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2024__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2025__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2084__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2085__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2086__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2498__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2499__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2500__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2501__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2502__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2503__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2504__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2505__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2506__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2507__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2508__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2509__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2510__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2511__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2512__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2513__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2514__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2515__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2518__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2517__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2603__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2604__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2605__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2628__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2629__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2630__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2631__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2632__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2633__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2634__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2764__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2765__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2766__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2767__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2768__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2769__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2770__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2771__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2772__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2773__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2774__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2775__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2821__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2822__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2823__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2824__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2825__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2826__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2827__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2895__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3198__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3199__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3200__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3201__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3202__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3203__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3204__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3205__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3206__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_3207__DATA);

// Static initialiser table (.ctor)

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", __vt__14CSaveMenuClass__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", __vt__11CMenuOption__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", __vt__11CManualMenu__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", manual_list_mesclstbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", config_option_num_i__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", config_option_num_f__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_1616__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", tbl_2023__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2335__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuop", at_2342__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(ManualMovie, 0x4);
INCLUDE_BSS(ManualMovieTex, 0x4);
INCLUDE_BSS(LocalMenuBGForm, 0x4);
INCLUDE_BSS(LocalMenuClipForm, 0x4);
INCLUDE_BSS(MenuReturnMsg, 0x4);
INCLUDE_BSS(MenuReturnMsgDrawFlag, 0x4);
INCLUDE_BSS(MovieBattleBGMPhase, 0x4);
INCLUDE_BSS(MoviePreBattleBGMVol_Save, 0x4);
INCLUDE_BSS(MoviePreBattleBGMVol, 0x4);
INCLUDE_BSS(Movie_DungeonFlag, 0x4);
INCLUDE_BSS(Movie_BossFlag, 0x4);
INCLUDE_BSS(MovieBgmBattleCheckStopFlag, 0x4);
INCLUDE_BSS(CManualPtr, 0x4);
INCLUDE_BSS(MovieViewFlag, 0x4);
INCLUDE_BSS(ManualMovieFadeCount_1253, 0x4);
INCLUDE_BSS(init_1254, 0x8);
INCLUDE_BSS(at_1306__5, 0x8);
INCLUDE_BSS(at_1342__3, 0x8);
INCLUDE_BSS(at_1523__2, 0x8);
INCLUDE_BSS(OptionButtonForm, 0x8);
INCLUDE_BSS(at_1614__2, 0x8);
INCLUDE_BSS(at_1615__3, 0x8);
INCLUDE_BSS(CMenuOptionPtr, 0x4);
INCLUDE_BSS(MemoryCardPtr, 0x4);
INCLUDE_BSS(SaveMenuPtr, 0x4);
INCLUDE_BSS(FormatCase_1968, 0x4);
INCLUDE_BSS(init_1969, 0x4);
INCLUDE_BSS(DarkClonicleFileMax_2004, 0x4);
INCLUDE_BSS(init_2005, 0x4);
INCLUDE_BSS(input_wait_counter_2067, 0x4);
INCLUDE_BSS(init_2068, 0x8);
INCLUDE_BSS(at_2115__3, 0x8);
INCLUDE_BSS(at_2276, 0x8);
INCLUDE_BSS(at_2319, 0x4);
INCLUDE_BSS(at_2326__2, 0x4);
INCLUDE_BSS(at_2327, 0x4);
INCLUDE_BSS(at_2328__2, 0x4);
INCLUDE_BSS(at_2330__2, 0x4);
INCLUDE_BSS(at_2331__2, 0x4);
INCLUDE_BSS(Tex_SaveFile, 0x4);
INCLUDE_BSS(space_2549, 0x4);
INCLUDE_BSS(init_2550, 0x4);
INCLUDE_BSS(MenuMapInfoSave_DngNo, 0x4);
INCLUDE_BSS(SubGameSaveOrLoad, 0x4);
INCLUDE_BSS(SubGameSaveOrLoadPhase, 0x4);
INCLUDE_BSS(SubGameSaveLoadStatus, 0x4);
INCLUDE_BSS(SubGameMCPort, 0x4);
INCLUDE_BSS(SubGameSaveBlock, 0x8);
INCLUDE_BSS(SubTrueTotalSaveFileSize, 0x4);
INCLUDE_BSS(SubCheckTotalSaveFileSize, 0x4);
INCLUDE_BSS(SubSaveTileXY, 0x4);
INCLUDE_BSS(SubGameSaveCFGBuffer, 0x4);
INCLUDE_BSS(SubGameSaveCFGBufferSize, 0x8);
INCLUDE_BSS(at_3070, 0x8);
INCLUDE_BSS(at_3091, 0x8);

// Uninitialised data (.bss)
INCLUDE_BSS(MnOnePictTex, 0x20);
INCLUDE_BSS(StaticMenuLocalStack, 0x30);
INCLUDE_BSS(StaticMenuLocalStack2, 0x30);
INCLUDE_BSS(SaveMenuStack, 0x30);
INCLUDE_BSS(SaveFileList, 0x38);
INCLUDE_BSS(MenuMapInfoSave, 0x18);
INCLUDE_BSS(SubGameDataBgm, 0x20);
