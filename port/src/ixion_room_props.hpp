#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

struct IxionRoom;
class CTreasureBoxManager;

// One DC1 placed object from the converter's props.json sidecar ("ixion-room-props" v1).
// Positions are already in room (and so DC2 world) units, dropped onto the pack's ground.
struct IxionRoomProp {
    std::string id;
    std::string kind;  // "treasure_box", "monster_spawn" or "atla"
    std::array<float, 3> position{};
    float rotation_y = 0.0f;
    int dc1_item = -1; // treasure_box only
    int count = 1;
    // treasure_box only: an authored DC2 item for a DC1 item the built-in table does not map (a
    // stand-in the converter chose and named in contents.mapping). -1 when absent.
    int dc2_item = -1;
    std::string model; // monster_spawn only: the DC1 roster id (e.g. "e03a") donor.model
    int slot = -1;     // monster_spawn only: DC1's own arrangement slot, donor.slot
};

// DC2's item numbers run 1..428 (menu/cfg7/comdat.cfg); an authored dc2_item outside it is refused at load.
constexpr int kIxionRoomPropsDc2ItemMax = 428;

struct IxionRoomProps {
    std::string room;
    std::vector<IxionRoomProp> props;
    // "park" (or empty): what to do with live native treasure boxes inside the room's bounds. A random
    // floor pack spans most of the DC2 floor, so native boxes always lie inside it; without this the
    // installation is refused (N9). "park" hides them for the visit (see IxionRoomPropsParkNativeBoxes).
    std::string native_boxes;

    static bool Load(const std::filesystem::path &json_path, IxionRoomProps &out, std::string &error);
};

// DC1 item -> native DC2 item for a dungeon, or -1 when unmapped. A small explicit table:
// DC1 195 (the floor's gate key) is DC2's own gate key, GetGateKeyIndex(0, floor) == 337.
int IxionRoomPropsMapItem(int dc1_item, int dungeon);

// The item a treasure_box prop holds: the built-in table first (the gate key must stay DC2's own),
// then the authored dc2_item stand-in, else -1.
int IxionRoomPropsResolveItem(const IxionRoomProp &prop, int dungeon);

// DC1's own monster arrangement for a floor pack (props.json monster_spawn entries): which DC1
// monster, where. Empty when props are off, absent, or built for another room.
struct IxionRoomMonsterSpawn {
    std::string id;
    std::string model;
    std::array<float, 3> position{};
    int slot = -1;
};
std::vector<IxionRoomMonsterSpawn> IxionRoomPropsMonsterSpawns(const IxionRoom &room);

// A native CTreasureBox slot that the adapter filled; it is ours, not native.
struct IxionRoomPlacedBox {
    std::string id;
    int slot = -1;
    int item = -1;
    std::array<float, 3> position{};
};

// Puts every mappable treasure_box prop into a free native slot of `boxes` through the retail
// CTreasureBoxManager::PutTreasureBox (flags 0x41, as AutoSetTreasureBox / ext 66 use). Slots in
// `reserved` (native boxes parked by the room, state NONE but owed back) are never taken.
// Returns the number placed. Props outside the room bounds or unmapped are skipped and logged.
int IxionRoomPropsPlace(CTreasureBoxManager *boxes, const IxionRoomProps &props, const IxionRoom &room, int dungeon,
                        const std::vector<int> &reserved, std::vector<IxionRoomPlacedBox> &placed);

// Clears our boxes at retirement. There is no removal API, so a slot is cleared (state NONE) only
// while it still holds our box, unopened or opened (the native open script leaves the slot live with
// the lid open); identity is the exact position we wrote. An empty or reused slot is left as it is.
// A null manager (scene changed) only drops the tracking.
void IxionRoomPropsRemove(CTreasureBoxManager *boxes, std::vector<IxionRoomPlacedBox> &placed);

// Parking, opt-in per pack (props.json "native_boxes":"park"). A parked native box keeps its slot
// (state unchanged, so nothing can post into it) but is moved 1e6 units below the floor: out of the
// 3D pickup test (CTreasureBoxManager::CheckEvent), the 1000-unit draw range and the room-bounds test.
// Retire puts it back at its exact original position, and only while the slot still holds the box at
// the parked position. Park runs once per installation, after every point where installation can fail.
struct IxionRoomParkedBox {
    int slot = -1;
    std::array<float, 3> original{};
    std::array<float, 3> parked{};
};
bool IxionRoomPropsParksNativeBoxes(const IxionRoom &room);
int IxionRoomPropsParkNativeBoxes(const IxionRoom &room);
// The same on an explicit manager (the runtime hooks use the live scene's): park every live box
// inside the room's bounds; restore puts back only boxes still live at the parked position.
int IxionRoomPropsParkBoxes(CTreasureBoxManager *boxes, const IxionRoom &room, std::vector<IxionRoomParkedBox> &parked);
void IxionRoomPropsRestoreBoxes(CTreasureBoxManager *boxes, std::vector<IxionRoomParkedBox> &parked);

// Runtime hooks for ixion_room.cpp. Opt-in: props.json beside DC2_IXION_ROOM; DC2_IXION_ROOM_PROPS=0
// disables. Opened identities survive continuing-floor retirement. Departure/BeginFloor/Discard
// end that logical lifetime; Discard never touches the stale scene.
void IxionRoomPropsInstall(const IxionRoom &room, const std::vector<int> &reserved_slots, int dungeon);
void IxionRoomPropsRetire(bool same_scene, bool departing = false);
void IxionRoomPropsBeginFloor();
void IxionRoomPropsDiscard();
// Test seam: forget the latched props.json load.
void IxionRoomPropsResetForTest();

// Exact native slot ownership, including the synchronous placement callback.
bool IxionRoomPropsOwnsSlot(const CTreasureBoxManager *boxes, int slot);
