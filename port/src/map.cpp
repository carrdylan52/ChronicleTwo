#include "common.h"

#include <algorithm>
#include <span>
#include <string_view>

#include "bounded_text.hpp"
#include "map.hpp"
#include "map_port.hpp"
#include "mapload.hpp"
#include "mapparts.hpp"
#include "mg_memory.hpp"

PORT_OVERRIDE void CMap::SetPlacePartsBuff(mgCMemory *memory, int capacity) {
    const std::size_t parts_bytes = sizeof(CMapParts) * capacity;
    place_parts = new (memory->Alloc((parts_bytes + 15) / 16 + 2)) CMapParts[capacity];
    const std::size_t pointers_bytes = sizeof(CMapParts *) * capacity;
    draw_parts = new (memory->Alloc((pointers_bytes + 15) / 16 + 2)) CMapParts *[capacity];
    place_parts_max = capacity;
    ClearPlaceParts();
}

namespace {

struct TreasureBoxFlagFix {
    std::string_view map_name;
    std::string_view placement_name; // Empty preserves the existing correction regardless of the point's owner.
    std::string_view point_name;
    int              retail_flag;
    int              opened_flag;
};

// Keep these assignments stable: they are stored in existing saves. f02's three p01_f0205 placements inherit
// item10's flag 3; its item19 and item11 also share flag 7. Flags 9, 21 and 22 are otherwise unused in f02.
// Other maps can intentionally reuse flags for alternate placements, so do not renumber duplicates globally.
constexpr TreasureBoxFlagFix kTreasureBoxFlagFixes[] = {
    {"f02", "", "item11", 7, 9},
    {"f02", "p01_f0205-1", "item10", 3, 21},
    {"f02", "p01_f0205-2", "item10", 3, 22},
};

int ResolveTreasureBoxFlag(const CMapTreasureBox &box, std::string_view map_name) {
    if (box.func_point == nullptr) {
        return box.flag_no;
    }

    const auto point_name     = bounded_text::View(box.func_point->name);
    const auto placement_name = box.parts != nullptr
                                    ? bounded_text::View(box.parts->name, sizeof(box.parts->name))
                                    : std::string_view();
    for (const auto &fix : kTreasureBoxFlagFixes) {
        if (map_name == fix.map_name && point_name == fix.point_name && box.flag_no == fix.retail_flag &&
            (fix.placement_name.empty() || placement_name == fix.placement_name)) {
            return fix.opened_flag;
        }
    }
    return box.flag_no;
}

} // namespace

void PortSplitTreasureBoxFlags(CMap &map, std::string_view map_name) {
    if (map.tr_box == nullptr) {
        return;
    }

    for (CMapTreasureBox &box : std::span(map.tr_box, static_cast<std::size_t>(std::max(map.tr_box_num, 0)))) {
        // Both UpdateTrBoxFlag and DeleteTrBox use this value, including temporary hides with no save flags.
        box.flag_no = ResolveTreasureBoxFlag(box, map_name);
    }
}
