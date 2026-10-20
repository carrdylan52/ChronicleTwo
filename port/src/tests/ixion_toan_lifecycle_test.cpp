#include <gtest/gtest.h>
#include <array>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "actionchara.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dataread_port.hpp"
#include "dng_main.hpp"
#include "dng_event.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "mainloop.hpp"
#include "mainloop_port.hpp"
#include "menuchr.hpp"
#include "menumain.hpp"
#include "menusys.hpp"
#include "menudraw.hpp"
#include "mg_memory.hpp"
#include "scene.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "sceneload.hpp"
#include "userdata.hpp"
#include "stubs/character.hpp"
#include "stubs/event_func.hpp"
#include "ixion_toan_body.hpp"
#include "ixion_toan_combat.hpp"
#include "ixion_dc1_toan.h"
#include "platform_fixture.hpp"

TEST(IxionBodyEnv, UnknownValueLogsAndRefuses) {
    EXPECT_EXIT({
        dc::test::SetEnv("DC2_IXION_PLAYER_BODY", "unsupported", 1);
        const bool enabled = IxionToanBodyEnabled();
        std::_Exit(enabled ? 1 : 0);
    }, testing::ExitedWithCode(0), "expected toan\\|goro\\|ungaga\\|xiao");
}

void InitEyeCamera(CActionChara *);
int _LOAD_CHARA_sub(int, char **, int, u32 *, int);

namespace {
// Authored test script, not disc data. Native skin parser can process its image declaration.
std::vector<u32> Pack(const char *script) {
    const size_t header = 3 * sizeof(PACK_ENTRY);
    const size_t length = std::strlen(script) + 1;
    std::vector<u32> bytes((header + length + 32 + 3) / 4);
    auto *raw = reinterpret_cast<char *>(bytes.data());
    PACK_ENTRY entry{};
    std::strcpy(entry.name, "info.cfg");
    entry.offset = header;
    entry.size = length;
    entry.next = sizeof(PACK_ENTRY);
    std::memcpy(raw, &entry, sizeof(entry));
    std::strcpy(entry.name, "pixel.bin");
    entry.offset = header + length - sizeof(PACK_ENTRY);
    entry.size = 7;
    std::memcpy(raw + sizeof(PACK_ENTRY), &entry, sizeof(entry));
    std::memcpy(raw + header, script, length);
    return bytes;
}

class IxionToanLifecycle : public testing::Test {
protected:
    CScene scene;
    std::array<CActionChara, 6> parts;
    std::unique_ptr<CSaveData> save = std::make_unique<CSaveData>();
    CSaveData *old_save = PortActiveSaveData;
    std::array<CHRINFO_KEY_SET, 88> keys{};
    CScene *old_scene = DngMainScene, *old_menu = MenuMainScene;
    CActionChara *old_main = MainChara__2;
    DNG_BATTLE_AREA *old_area = BattleAreaScene;
    decltype(DngStatus) old_status = DngStatus;
    decltype(MenuLoadInfo) old_load = MenuLoadInfo;

    void SetUp() override {
        // The named CTest cases launch with the opt-ins; keep ordinary test processes native.
        if (!IxionToanCombatDrivesKeys()) GTEST_SKIP() << "requires Toan-enabled test process";
        PortActiveSaveData = save.get();
        MainChara__2 = &parts[0];
        DngMainScene = MenuMainScene = &scene;
        BattleAreaScene = &scene.battle_area;
        DngStatus.eye_view = 0;
        DngStatus.mode = DNG_STATUS_FIELD;
        for (int i = 0; i < 6; ++i) {
            parts[i].Initialize(nullptr);
            scene.AssignChara(i, &parts[i], const_cast<char *>("test"));
        }
        for (auto &key : keys) { key.start_frame = 10; key.end_frame = 20; key.step = .25f; }
        keys[36].start_frame = 820; keys[36].end_frame = 830;
        parts[0].key_list[0] = keys.data(); parts[0].key_num[0] = keys.size();
        parts[0].move_check.landed = true;
        IxionToanBodyRegister(&parts[0], &parts[1], &scene);
        ColPrimMan.Initialize(&scene);
    }
    void TearDown() override {
        parts[0].Initialize(nullptr);
        IxionToanCombatRetire(nullptr, "test-end");
        ColPrimMan.Initialize(nullptr);
        MainChara__2 = old_main; DngMainScene = old_scene; MenuMainScene = old_menu;
        BattleAreaScene = old_area; DngStatus = old_status; MenuLoadInfo = old_load;
        PortActiveSaveData = old_save;
    }
    void Tick(float time, bool down = false) {
        parts[0].frame = time;
        ToanCombatFrame frame{};
        ToanCombatBegin(&parts[0], frame);
        frame.cross_down = down; frame.cross_held = down;
        ToanCombatTick(&parts[0], frame);
    }
    CColPrim *Open() {
        Tick(10, true); Tick(825);
        for (auto &prim : ColPrimMan.prim) if (prim.active) return &prim;
        return nullptr;
    }
    void PressR2() {
        const auto old_pad = GamePad__2;
        const auto old_debug = DebugFlag;
        auto *old_mark = RedMarkModel;
        CRedMarkModel mark;
        RedMarkModel = &mark;
        DebugFlag = 0;
        parts[0].menu_flag = true;
        GamePad__2.key_lock = 0;
        GamePad__2.previous_pad[0].button = 0;
        GamePad__2.pad[0].button = PAD_R2;
        DngMainKey(); // entry and exit both return before RunScript
        GamePad__2 = old_pad; DebugFlag = old_debug; RedMarkModel = old_mark;
    }
};

TEST_F(IxionToanLifecycle, R2EntryClosesOpenWindowWithoutAnotherScriptTick) {
    auto *prim = Open(); ASSERT_NE(prim, nullptr);
    const auto old_pad = GamePad__2;
    const auto old_debug = DebugFlag;
    auto *old_mark = RedMarkModel;
    CRedMarkModel mark;
    RedMarkModel = &mark;
    DebugFlag = 0;
    parts[0].menu_flag = true;
    GamePad__2.key_lock = 0;
    GamePad__2.previous_pad[0].button = 0;
    GamePad__2.pad[0].button = PAD_R2;
    DngMainKey(); // real R2 dispatch enters eye view and returns before RunScript
    GamePad__2 = old_pad; DebugFlag = old_debug; RedMarkModel = old_mark;
    EXPECT_TRUE(DngStatus.eye_view);
    EXPECT_FALSE(prim->active);
    IxionToanHud hud{};
    EXPECT_FALSE(IxionToanCombatHud(&hud));
    ToanCombatFrame frame{}; ToanCombatBegin(&parts[0], frame);
    EXPECT_FALSE(frame.active);
}

TEST_F(IxionToanLifecycle, R2RoundTripClosesWindowAndPreservesSpentGauge) {
    Tick(10, true); Tick(820); Tick(821, true); Tick(829); Tick(833);
    CColPrim *prim = nullptr;
    for (auto &candidate : ColPrimMan.prim) if (candidate.active) prim = &candidate;
    ASSERT_NE(prim, nullptr);
    IxionToanHud before{};
    ASSERT_TRUE(IxionToanCombatHud(&before));
    ASSERT_LT(before.gauge, 100);
    const auto generation = IxionToanBodyGeneration(&parts[0]);
    const auto incarnation = IxionToanPrimIncarnation(prim);
    PressR2();
    EXPECT_TRUE(DngStatus.eye_view);
    EXPECT_FALSE(prim->active);
    EXPECT_NE(IxionToanPrimIncarnation(prim), incarnation);
    EXPECT_FALSE(IxionToanCombatOwnsPrim(prim));
    PressR2();
    EXPECT_FALSE(DngStatus.eye_view);
    EXPECT_EQ(IxionToanBodyGeneration(&parts[0]), generation);
    ToanCombatFrame frame{};
    ToanCombatBegin(&parts[0], frame);
    EXPECT_TRUE(frame.active);
    IxionToanHud returned{};
    EXPECT_TRUE(IxionToanCombatHud(&returned));
    EXPECT_FLOAT_EQ(returned.gauge, before.gauge);
    EXPECT_EQ(returned.action, 0);
    frame.cross_down = frame.cross_held = 0;
    ToanCombatTick(&parts[0], frame);
    EXPECT_TRUE(IxionToanCombatHud(&returned));
    EXPECT_EQ(returned.action, 0);
    EXPECT_NEAR(returned.gauge, before.gauge + 70.0f / 30.0f * .25f, 1e-5f);
    for (const auto &candidate : ColPrimMan.prim) EXPECT_FALSE(candidate.active);
}

TEST_F(IxionToanLifecycle, R2RoundTripPreservesSpentGaugeAndCooldown) {
    // Spend 25 through the real combo debit, finish the second swing, then
    // end a fresh first swing while its previously spent gauge is still in place.
    Tick(10, true); Tick(820); Tick(821, true); Tick(829); Tick(19);
    // End a fresh first swing with the previously spent gauge still in place;
    // the real donor handler installs a 15-tick cooldown.
    Tick(10, true); Tick(825); Tick(829);
    IxionToanHud before{}, returned{};
    ASSERT_TRUE(IxionToanCombatHud(&before));
    ASSERT_LT(before.gauge, 100);
    ASSERT_EQ(before.action, 0);
    PressR2();
    EXPECT_TRUE(DngStatus.eye_view);
    ToanCombatFrame parked{}; ToanCombatBegin(&parts[0], parked);
    EXPECT_FALSE(parked.active);
    PressR2();
    EXPECT_FALSE(DngStatus.eye_view);
    ToanCombatFrame frame{}; ToanCombatBegin(&parts[0], frame);
    EXPECT_TRUE(frame.active);
    EXPECT_TRUE(IxionToanCombatHud(&returned));
    EXPECT_FLOAT_EQ(returned.gauge, before.gauge); // R2 does not tick upkeep
    frame.cross_down = frame.cross_held = 1;
    ToanCombatTick(&parts[0], frame); // resumed production path must refuse Cross
    EXPECT_TRUE(IxionToanCombatHud(&returned));
    EXPECT_EQ(returned.action, 0);
    const float refill = 70.0f / 30.0f * .25f;
    EXPECT_NEAR(returned.gauge, before.gauge + refill, 1e-5f);
    // Read the next donor output: only two normal upkeep ticks have elapsed
    // since the handler installed 15; neither R2 dispatch may clear or age it.
    IxionDc1ToanInput input{}; input.time = 10;
    IxionDc1ToanOutput output{};
    ixion_dc1_toan_tick(&input, &output);
    EXPECT_EQ(output.cooldown, 13);
    EXPECT_EQ(output.action_on, 0);
    EXPECT_EQ(output.hit_count, 0);
    EXPECT_NEAR(output.action_gauge, before.gauge + 2 * refill, 1e-5f);
}

TEST_F(IxionToanLifecycle, CharaReloadExitKeepsRigAndClosesCombat) {
    auto *prim = Open(); ASSERT_NE(prim, nullptr);
    CMenuItemInfo menu;
    CMenuKeyFunc controls;
    CMenuSystemData system{};
    CMenuPosDataManage forms{};
    std::array<MENU_ETCINFO, 96> etc{};
    forms.etc_tbl = etc.data(); forms.etc_tbl_num = etc.size();
    auto *old_controls = MenuCommonInfo; auto *old_system = MenuSystemDataPtr;
    auto *old_forms = MenuPosData;
    const auto old_arg = MenuArg; const auto old_param = MenuUserParam;
    MenuCommonInfo = &controls; MenuSystemDataPtr = &system; MenuPosData = &forms;
    MenuArg.active_chara_no = 0;
    MenuUserParam.chara[0] = GetUserDataMan()->GetCharaDataPtr(0);
    menu.sound_loaded = 0; menu.chara_reload = 1;
    menu.script = nullptr; menu.script_size = 0;
    for (int i=0; i<2; ++i) { menu.equip_flag[i]=false; menu.equip_list[i]=0; }
    const int generation = IxionToanBodyGeneration(&parts[0]);
    menu.ExitEnd();
    EXPECT_FALSE(prim->active);
    EXPECT_EQ(parts[0].key_list[0], keys.data());
    EXPECT_EQ(IxionToanBodyDagger(&parts[0]), &parts[1]);
    EXPECT_GT(IxionToanBodyGeneration(&parts[0]), generation);
    MenuCommonInfo = old_controls; MenuSystemDataPtr = old_system; MenuPosData = old_forms;
    MenuArg = old_arg; MenuUserParam = old_param;
}

TEST_F(IxionToanLifecycle, DeleteAndInitializeReuseCannotMoveDeleteOrOwnNativePrim) {
    for (int reset = 0; reset < 3; ++reset) {
        auto *prim = Open(); ASSERT_NE(prim, nullptr);
        ASSERT_TRUE(IxionToanCombatOwnsPrim(prim));
        const auto incarnation = IxionToanPrimIncarnation(prim);
        auto *row = prim->param;
        if (reset == 0) ColPrimMan.Delete(0);
        else if (reset == 1) ColPrimMan.Initialize(&scene);
        else prim->active = false; // expiry then GetPrim handout is another allocation
        auto *reused = ColPrimMan.GetPrim(); ASSERT_EQ(reused, prim);
        ASSERT_TRUE(reused->SetDamage(row->name, 0));
        EXPECT_NE(IxionToanPrimIncarnation(reused), incarnation);
        float point[] = {900, 800, 700, 1}; reused->SetCoord(point, point, 77);
        EXPECT_FALSE(IxionToanCombatOwnsPrim(reused));
        Tick(826);
        EXPECT_FLOAT_EQ(reused->radius, 77);
        IxionToanCombatRetire(&parts[0], "test-retire");
        EXPECT_TRUE(reused->active);
        reused->Delete(-1);
    }
}

TEST_F(IxionToanLifecycle, DirectPrimitiveResetRevokesIncarnationImmediately) {
    auto *prim = Open(); ASSERT_NE(prim, nullptr);
    auto incarnation = IxionToanPrimIncarnation(prim);
    prim->Initialize();
    EXPECT_NE(IxionToanPrimIncarnation(prim), incarnation);
    prim = Open(); ASSERT_NE(prim, nullptr);
    incarnation = IxionToanPrimIncarnation(prim);
    prim->Delete(-1);
    EXPECT_NE(IxionToanPrimIncarnation(prim), incarnation);
}

TEST_F(IxionToanLifecycle, BodyAndSceneRetirementCloseImmediately) {
    auto *prim = Open(); ASSERT_NE(prim, nullptr);
    parts[0].Initialize(nullptr);
    EXPECT_FALSE(prim->active);
    EXPECT_EQ(IxionToanBodyGeneration(&parts[0]), 0);
    parts[0].key_list[0] = keys.data(); parts[0].key_num[0] = keys.size();
    IxionToanBodyRegister(&parts[0], &parts[1], &scene);
    prim = Open(); ASSERT_NE(prim, nullptr);
    scene.Initialize();
    EXPECT_FALSE(prim->active);
    EXPECT_EQ(IxionToanBodyGeneration(&parts[0]), 0);
}

TEST_F(IxionToanLifecycle, ValidSkinAndEventEquipmentCallsProtectBodyAndAttachedParts) {
    parts[0].next = &parts[2]; parts[2].parent = &parts[0];
    auto skin = Pack("IMG 0, \"pixel.bin\"; IMG_END;");
    auto equip = Pack("SCALE 2.0, 3.0, 4.0;");
    std::array<u_long128, 4096> arena{}; mgCMemory stack;
    stack.stSetBuffer(arena.data(), arena.size());
    // Prove the same valid skin script reaches the real native parser for an unowned character.
    CCharacter2 native; load_img_size = -1;
    native.LoadSkin(skin.data(), const_cast<char *>("info.cfg"), const_cast<char *>(""), &stack, 0);
    ASSERT_EQ(load_img_size, 7);
    for (auto *target : {&parts[0], &parts[1], &parts[2]}) {
        load_img_size = -1;
        target->LoadSkin(skin.data(), const_cast<char *>("info.cfg"), const_cast<char *>(""), &stack, 0);
        ASSERT_EQ(load_img_size, -1);
    }
    auto *previous_event = EventScene; EventScene = &scene;
    CRunScript script;
    SetEventFunc(&script);
    EXPECT_EQ(script.ext_func_table[EVENT_EXT_LOAD_EQUIP], &_LOAD_EQUIP);
    RS_STACKDATA request[6]{};
    for (auto &arg : request) arg.type = RS_INT;
    request[0].val.i = 0; request[1].val.i = 0; request[2].val.i = 0;
    request[3].type = RS_STR; request[3].val.s = const_cast<char *>("info.cfg");
    request[4].val.i = 0; request[5].val.i = 8; // native attachment into a previously unowned slot
    EXPECT_TRUE(_LOAD_EQUIP(request, 6));
    EXPECT_EQ(scene.GetCharacter(8), nullptr);
    char *name = const_cast<char *>("info.cfg");
    for (int slot : {0, 1, 2}) {
        EXPECT_EQ(_LOAD_CHARA_sub(0, &name, slot, equip.data(), 0), slot);
        EXPECT_EQ(scene.GetCharacter(slot), &parts[slot]);
        EXPECT_EQ(scene.LoadChara(slot, equip.data(), name, &stack, &stack, &stack, 0, 0), slot);
        EXPECT_EQ(scene.GetCharacter(slot), &parts[slot]);
    }
    EventScene = previous_event;
    // Unowned destination still takes the native path and parses SCALE.
    ASSERT_EQ(scene.LoadChara(8, equip.data(), name, &stack, &stack, &stack, 0, 0), 8);
    EXPECT_FLOAT_EQ(scene.GetCharacter(8)->base_scale[0], 2);
}

TEST_F(IxionToanLifecycle, RealMenuLoaderPreservesWeaponAndCostumeArenasAndRefreshesBinding) {
    // Each pass submits a completed BG read through the native equipment loader, including preview load.
    std::array<CActionChara, 6> preview;
    CActionChara *preview_ptrs[6]; for (int i=0; i<6; ++i) preview_ptrs[i]=&preview[i];
    std::array<u_long128, 512> scene_memory{}, preview_memory{}, read_memory{};
    std::array<mgCMemory, 7> field_stacks;
    auto *old_stacks = MorattaStack;
    MorattaStack = field_stacks.data();
    auto model = Pack("SCALE 2.0, 3.0, 4.0;");
    auto skin = Pack("IMG 0, \"pixel.bin\"; IMG_END;");
    auto dir = std::filesystem::temp_directory_path() /
        ("ixion-toan-menu-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(dir));
    const auto old_user = MenuUserDataManPtr;
    const auto old_param = MenuUserParam;
    MenuUserDataManPtr = GetUserDataMan();
    MenuUserParam.chara[0] = MenuUserDataManPtr->GetCharaDataPtr(0);
    DataReadIndex(dir);
    for (int part : {2, 1}) {
        SCOPED_TRACE(part);
        auto &pack = part == 2 ? model : skin;
        auto file = dir / "part.chr";
        { std::ofstream out(file, std::ios::binary); out.write(reinterpret_cast<char *>(pack.data()), pack.size()*4); }
        DataReadIndex(dir); StartReadBG();
        int size=0; char path[]="part.chr";
        ASSERT_EQ(LoadFileBG(path, read_memory.data(), &size), 1);
        ReadBGSync(); ReadBGSync();
        MENU_BGREAD_INFO2 records[7]{}; MENU_BGREAD_INFO2 *info[7];
        for (int i=0; i<7; ++i) info[i]=&records[i];
        std::strcpy(records[part].path, path); records[part].reading=1;
        records[0].chara = &preview[0];
        MenuActionCharaBuffer[part].stSetBuffer(preview_memory.data(), preview_memory.size());
        MorattaStack[part].stSetBuffer(scene_memory.data(), scene_memory.size());
        MorattaStack[part].stack_used = 123;
        MenuLoadInfo = {}; MenuLoadInfo.mode=MENU_LOAD_MODE_MENU;
        MenuLoadInfo.update_scene=true; MenuLoadInfo.load_phase=6; MenuLoadInfo.request_phase=-1;
        mgCMemory unused;
        const int generation = IxionToanBodyGeneration(&parts[0]);
        ASSERT_EQ(MenuItemCharaDataLoadEndCheck(info, &unused, preview_ptrs, 0, 0, 0), 1);
        EXPECT_EQ(records[part].reading, 0);
        if (part==2) EXPECT_FLOAT_EQ(preview[1].base_scale[0], 2);
        EXPECT_EQ(MorattaStack[part].stack_used, 123);
        EXPECT_EQ(parts[0].key_list[0], keys.data());
        EXPECT_EQ(IxionToanBodyDagger(&parts[0]), &parts[1]);
        EXPECT_GT(IxionToanBodyGeneration(&parts[0]), generation);
        BreakReadBG();
    }
    MenuUserDataManPtr = old_user;
    MenuUserParam = old_param;
    MorattaStack = old_stacks;
    std::filesystem::remove_all(dir);
}
} // namespace

TEST(IxionToanNative, OffRegistrationKeepsValidNativeSkinAndEquipmentLoading) {
    if (IxionToanBodyEnabled()) GTEST_SKIP() << "native control runs without donor opt-ins";
    CActionChara body, part;
    IxionToanBodyRegister(&body, &part);
    EXPECT_EQ(IxionToanBodyGeneration(&body), 0);
    EXPECT_FALSE(IxionToanBodyOwns(&body));
    EXPECT_FALSE(IxionToanBodyOwns(&part));
    auto skin = Pack("IMG 0, \"pixel.bin\"; IMG_END;");
    std::array<u_long128, 4096> arena{}; mgCMemory stack;
    stack.stSetBuffer(arena.data(), arena.size());
    load_img_size = -1;
    body.LoadSkin(skin.data(), const_cast<char *>("info.cfg"), const_cast<char *>(""), &stack, 0);
    EXPECT_EQ(load_img_size, 7);
    CScene scene;
    scene.AssignChara(0, &body, const_cast<char *>("native"));
    auto model = Pack("SCALE 2.0, 3.0, 4.0;");
    EXPECT_EQ(scene.LoadChara(0, model.data(), const_cast<char *>("info.cfg"), &stack, &stack, &stack, 0, 0), 0);
    EXPECT_NE(scene.GetCharacter(0), &body);
    EXPECT_FLOAT_EQ(scene.GetCharacter(0)->base_scale[0], 2);
}
