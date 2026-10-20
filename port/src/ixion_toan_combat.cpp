#include "common.h"

#include <libvu0.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "actionchara.hpp"
#include "actscript.hpp"
#include "automap.hpp"
#include "cameracontrol.hpp"
#include "character.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dng_effect.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "dng_status.hpp"
#include "effect.hpp"
#include "effscript.hpp"
#include "gamedata.hpp"
#include "gameutil.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "map.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "menucommon.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "mglib_port.hpp"
#include "monster.hpp"
#include "padcontrol.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "sceneload.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "sphida.hpp"
#include "swordeffect.hpp"
#include "userdata.hpp"

#include "ixion_room.hpp"
#include "ixion_toan_body.hpp"

#if defined(IXION_DC1_TOAN_COMBAT)
#include "ixion_dc1_toan.h"
#include "ixion_dc1_toan_motion.h"
#include "ixion_toan_bridge.h"
#endif
#include "ixion_toan_combat.hpp"

// Declared in ps2/src/actionchara.cpp:77-79 rather than in a header.
int RockOn_TargetSel(CScene *scene, int index);
int DistCheck_Action2(CScene *scene, float unused, float range, float *out_dist, int rank, int *out_rank);
int Check_LockOn(CScene *scene, float range, int index);

// Ixion opt-in (DC2_IXION_PLAYER_COMBAT=dc1 with DC2_IXION_PLAYER_BODY=toan): Cross runs Dark Cloud's own
// ToanKey_On/ToanKey_Play (Chronicle ps2/src/charakey.cpp, compiled unmodified into the private ixdc1_combat
// library) every tick instead of Max's script attack. DC2 keeps position, collision, gravity, camera,
// incoming damage and Max's c01.stb for locomotion and reactions. The donor picks the combo step, motion
// number, gauge cost, push and hit windows; this bridge applies them to the DC2 body and turns each donor hit
// window into one native CColPrim at the Dagger's dcol1 point, with Max's melee row, so DC2 monsters take the
// hit through their own damage path once per window.
// Goro R2: the same bridge runs Goro (DC2_IXION_PLAYER_BODY=goro): GoroKey_On/GoroKey_Play, chosen at reset by
// the library table. Which KEYs are rescaled and which one loops, the weapon point (dcol0 on the hammer through the
// named-point callback), the shot's effect radius and life all come from that table; Toan's values are his row.

namespace {

// Separate from the retail pool layout; these tokens survive Delete/Initialize and slot reuse.
std::uint64_t prim_incarnations[COLPRIM_MAX] = {};

int PrimIndex(const CColPrim *prim) {
    const auto address = reinterpret_cast<std::uintptr_t>(prim);
    const auto base = reinterpret_cast<std::uintptr_t>(&ColPrimMan.prim[0]);
    if (address < base || address - base >= sizeof(ColPrimMan.prim) ||
        (address - base) % sizeof(CColPrim) != 0) return -1;
    return static_cast<int>((address - base) / sizeof(CColPrim));
}

bool ToanCombatEnabled() {
    static const bool enabled = [] {
        const char *combat = std::getenv("DC2_IXION_PLAYER_COMBAT");
        const bool  wanted = combat != nullptr && std::strcmp(combat, "dc1") == 0;
#if !defined(IXION_DC1_TOAN_COMBAT)
        if (wanted) {
            std::fprintf(stderr, "ixion toan combat: DC2_IXION_PLAYER_COMBAT=dc1 ignored (built without IXION_DC1_SOURCE_DIR)\n");
        }
        return false;
#else
        return wanted;
#endif
    }();
    return enabled && IxionToanBodyEnabled();
}

#if defined(IXION_DC1_TOAN_COMBAT)

// ミンツ剣攻撃, Max's melee row (colprim.cpp:355): LINE|TRAIL, DAMAGE_TARGET_BY_OWNER, MAX_MELEE, multi_hit 0.
char kMaxMeleeRow[] = "\x83~\x83\x93\x83"
                      "c\x8C\x95\x8DU\x8C\x82";

// Each character's starting weapon as Dark Cloud hands it over (Chronicle itemdata.cpp WeaponList):
// Toan's Dagger (:1430) attack 6, speed 70, no element (5); Goro's Turtle Shell (:1491) attack 36, speed 45,
// elem {0, 30, 0, 0, 0}, so ice (1) is its best element. Flags are 0 for both.
struct Dc1Weapon {
    int         character;
    const char *name;
    int         attack;
    int         speed;
    int         element;
};
// Ungaga R1: Ungaga's Fighting Stick (:1520) attack 35, speed 65, elem {0, 0, 0, 5, 0}: wind (3) is its best
// element (SetWeaponElementStatus, menu_misc.cpp:1607-1621).
constexpr Dc1Weapon kStartWeapons[] = {
    {IXION_DC1_CHARA_TOAN, "Dagger", 6, 70, 5},
    {IXION_DC1_CHARA_GORO, "Turtle Shell", 36, 45, 1},
    {IXION_DC1_CHARA_UNGAGA, "Fighting Stick", 35, 65, 3},
    // Xiao R1: Xiao's Wooden Slingshot (:1472) attack 4, speed 40, no element (5); model c04w01.
    {IXION_DC1_CHARA_XIAO, "Wooden Slingshot", 4, 40, 5},
};
const Dc1Weapon &StartWeapon(int character) {
    for (const Dc1Weapon &w : kStartWeapons) {
        if (w.character == character) return w;
    }
    return kStartWeapons[0];
}
using ixion_toan::kDc2MotionSpeed;
// One DC2 window per donor collision slot (DC1's CCollisionData has 96; Toan's handler uses the first one or two).
constexpr int kWindowSlots = 8;
static_assert(ixion_toan::kCrossSlots[0] == PAD_BTN_CONFIRM && ixion_toan::kCrossSlots[1] == PAD_BTN_CONFIRM_2 &&
                  ixion_toan::kCrossSlots[2] == PAD_BTN_ACTION_CONFIRM && ixion_toan::kCrossSlots[3] == PAD_BTN_ACTION_HELD,
              "ixion_toan_bridge.h Cross slots must match padcontrol.hpp");

// One DC2 prim standing in for one donor hit window (per collision slot) or one charged-shot record.
struct ToanPrim {
    CColPrim     *prim = nullptr;
    std::uint64_t incarnation = 0;
    DAMAGE_PARAM *param = nullptr;
    int           action = -1;
    int           no = 0;
    int           damage = 0;
    float         min_dist = 0.0f;
    bool Live() const {
        return prim != nullptr && incarnation == IxionToanPrimIncarnation(prim) &&
               prim->active && prim->param == param && prim->owner == 0;
    }
};

struct ToanCombatState {
    CActionChara    *body = nullptr;
    bool             suspended = false;
    int              generation = 0;
    // Goro R2: the library table's row for the bound character (loop KEY, donor KEYs, named points, effect).
    IxionDc1CharacterInfo info{IXION_DC1_CHARA_TOAN, "toan"};
    IxionDc1ToanConfig config{};
    CActionChara    *dagger = nullptr;
    mgCFrame        *tip = nullptr;
    int              asserted_motion = -1;
    int              last_action = 0;
    ixion_toan::SlotWindows<kWindowSlots> windows;
    ToanPrim         window[kWindowSlots];
    int              native_hits = 0;
    int              combo_no = 0;
    // Vertical motion (R4): Dark Cloud's jump step replaces DC2's gravity while the donor jumps.
    ixion_toan::LandingGuard landing_guard;
    int              jumping = 0;
    int              jump_ticks = 0;
    float            jump_start_y = 0.0f;
    float            jump_apex = 0.0f;
    // HUD (R4): the donor's last gauge and charge for the Toan status panel.
    float            hud_gauge = 100.0f;
    float            hud_charge_time = 0.0f;
    int              hud_charge_level = 0;
    int              hud_action = 0;
    int              wear_skipped = 0;
    // Charged shot (R5): DC1's stationary c01_fuusya burst, run here as CSHOT_EFFECT::Step would.
    ixion_toan::BurstClock burst;
    float            burst_pos[4] = {};
    int              burst_damage = 0;
    int              burst_record_ticks = 0;
    int              burst_pulses = 0;
    int              burst_no = 0;
    float            burst_radius = 0.0f;
    ToanPrim         burst_prim;
    int              cross_returned = 0;
    int              cross_kept = 0;
    // Goro R2: named points the resolver answered or declined, and the last smash sample (482 check).
    int              points_resolved = 0;
    int              points_declined = 0;
    int              quakes = 0;
    // Ungaga R1: moving shots run in the library's effect runner (DC1's CSHOT_EFFECT); each slot's records become
    // DC2 prims here. scale converts DC1 units for the collider; scene is where the shots were fired.
    struct RunnerShot {
        int      serial = 0; // the library slot's serial while it flies, 0 once it ended
        int      shown = 0;  // serial of the last spawn logged
        ToanPrim prim;       // the open record prim (DC1 record life 2)
        int      record_ticks = 0;
        int      records = 0;
        int      prim_hits = 0;
        float    min_enemy = -1.0f;
    };
    bool             runner_on = false;
    float            effect_scale = 1.0f;
    RunnerShot       runner[IXION_DC1_SHOT_SLOTS];
    CScene          *runner_scene = nullptr;
    int              runner_spawns = 0;
    int              runner_collisions[4] = {};
    // Xiao R1: her straight shots are the library's CSHOT pool (12 slots, CSHOT::step verbatim); each record (life 1)
    // becomes one owner-0 DC2 prim here, as Ungaga's runner records do. They share runner_scene for the scene check.
    bool             cshot_on = false;
    RunnerShot       cshot[IXION_DC1_CSHOT_SLOTS];
    int              cshot_spawns = 0;
    int              cshot_collisions[4] = {};
    int              cshot_records_skipped = 0;
};

// Only the character's charge loop (the table's charge_loop_key: Toan 14, Goro 13) loops; every other donor KEY
// holds its last frame so the handler sees its end window.

ToanCombatState state;

#define IXLOG(fmt, ...) std::fprintf(stderr, "ixion %s combat: " fmt, state.info.name, ##__VA_ARGS__)

int EnemyHp() {
    CActiveMonster *enemy = IxionRoomRuntimeEnemy();
    return enemy != nullptr ? enemy->life : -1;
}

// Horizontal distance from a point to the room's encounter enemy, or -1 without one.
float EnemyDistance(const float *point) {
    CActiveMonster *enemy = IxionRoomRuntimeEnemy();
    if (enemy == nullptr) return -1.0f;
    float position[4] = {};
    enemy->GetPosition(position);
    return std::sqrt((position[0] - point[0]) * (position[0] - point[0]) + (position[2] - point[2]) * (position[2] - point[2]));
}

mgCFrame *FindFrame(mgCFrame *root, const char *name) {
    const size_t length = std::strlen(name);
    for (int i = 0; root != NULL && root->frame_list != NULL && i < root->frame_num; i++) {
        mgCFrame *frame = root->frame_list[i];
        if (frame == NULL || frame->name == NULL) continue;
        if (std::strncmp(frame->name, name, length) == 0 && (frame->name[length] == '\0' || frame->name[length] == '-')) {
            return frame;
        }
    }
    return NULL;
}

// Goro R2: the library's named-point callback (getFramePos). WEAPON names are frames of the bound weapon part
// (Goro's dcol0, Toan's dcol1), BODY names frames of the DC2 body (Ungaga's "weapon"); both in DC2 world space,
// the space input.position uses. A name the bound parts lack is declined, and the library falls back to the tip.
int32_t ResolvePoint(void *user, int32_t frame, const char *name, float out[4]) {
    auto         *s = static_cast<ToanCombatState *>(user);
    CActionChara *owner = frame == IXION_DC1_FRAME_WEAPON ? s->dagger : s->body;
    mgCFrame     *found = owner != nullptr && name != nullptr ? FindFrame(owner->CObjectFrame::frame, name) : NULL;
    if (found == NULL) {
        ++s->points_declined;
        return 0;
    }
    found->GetWorldPosition0(out);
    out[3] = 1.0f;
    ++s->points_resolved;
    return 1;
}

// Closes one slot's DC2 prim (if the engine has not already freed it) and logs the window.
void CloseWindow(int slot, const char *why) {
    ToanPrim  &w = state.window[slot];
    if (w.prim == nullptr) return;
    const bool live = w.Live();
    const int  hits = live ? w.prim->hit_num : -1;
    const int  ticks = state.windows.win[slot].ticks;
    if (live) w.prim->Delete(-1);
    IXLOG(
                 "frame=%d window close n=%d slot=%d action=0x%x why=%s ticks=%d prim_hits=%d "
                 "min_tip_enemy_dist=%.1f enemy_hp=%d max_wrench_whp=%d\n",
                 mgPortFrameCount(), w.no, slot, w.action, why, ticks, hits, w.min_dist,
                 EnemyHp(), GetBattleCharaInfo()->GetWhpNowVol(0));
    w = ToanPrim{};
}

void CloseAllWindows(const char *why) {
    state.windows.CloseAll(why, [](int slot, const char *reason) { CloseWindow(slot, reason); });
    for (int slot = 0; slot < kWindowSlots; slot++) state.window[slot] = ToanPrim{};
}

void EndBurst(const char *why) {
    if (!state.burst.live && state.burst_prim.prim == nullptr) return;
    const bool live = state.burst_prim.Live();
    const int  hits = live ? state.burst_prim.prim->hit_num : -1;
    if (live) state.burst_prim.prim->Delete(-1);
    IXLOG("frame=%d charged shot end n=%d why=%s age=%d pulses=%d prim_hits=%d enemy_hp=%d\n",
                 mgPortFrameCount(), state.burst_no, why, state.burst.age, state.burst_pulses, hits, EnemyHp());
    state.burst = {};
    state.burst_prim = ToanPrim{};
    state.burst_record_ticks = 0;
}

// Ungaga R1: closes one runner slot's record prim (if the engine has not already freed it) and logs its hits.
void CloseRunnerPrim(int slot, const char *why) {
    ToanCombatState::RunnerShot &r = state.runner[slot];
    if (r.prim.prim == nullptr) return;
    const bool live = r.prim.Live();
    const int  hits = live ? r.prim.prim->hit_num : -1;
    if (live) r.prim.prim->Delete(-1);
    if (hits > 0) r.prim_hits += hits;
    IXLOG("frame=%d shot record close n=%d slot=%d why=%s prim_hits=%d enemy_hp=%d\n", mgPortFrameCount(),
          r.prim.no, slot, why, hits, EnemyHp());
    r.prim = ToanPrim{};
    r.record_ticks = 0;
}

// Ungaga R1: ends every runner shot from the host side (DC1's OffEffect): the room retires, the scene changes, the
// character swaps out, a rebind or death. Records already laid keep no prim.
// Xiao R1: closes one CSHOT slot's record prim, as CloseRunnerPrim does for the runner.
void CloseCShotPrim(int slot, const char *why) {
    ToanCombatState::RunnerShot &r = state.cshot[slot];
    if (r.prim.prim == nullptr) return;
    const bool live = r.prim.Live();
    const int  hits = live ? r.prim.prim->hit_num : -1;
    if (live) r.prim.prim->Delete(-1);
    if (hits > 0) r.prim_hits += hits;
    IXLOG("frame=%d cshot record close n=%d slot=%d why=%s prim_hits=%d enemy_hp=%d\n", mgPortFrameCount(), r.prim.no,
          slot, why, hits, EnemyHp());
    r.prim = ToanPrim{};
    r.record_ticks = 0;
}

void EndRunner(const char *why) {
    bool any = false;
    for (int slot = 0; slot < IXION_DC1_SHOT_SLOTS; slot++) {
        ToanCombatState::RunnerShot &r = state.runner[slot];
        if (r.serial == 0 && r.prim.prim == nullptr) continue;
        any = true;
        CloseRunnerPrim(slot, why);
        if (r.serial != 0) {
            IXLOG("frame=%d shot end n=%d slot=%d why=%s records=%d prim_hits=%d enemy_hp=%d\n", mgPortFrameCount(),
                  r.serial, slot, why, r.records, r.prim_hits, EnemyHp());
        }
        r = ToanCombatState::RunnerShot{};
    }
    // Xiao R1: her CSHOT slots end on the same host-side paths (room retire, scene change, swap, death, rebind).
    for (int slot = 0; slot < IXION_DC1_CSHOT_SLOTS; slot++) {
        ToanCombatState::RunnerShot &r = state.cshot[slot];
        if (r.serial == 0 && r.prim.prim == nullptr) continue;
        any = true;
        CloseCShotPrim(slot, why);
        if (r.serial != 0) {
            IXLOG("frame=%d cshot end n=%d slot=%d why=%s records=%d prim_hits=%d enemy_hp=%d\n", mgPortFrameCount(),
                  r.serial, slot, why, r.records, r.prim_hits, EnemyHp());
        }
        r = ToanCombatState::RunnerShot{};
    }
    if (state.runner_on || state.cshot_on) ixion_dc1_off_effect();
    if (any) IXLOG("frame=%d all shots retired (%s)\n", mgPortFrameCount(), why);
}

// Ungaga R1: DC1's checkCollision (shot_utils.cpp:24-91) for the library's runner, in DC2 world space. Target 2 tests
// monsters only, so Max (the player) is never tested. A monster is DC2's own hit spheres (CHARA_ENTRY_GROUP_HIT) at
// the reach a DC2 prim of this radius has (colprim.cpp:1069-1072: 2 * (r * scale / 2 + size)); a wall is DC2's map
// collision along the step, as DC2's own bullets test it (dng_object.cpp:705-717, CheckHit mask 4).
int32_t ShotCollide(void *user, const float position[4], const float movement[4], int32_t target, float radius,
                    float hit_out[4]) {
    auto *s = static_cast<ToanCombatState *>(user);
    if (nowScene__2 == nullptr) return IXION_DC1_SHOT_HIT_NONE;
    float from[4] = {position[0], position[1], position[2], 1.0f};
    if (target != 1 && ActiveMonster != nullptr && ActiveMonster->scene == nowScene__2) {
        const float reach = 2.0f * ixion_toan::Dc2PrimRadius(radius, s->effect_scale);
        for (CActiveMonster *monster : ActiveMonster->active) {
            if (monster == nullptr || monster->state != ACTIVE_MONSTER_LIVE || monster->life <= 0) continue;
            float               entry_position[4];
            int                 entry_no = 0;
            CHARA_ENTRY_OBJECT *entry;
            while ((entry = monster->GetEntryObjectPos(CHARA_ENTRY_GROUP_HIT, entry_no, entry_position)) != NULL) {
                ++entry_no;
                if (!entry->enable) continue;
                if (mgDistVector(from, entry_position) <= reach + 2.0f * entry->size) {
                    sceVu0CopyVector(hit_out, entry_position);
                    hit_out[3] = 1.0f;
                    return IXION_DC1_SHOT_HIT_MONSTER;
                }
            }
        }
    }
    CMap *map = nowScene__2->GetMap(nowScene__2->active_map);
    if (map == NULL) return IXION_DC1_SHOT_HIT_NONE;
    float to[4] = {position[0] + movement[0], position[1] + movement[1], position[2] + movement[2], 1.0f};
    // setCollisionData gathers 20 DC1 units around the effect (shot_utils.cpp:30).
    const float half = 20.0f * s->effect_scale;
    mgVu0FBOX   box;
    for (int k = 0; k < 3; k++) {
        box.max[k] = std::fmax(from[k], to[k]) + half;
        box.min[k] = std::fmin(from[k], to[k]) - half;
    }
    box.max[3] = box.min[3] = 1.0f;
    static CCPoly polys[128];
    const int     count = map->GetColPoly(polys, box, 128);
    float         hit[4];
    if (count > 0 && CheckHit(polys, count, from, to, hit, 1, 4) >= 0) {
        sceVu0CopyVector(hit_out, hit);
        hit_out[3] = 1.0f;
        return IXION_DC1_SHOT_HIT_MAP;
    }
    return IXION_DC1_SHOT_HIT_NONE;
}

void Bind(CActionChara *body) {
    CloseAllWindows("rebind");
    EndBurst("rebind");
    EndRunner("rebind");
    state = ToanCombatState{};
    state.body = body;
    state.generation = IxionToanBodyGeneration(body);
    state.dagger = IxionToanBodyDagger(body);
    // Goro R2: the character, its KEYs and its named points come from the library table, not Toan's constants.
    const int character = IxionDc1BodyCharacter();
    if (!ixion_dc1_character_info(character, &state.info)) {
        ixion_dc1_character_info(IXION_DC1_CHARA_TOAN, &state.info);
    }
    state.tip = state.dagger != nullptr ? FindFrame(state.dagger->CObjectFrame::frame, state.info.hit_point) : NULL;

    IxionDc1ToanConfig &config = state.config;
    CHRINFO_KEY_SET   *keys = body->key_list[0];
    const int          key_count = body->key_num[0];
    for (int i = 0; keys != NULL && i < IXION_DC1_TOAN_MOTIONS && i < key_count; i++) {
        config.motions[i] = {(float) keys[i].start_frame, (float) keys[i].end_frame, keys[i].step};
    }
    // The body setup divided the donor KEY steps by NormalDrive's 1.2 once at load; the donor reads DC1's own step.
    for (int k = 0; k < state.info.donor_key_count; k++) {
        const int i = state.info.donor_keys[k];
        if (keys != NULL && i < key_count) config.motions[i].step = ixion_toan::DonorStep(keys[i].step);
    }
    const Dc1Weapon &weapon = StartWeapon(state.info.character);
    config.character = state.info.character;
    config.attack = weapon.attack;
    config.weapon_speed = weapon.speed;
    config.element = weapon.element;
    config.action_gauge = 100.0f;
    // Dark Cloud's skill_owned[0] (UserStatus) gates the level-2 charge and the charged shot. It is cleared on
    // a new game (dngstatusdata.cpp:917) and set only by the story script _SSKILL_GET (editloop3.cpp:7419),
    // so a fresh Toan with the shop Dagger has it off: a held charge releases the jump attack (action 0xF).
    // Goro's handler reads no skill flag.
    const char *skill = std::getenv("DC2_IXION_TOAN_CHARGED_SKILL");
    config.charged_skill = skill != nullptr && std::strcmp(skill, "1") == 0 ? 1 : 0;
    // Ungaga R1: a moving effect (c10a_ex) runs in the library's CSHOT_EFFECT runner, at DC2's room scale; Toan's and
    // Goro's stationary bursts stay on BurstClock, so their config leaves the runner off.
    config.effect_key_count = ixion_toan::RunnerEffectKeys(state.info.effect, config.effect_keys);
    state.effect_scale = body->base_scale[0] > 0.0f ? body->base_scale[0] : 1.5584791f;
    config.effect_scale = config.effect_key_count > 0 ? state.effect_scale : 0.0f;
    state.runner_on = config.effect_key_count > 0;
    // Xiao R1: a character that fills the CSHOT pool (Xiao) gets it at the body scale, with the same collider.
    state.cshot_on = state.info.cshot_pool > 0;
    config.shot_scale = state.cshot_on ? state.effect_scale : 0.0f;
    ixion_dc1_reset(&config);
    ixion_dc1_set_point_resolver(ResolvePoint, &state);
    ixion_dc1_set_shot_collider(state.runner_on || state.cshot_on ? ShotCollide : nullptr, &state);
    IXLOG("bound character=%d (%s) weapon=%s attack=%d speed=%d element=%d hit_point=%s shot_point=%s loop_key=%d "
          "release_key=%d effect=%s r=%.0f life=%d\n",
          state.info.character, state.info.name, weapon.name, weapon.attack, weapon.speed, weapon.element,
          state.info.hit_point, state.info.shot_point[0] != '\0' ? state.info.shot_point : "(body pos)",
          state.info.charge_loop_key, state.info.release_key, state.info.effect, state.info.effect_radius,
          ixion_toan::ShotLifeTicks(state.info.effect));
    if (state.info.character == IXION_DC1_CHARA_XIAO) {
        IXLOG("donor keys 11=%.0f-%.0f step=%.3f 12=%.0f-%.0f step=%.3f 13=%.0f-%.0f step=%.3f (dc2 step13 %.3f) "
              "cshot_pool=%d shot_scale=%.4f tip=%s:%d generation=%d\n",
              config.motions[11].start, config.motions[11].end, config.motions[11].step, config.motions[12].start,
              config.motions[12].end, config.motions[12].step, config.motions[13].start, config.motions[13].end,
              config.motions[13].step, keys != NULL && 13 < key_count ? keys[13].step : -1.0f, state.info.cshot_pool,
              config.shot_scale, state.info.hit_point, state.tip != NULL ? 1 : 0, state.generation);
        return;
    }
    if (state.info.character == IXION_DC1_CHARA_UNGAGA) {
        IXLOG("donor keys 13=%.0f-%.0f step=%.3f 14=%.0f-%.0f step=%.3f 15=%.0f-%.0f step=%.3f 37=%.0f-%.0f "
              "38=%.0f-%.0f 39=%.0f-%.0f step37=%.3f (dc2 step37 %.3f) runner=%d effect_keys=%d scale=%.4f "
              "generation=%d\n",
              config.motions[13].start, config.motions[13].end, config.motions[13].step, config.motions[14].start,
              config.motions[14].end, config.motions[14].step, config.motions[15].start, config.motions[15].end,
              config.motions[15].step, config.motions[37].start, config.motions[37].end, config.motions[38].start,
              config.motions[38].end, config.motions[39].start, config.motions[39].end, config.motions[37].step,
              keys != NULL && 37 < key_count ? keys[37].step : -1.0f, state.runner_on ? 1 : 0,
              config.effect_key_count, config.effect_scale, state.generation);
        return;
    }
    if (state.info.character == IXION_DC1_CHARA_GORO) {
        IXLOG("donor keys 13=%.0f-%.0f step=%.3f 14=%.0f-%.0f step=%.3f 36=%.0f-%.0f step=%.3f (dc2 steps %.3f %.3f "
              "%.3f) generation=%d\n",
              config.motions[13].start, config.motions[13].end, config.motions[13].step, config.motions[14].start,
              config.motions[14].end, config.motions[14].step, config.motions[36].start, config.motions[36].end,
              config.motions[36].step, keys != NULL && 13 < key_count ? keys[13].step : -1.0f,
              keys != NULL && 14 < key_count ? keys[14].step : -1.0f,
              keys != NULL && 36 < key_count ? keys[36].step : -1.0f, state.generation);
        return;
    }
    IXLOG(
                 "donor keys 3=%.0f-%.0f 14=%.0f-%.0f 15=%.0f-%.0f 16=%.0f-%.0f 17=%.0f-%.0f "
                 "24=%.0f-%.0f 25=%.0f-%.0f step15=%.3f charged_skill=%d generation=%d\n",
                 config.motions[3].start, config.motions[3].end, config.motions[14].start, config.motions[14].end,
                 config.motions[15].start, config.motions[15].end, config.motions[16].start, config.motions[16].end,
                 config.motions[17].start, config.motions[17].end, config.motions[24].start, config.motions[24].end,
                 config.motions[25].start, config.motions[25].end, config.motions[15].step, config.charged_skill,
                 state.generation);
    IXLOG(
                 "bound body keys=%d dagger=%d dcol1=%d attack=%d speed=%d gauge=100 "
                 "combo keys 36-40 = %.0f-%.0f %.0f-%.0f %.0f-%.0f %.0f-%.0f %.0f-%.0f step=%.3f (dc2 step %.3f)\n",
                 key_count, state.dagger != nullptr ? 1 : 0, state.tip != NULL ? 1 : 0, weapon.attack, weapon.speed,
                 config.motions[36].start, config.motions[36].end, config.motions[37].start, config.motions[37].end,
                 config.motions[38].start, config.motions[38].end, config.motions[39].start, config.motions[39].end,
                 config.motions[40].start, config.motions[40].end, config.motions[36].step,
                 keys != NULL && 36 < key_count ? keys[36].step : -1.0f);
}

void OpenWindow(int slot, const IxionDc1ToanHit &hit, int action_no, float scale) {
    ToanPrim &w = state.window[slot];
    w = ToanPrim{};
    CColPrim *prim = ColPrimMan.GetPrim();
    w.no = state.windows.win[slot].no;
    w.action = action_no;
    w.damage = hit.damage;
    if (prim == NULL || !prim->SetDamage(kMaxMeleeRow, 0)) {
        IXLOG("frame=%d window open n=%d slot=%d failed (prim=%p)\n",
                     mgPortFrameCount(), w.no, slot, (void *) prim);
        return;
    }
    float point[4] = {hit.position[0], hit.position[1], hit.position[2], 1.0f};
    float point_b[4] = {hit.position[0], hit.position[1], hit.position[2], 1.0f};
    prim->SetCoord(point, point_b, ixion_toan::Dc2PrimRadius(hit.radius, scale));
    prim->damage = hit.damage;
    w.prim = prim;
    w.incarnation = IxionToanPrimIncarnation(prim);
    w.param = prim->param;
    w.min_dist = EnemyDistance(hit.position);
    IXLOG(
                 "frame=%d window open n=%d slot=%d action=0x%x dc1_radius=%.1f dc2_radius=%.2f "
                 "damage=%d knockback=%.1f tip=(%.1f,%.1f,%.1f) tip_enemy_dist=%.1f enemy_hp=%d\n",
                 mgPortFrameCount(), w.no, slot, action_no, hit.radius, ixion_toan::Dc2PrimRadius(hit.radius, scale),
                 hit.damage, hit.knockback_speed, hit.position[0], hit.position[1], hit.position[2],
                 EnemyDistance(hit.position), EnemyHp());
}

void MoveWindow(int slot, const IxionDc1ToanHit &hit, float scale) {
    ToanPrim &w = state.window[slot];
    if (w.prim == nullptr) return;
    float point[4] = {hit.position[0], hit.position[1], hit.position[2], 1.0f};
    float point_b[4] = {hit.position[0], hit.position[1], hit.position[2], 1.0f};
    w.prim->SetCoord(point, point_b, ixion_toan::Dc2PrimRadius(hit.radius, scale));
    const float distance = EnemyDistance(hit.position);
    if (distance >= 0.0f && distance < w.min_dist) w.min_dist = distance;
}

// DC1's CSHOT_EFFECT::Step for Toan's c01_fuusya (shot_effect.cpp:171-211), on DC2's clock: while phase 0 plays,
// a record (radius 20, life 2, the shot's damage) every wait + 1 steps at the effect's fixed position. Each
// record is one DC2 prim with Max's melee row at the DC2 radius conversion; a yellow DC2 hit flash marks it.
void StepBurst(float scale) {
    if (state.burst_prim.prim != nullptr && --state.burst_record_ticks <= 0) {
        const bool live = state.burst_prim.Live();
        const int  hits = live ? state.burst_prim.prim->hit_num : -1;
        if (live) state.burst_prim.prim->Delete(-1);
        IXLOG("frame=%d charged shot record close n=%d pulse=%d prim_hits=%d enemy_hp=%d\n",
                     mgPortFrameCount(), state.burst_no, state.burst_pulses, hits, EnemyHp());
        state.burst_prim = ToanPrim{};
    }
    if (!state.burst.live) return;
    if (state.burst.Step()) {
        ++state.burst_pulses;
        CColPrim *prim = ColPrimMan.GetPrim();
        if (prim == NULL || !prim->SetDamage(kMaxMeleeRow, 0)) {
            IXLOG("frame=%d charged shot record failed (prim=%p)\n", mgPortFrameCount(),
                         (void *) prim);
        } else {
            const float radius = ixion_toan::Dc2PrimRadius(state.burst_radius, scale);
            prim->SetCoord(state.burst_pos, state.burst_pos, radius);
            prim->damage = state.burst_damage;
            state.burst_prim.prim = prim;
            state.burst_prim.incarnation = IxionToanPrimIncarnation(prim);
            state.burst_prim.param = prim->param;
            state.burst_record_ticks = ixion_toan::kShotRecordLife;
            float flash[4] = {state.burst_pos[0], state.burst_pos[1] + 10.0f, state.burst_pos[2], 1.0f};
            if (nowScene__2 != nullptr) HitEffectSet(nowScene__2, flash, 0);
            IXLOG(
                         "frame=%d charged shot record n=%d pulse=%d age=%d pos=(%.1f,%.1f,%.1f) "
                         "dc1_radius=%.1f dc2_radius=%.2f damage=%d enemy_dist=%.1f enemy_hp=%d\n",
                         mgPortFrameCount(), state.burst_no, state.burst_pulses, state.burst.age - 1,
                         state.burst_pos[0], state.burst_pos[1], state.burst_pos[2], state.burst_radius, radius,
                         state.burst_damage, EnemyDistance(state.burst_pos), EnemyHp());
        }
    }
    if (!state.burst.live) EndBurst("motion-end");
}

void WeaponTip(CActionChara *body, float *out) {
    if (state.tip != NULL) {
        state.tip->GetWorldPosition0(out);
    } else {
        body->GetPosition(out);
    }
    out[3] = 1.0f;
}

// Ungaga R1: the library's effect runner moved every live c10a_ex this tick (CSHOT_EFFECT::Step). Each record it
// laid becomes one DC2 prim with Max's melee row (owner 0, so it never hits Max) for the record's 2-step life; a
// monster in reach takes it through DC2's own damage path. DC2 cannot draw the DC1 c10a_ex model (no converted
// effect pack), so the stand-in is DC2's hit spark: one at each record and a trail every 4 steps of flight.
const char *ShotEndName(int why) {
    switch (why) {
        case IXION_DC1_SHOT_END_MOTION: return "motion-end";
        case IXION_DC1_SHOT_END_NO_MOTION: return "no-motion";
        case IXION_DC1_SHOT_END_OFF: return "off";
        case IXION_DC1_SHOT_END_LIFE: return "life";
        case IXION_DC1_SHOT_END_HIT: return "hit";
    }
    return "?";
}
const char *ShotHitName(int hit) {
    switch (hit) {
        case IXION_DC1_SHOT_HIT_MAP: return "wall";
        case IXION_DC1_SHOT_HIT_PLAYER: return "player";
        case IXION_DC1_SHOT_HIT_MONSTER: return "monster";
    }
    return "none";
}

void StepRunner(const IxionDc1Output &output, const IxionDc1Input &input, float scale) {
    const int now = mgPortFrameCount();
    // The scene the shots were fired in went away (room retired, floor change): they go with it.
    if (state.runner_scene != nullptr && nowScene__2 != state.runner_scene) EndRunner("scene");
    for (int slot = 0; slot < IXION_DC1_SHOT_SLOTS; slot++) {
        ToanCombatState::RunnerShot &r = state.runner[slot];
        if (r.prim.prim != nullptr && !r.prim.Live()) {
            IXLOG("frame=%d shot record prim lost slot=%d (engine freed it)\n", now, slot);
            r.prim = ToanPrim{};
            r.record_ticks = 0;
        }
        if (r.prim.prim != nullptr && --r.record_ticks <= 0) CloseRunnerPrim(slot, "life");
    }
    if (output.shot.spawned != 0) {
        IXLOG("frame=%d charged shot set slot=%d time=%.4f pos=(%.1f,%.1f,%.1f) target=(%.1f,%.1f,%.1f) damage=%d "
              "wait=%d attribute=%d owner=%d sub=%d effect=%s gauge=%.2f\n",
              now, output.shot.slot, input.time, output.shot.position[0], output.shot.position[1],
              output.shot.position[2], output.shot.vector[0], output.shot.vector[1], output.shot.vector[2],
              output.shot.damage, output.shot.wait, output.shot.attribute, output.shot.owner, output.shot.sub_id,
              output.shot.effect, output.action_gauge);
    }
    for (int slot = 0; slot < IXION_DC1_SHOT_SLOTS; slot++) {
        const IxionDc1ShotStep &s = output.shots[slot];
        ToanCombatState::RunnerShot &r = state.runner[slot];
        if (!s.active && s.ended == 0) continue;
        if (s.serial != r.shown) {
            CloseRunnerPrim(slot, "reuse");
            r = ToanCombatState::RunnerShot{};
            r.serial = r.shown = s.serial;
            ++state.runner_spawns;
            state.runner_scene = nowScene__2;
            IXLOG("frame=%d shot spawn n=%d slot=%d pos=(%.1f,%.1f,%.1f) velocity_dc1=(%.3f,%.3f,%.3f) "
                  "step_dc2=%.3f phase=%d life=%d damage=%d owner=%d enemy_dist=%.1f\n",
                  now, s.serial, slot, s.position[0], s.position[1], s.position[2], s.velocity[0], s.velocity[1],
                  s.velocity[2],
                  std::sqrt(s.velocity[0] * s.velocity[0] + s.velocity[1] * s.velocity[1] +
                            s.velocity[2] * s.velocity[2]) * scale,
                  s.phase, s.phase_delay, s.damage, s.owner, EnemyDistance(s.position));
        }
        const float distance = EnemyDistance(s.position);
        if (distance >= 0.0f && (r.min_enemy < 0.0f || distance < r.min_enemy)) r.min_enemy = distance;
        if (s.collision != IXION_DC1_SHOT_HIT_NONE) {
            if (s.collision > 0 && s.collision < 4) ++state.runner_collisions[s.collision];
            IXLOG("frame=%d shot collide n=%d slot=%d age=%d phase=%d hit=%s pos=(%.1f,%.1f,%.1f) enemy_dist=%.1f\n",
                  now, s.serial, slot, s.age, s.phase, ShotHitName(s.collision), s.position[0], s.position[1],
                  s.position[2], distance);
        }
        if (s.record != 0) {
            CloseRunnerPrim(slot, "next");
            ++r.records;
            CColPrim *prim = ColPrimMan.GetPrim();
            const float radius = ixion_toan::Dc2PrimRadius(s.record_radius, scale);
            if (prim == NULL || !prim->SetDamage(kMaxMeleeRow, 0)) {
                IXLOG("frame=%d shot record failed n=%d (prim=%p)\n", now, s.serial, (void *) prim);
            } else {
                float point[4] = {s.record_position[0], s.record_position[1], s.record_position[2], 1.0f};
                prim->SetCoord(point, point, radius);
                prim->damage = s.damage;
                r.prim.prim = prim;
                r.prim.incarnation = IxionToanPrimIncarnation(prim);
                r.prim.param = prim->param;
                r.prim.no = s.serial;
                r.prim.damage = s.damage;
                r.record_ticks = s.record_life > 0 ? s.record_life : 1;
                if (nowScene__2 != nullptr) HitEffectSet(nowScene__2, point, 0);
            }
            IXLOG("frame=%d shot record n=%d slot=%d age=%d phase=%d pos=(%.1f,%.1f,%.1f) dc1_radius=%.1f "
                  "dc2_radius=%.2f damage=%d kind=%d flags=%d enemy_dist=%.1f enemy_hp=%d\n",
                  now, s.serial, slot, s.age, s.phase, s.record_position[0], s.record_position[1],
                  s.record_position[2], s.record_radius, radius, s.damage, s.record_kind, s.record_flags,
                  EnemyDistance(s.record_position), EnemyHp());
        } else if (s.active && (s.age % 4) == 0 && nowScene__2 != nullptr) {
            float spark[4] = {s.position[0], s.position[1], s.position[2], 1.0f};
            HitEffectSet(nowScene__2, spark, 0);
        }
        if (s.active && (s.age % 10) == 0) {
            IXLOG("frame=%d shot fly n=%d slot=%d age=%d phase=%d pos=(%.1f,%.1f,%.1f) effect_time=%.2f "
                  "phase_delay=%d enemy_dist=%.1f\n",
                  now, s.serial, slot, s.age, s.phase, s.position[0], s.position[1], s.position[2], s.effect_time,
                  s.phase_delay, distance);
        }
        if (s.ended != 0) {
            IXLOG("frame=%d shot end n=%d slot=%d why=%s age=%d phase=%d pos=(%.1f,%.1f,%.1f) records=%d "
                  "min_enemy_dist=%.1f enemy_hp=%d\n",
                  now, s.serial, slot, ShotEndName(s.ended), s.age, s.phase, s.position[0], s.position[1],
                  s.position[2], r.records, r.min_enemy, EnemyHp());
            r.serial = 0; // the record prim, if any, lives out its 2 steps
        }
    }
    if (output.shot_ended != 0) IXLOG("frame=%d EndEffect: live shots %d\n", now, output.shots_live);
}

// Xiao R1: the library's CSHOT pool after this tick's CSHOT::step. Every step of a live shot is logged (position and
// the distance to the room's enemy), so a hit after flight shows its steps. A record (life 1, radius 3, laid where
// the shot stood when it touched) becomes one DC2 prim with Max's melee row: owner 0, so it never hits Max, and a
// monster in reach takes it through DC2's own damage path. DC2 cannot draw DC1's basefx01 sling cell (CSHOT::draw),
// so the stand-in is DC2's hit spark every 4 steps and at the record.
void StepCShots(const IxionDc1Output &output, float scale) {
    const int now = mgPortFrameCount();
    if (state.runner_scene != nullptr && nowScene__2 != state.runner_scene) EndRunner("scene");
    for (int slot = 0; slot < IXION_DC1_CSHOT_SLOTS; slot++) {
        ToanCombatState::RunnerShot &r = state.cshot[slot];
        if (r.prim.prim != nullptr && !r.prim.Live()) {
            IXLOG("frame=%d cshot record prim lost slot=%d (engine freed it)\n", now, slot);
            r.prim = ToanPrim{};
            r.record_ticks = 0;
        }
        if (r.prim.prim != nullptr && --r.record_ticks <= 0) CloseCShotPrim(slot, "life");
    }
    if (output.point_missing > 0) IXLOG("frame=%d sling point missing (%d): no shot\n", now, output.point_missing);
    for (int slot = 0; slot < IXION_DC1_CSHOT_SLOTS; slot++) {
        const IxionDc1ShotStep &s = output.cshots[slot];
        ToanCombatState::RunnerShot &r = state.cshot[slot];
        if (!s.active && s.ended == 0) continue;
        const float distance = EnemyDistance(s.position);
        if (s.serial != r.shown) {
            CloseCShotPrim(slot, "reuse");
            r = ToanCombatState::RunnerShot{};
            r.serial = r.shown = s.serial;
            ++state.cshot_spawns;
            state.runner_scene = nowScene__2;
            IXLOG("frame=%d cshot spawn n=%d slot=%d age=%d pos=(%.1f,%.1f,%.1f) vector_dc2=(%.3f,%.3f,%.3f) "
                  "speed_dc2=%.3f speed_dc1=%.3f life=%d damage=%d owner=%d live=%d enemy_dist=%.1f enemy_hp=%d\n",
                  now, s.serial, slot, s.age, s.position[0], s.position[1], s.position[2], s.velocity[0], s.velocity[1],
                  s.velocity[2],
                  std::sqrt(s.velocity[0] * s.velocity[0] + s.velocity[1] * s.velocity[1] + s.velocity[2] * s.velocity[2]),
                  std::sqrt(s.velocity[0] * s.velocity[0] + s.velocity[1] * s.velocity[1] + s.velocity[2] * s.velocity[2]) /
                      scale,
                  s.life, s.damage, s.owner, output.cshots_live, distance, EnemyHp());
        } else if (s.active) {
            IXLOG("frame=%d cshot fly n=%d slot=%d age=%d pos=(%.1f,%.1f,%.1f) life=%d enemy_dist=%.1f\n", now, s.serial,
                  slot, s.age, s.position[0], s.position[1], s.position[2], s.life, distance);
        }
        if (distance >= 0.0f && (r.min_enemy < 0.0f || distance < r.min_enemy)) r.min_enemy = distance;
        if (s.collision != IXION_DC1_SHOT_HIT_NONE) {
            if (s.collision > 0 && s.collision < 4) ++state.cshot_collisions[s.collision];
            IXLOG("frame=%d cshot collide n=%d slot=%d age=%d hit=%s pos=(%.1f,%.1f,%.1f) enemy_dist=%.1f\n", now,
                  s.serial, slot, s.age, ShotHitName(s.collision), s.position[0], s.position[1], s.position[2], distance);
        }
        if (s.record != 0) {
            CloseCShotPrim(slot, "next");
            ++r.records;
            CColPrim   *prim = ColPrimMan.GetPrim();
            const float radius = ixion_toan::Dc2PrimRadius(s.record_radius, scale);
            float       point[4] = {s.record_position[0], s.record_position[1], s.record_position[2], 1.0f};
            if (prim == NULL || !prim->SetDamage(kMaxMeleeRow, 0)) {
                IXLOG("frame=%d cshot record failed n=%d (prim=%p)\n", now, s.serial, (void *) prim);
            } else {
                prim->SetCoord(point, point, radius);
                prim->damage = s.damage;
                r.prim.prim = prim;
                r.prim.incarnation = IxionToanPrimIncarnation(prim);
                r.prim.param = prim->param;
                r.prim.no = s.serial;
                r.prim.damage = s.damage;
                r.record_ticks = s.record_life > 0 ? s.record_life : 1;
                if (nowScene__2 != nullptr) HitEffectSet(nowScene__2, point, 0);
            }
            IXLOG("frame=%d cshot record n=%d slot=%d age=%d pos=(%.1f,%.1f,%.1f) dc1_radius=%.1f dc2_radius=%.2f "
                  "damage=%d life=%d target=%d kind=%d flags=%d enemy_dist=%.1f enemy_hp=%d\n",
                  now, s.serial, slot, s.age, point[0], point[1], point[2], s.record_radius, radius, s.damage,
                  s.record_life, s.record_target, s.record_kind, s.record_flags, EnemyDistance(point), EnemyHp());
        } else if (s.active && (s.age % 4) == 0 && nowScene__2 != nullptr) {
            float spark[4] = {s.position[0], s.position[1], s.position[2], 1.0f};
            HitEffectSet(nowScene__2, spark, 0);
        }
        if (s.ended != 0) {
            IXLOG("frame=%d cshot end n=%d slot=%d why=%s age=%d pos=(%.1f,%.1f,%.1f) records=%d min_enemy_dist=%.1f "
                  "enemy_hp=%d\n",
                  now, s.serial, slot, ShotEndName(s.ended), s.age, s.position[0], s.position[1], s.position[2],
                  r.records, r.min_enemy, EnemyHp());
            r.serial = 0; // the record prim, if any, lives out its step
        }
    }
}

// Xiao R1: DC1's lock-on point, BtActStatus.target_position = the locked monster's position raised 8 DC1 units
// (dun/gameloop.cpp:9271-9278), from DC2's lock-on target (CActionChara::target_no, actionchara.hpp:392).
void LockOnTarget(CActionChara *self, float scale, float out[4]) {
    out[0] = out[1] = out[2] = 0.0f;
    out[3] = 1.0f;
    if (self->lock_on == 0 || self->target_no < 0 || nowScene__2 == nullptr) return;
    CCharacter2 *target = nowScene__2->GetCharacter(self->target_no);
    if (target == nullptr) return;
    target->GetPosition(out);
    out[1] += 8.0f * scale;
    out[3] = 1.0f;
}

#endif

// Hook A, before pad_history/acumu_pad and the damage requests: take Cross for Dark Cloud's handler and keep it
// (and, mid-swing, the stick) from Max's script. Circle (lock-on, ACTION_CANCEL) stays with DC2.
void ToanCombatBeginImpl(CActionChara *self, ToanCombatFrame &frame) {
    frame.active = false;
    frame.jumping = 0;
#if defined(IXION_DC1_TOAN_COMBAT)
    if (!ToanCombatEnabled() || self != MainChara__2 || DngStatus.eye_view ||
        IxionToanBodyDagger(self) == nullptr) {
        // Ungaga R1: the player slot now runs someone else (quick change to Ridepod or Monica, a town): his shots
        // stop with him, as nothing steps them any more.
        if (ToanCombatEnabled() && self == MainChara__2 && (state.runner_on || state.cshot_on)) EndRunner("swap");
        return;
    }
    if (ixion_toan::NeedsBind(state.body, state.generation, self, IxionToanBodyGeneration(self))) {
        // A new body, or the same scene slot set up again (Ridepod or Monica and back to Max): SetupToanBody
        // reloaded the keys and the Dagger, so the tip and the bound config are stale.
        Bind(self);
    }
    state.suspended = false;
    if (self->damage_req == ACTION_DAMAGE_REQ_DEAD) {
        // DC2 kills the player (RunScript clears the request below): Dark Cloud's state starts over, gauge full.
        // Ordinary hits keep going through the donor's cancel branch, which keeps the gauge, as in DC1.
        const float gauge = state.hud_gauge;
        CloseAllWindows("dead");
        EndBurst("dead");
        EndRunner("dead");
        ixion_dc1_reset(&state.config);
        state.asserted_motion = -1;
        state.last_action = 0;
        state.jumping = 0;
        state.landing_guard = {};
        state.hud_gauge = state.config.action_gauge;
        state.hud_charge_time = 0.0f;
        state.hud_charge_level = 0;
        state.hud_action = 0;
        IXLOG("frame=%d dead: donor reset gauge %.2f -> %.2f\n", mgPortFrameCount(),
                     gauge, state.hud_gauge);
    }
    frame.active = true;
    frame.cross_down = PadCtrl.Btn(PAD_BTN_ACTION_CONFIRM) != 0;
    frame.cross_held = PadCtrl.Btn(PAD_BTN_ACTION_HELD) != 0;
    frame.cancel = self->damage_req != ACTION_DAMAGE_REQ_NONE;
    const float camera_angle = action_info.camera != nullptr ? action_info.camera->GetAngle() : 0.0f;
    const float stick_x = GamePad__2.GetLXf();
    const float stick_y = GamePad__2.GetLYf();
    frame.move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    frame.move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    frame.saved_pad = GamePad__2.pad[0];
    frame.saved_previous = GamePad__2.previous_pad[0];
    const int confirm = PadCtrl.btn[PAD_BTN_ACTION_CONFIRM].config & ~PAD_CTRL_TRIGGER_MASK;
    // Every Cross slot (CONFIRM, CONFIRM_2, ACTION_CONFIRM, ACTION_HELD); Hook B gives them back only if the
    // donor did not take the press.
    frame.cross_take.Take(PadCtrl);
    GamePad__2.pad[0].button &= ~confirm;
    GamePad__2.previous_pad[0].button &= ~confirm;
    if (state.asserted_motion >= 0) {
        GamePad__2.pad[0].left_x = GamePad__2.pad[0].left_y = 0x80;
    }
    if (self->melee_hit != 0) {
        // A native monster took the donor window's prim (monster.cpp:2282). Max's melee-hit program is not run.
        ++state.native_hits;
        IXLOG("frame=%d native melee hit #%d confirmed enemy_hp=%d\n",
                     mgPortFrameCount(), state.native_hits, EnemyHp());
        self->melee_hit = false;
    }
#else
    (void) self;
#endif
}

// Hook B, after Max's script and before CollisionCheck: tick the donor handler and apply its outputs once.
void ToanCombatTickImpl(CActionChara *self, ToanCombatFrame &frame) {
    if (!frame.active) {
        return;
    }
#if defined(IXION_DC1_TOAN_COMBAT)
    GamePad__2.pad[0] = frame.saved_pad;
    GamePad__2.previous_pad[0] = frame.saved_previous;

    IxionDc1ToanInput input{};
    input.time = self->frame;
    self->GetPosition(input.position);
    input.position[3] = 1.0f;
    WeaponTip(self, input.weapon_tip);
    float rotation[4];
    self->GetRotation(rotation);
    input.yaw = rotation[1];
    input.move_x = frame.move_x;
    input.move_z = frame.move_z;
    // Dark Cloud's ground height (pos - ground point, DC1 units) from DC2's last MoveCheck. The handler reads
    // last tick's value in Dark Cloud too: its MoveCheck runs after the key handler.
    const float body_scale = self->base_scale[0] > 0.0f ? self->base_scale[0] : 1.5584791f;
    const float height = ixion_toan::GroundHeight(self->move_check.landed != 0, self->move_check.ground_found != 0,
                                                  input.position[1], self->move_check.ground_point[1], body_scale);
    input.ground_height = state.landing_guard.Filter(state.last_action, height);
    if (state.landing_guard.fired) {
        IXLOG("frame=%d landing guard: 0x19 for %d ticks, ground reported\n",
                     mgPortFrameCount(), state.landing_guard.ticks);
        state.landing_guard = {};
    }
    input.cross_down = frame.cross_down;
    input.cross_held = frame.cross_held;
    input.lock_on = self->lock_on != 0;
    input.cancel = frame.cancel;
    // Xiao R1: the locked-on monster's point for Play_Jinn's aimed shot. DC2 has no first-person sling, so aimed and
    // the view angles stay 0 (body aim only).
    LockOnTarget(self, body_scale, input.target_position);
    static IxionDc1ToanOutput output;
    ixion_dc1_tick(&input, &output);

    const int   now = mgPortFrameCount();
    const float scale = self->base_scale[0] > 0.0f ? self->base_scale[0] : 1.5584791f;
    // Cross stays taken for consumers after RunScript. IsEventRun runs earlier in DngMainKey,
    // so this hook cannot suppress that event pass; event retirement has its own boundary.
    const bool donor_owns = output.action_on != 0;
    const bool pressed = frame.cross_take.saved[0] != 0 || frame.cross_take.saved[1] != 0;
    if (frame.cross_take.Settle(PadCtrl, donor_owns)) {
        if (pressed) ++state.cross_returned;
    } else {
        // Only PadCtrl's values stay taken. GamePad__2 is restored in full above: its pad becomes next frame's
        // previous_pad, and a cleared Cross bit there turns a held Cross into a fresh press every frame.
        if (pressed) {
            ++state.cross_kept;
            IXLOG(
                         "frame=%d cross kept from field prompts (PAD_BTN_CONFIRM now %d) action=0x%x "
                         "kept=%d returned=%d\n",
                         now, PadCtrl.Btn(PAD_BTN_CONFIRM), output.action_no, state.cross_kept, state.cross_returned);
        }
    }
    if (frame.cross_down) {
        IXLOG(
                     "frame=%d cross time=%.2f -> action_on=%d action=0x%x motion=%d combo_window=%d "
                     "queued=%d gauge=%.2f cooldown=%d\n",
                     now, input.time, output.action_on, output.action_no, output.motion_no, output.combo_window,
                     output.combo_queued, output.action_gauge, output.cooldown);
    }
    if (frame.cancel && state.asserted_motion >= 0) {
        IXLOG("frame=%d cancelled by DC2 damage request\n", now);
    }

    if (output.action_on != 0 && output.motion_no >= 0 && output.motion_no < IXION_DC1_TOAN_MOTIONS) {
        const int  motion = output.motion_no;
        const int  flags = CHARA_MOTION_RESTART | (motion != state.info.charge_loop_key ? CHARA_MOTION_HOLD : 0);
        const bool changed = motion != state.asserted_motion;
        // Re-asserted every tick with the same flags: a no-op unless the KEY changes (character.cpp:754, 1195).
        self->SetMotion(motion, flags);
        if (changed) {
            if (state.asserted_motion < 0) ++state.combo_no;
            IXLOG(
                         "frame=%d combo=%d step action=0x%x motion=%d (was %d) time=%.2f gauge=%.2f\n",
                         now, state.combo_no, output.action_no, motion, state.asserted_motion, input.time,
                         output.action_gauge);
            state.asserted_motion = motion;
        } else if (output.time != input.time) {
            self->SetNowFrame(output.time);
        }
    } else if (state.asserted_motion >= 0) {
        IXLOG("frame=%d combo=%d end last_action=0x%x time=%.2f gauge=%.2f cooldown=%d\n",
                     now, state.combo_no, state.last_action, input.time, output.action_gauge, output.cooldown);
        state.asserted_motion = -1;
    }
    state.last_action = output.action_no;
    state.hud_gauge = output.action_gauge;
    state.hud_charge_time = output.action_no == state.info.charge_loop_key ? output.charge_time : 0.0f;
    state.hud_charge_level = output.charge_level;
    state.hud_action = output.action_on != 0 ? output.action_no : 0;
    if (output.action_no == state.info.charge_loop_key && output.action_on != 0 && (now % 10) == 0) {
        IXLOG("frame=%d charging charge_time=%.3f level=%d held=%d\n", now,
                     output.charge_time, output.charge_level, frame.cross_held);
    }

    // Dark Cloud's jump: the donor's whole vertical step replaces DC2's velocity[1] (one gravity, DC1's).
    if (output.jumping != 0) {
        if (state.jumping == 0) {
            state.jump_ticks = 0;
            state.jump_start_y = input.position[1];
            state.jump_apex = 0.0f;
            IXLOG("frame=%d jump launch action=0x%x y=%.2f dc1_vy=%.2f dc2_vy=%.2f\n",
                         now, output.action_no, input.position[1], output.velocity[1],
                         ixion_toan::VerticalStep(output.velocity[1], scale));
        }
        // Once DC2 reports the floor, a falling step is held at 0, as Dark Cloud's MoveCheck holds Toan on it
        // while the slam finishes (the donor keeps lowering jump_velocity_y until 0x19 sees the ground).
        float step = ixion_toan::VerticalStep(output.velocity[1], scale);
        if (step < 0.0f && self->move_check.landed != 0) step = 0.0f;
        self->velocity[1] = step;
        frame.jumping = 1;
        const float rise = input.position[1] - state.jump_start_y;
        if (rise > state.jump_apex) state.jump_apex = rise;
        if ((state.jump_ticks % 4) == 0) {
            IXLOG(
                         "frame=%d jump t=%d action=0x%x y=%.2f rise=%.2f dc2_vy=%.2f landed=%d "
                         "ground_height_dc1=%.2f\n",
                         now, state.jump_ticks, output.action_no, input.position[1], rise, self->velocity[1],
                         self->move_check.landed, input.ground_height);
        }
        ++state.jump_ticks;
    } else if (state.jumping != 0) {
        IXLOG(
                     "frame=%d jump end action=0x%x ticks=%d apex_dc2=%.2f apex_dc1=%.2f y=%.2f "
                     "landed=%d\n",
                     now, output.action_no, state.jump_ticks, state.jump_apex, state.jump_apex / scale,
                     input.position[1], self->move_check.landed);
    }
    state.jumping = output.jumping;

    // Dark Cloud makes Toan untouchable through the jump attack and the charged shot (invincible_frames = 0x1770,
    // charakey.cpp:496, 519); DC2's player damage check skips while muteki_time > 0 (actionchara.cpp:2971).
    if (output.action_on != 0 && output.invincible != 0) {
        if (self->muteki_time < 2) self->muteki_time = 2;
    }

    // Donor push (DunMoveChara, movechara.cpp:1252-1262) goes through DC2's one CollisionCheck below.
    self->velocity[0] += output.velocity[0] * scale;
    self->velocity[2] += output.velocity[2] * scale;
    if (std::fabs(output.yaw - input.yaw) > 1.0e-6f) {
        self->SetRotation(0.0f, output.yaw, 0.0f);
    }

    // Engine-freed prims (scene load, chara change) are logged and forgotten, never deleted again.
    for (int slot = 0; slot < kWindowSlots; slot++) {
        if (state.window[slot].prim != nullptr && !state.window[slot].Live()) {
            CloseWindow(slot, "prim-lost");
            state.windows.Drop(slot);
        }
    }
    // Every donor record this tick, one DC2 window per collision slot (R4 used only the last record).
    const int dropped = state.windows.dropped;
    // Xiao R1: Xiao lays no swing records (SwordDmgCheck1 is a stub); every record in hits[] is a CSHOT hit, which
    // StepCShots turns into its own prim, so none opens a melee window.
    const int swing_records = state.cshot_on ? 0 : output.hit_count;
    if (state.cshot_on && output.hit_count > 0) state.cshot_records_skipped += output.hit_count;
    state.windows.Apply(
        output.hits, swing_records, output.action_no, frame.cancel != 0,
        [&](int slot, const IxionDc1ToanHit &hit) { OpenWindow(slot, hit, output.action_no, scale); },
        [&](int slot, const IxionDc1ToanHit &hit) { MoveWindow(slot, hit, scale); },
        [](int slot, const char *why) { CloseWindow(slot, why); });
    if (state.windows.dropped != dropped) {
        IXLOG("frame=%d %d donor records beyond window slot %d not mirrored\n", now,
                     state.windows.dropped - dropped, kWindowSlots);
    }

    // Goro R2: the smash's damage depends on which DC2 sample lands in [482, 482.4) (charakey.cpp:991-1020):
    // every sample around it is logged with the records and the shot it produced.
    if (state.info.character == IXION_DC1_CHARA_GORO && output.action_no == state.info.release_key &&
        input.time >= 480.0f && input.time <= 484.0f) {
        IXLOG("frame=%d smash sample time=%.4f records=%d record_damage=%d shot=%d shot_damage=%d\n", now,
              input.time, output.hit_count, output.hit_count > 0 ? output.hits[0].damage : 0, output.shot.spawned,
              output.shot.damage);
    }

    // DC1's camera shake (Goro: 2/0.01/20 after the swing, 8/0.1/30 with the smash shot, charakey.cpp:1017-1025)
    // becomes DC2's battle-area quake, the one _CAMERA_QUAKE sets (actscript.cpp:442-455): power, linear decay.
    if (output.camera_shake > 0.0f && output.camera_shake_frames > 0 && nowScene__2 != nullptr) {
        DNG_BATTLE_AREA *area = &nowScene__2->battle_area;
        area->quake_power = output.camera_shake;
        area->quake_step = output.camera_shake / (float) output.camera_shake_frames;
        area->quake_count = output.camera_shake_frames;
        ++state.quakes;
        IXLOG("frame=%d camera quake #%d power=%.2f frames=%d (dc1 decay %.2f)\n", now, state.quakes,
              output.camera_shake, output.camera_shake_frames, output.camera_shake_decay);
    }

    // Dark Cloud's charged shot: NowMainEffect->Set on release, at Toan's position (charakey.cpp:462-475) or at
    // Goro's hammer dcol0 (1009-1020). Both effects are stationary; the radius and life are the character's.
    if (output.shot.spawned != 0 && !state.runner_on) {
        EndBurst("respawn");
        ++state.burst_no;
        std::memcpy(state.burst_pos, output.shot.position, sizeof(state.burst_pos));
        state.burst_pos[3] = 1.0f;
        state.burst_damage = output.shot.damage;
        state.burst_radius = output.shot.effect_radius > 0.0f ? output.shot.effect_radius : ixion_toan::kShotRadius;
        state.burst_pulses = 0;
        int life = ixion_toan::ShotLifeTicks(output.shot.effect);
        if (life <= 0) life = 1;
        state.burst.Start(output.shot.wait, life);
        IXLOG("frame=%d charged shot spawn n=%d time=%.4f pos=(%.1f,%.1f,%.1f) damage=%d wait=%d attribute=%d "
              "owner=%d life=%d radius=%.0f speed=0 (stationary %s)\n",
              now, state.burst_no, input.time, state.burst_pos[0], state.burst_pos[1], state.burst_pos[2],
              output.shot.damage, output.shot.wait, output.shot.attribute, output.shot.owner, life, state.burst_radius,
              output.shot.effect);
    }
    if (output.point_fallbacks > 0) {
        static int warned = 0;
        if (warned++ < 4) {
            IXLOG("frame=%d %d of %d named points fell back to the weapon tip\n", now, output.point_fallbacks,
                  output.point_queries);
        }
    }
    StepBurst(scale);
    if (state.runner_on) StepRunner(output, input, scale);
    if (state.cshot_on) StepCShots(output, scale);
#else
    (void) self;
#endif
}

// Max's script damage entries (EntryDamage2) stay registered, but make no prim while Dark Cloud's combat runs.
void ToanCombatSuppressEntry(ACTION_DAMAGE *entry, float frame) {
    if (entry->prim != NULL) {
        entry->prim->Delete(-1);
        entry->prim = NULL;
    }
    if (frame >= entry->start_frame && frame < entry->end_frame) {
        static int logged = 0;
        if (logged < 12) {
            ++logged;
            IXLOG(
                         "frame=%d suppressed Max damage entry range=%.1f-%.1f now=%.2f radius=%.2f\n",
                         mgPortFrameCount(), entry->start_frame, entry->end_frame, frame, entry->radius);
        }
    }
}

} // namespace

void ToanCombatBegin(CActionChara *self, ToanCombatFrame &frame) { ToanCombatBeginImpl(self, frame); }
void ToanCombatTick(CActionChara *self, ToanCombatFrame &frame) { ToanCombatTickImpl(self, frame); }

void IxionToanCombatSuspend(CCharacter2 *body, const char *why) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (state.body == nullptr || (body != nullptr && state.body != body)) return;
    CloseAllWindows(why);
    EndBurst(why);
    EndRunner(why);
    ixion_dc1_toan_suspend();
    state.suspended = true;
    state.asserted_motion = -1;
    state.last_action = 0;
    state.jumping = 0;
    state.landing_guard = {};
    state.hud_charge_time = 0.0f;
    state.hud_charge_level = 0;
    state.hud_action = 0;
#endif
}

void IxionToanCombatRetire(CCharacter2 *body, const char *why) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (state.body == nullptr || (body != nullptr && state.body != body)) return;
    CloseAllWindows(why);
    EndBurst(why);
    EndRunner(why);
    ixion_dc1_toan_reset(&state.config);
    state = ToanCombatState{};
#endif
}

// Every reset and handout ends the previous allocation, even with the same row and owner.
std::uint64_t IxionToanPrimIncarnation(const CColPrim *prim) {
    const int index = PrimIndex(prim);
    return index >= 0 ? prim_incarnations[index] : 0;
}

void IxionToanCombatPrimInvalidated(const CColPrim *prim) {
    const int index = PrimIndex(prim);
    if (index >= 0) ++prim_incarnations[index];
#if defined(IXION_DC1_TOAN_COMBAT)
    for (int slot = 0; slot < kWindowSlots; ++slot) {
        if (state.window[slot].prim == prim) {
            state.window[slot].prim = nullptr;
            state.windows.Drop(slot);
        }
    }
    if (state.burst_prim.prim == prim) state.burst_prim.prim = nullptr;
    for (auto &r : state.runner) if (r.prim.prim == prim) r.prim.prim = nullptr;
    for (auto &r : state.cshot) if (r.prim.prim == prim) r.prim.prim = nullptr;
#endif
}

bool IxionToanCombatOwnsPrim(const CColPrim *prim) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (prim == nullptr || !ToanCombatEnabled()) return false;
    if (state.burst_prim.prim == prim && state.burst_prim.Live()) return true;
    for (const auto &w : state.window) if (w.prim == prim && w.Live()) return true;
    for (const auto &r : state.runner) if (r.prim.prim == prim && r.prim.Live()) return true;
    for (const auto &r : state.cshot) if (r.prim.prim == prim && r.prim.Live()) return true;
#endif
    return false;
}

// Ungaga R1: the Ixion room retired (ixion_room.cpp RetireScene): shots flying in it end now.
void IxionToanCombatRoomRetired(const char *reason) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (state.runner_on || state.cshot_on) EndRunner(reason != nullptr ? reason : "room-retire");
#else
    (void) reason;
#endif
}

bool IxionToanCombatDrivesKeys() { return ToanCombatEnabled(); }

void IxionToanCombatRescaleKeys(CCharacter2 *body) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (!ToanCombatEnabled() || body == nullptr) return;
    IxionDc1CharacterInfo info{};
    if (!ixion_dc1_character_info(IxionDc1BodyCharacter(), &info)) return;
    ixion_toan::RescaleKeys(body->key_list[0], body->key_num[0], info.donor_keys, info.donor_key_count);
#else
    (void) body;
#endif
}

bool IxionToanCombatHud(IxionToanHud *out) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (!ToanCombatEnabled() || state.suspended || state.body == nullptr || state.body != MainChara__2 ||
        IxionToanBodyDagger(state.body) == nullptr) {
        return false; // Ridepod or Monica in Toan's scene slot: their own HUD.
    }
    out->gauge = state.hud_gauge;
    out->charge_time = state.hud_charge_time;
    out->charge_level = state.hud_charge_level;
    out->action = state.hud_action;
    return true;
#else
    (void) out;
    return false;
#endif
}

// Seam for the later Dagger durability round: every DC2 wear request for Toan's donor prims lands here.
// DC1 would charge the Dagger's durability_f with BattleSubWeaponDmg (hitvalue.cpp:37-66) on each monster hit;
// until that is routed, Toan's hits wear nothing, and Max's equipped wrench is never touched.
static bool ToanCombatTakeWear(CActiveMonster *monster, CColPrim *col_prim) {
#if defined(IXION_DC1_TOAN_COMBAT)
    if (!ToanCombatEnabled() || col_prim == nullptr) return false;
    if (!IxionToanCombatOwnsPrim(col_prim)) return false;
    ++state.wear_skipped;
    const bool toan = state.info.character == IXION_DC1_CHARA_TOAN;
    const bool ungaga = state.info.character == IXION_DC1_CHARA_UNGAGA;
    const bool xiao = state.info.character == IXION_DC1_CHARA_XIAO;
    if (state.wear_skipped <= 12) IXLOG("frame=%d dc2 weapon wear skipped for %s prim #%d (monster whp=%d; %s durability deferred)\n",
          mgPortFrameCount(), toan ? "Toan's" : ungaga ? "Ungaga's" : xiao ? "Xiao's" : "Goro's", state.wear_skipped,
          monster != nullptr ? (int) monster->whp : -1, toan ? "dagger" : ungaga ? "stick" : xiao ? "slingshot" : "hammer");
    return true;
#else
    (void) monster;
    (void) col_prim;
    return false;
#endif
}

// Verbatim ps2/src/maintex.cpp calcWeaponParamWhp, plus the Ixion Toan gate (marked).
PORT_OVERRIDE void calcWeaponParamWhp(CActiveMonster *monster, CColPrim *col_prim) {
    CBattleCharaInfo    *battle;
    BATTLE_WEAPON_PARAM *weapon_param;
    int                  kind;
    float                old_whp;
    float                wear;
    u32                  status;

    // Ixion: Dark Cloud's Toan hits do not wear Max's equipped weapon.
    if (ToanCombatTakeWear(monster, col_prim)) {
        return;
    }

    battle = GetBattleCharaInfo();
    weapon_param = battle->weapon_param;
    kind = col_prim->param->kind;

    if (kind == DAMAGE_KIND_MONICA_MELEE || kind == DAMAGE_KIND_MAX_MELEE ||
        kind == DAMAGE_KIND_RIDEPOD_PUNCH || kind == DAMAGE_KIND_RIDEPOD_SWORD) {
        old_whp = (float) battle->GetWhpNowVol(0);
        wear = (float) (u_int) monster->whp;
        wear *= 0.5f;
        wear -= (float) (wear * (0.005 * weapon_param[0].status[WEAPON_STAT_DURABILITY]));
        status = col_prim->status;

        if (status & WEAPON_SPECIAL_INCREASE_WEAR) {
            wear *= 1.3f;
        }

        if (status & WEAPON_SPECIAL_REDUCE_WEAR) {
            wear *= 0.8f;
        }

        if (battle->AddWhp(0, -wear) <= 0.0f && old_whp > 0.0f) {
            battle->AddAbsRate(0, -0.1f, NULL);
        }
    }
}

/**
 *
 * Gives the radius with which a character of the given body width is checked against the map.
 *
 */
static inline float MoveCheckRadius(float width) {
    return 4.0f + 2.0f * width;
}

// Verbatim ps2/src/actionchara.cpp CActionChara::RunScript, plus the two Toan combat hooks (marked Ixion).
PORT_OVERRIDE void CActionChara::RunScript(CScene *scene, RUN_SCRIPT_ENV *env) {
    int                  pallet_u;
    int                  history;
    CBattleCharaInfo    *battle;
    CSphida             *sphida;
    int                  poly_count;
    float                frame;
    int                  sword_no;
    int                  pallet_no;
    int                  nearest_target;
    int                  pallet_v;
    int                  index;
    int                  foot_sound;
    ACTION_DAMAGE       *entry;
    DNG_BATTLE_AREA     *area;
    CColPrim            *reversed;
    CTreasureBoxManager *treasure;
    CMap                *map;
    float                target_distance;

    nowScene__2 = scene;
    area = &scene->battle_area;
    action_info.chara = this;
    action_info.camera = static_cast<mgCCameraFollow *>(scene->GetCamera(scene->GetCameraID("MainCam")));
    action_info.env = env;
    sceVu0FVECTOR adjusted_velocity = {0.0f, 0.0f, 0.0f, 0.0f};
    sceVu0FVECTOR old_velocity;
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR new_position;
    CCPoly        polys[128];
    mgVu0FBOX     box;
    // Ixion hook A: Dark Cloud's Toan takes Cross before the script sees it.
    ToanCombatFrame toan_combat;
    ToanCombatBegin(this, toan_combat);
    menu_flag = false;
    dir_gun = false;
    history = action_info.chara->pad_history;
    history |= PadCtrl.Btn(PAD_BTN_ACTION_CONFIRM);
    action_info.chara->pad_history = history;

    if (PadCtrl.Btn(PAD_BTN_ACTION_HELD) != 0) {
        action_info.chara->acumu_pad++;
    } else {
        action_info.chara->acumu_pad = 0;
    }

    if (melee_hit != 0) {
        prog_no = ACTION_PROG_MELEE_HIT;
        melee_hit = false;
    }

    if (damage_req == ACTION_DAMAGE_REQ_DEAD) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DEAD;
        damage_req = ACTION_DAMAGE_REQ_NONE;
        scene->battle_area.script.event_no = 1200;
    }

    if (damage_req == ACTION_DAMAGE_REQ_HOLD) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_HOLD;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }

    if (damage_req == ACTION_DAMAGE_REQ_SMALL) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DAMAGE_SMALL;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }

    if (damage_req == ACTION_DAMAGE_REQ_LARGE) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DAMAGE_LARGE;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }

    if (prog_no != ACTION_PROG_RUNNING) {
        if (script.check_program(prog_no) != 0) {
            script.run(prog_no);
            prog_no = ACTION_PROG_RUNNING;
        }
    } else {
        script.resume();

        if (script.end != 0) {
            prog_no = ACTION_PROG_MAIN;
        }
    }

    // Ixion hook B: Dark Cloud's ToanKey_On/ToanKey_Play tick; motion, push, yaw and hit window applied here.
    ToanCombatTick(this, toan_combat);

    GetPosition(position);
    GetRotation(rotation);
    sceVu0CopyVector(old_velocity, velocity);
    CollisionCheck(position, old_velocity, adjusted_velocity);
    map = nowScene__2->GetMap(nowScene__2->active_map);
    box.max[0] = 40.0f + position[0];
    box.min[0] = position[0] - 40.0f;
    box.max[1] = 40.0f + position[1];
    box.min[1] = position[1] - 40.0f;
    box.max[2] = 40.0f + position[2];
    box.min[2] = position[2] - 40.0f;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    poly_count = map->GetColPoly(polys, box, 128);
    treasure = area->treasure_box;

    if (treasure != NULL) {
        poly_count += treasure->PickupCollision(position, &polys[poly_count], box, 128 - poly_count);
    }

    sphida = GetSphidaPtr();

    if (sphida != NULL) {
        poly_count += sphida->PickupCollision(position, &polys[poly_count], box, 128 - poly_count);
    }

    move_check.radius = MoveCheckRadius(body_width);
    MoveCheck(position, adjusted_velocity, new_position, &move_check, polys, poly_count, 1);
    adjusted_velocity[0] = new_position[0] - position[0];
    adjusted_velocity[2] = new_position[2] - position[2];
    battle = GetBattleCharaInfo();

    if (move_check.landed != 0) {
        foot_sound = move_check.ground_poly.foot_sound;

        if (foot_sound == 0 && map != NULL) {
            foot_sound = map->map_info.def_foot;
        }

        sound_info.foot_sound_id = foot_sound;
        adjusted_velocity[1] = 0.0f;
    } else {
        if (battle->chr_no == USER_CHARA_ROBO) {
            adjusted_velocity[1] -= 3.0f;

            if (adjusted_velocity[1] <= -5.0f) {
                adjusted_velocity[1] = -5.0f;
            }
        } else {
            adjusted_velocity[1] -= 0.5f;

            if (adjusted_velocity[1] <= -3.5f) {
                adjusted_velocity[1] = -3.5f;
            }
        }

        sound_info.foot_sound_id = -1;
    }

    if (new_position[1] < -500.0f) {
        new_position[1] = 500.0f;
    }

    SetPosition(new_position);
    sceVu0CopyVector(velocity, adjusted_velocity);

    // Ixion: not while Dark Cloud's jump attack lands Toan itself (its air/slam/fall/land actions, charakey.cpp:549-600).
    if (hold_type == ACTION_HOLD_NONE &&
        (battle->chr_no == USER_CHARA_MAX || battle->chr_no == USER_CHARA_MONICA) &&
        move_check.landed != 0 && old_velocity[1] <= -3.5f && toan_combat.jumping == 0) {
        SetMotion("\x92\x85\x92\x6E", 6, 1);
        prog_no = ACTION_PROG_LAND;
    }

    if (move_check.landed == 0) {
        menu_flag = false;
    }

    for (index = 0; index < ACTION_DAMAGE_MAX; index++) {
        entry = &damage[index];

        if (entry->use) {
            frame = GetNowFrame(entry->chara);

            // Ixion: with Dark Cloud's combat on, Max's script damage entries make no prims. Their frame ranges
            // come from Max's KEY names, which the Toan pack aliases onto the same DC1 frames 820-884.
            if (toan_combat.active) {
                ToanCombatSuppressEntry(entry, frame);
                continue;
            }

            if (entry->prim == NULL) {
                if (frame >= entry->start_frame && frame < entry->end_frame) {
                    entry->prim = ColPrimMan.GetPrim();

                    if (entry->prim != NULL) {
                        entry->prim->SetDamage(entry->damage, 0);
                        entry->prim->SetCoord(entry->frame0, entry->frame1, entry->radius);
                        SetDamageParam(entry->prim, 0);
                        entry->prim->damage = (int) (entry->prim->damage * entry->power_rate);
                    }
                }
            } else if (frame < entry->start_frame || frame > entry->end_frame) {
                entry->prim->Delete(-1);
                entry->prim = NULL;
            } else {
                reversed = ColPrimMan.IsReversVec(entry->prim);

                if (reversed != NULL) {
                    reversed->reversed = true;
                    reversed->GetReversVec(reversed->revers_vec);
                }
            }
        }
    }

    pallet_no = battle->GetPalletNo(0);

    if (pallet_no >= 0) {
        pallet_u = pallet_no % 2;
        pallet_v = pallet_no / 2;

        for (sword_no = 0; sword_no < CHARA_SWORD_EFFECT_MAX; sword_no++) {
            if (sword_effect[sword_no] != NULL) {
                sword_effect[sword_no]->SetTexture(pallet_u * 64, pallet_v * 32, 64, 32);
            }
        }
    }

    if (area->lock_on_mode == DNG_LOCK_ON_SELECT) {
        target_no = RockOn_TargetSel(scene, target_no);
    }

    if (area->lock_on_mode == DNG_LOCK_ON_NEAREST) {
        if (lock_on == 0) {
            target_no = DistCheck_Action2(scene, 0.5f, 400.0f, &target_distance, 0, NULL);
        } else {
            lock_on = Check_LockOn(scene, 300.0f, target_no);

            if (lock_on == 0) {
                nearest_target = DistCheck_Action2(scene, 0.5f, 400.0f, &target_distance, 0, NULL);

                if (nearest_target != -1) {
                    target_no = nearest_target;
                    lock_on = true;
                } else {
                    target_no = nearest_target;
                }
            }
        }
    }
}
