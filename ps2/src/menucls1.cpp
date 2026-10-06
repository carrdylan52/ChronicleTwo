#include "common.h"
#include "menucls1.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "mglib.hpp"
#include <cmath>
#include <cstring>

// Preserve divide-by-zero traps in the decimal digit conversions.
#pragma divbyzerocheck on

// Full-width digit strings used by the menu font.
char *MenuBigNum[10] = {"\202O", "\202P", "\202Q", "\202R", "\202S", "\202T", "\202U", "\202V", "\202W", "\202X"};

CItemUseTarget MenuUsedTarget;

// Code (.text)
char *GetHatena() {
    static char *full_width = "\201H\201H\201H";
    static char *half_width = "???";
    if (LanguageCode >= 2 && LanguageCode < LANG_CHINESE) {
        return half_width;
    }
    return full_width;
}

char *GetMenuBigNum(int num) {
    return MenuBigNum[num % 10];
}

void SetMenuBigNum2(char *buff, int num) {
    int rest;
    if (buff != NULL) {
        rest = num;
        int pos = 0;
        int digits = GetNumberKeta(num);
        if (digits > 0) {
            for (;;) {
                char *glyph;
                if (digits == 1) {
                    glyph = GetMenuBigNum(rest);
                } else {
                    int divisor = (int)pow(10.0, (double)(digits - 1));
                    glyph = GetMenuBigNum(rest / divisor);
                    rest = rest % divisor;
                }
                char *dst = &buff[pos];
                digits -= 1;
                pos += 2;
                dst[0] = glyph[0];
                dst[1] = glyph[1];
                if (digits <= 0) {
                    break;
                }
            }
        }
        buff[pos] = 0;
    }
}

void SetMenuBigNum(char *buff, int num) {
    int rest;
    static char *digits_half[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
    if (buff != NULL) {
        rest = num;
        int pos = 0;
        int digits = GetNumberKeta(num);
        if (CheckNowEurope() != 0) {
            if (digits > 0) {
                do {
                    char *glyph;
                    if (digits == 1) {
                        glyph = digits_half[rest];
                    } else {
                        double exponent = (double)(digits - 1);
                        int divisor = (int)pow(10.0, exponent);
                        glyph = digits_half[rest / divisor];
                        rest = rest % divisor;
                    }
                    char digit = glyph[0];
                    digits -= 1;
                    buff[pos++] = digit;
                } while (digits > 0);
            }
        } else if (digits > 0) {
            for (;;) {
                char *glyph;
                if (digits == 1) {
                    glyph = GetMenuBigNum(rest);
                } else {
                    double base = 10.0;
                    const double &reference = base;
                    double exponent = (double)(digits - 1);
                    int divisor = (int)pow(reference, exponent);
                    glyph = GetMenuBigNum(rest / divisor);
                    rest = rest % divisor;
                }
                char *dst = &buff[pos];
                digits -= 1;
                pos += 2;
                dst[0] = glyph[0];
                dst[1] = glyph[1];
                if (digits <= 0) {
                    break;
                }
            }
        }
        buff[pos] = 0;
    }
}

CMenuFont::CMenuFont() {
    Init();
    SetClearance(16, 20);
    SetFuchi(5);
    SetColor(0x80686A6BU);
    unk_b0 = 0.0f;
    unk_b4 = 0.0f;
}

void MenuMesInit(ClsMes *mes) {
    int page, name, item, value, line;
    if (mes != NULL) {
        mes->npc_name_mode = 0;
        mes->char_num = 0;
        mes->text_w = 0;
        mes->text_h = 0;
        mes->page = 0;
        mes->page_num = 0;
        for (page = 0; page < MES_PAGE_MAX; page++) {
            mes->page_chars[page] = 0;
        }
        mes->last_x = 0;
        mes->last_y = 0;
        mes->fade = 0.0f;
        mes->open = 1;
        mes->draw_speed = mes->GetDrawSpeedDef();
        mes->page_wait = 0;
        mes->scroll_wait = 0;
        mes->reveal = 0.0f;
        mes->reveal_num = 0;
        mes->page_top = 0;
        mes->unk_1f4 = 0;
        mes->InitMesWinTbl();
        mes->color = mes->def_color;
        mes->wait = 0;
        mes->page_time = 0;
        mes->page_auto_time = 30;
        mes->mes_no = -1;
        mes->unk_1e40 = 0;
        mes->alpha = 0x80;
        for (name = 0; name < MES_NAME_MAX; name++) {
            memset(mes->name[name], 0, sizeof(mes->name[name]));
        }
        for (item = 0; item < MES_ITEM_MAX; item++) {
            mes->item_mes[item] = -1;
        }
        for (value = 0; value < MES_VALUE_MAX; value++) {
            mes->values[value] = 0;
            mes->value_width[value] = 0;
        }
        mes->value = 0;
        mes->value_sign = 0;
        mes->value_zero = 1;
        mes->value_half = 0;
        mes->value_space = 0;
        mes->digit_font = 0;
        mes->space_w = -1;
        mes->justify_w = -1;
        mes->select = -1;
        mes->goal_cursor_x = 0;
        mes->goal_cursor_y = 0;
        mes->cursor_x = 0;
        mes->cursor_y = 0;
        mes->select_shade = 0;
        mes->cursor_centering = 0;
        mes->cursor_time = 0;
        mes->choice_pos[0][0] = -1;
        mes->choice_pos[0][1] = -1;
        mes->choice_pos[1][0] = -1;
        mes->choice_pos[1][1] = -1;
        mes->select_top = 0;
        mes->cursor_off_y = 0;
        mes->voice_on = 0;
        mes->voice_type = 0;
        mes->voice_cnt = 0;
        mes->close_time = 0;
        mes->scissor_on = 0;
        mes->scissor.x = 0;
        mes->scissor.width = 0;
        mes->scissor.y = 0;
        mes->scissor.height = 0;
        for (line = 0; line < MES_LINE_MAX; line++) {
            mes->line_indent[line] = 0;
            mes->line_pos[line][0] = 0;
            mes->line_pos[line][1] = 0;
            mes->line_pos_on[line] = 0;
            mes->line_shade[line] = -1;
            mes->line_color[line] = 0;
            mes->equip_on[line] = 0;
            mes->equip_x[line] = 0;
            mes->equip_y[line] = 0;
            mes->line_w[line] = 0;
            mes->line_alpha[line] = -1;
            mes->cross_on[line] = 0;
            mes->cross_x[line] = 0;
            mes->cross_y[line] = 0;
            mes->unk_271c[line] = -1;
            mes->unk_276c[line] = -1;
            mes->unk_27bc[line] = 0;
            mes->unk_280c[line] = 0;
            mes->delta_on[line] = 0;
            mes->delta_x[line] = 0;
            mes->delta_y[line] = 0;
        }
        mes->abs_win.x = -1;
        mes->abs_win.y = -1;
        mes->abs_win.width = -10;
        mes->abs_win.height = -10;
        mes->abs_text_off_x = -1;
        mes->abs_text_off_y = -1;
        mes->fukidashi_pos = -1;
        mes->value_zero = 0;
        mes->rows = 5;
        mes->draw_speed = 0.0f;
        mes->draw_speed_def = 0.0f;
        mes->push_button = 0;
        mes->tail_on = 0;
        mes->fade_speed = 1.0f;
        mes->fuchi = 5;
        mes->SetHalfFontWPercent(-1.0f);
        mes->font_w = 15;
        mes->font_h = 24;
        if (LanguageCode > 0) {
            mes->font_w = 15;
            mes->font_h = 24;
        }
        mes->draw_off_x = 0.0f;
        mes->draw_off_y = 0.0f;
        mes->unk_b0 = 0.0f;
        mes->unk_b4 = 0.0f;
    }
}

CDC2Mes::CDC2Mes() {
    scissor.Set(0, 0, 0, 0);
    msg_change = 0;
    cursor = -1;
    text_off_x = -1;
    text_off_y = -1;
    mes_no = -1;
    cursor_on = 1;
    put_centering = 0;
    scissor_on = 0;
    scissor.Set(0, 0, 512, 415);
    memset(str, 0, sizeof(str));
}

void CDC2Mes::SetMessData(s16 *buff_system, s16 *buff) {
    SetBuff_system(buff_system);
    SetBuff(buff);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", MsgPreset__7CDC2MesFi);
void CDC2Mes::MsgPreset(int preset, int unused) {
    MsgPreset(preset);
    if (LanguageCode == LANG_ENGLISH) {
        value_half = 1;
    }
}

void CDC2Mes::SetMsgCursor(int cursor) {
    this->cursor = cursor;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", AddMsgCursor2__7CDC2MesFiii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", AddMsgCursor__7CDC2MesFiiii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", CommandMsgCursor__7CDC2MesFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", YesNoCursor__7CDC2MesFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", YesNoCursor2__7CDC2MesFi);
int CDC2Mes::GetMsgCursor() {
    return cursor;
}

int CDC2Mes::GetMsgItemNo(int index) {
    return item_mes[index];
}

void CDC2Mes::SetFontColor(s32 r, s32 g, s32 b, s32 a) {
    SetDefColor(r | (g << 8 | (a << 24 | b << 16)));
}

void CDC2Mes::SetPutPos(int x, int y, int w, int h) {
    abs_win.x = x;
    abs_win.y = y;
    abs_win.width = w;
    abs_win.height = h;
    if (put_centering != 0) {
        abs_win.x = (mgScreenWidth - text_w) >> 1;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetPutPos__7CDC2MesFPi);
void CDC2Mes::SetAbsPos(int pos) {
    fukidashi_pos = pos;
}

s32 CDC2Mes::GetStringDrawWidthDC(char *str) {
    s32 width = GetStrWidth(str);
    if (width >= 0) {
        return width;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMovePosCenteringGyou__7CDC2MesFiii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgItemNo__7CDC2MesFPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgItemNo__7CDC2MesFPPci);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgVolumeNo__7CDC2MesFPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgVolumeNo__7CDC2MesFPiPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgVolumeNoOne__7CDC2MesFi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMsgItemPos__7CDC2MesFPii);
void CDC2Mes::MakeMsg(s32 message_no) {
    mes_no = message_no;
    str[0] = 0;
}

void CDC2Mes::MakeMsg(char *str) {
    mes_no = -1;
    this->str[0] = 0;
    if (str != NULL) {
        strcpy(this->str, str);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", MakeMsg__7CDC2MesFP13CGameDataUsed);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", MakeMsg__7CDC2MesFP13CGameDataUsedP13CGameDataUsed);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", StepMsg__7CDC2MesFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", DrawMsg__7CDC2MesFv);
void CDC2Mes::SetMsgAlpha(int alpha) {
    this->alpha = alpha;
    if (alpha < 0) {
        this->alpha = 0;
    }
    if (alpha > 0x80) {
        this->alpha = 0x80;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", Initialize__13CMenuMoveItemFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", AttachForm__13CMenuMoveItemFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", CheckMove__13CMenuMoveItemFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", SetMoveItemInfo__13CMenuMoveItemFP19MENU_ITEM_MOVE_INFOPiPi);
int CheckRoboShieldKit(CUserDataManager *user, CGameDataUsed *target, int use, int *enable_num,
                       int *use_num) {
    if (target->item_type == 0xB) {
        int limit = GetShiledKitLimmit(target->item_no);
        ROBO_DATA *robo = &user->robo_data;
        if (robo == NULL) {
            return -1;
        }
        if (robo->shield_kit_num < limit) {
            *enable_num += 1;
            if (use != 0) {
                *use_num += 1;
                robo->shield_kit_num += 1;
                if (limit <= robo->shield_kit_num) {
                    robo->shield_kit_num = limit;
                }
                return 0xF;
            }
        }
    }
    return -1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menucls1", MenuUseItemCheckFunc__FP13CGameDataUsedP14CItemUseTargeti);
int CMenuItemUse::CheckItemUseEnable(CGameDataUsed *item, int target_type, void *target) {
    if (item == NULL || target == NULL) {
        return 0;
    }
    CItemUseTarget use_target;
    use_target.SetPtr(target_type, target);
    return MenuUseItemCheckFunc(item, &use_target, 0);
}

int CMenuItemUse::UseItem(CGameDataUsed *item, int target_type, void *target) {
    CItemUseTarget use_target(target_type, target);
    MenuUsedTarget.SetPtr(target_type, target);
    return UseItem(item, &use_target);
}

int CMenuItemUse::UseItem(CGameDataUsed *item, CItemUseTarget *target) {
    if (item == NULL) {
        return 0;
    }
    item_no = item->item_no;
    target_type = target->type;
    MenuUsedTarget.type = target->type;
    MenuUsedTarget.target.data = target->target.data;
    return MenuUseItemCheckFunc(item, target, 1);
}

void CMenuItemUse::Initialize(void) {
    item_no = 0;
    target_type = 0;
    unk_18 = 0;
}

s32 CheckNowStateUseThisItem(CGameDataUsed *item, CItemUseTarget *target) {
    return MenuUseItemCheckFunc(item, target, 0);
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", MenuBigNum__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", sn_944__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1415__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", st_bittable_1654__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_905__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_906__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_907__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_908__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_909__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_910__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_911__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_912__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_913__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_914__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_915__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_916__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_945__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_946__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_947__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_948__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_949__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_950__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_951__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_952__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_953__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_954__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1104__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1328__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1512__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1513__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1514__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1623__3__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menucls1", at_1371__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(MenuHatena_894, 0x4);
INCLUDE_BSS(init_895, 0x4);
INCLUDE_BSS(MenuHatena_1byte_897, 0x4);
INCLUDE_BSS(init_898, 0x4);
INCLUDE_BSS(at_1433__2, 0x4);
INCLUDE_BSS(MenuUsedItemNo, 0x4);
INCLUDE_BSS(MenuUsedItemType, 0x4);
INCLUDE_BSS(MenuUsedNotErrorCode, 0x4);
INCLUDE_BSS(MenuUsedTarget, 0x8);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1407__2, 0x10);
INCLUDE_BSS(at_1436__3, 0x18);
