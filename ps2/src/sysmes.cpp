#include "common.h"
#include "sysmes.hpp"
#include "dataread.hpp"
#include "mainloop.hpp"
#include "nd_meswin.hpp"

mgCMemory    SystemMesStack;              /**< Working memory for system messages. */
static short SystemMesBuffer[0x6800];       /**< Text inserted into system message windows. */
static short SysMesBuffer[0x9C40];          /**< Message text loaded from the sysmes file. */
static ClsMes SystemMessage;               /**< First system message window. */
static ClsMes SystemMessage2;              /**< Second system message window. */
static ClsMes SystemMessage3;              /**< Third system message window. */

// Code (.text)
ClsMes *GetSystemMessage() {
    return GetSystemMessage(0);
}

ClsMes *GetSystemMessage(int index) {
    if (index == 2) {
        return &SystemMessage3;
    }
    if (index == 1) {
        return &SystemMessage2;
    }
    return &SystemMessage;
}

#ifdef NONMATCHING
void LoadSystemMes() {
    char *system_file;
    char *sysmes_file;
    switch (LanguageCode) {
    case LANG_JAPANESE:
        system_file = "meswin/system.mes";
        sysmes_file = "meswin/sysmes.mes";
        break;
    case LANG_FRENCH:
        system_file = "meswin/system_2.mes";
        sysmes_file = "meswin/sysmes_2.mes";
        break;
    case LANG_GERMAN:
        system_file = "meswin/system_3.mes";
        sysmes_file = "meswin/sysmes_3.mes";
        break;
    case LANG_ITALIAN:
        system_file = "meswin/system_4.mes";
        sysmes_file = "meswin/sysmes_4.mes";
        break;
    case LANG_SPANISH:
        system_file = "meswin/system_5.mes";
        sysmes_file = "meswin/sysmes_5.mes";
        break;
    default:
        system_file = "meswin/system_1.mes";
        sysmes_file = "meswin/sysmes_1.mes";
        break;
    }
    int size;
    LoadFile(system_file, SystemMesBuffer, &size);
    LoadFile(sysmes_file, SysMesBuffer, NULL);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sysmes", LoadSystemMes__Fv);
#endif

short *GetSystemMesBuffer() {
    return SystemMesBuffer;
}

short *GetSysMesBuffer() {
    return SysMesBuffer;
}

void CreateSystemMes(void) {
    CreateSystemMes(0, 0);
    CreateSystemMes(1, 0);
    CreateSystemMes(2, 0);
}

#ifdef NONMATCHING
void CreateSystemMes(int index, int unused) {
    ClsMes *message = GetSystemMessage(index);
    message->Init();
    GetSystemMessage(index)->Preset(5);
    GetSystemMessage(index)->SetBuff(GetSysMesBuffer());
    GetSystemMessage(index)->SetBuff_system(GetSystemMesBuffer());
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sysmes", CreateSystemMes__Fii);
#endif

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_482__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_483__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_484__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_485__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_486__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_487__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_488__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_489__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_490__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_491__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_492__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_493__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sysmes", at_494__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(SystemMesStack, 0x30);
INCLUDE_BSS(SystemMesBuffer, 0xD000);
INCLUDE_BSS(SysMesBuffer, 0x13880);
INCLUDE_BSS(SystemMessage, 0x2960);
INCLUDE_BSS(SystemMessage2, 0x2960);
INCLUDE_BSS(SystemMessage3, 0x2960);
