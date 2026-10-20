#include "common.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string_view>
#include <utility>

#include "dataread_port.hpp"
#include "ixion_dc1_sound.hpp"
#include "mglib_port.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "sound_port.hpp"

namespace {

// Round 1: Opar's ids and the engine's monster hit rows of Dark Cloud's floor-1 set (snd100, TABLE 1
// so ids 100-299 use the dun table, snd.cpp:895-899). Rows are Chronicle ps2/src/snd.cpp: se_info[]
// (table at :1646; rows 2406-2417 = {64, 80..91, 0, 10, 0}) and dun[] (table at :2407; dun[19],
// dun[59], dun[60], dun[62]). Volumes are setbl.txt rows 203, 251, 252, 254 and 1331-1342.
// "audible" is whether the bank has a sample layer for the key at velocity 127 (hdbank Resolve):
// d01a_g program 64 has 19 splits and none for keys 80, 83, 84 and 87-90 (wave5-se-bank.md).
constexpr IxionDc1SeRoute kRoutes[] = {
    {119, 'i', 30, 39, 46, true},    // dun[19]: Opar's death fall (prog_120)
    {159, 'i', 36, 89, 42, true},    // dun[59] SE_HIT_NO_DAMAGE (monstorunit.cpp:1611)
    {160, 'i', 36, 90, 76, true},    // dun[60] SE_MONSTER_HIT (monstorunit.cpp:1602)
    {162, 'i', 36, 92, 62, true},    // dun[62] SE_HIT_BLOCKED (monstorunit.cpp:1266)
    {2406, 'g', 64, 80, 36, false},
    {2407, 'g', 64, 81, 80, true},   // Opar step (native 140 window)
    {2408, 'g', 64, 82, 80, true},   // Opar step (native 140 window)
    {2409, 'g', 64, 83, 88, false},
    {2410, 'g', 64, 84, 88, false},
    {2411, 'g', 64, 85, 91, true},   // Opar flop (native 140 window)
    {2412, 'g', 64, 86, 46, true},   // Opar spit (native 141)
    {2413, 'g', 64, 87, 66, false},
    {2414, 'g', 64, 88, 73, false},
    {2415, 'g', 64, 89, 88, false},
    {2416, 'g', 64, 90, 88, false},
    {2417, 'g', 64, 91, 76, true},   // Opar collapse (native 141, prog_120)
};
constexpr char kFiles[] = {'g', 'i'};

struct Residency {
    bool resident = false;
    int  bank[2] = {-1, -1}; // DC2 bank index of d01a_g and d01a_i on kIxionDc1SePort
    int  release_frame = -1; // pending deferred release
    bool warned = false;
};
Residency          g_sfx;
IxionDc1SfxReader g_reader;

bool ReadOverlay(const std::string &name, std::vector<std::uint8_t> &bytes) {
    const std::string game_name = "sound/ixion_dc1/" + name;
    const auto path = DataReadFind(game_name.c_str());
    if (!path) return false;
    std::ifstream stream(*path, std::ios::binary);
    bytes.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    return stream.good() || stream.eof();
}

bool Read(const std::string &name, std::vector<std::uint8_t> &bytes) {
    return g_reader ? g_reader(name, bytes) : ReadOverlay(name, bytes);
}

int FileIndex(char file) { return file == 'g' ? 0 : file == 'i' ? 1 : -1; }

// A sound re-initialisation (CSound::Init/Exit resets every port) drops the banks behind our back.
bool StillBound() {
    return g_sfx.resident && SoundBankCount(kIxionDc1SePort) == 2;
}

} // namespace

const IxionDc1SeRoute *IxionDc1SeLookup(int se) {
    for (const IxionDc1SeRoute &route : kRoutes) {
        if (route.se == se) return &route;
    }
    return nullptr;
}

bool IxionDc1SfxRequested() {
    const char *value = std::getenv("DC2_IXION_DC1_SFX");
    return value != nullptr && *value != '\0' && std::string_view(value) != "0";
}

bool IxionDc1SfxAcquire() {
    if (!IxionDc1SfxRequested()) return false;
    g_sfx.release_frame = -1;
    if (StillBound()) return true;
    if (g_sfx.resident) {
        std::fprintf(stderr, "ixion dc1 sfx banks were dropped by a sound reset (port %d holds %d); reloading\n",
                     kIxionDc1SePort, SoundBankCount(kIxionDc1SePort));
        g_sfx = Residency{};
    }
    if (SoundBankCount(kIxionDc1SePort) != 0) {
        // Never append to a port someone else filled; DC2 itself never uses this port.
        std::fprintf(stderr, "ixion dc1 sfx refused: driver port %d already holds %d banks\n", kIxionDc1SePort,
                     SoundBankCount(kIxionDc1SePort));
        return false;
    }
    std::vector<std::uint8_t> hd[2], bd[2];
    for (int i = 0; i < 2; ++i) {
        const std::string stem = std::string("d01a_") + kFiles[i];
        if (!Read(stem + ".hd", hd[i]) || !Read(stem + ".bd", bd[i]) || hd[i].empty() || bd[i].empty()) {
            if (!g_sfx.warned) {
                std::fprintf(stderr, "ixion dc1 sfx: sound/ixion_dc1/%s.hd/.bd not in the overlay; DC1 sounds stay off\n",
                             stem.c_str());
                g_sfx.warned = true;
            }
            return false;
        }
    }
    for (int i = 0; i < 2; ++i) {
        const int bank_no = SoundBankCount(kIxionDc1SePort);
        if (SoundLoadHdBdAdd(kIxionDc1SePort, hd[i].data(), (int) hd[i].size(), bd[i].data(), (int) bd[i].size()) != 0 ||
            SoundBankCount(kIxionDc1SePort) != bank_no + 1) {
            std::fprintf(stderr, "ixion dc1 sfx: d01a_%c rejected by SoundLoadHdBdAdd\n", kFiles[i]);
            SoundReleaseHdBd(kIxionDc1SePort);
            return false;
        }
        g_sfx.bank[i] = bank_no;
    }
    CSnd.SetVol(kIxionDc1SePort, 256); // a reset mixer port starts at volume 0
    g_sfx.resident = true;
    std::fprintf(stderr, "ixion dc1 sfx banks loaded port=%d d01a_g=bank%d (hd %zu bd %zu) d01a_i=bank%d (hd %zu bd %zu)\n",
                 kIxionDc1SePort, g_sfx.bank[0], hd[0].size(), bd[0].size(), g_sfx.bank[1], hd[1].size(), bd[1].size());
    return true;
}

void IxionDc1SfxReleaseNow(const char *reason) {
    g_sfx.release_frame = -1;
    if (!g_sfx.resident) return;
    const bool released = SoundReleaseHdBd(kIxionDc1SePort);
    std::fprintf(stderr, "ixion dc1 sfx banks released port=%d reason=%s ok=%d banks_left=%d\n", kIxionDc1SePort,
                 reason != nullptr ? reason : "-", released ? 1 : 0, SoundBankCount(kIxionDc1SePort));
    g_sfx.resident = false;
    g_sfx.bank[0] = g_sfx.bank[1] = -1;
}

void IxionDc1SfxReleaseLater(int frame, const char *reason) {
    if (!g_sfx.resident || g_sfx.release_frame >= 0) return;
    g_sfx.release_frame = frame + kIxionDc1SfxLinger;
    std::fprintf(stderr, "ixion dc1 sfx banks release scheduled frame=%d reason=%s\n", g_sfx.release_frame,
                 reason != nullptr ? reason : "-");
}

void IxionDc1SfxStep(int frame) {
    if (g_sfx.release_frame >= 0 && frame >= g_sfx.release_frame) IxionDc1SfxReleaseNow("linger elapsed");
}

bool IxionDc1SfxResident() { return StillBound(); }

int IxionDc1SfxBank(char file) {
    const int index = FileIndex(file);
    return StillBound() && index >= 0 ? g_sfx.bank[index] : -1;
}

int IxionDc1SfxPlay(int se, float level, float pan, int voice, const char *source, const char *donor, int slot) {
    const IxionDc1SeRoute *route = IxionDc1SeLookup(se);
    const int frame = mgPortFrameCount();
    if (!StillBound()) {
        std::fprintf(stderr, "ixion dc1 sound %s frame=%d donor=%s slot=%d dc1_se=%d (not played: no DC1 banks)\n", source,
                     frame, donor, slot, se);
        return -1;
    }
    if (route == nullptr) {
        std::fprintf(stderr, "ixion dc1 sound %s frame=%d donor=%s slot=%d dc1_se=%d (not played: unmapped in round 1)\n",
                     source, frame, donor, slot, se);
        return -1;
    }
    const int bank = g_sfx.bank[FileIndex(route->file)];
    if (!route->audible) {
        std::fprintf(stderr,
                     "ixion dc1 sound %s frame=%d donor=%s slot=%d dc1_se=%d file=d01a_%c port=%d bank=%d program=%d key=%d "
                     "(silent by source: no sample layer)\n",
                     source, frame, donor, slot, se, route->file, kIxionDc1SePort, bank, route->program, route->key);
        return 0;
    }
    // DC1 SndGetVolf (snd.cpp:1067-1094): the setbl level times the 0..1 distance level, 0..127.
    const int volume = std::clamp((int) ((float) route->volume * std::clamp(level, 0.0f, 1.0f)), 0, 127);
    // DC1 SndGetPanf (snd.cpp:1096-1106).
    const int hw_pan = (int) (63.0f * std::clamp(pan, -1.0f, 1.0f)) + 64;
    sndSePlayPBPrKr(kIxionDc1SePort, bank, route->program, route->key, 127, volume, hw_pan, 0x2000, voice);
    std::fprintf(stderr,
                 "ixion dc1 sound play %s frame=%d donor=%s slot=%d dc1_se=%d file=d01a_%c port=%d bank=%d program=%d key=%d "
                 "vol=%d pan=%d voice=%d\n",
                 source, frame, donor, slot, se, route->file, kIxionDc1SePort, bank, route->program, route->key, volume,
                 hw_pan, voice);
    return 1;
}

void IxionDc1SfxStop(int se, int voice) {
    const IxionDc1SeRoute *route = IxionDc1SeLookup(se);
    if (route == nullptr || !route->audible || !StillBound()) return;
    sndSeStopPBPrKr(kIxionDc1SePort, g_sfx.bank[FileIndex(route->file)], route->program, route->key, voice);
}

void IxionDc1SfxTestSetReader(IxionDc1SfxReader reader) { g_reader = std::move(reader); }
