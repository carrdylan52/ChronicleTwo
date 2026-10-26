#pragma once

#include <string_view>

class CMap;

// Resolves known retail chest flag collisions by map, placement and function point after CreateTrBox and before
// UpdateTrBoxFlag. Both the saved-state read and DeleteTrBox's write then use the same stable box.flag_no.
void PortSplitTreasureBoxFlags(CMap &map, std::string_view map_name);
