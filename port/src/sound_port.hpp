#pragma once

namespace audio {
class StreamChannel;
}

// Retail CSound passes bank and sequence addresses in int parameters; the host loads through these instead.
void SoundLoadHdBd(int port, const void *hd, int hd_size, const void *bd, int bd_size);
int  SoundLoadHdBdAdd(int port, const void *hd, int hd_size, const void *bd, int bd_size);
int  SoundLoadSeq(int port, const void *sq, int size, const char *name = nullptr);
// Banks resident on a driver port. SoundLoadHdBdAdd appends at this index.
int  SoundBankCount(int port);
// Stops a driver port and unbinds all of its banks. Refused (false) for a port that is linked to, or
// shares its sound processor with, another port, so a game port pair is never half-released.
bool SoundReleaseHdBd(int port);
// Config volume is a final gain, independent of retail fades and core volumes.
void SoundApplySettings();

// ezBgm's open commands carry the name's address in int; these take it as a pointer.
int EzBgmOpen(int channel, const char *name);
int EzBgmOpenFromPack(int channel, const char *name, const char *pack_name);

// Moves the stream channels on by one sound step when no audio output renders them.
void EzBgmStep();

audio::StreamChannel &EzBgmChannel(int channel);
