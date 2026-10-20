#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dng_event.hpp"
#include "dng_main.hpp"
#include "event_func.hpp"
#include "event.hpp"
#include "scene.hpp"
#include "ixion_room.hpp"
#include "ixion_room_props.hpp"
#include "platform_fixture.hpp"

namespace {

// Removes the per-case temp directories when the case ends (ctest runs cases as parallel processes).
struct TempDirs {
    std::vector<std::filesystem::path> dirs;
    ~TempDirs() {
        std::error_code ec;
        for (const auto &dir : dirs) std::filesystem::remove_all(dir, ec);
    }
};
TempDirs temp_dirs;

std::filesystem::path TempFile(const char *tag, const std::string &text) {
    const auto dir = std::filesystem::temp_directory_path() /
                     std::format("ct-ixion-props-{}-{}-{}", tag,
                                 testing::UnitTest::GetInstance()->current_test_info()->name(), dc::test::ProcessId());
    std::filesystem::create_directories(dir);
    temp_dirs.dirs.push_back(dir);
    const auto path = dir / "props.json";
    std::ofstream(path, std::ios::binary) << text;
    return path;
}

const char *kProps = R"({"format":"ixion-room-props","version":1,"room":"d01inter",
 "props":[{"id":"atla-0","kind":"atla","position":[244.68,0,202.6],"rotation_y":0},
          {"id":"treasure_box-0","kind":"treasure_box","position":[-144.9385533489984,0,-495.5963437094784],
           "rotation_y":0,"contents":{"dc1_item":195,"role":"gate_key","count":1}}],"deferred":[]})";

IxionRoom Room() {
    IxionRoom room;
    room.donor_room = "d01inter";
    room.bounds_min = {-387.4f, -10.0f, -625.0f};
    room.bounds_max = {487.1f, 60.0f, 375.6f};
    return room;
}

std::unique_ptr<CTreasureBoxManager> Manager() {
    auto boxes = std::make_unique<CTreasureBoxManager>();
    boxes->Initialize();
    return boxes;
}

TEST(IxionRoomProps, LoadsSidecarAndRejectsWrongVersionOrMissingItem) {
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("ok", kProps), props, error)) << error;
    ASSERT_EQ(props.props.size(), 2u);
    EXPECT_EQ(props.room, "d01inter");
    EXPECT_EQ(props.props[1].kind, "treasure_box");
    EXPECT_EQ(props.props[1].dc1_item, 195);
    EXPECT_FLOAT_EQ(props.props[1].position[2], -495.59634f);

    std::string bad = kProps;
    bad.replace(bad.find("\"version\":1"), 11, "\"version\":2");
    EXPECT_FALSE(IxionRoomProps::Load(TempFile("version", bad), props, error));
    std::string no_item = kProps;
    no_item.replace(no_item.find("\"dc1_item\":195"), 14, "\"item\":195");
    EXPECT_FALSE(IxionRoomProps::Load(TempFile("item", no_item), props, error));
    EXPECT_EQ(props.props.size(), 2u); // a failed load leaves the previous output untouched
}

TEST(IxionRoomProps, ItemTableMapsTheDc1GateKeyToTheNativeDc2GateKey) {
    EXPECT_EQ(IxionRoomPropsMapItem(195, 0), 337);
    EXPECT_EQ(IxionRoomPropsMapItem(195, 0), GetGateKeyIndex(0, 1));
    EXPECT_EQ(IxionRoomPropsMapItem(195, 1), -1);
    EXPECT_EQ(IxionRoomPropsMapItem(194, 0), -1);
}

TEST(IxionRoomProps, WrongSchemaTypesDisableWithoutThrowingOrReplacingOutput) {
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("valid", kProps), props, error));
    for (const char *value : {"0", "null", "[]", "{}", "true"}) {
        std::string bad = kProps;
        bad.replace(bad.find("\"ixion-room-props\""), 18, value);
        EXPECT_NO_THROW(EXPECT_FALSE(IxionRoomProps::Load(TempFile("wrong-type", bad), props, error)));
        EXPECT_EQ(props.room, "d01inter");
    }
}

TEST(IxionRoomProps, ContinuingFloorRetirementKeepsOpenedIdentityAtNativePickupBoundary) {
    auto boxes = Manager();
    auto scene = std::make_unique<CScene>();
    auto *saved_scene = DngMainScene;
    auto *saved_event = EventScene;
    DngMainScene = EventScene = scene.get();
    scene->battle_area.treasure_box = boxes.get();
    const auto path = TempFile("continuing", kProps);
    dc::test::SetEnv("DC2_IXION_ROOM", (path.parent_path() / "room.json").string().c_str(), 1);
    IxionRoomPropsResetForTest();
    float near[4] = {-144.94f, 0, -495.6f, 1};
    IxionRoomPropsInstall(Room(), {}, 0);
    ASSERT_EQ(boxes->CheckEvent(near, 30), 0);
    // Unopened retirement remains eligible for the native pickup query.
    IxionRoomPropsRetire(true);
    IxionRoomPropsInstall(Room(), {}, 0);
    ASSERT_EQ(boxes->CheckEvent(near, 30), 0);
    CRunScript script;
    SetEventFunc(&script);
    RS_STACKDATA opened{};
    opened.type = RS_INT;
    opened.val.i = 2;
    ASSERT_EQ(script.ext_func_table[EVENT_EXT_SET_TB_STATUS](&opened, 1), 1);
    ASSERT_EQ(boxes->CheckEvent(near, 30), -1);
    IxionRoomPropsRetire(true);
    IxionRoomPropsInstall(Room(), {}, 0);
    EXPECT_EQ(boxes->CheckEvent(near, 30), -1); // cannot initiate a second native opening/grant
    IxionRoomPropsBeginFloor();
    IxionRoomPropsInstall(Room(), {}, 0);
    EXPECT_EQ(boxes->CheckEvent(near, 30), 0); // new logical floor has a new consumption lifetime
    IxionRoomPropsDiscard();
    IxionRoomPropsResetForTest();
    dc::test::UnsetEnv("DC2_IXION_ROOM");
    DngMainScene = saved_scene;
    EventScene = saved_event;
}

TEST(IxionRoomProps, PlacesANativeBoxInAFreeUnreservedSlotAndRemovesOnlyOurUnopenedBox) {
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("place", kProps), props, error)) << error;
    auto boxes = Manager();
    float native[4] = {731.1f, 40.0f, 3373.8f, 1.0f};
    boxes->PutTreasureBox(0, native, 0.0f, 1, 10, 1, -1, 0); // a live native box
    // Slot 1 is a native box the room parked (state NONE, owed back on retire): reserved.
    std::vector<IxionRoomPlacedBox> placed;
    ASSERT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {1}, placed), 1);
    ASSERT_EQ(placed.size(), 1u);
    EXPECT_EQ(placed[0].slot, 2);
    const CTreasureBox &box = boxes->box[2];
    EXPECT_EQ(box.state, TREASURE_BOX_STATE_UNOPENED);
    EXPECT_EQ(box.item[0], 337);
    EXPECT_EQ(box.num[0], 1);
    EXPECT_EQ(box.item[1], -1);
    EXPECT_EQ(box.flags, 0x41);
    EXPECT_EQ(boxes->box[1].state, TREASURE_BOX_STATE_NONE);
    // The native pickup query finds our box from 20 units away (30-unit radius, dng_main.cpp:2961-2967).
    float near_pos[4] = {-144.94f + 20.0f, 0.0f, -495.6f, 1.0f};
    EXPECT_EQ(boxes->CheckEvent(near_pos, 30.0f), 2);

    IxionRoomPropsRemove(boxes.get(), placed);
    EXPECT_TRUE(placed.empty());
    EXPECT_EQ(boxes->box[2].state, TREASURE_BOX_STATE_NONE);
    EXPECT_EQ(boxes->box[0].state, TREASURE_BOX_STATE_UNOPENED); // native box untouched
}

TEST(IxionRoomProps, RetireLeavesOpenedOrReusedSlotsAndAFullPoolPlacesNothing) {
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("retire", kProps), props, error)) << error;
    auto boxes = Manager();
    std::vector<IxionRoomPlacedBox> placed;
    ASSERT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {}, placed), 1);
    const int slot = placed[0].slot;
    // Reused: something else now owns the slot.
    float other[4] = {5.0f, 0.0f, 5.0f, 1.0f};
    boxes->PutTreasureBox(slot, other, 0.0f, 1, 20, 1, -1, 0);
    IxionRoomPropsRemove(boxes.get(), placed);
    EXPECT_EQ(boxes->box[slot].state, TREASURE_BOX_STATE_UNOPENED);
    EXPECT_EQ(boxes->box[slot].item[0], 20);

    // Opened natively: the open script leaves the slot live with a script-set state and the lid
    // open (runs\props-retire-opened). It is still ours and is cleared.
    boxes->Initialize();
    ASSERT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {}, placed), 1);
    boxes->box[placed[0].slot].state = 2;
    boxes->box[placed[0].slot].lid_open = 2.0f;
    IxionRoomPropsRemove(boxes.get(), placed);
    EXPECT_EQ(boxes->box[0].state, TREASURE_BOX_STATE_NONE);
    EXPECT_EQ(boxes->box[0].lid_open, 0.0f);

    // Already empty: nothing to do.
    ASSERT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {}, placed), 1);
    boxes->box[placed[0].slot].state = TREASURE_BOX_STATE_NONE;
    IxionRoomPropsRemove(boxes.get(), placed);
    EXPECT_EQ(boxes->box[0].state, TREASURE_BOX_STATE_NONE);

    // Scene changed: tracking dropped without a manager.
    ASSERT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {}, placed), 1);
    IxionRoomPropsRemove(nullptr, placed);
    EXPECT_TRUE(placed.empty());

    // Every slot full: nothing placed, nothing overwritten.
    boxes->Initialize();
    float spot[4] = {1.0f, 0.0f, 1.0f, 1.0f};
    for (int i = 0; i < TREASURE_BOX_MAX; ++i) boxes->PutTreasureBox(i, spot, 0.0f, 1, 7, 1, -1, 0);
    EXPECT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 0, {}, placed), 0);
    EXPECT_TRUE(placed.empty());
    for (int i = 0; i < TREASURE_BOX_MAX; ++i) EXPECT_EQ(boxes->box[i].item[0], 7);

    // Unmapped dungeon: skipped.
    boxes->Initialize();
    EXPECT_EQ(IxionRoomPropsPlace(boxes.get(), props, Room(), 3, {}, placed), 0);
}

// A random-floor pack (RANDFLOOR-R2): chests carry an authored DC2 stand-in for DC1 items the
// built-in table does not know, and DC1's own monster arrangement rides along as monster_spawn.
const char *kFloorProps = R"({"format":"ixion-room-props","version":1,"room":"d01inter",
 "props":[{"id":"treasure_box-0","kind":"treasure_box","position":[10,0,10],"rotation_y":0,
           "contents":{"dc1_item":233,"count":1,"dc2_item":305,"mapping":"equivalent"}},
          {"id":"treasure_box-1","kind":"treasure_box","position":[20,0,20],"rotation_y":0,
           "contents":{"dc1_item":195,"count":1,"dc2_item":268}},
          {"id":"treasure_box-2","kind":"treasure_box","position":[30,0,30],"rotation_y":0,
           "contents":{"dc1_item":169,"count":1}},
          {"id":"monster-1","kind":"monster_spawn","position":[40,0,40],"donor":{"model":"e03a","slot":1}},
          {"id":"monster-2","kind":"monster_spawn","position":[9000,0,40],"donor":{"model":"e06a","slot":2}}]})";

TEST(IxionRoomProps, AuthoredStandInItemsAreResolvedAndTheGateKeyStaysNative) {
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("floor", kFloorProps), props, error)) << error;
    ASSERT_EQ(props.props.size(), 5u);
    EXPECT_EQ(props.props[0].dc2_item, 305);
    EXPECT_EQ(props.props[3].kind, "monster_spawn");
    EXPECT_EQ(props.props[3].model, "e03a");
    EXPECT_EQ(props.props[3].slot, 1);
    EXPECT_EQ(IxionRoomPropsResolveItem(props.props[0], 0), 305);
    EXPECT_EQ(IxionRoomPropsResolveItem(props.props[1], 0), 337); // the built-in gate key wins over dc2_item
    EXPECT_EQ(IxionRoomPropsResolveItem(props.props[2], 0), -1);  // unmapped and no stand-in

    auto boxes = Manager();
    std::vector<IxionRoomPlacedBox> placed;
    IxionRoom room = Room();
    // Only the two resolvable chests are placed; monster_spawn entries are not boxes.
    EXPECT_EQ(IxionRoomPropsPlace(boxes.get(), props, room, 0, {}, placed), 2);
    ASSERT_EQ(placed.size(), 2u);
    EXPECT_EQ(boxes->box[placed[0].slot].item[0], 305);
    EXPECT_EQ(boxes->box[placed[1].slot].item[0], 337);
}

TEST(IxionRoomProps, AuthoredItemAndMonsterFieldsAreValidated) {
    IxionRoomProps props;
    std::string error;
    for (const char *value : {"0", "429", "-3", "\"x\"", "1.5"}) {
        std::string text = kFloorProps;
        text.replace(text.find("\"dc2_item\":305"), 14, std::string("\"dc2_item\":") + value);
        EXPECT_FALSE(IxionRoomProps::Load(TempFile("badreward", text), props, error)) << value;
    }
    for (const char *value : {"\"\"", "\"e03a/../x\"", "\"abcdefghijklmnop\"", "7"}) {
        std::string text = kFloorProps;
        text.replace(text.find("\"model\":\"e03a\""), 14, std::string("\"model\":") + value);
        EXPECT_FALSE(IxionRoomProps::Load(TempFile("badmodel", text), props, error)) << value;
    }
    std::string no_donor = kFloorProps;
    no_donor.replace(no_donor.find("\"donor\":{\"model\":\"e03a\",\"slot\":1}"), 33, "\"x\":1");
    EXPECT_FALSE(IxionRoomProps::Load(TempFile("nodonor", no_donor), props, error));
}

TEST(IxionRoomProps, MonsterSpawnsFollowTheRoomAndStayInsideItsBounds) {
    const auto path = TempFile("spawns", kFloorProps);
    const std::string room_json = (path.parent_path() / "room.json").string();
#ifdef _WIN32
    ASSERT_EQ(_putenv_s("DC2_IXION_ROOM", room_json.c_str()), 0);
    ASSERT_EQ(_putenv_s("DC2_IXION_ROOM_PROPS", ""), 0);
#else
    ASSERT_EQ(setenv("DC2_IXION_ROOM", room_json.c_str(), 1), 0);
    ASSERT_EQ(unsetenv("DC2_IXION_ROOM_PROPS"), 0);
#endif
    IxionRoomPropsResetForTest();
    const auto spawns = IxionRoomPropsMonsterSpawns(Room());
    ASSERT_EQ(spawns.size(), 1u); // monster-2 is at x=9000, outside the room bounds
    EXPECT_EQ(spawns[0].model, "e03a");
    EXPECT_EQ(spawns[0].slot, 1);
    EXPECT_FLOAT_EQ(spawns[0].position[0], 40.0f);
    IxionRoom other = Room();
    other.donor_room = "another-room";
    EXPECT_TRUE(IxionRoomPropsMonsterSpawns(other).empty());
    IxionRoomPropsResetForTest();
#ifdef _WIN32
    _putenv_s("DC2_IXION_ROOM", "");
#else
    unsetenv("DC2_IXION_ROOM");
#endif
}

TEST(IxionRoomProps, ParkingIsOptInHidesOverlappingNativeBoxesAndRestoresOnlyWhatItMoved) {
    std::string park = kFloorProps;
    park.replace(park.find("\"room\":\"d01inter\""), 17, "\"room\":\"d01inter\",\"native_boxes\":\"park\"");
    IxionRoomProps props;
    std::string error;
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("park", park), props, error)) << error;
    EXPECT_EQ(props.native_boxes, "park");
    std::string bad = park;
    bad.replace(bad.find("\"native_boxes\":\"park\""), 22, "\"native_boxes\":\"drop\"");
    EXPECT_FALSE(IxionRoomProps::Load(TempFile("badpark", bad), props, error));
    ASSERT_TRUE(IxionRoomProps::Load(TempFile("nopark", kFloorProps), props, error)) << error;
    EXPECT_TRUE(props.native_boxes.empty());

    // Park and restore on an explicit manager: two native boxes inside the room, one outside.
    auto boxes = Manager();
    float inside_a[4] = {100.0f, 0.0f, 100.0f, 1.0f};
    float inside_b[4] = {-300.0f, 10.0f, 300.0f, 1.0f};
    float outside[4] = {9000.0f, 0.0f, 9000.0f, 1.0f};
    boxes->PutTreasureBox(0, inside_a, 0.0f, 1, 10, 1, -1, 0);
    boxes->PutTreasureBox(1, outside, 0.0f, 1, 11, 1, -1, 0);
    boxes->PutTreasureBox(2, inside_b, 0.0f, 1, 12, 1, -1, 0);
    std::vector<IxionRoomParkedBox> parked;
    ASSERT_EQ(IxionRoomPropsParkBoxes(boxes.get(), Room(), parked), 2);
    ASSERT_EQ(parked.size(), 2u);
    EXPECT_EQ(boxes->box[0].state, TREASURE_BOX_STATE_UNOPENED); // the slot stays occupied: nothing can post into it
    float near_pos[4] = {100.0f, 0.0f, 100.0f, 1.0f};
    EXPECT_EQ(boxes->CheckEvent(near_pos, 40.0f), -1);           // out of the pickup test
    float where[4] = {};
    boxes->box[0].GetPosition(where);
    EXPECT_LT(where[1], -1.0e5f);
    float other[4] = {};
    boxes->box[1].GetPosition(other);
    EXPECT_FLOAT_EQ(other[0], 9000.0f);                           // the outside box is untouched
    // A parked slot that something else took over is left alone; the other is restored exactly.
    float intruder[4] = {5.0f, 0.0f, 5.0f, 1.0f};
    boxes->PutTreasureBox(2, intruder, 0.0f, 1, 99, 1, -1, 0);
    IxionRoomPropsRestoreBoxes(boxes.get(), parked);
    EXPECT_TRUE(parked.empty());
    boxes->box[0].GetPosition(where);
    EXPECT_FLOAT_EQ(where[0], 100.0f);
    EXPECT_FLOAT_EQ(where[1], 0.0f);
    EXPECT_FLOAT_EQ(where[2], 100.0f);
    EXPECT_EQ(boxes->CheckEvent(near_pos, 40.0f), 0);
    boxes->box[2].GetPosition(where);
    EXPECT_FLOAT_EQ(where[0], 5.0f);
    EXPECT_EQ(boxes->box[2].item[0], 99);
}

} // namespace
