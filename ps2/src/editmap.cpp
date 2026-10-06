#include "common.h"
#include "editmap.hpp"
#include "editdata.hpp"
#include "editriver.hpp"
#include "funcpoint.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_sprite.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "scriptinterpreter.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

/**
 * Name returned for an edit map.
 */
char *CEditMapName = "CEditMap";

static mgCMemory *emapStack; /**< Storage for Georama definition tables. */
static CEditInfoMngr *emapInfo; /**< Manager receiving the parsed Georama definitions. */
static int emapFixNum; /**< Capacity of the fixed placement table. */
static int emapFixIdx; /**< Next fixed placement slot. */
static ePlaceData *emapFix; /**< Fixed placement records being parsed. */
static int emapInitNum; /**< Capacity of the initial placement table. */
static int emapInitIdx; /**< Next initial placement slot. */
static ePlaceData *emapInit; /**< Initial placement records being parsed. */

// Code (.text)
char *CEditMap::Iam(void) {
    return CEditMapName;
}

void CEditMap::Initialize() {
    int i;
    area_no = -1;
    edit_parts_max = 0;
    edit_parts = 0;
    parts_heap.Init();
    place_log_max = 0;
    place_log = 0;

    river_parts[0] = 0;
    river_piece[0] = 0;
    river_parts[1] = 0;
    river_piece[1] = 0;
    river_parts[2] = 0;
    river_piece[2] = 0;
    river_parts[3] = 0;
    river_piece[3] = 0;
    river_parts[4] = 0;
    river_piece[4] = 0;
    river_parts[5] = 0;
    river_piece[5] = 0;
    river_parts[6] = 0;
    river_piece[6] = 0;
    river_parts[7] = 0;
    river_piece[7] = 0;
    water_piece = 0;
    river_info = 0;
    mask_piece[0] = 0;
    river_poly_margin = 0;
    grid_max = EDIT_MAP_GRID_MAX;
    i = 0;

    for (; i < grid_max; i++) {
        grid[i] = NULL;
    }
    ClearHouse();
    focus_parts = -1;
    frame = 0;
    balance_moved = 0;
    CMap::Initialize();
}

void CEditMap::ClearGrid() {
    int i = 0;
    for (; i < grid_max; i++) {

        CEditGrid *grid = this->grid[i];
        if (grid != 0) {
            grid->Clear();
        }
    }
}

void CEditMap::ClearHouse(void) {
    s32 house_no = 0;
    do {
        memset(&house[house_no], 0, sizeof(CEditHouse));
        house_no++;
    } while (house_no < EDIT_MAP_HOUSE_MAX);
}

#ifdef NONMATCHING
void CEditMap::ClearAllParts() {
    int i;
    int log_index;
    ePlaceData *initial;
    CEditParts *placed;
    int k;
    CEditPartsInfo *info;
    parts_heap.ClearHeapMem();
    i = 0;

    for (; i < edit_parts_max; i++) {
        edit_parts[i].Initialize();
    }
    log_index = 0;

    for (; log_index < place_log_max; log_index++) {
        place_log[log_index].parts_no = -1;
    }
    ClearGrid();
    ClearHouse();
    k = 0;

    for (; k < info_mngr.fix_parts_num; k++) {
        initial = &info_mngr.fix_parts[k];
        sceVu0FVECTOR rotation = {0.0f, 0.0f, 0.0f, 0.0f};
        rotation[1] = GetEditAngle(initial->angle);
        info = info_mngr.GetePartsInfoAtID(initial->id);
        placed = 0;
        if (info != 0) {
            placed = PlaceEditParts(info->edit_name, initial->position, rotation);
        }
        if (area_no == 1 && placed != 0) {
            CMapParts *anchor = NULL;
            switch (k) {
                case 0:
                    anchor = GetPlaceParts("p09_g0201");
                    break;
                case 1:
                    anchor = GetPlaceParts("p09_g0201-1");
                    break;
                case 2:
                    anchor = GetPlaceParts("p08_g0201");
                    break;
                case 3:
                    anchor = GetPlaceParts("p08_g0201-1");
                    break;
            }
            placed->ground = anchor;
        }
    }
    focus_parts = -1;
    frame = 0;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", ClearAllParts__8CEditMapFv);
#endif

void CEditMap::InitialPlaceParts(CEditData *data) {
    EP_PLACE_INFO place_info;
    float rotation[4];
    int i;
    ePlaceData *placement;
    CEditPartsInfo *info;
    if (data->save_count != 0) {
        return;
    }
    i = 0;

    for (; i < info_mngr.init_parts_num; i++) {
        placement = &info_mngr.init_parts[i];
        info = GetePartsInfoAtID(placement->id);
        if (info != 0) {
            mgZeroVector(rotation);
            rotation[1] = GetEditAngle(placement->angle);
            if (CheckEditParts(info, placement->position, rotation[1], &place_info)) {
                int built = BuildEditParts(placement->id);
                if (built >= 0) {
                    PlaceEditParts(built, &place_info, placement->position, rotation, 0);
                }
            } else {
                printf("init place err %d %s\n", i, info->edit_name);
            }
        }
    }
}

int CEditMap::GetPoly(int kind, CCPoly *polys, mgVu0FBOX &box, int max) {
    int total;
    CEditParts *part;
    int i;
    int g;
    int j;
    int count;

    total = CMap::GetPoly(kind, polys, box, max);
    part = edit_parts;
    max -= total;
    polys += total;
    for (i = 0; i < edit_parts_max; i++, part++) {
        int is_free = part->name[0] == 0;
        if (is_free) {
            continue;
        }
        if (part->state != EDIT_PARTS_STATE_PLACED) {
            continue;
        }
        count = ((CMapParts *)part)->GetPoly(kind, polys, box, max);
        for (j = 0; j < count; j++, polys++) {
            polys->parts_no = ((s16)(u16)i) | 0x1000;
        }
        max -= count;
        total += count;
        if (max <= 0) {
            return total;
        }
    }
    if (kind == 1) {
        for (g = 0; g < grid_max; g++) {
            if (grid[g] != 0) {
                count = grid[g]->GetRiverPoly(polys, box, max, river_poly_margin);
                for (j = 0; j < count; j++, polys++) {
                    polys->ignore_mask = 0x10;
                }

                polys += count;
                max -= count;
                total += count;
                if (max < 0) {
                    return total;
                }
            }
        }
    }
    return total;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", CreateTable__8CEditMapFP9mgCMemoryii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", __ct__10CEditPartsFv);
CEditPartsInfo *CEditMap::GetePartsInfo(int no) {
    return info_mngr.GetePartsInfo(no);
}

CEditPartsInfo *CEditMap::GetePartsInfo(char *name) {
    return info_mngr.GetePartsInfo(name);
}

CEditPartsInfo *CEditMap::GetePartsInfoAtID(int id) {
    return info_mngr.GetePartsInfoAtID(id);
}

CEditPartsInfo *CEditMap::GetePartsInfoAtType(int type) {
    return info_mngr.GetePartsInfoAtType(type);
}

CEditPartsInfo *CEditMap::GetePartsInfoAtPlaceID(int no) {
    CEditParts *placed;

    placed = GetePlaceParts(no);
    if (placed != NULL) {
        return placed->info;
    }
    return 0;
}

int CEditMap::eNewPlaceParts() {
    int i;
    for (i = 0; i < edit_parts_max; i++) {
        int is_free = edit_parts[i].name[0] == 0;
        if (is_free) {
            return i;
        }
    }
    return -2;
}

CEditHouse *CEditMap::eNewHouseInfo() {
    int i;
    for (i = 0; i < EDIT_MAP_HOUSE_MAX; i++) {
        if (house[i].active == 0) {
            return &house[i];
        }
    }
    return 0;
}

CEditParts *CEditMap::GetePlaceParts(int no) {
    if (no < 0) {
        return 0;
    }
    if (no < 0 || no >= edit_parts_max) {
        return 0;
    }
    return &edit_parts[no];
}

CEditParts *CEditMap::GetePlaceParts(char *name) {
    int i;
    CEditParts *slot;

    i = 0;
    for (; i < edit_parts_max; i++) {
        slot = &edit_parts[i];
        int is_free = slot->name[0] == 0;
        if (!is_free) {
            if (slot->state != EDIT_PARTS_STATE_NONE) {
                if (slot->info != 0) {
                    CEditPartsInfo *info = slot->info;
                    if (info->parts_name != 0) {
                        if (strcmp(info->parts_name, name) == 0) {
                            return slot;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int CEditMap::GetePlaceIDList(int *out_no, int max) {
    int found = 0;
    int i = 0;
    int parts_index = 0;
    int output_index = 0;
    for (; i < edit_parts_max; i++) {
        CEditParts *part = &edit_parts[parts_index];
        int is_free = part->name[0] == 0;
        if (!is_free) {
            if (found >= max) {
                break;
            }
            found++;
            out_no[output_index++] = i;
        }

        parts_index++;
    }
    return found;
}

void CEditMap::GetRotMatrix(float (*out_matrix)[4], int angle) {
    angle = angle % EDIT_ANGLE_MAX;
    mgUnitMatrix(out_matrix);
    if (angle == 0) {
        return;
    }
    if (angle == 6) {
        out_matrix[2][2] = 0.0f;
        out_matrix[0][0] = 0.0f;
        out_matrix[0][2] = -1.0f;
        out_matrix[2][0] = 1.0f;
    } else if (angle == 12) {
        out_matrix[2][2] = -1.0f;
        out_matrix[0][0] = -1.0f;
    } else if (angle == 18) {
        out_matrix[2][2] = 0.0f;
        out_matrix[0][0] = 0.0f;
        out_matrix[0][2] = 1.0f;
        out_matrix[2][0] = -1.0f;
    } else {
        sceVu0RotMatrixY(out_matrix, out_matrix, mgAngleLimit(GetEditAngle(angle)));
    }
}

int CEditMap::GetEditAngle90(int angle) {
    angle = AngleLimit(angle);
    return angle / EDIT_ANGLE_90 * EDIT_ANGLE_90;
}

float CEditMap::GetEditAngle(int angle) {
    angle = angle % 24;
    return mgAngleLimit(6.2831855f * (float)angle / 24.0f);
}

int CEditMap::ConvEditAngle(float rot) {
    float steps;
    int whole;
    if (rot < 0.0f) {
        rot += 6.2831855f;
    }
    steps = rot / 0.2617994f;
    whole = (int)(steps);
    if (!(steps - (float)whole <= 0.5f)) {
        whole++;
    }
    return AngleLimit(whole);
}

s32 CEditMap::AngleLimit(s32 angle) {
    angle = angle % EDIT_ANGLE_MAX;
    if (angle < 0) {
        angle += EDIT_ANGLE_MAX;
    }
    return angle;
}

void CEditMap::GetEditPos(float *out_pos, float *pos) {
    if (!(pos[0] < 0.0f)) {
        out_pos[0] = (float)(int)(0.01f + pos[0]);
    }
    if (pos[0] < 0.0f) {
        out_pos[0] = (float)(int)(pos[0] - 0.01f);
    }
    if (!(pos[1] < 0.0f)) {
        out_pos[1] = (float)(int)(0.01f + pos[1]);
    }
    if (pos[1] < 0.0f) {
        out_pos[1] = (float)(int)(pos[1] - 0.01f);
    }
    if (!(pos[2] < 0.0f)) {
        out_pos[2] = (float)(int)(0.01f + pos[2]);
    }
    if (pos[2] < 0.0f) {
        out_pos[2] = (float)(int)(pos[2] - 0.01f);
    }
    out_pos[3] = pos[3];
}

s32 CEditMap::CmpEditAlt(float alt, float base_alt) {
    float difference = alt - base_alt;
    if (!(difference <= 0.5f)) {
        return -1;
    }
    s32 result = 1;
    if (!(difference < -0.5f)) {
        result = 0;
    }
    return result;
}

float CEditMap::GetEditAlt(float alt) {
    if (alt > 0.0f) {
        return (float)(int)(0.5f + alt);
    }
    return (float)(int)(alt - 0.5f);
}

int CEditMap::GetGridPos(float *pos, float *out_pos, float *out_size) {
    int i;
    CEditGrid *grid;
    int local[2];

    i = 0;
    for (; i < grid_max; i++) {
        grid = this->grid[i];
        if (grid != 0) {
            if (grid->GetLPos(local, pos[0], pos[2])) {
                grid->GetRiverPos(local[0], local[1], out_pos);
                out_size[0] = grid->step_x;
                out_size[1] = 0.0f;
                out_size[2] = grid->step_z;
                return 1;
            }
        }
    }
    return 0;
}

void CEditMap::GetMatrix(float (*out_matrix)[4], float *pos, int angle) {
    GetRotMatrix(out_matrix, angle);
    *(u_long128 *)out_matrix[3] = *(u_long128 *)pos;
    out_matrix[3][3] = 1.0f;
}

void CEditMap::GetInversMatrix(float (*out_matrix)[4], float (*matrix)[4]) {
    sceVu0InversMatrix(out_matrix, matrix);
}

int CEditMap::ConvertParts(CEditParts *parts) {
    return parts - edit_parts;
}

#ifdef NONMATCHING
int CEditMap::GetSameParts(int no) {
    CEditParts *target = GetePlaceParts(no);
    CEditPartsInfo *info;
    int i;
    if (target == 0) {
        return -1;
    }
    info = target->info;
    if (info == 0) {
        return -1;
    }
    i = 0;

    for (; i < edit_parts_max; i++) {
        CEditParts *slot = &edit_parts[i];
        int is_free = slot->name[0] == 0;
        if (!is_free) {
            if (slot->state == EDIT_PARTS_STATE_NONE) {
                if (slot->info == info) {
                    return i;
                }
            }
        }
    }
    return -1;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", GetSameParts__8CEditMapFi);
#endif

int CEditMap::BuildEditParts(int id) {
    CEditPartsInfo *info;

    info = GetePartsInfoAtID(id);
    if (info != NULL) {
        return BuildEditParts(info->edit_name);
    }
    return -1;
}

int CEditMap::GetTotalPolyn(int *out_polyn1, int *out_polyn2) {
    int poly_total;
    int vertex_sum;
    int texture_sum;
    CEditParts *part;
    int i;
    int river_count;
    CEditPartsInfo *info;

    i = 0;
    texture_sum = 0;
    vertex_sum = 0;
    part = edit_parts;
    poly_total = 0;
    for (; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part)) {
            info = part->info;
            if (info != 0) {
                poly_total += info->polyn[0];
                vertex_sum += info->polyn[1];
                texture_sum += info->polyn[2];
            }
        }
    }
    sceVu0FVECTOR river_pos = {0.0f, 0.0f, 0.0f, -1.0f};
    river_count = GetRiverNum(river_pos);
    info = GetePartsInfoAtType(EDIT_PARTS_TYPE_RIVER);
    if (info != 0) {
        texture_sum += river_count * info->polyn[2];
        vertex_sum += river_count * info->polyn[1];
        poly_total += river_count * info->polyn[0];
    }
    if (out_polyn1 != 0) {
        *out_polyn1 = vertex_sum;
    }
    if (out_polyn2 != 0) {
        *out_polyn2 = texture_sum;
    }
    return poly_total;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", BuildEditParts__8CEditMapFPc);
int CEditMap::DeleteEditParts(int no) {
    CEditParts *edit_parts = GetePlaceParts(no);
    if (edit_parts == 0) {
        return 0;
    }
    if (edit_parts->house != 0) {
        memset(edit_parts->house, 0, sizeof(CEditHouse));
    }
    if (edit_parts->unk_320 != 0) {
        parts_heap.Free((u_long128 *)edit_parts->unk_320);
    }
    edit_parts->Initialize();
    return 1;
}

int CEditMap::RemoveEditParts(int no, float *pos, RemoveInfo *info) {
    RemoveInfo *remove_info = info;
    float color[4];
    float parts_pos[4];
    float parts_rot[4];
    CEditParts *candidate;
    CEditHouse *extra;
    int id;
    int n;
    EditPlaceLog *other;
    CEditPartsInfo *candidate_info;
    int c;
    CEditPartsInfo *river_info;
    int j;
    int m;
    CEditParts *part;
    int extra_value;
    int i;
    EditPlaceLog *entry;

    part = GetePlaceParts(no);
    if (part != 0) {
        if (remove_info != 0 && remove_info->force == 0) {
            if (part->info != 0 && (part->info->attr & 0x1)) {
                return 0;
            }
        }
        id = part->GetInfoID();
        part->state = EDIT_PARTS_STATE_NONE;
        if (id >= 0 && id < EDIT_REMOVE_INFO_ID_MAX && remove_info != 0) {
            remove_info->parts_num[id] += 1;
        }
        extra = part->house;
        if (extra != 0) {
            extra_value = extra->npc_no[0];
            if (extra_value > 0 && remove_info != 0) {
                int slot = remove_info->house_num;
                remove_info->house_num = slot + 1;
                remove_info->house_npc[slot] = extra_value;
            }
        }
        if (remove_info != 0 && remove_info->color_num > 0) {
            if (part->IsFence() == 0) {
                for (c = 0; c < part->info->paint_num; c++) {
                    if (((CMapParts *)part)->GetColor(c, color) != 0) {
                        for (j = 0; j < remove_info->color_num;
                             j++) {
                            if (EditPartsCmpColor(
                                    color, remove_info->color[j]) != 0) {
                                remove_info->paint_num[j] += RePaintNum(part->info->paint_used);
                                break;
                            }
                        }
                    }
                }
            }
        }
        DeleteEditParts(no);
        if (place_log_max == 0 || (entry = place_log) == 0) {
            return 1;
        }
        for (m = 0; m < place_log_max; m++, entry++) {
            if ((s16)no == entry->parts_no) {
                other = place_log;
                for (n = 0; n < place_log_max; n++, other++) {
                    if (n != m && other->base_no == no) {
                        RemoveEditParts(other->parts_no, pos, info);
                    }
                }
                entry->parts_no = -1;
            }
        }
        return 1;
    }
    if (RemoveRiver(pos) != 0) {
        river_info = GetePartsInfoAtType(EDIT_PARTS_TYPE_RIVER);
        if (river_info != 0) {
            id = river_info->id;
            if (id >= 0 && id < EDIT_REMOVE_INFO_ID_MAX && remove_info != 0) {
                remove_info->parts_num[id] += 1;
            }
        }
        candidate = edit_parts;
        for (i = 0; i < edit_parts_max; i++, candidate++) {
            if (CheckNormalPlaceParts(candidate) != 0) {
                candidate_info = candidate->info;
                if (candidate_info != 0 && (candidate_info->attr & 0x80000)) {
                    candidate->GetPosition(parts_pos);
                    candidate->GetRotation(parts_rot);
                    if (CheckEditPartsOnRiver(candidate_info, parts_pos, parts_rot[1]) == 0) {
                        RemoveEditParts(i, parts_pos, info);
                    }
                }
            }
        }
        return 1;
    }
    return 0;
}

int CEditMap::PlaceBurnParts() {
    CEditParts *part = edit_parts;
    int i = 0;
    for (; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) && part->IsBurn()) {
            return 1;
        }
    }
    return 0;
}

int CEditMap::BurnEditParts(RemoveInfo *info) {

    sceVu0FVECTOR remove_pos = {10000000.0f, 10000000.0f, 1000000000.0f, 1.0f};
    float pos[4];
    float rotation[4];
    EP_PLACE_INFO place;
    int placed_high;
    int placed_low;
    CEditPartsInfo *high_info;
    CEditPartsInfo *low_info;
    CEditParts *part;
    int i;
    u32 flags;
    int i2;
    CEditPartsInfo *replacement;
    CEditPartsInfo *parts_info;
    int replacement_id;
    int *placed_count;
    CEditParts *part2;
    high_info = info_mngr.GetePartsInfoAtID(0x57);
    low_info = info_mngr.GetePartsInfoAtID(0x56);
    if (high_info == 0 || low_info == 0) {
        return 0;
    }
    placed_high = GetePlacePartsAtInfoID(0x57, 0, 0);
    placed_low = GetePlacePartsAtInfoID(0x56, 0, 0);
    part = edit_parts;
    i = 0;
    for (; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) != 0 && part->info != 0 &&
            (part->GetInfoID() == 0x57 || part->GetInfoID() == 0x56)) {
            RemoveEditParts(i, remove_pos, info);
        }
    }
    part2 = edit_parts;
    i2 = 0;
    for (; i2 < edit_parts_max; i2++, part2++) {
        if (CheckNormalPlaceParts(part2) != 0) {
            parts_info = part2->info;
            if (parts_info != 0) {
                flags = parts_info->attr;
                if (flags & EDIT_PARTS_ATR_BURN) {
                    part2->GetPosition(pos);
                    part2->GetRotation(rotation);
                    RemoveEditParts(i2, remove_pos, info);
                    if (flags & 0x4000) {
                        replacement = low_info;
                        replacement_id = 0x56;
                        placed_count = &placed_low;
                        if (flags & 0x40) {
                            replacement_id = 0x57;
                            replacement = high_info;
                            placed_count = &placed_high;
                        }
                        if (*placed_count < replacement->max_num &&
                            CheckEditParts(replacement, pos, rotation[1],
                                           &place) != 0) {
                            PlaceEditParts(BuildEditParts(replacement_id), &place, pos, rotation, 0);
                            *placed_count += 1;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

CEditParts *CEditMap::PlaceEditParts(char *name, float *pos, float *rot) {
    CEditParts *edit_parts = GetePlaceParts(BuildEditParts(name));
    if (edit_parts == 0) {
        return 0;
    }
    edit_parts->state = EDIT_PARTS_STATE_PLACED;
    edit_parts->SetPosition(pos);
    edit_parts->SetRotation(rot);
    edit_parts->SetScale(1.0f, 1.0f, 1.0f);
    return edit_parts;
}

CEditParts *CEditMap::PlaceEditParts(int no, EP_PLACE_INFO *place, float *pos, float *rot,
                             int *out_same) {
    CEditParts *edit_parts;
    CEditPartsInfo *info;
    edit_parts = GetePlaceParts(no);
    if (edit_parts == 0) {
        return 0;
    }
    info = edit_parts->info;
    if (info == 0) {
        return 0;
    }
    if (edit_parts->GetPartsType() != EDIT_PARTS_TYPE_RIVER) {
        if (!CreatePlaceLog(no, place)) {
            return 0;
        }
    }
    if (area_no == 1) {
        if (place != 0) {
            CEditParts *other = GetePlaceParts(place->base[0]);
            if (other != 0) {
                edit_parts->ground = other->ground;
            }
        }
    }
    edit_parts->state = EDIT_PARTS_STATE_PLACED;
    if (out_same != 0) {
        *out_same = GetSameParts(no);
    }
    if (info->attr & EDIT_PARTS_ATR_RIVER) {
        if (CheckRiverParts(pos)) {
            PlaceRiver(pos);
            edit_parts->state = EDIT_PARTS_STATE_RIVER;
            edit_parts->SetPosition(0.0f, -10000.0f, 0.0f);
        } else {
            edit_parts->state = EDIT_PARTS_STATE_NONE;
        }
        return 0;
    }
    edit_parts->SetPosition(pos);
    edit_parts->SetRotation(rot);
    edit_parts->SetScale(1.0f, 1.0f, 1.0f);
    return edit_parts;
}

s32 CEditMap::PlaceRiverParts(float *pos) {
    if (CheckRiverParts(pos) != 0) {
        PlaceRiver(pos);
        return 1;
    }
    return 0;
}

int CEditMap::CreatePlaceLog(int no, EP_PLACE_INFO *place) {
    EditPlaceLog *slot;
    int index;
    int remaining;
    int i;
    if (place == 0 || no < 0) {
        return 1;
    }
    if (place_log_max == 0 || place_log == 0) {
        return 0;
    }
    remaining = place->num;
    if (remaining < 0) {
        return 1;
    }
    slot = place_log;
    for (i = 0; i < place_log_max; i++, slot++) {
        int is_free = slot->parts_no < 0;
        if (is_free) {
            remaining--;
        }
        if (remaining <= 0) {
            break;
        }
    }
    if (remaining > 0) {
        return 0;
    }
    index = remaining;
    slot = place_log;
    for (i = 0; i < place_log_max; i++, slot++) {
        int is_free = slot->parts_no < 0;
        if (is_free) {
            slot->parts_no = no;
            slot->base_no = place->base[index];
            index++;
            if (!(index < place->num)) {
                break;
            }
        }
    }
    return 1;
}

int CEditMap::GetNearParts(CEditPartsInfo *info, float *pos, float rot_y, CEditParts **out_parts, int max) {
    float area_matrix[4][4];
    float part_matrix[4][4];
    float area_max[4];
    float area_min[4];
    float part_max[4];
    float part_min[4];
    float part_pos[4];
    float part_rotation[4];
    CEditParts *part;
    int i;
    int count;
    mgVu0FBOX *bounds;
    mgVu0FBOX *info_bounds;
    int output_index;
    if (info == 0) {
        return 0;
    }
    part = edit_parts;
    mgCreateMatrixPY(area_matrix, pos, rot_y);
    info->box.min[3] = 1.0f;
    info->box.max[3] = 1.0f;
    info_bounds = &info->box;
    mgApplyMatrix(area_max, area_min, area_matrix, info_bounds->max, info_bounds->min);
    area_max[0] += 55.0f;
    area_max[2] += 55.0f;
    area_min[0] -= 55.0f;
    area_min[2] -= 55.0f;
    count = 0;
    i = 0;
    output_index = 0;

    for (; i < edit_parts_max; i++, part++) {
        int is_free = part->name[0] == 0;
        if (is_free) {
            continue;
        }
        if (part->state != EDIT_PARTS_STATE_PLACED) {
            continue;
        }
        if (part->info == 0) {
            continue;
        }
        bounds = &part->info->box;
        part->GetPosition(part_pos);
        part->GetRotation(part_rotation);
        mgCreateMatrixPY(part_matrix, part_pos, part_rotation[1]);

        mgApplyMatrix(part_max, part_min, part_matrix, bounds->max, bounds->min);
        if (!(count < max)) {
            break;
        }
        count++;
        out_parts[output_index++] = part;
    }
    return count;
}

int CEditMap::GetNearParts(mgVu0FBOX &box, CEditParts **out_parts, int max) {
    float matrix[4][4];
    float world_max[4];
    float world_min[4];
    float pos[4];
    float rotation[4];
    int i;
    int count;
    CEditParts *part;
    mgVu0FBOX *bounds;
    int output_index = 0;
    count = 0;

    part = edit_parts;
    for (i = 0; i < edit_parts_max; i++, part++) {
        int is_free = part->name[0] == 0;
        if (is_free) {
            continue;
        }
        if (part->state != EDIT_PARTS_STATE_PLACED) {
            continue;
        }
        if (part->info == 0) {
            continue;
        }
        bounds = &part->info->box;
        part->GetPosition(pos);
        part->GetRotation(rotation);
        mgCreateMatrixPY(matrix, pos, rotation[1]);
        mgApplyMatrix(world_max, world_min, matrix, bounds->max, bounds->min);
        if (box.max[0] < world_min[0]) {
            continue;
        }
        if (box.max[2] < world_min[2]) {
            continue;
        }
        if (!(box.min[0] <= world_max[0])) {
            continue;
        }
        if (!(box.min[2] <= world_max[2])) {
            continue;
        }
        if (!(count < max)) {
            break;
        }
        count++;
        out_parts[output_index++] = part;
    }
    return count;
}

int CEditMap::GetePlaceParts(float *pos) {
    mgVu0FBOX box;
    CEditParts *near[0x200];
    int count;

    *(u_long128 *)box.max = *(u_long128 *)pos;
    *(u_long128 *)box.min = *(u_long128 *)pos;
    for (int i = 0; i < 3; i++) {
        box.max[i] += pos[3];
        box.min[i] -= pos[3];
    }
    count = GetNearParts(box, near, 0x200);
    return GetePlaceParts(pos, near, count);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", GetePlaceParts__8CEditMapFPfPP10CEditPartsi);
int CEditMap::CheckEditParts(CEditPartsInfo *info, float *pos, float rot_y, EP_PLACE_INFO *place) {
    CEditParts *near_parts[512];
    int count;
    if (info == 0) {
        return 0;
    }
    count = GetNearParts(info, pos, rot_y, near_parts, 512);
    return CheckEditParts(info, pos, rot_y, place, near_parts, count);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", GetEditPartsAlt__8CEditMapFP14CEditPartsInfoPff);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", MagnetParts__8CEditMapFP14CEditPartsInfoPfPfPP10CEditPartsi);
int CEditMap::MagnetParts(CEditPartsInfo *info, float *pos, float *rot) {
    CEditParts *near_parts[512];
    int count = GetNearParts(info, pos, rot[0], near_parts, 512);
    return MagnetParts(info, pos, rot, near_parts, count);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", CheckWallEditParts__8CEditMapFP14CEditPartsInfoPfiiP13EP_PLACE_INFO);
void CEditMap::Step() {
    frame += 1;
    CMap::Step();
}

int CEditMap::PreDraw(float *view_pos) {
    CEditParts *part;
    int i;
    CMap::PreDraw(view_pos);
    CFuncPointCheck check;
    CreateFuncCheck(&check);
    part = edit_parts;
    for (i = 0; i < edit_parts_max; i++, part++) {
        int is_free = part->name[0] == 0;
        if (!is_free) {
            part->StepFuncPoint(check);
        }
    }
    return 1;
}

int CEditMap::DrawSub(int direct) {
    float ambient[4];
    float pulsed[4];
    CEditParts *part;
    int total;
    int light_b;
    int i;
    int channel;
    int light_a;
    if (balance_moved != 0) {
        for (light_a = 0; light_a < 4; light_a++) {
            if (balance_parts[light_a] != 0) {
                balance_parts[light_a]->SetPosition(balance_base_pos[light_a]);
            }
        }
    }
    CFuncPointCheck check;
    CreateFuncCheck(&check);
    part = edit_parts;
    total = 0;
    i = 0;
    for (; i < edit_parts_max; i++, part++) {
        int is_free = part->name[0] == 0;
        if (!is_free && part->state == EDIT_PARTS_STATE_PLACED) {
            part->CopyFuncPointCheck(check);
            if (i == focus_parts) {
                float boost;
                mgGetAmbient(ambient);
                boost = 48.0f * (1.0f + sinf(6.2831855f * (float)(frame % 30) / 30.0f));
                for (channel = 0; channel < 3; channel++) {
                    pulsed[channel] = boost + ambient[channel];
                    if (!(pulsed[channel] <= 255.0f)) {
                        pulsed[channel] = 255.0f;
                    }
                }
                pulsed[3] = 128.0f;
                mgSetAmbient(pulsed);
            }
            if (direct != 0) {
                total += part->DrawDirect();
            } else {
                total += part->Draw();
            }
            if (i == focus_parts) {
                mgSetAmbient(ambient);
            }
        }
    }
    CMap::DrawSub(direct);
    if (balance_moved != 0) {
        for (light_b = 0; light_b < 4; light_b++) {
            if (balance_parts[light_b] != 0) {
                balance_parts[light_b]->SetPosition(balance_pos[light_b]);
            }
        }
    }
    return total;
}

/**
 * Begins river definitions.
 */
static int emapEDIT_RIVER(SPI_STACK *stack, int argc) {
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", emapRIVER_PARTS_NAME__FP9SPI_STACKi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", emapMASK_PARTS_NAME__FP9SPI_STACKi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", emapWATER_PARTS_NAME__FP9SPI_STACKi);
/**
 * Ends river definitions.
 */
static int emapEDIT_RIVER_END(SPI_STACK *stack, int argc) {
    return 1;
}

/**
 * Allocates the fixed parts table.
 */
static int emapFIX_EPARTS_START(SPI_STACK *stack, int argc) {
    int count = spiGetStackInt(stack);
    u32 size;
    int quadwords;
    ePlaceData *table;
    if (count <= 0) {
        return 0;
    }

    size = count * sizeof(ePlaceData);
    quadwords = (size & 0xF) ? (size >> 4) + 1 : size >> 4;
    table = new (emapStack->Alloc(quadwords + 2)) ePlaceData[count];
    emapInfo->SeteFixPartsTable(table, count);
    emapFixNum = count;
    emapFix = table;
    emapFixIdx = 0;
    return 1;
}

/**
 * Sets a fixed part placement.
 */
static int emapFIX_EPARTS(SPI_STACK *stack, int argc) {
    int index = emapFixIdx;
    ePlaceData *entry;
    if (index < 0 || index >= emapFixNum) {
        return 0;
    }
    if (emapFix == 0) {
        return 0;
    }
    entry = &emapFix[index];
    entry->id = spiGetStackInt(stack++);
    spiGetStackVector(entry->position, stack);
    entry->angle = spiGetStackInt(stack += 3);
    emapFixIdx++;
    return 1;
}

/**
 * Ends the fixed parts table.
 */
static int emapFIX_EPARTS_END(SPI_STACK *stack, int argc) {
    emapFixNum = 0;
    emapFixIdx = 0;
    emapFix = 0;
    return 1;
}

/**
 * Allocates the initial parts table.
 */
static int emapINIT_EPARTS_START(SPI_STACK *stack, int argc) {
    int count = spiGetStackInt(stack);
    u32 size;
    int quadwords;
    ePlaceData *table;
    if (count <= 0) {
        return 0;
    }

    size = count * sizeof(ePlaceData);
    quadwords = (size & 0xF) ? (size >> 4) + 1 : size >> 4;
    table = new (emapStack->Alloc(quadwords + 2)) ePlaceData[count];
    emapInfo->init_parts = table;
    emapInfo->init_parts_num = count;
    emapInit = table;
    emapInitNum = count;
    emapInitIdx = 0;
    return 1;
}

/**
 * Sets an initial part placement.
 */
static int emapINIT_EPARTS(SPI_STACK *stack, int argc) {
    int index = emapInitIdx;
    ePlaceData *entry;
    if (index < 0 || index >= emapInitNum) {
        return 0;
    }
    if (emapInit == 0) {
        return 0;
    }
    entry = &emapInit[index];
    entry->id = spiGetStackInt(stack++);
    spiGetStackVector(entry->position, stack);
    entry->angle = spiGetStackInt(stack += 3);
    emapInitIdx++;
    return 1;
}

/**
 * Ends the initial parts table.
 */
static int emapINIT_EPARTS_END(SPI_STACK *stack, int argc) {
    emapInitNum = 0;
    emapInitIdx = 0;
    emapInit = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", LoadEditInfo__8CEditMapFPciP9mgCMemory);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap", __ct__14CEditPartsInfoFv);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_830__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_988__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_1837__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", emap_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2257__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2278__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_449__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_450__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_451__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_452__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_474__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2072__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2073__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2074__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2075__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2076__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2077__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2078__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2079__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2080__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2081__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", at_2082__2__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", __vt__14CEditCollision__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap", __vt__8CEditMap__DATA);

// Small initialised data (.sdata)

// Small uninitialised data (.sbss)
INCLUDE_BSS(emapMap, 0x4);
INCLUDE_BSS(emapInfo, 0x4);
INCLUDE_BSS(emapStack, 0x4);
INCLUDE_BSS(emapIdx, 0x4);
INCLUDE_BSS(emapNowInfo, 0x4);
INCLUDE_BSS(emapRect, 0x4);
INCLUDE_BSS(emapRectNum, 0x4);
INCLUDE_BSS(emapRectIdx, 0x4);
INCLUDE_BSS(emapFixNum, 0x4);
INCLUDE_BSS(emapInitNum, 0x4);
INCLUDE_BSS(emapFixIdx, 0x4);
INCLUDE_BSS(emapInitIdx, 0x4);
INCLUDE_BSS(emapFix, 0x4);
INCLUDE_BSS(emapInit, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_426, 0x10);
