#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "collision.hpp"
#include "ixion_room.hpp"
#include "ixion_room_arena.hpp"
#include "platform_fixture.hpp"

namespace {

using Json = nlohmann::json;

std::string Sha(const std::string &text) {
    return IxionArenaSha256Hex(reinterpret_cast<const uint8_t *>(text.data()), text.size());
}

// A 1x1 RGBA PNG.
const unsigned char kPng[] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48,
                              0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00,
                              0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41, 0x54, 0x78,
                              0x9c, 0x63, 0xf8, 0xdf, 0xc0, 0xf0, 0x1f, 0x00, 0x06, 0x80, 0x02, 0x7f, 0x10, 0x4c,
                              0x1b, 0xe1, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

IxionArenaTile MakeTile(int slot) {
    IxionArenaTile tile;
    tile.slot = slot;
    tile.first_frame = 1.0f;
    tile.speed = 0.6f;
    tile.done_at = 59.0f;
    IxionRoomTriangle a, b;
    a.vertices = {{{0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}}};
    b.vertices = {{{10.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 10.0f}}};
    for (auto &triangle : {a, b}) {
        IxionRoomTriangle moved = triangle;
        for (auto &corner : moved.vertices) corner[0] += 20.0f * static_cast<float>(slot);
        tile.collision.push_back(moved);
    }
    return tile;
}

IxionArena MakeArena(int slots) {
    IxionArena arena;
    arena.room = "d01boss";
    arena.scale = 1.5584790682688f;
    arena.fall_below_y = -46.754372f;
    arena.fall_life = -40;
    arena.fall_script = 5;
    for (int slot = 0; slot < slots; ++slot) arena.tiles.push_back(MakeTile(slot));
    IxionArenaMarker marker;
    marker.name = "event01_2";
    marker.script = 500;
    marker.auto_start = true;
    marker.position = {0.0f, 1.558479f, -576.637493f};
    marker.horizontal_radius = 46.754372f;
    marker.height_window = 62.339163f;
    arena.markers.push_back(marker);
    return arena;
}

class ArenaFixture : public testing::Test {
protected:
    std::filesystem::path root;

    void SetUp() override {
        IxionRoomArenaResetForTest();
        root = std::filesystem::temp_directory_path() /
               std::format("ct-ixion-arena-{}-{}", testing::UnitTest::GetInstance()->current_test_info()->name(),
                           dc::test::ProcessId());
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root / "arena" / "fire");
        for (const char *name : {"arena/fire/lightling.png", "arena/fire/flame_flipbook.png"}) {
            std::ofstream out(root / name, std::ios::binary);
            out.write(reinterpret_cast<const char *>(kPng), sizeof kPng);
        }
    }

    void TearDown() override {
        IxionRoomArenaResetForTest();
        std::filesystem::remove_all(root);
    }

    static Json Sidecar() {
        Json vertex = {{"position", {0, 0, 0}}, {"normal", {0, 1, 0}}, {"uv", {0, 0}}, {"color", {1, 1, 1, 1}}};
        Json object = {{"index", 1},
                       {"name", "obj1"},
                       {"rest", {{"t", {1, 2, 3}}, {"q", {1, 0, 0, 0}}}},
                       {"groups", Json::array({{{"texture", 0}, {"mdt_material", 0}, {"vertices", {vertex, vertex, vertex}}}})},
                       {"track", {{"t", {{1, 2, 3}, {1, -8, 3}}}, {"q", {{1, 0, 0, 0}, {1, 0, 0, 0}}}, {"alpha", {{"0", {1.0, 0.0}}}}}}};
        Json tile = {{"slot", 0},
                     {"ground_kind", 1},
                     {"objects", Json::array({object})},
                     {"collision", Json::array({Json::array({{0, 0, 0}, {10, 0, 0}, {0, 0, 10}})})},
                     {"motion", {{"frames", {1, 2}}, {"speed", 0.6}, {"done_at", 1.5}}}};
        return {{"format", "ixion-room-arena"},
                {"version", 1},
                {"room", "d01boss"},
                {"room_json_sha256", std::string(64, 'a')},
                {"units", {{"scale", 1.5}}},
                {"field_tiles",
                 {{"rule",
                   {{"slots", 1},
                    {"initial_state", 3},
                    {"hit", {{"decrement_when_state_ge", 2}}},
                    {"fall", {{"below_y", -46.75}, {"life", -40}, {"script", 5}}}}},
                  {"tiles", Json::array({tile})}}},
                {"fires",
                 {{"emitters", Json::array({{{"id", "fire-0"}, {"position", {1, 2, 3}}}})},
                  {"textures", {{"lightling", "arena/fire/lightling.png"}}},
                  {"flipbook", {{"file", "arena/fire/flame_flipbook.png"}, {"frame_size", {1, 1}}, {"columns", 1}, {"frames", 1}}},
                  {"draw",
                   {{"offset_y", 7.0},
                    {"layers",
                     Json::array({{{"layer", 1}, {"textures", {"lightling", "flame"}}, {"size", {28, 14}}},
                                  {{"layer", 2},
                                   {"textures", {"lightling"}},
                                   {"size", {70, 35}},
                                   {"flicker", {{"every_steps", 5}, {"scale_max", 1.1}}},
                                   {"depth_bias_toward_camera", 23.0}}})}}}}},
                {"camera_collision", Json::array({{{"objects", Json::array({{{"triangles", Json::array({Json::array({{0, 0, 0}, {0, 10, 0}, {10, 0, 0}})})}}})}}})},
                {"markers",
                 Json::array({{{"name", "event01_2"},
                               {"script", 500},
                               {"switch_on", 0},
                               {"auto_start", 1},
                               {"position", {0, 1.5, -576}},
                               {"trigger", {{"horizontal_radius", 46.75}, {"height_window", 62.3}}}}})}};
    }

    std::filesystem::path Write(const Json &json) {
        const auto path = root / "arena.json";
        std::ofstream(path, std::ios::binary) << json.dump();
        return path;
    }
};

} // namespace

TEST(IxionArenaRule, Sha256MatchesKnownVectors) {
    EXPECT_EQ(Sha(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(Sha("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_EQ(Sha("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    EXPECT_EQ(Sha(std::string(1000000, 'a')), "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

// shot_utils.cpp:75-89 and dranmapfield.cpp:36,72,79-107.
TEST(IxionArenaRule, HitAndStepFollowDranMapField) {
    const std::vector<IxionArenaTile> tiles = {MakeTile(0), MakeTile(1)};
    std::vector<IxionArenaTileState> states(2);
    EXPECT_TRUE(IxionArenaTileCollides(3));
    EXPECT_TRUE(IxionArenaTileCollides(2));
    EXPECT_FALSE(IxionArenaTileCollides(1));
    EXPECT_TRUE(IxionArenaTileDrawn(1));
    EXPECT_FALSE(IxionArenaTileDrawn(0));

    EXPECT_TRUE(IxionArenaApplyHit(states, 0, 2));
    EXPECT_EQ(states[0].state, 2);
    EXPECT_FALSE(IxionArenaApplyHit(states, 7, 2));
    EXPECT_EQ(IxionArenaStepTiles(states, tiles), 1); // 2 -> 1 starts the collapse
    EXPECT_EQ(states[0].state, 1);
    EXPECT_FLOAT_EQ(states[0].time, 1.0f);
    EXPECT_FALSE(IxionArenaApplyHit(states, 0, 2)); // falling tiles ignore hits
    EXPECT_EQ(states[1].state, 3);
    int steps = 0;
    while (states[0].state == 1 && steps < 1000) {
        IxionArenaStepTiles(states, tiles);
        ++steps;
    }
    EXPECT_EQ(states[0].state, 0);
    EXPECT_EQ(steps, 97); // 1 + 0.6 * 97 >= 59, the R1 "about 97 steps"

    // Two hits before a step skip state 2 and its start sound, as DC1 does.
    std::vector<IxionArenaTileState> twice(1);
    EXPECT_TRUE(IxionArenaApplyHit(twice, 0, 2));
    EXPECT_TRUE(IxionArenaApplyHit(twice, 0, 2));
    EXPECT_EQ(twice[0].state, 1);
    EXPECT_EQ(IxionArenaStepTiles(twice, {MakeTile(0)}), 0);
}

TEST(IxionArenaRule, TriggerArmsOutsideAndFiresOnceInside) {
    IxionArena arena = MakeArena(1);
    const IxionArenaMarker &marker = arena.markers[0];
    const float spawn[3] = {0.0f, 1.558f, -576.637f};
    const float edge[3] = {0.0f, 1.558f, -576.637f + 46.0f};
    const float outside[3] = {0.0f, 1.558f, -576.637f + 47.5f};
    const float above[3] = {0.0f, 1.558f + 63.0f, -576.637f};
    EXPECT_TRUE(IxionArenaInsideMarker(marker, spawn));
    EXPECT_TRUE(IxionArenaInsideMarker(marker, edge));
    EXPECT_FALSE(IxionArenaInsideMarker(marker, outside));
    EXPECT_FALSE(IxionArenaInsideMarker(marker, above));

    IxionArenaTrigger off;
    EXPECT_FALSE(IxionArenaTriggerStep(off, false));
    EXPECT_FALSE(IxionArenaTriggerStep(off, true));

    IxionArenaTrigger trigger;
    trigger.switched_on = true;
    EXPECT_FALSE(IxionArenaTriggerStep(trigger, true)); // spawned on the marker: not armed yet
    EXPECT_FALSE(trigger.armed);
    EXPECT_FALSE(IxionArenaTriggerStep(trigger, false));
    EXPECT_TRUE(trigger.armed);
    EXPECT_TRUE(IxionArenaTriggerStep(trigger, true));
    EXPECT_FALSE(trigger.switched_on); // prog500 switches itself off
    EXPECT_FALSE(IxionArenaTriggerStep(trigger, false));
    EXPECT_FALSE(IxionArenaTriggerStep(trigger, true));
}

TEST(IxionArenaRule, PoseSamplingAndMatrix) {
    IxionArenaObject object;
    object.rest_t = {1.0f, 2.0f, 3.0f};
    const float h = std::sqrt(0.5f);
    object.track_t = {{1.0f, 2.0f, 3.0f}, {1.0f, -8.0f, 3.0f}};
    object.track_q = {{1.0f, 0.0f, 0.0f, 0.0f}, {-h, 0.0f, -h, 0.0f}}; // same rotation as (h, 0, h, 0)
    object.track_alpha = {{0, {1.0f, 0.0f}}};
    float t[3], q[4];
    IxionArenaSamplePose(object, 1.0f, 1.5f, t, q);
    EXPECT_FLOAT_EQ(t[1], -3.0f);
    // Shortest-path slerp halfway to 90 degrees about y is 45 degrees about y.
    EXPECT_NEAR(q[0], std::cos(0.3926991f), 1e-5f);
    EXPECT_NEAR(q[2], std::sin(0.3926991f), 1e-5f);
    EXPECT_NEAR(IxionArenaSampleAlpha(object, 0, 1.0f, 1.5f), 0.5f, 1e-6f);
    EXPECT_FLOAT_EQ(IxionArenaSampleAlpha(object, 3, 1.0f, 1.5f), 1.0f);
    IxionArenaSamplePose(object, 1.0f, 99.0f, t, q); // clamped to the last key
    EXPECT_FLOAT_EQ(t[1], -8.0f);

    // R*v with q = 90 degrees about y maps +x to -z; translation in the last column.
    const float quarter[4] = {h, 0.0f, h, 0.0f};
    const float move[3] = {5.0f, 6.0f, 7.0f};
    float m[16];
    IxionArenaPoseMatrix(move, quarter, m);
    EXPECT_NEAR(m[0], 0.0f, 1e-6f);
    EXPECT_NEAR(m[2], -1.0f, 1e-6f);
    EXPECT_NEAR(m[8], 1.0f, 1e-6f);
    EXPECT_FLOAT_EQ(m[12], 5.0f);
    EXPECT_FLOAT_EQ(m[14], 7.0f);
}

TEST(IxionArenaRule, CollisionCarriesTileTagsInPartsNo) {
    IxionRoom room;
    IxionRoomTriangle floor;
    floor.vertices = {{{-100.0f, 0.0f, -100.0f}, {100.0f, 0.0f, -100.0f}, {-100.0f, 0.0f, -50.0f}}};
    room.collision.push_back(floor);
    const IxionArenaTile tile = MakeTile(3);
    const std::vector<uint16_t> tags(tile.collision.size(), static_cast<uint16_t>(kIxionArenaPartsTag | 3));
    const float lo[3] = {-200.0f, -10.0f, -200.0f};
    const float hi[3] = {200.0f, 10.0f, 200.0f};
    CCPoly polys[8];
    ASSERT_EQ(IxionRoomCollision(room, lo, hi, polys, 8, tile.collision, tags), 3);
    int tagged = 0;
    for (int i = 0; i < 3; ++i) {
        int slot = -1;
        if (IxionArenaSlotFromParts(polys[i].parts_no, slot)) {
            EXPECT_EQ(slot, 3);
            ++tagged;
        } else {
            EXPECT_EQ(polys[i].parts_no, 0);
        }
    }
    EXPECT_EQ(tagged, 2);
    int slot = -1;
    EXPECT_FALSE(IxionArenaSlotFromParts(CCPOLY_PARTS_EDIT | 3, slot)); // Georama parts are not tiles
    EXPECT_EQ(IxionRoomCollision(room, lo, hi, polys, 8), 1);           // the old overload is unchanged
}

TEST_F(ArenaFixture, LoadsSidecarAndRejectsBadInput) {
    IxionArena arena;
    std::string error;
    ASSERT_TRUE(IxionArena::Load(Write(Sidecar()), 1, arena, error)) << error;
    ASSERT_EQ(arena.tiles.size(), 1u);
    EXPECT_EQ(arena.tiles[0].objects[0].track_t.size(), 2u);
    EXPECT_EQ(arena.fires.size(), 1u);
    ASSERT_EQ(arena.fire_layers.size(), 2u);
    EXPECT_TRUE(arena.fire_layers[0].flame);
    EXPECT_EQ(arena.fire_layers[1].flicker_every, 5);
    EXPECT_EQ(arena.camera.size(), 1u);
    EXPECT_EQ(arena.glow.width, 1u);

    EXPECT_FALSE(IxionArena::Load(Write(Sidecar()), 0, arena, error)); // texture not in room.json
    Json bad = Sidecar();
    bad["format"] = "ixion-room";
    EXPECT_FALSE(IxionArena::Load(Write(bad), 1, arena, error));
    bad = Sidecar();
    bad["field_tiles"]["tiles"][0]["objects"][0]["track"]["t"].push_back({1, 2, 3});
    EXPECT_FALSE(IxionArena::Load(Write(bad), 1, arena, error));
    bad = Sidecar();
    bad["units"]["scale"] = "big"; // wrong type is contained, never thrown
    EXPECT_FALSE(IxionArena::Load(Write(bad), 1, arena, error));
    EXPECT_NE(error.find("rejected"), std::string::npos);
    bad = Sidecar();
    bad["fires"]["textures"]["lightling"] = "../outside.png";
    EXPECT_FALSE(IxionArena::Load(Write(bad), 1, arena, error));
}

TEST_F(ArenaFixture, ConfigureNeedsMatchingRoomHashAndStaysOffWithoutSidecar) {
    const auto room_json = root / "room.json";
    std::ofstream(room_json, std::ios::binary) << "{\"room\":1}";
    IxionRoom room;
    room.donor_room = "d01boss";
    room.textures.resize(1);
    // No arena.json: nothing loads, the adapter path is unchanged.
    IxionRoomArenaConfigure(room_json, room);
    EXPECT_FALSE(IxionRoomArenaLoaded());

    IxionRoomArenaResetForTest();
    Write(Sidecar()); // room_json_sha256 is "aaaa...": does not match
    IxionRoomArenaConfigure(room_json, room);
    EXPECT_FALSE(IxionRoomArenaLoaded());

    IxionRoomArenaResetForTest();
    Json good = Sidecar();
    good["room_json_sha256"] = Sha("{\"room\":1}");
    Write(good);
    IxionRoomArenaConfigure(room_json, room);
    EXPECT_TRUE(IxionRoomArenaLoaded());
    EXPECT_TRUE(IxionRoomArenaDefersDonor());
}

TEST_F(ArenaFixture, RuntimeHitStepCollisionAndRetire) {
    IxionRoomArenaAdoptForTest(MakeArena(2));
    const std::vector<IxionRoomTriangle> *triangles = nullptr;
    const std::vector<uint16_t> *tags = nullptr;
    IxionRoomArenaActiveCollision(triangles, tags);
    EXPECT_TRUE(triangles->empty()); // not installed: no tile collision

    IxionRoomArenaInstall();
    ASSERT_TRUE(IxionRoomArenaInstalled());
    EXPECT_TRUE(IxionRoomArenaTriggerForTest().switched_on);
    IxionRoomArenaActiveCollision(triangles, tags);
    EXPECT_EQ(triangles->size(), 4u);

    IxionRoomArenaProjectileHit(0, "test-wall"); // untagged wall: nothing
    IxionRoomArenaProjectileHit(kIxionArenaPartsTag | 1, "test");
    EXPECT_EQ(IxionRoomArenaStatesForTest()[1].state, 2);
    IxionRoomArenaActiveCollision(triangles, tags);
    EXPECT_EQ(triangles->size(), 4u); // state 2 still collides

    const float player[3] = {0.0f, 1.558f, -576.6f};
    IxionRoomArenaFrame(100, false, true, player); // not FIELD: nothing steps
    EXPECT_EQ(IxionRoomArenaStatesForTest()[1].state, 2);
    IxionRoomArenaFrame(101, true, true, player);
    EXPECT_EQ(IxionRoomArenaStatesForTest()[1].state, 1);
    // mainloop.cpp runs the boundary before and after each frame: one DC1 step per frame number.
    const float time = IxionRoomArenaStatesForTest()[1].time;
    IxionRoomArenaFrame(101, true, true, player);
    EXPECT_FLOAT_EQ(IxionRoomArenaStatesForTest()[1].time, time);
    IxionRoomArenaActiveCollision(triangles, tags);
    ASSERT_EQ(triangles->size(), 2u);
    EXPECT_EQ((*tags)[0], kIxionArenaPartsTag | 0);

    // The walk-in trigger: spawned inside, leaves, returns.
    const float away[3] = {0.0f, 0.0f, -450.0f};
    EXPECT_FALSE(IxionRoomArenaFrame(102, true, true, away).event_fired);
    const auto fired = IxionRoomArenaFrame(103, true, true, player);
    EXPECT_TRUE(fired.event_fired);
    EXPECT_EQ(fired.event_script, 500);
    EXPECT_FALSE(IxionRoomArenaFrame(104, true, true, away).event_fired);
    EXPECT_FALSE(IxionRoomArenaFrame(105, true, true, player).event_fired);

    const float low[3] = {0.0f, -47.0f, -356.0f};
    EXPECT_TRUE(IxionRoomArenaFrame(106, true, true, low).fell);

    IxionRoomArenaRetire("test");
    EXPECT_FALSE(IxionRoomArenaInstalled());
    IxionRoomArenaActiveCollision(triangles, tags);
    EXPECT_TRUE(triangles->empty());
    IxionRoomArenaProjectileHit(kIxionArenaPartsTag | 0, "after-retire");
    // Reinstall resets every slot (DC1 resets on every floor load).
    IxionRoomArenaInstall();
    EXPECT_EQ(IxionRoomArenaStatesForTest()[0].state, 3);
    EXPECT_EQ(IxionRoomArenaStatesForTest()[1].state, 3);
    IxionRoomArenaDiscard();
    EXPECT_FALSE(IxionRoomArenaInstalled());
}

// DRAN-R2: event 510 (Dran's death) sets the boss flag to 100; prog100 then never switches 500 on.
TEST_F(ArenaFixture, DranDeathBossFlagKeepsTheEvent500TriggerOff) {
    IxionRoomArenaAdoptForTest(MakeArena(2));
    EXPECT_EQ(IxionRoomArenaBossFlag(), 0);
    IxionRoomArenaInstall();
    EXPECT_TRUE(IxionRoomArenaTriggerForTest().switched_on);
    IxionRoomArenaSetBossFlag(100);
    EXPECT_FALSE(IxionRoomArenaTriggerForTest().switched_on);
    IxionRoomArenaRetire("test-reinstall");
    IxionRoomArenaInstall(); // a later installation of the same floor
    EXPECT_FALSE(IxionRoomArenaTriggerForTest().switched_on);
    EXPECT_EQ(IxionRoomArenaBossFlag(), 100);
    IxionRoomArenaResetForTest();
    EXPECT_EQ(IxionRoomArenaBossFlag(), 0);
}

TEST_F(ArenaFixture, FallRespawnUsesTracedEventFiveOnlyWhenSupported) {
    IxionRoomArenaAdoptForTest(MakeArena(1));
    IxionRoomArenaInstall();
    IxionRoom room;
    room.spawn = {0.0f, 1.558f, -576.6f};
    float out[3];
    EXPECT_FALSE(IxionRoomArenaFallRespawn(room, out)); // no floor at event 5's point: spawn
    EXPECT_FLOAT_EQ(out[2], -576.6f);
    IxionRoomTriangle floor;
    floor.vertices = {{{-50.0f, 0.0f, -520.0f}, {50.0f, 0.0f, -520.0f}, {0.0f, 0.0f, -440.0f}}};
    room.collision.push_back(floor);
    EXPECT_TRUE(IxionRoomArenaFallRespawn(room, out));
    EXPECT_NEAR(out[2], -309.0f * 1.5584790682688f, 1e-3f);
    EXPECT_EQ(IxionRoomArenaFallLife(), -40);
}

// IXION_ROOM_ARENA_TEST_PACK=<dir with room.json and arena.json> (private converter output).
TEST(IxionArenaPack, LoadsTheConvertedDranArena) {
    const char *pack = std::getenv("IXION_ROOM_ARENA_TEST_PACK");
    if (pack == nullptr || *pack == '\0') GTEST_SKIP() << "IXION_ROOM_ARENA_TEST_PACK not set";
    const std::filesystem::path dir(pack);
    IxionRoom room;
    std::string error;
    ASSERT_TRUE(IxionRoom::Load(dir / "room.json", room, error)) << error;
    IxionArena arena;
    ASSERT_TRUE(IxionArena::Load(dir / "arena.json", room.textures.size(), arena, error)) << error;
    std::ifstream stream(dir / "room.json", std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    EXPECT_EQ(Sha(bytes), arena.room_json_sha256);
    EXPECT_EQ(arena.room, room.donor_room);
    ASSERT_EQ(arena.tiles.size(), 12u);
    size_t triangles = 0;
    for (const auto &tile : arena.tiles) {
        triangles += tile.collision.size();
        EXPECT_FLOAT_EQ(tile.speed, 0.6f);
        EXPECT_FLOAT_EQ(tile.done_at, 59.0f);
        for (const auto &object : tile.objects) EXPECT_EQ(object.track_t.size(), 60u);
    }
    EXPECT_EQ(triangles, 509u);
    EXPECT_EQ(arena.fires.size(), 6u);
    EXPECT_EQ(arena.camera.size(), 188u + 130u + 8u);
    ASSERT_EQ(arena.markers.size(), 2u);
    EXPECT_EQ(arena.markers[0].script, 500);
    EXPECT_TRUE(arena.markers[0].auto_start);
    // The room spawn is the event01_2 marker: why the trigger arms only after the player leaves it.
    EXPECT_TRUE(IxionArenaInsideMarker(arena.markers[0], room.spawn.data()));
    // Event 5's respawn point stands on the room's own floor.
    IxionRoomArenaAdoptForTest(arena);
    IxionRoomArenaInstall();
    float out[3];
    EXPECT_TRUE(IxionRoomArenaFallRespawn(room, out));
    IxionRoomArenaResetForTest();
}
