#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string_view>

#include "map.hpp"
#include "map_port.hpp"
#include "mapload.hpp"
#include "mapparts.hpp"

namespace {

/**
 *
 * Starlight Temple's two Fruit of Eden chests, which retail data both give opened flag 7.
 *
 */
class StarlightChests : public testing::Test {
protected:
    std::unique_ptr<CMapTreasureBox[]> boxes = std::make_unique<CMapTreasureBox[]>(2);
    std::array<CFuncPoint, 2>          points;
    char                               weapon_name[8] = "item19";
    char                               items_name[8] = "item11";
    CMapFlagData                       flags = {};
    CMap                               map;

    void SetUp() override {
        points[0].name = weapon_name;
        points[1].name = items_name;
        map.tr_box = boxes.get();
        map.tr_box_num = 2;
    }

    void TearDown() override {
        map.tr_box = nullptr;
        map.tr_box_num = 0;
    }

    // As CMap::CreateTrBox does on each visit, every chest takes its flag from its function point again.
    void Visit(std::string_view map_name) {
        for (int i = 0; i < 2; ++i) {
            points[i].flag_no = 7;
            points[i].enable = 1;
            boxes[i].active = 1;
            boxes[i].flag_no = points[i].flag_no;
            boxes[i].func_point = &points[i];
            boxes[i].parts = nullptr;
        }
        PortSplitTreasureBoxFlags(map, map_name);
        map.UpdateTrBoxFlag(&flags);
    }
};

/** Three placements of one Starlight Temple part, whose item10 points all start with opened flag 3. */
class PlacedStarlightChests : public testing::Test {
protected:
    static constexpr std::array<int, 3> kFlags = {3, 21, 22};

    std::unique_ptr<CMapTreasureBox[]> boxes = std::make_unique<CMapTreasureBox[]>(3);
    std::array<CMapParts, 3>          parts;
    std::array<CFuncPoint, 3>         points;
    char                              placement_names[3][32] = {"p01_f0205-0", "p01_f0205-1", "p01_f0205-2"};
    char                              definition_name[32] = "p01_f0205";
    char                              point_name[8] = "item10";
    CMapFlagData                      flags = {};
    CMap                              map;

    void SetUp() override {
        for (int i = 0; i < 3; ++i) {
            parts[i].SetName(placement_names[i]);
            parts[i].SetPartsName(definition_name);
            points[i].name = point_name;
        }
        map.tr_box = boxes.get();
        map.tr_box_num = 3;
    }

    void TearDown() override {
        map.tr_box = nullptr;
        map.tr_box_num = 0;
    }

    void Visit(std::array<int, 3> order = {0, 1, 2}, std::string_view map_name = "f02") {
        // A new visit reconstructs each placement's point and chest from the same retail definition.
        for (int i = 0; i < 3; ++i) {
            const int placement = order[i];
            points[placement].flag_no = 3;
            points[placement].enable = 1;
            boxes[i].active = 1;
            boxes[i].flag_no = points[placement].flag_no;
            boxes[i].func_point = &points[placement];
            boxes[i].parts = &parts[placement];
        }
        PortSplitTreasureBoxFlags(map, map_name);
        map.UpdateTrBoxFlag(&flags);
    }

    int BoxIndex(int placement) const {
        for (int i = 0; i < 3; ++i) {
            if (boxes[i].parts == &parts[placement]) {
                return i;
            }
        }
        return -1;
    }
};

} // namespace

TEST_F(StarlightChests, OpeningTheWeaponChestLeavesTheWindGemChest) {
    Visit("f02");
    EXPECT_EQ(boxes[0].flag_no, 7);
    EXPECT_EQ(boxes[1].flag_no, 9);
    map.DeleteTrBox(0, &flags);
    EXPECT_TRUE(flags.GetFlag(7));
    EXPECT_FALSE(flags.GetFlag(9));

    Visit("f02");
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 1);

    map.DeleteTrBox(1, &flags);
    Visit("f02");
    EXPECT_EQ(boxes[1].active, 0);
}

TEST_F(StarlightChests, OpeningTheWindGemChestLeavesTheWeaponChest) {
    Visit("f02");
    map.DeleteTrBox(1, &flags);
    Visit("f02");
    EXPECT_EQ(boxes[0].active, 1);
    EXPECT_EQ(boxes[1].active, 0);
}

TEST_F(StarlightChests, OtherMapsKeepTheirRetailFlags) {
    flags.SetFlag(7, 1);
    Visit("f01");
    EXPECT_EQ(boxes[1].flag_no, 7);
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 0);
}

TEST_F(StarlightChests, LegacyFlagSevenLeavesTheWindGemChestAvailable) {
    flags.SetFlag(7, 1);
    Visit("f02");
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 1);
    EXPECT_FALSE(flags.GetFlag(9));
}

TEST_F(PlacedStarlightChests, EachPlacementKeepsItsOpenedStateWhenChestOrderChanges) {
    for (int opened = 0; opened < 3; ++opened) {
        SCOPED_TRACE(opened);
        flags = {};
        Visit();
        map.DeleteTrBox(BoxIndex(opened), &flags);

        Visit({2, 0, 1});
        for (int placement = 0; placement < 3; ++placement) {
            const CMapTreasureBox &box = boxes[BoxIndex(placement)];
            EXPECT_EQ(box.flag_no, kFlags[placement]);
            EXPECT_EQ(flags.GetFlag(kFlags[placement]), placement == opened);
            EXPECT_EQ(box.active, placement != opened);
            EXPECT_EQ(points[placement].enable, placement != opened);
            EXPECT_EQ(points[placement].flag_no, 3);
        }
    }
}

TEST_F(PlacedStarlightChests, AllThreeCanBeCollectedAcrossSavedFlagCopies) {
    Visit();
    for (int placement : {1, 0, 2}) {
        SCOPED_TRACE(placement);
        ASSERT_EQ(boxes[BoxIndex(placement)].active, 1);
        map.DeleteTrBox(BoxIndex(placement), &flags);

        const CMapFlagData saved_flags = flags;
        flags = {};
        flags = saved_flags;
        Visit({2, 1, 0});
        EXPECT_EQ(boxes[BoxIndex(placement)].active, 0);
    }
    for (int placement = 0; placement < 3; ++placement) {
        EXPECT_TRUE(flags.GetFlag(kFlags[placement]));
        EXPECT_EQ(boxes[BoxIndex(placement)].active, 0);
    }
}

TEST_F(PlacedStarlightChests, LegacyFlagThreeLeavesTheOtherPlacementsAvailable) {
    flags.SetFlag(3, 1);
    Visit();
    EXPECT_EQ(boxes[0].active, 0);
    EXPECT_EQ(boxes[1].active, 1);
    EXPECT_EQ(boxes[2].active, 1);
    EXPECT_FALSE(flags.GetFlag(21));
    EXPECT_FALSE(flags.GetFlag(22));
}

TEST_F(PlacedStarlightChests, TemporaryFloorGateDoesNotRecordAnOpenedChest) {
    Visit();
    map.DeleteTrBox(1, nullptr);
    EXPECT_EQ(boxes[1].active, 0);
    EXPECT_EQ(points[1].enable, 0);
    for (int flag : kFlags) {
        EXPECT_FALSE(flags.GetFlag(flag));
    }

    Visit();
    EXPECT_EQ(boxes[1].active, 1);
    EXPECT_EQ(points[1].enable, 1);
}

TEST_F(PlacedStarlightChests, NormalizationIsRepeatableAndPreservesOtherFlags) {
    Visit();
    PortSplitTreasureBoxFlags(map, "f02");
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(boxes[i].flag_no, kFlags[i]);
    }

    boxes[1].flag_no = 6;
    boxes[2].flag_no = 0;
    PortSplitTreasureBoxFlags(map, "f02");
    EXPECT_EQ(boxes[1].flag_no, 6);
    EXPECT_EQ(boxes[2].flag_no, 0);
}

TEST_F(PlacedStarlightChests, UnrelatedMapsPlacementsAndPointsKeepRetailFlags) {
    Visit({0, 1, 2}, "f01");
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(boxes[i].flag_no, 3);
    }

    char unrelated_placement[] = "p01_f0206-1";
    char unrelated_point[] = "item12";
    parts[1].SetName(unrelated_placement);
    points[2].name = unrelated_point;
    Visit();
    EXPECT_EQ(boxes[0].flag_no, 3);
    EXPECT_EQ(boxes[1].flag_no, 3);
    EXPECT_EQ(boxes[2].flag_no, 3);
}

TEST_F(PlacedStarlightChests, MissingMetadataKeepsRetailFlags) {
    Visit({0, 1, 2}, "f01");
    boxes[0].func_point = nullptr;
    boxes[1].parts = nullptr;
    points[2].name = nullptr;
    PortSplitTreasureBoxFlags(map, "f02");
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(boxes[i].flag_no, 3);
    }
}
