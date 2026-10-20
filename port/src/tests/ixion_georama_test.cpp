#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include "collision.hpp"
#include "dataread_port.hpp"
#include "editdata.hpp"
#include "editloop.hpp"
#include "editmenu.hpp"
#include "editmap.hpp"
#include "editriver.hpp"
#include "gameutil.hpp"
#include "ixion_georama.hpp"
#include "villagermngr.hpp"
#include "mainloop_port.hpp"
#include "mapselect.hpp"
#include "menudraw.hpp"
#include "mg_memory.hpp"
#include "platform/paths.hpp"
#include "stubs/editmenu.hpp"
#include "savedata.hpp"

extern int SandboxMapNo asm("EditLoop_MapNo");

class IxionGeorama : public testing::Test {
protected:
    std::unique_ptr<CSaveData> live = std::make_unique<CSaveData>();
    CSaveData                 *previous = PortActiveSaveData;
    std::filesystem::path      root;

    static void OptIn(bool value) {
#ifdef _WIN32
        _putenv_s("DC2_IXION_GEORAMA_SANDBOX", value ? "1" : "");
#else
        setenv("DC2_IXION_GEORAMA_SANDBOX", value ? "1" : "", 1);
#endif
    }

    void SetUp() override {
        PortActiveSaveData = live.get();
        OptIn(false);
        IxionGeoramaPrepare(0);
        const char *base = std::getenv("IXION_GEORAMA_TEST_ROOT");
        root = (base ? std::filesystem::path(base) : std::filesystem::temp_directory_path()) /
               ("georama-r1-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        ASSERT_TRUE(std::filesystem::create_directory(root));
        PathsSetSaveRoot(root);
    }

    void TearDown() override {
        OptIn(false);
        IxionGeoramaPrepare(0);
        PortActiveSaveData = previous;
        PathsReset();
        std::filesystem::remove_all(root); // This fixture created this unique directory.
    }

    void MapTable() {
        std::filesystem::create_directories(root / "data/map");
        std::ofstream file(root / "data/map/map0.cfg");
        file << "MAP_NAME_NUM 6;\n";
        for (int i = 0; i < 5; ++i) {
            file << "MAP_NAME \"g0" << i + 1 << "\",\"town\",\"\",1,1;\n";
        }
        file << "MAP_NAME \"gix\",\"sandbox\",\"\",1,1;\n";
        file.close();
        DataReadIndex(root / "data");
        alignas(16) u_long128 buffer[2048];
        LoadMapName(0, buffer);
    }
};

TEST_F(IxionGeorama, OptOutPreservesCampaignSlotsAndRejectsOtherIndices) {
    ASSERT_FALSE(IxionGeoramaEnabled());
    CSaveData *save = GetSaveData();
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(save->GetEditData(i), &save->edit_data[i]);
    }
    EXPECT_EQ(save->GetEditData(-1), nullptr);
    EXPECT_EQ(save->GetEditData(5), nullptr);
    EXPECT_EQ(save->GetEditData(6), nullptr);
    EXPECT_EQ(save->GetEditData(300), nullptr);
}

TEST_F(IxionGeorama, NullAnalysisDoesNotReadOrWriteCampaignRecords) {
    CSaveData                 *save = GetSaveData();
    std::vector<unsigned char> before(sizeof(save->edit_data));
    std::memcpy(before.data(), save->edit_data, before.size());
    for (int map : {-1, 5, 6, 188}) {
        SandboxMapNo = map;
        KeepEditAnalyze();
        EXPECT_EQ(EditAnalyzeChanged(), 0);
    }
    EXPECT_EQ(std::memcmp(before.data(), save->edit_data, before.size()), 0);
    SandboxMapNo = 0;
}

TEST_F(IxionGeorama, SparseMenuPartsAndMissingTitleAreSafe) {
    CMenuPosDataForm   form{};
    MENUFORMPARTS_TYPE parts[3]{};
    parts[1].name = const_cast<char *>("Title0");
    form.parts = parts;
    form.parts_num = 3;
    EXPECT_EQ(form.GetPartInfo(const_cast<char *>("Title0")), &parts[1]);
    EXPECT_EQ(form.GetPartInfo(const_cast<char *>("Title5")), nullptr);
    EXPECT_EQ(form.GetPartInfo(nullptr), nullptr);
    form.SetPartDrawFlag(const_cast<char *>("Title5"), true);
    EXPECT_EQ(parts[1].draw_flag, 0);
}

TEST_F(IxionGeorama, EnabledSlotIsSeparateAndOtherSaveObjectsCannotAccessIt) {
    MapTable();
    std::ofstream(root / "ixion-georama-sandbox.marker") << "ixion-georama-sandbox-v1\n";
    std::vector<unsigned char> campaign(sizeof(live->edit_data));
    std::memcpy(campaign.data(), live->edit_data, campaign.size());
    OptIn(true);
    IxionGeoramaPrepare(5);
    ASSERT_TRUE(IxionGeoramaEnabled());
    auto *private_data = live->GetEditData(5);
    ASSERT_NE(private_data, nullptr);
    private_data->parts[0].id = 50;
    private_data->parts[0].angle = 7;
    private_data->parts[0].pos[0] = 321;
    auto other = std::make_unique<CSaveData>();
    EXPECT_EQ(other->GetEditData(5), nullptr);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(live->GetEditData(i), &live->edit_data[i]);
    }
    EXPECT_EQ(live->GetEditData(6), nullptr);
    EXPECT_EQ(std::memcmp(campaign.data(), live->edit_data, campaign.size()), 0);
    EXPECT_EQ(GetMaxPolyn(5), GetMaxPolyn(0));
    EXPECT_EQ(GetMaxDrawMem(5), GetMaxDrawMem(0));
}

TEST_F(IxionGeorama, MenuAnnouncementsAndLaterUiWritesNeverAliasTownZero) {
    MapTable();
    std::ofstream(root / "ixion-georama-sandbox.marker") << "ixion-georama-sandbox-v1\n";
    std::vector<unsigned char> campaign(sizeof(live->edit_data));
    std::memcpy(campaign.data(), live->edit_data, campaign.size());
    OptIn(true);
    IxionGeoramaPrepare(5);
    int count = 123, sub = 123, height = 123;
    // The sandbox branch needs no loaded campaign analysis, scene, or font stack.
    EXPECT_EQ(MakeDownLoadAnaunce(5, nullptr, &count, &sub, &height), 0);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(sub, 0);
    EXPECT_EQ(height, 0);
    ASSERT_EQ(MenuAnalyzeData, &live->GetEditData(5)->analyze);
    EXPECT_EQ(MenuEditAnalyzeSrc, nullptr);
    EXPECT_EQ(MenuEditAnalyzeDataSrc[0], nullptr);
    MenuAnalyzeData->data_open[0] = 1;
    MenuAnalyzeData->condition_open[5] = 1;
    EXPECT_EQ(live->GetEditData(5)->analyze.data_open[0], 1);
    EXPECT_EQ(std::memcmp(campaign.data(), live->edit_data, campaign.size()), 0);
}

TEST_F(IxionGeorama, MarkerAndUniqueMapAreRequired) {
    MapTable();
    OptIn(true);
    ASSERT_EXIT(IxionGeoramaPrepare(5), testing::ExitedWithCode(2), "disposable save marker");
    std::ofstream(root / "ixion-georama-sandbox.marker") << "ixion-georama-sandbox-v1\n";
    ASSERT_EXIT(IxionGeoramaPrepare(0), testing::ExitedWithCode(2), "requires map 5");
}

TEST_F(IxionGeorama, PreviewScaleKeepsRetailValueExceptForZeroSandboxEntries) {
    // Disabled: retail 0.85 * table for everything, zero stays zero (native ids unchanged).
    EXPECT_EQ(PortGeoramaPreviewScale(4, 2.0f, 0.2f), 0.85f * 0.2f);
    EXPECT_EQ(PortGeoramaPreviewScale(50, 2.0f, 0.0f), 0.0f);
    MapTable();
    std::ofstream(root / "ixion-georama-sandbox.marker") << "ixion-georama-sandbox-v1\n";
    OptIn(true);
    IxionGeoramaPrepare(5);
    ASSERT_TRUE(IxionGeoramaEnabled());
    const int saved_map = SandboxMapNo;
    SandboxMapNo = 5;
    EXPECT_EQ(PortGeoramaPreviewScale(4, 2.0f, 0.2f), 0.85f * 0.2f);
    EXPECT_EQ(PortGeoramaPreviewScale(48, 2.0f, 0.0f), 0.0f); // native id with a retail zero
    for (int id : {50, 51, 52, 54, 55, 56, 58}) {
        EXPECT_EQ(PortGeoramaPreviewScale(id, 2.0f, 0.0f), 2.0f) << id;
        EXPECT_EQ(PortGeoramaPreviewScale(id, 2.0f, 0.3f), 0.85f * 0.3f) << id;
    }
    SandboxMapNo = 0;
    EXPECT_EQ(PortGeoramaPreviewScale(50, 2.0f, 0.0f), 0.0f);
    SandboxMapNo = saved_map;
}

TEST_F(IxionGeorama, SidecarRoundTripAndCorruptionLeaveDestinationUntouched) {
    CEditData data;
    data.Initialize();
    data.save_count = 3;
    data.parts[0].id = 50;
    data.parts[0].state = 1;
    data.parts[0].angle = 1;
    data.parts[0].pos[0] = 250;
    std::array<short, 128> stock{};
    stock[50] = 3;
    const auto path = root / "ixion-georama-v1.bin";
    ASSERT_TRUE(IxionGeoramaWrite(path, data, stock));
    CEditData loaded;
    loaded.Initialize();
    std::array<short, 128> read_stock{};
    std::string            error;
    ASSERT_TRUE(IxionGeoramaRead(path, loaded, read_stock, error)) << error;
    EXPECT_EQ(std::memcmp(&data, &loaded, sizeof(data)), 0);
    EXPECT_EQ(read_stock, stock);
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    file.seekp(30);
    file.put('\xFF');
    file.close();
    ASSERT_FALSE(IxionGeoramaRead(path, loaded, read_stock, error));
    EXPECT_EQ(std::memcmp(&data, &loaded, sizeof(data)), 0);
    data.parts_max = 301;
    EXPECT_FALSE(IxionGeoramaWrite(root / "bad.bin", data, stock));
    EXPECT_FALSE(std::filesystem::exists(root / "bad.bin"));
}

TEST_F(IxionGeorama, SavePreflightRejectsNativeBufferOverflows) {
    CEditMap map;
    map.edit_parts_max = 256;
    map.place_log_max = 0;
    map.grid_max = 1;
    CEditGrid grid;
    map.grid[0] = &grid;
    grid.num_x = 20;
    grid.num_z = 20;
    EXPECT_TRUE(IxionGeoramaSaveSafe(map));
    grid.num_x = 255;
    grid.num_z = 255;
    EXPECT_FALSE(IxionGeoramaSaveSafe(map));
    map.grid_max = 0;
    map.edit_parts_max = 300;
    EXPECT_FALSE(IxionGeoramaSaveSafe(map));
    map.edit_parts_max = 256;
    map.place_log_max = 2049;
    EXPECT_FALSE(IxionGeoramaSaveSafe(map));
    map.place_log_max = 0;
    map.grid_max = 5;
    EXPECT_FALSE(IxionGeoramaSaveSafe(map));
    map.grid_max = 0;
}

TEST_F(IxionGeorama, ConvertedTreeLoadsInNativeCollisionAndBlocksWalking) {
    const char *path = std::getenv("IXION_GEORAMA_WORLD_MDS");
    if (!path) {
        GTEST_SKIP() << "private world_collision.mds fixture required";
    }
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(input.good());
    auto size = input.tellg();
    ASSERT_GT(size, 128);
    std::vector<u_long128> bytes((static_cast<std::size_t>(size) + 15) / 16);
    input.seekg(0);
    input.read(reinterpret_cast<char *>(bytes.data()), size);
    ASSERT_TRUE(input.good());
    std::vector<u_long128> arena(8192);
    mgCMemory              memory;
    memory.stSetBuffer(arena.data(), static_cast<int>(arena.size()));
    auto *frame = LoadCollisionFile(reinterpret_cast<MDS_HEADER *>(bytes.data()), &memory);
    ASSERT_NE(frame, nullptr);
    auto *collision = static_cast<CCollisionMDT *>(frame->collision);
    ASSERT_NE(collision, nullptr);
    ASSERT_EQ(collision->poly_count, 32);
    for (int i = 0; i < collision->poly_count; ++i) {
        EXPECT_EQ(collision->poly[i].area_kind, 2);
        EXPECT_EQ(collision->poly[i].ignore_mask, 0);
    }
    // Native Max's town radius is 13. Advance in normal walking-sized steps.
    sceVu0FVECTOR position = {0, 10, -120, 1};
    MoveCheckInfo move;
    move.Initialize();
    move.radius = 13;
    for (int step = 0; step < 60; ++step) {
        sceVu0FVECTOR velocity = {0, 0, 5, 0};
        sceVu0FVECTOR next;
        MoveCheck(position, velocity, next, &move, collision->poly, collision->poly_count, 1);
        std::memcpy(position, next, sizeof(position));
    }
    EXPECT_LT(position[2], 0) << "native walking crossed the central trunk's collision volume";
    EXPECT_GT(position[2], -120) << "native walking never approached the tree";
    sceVu0FVECTOR baseline = {0, 10, -120, 1};
    MoveCheckInfo clear;
    clear.Initialize();
    clear.radius = 13;
    for (int step = 0; step < 60; ++step) {
        sceVu0FVECTOR velocity = {0, 0, 5, 0};
        sceVu0FVECTOR next;
        MoveCheck(baseline, velocity, next, &clear, nullptr, 0, 1);
        std::memcpy(baseline, next, sizeof(baseline));
    }
    EXPECT_GT(baseline[2], 150);
    std::fprintf(stderr, "ixion georama test: native collision stopped walk at z=%.3f\n", position[2]);
}

TEST_F(IxionGeorama, VillagerFilterOnlyActsInsideTheEnabledSandbox) {
    CVillagerPlaceInfo  a, b, c;
    int                 ids[3] = {5, 420, 7};
    CVillagerPlaceInfo *places[3] = {&a, &b, &c};
    // Sandbox off: every row survives untouched, so campaign villagers and non-sandbox runs are unchanged.
    ASSERT_FALSE(IxionGeoramaEnabled());
    EXPECT_EQ(IxionGeoramaFilterVillagers(5, ids, places, 3), 3);
    EXPECT_EQ(ids[1], 420);

    // Sandbox on with a roster naming villager 420 on part 51, but no scene and no placed house: 420 is dropped and
    // the other rows keep their order.
    MapTable();
    std::filesystem::create_directories(root / "data");
    std::ofstream(root / "data/ix_villagers.txt") << "# header\n420 51 70.487 10.0 -12.296 1.5708 ix_p01a\n";
    std::ofstream(root / "data/ix_vtext.txt") << "420\tFighting is what a man should live for.\\nIt's a basic fact of life, right?\n";
    std::ofstream(root / "ixion-georama-sandbox.marker") << "ixion-georama-sandbox-v1\n";
    DataReadIndex(root / "data");
    OptIn(true);
    IxionGeoramaPrepare(5);
    ASSERT_TRUE(IxionGeoramaEnabled());
    int ids2[3] = {5, 420, 7};
    CVillagerPlaceInfo *places2[3] = {&a, &b, &c};
    EXPECT_EQ(IxionGeoramaFilterVillagers(5, ids2, places2, 3), 2);
    EXPECT_EQ(ids2[0], 5);
    EXPECT_EQ(ids2[1], 7);
    EXPECT_EQ(places2[1], &c);
}
