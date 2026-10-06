#include "common.h"
#include "mapselect.hpp"

#include "character.hpp"
#include "dataread.hpp"
#include "editdata.hpp"
#include "font.hpp"
#include "gamepad.hpp"
#include "mainloop.hpp"
#include "mg_frame.hpp"
#include "mg_memory.hpp"
#include "savedata.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "vlgr_info.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int MapNameNum;
static MAP_NAME_INFO *map_name;
static int pMapNameBuff;
static int pCharBuff;
static char *CharBuff;
static int now_no;
static u_long128 MapNameBuff[MAP_NAME_BUFF_SIZE];
static mgCMemory *MenuStack;
static int SelectMode;
static int SelectMapType;
static int SedSel;
static int EventInfoNum;
static int BossEventTop;
static int sel_event;
static int top_event;
static char **SelectMapList[MAP_SEL_TYPE_NUM];
static int SelectMapNum[MAP_SEL_TYPE_NUM];
EVENT_VIEW_INFO *EventInfo;
int BossBattleSelFlag;
/** Map categories shown by the debug selector. */
static char *map_sel_type[MAP_SEL_TYPE_NUM] = {
    "New", "Georama", "PalmBlinks", "Submap", "Future", "Dungeon", "Event", "Special"
};
/** Map name chosen by the debug selector. */
static char SelectMapName[0x100] = "";
/** Values edited by the save-data debug menu. */
static int SedSelData[SED_ITEM_NUM] = {0};
/** Configuration choices shown by the save-data debug menu. */
static char *config_str[1] = {"Caption off"};
static char *GetLine(char **columns, char *position, char *end);

// Code (.text)
/**
 * Initializes the map table and its string buffer from the script count.
 */
static int mlMAP_NAME_NUM(SPI_STACK *arguments, int argument_count) {
    pMapNameBuff = 0;
    pCharBuff = 0;
    int count = spiGetStackInt(arguments);
    MapNameNum = count;
    map_name = (MAP_NAME_INFO *)&MapNameBuff[pMapNameBuff];
    pMapNameBuff += ((count + 1) * sizeof(MAP_NAME_INFO)) / 16 + 1;
    CharBuff = (char *)&MapNameBuff[pMapNameBuff];
    for (int index = 0; index < count + 1; ++index) memset(&map_name[index], 0, sizeof(MAP_NAME_INFO));
    now_no = 0;
    return 1;
}

/**
 * Copies one map entry from script arguments into the map table.
 */
static int mlMAP_NAME(SPI_STACK *arguments, int argument_count) {
    char *strings[3];
    char *copied[3];
    int index;
    int length;

    strings[0] = spiGetStackString(arguments++);
    strings[1] = spiGetStackString(arguments++);
    strings[2] = spiGetStackString(arguments++);
    for (index = 0; index < 3; ++index) {
        if (strings[index] == NULL || strings[index][0] == 0) {
            copied[index] = NULL;
        } else {
            length = strlen(strings[index]);
            copied[index] = &CharBuff[pCharBuff];
            strcpy(copied[index], strings[index]);
            pCharBuff += length + 1;
        }
    }
    MAP_NAME_INFO &entry = map_name[now_no++];
    entry.name = copied[0];
    entry.title = copied[1];
    entry.add_path = copied[2];
    entry.type = spiGetStackInt(arguments++);
    entry.sel_type = spiGetStackInt(arguments++);
    entry.snd_data_id = -1;
    if (argument_count >= 6) {
        entry.snd_data_id = spiGetStackInt(arguments++);
    }
    if (argument_count >= 7) {
        entry.area_no = spiGetStackInt(arguments);
    }
    return 1;
}

void LoadMapName(int language, u_long128 *buffer) {
    static SPI_TAG_PARAM tag[] = {
        {"MAP_NAME_NUM", mlMAP_NAME_NUM},
        {"MAP_NAME", mlMAP_NAME},
        {NULL, NULL}
    };

    MapNameNum = 0;
    char path[0x80];
    sprintf(path, "map/map%d.cfg", language);
    int file_size;
    if (LoadFile2(path, buffer, &file_size, 0)) {
        CScriptInterpreter interpreter;
        interpreter.SetTag(tag);
        interpreter.SetScript((char *)buffer, file_size);
        interpreter.Run();
        pMapNameBuff += pCharBuff / 16 + 1;
    }
}

/**
 * Returns a map table entry only for a valid map number.
 */
static MAP_NAME_INFO *GetMapNameInfo(int map_no) {
    if (map_no < 0 || map_no >= MapNameNum) return NULL;
    return &map_name[map_no];
}

char *GetMapPath(char *path, char *name) {
    int length = strlen(name);
    char *rest = name;

    strcpy(path, "map/");
    strncat(path, name, 1);
    strcat(path, "/");
    if (length >= 3) {
        strncat(path, name, 3);
        rest = &name[3];
        strcat(path, "/");
    }
    if (length >= 6) {
        strncat(path, rest, 3);
        strcat(path, "/");
    }
    return strcat(path, name);
}

int GetMapType(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->type : -1;
}

int GetMapAreaNo(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->area_no : -1;
}

int GetMapSelType(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->sel_type : 0;
}

int GetMapSndDataID(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->snd_data_id : -1;
}

char *GetMapName(int map_no, char **title) {
    if (title != NULL) *title = NULL;
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    if (entry == NULL) return SelectMapName;
    if (title != NULL) *title = entry->title;
    return entry->name;
}

int SearchMapNo(char *name) {
    if (name == NULL) return -1;
    for (int map_no = 0; map_no < MapNameNum; ++map_no) {
        if (map_name[map_no].name != NULL && strcmp(map_name[map_no].name, name) == 0) return map_no;
    }
    return -1;
}

char *GetMapTitle(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->title : NULL;
}

char *GetAddMapPath(int map_no) {
    MAP_NAME_INFO *entry = GetMapNameInfo(map_no);
    return entry != NULL ? entry->add_path : NULL;
}

void InitMapSelect(mgCMemory *stack) {
    MenuStack = stack;
    SetCurrentDir(NULL);
    int list_size;
    LoadFile((char *)"map/map.lst", read_buffer, &list_size);
    input_str lines;
    lines.buffer = (char *)read_buffer;
    lines.size = list_size;
    for (int type = 0; type < MAP_SEL_TYPE_NUM; ++type) {
        SelectMapNum[type] = 0;
        SelectMapList[type] = new (MenuStack->Alloc(0x22)) char *[SELECT_MAP_MAX];
        for (int index = 0; index < SELECT_MAP_MAX; ++index) {
            SelectMapList[type][index] = NULL;
        }
    }
    SelectMode = MAP_SELECT_MODE_TYPE;
    char line[0x100];
    if (!lines.GetLine(line, sizeof(line), NULL)) {
        return;
    }
    int path_length = strlen(line) + 1;
    if (lines.GetLine(line, sizeof(line), NULL)) {
        do {
            if (line[0] != 0) {
                char *letter = line;
                for (; *letter != 0; ++letter) {
                    char ch = *letter;
                    if (ch == 0) {
                        break;
                    }
                    if (ch == '\\') {
                        *letter = '/';
                    }
                    if (strncmp(letter, "cmn", 3) == 0) {
                        letter = NULL;
                        break;
                    }
                }
                if (letter != NULL) {
                    char directory[0x80], name[0x80], extension[0x80];
                    DivPathNameExt(&line[path_length], directory, name, extension);
                    int type = GetMapSelType(SearchMapNo(name));
                    SelectMapList[type][SelectMapNum[type]++] = mgCopyString(name, MenuStack);
                }
            }
        } while (lines.GetLine(line, sizeof(line), NULL));
    }
}

/**
 * Steps and draws the debug map category picker.
 */
static int MapTypeSelect() {
    char display[0x800];
    char *cursor = display;
    static int select = 0;
    if (GamePad.Down(PAD_UP)) {
        select--;
    }
    if (GamePad.Down(PAD_DOWN)) {
        select++;
    }
    if (select < 0) {
        select = MAP_SEL_TYPE_NUM - 1;
    }
    if (select >= MAP_SEL_TYPE_NUM) {
        select = 0;
    }
    if (GamePad.Down(PAD_CIRCLE)) {
        if (SelectMapNum[select] > 0) {
            SelectMapType = select;
            SelectMode = MAP_SELECT_MODE_MAP;
        }
    }
    if (GamePad.Down(PAD_CROSS)) {
        SelectMode = MAP_SELECT_MODE_CANCEL;
    }
    cursor += sprintf(cursor, "\n\n");
    for (int type = 0; type < MAP_SEL_TYPE_NUM; type++) {
        if (type == select) {
            cursor += sprintf(cursor, ">>");
        } else {
            cursor += sprintf(cursor, "  ");
        }
        cursor += sprintf(cursor, "%s", map_sel_type[type]);
        if (type == select) {
            cursor += sprintf(cursor, "<<");
        }
        cursor += sprintf(cursor, "\n");
    }
    GetDebugFont()->DrawDirect(display, 10, 10);
    return 0;
}

/**
 * Steps and draws the maps in the selected category.
 */
static int MapSelect() {
    static int select[MAP_SEL_TYPE_NUM] = {0};
    static int first[MAP_SEL_TYPE_NUM] = {0};
    char display[0x800];
    char *title;
    char *cursor = display;
    int &selected = select[SelectMapType];
    int count;
    int &top = first[SelectMapType];
    int paged;
    int row;
    row = selected - top;
    if (GamePad.Down(PAD_UP)) {
        selected--;
    }
    if (GamePad.Down(PAD_DOWN)) {
        selected++;
    }
    paged = 0;
    if (GamePad.Down(PAD_L1)) {
        paged = 1;
        top -= 8;
    }
    if (GamePad.Down(PAD_R1)) {
        paged = 1;
        top += 8;
    }
    count = SelectMapNum[SelectMapType];
    if (selected < 0) {
        selected = 0;
    }
    if (selected >= count) {
        selected = count - 1;
    }
    if (paged == 0) {
        if (selected - top >= 8) {
            top++;
        }
        if (selected < top) {
            top--;
        }
    }
    if (top + 8 >= count) {
        top = count - 8;
    }
    if (top < 0) {
        top = 0;
    }
    if (paged != 0) {
        selected = top + row;
    }
    int chosen_map = SearchMapNo(SelectMapList[SelectMapType][selected]);
    cursor += sprintf(cursor, "\n\nMapNo = %d\n", chosen_map);
    int last = top + 8;
    if (last > count) {
        last = count;
    }
    for (int index = top; index < last; ++index) {
        int map_no = SearchMapNo(SelectMapList[SelectMapType][index]);
        if (index == selected) {
            cursor += sprintf(cursor, ">>");
        } else {
            cursor += sprintf(cursor, "  ");
        }
        cursor += sprintf(cursor, "%s", SelectMapList[SelectMapType][index]);
        if (chosen_map < 0) {
            cursor += sprintf(cursor, "   *");
        } else {
            cursor += sprintf(cursor, "    ");
        }
        title = NULL;
        GetMapName(map_no, &title);
        if (title != NULL) {
            cursor += sprintf(cursor, "%s", title);
        }
        if (index == selected) {
            cursor += sprintf(cursor, "<<");
        }
        cursor += sprintf(cursor, "\n");
    }
    GetDebugFont()->DrawDirect(display, 10, 10);
    if (GamePad.Down(PAD_CROSS)) {
        SelectMode = MAP_SELECT_MODE_TYPE;
    }
    if (GamePad.Down(PAD_CIRCLE)) {
        strcpy(SelectMapName, SelectMapList[SelectMapType][selected]);
        SelectMode = MAP_SELECT_MODE_DECIDE;
    }
    return 0;
}

int MapSelectLoop() {
    switch (SelectMode) {
    case MAP_SELECT_MODE_CANCEL:
        return MAP_SELECT_CANCEL;
    case MAP_SELECT_MODE_TYPE:
        MapTypeSelect();
        break;
    case MAP_SELECT_MODE_MAP:
        MapSelect();
        break;
    case MAP_SELECT_MODE_DECIDE:
        return MAP_SELECT_DECIDE;
    }
    return MAP_SELECT_CONTINUE;
}

void InitSaveDataEdit(mgCMemory *stack) {
}
int SaveDataEditLoop() {
    CScene *scene = GetMainScene();
    CSaveData *save = GetSaveData();
    char display[0x800];
    char *cursor = display;
    GAME_PROGRESS_INFO *progress;
    const char *progress_name;
    SV_CONFIG_OPTION *config = save->GetConfig();
    const char *marker[2] = {"  ", ">>"};
    const char *on_off[2] = {"OFF", "ON"};
    progress = GetGameProgressInfo(SedSelData[SED_PROGRESS]);
    SedSelData[SED_PLAY_TIME] = GetPlayTimeCountFlag();
    s8 *caption[1] = {&config->caption_off};
    progress_name = marker[0];
    if (progress != NULL) {
        progress_name = progress->name;
    }
    cursor += sprintf(cursor, "Save Data Editer\n\n");
    cursor += sprintf(cursor, "%sPROGRESS  %d(%s)\n", marker[SedSel == SED_PROGRESS], SedSelData[SED_PROGRESS], progress_name);
    cursor += sprintf(cursor, "%sTIME      %5.1f\n", marker[SedSel == SED_TIME], save->now_time);
    cursor += sprintf(cursor, "%sFLAG      %4d = %s\n", marker[SedSel == SED_FLAG], SedSelData[SED_FLAG], on_off[save->GetBitFlag(SedSelData[SED_FLAG]) != 0]);
    cursor += sprintf(cursor, "%sGEO COMP  %d\n", marker[SedSel == SED_GEO_COMP], SedSelData[SED_GEO_COMP]);
    cursor += sprintf(cursor, "%sPLAY TIME %d\n", marker[SedSel == SED_PLAY_TIME], SedSelData[SED_PLAY_TIME]);
    int config_value = *caption[0];
    const int &config_reference = config_value;
    cursor += sprintf(cursor, "%sCONFIG    %s = %d\n", marker[SedSel == SED_CONFIG], config_str[0], config_reference);
    SedSelData[SED_PROGRESS] = save->game_progress;
    if (SedSel == SED_PROGRESS) {
        if (GamePad.Down(PAD_RIGHT)) ++SedSelData[SED_PROGRESS];
        if (GamePad.Down(PAD_LEFT)) --SedSelData[SED_PROGRESS];
        if (SedSelData[SED_PROGRESS] <= 0) SedSelData[SED_PROGRESS] = 1;
        if (SedSelData[SED_PROGRESS] >= GetGameProgressNum()) SedSelData[SED_PROGRESS] = GetGameProgressNum() - 1;
        save->game_progress = SedSelData[SED_PROGRESS];
    }
    if (SedSel == SED_TIME) {
        int hour = (int)save->now_time;
        if (GamePad.Down(PAD_RIGHT)) ++hour;
        if (GamePad.Down(PAD_LEFT)) --hour;
        hour %= 24;
        if (GamePad.Down(PAD_TRIANGLE)) {
            if (hour == 12) {
                hour = 0;
            } else {
                hour = 12;
            }
        }
        scene->SetTime((float)hour);
        save->now_time = (float)hour;
    }
    if (SedSel == SED_FLAG) {
        if (GamePad.Down(PAD_RIGHT)) ++SedSelData[SED_FLAG];
        if (GamePad.Down(PAD_LEFT)) --SedSelData[SED_FLAG];
        if (GamePad.Down(PAD_R1)) SedSelData[SED_FLAG] += 10;
        if (GamePad.Down(PAD_L1)) SedSelData[SED_FLAG] -= 10;
        if (GamePad.Down(PAD_R2)) SedSelData[SED_FLAG] += 100;
        if (GamePad.Down(PAD_L2)) SedSelData[SED_FLAG] -= 100;
        if (SedSelData[SED_FLAG] < 0) SedSelData[SED_FLAG] = 0;
        if (GamePad.Down(PAD_CIRCLE)) save->SetBitFlag(SedSelData[SED_FLAG], !save->GetBitFlag(SedSelData[SED_FLAG]));
    }
    if (SedSel == SED_GEO_COMP) {
        if (GamePad.Down(PAD_RIGHT)) ++SedSelData[SED_GEO_COMP];
        if (GamePad.Down(PAD_LEFT)) --SedSelData[SED_GEO_COMP];
        if (SedSelData[SED_GEO_COMP] < 0) SedSelData[SED_GEO_COMP] = 0;
        if (GamePad.Down(PAD_CIRCLE) || GamePad.Down(PAD_TRIANGLE)) {
            DebugInfo.georama_debug = 1;
            CEditData *edit = save->GetEditData(SedSelData[SED_GEO_COMP]);
            if (edit != NULL) edit->dbgSetAllContintionFlag(SedSelData[SED_GEO_COMP], GamePad.Down(PAD_CIRCLE));
        }
    }
    if (SedSel == SED_PLAY_TIME) {
        if (GamePad.Down(PAD_RIGHT)) SedSelData[SED_PLAY_TIME] = 1;
        if (GamePad.Down(PAD_LEFT)) SedSelData[SED_PLAY_TIME] = 0;
        PlayTimeCount(SedSelData[SED_PLAY_TIME]);
    }
    if (SedSel == SED_CONFIG) {
        if (GamePad.Down(PAD_RIGHT)) ++SedSelData[SED_CONFIG];
        if (GamePad.Down(PAD_LEFT)) --SedSelData[SED_CONFIG];
        SedSelData[SED_CONFIG] = 0;
        if (GamePad.Down(PAD_CIRCLE)) {
            if (*caption[0] != 0) {
                *caption[0] = 0;
            } else {
                *caption[0] = 1;
            }
        }
    }
    if (GamePad.Down(PAD_DOWN)) ++SedSel;
    if (GamePad.Down(PAD_UP)) --SedSel;
    if (SedSel < 0) SedSel = SED_ITEM_NUM - 1;
    if (SedSel >= SED_ITEM_NUM) SedSel = 0;
    GetDebugFont()->DrawDirect(display, 10, 10);
    if (GamePad.Down(PAD_CROSS)) {
        return 1;
    }
    return 0;
}

int EventViewLoop() {
    char display[0x400];
    INIT_LOOP_ARG arg;
    char *cursor = display;
    cursor += sprintf(cursor, "\nEvent \n");
    if (BossBattleSelFlag) {
        if (top_event < BossEventTop) top_event = BossEventTop;
        BossBattleSelFlag = 0;
    }
    int index = top_event;
    int last = index + 10;
    const char *marker[2] = {"  ", ">>"};
    if (last >= EventInfoNum) last = EventInfoNum;
    for (; index < last; ++index) {
        EVENT_VIEW_INFO &entry = EventInfo[index];
        if (entry.name != NULL) cursor += sprintf(cursor, "%s%s   %s\n", marker[index == top_event + sel_event], entry.name, entry.detail);
    }
    GetDebugFont()->DrawDirect(display, 10, 10);
    if (GamePad.Down(PAD_UP)) --sel_event;
    if (GamePad.Down(PAD_DOWN)) ++sel_event;
    if (GamePad.Down(PAD_LEFT | PAD_L1)) top_event -= 10;
    if (GamePad.Down(PAD_RIGHT | PAD_R1)) top_event += 10;
    if (top_event < 0) top_event = 0;
    if (top_event >= EventInfoNum - 1) top_event -= 10;
    if (sel_event < 0) {
        sel_event = 9;
        if (top_event + 9 >= EventInfoNum) sel_event = EventInfoNum - top_event - 1;
    }
    if (sel_event >= 10 || top_event + sel_event >= EventInfoNum) sel_event = 0;
    if (GamePad.Down(PAD_CIRCLE)) {
        memset(&arg, 0, sizeof(arg));
        EVENT_VIEW_INFO &entry = EventInfo[top_event + sel_event];
        if (entry.map_no >= 0) {
            arg.map_no = entry.map_no;
            arg.floor_no = entry.floor_no;
            arg.event_no = entry.event_no;
            if (entry.dungeon != 0) {
                NextLoop(LOOP_DUNGEON, arg);
            } else {
                NextLoop(LOOP_EDIT, arg);
            }
            return EVENT_VIEW_START;
        }
    }
    if (GamePad.Down(PAD_CROSS)) {
        return EVENT_VIEW_CANCEL;
    }
    return EVENT_VIEW_CONTINUE;
}

void LoadEventViewData(u_long128 *buffer, mgCMemory *stack) {
    int file_size;
    char fields[16][0x80];
    char *columns[16];
    int index;
    char *end;
    EVENT_VIEW_INFO *entry;
    int map_no;
    char *next;
    int floor_no;
    int dungeon;
    if (!LoadFile2((char *)"event/view_pal.txt", buffer, &file_size, 0)) return;
    EventInfo = new (stack->Alloc(0x382)) EVENT_VIEW_INFO[EVENT_VIEW_MAX];
    for (index = 0; index < EVENT_VIEW_MAX; ++index) memset(&EventInfo[index], 0, sizeof(EVENT_VIEW_INFO));
    end = (char *)buffer + file_size;
    for (index = 0; index < 16; ++index) columns[index] = fields[index];
    entry = EventInfo;
    EventInfoNum = 0;
    BossEventTop = 0;
    next = GetLine(columns, (char *)buffer, end);
    while (next < end) {
        next = GetLine(columns, next, end);
        map_no = SearchMapNo(columns[0]);
        floor_no = 0;
        dungeon = 0;
        if (columns[1][0] != 0) {
            map_no = atoi(columns[1]) - 1;
            floor_no = atoi(columns[2]);
            dungeon = 1;
        }
        entry->map_no = map_no;
        entry->floor_no = floor_no;
        entry->dungeon = dungeon;
        entry->event_no = atoi(columns[3]);
        entry->name = mgCopyString(columns[6], stack);
        entry->detail = mgCopyString(columns[7], stack);
        EventInfoNum++;
        if (strcmp(columns[8], "B") != 0) ++BossEventTop;
        entry++;
    }
}

/**
 * Splits a tab-separated event record into its columns.
 */
static char *GetLine(char **columns, char *position, char *end) {
    char line_break[2] = {'\r', '\n'};
    int column;
    int length;
    if (position < end) {
        column = 0;
        do {
            if (memcmp(position, line_break, 2) == 0) {
                position += 2;
                break;
            }
            if (memcmp(position, line_break, 1) == 0) {
                position += 1;
                break;
            }
            if (memcmp(position, &line_break[1], 1) == 0) {
                position += 1;
                break;
            }
            length = 0;
            while (position < end) {
                if (memcmp(position, line_break, 2) == 0 ||
                    memcmp(position, line_break, 1) == 0 ||
                    memcmp(position, &line_break[1], 1) == 0) {
                    break;
                }
                s8 ch = *position;
                if (ch == '\t') {
                    char *next = columns[column + 1];
                    position++;
                    if (next != NULL) {
                        *next = 0;
                    }
                    break;
                }
                if (ch != ' ' && columns[column] != NULL) {
                    columns[column][length] = ch;
                    length++;
                }
                position++;
            }
            char *current = columns[column];
            if (current != NULL) {
                column++;
                current[length] = 0;
            }
        } while (position < end);
    }
    return position;
}

void AtraMiriaOnOff(int type, CCharacter2 *chara, int on) {
    mgCFrame *atlamillia;
    mgCFrame *cord;
    mgCFrame *gem;

    if (chara == NULL) {
        return;
    }
    mgCFrame *model = chara->CObjectFrame::frame;
    if (model == NULL) {
        return;
    }
    if (type == 0) {
        atlamillia = model->SearchFrame("atoramiria");
        cord = model->SearchFrame("himo");
        if (on) {
            if (atlamillia != NULL) {
                atlamillia->attr->draw = MG_FRAME_DRAW_VISIBLE | MG_FRAME_DRAW_SKIP_BY_PARENT;
            }
            if (cord != NULL) {
                cord->attr->draw = MG_FRAME_DRAW_VISIBLE;
            }
        } else {
            if (atlamillia != NULL) {
                atlamillia->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            }
            if (cord != NULL) {
                cord->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            }
        }
    }
    if (type == 1) {
        atlamillia = model->SearchFrame("atoramiria");
        if (on) {
            if (atlamillia != NULL) {
                atlamillia->attr->draw = MG_FRAME_DRAW_VISIBLE;
            }
        } else {
            if (atlamillia != NULL) {
                atlamillia->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            }
        }
    }
    if (type == 2) {
        gem = model->SearchFrame("atora");
        if (on) {
            if (gem != NULL) {
                gem->attr->draw = MG_FRAME_DRAW_VISIBLE | MG_FRAME_DRAW_SKIP_BY_PARENT;
            }
        } else {
            if (gem != NULL) {
                gem->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            }
        }
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", map_sel_type__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", SelectMapName__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", tag__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", select__1049__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", top__1050__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", SedSelData__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_792__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_793__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_794__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_795__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_796__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_797__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_798__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_799__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_800__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_801__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_842__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_859__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_860__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1004__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1005__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1040__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1041__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1042__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1043__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1044__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1045__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1103__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1104__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1105__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1117__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1126__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1127__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1222__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1223__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1224__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1225__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1226__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1227__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1228__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1323__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1324__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1372__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1373__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1469__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1470__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1471__3__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", config_str__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1125__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1128__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1270__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapselect", at_1377__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(MapNameNum, 0x4);

INCLUDE_BSS(map_name, 0x4);

INCLUDE_BSS(pMapNameBuff, 0x4);

INCLUDE_BSS(pCharBuff, 0x4);

INCLUDE_BSS(CharBuff, 0x4);

INCLUDE_BSS(now_no, 0x4);

INCLUDE_BSS(MenuStack, 0x4);

INCLUDE_BSS(SelectMode, 0x4);

INCLUDE_BSS(SelectMapType, 0x4);

INCLUDE_BSS(select_1009, 0x4);

INCLUDE_BSS(init_1010, 0x4);

INCLUDE_BSS(SedSel, 0x4);

INCLUDE_BSS(EventInfo, 0x4);

INCLUDE_BSS(EventInfoNum, 0x4);

INCLUDE_BSS(BossEventTop, 0x4);

INCLUDE_BSS(sel_event, 0x4);

INCLUDE_BSS(top_event, 0x4);

INCLUDE_BSS(BossBattleSelFlag, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(MapNameBuff, 0x8000);

INCLUDE_BSS(SelectMapList, 0x20);

INCLUDE_BSS(SelectMapNum, 0x20);
