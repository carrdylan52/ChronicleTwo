#include <gtest/gtest.h>
#include "ixion_dc1_toan.h"
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

// Xiao R1: BattleActionOn_Jinn/BattleActionPlay_Jinn (dun/gameloop.cpp:7655-7820) and the 12-slot CSHOT pool
// (CSHOT::step, shot_effect.cpp:54-87), both verbatim through the same library.
namespace {
constexpr float kEff30[4] = {10, 20, 30, 1};

IxionDc1Config xiao_config() {
    IxionDc1Config c{};
    c.character = IXION_DC1_CHARA_XIAO;
    // c04b base.cfg KEY 11 (draw), 12 (hold) and 13 (fire); the caller supplies these, no asset is committed.
    c.motions[11] = {240, 251, .7f};
    c.motions[12] = {250, 250, 0};
    c.motions[13] = {251, 255, .7f};
    // Wooden Slingshot (itemdata.cpp:1472): attack 4, speed 40, no element (5).
    c.attack = 4; c.weapon_speed = 40; c.element = 5; c.weapon_flags = 0;
    c.action_gauge = 100;
    return c;
}

struct Points {
    std::vector<std::string> names;
    bool answer = true;
};
int32_t resolve(void *user, int32_t frame, const char *name, float out[4]) {
    auto *p = static_cast<Points *>(user);
    p->names.emplace_back(std::string(frame == IXION_DC1_FRAME_BODY ? "body:" : "weapon:") + name);
    if (!p->answer || frame != IXION_DC1_FRAME_WEAPON || std::strcmp(name, "eff30") != 0) return 0;
    std::memcpy(out, kEff30, sizeof(kEff30));
    return 1;
}
struct Resolver {
    Points points;
    Resolver() { ixion_dc1_set_point_resolver(resolve, &points); }
    ~Resolver() { ixion_dc1_set_point_resolver(nullptr, nullptr); }
};

struct Collider {
    int calls = 0, target = -1, hit_on = 0, result = IXION_DC1_SHOT_HIT_MONSTER;
    float radius = -1, movement[4] = {};
    float contact[4] = {100, 200, 300, 1};
};
int32_t collide(void *user, const float *, const float *movement, int32_t target, float radius, float hit_out[4]) {
    auto *c = static_cast<Collider *>(user);
    ++c->calls;
    c->target = target;
    c->radius = radius;
    std::memcpy(c->movement, movement, sizeof(c->movement));
    if (c->calls != c->hit_on) return IXION_DC1_SHOT_HIT_NONE;
    std::memcpy(hit_out, c->contact, sizeof(c->contact));
    return c->result;
}
struct ColliderScope {
    Collider c;
    ColliderScope() { ixion_dc1_set_shot_collider(collide, &c); }
    ~ColliderScope() { ixion_dc1_set_shot_collider(nullptr, nullptr); }
};

struct Pad {
    float yaw = 0;
    int lock_on = 0, aimed = 0;
    float target[4] = {};
    float view_yaw = 0, view_pitch = 0;
};
IxionDc1Output tick(float time, bool down, bool held, const Pad &pad = Pad{}) {
    IxionDc1Input in{};
    in.time = time;
    in.position[3] = in.weapon_tip[3] = 1;
    in.yaw = pad.yaw;
    in.cross_down = down;
    in.cross_held = held;
    in.lock_on = pad.lock_on;
    std::memcpy(in.target_position, pad.target, sizeof(in.target_position));
    in.view_yaw = pad.view_yaw;
    in.view_pitch = pad.view_pitch;
    in.aimed = pad.aimed;
    IxionDc1Output out{};
    ixion_dc1_tick(&in, &out);
    return out;
}
// A tapped shot: press at 240, released before the draw ends, 13 at 249.5, fire at 251.5. Returns the fire tick.
IxionDc1Output tap_and_fire(const Pad &pad = Pad{}) {
    tick(240, true, true, pad);
    tick(245, false, false, pad);
    const IxionDc1Output to13 = tick(249.5f, false, false, pad);
    EXPECT_EQ(to13.action_no, 13);
    return tick(251.5f, false, false, pad);
}
int spawned_slot(const IxionDc1Output &out) {
    int slot = -1;
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        if (out.cshots[i].spawned) {
            EXPECT_EQ(slot, -1) << "two spawns in one tick";
            slot = i;
        }
    }
    return slot;
}
float length3(const float *v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }
} // namespace

TEST(IxionDc1Xiao, TheTableRowIsTheJinnHandlerWithTwelveShotsFromTheSlingsEff30) {
    IxionDc1CharacterInfo info{};
    ASSERT_EQ(ixion_dc1_character_info(IXION_DC1_CHARA_XIAO, &info), 1);
    EXPECT_STREQ(info.name, "xiao");
    ASSERT_EQ(info.donor_key_count, 3);
    EXPECT_EQ(info.donor_keys[0], 11);
    EXPECT_EQ(info.donor_keys[2], 13);
    EXPECT_EQ(info.combo_key_count, 0);
    EXPECT_EQ(info.charge_loop_key, 12);
    EXPECT_EQ(info.release_key, 13);
    EXPECT_EQ(info.on_gauge_min, 100.0f);
    EXPECT_EQ(info.on_zeroes_gauge, 1);
    EXPECT_EQ(info.hit_owner, 1);
    EXPECT_STREQ(info.shot_point, "eff30");
    EXPECT_EQ(info.shot_frame, IXION_DC1_FRAME_WEAPON);
    EXPECT_EQ(info.cshot_pool, IXION_DC1_CSHOT_SLOTS);
    EXPECT_STREQ(info.effect, "");
    for (int other : {IXION_DC1_CHARA_TOAN, IXION_DC1_CHARA_GORO, IXION_DC1_CHARA_UNGAGA}) {
        ASSERT_EQ(ixion_dc1_character_info(other, &info), 1);
        EXPECT_EQ(info.cshot_pool, 0);
    }
}

TEST(IxionDc1Xiao, OnNeedsAFullGaugeEmptiesItAndXiaoRefillsUndamped) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    c.action_gauge = 99.5f; // (s32) 99 != 100 (gameloop.cpp:7656)
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    IxionDc1Output out = tick(240, true, true);
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.character, IXION_DC1_CHARA_XIAO);
    // Upkeep at the end of that tick: 40/30 per tick, no swing damp for Xiao (gameloop.cpp:6113-6135).
    out = tick(240, true, true);
    EXPECT_EQ(out.action_on, 1);
    EXPECT_EQ(out.action_no, 11);
    EXPECT_EQ(out.motion_no, 11);
    EXPECT_EQ(out.restart, 1);
    EXPECT_EQ(out.action_gauge, 0.0f);
    out = tick(241, false, true);
    EXPECT_NEAR(out.action_gauge, 40.0f / 30.0f, 1e-5);
    out = tick(242, false, true);
    EXPECT_NEAR(out.action_gauge, 80.0f / 30.0f, 1e-5);
}

TEST(IxionDc1Xiao, ATapFiresOneShotFromEff30AlongTheFacingAtSpeedFive) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    Pad pad;
    pad.yaw = 0.5f;
    tick(240, true, true, pad);
    tick(245, false, false, pad);
    IxionDc1Output out = tick(249.5f, false, false, pad); // released: 11's end-2..end goes straight to 13
    EXPECT_EQ(out.action_no, 13);
    EXPECT_EQ(out.motion_no, 13);
    EXPECT_EQ(out.cshots_live, 0);
    out = tick(251.0f, false, false, pad); // 251.0 is outside the open window 251 < now < 252
    EXPECT_EQ(spawned_slot(out), -1);
    out = tick(251.5f, false, false, pad);
    const int slot = spawned_slot(out);
    ASSERT_EQ(slot, 0);
    const IxionDc1ShotStep &s = out.cshots[slot];
    EXPECT_EQ(s.active, 1);
    EXPECT_EQ(s.serial, 1);
    EXPECT_EQ(s.age, 1); // NowShotData->step() runs in the spawn tick (gameloop.cpp:6309)
    EXPECT_EQ(s.damage, 4);
    EXPECT_EQ(s.owner, 1);
    EXPECT_EQ(s.life, 0x78 - 1);
    EXPECT_NEAR(length3(s.velocity), 5.0f, 1e-5);
    EXPECT_NEAR(s.velocity[0], 5.0f * std::sin(0.5f), 1e-5);
    EXPECT_NEAR(s.velocity[1], 0.0f, 1e-6);
    EXPECT_NEAR(s.velocity[2], 5.0f * std::cos(0.5f), 1e-5);
    for (int k = 0; k < 3; k++) EXPECT_NEAR(s.position[k], kEff30[k] + s.velocity[k], 1e-4) << k;
    EXPECT_EQ(out.cshots_live, 1);
    EXPECT_EQ(out.point_missing, 0);
    ASSERT_FALSE(r.points.names.empty());
    EXPECT_EQ(r.points.names.back(), "weapon:eff30");
    out = tick(252.0f, false, false, pad); // closed again
    EXPECT_EQ(spawned_slot(out), -1);
    EXPECT_EQ(out.cshots[0].age, 2);
    out = tick(253.5f, false, false, pad); // 13's end-2..end ends the action; no lock-on: motion 0
    EXPECT_EQ(out.action_on, 0);
    EXPECT_EQ(out.motion_no, 0);
    EXPECT_EQ(out.hit_count, 0); // SwordDmgCheck1 stays a stub; Xiao lays no melee records
}

TEST(IxionDc1Xiao, LockOnAimsTheShotAtTheTargetAndEndsInBattleStance) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    Pad pad;
    pad.lock_on = 1;
    pad.yaw = 2.0f; // ignored with lock-on
    pad.target[0] = kEff30[0] + 30; pad.target[1] = kEff30[1] + 40; pad.target[2] = kEff30[2];
    const IxionDc1Output out = tap_and_fire(pad);
    const int slot = spawned_slot(out);
    ASSERT_GE(slot, 0);
    EXPECT_NEAR(out.cshots[slot].velocity[0], 3.0f, 1e-5); // 5 * (30, 40, 0) / 50
    EXPECT_NEAR(out.cshots[slot].velocity[1], 4.0f, 1e-5);
    EXPECT_NEAR(out.cshots[slot].velocity[2], 0.0f, 1e-5);
    EXPECT_EQ(tick(253.5f, false, false, pad).motion_no, 0x12);
}

TEST(IxionDc1Xiao, HoldingThroughTheDrawWaitsInTwelveUntilTheRelease) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    tick(240, true, true);
    IxionDc1Output out = tick(249.5f, false, true);
    EXPECT_EQ(out.action_no, 12);
    EXPECT_EQ(out.motion_no, 12);
    EXPECT_EQ(out.restart, 1);
    out = tick(250, false, true);
    EXPECT_EQ(out.action_no, 12);
    out = tick(250, false, false); // the release
    EXPECT_EQ(out.action_no, 13);
    out = tick(251.5f, false, false);
    EXPECT_GE(spawned_slot(out), 0);
}

TEST(IxionDc1Xiao, AShotLives120StepsAndAThirteenthFindsNoSlot) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    c.weapon_speed = 3000; // the gauge refills in one tick, so shots come every four ticks
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    int fired = 0, first_end_age = -1;
    for (int shot = 0; shot < 13; shot++) {
        const float times[4] = {240, 245, 249.5f, 251.5f};
        for (int k = 0; k < 4; k++) {
            const IxionDc1Output out = tick(times[k], k == 0, k == 0);
            if (k == 3) {
                const int slot = spawned_slot(out);
                if (shot < 12) {
                    EXPECT_EQ(slot, shot);
                    EXPECT_EQ(out.cshots_live, shot + 1);
                } else {
                    EXPECT_EQ(slot, -1); // all 12 used: the loop finds no free slot (gameloop.cpp:7729-7741)
                    EXPECT_EQ(out.cshots_live, 12);
                }
                if (slot >= 0) ++fired;
            }
        }
        tick(253.5f, false, false);
    }
    EXPECT_EQ(fired, 12);
    // Shot 1 ends on the step its life reaches 0 (shot_effect.cpp:81-85): age 120, having moved every step.
    for (int t = 0; t < 200 && first_end_age < 0; t++) {
        const IxionDc1Output out = tick(0, false, false);
        if (out.cshots[0].ended != 0) {
            first_end_age = out.cshots[0].age;
            EXPECT_EQ(out.cshots[0].ended, IXION_DC1_SHOT_END_LIFE);
            EXPECT_EQ(out.cshots[0].active, 0);
            EXPECT_EQ(out.cshots[0].life, 0);
            for (int k = 0; k < 3; k++)
                EXPECT_NEAR(out.cshots[0].position[k], kEff30[k] + 120 * out.cshots[0].velocity[k], 1e-2) << k;
        }
    }
    EXPECT_EQ(first_end_age, 120);
}

TEST(IxionDc1Xiao, DamageIsTheWeaponsAttackDoubledUnderStamina) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    c.attack = 21;
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    IxionDc1Output out = tap_and_fire();
    ASSERT_GE(spawned_slot(out), 0);
    EXPECT_EQ(out.cshots[spawned_slot(out)].damage, 21);
    c.ailments = 0x08; // AILMENT_STAMINA (userstatus.hpp:32)
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    out = tap_and_fire();
    ASSERT_GE(spawned_slot(out), 0);
    EXPECT_EQ(out.cshots[spawned_slot(out)].damage, 42);
}

TEST(IxionDc1Xiao, HandAimFliesAtThreePointFiveAlongTheViewInThePalWindow) {
    Resolver r;
    IxionDc1Config c = xiao_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    Pad pad;
    pad.aimed = 1;
    pad.yaw = 2.0f; // the body's facing is not used when hand-aimed
    pad.view_yaw = 0.3f;
    pad.view_pitch = -0.2f;
    tick(240, true, true, pad);
    tick(245, false, false, pad);
    EXPECT_EQ(tick(249.5f, false, false, pad).action_no, 13);
    // 21.6 is outside the NTSC window (21.0-21.5) and inside PAL_TIMING's (21.0-21.8, gameloop.cpp:7749-7753).
    const IxionDc1Output out = tick(21.6f, false, false, pad);
    const int slot = spawned_slot(out);
    ASSERT_GE(slot, 0);
    const float *v = out.cshots[slot].velocity;
    EXPECT_NEAR(length3(v), 3.5f, 1e-5);
    // setShotVector(3.5, H, V) (btitem.cpp:1316-1328): RotX(V), then RotY(H), applied to (0, 0, 3.5).
    EXPECT_NEAR(v[0], 3.5f * std::cos(-0.2f) * std::sin(0.3f), 1e-5);
    EXPECT_NEAR(v[1], -3.5f * std::sin(-0.2f), 1e-5);
    EXPECT_NEAR(v[2], 3.5f * std::cos(-0.2f) * std::cos(0.3f), 1e-5);
    EXPECT_EQ(spawned_slot(tick(21.85f, false, false, pad)), -1);
}

TEST(IxionDc1Xiao, FirstContactLaysAnOwnerOneRecordAtThePreStepPositionAndFreesTheSlot) {
    Resolver r;
    ColliderScope cs;
    cs.c.hit_on = 3; // the third step asks and finds the monster
    IxionDc1Config c = xiao_config();
    c.element = 1;     // best_elem ice: GetWeaponElementAttr(1) = 2 (hitvalue.cpp:29)
    c.shot_scale = 2;  // host units per DC1 unit
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    IxionDc1Output out = tap_and_fire();
    const int slot = spawned_slot(out);
    ASSERT_EQ(slot, 0);
    EXPECT_EQ(cs.c.target, 2); // CSHOT asks for monsters only: the player (Max) is never tested
    EXPECT_EQ(cs.c.radius, 2.0f);
    EXPECT_NEAR(length3(cs.c.movement), 10.0f, 1e-4); // 5.0 DC1 units per step at shot_scale 2
    float before[4];
    std::memcpy(before, out.cshots[slot].position, sizeof(before));
    out = tick(252, false, false);
    EXPECT_EQ(out.cshots[slot].collision, IXION_DC1_SHOT_HIT_NONE);
    std::memcpy(before, out.cshots[slot].position, sizeof(before));
    out = tick(253, false, false);
    const IxionDc1ShotStep &s = out.cshots[slot];
    EXPECT_EQ(s.age, 3);
    EXPECT_EQ(s.collision, IXION_DC1_SHOT_HIT_MONSTER);
    EXPECT_EQ(s.ended, IXION_DC1_SHOT_END_HIT);
    EXPECT_EQ(s.active, 0);
    ASSERT_EQ(s.record, 1);
    for (int k = 0; k < 3; k++) {
        EXPECT_EQ(s.position[k], before[k]) << k; // no move on the hit step
        EXPECT_EQ(s.record_position[k], before[k]) << k;
    }
    EXPECT_EQ(s.record_radius, 3.0f);
    EXPECT_EQ(s.record_life, 1);
    EXPECT_EQ(s.record_target, 2);
    EXPECT_EQ(s.record_kind, 2);
    EXPECT_EQ(s.record_flags, 2);
    EXPECT_EQ(s.damage, 4);
    // The same record is a raw DC1 record in hits[], owner 1 from SetUserID(1, 0).
    ASSERT_EQ(out.hit_count, 1);
    EXPECT_EQ(out.hits[0].owner, 1);
    EXPECT_EQ(out.hits[0].kind, 2);
    EXPECT_EQ(out.hits[0].damage, 4);
    EXPECT_EQ(out.cshots_live, 0);
    out = tick(254, false, false);
    EXPECT_EQ(out.hit_count, 0); // life 1: aged away
    EXPECT_EQ(out.cshots[slot].ended, 0);
    EXPECT_EQ(cs.c.calls, 3);
}

TEST(IxionDc1Xiao, NoEff30OnTheWeaponMeansNoShotAndOffEffectEmptiesThePool) {
    Resolver r;
    r.points.answer = false;
    IxionDc1Config c = xiao_config();
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    IxionDc1Output out = tap_and_fire();
    EXPECT_EQ(spawned_slot(out), -1);
    EXPECT_EQ(out.point_missing, 1);
    EXPECT_EQ(out.cshots_live, 0);
    r.points.answer = true;
    ASSERT_EQ(ixion_dc1_reset(&c), 1);
    out = tap_and_fire();
    EXPECT_EQ(out.cshots_live, 1);
    ixion_dc1_off_effect();
    out = tick(252, false, false);
    EXPECT_EQ(out.cshots_live, 0);
    EXPECT_EQ(out.cshots[0].age, 0); // nothing stepped
}
