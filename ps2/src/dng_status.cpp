#include "common.h"
#include "dng_status.hpp"

#include <cmath>

#include "actionchara.hpp"
#include "dng_main.hpp"
#include "maintex.hpp"
#include "prespr.hpp"
#include "scenesnd.hpp"
#include "subgame.hpp"
#include "userdata.hpp"

static void DrawActiveItemCursor(int x, int y, float fade);

// Trap on division by zero for variable integer divisors.
#pragma divbyzerocheck on

// Code (.text)
void PrintV(int x, int y, int value, mgCTexture *texture, mgRect<int> glyph, int digits,
            int align_right, int pitch, SP_RGBA *color) {
    int digit[8];
    int divisor = 1;
    int index;
    int shown;
    int power;
    int position;
    digit[0] = -1;
    digit[1] = -1;
    digit[2] = -1;
    digit[3] = -1;
    digit[4] = -1;
    digit[5] = -1;
    for (power = 0; power < digits - 1; power++) {
        divisor *= 10;
    }
    for (position = digits - 1; position >= 0; position--) {
        int digit_value = value / divisor;
        digit[position] = digit_value;
        value -= digit_value * divisor;
        divisor /= 10;
    }
    shown = digits;
    for (index = digits - 1; index > 0; index--) {
        if (digit[index] != 0) {
            break;
        }
        shown--;
    }

    CPreSprite sprite[2];
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Texture(texture);
    if (color != NULL) {
        sprite[0].Color(color->r, color->g, color->b, color->a);
    } else {
        sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    }
    if (pitch < 0) {
        pitch = glyph.right;
    }
    if (align_right != 0) {
        x += pitch * (digits - shown);
    }
    for (shown--; shown >= 0; shown--) {
        sprite[0].SetIRect(x, y, glyph.right, glyph.bottom + 1, glyph.left + glyph.right * digit[shown], glyph.top);
        x += pitch;
    }
    sprite[0].End();
}

void DrawDrumCounter(int x, int y, int value) {
    int digit[6];
    int index;

    index = 0;
    digit[index] = 0;
    digit[1] = 0;
    digit[2] = 0;
    digit[3] = 0;
    digit[4] = 0;
    digit[5] = 0;
    value -= (digit[index] = value / 10000) * 10000;
    value -= (digit[1] = value / 1000) * 1000;
    value -= (digit[2] = value / 100) * 100;
    value -= (digit[3] = value / 10) * 10;
    digit[4] = value;

    CPreSprite sprite[2];

    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    for (index = 0; index < 5; index++) {
        sprite[0].SetIRect(x, y, 12, 12, digit[index] * 12, 0xE8);
        x += 15;
    }
    sprite[0].End();
}

/**
 * Draws the rotating highlight around the active item slot with its fade alpha.
 */
static void DrawActiveItemCursor(int x, int y, float fade) {
    CPreSprite sprite[2];
    static float cur_ang;
    static char  init;
    float        position[4];
    float        horizontal;
    float        vertical;

    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Bilinear(1);
    sprite[0].Begin(MG_PRIM_TRIANGLE);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].SetAlphaBlend(MG_ALPHA_BLEND_ADD);
    sprite[0].Color(0x80, 0x80, 0x80, (int)(128.0f * fade));
    if (!init) {
        cur_ang = -3.1415927f;
        init = 1;
    }
    cur_ang += 0.017453292f;
    if (cur_ang > 3.1415927f) {
        cur_ang -= 25.132742f;
    }

    horizontal = -28.0f;
    vertical = -28.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0x84, 0xCA);
    sprite[0].Vertex(position);

    horizontal += 55.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0xB9, 0xCA);
    sprite[0].Vertex(position);

    horizontal -= 55.0f;
    vertical += 55.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0x84, 0xFF);
    sprite[0].Vertex(position);

    horizontal += 55.0f;
    vertical -= 55.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0xB9, 0xCA);
    sprite[0].Vertex(position);

    horizontal -= 55.0f;
    vertical += 55.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0x84, 0xFF);
    sprite[0].Vertex(position);

    horizontal += 55.0f;
    position[0] = (float)x + (vertical * sinf(cur_ang) - horizontal * cosf(cur_ang));
    position[1] = (float)y + (horizontal * sinf(cur_ang) + vertical * cosf(cur_ang));
    sprite[0].TextureCrd(0xB9, 0xFF);
    sprite[0].Vertex(position);
    sprite[0].End();
}

#ifdef NONMATCHING
void DrawMainUnitStatusBord(float rate) {
    int               charge_position[7][2] = {
        { 270, 18 }, { 266, 33 }, { 270, 48 }, { 281, 59 }, { 296, 63 }, { 311, 59 }, { 322, 48 }
    };
    int               charge_glyph[4][2] = { { 0xDA, 0xCE }, { 0xE4, 0xCE }, { 0xDA, 0xD8 }, { 0xE4, 0xD8 } };
    int               status_mask[7] = {
        CHARA_STATUS_POISON, CHARA_STATUS_UNK_2, CHARA_STATUS_UNK_8, CHARA_STATUS_UNK_4,
        CHARA_STATUS_POWER, CHARA_STATUS_UNK_20, CHARA_STATUS_UNK_40
    };
    s16               status_glyph[7][2] = { { 0, 0 }, { 24, 24 }, { 0, 24 }, { 48, 0 }, { 24, 0 }, { 48, 24 }, { 72, 0 } };
    SP_RGBA           color = { 0x80, 0x80, 0x80, 0x80 };
    static float      palanim;
    static char       init;
    CActionChara     *character;
    CBattleCharaInfo *info;
    CGameDataUsed    *active_items;
    int               hp_max;
    int               hp_now;
    int               whp[2][2];
    int               abs[2][2];
    int               weapon_y;
    int               hp_y;
    int               weapon_x;
    int               second_weapon_x;
    int               event_running;
    float             hp_rate;
    float             whp_rate[2];
    float             flash;
    int               index;
    int               item_x;
    int               charge_max;
    int               charge_now;
    int               element;
    int               alpha;
    int               status_attr;
    int               status_x;
    int               width;
    int               top_right;
    int               bottom_right;
    int               red;
    int               green;
    int               blue;
    int               pulse;
    int               gauge_left;
    int               gauge_right;
    int               number_x;
    s16               weapon_no;
    s16               second_weapon_no;

    weapon_y = (int)(80.0f * rate) - 72;
    hp_y = weapon_y;
    weapon_x = 280;
    second_weapon_x = 580 - (int)(300.0f * rate);
    if (SubGameRunning() != 0) {
        weapon_y = -72;
        weapon_x = 280;
        second_weapon_x = 580;
    }
    event_running = 0;
    character = (CActionChara *)DngMainScene->GetCharacter(0);
    if (character != NULL) {
        event_running = character->CheckRunEvent();
    }
    info = GetBattleCharaInfo();
    hp_max = info->GetMaxHp_i();
    hp_now = info->GetNowHp_i();
    info->GetNowWhp(0, whp[0]);
    info->GetNowWhp(1, whp[1]);
    hp_rate = (float)hp_now / (float)hp_max;
    whp_rate[0] = (float)whp[0][0] / (float)whp[0][1];
    whp_rate[1] = (float)whp[1][0] / (float)whp[1][1];
    if (!init) {
        palanim = 0.0f;
        init = 1;
    }
    palanim += 0.19634955f;
    if (palanim > 0.0f) {
        palanim -= 3.1415927f;
    }
    flash = sinf(palanim);

    CPreSprite sprite[2];

    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Bilinear(0);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIRect(24, hp_y, 203, 21, 0, 0x8C);
    sprite[0].SetIRect(212, hp_y + 14, 12, 12, 0x78, 0xE8);
    sprite[0].SetIRect(48, hp_y + 18, 149, 47, 0xEC, 0);
    active_items = info->GetActiveItemInfo(0);
    sprite[0].Texture(TEX_DummyIcon1);
    if (active_items[0].GetNum() > 0) {
        sprite[0].SetIStretch(54, hp_y + 20, 28, 35, 0, 0, 31, 31);
    }
    if (active_items[1].GetNum() > 0) {
        sprite[0].SetIStretch(96, hp_y + 20, 28, 35, 32, 0, 31, 31);
    }
    if (active_items[2].GetNum() > 0) {
        sprite[0].SetIStretch(138, hp_y + 20, 28, 35, 0, 32, 31, 31);
    }
    sprite[0].End();
    if (event_running != 0) {
        DngStatus.cursor_fade += 0.16666667f;
        if (DngStatus.cursor_fade >= 1.0f) {
            DngStatus.cursor_fade = 1.0f;
        }
    } else {
        DngStatus.cursor_fade -= 0.33333334f;
        if (DngStatus.cursor_fade <= 0.0f) {
            DngStatus.cursor_fade = 0.0f;
        }
    }
    DrawActiveItemCursor(DngStatus.active_item * 42 + 68, hp_y + 38, DngStatus.cursor_fade);
    for (index = 0, item_x = 0; index < 3; index++, active_items++, item_x += 41) {
        if (active_items->GetNum() >= 2) {
            mgRect<int> item_glyph(0, 0xE8, 12, 12);
            PrintV(item_x + 64, hp_y + 45, active_items->GetNum(), TEX_SystenFrame, item_glyph, 2, 1, 10, NULL);
        }
    }
    charge_max = info->GetMagicSwordCounterMax();
    element = info->GetMagicSwordElem();
    charge_now = info->GetMagicSwordCounterNow();

    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Texture(TEX_SystenFrame);
    alpha = (int)(128.0f * rate);
    sprite[0].Color(0x80, 0x80, 0x80, alpha);
    for (index = 0; index < charge_max; index++) {
        if (index < charge_now) {
            sprite[0].SetIRect(charge_position[index][0], charge_position[index][1], 10, 10,
                              charge_glyph[element][0], charge_glyph[element][1]);
        } else {
            sprite[0].SetIRect(charge_position[index][0], charge_position[index][1], 10, 10, 0xD0, 0xD8);
        }
    }
    sprite[0].End();
    status_attr = info->GetAttr();

    if (status_attr != 0) {
        status_x = 24;
        sprite[0].Initialize(NULL, NULL);
        sprite[0].Preset2D();
        sprite[0].Begin(MG_PRIM_SPRITE);
        sprite[0].Texture(TEX_StatusIcon);
        sprite[0].Color(0x80, 0x80, 0x80, alpha);
        for (index = 0; index < 7; index++) {
            if (status_attr & status_mask[index]) {
                sprite[0].SetIRect(status_x, 74, 24, 24, status_glyph[index][0], status_glyph[index][1]);
                status_x += 26;
            }
        }
        sprite[0].End();
    }
    width = (int)(171.0f * hp_rate);
    red = 0x80;
    green = 0x80;
    blue = 0x80;
    if (hp_rate < 0.2f) {
        pulse = (int)(-64.0f * flash);
        red = pulse + 0x80;
        green = 0x80 - pulse;
        blue = 0x80 - pulse;
    }
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(red, green, blue, 0x80);
    if (hp_now > 0) {
        top_right = width + 45;
        bottom_right = top_right;
        if (top_right < 50) {
            top_right = 50;
        }
        if (bottom_right >= 212) {
            bottom_right = 211;
        }
        sprite[0].TextureCrd(0x46, 0xA2);
        sprite[0].Vertex(50, hp_y + 6, 0);
        sprite[0].TextureCrd(0x4E, 0xA2);
        sprite[0].Vertex(top_right, hp_y + 6, 0);
        sprite[0].TextureCrd(0x46, 0xA7);
        sprite[0].Vertex(45, hp_y + 11, 0);
        sprite[0].TextureCrd(0x4E, 0xA7);
        sprite[0].Vertex(bottom_right, hp_y + 11, 0);
    }
    sprite[0].End();
    mgRect<int> hp_now_glyph(0, 0xE8, 12, 12);
    PrintV(161, hp_y + 14, hp_now, TEX_SystenFrame, hp_now_glyph, 5, 1, 10, NULL);
    mgRect<int> hp_max_glyph(0, 0xE8, 12, 12);
    PrintV(221, hp_y + 14, hp_max, TEX_SystenFrame, hp_max_glyph, 5, 0, 10, NULL);
    second_weapon_no = info->equip[1].item_no;
    weapon_no = info->equip[0].item_no;
    if (whp_rate[0] < 0.2f) {
        if (whp[0][0] <= 0) {
            pulse = (int)(-64.0f * flash);
            color.r = pulse + 0x80;
            color.g = 0x80 - pulse;
            color.b = color.g;
        } else {
            color.r = 0x80 - (int)(-64.0f * flash);
            color.g = color.r;
            color.b = color.r;
        }
    } else {
        color.r = 0x80;
        color.g = 0x80;
        color.b = 0x80;
    }
    color.a = 0x80;
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Color(color.r, color.g, color.b, color.a);
    sprite[0].SetIRect(weapon_x, weapon_y, 148, 50, 0xEC, 0x2E);
    sprite[0].SetIRect(weapon_x + 64, weapon_y + 19, 12, 12, 0x78, 0xE8);
    if (weapon_no > 0) {
        sprite[0].Texture(TEX_DummyIcon2);
        sprite[0].SetIStretch(weapon_x + 4, weapon_y + 10, 28, 35, 0, 0, 31, 31);
        sprite[0].Texture(TEX_SystenFrame);
        if (whp[0][0] <= 0) {
            sprite[0].SetIRect(weapon_x + 20, weapon_y + 28, 14, 16, 0x50, 0xBA);
        }
    }
    sprite[0].End();
    number_x = weapon_x + 76;
    mgRect<int> whp_now_glyph(0, 0xE8, 12, 12);
    PrintV(number_x - 61, weapon_y + 19, whp[0][0], TEX_SystenFrame, whp_now_glyph, 5, 1, 10, &color);
    mgRect<int> whp_max_glyph(0, 0xE8, 12, 12);
    PrintV(number_x - 4, weapon_y + 19, whp[0][1], TEX_SystenFrame, whp_max_glyph, 5, 0, 10, &color);
    gauge_left = weapon_x + 36;
    gauge_right = gauge_left + (int)(95.0f * whp_rate[0]);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].TextureCrd(0x46, 0xB2);
    sprite[0].Vertex(gauge_left, weapon_y + 5, 0);
    sprite[0].TextureCrd(0x4E, 0xB2);
    sprite[0].Vertex(gauge_right, weapon_y + 5, 0);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(gauge_left, weapon_y + 9, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(gauge_right, weapon_y + 9, 0);
    sprite[0].End();
    info->GetNowAbs(0, abs[0]);
    gauge_left = weapon_x + 38;
    gauge_right = gauge_left + (int)(95.0f * ((float)abs[0][0] / (float)abs[0][1]));
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(gauge_left, weapon_y + 10, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(gauge_right, weapon_y + 10, 0);
    sprite[0].TextureCrd(0x46, 0xBA);
    sprite[0].Vertex(gauge_left, weapon_y + 13, 0);
    sprite[0].TextureCrd(0x4E, 0xBA);
    sprite[0].Vertex(gauge_right, weapon_y + 13, 0);
    sprite[0].End();
    if (whp_rate[1] < 0.2f) {
        if (whp[1][0] <= 0) {
            pulse = (int)(-64.0f * flash);
            color.r = pulse + 0x80;
            color.g = 0x80 - pulse;
            color.b = color.g;
        } else {
            color.r = 0x80 - (int)(-64.0f * flash);
            color.g = color.r;
            color.b = color.r;
        }
    } else {
        color.r = 0x80;
        color.g = 0x80;
        color.b = 0x80;
    }
    color.a = 0x80;
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Color(color.r, color.g, color.b, color.a);
    sprite[0].SetIRect(second_weapon_x + 58, 17, 148, 50, 0xEC, 0x60);
    sprite[0].SetIRect(second_weapon_x + 129, 36, 12, 12, 0x78, 0xE8);
    if (second_weapon_no > 0) {
        sprite[0].Texture(TEX_DummyIcon2);
        sprite[0].SetIStretch(second_weapon_x + 170, 18, 28, 35, 32, 0, 31, 31);
        sprite[0].Texture(TEX_SystenFrame);
        if (whp[1][0] <= 0) {
            sprite[0].SetIRect(second_weapon_x + 186, weapon_y + 28, 14, 16, 0x50, 0xBA);
        }
    }
    sprite[0].End();
    gauge_left = second_weapon_x + 74;
    gauge_right = gauge_left + (int)(95.0f * whp_rate[1]);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].TextureCrd(0x46, 0xB2);
    sprite[0].Vertex(gauge_left, 55, 0);
    sprite[0].TextureCrd(0x4E, 0xB2);
    sprite[0].Vertex(gauge_right, 55, 0);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(gauge_left, 59, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(gauge_right, 59, 0);
    sprite[0].End();
    info->GetNowAbs(1, abs[1]);
    gauge_left = second_weapon_x + 76;
    gauge_right = gauge_left + (int)(95.0f * ((float)abs[1][0] / (float)abs[1][1]));
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(gauge_left, 60, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(gauge_right, 60, 0);
    sprite[0].TextureCrd(0x46, 0xBA);
    sprite[0].Vertex(gauge_left, 63, 0);
    sprite[0].TextureCrd(0x4E, 0xBA);
    sprite[0].Vertex(gauge_right, 63, 0);
    sprite[0].End();
    number_x = second_weapon_x + 142;
    mgRect<int> second_whp_now_glyph(0, 0xE8, 12, 12);
    PrintV(number_x - 61, 36, whp[1][0], TEX_SystenFrame, second_whp_now_glyph, 5, 1, 10, &color);
    mgRect<int> second_whp_max_glyph(0, 0xE8, 12, 12);
    PrintV(number_x - 4, 36, whp[1][1], TEX_SystenFrame, second_whp_max_glyph, 5, 0, 10, &color);
    if (rate >= 1.0f) {
        if (SubGameRunning() != 0) {
            return;
        }
        if (hp_rate < 0.3f) {
            WarningGage2.warning[0] = 1;
        } else {
            WarningGage2.warning[0] = 0;
        }
        if (whp_rate[0] < 0.2f) {
            WarningGage2.warning[1] = 1;
        } else {
            WarningGage2.warning[1] = 0;
        }
        if (whp_rate[1] < 0.2f) {
            WarningGage2.warning[2] = 1;
        } else {
            WarningGage2.warning[2] = 0;
        }
        WarningGage2.rate[0] = hp_rate;
        WarningGage2.rate[1] = whp_rate[0];
        WarningGage2.rate[2] = whp_rate[1];
        WarningGage2.layout = WARNING_GAGE_LAYOUT_MAIN;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_status", DrawMainUnitStatusBord__Ff);
#endif

void DrawRoboUnitStatusBord(float rate) {
    SP_RGBA           color;
    static float      palanim;
    static char       init;
    CBattleCharaInfo *info;
    int               hp_max;
    int               hp_now;
    int               whp[2];
    int               abs[2];
    float             hp_rate[1];
    float             whp_rate;
    float             flash;
    int               pulse;
    int               width;
    int               top_right;
    int               bottom_right;

    color.r = 0x80;
    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    int y = (int)(80.0f * rate) - 72;
    info = GetBattleCharaInfo();
    hp_max = info->GetMaxHp_i();
    hp_now = info->GetNowHp_i();
    info->GetNowWhp(0, whp);
    hp_rate[0] = (float)hp_now / (float)hp_max;
    whp_rate = (float)whp[0] / (float)whp[1];
    if (!init) {
        palanim = 0.0f;
        init = 1;
    }
    palanim += 0.19634955f;
    if (palanim > 0.0f) {
        palanim -= 3.1415927f;
    }
    flash = sinf(palanim);
    if (whp_rate < 0.2f) {
        if (whp_rate <= 0.0f) {
            pulse = (int)(-64.0f * flash);
            color.r = pulse + 0x80;
            color.g = 0x80 - pulse;
            color.b = 0x80 - pulse;
            color.a = 0x80;
        } else {
            pulse = (int)(-64.0f * flash);
            color.r = 0x80 - pulse;
            color.g = 0x80 - pulse;
            color.b = 0x80 - pulse;
            color.a = 0x80;
        }
    } else {
        color.r = 0x80;
        color.g = 0x80;
        color.b = 0x80;
        color.a = 0x80;
    }

    CPreSprite sprite[2];

    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Bilinear(0);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIRect(186, y + 8, 42, 22, 0xC0, 0);
    sprite[0].SetIRect(219, y, 96, 46, 0x120, 0x92);
    sprite[0].SetIRect(16, y, 192, 42, 0, 0);
    sprite[0].Color(color.r, color.g, color.b, 0x80);
    sprite[0].SetIRect(320, y, 176, 40, 0, 0x2A);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIRect(106, y + 20, 12, 12, 0x78, 0xE8);
    sprite[0].Color(color.r, color.g, color.b, 0x80);
    sprite[0].SetIRect(420, y + 18, 12, 12, 0x78, 0xE8);
    sprite[0].Texture(TEX_DummyIcon2);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIStretch(22, y + 6, 28, 35, 0, 0, 31, 31);
    sprite[0].Color(color.r, color.g, color.b, 0x80);
    sprite[0].SetIStretch(460, y + 6, 28, 35, 32, 0, 31, 31);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].End();
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    top_right = (width = (int)(141.0f * hp_rate[0])) + 58;
    bottom_right = top_right;
    if (bottom_right > 196) {
        bottom_right = 196;
    }
    sprite[0].TextureCrd(0x50, 0xB4);
    sprite[0].Vertex(62, y + 5, 0);
    sprite[0].TextureCrd(0x58, 0xB4);
    sprite[0].Vertex(top_right, y + 5, 0);
    sprite[0].TextureCrd(0x50, 0xB9);
    sprite[0].Vertex(58, y + 9, 0);
    sprite[0].TextureCrd(0x58, 0xB9);
    sprite[0].Vertex(bottom_right, y + 9, 0);
    sprite[0].End();
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(color.r, color.g, color.b, 0x80);
    top_right = (int)(107.0f * ((float)whp[0] / (float)whp[1])) + 337;
    sprite[0].TextureCrd(0x46, 0xB2);
    sprite[0].Vertex(337, y + 8, 0);
    sprite[0].TextureCrd(0x4E, 0xB2);
    sprite[0].Vertex(top_right, y + 8, 0);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(337, y + 12, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(top_right, y + 12, 0);
    sprite[0].End();
    int blue = ((SP_RGBA *)&color.b)->r;
    int green = ((SP_RGBA *)&color.g)->r;
    color.g = green;
    color.b = blue;
    color.a = 0x80;
    mgRect<int> whp_now_glyph(0, 0xE8, 12, 12);
    PrintV(370, y + 18, whp[0], TEX_SystenFrame, whp_now_glyph, 5, 1, 10, &color);
    mgRect<int> whp_max_glyph(0, 0xE8, 12, 12);
    PrintV(430, y + 18, whp[1], TEX_SystenFrame, whp_max_glyph, 5, 0, 10, &color);
    color.r = 0x80;
    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    mgRect<int> hp_now_glyph(0, 0xE8, 12, 12);
    PrintV(56, y + 20, hp_now, TEX_SystenFrame, hp_now_glyph, 5, 1, 10, &color);
    mgRect<int> hp_max_glyph(0, 0xE8, 12, 12);
    PrintV(116, y + 20, hp_max, TEX_SystenFrame, hp_max_glyph, 5, 0, 10, &color);
    info->GetNowAbs(0, abs);
    DrawDrumCounter(230, y + 19, abs[0]);
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIRect(width + 57, y - 3, 8, 24, 0xB0, 0x2A);
    sprite[0].End();
    if (hp_rate[0] < 0.3f) {
        WarningGage2.warning[0] = 1;
    } else {
        WarningGage2.warning[0] = 0;
    }
    if (whp_rate < 0.2f) {
        WarningGage2.warning[1] = 1;
    } else {
        WarningGage2.warning[1] = 0;
    }
    WarningGage2.rate[0] = hp_rate[0];
    WarningGage2.layout = WARNING_GAGE_LAYOUT_ROBO;
    WarningGage2.rate[1] = whp_rate;
}

void DrawMonsterUnitStatusBord(float rate) {
    int               whp[2];
    int               abs[2];
    CBattleCharaInfo *info;
    int               hp_max;
    int               hp_now;
    float             hp_rate;
    int               top_right;
    int               bottom_right;
    int               top_left;
    int               abs_right;

    if (rate < 1.0f) {
        return;
    }
    info = GetBattleCharaInfo();
    hp_max = info->GetMaxHp_i();
    hp_now = info->GetNowHp_i();
    info->GetNowWhp(0, whp);

    CPreSprite sprite[2];
    SP_RGBA color;
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_SPRITE);
    sprite[0].Bilinear(0);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    sprite[0].SetIRect(16, 8, 202, 22, 0, 0x52);
    sprite[0].SetIRect(300, 8, 200, 36, 0, 0x68);
    sprite[0].SetIRect(162, 24, 12, 12, 0x78, 0xE8);
    sprite[0].SetIRect(375, 29, 12, 12, 0x78, 0xE8);
    sprite[0].End();
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    hp_rate = (float)hp_now / (float)hp_max;
    top_right = (int)(176.0f * hp_rate) + 32;
    bottom_right = top_right;
    if (bottom_right > 200) {
        bottom_right = 200;
    }
    sprite[0].TextureCrd(0x46, 0xA2);
    sprite[0].Vertex(40, 14, 0);
    sprite[0].TextureCrd(0x4E, 0xA2);
    sprite[0].Vertex(top_right, 14, 0);
    sprite[0].TextureCrd(0x46, 0xA7);
    sprite[0].Vertex(32, 19, 0);
    sprite[0].TextureCrd(0x4E, 0xA7);
    sprite[0].Vertex(bottom_right, 19, 0);
    sprite[0].End();
    sprite[0].Initialize(NULL, NULL);
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].Bilinear(0);
    sprite[0].Texture(TEX_SystenFrame);
    sprite[0].Color(0x80, 0x80, 0x80, 0x80);
    top_right = (int)(137.0f * ((float)whp[0] / (float)whp[1])) + 343;
    top_left = 343;
    bottom_right = top_right + 4;
    top_left -= 4;
    if (top_left < 339) {
        top_left = 339;
    }
    if (bottom_right > 481) {
        bottom_right = 481;
    }
    sprite[0].TextureCrd(0x46, 0xA8);
    sprite[0].Vertex(top_left, 13, 0);
    sprite[0].TextureCrd(0x4E, 0xA8);
    sprite[0].Vertex(top_right, 13, 0);
    sprite[0].TextureCrd(0x46, 0xAC);
    sprite[0].Vertex(343, 18, 0);
    sprite[0].TextureCrd(0x4E, 0xAC);
    sprite[0].Vertex(bottom_right, 18, 0);
    sprite[0].End();
    info->GetNowAbs(0, abs);
    abs_right = (int)(137.0f * ((float)abs[0] / (float)abs[1])) + 343;
    sprite[0].Preset2D();
    sprite[0].Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite[0].TextureCrd(0x46, 0xB6);
    sprite[0].Vertex(343, 20, 0);
    sprite[0].TextureCrd(0x4E, 0xB6);
    sprite[0].Vertex(abs_right, 20, 0);
    sprite[0].TextureCrd(0x46, 0xBA);
    sprite[0].Vertex(343, 23, 0);
    sprite[0].TextureCrd(0x4E, 0xBA);
    sprite[0].Vertex(abs_right, 23, 0);
    sprite[0].End();

    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    color.r = 0x80;
    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    mgRect<int> hp_now_glyph(0, 0xE8, 12, 12);
    PrintV(110, 24, hp_now, TEX_SystenFrame, hp_now_glyph, 5, 1, 10, &color);
    mgRect<int> hp_max_glyph(0, 0xE8, 12, 12);
    PrintV(172, 24, hp_max, TEX_SystenFrame, hp_max_glyph, 5, 0, 10, &color);
    color.r = 0x80;
    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    mgRect<int> whp_max_glyph(0, 0xE8, 12, 12);
    PrintV(352, 29, whp[1], TEX_SystenFrame, whp_max_glyph, 5, 1, 10, &color);
    mgRect<int> whp_now_glyph(0, 0xE8, 12, 12);
    PrintV(355, 29, whp[0], TEX_SystenFrame, whp_now_glyph, 5, 0, 10, &color);
    if (hp_rate < 0.3f) {
        WarningGage2.warning[0] = 1;
    } else {
        WarningGage2.warning[0] = 0;
    }
    WarningGage2.rate[0] = hp_rate;
    WarningGage2.layout = WARNING_GAGE_LAYOUT_MAIN;
}

void DrawStatusBord() {
    float rate;
    int   chara;

    chara = DngUserData->active_chr_no;
    rate = BattleAreaScene->statusbar_rate;
    WarningGage2.warning[0] = 0;
    WarningGage2.warning[1] = 0;
    WarningGage2.warning[2] = 0;
    LockOnModel.pos[3] = 0.0f;
    switch (chara) {
    case USER_CHARA_MAX:
    case USER_CHARA_MONICA:
        DrawMainUnitStatusBord(rate);
        break;
    case USER_CHARA_ROBO:
        DrawRoboUnitStatusBord(rate);
        break;
    case USER_CHARA_MONSTER:
        DrawMonsterUnitStatusBord(rate);
        break;
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_status", at_1048__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_status", at_1049__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_status", at_1058__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_status", at_1059__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(cur_ang_1005, 0x4);
INCLUDE_BSS(init_1006, 0x4);
INCLUDE_BSS(palanim_1023, 0x4);
INCLUDE_BSS(init_1024, 0x4);
INCLUDE_BSS(palanim_1222, 0x4);
INCLUDE_BSS(init_1223, 0x4);
