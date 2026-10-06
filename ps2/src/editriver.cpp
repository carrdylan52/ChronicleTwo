#include "common.h"
#include "editriver.hpp"

#include <cstring>

#include "editmap.hpp"
#include "editparts.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"

// Code (.text)
int CEditMap::PlaceRiver(float *pos) {
    for (int i = 0; i < grid_max; i++) {
        CEditGrid *grid = this->grid[i];
        if (grid != NULL && grid->SetRiver(pos[0], pos[2])) {
            return 1;
        }
    }
    return 0;
}

int CEditMap::RemoveRiver(float *pos) {
    for (int i = 0; i < grid_max; i++) {
        CEditGrid *grid = this->grid[i];
        if (grid != NULL && grid->ResetRiver(pos[0], pos[2])) {
            return 1;
        }
    }
    return 0;
}

void CEditMap::CreateGrid(float *max, float *min, mgCMemory *stack, float *ofs) {
    sceVu0FMATRIX matrix;
    float origin_x = min[0];
    float origin_z = min[2];
    origin_x += ofs[0];
    origin_z += ofs[2];
    int num_x = (int)((max[0] - origin_x) / 160.0f);
    int num_z = (int)((max[2] - origin_z) / 160.0f);
    for (int i = 0; i < grid_max; i++) {
        if (grid[i] == NULL) {
            CEditGrid *grid;
            if ((grid = new (stack->Alloc(sizeof(CEditGrid) / sizeof(u_long128) + 2)) CEditGrid) != NULL) {
                grid->Initialize();
            }
            this->grid[i] = grid;
            this->grid[i]->Create(num_x, num_z, stack);
            this->grid[i]->step_x = 160.0f;
            this->grid[i]->step_z = 160.0f;
            this->grid[i]->origin[0] = origin_x;
            this->grid[i]->origin[1] = min[1];
            this->grid[i]->origin[2] = origin_z;
            this->grid[i]->Clear();
            for (int x = 0; x < num_x; x++) {
                for (int y = 0; y < num_z; y++) {
                }
            }
            for (int turn = 0; turn < EDIT_GRID_ROT_MAX; turn++) {
                mgUnitMatrix(matrix);
                if (turn == 3) {
                    matrix[2][2] = 0.0f;
                    matrix[0][2] = -1.0f;
                    matrix[0][0] = 0.0f;
                    matrix[2][0] = 1.0f;
                }
                if (turn == 2) {
                    matrix[2][2] = -1.0f;
                    matrix[0][0] = -1.0f;
                }
                if (turn == 1) {
                    matrix[2][2] = 0.0f;
                    matrix[0][2] = 1.0f;
                    matrix[0][0] = 0.0f;
                    matrix[2][0] = -1.0f;
                }
                sceVu0CopyMatrix(this->grid[i]->rot[turn], matrix);
            }
            return;
        }
    }
}

int CEditMap::GetRiverNum(float *sphere) {
    sceVu0FVECTOR cell_pos;
    int count = 0;
    for (int g = 0; g < grid_max; g++) {
        CEditGrid *grid = this->grid[g];
        if (grid != NULL) {
            for (int x = 0; x < grid->num_x; x++) {
                for (int y = 0; y < grid->num_z; y++) {
                    if (grid->River(x, y)) {
                        grid->GetRiverPos(x, y, cell_pos);
                        if (sphere[3] < 0.0f || mgDistVectorXZ(cell_pos, sphere) <
                                                      sphere[3] + 1.4f * grid->step_x) {
                            count++;
                        }
                    }
                }
            }
        }
    }
    return count;
}

int CEditMap::IsRiverGrid(float *pos) {
    int cell[2];
    for (int i = 0; i < grid_max; i++) {
        CEditGrid *grid = this->grid[i];
        if (grid != NULL && grid->GetLPos(cell, pos[0], pos[2]) && grid->River(cell[0], cell[1])) {
            return 1;
        }
    }
    return 0;
}

int CEditMap::GetRiverNum(int no, float range) {
    sceVu0FVECTOR sphere;
    CEditParts *edit_parts = GetePlaceParts(no);
    if (edit_parts == NULL) {
        return 0;
    }
    if (!CheckNormalPlaceParts(edit_parts)) {
        return 0;
    }
    edit_parts->GetPosition(sphere);
    sphere[3] = range;
    return GetRiverNum(sphere);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editriver", DrawRiverMask__8CEditMapFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editriver", DrawRiver__8CEditMapFv);
void CEditGrid::Create(int num_x, int num_z, mgCMemory *stack) {
    int count = num_x * num_z;
    u32 blocks;

    if (((u32)count * sizeof(CGridData)) & 0xF) {
        blocks = (((u32)count * sizeof(CGridData)) >> 4) + 1;
    } else {
        blocks = ((u32)count * sizeof(CGridData)) >> 4;
    }
    u_long128 *block = stack->Alloc(blocks + 2);
    data = new (block) CGridData[count];
    this->num_x = num_x;
    this->num_z = num_z;
}

CGridData::CGridData() {
    memset(this, 0, sizeof(CGridData));
}

void CEditGrid::Clear() {
    memset(data, 0, num_x * num_z * sizeof(CGridData));
}

void CEditGrid::Initialize() {
    num_z = 0;
    num_x = 0;
    data = NULL;
    step_z = 0.0f;
    step_x = 0.0f;
    mgZeroVector(origin);
}

int CEditGrid::Check(int x, int z) {
    if (x < 0 || x >= num_x) {
        return 0;
    }
    if (z < 0 || z >= num_z) {
        return 0;
    }
    return 1;
}

CGridData *CEditGrid::Get(int x, int z) {
    if (!Check(x, z)) {
        return NULL;
    }
    return GetFast(x, z);
}

CGridData *CEditGrid::GetFast(int x, int z) {
    return &data[x + z * num_x];
}

int CEditGrid::GetLPos(int *lpos, float x, float z) {
    float cell_x = (x - origin[0]) / step_x;
    float cell_y = (z - origin[2]) / step_z;
    lpos[0] = (int)cell_x;
    lpos[1] = (int)cell_y;
    if (cell_x < 0.0f || cell_y < 0.0f) {
        return 0;
    }
    return Check(lpos[0], lpos[1]) != 0;
}

void CEditGrid::GetWPos(float *pos, int x, int z) {
    pos[0] = origin[0] + (float)x * step_x;
    pos[2] = origin[2] + (float)z * step_z;
    pos[1] = 0.0f;
    pos[3] = 1.0f;
}

s32 CEditGrid::SetRiver(float x, float z) {
    s32 grid_pos[2];
    s32 result;
    if (GetLPos(grid_pos, x, z)) {
        result = SetRiver(grid_pos[0], grid_pos[1]);
    } else {
        result = 0;
    }
    return result;
}

s32 CEditGrid::ResetRiver(float x, float z) {
    s32 grid_pos[2];
    s32 result;
    if (GetLPos(grid_pos, x, z)) {
        result = ResetRiver(grid_pos[0], grid_pos[1]);
    } else {
        result = 0;
    }
    return result;
}

s32 CEditGrid::SetRiver(s32 x, s32 z) {
    CGridData *cell;

    cell = Get(x, z);
    if (cell == NULL) {
        return 0;
    }
    cell->river = 1;
    UpdateRiver(x, z);
    UpdateRiver(x - 1, z);
    UpdateRiver(x + 1, z);
    UpdateRiver(x, z + 1);
    UpdateRiver(x, z - 1);
    UpdateRiver(x - 1, z - 1);
    UpdateRiver(x + 1, z - 1);
    UpdateRiver(x + 1, z + 1);
    UpdateRiver(x - 1, z + 1);
    return 1;
}

s32 CEditGrid::ResetRiver(s32 x, s32 z) {
    CGridData *cell;

    cell = Get(x, z);
    if (cell == NULL) {
        return 0;
    }
    if (cell->river == 0) {
        return 0;
    }
    cell->river = 0;
    UpdateRiver(x, z);
    UpdateRiver(x - 1, z);
    UpdateRiver(x + 1, z);
    UpdateRiver(x, z + 1);
    UpdateRiver(x, z - 1);
    UpdateRiver(x - 1, z - 1);
    UpdateRiver(x + 1, z - 1);
    UpdateRiver(x + 1, z + 1);
    UpdateRiver(x - 1, z + 1);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editriver", UpdateRiver__9CEditGridFii);
s32 CEditGrid::River(s32 x, s32 z) {
    CGridData *cell;

    cell = Get(x, z);
    if (cell != NULL) {
        return cell->river;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editriver", GetRiverPos__9CEditGridFiiPA4_f);
void CEditGrid::GetRiverPos(int x, int z, float *pos) {
    float half_width = step_x / 2.0f;
    float half_height = step_z / 2.0f;
    GetWPos(pos, x, z);
    pos[0] += half_width;
    pos[2] += half_height;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editriver", GetRiverPoly__9CEditGridFP6CCPolyRC9mgVu0FBOXif);
void CEditGrid::GetGridBox(mgVu0FBOX *box, float *pos) {
    int cell[2];
    float corner[4];
    GetLPos(cell, pos[0], pos[2]);
    GetWPos(corner, cell[0], cell[1]);
    *(u_long128 *)box->min = *(u_long128 *)corner;
    box->min[3] = 1.0f;
    *(u_long128 *)box->max = *(u_long128 *)corner;
    box->min[3] = 1.0f;
    box->max[0] += step_x;
    box->max[2] += step_z;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_504__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_505__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_506__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_507__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_590__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_591__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_592__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_593__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_594__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editriver", at_799__3__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_733__2, 0x10);
INCLUDE_BSS(at_734, 0x10);
