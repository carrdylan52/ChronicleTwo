#include <gtest/gtest.h>
#include "ixion_dc1_toan.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

// Ungaga R1: UngagaKey_On/Play (charakey.cpp:604-934) through the same library, and the main-effect runner that
// plays his c10a_ex shot as DC1's CSHOT_EFFECT does (shot_effect.cpp:126-479).
namespace {
IxionDc1Config ungaga_config(bool runner = true) {
    IxionDc1Config c{};
    c.character = IXION_DC1_CHARA_UNGAGA;
    // c10b base.cfg KEY rows 0-39 (40 rows); the caller supplies these, so no retail-derived asset is committed.
    const IxionDc1Motion keys[40] = {
        {10, 20, .15f},   {60, 80, .55f},   {30, 50, .27f},   {30, 50, .27f},   {285, 290, .2f},  {285, 290, .2f},
        {295, 334, .3f},  {315, 334, .3f},  {210, 215, .3f},  {215, 225, .2f},  {225, 230, .3f},  {103, 121, .4f},
        {122, 141, .33f}, {155, 160, .2f},  {160, 170, .7f},  {171, 175, .15f}, {160, 170, .7f},  {0, 0, 0},
        {90, 100, .15f},  {340, 360, .3f},  {370, 390, .3f},  {400, 420, .3f},  {430, 450, .3f},  {463, 480, .2f},
        {463, 480, .2f},  {180, 206, .25f}, {186, 186, 0},    {186, 206, .22f}, {590, 625, .25f}, {490, 530, .22f},
        {490, 530, .22f}, {490, 530, .22f}, {490, 530, .22f}, {238, 248, .22f}, {630, 640, .3f},  {643, 653, .3f},
        {670, 714, .3f},  {670, 678, .28f}, {678, 694, .32f}, {695, 714, .32f},
    };
    for (int i = 0; i < 40; ++i) c.motions[i] = keys[i];
    // Fighting Stick (itemdata.cpp:1520): attack 35, speed 65, elements {0, 0, 0, 5, 0}: wind (3) is its best.
    c.attack = 35; c.weapon_speed = 65; c.element = 3; c.weapon_flags = 0;
    c.action_gauge = 100;
    if (runner) {
        // c10a_ex.chr cfg: KEY 5, 25, 0.6 / 30, 50, 0.8 / 55, 70, 0.6 (MyEntryEffect_UNGAGA motion {0, 1, 2, 2}).
        c.effect_keys[0] = {5, 25, .6f};
        c.effect_keys[1] = {30, 50, .8f};
        c.effect_keys[2] = {55, 70, .6f};
        c.effect_key_count = 3;
        c.effect_scale = 1;
    }
    return c;
}

// Named points: the stick's dcol0/dcol1 and the body's ef00. "weapon" (the DC1 name) is declined, as the converted
// c10b body only has ef00, so the library must take the row's alias.
struct Points {
    std::vector<std::string> names;
    bool answer_weapon = false, answer_ef00 = true;
};
int32_t resolve(void *user, int32_t frame, const char *name, float out[4]) {
    auto *p = static_cast<Points *>(user);
    p->names.emplace_back(std::string(frame == IXION_DC1_FRAME_BODY ? "body:" : "weapon:") + name);
    const float dcol0[4] = {30, 40, 50, 1}, dcol1[4] = {31, 41, 51, 1}, ef00[4] = {5, 6, 7, 1};
    const float *src = nullptr;
    if (frame == IXION_DC1_FRAME_WEAPON && std::strcmp(name, "dcol0") == 0) src = dcol0;
    if (frame == IXION_DC1_FRAME_WEAPON && std::strcmp(name, "dcol1") == 0) src = dcol1;
    if (frame == IXION_DC1_FRAME_BODY && std::strcmp(name, "ef00") == 0 && p->answer_ef00) src = ef00;
    if (frame == IXION_DC1_FRAME_BODY && std::strcmp(name, "weapon") == 0 && p->answer_weapon) src = ef00;
    if (src == nullptr) return 0;
    std::memcpy(out, src, sizeof(float) * 4);
    return 1;
}
struct Resolver {
    Points points;
    Resolver() { ixion_dc1_set_point_resolver(resolve, &points); }
    ~Resolver() { ixion_dc1_set_point_resolver(nullptr, nullptr); }
};

// The host's checkCollision: answers `result` (at `contact`) on the listed ages of the given serial, else NONE.
struct Collider {
    int calls = 0, target = -1;
    float radius = -1;
    int result = IXION_DC1_SHOT_HIT_NONE;
    std::vector<int> on_calls; // 1-based call numbers that collide
    float contact[4] = {100, 200, 300, 1};
};
int32_t collide(void *user, const float *, const float *, int32_t target, float radius, float hit_out[4]) {
    auto *c = static_cast<Collider *>(user);
    ++c->calls;
    c->target = target;
    c->radius = radius;
    for (int n : c->on_calls) {
        if (n == c->calls) {
            std::memcpy(hit_out, c->contact, sizeof(c->contact));
            return c->result;
        }
    }
    return IXION_DC1_SHOT_HIT_NONE;
}
struct ColliderScope {
    Collider c;
    ColliderScope() { ixion_dc1_set_shot_collider(collide, &c); }
    ~ColliderScope() { ixion_dc1_set_shot_collider(nullptr, nullptr); }
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

// A host clock at the authored KEY steps: restart on a new motion, loop the table's charge-loop KEY (14), hold the
// end of any other KEY.
struct Host {
    IxionDc1Config c;
    IxionDc1CharacterInfo info{};
    float frame = 10;
    int motion = -1;
    explicit Host(const IxionDc1Config &config) : c(config) {
        ixion_dc1_character_info(IXION_DC1_CHARA_UNGAGA, &info);
        ixion_dc1_reset(&c);
    }
    IxionDc1Output Step(bool down, bool held) {
        IxionDc1Output out = tick(frame, down, held);
        if (!out.action_on) {
            motion = -1;
        } else if (out.motion_no != motion) {
            motion = out.motion_no;
            frame = c.motions[motion].start;
        } else {
            frame += c.motions[motion].step;
            if (frame > c.motions[motion].end)
                frame = motion == info.charge_loop_key ? c.motions[motion].start : c.motions[motion].end;
        }
        return out;
    }
};

// Press and hold through swing 1's end into 13 and on into the loop 14.
void EnterLoop(Host &h, IxionDc1Output &out, int max_ticks = 200) {
    out = h.Step(true, true);
    for (int t = 0; t < max_ticks && out.action_no != 14; ++t) out = h.Step(false, true);
}
}

TEST(IxionDc1Ungaga, ComboSwingsThreeTimesOnQueuedPressesAndHitsInsideEachWindow) {
    // On starts 0x25 (charakey.cpp:613-628). A press while combo_window is set and the gauge is at least 40 queues
    // (630-638); swing 1's end-1..end then moves to 0x26 for 20 (738-752), swing 2's end-0.5..end to 0x27 (791-804),
    // and 0x27 ends the action at its end-1..end (835-845). Two records per tick, at dcol0 and dcol1, exactly while
    // 674-677, 686-692 and 700-704 (664-689): inside KEY 37 (670-678), 38 (678-694) and 39 (695-714).
    Resolver r;
    auto c = ungaga_config(false);
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    auto out = tick(10, true, false);
    EXPECT_EQ(out.character, IXION_DC1_CHARA_UNGAGA);
    ASSERT_EQ(out.action_no, 0x25); EXPECT_EQ(out.motion_no, 37); EXPECT_EQ(out.restart, 1);
    std::vector<int> chain{out.action_no};
    std::vector<float> gauges{out.action_gauge};
    struct Window { float lo, hi, knockback; int key; };
    const Window windows[3] = {{674, 677, 1.2f}, {686, 692, 1.5f}, {700, 704, 2.0f}};
    int records = 0;
    for (int swing = 0; swing < 3; ++swing) {
        const IxionDc1Motion key = c.motions[37 + swing];
        for (int half = static_cast<int>(key.start * 2); half <= static_cast<int>(key.end * 2) && out.action_on; ++half) {
            const float t = half * .5f;
            // One fresh press a tick after the swing starts queues the next swing (none after the third).
            const bool press = swing < 2 && half == static_cast<int>(key.start * 2) + 2;
            out = tick(t, press, false);
            SCOPED_TRACE(t);
            if (chain.back() != out.action_no) {
                chain.push_back(out.action_no);
                gauges.push_back(out.action_gauge);
            }
            const Window &w = windows[swing];
            const bool inside = t >= w.lo && t <= w.hi;
            ASSERT_EQ(out.hit_count, inside ? 2 : 0);
            if (!inside) continue;
            records += out.hit_count;
            for (int i = 0; i < 2; ++i) {
                const auto &hit = out.hits[i];
                EXPECT_FLOAT_EQ(hit.position[0], i == 0 ? 30 : 31); // dcol0 then dcol1, never weapon_tip
                EXPECT_FLOAT_EQ(hit.radius, 6); EXPECT_EQ(hit.damage, 35); EXPECT_EQ(hit.life, 1);
                EXPECT_EQ(hit.owner, 4); EXPECT_EQ(hit.target_mask, 2); EXPECT_EQ(hit.kind, 2);
                EXPECT_FLOAT_EQ(hit.knockback_speed, w.knockback);
                EXPECT_FLOAT_EQ(hit.knockback_origin[0], 70);
            }
            // Every window lies inside its own KEY, so a host that plays the KEYs reaches all of it.
            EXPECT_GT(t, key.start); EXPECT_LT(t, key.end);
        }
    }
    EXPECT_EQ(chain, (std::vector<int>{0x25, 0x26, 0x27}));
    EXPECT_EQ(records, 2 * (7 + 13 + 9)); // half frames: 674-677, 686-692, 700-704
    EXPECT_EQ(out.action_on, 0);
    // Each queued link cost 20 from a full bar; the swings themselves refill nothing (swinging, 6113-6135).
    ASSERT_EQ(gauges.size(), 3u);
    // 100 does not refill ((int)gauge < 100 fails). After the first link only the two ticks at exactly 678 (the
    // end of KEY 37 and the start of KEY 38, strictly inside neither) refill 0.25 * 65 / 30 each.
    const float gain = 65.0f / 30.0f * .25f;
    EXPECT_FLOAT_EQ(gauges[1], 80);
    EXPECT_NEAR(gauges[2], 60 + 2 * gain, 1e-4f);
    // No queue below 40: from a 30 gauge the press during swing 1 is ignored and the action ends after it.
    c.action_gauge = 30; ixion_dc1_reset(&c);
    out = tick(10, true, false);
    for (int half = 1340; half <= 1356 && out.action_on; ++half) out = tick(half * .5f, half == 1342, false);
    EXPECT_EQ(out.action_no, 0x25);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.cooldown, 20); // the no-chain end of swing 1 (770)
}

TEST(IxionDc1Ungaga, HoldingThroughSwingOneChargesAndAReleaseInThirteenCancels) {
    // Held from the press to swing 1's end-1..end, action_step stays 0 and the handler enters 13 (754-759). Released
    // inside 13, action_on drops (855-863): no loop, no shot, no EndEffect.
    Resolver r;
    Host h(ungaga_config());
    auto out = h.Step(true, true);
    std::vector<int> chain{out.action_no};
    for (int t = 0; t < 60 && out.action_no != 13; ++t) {
        out = h.Step(false, true);
        if (chain.back() != out.action_no) chain.push_back(out.action_no);
    }
    ASSERT_EQ(out.action_no, 13);
    EXPECT_EQ(out.motion_no, 13);
    EXPECT_EQ(out.restart, 1);
    out = h.Step(false, true); // first tick of KEY 13 at 155
    out = h.Step(false, false);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.motion_no, 0);
    EXPECT_EQ(out.shot.spawned, 0);
    EXPECT_EQ(out.shot_ended, 0);
    EXPECT_EQ(out.shots_live, 0);
    EXPECT_EQ(chain, (std::vector<int>{0x25, 13}));
    // The same hold kept to 13's end-2..end enters the loop 14 (865-871).
    Host h2(ungaga_config());
    EnterLoop(h2, out);
    EXPECT_EQ(out.action_no, 14);
    EXPECT_EQ(out.action_on, 1);
    EXPECT_GE(out.time, 158.0f - 1e-3f);
}

TEST(IxionDc1Ungaga, TheLoopFiresC10aExEveryThirtyTicksAndTheReleaseEndsTheEffect) {
    // State 14 counts every Play tick on a static wait and fires at 30 (881-909): the shot spawns at the body's
    // "weapon" point (answered through the ef00 alias), aimed one unit along his facing (getCharacterVector), owner
    // 4, sub 2, damage 1.5x, wait 45, the element attribute. A release goes to 15, whose end-2..end ends the action
    // and calls EndEffect (911-932). The counter is static, so the first shot of a charge depends on the last one.
    Resolver r;
    Host h(ungaga_config());
    IxionDc1Output out{};
    EnterLoop(h, out);
    ASSERT_EQ(out.action_no, 14);
    std::vector<int> shot_ticks;
    std::vector<IxionDc1Shot> shots;
    int loop_tick = 0;
    for (; loop_tick < 100; ++loop_tick) {
        if (loop_tick > 0) out = h.Step(false, true);
        ASSERT_EQ(out.action_no, 14);
        if (out.shot.spawned) { shot_ticks.push_back(loop_tick); shots.push_back(out.shot); }
    }
    ASSERT_GE(shot_ticks.size(), 3u);
    EXPECT_LE(shot_ticks[0], 29);
    for (size_t i = 1; i < shot_ticks.size(); ++i) EXPECT_EQ(shot_ticks[i] - shot_ticks[i - 1], 30);
    const IxionDc1Shot &s = shots[0];
    EXPECT_FLOAT_EQ(s.position[0], 5); EXPECT_FLOAT_EQ(s.position[1], 6); EXPECT_FLOAT_EQ(s.position[2], 7);
    EXPECT_FLOAT_EQ(s.vector[0], 5); EXPECT_FLOAT_EQ(s.vector[1], 6); EXPECT_FLOAT_EQ(s.vector[2], 8); // + (0, 0, 1)
    EXPECT_EQ(s.owner, 4); EXPECT_EQ(s.sub_id, 2); EXPECT_EQ(s.source, 0); EXPECT_EQ(s.initial_phase, -1);
    EXPECT_EQ(s.damage, 52); // (int)(1.5 * 35)
    EXPECT_EQ(s.wait, 45); EXPECT_EQ(s.attribute, 8); // element_tbl[3]
    EXPECT_STREQ(s.effect, "c10a_ex"); EXPECT_FLOAT_EQ(s.effect_radius, 8);
    EXPECT_GE(s.slot, 0);
    // Release: 0xF on the same tick (the 30-count runs first), then 15 plays to its end-2..end.
    // wait after the last shot tick, plus the release tick's own count (or 0 if that tick fired).
    out = h.Step(false, false);
    EXPECT_EQ(out.action_no, 0xF);
    EXPECT_EQ(out.motion_no, 0xF);
    const int wait_after = out.shot.spawned ? 0 : loop_tick - shot_ticks.back();
    int ticks15 = 0;
    for (; ticks15 < 60 && out.action_on; ++ticks15) out = h.Step(false, false);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.shot_ended, 1);
    EXPECT_GE(ticks15, 13);
    // EndEffect moved every live c10a_ex still starting or flying to its impact phase.
    int live = 0;
    for (const auto &st : out.shots) {
        if (!st.active) continue;
        ++live;
        EXPECT_EQ(st.phase, 2);
    }
    EXPECT_GT(live, 0);
    // The next charge's first shot comes when the carried counter reaches 30.
    out = h.Step(true, true);
    for (int t = 0; t < 200 && out.action_no != 14; ++t) out = h.Step(false, true);
    ASSERT_EQ(out.action_no, 14);
    int first = -1;
    for (int t = 0; t < 40 && first < 0; ++t) {
        if (t > 0) out = h.Step(false, true);
        if (out.shot.spawned) first = t + 1; // 1-based loop tick
    }
    EXPECT_EQ(first, 30 - wait_after);
}

TEST(IxionDc1Ungaga, TheShotFliesAtItsPhaseSpeedLaysARecordEvery46StepsAndRetires) {
    // CSHOT_EFFECT::Set/Step for MyEntryEffect_UNGAGA (c10a_ex: not stationary, speed {0.2, 0.9, 0.2, 0.2}, radius
    // {8, 8, 6, 6}, life 90, hit kind 3, target 2, motion {0, 1, 2, 2}). Velocity is (target - position) normalised
    // and scaled by speed[0]; Step's 0 -> 1 hand-over keeps it (only the 1 -> 2 and 1 -> 3 hand-overs rescale,
    // shot_effect.cpp:148-159, 243-244, 272-273), so it flies at 0.2 throughout. Phase 0 ends when the effect's KEY 0
    // (5-25, 0.6) reaches 24: 32 steps. Phase 1 runs life + 1 = 91 steps, phase 3 until KEY 2 (55-70) reaches 69.
    Resolver r;
    ColliderScope col;
    auto c = ungaga_config();
    c.element = 1; // ice: SetAttribute writes element_tbl[1] = 2 over the row's hit_flags 8
    Host h(c);
    IxionDc1CharacterInfo info{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_UNGAGA, &info), 1);
    EXPECT_EQ(info.effect_stationary, 0);
    EXPECT_FLOAT_EQ(info.effect_speed[0], .2f); EXPECT_FLOAT_EQ(info.effect_speed[1], .9f);
    EXPECT_EQ(info.effect_life, 90);
    EXPECT_STREQ(info.shot_point_alias, "ef00");
    IxionDc1Output out{};
    EnterLoop(h, out);
    int slot = -1;
    for (int t = 0; t < 40 && slot < 0; ++t) {
        out = h.Step(false, true);
        if (out.shot.spawned) slot = out.shot.slot;
    }
    ASSERT_GE(slot, 0);
    const int serial = out.shots[slot].serial;
    // Keep charging (the loop keeps firing into other slots) and follow this one slot.
    std::vector<IxionDc1ShotStep> track{out.shots[slot]};
    for (int t = 0; t < 200; ++t) {
        out = h.Step(false, true);
        const auto &st = out.shots[slot];
        if (st.serial != serial) break;
        track.push_back(st);
        if (!st.active) break;
    }
    ASSERT_GT(track.size(), 100u);
    EXPECT_EQ(track.front().spawned, 1);
    EXPECT_EQ(track.front().age, 1);
    EXPECT_EQ(col.c.target, 2); // monsters only: checkCollision never tests the player
    std::vector<int> record_ages;
    int phase1_from = -1, phase3_from = -1;
    for (const auto &st : track) {
        SCOPED_TRACE(st.age);
        EXPECT_NEAR(std::sqrt(st.velocity[0] * st.velocity[0] + st.velocity[1] * st.velocity[1] +
                              st.velocity[2] * st.velocity[2]), .2f, 1e-5f);
        EXPECT_NEAR(st.velocity[2], .2f, 1e-5f); // facing +Z at yaw 0
        EXPECT_EQ(st.owner, 4); EXPECT_EQ(st.sub_id, 2); EXPECT_EQ(st.damage, 52);
        if (st.phase == 1 && phase1_from < 0) phase1_from = st.age;
        if (st.phase == 3 && phase3_from < 0) phase3_from = st.age;
        if (st.record) {
            record_ages.push_back(st.age);
            EXPECT_FLOAT_EQ(st.record_radius, st.phase < 2 ? 8 : 6);
            EXPECT_EQ(st.record_life, 2); EXPECT_EQ(st.record_kind, 3); EXPECT_EQ(st.record_target, 2);
            EXPECT_EQ(st.record_flags, 2); EXPECT_EQ(st.record_weapon_flags, 0);
            EXPECT_NEAR(st.record_throw[2], .2f, 1e-5f);
        }
        // The position moves 0.2 a step from ef00 (no collision answered).
        EXPECT_NEAR(st.position[2], 7 + .2f * st.age, 1e-3f);
        EXPECT_FLOAT_EQ(st.position[0], 5);
    }
    EXPECT_EQ(track.front().record, 1);
    EXPECT_FLOAT_EQ(track.front().record_position[2], 7); // laid before the first move
    EXPECT_EQ(record_ages, (std::vector<int>{1, 47, 93, 139}));
    EXPECT_EQ(phase1_from, 33);
    EXPECT_EQ(phase3_from, 32 + 91); // phase_delay 90 -> -1 over 91 flight steps, the first at age 33
    const auto &last = track.back();
    EXPECT_EQ(last.active, 0);
    EXPECT_EQ(last.ended, IXION_DC1_SHOT_END_MOTION);
    EXPECT_EQ(last.age, 32 + 91 + 24); // KEY 2 from 55 reaches 69.4 on the 24th advance
}

TEST(IxionDc1Ungaga, AMonsterStopsTheFlightAWallInPhaseZeroOnlyStallsItAndOffEffectClears) {
    // Step asks checkCollision only in phases 0 and 1 (shot_effect.cpp:168-170). A hit in phase 1 holds the
    // position, records at the contact if the wait clock is due, and moves to the impact phase 2, which ends when
    // KEY 2 reaches 69 (252-290). A hit in phase 0 changes nothing but the step's move (214-218).
    Resolver r;
    ColliderScope col;
    Host h(ungaga_config());
    IxionDc1Output out{};
    EnterLoop(h, out);
    int slot = -1;
    for (int t = 0; t < 40 && slot < 0; ++t) {
        out = h.Step(false, true);
        if (out.shot.spawned) slot = out.shot.slot;
    }
    ASSERT_GE(slot, 0);
    // Only this shot from here on. A release would reach 15 and EndEffect and end it early, so the handler is
    // cancelled instead (a DC2 hit does the same), which leaves the effect running as in DC1.
    const int serial = out.shots[slot].serial;
    const int calls_at_spawn = col.c.calls;
    const int age_at_spawn = out.shots[slot].age;
    ASSERT_EQ(age_at_spawn, 1);
    // Phase 0 wall at age 5, monster at age 40 (phase 1); age 47 would be the second record.
    col.c.result = IXION_DC1_SHOT_HIT_MAP;
    std::vector<IxionDc1ShotStep> track{out.shots[slot]};
    auto follow = [&](int ticks) {
        for (int t = 0; t < ticks; ++t) {
            IxionDc1Input in{};
            in.time = 10; in.cancel = 1; in.position[3] = in.weapon_tip[3] = 1;
            ixion_dc1_tick(&in, &out);
            const auto &st = out.shots[slot];
            if (st.serial != serial) return;
            track.push_back(st);
            if (!st.active) return;
        }
    };
    // Calls are one per live slot per tick; with only this slot live after the cancel, call n is age n.
    for (const auto &st : out.shots) ASSERT_TRUE(!st.active || st.serial == serial);
    col.c.on_calls = {calls_at_spawn + 4};
    follow(5);
    ASSERT_GE(track.size(), 6u);
    EXPECT_EQ(track[4].collision, IXION_DC1_SHOT_HIT_MAP);
    EXPECT_EQ(track[4].phase, 0);
    EXPECT_FLOAT_EQ(track[4].position[2], track[3].position[2]); // stalled, not stopped
    EXPECT_NEAR(track[5].position[2], track[3].position[2] + .2f, 1e-4f);
    col.c.result = IXION_DC1_SHOT_HIT_MONSTER;
    col.c.on_calls = {calls_at_spawn + 46}; // age 47: phase 1, and the wait clock's second record
    follow(200);
    const auto hit = std::find_if(track.begin(), track.end(),
                                  [](const IxionDc1ShotStep &st) { return st.collision == IXION_DC1_SHOT_HIT_MONSTER; });
    ASSERT_NE(hit, track.end());
    EXPECT_EQ(hit->age, 47);
    EXPECT_EQ(hit->phase, 2);
    EXPECT_EQ(hit->record, 1);
    EXPECT_FLOAT_EQ(hit->record_position[0], 100); // the monster's sphere, not the effect's position
    EXPECT_FLOAT_EQ(hit->position[2], (hit - 1)->position[2]);
    EXPECT_FLOAT_EQ(col.c.radius, 8);
    EXPECT_EQ(track.back().ended, IXION_DC1_SHOT_END_MOTION);
    EXPECT_EQ(track.back().age, 47 + 24);
    for (auto it = hit + 1; it != track.end(); ++it) EXPECT_EQ(it->record, 0); // next due at 93
    // OffEffect: every slot goes at once, with nothing reported afterwards.
    Host h2(ungaga_config());
    EnterLoop(h2, out);
    for (int t = 0; t < 40 && out.shots_live == 0; ++t) out = h2.Step(false, true);
    ASSERT_GT(out.shots_live, 0);
    ixion_dc1_off_effect();
    IxionDc1Input cancel{};
    cancel.time = 10; cancel.cancel = 1; cancel.position[3] = cancel.weapon_tip[3] = 1;
    ixion_dc1_tick(&cancel, &out);
    EXPECT_EQ(out.shots_live, 0);
    for (const auto &st : out.shots) EXPECT_EQ(st.active, 0);
}

TEST(IxionDc1Ungaga, GaugeRefillsOnlyOutsideHisSwingsAndGoroIsNotDamped) {
    // gameloop.cpp:6113-6135: for Toan and Ungaga the refill is 0.25x outside a swing and nothing inside one (the
    // handler sets swinging while start < time < end of KEY 37-39, charakey.cpp:691-700). Goro has no such branch.
    auto c = ungaga_config(false);
    c.action_gauge = 50;
    ixion_dc1_reset(&c);
    auto out = tick(10, true, false); // On; Play at time 10 (not inside a swing KEY): refill this tick
    const float gain = 65.0f / 30.0f * .25f;
    auto next = tick(672); // inside KEY 37: swinging
    EXPECT_FLOAT_EQ(next.action_gauge, out.action_gauge + gain);
    auto after = tick(672.3f);
    EXPECT_FLOAT_EQ(after.action_gauge, next.action_gauge); // no refill from the swinging tick
    // Goro (speed 30) refills a whole 1.0 per tick, not 0.25.
    IxionDc1Config g{};
    g.character = IXION_DC1_CHARA_GORO;
    g.motions[36] = {90, 108, .35f};
    g.attack = 36; g.weapon_speed = 30; g.action_gauge = 50;
    ixion_dc1_reset(&g);
    auto g0 = tick(10);
    auto g1 = tick(10);
    EXPECT_FLOAT_EQ(g1.action_gauge, g0.action_gauge + 1.0f);
}

TEST(IxionDc1Ungaga, NamedPointsAskWeaponThenItsEf00AliasAndOnlyThenFallBack) {
    // charakey.cpp:897 reads the body's "weapon"; the c10b BodyProfile renamed it ef00. The library asks the DC1
    // name first, then the row's alias, and only then uses weapon_tip.
    Resolver r;
    Host h(ungaga_config(false));
    IxionDc1Output out{};
    EnterLoop(h, out);
    int aliases = 0;
    for (int t = 0; t < 40 && !out.shot.spawned; ++t) { out = h.Step(false, true); aliases += out.point_aliases; }
    ASSERT_EQ(out.shot.spawned, 1);
    EXPECT_EQ(out.shot.slot, -1); // runner off: reported only
    EXPECT_EQ(aliases, 1);
    EXPECT_EQ(out.point_fallbacks, 0);
    EXPECT_FLOAT_EQ(out.shot.position[0], 5);
    const auto &names = r.points.names;
    ASSERT_GE(names.size(), 2u);
    EXPECT_EQ(names[names.size() - 2], "body:weapon");
    EXPECT_EQ(names.back(), "body:ef00");
    // A body that answers "weapon" itself needs no alias; one that answers neither falls back to the tip.
    r.points.answer_ef00 = false;
    Host h2(ungaga_config(false));
    EnterLoop(h2, out);
    for (int t = 0; t < 40 && !out.shot.spawned; ++t) out = h2.Step(false, true);
    ASSERT_EQ(out.shot.spawned, 1);
    EXPECT_FLOAT_EQ(out.shot.position[0], 12);
    EXPECT_EQ(out.point_fallbacks, 1);
    // The table row.
    IxionDc1CharacterInfo info{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_UNGAGA, &info), 1);
    EXPECT_EQ(info.charge_loop_key, 14); EXPECT_EQ(info.release_key, 15);
    EXPECT_EQ(std::vector<int>(info.donor_keys, info.donor_keys + info.donor_key_count),
              (std::vector<int>{13, 14, 15, 37, 38, 39}));
    EXPECT_EQ(std::vector<int>(info.combo_keys, info.combo_keys + info.combo_key_count), (std::vector<int>{37, 38, 39}));
    EXPECT_STREQ(info.hit_point, "dcol0"); EXPECT_STREQ(info.hit_point_2, "dcol1");
    EXPECT_STREQ(info.shot_point, "weapon"); EXPECT_EQ(info.shot_frame, IXION_DC1_FRAME_BODY);
    // Toan's and Goro's rows have no alias, so their point traffic is unchanged.
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_GORO, &info), 1);
    EXPECT_STREQ(info.shot_point_alias, "");
    EXPECT_EQ(info.effect_stationary, 0); // MyEntryEffect_GORO: stationary 0 but speed 0
    EXPECT_FLOAT_EQ(info.effect_speed[0], 0);
}
