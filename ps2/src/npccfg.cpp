#include "common.h"
#include "npccfg.hpp"
#include "scriptinterpreter.hpp"
#include "dataread.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include <cstdio>
#include <cstring>

/** Number of initialized party-character entries. */
static int NpcBaseDataTotalNum;
/** Party-character entries loaded from the NPC script. */
static NPC_BASE_DATA NpcBaseData[180];
/** Next free party-character entry while the NPC script runs. */
static u8 npc_spi_count_num;

static int _NPC_NUM(SPI_STACK *stack, int argument_count);
static int _NPC_INFO(SPI_STACK *stack, int argument_count);

/**
 * Commands accepted by the NPC configuration script.
 */
static SPI_TAG_PARAM npc_spitag[3] = {
    {"NPC_NUM", _NPC_NUM}, {"NPC_INFO", _NPC_INFO}, {NULL, NULL}
};

// Code (.text)
/**
 * Records the number of party characters declared by the NPC script.
 */
static int _NPC_NUM(SPI_STACK *stack, int argument_count) {
    NpcBaseDataTotalNum = spiGetStackInt(stack);
    return 1;
}

/**
 * Reads one party-character record from the NPC script's stack.
 */
static int _NPC_INFO(SPI_STACK *stack, int argument_count) {
    NPC_BASE_DATA *data = &NpcBaseData[npc_spi_count_num++];
    SPI_STACK     *argument = stack;
    int            chara_no = spiGetStackInt(argument++);
    char          *name = spiGetStackString(argument++);
    char          *model = spiGetStackString(argument++);
    data->chara_no = chara_no;
    if (name != 0) {
        strcpy(data->name, name);
        if (strlen(name) > sizeof(data->name) - 1) {
            printf("NAME OVER!!!!!!:%s\n", name);
        }
    }
    if (model != 0) {
        strcpy(data->model, model);
    }
    data->unk_31 = spiGetStackInt(argument++);
    data->ability_num = spiGetStackInt(argument++);
    data->max_npc_point = spiGetStackInt(argument++);
    data->ability_cost[0] = spiGetStackInt(argument++);
    data->ability_cost[1] = spiGetStackInt(argument++);
    data->ability_cost[2] = spiGetStackInt(argument++);
    data->ability_cost[3] = spiGetStackInt(argument++);
    data->debug_flag = spiGetStackInt(argument);
    return 1;
}

void LoadNPCCfg() {
    u_long128 work[2048];
    char path[32];
    int size;
    u_long128 *buffer = MenuCalcBufAlignment(work);
    sprintf(path, "npc%d.cfg", LanguageCode);
    npc_spi_count_num = 0;
    if (LoadFile2(path, buffer, &size, 0) != 0) {
        CScriptInterpreter interpreter;
        interpreter.SetTag(npc_spitag);
        interpreter.SetScript((char *)buffer, size);
        interpreter.Run();
    }
    NpcBaseDataTotalNum = npc_spi_count_num;
}

int GetPartyCharaMessage(int chara_no, int type, int event) {
    static signed char message_offsets[16] = {
        0, 10, 20, 30, 2, 40, 45, 50, 55, 0, 90, 60, 0, 21, 25, 0
    };
    if (GetPartyNPCData(chara_no) == 0) {
        return 0;
    }
    int message = message_offsets[type] + chara_no * 100;
    if (type == 12) {
        message = chara_no + 3000;
    }
    if (event != 0) {
        message += 30000;
    }
    return message;
}

char *GetNPCModelName(int chara_no) {
    NPC_BASE_DATA *data = GetPartyNPCData(chara_no);
    if (data != 0) {
        return data->model;
    }
    return 0;
}

char *GetNPCName(int chara_no) {
    NPC_BASE_DATA *data = GetPartyNPCData(chara_no);
    if (data != 0) {
        return data->name;
    }
    return 0;
}

char *GetPartyCharaModelName(int chara_no, int type) {
    static char path[0x40];
    static char info_cfg[] = "info.cfg";
    if (chara_no <= 0 || chara_no > 32) {
        return 0;
    }
    path[0] = '\0';
    char *model = GetNPCModelName(chara_no);
    if (model != 0) {
        if (type == NPC_MODEL_PATH_CHARA) {
            strcpy(path, "chara/");
            strcat(path, model);
            strcat(path, ".chr");
            return path;
        }
        if (type == NPC_MODEL_PATH_INFO) {
            return info_cfg;
        }
        if (type == NPC_MODEL_PATH_EVENT_TRAIN) {
            sprintf(path, "event/train/t%s.chr", model);
            return path;
        }
        if (type == NPC_MODEL_PATH_MENU) {
            sprintf(path, "menu/npc/t%s.chr", model);
            return path;
        }
    }
    return 0;
}

NPC_BASE_DATA *GetPartyNPCData(int chara_no) {
    for (int i = 0; i < NpcBaseDataTotalNum; i++) {
        if (NpcBaseData[i].chara_no == chara_no) {
            return &NpcBaseData[i];
        }
    }
    return 0;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", npc_spitag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", typetbl_853__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", infocfg_886__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_838__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_839__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_840__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_847__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_898__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_899__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_900__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/npccfg", at_901__3__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(NpcBaseDataTotalNum, 0x4);
INCLUDE_BSS(npc_spi_count_num, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(NpcBaseData, 0x2600);
INCLUDE_BSS(path_885, 0x40);
