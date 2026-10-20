#include "common.h"

#include <cmath>

#include "actionchara.hpp"
#include "dng_hud.hpp"
#include "dng_main.hpp"
#include "dng_status.hpp"
#include "mg_drawprim.hpp"
#include "maintex.hpp"
#include "mglib.hpp"
#include "prespr.hpp"
#include "subgame.hpp"
#include "userdata.hpp"

#include "ixion_toan_combat.hpp"

// Ixion: Toan's status panel while Dark Cloud's combat drives him. The HP/item half is DC2's own
// DrawMainUnitStatusBord (ps2/src/dng_status.cpp:246-487, copied verbatim apart from its static names).
// The weapon half shows Dark Cloud's action gauge (BtActStatus.action_gauge, 0-100) in the wrench panel,
// drawn with the wrench panel's own frame, numbers and bars; the second bar fills while Toan charges.
// Max's wrench and gun durability are not shown: Toan's Dagger has no DC2 durability yet. No DC1 Dagger
// icon exists in DC2's textures, so the icon box stays empty.

// Defined in ps2/src/dng_status.cpp:184 without a header declaration.
void DrawActiveItemCursor(int x, int y, float alpha);

namespace {
float toan_palanim;
bool  toan_palanim_init;
}

void IxionDrawToanUnitStatusBord(float rate, const IxionToanHud &hud) {
    SP_RGBA           color;
    CActionChara     *character;
    CBattleCharaInfo *info;
    CGameDataUsed    *active_items;
    int               weapon_x;
    int               second_weapon_x;
    int               hp_max;
    int               hp_now;
    int               second_weapon_y;
    int               event_running;
    float             hp_rate;
    float             flash[1];
    int               index;
    int               item_x;
    int               charge_max;
    int               element;
    int               charge_now;
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

    color.r = 0x80;
    color.g = 0x80;
    color.b = 0x80;
    color.a = 0x80;
    int weapon_y = (int) (80.0f * rate) - 72;
    int hp_y = weapon_y;
    weapon_x = 280;
    second_weapon_x = 580 - (int) (300.0f * rate);
    second_weapon_y = 8;

    if (SubGameRunning() != 0) {
        weapon_y = -72;
        weapon_x = 280;
        second_weapon_x = 580;
        second_weapon_y = 8;
    }

    event_running = false;
    character = (CActionChara *) DngMainScene->GetCharacter(0);

    if (character != NULL) {
        event_running = character->CheckRunEvent();
    }

    info = GetBattleCharaInfo();
    hp_max = info->GetMaxHp_i();
    hp_now = info->GetNowHp_i();
    hp_rate = (float) hp_now / (float) hp_max;

    if (!toan_palanim_init) {
        toan_palanim = 0.0f;
        toan_palanim_init = true;
    }

    toan_palanim += PI / 16.0f;

    if (!(toan_palanim <= 0.0f)) {
        toan_palanim -= PI;
    }

    flash[0] = sinf(toan_palanim);

    CPreSprite sprite;
    CPreSprite spare;
    sprite.Initialize(NULL, NULL);
    sprite.Preset2D();
    sprite.Begin(MG_PRIM_SPRITE);
    sprite.Bilinear(0);
    sprite.Texture(TEX_SystenFrame);
    sprite.Color(0x80, 0x80, 0x80, 0x80);
    sprite.SetIRect(24, hp_y, 203, 21, 0, 0x8C);
    sprite.SetIRect(212, hp_y + 14, 12, 12, 0x78, 0xE8);
    sprite.SetIRect(48, hp_y + 18, 149, 47, 0xEC, 0);
    active_items = info->GetActiveItemInfo(0);
    sprite.Texture(TEX_DummyIcon1);

    if (active_items[0].GetNum() > 0) {
        sprite.SetIStretch(54, hp_y + 20, 28, 35, 0, 0, 31, 31);
    }

    if (active_items[1].GetNum() > 0) {
        sprite.SetIStretch(96, hp_y + 20, 28, 35, 32, 0, 31, 31);
    }

    if (active_items[2].GetNum() > 0) {
        sprite.SetIStretch(138, hp_y + 20, 28, 35, 0, 32, 31, 31);
    }

    sprite.End();

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
        if (active_items->GetNum() > 1) {
            PrintV(item_x + 64, hp_y + 45, active_items->GetNum(), TEX_SystenFrame, mgRect<int>(0, 0xE8, 12, 12), 2, true, 10, NULL);
        }
    }

    charge_max = info->GetMagicSwordCounterMax();
    element = info->GetMagicSwordElem();
    charge_now = info->GetMagicSwordCounterNow();
    int charge_position[7][2] = {
        {270, 18},
        {266, 33},
        {270, 48},
        {281, 59},
        {296, 63},
        {311, 59},
        {322, 48}
    };
    int charge_glyph[4][2] = {
        {218, 206},
        {228, 206},
        {218, 216},
        {228, 216}
    };

    sprite.Initialize(NULL, NULL);
    sprite.Preset2D();
    sprite.Begin(MG_PRIM_SPRITE);
    sprite.Texture(TEX_SystenFrame);
    sprite.Color(0x80, 0x80, 0x80, (int) (128.0f * rate));

    for (int i = 0; i < charge_max; i++) {
        if (i < charge_now) {
            sprite.SetIRect(charge_position[i][0], charge_position[i][1], 10, 10,
                            charge_glyph[element][0], charge_glyph[element][1]);
        } else {
            sprite.SetIRect(charge_position[i][0], charge_position[i][1], 10, 10, 0xD0, 0xD8);
        }
    }

    sprite.End();
    status_attr = info->GetAttr();
    int status_mask[7] = {CHARA_STATUS_POISON, CHARA_STATUS_SLOW, CHARA_STATUS_STOP, CHARA_STATUS_CURSE, CHARA_STATUS_POWER, CHARA_STATUS_STONE, CHARA_STATUS_DRY};
    s16 status_glyph[7][2] = {
        {0,  0 },
        {24, 24},
        {0,  24},
        {48, 0 },
        {24, 0 },
        {48, 24},
        {72, 0 }
    };

    if (status_attr != 0) {
        status_x = 24;
        sprite.Initialize(NULL, NULL);
        sprite.Preset2D();
        sprite.Begin(MG_PRIM_SPRITE);
        sprite.Texture(TEX_StatusIcon);
        sprite.Color(0x80, 0x80, 0x80, (int) (128.0f * rate));

        for (int i = 0; i < 7; i++) {
            if (status_attr & status_mask[i]) {
                sprite.SetIRect(status_x, 74, 24, 24, status_glyph[i][0], status_glyph[i][1]);
                status_x += 26;
            }
        }

        sprite.End();
    }

    width = (int) (171.0f * hp_rate);
    red = 0x80;
    green = 0x80;
    blue = 0x80;

    if (hp_rate < 0.2f) {
        pulse = (int) (-64.0f * flash[0]);
        red += pulse;
        green -= pulse;
        blue -= pulse;
    }

    sprite.Initialize(NULL, NULL);
    sprite.Preset2D();
    sprite.Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite.Texture(TEX_SystenFrame);
    sprite.Color(red, green, blue, 0x80);

    if (hp_now > 0) {
        top_right = width + 45;
        bottom_right = top_right;

        if (top_right < 50) {
            top_right = 50;
        }

        if (bottom_right > 211) {
            bottom_right = 211;
        }

        sprite.TextureCrd(0x46, 0xA2);
        sprite.Vertex(50, hp_y + 6, 0);
        sprite.TextureCrd(0x4E, 0xA2);
        sprite.Vertex(top_right, hp_y + 6, 0);
        sprite.TextureCrd(0x46, 0xA7);
        sprite.Vertex(45, hp_y + 11, 0);
        sprite.TextureCrd(0x4E, 0xA7);
        sprite.Vertex(bottom_right, hp_y + 11, 0);
    }

    sprite.End();
    PrintV(161, hp_y + 14, hp_now, TEX_SystenFrame, mgRect<int>(0, 0xE8, 12, 12), 5, true, 10, NULL);
    PrintV(221, hp_y + 14, hp_max, TEX_SystenFrame, mgRect<int>(0, 0xE8, 12, 12), 5, false, 10, NULL);

    // Ixion: Dark Cloud's gauge in the wrench panel. Below 20 a combo cannot chain (charakey.cpp:63), so the
    // panel pulses like DC2's low-durability warning.
    const int   gauge = (int) hud.gauge;
    const float gauge_rate = hud.gauge < 0.0f ? 0.0f : (hud.gauge > 100.0f ? 1.0f : hud.gauge / 100.0f);
    if (hud.gauge < 20.0f) {
        pulse = (int) (-64.0f * flash[0]);
        color.r = 0x80 - pulse;
        color.g = 0x80 - pulse;
        color.b = 0x80 - pulse;
        color.a = 0x80;
    } else {
        color.r = 0x80;
        color.g = 0x80;
        color.b = 0x80;
        color.a = 0x80;
    }

    sprite.Preset2D();
    sprite.Begin(MG_PRIM_SPRITE);
    sprite.Color(color.r, color.g, color.b, color.a);
    sprite.SetIRect(weapon_x, weapon_y, 148, 50, 0xEC, 0x2E);
    sprite.SetIRect(weapon_x + 64, weapon_y + 19, 12, 12, 0x78, 0xE8);
    sprite.End();
    number_x = weapon_x + 76;
    PrintV(number_x - 61, weapon_y + 19, gauge, TEX_SystenFrame, mgRect<int>(0, 0xE8, 12, 12), 5, true, 10, &color);
    PrintV(number_x - 4, weapon_y + 19, 100, TEX_SystenFrame, mgRect<int>(0, 0xE8, 12, 12), 5, false, 10, &color);
    width = (int) (95.0f * gauge_rate);
    gauge_left = weapon_x + 36;
    gauge_right = gauge_left + width;
    sprite.Preset2D();
    sprite.Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite.Color(0x80, 0x80, 0x80, 0x80);
    sprite.TextureCrd(0x46, 0xB2);
    sprite.Vertex(gauge_left, weapon_y + 5, 0);
    sprite.TextureCrd(0x4E, 0xB2);
    sprite.Vertex(gauge_right, weapon_y + 5, 0);
    sprite.TextureCrd(0x46, 0xB6);
    sprite.Vertex(gauge_left, weapon_y + 9, 0);
    sprite.TextureCrd(0x4E, 0xB6);
    sprite.Vertex(gauge_right, weapon_y + 9, 0);
    sprite.End();

    // The second bar is the charge: empty until Cross is held into action 14, full at charge level 1 (1.5).
    float charge_rate = hud.charge_time > 1.0f ? (hud.charge_time - 1.0f) / 0.5f : 0.0f;
    if (charge_rate > 1.0f) charge_rate = 1.0f;
    width = (int) (95.0f * charge_rate);
    gauge_left = weapon_x + 38;
    gauge_right = gauge_left + width;
    sprite.Preset2D();
    sprite.Begin(MG_PRIM_TRIANGLE_STRIP);
    sprite.TextureCrd(0x46, 0xB6);
    sprite.Vertex(gauge_left, weapon_y + 10, 0);
    sprite.TextureCrd(0x4E, 0xB6);
    sprite.Vertex(gauge_right, weapon_y + 10, 0);
    sprite.TextureCrd(0x46, 0xBA);
    sprite.Vertex(gauge_left, weapon_y + 13, 0);
    sprite.TextureCrd(0x4E, 0xBA);
    sprite.Vertex(gauge_right, weapon_y + 13, 0);
    sprite.End();
    (void) second_weapon_x;
    (void) second_weapon_y;

    if (rate < 1.0f || SubGameRunning() != 0) {
        return;
    }

    WarningGage2.warning[0] = hp_rate < 0.3f;
    WarningGage2.warning[1] = false;
    WarningGage2.warning[2] = false;
    WarningGage2.rate[0] = hp_rate;
    WarningGage2.rate[1] = gauge_rate;
    WarningGage2.rate[2] = 1.0f;
    WarningGage2.layout = WARNING_GAGE_LAYOUT_MAIN;
}
