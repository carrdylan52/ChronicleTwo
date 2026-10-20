#include "common.h"
#include "mw_runtime.h"

#include <libvu0.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json.hpp>

#include "actionchara.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "colprim.hpp"
#include "ixion_room_arena.hpp"
#include "dng_effect.hpp"
#include "effscript.hpp"
#include "dng_event.hpp"
#include "gamedata.hpp"
#include "userdata.hpp"
#include "dataread_port.hpp"
#include "dng_main.hpp"
#include "gameutil.hpp"
#include "ixion_dc1_monster.hpp"
#include "ixion_dc1_sound.hpp"
#include "ixion_room.hpp"
#include "ixion_monica_faithful.hpp"
#include "map.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mglib_port.hpp"
#include "monster.hpp"
#include "runscript.hpp"
#include "scene.hpp"
#include "snd_mngr.hpp"
#include "cameracontrol.hpp"
#include "dng_status.hpp"
#include "padcontrol.hpp"
#include "mainloop.hpp"
#include "sound.hpp"
#include "actscript.hpp"
#include "menucommon.hpp"
#include "nd_meswin.hpp"
#include "savedata.hpp"

// DC2's monster callbacks publish the monster whose script runs (runscript_opcodes.cpp).
extern CActiveMonster *nowMonster;
// DC2's own death natives (runscript_opcodes.cpp:3184, :3344) and amulet check (actionchara.cpp:2874).
int _SET_DEAD_START(RS_STACKDATA *args, int argc);
int _SET_DEAD_OFF(RS_STACKDATA *args, int argc);
int CheckAmuletAvoid(int item_no);
int CheckEquipSetItem(int item_no);
void GuardEffectSet(CScene *scene, float *point);
void HitEffectSet(CScene *scene, float *point);

namespace ixion::dc1rt {
namespace {

// Room and pack scale (DC1 units -> DC2 units), the same k the room and monster converters use.
constexpr float kScale = 1.5584790683f;
// Monster kind number of the Ixion row; outside the monster book (CountKill bounds it) and the
// native table, so no native lookup can alias it.
constexpr s16 kDonorId = 0x7000;
// DC1 program numbers (Chronicle ps2/src/monstorunit.cpp:1980-2001).
constexpr int kDc1Setup = 1;
constexpr int kDc1Main = 100;
constexpr int kDc1Hit = 110;
constexpr int kDc1Death = 120;
// The DC1 set-up program is renumbered out of DC2's INIT (100) so SetActiveMonster's INIT call,
// made before the DC1 table is installed, finds no program; the spawn runs it under the DC1 table.
constexpr int kSetupProgram = 9001;
constexpr float kPi = 3.14159265358979f;

// Per-donor retail properties; behaviour, motions and paths below are independent of the id.
// Chronicle ps2/src/monstorunit.cpp:139 (Ghost), :263 (Opar).
struct Donor {
    std::string id;
    std::string name;
    int life;
    int defense;
    float radius;
    bool no_blow;
    int shot_effect;
    int experience = 3;
    int money = 6;
    s16 drops[3] = {0, 182, 222};
    // DC1 MONSTER_KIND (Chronicle monstorunit.hpp:29-32): 0 normal, 2 no lock-on, 3 small mimic,
    // 4 large mimic. ArrangementPos places a kind other than 0 and 3 at most once
    // (monstorunit.cpp:610-616); a mimic kind starts hidden in its chest (monstorunit.cpp:2506-2518).
    int dc1_kind = 0;
};
// The DC2 program number given to DC1's prog_50 (the mimic's appearance, monstorunit.cpp:1997-1999).
constexpr int kRevealProgram = 9050;
constexpr int kDc1Reveal = 50;
const Donor kDonors[] = {
    // Chronicle monstorunit.cpp:263: experience 3, money 5 (the stand-in row had carried 6).
    {"e104a", "Opar", 28, 1, 15.0f, true, 29, 3, 5, {0, 182, 222}},
    {"e42a", "Ghost", 15, 0, 3.6f, false, 9, 3, 5, {0, 0, 0}},
    // Chronicle monstorunit.cpp:75. Werewolf has no projectile and allows knockback.
    {"e07a", "Werewolf", 180, 5, 7.5f, false, -1, 12, 15, {0, 0, 0}},
};
// Supported DC1 shot effects (Chronicle ps2/include/monstorunit_effect_data.inc): 5 f_boll_3 (:11,
// flight phase 1: speed 1.5, radius 3.8, life 120, damage 17, kind 3), 9 magic_noroi (:19, phase 0)
// and 29 nebaneba_b (:60). Straight flight with no gravity or homing (shot_effect.cpp:392-480).
struct ShotEffect {
    int   effect;
    float speed;  // DC1 units per frame
    float radius; // DC1 units
    int   life;
    int   damage; // used only when the script omits the damage override
    bool  goo;
    bool  blow;   // hit kind 3 (launch) rather than kind 2
    // DC1 phase 2 (shot_effect.cpp:248-268): a map hit stops a flying effect and its impact motion
    // keeps hitting with radius[2] until the motion ends. 0 = the shot simply ends at the wall.
    float impact_radius = 0.0f; // DC1 units
    int   impact_frames = 0;
    bool  fire = false;         // drawn with DC2's fire after-hit flames (stand-in for the DC1 model)
};
const ShotEffect kShotEffects[] = {
    {5, 1.5f, 3.8f, 120, 17, false, true},
    {9, 1.1f, 1.9f, 160, 16, false, false},
    // 13 f_boll_2, Dran's fireball (monstorunit_effect_data.inc:29): motion[0] = -1, so Set starts it
    // in phase 1 (shot_effect.cpp:410-415): speed 2.5, radius 4, life 180, damage 35, hit kind 3.
    // Phase 2 is motion 1 with speed 0 and radius 20; its length is the f_boll_2 model's motion 1,
    // which is not traced, so 20 frames stand in. end_effect is -1, so bomb_scale/bomb_damage
    // (1.5, 20) are never used (shot_effect.cpp:228-235). SE 1729 is a DC1 sound, absent in DC2.
    {13, 2.5f, 4.0f, 180, 35, false, true, 20.0f, 20, true},
    {29, 0.4f, 5.0f, 180, 15, true, false},
};
const ShotEffect *FindShotEffect(int effect) {
    for (const ShotEffect &entry : kShotEffects) if (entry.effect == effect) return &entry;
    return nullptr;
}

const Donor *ResolveDonor(std::string_view id) {
    for (const Donor &donor : kDonors) {
        if (id == donor.id) return &donor;
    }
    // Other converted DC1 ids supply retail properties in a private overlay descriptor. No
    // guessed Opar stats or arbitrary path fragments are accepted. Behaviour still needs supported natives.
    const std::string key(id);
    if (key.empty() || key.size() > 12 || key.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789") != std::string::npos)
        return nullptr;
    static std::map<std::string, Donor> profiles; // stable addresses for the per-profile row cache
    if (auto found = profiles.find(key); found != profiles.end()) return &found->second;
    const std::string filename = "dungeon/monster/ix_" + key + ".dc1.json";
    const auto path = DataReadFind(filename.c_str());
    if (!path) return nullptr;
    try {
        std::ifstream stream(*path);
        const auto json = nlohmann::json::parse(stream);
        if (json.at("format") != "ixion-dc1-monster" || json.at("version") != 1) return nullptr;
        Donor donor{key, json.at("name").get<std::string>(), json.at("life").get<int>(),
                    json.at("defense").get<int>(), json.at("body_radius").get<float>(),
                    json.at("no_blow").get<bool>(), json.at("shot_effect").get<int>(),
                    json.at("reward_exp").get<int>(), json.at("reward_money").get<int>(), {0, 0, 0}};
        // Optional DC1 kind (Chronicle MonstorTable column; absent means normal).
        const std::string kind = json.value("kind", std::string("normal"));
        donor.dc1_kind = kind == "normal" ? 0 : kind == "no_lock_on" ? 2 : kind == "mimic_small" ? 3
                         : kind == "mimic_large" ? 4 : -1;
        if (donor.dc1_kind < 0) return nullptr;
        if (donor.name.empty() || donor.name.size() >= 32 || donor.life <= 0 || donor.life > 1000000 ||
            donor.defense < 0 || donor.defense > 255 || !std::isfinite(donor.radius) || donor.radius <= 0 ||
            donor.radius > 1000 || donor.experience < 0 || donor.experience > 65535 || donor.money < 0 ||
            donor.money > 65535 || (donor.shot_effect != -1 && FindShotEffect(donor.shot_effect) == nullptr))
            return nullptr;
        std::fprintf(stderr, "ixion dc1 descriptor selected id=%s life=%d radius=%.2f shot_effect=%d dc1_kind=%d\n",
                     key.c_str(), donor.life, donor.radius, donor.shot_effect, donor.dc1_kind);
        return &profiles.emplace(key, std::move(donor)).first->second;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "ixion dc1 descriptor refused id=%s: %s\n", key.c_str(), error.what());
    }
    return nullptr;
}

// DC2_IXION_ROOM_DONOR_MONSTER is one id; DC2_IXION_ROOM_DONOR_ROSTER is a comma-separated roster. Kind i
// of the roster is the private DC2 row 0x7000 + i; a single id is kind 0, exactly as before. The
// whole roster is refused when any id is unknown, repeated or the list is longer than kRosterMax.
constexpr int kRosterMax = 8;
bool IsDonorId(int id) { return id >= kDonorId && id < kDonorId + kRosterMax; }
const std::vector<const Donor *> &Roster() {
    static std::string cached;
    static bool cached_valid = false;
    static std::string refused_logged;
    static std::vector<const Donor *> roster;
    const char *single = std::getenv("DC2_IXION_ROOM_DONOR_MONSTER");
    if (single != nullptr && std::strchr(single, ',') != nullptr) {
        static std::string refused_single;
        if (refused_single != single) {
            refused_single = single;
            std::fprintf(stderr, "ixion dc1 DC2_IXION_ROOM_DONOR_MONSTER refused list: %s; use DC2_IXION_ROOM_DONOR_ROSTER\n", single);
        }
        roster.clear();
        cached_valid = false;
        return roster;
    }
    const char *list = std::getenv("DC2_IXION_ROOM_DONOR_ROSTER");
    const char *text = list != nullptr ? list : single;
    const std::string value = text != nullptr ? text : "";
    if (cached_valid && value == cached) return roster;
    roster.clear();
    cached_valid = false;
    if (text == nullptr) return roster;
    std::vector<const Donor *> parsed;
    std::string_view rest(value);
    bool ok = true;
    while (ok) {
        const size_t comma = rest.find(',');
        const std::string_view token = rest.substr(0, comma);
        const Donor *donor = ResolveDonor(token);
        ok = donor != nullptr && parsed.size() < (size_t) kRosterMax &&
             std::find(parsed.begin(), parsed.end(), donor) == parsed.end();
        if (ok) parsed.push_back(donor);
        if (comma == std::string_view::npos) break;
        rest.remove_prefix(comma + 1);
    }
    if (!ok) {
        // Descriptor lookups depend on the overlay index, so a refusal is not cached.
        if (refused_logged != value && value.find(',') != std::string::npos) {
            refused_logged = value;
            std::fprintf(stderr, "ixion dc1 roster refused: %s (unknown, repeated or more than %d ids)\n", value.c_str(),
                         kRosterMax);
        }
        return roster;
    }
    roster = std::move(parsed);
    cached = value;
    cached_valid = true;
    if (roster.size() > 1) {
        std::string ids;
        for (size_t i = 0; i < roster.size(); ++i) ids += (i ? "," : "") + roster[i]->id;
        std::fprintf(stderr, "ixion dc1 roster selected kinds=%zu ids=%s rows=0x%x..0x%x\n", roster.size(), ids.c_str(),
                     kDonorId, kDonorId + (int) roster.size() - 1);
    }
    return roster;
}
const Donor *SelectedDonor() {
    const auto &roster = Roster();
    return roster.empty() ? nullptr : roster[0];
}
const Donor *DonorForId(int monster_id) {
    const auto &roster = Roster();
    return IsDonorId(monster_id) && monster_id - kDonorId < (int) roster.size() ? roster[monster_id - kDonorId] : nullptr;
}

struct DamageSphere {
    char  frame[16] = {};
    float radius = 0.0f;
    float start = 0.0f;
    float end = 0.0f;
    int   damage = 0;
    int   flags = 0;
    int   kind = 0;
    float angle = 0.0f;
    int   slot = -1; // DC2 damage[] entry
};

// Friendly is the playtest policy (wave4-balance); faithful retains the original 70-HP basis.
constexpr float kDc1PlayerHp = 146.0f;
float BalanceHp() {
    const char *mode = std::getenv("DC2_IXION_DC1_BALANCE");
    const std::string_view value = mode != nullptr ? std::string_view(mode) : std::string_view();
    if (mode != nullptr && value != "faithful" && value != "friendly") {
        // Precheck P3: an unrecognised value still means friendly, but now says so once per value.
        static std::string warned;
        if (warned != value) {
            warned = std::string(value);
            std::fprintf(stderr, "ixion dc1 balance WARNING: DC2_IXION_DC1_BALANCE=\"%s\" is not faithful or friendly; using friendly\n",
                         mode);
        }
    }
    return value == "faithful" ? 70.0f : kDc1PlayerHp;
}
// Toan's starting defence (dngstatusdata.cpp:893), taken off every DC1 hit (gameloop.cpp:7136-7147);
// DC2 then applies its own defence and +1-3 (actionchara.cpp:2988-2999), roughly cancelling out.
constexpr int   kDc1PlayerDefense = 3;
// Goo (Opar's effect 29) is a 65% roll that halves the walk (gameloop.cpp:7267-7283).
constexpr int   kGooChance = 65;
// DC2_IXION_DC1_GOO_CHANCE overrides the roll (0 = no goo), for a same-input movement baseline.
int GooChance() {
    const char *text = std::getenv("DC2_IXION_DC1_GOO_CHANCE");
    return text != nullptr && *text != 0 ? std::atoi(text) : kGooChance;
}
// DC1 goo has no timer (gameloop.cpp:6068-6086 counts only freeze and stamina). DC2's slow is timed
// (status_time is s16); the longest DC2 time stands in for "until cured".
constexpr int   kGooTime = 32767;

struct Shot {
    bool      live = false;
    float     pos[4] = {};
    float     vel[4] = {}; // DC2 units per frame
    int       life = 0;
    int       damage = 0;
    int       owner = 0;
    int       slot = -1;   // owning donor instance
    CColPrim *prim = nullptr;
    bool      death = false;
    int       effect = 29;
    float     radius = 5.0f;
    // Ixion Monica faithful (DC2_IXION_MONICA_FAITHFUL): the player fired it. Owner 0, monsters only, no
    // goo roll, and the donor's lifecycle (respawn, despawn, forget) leaves it alone.
    bool      player_owned = false;
    int       hit_chara = -1; // scene character id of the monster CheckHit reported, for the log
    int       hit_life = -1;  // that monster's life at the hit, before DC2's damage step
    // Monica faithful round 2: DC2's own goo model (effect script ねばねばＬ) pinned to the ball. The slot is in
    // the player's effect manager (group 0); the pointer is checked against the slot every tick.
    int          fx_slot = -1;
    _EFF_SCRIPT *fx_script = nullptr;
    int          fx_ticks = 0;
    bool      impact = false; // DC1 phase 2: stopped at a map hit, hitting with the impact radius
    float     impact_radius = 0.0f;
    int       impact_frames = 0;
    bool      fire = false;
};

// One hurt sphere from _SET_BODY_COL: a DC2 HIT entry and its optional DC1 motion window.
struct BodyCol {
    int   entry = -1; // index into entry_object
    float start = 0.0f;
    float end = 0.0f;
};

// The subset of DC1's MONSTOR record (Chronicle monstorunit.hpp) that a donor's natives touch.
// One per live donor instance (DC1 keeps one record per monster slot, NowMonstorUnit->monster[n]).
struct State {
    CActiveMonster *monster = nullptr;
    const Donor    *donor = nullptr; // kind of this instance (roster entry for its row id)
    int             owner = -1;      // chara_type at install, kept so retirement never reads the actor
    float           turn_target[3] = {};
    float           turn_speed = 0.0f;
    float           move_dir[3] = {};
    float           move_speed = 0.0f; // DC1 units per frame
    bool            falls = true;
    bool            free_fall = false;
    bool            vertical_control = false; // Opar retains its existing DC2 grounding path.
    float           before_move_y = 0.0f;
    float           shadow_length = 1.0f;
    int             motion_no = 0;
    int             motion_flags = 0;
    float           motion_speed = -1.0f; // <= 0: the key's own step
    bool            motion_pending = false;
    float           alpha = 128.0f;
    float           alpha_step = 0.0f;
    float           clip = 0.0f;
    DamageSphere    damage[16];
    int             damage_count = 0;
    int             last_damage = -1;
    int             last_prog = -2;
    int             frames = 0;
    bool            first_call[256] = {};
    bool            window_open[16] = {};
    bool            dead_done = false;
    bool            dead_pending = false; // killed, waiting for DC2's removal step to count it
    int             kills_before = -1;
    int             hits_taken = 0;
    int             last_life = -1;
    int             last_hit_damage = -1; // damage this monster dealt to the player since SET_MOTION
    int             guard_start[3] = {};
    int             guard_end[3] = {};
    int             guard_count = 0;
    int             frames_since_attack = 3600;
    bool            guard_pending = false;
    int             guard_life_before = 0;
    int             guard_frame = 0;
    int             guard_slot = 0;
    int             guard_incoming = 0;
    int             guard_kind = 0;
    float           guard_motion_frame = 0;
    BodyCol         body_cols[16];
    int             body_col_count = 0;
    int             mov_col_count = 0;
    float           lockon_scale[2] = {1.0f, 1.0f};
    float           loop_start = 0.0f;
    float           loop_end = 0.0f;
    int             loop_step = 0;
    int             loop_id = -1;
    // DC1 MONSTOR sound record (Chronicle monstorunit.hpp): native 140's 16 frame windows, kept
    // from set-up until the floor is cleaned (monstorunit.cpp:2355-2358), each with a 10-frame
    // cooldown (SoundCheck, :724-735).
    float           snd_start[16] = {};
    float           snd_end[16] = {};
    int             snd_id[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    int             snd_cooldown[16] = {};
    // The loop sequence (DC1 se_seq entry, snd.cpp:1242-1282): started on a fresh refresh, kept
    // alive by each in-window refresh, stopped `length` frames after the last one.
    int             seq_id = -1;
    int             seq_step = 0;
    int             seq_length = 0;
    // DC1 `revealed` (monstorunit.hpp:115): 0 while a mimic hides in its chest. Hidden donors run no
    // script, take no damage and cannot be locked on (monstorunit.cpp:1945 gates the whole step).
    bool            hidden = false;
    u32             hidden_attrib = 0; // attrib bits the hide added, removed again at the reveal
    // 108 _STATUS_SET_COL_OFF (runscript_opcodes.cpp:557): monster[n].collision_off_timer.
    int             col_off = 0;
    u32             col_off_attrib = 0; // attrib bits the timer added
    // 116 _STATUS_SET_LOCKON_DIST (:691): monster[n].lock_range, DC1 units (default 120, :2310).
    float           lock_range = 120.0f;
    // 115 _STATUS_GET_DMG_ID (:687): monster[n].last_hit_id = owner * 10 + attack (monstorunit.cpp:1279-1290).
    int             last_hit_id = -1;
    // 134 _SET_BODY_COL_PARA (:804-821): per hurt sphere, body_parameter[0..9] (read by nothing) and
    // parameter[0..5], the percent of damage taken from DC1 attacker (owner) 0-5 (monstorunit.cpp:1523).
    int             body_parameter[16][10] = {};
    int             attacker_percent[16][6] = {};
    int             last_body_col = -1; // DC1 bak_ColNo
    // DC1 runs prog_110 for every accepted hit, damage or not (monstorunit.cpp:1332, :1976-1980);
    // DC2 gives a zero-damage hit no reaction (monster.cpp:2268-2285). Set by IxionDc1NoteBodyHit.
    bool            dc1_hit_request = false;
    int             lock_frames = 0; // thinks spent as the player's lock-on target (capture only)
};

// DC1 runs at most 16 monsters per unit (Chronicle monstorunit.cpp:1655); DC2 has 24 slots.
constexpr int kInstanceMax = 16;
State g_states[kInstanceMax];
Shot  g_shots[16];
bool  g_captured[24] = {}; // 13-17: DRAN-R2 (gun hit, melee hit, death, lock-on framing, volley shot)
bool  g_paused = false;
int   g_pause_trace_frame = -1;
struct PausedPrim {
    CColPrim *prim;
    int target;
    int owner;
    DAMAGE_PARAM *param;
    int slot; // owning donor instance
};
std::vector<PausedPrim> g_paused_prims;
// Primitives live in a static pool. Keep their identities separately from arena-owned actors,
// so stale retirement never dereferences the old monster's damage[] array.
std::vector<PausedPrim> g_owned_prims;
int SlotOf(const State *state) { return state != nullptr ? (int) (state - g_states) : -1; }
void TrackPrim(CColPrim *prim, int slot) {
    if (prim == nullptr) return;
    for (PausedPrim &entry : g_owned_prims) {
        if (entry.prim == prim) { entry = {prim, prim->target, prim->owner, prim->param, slot}; return; }
    }
    g_owned_prims.push_back({prim, prim->target, prim->owner, prim->param, slot});
}
int LiveInstances() {
    int live = 0;
    for (const State &state : g_states) live += state.monster != nullptr ? 1 : 0;
    return live;
}
int (*g_player_table[ACTION_EXT_FUNC_MAX])(RS_STACKDATA *, int);
int (**g_original_player_table)(RS_STACKDATA *, int) = nullptr;
void RestorePlayerTable() {
    if (MainChara__2 != nullptr && MainChara__2->script.ext_func_table == g_player_table)
        MainChara__2->script.ext_func(g_original_player_table, MainChara__2->script.ext_func_num);
}
void PlayerAttackStarted() {
    if (action_info.chara != MainChara__2 || LiveInstances() == 0 || IxionDc1UpdatePause()) return;
    // DC1 keeps one host attack age (BtActStatus.frames_since_attack); every donor observes it.
    for (State &state : g_states) if (state.monster != nullptr) state.frames_since_attack = 0;
}
template<int Slot>
int ObservePlayerAttack(RS_STACKDATA *stack, int argc) {
    const bool charge_start = Slot != ACTION_EXT_SET_ACCUME_FLAG ||
        (argc == 1 && rsGetStackInt(stack) == ACCUME_EFFECT_MODE_START);
    const int result = g_original_player_table[Slot](stack, argc);
    if (result && charge_start) PlayerAttackStarted();
    return result;
}
int             g_kind2_observe_end = -1;
// Private damage rows (never the shared Damage_Param_Table): copies of the stock blow and goo rows.
DAMAGE_PARAM g_blow_param;
DAMAGE_PARAM g_goo_param;
DAMAGE_PARAM g_normal_param;
bool         g_params_ready = false;
// Backing store of the donors' own memory stack (models, scripts, sound banks): 2 MiB, shared by
// every roster kind on the floor.
constexpr int kDonorQuadwords = 131072;
u_long128     g_donor_buffer[kDonorQuadwords];
struct DonorRow { BASE_MONSTER_TBL row; bool ready = false; };
std::map<std::pair<const Donor *, int>, DonorRow> g_rows;
// Patched copies of each kind's DC1 STB, 16-byte aligned. Built once per kind and never
// reassigned, so a live instance's script pointer stays valid while other kinds spawn.
std::map<std::string, std::vector<u_long128>> g_programs;
// Kinds whose STB has DC1 prog_50 (the mimic appearance), mapped to kRevealProgram.
std::map<std::string, bool> g_has_reveal;

// Optional one-shot phase evidence, written by mgEndFrame after this frame renders.
void Capture(int slot, const char *phase) {
    const char *dir = std::getenv("DC2_IXION_DC1_CAPTURE_DIR");
    if (dir == nullptr || *dir == 0 || g_captured[slot]) return;
    g_captured[slot] = true;
    char name[80];
    std::snprintf(name, sizeof name, "%s-f%d.png", phase, mgPortFrameCount() + 1);
    mgPortQueueScreenshot(std::filesystem::path(dir) / name);
}

// Pointer identity only: never dereferences the actor (it may be gone after an arena reset).
State *Slot(const CActiveMonster *monster) {
    if (monster == nullptr) return nullptr;
    for (State &state : g_states) if (state.monster == monster) return &state;
    return nullptr;
}

State *Find(const CActiveMonster *monster) {
    // The slot object outlives the monster; the kind number guards against a later occupant.
    return monster != nullptr && IsDonorId(monster->monster_id) ? Slot(monster) : nullptr;
}

const char *DonorName(const State *state) { return state != nullptr && state->donor != nullptr ? state->donor->id.c_str() : "-"; }

State *Now() { return Find(nowMonster); }

int GetInt(RS_STACKDATA *s) { return s->type == RS_FLOAT ? (int) s->val.f : s->val.i; }
float GetFloat(RS_STACKDATA *s) { return s->type == RS_INT ? (float) s->val.i : s->val.f; }
const char *GetString(RS_STACKDATA *s) { return s->type == RS_STR ? s->val.s : ""; }
void SetInt(RS_STACKDATA *s, int value) {
    if (s->type == RS_PTR) s->val.p->val.i = value;
}
void SetFloat(RS_STACKDATA *s, float value) {
    if (s->type == RS_PTR) s->val.p->val.f = value;
}

void FirstCall(int number, const char *name, const char *status) {
    State *state = Now();
    if (state == nullptr || number < 0 || number > 255 || state->first_call[number]) return;
    state->first_call[number] = true;
    std::fprintf(stderr, "ixion dc1 native first-call frame=%d donor=%s slot=%d ext=%d %s (%s)\n", mgPortFrameCount(),
                 DonorName(state), SlotOf(state), number, name, status);
}

// Positions cross the boundary divided by k, so the DC1 script's own distances stay valid.
void SelfPosition(float out[4]) {
    nowMonster->GetPosition(out);
    out[0] /= kScale;
    out[1] /= kScale;
    out[2] /= kScale;
    out[3] = 1.0f;
}

CCharacter2 *Player(const CActiveMonster *monster) {
    if (ActiveMonster == nullptr || ActiveMonster->scene == nullptr) return nullptr;
    const int target = monster->target_no >= 0 ? monster->target_no : 0;
    return ActiveMonster->scene->GetCharacter(target);
}

// DC1 reads the main character, CharaMain.pos (Chronicle runscript_opcodes.cpp:203, :224).
void PlayerPosition(float out[4]) {
    out[0] = out[1] = out[2] = 0.0f;
    out[3] = 1.0f;
    if (CCharacter2 *player = Player(nowMonster)) {
        player->GetPosition(out);
        out[0] /= kScale;
        out[1] /= kScale;
        out[2] /= kScale;
        out[3] = 1.0f;
    }
}

float Distance(const float a[4], const float b[4]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void Normalize3(float v[4]) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length > 0.0f) {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

void RotateY(float v[4], float angle) {
    sceVu0FMATRIX identity;
    sceVu0FMATRIX turn;
    sceVu0UnitMatrix(identity);
    sceVu0RotMatrixY(turn, identity, angle);
    sceVu0ApplyMatrix(v, turn, v);
}

// Chronicle mathutil.cpp:1189 AngleInterpolate, INTERPOLATE_STEP.
float AngleStep(float from, float to, float step) {
    float delta = to - from;
    if (delta > kPi) delta -= 2.0f * kPi;
    if (delta <= -kPi) delta += 2.0f * kPi;
    if (std::fabs(delta) < step) return to;
    float result = from + (delta < 0.0f ? -step : step);
    if (result > kPi) result -= 2.0f * kPi;
    if (result <= -kPi) result += 2.0f * kPi;
    return result;
}

float AngleDelta(float a, float b) {
    float delta = a - b;
    if (delta > kPi) delta -= 2.0f * kPi;
    if (delta < -kPi) delta += 2.0f * kPi;
    return std::fabs(delta);
}

// DC1 tests walls against the polygons within 30 units of the monster (Chronicle
// monstorunit.cpp:1953 setCollisionData(..., 30, 5)). DC2's map query is the room's while the
// selected enemy thinks (ixion_room_map.cpp), so the walls are the imported DC1 room's.
bool RayBlockedNear(const float position[4], const float from_dc2[4], const float to_dc2[4], int *hit_parts = nullptr) {
    if (ActiveMonster == nullptr || ActiveMonster->scene == nullptr) return false;
    CScene *scene = ActiveMonster->scene;
    CMap   *map = scene->GetMap(scene->active_map);
    if (map == nullptr) return false;
    const float reach = 30.0f * kScale;
    mgVu0FBOX box;
    box.max[0] = position[0] + reach;
    box.min[0] = position[0] - reach;
    box.max[2] = position[2] + reach;
    box.min[2] = position[2] - reach;
    box.max[1] = position[1] + 200.0f;
    box.min[1] = position[1] - 200.0f;
    box.max[3] = box.min[3] = 1.0f;
    static CCPoly polys[128];
    const int count = map->GetColPoly(polys, box, 128);
    float from[4] = {from_dc2[0], from_dc2[1], from_dc2[2], 1.0f};
    float to[4] = {to_dc2[0], to_dc2[1], to_dc2[2], 1.0f};
    float hit[4];
    if (hit_parts == nullptr) return CheckHit(polys, count, from, to, hit, 0, 0) >= 0;
    const int index = CheckHit(polys, count, from, to, hit, 1, 0);
    if (index >= 0) *hit_parts = polys[index].parts_no;
    return index >= 0;
}

bool RayBlocked(const CActiveMonster *monster, const float from_dc2[4], const float to_dc2[4]) {
    float position[4];
    const_cast<CActiveMonster *>(monster)->GetPosition(position);
    return RayBlockedNear(position, from_dc2, to_dc2);
}

// Bounded lookup (the stock SetDamage loop has no end bound, wave3-damage-adapter.md).
const DAMAGE_PARAM *FindParam(const char *name) {
    for (int i = 0; i < DAMAGE_PARAM_MAX; ++i) {
        if (std::strcmp(Damage_Param_Table[i].name, name) == 0) return &Damage_Param_Table[i];
    }
    return nullptr;
}

// 吹っ飛び攻撃 (a knock-back blow, DC1 kind 3) and ねば攻撃 (DC2's goo ball row).
char kBlowHit[] = "\x90\x81\x82\xC1\x94\xF2\x82\xD1\x8D\x55\x8C\x82";
char kGooHit[] = "\x82\xCB\x82\xCE\x8D\x55\x8C\x82";

void PrepareParams() {
    if (g_params_ready) return;
    const DAMAGE_PARAM *blow = FindParam(kBlowHit);
    const DAMAGE_PARAM *goo = FindParam(kGooHit);
    if (blow == nullptr || goo == nullptr) return;
    g_blow_param = *blow;
    g_goo_param = *goo;
    // DC1 kind 2 is a standing hurt reaction, not kind 3's launched body (dun/gameloop.cpp:7351).
    g_normal_param = *blow;
    g_normal_param.hit_flags &= ~DAMAGE_HIT_KNOCKDOWN;
    g_normal_param.stagger = 2;
    g_normal_param.status = 0;
    // DC1 goo is rolled by the shim (65%, untimed); DC2's own 30%/3600 slow roll stays off.
    g_goo_param.status = 0;
    g_params_ready = true;
}

// Scale net DC1 damage by the selected balance basis and live host maximum HP.
int ScaledDamage(int dc1_damage) {
    CBattleCharaInfo *battle = GetBattleCharaInfo();
    const float max_hp = battle != nullptr && battle->GetMaxHp_i() > 0 ? (float) battle->GetMaxHp_i() : 32.0f;
    return IxionDc1ScaledDamage(dc1_damage, max_hp, BalanceHp());
}

void AttachGooVisual(Shot &shot);
bool SpawnShot(State *state, const ShotEffect &effect, CActiveMonster *monster, const float from_dc2[4],
               const float target_dc2[4], int dc1_damage, bool death, bool player_owned = false,
               float caster_max_hp = 0.0f) {
    PrepareParams();
    Shot *shot = nullptr;
    for (Shot &candidate : g_shots) {
        if (!candidate.live) {
            shot = &candidate;
            break;
        }
    }
    if (shot == nullptr || !g_params_ready) return false;
    if (monster == nullptr && !player_owned) return false;
    CColPrim *prim = ColPrimMan.GetPrim();
    if (prim == nullptr) return false;
    *shot = Shot{};
    const int owner_id = player_owned ? 0 : monster->chara_type;
    prim->SetDamage(effect.goo ? kGooHit : kBlowHit, owner_id);
    prim->param = effect.goo ? &g_goo_param : effect.blow ? &g_blow_param : &g_normal_param;
    prim->status = 0;
    shot->player_owned = player_owned;
    shot->damage = player_owned ? IxionDc1ScaledDamage(dc1_damage, caster_max_hp, BalanceHp()) : ScaledDamage(dc1_damage);
    prim->damage = shot->damage;
    std::memcpy(shot->pos, from_dc2, sizeof shot->pos);
    shot->pos[3] = 1.0f;
    shot->vel[0] = target_dc2[0] - from_dc2[0];
    shot->vel[1] = target_dc2[1] - from_dc2[1];
    shot->vel[2] = target_dc2[2] - from_dc2[2];
    Normalize3(shot->vel);
    const float speed = effect.speed;
    shot->radius = effect.radius;
    shot->effect = effect.effect;
    shot->impact_radius = effect.impact_radius;
    shot->impact_frames = effect.impact_frames;
    shot->fire = effect.fire;
    shot->vel[0] *= speed * kScale;
    shot->vel[1] *= speed * kScale;
    shot->vel[2] *= speed * kScale;
    shot->vel[3] = 0.0f;
    // DC2's IsHit tests 2 x (radius + hit-entry size); half the DC1 radius keeps DC1's reach.
    prim->SetCoord(shot->pos, 0.5f * shot->radius * kScale);
    shot->prim = prim;
    shot->life = effect.life;
    shot->owner = owner_id;
    shot->slot = SlotOf(state);
    shot->death = death;
    shot->live = true;
    if (player_owned) {
        std::fprintf(stderr,
                     "ixion dc1 shot spawn frame=%d death=0 from=(%.1f,%.1f,%.1f) to=(%.1f,%.1f,%.1f) damage=%d (DC1 %d, "
                     "caster max hp %.0f, balance=%s basis=%.0f) speed=%.2f radius=%.2f effect=%d owner=player "
                     "target=monsters status=0\n",
                     mgPortFrameCount(), from_dc2[0], from_dc2[1], from_dc2[2], target_dc2[0], target_dc2[1],
                     target_dc2[2], shot->damage, dc1_damage, caster_max_hp,
                     BalanceHp() == 70.0f ? "faithful" : "friendly", BalanceHp(), speed * kScale,
                     0.5f * shot->radius * kScale, shot->effect);
        AttachGooVisual(*shot);
        return true;
    }
    TrackPrim(prim, shot->slot);
    std::fprintf(stderr,
                 "ixion dc1 shot spawn frame=%d donor=%s slot=%d death=%d from=(%.1f,%.1f,%.1f) to=(%.1f,%.1f,%.1f) "
                 "damage=%d (DC1 %d) speed=%.2f radius=%.2f effect=%d\n",
                 mgPortFrameCount(), DonorName(state), shot->slot, death ? 1 : 0, from_dc2[0], from_dc2[1], from_dc2[2],
                 target_dc2[0], target_dc2[1], target_dc2[2], shot->damage, dc1_damage, speed * kScale,
                 0.5f * shot->radius * kScale, shot->effect);
    return true;
}

CActiveMonster *SceneMonster(int chara_id) {
    if (ActiveMonster == nullptr || ActiveMonster->scene == nullptr) return nullptr;
    if (chara_id < MONSTER_ACTIVE_MAX || chara_id >= 2 * MONSTER_ACTIVE_MAX) return nullptr;
    return static_cast<CActiveMonster *>(ActiveMonster->scene->GetCharacter(chara_id));
}

Shot *PlayerShotOf(const CColPrim *prim) {
    if (prim == nullptr) return nullptr;
    for (Shot &shot : g_shots) {
        if (shot.live && shot.player_owned && shot.prim == prim) return &shot;
    }
    return nullptr;
}

// ---- Monica faithful round 2: the player's ball wears DC2's own goo model ----
// DC2's ねばねばＬ (m_nebaneba_l.chr + .stb) is a self-flying lob that makes its own attack prim at its sixth
// step. The ball must fly on DC1's rules (straight, 0.4/frame, one prim: the Shot's), so the script is used as a
// model only: it runs one step (sets its motion, fade and scale), then is held in EFF_SPT_STATE_SCRIPT_PAUSE
// (moves and draws, no script, so it never reaches its own colprim) and the model is placed on the ball each
// tick. Order inside a frame: StepEffect (script step) runs before ThinkHost (this pin), Draw after.
char kGooFlightEffect[] = "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B"; // ねばねばＬ
char kGooSplatEffect[] = "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67";  // ねばねばＨ (the lob's own hit splat)

CEffectScriptMan *PlayerEffects() {
    auto *body = static_cast<CActionChara *>(MainChara__2);
    return body != nullptr ? body->effect_man : nullptr;
}

float ShotYaw(const Shot &shot) { return std::atan2(shot.vel[0], shot.vel[2]); }

void AttachGooVisual(Shot &shot) {
    CEffectScriptMan *effects = PlayerEffects();
    if (effects == nullptr) {
        std::fprintf(stderr, "ixion dc1 goo visual frame=%d: no player effect manager, placeholder sparks\n",
                     mgPortFrameCount());
        return;
    }
    const int base = effects->SearchBaseNo(kGooFlightEffect);
    const int slot = base >= 0 ? effects->CreateEffSpt(kGooFlightEffect, 0, 1) : -1;
    _EFF_SCRIPT *script = slot >= 0 && slot < EFF_SPT_OWNER_SLOT_MAX ? effects->slot[0][slot] : nullptr;
    if (script == nullptr) {
        std::fprintf(stderr,
                     "ixion dc1 goo visual frame=%d: m_nebaneba_l unavailable (base=%d slot=%d), placeholder sparks\n",
                     mgPortFrameCount(), base, slot);
        return;
    }
    std::memcpy(script->work_vect1, shot.pos, sizeof script->work_vect1);
    script->work_vect2[0] = 0.0f;
    script->work_vect2[1] = ShotYaw(shot);
    script->work_vect2[2] = 0.0f;
    script->work_vect2[3] = 1.0f;
    script->value[0].i = 0; // its own attack value; the script is paused before it can make a prim
    shot.fx_slot = slot;
    shot.fx_script = script;
    shot.fx_ticks = 0;
    std::fprintf(stderr, "ixion dc1 goo visual frame=%d: m_nebaneba_l model attached slot=%d base=%d chara=%d\n",
                 mgPortFrameCount(), slot, base, script->chara != nullptr ? 1 : 0);
}

// Pins the model to the ball. False when there is no model (the caller draws the placeholder sparks).
bool PinGooVisual(Shot &shot) {
    if (shot.fx_script == nullptr) return false;
    CEffectScriptMan *effects = PlayerEffects();
    if (effects == nullptr || shot.fx_slot < 0 || effects->slot[0][shot.fx_slot] != shot.fx_script) {
        std::fprintf(stderr, "ixion dc1 goo visual frame=%d: model lost (effects cleared), placeholder sparks\n",
                     mgPortFrameCount());
        shot.fx_script = nullptr;
        shot.fx_slot = -1;
        return false;
    }
    _EFF_SCRIPT *script = shot.fx_script;
    if (script->colprim != nullptr || script->chara == nullptr) {
        std::fprintf(stderr, "ixion dc1 goo visual frame=%d: model unusable (colprim=%d chara=%d), removed\n",
                     mgPortFrameCount(), script->colprim != nullptr ? 1 : 0, script->chara != nullptr ? 1 : 0);
        effects->DeleteEffSpt(0, shot.fx_slot);
        shot.fx_script = nullptr;
        shot.fx_slot = -1;
        return false;
    }
    script->state = EFF_SPT_STATE_SCRIPT_PAUSE;
    alignas(16) float aligned[4] = {shot.pos[0], shot.pos[1], shot.pos[2], 1.0f}; // 16-byte copies in the engine
    script->chara->SetPosition(aligned);
    script->chara->SetRotation(0.0f, ShotYaw(shot), 0.0f);
    if (shot.fx_ticks++ == 0) {
        std::fprintf(stderr, "ixion dc1 goo visual frame=%d: model pinned at (%.1f,%.1f,%.1f) script held\n",
                     mgPortFrameCount(), shot.pos[0], shot.pos[1], shot.pos[2]);
    }
    return true;
}

// Removes the model with the ball; on a hit or a wall DC2's own splat (ねばねばＨ) marks the spot, as the lob does.
void ReleaseGooVisual(Shot &shot, bool splat) {
    CEffectScriptMan *effects = PlayerEffects();
    if (effects != nullptr && shot.fx_script != nullptr && shot.fx_slot >= 0 &&
        effects->slot[0][shot.fx_slot] == shot.fx_script) {
        effects->DeleteEffSpt(0, shot.fx_slot);
    }
    if (splat && effects != nullptr && shot.player_owned && effects->SearchBaseNo(kGooSplatEffect) >= 0) {
        effects->CreateEffSpt(kGooSplatEffect, 0, 0);
        if (effects->now != nullptr) {
            alignas(16) float aligned[4] = {shot.pos[0], shot.pos[1], shot.pos[2], 1.0f};
            effects->SetScriptVect1(aligned, 0, -1);
            std::fprintf(stderr, "ixion dc1 goo visual frame=%d: m_nebaneba_h splat at (%.1f,%.1f,%.1f)\n", mgPortFrameCount(),
                         shot.pos[0], shot.pos[1], shot.pos[2]);
        }
    }
    shot.fx_script = nullptr;
    shot.fx_slot = -1;
}

void EndShot(Shot &shot, const char *why) {
    if (shot.player_owned) {
        ReleaseGooVisual(shot, std::strcmp(why, "hit-monster") == 0 || std::strcmp(why, "wall") == 0);
    }
    if (shot.prim != nullptr && shot.prim->owner == shot.owner &&
        (shot.prim->param == &g_goo_param || shot.prim->param == &g_normal_param || shot.prim->param == &g_blow_param))
        shot.prim->Delete(-1);
    std::fprintf(stderr, "ixion dc1 shot end frame=%d why=%s pos=(%.1f,%.1f,%.1f) life_left=%d\n", mgPortFrameCount(),
                 why, shot.pos[0], shot.pos[1], shot.pos[2], shot.life);
    shot = Shot{};
}

// The drawn stand-in for DC1's fireball model: DC2's fire-element after-hit burst (dng_effect.cpp:374,
// stepped and drawn by DngMain, dng_main.cpp:2082/1536). Pool entry 0 stays DC2's own monster hits
// (monster.cpp:2387); the shim cycles 1-5. size 0 is a tight trail, 45 a full burst.
void FireStandIn(const float position[4], float size) {
    static int next = 0;
    float at[4] = {position[0], position[1], position[2], 1.0f};
    fireAfterHit[1 + next].SetPos(at, size, 255);
    next = (next + 1) % (DNG_ELEMENT_EFFECT_NUM - 1);
}

// DC1 goo (Chronicle gameloop.cpp:7267-7283): 65%, not already gooed, Anti-Goo amulet blocks.
void ApplyGoo() {
    CBattleCharaInfo *battle = GetBattleCharaInfo();
    if (battle == nullptr) return;
    const int roll = iRand(100);
    const int chance = GooChance();
    const bool already = (battle->GetAttr() & CHARA_STATUS_SLOW) != 0;
    bool       applied = false;
    if (roll < chance && !already && CheckAmuletAvoid(ITEM_ID_ANTI_GOO_AMULET) == 0) {
        battle->SetAttrVol(CHARA_STATUS_SLOW, kGooTime);
        if (ActiveMonster != nullptr && ActiveMonster->scene != nullptr) {
            sndSePlay(ActiveMonster->scene->se_battle_id, 0x52, 0);
        }
        applied = true;
    }
    std::fprintf(stderr, "ixion dc1 goo roll frame=%d roll=%d chance=%d already=%d applied=%d attr=0x%x\n",
                 mgPortFrameCount(), roll, chance, already ? 1 : 0, applied ? 1 : 0, battle->GetAttr());
}

CHRINFO_KEY_SET *KeyByIndex(CActiveMonster *monster, int number) {
    if (number < 0 || monster->key_list[0] == nullptr || number >= monster->key_num[0]) return nullptr;
    auto *key = &monster->key_list[0][number];
    // _KEY_END includes an empty sentinel in key_num; never select that as a motion.
    return key->name[0] != 0 ? key : nullptr;
}

// ---- natives (DC1 numbering; Chronicle ps2/src/runscript_opcodes.cpp) ----

// 10 _GET_DISTANCE, :195.
int GetDistance(RS_STACKDATA *stack, int argc) {
    FirstCall(10, "_GET_DISTANCE", "implemented");
    float self[4], target[4];
    SelfPosition(self);
    if (argc == 1) {
        PlayerPosition(target);
    } else {
        target[0] = GetFloat(stack++);
        target[1] = GetFloat(stack++);
        target[2] = GetFloat(stack++);
    }
    SetFloat(stack, Distance(self, target));
    return 1;
}

// 11 _GET_POSITION, :214. -1 self, -2 the main character.
int GetPosition(RS_STACKDATA *stack, int argc) {
    FirstCall(11, "_GET_POSITION", "implemented");
    const int target = (int) GetFloat(stack++);
    float position[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (target == -1) SelfPosition(position);
    if (target == -2) PlayerPosition(position);
    SetFloat(stack++, position[0]);
    SetFloat(stack++, position[1]);
    SetFloat(stack, position[2]);
    return 1;
}

// 12 _SET_ROTATION, :233. A negative speed faces the point at once.
int SetRotation(RS_STACKDATA *stack, int argc) {
    FirstCall(12, "_SET_ROTATION", "implemented");
    State *state = Now();
    state->turn_target[0] = GetFloat(stack++);
    state->turn_target[1] = GetFloat(stack++);
    state->turn_target[2] = GetFloat(stack++);
    state->turn_speed = GetFloat(stack);
    if (state->turn_speed < 0.0f) {
        float position[4], rotation[4];
        SelfPosition(position);
        nowMonster->GetRotation(rotation);
        rotation[1] = std::atan2(state->turn_target[0] - position[0], state->turn_target[2] - position[2]);
        nowMonster->SetRotation(rotation);
        state->turn_speed = 0.0f;
    }
    return 1;
}

// 13 _CHK_ROTATION, :257.
int ChkRotation(RS_STACKDATA *stack, int argc) {
    FirstCall(13, "_CHK_ROTATION", "implemented");
    SetInt(stack, Now()->turn_speed == 0.0f ? 1 : 0);
    return 1;
}

// 14 _CHK_MOVE, Chronicle runscript_opcodes.cpp:269. The optional float measures the retail
// movement vector against self position (despite its surprising units); Ghost only asks for done.
int ChkMove(RS_STACKDATA *stack, int argc) {
    FirstCall(14, "_CHK_MOVE", "implemented");
    State *state = Now();
    SetInt(stack++, state->move_speed == 0.0f ? 1 : 0);
    if (argc == 2) {
        float position[4];
        SelfPosition(position);
        const float movement[4] = {state->move_dir[0], state->move_dir[1], state->move_dir[2], 1.0f};
        SetFloat(stack, Distance(position, movement));
    }
    return 1;
}

// 30 _GET_VECTOR, :331. Unit vector from the monster to a point, optionally turned about Y.
int GetVector(RS_STACKDATA *stack, int argc) {
    FirstCall(30, "_GET_VECTOR", "implemented");
    float position[4], direction[4];
    SelfPosition(position);
    direction[0] = GetFloat(stack++);
    direction[1] = GetFloat(stack++);
    direction[2] = GetFloat(stack++);
    direction[3] = 1.0f;
    float angle = 0.0f;
    if (argc == 7) {
        angle = GetFloat(stack++);
        if (angle >= 180.0f) angle -= 360.0f;
        angle = angle * kPi / 180.0f;
        if (angle > 2.0f * kPi) angle -= 2.0f * kPi;
        if (angle < -kPi) angle += 2.0f * kPi;
    }
    direction[0] -= position[0];
    direction[1] -= position[1];
    direction[2] -= position[2];
    Normalize3(direction);
    if (argc == 7) RotateY(direction, angle);
    SetFloat(stack++, direction[0]);
    SetFloat(stack++, direction[1]);
    SetFloat(stack, direction[2]);
    return 1;
}

// 31 _GET_DIRECTION, :380. The facing, optionally turned by degrees.
int GetDirection(RS_STACKDATA *stack, int argc) {
    FirstCall(31, "_GET_DIRECTION", "implemented");
    float rotation[4];
    nowMonster->GetRotation(rotation);
    if (argc == 4) {
        rotation[1] += GetFloat(stack++) * kPi / 180.0f;
        if (rotation[1] > kPi) rotation[1] -= 2.0f * kPi;
        if (rotation[1] < -kPi) rotation[1] += 2.0f * kPi;
    }
    float direction[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    RotateY(direction, rotation[1]);
    Normalize3(direction);
    SetFloat(stack++, direction[0]);
    SetFloat(stack++, direction[1]);
    SetFloat(stack, direction[2]);
    return 1;
}

// 32 _SET_MOVE, :413. Walk along the unit vector toward a point at a speed per frame.
int SetMove(RS_STACKDATA *stack, int argc) {
    FirstCall(32, "_SET_MOVE", "implemented (DC1 slow halving not mapped)");
    State *state = Now();
    float position[4], direction[4];
    SelfPosition(position);
    for (int i = 0; i < 3; ++i) {
        direction[i] = GetFloat(stack++) - position[i];
    }
    direction[3] = 1.0f;
    Normalize3(direction);
    state->move_dir[0] = direction[0];
    state->move_dir[1] = direction[1];
    state->move_dir[2] = direction[2];
    state->move_speed = GetFloat(stack);
    return 1;
}

// 33 _CHK_MOVE_INFO, :438. 1 when the line from the monster to the point is clear of walls.
int ChkMoveInfo(RS_STACKDATA *stack, int argc) {
    FirstCall(33, "_CHK_MOVE_INFO", "implemented (room walls; other monsters' spheres not included)");
    float from[4], to[4];
    nowMonster->GetPosition(from);
    to[0] = GetFloat(stack++) * kScale;
    to[1] = GetFloat(stack++) * kScale;
    to[2] = GetFloat(stack++) * kScale;
    from[1] += 5.0f * kScale;
    to[1] += 5.0f * kScale;
    SetInt(stack, RayBlocked(nowMonster, from, to) ? 0 : 1);
    return 1;
}

// 34 _SET_MOVE_CANSEL, :461.
int SetMoveCancel(RS_STACKDATA *stack, int argc) {
    FirstCall(34, "_SET_MOVE_CANSEL", "implemented");
    Now()->move_speed = 0.0f;
    return 1;
}

// 35 _SET_ROT_CANSEL, Chronicle runscript_opcodes.cpp:466.
int SetRotCancel(RS_STACKDATA *stack, int argc) {
    FirstCall(35, "_SET_ROT_CANSEL", "implemented");
    Now()->turn_speed = 0.0f;
    return 1;
}

// 36 _SET_POSITION, Chronicle runscript_opcodes.cpp:471 (unchecked teleport).
int SetPosition(RS_STACKDATA *stack, int argc) {
    FirstCall(36, "_SET_POSITION", "implemented: scaled world position");
    float old[4], position[4];
    nowMonster->GetPosition(old);
    for (int i = 0; i < 3; ++i) position[i] = GetFloat(stack++) * kScale;
    position[3] = 1.0f;
    nowMonster->SetPosition(position);
    std::memcpy(nowMonster->next_pos, position, sizeof position);
    std::fprintf(stderr, "ixion dc1 teleport frame=%d from=(%.2f,%.2f,%.2f) to=(%.2f,%.2f,%.2f) alpha=%.1f muteki=%d\n",
                 mgPortFrameCount(), old[0], old[1], old[2], position[0], position[1], position[2], Now()->alpha,
                 nowMonster->muteki_time);
    Capture(2, "teleport-hidden");
    return 1;
}

// 100 _STATUS_SET_FALL, Chronicle runscript_opcodes.cpp:481.
int SetFall(RS_STACKDATA *stack, int argc) {
    FirstCall(100, "_STATUS_SET_FALL", "implemented: DC1 vertical movement");
    Now()->falls = GetInt(stack) != 0;
    Now()->vertical_control = true;
    std::fprintf(stderr, "ixion dc1 falls frame=%d falls=%d\n", mgPortFrameCount(), Now()->falls ? 1 : 0);
    return 1;
}

// 242 _SET_GRAVITY, Chronicle runscript_opcodes.cpp:1434. Independent of falls: snap vs free fall.
int SetGravity(RS_STACKDATA *stack, int argc) {
    FirstCall(242, "_SET_GRAVITY", "implemented: DC1 snap/free-fall policy");
    Now()->free_fall = GetInt(stack) != 0;
    Now()->vertical_control = true;
    return 1;
}

// 227 _SET_LOCKON_SW, Chronicle runscript_opcodes.cpp:1098, read in dun/gameloop.cpp:9085.
int SetLockonSw(RS_STACKDATA *stack, int argc) {
    FirstCall(227, "_SET_LOCKON_SW", "implemented: DC2 lock-on eligibility and release");
    const bool enabled = GetInt(stack) != 0;
    if (enabled) nowMonster->attrib &= ~MONSTER_ATTRIB_NO_LOCK_ON;
    else {
        nowMonster->attrib |= MONSTER_ATTRIB_NO_LOCK_ON;
        if (MainChara__2 != nullptr && MainChara__2->target_no == nowMonster->chara_type) {
            MainChara__2->lock_on = 0;
            MainChara__2->target_no = -1;
        }
    }
    std::fprintf(stderr, "ixion dc1 lockon switch frame=%d enabled=%d alpha=%.1f\n", mgPortFrameCount(),
                 enabled ? 1 : 0, Now()->alpha);
    return 1;
}

// 101 _STATUS_SET_MUTEKI, :488 -> DC2 muteki_time.
int SetMuteki(RS_STACKDATA *stack, int argc) {
    FirstCall(101, "_STATUS_SET_MUTEKI", "implemented as DC2 muteki_time");
    nowMonster->muteki_time = GetInt(stack);
    return 1;
}

// 102 _STATUS_SET_ALPHA, :500. Fade step per frame from 128.
int SetAlpha(RS_STACKDATA *stack, int argc) {
    FirstCall(102, "_STATUS_SET_ALPHA", "implemented (fade on DC2 alpha)");
    State *state = Now();
    state->alpha_step = GetFloat(stack);
    std::fprintf(stderr, "ixion dc1 fade frame=%d alpha=%.1f step=%.1f\n", mgPortFrameCount(), state->alpha,
                 state->alpha_step);
    return 1;
}

// 103 _STATUS_CHK_ALPHA, :514.
int ChkAlpha(RS_STACKDATA *stack, int argc) {
    FirstCall(103, "_STATUS_CHK_ALPHA", "implemented");
    State *state = Now();
    int fading_in = 0;
    if (argc == 2) fading_in = GetInt(stack++);
    const bool done = fading_in == 0 ? state->alpha <= 0.0f : state->alpha >= 128.0f;
    SetInt(stack, done ? 1 : 0);
    return 1;
}

// 104 _STATUS_SET_DEAD, :538. DC1 removes the monster here, after its own fade. DC2's death runs
// once: _SET_DEAD_START (state, money and item drops) and _SET_DEAD_OFF (dead effect, experience
// pickups). DC2's dead_alpha fade is cut to one step because Opar has already faded (no double
// fade); ThinkHost then counts the kill once (KillMonsterCount, floor kill_count) and frees the slot.
void DonorDeath(State *state, const char *why, bool cut_fade);
int SetDead(RS_STACKDATA *stack, int argc) {
    FirstCall(104, "_STATUS_SET_DEAD", "implemented: DC2 _SET_DEAD_START + _SET_DEAD_OFF once");
    DonorDeath(Now(), "dc1 104", true);
    return 1;
}

// 105 _STATUS_SET_PALLET, :648 -> DC2 pallet[2] (pallet[0] is DC2's own hit flash, [1] rage).
// DC1 blends to the colour and back at `step` per frame, `cycles` times (monstorunit.cpp:684-695).
int SetPallet(RS_STACKDATA *stack, int argc) {
    FirstCall(105, "_STATUS_SET_PALLET", "implemented as DC2 pallet[2] pulses");
    int red = 0xFF, green = 0xB4, blue = 0x80; // DC2's default hit colour stands in for the element colour
    if (argc == 5) {
        red = (int) GetFloat(stack++);
        green = (int) GetFloat(stack++);
        blue = (int) GetFloat(stack++);
    }
    const int   cycles = GetInt(stack++);
    const float step = GetFloat(stack);
    const int   period = step > 0.0f ? (int) std::lround(2.0f / step) : 0;
    if (cycles <= 0 || period <= 0) return 1;
    nowMonster->pallet[2].SetAnim((s16) red, (s16) green, (s16) blue, (s16) cycles, (s16) (cycles * period), 0);
    std::fprintf(stderr, "ixion dc1 pallet frame=%d rgb=(%d,%d,%d) pulses=%d frames=%d\n", mgPortFrameCount(), red,
                 green, blue, cycles, cycles * period);
    return 1;
}

// 106 _STATUS_SET_CLIPLEVEL, :669 -> DC2 clip_dist (scaled).
int SetClipLevel(RS_STACKDATA *stack, int argc) {
    FirstCall(106, "_STATUS_SET_CLIPLEVEL", "implemented as DC2 clip_dist * k");
    Now()->clip = GetFloat(stack);
    nowMonster->clip_dist = Now()->clip * kScale;
    return 1;
}

// 114 _STATUS_GET_MOTION_ID, :681.
int GetMotionId(RS_STACKDATA *stack, int argc) {
    FirstCall(114, "_STATUS_GET_MOTION_ID", "implemented");
    SetInt(stack, Now()->motion_no);
    return 1;
}

// 113 _STATUS_GET_HITDMG_VOL (:676): reset by every SET_MOTION, including the same key.
int GetHitDamage(RS_STACKDATA *stack, int argc) {
    FirstCall(113, "_STATUS_GET_HITDMG_VOL", "implemented: outgoing player damage since SET_MOTION");
    SetInt(stack, Now()->last_hit_damage);
    return 1;
}

// 115 _STATUS_GET_DMG_ID (:687): monster[n].last_hit_id, DC1 owner * 10 + attack of the last
// accepted hit (monstorunit.cpp:1279-1290). The DC2 attacker is mapped to a DC1 owner/attack in
// IxionDc1HitIdFor; the value is kept until the next accepted hit (DC1 clears it at the start of each
// CheckDmg, :1134, and runs prog_110 in the same step; DC2 runs the hit program a think later).
int GetDamageId(RS_STACKDATA *stack, int argc) {
    FirstCall(115, "_STATUS_GET_DMG_ID", "implemented: DC1 owner*10+attack from the DC2 attacker kind");
    State *state = Now();
    SetInt(stack, state->last_hit_id);
    std::fprintf(stderr, "ixion dc1 dmg id frame=%d donor=%s slot=%d last_hit_id=%d\n", mgPortFrameCount(),
                 DonorName(state), SlotOf(state), state->last_hit_id);
    return 1;
}

// 202 _GET_MOTION_FRM (:181-186): the current motion time. Key n is DC1 motion n with DC1's own frame
// numbers, so DC2's frame is the DC1 time; a SET_MOTION not yet stepped reads its start frame, as
// _CHK_MOTION_FRM does here.
int GetMotionFrm(RS_STACKDATA *stack, int argc) {
    FirstCall(202, "_GET_MOTION_FRM", "implemented: DC2 key frame = DC1 motion time");
    State           *state = Now();
    CHRINFO_KEY_SET *key = KeyByIndex(nowMonster, state->motion_no);
    float            frame = nowMonster->frame;
    if (key != nullptr && (state->motion_pending || nowMonster->now_key != key)) frame = (float) key->start_frame;
    SetFloat(stack, frame);
    return 1;
}

// 108 _STATUS_SET_COL_OFF (:557-561): collision_off_timer. While it is above 0 DC1 skips the map
// slide, the monster push and the wall stop (monstorunit.cpp:2026-2043); it drops by one per step
// (:2133-2135). DC2: MONSTER_ATTRIB_NO_MAP_HIT and NO_BODY_HIT for that many thinks (IxionDc1AfterScript).
int SetColOff(RS_STACKDATA *stack, int argc) {
    FirstCall(108, "_STATUS_SET_COL_OFF", "implemented: DC2 NO_MAP_HIT|NO_BODY_HIT countdown");
    State *state = Now();
    state->col_off = GetInt(stack);
    if (state->col_off > 0) {
        const u32 bits = MONSTER_ATTRIB_NO_MAP_HIT | MONSTER_ATTRIB_NO_BODY_HIT;
        state->col_off_attrib |= bits & ~nowMonster->attrib;
        nowMonster->attrib |= bits;
    }
    std::fprintf(stderr, "ixion dc1 col off frame=%d donor=%s slot=%d timer=%d attrib=0x%x\n", mgPortFrameCount(),
                 DonorName(state), SlotOf(state), state->col_off, nowMonster->attrib);
    return 1;
}

// 116 _STATUS_SET_LOCKON_DIST (:691-696): lock_range, read by the DC1 cursor (dun/gameloop.cpp:9093,
// :9246). DC2's lock-on range is fixed at 300 (actionchara.cpp:3464), so the value is recorded and
// logged; DC2's own lock-on stays the boss framing (wave5-boss-camera.md).
int SetLockonDist(RS_STACKDATA *stack, int argc) {
    FirstCall(116, "_STATUS_SET_LOCKON_DIST", "recorded: DC2 lock-on range is fixed (300)");
    State *state = Now();
    state->lock_range = GetFloat(stack);
    std::fprintf(stderr, "ixion dc1 lockon dist donor=%s slot=%d dc1=%.1f (dc2 units %.1f; DC2 range fixed 300)\n",
                 DonorName(state), SlotOf(state), state->lock_range, state->lock_range * kScale);
    return 1;
}

// 134 _SET_BODY_COL_PARA (:804-821) on the last _SET_BODY_COL sphere: index 0-9 body_parameter (no
// reader in DC1), 10-15 the percent of damage from DC1 attacker index-10 (monstorunit.cpp:1521-1527).
int SetBodyColPara(RS_STACKDATA *stack, int argc) {
    FirstCall(134, "_SET_BODY_COL_PARA", "implemented: per-sphere DC1 attacker damage percent");
    State    *state = Now();
    const int index = GetInt(stack++);
    const int value = GetInt(stack);
    const int col = state->last_body_col;
    if (col < 0) return 1;
    if (index >= 0 && index <= 9) state->body_parameter[col][index] = value;
    if (index >= 10 && index <= 15) state->attacker_percent[col][index - 10] = value;
    std::fprintf(stderr, "ixion dc1 body col para donor=%s slot=%d sphere=%d entry=%d index=%d value=%d%s\n", DonorName(state),
                 SlotOf(state), col, state->body_cols[col].entry, index, value,
                 index >= 10 && index <= 15 ? " (attacker percent)" : "");
    return 1;
}

// DC2 player attack kind -> DC1 attacker (owner) and attack number. Dran's floor (DBC) has Toan and
// Xiao only: DC2 melee is Toan's sword (owner 0, attack 0, charakey.cpp:124-149 basic_damage(0, 0));
// DC2 guns are Xiao's slingshot pellet (owner 1, attack 0, shot_effect.cpp:71 SetUserID(1, 0));
// Monica's magic is Ruby's (owner 3, attack 0). Ridepod and monster kinds have no DC1 owner (-1).
struct Dc1Attacker { int owner; int attack; };
Dc1Attacker AttackerFor(int dc2_kind) {
    switch (dc2_kind) {
        case DAMAGE_KIND_MAX_MELEE:
        case DAMAGE_KIND_MONICA_MELEE: return {0, 0};
        case DAMAGE_KIND_MAX_GUN:
        case DAMAGE_KIND_LASER_GUN:
        case DAMAGE_KIND_GRENADE: return {1, 0};
        case DAMAGE_KIND_MONICA_MAGIC: return {3, 0};
        default: return {-1, -1};
    }
}

// The DC1 run script (event) ids a donor may request through 111, mapped to port-owned ids. DC2's
// event_no is never written: the room accepts only 1200 there and retires on anything else.
constexpr int kPortEventBase = 30000;
constexpr int kDc1EventDranDefeated = 510; // d01 prog510: EXT(711)(0, 100) boss flag + cutscene F1fcc0

// Prims whose damage the 134 percent scaled for one CheckHit; restored before the next query.
struct ScaledPrim { CColPrim *prim; int damage; };
std::vector<ScaledPrim> g_scaled_prims;
void RestoreScaledPrims() {
    for (const ScaledPrim &entry : g_scaled_prims) entry.prim->damage = entry.damage;
    g_scaled_prims.clear();
}

// DC2's death once (_SET_DEAD_START: state, money, drops; _SET_DEAD_OFF: dead effect, experience),
// then the room's kill memory. cut_fade: the DC1 script has already faded (104), so DC2's fade is one step.
void DonorDeath(State *state, const char *why, bool cut_fade) {
    if (state->dead_done) return;
    state->dead_done = true;
    const int money = nowMonster->reward_money;
    const int exp = nowMonster->reward_exp;
    _SET_DEAD_START(nullptr, 0);
    _SET_DEAD_OFF(nullptr, 0);
    if (cut_fade) {
        nowMonster->dead_alpha = 3;
        nowMonster->alpha = 0.0f;
    }
    state->dead_pending = true;
    state->kills_before = NowFloorInfoPtr != nullptr ? NowFloorInfoPtr->kill_count : -1;
    Capture(15, "death-start");
    std::fprintf(stderr, "ixion dc1 dead frame=%d donor=%s slot=%d DC2 death once (%s): money=%d exp=%d drops={%d,%d,%d} state=%d dead_alpha=%d\n",
                 mgPortFrameCount(), DonorName(state), SlotOf(state), why, money, exp, nowMonster->tbl->drop_item[0],
                 nowMonster->tbl->drop_item[1], nowMonster->tbl->drop_item[2], nowMonster->state, nowMonster->dead_alpha);
    // Kill memory: the room remembers this spawn for the rest of the floor lifetime (DC1 keeps a
    // dead monster's state -1 until the next _LOAD_MONSTOR), so a reinstall neither respawns nor pays.
    IxionRoomRecordDonorKill(nowMonster, DonorName(state));
}

// 111 _RUN_SCRIPT (:551-555): requested_event, which DC1's loop runs as an event script
// (dun/gameloop.cpp:3395-3399). Mapped to a port-owned id and handled by the room
// (IxionRoomDonorEvent); an unmapped id is refused and logged. Dran's killed program (prog_120:
// MUTEKI 100, _RUN_SCRIPT(510)) never calls 104, because event 510's cutscene removes him in DC1,
// so the mapped death event is where DC2's death runs, with DC2's own fade.
int RunScript(RS_STACKDATA *stack, int argc) {
    FirstCall(111, "_RUN_SCRIPT", "implemented: port-owned event id; DC2 event_no untouched");
    State    *state = Now();
    const int dc1_event = GetInt(stack);
    const int port_event = IxionDc1PortEventId(dc1_event);
    std::fprintf(stderr, "ixion dc1 run script frame=%d donor=%s slot=%d dc1_event=%d -> port_event=%d prog=%d hp=%d/%d\n",
                 mgPortFrameCount(), DonorName(state), SlotOf(state), dc1_event, port_event, nowMonster->now_prog,
                 nowMonster->life, nowMonster->max_life);
    if (port_event < 0) {
        std::fprintf(stderr, "ixion dc1 run script refused: DC1 event %d has no port mapping (nothing run)\n", dc1_event);
        return 1;
    }
    IxionRoomDonorEvent(port_event, dc1_event, nowMonster);
    if (dc1_event == kDc1EventDranDefeated && (nowMonster->now_prog == MONSTER_PROG_DEAD || nowMonster->life <= 0))
        DonorDeath(state, "dc1 event 510", false);
    return 1;
}

// 210 (:1012): host's currently controlled character. Max is 0, Monica is 1.
int GetCharacterId(RS_STACKDATA *stack, int argc) {
    FirstCall(210, "_GET_CHR_ID", "implemented: DC2 controlled character id");
    SetInt(stack, DngUserData != nullptr ? DngUserData->active_chr_no : 0);
    return 1;
}

// 244 (:1441): three inclusive integer-truncated motion windows, independent of motion id.
int SetGuardFrame(RS_STACKDATA *stack, int argc) {
    FirstCall(244, "_SET_GUARD_FRAME", "implemented: inclusive DC1 full-block window");
    State *state = Now();
    const int start = (int) GetFloat(stack++);
    const int end = (int) GetFloat(stack);
    if (state->guard_count < 3) {
        const int slot = state->guard_count++;
        state->guard_start[slot] = start;
        state->guard_end[slot] = end;
        std::fprintf(stderr, "ixion dc1 guard window slot=%d frames=%d-%d\n", slot, start, end);
    }
    return 1;
}

// 245 (:1468): attack age, distance in DC1 units, current character. The direction test
// really invalidates the age on dot >= .35, as the retail source does.
int GuardSearch(RS_STACKDATA *stack, int argc) {
    FirstCall(245, "_GUARD_SEARCH", "implemented: host attack age, DC1 distance and direction test");
    State *state = Now();
    float player[4], self[4];
    PlayerPosition(player);
    SelfPosition(self);
    float direction[4] = {self[0]-player[0], 0, self[2]-player[2], 0};
    Normalize3(direction);
    CScene *scene = ActiveMonster != nullptr ? ActiveMonster->scene : nullptr;
    auto *camera = scene != nullptr ? static_cast<CCameraControl *>(scene->GetCamera(scene->active_camera)) : nullptr;
    const float angle = camera != nullptr ? camera->GetAngle() : 0.0f;
    const float x = GamePad__2.GetLXf(), y = GamePad__2.GetLYf();
    float input[4] = {x*std::cos(angle)+y*std::sin(angle), 0,
                      -x*std::sin(angle)+y*std::cos(angle), 0};
    // DC1 gameloop.cpp:3025-3027 keeps the raw rotated stick (magnitude matters).
    if (input[0]*direction[0] + input[2]*direction[2] >= 0.35f) state->frames_since_attack = 3600;
    SetInt(stack++, state->frames_since_attack);
    SetFloat(stack++, Distance(player, self));
    SetInt(stack, DngUserData != nullptr ? DngUserData->active_chr_no : 0);
    return 1;
}

// 117 _STATUS_SET_SHADOW_LEN, Chronicle runscript_opcodes.cpp:698. Recorded while shadows are dropped.
int SetShadowLen(RS_STACKDATA *stack, int argc) {
    FirstCall(117, "_STATUS_SET_SHADOW_LEN", "recorded: donor shadow disabled");
    Now()->shadow_length = GetFloat(stack);
    return 1;
}

// 118 _STATUS_SET_LOCKON_TRG, Chronicle runscript_opcodes.cpp:705-730: the named frame becomes the
// lock-on point (monstorunit.cpp:800-801 tracks it; dun/gameloop.cpp:9274-9281 draws the cursor
// there, scaled by the optional x/y). DC2 aims, gauges and locks on at the first TARGET (group 0)
// entry (dng_hud.cpp:854, monster.cpp ThinkHost center_pos), so that entry's frame is replaced.
// The DC1 cursor scale is recorded; DC2's marker keeps the converted entry size.
int SetLockonTrg(RS_STACKDATA *stack, int argc) {
    FirstCall(118, "_STATUS_SET_LOCKON_TRG", "implemented: DC2 TARGET entry frame");
    State      *state = Now();
    const char *name = GetString(stack++);
    float       scale_x = 1.0f, scale_y = 1.0f;
    if (argc == 3) {
        scale_x = GetFloat(stack++);
        scale_y = GetFloat(stack);
    }
    mgCFrame *frame = nowMonster->SearchObject(const_cast<char *>(name));
    int       entry = -1;
    if (frame != nullptr) {
        for (int i = 0; i < CHARA_ENTRY_OBJECT_MAX && entry < 0; ++i) {
            if (nowMonster->entry_object[i].frame != nullptr && nowMonster->entry_object[i].group == CHARA_ENTRY_GROUP_TARGET)
                entry = i;
        }
        for (int i = 0; i < CHARA_ENTRY_OBJECT_MAX && entry < 0; ++i) {
            if (nowMonster->entry_object[i].frame == nullptr) {
                entry = i;
                nowMonster->entry_object[i].size = 1.0f;
            }
        }
        if (entry >= 0) {
            nowMonster->entry_object[entry].frame = frame;
            nowMonster->entry_object[entry].group = CHARA_ENTRY_GROUP_TARGET;
            nowMonster->entry_object[entry].enable = true;
            // DRAN-R2: DC2's cfg gives the TARGET entry and entry_frame[0] the same frame
            // (character.cpp:2283-2287), and effect scripts aim at entry_frame[] (effscript.cpp:4700,
            // _SCN_GET_CHR_ENTOBJ_POS), Max's gun among them. Without this the gun aimed at the
            // converter's neutral root frame (null74_1), 50 units below Dran's body, and always missed.
            nowMonster->entry_frame[0] = frame;
        }
        state->lockon_scale[0] = scale_x;
        state->lockon_scale[1] = scale_y;
    }
    std::fprintf(stderr, "ixion dc1 lockon donor=%s slot=%d frame=%s found=%d entry=%d scale=(%.2f,%.2f)\n",
                 DonorName(state), SlotOf(state), name, frame != nullptr ? 1 : 0, entry, scale_x, scale_y);
    return 1;
}

// Adds a DC2 entry sphere of `group` on a named frame; -1 when the frame or a free entry is missing.
int AddEntrySphere(CActiveMonster *monster, const char *name, int group, float dc1_radius) {
    mgCFrame *frame = monster->SearchObject(const_cast<char *>(name));
    if (frame == nullptr) return -1;
    for (int i = 0; i < CHARA_ENTRY_OBJECT_MAX; ++i) {
        CHARA_ENTRY_OBJECT &entry = monster->entry_object[i];
        if (entry.frame != nullptr) continue;
        entry.frame = frame;
        // DC1 compares centre distance with the sum of radii (collisiondata.hpp:146-152 for hurt
        // spheres, monstorunit.cpp:1712 for push); DC2 tests 2 x (radius + size) (colprim.cpp:1067,
        // actionchara.cpp:1427), so half the scaled DC1 radius keeps DC1's reach.
        entry.size = 0.5f * dc1_radius * kScale;
        entry.group = group;
        entry.enable = true;
        return i;
    }
    return -1;
}

// 130 _SET_BODY_COL, Chronicle runscript_opcodes.cpp:757-802: a hurt sphere on a named frame, with
// an optional motion window. DC1 tests the player's hits against these spheres only
// (monstorunit.cpp:1195-1208, active unless start != 0 and the motion time is outside
// (start, end]). Here each becomes a DC2 HIT (group 2) entry, the group colprim.cpp:1064 tests; the
// window toggles the entry's enable flag each think (IxionDc1AfterScript).
int SetBodyCol(RS_STACKDATA *stack, int argc) {
    FirstCall(130, "_SET_BODY_COL", "implemented: DC2 HIT entry sphere with DC1 window");
    State      *state = Now();
    const char *name = GetString(stack++);
    const float radius = GetFloat(stack++);
    float       start = 0.0f, end = 0.0f;
    if (argc == 4) {
        start = GetFloat(stack++);
        end = GetFloat(stack);
    }
    int entry = -1;
    state->last_body_col = -1; // DC1 bak_ColNo = -1 when the frame is missing (:794-797)
    if (state->body_col_count < 16) {
        entry = AddEntrySphere(nowMonster, name, CHARA_ENTRY_GROUP_HIT, radius);
        if (entry >= 0) {
            const int col = state->body_col_count++;
            state->body_cols[col] = {entry, start, end};
            // :784-790: body_parameter[0..4] and parameter[0..5] start at 100 (full damage).
            for (int j = 0; j < 10; ++j) state->body_parameter[col][j] = j < 5 ? 100 : 0;
            for (int j = 0; j < 6; ++j) state->attacker_percent[col][j] = 100;
            state->last_body_col = col;
        }
    }
    std::fprintf(stderr, "ixion dc1 body col donor=%s slot=%d %s r=%.1f window=%.0f-%.0f -> HIT entry=%d size=%.2f\n",
                 DonorName(state), SlotOf(state), name, radius, start, end, entry, 0.5f * radius * kScale);
    return 1;
}

// 131 _SET_DMG_COL, :824. Recorded for R3 (attack windows into DC2 damage[]).
int SetDmgCol(RS_STACKDATA *stack, int argc) {
    FirstCall(131, "_SET_DMG_COL", "implemented as DC2 damage[] attack window");
    State *state = Now();
    if (state->damage_count >= 16) {
        state->last_damage = -1;
        return 1;
    }
    DamageSphere &sphere = state->damage[state->damage_count];
    std::snprintf(sphere.frame, sizeof sphere.frame, "%s", GetString(stack++));
    sphere.radius = GetFloat(stack++);
    sphere.start = GetFloat(stack++);
    sphere.end = GetFloat(stack);
    state->last_damage = state->damage_count++;
    // DC2's attack-window loop (ThinkHost) turns each damage[] entry into a collision primitive
    // while the motion frame lies in [start, end), as DC1's CheckDmg (monstorunit.cpp:1066-1080)
    // emits the sphere on the named frame by motion time. The primitive's damage settings are the
    // stock row 吹っ飛び攻撃 (a knock-back blow, like DC1 kind 3); its damage is the row's attack
    // (ThinkHost sets prim->damage = monster->attack), not DC1's 35.
    mgCFrame *frame = nowMonster->SearchObject(sphere.frame);
    int       slot = -1;
    for (int i = 0; i < ACTION_DAMAGE_MAX && frame != nullptr; ++i) {
        if (!nowMonster->damage[i].use) {
            slot = i;
            break;
        }
    }
    if (slot >= 0) {
        ACTION_DAMAGE &entry = nowMonster->damage[slot];
        entry.use = true;
        entry.chara = nullptr;
        entry.frame0 = frame;
        entry.frame1 = nullptr;
        // DC1 hits when the player's feet are within the radius (collisiondata.cpp:840-858); DC2's
        // IsHit tests 2 x (radius + hit-entry size) (colprim.cpp), so half the radius keeps DC1's reach.
        entry.radius = 0.5f * sphere.radius * kScale;
        entry.power_rate = 1.0f;
        entry.start_frame = sphere.start;
        entry.end_frame = sphere.end;
        entry.damage = kBlowHit;
        entry.prim = nullptr;
        nowMonster->damage_num++;
        sphere.slot = slot;
    }
    std::fprintf(stderr, "ixion dc1 dmg col %s r=%.1f frames %.0f-%.0f -> DC2 damage[%d] radius=%.1f frame_found=%d\n",
                 sphere.frame, sphere.radius, sphere.start, sphere.end, slot, 0.5f * sphere.radius * kScale,
                 frame != nullptr ? 1 : 0);
    return 1;
}

// 132 _SET_DMG_PARA, :855.
int SetDmgPara(RS_STACKDATA *stack, int argc) {
    FirstCall(132, "_SET_DMG_PARA", "implemented: DC1 damage and kind 2/3 reaction");
    State *state = Now();
    if (state->last_damage < 0) return 1;
    DamageSphere &sphere = state->damage[state->last_damage];
    sphere.damage = GetInt(stack++);
    sphere.flags = GetInt(stack++);
    sphere.kind = GetInt(stack++);
    sphere.angle = sphere.kind == 3 && argc == 4 ? GetFloat(stack) : 0.0f;
    std::fprintf(stderr, "ixion dc1 damage para sphere=%d damage=%d flags=0x%x kind=%d\n", state->last_damage,
                 sphere.damage, sphere.flags, sphere.kind);
    return 1;
}

// 133 _SET_SHOT, :873. The donor's effect (5, 9 or 29) leaves the named frame toward the given
// point (DC1 units), straight at a fixed velocity (shot_effect.cpp:392-480); a fifth argument
// overrides the damage. The shot is a shim-owned DC2 collision primitive on a private row copy.
int SetShot(RS_STACKDATA *stack, int argc) {
    FirstCall(133, "_SET_SHOT", "implemented: shim projectile on a private DC2 row copy");
    State      *state = Now();
    const char *name = GetString(stack++);
    float       target[4];
    target[0] = GetFloat(stack++) * kScale;
    target[1] = GetFloat(stack++) * kScale;
    target[2] = GetFloat(stack++) * kScale;
    target[3] = 1.0f;
    const Donor      *donor = state->donor;
    const ShotEffect *effect = donor != nullptr ? FindShotEffect(donor->shot_effect) : nullptr;
    if (effect == nullptr) {
        std::fprintf(stderr, "ixion dc1 shot refused: donor %s has no supported shot effect\n", DonorName(state));
        return 1;
    }
    const int damage = argc == 5 ? GetInt(stack) : effect->damage;
    mgCFrame *frame = nowMonster->SearchObject(const_cast<char *>(name));
    float     from[4];
    if (frame != nullptr) {
        frame->GetWorldPosition0(from);
    } else {
        nowMonster->GetPosition(from);
        from[1] += 10.0f * kScale;
    }
    from[3] = 1.0f;
    SpawnShot(state, *effect, nowMonster, from, target, damage, nowMonster->now_prog == MONSTER_PROG_DEAD);
    return 1;
}

// 136 _SET_MOV_COL, Chronicle runscript_opcodes.cpp:732-755: a push sphere on a named frame. With
// any of these DC1 pushes the player by them instead of body_radius (monstorunit.cpp:1655-1730).
// Each becomes a DC2 BODY (group 4) entry, the group the player's push reads (actionchara.cpp:1410).
int SetMovCol(RS_STACKDATA *stack, int argc) {
    FirstCall(136, "_SET_MOV_COL", "implemented: DC2 BODY entry sphere");
    State      *state = Now();
    const char *name = GetString(stack++);
    const float radius = GetFloat(stack);
    int         entry = -1;
    if (state->mov_col_count < 12) { // DC1 keeps 12 (runscript_opcodes.cpp:738)
        entry = AddEntrySphere(nowMonster, name, CHARA_ENTRY_GROUP_BODY, radius);
        if (entry >= 0) ++state->mov_col_count;
    }
    std::fprintf(stderr, "ixion dc1 mov col donor=%s slot=%d %s r=%.1f -> BODY entry=%d size=%.2f\n", DonorName(state),
                 SlotOf(state), name, radius, entry, 0.5f * radius * kScale);
    return 1;
}

// 140 _SET_SND_FRM, Chronicle runscript_opcodes.cpp:943-968: arms a frame window on the first free
// slot (only when its cooldown has run out); SoundCheck plays it (IxionDc1AfterScript). Without
// DC2_IXION_DC1_SFX the window is armed but never sounds, and the log is the round-0 line.
int SetSndFrm(RS_STACKDATA *stack, int argc) {
    FirstCall(140, "_SET_SND_FRM", IxionDc1SfxRequested() ? "implemented: DC1 SE window on DC2's mixer"
                                                          : "logged only (DC2_IXION_DC1_SFX unset)");
    State      *state = Now();
    const float start = GetFloat(stack++), end = GetFloat(stack++);
    const int   id = GetInt(stack);
    int         slot = -1;
    for (int i = 0; i < 16; ++i) {
        if (state->snd_id[i] == -1) {
            if (state->snd_cooldown[i] == 0) slot = i;
            break;
        }
    }
    if (slot >= 0) {
        state->snd_start[slot] = start;
        state->snd_end[slot] = end;
        state->snd_id[slot] = id;
    }
    if (!IxionDc1SfxRequested()) {
        std::fprintf(stderr, "ixion dc1 sound window frames=%.1f-%.1f dc1_se=%d (not played)\n", start, end, id);
    } else {
        std::fprintf(stderr, "ixion dc1 sound window donor=%s slot=%d frames=%.1f-%.1f dc1_se=%d window=%d (armed)\n",
                     DonorName(state), SlotOf(state), start, end, id, slot);
    }
    return 1;
}

// 141 _SET_SND_NOW, :1002-1005: SndSePlay(id, -1, 0), the setbl level at the centre.
int SetSndNow(RS_STACKDATA *stack, int argc) {
    FirstCall(141, "_SET_SND_NOW", IxionDc1SfxRequested() ? "implemented: DC1 SE on DC2's mixer"
                                                          : "logged only (DC2_IXION_DC1_SFX unset)");
    const int id = GetInt(stack);
    if (!IxionDc1SfxRequested()) {
        std::fprintf(stderr, "ixion dc1 sound now frame=%d dc1_se=%d (not played)\n", mgPortFrameCount(), id);
        return 1;
    }
    IxionDc1SfxPlay(id, 1.0f, 0.0f, 0, "now", DonorName(Now()), SlotOf(Now()));
    return 1;
}

// 15 _CHK_USER_INNER_PRODUCT, Chronicle runscript_opcodes.cpp:289-327: 1 when the monster's facing
// and the player-to-monster direction (horizontal) have cosine >= 1 - deg/90, and the 3D distance
// is below the optional limit (default 100000). Retail falls off the end without `return 1`; the
// adapter returns 1 like every other native.
int ChkUserInnerProduct(RS_STACKDATA *stack, int argc) {
    FirstCall(15, "_CHK_USER_INNER_PRODUCT", "implemented: DC1 facing cosine and distance test");
    const float min_cosine = 1.0f - 0.011111111f * (float) GetInt(stack++);
    float       max_distance = 100000.0f;
    if (argc == 3) max_distance = GetFloat(stack++);
    float player[4], position[4], rotation[4];
    PlayerPosition(player);
    SelfPosition(position);
    nowMonster->GetRotation(rotation);
    float direction[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    RotateY(direction, rotation[1]);
    Normalize3(direction);
    float away[4] = {position[0] - player[0], 0.0f, position[2] - player[2], 1.0f};
    Normalize3(away);
    const float dot = direction[0] * away[0] + direction[1] * away[1] + direction[2] * away[2];
    SetInt(stack, dot >= min_cosine && Distance(position, player) < max_distance ? 1 : 0);
    return 1;
}

// 109 _STATUS_GET_LIFE_RATE, :564-570: 100 * hp / max_hp as a float. DC2 owns the donor's HP.
int GetLifeRate(RS_STACKDATA *stack, int argc) {
    FirstCall(109, "_STATUS_GET_LIFE_RATE", "implemented: DC2 life / max_life * 100");
    const float max_life = nowMonster->max_life > 0 ? (float) nowMonster->max_life : 1.0f;
    SetFloat(stack, 100.0f * ((float) nowMonster->life / max_life));
    return 1;
}

// 228 _STATUS_SET_LIFE, :1320-1326: hp = (int)(max_hp * rate). Master Jacket's death program uses it
// for its 10% revive (prog_120); DC2's own life drives the gauge, kill and reward.
int SetLife(RS_STACKDATA *stack, int argc) {
    FirstCall(228, "_STATUS_SET_LIFE", "implemented: DC2 life = max_life * rate");
    const float rate = GetFloat(stack);
    const int   before = nowMonster->life;
    nowMonster->life = (int) ((float) nowMonster->max_life * rate);
    std::fprintf(stderr, "ixion dc1 set life frame=%d donor=%s slot=%d rate=%.3f hp=%d->%d/%d prog=%d\n",
                 mgPortFrameCount(), DonorName(Now()), SlotOf(Now()), rate, before, nowMonster->life,
                 nowMonster->max_life, nowMonster->now_prog);
    return 1;
}

// 230 _SET_LOOP_SND, :971-979: records the monster's looped sound (motion window, step, id).
// SoundCheck refreshes it while the motion time is inside the window (IxionDc1AfterScript).
int SetLoopSnd(RS_STACKDATA *stack, int argc) {
    FirstCall(230, "_SET_LOOP_SND", IxionDc1SfxRequested() ? "implemented: DC1 loop sequence on DC2's mixer"
                                                           : "recorded and logged (DC2_IXION_DC1_SFX unset)");
    State *state = Now();
    state->loop_start = GetFloat(stack++);
    state->loop_end = GetFloat(stack++);
    state->loop_step = GetInt(stack++);
    state->loop_id = GetInt(stack);
    std::fprintf(stderr, "ixion dc1 loop sound donor=%s slot=%d frames=%.1f-%.1f step=%d dc1_se=%d %s\n",
                 DonorName(state), SlotOf(state), state->loop_start, state->loop_end, state->loop_step, state->loop_id,
                 IxionDc1SfxRequested() ? "(armed)" : "(not played)");
    return 1;
}

// 232 _DEL_LOOP_SND, :994-1000: the monster stops owning its loop (retail lets the sound play on).
int DelLoopSnd(RS_STACKDATA *stack, int argc) {
    FirstCall(232, "_DEL_LOOP_SND", "stub: loop ownership cleared and logged");
    State *state = Now();
    std::fprintf(stderr, "ixion dc1 loop sound released donor=%s slot=%d dc1_se=%d\n", DonorName(state), SlotOf(state),
                 state->loop_id);
    state->loop_id = -1;
    return 1;
}

// 246 _GET_MOVE_VEC, :1494-1499 returns BtActStatus.script_move_vector. Nothing in Chronicle's
// ps2/src or port/src writes that field (only its declaration, btactstatus.hpp:57, and this reader),
// so DC1 hands the script its zero-initialised value: Dragon's lead term (6 x vec) adds nothing.
int GetMoveVec(RS_STACKDATA *stack, int argc) {
    FirstCall(246, "_GET_MOVE_VEC", "implemented: DC1's never-written script_move_vector (zero)");
    SetFloat(stack++, 0.0f);
    SetFloat(stack++, 0.0f);
    SetFloat(stack, 0.0f);
    return 1;
}

// 112 _STATUS_GET_HEIGHT, Chronicle runscript_opcodes.cpp:605-608: the monster's ground_distance
// (DC1 units above the floor below it). DC2's MoveUnit measures the same distance into `height`
// against the imported room's ground probe, so the value crosses the boundary divided by k.
int GetHeight(RS_STACKDATA *stack, int argc) {
    FirstCall(112, "_STATUS_GET_HEIGHT", "implemented: DC2 ground height / k");
    SetFloat(stack, nowMonster->height > 0.0f ? nowMonster->height / kScale : 0.0f);
    return 1;
}

// 182 _SIN_DEG / 183 _COS_DEG, Chronicle runscript_opcodes.cpp:626-640: sinf/cosf of degrees.
int SinDeg(RS_STACKDATA *stack, int argc) {
    FirstCall(182, "_SIN_DEG", "implemented");
    const float angle = GetFloat(stack++) * (kPi / 180.0f);
    SetFloat(stack, std::sin(angle));
    return 1;
}
int CosDeg(RS_STACKDATA *stack, int argc) {
    FirstCall(183, "_COS_DEG", "implemented");
    const float angle = GetFloat(stack++) * (kPi / 180.0f);
    SetFloat(stack, std::cos(angle));
    return 1;
}

// 207 _GET_NEAR_MONSTER, Chronicle runscript_opcodes.cpp:1388-1418: the nearest other monster on
// the floor (state 2) closer than 240 DC1 units; writes its position and index. Here "the floor"
// is every live donor instance (DC1 keeps 16 per unit; the index is the instance slot). Retail
// leaves the position uninitialised and returns nothing when none is found; the adapter writes
// zeros with index -1 and returns 1.
int GetNearMonster(RS_STACKDATA *stack, int argc) {
    FirstCall(207, "_GET_NEAR_MONSTER", "implemented: nearest live donor instance within 240 DC1");
    State *self = Now();
    float position[4], other[4], found_position[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    SelfPosition(position);
    float nearest = 240.0f;
    int   found = -1;
    for (State &state : g_states) {
        if (&state == self || state.monster == nullptr || Find(state.monster) != &state ||
            state.monster->state != ACTIVE_MONSTER_LIVE || state.monster->life <= 0)
            continue;
        state.monster->GetPosition(other);
        for (int i = 0; i < 3; ++i) other[i] /= kScale;
        const float distance = Distance(position, other);
        if (distance < nearest) {
            nearest = distance;
            found = SlotOf(&state);
            std::memcpy(found_position, other, sizeof found_position);
        }
    }
    SetFloat(stack++, found_position[0]);
    SetFloat(stack++, found_position[1]);
    SetFloat(stack++, found_position[2]);
    SetInt(stack, found);
    static int logged = 0;
    if (logged < 16) {
        ++logged;
        std::fprintf(stderr, "ixion dc1 near monster frame=%d donor=%s slot=%d found=%d (%s) dist_dc1=%.1f\n",
                     mgPortFrameCount(), DonorName(self), SlotOf(self), found,
                     found >= 0 ? DonorName(&g_states[found]) : "-", found >= 0 ? nearest : -1.0f);
    }
    return 1;
}

// 180 _GET_RAND, :610. The port's rand() is retail's 31-bit generator (runtime.cpp).
int GetRand(RS_STACKDATA *stack, int argc) {
    FirstCall(180, "_GET_RAND", "implemented");
    const int bound = GetInt(stack++);
    SetInt(stack, (int) ((float) bound * (float) rand() / 2147483648.0f));
    return 1;
}

// 181 _GET_RANDF, Chronicle runscript_opcodes.cpp:619: intentionally truncated, then stored as float.
int GetRandF(RS_STACKDATA *stack, int argc) {
    FirstCall(181, "_GET_RANDF", "implemented: DC1 integer-valued float roll");
    const float bound = GetFloat(stack++);
    SetFloat(stack, (float) (int) (bound * (float) rand() / 2147483648.0f));
    return 1;
}

// 200 _SET_MOTION, :85. Key n of the converted pack is DC1 motion n. The DC1 call takes effect at
// once, so a pending DC2 key change is reported as "just started" by _CHK_MOTION_FRM.
int SetMotion(RS_STACKDATA *stack, int argc) {
    FirstCall(200, "_SET_MOTION", "implemented (DC1 slow halving not mapped)");
    State *state = Now();
    state->last_hit_damage = -1;
    const int motion = GetInt(stack++);
    float speed = -1.0f;
    int   flags = 0;
    if (argc >= 2) speed = GetFloat(stack++);
    if (argc >= 3) flags = GetInt(stack++);
    CHRINFO_KEY_SET *key = KeyByIndex(nowMonster, motion);
    if (key == nullptr) {
        std::fprintf(stderr, "ixion dc1 _SET_MOTION %d has no key\n", motion);
        return 1;
    }
    nowMonster->SetMotion(motion, flags);
    if ((flags & CHARA_MOTION_RESTART) && nowMonster->now_key == key) {
        nowMonster->frame = (float) key->start_frame;
    }
    if (motion != state->motion_no || (flags & CHARA_MOTION_RESTART)) {
        float position[4];
        nowMonster->GetPosition(position);
        std::fprintf(stderr, "ixion dc1 motion frame=%d donor=%s slot=%d dc1_%02d speed=%.2f flags=%d pos=(%.1f,%.1f,%.1f)\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), motion, speed, flags, position[0],
                     position[1], position[2]);
    }
    state->motion_no = motion;
    state->motion_flags = flags;
    state->motion_speed = speed;
    state->motion_pending = true;
    return 1;
}

// 201 _CHK_MOTION_FRM, :161. Done on the motion's last frame ([end-1, end]; DC2 holds on end).
int ChkMotionFrm(RS_STACKDATA *stack, int argc) {
    FirstCall(201, "_CHK_MOTION_FRM", "implemented");
    State           *state = Now();
    CHRINFO_KEY_SET *key = KeyByIndex(nowMonster, state->motion_no);
    float            frame = nowMonster->frame;
    int              done = 0;
    if (key != nullptr && (state->motion_pending || nowMonster->now_key != key)) {
        frame = (float) key->start_frame;
    } else if (key != nullptr) {
        // DC1 :165-169 is done only while end-1 <= time < end; DC2 holds on end itself, so the upper
        // bound is inclusive. DRAN-R2: without it a frame left over from the previous key (DC2 keeps
        // it through the blend, as DC1 keeps state.time, gameutil.cpp:513-527) counted as done, so
        // Dran's F0005c motion waits returned at once and his volley fired every 5 frames.
        done = frame >= (float) key->end_frame - 1.0f && frame <= (float) key->end_frame ? 1 : 0;
    }
    SetInt(stack++, done);
    if (argc == 2) SetFloat(stack, frame);
    return 1;
}

// Every DC1 number no supported donor uses: logged, never routed to a DC2 native.
int Unimplemented(RS_STACKDATA *stack, int argc) {
    static bool logged = false;
    if (!logged) {
        logged = true;
        std::fprintf(stderr, "ixion dc1 native unimplemented (argc=%d); no DC2 fallthrough\n", argc);
    }
    return 1;
}

using Native = int (*)(RS_STACKDATA *, int);
Native g_table[256];

template<Native Function>
int CheckedNative(RS_STACKDATA *stack, int argc) {
    // A saved table may be called after retirement or for a different active slot.
    if (Now() == nullptr) return 0;
    return Function(stack, argc);
}

void BuildTable() {
    for (Native &slot : g_table) slot = Unimplemented;
    g_table[10] = CheckedNative<GetDistance>;
    g_table[11] = CheckedNative<GetPosition>;
    g_table[12] = CheckedNative<SetRotation>;
    g_table[13] = CheckedNative<ChkRotation>;
    g_table[14] = CheckedNative<ChkMove>;
    g_table[15] = CheckedNative<ChkUserInnerProduct>;
    g_table[30] = CheckedNative<GetVector>;
    g_table[31] = CheckedNative<GetDirection>;
    g_table[32] = CheckedNative<SetMove>;
    g_table[33] = CheckedNative<ChkMoveInfo>;
    g_table[34] = CheckedNative<SetMoveCancel>;
    g_table[35] = CheckedNative<SetRotCancel>;
    g_table[36] = CheckedNative<SetPosition>;
    g_table[100] = CheckedNative<SetFall>;
    g_table[101] = CheckedNative<SetMuteki>;
    g_table[102] = CheckedNative<SetAlpha>;
    g_table[103] = CheckedNative<ChkAlpha>;
    g_table[104] = CheckedNative<SetDead>;
    g_table[105] = CheckedNative<SetPallet>;
    g_table[106] = CheckedNative<SetClipLevel>;
    g_table[108] = CheckedNative<SetColOff>;
    g_table[111] = CheckedNative<RunScript>;
    g_table[116] = CheckedNative<SetLockonDist>;
    g_table[134] = CheckedNative<SetBodyColPara>;
    g_table[202] = CheckedNative<GetMotionFrm>;
    g_table[109] = CheckedNative<GetLifeRate>;
    g_table[113] = CheckedNative<GetHitDamage>;
    g_table[114] = CheckedNative<GetMotionId>;
    g_table[115] = CheckedNative<GetDamageId>;
    g_table[117] = CheckedNative<SetShadowLen>;
    g_table[118] = CheckedNative<SetLockonTrg>;
    g_table[130] = CheckedNative<SetBodyCol>;
    g_table[131] = CheckedNative<SetDmgCol>;
    g_table[132] = CheckedNative<SetDmgPara>;
    g_table[133] = CheckedNative<SetShot>;
    g_table[136] = CheckedNative<SetMovCol>;
    g_table[140] = CheckedNative<SetSndFrm>;
    g_table[141] = CheckedNative<SetSndNow>;
    g_table[112] = CheckedNative<GetHeight>;
    g_table[180] = CheckedNative<GetRand>;
    g_table[181] = CheckedNative<GetRandF>;
    g_table[182] = CheckedNative<SinDeg>;
    g_table[183] = CheckedNative<CosDeg>;
    g_table[207] = CheckedNative<GetNearMonster>;
    g_table[200] = CheckedNative<SetMotion>;
    g_table[201] = CheckedNative<ChkMotionFrm>;
    g_table[210] = CheckedNative<GetCharacterId>;
    g_table[227] = CheckedNative<SetLockonSw>;
    g_table[228] = CheckedNative<SetLife>;
    g_table[230] = CheckedNative<SetLoopSnd>;
    g_table[232] = CheckedNative<DelLoopSnd>;
    g_table[242] = CheckedNative<SetGravity>;
    g_table[244] = CheckedNative<SetGuardFrame>;
    g_table[245] = CheckedNative<GuardSearch>;
    g_table[246] = CheckedNative<GetMoveVec>;
}

// The DC2 row: a stand-in early-floor row (Froggy, id 8: sound bank, attack, defence, drops) with
// the DC1 body and script and the donor's DC1 life, defence and rewards. One per roster kind,
// under its own id 0x7000 + kind (outside the book; CountKill bounds it, quest.cpp:217).
BASE_MONSTER_TBL *Row(int monster_id) {
    const Donor *donor = DonorForId(monster_id);
    if (donor == nullptr) return nullptr;
    DonorRow &cached = g_rows[{donor, monster_id}];
    BASE_MONSTER_TBL &g_row = cached.row;
    bool &g_row_ready = cached.ready;
    if (g_row_ready) return &g_row;
    for (BASE_MONSTER_TBL *entry = base_monster_define; entry->name[0] != 0; ++entry) {
        if (entry->id == 8) {
            g_row = *entry;
            g_row_ready = true;
            break;
        }
    }
    if (!g_row_ready) return nullptr;
    g_row.id = (s16) monster_id;
    std::snprintf(g_row.name, sizeof g_row.name, "%s", donor->name.c_str());
    std::snprintf(g_row.model, sizeof g_row.model, "ix_%s", donor->id.c_str());
    std::snprintf(g_row.script, sizeof g_row.script, "ix_%s", donor->id.c_str());
    g_row.life = donor->life;
    g_row.reward_exp = donor->experience;
    g_row.reward_money = donor->money;
    std::memcpy(g_row.drop_item, donor->drops, sizeof donor->drops);
    // DC1 scripts own hit/guard/dodge decisions. Disable DC2's randomized guard/escape/rage;
    // damaging hits still enter prog_110, and the donor profile owns defence and drops.
    // Froggy's ext_param (all 100, so Ridepod kinds 6-9 deal damage) is retained.
    g_row.stagger = 0;
    g_row.flags = 0;
    g_row.guard_rate = 0;
    g_row.escape_rate0 = 0;
    g_row.escape_rate1 = 0;
    g_row.gekirin_num = 0;
    g_row.defense = donor->defense;
    g_row.next_id = -1;
    g_row.share_model = 0;
    g_row.boss = 0;
    g_row.sw_effect_num = 0;
    return &g_row;
}

// Copies the DC1 STB with its program table rebuilt for DC2's program numbers. DC2's RunScript
// looks the requested number up; every DC2 reaction number maps to the DC1 program for it.
bool PatchProgram(MONSTER_REFER &refer, const Donor *donor) {
    if (donor == nullptr) return false;
    if (auto built = g_programs.find(donor->id); built != g_programs.end() && !built->second.empty()) {
        refer.script = reinterpret_cast<char *>(built->second.data());
        return true;
    }
    const std::string script_path = std::string("dungeon/monster/ix_") + donor->id + ".stb";
    const auto path = DataReadFind(script_path.c_str());
    if (!path) return false;
    std::ifstream input(*path, std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (bytes.size() < sizeof(RS_PROG_HEADER)) return false;
    const auto *header = reinterpret_cast<const RS_PROG_HEADER *>(bytes.data());
    if (header->prog < 0 || header->prog_num <= 0 ||
        (size_t) header->prog + (size_t) header->prog_num * sizeof(RS_PROGDATA) > bytes.size()) {
        return false;
    }
    int setup = -1, main = -1, hit = -1, death = -1, reveal = -1;
    const auto *entries = reinterpret_cast<const RS_PROGDATA *>(bytes.data() + header->prog);
    for (int i = 0; i < header->prog_num; ++i) {
        if (entries[i].no == kDc1Setup) setup = entries[i].func;
        if (entries[i].no == kDc1Main) main = entries[i].func;
        if (entries[i].no == kDc1Hit) hit = entries[i].func;
        if (entries[i].no == kDc1Death) death = entries[i].func;
        if (entries[i].no == kDc1Reveal) reveal = entries[i].func;
    }
    if (setup < 0 || main < 0 || hit < 0 || death < 0) return false;
    // DC1 prog_50 has no DC2 request; it becomes kRevealProgram, requested by the chest opening
    // (IxionDc1HiddenStep). Scripts without it keep the table below unchanged in meaning.
    const RS_PROGDATA table[] = {
        {kRevealProgram, reveal >= 0 ? reveal : main},
        {kSetupProgram, setup},
        {MONSTER_PROG_MAIN, main},
        {MONSTER_PROG_STATUS, hit},
        {MONSTER_PROG_DAMAGE, hit},
        {MONSTER_PROG_KNOCK, hit},
        {MONSTER_PROG_GUARD, hit},
        {MONSTER_PROG_PIYORI, hit},
        {MONSTER_PROG_DEAD, death},
        // No DC1 counterpart: dodges, catch and landing resume the main program.
        {MONSTER_PROG_ESCAPE_0, main},
        {MONSTER_PROG_ESCAPE_1, main},
        {MONSTER_PROG_CATCH, main},
        {MONSTER_PROG_LAND, main},
    };
    const size_t table_offset = (bytes.size() + 15) & ~size_t(15);
    const size_t total = table_offset + sizeof table;
    std::vector<u_long128> &program = g_programs[donor->id];
    program.assign((total + 15) / 16, u_long128{});
    char *base = reinterpret_cast<char *>(program.data());
    std::memcpy(base, bytes.data(), bytes.size());
    std::memcpy(base + table_offset, table, sizeof table);
    auto *patched = reinterpret_cast<RS_PROG_HEADER *>(base);
    patched->prog = (int) table_offset;
    patched->prog_num = (int) (sizeof table / sizeof table[0]);
    refer.script = base;
    g_has_reveal[donor->id] = reveal >= 0;
    std::fprintf(stderr,
                 "ixion dc1 program table: donor=%s magic=%.3s size=%zu dc1 progs setup@0x%x main@0x%x hit@0x%x death@0x%x "
                 "reveal@%s -> DC2 %d/200/400,500,600,700,1400/1000/%d\n",
                 donor->id.c_str(), header->magic, bytes.size(), setup, main, hit, death,
                 reveal >= 0 ? std::to_string(reveal).c_str() : "none", kSetupProgram, kRevealProgram);
    return true;
}

} // namespace
} // namespace ixion::dc1rt

using namespace ixion::dc1rt;

float IxionDc1BalanceHp() { return ixion::dc1rt::BalanceHp(); }

int IxionDc1ScaledDamage(int dc1_damage, float max_hp, float dc1_hp) {
    const float net = std::fmax(0.0f, (float) dc1_damage - kDc1PlayerDefense);
    const int damage = (int) std::lround(net * max_hp / dc1_hp);
    return damage > 0 ? damage : 1;
}

int IxionDc1PortEventId(int dc1_event) {
    return dc1_event == kDc1EventDranDefeated ? kPortEventBase + dc1_event : -1;
}

int IxionDc1HitIdFor(int dc2_kind) {
    const Dc1Attacker attacker = AttackerFor(dc2_kind);
    if (attacker.owner < 0) return -1;
    return attacker.owner * 10 + attacker.attack; // monstorunit.cpp:1281-1290
}

int IxionDc1BodyAttackerPercent(const CActiveMonster *monster, int col, int dc1_owner) {
    const State *state = Find(monster);
    if (state == nullptr || col < 0 || col >= state->body_col_count || dc1_owner < 0 || dc1_owner > 5) return 100;
    return state->attacker_percent[col][dc1_owner];
}

int IxionDc1PercentPrimDamage(int prim_damage, int defense, int percent) {
    // DC1 scales its damage after the weapon terms (monstorunit.cpp:1521-1527); DC2 subtracts
    // defence from the prim damage first (monster.cpp:2046), so the percent applies to the net part.
    // DC1 order (monstorunit.cpp): damage -= defence, at least 1 (:1486-1496); then the attacker percent
    // (:1521-1527); then rounded up (:1554-1558, amount = ceil(damage)). DC2 subtracts the defence
    // again from what is returned (monster.cpp:2046), so the result is defence + the DC1 amount.
    const float net = std::fmax(1.0f, (float) (prim_damage - defense));
    const float scaled = net * 0.01f * (float) std::max(0, percent);
    return defense + (int) std::ceil(scaled - 1e-4f);
}

int IxionDc1NoteBodyHit(CActiveMonster *monster, int dc2_kind, int entry, int prim_damage) {
    State *state = Find(monster);
    if (state == nullptr) return prim_damage;
    const Dc1Attacker attacker = AttackerFor(dc2_kind);
    int col = -1;
    for (int i = 0; i < state->body_col_count && col < 0; ++i)
        if (state->body_cols[i].entry == entry) col = i;
    const int percent = attacker.owner >= 0 ? IxionDc1BodyAttackerPercent(monster, col, attacker.owner) : 100;
    // DRAN-R2 debug knob (stated in the receipt): DC2_IXION_DC1_DEBUG_ATTACK_SCALE=<n> multiplies the
    // player's prim damage against donors before the DC1 rule, so the 134 percent is visible in HP.
    static const int attack_scale = [] {
        const char *text = std::getenv("DC2_IXION_DC1_DEBUG_ATTACK_SCALE");
        const int value = text != nullptr ? std::atoi(text) : 1;
        return value >= 1 && value <= 100 ? value : 1;
    }();
    if (attack_scale != 1)
        std::fprintf(stderr, "ixion dc1 DEBUG attack scale x%d: prim damage %d -> %d (DC2_IXION_DC1_DEBUG_ATTACK_SCALE)\n",
                     attack_scale, prim_damage, prim_damage * attack_scale);
    prim_damage *= attack_scale;
    const int damage = IxionDc1PercentPrimDamage(prim_damage, monster->defense, percent);
    // DC1 records the id only for a hit it accepts, never while invincible (monstorunit.cpp:1189).
    if (monster->muteki_time <= 0) {
        state->last_hit_id = IxionDc1HitIdFor(dc2_kind);
        // Applied next think only if DC2 itself requested no reaction (IxionDc1BeforeScript).
        state->dc1_hit_request = true;
    }
    const char *frame = entry >= 0 && entry < CHARA_ENTRY_OBJECT_MAX && monster->entry_object[entry].frame != nullptr
                            ? monster->entry_object[entry].frame->name : "-";
    if (attacker.owner == 1) Capture(13, "body-hit-owner1");
    if (attacker.owner == 0) Capture(14, "body-hit-owner0");
    std::fprintf(stderr, "ixion dc1 body hit frame=%d donor=%s slot=%d sphere=%d frame_name=%s dc2_kind=%d -> dc1 owner=%d attack=%d hit_id=%d "
                         "percent=%d prim_damage=%d->%d defense=%d net=%d->%d muteki=%d hp=%d/%d\n",
                 mgPortFrameCount(), DonorName(state), SlotOf(state), col, frame != nullptr ? frame : "-", dc2_kind,
                 attacker.owner, attacker.attack, state->last_hit_id, percent, prim_damage, damage, monster->defense,
                 std::max(0, prim_damage - monster->defense), std::max(0, damage - monster->defense), monster->muteki_time,
                 monster->life, monster->max_life);
    return damage;
}

int IxionDc1ColOffForTest(const CActiveMonster *monster) {
    const State *state = Find(monster);
    return state != nullptr ? state->col_off : -1;
}

bool IxionDc1ShotEffectForTest(int effect, float &speed, float &radius, int &life, int &damage, float &impact_radius) {
    const ShotEffect *row = FindShotEffect(effect);
    if (row == nullptr) return false;
    speed = row->speed;
    radius = row->radius;
    life = row->life;
    damage = row->damage;
    impact_radius = row->impact_radius;
    return true;
}

// Retail colprim.cpp:893-932 with an observer at actual player projectile emission.
// DC1 gameloop.cpp:7743,7782,7933,7960,8023,8036,8050,8194,8273,8312,8376
// resets attack age for each shot/burst tick, independently of melee announcements.
PORT_OVERRIDE int CColPrim::SetDamage(char *name, int owner_id) {
    int index = 0;
    DAMAGE_PARAM *row = Damage_Param_Table;
    for (;;) {
        if (row->name[0] == '\0') return false;
        if (std::strcmp(row->name, name) == 0) {
            Initialize();
            param_no = index;
            active = true;
            param = row;
            owner = owner_id;
            damage = row->damage;
            step_count = 0;
            coord_type = COLPRIM_COORD_VECTOR;
            attacker = -1;
            range = 10000.0f;
            std::memcpy(element, row->element, sizeof element);
            status = row->status;
            target = row->target & DAMAGE_TARGET_BY_OWNER
                ? (owner_id == 0 ? DAMAGE_TARGET_MONSTER : DAMAGE_TARGET_PLAYER) : row->target;
            if (owner_id == 0 && (target & DAMAGE_TARGET_MONSTER) && row->kind != DAMAGE_KIND_CHECK &&
                row->kind != DAMAGE_KIND_MAX_MELEE && row->kind != DAMAGE_KIND_MONICA_MELEE)
                PlayerAttackStarted();
            // DRAN-R2 opt-in trace: which player attack kinds are emitted (gun vs melee evidence).
            static const bool trace = [] {
                const char *text = std::getenv("DC2_IXION_DC1_PLAYER_PRIM_TRACE");
                return text != nullptr && std::string_view(text) == "1";
            }();
            static int traced = 0;
            if (trace && owner_id == 0 && traced < 64) {
                ++traced;
                std::fprintf(stderr, "ixion dc1 player prim frame=%d kind=%d damage=%d target=0x%x\n", mgPortFrameCount(),
                             row->kind, damage, target);
            }
            return true;
        }
        ++index;
        ++row;
    }
}

// CMonsterMan::GetReferPtr2 (ps2/src/monster.cpp) with the Ixion DC1 row added under its own id.
PORT_OVERRIDE BASE_MONSTER_TBL *CMonsterMan::GetReferPtr2(int monster_id) {
    if (IsDonorId(monster_id) && DonorForId(monster_id) != nullptr) {
        return Row(monster_id);
    }
    BASE_MONSTER_TBL *entry = base_monster_define;
    while (entry->name[0] != 0) {
        if (entry->id == monster_id) {
            return entry;
        }
        entry++;
    }
    return NULL;
}

bool IxionDc1DonorRequested() {
    return SelectedDonor() != nullptr;
}

bool IxionDc1IsDonor(const CActiveMonster *monster) { return Find(monster) != nullptr; }

void IxionDc1InstallNatives(CActiveMonster *monster) {
    IxionDc1Forget(monster); // slot replacement: the previous occupant's shots and prims end
    if (LiveInstances() == 0) {
        g_paused = false;
        g_pause_trace_frame = -1;
        g_paused_prims.clear();
        std::fill(std::begin(g_captured), std::end(g_captured), false);
    }
    State *state = nullptr;
    for (State &candidate : g_states) {
        if (candidate.monster == nullptr) { state = &candidate; break; }
    }
    BuildTable();
    if (state == nullptr) {
        // Every native then refuses (CheckedNative finds no context); the monster stays inert.
        std::fprintf(stderr, "ixion dc1 install refused: %d donor instances live\n", kInstanceMax);
        monster->mons_script.ext_func(g_table, 256);
        return;
    }
    *state = State{};
    state->monster = monster;
    state->donor = DonorForId(monster->monster_id);
    state->owner = monster->chara_type;
    monster->mons_script.ext_func(g_table, 256);
    // Opt-in DC1 SE banks on DC2's mixer (DC2_IXION_DC1_SFX); without it this is a no-op.
    if (state->donor != nullptr) IxionDc1SfxAcquire();
    // _SET_MURDEROUS is file-local in retail, so observe it through a private copy of
    // the host player's table. The original function still owns argument checks/writes.
    if (MainChara__2 != nullptr) {
        auto &script = MainChara__2->script;
        if (script.ext_func_table != nullptr && script.ext_func_table != g_player_table &&
            script.ext_func_num > ACTION_EXT_SET_MURDEROUS && script.ext_func_num <= ACTION_EXT_FUNC_MAX) {
            g_original_player_table = script.ext_func_table;
            std::memcpy(g_player_table, script.ext_func_table, script.ext_func_num * sizeof g_player_table[0]);
            // DC1 charakey.cpp:72,239,286,332,378,626,749,802,951,1121:
            // accepted swings/combos and charge starts reset age, not held input.
            if (g_player_table[ACTION_EXT_SET_MURDEROUS] != nullptr)
                g_player_table[ACTION_EXT_SET_MURDEROUS] = ObservePlayerAttack<ACTION_EXT_SET_MURDEROUS>;
            if (g_player_table[ACTION_EXT_SET_ACCUME_FLAG] != nullptr)
                g_player_table[ACTION_EXT_SET_ACCUME_FLAG] = ObservePlayerAttack<ACTION_EXT_SET_ACCUME_FLAG>;
            script.ext_func(g_player_table, script.ext_func_num);
        }
    }
}

void IxionDc1BeforeScript(CActiveMonster *monster) {
    State *state = Find(monster);
    if (state != nullptr && state->guard_pending) {
        // DngStep calls native CheckDamage after ThinkHost. Sample on the following tick,
        // independently of the collision snapshot, after native HP processing.
        std::fprintf(stderr, "ixion dc1 guard blocked frame=%d donor=%s slot=%d motion=%d mframe=%.2f incoming=%d hp=%d->%d window=%d-%d kind=%d sampled_frame=%d\n",
                     state->guard_frame, DonorName(state), SlotOf(state), state->motion_no, state->guard_motion_frame, state->guard_incoming,
                     state->guard_life_before, monster->life, state->guard_start[state->guard_slot],
                     state->guard_end[state->guard_slot], state->guard_kind, mgPortFrameCount());
        state->guard_pending = false;
    }
    if (state != nullptr && state->frames_since_attack < 3600) ++state->frames_since_attack;
    if (state != nullptr && state->dc1_hit_request) {
        state->dc1_hit_request = false;
        if (monster->req_prog == MONSTER_PROG_RUNNING && monster->life > 0 && monster->now_prog != MONSTER_PROG_DEAD &&
            monster->state == ACTIVE_MONSTER_LIVE) {
            monster->req_prog = MONSTER_PROG_DAMAGE;
            std::fprintf(stderr, "ixion dc1 hit without DC2 reaction -> DC1 prog_110 frame=%d donor=%s slot=%d last_hit_id=%d (DC1 runs the hit program for every accepted hit)\n",
                         mgPortFrameCount(), DonorName(state), SlotOf(state), state->last_hit_id);
        }
    }
}

void IxionDc1RecordPlayerDamage(const CColPrim *prim, int damage) {
    if (prim == nullptr) return;
    State *state = nullptr;
    for (State &candidate : g_states) {
        if (candidate.monster != nullptr && Find(candidate.monster) == &candidate &&
            prim->owner == candidate.monster->chara_type) state = &candidate;
    }
    if (state == nullptr) return;
    state->last_hit_damage = damage;
    std::fprintf(stderr, "ixion dc1 outgoing hit frame=%d donor=%s slot=%d owner=%d motion=%d damage=%d (native 113)\n",
                 mgPortFrameCount(), DonorName(state), SlotOf(state), prim->owner, state->motion_no, damage);
}

bool IxionDc1PauseRequested(int mode, bool script_running, unsigned pause_flags) {
    // Menus do not run DngStep; FIELD resumes only after the durable event state clears.
    return mode != DNG_STATUS_FIELD || script_running ||
        (pause_flags & (DNG_PAUSE_MONSTER_AI | DNG_PAUSE_ENEMY_STEP | DNG_PAUSE_FIELD_STEP |
                        DNG_PAUSE_PLAYER_STEP | DNG_PAUSE_PLAYER_CONTROL | DNG_PAUSE_PLAYER_STATUS)) != 0;
}

bool IxionDc1UpdatePause() {
    if (LiveInstances() == 0 && g_paused_prims.empty() && IxionDc1PlayerShotsLive() == 0) return false;
    CScene *scene = ActiveMonster != nullptr ? ActiveMonster->scene : nullptr;
    if (scene == nullptr) return false;
    DNG_BATTLE_AREA &area = scene->battle_area;
    const bool paused = IxionDc1PauseRequested(DngStatus.mode, area.script.running != 0, area.pause_flag);
    if (paused && mgPortFrameCount() >= 1800) Capture(11, "dialogue-held");
    // Bounded, opt-in floor-entry trace requested by wave4-donor-pause; never a pause signal.
    const int frame = mgPortFrameCount();
    if (std::getenv("DC2_IXION_DC1_CAPTURE_DIR") != nullptr && frame >= 1690 && frame <= 1860 &&
        frame % 10 == 0 && frame != g_pause_trace_frame) {
        g_pause_trace_frame = frame;
        auto *battle = GetBattleCharaInfo();
        std::fprintf(stderr, "ixion dc1 pause trace frame=%d paused=%d mode=%d script_running=%d pause=0x%x event_request=%d event_mes=%d episode=%d player_hp=%d\n",
                     frame, paused, DngStatus.mode, area.script.running, area.pause_flag, scene->event_run,
                     EventMess != nullptr ? EventMess->State() : -1, StartupEpisodeTitle.state,
                     battle != nullptr ? battle->GetNowHp_i() : -1);
    }
    if (paused == g_paused) return paused;
    if (paused) {
        auto freeze = [](CColPrim *prim, int slot) {
            if (prim == nullptr || !prim->active) return;
            TrackPrim(prim, slot);
            g_paused_prims.push_back({prim, prim->target, prim->owner, prim->param, slot});
            // Keep allocated: active=false would let native GetPrim reuse a frozen projectile.
            prim->target = 0;
        };
        for (State &state : g_states) {
            if (state.monster == nullptr || Find(state.monster) != &state) continue;
            for (ACTION_DAMAGE &entry : state.monster->damage) freeze(entry.prim, SlotOf(&state));
        }
        for (Shot &shot : g_shots) if (shot.live) freeze(shot.prim, shot.slot);
    } else {
        for (const PausedPrim &entry : g_paused_prims) {
            if (entry.prim->active && entry.prim->owner == entry.owner && entry.prim->param == entry.param)
                entry.prim->target = entry.target;
        }
        g_paused_prims.clear();
    }
    g_paused = paused;
    auto *battle = GetBattleCharaInfo();
    const State *first = nullptr;
    for (const State &state : g_states) {
        if (first == nullptr && state.monster != nullptr && Find(state.monster) == &state) first = &state;
    }
    std::fprintf(stderr, "ixion dc1 %s frame=%d mode=%d script_running=%d pause=0x%x prims=%zu donors=%d mframe=%.2f hp=%d player_hp=%d\n",
                 paused ? "pause" : "resume", mgPortFrameCount(), DngStatus.mode, area.script.running,
                 area.pause_flag, g_paused_prims.size(), LiveInstances(),
                 first != nullptr ? first->monster->frame : -1.0f, first != nullptr ? first->monster->life : -1,
                 battle != nullptr ? battle->GetNowHp_i() : -1);
    if (!paused && mgPortFrameCount() >= 1700) Capture(10, "dialogue-resume");
    return paused;
}

bool IxionDc1PrimPaused(const CColPrim *prim) {
    for (const PausedPrim &entry : g_paused_prims) if (entry.prim == prim) return true;
    return false;
}

bool IxionDc1GuardHit(CActiveMonster *monster, CColPrim *prim) {
    State *state = Find(monster);
    if (state == nullptr || prim == nullptr || prim->param == nullptr || prim->has_gift ||
        prim->param->kind == DAMAGE_KIND_CHECK) return false;
    for (int i = 0; i < state->guard_count; ++i) {
        // DC1 monstorunit.cpp:1237-1269: unconditional full block, all directions.
        if (monster->frame < state->guard_start[i] || monster->frame > state->guard_end[i]) continue;
        const int life_before = monster->life;
        if (ActiveMonster != nullptr && ActiveMonster->scene != nullptr)
            GuardEffectSet(ActiveMonster->scene, prim->hit_pos, 0);
        // DC1 consumes .1 weapon durability on a guarded sword hit (monstorunit.cpp:1256).
        // Native DC2 owns its durability bookkeeping; donor hardness/weapon-special wear differs.
        if (MainChara__2 != nullptr && (prim->param->kind == DAMAGE_KIND_MAX_MELEE ||
                                      prim->param->kind == DAMAGE_KIND_MONICA_MELEE)) {
            if (auto *battle = GetBattleCharaInfo()) battle->AddWhp(0, -0.1f);
        }
        // DC1 Toan's ordinary swings also use knockback_mode=2 (charakey.cpp:125-164).
        // Map melee's native push, not only a heavy/knockdown attack, at the DC1 1.5 multiplier.
        if (prim->param->kind == DAMAGE_KIND_MAX_MELEE || prim->param->kind == DAMAGE_KIND_MONICA_MELEE ||
            (prim->param->hit_flags & DAMAGE_HIT_KNOCKDOWN)) {
            float pos[4];
            monster->GetPosition(pos);
            sceVu0SubVector(monster->blow_vec, pos, prim->origin);
            monster->blow_vec[1] = 0;
            sceVu0Normalize(monster->blow_vec, monster->blow_vec);
            monster->blow_speed = 3.0f; // 1.5 times the native unguarded push (2.0).
            monster->blow_rate = 1.0f;
            monster->blow_decel = 0;
            monster->blow_time = 3;
        }
        if (MainChara__2 != nullptr && ActiveMonster != nullptr && ActiveMonster->scene != nullptr)
            sndSePlay(ActiveMonster->scene->se_battle_id, 0x26, 0);
        state->guard_pending = true;
        state->guard_life_before = life_before;
        state->guard_frame = mgPortFrameCount();
        state->guard_slot = i;
        state->guard_motion_frame = monster->frame;
        state->guard_incoming = prim->damage;
        state->guard_kind = prim->param->kind;
        Capture(8, "guard-block");
        return true; // IsHit already consumed the collision; do not start DC2 prog 700/110.
    }
    return false;
}

// Retail primitive manager loops with donor-only pause and guard at the hit boundary.
PORT_OVERRIDE void CColPrimMan::Step() {
    IxionDc1UpdatePause(); // runs even when DNG_PAUSE_MONSTER_AI skipped ThinkHost
    RestoreScaledPrims();
    for (int i = 0; i < COLPRIM_MAX; ++i) {
        if (!IxionDc1PrimPaused(&prim[i])) prim[i].Step();
    }
}

PORT_OVERRIDE CColPrim *CColPrimMan::IsReversVec(CColPrim *attack) {
    IxionDc1UpdatePause();
    for (int i = 0; i < COLPRIM_MAX; ++i) {
        // Untargeting a frozen projectile alone does not stop native reflection queries.
        if (attack->id != i && !IxionDc1PrimPaused(&prim[i]) && prim[i].IsReversVec(attack))
            return &prim[i];
    }
    return nullptr;
}

PORT_OVERRIDE CColPrim *CColPrimMan::CheckHit(int chara_id) {
    const bool paused = IxionDc1UpdatePause();
    auto *monster = scene != nullptr && chara_id >= MONSTER_ACTIVE_MAX && chara_id < 2*MONSTER_ACTIVE_MAX
        ? static_cast<CActiveMonster *>(scene->GetCharacter(chara_id)) : nullptr;
    if (IxionDc1IsDonor(monster) && (paused || monster->life <= 0 || monster->now_prog == MONSTER_PROG_DEAD))
        return nullptr; // no HP/status changes during events or a running DC1 death program
    RestoreScaledPrims(); // a 134 percent applies to one monster's CheckDamage only
    // DRAN-R2 opt-in trace: how near each live player gun prim passes the donor's HIT spheres.
    static const bool near_trace = [] {
        const char *text = std::getenv("DC2_IXION_DC1_PLAYER_PRIM_TRACE");
        return text != nullptr && std::string_view(text) == "1";
    }();
    static int near_logs = 0;
    if (near_trace && near_logs < 200 && IxionDc1IsDonor(monster)) {
        for (int i = 0; i < COLPRIM_MAX; ++i) {
            const CColPrim &p = prim[i];
            if (!p.active || p.param == nullptr || p.owner != 0 || p.param->kind != DAMAGE_KIND_MAX_GUN) continue;
            float best = 1e30f, need = 0.0f;
            int best_entry = -1;
            for (int e = 0; e < CHARA_ENTRY_OBJECT_MAX; ++e) {
                const CHARA_ENTRY_OBJECT &object = monster->entry_object[e];
                if (object.frame == nullptr || object.group != CHARA_ENTRY_GROUP_HIT || !object.enable) continue;
                float position[4];
                object.frame->GetWorldPosition0(position);
                const float d = mgDistVector(const_cast<float *>(p.pos[0]), position);
                if (d - 2.0f * (p.radius + object.size) < best) {
                    best = d - 2.0f * (p.radius + object.size);
                    need = 2.0f * (p.radius + object.size);
                    best_entry = e;
                }
            }
            if (best > 30.0f) continue;
            ++near_logs;
            if (near_logs % 10 == 1) {
                float root[4], nearest[4] = {}, target[4] = {};
                monster->GetPosition(root);
                if (best_entry >= 0) monster->entry_object[best_entry].frame->GetWorldPosition0(nearest);
                monster->GetEntryObjectPos(CHARA_ENTRY_GROUP_TARGET, 0, target);
                std::fprintf(stderr, "ixion dc1 gun aim geometry frame=%d root=(%.1f,%.1f,%.1f) center=(%.1f,%.1f,%.1f) target_entry=(%.1f,%.1f,%.1f) nearest_hit=(%.1f,%.1f,%.1f)\n",
                             mgPortFrameCount(), root[0], root[1], root[2], monster->center_pos[0], monster->center_pos[1],
                             monster->center_pos[2], target[0], target[1], target[2], nearest[0], nearest[1], nearest[2]);
            }
            std::fprintf(stderr, "ixion dc1 gun prim near frame=%d prim=%d pos=(%.1f,%.1f,%.1f) shape=0x%x radius=%.2f hit_mask=0x%x nearest_entry=%d gap=%.1f (reach %.1f)\n",
                         mgPortFrameCount(), i, p.pos[0][0], p.pos[0][1], p.pos[0][2], p.param->shape, p.radius, p.hit_mask,
                         best_entry, best, need);
        }
    }
    for (int i = 0; i < COLPRIM_MAX; ++i) {
        if (prim[i].IsHit(scene, chara_id)) {
            if (Shot *player_shot = PlayerShotOf(&prim[i])) {
                player_shot->hit_chara = chara_id;
                player_shot->hit_life = monster != nullptr ? monster->life : -1;
            }
            if (IxionDc1GuardHit(monster, &prim[i])) return nullptr;
            if (IxionDc1IsDonor(monster) && prim[i].param != nullptr && prim[i].param->kind != DAMAGE_KIND_CHECK &&
                !prim[i].has_gift) {
                // IsHit leaves hit_pos at the HIT entry it touched (colprim.cpp:1078-1080): find that entry.
                int entry = -1;
                float best = 1e30f;
                for (int e = 0; e < CHARA_ENTRY_OBJECT_MAX; ++e) {
                    const CHARA_ENTRY_OBJECT &object = monster->entry_object[e];
                    if (object.frame == nullptr || object.group != CHARA_ENTRY_GROUP_HIT || !object.enable) continue;
                    float position[4];
                    object.frame->GetWorldPosition0(position);
                    const float dx = position[0] - prim[i].hit_pos[0], dy = position[1] - prim[i].hit_pos[1],
                                dz = position[2] - prim[i].hit_pos[2];
                    const float distance = dx * dx + dy * dy + dz * dz;
                    if (distance < best) { best = distance; entry = e; }
                }
                const int damage = IxionDc1NoteBodyHit(monster, prim[i].param->kind, entry, prim[i].damage);
                if (damage != prim[i].damage) {
                    g_scaled_prims.push_back({&prim[i], prim[i].damage});
                    prim[i].damage = damage;
                }
            }
            return &prim[i];
        }
    }
    return nullptr;
}

void IxionDc1Forget(const CActiveMonster *monster) {
    // Pointer comparison only: after an arena reset the monster may no longer exist.
    State *state = Slot(monster);
    if (state == nullptr) return;
    const int slot = SlotOf(state);
    for (Shot &shot : g_shots) if (shot.live && !shot.player_owned && shot.slot == slot) EndShot(shot, "forget");
    for (const PausedPrim &entry : g_owned_prims) {
        if (entry.slot == slot && entry.prim->active && entry.prim->owner == entry.owner && entry.prim->param == entry.param)
            entry.prim->Delete(-1);
    }
    g_owned_prims.erase(std::remove_if(g_owned_prims.begin(), g_owned_prims.end(),
                                       [slot](const PausedPrim &entry) { return entry.slot == slot; }),
                        g_owned_prims.end());
    g_paused_prims.erase(std::remove_if(g_paused_prims.begin(), g_paused_prims.end(),
                                        [slot](const PausedPrim &entry) { return entry.slot == slot; }),
                         g_paused_prims.end());
    if (state->seq_id != -1) IxionDc1SfxStop(state->seq_id, slot * 2); // its loop sequence ends with it
    *state = State{};
    if (LiveInstances() == 0) {
        // The last donor leaves: the host table and the shared pause bookkeeping return to native.
        RestorePlayerTable();
        g_kind2_observe_end = -1;
        g_paused_prims.clear();
        g_paused = false;
        // Kill or slot reuse: the DC1 banks linger so a death sound finishes (room retire is immediate).
        IxionDc1SfxReleaseLater(mgPortFrameCount(), "last donor left");
    }
}

void IxionDc1Removed(CActiveMonster *monster) {
    State *state = Slot(monster);
    if (state == nullptr) return;
    if (state->dead_pending) {
        Capture(7, "dead-reward");
        std::fprintf(stderr, "ixion dc1 kill counted frame=%d donor=%s slot=%d floor kill_count %d->%d (DC2 removal step)\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), state->kills_before,
                     NowFloorInfoPtr != nullptr ? NowFloorInfoPtr->kill_count : -1);
    }
    IxionDc1Forget(monster);
}

int IxionDc1RosterSize() { return (int) Roster().size(); }

const char *IxionDc1RosterId(int kind) {
    const auto &roster = Roster();
    return kind >= 0 && kind < (int) roster.size() ? roster[kind]->id.c_str() : nullptr;
}

CActiveMonster *IxionDc1SpawnDonor(const float position[4]) { return IxionDc1SpawnDonorKind(0, position); }

CActiveMonster *IxionDc1SpawnDonorKind(int kind, const float position[4]) {
    const int id = kDonorId + kind;
    const Donor *selected = DonorForId(id);
    if (selected == nullptr || ActiveMonster == nullptr || DngMainScene == nullptr || Row(id) == nullptr) {
        return nullptr;
    }
    const Donor &donor = *selected;
    std::fprintf(stderr, "ixion dc1 balance=%s dc1_player_hp=%.0f donor=%s kind=%d tier=%s\n",
                 BalanceHp() == 70.0f ? "faithful" : "friendly", BalanceHp(), donor.id.c_str(), kind,
                 donor.id == "e07a" ? "boss" : "ordinary");
    // The donors' models, scripts and sound banks live in their own stack, not the floor's monster
    // stack: a re-entered floor fills stack 3 with its native monsters, and the retail _IMG copy
    // then writes through a failed (NULL) allocation. A floor's refer table is rebuilt on every
    // floor load, so when no roster kind's refer is present nothing still points into this stack
    // and it is reset; otherwise further kinds accumulate after the ones already loaded.
    static mgCMemory donor_memory;
    static bool      donor_memory_ready = false;
    if (ActiveMonster->SearchActiveMonsterBlock() < 0) {
        // Checked before loading, so a refused kind costs no refer or donor-stack space.
        std::fprintf(stderr, "ixion dc1 spawn failed: donor=%s no free monster slot (of %d)\n", donor.id.c_str(),
                     MONSTER_ACTIVE_MAX);
        return nullptr;
    }
    if (ActiveMonster->SearchBaseIndex(id) < 0) {
        bool any_loaded = false;
        for (int other = 0; other < IxionDc1RosterSize(); ++other)
            any_loaded = any_loaded || ActiveMonster->SearchBaseIndex(kDonorId + other) >= 0;
        if (!donor_memory_ready) {
            donor_memory.stSetBuffer(g_donor_buffer, kDonorQuadwords);
            donor_memory_ready = true;
        } else if (!any_loaded) {
            donor_memory.stReset();
        }
        if (!ActiveMonster->EntryRefer(id, &donor_memory)) {
            std::fprintf(stderr, "ixion dc1 spawn failed: EntryRefer(ix_%s) (is DC2_IXION_EXTRA_DATA set? refer table full?)\n",
                         donor.id.c_str());
            return nullptr;
        }
        std::fprintf(stderr, "ixion dc1 refer loaded into donor stack: donor=%s used=%d of %d quadwords\n",
                     donor.id.c_str(), donor_memory.stGetUsed(), donor_memory.stGetSize());
    }
    const int refer_no = ActiveMonster->SearchBaseIndex(id);
    MONSTER_REFER &refer = ActiveMonster->refer[refer_no];
    if (refer.chara.GetFrame() == nullptr || !PatchProgram(refer, &donor)) {
        std::fprintf(stderr, "ixion dc1 spawn failed: donor=%s refer=%d frame=%p program not patched\n", donor.id.c_str(),
                     refer_no, static_cast<void *>(refer.chara.GetFrame()));
        return nullptr;
    }
    float place[4] = {position[0], position[1], position[2], 1.0f};
    float rotation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    CActiveMonster *monster = ActiveMonster->SetActiveMonster(refer_no, place, rotation, -1);
    if (monster == nullptr) {
        std::fprintf(stderr, "ixion dc1 spawn failed: donor=%s no free monster slot\n", donor.id.c_str());
        return nullptr;
    }
    IxionDc1InstallNatives(monster);
    State *state = Find(monster);
    if (state == nullptr) return monster; // install refused and logged; the slot runs no native
    // DC1 keeps the player and walls body_radius = 15 from Opar (Chronicle monstorunit.cpp:263,
    // :2487). The converted cfg's BODY_SIZE (DC1 56 x k = 87) is a box size, and DC2 reads
    // body_width as a half-reach (CollisionCheck sums 2 * width of both bodies), so 87 kept the
    // player out of a 185-unit circle and froze the walk. Half of DC1's radius in DC2 units:
    const float native_width = monster->body_width;
    monster->body_width = 0.5f * donor.radius * kScale;
    // DC1 knockback_scale 0.0 (monstorunit.cpp:262): hits never push Opar. NO_BLOW only skips the
    // blow vector in CheckDamage; req_prog still goes to DAMAGE, so prog_110 (counter-spit) runs.
    if (donor.no_blow) monster->attrib |= MONSTER_ATTRIB_NO_BLOW;
    // The converted shadow is DC1's five open boxes, which DC2's stencil projection draws as a
    // jagged polygon (MC-R1 risk cube38). Until the converter emits a closed prism, donors cast
    // none: DrawShadowDirect and ShadowStep skip a character whose shadow_frame is NULL.
    const bool had_shadow = monster->shadow_frame != nullptr;
    monster->shadow_frame = nullptr;
    // The brainless converter's HIT (2) and BODY (4) groups are placeholders (MC-R1, radius 5 on
    // every bcol/mcol frame). DC1's set-up program declares the real ones through 130 and 136.
    int placeholders = 0;
    for (CHARA_ENTRY_OBJECT &entry : monster->entry_object) {
        if (entry.frame != nullptr && (entry.group == CHARA_ENTRY_GROUP_HIT || entry.group == CHARA_ENTRY_GROUP_BODY)) {
            entry.frame = nullptr;
            entry.size = 0.0f;
            entry.group = -1;
            entry.enable = false;
            ++placeholders;
        }
    }
    PrepareParams();
    // The DC1 table replaces DC2's for this interpreter only; no DC2 native is reachable from it.
    CActiveMonster *saved = nowMonster;
    nowMonster = monster;
    const int result = monster->mons_script.run(kSetupProgram);
    nowMonster = saved;
    monster->req_prog = MONSTER_PROG_MAIN;
    if (state->mov_col_count == 0) {
        // Without _SET_MOV_COL DC1 pushes by body_radius about the position (monstorunit.cpp:1657-1676).
        for (CHARA_ENTRY_OBJECT &entry : monster->entry_object) {
            if (entry.frame != nullptr) continue;
            entry.frame = monster->GetFrame();
            entry.size = 0.5f * donor.radius * kScale;
            entry.group = CHARA_ENTRY_GROUP_BODY;
            entry.enable = entry.frame != nullptr;
            break;
        }
    }
    // DC1 SetupViewMonstor (monstorunit.cpp:2506-2518): a mimic family monster starts unrevealed and
    // DC1 posts a chest event in its place; the set-up program has already posed it as the chest.
    const auto reveal = g_has_reveal.find(donor.id);
    if ((donor.dc1_kind == 3 || donor.dc1_kind == 4) && reveal != g_has_reveal.end() && reveal->second) {
        state->hidden = true;
        state->hidden_attrib = (MONSTER_ATTRIB_NO_LOCK_ON | MONSTER_ATTRIB_NO_DAMAGE) & ~monster->attrib;
        monster->attrib |= MONSTER_ATTRIB_NO_LOCK_ON | MONSTER_ATTRIB_NO_DAMAGE;
        std::fprintf(stderr, "ixion dc1 mimic hidden donor=%s slot=%d dc1_kind=%d motion=%d (DC1 revealed=0; opens to prog_50)\n",
                     donor.id.c_str(), SlotOf(state), donor.dc1_kind, state->motion_no);
    }
    // DRAN-R2 debug knob for the kill replay: DC2_IXION_DC1_DEBUG_LIFE=<n> starts every donor at n life
    // (stated in the receipt; the DC1 row value stays in the descriptor).
    if (const char *text = std::getenv("DC2_IXION_DC1_DEBUG_LIFE"); text != nullptr && std::atoi(text) > 0) {
        const int life = std::atoi(text);
        std::fprintf(stderr, "ixion dc1 DEBUG life override donor=%s life %d -> %d (DC2_IXION_DC1_DEBUG_LIFE)\n",
                     donor.id.c_str(), monster->life, life);
        monster->life = monster->max_life = life;
    }
    std::fprintf(stderr,
                 "ixion dc1 spawned %s kind=%d slot=%d refer=%d id=%d life=%d keys=%d pos=(%.1f,%.1f,%.1f) setup_run=%d "
                 "setup_end=%d dmg_cols=%d body_cols=%d mov_cols=%d placeholders_cleared=%d body_width=%.2f (cfg %.2f) "
                 "shadow_dropped=%d\n",
                 donor.id.c_str(), kind, monster->chara_type - MONSTER_ACTIVE_MAX, refer_no, monster->monster_id,
                 monster->life, monster->key_num[0] - 1, place[0], place[1], place[2], result, monster->mons_script.end,
                 state->damage_count, state->body_col_count, state->mov_col_count, placeholders, monster->body_width,
                 native_width, had_shadow ? 1 : 0);
    return monster;
}

bool IxionDc1RosterKindUnique(int kind) {
    const auto &roster = Roster();
    if (kind < 0 || kind >= (int) roster.size()) return false;
    const int dc1_kind = roster[kind]->dc1_kind;
    return dc1_kind != 0 && dc1_kind != 3; // monstorunit.cpp:610: NORMAL and MIMIC_SMALL may repeat
}

bool IxionDc1MimicOpens(float distance_dc1, float body_radius_dc1, bool confirm_pressed) {
    // DC1 posts the chest event 8-10 units in front of the box with radius 8 and opens it on the
    // confirm press (dungeonmap.cpp:2616-2628, dun/gameloop.cpp:3575, :3652-3659). DC2 has no box event
    // here, so the press opens the chest from any side within the body radius plus that 8.
    return confirm_pressed && distance_dc1 <= body_radius_dc1 + 8.0f;
}

bool IxionDc1HiddenStep(CActiveMonster *monster) {
    State *state = Find(monster);
    if (state == nullptr || !state->hidden) return false;
    // The closed chest stays drawn (DC1 draws the box in the mimic's place); ThinkHost's view check
    // is skipped while hidden, so the spawn's INIT view and zero alpha would otherwise hide it.
    monster->view_state = MONSTER_VIEW_IN;
    monster->view_alpha = 1.0f;
    CCharacter2 *player = MainChara__2;
    if (player == nullptr || DngStatus.mode != DNG_STATUS_FIELD) return true;
    float self[4], position[4];
    monster->GetPosition(self);
    player->GetPosition(position);
    const float distance = Distance(self, position) / kScale;
    const float radius = state->donor != nullptr ? state->donor->radius : 5.0f;
    if (distance <= 2.0f * (radius + 8.0f)) Capture(12, "mimic-closed");
    if (!IxionDc1MimicOpens(distance, radius, GamePad__2.Down(PAD_CROSS))) return true;
    // DNG_EVENT_MIMIC (dun/gameloop.cpp:3652-3659) sets revealed = 1; the next script start runs
    // prog_50 once and then revealed = -1 (monstorunit.cpp:1997-1999). DC2's RunScript runs the
    // requested program and returns to MAIN (200 -> DC1 prog_100) when it ends.
    state->hidden = false;
    monster->attrib &= ~state->hidden_attrib;
    state->hidden_attrib = 0;
    monster->req_prog = kRevealProgram;
    std::fprintf(stderr, "ixion dc1 mimic opened frame=%d donor=%s slot=%d dist_dc1=%.1f -> DC1 prog_50 (DC2 %d)\n",
                 mgPortFrameCount(), DonorName(state), SlotOf(state), distance, kRevealProgram);
    Capture(6, "mimic-open");
    return false;
}

bool IxionDc1RefusesReaction(const CActiveMonster *monster) {
    // Precheck P3: a dead donor (life gone or DC1 death program running) takes no new reaction.
    return Find(monster) != nullptr && (monster->life <= 0 || monster->now_prog == MONSTER_PROG_DEAD);
}

void IxionDc1AttackPrim(CActiveMonster *monster, ACTION_DAMAGE *entry) {
    State *state = Find(monster);
    if (state == nullptr || entry == nullptr || entry->prim == nullptr) return;
    TrackPrim(entry->prim, SlotOf(state));
    const int slot = (int) (entry - monster->damage);
    for (int i = 0; i < state->damage_count; ++i) {
        const DamageSphere &sphere = state->damage[i];
        if (sphere.slot != slot || sphere.damage <= 0) continue;
        // ThinkHost (as monster.cpp:2828-2830) wrote the row attack; the sphere's DC1 damage wins.
        PrepareParams();
        if (g_params_ready) entry->prim->param = sphere.kind == 2 ? &g_normal_param : &g_blow_param;
        TrackPrim(entry->prim, SlotOf(state));
        entry->prim->damage = ScaledDamage(sphere.damage);
        std::fprintf(stderr, "ixion dc1 attack prim frame=%d damage[%d] dc1=%d dc2=%d (row attack %d) prog=%d kind=%d\n",
                     mgPortFrameCount(), slot, sphere.damage, entry->prim->damage, monster->attack, monster->now_prog, sphere.kind);
        return;
    }
}

void IxionDc1StepShots() {
    IxionDc1SfxStep(mgPortFrameCount()); // a deferred DC1 bank release, also with no donor left
    if (IxionDc1UpdatePause()) return;
    if (CCharacter2 *player = MainChara__2) {
        // Movement measurement for the goo claim: the player's horizontal speed over 10 frames.
        static float last[4] = {};
        static int   last_frame = -100;
        const int    frame = mgPortFrameCount();
        if (frame - last_frame >= 10) {
            float position[4];
            player->GetPosition(position);
            const float dx = position[0] - last[0], dz = position[2] - last[2];
            const float speed = std::sqrt(dx * dx + dz * dz) / (float) (frame - last_frame);
            CBattleCharaInfo *battle = GetBattleCharaInfo();
            if (frame <= g_kind2_observe_end) {
                const auto *action = static_cast<CActionChara *>(player);
                std::fprintf(stderr, "ixion dc1 kind2 reaction frame=%d blow_speed=%.2f blow_time=%d blow_rate=%.1f position=(%.2f,%.2f,%.2f)\n",
                             frame, action->blow_speed, action->blow_time, action->blow_rate,
                             position[0], position[1], position[2]);
            }
            // Precheck P3: this trace printed every 10 frames while any donor lived; it is now opt-in.
            static const bool speed_trace = [] {
                const char *text = std::getenv("DC2_IXION_DC1_SPEED_TRACE");
                return text != nullptr && std::string_view(text) == "1";
            }();
            if (speed_trace && frame - last_frame == 10 && speed > 0.01f && speed < 20.0f && LiveInstances() > 0) {
                std::fprintf(stderr, "ixion dc1 player speed frame=%d speed=%.3f/frame slow=%d\n", frame, speed,
                             battle != nullptr && (battle->GetAttr() & CHARA_STATUS_SLOW) ? 1 : 0);
            }
            std::memcpy(last, position, sizeof last);
            last_frame = frame;
        }
    }
    for (Shot &shot : g_shots) {
        if (!shot.live) continue;
        if (shot.prim == nullptr || !shot.prim->active || shot.prim->owner != shot.owner) {
            if (shot.player_owned) ReleaseGooVisual(shot, false);
            shot = Shot{};
            continue;
        }
        if (shot.prim->hit_num > 0) {
            if (shot.player_owned) {
                // The monster side already took the damage in its CheckDamage; no goo is rolled on a player shot.
                auto *hit = SceneMonster(shot.hit_chara);
                std::fprintf(stderr, "ixion dc1 player shot hit frame=%d chara=%d life_at_hit=%d life_now=%d damage=%d\n",
                             mgPortFrameCount(), shot.hit_chara, shot.hit_life, hit != nullptr ? hit->life : -1,
                             shot.damage);
                EndShot(shot, "hit-monster");
                continue;
            }
            if (shot.effect == 29) ApplyGoo();
            EndShot(shot, "hit-player");
            continue;
        }
        if (--shot.life <= 0) {
            EndShot(shot, shot.impact ? "impact-end" : "life");
            continue;
        }
        if (shot.impact) {
            // DC1 phase 2 holds still and keeps its impact sphere (speed[2] = 0, radius[2]).
            if (shot.fire && shot.life % 6 == 0) FireStandIn(shot.pos, 20.0f);
            continue;
        }
        float next[4] = {shot.pos[0] + shot.vel[0], shot.pos[1] + shot.vel[1], shot.pos[2] + shot.vel[2], 1.0f};
        int hit_parts = 0;
        if (RayBlockedNear(shot.pos, shot.pos, next, &hit_parts)) {
            IxionRoomArenaProjectileHit(hit_parts, "dc1-shot"); // DC1 shot_utils.cpp:75-89
            if (shot.impact_radius > 0.0f && shot.impact_frames > 0) {
                // DC1 shot_effect.cpp:248-268: the flying phase becomes the impact phase in place.
                shot.impact = true;
                shot.life = shot.impact_frames;
                shot.prim->SetCoord(shot.pos, 0.5f * shot.impact_radius * kScale);
                if (shot.fire) FireStandIn(shot.pos, 45.0f);
                std::fprintf(stderr, "ixion dc1 shot impact frame=%d effect=%d pos=(%.1f,%.1f,%.1f) parts=0x%x radius_dc1=%.1f frames=%d (DC1 phase 2)\n",
                             mgPortFrameCount(), shot.effect, shot.pos[0], shot.pos[1], shot.pos[2], hit_parts,
                             shot.impact_radius, shot.impact_frames);
                continue;
            }
            EndShot(shot, "wall");
            continue;
        }
        float old[4];
        std::memcpy(old, shot.pos, sizeof old);
        std::memcpy(shot.pos, next, sizeof next);
        shot.prim->SetCoord(shot.pos, 0.5f * shot.radius * kScale);
        // Visual: Monica's ball wears DC2's goo model; DC2's guard spark every 4 frames is only the fallback
        // for monster-owned shots and for a ball whose model could not be made.
        const bool modelled = shot.player_owned && PinGooVisual(shot);
        if (shot.fire) {
            // DC2 stand-in for the f_boll_2 model: the fire-element after-hit flames every 3 frames.
            if (shot.life % 3 == 0) FireStandIn(shot.pos, 0.0f);
            if (shot.life == 160) Capture(17, "volley-shot-flight"); // 20 frames into the first fireball's flight
        } else if (!modelled && shot.life % 4 == 0 && ActiveMonster != nullptr && ActiveMonster->scene != nullptr) {
            // Placeholder visual: DC2's guard spark every 4 frames along the flight (no goo model yet).
            GuardEffectSet(ActiveMonster->scene, shot.pos, 0);
        }
        if (shot.life % 15 == 0) {
            std::fprintf(stderr, "ixion dc1 shot frame=%d pos=(%.1f,%.1f,%.1f) life=%d\n", mgPortFrameCount(),
                         shot.pos[0], shot.pos[1], shot.pos[2], shot.life);
        }
        (void) old;
    }
}

int IxionDc1PlayerShotsLive() {
    int live = 0;
    for (const Shot &shot : g_shots) {
        if (shot.live && shot.player_owned) ++live;
    }
    return live;
}

bool IxionDc1SpawnPlayerShot(const float from[4], const float target[4], int dc1_damage, float caster_max_hp) {
    return SpawnShot(nullptr, *FindShotEffect(29), nullptr, from, target, dc1_damage, false, true, caster_max_hp);
}

void IxionDc1RetirePlayerShots(const char *why) {
    for (Shot &shot : g_shots) {
        if (shot.live && shot.player_owned) EndShot(shot, why != nullptr ? why : "retire");
    }
}

// Monica's belly-flop window (DC1 _SET_DMG_COL + _SET_DMG_PARA kind 3): one point prim of the 吹っ飛び攻撃 row,
// owner 0 (the player, so the BY_OWNER target is monsters only), no status, DC1's number scaled from the caster's
// maximum HP. The radius is DC2's (the caller sizes it; DC2's test is 2 x (radius + hit-entry size)).
static const DAMAGE_PARAM *g_player_blow_row = nullptr;

CColPrim *IxionDc1OpenPlayerBlow(const float position[4], float dc2_radius, int dc1_damage, float caster_max_hp) {
    CColPrim *prim = ColPrimMan.GetPrim();
    if (prim == nullptr) return nullptr;
    if (!prim->SetDamage(kBlowHit, 0)) {
        prim->Delete(-1);
        return nullptr;
    }
    g_player_blow_row = prim->param;
    prim->status = 0;
    prim->damage = IxionDc1ScaledDamage(dc1_damage, caster_max_hp, BalanceHp());
    prim->SetCoord(const_cast<float *>(position), dc2_radius);
    return prim;
}

void IxionDc1MovePlayerBlow(CColPrim *prim, const float position[4], float dc2_radius) {
    if (IxionDc1PlayerBlowLive(prim)) prim->SetCoord(const_cast<float *>(position), dc2_radius);
}

bool IxionDc1PlayerBlowLive(const CColPrim *prim) {
    return prim != nullptr && prim->active && prim->owner == 0 && g_player_blow_row != nullptr &&
           prim->param == g_player_blow_row;
}

void IxionDc1ClosePlayerBlow(CColPrim *prim) {
    if (IxionDc1PlayerBlowLive(prim)) prim->Delete(-1);
}

void IxionDc1Despawn(CActiveMonster *monster, const char *reason) {
    State *slot_state = Slot(monster);
    if (slot_state == nullptr) return;
    const int slot = SlotOf(slot_state);
    for (Shot &shot : g_shots) {
        if (shot.live && !shot.player_owned && shot.slot == slot) EndShot(shot, "retire");
    }
    if (Find(monster) == nullptr) { IxionDc1Forget(monster); return; }
    if (monster->state == ACTIVE_MONSTER_NONE) {
        std::fprintf(stderr, "ixion dc1 despawn frame=%d donor=%s slot=%d reason=%s: already removed (killed=%d)\n",
                     mgPortFrameCount(), DonorName(slot_state), slot, reason != nullptr ? reason : "-",
                     slot_state->dead_done ? 1 : 0);
        IxionDc1Forget(monster);
        return;
    }
    for (ACTION_DAMAGE &damage : monster->damage) {
        if (damage.use != 0 && damage.prim != nullptr) {
            damage.prim->Delete(-1);
            damage.prim = nullptr;
        }
    }
    const int life = monster->life;
    monster->state = ACTIVE_MONSTER_NONE;
    monster->chara_kind = ACTION_KIND_NONE;
    monster->alpha = 0.0f;
    if (MainChara__2 != nullptr && MainChara__2->lock_on &&
        MainChara__2->target_no == monster->chara_type) {
        MainChara__2->lock_on = 0;
        MainChara__2->target_no = -1;
    }
    std::fprintf(stderr, "ixion dc1 despawn frame=%d donor=%s reason=%s slot=%d hp=%d (not a kill)\n", mgPortFrameCount(),
                 DonorName(slot_state), reason != nullptr ? reason : "-", monster->chara_type - MONSTER_ACTIVE_MAX, life);
    IxionDc1Forget(monster);
    if (LiveInstances() == 0) IxionDc1SfxReleaseNow(reason != nullptr ? reason : "despawn");
}

float IxionDc1MotionStep(CActiveMonster *monster, float native_step) {
    State *state = Find(monster);
    if (state == nullptr) return native_step;
    // SetMotion takes effect in this Step; from the next script call the key is current.
    state->motion_pending = false;
    return state->motion_speed > 0.0f ? state->motion_speed : native_step;
}

// DC1 CMonstorUnit::SoundCheck (Chronicle monstorunit.cpp:704-736) on DC2's mixer: the loop sequence
// and the native 140 windows against the motion time, at DC1's distance level and pan (near 50 /
// far 500 DC1 units, 350 / 1000 for a no-lock-on kind; DC2's sndGetVolPan is the same formula
// around the DC2 microphone). The loop sequence step is SndSeSeqStep (snd.cpp:1263-1282).
static void SoundCheck(State &state, CActiveMonster *monster) {
    if (!IxionDc1SfxResident()) return;
    const float frame = monster->frame;
    float       position[4];
    monster->GetPosition(position);
    const bool no_lock_on = state.donor != nullptr && state.donor->dc1_kind == 2;
    float      level = 1.0f, pan = 0.0f;
    sndGetVolPan(&level, &pan, position, (no_lock_on ? 350.0f : 50.0f) * kScale, (no_lock_on ? 1000.0f : 500.0f) * kScale);
    const int voice = SlotOf(&state) * 2; // DC1 current_monster * 2
    if (state.loop_id != -1 && state.loop_start <= frame && !(state.loop_end <= frame)) {
        if (state.seq_id == state.loop_id) {
            state.seq_step = 1; // refreshed: SndSeSeqPlayStop on a found entry
        } else {
            if (state.seq_id != -1) IxionDc1SfxStop(state.seq_id, voice);
            state.seq_id = state.loop_id;
            state.seq_step = 0;
        }
        state.seq_length = state.loop_step;
    }
    if (state.seq_id != -1) {
        if (state.seq_step == 0) IxionDc1SfxPlay(state.seq_id, level, pan, voice, "loop", DonorName(&state), SlotOf(&state));
        if (state.seq_step >= state.seq_length) {
            IxionDc1SfxStop(state.seq_id, voice);
            std::fprintf(stderr, "ixion dc1 sound loop stopped frame=%d donor=%s slot=%d dc1_se=%d after %d frames unrefreshed\n",
                         mgPortFrameCount(), DonorName(&state), SlotOf(&state), state.seq_id, state.seq_length);
            state.seq_id = -1;
        } else {
            ++state.seq_step;
        }
    }
    for (int i = 0; i < 16; ++i) {
        if (state.snd_cooldown[i] > 0) {
            --state.snd_cooldown[i];
        } else if (state.snd_id[i] != -1 && state.snd_start[i] <= frame && !(state.snd_end[i] <= frame)) {
            IxionDc1SfxPlay(state.snd_id[i], level, pan, 0, "window", DonorName(&state), SlotOf(&state));
            state.snd_cooldown[i] = 10;
        }
    }
}

void IxionDc1AfterScript(CActiveMonster *monster) {
    State *state = Find(monster);
    if (state == nullptr) return;
    SoundCheck(*state, monster);
    if (monster->now_prog != state->last_prog) {
        const int dc2 = monster->now_prog;
        const int dc1 = dc2 == MONSTER_PROG_MAIN ? kDc1Main
                        : dc2 == MONSTER_PROG_DEAD ? kDc1Death
                        : dc2 == kRevealProgram ? kDc1Reveal
                        : (dc2 == MONSTER_PROG_STATUS || dc2 == MONSTER_PROG_DAMAGE || dc2 == MONSTER_PROG_KNOCK ||
                           dc2 == MONSTER_PROG_GUARD || dc2 == MONSTER_PROG_PIYORI)
                            ? kDc1Hit
                            : -1;
        std::fprintf(stderr, "ixion dc1 program frame=%d donor=%s slot=%d DC2 prog=%d -> DC1 prog_%d\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), dc2, dc1);
        state->last_prog = monster->now_prog;
    }
    if (state->last_life >= 0 && monster->life < state->last_life) {
        state->hits_taken++;
        std::fprintf(stderr,
                     "ixion dc1 hit taken frame=%d donor=%s slot=%d hp=%d->%d/%d hits=%d prog=%d motion=%d muteki=%d "
                     "blow_speed=%.2f blow_time=%d attrib=0x%x\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), state->last_life, monster->life,
                     monster->max_life, state->hits_taken, monster->now_prog, state->motion_no, monster->muteki_time,
                     monster->blow_speed, monster->blow_time, monster->attrib);
        // DC1 plays SE_MONSTER_HIT when a hit takes HP (monstorunit.cpp:1602, SndSePlay(160, -1, 0)).
        // DC2 applied the damage in CheckDamage after the last think, so it sounds one tick later.
        if (IxionDc1SfxRequested()) IxionDc1SfxPlay(160, 1.0f, 0.0f, 0, "hurt", DonorName(state), SlotOf(state));
    }
    state->last_life = monster->life;
    // DC1 hurt-sphere windows (monstorunit.cpp:1200): with start != 0 the sphere counts only while
    // start < motion time <= end. The DC2 HIT entry is enabled for exactly that span.
    for (int i = 0; i < state->body_col_count; ++i) {
        const BodyCol &col = state->body_cols[i];
        if (col.entry < 0 || col.start == 0.0f) continue;
        monster->entry_object[col.entry].enable = col.start < monster->frame && monster->frame <= col.end;
    }
    float position[4];
    monster->GetPosition(position);
    // Movement (Chronicle monstorunit.cpp:2036-2069): the DC1 walk stops when a wall lies within
    // 10 units ahead; otherwise it advances along the vector. DC2's MoveUnit then slides, pushes
    // and grounds the body; its velocity carries only gravity, so the walk speed stays DC1's.
    // 108: while the DC1 collision-off timer runs the wall stop is skipped too (monstorunit.cpp:2043);
    // it drops by one per DC1 step (:2133-2135), here per think, and the DC2 bits go when it ends.
    const bool col_off = state->col_off > 0;
    if (col_off && --state->col_off == 0) {
        monster->attrib &= ~state->col_off_attrib;
        std::fprintf(stderr, "ixion dc1 col off ended frame=%d donor=%s slot=%d attrib=0x%x pos=(%.1f,%.1f,%.1f)\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), monster->attrib, position[0], position[1],
                     position[2]);
        state->col_off_attrib = 0;
    }
    if (state->move_speed != 0.0f && !col_off) {
        float from[4] = {position[0], position[1] + 5.0f * kScale, position[2], 1.0f};
        float to[4] = {from[0] + 10.0f * kScale * state->move_dir[0], from[1] + 10.0f * kScale * state->move_dir[1],
                       from[2] + 10.0f * kScale * state->move_dir[2], 1.0f};
        if (RayBlocked(monster, from, to)) state->move_speed = 0.0f;
    }
    monster->velocity[0] = 0.0f;
    monster->velocity[2] = 0.0f;
    if (state->move_speed != 0.0f) {
        monster->next_pos[0] = position[0] + 1000.0f * state->move_dir[0];
        monster->next_pos[1] = position[1] + (state->vertical_control ? 1000.0f * state->move_dir[1] : 0.0f);
        monster->next_pos[2] = position[2] + 1000.0f * state->move_dir[2];
        monster->next_pos[3] = 1.0f;
        monster->move_speed = state->move_speed * kScale;
        monster->arrive_dist = 0.0f;
    } else {
        monster->move_speed = 0.0f;
    }
    if (state->vertical_control) {
        state->before_move_y = position[1];
        // DC1 monstorunit.cpp:2050-2069: fall by 0.2 per tick when airborne, feed that displacement
        // back into the movement vector/speed. DC2's accumulated gravity is replaced each tick.
        const float fall = state->falls && monster->height > 0.0001f ? 0.2f : 0.0f;
        monster->velocity[1] = -fall * kScale;
        if (fall != 0.0f) {
            float delta[4] = {state->move_dir[0] * state->move_speed,
                              state->move_dir[1] * state->move_speed - fall,
                              state->move_dir[2] * state->move_speed, 0.0f};
            state->move_speed = std::sqrt(delta[0]*delta[0] + delta[1]*delta[1] + delta[2]*delta[2]);
            Normalize3(delta);
            std::memcpy(state->move_dir, delta, sizeof state->move_dir);
        }
        if (!state->falls) monster->attrib |= MONSTER_ATTRIB_SKIP_GROUND;
        else monster->attrib &= ~MONSTER_ATTRIB_SKIP_GROUND;
    }
    // Turning (Chronicle monstorunit.cpp:2116-2127): a fixed step toward the target point, done
    // within 0.0524 rad. DC2's own turn is left idle.
    monster->rot_speed = 0.0f;
    if (state->turn_speed != 0.0f) {
        float rotation[4];
        monster->GetRotation(rotation);
        const float angle =
            std::atan2(state->turn_target[0] * kScale - position[0], state->turn_target[2] * kScale - position[2]);
        rotation[1] = AngleStep(rotation[1], angle, state->turn_speed);
        monster->SetRotation(rotation);
        if (AngleDelta(rotation[1], angle) <= 0.052359879f) state->turn_speed = 0.0f;
    }
    for (int i = 0; i < ACTION_DAMAGE_MAX && i < 16; ++i) {
        const bool open = monster->damage[i].use && monster->damage[i].prim != nullptr;
        if (open != state->window_open[i]) {
            state->window_open[i] = open;
            std::fprintf(stderr, "ixion dc1 attack window %s frame=%d donor=%s slot=%d damage[%d] motion=%d mframe=%.1f\n",
                         open ? "open" : "closed", mgPortFrameCount(), DonorName(state), SlotOf(state), i,
                         state->motion_no, monster->frame);
        }
    }
    if (state->alpha_step != 0.0f) {
        const float previous = state->alpha;
        state->alpha -= state->alpha_step;
        if (state->alpha < 0.0f) state->alpha = 0.0f;
        if (state->alpha > 128.0f) state->alpha = 128.0f;
        monster->alpha = state->alpha / 128.0f;
        if (state->motion_no == 18 && previous > 64.0f && state->alpha <= 64.0f) Capture(1, "fade-out");
        if (state->motion_no == 20 && previous < 64.0f && state->alpha >= 64.0f) Capture(3, "fade-in");
        if (previous != state->alpha && (state->alpha == 0.0f || state->alpha == 128.0f)) {
            std::fprintf(stderr, "ixion dc1 fade endpoint frame=%d alpha=%.1f pos=(%.2f,%.2f,%.2f) muteki=%d\n",
                         mgPortFrameCount(), state->alpha, position[0], position[1], position[2], monster->muteki_time);
            if (state->motion_no == 20 && state->alpha == 128.0f) Capture(4, "teleport-visible");
        }
    }
    if (state->frames == 0) Capture(0, "spawn");
    // DRAN-R2 boss framing evidence: 45 thinks after DC2's lock-on settles on this donor.
    if (MainChara__2 != nullptr && MainChara__2->lock_on && MainChara__2->target_no == monster->chara_type) {
        if (++state->lock_frames == 45) Capture(16, "lockon-framed");
    }
    if (state->frames++ % 30 == 0) {
        float rotation[4];
        monster->GetRotation(rotation);
        float player[4] = {};
        if (CCharacter2 *chara = Player(monster)) chara->GetPosition(player);
        player[3] = 1.0f;
        std::fprintf(stderr,
                     "ixion dc1 state frame=%d donor=%s slot=%d pos=(%.1f,%.1f,%.1f) dc1=(%.1f,%.1f,%.1f) yaw=%.2f "
                     "motion=%d key=%s mframe=%.1f move=%.2f turn=%.2f dist_dc1=%.1f prog=%d hp=%d/%d muteki=%d "
                     "player_lock=%d player_target=%d self=%d\n",
                     mgPortFrameCount(), DonorName(state), SlotOf(state), position[0], position[1], position[2], position[0] / kScale,
                     position[1] / kScale, position[2] / kScale, rotation[1], state->motion_no,
                     monster->GetNowMotionName() != nullptr ? monster->GetNowMotionName() : "-", monster->frame,
                     state->move_speed, state->turn_speed, Distance(position, player) / kScale, monster->now_prog,
                     monster->life, monster->max_life, monster->muteki_time,
                     MainChara__2 != nullptr ? (int) MainChara__2->lock_on : -1,
                     MainChara__2 != nullptr ? (int) MainChara__2->target_no : -1, monster->chara_type);
    }
}

void IxionDc1AfterMove(CActiveMonster *monster) {
    State *state = Find(monster);
    if (state == nullptr || !state->vertical_control || !state->falls) return;
    float position[4];
    monster->GetPosition(position);
    // DC1 monstorunit.cpp:2088-2111: ground clamp, bounce damping, or immediate ground snap.
    if (monster->mons_move_check.landed) {
        if (state->move_dir[1] < 0.0f) {
            state->move_dir[1] *= -1.0f;
            state->move_speed *= 0.5f * (2.0f - state->move_dir[1]);
            if (state->move_speed < 0.2f) state->move_speed = 0.0f;
        }
    } else if (!state->free_fall && monster->height > 0.0f) {
        // MoveUnit measures height before movement. Its ground probe is the imported room's.
        position[1] = state->before_move_y - monster->height;
        monster->SetPosition(position);
    }
}

// Which donor instance owns a primitive (for the player-hit log).
const char *AttackerName(const CColPrim *prim) {
    for (const State &state : g_states)
        if (state.monster != nullptr && state.owner == prim->owner) return DonorName(&state);
    return "-";
}

// Retail CheckDamage copied verbatim except the private DC1 kind-2 push hook below.
PORT_OVERRIDE int CActionChara::CheckDamage() {
    sceVu0FVECTOR     position;
    CBattleCharaInfo *battle;
    CColPrim         *hit;
    float             damage;
    u32               attributes;
    int               max_hp;
    int               now_hp;
    int               damage_points;
    int               guarded;
    int               immobilized;
    int               reaction;
    int               handled;
    int               element;
    int               strongest;
    int               index;

    if (nowScene__2 == NULL) {
        return false;
    }

    DNG_BATTLE_AREA *battle_area = &nowScene__2->battle_area;
    u32              battle_sound = nowScene__2->se_battle_id;
    u32              chara_sound = sound_info.se_bank;
    battle = GetBattleCharaInfo();
    GetPosition(position);

    if (damage_time > 0) {
        return false;
    }

    if (muteki_time > 0) {
        return false;
    }

    handled = false;
    hit = ColPrimMan.CheckHit(0);

    if (hit != NULL) {
        immobilized = false;
        attributes = battle->GetAttr();

        if ((attributes & (CHARA_STATUS_STOP | CHARA_STATUS_STONE)) != 0) {
            immobilized = true;
        }

        max_hp = battle->GetMaxHp_i();
        now_hp = battle->GetNowHp_i();
        damage = hit->damage * (1.0f + fRand(0.15f));
        damage -= battle->GetDefenceVol();

        if (damage <= 0.0f) {
            damage = 0.0f;
        }

        if (hit->param->hit_count > 1) {
            damage /= hit->param->hit_count;
        }

        damage += 1.0f + fRand(2.0f);

        if ((hit->status & WEAPON_SPECIAL_RATIO_DAMAGE) != 0) {
            damage = (int) (max_hp * (0.01f * hit->damage));

            if (now_hp - damage < 0.0f) {
                damage = now_hp - 1.0f;
            }
        }

        if ((hit->status & WEAPON_SPECIAL_HALF_HP) != 0) {
            damage = now_hp / 2;

            if (damage <= 1.0f) {
                damage = 1.0f;
            }
        }

        if ((int) damage <= 0) {
            damage = 0.0f;
        }

        guarded = guard_flag;

        if (guarded != 0) {
            damage *= 0.01f * hit->param->critical_rate;
        }

        if ((hit->param->hit_flags & DAMAGE_HIT_IGNORE_GUARD) != 0) {
            guard_flag = false;
            guarded = false;
        }

        if (damage >= 0.0f) {
            damage = GetDispVolumeForFloat(damage);
        }

        battle->AddHp_Point(-damage, 0.0f);
        // Chronicle dun/gameloop.cpp:7157-7160 writes damage on the attacking monster,
        // not the victim. Owner identity links this accepted hit back to the private donor.
        IxionDc1RecordPlayerDamage(hit, (int) damage);
        // Monica faithful round 2: Opar's DC1 hit reaction (prog 110) may spit back at a far attacker.
        IxionMonicaOnPlayerHit(hit, (int) damage);

        if ((damage_points = (int) damage) > 0) {
            if ((hit->status & WEAPON_SPECIAL_POISON) != 0 && iRand(100) < 30 &&
                (attributes & CHARA_STATUS_POISON) == 0 && CheckAmuletAvoid(ITEM_ID_ANTIDOTE_AMULET) == 0) {
                battle->SetAttr(CHARA_STATUS_POISON, 0);
                sndSePlay(battle_sound, 0x18, 0);
            }

            if ((hit->status & WEAPON_SPECIAL_SLOW) != 0 && battle_area->weather != DNG_WEATHER_RAIN && iRand(100) < 30 &&
                (attributes & CHARA_STATUS_SLOW) == 0 && CheckAmuletAvoid(ITEM_ID_ANTI_GOO_AMULET) == 0) {
                battle->SetAttrVol(CHARA_STATUS_SLOW, 3600);
                sndSePlay(battle_sound, 0x52, 0);
            }

            if ((hit->status & WEAPON_SPECIAL_STONE) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_STONE) == 0 && CheckAmuletAvoid(ITEM_ID_ANTI_PETRIFY_AMULET) == 0) {
                battle->SetAttrVol(CHARA_STATUS_STONE, 900);
                immobilized = true;
                sndSePlay(battle_sound, 0x53, 0);
            }

            if ((hit->status & WEAPON_SPECIAL_STOP) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_STOP) == 0 && CheckAmuletAvoid(ITEM_ID_NON_STOP_AMULET) == 0) {
                battle->SetAttrVol(CHARA_STATUS_STOP, 300);
                immobilized = true;
                sndSePlay(battle_sound, 0x54, 0);
            }

            if ((hit->status & WEAPON_SPECIAL_CURSE) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_CURSE) == 0 && CheckAmuletAvoid(ITEM_ID_ANTI_CURSE_AMULET) == 0) {
                battle->SetAttr(CHARA_STATUS_CURSE, 0);
                sndSePlay(battle_sound, 0x55, 0);
            }

            if ((hit->status & WEAPON_SPECIAL_DRY) != 0 && iRand(100) < 50) {
                battle->SetAttr(CHARA_STATUS_DRY, 0);
                sndSePlay(battle_sound, 0x52, 0);
            }
        }

        reaction = ACTION_REACTION_STAGGER;

        if ((hit->param->hit_flags & DAMAGE_HIT_KNOCKDOWN) != 0) {
            reaction = ACTION_REACTION_KNOCKDOWN;
        }

        if ((hit->param->hit_flags & DAMAGE_HIT_NO_STAGGER) != 0) {
            reaction = ACTION_REACTION_NONE;
        }

        if (guarded != 0) {
            reaction = ACTION_REACTION_GUARD;
        }

        if (immobilized != 0) {
            reaction = ACTION_REACTION_HOLD;
        }

        if (battle->GetNowHp_i() <= 0) {
            if (CheckEquipSetItem(ITEM_ID_RESURRECTION_POWDER) == 0) {
                reaction = ACTION_REACTION_DEAD;
            } else {
                battle->SetHpRate(0.5f);
                pallet[0].SetAnim(96, 180, 255, 1, 45, 0);
                effect_man->CreateEffSpt("\x92\xCA\x8F\xED\x89\xF1\x95\x9C", 0, 0);
                effect_man->SetScriptTargetId(0, -1, -1);
                effect_man->SetValue(0, 0, 0, -1);
                sndSePlay(SystemSND_ID, SYSTEM_SE_RECOVER, 0);
            }
        }

        sceVu0CopyVector(blow_vec, hit->hit_vec);
        blow_speed = 4.0f;
        blow_rate = 1.0f;
        blow_decel = 0.0f;
        blow_time = 5;

        // DC1 kind 2 hurts standing without launch (dun/gameloop.cpp:7351-7435).
        // Only this adapter's private normal row changes; native DC2 and Opar keep retail behaviour.
        if (hit->param == &g_normal_param) {
            if (!guarded) {
                blow_speed = 0.0f;
                blow_time = 0;
                // DC2's hurt script restarts the push with _BLOW_START (actscript.cpp:980).
                // Its speed is multiplied by blow_rate; the next CheckDamage resets the rate to 1.
                blow_rate = 0.0f;
                g_kind2_observe_end = mgPortFrameCount() + 30;
            }
            std::fprintf(stderr, "ixion dc1 player hit frame=%d kind=2 hp=%d->%d damage=%d guarded=%d blow_speed=%.1f blow_time=%d blow_rate=%.1f attacker=%s\n",
                         mgPortFrameCount(), now_hp, battle->GetNowHp_i(), damage_points, guarded, blow_speed, blow_time, blow_rate,
                         AttackerName(hit));
            Capture(5, "player-hit");
        }
        if (hit->param == &g_blow_param) {
            std::fprintf(stderr, "ixion dc1 player hit frame=%d kind=3 hp=%d->%d damage=%d guarded=%d attacker=%s\n",
                         mgPortFrameCount(), now_hp, battle->GetNowHp_i(), damage_points, guarded, AttackerName(hit));
            Capture(9, "melee-hit");
        }


        if (guarded != 0) {
            sndSePlay(battle_sound, 0x21, 0);

            if (immobilized == 0) {
                sndSePlay(chara_sound, 0x1D, 0);
            }

            GuardEffectSet(nowScene__2, hit->hit_pos);

            if (battle->chr_no == USER_CHARA_MONICA) {
                element = -1;

                if (battle->equip->data.weapon.status[WEAPON_STAT_DURABILITY] > 30) {
                    strongest = 0;

                    for (index = 0; index < 4; index++) {
                        if (strongest < hit->param->element[index]) {
                            strongest = hit->param->element[index];
                            element = index;
                        }
                    }

                    if (element >= 0) {
                        battle->SetMagicSwordPow(element, hit->damage);
                    }
                }
            }

            if (damage > 0.0f) {
                HitEffectSet(nowScene__2, hit->hit_pos);
                pallet[0].SetAnim(255, 128, 128, 1, 45, 0);
                shake.time = 6;
                DamageScore2.SetValue(0, damage_points, 0.0f);
                damage_time = hit->param->stun_time * 2;
                GamePad__2.SetVibration(0, 96, 6);
            } else {
                GamePad__2.SetVibration(0, 72, 6);
            }
        } else {
            if (reaction == ACTION_REACTION_KNOCKDOWN) {
                damage_time = hit->param->stun_time * 7;
                pallet[0].SetAnim(255, 128, 128, 1, 90, 0);
                shake.time = 8;
                GamePad__2.SetVibration(1, 180, 30);
            } else {
                damage_time = hit->param->stun_time * 3;
                pallet[0].SetAnim(255, 128, 128, 1, 30, 0);
                shake.time = 8;
                GamePad__2.SetVibration(1, 128, 20);
            }

            if (immobilized == 0) {
                sndSePlay(chara_sound, 0x24, 0);
            }

            sndSePlay(battle_sound, 0x16, 0);
            HitEffectSet(nowScene__2, hit->hit_pos);
            DamageScore2.SetValue(0, damage_points, body_height);
        }

        switch (reaction) {
            case ACTION_REACTION_GUARD:
            case ACTION_REACTION_NONE:
            case ACTION_REACTION_UNUSED:
                break;
            case ACTION_REACTION_HOLD:
                handled = true;
                damage_req = ACTION_DAMAGE_REQ_HOLD;
                break;
            case ACTION_REACTION_STAGGER:
                stagger += hit->param->stagger;
                stagger_time = 60;

                if (stagger >= 2 || (menu_flag != 0 && stand_flag != 0)) {
                    damage_req = ACTION_DAMAGE_REQ_SMALL;
                }

                handled = true;
                break;
            case ACTION_REACTION_KNOCKDOWN:
                handled = true;
                damage_req = ACTION_DAMAGE_REQ_LARGE;
                break;
            case ACTION_REACTION_DEAD:
                handled = true;
                break;
        }
    }

    if (handled != 0) {
        menu_flag = 0;
    }

    return handled;
}
