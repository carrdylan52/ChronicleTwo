#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

struct CCPoly;
class CActiveMonster;
class CCharacter2;
class CColPrim;
class CEffectScriptMan;
class CMonsterMan;
struct _EFF_SCRIPT;
struct RS_STACKDATA;
class CTreasureBoxManager;

struct IxionRoomTexture {
    std::string name;
    std::filesystem::path relative_file;
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<uint8_t> rgba;
};

struct IxionRoomVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};
    std::array<float, 4> color{};
};

struct IxionRoomVisual {
    int material = -1;
    std::vector<IxionRoomVertex> vertices;
};

struct IxionRoomTriangle {
    std::array<std::array<float, 3>, 3> vertices{};
};

// The pack owns all parsed values and PNG bytes. No JSON node, input buffer, or retail-data pointer escapes.
struct IxionRoom {
    std::string donor_room;
    float unit_scale = 1.0f;
    std::array<float, 3> spawn{};
    std::array<float, 3> enemy_spawn{};
    std::array<float, 3> exit{};
    std::array<float, 3> bounds_min{};
    std::array<float, 3> bounds_max{};
    std::vector<IxionRoomTexture> textures;
    std::vector<IxionRoomVisual> visuals;
    std::vector<IxionRoomTriangle> collision;

    static bool Load(const std::filesystem::path &json_path, IxionRoom &out, std::string &error);
};

// Shared production/test allocation seam: a failed later texture or mesh
// destroys every earlier handle and never publishes a partial bundle.
template <class Texture, class Mesh, class MakeTexture, class MakeMesh,
          class ValidTexture, class ValidMesh, class FreeTexture, class FreeMesh>
bool IxionRoomStageGpuBundle(size_t texture_count, size_t mesh_count,
                             MakeTexture make_texture, MakeMesh make_mesh,
                             ValidTexture valid_texture, ValidMesh valid_mesh,
                             FreeTexture free_texture, FreeMesh free_mesh,
                             std::vector<Texture> &published_textures,
                             std::vector<Mesh> &published_meshes) {
    std::vector<Texture> textures;
    std::vector<Mesh> meshes;
    textures.reserve(texture_count);
    meshes.reserve(mesh_count);
    auto rollback = [&] {
        for (Mesh mesh : meshes) if (valid_mesh(mesh)) free_mesh(mesh);
        for (Texture texture : textures) if (valid_texture(texture)) free_texture(texture);
    };
    for (size_t i=0;i<texture_count;++i) {
        Texture handle=make_texture(i);
        if (!valid_texture(handle)) { rollback(); return false; }
        textures.push_back(handle);
    }
    for (size_t i=0;i<mesh_count;++i) {
        Mesh handle=make_mesh(i);
        if (!valid_mesh(handle)) { rollback(); return false; }
        meshes.push_back(handle);
    }
    published_textures=std::move(textures);
    published_meshes=std::move(meshes);
    return true;
}

// Ixion DC1 roster placement. The imported room has one enemy marker (DC1's d01inter.cfg has no
// monster markers), so kind i of n (i >= 1) stands on a ring about it: radius 60 then 45 DC2 units
// (38.5 and 28.9 DC1, beyond DC1's 25-unit spacing, monstorunit.cpp:550-628), evenly spaced
// angles (or the half-step between them), with a clear line from the marker and floor beneath.
// Writes the floor point and returns true, or returns false when no candidate passes.
bool IxionRoomRosterPoint(const IxionRoom &room, int index, int count, float out[3]);
// (R1 stand-in, superseded at install by IxionRoomArrange; kept for its tests.)

// Ixion R2: DC1's CMonstorUnit::ArrangementPos (Chronicle monstorunit.cpp:550-628) on the imported
// room. DC1 picks a random open cell (SearchiDoPutArea, dungeonparts.cpp:1786-1880), refuses a point
// within 20 of a box/Atla (here: the room's player spawn and exit markers), within 25 of a placed
// monster, or with 3 or more placed monsters within 480, and gives up after 65000 tries. The open
// cells here are the room's reachable flat floor (flood-filled from the player spawn, walls and
// steps tested against the imported collision) whose 8 neighbours are also reachable. All
// distances are DC1 units times the room scale. Spawn 0 stands at the authored enemy marker.
// One room stands for a whole DC1 floor (DC1 spreads 15 over a 20x20-cell map), so a monster the
// strict pass cannot place gets a relaxed pass without the 480 rule; it is marked relaxed.
// Kind choice: spawn i < kinds takes kind i (every listed kind appears); later spawns roll a kind
// as DC1 does (model_no -1), a unique kind at most once. Deterministic for a given seed.
struct IxionRoomArrangeSpawn {
    int kind = -1;
    std::array<float, 3> position{};
    bool relaxed = false;
};
struct IxionRoomArrangeResult {
    std::vector<IxionRoomArrangeSpawn> spawns; // index = spawn slot
    int walkable_cells = 0;
    int reachable_cells = 0;
    int candidate_cells = 0;
    int strict = 0;
    int relaxed = 0;
    int dropped = 0;
};
IxionRoomArrangeResult IxionRoomArrange(const IxionRoom &room, int count, const std::vector<bool> &unique_kinds,
                                        uint32_t seed);
// Seed of one floor lifetime: FNV-1a of "<donor room>:<dungeon>:<floor>:<lifetime>".
uint32_t IxionRoomArrangeSeed(const std::string &room, int dungeon, int floor, uint64_t lifetime);

// Ixion R2 kill memory. A floor lifetime runs from one native CMonsterMan::Initialize (the floor's
// monster population reset, DC2's counterpart of DC1's per-floor _LOAD_MONSTOR -> BtLoadMonstor +
// BtArrengeMonstor, btsysscript.cpp:638-641, dun/gameloop.cpp:6487-6490) to the next. A donor whose
// DC1 death paid is remembered by its kill key for the rest of that lifetime: a retire and reinstall
// on the same floor neither respawns nor pays it again. A new lifetime (floor re-entry) starts with
// a fresh arrangement, as DC1's does.
// Key: "<donor room>:<dungeon>:<floor>:spawn<NN>:<DC1 id>" (shared with the Ledger lane).
std::string IxionRoomDonorKillKey(const std::string &room, int dungeon, int floor, int spawn_slot,
                                  const char *donor_id);
// Called by the DC1 death native once its payout ran (pointer and incarnation identify the spawn).
void IxionRoomRecordDonorKill(const CActiveMonster *monster, const char *donor_id);
bool IxionRoomKillRemembered(const std::string &key);
// Key of a live room-owned donor, or empty.
std::string IxionRoomDonorKillKeyFor(const CActiveMonster *monster);
uint64_t IxionRoomFloorLifetime();
// DRAN-R2: a donor's DC1 run-script request (111) under its port-owned id. The room never writes
// DC2's event_no for it, so it neither starts a DC2 event nor retires the installation. Port id
// 30510 (DC1 event 510, Dran defeated) sets the arena boss flag to 100, which keeps the event-500
// trigger off (d01 prog100 switches it on only while the flag is below 100).
void IxionRoomDonorEvent(int port_event, int dc1_event, const CActiveMonster *monster);

// Convert in-bounds room triangles into the native collision record consumed by CMap queries.
int IxionRoomCollision(const IxionRoom &room, const float bounds_min[3], const float bounds_max[3],
                      CCPoly *native_polygons, int capacity);
// The same, with extra triangles (Ixion arena tiles) sharing the candidate order; each extra
// polygon carries its tag in CCPoly::parts_no.
int IxionRoomCollision(const IxionRoom &room, const float bounds_min[3], const float bounds_max[3],
                      CCPoly *native_polygons, int capacity, const std::vector<IxionRoomTriangle> &extra,
                      const std::vector<uint16_t> &extra_tags);

// GS vertex colours are 7-bit modulation values: 0x80 is neutral white.
uint8_t IxionRoomGsColor(float modulation);
std::array<uint8_t, 4> IxionRoomModulateTexel(std::array<uint8_t, 4> texel,
                                             std::array<float, 4> modulation);

// Pure lifecycle helpers, shared by the runtime and its tests.
struct IxionRoomFloorIdentity {
    bool known = false;      // a dungeon loop with live save data is running
    int status_dungeon = -1; // DngStatus.dungeon_no
    int stage = -1;          // DngSaveDataDungeon->stage_id
    int floor = -1;          // DngSaveDataDungeon->floor_id[stage]
};
bool IxionRoomTargetMatches(int selected_dungeon, int selected_floor, const IxionRoomFloorIdentity &current);

struct IxionRoomBoundaryInput {
    bool enabled = false;
    bool installed = false;
    bool owner_changed = false;
    bool target_matches = false;
    bool native_ready = false;
};
struct IxionRoomBoundaryDecision {
    bool retire = false;
    bool install = false;
};
IxionRoomBoundaryDecision IxionRoomDecideBoundary(const IxionRoomBoundaryInput &input);

// Native CAutoMapGen cell of a world x/z, with the native negative clamp and
// an explicit upper bound. False when the cell would lie outside the grid.
bool IxionRoomNativeGridCell(float x_position, float z_position, int grid_w, int grid_h, float cell_w, float cell_d,
                             int &x, int &z);
bool IxionRoomBoundsFitNativeGrid(const std::array<float, 3> &bounds_min, const std::array<float, 3> &bounds_max,
                                  int grid_w, int grid_h, float cell_w, float cell_d);

// Opt-in through DC2_IXION_ROOM and DC2_IXION_ROOM_FLOOR="dungeon:floor".
// DC2_IXION_ROOM_MODE=inspect|encounter (default encounter) and
// DC2_IXION_ROOM_ENCOUNTER=0|1 (default 0 for inspect, 1 for encounter).
// The selection is latched once, then applies only to that native dungeon floor.
int IxionRoomAppendRuntimeCollision(CCPoly *polygons, int capacity, const float bounds_min[3],
                                    const float bounds_max[3], int dungeon, int floor);
bool IxionRoomRuntimeActive(int dungeon, int floor);
int IxionRoomDrawRuntime(int dungeon, int floor);
void IxionRoomObserveDungeonLoop();
// Post-frame boundary (MainLoop, after mgEndFrame) and dungeon-loop entry
// (MainLoop, after LoopInit) for LOOP_DUNGEON.
void IxionRoomFrameBoundary();
void IxionRoomLoopEntry();
// The single retirement entry point for every departure; idempotent.
void IxionRoomRetire(const char *reason);
// In-frame 2D overlay: room label, exit distance and the Cross exit prompt.
void IxionRoomDrawOverlay();
bool IxionRoomRuntimeOwnedBy(const void *scene, const void *map);
// Called before LoopInit for every loop: a still-installed room there is an
// invariant violation and is discarded without touching old actors.
void IxionRoomBeforeLoopInit();
// N5: keeps the exit-confirming Cross out of native action input.
void IxionRoomConsumeExitPress();
// F4 policy hooks used by the CMonsterMan::ThinkHost port override.
bool IxionRoomMonsterSuspended(CActiveMonster *monster);
// Pure query (no parking side effect) for the CollisionCheck push filter.
bool IxionRoomMonsterParked(const CActiveMonster *monster);
// N1/N6/N7: ends the effect scripts whose recorded source is this native actor
// (owner = its chara_type; the mutable user_id is not consulted) and exactly the
// collision primitives handed out to those scripts or to the actor's attack
// windows that have not been handed out again since. Never a wildcard sweep.
void IxionRoomEndOwnedAttacks(int owner, const CActiveMonster *monster = nullptr);
// Exact ownership bookkeeping, fed by the production allocation sites:
// CEffectScriptMan::CreateEffSpt (port), CColPrimMan::GetPrim, _COLPRIM_CREATE
// and _ES_CREATE (ixion_room_ownership.cpp) and the ThinkHost attack windows.
void IxionRoomNoteEffectCreated(const _EFF_SCRIPT *effect, int group);
// Invalidation stays active outside installation and never creates claims.
void IxionRoomForgetEffect(const _EFF_SCRIPT *effect);
// End exact live borrowers and guard the raw colprim before native arena release.
// With no adapter origin, leave native state and owner-checked deletion unchanged.
void IxionRoomBeforeEffectDelete(_EFF_SCRIPT *effect);
void IxionRoomClearAttackClaims();
void IxionRoomSetEffectParent(const _EFF_SCRIPT *parent);
void IxionRoomNotePrimHandedOut(const CColPrim *prim);
void IxionRoomNoteEffectPrim(const CColPrim *prim, const _EFF_SCRIPT *script);
void IxionRoomNoteMonsterPrim(const CColPrim *prim, const CActiveMonster *monster);
// Recorded source/row of a live script, -1 when unknown.
int IxionRoomEffectSource(const _EFF_SCRIPT *script);
int IxionRoomEffectRow(const _EFF_SCRIPT *script);
size_t IxionRoomEffectOriginCountForTest();
// True when prim is still the incarnation handed out to this script/monster.
bool IxionRoomPrimHeldByEffect(const CColPrim *prim, const _EFF_SCRIPT *script);
bool IxionRoomPrimHeldByMonster(const CColPrim *prim, const CActiveMonster *monster);
// The effect script whose operand stack holds an external command's arguments.
_EFF_SCRIPT *IxionRoomRunningEffect(CEffectScriptMan *manager, const RS_STACKDATA *stack);
// Retained native event policy: true while the room owns the live floor; the
// IsEventRun override then skips every position-keyed native field event.
bool IxionRoomGatesNativeFieldEvents();
// N8: every CMonsterMan::Initialize (debug enemy reload/reset, _DEL_MONSTER,
// LoadMonsterFile) retires an installed room before the actors are reset.
void IxionRoomBeforeMonsterReset(const char *reason);
// N9: boxes are never parked; a live box whose 40-unit pickup radius reaches
// the donor bounds refuses the installation. Returns the overlapping index or -1.
int IxionRoomOverlappingTreasureBox(CTreasureBoxManager *boxes, const std::array<float, 3> &bounds_min,
                                    const std::array<float, 3> &bounds_max);
// N10/N12: retained native event policy at monster dispatch and scene VM start.
// Monster requests (IsEventRun override): while the room owns the floor a
// parked monster's queued request stays queued, deferred until retirement
// returns that monster to native play; other slots are consumed natively.
int IxionRoomTakeMonsterEvent(CMonsterMan *manager);
// Scene scripts (RunEvent override, with an early EventScriptSetup check):
// every dungeon VM start, including debug/load continuations. A supported event runs
// with the room; any other event retires the room first (native player
// position restored) and refuses reinstallation for this dungeon-loop entry.
bool IxionRoomNativeEventSupported(int event_no);
void IxionRoomBeforeNativeEvent(int event_no);
bool IxionRoomEventRefusalActive();
// N10: a box posted while installed (any retained script, _POST_TREASURE_BOX
// included) whose pickup radius reaches the donor bounds retires the room.
void IxionRoomAfterTreasureBoxPosted(CTreasureBoxManager *boxes, int index);
// N11: every native spawn (CMonsterMan::SetActiveMonster override) starts a new
// incarnation of a reused scene character. Ownership records are (address,
// incarnation) pairs: the old selected-enemy exemption, its saved native state
// and any parked record for that address are dropped before the slot is
// overwritten, and the new incarnation is parked when spawned while installed.
void IxionRoomBeforeMonsterSpawn(CActiveMonster *slot_actor);
void IxionRoomAfterMonsterSpawn(CActiveMonster *monster);
uint64_t IxionRoomMonsterIncarnation(const CActiveMonster *monster);
bool IxionRoomInstalled();
struct IxionLedgerKill;
// Snapshot an installed encounter's identity while its incarnation is owned.
bool IxionRoomCaptureLedgerKill(const CActiveMonster *monster, IxionLedgerKill &kill);
// Test seam: adopt an installed state without GPU or a live floor, so tests
// can drive the production RetireScene/discard paths.
struct IxionRoomTestInstall {
    std::string ledger_room;
    int ledger_dungeon = -1;
    int ledger_floor = -1;
    void *scene = nullptr;
    void *map = nullptr;
    CActiveMonster *enemy = nullptr;
    float enemy_saved_position[4]{};
    float enemy_marker[3]{};
    bool enemy_pending = false;
    std::vector<CActiveMonster *> parked;
    std::vector<CActiveMonster *> roster; // further DC1 roster donors owned by the room
    std::vector<std::string> roster_keys;  // their kill keys (R2), parallel to roster when given
    std::string enemy_key;                 // the selected donor's kill key (R2)
    bool navigation_suspended = false;
    int saved_navi_enable = 0;
    int saved_minimap_enable = 0;
    int saved_navi_depth = 4;
    bool player_native_saved = false;
    float player_saved_position[4]{};
    // N10 tests: treat the adopted scene as the selected live floor, with these donor bounds.
    bool target_matches = false;
    std::array<float, 3> bounds_min{};
    std::array<float, 3> bounds_max{};
};
void IxionRoomTestAdoptInstalled(const IxionRoomTestInstall &state);
void IxionRoomSetThinkingMonster(const CActiveMonster *monster);
bool IxionRoomOwnsNavigationQuery();
CActiveMonster *IxionRoomRuntimeEnemy();
int IxionRoomRuntimeEnemyCount();
bool IxionRoomRedirectPosition(CCharacter2 *actor,float position[4]);
void IxionRoomActionPositionBoundary(CCharacter2 *actor);
