#include <gtest/gtest.h>
#include "ixion_dc1_toan.h"
#include "ixion_toan_bridge.h"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

// Goro through the same library: GoroKey_On/Play (charakey.cpp:936-1157) selected by IxionDc1Config.character.
namespace {
IxionDc1Config goro_config() {
    IxionDc1Config c{};
    c.character = IXION_DC1_CHARA_GORO;
    // c06b base.cfg KEY rows 0-36 (37 rows); the caller supplies these, so no retail-derived asset is committed.
    const IxionDc1Motion keys[37] = {
        {10, 20, .2f},    {60, 80, .5f},    {30, 50, .5f},    {10, 20, .5f},    {200, 210, .4f},  {10, 20, .5f},
        {210, 238, .3f},  {225, 238, .23f}, {181, 194, .6f},  {140, 150, .4f},  {194, 200, .6f},  {90, 117, .4f},
        {118, 122, .3f},  {123, 127, .3f},  {453, 498, .4f},  {440, 450, .5f},  {460, 470, .5f},  {0, 0, .1f},
        {10, 20, .2f},    {335, 345, .5f},  {355, 365, .5f},  {30, 50, .5f},    {305, 325, .5f},  {240, 256, .25f},
        {256, 257, 0},    {260, 275, .3f},  {264, 264, 0},    {264, 275, .3f},  {277, 302, .2f},  {60, 80, .5f},
        {155, 165, .3f},  {421, 450, .5f},  {0, 0, 0},        {550, 560, .2f},  {515, 525, .3f},  {530, 540, .3f},
        {90, 108, .35f},
    };
    for (int i = 0; i < 37; ++i) c.motions[i] = keys[i];
    c.attack = 100; c.weapon_speed = 30; c.element = 2; c.weapon_flags = 7;
    c.action_gauge = 100;
    return c;
}

// The host's named points: dcol0 and dcol1 on the hammer, "weapon" on the body. weapon_tip is elsewhere, so a
// record that sits on it would show that the resolver was bypassed.
struct Points {
    int calls = 0;
    std::vector<std::string> names;
    std::vector<int> frames;
    bool answer = true;
};
int32_t resolve(void *user, int32_t frame, const char *name, float out[4]) {
    auto *p = static_cast<Points *>(user);
    ++p->calls;
    p->names.emplace_back(name);
    p->frames.push_back(frame);
    if (!p->answer) return 0;
    const float dcol0[4] = {30, 40, 50, 1}, dcol1[4] = {31, 41, 51, 1}, weapon[4] = {5, 6, 7, 1};
    const float *src = std::strcmp(name, "dcol0") == 0 ? dcol0 : std::strcmp(name, "dcol1") == 0 ? dcol1 : weapon;
    std::memcpy(out, src, sizeof(float) * 4);
    return 1;
}
// Installs the resolver for one test and always removes it, so the Toan suite keeps its R1 behaviour.
struct Resolver {
    Points points;
    Resolver() { ixion_dc1_set_point_resolver(resolve, &points); }
    ~Resolver() { ixion_dc1_set_point_resolver(nullptr, nullptr); }
};

IxionDc1Output tick(float time, bool down = false, bool held = false) {
    IxionDc1Input in{};
    in.time = time; in.cross_down = down; in.cross_held = held;
    in.position[0] = 70; in.position[1] = 37; in.position[2] = -4;
    in.position[3] = in.weapon_tip[3] = 1;
    in.weapon_tip[0] = 12; in.weapon_tip[1] = 8; in.weapon_tip[2] = 3;
    IxionDc1Output out{};
    ixion_dc1_tick(&in, &out);
    return out;
}

// Press, hold through the swing's 95.0-95.5 check (charakey.cpp:1063), and enter the charge loop 13.
IxionDc1Output enter_charge() {
    tick(10, true, true);
    IxionDc1Output out{};
    for (int half = 180; half <= 190; ++half) out = tick(half * .5f, false, true);
    return out;
}

// A DC2-like host clock: frames advance by the authored KEY step, restart on a new motion, loop the table's
// charge-loop KEY and hold at the end of any other KEY.
struct Run {
    std::vector<int> actions;
    std::vector<IxionDc1Hit> hits;
    std::vector<float> hit_times;
    std::vector<IxionDc1Shot> shots;
    std::vector<float> shot_times;
    std::vector<float> loop_times;
    int loop_ticks = 0, ticks = 0, invincible_ticks = 0;
    bool finished = false, invincible_at_end = true;
    float gauge_at_release = -1;
};
Run RunGoro(int hold_loop_ticks, const IxionDc1Config &c) {
    IxionDc1CharacterInfo info{};
    ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &info);
    ixion_dc1_reset(&c);
    Run run;
    float frame = 10;
    int motion = -1;
    for (; run.ticks < 800; ++run.ticks) {
        const bool press = run.ticks == 0;
        const bool held = press || (run.actions.size() <= 1 || run.loop_ticks < hold_loop_ticks);
        IxionDc1Input in{};
        in.time = frame; in.cross_down = press; in.cross_held = held;
        in.position[3] = in.weapon_tip[3] = 1;
        IxionDc1Output out{};
        ixion_dc1_tick(&in, &out);
        if (run.actions.empty() || run.actions.back() != out.action_no) {
            if (out.action_no == 14) run.gauge_at_release = out.action_gauge;
            run.actions.push_back(out.action_no);
        }
        if (out.action_no == 13) { ++run.loop_ticks; run.loop_times.push_back(frame); }
        if (out.action_no == 14 && out.invincible) ++run.invincible_ticks;
        for (int i = 0; i < out.hit_count; ++i) { run.hits.push_back(out.hits[i]); run.hit_times.push_back(frame); }
        if (out.shot.spawned) { run.shots.push_back(out.shot); run.shot_times.push_back(frame); }
        if (!out.action_on) { run.finished = true; run.invincible_at_end = out.invincible; break; }
        if (out.motion_no != motion) { motion = out.motion_no; frame = c.motions[motion].start; }
        else {
            frame += c.motions[motion].step;
            if (frame > c.motions[motion].end)
                frame = motion == info.charge_loop_key ? c.motions[motion].start : c.motions[motion].end;
        }
    }
    return run;
}
}

TEST(IxionDc1Goro, ComboSequenceSwingChargeEarlyReleaseRestartsTheSwing) {
    // Goro's "combo" is one swing (action/motion 0x24 on KEY 36): held through 95.0-95.5 it enters the charge
    // loop 13; released before 45 loop ticks it restarts the swing (charakey.cpp:1106-1118); released again
    // it ends at KEY 36's end-1..end (1069-1088). No 0x25+ follow-ups exist for him.
    auto c = goro_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    auto out = tick(10, true, true);
    EXPECT_EQ(out.character, IXION_DC1_CHARA_GORO);
    ASSERT_EQ(out.action_no, 0x24); EXPECT_EQ(out.motion_no, 36); EXPECT_EQ(out.restart, 1);
    EXPECT_FLOAT_EQ(out.time, 136); // GoroKey_On's write; the host restarts KEY 36 at 90.
    std::vector<int> chain{out.action_no};
    auto note = [&](const IxionDc1Output &o) { if (chain.back() != o.action_no) chain.push_back(o.action_no); };
    for (int half = 180; half <= 190; ++half) note(out = tick(half * .5f, false, true));
    ASSERT_EQ(out.action_no, 13);
    EXPECT_EQ(out.motion_no, 13);
    EXPECT_EQ(out.hit_count, 0);
    note(out = tick(123, false, true));
    note(out = tick(123.3f)); // released after 2 loop ticks
    EXPECT_EQ(out.action_no, 0x24);
    EXPECT_EQ(out.action_on, 1);
    int records = 0;
    for (int half = 180; half <= 216 && out.action_on; ++half) {
        note(out = tick(half * .5f));
        records += out.hit_count;
    }
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.motion_no, 0);
    EXPECT_EQ(chain, (std::vector<int>{0x24, 13, 0x24}));
    EXPECT_EQ(records, 7); // 99.0-102.0 at half frames
}

TEST(IxionDc1Goro, SwingHitWindowsFollowKey36) {
    // Every half frame of KEY 36 (90-108): one record exactly while 99 <= time <= 102 (charakey.cpp:985-989), at
    // the hammer's dcol0 (982), radius 5, life 1, mask 2, kind 2, knockback 2.5/0.1 mode 2 from Goro's position.
    // 103.0-103.4 shakes the camera 2.0/0.01/20 (1022-1026); the swing ends at 107 (end-1).
    Resolver r;
    auto c = goro_config(); ixion_dc1_reset(&c);
    auto out = tick(10, true, false);
    std::vector<float> hit_times;
    float shake_time = -1;
    for (int half = 180; half <= 216 && out.action_on; ++half) {
        const float t = half * .5f;
        out = tick(t);
        SCOPED_TRACE(t);
        const bool inside = t >= 99 && t <= 102;
        ASSERT_EQ(out.hit_count, inside ? 1 : 0);
        if (out.camera_shake != 0) {
            shake_time = t;
            EXPECT_FLOAT_EQ(out.camera_shake, 2); EXPECT_FLOAT_EQ(out.camera_shake_decay, .01f);
            EXPECT_EQ(out.camera_shake_frames, 20);
        }
        if (!inside) continue;
        hit_times.push_back(t);
        const auto &hit = out.hits[0];
        EXPECT_FLOAT_EQ(hit.position[0], 30); EXPECT_FLOAT_EQ(hit.position[1], 40); EXPECT_FLOAT_EQ(hit.position[2], 50);
        EXPECT_FLOAT_EQ(hit.radius, 5); EXPECT_EQ(hit.damage, 100); EXPECT_EQ(hit.life, 1);
        EXPECT_EQ(hit.target_mask, 2); EXPECT_EQ(hit.kind, 2);
        EXPECT_EQ(hit.element_flags, 4); EXPECT_EQ(hit.weapon_flags, 7);
        EXPECT_FLOAT_EQ(hit.knockback_speed, 2.5f); EXPECT_FLOAT_EQ(hit.knockback_decay, .1f);
        EXPECT_EQ(hit.knockback_mode, 2);
        EXPECT_FLOAT_EQ(hit.knockback_origin[0], 70); EXPECT_FLOAT_EQ(hit.knockback_origin[2], -4);
    }
    EXPECT_EQ(hit_times, (std::vector<float>{99, 99.5f, 100, 100.5f, 101, 101.5f, 102}));
    EXPECT_FLOAT_EQ(shake_time, 103);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_FLOAT_EQ(out.time, 107);
}

TEST(IxionDc1Goro, ChargeLoopHoldsAtKey13UntilFortyFiveTicksThenSmashes) {
    // The table names 13 as Goro's loop (Toan's is 14). Held, the host loops KEY 13 (123-127); before 45 ticks a
    // release restarts the swing, from 45 on it is the smash 14 (charakey.cpp:1097-1135).
    IxionDc1CharacterInfo goro{}, toan{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &goro), 1);
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_TOAN, &toan), 1);
    EXPECT_EQ(goro.charge_loop_key, 13); EXPECT_EQ(goro.release_key, 14);
    EXPECT_EQ(toan.charge_loop_key, 14);
    auto c = goro_config();
    auto run = RunGoro(120, c); // two full loops of KEY 13 at 0.3
    EXPECT_TRUE(run.finished);
    EXPECT_EQ(run.actions, (std::vector<int>{0x24, 13, 14}));
    EXPECT_EQ(run.loop_ticks, 120);
    float hi = -1e9f; int wraps = 0;
    for (size_t i = 0; i < run.loop_times.size(); ++i) {
        hi = std::max(hi, run.loop_times[i]);
        if (i > 1 && run.loop_times[i] < run.loop_times[i - 1]) ++wraps;
    }
    // The first loop tick still reads KEY 36's time (the motion switched inside that tick), then 123-127.
    EXPECT_NEAR(run.loop_times.front(), 95.25f, 1e-3f);
    EXPECT_GE(*std::min_element(run.loop_times.begin() + 1, run.loop_times.end()), 123.0f);
    EXPECT_LE(hi, 127.0f);
    EXPECT_GE(wraps, 7); // 119 ticks over a 14-sample cycle
    // Boundary: 44 loop ticks restart the swing, 45 smash.
    auto early = RunGoro(44, c);
    ASSERT_GE(early.actions.size(), 3u);
    EXPECT_EQ(early.actions[2], 0x24);
    auto just = RunGoro(45, c);
    ASSERT_GE(just.actions.size(), 3u);
    EXPECT_EQ(just.actions[2], 14);
    // The smash is invincible until its own end-1..end (1131, 1141-1150).
    EXPECT_GT(run.invincible_ticks, 100);
    EXPECT_FALSE(run.invincible_at_end);
}

TEST(IxionDc1Goro, SmashDamagePhaseAndTheShotAt482) {
    // 462 <= time <= 482 lays a record at 1.5x (charakey.cpp:991-1007); [482, 482.4) spawns the shot at
    // 1.2x the already-scaled damage (1009-1020): 180 at exactly 482.0 but 120 at any later sample.
    Resolver r;
    auto c = goro_config(); ixion_dc1_reset(&c);
    auto out = enter_charge();
    for (int i = 0; i < 46; ++i) out = tick(123, false, true);
    out = tick(124); // release after 46 loop ticks
    ASSERT_EQ(out.action_no, 14);
    int records = 0;
    for (int half = 924; half <= 964; ++half) { // 462.0 .. 482.0
        out = tick(half * .5f);
        SCOPED_TRACE(half * .5f);
        ASSERT_EQ(out.hit_count, 1);
        ++records;
        EXPECT_EQ(out.hits[0].damage, 150); EXPECT_EQ(out.hits[0].attack_no, 1);
        EXPECT_FLOAT_EQ(out.hits[0].radius, 5);
        EXPECT_EQ(out.shot.spawned, half == 964 ? 1 : 0);
    }
    EXPECT_EQ(records, 41);
    // 482.0: shot at dcol0, 180, Goro's effect; the kick-back lands on the 482.0 smash record (1016).
    EXPECT_EQ(out.shot.damage, 180);
    EXPECT_EQ(out.shot.wait, 45); EXPECT_EQ(out.shot.attribute, 4); EXPECT_EQ(out.shot.weapon_flags, 7);
    EXPECT_FLOAT_EQ(out.shot.position[0], 30); EXPECT_FLOAT_EQ(out.shot.position[2], 50);
    EXPECT_FLOAT_EQ(out.shot.vector[0], .001f); EXPECT_FLOAT_EQ(out.shot.vector[1], 1);
    EXPECT_FLOAT_EQ(out.shot.vector[2], .001f); EXPECT_FLOAT_EQ(out.shot.vector[3], 1);
    EXPECT_STREQ(out.shot.effect, "c06a_tameex"); EXPECT_FLOAT_EQ(out.shot.effect_radius, 15);
    EXPECT_FLOAT_EQ(out.hits[0].knockback_speed, 2.5f);
    EXPECT_FLOAT_EQ(out.camera_shake, 8); EXPECT_FLOAT_EQ(out.camera_shake_decay, .1f);
    EXPECT_EQ(out.camera_shake_frames, 30);
    out = tick(482.4f);
    EXPECT_EQ(out.shot.spawned, 0); EXPECT_EQ(out.hit_count, 0);
    // A later sample: no 1.5x record (time > 482), so the shot is 1.2 * 100.
    ixion_dc1_reset(&c); enter_charge();
    for (int i = 0; i < 46; ++i) tick(123, false, true);
    tick(124);
    out = tick(482.2f);
    EXPECT_EQ(out.hit_count, 0);
    ASSERT_EQ(out.shot.spawned, 1);
    EXPECT_EQ(out.shot.damage, 120);
    // The authored 0.4 step from 453 samples 481.8 then 482.2: one shot, at 120 (the 180 needs an exact 482.0).
    auto run = RunGoro(46, c);
    ASSERT_EQ(run.shots.size(), 1u);
    EXPECT_NEAR(run.shot_times[0], 482.2f, 1e-3f);
    EXPECT_EQ(run.shots[0].damage, 120);
    int smash = 0;
    for (const auto &h : run.hits) smash += h.attack_no == 1;
    EXPECT_EQ(smash, 50); // 462.2 .. 481.8
    // Stamina doubles the base before either multiplier: swing 200, smash 300, 482.0 shot 360.
    c.ailments = 8; ixion_dc1_reset(&c);
    out = tick(10, true, false);
    out = tick(100);
    ASSERT_EQ(out.hit_count, 1); EXPECT_EQ(out.hits[0].damage, 200);
    c.ailments = 8; ixion_dc1_reset(&c); enter_charge();
    for (int i = 0; i < 46; ++i) tick(123, false, true);
    tick(124);
    out = tick(482);
    EXPECT_EQ(out.hits[0].damage, 300); EXPECT_EQ(out.shot.damage, 360);
}

TEST(IxionDc1Goro, OwnerFieldAndNamedPointsAreGoros) {
    // basic_damage(0, 2) (charakey.cpp:988) and the smash's own writes (1004-1005) make every record owner 2;
    // the shot's Set passes owner 2, sub -1, source 0, phase -1 (1010). Toan's stay owner 0.
    Points log;
    ixion_dc1_set_point_resolver(resolve, &log);
    auto c = goro_config();
    auto run = RunGoro(46, c);
    ixion_dc1_set_point_resolver(nullptr, nullptr);
    // Held, the swing turns into the charge at 95.25, before its 99-102 window: only the smash lays records.
    ASSERT_EQ(run.hits.size(), 50u);
    for (const auto &h : run.hits) {
        EXPECT_EQ(h.owner, 2); EXPECT_EQ(h.attack_no, 1); EXPECT_EQ(h.damage, 150);
        EXPECT_FLOAT_EQ(h.position[0], 30); // dcol0, not weapon_tip
    }
    ASSERT_EQ(run.shots.size(), 1u);
    EXPECT_EQ(run.shots[0].owner, 2); EXPECT_EQ(run.shots[0].sub_id, -1);
    EXPECT_EQ(run.shots[0].source, 0); EXPECT_EQ(run.shots[0].initial_phase, -1);
    // Goro asks only for the hammer's dcol0, once per handler tick.
    ASSERT_FALSE(log.names.empty());
    for (size_t i = 0; i < log.names.size(); ++i) {
        EXPECT_EQ(log.names[i], "dcol0");
        EXPECT_EQ(log.frames[i], IXION_DC1_FRAME_WEAPON);
    }
    // A resolver that declines falls back to weapon_tip and says so.
    Resolver r; r.points.answer = false;
    ixion_dc1_reset(&c);
    tick(10, true, false);
    auto out = tick(100);
    ASSERT_EQ(out.hit_count, 1);
    EXPECT_EQ(out.hits[0].owner, 2); EXPECT_EQ(out.hits[0].attack_no, 0); // the plain swing
    EXPECT_FLOAT_EQ(out.hits[0].position[0], 12);
    EXPECT_EQ(out.point_queries, 1); EXPECT_EQ(out.point_fallbacks, 1);
    // Table rows and selection: the Toan wrapper always selects Toan; an unknown id is refused and runs Toan.
    IxionDc1CharacterInfo info{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &info), 1);
    EXPECT_EQ(info.hit_owner, 2); EXPECT_STREQ(info.hit_point, "dcol0"); EXPECT_STREQ(info.shot_point, "dcol0");
    EXPECT_EQ(info.donor_key_count, 3);
    EXPECT_EQ(std::vector<int>(info.donor_keys, info.donor_keys + 3), (std::vector<int>{13, 14, 36}));
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_TOAN, &info), 1);
    EXPECT_EQ(info.hit_owner, 0); EXPECT_STREQ(info.hit_point, "dcol1"); EXPECT_FLOAT_EQ(info.effect_radius, 20);
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_UNGAGA, &info), 1);
    EXPECT_EQ(info.hit_owner, 4); EXPECT_STREQ(info.shot_point, "weapon");
    // Ruby: other handlers, not in this library (Xiao R1 added Xiao, id 1, so she is no longer the refused example).
    EXPECT_EQ(ixion_dc1_character_info(3, &info), 0);
    ixion_dc1_toan_reset(&c); // character 2 in the struct, but the Toan boundary
    EXPECT_EQ(tick(10).character, IXION_DC1_CHARA_TOAN);
    auto unknown = c; unknown.character = 5;
    EXPECT_EQ(ixion_dc1_reset(&unknown), 0);
    EXPECT_EQ(tick(10).character, IXION_DC1_CHARA_TOAN);
}

TEST(IxionDc1Goro, GaugeNeedsAFullBarAndTheEarlyRestartDebitIsUnclamped) {
    // GoroKey_On needs action_gauge >= 100 and empties it (charakey.cpp:938-944). The end-of-tick upkeep refills
    // the full weapon rate, 1.0 per tick at speed 30, through the swing: the 0.25 swinging damp is Toan's and
    // Ungaga's only (gameloop.cpp:6113-6135; Ungaga R1 corrected R1's 0.25 for Goro). The early restart debits 15
    // with no clamp (1117): the raw value goes negative. The smash release empties it again (1126).
    auto c = goro_config(); c.action_gauge = 99.75f; ixion_dc1_reset(&c);
    auto out = tick(10, true, true);
    EXPECT_EQ(out.action_on, 0); // 99.75 refused; upkeep then reaches 100
    out = tick(10, true, true);
    ASSERT_EQ(out.action_on, 1);
    EXPECT_FLOAT_EQ(out.action_gauge, 0);
    float previous = out.action_gauge;
    out = tick(90, true, true); // a press during the swing is not a queue for Goro
    EXPECT_EQ(out.combo_queued, 0);
    EXPECT_FLOAT_EQ(out.action_gauge, previous + 1);
    for (int half = 181; half <= 190; ++half) { previous = out.action_gauge; out = tick(half * .5f, false, true); }
    ASSERT_EQ(out.action_no, 13);
    EXPECT_FLOAT_EQ(out.action_gauge, previous + 1);
    previous = out.action_gauge;
    out = tick(123); // released on the first loop tick
    ASSERT_EQ(out.action_no, 0x24);
    EXPECT_FLOAT_EQ(out.action_gauge, previous + 1 - 15);
    EXPECT_LT(out.action_gauge, 0);
    // A second press is refused while the gauge is below 100, even after the swing ends.
    for (int half = 180; half <= 216 && out.action_on; ++half) out = tick(half * .5f);
    ASSERT_EQ(out.action_on, 0);
    out = tick(10, true, true);
    EXPECT_EQ(out.action_on, 0);
    // The smash release empties a part-refilled gauge.
    auto run = RunGoro(46, goro_config());
    EXPECT_FLOAT_EQ(run.gauge_at_release, 0);
}

// Goro R2: the DC2 bridge (port/src/ixion_toan_combat.cpp) reads every Goro difference from the table above.
namespace {
// DC2's CCharacter2::NormalDrive (ps2/src/character.cpp:1191-1254) for one KEY, in its own float arithmetic: a
// restart puts the frame at the KEY start and the same Drive adds step * 1.2; past the end a HOLD KEY parks on
// the end (step 0), any other KEY wraps to its start. The bridge's donor tick reads the frame the last Drive left.
struct Dc2Drive {
    float frame = 10, step = 0, start = 0, end = 0;
    bool hold = true;
    void Restart(const IxionDc1Motion &key, float dc2_step, bool hold_at_end) {
        start = key.start; end = key.end; step = dc2_step; hold = hold_at_end; frame = start;
    }
    void Drive() {
        const float motion_speed = 1.2f;
        frame += step * motion_speed;
        if (!(frame + step * motion_speed <= end)) {
            if (hold) { frame = end; step = 0.0f; } else frame = start;
        }
    }
};
struct BridgeRun {
    std::vector<float> smash_samples, shot_times;
    std::vector<int> shot_damage, record_damage;
    int smash_records = 0, swing_records = 0, loop_ticks = 0;
    bool finished = false;
};
// The bridge's per-tick order for Goro: donor tick on the current frame, assert the donor's motion (restart on a
// change; HOLD unless it is the table's charge loop), then DC2 drives the motion. Cross is tapped (hold_loop < 0)
// or held through hold_loop charge-loop ticks.
BridgeRun RunGoroOnDc2Clock(const IxionDc1Config &dc1, int hold_loop) {
    IxionDc1CharacterInfo info{};
    ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &info);
    // What the body setup leaves in the DC2 key list (the donor KEYs divided by 1.2) and what Bind hands the donor.
    struct Key { float step; } keys[37];
    for (int i = 0; i < 37; ++i) keys[i].step = dc1.motions[i].step;
    ixion_toan::RescaleKeys(keys, 37, info.donor_keys, info.donor_key_count);
    IxionDc1Config c = dc1;
    for (int k = 0; k < info.donor_key_count; ++k) {
        const int i = info.donor_keys[k];
        c.motions[i].step = ixion_toan::DonorStep(keys[i].step);
    }
    ixion_dc1_reset(&c);
    Dc2Drive drive;
    int motion = -1;
    BridgeRun run;
    for (int t = 0; t < 1200; ++t) {
        const bool press = t == 0;
        const bool held = hold_loop >= 0 && (press || run.loop_ticks < hold_loop);
        IxionDc1Input in{};
        in.time = drive.frame; in.cross_down = press; in.cross_held = held;
        in.position[3] = in.weapon_tip[3] = 1;
        IxionDc1Output out{};
        ixion_dc1_tick(&in, &out);
        if (out.action_no == info.charge_loop_key && out.action_on) ++run.loop_ticks;
        if (out.action_no == info.release_key && in.time >= 480 && in.time <= 484) run.smash_samples.push_back(in.time);
        for (int i = 0; i < out.hit_count; ++i) {
            (out.hits[i].attack_no == 1 ? run.smash_records : run.swing_records)++;
            run.record_damage.push_back(out.hits[i].damage);
        }
        if (out.shot.spawned) { run.shot_times.push_back(in.time); run.shot_damage.push_back(out.shot.damage); }
        if (t > 0 && !out.action_on) { run.finished = true; break; }
        if (out.motion_no != motion) {
            motion = out.motion_no;
            drive.Restart(c.motions[motion], keys[motion].step, motion != info.charge_loop_key);
        }
        drive.Drive();
    }
    return run;
}
IxionDc1Config turtle_shell() {
    auto c = goro_config();
    c.attack = 36; c.weapon_speed = 45; c.element = 1; c.weapon_flags = 0; // itemdata.cpp:1491
    return c;
}
}

TEST(IxionDc1GoroBridge, TheTableGivesGorosRescaleListLoopKeyAndToansIsUnchanged) {
    IxionDc1CharacterInfo goro{}, toan{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &goro), 1);
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_TOAN, &toan), 1);
    // Toan's row is exactly the bridge's R5 list, so his rescale is unchanged.
    ASSERT_EQ(toan.donor_key_count, (int) (sizeof(ixion_toan::kDonorKeys) / sizeof(int)));
    for (int k = 0; k < toan.donor_key_count; ++k) EXPECT_EQ(toan.donor_keys[k], ixion_toan::kDonorKeys[k]);
    EXPECT_EQ(toan.charge_loop_key, 14);
    EXPECT_EQ(goro.charge_loop_key, 13);
    EXPECT_EQ(goro.release_key, 14);
    struct Key { float step; } keys[41];
    for (Key &k : keys) k.step = .4f;
    ixion_toan::RescaleKeys(keys, 41, goro.donor_keys, goro.donor_key_count);
    for (int i = 0; i < 41; ++i) {
        const bool donor = i == 13 || i == 14 || i == 36;
        EXPECT_NEAR(keys[i].step, donor ? .4f / 1.2f : .4f, 1e-6f) << i;
    }
    // A short key list (fewer KEYs than the table names) is left alone past its end.
    Key few[14];
    for (Key &k : few) k.step = .3f;
    ixion_toan::RescaleKeys(few, 14, goro.donor_keys, goro.donor_key_count);
    EXPECT_NEAR(few[13].step, .3f / 1.2f, 1e-6f);
}

TEST(IxionDc1GoroBridge, ShotLifeIsEachEffectsOwnPhaseZero) {
    EXPECT_EQ(ixion_toan::ShotLifeTicks("c01_fuusya"), ixion_toan::kShotLifeTicks); // 85, Toan's R5 value
    EXPECT_EQ(ixion_toan::ShotLifeTicks("c06a_tameex"), 49);                        // (19 - 2) / 0.35
    EXPECT_EQ(ixion_toan::ShotLifeTicks("c10a_ex"), 0);
    // Goro's shot (wait 45) lays two records in its 49 steps, at age 0 and 46.
    ixion_toan::BurstClock clock;
    clock.Start(45, ixion_toan::ShotLifeTicks("c06a_tameex"));
    std::vector<int> pulses;
    for (int i = 0; i < 100; ++i) if (clock.Step()) pulses.push_back(i);
    EXPECT_EQ(pulses, (std::vector<int>{0, 46}));
    EXPECT_EQ(clock.age, 49);
    EXPECT_NEAR(ixion_toan::Dc2PrimRadius(15.0f, 1.5584791f), 11.69f, 0.01f);
}

TEST(IxionDc1GoroBridge, OnDc2sRescaledClockTheSmashShotLandsAt482Point2For43) {
    // The bridge rescales KEY 14 to 0.4 / 1.2 and NormalDrive plays it at step * 1.2. From the restart at 453 the
    // donor samples 453 + k * (that step), k >= 1. One sample falls in [482, 482.4) and it is not exactly 482.0,
    // so the 1.5x branch has not run on it: the Turtle Shell's shot is (int)(1.2 * 36) = 43, never 64.
    const auto run = RunGoroOnDc2Clock(turtle_shell(), 46);
    std::string samples;
    for (float s : run.smash_samples) samples += std::to_string(s) + " ";
    SCOPED_TRACE(samples);
    ASSERT_TRUE(run.finished);
    ASSERT_EQ(run.shot_times.size(), 1u);
    EXPECT_NEAR(run.shot_times[0], 482.2f, 2e-3f);
    EXPECT_NE(run.shot_times[0], 482.0f);
    EXPECT_EQ(run.shot_damage[0], 43);
    EXPECT_EQ(run.swing_records, 0); // held through 95.0-95.5: no plain swing hit
    EXPECT_EQ(run.smash_records, 50);
    for (int d : run.record_damage) EXPECT_EQ(d, 54); // (int)(36 * 1.5)
    EXPECT_GE(run.smash_samples.size(), 9u);
    // A tap swings once: the 99-102 window lays the plain hit at the Turtle Shell's 36.
    const auto tap = RunGoroOnDc2Clock(turtle_shell(), -1);
    ASSERT_TRUE(tap.finished);
    EXPECT_GT(tap.swing_records, 0);
    EXPECT_EQ(tap.smash_records, 0);
    for (int d : tap.record_damage) EXPECT_EQ(d, 36);
    EXPECT_TRUE(tap.shot_times.empty());
}
