#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "gfx/gfx.hpp"
#include "ixion_room.hpp"

struct CCPoly;

// Dran's arena from the converter's private arena.json sidecar ("ixion-room-arena" v1) beside an
// unchanged room.json. DC1 authority (Chronicle 572646bf): CDranMapField (dranmapfield.cpp:13-107)
// for the collapsing floor tiles, shot_utils.cpp:75-89 for the hit rule, CFireOmni (fireomni.cpp)
// for the torches, autoCamTrial's PT_CAM (gameloop.cpp:8506-8662) for camera collision,
// CDungeonEventMan::SearchDataSlotPos2 (dungeoneventman.cpp:175-200) for the event01_2 walk-in
// trigger and gameloop.cpp:4262-4270 for the fall. All positions are already in room units.

struct IxionArenaGroup {
    int texture = -1; // room.json texture index
    int material = 0; // MDT material index; keys the alpha track
    std::vector<IxionRoomVertex> vertices; // object-local triangle list
};

struct IxionArenaObject {
    std::string name;
    std::array<float, 3> rest_t{};
    std::array<float, 4> rest_q{1.0f, 0.0f, 0.0f, 0.0f}; // w, x, y, z; R*v
    std::vector<std::array<float, 3>> track_t;            // one key per motion frame
    std::vector<std::array<float, 4>> track_q;
    std::vector<std::pair<int, std::vector<float>>> track_alpha; // material -> alpha per frame
    std::vector<IxionArenaGroup> groups;
};

struct IxionArenaTile {
    int slot = 0;
    float first_frame = 1.0f;
    float speed = 0.6f;
    float done_at = 59.0f;
    std::vector<IxionArenaObject> objects;
    std::vector<IxionRoomTriangle> collision; // world triangles; ground_kind slot+1 in DC1
};

struct IxionArenaMarker {
    std::string name;
    int script = -1;
    bool switch_on = false;
    bool auto_start = false;
    std::array<float, 3> position{};
    float horizontal_radius = 0.0f; // DC1 radius + 10, in room units
    float height_window = 0.0f;     // DC1 |dy| < 40, in room units
};

struct IxionArenaFireLayer {
    float width = 0.0f;
    float height = 0.0f;
    bool flame = false;          // draws the flipbook over the glow
    int flicker_every = 0;       // steps; 0 = steady
    float flicker_max = 1.0f;
    float toward_camera = 0.0f;  // depth bias in room units
};

struct IxionArena {
    std::string room;
    std::string room_json_sha256;
    float scale = 1.0f;
    int initial_state = 3;
    int hit_decrement_ge = 2;
    float fall_below_y = 0.0f;
    int fall_life = 0;
    int fall_script = -1;
    std::vector<IxionArenaTile> tiles; // index == slot
    std::vector<std::array<float, 3>> fires;
    float fire_offset_y = 0.0f;
    std::vector<IxionArenaFireLayer> fire_layers;
    IxionRoomTexture glow;
    IxionRoomTexture flipbook;
    int flipbook_columns = 1;
    int flipbook_frames = 1;
    int flipbook_frame_w = 0;
    int flipbook_frame_h = 0;
    std::vector<IxionRoomTriangle> camera; // PT_CAM triangles, world
    std::vector<IxionArenaMarker> markers;

    // texture_count bounds the group texture indices (room.json's texture list).
    static bool Load(const std::filesystem::path &json_path, size_t texture_count, IxionArena &out,
                     std::string &error);
};

// ---- Pure rule (shared by the runtime and its tests) ----------------------------------------

struct IxionArenaTileState {
    int state = 3;
    float time = 1.0f;
};

// shot_utils.cpp:75-89: a projectile map hit on slot k decrements it while it is >= decrement_ge.
bool IxionArenaApplyHit(std::vector<IxionArenaTileState> &states, int slot, int decrement_ge);
// dranmapfield.cpp:79-107, one DC1 step: 2 -> 1 (collapse starts, motion from its first frame),
// 1 advances by speed and becomes 0 at done_at. Returns the number of tiles that started.
int IxionArenaStepTiles(std::vector<IxionArenaTileState> &states, const std::vector<IxionArenaTile> &tiles);
inline bool IxionArenaTileCollides(int state) { return state > 1; } // dranmapfield.cpp:36
inline bool IxionArenaTileDrawn(int state) { return state != 0; }   // dranmapfield.cpp:72
// Track pose at a motion time (lerp t, shortest-path slerp q), clamped to the track.
void IxionArenaSamplePose(const IxionArenaObject &object, float first_frame, float time, float t[3], float q[4]);
float IxionArenaSampleAlpha(const IxionArenaObject &object, int material, float first_frame, float time);
// Column-major object-to-world matrix: translate(t) * rotate(q).
void IxionArenaPoseMatrix(const float t[3], const float q[4], float out[16]);
// SearchDataSlotPos2: |dy| < height window and horizontal distance <= radius.
bool IxionArenaInsideMarker(const IxionArenaMarker &marker, const float position[3]);

// The event01_2 trigger. DC1 fires on any frame the player is inside while it is switched on;
// the room's spawn is the marker itself (DC1 enters through the boss door, outside it), so the
// trigger arms only once the player has been seen outside since it was switched on.
struct IxionArenaTrigger {
    bool switched_on = false;
    bool armed = false;
};
// True on the frame the trigger fires; it then switches itself off (prog500 EXT(1055)(500, 0)).
bool IxionArenaTriggerStep(IxionArenaTrigger &trigger, bool inside);

// Collision tag carried in CCPoly::parts_no (DC2 gives ground_kind footstep meaning).
inline constexpr uint16_t kIxionArenaPartsTag = 0x2000;
inline constexpr uint16_t kIxionArenaPartsMask = 0xF000;
bool IxionArenaSlotFromParts(int parts_no, int &slot);

std::string IxionArenaSha256Hex(const uint8_t *data, size_t size);

// ---- Runtime seams called from ixion_room.cpp ------------------------------------------------

// Once, after room.json loads. DC2_IXION_ROOM_ARENA unset: arena.json beside room.json when
// present; 0: off; any other value: a sidecar path. The sidecar's room_json_sha256 must match.
void IxionRoomArenaConfigure(const std::filesystem::path &room_json, const IxionRoom &room);
bool IxionRoomArenaLoaded();
bool IxionRoomArenaInstalled();
// True when a configured DC1 donor should spawn at the event-500 trigger instead of at install.
bool IxionRoomArenaDefersDonor();
// Install: every slot back to its initial state (DC1 resets on every floor load) and the trigger
// switched on (event 100 while the boss is alive). Retire/Discard release every GPU handle.
void IxionRoomArenaInstall();
// DRAN-R2: DC1 game int flag 0 (the d01 boss flag), set to 100 by event 510 when Dran dies. At 100
// or more the trigger is switched off now and at every later install (prog100). Process lifetime.
void IxionRoomArenaSetBossFlag(int value);
int IxionRoomArenaBossFlag();
void IxionRoomArenaRetire(const char *reason);
void IxionRoomArenaDiscard();

struct IxionRoomArenaFrameResult {
    bool event_fired = false;
    int event_script = -1;
    bool fell = false;
};
// Frame step, once per frame number (repeat calls for the same frame do nothing). field: ordinary
// FIELD play (tiles and the trigger step only then).
IxionRoomArenaFrameResult IxionRoomArenaFrame(int frame, bool field, bool player_valid, const float player[3]);
// Tile triangles whose state still collides, with their parts_no tags; rebuilt on state change.
void IxionRoomArenaActiveCollision(const std::vector<IxionRoomTriangle> *&triangles,
                                   const std::vector<uint16_t> *&tags);
// PT_CAM triangles in the box as native camera polygons; 0 when not installed.
int IxionRoomArenaCameraPoly(CCPoly *polygons, int capacity, const float bounds_min[3], const float bounds_max[3]);
// Tiles (state != 0) after the room, then the fire billboards.
void IxionRoomArenaDraw(const std::vector<gfx::TextureHandle> &room_textures, const gfx::MeshConstants &base,
                        const gfx::DrawState &base_state);
// A projectile's map hit polygon: tagged tiles take the DC1 hit rule.
void IxionRoomArenaProjectileHit(int parts_no, const char *source);
// Fall handling: the DC1 respawn (event 5) when traced for this room and supported, else spawn.
bool IxionRoomArenaFallRespawn(const IxionRoom &room, float out[3]);
int IxionRoomArenaFallLife();

// Test seams.
void IxionRoomArenaResetForTest();
void IxionRoomArenaAdoptForTest(IxionArena arena);
std::vector<IxionArenaTileState> IxionRoomArenaStatesForTest();
const IxionArenaTrigger &IxionRoomArenaTriggerForTest();
// True when (x, z) has a floor within 20 units of y among the room's collision and colliding tiles.
bool IxionArenaHasSupport(const IxionRoom &room, const std::vector<IxionRoomTriangle> &tiles, const float p[3]);
