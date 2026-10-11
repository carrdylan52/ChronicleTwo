#include "sound.hpp"

PORT_OVERRIDE void CSound::StopVoice(int core) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SndInReverb(bool enable) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SetReverb(int core, int mode, int depth) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void set_spu(int mode0, int mode1, int depth0, int depth1) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int TransHdBd(int hd, int hd_size, int bd, int bd_size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CSound::Init(int mode0, int mode1, int depth0, int depth1) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CSound::Exit() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::DEL_PORT(int port) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SE_Play(int port, int bank, int program, int key, int pan, int velocity, int volume, int pitch, int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SE_SetVol(int port, int bank, int program, int key, int volume, int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SE_SetPan(int port, int bank, int program, int key, int pan, int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SE_Stop(int port, int bank, int program, int key, int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SetMasterVol(int core, int volume) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::LoadHdBd2(int port, int hd, int hd_size, int bd, int bd_size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CSound::LoadSeq(int port, int address, int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void CSound::SE_SetPitch(int port, int bank, int program, int key, int pitch, int id) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int CSound::TransBdState(int channel) {
    PS2_UNIMPLEMENTED();
}
