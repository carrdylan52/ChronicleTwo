#include <gtest/gtest.h>
#include "ixion_delivery_lock.hpp"
#include "ixion_delivery.hpp"
#include "ixion_ledger.hpp"
#include "ixion_room.hpp"
#include "ixion_dc1_monster.hpp"
#include "monster.hpp"
#include "scene.hpp"
#include "dng_main.hpp"
#include "platform_fixture.hpp"
#include "platform/files.hpp"
#include <cstdlib>
#include <random>
#include <memory>

namespace {
using namespace ixion::delivery;
TEST(IxionLedgerEnv, SingleDonorVariableRefusesAListWithDiagnostic) {
    const char *key = "DC2_IXION_ROOM_DONOR_MONSTER";
    const char *previous = std::getenv(key);
    const bool had = previous != nullptr;
    const std::string saved = had ? previous : "";
    dc::test::SetEnv(key, "e104a,e42a", 1);
    testing::internal::CaptureStderr();
    EXPECT_FALSE(IxionDc1DonorRequested());
    EXPECT_EQ(IxionDc1RosterSize(), 0);
    const std::string log = testing::internal::GetCapturedStderr();
    EXPECT_NE(log.find("DC2_IXION_ROOM_DONOR_MONSTER refused list"), std::string::npos) << log;
    if (had) dc::test::SetEnv(key, saved.c_str(), 1);
    else dc::test::UnsetEnv(key);
}
// Real ledger and cursor files; each case kills, consumes, then re-kills the
// same world enemy the way a relaunch, a save reload or a respawn presents it.
class IxionLedgerWorldKill : public testing::Test {
protected:
    Path root, ledger, cursor;
    bool owned = false;
    const std::string identity = "0123456789abcdef0123456789abcdef";
    int grants = 0;
    std::string error;
    void SetUp() override {
        const char *base = std::getenv("IXION_LEDGER_TEST_ROOT");
        const Path parent = base ? Path(base) : std::filesystem::temp_directory_path();
        std::random_device random;
        root = parent / ("ledger-kill-test-" + std::to_string(random()) + "-" + std::to_string(random()));
        owned = std::filesystem::create_directory(root);
        ASSERT_TRUE(owned);
        ledger = root / "ledger.jsonl";
        cursor = root / "dc1-saves" / "ixion-delivery.cursor.json";
        std::filesystem::create_directories(cursor.parent_path());
        ASSERT_TRUE(Init(ledger, cursor, identity));
    }
    void TearDown() override { if (owned) std::filesystem::remove_all(root); }
    static bool Write(const Path &path, const std::string &text) {
        return FilesWrite(path, text.data(), text.size(), true) == FilesResult::kWritten;
    }
    // What tools/ledger.py writes: cursor, sealed ledger and run marker.
    static bool Init(const Path &ledger_path, const Path &cursor_path, const std::string &id) {
        return Write(cursor_path, CursorText({id, 0})) &&
               Write(ConsumedPath(ledger_path), ConsumedText({id, 0})) &&
               Write(ledger_path, NewLedgerText(id, cursor_path.string())) &&
               Write(ledger_path.parent_path() / kDisposableMarker,
                     Json{{"version", 2u}, {"ledger_format", 2u}, {"ledger", id}}.dump() + "\n");
    }
    static IxionLedgerKill Kill(const std::string &session, uint64_t generation, int monster_id) {
        IxionLedgerKill kill;
        kill.room = "d01inter";
        kill.dungeon = 0;
        kill.floor = 1;
        kill.slot = 0;
        kill.session = session;
        kill.generation = generation;
        kill.monster_id = monster_id;
        return kill;
    }
    bool Publish(const IxionLedgerKill &kill, bool &added) {
        uint64_t seq = 0;
        bool retryable = false;
        std::string event;
        return IxionLedgerPublishKill(ledger, kill, root / "dc2-saves", 1791990000u, seq, added, event, retryable, error);
    }
    void ConsumeAll() {
        uint64_t applied = 0;
        ASSERT_TRUE(Consume(ledger, cursor, Write, [&](const Record &) { ++grants; }, applied, error)) << error;
    }
    size_t Records() {
        std::string text;
        Ledger parsed;
        EXPECT_TRUE(Read(ledger, text, error));
        EXPECT_TRUE(ParseLedger(text, parsed, error)) << error;
        return parsed.records.size();
    }
    void KillConsumeAndReKill(const IxionLedgerKill &first, const IxionLedgerKill &again) {
        bool added = false;
        ASSERT_TRUE(Publish(first, added)) << error;
        EXPECT_TRUE(added);
        ConsumeAll();
        ASSERT_TRUE(Publish(again, added)) << error;
        EXPECT_FALSE(added) << "the same world kill was appended twice";
        ConsumeAll();
        EXPECT_EQ(Records(), 1u);
        EXPECT_EQ(grants, 1);
    }
};
const std::string kSessionA = "aaaaaaaabbbbbbbbccccccccdddddddd";
const std::string kSessionB = "11111111222222223333333344444444";

TEST_F(IxionLedgerWorldKill, SameLedgerRelaunchReKillYieldsOneRecordAndOneGrant) {
    // A new process has a new nonce and starts at room generation 1 again.
    KillConsumeAndReKill(Kill(kSessionA, 1, 7), Kill(kSessionB, 1, 7));
}

TEST_F(IxionLedgerWorldKill, ReloadPreKillSaveReKillYieldsOneRecordAndOneGrant) {
    // Loading the pre-kill save in the same process reinstalls the room:
    // a new generation over the same world enemy.
    KillConsumeAndReKill(Kill(kSessionA, 1, 7), Kill(kSessionA, 2, 7));
}

TEST_F(IxionLedgerWorldKill, RespawnReKillYieldsOneRecordAndOneGrant) {
    // Leaving and re-entering respawns the encounter; the native pool may
    // seat a different occupant in the room's single encounter slot.
    KillConsumeAndReKill(Kill(kSessionA, 1, 7), Kill(kSessionA, 3, 9));
}

TEST_F(IxionLedgerWorldKill, ResealedConsumedEventRewriteRefusesProductionReKillAndConsume) {
    bool added = false;
    ASSERT_TRUE(Publish(Kill(kSessionA, 1, 7), added)) << error;
    ASSERT_TRUE(added);
    ConsumeAll();
    ASSERT_EQ(grants, 1);
    const std::vector<Path> retained = {cursor, ConsumedPath(ledger), root / kDisposableMarker};
    std::vector<std::string> before;
    for (const auto &path : retained) {
        std::string text;
        ASSERT_TRUE(Read(path, text, error));
        before.push_back(text);
    }
    // B6-N1: edit ONLY the consumed record's event, keeping every other field
    // and the genuine marker/cursor/high-water. Publicly reseal the ledger.
    std::string text;
    ASSERT_TRUE(Read(ledger, text, error));
    std::string body = text.substr(0, text.rfind('\n', text.size() - 2) + 1);
    const auto at = body.find(":e0:native");
    ASSERT_NE(at, std::string::npos);
    body.replace(at, 10, ":e1:native");
    const std::string edited = body + Trailer(body, 1);
    ASSERT_TRUE(Write(ledger, edited));
    Ledger parsed;
    ASSERT_TRUE(ParseLedger(edited, parsed, error)) << error;
    ASSERT_EQ(parsed.records.size(), 1u);
    EXPECT_EQ(parsed.records[0].source_event, "kill:" + identity + ":d01inter:0:1:e1:native");

    for (int restart = 0; restart < 2; ++restart) {
        IxionLedgerRetry retry; // Real production publisher and refusal logger.
        retry.Arm(Kill(kSessionB, 2, 9)); // Same stable e0 after reload/relaunch.
        testing::internal::CaptureStderr();
        const auto step = retry.Step(ledger, root / "dc2-saves", 1791990001u);
        const auto log = testing::internal::GetCapturedStderr();
        EXPECT_EQ(step, IxionLedgerStep::kRefused);
        EXPECT_FALSE(retry.added);
        EXPECT_FALSE(retry.pending);
        EXPECT_NE(log.find("ixion ledger delivery REFUSED: consumed history mismatch"), std::string::npos) << log;

        uint64_t applied = 99;
        int writes = 0;
        auto write = [&](const Path &p, const std::string &value) { ++writes; return Write(p, value); };
        std::string last_error;
        testing::internal::CaptureStderr();
        const bool ok = Consume(ledger, cursor, write, [&](const Record &) { ++grants; }, applied, error);
        if (!ok) LogConsumeRefusal(error, last_error);
        const auto consume_log = testing::internal::GetCapturedStderr();
        EXPECT_FALSE(ok);
        EXPECT_NE(consume_log.find("ixion ledger refused: consumed history mismatch"), std::string::npos) << consume_log;
        EXPECT_EQ(writes, 0);
        EXPECT_EQ(applied, 0u);
        EXPECT_EQ(grants, 1);
        EXPECT_EQ(Records(), 1u);
        ASSERT_TRUE(Read(ledger, text, error));
        EXPECT_EQ(text, edited);
        for (size_t i = 0; i < retained.size(); ++i) {
            ASSERT_TRUE(Read(retained[i], text, error));
            EXPECT_EQ(text, before[i]) << retained[i];
        }
    }
}

TEST_F(IxionLedgerWorldKill, DifferentRoomFloorSlotOrDonorIsADifferentKill) {
    const IxionLedgerKill base = Kill(kSessionA, 1, 7);
    IxionLedgerKill room = base, floor = base, slot = base, donor = base;
    room.room = "d01inter2";
    floor.floor = 2;
    slot.slot = 1;
    donor.donor = "e104a";
    for (const auto &other : {room, floor, slot, donor})
        EXPECT_NE(IxionLedgerKillKey(base), IxionLedgerKillKey(other));
}

TEST_F(IxionLedgerWorldKill, AFreshLedgerGrantsTheSameKillAgain) {
    bool added = false;
    ASSERT_TRUE(Publish(Kill(kSessionA, 1, 7), added)) << error;
    EXPECT_TRUE(added);
    const Path second = root / "second" / "ledger.jsonl";
    const Path second_cursor = root / "second" / "dc1-saves" / "ixion-delivery.cursor.json";
    std::filesystem::create_directories(second_cursor.parent_path());
    ASSERT_TRUE(Init(second, second_cursor, "fedcba9876543210fedcba9876543210"));
    uint64_t seq = 0;
    bool retryable = false;
    std::string event;
    ASSERT_TRUE(IxionLedgerPublishKill(second, Kill(kSessionB, 1, 7), root / "second" / "dc2-saves", 1791990000u, seq,
                                       added, event, retryable, error)) << error;
    EXPECT_TRUE(added);
    EXPECT_EQ(seq, 1u);
    EXPECT_EQ(event, "kill:fedcba9876543210fedcba9876543210:d01inter:0:1:e0:native");
}

TEST_F(IxionLedgerWorldKill, BusyLockRetriesOnALaterFrameAndPublishesOnce) {
    IxionLedgerRetry retry;
    retry.Arm(Kill(kSessionA, 1, 7));
    {
        ixion::delivery::Lock busy(ledger); // e.g. DC1 consuming at that moment
        ASSERT_TRUE(busy.ok());
        EXPECT_EQ(retry.Step(ledger, root / "dc2-saves", 1791990000u), IxionLedgerStep::kRetrying);
        EXPECT_EQ(retry.Step(ledger, root / "dc2-saves", 1791990000u), IxionLedgerStep::kRetrying);
        EXPECT_TRUE(retry.pending);
        EXPECT_EQ(Records(), 0u);
    }
    EXPECT_EQ(retry.Step(ledger, root / "dc2-saves", 1791990000u), IxionLedgerStep::kPublished);
    EXPECT_TRUE(retry.added);
    EXPECT_EQ(retry.attempts, 3);
    EXPECT_EQ(retry.Step(ledger, root / "dc2-saves", 1791990000u), IxionLedgerStep::kIdle);
    ConsumeAll();
    EXPECT_EQ(Records(), 1u);
    EXPECT_EQ(grants, 1);
}

TEST_F(IxionLedgerWorldKill, RetryIsBoundedAndReportsTheLoss) {
    IxionLedgerRetry retry;
    retry.Arm(Kill(kSessionA, 1, 7));
    ixion::delivery::Lock busy(ledger);
    ASSERT_TRUE(busy.ok());
    IxionLedgerStep step = IxionLedgerStep::kRetrying;
    int frames = 0;
    while (step == IxionLedgerStep::kRetrying) {
        step = retry.Step(ledger, root / "dc2-saves", 1791990000u);
        ++frames;
    }
    EXPECT_EQ(step, IxionLedgerStep::kLost);
    EXPECT_EQ(frames, IxionLedgerRetry::kMaxAttempts);
    EXPECT_FALSE(retry.pending);
    EXPECT_EQ(retry.error, "ledger busy or inaccessible");
}

TEST_F(IxionLedgerWorldKill, UnmarkedRunIsRefusedWithoutRetry) {
    std::filesystem::remove(root / kDisposableMarker);
    IxionLedgerRetry retry;
    retry.Arm(Kill(kSessionA, 1, 7));
    EXPECT_EQ(retry.Step(ledger, root / "dc2-saves", 1791990000u), IxionLedgerStep::kRefused);
    EXPECT_FALSE(retry.pending);
    EXPECT_EQ(retry.attempts, 1);
    EXPECT_NE(retry.error.find("marker"), std::string::npos) << retry.error;
}

// Use the production installed-identity capture and confirmation queue with
// native actor storage, then publish/consume real disposable ledger files.
struct InstalledEncounter {
    std::unique_ptr<CScene, decltype(&std::free)> scene{static_cast<CScene *>(std::calloc(1, sizeof(CScene))), &std::free};
    std::unique_ptr<CMonsterMan> manager = std::make_unique<CMonsterMan>();
    std::unique_ptr<CActiveMonster[]> actors = std::make_unique<CActiveMonster[]>(2);
    CScene *saved_scene = DngMainScene;
    CMonsterMan *saved_manager = ActiveMonster;
    InstalledEncounter() {
        manager->scene = scene.get();
        for (auto &slot : manager->active) slot = nullptr;
        for (int i = 0; i < 2; ++i) {
            manager->active[i] = &actors[i];
            actors[i].monster_id = 8 + i;
            actors[i].state = ACTIVE_MONSTER_DEAD;
            actors[i].chara_kind = ACTION_KIND_SCRIPT;
            actors[i].life = 0;
            for (auto &damage : actors[i].damage) damage = ACTION_DAMAGE{};
        }
        DngMainScene = scene.get();
        ActiveMonster = manager.get();
    }
    void Install(const char *first = "e104a", const char *second = "e42a") {
        IxionRoomTestInstall state;
        state.scene = scene.get();
        state.enemy = &actors[0];
        state.ledger_room = "d01inter";
        state.ledger_dungeon = 0;
        state.ledger_floor = 1;
        if (first) state.enemy_key = IxionRoomDonorKillKey("d01inter", 0, 1, 0, first);
        if (second) {
            state.roster = {&actors[1]};
            state.roster_keys = {IxionRoomDonorKillKey("d01inter", 0, 1, 1, second)};
        }
        IxionRoomTestAdoptInstalled(state);
    }
    ~InstalledEncounter() {
        IxionRoomRetire("ledger-test-teardown");
        DngMainScene = saved_scene;
        ActiveMonster = saved_manager;
    }
};

TEST_F(IxionLedgerWorldKill, TwoRosterDeathsInOneFrameRetainTwoImmutableEvents) {
    InstalledEncounter world;
    world.Install();
    IxionLedgerQueue queue;
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    ASSERT_TRUE(queue.Confirm(&world.actors[1]));
    EXPECT_EQ(queue.PendingCount(), 2u);
    world.actors[0].state = world.actors[1].state = ACTIVE_MONSTER_NONE;
    IxionRoomRetire("two-deaths-then-depart");
    // The source actors and roster no longer exist at the publication boundary.
    world.actors[0].monster_id = 99;
    world.Install("e07a", "e60a");
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kPublished);
    EXPECT_EQ(queue.retry.event, "kill:" + identity + ":d01inter:0:1:e0:dc1-e104a");
    EXPECT_EQ(queue.retry.kill.monster_id, 8);
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kPublished);
    EXPECT_EQ(queue.retry.event, "kill:" + identity + ":d01inter:0:1:e1:dc1-e42a");
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kIdle);
    ConsumeAll();
    EXPECT_EQ(Records(), 2u);
    EXPECT_EQ(grants, 2);
}

TEST_F(IxionLedgerWorldKill, ConfirmedKillSurvivesDepartureAtTheSameBoundary) {
    InstalledEncounter world;
    world.Install();
    IxionLedgerQueue queue;
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    IxionRoomRetire("count-then-exit");
    EXPECT_FALSE(IxionRoomInstalled());
    {
        ixion::delivery::Lock busy(ledger);
        EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kRetrying);
    }
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 2), IxionLedgerStep::kPublished);
    EXPECT_EQ(queue.retry.attempts, 2);
    ConsumeAll();
    EXPECT_EQ(Records(), 1u);
    EXPECT_EQ(grants, 1);
}

TEST_F(IxionLedgerWorldKill, RetireDespawnForeignAndReusedIncarnationNeverProduce) {
    InstalledEncounter world;
    world.Install("e104a", nullptr);
    IxionLedgerQueue queue;
    EXPECT_FALSE(queue.Confirm(&world.actors[1])); // not a roster member
    world.actors[0].state = ACTIVE_MONSTER_NONE; // uncounted despawn while still installed
    EXPECT_FALSE(queue.Confirm(&world.actors[0]));
    world.actors[0].state = ACTIVE_MONSTER_LIVE;
    EXPECT_FALSE(queue.Confirm(&world.actors[0]));
    world.actors[0].state = ACTIVE_MONSTER_DEAD;
    // Same address, replaced native incarnation: the saved identity is stale.
    IxionRoomBeforeMonsterSpawn(&world.actors[0]);
    EXPECT_FALSE(queue.Confirm(&world.actors[0]));
    IxionRoomRetire("despawn-is-not-a-kill");
    EXPECT_FALSE(queue.Confirm(&world.actors[0]));
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kIdle);
    EXPECT_EQ(Records(), 0u);
}

TEST_F(IxionLedgerWorldKill, InstalledNativeIgnoresFailedDonorRequestWhenReKilled) {
    InstalledEncounter world;
    world.Install(nullptr, nullptr); // native actually installed, as in the old failed-donor fallback
    const char *old = std::getenv("DC2_IXION_ROOM_DONOR_MONSTER");
    const bool had = old != nullptr;
    const std::string saved = old ? old : "";
    dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", "e104a", 1);
    IxionLedgerQueue queue;
    EXPECT_TRUE(queue.Confirm(&world.actors[0]));
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kPublished);
    EXPECT_EQ(queue.retry.event, "kill:" + identity + ":d01inter:0:1:e0:native");
    dc::test::UnsetEnv("DC2_IXION_ROOM_DONOR_MONSTER");
    EXPECT_TRUE(queue.Confirm(&world.actors[0]));
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 2), IxionLedgerStep::kPublished);
    EXPECT_FALSE(queue.retry.added);
    if (had) dc::test::SetEnv("DC2_IXION_ROOM_DONOR_MONSTER", saved.c_str(), 1);
    ConsumeAll();
    EXPECT_EQ(Records(), 1u);
    EXPECT_EQ(grants, 1);
}

TEST_F(IxionLedgerWorldKill, NewFormatContinuationAndChangingUnrelatedRosterMemberNeverRegrants) {
    // The new ledger format keeps the exact R2 event encoding, not its header.
    uint64_t seq = 0;
    bool added = false;
    ASSERT_TRUE(Append(ledger, "kill:" + identity + ":d01inter:0:1:e0:dc1-e104a", 1, Write, seq, added, error));
    InstalledEncounter world;
    world.Install("e104a", "e42a");
    IxionLedgerQueue queue;
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 2), IxionLedgerStep::kPublished);
    EXPECT_FALSE(queue.retry.added);
    IxionRoomRetire("change-unrelated-roster-member");
    world.Install("e104a", "e07a");
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 3), IxionLedgerStep::kPublished);
    EXPECT_FALSE(queue.retry.added);
    ConsumeAll();
    EXPECT_EQ(Records(), 1u);
    EXPECT_EQ(grants, 1);
}

TEST_F(IxionLedgerWorldKill, LegacyFallbackWithConsumedCursorRefusesNativeReKill) {
    // Literal R2 history: a native fallback was mislabeled as the requested
    // donor and already granted once. Do not manufacture it via today's writer.
    const std::string header = Json{{"version", 1u}, {"ledger", identity}, {"cursor", cursor.string()}}.dump() + "\n";
    const std::string record = "{\"seq\":1,\"source_game\":\"dc2\",\"source_event\":\"kill:" + identity +
        ":d01inter:0:1:e0:dc1-e104a\",\"item\":145,\"created\":1791990000}\n";
    const std::string legacy = header + record + Trailer(header + record, 1);
    const std::string consumed = CursorText({identity, 1});
    ASSERT_TRUE(Write(ledger, legacy));
    ASSERT_TRUE(Write(cursor, consumed));
    ASSERT_TRUE(Write(root / kDisposableMarker, Json{{"version", 1u}, {"ledger", identity}}.dump() + "\n"));
    grants = 1; // Historical delivery represented by the consumed cursor.

    InstalledEncounter world;
    world.Install(nullptr, nullptr); // The actual native encounter under current code.
    IxionLedgerKill captured;
    ASSERT_TRUE(IxionRoomCaptureLedgerKill(&world.actors[0], captured));
    EXPECT_EQ(IxionLedgerKillKey(captured), "d01inter:0:1:e0:native");
    IxionLedgerQueue queue;
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    testing::internal::CaptureStderr();
    const auto step = queue.Step(ledger, root / "dc2-saves", 1791990001u);
    const auto log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(step, IxionLedgerStep::kRefused);
    EXPECT_EQ(queue.retry.attempts, 1);
    EXPECT_FALSE(queue.retry.added);
    EXPECT_EQ(queue.PendingCount(), 0u);
    EXPECT_NE(log.find("ixion ledger delivery REFUSED: legacy ledger version 1"), std::string::npos) << log;

    uint64_t applied = 99;
    EXPECT_FALSE(Consume(ledger, cursor, Write, [&](const Record &) { ++grants; }, applied, error));
    EXPECT_NE(error.find("legacy ledger version 1"), std::string::npos) << error;
    EXPECT_EQ(applied, 0u);
    EXPECT_EQ(grants, 1);
    std::string after;
    ASSERT_TRUE(Read(ledger, after, error));
    EXPECT_EQ(after, legacy); // Still exactly the one sealed record, byte-for-byte.
    ASSERT_TRUE(Read(cursor, after, error));
    EXPECT_EQ(after, consumed);
}

TEST_F(IxionLedgerWorldKill, ReheaderedResealedR2FallbackWithOriginalMarkerAndCursorRefusesNativeReKill) {
    // Authentic R2 fallback history, including its original authorization and
    // consumed cursor. Only the ledger header and its unkeyed seal are edited.
    const std::string header = Json{{"version", 1u}, {"ledger", identity}, {"cursor", cursor.string()}}.dump() + "\n";
    const std::string record = "{\"seq\":1,\"source_game\":\"dc2\",\"source_event\":\"kill:" + identity +
        ":d01inter:0:1:e0:dc1-e104a\",\"item\":145,\"created\":1791990000}\n";
    const std::string consumed = CursorText({identity, 1});
    const std::string marker = Json{{"version", 1u}, {"ledger", identity}}.dump() + "\n";
    ASSERT_TRUE(Write(ledger, header + record + Trailer(header + record, 1)));
    ASSERT_TRUE(Write(cursor, consumed));
    ASSERT_TRUE(Write(root / kDisposableMarker, marker));
    grants = 1;

    std::string edited = header + record;
    edited.replace(edited.find("\"version\":1"), 11, "\"version\":2");
    const std::string resealed = edited + Trailer(edited, 1);
    ASSERT_TRUE(Write(ledger, resealed));
    Ledger parsed;
    ASSERT_TRUE(ParseLedger(resealed, parsed, error)) << error;
    ASSERT_EQ(parsed.records.size(), 1u); // Header/checksum alone still parse.

    InstalledEncounter world;
    world.Install(nullptr, nullptr);
    IxionLedgerKill captured;
    ASSERT_TRUE(IxionRoomCaptureLedgerKill(&world.actors[0], captured));
    EXPECT_EQ(IxionLedgerKillKey(captured), "d01inter:0:1:e0:native");
    IxionLedgerQueue queue;
    ASSERT_TRUE(queue.Confirm(&world.actors[0]));
    testing::internal::CaptureStderr();
    const auto step = queue.Step(ledger, root / "dc2-saves", 1791990001u);
    const auto log = testing::internal::GetCapturedStderr();
    EXPECT_EQ(step, IxionLedgerStep::kRefused);
    EXPECT_EQ(queue.retry.attempts, 1);
    EXPECT_FALSE(queue.retry.added);
    EXPECT_EQ(queue.PendingCount(), 0u);
    EXPECT_NE(log.find("ixion ledger delivery REFUSED: legacy disposable-run marker version 1"), std::string::npos) << log;

    uint64_t applied = 99;
    EXPECT_FALSE(Consume(ledger, cursor, Write, [&](const Record &) { ++grants; }, applied, error));
    EXPECT_NE(error.find("ledger v2 binding"), std::string::npos) << error;
    EXPECT_EQ(applied, 0u);
    EXPECT_EQ(grants, 1);
    std::string after;
    ASSERT_TRUE(Read(ledger, after, error));
    EXPECT_EQ(after, resealed);
    ASSERT_TRUE(ParseLedger(after, parsed, error)) << error;
    EXPECT_EQ(parsed.records.size(), 1u);
    ASSERT_TRUE(Read(cursor, after, error));
    EXPECT_EQ(after, consumed);
    ASSERT_TRUE(Read(root / kDisposableMarker, after, error));
    EXPECT_EQ(after, marker);
}

TEST_F(IxionLedgerWorldKill, QueueOverflowLogsLostWithoutReplacingPendingKills) {
    IxionLedgerQueue queue;
    IxionLedgerKill kill = Kill(kSessionA, 1, 7);
    for (size_t i = 0; i < IxionLedgerQueue::kCapacity; ++i) {
        kill.slot = static_cast<int>(i);
        ASSERT_TRUE(queue.Enqueue(kill));
    }
    // A busy head still counts toward capacity; later kills cannot replace it.
    {
        ixion::delivery::Lock busy(ledger);
        EXPECT_EQ(queue.Step(ledger, root / "dc2-saves", 1), IxionLedgerStep::kRetrying);
    }
    testing::internal::CaptureStderr();
    EXPECT_FALSE(queue.Enqueue(kill));
    EXPECT_NE(testing::internal::GetCapturedStderr().find("ixion ledger LOST delivery: kill queue overflow"), std::string::npos);
    kill.room = "mutated";
    for (size_t i = 0; i < IxionLedgerQueue::kCapacity; ++i) {
        ASSERT_EQ(queue.Step(ledger, root / "dc2-saves", 2), IxionLedgerStep::kPublished);
        EXPECT_EQ(queue.retry.kill.room, "d01inter");
        EXPECT_EQ(queue.retry.kill.slot, static_cast<int>(i));
    }
    EXPECT_EQ(Records(), IxionLedgerQueue::kCapacity);
}
} // namespace
