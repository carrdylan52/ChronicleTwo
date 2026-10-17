#include <gtest/gtest.h>

#include <memory>

#include "mainloop.hpp"
#include "mainloop_port.hpp"
#include "map.hpp"
#include "map_port.hpp"
#include "mapload.hpp"
#include "mapparts.hpp"
#include "savedata.hpp"

namespace {
constexpr int kStarlightTemple = 26;

// Starlight Temple's two Fruit of Eden chests that retail data both give map flag 7.
class StarlightChests : public testing::Test {
protected:
    std::unique_ptr<CSaveData>         save = std::make_unique<CSaveData>();
    CSaveData                         *previous = PortActiveSaveData;
    std::unique_ptr<CMapTreasureBox[]> boxes = std::make_unique<CMapTreasureBox[]>(2);
    CFuncPoint                         points[2];
    char                               weapon_name[8] = "item19";
    char                               items_name[8] = "item11";
    CMap                               map;

    void SetUp() override {
        PortActiveSaveData = save.get();
        for (int map_no = 0; map_no < SAVE_MAP_FLAG_MAX; ++map_no) {
            *save->GetMapFlag(map_no) = {};
        }
        points[0].name = weapon_name;
        points[1].name = items_name;
        for (int i = 0; i < 2; ++i) {
            points[i].flag_no = 7;
            points[i].enable = 1;
            boxes[i].active = 1;
            boxes[i].flag_no = 7;
            boxes[i].func_point = &points[i];
            boxes[i].parts = nullptr;
        }
        map.tr_box = boxes.get();
        map.tr_box_num = 2;
    }

    void TearDown() override {
        map.tr_box = nullptr;
        map.tr_box_num = 0;
        PortActiveSaveData = previous;
    }
};
} // namespace

TEST(MapTreasureFlag, OnlyStarlightItem11MovesToFlag9) {
    EXPECT_EQ(PortTreasureBoxFlag(kStarlightTemple, "item11", 7), 9);
    EXPECT_EQ(PortTreasureBoxFlag(kStarlightTemple, "item19", 7), 7);
    EXPECT_EQ(PortTreasureBoxFlag(kStarlightTemple, "item11", 5), 5);
    EXPECT_EQ(PortTreasureBoxFlag(kStarlightTemple - 1, "item11", 7), 7);
    EXPECT_EQ(PortTreasureBoxFlag(kStarlightTemple, nullptr, 7), 7);
}

TEST_F(StarlightChests, OpeningTheWeaponChestEarlyLeavesTheWindGemChest) {
    CMapFlagData *flags = save->GetMapFlag(kStarlightTemple);
    map.UpdateTrBoxFlag(flags);
    EXPECT_EQ(boxes[0].flag_no, 7);
    EXPECT_EQ(boxes[1].flag_no, 9);

    map.DeleteTrBox(0, flags);
    EXPECT_TRUE(flags->GetFlag(7));
    EXPECT_FALSE(flags->GetFlag(9));

    // The next visit to the town builds the chests from the function points again.
    boxes[0].flag_no = boxes[1].flag_no = 7;
    boxes[1].active = 1;
    map.UpdateTrBoxFlag(flags);
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 1);
    EXPECT_EQ(points[1].enable, 1);

    map.DeleteTrBox(1, flags);
    EXPECT_TRUE(flags->GetFlag(9));
    boxes[1].flag_no = 7;
    map.UpdateTrBoxFlag(flags);
    EXPECT_EQ(boxes[1].active, 0);
}

TEST_F(StarlightChests, OtherMapsKeepTheirRetailFlags) {
    CMapFlagData *flags = save->GetMapFlag(kStarlightTemple - 1);
    flags->SetFlag(7, 1);
    map.UpdateTrBoxFlag(flags);
    EXPECT_EQ(boxes[1].flag_no, 7);
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 0);
}
