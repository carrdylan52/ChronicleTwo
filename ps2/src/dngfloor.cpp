#include "common.h"
#include "dngfloor.hpp"
#include "dataread.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "menumain.hpp"
#include "mg_memory.hpp"
#include "savedata.hpp"
#include "savedatadungeon.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "userdata.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static int _TREE_MAPINFO(SPI_STACK *stack, int argc);
static int _GLID_INFO(SPI_STACK *stack, int argc);
static int _ROOT_INFO(SPI_STACK *stack, int argc);
static int _ROOM_INFO(SPI_STACK *stack, int argc);
static int _ROOM_LINK(SPI_STACK *stack, int count);
static int _ROOM_OPTION(SPI_STACK *stack, int argc);
static int _ROOM_KEYROOM(SPI_STACK *stack, int argc);
static int _ROOM_TEXNO(SPI_STACK *stack, int argc);
static int _ROOM_FLOOR_INFO(SPI_STACK *stack, int argc);
static int _ROOM_FLOOR_INFO2(SPI_STACK *stack, int argc);
static int _ROOM_TITLE(SPI_STACK *stack, int argc);

static DNGMAP_ROOM_INFO *tree_spi_roominfo;
static DNGMAP_ROOT_INFO *tree_spi_rootinfo;
static CDngFloorManager *tree_dngmap;
static GLID_INFO *tree_glid_info;
static s16 menu_dng_debug_glidcnt;
static mgCMemory *tree_spi_stack;
static s8 diff_conditiontable[2][7] = {
    {1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 1, 0}
};
static u16 check_bittable[3][6] = {
    {2, 4, 8, 16, 32, 64},
    {1, 2, 4, 8, 16, 64},
    {32, 2, 4, 8, 16, 1}
};
static u16 cbit[4][5] = {
    {4, 8, 16, 32, 64},
    {2, 8, 16, 32, 64},
    {2, 4, 16, 32, 64},
    {2, 4, 8, 32, 64}
};
static int texture_group_start[6] = {0, 0, 4, 8, 12, 16};
static SPI_TAG_PARAM tree_map_tag[12] = {
    {"TREE_INFO", _TREE_MAPINFO},
    {"GI", _GLID_INFO},
    {"RT", _ROOT_INFO},
    {"RI", _ROOM_INFO},
    {"RI_LINK", _ROOM_LINK},
    {"RI_OP", _ROOM_OPTION},
    {"RI_TEX", _ROOM_TEXNO},
    {"RI_KEYROOM", _ROOM_KEYROOM},
    {"RF_INFO", _ROOM_FLOOR_INFO},
    {"RF_INFO2", _ROOM_FLOOR_INFO2},
    {"RI_TITLE", _ROOM_TITLE},
    {NULL, NULL}
};

// Code (.text)
void CDngFloorManager::Initialize(void) {
    dng_no = 0;
    glid_info = NULL;
    glid_num = 0;
    glid_w = 0;
    glid_h = 0;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _TREE_MAPINFO(SPI_STACK *stack, int argc) {
    int width;
    width = spiGetStackInt(stack++);
    int height = spiGetStackInt(stack++);
    int room_count = spiGetStackInt(stack);
    u32 bytes = room_count * sizeof(GLID_INFO);
    tree_dngmap->glid_w = width;
    tree_dngmap->glid_h = height;
    u32 blocks;
    if ((bytes & 0xF) != 0) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    tree_dngmap->glid_info =
        (GLID_INFO *)operator new[](bytes, (u_long128 *)(tree_spi_stack)->Alloc(blocks + 2));
    tree_dngmap->glid_num = room_count;
    tree_glid_info = tree_dngmap->glid_info;
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _GLID_INFO(SPI_STACK *stack, int argc) {
    tree_glid_info->type = spiGetStackInt(stack++);
    tree_glid_info->x = spiGetStackInt(stack++);
    tree_glid_info->y = spiGetStackInt(stack++);
    tree_glid_info->unk_6 = spiGetStackInt(stack++);
    tree_glid_info->unk_8 = spiGetStackInt(stack);
    tree_glid_info->link_glid[0] = 0;
    tree_glid_info->link_glid[1] = 0;
    tree_glid_info->link_glid[2] = 0;
    tree_glid_info->link_glid[3] = 0;
    tree_glid_info->blink = 0;
    DNGMAP_ROOM_INFO *info = &tree_glid_info->room;
    tree_spi_rootinfo = &tree_glid_info->root;
    tree_spi_roominfo = info;
    memset(info, 0, sizeof(DNGMAP_ROOM_INFO));
    memset(tree_spi_rootinfo, 0, sizeof(DNGMAP_ROOT_INFO));
    tree_glid_info += 1;
    menu_dng_debug_glidcnt += 1;
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOT_INFO(SPI_STACK *stack, int argc) {
    tree_spi_rootinfo->type = spiGetStackInt(stack++);
    tree_spi_rootinfo->shape = spiGetStackInt(stack++);
    tree_spi_rootinfo->show_mark = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_INFO(SPI_STACK *stack, int argc) {
    tree_spi_roominfo->floor_id = spiGetStackInt(stack++);
    tree_spi_roominfo->order = spiGetStackInt(stack++);
    spiGetStackInt(stack++);
    tree_spi_roominfo->unk_0 = NULL;
    if (argc >= 4) {
        tree_spi_roominfo->unk_0 = mgCopyString(spiGetStackString(stack), tree_spi_stack);
    }
    tree_spi_roominfo->unk_44 = 0;
    tree_spi_roominfo->visited = 0;
    tree_spi_roominfo->flag = DNGMAP_ROOM_FLAG_ROOM;
    tree_spi_roominfo->offset_y = 0;
    tree_spi_roominfo->offset_x = 0;
    tree_spi_roominfo->practice_type = -1;
    tree_spi_roominfo->mark_phase = 0;
    tree_spi_roominfo->unk_4c = 0;
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_LINK(SPI_STACK *stack, int count) {
    for (int i = 0; i < count; i++) {
        tree_spi_roominfo->link[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_OPTION(SPI_STACK *stack, int argc) {
    int flags;
    MENU_SPI_ANALYZE_STRUCT1 options[5] = {
        {"start", DNGMAP_ROOM_FLAG_START}, {"exit", DNGMAP_ROOM_FLAG_EXIT},
        {"boss", DNGMAP_ROOM_FLAG_BOSS}, {"sub", DNGMAP_ROOM_FLAG_SUB}, {NULL, 0}
    };
    flags = DNGMAP_ROOM_FLAG_ROOM;
    for (int i = 0; i < argc; i++) {
        flags |= menu_spi_analyze_func_strcut1(options, spiGetStackString(stack++));
    }
    tree_spi_roominfo->flag |= flags;
    if ((tree_spi_roominfo->flag & DNGMAP_ROOM_FLAG_SUB) || (tree_spi_roominfo->flag & DNGMAP_ROOM_FLAG_BOSS)) {
        tree_spi_roominfo->offset_x = 0;
        tree_spi_roominfo->offset_y = -0x1A;
        if (argc > 1) {
            tree_spi_roominfo->offset_x = spiGetStackInt(stack++);
            tree_spi_roominfo->offset_y = spiGetStackInt(stack);
        }
    }
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_KEYROOM(SPI_STACK *stack, int argc) {
    tree_spi_roominfo->key_room[0] = spiGetStackInt(stack++);
    tree_spi_roominfo->key_room[1] = spiGetStackInt(stack++);
    tree_spi_roominfo->key_room[2] = spiGetStackInt(stack++);
    tree_spi_roominfo->key_room[3] = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_TEXNO(SPI_STACK *stack, int argc) {
    int texture_no;
    texture_no = spiGetStackInt(stack);
    if (texture_no < 0) {
        int texture_group = abs(texture_no);
        texture_no = GetRandI(4);
        texture_no += texture_group_start[texture_group];
    }
    tree_spi_roominfo->tex_no = texture_no;
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_FLOOR_INFO(SPI_STACK *stack, int argc) {
    DNGMAP_ROOM_INFO *info = tree_dngmap->GetDngMapFloorInfo(spiGetStackInt(stack++));
    if (info == 0) {
        return 0;
    }
    info->geostone = spiGetStackInt(stack++);
    info->fast_destroy_time = spiGetStackInt(stack++);
    info->fishing = spiGetStackInt(stack++);
    info->fishing_record = spiGetStackInt(stack++);
    info->seal = spiGetStackInt(stack++);
    info->spheda = spiGetStackInt(stack++);
    for (int i = 0; i < 3; i++) {
        info->spheda_prize_item[i] = spiGetStackInt(stack++);
        info->spheda_prize_num[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_FLOOR_INFO2(SPI_STACK *stack, int argc) {
    DNGMAP_ROOM_INFO *info = tree_dngmap->GetDngMapFloorInfo(spiGetStackInt(stack++));
    if (info == 0) {
        return 0;
    }
    info->practice_type = spiGetStackInt(stack++);
    info->practice_param = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the dungeon grid or floor settings from script arguments.
 */
static int _ROOM_TITLE(SPI_STACK *stack, int argc) {
    DNGMAP_ROOM_INFO *info = tree_dngmap->GetDngMapFloorInfo(spiGetStackInt(stack++));
    if (info == 0) {
        return 0;
    }
    char *name = spiGetStackString(stack);

    char empty[4] = "err";
    if (name == 0) {
        name = empty;
    }
    info->title = mgCopyString(name, tree_spi_stack);
    return 1;
}

void CDngFloorManager::AnalyzeFile(char *script, int size, mgCMemory *stack) {
    tree_spi_stack = stack;
    tree_dngmap = this;
    stack->Align64();
    menu_dng_debug_glidcnt = 0;
    CScriptInterpreter interpreter;
    interpreter.SetTag(tree_map_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
    tree_spi_stack->Align64();
    RelationGlid();
}

void CDngFloorManager::LoadDataTable(int dng_no, mgCMemory *stack) {
    char path[0x60];
    u8   scratch[0xA000];
    char menu_path[0x40];
    int  size;

    if (stack == NULL) {
        return;
    }
    if (dng_no < 0 || dng_no >= DNGMAP_DUNGEON_MAX + 1) {
        dng_no = 0;
    }
    Initialize();
    this->dng_no = dng_no;
    sprintf(path, "menu/dngmap/dmap%d.cfg", dng_no);
    u8 *data = (u8 *)MenuCalcBufAlignment((u_long128 *)scratch);
    LoadFile2(path, data, &size, LOAD_FILE_READ);
    if (size > 0) {
        AnalyzeFile((char *)data, size, stack);
    }
    sprintf(path, "menu/dngmap/dflr%d.cfg", dng_no);
    data = (u8 *)MenuCalcBufAlignment((u_long128 *)scratch);
    LoadFile2(path, data, &size, LOAD_FILE_READ);
    if (size > 0) {
        AnalyzeFile((char *)data, size, stack);
    }
    sprintf(menu_path, "flrtitle%d.txt", dng_no);
    size = LoadFileMenu(menu_path, (u_long128 *)data, 1);
    if (size > 0) {
        AnalyzeFile((char *)data, size, stack);
    }
}

GLID_INFO *CDngFloorManager::GetDngMapFloorGlidInfo(int floor_id) {
    int i;

    if (this->glid_info == NULL || this->glid_num <= 0) {
        return NULL;
    }
    for (i = 0; i < this->glid_num; i++) {
        if (this->glid_info[i].type == GLID_TYPE_ROOM && floor_id == this->glid_info[i].room.floor_id) {
            return &this->glid_info[i];
        }
    }
    return NULL;
}

s8 CDngFloorManager::IsGeoStone(int floor_id) {
    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor_id);
    if (info != NULL) {
        return info->geostone;
    }
    return 0;
}

int CDngFloorManager::GetSphedaPrize(int floor_id, int rank, int *item, int *num) {
    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor_id);
    if (info == NULL) {
        return 0;
    }
    if (rank < 0) {
        return 0;
    }
    if (rank >= 3) {
        rank = 2;
    }
    if (item != NULL) {
        *item = info->spheda_prize_item[rank];
    }
    if (num != NULL) {
        *num = info->spheda_prize_num[rank];
    }
    return 1;
}

int CDngFloorManager::GetSphedaPrize(int rank, int *item, int *num) {
    CSaveDataDungeon *dungeon = menu_GetSaveDataDungeon();
    if (dungeon == NULL) {
        return 0;
    }
    return GetSphedaPrize(dungeon->floor_id[dungeon->stage_id], rank, item, num);
}

int CDngFloorManager::IsPlaySubGame() {
    int games;
    games = 0;
    DNGMAP_ROOM_INFO *info = GetActiveFloorInfo();
    if (info == NULL) {
        return 0;
    }
    if (info->fishing != 0) {
        games |= DNGMAP_SUB_GAME_FISHING;
    }
    if (info->spheda != 0) {
        games |= DNGMAP_SUB_GAME_SPHEDA;
    }
    return games;
}

s8 CDngFloorManager::IsSealFloor(int floor_id) {
    int seal;
    CSaveDataDungeon *dungeon = menu_GetSaveDataDungeon();
    if (dungeon == NULL) {
        return 0;
    }
    if (floor_id < 0) {
        floor_id = dungeon->floor_id[dungeon->stage_id];
    }
    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor_id);
    if (info == NULL) {
        return 0;
    }
    DNG_FLOOR_SAVE *saved = dungeon->GetFloorInfoPtr(dungeon->stage_id, floor_id);
    seal = info->seal;
    if (saved != NULL && (saved->flag & DNG_FLOOR_FLAG_SEAL_CLEAR)) {
        seal = 0;
    }

    CUserDataManager *user = GetUserDataMan();
    if (user != NULL) {
        int members = user->GetNowPartyMember();
        if (seal == 1 && !(members & 2)) {
            seal = 0;
        }
        if (seal == 2 && !(members & 1)) {
            seal = 0;
        }
    }
    return seal;
}

int CDngFloorManager::IsClearMostFastDestroy() {
    int result;
    int elapsed;
    DNG_BATTLE_AREA *scene = (DNG_BATTLE_AREA *)menu_GetBattleAreaScene();
    CSaveData *save = GetSaveData();
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL || scene == NULL) {
        return 0;
    }
    int floor = dungeon->floor_id[dungeon->stage_id];
    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor);
    DNG_FLOOR_SAVE *saved = dungeon->GetFloorInfoPtr(dungeon->stage_id, floor);
    if (info == NULL || saved == NULL) {
        return 0;
    }

    elapsed = ((int)save->play_time - (int)scene->subject_counter) * 6 / 5;
    result = 0;
    if (saved->fast_destroy_time == 0) {
        if (elapsed < info->fast_destroy_time) {
            saved->fast_destroy_time = elapsed;
            result = 1;
            GetUserDataMan()->AddYarikomiMedal(result);
            saved->flag |= DNG_FLOOR_FLAG_FAST_DESTROY_CLEAR;
        }
    } else if (elapsed < saved->fast_destroy_time) {
        saved->fast_destroy_time = elapsed;
        result = 2;
    }
    return result;
}

int CDngFloorManager::IsClearPractice(int check_type) {
    int floor;
    CSaveDataDungeon *dungeon = menu_GetSaveDataDungeon();
    DNG_BATTLE_AREA *scene = (DNG_BATTLE_AREA *)menu_GetBattleAreaScene();
    floor = dungeon->floor_id[dungeon->stage_id];
    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor);
    DNG_FLOOR_SAVE *saved = dungeon->GetFloorInfoPtr(dungeon->stage_id, floor);
    if (info == NULL || saved == NULL || scene == NULL) {
        return 0;
    }

    int result;
    int mask;
    int found;
    int practice_type;
    int active;
    int r;
    int i;
    int k;
    int l;
    int m;

    practice_type = info->practice_type;
    if (practice_type < 0) {
        return 0;
    }
    active = scene->unk_5c;
    result = 0;
    if (diff_conditiontable[check_type][practice_type] == 0) {
        return 0;
    }

    mask = scene->unk_98;


    int j = 0;
    while (++j < 7) {
    }

    switch (practice_type) {
        case 0:
            if (active != 0) {
                if (scene->timer < info->practice_param) {
                    result = 2;
                }
            }
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            if (active != 0) {
                found = 0;
                if (practice_type == 1) {
                    for (i = 0; i < 6; i++) {
                        if (mask & check_bittable[0][i]) {
                            found = 1;
                        }
                    }
                }
                if (practice_type == 3) {
                    for (k = 0; k < 6; k++) {
                        if (mask & check_bittable[1][k]) {
                            found = 1;
                        }
                    }
                }
                if (practice_type == 4) {
                    for (l = 0; l < 6; l++) {
                        if (mask & check_bittable[2][l]) {
                            found = 1;
                        }
                    }
                }
                if (practice_type == 2) {
                    if ((mask & 0x1) || (mask & 0x20) || (mask & 0x40)) {
                        found = 1;
                    } else {

                        r = info->practice_param - 1;
                        for (m = 0; m < 5; m++) {
                            if (mask & cbit[r][m]) {
                                found = 1;
                            }
                        }
                    }
                }
                if ((mask & (1 << info->practice_param)) && found == 0) {
                    result = 2;
                }
            }
            break;
        case 5:
            result = 2;
            if (mask & 0x80) {
                result = 1;
            }
            break;
        case 6:
            break;
    }

    if (result == 2) {
        if (saved->flag & DNG_FLOOR_FLAG_PRACTICE_CLEAR) {
            result = 3;
        }
    }
    if (result == 2 || result == 3) {
        saved->flag |= DNG_FLOOR_FLAG_PRACTICE_CLEAR;
    }
    if (result == 2) {
        GetUserDataMan()->AddYarikomiMedal(1);
    }
    return result;
}

DNGMAP_ROOM_INFO *CDngFloorManager::GetDngMapFloorInfo(int floor_id) {
    if (this->glid_info == NULL) {
        return NULL;
    }

    GLID_INFO *glid = GetDngMapFloorGlidInfo(floor_id);
    if (glid != NULL) {
        return &glid->room;
    }
    return NULL;
}

DNGMAP_ROOM_INFO *CDngFloorManager::GetActiveFloorInfo() {
    CSaveDataDungeon *save = menu_GetSaveDataDungeon();
    if (save == NULL) {
        return NULL;
    }
    return GetDngMapFloorInfo(save->floor_id[save->stage_id]);
}

void CDngFloorManager::RelationGlid() {
    int       i;
    int       j;
    GLID_INFO *room;
    GLID_INFO *other;

    for (i = 0; i < glid_num; i++) {
        room = &glid_info[i];
        for (j = 0; j < glid_num; j++) {
            other = &glid_info[j];
            if (other != room) {
                if (room->x == other->x) {
                    if (other->y == room->y - 1) {
                        room->link_glid[0] = other;
                    }
                    if (other->y == room->y + 1) {
                        room->link_glid[1] = other;
                        break;
                    }
                }
                if (room->y == other->y) {
                    if (other->x == room->x - 1) {
                        room->link_glid[2] = other;
                    }
                    if (other->x == room->x + 1) {
                        room->link_glid[3] = other;
                    }
                }
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dngfloor", CheckDrawGlidInfo__16CDngFloorManagerFv);
GLID_INFO *CDngFloorManager::GetNextGlid(GLID_INFO *glid, int *dir) {
    static int search_other[4][3] = {
        {0, 2, 3},
        {1, 2, 3},
        {2, 0, 1},
        {3, 0, 1}
    };

    static int search_dungeon3[4][3] = {
        {0, 2, 3},
        {1, 3, 2},
        {2, 0, 1},
        {3, 0, 1}
    };

    static int search_dungeon2[4][3] = {
        {0, 2, 3},
        {1, 3, 2},
        {2, 1, 0},
        {3, 0, 1}
    };

    int       k;
    int       *row;
    int       current;
    GLID_INFO *result;
    GLID_INFO *room = glid;

    if (glid == NULL) {
        return 0;
    }
    current = *dir;
    if (dng_no == 2) {
        row = search_dungeon2[current];
    } else if (dng_no == 3) {
        row = search_dungeon3[current];
    } else {
        row = search_other[current];
    }
    for (k = 0; k < 3; k++) {
        if (room->link_glid[row[k]] != NULL) {
            *dir = row[k];
            break;
        }
    }
    current = *dir;
    result = NULL;
    if (0 <= current) {
        result = room->link_glid[current];
    }
    return result;
}

GLID_INFO *CDngFloorManager::GetNextRoom(int floor_id, int dir, GLID_INFO *glid, int unused,
                                         int *found_dir) {
    GLID_INFO        *room;
    int              *row;
    int              i;
    int              k;
    s16              next;
    DNGMAP_ROOM_INFO *info;

    room = GetDngMapFloorGlidInfo(floor_id);
    if (room == NULL) {
        return NULL;
    }
    if (room->type != GLID_TYPE_ROOM) {
        return NULL;
    }
    int dirs[4][3] = {{0, 2, 3}, {1, 3, 2}, {2, 0, 1}, {3, 1, 1}};
    row = dirs[dir];
    info = &room->room;

    i = 0;
    while (++i < 4) {
    }
    for (k = 0; k < 2; k++) {
        next = info->link[row[k]];
        if (0 <= next) {
            room = GetDngMapFloorGlidInfo(next);
            if (found_dir != NULL) {
                *found_dir = row[k];
            }
            break;
        }
    }
    return room;
}

GLID_INFO *CDngFloorManager::GetKeyNextRoom(int floor_id, int dir, GLID_INFO *glid) {
    GLID_INFO *room = GetDngMapFloorGlidInfo(floor_id);
    if (room == NULL) {
        return NULL;
    }
    if (room->type != GLID_TYPE_ROOM) {
        return NULL;
    }
    return GetDngMapFloorGlidInfo(room->room.key_room[dir]);
}

int CDngFloorManager::GetDngMapNextFloorID(int floor_id, int root_type) {
    GLID_INFO        *room;
    GLID_INFO        *other;
    GLID_INFO        *next;
    DNGMAP_ROOM_INFO *info;
    int              d;
    DNGMAP_ROOM_INFO *other_info;
    int              index;

    room = GetDngMapFloorGlidInfo(floor_id);
    if (room == NULL) {
        return 0;
    }
    if (dng_no == 2) {
        if (floor_id == 8) {
            return 8;
        }
    }
    info = &room->room;
    for (d = 0; d < 4; d++) {
        if (info->link[d] >= 0) {
            other = GetDngMapFloorGlidInfo(info->link[d]);
            if (other != NULL) {
                other_info = &other->room;
                if (other_info != NULL && other_info->order > info->order) {
                    index = d;
                    next = GetNextGlid(room, &index);
                    if (next != NULL && next->type == GLID_TYPE_ROOT && next->root.type == root_type) {
                        return other_info->floor_id;
                    }
                }
            }
        }
    }
    return 0;
}

char *CDngFloorManager::GetFloorTitle(int floor_id) {
    static char *fl_t[2] = {"\x95\x7c\x82\xa2\x90\x58", "Wonder Forest"};

    DNGMAP_ROOM_INFO *info = GetDngMapFloorInfo(floor_id);
    if (dng_no == 1 && floor_id == DNGMAP_FLOOR_SPECIAL) {
        int language = LanguageCode;
        if (language > 1) {
            language = 1;
        }
        return fl_t[language];
    }
    if (info == NULL) {
        return 0;
    }
    return info->title;
}

int CDngFloorManager::GetDngMapNextRoot(int floor_id) {
    GLID_INFO        *room;
    GLID_INFO        *other;
    GLID_INFO        *next;
    int              mask;
    DNGMAP_ROOM_INFO *info;
    DNGMAP_ROOM_INFO *other_info;
    int              d;
    int              index;

    if (floor_id == DNGMAP_FLOOR_SPECIAL) {
        return 1;
    }
    room = GetDngMapFloorGlidInfo(floor_id);
    mask = 0;
    if (room == NULL) {
        return 0;
    }
    info = &room->room;
    for (d = 0; d < 4; d++) {
        if (info->link[d] >= 0) {
            other = GetDngMapFloorGlidInfo(info->link[d]);
            if (other != NULL) {
                other_info = &other->room;
                if (other_info != NULL && other_info->order > info->order) {
                    index = d;
                    next = GetNextGlid(room, &index);
                    if (next != NULL && next->type == GLID_TYPE_ROOT) {
                        mask |= 1 << next->root.type;
                    }
                }
            }
        }
    }
    return mask;
}

int GetCountSphedaClear(void) {
    CSaveDataDungeon *save;
    int              count;
    int              dungeon;
    int              floor;
    DNG_FLOOR_SAVE   *info;

    save = menu_GetSaveDataDungeon();
    count = 0;
    if (save == NULL) {
        return 0;
    }
    for (dungeon = 0; dungeon < SAVE_DUNGEON_NUM; dungeon++) {
        for (floor = 0; floor < 40; floor++) {
            info = save->GetFloorInfoPtr(dungeon, floor);
            if (info == NULL) {
                break;
            }
            if (info->spheda_clear > 0) {
                count += 1;
            }
        }
    }
    return count;
}

int CheckFishingRecord(float size) {
    CSaveDataDungeon *save;
    CDngFloorManager *floors;
    DNG_FLOOR_SAVE   *save_info;
    DNGMAP_ROOM_INFO *info;
    int              centimeters;
    int              met;
    s8               kind;
    u16              flags;

    save = menu_GetSaveDataDungeon();
    if (save == NULL) {
        return 0;
    }
    floors = &((DNG_BATTLE_AREA *)menu_GetBattleAreaScene())->floor_manager;
    save_info = save->GetFloorInfoPtr(save->stage_id, save->floor_id[save->stage_id]);
    info = floors->GetDngMapFloorInfo(save->floor_id[save->stage_id]);
    if (save_info == NULL || info == NULL) {
        return 0;
    }
    kind = info->fishing;
    if (kind == 0) {
        return 0;
    }
    centimeters = (int)(100.0f * size);
    met = 0;
    if (kind < 0) {
        if (centimeters <= info->fishing_record) {
            met = 1;
        }
    }
    if (0 < kind) {
        if (info->fishing_record <= centimeters) {
            met = 1;
        }
    }
    if (met != 0) {
        flags = save_info->flag;
        if (!(flags & DNG_FLOOR_FLAG_FISHING_CLEAR)) {
            save_info->flag = flags | DNG_FLOOR_FLAG_FISHING_CLEAR;
            GetUserDataMan()->AddYarikomiMedal(1);
            return 1;
        }
    }
    return 0;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_886__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", D_0036178C__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", offsetTable_911__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", tree_map_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", diff_conditiontable_1102__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", check_bittable_1123__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", cbit_1158__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_1259__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", search_tbl_1366__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", search_tbl_1370__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", search_tbl_1372__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_1395__4__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_882__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_883__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_884__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_885__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_942__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_943__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_944__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_945__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_946__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_947__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_948__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_949__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_950__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_951__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_952__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_976__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_977__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_978__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_1200__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_1468__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_1469__5__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", at_938__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dngfloor", fl_t_1467__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(tree_dngmap, 0x4);
INCLUDE_BSS(tree_glid_info, 0x4);
INCLUDE_BSS(tree_spi_stack, 0x4);
INCLUDE_BSS(tree_spi_rootinfo, 0x4);
INCLUDE_BSS(tree_spi_roominfo, 0x4);
INCLUDE_BSS(menu_dng_debug_glidcnt, 0x4);
