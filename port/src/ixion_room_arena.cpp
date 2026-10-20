#include "common.h"
#include "mw_runtime.h"

#include "ixion_room_arena.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <libvu0.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>
#include <string_view>

#include "collision.hpp"
#include "effscript.hpp"
#include "gameutil.hpp"
#include "map.hpp"
#include "mg_math.hpp"
#include "mglib_port.hpp"
#include "platform/paths.hpp"
#include "runscript.hpp"
#include "scene.hpp"
#include "scenesnd.hpp"

namespace {

using Json = nlohmann::json;
constexpr std::uintmax_t kSidecarLimit = 64u * 1024u * 1024u;
constexpr std::uintmax_t kPngLimit = 32u * 1024u * 1024u;
constexpr int kMaxSlots = 12;

bool Fail(std::string &error, const std::string &message) {
    error = message;
    return false;
}

bool ReadFileBytes(const std::filesystem::path &path, std::uintmax_t limit, std::vector<uint8_t> &out) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0 || size > limit) return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    out.resize(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(stream);
}

// The parse throws std::exception subclasses for missing members and wrong types; Load contains them.
struct SchemaError : std::exception {
    std::string message;
    explicit SchemaError(std::string text) : message(std::move(text)) {}
    const char *what() const noexcept override { return message.c_str(); }
};

float Num(const Json &value, const char *what) {
    if (!value.is_number()) throw SchemaError(std::string(what) + " is not a number");
    const double number = value.get<double>();
    if (!std::isfinite(number) || std::abs(number) > 1.0e7) throw SchemaError(std::string(what) + " is not finite");
    return static_cast<float>(number);
}

int Int(const Json &value, const char *what, int lo, int hi) {
    if (!value.is_number_integer()) throw SchemaError(std::string(what) + " is not an integer");
    const long long number = value.get<long long>();
    if (number < lo || number > hi) throw SchemaError(std::string(what) + " is out of range");
    return static_cast<int>(number);
}

template <size_t N>
std::array<float, N> Vec(const Json &value, const char *what) {
    if (!value.is_array() || value.size() != N) throw SchemaError(std::string(what) + " is not a " + std::to_string(N) + "-vector");
    std::array<float, N> out{};
    for (size_t i = 0; i < N; ++i) out[i] = Num(value[i], what);
    return out;
}

const Json &Arr(const Json &value, const char *what) {
    if (!value.is_array()) throw SchemaError(std::string(what) + " is not an array");
    return value;
}

std::string Str(const Json &value, const char *what) {
    if (!value.is_string()) throw SchemaError(std::string(what) + " is not a string");
    return value.get<std::string>();
}

IxionRoomTriangle Triangle(const Json &value) {
    if (!value.is_array() || value.size() != 3) throw SchemaError("triangle is not three corners");
    IxionRoomTriangle triangle;
    for (int i = 0; i < 3; ++i) triangle.vertices[i] = Vec<3>(value[i], "triangle corner");
    return triangle;
}

bool SafeRelative(const std::string &text, std::filesystem::path &out) {
    if (text.empty() || text.size() > 240 || text.find(':') != std::string::npos ||
        text.find('\\') != std::string::npos)
        return false;
    const std::filesystem::path path(text);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
    for (const auto &part : path)
        if (part == ".." || part == ".") return false;
    out = path;
    return true;
}

void LoadPng(const std::filesystem::path &dir, const std::string &relative, IxionRoomTexture &out) {
    std::filesystem::path rel;
    if (!SafeRelative(relative, rel)) throw SchemaError("texture path is unsafe");
    std::error_code ec;
    const auto root = std::filesystem::weakly_canonical(dir, ec);
    const auto file = std::filesystem::weakly_canonical(dir / rel, ec);
    const auto inside = file.lexically_relative(root);
    if (ec || inside.empty() || *inside.begin() == "..") throw SchemaError("texture is outside the pack");
    std::vector<uint8_t> png;
    if (!ReadFileBytes(file, kPngLimit, png) || png.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw SchemaError("texture is missing or oversized");
    int width = 0, height = 0, channels = 0;
    stbi_uc *rgba = stbi_load_from_memory(png.data(), static_cast<int>(png.size()), &width, &height, &channels, 4);
    if (rgba == nullptr || width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        if (rgba != nullptr) stbi_image_free(rgba);
        throw SchemaError("texture is not a supported PNG");
    }
    out.name = relative;
    out.relative_file = rel;
    out.width = static_cast<uint32_t>(width);
    out.height = static_cast<uint32_t>(height);
    out.rgba.assign(rgba, rgba + static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
    stbi_image_free(rgba);
}

void ParseArena(const Json &root, const std::filesystem::path &dir, size_t texture_count, IxionArena &out) {
    if (!root.is_object() || Str(root.at("format"), "format") != "ixion-room-arena" ||
        Int(root.at("version"), "version", 0, 1000) != 1)
        throw SchemaError("unsupported arena format or version");
    out.room = Str(root.at("room"), "room");
    out.room_json_sha256 = Str(root.at("room_json_sha256"), "room_json_sha256");
    if (out.room_json_sha256.size() != 64 ||
        !std::all_of(out.room_json_sha256.begin(), out.room_json_sha256.end(),
                     [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'); }))
        throw SchemaError("room_json_sha256 is not a sha256");
    out.scale = Num(root.at("units").at("scale"), "units.scale");
    if (!(out.scale > 0.0f)) throw SchemaError("units.scale is not positive");

    const Json &field = root.at("field_tiles");
    const Json &rule = field.at("rule");
    const int slots = Int(rule.at("slots"), "rule.slots", 1, kMaxSlots);
    out.initial_state = Int(rule.at("initial_state"), "rule.initial_state", 0, 3);
    out.hit_decrement_ge = Int(rule.at("hit").at("decrement_when_state_ge"), "rule.hit", 1, 3);
    out.fall_below_y = Num(rule.at("fall").at("below_y"), "rule.fall.below_y");
    out.fall_life = Int(rule.at("fall").at("life"), "rule.fall.life", -10000, 10000);
    out.fall_script = Int(rule.at("fall").at("script"), "rule.fall.script", -1, 100000);
    const Json &tiles = Arr(field.at("tiles"), "tiles");
    if (static_cast<int>(tiles.size()) != slots) throw SchemaError("tile count does not match rule.slots");
    out.tiles.clear();
    for (size_t index = 0; index < tiles.size(); ++index) {
        const Json &source = tiles[index];
        IxionArenaTile tile;
        tile.slot = Int(source.at("slot"), "tile.slot", 0, kMaxSlots - 1);
        if (tile.slot != static_cast<int>(index)) throw SchemaError("tiles are not in slot order");
        const Json &motion = source.at("motion");
        const auto frames = Vec<2>(motion.at("frames"), "motion.frames");
        tile.first_frame = frames[0];
        tile.speed = Num(motion.at("speed"), "motion.speed");
        tile.done_at = Num(motion.at("done_at"), "motion.done_at");
        if (!(tile.speed > 0.0f) || !(frames[1] > frames[0]) || tile.done_at < frames[0])
            throw SchemaError("motion frames, speed or done_at are invalid");
        const size_t keys = static_cast<size_t>(std::lround(frames[1] - frames[0])) + 1u;
        for (const Json &triangle : Arr(source.at("collision"), "tile.collision")) tile.collision.push_back(Triangle(triangle));
        for (const Json &object_json : Arr(source.at("objects"), "tile.objects")) {
            IxionArenaObject object;
            object.name = Str(object_json.at("name"), "object.name");
            object.rest_t = Vec<3>(object_json.at("rest").at("t"), "rest.t");
            object.rest_q = Vec<4>(object_json.at("rest").at("q"), "rest.q");
            const Json &track = object_json.at("track");
            for (const Json &t : Arr(track.at("t"), "track.t")) object.track_t.push_back(Vec<3>(t, "track.t"));
            for (const Json &q : Arr(track.at("q"), "track.q")) object.track_q.push_back(Vec<4>(q, "track.q"));
            if (object.track_t.size() != keys || object.track_q.size() != keys)
                throw SchemaError("track length does not match motion.frames");
            const Json &alpha = track.at("alpha");
            if (!alpha.is_object()) throw SchemaError("track.alpha is not an object");
            for (const auto &[material_text, values] : alpha.items()) {
                int material = 0;
                const auto [end, ec] = std::from_chars(material_text.data(), material_text.data() + material_text.size(), material);
                if (ec != std::errc() || end != material_text.data() + material_text.size() || material < 0)
                    throw SchemaError("track.alpha material is not an index");
                std::vector<float> curve;
                for (const Json &value : Arr(values, "track.alpha values")) curve.push_back(Num(value, "track.alpha"));
                if (curve.size() != keys) throw SchemaError("track.alpha length does not match motion.frames");
                object.track_alpha.emplace_back(material, std::move(curve));
            }
            for (const Json &group_json : Arr(object_json.at("groups"), "object.groups")) {
                IxionArenaGroup group;
                group.texture = Int(group_json.at("texture"), "group.texture", 0, 1 << 20);
                if (static_cast<size_t>(group.texture) >= texture_count) throw SchemaError("group texture is not a room.json texture");
                group.material = Int(group_json.at("mdt_material"), "group.mdt_material", 0, 1 << 20);
                for (const Json &vertex_json : Arr(group_json.at("vertices"), "group.vertices")) {
                    IxionRoomVertex vertex;
                    vertex.position = Vec<3>(vertex_json.at("position"), "vertex.position");
                    vertex.normal = Vec<3>(vertex_json.at("normal"), "vertex.normal");
                    vertex.uv = Vec<2>(vertex_json.at("uv"), "vertex.uv");
                    vertex.color = Vec<4>(vertex_json.at("color"), "vertex.color");
                    group.vertices.push_back(vertex);
                }
                if (group.vertices.empty() || group.vertices.size() % 3 != 0)
                    throw SchemaError("group vertices are not a triangle list");
                object.groups.push_back(std::move(group));
            }
            tile.objects.push_back(std::move(object));
        }
        out.tiles.push_back(std::move(tile));
    }

    const Json &fires = root.at("fires");
    out.fires.clear();
    for (const Json &emitter : Arr(fires.at("emitters"), "fires.emitters")) out.fires.push_back(Vec<3>(emitter.at("position"), "emitter.position"));
    const Json &draw = fires.at("draw");
    out.fire_offset_y = Num(draw.at("offset_y"), "fires.draw.offset_y");
    out.fire_layers.clear();
    for (const Json &layer_json : Arr(draw.at("layers"), "fires.draw.layers")) {
        IxionArenaFireLayer layer;
        const auto size = Vec<2>(layer_json.at("size"), "layer.size");
        layer.width = size[0];
        layer.height = size[1];
        for (const Json &name : Arr(layer_json.at("textures"), "layer.textures"))
            if (Str(name, "layer texture") == "flame") layer.flame = true;
        if (layer_json.contains("flicker")) {
            layer.flicker_every = Int(layer_json["flicker"].at("every_steps"), "flicker.every_steps", 1, 1000);
            layer.flicker_max = Num(layer_json["flicker"].at("scale_max"), "flicker.scale_max");
        }
        if (layer_json.contains("depth_bias_toward_camera"))
            layer.toward_camera = Num(layer_json["depth_bias_toward_camera"], "depth_bias_toward_camera");
        out.fire_layers.push_back(layer);
    }
    LoadPng(dir, Str(fires.at("textures").at("lightling"), "fires.textures.lightling"), out.glow);
    const Json &flipbook = fires.at("flipbook");
    LoadPng(dir, Str(flipbook.at("file"), "flipbook.file"), out.flipbook);
    const auto frame_size = Vec<2>(flipbook.at("frame_size"), "flipbook.frame_size");
    out.flipbook_frame_w = static_cast<int>(frame_size[0]);
    out.flipbook_frame_h = static_cast<int>(frame_size[1]);
    out.flipbook_columns = Int(flipbook.at("columns"), "flipbook.columns", 1, 64);
    out.flipbook_frames = Int(flipbook.at("frames"), "flipbook.frames", 1, 4096);
    const int rows = (out.flipbook_frames + out.flipbook_columns - 1) / out.flipbook_columns;
    if (out.flipbook_frame_w <= 0 || out.flipbook_frame_h <= 0 ||
        static_cast<uint32_t>(out.flipbook_frame_w * out.flipbook_columns) > out.flipbook.width ||
        static_cast<uint32_t>(out.flipbook_frame_h * rows) > out.flipbook.height)
        throw SchemaError("flipbook frames do not fit the image");

    out.camera.clear();
    for (const Json &part : Arr(root.at("camera_collision"), "camera_collision"))
        for (const Json &object : Arr(part.at("objects"), "camera objects"))
            for (const Json &triangle : Arr(object.at("triangles"), "camera triangles")) out.camera.push_back(Triangle(triangle));

    out.markers.clear();
    for (const Json &marker_json : Arr(root.at("markers"), "markers")) {
        IxionArenaMarker marker;
        marker.name = Str(marker_json.at("name"), "marker.name");
        marker.script = Int(marker_json.at("script"), "marker.script", -1, 100000);
        marker.switch_on = Int(marker_json.at("switch_on"), "marker.switch_on", 0, 1) != 0;
        marker.auto_start = Int(marker_json.at("auto_start"), "marker.auto_start", 0, 1) != 0;
        marker.position = Vec<3>(marker_json.at("position"), "marker.position");
        marker.horizontal_radius = Num(marker_json.at("trigger").at("horizontal_radius"), "trigger.horizontal_radius");
        marker.height_window = Num(marker_json.at("trigger").at("height_window"), "trigger.height_window");
        if (!(marker.horizontal_radius > 0.0f) || !(marker.height_window > 0.0f)) throw SchemaError("marker trigger is not positive");
        out.markers.push_back(std::move(marker));
    }
}

float Clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

void TrackIndex(size_t keys, float first_frame, float time, size_t &i0, size_t &i1, float &blend) {
    const float position = std::clamp(time - first_frame, 0.0f, static_cast<float>(keys - 1));
    i0 = static_cast<size_t>(std::floor(position));
    i1 = std::min(i0 + 1, keys - 1);
    blend = position - static_cast<float>(i0);
}

// Height of a non-steep triangle at (x, z), or false when (x, z) lies outside it.
bool HeightAt(const IxionRoomTriangle &t, float x, float z, float &height) {
    const auto &a = t.vertices[0];
    const auto &b = t.vertices[1];
    const auto &c = t.vertices[2];
    const float den = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2]);
    if (std::abs(den) < 1.0e-8f) return false;
    const float u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / den;
    const float v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / den;
    const float w = 1.0f - u - v;
    if (u < -1.0e-5f || v < -1.0e-5f || w < -1.0e-5f) return false;
    height = u * a[1] + v * b[1] + w * c[1];
    return true;
}

void Multiply(const float a[16], const float b[16], float out[16]) {
    float result[16];
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a[k * 4 + row] * b[column * 4 + k];
            result[column * 4 + row] = sum;
        }
    std::copy_n(result, 16, out);
}

struct ArenaRuntime {
    bool configured = false;
    bool loaded = false;
    bool installed = false;
    IxionArena arena;
    std::vector<IxionArenaTileState> states;
    IxionArenaTrigger trigger;
    int event_marker = -1;
    bool collision_dirty = true;
    std::vector<IxionRoomTriangle> active;
    std::vector<uint16_t> active_tags;
    IxionRoom camera_room; // collision only: the PT_CAM triangles
    std::vector<gfx::MeshHandle> meshes; // tile, object, group order
    gfx::TextureHandle glow = gfx::kNullTexture;
    gfx::TextureHandle flipbook = gfx::kNullTexture;
    bool gpu_failed = false;
    uint64_t step = 0;
    int hit_logs = 0;
    int map_hit_logs = 0;
    std::vector<std::pair<int, int>> debug_hits; // frame, slot
    int last_step_frame = -1;
    int query_logs = 0;
    size_t next_debug_hit = 0;
    bool draw_logged = false;
};

ArenaRuntime &Runtime() {
    static ArenaRuntime runtime;
    return runtime;
}

// DC1 game int flag 0 for d01 (the boss flag prog100 tests); outlives configure and install.
int g_boss_flag = 0;

void ReleaseGpu(ArenaRuntime &rt) {
    for (gfx::MeshHandle mesh : rt.meshes) if (mesh != gfx::kNullMesh) gfx::DestroyMesh(mesh);
    rt.meshes.clear();
    if (rt.glow != gfx::kNullTexture) gfx::DestroyTexture(rt.glow);
    if (rt.flipbook != gfx::kNullTexture) gfx::DestroyTexture(rt.flipbook);
    rt.glow = gfx::kNullTexture;
    rt.flipbook = gfx::kNullTexture;
}

gfx::TextureHandle Upload(const IxionRoomTexture &source) {
    auto texture = gfx::CreateTexture({source.width, source.height, gfx::TextureFormat::Rgba8});
    if (texture == gfx::kNullTexture) return texture;
    if (!gfx::UpdateTextureNow(texture, 0, 0, 0, source.width, source.height, source.rgba.data())) {
        gfx::DestroyTexture(texture);
        return gfx::kNullTexture;
    }
    return texture;
}

bool PrepareGpu(ArenaRuntime &rt) {
    if (rt.gpu_failed) return false;
    if (!rt.meshes.empty()) return true;
    size_t groups = 0;
    for (const auto &tile : rt.arena.tiles)
        for (const auto &object : tile.objects) groups += object.groups.size();
    rt.meshes.reserve(groups);
    bool ok = true;
    for (const auto &tile : rt.arena.tiles) {
        for (const auto &object : tile.objects) {
            for (const auto &group : object.groups) {
                std::vector<gfx::Vertex3D> vertices(group.vertices.size());
                std::vector<uint32_t> indices(group.vertices.size());
                for (size_t v = 0; v < group.vertices.size(); ++v) {
                    const IxionRoomVertex &input = group.vertices[v];
                    std::copy(input.position.begin(), input.position.end(), vertices[v].position);
                    std::copy(input.normal.begin(), input.normal.end(), vertices[v].normal);
                    std::copy(input.uv.begin(), input.uv.end(), vertices[v].uv);
                    for (int channel = 0; channel < 4; ++channel) vertices[v].color[channel] = IxionRoomGsColor(input.color[channel]);
                    indices[v] = static_cast<uint32_t>(v);
                }
                const gfx::MeshHandle mesh = gfx::CreateMesh(vertices, indices);
                rt.meshes.push_back(mesh);
                ok = ok && mesh != gfx::kNullMesh;
            }
        }
    }
    rt.glow = Upload(rt.arena.glow);
    rt.flipbook = Upload(rt.arena.flipbook);
    ok = ok && rt.glow != gfx::kNullTexture && rt.flipbook != gfx::kNullTexture;
    if (!ok) {
        ReleaseGpu(rt);
        rt.gpu_failed = true;
        std::fprintf(stderr, "ixion arena GPU prepare failed; tiles keep colliding but are not drawn\n");
        return false;
    }
    std::fprintf(stderr, "ixion arena GPU prepared: tile_meshes=%zu fire_textures=2 glow=%ux%u flipbook=%ux%u\n",
                 rt.meshes.size(), rt.arena.glow.width, rt.arena.glow.height, rt.arena.flipbook.width,
                 rt.arena.flipbook.height);
    return true;
}

void ResetState(ArenaRuntime &rt) {
    rt.states.assign(rt.arena.tiles.size(), IxionArenaTileState{});
    for (size_t i = 0; i < rt.states.size(); ++i) {
        rt.states[i].state = rt.arena.initial_state;
        rt.states[i].time = rt.arena.tiles[i].first_frame;
    }
    rt.trigger = {};
    rt.collision_dirty = true;
    rt.step = 0;
    rt.next_debug_hit = 0;
    rt.last_step_frame = -1;
}

void RebuildCollision(ArenaRuntime &rt) {
    if (!rt.collision_dirty) return;
    rt.collision_dirty = false;
    rt.active.clear();
    rt.active_tags.clear();
    if (!rt.installed) return;
    for (size_t slot = 0; slot < rt.arena.tiles.size(); ++slot) {
        if (!IxionArenaTileCollides(rt.states[slot].state)) continue;
        for (const IxionRoomTriangle &triangle : rt.arena.tiles[slot].collision) {
            rt.active.push_back(triangle);
            rt.active_tags.push_back(static_cast<uint16_t>(kIxionArenaPartsTag | slot));
        }
    }
}

void Hit(ArenaRuntime &rt, int slot, const char *source) {
    if (!rt.installed || slot < 0 || slot >= static_cast<int>(rt.states.size())) return;
    const int before = rt.states[slot].state;
    const bool changed = IxionArenaApplyHit(rt.states, slot, rt.arena.hit_decrement_ge);
    if (changed) rt.collision_dirty = true;
    if (rt.hit_logs < 64) {
        ++rt.hit_logs;
        std::fprintf(stderr, "ixion arena hit slot=%d source=%s state=%d->%d frame=%d\n", slot, source, before,
                     rt.states[slot].state, mgPortFrameCount());
    }
}

void ParseDebugHits(ArenaRuntime &rt) {
    rt.debug_hits.clear();
    const char *text = std::getenv("DC2_IXION_ROOM_ARENA_DEBUG_HIT");
    if (text == nullptr || *text == '\0') return;
    std::string_view rest(text);
    while (!rest.empty()) {
        const size_t comma = rest.find(',');
        const std::string_view item = rest.substr(0, comma);
        rest = comma == std::string_view::npos ? std::string_view() : rest.substr(comma + 1);
        const size_t colon = item.find(':');
        int frame = -1, slot = -1;
        if (colon != std::string_view::npos &&
            std::from_chars(item.data(), item.data() + colon, frame).ec == std::errc() &&
            std::from_chars(item.data() + colon + 1, item.data() + item.size(), slot).ec == std::errc() && frame >= 0 &&
            slot >= 0 && slot < kMaxSlots) {
            rt.debug_hits.emplace_back(frame, slot);
        } else {
            std::fprintf(stderr, "ixion arena DC2_IXION_ROOM_ARENA_DEBUG_HIT item ignored (want frame:slot)\n");
        }
    }
    std::sort(rt.debug_hits.begin(), rt.debug_hits.end());
    std::fprintf(stderr, "ixion arena debug hits armed: %zu (proof knob; DC1 hits come from projectiles)\n", rt.debug_hits.size());
}

// d01-event prog5 (scratch/w4-dranai/d01-event.stmts.txt:673-727): after the fall cut-scene the
// player is set at donor (0, 0, -309) (EXT(137) at c+0x3b5b0) facing 3.14 (EXT(436)).
struct TracedRespawn {
    const char *room;
    std::array<float, 3> donor;
};
constexpr TracedRespawn kTracedRespawns[] = {{"d01boss", {0.0f, 0.0f, -309.0f}}};

} // namespace

// ---- Pure rule -----------------------------------------------------------------------------

bool IxionArenaApplyHit(std::vector<IxionArenaTileState> &states, int slot, int decrement_ge) {
    if (slot < 0 || slot >= static_cast<int>(states.size())) return false;
    if (states[slot].state < decrement_ge) return false;
    --states[slot].state;
    return true;
}

int IxionArenaStepTiles(std::vector<IxionArenaTileState> &states, const std::vector<IxionArenaTile> &tiles) {
    int started = 0;
    for (size_t i = 0; i < states.size() && i < tiles.size(); ++i) {
        IxionArenaTileState &tile = states[i];
        if (tile.state == 2) {
            tile.state = 1;
            tile.time = tiles[i].first_frame;
            ++started;
        } else if (tile.state == 1) {
            tile.time += tiles[i].speed;
            if (tile.time >= tiles[i].done_at) tile.state = 0;
        }
    }
    return started;
}

void IxionArenaSamplePose(const IxionArenaObject &object, float first_frame, float time, float t[3], float q[4]) {
    if (object.track_t.empty() || object.track_q.size() != object.track_t.size()) {
        std::copy(object.rest_t.begin(), object.rest_t.end(), t);
        std::copy(object.rest_q.begin(), object.rest_q.end(), q);
        return;
    }
    size_t i0 = 0, i1 = 0;
    float blend = 0.0f;
    TrackIndex(object.track_t.size(), first_frame, time, i0, i1, blend);
    for (int axis = 0; axis < 3; ++axis)
        t[axis] = object.track_t[i0][axis] + (object.track_t[i1][axis] - object.track_t[i0][axis]) * blend;
    const auto &a = object.track_q[i0];
    auto b = object.track_q[i1];
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    if (dot < 0.0f) {
        for (float &component : b) component = -component;
        dot = -dot;
    }
    float wa = 1.0f - blend, wb = blend;
    if (dot < 0.9995f) {
        const float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
        const float sine = std::sin(angle);
        wa = std::sin((1.0f - blend) * angle) / sine;
        wb = std::sin(blend * angle) / sine;
    }
    float length = 0.0f;
    for (int i = 0; i < 4; ++i) {
        q[i] = wa * a[i] + wb * b[i];
        length += q[i] * q[i];
    }
    length = std::sqrt(length);
    if (length > 1.0e-8f)
        for (int i = 0; i < 4; ++i) q[i] /= length;
}

float IxionArenaSampleAlpha(const IxionArenaObject &object, int material, float first_frame, float time) {
    for (const auto &[key, curve] : object.track_alpha) {
        if (key != material || curve.empty()) continue;
        size_t i0 = 0, i1 = 0;
        float blend = 0.0f;
        TrackIndex(curve.size(), first_frame, time, i0, i1, blend);
        return Clamp01(curve[i0] + (curve[i1] - curve[i0]) * blend);
    }
    return 1.0f;
}

void IxionArenaPoseMatrix(const float t[3], const float q[4], float out[16]) {
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    out[0] = 1.0f - 2.0f * (y * y + z * z);
    out[1] = 2.0f * (x * y + w * z);
    out[2] = 2.0f * (x * z - w * y);
    out[3] = 0.0f;
    out[4] = 2.0f * (x * y - w * z);
    out[5] = 1.0f - 2.0f * (x * x + z * z);
    out[6] = 2.0f * (y * z + w * x);
    out[7] = 0.0f;
    out[8] = 2.0f * (x * z + w * y);
    out[9] = 2.0f * (y * z - w * x);
    out[10] = 1.0f - 2.0f * (x * x + y * y);
    out[11] = 0.0f;
    out[12] = t[0];
    out[13] = t[1];
    out[14] = t[2];
    out[15] = 1.0f;
}

bool IxionArenaInsideMarker(const IxionArenaMarker &marker, const float position[3]) {
    if (std::abs(position[1] - marker.position[1]) >= marker.height_window) return false;
    const float dx = position[0] - marker.position[0];
    const float dz = position[2] - marker.position[2];
    return dx * dx + dz * dz <= marker.horizontal_radius * marker.horizontal_radius;
}

bool IxionArenaTriggerStep(IxionArenaTrigger &trigger, bool inside) {
    if (!trigger.switched_on) return false;
    if (!inside) {
        trigger.armed = true;
        return false;
    }
    if (!trigger.armed) return false;
    trigger.switched_on = false;
    trigger.armed = false;
    return true;
}

bool IxionArenaSlotFromParts(int parts_no, int &slot) {
    if ((parts_no & kIxionArenaPartsMask) != kIxionArenaPartsTag) return false;
    slot = parts_no & ~kIxionArenaPartsMask;
    return slot < kMaxSlots;
}

std::string IxionArenaSha256Hex(const uint8_t *data, size_t size) {
    static constexpr uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    auto rotr = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    auto block = [&](const uint8_t *p) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t(p[4 * i]) << 24) | (uint32_t(p[4 * i + 1]) << 16) | (uint32_t(p[4 * i + 2]) << 8) | uint32_t(p[4 * i + 3]);
        for (int i = 16; i < 64; ++i) {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            const uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    };
    size_t offset = 0;
    for (; offset + 64 <= size; offset += 64) block(data + offset);
    uint8_t tail[128] = {};
    const size_t rest = size - offset;
    if (rest > 0) std::memcpy(tail, data + offset, rest);
    tail[rest] = 0x80;
    const size_t tail_size = rest + 1 + 8 <= 64 ? 64 : 128;
    const uint64_t bits = static_cast<uint64_t>(size) * 8u;
    for (int i = 0; i < 8; ++i) tail[tail_size - 1 - i] = static_cast<uint8_t>(bits >> (8 * i));
    block(tail);
    if (tail_size == 128) block(tail + 64);
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(64);
    for (uint32_t word : h)
        for (int shift = 28; shift >= 0; shift -= 4) out.push_back(kHex[(word >> shift) & 0xF]);
    return out;
}

bool IxionArenaHasSupport(const IxionRoom &room, const std::vector<IxionRoomTriangle> &tiles, const float p[3]) {
    auto supported = [&](const std::vector<IxionRoomTriangle> &triangles) {
        for (const IxionRoomTriangle &triangle : triangles) {
            float height = 0.0f;
            if (HeightAt(triangle, p[0], p[2], height) && std::abs(height - p[1]) <= 20.0f) return true;
        }
        return false;
    };
    return supported(room.collision) || supported(tiles);
}

bool IxionArena::Load(const std::filesystem::path &json_path, size_t texture_count, IxionArena &out, std::string &error) {
    std::vector<uint8_t> bytes;
    if (!ReadFileBytes(json_path, kSidecarLimit, bytes)) return Fail(error, "arena.json is missing, empty or oversized");
    try {
        const Json root = Json::parse(bytes.begin(), bytes.end());
        IxionArena parsed;
        ParseArena(root, json_path.parent_path(), texture_count, parsed);
        out = std::move(parsed);
        return true;
    } catch (const std::exception &exception) {
        return Fail(error, std::string("arena.json rejected: ") + exception.what());
    }
}

// ---- Runtime --------------------------------------------------------------------------------

void IxionRoomArenaConfigure(const std::filesystem::path &room_json, const IxionRoom &room) {
    ArenaRuntime &rt = Runtime();
    if (rt.configured) return;
    rt.configured = true;
    std::filesystem::path sidecar;
    const char *setting = std::getenv("DC2_IXION_ROOM_ARENA");
    if (setting != nullptr && std::string_view(setting) == "0") {
        std::fprintf(stderr, "ixion arena disabled by DC2_IXION_ROOM_ARENA=0\n");
        return;
    }
    if (setting != nullptr && *setting != '\0' && std::string_view(setting) != "1") {
        sidecar = PathsFromUtf8(setting);
    } else {
        sidecar = room_json.parent_path() / "arena.json";
        std::error_code ec;
        if (!std::filesystem::exists(sidecar, ec)) {
            if (setting != nullptr && std::string_view(setting) == "1")
                std::fprintf(stderr, "ixion arena rejected: DC2_IXION_ROOM_ARENA=1 but no arena.json beside room.json\n");
            return; // absent: the adapter is unchanged and logs nothing
        }
    }
    std::string error;
    IxionArena arena;
    if (!IxionArena::Load(sidecar, room.textures.size(), arena, error)) {
        std::fprintf(stderr, "ixion arena rejected: %s; room without arena\n", error.c_str());
        return;
    }
    if (arena.room != room.donor_room) {
        std::fprintf(stderr, "ixion arena rejected: sidecar room %s does not match room donor %s\n", arena.room.c_str(),
                     room.donor_room.c_str());
        return;
    }
    std::vector<uint8_t> room_bytes;
    if (!ReadFileBytes(room_json, kSidecarLimit, room_bytes) ||
        IxionArenaSha256Hex(room_bytes.data(), room_bytes.size()) != arena.room_json_sha256) {
        std::fprintf(stderr, "ixion arena rejected: room_json_sha256 does not match the loaded room.json\n");
        return;
    }
    rt.arena = std::move(arena);
    rt.camera_room = IxionRoom{};
    // DC2_IXION_ROOM_ARENA_CAMERA=0 keeps DC2's camera on the room collision only.
    const char *camera = std::getenv("DC2_IXION_ROOM_ARENA_CAMERA");
    if (camera == nullptr || std::string_view(camera) != "0") rt.camera_room.collision = rt.arena.camera;
    rt.event_marker = -1;
    for (size_t i = 0; i < rt.arena.markers.size(); ++i)
        if (rt.arena.markers[i].auto_start && rt.arena.markers[i].script == 500) rt.event_marker = static_cast<int>(i);
    rt.loaded = true;
    ParseDebugHits(rt);
    size_t triangles = 0, groups = 0;
    for (const auto &tile : rt.arena.tiles) {
        triangles += tile.collision.size();
        for (const auto &object : tile.objects) groups += object.groups.size();
    }
    std::fprintf(stderr,
                 "ixion arena loaded: room=%s tiles=%zu tile_triangles=%zu tile_groups=%zu fires=%zu camera_triangles=%zu markers=%zu event_marker=%s fall_below_y=%.3f\n",
                 rt.arena.room.c_str(), rt.arena.tiles.size(), triangles, groups, rt.arena.fires.size(),
                 rt.camera_room.collision.size(), rt.arena.markers.size(),
                 rt.event_marker >= 0 ? rt.arena.markers[rt.event_marker].name.c_str() : "none", rt.arena.fall_below_y);
}

bool IxionRoomArenaLoaded() { return Runtime().loaded; }

bool IxionRoomArenaInstalled() { return Runtime().installed; }

void IxionRoomArenaSetBossFlag(int value) {
    ArenaRuntime &rt = Runtime();
    const int before = g_boss_flag;
    g_boss_flag = value;
    if (value >= 100) rt.trigger.switched_on = false;
    std::fprintf(stderr, "ixion arena boss flag %d -> %d (DC1 game int flag 0); event-500 trigger switched_on=%d\n", before,
                 value, rt.trigger.switched_on ? 1 : 0);
}

int IxionRoomArenaBossFlag() { return g_boss_flag; }

bool IxionRoomArenaDefersDonor() {
    const ArenaRuntime &rt = Runtime();
    return rt.loaded && rt.event_marker >= 0;
}

void IxionRoomArenaInstall() {
    ArenaRuntime &rt = Runtime();
    if (!rt.loaded || rt.installed) return;
    rt.installed = true;
    ResetState(rt);
    // prog100 (d01-event.stmts.txt:1241-1300): while the boss flag is below 100 it switches 500 on.
    // DRAN-R2: event 510 (Dran's death) sets the flag to 100 (IxionRoomArenaSetBossFlag).
    rt.trigger.switched_on = rt.event_marker >= 0 && g_boss_flag < 100;
    rt.draw_logged = false;
    std::fprintf(stderr, "ixion arena installed: tiles=%zu state=%d trigger=%s switched_on=%d\n", rt.states.size(),
                 rt.arena.initial_state, rt.event_marker >= 0 ? rt.arena.markers[rt.event_marker].name.c_str() : "none",
                 rt.trigger.switched_on ? 1 : 0);
}

void IxionRoomArenaRetire(const char *reason) {
    ArenaRuntime &rt = Runtime();
    if (!rt.installed && rt.meshes.empty() && rt.glow == gfx::kNullTexture) return;
    const size_t meshes = rt.meshes.size();
    rt.installed = false;
    ReleaseGpu(rt);
    rt.collision_dirty = true;
    RebuildCollision(rt);
    std::fprintf(stderr, "ixion arena retired: reason=%s released_meshes=%zu live_meshes=%zu active_triangles=%zu\n",
                 reason != nullptr ? reason : "unspecified", meshes, rt.meshes.size(), rt.active.size());
}

void IxionRoomArenaDiscard() { IxionRoomArenaRetire("discard"); }

IxionRoomArenaFrameResult IxionRoomArenaFrame(int frame, bool field, bool player_valid, const float player[3]) {
    IxionRoomArenaFrameResult result;
    ArenaRuntime &rt = Runtime();
    if (!rt.installed) return result;
    // mainloop.cpp calls the boundary before and after every frame; DC1 steps once per frame.
    if (frame == rt.last_step_frame) return result;
    rt.last_step_frame = frame;
    ++rt.step;
    while (rt.next_debug_hit < rt.debug_hits.size() && frame >= rt.debug_hits[rt.next_debug_hit].first) {
        Hit(rt, rt.debug_hits[rt.next_debug_hit].second, "debug-knob");
        ++rt.next_debug_hit;
    }
    if (!field) return result;
    std::vector<int> before(rt.states.size());
    for (size_t i = 0; i < rt.states.size(); ++i) before[i] = rt.states[i].state;
    IxionArenaStepTiles(rt.states, rt.arena.tiles);
    for (size_t i = 0; i < rt.states.size(); ++i) {
        if (before[i] == rt.states[i].state) continue;
        if (IxionArenaTileCollides(before[i]) != IxionArenaTileCollides(rt.states[i].state)) rt.collision_dirty = true;
        if (rt.states[i].state == 1)
            std::fprintf(stderr, "ixion arena tile slot=%zu collapse started frame=%d collision=off (SE_DRAN_FIELD_START has no DC2 mapping; silent)\n",
                         i, frame);
        if (rt.states[i].state == 0)
            std::fprintf(stderr, "ixion arena tile slot=%zu removed frame=%d time=%.2f\n", i, frame, rt.states[i].time);
    }
    if (!player_valid || player == nullptr) return result;
    if (rt.event_marker >= 0) {
        const IxionArenaMarker &marker = rt.arena.markers[rt.event_marker];
        const bool was_armed = rt.trigger.armed;
        const bool inside = IxionArenaInsideMarker(marker, player);
        if (IxionArenaTriggerStep(rt.trigger, inside)) {
            result.event_fired = true;
            result.event_script = marker.script;
            std::fprintf(stderr, "ixion arena event %d fired: marker=%s frame=%d player=(%.3f,%.3f,%.3f) marker=(%.3f,%.3f,%.3f) switched_on=0\n",
                         marker.script, marker.name.c_str(), frame, player[0], player[1], player[2], marker.position[0],
                         marker.position[1], marker.position[2]);
        } else if (!was_armed && rt.trigger.armed) {
            std::fprintf(stderr, "ixion arena trigger %s armed (player left it) frame=%d player=(%.3f,%.3f,%.3f)\n",
                         marker.name.c_str(), frame, player[0], player[1], player[2]);
        }
    }
    if (player[1] <= rt.arena.fall_below_y) result.fell = true;
    return result;
}

void IxionRoomArenaActiveCollision(const std::vector<IxionRoomTriangle> *&triangles, const std::vector<uint16_t> *&tags) {
    ArenaRuntime &rt = Runtime();
    RebuildCollision(rt);
    triangles = &rt.active;
    tags = &rt.active_tags;
}

int IxionRoomArenaCameraPoly(CCPoly *polygons, int capacity, const float bounds_min[3], const float bounds_max[3]) {
    const ArenaRuntime &rt = Runtime();
    if (!rt.installed || rt.camera_room.collision.empty() || capacity <= 0) return 0;
    return IxionRoomCollision(rt.camera_room, bounds_min, bounds_max, polygons, capacity);
}

void IxionRoomArenaDraw(const std::vector<gfx::TextureHandle> &room_textures, const gfx::MeshConstants &base,
                        const gfx::DrawState &base_state) {
    ArenaRuntime &rt = Runtime();
    if (!rt.installed || !PrepareGpu(rt)) return;
    size_t mesh_index = 0;
    int tiles_drawn = 0, falling = 0;
    for (size_t slot = 0; slot < rt.arena.tiles.size(); ++slot) {
        const IxionArenaTile &tile = rt.arena.tiles[slot];
        const IxionArenaTileState &state = rt.states[slot];
        size_t groups = 0;
        for (const auto &object : tile.objects) groups += object.groups.size();
        if (!IxionArenaTileDrawn(state.state)) {
            mesh_index += groups;
            continue;
        }
        ++tiles_drawn;
        if (state.state == 1) ++falling;
        for (const IxionArenaObject &object : tile.objects) {
            float t[3], q[4];
            if (state.state >= 2) {
                std::copy(object.rest_t.begin(), object.rest_t.end(), t);
                std::copy(object.rest_q.begin(), object.rest_q.end(), q);
            } else {
                IxionArenaSamplePose(object, tile.first_frame, state.time, t, q);
            }
            float model[16];
            IxionArenaPoseMatrix(t, q, model);
            gfx::MeshConstants constants = base;
            Multiply(base.mvp, model, constants.mvp);
            const auto transform = mgPortMeshTransform(model);
            for (const IxionArenaGroup &group : object.groups) {
                const gfx::MeshHandle mesh = rt.meshes[mesh_index++];
                const float alpha = state.state == 1 ? IxionArenaSampleAlpha(object, group.material, tile.first_frame, state.time) : 1.0f;
                if (alpha <= 1.0f / 255.0f) continue;
                gfx::MeshConstants draw_constants = constants;
                draw_constants.diffuse[3] = alpha;
                gfx::DrawState draw_state = base_state;
                if (alpha < 1.0f) {
                    draw_state.blend = true;
                    draw_state.alpha = gfx::GsBlend{}; // (Cs - Cd) * As + Cd
                    draw_state.depth_write = false;
                }
                gfx::TextureBinding binding;
                if (static_cast<size_t>(group.texture) < room_textures.size()) binding.texture = room_textures[group.texture];
                binding.filter = gfx::Filter::Linear;
                binding.wrap_u = gfx::Wrap::Repeat;
                binding.wrap_v = gfx::Wrap::Repeat;
                mgPortDrawMesh(mesh, 0, static_cast<uint32_t>(group.vertices.size()), draw_constants, binding, draw_state,
                               &transform);
            }
        }
    }

    // CFireOmni (fireomni.cpp:68-195): additive, no depth write, drawn after the opaque pass.
    const gfx::MeshTransform identity = mgPortMeshTransform(gfx::IdentityMeshTransform().model);
    float eye_to_world[16];
    if (!rt.arena.fires.empty() && gfx::InvertAffineTransform(identity.view, eye_to_world)) {
        const float camera[3] = {eye_to_world[12], eye_to_world[13], eye_to_world[14]};
        gfx::MeshConstants constants = base;
        constants.flags = gfx::kMeshVertexColor;
        std::fill_n(constants.diffuse, 4, 1.0f);
        gfx::DrawState state = base_state;
        state.blend = true;
        state.alpha = gfx::GsBlend{0, 2, 0, 1, 0x80}; // Cs * As + Cd
        state.depth_write = false;
        state.fog = false;
        state.cull = gfx::CullMode::None;
        const int frames = std::max(1, rt.arena.flipbook_frames);
        const int frame = static_cast<int>(rt.step % static_cast<uint64_t>(frames));
        const float fu = static_cast<float>(rt.arena.flipbook_frame_w) / static_cast<float>(rt.arena.flipbook.width);
        const float fv = static_cast<float>(rt.arena.flipbook_frame_h) / static_cast<float>(rt.arena.flipbook.height);
        const float u0 = static_cast<float>(frame % rt.arena.flipbook_columns) * fu;
        const float v0 = static_cast<float>(frame / rt.arena.flipbook_columns) * fv;
        auto quad = [&](const float center[3], const float right[3], float half, gfx::TextureHandle texture, float ua,
                        float va, float ub, float vb) {
            gfx::Vertex3D vertices[4] = {};
            const float corner[4][2] = {{-1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}};
            const float uv[4][2] = {{ua, va}, {ub, va}, {ub, vb}, {ua, vb}};
            for (int i = 0; i < 4; ++i) {
                vertices[i].position[0] = center[0] + right[0] * corner[i][0] * half;
                vertices[i].position[1] = center[1] + corner[i][1] * half;
                vertices[i].position[2] = center[2] + right[2] * corner[i][0] * half;
                vertices[i].normal[1] = 1.0f;
                vertices[i].uv[0] = uv[i][0];
                vertices[i].uv[1] = uv[i][1];
                std::fill_n(vertices[i].color, 4, uint8_t{0x80});
            }
            const uint32_t indices[6] = {0, 1, 2, 0, 2, 3};
            gfx::TextureBinding binding;
            binding.texture = texture;
            binding.filter = gfx::Filter::Linear;
            binding.wrap_u = gfx::Wrap::Clamp;
            binding.wrap_v = gfx::Wrap::Clamp;
            mgPortDrawMeshImmediate(vertices, indices, constants, binding, state, &identity);
        };
        for (size_t fire = 0; fire < rt.arena.fires.size(); ++fire) {
            const float base_center[3] = {rt.arena.fires[fire][0], rt.arena.fires[fire][1] + rt.arena.fire_offset_y,
                                          rt.arena.fires[fire][2]};
            float to_camera[3] = {camera[0] - base_center[0], camera[1] - base_center[1], camera[2] - base_center[2]};
            const float distance = std::sqrt(to_camera[0] * to_camera[0] + to_camera[1] * to_camera[1] + to_camera[2] * to_camera[2]);
            if (!(distance > 1.0e-3f)) continue;
            for (float &component : to_camera) component /= distance;
            // Cylindrical billboard about world up: right = up x to_camera.
            float right[3] = {to_camera[2], 0.0f, -to_camera[0]};
            const float right_length = std::sqrt(right[0] * right[0] + right[2] * right[2]);
            if (right_length < 1.0e-4f) {
                right[0] = 1.0f;
                right[2] = 0.0f;
            } else {
                right[0] /= right_length;
                right[2] /= right_length;
            }
            // Big glow (layer 2) first, then the small glow and the flame (layer 1).
            for (auto layer = rt.arena.fire_layers.rbegin(); layer != rt.arena.fire_layers.rend(); ++layer) {
                float scale = 1.0f;
                if (layer->flicker_every > 0) {
                    const uint64_t tick = rt.step / static_cast<uint64_t>(layer->flicker_every);
                    const uint32_t hash = static_cast<uint32_t>((tick * 2654435761u) ^ (fire * 40503u));
                    scale = 1.0f + (layer->flicker_max - 1.0f) * static_cast<float>(hash % 1000u) / 999.0f;
                }
                // Square quads: the flame frames are 128x128 and DC1's 2:1 sizes are field-height
                // (448-line output of a 224-line field), an unverified reading (ARENA-R1).
                const float half = 0.5f * layer->width * scale;
                const float center[3] = {base_center[0] + to_camera[0] * layer->toward_camera,
                                         base_center[1] + to_camera[1] * layer->toward_camera,
                                         base_center[2] + to_camera[2] * layer->toward_camera};
                quad(center, right, half, rt.glow, 0.0f, 0.0f, 1.0f, 1.0f);
                if (layer->flame) quad(center, right, half, rt.flipbook, u0, v0, u0 + fu, v0 + fv);
            }
        }
    }
    if (!rt.draw_logged) {
        rt.draw_logged = true;
        std::fprintf(stderr, "ixion arena render submitted: frame=%d tiles_drawn=%d falling=%d fires=%zu layers=%zu\n",
                     mgPortFrameCount(), tiles_drawn, falling, rt.arena.fires.size(), rt.arena.fire_layers.size());
    }
}

void IxionRoomArenaProjectileHit(int parts_no, const char *source) {
    ArenaRuntime &rt = Runtime();
    if (!rt.installed) return;
    int slot = -1;
    const bool tile = IxionArenaSlotFromParts(parts_no, slot);
    if (rt.map_hit_logs < 32) {
        ++rt.map_hit_logs;
        std::fprintf(stderr, "ixion arena projectile map hit source=%s parts_no=0x%04x tile=%d frame=%d\n", source,
                     parts_no & 0xFFFF, tile ? slot : -1, mgPortFrameCount());
    }
    if (tile) Hit(rt, slot, source);
}

bool IxionRoomArenaFallRespawn(const IxionRoom &room, float out[3]) {
    const ArenaRuntime &rt = Runtime();
    out[0] = room.spawn[0];
    out[1] = room.spawn[1];
    out[2] = room.spawn[2];
    if (!rt.loaded) return false;
    for (const TracedRespawn &traced : kTracedRespawns) {
        if (rt.arena.room != traced.room) continue;
        const float candidate[3] = {traced.donor[0] * rt.arena.scale, traced.donor[1] * rt.arena.scale,
                                    traced.donor[2] * rt.arena.scale};
        std::vector<IxionRoomTriangle> tiles;
        for (size_t slot = 0; slot < rt.arena.tiles.size() && slot < rt.states.size(); ++slot)
            if (IxionArenaTileCollides(rt.states[slot].state))
                tiles.insert(tiles.end(), rt.arena.tiles[slot].collision.begin(), rt.arena.tiles[slot].collision.end());
        if (IxionArenaHasSupport(room, tiles, candidate)) {
            std::copy_n(candidate, 3, out);
            return true;
        }
    }
    return false;
}

int IxionRoomArenaFallLife() { return Runtime().arena.fall_life; }

void IxionRoomArenaResetForTest() {
    ArenaRuntime &rt = Runtime();
    ReleaseGpu(rt);
    rt = ArenaRuntime{};
    g_boss_flag = 0;
}

void IxionRoomArenaAdoptForTest(IxionArena arena) {
    ArenaRuntime &rt = Runtime();
    rt = ArenaRuntime{};
    rt.configured = true;
    rt.arena = std::move(arena);
    rt.camera_room.collision = rt.arena.camera;
    for (size_t i = 0; i < rt.arena.markers.size(); ++i)
        if (rt.arena.markers[i].auto_start && rt.arena.markers[i].script == 500) rt.event_marker = static_cast<int>(i);
    rt.loaded = true;
}

std::vector<IxionArenaTileState> IxionRoomArenaStatesForTest() { return Runtime().states; }

const IxionArenaTrigger &IxionRoomArenaTriggerForTest() { return Runtime().trigger; }

// ps2/src/effscript.cpp's _INTERSECTION_POINT, unchanged, plus one bookkeeping call: when the running
// effect carries a damage primitive (a projectile such as Max's 銃弾, ShotNormalGun) and its segment
// hits an arena tile, the DC1 hit rule runs (DC1: CSHOT/CSHOT_EFFECT/CSHOT_MACHINGUN map hits).
PORT_OVERRIDE int _INTERSECTION_POINT(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *const entry = stack;
    sceVu0FVECTOR start;
    sceVu0FVECTOR end;
    sceVu0FVECTOR hit;
    sceVu0FVECTOR reflection;
    mgVu0FBOX     box;
    CCPoly        poly[0x80];
    sceVu0FVECTOR normal;
    auto stack_int = [](RS_STACKDATA *slot) { return slot->type == RS_FLOAT ? fptosi(slot->val.f) : slot->val.i; };
    auto stack_float = [](RS_STACKDATA *slot) { return slot->type == RS_INT ? static_cast<float>(slot->val.i) : slot->val.f; };
    auto stack_vector = [&](float *vector, RS_STACKDATA *slot) {
        vector[0] = stack_float(slot++);
        vector[1] = stack_float(slot++);
        vector[2] = stack_float(slot);
        vector[3] = 1.0f;
    };
    auto set_int = [](RS_STACKDATA *slot, int value) {
        if (slot->type == RS_PTR) slot->val.p->val.i = value;
    };
    auto set_float = [](RS_STACKDATA *slot, float value) {
        if (slot->type == RS_PTR) slot->val.p->val.f = value;
    };

    if (argc != 8 && argc != 9 && argc != 10 && argc != 11 && argc != 12 && argc != 13 && argc != 14 && argc != 15 && argc != 16) {
        return 0;
    }

    int ignore_mask = stack_int(stack++);
    stack_vector(start, stack);
    stack_vector(end, stack + 3);
    stack += 6;
    float range = 10.0f + mgDistVector(start, end);
    box.max[0] = range + start[0];
    box.min[0] = start[0] - range;
    box.max[1] = range + start[1];
    box.min[1] = start[1] - range;
    box.max[2] = range + start[2];
    box.min[2] = start[2] - range;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    int poly_num = now_scene->GetColPoly(poly, box, 0x80);

    if (poly_num >= 0x80) {
        printf("effect script[INTERSECTION_POINT] poly num over[%d]\n", poly_num);
        return 0;
    }

    CCPoly *hit_poly = poly;
    int     hit_no = CheckHit(hit_poly, poly_num, start, end, hit, 1, ignore_mask);
    int     foot_sound;
    int     area_kind;

    if (hit_no >= 0) {
        hit_poly = &hit_poly[hit_no];
        sceVu0Normalize(normal, hit_poly->normal);
        mgReflectionPlane(normal, hit, start, reflection);
        sceVu0Normalize(reflection, reflection);
        foot_sound = hit_poly->foot_sound;
        area_kind = hit_poly->area_kind;

        if (foot_sound == 0) {
            CMap *map = now_scene->GetMap(now_scene->active_map);

            if (map != NULL) {
                foot_sound = map->map_info.def_foot;
            }
        }

        if (IxionRoomArenaInstalled()) {
            const _EFF_SCRIPT *script = IxionRoomRunningEffect(EffScriptMan, entry);
            if (script != nullptr && script->colprim != nullptr) IxionRoomArenaProjectileHit(hit_poly->parts_no, "effect-projectile");
        }
    }
    if (IxionRoomArenaInstalled()) {
        ArenaRuntime &rt = Runtime();
        const _EFF_SCRIPT *script = IxionRoomRunningEffect(EffScriptMan, entry);
        if (rt.query_logs < 24 && script != nullptr && script->colprim != nullptr) {
            ++rt.query_logs;
            std::fprintf(stderr, "ixion arena projectile map query frame=%d polys=%d hit=%d parts_no=0x%04x\n",
                         mgPortFrameCount(), poly_num, hit_no, hit_no >= 0 ? hit_poly->parts_no : 0);
        }
    }

    switch (argc) {
        case 8:
        case 9:
        case 10:
            set_int(stack++, hit_no);

            if (argc >= 9) {
                set_int(stack++, area_kind);
            }

            if (argc == 10) {
                set_int(stack, foot_sound);
            }

            break;
        case 11:
        case 12:
        case 13:
            set_float(stack++, hit[0]);
            set_float(stack++, hit[1]);
            set_float(stack++, hit[2]);
            set_int(stack++, hit_no);

            if (argc >= 12) {
                set_int(stack++, area_kind);
            }

            if (argc == 13) {
                set_int(stack, foot_sound);
            }

            break;
        case 14:
        case 15:
        case 16:
            set_float(stack++, hit[0]);
            set_float(stack++, hit[1]);
            set_float(stack++, hit[2]);
            set_float(stack++, reflection[0]);
            set_float(stack++, reflection[1]);
            set_float(stack++, reflection[2]);
            set_int(stack++, hit_no);

            if (argc >= 15) {
                set_int(stack++, area_kind);
            }

            if (argc == 16) {
                set_int(stack, foot_sound);
            }

            break;
        default:
            return 0;
    }

    return 1;
}
