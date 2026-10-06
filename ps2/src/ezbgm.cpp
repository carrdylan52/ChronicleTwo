#include "common.h"
#include "ezbgm.hpp"
#include "sound.hpp"
#include <sifrpc.h>
#include <cstdio>

/** Client connection to the EZBGM IOP server. */
static sceSifClientData gCd2;
/** Command send and response buffer shared with the EZBGM server. */
static int sbuff[16];

// Code (.text)
int ezBgmInit() {
    printf("EZ_BGMINIT START \n");
    sceSifInitRpc(0);
    do {
        if (sceSifBindRpc(&gCd2, 0x12345, 0) < 0) {
            printf("error: sceSifBindRpc \n");
            for (;;) {}
        }
        int wait = 10000;
        do {
        } while (wait--);
    } while (gCd2.server == 0);
    return 1;
}

int ezBgm(int command, int argument) {
    switch (command & EZBGM_COMMAND_MASK) {
    case EZBGM_OPEN:
    case EZBGM_UNK_8A00:
    case EZBGM_OPEN_FROM_PACK:
        if (sceSifCheckStatRpc(&gCd2)) {
            printf("########### Rpc is bussy1!! \n");
            return 0;
        }
        sceSifCallRpc(&gCd2, command, 1, (void *)argument, 0x40, sbuff, 0x40, 0, 0);
        break;
    case EZBGM_PRELOAD:
        if (sceSifCheckStatRpc(&gCd2)) {
            printf("########### Rpc is bussy2!! \n");
            return 0;
        }
        sbuff[0] = argument;
        sceSifCallRpc(&gCd2, command, 1, sbuff, 0x10, sbuff, 0x40, 0, 0);
        break;
    default:
        if (sceSifCheckStatRpc(&gCd2)) {
            printf("########### Rpc is bussy3!! \n");
            return 0;
        }
        sbuff[0] = argument;
        sceSifCallRpc(&gCd2, command, 0, sbuff, 0x10, sbuff, 0x40, 0, 0);
        break;
    }
    return sbuff[0];
}

int CSound::StreamOpenState() {
    return sceSifCheckStatRpc(&gCd2);
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/ezbgm", at_32__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/ezbgm", at_33__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/ezbgm", at_52__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/ezbgm", at_53__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/ezbgm", at_54__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(sbuff__3, 0x40);
INCLUDE_BSS(gCd2, 0x30);
