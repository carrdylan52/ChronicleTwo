#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "ixion_room_atmos.hpp"
#include "ixion_room.hpp"
#include "map.hpp"
#include "dng_main.hpp"
#include "scene.hpp"
#include "platform_fixture.hpp"

namespace {

std::vector<uint8_t> MakePack(const std::vector<std::pair<std::string, std::string>> &members) {
    std::vector<uint8_t> out;
    for (const auto &[name, payload] : members) {
        const size_t step = (80 + payload.size() + 15) / 16 * 16;
        const size_t base = out.size();
        out.resize(base + step, 0);
        std::memcpy(out.data() + base, name.data(), name.size());
        const int32_t header[3] = {80, static_cast<int32_t>(payload.size()), static_cast<int32_t>(step)};
        std::memcpy(out.data() + base + 64, header, sizeof header);
        std::memcpy(out.data() + base + 80, payload.data(), payload.size());
    }
    out.resize(out.size() + 80, 0);
    return out;
}

std::vector<std::pair<std::string, std::string>> GoodMembers() {
    return {{"d01a_a.hd", "IECSsreV"}, {"d01a_a.bd", std::string(32, '\0')}, {"d01c_a.sq", "seq0"},
            {"d01a_a.sq", "seq1"},     {"bgm100.vol", "0\t40\nEND\n"},        {"bgm100.cfg", "0\tx\tx\t0\td01c_a.sq\rEND\r"}};
}

class AtmosFixture : public testing::Test {
protected:
    std::filesystem::path root;

    void SetUp() override {
        root = std::filesystem::temp_directory_path() /
               std::format("ct-ixion-atmos-{}-{}", testing::UnitTest::GetInstance()->current_test_info()->name(),
                           dc::test::ProcessId());
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
        WritePack(MakePack(GoodMembers()));
    }

    void TearDown() override { std::filesystem::remove_all(root); }

    void WritePack(const std::vector<uint8_t> &bytes) const {
        std::ofstream(root / "bgm.snd", std::ios::binary)
            .write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    static nlohmann::json Document() {
        nlohmann::json d;
        d["format"] = "ixion-room-atmosphere";
        d["version"] = 1;
        d["room"] = {{"donor_room", "d01inter"}, {"unit_scale", 1.5}};
        d["bgm"] = {{"pack", "bgm.snd"}, {"sequence", 0}};
        d["fog"] = {{"near", 300.0}, {"far", 1200.0}, {"color", {0, 0, 0}}, {"far_value", 128.0}, {"near_value", 255.0}};
        d["lights"] = {{{"slot", 2}, {"direction", {1, 0.6, 1}}, {"color", {120, 110, 100}}},
                       {{"slot", 3}, {"direction", {-1, -0.8, -1}}, {"color", {60, 40, 30}}}};
        d["ambient"] = {0, 0, 0};
        d["bg_color"] = {0, 0, 0};
        return d;
    }

    bool Load(const nlohmann::json &document, IxionRoomAtmosphere &out, std::string &error) const {
        std::ofstream(root / "atmosphere.json") << document.dump();
        return IxionRoomAtmosphere::Load(root / "atmosphere.json", out, error);
    }

    void ExpectRejected(const nlohmann::json &document, const char *needle) const {
        IxionRoomAtmosphere out;
        std::string error;
        EXPECT_FALSE(Load(document, out, error));
        EXPECT_NE(error.find(needle), std::string::npos) << error;
    }
};

TEST_F(AtmosFixture, LoadsSidecarAndPack) {
    IxionRoomAtmosphere out;
    std::string error;
    ASSERT_TRUE(Load(Document(), out, error)) << error;
    EXPECT_EQ(out.donor_room, "d01inter");
    EXPECT_TRUE(out.has_bgm);
    EXPECT_EQ(out.bgm_sequence_count, 2);
    EXPECT_EQ(out.bgm_sequence_name, "d01c_a.sq");
    EXPECT_EQ(out.bgm_pack.size() * 4, (out.bgm_pack_bytes + 3) / 4 * 4);
    EXPECT_FLOAT_EQ(out.fog_near, 300.0f);
    EXPECT_FLOAT_EQ(out.fog_far, 1200.0f);
    EXPECT_FLOAT_EQ(out.fog_far_value, 128.0f);
    EXPECT_FLOAT_EQ(out.fog_near_value, 255.0f);
    ASSERT_EQ(out.lights.size(), 2u);
    EXPECT_EQ(out.lights[0].slot, 2);
    const auto &d = out.lights[0].direction;
    EXPECT_NEAR(d[0] * d[0] + d[1] * d[1] + d[2] * d[2], 1.0f, 1e-5f);
    EXPECT_FLOAT_EQ(out.lights[0].color[0], 120.0f);
    EXPECT_TRUE(out.has_ambient);
    EXPECT_TRUE(out.has_bg_color);
}

TEST_F(AtmosFixture, BgmIsOptional) {
    auto document = Document();
    document.erase("bgm");
    IxionRoomAtmosphere out;
    std::string error;
    ASSERT_TRUE(Load(document, out, error)) << error;
    EXPECT_FALSE(out.has_bgm);
}

TEST_F(AtmosFixture, StaleDiscardRevokesLightingEvenWhenSceneAndMapAddressesRecur) {
    auto document = Document();
    document.erase("bgm");
    IxionRoomAtmosphere atmosphere;
    std::string error;
    ASSERT_TRUE(Load(document, atmosphere, error));
    IxionRoom room;
    room.donor_room = "d01inter";
    auto map = std::make_unique<CMap>();
    CMapLightingInfo base{};
    base.fog.near_dist = 17;
    map->map_info.active_light_no = 0;
    map->map_info.lighting_info_num = 1;
    map->map_info.lighting_info = &base;
    map->map_info.time_light_num = 0;
    int scene_token = 0;
    dc::test::SetEnv("DC2_IXION_ROOM_ATMOS", (root / "atmosphere.json").string().c_str(), 1);
    IxionRoomAtmosConfigure(root / "room.json", room);
    IxionRoomAtmosInstall(&scene_token, map.get());
    ASSERT_TRUE(IxionRoomAtmosActive());
    IxionRoomTestInstall state;
    state.scene = &scene_token;
    state.map = map.get();
    IxionRoomTestAdoptInstalled(state);
    IxionRoomBeforeLoopInit(); // production discard: scene token is deliberately not a CScene
    EXPECT_FALSE(IxionRoomAtmosActive());
    // Reuse the map address in a live native scene, before any new install.
    auto scene = std::make_unique<CScene>();
    auto *saved_scene = DngMainScene;
    DngMainScene = scene.get();
    scene->map_num = 1;
    scene->active_map = 0;
    ASSERT_EQ(scene->AssignMap(0, map.get(), const_cast<char *>("native")), 0);
    CMapLightingInfo native{};
    map->GetLightInfo(&native);
    EXPECT_FLOAT_EQ(native.fog.near_dist, 17);
    IxionRoomAtmosInstall(&scene_token, map.get());
    EXPECT_TRUE(IxionRoomAtmosActive());
    IxionRoomAtmosRetire(true);
    IxionRoomAtmosInstall(scene.get(), map.get());
    map->GetLightInfo(&native);
    EXPECT_FLOAT_EQ(native.fog.near_dist, 300); // new live owner actually receives donor lighting
    IxionRoomAtmosRetire(true);
    DngMainScene = saved_scene;
    dc::test::UnsetEnv("DC2_IXION_ROOM_ATMOS");
}

TEST_F(AtmosFixture, RejectsWrongFormatOrVersion) {
    auto document = Document();
    document["version"] = 2;
    ExpectRejected(document, "version 1");
    document = Document();
    document["format"] = "ixion-room";
    ExpectRejected(document, "version 1");
}

TEST_F(AtmosFixture, RejectsUnsafePackPath) {
    auto document = Document();
    document["bgm"]["pack"] = "../bgm.snd";
    ExpectRejected(document, "safe relative");
    document["bgm"]["pack"] = "C:/bgm.snd";
    ExpectRejected(document, "safe relative");
}

TEST_F(AtmosFixture, RejectsSequenceOutsidePack) {
    auto document = Document();
    document["bgm"]["sequence"] = 2;
    ExpectRejected(document, "outside");
    document["bgm"]["sequence"] = -1;
    ExpectRejected(document, "outside");
}

TEST_F(AtmosFixture, RejectsPackWithoutVol) {
    auto members = GoodMembers();
    members.erase(members.begin() + 4);
    WritePack(MakePack(members));
    ExpectRejected(Document(), ".vol");
}

TEST_F(AtmosFixture, RejectsTruncatedPack) {
    auto bytes = MakePack(GoodMembers());
    bytes.resize(bytes.size() - 120);
    WritePack(bytes);
    ExpectRejected(Document(), "bgm pack");
}

TEST_F(AtmosFixture, RejectsBadFog) {
    auto document = Document();
    document["fog"]["far"] = 100.0;
    ExpectRejected(document, "near < far");
    document = Document();
    document["fog"]["color"] = {0, 0, 300};
    ExpectRejected(document, "fog.color");
    document = Document();
    document["fog"]["near_value"] = -1.0;
    ExpectRejected(document, "near_value");
    document = Document();
    document.erase("fog");
    ExpectRejected(document, "fog missing");
}

TEST_F(AtmosFixture, RejectsBadLights) {
    auto document = Document();
    document["lights"][1]["slot"] = 2;
    ExpectRejected(document, "unique");
    document = Document();
    document["lights"][0]["slot"] = 5;
    ExpectRejected(document, "unique");
    document = Document();
    document["lights"][0]["direction"] = {0, 0, 0};
    ExpectRejected(document, "zero");
    document = Document();
    document["lights"][0]["color"] = {0, 256, 0};
    ExpectRejected(document, "light color");
}

TEST_F(AtmosFixture, RejectsInvalidJson) {
    std::ofstream(root / "atmosphere.json") << "{ not json";
    IxionRoomAtmosphere out;
    std::string error;
    EXPECT_FALSE(IxionRoomAtmosphere::Load(root / "atmosphere.json", out, error));
    EXPECT_NE(error.find("invalid atmosphere JSON"), std::string::npos) << error;
}

TEST(IxionRoomAtmosPack, CountsSequencesAndRejectsUnterminatedNames) {
    std::string error;
    std::vector<std::string> names;
    const auto good = MakePack(GoodMembers());
    EXPECT_EQ(IxionRoomAtmosValidateBgmPack(good.data(), good.size(), error, &names), 2) << error;
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[1], "d01a_a.sq");
    auto bad = good;
    std::memset(bad.data(), 'x', 64);
    EXPECT_EQ(IxionRoomAtmosValidateBgmPack(bad.data(), bad.size(), error), -1);
    EXPECT_NE(error.find("not terminated"), std::string::npos) << error;
}

} // namespace
