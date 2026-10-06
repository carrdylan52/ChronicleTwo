#include "common.h"
#include "pot.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "effscript.hpp"
#include "mapparts.hpp"
#include "mg_drawenv.hpp"
#include "mg_frame.hpp"
#include "scenesnd.hpp"
#include "mainloop.hpp"
#include "dng_main.hpp"
#include "scene.hpp"
#include "mg_math.hpp"
#include "sound.hpp"
#include <cmath>
#include <cstdio>

float box_offset[12][4] = {
    {-5.19999981f, 9.19999981f, -5.19999981f, 1.0f},
    {5.0f, 9.39999962f, 5.19999981f, 1.0f},
    {-5.19999981f, 0.0f, 5.19999981f, 1.0f},
    {-5.19999981f, 0.0f, -5.19999981f, 1.0f},
    {5.19999981f, 5.0f, 0.0f, 1.0f},
    {0.0f, 5.0f, -5.19999981f, 1.0f},
    {5.5999999f, 5.0f, 3.0f, 1.0f},
    {-3.0f, 5.5999999f, 2.20000005f, 1.0f},
    {3.19999981f, 6.80000019f, 3.60000014f, 1.0f},
    {-3.60000014f, 0.0f, -2.79999995f, 1.0f},
    {-4.0f, 3.0f, 3.0f, 1.0f},
    {5.5999999f, 5.0f, -3.19999981f, 1.0f},
};
float iwa0_offset[10][4] = {
    {-4.4000001f, 11.1999998f, 4.19999981f, 1.0f},
    {-3.70000005f, 10.8000002f, -2.39999986f, 1.0f},
    {-4.5999999f, 4.79999971f, 4.0f, 1.0f},
    {-2.20000005f, 2.0f, 2.0f, 1.0f},
    {-4.5999999f, 3.79999995f, -2.5999999f, 1.0f},
    {2.0f, 10.8000002f, 4.0f, 1.0f},
    {2.79999995f, 11.0f, -3.0f, 1.0f},
    {2.0f, 4.0f, 4.0f, 1.0f},
    {3.19999981f, 4.79999971f, -3.19999981f, 1.0f},
    {2.0f, 2.0f, -3.0f, 1.0f},
};
float iwa1_offset[9][4] = {
    {-4.36399984f, 12.0220003f, 0.0219999999f, 1.0f},
    {-0.843999982f, 12.5240002f, 3.80200005f, 1.0f},
    {3.51200008f, 12.9960003f, 1.84800005f, 1.0f},
    {2.76200008f, 12.198f, 3.03799987f, 1.0f},
    {-2.32999992f, 11.2320004f, -3.30000019f, 1.0f},
    {2.21600008f, 5.97599983f, 0.167999998f, 1.0f},
    {-2.11199999f, 2.66400003f, -1.778f, 1.0f},
    {1.56200004f, 1.33000004f, 0.491999984f, 1.0f},
    {-1.76800001f, 4.29199982f, -1.79399991f, 1.0f},
};

// Code (.text)
void CalcReflectionVector(float *direction, float *normal, float *out_reflection) {
    sceVu0FVECTOR unit_normal;
    float dx;
    float dy;
    float dz;
    float dot;
    sceVu0Normalize(unit_normal, normal);
    unit_normal[3] = 1.0f;
    dx = -1.0f * direction[0];
    dy = -1.0f * direction[1];
    dz = -1.0f * direction[2];
    dot = dx * unit_normal[0] + dy * unit_normal[1] + dz * unit_normal[2];
    out_reflection[0] = 2.0f * unit_normal[0] * dot - dx;
    out_reflection[1] = 2.0f * unit_normal[1] * dot - dy;
    out_reflection[2] = 2.0f * unit_normal[2] * dot - dz;
    out_reflection[3] = 1.0f;
}

void CFragment::Draw(float *origin, float alpha) {
    if (active != 0 && frame != NULL) {
        mgCFrameAttr *attr = frame->attr;
        sceVu0FVECTOR relative;
        if (attr != NULL) {
            attr->draw |= 1;
            attr->obj_alpha = alpha;
            frame->attr = attr;
        }
        sceVu0SubVector(relative, position, origin);
        frame->SetPosition(relative);
        frame->SetRotation(rotation);
    }
}

void CFragment::Step(CCPoly *polys, int poly_num) {
    sceVu0FVECTOR next_position;
    sceVu0FVECTOR hit_position;
    sceVu0FVECTOR reflected;
    int hit_indices[32];
    sceVu0FVECTOR hit_points[64];
    int hit;
    int hit_count;
    int moved;
    int i;
    int axis;
    float delta_x;
    float delta_z;
    if (active != 0) {
        moved = 1;
        next_position[0] = position[0] + velocity[0];
        next_position[1] = position[1] + velocity[1];
        next_position[2] = position[2] + velocity[2];
        next_position[3] = 1.0f;
        mgDistVector(velocity);
        hit = CheckHit(polys, poly_num, position, next_position, hit_position, moved, 4);
        if (0 <= hit) {
            moved = 0;
            CalcReflectionVector(velocity, polys[hit].normal, reflected);
            sceVu0ScaleVector(reflected, reflected, 0.5f);
            reflected[3] = 1.0f;
        } else {
            hit_count = CheckHits(polys, poly_num, position, next_position, 0x20, hit_indices,
                                 hit_points, moved, 0);
            if (hit_count != 0) {
                for (i = 0; i < hit_count; i++) {
                    s16 kind = polys[hit_indices[i]].area_kind;
                    switch (kind) {
                        case 1:
                        case 7: {
                            CEffectScriptMan *effects =
                                (CEffectScriptMan *)GetMainScene()->GetEffect(0);
                            if (effects != NULL) {
                                effects->CreateEffSpt("\221\253\220\205\203p\203V\203\203", -1, 0);
                                effects->SetScriptVect1(hit_points[i], -1, -1);
                            }
                            break;
                        }
                    }
                }
            }
        }
        delta_x = next_position[0] - position[0];
        delta_z = next_position[2] - position[2];
        if (moved != 0) {
            position[0] = next_position[0];
            position[1] = next_position[1];
            position[2] = next_position[2];
            position[3] = 1.0f;
            velocity[0] += gravity[0];
            velocity[1] += gravity[1];
            velocity[2] += gravity[2];
            velocity[3] = 1.0f;
        } else {
            position[0] = hit_position[0];
            position[1] = hit_position[1];
            position[2] = hit_position[2];
            position[3] = 1.0f;
            velocity[0] = reflected[0];
            velocity[1] = reflected[1];
            velocity[2] = reflected[2];
            velocity[3] = 1.0f;
        }
        rotation[0] = rotation[0] - 0.0625f * delta_x;
        rotation[1] = 0.0f;
        rotation[2] -= 0.0625f * delta_z;
        for (axis = 0; axis < 3; axis++) {
            if (rotation[axis] < -3.1415927f) {
                rotation[axis] += 6.2831855f;
            }
            if (3.1415927f < rotation[axis]) {
                rotation[axis] -= 6.2831855f;
            }
        }
    }
}

void CFragment::Set(float *position, float *velocity) {
    active = 1;
    sceVu0CopyVector(this->position, position);
    sceVu0CopyVector(this->velocity, velocity);
    gravity[0] = 0.0f;
    gravity[1] = -0.5f;
    gravity[2] = 0.0f;
    gravity[3] = 1.0f;
}

void CFragment::Init() {
    no = -1;
    active = 0;
    InitVector(position);
    InitVector(velocity);
    InitVector(gravity);
    InitVector(rotation);
    frame = NULL;
}

void CBPot::Clash(float *position, float *normal, float *velocity) {
    sceVu0FVECTOR shard_velocity;
    sceVu0FVECTOR shard_position;
    timer = BPOT_BREAK_TIME;
    sceVu0CopyVector(this->position, position);
    if (this->parts != NULL) {
        this->parts->Show(1);
        this->parts->SetPosition(position);
    }
    if (this->frame != NULL) {
        mgCFrameAttr *attr = this->frame->attr;
        if (attr != NULL) {
            attr->draw |= 1;
            this->frame->attr = attr;
        }
        this->frame->SetPosition(position);
    }

    for (int i = 0; i < fragment_num; i++) {
        {
            float *source = offset[i];
            sceVu0ScaleVector(shard_velocity, source, 0.25f);
        }
        shard_velocity[3] = 1.0f;
        sceVu0AddVector(shard_velocity, shard_velocity, velocity);
        shard_velocity[3] = 1.0f;
        sceVu0AddVector(shard_position, position, offset[i]);
        shard_position[3] = 1.0f;
        fragment[i].Set(shard_position, shard_velocity);
    }
}

void CBPot::Step() {
    mgVu0FBOX box;
    CCPoly polys[0x200];
    float fade;
    int poly_count;
    int i;
    int j;
    if (timer > 1) {
        box.max[0] = 100.0f + position[0];
        box.min[0] = position[0] - 100.0f;
        box.max[1] = 100.0f + position[1];
        box.min[1] = position[1] - 100.0f;
        box.max[2] = 100.0f + position[2];
        box.min[2] = position[2] - 100.0f;
        box.max[3] = 1.0f;
        box.min[3] = 1.0f;
        poly_count = GetMainScene()->GetColPoly(polys, box, 0x200);
        timer--;
        for (i = 0; i < fragment_num; i++) {
            fragment[i].Step(polys, poly_count);
        }
    } else if (timer == 1) {
        if (this->parts != NULL) {
            this->parts->Show(0);
        }
    }
    fade = 1.0f;
    if (timer < BPOT_FADE_TIME) {
        fade = (float)timer / BPOT_FADE_TIME;
    }
    for (j = 0; j < fragment_num; j++) {
        fragment[j].Draw(position, fade);
    }
}

int CBPot::SetObject2(int kind, CMapParts *parts) {
    char name[0x20];
    char *prefix;
    int found;
    int i;
    if (parts == NULL) {
        return 0;
    }
    this->parts = parts;
    if (kind == 0) {
        type = BPOT_TYPE_BOX;
    } else if (kind == 5) {
        type = BPOT_TYPE_ROCK1;
    } else if (kind > 0 && kind < 5) {
        type = BPOT_TYPE_ROCK0;
    } else if (kind == 6) {
        type = BPOT_TYPE_ROCK0;
    } else {
        return 0;
    }
    if (type == BPOT_TYPE_BOX) {
        fragment_num = 12;
        prefix = "box";
        offset = box_offset;
    } else if (type == BPOT_TYPE_ROCK0) {
        fragment_num = 10;
        prefix = "rock";
        offset = iwa0_offset;
    } else if (type == BPOT_TYPE_ROCK1) {
        fragment_num = 9;
        prefix = "rock";
        offset = iwa1_offset;
    } else {
        return 0;
    }
    piece = this->parts->SearchPiece("rnd_obj02-m0");
    if (piece == NULL) {
        return 0;
    }
    this->frame = piece->frame;
    if (this->frame == NULL) {
        return 0;
    }
    found = 0;
    for (i = 0; i < fragment_num; i++) {
        if (i >= BPOT_FRAGMENT_MAX) {
            return found;
        }
        mgCFrame *frame;
        sprintf(name, "%s%02d", prefix, i + 1);
        frame = this->frame->SearchFrame(name);
        if (frame != NULL) {
            fragment[i].no = found;
            fragment[i].frame = frame;
            found++;
        }
    }
    return found;
}

void CBPot::Init() {
    int i;
    this->parts = NULL;
    piece = NULL;
    this->frame = NULL;
    InitVector(position);
    timer = 0;
    type = BPOT_TYPE_NONE;
    fragment_num = 0;
    for (i = 0; i < BPOT_FRAGMENT_MAX; i++) {
        fragment[i].Init();
    }
    offset = NULL;
}

void CPot::HoldStep() {
    if (parts != NULL) {

        parts->GetPosition(position);
        sceVu0CopyVector(prev_hold_pos, hold_pos);
        sceVu0CopyVector(hold_pos, position);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/pot", FlyStep__4CPotFv);
void CPot::Clear() {
    if (parts != NULL) {
        sceVu0FVECTOR parts_position;
        sceVu0CopyVector(break_pos, position);
        parts->GetPosition(parts_position);
        parts_position[1] -= 1000.0f;
        parts->SetPosition(parts_position);
        Init(1);
    }
}

void CPot::Bakuhatsu(float *normal, float *velocity) {
    if (parts != NULL) {

        u32 se_handle = GetMainScene()->se_battle_id;
        if (BTsubo2.type == BPOT_TYPE_BOX) {
            sndSePlay(se_handle, 0x39, 0);
        } else if (BTsubo2.type == BPOT_TYPE_ROCK0) {
            sndSePlay(se_handle, 0x3A, 0);
        } else if (BTsubo2.type == BPOT_TYPE_ROCK1) {
            sndSePlay(se_handle, 0x3B, 0);
        }
        BTsubo2.Clash(position, normal, velocity);
        Clear();
    }
}

int CPot::Step() {
    int result;
    switch (state) {
        case POT_STATE_HOLD:
            HoldStep();
            break;
        case POT_STATE_FLY:
            result = FlyStep();
            if (result == POT_STEP_BREAK) {
                return POT_STEP_BREAK;
            }
            if (result == POT_STEP_TIMEOUT) {
                return POT_STEP_TIMEOUT;
            }
        default:
            break;
    }
    return POT_STEP_NONE;
}

void CPot::Throw() {
    if (parts != NULL) {
        sceVu0FVECTOR player_position;
        state = POT_STATE_FLY;
        fly_time = 0;
        position[0] = hold_pos[0];
        position[1] = hold_pos[1];
        position[2] = hold_pos[2];
        position[3] = 1.0f;
        CCharacter2 *player = GetMainScene()->GetCharacter(0);
        if (player != NULL) {
            player->GetRotation(player_position);
            velocity[0] = 10.0 * sin(player_position[1]);
            velocity[1] = 2.5f;
            velocity[2] = 10.0 * cos(player_position[1]);
            velocity[3] = 1.0f;
        }
        gravity[0] = 0.0f;
        gravity[1] = -0.5f;
        gravity[2] = 0.0f;
        gravity[3] = 1.0f;
    }
}

void CPot::Hold(CMapParts *parts) {
    Init(0);
    this->parts = parts;
    state = POT_STATE_HOLD;
    HoldStep();
}

void CPot::Init(int keep) {
    state = POT_STATE_NONE;
    parts = NULL;
    InitVector(position);
    if (keep != 1) {
        InitVector(velocity);
    }
    InitVector(gravity);
    InitVector(hold_pos);
    InitVector(prev_hold_pos);
    if (keep != 1) {
        InitVector(break_pos);
    }
    fly_time = 0;
}

// Initialised data (.data)

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1196__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1323__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1324__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1325__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1326__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/pot", at_1438__4__DATA);
