#include "common.h"
#include "editparts.hpp"

#include <libvu0.h>

#include "mdslist.hpp"
#include "mg_math.hpp"

// Code (.text)
void CEditPartsInfo::Initialize() {
    id = -999;
    attr = 0;
    edit_name = NULL;
    comment = NULL;
    parts_name = NULL;
    parts = NULL;
    place_anime = 0;
    bury_depth = 0.0f;
    col_area1.Initialize();
    col_floor.Initialize();
    col_wall.Initialize();
    col_area3.Initialize();
    wall_group_num = 0;
    cpoint[0] = 0;
    cpoint[1] = 0;
    weight = 0;
    geo_stone = -1;
    max_num = 0;
    material[0].num = 0;
    material[0].item_no = 0;
    material[1].num = 0;
    material[1].item_no = 0;
    material[2].num = 0;
    material[2].item_no = 0;
    material[3].num = 0;
    material[3].item_no = 0;
    paint_num = 0;
    paint_used = 0;
    parts_type = 0;
    map_no = -1;
    polyn[0] = 0;
    polyn[1] = 0;
    polyn[2] = 0;
    place_eps = 0.0f;
}

s32 CEditPartsInfo::GetPartsType(void) {
    if (attr & EDIT_PARTS_ATR_TYPE_ONE) {
        return 1;
    }
    if (attr & EDIT_PARTS_ATR_RIVER) {
        return EDIT_PARTS_TYPE_RIVER;
    }
    return parts_type;
}

void CEditPartsInfo::CreateBox() {
    mgVu0FBOX extent;

    *(u_long128 *)box.max = *(u_long128 *)extent.max;
    box.max[3] = 1.0f;
    *(u_long128 *)box.min = *(u_long128 *)extent.min;
    box.min[3] = 1.0f;
}

float CEditPartsInfo::GetPartsHeight() {
    return box.max[1] - box.min[1];
}

float CEditPartsInfo::GetPartsMaxWidth() {
    float width;
    float height;
    float depth;

    width = box.max[0] - box.min[0];
    height = box.max[1] - box.min[1];
    depth = box.max[2] - box.min[2];
    if (width > height) {
        if (width > depth) {
            return width;
        }
        return depth;
    }
    if (height > depth) {
        return height;
    }
    return depth;
}

EditPartsMaterial *CEditPartsInfo::GetMaterial(int no) {
    if (no < 0 || no >= EDIT_PARTS_MATERIAL_MAX) {
        return NULL;
    }
    return &material[no];
}

int CEditPartsInfo::GetDefColor(int no, float *out_rgba) {
    if (parts == NULL) {
        return 0;
    }
    return parts->GetDefColor(no, out_rgba);
}

int CEditHouse::LiveChara() {
    int npc;

    for (npc = 0; npc < EDIT_HOUSE_NPC_MAX; npc++) {
        if (npc_no[npc] > 0) {
            return 1;
        }
    }
    return 0;
}

void CEditParts::Initialize() {
    piece_list = NULL;
    anime_list = NULL;
    info = NULL;
    state = EDIT_PARTS_STATE_NONE;
    house = NULL;
    ground = NULL;
    max_material_num = 0;
    CMapParts::Initialize();
}

/**
 * Snaps a height to a whole unit with an allowance away from zero.
 */
static float StandardPos(float pos) {
    if (pos > 0.0f) {
        return (float)(int)(0.001f + pos);
    }
    return (float)(int)(pos - 0.001f);
}

void CEditParts::SetPosition(float *position) {
    sceVu0FVECTOR ground_position;
    sceVu0FVECTOR local_position;

    if (ground == NULL) {
        mgCObject::SetPosition(position);
        return;
    }
    ground->GetPosition(ground_position);
    *(u_long128 *)local_position = *(u_long128 *)position;
    local_position[1] -= StandardPos(ground_position[1]);
    mgCObject::SetPosition(local_position);
}

void CEditParts::SetPosition(float x, float y, float z) {
    sceVu0FVECTOR position = { 0.0f, 0.0f, 0.0f, 1.0f };

    position[0] = x;
    position[1] = y;
    position[2] = z;
    SetPosition(position);
}

void CEditParts::GetPosition(float *out_position) {
    sceVu0FVECTOR ground_position;

    GetLocalPos(out_position);
    if (ground != NULL) {
        ground->GetPosition(ground_position);
        out_position[1] += StandardPos(ground_position[1]);
    }
}

void CEditParts::GetLocalPos(float *out_position) {
    mgCObject::GetPosition(out_position);
}

void CEditParts::UpDatePosition() {
    sceVu0FVECTOR world_position;
    sceVu0FVECTOR ground_position;

    if (changed || ground != NULL) {
        GetLocalPos(world_position);
        if (ground != NULL) {
            ground->GetPosition(ground_position);
            world_position[1] += ground_position[1];
        }
        frame.SetPosition(world_position);
        frame.SetRotation(rotation);
        frame.SetScale(scale);
        changed = 0;
    }
}

int CEditParts::GetInfoID() {
    if (info != NULL) {
        return info->id;
    }
    return -1;
}

s32 CEditParts::GetLiveNPC(void) {
    CEditHouse *part_house = house;
    if (part_house != NULL) {
        return part_house->npc_no[0];
    }
    return -1;
}

int CEditParts::IsWallParts() {
    if (info == NULL) {
        return 0;
    }
    if (info->col_wall.poly_count <= 0) {
        return 0;
    }
    return 1;
}

int CEditParts::IsFence() {
    if (info == NULL) {
        return 0;
    }
    return (info->attr & EDIT_PARTS_ATR_FENCE) == EDIT_PARTS_ATR_FENCE;
}

int CEditParts::IsBurn() {
    if (info == NULL) {
        return 0;
    }
    return (info->attr & EDIT_PARTS_ATR_BURN) != 0;
}

int CEditParts::GetFenceSide(float *out_side0, float *out_side1) {
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR side0;
    sceVu0FVECTOR side1;

    if (!bound_valid) {
        return 0;
    }
    if (info == NULL) {
        return 0;
    }
    GetLWMatrix(matrix);
    *(u_long128 *)side0 = *(u_long128 *)info->area3_box.max;
    *(u_long128 *)side1 = *(u_long128 *)info->area3_box.min;
    side1[3] = 1.0f;
    side0[3] = 1.0f;
    sceVu0ApplyMatrix(out_side0, matrix, side0);
    sceVu0ApplyMatrix(out_side1, matrix, side1);
    return 1;
}

#ifdef NONMATCHING
int CEditParts::GetWallPlane(int wall_no, WallInfo *out_info) {
    sceVu0FVECTOR sum;
    sceVu0FVECTOR max;
    sceVu0FVECTOR min;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR poly_min;
    CCPoly       *poly;
    int           vertex_count;
    int           found;
    int           poly_count;
    int           i;

    if (!IsWallParts()) {
        return 0;
    }
    vertex_count = 0;
    poly_count = info->col_wall.poly_count;
    poly = info->col_wall.poly;
    found = 0;
    mgZeroVector(sum);
    for (i = 0; i < poly_count; i++, poly++) {
        if (poly->ignore_mask == wall_no) {
            if (!found) {
                sceVu0Normalize(out_info->plane, poly->normal);
                out_info->plane[3] = -sceVu0InnerProduct(out_info->plane, poly->vertex[0]);
                mgVectorMaxMin(max, min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);
                found = 1;
            } else {
                mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);
                mgVectorMaxMin(max, min, max, min, poly_max, poly_min);
            }
            mgAddVector(sum, poly->vertex[0]);
            mgAddVector(sum, poly->vertex[1]);
            mgAddVector(sum, poly->vertex[2]);
            vertex_count += 3;
        }
    }
    sceVu0ScaleVector(out_info->center, sum, 1.0f / (float)vertex_count);
    out_info->box.max[1] = max[1] - out_info->center[1];
    out_info->box.min[1] = min[1] - out_info->center[1];
    out_info->box.max[0] = mgDistVectorXZ(max, out_info->center);
    out_info->box.min[0] = -mgDistVectorXZ(min, out_info->center);
    out_info->box.max[2] = 0.0f;
    out_info->box.min[2] = 0.0f;
    out_info->box.max[3] = 1.0f;
    out_info->box.min[3] = 1.0f;
    out_info->center[3] = 1.0f;
    return found;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editparts", GetWallPlane__10CEditPartsFiPQ210CEditParts8WallInfo);
#endif

int CEditParts::GetWallGroupNum() {
    if (info != NULL) {
        return info->wall_group_num;
    }
    return 0;
}

s32 CEditParts::GetPartsType(void) {
    CEditPartsInfo *part_info = info;
    if (part_info != NULL) {
        return part_info->GetPartsType();
    }
    return -1;
}

void CEditParts::Copy(CMapParts &dest, mgCMemory *memory) {
    CMapParts::Copy(dest, memory);
}

int CEditParts::CheckTerritory(CEditParts *other) {
    sceVu0FVECTOR center;
    sceVu0FVECTOR other_center;
    sceVu0FMATRIX matrix;
    sceVu0FMATRIX other_matrix;
    float         radius;
    float         height_limit;
    float         height_difference;

    if (info == NULL || other == NULL || other->info == NULL) {
        return 0;
    }
    if (other->info->attr & 0xAC2) {
        return 0;
    }
    GetLWMatrix(matrix);
    other->GetLWMatrix(other_matrix);
    sceVu0ApplyMatrix(center, matrix, info->territory_center);
    sceVu0ApplyMatrix(other_center, other_matrix, other->info->territory_center);
    radius = info->territory_radius + other->info->territory_radius;
    if (mgDistVectorXZ(center, other_center) > radius) {
        return 0;
    }
    height_limit = info->territory_height + other->info->territory_height;
    height_difference = other_center[1] - center[1];
    if (height_difference < 0.0f) {
        height_difference = -height_difference;
    }
    if (height_difference > height_limit) {
        return 0;
    }
    return 1;
}

void CEditParts::CheckColorUpdate() {
    CList<CMapPiece> *piece;
    int              material_count;

    piece = piece_list;
    max_material_num = 0;
    for (; piece != NULL; piece = piece->next) {
        material_count = piece->data.material_num;
        if (material_count > 0) {
            max_material_num = material_count < max_material_num ? max_material_num : material_count;
        }
    }
}

int EditPartsCmpColor(float *color0, float *color1) {
    int same;

    if (mgDistVector(color0, color1) < 0.02f) {
        return 1;
    }
    return 0;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editparts", at_418__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editparts", __vt__10CEditParts__DATA);
