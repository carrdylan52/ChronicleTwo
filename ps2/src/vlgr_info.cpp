#include "common.h"

#include <cstdio>
#include <cstring>

#include "dataread.hpp"
#include "mg_memory.hpp"
#include "scriptinterpreter.hpp"
#include "villagermngr.hpp"
#include "vlgr_info.hpp"

/**
 * Number of villager places in the loaded table.
 */
static int PlaceInfoNum;
/**
 * Loaded villager place records.
 */
static CVillagerPlaceInfo           *PlaceInfo;
/**
 * Number of loaded villager model records.
 */
static int                           VlgrInfoNum;
/**
 * Loaded villager model and appearance records.
 */
static CVillagerInfo                *VlgrInfo;
extern CVillagerPlace VlgrPlace[VLGR_PLACE_MAX];
/**
 * Memory used to allocate villager schedule records.
 */
static mgCMemory                    *niStack;
/**
 * Villager schedule currently being parsed.
 */
static CVillagerPlace               *niVlgr;
/**
 * Number of progress conditions in the current schedule.
 */
static int                           niProgNum;
/**
 * Progress-time state of the current schedule.
 */
static int                           niProgTime;
/**
 * Alternative placement selected by the current progress condition.
 */
static int                           niProgDupliID;
/**
 * Whether the current condition applies from its progress point onward.
 */
static int                           niProgCon;
/**
 * Temporary progress conditions for the current villager.
 */
static CVillagerPlace::ProgressInfo *niProgInfo;
/**
 * Progress condition currently being parsed.
 */
static CVillagerPlace::ProgressInfo *niNowProgInfo;
/**
 * Place records available to the schedule parser.
 */
static CVillagerPlaceInfo           *niPlaceInfo;
/**
 * Number of places available to the schedule parser.
 */
static int                           niPlaceInfoNum;
/**
 * Next villager model record to fill.
 */
static int                           niVlgrInfoIdx;
/**
 * Memory used to allocate villager place records.
 */
static mgCMemory                    *vpiStack;
/**
 * Villager place currently being parsed.
 */
static CVillagerPlaceInfo           *vpiInfo;
extern SPI_TAG_PARAM                 ni_tag[];
extern SPI_TAG_PARAM                 tag__9[];
extern SPI_TAG_PARAM                 gi_tag[];
extern char                          at_351[];
extern char                          at_352[];
extern char                          at_353__2[];
extern char                          at_354[];
extern char                          at_355[];
extern char                          at_356__2[];
extern char                          at_357__2[];
extern char                          at_358__3[];
extern char                          at_359__2[];
extern char                          at_364__2[];
extern char                          at_365__2[];
extern char                          at_366__2[];
extern char                          at_367__2[];
extern char                          at_368__3[];
extern char                          at_369__5[];
extern char                          at_370__4[];
extern char                          at_371__3[];
extern char                          at_372__3[];
extern char                          at_373__4[];
extern char                          at_374__3[];
extern char                          at_509[];
/**
 * Number of story progress points.
 */
static int                           ProgressNum;
/**
 * Story progress points loaded from the configuration script.
 */
GAME_PROGRESS_INFO                   ProgressInfo[GAME_PROGRESS_MAX];
/**
 * Story progress records available to the game-info parser.
 */
static GAME_PROGRESS_INFO           *giGamePI;
/**
 * Memory used to allocate story progress names.
 */
static mgCMemory                    *giStack;

// Code (.text)
CVillagerPlaceInfo *GetVlgrPlaceInfo(int index) {
    if (index < 0 || index >= PlaceInfoNum) {
        return NULL;
    }

    return PlaceInfo + index;
}

CVillagerPlace *GetVlgrPlaceTable(int *count) {
    *count = VLGR_PLACE_MAX;
    return VlgrPlace;
}

CVillagerInfo *GetVillagerInfo(int villager_no) {
    CVillagerInfo *table = VlgrInfo;
    int            low;
    int            high;
    int            middle;

    if (table == NULL || VlgrInfoNum <= 0) {
        return NULL;
    }

    low = 0;
    high = VlgrInfoNum - 1;

    if (0 < high) {
        do {
            middle = (low + high) / 2;

            if (table[middle].vlgr_id < villager_no) {
                low = middle + 1;
            } else {
                high = middle;
            }
        } while (low < high);
    }

    if (villager_no == table[low].vlgr_id) {
        return &table[low];
    }

    return NULL;
}

int GetVillagerModelName(int villager_no, char *path) {
    CVillagerInfo *info;

    info = GetVillagerInfo(villager_no);
    *path = 0;

    if (info == NULL) {
        return 0;
    }

    sprintf(path, "chara/%s.chr", info->model_name);
    return 1;
}

/**
 *
 * Selects and clears a villager place entry while reading NPC configuration.
 *
 */
int niNPC(SPI_STACK *stack, int argument_count) {
    niVlgr = NULL;
    int villager_no = spiGetStackInt(stack);

    if (villager_no < 0 || villager_no >= VLGR_PLACE_MAX) {
        return 0;
    }

    niVlgr = VlgrPlace + villager_no;
    memset(niVlgr, 0, sizeof(CVillagerPlace));
    niProgNum = 0;
    niProgTime = 0;
    niProgDupliID = 0;
    return 1;
}

/**
 *
 * Ends the current villager place entry.
 *
 */
int niNPC_END(SPI_STACK *stack, int argc) {
    niVlgr = 0;
    return 1;
}

/**
 *
 * Selects or creates a villager progress condition and its alternate place index.
 *
 */
int niPROGRESS(SPI_STACK *stack, int argument_count) {
    int progress = spiGetStackInt(stack++);
    niNowProgInfo = NULL;

    for (int index = 0; index < niProgNum; index++) {
        if (progress == niProgInfo[index].progress) {
            niNowProgInfo = niProgInfo + index;
            break;
        }
    }

    if (niNowProgInfo == NULL) {
        if (niProgNum >= GAME_PROGRESS_MAX) {
            return 0;
        }

        niNowProgInfo = niProgInfo + niProgNum++;
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
 *
 * Copies the parsed progress conditions into the current villager entry.
 *
 */
int niPROGRESS_END(SPI_STACK *stack, int argument_count) {
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
 *
 * Accepts a place section tag without changing the current villager entry.
 *
 */
int niPLACE(SPI_STACK *stack, int argc) {
    return 1;
}

/**
 *
 * Assigns a daytime place to the current progress condition.
 *
 */
int niNOON_PLACE(SPI_STACK *stack, int argc) {
    int place_no = spiGetStackInt(stack);

    if (place_no < 0 || place_no >= niPlaceInfoNum) {
        return 0;
    }

    niNowProgInfo->place[niProgDupliID][0] = niPlaceInfo + place_no;
    return 1;
}

/**
 *
 * Assigns a nighttime place to the current progress condition.
 *
 */
int niNIGHT_PLACE(SPI_STACK *stack, int argc) {
    int place_no = spiGetStackInt(stack);

    if (place_no < 0 || place_no >= niPlaceInfoNum) {
        return 0;
    }

    niNowProgInfo->place[niProgDupliID][1] = niPlaceInfo + place_no;
    return 1;
}

/**
 *
 * Allocates the villager information table specified by a script.
 *
 */
int niNPC_INFO_NUM(SPI_STACK *stack, int argument_count) {
    VlgrInfoNum = spiGetStackInt(stack);
    int count = VlgrInfoNum;
    u32 size = count * sizeof(CVillagerInfo);
    u32 blocks;

    if (size & 0xF) {
        blocks = (size >> 4) + 1;
    } else {
        blocks = size >> 4;
    }

    VlgrInfo = (CVillagerInfo *) (int) new (niStack->Alloc(blocks + 2)) CVillagerInfo[count];

    if (VlgrInfo == NULL) {
        VlgrInfoNum = 0;
    }

    niVlgrInfoIdx = 0;
    return 1;
}

CVillagerInfo::CVillagerInfo() {
    vlgr_id = -1;
    model_name = NULL;
    house_type = 0;
    hide_frames = NULL;
    show_frames = NULL;
    unk_18 = -1;
    unk_14 = -1;
}

/**
 *
 * Stores one villager model, house type, and frame visibility entry.
 *
 */
int niNPC_INFO(SPI_STACK *stack, int argument_count) {
    if (niVlgrInfoIdx >= VlgrInfoNum) {
        return 0;
    }

    CVillagerInfo *info = VlgrInfo + niVlgrInfoIdx;
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

void LoadNPCInfo(char *script, int length, mgCMemory *memory) {
    CVillagerPlace::ProgressInfo progress_table[GAME_PROGRESS_MAX];

    niProgInfo = progress_table;
    niStack = memory;
    niVlgr = 0;
    niProgNum = 0;
    niNowProgInfo = NULL;
    niPlaceInfo = PlaceInfo;
    niPlaceInfoNum = PlaceInfoNum;
    CScriptInterpreter interpreter;
    interpreter.SetTag(ni_tag);
    interpreter.SetScript(script, length);
    interpreter.Run();
}

void LoadPlaceInfo(char *script, int length, mgCMemory *memory) {
    vpiStack = memory;
    vpiInfo = NULL;
    CScriptInterpreter interpreter;
    interpreter.SetTag(tag__9);
    interpreter.SetScript(script, length);
    interpreter.Run();
}

/**
 *
 * Allocates the villager place table specified by a script.
 *
 */
int vpiNPC_PLACE_NUM(SPI_STACK *stack, int argc) {
    int count = spiGetStackInt(stack);

    if (count <= 0) {
        return 0;
    }

    u32 blocks;

    if (((u32) count * sizeof(CVillagerPlaceInfo)) & 0xF) {
        blocks = (((u32) count * sizeof(CVillagerPlaceInfo)) >> 4) + 1;
    } else {
        blocks = ((u32) count * sizeof(CVillagerPlaceInfo)) >> 4;
    }

    void               *block = vpiStack->Alloc(blocks + 2);
    CVillagerPlaceInfo *places = new ((u_long128 *) block) CVillagerPlaceInfo[count];

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
 *
 * Selects a place entry and sets its map number.
 *
 */
int vpiNPC_PLACE(SPI_STACK *stack, int argc) {
    int place_no = spiGetStackInt(stack++);
    int id = spiGetStackInt(stack);
    vpiInfo = PlaceInfo + place_no;
    vpiInfo->map_no = id;
    vpiInfo->move_motion = 1;
    return 1;
}

/**
 *
 * Ends the current villager place entry.
 *
 */
int vpiNPC_PLACE_END(SPI_STACK *stack, int argc) {
    vpiInfo = NULL;
    return 1;
}

/**
 *
 * Sets the position and facing angle of the current place.
 *
 */
int vpiPLACE_POS(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    spiGetStackVector(vpiInfo->pos, stack);
    vpiInfo->pos[3] = spiGetStackFloat(stack += 3);
    return 1;
}

/**
 *
 * Adds a movement destination to the current villager place.
 *
 */
int vpiMOVE_TO(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    CVillagerPlaceInfo::Node *node = vpiInfo->Add(vpiStack);

    if (node == NULL) {
        return 0;
    }

    node->type = 1;
    spiGetStackVector(node->pos, stack);
    node->pos[3] = spiGetStackFloat(stack += 3);
    return 1;
}

/**
 *
 * Adds a timed wait and motion to the current villager place.
 *
 */
int vpiWAIT(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    CVillagerPlaceInfo::Node *node = vpiInfo->Add(vpiStack);

    if (node == NULL) {
        return 0;
    }

    node->type = 2;
    char *posture = spiGetStackString(stack++);
    int   motion_end = 0;

    if (posture != NULL) {
        if (strcmp(posture, "mtn") == 0) {
            motion_end = 1;
        }
    }

    node->wait.motion_end = motion_end;
    node->wait.time = spiGetStackInt(stack++);
    char *motion_name = NULL;

    if (argc >= 3) {
        motion_name = spiGetStackString(stack);
    }

    node->wait.motion = vpiGetMotionID(motion_name);
    return 1;
}

/**
 *
 * Sets the place motion when its configured name is recognized.
 *
 */
int vpiMOTION(SPI_STACK *stack, int argc) {
    char *name;

    if (vpiInfo == NULL) {
        return 0;
    }

    name = spiGetStackString(stack);

    if (name == NULL) {
        return 1;
    }

    if (strcmp(name, "sit") == 0) {
        vpiInfo->motion = 4;
    }

    return 1;
}

/**
 *
 * Sets the conversation position offset of the current place.
 *
 */
int vpiTALK_OFFSET(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    spiGetStackVector(vpiInfo->talk_offset, stack);
    return 1;
}

/**
 *
 * Sets the motion used while moving through the current place.
 *
 */
int vpiMOVE_MOTION(SPI_STACK *stack, int argc) {
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
 *
 * Sets the movement speed of the current place.
 *
 */
int vpiMOVE_SPEED(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    vpiInfo->move_speed = spiGetStackFloat(stack);
    return 1;
}

/**
 *
 * Sets whether the current place hides the villager shadow.
 *
 */
int vpiSHADOW(SPI_STACK *stack, int argc) {
    if (vpiInfo == NULL) {
        return 0;
    }

    vpiInfo->no_shadow = (((spiGetStackInt(stack) != 0) ^ 1) & 0xFF);
    return 1;
}

/**
 *
 * Maps a villager motion name to its motion number.
 *
 */
int vpiGetMotionID(char *name) {
    if (name == NULL || *name == '\0') {
        return -1;
    }

    if (strcmp(name, "sit") == 0) {
        return 4;
    }

    if (strcmp(name, "stand") == 0) {
        return 0;
    }

    if (strcmp(name, "special") == 0) {
        return 8;
    }

    if (strcmp(name, "walk") == 0) {
        return 1;
    }

    return (strcmp(name, "run") == 0) ? 2 : -1;
}

/**
 *
 * Stores chapter, section, order, and name for a progress entry.
 *
 */
int giPROG_INFO(SPI_STACK *stack, int argument_count) {
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

void LoadGameInfo(mgCMemory *memory) {
    int  script_size;
    char script[0x19000];

    if (LoadFile2("place.cfg", script, &script_size, 0) == 0) {
        return;
    }

    LoadPlaceInfo(script, script_size, memory);

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
    giStack = memory;
    ProgressNum = GAME_PROGRESS_MAX;
    CScriptInterpreter interpreter;
    interpreter.SetTag(gi_tag);
    interpreter.SetScript(script, script_size);
    interpreter.Run();
    LoadNPCInfo(script, script_size, memory);
    printf("rm %d\n", memory->stack_size - memory->stack_used);
}

GAME_PROGRESS_INFO *GetGameProgressInfo(int index) {
    if (index < 0 || index >= ProgressNum) {
        return NULL;
    }

    return &ProgressInfo[index];
}

int GetGameChapter(int index) {
    GAME_PROGRESS_INFO *info = GetGameProgressInfo(index);

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
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/vlgr_info", at_509__DATA);

// Small uninitialised data (.sbss)

// Uninitialised data (.bss)
/**
 * Villager placement schedules indexed by villager number.
 */
CVillagerPlace VlgrPlace[VLGR_PLACE_MAX];
