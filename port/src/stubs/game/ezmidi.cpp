#include "ezmidi.hpp"

namespace {
bool g_bound;
}

PORT_OVERRIDE int ezMidiInit() {
    PS2_UNIMPLEMENTED();
}

// CAquarium's static constructor stops its sound port before ezMidiInit binds the EZMIDI client; the
// PS2 ran it ahead of sceSifInitRpc, so no command sent unbound reached the IOP.
PORT_OVERRIDE int ezMidi(int command, int argument) {
    if (!g_bound) {
        return argument;
    }
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int ezTransToIOP2(void *iop_address, void *ee_address, int size) {
    PS2_UNIMPLEMENTED();
}
