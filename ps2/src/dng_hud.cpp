#include "common.h"
#include "dng_hud.hpp"
#include "maintex.hpp"
#include "mglib.hpp"
#include "mainloop.hpp"
#include "dng_main.hpp"
#include "scenesnd.hpp"
#include "nd_meswin.hpp"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

/**
 * Vertical offsets for the enemy's breaking rage mark.
 */
static s32 gekirin_anim[16] = {0, 3, 6, 5, 4, 3, 2, 1, 3, 4, 3, 2, 1, 2, 1, 0};

// Code (.text)

void CLevelupInfo::SetLevelUpInfo(s32 screen_x, s32 screen_y, s32 source, s32 value) {
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    unk_14 = 0;
    unk_18 = 0;
    unk_1c = 0;
    progress = 0.0f;
    phase = LEVELUP_INFO_PHASE_APPEAR;
    x = screen_x - 0x23;
    y = screen_y - 6;
    unk_30 = source;
    unk_34 = value;
}

void CLevelupInfo::Draw(void) {
    int i;

    if (phase != LEVELUP_INFO_PHASE_NONE) {
        CPreSprite sprite;
        sprite.Initialize(NULL, NULL);
        sprite.Preset2D();
        sprite.Begin(6);
        sprite.Texture(TEX_SystenFrame);
        switch (phase) {
            case LEVELUP_INFO_PHASE_APPEAR: {
                int rise = (int)(32.0f * sinf(4.712389f * progress - 1.5707964f));
                sprite.Color(0x80, 0x80, 0x80, (int)(128.0f * progress));
                sprite.SetIRect(x, y - rise, 70, 12, 0, 0xA2);
                sprite.SetIRect(x - 0x10, y - rise - 3, 0x10, 0x10, 0x30, 0x40);
                break;
            }
            case LEVELUP_INFO_PHASE_FLASH: {
                float pulse;
                sprite.Color(0x80, 0x80, 0x80, 0x80);
                sprite.SetIRect(x, y, 70, 12, 0, 0xA2);
                sprite.SetIRect(x - 0x10, y - 3, 0x10, 0x10, 0x30, 0x40);
                pulse = sinf(3.1415927f * progress);
                sprite.SetAlphaBlend(2);
                sprite.Color(0x80, 0x80, 0x80, (int)(32.0f * pulse));
                for (i = 0; i < 4; i++) {
                    sprite.SetIStretch(x - i, y - i, i * 2 + 70, i * 2 + 12, 0,
                                       0xA2, 70, 12);
                    sprite.SetIStretch(x - i - 0x10, y - i - 3, i * 2 + 0x10, i * 2 + 0x10, 0x30,
                                       0x40, 0x10, 0x10);
                }
                break;
            }
            case LEVELUP_INFO_PHASE_HOLD:
                sprite.Color(0x80, 0x80, 0x80, 0x80);
                sprite.SetIRect(x, y, 70, 12, 0, 0xA2);
                sprite.SetIRect(x - 0x10, y - 3, 0x10, 0x10, 0x30, 0x40);
                break;
            case LEVELUP_INFO_PHASE_FADE:

                sinf(4.712389f * progress - 1.5707964f);
                sprite.Color(0x80, 0x80, 0x80, 0x80 - (int)(128.0f * progress));
                sprite.SetIRect(x, y, 70, 12, 0, 0xA2);
                sprite.SetIRect(x - 0x10, y - 3, 0x10, 0x10, 0x30, 0x40);
                break;
        }
        sprite.End();
    }
}

void CLevelupInfo::Step(void) {
    if (phase != LEVELUP_INFO_PHASE_NONE) {
        switch (phase) {
            case LEVELUP_INFO_PHASE_APPEAR:
            case LEVELUP_INFO_PHASE_FLASH:
            case LEVELUP_INFO_PHASE_HOLD:
                if (progress < 0.9f) {
                    progress += 0.1f;
                } else {
                    progress = 0.0f;
                    phase += 1;
                }
                break;
            case LEVELUP_INFO_PHASE_FADE:
                if (progress < 0.9f) {
                    progress += 0.1f;
                } else {
                    phase = LEVELUP_INFO_PHASE_NONE;
                }
                break;
        }
    }
}

void CPiyori::Initialize(void) {
    target = NULL;
}

void CPiyori::Reset(void) {
    target = NULL;
    time = 0;
}

void CPiyori::Set(mgCObject *target, float height, float radius, short time) {
    int i;

    if (target != NULL) {
        this->target = target;
        this->height = height;
        this->radius = radius;
        this->time = time;
        circle_angle = 0.0f;
        for (i = 0; i < 3; i++) {
            star_angle[i] = fRand(6.2831855f) - 3.1415927f;
        }
        se_wait = 0;
    }
}

void CPiyori::Set(mgCObject *target, s16 time) {
    if (target != NULL) {
        CCharacter2 *character = reinterpret_cast<CCharacter2 *>(target);
        this->Set(target, 2.0f * character->body_height, 2.0f * character->body_width, time);
    }
}

void CPiyori::Draw(void) {
    float char_pos[4];
    sceVu0FVECTOR pos;
    int i;
    int alpha;
    int draw_time;
    float draw_angle;
    float draw_radius;

    if (this->target != NULL) {
        this->target->GetPosition(char_pos);
        alpha = 0x80;
        char_pos[1] += this->height;
        draw_time = this->time;
        draw_radius = this->radius;
        draw_angle = this->circle_angle;
        if (draw_time < 16) {
            alpha = draw_time * 8;

            draw_radius = this->radius / 24.0 * draw_time;
        }

        CPreSprite prim;
        int quad_a[4];
        int quad_b[4];
        prim.Initialize(NULL, NULL);
        prim.Preset2D();
        prim.Coord(1);
        prim.DepthTestEnable(1);
        prim.ZMask(-1);
        prim.Bilinear(1);
        prim.TextureMapEnable(1);
        prim.Begin(6);
        prim.Color(0x80, 0x80, 0x80, alpha);
        prim.Texture(TEX_SystemEffect1);
        pos[3] = 1.0f;
        for (i = 0; i < 3; i++) {
            pos[0] = char_pos[0] + draw_radius * cosf(draw_angle);
            pos[1] = char_pos[1] + sinf(this->star_angle[i]);
            pos[2] = char_pos[2] + draw_radius * sinf(draw_angle);
            if (mgTransWorldPrim3DSprite(quad_a, quad_b, pos, 4.0f, 4.0f, 0) != 0) {
                prim.TextureCrd(0xC1, 0x61);
                prim.Vertex4(quad_a);
                prim.TextureCrd(0xDF, 0x7F);
                prim.Vertex4(quad_b);
            }
            draw_angle += 2.0943952f;
        }
        prim.End();
    }
}

void CPiyori::Step(void) {
    float pos[4];
    float vol;
    float pan;
    int i;

    if (target != NULL) {
        time -= 1;
        if (time <= 0) {
            target = NULL;
            return;
        }
        se_wait -= 1;
        if (se_wait <= 0) {
            static_cast<CCharacter2 *>(target)->GetEntryObjectPos(0, pos);
            sndGetVolPan(&vol, &pan, pos, 160.0f, 1200.0f);
            sndSePlayVPf(GetMainScene()->se_battle_id, 0x24, vol, pan, 0);
            se_wait = 13;
        }
        {
            float a = circle_angle;
            a += 0.10471976f;
            circle_angle = a;
            if (a > 3.1415927f) {
                circle_angle = a - 6.2831855f;
            }
        }
        for (i = 0; i < 3; i++) {
            float a = star_angle[i];
            a += 0.20943952f;
            star_angle[i] = a;
            if (a > 3.1415927f) {
                star_angle[i] = a - 6.2831855f;
            }
        }
    }
}

void CGiftMark::Set(CCharacter2 *chara, float height) {
    Initialize();
    this->chara = chara;
    this->height = height;
    active = 1;
}

void CGiftMark::Draw(void) {
    sceVu0FVECTOR pos;
    int i;

    if (active != 0) {
        if (chara != NULL) {
            chara->GetEntryObjectPos(0, pos);
            pos[1] += 10.0f + 2.0f * height;
            pos[1] += 5.0f * sinf(angle);

            CPreSprite prim;
            int quad_a[4];
            int quad_b[4];
            prim.Initialize(NULL, NULL);
            prim.Preset2D();
            prim.Coord(1);
            prim.DepthTestEnable(1);
            prim.ZMask(-1);
            prim.Bilinear(1);
            prim.TextureMapEnable(1);
            prim.Begin(6);
            prim.Color(0x80, 0x80, 0x80, 0x80);
            prim.Texture(TEX_SystemEffect1);
            pos[3] = 1.0f;
            for (i = 0; i < 3; i++) {
                if (mgTransWorldPrim3DSprite(quad_a, quad_b, pos, 8.0f, 8.0f, 0) != 0) {
                    prim.TextureCrd(0xE1, 0x41);
                    prim.Vertex4(quad_a);
                    prim.TextureCrd(0xFC, 0x62);
                    prim.Vertex4(quad_b);
                }
            }
            prim.End();
        }
    }
}

void CGiftMark::Step(void) {
    if (active != 0) {
        angle += 0.1308997f;
        if (angle > 3.1415927f) {
            angle -= 3.1415927f;
        }
        time += 1;
        if (time > 240) {
            active = 0;
        }
    }
}

void CGiftMark::Initialize(void) {
    chara = NULL;
    active = 0;
    angle = 0.0f;
    time = 0;
}

void CEnemyGekirin::Draw(CPreSprite *sprite, int x, int y) {
    if (state != GEKIRIN_STATE_NONE) {
        sprite->SetAlphaBlend(0);
        sprite->Color(0x80, 0x80, 0x80, 0x80);
        sprite->SetIRect(x, y - gekirin_anim[frame] * 4, 8, 8, 0x78, 0xCA);
        if (state == GEKIRIN_STATE_BREAK) {
            sprite->SetAlphaBlend(2);
            sprite->Color(0x80, 0x80, 0x80, 0x40);
            sprite->SetIStretch(x - 4, y - gekirin_anim[frame] * 4 - 4, 0x10, 0x10, 0x78, 0xCA, 8,
                                8);
        }
    }
}

void CEnemyGekirin::Step(void) {
    if (state == GEKIRIN_STATE_BREAK) {
        frame += 1;
        if (gekirin_anim[frame] == 0) {
            state = GEKIRIN_STATE_NONE;
        }
    }
}

void CEnemyLifeGage::SetView(int view) {
    if (view != 0) {
        if (this->view == 0) {
            scale = 0.5f;
        }
    } else {
        if (this->view != 0) {
            scale = 1.0f;
        }
    }
    this->view = view;
}

void CEnemyLifeGage::Set(float *pos, int max_hp, int hp, int gekirin_num, int screen) {
    int i;

    sceVu0CopyVector(this->pos, pos);
    this->max_hp = max_hp;
    this->hp = hp;
    this->screen = screen;
    for (i = 0; i < ENEMY_LIFE_GAGE_GEKIRIN_MAX; i++) {
        if (i > gekirin_num - 1 && gekirin[i].state == GEKIRIN_STATE_SHOW) {
            gekirin[i].frame = 0;
            gekirin[i].state = GEKIRIN_STATE_BREAK;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_hud", Draw__14CEnemyLifeGageFi);

void CEnemyLifeGage::Step(void) {
    int i;

    if (screen != 0) {
        scale = 1.0f;
        return;
    }
    if (view != 0) {
        if (scale < 1.0f) {
            scale += 0.005f + scale / 3.0f;
        }
        if (scale >= 1.0f) {
            scale = 1.0f;
        }
    } else {
        if (scale > 0.0f) {
            scale -= 0.01f + scale / 2.0f;
        }
        if (scale <= 0.1f) {
            scale = 0.0f;
        }
    }
    for (i = 0; i < ENEMY_LIFE_GAGE_GEKIRIN_MAX; i++) {
        gekirin[i].Step();
    }
}

void CEnemyLifeGage::ResetGekirin(int gekirin_num) {
    int i;

    for (i = 0; i < ENEMY_LIFE_GAGE_GEKIRIN_MAX; i++) {
        gekirin[i].frame = 0;
        if (i < gekirin_num) {
            gekirin[i].state = GEKIRIN_STATE_SHOW;
        } else {
            gekirin[i].state = GEKIRIN_STATE_NONE;
        }
    }
}

void CEnemyLifeGage::Initialize(s32 gekirin_num) {
    this->ResetGekirin(gekirin_num);
    hp = 0;
    max_hp = 0;
    view = 0;
    scale = 0.0f;
}

void CDamageScore::SetValue(float *pos, int value) {
    sceVu0CopyVector(this->pos, pos);
    alpha = 0;
    phase = DAMAGE_SCORE_PHASE_APPEAR;
    active = 1;
    sprite = 0;
    sprintf(text, "%d", value);
    length = strlen(text);
    for (int i = 0; i < length; i++) {
        bounce[i] = 3.1415927f;
    }
}

void CDamageScore::SetColor(s16 red, s16 green, s16 blue) {
    color[0] = red;
    color[1] = green;
    color[2] = blue;
}

void CDamageScore::SetSprite(float *pos, int u, int v, int w, int h) {
    sceVu0CopyVector(this->pos, pos);
    alpha = 0;
    phase = DAMAGE_SCORE_PHASE_APPEAR;
    active = 1;
    sprite = 1;
    bounce[0] = 3.1415927f;
    sprite_w = w;
    sprite_h = h;
    sprite_u = u;
    sprite_v = v;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_hud", Draw__12CDamageScoreFv);

void CDamageScore::Step() {
    if (active != 0) {
        if (sprite != 0) {
            if (phase == DAMAGE_SCORE_PHASE_APPEAR) {
                bounce[0] = bounce[0] - 0.3926991f;
                if (bounce[0] < -3.1415927f) {
                    bounce[0] = -3.1415927f;
                    phase = DAMAGE_SCORE_PHASE_FADE;
                }
                if (alpha < 0x80) {
                    alpha = alpha + 0xC;
                }
            }
            if (phase == DAMAGE_SCORE_PHASE_FADE) {
                alpha -= 4;
                if (alpha < 0) {
                    active = 0;
                }
            }
        }
        if (sprite == 0) {
            if (phase == DAMAGE_SCORE_PHASE_APPEAR) {
                for (int i = 0; i < length; i++) {
                    bounce[i] = bounce[i] - (3.1415927f / (10.0f + (2.0f * (float)i)));
                    if (bounce[i] < -3.1415927f) {
                        bounce[i] = -3.1415927f;
                        if (i == length - 1) {
                            phase = DAMAGE_SCORE_PHASE_FADE;
                        }
                    }
                }
                if (alpha < 0x80) {
                    alpha = alpha + 6;
                }
            }
            if (phase == DAMAGE_SCORE_PHASE_FADE) {
                alpha -= 6;
                if (alpha < 0) {
                    active = 0;
                }
            }
        }
    }
}

void CDamageScore2::SetValue(int chara_no, int value, float height) {
    this->chara_no = chara_no;
    alpha = 0;
    this->value = value;
    phase = DAMAGE_SCORE2_PHASE_JUMP;
    offset_y = 0.0f;
    this->height = 2.0f * height;
    progress = 0.0f;
    sprintf(text, "%d", this->value);
    length = strlen(text);
}

void CDamageScore2::Draw(CScene *scene) {
    if (phase != DAMAGE_SCORE2_PHASE_NONE && length > 0) {
        CCharacter2 *character = scene->GetCharacter(chara_no);
        if (character != NULL) {
            mgCFrame *frame = character->GetFrame();
            if (frame != NULL) {
                CPreSprite sprite;
                int screen[4];
                sceVu0FVECTOR position;
                sprite.Initialize(NULL, NULL);
                sprite.Preset2D();
                sprite.Coord(1);
                sprite.Begin(6);
                sprite.Texture(TEX_SystenFrame);
                sprite.AlphaTestEnable(1);
                frame->GetWorldPosition0(position);
                position[1] += height;
                position[3] = 1.0f;
                for (int index = 0; text[index] > 0; index++) {
                    if (mgTransWorldPrim(screen, position)) {
                        screen[0] -= (length * 14 / 2) << 4;
                        screen[0] += (index * 14) << 4;
                        screen[1] += (int)offset_y << 4;
                        int digit = text[index] - '0';
                        sprite.Color(220, 96, 96, (int)(128.0f * alpha));
                        sprite.TextureCrd(digit * 12 + 78, 162);
                        sprite.Vertex4(screen[0], screen[1], 0);
                        sprite.TextureCrd(digit * 12 + 90, 179);
                        sprite.Vertex4(screen[0] + 224, screen[1] + 304, 0);
                    }
                }
                sprite.End();
            }
        }
    }
}

void CDamageScore2::Step() {
    if (phase != DAMAGE_SCORE2_PHASE_NONE) {
        if (phase == DAMAGE_SCORE2_PHASE_JUMP) {
            progress += 0.1f;
            alpha += 0.1f;
            offset_y = (int)(64.0f * sinf(-2.3561945f * progress));
            if (progress > 1.0f) {
                phase = DAMAGE_SCORE2_PHASE_HOLD;
                progress = 0.0f;
                alpha = 1.0f;
            }
        }
        if (phase == DAMAGE_SCORE2_PHASE_HOLD) {
            progress += 0.05f;
            if (progress > 1.0f) {
                phase = DAMAGE_SCORE2_PHASE_FADE;
                progress = 0.0f;
            }
        }
        if (phase == DAMAGE_SCORE2_PHASE_FADE) {
            progress += 0.125f;
            alpha -= 0.125f;
            offset_y += (int)(8.0f * progress);
            if (progress > 1.0f) {
                phase = DAMAGE_SCORE2_PHASE_NONE;
                alpha = 0.0f;
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_hud", Draw__12CLockOnModelFv);

void CLockOnModel::DrawMess(int tex_block) {
    if (name != NULL) {
        if (mes->MakeAnd3DPosSet(name, pos, 0, -48) == 0) {
            ClsMes *message = mes;
            message->draw_speed = message->GetDrawSpeedDef();
            message->mes_no = -1;
            message->unk_1e40 = 0;
            message->open = 0;
            message->fade = 0.0f;
            message->fukidashi_centre_x = -1;
            message->fukidashi_centre_y = -1;
        }
        mes->Step();
        mgTexManager.ReloadTexture(tex_block, (sceVif1Packet *)NULL);
        mes->DrawMesWin();
    }
}

void CLockOnModel::Step() {
    angle += 0.06981317f;
    if (angle > 3.1415927f) {
        angle -= 6.2831855f;
    }
}

void CWarningGage2::Step() {
    time += 1;
    if (time >= 0x28) {
        time = 0;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_hud", Draw__13CWarningGage2Fv);

void CLockOnModel::Initialize(CScene *scene) {
    this->scene = scene;
    name = NULL;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_hud", gekirin_anim__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_hud", at_1221__2__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_hud", __vt__12CLockOnModel__DATA);
