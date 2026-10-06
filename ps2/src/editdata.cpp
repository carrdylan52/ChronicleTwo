#include "common.h"
#include "editdata.hpp"
#include <cstring>

#include <cstdio>

#include "dataread.hpp"
#include "editparts.hpp"
#include "mg_memory.hpp"
#include "scriptinterpreter.hpp"

static EditAnalyzeSrc AnalyzeSrc[EDIT_ANALYZE_MAP_MAX]; /**< Georama analysis definitions for each map. */
static EditAnalyzeSrc *eaAnaSrc; /**< Map analysis definitions being parsed. */
static EditAnalyzeDataSrc *eaAnaData; /**< Analysis request being parsed. */
static mgCMemory *eaStack; /**< Storage for parsed analysis definitions. */

static void LoadEditAnalyzeData(char *script, int size, mgCMemory *stack);

static int eaGEO_ANALYZE(SPI_STACK *stack, int argc);

static int eaCONDITION(SPI_STACK *stack, int argc);

static int eaANALYZE(SPI_STACK *stack, int argc);

static int eaCON_NO(SPI_STACK *stack, int argc);

static int eaON_PARTS(SPI_STACK *stack, int argc);

static int eaOFF_PARTS(SPI_STACK *stack, int argc);

static int eaPERCENT(SPI_STACK *stack, int argc);

static int eaEND_ANALYZE(SPI_STACK *stack, int argc);

static int eaEND_GEO_ANALYZE(SPI_STACK *stack, int argc);

static SPI_TAG_PARAM tag[] = {
    { "GEO_ANALYZE", eaGEO_ANALYZE },
    { "CONDITION", eaCONDITION },
    { "ANALYZE", eaANALYZE },
    { "CON_NO", eaCON_NO },
    { "ON_PARTS", eaON_PARTS },
    { "OFF_PARTS", eaOFF_PARTS },
    { "PERCENT", eaPERCENT },
    { "END_ANALYZE", eaEND_ANALYZE },
    { "END_GEO_ANALYZE", eaEND_GEO_ANALYZE },
    { NULL, NULL },
};

// Code (.text)
void EditAnalyzeDataSrc::Init(void) {
    message = NULL;
    percent = 0;
    geo_floor = 0;
    con_no[0] = -1;
    con_no[1] = -1;
    con_no[2] = -1;
    con_no[3] = -1;
    con_no[4] = -1;
    con_no[5] = -1;
    con_no[6] = -1;
    con_no[7] = -1;
    unk_10 = -1;
    on_parts = NULL;
    off_parts = NULL;
}

void EditAnalyzeSrc::Init() {
    for (int i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        condition[i] = 0;
        geo_floor[i] = 0;
    }
    for (int i = 0; i < EDIT_ANALYZE_DATA_MAX; i++) {
        data[i].Init();
    }
}

/**
 * Returns a defined town request after checking the preceding entries.
 */
static EditAnalyzeDataSrc *GetAnalyzeDataSrc(int area, int entry) {
    if (area < 0 || area >= EDIT_ANALYZE_MAP_MAX) {
        return NULL;
    }
    if (entry < 0 || entry >= EDIT_ANALYZE_DATA_MAX) {
        return NULL;
    }
    for (int i = 0; i <= entry; i++) {
        if (AnalyzeSrc[area].data[i].message == NULL) {
            return NULL;
        }
    }
    return &AnalyzeSrc[area].data[entry];
}

void CEditData::Initialize(void) {
    memset(this, 0, sizeof(*this));
    InitPlaceData();
    memset(&analyze, 0, sizeof(analyze));
}

void CEditData::InitPlaceData(void) {
    int i;
    culture_point = 0;
    parts_max = EDIT_DATA_PARTS_MAX;
    for (i = 0; i < parts_max; i++) {
        memset(&parts[i], 0, sizeof(EditDataParts));
    }
    house_max = EDIT_DATA_HOUSE_MAX;
    for (i = 0; i < house_max; i++) {
        memset(&house[i], 0, sizeof(EditDataHouse));
    }
    for (i = 0; i < EDIT_DATA_PLACE_LOG_MAX; i++) {
        place_log[i].parts_no = -1;
    }
    for (i = 0; i < EDIT_DATA_GRID_SIZE; i++) {
        grid[i] = 0;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editdata", SaveData__8CEditMapFP9CEditData);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editdata", LoadData__8CEditMapFP9CEditData);
/**
 * Returns the culture contribution stored for an edit part.
 */
static int GetCulturePoint(CEditParts *parts, int) {
    if (parts == NULL || parts->info == NULL) {
        return 0;
    }
    return parts->info->cpoint[1];
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editdata", CultureAnalyzeParts__8CEditMapFii);

int CEditMap::CultureAnalyze(int cpoint_no) {
    int total;
    int pass = 0;
    do {
        total = 0;
        for (int i = 0; i < edit_parts_max; i++) {
            total += CultureAnalyzeParts(i, cpoint_no);
        }
        pass++;
    } while (pass <= 0);
    return total;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editdata", GetOnOffParts__8CEditMapFPcPP9CMapPartsPP9CMapPiecei);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editdata", PartsOnOff__8CEditMapFiP9CEditData);

int CEditData::GetPartsNumID(int id) {
    int number = 0;
    EditDataParts *entry = parts;
    for (int i = 0; i < parts_max; i++, entry++) {
        if (entry->id != 0 && id == entry->id && entry->state != 0) {
            number++;
        }
    }
    return number;
}

s8 CEditData::Analyze(int data_no, int map_no, int *con_src, int depth) {
    EditAnalyzeDataSrc *src;
    s8 result;
    int i;
    s8 flag_no;

    if (depth > EDIT_ANALYZE_DEPTH_MAX) {
        printf("infinty loop!!!\n");
        return 0;
    }
    src = GetAnalyzeDataSrc(map_no, data_no);
    if (src == NULL) {
        return 0;
    }
    result = 0;
    if (src->message == NULL) {
        return 0;
    }
    for (i = 0; i < EDIT_ANALYZE_CON_NO_MAX; i++) {
        flag_no = src->con_no[i];
        if (flag_no < 0) {
            break;
        }
        result = 1;
        if (con_src[flag_no] >= 0) {
            analyze.condition[flag_no] = Analyze(con_src[flag_no], map_no, con_src, depth + 1);
            con_src[flag_no] = -1;
        }
        if (analyze.condition[flag_no] == 0) {
            return 0;
        }
    }
    return result;
}

void CEditData::Analize(int map_no, int *con_value, int *con_src) {
    int i;
    int j;

    for (i = 0; i < EDIT_ANALYZE_CONDITION_MAX; i++) {
        if (con_src[i] < 0) {
            analyze.condition[i] = con_value[i];
        }
    }
    for (j = 0; j < EDIT_ANALYZE_CONDITION_MAX; j++) {
        if (con_src[j] >= 0) {
            analyze.condition[j] = Analyze(con_src[j], map_no, con_src, 0);
            con_src[j] = -1;
        }
    }
}

EditAnalyzeDataSrc *CEditData::GetAnalyzeData(int map_no, int data_no) {
    return GetAnalyzeDataSrc(map_no, data_no);
}

EditAnalyzeSrc *CEditData::GetAnalyzeSrc(int map_no) {
    if (map_no < 0 || map_no >= EDIT_ANALYZE_MAP_MAX) {
        return NULL;
    }
    return &AnalyzeSrc[map_no];
}

int CEditData::GetAnalyzePercent(int map_no) {
    int total;
    int entry_no;
    EditAnalyzeDataSrc *entry;

    total = 0;
    for (entry_no = 0; entry_no < EDIT_ANALYZE_DATA_MAX; entry_no++) {
        entry = GetAnalyzeDataSrc(map_no, entry_no);
        if (entry == NULL) {
            return total;
        }
        if ((entry->message != NULL) && (GetAnalyzeFlag(map_no, entry_no) != 0)) {
            total += entry->percent;
        }
    }
    return total;
}

int CEditData::GetAnalyzeFlag(int map_no, int data_no, int *con_no, int *con_flag) {
    EditAnalyzeDataSrc *src = GetAnalyzeDataSrc(map_no, data_no);
    int all_hold = 1;
    int i;
    if (src == NULL) {
        return 0;
    }
    for (i = 0; i < EDIT_ANALYZE_CON_NO_MAX; i++) {
        int condition_no = src->con_no[i];
        con_no[i] = condition_no;
        if (condition_no < 0) {
            if (i == 0) {
                return 0;
            }
            break;
        }
        con_flag[i] = analyze.condition[condition_no];
        if (con_flag[i] == 0) {
            all_hold = 0;
        }
    }
    return all_hold;
}

s32 CEditData::GetAnalyzeFlag(s32 map_no, s32 data_no) {
    s32 condition_numbers[EDIT_ANALYZE_CON_NO_MAX];
    s32 condition_flags[EDIT_ANALYZE_CON_NO_MAX];
    return GetAnalyzeFlag(map_no, data_no, condition_numbers, condition_flags);
}

void CEditData::dbgSetContintionFlag(int map_no, int con_no, int flag) {
    if (con_no < 0 || con_no >= EDIT_ANALYZE_CONDITION_MAX) {
        return;
    }
    analyze.condition[con_no] = flag;
}

void CEditData::dbgSetAnalyzeFlag(s32 map_no, s32 data_no, s32 flag) {
    EditAnalyzeDataSrc *request = GetAnalyzeData(map_no, data_no);
    if (request == NULL) {
        return;
    }
    for (s32 i = 0; i < EDIT_ANALYZE_CON_NO_MAX; i++) {
        s8 condition_no = request->con_no[i];
        if (condition_no < 0) {
            break;
        }
        dbgSetContintionFlag(map_no, condition_no, flag);
    }
}

void CEditData::dbgSetAllContintionFlag(int map_no, int flag) {
    int condition_no = 0;
    do {
        s8 *conditions = &analyze.condition[condition_no];
        conditions[0] = flag;
        conditions[1] = flag;
        conditions[2] = flag;
        conditions[3] = flag;
        conditions[4] = flag;
        conditions[5] = flag;
        conditions[6] = flag;
        conditions[7] = flag;
        condition_no += 8;
    } while (condition_no < EDIT_ANALYZE_CONDITION_MAX);
}

int CEditData::dbgGetContintionFlag(int map_no, int con_no, char *name) {
    if (con_no < 0 || con_no >= EDIT_ANALYZE_CONDITION_MAX) {
        return 0;
    }
    if (name != NULL) {
        name[0] = 0;
        if (map_no >= 0 && map_no < EDIT_ANALYZE_MAP_MAX + 1) {
            char *source = AnalyzeSrc[map_no].condition[con_no];
            if (source != NULL) {
                strcpy(name, source);
            }
        }
    }
    return analyze.condition[con_no];
}

void LoadEditAnalyzeData(int language, u_long128 *buffer) {
    char path[0x4C];
    int size;

    static mgCMemory Stack;
    static u_long128 stack_buffer[0x300];
    Stack.stSetBuffer(stack_buffer, 0x300);
    sprintf(path, "geo%d.cfg", language);
    if (LoadFile2(path, buffer, &size, 0) != 0) {
        LoadEditAnalyzeData((char *)buffer, size, &Stack);
    }
    printf("GeoData Remain = %dkbyte\n", ((Stack.stack_size - Stack.stack_used) * 16) / 1024);
}

/**
 * Parses the Georama analysis definitions from a script.
 */
static void LoadEditAnalyzeData(char *script, int size, mgCMemory *stack) {
    eaStack = stack;
    eaAnaSrc = NULL;
    CScriptInterpreter interpreter;
    interpreter.SetTag(tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

/**
 * Selects and clears the analysis source for a town.
 */
static int eaGEO_ANALYZE(SPI_STACK *stack, int) {
    eaAnaSrc = NULL;
    int id = spiGetStackInt(stack);
    if (id < 0 || id >= EDIT_ANALYZE_MAP_MAX) {
        return 0;
    }
    eaAnaSrc = &AnalyzeSrc[id];
    eaAnaSrc->Init();
    eaAnaData = NULL;
    return 1;
}

/**
 * Stores a condition name and its Georama floor.
 */
static int eaCONDITION(SPI_STACK *stack, int argc) {
    int con_no;
    if (eaAnaSrc == NULL) {
        return 0;
    }
    con_no = spiGetStackInt(stack++);
    if (con_no < 0 || con_no >= EDIT_ANALYZE_CONDITION_MAX) {
        return 0;
    }
    eaAnaSrc->condition[con_no] = mgCopyString(spiGetStackString(stack++), eaStack);
    if (argc >= 3) {
        eaAnaSrc->geo_floor[con_no] = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Selects a request and stores its message and floor.
 */
static int eaANALYZE(SPI_STACK *stack, int argc) {
    int entry_no;
    if (eaAnaSrc == NULL) {
        return 0;
    }
    entry_no = spiGetStackInt(stack++);
    if (entry_no < 0 || entry_no >= EDIT_ANALYZE_DATA_MAX) {
        return 0;
    }
    eaAnaData = &eaAnaSrc->data[entry_no];
    eaAnaData->unk_10 = spiGetStackInt(stack++);
    eaAnaData->message = mgCopyString(spiGetStackString(stack++), eaStack);
    eaAnaData->con_no[0] = -1;
    if (argc >= 4) {
        eaAnaData->geo_floor = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Assigns the condition numbers required by the current request.
 */
static int eaCON_NO(SPI_STACK *stack, int count) {
    if (eaAnaData == NULL) {
        return 0;
    }
    if (count >= EDIT_ANALYZE_CON_NO_MAX) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        eaAnaData->con_no[i] = spiGetStackInt(stack++);
    }
    eaAnaData->con_no[count] = -1;
    return 1;
}

/**
 * Stores the parts enabled when the current request is satisfied.
 */
static int eaON_PARTS(SPI_STACK *stack, int) {
    if (eaAnaData == NULL) {
        return 0;
    }
    eaAnaData->on_parts = mgCopyString(spiGetStackString(stack), eaStack);
    return 1;
}

/**
 * Stores the parts disabled when the current request is satisfied.
 */
static int eaOFF_PARTS(SPI_STACK *stack, int) {
    if (eaAnaData == NULL) {
        return 0;
    }
    eaAnaData->off_parts = mgCopyString(spiGetStackString(stack), eaStack);
    return 1;
}

/**
 * Stores the progress percentage of the current request.
 */
static int eaPERCENT(SPI_STACK *stack, int) {
    if (eaAnaData == NULL) {
        return 0;
    }
    eaAnaData->percent = spiGetStackInt(stack);
    return 1;
}

/**
 * Finishes the current request.
 */
static int eaEND_ANALYZE(SPI_STACK *, int) {
    eaAnaData = NULL;
    return 1;
}

/**
 * Finishes the current town analysis source.
 */
static int eaEND_GEO_ANALYZE(SPI_STACK *, int) {
    eaAnaSrc = NULL;
    return 1;
}

s32 GetMaxPolyn(s32 map_no) {
    s32 max_polygons = 0xFA0;
    if (map_no != 4) {
        max_polygons = 0x1770;
        switch (map_no) {
        case 0:
            return 0xFA0;
        case 1:
            return 0x1770;
        case 2:
            return 0x1770;
        case 3:
            return max_polygons;
        default:
            return 0;
        }
    } else {
        return max_polygons;
    }
}

s32 GetMaxDrawMem(s32 map_no) {
    s32 max_draw_memory = 0xBB80;
    if (map_no != 4) {
        max_draw_memory = 0xD2F0;
        switch (map_no) {
        case 0:
            return 0xBB80;
        case 1:
            return 0xC350;
        case 2:
            return 0xD2F0;
        case 3:
            return max_draw_memory;
        default:
            return 0;
        }
    } else {
        return max_draw_memory;
    }
}

// Static initialiser (.init)

// Initialised data (.data)

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editdata", at_713__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editdata", at_714__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editdata", at_917__3__DATA);

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)
INCLUDE_BSS(init_1273, 0x4);
INCLUDE_BSS(eaAnaSrc, 0x4);
INCLUDE_BSS(eaAnaData, 0x4);
INCLUDE_BSS(eaStack, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(AnalyzeSrc, 0x1040);
INCLUDE_BSS(buff_1271, 0x3000);
INCLUDE_BSS(Stack_1272, 0x30);
