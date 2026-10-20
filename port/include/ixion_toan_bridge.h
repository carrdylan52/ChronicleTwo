#ifndef IXION_TOAN_BRIDGE_H
#define IXION_TOAN_BRIDGE_H

/* Engine-free pieces of the DC2 Toan combat bridge (port/src/ixion_toan_combat.cpp), kept here so
 * ixdc1_toan_tests covers them headless: the per-slot hit-window table, the one-time KEY rescale,
 * the Cross take/settle rule and DC1's charged-shot clock. */

namespace ixion_toan {

/* DC1 KEYs the donor handler drives (charakey.cpp): 3 slam, 14 charge, 15 leap, 16 air, 17 land, 24 charged
 * shot, 25 fall, 36-40 the combo. */
constexpr int kDonorKeys[] = {3, 14, 15, 16, 17, 24, 25, 36, 37, 38, 39, 40};
/* DC2's NormalDrive plays every KEY at step * 1.2 (character.cpp:1193); Dark Cloud adds the KEY step once. */
constexpr float kDc2MotionSpeed = 1.2f;

/* Called once per body load, right after LoadPack builds the key list: the donor KEYs then play at Dark Cloud's
 * rate under NormalDrive. A reload builds a fresh list, so it is rescaled exactly once whatever its address
 * (R4 keyed a static guard on the list pointer; a reload at the same address read as already rescaled). */
template <class Key> void RescaleKeys(Key *keys, int key_count, const int *donor_keys, int donor_count) {
    for (int k = 0; k < donor_count; k++) {
        const int i = donor_keys[k];
        if (keys != nullptr && i >= 0 && i < key_count) keys[i].step /= kDc2MotionSpeed;
    }
}
template <class Key> void RescaleDonorKeys(Key *keys, int key_count) {
    RescaleKeys(keys, key_count, kDonorKeys, static_cast<int>(sizeof(kDonorKeys) / sizeof(kDonorKeys[0])));
}
/* The DC1 step Bind hands the donor for a rescaled donor KEY. */
inline float DonorStep(float dc2_step) { return dc2_step * kDc2MotionSpeed; }

/* A body needs a fresh Bind when the main character changes, or when the same scene slot was set up again
 * (quick change to Ridepod or Monica and back): SetupToanBody bumps the generation on every load. */
inline bool NeedsBind(const void *bound, int bound_generation, const void *body, int generation) {
    return bound != body || bound_generation != generation;
}

/* One DC2 hit window per donor collision slot. Toan currently emits one record per tick; separate slots
 * also preserve multiple records from other donor handlers. A window closes when its slot is absent,
 * its action or radius changes, or on cancel. */
template <int N> struct SlotWindows {
    struct Win {
        bool  open = false;
        int   action = -1;
        float radius = 0.0f;
        int   ticks = 0;
        int   no = 0;
    };
    Win win[N];
    int opened = 0;
    int dropped = 0; /* Records whose slot falls outside the table: counted, never silently merged. */

    template <class Hit, class Open, class Move, class Close>
    void Apply(const Hit *hits, int count, int action, bool cancel, Open &&open, Move &&move, Close &&close) {
        bool seen[N] = {};
        for (int i = 0; !cancel && i < count; i++) {
            const int slot = hits[i].slot;
            if (slot < 0 || slot >= N) {
                ++dropped;
                continue;
            }
            seen[slot] = true;
            Win &w = win[slot];
            if (w.open && (w.action != action || w.radius != hits[i].radius)) {
                close(slot, "next");
                w = Win{};
            }
            if (!w.open) {
                w.open = true;
                w.action = action;
                w.radius = hits[i].radius;
                w.ticks = 0;
                w.no = ++opened;
                open(slot, hits[i]);
            } else {
                move(slot, hits[i]);
            }
            ++w.ticks;
        }
        for (int slot = 0; slot < N; slot++) {
            if (win[slot].open && !seen[slot]) {
                close(slot, cancel ? "cancel" : "end");
                win[slot] = Win{};
            }
        }
    }
    template <class Close> void CloseAll(const char *why, Close &&close) {
        for (int slot = 0; slot < N; slot++) {
            if (win[slot].open) close(slot, why);
            win[slot] = Win{};
        }
    }
    /* The engine freed the slot's prim (scene load, chara change): forget it without a callback. */
    void Drop(int slot) { win[slot] = Win{}; }
    int OpenCount() const {
        int n = 0;
        for (const Win &w : win) n += w.open ? 1 : 0;
        return n;
    }
};

/* PadCtrl slots (padcontrol.hpp): CONFIRM 0, CONFIRM_2 0x10, ACTION_CONFIRM 0x32, ACTION_HELD 0x38. In English
 * all four are Cross. Hook A takes them before Max's script; after the donor tick, Hook B gives them back only
 * when Dark Cloud's handler did not take the press (no action running). This covers consumers after RunScript;
 * IsEventRun runs earlier in DngMainKey, so its control transition needs a separate retirement boundary. */
constexpr int kCrossSlots[] = {0x00, 0x10, 0x32, 0x38};
constexpr int kCrossSlotCount = 4;

struct CrossTake {
    int saved[kCrossSlotCount] = {};
    template <class Pad> void Take(Pad &pad) {
        for (int i = 0; i < kCrossSlotCount; i++) {
            saved[i] = pad.btn[kCrossSlots[i]].value;
            pad.btn[kCrossSlots[i]].value = 0;
        }
    }
    /* Returns true when the values were given back. */
    template <class Pad> bool Settle(Pad &pad, bool donor_owns) {
        if (donor_owns) return false;
        for (int i = 0; i < kCrossSlotCount; i++) pad.btn[kCrossSlots[i]].value = saved[i];
        return true;
    }
};

/* Toan's charged shot (MyEntryEffect_TOAN, Chronicle dun/gameloop.cpp:642-648): c01_fuusya, stationary,
 * speed 0 and radius 20 in phase 0, motion {0,-1,-1,-1}. */
constexpr float kShotRadius = 20.0f;
/* Phase 0 plays c01_fuusya KEY 0 (c01_fuusya.cfg "KEY 5, 40, 0.40") from 5 and ends at time >= 39
 * (shot_effect.cpp:146-155), when motion[1] == -1 retires it: (39 - 5) / 0.4 = 85 steps that can hit.
 * life_time stays -1 (Set, shot_effect.cpp:470), so only the motion ends it. */
constexpr int kShotLifeTicks = 85;
/* Each record lives 2 steps (NowColData->Set(..., 2, ...), shot_effect.cpp:181). */
constexpr int kShotRecordLife = 2;

/* Steps a stationary effect's phase 0 can lay records: CSHOT_EFFECT::Step retires it once its KEY 0 time reaches
 * end - 1 (shot_effect.cpp:146-155), from start at the KEY step. c01_fuusya KEY 0 is (5, 40, 0.40): 85 steps.
 * Goro's c06a_tameex (dun/mainchara/wep_eff/c06a_tameex.chr cfg "KEY 2, 20, 0.35") gives (19 - 2) / 0.35 =
 * 48.6, so 49 steps. Both entries are speed 0 with motion {0, -1, -1, -1} (MyEntryEffect_TOAN/GORO,
 * dun/gameloop.cpp:642-667): one phase, never moving. */
inline int StationaryShotSteps(float key_start, float key_end, float key_step) {
    const double steps = (static_cast<double>(key_end) - 1.0 - key_start) / key_step;
    const int whole = static_cast<int>(steps);
    return steps - whole > 1.0e-3 ? whole + 1 : whole; /* ceil, with 34 / 0.4 kept at 85 */
}
/* The life of the effect a character's shot names, or 0 for one the bridge does not know (one record then). */
inline int ShotLifeTicks(const char *effect) {
    auto same = [](const char *a, const char *b) {
        while (*a != '\0' && *a == *b) ++a, ++b;
        return *a == *b;
    };
    if (effect == nullptr) return 0;
    if (same(effect, "c01_fuusya")) return StationaryShotSteps(5.0f, 40.0f, 0.40f);
    if (same(effect, "c06a_tameex")) return StationaryShotSteps(2.0f, 20.0f, 0.35f);
    return 0;
}

/* Ungaga R1: the KEY rows of an effect the library's runner plays (CSHOT_EFFECT, shot_effect.cpp:126-479),
 * indexed by MyEntryEffect_*.motion[phase]. Ungaga's c10a_ex (dun/mainchara/wep_eff/c10a_ex.chr cfg) is
 * "KEY 5, 25, 0.6", "KEY 30, 50, 0.8", "KEY 55, 70, 0.6". The stationary c01_fuusya and c06a_tameex stay on
 * BurstClock below, so Toan and Goro do not use the runner. Returns the row count, 0 for an effect it does not run. */
template <class Motion> int RunnerEffectKeys(const char *effect, Motion *out) {
    auto same = [](const char *a, const char *b) {
        while (*a != '\0' && *a == *b) ++a, ++b;
        return *a == *b;
    };
    if (effect == nullptr || !same(effect, "c10a_ex")) return 0;
    out[0] = {5.0f, 25.0f, 0.6f};
    out[1] = {30.0f, 50.0f, 0.8f};
    out[2] = {55.0f, 70.0f, 0.6f};
    return 3;
}

/* CSHOT_EFFECT::Step's hit clock (shot_effect.cpp:171-211): a record whenever wait_state <= 0, then
 * wait_state = wait; otherwise wait_state--. With SetWait(45) that is one record every 46 steps. */
struct BurstClock {
    bool live = false;
    int  ticks_left = 0;
    int  wait = 0;
    int  wait_state = 0;
    int  age = 0;
    void Start(int wait_ticks, int life_ticks = kShotLifeTicks) {
        live = true;
        ticks_left = life_ticks;
        wait = wait_ticks;
        wait_state = 0;
        age = 0;
    }
    /* One host tick. Returns true when the effect lays a record this tick. */
    bool Step() {
        if (!live) return false;
        if (ticks_left <= 0) {
            live = false;
            return false;
        }
        bool pulse = false;
        if (wait_state <= 0) {
            pulse = true;
            wait_state = wait;
        } else {
            --wait_state;
        }
        --ticks_left;
        ++age;
        return pulse;
    }
};

/* DC1 hit radius to DC2 prim radius: Dark Cloud hits at dist <= r_monster + r_hit (collisiondata.hpp:148), DC2 at
 * dist <= 2 (r + entry size) (colprim.cpp:1072); the room scale converts donor units. */
inline float Dc2PrimRadius(float dc1_radius, float scale) { return dc1_radius * scale / 2.0f; }

} // namespace ixion_toan

#endif
