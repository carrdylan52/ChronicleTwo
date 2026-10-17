#include "common.h"

#include <cstring>

#include "mainloop.hpp"
#include "map.hpp"
#include "map_port.hpp"
#include "mapload.hpp"
#include "mapparts.hpp"
#include "mg_memory.hpp"
#include "savedata.hpp"

PORT_OVERRIDE void CMap::SetPlacePartsBuff(mgCMemory *memory, int capacity) {
    const std::size_t parts_bytes = sizeof(CMapParts) * capacity;
    place_parts = new (memory->Alloc((parts_bytes + 15) / 16 + 2)) CMapParts[capacity];
    const std::size_t pointers_bytes = sizeof(CMapParts *) * capacity;
    draw_parts = new (memory->Alloc((pointers_bytes + 15) / 16 + 2)) CMapParts *[capacity];
    place_parts_max = capacity;
    ClearPlaceParts();
}

namespace {
struct TreasureFlagFix {
    int         map_no;
    const char *point_name;
    int         retail_flag;
    int         fixed_flag;
};

// Retail data gives two of Starlight Temple's (map 26, f02.map) chests the same map flag 7: "item19", the
// Fruit of Eden by the weapon platform (walkthrough chest 17, floor 309), and "item11", the Fruit of Eden by
// Starlight Items (chest 20) that needs "Looking for the Wind Gem" (floor 312). Opening item19 before that floor
// is entered hides item11 for good (upstream issue #25). Flag 9 is the one number from 1 to 20 that no chest of
// the map uses. A save whose flag 7 is already set gets item11 back.
constexpr TreasureFlagFix kTreasureFlagFixes[] = {
    {26, "item11", 7, 9},
};
} // namespace

int PortTreasureBoxFlag(int map_no, const char *point_name, int flag_no) {
    if (point_name == nullptr) {
        return flag_no;
    }
    for (const auto &fix : kTreasureFlagFixes) {
        if (fix.map_no == map_no && fix.retail_flag == flag_no && std::strcmp(fix.point_name, point_name) == 0) {
            return fix.fixed_flag;
        }
    }
    return flag_no;
}

// Retail's body, after giving the chests that share another chest's flag their own (PortTreasureBoxFlag).
PORT_OVERRIDE void CMap::UpdateTrBoxFlag(CMapFlagData *flag_data) {
    if (flag_data == NULL) {
        return;
    }

    int        map_no = -1;
    CSaveData *save = GetSaveData();
    for (const auto &fix : kTreasureFlagFixes) {
        if (save != NULL && flag_data == save->GetMapFlag(fix.map_no)) {
            map_no = fix.map_no;
        }
    }

    CMapTreasureBox *box = tr_box;
    for (int i = 0; i < tr_box_num; i++, box++) {
        if (map_no >= 0 && box->func_point != NULL) {
            box->flag_no = PortTreasureBoxFlag(map_no, box->func_point->name, box->flag_no);
        }

        if (box->flag_no > 0) {
            box->active = !(flag_data->GetFlag(box->flag_no) != 0);

            if (box->func_point != NULL) {
                box->func_point->enable = box->active;
            }
        }
    }
}
