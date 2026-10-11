#include "dataread.hpp"

PORT_OVERRIDE int size_to_sector(int size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void ReadBG() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void BreakReadBG() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void InitCDFile() {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int LoadFile(char *path, void *buffer, int *out_size) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int LoadFile2(char *path, void *buffer, int *out_size, int mode) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE int WriteFile(char *path, void *buffer, int size) {
    PS2_UNIMPLEMENTED();
}
