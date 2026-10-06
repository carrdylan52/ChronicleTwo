#include "common.h"
#include "gamedata.hpp"
#include <cstring>

#include <cstdio>

#include "dataread.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "mg_memory.hpp"
#include "savedata.hpp"
#include "scriptinterpreter.hpp"
#include "userdata.hpp"

static s8 ItemCmdMsgTbl[33][8] = { /**< Menu commands for each item group. */
    { 2, 3, 4, 9, 23, 26, 1, -1 },
    { 9, 3, 1, -1, 0, 0, 0, 0 },
    { 10, 9, 26, 1, -1, 0, 0, 0 },
    { 15, 16, 5, 9, 1, -1, 0, 0 },
    { 5, 9, 1, -1, 0, 0, 0, 0 },
    { 30, 3, 1, -1, 0, 0, 0, 0 },
    { 2, 3, 9, 1, -1, 0, 0, 0 },
    { 3, 26, 34, -1, 0, 0, 0, 0 },
    { 2, 9, 1, -1, 0, 0, 0, 0 },
    { 11, -1, 0, 0, 0, 0, 0, 0 },
    { 12, -1, 0, 0, 0, 0, 0, 0 },
    { 13, 5, 1, -1, 0, 0, 0, 0 },
    { 14, 45, 42, 9, 26, 1, -1, 0 },
    { 19, 5, 9, 1, -1, 0, 0, 0 },
    { 2, 3, 46, 26, 1, -1, 0, 0 },
    { 25, 9, 1, -1, -1, 0, 0, 0 },
    { 10, 22, 9, 26, 1, -1, 0, 0 },
    { 20, 5, 9, 1, -1, 0, 0, 0 },
    { 15, -1, 0, 0, 0, 0, 0, 0 },
    { 16, -1, 0, 0, 0, 0, 0, 0 },
    { 15, 16, -1, 0, 0, 0, 0, 0 },
    { 29, -1, 0, 0, 0, 0, 0, 0 },
    { 10, 3, 4, 9, 26, 1, -1, 0 },
    { 22, 9, 1, -1, 0, 0, 0, 0 },
    { 9, 1, -1, 0, 0, 0, 0, 0 },
    { 37, 9, 1, -1, 0, 0, 0, 0 },
    { 38, -1, 0, 0, 0, 0, 0, 0 },
    { 43, 9, 1, -1, 0, 0, 0, 0 },
    { 44, 9, 1, -1, 0, 0, 0, 0 },
    { 15, 16, 1, -1, 0, 0, 0, 0 },
    { 47, -1, 0, 0, 0, 0, 0, 0 },
    { 19, 20, 9, 1, -1, 0, 0, 0 },
    { 48, -1, 0, 0, 0, 0, 0, 0 },
};
static int _DATACOMINIT(SPI_STACK *stack, int argument_count);
static int _DATACOM(SPI_STACK *stack, int argument_count);
static int _DATAWEPNUM(SPI_STACK *stack, int argument_count);
static int _DATAWEP(SPI_STACK *stack, int argument_count);
static int _DATAWEP_ST(SPI_STACK *stack, int argument_count);
static int _DATAWEP_ST_L(SPI_STACK *stack, int argument_count);
static int _DATAWEP2_ST(SPI_STACK *stack, int argument_count);
static int _DATAWEP2_ST_L(SPI_STACK *stack, int argument_count);
static int _DATAWEP_SPE(SPI_STACK *stack, int argument_count);
static int _DATAWEP_BUILDUP(SPI_STACK *stack, int argument_count);
static int _DATAITEMINIT(SPI_STACK *stack, int argument_count);
static int _DATAITEM(SPI_STACK *stack, int argument_count);
static int _DATAATTACHINIT(SPI_STACK *stack, int argument_count);
static int _DATAATTACH_ST(SPI_STACK *stack, int argument_count);
static int _DATAATTACH_ST2(SPI_STACK *stack, int argument_count);
static int _DATAATTACH_ST_SP(SPI_STACK *stack, int argument_count);
static int _DATAROBOINIT(SPI_STACK *stack, int argument_count);
static int _DATAROBO_ANALYZE(SPI_STACK *stack, int argument_count);
static int _DATAGAURDNUM(SPI_STACK *stack, int argument_count);
static int _DATAGAURD(SPI_STACK *stack, int argument_count);
static int _DATAFISHINIT(SPI_STACK *stack, int argument_count);
static int _DATAFISH(SPI_STACK *stack, int argument_count);
static int _MES_SYS(SPI_STACK *stack, int argument_count);
static int _MES_SYS_SPECTOL(SPI_STACK *stack, int argument_count);

static SPI_TAG_PARAM gamedata_tag[] = { /**< Tags of the item configuration scripts. */
    { "COMINIT", _DATACOMINIT },
    { "COM", _DATACOM },
    { "WEPNUM", _DATAWEPNUM },
    { "WEP", _DATAWEP },
    { "WEP_ST", _DATAWEP_ST },
    { "WEP_ST_L", _DATAWEP_ST_L },
    { "WEP_ST2", _DATAWEP2_ST },
    { "WEP_ST2_L", _DATAWEP2_ST_L },
    { "WEP_SPE", _DATAWEP_SPE },
    { "WEP_BUILD", _DATAWEP_BUILDUP },
    { "ITEMINIT", _DATAITEMINIT },
    { "ITEM", _DATAITEM },
    { "AT_INIT", _DATAATTACHINIT },
    { "AT_ST", _DATAATTACH_ST },
    { "AT_ST2", _DATAATTACH_ST2 },
    { "AT_ST_SP", _DATAATTACH_ST_SP },
    { "ROBOINIT", _DATAROBOINIT },
    { "RB_PARTS", _DATAROBO_ANALYZE },
    { "GRDNUM", _DATAGAURDNUM },
    { "GRD", _DATAGAURD },
    { "FISHINIT", _DATAFISHINIT },
    { "FISH", _DATAFISH },
    { "MES_SYS", _MES_SYS },
    { "MES_SYSSPE", _MES_SYS_SPECTOL },
    { NULL, NULL },
};
static mgCMemory     *gamedata_build_stack; /**< Storage for item names. */
static CDataCommon   *comdatapt; /**< Common entry being filled. */
static int            comdatapt_num; /**< Index of the common entry being filled. */
static CDataCommon    local_com_itemdata[432]; /**< Common item storage. */
static CDataItem      local_itemdata[162]; /**< Usable item storage. */
static CDataWeapon    local_weapondata[116]; /**< Weapon storage. */
static CDataAttach    local_attachdata[38]; /**< Attachment storage. */
static CDataRoboPart  local_robodata[68]; /**< Ridepod part storage. */
static CDataBreedFish local_fishdata[20]; /**< Fish storage. */
static s16            local_guarddata[35]; /**< Guard storage. */
static s16            local_itemdatano_converttable[512]; /**< Item number to common entry index. */

// Code (.text)
CGameData *GetGameDataPt() {
    return &GameItemDataManage;
}

CDataItem::CDataItem(void) {
    use_flags = 0;
    status_flags = 0;
    value[0] = 0;
    value[1] = 0;
    value[2] = 0;
}
CDataAttach::CDataAttach() {
    memset(this, 0, sizeof(CDataAttach));
}

CDataWeapon::CDataWeapon() {
    memset(this, 0, sizeof(CDataWeapon));
    durability = 20;
    levelup_exp = 20;
}

int CDataRoboPart::GetOffsetNo() { return this->offset_no; }
CDataBreedFish::CDataBreedFish() {
    memset(this, 0, sizeof(CDataBreedFish));
}

void CGameData::Initialize() {
    max_item_no = 0;
    common_data = local_com_itemdata;
    common_num = 0;
    item_data = local_itemdata;
    item_num = 0;
    weapon_data = local_weapondata;
    weapon_num = 0;
    guard_data = local_guarddata;
    guard_num = 0;
    attach_data = local_attachdata;
    attach_num = 0;
    robo_data = local_robodata;
    robo_num = 0;
    fish_data = local_fishdata;
    fish_num = 0;
    InitItemMes(1, 1);
}

/**
 * Starts the common item table.
 */
static int _DATACOMINIT(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.common_num = spiGetStackInt(stack);
    comdatapt_num = 0;
    comdatapt = GameItemDataManage.common_data;
    memset(local_itemdatano_converttable, -1, sizeof(local_itemdatano_converttable));
    return 1;
}

/**
 * Reads one common item entry and records its item number.
 */
static int _DATACOM(SPI_STACK *stack, int argument_count) {
    char *file_name;

    comdatapt->item_no = spiGetStackInt(stack++);
    comdatapt->type = spiGetStackInt(stack++);
    comdatapt->list_no = spiGetStackInt(stack++);
    comdatapt->active_set = spiGetStackInt(stack++);
    comdatapt->stack_num = spiGetStackInt(stack++);
    comdatapt->max_num = spiGetStackInt(stack++);
    if (ConvertUsedItemType(comdatapt->type) == USED_ITEM_TYPE_WEAPON && comdatapt->max_num > 100) {
        comdatapt->max_num = 144;
    }
    comdatapt->unk_20 = spiGetStackInt(stack++);
    comdatapt->icon_no = spiGetStackInt(stack++);
    comdatapt->message_no = spiGetStackInt(stack++);
    file_name = spiGetStackString(stack++);
    if (file_name != NULL) {
        strcpy(comdatapt->file_name, file_name);
    }
    comdatapt->attribute = spiGetStackInt(stack);
    comdatapt->name = NULL;
    local_itemdatano_converttable[comdatapt->item_no] = comdatapt_num;
    comdatapt_num++;
    comdatapt++;
    return 1;
}

/**
 * Copies the display name of an item into the name buffer.
 */
static int _MES_SYS(SPI_STACK *stack, int argument_count) {
    char         converted[256];
    int          item_no;
    char        *name;
    CDataCommon *data;

    item_no = spiGetStackInt(stack++);
    name = spiGetStackString(stack);
    data = GameItemDataManage.GetCommonData(item_no);
    if (data != NULL) {
        if (LanguageCode >= 2 && LanguageCode < 6) {
            memset(converted, 0, sizeof(converted));
            ConvertFontCode(name, converted);
            data->name = mgCopyString(converted, gamedata_build_stack);
        } else {
            data->name = mgCopyString(name, gamedata_build_stack);
        }
    }
    return 1;
}

/**
 * Reads the unused spectrumising message arguments.
 */
static int _MES_SYS_SPECTOL(SPI_STACK *stack, int argument_count) {
    spiGetStackInt(stack++);
    spiGetStackString(stack);
    return 1;
}

/**
 * Starts the weapon table.
 */
static int _DATAWEPNUM(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.weapon_num = spiGetStackInt(stack);
    SpiWeaponPt = GameItemDataManage.weapon_data;
    return 1;
}

/**
 * Reads two weapon parameters.
 */
static int _DATAWEP(SPI_STACK *stack, int argument_count) {
    if (SpiWeaponPt == NULL) {
        return 0;
    }
    SpiWeaponPt->durability = spiGetStackInt(stack++);
    SpiWeaponPt->levelup_exp = spiGetStackInt(stack);
    return 1;
}

/**
 * Reads two weapon parameters.
 */
static int _DATAWEP_ST(SPI_STACK *stack, int argument_count) {
    if (SpiWeaponPt == NULL) {
        return 0;
    }
    SpiWeaponPt->status[0] = spiGetStackInt(stack++);
    SpiWeaponPt->status[1] = spiGetStackInt(stack);
    return 1;
}

/**
 * Reads two weapon parameters.
 */
static int _DATAWEP_ST_L(SPI_STACK *stack, int argument_count) {
    if (SpiWeaponPt == NULL) {
        return 0;
    }
    SpiWeaponPt->status_max[0] = spiGetStackInt(stack++);
    SpiWeaponPt->status_max[1] = spiGetStackInt(stack);
    return 1;
}

/**
 * Reads the eight weapon attribute parameters.
 */
static int _DATAWEP2_ST(SPI_STACK *stack, int argument_count) {
    int i;

    if (SpiWeaponPt == NULL) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        SpiWeaponPt->attribute[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Reads the eight weapon attribute parameters.
 */
static int _DATAWEP2_ST_L(SPI_STACK *stack, int argument_count) {
    int i;

    if (SpiWeaponPt == NULL) {
        return 0;
    }
    for (i = 0; i < 8; i++) {
        SpiWeaponPt->attribute_max[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Reads weapon abilities and model options.
 */
static int _DATAWEP_SPE(SPI_STACK *stack, int argument_count) {
    if (SpiWeaponPt == NULL) {
        return 0;
    }
    SpiWeaponPt->unk_38 = (u8)spiGetStackFloat(stack++);
    SpiWeaponPt->pallet_color = spiGetStackInt(stack++);
    SpiWeaponPt->unk_47 = spiGetStackInt(stack++);
    SpiWeaponPt->fusion_point = spiGetStackInt(stack++);
    SpiWeaponPt->special = spiGetStackInt(stack++);
    SpiWeaponPt->attack_type = 0;
    if (argument_count >= 6) {
        SpiWeaponPt->attack_type = spiGetStackInt(stack++);
    }
    SpiWeaponPt->model_no = 0;
    if (argument_count >= 7) {
        SpiWeaponPt->model_no = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Reads weapon build-up requirements and advances the weapon entry.
 */
static int _DATAWEP_BUILDUP(SPI_STACK *stack, int argument_count) {
    SpiWeaponPt->buildup_weapon[0] = spiGetStackInt(stack++);
    SpiWeaponPt->buildup_weapon[1] = spiGetStackInt(stack++);
    SpiWeaponPt->buildup_weapon[2] = spiGetStackInt(stack++);
    if (argument_count > 3) {
        SpiWeaponPt->buildup_monster[0] = spiGetStackInt(stack++);
        SpiWeaponPt->buildup_monster[1] = spiGetStackInt(stack++);
        SpiWeaponPt->buildup_monster[2] = spiGetStackInt(stack);
    }
    SpiWeaponPt++;
    return 1;
}

/**
 * Starts the item table.
 */
static int _DATAITEMINIT(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.item_num = spiGetStackInt(stack);
    SpiItemPt = GameItemDataManage.item_data;
    return 1;
}

/**
 * Reads the effects of a usable item.
 */
static int _DATAITEM(SPI_STACK *stack, int argument_count) {
    int use_flags;

    SpiItemPt = GetItemInfoData(spiGetStackInt(stack++));
    if (SpiItemPt != NULL) {
        use_flags = spiGetStackInt(stack++);
        if (use_flags & ITEM_USE_FLAG_CURE_ALL) {
            use_flags = (use_flags & ~ITEM_USE_FLAG_CURE_ALL) | ITEM_USE_FLAG_CURE_STATUS_UNK_8
                        | ITEM_USE_FLAG_CURE_POISON | ITEM_USE_FLAG_CURE_STATUS_UNK_4
                        | ITEM_USE_FLAG_CURE_STATUS_UNK_2 | ITEM_USE_FLAG_CURE_STATUS_UNK_20
                        | ITEM_USE_FLAG_CURE_STATUS_UNK_40;
        }
        SpiItemPt->use_flags = use_flags;
        SpiItemPt->status_flags = spiGetStackInt(stack++);
        SpiItemPt->target_flags = spiGetStackInt(stack++);
        SpiItemPt->value[0] = spiGetStackInt(stack++);
        SpiItemPt->value[1] = spiGetStackInt(stack++);
        SpiItemPt->value[2] = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Starts the attach table.
 */
static int _DATAATTACHINIT(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.attach_num = spiGetStackInt(stack);
    SpiAttach = GameItemDataManage.attach_data;
    return 1;
}

/**
 * Reads attachment parameters.
 */
static int _DATAATTACH_ST(SPI_STACK *stack, int argument_count) {
    int i;

    if (SpiAttach == NULL) {
        return 0;
    }
    for (i = 0; i < 2; i++) {
        SpiAttach->status[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Reads attachment parameters.
 */
static int _DATAATTACH_ST2(SPI_STACK *stack, int argument_count) {
    int i;

    if (SpiAttach == NULL) {
        return 1;
    }
    for (i = 0; i < 8; i++) {
        SpiAttach->attribute[i] = spiGetStackInt(stack++);
    }
    return 1;
}

/**
 * Reads attachment abilities and advances the entry.
 */
static int _DATAATTACH_ST_SP(SPI_STACK *stack, int argument_count) {
    if (SpiAttach == NULL) {
        return 1;
    }
    SpiAttach->special = spiGetStackInt(stack);
    SpiAttach++;
    return 1;
}

/**
 * Starts the robo table.
 */
static int _DATAROBOINIT(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.robo_num = spiGetStackInt(stack);
    SpiRoboPart = GameItemDataManage.robo_data;
    return 1;
}

/**
 * Reads a ridepod part according to its kind.
 */
static int _DATAROBO_ANALYZE(SPI_STACK *stack, int argument_count) {
    int kind;
    int i;

    SpiRoboPart = GameItemDataManage.GetRoboData(spiGetStackInt(stack++));
    if (SpiRoboPart == NULL) {
        return 0;
    }
    kind = spiGetStackInt(stack++);
    SpiRoboPart->use_capacity = spiGetStackInt(stack++);
    SpiRoboPart->offset_no = spiGetStackInt(stack++);
    if (kind == 0) {
        SpiRoboPart->unk_1c = spiGetStackInt(stack++);
        spiGetStackString(stack);
    } else if (kind == 1) {
        SpiRoboPart->unk_6 = spiGetStackInt(stack++);
        SpiRoboPart->unk_8 = spiGetStackInt(stack++);
        SpiRoboPart->unk_a = spiGetStackInt(stack++);
        for (i = 0; i < 8; i++) {
            SpiRoboPart->unk_c[i] = spiGetStackInt(stack++);
        }
        SpiRoboPart->info_type_d = spiGetStackInt(stack++);
        spiGetStackString(stack);
    } else if (kind == 2) {
        SpiRoboPart->unk_4 = spiGetStackInt(stack++);
        SpiRoboPart->info_type_e = spiGetStackInt(stack);
    } else if (kind == 3) {
        SpiRoboPart->unk_2 = spiGetStackInt(stack);
    }
    SpiRoboPart++;
    return 1;
}

/**
 * Starts the fish table.
 */
static int _DATAFISHINIT(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.fish_num = spiGetStackInt(stack);
    SpiFish = GameItemDataManage.fish_data;
    return 1;
}

/**
 * Reads the base parameters of a fish.
 */
static int _DATAFISH(SPI_STACK *stack, int argument_count) {
    SpiFish = GameItemDataManage.GetFishData(spiGetStackInt(stack++));
    if (SpiFish != NULL) {
        SpiFish->size = spiGetStackFloat(stack++);
        SpiFish->unk_4 = spiGetStackInt(stack++);
        SpiFish->unk_6 = spiGetStackInt(stack++);
        SpiFish->unk_a = spiGetStackInt(stack++);
        SpiFish->unk_c = spiGetStackInt(stack++);
        SpiFish->unk_e = spiGetStackInt(stack++);
        SpiFish->unk_8 = spiGetStackInt(stack++);
        if (argument_count < 8) {
            return 1;
        }
        SpiFish->unk_10 = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the number of guard entries.
 */
static int _DATAGAURDNUM(SPI_STACK *stack, int argument_count) {
    GameItemDataManage.guard_num = spiGetStackInt(stack);
    return 1;
}

/**
 * Reads a guard entry and consumes its remaining parameters.
 */
static int _DATAGAURD(SPI_STACK *stack, int argument_count) {
    s16 *guard;

    guard = GameItemDataManage.GetGuardData(spiGetStackInt(stack++));
    if (guard == NULL) {
        return 1;
    }
    *guard = spiGetStackInt(stack++);
    spiGetStackInt(stack++);
    spiGetStackInt(stack++);
    spiGetStackInt(stack++);
    spiGetStackInt(stack++);
    spiGetStackInt(stack);
    return 1;
}

/**
 * Reads one item configuration script into the item tables.
 */
static int LoadGameDataAnalyze(char *file_name) {
    u_long128  buffer[0x780];
    char       path[64];
    u_long128 *aligned_buffer;
    int        size;

    aligned_buffer = MenuCalcBufAlignment(buffer);
    SetCurrentDir(NULL);
    sprintf(path, "menu/cfg7/%s", file_name);
    if (LoadFile2(path, aligned_buffer, &size, LOAD_FILE_READ) == 0) {
        return 0;
    }

    CScriptInterpreter interpreter;

    interpreter.SetTag(gamedata_tag);
    interpreter.SetScript((char *)aligned_buffer, size);
    interpreter.Run();
    return 1;
}

int CGameData::LoadData() {
    int item_no;

    Initialize();
    comdatapt = common_data;
    comdatapt_num = 0;
    memset(local_itemdatano_converttable, -1, sizeof(local_itemdatano_converttable));
    LoadGameDataAnalyze("comdat.cfg");
    LoadGameDataAnalyze("wepdat.cfg");
    LoadGameDataAnalyze("itemdat.cfg");
    LoadGameDataAnalyze("atdat.cfg");
    LoadGameDataAnalyze("robodat.cfg");
    LoadGameDataAnalyze("fishdat.cfg");
    LoadGameDataAnalyze("grddat.cfg");
    common_num = comdatapt_num;
    max_item_no = 0;
    for (item_no = 0; item_no < 512; item_no++) {
        if (0 <= local_itemdatano_converttable[item_no]) {
            max_item_no = item_no;
        }
    }
    return unk_0;
}

int CGameData::LoadItemSystemMes(int language) {
    static u_long128  gamedata_sysword_buffer[0x280]; /**< Item display name storage. */
    u_long128         buffer[0x780];
    u_long128        *aligned_buffer;
    int               size;

    aligned_buffer = MenuCalcBufAlignment(buffer);
    memset(gamedata_sysword_buffer, 0, sizeof(gamedata_sysword_buffer));
    mgCMemory memory;
    char path[64];
    memory.stSetBuffer(gamedata_sysword_buffer, 0x280);
    gamedata_build_stack = &memory;
    sprintf(path, "menu/cfg7/comdatmes%d.cfg", language);
    if (LoadFile2(path, aligned_buffer, &size, LOAD_FILE_READ) != 0) {
        CScriptInterpreter interpreter;

        interpreter.SetTag(gamedata_tag);
        interpreter.SetScript((char *)aligned_buffer, size);
        interpreter.Run();
    }
    return 1;
}

void CGameData::InitItemMes(int clear_name, int unused) {
    int i;

    if (clear_name != 0) {
        for (i = 0; i < 432; i++) {
            local_com_itemdata[i].name = NULL;
        }
    }
}

CDataCommon *CGameData::GetCommonData(int item_no) {
    s16 index;

    if (item_no <= 0 || item_no > 511) {
        return NULL;
    }
    index = local_itemdatano_converttable[item_no];
    if (index < 0) {
        return NULL;
    }
    return &common_data[index];
}

CDataWeapon *CGameData::GetWeaponData(int item_no) {
    CDataCommon *data;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (weapon_num <= data->list_no) {
        return NULL;
    }
    if (weapon_data == NULL) {
        return NULL;
    }
    if (ConvertUsedItemType(data->type) != USED_ITEM_TYPE_WEAPON) {
        return NULL;
    }
    return &weapon_data[data->list_no];
}

CDataItem *CGameData::GetItemData(int item_no) {
    CDataCommon *data;
    int          family;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (item_num <= data->list_no) {
        return NULL;
    }
    if (item_data == NULL) {
        return NULL;
    }
    family = ConvertUsedItemType(data->type);
    if (family == USED_ITEM_TYPE_ITEM || family == USED_ITEM_TYPE_GIFT_BOX || family == USED_ITEM_TYPE_BOILED) {
        return &item_data[data->list_no];
    }
    return NULL;
}

CDataAttach *CGameData::GetAttachData(int item_no) {
    CDataCommon *data;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (attach_num <= data->list_no) {
        return NULL;
    }
    if (attach_data == NULL) {
        return NULL;
    }
    if (ConvertUsedItemType(data->type) != USED_ITEM_TYPE_ATTACH) {
        return NULL;
    }
    return &attach_data[data->list_no];
}

CDataRoboPart *CGameData::GetRoboData(int item_no) {
    CDataCommon *data;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (robo_num <= data->list_no) {
        return NULL;
    }
    if (robo_data != NULL) {
        return &robo_data[data->list_no];
    }
    return NULL;
}

CDataBreedFish *CGameData::GetFishData(int item_no) {
    CDataCommon *data;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (fish_num <= data->list_no) {
        return NULL;
    }
    if (fish_data == NULL) {
        return NULL;
    }
    if (ConvertUsedItemType(data->type) != USED_ITEM_TYPE_FISH) {
        return NULL;
    }
    return &fish_data[data->list_no];
}

s16 *CGameData::GetGuardData(int item_no) {
    CDataCommon *data;

    data = GetCommonData(item_no);
    if (data == NULL) {
        return NULL;
    }
    if (guard_num <= data->list_no) {
        return NULL;
    }
    if (guard_data != NULL) {
        return &guard_data[data->list_no];
    }
    return NULL;
}

int CGameData::GetDataType(s32 item_no) {
    CDataCommon *common = GetCommonData(item_no);
    if (common != NULL) {
        return common->type;
    }
    return 0U;
}
int CGameData::GetDataTypeStartListNo(int type) {
    CDataCommon *data;
    int          i;

    data = GetCommonData(1);
    for (i = 0; i < common_num; i++, data++) {
        if (data->type == type) {
            return data->item_no;
        }
    }
    return 0;
}

CDataCommon *GetCommonItemData(int item_no) {
    return GameItemDataManage.GetCommonData(item_no);
}

CDataItem *GetItemInfoData(int item_no) {
    return GameItemDataManage.GetItemData(item_no);
}

CDataWeapon *GetWeaponInfoData(int item_no) {
    return GameItemDataManage.GetWeaponData(item_no);
}

CDataRoboPart *GetRoboPartInfoData(int item_no) {
    return GameItemDataManage.GetRoboData(item_no);
}

CDataBreedFish *GetBreedFishInfoData(int item_no) {
    return GameItemDataManage.GetFishData(item_no);
}

char *GetItemFileName(int item_no, int with_extension) {
    static char  filename[32]; /**< Item model file name buffer. */
    CDataCommon *data;
    CSaveData   *save;
    char        *file_name;

    data = GetCommonItemData(item_no);
    if (data == NULL) {
        return NULL;
    }
    file_name = data->file_name;
    if (file_name == NULL) {
        return NULL;
    }
    strcpy(filename, file_name);
    save = GetSaveData();
    if ((data->type == 5 || data->type == 8) && save->GetBitFlag(799) != 0) {
        strcat(filename, "t");
    }
    if (with_extension != 0 && with_extension == 1) {
        strcat(filename, ".chr");
    }
    return filename;
}

char *GetItemFilePath(int item_no, int alternate) {
    static char  item_file_path[128]; /**< Item model path buffer. */
    CDataCommon *data;
    char         file_name[32];
    char        *name;
    int          family;

    item_file_path[0] = '\0';
    data = GameItemDataManage.GetCommonData(item_no);
    if (data != NULL) {
        family = ConvertUsedItemType(data->type);
        name = GetItemFileName(item_no, 0);
        if (name != NULL) {
            strcpy(file_name, name);
        }
        switch (family) {
            case USED_ITEM_TYPE_WEAPON:
            case USED_ITEM_TYPE_UNK_4:
                strcpy(item_file_path, "mainchr/");
                break;
            case USED_ITEM_TYPE_ROBO_PART:
                strcpy(item_file_path, "dungeon/robo/");
                break;
            default:
                strcpy(item_file_path, "item/");
                break;
        }
        strcat(item_file_path, file_name);
        strcat(item_file_path, ".chr");
        if (alternate == 1 && family == USED_ITEM_TYPE_WEAPON) {
            sprintf(item_file_path, "wep_t/%s_item.chr", file_name);
        }
        if (alternate == 1 && (data->type == 0xD || data->type == 0xE)) {
            sprintf(item_file_path, "wep_t/%s.chr", file_name);
        }
    }
    return item_file_path;
}

int GetItemDataType(s32 item_no) {
    return GameItemDataManage.GetDataType(item_no);
}
u32 GetItemDataAttribute(int item_no) {
    CDataCommon *data;

    data = GameItemDataManage.GetCommonData(item_no);
    if (data != NULL) {
        return data->attribute;
    }
    return 0;
}

int ConvertUsedItemType(int type) {
    int family;

    family = USED_ITEM_TYPE_NONE;
    if (type > 0 && type < 5) {
        family = USED_ITEM_TYPE_WEAPON;
    } else if (type >= 5 && type < 11) {
        family = USED_ITEM_TYPE_UNK_4;
    } else if (type > 11 && type < 16) {
        family = USED_ITEM_TYPE_ROBO_PART;
    } else if ((type >= 16 && type <= 19) || type == 0x22) {
        family = USED_ITEM_TYPE_ATTACH;
    } else if (type == 11 || type >= 20) {
        family = USED_ITEM_TYPE_ITEM;
    }
    if (type == 28) {
        family = USED_ITEM_TYPE_GIFT_BOX;
    } else if (type == 30) {
        family = USED_ITEM_TYPE_FISH;
    } else if (type == 35) {
        family = USED_ITEM_TYPE_BOILED;
    }
    return family;
}

int GetItemMessageNo(int item_no, int message) {
    static s16   msg_offsettbl[3] = { 0, 10000, 0 }; /**< Offsets of the item message groups. */
    CDataCommon *data;

    data = GameItemDataManage.GetCommonData(item_no);
    if (data == NULL) {
        return -1;
    }
    return data->message_no + msg_offsettbl[message];
}

char * GetItemMessage(int item_no) {
    CDataCommon *data;

    data = GameItemDataManage.GetCommonData(item_no);
    if (data != NULL) {
        return data->name;
    }
    return NULL;
}

int GetItemIconNo(int item_no) {
    CDataCommon *common = GameItemDataManage.GetCommonData(item_no);
    if (common != NULL) {
        return common->icon_no;
    }
    return -1;
}
void SetItemSpectolPoint(int item_no, ATTACH_USED *attach, int num) {
    int index;
    int slot;
    int value;

    if (item_no > 0 && attach != NULL) {
        index = item_no - 1;
        slot = etcitem_spectol_table[index][0];
        value = etcitem_spectol_table[index][1];
        if (slot < 8) {
            attach->attribute[slot] = value * num;
        }
        if (slot >= 10) {
            attach->status[slot - 10] = value * num;
        }
    }
}

/**
 * Copies one item command group's message numbers and terminates the list.
 */
static int ItemCmdMsgSet(int group, int *message_list) {
    int count;
    int i;

    count = 0;
    for (i = 0; i < 8; i++) {
        message_list[i] = ItemCmdMsgTbl[group][i] + 5000;
        if (message_list[i] < 5000) {
            break;
        }
        count++;
    }
    message_list[i] = -1;
    return count;
}

#ifdef NONMATCHING
int GetMenuCommandMsg(int item_no, int *message_list) {
    int count;
    int type;

    count = 0;
    type = GetItemDataType(item_no);
    switch (type) {
        case 1:
        case 2:
        case 3:
        case 4:
            count = ItemCmdMsgSet(0, message_list);
            if (item_no == 0x12E || item_no == 0x12F) {
                count = ItemCmdMsgSet(14, message_list);
            }
            break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
            count = ItemCmdMsgSet(8, message_list);
            break;
        case 16:
        case 18:
        case 34:
            count = ItemCmdMsgSet(1, message_list);
            break;
        case 19:
            count = ItemCmdMsgSet(24, message_list);
            break;
        case 17:
            count = ItemCmdMsgSet(5, message_list);
            break;
        case 11:
            count = ItemCmdMsgSet(7, message_list);
            break;
        case 12:
        case 14:
            count = ItemCmdMsgSet(2, message_list);
            break;
        case 13:
            count = ItemCmdMsgSet(22, message_list);
            break;
        case 15:
            count = ItemCmdMsgSet(16, message_list);
            break;
        case 22:
            count = ItemCmdMsgSet(3, message_list);
            break;
        case 20:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 33:
            switch (item_no) {
                case 0x126:
                    count = ItemCmdMsgSet(13, message_list);
                    break;
                case 0x12A:
                case 0x160:
                    count = ItemCmdMsgSet(17, message_list);
                    break;
                case 0x17D:
                    count = ItemCmdMsgSet(23, message_list);
                    break;
                case 0x128:
                    count = ItemCmdMsgSet(20, message_list);
                    break;
                case 0x184:
                    count = ItemCmdMsgSet(18, message_list);
                    break;
                case 0x185:
                    count = ItemCmdMsgSet(19, message_list);
                    break;
                case 0x182:
                    count = ItemCmdMsgSet(21, message_list);
                    break;
                case 0x124:
                case 0x111:
                case 0x11F:
                    count = ItemCmdMsgSet(3, message_list);
                    break;
                case 0x163:
                    count = ItemCmdMsgSet(26, message_list);
                    break;
                case 0x1A7:
                    count = ItemCmdMsgSet(25, message_list);
                    break;
                case 0x125:
                    count = ItemCmdMsgSet(27, message_list);
                    break;
                case 0xAE:
                    count = ItemCmdMsgSet(28, message_list);
                    break;
                case 0xAC:
                    count = ItemCmdMsgSet(30, message_list);
                    break;
                case 0x127:
                    count = ItemCmdMsgSet(31, message_list);
                    break;
                default:
                    count = ItemCmdMsgSet(4, message_list);
                    break;
            }
            break;
        case 29:
            count = ItemCmdMsgSet(9, message_list);
            break;
        case 21:
            count = ItemCmdMsgSet(10, message_list);
            break;
        case 28:
            count = ItemCmdMsgSet(11, message_list);
            break;
        case 30:
            count = ItemCmdMsgSet(12, message_list);
            break;
        case 32:
            count = ItemCmdMsgSet(15, message_list);
            break;
        case 35:
            count = ItemCmdMsgSet(29, message_list);
            break;
    }
    return count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gamedata", GetMenuCommandMsg__FiPi);
#endif

#ifdef NONMATCHING
int CheckItemEquip(int chara, int item_no) {
    if (GetItemInfoData(item_no) == NULL) {
        return 0;
    }
    switch (item_no) {
        case 0x12A:
        case 0x171:
            if (chara != 0) {
                return 0;
            }
            break;
        case 0x160:
            if (chara != 1) {
                return 0;
            }
            break;
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gamedata", CheckItemEquip__Fii);
#endif

int SearchItemByName(char *name) {
    CDataCommon *data;
    int          item_no;

    for (item_no = 1; item_no < 512; item_no++) {
        data = GetCommonItemData(item_no);
        if (data != NULL && data->name != NULL && strcmp(data->name, name) == 0) {
            return item_no;
        }
    }
    return -1;
}

#ifdef NONMATCHING
int GetRidePodCore(int index) {
    static s16 table[8] = { 246, 247, 248, 249, 250, 251, 252, -1 }; /**< Ridepod core item numbers. */

    if (index < 0 || index >= 7) {
        return 0;
    }
    return table[index];
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gamedata", GetRidePodCore__Fi);
#endif

/**
 * Clears the effects of a usable item.
 */
static void Init_USEITEM_EFFECT(USEITEM_EFFECT *effect) {
    effect->target_flags = 0;
    effect->use_flags = 0;
    effect->status_flags = 0;
    effect->value[3] = 0;
    effect->value[2] = 0;
    effect->value[1] = 0;
    effect->value[0] = 0;
}
#ifdef NONMATCHING
int GetUsedItemAfterEffect(int item_no, USEITEM_EFFECT *effect) {
    CDataItem *data;

    Init_USEITEM_EFFECT(effect);
    data = GetItemInfoData(item_no);
    if (data == NULL || effect == NULL) {
        return 0;
    }
    effect->status_flags = data->status_flags;
    effect->use_flags = data->use_flags;
    effect->target_flags = data->target_flags;
    effect->value[0] = data->value[0];
    effect->value[1] = data->value[1];
    effect->value[2] = data->value[2];
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gamedata", GetUsedItemAfterEffect__FiP14USEITEM_EFFECT);
#endif

void CItemUseTarget::SetPtr(int type, void *target) {
    this->type = type;
    if (this->type == ITEM_USE_TARGET_CHARA) {
        this->target.data = target;
    }
    if (this->type == ITEM_USE_TARGET_ITEM) {
        this->target.data = target;
    }
    if (this->type == ITEM_USE_TARGET_ROBO) {
        this->target.data = target;
    }
    if (this->type == ITEM_USE_TARGET_MONSTER) {
        this->target.data = target;
    }
}


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", etcitem_spectol_table__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", gamedata_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", ItemCmdMsgTbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", table_1553__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1018__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1019__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1020__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1021__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1022__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1023__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1024__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1025__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1026__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1027__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1028__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1029__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1030__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1031__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1032__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1033__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1034__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1035__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1036__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1037__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1038__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1039__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1040__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1041__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1048__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1063__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1064__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1065__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1066__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1067__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1068__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1069__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1079__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1283__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1284__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1307__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1308__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1309__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1310__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1311__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", at_1501__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gamedata", msg_offsettbl_1363__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(gamedata_build_stack, 0x4);
INCLUDE_BSS(comdatapt, 0x4);
INCLUDE_BSS(comdatapt_num, 0x4);
INCLUDE_BSS(SpiWeaponPt, 0x4);
INCLUDE_BSS(SpiItemPt, 0x4);
INCLUDE_BSS(SpiAttach, 0x4);
INCLUDE_BSS(SpiRoboPart, 0x4);
INCLUDE_BSS(SpiFish, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(GameItemDataManage, 0x30);
INCLUDE_BSS(local_com_itemdata, 0x4A40);
INCLUDE_BSS(local_itemdata, 0xA20);
INCLUDE_BSS(local_weapondata, 0x2270);
INCLUDE_BSS(local_attachdata, 0x390);
INCLUDE_BSS(local_robodata, 0x990);
INCLUDE_BSS(local_fishdata, 0x190);
INCLUDE_BSS(local_guarddata, 0x50);
INCLUDE_BSS(local_itemdatano_converttable, 0x400);
INCLUDE_BSS(gamedata_sysword_buffer_1073, 0x2800);
INCLUDE_BSS(filename_1267, 0x20);
INCLUDE_BSS(item_file_path_1288, 0x80);
