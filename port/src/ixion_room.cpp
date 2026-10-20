#include "common.h"

#include "ixion_dc1_monster.hpp"
#include "ixion_monica_faithful.hpp"
#include "ixion_room.hpp"
#include "ixion_room_logging.hpp"
#include "ixion_ledger.hpp"
#include "ixion_room_props.hpp"
#include "ixion_room_atmos.hpp"
#include "ixion_room_arena.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <random>
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "actionchara.hpp"
#include "automap.hpp"
#include "collision.hpp"
#include "colprim.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "gameutil.hpp"
#include "effscript.hpp"
#include "font.hpp"
#include "gamepad.hpp"
#include "gfx/gfx.hpp"
#include "mainloop.hpp"
#include "monster.hpp"
#include "mglib_port.hpp"
#include "padcontrol.hpp"
#include "platform/paths.hpp"
#include "savedatadungeon.hpp"
#include "scene.hpp"
#include "ixion_toan_combat.hpp"

extern CFont Font;

namespace {

using Json = nlohmann::json;
constexpr std::uintmax_t kJsonLimit = 64u * 1024u * 1024u;
constexpr std::uintmax_t kPngLimit = 32u * 1024u * 1024u;
constexpr size_t kMaxTextures = 128;
constexpr size_t kMaxVisuals = 100'000;
constexpr size_t kMaxVertices = 2'000'000;
constexpr size_t kMaxCollision = 1'000'000;

bool Fail(std::string &error, const char *message) {
    error = message;
    return false;
}

bool Number(const Json &value, float &out) {
    if (!value.is_number()) {
        return false;
    }
    const double number = value.get<double>();
    if (!std::isfinite(number) || std::abs(number) > 1.0e7) {
        return false;
    }
    out = static_cast<float>(number);
    return std::isfinite(out);
}

template <size_t N>
bool Vector(const Json &value, std::array<float, N> &out) {
    if (!value.is_array() || value.size() != N) {
        return false;
    }
    for (size_t i = 0; i < N; ++i) {
        if (!Number(value[i], out[i])) {
            return false;
        }
    }
    return true;
}

bool RelativePath(const std::string &text, std::filesystem::path &out) {
    if (text.empty() || text.size() > 240 || text.find(':') != std::string::npos ||
        text.find('\\') != std::string::npos) {
        return false;
    }
    const std::filesystem::path path(text);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return false;
    }
    for (const auto &part : path) {
        if (part == ".." || part == ".") {
            return false;
        }
    }
    out = path;
    return true;
}

bool UnderRoot(const std::filesystem::path &root, const std::filesystem::path &file) {
    const auto relative = file.lexically_relative(root);
    if (relative.empty() || relative.is_absolute()) {
        return false;
    }
    for (const auto &part : relative) {
        if (part == "..") {
            return false;
        }
    }
    return true;
}

bool ReadBytes(const std::filesystem::path &path, std::uintmax_t limit, std::vector<uint8_t> &out) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0 || size > limit) {
        return false;
    }
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!stream || stream.peek() != std::char_traits<char>::eof()) {
        return false;
    }
    out = std::move(bytes);
    return true;
}

bool Parse(const Json &root, const std::filesystem::path &pack_root, IxionRoom &room, std::string &error) {
    if (!root.is_object() || root.value("format", std::string()) != "ixion-room" ||
        !root.contains("version") || !root["version"].is_number_integer() || root["version"] != 1) {
        return Fail(error, "unsupported room format or version");
    }

    for (const char *required : {"donor", "units", "spawn", "enemy_spawn", "exit", "bounds", "textures", "visuals", "collision"}) {
        if (!root.contains(required)) {
            return Fail(error, "room is missing a required member");
        }
    }

    const Json &donor = root.at("donor");
    const Json &units = root.at("units");
    const Json &bounds = root.at("bounds");
    if (!donor.is_object() || donor.value("game", std::string()) != "dc1" ||
        !donor.contains("room") || !donor["room"].is_string() ||
        !donor.contains("sources") || !donor["sources"].is_array() || donor["sources"].empty() ||
        !units.is_object() || !units.contains("scale") || !Number(units["scale"], room.unit_scale) ||
        room.unit_scale <= 0 || room.unit_scale > 1000 ||
        !Vector(root.at("spawn"), room.spawn) ||
        !Vector(root.at("enemy_spawn"), room.enemy_spawn) ||
        !Vector(root.at("exit"), room.exit) ||
        !bounds.is_object() || !bounds.contains("min") || !bounds.contains("max") ||
        !Vector(bounds.at("min"), room.bounds_min) || !Vector(bounds.at("max"), room.bounds_max)) {
        return Fail(error, "malformed donor, units, spawn, exit, or bounds fields");
    }
    room.donor_room = donor["room"].get<std::string>();
    if (room.donor_room.empty() || room.donor_room.size() > 64) {
        return Fail(error, "invalid donor room name");
    }
    for (int axis = 0; axis < 3; ++axis) {
        if (!(room.bounds_min[axis] < room.bounds_max[axis]) ||
            room.bounds_max[axis] - room.bounds_min[axis] > 200000.0f) {
            return Fail(error, "invalid room bounds");
        }
    }
    for (const auto *marker : {&room.spawn, &room.enemy_spawn, &room.exit}) {
        for (int axis = 0; axis < 3; ++axis) {
            if ((*marker)[axis] < room.bounds_min[axis] || (*marker)[axis] > room.bounds_max[axis]) {
                return Fail(error, "spawn or exit marker outside declared room bounds");
            }
        }
    }
    if (donor["sources"].size() > 4096) {
        return Fail(error, "too many donor source records");
    }
    for (const Json &source : donor["sources"]) {
        if (!source.is_object() || !source.contains("path") || !source["path"].is_string() ||
            !source.contains("sha256") || !source["sha256"].is_string()) {
            return Fail(error, "malformed donor source record");
        }
        std::filesystem::path ignored;
        const std::string hash = source["sha256"].get<std::string>();
        if (!RelativePath(source["path"].get<std::string>(), ignored) || hash.size() != 64 ||
            !std::all_of(hash.begin(), hash.end(), [](unsigned char c) { return std::isxdigit(c) != 0; })) {
            return Fail(error, "invalid donor source path or digest");
        }
    }

    const Json &textures = root.at("textures");
    const Json &visuals = root.at("visuals");
    const Json &collision = root.at("collision");
    if (!textures.is_array() || textures.size() > kMaxTextures || !visuals.is_array() || visuals.empty() ||
        visuals.size() > kMaxVisuals || !collision.is_array() || collision.empty() ||
        collision.size() > kMaxCollision) {
        return Fail(error, "room geometry or texture count outside supported limits");
    }

    std::error_code ec;
    const auto canonical_root = std::filesystem::weakly_canonical(pack_root, ec);
    if (ec) {
        return Fail(error, "cannot resolve room pack root");
    }
    room.textures.reserve(textures.size());
    for (const Json &texture : textures) {
        if (!texture.is_object() || !texture.contains("name") || !texture["name"].is_string() ||
            !texture.contains("file") || !texture["file"].is_string()) {
            return Fail(error, "malformed texture record");
        }
        IxionRoomTexture parsed;
        parsed.name = texture["name"].get<std::string>();
        if (parsed.name.empty() || parsed.name.size() > 64 ||
            !RelativePath(texture["file"].get<std::string>(), parsed.relative_file)) {
            return Fail(error, "invalid texture name or relative path");
        }
        const auto absolute = std::filesystem::weakly_canonical(pack_root / parsed.relative_file, ec);
        std::vector<uint8_t> png;
        if (ec || !UnderRoot(canonical_root, absolute) || !ReadBytes(absolute, kPngLimit, png) ||
            png.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return Fail(error, "texture is missing, outside the pack, oversized, or not a PNG");
        }
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc *rgba = stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &width, &height, &channels, 4);
        if (rgba == nullptr || width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            static_cast<uint64_t>(width) * static_cast<uint64_t>(height) > 64u * 1024u * 1024u) {
            stbi_image_free(rgba);
            return Fail(error, "texture is not a complete supported PNG or exceeds the pixel limit");
        }
        parsed.width = static_cast<uint32_t>(width);
        parsed.height = static_cast<uint32_t>(height);
        parsed.rgba.assign(rgba, rgba + static_cast<size_t>(width) * height * 4);
        stbi_image_free(rgba);
        room.textures.push_back(std::move(parsed));
    }

    size_t vertex_total = 0;
    room.visuals.reserve(visuals.size());
    for (const Json &visual : visuals) {
        if (!visual.is_object() || !visual.contains("material") || !visual["material"].is_number_integer() ||
            !visual.contains("vertices") || !visual["vertices"].is_array()) {
            return Fail(error, "malformed visual record");
        }
        const int material = visual["material"].get<int>();
        const Json &vertices = visual["vertices"];
        if (vertices.empty() || vertices.size() % 3 != 0 ||
            material < -1 || material >= static_cast<int>(room.textures.size()) ||
            vertices.size() > kMaxVertices - vertex_total) {
            return Fail(error, "invalid visual material or triangle vertex count");
        }
        IxionRoomVisual parsed;
        parsed.material = material;
        parsed.vertices.reserve(vertices.size());
        for (const Json &vertex : vertices) {
            IxionRoomVertex v;
            if (!vertex.is_object() || !Vector(vertex.value("position", Json()), v.position) ||
                !Vector(vertex.value("normal", Json()), v.normal) || !Vector(vertex.value("uv", Json()), v.uv) ||
                !Vector(vertex.value("color", Json()), v.color)) {
                return Fail(error, "malformed visual vertex");
            }
            for (int axis = 0; axis < 3; ++axis) {
                if (v.position[axis] < room.bounds_min[axis] - 0.01f ||
                    v.position[axis] > room.bounds_max[axis] + 0.01f) {
                    return Fail(error, "visual vertex outside declared room bounds");
                }
            }
            if (std::any_of(v.color.begin(), v.color.end(), [](float c) { return c < 0 || c > 1; })) {
                return Fail(error, "visual color outside 0..1 range");
            }
            parsed.vertices.push_back(v);
        }
        vertex_total += vertices.size();
        room.visuals.push_back(std::move(parsed));
    }

    room.collision.reserve(collision.size());
    for (const Json &triangle : collision) {
        IxionRoomTriangle parsed;
        if (!triangle.is_array() || triangle.size() != 3 ||
            !Vector(triangle[0], parsed.vertices[0]) || !Vector(triangle[1], parsed.vertices[1]) ||
            !Vector(triangle[2], parsed.vertices[2])) {
            return Fail(error, "malformed collision triangle");
        }
        const float ax = parsed.vertices[1][0] - parsed.vertices[0][0];
        const float ay = parsed.vertices[1][1] - parsed.vertices[0][1];
        const float az = parsed.vertices[1][2] - parsed.vertices[0][2];
        const float bx = parsed.vertices[2][0] - parsed.vertices[0][0];
        const float by = parsed.vertices[2][1] - parsed.vertices[0][1];
        const float bz = parsed.vertices[2][2] - parsed.vertices[0][2];
        const float nx = ay * bz - az * by;
        const float ny = az * bx - ax * bz;
        const float nz = ax * by - ay * bx;
        if (!(nx * nx + ny * ny + nz * nz > 1.0e-12f)) {
            return Fail(error, "degenerate collision triangle");
        }
        for (const auto &point : parsed.vertices) {
            for (int axis = 0; axis < 3; ++axis) {
                if (point[axis] < room.bounds_min[axis] - 0.01f || point[axis] > room.bounds_max[axis] + 0.01f) {
                    return Fail(error, "collision vertex outside declared room bounds");
                }
            }
        }
        room.collision.push_back(parsed);
    }
    return true;
}

bool Overlaps(const IxionRoomTriangle &triangle, const float min[3], const float max[3]) {
    // Full triangle/AABB SAT removes the many broad-phase false positives that
    // starved the native 128-poly actor queries on the real donor room.
    float center[3], half[3];
    for (int axis = 0; axis < 3; ++axis) {
        center[axis] = (min[axis] + max[axis]) * 0.5f;
        half[axis] = (max[axis] - min[axis]) * 0.5f;
    }
    float v[3][3]{};
    for (int i = 0; i < 3; ++i) {
        for (int axis = 0; axis < 3; ++axis) v[i][axis] = triangle.vertices[i][axis] - center[axis];
    }
    const float edges[3][3] = {
        {v[1][0]-v[0][0], v[1][1]-v[0][1], v[1][2]-v[0][2]},
        {v[2][0]-v[1][0], v[2][1]-v[1][1], v[2][2]-v[1][2]},
        {v[0][0]-v[2][0], v[0][1]-v[2][1], v[0][2]-v[2][2]}};
    for (int axis = 0; axis < 3; ++axis) {
        const float low = std::min({v[0][axis], v[1][axis], v[2][axis]});
        const float high = std::max({v[0][axis], v[1][axis], v[2][axis]});
        if (low > half[axis] || high < -half[axis]) return false;
    }
    auto separates = [&](float ax, float ay, float az) {
        const float length2 = ax*ax + ay*ay + az*az;
        if (length2 < 1.0e-12f) return false;
        const float p0 = v[0][0]*ax + v[0][1]*ay + v[0][2]*az;
        const float p1 = v[1][0]*ax + v[1][1]*ay + v[1][2]*az;
        const float p2 = v[2][0]*ax + v[2][1]*ay + v[2][2]*az;
        const float radius = half[0]*std::abs(ax) + half[1]*std::abs(ay) + half[2]*std::abs(az);
        return std::min({p0,p1,p2}) > radius || std::max({p0,p1,p2}) < -radius;
    };
    for (const auto &edge : edges) {
        if (separates(0, edge[2], -edge[1]) || separates(-edge[2], 0, edge[0]) ||
            separates(edge[1], -edge[0], 0)) return false;
    }
    const float nx = edges[0][1]*edges[1][2] - edges[0][2]*edges[1][1];
    const float ny = edges[0][2]*edges[1][0] - edges[0][0]*edges[1][2];
    const float nz = edges[0][0]*edges[1][1] - edges[0][1]*edges[1][0];
    return !separates(nx, ny, nz);
}

struct RuntimeSelection {
    bool attempted = false;
    bool enabled = false;
    int dungeon = -1;
    int floor = -1;
    CScene *owner_scene = nullptr;
    CMap *owner_map = nullptr;
    bool scene_installed = false;
    bool scene_ready = false;
    bool spawn_applied = false;
    uint64_t scene_generation = 0;
    int native_ready_ticks = 0;
    int last_readiness_frame = -1;
    float readiness_position[3]{};
    bool player_spawn_pending = false;
    bool enemy_spawn_pending = false;
    bool enemy_spawn_boundary_set = false;
    int player_spawn_frame = -1;
    int enemy_spawn_frame = -1;
    CActiveMonster *encounter_enemy = nullptr;
    // Further DC1 roster donors, room-owned like the encounter enemy: never suspended, despawned
    // (no kill) on retirement. N11: each record names the incarnation spawned at install.
    struct RosterDonor {
        CActiveMonster *monster = nullptr;
        uint64_t incarnation = 0;
        int spawn_slot = -1; // R2: IxionRoomArrange spawn slot
        std::string key;     // R2: kill key (IxionRoomDonorKillKey)
    };
    std::vector<RosterDonor> roster_extra;
    // N11: the selected enemy's incarnation at install; its exemption and saved
    // native state apply to that incarnation only.
    uint64_t encounter_incarnation = 0;
    std::string encounter_key; // R2: kill key of a selected DC1 donor (spawn slot 0)
    // R2: floor lifetime (bumped by each native CMonsterMan::Initialize), its kill memory and the
    // DC1 arrangement planned once per lifetime so a reinstall reuses the same spawn slots.
    uint64_t floor_lifetime = 1;
    // R2: DC2 thinks and draws only the 5 nearest monsters (CMonsterMan priority_limit, monster.cpp:774,
    // CheckView :639-646); DC1 steps every monster on the floor (monstorunit.cpp:1909-1911). A DC1
    // roster lifts the limit while installed; -1 when nothing was saved.
    int saved_priority_limit = -1;
    int census_frame = -1;
    struct KillMemory {
        uint64_t lifetime = 0;
        int dungeon = -1;
        int floor = -1;
        std::vector<std::string> keys;
    } kills;
    struct ArrangePlan {
        uint64_t lifetime = 0;
        int dungeon = -1;
        int floor = -1;
        int count = 0;
        int kinds = 0;
        IxionRoomArrangeResult result;
    } plan;
    int enemy_saved_view_state = 0;
    int enemy_saved_target_no = -1;
    float enemy_saved_position[4]{};
    float enemy_saved_rotation[4]{};
    float enemy_saved_velocity[4]{};
    bool enemy_native_state_saved = false;
    int last_enemy_life = -1;
    float last_player_hp = -1.0f;
    // Player-facing mode: inspect installs at dungeon-loop entry with no
    // readiness wait; encounter keeps the stable-FIELD readiness gate.
    bool inspect = false;
    bool encounter = true;
    bool grid_refusal_logged = false;
    float player_saved_position[4]{};
    bool player_native_saved = false;
    // F3: native navigation/minimap writes are suspended while installed.
    bool navigation_suspended = false;
    int saved_navi_enable = 0;
    int saved_minimap_enable = 0;
    // F4: every non-selected native monster is suspended while installed.
    struct SuspendedMonster {
        CActiveMonster *monster = nullptr;
        uint32_t attrib = 0;
        float target_dist = 0.0f;
        int view_state = 0;
        float view_alpha = 1.0f;
        uint64_t incarnation = 0; // N11
    };
    std::vector<SuspendedMonster> suspended;
    // N9: boxes are never parked. An overlapping box refuses installation for
    // the current dungeon-loop entry (cleared before the next LoopInit).
    bool box_refusal_active = false;
    // N10: an unsupported native event retired the room; refused for the
    // current dungeon-loop entry (cleared before the next LoopInit).
    bool event_refusal_active = false;
    bool test_target_matches = false;
    const CActiveMonster *thinking = nullptr;
    bool exit_in_range = false;
    // N5: the Cross press that confirmed the exit is consumed until departure.
    bool exit_requested = false;
    IxionRoomRequestLog exit_log;
    // N2: set while RetireScene restores native actors; the placement
    // redirect and marker boundary never act on a restoration write.
    bool restoring = false;
    // N3: depth of the last native UpdateNaviMap before suspension.
    int saved_navi_depth = 4;
    bool player_placement_logged = false;
    IxionRoom room;
    std::vector<gfx::TextureHandle> gpu_textures;
    std::vector<gfx::MeshHandle> gpu_visuals;
};

// Set while the event-500 trigger spawns the deferred DC1 donor: it is the room's own encounter.
bool arena_adopting_donor = false;

RuntimeSelection &Selection() {
    static RuntimeSelection selection;
    return selection;
}

// N11: incarnation of each scene character address, bumped by every native
// spawn (CMonsterMan::SetActiveMonster) before the slot is overwritten.
uint64_t monster_spawn_serial = 0;
std::unordered_map<const CActiveMonster *, uint64_t> &MonsterIncarnations() {
    static std::unordered_map<const CActiveMonster *, uint64_t> incarnations;
    return incarnations;
}

uint64_t Incarnation(const CActiveMonster *monster) {
    const auto found = MonsterIncarnations().find(monster);
    return found == MonsterIncarnations().end() ? 0 : found->second;
}

bool IsSelectedIncarnation(const RuntimeSelection &selection, const CActiveMonster *monster) {
    return monster != nullptr && monster == selection.encounter_enemy &&
           Incarnation(monster) == selection.encounter_incarnation;
}

bool IsRosterIncarnation(const RuntimeSelection &selection, const CActiveMonster *monster) {
    if (monster == nullptr) return false;
    for (const auto &extra : selection.roster_extra)
        if (extra.monster == monster && extra.incarnation == Incarnation(monster)) return true;
    return false;
}

// N11: drop every ownership record that names an older incarnation of this
// scene character address: the selected-enemy exemption with its saved native
// position/rotation/view/target, and any parked record with its saved state.
void DropStaleOwnership(RuntimeSelection &selection, const CActiveMonster *monster, const char *when) {
    if (monster == nullptr) return;
    const uint64_t current = Incarnation(monster);
    if (monster == selection.encounter_enemy && selection.encounter_incarnation != current) {
        std::fprintf(stderr, "ixion room selected enemy slot reused by a new incarnation (%s); old exemption and saved native state dropped old=%llu new=%llu\n",
                     when, static_cast<unsigned long long>(selection.encounter_incarnation),
                     static_cast<unsigned long long>(current));
        selection.encounter_enemy = nullptr;
        selection.encounter_incarnation = 0;
        selection.enemy_native_state_saved = false;
        selection.enemy_spawn_pending = false;
        selection.enemy_spawn_boundary_set = true;
        if (selection.thinking == monster) selection.thinking = nullptr;
    }
    const size_t before = selection.suspended.size();
    std::erase_if(selection.suspended, [&](const auto &saved) {
        return saved.monster == monster && saved.incarnation != current;
    });
    if (selection.suspended.size() != before) {
        std::fprintf(stderr, "ixion room parked record for a reused slot dropped (%s); old restoration state discarded\n",
                     when);
    }
}

bool ParseFloor(const char *text, int &dungeon, int &floor) {
    if (text == nullptr) {
        return false;
    }
    const std::string_view value(text);
    const size_t separator = value.find(':');
    if (separator == std::string_view::npos || separator == 0 || separator + 1 == value.size()) {
        return false;
    }
    const auto [dungeon_end, dungeon_error] = std::from_chars(value.data(), value.data() + separator, dungeon);
    const auto [floor_end, floor_error] = std::from_chars(value.data() + separator + 1,
                                                          value.data() + value.size(), floor);
    return dungeon_error == std::errc() && floor_error == std::errc() && dungeon_end == value.data() + separator &&
           floor_end == value.data() + value.size() && dungeon >= 0 && dungeon < 7 && floor >= 0 && floor < 100;
}

void LoadRuntimeSelection() {
    RuntimeSelection &selection = Selection();
    if (selection.attempted) {
        return;
    }
    selection.attempted = true;
    const char *room_path = std::getenv("DC2_IXION_ROOM");
    const char *floor_text = std::getenv("DC2_IXION_ROOM_FLOOR");
    if (room_path == nullptr || *room_path == '\0') {
        return;
    }
    if (!ParseFloor(floor_text, selection.dungeon, selection.floor)) {
        std::fprintf(stderr, "ixion room disabled: set DC2_IXION_ROOM and DC2_IXION_ROOM_FLOOR=dungeon:floor\n");
        return;
    }
    std::string error;
    if (!IxionRoom::Load(PathsFromUtf8(room_path), selection.room, error)) {
        std::fprintf(stderr, "ixion room disabled: %s\n", error.c_str());
        return;
    }
    const char *mode_text = std::getenv("DC2_IXION_ROOM_MODE");
    const std::string_view mode = mode_text != nullptr ? std::string_view(mode_text) : std::string_view("encounter");
    if (mode != "inspect" && mode != "encounter") {
        std::fprintf(stderr, "ixion room disabled: DC2_IXION_ROOM_MODE must be inspect or encounter\n");
        return;
    }
    selection.inspect = mode == "inspect";
    selection.encounter = !selection.inspect;
    if (const char *encounter = std::getenv("DC2_IXION_ROOM_ENCOUNTER"); encounter != nullptr && *encounter != '\0') {
        const std::string_view value(encounter);
        if (value != "0" && value != "1") {
            std::fprintf(stderr, "ixion room disabled: DC2_IXION_ROOM_ENCOUNTER must be 0 or 1\n");
            return;
        }
        selection.encounter = value == "1";
    }
    selection.enabled = true;
    std::fprintf(stderr, "ixion room loaded: donor=%s visuals=%zu collision=%zu mode=%s encounter=%d target=%d:%d\n",
                 selection.room.donor_room.c_str(), selection.room.visuals.size(), selection.room.collision.size(),
                 selection.inspect ? "inspect" : "encounter", selection.encounter ? 1 : 0, selection.dungeon,
                 selection.floor);
    IxionRoomAtmosConfigure(PathsFromUtf8(room_path), selection.room);
    IxionRoomArenaConfigure(PathsFromUtf8(room_path), selection.room);
}

// F1: the complete live target identity. Every install, ownership and query
// decision requires the current dungeon loop floor to be the selected one.
bool CurrentTargetMatches(const RuntimeSelection &selection) {
    if (selection.test_target_matches) return true; // set only by IxionRoomTestAdoptInstalled
    IxionRoomFloorIdentity current;
    if (GetNowLoopNo() == LOOP_DUNGEON && DngSaveDataDungeon != nullptr) {
        current.known = true;
        current.status_dungeon = DngStatus.dungeon_no;
        current.stage = DngSaveDataDungeon->stage_id;
        current.floor = current.stage >= 0 && current.stage < SAVE_DUNGEON_NUM
                            ? DngSaveDataDungeon->floor_id[current.stage] : -1;
    }
    return IxionRoomTargetMatches(selection.dungeon, selection.floor, current);
}

void ReleaseGpu(RuntimeSelection &selection) {
    for (auto mesh : selection.gpu_visuals) if (mesh != gfx::kNullMesh) gfx::DestroyMesh(mesh);
    for (auto texture : selection.gpu_textures) if (texture != gfx::kNullTexture) gfx::DestroyTexture(texture);
    selection.gpu_visuals.clear();
    selection.gpu_textures.clear();
}

bool PrepareGpu(RuntimeSelection &selection) {
    if (!selection.gpu_visuals.empty() || !selection.gpu_textures.empty()) return true;
    std::vector<gfx::TextureHandle> textures;
    std::vector<gfx::MeshHandle> visuals;
    size_t allocations = 0;
    size_t injected_failure = std::numeric_limits<size_t>::max();
    if (const char *failure = std::getenv("DC2_IXION_GPU_FAIL_AFTER")) {
        size_t requested = 0;
        const auto length = std::strlen(failure);
        const auto [end, ec] = std::from_chars(failure, failure + length, requested);
        if (ec == std::errc() && end == failure + length) injected_failure = requested;
    }
    auto fail_now = [&] { return allocations++ == injected_failure; };
    auto make_texture = [&](size_t i) {
        if (fail_now()) return gfx::kNullTexture;
        const IxionRoomTexture &source=selection.room.textures[i];
        auto texture=gfx::CreateTexture({source.width,source.height,gfx::TextureFormat::Rgba8});
        if (texture==gfx::kNullTexture || !gfx::UpdateTextureNow(texture,0,0,0,source.width,source.height,source.rgba.data())) {
            if (texture!=gfx::kNullTexture) gfx::DestroyTexture(texture);
            return gfx::kNullTexture;
        }
        return texture;
    };
    auto make_mesh = [&](size_t i) {
        if (fail_now()) return gfx::kNullMesh;
        const IxionRoomVisual &source=selection.room.visuals[i];
        std::vector<gfx::Vertex3D> vertices(source.vertices.size());
        std::vector<uint32_t> indices(source.vertices.size());
        for (size_t v=0;v<source.vertices.size();++v) {
            const IxionRoomVertex &input=source.vertices[v];
            gfx::Vertex3D &output=vertices[v];
            std::copy(input.position.begin(),input.position.end(),output.position);
            std::copy(input.normal.begin(),input.normal.end(),output.normal);
            std::copy(input.uv.begin(),input.uv.end(),output.uv);
            for (int channel=0;channel<4;++channel) output.color[channel]=IxionRoomGsColor(input.color[channel]);
            indices[v]=static_cast<uint32_t>(v);
        }
        return gfx::CreateMesh(vertices,indices);
    };
    const bool ok=IxionRoomStageGpuBundle<gfx::TextureHandle,gfx::MeshHandle>(
        selection.room.textures.size(),selection.room.visuals.size(),make_texture,make_mesh,
        [](auto handle){return handle!=gfx::kNullTexture;},[](auto handle){return handle!=gfx::kNullMesh;},
        [](auto handle){gfx::DestroyTexture(handle);},[](auto handle){gfx::DestroyMesh(handle);},textures,visuals);
    if (!ok) {
        std::fprintf(stderr,"ixion room GPU prepare rolled back after %zu allocations\n",allocations);
        return false;
    }
    selection.gpu_textures=std::move(textures);
    selection.gpu_visuals=std::move(visuals);
    return true;
}

CActiveMonster *FirstLiveNativeEnemy() {
    if (ActiveMonster == nullptr || ActiveMonster->scene != DngMainScene) return nullptr;
    for (CActiveMonster *enemy : ActiveMonster->active) {
        if (enemy != nullptr && enemy->state == ACTIVE_MONSTER_LIVE && enemy->max_life > 0 && enemy->life > 0) return enemy;
    }
    return nullptr;
}

bool NativeGameplayReady(RuntimeSelection &selection) {
    if (DngMainScene==nullptr || DngStatus.mode!=DNG_STATUS_FIELD || DngMainScene->event_run ||
        MainChara__2==nullptr || FirstLiveNativeEnemy()==nullptr) {
        selection.native_ready_ticks=0;
        selection.last_readiness_frame=-1;
        return false;
    }
    const int frame=mgPortFrameCount();
    if (selection.last_readiness_frame==frame) return selection.native_ready_ticks>=180;
    float position[4]={}; MainChara__2->GetPosition(position);
    if (selection.native_ready_ticks==0) {
        std::copy_n(position,3,selection.readiness_position);
        selection.native_ready_ticks=1;
    } else {
        const float dx=position[0]-selection.readiness_position[0];
        const float dy=position[1]-selection.readiness_position[1];
        const float dz=position[2]-selection.readiness_position[2];
        if (dx*dx+dy*dy+dz*dz>30.0f*30.0f) {
            selection.native_ready_ticks=0;
            std::copy_n(position,3,selection.readiness_position);
        } else {
            ++selection.native_ready_ticks;
            std::copy_n(position,3,selection.readiness_position);
        }
    }
    selection.last_readiness_frame=frame;
    return selection.native_ready_ticks>=180;
}

int LiveNativeEnemyCount() {
    if (ActiveMonster==nullptr || ActiveMonster->scene!=DngMainScene) return 0;
    int count=0;
    for (CActiveMonster *enemy:ActiveMonster->active)
        if (enemy!=nullptr && enemy->state==ACTIVE_MONSTER_LIVE && enemy->life>0) ++count;
    return count;
}

// Encounter installs only in stable FIELD play. Inspect also installs while a
// native floor-entry event runs, so the native floor geometry is never drawn:
// the event (for example the 0:1 first-visit tutorial) plays inside the room.
bool InstallModeAllowed(const RuntimeSelection &selection) {
    return DngStatus.mode==DNG_STATUS_FIELD || (selection.inspect && DngStatus.mode==DNG_STATUS_EVENT);
}

bool MonsterStillOwned(const RuntimeSelection &selection, const CActiveMonster *monster, uint64_t incarnation) {
    if (monster == nullptr || selection.owner_scene == nullptr || DngMainScene != selection.owner_scene ||
        ActiveMonster == nullptr || ActiveMonster->scene != selection.owner_scene ||
        Incarnation(monster) != incarnation) return false;
    for (CActiveMonster *candidate : ActiveMonster->active) if (candidate == monster) return true;
    return false;
}

void ClearMonsterAttackWindows(CActiveMonster *monster) {
    // N1/N6/N7: the actor's attack windows, effect scripts by recorded source,
    // and exactly the primitives still held by those windows and scripts.
    IxionRoomEndOwnedAttacks(monster->chara_type, monster);
}

// F4: park one non-selected native monster. It keeps its slot, life state,
// kind and script, so nothing can reuse the slot and native floor counters
// (GetMonsterNum, all-defeated medals) still see it. It is not updated
// (ThinkHost override), not drawn (view OUT, alpha 0), not shadowed, gauged or
// minimapped (target distance beyond clip), not lockable or body-hit, cannot
// take damage (MONSTER_ATTRIB_NO_DAMAGE) and has no live attack primitive.
void SuspendMonster(RuntimeSelection &selection, CActiveMonster *monster, const char *when) {
    RuntimeSelection::SuspendedMonster saved;
    saved.monster = monster;
    saved.attrib = monster->attrib;
    saved.target_dist = monster->target_dist;
    saved.view_state = monster->view_state;
    saved.view_alpha = monster->view_alpha;
    saved.incarnation = Incarnation(monster);
    selection.suspended.push_back(saved);
    monster->view_state = MONSTER_VIEW_OUT;
    monster->view_alpha = 0.0f;
    monster->attrib |= MONSTER_ATTRIB_NO_LOCK_ON | MONSTER_ATTRIB_NO_BODY_HIT | MONSTER_ATTRIB_NO_DAMAGE;
    monster->target_dist = 9999999.0f;
    ClearMonsterAttackWindows(monster);
    if (MainChara__2 != nullptr && MainChara__2->lock_on && ActiveMonster != nullptr) {
        for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
            if (ActiveMonster->active[i] == monster && MainChara__2->target_no == i + MONSTER_ACTIVE_MAX) {
                MainChara__2->lock_on = 0;
                MainChara__2->target_no = -1;
            }
        }
    }
    std::fprintf(stderr, "ixion room suspended native monster id=%d state=%d hp=%d/%d (%s)\n",
                 monster->monster_id, monster->state, monster->life, monster->max_life, when);
}

void SuspendNativeMonsters(RuntimeSelection &selection) {
    if (ActiveMonster == nullptr || ActiveMonster->scene != DngMainScene) return;
    for (CActiveMonster *monster : ActiveMonster->active) {
        if (monster == nullptr || monster->state == ACTIVE_MONSTER_NONE || monster == selection.encounter_enemy ||
            IxionDc1IsDonor(monster)) continue;
        SuspendMonster(selection, monster, "install");
    }
}

void RestoreNativeMonsters(RuntimeSelection &selection) {
    int restored = 0;
    for (const auto &saved : selection.suspended) {
        if (!MonsterStillOwned(selection, saved.monster, saved.incarnation)) continue;
        saved.monster->attrib = saved.attrib;
        saved.monster->target_dist = saved.target_dist;
        saved.monster->view_state = static_cast<s16>(saved.view_state);
        saved.monster->view_alpha = saved.view_alpha;
        ++restored;
    }
    if (!selection.suspended.empty()) {
        std::fprintf(stderr, "ixion room restored suspended native monsters %d/%zu\n", restored,
                     selection.suspended.size());
    }
    selection.suspended.clear();
}

bool InsideDonorBounds(const RuntimeSelection &selection, const float position[4], float margin) {
    const auto &low = selection.room.bounds_min;
    const auto &high = selection.room.bounds_max;
    return position[0] >= low[0] - margin && position[0] <= high[0] + margin && position[2] >= low[2] - margin &&
           position[2] <= high[2] + margin && position[1] >= low[1] - 200.0f && position[1] <= high[1] + 200.0f;
}

// N9: native treasure boxes are never parked, so no native slot is ever
// advertised as free while it holds a reward. Every live box is listed at
// install; one whose pickup collision (40-unit radius) could reach the donor
// bounds refuses the installation for this dungeon-loop entry.
bool TreasureBoxesAllowInstall(RuntimeSelection &selection) {
    CTreasureBoxManager *boxes = DngMainScene != nullptr ? DngMainScene->battle_area.treasure_box : nullptr;
    if (boxes == nullptr) {
        std::fprintf(stderr, "ixion room treasure boxes: none on this floor\n");
        return true;
    }
    int live = 0;
    for (int i = 0; i < TREASURE_BOX_MAX; ++i) {
        CTreasureBox &box = boxes->box[i];
        if (box.state == TREASURE_BOX_STATE_NONE) continue;
        ++live;
        float position[4] = {};
        box.GetPosition(position);
        std::fprintf(stderr, "ixion room treasure box index=%d state=%d pos=(%.1f,%.1f,%.1f) inside_donor_bounds=%d\n", i,
                     box.state, position[0], position[1], position[2],
                     InsideDonorBounds(selection, position, 40.0f) ? 1 : 0);
    }
    const int overlapping = IxionRoomOverlappingTreasureBox(boxes, selection.room.bounds_min, selection.room.bounds_max);
    if (overlapping >= 0 && IxionRoomPropsParksNativeBoxes(selection.room)) {
        // RANDFLOOR-R2: a random floor pack asks for its overlapping native boxes to be parked for the visit.
        std::fprintf(stderr, "ixion room treasure boxes: live=%d, native box index=%d reaches the donor bounds; the pack parks them (props.json native_boxes)\n",
                     live, overlapping);
        return true;
    }
    if (overlapping >= 0) {
        selection.box_refusal_active = true;
        std::fprintf(stderr, "ixion room refused: native treasure box index=%d reaches the donor bounds; boxes are never parked; native floor remains active for this entry\n",
                     overlapping);
        return false;
    }
    std::fprintf(stderr, "ixion room treasure boxes: live=%d overlapping=0 parked=0\n", live);
    return true;
}

void SuspendNavigation(RuntimeSelection &selection) {
    selection.saved_navi_enable = AutoMapGen.navi_enable;
    selection.saved_minimap_enable = AutoMapGen.minimap_enable;
    selection.saved_navi_depth = AutoMapGen.navi_depth > 0 ? AutoMapGen.navi_depth : 4;
    AutoMapGen.navi_enable = 0;
    AutoMapGen.minimap_enable = 0;
    selection.navigation_suspended = true;
}

// N3: retained native callers (MinimapDoorEnable from door events) can change
// grid walls while UpdateNaviMap is suspended, and the native cell cache
// (file-static cax/cay in ps2/src/automap.cpp) would then skip the rebuild if
// play resumes in the cached cell. Force the native rebuild: one native
// UpdateNaviMap from a guaranteed-different in-grid cell moves the cache off,
// then the real native position rebuilds distances over the current walls.
void RebuildNativeNavigation(const float native_position[4], int depth) {
    int x = 0;
    int z = 0;
    if (AutoMapGen.grid == nullptr || !AutoMapGen.navi_enable ||
        !IxionRoomNativeGridCell(native_position[0], native_position[2], AutoMapGen.grid_w, AutoMapGen.grid_h,
                                 AutoMapGen.cell_w, AutoMapGen.cell_d, x, z)) {
        std::fprintf(stderr, "ixion room navigation rebuild skipped (grid=%p navi_enable=%d)\n",
                     static_cast<void *>(AutoMapGen.grid), AutoMapGen.navi_enable);
        return;
    }
    int other_x = x;
    int other_z = z;
    if (AutoMapGen.grid_w > 1) {
        other_x = x > 0 ? x - 1 : x + 1;
    } else if (AutoMapGen.grid_h > 1) {
        other_z = z > 0 ? z - 1 : z + 1;
    } else {
        std::fprintf(stderr, "ixion room navigation rebuild skipped (single-cell grid)\n");
        return;
    }
    // The cell centre maps back to exactly this cell through the native expression.
    float other[4] = {static_cast<float>(other_x) * AutoMapGen.cell_w, native_position[1],
                      static_cast<float>(other_z) * AutoMapGen.cell_d, 1.0f};
    float position[4] = {native_position[0], native_position[1], native_position[2], 1.0f};
    AutoMapGen.UpdateNaviMap(other, depth);
    AutoMapGen.UpdateNaviMap(position, depth);
    std::fprintf(stderr, "ixion room navigation rebuilt cell=(%d,%d) via=(%d,%d) depth=%d\n", x, z, other_x, other_z,
                 depth);
}

void RestoreNavigation(RuntimeSelection &selection, bool same_scene) {
    if (!selection.navigation_suspended) return;
    selection.navigation_suspended = false;
    if (!same_scene) {
        // The owning scene is gone: never write old flags into a new floor.
        std::fprintf(stderr, "ixion room navigation flags not restored: owner scene changed\n");
        return;
    }
    AutoMapGen.navi_enable = selection.saved_navi_enable;
    AutoMapGen.minimap_enable = static_cast<s16>(selection.saved_minimap_enable);
    if (selection.player_native_saved) {
        RebuildNativeNavigation(selection.player_saved_position, selection.saved_navi_depth);
    }
}

// N4: a stale installation found after the arena may have been reset. The
// saved actor pointers and navigation flags belong to a scene that no longer
// exists, so nothing is dereferenced or restored; only the adapter-owned GPU
// handles are released and every pointer is dropped.
void DiscardStale(RuntimeSelection &selection, const char *reason) {
    const size_t textures = selection.gpu_textures.size();
    const size_t meshes = selection.gpu_visuals.size();
    const size_t monsters = selection.suspended.size();
    selection.suspended.clear();
    IxionMonicaFaithfulRetire(reason); // Monica's goo ball belongs to the room too
    IxionRoomPropsDiscard();
    IxionRoomAtmosDiscard();
    IxionRoomClearAttackClaims();
    selection.scene_installed = false;
    selection.scene_ready = false;
    selection.spawn_applied = false;
    selection.owner_scene = nullptr;
    selection.owner_map = nullptr;
    IxionDc1Forget(selection.encounter_enemy);
    selection.encounter_enemy = nullptr;
    for (const auto &extra : selection.roster_extra) IxionDc1Forget(extra.monster); // pointer identity only
    selection.roster_extra.clear();
    selection.encounter_incarnation = 0;
    selection.encounter_key.clear();
    selection.saved_priority_limit = -1; // the owning manager was reset; nothing is written back
    selection.thinking = nullptr;
    selection.exit_in_range = false;
    selection.exit_requested = false;
    selection.exit_log.Observe(false);
    selection.restoring = false;
    selection.navigation_suspended = false;
    selection.player_spawn_pending = false;
    selection.enemy_spawn_pending = false;
    selection.enemy_spawn_boundary_set = false;
    selection.enemy_native_state_saved = false;
    selection.player_native_saved = false;
    selection.player_placement_logged = false;
    IxionRoomArenaDiscard();
    ReleaseGpu(selection);
    std::fprintf(stderr, "ixion room INVARIANT: stale installation discarded generation=%llu reason=%s old_actors_dereferenced=0 dropped_monsters=%zu navigation_flags_untouched=1 released_textures=%zu released_meshes=%zu live_textures=%zu live_meshes=%zu\n",
                 static_cast<unsigned long long>(selection.scene_generation), reason, monsters, textures, meshes,
                 selection.gpu_textures.size(), selection.gpu_visuals.size());
}

// F2: the single retirement transaction. Restores every native owner the
// installation borrowed (selected enemy, suspended monsters, navigation and,
// when the floor continues, the player's native position), then releases the
// GPU bundle and clears all scene/map/actor pointers. Idempotent.
void RetireScene(RuntimeSelection &selection, const char *reason, bool departing) {
    if (!selection.scene_installed) {
        ReleaseGpu(selection);
        return;
    }
    // Ungaga R1: DC1 shots flying in the room end with it.
    IxionToanCombatRoomRetired(reason);
    const size_t textures = selection.gpu_textures.size();
    const size_t meshes = selection.gpu_visuals.size();
    // N2: revoke placement ownership before any native actor is restored, so
    // the room's own pending-marker redirect cannot intercept a restoration.
    selection.restoring = true;
    selection.exit_log.Observe(false);
    selection.player_spawn_pending = false;
    selection.enemy_spawn_pending = false;
    selection.exit_requested = false;
    const bool same_scene = selection.owner_scene != nullptr && DngMainScene == selection.owner_scene;
    IxionMonicaFaithfulRetire(reason); // Monica's goo ball belongs to the room too
    if (IxionDc1IsDonor(selection.encounter_enemy) && MonsterStillOwned(selection, selection.encounter_enemy, selection.encounter_incarnation)) {
        // Ixion: the DC1 monster belongs to the room, not the native floor; it leaves with the room
        // (no kill) and the next installation spawns it afresh.
        IxionDc1Despawn(selection.encounter_enemy, reason);
        selection.enemy_native_state_saved = false;
    }
    for (const auto &extra : selection.roster_extra) {
        if (MonsterStillOwned(selection, extra.monster, extra.incarnation) && IxionDc1IsDonor(extra.monster))
            IxionDc1Despawn(extra.monster, reason);
        else IxionDc1Forget(extra.monster);
    }
    selection.roster_extra.clear();
    if (selection.saved_priority_limit >= 0) {
        if (same_scene && ActiveMonster != nullptr && ActiveMonster->scene == selection.owner_scene)
            ActiveMonster->priority_limit = selection.saved_priority_limit;
        selection.saved_priority_limit = -1;
    }
    if (selection.enemy_native_state_saved && MonsterStillOwned(selection, selection.encounter_enemy, selection.encounter_incarnation)) {
        if (selection.encounter_enemy->state==ACTIVE_MONSTER_LIVE && selection.encounter_enemy->life>0) {
            selection.encounter_enemy->SetPosition(selection.enemy_saved_position);
            selection.encounter_enemy->SetRotation(selection.enemy_saved_rotation);
            std::copy_n(selection.enemy_saved_velocity,4,selection.encounter_enemy->velocity);
            selection.encounter_enemy->view_state=selection.enemy_saved_view_state;
            selection.encounter_enemy->target_no=selection.enemy_saved_target_no;
            float actual[4]={}; selection.encounter_enemy->GetPosition(actual);
            const bool matches=actual[0]==selection.enemy_saved_position[0] &&
                               actual[1]==selection.enemy_saved_position[1] && actual[2]==selection.enemy_saved_position[2];
            std::fprintf(stderr,"ixion room restored native enemy ownership id=%d hp=%d/%d actual=(%.3f,%.3f,%.3f) saved=(%.3f,%.3f,%.3f) position_restored=%d\n",
                         selection.encounter_enemy->monster_id,selection.encounter_enemy->life,selection.encounter_enemy->max_life,
                         actual[0],actual[1],actual[2],selection.enemy_saved_position[0],selection.enemy_saved_position[1],
                         selection.enemy_saved_position[2],matches?1:0);
        } else {
            std::fprintf(stderr,"ixion room native enemy ownership not restored because actor is no longer live\n");
        }
    }
    RestoreNativeMonsters(selection);
    IxionRoomPropsRetire(same_scene, departing);
    RestoreNavigation(selection, same_scene);
    IxionRoomAtmosRetire(departing);
    IxionRoomClearAttackClaims();
    IxionRoomArenaRetire(reason);
    const bool restore_player = !departing && selection.player_native_saved && MainChara__2 != nullptr &&
                                DngMainScene == selection.owner_scene && DngStatus.mode != DNG_STATUS_EXIT;
    selection.scene_installed=false;
    if (restore_player) {
        // The floor continues natively: return the player to the native
        // position so no imported coordinate reaches a native spatial consumer.
        MainChara__2->SetPosition(selection.player_saved_position);
        std::fill_n(MainChara__2->velocity, 3, 0.0f);
        std::fprintf(stderr, "ixion room placement actor=player reason=retire-restore pos=(%.3f,%.3f,%.3f)\n",
                     selection.player_saved_position[0], selection.player_saved_position[1],
                     selection.player_saved_position[2]);
    }
    selection.scene_ready=false;
    selection.spawn_applied=false;
    IxionDc1Forget(selection.encounter_enemy);
    selection.owner_scene=nullptr;
    selection.owner_map=nullptr;
    selection.encounter_enemy=nullptr;
    selection.encounter_incarnation=0;
    selection.encounter_key.clear();
    selection.thinking=nullptr;
    selection.exit_in_range=false;
    selection.last_enemy_life=-1;
    selection.last_player_hp=-1.0f;
    selection.native_ready_ticks=0;
    selection.last_readiness_frame=-1;
    selection.player_spawn_pending=false;
    selection.enemy_spawn_pending=false;
    selection.enemy_spawn_boundary_set=false;
    selection.player_spawn_frame=-1;
    selection.enemy_spawn_frame=-1;
    selection.enemy_native_state_saved=false;
    selection.player_native_saved=false;
    selection.player_placement_logged=false;
    selection.restoring=false;
    ReleaseGpu(selection);
    std::fprintf(stderr,"ixion room scene retired generation=%llu reason=%s departing=%d released_textures=%zu released_meshes=%zu live_textures=%zu live_meshes=%zu\n",
                 static_cast<unsigned long long>(selection.scene_generation), reason != nullptr ? reason : "unspecified",
                 departing ? 1 : 0, textures, meshes, selection.gpu_textures.size(), selection.gpu_visuals.size());
}

// R2 kill memory scope: a new floor lifetime, dungeon or floor forgets every remembered kill.
RuntimeSelection::KillMemory &CurrentKills(RuntimeSelection &selection) {
    auto &kills = selection.kills;
    if (kills.lifetime != selection.floor_lifetime || kills.dungeon != selection.dungeon || kills.floor != selection.floor) {
        if (!kills.keys.empty())
            std::fprintf(stderr, "ixion room kill memory cleared: lifetime %llu -> %llu (%zu kills forgotten; DC1 re-arranges on floor load)\n",
                         static_cast<unsigned long long>(kills.lifetime),
                         static_cast<unsigned long long>(selection.floor_lifetime), kills.keys.size());
        kills = {selection.floor_lifetime, selection.dungeon, selection.floor, {}};
    }
    return kills;
}

int FreeMonsterSlots() {
    if (ActiveMonster == nullptr) return 0;
    int free = 0;
    for (CActiveMonster *monster : ActiveMonster->active)
        if (monster == nullptr || monster->state == ACTIVE_MONSTER_NONE) ++free;
    return free;
}

// R2 slot budget. Floor 0:1 parks 18 natives in DC2's 24 slots. A DC1 roster that needs more slots
// releases parked-to-be natives (never a gate-key holder, never a donor): slot state NONE without a
// kill, attack windows and owned primitives ended, lock-on dropped. Released natives are not
// restored at retirement (their slots now hold DC1 monsters); the rest are parked as before.
int ReleaseNativesForRoster(int wanted) {
    if (wanted <= 0 || ActiveMonster == nullptr || ActiveMonster->scene != DngMainScene) return 0;
    int released = 0;
    for (int i = MONSTER_ACTIVE_MAX - 1; i >= 0 && released < wanted; --i) {
        CActiveMonster *monster = ActiveMonster->active[i];
        if (monster == nullptr || monster->state == ACTIVE_MONSTER_NONE || IxionDc1IsDonor(monster) ||
            monster->gate_key > 0)
            continue;
        ClearMonsterAttackWindows(monster);
        if (MainChara__2 != nullptr && MainChara__2->lock_on && MainChara__2->target_no == monster->chara_type) {
            MainChara__2->lock_on = 0;
            MainChara__2->target_no = -1;
        }
        std::fprintf(stderr, "ixion room released native monster id=%d slot=%d hp=%d/%d for the DC1 roster (not a kill; not restored)\n",
                     monster->monster_id, i, monster->life, monster->max_life);
        monster->state = ACTIVE_MONSTER_NONE;
        monster->chara_kind = ACTION_KIND_NONE;
        monster->view_alpha = 0.0f;
        monster->alpha = 0.0f;
        ++released;
    }
    return released;
}

int RosterCount(int kinds) {
    int count = kinds;
    if (const char *text = std::getenv("DC2_IXION_ROOM_DONOR_COUNT"); text != nullptr && *text != '\0') {
        int parsed = 0;
        const auto length = std::strlen(text);
        const auto [end, ec] = std::from_chars(text, text + length, parsed);
        if (ec == std::errc() && end == text + length && parsed >= 1 && parsed <= 15) count = parsed;
        else std::fprintf(stderr, "ixion room WARNING: DC2_IXION_ROOM_DONOR_COUNT=\"%s\" is not 1..15; using %d\n", text, kinds);
    }
    return count;
}

// Spawns this floor lifetime's DC1 arrangement, minus remembered kills. Returns spawn slot 0's donor.
CActiveMonster *SpawnDc1Roster(RuntimeSelection &selection) {
    const int kinds = IxionDc1RosterSize();
    const int count = RosterCount(kinds);
    auto &plan = selection.plan;
    if (plan.lifetime != selection.floor_lifetime || plan.dungeon != selection.dungeon || plan.floor != selection.floor ||
        plan.count != count || plan.kinds != kinds) {
        std::vector<bool> unique(static_cast<size_t>(kinds));
        for (int kind = 0; kind < kinds; ++kind) unique[kind] = IxionDc1RosterKindUnique(kind);
        const uint32_t seed = IxionRoomArrangeSeed(selection.room.donor_room, selection.dungeon, selection.floor,
                                                   selection.floor_lifetime);
        plan = {selection.floor_lifetime, selection.dungeon, selection.floor, count, kinds,
                IxionRoomArrange(selection.room, count, unique, seed)};
        const auto authored = IxionRoomPropsMonsterSpawns(selection.room);
        if (!authored.empty()) {
            plan.result.spawns.clear();
            int skipped_models = 0;
            for (const auto &spawn : authored) {
                int kind = -1;
                for (int k = 0; k < kinds; ++k) {
                    if (spawn.model == IxionDc1RosterId(k)) { kind = k; break; }
                }
                if (kind < 0) { ++skipped_models; continue; }
                plan.result.spawns.push_back({kind, spawn.position, false});
            }
            if (!plan.result.spawns.empty()) selection.room.enemy_spawn = plan.result.spawns.front().position;
            std::fprintf(stderr, "ixion room authored arrangement: %zu DC1 spawns in the pack, selected=%zu skipped_models=%d\n",
                         authored.size(), plan.result.spawns.size(), skipped_models);
        }
        const auto &result = plan.result;
        std::fprintf(stderr, "ixion room arrange lifetime=%llu seed=0x%08x count=%d kinds=%d walkable=%d reachable=%d candidates=%d placed=%zu strict=%d relaxed=%d dropped=%d (DC1 ArrangementPos)\n",
                     static_cast<unsigned long long>(selection.floor_lifetime), seed, count, kinds,
                     result.walkable_cells, result.reachable_cells, result.candidate_cells, result.spawns.size(),
                     result.strict, result.relaxed, result.dropped);
        for (size_t i = 0; i < result.spawns.size(); ++i) {
            const auto &spawn = result.spawns[i];
            std::fprintf(stderr, "ixion room arrange spawn=%zu kind=%d id=%s pos=(%.1f,%.1f,%.1f) pass=%s\n", i, spawn.kind,
                         IxionDc1RosterId(spawn.kind), spawn.position[0], spawn.position[1], spawn.position[2],
                         i == 0 ? "marker" : spawn.relaxed ? "relaxed" : "strict");
        }
    }
    auto &kills = CurrentKills(selection);
    std::vector<int> todo;
    int skipped = 0;
    for (size_t i = 0; i < plan.result.spawns.size(); ++i) {
        const auto &spawn = plan.result.spawns[i];
        const std::string key = IxionRoomDonorKillKey(selection.room.donor_room, selection.dungeon, selection.floor,
                                                      static_cast<int>(i), IxionDc1RosterId(spawn.kind));
        if (std::find(kills.keys.begin(), kills.keys.end(), key) != kills.keys.end()) {
            ++skipped;
            std::fprintf(stderr, "ixion room kill memory: %s killed earlier this floor lifetime=%llu; not respawned, no payout (DC1 state -1 until the next _LOAD_MONSTOR)\n",
                         key.c_str(), static_cast<unsigned long long>(selection.floor_lifetime));
            continue;
        }
        todo.push_back(static_cast<int>(i));
    }
    const int free_before = FreeMonsterSlots();
    const int released = static_cast<int>(todo.size()) > free_before
                             ? ReleaseNativesForRoster(static_cast<int>(todo.size()) - free_before) : 0;
    CActiveMonster *enemy = nullptr;
    int spawned = 0;
    for (const int slot : todo) {
        const auto &spawn = plan.result.spawns[slot];
        const float place[4] = {spawn.position[0], spawn.position[1], spawn.position[2], 1.0f};
        CActiveMonster *monster = IxionDc1SpawnDonorKind(spawn.kind, place);
        if (monster == nullptr) continue; // logged by the spawn
        ++spawned;
        const std::string key = IxionRoomDonorKillKey(selection.room.donor_room, selection.dungeon, selection.floor, slot,
                                                      IxionDc1RosterId(spawn.kind));
        if (slot == 0) {
            enemy = monster;
            selection.encounter_key = key;
        } else {
            monster->view_state = MONSTER_VIEW_INIT;
            monster->target_no = 0;
            selection.roster_extra.push_back({monster, Incarnation(monster), slot, key});
        }
        std::fprintf(stderr, "ixion room roster spawn=%d id=%s placed at (%.1f,%.1f,%.1f) slot=%d key=%s\n", slot,
                     IxionDc1RosterId(spawn.kind), place[0], place[1], place[2], monster->chara_type - MONSTER_ACTIVE_MAX,
                     key.c_str());
    }
    std::fprintf(stderr, "ixion room roster spawned %d of %zu planned (kinds=%d) killed_skipped=%d free_slots_before=%d released_natives=%d lifetime=%llu\n",
                 spawned, plan.result.spawns.size(), kinds, skipped, free_before, released,
                 static_cast<unsigned long long>(selection.floor_lifetime));
    return enemy;
}

bool InstallOnMap(RuntimeSelection &selection, CMap *map) {
    if (!selection.enabled || map==nullptr || DngMainScene==nullptr || !InstallModeAllowed(selection) ||
        DngMainScene->active_map!=0 || DngMainScene->GetMap(0)!=map || MainChara__2==nullptr ||
        !CurrentTargetMatches(selection)) return false;
    if (!selection.inspect && DngMainScene->event_run) return false;
    // F3: refuse rooms whose bounds do not index inside the live native grid.
    if (AutoMapGen.grid == nullptr ||
        !IxionRoomBoundsFitNativeGrid(selection.room.bounds_min, selection.room.bounds_max, AutoMapGen.grid_w,
                                      AutoMapGen.grid_h, AutoMapGen.cell_w, AutoMapGen.cell_d)) {
        if (!selection.grid_refusal_logged) {
            selection.grid_refusal_logged = true;
            std::fprintf(stderr, "ixion room refused: bounds x %.1f..%.1f z %.1f..%.1f do not fit native grid %dx%d cell %.1fx%.1f (grid=%p); native floor remains active\n",
                         selection.room.bounds_min[0], selection.room.bounds_max[0], selection.room.bounds_min[2],
                         selection.room.bounds_max[2], AutoMapGen.grid_w, AutoMapGen.grid_h, AutoMapGen.cell_w,
                         AutoMapGen.cell_d, static_cast<void *>(AutoMapGen.grid));
        }
        selection.enabled = false;
        return false;
    }
    if (!mgPortRendererReady()) return false;
    // N9: decided before any native state is borrowed.
    // N10: an unsupported native event retired this entry's installation.
    if (selection.event_refusal_active) return false;
    if (selection.box_refusal_active || !TreasureBoxesAllowInstall(selection)) return false;
    if (selection.encounter && !selection.inspect && !IxionDc1DonorRequested() && FirstLiveNativeEnemy() == nullptr) return false;
    if (!PrepareGpu(selection)) {
        selection.enabled=false;
        std::fprintf(stderr,"ixion room GPU prepare failed; native map remains active\n");
        return false;
    }
    CActiveMonster *enemy = nullptr;
    selection.roster_extra.clear();
    selection.encounter_key.clear();
    // Ixion arena: the DC1 donor spawns at the event-500 walk-in trigger, not at installation.
    const bool donor_deferred = selection.encounter && IxionDc1DonorRequested() && IxionRoomArenaDefersDonor();
    // Ixion: with a DC1 roster the room's monsters are DC1's own arrangement (SpawnDc1Roster); the
    // selected enemy is spawn slot 0's donor, never a native stand-in, and may be absent when that
    // spawn was killed earlier this floor lifetime.
    const bool dc1_roster = selection.encounter && IxionDc1DonorRequested();
    if (dc1_roster && !donor_deferred) {
        enemy = SpawnDc1Roster(selection);
        if (ActiveMonster != nullptr && selection.saved_priority_limit < 0) {
            selection.saved_priority_limit = ActiveMonster->priority_limit;
            if (ActiveMonster->priority_limit < MONSTER_ACTIVE_MAX) ActiveMonster->priority_limit = MONSTER_ACTIVE_MAX;
            std::fprintf(stderr, "ixion room priority limit %d -> %d while the DC1 roster is installed (DC1 steps every monster)\n",
                         selection.saved_priority_limit, ActiveMonster->priority_limit);
        }
    }
    if (selection.encounter && enemy == nullptr && !dc1_roster) enemy = FirstLiveNativeEnemy();
    if (selection.encounter && enemy == nullptr && !selection.inspect && !dc1_roster) return false;
    selection.owner_scene=DngMainScene;
    selection.owner_map=map;
    selection.encounter_enemy=enemy;
    selection.encounter_incarnation=Incarnation(enemy);
    selection.thinking=nullptr;
    MainChara__2->GetPosition(selection.player_saved_position);
    selection.player_native_saved=true;
    if (enemy != nullptr) {
        selection.enemy_saved_view_state=enemy->view_state;
        selection.enemy_saved_target_no=enemy->target_no;
        enemy->GetPosition(selection.enemy_saved_position);
        enemy->GetRotation(selection.enemy_saved_rotation);
        std::copy_n(enemy->velocity,4,selection.enemy_saved_velocity);
        selection.enemy_native_state_saved=true;
        enemy->view_state=MONSTER_VIEW_INIT;
        enemy->target_no=0;
    }
    SuspendNavigation(selection);
    SuspendNativeMonsters(selection);
    IxionRoomPropsParkNativeBoxes(selection.room);
    IxionRoomPropsInstall(selection.room, {}, selection.dungeon);
    IxionRoomArenaInstall();
    selection.exit_requested=false;
    selection.restoring=false;
    selection.player_placement_logged=selection.inspect;
    const int frame=mgPortFrameCount();
    selection.player_spawn_pending=true;
    selection.enemy_spawn_pending=enemy!=nullptr;
    selection.player_spawn_frame=frame;
    selection.enemy_spawn_frame=frame;
    selection.scene_installed=true;
    selection.spawn_applied=true;
    ++selection.scene_generation;
    std::fprintf(stderr,"ixion room scene installed generation=%llu mode=%s encounter=%d native_stable_ticks=%d pending_player=(%.3f,%.3f,%.3f) enemy=%d suspended_native=%zu navigation_suspended=1 gpu_textures=%zu gpu_meshes=%zu\n",
                 static_cast<unsigned long long>(selection.scene_generation),selection.inspect?"inspect":"encounter",
                 enemy!=nullptr?1:0,selection.native_ready_ticks,
                 selection.room.spawn[0],selection.room.spawn[1],selection.room.spawn[2],
                 enemy!=nullptr?enemy->monster_id:-1,selection.suspended.size(),
                 selection.gpu_textures.size(),selection.gpu_visuals.size());
    IxionRoomAtmosInstall(selection.owner_scene, selection.owner_map);
    if (selection.inspect) {
        // Direct controlled entry: commit the player at the authored spawn now,
        // before the first native floor frame is drawn.
        float spawn[4]={selection.room.spawn[0],selection.room.spawn[1],selection.room.spawn[2],1.0f};
        MainChara__2->SetPosition(spawn);
        std::fill_n(MainChara__2->velocity,3,0.0f);
        std::fprintf(stderr,"ixion room placement actor=player reason=inspect-entry frame=%d pos=(%.3f,%.3f,%.3f)\n",
                     frame,spawn[0],spawn[1],spawn[2]);
    }
    return true;
}

} // namespace

bool IxionRoom::Load(const std::filesystem::path &json_path, IxionRoom &out, std::string &error) {
    std::vector<uint8_t> bytes;
    if (!ReadBytes(json_path, kJsonLimit, bytes)) {
        return Fail(error, "cannot read room.json or file exceeds 64 MiB");
    }
    try {
        const Json document = Json::parse(bytes.begin(), bytes.end());
        IxionRoom candidate;
        if (!Parse(document, json_path.parent_path(), candidate, error)) {
            return false;
        }
        out = std::move(candidate);
        error.clear();
        return true;
    } catch (const std::exception &exception) {
        error = std::string("invalid room JSON: ") + exception.what();
        return false;
    }
}

int IxionRoomCollision(const IxionRoom &room, const float bounds_min[3], const float bounds_max[3],
                       CCPoly *polygons, int capacity) {
    static const std::vector<IxionRoomTriangle> no_extra;
    static const std::vector<uint16_t> no_tags;
    return IxionRoomCollision(room, bounds_min, bounds_max, polygons, capacity, no_extra, no_tags);
}

int IxionRoomCollision(const IxionRoom &room, const float bounds_min[3], const float bounds_max[3],
                       CCPoly *polygons, int capacity, const std::vector<IxionRoomTriangle> &extra,
                       const std::vector<uint16_t> &extra_tags) {
    if (bounds_min == nullptr || bounds_max == nullptr || polygons == nullptr || capacity <= 0) {
        return 0;
    }
    struct Candidate { const IxionRoomTriangle *triangle; bool supports_center; float distance; uint16_t tag; };
    std::vector<Candidate> candidates;
    const float cx = (bounds_min[0] + bounds_max[0]) * 0.5f;
    const float cy = (bounds_min[1] + bounds_max[1]) * 0.5f;
    const float cz = (bounds_min[2] + bounds_max[2]) * 0.5f;
    auto center_ground = [&](const IxionRoomTriangle &t, float &height) {
        const auto &a=t.vertices[0]; const auto &b=t.vertices[1]; const auto &c=t.vertices[2];
        const float den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
        if (std::abs(den)<1.0e-8f) return false;
        const float u=((b[2]-c[2])*(cx-c[0])+(c[0]-b[0])*(cz-c[2]))/den;
        const float v=((c[2]-a[2])*(cx-c[0])+(a[0]-c[0])*(cz-c[2]))/den;
        const float w=1.0f-u-v;
        if (u < -1e-5f || v < -1e-5f || w < -1e-5f) return false;
        const float nx=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
        const float ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
        const float nz=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
        if (ny <= std::sqrt(nx*nx+ny*ny+nz*nz)*0.65f) return false;
        height=u*a[1]+v*b[1]+w*c[1];
        return height >= bounds_min[1]-0.01f && height <= bounds_max[1]+0.01f;
    };
    auto consider = [&](const IxionRoomTriangle &triangle, uint16_t tag) {
        if (!Overlaps(triangle, bounds_min, bounds_max)) return;
        float ground_y = 0.0f;
        const bool support = center_ground(triangle, ground_y);
        float distance = 0.0f;
        for (int axis=0; axis<3; ++axis) {
            float low=triangle.vertices[0][axis], high=low;
            for (int i=1;i<3;++i) { low=std::min(low,triangle.vertices[i][axis]); high=std::max(high,triangle.vertices[i][axis]); }
            const float c = axis==0 ? cx : axis==1 ? cy : cz;
            const float d = c < low ? low-c : c > high ? c-high : 0.0f;
            distance += d*d;
        }
        candidates.push_back({&triangle,support,distance,tag});
    };
    for (const IxionRoomTriangle &triangle : room.collision) consider(triangle, 0);
    for (size_t i = 0; i < extra.size(); ++i) consider(extra[i], i < extra_tags.size() ? extra_tags[i] : 0);
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate &a,const Candidate &b) {
        if (a.supports_center != b.supports_center) return a.supports_center;
        return a.distance < b.distance;
    });
    if (candidates.size() > static_cast<size_t>(capacity)) candidates.resize(static_cast<size_t>(capacity));
    int count = 0;
    for (const Candidate &candidate : candidates) {
        const IxionRoomTriangle &triangle = *candidate.triangle;
        CCPoly &polygon = polygons[count++];
        std::memset(&polygon, 0, sizeof polygon);
        polygon.parts_no = candidate.tag;
        for (int corner = 0; corner < 3; ++corner) {
            for (int axis = 0; axis < 3; ++axis) {
                polygon.vertex[corner][axis] = triangle.vertices[corner][axis];
            }
            polygon.vertex[corner][3] = 1.0f;
        }
        const float ax = polygon.vertex[1][0] - polygon.vertex[0][0];
        const float ay = polygon.vertex[1][1] - polygon.vertex[0][1];
        const float az = polygon.vertex[1][2] - polygon.vertex[0][2];
        const float bx = polygon.vertex[2][0] - polygon.vertex[0][0];
        const float by = polygon.vertex[2][1] - polygon.vertex[0][1];
        const float bz = polygon.vertex[2][2] - polygon.vertex[0][2];
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;
        const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (!(length > 1.0e-6f)) {
            --count;
            continue;
        }
        polygon.normal[0] = nx / length;
        polygon.normal[1] = ny / length;
        polygon.normal[2] = nz / length;
        polygon.normal[3] = 0.0f;
        if (polygon.normal[1] >= 0.65f) {
            polygon.area_kind = COL_AREA_FLAT;
        }
    }
    return count;
}

bool IxionRoomRosterPoint(const IxionRoom &room, int index, int count, float out[3]) {
    if (index < 1 || count < 2 || index >= count || out == nullptr) return false;
    constexpr float kPi = 3.14159265358979f;
    const auto &marker = room.enemy_spawn;
    std::vector<CCPoly> polygons(512);
    for (const float radius : {60.0f, 45.0f}) {
        for (const float offset : {0.0f, 0.5f}) {
            const float angle = 2.0f * kPi * ((float) (index - 1) + offset) / (float) (count - 1);
            const float point[3] = {marker[0] + radius * std::cos(angle), marker[1], marker[2] + radius * std::sin(angle)};
            const float low[3] = {std::min(marker[0], point[0]) - 1.0f, marker[1] - 80.0f, std::min(marker[2], point[2]) - 1.0f};
            const float high[3] = {std::max(marker[0], point[0]) + 1.0f, marker[1] + 80.0f, std::max(marker[2], point[2]) + 1.0f};
            const int found = IxionRoomCollision(room, low, high, polygons.data(), (int) polygons.size());
            float hit[4] = {};
            float from[4] = {marker[0], marker[1] + 10.0f, marker[2], 1.0f};
            float to[4] = {point[0], point[1] + 10.0f, point[2], 1.0f};
            if (CheckHit(polygons.data(), found, from, to, hit, 0, 0) >= 0) continue; // wall between
            float above[4] = {point[0], point[1] + 30.0f, point[2], 1.0f};
            float below[4] = {point[0], point[1] - 60.0f, point[2], 1.0f};
            if (CheckHit(polygons.data(), found, above, below, hit, 1, 0) < 0) continue; // no floor
            out[0] = point[0];
            out[1] = hit[1];
            out[2] = point[2];
            return true;
        }
    }
    return false;
}

namespace {

// The imported room's walkable floor as a grid of 10-unit cells (about 6.4 DC1 units).
struct WalkGrid {
    static constexpr float kCell = 10.0f;
    static constexpr float kBucket = 40.0f;
    static constexpr float kStep = 12.0f; // largest height change between neighbouring cells
    static constexpr float kEye = 10.0f;  // wall test height above the floor
    float x0 = 0.0f, z0 = 0.0f;
    int w = 0, h = 0;
    int bw = 0, bh = 0;
    std::vector<std::vector<int>> buckets;
    std::vector<uint8_t> flat;
    std::vector<std::vector<float>> floors; // per cell: flat floor heights under the centre
    std::vector<float> height;             // per cell: the height reached by the flood fill
    std::vector<uint8_t> reachable;

    float CenterX(int x) const { return x0 + (x + 0.5f) * kCell; }
    float CenterZ(int z) const { return z0 + (z + 0.5f) * kCell; }
    int Bucket(float x, float z) const {
        const int bx = std::clamp(static_cast<int>((x - x0) / kBucket), 0, bw - 1);
        const int bz = std::clamp(static_cast<int>((z - z0) / kBucket), 0, bh - 1);
        return bz * bw + bx;
    }
};

bool SegmentHitsTriangle(const IxionRoomTriangle &t, const float p[3], const float q[3]) {
    // Moller-Trumbore on the segment p->q (t in [0, 1]).
    const float d[3] = {q[0] - p[0], q[1] - p[1], q[2] - p[2]};
    const auto &a = t.vertices[0], &b = t.vertices[1], &c = t.vertices[2];
    const float e1[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float e2[3] = {c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    const float s[3] = {d[1] * e2[2] - d[2] * e2[1], d[2] * e2[0] - d[0] * e2[2], d[0] * e2[1] - d[1] * e2[0]};
    const float det = e1[0] * s[0] + e1[1] * s[1] + e1[2] * s[2];
    if (std::abs(det) < 1.0e-8f) return false;
    const float inv = 1.0f / det;
    const float o[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    const float u = (o[0] * s[0] + o[1] * s[1] + o[2] * s[2]) * inv;
    if (u < 0.0f || u > 1.0f) return false;
    const float r[3] = {o[1] * e1[2] - o[2] * e1[1], o[2] * e1[0] - o[0] * e1[2], o[0] * e1[1] - o[1] * e1[0]};
    const float v = (d[0] * r[0] + d[1] * r[1] + d[2] * r[2]) * inv;
    if (v < 0.0f || u + v > 1.0f) return false;
    const float hit = (e2[0] * r[0] + e2[1] * r[1] + e2[2] * r[2]) * inv;
    return hit >= 0.0f && hit <= 1.0f;
}

WalkGrid BuildWalkGrid(const IxionRoom &room) {
    WalkGrid grid;
    grid.x0 = room.bounds_min[0];
    grid.z0 = room.bounds_min[2];
    grid.w = std::max(1, static_cast<int>(std::ceil((room.bounds_max[0] - grid.x0) / WalkGrid::kCell)));
    grid.h = std::max(1, static_cast<int>(std::ceil((room.bounds_max[2] - grid.z0) / WalkGrid::kCell)));
    grid.bw = std::max(1, static_cast<int>(std::ceil((room.bounds_max[0] - grid.x0) / WalkGrid::kBucket)));
    grid.bh = std::max(1, static_cast<int>(std::ceil((room.bounds_max[2] - grid.z0) / WalkGrid::kBucket)));
    grid.buckets.assign(static_cast<size_t>(grid.bw * grid.bh), {});
    grid.flat.assign(room.collision.size(), 0);
    for (size_t i = 0; i < room.collision.size(); ++i) {
        const auto &t = room.collision[i];
        const float e1[3] = {t.vertices[1][0] - t.vertices[0][0], t.vertices[1][1] - t.vertices[0][1], t.vertices[1][2] - t.vertices[0][2]};
        const float e2[3] = {t.vertices[2][0] - t.vertices[0][0], t.vertices[2][1] - t.vertices[0][1], t.vertices[2][2] - t.vertices[0][2]};
        const float nx = e1[1] * e2[2] - e1[2] * e2[1], ny = e1[2] * e2[0] - e1[0] * e2[2], nz = e1[0] * e2[1] - e1[1] * e2[0];
        // Same floor rule as IxionRoomCollision (upward normal y >= 0.65 of its length).
        grid.flat[i] = ny > std::sqrt(nx * nx + ny * ny + nz * nz) * 0.65f ? 1 : 0;
        float lo[2] = {t.vertices[0][0], t.vertices[0][2]}, hi[2] = {lo[0], lo[1]};
        for (int k = 1; k < 3; ++k) {
            lo[0] = std::min(lo[0], t.vertices[k][0]); hi[0] = std::max(hi[0], t.vertices[k][0]);
            lo[1] = std::min(lo[1], t.vertices[k][2]); hi[1] = std::max(hi[1], t.vertices[k][2]);
        }
        const int b0 = grid.Bucket(lo[0], lo[1]), b1 = grid.Bucket(hi[0], hi[1]);
        for (int bz = b0 / grid.bw; bz <= b1 / grid.bw; ++bz)
            for (int bx = b0 % grid.bw; bx <= b1 % grid.bw; ++bx) grid.buckets[bz * grid.bw + bx].push_back(static_cast<int>(i));
    }
    const size_t cells = static_cast<size_t>(grid.w * grid.h);
    grid.floors.assign(cells, {});
    grid.height.assign(cells, 0.0f);
    grid.reachable.assign(cells, 0);
    for (int z = 0; z < grid.h; ++z) {
        for (int x = 0; x < grid.w; ++x) {
            const float cx = grid.CenterX(x), cz = grid.CenterZ(z);
            for (const int index : grid.buckets[grid.Bucket(cx, cz)]) {
                if (!grid.flat[index]) continue;
                const auto &t = room.collision[index];
                const auto &a = t.vertices[0], &b = t.vertices[1], &c = t.vertices[2];
                const float den = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2]);
                if (std::abs(den) < 1.0e-8f) continue;
                const float u = ((b[2] - c[2]) * (cx - c[0]) + (c[0] - b[0]) * (cz - c[2])) / den;
                const float v = ((c[2] - a[2]) * (cx - c[0]) + (a[0] - c[0]) * (cz - c[2])) / den;
                if (u < -1e-5f || v < -1e-5f || 1.0f - u - v < -1e-5f) continue;
                grid.floors[z * grid.w + x].push_back(u * a[1] + v * b[1] + (1.0f - u - v) * c[1]);
            }
        }
    }
    return grid;
}

bool WallBetween(const IxionRoom &room, const WalkGrid &grid, const float p[3], const float q[3]) {
    int seen[2] = {grid.Bucket(p[0], p[2]), grid.Bucket(q[0], q[2])};
    for (int k = 0; k < 2; ++k) {
        if (k == 1 && seen[1] == seen[0]) break;
        for (const int index : grid.buckets[seen[k]])
            if (!grid.flat[index] && SegmentHitsTriangle(room.collision[index], p, q)) return true;
    }
    return false;
}

// Flood fill from the player spawn over cells joined by a small step and no wall.
void FloodFrom(const IxionRoom &room, WalkGrid &grid, int &walkable, int &reachable) {
    walkable = 0;
    for (const auto &floors : grid.floors) walkable += floors.empty() ? 0 : 1;
    reachable = 0;
    const int sx = std::clamp(static_cast<int>((room.spawn[0] - grid.x0) / WalkGrid::kCell), 0, grid.w - 1);
    const int sz = std::clamp(static_cast<int>((room.spawn[2] - grid.z0) / WalkGrid::kCell), 0, grid.h - 1);
    const auto &start = grid.floors[sz * grid.w + sx];
    if (start.empty()) return;
    float best = start[0];
    for (const float y : start) if (std::abs(y - room.spawn[1]) < std::abs(best - room.spawn[1])) best = y;
    std::vector<int> queue{sz * grid.w + sx};
    grid.reachable[queue[0]] = 1;
    grid.height[queue[0]] = best;
    for (size_t head = 0; head < queue.size(); ++head) {
        const int cell = queue[head];
        ++reachable;
        const int x = cell % grid.w, z = cell / grid.w;
        const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
            const int nx = x + dx[k], nz = z + dz[k];
            if (nx < 0 || nz < 0 || nx >= grid.w || nz >= grid.h) continue;
            const int next = nz * grid.w + nx;
            if (grid.reachable[next]) continue;
            const float here = grid.height[cell];
            float chosen = 0.0f;
            bool found = false;
            for (const float y : grid.floors[next]) {
                if (std::abs(y - here) <= WalkGrid::kStep && (!found || std::abs(y - here) < std::abs(chosen - here))) {
                    chosen = y;
                    found = true;
                }
            }
            if (!found) continue;
            const float p[3] = {grid.CenterX(x), here + WalkGrid::kEye, grid.CenterZ(z)};
            const float q[3] = {grid.CenterX(nx), chosen + WalkGrid::kEye, grid.CenterZ(nz)};
            if (WallBetween(room, grid, p, q)) continue;
            grid.reachable[next] = 1;
            grid.height[next] = chosen;
            queue.push_back(next);
        }
    }
}

float Distance3(const std::array<float, 3> &a, const std::array<float, 3> &b) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

uint32_t IxionRoomArrangeSeed(const std::string &room, int dungeon, int floor, uint64_t lifetime) {
    const std::string text = room + ":" + std::to_string(dungeon) + ":" + std::to_string(floor) + ":" + std::to_string(lifetime);
    uint32_t hash = 2166136261u;
    for (const unsigned char c : text) {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

IxionRoomArrangeResult IxionRoomArrange(const IxionRoom &room, int count, const std::vector<bool> &unique_kinds,
                                        uint32_t seed) {
    IxionRoomArrangeResult result;
    const int kinds = static_cast<int>(unique_kinds.size());
    if (count <= 0 || kinds <= 0 || room.collision.empty()) return result;
    WalkGrid grid = BuildWalkGrid(room);
    FloodFrom(room, grid, result.walkable_cells, result.reachable_cells);
    std::vector<int> candidates;
    for (int z = 1; z + 1 < grid.h; ++z) {
        for (int x = 1; x + 1 < grid.w; ++x) {
            bool open = true;
            for (int dz = -1; dz <= 1 && open; ++dz)
                for (int dx = -1; dx <= 1 && open; ++dx) open = grid.reachable[(z + dz) * grid.w + x + dx] != 0;
            if (open) candidates.push_back(z * grid.w + x);
        }
    }
    result.candidate_cells = static_cast<int>(candidates.size());
    const float k = room.unit_scale;
    std::mt19937 rng(seed);
    // DC1's (float) n * rand() / 2^31 roll, from a deterministic 31-bit stream.
    auto roll = [&](float n) { return n * static_cast<float>(rng() >> 1) / 2147483648.0f; };
    std::vector<bool> used(static_cast<size_t>(kinds), false);
    // Spawn 0: the authored enemy marker (DC1 places event monsters at explicit points, _SET_MONSTOR_ID).
    result.spawns.push_back({0, room.enemy_spawn, false});
    used[0] = true;
    for (int n = 1; n < count; ++n) {
        bool placed = false;
        for (int pass = 0; pass < 2 && !placed && !candidates.empty(); ++pass) {
            const bool strict = pass == 0;
            for (int attempt = 0; attempt < 65000 && !placed; ++attempt) {
                const int cell = candidates[std::min(static_cast<int>(roll(static_cast<float>(candidates.size()))),
                                                     static_cast<int>(candidates.size()) - 1)];
                const int cx = cell % grid.w, cz = cell / grid.w;
                const std::array<float, 3> point = {grid.x0 + (cx + roll(1.0f)) * WalkGrid::kCell, grid.height[cell],
                                                    grid.z0 + (cz + roll(1.0f)) * WalkGrid::kCell};
                // CheckTreasureBox / CheckAtra (20): the room's player spawn and exit stand in.
                if (Distance3(point, room.spawn) <= 20.0f * k || Distance3(point, room.exit) <= 20.0f * k) continue;
                bool close = false;
                int nearby = 0;
                for (const auto &other : result.spawns) {
                    const float distance = Distance3(point, other.position);
                    close = close || distance <= 25.0f * k;
                    nearby += distance <= 480.0f * k ? 1 : 0;
                }
                if (close || (strict && nearby >= 3)) continue;
                int kind = n < kinds ? n : std::min(static_cast<int>(roll(static_cast<float>(kinds))), kinds - 1);
                if (n >= kinds && unique_kinds[kind] && used[kind]) continue;
                used[kind] = true;
                result.spawns.push_back({kind, point, !strict});
                (strict ? result.strict : result.relaxed) += 1;
                placed = true;
            }
        }
        if (!placed) ++result.dropped; // DC1 gives up after 65000 tries (monstorunit.cpp:570-572)
    }
    return result;
}

std::string IxionRoomDonorKillKey(const std::string &room, int dungeon, int floor, int spawn_slot, const char *donor_id) {
    char slot[16];
    std::snprintf(slot, sizeof slot, "spawn%02d", spawn_slot);
    return room + ":" + std::to_string(dungeon) + ":" + std::to_string(floor) + ":" + slot + ":" +
           (donor_id != nullptr ? donor_id : "-");
}

uint8_t IxionRoomGsColor(float modulation) {
    if (!std::isfinite(modulation)) return 0;
    return static_cast<uint8_t>(std::clamp(std::lround(modulation * 128.0f), 0l, 128l));
}

std::array<uint8_t, 4> IxionRoomModulateTexel(std::array<uint8_t, 4> texel,
                                             std::array<float, 4> modulation) {
    std::array<uint8_t, 4> result{};
    for (size_t i=0;i<4;++i) {
        const float factor = static_cast<float>(IxionRoomGsColor(modulation[i])) / 128.0f;
        result[i] = static_cast<uint8_t>(std::clamp(std::lround(texel[i]*factor), 0l, 255l));
    }
    return result;
}

bool IxionRoomTargetMatches(int selected_dungeon, int selected_floor, const IxionRoomFloorIdentity &current) {
    return selected_dungeon >= 0 && selected_floor >= 0 && current.known &&
           current.status_dungeon == selected_dungeon && current.stage == selected_dungeon &&
           current.floor == selected_floor;
}

IxionRoomBoundaryDecision IxionRoomDecideBoundary(const IxionRoomBoundaryInput &input) {
    IxionRoomBoundaryDecision decision;
    decision.retire = input.installed && (input.owner_changed || !input.target_matches);
    const bool installed_after = input.installed && !decision.retire;
    decision.install = input.enabled && !installed_after && input.target_matches && !input.owner_changed &&
                       input.native_ready;
    return decision;
}

bool IxionRoomNativeGridCell(float x_position, float z_position, int grid_w, int grid_h, float cell_w, float cell_d,
                             int &x, int &z) {
    if (grid_w <= 0 || grid_h <= 0 || !(cell_w > 0.0f) || !(cell_d > 0.0f) || !std::isfinite(x_position) ||
        !std::isfinite(z_position)) {
        return false;
    }
    // Same float expression as CAutoMapGen::UpdateNaviMap/MinimapVisTest,
    // evaluated before any narrowing so an out-of-range value never reaches int.
    const float fx = (x_position + 0.5f * cell_w) / cell_w;
    const float fz = (z_position + 0.5f * cell_d) / cell_d;
    if (!(fx < static_cast<float>(grid_w)) || !(fz < static_cast<float>(grid_h))) return false;
    x = fx < 0.0f ? 0 : static_cast<int>(fx);
    z = fz < 0.0f ? 0 : static_cast<int>(fz);
    if (x < 0) x = 0;
    if (z < 0) z = 0;
    return x < grid_w && z < grid_h;
}

bool IxionRoomBoundsFitNativeGrid(const std::array<float, 3> &bounds_min, const std::array<float, 3> &bounds_max,
                                  int grid_w, int grid_h, float cell_w, float cell_d) {
    int x = 0;
    int z = 0;
    // Negative indices are clamped to zero natively; the maximum corner (plus
    // the loader's 0.01 containment slack) decides the largest native index.
    return IxionRoomNativeGridCell(bounds_min[0], bounds_min[2], grid_w, grid_h, cell_w, cell_d, x, z) &&
           IxionRoomNativeGridCell(bounds_max[0] + 0.01f, bounds_max[2] + 0.01f, grid_w, grid_h, cell_w, cell_d, x, z);
}

int IxionRoomAppendRuntimeCollision(CCPoly *polygons, int capacity, const float bounds_min[3],
                                    const float bounds_max[3], int dungeon, int floor) {
    LoadRuntimeSelection();
    RuntimeSelection &selection = Selection();
    if (!selection.scene_installed || !selection.enabled || dungeon != selection.dungeon || floor != selection.floor ||
        !CurrentTargetMatches(selection)) {
        return 0;
    }
    if (selection.owner_scene!=DngMainScene || selection.owner_map!=DngMainScene->GetMap(0)) return 0;
    const std::vector<IxionRoomTriangle> *arena = nullptr;
    const std::vector<uint16_t> *arena_tags = nullptr;
    IxionRoomArenaActiveCollision(arena, arena_tags);
    return IxionRoomCollision(selection.room, bounds_min, bounds_max, polygons, capacity, *arena, *arena_tags);
}

bool IxionRoomRuntimeActive(int dungeon, int floor) {
    LoadRuntimeSelection();
    const RuntimeSelection &selection = Selection();
    return selection.scene_installed && selection.enabled && dungeon == selection.dungeon && floor == selection.floor &&
           selection.owner_scene==DngMainScene && DngMainScene!=nullptr && selection.owner_map==DngMainScene->GetMap(0) &&
           CurrentTargetMatches(selection);
}

void IxionRoomObserveDungeonLoop() {
    // Query and draw hooks observe identity only. Retirement happens through
    // IxionRoomRetire at every departure and at the post-frame boundary.
}

void IxionRoomRetire(const char *reason) {
    RuntimeSelection &selection = Selection();
    if (!selection.scene_installed) return;
    RetireScene(selection, reason, true);
}

// Ixion arena per-frame actions that need the adapter's actors: the event-500 walk-in trigger
// (log, then the deferred DC1 donor at enemy_spawn) and the fall (DC1 gameloop.cpp:4262-4270:
// below y -30 donor costs 40 life and runs script 5, whose last placement is the respawn).
static void ArenaFrame(RuntimeSelection &selection) {
    if (!selection.scene_installed || !IxionRoomArenaInstalled() || DngMainScene != selection.owner_scene) return;
    const bool field = DngStatus.mode == DNG_STATUS_FIELD && DngMainScene != nullptr && !DngMainScene->event_run;
    float player[4] = {};
    const bool player_valid = MainChara__2 != nullptr && !selection.player_spawn_pending;
    if (player_valid) MainChara__2->GetPosition(player);
    const IxionRoomArenaFrameResult result = IxionRoomArenaFrame(mgPortFrameCount(), field, player_valid, player);
    if (result.event_fired) {
        std::fprintf(stderr, "ixion arena event %d: DC1 fades, closes boss_door/_a/_v and starts Dran's intro; here the donor (if configured) spawns now\n",
                     result.event_script);
        if (selection.encounter && IxionDc1DonorRequested() && selection.encounter_enemy == nullptr) {
            const float spawn[4] = {selection.room.enemy_spawn[0], selection.room.enemy_spawn[1], selection.room.enemy_spawn[2], 1.0f};
            // DRAN-R2 merge: the deferred spawn goes through the roster path, so spawn 0 stays on the
            // authored marker and the floor-lifetime kill memory applies (a killed Dran is skipped:
            // no respawn, no second payout), as at installation without an arena.
            arena_adopting_donor = true;
            selection.roster_extra.clear();
            selection.encounter_key.clear();
            CActiveMonster *enemy = SpawnDc1Roster(selection);
            arena_adopting_donor = false;
            if (enemy != nullptr && ActiveMonster != nullptr && selection.saved_priority_limit < 0) {
                selection.saved_priority_limit = ActiveMonster->priority_limit;
                if (ActiveMonster->priority_limit < MONSTER_ACTIVE_MAX) ActiveMonster->priority_limit = MONSTER_ACTIVE_MAX;
            }
            if (enemy != nullptr) {
                selection.encounter_enemy = enemy;
                selection.encounter_incarnation = Incarnation(enemy);
                selection.enemy_native_state_saved = false;
                selection.enemy_spawn_pending = false;
                selection.enemy_spawn_boundary_set = true;
                std::fprintf(stderr, "ixion arena event %d spawned donor id=%d hp=%d/%d at (%.3f,%.3f,%.3f)\n", result.event_script,
                             enemy->monster_id, enemy->life, enemy->max_life, spawn[0], spawn[1], spawn[2]);
            } else {
                std::fprintf(stderr, "ixion arena event %d: no donor spawned (killed earlier this floor lifetime, or spawn refused)\n", result.event_script);
            }
        }
    }
    if (result.fell && MainChara__2 != nullptr) {
        float respawn[3] = {};
        const bool traced = IxionRoomArenaFallRespawn(selection.room, respawn);
        const int character = DngUserData != nullptr ? DngUserData->GetActiveChrNo() : -1;
        COMMON_GAGE *gage = DngUserData != nullptr ? DngUserData->GetCharaHpGage(character) : nullptr;
        float before = -1.0f, after = -1.0f;
        int damage = 0;
        if (gage != nullptr) {
            // DC1's life loss through the DC1->DC2 balance scaling the donor's hits use, floored at
            // 1 HP: writing the gauge is not a DC2 death, so it must not reach zero here.
            damage = IxionDc1ScaledDamage(-IxionRoomArenaFallLife(), gage->max, IxionDc1BalanceHp());
            before = gage->now;
            gage->now = std::max(std::min(gage->now, 1.0f), gage->now - static_cast<float>(damage));
            after = gage->now;
        }
        float fell_from[4] = {};
        MainChara__2->GetPosition(fell_from);
        const float position[4] = {respawn[0], respawn[1], respawn[2], 1.0f};
        MainChara__2->SetPosition(const_cast<float *>(position));
        std::fill_n(MainChara__2->velocity, 3, 0.0f);
        std::fprintf(stderr, "ixion arena fall: player y=%.3f frame=%d dc1_life=%d scaled_damage=%d hp=%.1f->%.1f respawn=(%.3f,%.3f,%.3f) source=%s\n",
                     fell_from[1], mgPortFrameCount(), IxionRoomArenaFallLife(), damage, before, after, respawn[0],
                     respawn[1], respawn[2], traced ? "dc1-event5" : "room-spawn");
    }
}

void IxionRoomFrameBoundary() {
    LoadRuntimeSelection();
    RuntimeSelection &selection=Selection();
    if (selection.exit_log.Observe(selection.scene_installed && selection.exit_requested)) {
        // What the next native FIELD step will read from the logical action input.
        std::fprintf(stderr,"ixion room exit pending frame=%d action_confirm=%d action_held=%d raw_cross_down=%d\n",
                     mgPortFrameCount(),PadCtrl.Btn(PAD_BTN_ACTION_CONFIRM),PadCtrl.Btn(PAD_BTN_ACTION_HELD),
                     GamePad__2.Down(PAD_CROSS)?1:0);
    }
    // R2 test seam: DC2_IXION_ROOM_TEST_RETIRE_FRAME=<frame> retires an installed room at that frame
    // as a continuing-floor retirement (departing=0); the boundary below may reinstall at once. It
    // reproduces a retire/reinstall within one floor lifetime for the kill-memory replay.
    if (selection.scene_installed) {
        static const int forced = [] {
            const char *text = std::getenv("DC2_IXION_ROOM_TEST_RETIRE_FRAME");
            return text != nullptr && *text != '\0' ? std::atoi(text) : -1;
        }();
        static bool forced_done = false; // the boundary can run twice in one frame; retire once
        if (forced >= 0 && !forced_done && mgPortFrameCount() >= forced) {
            forced_done = true;
            RetireScene(selection, "test-forced-retire", false);
        }
    }
    // R2 roster census, every 300 frames while a DC1 roster is installed: which spawns are alive.
    if (selection.scene_installed && (!selection.encounter_key.empty() || !selection.roster_extra.empty()) &&
        mgPortFrameCount() / 300 != selection.census_frame) {
        selection.census_frame = mgPortFrameCount() / 300;
        int live = 0, thinking = 0;
        std::string list;
        auto add = [&](const CActiveMonster *monster, uint64_t incarnation, int spawn) {
            if (!MonsterStillOwned(selection, monster, incarnation)) return;
            const bool alive = monster->state == ACTIVE_MONSTER_LIVE && monster->life > 0;
            live += alive ? 1 : 0;
            thinking += alive && monster->view_state != MONSTER_VIEW_OUT ? 1 : 0;
            char entry[48];
            std::snprintf(entry, sizeof entry, " %d:%d/%d:v%d", spawn, monster->life, monster->max_life, monster->view_state);
            list += entry;
        };
        if (!selection.encounter_key.empty()) add(selection.encounter_enemy, selection.encounter_incarnation, 0);
        for (const auto &extra : selection.roster_extra) add(extra.monster, extra.incarnation, extra.spawn_slot);
        std::fprintf(stderr, "ixion room roster census frame=%d live=%d in_view=%d priority_limit=%d spawns(slot:hp:view)=%s\n",
                     mgPortFrameCount(), live, thinking, ActiveMonster != nullptr ? ActiveMonster->priority_limit : -1,
                     list.c_str());
    }
    const bool target_matches=CurrentTargetMatches(selection);
    const bool owner_changed=selection.scene_installed &&
        (DngMainScene==nullptr || DngMainScene!=selection.owner_scene ||
         DngMainScene->active_map!=0 || DngMainScene->GetMap(0)!=selection.owner_map ||
         DngStatus.mode==DNG_STATUS_EXIT);
    bool native_ready=false;
    if (selection.enabled && !selection.scene_installed && target_matches && GetNowLoopNo()==LOOP_DUNGEON &&
        InstallModeAllowed(selection) && DngMainScene!=nullptr && DngMainScene->active_map==0 &&
        DngMainScene->GetMap(0)!=nullptr) {
        // Inspect mode needs no stable-FIELD wait; it normally installs at
        // dungeon-loop entry and this is only the fallback.
        native_ready = selection.inspect || NativeGameplayReady(selection);
    }
    const IxionRoomBoundaryDecision decision=IxionRoomDecideBoundary(
        {selection.enabled,selection.scene_installed,owner_changed,target_matches,native_ready});
    if (decision.retire) {
        RetireScene(selection, owner_changed ? (DngStatus.mode==DNG_STATUS_EXIT ? "frame-boundary-native-exit"
                                                                               : "frame-boundary-owner-change")
                                             : "frame-boundary-target-floor-left",
                    DngStatus.mode==DNG_STATUS_EXIT || !target_matches);
    }
    if (decision.install) InstallOnMap(selection,DngMainScene->GetMap(0));
    ArenaFrame(selection);
}

void IxionRoomBeforeLoopInit() {
    RuntimeSelection &selection=Selection();
    // Every departure retires before LoopExit; anything still installed here
    // belongs to an arena LoopInit is about to reset and is only discarded.
    if (selection.scene_installed) DiscardStale(selection, "before-loop-init");
    IxionRoomPropsBeginFloor();
    // N9: a box refusal holds for one dungeon-loop entry; the next floor layout is checked afresh.
    selection.box_refusal_active = false;
    selection.event_refusal_active = false;
}

void IxionRoomLoopEntry() {
    LoadRuntimeSelection();
    RuntimeSelection &selection=Selection();
    // N4: LoopInit has already reset the arena; never restore into it.
    if (selection.scene_installed) DiscardStale(selection, "loop-entry-stale");
    if (!selection.enabled || !selection.inspect) return;
    const int loop = GetNowLoopNo();
    const int active_map = DngMainScene != nullptr ? DngMainScene->active_map : -1;
    static IxionRoomLoopLog log;
    const bool skip = loop != LOOP_DUNGEON || DngMainScene == nullptr || active_map != 0;
    const bool changed = log.Observe(skip, loop, DngMainScene, active_map);
    if (skip) {
        if (changed) {
            std::fprintf(stderr,"ixion room loop entry skipped: loop=%d scene=%p active_map=%d\n",loop,
                         static_cast<void *>(DngMainScene),active_map);
        }
        return;
    }
    const bool target_matches=CurrentTargetMatches(selection);
    std::fprintf(stderr,"ixion room loop entry: target_matches=%d dungeon=%d stage=%d floor=%d mode=%d event_run=%d\n",
                 target_matches?1:0,DngStatus.dungeon_no,DngSaveDataDungeon!=nullptr?DngSaveDataDungeon->stage_id:-1,
                 DngSaveDataDungeon!=nullptr && DngSaveDataDungeon->stage_id>=0 && DngSaveDataDungeon->stage_id<SAVE_DUNGEON_NUM
                     ? DngSaveDataDungeon->floor_id[DngSaveDataDungeon->stage_id] : -1,
                 DngStatus.mode,DngMainScene->event_run?1:0);
    if (!target_matches) return;
    InstallOnMap(selection,DngMainScene->GetMap(0));
}

void IxionRoomDrawOverlay() {
    RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || DngMainScene==nullptr || DngMainScene!=selection.owner_scene ||
        MainChara__2==nullptr || !CurrentTargetMatches(selection)) return;
    float position[4]={}; MainChara__2->GetPosition(position);
    const float dx=position[0]-selection.room.exit[0];
    const float dy=position[1]-selection.room.exit[1];
    const float dz=position[2]-selection.room.exit[2];
    const float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    const bool playable=DngStatus.mode==DNG_STATUS_FIELD && !DngMainScene->event_run;
    const bool in_range=playable && distance<=45.0f;
    if (in_range!=selection.exit_in_range) {
        selection.exit_in_range=in_range;
        std::fprintf(stderr,"ixion room exit prompt %s frame=%d distance=%.1f\n",in_range?"shown":"hidden",
                     mgPortFrameCount(),distance);
    }
    if (selection.exit_requested) {
        // Keep the confirming press away from native action handling until
        // the native exit (requested below) takes the floor down.
        IxionRoomConsumeExitPress();
        return;
    }
    if (!playable) return;
    char line[96];
    Font.SetColor(0x80,0x80,0x80,0x80);
    std::snprintf(line,sizeof line,"Ixion room: %s  exit %d",selection.room.donor_room.c_str(),
                  static_cast<int>(distance));
    Font.DrawDirect(line,24,150);
    if (!in_range) return;
    Font.SetColor(0x80,0x80,0x20,0x80);
    std::snprintf(line,sizeof line,"Cross: leave room");
    Font.DrawDirect(line,200,380);
    if (GamePad__2.Down(PAD_CROSS)) {
        DngMainScene->exit_flag=true;
        selection.exit_requested=true;
        // N5: this runs after PadCtrl.Update, so the next native FIELD step
        // would otherwise read the same press as an attack/confirm.
        IxionRoomConsumeExitPress();
        std::fprintf(stderr,"ixion room exit confirmed by Cross at frame=%d distance=%.1f; press consumed, requesting native dungeon exit\n",
                     mgPortFrameCount(),distance);
    }
}

void IxionRoomConsumeExitPress() {
    GamePad__2.previous_pad[0].button |= PAD_CROSS;
    for (auto &entry : PadCtrl.btn) {
        if (entry.config != 0 && (entry.config & 0xFFFF & PAD_CROSS) != 0) entry.value = 0;
    }
}

bool IxionRoomRuntimeOwnedBy(const void *scene,const void *map) {
    const RuntimeSelection &selection=Selection();
    return selection.scene_installed && selection.owner_scene==scene && selection.owner_map==map &&
           CurrentTargetMatches(selection);
}

bool IxionRoomMonsterSuspended(CActiveMonster *monster) {
    RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || monster==nullptr || IsSelectedIncarnation(selection, monster)) return false;
    // Room-owned DC1 roster donors run (they are not native floor monsters); N11: same incarnation only.
    if (IsRosterIncarnation(selection, monster)) return false;
    for (const auto &saved : selection.suspended)
        if (saved.monster==monster && saved.incarnation==Incarnation(monster)) return true;
    // N11: a record naming an older incarnation of this address never applies.
    DropStaleOwnership(selection, monster, "think");
    // A monster spawned natively after installation is parked on first sight.
    if (DngMainScene==selection.owner_scene && monster->state!=ACTIVE_MONSTER_NONE) {
        SuspendMonster(selection, monster, "spawned-while-installed");
        return true;
    }
    return false;
}

bool IxionRoomMonsterParked(const CActiveMonster *monster) {
    const RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || monster==nullptr || IsSelectedIncarnation(selection, monster)) return false;
    for (const auto &saved : selection.suspended)
        if (saved.monster==monster && saved.incarnation==Incarnation(monster)) return true;
    return false;
}

namespace {
// N6/N7: exact effect and primitive ownership. Every effect script is recorded
// at its single production creation point with its registration row (the
// creation group, which never changes) and its lifetime source: the creation
// group, or for a child created by _ES_CREATE the parent's source, so a
// _ESM_SET_USER_ID retag of the damage user_id cannot move it. Every primitive
// hand-out (CColPrimMan::GetPrim) clears that slot's holder; only
// _COLPRIM_CREATE and the ThinkHost attack windows record a new holder. A
// pointer to a primitive that was cleared and handed out again therefore no
// longer matches its holder and is never deleted through.
struct EffectOrigin {
    int row = -1;
    int source = -1;
    uint64_t serial = 0;
};
struct PrimHolder {
    enum Kind { kNone, kEffect, kMonster };
    Kind kind = kNone;
    const void *holder = nullptr;
    uint64_t serial = 0; // effect incarnation for kEffect
    int source = -1;     // recorded source for kEffect
};
uint64_t effect_serial = 0;
const _EFF_SCRIPT *effect_parent = nullptr;

std::unordered_map<const _EFF_SCRIPT *, EffectOrigin> &EffectOrigins() {
    static std::unordered_map<const _EFF_SCRIPT *, EffectOrigin> origins;
    return origins;
}

std::array<PrimHolder, COLPRIM_MAX> &PrimHolders() {
    static std::array<PrimHolder, COLPRIM_MAX> holders{};
    return holders;
}

int PrimIndex(const CColPrim *prim) {
    if (prim == nullptr || prim < ColPrimMan.prim || prim >= ColPrimMan.prim + COLPRIM_MAX) return -1;
    return static_cast<int>(prim - ColPrimMan.prim);
}

const EffectOrigin *OriginOf(const _EFF_SCRIPT *script) {
    const auto found = EffectOrigins().find(script);
    return found == EffectOrigins().end() ? nullptr : &found->second;
}
} // namespace

void IxionRoomNoteEffectCreated(const _EFF_SCRIPT *effect, int group) {
    if (!IxionRoomInstalled() || effect == nullptr) return;
    EffectOrigin origin;
    origin.row = group;
    origin.source = group;
    origin.serial = ++effect_serial;
    if (effect_parent != nullptr) {
        if (const EffectOrigin *parent = OriginOf(effect_parent)) origin.source = parent->source;
    }
    // A new incarnation at a reused address replaces the old record.
    EffectOrigins()[effect] = origin;
}

void IxionRoomSetEffectParent(const _EFF_SCRIPT *parent) {
    if (!IxionRoomInstalled()) return;
    effect_parent = parent;
}

void IxionRoomNotePrimHandedOut(const CColPrim *prim) {
    const int index = PrimIndex(prim);
    if (index >= 0) PrimHolders()[index] = PrimHolder{};
}

void IxionRoomNoteEffectPrim(const CColPrim *prim, const _EFF_SCRIPT *script) {
    if (!IxionRoomInstalled()) return;
    const int index = PrimIndex(prim);
    const EffectOrigin *origin = OriginOf(script);
    if (index < 0 || origin == nullptr) return;
    PrimHolders()[index] = PrimHolder{PrimHolder::kEffect, script, origin->serial, origin->source};
}

void IxionRoomNoteMonsterPrim(const CColPrim *prim, const CActiveMonster *monster) {
    if (!IxionRoomInstalled() || monster == nullptr) return;
    const int index = PrimIndex(prim);
    if (index >= 0) PrimHolders()[index] = PrimHolder{PrimHolder::kMonster, monster, Incarnation(monster), -1};
}

void IxionRoomForgetEffect(const _EFF_SCRIPT *effect) {
    EffectOrigins().erase(effect);
    if (effect_parent == effect) effect_parent = nullptr;
    for (auto &holder : PrimHolders()) {
        if (holder.kind == PrimHolder::kEffect && holder.holder == effect) holder = PrimHolder{};
    }
}

void IxionRoomBeforeEffectDelete(_EFF_SCRIPT *effect) {
    if (OriginOf(effect) != nullptr) {
        // Validate before Delete clears holders: equal owner IDs do not prove
        // that a reset/reused raw pointer still names this script's borrower.
        if (effect->colprim != nullptr && !IxionRoomPrimHeldByEffect(effect->colprim, effect)) {
            effect->colprim = nullptr;
        }
        // Includes primitives displaced by a later _COLPRIM_CREATE. End them
        // while this script's incarnation is known, before freeing frame arenas.
        for (auto &prim : ColPrimMan.prim) {
            if (prim.active && IxionRoomPrimHeldByEffect(&prim, effect)) prim.Delete(-1);
        }
    }
    IxionRoomForgetEffect(effect);
}

void IxionRoomClearAttackClaims() {
    EffectOrigins().clear();
    PrimHolders().fill(PrimHolder{});
    effect_parent = nullptr;
}

int IxionRoomEffectSource(const _EFF_SCRIPT *script) {
    const EffectOrigin *origin = OriginOf(script);
    return origin != nullptr ? origin->source : -1;
}

int IxionRoomEffectRow(const _EFF_SCRIPT *script) {
    const EffectOrigin *origin = OriginOf(script);
    return origin != nullptr ? origin->row : -1;
}

size_t IxionRoomEffectOriginCountForTest() {
    return EffectOrigins().size();
}

bool IxionRoomPrimHeldByEffect(const CColPrim *prim, const _EFF_SCRIPT *script) {
    const int index = PrimIndex(prim);
    const EffectOrigin *origin = OriginOf(script);
    if (index < 0 || origin == nullptr) return false;
    const PrimHolder &holder = PrimHolders()[index];
    return holder.kind == PrimHolder::kEffect && holder.holder == script && holder.serial == origin->serial;
}

bool IxionRoomPrimHeldByMonster(const CColPrim *prim, const CActiveMonster *monster) {
    const int index = PrimIndex(prim);
    if (index < 0 || monster == nullptr) return false;
    const PrimHolder &holder = PrimHolders()[index];
    return holder.kind == PrimHolder::kMonster && holder.holder == monster && holder.serial == Incarnation(monster);
}

_EFF_SCRIPT *IxionRoomRunningEffect(CEffectScriptMan *manager, const RS_STACKDATA *stack) {
    if (manager == nullptr || stack == nullptr) return nullptr;
    for (_EFF_SCRIPT *script = manager->head; script != nullptr; script = script->next) {
        const CRunScript &run = script->run;
        const RS_STACKDATA *base = run.global != nullptr ? run.global : run.stack;
        if (base != nullptr && stack >= base && stack < run.stack_end) return script;
    }
    return nullptr;
}

void IxionRoomEndOwnedAttacks(int owner, const CActiveMonster *monster) {
    if (owner < 0) return;
    int windows = 0;
    int stale_windows = 0;
    if (monster != nullptr) {
        auto *actor = const_cast<CActiveMonster *>(monster);
        for (ACTION_DAMAGE &damage : actor->damage) {
            if (damage.use == 0 || damage.prim == nullptr) continue;
            if (IxionRoomPrimHeldByMonster(damage.prim, monster)) {
                damage.prim->Delete(-1);
                ++windows;
            } else {
                ++stale_windows;
            }
            damage.prim = nullptr;
        }
    }
    int scripts = 0;
    int retagged = 0;
    int unrecorded = 0;
    int stale_prims = 0;
    int orphans = 0;
    if (FxScriptMan != nullptr && owner < EFF_SPT_OWNER_MAX) {
        _EFF_SCRIPT *script = FxScriptMan->head;
        while (script != nullptr) {
            _EFF_SCRIPT *next = script->next;
            const EffectOrigin *origin = OriginOf(script);
            // Lifetime follows the recorded source; the mutable user_id is used
            // only when no record exists (an invariant breach, logged).
            const bool owned = origin != nullptr ? origin->source == owner : script->user_id == owner;
            if (!owned) {
                script = next;
                continue;
            }
            if (origin == nullptr) ++unrecorded;
            if (script->colprim != nullptr) {
                // Explicit other owners are supported; a reused slot is not touched.
                if (IxionRoomPrimHeldByEffect(script->colprim, script)) {
                    script->colprim->Delete(-1);
                    PrimHolders()[PrimIndex(script->colprim)] = PrimHolder{};
                } else {
                    ++stale_prims;
                }
                script->colprim = nullptr;
            }
            if (origin != nullptr && script->user_id != origin->row) ++retagged;
            // Native DeleteEffSpt clears slot[user_id][slot]. Point it at the
            // registration row, or at nothing when that row no longer holds this
            // script, so a retag can neither leave the original row dangling
            // nor clear an unrelated script's registration.
            if (script->slot >= 0) {
                const int row = origin != nullptr ? origin->row : script->user_id;
                if (row >= 0 && row < EFF_SPT_OWNER_MAX && script->slot < EFF_SPT_OWNER_SLOT_MAX &&
                    FxScriptMan->slot[row][script->slot] == script) {
                    script->user_id = row;
                } else {
                    script->slot = -1;
                }
            }
            // End explicit-other-owner orphans before DeleteEffSpt revokes this incarnation.
            for (auto &prim : ColPrimMan.prim) {
                if (prim.active && IxionRoomPrimHeldByEffect(&prim, script)) {
                    prim.Delete(-1);
                    ++orphans;
                }
            }
            FxScriptMan->DeleteEffSpt(script);
            EffectOrigins().erase(script);
            ++scripts;
            script = next;
        }
    }
    // Primitives still held by a script of this source (or by this actor's
    // windows) that no pointer carries any more, e.g. one replaced by a later
    // _COLPRIM_CREATE with an explicit other owner.
    for (int i = 0; i < COLPRIM_MAX; ++i) {
        PrimHolder &holder = PrimHolders()[i];
        CColPrim &prim = ColPrimMan.prim[i];
        if (!prim.active) continue;
        const auto *origin = holder.kind == PrimHolder::kEffect ? OriginOf(static_cast<const _EFF_SCRIPT *>(holder.holder)) : nullptr;
        const bool effect_orphan = origin != nullptr && origin->serial == holder.serial && holder.source == owner;
        const bool window_orphan = IxionRoomPrimHeldByMonster(&prim, monster);
        if (effect_orphan || window_orphan) {
            prim.Delete(-1);
            holder = PrimHolder{};
            ++orphans;
        }
    }
    if (windows != 0 || stale_windows != 0 || scripts != 0 || stale_prims != 0 || orphans != 0) {
        std::fprintf(stderr, "ixion room ended owned attacks owner=%d windows=%d effect_scripts=%d retagged=%d unrecorded=%d held_orphan_colprims=%d stale_pointers_untouched=%d\n",
                     owner, windows, scripts, retagged, unrecorded, orphans, stale_windows + stale_prims);
    }
}

// N8: CMonsterMan::Initialize reinitializes the same scene character addresses
// inside a running dungeon (debug enemy reload/reset, event _DEL_MONSTER and
// LoadMonsterFile). Retire first, while the saved pointers still name the old
// incarnation, so no saved state or selected-enemy exemption outlives it. A
// floor that continues re-evaluates installation at the next frame boundary.
void IxionRoomBeforeMonsterReset(const char *reason) {
    RuntimeSelection &selection = Selection();
    // R2: the native floor's monster population resets, so a new floor lifetime begins (DC1 re-runs
    // _LOAD_MONSTOR on every floor load); kill memory and the arrangement start afresh.
    ++selection.floor_lifetime;
    const char *room_opt_in = std::getenv("DC2_IXION_ROOM");
    if (room_opt_in && *room_opt_in)
        std::fprintf(stderr, "ixion room floor lifetime %llu begins (%s)\n",
                     static_cast<unsigned long long>(selection.floor_lifetime), reason != nullptr ? reason : "-");
    if (!selection.scene_installed) return;
    const bool same_scene = selection.owner_scene != nullptr && DngMainScene == selection.owner_scene;
    if (!same_scene) {
        DiscardStale(selection, reason);
        return;
    }
    RetireScene(selection, reason, false);
}

// N10: the room owns the live floor (same predicate as the field-event gate,
// without its one-time log).
static bool RoomOwnsLiveFloor(const RuntimeSelection &selection) {
    return selection.scene_installed && DngMainScene != nullptr && DngMainScene == selection.owner_scene &&
           CurrentTargetMatches(selection);
}

int IxionRoomTakeMonsterEvent(CMonsterMan *manager) {
    if (manager == nullptr) return -1;
    RuntimeSelection &selection = Selection();
    if (!RoomOwnsLiveFloor(selection)) return manager->IsRunEvent();
    // Retail CMonsterMan::IsRunEvent order, except that a parked monster's
    // request is left queued in its own event_no. Parked monsters run no
    // script, so nothing rewrites it; retirement restores the monster and the
    // native dispatcher then consumes the request in native coordinates.
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        CActiveMonster *monster = manager->active[i];
        if (monster == nullptr || monster->event_no == -1) continue;
        if (IxionRoomMonsterParked(monster)) {
            static const CActiveMonster *reported_monster = nullptr;
            static int reported_event = -1;
            if (reported_monster != monster || reported_event != monster->event_no) {
                reported_monster = monster;
                reported_event = monster->event_no;
                std::fprintf(stderr, "ixion room native event deferred: parked monster slot=%d id=%d event=%d stays queued until retirement\n",
                             i, monster->monster_id, monster->event_no);
            }
            continue;
        }
        const int event = monster->event_no;
        monster->event_no = -1;
        return event;
    }
    return -1;
}

// The retained native system scripts that may run while the room owns the
// floor. Only the player's death/continue script is qualified (T1-R2-QUAL
// gap 1: no relocation into native coordinates). Everything else, including
// requests from the selected enemy, scene event_run requests, guides, floor
// clear and treasure/geostone/map events, retires the room before it starts.
bool IxionRoomNativeEventSupported(int event_no) {
    return event_no == 1200; // ActionChara ACTION_DAMAGE_REQ_DEAD (actionchara.cpp:3301)
}

void IxionRoomBeforeNativeEvent(int event_no) {
    RuntimeSelection &selection = Selection();
    if (!RoomOwnsLiveFloor(selection)) return;
    const auto *chests = DngMainScene->battle_area.treasure_box;
    const bool own_chest = event_no == DNG_EVENT_TREASURE_BOX && chests != nullptr &&
                           IxionRoomPropsOwnsSlot(chests, chests->near_box);
    if (IxionRoomNativeEventSupported(event_no) || own_chest) {
        std::fprintf(stderr, "ixion room native event %d retained (supported) generation=%llu frame=%d\n", event_no,
                     static_cast<unsigned long long>(selection.scene_generation), mgPortFrameCount());
        return;
    }
    std::fprintf(stderr, "ixion room native event %d unsupported while installed; retiring before it runs generation=%llu frame=%d\n",
                 event_no, static_cast<unsigned long long>(selection.scene_generation), mgPortFrameCount());
    selection.event_refusal_active = true;
    RetireScene(selection, "unsupported-native-event", false);
}

bool IxionRoomEventRefusalActive() {
    return Selection().event_refusal_active;
}

void IxionRoomAfterTreasureBoxPosted(CTreasureBoxManager *boxes, int index) {
    RuntimeSelection &selection = Selection();
    if (boxes == nullptr || index < 0 || index >= TREASURE_BOX_MAX || !RoomOwnsLiveFloor(selection)) return;
    CTreasureBox &box = boxes->box[index];
    if (box.state == TREASURE_BOX_STATE_NONE) return;
    if (IxionRoomPropsOwnsSlot(boxes, index)) return;
    float position[4] = {};
    box.GetPosition(position);
    if (!InsideDonorBounds(selection, position, 40.0f)) {
        std::fprintf(stderr, "ixion room native treasure box posted index=%d pos=(%.1f,%.1f,%.1f) outside the donor bounds; retained\n",
                     index, position[0], position[1], position[2]);
        return;
    }
    std::fprintf(stderr, "ixion room native treasure box posted index=%d pos=(%.1f,%.1f,%.1f) reaches the donor bounds; retiring, boxes are never parked\n",
                 index, position[0], position[1], position[2]);
    selection.box_refusal_active = true;
    RetireScene(selection, "treasure-box-posted-inside-donor-bounds", false);
}

void IxionRoomBeforeMonsterSpawn(CActiveMonster *slot_actor) {
    if (slot_actor == nullptr) return;
    MonsterIncarnations()[slot_actor] = ++monster_spawn_serial;
    RuntimeSelection &selection = Selection();
    if (!selection.scene_installed) return;
    DropStaleOwnership(selection, slot_actor, "native-spawn");
}

void IxionRoomAfterMonsterSpawn(CActiveMonster *monster) {
    RuntimeSelection &selection = Selection();
    if (monster == nullptr || !selection.scene_installed || DngMainScene != selection.owner_scene ||
        ActiveMonster == nullptr || ActiveMonster->scene != selection.owner_scene ||
        monster->state == ACTIVE_MONSTER_NONE || arena_adopting_donor) return;
    // Bounded spawn policy: a native spawn while installed is parked at once,
    // with the new incarnation's own state saved for restoration.
    IxionRoomMonsterSuspended(monster);
}

uint64_t IxionRoomMonsterIncarnation(const CActiveMonster *monster) {
    return Incarnation(monster);
}

std::string IxionRoomDonorKillKeyFor(const CActiveMonster *monster) {
    const RuntimeSelection &selection = Selection();
    if (monster == nullptr || !selection.scene_installed) return {};
    if (IsSelectedIncarnation(selection, monster)) return selection.encounter_key;
    for (const auto &extra : selection.roster_extra)
        if (extra.monster == monster && extra.incarnation == Incarnation(monster)) return extra.key;
    return {};
}

bool IxionRoomCaptureLedgerKill(const CActiveMonster *monster, IxionLedgerKill &kill) {
    const RuntimeSelection &selection = Selection();
    if (!selection.scene_installed || monster == nullptr || monster->state != ACTIVE_MONSTER_DEAD || monster->life > 0 ||
        (!IsSelectedIncarnation(selection, monster) && !IsRosterIncarnation(selection, monster)) ||
        !MonsterStillOwned(selection, monster, Incarnation(monster))) return false;
    IxionLedgerKill captured;
    captured.room = selection.room.donor_room;
    captured.dungeon = selection.dungeon;
    captured.floor = selection.floor;
    captured.generation = selection.scene_generation;
    captured.monster_id = monster->monster_id;
    const std::string key = IxionRoomDonorKillKeyFor(monster);
    if (!key.empty()) {
        if (!IxionLedgerKillFromRosterKey(key, captured)) return false;
    } else if (IxionDc1IsDonor(monster)) {
        return false; // a donor without saved spawn attribution is not native
    }
    kill = captured;
    return true;
}

void IxionRoomRecordDonorKill(const CActiveMonster *monster, const char *donor_id) {
    RuntimeSelection &selection = Selection();
    const std::string key = IxionRoomDonorKillKeyFor(monster);
    if (key.empty()) {
        std::fprintf(stderr, "ixion room kill memory: donor=%s death not attributed to a room spawn (not remembered)\n",
                     donor_id != nullptr ? donor_id : "-");
        return;
    }
    auto &kills = CurrentKills(selection);
    if (std::find(kills.keys.begin(), kills.keys.end(), key) == kills.keys.end()) kills.keys.push_back(key);
    std::fprintf(stderr, "ixion room kill memory recorded key=%s lifetime=%llu remembered=%zu\n", key.c_str(),
                 static_cast<unsigned long long>(selection.floor_lifetime), kills.keys.size());
}

void IxionRoomDonorEvent(int port_event, int dc1_event, const CActiveMonster *monster) {
    RuntimeSelection &selection = Selection();
    const std::string key = IxionRoomDonorKillKeyFor(monster);
    if (dc1_event == 510) {
        // d01 prog510 (d01-event.stmts.txt:1502): EXT(711)(0, 100) _SSET_GAME_INT_FLAG, then cutscene
        // F1fcc0. The flag is the one prog100 tests before switching event 500 on.
        IxionRoomArenaSetBossFlag(100);
    }
    std::fprintf(stderr, "ixion room donor event port=%d (DC1 %d) frame=%d key=%s installed=%d generation=%llu: "
                         "DC2 event_no untouched, room stays installed; boss_flag=%d (DC1 cutscene F1fcc0 not played)\n",
                 port_event, dc1_event, mgPortFrameCount(), key.empty() ? "-" : key.c_str(), selection.scene_installed ? 1 : 0,
                 static_cast<unsigned long long>(selection.scene_generation), IxionRoomArenaBossFlag());
}

bool IxionRoomKillRemembered(const std::string &key) {
    RuntimeSelection &selection = Selection();
    const auto &kills = CurrentKills(selection);
    return std::find(kills.keys.begin(), kills.keys.end(), key) != kills.keys.end();
}

uint64_t IxionRoomFloorLifetime() {
    return Selection().floor_lifetime;
}

int IxionRoomOverlappingTreasureBox(CTreasureBoxManager *boxes, const std::array<float, 3> &bounds_min,
                                    const std::array<float, 3> &bounds_max) {
    if (boxes == nullptr) return -1;
    for (int i = 0; i < TREASURE_BOX_MAX; ++i) {
        CTreasureBox &box = boxes->box[i];
        if (box.state == TREASURE_BOX_STATE_NONE) continue;
        float position[4] = {};
        box.GetPosition(position);
        constexpr float kMargin = 40.0f; // native pickup collision gate, dng_event.cpp
        if (position[0] >= bounds_min[0] - kMargin && position[0] <= bounds_max[0] + kMargin &&
            position[2] >= bounds_min[2] - kMargin && position[2] <= bounds_max[2] + kMargin &&
            position[1] >= bounds_min[1] - 200.0f && position[1] <= bounds_max[1] + 200.0f) {
            return i;
        }
    }
    return -1;
}

void IxionRoomTestAdoptInstalled(const IxionRoomTestInstall &state) {
    RuntimeSelection &selection=Selection();
    if (!state.ledger_room.empty()) {
        selection.room.donor_room = state.ledger_room;
        selection.dungeon = state.ledger_dungeon;
        selection.floor = state.ledger_floor;
    }
    selection.attempted=true;
    selection.scene_installed=true;
    selection.owner_scene=static_cast<CScene *>(state.scene);
    selection.owner_map=static_cast<CMap *>(state.map);
    selection.encounter_enemy=state.enemy;
    selection.encounter_incarnation=Incarnation(state.enemy);
    selection.roster_extra.clear();
    for (size_t i = 0; i < state.roster.size(); ++i)
        selection.roster_extra.push_back({state.roster[i], Incarnation(state.roster[i]), static_cast<int>(i) + 1,
                                          i < state.roster_keys.size() ? state.roster_keys[i] : std::string()});
    selection.encounter_key = state.enemy_key;
    selection.enemy_native_state_saved=state.enemy!=nullptr;
    std::copy_n(state.enemy_saved_position,4,selection.enemy_saved_position);
    selection.enemy_saved_rotation[3]=1.0f;
    selection.enemy_spawn_pending=state.enemy_pending;
    selection.enemy_spawn_frame=-1000;
    selection.room.enemy_spawn={state.enemy_marker[0],state.enemy_marker[1],state.enemy_marker[2]};
    selection.suspended.clear();
    for (CActiveMonster *monster : state.parked)
        selection.suspended.push_back({monster,0u,0.0f,0,0.0f,Incarnation(monster)});
    selection.navigation_suspended=state.navigation_suspended;
    selection.saved_navi_enable=state.saved_navi_enable;
    selection.saved_minimap_enable=state.saved_minimap_enable;
    selection.saved_navi_depth=state.saved_navi_depth;
    selection.player_native_saved=state.player_native_saved;
    std::copy_n(state.player_saved_position,4,selection.player_saved_position);
    selection.restoring=false;
    selection.test_target_matches=state.target_matches;
    selection.room.bounds_min=state.bounds_min;
    selection.room.bounds_max=state.bounds_max;
    ++selection.scene_generation;
}

bool IxionRoomGatesNativeFieldEvents() {
    RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || DngMainScene==nullptr || DngMainScene!=selection.owner_scene ||
        !CurrentTargetMatches(selection)) return false;
    static uint64_t reported_generation=0;
    if (reported_generation!=selection.scene_generation) {
        reported_generation=selection.scene_generation;
        std::fprintf(stderr,"ixion room native field events gated generation=%llu frame=%d (random circle, map event points, guides, random stone, geostone, treasure box, monster talk)\n",
                     static_cast<unsigned long long>(selection.scene_generation),mgPortFrameCount());
    }
    return true;
}

bool IxionRoomInstalled() {
    return Selection().scene_installed;
}

void IxionRoomSetThinkingMonster(const CActiveMonster *monster) {
    Selection().thinking=monster;
}

bool IxionRoomOwnsNavigationQuery() {
    const RuntimeSelection &selection=Selection();
    const bool roster=IsRosterIncarnation(selection,selection.thinking);
    return selection.scene_installed && selection.encounter_enemy!=nullptr &&
           (selection.thinking==selection.encounter_enemy || roster) && DngMainScene==selection.owner_scene;
}

CActiveMonster *IxionRoomRuntimeEnemy() {
    return Selection().scene_installed ? Selection().encounter_enemy : nullptr;
}

int IxionRoomRuntimeEnemyCount() {
    return Selection().scene_installed ? LiveNativeEnemyCount() : 0;
}

bool IxionRoomRedirectPosition(CCharacter2 *actor,float position[4]) {
    RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || selection.restoring || actor==nullptr || position==nullptr) return false;
    bool *pending=nullptr;
    int *pending_frame=nullptr;
    const std::array<float,3> *marker=nullptr;
    const char *actor_name=nullptr;
    if (actor==MainChara__2 && selection.player_spawn_pending) {
        pending=&selection.player_spawn_pending;
        pending_frame=&selection.player_spawn_frame;
        marker=&selection.room.spawn;
        actor_name="player";
    } else if (actor==selection.encounter_enemy && selection.enemy_spawn_pending) {
        pending=&selection.enemy_spawn_pending;
        pending_frame=&selection.enemy_spawn_frame;
        marker=&selection.room.enemy_spawn;
        actor_name="enemy";
    }
    if (pending==nullptr) return false;
    const int frame=mgPortFrameCount();
    // Inspect entry holds the player at the authored spawn through any native
    // floor-entry event and commits only once ordinary FIELD play begins, so
    // the event's own placement never moves the player into native coordinates.
    const bool commit_allowed=!selection.inspect || actor!=MainChara__2 ||
        (DngStatus.mode==DNG_STATUS_FIELD && DngMainScene!=nullptr && !DngMainScene->event_run);
    if (commit_allowed && frame>*pending_frame) {
        float current[4]={}; actor->GetPosition(current);
        const float dx=current[0]-(*marker)[0],dy=current[1]-(*marker)[1],dz=current[2]-(*marker)[2];
        if (dx*dx+dy*dy+dz*dz<1.0f) {
            *pending=false;
            std::fprintf(stderr,"ixion room marker committed actor=%s frame=%d pos=(%.3f,%.3f,%.3f)\n",
                         actor_name,frame,current[0],current[1],current[2]);
            if (actor==MainChara__2 && selection.encounter_enemy==nullptr && !selection.scene_ready) {
                selection.scene_ready=true;
                std::fprintf(stderr,"ixion room player committed; scene ready generation=%llu frame=%d (no encounter)\n",
                             static_cast<unsigned long long>(selection.scene_generation),frame);
            }
            return false;
        }
        *pending_frame=frame;
    }
    if (actor==MainChara__2 && !selection.player_placement_logged) {
        // Encounter mode places the player through this pending redirect.
        selection.player_placement_logged=true;
        std::fprintf(stderr,"ixion room placement actor=player reason=encounter-entry frame=%d pos=(%.3f,%.3f,%.3f)\n",
                     frame,(*marker)[0],(*marker)[1],(*marker)[2]);
    }
    position[0]=(*marker)[0]; position[1]=(*marker)[1]; position[2]=(*marker)[2]; position[3]=1.0f;
    return true;
}

void IxionRoomActionPositionBoundary(CCharacter2 *actor) {
    RuntimeSelection &selection=Selection();
    if (!selection.scene_installed || selection.restoring || actor==nullptr) return;
    if (actor==selection.encounter_enemy && selection.enemy_spawn_pending) {
        float enemy_position[4]={}; actor->GetPosition(enemy_position);
        const float dx=enemy_position[0]-selection.room.enemy_spawn[0];
        const float dy=enemy_position[1]-selection.room.enemy_spawn[1];
        const float dz=enemy_position[2]-selection.room.enemy_spawn[2];
        if (dx*dx+dy*dy+dz*dz<=1.0f) selection.enemy_spawn_pending=false;
    }
    if (actor!=MainChara__2 || selection.enemy_spawn_boundary_set || selection.encounter_enemy==nullptr) return;
    // Inspect entry keeps holding the player through native entry events.
    if (selection.inspect && (DngStatus.mode!=DNG_STATUS_FIELD || DngMainScene==nullptr || DngMainScene->event_run)) return;
    float player[4]={}; actor->GetPosition(player);
    const float dx=player[0]-selection.room.spawn[0];
    const float dy=player[1]-selection.room.spawn[1];
    const float dz=player[2]-selection.room.spawn[2];
    if (dx*dx+dy*dy+dz*dz>5.0f*5.0f) return;
    selection.player_spawn_pending=false;
    // The native field entry already has a live movement vector when the
    // authored room marker is committed. Drop that stale velocity once so it
    // cannot carry the player across the marker into nearby imported geometry.
    std::fill_n(actor->velocity,3,0.0f);
    float enemy_spawn[4]={selection.room.enemy_spawn[0],selection.room.enemy_spawn[1],selection.room.enemy_spawn[2],1.0f};
    selection.encounter_enemy->SetPosition(enemy_spawn);
    selection.enemy_spawn_boundary_set=true;
    std::fprintf(stderr,"ixion room placement actor=enemy reason=encounter-marker frame=%d enemy=%d pos=(%.3f,%.3f,%.3f)\n",
        mgPortFrameCount(),selection.encounter_enemy->monster_id,enemy_spawn[0],enemy_spawn[1],enemy_spawn[2]);
    float committed_enemy[4]={}; selection.encounter_enemy->GetPosition(committed_enemy);
    const float edx=committed_enemy[0]-selection.room.enemy_spawn[0];
    const float edy=committed_enemy[1]-selection.room.enemy_spawn[1];
    const float edz=committed_enemy[2]-selection.room.enemy_spawn[2];
    if (edx*edx+edy*edy+edz*edz<=1.0f) selection.enemy_spawn_pending=false;
    if (!selection.player_spawn_pending && !selection.enemy_spawn_pending) {
        selection.scene_ready=true;
        std::fprintf(stderr,"ixion room actors committed; scene ready generation=%llu frame=%d\n",
                     static_cast<unsigned long long>(selection.scene_generation),mgPortFrameCount());
    }
}

int IxionRoomDrawRuntime(int dungeon, int floor) {
    LoadRuntimeSelection();
    RuntimeSelection &selection = Selection();
    if (!selection.scene_installed || !selection.enabled || dungeon != selection.dungeon || floor != selection.floor ||
        !CurrentTargetMatches(selection)) {
        return 0;
    }
    if (!PrepareGpu(selection)) return -1;

    gfx::MeshConstants constants{};
    mgPortWorldToClip(constants.mvp);
    constants.flags = gfx::kMeshVertexColor;
    std::fill_n(constants.diffuse, 4, 1.0f);
    gfx::DrawState state = mgPortDrawState();
    state.depth_test = gfx::DepthTest::GEqual;
    state.depth_write = true;
    state.cull = gfx::CullMode::None;
    IxionRoomAtmosDecorateDraw(constants, state);
    const auto transform = mgPortMeshTransform(gfx::IdentityMeshTransform().model);
    for (size_t i = 0; i < selection.room.visuals.size(); ++i) {
        const IxionRoomVisual &visual = selection.room.visuals[i];
        gfx::TextureBinding binding;
        if (visual.material >= 0) {
            binding.texture = selection.gpu_textures[visual.material];
        }
        binding.filter = gfx::Filter::Linear;
        binding.wrap_u = gfx::Wrap::Repeat;
        binding.wrap_v = gfx::Wrap::Repeat;
        mgPortDrawMesh(selection.gpu_visuals[i], 0, static_cast<uint32_t>(visual.vertices.size()),
                       constants, binding, state, &transform);
    }
    IxionRoomArenaDraw(selection.gpu_textures, constants, state);
    static uint64_t reported_generation = 0;
    if (reported_generation != selection.scene_generation) {
        reported_generation = selection.scene_generation;
        std::fprintf(stderr, "ixion room render submitted: generation=%llu frame=%d visuals=%zu textures=%zu\n",
                     static_cast<unsigned long long>(selection.scene_generation), mgPortFrameCount(),
                     selection.room.visuals.size(), selection.room.textures.size());
    }
    return static_cast<int>(selection.room.visuals.size());
}
