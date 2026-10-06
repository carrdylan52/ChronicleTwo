#include "common.h"
#include "editanalyze.hpp"
#include "vlgr_info.hpp"
#include "savedata.hpp"
#include "mainloop.hpp"
#include "mg_math.hpp"
#include "editdata.hpp"
#include "editmap.hpp"
#include "editmenu.hpp"

static int CheckSaku(CEditMap *map, int parts_no);
static int GetTreeNum(CEditMap *map);
static int CheckInfoID(CEditMap *map, int parts_no, int id);
static void AnalyzeSharlot(CEditData *data, CEditMap *map);
static void AnalyzeStera(CEditData *data, CEditMap *map);
static void AnalyzeBenietio(CEditData *data, CEditMap *map);
static int GetColorType(CEditParts *parts, int color_no);
static void AnalyzeHeim(CEditData *data, CEditMap *map);
static void AnalyzeMoonFlower(CEditData *data, CEditMap *map);

static const int info_tree_a = 0x28;
static const int info_tree_b = 0x29;
static const int info_tree_c = 0x2A;
static const int info_stone_wall = 0x2B;
static const int info_fence = 0x2F;
static const int parts_list_max = 0x200;

// Code (.text)
void AnalyzeEditMap(int map_no, CEditMap *edit_map) {
    CEditData *data;

    if (edit_map != NULL) {
        data = GetSaveData()->GetEditData(map_no);
        if (data != NULL) {
            if (map_no == 0) {
                AnalyzeSharlot(data, edit_map);
            }
            if (map_no == 1) {
                AnalyzeStera(data, edit_map);
            }
            if (map_no == 2) {
                AnalyzeBenietio(data, edit_map);
            }
            if (map_no == 3) {
                AnalyzeHeim(data, edit_map);
            }
            if (map_no == 4) {
                AnalyzeMoonFlower(data, edit_map);
            }
        }
    }
}

int CountPartsType(int parts_type, CEditMap *edit_map, int *list, int num) {
    CEditParts *parts;
    int matches;
    int i;

    matches = 0;
    for (i = 0; i < num; ++i) {
        parts = edit_map->GetePlaceParts(list[i]);
        if ((parts != NULL) && (parts_type == parts->GetPartsType())) {
            matches += 1;
        }
    }
    return matches;
}

int CountPartsInfoID(int info_id, CEditMap *edit_map, int *list, int num) {
    CEditParts *parts;
    int matches;
    int i;

    matches = 0;
    for (i = 0; i < num; ++i) {
        parts = edit_map->GetePlaceParts(list[i]);
        if ((parts != NULL) && (info_id == parts->GetInfoID())) {
            matches += 1;
        }
    }
    return matches;
}

/**
 * Checks whether a placed part is a fence.
 *
 * @mangled CheckSaku__FP8CEditMapi
 * @address 0x31c020
 * @size 0x3c
 */
static int CheckSaku(CEditMap *map, int parts_no) {
    CEditParts *parts;

    parts = map->GetePlaceParts(parts_no);
    if (parts == NULL) {
        return 0;
    }
    return parts->GetPartsType() == 8;
}

/**
 * Counts the placed tree parts in the town.
 *
 * @mangled GetTreeNum__FP8CEditMap
 * @address 0x31c060
 * @size 0xd0
 */
static int GetTreeNum(CEditMap *map) {
    int n;

    n = map->GetePlacePartsAtInfoID(7, NULL, 0);
    n += map->GetePlacePartsAtInfoID(0x13, NULL, 0);
    n += map->GetePlacePartsAtInfoID(0x1D, NULL, 0);
    n += map->GetePlacePartsAtInfoID(0x23, NULL, 0);
    n += map->GetePlacePartsAtInfoID(info_tree_a, NULL, 0);
    n += map->GetePlacePartsAtInfoID(info_tree_b, NULL, 0);
    n += map->GetePlacePartsAtInfoID(info_tree_c, NULL, 0);
    return n;
}

int GetHouseParts(CEditMap *edit_map, int *list, int max) {
    int ids[4] = {1, 9, 22, 31};
    int total = 0;
    for (int i = 0; i < 4; i++) {
        int found = edit_map->GetePlacePartsAtInfoID(ids[i], list, max);
        list += found;
        total += found;
        max -= found;
        if (max <= 0) {
            break;
        }
    }
    return total;
}

/**
 * Checks whether a placed part has the requested definition ID.
 *
 * @mangled CheckInfoID__FP8CEditMapii
 * @address 0x31c1e0
 * @size 0x44
 */
static int CheckInfoID(CEditMap *map, int parts_no, int id) {
    CEditParts *parts;

    parts = map->GetePlaceParts(parts_no);
    if (parts == NULL) {
        return 0;
    }
    return id == parts->GetInfoID();
}

CEditParts *GetPartsPos(CEditMap *map, int parts_no, float *position) {
    if (map == NULL) {
        return NULL;
    }
    CEditParts *parts = map->GetePlaceParts(parts_no);
    if (parts == NULL) {
        return NULL;
    }
    parts->GetPosition(position);
    return parts;
}

/**
 * Stores the Sharlot town conditions in its saved analysis.
 *
 * @mangled AnalyzeSharlot__FP9CEditDataP8CEditMap
 * @address 0x31c2a0
 * @size 0x54c
 */
static void AnalyzeSharlot(CEditData *data, CEditMap *map) {
    int condition[EDIT_ANALYZE_CONDITION_MAX];
    int target[EDIT_ANALYZE_CONDITION_MAX];
    int parts_nos[parts_list_max];
    sceVu0FVECTOR river_pos;
    float tree_pos[3][4];
    float to_second[4];
    float to_third[4];
    float closest[4];
    int tree_a;
    int tree_b;
    int tree_c;
    int enough_trees;
    int stone_wall;
    int fence;

    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = 0;
        target[i] = -1;
    }
    static const float initial_river_pos[4] = {0.0f, 0.0f, 0.0f, -1.0f};
    *(u_long128 *)river_pos = *(const u_long128 *)initial_river_pos;
    condition[0] = map->GetRiverNum(river_pos) >= 0xF;
    int tree_count = map->GetePlacePartsAtInfoID(info_tree_a, &parts_nos[0], 1);
    tree_count += map->GetePlacePartsAtInfoID(info_tree_b, &parts_nos[1], 1);
    tree_count += map->GetePlacePartsAtInfoID(info_tree_c, &parts_nos[2], 1);
    while (tree_count == 3) {
        for (int i = 0; i < 3; i++) {
            CEditParts *parts = map->GetePlaceParts(parts_nos[i]);
            if (parts != NULL) {
                parts->GetPosition(tree_pos[i]);
            }
            tree_pos[i][1] = 0.0f;
        }
        sceVu0SubVector(to_second, tree_pos[1], tree_pos[0]);
        sceVu0SubVector(to_third, tree_pos[2], tree_pos[0]);
        float length = mgDistVector(to_second);
        if (length <= 800.0f) {
            sceVu0Normalize(to_second, to_second);
            float along = sceVu0InnerProduct(to_second, to_third) / length;
            if (along >= 0.0f && along < 1.0f) {
                if (mgDistLinePoint(tree_pos[2], tree_pos[0], tree_pos[1], closest) <= 100.0f) {
                    condition[1] = 1;
                }
            }
        }
        break;
    }
    int river_total = 0;
    if (map->GetePlacePartsAtInfoID(info_tree_a, &tree_a, 1) != 0) {
        int rivers = map->GetRiverNum(tree_a, 350.0f);
        if (rivers > 6) {
            rivers = 6;
        }
        river_total += rivers;
    }
    if (map->GetePlacePartsAtInfoID(info_tree_b, &tree_b, 1) != 0) {
        int rivers = map->GetRiverNum(tree_b, 350.0f);
        if (rivers > 6) {
            rivers = 6;
        }
        river_total += rivers;
    }
    if (map->GetePlacePartsAtInfoID(info_tree_c, &tree_c, 1) != 0) {
        int rivers = map->GetRiverNum(tree_c, 350.0f);
        if (rivers > 4) {
            rivers = 4;
        }
        river_total += rivers;
    }
    condition[2] = river_total >= 0xF;
    condition[3] = 0;
    target[3] = 2;
    condition[4] = 0;
    target[4] = 0;
    condition[5] = map->CheckLiveNPC(3, -1);
    condition[6] = 0;
    target[6] = 3;
    condition[7] = data->culture_point >= 0x1E;
    condition[8] = map->CheckLiveNPC(0xB, -1);
    condition[9] = map->CheckLiveNPC(0xC, -1);
    condition[10] = map->CheckLiveNPC(0xF, -1);
    enough_trees = GetTreeNum(map) >= 0xA;
    target[12] = 7;
    condition[12] = 0;
    condition[11] = enough_trees;
    condition[13] = map->CheckLiveNPC(-1, 1);
    if (map->GetePlacePartsAtInfoID(info_stone_wall, &stone_wall, 1) > 0) {
        int place_log_max = map->GetChildParts(stone_wall, parts_nos, parts_list_max);
        for (int i = 0; i < place_log_max; i++) {
            if (CheckInfoID(map, parts_nos[i], info_fence) != 0) {
                condition[14] = 1;
            }
        }
    }
    if (map->GetePlacePartsAtInfoID(info_fence, &fence, 1) > 0) {
        int count = map->GetTerritoryParts(fence, parts_nos, parts_list_max);
        int fence_num = 0;
        for (int i = 0; i < count; i++) {
            if (CheckSaku(map, parts_nos[i]) != 0) {
                fence_num++;
            }
        }
        condition[15] = fence_num >= 0xF;
    }
    condition[16] = data->culture_point >= 0x28;
    condition[17] = data->culture_point >= 0x32;
    data->Analize(EDIT_ANALYZE_MAP_SHARLOT, condition, target);
}

/**
 * Stores the Stera town conditions in its saved analysis.
 *
 * @mangled AnalyzeStera__FP9CEditDataP8CEditMap
 * @address 0x31c7f0
 * @size 0x230
 */
static void AnalyzeStera(CEditData *data, CEditMap *map) {
    int condition[EDIT_ANALYZE_CONDITION_MAX];
    int target[EDIT_ANALYZE_CONDITION_MAX];
    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; ++i) {
        condition[i] = 0;
        target[i] = -1;
    }
    map->GroundBalance(0);
    condition[0] = map->BalanceCheck();
    condition[1] = data->culture_point >= 20;
    target[2] = 0;
    condition[3] = GetTreeNum(map) >= 15;
    condition[4] = data->culture_point >= 30;
    target[5] = 2;
    condition[6] = map->GetePlacePartsAtInfoID(4, NULL, 0) >= 4;
    condition[7] = map->CheckLiveNPC(18, -1);
    condition[8] = map->CheckLiveNPC(10, -1);
    condition[9] = data->culture_point >= 40;
    condition[10] = map->CheckLiveNPC(7, 73);
    condition[11] = GetSaveData()->GetBitFlag(0x14A);
    condition[12] = map->CheckLiveNPC(5, -1);
    condition[13] = GetSaveData()->GetBitFlag(0x164);
    target[14] = 1;
    condition[15] = map->CheckLiveNPC(-1, -1) >= 2;
    condition[16] = map->GetePlacePartsAtInfoID(74, NULL, 0) > 0;
    condition[17] = map->CheckLiveNPC(14, -1);
    condition[18] = data->culture_point >= 50;
    target[19] = 4;
    data->Analize(EDIT_ANALYZE_MAP_STERA, condition, target);
}

/**
 * Stores the Benietio town conditions in its saved analysis.
 *
 * @mangled AnalyzeBenietio__FP9CEditDataP8CEditMap
 * @address 0x31ca20
 * @size 0x48c
 */
static void AnalyzeBenietio(CEditData *data, CEditMap *map) {
    int condition[EDIT_ANALYZE_CONDITION_MAX];
    int target[EDIT_ANALYZE_CONDITION_MAX];
    int parts_nos[parts_list_max];
    int territory[parts_list_max];
    int i;
    CEditParts *parts;
    int slot;
    int count;

    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = 0;
        target[i] = -1;
    }
    condition[0] = map->GetePlacePartsAtInfoID(0x35, NULL, 0) >= 8;
    condition[1] = map->CheckLiveNPC(-1, -1) >= 1;
    target[2] = 0;
    count = map->GetePlacePartsAtInfoID(0x1F, parts_nos, parts_list_max);
    for (i = 0; i < count; i++) {
        parts = map->GetePlaceParts(parts_nos[i]);
        if (parts != NULL) {
            CEditPartsInfo *info = parts->info;
            if (info != NULL) {
                int color;
                if (info->paint_num == 1) {
                    color = GetColorType(parts, 0);
                } else {
                    color = GetColorType(parts, 1);
                }
                if (color >= 0) {
                    slot = 3;
                    if (color == 1) {
                        slot = 6;
                    }
                    if (color == 5) {
                        slot = 9;
                    }
                    if (color == 3) {
                        slot = 0xB;
                    }
                    condition[slot] = 1;
                    int territory_count =
                        map->GetTerritoryParts(parts_nos[i], territory, parts_list_max);
                    if (CountPartsInfoID(0x4E, map, territory, territory_count) != 0) {
                        if (slot < 9) {
                            condition[slot + 2] = 1;
                        } else {
                            condition[slot + 1] = 1;
                        }
                    }
                    if (slot == 3) {

                        CEditHouse *extra = parts->house;
                        if (extra == NULL) {
                            continue;
                        }
                        if (extra->npc_no[0] == 8) {
                            condition[slot + 1] = 1;
                        }
                    }
                    if (slot == 6) {
                        CEditHouse *extra = parts->house;
                        if (extra != NULL && extra->npc_no[0] == 4) {
                            condition[slot + 1] = 1;
                        }
                    }
                }
            }
        }
    }
    condition[14] = GetSaveData()->GetBitFlag(0x1BC);
    target[15] = 5;
    condition[16] = map->GetePlacePartsAtInfoID(0x4D, NULL, 0) >= 8;
    condition[17] = map->GetePlacePartsAtInfoID(0x4F, NULL, 0) >= 1;
    condition[18] = data->culture_point >= 0x1E;
    condition[19] = data->culture_point >= 0x32;
    condition[20] = data->culture_point >= 0x3C;
    condition[21] = data->culture_point >= 0x50;
    data->Analize(EDIT_ANALYZE_MAP_BENIETIO, condition, target);
    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = data->analyze.condition[i];
        target[i] = -1;
    }
    int flag1 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_BENIETIO, 1);
    int flag2 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_BENIETIO, 2);
    int flag3 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_BENIETIO, 3);
    int flag4 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_BENIETIO, 4);
    condition[13] = flag1 != 0 && flag2 != 0 && flag3 != 0 && flag4 != 0;
    target[15] = 5;
    data->Analize(EDIT_ANALYZE_MAP_BENIETIO, condition, target);
}

/**
 * Returns the colour classification of the selected part surface.
 *
 * @mangled GetColorType__FP10CEditPartsi
 * @address 0x31ceb0
 * @size 0x128
 */
static int GetColorType(CEditParts *parts, int color_no) {
    float color[4];
    float default_color[4];
    float paint_color[4];
    if (parts == NULL) {
        return -1;
    }
    if (!parts->GetColor(color_no, color)) {
        return -1;
    }
    CEditPartsInfo *info = parts->info;
    if (info == NULL) {
        return -1;
    }
    if (!info->GetDefColor(color_no, default_color)) {
        return -1;
    }
    if (EditPartsCmpColor(color, default_color)) {
        return -1;
    }
    sceVu0ScaleVector(color, color, 128.0f);
    int closest = -1;
    float closest_distance;
    for (int index = 0; index < 8; ++index) {
        GetPenkiColor(index, paint_color);
        float distance = mgDistVector(paint_color, color);
        if (distance <= 2.0f && (closest < 0 || distance < closest_distance)) {
            closest = index;
            closest_distance = distance;
        }
    }
    return closest < 0 ? -1 : closest;
}

/**
 * Stores the Heim town conditions in its saved analysis.
 *
 * @mangled AnalyzeHeim__FP9CEditDataP8CEditMap
 * @address 0x31cfe0
 * @size 0x564
 */
static void AnalyzeHeim(CEditData *data, CEditMap *map) {
    int condition[EDIT_ANALYZE_CONDITION_MAX];
    int target[EDIT_ANALYZE_CONDITION_MAX];
    int house_nos[parts_list_max];
    int child_nos[parts_list_max];
    float position[4];
    int i;

    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = 0;
        target[i] = -1;
    }
    int bit_a = GetSaveData()->GetBitFlag(0x208);
    int bit_b = GetSaveData()->GetBitFlag(0x218);
    int placed = 0;
    int house_num = GetHouseParts(map, house_nos, parts_list_max);
    for (i = 0; i < house_num; i++) {
        CEditParts *parts = map->GetePlaceParts(house_nos[i]);
        if (parts != NULL) {
            parts->GetPosition(position);
            placed++;
            if (position[1] >= 125.0f) {
                condition[1] = 1;
            }
            if (position[1] >= 167.0f && parts->GetLiveNPC() > 0) {
                condition[16] = 1;
            }
            int has_a = 0;
            int has_b = 0;
            int has_c = 0;
            int has_fence = 0;
            int place_log = map->GetChildParts(house_nos[i], child_nos, parts_list_max);
            if (CountPartsInfoID(0x51, map, child_nos, place_log) > 0) {
                has_a = 1;
            }
            if (CountPartsInfoID(0x52, map, child_nos, place_log) > 0) {
                has_b = 1;
            }
            if (CountPartsType(3, map, child_nos, place_log) > 0) {
                has_c = 1;
            }
            int resident = parts->GetLiveNPC();
            if (has_a != 0 && resident > 0) {
                condition[9] = 1;
            }
            if (has_b != 0 && resident > 0) {
                condition[11] = 1;
            }
            if (has_c != 0 && resident > 0) {
                condition[17] = 1;
            }
            if (CountPartsInfoID(0x4F, map, child_nos,
                                 map->GetTerritoryParts(house_nos[i], child_nos, parts_list_max)) >
                0) {
                has_fence = 1;
            }
            if (has_fence != 0 && resident == 0xD) {
                condition[18] = 1;
            }
        }
    }
    condition[0] = placed >= 3;
    target[3] = 1;
    target[2] = 0;
    condition[4] = map->CheckLiveNPC(2, -1);
    if (map->GetePlacePartsAtInfoID(0x50, NULL, 0) > 0) {
        condition[6] = 1;
    }
    condition[7] = map->CheckLiveNPC(1, -1);
    target[8] = 3;
    condition[10] = map->GetePlacePartsAtInfoID(0x4F, NULL, 0) >= 1;
    target[12] = 6;
    condition[13] =
        map->GetePlacePartsAtInfoID(5, NULL, 0) + map->GetePlacePartsAtInfoID(4, NULL, 0) >= 0xA;
    condition[14] = bit_a;
    condition[15] = map->CheckLiveNPC(0x10, -1);
    condition[19] = bit_b;
    condition[20] = data->culture_point >= 0x1E;
    condition[21] = data->culture_point >= 0x3C;
    condition[22] = data->culture_point >= 0x46;
    condition[23] = data->culture_point >= 0x50;
    condition[24] = data->culture_point >= 0x64;
    target[25] = 2;
    data->Analize(EDIT_ANALYZE_MAP_HEIM, condition, target);
    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = data->analyze.condition[i];
        target[i] = -1;
    }
    int flag4 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_HEIM, 4);
    int flag5 = data->GetAnalyzeFlag(EDIT_ANALYZE_MAP_HEIM, 5);
    condition[5] = flag4 != 0 && flag5 != 0;
    target[8] = 3;
    target[12] = 6;
    data->Analize(EDIT_ANALYZE_MAP_HEIM, condition, target);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editanalyze", AnalyzeMoonFlower__FP9CEditDataP8CEditMap);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editanalyze", CheckLiveChara__FiP8CEditMapii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editanalyze", EditMapInitEvent__FiP8CEditMap);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editanalyze", at_913__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editanalyze", at_964__4__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editanalyze", at_1618__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editanalyze", at_1632__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editanalyze", at_1633__3__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1297__4, 0x10);
