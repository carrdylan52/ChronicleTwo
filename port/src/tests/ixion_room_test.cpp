#include <array>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <memory>

#include "actionchara.hpp"
#include "actscript.hpp"
#include "automap.hpp"
#include "collision.hpp"
#include "colprim.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "effscript.hpp"
#include "dataread_port.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "mainloop.hpp"
#include "mainloop_port.hpp"
#include "platform/paths.hpp"
#include "padcontrol.hpp"
#include "savedata.hpp"
#include "stubs/event.hpp"
#include "gfx/gfx.hpp"
#include "ixion_room.hpp"
#include "ixion_room_props.hpp"
#include "ixion_dc1_monster.hpp"
#include "ixion_monica_faithful.hpp"
#include "ixion_room_arena.hpp"
extern CActiveMonster *nowMonster;
#include "dataread_port.hpp"
#include "mg_memory.hpp"
#include "mainloop.hpp"
#include "monster.hpp"
#include "platform/arena.hpp"
#include "platform_fixture.hpp"
#include "runscript.hpp"
#include "mglib.hpp"
#include "scene.hpp"
#include "userdata.hpp"

int _COLPRIM_CREATE(RS_STACKDATA *stack, int argc);
int _ES_CREATE(RS_STACKDATA *stack, int argc);
EFF_SPT_BASE_DEF *GetEffSptBaseDefPtr(int index);

namespace {

class RoomFixture : public testing::Test {
protected:
    std::filesystem::path root;

    void SetUp() override {
        // ctest runs each case as its own process, in parallel with -j, so a shared directory races.
        root = std::filesystem::temp_directory_path() /
               std::format("ct-ixion-room-fixture-{}-{}", testing::UnitTest::GetInstance()->current_test_info()->name(),
                           dc::test::ProcessId());
        std::filesystem::remove_all(root);
        std::filesystem::create_directories(root);
        const uint8_t pixel[] = {70, 110, 150, 255};
        ASSERT_TRUE(gfx::WritePng(root / "floor.png", pixel, 1, 1));
    }

    void TearDown() override { std::filesystem::remove_all(root); }

    nlohmann::json Document() const {
        const nlohmann::json vertex = {
            {"position", {0, 0, 0}}, {"normal", {0, 1, 0}}, {"uv", {0, 0}}, {"color", {1, 1, 1, 1}}};
        nlohmann::json document;
        document["format"] = "ixion-room";
        document["version"] = 1;
        document["donor"] = {{"game", "dc1"}, {"room", "d01inter"},
                              {"sources", {{{"path", "dun/d01inter.cfg"},
                                            {"sha256", std::string(64, 'a')}}}}};
        document["units"] = {{"scale", 1.0}};
        document["spawn"] = {1, 0, 1};
        document["enemy_spawn"] = {3, 0, 3};
        document["exit"] = {5, 0, 5};
        document["bounds"] = {{"min", {-10, -2, -10}}, {"max", {10, 10, 10}}};
        document["textures"] = {{{"name", "floor"}, {"file", "floor.png"}}};
        document["visuals"] = {{{"material", 0}, {"vertices", {vertex, vertex, vertex}}}};
        document["collision"] = {{{0, 0, 0}, {5, 0, 0}, {0, 0, 5}}};
        return document;
    }

    std::filesystem::path Write(const nlohmann::json &document) const {
        const auto path = root / "room.json";
        std::ofstream stream(path, std::ios::binary);
        stream << document.dump();
        return path;
    }
};

TEST_F(RoomFixture, LoadsOwnedGeometryAndPngAndConvertsCollision) {
    IxionRoom room;
    std::string error;
    ASSERT_TRUE(IxionRoom::Load(Write(Document()), room, error)) << error;
    EXPECT_EQ(room.donor_room, "d01inter");
    ASSERT_EQ(room.visuals.size(), 1u);
    EXPECT_EQ(room.visuals[0].vertices.size(), 3u);
    ASSERT_EQ(room.textures.size(), 1u);
    EXPECT_EQ(room.textures[0].width, 1u);
    EXPECT_EQ(room.textures[0].height, 1u);
    EXPECT_EQ(room.textures[0].rgba, (std::vector<uint8_t>{70, 110, 150, 255}));

    std::array<CCPoly, 2> polygons{};
    const float query_min[3] = {-1, -1, -1};
    const float query_max[3] = {1, 1, 1};
    EXPECT_EQ(IxionRoomCollision(room, query_min, query_max, polygons.data(), polygons.size()), 1);
    EXPECT_NEAR(polygons[0].normal[1], -1.0f, 1e-6f);
    EXPECT_FLOAT_EQ(polygons[0].vertex[0][3], 1.0f);
    const float distant_min[3] = {100, 100, 100};
    const float distant_max[3] = {101, 101, 101};
    EXPECT_EQ(IxionRoomCollision(room, distant_min, distant_max, polygons.data(), polygons.size()), 0);
}

TEST_F(RoomFixture, RejectsBadVersionAndLeavesPreviousOutputUntouched) {
    IxionRoom room;
    room.donor_room = "previous";
    std::string error;
    auto bad = Document();
    bad["version"] = 2;
    EXPECT_FALSE(IxionRoom::Load(Write(bad), room, error));
    EXPECT_EQ(room.donor_room, "previous");
    EXPECT_NE(error.find("unsupported"), std::string::npos);
}

TEST_F(RoomFixture, MissingOrWrongRequiredMembersRejectInReleaseAndPreserveOutput) {
    const std::vector<std::string> paths={
        "/format","/version","/donor","/donor/game","/donor/room","/donor/sources",
        "/donor/sources/0","/donor/sources/0/path","/donor/sources/0/sha256",
        "/units","/units/scale","/spawn","/enemy_spawn","/exit","/bounds",
        "/bounds/min","/bounds/max","/textures","/textures/0","/textures/0/name",
        "/textures/0/file","/visuals","/visuals/0","/visuals/0/material",
        "/visuals/0/vertices","/visuals/0/vertices/0","/visuals/0/vertices/0/position",
        "/visuals/0/vertices/0/normal","/visuals/0/vertices/0/uv",
        "/visuals/0/vertices/0/color","/collision","/collision/0",
        "/collision/0/0","/collision/0/1","/collision/0/2"};
    for (const std::string &path:paths) {
        for (bool wrong_type:{false,true}) {
            auto bad=Document();
            if (wrong_type) bad[nlohmann::json::json_pointer(path)]=nullptr;
            else {
                const size_t slash=path.rfind('/');
                const std::string leaf=path.substr(slash+1);
                nlohmann::json *parent=slash<=1?&bad:&bad[nlohmann::json::json_pointer(path.substr(0,slash))];
                if (parent->is_array()) parent->erase(static_cast<size_t>(std::stoul(leaf)));
                else parent->erase(leaf);
            }
            IxionRoom room; room.donor_room="previous"; room.unit_scale=17.0f;
            std::string error;
            EXPECT_FALSE(IxionRoom::Load(Write(bad),room,error))<<path<<" wrong_type="<<wrong_type;
            EXPECT_EQ(room.donor_room,"previous");
            EXPECT_FLOAT_EQ(room.unit_scale,17.0f);
            EXPECT_FALSE(error.empty());
        }
    }
    SUCCEED()<<"all "<<paths.size()*2<<" missing/wrong-type cases returned normally";
}

TEST_F(RoomFixture, RejectsTextureTraversalAndDegenerateCollision) {
    IxionRoom room;
    std::string error;
    auto traversal = Document();
    traversal["textures"][0]["file"] = "../floor.png";
    EXPECT_FALSE(IxionRoom::Load(Write(traversal), room, error));

    auto degenerate = Document();
    degenerate["collision"] = {{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}}};
    room.donor_room = "preserved";
    EXPECT_FALSE(IxionRoom::Load(Write(degenerate), room, error));
    EXPECT_EQ(room.donor_room, "preserved");
    EXPECT_NE(error.find("degenerate"), std::string::npos);
}

TEST_F(RoomFixture, RejectsOutOfBoundsVisualAndMissingPng) {
    IxionRoom room;
    std::string error;
    auto outside = Document();
    for (auto &vertex : outside["visuals"][0]["vertices"]) {
        vertex["position"][0] = 100;
    }
    EXPECT_FALSE(IxionRoom::Load(Write(outside), room, error));

    auto missing = Document();
    missing["textures"][0]["file"] = "missing.png";
    EXPECT_FALSE(IxionRoom::Load(Write(missing), room, error));
}

TEST(IxionRoomRealPack, LoadsSuppliedPrivateDonorPack) {
    const char *pack = std::getenv("IXION_ROOM_TEST_PACK");
    if (pack == nullptr || *pack == '\0') {
        GTEST_SKIP() << "IXION_ROOM_TEST_PACK was not set";
    }
    IxionRoom room;
    std::string error;
    ASSERT_TRUE(IxionRoom::Load(std::filesystem::path(pack), room, error)) << error;
    EXPECT_EQ(room.donor_room, "d01inter");
    EXPECT_EQ(room.textures.size(), 20u);
    EXPECT_EQ(room.visuals.size(), 382u);
    EXPECT_EQ(room.collision.size(), 3192u);
    // The supported R3 room indexes inside the ordinary generated 14x14 native grid.
    EXPECT_TRUE(IxionRoomBoundsFitNativeGrid(room.bounds_min, room.bounds_max, 14, 14, 320.0f, 320.0f));

    std::vector<CCPoly> polygons(128);
    const float min[3]={room.spawn[0]-40,room.spawn[1]-40,room.spawn[2]-40};
    const float max[3]={room.spawn[0]+40,room.spawn[1]+40,room.spawn[2]+40};
    const int count=IxionRoomCollision(room,min,max,polygons.data(),static_cast<int>(polygons.size()));
    EXPECT_GT(count, 0);
    EXPECT_GT(std::count_if(polygons.begin(), polygons.begin() + count,
                            [](const CCPoly &polygon) { return polygon.area_kind == COL_AREA_FLAT; }), 0);
    auto covers=[](const CCPoly &p,float x,float z,float ground_y) {
        const float *a=p.vertex[0],*b=p.vertex[1],*c=p.vertex[2];
        const float den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
        if (std::abs(den)<1e-7f) return false;
        const float u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/den;
        const float v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/den;
        const float w=1-u-v;
        const float y=u*a[1]+v*b[1]+w*c[1];
        return u>=-1e-5f&&v>=-1e-5f&&w>=-1e-5f&&std::abs(y-ground_y)<1e-2f&&p.normal[1]>=0.65f;
    };
    auto independent_ground=[](const IxionRoom &source,const std::array<float,3> &point) {
        for (const auto &triangle:source.collision) {
            const auto &a=triangle.vertices[0]; const auto &b=triangle.vertices[1]; const auto &c=triangle.vertices[2];
            const float den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2]);
            if (std::abs(den)<1e-8f) continue;
            const float u=((b[2]-c[2])*(point[0]-c[0])+(c[0]-b[0])*(point[2]-c[2]))/den;
            const float v=((c[2]-a[2])*(point[0]-c[0])+(a[0]-c[0])*(point[2]-c[2]))/den;
            const float w=1.0f-u-v;
            if (u<-1e-5f||v<-1e-5f||w<-1e-5f) continue;
            const float nx=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
            const float ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
            const float nz=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
            const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
            if (length>1e-5f&&ny/length>=0.65f&&std::abs(u*a[1]+v*b[1]+w*c[1]-point[1])<0.01f) return true;
        }
        return false;
    };
    EXPECT_TRUE(independent_ground(room,room.spawn)) << "independent full-pack oracle found no player-spawn floor";
    EXPECT_TRUE(independent_ground(room,room.enemy_spawn)) << "independent full-pack oracle found no enemy-spawn floor";
    EXPECT_TRUE(independent_ground(room,room.exit)) << "independent full-pack oracle found no exit-marker floor";
    EXPECT_TRUE(std::any_of(polygons.begin(),polygons.begin()+count,[&](const CCPoly &p){return covers(p,room.spawn[0],room.spawn[2],room.spawn[1]);}))
        <<"native-sized 128-result query omitted ground support at the authored player marker";
    for (const auto *marker:{&room.enemy_spawn,&room.exit}) {
        const float marker_min[3]={(*marker)[0]-40,(*marker)[1]-40,(*marker)[2]-40};
        const float marker_max[3]={(*marker)[0]+40,(*marker)[1]+40,(*marker)[2]+40};
        const int marker_count=IxionRoomCollision(room,marker_min,marker_max,polygons.data(),static_cast<int>(polygons.size()));
        ASSERT_GT(marker_count,0);
        EXPECT_LE(marker_count,128);
        EXPECT_TRUE(std::any_of(polygons.begin(),polygons.begin()+marker_count,[&](const CCPoly &p){
            return covers(p,(*marker)[0],(*marker)[2],(*marker)[1]);
        })) << "native-sized 128-result query omitted ground support at an authored enemy/exit marker";
    }

    // Select a real near-vertical obstacle nearest the authored player marker.
    const IxionRoomTriangle *wall=nullptr;
    float nearest_wall_distance=std::numeric_limits<float>::max();
    for (const auto &triangle:room.collision) {
        const auto &a=triangle.vertices[0]; const auto &b=triangle.vertices[1]; const auto &c=triangle.vertices[2];
        const float nx=(b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
        const float ny=(b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
        const float nz=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
        const float length=std::sqrt(nx*nx+ny*ny+nz*nz);
        if (length<=1e-5f||std::abs(ny/length)>=0.15f) continue;
        const float x=(a[0]+b[0]+c[0])/3.0f;
        const float y=(a[1]+b[1]+c[1])/3.0f;
        const float z=(a[2]+b[2]+c[2])/3.0f;
        if(y<room.spawn[1]-20.0f||y>room.spawn[1]+100.0f) continue;
        const float dx=x-room.spawn[0],dy=y-room.spawn[1],dz=z-room.spawn[2];
        const float distance=dx*dx+dy*dy+dz*dz;
        if(distance<nearest_wall_distance) { wall=&triangle; nearest_wall_distance=distance; }
    }
    ASSERT_NE(wall,nullptr);
    const std::array<float,3> wall_center={
        (wall->vertices[0][0]+wall->vertices[1][0]+wall->vertices[2][0])/3.0f,
        (wall->vertices[0][1]+wall->vertices[1][1]+wall->vertices[2][1])/3.0f,
        (wall->vertices[0][2]+wall->vertices[1][2]+wall->vertices[2][2])/3.0f};
    float wall_min[3],wall_max[3];
    for (int axis=0;axis<3;++axis) {
        wall_min[axis]=wall_max[axis]=wall->vertices[0][axis];
        for(int i=1;i<3;++i){wall_min[axis]=std::min(wall_min[axis],wall->vertices[i][axis]);wall_max[axis]=std::max(wall_max[axis],wall->vertices[i][axis]);}
        wall_min[axis]-=20.0f;wall_max[axis]+=20.0f;
    }
    const int wall_count=IxionRoomCollision(room,wall_min,wall_max,polygons.data(),static_cast<int>(polygons.size()));
    ASSERT_GT(wall_count,0);
    EXPECT_TRUE(std::any_of(polygons.begin(),polygons.begin()+wall_count,[&](const CCPoly &p){
        const float x=(p.vertex[0][0]+p.vertex[1][0]+p.vertex[2][0])/3.0f;
        const float y=(p.vertex[0][1]+p.vertex[1][1]+p.vertex[2][1])/3.0f;
        const float z=(p.vertex[0][2]+p.vertex[1][2]+p.vertex[2][2])/3.0f;
        const float dx=x-wall_center[0],dy=y-wall_center[1],dz=z-wall_center[2];
        return dx*dx+dy*dy+dz*dz<1e-4f&&std::abs(p.normal[1])<0.15f;
    }))<<"native-sized collision query dropped the exact real wall nearest the player marker";
    EXPECT_LE(count,128);
}

TEST(IxionRoomBackend, NeutralWhitePreservesTexelAndRgbAlphaModulateSeparately) {
    EXPECT_EQ(IxionRoomGsColor(1.0f),128);
    const std::array<uint8_t,4> texel={73,121,201,180};
    EXPECT_EQ(IxionRoomModulateTexel(texel,{1,1,1,1}),texel);
    const auto tinted=IxionRoomModulateTexel(texel,{0.5f,1,1,1});
    EXPECT_EQ(tinted,(std::array<uint8_t,4>{37,121,201,180}));
    const auto translucent=IxionRoomModulateTexel(texel,{1,1,1,0.5f});
    EXPECT_EQ(translucent,(std::array<uint8_t,4>{73,121,201,90}));
}

TEST(IxionRoomGpuTransaction, LaterTextureAndMeshFailureReleasePriorHandlesAndPublishNothing) {
    auto run=[&](int fail_at) {
        struct Fake { int next=1; int live=0; int attempt=0; } backend;
        std::vector<int> textures={91};
        std::vector<int> meshes={92};
        auto allocate=[&]() { if(backend.attempt++==fail_at) return 0; ++backend.live; return backend.next++; };
        const bool ok=IxionRoomStageGpuBundle<int,int>(3,3,
            [&](size_t){return allocate();},[&](size_t){return allocate();},
            [](int h){return h!=0;},[](int h){return h!=0;},
            [&](int){--backend.live;},[&](int){--backend.live;},textures,meshes);
        EXPECT_FALSE(ok);
        EXPECT_EQ(backend.live,0);
        EXPECT_EQ(textures,(std::vector<int>{91}));
        EXPECT_EQ(meshes,(std::vector<int>{92}));
    };
    run(1); // second texture allocation
    run(4); // later mesh allocation after every texture and one mesh succeeded
}

TEST(IxionRoomLifecycle, UnselectedFloorNeverInstalls) {
    // Selected 0:1; the live dungeon is 0:2, then dungeon 1 floor 1.
    const IxionRoomFloorIdentity other_floor{true, 0, 0, 2};
    const IxionRoomFloorIdentity other_dungeon{true, 1, 1, 1};
    const IxionRoomFloorIdentity not_in_dungeon{};
    for (const auto &current : {other_floor, other_dungeon, not_in_dungeon}) {
        EXPECT_FALSE(IxionRoomTargetMatches(0, 1, current));
        const auto decision = IxionRoomDecideBoundary({true, false, false, false, true});
        EXPECT_FALSE(decision.install);
        EXPECT_FALSE(decision.retire);
    }
    // Mismatched status/save identity (stage says 0, status says 1) is not the target either.
    EXPECT_FALSE(IxionRoomTargetMatches(0, 1, IxionRoomFloorIdentity{true, 1, 0, 1}));
    EXPECT_TRUE(IxionRoomTargetMatches(0, 1, IxionRoomFloorIdentity{true, 0, 0, 1}));
}

TEST(IxionRoomLifecycle, LeavingToAnotherFloorRetiresAndDoesNotReinstall) {
    const auto left = IxionRoomDecideBoundary({true, true, false, false, true});
    EXPECT_TRUE(left.retire);
    EXPECT_FALSE(left.install);
    // Native exit / scene change retires and never reinstalls in the same boundary.
    const auto exited = IxionRoomDecideBoundary({true, true, true, true, true});
    EXPECT_TRUE(exited.retire);
    EXPECT_FALSE(exited.install);
    // Still on the target with the same owner: keep it.
    const auto stay = IxionRoomDecideBoundary({true, true, false, true, true});
    EXPECT_FALSE(stay.retire);
    EXPECT_FALSE(stay.install);
    // On the target, not installed: install only when ready and enabled.
    EXPECT_TRUE(IxionRoomDecideBoundary({true, false, false, true, true}).install);
    EXPECT_FALSE(IxionRoomDecideBoundary({true, false, false, true, false}).install);
    EXPECT_FALSE(IxionRoomDecideBoundary({false, false, false, true, true}).install);
}

TEST(IxionRoomNativeGrid, UpperBoundsAreCheckedAndNegativeIndicesClamp) {
    int x = -1, z = -1;
    ASSERT_TRUE(IxionRoomNativeGridCell(-387.0f, -625.0f, 14, 14, 320.0f, 320.0f, x, z));
    EXPECT_EQ(x, 0);
    EXPECT_EQ(z, 0);
    ASSERT_TRUE(IxionRoomNativeGridCell(487.0f, 375.0f, 14, 14, 320.0f, 320.0f, x, z));
    EXPECT_EQ(x, 2);
    EXPECT_EQ(z, 1);
    // Reviewer trigger: z around 1,000,000 is native cell 3125, far past any grid.
    EXPECT_FALSE(IxionRoomNativeGridCell(0.0f, 1000000.0f, 14, 14, 320.0f, 320.0f, x, z));
    EXPECT_FALSE(IxionRoomNativeGridCell(0.0f, 1000000.0f, 30, 30, 320.0f, 320.0f, x, z));
    // Last cell edge: index 13 fits a 14-wide grid, index 14 does not.
    EXPECT_TRUE(IxionRoomNativeGridCell(13.0f * 320.0f, 0.0f, 14, 14, 320.0f, 320.0f, x, z));
    EXPECT_FALSE(IxionRoomNativeGridCell(14.0f * 320.0f, 0.0f, 14, 14, 320.0f, 320.0f, x, z));
    EXPECT_FALSE(IxionRoomNativeGridCell(0.0f, 0.0f, 0, 14, 320.0f, 320.0f, x, z));
    EXPECT_FALSE(IxionRoomNativeGridCell(0.0f, 0.0f, 14, 14, 0.0f, 320.0f, x, z));
}

TEST_F(RoomFixture, FarSpawnPackLoadsButIsRefusedByTheNativeGridGate) {
    // A structurally valid pack whose markers and geometry sit around z=1,000,000
    // inside small valid bounds: the loader accepts it, install must refuse it.
    auto document = Document();
    document["bounds"] = {{"min", {-10, -2, 999990}}, {"max", {10, 10, 1000010}}};
    document["spawn"] = {0, 0, 1000000};
    document["enemy_spawn"] = {1, 0, 1000000};
    document["exit"] = {2, 0, 1000000};
    const nlohmann::json vertex = {
        {"position", {0, 0, 1000000}}, {"normal", {0, 1, 0}}, {"uv", {0, 0}}, {"color", {1, 1, 1, 1}}};
    document["visuals"] = {{{"material", 0}, {"vertices", {vertex, vertex, vertex}}}};
    document["collision"] = {{{0, 0, 999995}, {5, 0, 999995}, {0, 0, 1000005}}};
    IxionRoom room;
    std::string error;
    ASSERT_TRUE(IxionRoom::Load(Write(document), room, error)) << error;
    EXPECT_FALSE(IxionRoomBoundsFitNativeGrid(room.bounds_min, room.bounds_max, 14, 14, 320.0f, 320.0f));
    EXPECT_FALSE(IxionRoomBoundsFitNativeGrid(room.bounds_min, room.bounds_max, 30, 30, 320.0f, 320.0f));
    ASSERT_TRUE(IxionRoom::Load(Write(Document()), room, error)) << error;
    EXPECT_TRUE(IxionRoomBoundsFitNativeGrid(room.bounds_min, room.bounds_max, 14, 14, 320.0f, 320.0f));
}

// Production-path lifecycle tests (R3). They adopt an installed state through
// the test seam and then drive the real RetireScene / discard / override code.
struct NativeWorld {
    std::unique_ptr<CScene, decltype(&std::free)> scene{static_cast<CScene *>(std::calloc(1, sizeof(CScene))),
                                                        &std::free};
    std::unique_ptr<CMonsterMan> manager = std::make_unique<CMonsterMan>();
    std::unique_ptr<CActiveMonster[]> monsters = std::make_unique<CActiveMonster[]>(2);
    CScene *saved_scene = DngMainScene;
    CMonsterMan *saved_manager = ActiveMonster;

    NativeWorld() {
        manager->scene = scene.get();
        for (auto &slot : manager->active) slot = nullptr;
        for (int i = 0; i < 2; ++i) {
            CActiveMonster &monster = monsters[i];
            monster.state = ACTIVE_MONSTER_LIVE;
            monster.life = monster.max_life = 25;
            monster.chara_kind = ACTION_KIND_SCRIPT;
            monster.catch_state = 0;
            monster.body_width = 10.0f;
            monster.body_height = 50.0f;
            // The actor array is not zeroed; the donor pause loop walks every damage[] primitive.
            for (ACTION_DAMAGE &damage : monster.damage) damage = ACTION_DAMAGE{};
            monster.next = nullptr; // SetMotion walks the part chain (actionchara.cpp:862)
            manager->active[i] = &monster;
        }
        DngMainScene = scene.get();
        ActiveMonster = manager.get();
    }
    ~NativeWorld() {
        IxionRoomRetire("test-teardown");
        DngMainScene = saved_scene;
        ActiveMonster = saved_manager;
    }
};

void Place(CActiveMonster &monster, float x, float y, float z) {
    float position[4] = {x, y, z, 1.0f};
    monster.SetPosition(position);
}

TEST(IxionRoomRetireOrder, RetireBeforeMarkerCommitRestoresTheNativeSavedPosition) {
    // N2: inspect + encounter 1, departure during the entry event: the
    // selected enemy is still at its native position and its marker is pending.
    NativeWorld world;
    CActiveMonster &enemy = world.monsters[0];
    Place(enemy, 1026.858f, 40.0f, 2994.308f);
    IxionRoomTestInstall state;
    state.scene = world.scene.get();
    state.enemy = &enemy;
    const float saved[4] = {1026.858f, 40.0f, 2994.308f, 1.0f};
    std::copy_n(saved, 4, state.enemy_saved_position);
    state.enemy_marker[0] = 93.509f;
    state.enemy_pending = true;
    IxionRoomTestAdoptInstalled(state);
    ASSERT_TRUE(IxionRoomInstalled());
    IxionRoomRetire("test-retire-before-commit");
    EXPECT_FALSE(IxionRoomInstalled());
    float actual[4] = {};
    enemy.GetPosition(actual);
    EXPECT_FLOAT_EQ(actual[0], saved[0]);
    EXPECT_FLOAT_EQ(actual[1], saved[1]);
    EXPECT_FLOAT_EQ(actual[2], saved[2]);
}

TEST(IxionRoomRetireOrder, StaleInstallationIsDiscardedWithoutTouchingOldActorsOrNewNavigation) {
    // N4: after LoopInit reset the arena, the old actor memory and navigation
    // flags belong to someone else; discard must not read or write either.
    for (int entry = 0; entry < 2; ++entry) {
        alignas(16) unsigned char poison[sizeof(CActiveMonster)];
        std::memset(poison, 0xCD, sizeof poison);
        auto *stale = reinterpret_cast<CActiveMonster *>(poison);
        const int saved_navi = AutoMapGen.navi_enable;
        const s16 saved_minimap = AutoMapGen.minimap_enable;
        AutoMapGen.navi_enable = 7;
        AutoMapGen.minimap_enable = 5;
        IxionRoomTestInstall state;
        state.scene = reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x1000));
        state.enemy = stale;
        state.parked = {stale};
        state.navigation_suspended = true;
        state.saved_navi_enable = 1;
        state.saved_minimap_enable = 1;
        IxionRoomTestAdoptInstalled(state);
        if (entry == 0) {
            IxionRoomBeforeLoopInit();
        } else {
            IxionRoomLoopEntry();
        }
        EXPECT_FALSE(IxionRoomInstalled());
        EXPECT_EQ(IxionRoomRuntimeEnemy(), nullptr);
        EXPECT_FALSE(IxionRoomMonsterParked(stale));
        EXPECT_EQ(AutoMapGen.navi_enable, 7);
        EXPECT_EQ(AutoMapGen.minimap_enable, 5);
        EXPECT_TRUE(std::all_of(std::begin(poison), std::end(poison), [](unsigned char b) { return b == 0xCD; }));
        AutoMapGen.navi_enable = saved_navi;
        AutoMapGen.minimap_enable = saved_minimap;
    }
}

// N1/N6/N7 production allocation paths: CEffectScriptMan::CreateEffSpt (port),
// _COLPRIM_CREATE / _ES_CREATE (port overrides, dispatched with the running
// script's operand stack), CColPrimMan::GetPrim and the ThinkHost window note.
struct EffectWorld {
    mgCMemory work;
    RS_PROG_HEADER program{};
    EFF_SPT_BASE base{};
    CEffectScriptMan effects;
    CEffectScriptMan *saved_fx = FxScriptMan;
    CEffectScriptMan *saved_es = EffScriptMan;
    CScene *saved_prim_scene = ColPrimMan.scene;
    std::vector<CColPrim> saved_prims{std::begin(ColPrimMan.prim), std::end(ColPrimMan.prim)};

    EffectWorld() {
        static char key;
        auto *buffer = reinterpret_cast<u_long128 *>(ArenaBlock(&key, 8192 * sizeof(u_long128)));
        work.SetHeapMem(buffer, 8192);
        std::memcpy(program.magic, "SB2", 3);
        base.base_no = 7;
        base.work_size = 161;
        base.script = reinterpret_cast<char *>(&program);
        effects.SetWorkBuffer(&work);
        effects.base[0] = &base;
        effects.base_num = 1;
        for (CColPrim &prim : ColPrimMan.prim) prim.active = false;
        FxScriptMan = &effects;
        EffScriptMan = &effects;
    }
    ~EffectWorld() {
        FxScriptMan = saved_fx;
        EffScriptMan = saved_es;
        ColPrimMan.scene = saved_prim_scene;
        std::copy(saved_prims.begin(), saved_prims.end(), ColPrimMan.prim);
    }
};

char *DamageName() {
    return Damage_Param_Table[0].name;
}

// _COLPRIM_CREATE as the effect interpreter dispatches it (args on the script's stack).
CColPrim *RunColprimCreate(_EFF_SCRIPT *script, int explicit_owner) {
    RS_STACKDATA *args = script->run.stack;
    args[0].type = RS_STR;
    args[0].val.s = DamageName();
    args[1].type = RS_INT;
    args[1].val.i = explicit_owner;
    EXPECT_EQ(_COLPRIM_CREATE(args, explicit_owner >= 0 ? 2 : 1), 1);
    return script->colprim;
}

// The ThinkHost attack-window allocation, line for line.
CColPrim *OpenWindow(CActiveMonster &monster, int owner) {
    CColPrim *prim = ColPrimMan.GetPrim();
    prim->SetDamage(DamageName(), owner);
    IxionRoomNoteMonsterPrim(prim, &monster);
    monster.damage[0].use = 1;
    monster.damage[0].prim = prim;
    return prim;
}

void AdoptWithSelected(NativeWorld &world, CActiveMonster &selected) {
    IxionRoomTestInstall state;
    state.scene = world.scene.get();
    state.enemy = &selected;
    state.enemy_saved_position[3] = 1.0f;
    IxionRoomTestAdoptInstalled(state);
}

TEST(IxionRoomSuspension, RetiredAndDiscardedClaimsCannotDeleteReusedNativeAllocations) {
    for (bool discard : {false, true}) {
        NativeWorld world;
        EffectWorld fx;
        AdoptWithSelected(world, world.monsters[0]);
        auto *old = fx.effects.CreateEffSpt(7, 25, 1);
        ASSERT_NE(old, nullptr);
        auto *prim = RunColprimCreate(old, 99);
        ASSERT_TRUE(IxionRoomPrimHeldByEffect(prim, old));
        if (discard) IxionRoomBeforeLoopInit();
        else IxionRoomRetire("reuse-regression");
        EXPECT_EQ(IxionRoomEffectSource(old), -1);
        EXPECT_FALSE(IxionRoomPrimHeldByEffect(prim, old));
        fx.effects.DeleteEffSpt(old);
        auto *native = fx.effects.CreateEffSpt(7, 24, 1);
        ASSERT_EQ(native, old); // actual arena reuse
        prim->active = false;
        auto *native_prim = RunColprimCreate(native, 99);
        ASSERT_EQ(native_prim, prim); // actual pool reuse while uninstalled
        IxionRoomNoteMonsterPrim(native_prim, &world.monsters[1]);
        EXPECT_FALSE(IxionRoomPrimHeldByMonster(native_prim, &world.monsters[1]));
        AdoptWithSelected(world, world.monsters[0]);
        IxionRoomEndOwnedAttacks(25, &world.monsters[1]);
        EXPECT_EQ(fx.effects.head, native);
        EXPECT_TRUE(native_prim->active);
    }
}

TEST(IxionRoomSuspension, NaturalEffectDeathRevokesClaimsBeforeArenaReuse) {
    NativeWorld world;
    EffectWorld fx;
    AdoptWithSelected(world, world.monsters[0]);
    auto *script = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(script, nullptr);
    auto *prim = RunColprimCreate(script, 25);
    fx.effects.DeleteEffSpt(script);
    EXPECT_EQ(IxionRoomEffectSource(script), -1);
    EXPECT_FALSE(IxionRoomPrimHeldByEffect(prim, script));
}

TEST(IxionRoomSuspension, NaturalEffectDeathEndsExplicitOtherOwnerAndDisplacedPrimitives) {
    NativeWorld world;
    EffectWorld fx;
    AdoptWithSelected(world, world.monsters[0]);
    auto *script = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(script, nullptr);
    auto *displaced = RunColprimCreate(script, 99);
    auto *current = RunColprimCreate(script, 99);
    ASSERT_NE(current, displaced);
    ASSERT_TRUE(displaced->active);
    ASSERT_TRUE(current->active);
    auto *other_script = fx.effects.CreateEffSpt(7, 24, 1);
    ASSERT_NE(other_script, nullptr);
    auto *unrelated = RunColprimCreate(other_script, 99);

    fx.effects.DeleteEffSpt(script);
    EXPECT_FALSE(displaced->active);
    EXPECT_FALSE(current->active);
    EXPECT_TRUE(unrelated->active);
    EXPECT_EQ(fx.effects.slot[25][0], nullptr);
    EXPECT_EQ(IxionRoomEffectSource(script), -1);
    EXPECT_FALSE(IxionRoomPrimHeldByEffect(displaced, script));
    EXPECT_FALSE(IxionRoomPrimHeldByEffect(current, script));

    // Cleanup after natural death cannot retain borrowers or touch another script.
    IxionRoomEndOwnedAttacks(25);
    EXPECT_FALSE(displaced->active);
    EXPECT_FALSE(current->active);
    EXPECT_TRUE(unrelated->active);
    EXPECT_EQ(fx.effects.head, other_script);
    fx.effects.DeleteEffSpt(other_script);
}

TEST(IxionRoomSuspension, NaturalEffectDeathEndsFrameBorrowerBeforeReleasingCharacterArenas) {
    NativeWorld world;
    EffectWorld fx;
    AdoptWithSelected(world, world.monsters[0]);
    auto *script = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(script, nullptr);
    const int frame_size = static_cast<int>((sizeof(mgCFrame) + 15) / 16);
    auto allocate_frame = [&](u_long128 *&token) {
        token = fx.work.StartStackMode(MG_STACK_MODE_FIT, frame_size + 4);
        if (token == nullptr) return static_cast<mgCFrame *>(nullptr);
        auto *frame = new (fx.work.Alloc(frame_size)) mgCFrame;
        fx.work.EndStackMode();
        return frame;
    };
    auto *start = allocate_frame(script->chara_work);
    auto *end = allocate_frame(script->sub_chara_work);
    ASSERT_NE(start, nullptr);
    ASSERT_NE(end, nullptr);
    auto *prim = RunColprimCreate(script, 99);
    prim->SetCoord(start, end, 10.0f);
    ASSERT_EQ(prim->coord_type, COLPRIM_COORD_FRAME);
    ASSERT_EQ(prim->frame[0], start);
    ASSERT_EQ(prim->frame[1], end);
    ASSERT_TRUE(prim->Step()); // real frame dereferences before death
    fx.effects.DeleteEffSpt(script);
    EXPECT_FALSE(prim->active);
    // Step must return before dereferencing either released arena.
    if (!prim->active) EXPECT_FALSE(prim->Step());
    IxionRoomEndOwnedAttacks(25);
    EXPECT_FALSE(prim->active);
}

TEST(IxionRoomSuspension, NaturalEffectDeathPreservesResetAndReusedPrimitiveIncarnations) {
    for (bool manager_reset : {false, true}) {
        NativeWorld world;
        EffectWorld fx;
        AdoptWithSelected(world, world.monsters[0]);
        auto *script = fx.effects.CreateEffSpt(7, 25, 1);
        ASSERT_NE(script, nullptr);
        auto *old_prim = RunColprimCreate(script, 25);
        if (manager_reset) ColPrimMan.Initialize(world.scene.get());
        else old_prim->Initialize();
        auto *reused = OpenWindow(world.monsters[0], 25); // same native owner is insufficient
        ASSERT_EQ(reused, old_prim);
        ASSERT_TRUE(reused->active);
        ASSERT_FALSE(IxionRoomPrimHeldByEffect(reused, script));
        ASSERT_TRUE(IxionRoomPrimHeldByMonster(reused, &world.monsters[0]));
        fx.effects.DeleteEffSpt(script);
        EXPECT_TRUE(reused->active);
        EXPECT_TRUE(IxionRoomPrimHeldByMonster(reused, &world.monsters[0]));
        IxionRoomEndOwnedAttacks(25);
        EXPECT_TRUE(reused->active);
        reused->Delete(-1);
    }
}

TEST(IxionRoomRound2, NativeNaturalEffectDeathKeepsOwnerCheckedDeletionWithoutAdapterClaims) {
    NativeWorld world;
    EffectWorld fx;
    IxionRoomClearAttackClaims();
    auto *script = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(script, nullptr);
    auto *prim = RunColprimCreate(script, 99);
    ASSERT_EQ(IxionRoomEffectSource(script), -1);
    fx.effects.DeleteEffSpt(script);
    EXPECT_TRUE(prim->active); // unchanged native owner check
    prim->Delete(-1);
}

TEST(IxionRoomSuspension, PrimitiveResetAndMonsterIncarnationRevokeHolderValidity) {
    NativeWorld world;
    EffectWorld fx;
    auto &monster = world.monsters[1];
    AdoptWithSelected(world, world.monsters[0]);
    auto *prim = OpenWindow(monster, 25);
    ASSERT_TRUE(IxionRoomPrimHeldByMonster(prim, &monster));
    IxionRoomBeforeMonsterSpawn(&monster);
    EXPECT_FALSE(IxionRoomPrimHeldByMonster(prim, &monster));
    // EndOwnedAttacks must not mistake this native incarnation for the old holder.
    monster.damage[0].prim = nullptr;
    IxionRoomEndOwnedAttacks(25, &monster);
    EXPECT_TRUE(prim->active);
    IxionRoomNoteMonsterPrim(prim, &monster);
    ASSERT_TRUE(IxionRoomPrimHeldByMonster(prim, &monster));
    ColPrimMan.Initialize(world.scene.get());
    EXPECT_FALSE(IxionRoomPrimHeldByMonster(prim, &monster));
    auto *effect = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(effect, nullptr);
    EXPECT_EQ(IxionRoomEffectSource(effect), 25);
    fx.effects.Initialize(nullptr, 0, 0);
    EXPECT_EQ(IxionRoomEffectOriginCountForTest(), 0u);
}

TEST(IxionRoomSuspension, ParkedOwnerEffectScriptsAndPrimitivesEnd) {
    // N1 through production suspension: the parked monster's attack window, its
    // effect script and that script's explicit-other-owner primitive end; the
    // selected enemy's effect and primitive and the player's window survive.
    NativeWorld world;
    EffectWorld fx;
    CActiveMonster &selected = world.monsters[0];
    CActiveMonster &parked = world.monsters[1];
    selected.chara_type = 24;
    parked.chara_type = 25;
    AdoptWithSelected(world, selected);
    CColPrim *player = ColPrimMan.GetPrim();
    player->SetDamage(DamageName(), 0);
    CColPrim *window = OpenWindow(parked, 25);
    _EFF_SCRIPT *parked_script = fx.effects.CreateEffSpt(7, 25, 1);
    _EFF_SCRIPT *selected_script = fx.effects.CreateEffSpt(7, 24, 1);
    CColPrim *parked_prim = RunColprimCreate(parked_script, 99);
    CColPrim *selected_prim = RunColprimCreate(selected_script, -1);
    ASSERT_EQ(parked_prim->owner, 99);
    AdoptWithSelected(world, selected);
    ASSERT_TRUE(IxionRoomMonsterSuspended(&parked));
    EXPECT_FALSE(window->active);
    EXPECT_EQ(parked.damage[0].prim, nullptr);
    EXPECT_FALSE(parked_prim->active);
    EXPECT_EQ(fx.effects.slot[25][0], nullptr);
    EXPECT_EQ(fx.effects.slot[24][0], selected_script);
    EXPECT_EQ(fx.effects.head, selected_script);
    EXPECT_TRUE(selected_prim->active);
    EXPECT_TRUE(player->active);
}

TEST(IxionRoomSuspension, RetaggedEffectEndsWithItsSourceAndKeepsUnrelatedRegistrations) {
    // N6: monster 25's registered effect is retagged to user_id 24 by the
    // production _ESM_SET_USER_ID call, then creates an attack (owner 24) and a
    // child filed under 24. Parking 25 must remove slot[25][0], keep the
    // unrelated slot[24][0], and cancel every outgoing primitive.
    NativeWorld world;
    EffectWorld fx;
    CActiveMonster &selected = world.monsters[0];
    CActiveMonster &parked = world.monsters[1];
    selected.chara_type = 24;
    parked.chara_type = 25;
    AdoptWithSelected(world, selected);
    _EFF_SCRIPT *unrelated = fx.effects.CreateEffSpt(7, 24, 1);
    _EFF_SCRIPT *retagged = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_EQ(fx.effects.slot[24][0], unrelated);
    ASSERT_EQ(fx.effects.slot[25][0], retagged);
    CColPrim *unrelated_prim = RunColprimCreate(unrelated, -1);
    CColPrim *first = RunColprimCreate(retagged, -1);
    ASSERT_EQ(fx.effects.SetScriptUserId(24, 25, 0), 1);
    ASSERT_EQ(retagged->user_id, 24);
    CColPrim *outgoing = RunColprimCreate(retagged, -1);
    ASSERT_NE(outgoing, first);
    ASSERT_TRUE(first->active) << "the owner-checked native replace leaves the owner-25 primitive live";
    ASSERT_EQ(outgoing->owner, 24);
    RS_STACKDATA *args = retagged->run.stack;
    args[0].type = RS_STR;
    args[0].val.s = GetEffSptBaseDefPtr(7)->name;
    ASSERT_EQ(_ES_CREATE(args, 1), 1);
    _EFF_SCRIPT *child = fx.effects.now;
    ASSERT_NE(child, nullptr);
    ASSERT_NE(child, retagged);
    EXPECT_EQ(child->user_id, 24);
    EXPECT_EQ(IxionRoomEffectSource(child), 25);
    EXPECT_EQ(IxionRoomEffectSource(retagged), 25);
    AdoptWithSelected(world, selected);
    ASSERT_TRUE(IxionRoomMonsterSuspended(&parked));
    EXPECT_EQ(fx.effects.slot[25][0], nullptr) << "original registration removed";
    EXPECT_EQ(fx.effects.slot[24][0], unrelated) << "unrelated registration preserved";
    EXPECT_EQ(fx.effects.head, unrelated);
    EXPECT_EQ(unrelated->next, nullptr) << "retagged script and its child ended";
    EXPECT_FALSE(first->active);
    EXPECT_FALSE(outgoing->active);
    EXPECT_TRUE(unrelated_prim->active);
    EXPECT_EQ(unrelated->colprim, unrelated_prim);
}

TEST(IxionRoomSuspension, ReusedPrimitiveSlotSurvivesTheOldScriptsSuspension) {
    // N7: a parked owner's script still points at primitive N after the
    // primitive-only reset (_DNG_COLLISION_ALL_CLR = ColPrimMan.Initialize);
    // the player then gets N. Parking must not cancel the player's or the
    // selected enemy's current attacks, while a genuinely current
    // explicit-other-owner primitive of the parked source still ends.
    NativeWorld world;
    EffectWorld fx;
    CActiveMonster &selected = world.monsters[0];
    CActiveMonster &parked = world.monsters[1];
    selected.chara_type = 24;
    parked.chara_type = 25;
    AdoptWithSelected(world, selected);
    _EFF_SCRIPT *old_script = fx.effects.CreateEffSpt(7, 25, 1);
    CColPrim *stale = RunColprimCreate(old_script, -1);
    ColPrimMan.Initialize(world.scene.get());
    ASSERT_FALSE(stale->active);
    ASSERT_EQ(old_script->colprim, stale);
    CColPrim *player = ColPrimMan.GetPrim();
    ASSERT_EQ(player, stale) << "the pool hands the same slot out again";
    player->SetDamage(DamageName(), 0);
    CColPrim *enemy_window = OpenWindow(selected, 24);
    _EFF_SCRIPT *selected_script = fx.effects.CreateEffSpt(7, 24, 1);
    CColPrim *selected_explicit = RunColprimCreate(selected_script, 25);
    _EFF_SCRIPT *parked_explicit = fx.effects.CreateEffSpt(7, 25, 1);
    CColPrim *parked_prim = RunColprimCreate(parked_explicit, 99);
    CColPrim *parked_window = OpenWindow(parked, 25);
    AdoptWithSelected(world, selected);
    ASSERT_TRUE(IxionRoomMonsterSuspended(&parked));
    EXPECT_TRUE(player->active);
    EXPECT_EQ(player->owner, 0);
    EXPECT_TRUE(enemy_window->active);
    EXPECT_TRUE(selected_explicit->active) << "owner 25 but held by the selected enemy's script";
    EXPECT_FALSE(parked_prim->active);
    EXPECT_FALSE(parked_window->active);
    EXPECT_EQ(fx.effects.head, selected_script);
    EXPECT_EQ(selected_script->next, nullptr);
    selected.damage[0].use = 0;
    selected.damage[0].prim = nullptr;
}

TEST(IxionRoomLifetime, InLoopMonsterResetRetiresBeforeTheActorsAreReinitialized) {
    // N8: debug enemy reload/reset and event _DEL_MONSTER all reach
    // CMonsterMan::Initialize inside a running dungeon with unchanged scene and
    // map pointers. The room retires first; no saved entry, exemption or
    // parked pointer survives into the new incarnation.
    auto manager = std::make_unique<CMonsterMan>();
    auto monsters = std::make_unique<CActiveMonster[]>(MONSTER_ACTIVE_MAX);
    std::unique_ptr<CScene, decltype(&std::free)> scene(static_cast<CScene *>(std::calloc(1, sizeof(CScene))),
                                                        &std::free);
    scene->chara_num = 2 * MONSTER_ACTIVE_MAX;
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        scene->chara[i + MONSTER_ACTIVE_MAX].status = 1;
        scene->chara[i + MONSTER_ACTIVE_MAX].chara = &monsters[i];
        monsters[i].state = ACTIVE_MONSTER_LIVE;
        monsters[i].life = monsters[i].max_life = 25;
        manager->active[i] = &monsters[i];
    }
    manager->scene = scene.get();
    CEffectScriptMan effects;
    CScene *saved_scene = DngMainScene;
    CMonsterMan *saved_manager = ActiveMonster;
    CEffectScriptMan *saved_effects = FxScriptMan;
    DngMainScene = scene.get();
    ActiveMonster = manager.get();
    FxScriptMan = &effects;
    IxionRoomTestInstall state;
    state.scene = scene.get();
    state.enemy = &monsters[0];
    state.enemy_saved_position[3] = 1.0f;
    state.parked = {&monsters[1]};
    IxionRoomTestAdoptInstalled(state);
    ASSERT_TRUE(IxionRoomMonsterParked(&monsters[1]));
    manager->Initialize(scene.get());
    EXPECT_FALSE(IxionRoomInstalled());
    EXPECT_EQ(IxionRoomRuntimeEnemy(), nullptr);
    EXPECT_FALSE(IxionRoomMonsterParked(&monsters[1]));
    EXPECT_FALSE(IxionRoomMonsterSuspended(&monsters[1])) << "no fresh parking after retirement";
    EXPECT_EQ(DngMainScene, scene.get());
    EXPECT_EQ(manager->scene, scene.get());
    EXPECT_EQ(manager->active[0], &monsters[0]) << "same scene character addresses, new incarnation";
    DngMainScene = saved_scene;
    ActiveMonster = saved_manager;
    FxScriptMan = saved_effects;
}

TEST(IxionRoomTreasure, OverlappingBoxRefusesInstallationAndIsNeverParked) {
    // N9: boxes are never parked; one whose 40-unit pickup radius reaches the
    // donor bounds refuses the install (bounds are the R3 donor room's).
    auto boxes = std::make_unique<CTreasureBoxManager>();
    boxes->Initialize();
    const std::array<float, 3> low{-387.4f, -60.0f, -625.0f};
    const std::array<float, 3> high{487.1f, 60.0f, 375.6f};
    float far_box[4] = {731.1f, 40.0f, 3373.8f, 1.0f};
    boxes->box[1].state = 1;
    boxes->box[1].SetPosition(far_box);
    EXPECT_EQ(IxionRoomOverlappingTreasureBox(boxes.get(), low, high), -1);
    float just_outside[4] = {high[0] + 41.0f, 40.0f, 0.0f, 1.0f};
    boxes->box[3].state = 1;
    boxes->box[3].SetPosition(just_outside);
    EXPECT_EQ(IxionRoomOverlappingTreasureBox(boxes.get(), low, high), -1);
    float reaching[4] = {high[0] + 39.0f, 40.0f, 0.0f, 1.0f};
    boxes->box[2].state = 1;
    boxes->box[2].SetPosition(reaching);
    EXPECT_EQ(IxionRoomOverlappingTreasureBox(boxes.get(), low, high), 2);
    EXPECT_EQ(boxes->box[2].state, 1) << "the slot keeps its native state";
}

TEST(IxionRoomSuspension, ParkedMonsterDoesNotPushTheSelectedEnemy) {
    // F4: CMonsterMan::CollisionCheck (port override) skips parked bodies.
    NativeWorld world;
    CActiveMonster &selected = world.monsters[0];
    CActiveMonster &parked = world.monsters[1];
    Place(selected, 0.0f, 0.0f, 0.0f);
    Place(parked, 15.0f, 0.0f, 0.0f);
    float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float move[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    float push[4] = {};
    world.manager->CollisionCheck(&selected, pos, move, push);
    EXPECT_GT(std::fabs(push[0] - move[0]), 1.0f) << "native: the other body pushes";
    IxionRoomTestInstall state;
    state.scene = world.scene.get();
    state.enemy = &selected;
    state.enemy_saved_position[3] = 1.0f;
    state.parked = {&parked};
    IxionRoomTestAdoptInstalled(state);
    world.manager->CollisionCheck(&selected, pos, move, push);
    EXPECT_FLOAT_EQ(push[0], move[0]);
    EXPECT_FLOAT_EQ(push[1], move[1]);
    EXPECT_FLOAT_EQ(push[2], move[2]);
    IxionRoomRetire("test-push");
    world.manager->CollisionCheck(&selected, pos, move, push);
    EXPECT_GT(std::fabs(push[0] - move[0]), 1.0f) << "restored: native push again";
}

TEST(IxionRoomNavigation, SuspendedWallChangeForcesTheNativeRebuildOnRestore) {
    // N3: a door/wall change while UpdateNaviMap is suspended, then native play
    // resumes in the cached cell. The native cache alone would skip the rebuild.
    CAutoMapParts cells[3] = {};
    for (auto &cell : cells) {
        cell.parts_no = 0;
        cell.wall = 0;
        cell.navi = 0;
    }
    CAutoMapParts *saved_grid = AutoMapGen.grid;
    const s16 saved_w = AutoMapGen.grid_w, saved_h = AutoMapGen.grid_h;
    const float saved_cw = AutoMapGen.cell_w, saved_cd = AutoMapGen.cell_d;
    const int saved_enable = AutoMapGen.navi_enable, saved_depth = AutoMapGen.navi_depth;
    AutoMapGen.grid = cells;
    AutoMapGen.grid_w = 3;
    AutoMapGen.grid_h = 1;
    AutoMapGen.cell_w = AutoMapGen.cell_d = 100.0f;
    AutoMapGen.navi_enable = 1;
    float player[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float elsewhere[4] = {200.0f, 0.0f, 0.0f, 1.0f};
    AutoMapGen.UpdateNaviMap(elsewhere, 4);
    AutoMapGen.UpdateNaviMap(player, 4); // native cache now holds cell (0,0)
    ASSERT_EQ(cells[0].navi, 4);
    ASSERT_EQ(cells[1].navi, 3);
    ASSERT_EQ(cells[2].navi, 2);
    IxionRoomTestInstall state;
    state.scene = reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x2000));
    state.navigation_suspended = true;
    state.saved_navi_enable = 1;
    state.saved_navi_depth = 4;
    state.player_native_saved = true;
    std::copy_n(player, 4, state.player_saved_position);
    CScene *saved_scene = DngMainScene;
    DngMainScene = static_cast<CScene *>(state.scene);
    IxionRoomTestAdoptInstalled(state);
    AutoMapGen.navi_enable = 0; // suspended while installed
    cells[0].wall |= AUTOMAP_WALL_POS_X;
    cells[1].wall |= AUTOMAP_WALL_NEG_X;
    AutoMapGen.UpdateNaviMap(player, 4); // the retained door caller's rebuild is skipped
    EXPECT_EQ(cells[1].navi, 3);
    IxionRoomRetire("test-navigation");
    DngMainScene = saved_scene;
    EXPECT_EQ(AutoMapGen.navi_enable, 1);
    EXPECT_EQ(cells[0].navi, 4);
    EXPECT_EQ(cells[1].navi, 0) << "the closed wall must cut the route";
    EXPECT_EQ(cells[2].navi, 0);
    AutoMapGen.grid = saved_grid;
    AutoMapGen.grid_w = saved_w;
    AutoMapGen.grid_h = saved_h;
    AutoMapGen.cell_w = saved_cw;
    AutoMapGen.cell_d = saved_cd;
    AutoMapGen.navi_enable = saved_enable;
    AutoMapGen.navi_depth = saved_depth;
}

// R5 (N10/N11) fixtures: the dungeon globals production IsEventRun reads.
struct FieldGlobals {
    std::unique_ptr<CActionChara> player = std::make_unique<CActionChara>();
    CTreasureBoxManager boxes;
    COMMON_GAGE gauge{};
    CActionChara *saved_player = MainChara__2;
    DNG_BATTLE_AREA *saved_area = BattleAreaScene;
    CTreasureBoxManager *saved_boxes = TreasureBoxMan;
    COMMON_GAGE *saved_hp = BattleParamater.hp;
    int saved_chr = BattleParamater.chr_no;
    int saved_eye = DngStatus.eye_view;
    int saved_mode = DngStatus.mode;

    explicit FieldGlobals(CScene *scene) {
        boxes.Initialize();
        gauge.max = gauge.now = 100.0f;
        player->move_check.landed = 1;
        player->move_check.ground_poly.area_kind = 0;
        MainChara__2 = player.get();
        BattleAreaScene = &scene->battle_area;
        BattleAreaScene->script.event_no = -1;
        TreasureBoxMan = &boxes;
        BattleParamater.hp = &gauge;
        BattleParamater.chr_no = USER_CHARA_ROBO; // no NPC-point or healing branch
        DngStatus.eye_view = 1;                     // stop right after the dispatch and floor-clear check
        DngStatus.mode = DNG_STATUS_FIELD;
    }
    ~FieldGlobals() {
        IxionRoomRetire("test-teardown");
        MainChara__2 = saved_player;
        BattleAreaScene = saved_area;
        TreasureBoxMan = saved_boxes;
        BattleParamater.hp = saved_hp;
        BattleParamater.chr_no = saved_chr;
        DngStatus.eye_view = saved_eye;
        DngStatus.mode = saved_mode;
    }
};

IxionRoomTestInstall LiveFloorInstall(NativeWorld &world, CActiveMonster *selected, std::vector<CActiveMonster *> parked) {
    IxionRoomTestInstall state;
    state.scene = world.scene.get();
    state.enemy = selected;
    state.enemy_saved_position[3] = 1.0f;
    state.parked = std::move(parked);
    state.target_matches = true;
    state.bounds_min = {-387.4f, -60.0f, -625.0f}; // the R3 donor room's bounds
    state.bounds_max = {487.1f, 60.0f, 375.6f};
    return state;
}

TEST(IxionRoomEvents, ParkedOwnersQueuedRequestStaysQueuedThroughProductionIsEventRun) {
    // N10: a parked monster's request posted by _RUN_EVENT_SCRIPT before
    // installation (deferred natively while Max was not landed) must not
    // become the scene's event_no while the room owns the floor. It stays
    // queued and the native dispatcher consumes it after retirement.
    NativeWorld world;
    CActiveMonster &selected = world.monsters[0];
    CActiveMonster &parked = world.monsters[1];
    selected.event_no = -1;
    parked.event_no = DNG_EVENT_MONSTER_TALK;
    FieldGlobals field(world.scene.get());
    IxionRoomTestAdoptInstalled(LiveFloorInstall(world, &selected, {&parked}));
    ASSERT_TRUE(IxionRoomMonsterParked(&parked));
    IsEventRun();
    EXPECT_EQ(BattleAreaScene->script.event_no, -1) << "parked owner's request must not dispatch";
    EXPECT_EQ(parked.event_no, DNG_EVENT_MONSTER_TALK) << "deferred, not dropped";
    // The selected enemy's own request still dispatches natively; the
    // The scene VM start policy decides it (see the next test).
    selected.event_no = 2540;
    IsEventRun();
    EXPECT_EQ(BattleAreaScene->script.event_no, 2540);
    EXPECT_EQ(selected.event_no, -1);
    EXPECT_EQ(parked.event_no, DNG_EVENT_MONSTER_TALK);
    BattleAreaScene->script.event_no = -1;
    IxionRoomRetire("test-n10-restore");
    IsEventRun();
    EXPECT_EQ(BattleAreaScene->script.event_no, DNG_EVENT_MONSTER_TALK) << "restored monster's request runs natively";
    EXPECT_EQ(parked.event_no, -1);
}

TEST_F(RoomFixture, OwnedPropChestKeepsRoomWhileUnownedChestRetires) {
    NativeWorld world;
    FieldGlobals field(world.scene.get());
    auto *saved_boxes = world.scene->battle_area.treasure_box;
    world.scene->battle_area.treasure_box = &field.boxes;
    const char *old = std::getenv("DC2_IXION_ROOM");
    const std::string previous = old ? old : "";
    dc::test::SetEnv("DC2_IXION_ROOM", PathsDisplay(root/"room.json").c_str(), 1);
    IxionRoomPropsResetForTest();
    std::ofstream(root/"props.json") << R"({"format":"ixion-room-props","version":1,"room":"d01inter",
      "props":[{"id":"test-box","kind":"treasure_box","position":[0,0,0],"contents":{"dc1_item":195}}]})";
    IxionRoom room; room.donor_room="d01inter";
    room.bounds_min={-10,-10,-10}; room.bounds_max={10,10,10};
    IxionRoomTestAdoptInstalled(LiveFloorInstall(world, &world.monsters[0], {}));
    // Place while installed: the synchronous PutTreasureBox callback must not retire it.
    IxionRoomPropsInstall(room, {}, 0);
    EXPECT_TRUE(IxionRoomInstalled());
    EXPECT_TRUE(IxionRoomPropsOwnsSlot(&field.boxes, 0));
    CSaveData save{};
    auto *previous_save = DngSaveData;
    const auto previous_confirm = PadCtrl.btn[PAD_BTN_CONFIRM].value;
    DngSaveData = &save;
    save.SetBitFlag(SAVE_FLAG_GUIDE_MAGIC_SWORD, 1);
    DngStatus.eye_view = 0;
    field.player->menu_flag = 1;
    world.monsters[0].event_no = world.monsters[1].event_no = -1;
    world.monsters[0].target_dist = world.monsters[1].target_dist = 999.0f;
    PadCtrl.btn[PAD_BTN_CONFIRM].value = 1;
    IsEventRun();
    EXPECT_EQ(BattleAreaScene->script.event_no, DNG_EVENT_TREASURE_BOX);
    EXPECT_TRUE(IxionRoomInstalled());
    DngSaveData = previous_save;
    PadCtrl.btn[PAD_BTN_CONFIRM].value = previous_confirm;
    field.boxes.near_box=0;
    IxionRoomBeforeNativeEvent(DNG_EVENT_TREASURE_BOX);
    EXPECT_TRUE(IxionRoomInstalled());
    field.boxes.box[0].state=2; // opened boxes remain owned for retirement
    EXPECT_TRUE(IxionRoomPropsOwnsSlot(&field.boxes,0));
    field.boxes.near_box=1;
    IxionRoomBeforeNativeEvent(DNG_EVENT_TREASURE_BOX);
    EXPECT_FALSE(IxionRoomInstalled());
    EXPECT_EQ(field.boxes.box[0].state,TREASURE_BOX_STATE_NONE);
    EXPECT_FALSE(IxionRoomPropsOwnsSlot(&field.boxes,0));
    world.scene->battle_area.treasure_box=saved_boxes;
    IxionRoomPropsResetForTest();
    if (old) dc::test::SetEnv("DC2_IXION_ROOM",previous.c_str(),1);
    else dc::test::UnsetEnv("DC2_IXION_ROOM");
}

TEST(IxionRoomEvents, OnlyTheQualifiedNativeEventRunsWithTheRoomAndOthersRetireFirst) {
    // N10: pure policy test; RoomEventStart covers actual loaded VM paths.
    // Supported: the player's death/continue script 1200. Everything else
    // (selected-enemy requests, scene event_run, guides, floor clear, boxes,
    // map events) retires the room first and refuses this loop entry.
    for (int event : std::initializer_list<int>{DNG_EVENT_FLOOR_START, DNG_EVENT_TREASURE_BOX, DNG_EVENT_RANDOM_CIRCLE, DNG_EVENT_GEOSTONE,
                      DNG_EVENT_MONSTER_TALK, DNG_EVENT_FLOOR_CLEAR, DNG_EVENT_RANDOM_STONE, DNG_EVENT_LEVEL_UP_GUIDE,
                      DNG_EVENT_ROBO_BIKE_GUIDE, DNG_EVENT_MAGIC_SWORD_GUIDE, 0, 1, 7777}) {
        EXPECT_FALSE(IxionRoomNativeEventSupported(event)) << event;
    }
    EXPECT_TRUE(IxionRoomNativeEventSupported(1200));
    NativeWorld world;
    CActiveMonster &selected = world.monsters[0];
    FieldGlobals field(world.scene.get());
    float native_player[4] = {1026.0f, 40.0f, 2994.0f, 1.0f};
    IxionRoomTestInstall state = LiveFloorInstall(world, &selected, {&world.monsters[1]});
    state.player_native_saved = true;
    std::copy_n(native_player, 4, state.player_saved_position);
    IxionRoomTestAdoptInstalled(state);
    float room_spawn[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    MainChara__2->SetPosition(room_spawn);
    IxionRoomBeforeNativeEvent(1200);
    EXPECT_TRUE(IxionRoomInstalled()) << "the qualified death script keeps the room";
    IxionRoomBeforeNativeEvent(2540); // e.g. a selected-enemy or guide request
    EXPECT_FALSE(IxionRoomInstalled());
    EXPECT_TRUE(IxionRoomEventRefusalActive());
    float actual[4] = {};
    MainChara__2->GetPosition(actual);
    EXPECT_FLOAT_EQ(actual[0], native_player[0]) << "the native event runs at the native position";
    EXPECT_FLOAT_EQ(actual[2], native_player[2]);
    EXPECT_FALSE(IxionRoomMonsterParked(&world.monsters[1]));
    IxionRoomBeforeLoopInit();
    EXPECT_FALSE(IxionRoomEventRefusalActive()) << "the refusal lasts one dungeon-loop entry";
}

// A loaded SB2 program records the position seen by its first production
// external. This catches a guard that retires after the VM already ran.
struct RoomEventProgram {
    RS_PROG_HEADER header{};
    RS_PROGDATA entry{2540, offsetof(RoomEventProgram, function)};
    funcdata function{};
    vmcode_t code[7]{
        {RS_OP_PUSH_CONST, RS_CONST_INT, {EVENT_EXT_GET_CHARA_POS}},
        {RS_OP_PUSH_CONST, RS_CONST_INT, {0}},
        {RS_OP_LOAD_ADDR, 0, {RS_ADDR_GLOBAL}},
        {RS_OP_LOAD_ADDR, 1, {RS_ADDR_GLOBAL}},
        {RS_OP_LOAD_ADDR, 2, {RS_ADDR_GLOBAL}},
        {RS_OP_EXT, 5, {0}},
        {RS_OP_END, 0, {0}}
    };
    RoomEventProgram() {
        std::memcpy(header.magic, "SB2", 3);
        header.main = offsetof(RoomEventProgram, function);
        header.code = offsetof(RoomEventProgram, code);
        header.prog = offsetof(RoomEventProgram, entry);
        header.prog_num = 1;
        header.global_num = 3;
    }
};

struct RoomLoadProgram {
    RS_PROG_HEADER header{};
    RS_PROGDATA entry{1200, offsetof(RoomLoadProgram, function)};
    funcdata function{};
    vmcode_t code[5]{
        {RS_OP_PUSH_CONST, RS_CONST_INT, {EVENT_EXT_LOAD_SCRIPT}},
        {RS_OP_PUSH_CONST, RS_CONST_STR, {offsetof(RoomLoadProgram, name) - offsetof(RoomLoadProgram, code)}},
        {RS_OP_PUSH_CONST, RS_CONST_INT, {2540}},
        {RS_OP_EXT, 3, {0}},
        {RS_OP_END, 0, {0}}
    };
    char name[32] = "d01/next_1.stb";
    RoomLoadProgram() {
        std::memcpy(header.magic, "SB2", 3);
        header.main = offsetof(RoomLoadProgram, function);
        header.code = offsetof(RoomLoadProgram, code);
        header.prog = offsetof(RoomLoadProgram, entry);
        header.prog_num = 1;
    }
};

class RoomEventStart : public RoomFixture {
protected:
    int previous_loop = PortLoopNo;
    int previous_language = LanguageCode;
    int previous_dungeon = DngStatus.dungeon_no;
    int previous_omake = OmakeFlag;
    CScene *previous_event_scene = EventScene;
    mgCMemory previous_script_buffer = BuffScriptData;
    mgCMemory previous_event_buffer = BuffEventData[0];
    mgCMemory previous_main_buffer = *GetMainStack();
    decltype(EdEventInfo) previous_info = EdEventInfo;
    alignas(64) u_long128 storage[4096]{};
    alignas(64) u_long128 main_storage[4096]{};
    RoomEventProgram program;

    void SetUp() override {
        RoomFixture::SetUp();
        PortLoopNo = LOOP_DUNGEON;
        LanguageCode = 1;
        DngStatus.dungeon_no = 0;
        OmakeFlag = 0;
        EdEventInfo = {};
        EdEventInfo.event_no = -1;
        BuffScriptData.stSetBuffer(storage, 4096);
        GetMainStack()->stSetBuffer(main_storage, 4096);
    }
    void TearDown() override {
        SetEventScript(nullptr, nullptr, nullptr);
        BuffScriptData = previous_script_buffer;
        BuffEventData[0] = previous_event_buffer;
        *GetMainStack() = previous_main_buffer;
        EventScene = previous_event_scene;
        EdEventInfo = previous_info;
        PortLoopNo = previous_loop;
        LanguageCode = previous_language;
        DngStatus.dungeon_no = previous_dungeon;
        OmakeFlag = previous_omake;
        PathsReset();
        RoomFixture::TearDown();
    }
    void Install(NativeWorld &world, FieldGlobals &field) {
        world.scene->chara_num = 1;
        world.scene->chara[0].status = 1;
        world.scene->chara[0].chara = field.player.get();
        IxionRoomTestInstall state = LiveFloorInstall(world, &world.monsters[0], {&world.monsters[1]});
        state.player_native_saved = true;
        const float native[4] = {1026.0f, 40.0f, 2994.0f, 1.0f};
        std::copy_n(native, 4, state.player_saved_position);
        IxionRoomTestAdoptInstalled(state);
        float donor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        field.player->SetPosition(donor);
        InitEvent(world.scene.get());
    }
    void WriteProgram(const std::filesystem::path &relative) {
        auto path = root / relative;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char *>(&program), sizeof(program));
        file.close();
        DataReadIndex(root);
    }
    void ExpectNativeBeforeExecution(NativeWorld &world) {
        ASSERT_NE(EventScript.prog, nullptr);
        ASSERT_EQ(EventScript.prog->global_num, 3);
        EXPECT_FLOAT_EQ(EventScript.global[0].val.f, 1026.0f);
        EXPECT_FLOAT_EQ(EventScript.global[1].val.f, 40.0f);
        EXPECT_FLOAT_EQ(EventScript.global[2].val.f, 2994.0f);
        EXPECT_FALSE(IxionRoomInstalled());
        EXPECT_TRUE(IxionRoomEventRefusalActive());
        EXPECT_FALSE(IxionRoomMonsterParked(&world.monsters[1]));
    }
};

TEST_F(RoomEventStart, ProductionDebugCommandRetiresBeforeLoadedScriptExecutes) {
    NativeWorld world;
    FieldGlobals field(world.scene.get());
    WriteProgram("map/d/d01/d01_1.stb");
    Install(world, field);
    DBGCMD_RunScript(2540);
    ExpectNativeBeforeExecution(world);
}

TEST_F(RoomEventStart, ProductionLoadScriptContinuationRetiresBeforeLoadedScriptExecutes) {
    NativeWorld world;
    FieldGlobals field(world.scene.get());
    WriteProgram("map/d/d01/next_1.stb");
    Install(world, field);
    RoomLoadProgram loader;
    SetEventScript(reinterpret_cast<char *>(&loader), nullptr, &BuffScriptData);
    ASSERT_EQ(RunEvent(1200, world.scene.get()), 0);
    ASSERT_TRUE(IxionRoomInstalled()) << "qualified death script stays in donor coordinates";
    ASSERT_EQ(EdEventInfo.request, EVENT_REQUEST_LOAD_SCRIPT);
    EXPECT_EQ(EventLoop(), EVENT_REQUEST_LOAD_SCRIPT);
    ExpectNativeBeforeExecution(world);
}

TEST_F(RoomEventStart, ProductionRunEventPreservesQualifiedAndNonRoomExecution) {
    NativeWorld world;
    FieldGlobals field(world.scene.get());
    Install(world, field);
    program.entry.no = 1200;
    SetEventScript(reinterpret_cast<char *>(&program), nullptr, &BuffScriptData);
    EXPECT_EQ(RunEvent(1200, world.scene.get()), 0);
    EXPECT_TRUE(IxionRoomInstalled());
    EXPECT_FALSE(IxionRoomEventRefusalActive());
    EXPECT_FLOAT_EQ(EventScript.global[0].val.f, 0.0f);
    // RunEvent is also the two Sphida start sites' production boundary.
    program.entry.no = 2540;
    EXPECT_EQ(RunEvent(2540, world.scene.get()), 0);
    ExpectNativeBeforeExecution(world);
    IxionRoomBeforeLoopInit();
    EXPECT_FALSE(IxionRoomEventRefusalActive());
    EXPECT_EQ(RunEvent(2540, world.scene.get()), 0) << "native dungeon without a room";
    EXPECT_FALSE(IxionRoomEventRefusalActive());
    Install(world, field);
    PortLoopNo = LOOP_EDIT;
    EXPECT_EQ(RunEvent(2540, world.scene.get()), 0) << "native town";
    EXPECT_TRUE(IxionRoomInstalled()) << "non-dungeon starts do not apply the dungeon policy";
    EXPECT_FLOAT_EQ(EventScript.global[0].val.f, 0.0f);
    EXPECT_FALSE(IxionRoomEventRefusalActive());
}

TEST(IxionRoomEvents, BoxPostedInsideTheDonorBoundsRetiresTheRoomAndKeepsTheBox) {
    // N10: _POST_TREASURE_BOX and every other retained creation go through the
    // production CTreasureBoxManager::PutTreasureBox override.
    NativeWorld world;
    FieldGlobals field(world.scene.get());
    IxionRoomTestAdoptInstalled(LiveFloorInstall(world, &world.monsters[0], {&world.monsters[1]}));
    float far_box[4] = {731.1f, 40.0f, 3373.8f, 1.0f};
    field.boxes.PutTreasureBox(-1, far_box, 0.0f, 0x41, 5, 1, -1, 0);
    EXPECT_TRUE(IxionRoomInstalled()) << "a box outside the donor bounds is retained";
    float inside[4] = {10.0f, 0.0f, 10.0f, 1.0f};
    field.boxes.PutTreasureBox(-1, inside, 0.0f, 0x41, 6, 1, -1, 0);
    EXPECT_FALSE(IxionRoomInstalled());
    EXPECT_EQ(field.boxes.box[1].state, TREASURE_BOX_STATE_UNOPENED) << "boxes are never parked";
    EXPECT_EQ(field.boxes.box[1].item[0], 6);
}

TEST(IxionRoomLifetime, SelectedSlotReusedByProductionSpawnInheritsNoExemptionOrRestoration) {
    // N11: kill the selected enemy to NONE, then the production
    // CMonsterMan::SetActiveMonster (_AUTO_SET_MONSTER / _CREATE_MONSTER)
    // reuses that slot's scene character address with unchanged scene and
    // map pointers. The new incarnation gets no exemption and no old saved state.
    auto manager = std::make_unique<CMonsterMan>();
    auto monsters = std::make_unique<CActiveMonster[]>(MONSTER_ACTIVE_MAX);
    std::unique_ptr<CScene, decltype(&std::free)> scene(static_cast<CScene *>(std::calloc(1, sizeof(CScene))),
                                                        &std::free);
    scene->chara_num = 2 * MONSTER_ACTIVE_MAX;
    for (int i = 0; i < MONSTER_ACTIVE_MAX; ++i) {
        scene->chara[i + MONSTER_ACTIVE_MAX].status = 1;
        scene->chara[i + MONSTER_ACTIVE_MAX].chara = &monsters[i];
        monsters[i].state = ACTIVE_MONSTER_LIVE;
        monsters[i].life = monsters[i].max_life = 25;
        monsters[i].event_no = -1;
        manager->active[i] = &monsters[i];
    }
    manager->scene = scene.get();
    for (auto &refer : manager->refer) refer.id = -1;
    std::vector<u_long128> slot_memory(8192);
    manager->memory[0].stSetBuffer(slot_memory.data(), static_cast<int>(slot_memory.size()));
    // A loaded refer entry: a kind that shares its model (no frame copy) and an
    // empty SB2 program (no INIT entry runs).
    const BASE_MONSTER_TBL *kind = nullptr;
    for (const BASE_MONSTER_TBL *entry = base_monster_define; entry->name[0] != 0; ++entry) {
        if (entry->share_model != 0 && (kind == nullptr || (kind->boss && !entry->boss))) kind = entry;
    }
    ASSERT_NE(kind, nullptr);
    static mgCFrame frame;
    static RS_PROG_HEADER program{};
    std::memcpy(program.magic, "SB2", 3);
    manager->refer[0].id = kind->id;
    manager->refer[0].chara.CObjectFrame::frame = &frame;
    manager->refer[0].script = reinterpret_cast<char *>(&program);
    CEffectScriptMan effects;
    CScene *saved_scene = DngMainScene;
    CMonsterMan *saved_manager = ActiveMonster;
    CEffectScriptMan *saved_effects = FxScriptMan;
    DngMainScene = scene.get();
    ActiveMonster = manager.get();
    FxScriptMan = &effects;

    CActiveMonster &rat = monsters[0];
    float rat_native[4] = {1026.858f, 40.0f, 2994.308f, 1.0f};
    IxionRoomTestInstall state;
    state.scene = scene.get();
    state.enemy = &rat;
    std::copy_n(rat_native, 4, state.enemy_saved_position);
    state.parked = {&monsters[1]};
    IxionRoomTestAdoptInstalled(state);
    ASSERT_EQ(IxionRoomRuntimeEnemy(), &rat);
    ASSERT_FALSE(IxionRoomMonsterSuspended(&rat)) << "the selected incarnation is exempt";
    const uint64_t before = IxionRoomMonsterIncarnation(&rat);
    rat.life = 0;
    rat.state = ACTIVE_MONSTER_NONE; // killed, as r4-fightB run.log:183 (state=0 hp=0/25)

    float spawn[4] = {-40.0f, 0.0f, 25.0f, 1.0f};
    float rotation[4] = {0.0f, 0.5f, 0.0f, 1.0f};
    CActiveMonster *replacement = manager->SetActiveMonster(0, spawn, rotation, 0);
    ASSERT_EQ(replacement, &rat) << "the NONE slot's scene character address is reused";
    EXPECT_EQ(replacement->state, ACTIVE_MONSTER_LIVE);
    EXPECT_NE(IxionRoomMonsterIncarnation(&rat), before);
    EXPECT_TRUE(IxionRoomInstalled()) << "bounded spawn policy: park, not retire";
    EXPECT_EQ(IxionRoomRuntimeEnemy(), nullptr) << "no inherited selected-enemy exemption";
    EXPECT_TRUE(IxionRoomMonsterParked(replacement)) << "spawned while installed: parked at once";
    EXPECT_EQ(replacement->view_state, MONSTER_VIEW_OUT);
    EXPECT_NE(replacement->attrib & MONSTER_ATTRIB_NO_DAMAGE, 0u);
    EXPECT_TRUE(IxionRoomMonsterSuspended(replacement));

    IxionRoomRetire("test-n11");
    float actual[4] = {};
    replacement->GetPosition(actual);
    EXPECT_FLOAT_EQ(actual[0], spawn[0]) << "the old rat's saved native position is not applied";
    EXPECT_FLOAT_EQ(actual[2], spawn[2]);
    EXPECT_EQ(replacement->view_state, MONSTER_VIEW_INIT) << "its own spawn-time state is restored";
    EXPECT_EQ(replacement->attrib & MONSTER_ATTRIB_NO_DAMAGE, 0u);
    DngMainScene = saved_scene;
    ActiveMonster = saved_manager;
    FxScriptMan = saved_effects;
}

} // namespace

TEST(IxionDc1Selection, DonorRowsStayIndependentAndNativeRowsAreUntouched) {
    const char *previous = std::getenv("DC2_IXION_ROOM_DONOR_MONSTER");
    const std::string saved = previous != nullptr ? previous : "";
    auto manager = std::make_unique<CMonsterMan>();
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e104a", 1);
    ASSERT_TRUE(IxionDc1DonorRequested());
    const BASE_MONSTER_TBL opar = *manager->GetReferPtr2(0x7000);
    EXPECT_STREQ(opar.model, "ix_e104a");
    EXPECT_EQ(opar.life, 28);
    EXPECT_EQ(opar.defense, 1);
    EXPECT_EQ(opar.reward_money, 5); // Chronicle monstorunit.cpp:263
    const BASE_MONSTER_TBL *native = manager->GetReferPtr2(8);
    const BASE_MONSTER_TBL native_before = *native;
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e42a", 1);
    ASSERT_TRUE(IxionDc1DonorRequested());
    const BASE_MONSTER_TBL *ghost = manager->GetReferPtr2(0x7000);
    ASSERT_NE(ghost, nullptr);
    EXPECT_STREQ(ghost->model, "ix_e42a");
    EXPECT_STREQ(ghost->script, "ix_e42a");
    EXPECT_EQ(ghost->life, 15);
    EXPECT_EQ(ghost->defense, 0);
    EXPECT_EQ(ghost->reward_exp, 3);
    EXPECT_EQ(ghost->reward_money, 5);
    EXPECT_EQ(ghost->drop_item[0], 0);
    EXPECT_EQ(ghost->drop_item[1], 0);
    EXPECT_EQ(ghost->drop_item[2], 0);
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e07a", 1);
    ASSERT_TRUE(IxionDc1DonorRequested());
    const BASE_MONSTER_TBL *werewolf = manager->GetReferPtr2(0x7000);
    ASSERT_NE(werewolf, nullptr);
    EXPECT_STREQ(werewolf->model, "ix_e07a");
    EXPECT_EQ(werewolf->life, 180);
    EXPECT_EQ(werewolf->defense, 5);
    EXPECT_EQ(werewolf->reward_exp, 12);
    EXPECT_EQ(werewolf->reward_money, 15);
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e104a", 1);
    EXPECT_EQ(std::memcmp(manager->GetReferPtr2(0x7000), &opar, sizeof opar), 0);
    EXPECT_EQ(std::memcmp(native, &native_before, sizeof native_before), 0);
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "../e42a", 1);
    EXPECT_FALSE(IxionDc1DonorRequested());
    EXPECT_EQ(manager->GetReferPtr2(0x7000), nullptr);
    if (previous != nullptr) dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", saved.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_MONSTER");
}

TEST(IxionDc1Pause, DurableEventsAndPauseBitsExcludeUnrelatedDrawFlags) {
    EXPECT_FALSE(IxionDc1PauseRequested(DNG_STATUS_FIELD, false, 0));
    EXPECT_TRUE(IxionDc1PauseRequested(DNG_STATUS_EVENT, false, 0));
    EXPECT_TRUE(IxionDc1PauseRequested(DNG_STATUS_FIELD, true, 0));
    EXPECT_TRUE(IxionDc1PauseRequested(DNG_STATUS_MENU, false, 0));
    for (unsigned flag : {DNG_PAUSE_MONSTER_AI, DNG_PAUSE_ENEMY_STEP, DNG_PAUSE_PLAYER_STATUS,
                          DNG_PAUSE_PLAYER_STEP, DNG_PAUSE_PLAYER_CONTROL, DNG_PAUSE_FIELD_STEP})
        EXPECT_TRUE(IxionDc1PauseRequested(DNG_STATUS_FIELD, false, flag));
    EXPECT_FALSE(IxionDc1PauseRequested(DNG_STATUS_FIELD, false, DNG_PAUSE_MAP_DRAW | DNG_PAUSE_MINIMAP));
}

TEST(IxionDc1Pause, LivePrimitiveStaysAllocatedAndFrozenThenResumesWithoutNativeChanges) {
    NativeWorld world;
    CActiveMonster &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    CActiveMonster *saved_now = nowMonster;
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    const int saved_mode = DngStatus.mode;
    IxionDc1InstallNatives(&donor);
    CColPrimMan prims{};
    prims.Initialize(world.scene.get());
    DAMAGE_PARAM param{};
    auto &attack = prims.prim[0];
    attack.active = true;
    attack.owner = MONSTER_ACTIVE_MAX;
    attack.target = DAMAGE_TARGET_PLAYER;
    attack.param = &param;
    attack.life = 3;
    donor.damage[0].prim = &attack;
    auto &native = prims.prim[1];
    native.active = true;
    native.target = DAMAGE_TARGET_PLAYER;
    native.param = &param;
    native.life = -1;
    DngStatus.mode = DNG_STATUS_FIELD;
    world.scene->battle_area.script.running = true; // event request bit is deliberately absent
    prims.Step();
    EXPECT_TRUE(attack.active);
    EXPECT_EQ(attack.target, 0);
    EXPECT_EQ(attack.step_count, 0);
    EXPECT_NE(prims.GetPrim(), &attack); // setting active=false would let GetPrim steal it
    EXPECT_EQ(native.step_count, 1);
    EXPECT_EQ(native.target, DAMAGE_TARGET_PLAYER);
    CColPrim probe{};
    probe.id = 2;
    probe.attacker = 0;
    EXPECT_EQ(prims.IsReversVec(&probe), &native); // a frozen donor cannot be reflected either
    prims.Step();
    EXPECT_EQ(attack.step_count, 0);
    world.scene->battle_area.script.running = false;
    prims.Step();
    EXPECT_EQ(attack.target, DAMAGE_TARGET_PLAYER);
    EXPECT_EQ(attack.step_count, 1);
    EXPECT_EQ(native.step_count, 3);
    EXPECT_FALSE(IxionDc1PrimPaused(&attack));
    EXPECT_EQ(prims.IsReversVec(&probe), &attack);
    donor.damage[0].prim = nullptr;
    IxionDc1Forget(&donor);
    DngStatus.mode = saved_mode;
    MainChara__2 = saved_player;
    nowMonster = saved_now;
}

TEST(IxionDc1Werewolf, GuardUsesInclusiveTruncatedFramesAndOnlyTheDonor) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    donor.muteki_time = 0;
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    auto *saved_now = nowMonster;
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    RS_STACKDATA frames[2]{};
    frames[0].type = frames[1].type = RS_FLOAT;
    frames[0].val.f = 244.9f;
    frames[1].val.f = 263.9f;
    EXPECT_EQ(donor.mons_script.ext_func_table[244](frames, 2), 1);
    DAMAGE_PARAM param{};
    param.kind = DAMAGE_KIND_MAX_MELEE;
    CColPrim hit{};
    hit.param = &param;
    hit.damage = 8;
    donor.frame = 243.99f;
    EXPECT_FALSE(IxionDc1GuardHit(&donor, &hit));
    donor.frame = 244.0f;
    EXPECT_TRUE(IxionDc1GuardHit(&donor, &hit));
    EXPECT_EQ(donor.blow_speed, 3.0f);
    EXPECT_EQ(donor.blow_time, 3);
    donor.frame = 263.0f;
    EXPECT_TRUE(IxionDc1GuardHit(&donor, &hit));
    EXPECT_EQ(donor.life, 25);
    donor.frame = 263.01f;
    EXPECT_FALSE(IxionDc1GuardHit(&donor, &hit));
    world.monsters[1].frame = 250.0f;
    EXPECT_FALSE(IxionDc1GuardHit(&world.monsters[1], &hit));
    RS_STACKDATA result{};
    RS_STACKDATA output{};
    result.type = RS_PTR;
    result.val.p = &output;
    EXPECT_EQ(donor.mons_script.ext_func_table[115](&result, 1), 1);
    EXPECT_EQ(output.val.i, -1);
    EXPECT_NE(donor.mons_script.ext_func_table[113], donor.mons_script.ext_func_table[0]);
    EXPECT_NE(donor.mons_script.ext_func_table[210], donor.mons_script.ext_func_table[0]);
    EXPECT_NE(donor.mons_script.ext_func_table[245], donor.mons_script.ext_func_table[0]);
    IxionDc1Forget(&donor);
    nowMonster = saved_now;
    MainChara__2 = saved_player;
}

namespace {
int WerewolfTestAttack(RS_STACKDATA *args, int argc) {
    if (argc != 2) return false;
    action_info.chara->murderous = rsGetStackInt(args++);
    action_info.chara->murderous_time = rsGetStackInt(args);
    return true;
}
int WerewolfTestCharge(RS_STACKDATA *args, int argc) {
    return argc == 1;
}
}

TEST(IxionDc1Werewolf, MotionDamageResetAndGuardSearchObserveAcceptedHostAttacks) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    donor.target_no = 0;
    donor.chara_type = MONSTER_ACTIVE_MAX;
    CHRINFO_KEY_SET keys[2]{};
    std::strcpy(keys[0].name, "dc1_00");
    keys[0].start_frame = 10;
    keys[0].end_frame = 20;
    donor.key_list[0] = keys;
    donor.key_num[0] = 2;
    CActionChara player;
    auto *saved_player = MainChara__2;
    auto *saved_now = nowMonster;
    auto *saved_action = action_info.chara;
    auto *saved_user = DngUserData;
    auto user = std::make_unique<CUserDataManager>();
    user->active_chr_no = USER_CHARA_MONICA;
    DngUserData = user.get();
    MainChara__2 = &player;
    action_info.chara = &player;
    int (*native_table[ACTION_EXT_FUNC_MAX])(RS_STACKDATA *, int) = {};
    native_table[ACTION_EXT_SET_MURDEROUS] = WerewolfTestAttack;
    native_table[ACTION_EXT_SET_ACCUME_FLAG] = WerewolfTestCharge;
    player.script.ext_func(native_table, ACTION_EXT_FUNC_MAX);
    world.scene->chara_num = 128;
    EXPECT_EQ(world.scene->AssignChara(0, &player, const_cast<char *>("host")), 0);
    Place(donor, 15.584790683f, 0, 0);
    float origin[4] = {0, 0, 0, 1};
    player.SetPosition(origin);
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    RS_STACKDATA set_motion{};
    set_motion.type = RS_INT;
    set_motion.val.i = 0;
    auto call = [&](int number, RS_STACKDATA *args, int argc) {
        return donor.mons_script.ext_func_table[number](args, argc);
    };
    RS_STACKDATA out[3]{};
    RS_STACKDATA ptr[3]{};
    for (int i = 0; i < 3; ++i) { ptr[i].type = RS_PTR; ptr[i].val.p = &out[i]; }
    call(200, &set_motion, 1);
    donor.life = 19;
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, -1); // damage taken is unrelated to DC1 native 113
    CColPrim landed{};
    landed.owner = donor.chara_type;
    IxionDc1RecordPlayerDamage(&landed, 6);
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, 6);
    call(200, &set_motion, 1); // same-key SET_MOTION still clears the previous damage
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, -1);
    donor.life = 17;
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, -1);
    landed.owner = donor.chara_type + 1;
    IxionDc1RecordPlayerDamage(&landed, 2);
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, -1); // another actor cannot stamp this donor's result
    landed.owner = donor.chara_type;
    IxionDc1RecordPlayerDamage(&landed, 0);
    call(113, ptr, 1);
    EXPECT_EQ(out[0].val.i, 0); // an accepted zero-damage hit is distinct from a miss (-1)
    call(210, ptr, 1);
    EXPECT_EQ(out[0].val.i, USER_CHARA_MONICA);
    RS_STACKDATA attack[2]{};
    attack[0].val.i = 1;
    attack[1].val.i = 12;
    EXPECT_EQ(player.script.ext_func_table[ACTION_EXT_SET_MURDEROUS](attack, 1), false);
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 3600); // refused call must not invent an attack
    EXPECT_EQ(player.script.ext_func_table[ACTION_EXT_SET_MURDEROUS](attack, 2), true);
    EXPECT_EQ(player.murderous, 1);
    EXPECT_EQ(player.murderous_time, 12);
    EXPECT_EQ(native_table[ACTION_EXT_SET_MURDEROUS], WerewolfTestAttack); // original table untouched
    for (int i = 0; i < 3; ++i) IxionDc1BeforeScript(&donor);
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 3);
    EXPECT_NEAR(out[1].val.f, 10.0f, 0.0001f);
    EXPECT_EQ(out[2].val.i, USER_CHARA_MONICA);
    const auto saved_pad = GamePad__2.pad[0];
    GamePad__2.pad[0].left_y = 128;
    GamePad__2.pad[0].left_x = 200; // small positive raw tilt (< .35), despite normalized dot 1
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 3);
    GamePad__2.pad[0].left_x = 220; // raw dot > .35 invalidates the search age
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 3600);
    GamePad__2.pad[0] = saved_pad;
    player.script.ext_func_table[ACTION_EXT_SET_MURDEROUS](attack, 2);
    for (int i = 0; i < 3; ++i) IxionDc1BeforeScript(&donor);
    RS_STACKDATA charge{};
    charge.type = RS_INT;
    charge.val.i = ACCUME_EFFECT_MODE_START;
    EXPECT_TRUE(player.script.ext_func_table[ACTION_EXT_SET_ACCUME_FLAG](&charge, 1));
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 0);
    IxionDc1BeforeScript(&donor);
    charge.val.i = ACCUME_EFFECT_MODE_END;
    EXPECT_TRUE(player.script.ext_func_table[ACTION_EXT_SET_ACCUME_FLAG](&charge, 1));
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 1); // charge end is not a new attack
    CColPrim projectile{};
    DAMAGE_PARAM *ranged = Damage_Param_Table;
    while (ranged->name[0] && (ranged->kind != DAMAGE_KIND_MAX_GUN ||
           !(ranged->target & (DAMAGE_TARGET_MONSTER | DAMAGE_TARGET_BY_OWNER)))) ++ranged;
    ASSERT_NE(ranged->name[0], 0);
    EXPECT_TRUE(projectile.SetDamage(ranged->name, 0));
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 0); // actual projectile emission, independent of SET_MURDEROUS
    IxionDc1BeforeScript(&donor);
    EXPECT_FALSE(projectile.SetDamage(const_cast<char *>("not-a-damage-row"), 0));
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 1);
    EXPECT_TRUE(projectile.SetDamage(ranged->name, 1));
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 1); // a monster's shot is not a player attack
    for (int i = 0; i < 2; ++i) IxionDc1BeforeScript(&donor);
    world.scene->battle_area.script.running = true;
    EXPECT_EQ(player.script.ext_func_table[ACTION_EXT_SET_MURDEROUS](attack, 2), true);
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 3); // an event ignores attack-age observation, preserving the native call
    world.scene->battle_area.script.running = false;
    EXPECT_EQ(player.script.ext_func_table[ACTION_EXT_SET_MURDEROUS](attack, 2), true);
    call(245, ptr, 3);
    EXPECT_EQ(out[0].val.i, 0); // first resumed action reads live state, not the old paused cache
    IxionDc1Forget(&donor);
    EXPECT_EQ(player.script.ext_func_table, native_table); // host table restored on retirement
    nowMonster = saved_now;
    MainChara__2 = saved_player;
    action_info.chara = saved_action;
    DngUserData = saved_user;
}

TEST(IxionDc1Round4, SupportedNativesRefuseMissingOrForeignContext) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    auto *saved_now = nowMonster;
    IxionDc1InstallNatives(&donor);
    auto table = donor.mons_script.ext_func_table;
    for (auto *context : {static_cast<CActiveMonster *>(nullptr), &world.monsters[1]}) {
        nowMonster = context;
        for (int i = 0; i < 256; ++i) {
            if (table[i] != table[0]) EXPECT_EQ(table[i](nullptr, 0), 0) << "native " << i;
        }
    }
    IxionDc1Forget(&donor);
    nowMonster = &donor;
    EXPECT_EQ(table[245](nullptr, 0), 0);
    nowMonster = saved_now;
}

TEST(IxionDc1Round4, LastNamedKeyWorksAndSentinelAndOutOfRangeDoNot) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    CHRINFO_KEY_SET keys[3]{};
    std::strcpy(keys[0].name, "first");
    std::strcpy(keys[1].name, "last");
    donor.key_list[0] = keys;
    donor.key_num[0] = 2; // also accept a bounded list without an empty sentinel
    auto *saved_now = nowMonster;
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    RS_STACKDATA motion{};
    motion.type = RS_INT;
    motion.val.i = 1;
    EXPECT_TRUE(donor.mons_script.ext_func_table[200](&motion, 1));
    EXPECT_EQ(donor.next_key, &keys[1]);
    donor.key_num[0] = 3; // retail _KEY_END includes this empty entry in the count
    for (int invalid : {-1, 2, 3}) {
        motion.val.i = invalid;
        donor.mons_script.ext_func_table[200](&motion, 1);
        EXPECT_EQ(donor.next_key, &keys[1]);
    }
    IxionDc1Forget(&donor);
    nowMonster = saved_now;
}

TEST(IxionDc1Round4, GuardLogSamplesLaterLifeRatherThanPrintingSnapshotTwice) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    donor.muteki_time = 8; // DC1's full guard window is independent of muteki
    auto *saved_now = nowMonster;
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    RS_STACKDATA range[2]{};
    range[0].type = range[1].type = RS_INT;
    range[0].val.i = 244;
    range[1].val.i = 263;
    donor.mons_script.ext_func_table[244](range, 2);
    donor.frame = 250;
    DAMAGE_PARAM param{};
    param.kind = DAMAGE_KIND_MAX_MELEE;
    CColPrim hit{};
    hit.param = &param;
    hit.damage = 8;
    testing::internal::CaptureStderr();
    EXPECT_TRUE(IxionDc1GuardHit(&donor, &hit));
    EXPECT_EQ(donor.life, 25);
    donor.life = 24; // independent intervening HP change must be visible to the logger
    IxionDc1BeforeScript(&donor);
    const auto log = testing::internal::GetCapturedStderr();
    EXPECT_NE(log.find("hp=25->24"), std::string::npos);
    EXPECT_NE(log.find("sampled_frame="), std::string::npos);
    IxionDc1Forget(&donor);
    nowMonster = saved_now;
    MainChara__2 = saved_player;
}

TEST(IxionDc1Round4, DeathProgramRejectsHitsWithoutConsumingOrRestartingThem) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    world.scene->chara_num = 128;
    ASSERT_EQ(world.scene->AssignChara(MONSTER_ACTIVE_MAX, &donor, const_cast<char *>("donor")), MONSTER_ACTIVE_MAX);
    CColPrimMan prims{};
    prims.Initialize(world.scene.get());
    DAMAGE_PARAM damage{};
    damage.kind = DAMAGE_KIND_MAX_MELEE;
    damage.shape = DAMAGE_SHAPE_POINT;
    damage.target = DAMAGE_TARGET_MONSTER;
    mgCFrame frame;
    donor.entry_object[0].frame = &frame;
    donor.entry_object[0].group = CHARA_ENTRY_GROUP_HIT;
    donor.entry_object[0].enable = true;
    float position[4];
    frame.GetWorldPosition0(position);
    auto &hit = prims.prim[0];
    hit.active = true;
    hit.param = &damage;
    hit.target = DAMAGE_TARGET_MONSTER;
    hit.radius = 10;
    hit.SetCoord(position, 10);
    IxionDc1InstallNatives(&donor);
    donor.life = 0;
    donor.now_prog = MONSTER_PROG_DEAD;
    donor.req_prog = -1;
    EXPECT_EQ(prims.CheckHit(MONSTER_ACTIVE_MAX), nullptr);
    EXPECT_EQ(hit.hit_num, 0);
    EXPECT_EQ(donor.req_prog, -1);
    donor.life = 25; // a running death program also refuses hits even if life was restored
    EXPECT_EQ(prims.CheckHit(MONSTER_ACTIVE_MAX), nullptr);
    donor.now_prog = MONSTER_PROG_MAIN;
    EXPECT_EQ(prims.CheckHit(MONSTER_ACTIVE_MAX), &hit); // same collision is valid on a live donor
    IxionDc1Forget(&donor);
}

TEST(IxionDc1Round4, ScalingMatchesFriendlyAndFaithfulPolicy) {
    for (auto pair : {std::pair{35, 7}, {20, 4}, {21, 4}, {68, 14}, {62, 13}})
        EXPECT_EQ(IxionDc1ScaledDamage(pair.first, 32, 146), pair.second);
    for (auto pair : {std::pair{35, 15}, {20, 8}, {21, 8}, {68, 30}, {62, 27}})
        EXPECT_EQ(IxionDc1ScaledDamage(pair.first, 32, 70), pair.second);
    EXPECT_EQ(IxionDc1ScaledDamage(35, 48, 146), 11);
    EXPECT_EQ(IxionDc1ScaledDamage(3, 32, 146), 1); // retains the adapter's minimum damage policy
}

TEST(IxionDc1Round4, StaleDiscardAndOtherRetirePathsClearPausedShotsAndAttackPrims) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    auto *saved_now = nowMonster;
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    const int saved_mode = DngStatus.mode;
    DngStatus.mode = DNG_STATUS_FIELD;
    auto saved_prims = std::make_unique<CColPrimMan>(ColPrimMan);
    const char *old_donor = std::getenv("DC2_IXION_ROOM_DONOR_MONSTER");
    const std::string saved_donor = old_donor != nullptr ? old_donor : "";
    const char *old_balance = std::getenv("DC2_IXION_DC1_BALANCE");
    const std::string saved_balance = old_balance != nullptr ? old_balance : "";
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e104a", 1);
    dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
    for (int path = 0; path < 5; ++path) {
        ColPrimMan.Initialize(world.scene.get());
        donor.monster_id = 0x7000;
        donor.state = ACTIVE_MONSTER_LIVE;
        donor.chara_type = MONSTER_ACTIVE_MAX;
        world.scene->battle_area.script.running = false;
        IxionDc1InstallNatives(&donor);
        nowMonster = &donor;
        RS_STACKDATA shot[5]{};
        shot[0].type = RS_STR;
        shot[0].val.s = const_cast<char *>("missing-frame");
        for (int i = 1; i < 4; ++i) { shot[i].type = RS_FLOAT; shot[i].val.f = 100; }
        shot[4].type = RS_INT;
        shot[4].val.i = 35;
        EXPECT_TRUE(donor.mons_script.ext_func_table[133](shot, 5));
        auto &projectile = ColPrimMan.prim[0];
        ASSERT_TRUE(projectile.active);
        EXPECT_EQ(projectile.damage, 7);
        dc::test::SetEnv("DC2_IXION_DC1_BALANCE", "faithful", 1);
        EXPECT_TRUE(donor.mons_script.ext_func_table[133](shot, 5));
        EXPECT_EQ(ColPrimMan.prim[1].damage, 15);
        dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
        auto &attack = ColPrimMan.prim[2];
        attack.active = true;
        attack.owner = donor.chara_type;
        donor.damage[0].prim = &attack;
        IxionDc1AttackPrim(&donor, &donor.damage[0]);
        auto &native = ColPrimMan.prim[3];
        native.active = true;
        native.owner = 1;
        native.life = -1;
        world.scene->battle_area.script.running = true;
        EXPECT_TRUE(IxionDc1UpdatePause());
        const float previous_x = projectile.pos[0][0];
        IxionDc1StepShots();
        EXPECT_FLOAT_EQ(projectile.pos[0][0], previous_x);
        if (path == 0) {
            IxionRoomTestInstall state;
            state.scene = reinterpret_cast<void *>(static_cast<std::uintptr_t>(0x1000));
            state.enemy = &donor;
            IxionRoomTestAdoptInstalled(state);
            std::array<unsigned char, sizeof(CActiveMonster)> bytes;
            std::memcpy(bytes.data(), static_cast<void *>(&donor), bytes.size());
            std::memset(static_cast<void *>(&donor), 0xCD, sizeof donor); // reset actor arena, retain prim identities
            IxionRoomBeforeLoopInit();
            const auto *poison = reinterpret_cast<const unsigned char *>(&donor);
            EXPECT_TRUE(std::all_of(poison, poison + sizeof donor, [](unsigned char b) { return b == 0xCD; }));
            std::memcpy(static_cast<void *>(&donor), bytes.data(), bytes.size());
        } else if (path == 1) IxionDc1Despawn(&donor, "test-retire");
        else if (path == 2) IxionDc1InstallNatives(&donor); // slot replacement
        else if (path == 3) { donor.state = ACTIVE_MONSTER_NONE; IxionDc1Despawn(&donor, "test-already-removed"); }
        else { donor.state = ACTIVE_MONSTER_NONE; IxionDc1Removed(&donor); }
        EXPECT_FALSE(projectile.active);
        EXPECT_FALSE(ColPrimMan.prim[1].active);
        EXPECT_FALSE(attack.active);
        EXPECT_TRUE(native.active);
        world.scene->battle_area.script.running = false;
        IxionDc1StepShots();
        EXPECT_FALSE(projectile.active);
        IxionDc1Forget(&donor);
        donor.damage[0].prim = nullptr;
    }
    ColPrimMan = *saved_prims;
    if (old_donor != nullptr) dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", saved_donor.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_MONSTER");
    if (old_balance != nullptr) dc::test::SetEnv("DC2_IXION_DC1_BALANCE", saved_balance.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
    nowMonster = saved_now;
    MainChara__2 = saved_player;
    DngStatus.mode = saved_mode;
}

TEST(IxionDc1Selection, OverlayDescriptorSelectsAnotherIdAndRejectsInvalidProperties) {
    const char *previous = std::getenv("DC2_IXION_ROOM_DONOR_MONSTER");
    const std::string saved = previous != nullptr ? previous : "";
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("ghost-r1-profile-" + std::to_string(dc::test::ProcessId()) + "-" + std::to_string(unique));
    ASSERT_FALSE(std::filesystem::exists(root));
    std::filesystem::create_directories(root / "dungeon/monster");
    nlohmann::json profile = {{"format", "ixion-dc1-monster"}, {"version", 1}, {"name", "Synthetic donor"},
        {"life", 19}, {"defense", 2}, {"body_radius", 4.0}, {"no_blow", false}, {"shot_effect", -1},
        {"reward_exp", 7}, {"reward_money", 9}};
    const auto good = root / "dungeon/monster/ix_e900a.dc1.json";
    const auto bad = root / "dungeon/monster/ix_e900b.dc1.json";
    { std::ofstream stream(good); stream << profile; }
    profile["life"] = 0;
    { std::ofstream stream(bad); stream << profile; }
    DataReadIndex(root);
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e900a", 1);
    ASSERT_TRUE(IxionDc1DonorRequested());
    auto manager = std::make_unique<CMonsterMan>();
    const auto *row = manager->GetReferPtr2(0x7000);
    ASSERT_NE(row, nullptr);
    EXPECT_STREQ(row->model, "ix_e900a");
    EXPECT_STREQ(row->script, "ix_e900a");
    EXPECT_EQ(row->life, 19);
    EXPECT_EQ(row->defense, 2);
    EXPECT_EQ(row->reward_exp, 7);
    EXPECT_EQ(row->reward_money, 9);
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e900b", 1);
    EXPECT_FALSE(IxionDc1DonorRequested());
    EXPECT_EQ(manager->GetReferPtr2(0x7000), nullptr);
    if (previous != nullptr) dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", saved.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_MONSTER");
    std::filesystem::remove(good);
    std::filesystem::remove(bad);
    std::filesystem::remove(root / "dungeon/monster");
    std::filesystem::remove(root / "dungeon");
    std::filesystem::remove(root);
}

namespace {
struct DonorEnv {
    std::string saved;
    bool had = false;
    explicit DonorEnv(const char *value) {
        const char *previous = std::getenv("DC2_IXION_ROOM_DONOR_ROSTER");
        had = previous != nullptr;
        saved = had ? previous : "";
        Set(value);
    }
    void Set(const char *value) { dc::test::SetEnv("DC2_IXION_ROOM_DONOR_ROSTER", value, 1); }
    ~DonorEnv() {
        if (had) dc::test::SetEnv("DC2_IXION_ROOM_DONOR_ROSTER", saved.c_str(), 1);
        else dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_ROSTER");
    }
};
} // namespace

TEST(IxionDc1Roster, ListSelectsEachKindUnderItsOwnRowAndRefusesBadLists) {
    DonorEnv env("e104a,e42a,e07a");
    auto manager = std::make_unique<CMonsterMan>();
    ASSERT_TRUE(IxionDc1DonorRequested());
    EXPECT_EQ(IxionDc1RosterSize(), 3);
    const BASE_MONSTER_TBL *opar = manager->GetReferPtr2(0x7000);
    const BASE_MONSTER_TBL *ghost = manager->GetReferPtr2(0x7001);
    const BASE_MONSTER_TBL *werewolf = manager->GetReferPtr2(0x7002);
    ASSERT_NE(opar, nullptr);
    ASSERT_NE(ghost, nullptr);
    ASSERT_NE(werewolf, nullptr);
    EXPECT_STREQ(opar->model, "ix_e104a");
    EXPECT_STREQ(ghost->model, "ix_e42a");
    EXPECT_STREQ(werewolf->script, "ix_e07a");
    EXPECT_EQ(opar->id, 0x7000);
    EXPECT_EQ(ghost->id, 0x7001);
    EXPECT_EQ(werewolf->id, 0x7002);
    EXPECT_EQ(werewolf->life, 180);
    EXPECT_EQ(manager->GetReferPtr2(0x7003), nullptr);
    EXPECT_STREQ(IxionDc1RosterId(1), "e42a");
    EXPECT_EQ(IxionDc1RosterId(3), nullptr);
    for (const char *bad : {"e104a,e104a", "e104a,../e42a", "e104a,", ",e104a", "e104a,e42a,e07a,e104a,e42a,e07a,e104a,e42a,e07a"}) {
        env.Set(bad);
        EXPECT_FALSE(IxionDc1DonorRequested()) << bad;
        EXPECT_EQ(manager->GetReferPtr2(0x7000), nullptr) << bad;
        EXPECT_EQ(IxionDc1RosterSize(), 0) << bad;
    }
    env.Set("e42a"); // a single id is kind 0 exactly as before
    EXPECT_EQ(IxionDc1RosterSize(), 1);
    ASSERT_NE(manager->GetReferPtr2(0x7000), nullptr);
    EXPECT_STREQ(manager->GetReferPtr2(0x7000)->model, "ix_e42a");
    EXPECT_EQ(manager->GetReferPtr2(0x7001), nullptr);
}

TEST(IxionDc1Roster, TwoInstancesKeepTheirOwnStateShotsAndRetirement) {
    NativeWorld world;
    DonorEnv env("e104a,e42a");
    auto *saved_now = nowMonster;
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    const int saved_mode = DngStatus.mode;
    DngStatus.mode = DNG_STATUS_FIELD;
    auto saved_prims = std::make_unique<CColPrimMan>(ColPrimMan);
    ColPrimMan.Initialize(world.scene.get());
    CActiveMonster &opar = world.monsters[0];
    CActiveMonster &ghost = world.monsters[1];
    opar.monster_id = 0x7000;
    opar.chara_type = MONSTER_ACTIVE_MAX;
    ghost.monster_id = 0x7001;
    ghost.chara_type = MONSTER_ACTIVE_MAX + 1;
    IxionDc1InstallNatives(&opar);
    IxionDc1InstallNatives(&ghost); // the second install must not forget the first
    EXPECT_TRUE(IxionDc1IsDonor(&opar));
    EXPECT_TRUE(IxionDc1IsDonor(&ghost));
    RS_STACKDATA shot[5]{};
    shot[0].type = RS_STR;
    shot[0].val.s = const_cast<char *>("missing-frame");
    for (int i = 1; i < 4; ++i) { shot[i].type = RS_FLOAT; shot[i].val.f = 100; }
    shot[4].type = RS_INT;
    shot[4].val.i = 35;
    nowMonster = &opar;
    EXPECT_TRUE(opar.mons_script.ext_func_table[133](shot, 5));
    nowMonster = &ghost;
    EXPECT_TRUE(ghost.mons_script.ext_func_table[133](shot, 4)); // Ghost's own effect 9, default damage
    CColPrim &goo = ColPrimMan.prim[0];
    CColPrim &curse = ColPrimMan.prim[1];
    ASSERT_TRUE(goo.active);
    ASSERT_TRUE(curse.active);
    EXPECT_EQ(goo.owner, opar.chara_type);
    EXPECT_EQ(curse.owner, ghost.chara_type);
    // Per-instance native state: a guard window on Opar does not appear on Ghost.
    RS_STACKDATA window[2]{};
    window[0].type = window[1].type = RS_INT;
    window[0].val.i = 10;
    window[1].val.i = 20;
    nowMonster = &opar;
    opar.mons_script.ext_func_table[244](window, 2);
    DAMAGE_PARAM param{};
    param.kind = DAMAGE_KIND_MAX_MELEE;
    CColPrim hit{};
    hit.param = &param;
    opar.frame = ghost.frame = 15.0f;
    EXPECT_TRUE(IxionDc1GuardHit(&opar, &hit));
    EXPECT_FALSE(IxionDc1GuardHit(&ghost, &hit));
    // Outgoing damage is stamped on the owner only (native 113).
    RS_STACKDATA out{};
    RS_STACKDATA ptr{};
    ptr.type = RS_PTR;
    ptr.val.p = &out;
    CColPrim landed{};
    landed.owner = ghost.chara_type;
    IxionDc1RecordPlayerDamage(&landed, 4);
    nowMonster = &ghost;
    ghost.mons_script.ext_func_table[113](&ptr, 1);
    EXPECT_EQ(out.val.i, 4);
    nowMonster = &opar;
    opar.mons_script.ext_func_table[113](&ptr, 1);
    EXPECT_EQ(out.val.i, -1);
    // Retiring Opar ends only Opar's shot; Ghost keeps running and keeps its shot.
    IxionDc1Despawn(&opar, "test-roster-retire");
    EXPECT_FALSE(goo.active);
    EXPECT_TRUE(curse.active);
    EXPECT_FALSE(IxionDc1IsDonor(&opar));
    EXPECT_TRUE(IxionDc1IsDonor(&ghost));
    IxionDc1StepShots();
    EXPECT_TRUE(curse.active);
    IxionDc1Forget(&ghost);
    EXPECT_FALSE(curse.active);
    EXPECT_FALSE(IxionDc1IsDonor(&ghost));
    ColPrimMan = *saved_prims;
    nowMonster = saved_now;
    MainChara__2 = saved_player;
    DngStatus.mode = saved_mode;
}

TEST(IxionDc1Roster, RetireDespawnsEveryRosterDonorAndNoneIsSuspended) {
    NativeWorld world;
    DonorEnv env("e104a,e42a");
    auto *saved_player = MainChara__2;
    MainChara__2 = nullptr;
    CActiveMonster &primary = world.monsters[0];
    CActiveMonster &extra = world.monsters[1];
    primary.monster_id = 0x7000;
    primary.chara_type = MONSTER_ACTIVE_MAX;
    extra.monster_id = 0x7001;
    extra.chara_type = MONSTER_ACTIVE_MAX + 1;
    IxionDc1InstallNatives(&primary);
    IxionDc1InstallNatives(&extra);
    IxionRoomTestInstall state;
    state.scene = world.scene.get();
    state.enemy = &primary;
    state.roster = {&extra};
    IxionRoomTestAdoptInstalled(state);
    EXPECT_FALSE(IxionRoomMonsterSuspended(&extra));
    EXPECT_FALSE(IxionRoomMonsterParked(&extra));
    IxionRoomSetThinkingMonster(&extra);
    EXPECT_TRUE(IxionRoomOwnsNavigationQuery());
    IxionRoomSetThinkingMonster(nullptr);
    IxionRoomRetire("test-roster-retire");
    EXPECT_EQ(primary.state, ACTIVE_MONSTER_NONE);
    EXPECT_EQ(extra.state, ACTIVE_MONSTER_NONE);
    EXPECT_FALSE(IxionDc1IsDonor(&primary));
    EXPECT_FALSE(IxionDc1IsDonor(&extra));
    MainChara__2 = saved_player;
}

TEST(IxionDc1Roster, RingPointsAvoidWallsNeedFloorAndAreDeterministic) {
    IxionRoom room;
    room.enemy_spawn = {100.0f, 0.0f, 0.0f};
    auto tri = [](std::array<float, 3> a, std::array<float, 3> b, std::array<float, 3> c) {
        IxionRoomTriangle t;
        t.vertices = {a, b, c};
        return t;
    };
    float out[3] = {};
    EXPECT_FALSE(IxionRoomRosterPoint(room, 1, 2, out)) << "no floor at all";
    room.collision.push_back(tri({-200, 0, -200}, {-200, 0, 200}, {300, 0, -200}));
    room.collision.push_back(tri({300, 0, -200}, {-200, 0, 200}, {300, 0, 200}));
    ASSERT_TRUE(IxionRoomRosterPoint(room, 1, 5, out));
    EXPECT_NEAR(out[0], 160.0f, 0.01f);
    EXPECT_NEAR(out[2], 0.0f, 0.01f);
    ASSERT_TRUE(IxionRoomRosterPoint(room, 2, 5, out));
    EXPECT_NEAR(out[0], 100.0f, 0.01f);
    EXPECT_NEAR(out[2], 60.0f, 0.01f);
    EXPECT_NEAR(out[1], 0.0f, 0.01f);
    // A wall at x=130 blocks the first ring point; the half-step point behind the marker is used.
    for (auto wall : {tri({130, -10, -20}, {130, -10, 20}, {130, 50, -20}), tri({130, -10, -20}, {130, 50, -20}, {130, -10, 20}),
                      tri({130, 50, 20}, {130, 50, -20}, {130, -10, 20}), tri({130, 50, 20}, {130, -10, 20}, {130, 50, -20})})
        room.collision.push_back(wall);
    ASSERT_TRUE(IxionRoomRosterPoint(room, 1, 2, out));
    EXPECT_NEAR(out[0], 40.0f, 0.01f);
    EXPECT_NEAR(out[2], 0.0f, 0.1f);
    float again[3] = {};
    ASSERT_TRUE(IxionRoomRosterPoint(room, 1, 2, again));
    EXPECT_EQ(std::memcmp(out, again, sizeof out), 0);
    EXPECT_FALSE(IxionRoomRosterPoint(room, 0, 2, out));
    EXPECT_FALSE(IxionRoomRosterPoint(room, 2, 2, out));
}

TEST(IxionDc1Natives, LifeInnerProductMoveVecLoopSoundAndCollisionSpheres) {
    NativeWorld world;
    auto &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    donor.target_no = 0;
    donor.life = donor.max_life = 38;
    donor.now_key = nullptr; // IxionDc1AfterScript logs the motion name
    CActionChara player;
    auto *saved_player = MainChara__2;
    auto *saved_now = nowMonster;
    MainChara__2 = &player;
    world.scene->chara_num = 128;
    ASSERT_EQ(world.scene->AssignChara(0, &player, const_cast<char *>("host")), 0);
    float origin[4] = {0, 0, 0, 1};
    player.SetPosition(origin);
    Place(donor, 0.0f, 0.0f, 15.584790683f); // 10 DC1 units in front of the player along +z
    float facing[4] = {0, 0, 0, 1};
    donor.SetRotation(facing); // facing +z: away from the player
    // Frames for the collision natives: root -> {bcol0, mcol0, lockon}.
    mgCFrame root, bcol, mcol, lockon;
    root.SetName(const_cast<char *>("root"));
    bcol.SetName(const_cast<char *>("bcol0"));
    mcol.SetName(const_cast<char *>("mcol0"));
    lockon.SetName(const_cast<char *>("lockon"));
    root.SetChild(&bcol);
    root.SetChild(&mcol);
    root.SetChild(&lockon);
    mgCFrame *saved_frame = donor.CObjectFrame::frame;
    donor.CObjectFrame::frame = &root;
    for (auto &entry : donor.entry_object) entry = CHARA_ENTRY_OBJECT{nullptr, 0.0f, -1, 0};
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    auto call = [&](int number, RS_STACKDATA *args, int argc) { return donor.mons_script.ext_func_table[number](args, argc); };
    RS_STACKDATA out[3]{};
    RS_STACKDATA ptr[3]{};
    for (int i = 0; i < 3; ++i) { ptr[i].type = RS_PTR; ptr[i].val.p = &out[i]; }
    // 15: cos >= 1 - 80/90 and inside the optional distance.
    RS_STACKDATA inner[3]{};
    inner[0].type = RS_INT;
    inner[0].val.i = 80;
    inner[1] = ptr[0];
    EXPECT_EQ(call(15, inner, 2), 1);
    EXPECT_EQ(out[0].val.i, 1);
    inner[1].type = RS_FLOAT;
    inner[1].val.f = 5.0f; // the player is 10 DC1 units away
    inner[2] = ptr[0];
    call(15, inner, 3);
    EXPECT_EQ(out[0].val.i, 0);
    facing[1] = 3.14159265f; // now facing the player
    donor.SetRotation(facing);
    inner[1] = ptr[0];
    call(15, inner, 2);
    EXPECT_EQ(out[0].val.i, 0);
    // 109 and 228.
    donor.life = 15;
    call(109, ptr, 1);
    EXPECT_NEAR(out[0].val.f, 100.0f * 15.0f / 38.0f, 0.001f);
    RS_STACKDATA rate{};
    rate.type = RS_FLOAT;
    rate.val.f = 0.45f;
    call(228, &rate, 1);
    EXPECT_EQ(donor.life, 17); // (int)(38 * 0.45)
    // 246: DC1 never writes script_move_vector.
    out[0].val.f = out[1].val.f = out[2].val.f = 9.0f;
    call(246, ptr, 3);
    EXPECT_EQ(out[0].val.f, 0.0f);
    EXPECT_EQ(out[1].val.f, 0.0f);
    EXPECT_EQ(out[2].val.f, 0.0f);
    // 230 / 232: recorded, logged, no DC2 fallthrough.
    RS_STACKDATA loop[4]{};
    for (auto &arg : loop) { arg.type = RS_FLOAT; arg.val.f = 88.0f; }
    EXPECT_EQ(call(230, loop, 4), 1);
    EXPECT_EQ(call(232, nullptr, 0), 1);
    // 130 with a window, 136, 118: DC2 HIT, BODY and TARGET entries on the named frames.
    RS_STACKDATA body[4]{};
    body[0].type = RS_STR;
    body[0].val.s = const_cast<char *>("bcol0");
    body[1].type = RS_FLOAT;
    body[1].val.f = 20.0f;
    body[2].type = body[3].type = RS_FLOAT;
    body[2].val.f = 30.0f;
    body[3].val.f = 40.0f;
    call(130, body, 4);
    body[0].val.s = const_cast<char *>("mcol0");
    body[1].val.f = 10.0f;
    call(136, body, 2);
    body[0].val.s = const_cast<char *>("nope");
    call(136, body, 2); // a missing frame adds nothing
    body[0].val.s = const_cast<char *>("lockon");
    body[1].val.f = 1.2f;
    body[2].val.f = 1.3f;
    call(118, body, 3);
    float position[4];
    CHARA_ENTRY_OBJECT *hit = donor.GetEntryObjectPos(CHARA_ENTRY_GROUP_HIT, 0, position);
    CHARA_ENTRY_OBJECT *push = donor.GetEntryObjectPos(CHARA_ENTRY_GROUP_BODY, 0, position);
    CHARA_ENTRY_OBJECT *target = donor.GetEntryObjectPos(CHARA_ENTRY_GROUP_TARGET, 0, position);
    ASSERT_NE(hit, nullptr);
    ASSERT_NE(push, nullptr);
    ASSERT_NE(target, nullptr);
    EXPECT_EQ(hit->frame, &bcol);
    EXPECT_NEAR(hit->size, 0.5f * 20.0f * 1.5584790683f, 0.001f);
    EXPECT_EQ(push->frame, &mcol);
    EXPECT_NEAR(push->size, 0.5f * 10.0f * 1.5584790683f, 0.001f);
    EXPECT_EQ(donor.GetEntryObjectPos(CHARA_ENTRY_GROUP_BODY, 1, position), nullptr);
    EXPECT_EQ(target->frame, &lockon);
    // The DC1 window (start < time <= end) drives the HIT entry each think.
    donor.frame = 30.0f;
    IxionDc1AfterScript(&donor);
    EXPECT_FALSE(hit->enable);
    donor.frame = 40.0f;
    IxionDc1AfterScript(&donor);
    EXPECT_TRUE(hit->enable);
    donor.frame = 40.5f;
    IxionDc1AfterScript(&donor);
    EXPECT_FALSE(hit->enable);
    IxionDc1Forget(&donor);
    for (auto &entry : donor.entry_object) entry = CHARA_ENTRY_OBJECT{nullptr, 0.0f, -1, 0};
    donor.CObjectFrame::frame = saved_frame;
    nowMonster = saved_now;
    MainChara__2 = saved_player;
}

// ---- Roster R2: DC1 ArrangementPos, kill memory, round-2 natives, mimic reveal ----

namespace {
IxionRoom FlatRoomWithWall() {
    // 400 x 400 floor at y=0 split by a wall at x=200 (z 0..400, height 100). Spawn on the west side.
    IxionRoom room;
    room.donor_room = "synthetic";
    room.unit_scale = 1.0f;
    room.bounds_min = {0.0f, -10.0f, 0.0f};
    room.bounds_max = {400.0f, 110.0f, 400.0f};
    room.spawn = {40.0f, 0.0f, 200.0f};
    room.enemy_spawn = {100.0f, 0.0f, 200.0f};
    room.exit = {40.0f, 0.0f, 360.0f};
    auto tri = [&](std::array<float, 3> a, std::array<float, 3> b, std::array<float, 3> c) {
        IxionRoomTriangle t;
        t.vertices = {a, b, c};
        room.collision.push_back(t);
    };
    tri({0, 0, 0}, {0, 0, 400}, {400, 0, 0});     // upward normal
    tri({400, 0, 0}, {0, 0, 400}, {400, 0, 400});
    tri({200, 0, 0}, {200, 100, 0}, {200, 0, 400}); // the wall
    tri({200, 100, 0}, {200, 100, 400}, {200, 0, 400});
    return room;
}
} // namespace

TEST(IxionRoomArrange, Dc1RuleSpacingReachabilityKindsAndDeterminism) {
    const IxionRoom room = FlatRoomWithWall();
    const std::vector<bool> unique = {false, false, true};
    const auto first = IxionRoomArrange(room, 12, unique, 1234u);
    const auto again = IxionRoomArrange(room, 12, unique, 1234u);
    const auto other = IxionRoomArrange(room, 12, unique, 99u);
    ASSERT_EQ(first.spawns.size(), 12u);
    EXPECT_EQ(first.dropped, 0);
    EXPECT_GT(first.candidate_cells, 0);
    EXPECT_LT(first.reachable_cells, first.walkable_cells); // the east half is behind the wall
    // Spawn 0 is the authored enemy marker; spawns 1 and 2 take kinds 1 and 2.
    EXPECT_EQ(first.spawns[0].position, room.enemy_spawn);
    EXPECT_EQ(first.spawns[0].kind, 0);
    EXPECT_EQ(first.spawns[1].kind, 1);
    EXPECT_EQ(first.spawns[2].kind, 2);
    int unique_count = 0;
    for (size_t i = 0; i < first.spawns.size(); ++i) {
        const auto &spawn = first.spawns[i];
        unique_count += spawn.kind == 2 ? 1 : 0;
        EXPECT_LT(spawn.position[0], 200.0f) << "spawn " << i << " is behind the wall";
        EXPECT_NEAR(spawn.position[1], 0.0f, 0.01f);
        for (size_t j = 0; j < i; ++j) {
            const float dx = spawn.position[0] - first.spawns[j].position[0];
            const float dz = spawn.position[2] - first.spawns[j].position[2];
            EXPECT_GT(std::sqrt(dx * dx + dz * dz), 25.0f) << i << " vs " << j; // DC1 25-unit spacing
        }
        const float sx = spawn.position[0] - room.spawn[0], sz = spawn.position[2] - room.spawn[2];
        if (i > 0) EXPECT_GT(std::sqrt(sx * sx + sz * sz), 20.0f);
        EXPECT_EQ(spawn.position, again.spawns[i].position);
        EXPECT_EQ(spawn.kind, again.spawns[i].kind);
    }
    EXPECT_EQ(unique_count, 1); // a non-normal DC1 kind is placed once (monstorunit.cpp:610-616)
    // The whole room lies within 480 of every point: the strict pass stops at three, the rest relax.
    EXPECT_EQ(first.strict, 2);
    EXPECT_EQ(first.relaxed, 9);
    EXPECT_FALSE(first.spawns[1].relaxed);
    EXPECT_TRUE(first.spawns[11].relaxed);
    bool differs = false;
    for (size_t i = 1; i < other.spawns.size(); ++i) differs = differs || other.spawns[i].position != first.spawns[i].position;
    EXPECT_TRUE(differs);
    EXPECT_EQ(IxionRoomArrangeSeed("d01inter", 0, 1, 2), IxionRoomArrangeSeed("d01inter", 0, 1, 2));
    EXPECT_NE(IxionRoomArrangeSeed("d01inter", 0, 1, 2), IxionRoomArrangeSeed("d01inter", 0, 1, 3));
}

TEST(IxionRoomArrange, RealDonorRoomPlacesFifteenOnReachableFloor) {
    const char *pack = std::getenv("IXION_ROOM_TEST_PACK");
    if (pack == nullptr || *pack == '\0') GTEST_SKIP() << "IXION_ROOM_TEST_PACK was not set";
    IxionRoom room;
    std::string error;
    ASSERT_TRUE(IxionRoom::Load(std::filesystem::path(pack), room, error)) << error;
    const auto result = IxionRoomArrange(room, 15, std::vector<bool>(5, false), IxionRoomArrangeSeed(room.donor_room, 0, 1, 2));
    std::printf("arrange real room: walkable=%d reachable=%d candidates=%d placed=%zu strict=%d relaxed=%d dropped=%d\n",
                result.walkable_cells, result.reachable_cells, result.candidate_cells, result.spawns.size(), result.strict,
                result.relaxed, result.dropped);
    EXPECT_EQ(result.spawns.size(), 15u);
    EXPECT_GE(result.strict, 1);
    for (const auto &spawn : result.spawns) {
        for (int axis = 0; axis < 3; ++axis) {
            EXPECT_GE(spawn.position[axis], room.bounds_min[axis]);
            EXPECT_LE(spawn.position[axis], room.bounds_max[axis]);
        }
    }
}

TEST(IxionRoomKillMemory, KilledSpawnIsRememberedAcrossRetireUntilTheFloorLifetimeEnds) {
    NativeWorld world;
    CActiveMonster &first = world.monsters[0];
    CActiveMonster &second = world.monsters[1];
    IxionRoomTestInstall install;
    install.scene = world.scene.get();
    install.enemy = &first;
    install.enemy_key = IxionRoomDonorKillKey("d01inter", 0, 1, 0, "e06a");
    install.roster = {&second};
    install.roster_keys = {IxionRoomDonorKillKey("d01inter", 0, 1, 1, "e03a")};
    EXPECT_EQ(install.roster_keys[0], "d01inter:0:1:spawn01:e03a");
    IxionRoomTestAdoptInstalled(install);
    const uint64_t lifetime = IxionRoomFloorLifetime();
    EXPECT_EQ(IxionRoomDonorKillKeyFor(&second), install.roster_keys[0]);
    EXPECT_EQ(IxionRoomDonorKillKeyFor(&first), install.enemy_key);
    EXPECT_FALSE(IxionRoomKillRemembered(install.roster_keys[0]));
    IxionRoomRecordDonorKill(&second, "e03a");
    IxionRoomRecordDonorKill(&second, "e03a"); // a repeated report is one memory
    EXPECT_TRUE(IxionRoomKillRemembered(install.roster_keys[0]));
    EXPECT_FALSE(IxionRoomKillRemembered(install.enemy_key));
    // Retirement on a continuing floor keeps the memory (Sol B4: kill -> retire -> reinstall).
    IxionRoomRetire("test-kill-memory");
    EXPECT_FALSE(IxionRoomInstalled());
    EXPECT_TRUE(IxionRoomKillRemembered(install.roster_keys[0]));
    EXPECT_EQ(IxionRoomDonorKillKeyFor(&second), ""); // nothing installed owns it now
    IxionRoomRecordDonorKill(&second, "e03a");          // an unowned death is not remembered twice
    // The native floor's monster population resets: a new lifetime forgets every kill.
    IxionRoomBeforeMonsterReset("test-floor-load");
    EXPECT_EQ(IxionRoomFloorLifetime(), lifetime + 1);
    EXPECT_FALSE(IxionRoomKillRemembered(install.roster_keys[0]));
}

TEST(IxionDc1Natives, HeightSinCosNearMonsterAndDeadReaction) {
    NativeWorld world;
    DonorEnv env("e104a,e42a");
    auto *saved_now = nowMonster;
    CActiveMonster &bat = world.monsters[0];
    CActiveMonster &dog = world.monsters[1];
    bat.monster_id = 0x7000;
    bat.chara_type = MONSTER_ACTIVE_MAX;
    dog.monster_id = 0x7001;
    dog.chara_type = MONSTER_ACTIVE_MAX + 1;
    Place(bat, 0.0f, 0.0f, 0.0f);
    Place(dog, 1.5584790683f * 30.0f, 0.0f, 0.0f); // 30 DC1 units away
    IxionDc1InstallNatives(&bat);
    IxionDc1InstallNatives(&dog);
    RS_STACKDATA out[4]{};
    RS_STACKDATA ptr[4]{};
    for (int i = 0; i < 4; ++i) { ptr[i].type = RS_PTR; ptr[i].val.p = &out[i]; }
    nowMonster = &bat;
    auto call = [&](int number, RS_STACKDATA *args, int argc) { return bat.mons_script.ext_func_table[number](args, argc); };
    // 112: DC2 ground height crosses the boundary divided by k.
    bat.height = 15.584790683f;
    EXPECT_EQ(call(112, ptr, 1), 1);
    EXPECT_NEAR(out[0].val.f, 10.0f, 0.001f);
    // 182 / 183: degrees.
    RS_STACKDATA angle[2]{};
    angle[0].type = RS_FLOAT;
    angle[0].val.f = 30.0f;
    angle[1] = ptr[0];
    call(182, angle, 2);
    EXPECT_NEAR(out[0].val.f, 0.5f, 1e-5f);
    angle[0].type = RS_INT;
    angle[0].val.i = 60;
    call(183, angle, 2);
    EXPECT_NEAR(out[0].val.f, 0.5f, 1e-5f);
    // 207: the nearest other live donor within 240 DC1 units, its position in DC1 units and index.
    EXPECT_EQ(call(207, ptr, 4), 1);
    EXPECT_NEAR(out[0].val.f, 30.0f, 0.01f);
    EXPECT_EQ(out[3].val.i, 1);
    dog.life = 0; // a dead monster is not a candidate (DC1 state 2 only)
    call(207, ptr, 4);
    EXPECT_EQ(out[3].val.i, -1);
    EXPECT_EQ(out[0].val.f, 0.0f);
    // Precheck P3: a dead donor refuses a thrown monster's stun; a live one and a native do not.
    EXPECT_TRUE(IxionDc1RefusesReaction(&dog));
    EXPECT_FALSE(IxionDc1RefusesReaction(&bat));
    bat.now_prog = MONSTER_PROG_DEAD;
    EXPECT_TRUE(IxionDc1RefusesReaction(&bat));
    CActiveMonster native_monster;
    native_monster.monster_id = 8;
    native_monster.life = 0;
    EXPECT_FALSE(IxionDc1RefusesReaction(&native_monster));
    IxionDc1Forget(&bat);
    IxionDc1Forget(&dog);
    nowMonster = saved_now;
}

TEST(IxionDc1Natives, MimicOpensOnConfirmWithinTheDc1EventReach) {
    // DC1 chest events: radius 8 about a point 8-10 units in front (dungeonmap.cpp:2616-2628).
    EXPECT_TRUE(IxionDc1MimicOpens(12.0f, 5.0f, true));
    EXPECT_TRUE(IxionDc1MimicOpens(13.0f, 5.0f, true));
    EXPECT_FALSE(IxionDc1MimicOpens(13.5f, 5.0f, true));
    EXPECT_FALSE(IxionDc1MimicOpens(3.0f, 5.0f, false)); // no press, no opening
    EXPECT_TRUE(IxionDc1MimicOpens(19.0f, 12.0f, true)); // King Mimic's 12-unit body
}

// ---- Monica faithful round 1: a player-owned goo ball from the DC1 shot code (ixion_monica_faithful.*) ----

TEST(IxionMonicaFaithful, EnabledOnlyWithBothVariablesAndTheSupportedForm) {
    const char *old_faithful = std::getenv("DC2_IXION_MONICA_FAITHFUL");
    const char *old_form = std::getenv("DC2_IXION_MONICA_FORM");
    const std::string saved_faithful = old_faithful != nullptr ? old_faithful : "";
    const std::string saved_form = old_form != nullptr ? old_form : "";
    dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    dc::test::UnsetEnv("DC2_IXION_MONICA_FORM");
    EXPECT_FALSE(IxionMonicaFaithfulEnabled());
    dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", "1", 1);
    testing::internal::CaptureStderr();
    EXPECT_FALSE(IxionMonicaFaithfulEnabled()); // the faithful switch alone does nothing
    EXPECT_NE(testing::internal::GetCapturedStderr().find("requires DC2_IXION_MONICA_FORM=e104a"), std::string::npos);
    dc::test::SetEnv("DC2_IXION_MONICA_FORM", "e42a", 1);
    EXPECT_FALSE(IxionMonicaFaithfulEnabled()); // only Opar's script is mapped
    dc::test::SetEnv("DC2_IXION_MONICA_FORM", "e104a", 1);
    EXPECT_TRUE(IxionMonicaFaithfulEnabled());
    dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", "0", 1);
    EXPECT_FALSE(IxionMonicaFaithfulEnabled());
    dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    EXPECT_FALSE(IxionMonicaFaithfulEnabled());
    if (old_faithful != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", saved_faithful.c_str(), 1);
    if (old_form != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FORM", saved_form.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_MONICA_FORM");
}

TEST(IxionMonicaFaithful, SpitGateFiresOnceAboveFrame103AndRearmsWhenTheMotionIsLeftOrWraps) {
    IxionMonicaSpitGate gate;
    EXPECT_FALSE(gate.Update(false, 200.0f)); // another motion never fires
    EXPECT_FALSE(gate.Update(true, 100.0f));
    EXPECT_FALSE(gate.Update(true, 103.0f)); // DC1's test is "> 103", not ">="
    EXPECT_TRUE(gate.Update(true, 103.5f));
    EXPECT_FALSE(gate.Update(true, 110.0f)); // once per visit
    EXPECT_FALSE(gate.Update(true, 120.0f));
    EXPECT_FALSE(gate.Update(false, 120.0f)); // 攻撃１out
    EXPECT_TRUE(gate.Update(true, 110.0f));   // the next Cross: the key starts at 110 (> 103) so it fires at once
    EXPECT_FALSE(gate.Update(true, 112.0f));
    IxionMonicaSpitGate wrapped;
    EXPECT_TRUE(wrapped.Update(true, 115.0f));
    EXPECT_TRUE(wrapped.Update(true, 110.0f)); // the frame went backwards without leaving the motion: a new swing
}

TEST(IxionMonicaFaithful, AimTakesTheNearestMonsterInFrontElseFliesStraightAhead) {
    const float from[4] = {0, 20, 0, 1};
    const float forward[2] = {0, 1}; // +z
    IxionMonicaCandidate list[3] = {
        {{0, 30, 90, 1}, 90.0f},   // in front, far
        {{0, 30, 60, 1}, 60.0f},   // in front, near: the pick
        {{0, 30, -20, 1}, 20.0f},  // behind: never
    };
    float out[4];
    int   chosen = -2;
    EXPECT_TRUE(IxionMonicaPickAim(from, forward, list, 3, 300.0f, 100.0f, out, &chosen));
    EXPECT_EQ(chosen, 1);
    EXPECT_FLOAT_EQ(out[2], 60.0f);
    EXPECT_FALSE(IxionMonicaPickAim(from, forward, list + 2, 1, 300.0f, 100.0f, out, &chosen));
    EXPECT_EQ(chosen, -1);
    EXPECT_FLOAT_EQ(out[0], 0.0f);
    EXPECT_FLOAT_EQ(out[1], 20.0f); // muzzle height
    EXPECT_FLOAT_EQ(out[2], 100.0f);
    EXPECT_FALSE(IxionMonicaPickAim(from, forward, list, 2, 50.0f, 100.0f, out, &chosen)); // out of reach
    EXPECT_FALSE(IxionMonicaPickAim(from, forward, list, 0, 300.0f, 100.0f, out, &chosen));
}

namespace {
// A rat in monster slot 0 and a stand-in for Max/Monica in player slot 0, each with one hit entry at its frame.
struct GooWorld {
    NativeWorld   world;
    mgCFrame      rat_frame;
    mgCFrame      player_frame;
    std::unique_ptr<CColPrimMan> saved_prims = std::make_unique<CColPrimMan>(ColPrimMan);
    CActiveMonster *saved_now = nowMonster;
    CActionChara  *saved_player = MainChara__2;
    int            saved_mode = DngStatus.mode;
    float          muzzle[4] = {};

    GooWorld() {
        dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
        MainChara__2 = nullptr;
        DngStatus.mode = DNG_STATUS_FIELD;
        world.scene->chara_num = 128;
        CActiveMonster &rat = world.monsters[0];
        CActiveMonster &player = world.monsters[1];
        world.scene->AssignChara(MONSTER_ACTIVE_MAX, &rat, const_cast<char *>("rat"));
        world.scene->SetType(SCENE_DATA_CHARA, MONSTER_ACTIVE_MAX, SCENE_CHARA_TYPE_MONSTER);
        world.scene->AssignChara(0, &player, const_cast<char *>("player"));
        world.scene->SetType(SCENE_DATA_CHARA, 0, SCENE_CHARA_TYPE_PLAYER);
        auto give_hit_entry = [](CActiveMonster &actor, mgCFrame &frame) {
            actor.entry_object[0].frame = &frame;
            actor.entry_object[0].group = CHARA_ENTRY_GROUP_HIT;
            actor.entry_object[0].enable = true;
            actor.entry_object[0].size = 10.0f;
        };
        give_hit_entry(rat, rat_frame);
        give_hit_entry(player, player_frame);
        rat_frame.GetWorldPosition0(muzzle); // both frames sit at the origin: a ball spawned there overlaps both
        muzzle[3] = 1.0f;
        ColPrimMan.Initialize(world.scene.get());
    }
    ~GooWorld() {
        IxionDc1RetirePlayerShots("test-teardown");
        ColPrimMan = *saved_prims;
        nowMonster = saved_now;
        MainChara__2 = saved_player;
        DngStatus.mode = saved_mode;
    }
};
} // namespace

TEST(IxionMonicaFaithful, PlayerGooIsOwnedByThePlayerHitsMonstersOnlyAndRollsNoGoo) {
    GooWorld g;
    const float aim[4] = {0, 0, 100, 1};
    testing::internal::CaptureStderr();
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(g.muzzle, aim, 35, 48.0f));
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 1);
    CColPrim &ball = ColPrimMan.prim[0];
    ASSERT_TRUE(ball.active);
    EXPECT_EQ(ball.owner, 0);                       // the player, not a monster slot
    EXPECT_EQ(ball.target, DAMAGE_TARGET_MONSTER);  // monsters only: BY_OWNER resolved for owner 0
    EXPECT_EQ(ball.status, 0u);                     // no goo status rides on it
    EXPECT_EQ(ball.damage, IxionDc1ScaledDamage(35, 48.0f, 146.0f)); // DC1 35 scaled from Monica's HP (friendly)
    EXPECT_EQ(ball.damage, 11);
    // The player-side character is never hit; the rat is, through DC2's own CheckHit.
    EXPECT_EQ(ColPrimMan.CheckHit(0), nullptr);
    EXPECT_EQ(ball.hit_num, 0);
    EXPECT_EQ(ColPrimMan.CheckHit(MONSTER_ACTIVE_MAX), &ball);
    EXPECT_EQ(ball.hit_num, 1);
    IxionDc1StepShots();
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_NE(log.find("owner=player target=monsters status=0"), std::string::npos) << log;
    EXPECT_NE(log.find("player shot hit"), std::string::npos) << log;
    EXPECT_NE(log.find("why=hit-monster"), std::string::npos) << log;
    EXPECT_EQ(log.find("goo roll"), std::string::npos) << log; // ApplyGoo never runs for a player ball
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 0);
    EXPECT_FALSE(ball.active);
}

TEST(IxionMonicaFaithful, FaithfulBalanceScalesFromTheCastersHpAndHasMinimumOne) {
    GooWorld g;
    dc::test::SetEnv("DC2_IXION_DC1_BALANCE", "faithful", 1);
    const float aim[4] = {0, 0, 100, 1};
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(g.muzzle, aim, 35, 48.0f));
    EXPECT_EQ(ColPrimMan.prim[0].damage, IxionDc1ScaledDamage(35, 48.0f, 70.0f)); // 22
    dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
    IxionDc1RetirePlayerShots("test");
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(g.muzzle, aim, 3, 48.0f));
    EXPECT_EQ(ColPrimMan.prim[0].damage, 1);
}

TEST(IxionMonicaFaithful, PlayerGooRetiresOnLifeFormExitRoomRetireAndSurvivesTheDonorLifecycle) {
    GooWorld g;
    const float far_aim[4] = {0, 0, 5000, 1};
    // Life: the DC1 flight is 180 frames (kShotLife), then the ball ends on its own.
    const float elsewhere[4] = {500, 500, 500, 1}; // away from both hit entries
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(elsewhere, far_aim, 35, 48.0f));
    testing::internal::CaptureStderr();
    int steps = 0;
    while (IxionDc1PlayerShotsLive() > 0 && steps < 400) {
        IxionDc1StepShots();
        ++steps;
    }
    std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(steps, 180);
    EXPECT_NE(log.find("why=life"), std::string::npos) << log;
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 0);

    // Form exit: with the switches on and no Himarra form active the step retires a live ball.
    const char *old_faithful = std::getenv("DC2_IXION_MONICA_FAITHFUL");
    const char *old_form = std::getenv("DC2_IXION_MONICA_FORM");
    const std::string saved_faithful = old_faithful != nullptr ? old_faithful : "";
    const std::string saved_form = old_form != nullptr ? old_form : "";
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(elsewhere, far_aim, 35, 48.0f));
    dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    IxionMonicaFaithfulStep(); // disabled: nothing happens, the ball is untouched
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 1);
    dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", "1", 1);
    dc::test::SetEnv("DC2_IXION_MONICA_FORM", "e104a", 1);
    testing::internal::CaptureStderr();
    IxionMonicaFaithfulStep(); // enabled, but MainChara__2 is null: not the form
    log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 0);
    EXPECT_NE(log.find("why=form-exit"), std::string::npos) << log;

    // Room retire.
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(elsewhere, far_aim, 35, 48.0f));
    testing::internal::CaptureStderr();
    IxionMonicaFaithfulRetire("test-room-retire");
    log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 0);
    EXPECT_NE(log.find("why=test-room-retire"), std::string::npos) << log;

    // The donor's lifecycle never ends Monica's ball, while it still ends the donor's own shots.
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e104a", 1);
    CActiveMonster &donor = g.world.monsters[0];
    donor.monster_id = 0x7000;
    donor.chara_type = MONSTER_ACTIVE_MAX;
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    RS_STACKDATA shot[5]{};
    shot[0].type = RS_STR;
    shot[0].val.s = const_cast<char *>("missing-frame");
    for (int i = 1; i < 4; ++i) { shot[i].type = RS_FLOAT; shot[i].val.f = 100; }
    shot[4].type = RS_INT;
    shot[4].val.i = 35;
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(elsewhere, far_aim, 35, 48.0f));
    CColPrim *player_ball = &ColPrimMan.prim[0];
    EXPECT_TRUE(donor.mons_script.ext_func_table[133](shot, 5));
    CColPrim *donor_ball = &ColPrimMan.prim[1];
    EXPECT_EQ(donor_ball->owner, donor.chara_type); // monster-owned: the old path, unchanged
    EXPECT_EQ(donor_ball->target, DAMAGE_TARGET_PLAYER);
    IxionDc1Despawn(&donor, "test-despawn");
    EXPECT_TRUE(player_ball->active);
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 1);
    EXPECT_FALSE(donor_ball->active);
    IxionDc1Forget(&donor);
    EXPECT_TRUE(player_ball->active);
    IxionDc1RetirePlayerShots("test");
    EXPECT_FALSE(player_ball->active);
    dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_MONSTER");
    if (old_faithful != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", saved_faithful.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    if (old_form != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FORM", saved_form.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_MONICA_FORM");
}

TEST(IxionMonicaFaithful, PlayerGooFreezesDuringEventPausesAndResumes) {
    GooWorld g;
    const float aim[4] = {0, 0, 100, 1};
    const float elsewhere[4] = {500, 500, 500, 1};
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(elsewhere, aim, 35, 48.0f));
    CColPrim &ball = ColPrimMan.prim[0];
    IxionDc1StepShots();
    const float moved = ball.pos[0][2];
    g.world.scene->battle_area.script.running = true; // an event script runs: DC1 shots freeze
    EXPECT_TRUE(IxionDc1UpdatePause());
    EXPECT_EQ(ball.target, 0);
    EXPECT_TRUE(IxionDc1PrimPaused(&ball));
    IxionDc1StepShots();
    EXPECT_FLOAT_EQ(ball.pos[0][2], moved);
    g.world.scene->battle_area.script.running = false;
    EXPECT_FALSE(IxionDc1UpdatePause());
    EXPECT_EQ(ball.target, DAMAGE_TARGET_MONSTER); // the monster mask returns, no other target appears
    IxionDc1StepShots();
    EXPECT_NE(ball.pos[0][2], moved);
}

// ---- Monica faithful round 2: belly-flop lunge, locked-on aim, counter-spit ----

TEST(IxionMonicaFaithful, LungeClockOpensAt138ClosesAfter149AndTravelsOnlyAcross135To146) {
    IxionMonicaLungeClock clock;
    float travel = 0.0f;
    int   opens = 0, closes = 0;
    const char *why = "";
    // Key 130-150, half a motion frame per tick (the real step is not whole).
    for (float frame = 130.0f; frame <= 150.0f; frame += 0.5f) {
        const IxionMonicaLungeStep step = clock.Update(true, frame);
        travel += step.move_frames;
        if (step.open) {
            ++opens;
            EXPECT_FLOAT_EQ(frame, 138.0f); // DC1's window starts on frame 138
        }
        if (step.close) {
            ++closes;
            why = step.close_why;
            EXPECT_FLOAT_EQ(frame, 149.5f); // the first tick past 149
        }
    }
    EXPECT_EQ(opens, 1);
    EXPECT_EQ(closes, 1);
    EXPECT_STREQ(why, "window-end");
    // 135..146 is 11 motion frames of 0.4 DC1 units: the 4.4 DC1 units of F038b8's _SET_MOVE.
    EXPECT_NEAR(travel, 11.0f, 1.0e-4f);
    EXPECT_NEAR(travel * kIxionMonicaLungeStep, 4.4f, 1.0e-3f);
    // It does not reopen in the same swing.
    EXPECT_FALSE(clock.Update(true, 150.0f).open);
}

TEST(IxionMonicaFaithful, LungeClockClosesWhenTheMotionIsLeftOrRewoundAndSkipsAnEntryPastTheWindow) {
    IxionMonicaLungeClock clock;
    EXPECT_FALSE(clock.Update(false, 140.0f).open); // another motion
    EXPECT_FALSE(clock.Update(true, 131.0f).open);
    EXPECT_TRUE(clock.Update(true, 139.0f).open);
    IxionMonicaLungeStep left = clock.Update(false, 140.0f); // hit out of the motion (damage reaction)
    EXPECT_TRUE(left.close);
    EXPECT_STREQ(left.close_why, "motion-left");
    EXPECT_FALSE(clock.Update(false, 141.0f).close); // already closed
    // The next Square is a new swing and opens again.
    EXPECT_FALSE(clock.Update(true, 131.0f).open);
    EXPECT_TRUE(clock.Update(true, 138.0f).open);
    // Rewind without leaving the motion: close, then a fresh swing.
    IxionMonicaLungeStep rewound = clock.Update(true, 131.0f);
    EXPECT_TRUE(rewound.close);
    EXPECT_STREQ(rewound.close_why, "rewind");
    EXPECT_FALSE(rewound.open);
    // Finish (a hit) ends the window for the rest of the swing.
    EXPECT_TRUE(clock.Update(true, 139.0f).open);
    clock.Finish();
    EXPECT_FALSE(clock.Update(true, 141.0f).open);
    EXPECT_FALSE(clock.Update(true, 142.0f).close);
    // Entering the motion already past 149 makes no window.
    IxionMonicaLungeClock late;
    EXPECT_FALSE(late.Update(true, 150.0f).open);
    EXPECT_FALSE(late.Update(true, 150.5f).open);
}

TEST(IxionMonicaFaithful, LockOnPicksTheLockedMonsterOnlyWhileLockedAndInTheList) {
    const int ids[3] = {130, 131, 135};
    EXPECT_EQ(IxionMonicaFindLocked(true, 131, ids, 3), 1);
    EXPECT_EQ(IxionMonicaFindLocked(true, 135, ids, 3), 2);
    EXPECT_EQ(IxionMonicaFindLocked(false, 131, ids, 3), -1); // not locked on: the nearest in front decides
    EXPECT_EQ(IxionMonicaFindLocked(true, 140, ids, 3), -1);  // a dead or absent target
    EXPECT_EQ(IxionMonicaFindLocked(true, -1, ids, 3), -1);
}

TEST(IxionMonicaFaithful, CounterSpitGateIsDc1Prog110FartherThan32DC1UnitsAndTenPercent) {
    const float k = kIxionDc1UnitScale;
    EXPECT_TRUE(IxionMonicaCounterGate(33.0f * k, 0));
    EXPECT_TRUE(IxionMonicaCounterGate(33.0f * k, 9));
    EXPECT_FALSE(IxionMonicaCounterGate(33.0f * k, 10)); // roll < 10, not <=
    EXPECT_FALSE(IxionMonicaCounterGate(32.0f * k, 0));  // distance > 32, not >=
    EXPECT_FALSE(IxionMonicaCounterGate(10.0f * k, 0));  // a bite's range: never
    EXPECT_TRUE(IxionMonicaCounterGate(80.0f, 0));       // the rat's 80-unit stand-off is 51 DC1 units: far enough
}

TEST(IxionMonicaFaithful, LungeRadiusIsDc1HalfSphereOrTheBodyContactRadiusWhicheverIsLarger) {
    EXPECT_NEAR(IxionMonicaLungeRadius(0.0f), 15.58f, 0.01f);   // a thin body keeps DC1's sphere
    EXPECT_FLOAT_EQ(IxionMonicaLungeRadius(30.0f), 38.0f);      // Monica's Opar body: width 30 -> 38
    // 2 x (radius + a 5-unit hit entry) must clear the 2 x (size 10 + width 30) = 80 collision separation.
    EXPECT_GE(2.0f * (IxionMonicaLungeRadius(30.0f) + 5.0f), 80.0f);
}

TEST(IxionMonicaFaithful, CounterSpitHookReturnsAtOnceUnlessTheFaithfulFormIsActive) {
    const char *old_faithful = std::getenv("DC2_IXION_MONICA_FAITHFUL");
    const char *old_form = std::getenv("DC2_IXION_MONICA_FORM");
    const std::string saved_faithful = old_faithful != nullptr ? old_faithful : "";
    const std::string saved_form = old_form != nullptr ? old_form : "";
    static int rolls;
    rolls = 0;
    IxionMonicaSetRollSourceForTest([](int limit) { ++rolls; return limit - limit; });
    CColPrim prim;
    prim.owner = MONSTER_ACTIVE_MAX;
    testing::internal::CaptureStderr();
    dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    IxionMonicaOnPlayerHit(&prim, 5); // switch off
    dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", "1", 1);
    dc::test::SetEnv("DC2_IXION_MONICA_FORM", "e104a", 1);
    IxionMonicaOnPlayerHit(&prim, 5); // switches on, but the player is not the Himarra form (no body)
    IxionMonicaOnPlayerHit(nullptr, 5);
    IxionMonicaOnPlayerHit(&prim, 0); // no damage taken
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(rolls, 0) << log; // no roll was spent
    EXPECT_EQ(log.find("counter-spit"), std::string::npos) << log;
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 0);
    IxionMonicaResetForTest();
    if (old_faithful != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FAITHFUL", saved_faithful.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_MONICA_FAITHFUL");
    if (old_form != nullptr) dc::test::SetEnv("DC2_IXION_MONICA_FORM", saved_form.c_str(), 1);
    else dc::test::UnsetEnv("DC2_IXION_MONICA_FORM");
}

TEST(IxionMonicaFaithful, LungeWindowIsPlayerOwnedKindThreeBlowHitsMonstersOnlyAndScalesFromCasterHp) {
    GooWorld g;
    testing::internal::CaptureStderr();
    CColPrim *window = IxionDc1OpenPlayerBlow(g.muzzle, 38.0f, 35, 48.0f);
    ASSERT_NE(window, nullptr);
    EXPECT_TRUE(IxionDc1PlayerBlowLive(window));
    EXPECT_EQ(window->owner, 0);                      // the player: BY_OWNER resolves to monsters only
    EXPECT_EQ(window->target, DAMAGE_TARGET_MONSTER);
    EXPECT_EQ(window->status, 0u);                    // no status rides on the flop
    EXPECT_EQ(window->damage, 11);                    // DC1 35 scaled from 48 HP, friendly (146)
    EXPECT_NE(window->param->hit_flags & DAMAGE_HIT_KNOCKDOWN, 0u); // DC1 kind 3: the blow knocks down
    EXPECT_EQ(ColPrimMan.CheckHit(0), nullptr);       // never the player
    EXPECT_EQ(window->hit_num, 0);
    EXPECT_EQ(ColPrimMan.CheckHit(MONSTER_ACTIVE_MAX), window); // the rat is hit through DC2's own CheckHit
    EXPECT_EQ(window->hit_num, 1);
    // Moving it away takes it off the rat; closing frees it and it is no longer live.
    const float away[4] = {900, 900, 900, 1};
    IxionDc1MovePlayerBlow(window, away, 38.0f);
    EXPECT_FLOAT_EQ(window->pos[0][0], 900.0f);
    IxionDc1ClosePlayerBlow(window);
    EXPECT_FALSE(window->active);
    EXPECT_FALSE(IxionDc1PlayerBlowLive(window));
    IxionDc1ClosePlayerBlow(window); // closing twice is harmless
    // Faithful balance: 22.
    dc::test::SetEnv("DC2_IXION_DC1_BALANCE", "faithful", 1);
    CColPrim *faithful = IxionDc1OpenPlayerBlow(g.muzzle, 38.0f, 35, 48.0f);
    ASSERT_NE(faithful, nullptr);
    EXPECT_EQ(faithful->damage, 22);
    EXPECT_FLOAT_EQ(IxionDc1BalanceHp(), 70.0f);
    IxionDc1ClosePlayerBlow(faithful);
    dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
    EXPECT_FLOAT_EQ(IxionDc1BalanceHp(), 146.0f);
    testing::internal::GetCapturedStderr();
}

TEST(IxionMonicaFaithful, PlayerGooSpawnLogsTheBalanceBasisAndFliesWithoutAGooModel) {
    GooWorld g;
    const float aim[4] = {0, 0, 100, 1};
    testing::internal::CaptureStderr();
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(g.muzzle, aim, 35, 48.0f)); // MainChara__2 is null: no effect manager
    dc::test::SetEnv("DC2_IXION_DC1_BALANCE", "faithful", 1);
    ASSERT_TRUE(IxionDc1SpawnPlayerShot(g.muzzle, aim, 35, 48.0f));
    dc::test::UnsetEnv("DC2_IXION_DC1_BALANCE");
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_NE(log.find("damage=11 (DC1 35, caster max hp 48, balance=friendly basis=146)"), std::string::npos) << log;
    EXPECT_NE(log.find("damage=22 (DC1 35, caster max hp 48, balance=faithful basis=70)"), std::string::npos) << log;
    EXPECT_NE(log.find("no player effect manager, placeholder sparks"), std::string::npos) << log;
    EXPECT_EQ(IxionDc1PlayerShotsLive(), 2);
}

// DRAN-R2: 111's port-owned event id is never the room's one accepted DC2 event (1200) nor raw DC1.
TEST(IxionDc1Dran, RunScriptMapsOnlyEvent510ToAPortOwnedId) {
    EXPECT_EQ(IxionDc1PortEventId(510), 30510);
    EXPECT_NE(IxionDc1PortEventId(510), 1200);
    EXPECT_NE(IxionDc1PortEventId(510), 510);
    EXPECT_EQ(IxionDc1PortEventId(1200), -1);
    EXPECT_EQ(IxionDc1PortEventId(500), -1);
    EXPECT_EQ(IxionDc1PortEventId(-1), -1);
}

// DRAN-R2: 115 ids (DC1 owner * 10 + attack) and 134 percent with DC1's defence floor and rounding.
TEST(IxionDc1Dran, HitIdsAndAttackerPercentFollowDc1) {
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_MAX_MELEE), 0);     // Toan's sword, basic_damage(0, 0)
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_MONICA_MELEE), 0);
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_MAX_GUN), 10);      // Xiao's pellet, SetUserID(1, 0)
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_LASER_GUN), 10);
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_GRENADE), 10);
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_MONICA_MAGIC), 30);
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_RIDEPOD_PUNCH), -1);
    EXPECT_EQ(IxionDc1HitIdFor(DAMAGE_KIND_MONSTER), -1);
    // defence + ceil(max(1, prim - defence) * percent / 100)
    EXPECT_EQ(IxionDc1PercentPrimDamage(36, 10, 100), 36);
    EXPECT_EQ(IxionDc1PercentPrimDamage(28, 10, 5), 11);  // net 18 at 5% -> 0.9 -> 1
    EXPECT_EQ(IxionDc1PercentPrimDamage(8, 10, 100), 11); // DC1's minimum of 1
    EXPECT_EQ(IxionDc1PercentPrimDamage(7, 10, 5), 11);
    EXPECT_EQ(IxionDc1PercentPrimDamage(30, 10, 0), 10);  // 0% takes nothing
    EXPECT_EQ(IxionDc1PercentPrimDamage(210, 10, 5), 20); // net 200 at 5% -> 10
}

// DRAN-R2: effect 13 (f_boll_2) is a supported shot with DC1's flight and impact radii.
TEST(IxionDc1Dran, FireballEffect13RowIsDc1) {
    float speed = 0, radius = 0, impact = 0;
    int life = 0, damage = 0;
    ASSERT_TRUE(IxionDc1ShotEffectForTest(13, speed, radius, life, damage, impact));
    EXPECT_FLOAT_EQ(speed, 2.5f);  // phase 1: motion[0] = -1 skips phase 0's 0.1
    EXPECT_FLOAT_EQ(radius, 4.0f);
    EXPECT_EQ(life, 180);
    EXPECT_EQ(damage, 35);
    EXPECT_FLOAT_EQ(impact, 20.0f); // radius[2]
    EXPECT_FALSE(IxionDc1ShotEffectForTest(12, speed, radius, life, damage, impact));
}

// DRAN-R2: natives 202, 108, 116, 134, 115 and 111 on an installed donor.
TEST(IxionDc1Dran, MotionFrameColOffLockonBodyParaDamageIdAndRunScript) {
    NativeWorld world;
    DonorEnv env("e104a");
    auto *saved_now = nowMonster;
    CActiveMonster &donor = world.monsters[0];
    donor.monster_id = 0x7000;
    donor.chara_type = MONSTER_ACTIVE_MAX;
    donor.target_no = 0;
    donor.defense = 10;
    donor.muteki_time = 0;
    donor.attrib = 0;
    donor.now_key = nullptr;
    Place(donor, 0.0f, 0.0f, 0.0f);
    mgCFrame root, bcol;
    root.SetName(const_cast<char *>("root"));
    bcol.SetName(const_cast<char *>("bcol0"));
    root.SetChild(&bcol);
    mgCFrame *saved_frame = donor.CObjectFrame::frame;
    donor.CObjectFrame::frame = &root;
    for (auto &entry : donor.entry_object) entry = CHARA_ENTRY_OBJECT{nullptr, 0.0f, -1, 0};
    IxionDc1InstallNatives(&donor);
    nowMonster = &donor;
    auto call = [&](int number, RS_STACKDATA *args, int argc) { return donor.mons_script.ext_func_table[number](args, argc); };
    RS_STACKDATA out[2]{};
    RS_STACKDATA ptr[2]{};
    for (int i = 0; i < 2; ++i) { ptr[i].type = RS_PTR; ptr[i].val.p = &out[i]; }
    auto integer = [](int value) { RS_STACKDATA s{}; s.type = RS_INT; s.val.i = value; return s; };
    // 202: the current motion time (no key selected: the raw frame).
    donor.frame = 187.5f;
    ASSERT_EQ(call(202, ptr, 1), 1);
    EXPECT_FLOAT_EQ(out[0].val.f, 187.5f);
    // 108: NO_MAP_HIT | NO_BODY_HIT for exactly n thinks, then the bits go.
    RS_STACKDATA off[1] = {integer(3)};
    call(108, off, 1);
    EXPECT_EQ(IxionDc1ColOffForTest(&donor), 3);
    EXPECT_EQ(donor.attrib & (MONSTER_ATTRIB_NO_MAP_HIT | MONSTER_ATTRIB_NO_BODY_HIT),
              (u32) (MONSTER_ATTRIB_NO_MAP_HIT | MONSTER_ATTRIB_NO_BODY_HIT));
    IxionDc1AfterScript(&donor);
    IxionDc1AfterScript(&donor);
    EXPECT_NE(donor.attrib & MONSTER_ATTRIB_NO_MAP_HIT, 0u);
    IxionDc1AfterScript(&donor);
    EXPECT_EQ(IxionDc1ColOffForTest(&donor), 0);
    EXPECT_EQ(donor.attrib & (MONSTER_ATTRIB_NO_MAP_HIT | MONSTER_ATTRIB_NO_BODY_HIT), 0u);
    // 116 records; 130 + 134(11, 5): attacker 1 (Xiao) takes 5%, everyone else 100%.
    RS_STACKDATA dist[1] = {integer(600)};
    EXPECT_EQ(call(116, dist, 1), 1);
    RS_STACKDATA body[2]{};
    body[0].type = RS_STR;
    body[0].val.s = const_cast<char *>("bcol0");
    body[1].type = RS_FLOAT;
    body[1].val.f = 8.0f;
    call(130, body, 2);
    RS_STACKDATA para[2] = {integer(11), integer(5)};
    call(134, para, 2);
    EXPECT_EQ(IxionDc1BodyAttackerPercent(&donor, 0, 1), 5);
    EXPECT_EQ(IxionDc1BodyAttackerPercent(&donor, 0, 0), 100);
    int entry = -1;
    for (int i = 0; i < CHARA_ENTRY_OBJECT_MAX; ++i)
        if (donor.entry_object[i].frame == &bcol && donor.entry_object[i].group == CHARA_ENTRY_GROUP_HIT) entry = i;
    ASSERT_GE(entry, 0);
    // 115: -1 until a hit; a gun hit on that sphere is DC1 id 10 at 5%, a melee hit id 0 at 100%.
    call(115, ptr, 1);
    EXPECT_EQ(out[0].val.i, -1);
    EXPECT_EQ(IxionDc1NoteBodyHit(&donor, DAMAGE_KIND_MAX_GUN, entry, 28), 11);
    call(115, ptr, 1);
    EXPECT_EQ(out[0].val.i, 10);
    EXPECT_EQ(IxionDc1NoteBodyHit(&donor, DAMAGE_KIND_MAX_MELEE, entry, 36), 36);
    call(115, ptr, 1);
    EXPECT_EQ(out[0].val.i, 0);
    // DC1 records no hit id while invincible.
    donor.muteki_time = 30;
    IxionDc1NoteBodyHit(&donor, DAMAGE_KIND_MAX_GUN, entry, 28);
    call(115, ptr, 1);
    EXPECT_EQ(out[0].val.i, 0);
    donor.muteki_time = 0;
    // 111: an unmapped event runs nothing; 510 raises the arena boss flag (no DC2 event_no write).
    IxionRoomArenaResetForTest();
    RS_STACKDATA unknown[1] = {integer(777)};
    EXPECT_EQ(call(111, unknown, 1), 1);
    EXPECT_EQ(IxionRoomArenaBossFlag(), 0);
    RS_STACKDATA dran[1] = {integer(510)};
    EXPECT_EQ(call(111, dran, 1), 1);
    EXPECT_EQ(IxionRoomArenaBossFlag(), 100);
    EXPECT_EQ(donor.state, ACTIVE_MONSTER_LIVE); // alive: no DC2 death outside the death program
    IxionRoomArenaResetForTest();
    IxionDc1Forget(&donor);
    donor.CObjectFrame::frame = saved_frame;
    nowMonster = saved_now;
}
