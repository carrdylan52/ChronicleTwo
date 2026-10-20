#pragma once

#include <cmath>

// Ixion opt-in (DC2_IXION_MONICA_FAITHFUL=1 together with DC2_IXION_MONICA_FORM=e104a): Monica's Himarra
// form wears Opar's converted DC1 body (ixion_monica_form.cpp). With this switch the body also does what
// Opar does with its first attack: at Opar's DC1 fire frame (F0419c spit, e104a.stb: motion 13, the shot
// leaves 'dcol1' once the motion frame passes 103, damage 35) a goo ball leaves the body's dcol01 frame.
// The ball is the DC1 shot code (ixion_dc1_monster.cpp) with the player as owner. Everything else stays
// f216a's: the pose chain 攻撃１in -> 攻撃１ -> 攻撃１out, its spin effect, its damage window.
// With the variable unset the whole module returns at its first test and nothing else changes.

// Both variables set to the supported combination. Read on every call (cheap), so tests can flip them.
bool IxionMonicaFaithfulEnabled();

// Called once per CMonsterMan::ThinkHost, beside IxionDc1StepShots. Fires the spit, and ends it on form exit.
void IxionMonicaFaithfulStep();

// Room retirement and stale discard: the ball does not outlive the room (or a floor change).
void IxionMonicaFaithfulRetire(const char *reason);

// DC1 e104a.stb F0419c: `_CHK_MOTION_FRM` frame above 103 and not yet fired in this motion.
constexpr float kIxionMonicaSpitFireFrame = 103.0f;
// _SET_SHOT('dcol1', ..., 35): the fifth argument overrides the damage with 35 (F0419c, c+0x04744).
constexpr int kIxionMonicaSpitDamage = 35;

// ---- Round 2 -------------------------------------------------------------------------------------------
// Square (f216a's 攻撃２ motion, the converted pack's alias of Opar's dc1_16 belly-flop): DC1 e104a.stb prog 1
// registers `_SET_DMG_COL('dcol0', 20, 138, 149)` + `_SET_DMG_PARA(35, 0, 3)` for it, and F038b8 steps the body
// forward (`_SET_MOVE(self + 20 dir, 0.4)`) only on motion frames 135-145. The lunge here is exactly that, on
// DC2's clock: a player-owned kind-3 blow window on the body's dcol00 frame for motion frames 138-149, and
// 0.4 DC1 units of forward travel per motion frame crossed inside 135-146 (4.4 DC1 units in all, whatever the
// motion step is). f216a's own Square behaviour (its goo lob from dcol01 at 39-42% of the motion) is untouched:
// it is a separate effect-script prim; the lunge window is this module's, closes on its first hit, and never
// shares a hit with the lob.
constexpr float kIxionMonicaLungeWindowStart = 138.0f;
constexpr float kIxionMonicaLungeWindowEnd = 149.0f;
constexpr float kIxionMonicaLungeMoveStart = 135.0f;
constexpr float kIxionMonicaLungeMoveEnd = 146.0f; // F038b8 moves on 135-145 and cancels at 146: 11 motion frames
constexpr float kIxionMonicaLungeStep = 0.4f;     // DC1 units per motion frame
constexpr int   kIxionMonicaLungeDamage = 35;     // _SET_DMG_PARA first argument
constexpr float kIxionMonicaLungeRadius = 20.0f;  // _SET_DMG_COL radius (DC1 units)
// DC1 prog 110 (the hit reaction, e104a.stb): when Opar is hit, the attacker is farther than 32 DC1 units and
// _GET_RAND(100) < 10, F0419c (the spit) runs.
constexpr float kIxionMonicaCounterDistance = 32.0f;
constexpr int   kIxionMonicaCounterPercent = 10;

// The window's DC2 radius. DC1's sphere (20 units; half of that times k is the DC2 prim radius the monster-side
// windows use) is 15.58. But DC2 keeps bodies apart by 2 x (the monster's body entry size + the player's
// body_width): about 70-80 for Monica's Opar body (body_width 30), measured, and the rat never stands closer. A
// window that reaches less than that touches nothing, so the radius is at least the contact radius
// (body_width + 8: reach 2 x (radius + a 5-unit hit entry) clears the 80-unit separation).
inline float IxionMonicaLungeRadius(float body_width) {
    const float dc1 = 0.5f * kIxionMonicaLungeRadius * 1.5584790683f;
    const float contact = body_width + 8.0f;
    return contact > dc1 ? contact : dc1;
}


struct IxionMonicaLungeStep {
    bool        open = false;       // open the window now
    bool        close = false;      // close the window now
    const char *close_why = "";     // "window-end", "motion-left", "rewind"
    float       move_frames = 0.0f; // motion frames crossed inside the movement range this tick
};

// Pure clock for one visit to the lunge motion. `in_lunge` is true while the body plays 攻撃２.
struct IxionMonicaLungeClock {
    bool  in_motion = false;
    bool  window_open = false;
    bool  done = false; // the window has run (or was ended by a hit) in this swing
    float last_frame = -1.0f;
    IxionMonicaLungeStep Update(bool in_lunge, float frame) {
        IxionMonicaLungeStep out;
        if (!in_lunge) {
            if (window_open) {
                out.close = true;
                out.close_why = "motion-left";
            }
            *this = IxionMonicaLungeClock{};
            return out;
        }
        if (!in_motion || frame < last_frame) {
            if (in_motion && window_open) {
                out.close = true;
                out.close_why = "rewind";
            }
            in_motion = true;
            window_open = false;
            done = false;
            last_frame = frame; // no travel counted for the tick that enters
        }
        const float from = last_frame < kIxionMonicaLungeMoveStart ? kIxionMonicaLungeMoveStart : last_frame;
        const float to = frame > kIxionMonicaLungeMoveEnd ? kIxionMonicaLungeMoveEnd : frame;
        if (to > from) out.move_frames = to - from;
        last_frame = frame;
        if (window_open && frame > kIxionMonicaLungeWindowEnd) {
            window_open = false;
            done = true;
            out.close = true;
            out.close_why = "window-end";
        } else if (!window_open && !done) {
            if (frame > kIxionMonicaLungeWindowEnd) {
                done = true; // entered past the window: this swing makes none
            } else if (frame >= kIxionMonicaLungeWindowStart) {
                window_open = true;
                out.open = true;
            }
        }
        return out;
    }
    // The window ended on its own (a hit, a lost prim): it does not reopen this swing.
    void Finish() {
        window_open = false;
        done = true;
    }
};

// Locked-on aim: the index of `target_no` in `ids` when the player is locked on, else -1 (the caller then takes
// the nearest monster in front). DC1's spit aims once at fire time at `_GET_POSITION(-2)`; the player's
// equivalent is the monster the lock-on holds.
inline int IxionMonicaFindLocked(bool lock_on, int target_no, const int *ids, int count) {
    if (!lock_on) return -1;
    for (int i = 0; i < count; ++i) {
        if (ids[i] == target_no) return i;
    }
    return -1;
}

// DC1 prog 110's gate: attacker farther than 32 DC1 units, roll below 10 of 100. Pure; `roll` is 0-99.
inline bool IxionMonicaCounterGate(float attacker_distance_dc2, int roll) {
    const bool far = attacker_distance_dc2 / 1.5584790683f > kIxionMonicaCounterDistance;
    return far && roll < kIxionMonicaCounterPercent;
}

// Replay-only (DC2_IXION_MONICA_TEST_HIT=<frame>): at that frame a monster-owned DC2 attack prim (the stock
// monster attack row, damage 6) overlaps the body for two ticks, so DC2's real CheckDamage accepts a hit from the
// nearest live monster and the hook below runs on it. No monster in the replay room reaches this body (the rat
// stays 77-80 units off and never hurts it), so the replay cannot get a natural hit.

// Called from the player's accepted-hit branch (CActionChara::CheckDamage, after the HP is taken). With the
// Himarra form on and the faithful switch set, may fire the counter-spit; otherwise returns at once.
class CColPrim;
void IxionMonicaOnPlayerHit(const CColPrim *hit, int damage);

// Test seams. The roll source replaces iRand(100) (null restores it). The replay proof's equivalent is the
// environment variable DC2_IXION_MONICA_COUNTER_ROLL=<0-99>, which replaces the roll only (logged ROLL-FORCED);
// the distance gate always runs on the real distance.
void IxionMonicaSetRollSourceForTest(int (*source)(int));
void IxionMonicaResetForTest();

// One spit per visit to the attack-1 motion: re-armed when the motion is left or its frame wraps back.
struct IxionMonicaSpitGate {
    bool  fired = false;
    float last_frame = -1.0f;
    // `in_spit` is true while the body plays 攻撃１ (Opar's motion 13). Returns true on the tick to fire.
    bool Update(bool in_spit, float frame) {
        if (!in_spit) {
            fired = false;
            last_frame = -1.0f;
            return false;
        }
        if (frame < last_frame) fired = false;
        last_frame = frame;
        if (fired || !(frame > kIxionMonicaSpitFireFrame)) return false;
        fired = true;
        return true;
    }
};

struct IxionMonicaCandidate {
    float position[4]; // aim point (monster centre)
    float distance;    // horizontal distance from the muzzle
};

// Aim: the nearest live monster in front of the body (forward is a horizontal unit vector) within
// `max_distance`, else a point `fallback` ahead of the muzzle at muzzle height. Returns true when a
// monster was chosen (`chosen` then indexes the list).
inline bool IxionMonicaPickAim(const float from[4], const float forward[2], const IxionMonicaCandidate *list,
                               int count, float max_distance, float fallback, float out[4], int *chosen) {
    int   best = -1;
    float best_distance = max_distance;
    for (int i = 0; i < count; ++i) {
        const float dx = list[i].position[0] - from[0];
        const float dz = list[i].position[2] - from[2];
        if (dx * forward[0] + dz * forward[1] <= 0.0f) continue; // behind the body
        if (list[i].distance >= best_distance) continue;
        best = i;
        best_distance = list[i].distance;
    }
    if (best >= 0) {
        for (int k = 0; k < 4; ++k) out[k] = list[best].position[k];
        out[3] = 1.0f;
    } else {
        out[0] = from[0] + forward[0] * fallback;
        out[1] = from[1];
        out[2] = from[2] + forward[1] * fallback;
        out[3] = 1.0f;
    }
    if (chosen != nullptr) *chosen = best;
    return best >= 0;
}
