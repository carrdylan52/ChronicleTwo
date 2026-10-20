#include "common.h"

#include "ixion_room_props.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string_view>
#include <unordered_set>

#include "dng_event.hpp"
#include "dng_main.hpp"
#include "ixion_room.hpp"
#include "platform/paths.hpp"
#include "scene.hpp"

namespace {

using Json = nlohmann::json;
constexpr std::uintmax_t kPropsJsonLimit = 4u * 1024u * 1024u;
constexpr size_t kMaxProps = 64;
// Flags of a script-placed single-item box: AutoSetTreasureBox (dng_event.cpp:1679-1681).
constexpr int kScriptBoxFlags = 0x41;
int g_placing_slot = -1;
const CTreasureBoxManager *g_placing_boxes = nullptr;

bool Fail(std::string &error, const std::string &message) {
    error = message;
    return false;
}

bool Finite(const Json &value, float &out) {
    if (!value.is_number()) return false;
    const double number = value.get<double>();
    if (!std::isfinite(number) || std::abs(number) > 1.0e7) return false;
    out = static_cast<float>(number);
    return true;
}

struct ItemMapping {
    int dc1_item;
    int dungeon;
    int dc2_item;
};
// DC1 195: SET_IBOX(-93,0,-318,195) in d01 event.stb program 100, the floor's gate key
// (crosswalk-items-world.md:95). DC2 dungeon 0's gate key is gatekey_index[0] = 337 "Key Handle".
constexpr ItemMapping kItemMap[] = {
    {195, 0, 337},
};

// Where a parked native box waits: far below any floor (the DC2 world spans a few thousand units).
constexpr float kParkedY = -1.0e6f;
constexpr float kParkMargin = 40.0f; // the native pickup gate, as IxionRoomOverlappingTreasureBox

struct PropsRuntime {
    bool attempted = false;
    bool enabled = false;
    IxionRoomProps props;
    std::vector<IxionRoomPlacedBox> placed;
    // Logical floor lifetime, independent of adapter installation generations.
    std::unordered_set<std::string> consumed;
    std::vector<IxionRoomParkedBox> parked;
};

PropsRuntime &Runtime() {
    static PropsRuntime runtime;
    return runtime;
}

CTreasureBoxManager *LiveBoxes() {
    return DngMainScene != nullptr ? DngMainScene->battle_area.treasure_box : nullptr;
}

void LoadOnce(PropsRuntime &runtime) {
    if (runtime.attempted) return;
    runtime.attempted = true;
    if (const char *flag = std::getenv("DC2_IXION_ROOM_PROPS"); flag != nullptr && std::string_view(flag) == "0") {
        std::fprintf(stderr, "ixion room props disabled by DC2_IXION_ROOM_PROPS=0\n");
        return;
    }
    const char *room_path = std::getenv("DC2_IXION_ROOM");
    if (room_path == nullptr || *room_path == '\0') return;
    const std::filesystem::path path = PathsFromUtf8(room_path).parent_path() / "props.json";
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        std::fprintf(stderr, "ixion room props: none (no props.json beside room.json)\n");
        return;
    }
    std::string error;
    if (!IxionRoomProps::Load(path, runtime.props, error)) {
        std::fprintf(stderr, "ixion room props disabled: %s\n", error.c_str());
        return;
    }
    runtime.enabled = true;
    std::fprintf(stderr, "ixion room props loaded: room=%s props=%zu\n", runtime.props.room.c_str(),
                 runtime.props.props.size());
}

} // namespace

bool IxionRoomProps::Load(const std::filesystem::path &json_path, IxionRoomProps &out, std::string &error) {
    try {
        std::error_code ec;
        const std::uintmax_t size = std::filesystem::file_size(json_path, ec);
        if (ec || size == 0 || size > kPropsJsonLimit) return Fail(error, "cannot read props.json or it exceeds 4 MiB");
        std::ifstream file(json_path, std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (!file.good() && !file.eof()) return Fail(error, "cannot read props.json");
        const Json doc = Json::parse(text, nullptr, false);
        if (doc.is_discarded() || !doc.is_object()) return Fail(error, "props.json is not a JSON object");
        if (!doc.contains("format") || !doc["format"].is_string() || doc["format"] != "ixion-room-props" ||
            !doc.contains("version") || !doc["version"].is_number_integer() || doc["version"] != 1)
            return Fail(error, "unsupported props.json version");
        if (!doc.contains("room") || !doc["room"].is_string()) return Fail(error, "props.json lacks room");
        if (!doc.contains("props") || !doc["props"].is_array() || doc["props"].size() > kMaxProps)
            return Fail(error, "props.json props must be an array of at most 64");
        IxionRoomProps parsed;
        std::unordered_set<std::string> ids;
        parsed.room = doc["room"].get<std::string>();
        if (doc.contains("native_boxes")) {
            if (!doc["native_boxes"].is_string() || doc["native_boxes"].get<std::string>() != "park")
                return Fail(error, "props.json native_boxes must be \"park\"");
            parsed.native_boxes = "park";
        }
        for (const Json &entry : doc["props"]) {
            if (!entry.is_object() || !entry.contains("kind") || !entry["kind"].is_string() || !entry.contains("id") ||
                !entry["id"].is_string())
                return Fail(error, "prop needs string id and kind");
            IxionRoomProp prop;
            prop.id = entry["id"].get<std::string>();
            if (prop.id.empty() || !ids.insert(prop.id).second) return Fail(error, "prop id must be unique and nonempty");
            prop.kind = entry["kind"].get<std::string>();
            const Json &pos = entry.contains("position") ? entry["position"] : Json();
            if (!pos.is_array() || pos.size() != 3 || !Finite(pos[0], prop.position[0]) || !Finite(pos[1], prop.position[1]) ||
                !Finite(pos[2], prop.position[2]))
                return Fail(error, "prop " + prop.id + " needs a finite position");
            if (entry.contains("rotation_y") && !Finite(entry["rotation_y"], prop.rotation_y))
                return Fail(error, "prop " + prop.id + " has an invalid rotation_y");
            if (prop.kind == "treasure_box") {
                const Json &contents = entry.contains("contents") ? entry["contents"] : Json();
                if (!contents.is_object() || !contents.contains("dc1_item") || !contents["dc1_item"].is_number_integer())
                    return Fail(error, "treasure_box " + prop.id + " needs contents.dc1_item");
                if (contents["dc1_item"] < 0 || contents["dc1_item"] > 0x7fff)
                    return Fail(error, "treasure_box item out of range");
                prop.dc1_item = contents["dc1_item"].get<int>();
                if (contents.contains("count")) {
                    if (!contents["count"].is_number_integer()) return Fail(error, "treasure_box count must be an integer");
                    if (contents["count"] < 1 || contents["count"] > 99) return Fail(error, "treasure_box count out of range");
                    prop.count = contents["count"].get<int>();
                }
                if (prop.dc1_item < 0 || prop.dc1_item > 0x7fff || prop.count < 1 || prop.count > 99)
                    return Fail(error, "treasure_box " + prop.id + " has an out-of-range item or count");
                if (contents.contains("dc2_item")) {
                    if (!contents["dc2_item"].is_number_integer()) return Fail(error, "treasure_box dc2_item must be an integer");
                    if (contents["dc2_item"] < 1 || contents["dc2_item"] > kIxionRoomPropsDc2ItemMax)
                        return Fail(error, "treasure_box " + prop.id + " has an out-of-range dc2_item");
                    prop.dc2_item = contents["dc2_item"].get<int>();
                }
            } else if (prop.kind == "monster_spawn") {
                const Json &donor = entry.contains("donor") ? entry["donor"] : Json();
                if (!donor.is_object() || !donor.contains("model") || !donor["model"].is_string())
                    return Fail(error, "monster_spawn " + prop.id + " needs donor.model");
                prop.model = donor["model"].get<std::string>();
                const bool plain = !prop.model.empty() && prop.model.size() <= 15 &&
                                   std::all_of(prop.model.begin(), prop.model.end(), [](unsigned char c) { return std::isalnum(c) != 0; });
                if (!plain) return Fail(error, "monster_spawn " + prop.id + " has an invalid donor.model");
                if (donor.contains("slot")) {
                    if (!donor["slot"].is_number_integer()) return Fail(error, "monster_spawn slot must be an integer");
                    prop.slot = donor["slot"].get<int>();
                }
            }
            parsed.props.push_back(std::move(prop));
        }
        out = std::move(parsed);
        error.clear();
        return true;
    } catch (const std::exception &exception) {
        return Fail(error, std::string("invalid props JSON: ") + exception.what());
    }
}

int IxionRoomPropsMapItem(int dc1_item, int dungeon) {
    for (const ItemMapping &mapping : kItemMap) {
        if (mapping.dc1_item == dc1_item && mapping.dungeon == dungeon) return mapping.dc2_item;
    }
    return -1;
}

int IxionRoomPropsResolveItem(const IxionRoomProp &prop, int dungeon) {
    const int builtin = IxionRoomPropsMapItem(prop.dc1_item, dungeon);
    if (builtin >= 0) return builtin;
    return prop.dc2_item >= 1 && prop.dc2_item <= kIxionRoomPropsDc2ItemMax ? prop.dc2_item : -1;
}

std::vector<IxionRoomMonsterSpawn> IxionRoomPropsMonsterSpawns(const IxionRoom &room) {
    PropsRuntime &runtime = Runtime();
    LoadOnce(runtime);
    std::vector<IxionRoomMonsterSpawn> out;
    if (!runtime.enabled || runtime.props.room != room.donor_room) return out;
    for (const IxionRoomProp &prop : runtime.props.props) {
        if (prop.kind != "monster_spawn") continue;
        bool inside = true;
        for (int axis = 0; axis < 3; ++axis) {
            inside = inside && prop.position[axis] >= room.bounds_min[axis] && prop.position[axis] <= room.bounds_max[axis];
        }
        if (!inside) {
            std::fprintf(stderr, "ixion room prop %s skipped: position outside the room bounds\n", prop.id.c_str());
            continue;
        }
        out.push_back({prop.id, prop.model, prop.position, prop.slot});
    }
    return out;
}

int IxionRoomPropsPlace(CTreasureBoxManager *boxes, const IxionRoomProps &props, const IxionRoom &room, int dungeon,
                        const std::vector<int> &reserved, std::vector<IxionRoomPlacedBox> &placed) {
    if (boxes == nullptr) {
        std::fprintf(stderr, "ixion room props: no native treasure box manager; nothing placed\n");
        return 0;
    }
    int count = 0;
    for (const IxionRoomProp &prop : props.props) {
        if (prop.kind == "monster_spawn") continue; // placed by the roster install, not here
        if (prop.kind != "treasure_box") {
            std::fprintf(stderr, "ixion room prop %s kind=%s deferred (no native placement yet)\n", prop.id.c_str(),
                         prop.kind.c_str());
            continue;
        }
        const int item = IxionRoomPropsResolveItem(prop, dungeon);
        if (item < 0) {
            std::fprintf(stderr, "ixion room prop %s skipped: DC1 item %d has no DC2 mapping for dungeon %d\n",
                         prop.id.c_str(), prop.dc1_item, dungeon);
            continue;
        }
        bool inside = true;
        for (int axis = 0; axis < 3; ++axis) {
            inside = inside && prop.position[axis] >= room.bounds_min[axis] && prop.position[axis] <= room.bounds_max[axis];
        }
        if (!inside) {
            std::fprintf(stderr, "ixion room prop %s skipped: position outside the room bounds\n", prop.id.c_str());
            continue;
        }
        int slot = -1;
        for (int i = 0; i < TREASURE_BOX_MAX && slot < 0; ++i) {
            if (boxes->box[i].state == TREASURE_BOX_STATE_NONE &&
                std::find(reserved.begin(), reserved.end(), i) == reserved.end())
                slot = i;
        }
        if (slot < 0) {
            // PutTreasureBox would return silently with every slot full (dng_event.cpp:691-705).
            std::fprintf(stderr, "ixion room prop %s skipped: no free native treasure box slot\n", prop.id.c_str());
            continue;
        }
        float position[4] = {prop.position[0], prop.position[1], prop.position[2], 1.0f};
        g_placing_boxes = boxes;
        g_placing_slot = slot;
        boxes->PutTreasureBox(slot, position, prop.rotation_y, kScriptBoxFlags, item, prop.count, -1, 0);
        g_placing_slot = -1;
        g_placing_boxes = nullptr;
        CTreasureBox &box = boxes->box[slot];
        if (box.state != TREASURE_BOX_STATE_UNOPENED || box.item[0] != item) {
            std::fprintf(stderr, "ixion room prop %s: native PutTreasureBox did not fill slot %d\n", prop.id.c_str(), slot);
            continue;
        }
        placed.push_back({prop.id, slot, item, prop.position});
        ++count;
        std::fprintf(stderr,
                     "ixion room prop %s placed native treasure box slot=%d dc1_item=%d dc2_item=%d (%s) count=%d pos=(%.3f,%.3f,%.3f) rot=%.3f flags=0x%x\n",
                     prop.id.c_str(), slot, prop.dc1_item, item,
                     IxionRoomPropsMapItem(prop.dc1_item, dungeon) >= 0 ? "built-in table" : "authored mapping", prop.count,
                     position[0], position[1], position[2], prop.rotation_y, kScriptBoxFlags);
    }
    return count;
}

void IxionRoomPropsRemove(CTreasureBoxManager *boxes, std::vector<IxionRoomPlacedBox> &placed) {
    for (const IxionRoomPlacedBox &ours : placed) {
        if (boxes == nullptr) {
            std::fprintf(stderr, "ixion room prop %s slot=%d not touched (owner scene changed)\n", ours.id.c_str(),
                         ours.slot);
            continue;
        }
        CTreasureBox &box = boxes->box[ours.slot];
        float position[4] = {};
        box.GetPosition(position);
        // The native open script keeps the slot live with a script-set state (_SET_TB_STATUS) and the lid
        // open, so an opened box of ours is still ours: identity is the exact position we wrote. Only
        // PutTreasureBox at the identical float position could reuse the slot unnoticed.
        const bool ours_still = box.state != TREASURE_BOX_STATE_NONE && position[0] == ours.position[0] &&
                                position[1] == ours.position[1] && position[2] == ours.position[2];
        if (box.state == TREASURE_BOX_STATE_NONE) {
            std::fprintf(stderr, "ixion room prop %s slot=%d already empty\n", ours.id.c_str(), ours.slot);
        } else if (ours_still) {
            const int state = box.state;
            const int item = box.item[0];
            const float lid = box.lid_open;
            box.state = TREASURE_BOX_STATE_NONE;
            box.lid_open = 0.0f;
            if (boxes->near_box == ours.slot) boxes->near_box = -1;
            std::fprintf(stderr, "ixion room prop %s slot=%d removed %s box at retire (state=%d item=%d lid=%.2f)\n",
                         ours.id.c_str(), ours.slot, state == TREASURE_BOX_STATE_UNOPENED ? "unopened" : "opened",
                         state, item, lid);
        } else {
            std::fprintf(stderr, "ixion room prop %s slot=%d reused by another box (state=%d item=%d); left alone\n",
                         ours.id.c_str(), ours.slot, box.state, box.item[0]);
        }
    }
    placed.clear();
}

void IxionRoomPropsInstall(const IxionRoom &room, const std::vector<int> &reserved_slots, int dungeon) {
    PropsRuntime &runtime = Runtime();
    LoadOnce(runtime);
    if (!runtime.enabled) return;
    if (!runtime.placed.empty()) {
        // A previous generation was never retired through RetireScene; never place twice.
        std::fprintf(stderr, "ixion room props: %zu boxes still tracked at install; not placing again\n",
                     runtime.placed.size());
        return;
    }
    if (runtime.props.room != room.donor_room) {
        std::fprintf(stderr, "ixion room props skipped: props.json is for %s, room is %s\n", runtime.props.room.c_str(),
                     room.donor_room.c_str());
        return;
    }
    IxionRoomProps remaining = runtime.props;
    std::erase_if(remaining.props, [&](const auto &prop) { return runtime.consumed.contains(prop.id); });
    const int count = IxionRoomPropsPlace(LiveBoxes(), remaining, room, dungeon, reserved_slots, runtime.placed);
    std::fprintf(stderr, "ixion room props installed: treasure_boxes=%d\n", count);
}

bool IxionRoomPropsParksNativeBoxes(const IxionRoom &room) {
    PropsRuntime &runtime = Runtime();
    LoadOnce(runtime);
    return runtime.enabled && runtime.props.native_boxes == "park" && runtime.props.room == room.donor_room;
}

int IxionRoomPropsParkBoxes(CTreasureBoxManager *boxes, const IxionRoom &room, std::vector<IxionRoomParkedBox> &parked) {
    if (boxes == nullptr) return 0;
    int count = 0;
    for (int i = 0; i < TREASURE_BOX_MAX; ++i) {
        CTreasureBox &box = boxes->box[i];
        if (box.state == TREASURE_BOX_STATE_NONE) continue;
        float position[4] = {};
        box.GetPosition(position);
        const bool inside = position[0] >= room.bounds_min[0] - kParkMargin && position[0] <= room.bounds_max[0] + kParkMargin &&
                            position[2] >= room.bounds_min[2] - kParkMargin && position[2] <= room.bounds_max[2] + kParkMargin &&
                            position[1] >= room.bounds_min[1] - 200.0f && position[1] <= room.bounds_max[1] + 200.0f;
        if (!inside) continue;
        float away[4] = {position[0], kParkedY, position[2], 1.0f};
        box.SetPosition(away);
        if (boxes->near_box == i) boxes->near_box = -1;
        parked.push_back({i, {position[0], position[1], position[2]}, {away[0], away[1], away[2]}});
        ++count;
        std::fprintf(stderr, "ixion room native treasure box index=%d parked from (%.1f,%.1f,%.1f) for the visit (state=%d item=%d)\n", i,
                     position[0], position[1], position[2], box.state, box.item[0]);
    }
    return count;
}

void IxionRoomPropsRestoreBoxes(CTreasureBoxManager *boxes, std::vector<IxionRoomParkedBox> &parked) {
    for (const IxionRoomParkedBox &entry : parked) {
        if (boxes == nullptr) {
            std::fprintf(stderr, "ixion room native treasure box index=%d not touched (owner scene changed)\n", entry.slot);
            continue;
        }
        CTreasureBox &box = boxes->box[entry.slot];
        float position[4] = {};
        box.GetPosition(position);
        const bool still = box.state != TREASURE_BOX_STATE_NONE && position[0] == entry.parked[0] &&
                           position[1] == entry.parked[1] && position[2] == entry.parked[2];
        if (!still) {
            std::fprintf(stderr, "ixion room native treasure box index=%d changed while parked (state=%d); left alone\n", entry.slot,
                         box.state);
            continue;
        }
        float back[4] = {entry.original[0], entry.original[1], entry.original[2], 1.0f};
        box.SetPosition(back);
        std::fprintf(stderr, "ixion room native treasure box index=%d restored to (%.1f,%.1f,%.1f) (state=%d)\n", entry.slot, back[0],
                     back[1], back[2], box.state);
    }
    parked.clear();
}

int IxionRoomPropsParkNativeBoxes(const IxionRoom &room) {
    PropsRuntime &runtime = Runtime();
    if (!IxionRoomPropsParksNativeBoxes(room) || LiveBoxes() == nullptr) return 0;
    if (!runtime.parked.empty()) {
        std::fprintf(stderr, "ixion room props: %zu native boxes still parked at install; not parking again\n",
                     runtime.parked.size());
        return 0;
    }
    const int count = IxionRoomPropsParkBoxes(LiveBoxes(), room, runtime.parked);
    std::fprintf(stderr, "ixion room props: parked %d native treasure boxes inside the pack's bounds\n", count);
    return count;
}

void IxionRoomPropsRetire(bool same_scene, bool departing) {
    PropsRuntime &runtime = Runtime();
    if (runtime.placed.empty() && runtime.parked.empty()) {
        if (departing || !same_scene) runtime.consumed.clear();
        return;
    }
    if (same_scene && LiveBoxes() != nullptr) {
        for (const auto &ours : runtime.placed) {
            auto &box = LiveBoxes()->box[ours.slot];
            float position[4]{};
            box.GetPosition(position);
            if (position[0] == ours.position[0] && position[1] == ours.position[1] &&
                position[2] == ours.position[2] && box.state != TREASURE_BOX_STATE_UNOPENED)
                runtime.consumed.insert(ours.id);
        }
    }
    IxionRoomPropsRemove(same_scene ? LiveBoxes() : nullptr, runtime.placed);
    if (departing || !same_scene) runtime.consumed.clear();
    IxionRoomPropsRestoreBoxes(same_scene ? LiveBoxes() : nullptr, runtime.parked);
}

void IxionRoomPropsBeginFloor() { Runtime().consumed.clear(); }

void IxionRoomPropsDiscard() {
    PropsRuntime &runtime = Runtime();
    if (!runtime.placed.empty()) {
        std::fprintf(stderr, "ixion room props: dropped %zu tracked boxes without touching the stale scene\n",
                     runtime.placed.size());
    }
    if (!runtime.parked.empty()) {
        std::fprintf(stderr, "ixion room props: dropped %zu parked native boxes without touching the stale scene\n",
                     runtime.parked.size());
    }
    runtime.placed.clear();
    runtime.consumed.clear();
    runtime.parked.clear();
}

void IxionRoomPropsResetForTest() {
    Runtime() = PropsRuntime{};
}

bool IxionRoomPropsOwnsSlot(const CTreasureBoxManager *boxes, int slot) {
    if (boxes == nullptr || boxes != LiveBoxes() || slot < 0 || slot >= TREASURE_BOX_MAX) return false;
    if (boxes == g_placing_boxes && slot == g_placing_slot) return true;
    auto &box = const_cast<CTreasureBox &>(boxes->box[slot]);
    if (box.state == TREASURE_BOX_STATE_NONE) return false;
    float position[4] = {};
    box.GetPosition(position);
    for (const auto &ours : Runtime().placed) {
        if (ours.slot == slot && position[0] == ours.position[0] && position[1] == ours.position[1] &&
            position[2] == ours.position[2]) return true;
    }
    return false;
}
