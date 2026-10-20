#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Ixion: a DC1 donor monster's own sound effects, played by DC2's mixer.
//
// Opt-in through DC2_IXION_DC1_SFX=1 (with a DC1 donor installed). Without it nothing loads and the
// DC1 sound natives only log, as before. With it, the first installed donor loads Dark Cloud's
// floor-1 SE banks (snd100.snd's d01a_g and d01a_i, read from the private DC2_IXION_EXTRA_DATA
// overlay as sound/ixion_dc1/d01a_<g|i>.<hd|bd>) with SoundLoadHdBdAdd onto driver port 6, which no
// DC2 game port maps to (snd_mngr.cpp GetCSndPortNo) and which is not the movie port (5). DC2's own
// banks and ports are never touched. The banks leave when the room retires its donors, or a few
// seconds after the last donor dies so its death sounds can finish.
//
// A DC1 SE id resolves the way Chronicle's SndSePlay does (ps2/src/snd.cpp GetSeInfo :789-816 and
// the se_info/dun rows): DC1 "bank" is the hd program and "prog" is the key (port/src/sound.cpp
// SE_Play(port, se_no, voice) :544-549; mixer EffectOn). DC1 port 10 is slot g (LoadSlotG) and
// port 14 is slot i (LoadSlotI). The volume is setbl's first row for (program, key)
// (SndInitSeTable/GetSeNo, port/src/sound.cpp:317-324).

struct IxionDc1SeRoute {
    int  se;      // DC1 SE id
    char file;    // 'g' = d01a_g (DC1 port 10), 'i' = d01a_i (DC1 port 14)
    int  program; // hd program (the DC1 row's "bank")
    int  key;     // key (the DC1 row's "prog")
    int  volume;  // setbl VOL of the first (program, key) row
    bool audible; // false: the bank has no sample layer for the key, so retail is silent too
};

inline constexpr int kIxionDc1SePort = 6;
// After the last donor dies the banks stay this many frames, so the death sounds finish.
inline constexpr int kIxionDc1SfxLinger = 180;

// The DC1 route of an SE id, or null when round 1 does not map it.
const IxionDc1SeRoute *IxionDc1SeLookup(int se);
// True when DC2_IXION_DC1_SFX is set to a value other than 0.
bool IxionDc1SfxRequested();
// Loads the banks when requested and not resident (a donor was installed). True when resident.
bool IxionDc1SfxAcquire();
// Removes the banks from the private port now (room retirement), or after kIxionDc1SfxLinger frames.
void IxionDc1SfxReleaseNow(const char *reason);
void IxionDc1SfxReleaseLater(int frame, const char *reason);
// Once per monster think: completes a deferred release whose frame has come.
void IxionDc1SfxStep(int frame);
bool IxionDc1SfxResident();
// The DC2 bank index of a DC1 bank file on the private port, or -1.
int IxionDc1SfxBank(char file);
// Plays a DC1 SE. level (0..1) scales the setbl volume and pan is -1..1 (DC1 SndGetVolPan/SndGetPanf).
// Returns 1 when played, 0 when silent by source, -1 when not played (no opt-in, no banks, unmapped).
// Every outcome is logged with the id, bank, program and key.
int IxionDc1SfxPlay(int se, float level, float pan, int voice, const char *source, const char *donor, int slot);
void IxionDc1SfxStop(int se, int voice);

// Test seam: replaces the overlay reader (name -> bytes). Null restores the overlay reader.
using IxionDc1SfxReader = std::function<bool(const std::string &name, std::vector<std::uint8_t> &bytes)>;
void IxionDc1SfxTestSetReader(IxionDc1SfxReader reader);
