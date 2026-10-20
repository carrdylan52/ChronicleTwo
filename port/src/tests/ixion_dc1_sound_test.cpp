#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "audio/mixer.hpp"
#include "audio_bank_fixture.hpp"
#include "ixion_dc1_sound.hpp"
#include "platform_fixture.hpp"
#include "sound.hpp"
#include "sound_port.hpp"

namespace fx = dc::test;

namespace {

constexpr int kDc2EnemyPort = 13; // GetCSndPortNo(SND_PORT_ENEMY)
constexpr int kFrames = 4800;

// Synthetic stand-ins for d01a_g (programs 0..64) and d01a_i (programs 0..36): every key sounds.
std::vector<std::uint8_t> Hd(int programs) {
    fx::BankSpec spec;
    spec.programs = programs;
    return fx::MakeHd(spec);
}
const std::vector<std::uint8_t> &Bd() {
    static const std::vector<std::uint8_t> body = fx::SquareVag(2);
    return body;
}

struct SfxEnv {
    explicit SfxEnv(const char *value) {
        if (value) fx::SetEnv("DC2_IXION_DC1_SFX", value, 1);
        else fx::UnsetEnv("DC2_IXION_DC1_SFX");
        CSnd.Exit(); // Init is a no-op while initialised; start from reset ports
        CSnd.Init(4, 0, 40, 0);
        IxionDc1SfxReleaseNow("test setup");
        reads = 0;
        IxionDc1SfxTestSetReader([this](const std::string &name, std::vector<std::uint8_t> &bytes) {
            ++reads;
            if (name == "d01a_g.hd") bytes = Hd(65);
            else if (name == "d01a_i.hd") bytes = Hd(37);
            else if (name == "d01a_g.bd" || name == "d01a_i.bd") bytes = Bd();
            else return false;
            return true;
        });
        // A DC2 monster bank on its own port, which the DC1 banks must leave alone.
        const std::vector<std::uint8_t> dc2 = Hd(1);
        SoundLoadHdBd(kDc2EnemyPort, dc2.data(), (int) dc2.size(), Bd().data(), (int) Bd().size());
    }
    ~SfxEnv() {
        IxionDc1SfxReleaseNow("test teardown");
        IxionDc1SfxTestSetReader(nullptr);
        fx::UnsetEnv("DC2_IXION_DC1_SFX");
        CSnd.Exit();
    }
    int reads = 0;
};

double RenderRms() {
    return fx::Rms(fx::RenderFrames(audio::DefaultMixer(), kFrames));
}

} // namespace

// Chronicle snd.cpp se_info[2406..2417] = {64, 80..91, 0, 10, 0} (port 10 = slot g) and dun[60] =
// {36, 90, 0, 14, 0} (port 14 = slot i); setbl VOL of the first (program, key) row.
TEST(IxionDc1Sfx, OparIdsMapToBankProgramKey) {
    struct Want { int se; char file; int program; int key; int volume; bool audible; };
    const Want want[] = {
        {2406, 'g', 64, 80, 36, false}, {2407, 'g', 64, 81, 80, true},  {2408, 'g', 64, 82, 80, true},
        {2409, 'g', 64, 83, 88, false}, {2410, 'g', 64, 84, 88, false}, {2411, 'g', 64, 85, 91, true},
        {2412, 'g', 64, 86, 46, true},  {2413, 'g', 64, 87, 66, false}, {2414, 'g', 64, 88, 73, false},
        {2415, 'g', 64, 89, 88, false}, {2416, 'g', 64, 90, 88, false}, {2417, 'g', 64, 91, 76, true},
        {119, 'i', 30, 39, 46, true},   {160, 'i', 36, 90, 76, true},
    };
    for (const Want &w : want) {
        const IxionDc1SeRoute *route = IxionDc1SeLookup(w.se);
        ASSERT_NE(route, nullptr) << w.se;
        EXPECT_EQ(route->file, w.file) << w.se;
        EXPECT_EQ(route->program, w.program) << w.se;
        EXPECT_EQ(route->key, w.key) << w.se;
        EXPECT_EQ(route->volume, w.volume) << w.se;
        EXPECT_EQ(route->audible, w.audible) << w.se;
    }
    EXPECT_EQ(IxionDc1SeLookup(2356), nullptr); // a roster loop id, unmapped in round 1
    EXPECT_EQ(IxionDc1SeLookup(-1), nullptr);
}

TEST(IxionDc1Sfx, WithoutOptInNothingLoadsOrPlays) {
    SfxEnv env(nullptr);
    EXPECT_FALSE(IxionDc1SfxRequested());
    EXPECT_FALSE(IxionDc1SfxAcquire());
    EXPECT_EQ(env.reads, 0);
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 0);
    EXPECT_EQ(IxionDc1SfxPlay(2412, 1.0f, 0.0f, 0, "test", "e104a", 0), -1);
    CSnd.Step();
    EXPECT_EQ(RenderRms(), 0.0);
    EXPECT_EQ(SoundBankCount(kDc2EnemyPort), 1);
    fx::SetEnv("DC2_IXION_DC1_SFX", "0", 1);
    EXPECT_FALSE(IxionDc1SfxRequested());
}

// Opt-in but no donor installed: IxionDc1SfxAcquire runs only at donor install, so nothing is resident.
TEST(IxionDc1Sfx, NoDonorIsANoOp) {
    SfxEnv env("1");
    EXPECT_TRUE(IxionDc1SfxRequested());
    EXPECT_FALSE(IxionDc1SfxResident());
    EXPECT_EQ(IxionDc1SfxPlay(2412, 1.0f, 0.0f, 0, "test", "e104a", 0), -1);
    IxionDc1SfxReleaseLater(0, "test");
    IxionDc1SfxStep(1000);
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 0);
    EXPECT_EQ(SoundBankCount(kDc2EnemyPort), 1);
    EXPECT_EQ(env.reads, 0);
}

TEST(IxionDc1Sfx, LoadPlayAndReleaseLeaveDc2BanksAlone) {
    SfxEnv env("1");
    ASSERT_TRUE(IxionDc1SfxAcquire());
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 2);
    EXPECT_EQ(IxionDc1SfxBank('g'), 0);
    EXPECT_EQ(IxionDc1SfxBank('i'), 1);
    EXPECT_EQ(SoundBankCount(kDc2EnemyPort), 1);
    // A second donor reuses the resident banks.
    const int reads = env.reads;
    EXPECT_TRUE(IxionDc1SfxAcquire());
    EXPECT_EQ(env.reads, reads);
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 2);

    // Silent by source: no message is sent, so the mix stays silent.
    EXPECT_EQ(IxionDc1SfxPlay(2406, 1.0f, 0.0f, 0, "test", "e104a", 0), 0);
    CSnd.Step();
    EXPECT_EQ(RenderRms(), 0.0);
    // Unmapped: not played.
    EXPECT_EQ(IxionDc1SfxPlay(2356, 1.0f, 0.0f, 0, "test", "e101a", 0), -1);

    // Opar's spit (bank 0 program 64 key 86) and the hurt row (bank 1 program 36 key 90) both sound.
    EXPECT_EQ(IxionDc1SfxPlay(2412, 1.0f, 0.0f, 0, "test", "e104a", 0), 1);
    CSnd.Step();
    EXPECT_GT(RenderRms(), 0.001);
    IxionDc1SfxStop(2412, 0);
    CSnd.Step();
    fx::RenderFrames(audio::DefaultMixer(), 48000);
    EXPECT_LT(RenderRms(), 1e-3);
    EXPECT_EQ(IxionDc1SfxPlay(160, 1.0f, 0.0f, 0, "test", "e104a", 0), 1);
    CSnd.Step();
    EXPECT_GT(RenderRms(), 0.001);

    // Deferred release: the banks stay until the linger frame, then leave; DC2's bank stays.
    IxionDc1SfxReleaseLater(100, "test kill");
    IxionDc1SfxStep(100 + kIxionDc1SfxLinger - 1);
    EXPECT_TRUE(IxionDc1SfxResident());
    IxionDc1SfxStep(100 + kIxionDc1SfxLinger);
    EXPECT_FALSE(IxionDc1SfxResident());
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 0);
    EXPECT_EQ(SoundBankCount(kDc2EnemyPort), 1);
    fx::RenderFrames(audio::DefaultMixer(), 48000);
    EXPECT_EQ(IxionDc1SfxPlay(2412, 1.0f, 0.0f, 0, "test", "e104a", 0), -1);

    // The next spawn reloads; a donor installed during the linger keeps the banks; retirement is immediate.
    ASSERT_TRUE(IxionDc1SfxAcquire());
    IxionDc1SfxReleaseLater(0, "test kill");
    EXPECT_TRUE(IxionDc1SfxAcquire()); // a new donor before the linger ends keeps the banks
    IxionDc1SfxStep(100000);
    EXPECT_TRUE(IxionDc1SfxResident());
    IxionDc1SfxReleaseNow("test retire");
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 0);
    EXPECT_EQ(SoundBankCount(kDc2EnemyPort), 1);
}

TEST(IxionDc1Sfx, SoundResetIsDetectedAndBanksReload) {
    SfxEnv env("1");
    ASSERT_TRUE(IxionDc1SfxAcquire());
    CSnd.Exit();
    CSnd.Init(4, 0, 40, 0);
    EXPECT_FALSE(IxionDc1SfxResident());
    EXPECT_EQ(IxionDc1SfxPlay(2412, 1.0f, 0.0f, 0, "test", "e104a", 0), -1);
    ASSERT_TRUE(IxionDc1SfxAcquire());
    EXPECT_EQ(SoundBankCount(kIxionDc1SePort), 2);
}

TEST(IxionDc1Sfx, ReleaseRefusesLinkedGamePorts) {
    CSnd.Exit();
    CSnd.Init(4, 0, 40, 0);
    EXPECT_FALSE(SoundReleaseHdBd(2));  // linked to 14
    EXPECT_FALSE(SoundReleaseHdBd(14)); // linked to 2
    EXPECT_FALSE(SoundReleaseHdBd(1));  // shares its address with 15, 2 and 14
    EXPECT_TRUE(SoundReleaseHdBd(kIxionDc1SePort));
    CSnd.Exit();
}
