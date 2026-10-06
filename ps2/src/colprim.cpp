#include "common.h"

#include "colprim.hpp"
#include "character.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "scenesnd.hpp"
#include <cstring>

// Code (.text)
int CColPrim::SetDamage(char *name, int attack_owner) {
    int row = 0;
    DAMAGE_PARAM *damage_param = Damage_Param_Table;
    for (;;) {
        if (damage_param->name[0] == 0) {
            return 0;
        }
        if (strcmp(damage_param->name, name) == 0) {
            Initialize();
            param_no = row;
            active = 1;
            param = damage_param;
            owner = attack_owner;
            damage = damage_param->damage;
            step_count = 0;
            coord_type = COLPRIM_COORD_VECTOR;
            attacker = -1;
            range = 10000.0f;
            memcpy(element, damage_param->element, sizeof(element));
            status = damage_param->status;
            if (damage_param->target & DAMAGE_TARGET_BY_OWNER) {
                if (attack_owner == 0) {
                    target = DAMAGE_TARGET_MONSTER;
                } else {
                    target = DAMAGE_TARGET_PLAYER;
                }
            } else {
                target = damage_param->target;
            }
            return 1;
        }
        ++row;
        damage_param++;
    }
    return 0;
}

void CColPrim::SetCoord(float *position, float new_radius) {
    position[3] = 1.0f;
    if (step_count == 0) {
        sceVu0CopyVector(pos[0], position);
        sceVu0CopyVector(old_pos[0], position);
        sceVu0CopyVector(origin, position);
    } else {
        sceVu0CopyVector(old_pos[0], pos[0]);
        sceVu0CopyVector(pos[0], position);
    }
    radius = new_radius;
    coord_type = COLPRIM_COORD_VECTOR;
}

void CColPrim::SetCoord(float *start, float *end, float new_radius) {
    start[3] = 1.0f;
    end[3] = 1.0f;
    if (step_count == 0) {
        sceVu0CopyVector(pos[0], start);
        sceVu0CopyVector(pos[1], end);
        sceVu0CopyVector(old_pos[0], start);
        sceVu0CopyVector(old_pos[1], end);
        sceVu0CopyVector(origin, start);
    } else {
        sceVu0CopyVector(old_pos[0], pos[0]);
        sceVu0CopyVector(old_pos[1], pos[1]);
        sceVu0CopyVector(pos[0], start);
        sceVu0CopyVector(pos[1], end);
    }
    radius = new_radius;
    coord_type = COLPRIM_COORD_VECTOR;
}

void CColPrim::SetCoord(mgCFrame *start, float new_radius) {
    frame[0] = start;
    frame[1] = NULL;
    radius = new_radius;
    coord_type = COLPRIM_COORD_FRAME;
    if (step_count == 0 && start) start->GetWorldPosition0(origin);
}

void CColPrim::SetCoord(mgCFrame *start, mgCFrame *end, float new_radius) {
    frame[0] = start;
    frame[1] = end;
    radius = new_radius;
    coord_type = COLPRIM_COORD_FRAME;
    if (step_count == 0 && start) start->GetWorldPosition0(origin);
}

int CColPrim::IsHit(CScene *scene, int chara_id) {
    if (active == 0) return 0;
    if (param == 0) return 0;
    CCharacter2 *chara = scene->GetCharacter(chara_id);
    if (!chara) return 0;
    int chara_type = scene->GetType(1, chara_id);
    if (chara_type == 1 && !(target & DAMAGE_TARGET_PLAYER)) return 0;
    if (chara_type == 3 && !(target & DAMAGE_TARGET_MONSTER)) return 0;
    if (chara_id != -1 && (hit_mask & (1 << chara_id))) return 0;
    int segment_count = 0;
    int entry_no = 0;
    int hit = 0;
    sceVu0FVECTOR entry_position;
    sceVu0FVECTOR displacement;
    sceVu0FVECTOR starts[8];
    sceVu0FVECTOR ends[8];
    if (param->shape & DAMAGE_SHAPE_POINT) {
        sceVu0CopyVector(starts[segment_count], pos[0]);
        segment_count++;
        if ((param->shape & DAMAGE_SHAPE_TRAIL) && step_count > 0) {
            sceVu0SubVector(displacement, old_pos[0], pos[0]);
            sceVu0ScaleVector(displacement, displacement, 0.5f);
            sceVu0AddVector(starts[segment_count], displacement, pos[0]);
            segment_count++;
        }
    }
    if (param->shape & DAMAGE_SHAPE_LINE) {
        sceVu0CopyVector(starts[segment_count], pos[0]);
        sceVu0CopyVector(ends[segment_count], pos[1]);
        segment_count++;
        if ((param->shape & DAMAGE_SHAPE_TRAIL) && step_count > 0) {
            sceVu0CopyVector(starts[segment_count], pos[0]);
            sceVu0CopyVector(ends[segment_count], old_pos[0]);
            sceVu0SubVector(displacement, pos[0], pos[1]);
            sceVu0ScaleVector(displacement, displacement, 0.5f);
            sceVu0AddVector(starts[segment_count + 1], pos[1], displacement);
            sceVu0SubVector(displacement, old_pos[0], old_pos[1]);
            sceVu0ScaleVector(displacement, displacement, 0.5f);
            sceVu0AddVector(ends[segment_count + 1], old_pos[1], displacement);
            segment_count += 2;
        }
    }
    CHARA_ENTRY_OBJECT *entry;
    while ((entry = chara->GetEntryObjectPos(2, entry_no, entry_position)) != 0) {
        if (entry->enable == 0) {
            ++entry_no;
            continue;
        }
        if (param->shape & DAMAGE_SHAPE_POINT) {
            for (int i = 0; i < segment_count; ++i) {
                if (mgDistVector(starts[i], entry_position) <= 2.0f * (radius + entry->unk_04)) {
                    entry_position[3] = 1.0f;
                    sceVu0CopyVector(hit_pos, entry_position);
                    if (param->shape & DAMAGE_SHAPE_TRAIL) {
                        sceVu0SubVector(hit_vec, pos[0], old_pos[0]);
                    } else {
                        sceVu0SubVector(hit_vec, entry_position, starts[i]);
                    }
                    hit_vec[1] = 0.0f;
                    sceVu0Normalize(hit_vec, hit_vec);
                    hit = true;
                    break;
                }
            }
        }
        if (param->shape & DAMAGE_SHAPE_LINE) {
            for (int i = 0; i < segment_count; ++i) {
                if (mgDistLinePoint(entry_position, starts[i], ends[i], hit_pos) <= 2.0f * (radius + entry->unk_04)) {
                    sceVu0SubVector(hit_vec, pos[1], old_pos[1]);
                    hit = true;
                    break;
                }
            }
        }
        if (hit) {
            if (chara_id != -1 && !param->multi_hit) hit_mask |= 1 << chara_id;
            ++hit_num;
            return 1;
        }
        ++entry_no;
    }
    return 0;
}

int CColPrim::IsReversVec(CColPrim *attack) {
    if (!active) {
        return 0;
    }
    if (!param) {
        return 0;
    }
    if (attack->attacker != 0) {
        return 0;
    }
    attack->pos[0][3] = 1.0f;
    pos[0][3] = 1.0f;
    if (mgDistVector(pos[0], attack->pos[0]) <= 2.0f * (radius + 2.0f * attack->radius)) {
        return 1;
    }
    return 0;
}

void CColPrim::GetReversVec(float *out_vector) {
    if (active && param) sceVu0SubVector(out_vector, old_pos[0], pos[0]);
}

void CColPrim::DebugDraw() {}

int CColPrim::Step() {
    if (!active) return 0;
    if (coord_type & COLPRIM_COORD_FRAME) {
        if (step_count == 0) {
            for (int i = 0; i < 2; ++i) {
                if (frame[i]) frame[i]->GetWorldPosition0(pos[i]);
                sceVu0CopyVector(old_pos[i], pos[i]);
            }
        } else {
            for (int i = 0; i < 2; ++i) {
                sceVu0CopyVector(old_pos[i], pos[i]);
                if (frame[i]) frame[i]->GetWorldPosition0(pos[i]);
            }
        }
    }
    ++step_count;
    if (life != -1 && step_count >= life) active = 0;
    return 1;
}

void CColPrim::Delete(int attack_owner) {
    if (active) {
        if (attack_owner == -1) {
            active = 0;
        } else if (owner == attack_owner) {
            active = 0;
        }
    }
}

void CColPrim::Initialize(void) {
    active = 0;
    owner = -1;
    hit_mask = 0;
    step_count = 0;
    life = -1;
    hit_num = 0;
    reversed = 0;
    has_gift = 0;
    unk_34 = 0;
    frame[1] = NULL;
    frame[0] = NULL;
    radius = 0;
    unk_8c = -1;
}

CColPrim *CColPrimMan::GetPrim() {
    for (int i = 0; i < COLPRIM_MAX; ++i)
        if (!prim[i].active) { prim[i].id = i; return &prim[i]; }
    return NULL;
}

CColPrim *CColPrimMan::GetID2Prim(int id) {
    if (id < 0 || id >= COLPRIM_MAX) return NULL;
    prim[id].id = id;
    return &prim[id];
}

int CColPrimMan::ActivePrimNum() {
    int count = 0;
    for (int i = 0; i < COLPRIM_MAX; ++i) if (prim[i].active) ++count;
    return count;
}

void CColPrimMan::Delete(int owner) {
    for (int i = 0; i < COLPRIM_MAX; ++i) prim[i].Delete(owner);
}

CColPrim *CColPrimMan::CheckHit(int chara_id) {
    for (int i = 0; i < COLPRIM_MAX; ++i)
        if (prim[i].IsHit(scene, chara_id)) return &prim[i];
    return NULL;
}

CColPrim *CColPrimMan::IsReversVec(CColPrim *attack) {
    for (int i = 0; i < COLPRIM_MAX; ++i)
        if (attack->id != i && prim[i].IsReversVec(attack)) return &prim[i];
    return NULL;
}

void CColPrimMan::Step() {
    for (int i = 0; i < COLPRIM_MAX; ++i) prim[i].Step();
}

void CColPrimMan::Initialize(CScene *new_scene) {
    scene = new_scene;
    for (int i = 0; i < COLPRIM_MAX; ++i) { prim[i].Initialize(); prim[i].id = i; }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/colprim", Damage_Param_Table__DATA);
