#include "common.h"
#include "vlgr_info.hpp"
#include "dataread.hpp"
#include "mg_memory.hpp"
#include "scriptinterpreter.hpp"
#include <cstring>
#include <cstdio>

/**
 * Number of villager places.
 */
static int PlaceInfoNum;
/**
 * Villager place definitions.
 */
static CVillagerPlaceInfo *PlaceInfo;
/**
 * Number of villager model records.
 */
static int VlgrInfoNum;
/**
 * Villager model records.
 */
static CVillagerInfo *VlgrInfo;
/**
 * Schedules indexed by villager number.
 */
static CVillagerPlace VlgrPlace[VLGR_PLACE_MAX];
/**
 * Memory used for schedule and model records.
 */
static mgCMemory *niStack;
/**
 * Villager schedule being filled.
 */
static CVillagerPlace *niVlgr;
/**
 * Number of schedule progress entries.
 */
static int niProgNum;
/**
 * Schedule time value cleared for each villager.
 */
static int niProgTime;
/**
 * Alternative place index.
 */
static int niProgDupliID;
/**
 * Whether the current progress condition applies afterward.
 */
static int niProgCon;
/**
 * Temporary schedule progress entries.
 */
static CVillagerPlace::ProgressInfo *niProgInfo;
/**
 * Progress entry being filled.
 */
static CVillagerPlace::ProgressInfo *niNowProgInfo;
/**
 * Places available to the schedule script.
 */
static CVillagerPlaceInfo *niPlaceInfo;
/**
 * Number of places available to the schedule script.
 */
static int niPlaceInfoNum;
/**
 * Next villager model record.
 */
static int niVlgrInfoIdx;
/**
 * Memory used for place definitions.
 */
static mgCMemory *vpiStack;
/**
 * Place definition being filled.
 */
static CVillagerPlaceInfo *vpiInfo;
/**
 * Number of story progress entries.
 */
static int ProgressNum;
/**
 * Story progress entries.
 */
static GAME_PROGRESS_INFO ProgressInfo[GAME_PROGRESS_MAX];
/**
 * Story progress table being filled.
 */
static GAME_PROGRESS_INFO *giGamePI;
/**
 * Memory used for story progress names.
 */
static mgCMemory *giStack;

static int niNPC(SPI_STACK *stack, int argument_count);
static int niNPC_END(SPI_STACK *stack, int argument_count);
static int niPROGRESS(SPI_STACK *stack, int argument_count);
static int niPROGRESS_END(SPI_STACK *stack, int argument_count);
static int niPLACE(SPI_STACK *stack, int argument_count);
static int niNOON_PLACE(SPI_STACK *stack, int argument_count);
static int niNIGHT_PLACE(SPI_STACK *stack, int argument_count);
static int niNPC_INFO_NUM(SPI_STACK *stack, int argument_count);
static int niNPC_INFO(SPI_STACK *stack, int argument_count);
static int vpiNPC_PLACE_NUM(SPI_STACK *stack, int argument_count);
static int vpiNPC_PLACE(SPI_STACK *stack, int argument_count);
static int vpiNPC_PLACE_END(SPI_STACK *stack, int argument_count);
static int vpiPLACE_POS(SPI_STACK *stack, int argument_count);
static int vpiMOVE_TO(SPI_STACK *stack, int argument_count);
static int vpiWAIT(SPI_STACK *stack, int argument_count);
static int vpiMOTION(SPI_STACK *stack, int argument_count);
static int vpiTALK_OFFSET(SPI_STACK *stack, int argument_count);
static int vpiMOVE_MOTION(SPI_STACK *stack, int argument_count);
static int vpiMOVE_SPEED(SPI_STACK *stack, int argument_count);
static int vpiSHADOW(SPI_STACK *stack, int argument_count);
static int giPROG_INFO(SPI_STACK *stack, int argument_count);
static int vpiGetMotionID(char *name);

/**
 * Villager schedule and model configuration tags.
 */
static SPI_TAG_PARAM ni_tag[] = {
    {"NPC", niNPC},
    {"NPC_END", niNPC_END},
    {"PROGRESS", niPROGRESS},
    {"PROGRESS_END", niPROGRESS_END},
    {"PLACE", niPLACE},
    {"NOON_PLACE", niNOON_PLACE},
    {"NIGHT_PLACE", niNIGHT_PLACE},
    {"NPC_INFO_NUM", niNPC_INFO_NUM},
    {"NPC_INFO", niNPC_INFO},
    {NULL, NULL}
};

/**
 * Villager placement configuration tags.
 */
static SPI_TAG_PARAM tag[] = {
    {"NPC_PLACE_NUM", vpiNPC_PLACE_NUM},
    {"NPC_PLACE", vpiNPC_PLACE},
    {"NPC_PLACE_END", vpiNPC_PLACE_END},
    {"PLACE_POS", vpiPLACE_POS},
    {"MOTION", vpiMOTION},
    {"MOVE_TO", vpiMOVE_TO},
    {"WAIT", vpiWAIT},
    {"TALK_OFFSET", vpiTALK_OFFSET},
    {"MOVE_MOTION", vpiMOVE_MOTION},
    {"MOVE_SPEED", vpiMOVE_SPEED},
    {"SHADOW", vpiSHADOW},
    {NULL, NULL}
};

/**
 * Story progress configuration tags.
 */
static SPI_TAG_PARAM gi_tag[] = {
    {"PROG_INFO", giPROG_INFO},
    {NULL, NULL}
};

// Code (.text)
CVillagerPlaceInfo *GetVlgrPlaceInfo(int place_no) {
    if (place_no < 0 || place_no >= PlaceInfoNum) {
        return NULL;
    }
    return &PlaceInfo[place_no];
}

CVillagerPlace *GetVlgrPlaceTable(int *num) {
    *num = VLGR_PLACE_MAX;
    return VlgrPlace;
}

CVillagerInfo *GetVillagerInfo(int vlgr_id) {
    CVillagerInfo *table = VlgrInfo;
    int low;
    int high;
    int middle;

    if (table == NULL || VlgrInfoNum <= 0) {
        return NULL;
    }
    low = 0;
    high = VlgrInfoNum - 1;
    while (low < high) {
        middle = (low + high) / 2;
        if (table[middle].vlgr_id < vlgr_id) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (vlgr_id == table[low].vlgr_id) {
        return &table[low];
    }
    return NULL;
}

int GetVillagerModelName(int vlgr_id, char *name) {
    CVillagerInfo *info;

    info = GetVillagerInfo(vlgr_id);
    *name = 0;
    if (info == NULL) {
        return 0;
    }
    sprintf(name, "chara/%s.chr", info->model_name);
    return 1;
}

/**
 * Selects and clears the villager placement being configured.
 */
static int niNPC(SPI_STACK *stack, int argument_count) {
    niVlgr = NULL;
    int villager_no = spiGetStackInt(stack);
    if (villager_no < 0 || villager_no >= VLGR_PLACE_MAX) {
        return 0;
    }
    niVlgr = &VlgrPlace[villager_no];
    memset(niVlgr, 0, sizeof(CVillagerPlace));
    niProgNum = 0;
    niProgTime = 0;
    niProgDupliID = 0;
    return 1;
}

/**
 * Ends configuration of the selected villager placement.
 */
static int niNPC_END(SPI_STACK *stack, int argument_count) {
    niVlgr = NULL;
    return 1;
}

/**
 * Selects or creates the placement settings for a game progress value.
 */
static int niPROGRESS(SPI_STACK *stack, int argument_count) {
    int progress = spiGetStackInt(stack++);
    niNowProgInfo = NULL;
    for (int index = 0; index < niProgNum; index++) {
        if (progress == niProgInfo[index].progress) {
            niNowProgInfo = &niProgInfo[index];
            break;
        }
    }
    if (niNowProgInfo == NULL) {
        if (niProgNum >= GAME_PROGRESS_MAX) {
            return 0;
        }
        niNowProgInfo = &niProgInfo[niProgNum++];
        niNowProgInfo->Init();
    }
    niNowProgInfo->progress = progress;
    niProgDupliID = spiGetStackInt(stack++);
    niProgCon = 0;
    char *condition = spiGetStackString(stack);
    if (condition != NULL && strcmp(condition, "\210\310\214\343") == 0) {
        niProgCon = 1;
    }
    niNowProgInfo->after = niProgCon;
    return 1;
}

/**
 * Copies the configured progress settings into the villager placement.
 */
static int niPROGRESS_END(SPI_STACK *stack, int argument_count) {
    if (niVlgr == NULL) {
        return 0;
    }
    if (niProgNum <= 0) {
        return 1;
    }
    u32 size = niProgNum * sizeof(CVillagerPlace::ProgressInfo);
    u32 blocks;
    if (size & 0xF) {
        blocks = (size >> 4) + 1;
    } else {
        blocks = size >> 4;
    }
    niVlgr->prog_info = new (niStack->Alloc(blocks + 2)) CVillagerPlace::ProgressInfo[niProgNum];
    niVlgr->prog_num = 0;
    if (niVlgr->prog_info == NULL) {
        return 0;
    }
    niVlgr->prog_num = niProgNum;
    for (int index = 0; index < niProgNum; index++) {
        niVlgr->prog_info[index] = niProgInfo[index];
    }
    return 1;
}

/**
 * Accepts the placement separator without changing state.
 */
static int niPLACE(SPI_STACK *stack, int argument_count) {
    return 1;
}

/**
 * Sets the current progress setting's daytime placement.
 */
static int niNOON_PLACE(SPI_STACK *stack, int argument_count) {
    int place_no = spiGetStackInt(stack);
    if (place_no < 0 || place_no >= niPlaceInfoNum) {
        return 0;
    }
    niNowProgInfo->place[niProgDupliID][VLGR_TIME_NOON] = &niPlaceInfo[place_no];
    return 1;
}

/**
 * Sets the current progress setting's nighttime placement.
 */
static int niNIGHT_PLACE(SPI_STACK *stack, int argument_count) {
    int place_no = spiGetStackInt(stack);
    if (place_no < 0 || place_no >= niPlaceInfoNum) {
        return 0;
    }
    niNowProgInfo->place[niProgDupliID][VLGR_TIME_NIGHT] = &niPlaceInfo[place_no];
    return 1;
}

/**
 * Allocates the configured number of villager information records.
 */
static int niNPC_INFO_NUM(SPI_STACK *stack, int argument_count) {
    VlgrInfoNum = spiGetStackInt(stack);
    int count = VlgrInfoNum;
    u32 size = count * sizeof(CVillagerInfo);
    u32 blocks;
    if (size & 0xF) {
        blocks = (size >> 4) + 1;
    } else {
        blocks = size >> 4;
    }
    VlgrInfo = (CVillagerInfo *)(int)new (niStack->Alloc(blocks + 2)) CVillagerInfo[count];
    if (VlgrInfo == NULL) {
        VlgrInfoNum = 0;
    }
    niVlgrInfoIdx = 0;
    return 1;
}

CVillagerInfo::CVillagerInfo(void) {
    vlgr_id = -1;
    model_name = NULL;
    house_type = 0;
    hide_frames = NULL;
    show_frames = NULL;
    unk_18 = -1;
    unk_14 = -1;
}

/**
 * Fills the next villager record with its model and frame settings.
 */
static int niNPC_INFO(SPI_STACK *stack, int argument_count) {
    if (niVlgrInfoIdx >= VlgrInfoNum) {
        return 0;
    }
    CVillagerInfo *info = &VlgrInfo[niVlgrInfoIdx];
    info->vlgr_id = spiGetStackInt(stack++);
    char *name = spiGetStackString(stack++);
    if (name != NULL) {
        info->model_name = mgCopyString(name, niStack);
    }
    if (argument_count >= 3) {
        info->house_type = spiGetStackInt(stack++);
    }
    if (argument_count >= 4) {
        char *show_frames = spiGetStackString(stack++);
        if (show_frames != NULL) {
            info->show_frames = mgCopyString(show_frames, niStack);
        }
    }
    if (argument_count >= 5) {
        char *hide_frames = spiGetStackString(stack++);
        if (hide_frames != NULL) {
            info->hide_frames = mgCopyString(hide_frames, niStack);
        }
    }
    if (argument_count >= 7) {
        info->unk_14 = spiGetStackInt(stack++);
        info->unk_18 = spiGetStackInt(stack);
    }
    niVlgrInfoIdx++;
    return 1;
}

void LoadNPCInfo(char *script, int size, mgCMemory *stack) {
    CVillagerPlace::ProgressInfo progressTable[GAME_PROGRESS_MAX];

    niProgInfo = progressTable;
    niStack = stack;
    niVlgr = NULL;
    niProgNum = 0;
    niNowProgInfo = NULL;
    niPlaceInfo = PlaceInfo;
    niPlaceInfoNum = PlaceInfoNum;
    CScriptInterpreter interpreter;
    interpreter.SetTag(ni_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

void LoadPlaceInfo(char *script, int size, mgCMemory *stack) {
    vpiStack = stack;
    vpiInfo = NULL;
    CScriptInterpreter interpreter;
    interpreter.SetTag(tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

/**
 * Allocates the configured number of placement records.
 */
static int vpiNPC_PLACE_NUM(SPI_STACK *stack, int argument_count) {
    int count = spiGetStackInt(stack);
    if (count <= 0) {
        return 0;
    }

    u32 blocks;
    if (((u32)count * sizeof(CVillagerPlaceInfo)) & 0xF) {
        blocks = (((u32)count * sizeof(CVillagerPlaceInfo)) >> 4) + 1;
    } else {
        blocks = ((u32)count * sizeof(CVillagerPlaceInfo)) >> 4;
    }
    void *block = vpiStack->Alloc(blocks + 2);
    CVillagerPlaceInfo *places = new ((u_long128 *)block) CVillagerPlaceInfo[count];
    if (places == NULL) {
        return 0;
    }
    PlaceInfo = places;
    PlaceInfoNum = count;
    return 1;
}

CVillagerPlaceInfo::CVillagerPlaceInfo() {
    memset(this, 0, sizeof(CVillagerPlaceInfo));
    map_no = -1;
}

/**
 * Selects a placement and sets its map and walking motion.
 */
static int vpiNPC_PLACE(SPI_STACK *stack, int argument_count) {
    int place_no = spiGetStackInt(stack++);
    int id = spiGetStackInt(stack);
    vpiInfo = &PlaceInfo[place_no];
    vpiInfo->map_no = id;
    vpiInfo->move_motion = VLGR_MOTION_WALK;
    return 1;
}

/**
 * Ends configuration of the selected placement.
 */
static int vpiNPC_PLACE_END(SPI_STACK *stack, int argument_count) {
    vpiInfo = NULL;
    return 1;
}

/**
 * Sets the selected placement's position and fourth coordinate.
 */
static int vpiPLACE_POS(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }
    spiGetStackVector(vpiInfo->pos, stack);
    vpiInfo->pos[3] = spiGetStackFloat(stack += 3);
    return 1;
}

/**
 * Appends a movement node to the selected placement's route.
 */
static int vpiMOVE_TO(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }
    CVillagerPlaceInfo::Node *node = vpiInfo->Add(vpiStack);
    if (node == NULL) {
        return 0;
    }
    node->type = VLGR_ROUTE_MOVE;
    spiGetStackVector(node->pos, stack);
    node->pos[3] = spiGetStackFloat(stack += 3);
    return 1;
}

/**
 * Appends a wait node with its duration and motion.
 */
static int vpiWAIT(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }
    CVillagerPlaceInfo::Node *node = vpiInfo->Add(vpiStack);
    if (node == NULL) {
        return 0;
    }
    node->type = VLGR_ROUTE_WAIT;
    char *posture = spiGetStackString(stack++);
    int motion_end = 0;
    if (posture != NULL) {
        if (strcmp(posture, "mtn") == 0) {
            motion_end = 1;
        }
    }
    node->wait.motion_end = motion_end;
    node->wait.time = spiGetStackInt(stack++);
    char *motion_name = NULL;
    if (argument_count >= 3) {
        motion_name = spiGetStackString(stack);
    }
    node->wait.motion = vpiGetMotionID(motion_name);
    return 1;
}

/**
 * Sets the selected placement's sitting motion when requested.
 */
static int vpiMOTION(SPI_STACK *stack, int argument_count) {
    char *name;

    if (vpiInfo == NULL) {
        return 0;
    }
    name = spiGetStackString(stack);
    if (name == NULL) {
        return 1;
    }
    if (strcmp(name, "sit") == 0) {
        vpiInfo->motion = VLGR_MOTION_SIT;
    }
    return 1;
}

/**
 * Sets the selected placement's talking offset.
 */
static int vpiTALK_OFFSET(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }
    spiGetStackVector(vpiInfo->talk_offset, stack);
    return 1;
}

/**
 * Sets the selected placement's movement motion by name.
 */
static int vpiMOVE_MOTION(SPI_STACK *stack, int argument_count) {
    char *name;

    if (vpiInfo == NULL) {
        return 0;
    }
    name = spiGetStackString(stack);
    if (name == NULL) {
        return 1;
    }
    vpiInfo->move_motion = vpiGetMotionID(name);
    return 1;
}

/**
 * Sets the selected placement's movement speed.
 */
static int vpiMOVE_SPEED(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }
    vpiInfo->move_speed = spiGetStackFloat(stack);
    return 1;
}

/**
 * Sets whether the selected placement suppresses its shadow.
 */
static int vpiSHADOW(SPI_STACK *stack, int argument_count) {
    if (vpiInfo == NULL) {
        return 0;
    }

    vpiInfo->no_shadow = (u8)((spiGetStackInt(stack) != 0) ^ 1);
    return 1;
}

/**
 * Returns the motion number for a configuration name.
 */
static int vpiGetMotionID(char *name) {
    if (name == NULL || *name == 0) {
        return -1;
    }
    if (strcmp(name, "sit") == 0) {
        return VLGR_MOTION_SIT;
    }
    if (strcmp(name, "stand") == 0) {
        return VLGR_MOTION_STAND;
    }
    if (strcmp(name, "special") == 0) {
        return VLGR_MOTION_SPECIAL;
    }
    if (strcmp(name, "walk") == 0) {
        return VLGR_MOTION_WALK;
    }
    return (strcmp(name, "run") == 0) ? VLGR_MOTION_RUN : VLGR_MOTION_NONE;
}

/**
 * Sets the chapter, section, order and name of a game progress entry.
 */
static int giPROG_INFO(SPI_STACK *stack, int argument_count) {
    int progress = spiGetStackInt(stack++);
    if (progress < 0 || progress >= GAME_PROGRESS_MAX) {
        return 0;
    }
    giGamePI[progress].chapter = spiGetStackInt(stack++);
    giGamePI[progress].section = spiGetStackInt(stack++);
    giGamePI[progress].order = spiGetStackInt(stack++);
    char *name = spiGetStackString(stack);
    if (name != NULL) {
        giGamePI[progress].name = mgCopyString(name, giStack);
    }
    return 1;
}

void LoadGameInfo(mgCMemory *stack) {
    int script_size;
    char script[0x19000];
    if (LoadFile2("place.cfg", script, &script_size, 0) == 0) {
        return;
    }
    LoadPlaceInfo(script, script_size, stack);
    for (int progress = 0; progress < GAME_PROGRESS_MAX; progress++) {
        ProgressInfo[progress].section = 0;
        ProgressInfo[progress].chapter = 0;
        ProgressInfo[progress].name = NULL;
        ProgressInfo[progress].order = 0;
    }
    if (LoadFile2("npc_place4.cfg", script, &script_size, 0) == 0) {
        return;
    }
    giGamePI = ProgressInfo;
    giStack = stack;
    ProgressNum = GAME_PROGRESS_MAX;
    CScriptInterpreter interpreter;
    interpreter.SetTag(gi_tag);
    interpreter.SetScript(script, script_size);
    interpreter.Run();
    LoadNPCInfo(script, script_size, stack);
    printf("rm %d\n", stack->stack_size - stack->stack_used);
}

GAME_PROGRESS_INFO *GetGameProgressInfo(int progress) {
    if (progress < 0 || progress >= ProgressNum) {
        return NULL;
    }
    return &ProgressInfo[progress];
}

int GetGameChapter(int progress) {
    GAME_PROGRESS_INFO *info = GetGameProgressInfo(progress);

    if (info != NULL) {
        return info->chapter;
    }
    return 0;
}

int GetGameProgressNum() {
    return ProgressNum;
}

CVillagerPlace::CVillagerPlace() {
    memset(this, 0, sizeof(CVillagerPlace));
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", ni_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", tag__9__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", gi_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_214__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_250__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_351__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_352__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_353__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_354__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_355__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_356__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_357__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_358__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_359__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_364__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_365__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_366__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_367__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_368__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_369__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_370__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_371__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_372__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_373__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_374__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_439__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_450__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_495__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_496__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_497__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_498__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_509__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_555__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_556__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_557__DATA);

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)
INCLUDE_BSS(PlaceInfoNum, 0x4);
INCLUDE_BSS(PlaceInfo, 0x4);
INCLUDE_BSS(VlgrInfoNum, 0x4);
INCLUDE_BSS(VlgrInfo, 0x4);
INCLUDE_BSS(ProgressNum, 0x4);
INCLUDE_BSS(niStack, 0x4);
INCLUDE_BSS(niVlgr, 0x4);
INCLUDE_BSS(niProgNum, 0x4);
INCLUDE_BSS(niProgTime, 0x4);
INCLUDE_BSS(niProgDupliID, 0x4);
INCLUDE_BSS(niProgCon, 0x4);
INCLUDE_BSS(niProgInfo, 0x4);
INCLUDE_BSS(niNowProgInfo, 0x4);
INCLUDE_BSS(niPlaceInfo, 0x4);
INCLUDE_BSS(niPlaceInfoNum, 0x4);
INCLUDE_BSS(niVlgrInfoIdx, 0x4);
INCLUDE_BSS(vpiStack, 0x4);
INCLUDE_BSS(vpiInfo, 0x4);
INCLUDE_BSS(giGamePI, 0x4);
INCLUDE_BSS(giStack, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(VlgrPlace, 0x1000);
INCLUDE_BSS(ProgressInfo, 0xC00);
