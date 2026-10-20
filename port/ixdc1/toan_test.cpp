#include <gtest/gtest.h>
#include "ixion_dc1_toan.h"
#include "ixion_dc1_toan_motion.h"
#include "ixion_toan_bridge.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace {
IxionDc1ToanConfig config() {
    IxionDc1ToanConfig c{};
    // c01d dungeon KEYs 0,3,14-17,24-25,36-40; caller supplies these,
    // so no retail-derived asset is committed or required by this test.
    c.motions[0] = {10, 20, .2f}; c.motions[3] = {191, 196, .2f};
    c.motions[14] = {160, 170, .4f}; c.motions[15] = {180, 191, .4f};
    c.motions[16] = {191, 192, 0}; c.motions[17] = {196, 206, .4f};
    c.motions[24] = {715, 750, .4f}; c.motions[25] = {196, 196, 0};
    c.motions[36] = {820, 830, .3f}; c.motions[37] = {830, 838, .3f};
    c.motions[38] = {838, 847, .3f}; c.motions[39] = {847, 857, .3f};
    c.motions[40] = {856, 884, .3f};
    c.attack = 100; c.weapon_speed = 30; c.element = 2; c.weapon_flags = 7;
    c.action_gauge = 100;
    return c;
}
IxionDc1ToanOutput tick(float time, bool down = false, bool held = false) {
    IxionDc1ToanInput in{};
    in.time = time; in.cross_down = down; in.cross_held = held;
    in.position[3] = in.weapon_tip[3] = 1;
    in.weapon_tip[0] = 12; in.weapon_tip[1] = 8; in.weapon_tip[2] = 3;
    IxionDc1ToanOutput out{};
    ixion_dc1_toan_tick(&in, &out);
    return out;
}
}

TEST(IxionDc1Toan, FiveHitComboUsesRetailActionsWindowsDamageAndCosts) {
    auto c = config(); ixion_dc1_toan_reset(&c);
    auto out = tick(10, true, true);
    ASSERT_EQ(out.motion_no, 36); ASSERT_EQ(out.restart, 1);
    EXPECT_FLOAT_EQ(out.time, 136); // Donor On write, then host restarts KEY 36.
    const std::array<int, 4> presses{824, 832, 841, 850};
    const std::array<int, 5> first{825, 833, 842, 851, 862};
    const std::array<int, 5> last{828, 835, 844, 854, 866};
    const std::array<float, 5> radius{2.8f, 5.3f, 6.2f, 6.2f, 6.2f};
    const std::array<float, 5> kick{1.2f, 1.2f, 1.5f, 2, 3};
    std::array<int, 5> records{};
    std::vector<int> chain{36};
    float previous_gauge = 100;
    int previous_action = 36;
    for (int half = 1640; half <= 1768; ++half) {
        const float frame = half * .5f;
        bool down = false;
        for (int press : presses) down |= frame == press;
        out = tick(frame, down, down);
        SCOPED_TRACE(frame);
        int window = -1;
        for (int i = 0; i < 5; ++i) if (frame >= first[i] && frame <= last[i]) window = i;
        ASSERT_EQ(out.hit_count, window >= 0 ? 1 : 0);
        if (window >= 0) {
            ++records[window]; const auto &hit = out.hits[0];
            EXPECT_EQ(out.action_no, 36 + window);
            EXPECT_FLOAT_EQ(hit.radius, radius[window]);
            EXPECT_EQ(hit.damage, window == 4 ? 180 : 100);
            EXPECT_EQ(hit.life, 1); EXPECT_EQ(hit.target_mask, 2); EXPECT_EQ(hit.kind, 2);
            EXPECT_EQ(hit.element_flags, 4); EXPECT_EQ(hit.weapon_flags, 7);
            EXPECT_EQ(hit.owner, 0); EXPECT_EQ(hit.attack_no, 0);
            EXPECT_FLOAT_EQ(hit.position[0], 12); EXPECT_FLOAT_EQ(hit.position[1], 8);
            EXPECT_FLOAT_EQ(hit.knockback_speed, kick[window]); EXPECT_EQ(hit.knockback_mode, 2);
        }
        if (out.action_no != previous_action) {
            chain.push_back(out.action_no);
            // First swing can refill .25 per tick; follow-ups stop refilling.
            const float gain = previous_action == 36 && previous_gauge < 100 ? .25f : 0;
            EXPECT_FLOAT_EQ(out.action_gauge, std::min(100.0f, previous_gauge + gain) - 25);
        }
        previous_gauge = out.action_gauge; previous_action = out.action_no;
    }
    EXPECT_EQ(chain, (std::vector<int>{36, 37, 38, 39, 40}));
    EXPECT_EQ(records, (std::array<int, 5>{7, 5, 5, 7, 9}));
    EXPECT_EQ(out.action_on, 0); EXPECT_EQ(out.motion_no, 0);
    RecordProperty("windows", "825-828,833-835,842-844,851-854,862-866");
}

TEST(IxionDc1Toan, AuthoredPointThreeStepCompletesFiveMotions) {
    auto c = config(); ixion_dc1_toan_reset(&c);
    auto out = tick(10, true, true);
    std::vector<int> chain{out.motion_no};
    std::array<int, 5> records{};
    bool queued = false;
    float frame = 820;
    for (int i = 0; i < 400 && out.action_on; ++i) {
        bool down = !queued && out.combo_window && out.action_no < 40;
        if (down) queued = true;
        const int old = out.motion_no;
        out = tick(frame, down, down);
        if (out.hit_count) ++records[out.action_no - 36];
        if (out.motion_no && out.motion_no != old) {
            chain.push_back(out.motion_no); queued = false;
            frame = c.motions[out.motion_no].start;
        } else frame += .3f;
    }
    EXPECT_EQ(chain, (std::vector<int>{36,37,38,39,40}));
    EXPECT_EQ(out.action_on, 0);
    for (int n : records) EXPECT_GT(n, 0);
}

TEST(IxionDc1Toan, HeldCrossEntersRealChargeAndReleaseStartsJumpAttack) {
    auto c = config(); ixion_dc1_toan_reset(&c);
    tick(10, true, true);
    auto out = tick(829, false, true);
    ASSERT_EQ(out.action_no, 14); EXPECT_EQ(out.motion_no, 14); EXPECT_FLOAT_EQ(out.action_gauge, 0);
    for (int i = 0; i < 31; ++i) out = tick(165, false, true);
    EXPECT_EQ(out.charge_level, 1);
    out = tick(165);
    EXPECT_EQ(out.action_no, 15); EXPECT_EQ(out.motion_no, 15); EXPECT_EQ(out.restart, 1);
}

TEST(IxionDc1Toan, QueueRequiresTwentyGaugeAndDebitClampsAtZero) {
    for (float gauge : {19.999f, 20.0f}) {
        // Two setup ticks each refill .25, so the press observes the exact
        // threshold (or just below it), rather than a replenished 20.5.
        auto c = config(); c.action_gauge = gauge - .5f; ixion_dc1_toan_reset(&c);
        tick(10, true, true); tick(820);
        auto out = tick(821, true, true);
        EXPECT_EQ(out.combo_queued, gauge == 20 ? 1 : 0);
        out = tick(829);
        EXPECT_EQ(out.action_no, gauge == 20 ? 37 : 36);
        if (gauge == 20) EXPECT_FLOAT_EQ(out.action_gauge, 0);
        else { EXPECT_EQ(out.action_on, 0); EXPECT_EQ(out.cooldown, 15); }
    }
}

TEST(IxionDc1Toan, CancelRetiresHitsAndMovementAndResetClearsState) {
    auto c = config(); ixion_dc1_toan_reset(&c); tick(10, true, true);
    ASSERT_EQ(tick(825).hit_count, 1);
    IxionDc1ToanInput in{}; in.time = 826; in.cancel = 1;
    IxionDc1ToanOutput out{}; ixion_dc1_toan_tick(&in, &out);
    EXPECT_EQ(out.hit_count, 0); EXPECT_EQ(out.action_on, 0); EXPECT_FLOAT_EQ(out.velocity[2], 0);
    ixion_dc1_toan_reset(&c); out = tick(10);
    EXPECT_EQ(out.action_on, 0); EXPECT_EQ(out.combo_queued, 0); EXPECT_FLOAT_EQ(out.action_gauge, 100);
}

TEST(IxionDc1Toan, StaminaDoublesDonorDamageAndYawTurnsPush) {
    auto c = config(); c.ailments = 8; // AILMENT_STAMINA in DC1.
    ixion_dc1_toan_reset(&c); tick(10, true, true);
    IxionDc1ToanInput in{}; in.time = 820; in.yaw = 1.57079632679f;
    IxionDc1ToanOutput out{}; ixion_dc1_toan_tick(&in, &out);
    EXPECT_NEAR(out.velocity[0], .17f, 1e-6f); EXPECT_NEAR(out.velocity[2], 0, 1e-6f);
    out = tick(825); ASSERT_EQ(out.hit_count, 1); EXPECT_EQ(out.hits[0].damage, 200);
}

namespace {
// A DC2-like host for the charge/jump chain: animation frames advance by the KEY step (restart on a new
// motion, hold at the end except the charge loop 14), and the body moves on a floor at Y=37 with the
// bridge's vertical rule and ground height. R3 fed world Y as the ground height.
struct JumpRun {
    std::vector<int> actions;
    float            apex = 0;
    int              slam_hits = 0, slam_damage = 0, ticks = 0, end_action = -1;
    bool             finished = false, guard_fired = false;
    int              shots = 0, shot_tick = -1;
    IxionDc1ToanShot shot{};
};
JumpRun RunChargeJump(bool world_y_ground, int hold_ticks, int skill = 0, int attack = 100, int element = 2) {
    constexpr float kFloor = 37, kScale = 1.5584791f;
    auto c = config(); c.charged_skill = skill; c.attack = attack; c.element = element; ixion_dc1_toan_reset(&c);
    ixion_toan::LandingGuard guard;
    JumpRun run;
    float frame = 10, y = kFloor, vy = 0;
    bool landed = true, found = true;
    int motion = -1, previous_action = 0, charge_ticks = 0;
    for (; run.ticks < 600; ++run.ticks) {
        IxionDc1ToanInput in{};
        in.time = frame; in.position[0] = 70; in.position[1] = y; in.position[2] = -4;
        in.position[3] = in.weapon_tip[3] = 1;
        in.weapon_tip[1] = y + 8;
        const bool press = run.ticks == 0;
        const bool held = press || charge_ticks < hold_ticks;
        in.cross_down = press; in.cross_held = held;
        const float height = world_y_ground ? y : ixion_toan::GroundHeight(landed, found, y, kFloor, kScale);
        in.ground_height = world_y_ground ? height : guard.Filter(previous_action, height);
        IxionDc1ToanOutput out{}; ixion_dc1_toan_tick(&in, &out);
        if (out.action_no == 14) ++charge_ticks;
        if (out.shot.spawned) { ++run.shots; run.shot = out.shot; run.shot_tick = run.ticks; }
        if (run.actions.empty() || run.actions.back() != out.action_no) run.actions.push_back(out.action_no);
        if ((out.action_no == 3 || out.action_no == 0x18) && out.hit_count) {
            ++run.slam_hits; run.slam_damage = out.hits[0].damage;
        }
        // Vertical: the donor's step while jumping, else DC2 gravity (-0.5, terminal -3.5).
        vy = out.jumping ? ixion_toan::VerticalStep(out.velocity[1], kScale) : std::max(vy - .5f, -3.5f);
        y += vy;
        landed = y <= kFloor;
        if (landed) { y = kFloor; vy = 0; }
        found = y - kFloor < 20;
        run.apex = std::max(run.apex, y - kFloor);
        previous_action = out.action_no;
        if (!out.action_on) { run.finished = true; run.end_action = out.action_no; break; }
        if (out.motion_no != motion) { motion = out.motion_no; frame = c.motions[motion].start; }
        else {
            frame += c.motions[motion].step;
            if (frame > c.motions[motion].end) frame = motion == 14 ? c.motions[motion].start : c.motions[motion].end;
        }
    }
    run.guard_fired = guard.fired;
    return run;
}
}

TEST(IxionDc1Toan, ChargeReleaseLeapsSlamsAndLandsOnARaisedFloor) {
    // Held through swing 1 then 31 charge ticks (charge_time 1.0 + 31/60 > 1.5), no event skill: Dark Cloud's
    // jump attack. 0x24 -> 14 charge -> 0xF leap -> 0x10 air -> 3 slam -> (0x19 fall) -> 0x11 land -> idle.
    auto run = RunChargeJump(false, 31);
    EXPECT_TRUE(run.finished);
    EXPECT_FALSE(run.guard_fired);
    // Toan is already down when the slam ends, so 0x19 sees ground_height < 2 on its first tick and hands
    // over to 0x11 inside the same handler call (charakey.cpp:572-580): 0x19 never shows as a tick's action.
    EXPECT_EQ(run.actions, (std::vector<int>{0x24, 14, 0xF, 0x10, 3, 0x11}));
    // +2.0 then -0.1 per tick: 2 + 1.9 + ... + 0.1 = 21 DC1 units, times the room scale. Retail relaunches on
    // every tick with time in [187, 188) (charakey.cpp:517), so at step 0.4 the first 2-3 ticks each restart
    // at +2.0 and the apex is 23-25 DC1 units; this host's frame phase gives three launches (25).
    RecordProperty("apex_dc1_units", std::to_string(run.apex / 1.5584791f));
    EXPECT_GE(run.apex, 21.0f * 1.5584791f);
    EXPECT_LE(run.apex, 25.01f * 1.5584791f);
    EXPECT_GT(run.slam_hits, 0);
    EXPECT_EQ(run.slam_damage, 150); // attack 100 * 1.5 (charakey.cpp:153-155).
    EXPECT_LT(run.ticks, 200);
}

TEST(IxionDc1Toan, WorldYGroundHeightStrandsToanInTheFallAction) {
    // The R3 bug, kept as a regression witness: on a floor at Y=37, world Y never drops below 2.
    auto run = RunChargeJump(true, 31);
    EXPECT_FALSE(run.finished);
    EXPECT_EQ(run.actions.back(), 0x19);
}

TEST(IxionDc1Toan, ShortHoldCancelsWithoutLeaping) {
    // Released well before charge_time 1.5 (28 increments): Dark Cloud drops the charge, no jump.
    auto run = RunChargeJump(false, 27);
    EXPECT_TRUE(run.finished);
    EXPECT_EQ(run.actions, (std::vector<int>{0x24, 14}));
    EXPECT_FLOAT_EQ(run.apex, 0);
}

TEST(IxionDc1Toan, GroundHeightAndLandingGuard) {
    EXPECT_FLOAT_EQ(ixion_toan::GroundHeight(true, true, 90, 37, 1.5f), 0);
    EXPECT_FLOAT_EQ(ixion_toan::GroundHeight(false, true, 40, 37, 1.5f), 2);
    EXPECT_FLOAT_EQ(ixion_toan::GroundHeight(false, false, 40, 37, 1.5f), ixion_toan::kNoGround);
    EXPECT_FLOAT_EQ(ixion_toan::GroundHeight(false, true, 30, 37, 1.5f), 0);
    ixion_toan::LandingGuard guard;
    for (int i = 0; i < ixion_toan::kLandingGuardTicks; ++i) EXPECT_FLOAT_EQ(guard.Filter(0x19, 50), 50);
    EXPECT_FLOAT_EQ(guard.Filter(0x19, 50), 0);
    EXPECT_TRUE(guard.fired);
    EXPECT_FLOAT_EQ(guard.Filter(3, 50), 50);
}

TEST(IxionDc1Toan, EventSkillLongChargeTakesTheShotActionInstead) {
    // With skill_owned[0] (set only by the story script, editloop3.cpp:7419) a hold past 2.5 releases the
    // charged shot action 0x18; its projectile is stubbed here, but the action's own melee hit (720-722) runs.
    auto run = RunChargeJump(false, 95, 1);
    EXPECT_TRUE(run.finished);
    EXPECT_EQ(run.actions, (std::vector<int>{0x24, 14, 0x18}));
    EXPECT_GT(run.slam_hits, 0);
    EXPECT_EQ(run.slam_damage, 150);
    EXPECT_FLOAT_EQ(run.apex, 0);
}

// ---- Round 5: bridge rules (engine-free) and the charged shot ----

namespace {
struct FakeHit {
    int   slot;
    float radius;
};
}

TEST(IxionToanBridge, SlotWindowsKeepEveryRecordAndCloseAbsentOnes) {
    // Two records in one tick (DC1's two-handed swings, charakey.cpp:664-686) get two windows; R4 kept only the
    // last record. An absent slot closes its own window; cancel closes all.
    ixion_toan::SlotWindows<8> table;
    std::vector<std::string> log;
    auto open = [&](int s, const FakeHit &) { log.push_back("open" + std::to_string(s)); };
    auto move = [&](int s, const FakeHit &) { log.push_back("move" + std::to_string(s)); };
    auto close = [&](int s, const char *why) { log.push_back("close" + std::to_string(s) + ":" + why); };
    const FakeHit both[2] = {{0, 6}, {1, 6}};
    table.Apply(both, 2, 36, false, open, move, close);
    EXPECT_EQ(table.OpenCount(), 2);
    table.Apply(both, 2, 36, false, open, move, close);
    const FakeHit right[1] = {{1, 6}};
    table.Apply(right, 1, 36, false, open, move, close);
    EXPECT_EQ(table.OpenCount(), 1);
    const FakeHit wider[1] = {{1, 12}};
    table.Apply(wider, 1, 36, false, open, move, close); // radius change: a new window on the same slot
    const FakeHit stray[1] = {{9, 6}};
    table.Apply(stray, 1, 36, false, open, move, close); // out-of-table slot: counted; slot 1 closes
    EXPECT_EQ(table.dropped, 1);
    table.Apply(both, 2, 37, false, open, move, close);
    table.Apply(both, 2, 37, true, open, move, close);
    EXPECT_EQ(table.OpenCount(), 0);
    EXPECT_EQ(log, (std::vector<std::string>{"open0", "open1", "move0", "move1", "move1", "close0:end", "close1:next",
                                             "open1", "close1:end", "open0", "open1", "close0:cancel",
                                             "close1:cancel"}));
    EXPECT_EQ(table.opened, 5);
}

TEST(IxionToanBridge, ReloadAtTheSameAddressRescalesOnceAndRebinds) {
    // P1: Max -> Ridepod -> Max reloads the key list into the same scene slot. Each load rescales its own fresh
    // list once, so Bind reads Dark Cloud's step every time (R4's pointer guard read a fresh list as done).
    struct Key {
        float step;
    };
    static Key keys[41];
    for (int load = 0; load < 2; ++load) {
        for (Key &k : keys) k.step = .4f; // LoadPack: a fresh list from the pack, same address.
        ixion_toan::RescaleDonorKeys(keys, 41);
        for (int i : ixion_toan::kDonorKeys) {
            EXPECT_NEAR(keys[i].step, .4f / 1.2f, 1e-6f) << i;
            EXPECT_NEAR(ixion_toan::DonorStep(keys[i].step), .4f, 1e-6f) << i;
        }
        EXPECT_FLOAT_EQ(keys[0].step, .4f); // Max's own KEYs are untouched.
    }
    int body = 0;
    EXPECT_FALSE(ixion_toan::NeedsBind(&body, 3, &body, 3));
    EXPECT_TRUE(ixion_toan::NeedsBind(&body, 3, &body, 4)); // same pointer, set up again
    EXPECT_TRUE(ixion_toan::NeedsBind(nullptr, 3, &body, 3));
}

TEST(IxionToanBridge, DeathResetRestoresTheGaugeAndClosesTheWindow) {
    // P2: DC2's DEAD request runs the bridge's dead path (close every window, reset with the bound config). The
    // donor's cancel branch alone keeps the spent gauge, as an ordinary hit should.
    auto c = config(); ixion_dc1_toan_reset(&c);
    tick(10, true, true);
    auto out = tick(825, false, true);
    ASSERT_EQ(out.hit_count, 1);
    ixion_toan::SlotWindows<8> table;
    int closes = 0;
    auto none = [](int, const IxionDc1ToanHit &) {};
    table.Apply(out.hits, out.hit_count, out.action_no, false, none, none, [&](int, const char *) { ++closes; });
    out = tick(829, false, true); // into the charge: gauge spent
    ASSERT_EQ(out.action_no, 14);
    EXPECT_FLOAT_EQ(out.action_gauge, 0);
    out = tick(165, false, true);
    // Ordinary damage: cancel keeps the spent gauge.
    IxionDc1ToanInput in{}; in.time = 165; in.cancel = 1;
    ixion_dc1_toan_tick(&in, &out);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_LT(out.action_gauge, 1.0f);
    // Death: the bridge's dead path.
    std::string why;
    table.CloseAll("dead", [&](int, const char *w) { why = w; ++closes; });
    ixion_dc1_toan_reset(&c);
    for (int i = 0; i < 40; ++i) out = tick(165); // released: no leap, no hit
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.jumping, 0);
    EXPECT_EQ(out.hit_count, 0);
    EXPECT_FLOAT_EQ(out.action_gauge, 100);
    EXPECT_EQ(table.OpenCount(), 0);
    EXPECT_EQ(closes, 1);
    EXPECT_EQ(why, "dead");
}

TEST(IxionToanBridge, CrossGoesBackToTheFieldOnlyWhenTheDonorDidNotTakeIt) {
    struct Pad {
        struct {
            int value;
        } btn[0x70];
    } pad{};
    for (int slot : ixion_toan::kCrossSlots) pad.btn[slot].value = 1;
    pad.btn[0x33].value = 1; // Square stays with DC2
    ixion_toan::CrossTake take;
    take.Take(pad);
    for (int slot : ixion_toan::kCrossSlots) EXPECT_EQ(pad.btn[slot].value, 0) << slot;
    EXPECT_EQ(pad.btn[0x33].value, 1);
    // The press swung the Dagger: dng_main.cpp:2904 reads PAD_BTN_CONFIRM 0 for the rest of the frame.
    EXPECT_FALSE(take.Settle(pad, true));
    EXPECT_EQ(pad.btn[0x00].value, 0);
    EXPECT_EQ(pad.btn[0x10].value, 0);
    // The handler refused the press (cooldown, no action): the field prompt sees Cross again.
    EXPECT_TRUE(take.Settle(pad, false));
    for (int slot : ixion_toan::kCrossSlots) EXPECT_EQ(pad.btn[slot].value, 1) << slot;
}

TEST(IxionToanBridge, ChargedShotIsOneStationaryBurstAtToansPositionWithDaggerDamage) {
    // Skill on, Dagger (attack 6, no element 5): one NowMainEffect->Set at CharaMain.pos, damage (int)(1.5 * 6) = 9,
    // wait 45, attribute GetWeaponElementAttr(5) = 0 (charakey.cpp:462-475); then action 0x18, no leap.
    auto run = RunChargeJump(false, 95, 1, 6, 5);
    EXPECT_EQ(run.actions, (std::vector<int>{0x24, 14, 0x18}));
    EXPECT_EQ(run.shots, 1);
    EXPECT_EQ(run.shot.damage, 9);
    EXPECT_EQ(run.shot.wait, 45);
    EXPECT_EQ(run.shot.attribute, 0);
    EXPECT_FLOAT_EQ(run.shot.position[0], 70);
    EXPECT_FLOAT_EQ(run.shot.position[1], 37);
    EXPECT_FLOAT_EQ(run.shot.position[2], -4);
    EXPECT_FLOAT_EQ(run.apex, 0);
    // Skill off: the same hold is the jump attack and lays no shot.
    auto leap = RunChargeJump(false, 95, 0, 6, 5);
    EXPECT_EQ(leap.shots, 0);
    ASSERT_GE(leap.actions.size(), 3u);
    EXPECT_EQ(leap.actions[2], 0xF);
}

TEST(IxionToanBridge, BurstClockLaysARecordEveryFortySixStepsUntilTheMotionEnds) {
    ixion_toan::BurstClock clock;
    clock.Start(45);
    std::vector<int> pulses;
    for (int i = 0; i < 200; ++i) {
        if (clock.Step()) pulses.push_back(i);
    }
    EXPECT_EQ(pulses, (std::vector<int>{0, 46}));
    EXPECT_EQ(clock.age, ixion_toan::kShotLifeTicks);
    EXPECT_FALSE(clock.live);
    EXPECT_NEAR(ixion_toan::Dc2PrimRadius(ixion_toan::kShotRadius, 1.5584791f), 15.58f, 0.01f);
}
