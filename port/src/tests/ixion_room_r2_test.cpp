#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

#include "colprim.hpp"
#include "dng_main.hpp"
#include "effscript.hpp"
#include "ixion_room.hpp"
#include "ixion_room_atmos.hpp"
#include "ixion_room_logging.hpp"
#include "map.hpp"
#include "mg_memory.hpp"
#include "mg_drawenv.hpp"
#include "platform/arena.hpp"
#include "runscript.hpp"

void SetEffectScriptFunc();
EFF_SPT_BASE_DEF *GetEffSptBaseDefPtr(int index);

namespace {
struct NativeEffectProgram {
    RS_PROG_HEADER header{};
    RS_PROGDATA entry{200, offsetof(NativeEffectProgram, function)};
    funcdata function{};
    vmcode_t code[8]{};
    char child_name[64]{};
    char damage_name[64]{};

    NativeEffectProgram() {
        std::memcpy(header.magic, "SB2", 3);
        header.main = offsetof(NativeEffectProgram, function);
        header.code = offsetof(NativeEffectProgram, code);
        header.prog = offsetof(NativeEffectProgram, entry);
        header.prog_num = 1;
        std::strcpy(child_name, GetEffSptBaseDefPtr(8)->name);
        std::strcpy(damage_name, Damage_Param_Table[0].name);
        code[0] = {RS_OP_PUSH_CONST, RS_CONST_INT, {EFF_EXT_ES_CREATE}};
        code[1] = {RS_OP_PUSH_CONST, RS_CONST_STR,
                   {offsetof(NativeEffectProgram, child_name) - offsetof(NativeEffectProgram, code)}};
        code[2] = {RS_OP_EXT, 2, {0}};
        code[3] = {RS_OP_PUSH_CONST, RS_CONST_INT, {EFF_EXT_COLPRIM_CREATE}};
        code[4] = {RS_OP_PUSH_CONST, RS_CONST_STR,
                   {offsetof(NativeEffectProgram, damage_name) - offsetof(NativeEffectProgram, code)}};
        code[5] = {RS_OP_EXT, 2, {0}};
        code[6] = {RS_OP_WAIT, 0, {0}};
        code[7] = {RS_OP_END, 0, {0}};
    }
};

struct EffectWorld {
    mgCMemory work;
    NativeEffectProgram parent_program;
    RS_PROG_HEADER child_program{};
    EFF_SPT_BASE parent_base{}, child_base{};
    CEffectScriptMan effects;
    CEffectScriptMan *saved_fx = FxScriptMan, *saved_es = EffScriptMan;
    std::vector<CColPrim> saved_prims{std::begin(ColPrimMan.prim), std::end(ColPrimMan.prim)};

    EffectWorld() {
        static char arena_key;
        work.SetHeapMem(reinterpret_cast<u_long128 *>(ArenaBlock(&arena_key, 8192 * sizeof(u_long128))), 8192);
        SetEffectScriptFunc();
        std::memcpy(child_program.magic, "SB2", 3);
        parent_base.base_no = 7;
        parent_base.script = reinterpret_cast<char *>(&parent_program);
        child_base.base_no = 8;
        child_base.script = reinterpret_cast<char *>(&child_program);
        effects.SetWorkBuffer(&work);
        effects.base[0] = &parent_base;
        effects.base[1] = &child_base;
        effects.base_num = 2;
        for (auto &prim : ColPrimMan.prim) prim.active = false;
        FxScriptMan = EffScriptMan = &effects;
    }
    ~EffectWorld() {
        FxScriptMan = saved_fx;
        EffScriptMan = saved_es;
        std::copy(saved_prims.begin(), saved_prims.end(), ColPrimMan.prim);
    }
};
} // namespace

TEST(IxionRoomRound2, NativeEffectWorldKeepsCreationAndOpcodesWithoutOwnershipGrowth) {
    ASSERT_FALSE(IxionRoomInstalled());
    const size_t before = IxionRoomEffectOriginCountForTest();
    EffectWorld fx;
    auto *parent = fx.effects.CreateEffSpt(7, 25, 1);
    ASSERT_NE(parent, nullptr);
    testing::internal::CaptureStderr();
    fx.effects.Step();
    const auto log = testing::internal::GetCapturedStderr();
    ASSERT_NE(parent->colprim, nullptr);
    EXPECT_TRUE(parent->colprim->active);
    EXPECT_EQ(parent->colprim->owner, 25);
    auto *child = parent->next;
    ASSERT_NE(child, nullptr);
    ASSERT_NE(child, parent);
    EXPECT_EQ(child->user_id, parent->user_id);
    EXPECT_EQ(log.find("ixion room INVARIANT"), std::string::npos);
    EXPECT_EQ(IxionRoomEffectOriginCountForTest(), before);
    // The helpers are also inert if called directly on the native path.
    IxionRoomNoteEffectCreated(parent, 99);
    IxionRoomSetEffectParent(parent);
    IxionRoomNoteEffectPrim(parent->colprim, parent);
    IxionRoomNotePrimHandedOut(parent->colprim);
    EXPECT_EQ(IxionRoomEffectOriginCountForTest(), before);
    EXPECT_FALSE(IxionRoomPrimHeldByEffect(parent->colprim, parent));
}

TEST(IxionRoomRound2, SkippedLoopReceiptsFollowStateChangesAndReentry) {
    IxionRoomLoopLog log;
    int first_scene, second_scene;
    EXPECT_TRUE(log.Observe(true, 1, &first_scene, 0));
    for (int i = 0; i < 120; ++i) EXPECT_FALSE(log.Observe(true, 1, &first_scene, 0));
    EXPECT_TRUE(log.Observe(true, 2, &first_scene, 0));
    EXPECT_TRUE(log.Observe(true, 2, &second_scene, 0));
    EXPECT_TRUE(log.Observe(true, 2, &second_scene, 1));
    EXPECT_FALSE(log.Observe(false, 2, &second_scene, 0));
    EXPECT_TRUE(log.Observe(true, 2, &second_scene, 1));
    EXPECT_TRUE(log.Observe(true, 2, nullptr, -1));
    EXPECT_FALSE(log.Observe(true, 2, nullptr, -1));
    EXPECT_EQ(IxionRoomLogInterval(1), 10);
    EXPECT_EQ(IxionRoomLogInterval(9), 10);
    EXPECT_EQ(IxionRoomLogInterval(10), 10);
    EXPECT_EQ(IxionRoomLogInterval(30), 30);
    EXPECT_EQ(IxionRoomLogInterval(60), 60);
    EXPECT_EQ(IxionRoomLogInterval(0), 60);
    EXPECT_EQ(IxionRoomLogInterval(61), 60);
}

TEST(IxionRoomRound2, ExitRequestLogsOnceAndBodyTraceHasAMinimumFrameInterval) {
    IxionRoomRequestLog request;
    EXPECT_FALSE(request.Observe(false));
    EXPECT_TRUE(request.Observe(true));
    for (int frame = 0; frame < 120; ++frame) {
        EXPECT_FALSE(request.Observe(true)); // both frame boundaries, indefinitely pending
        EXPECT_FALSE(request.Observe(true));
    }
    EXPECT_FALSE(request.Observe(false));
    EXPECT_TRUE(request.Observe(true)); // a new request is reportable
    IxionRoomTraceLog trace;
    EXPECT_TRUE(trace.Observe(0));
    for (int frame = 0; frame < 30; ++frame) {
        EXPECT_FALSE(trace.Observe(frame)); // key changes cannot bypass this gate
        EXPECT_FALSE(trace.Observe(frame));
    }
    EXPECT_TRUE(trace.Observe(30));
    EXPECT_FALSE(trace.Observe(30));
    EXPECT_TRUE(trace.Observe(0)); // a restarted frame clock
}

TEST(IxionRoomRound2, PolygonQueriesRejectMissingOrExhaustedOutputBeforeMapAccess) {
    CMap map;
    mgVu0FBOX box{};
    CCPoly poly;
    EXPECT_EQ(map.GetColPoly(nullptr, box, 1), 0);
    EXPECT_EQ(map.GetCameraPoly(nullptr, box, 1), 0);
    EXPECT_EQ(map.GetColPoly(&poly, box, 0), 0);
    EXPECT_EQ(map.GetCameraPoly(&poly, box, -1), 0);
}

TEST(IxionRoomRound2, FogTablePreservesDonorArgumentOrderAndNativeCoefficients) {
    struct Case { float near_dist, far_dist, near_value, far_value, offset, scale; };
    // Distances and intensities deliberately differ: swapping either pair must fail.
    const Case cases[] = {{300, 1200, 255, 128, 85.6666667f, 50800},
                          {100, 500, 20, 180, 220, -20000},
                          {100, 500, 128, 128, 128, 0}};
    for (const auto &c : cases) {
        IxionRoomAtmosphere atmosphere;
        atmosphere.fog_near = c.near_dist;
        atmosphere.fog_far = c.far_dist;
        atmosphere.fog_near_value = c.near_value;
        atmosphere.fog_far_value = c.far_value;
        atmosphere.fog_color = {11, 37, 99};
        mgFOG_PARAM fog{};
        IxionRoomAtmosApplyFog(atmosphere, fog);
        EXPECT_FLOAT_EQ(fog.near_dist, c.near_dist);
        EXPECT_FLOAT_EQ(fog.far_dist, c.far_dist);
        EXPECT_FLOAT_EQ(fog.near_value, c.near_value);
        EXPECT_FLOAT_EQ(fog.far_value, c.far_value);
        EXPECT_EQ(fog.r, 11); EXPECT_EQ(fog.g, 37); EXPECT_EQ(fog.b, 99);
        EXPECT_NEAR(fog.offset, c.offset, 0.0001f);
        EXPECT_FLOAT_EQ(fog.scale, c.scale);
        EXPECT_FLOAT_EQ(fog.coef[0], c.offset);
        EXPECT_FLOAT_EQ(fog.coef[1], c.far_value);
        EXPECT_FLOAT_EQ(fog.coef[2], c.near_value);
        EXPECT_FLOAT_EQ(fog.coef[3], c.scale);
    }
}
