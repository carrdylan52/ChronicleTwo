#include "common.h"
#include "editmap2.hpp"

#include <cstdio>
#include <cstring>

#include "editdata.hpp"
#include "editparts.hpp"
#include "editriver.hpp"
#include "funcpoint.hpp"
#include "mainloop.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "mg_math.hpp"
#include "snd_mngr.hpp"

static const float kFenceChainDistance = 5.0f;
static const int kBalanceLimit = 4;
static const int kNpcLiveLength = 7;
static const int kChildIdMax = 0x200;

// Code (.text)
/**
 * Computes the plane normal from the XZ projections of three vertices.
 */
static void PlaneNormalXZ(float *normal, float *p0, float *p1, float *p2) {
    asm {
        lqc2 vf15, 0(p0)
        vsub.xyzw vf10, vf10, vf10
        lqc2 vf16, 0(p1)
        vsub.xyzw vf11, vf11, vf11
        lqc2 vf17, 0(p2)
        vsub.xz vf10, vf16, vf15
        vsub.xz vf11, vf17, vf15
        vopmula.xyz ACC, vf10, vf11
        vopmsub.xyz vf12, vf11, vf10
        sqc2 vf12, 0(normal)
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", GetEditPartsAlt__8CEditMapFP14CEditPartsInfoPffPP10CEditPartsi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", CheckEditParts__8CEditMapFP14CEditPartsInfoPffP13EP_PLACE_INFOPP10CEditPartsi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", CheckEditPartsOnRiver__8CEditMapFP14CEditPartsInfoPff);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", CheckRiverParts__8CEditMapFPf);
s32 CEditMap::CheckNormalPlaceParts(s32 place_no) {
    CEditParts *parts = GetePlaceParts(place_no);
    return CheckNormalPlaceParts(parts);
}

int CEditMap::CheckNormalPlaceParts(CEditParts *parts) {
    if (parts == NULL) {
        return 0;
    }
    if ((parts->name[0] == 0) != 0 || parts->state != EDIT_PARTS_STATE_PLACED) {
        return 0;
    }
    return 1;
}

int CEditMap::CheckLiveNPC(int npc_no, int id) {
    CEditParts *part;
    int count;
    int i;

    i = 0;
    part = edit_parts;
    count = 0;
    while (i < edit_parts_max) {
        if (CheckNormalPlaceParts(part) != 0 && part->house != NULL) {
            if (id < 0 && npc_no < 0) {
                if (part->house->npc_no[0] > 0) {
                    count++;
                }
            } else if (id <= 0 || part->info->id == id) {
                if (npc_no < 0) {
                    if (part->house->npc_no[0] > 0) {
                        return 1;
                    }
                } else if (part->house->npc_no[0] == npc_no) {
                    return 1;
                }
            }
        }
        i++;
        part++;
    }
    return count;
}

int CEditMap::GetePlacePartsAtInfoID(int id, int *out_no, int max) {
    int found = 0;
    int limit = max;
    CEditPartsInfo *info;
    CEditParts *part;
    int i;

    info = GetePartsInfoAtID(id);
    if (info == NULL) {
        return 0;
    }
    if (info->GetPartsType() == EDIT_PARTS_TYPE_RIVER) {
        sceVu0FVECTOR position = { 0.0f, 0.0f, 0.0f, -1.0f };
        int river_count = GetRiverNum(position);
        limit = river_count < limit ? river_count : limit;
        for (i = 0; i < limit; i++) {
            out_no[i] = 0;
        }
        return river_count;
    }
    part = edit_parts;
    for (i = 0; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) != 0 && part->info->id == id) {
            if (out_no != NULL) {
                out_no[found] = i;
                found++;
                if (found >= limit) {
                    break;
                }
            } else {
                found++;
            }
        }
    }
    return found;
}

int CEditMap::GetTerritoryParts(int no, int *out_no, int max) {
    CEditParts *place;
    int found;
    CEditParts *part;
    int i;
    if (out_no == NULL || max <= 0) {
        return 0;
    }
    place = GetePlaceParts(no);
    if (place == NULL) {
        return 0;
    }
    if (CheckNormalPlaceParts(no) == 0) {
        return 0;
    }
    part = edit_parts;
    found = 0;
    for (i = 0; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) != 0 && place->CheckTerritory(part) != 0) {
            found++;
            *out_no++ = i;
            if (found >= max) {
                break;
            }
        }
    }
    return found;
}

int CEditMap::GetChildParts(int no, int *out_no, int max) {
    int found;
    EditPlaceLog *child;
    int i;
    s16 id;
    if (GetePlaceParts(no) == NULL) {
        return 0;
    }
    child = place_log;
    found = 0;
    for (i = 0; i < place_log_max; i++, child++) {
        id = child->parts_no;

        if ((u8)(id < 0) != 0 || child->base_no != no) {
            continue;
        }
        out_no[found] = id;
        found++;
        if (found >= max) {
            break;
        }
    }
    return found;
}

s32 CEditMap::RePaintNum(s32 count) {
    return count / 2;
}

int CEditMap::PaintFence(int no, float *color, int num) {
    CEditParts *fence;
    CEditParts *candidates[0x800];
    CEditParts *part;
    int i;

    fence = GetePlaceParts(no);
    if (fence == NULL || fence->info == NULL) {
        return 0;
    } else {
        if (fence->IsFence() == 0) {
            return 0;
        }

        if (fence->info->paint_num <= 0) {
            return 0;
        }
        fence_list = candidates;
        *(u_long128 *)fence_color = *(u_long128 *)color;
        paint_num = num;
        part = edit_parts;
        fence_num = 0;
        for (i = 0; i < edit_parts_max; i++, part++) {
            if (CheckNormalPlaceParts(part) != 0 && part->IsFence() != 0 &&
                part->info->paint_num > 0 && part != fence) {
                candidates[fence_num] = part;
                fence_num++;
            }
        }
        candidates[fence_num] = NULL;
        return PaintFence(fence);
    }
}

/**
 * Tests whether two fences have touching endpoints.
 */
static int CheckFenceChain(CEditParts *a, CEditParts *b) {
    sceVu0FVECTOR sphere_a;
    sceVu0FVECTOR sphere_b;
    sceVu0FVECTOR a_start;
    sceVu0FVECTOR b_start;
    sceVu0FVECTOR a_end;
    sceVu0FVECTOR b_end;

    if (a == NULL || b == NULL) {
        return 0;
    }
    if (a->GetBoundSphere(sphere_a) == 0) {
        return 0;
    }
    if (b->GetBoundSphere(sphere_b) == 0) {
        return 0;
    }
    if (!(mgDistVector(sphere_a, sphere_b) <= sphere_a[3] + sphere_b[3])) {
        return 0;
    }
    if (a->GetFenceSide(a_start, a_end) == 0) {
        return 0;
    }
    if (b->GetFenceSide(b_start, b_end) == 0) {
        return 0;
    }
    if (mgDistVector(a_start, b_end) < kFenceChainDistance) {
        return 1;
    }
    if (mgDistVector(b_start, a_end) < kFenceChainDistance) {
        return 1;
    }
    if (mgDistVector(a_start, b_start) < kFenceChainDistance) {
        return 1;
    }
    if (mgDistVector(a_end, b_end) < kFenceChainDistance) {
        return 1;
    }
    return 0;
}

int CEditMap::PaintFence(CEditParts *parts) {
    int painted = 0;
    int i;

    parts->SetColor(0, fence_color);
    parts->UpdateColor();
    painted++;
    paint_num--;
    for (i = 0; i < fence_num; i++) {
        fence_now = fence_list[i];
        if (fence_now != NULL && CheckFenceChain(parts, fence_now) != 0) {
            fence_list[i] = NULL;
            painted += PaintFence(fence_now);
        }
    }
    return painted;
}

void CEditMap::UpdateHouse() {
    int index;
    CList<CMapPiece> *node;
    int live_length;
    CMapPiece *model;
    CEditParts *part;
    char *node_name;
    int visible;
    float fade;
    int child_ids[kChildIdMax];
    int child_num;
    int i;
    CEditParts *child;
    part = edit_parts;
    for (index = 0; index < edit_parts_max; index++, part++) {
        if (CheckNormalPlaceParts(part) != 0 && part->house != NULL) {
            fade = -1.0f;
            visible = 0;
            if (part->house->LiveChara() == 0) {
                visible = 1;
                fade = 12.0f;
            }
            part->fixed_time = fade;
            for (node = part->piece_list; node != NULL; node = node->next) {
                node_name = node->data.name;
                model = &node->data;
                char live_name[10] = "npclive";
                char suffix[10];
                live_length = kNpcLiveLength;
                if (LanguageCode > 0) {
                    live_length += sprintf(suffix, "%d", LanguageCode);
                    strcat(live_name, suffix);
                }
                if (node_name != NULL && strncmp(node_name, "npclive", kNpcLiveLength) == 0) {
                    model->Show(0);
                    if (strncmp(node_name, live_name, live_length) == 0) {
                        model->Show(visible);
                    }
                }
            }
            child_num = GetChildParts(index, child_ids, kChildIdMax);
            for (i = 0; i < child_num; i++) {
                child = GetePlaceParts(child_ids[i]);
                if (child != NULL) {
                    child->fixed_time = fade;
                }
            }
        }
    }
}

void CEditMap::GroundBalance(int keep) {
    if (area_no != 1) {
        return;
    }
    int weights[4] = {0, 0, 0, 0};
    balance_parts[0] = GetPlaceParts("p09_g0201");
    balance_parts[1] = GetPlaceParts("p09_g0201-1");
    balance_parts[2] = GetPlaceParts("p08_g0201");
    balance_parts[3] = GetPlaceParts("p08_g0201-1");
    if (balance_parts[0] == NULL || balance_parts[1] == NULL ||
        balance_parts[2] == NULL || balance_parts[3] == NULL) {
        return;
    }
    int ground_index;
    CEditParts *part = edit_parts;
    for (int index = 0; index < edit_parts_max; ++index, ++part) {
        if (CheckNormalPlaceParts(part)) {
            CMapParts *ground = part->ground;
            if (ground == NULL) {
                continue;
            }
            for (ground_index = 0; ground_index < 4; ++ground_index) {
                if (ground == balance_parts[ground_index]) {
                    if (part->info != NULL) {
                        weights[ground_index] += part->info->weight;
                    }
                    break;
                }
            }
        }
    }
    int first_difference = weights[1] - weights[0];
    if (first_difference < 4 && first_difference > 0) {
        first_difference = 0;
    }
    if (first_difference > -4 && first_difference < 0) {
        first_difference = 0;
    }
    int second_difference = weights[3] - weights[2];
    if (second_difference < 4 && second_difference > 0) {
        second_difference = 0;
    }
    if (second_difference > -4 && second_difference < 0) {
        second_difference = 0;
    }
    if (first_difference > 30) {
        first_difference = 30;
    }
    if (first_difference < -30) {
        first_difference = -30;
    }
    if (second_difference > 30) {
        second_difference = 30;
    }
    if (second_difference < -30) {
        second_difference = -30;
    }
    if (balance_moved == 0) {
        for (int index = 0; index < 4; ++index) {
            balance_parts[index]->GetPosition(balance_base_pos[index]);
        }
        balance_moved = 1;
    }
    balance_parts[0]->GetPosition(balance_pos[0]);
    balance_pos[0][1] = 4.0f * first_difference;
    balance_parts[1]->GetPosition(balance_pos[1]);
    balance_pos[1][1] = 4.0f * -first_difference;
    balance_parts[2]->GetPosition(balance_pos[2]);
    balance_pos[2][1] = 4.0f * second_difference;
    balance_parts[3]->GetPosition(balance_pos[3]);
    balance_pos[3][1] = 4.0f * -second_difference;
    if (keep == 0) {
        balance_moved = 0;
    }
    for (int index = 0; index < 4; ++index) {
        balance_parts[index]->SetPosition(balance_pos[index]);
    }
    balance_weight[0] = weights[0];
    balance_weight[1] = weights[1];
    balance_weight[2] = weights[2];
    balance_weight[3] = weights[3];
}

int CEditMap::BalanceCheck() {
    float side_diff = (float)(balance_weight[0] - balance_weight[1]);
    if (side_diff < 0.0f) {
        side_diff = -side_diff;
    }
    int side = (int)side_diff;
    float depth_diff = (float)(balance_weight[2] - balance_weight[3]);
    if (depth_diff < 0.0f) {
        depth_diff = -depth_diff;
    }
    int depth = (int)depth_diff;
    return side < kBalanceLimit && depth < kBalanceLimit;
}

CFuncPoint *CEditMap::InScreenFunc(InScreenFuncInfo *info) {
    CFuncPoint *result;
    CEditParts *part;
    float best_near;
    float best_distance;
    int i;

    result = CMap::InScreenFunc(info);
    best_near = info->unk_04;
    part = edit_parts;
    best_distance = info->dist;
    for (i = 0; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) && part->CheckDraw()) {
            CFuncPoint *hit = part->InScreenFunc(info);
            if (hit != NULL && (result == NULL || info->dist < best_distance)) {
                result = hit;
                best_near = info->unk_04;
                best_distance = info->dist;
            }
        }
    }

    info->unk_04 = best_near;
    info->unk_04 = best_distance;
    return result;
}

void CEditMap::DrawScreenFunc(mgCFrame *marker) {
    CMap::DrawScreenFunc(marker);
    CEditParts *part = edit_parts;
    for (int i = 0; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) && part->CheckDraw()) {
            part->DrawScreenFunc(marker);
        }
    }
}

#ifdef NONMATCHING
int CEditMap::GetSeSrcVolPan(int *se_no, float *vol, float *pan, int max) {
    CFuncPointCheck check;
    float matrix[4][4];
    int count;
    int i;
    int x;
    int y;
    CEditParts *part;
    int added;

    check.time = 0.0f;
    CreateFuncCheck(&check);
    count = 0;
    mgUnitMatrix(matrix);
    added = CMap::GetSeSrcVolPan(se_no, vol, pan, max);
    max -= added;
    count += added;
    se_no += added;
    pan += added;
    i = 0;
    part = edit_parts;
    vol += added;
    for (; i < edit_parts_max; i++, part++) {
        if (CheckNormalPlaceParts(part) && (part->func_point_mngr.flag & FUNC_POINT_MNGR_SOUND)) {
            part->GetLWMatrix(matrix);
            if (max <= 0) {
                return count;
            }
            added = ::GetSeSrcVolPan(matrix, &part->func_point_mngr, &check, se_no, vol, pan, max);
            max -= added;
            count += added;
            se_no += added;
            vol += added;
            pan += added;
        }
    }
    if (max <= 0) {
        return count;
    }
    float near_dist = 10.0f;
    float far_dist = 2000.0f;
    float max_vol = 0.0f;
    int river_count = 0;
    float pan_sum = 0.0f;
    CEditGrid *grid;
    CEditGrid **next_grid;
    int g;
    for (g = 0, next_grid = this->grid; g < grid_max; next_grid++, g++) {

        grid = *next_grid;
        if (grid != NULL) {
            for (x = 0; x < grid->num_x; x++) {
                for (y = 0; y < grid->num_z; y++) {
                    if (grid->River(x, y)) {
                        float pos[4];
                        float river_vol;
                        float river_pan;
                        grid->GetWPos(pos, x, y);
                        sndGetVolPan(&river_vol, &river_pan, pos, near_dist, far_dist);
                        if (!(river_vol <= 0.0f)) {
                            river_count++;
                            if (max_vol < river_vol) {
                                max_vol = river_vol;
                            }
                            pan_sum += river_pan * river_vol;
                        }
                    }
                }
            }
        }
    }
    if (river_count > 0) {
        count++;
        *se_no = 6;
        *vol = max_vol;
        *pan = pan_sum / (float)river_count;
    }
    return count;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", GetSeSrcVolPan__8CEditMapFPiPfPfi);
#endif

// Initialised data (.data)

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap2", at_1127__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap2", at_1128__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap2", at_1129__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editmap2", at_1130__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(cnt_482, 0x4);
INCLUDE_BSS(init_483, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1050__2, 0x10);
