#include "common.h"
#include "mw_runtime.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "actionchara.hpp"
#include "character.hpp"
#include "colprim.hpp"
#include "dng_effect.hpp"
#include "dng_main.hpp"
#include "dng_status.hpp"
#include "ixion_dc1_monster.hpp"
#include "ixion_monica_faithful.hpp"
#include "ixion_room.hpp"
#include "mglib_port.hpp"
#include "monster.hpp"
#include "scene.hpp"
#include "userdata.hpp"

namespace {

constexpr int kMonicaFormMonster = 52; // Himarra, badge 4's first form (ixion_monica_form.cpp)
// Shift-JIS 攻撃１ (f216a's CROSS motion; the converted pack aliases it to Opar's dc1_13, the gill spit).
char kSpitMotion[] = "\x8D\x55\x8C\x82\x82\x50";
// Shift-JIS 攻撃２ (f216a's SQUARE motion; aliased to Opar's dc1_16, the belly-flop).
char kLungeMotion[] = "\x8D\x55\x8C\x82\x82\x51";
// The body's muzzle frame: DC1 'dcol1' after the converter's two-digit rename (profile frame_style).
char kMuzzleFrame[] = "dcol01";
// The belly-flop's hit frame: DC1 'dcol0' renamed the same way.
char kLungeFrame[] = "dcol00";
// Candidates farther than this are not aimed at (the ball reaches 180 frames x 0.4 x k = 112 units).
constexpr float kAimRange = 300.0f;
// Without a monster to aim at the ball flies straight ahead this far at muzzle height.
constexpr float kAimFallback = 100.0f;

IxionMonicaSpitGate   g_gate;
IxionMonicaLungeClock g_lunge;
bool                  g_logged_enabled = false;
int                 (*g_roll_source)(int) = nullptr;

// The belly-flop window and its bookkeeping (one swing at a time).
struct Lunge {
    CColPrim *prim = nullptr;
    int       opened = 0;
    int       lives[MONSTER_ACTIVE_MAX] = {};
    float     start[4] = {};
    int       hits_logged = 0;
    float     radius = 0.0f;
    float     travelled = 0.0f; // DC2 units asked of the body this swing
    bool      moving = false;
};
Lunge g_swing;

bool FormActive() {
    return DngUserData != nullptr && MainChara__2 != nullptr && DngUserData->active_chr_no == USER_CHARA_MONSTER &&
           DngUserData->monster_id == kMonicaFormMonster;
}

bool Paused() {
    if (ActiveMonster == nullptr || ActiveMonster->scene == nullptr) return false;
    DNG_BATTLE_AREA &area = ActiveMonster->scene->battle_area;
    return IxionDc1PauseRequested(DngStatus.mode, area.script.running != 0, area.pause_flag);
}

CScene *SceneOf() { return ActiveMonster != nullptr ? ActiveMonster->scene : nullptr; }

CActiveMonster *SceneMonster(CScene *scene, int chara_id) {
    if (scene == nullptr || chara_id < MONSTER_ACTIVE_MAX || chara_id >= 2 * MONSTER_ACTIVE_MAX) return nullptr;
    return static_cast<CActiveMonster *>(scene->GetCharacter(chara_id));
}

// Horizontal facing: from the body to its muzzle (dcol01 sits ahead of Opar's centre), else the yaw.
void Facing(CActionChara *body, mgCFrame *muzzle, const float from[4], float forward[2]) {
    float body_position[4];
    body->GetPosition(body_position);
    forward[0] = from[0] - body_position[0];
    forward[1] = from[2] - body_position[2];
    float length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
    if (length < 0.5f || muzzle == nullptr) {
        float rotation[4];
        body->GetRotation(rotation);
        forward[0] = std::sin(rotation[1]);
        forward[1] = std::cos(rotation[1]);
        length = 1.0f;
    }
    forward[0] /= length;
    forward[1] /= length;
}

// `cause` is "attack" (the Cross chain's fire frame) or "counter" (prog 110); `attacker` is the scene character
// id the counter-spit answers (-1 otherwise).
void Spit(CActionChara *body, const char *cause, int attacker) {
    CScene *scene = SceneOf();
    mgCFrame *muzzle = body->SearchObject(kMuzzleFrame);
    const CHRINFO_KEY_SET *key = body->now_key;
    if (scene == nullptr || muzzle == nullptr) {
        std::fprintf(stderr, "ixion monica faithful spit refused frame=%d scene=%d muzzle=%d\n", mgPortFrameCount(),
                     scene != nullptr ? 1 : 0, muzzle != nullptr ? 1 : 0);
        return;
    }
    float from[4];
    muzzle->GetWorldPosition0(from);
    from[3] = 1.0f;
    float forward[2];
    Facing(body, muzzle, from, forward);

    IxionMonicaCandidate list[MONSTER_ACTIVE_MAX];
    int                  ids[MONSTER_ACTIVE_MAX];
    int                  count = 0;
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        auto *monster = static_cast<CActiveMonster *>(scene->GetCharacter(MONSTER_ACTIVE_MAX + i));
        if (monster == nullptr || monster->state != ACTIVE_MONSTER_LIVE || monster->life <= 0) continue;
        if (IxionRoomMonsterSuspended(monster)) continue;
        float position[4];
        monster->GetPosition(position);
        position[1] += 0.5f * monster->body_height;
        position[3] = 1.0f;
        const float dx = position[0] - from[0], dz = position[2] - from[2];
        std::memcpy(list[count].position, position, sizeof position);
        list[count].distance = std::sqrt(dx * dx + dz * dz);
        ids[count++] = MONSTER_ACTIVE_MAX + i;
    }
    // DC1 aims once, at fire time. The counter-spit answers its attacker; the attack spit takes the monster the
    // player is locked on to, else the nearest in front. Never re-aimed afterwards (no homing).
    float       aim[4];
    int         chosen = -1;
    const char *how = "ahead";
    if (attacker >= 0) chosen = IxionMonicaFindLocked(true, attacker, ids, count);
    if (chosen >= 0) {
        how = "attacker";
    } else {
        chosen = IxionMonicaFindLocked(body->lock_on != 0, body->target_no, ids, count);
        if (chosen >= 0) how = "lock-on";
    }
    bool has_target = chosen >= 0;
    if (has_target) {
        for (int k = 0; k < 4; ++k) aim[k] = list[chosen].position[k];
        aim[3] = 1.0f;
    } else {
        has_target = IxionMonicaPickAim(from, forward, list, count, kAimRange, kAimFallback, aim, &chosen);
        if (has_target) how = "nearest";
    }

    CBattleCharaInfo *battle = GetBattleCharaInfo();
    const float max_hp = battle != nullptr && battle->GetMaxHp_i() > 0 ? (float) battle->GetMaxHp_i() : 32.0f;
    const bool spawned = IxionDc1SpawnPlayerShot(from, aim, kIxionMonicaSpitDamage, max_hp);
    std::fprintf(stderr,
                 "ixion monica faithful spit frame=%d cause=%s spawned=%d key=%s mframe=%.2f range=%d-%d "
                 "muzzle=(%.1f,%.1f,%.1f) facing=(%.2f,%.2f) aim=(%.1f,%.1f,%.1f) target=%s lock_on=%d target_no=%d "
                 "chara=%d dist=%.1f candidates=%d\n",
                 mgPortFrameCount(), cause, spawned ? 1 : 0,
                 key == nullptr ? "-" : std::strcmp(key->name, kSpitMotion) == 0 ? "attack1" : "other", body->frame,
                 key != nullptr ? (int) key->start_frame : -1, key != nullptr ? (int) key->end_frame : -1, from[0],
                 from[1], from[2], forward[0], forward[1], aim[0], aim[1], aim[2], has_target ? how : "ahead",
                 body->lock_on != 0 ? 1 : 0, (int) body->target_no, has_target ? ids[chosen] : -1,
                 has_target ? list[chosen].distance : -1.0f, count);
}

float CasterMaxHp() {
    CBattleCharaInfo *battle = GetBattleCharaInfo();
    return battle != nullptr && battle->GetMaxHp_i() > 0 ? (float) battle->GetMaxHp_i() : 32.0f;
}

void SnapshotLives(CScene *scene) {
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        CActiveMonster *monster = SceneMonster(scene, MONSTER_ACTIVE_MAX + i);
        g_swing.lives[i] = monster != nullptr && monster->state == ACTIVE_MONSTER_LIVE ? monster->life : -1;
    }
}

void LogLunge(const char *what, const char *why, CActionChara *body) {
    float position[4];
    body->GetPosition(position);
    const float dx = position[0] - g_swing.start[0], dz = position[2] - g_swing.start[2];
    std::fprintf(stderr,
                 "ixion monica faithful lunge %s frame=%d%s%s mframe=%.2f body=(%.1f,%.1f,%.1f) moved=%.2f "
                 "(asked %.2f, DC1 4.4 = %.2f)\n",
                 what, mgPortFrameCount(), why != nullptr ? " why=" : "", why != nullptr ? why : "", body->frame,
                 position[0], position[1], position[2], std::sqrt(dx * dx + dz * dz), g_swing.travelled,
                 4.4f * kIxionDc1UnitScale);
}

// Where each live monster's hit entries stand against the window: DC2 hits when the distance is within
// 2 x (window radius + entry size), so this line says whether a swing could have reached at all.
void LogReach(CScene *scene, const float point[4], float window_radius, const char *when) {
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        CActiveMonster *monster = SceneMonster(scene, MONSTER_ACTIVE_MAX + i);
        if (monster == nullptr || monster->state != ACTIVE_MONSTER_LIVE || monster->life <= 0) continue;
        int                 entry_no = 0;
        float               entry_position[4];
        CHARA_ENTRY_OBJECT *entry;
        while ((entry = monster->GetEntryObjectPos(CHARA_ENTRY_GROUP_HIT, entry_no, entry_position)) != nullptr) {
            const float cx = entry_position[0] - point[0], cz = entry_position[2] - point[2];
            if (entry->enable && cx * cx + cz * cz < 400.0f * 400.0f) {
                const float dx = entry_position[0] - point[0], dy = entry_position[1] - point[1],
                            dz = entry_position[2] - point[2];
                std::fprintf(stderr,
                             "ixion monica faithful lunge reach %s frame=%d chara=%d entry=%d dist=%.1f size=%.1f "
                             "needs<=%.1f %s\n",
                             when, mgPortFrameCount(), MONSTER_ACTIVE_MAX + i, entry_no,
                             std::sqrt(dx * dx + dy * dy + dz * dz), entry->size, 2.0f * (window_radius + entry->size),
                             std::sqrt(dx * dx + dy * dy + dz * dz) <= 2.0f * (window_radius + entry->size) ? "IN" : "out");
            }
            ++entry_no;
        }
    }
}

void CloseLunge(CActionChara *body, const char *why) {
    if (g_swing.prim == nullptr) return;
    CScene *scene = SceneOf();
    const bool live = IxionDc1PlayerBlowLive(g_swing.prim);
    const int  hits = live ? g_swing.prim->hit_num : -1;
    IxionDc1ClosePlayerBlow(g_swing.prim);
    g_swing.prim = nullptr;
    std::fprintf(stderr, "ixion monica faithful lunge window close frame=%d why=%s prim_hits=%d\n", mgPortFrameCount(),
                 why, hits);
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        CActiveMonster *monster = SceneMonster(scene, MONSTER_ACTIVE_MAX + i);
        const int       now = monster != nullptr && monster->state == ACTIVE_MONSTER_LIVE ? monster->life : -1;
        if (g_swing.lives[i] >= 0 && now != g_swing.lives[i]) {
            std::fprintf(stderr, "ixion monica faithful lunge monster chara=%d life %d -> %d\n", MONSTER_ACTIVE_MAX + i,
                         g_swing.lives[i], now);
        }
    }
    if (body != nullptr) LogLunge("close", why, body);
}

// One tick of the belly-flop: window, travel, hit.
void StepLunge(CActionChara *body, bool in_lunge) {
    const IxionMonicaLungeStep step = g_lunge.Update(in_lunge, body->frame);
    if (step.close) CloseLunge(body, step.close_why);
    if (step.move_frames > 0.0f) {
        mgCFrame *muzzle = body->SearchObject(kMuzzleFrame);
        float     from[4] = {};
        if (muzzle != nullptr) muzzle->GetWorldPosition0(from);
        float forward[2];
        Facing(body, muzzle, from, forward);
        const float distance = step.move_frames * kIxionMonicaLungeStep * kIxionDc1UnitScale;
        if (!g_swing.moving) {
            g_swing.moving = true;
            body->GetPosition(g_swing.start);
            g_swing.travelled = 0.0f;
            LogLunge("move start", nullptr, body);
        }
        // Consumed by this frame's RunScript (CollisionCheck + MoveCheck keep the body inside the walls).
        body->velocity[0] += forward[0] * distance;
        body->velocity[2] += forward[1] * distance;
        g_swing.travelled += distance;
    }
    if (step.open) {
        mgCFrame *frame = body->SearchObject(kLungeFrame);
        CScene   *scene = SceneOf();
        if (frame == nullptr || scene == nullptr) {
            std::fprintf(stderr, "ixion monica faithful lunge refused frame=%d frame=%d scene=%d\n", mgPortFrameCount(),
                         frame != nullptr ? 1 : 0, scene != nullptr ? 1 : 0);
            g_lunge.Finish();
        } else {
            float position[4];
            frame->GetWorldPosition0(position);
            position[3] = 1.0f;
            if (!g_swing.moving) body->GetPosition(g_swing.start);
            SnapshotLives(scene);
            g_swing.hits_logged = 0;
            g_swing.radius = IxionMonicaLungeRadius(body->body_width);
            g_swing.prim = IxionDc1OpenPlayerBlow(position, g_swing.radius, kIxionMonicaLungeDamage, CasterMaxHp());
            if (g_swing.prim == nullptr) {
                std::fprintf(stderr, "ixion monica faithful lunge refused frame=%d no free prim\n", mgPortFrameCount());
                g_lunge.Finish();
            } else {
                ++g_swing.opened;
                const float hp = IxionDc1BalanceHp();
                std::fprintf(stderr,
                             "ixion monica faithful lunge window open frame=%d n=%d mframe=%.2f dcol00=(%.1f,%.1f,%.1f) "
                             "radius=%.2f (DC1 half %.2f) damage=%d (DC1 %d, caster max hp %.0f, balance=%s basis=%.0f) owner=player "
                             "target=monsters row=kind3-blow\n",
                             mgPortFrameCount(), g_swing.opened, body->frame, position[0], position[1], position[2],
                             g_swing.radius, 0.5f * kIxionMonicaLungeRadius * kIxionDc1UnitScale, g_swing.prim->damage,
                             kIxionMonicaLungeDamage, CasterMaxHp(), hp == 70.0f ? "faithful" : "friendly", hp);
                std::fprintf(stderr, "ixion monica faithful lunge body_width=%.1f body_height=%.1f\n", body->body_width,
                             body->body_height);
                LogReach(scene, position, g_swing.radius, "open");
            }
        }
    }
    if (g_swing.prim != nullptr && !step.close) {
        if (!IxionDc1PlayerBlowLive(g_swing.prim)) {
            g_swing.prim = nullptr; // the engine freed it (scene change): forgotten, never deleted again
            g_lunge.Finish();
            std::fprintf(stderr, "ixion monica faithful lunge window lost frame=%d\n", mgPortFrameCount());
        } else {
            if (g_swing.prim->hit_num != g_swing.hits_logged) {
                g_swing.hits_logged = g_swing.prim->hit_num;
                std::fprintf(stderr, "ixion monica faithful lunge hit frame=%d mframe=%.2f prim_hits=%d\n",
                             mgPortFrameCount(), body->frame, g_swing.prim->hit_num);
            }
        }
        if (g_swing.prim != nullptr && !step.open) {
            mgCFrame *frame = body->SearchObject(kLungeFrame);
            if (frame != nullptr) {
                float position[4];
                frame->GetWorldPosition0(position);
                position[3] = 1.0f;
                IxionDc1MovePlayerBlow(g_swing.prim, position, g_swing.radius);
            }
        }
    }
    if (!in_lunge && g_swing.moving) {
        LogLunge("move end", nullptr, body);
        g_swing.moving = false;
    }
}

int RollPercent() { return g_roll_source != nullptr ? g_roll_source(100) : iRand(100); }

// Replay-only: a forced roll (-1 = not forced).
int ForcedRoll() {
    const char *forced = std::getenv("DC2_IXION_MONICA_COUNTER_ROLL");
    if (forced == nullptr || forced[0] == 0) return -1;
    const int value = std::atoi(forced);
    return value < 0 ? 0 : value > 99 ? 99 : value;
}

// Replay-only (DC2_IXION_MONICA_TEST_HIT=<frame>): see the header. A stock monster attack row, owned by the
// nearest live monster, overlaps the body for two ticks so DC2's own CheckDamage takes the hit.
char      kMonsterAttackRow[] = "\x83\x82\x83\x93\x83\x58\x83\x5E\x81\x5B\x8D\x55\x8C\x82"; // monster attack
CColPrim *g_test_prim = nullptr;
int       g_test_ticks = 0;
bool      g_test_done = false;

void StepTestHit(CActionChara *body) {
    if (g_test_prim != nullptr) {
        if (--g_test_ticks <= 0) {
            if (g_test_prim->active && g_test_prim->owner != 0) g_test_prim->Delete(-1);
            g_test_prim = nullptr;
        }
        return;
    }
    const char *at = std::getenv("DC2_IXION_MONICA_TEST_HIT");
    if (at == nullptr || g_test_done || mgPortFrameCount() < std::atoi(at)) return;
    CScene *scene = SceneOf();
    float   self[4];
    body->GetPosition(self);
    int   owner = -1;
    float best = 1.0e30f;
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        CActiveMonster *monster = SceneMonster(scene, MONSTER_ACTIVE_MAX + i);
        if (monster == nullptr || monster->state != ACTIVE_MONSTER_LIVE || monster->life <= 0) continue;
        if (IxionRoomMonsterSuspended(monster)) continue;
        float other[4];
        monster->GetPosition(other);
        const float dx = other[0] - self[0], dz = other[2] - self[2];
        if (dx * dx + dz * dz < best) {
            best = dx * dx + dz * dz;
            owner = MONSTER_ACTIVE_MAX + i;
        }
    }
    if (owner < 0) return;
    g_test_done = true;
    CColPrim *prim = ColPrimMan.GetPrim();
    if (prim == nullptr || !prim->SetDamage(kMonsterAttackRow, owner)) return;
    float point[4] = {self[0], self[1] + 10.0f, self[2], 1.0f};
    prim->damage = 6;
    prim->SetCoord(point, 20.0f);
    g_test_prim = prim;
    g_test_ticks = 2;
    std::fprintf(stderr,
                 "ixion monica faithful TEST-HIT frame=%d: stock monster attack prim owner=%d damage=6 on the body (replay "
                 "only; the rat never reaches this body)\n",
                 mgPortFrameCount(), owner);
}

} // namespace

bool IxionMonicaFaithfulEnabled() {
    const char *faithful = std::getenv("DC2_IXION_MONICA_FAITHFUL");
    const char *form = std::getenv("DC2_IXION_MONICA_FORM");
    const bool  enabled = faithful != nullptr && std::strcmp(faithful, "1") == 0 && form != nullptr &&
                          std::strcmp(form, "e104a") == 0;
    if (faithful != nullptr && std::strcmp(faithful, "1") == 0 && !enabled) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            std::fprintf(stderr, "ixion monica faithful refused: DC2_IXION_MONICA_FAITHFUL=1 requires DC2_IXION_MONICA_FORM=e104a\n");
        }
    }
    if (enabled && !g_logged_enabled) {
        g_logged_enabled = true;
        std::fprintf(stderr,
                     "ixion monica faithful: on (DC2_IXION_MONICA_FORM=e104a); Cross spits Opar's goo ball at motion "
                     "frame > %.0f, damage %d, owner player; Square lunges (window %.0f-%.0f, travel %.0f-%.0f); a hit "
                     "from > %.0f DC1 units may spit back (%d%%); balance basis %.0f\n",
                     kIxionMonicaSpitFireFrame, kIxionMonicaSpitDamage, kIxionMonicaLungeWindowStart,
                     kIxionMonicaLungeWindowEnd, kIxionMonicaLungeMoveStart, kIxionMonicaLungeMoveEnd,
                     kIxionMonicaCounterDistance, kIxionMonicaCounterPercent, IxionDc1BalanceHp());
    }
    return enabled;
}

void IxionMonicaFaithfulStep() {
    if (!IxionMonicaFaithfulEnabled()) return;
    if (!FormActive()) {
        g_gate = IxionMonicaSpitGate{};
        if (g_swing.prim != nullptr) CloseLunge(nullptr, "form-exit");
        g_lunge = IxionMonicaLungeClock{};
        g_swing.moving = false;
        if (IxionDc1PlayerShotsLive() > 0) IxionDc1RetirePlayerShots("form-exit");
        return;
    }
    CActionChara *body = MainChara__2;
    if (Paused()) {
        // The ball freezes with the donor pause and the motion does not advance either; a window does not wait.
        if (g_swing.prim != nullptr) {
            CloseLunge(body, "event-pause");
            g_lunge.Finish();
        }
        return;
    }
    const bool in_spit = body->now_key != nullptr && std::strcmp(body->now_key->name, kSpitMotion) == 0;
    const bool in_lunge = body->now_key != nullptr && std::strcmp(body->now_key->name, kLungeMotion) == 0;
    if (g_gate.Update(in_spit, body->frame)) Spit(body, "attack", -1);
    StepLunge(body, in_lunge);
    StepTestHit(body);
}

void IxionMonicaFaithfulRetire(const char *reason) {
    g_gate = IxionMonicaSpitGate{};
    if (g_swing.prim != nullptr) CloseLunge(nullptr, reason != nullptr ? reason : "room-retire");
    g_lunge = IxionMonicaLungeClock{};
    g_swing.moving = false;
    if (IxionDc1PlayerShotsLive() > 0) IxionDc1RetirePlayerShots(reason != nullptr ? reason : "room-retire");
}

void IxionMonicaOnPlayerHit(const CColPrim *hit, int damage) {
    if (hit == nullptr || damage <= 0 || !IxionMonicaFaithfulEnabled() || !FormActive()) return;
    CActionChara *body = MainChara__2;
    CScene       *scene = SceneOf();
    CActiveMonster *attacker = SceneMonster(scene, hit->owner);
    if (attacker == nullptr) {
        std::fprintf(stderr, "ixion monica faithful counter-spit frame=%d skipped: hit owner=%d is not a scene monster\n",
                     mgPortFrameCount(), hit->owner);
        return;
    }
    float self[4], other[4];
    body->GetPosition(self);
    attacker->GetPosition(other);
    const float dx = other[0] - self[0], dy = other[1] - self[1], dz = other[2] - self[2];
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const int   forced = ForcedRoll();
    const int   roll = forced >= 0 ? forced : RollPercent();
    const bool  fire = IxionMonicaCounterGate(distance, roll);
    std::fprintf(stderr,
                 "ixion monica faithful counter-spit frame=%d hit_by=%d damage=%d dist=%.1f (DC1 %.1f, gate > %.0f) "
                 "roll=%d (< %d)%s -> %s\n",
                 mgPortFrameCount(), hit->owner, damage, distance, distance / kIxionDc1UnitScale,
                 kIxionMonicaCounterDistance, roll, kIxionMonicaCounterPercent, forced >= 0 ? " ROLL-FORCED" : "",
                 fire ? "spit" : "no spit");
    if (fire) Spit(body, "counter", hit->owner);
}

void IxionMonicaSetRollSourceForTest(int (*source)(int)) { g_roll_source = source; }

void IxionMonicaResetForTest() {
    g_gate = IxionMonicaSpitGate{};
    g_lunge = IxionMonicaLungeClock{};
    g_swing = Lunge{};
    g_roll_source = nullptr;
    g_test_prim = nullptr;
    g_test_done = false;
}
