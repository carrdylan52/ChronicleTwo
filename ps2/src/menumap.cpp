#include "common.h"
#include "menumap.hpp"
#include "menuaqua.hpp"
#include "dngmenu.hpp"
#include "mapselect.hpp"
#include "menucommon.hpp"
#include "menumain.hpp"
#include "scriptinterpreter.hpp"
#include <cstring>

static int _WMAP_POSNUM(SPI_STACK *stack, int argc);
static int _WMAP_POS(SPI_STACK *stack, int argc);
static int _WMAP_AREANUM(SPI_STACK *stack, int argc);
static int _WMAP_AREA(SPI_STACK *stack, int argc);

static void worldmap_analyze(mgCMemory *stack, char *script, int size);

static signed char WorldMapMenuType;
static CWorldMapMenu *WorldMapPtr;
static short Sfida_NowPlayHorlBlink;
static short WorldMap_NextLoopNo;
static short WorldMap_MapNo;
static short spi_wmappos_tblnum;
static WMAP_POS_DATA *spi_wmappos_tbl;
static short spi_wmaparea_tblnum;
static WMAP_AREA_DATA *spi_wmaparea_tbl;
static mgCMemory *spi_wmapstack;
static short MapEnableNum;
static SPI_TAG_PARAM menu_wmap_analyze_tag[] = {
    {"POS_NUM", _WMAP_POSNUM},
    {"POS", _WMAP_POS},
    {"AREA_NUM", _WMAP_AREANUM},
    {"AREA", _WMAP_AREA},
    {NULL, NULL}
};

// Code (.text)
/**
 * Allocates the world-map destination table.
 */
static int _WMAP_POSNUM(SPI_STACK *stack, int argc) {
    unsigned int bytes;
    unsigned int blocks;
    spi_wmappos_tblnum = spiGetStackInt(stack);
    bytes = spi_wmappos_tblnum * sizeof(WMAP_POS_DATA);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    spi_wmappos_tbl = (WMAP_POS_DATA *)spi_wmapstack->Alloc(blocks);
    return 1;
}

/**
 * Stores one destination and its name.
 */
static int _WMAP_POS(SPI_STACK *stack, int argc) {
    char converted_title[0x100];
    WMAP_POS_DATA *pos = &spi_wmappos_tbl[spiGetStackInt(stack++)];
    pos->map_no = spiGetStackInt(stack++);
    pos->loop_no = spiGetStackInt(stack++);
    pos->area_no = spiGetStackInt(stack++);
    pos->dng_no = spiGetStackInt(stack++);
    pos->floor = spiGetStackInt(stack++);
    ConvertFontCode(GetMapTitle(pos->map_no), converted_title);
    pos->name = mgCopyString(converted_title, spi_wmapstack);
    pos->flag_no = spiGetStackInt(stack++);
    int unlocked = CheckBitFlagMenu(pos->flag_no);
    pos->enable = 0;
    if (unlocked != 0) {
        pos->enable = 1;
    }
    pos->type = spiGetStackInt(stack);
    if (pos->type == 4 && CheckBitFlagMenu(0x2E0) != 0) {
        pos->enable = 0;
    }
    return 1;
}

/**
 * Allocates the world-map area table.
 */
static int _WMAP_AREANUM(SPI_STACK *stack, int argc) {
    unsigned int bytes;
    unsigned int blocks;
    spi_wmaparea_tblnum = spiGetStackInt(stack);
    bytes = spi_wmaparea_tblnum * sizeof(WMAP_AREA_DATA);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    spi_wmaparea_tbl = (WMAP_AREA_DATA *)spi_wmapstack->Alloc(blocks);
    return 1;
}

/**
 * Stores an area and its destinations.
 */
static int _WMAP_AREA(SPI_STACK *stack, int argc) {
    char converted_title[0x100];
    int area_no = spiGetStackInt(stack++);
    int map_no = spiGetStackInt(stack++);
    WMAP_AREA_DATA *area = &spi_wmaparea_tbl[area_no];
    area->area_no = area_no;
    area->unk_24 = map_no;
    area->x = spiGetStackInt(stack++);
    area->y = spiGetStackInt(stack++);
    area->name_x = spiGetStackInt(stack++);
    area->name_y = spiGetStackInt(stack++);
    area->name_side = spiGetStackInt(stack++);
    area->y = (int)(1.15f * (float)area->y);
    area->name_y = (int)(1.15f * (float)area->name_y);
    char *title = spiGetStackString(stack);
    memset(converted_title, 0, sizeof(converted_title));
    ConvertFontCode(title, converted_title);
    area->name = mgCopyString(converted_title, spi_wmapstack);
    int position_count = 0;
    area->enable = 0;
    for (int i = 0; i < spi_wmappos_tblnum; i++) {
        if (area_no == spi_wmappos_tbl[i].area_no) {
            WMAP_POS_DATA *pos = &spi_wmappos_tbl[i];
            area->pos[position_count] = pos;
            if (area->pos[position_count]->enable != 0) {
                area->enable = 1;
            }
            position_count++;
        }
    }
    if (area->area_no == 8) {
        if (CheckBitFlagMenu(0x268) != 0) {
            area->enable = 0;
        }
    }
    if (CheckBitFlagMenu(0x320) != 0 && (area->area_no == 6 || area->area_no == 7)) {
        area->enable = 0;
    }
    if (area->enable != 0) {
        MapEnableNum++;
    }
    for (int slot = position_count; slot < 8; slot++) {
        area->pos[slot] = NULL;
    }
    return 1;
}

/**
 * Builds the world-map areas and destinations from their script.
 */
static void worldmap_analyze(mgCMemory *stack, char *script, int size) {
    spi_wmapstack = stack;
    spi_wmappos_tbl = 0;
    MapEnableNum = 0;
    CScriptInterpreter interpreter;
    interpreter.SetTag(menu_wmap_analyze_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

void CWorldMapMenu::SetMsgBuffer() {
    MenuDCMsg[4]->SetMessData(menu_mes_data, menu_mes_data);
    MenuDCMsg[4]->MsgPreset(15);
    MenuDCMsg[4]->ClsMes::fuchi = 5;
    MenuDCMsg[2]->SetMessData(mes_data, menu_mes_data);
    MenuDCMsg[3]->SetMessData(mes_data, menu_mes_data);
    MenuDCMsg[3]->ClsMes::push_button = 0;
    MenuDCMsg[3]->ClsMes::fade_speed = 1.0f;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", KeyStep__13CWorldMapMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", Draw__13CWorldMapMenuFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", WorldMoveInit__FP9mgCMemoryPii);
int WorldMoveKey() {
    if (WorldMapMenuType == 0) {
        return WorldMapPtr->KeyStep();
    }
    if (WorldMapMenuType == 1) {
        int result = DngTreeMapKey();
        if (result == 1) {
            MenuArg.end_code = 0;
            MenuArg.result[0] = 0;
            TreeMapCalledWorldMap = 0;
            MenuArg.result[1] = 0;
            WorldMapMenuType = 0;
            WorldMapPtr->step = 0;
            WorldMapPtr->cursor_view = 1;
            WorldMapPtr->FadeInMenu(40, 0.0f);
            WorldMapPtr->SetMsgBuffer();
            return 0;
        }
        if (result == 2) {
            MenuArg.end_code = 6;
            TreeMapCalledWorldMap = 0;
            MenuArg.result[0] = WorldMap_NextLoopNo;
            MenuArg.result[1] = WorldMap_MapNo;
            MakeDngTreeMapJumpNo(WorldMap_MapNo, MenuArg.result[2], &MenuArg.result[0],
                                 &MenuArg.result[1]);
            MenuArg.result[3] = 0;
            return 2;
        }
    }
    return 0;
}

void WorldMoveDraw() {
    if (WorldMapMenuType == 0) {
        WorldMapPtr->Draw();
    }
    if (WorldMapMenuType == 1) {
        DngTreeMapDraw();
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaScreListUpdate__FP7CDC2Mesi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaMenuInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", OmakeSfidaSelect__Fi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaMenuKey__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaMenuDraw__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaScoreViewInit__FP9mgCMemoryPii);
int SphidaScoreViewKey() {
    if (MenuCommonInfo->CheckPushButton()) {
        MenuSePlay(1);
        return 1;
    }
    Sfida_NowPlayHorlBlink = Sfida_NowPlayHorlBlink + 1;
    if (Sfida_NowPlayHorlBlink > 0x32) {
        Sfida_NowPlayHorlBlink = 0;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", SphidaScoreViewDraw__Fv);

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menumap", __sinit_menumap_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1072__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1081__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1095__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", geo_table_1183__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1342__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1383__2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1184__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1185__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1186__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1187__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1188__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1189__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1302__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1303__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1304__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1305__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1306__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1307__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1308__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1309__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1310__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1311__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1312__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1313__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1314__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1498__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1556__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1557__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1674__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1675__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1676__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1677__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1937__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1938__2__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", D_0037B058__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", __vt__13CWorldMapMenu__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menumap", at_1343__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(spi_wmapstack, 0x4);
INCLUDE_BSS(spi_wmaparea_tblnum, 0x4);
INCLUDE_BSS(spi_wmaparea_tbl, 0x4);
INCLUDE_BSS(spi_wmappos_tblnum, 0x4);
INCLUDE_BSS(spi_wmappos_tbl, 0x4);
INCLUDE_BSS(MapEnableNum, 0x4);
INCLUDE_BSS(WorldMapMenuType, 0x4);
INCLUDE_BSS(WorldMap_NextLoopNo, 0x4);
INCLUDE_BSS(WorldMap_MapNo, 0x4);
INCLUDE_BSS(WorldMap_DngFloor, 0x4);
INCLUDE_BSS(at_1218__2, 0x4);
INCLUDE_BSS(at_1393__2, 0x8);
INCLUDE_BSS(WorldMapPtr, 0x4);
INCLUDE_BSS(SubSaveData__2, 0x4);
INCLUDE_BSS(SubSphidaData, 0x4);
INCLUDE_BSS(SphidaMenuMes, 0x4);
INCLUDE_BSS(SphidaMenuQus, 0x4);
INCLUDE_BSS(SphidaMenuQusDrawFlag, 0x4);
INCLUDE_BSS(SphidaScore, 0x4);
INCLUDE_BSS(SphidaTex, 0x4);
INCLUDE_BSS(SphidaTex2, 0x4);
INCLUDE_BSS(SphidaTex_Sys, 0x4);
INCLUDE_BSS(SphidaCursor, 0x4);
INCLUDE_BSS(SphidaCursorDrawFlag, 0x4);
INCLUDE_BSS(SphidaCursorY, 0x4);
INCLUDE_BSS(SphidaCursorCount, 0x4);
INCLUDE_BSS(SphidaInfoMsgDrawFlag, 0x4);
INCLUDE_BSS(SfidaBGXY, 0x4);
INCLUDE_BSS(SphidaScoreListY, 0x4);
INCLUDE_BSS(SphidaScoreListBarY, 0x4);
INCLUDE_BSS(SphidaSelect, 0x8);
INCLUDE_BSS(SphidaMenuPhase, 0x4);
INCLUDE_BSS(SfidaMakeLine, 0x4);
INCLUDE_BSS(SfidaMoveInitFlag, 0x8);
INCLUDE_BSS(at_1764__3, 0x8);
INCLUDE_BSS(Sfida_NowPlayHorlBlink, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(WorldMapStack, 0x30);
INCLUDE_BSS(SphidaStack, 0x30);
INCLUDE_BSS(SphidaMenuTexbk, 0x20);
