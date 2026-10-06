#include "common.h"
#include "userdata.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "gamedata.hpp"
#include "savedata.hpp"
#include "mainloop.hpp"
#include "menucommon.hpp"
#include "menucls1.hpp"
#include "scene.hpp"
#include "monster.hpp"
#include "menuchr.hpp"
#include "memcard.hpp"
#include "quest.hpp"
#include "npccfg.hpp"
#include "scenesnd.hpp"

// Integer division in item names checks for a zero divisor.
#pragma divbyzerocheck on

/**
 * Fields encoded in a fish password.
 */
struct PackedFish {
    u16 item_no : 9; /**< Item number of the fish. */
    u8  sex : 1; /**< Sex of the fish. */
    u8  unk_3a : 5;
    u8  param_3 : 7; /**< Encoded fifth fish parameter. */
    u16 unk_3c : 7;
    u16 length : 15; /**< Size of the fish. */
    u16 weight : 15; /**< Weight of the fish. */
    u8  param_0 : 7; /**< Encoded first fish parameter. */
    u16 param_1 : 7; /**< Encoded second fish parameter. */
    u8  color_no : 2; /**< Two-bit value stored in the fish's unk_16 field. */
    u8  param_2 : 7; /**< Encoded third fish parameter. */
    u16 flags : 8; /**< Special flags of the fish. */
};
/**
 * Byte representation of a fish password.
 */
union PackedFishBuffer {
    signed char bytes[14]; /**< Encoded password bytes. */
    PackedFish  fish; /**< Structured password fields. */
};

/**
 * Equipment saved before entering a fishing game.
 */
static CGameDataUsed *FishGamePreEquip;

/**
 * Scene time cached for the battle parameters.
 */
static float BattleParamater_Time;

/**
 * Time band cached for the battle parameters.
 */
static int BattleParamater_TimeBand;

/**
 * Number of fish each aquarium tank holds.
 */
s8 aquarium_fish_maxtbl[3] = {6, 4, 2};

/**
 * Fish item numbers in fishing-record slot order.
 */
static s16 fish_record_dataindex_convert[19] = {320, 321, 322, 323, 324, 325, 326, 327, 328, 329,
                                                330, 331, 332, 333, 334, 335, 336, 310, -1};

/**
 * Parameters for the monster transformations.
 */
static MOS_HENGE_PARAM mos_henge_param[57] = {
    {0, 8, 4, 0, "f201a", {NULL, NULL, NULL, NULL}},
    {1, 22, 12, 0, "f201a", {NULL, NULL, NULL, NULL}},
    {2, 64, 24, 0, "f201a", {NULL, NULL, NULL, NULL}},
    {3, 96, 42, 0, "f201a", {NULL, NULL, NULL, NULL}},
    {8, 7, 3, 0, "f203a", {"\x93\xC5\x89\x74\x4C", "\x93\xC5\x89\x74\x48", NULL, NULL}},
    {9, 23, 11, 0, "f203a", {"\x93\xC5\x89\x74\x4C", "\x93\xC5\x89\x74\x48", NULL, NULL}},
    {10, 60, 23, 0, "f203a", {"\x93\xC5\x89\x74\x4C", "\x93\xC5\x89\x74\x48", NULL, NULL}},
    {11, 95, 41, 0, "f203a", {"\x93\xC5\x89\x74\x4C", "\x93\xC5\x89\x74\x48", NULL, NULL}},
    {22,
     60,
     20,
     0,
     "f206a",
     {"\x66\x6F\x78\x5F\x8F\x65\x92\x65",
      "\x83\x7D\x83\x59\x83\x8B\x83\x74\x83\x89\x83\x62\x83\x56\x83\x85", NULL, NULL}},
    {23,
     88,
     39,
     0,
     "f206a",
     {"\x66\x6F\x78\x5F\x8F\x65\x92\x65",
      "\x83\x7D\x83\x59\x83\x8B\x83\x74\x83\x89\x83\x62\x83\x56\x83\x85", NULL, NULL}},
    {24, 8, 3, 0, "f207a", {NULL, NULL, NULL, NULL}},
    {25, 22, 10, 0, "f207a", {NULL, NULL, NULL, NULL}},
    {26, 64, 21, 0, "f207a", {NULL, NULL, NULL, NULL}},
    {27, 96, 39, 0, "f207a", {NULL, NULL, NULL, NULL}},
    {44,
     7,
     3,
     0,
     "f213a",
     {"\x89\xCE\x92\x65\x82\x6B\x82\x52", "\x89\xCE\x92\x65\x82\x67", NULL, NULL}},
    {45,
     23,
     10,
     0,
     "f213a",
     {"\x95\x97\x92\x65\x82\x6B\x82\x51", "\x95\x97\x92\x65\x82\x67", NULL, NULL}},
    {46,
     60,
     21,
     0,
     "f213a",
     {"\x95\x58\x92\x65\x82\x6B\x82\x51", "\x95\x58\x92\x65\x82\x67", NULL, NULL}},
    {47,
     95,
     39,
     0,
     "f213a",
     {"\x97\x8B\x92\x65\x82\x6B\x82\x51", "\x97\x8B\x92\x65\x82\x67", NULL, NULL}},
    {52,
     9,
     4,
     0,
     "f216a",
     {"\x83\x71\x83\x7D\x81\x5B\x83\x89\x89\xF1\x93\x5D\x8D\x55\x8C\x82",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67",
      NULL}},
    {53,
     23,
     12,
     0,
     "f216a",
     {"\x83\x71\x83\x7D\x81\x5B\x83\x89\x89\xF1\x93\x5D\x8D\x55\x8C\x82",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67",
      NULL}},
    {54,
     65,
     24,
     0,
     "f216a",
     {"\x83\x71\x83\x7D\x81\x5B\x83\x89\x89\xF1\x93\x5D\x8D\x55\x8C\x82",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67",
      NULL}},
    {55,
     99,
     42,
     0,
     "f216a",
     {"\x83\x71\x83\x7D\x81\x5B\x83\x89\x89\xF1\x93\x5D\x8D\x55\x8C\x82",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67",
      NULL}},
    {72,
     7,
     3,
     0,
     "f221a",
     {"\x8E\xF4\x82\xA2\x92\x65\x82\x65", "\x8E\xF4\x82\xA2\x92\x65\x82\x6B",
      "\x8E\xF4\x82\xA2\x92\x65\x82\x67", NULL}},
    {73,
     23,
     11,
     0,
     "f221a",
     {"\x8E\xF4\x82\xA2\x92\x65\x82\x65", "\x8E\xF4\x82\xA2\x92\x65\x82\x6B",
      "\x8E\xF4\x82\xA2\x92\x65\x82\x67", NULL}},
    {74,
     60,
     23,
     0,
     "f221a",
     {"\x8E\xF4\x82\xA2\x92\x65\x82\x65", "\x8E\xF4\x82\xA2\x92\x65\x82\x6B",
      "\x8E\xF4\x82\xA2\x92\x65\x82\x67", NULL}},
    {75,
     95,
     41,
     0,
     "f221a",
     {"\x8E\xF4\x82\xA2\x92\x65\x82\x65", "\x8E\xF4\x82\xA2\x92\x65\x82\x6B",
      "\x8E\xF4\x82\xA2\x92\x65\x82\x67", NULL}},
    {102,
     59,
     30,
     0,
     "f10a",
     {"\x82\xA9\x82\xDA\x82\xBF\x82\xE1\x94\x9A\x92\x65", "\x82\x6C\x8F\xAC\x94\x9A\x94\xAD", NULL,
      NULL}},
    {103,
     88,
     50,
     0,
     "f10a",
     {"\x82\xA9\x82\xDA\x82\xBF\x82\xE1\x94\x9A\x92\x65", "\x82\x6C\x8F\xAC\x94\x9A\x94\xAD", NULL,
      NULL}},
    {112,
     80,
     26,
     0,
     "f114a",
     {"\x89\xCE\x92\x65\x82\x65", "\x89\xCE\x92\x65\x82\x6B\x82\x52", NULL, NULL}},
    {116,
     44,
     22,
     0,
     "f114a",
     {"\x95\x58\x92\x65\x82\x65", "\x95\x58\x92\x65\x82\x6B\x82\x51", "\x95\x58\x92\x65\x82\x67",
      NULL}},
    {120,
     55,
     24,
     0,
     "f114a",
     {"\x97\x8B\x92\x65\x82\x65", "\x97\x8B\x92\x65\x82\x6B\x82\x51", "\x97\x8B\x92\x65\x82\x67",
      NULL}},
    {124,
     33,
     20,
     0,
     "f114a",
     {"\x95\x97\x92\x65\x82\x65", "\x95\x97\x92\x65\x82\x6B\x82\x51", "\x95\x97\x92\x65\x82\x67",
      NULL}},
    {128,
     75,
     28,
     0,
     "f114a",
     {"\x90\xB9\x92\x65\x82\x65", "\x90\xB9\x92\x65\x82\x6B\x82\x51", "\x90\xB9\x92\x65\x82\x67",
      NULL}},
    {136, 30, 12, 0, "f06a", {NULL, NULL, NULL, NULL}},
    {137, 70, 24, 0, "f06a", {NULL, NULL, NULL, NULL}},
    {139, 100, 34, 0, "f06a", {NULL, NULL, NULL, NULL}},
    {150,
     60,
     23,
     0,
     "f24a",
     {"\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B\x82\x51",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67", NULL, NULL}},
    {151,
     95,
     41,
     0,
     "f24a",
     {"\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x6B\x82\x51",
      "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67", NULL, NULL}},
    {154,
     55,
     30,
     0,
     "f26a",
     {"\x90\xCE\x89\xBB\x92\x65\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67", NULL, NULL}},
    {155,
     88,
     42,
     0,
     "f26a",
     {"\x90\xCE\x89\xBB\x92\x65\x82\x6B", "\x82\xCB\x82\xCE\x82\xCB\x82\xCE\x82\x67", NULL, NULL}},
    {166, 65, 40, 0, "f05a", {NULL, NULL, NULL, NULL}},
    {167, 85, 60, 0, "f05a", {NULL, NULL, NULL, NULL}},
    {176, 7, 3, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {177, 23, 11, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {180, 60, 23, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {183, 95, 41, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {186, 61, 25, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {187, 89, 38, 0, "f03a", {NULL, NULL, NULL, NULL}},
    {220, 8, 3, 0, "f49a", {"\x83\x7B\x83\x93\x83\x6F\x83\x77\x8E\xA9\x94\x9A", NULL, NULL, NULL}},
    {221,
     22,
     10,
     0,
     "f49a",
     {"\x83\x7B\x83\x93\x83\x6F\x83\x77\x8E\xA9\x94\x9A", NULL, NULL, NULL}},
    {222,
     64,
     21,
     0,
     "f49a",
     {"\x83\x7B\x83\x93\x83\x6F\x83\x77\x8E\xA9\x94\x9A", NULL, NULL, NULL}},
    {223,
     96,
     39,
     0,
     "f49a",
     {"\x83\x7B\x83\x93\x83\x6F\x83\x77\x8E\xA9\x94\x9A", NULL, NULL, NULL}},
    {224, 36, 20, 0, "f45a", {NULL, NULL, NULL, NULL}},
    {228, 57, 24, 0, "f45a", {NULL, NULL, NULL, NULL}},
    {232, 66, 40, 0, "f45a", {NULL, NULL, NULL, NULL}},
    {236, 45, 22, 0, "f45a", {NULL, NULL, NULL, NULL}},
    {240, 80, 20, 0, "f45a", {NULL, NULL, NULL, NULL}},
};
// Code (.text)
CUserDataManager *GetUserDataMan() {
    CSaveData *save = GetSaveData();
    return save != NULL ? &save->user_data : NULL;
}

CFishingTournament *GetFishTournament() {
    CUserDataManager *user = GetUserDataMan();
    return user != NULL ? &user->fish_tournament : NULL;
}

CFishAquarium *GetAquariumData() {
    CUserDataManager *user = GetUserDataMan();
    return user != NULL ? &user->aquarium : NULL;
}

int COMMON_GAGE::CheckFill() {
    int full;

    full = 1;
    if (this->max != this->now) {
        full = 0;
    }
    return full;
}

float COMMON_GAGE::GetRate() {
    if (this->max != 0.0f) {
        return this->now / this->max;
    }
    return 0.0f;
}

void COMMON_GAGE::SetFillRate(float rate) {
    this->now = this->max * rate;
}

void COMMON_GAGE::AddPoint(float point) {
    float now;
    float max;

    now = this->now + point;
    this->now = now;
    if (now <= 0.0f) {
        this->now = 0.0f;
    }
    max = this->max;
    if (max <= this->now) {
        this->now = max;
    }
}

void COMMON_GAGE::AddRate(float rate) {
    float now;
    float max;

    this->now += this->max * rate;
    now = this->now;
    if (now <= 0.0f) {
        this->now = 0.0f;
    }
    max = this->max;
    if (max <= this->now) {
        this->now = max;
    }
}

float GetCommonGageRate(COMMON_GAGE *gage) {
    if (gage != NULL) return gage->GetRate();
    return 0.0f;
}

int CalcBreedFishParam(BREEDFISH_USED *fish) {
    u16 *param = fish->param;
    int  total = 0;
    int  i;
    for (i = 0; i < 5; i++) {
        total += param[i];
    }
    return total;
}

void SetFishingGamePreEquip(CGameDataUsed *weapon) {
    FishGamePreEquip = weapon;
}

void ReEquipFishingGameWeapon(void) {
    if (FishGamePreEquip != NULL) {
        CUserDataManager *manager = GetUserDataMan();
        manager->SetChrEquip(0, FishGamePreEquip);
        FishGamePreEquip = NULL;
    }
}

int CheckFishingWeapon(CGameDataUsed *weapon) {
    return FishGamePreEquip == weapon;
}

void GameDataSwap(CGameDataUsed *a, CGameDataUsed *b, int check_fishing) {

    int            first_is_rod;
    CGameDataUsed *equipped;

    if (a == NULL || b == NULL) {
        return;
    }
    {

        CGameDataUsed spare;
        spare.CopyGameData(a);
        a->CopyGameData(b);
        b->CopyGameData(&spare);
        if (check_fishing == 1) {
            equipped = &GetUserDataMan()->chara_data[USER_CHARA_MAX].equip[0];
            if (equipped == a || equipped == b) {
                first_is_rod = a->IsFishingRod();
                if (first_is_rod != b->IsFishingRod()) {
                    if (equipped == a) {
                        SetFishingGamePreEquip(NULL);
                        if (a->IsFishingRod()) {
                            SetFishingGamePreEquip(b);
                        }
                    }
                    if (equipped == b) {
                        SetFishingGamePreEquip(NULL);
                        if (b->IsFishingRod()) {
                            SetFishingGamePreEquip(a);
                        }
                    }
                    return;
                }
            }
            if (equipped->IsFishingRod() && (a == FishGamePreEquip || b == FishGamePreEquip)) {
                if (a == FishGamePreEquip) {
                    SetFishingGamePreEquip(b);
                    return;
                }
                if (b == FishGamePreEquip) {
                    SetFishingGamePreEquip(a);
                }
            }
        }
    }
}

int CheckNowRoboUseCapacity(ROBO_DATA *robo, int *capacity) {
    int used = 0;
    for (int i = 0; i < 4; i++) {
        used += robo->parts[i].GetUseCapacity();
    }
    if (capacity != NULL) {
        *capacity = GetUserDataMan()->CheckCapacity();
    }
    return used;
}

CGameDataUsed::CGameDataUsed() {
    Init();
}

void CGameDataUsed::Init() {
    memset(this, 0, sizeof(CGameDataUsed));
}

int CGameDataUsed::CheckTypeEnableStack() {
    if (used_type == USED_ITEM_TYPE_ITEM) {
        return 1;
    }
    if (used_type == USED_ITEM_TYPE_ATTACH) {
        if (item_no == 185) {
            return 0;
        }
        return (item_no == 0x17F) ^ 1;
    }
    return 0;
}

char *CGameDataUsed::GetDataPath() {
    return GetItemFilePath(item_no, 0);
}

int CGameDataUsed::IsWhoEquip() {
    int item_type;

    if (this->used_type == USED_ITEM_TYPE_WEAPON) {
        item_type = this->item_type;
        if (item_type == 1 || item_type == 2 || item_type == 5 || item_type == 6 ||
            item_type == 7) {
            return 0;
        }
        if (item_type == 3 || item_type == 4 || item_type == 8 || item_type == 9 ||
            item_type == 0xA) {
            return 1;
        }
        return -1;
    }
    if (this->used_type == USED_ITEM_TYPE_ROBO_PART) {
        return 2;
    }
    return -1;
}

int CGameDataUsed::GetLevel() {
    if (used_type == USED_ITEM_TYPE_WEAPON) {
        return data.weapon.level;
    }
    if (used_type == USED_ITEM_TYPE_ATTACH) {
        return this->data.attach.level;
    }
    return 0;
}

int CGameDataUsed::GetPalletColor() {
    CDataWeapon *data;

    switch (this->used_type) {
    case USED_ITEM_TYPE_WEAPON:
        data = GameItemDataManage.GetWeaponData(this->item_no);
        if (data != NULL) {
            return (s8)data->pallet_color;
        }
        return 0;
    }
    return 0;
}

int CGameDataUsed::GetSpectolNo() {
    if (item_no == 185) {
        return this->data.attach.spectol_item_no;
    }
    return 0;
}

int CGameDataUsed::CheckStackRemain() {
    CDataCommon *record;

    if (CheckTypeEnableStack() == 0) {
        return 0;
    }
    record = GetCommonItemData(this->item_no);
    if (record != NULL) {
        return record->stack_num - GetNum();
    }
    return 0;
}

int CGameDataUsed::GetNum() {
    if (item_no <= 0) {
        return 0;
    }
    switch (used_type) {
    case USED_ITEM_TYPE_ITEM:
    case USED_ITEM_TYPE_UNK_4:
        return this->data.item.num;
    case USED_ITEM_TYPE_ATTACH:
        if (item_type == 0x11) {
            return 1;
        }
        if (item_type == 0x22) {
            return 1;
        }
        return this->data.attach.num;
    default:
        return 1;
    }
}

u8 CGameDataUsed::GetActiveSetNum(void) {
    return this->IsActiveSet();
}

int CGameDataUsed::AddNum(int num, int clear_empty) {
    int            current_num;
    CDataCommon   *record;
    CGameDataUsed *stacked = this;
    CGameDataUsed *attachment = this;

    if (item_no <= 0) {
        return 0;
    }
    record = GetCommonItemData(item_no);
    if (used_type != USED_ITEM_TYPE_ATTACH) {
        current_num = 1;
        if (used_type != USED_ITEM_TYPE_ITEM) {
            current_num += num;
        } else {
            stacked->data.item.num += num;
            if (stacked->data.item.num < 0) {
                stacked->data.item.num = 0;
            }
            if (record->max_num < stacked->data.item.num) {
                stacked->data.item.num = record->max_num;
            }
            current_num = stacked->data.item.num;
        }
    } else {
        attachment->data.attach.num += num;
        if (attachment->data.attach.num < 0) {
            attachment->data.attach.num = 0;
        }
        if (record->max_num < attachment->data.attach.num) {
            attachment->data.attach.num = record->max_num;
        }
        current_num = attachment->data.attach.num;
    }
    if (current_num <= 0) {
        if (clear_empty != 0) {
            Init();
        }
    }
    return current_num;
}

int CGameDataUsed::GetUseCapacity() {
    CDataRoboPart *robo_data = GameItemDataManage.GetRoboData(item_no);
    if (robo_data != NULL) {
        return robo_data->use_capacity;
    }
    return 0;
}

int CGameDataUsed::AddFishHp(int hp) {
    if (used_type == USED_ITEM_TYPE_FISH) {
        CGameDataUsed *fish = this;
        int            new_hp = fish->data.fish.hp + hp;
        if (new_hp < 0) {
            new_hp = 0;
        }
        if (new_hp > 100) {
            new_hp = 100;
        }
        fish->data.fish.hp = new_hp;
        return fish->data.fish.hp;
    }
    return 0;
}

int CGameDataUsed::Boiled() {
    static char *boiled_formats[7] = {
        " ",        "Grilled %s", "R[UNI00f4]tir %s", "Ger[UNI00f6]steter %s",
        "Cuoci %s", "Asar %s",    "Roast %s"};

    CGameDataUsed *fish = this;
    char           converted[0x40];
    char           text[0x40];
    int            value;

    sprintf(text, boiled_formats[LanguageCode], this->GetName(0));
    ConvertFontCode(text, converted);
    value = fish->data.fish.size / 100 +
            (fish->data.fish.param[0] + fish->data.fish.param[1] + fish->data.fish.param[2]) / 3;
    strcpy(fish->data.boiled.name, converted);
    fish->data.boiled.base_item_no = fish->item_no;
    fish->data.boiled.value = value + 0x14;
    fish->item_type = 0x23;
    fish->used_type = USED_ITEM_TYPE_BOILED;
    fish->item_no = 0x1AA;
    return 1;
}

u8 CGameDataUsed::IsActiveSet(void) {
    CDataCommon *item = GetCommonItemData(item_no);
    if (item != NULL) {
        return item->active_set;
    }
    return 0U;
}

void CGameDataUsed::SetName(char *name) {
    char *buffer;

    buffer = NULL;
    switch (used_type) {
    case USED_ITEM_TYPE_WEAPON:
        buffer = this->data.weapon.name;
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        buffer = this->data.robopart.name;
        break;
    case USED_ITEM_TYPE_FISH:
        buffer = this->data.fish.name;
        break;
    case USED_ITEM_TYPE_ATTACH:
        buffer = this->data.attach.name;
        break;
    }
    if ((buffer != NULL) && (strlen(name) < 0x20U)) {
        strcpy(buffer, name);
        char *default_name = GetItemMessage(item_no);
        rename_flag = 0;
        if ((default_name != 0) && (strcmp(default_name, buffer) != 0)) {
            rename_flag = 1;
        }
    }
}

char *CGameDataUsed::GetName(int mode) {
    static char *name_quotes[8][2][2] = {{{"\x81\x77", "\x81\x78"}, {"\x81\x79", "\x81\x7A"}},
                                         {{"\"", "\""}, {"\x81\x79", "\x81\x7A"}},
                                         {{"\"", "\""}, {"'", "'"}},
                                         {{"\"", "\""}, {"'", "'"}},
                                         {{"\"", "\""}, {"'", "'"}},
                                         {{"\"", "\""}, {"'", "'"}},
                                         {{"\"", "\""}, {"'", "'"}},
                                         {{"\"", "\""}, {"\x81\x79", "\x81\x7A"}}};

    static char name_buffer[0x61];

    char *name;
    int   weapon_level;
    int   digits;
    int   divisor;
    int   digit;

    memset(name_buffer, 0, 0x61);
    switch (used_type) {
    case USED_ITEM_TYPE_WEAPON:
        name = data.weapon.name;
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        name = data.robopart.name;
        break;
    case USED_ITEM_TYPE_FISH:
        name = data.fish.name;
        break;
    case USED_ITEM_TYPE_ATTACH:
        if (item_no == 185) {
            name = data.attach.name;
        } else {
            name = GetItemMessage(item_no);
        }
        break;
    case USED_ITEM_TYPE_BOILED:
        name = data.boiled.name;
        break;
    default:
        name = GetItemMessage(item_no);
        break;
    }
    if (name != NULL) {
        if (mode == 2) {
            strcpy(name_buffer, name_quotes[LanguageCode][(signed char)rename_flag][0]);
            strcat(name_buffer, name);
        }
        if (mode == 0 || mode == 1) {
            strcpy(name_buffer, name);
        }
    }
    if (mode > 0 && (used_type == USED_ITEM_TYPE_WEAPON || used_type == USED_ITEM_TYPE_ATTACH)) {
        weapon_level = GetLevel();
        if (weapon_level > 0) {
            if ((int)LanguageCode > 0) {
                strcat(name_buffer, " + %d");
                sprintf(name_buffer, name_buffer, weapon_level);
            } else {
                strcat(name_buffer, "\x81\x7B");
                digits = GetNumberKeta(weapon_level);
                if (digits > 0) {
                    do {
                        if (digits == 1) {
                            strcat(name_buffer, MenuBigNum[weapon_level]);
                            digits -= 1;
                        } else {
                            divisor = (int)pow(10.0, (double)(digits - 1));
                            digit = weapon_level / divisor;
                            strcat(name_buffer, MenuBigNum[digit]);
                            digits -= 1;
                            weapon_level -= digit * divisor;
                        }
                    } while (digits > 0);
                }
            }
        }
    }
    if (name != NULL && mode == 2) {
        strcat(name_buffer, name_quotes[LanguageCode][(signed char)rename_flag][1]);
    }
    return name_buffer;
}

void CGameDataUsed::TransToPassword(char *password, int length) {
    CGameDataUsed   *item = this;
    PackedFishBuffer buffer;
    signed char     *src;
    int              i;
    BREEDFISH_USED  *body;

    if (password != NULL) {
        memset(password, 0, 4);
        switch (used_type) {
        case USED_ITEM_TYPE_FISH: {
            body = &item->data.fish;
            memset(&buffer, 0, 14);
            buffer.fish.item_no = item->item_no;
            buffer.fish.sex = (s8)body->sex;
            buffer.fish.unk_3a = body->unk_3a;
            buffer.fish.param_3 = body->param[4];
            buffer.fish.unk_3c = body->param[3];
            buffer.fish.length = body->size;
            buffer.fish.weight = body->weight;
            buffer.fish.param_0 = body->param[0];
            buffer.fish.param_1 = body->param[1];
            buffer.fish.color_no = body->unk_16;
            buffer.fish.param_2 = body->param[2];
            buffer.fish.flags = body->flags;
            src = buffer.bytes;
            for (i = 0; i < length && i < 14; i++) {
                password[i] = src[i];
            }
            password[i] = 0;
        }
        }
    }
}

void CGameDataUsed::TransToData(char *password, int length) {
    CGameDataUsed   *item = this;
    PackedFishBuffer buffer;
    signed char     *dst;
    int              i;

    if (password != NULL) {
        switch (used_type) {
        case USED_ITEM_TYPE_FISH: {
            dst = buffer.bytes;
            for (i = 0; i < 14 && i < length; i++) {
                dst[i] = ((signed char *)password)[i];
            }
            item->item_no = buffer.fish.item_no;
            item->data.fish.sex = buffer.fish.sex;
            item->data.fish.unk_3a = buffer.fish.unk_3a;
            item->data.fish.param[4] = buffer.fish.param_3;
            item->data.fish.param[3] = buffer.fish.unk_3c;
            item->data.fish.size = buffer.fish.length;
            item->data.fish.weight = buffer.fish.weight;
            item->data.fish.param[0] = buffer.fish.param_0;
            item->data.fish.param[1] = buffer.fish.param_1;
            item->data.fish.param[2] = buffer.fish.param_2;
            item->data.fish.unk_16 = buffer.fish.color_no;
            item->data.fish.flags = buffer.fish.flags;
        }
        }
    }
}

int CGameDataUsed::DeleteNum(int num) {
    int before;
    int delta;

    if (num <= 0) {
        return 0;
    }
    before = GetNum();
    switch (this->used_type) {
    case USED_ITEM_TYPE_ITEM:
    case USED_ITEM_TYPE_ATTACH:
        delta = AddNum(-num, 1);
        break;
    default:
        delta = 0;
        Init();
        break;
    }
    return before - delta;
}

int CGameDataUsed::RemainFusion() {
    if (used_type == USED_ITEM_TYPE_WEAPON) {
        return this->data.weapon.fusion_point;
    }
    return 0;
}

int CGameDataUsed::AddFusionPoint(int point) {
    int            total;
    CGameDataUsed *weapon = this;

    if (used_type == USED_ITEM_TYPE_WEAPON) {
        total = weapon->data.weapon.fusion_point + point;
        if (total < 0) {
            total = 0;
        }
        if (weapon->IsFishingRod()) {
            if (total >= 9999) {
                total = 9999;
            }
        } else if (total >= 999) {
            total = 999;
        }
        weapon->data.weapon.fusion_point = total;
        return weapon->data.weapon.fusion_point;
    }
    return 0;
}

int CGameDataUsed::GetEffectReadType(char **effect, char **sound, int *power) {
    static char *spell_effect_names[8] = {
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x89\xCE",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x89\xCE\x83\x71\x83\x62\x83\x67",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x95\x58",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x95\x58\x83\x71\x83\x62\x83\x67",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x97\x8B",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x97\x8B\x83\x71\x83\x62\x83\x67",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x95\x97",
        "\x83\x82\x83\x6A\x83\x4A\x96\x82\x96\x40\x81\x7C\x95\x97\x83\x71\x83\x62\x83\x67"};

    int elem;

    if (used_type == USED_ITEM_TYPE_WEAPON) {
        int weapon_type = item_type;
        GetWeaponInfoData(item_no);
        if (weapon_type == 4) {
            elem = this->GetActiveElem();
            if (effect != NULL) {
                *effect = spell_effect_names[elem * 2];
            }
            if (sound != NULL) {
                *sound = spell_effect_names[elem * 2 + 1];
            }
            if (power != NULL) {
                *power = data.weapon.attribute[elem];
            }
            return elem;
        }
    }
    return 0;
}

void CGameDataUsed::GetMsgAddInfo(char **name, char **sub_name, int *value) {
    static char message_buffer[0x40];

    static char *rename_formats[8] = {"\x81\x77\x25\x73\x81\x78",
                                      "\"%s\"",
                                      "\"%s\"",
                                      "\"%s\"",
                                      "\"%s\"",
                                      "\"%s\"",
                                      "\"%s\"",
                                      "\"%s\""};

    ATTACH_USED *body;
    char        *text;

    if (value != NULL) {
        value[0] = 0;
    }
    if (name != NULL) {
        *name = NULL;
    }
    if (sub_name != NULL) {
        *sub_name = NULL;
    }
    *name = GetName(2);
    switch (used_type) {
    case USED_ITEM_TYPE_ITEM:
        if (item_no == 0x137) {
            value[0] = GetUserDataMan()->GetYarikomiMedal();
        }
        break;
    case USED_ITEM_TYPE_WEAPON:
        if (value != NULL) {
            value[0] = GetLevel();
        }
        break;
    case USED_ITEM_TYPE_ATTACH:
        body = &this->data.attach;
        if (value != NULL) {
            value[0] = 0;
            if ((s8)body->spectol_type == SPECTOL_TYPE_WEAPON) {
                value[0] = body->level;
            }
            value[1] = body->spectol_value;
        }
        if ((s8)body->spectol_type != SPECTOL_TYPE_NONE) {
            *name = body->name;
        } else {
            *name = GetItemMessage(item_no);
        }
        text = *name;
        if (text != NULL) {
            sprintf(message_buffer, rename_formats[LanguageCode], text);
            *name = message_buffer;
        }
        *sub_name = GetName(1);
        break;
    }
}

float CGameDataUsed::GetWHp(int *whp) {
    float        rate;
    COMMON_GAGE *gauge;

    rate = 1.0f;
    gauge = NULL;
    if (whp != NULL) {
        whp[0] = 0;
        whp[1] = 0;
    }
    switch (used_type) {
    case USED_ITEM_TYPE_WEAPON:
        gauge = &data.weapon.whp;
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        if (item_type == 0xD) {
            gauge = &data.weapon.abs;
        }
        if (item_type == 0xF) {
            gauge = &data.weapon.whp;
        }
        break;
    }
    if (gauge != NULL) {
        if (whp != NULL) {
            whp[0] = GetDispVolumeForFloat(gauge->now);
            whp[1] = (int)gauge->max;
        }
        rate = gauge->GetRate();
    }
    return rate;
}

int CGameDataUsed::IsRepair() {
    switch (this->used_type) {
    case USED_ITEM_TYPE_WEAPON:
        if ((float)GetDispVolumeForFloat(this->data.weapon.whp.now) < this->data.weapon.whp.max) {
            return 1;
        }
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        if (this->item_type == 0xD &&
            (float)GetDispVolumeForFloat(this->data.weapon.abs.now) < this->data.weapon.abs.max) {
            return 1;
        }
        if (this->item_type == 0xF &&
            (float)GetDispVolumeForFloat(this->data.weapon.whp.now) < this->data.weapon.whp.max) {
            return 1;
        }
        break;
    }
    return 0;
}

int CGameDataUsed::Repair(int point) {
    COMMON_GAGE *gauge;

    gauge = NULL;
    switch (this->used_type) {
    case USED_ITEM_TYPE_WEAPON:
        gauge = &this->data.weapon.whp;
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        if (this->item_type == 0xD) {
            gauge = &this->data.weapon.abs;
        }
        if (this->item_type == 0xF) {
            gauge = &this->data.weapon.whp;
        }
        break;
    }
    if (gauge != NULL) {
        gauge->AddPoint((float)point);
    }
    return 1;
}

int CGameDataUsed::GetEnableRepairItemNo() {
    if (item_type == 1 || item_type == 3 || item_type == 0xD) {
        return 0x126;
    }
    if (item_type == 2) {
        return 0x12A;
    }
    if (item_type == 4) {
        return 0x160;
    }
    if (item_type == 0xF) {
        return 0x17D;
    }
    return 0;
}

s32 CGameDataUsed::IsEnableUseRepair(s32 item_no) {
    return item_no == GetEnableRepairItemNo();
}

int CGameDataUsed::GetRoboInfoType() {
    CDataRoboPart *info = GetRoboPartInfoData(item_no);
    if (info == NULL) {
        return -1;
    }
    if (item_type == 0xD) {
        return info->info_type_d;
    }
    if (item_type == 0xE) {
        return info->info_type_e;
    }
    return -1;
}

void CGameDataUsed::GetRoboJointName(char *name) {
    CDataRoboPart *record = GetRoboPartInfoData(item_no);

    if (record == NULL || name == NULL) {
        return;
    }
    if (item_type == 0xD) {
        sprintf(name, "body%d", record->GetOffsetNo());
    }
    if (item_type == 0xC) {
        sprintf(name, "arm%d", record->GetOffsetNo());
    }
}

void CGameDataUsed::GetRoboSoundFileName(char *name) {
    CDataRoboPart *record = GetRoboPartInfoData(item_no);

    if (record == NULL || name == NULL) {
        return;
    }
    if (item_type == 0xD) {
        int sound_no = record->GetOffsetNo() + 0x27;
        if (sound_no < 0x28 || sound_no > 0x32) {
            sound_no = 0x28;
        }
        sprintf(name, "CH_0%d", sound_no);
    }
}

int CGameDataUsed::IsBroken() {
    int hp[2];

    if (item_type == 0xF) {
        GetWHp(hp);
        if (hp[0] <= 0) {
            return 1;
        }
    }
    return 0;
}

int CGameDataUsed::IsLevelUp() {

    switch (this->used_type) {
    case USED_ITEM_TYPE_WEAPON:
        if (this->data.weapon.level < 99) {
            if (this->data.weapon.abs.max <=
                (float)GetDispVolumeForFloat(this->data.weapon.abs.now)) {
                return 1;
            }
        }
        break;
    }
    return 0;
}

void CGameDataUsed::LevelUp() {
    static s8 whp_bonus[10] = {1, 1, 1, 1, 1, 2, 2, 2, 3, 3};

    CUserDataManager *manager = GetUserDataMan();
    CDataWeapon      *info = GetWeaponInfoData(item_no);
    int               party_chara;
    WEAPON_USED      *block;
    float             rate;
    float             new_point;
    int               favoured;
    int               found;
    int               tries;
    int               slot;
    int               i;

    if (manager == NULL || info == NULL) {
        return;
    }
    {
        party_chara = manager->NowPartyCharaID();
        block = &data.weapon;
        rate = block->whp.GetRate();
        block->whp.max += (float)whp_bonus[GetRandI(10)];
        if (255.0f <= block->whp.max) {
            block->whp.max = 255.0f;
        }
        new_point = block->whp.max * rate;
        if (new_point > block->whp.now) {
            block->whp.now = new_point;
        }
        if (block->level < 5) {
            if (block->status[0] < 100) {
                block->status[0] = block->status[0] + 2;
            } else {
                block->status[0] = block->status[0] + 3;
            }
        } else {
            block->status[0] = block->status[0] + 1;
        }
        block->status[1] = block->status[1] + 1;
        block->abs.now = 0.0f;
        block->abs.max = (float)(info->levelup_exp + info->levelup_exp / 2 * block->level);
        AddFusionPoint(info->fusion_point);
        favoured = 0;
        if (party_chara == 1) {
            if (item_type == 1) {
                favoured = 1;
            }
        }
        if (party_chara == 0xF) {
            if (item_type == 3) {
                favoured = 1;
            }
        }
        if (party_chara == 0x10) {
            if (item_type == 2) {
                favoured = 1;
            }
        }
        if (party_chara == 0x1A) {
            if (item_type == 4) {
                favoured = 1;
            }
        }
        if (favoured == 1) {
            AddFusionPoint(1);
            CheckParamLimmit();
            found = 0;
            tries = 0;
            do {
                slot = GetRandI(0x11) % 8;
                if (block->attribute[slot] < info->attribute_max[slot]) {
                    found = 1;
                    block->attribute[slot] = block->attribute[slot] + 1;
                }
                tries++;
            } while (tries < 0x80 && found == 0);
            if (found <= 0 && tries >= 0x80) {
                for (i = 0; i < 8; i++) {
                    if (block->attribute[i] < info->attribute_max[i]) {
                        block->attribute[i] = block->attribute[i] + 1;
                        break;
                    }
                }
            }
        }
        block->level = block->level + 1;
        if (block->level > 99) {
            block->level = 99;
        }
        CheckParamLimmit();
    }
}

s32 CGameDataUsed::IsTrush(void) {
    s32 is_rubbish = 0;
    CDataCommon *item = GetCommonItemData(item_no);
    if (item != NULL && (item->attribute & ITEM_ATTRIBUTE_TRUSH)) {
        is_rubbish = 1;
    }
    if (used_type == USED_ITEM_TYPE_FISH) {
        if (data.fish.flags & BREEDFISH_FLAG_ELECTRIC) {
            is_rubbish = 0;
        }
    }
    return is_rubbish;
}

int CGameDataUsed::IsSpectolTrans() {
    CDataCommon *record = GetCommonItemData(item_no);
    if (record != NULL && (record->attribute & ITEM_ATTRIBUTE_SPECTOL_TRANS)) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", ToSpectolTrans__13CGameDataUsedFP13CGameDataUsedi);
void CGameDataUsed::GetStatusParam(short *status) {
    if (status != NULL) {
        if (used_type == USED_ITEM_TYPE_WEAPON) {
            status[0] = data.weapon.status[0];
            status[1] = data.weapon.status[1];
            status[2] = data.weapon.attribute[0];
            status[3] = data.weapon.attribute[1];
            status[4] = data.weapon.attribute[2];
            status[5] = data.weapon.attribute[3];
            status[6] = data.weapon.attribute[4];
            status[7] = data.weapon.attribute[5];
            status[8] = data.weapon.attribute[6];
            status[9] = data.weapon.attribute[7];
        } else if (used_type == USED_ITEM_TYPE_ATTACH) {
            CGameDataUsed *attachment = this;
            status[0] = attachment->data.attach.status[0];
            status[1] = attachment->data.attach.status[1];
            status[2] = attachment->data.attach.attribute[0];
            status[3] = attachment->data.attach.attribute[1];
            status[4] = attachment->data.attach.attribute[2];
            status[5] = attachment->data.attach.attribute[3];
            status[6] = attachment->data.attach.attribute[4];
            status[7] = attachment->data.attach.attribute[5];
            status[8] = attachment->data.attach.attribute[6];
            status[9] = attachment->data.attach.attribute[7];
        } else if (used_type == USED_ITEM_TYPE_ROBO_PART) {

            status[0] = data.weapon.level;
            status[1] = data.weapon.status[0];
            status[2] = data.weapon.status[1];
            status[3] = data.weapon.attribute[0];
            status[4] = data.weapon.attribute[1];
            status[5] = data.weapon.attribute[2];
            status[6] = data.weapon.attribute[3];
            status[7] = data.weapon.attribute[4];
            status[8] = data.weapon.attribute[5];
            status[9] = data.weapon.attribute[6];
        }
    }
}

void CGameDataUsed::GetStatusParam(short *status, float time) {
    this->GetStatusParam(status);
    if (this->item_no == 0x38) {
        if (GetTimeBand(time) == 2) {
            *status = *status + (*status >> 1);
        } else {
            *status = *status >> 1;
        }
    }
}

int CGameDataUsed::IsBuildUp(int *num, int *weapon_no, int *enable) {
    WEAPON_USED *weapon;
    CDataWeapon *info;
    CDataWeapon *target;
    float        mine[16];
    float        other[16];
    int          built_up;
    int          found;
    int          i;
    int          k;
    int          ok;

    built_up = 0;
    found = 0;
    if (this->used_type == USED_ITEM_TYPE_WEAPON) {
        weapon = &this->data.weapon;
        info = GetWeaponInfoData(this->item_no);
        if (info == NULL) {
            return 0;
        }

        mine[0] = weapon->status[0];
        mine[1] = weapon->attribute[0];
        mine[2] = weapon->attribute[1];
        mine[3] = weapon->attribute[2];
        mine[4] = weapon->attribute[3];
        mine[5] = weapon->attribute[4];
        mine[6] = weapon->attribute[5];
        mine[7] = weapon->attribute[6];
        mine[8] = weapon->attribute[7];
        for (i = 0; i < 3; i++) {
            target = GetWeaponInfoData(info->buildup_weapon[i]);
            if (target != NULL) {
                other[0] = target->status[0];
                other[1] = target->attribute[0];
                other[2] = target->attribute[1];
                other[3] = target->attribute[2];
                other[4] = target->attribute[3];
                other[5] = target->attribute[4];
                other[6] = target->attribute[5];
                other[7] = target->attribute[6];
                other[8] = target->attribute[7];
                ok = 1;
                for (k = 0; k < 9; k++) {
                    other[k] *= 0.9f;
                    if (mine[k] < other[k]) {
                        ok = 0;
                        break;
                    }
                }
                found++;
                if (weapon_no != NULL) {
                    weapon_no[i] = info->buildup_weapon[i];
                }
                if (enable != NULL) {
                    enable[i] = ok;
                }
                if (ok != 0) {
                    built_up++;
                }
            }
        }
    }
    if (num != NULL) {
        *num = found;
    }
    return built_up;
}

int CGameDataUsed::IsFishingRod() {
    if (this->item_no == 0x12E || this->item_no == 0x12F) {
        return 1;
    }
    return 0;
}

int CGameDataUsed::GetActiveElem() {
    CGameDataUsed *weapon = this;
    int            best;
    int            i;

    if (this->used_type == USED_ITEM_TYPE_WEAPON) {
        best = 0;
        for (i = 1; i < 4; i++) {
            if (weapon->data.weapon.attribute[best] < weapon->data.weapon.attribute[i]) {
                best = i;
            }
        }
        return best;
    }
    return -1;
}

char CGameDataUsed::GetAttackType() {
    CDataWeapon *info;

    if (this->used_type == USED_ITEM_TYPE_WEAPON) {
        info = GetWeaponInfoData(this->item_no);
        if (info != NULL) {
            return (s8)info->attack_type;
        }
    }
    return this->used_type == USED_ITEM_TYPE_ROBO_PART ? this->GetRoboInfoType() : -1;
}

int CGameDataUsed::GetModelNo(void) {
    if (used_type == USED_ITEM_TYPE_WEAPON) {
        CDataWeapon *weapon = GetWeaponInfoData(item_no);
        if (weapon != NULL) {
            return (s8)weapon->model_no;
        }
        return -1;
    }
    return -1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", GetMainCharaModelName__FiPci);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", CheckParamLimmit__13CGameDataUsedFv);
void CGameDataUsed::TimeCheck(int time) {
    int time_left;

    if (used_type == USED_ITEM_TYPE_FISH) {
        CGameDataUsed *fish = this;
        time_left = (int)(fish->data.fish.timer - time);
        if (time_left < 0) {
            time_left = 0;
        }
        fish->data.fish.timer = (u16)time_left;
    }
}

int CGameDataUsed::GetGiftBoxItemNum() {
    int count = 0;
    if (used_type == USED_ITEM_TYPE_GIFT_BOX) {
        for (int i = 0; i < 3; i++) {
            if (this->data.giftbox.item_no[i] > 0) {
                count++;
            }
        }
    }
    return count;
}

int CGameDataUsed::SetGiftBoxItem(int item_no, int index) {
    CGameDataUsed *box = this;
    int            result;
    int            i;

    result = -1;
    if (this->used_type == USED_ITEM_TYPE_GIFT_BOX) {
        if (index < 0) {
            for (i = 0; i < 3; i++) {
                if (box->data.giftbox.item_no[i] <= 0) {
                    result = i;
                    box->data.giftbox.item_no[i] = item_no;
                    break;
                }
            }
        } else {
            box->data.giftbox.item_no[index] = item_no;
        }
    }
    return result;
}

int CGameDataUsed::GetGiftBoxItemNo(int index) {
    if (used_type == USED_ITEM_TYPE_GIFT_BOX) {
        if (0 <= index && index < 3) {
            return data.giftbox.item_no[index];
        }
    }
    return 0;
}

int CGameDataUsed::GetGiftBoxSameItemNum(int item_no) {
    if (used_type != USED_ITEM_TYPE_GIFT_BOX) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (this->data.giftbox.item_no[i] == item_no) {
            count++;
        }
    }
    return count;
}

void CGameDataUsed::CopyGameData(CGameDataUsed *src) {
    ROBO_DATA *robo;
    int        old_max;

    if (src != NULL) {
        memcpy(this, src, sizeof(CGameDataUsed));
        robo = &GetUserDataMan()->robo_data;
        if (robo != NULL && &robo->parts[2] == this) {
            old_max = (int)(robo->hp.max);
            robo->hp.max = this->data.weapon.whp.max;
            if ((float)old_max <= 0.0f) {
                robo->hp.now = robo->hp.max;
            }
            if (robo->hp.max < robo->hp.now) {
                robo->hp.now = robo->hp.max;
            }
        }
    }
}

int CGameDataUsed::CopyDataWeapon(int item_no) {
    CDataWeapon *record;
    char        *message;
    float        durability;
    WEAPON_USED *weapon;

    record = GameItemDataManage.GetWeaponData(item_no);
    if (record == NULL) {
        return 0;
    }
    this->used_type = USED_ITEM_TYPE_WEAPON;
    this->item_no = item_no;
    this->item_type = GetItemDataType(item_no);
    weapon = &this->data.weapon;
    this->data.weapon.level = 0;
    durability = record->durability;
    this->data.weapon.whp.max = durability;
    this->data.weapon.whp.now = durability;
    this->data.weapon.abs.now = 0.0f;
    this->data.weapon.abs.max = record->levelup_exp;
    this->data.weapon.status[0] = record->status[0];
    this->data.weapon.status[1] = record->status[1];
    this->data.weapon.attribute[0] = record->attribute[0];
    this->data.weapon.attribute[1] = record->attribute[1];
    this->data.weapon.attribute[2] = record->attribute[2];
    this->data.weapon.attribute[3] = record->attribute[3];
    this->data.weapon.attribute[4] = record->attribute[4];
    this->data.weapon.attribute[5] = record->attribute[5];
    this->data.weapon.attribute[6] = record->attribute[6];
    this->data.weapon.attribute[7] = record->attribute[7];
    this->data.weapon.fusion_point = record->unk_38;
    this->data.weapon.special = record->special;
    this->data.weapon.unk_2e = 0;
    this->data.weapon.unk_30 = 0;
    message = GetItemMessage(item_no);
    if (message != NULL) {
        strcpy(weapon->name, message);
    }
    this->rename_flag = 0;
    return 1;
}

int CGameDataUsed::CopyDataAttach(int item_no) {
    CDataAttach   *data = GameItemDataManage.GetAttachData(item_no);
    CGameDataUsed *attachment = this;

    if (data == NULL) {
        return 0;
    }
    if (item_no == this->item_no) {
        if (CheckTypeEnableStack() != 0) {
            AddNum(1, 1);
            return 1;
        }
    }
    used_type = USED_ITEM_TYPE_ATTACH;
    this->item_no = (short)item_no;
    item_type = GetItemDataType(item_no);

    attachment->data.attach.status[0] = data->status[0];
    attachment->data.attach.status[1] = data->status[1];
    attachment->data.attach.attribute[0] = data->attribute[0];
    attachment->data.attach.attribute[1] = data->attribute[1];
    attachment->data.attach.attribute[2] = data->attribute[2];
    attachment->data.attach.attribute[3] = data->attribute[3];
    attachment->data.attach.attribute[4] = data->attribute[4];
    attachment->data.attach.attribute[5] = data->attribute[5];
    attachment->data.attach.attribute[6] = data->attribute[6];
    attachment->data.attach.attribute[7] = data->attribute[7];
    attachment->data.attach.special = 0;
    attachment->data.attach.special |= data->special;
    attachment->data.attach.num = 1;
    return 1;
}

int CGameDataUsed::CopyDataItem(int item_no) {
    CDataCommon *record;
    ITEM_USED       *stack;

    record = GetCommonItemData(item_no);
    if (record == NULL) {
        return 0;
    }
    stack = &this->data.item;
    if (this->item_no == item_no) {
        if (this->CheckStackRemain() > 0) {
            stack->num = stack->num + 1;
        }
        return 1;
    }
    this->item_type = record->type;
    this->used_type = ConvertUsedItemType(this->item_type);
    this->item_no = item_no;
    stack->num = 1;
    stack->unk_2 = 0;
    return 1;
}

int CGameDataUsed::CopyDataFish(int item_no) {
    CGameDataUsed  *fish = this;
    CDataBreedFish *record;
    char           *message;
    float           value;

    record = GetBreedFishInfoData(item_no);
    if (record == NULL) {
        return 0;
    }
    this->used_type = USED_ITEM_TYPE_FISH;
    this->item_no = item_no;
    this->item_type = GetItemDataType(item_no);
    message = GetItemMessage(item_no);
    if (message != NULL) {
        strcpy(fish->data.fish.name, message);
    }
    value = record->size / 2.0f + GetRandF(30.0f);
    fish->data.fish.size = (u16)(value - GetRandF(10.0f));
    value = 400.0f + GetRandF(500.0f);
    fish->data.fish.weight = (u16)(value + GetRandF(500.0f));
    fish->data.fish.sex = GetRandI(2);
    fish->data.fish.unk_1c = GetRandI(4);
    fish->data.fish.unk_16 = GetRandI(4);
    fish->data.fish.hp = 100;
    fish->data.fish.fatigue = 0;
    fish->data.fish.param[4] = record->unk_6;
    fish->data.fish.param[0] = record->unk_a;
    fish->data.fish.param[1] = record->unk_c;
    fish->data.fish.param[2] = record->unk_e;
    fish->data.fish.param[3] = record->unk_8;
    fish->data.fish.unk_36 = GetRandI(0x33) + 0xC8;
    fish->data.fish.unk_35 = 0;
    fish->data.fish.timer = 0;
    fish->data.fish.flags = 0;
    fish->data.fish.unk_3c = GetRandI(0x100);
    fish->data.fish.unk_3d = 0;
    return 1;
}

s32 CGameDataUsed::CopyDataGiftBox(s32 item_no) {
    if (GetItemInfoData(item_no) == NULL) {
        return 0;
    }
    used_type = USED_ITEM_TYPE_GIFT_BOX;
    this->item_no = item_no;
    item_type = GetItemDataType(item_no);
    data.giftbox.item_no[2] = 0;
    data.giftbox.item_no[1] = 0;
    data.giftbox.item_no[0] = 0;
    return 1;
}

int CGameDataUsed::CopyDataItem(CGameDataUsed *src) {
    CDataCommon *record;
    int          total;

    if (src == NULL) {
        return 0;
    }
    if (0 < this->item_no && this->item_no == src->item_no) {
        if (CheckTypeEnableStack() != 0) {
            record = GetCommonItemData(this->item_no);
            total = GetNum() + src->GetNum();
            if ((short)total > record->stack_num) {
                return 0;
            }
            AddNum(src->GetNum(), 1);
            src->Init();
            return 1;
        }
    }
    GameDataSwap(this, src, 0);
    return 1;
}

int CGameDataUsed::CopyDataRoboPart(int item_no) {
    CDataRoboPart *record;
    char          *message;
    ROBOPART_USED *part;
    float          energy;
    float          hp;

    record = GameItemDataManage.GetRoboData(item_no);
    if (record == NULL) {
        return 0;
    }
    this->used_type = USED_ITEM_TYPE_ROBO_PART;
    this->item_no = item_no;
    this->item_type = GetItemDataType(item_no);
    part = &this->data.robopart;
    energy = record->unk_6;
    this->data.robopart.gage1.max = energy;
    this->data.robopart.gage1.now = energy;
    hp = record->unk_2;
    this->data.robopart.gage0.max = hp;
    this->data.robopart.gage0.now = hp;
    this->data.robopart.defence = record->unk_1c;
    this->data.robopart.unk_26 = record->unk_4;
    this->data.robopart.status[0] = record->unk_8;
    this->data.robopart.status[1] = record->unk_a;
    this->data.robopart.status[2] = record->unk_c[0];
    this->data.robopart.status[3] = record->unk_c[1];
    this->data.robopart.status[4] = record->unk_c[2];
    this->data.robopart.status[5] = record->unk_c[3];
    this->data.robopart.status[6] = record->unk_c[4];
    this->data.robopart.status[7] = record->unk_c[5];
    this->data.robopart.status[8] = record->unk_c[6];
    this->data.robopart.status[9] = record->unk_c[7];
    message = GetItemMessage(item_no);
    if (message != NULL) {
        strcpy(part->name, message);
    }
    return 1;
}

void CFishAquarium::Initialize() {
    int i;
    unk_0 = 0;
    unk_2 = 0;
    for (i = 0; i < 6; i++) {
        fish_tank[i].Init();
    }
    for (i = 0; i < 4; i++) {
        sub_tank[i].Init();
    }
    for (i = 0; i < 2; i++) {
        breed_tank[i].Init();
    }
    unk_518 = 0;
    last_time = 0;
    last_day = 0;
    last_hour = 0;
}

CGameDataUsed *CFishAquarium::GetAquariumFishTop(int tank) {
    if (tank == 0) {
        return fish_tank;
    }
    if (tank == 1) {
        return sub_tank;
    }
    if (tank == 2) {
        return breed_tank;
    }
    return 0;
}

int CFishAquarium::SearchAqua1NotUsed(int tank) {
    CGameDataUsed *slot = GetAquariumFishTop(tank);
    if (slot == 0) {
        return -1;
    }
    for (int i = 0; i < aquarium_fish_maxtbl[tank]; i++, slot++) {
        if (slot->item_no <= 0) {
            return i;
        }
    }
    return -1;
}

void CFishAquarium::FishIntoAquarium(int tank, int index, CGameDataUsed *fish) {
    CGameDataUsed *entry;
    if (tank < 0 || tank > 3) {
        return;
    }
    entry = 0;
    if (tank == 0) {
        if (index >= 0 && index < 6) {
            entry = &fish_tank[index];
        }
    } else if (tank == 1) {
        if (index >= 0 && index < 4) {
            entry = &sub_tank[index];
        }
    } else if (tank == 2) {
        if (index >= 0 && index < 2) {
            entry = &breed_tank[index];
        }
    }
    if (entry == 0) {
        return;
    }
    entry->CopyGameData(fish);
    if (tank == 1) {
        entry->data.fish.tank_day = GetMainScene()->day;
        entry->data.fish.tank_hour = GetMainScene()->time;
    }
}

int CFishAquarium::GetAquariumFishNum(int tank) {
    int            capacity = aquarium_fish_maxtbl[tank];
    CGameDataUsed *slot = GetAquariumFishTop(tank);
    if (slot == 0) {
        return 0;
    }
    int count = 0;
    int i = 0;

    for (; i < capacity; i++, slot++) {
        if (0 < slot->item_no) {
            count++;
        }
    }
    return count;
}

int CFishAquarium::CheckHaigouTankSex(CGameDataUsed *fish) {
    int i = 0;
    if (fish == 0) {
        return 0;
    }
    for (; i < 2; i++) {
        if (breed_tank[i].item_no > 0 &&
            (s8)breed_tank[i].data.fish.sex == (s8)fish->data.fish.sex) {
            return 0;
        }
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", RefreshParam__13CFishAquariumFv);
int GetShiledKitLimmit(int item_no) {
    static u8 shield_kit_limits[7] = {3, 6, 9, 12, 15, 18, 21};

    int index = item_no - 0xF6;
    if (index < 0) {
        index = 0;
    }
    if (index > 6) {
        index = 6;
    }
    return shield_kit_limits[index];
}

float ROBO_DATA::AddPoint(float point) {
    hp.AddPoint(point);
    return hp.GetRate();
}

s32 ROBO_DATA::GetDefenceVol(void) {
    return parts[1].data.robopart.defence + (shield_kit_num << 2);
}

BASE_MONSTER_TBL *GetMonsterBaseInfo(int monster_id) {
    return GetMonsterTable(monster_id);
}

MOS_HENGE_PARAM *GetMonsterHengeParam(int monster_id) {
    for (int i = 0; i < 57; i++) {
        if (mos_henge_param[i].monster_id == monster_id) {
            return &mos_henge_param[i];
        }
    }
    return 0;
}

int MOS_CHANGE_PARAM::GetAttackVol(int monster_id) {
    float scale;
    int   value;
    if (monster_id < 0) {
        monster_id = this->monster_id;
    }
    scale = 1.0f + 2.0f * ((float)level / 98.0f);
    MOS_HENGE_PARAM *param = GetMonsterHengeParam(monster_id);
    value = 0;
    if (param != 0) {
        value = (int)((float)param->attack * scale);
    }
    if (value > 999) {
        value = 999;
    }
    return value;
}

int MOS_CHANGE_PARAM::GetDefenceVol(int monster_id) {
    int value;
    if (monster_id < 0) {
        monster_id = this->monster_id;
    }
    MOS_HENGE_PARAM *param = GetMonsterHengeParam(monster_id);
    value = 0;
    if (param != 0) {
        value = (int)((float)param->defence + (float)(GetDegreeLevel() * 2));
    }
    if (value > 999) {
        value = 999;
    }
    return value;
}

s32 MOS_CHANGE_PARAM::CheckClassChange(void) {
    if (class_level >= 3) {
        return 0;
    }
    return class_level < level / 25;
}

s32 MOS_CHANGE_PARAM::GetDegreeLevel(void) {
    s32 degree = level / 6;
    if (degree > 15) {
        degree = 15;
    }
    return degree;
}

int MOS_CHANGE_PARAM::LevelUp() {
    if (abs.CheckFill() != 0) {
        if (level < 98) {
            abs.now = 0;
            int bonus = 0;
            if (level > 49) {
                bonus = (level - 49) * 25;
            }
            abs.max = (float)(level * 100 + 100 + bonus);
            level = level + 1;
            return 1;
        }
    }
    return 0;
}

void CMonsterBox::Initialize() {
    memset(this, 0, sizeof(*this));
    for (int i = 0; i < 64; i++) {
        monster[i].no = i;
        monster[i].hp.max = 64.0f;
        monster[i].hp.now = 64.0f;
        monster[i].abs.max = 100.0f;
    }
}

MOS_CHANGE_PARAM *CMonsterBox::GetMonsterBajjiData(int no) {
    if (no <= 0 || no >= 64) {
        return 0;
    }
    return &monster[no - 1];
}

MOS_CHANGE_PARAM *CMonsterBox::GetMonsterBajjiDataByMonsterID(int monster_id) {
    return GetMonsterBajjiData(get_gajji_id_from_monster_progress_table(monster_id, 0) + 1);
}

void CMonsterBox::EnableChange(int no) {
    int               level[8];
    MOS_CHANGE_PARAM *record = GetMonsterBajjiData(no);
    if (record != 0) {
        record->enable = 1;
        record->progress = get_default_monster_progresstbl(no - 1);
        get_monster_tbl_bajjilevel(level, no - 1, -1, 0);
        record->monster_id = level[0];
    }
}

int CMonsterBox::IsChange(int no) {
    MOS_CHANGE_PARAM *record = GetMonsterBajjiData(no);
    if (record != 0) {
        return (u8)record->enable;
    }
    return 0;
}

void CMonsterBox::AllCure() {
    for (int i = 0; i < 64; i++) {
        monster[i].hp.SetFillRate(1.0f);
    }
}

/**
 * Finds the fishing-record index of an item number.
 */
static int GetConvertIndexFromFishNo(int fish_no) {
    for (int index = 0; 0 < fish_record_dataindex_convert[index]; index++) {
        if (fish_no == fish_record_dataindex_convert[index]) {
            return index;
        }
    }
    return -1;
}

CFishingRecord::CFishingRecord() {
    memset(this, 0, sizeof(*this));
}

FISH_RECORD *CFishingRecord::GetFishRecord(int item_no) {
    int index = GetConvertIndexFromFishNo(item_no);
    if (index < 0) {
        return 0;
    }
    return &record[index];
}

int CFishingRecord::CheckRecordFish(int item_no, float size, float weight) {
    FISH_RECORD *record = GetFishRecord(item_no);
    int          result;
    if (record == 0) {
        return 0;
    }
    result = 0;
    if (record->size < size) {
        record->prev_size = record->size;
        result |= 1;
        record->size = size;
    }
    if (record->weight < weight) {
        record->prev_weight = record->weight;
        result |= 2;
        record->weight = weight;
    }
    record->num = record->num + 1;
    if (record->num > 999999) {
        record->num = 999999;
    }
    return result;
}

void CFishingTournament::Initialize(void) {
    memset(this, 0, sizeof(*this));
}

void CFishingTournament::ResetRecord(void) {
    memset(entry, 0, sizeof(entry));
}

void CFishingTournament::EntryFish(int item_no, int size, int weight) {
    for (int i = 0; i < 10; i++) {
        if (entry[i].item_no <= 0) {

            entry[i].item_no = item_no;
            entry[i].size = size;
            entry[i].weight = weight;
            break;
        }
    }
    EntryRemain();
}

int CFishingTournament::EntryRemain() {
    int used = 0;
    int i = 0;

    for (; i < 10; i++) {
        if (entry[i].item_no > 0) {
            used += 1;
        }
    }
    return 10 - used;
}

FISH_TOURNAMENT_ENTRY *CFishingTournament::GetRecord(int index) {
    if (index < 0 || index >= 10) {
        return 0;
    }
    return &entry[index];
}

void CFishingTournament::SetRank(s32 rank) {
    if (rank < 0) {
        rank = 0;
    }
    if (rank > 100) {
        rank = 100;
    }
    this->rank = rank;
}

void CFishingTournament::SortRecord() {
    FISH_TOURNAMENT_ENTRY temp;
    int                   i = 0;
    for (; i < 10; i++) {
        FISH_TOURNAMENT_ENTRY *slot = &entry[i];
        if (slot->item_no > 0) {
            int j = i;
            for (; j < 10; j++) {
                if (slot->weight < entry[j].weight) {
                    memcpy(&temp, slot, sizeof(FISH_TOURNAMENT_ENTRY));
                    memcpy(slot, &entry[j], sizeof(FISH_TOURNAMENT_ENTRY));
                    slot = &entry[j];
                    memcpy(slot, &temp, sizeof(FISH_TOURNAMENT_ENTRY));
                    i = -1;
                    break;
                }
            }
        }
    }
}

s32 CFishingTournament::CalcTopWeight(void) {
    this->SortRecord();
    return entry[2].weight + (entry[0].weight + entry[1].weight);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", Initialize__16CUserDataManagerFv);
void CUserDataManager::RefreshParam() {
    RefreshNPCStatus(0);
    int now = (int)GetSaveData()->play_time;
    int elapsed = (int)(now - last_refresh_time);
    for (int slot = 0; slot < 150; slot++) {
        used_data[slot].TimeCheck(elapsed);
    }
    unk_451d0 = unk_451d0 % 0x534;
    aquarium.RefreshParam();
    last_refresh_time = now;
}

CGameDataUsed *CUserDataManager::GetUsedDataPtr(int index) {
    if (index < 0 || index >= 150) {
        return 0;
    }
    return &used_data[index];
}

CHARA_DATA *CUserDataManager::GetCharaDataPtr(int chara) {
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        return &chara_data[chara];
    }
    return 0;
}

COMMON_GAGE *CUserDataManager::GetCharaHpGage(int chara) {
    unsigned int id = chara;
    if (id <= USER_CHARA_MONICA || id == USER_CHARA_MONSTER) {
        if (id == USER_CHARA_MONSTER) {
            id = USER_CHARA_MONICA;
        }
        return &chara_data[id].hp;
    }
    if (id == USER_CHARA_ROBO) {
        return &robo_data.hp;
    }
    return 0;
}

int CUserDataManager::AddHp(int chara, int hp) {
    COMMON_GAGE *gage = GetCharaHpGage(chara);
    if (gage != 0) {
        gage->AddPoint((float)hp);
        return (int)(gage->now);
    }
    return 0;
}

float CUserDataManager::GetHp(int chara) {
    COMMON_GAGE *gage = GetCharaHpGage(chara);
    if (gage != 0) {
        return (float)(int)(gage->now);
    }
    return 0.0f;
}

float CUserDataManager::AddHp_Rate(int chara, float rate) {
    COMMON_GAGE *gage = GetCharaHpGage(chara);
    if (gage == 0) {
        return 0.0f;
    }
    gage->AddRate(rate);
    if (gage->now < 1.0f) {
        gage->now = 1.0f;
    }
    return gage->GetRate();
}

COMMON_GAGE *CUserDataManager::GetWHpGage(int chara, int weapon) {
    if (chara == USER_CHARA_ROBO) {
        return &robo_data.parts[0].data.robopart.gage1;
    }
    if (chara == USER_CHARA_MONSTER) {
        MOS_CHANGE_PARAM *monster = monster_box.GetMonsterBajjiData(monster_id);
        if (monster != 0) {
            return &monster->hp;
        }
    }
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        if (weapon < 0 || weapon >= 2) {
            return 0;
        }
        return &chara_data[chara].equip[weapon].data.weapon.whp;
    }
    return 0;
}

COMMON_GAGE *CUserDataManager::GetAbsGage(int chara, int weapon) {
    if (chara == USER_CHARA_ROBO) {
        return &robo_data.abs;
    }
    if (chara == USER_CHARA_MONSTER) {
        MOS_CHANGE_PARAM *monster = monster_box.GetMonsterBajjiData(monster_id);
        if (monster != 0) {
            return &monster->abs;
        }
    }
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        if (weapon < 0 || weapon >= 2) {
            return 0;
        }
        return &chara_data[chara].equip[weapon].data.weapon.abs;
    }
    return 0;
}

int CUserDataManager::AddWhp(int chara, int weapon, int whp) {
    COMMON_GAGE *gage = GetWHpGage(chara, weapon);
    if (gage == 0) {
        return 0;
    }
    gage->AddPoint((float)whp);
    return (int)(gage->now);
}

int CUserDataManager::GetWhp(int chara, int weapon, int *max) {
    COMMON_GAGE *gage = GetWHpGage(chara, weapon);
    if (gage == 0) {
        return 0;
    }
    if (max != 0) {
        *max = (int)(gage->max);
    }
    return (int)(gage->now);
}

int CUserDataManager::AddAbs(int chara, int weapon, int abs) {
    if (chara == USER_CHARA_ROBO) {
        AddRoboAbs((float)abs);
        return (int)(GetRoboAbs());
    }
    COMMON_GAGE *gage = GetAbsGage(chara, weapon);
    if (gage == 0) {
        return 0;
    }
    gage->AddPoint((float)abs);
    return (int)(gage->now);
}

int CUserDataManager::GetAbs(int chara, int weapon, int *max) {
    if (chara == USER_CHARA_ROBO) {
        if (max != 0) {
            *max = 0;
        }
        return (int)(GetRoboAbs());
    }
    COMMON_GAGE *gage = GetAbsGage(chara, weapon);
    if (gage == 0) {
        return 0;
    }
    if (max != 0) {
        *max = (int)(gage->max);
    }
    return (int)(gage->now);
}

void CUserDataManager::JoinPartyMember(int chara) {
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_MONSTER) {
        return;
    }
    party_member |= (1 << chara) & 0xFFFF;
}

void CUserDataManager::LeavePartyMember(int chara) {
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_MONSTER) {
        return;
    }
    party_member &= ~(1 << chara) & 0xFFFF;
}

int CUserDataManager::GetNowPartyMember() {
    int mask = party_member;
    int result = mask;
    if (SearchItemOnItemBrd(0x134, 0) != 0) {
        result = mask | 8;
    }
    return result;
}

void CUserDataManager::EnableCharaChange(int chara) {
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_MONSTER) {
        return;
    }
    chara_change |= (1 << chara) & 0xFFFF;
}

void CUserDataManager::DisableCharaChange(int chara) {
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_MONSTER) {
        return;
    }
    chara_change &= ~(1 << chara) & 0xFFFF;
}

int CUserDataManager::CheckEnableCharaChange(int chara, int *reason) {
    int allowed;
    int flag = GetEnableCharaChangeFlag();
    allowed = 0;
    if (flag & (1 << chara)) {
        allowed = 1;
    }
    int alive = 1;
    int able = 1;
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        if (chara_data[chara].hp.now <= 0.0f) {
            alive = 0;
        }
        int status = GetCharaStatusAttirbute(chara);
        if ((status & CHARA_STATUS_UNK_8) != 0 || (status & CHARA_STATUS_UNK_20) != 0) {
            able = 0;
        }
    }
    if (chara == USER_CHARA_ROBO) {
        if (robo_data.hp.now <= 0.0f || chara_data[USER_CHARA_MAX].hp.now <= 0.0f) {
            alive = 0;
        }
    }
    int forbidden;
    if (chara == USER_CHARA_MONSTER) {
        if (chara_data[USER_CHARA_MONICA].hp.now <= 0.0f) {
            alive = 0;
        }
    }
    forbidden = 0;
    DNG_BATTLE_AREA *scene = &GetMainScene()->battle_area;
    if (scene != 0) {
        u16 flags = scene->floor_status;
        if (flags & 1) {
            forbidden = 1;
        }
        if (flags & 2) {
            forbidden = 1;
        }
    }
    int result;
    if (reason != 0) {
        *reason = 0;
        if (alive != 0) {
            *reason |= 1;
        }
        if (forbidden != 0) {
            *reason |= 2;
        }
        if (able != 0) {
            *reason |= 4;
        }
    }
    result = allowed != 0;
    if (result != 0) {
        result = alive != 0;
    }
    if (result != 0) {
        result = able != 0;
    }
    return result & 0xFF;
}

int CUserDataManager::CheckQuickChange(int chara, int *reason) {
    int state = 0;
    int party_chara = GetNowPartyMember();
    if (party_chara & (1 << chara)) {
        state |= 1;
    }
    int enabled = CheckEnableCharaChange(chara, reason);
    int can_change = 0;
    if (enabled != 0) {
        state |= 2;
        can_change = 1;
    }
    if (chara == USER_CHARA_MONSTER) {
        CGameDataUsed *badge_item = SearchItemOnItemBrd(0x134, 0);
        state = 0;
        int          changeable;
        int          i;
        CMonsterBox *box;
        box = &monster_box;
        changeable = 0;
        i = 0;
        int former_monster = monster_id;
        for (; i < 10; i++) {
            if (box->IsChange(i + 1) != 0) {
                changeable++;
                if (monster_id < 0) {
                    monster_id = box->monster[i].monster_id;
                }
            }
        }
        if (badge_item != 0 && 0 < changeable) {
            state |= 1;
            if (enabled != 0) {
                state |= 2;
                if (can_change == 0) {
                    state &= ~2;
                }
            }
            MOS_CHANGE_PARAM *badge = box->GetMonsterBajjiDataByMonsterID(former_monster);
            if (badge != 0 && badge->hp.GetRate() <= 0.0f) {
                state &= ~2;
            }
        }
    }
    int status = GetCharaStatusAttirbute(active_chr_no);
    if ((status & CHARA_STATUS_UNK_8) != 0 || (status & CHARA_STATUS_UNK_20) != 0) {
        state &= ~2;
    }
    return state;
}

void CUserDataManager::EnableCharaChangeMask(int chara) {
    chara_change_mask |= (1 << chara) & 0xFF;
}

void CUserDataManager::DisableCharaChangeMask(int chara) {
    chara_change_mask &= ~(1 << chara) & 0xFF;
}

void CUserDataManager::InitCharaChangeMask() {
    chara_change_mask = 15;
}

u32 CUserDataManager::GetEnableCharaChangeFlag() {
    int party_chara = GetNowPartyMember();
    int mask = chara_change & chara_change_mask;
    int bits[4] = {1, 2, 4, 2};
    for (int chara_no = USER_CHARA_MAX; chara_no < USER_CHARA_NUM; chara_no++) {
        if ((party_chara & bits[chara_no]) == 0) {
            mask &= ~(1 << chara_no);
        }
    }
    DNG_BATTLE_AREA *scene = &GetMainScene()->battle_area;
    if (scene != 0) {
        u16 flags = scene->floor_status;
        if (flags & 1) {
            mask &= ~5;
        }
        if (flags & 2) {
            mask &= ~0xA;
        }
    }
    return mask;
}

u16 *CUserDataManager::GetCharaStatusAttirbutePtr(int chara) {
    u16 *attr;
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_MONSTER) {
        return 0;
    }
    attr = 0;
    if (chara < USER_CHARA_ROBO) {
        attr = &chara_data[chara].status_attr;
    }
    if (chara == USER_CHARA_ROBO) {
        attr = 0;
    }
    if (chara == USER_CHARA_MONSTER) {
        attr = 0;
    }
    return attr;
}

int CUserDataManager::SetCharaStatusAttirbute(int chara, unsigned int attr, int clear) {
    u16 *word = GetCharaStatusAttirbutePtr(chara);
    if (word == 0) {
        return 0;
    }
    if (chara == USER_CHARA_ROBO) {
        return 0;
    }
    if (clear == 1) {
        *word &= ~attr;
    } else {
        if (*word & CHARA_STATUS_POWER) {
            attr &= ~3;
        }
        if (attr & CHARA_STATUS_POWER) {
            attr &= ~3;
            *word &= 0xFFFE;
            *word &= 0xFFFD;
        }
        *word = *word | attr;
    }
    return *word;
}

int CUserDataManager::SetCharaStatusAttirbuteVol(int chara, unsigned int attr, int time) {
    int result = SetCharaStatusAttirbute(chara, attr, 0);
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        CHARA_DATA *chara_info = &chara_data[USER_CHARA_MONICA];
        if (chara == USER_CHARA_MAX) {
            chara_info = &chara_data[USER_CHARA_MAX];
        }
        if (attr & CHARA_STATUS_POWER) {
            chara_info->status_time[0] = time;
        }
        if (attr & CHARA_STATUS_UNK_2) {
            chara_info->status_time[1] = time;
        }
        if (attr & CHARA_STATUS_UNK_8) {
            chara_info->status_time[2] = time;
        }
        if (attr & CHARA_STATUS_UNK_20) {
            chara_info->status_time[3] = time;
        }
    }
    if (chara == USER_CHARA_ROBO) {
        ROBO_DATA *robot = &robo_data;
        if (attr & CHARA_STATUS_UNK_2) {
            robot->status_time[0] = time;
        }
        if (attr & CHARA_STATUS_UNK_8) {
            robot->status_time[1] = time;
        }
        if (attr & CHARA_STATUS_UNK_20) {
            robot->status_time[2] = time;
        }
    }
    if (chara == USER_CHARA_MONSTER) {
        MOS_CHANGE_PARAM *badge = GetMonsterBajjiDataPtrMosId(monster_id);
        if (badge != 0) {
            if (attr & CHARA_STATUS_POWER) {
                badge->status_time_10 = time;
            }
            if (attr & CHARA_STATUS_POISON) {
                badge->status_time_1 = time;
            }
        }
    }
    return result;
}

int CUserDataManager::GetCharaStatusAttirbute(int chara) {
    u16 *attr = GetCharaStatusAttirbutePtr(chara);
    if (attr != 0) {
        return *attr;
    }
    return 0;
}

MOS_CHANGE_PARAM *CUserDataManager::GetMonsterBajjiDataPtr(int no) {
    return monster_box.GetMonsterBajjiData(no);
}

MOS_CHANGE_PARAM *CUserDataManager::GetMonsterBajjiDataPtrMosId(int monster_id) {
    return monster_box.GetMonsterBajjiDataByMonsterID(monster_id);
}

int CUserDataManager::GetItemBoardOverNum() {
    if (GetSaveData()->GetBitFlag(254) != 0) {
        return 6;
    }
    return 12;
}

int CUserDataManager::GetItemBoardMaxNum(int with_over) {
    int size = 0;
    if (with_over == 0) {
        size = 0x8A;
    }
    if (GetSaveData()->GetBitFlag(254) == 1) {
        if (with_over == 0) {
            size = 0x90;
        }
    }
    int result = size;
    if (with_over == 1) {
        result = 150;
    }
    return result;
}

void CUserDataManager::SetActiveChrNo(int chara) {
    active_chr_no = chara;
    CBattleCharaInfo *info = GetBattleCharaInfo();
    if (info != 0) {
        info->SetChrNo(chara);
    }
}

void CUserDataManager::SetRoboName(char *name) {
    if (name != 0) {
        strcpy(robo_data.name, name);
    }
}

char *CUserDataManager::GetRoboName() {
    return robo_data.name;
}

char *CUserDataManager::GetRoboNameDefault() {
    static char *ridepod_names[7] = {"\x83\x89\x83\x43\x83\x68\x83\x7C\x83\x62\x83\x68",
                                     "Ridepod",
                                     "Robomobil",
                                     "Ridepod",
                                     "Robomobile",
                                     "Ridepod",
                                     "Ridepod"};

    return ridepod_names[LanguageCode];
}

void CUserDataManager::SetVoiceUnit(s32 fitted) {
    robo_data.voice_unit = fitted;
    if (fitted != 0) {
        this->SetRoboVoiceFlag(1);
    }
}

s8 CUserDataManager::CheckVoiceUnit() {
    return robo_data.voice_unit;
}

void CUserDataManager::SetRoboVoiceFlag(int on) {
    robo_data.voice_flag = on;
}

s32 CUserDataManager::CheckRoboVoiceFlag(void) {
    s32 enabled = robo_data.voice_unit != 0;
    if (enabled != 0) {
        enabled = robo_data.voice_flag != 0;
    }
    return enabled & 0xFF;
}

float CUserDataManager::AddRoboAbs(float abs) {
    float value = robo_data.abs.now + abs;
    robo_data.abs.now = value;
    if (value < 0.0f) {
        robo_data.abs.now = 0.0f;
    }
    if (99999.0f < robo_data.abs.now) {
        robo_data.abs.now = 99999.0f;
    }
    return robo_data.abs.now;
}

float CUserDataManager::GetRoboAbs() {
    return robo_data.abs.now;
}

int CUserDataManager::CheckCapacity() {
    int i = 0;

    for (; i < 150; i++) {
        CGameDataUsed *item = &used_data[i];
        if (item->item_type == 11) {
            CDataItem *info = GetItemInfoData(item->item_no);
            if (info != 0) {
                return info->value[0];
            }
        }
    }
    return 0;
}

s16 CUserDataManager::CheckRobotCore() {
    for (int i = 0; i < 150; i++) {
        if (used_data[i].item_type == 11) {
            return used_data[i].item_no;
        }
    }
    return -1;
}

int CUserDataManager::GetDefenceVol(int chara) {
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        CHARA_DATA *chara_info = &chara_data[chara];
        if (chara_info == 0) {
            return 0;
        }
        return (u16)chara_info->defence;
    }
    if (chara == USER_CHARA_ROBO) {
        return robo_data.GetDefenceVol();
    }
    if (chara == USER_CHARA_MONSTER) {
        CHARA_DATA *monster = &chara_data[USER_CHARA_MONICA];
        if (monster != 0) {
            return (u16)monster->defence;
        }
    }
    return 0;
}

void CUserDataManager::JoinPartyChara(int chara_no, int status, int unused) {
    if (chara_no <= 0 || chara_no > 32) {
        return;
    }
    PARTY_CHARA_INFO *slot = &party_chara[chara_no - 1];
    slot->status = status;
    slot->chara_no = chara_no;
    NPC_BASE_DATA *npc = GetPartyNPCData(chara_no);
    if (npc != 0) {
        slot->point = npc->max_npc_point;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", SetPartyCharaStatus__16CUserDataManagerFii);
int CUserDataManager::GetPartyCharaStatus(int chara_no) {
    int index = chara_no - 1;
    if (index < 0 || index >= 32) {
        return 0;
    }
    return party_chara[index].status;
}

int CUserDataManager::NowPartyCharaID() {
    for (int i = 0; i < 32; i++) {
        if (party_chara[i].status & 1) {
            return i + 1;
        }
    }
    return -1;
}

void CUserDataManager::LeaveHouse(int chara_no) {
    int was_in_party = GetPartyCharaStatus(chara_no) & 1;

    int flag = 0;
    if (was_in_party) {
        flag = 1;
    }
    SetPartyCharaStatus(chara_no, 2);
    if (flag) {
        SetPartyCharaStatus(chara_no, 1);
    }
}

PARTY_CHARA_INFO *CUserDataManager::GetPartyCharaInfo(int chara_no) {
    if (chara_no <= 0 || chara_no > 32) {
        return 0;
    }
    return &party_chara[chara_no - 1];
}

int CUserDataManager::UseNpcAbility(int chara_no, int ability, int use) {
    int               usable = 0;
    PARTY_CHARA_INFO *member = GetPartyCharaInfo(chara_no);
    NPC_BASE_DATA    *npc_data = GetPartyNPCData(chara_no);
    if (member == 0 || npc_data == 0) {
        return 0;
    }
    short gauge = member->point;
    u8    cost = npc_data->ability_cost[ability];
    if (cost <= gauge) {
        usable = 1;
        if (use != 0) {
            member->point = gauge - (cost & 0xFF);
            if (member->point < 0) {
                member->point = 0;
            }
        }
    }
    return usable;
}

void CUserDataManager::AllWeaponRepair() {
    int i = 0;

    for (; i < 150; i++) {
        CGameDataUsed *item = &used_data[i];
        if (item->used_type == USED_ITEM_TYPE_ROBO_PART) {
            item->Repair(999);
        }
    }
    robo_data.parts[0].Repair(999);
    robo_data.AddPoint(999.0f);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", RefreshNPCStatus__16CUserDataManagerFi);
int CUserDataManager::GetFishingRodNo() {
    return chara_data[USER_CHARA_MAX].equip[0].item_no;
}

s32 CUserDataManager::NowFishingStyle(void) {
    CGameDataUsed *rod = &chara_data[0].equip[0];
    if (rod != NULL) {
        return rod->IsFishingRod();
    }
    return 0;
}

CGameDataUsed *CUserDataManager::GetActiveEsa() {
    return GetActiveEsa(GetFishingRodNo());
}

CGameDataUsed *CUserDataManager::GetActiveEsa(int rod_no) {
    if (rod_no == 302) {
        return &esa[0];
    }
    if (rod_no == 303) {
        return &esa[1];
    }
    return 0;
}

int CUserDataManager::GetFishBait() {
    int rod = GetFishingRodNo();
    if (rod == 302) {
        return esa[0].item_no;
    }
    if (rod == 303) {
        return esa[1].item_no;
    }
    return 0;
}

void CUserDataManager::DeleteBait() {
    CGameDataUsed *bait = GetActiveEsa();

    GetFishingRodNo();
    if (bait != 0) {
        bait->DeleteNum(1);
    }
}

int CUserDataManager::GetFishInAquarium(int item_no, float size, float weight) {
    CGameDataUsed fish;
    int           i;

    CopyGameData(&fish, item_no);
    fish.data.fish.size = (int)(1000.0f * size);
    fish.data.fish.weight = (u_int)(weight);
    fish.data.fish.unk_1c = GetRandI(3) + 1;
    fish.data.fish.hp = 100;
    fish.data.fish.param[4] += GetRandI(4);
    fish.data.fish.param[3] += GetRandI(4);
    fish.data.fish.param[0] += GetRandI(3);
    fish.data.fish.param[1] += GetRandI(3);
    fish.data.fish.param[2] += GetRandI(3);
    fish.data.fish.unk_36 = GetRandI(0x33) + 200;
    fish.data.fish.unk_35 = 0;
    CGameDataUsed *slot = SearchSpaceUsedDataPtr();
    if (slot != 0) {
        slot->CopyGameData(&fish);
        return 0;
    }
    if (GetNumSameItem(0x135) != 0) {
        if (FishInAquarium(&fish, 0) != 0) {
            return 0;
        }
    }
    i = 0;
    if (0 < GetItemBoardOverNum()) {
        do {
            CGameDataUsed *overflow = &used_data[i + GetItemBoardMaxNum(0)];
            if (overflow->item_no <= 0) {
                overflow->CopyGameData(&fish);
                return 1;
            }
            i++;
        } while (i < GetItemBoardOverNum());
    }
    return 2;
}

int CUserDataManager::CheckFishRecordUpdate(int item_no, float size, float weight) {
    CFishingRecord *log = &fish_record;
    if (log != 0) {
        return log->CheckRecordFish(item_no, size, weight);
    }
    return 0;
}

void CUserDataManager::GetFishRecord(int item_no, float *size, float *weight) {
    CFishingRecord *log = &fish_record;
    if (log != 0) {
        FISH_RECORD *record = log->GetFishRecord(item_no);
        if (record != 0) {
            if (size != 0) {
                *size = record->size;
            }
            if (weight != 0) {
                *weight = record->weight;
            }
        }
    }
}

void CUserDataManager::GetRodStatus(int *status) {
    if (status != 0 && GetFishingRodNo() > 0) {
        status[0] = chara_data[USER_CHARA_MAX].equip[0].data.weapon.attribute[0];
        status[1] = chara_data[USER_CHARA_MAX].equip[0].data.weapon.attribute[1];
        status[2] = chara_data[USER_CHARA_MAX].equip[0].data.weapon.attribute[2];
        status[3] = chara_data[USER_CHARA_MAX].equip[0].data.weapon.attribute[3];
        status[4] = chara_data[USER_CHARA_MAX].equip[0].data.weapon.attribute[4];
    }
}

int CUserDataManager::AddFp(int point) {
    if (GetFishingRodNo() <= 0) {
        return 0;
    }
    return chara_data[USER_CHARA_MAX].equip[0].AddFusionPoint(point);
}

int CUserDataManager::SetChrEquip(int chara, CGameDataUsed *item) {
    int slot;
    if (item == 0) {
        return 0;
    }
    CGameData        *game_data = GetGameDataPt();
    short             item_no = item->item_no;
    int               item_type = game_data->GetDataType(item_no);
    CBattleCharaInfo *battle = GetBattleCharaInfo();
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        CHARA_DATA *chara_info = GetCharaDataPtr(chara);
        int         owner = IsItemtypeWhoisEquip(item_no, &slot);
        if (owner == chara && 0 <= slot) {
            GameDataSwap(item, &chara_info->equip[slot], 0);
            if (battle != 0) {
                battle->RefreshParamater();
            }
            return 1;
        }
    }
    if (chara == USER_CHARA_ROBO) {
        ROBO_DATA *ridepod = &robo_data;
        for (int part = 0; part < 4; part++) {
            if (item_type == SearchEquipType(USER_CHARA_ROBO, part)) {
                GameDataSwap(&ridepod->parts[part], item, 0);
                if (battle != 0) {
                    battle->RefreshParamater();
                }
                return 1;
            }
        }
    }
    return 0;
}

int CUserDataManager::SetChrEquip(int chara, int item_no) {
    CGameDataUsed *item;

    if (item_no <= 0) {
        return 0;
    }
    if ((chara < 0) || (chara > 2)) {
        return 0;
    }
    if (this->SearchEquip(chara, item_no) != 0) {
        return 0;
    }
    item = this->SearchItemOnItemBrd(item_no, 1);
    if (item == NULL) {
        return 0;
    }
    this->SetChrEquip(chara, item);
    return 1;
}

int CUserDataManager::SetChrEquipDirect(int chara, int item_no) {

    if (item_no <= 0) {
        return 0;
    }
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_ROBO) {
        return 0;
    }
    if (SearchEquip(chara, item_no) != 0) {
        return 0;
    }

    CGameDataUsed item;
    CopyGameData(&item, item_no);
    SetChrEquip(chara, &item);
    return 1;
}

CGameDataUsed *CUserDataManager::SearchEquip(int chara, int item_no) {
    CGameDataUsed *found = 0;
    int            i;
    ROBO_DATA     *robot;
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        CHARA_DATA *chara_info = GetCharaDataPtr(chara);
        i = 0;
        do {
            if (item_no == chara_info->active_item[i].item_no) {
                found = &chara_info->active_item[i];
            }
            i++;
        } while (i < 3);
        i = 0;
        do {
            if (item_no == chara_info->equip[i].item_no) {
                found = &chara_info->equip[i];
            }
            i++;
        } while (i < 5);
    }
    if (chara == USER_CHARA_ROBO) {
        robot = &robo_data;
        i = 0;
        do {
            if (item_no == robot->parts[i].item_no) {
                found = &robot->parts[i];
            }
            i++;
        } while (i < 3);
    }
    return found;
}

char *CUserDataManager::GetCharaEquipDataPath(int chara, int slot) {
    if (chara < USER_CHARA_MAX || chara > USER_CHARA_ROBO) {
        return 0;
    }
    if (chara < USER_CHARA_ROBO) {
        if (slot < 0 || slot > 4) {
            return 0;
        }
        return chara_data[chara].equip[slot].GetDataPath();
    }
    if (slot < 0 || slot > 3) {
        return 0;
    }
    return robo_data.parts[slot].GetDataPath();
}

int CUserDataManager::AddFusionPoint(int chara, int weapon, int point) {
    if (chara == USER_CHARA_MAX || chara == USER_CHARA_MONICA) {
        if (weapon == 0 || weapon == 1) {
            return chara_data[chara].equip[weapon].AddFusionPoint(point);
        }
    }
    return 0;
}

int CUserDataManager::SearchSpaceUsedData() {
    int bag_size = GetNowBagMax(0);
    for (int i = 0; i < bag_size; i++) {
        if (used_data[i].item_no <= 0) {
            return i;
        }
    }
    return -1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", SearchSpaceUsedData__16CUserDataManagerFi);
CGameDataUsed *CUserDataManager::SearchSpaceUsedDataPtr() {
    int index = SearchSpaceUsedData();
    if (index < 0) {
        return 0;
    }
    return &used_data[index];
}

CGameDataUsed *CUserDataManager::SearchSpaceUsedDataPtr(int item_no) {
    int index = SearchSpaceUsedData(item_no);
    if (index < 0) {
        return 0;
    }
    return &used_data[index];
}

int CUserDataManager::SearchActiveItemTableSpace(int chara, int item_no) {
    CHARA_DATA *chara_info = GetCharaDataPtr(chara);
    int         i = 0;
    if (chara_info == 0) {
        return -1;
    }
    for (; i < 3; i++) {
        CGameDataUsed *item = &chara_info->active_item[i];
        if (item->item_no == item_no && item->CheckStackRemain() > 0) {
            return i;
        }
    }
    int j = 0;
    for (; j < 3; j++) {
        if (chara_info->active_item[j].item_no <= 0) {
            return j;
        }
    }
    return -1;
}

CGameDataUsed *CUserDataManager::SearchItemOnItemBrd(int item_no, int with_over) {
    CGameDataUsed *item = GetUsedDataPtr(0);
    int            limit = GetNowBagMax(0);
    if (with_over != 0) {
        limit = GetNowBagMax(1);
    }
    for (int i = 0; i < limit; i++, item++) {
        if (item_no == item->item_no) {
            return item;
        }
    }
    return 0;
}

s32 CUserDataManager::GetNumStackOverBoard(void) {
    s32 count = 0;
    CGameDataUsed *item = GetUsedDataPtr(GetNowBagMax(0));
    for (s32 index = 0; index < GetItemBoardOverNum(); index++, item++) {
        if (item->item_no > 1) {
            count += 1;
        }
    }
    return count;
}

CGameDataUsed *CUserDataManager::SearchAllHaveItem(int item_no) {
    CGameDataUsed *found = SearchItemOnItemBrd(item_no, 1);
    if (found == 0) {
        for (int chara_no = USER_CHARA_MAX; chara_no < USER_CHARA_ROBO; chara_no++) {
            CHARA_DATA *chara = &chara_data[chara_no];
            for (int slot = 0; slot < 3; slot++) {
                if (item_no == chara->active_item[slot].item_no) {
                    found = &chara->active_item[slot];
                    break;
                }
            }
        }
    }
    return found;
}

s32 CUserDataManager::FishInAquarium(CGameDataUsed *fish, s32 tank) {
    CFishAquarium *aquarium = &this->aquarium;
    if ((tank < 0) || (tank > 2)) {
        return 0;
    }
    s32 space = aquarium->SearchAqua1NotUsed(0);
    if ((space < 0) || (fish == NULL)) {
        return 0;
    }
    aquarium->FishIntoAquarium(tank, space, fish);
    fish->Init();
    return 1;
}

int CUserDataManager::CheckElectricFish() {
    CFishAquarium *tanks = &aquarium;
    if (tanks == 0) {
        return 0;
    }
    CGameDataUsed *fish = tanks->GetAquariumFishTop(0);
    for (int i = 0; i < 6; i++) {
        if (fish[i].item_no > 0 && (fish[i].data.fish.flags & BREEDFISH_FLAG_ELECTRIC)) {
            return 1;
        }
    }
    return 0;
}

#ifdef NONMATCHING
int CUserDataManager::GetNumSameItem(int item_no) {
    int            i;
    int            bag_size;
    int            j;
    int            total;
    int            slot;
    CGameDataUsed *entry;
    int            chara;
    CGameDataUsed *item;
    CHARA_DATA    *data;
    int            part;
    total = 0;
    bag_size = GetNowBagMax(1);
    i = 0;
    if (0 < bag_size) {
        do {
            item = &used_data[i];
            if (item_no == item->item_no) {
                total += item->GetNum();
            }
            total += item->GetGiftBoxSameItemNum(item_no);
            i++;
        } while (i < bag_size);
    }
    chara = USER_CHARA_MAX;
    do {
        data = &chara_data[chara];
        j = 0;
        do {
            entry = &data->active_item[j];
            if (item_no == entry->item_no) {
                total += entry->GetNum();
            }
            total += entry->GetGiftBoxSameItemNum(item_no);
            j++;
        } while (j < 3);
        slot = 0;
        do {
            if (item_no == data->equip[slot].item_no) {
                total += 1;
            }
            slot++;
        } while (slot < 5);
        chara++;
    } while (chara < USER_CHARA_ROBO);
    part = 0;
    do {
        if (item_no == robo_data.parts[part].item_no) {
            total += 1;
        }
        part++;
    } while (part < 4);
    return total;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", GetNumSameItem__16CUserDataManagerFi);
#endif
s16 CUserDataManager::AddYarikomiMedal(int num) {
    short *count = &yarikomi_medal;
    *count = *count + num;
    if (yarikomi_medal < 0) {
        yarikomi_medal = 0;
    }
    if (yarikomi_medal > 999) {
        yarikomi_medal = 999;
    }
    return yarikomi_medal;
}

int CUserDataManager::GetYarikomiMedal() {
    return yarikomi_medal;
}

int CUserDataManager::GetItem(int item_no, int num) {
    int          limit;
    int          fit;
    int          over;
    CDataCommon *common;
    fit = GetItemNotOver(item_no, num);
    over = num - fit;
    common = GetCommonItemData(item_no);
    if (common != NULL && (common->attribute & 0x40)) {
        limit = common->max_num;
        if (limit <= GetNumSameItem(item_no)) {
            return 1;
        }
    }
    if (over > 0) {
        GetOverItem(item_no, over);
    }
    return fit;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", GetItemNotOver__16CUserDataManagerFii);
int CUserDataManager::GetOverItem(int item_no, int num) {
    if (item_no <= 0 || num <= 0) {
        return 0;
    }
    CDataCommon *common = GetCommonItemData(item_no);
    if (common == 0) {
        return 0;
    }
    ConvertUsedItemType(common->type);
    int            overflow_start = GetNowBagMax(0);
    int            overflow_size = GetItemBoardOverNum();
    CGameDataUsed *target;
    int            placed = 0;
    if (0 < num) {
        target = 0;
        do {
            for (int i = 0; i < overflow_size && target == 0; i++) {
                int            slot_item;
                CGameDataUsed *slot;
                slot = &used_data[overflow_start + i];
                slot_item = slot->item_no;
                if (slot_item == item_no && slot->CheckTypeEnableStack() != 0 &&
                    0 < slot->CheckStackRemain()) {
                    target = slot;
                }
                if (slot_item <= 0 && target == 0) {
                    target = slot;
                }
            }
            if (target != 0) {
                CopyGameData(target, item_no);
                GetCostume(item_no);
                placed++;
                target = 0;
                if (placed < num) {
                    continue;
                }
            }
            break;
        } while (1);
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", CheckItemLimmitOver__16CUserDataManagerFv);
/**
 * Removes an item count from one inventory entry.
 */
static int DeleteItem_Local(CGameDataUsed *item, int item_no, int count) {
    int removed;
    if (item_no <= 0) {
        return 0;
    }
    removed = 0;
    if (item_no == item->item_no) {
        removed += item->DeleteNum(count);
    } else if (item->used_type == USED_ITEM_TYPE_GIFT_BOX) {
        for (int index = 0; index < 3; index++) {
            if (0 < count && item_no == item->GetGiftBoxItemNo(index)) {
                item->SetGiftBoxItem(0, index);
                removed++;
                count--;
            }
        }
    }
    return removed;
}

int CUserDataManager::DeleteItem(int item_no, int num) {
    CGameDataUsed *bag = GetUsedDataPtr(0);
    for (int slot = 149; slot >= 0; slot--) {
        int removed = DeleteItem_Local(&bag[slot], item_no, num);
        if (0 < removed) {
            num -= removed;
        }
        if (!(0 < num)) {
            break;
        }
    }
    for (int chara_no = USER_CHARA_MAX; chara_no < USER_CHARA_ROBO; chara_no++) {
        for (int slot = 0; slot < 3; slot++) {
            int removed = DeleteItem_Local(&chara_data[chara_no].active_item[slot], item_no, num);
            if (0 < removed) {
                num -= removed;
            }
            if (!(0 < num)) {
                break;
            }
        }
    }
    return 1;
}

int CUserDataManager::CopyGameData(CGameDataUsed *place, int item_no) {
    unsigned int used_type;
    CDataCommon *common;

    if (place == 0) {
        return 0;
    }
    common = GetCommonItemData(item_no);
    if (common == 0) {
        return 0;
    }
    used_type = ConvertUsedItemType(common->type);
    switch (used_type) {
    case USED_ITEM_TYPE_ITEM:
    case USED_ITEM_TYPE_UNK_4:
        place->CopyDataItem(item_no);
        break;
    case USED_ITEM_TYPE_ATTACH:
        place->CopyDataAttach(item_no);
        break;
    case USED_ITEM_TYPE_WEAPON:
        place->CopyDataWeapon(item_no);
        break;
    case USED_ITEM_TYPE_ROBO_PART:
        place->CopyDataRoboPart(item_no);
        break;
    case USED_ITEM_TYPE_GIFT_BOX:
        place->CopyDataGiftBox(item_no);
        break;
    case USED_ITEM_TYPE_FISH:
        place->CopyDataFish(item_no);
        break;
    }
    return 1;
}

int CUserDataManager::AddMoney(int money) {
    int *total = &this->money;
    *total = *total + money;
    if (this->money < 0) {
        this->money = 0;
    }
    if (999999 < this->money) {
        this->money = 999999;
    }
    return this->money;
}

void CUserDataManager::SetCostumeBit(unsigned long bit) {
    costume_bit = bit;
}

unsigned long CUserDataManager::GetCostumeBit() {
    return costume_bit;
}

void CUserDataManager::GetCostume(int item_no) {
    COSBIT_INFO *info = GetCosInfo(item_no);
    if (info != 0) {
        costume_bit |= (s64)1 << info->bit_no;
    }
}

int CUserDataManager::CountFish() {
    int            count = 0;
    CGameDataUsed *item = used_data;
    for (int i = 0; i < 150; i++, item++) {
        if (item->used_type == USED_ITEM_TYPE_FISH) {
            count++;
        }
    }
    CFishAquarium *tanks = &aquarium;
    for (int i = 0; i < 6; i++) {
        if (0 < tanks->fish_tank[i].item_no) {
            count++;
        }
    }
    return count;
}

void SetEnvUserDataMan(int dungeon) {
    CUserDataManager *manager = GetUserDataMan();
    if (dungeon == 0) {
        manager->InitCharaChangeMask();
        manager->DisableCharaChange(USER_CHARA_ROBO);
        manager->DisableCharaChange(USER_CHARA_MONSTER);
    }
    if (dungeon == 1) {
        manager->InitCharaChangeMask();
        manager->EnableCharaChange(USER_CHARA_ROBO);
        manager->EnableCharaChange(USER_CHARA_MONSTER);
    }
}

void GetCharaDefaultWeapon(int chara, int *item_no) {
    static s16 starting_weapons[2][10] = {{1, 22, 111, 117, 258, 41, 91, 123, 129, 263},
                                          {1, 22, 111, 117, 260, 41, 91, 123, 129, 263}};

    int language = LanguageCode;
    if (language > 1) {
        language = 1;
    }
    int    base = chara * 5;
    short *table = starting_weapons[language];
    for (int i = 0; i < 5; i++) {
        item_no[i] = table[base + i];
    }
    item_no[5] = -1;
}

void LanguageEquipChange(void) {
    CUserDataManager *manager = GetUserDataMan();
    int               weapons[6];
    if (manager != 0) {
        for (int chara_no = USER_CHARA_MAX; chara_no < USER_CHARA_ROBO; chara_no++) {
            GetCharaDefaultWeapon(chara_no, weapons);
            for (int slot = 0; slot < 5; slot++) {
                manager->SetChrEquipDirect(chara_no, weapons[slot]);
            }
        }
        manager->SetRoboName(manager->GetRoboNameDefault());
    }
}

void CheckEquipChange(int chara) {
    int weapons[6];
    if (chara == USER_CHARA_MONICA) {
        CHARA_DATA *chara_info = GetUserDataMan()->GetCharaDataPtr(USER_CHARA_MONICA);
        if (chara_info != 0) {
            GetCharaDefaultWeapon(USER_CHARA_MONICA, weapons);
            GetUserDataMan()->SetChrEquipDirect(USER_CHARA_MONICA, weapons[0]);
            if ((s8)chara_info->unk_2b == 0) {
                GetUserDataMan()->SetChrEquipDirect(USER_CHARA_MONICA, weapons[2]);
                GetUserDataMan()->SetChrEquipDirect(USER_CHARA_MONICA, weapons[3]);
                GetUserDataMan()->SetChrEquipDirect(USER_CHARA_MONICA, weapons[4]);
            }
            chara_info->unk_2b = 0;
        }
    }
}

void CBattleCharaInfo::Initialize(void) {
    memset(this, 0, 0x90);
    chr_no = USER_CHARA_MAX;
    chara_type = -1;
    chara_data = 0;
    chara_data = 0;
    hp = 0;
    equip = 0;
    hp_change_step = 0;
    unk_80 = -1.0f;
    disp_hp = -1.0f;
}

CGameDataUsed *CBattleCharaInfo::GetEquipTablePtr(int slot) {
    if (slot < 0 || slot > 3) {
        return 0;
    }
    return &equip[slot];
}

void CBattleCharaInfo::SetChrNo(int chara) {
    CUserDataManager *manager = GetUserDataMan();
    if (chr_no != chara) {
        ClearMagicSwordPow();
    }
    chr_no = chara;
    unk_2 = 0;
    if (USER_CHARA_MAX <= chr_no && chr_no < USER_CHARA_ROBO) {
        chara_type = BATTLE_CHARA_HUMAN;
        chara_data = manager->GetCharaDataPtr(chr_no);
        active_item = ((CHARA_DATA *)chara_data)->active_item;
        equip = ((CHARA_DATA *)chara_data)->equip;
        hp = &((CHARA_DATA *)chara_data)->hp;
        disp_hp = hp->now;
        unk_80 = hp->max;
        prev_hp = hp->now;
        unk_88 = hp->max;
    } else if (chr_no == USER_CHARA_ROBO) {
        chara_type = BATTLE_CHARA_ROBO;
        chara_data = &manager->robo_data;
        active_item = 0;
        equip = &((ROBO_DATA *)chara_data)->parts[0];
        hp = &((ROBO_DATA *)chara_data)->hp;
        disp_hp = hp->now;
        unk_80 = hp->max;
        prev_hp = hp->now;
        unk_88 = hp->max;
    } else if (chr_no == USER_CHARA_MONSTER) {
        chara_type = BATTLE_CHARA_MONSTER;
        int monster_id = GetMonsterID();
        chara_data = manager->GetMonsterBajjiDataPtrMosId(monster_id);
        int               base = 0;
        BASE_MONSTER_TBL *info = GetMonsterBaseInfo(monster_id);
        if (info != 0) {
            base = info->user_mons_id;
        }
        unk_2 = base;
        active_item = 0;
        equip = 0;
        hp = &manager->GetCharaDataPtr(USER_CHARA_MONICA)->hp;
        disp_hp = hp->now;
        unk_80 = hp->max;
        prev_hp = hp->now;
        unk_88 = hp->max;
    }
    poison_count = 0;
    BattleParamater_Time = 0;
    BattleParamater_TimeBand = 0;
    RefreshParamater();
}

int CBattleCharaInfo::GetMonsterID(void) {
    CUserDataManager *manager = GetUserDataMan();
    if (manager != 0) {
        return manager->monster_id;
    }
    return 0;
}

int CBattleCharaInfo::GetNowNPC(void) {
    return now_npc;
}

int CBattleCharaInfo::UseNPCPoint(int unused) {
    int npc_no = now_npc;
    if (npc_no <= 0) {
        return 0;
    }
    if (GetUserDataMan()->GetPartyCharaInfo(npc_no) == 0) {
        return 0;
    }
    if (GetPartyNPCData(now_npc) == 0) {
        return 0;
    }
    if (now_npc == 10) {
        if (GetMainScene()->battle_area.floor_status & 4) {
            return 0;
        }
        if (1.0f <= hp->GetRate()) {
            return 0;
        }
        if (GetUserDataMan()->UseNpcAbility(10, 3, 1) != 0) {
            hp->AddRate(0.05f);
            return 1;
        }
    }
    return 0;
}

CGameDataUsed *CBattleCharaInfo::GetActiveItemInfo(int index) {
    CGameDataUsed *table = active_item;
    CGameDataUsed *item = 0;
    if (table != 0) {
        item = &table[index];
    }
    return item;
}

int CBattleCharaInfo::UseActiveItem(CGameDataUsed *item) {

    int item_no;

    if (item == 0) {
        return 0;
    }
    item_no = item->item_no;
    CItemUseTarget target;
    target.SetPtr(ITEM_USE_TARGET_CHARA, chara_data);
    if (item_no == 294) {
        target.SetPtr(ITEM_USE_TARGET_ITEM, GetEquipTablePtr(0));
    }
    if (item_no == 298 || item_no == 352) {
        target.SetPtr(ITEM_USE_TARGET_ITEM, GetEquipTablePtr(1));
    }
    return MenuUseItemCheckFunc(item, &target, 1);
}

u32 CBattleCharaInfo::GetSpecialStatus(int weapon) {
    if (chara_type == BATTLE_CHARA_HUMAN) {
        if (weapon == 0 || weapon == 1) {

            return weapon_param[weapon].special;
        }
    }
    return 0;
}

s16 CBattleCharaInfo::GetPalletNo(int weapon) {
    if (chara_type == BATTLE_CHARA_HUMAN) {
        if (weapon == 0 || weapon == 1) {

            return weapon_param[weapon].pallet_no;
        }
    }
    return -1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", RefreshParamater__16CBattleCharaInfoFv);
COMMON_GAGE *CBattleCharaInfo::GetNowAccessWHp(int weapon) {
    COMMON_GAGE *gage = 0;
    short        current_mode = chara_type;
    if (current_mode == BATTLE_CHARA_HUMAN) {
        CGameDataUsed *table = equip;
        if (table == 0) {
            return gage;
        }
        gage = &table[weapon].data.weapon.whp;
    } else if (current_mode == BATTLE_CHARA_ROBO) {
        gage = (COMMON_GAGE *)&equip->data.robopart;
        gage = &gage[1];
    } else if (current_mode == BATTLE_CHARA_MONSTER) {
        gage = &((MOS_CHANGE_PARAM *)chara_data)->hp;
    }
    return gage;
}

COMMON_GAGE *CBattleCharaInfo::GetNowAccessAbs(int weapon) {
    COMMON_GAGE *gage = 0;
    short        current_mode = chara_type;
    if (current_mode == BATTLE_CHARA_HUMAN) {
        CGameDataUsed *table = equip;
        if (table == 0) {
            return gage;
        }
        gage = &table[weapon].data.weapon.whp;
        gage = &gage[1];
    } else if (current_mode == BATTLE_CHARA_ROBO) {
        gage = &((ROBO_DATA *)chara_data)->abs;
    } else if (current_mode == BATTLE_CHARA_MONSTER) {
        gage = &((MOS_CHANGE_PARAM *)chara_data)->abs;
    }
    return gage;
}

float CBattleCharaInfo::AddWhp(int weapon, float whp) {
    COMMON_GAGE *gage = GetNowAccessWHp(weapon);
    if (gage == 0) {
        return 0.0f;
    }
    gage->AddPoint(whp);
    if (gage->max != 0.0f) {
        return gage->GetRate();
    }
    return 0.0f;
}

void CBattleCharaInfo::GetNowWhp(int weapon, int *whp) {
    COMMON_GAGE *gage = GetNowAccessWHp(weapon);
    if (gage != 0) {
        whp[0] = GetDispVolumeForFloat(gage->now);
        whp[1] = (int)(gage->max);
    }
}

int CBattleCharaInfo::GetWhpNowVol(int weapon) {
    COMMON_GAGE *gage = GetNowAccessWHp(weapon);
    if (gage != 0) {
        return GetDispVolumeForFloat(gage->now);
    }
    return 0;
}

void CBattleCharaInfo::SetMagicSwordPow(int elem, int pow) {
    if (magic_sword_elem != elem) {
        ClearMagicSwordPow();
    }
    if (elem < 0 || elem > 3) {
        return;
    }
    if (chr_no != USER_CHARA_MONICA) {
        return;
    }
    int counter_max = GetMagicSwordCounterMax();
    if (magic_sword_num < counter_max && pow > 0) {
        magic_sword_elem = elem;
        magic_sword_pow[magic_sword_num] = pow;
        magic_sword_num = magic_sword_num + 1;
    }
}

int CBattleCharaInfo::GetMagicSwordElem(void) {
    s16 element = -1;
    if (!(chr_no == USER_CHARA_MONICA)) {
        return element;
    }
    element = magic_sword_elem;
    return element;
}

int CBattleCharaInfo::GetMagicSwordPow(void) {
    int total = 0;
    for (int i = 0; i < magic_sword_num; i++) {
        total += magic_sword_pow[i];
    }
    if (chr_no == USER_CHARA_MONICA) {
        return total;
    }
    return 0;
}

int CBattleCharaInfo::GetMagicSwordCounterNow(void) {
    if (chr_no != USER_CHARA_MONICA) {
        return 0;
    }
    return magic_sword_num;
}

s32 CBattleCharaInfo::GetMagicSwordCounterMax(void) {
    CGameDataUsed *weapon = equip;
    if (weapon == NULL) {
        return 0;
    }
    if (chr_no != USER_CHARA_MONICA) {
        return 0;
    }
    if (weapon == NULL) {
        return 0;
    }
    s16 power = weapon->data.weapon.status[1];
    if (power < 32) {
        return 0;
    }
    s32 max_charges = (power - 32) / 16 + 3;
    if (max_charges > 7) {
        max_charges = 7;
    }
    return max_charges;
}

void CBattleCharaInfo::ClearMagicSwordPow(void) {
    magic_sword_elem = -1;
    magic_sword_num = 0;
    for (s32 i = 0; i < 7; i++) {
        magic_sword_pow[i] = 0;
    }
}

float CBattleCharaInfo::AddAbs(int weapon, float abs, int *level_up) {
    float        rate;
    COMMON_GAGE *gage = GetNowAccessAbs(weapon);
    if (gage == 0) {
        return 0.0f;
    }
    if (chara_type == BATTLE_CHARA_ROBO) {
        rate = 0.0f;
        gage->now += abs;
        GetUserDataMan()->AddRoboAbs(abs);
    } else if (chara_type == BATTLE_CHARA_MONSTER) {
        ((MOS_CHANGE_PARAM *)chara_data)->abs.AddPoint(abs);
        MOS_CHANGE_PARAM *badge = (MOS_CHANGE_PARAM *)chara_data;
        if (badge != 0) {
            int leveled = badge->LevelUp();
            if (level_up != 0 && leveled != 0) {
                *level_up = 1;
            }
        }
    } else {
        CGameDataUsed *item = equip;
        if (item == 0) {
            return 0.0f;
        }
        if (chr_no == USER_CHARA_MAX && weapon == 0 && item->IsFishingRod() != 0) {
            return 0.0f;
        }
        gage->AddPoint(abs);
        rate = 0.0f;
        if (gage->max != 0.0f) {
            rate = gage->GetRate();
            int leveled = LevelUpWeapon(&equip[weapon]);
            if (level_up != 0 && leveled != 0) {
                *level_up = 1;
            }
        }
    }
    return rate;
}

int CBattleCharaInfo::AddAbsRate(int weapon, float rate, int *level_up) {
    if (equip == 0) {
        return 0;
    }
    COMMON_GAGE *gage = GetNowAccessAbs(weapon);
    if (gage == 0 || chara_type == BATTLE_CHARA_ROBO) {
        return 0;
    }
    gage->now += gage->max * rate;
    if (gage->now < 1.0f) {
        gage->now = 0.0f;
    }
    if (gage->max <= gage->now) {
        gage->now = gage->max;
    }
    int leveled = LevelUpWeapon(&equip[weapon]);
    if (level_up != 0 && leveled != 0) {
        *level_up = 1;
    }
    return leveled;
}

void CBattleCharaInfo::GetNowAbs(int weapon, int *abs) {
    COMMON_GAGE *gage = GetNowAccessAbs(weapon);
    if (gage != 0) {
        abs[0] = GetDispVolumeForFloat(gage->now);
        abs[1] = (int)(gage->max);
    }
}

int CBattleCharaInfo::LevelUpWeapon(CGameDataUsed *weapon) {
    if (chara_type == BATTLE_CHARA_HUMAN) {
        if (weapon->IsLevelUp() == 0) {
            return 0;
        }
        weapon->LevelUp();
        RefreshParamater();
        return 1;
    }
    return 0;
}

s16 CBattleCharaInfo::GetDefenceVol(void) {
    return defence;
}

float CBattleCharaInfo::AddHp_Point(float hp, float frames) {
    if (this->hp == 0) {
        return 0.0f;
    }
    hp_change_frames = frames;
    if (frames <= 1.0f) {
        hp_change_step = hp;
    } else {
        hp_change_step = hp / frames;
    }
    prev_hp = this->hp->now;
    float now = this->hp->now;
    disp_hp = now;
    this->hp->now = now + hp;
    if (this->hp->now <= 0.0f) {
        this->hp->now = 0.0f;
    }
    if (this->hp->max <= this->hp->now) {
        this->hp->now = this->hp->max;
    }
    if (this->hp->max == 0.0f) {
        return 0.0f;
    }
    return this->hp->now / this->hp->max;
}

float CBattleCharaInfo::AddHp_Rate(float rate, int mode, float frames) {
    if (hp == 0) {
        return 0.0f;
    }
    hp_change_frames = frames;
    prev_hp = hp->now;
    disp_hp = hp->now;
    switch (mode) {
    case 0:
    case 2:
        hp->now += hp->max * rate;
        if (mode == 2) {
            if (hp->now <= 1.0f) {
                hp->now = 1.0f;
            }
        }
        break;
    case 1:
    case 3: {
        float now = hp->now;
        hp->now = now + now * rate;
        if (mode == 3) {
            if (hp->now < 1.0f) {
                hp->now = 1.0f;
            }
        }
        break;
    }
    }
    hp->now = (float)GetDispVolumeForFloat(hp->now);
    if (hp->now < 0.0f) {
        hp->now = 0.0f;
    }
    if (hp->max < hp->now) {
        hp->now = hp->max;
    }
    float diff = hp->now - prev_hp;
    if (hp_change_frames <= 1.0f) {
        hp_change_step = diff;
    } else {
        hp_change_step = diff / hp_change_frames;
    }
    return hp->GetRate();
}

void CBattleCharaInfo::SetHpRate(float rate) {
    COMMON_GAGE *gage = hp;
    if (gage != 0) {
        gage->SetFillRate(rate);
    }
}

int CBattleCharaInfo::GetMaxHp_i(void) {
    COMMON_GAGE *gage = hp;
    if (gage != 0) {
        return (int)(gage->max);
    }
    return 0;
}

int CBattleCharaInfo::GetNowHp_i(void) {
    COMMON_GAGE *gage = hp;
    if (gage != 0) {
        return GetDispVolumeForFloat(gage->now);
    }
    return 0;
}

int CBattleCharaInfo::SetAttr(int attr, int clear) {
    CUserDataManager *manager = GetUserDataMan();
    int               result = 0;
    if (manager != 0) {
        manager->SetCharaStatusAttirbute(chr_no, attr, clear);
        result = manager->GetCharaStatusAttirbute(chr_no);
    }
    return result;
}

int CBattleCharaInfo::SetAttrVol(int attr, int time) {
    CUserDataManager *manager = GetUserDataMan();
    int               result = 0;
    if (manager != 0) {
        manager->SetCharaStatusAttirbuteVol(chr_no, attr, time);
        result = manager->GetCharaStatusAttirbute(chr_no);
    }
    return result;
}

int CBattleCharaInfo::GetAttr(void) {
    CUserDataManager *manager = GetUserDataMan();
    if (manager != 0) {
        return manager->GetCharaStatusAttirbute(chr_no);
    }
    return 0;
}

void CBattleCharaInfo::ForceSet(void) {
    COMMON_GAGE *gage = hp;
    if (gage != 0) {
        float point = gage->now;
        if (disp_hp != point) {
            disp_hp = point;
            prev_hp = -1.0f;
            hp_change_step = 0;
        }
    }
}

int GetRandomCircleTrapID(int kind) {
    static s8 secondary_traps[2] = {6, 7};

    static s8 primary_traps[4] = {1, 2, 3, 4};

    int roll;
    int chara_no = GetBattleCharaInfo()->chr_no;
    roll = rand();
    int trap;
    srand(roll);
    trap = 0;
    if (kind == 0) {
        trap = primary_traps[roll % 3];
    }
    if (kind == 1) {
        trap = secondary_traps[roll % 2];
        if (trap == 7) {
            trap = (GetRandI(11) + GetRandI(21)) % 2 + 8;
            if (chara_no == USER_CHARA_MONICA) {
                trap += 2;
            }
        }
    }
    if (chara_no == USER_CHARA_ROBO) {
        trap = -2;
    }
    if (chara_no == USER_CHARA_MONSTER && trap != 4 && trap != 12) {
        trap = -1;
    }
    return trap;
}

int SetRandamCircleStatus(int trap, float &value) {
    if (trap <= 0) {
        return 0;
    }
    CBattleCharaInfo *battle = GetBattleCharaInfo();
    int               applied;
    if (trap == 3) {
        CUserDataManager *manager = GetUserDataMan();
        if (manager != 0) {
            CHARA_DATA *max = manager->GetCharaDataPtr(USER_CHARA_MAX);
            max->equip[0].Repair(999);
            max->equip[1].Repair(999);
            CHARA_DATA *monica = manager->GetCharaDataPtr(USER_CHARA_MONICA);
            monica->equip[0].Repair(999);
            monica->equip[1].Repair(999);
            return 1;
        }
    }
    applied = 0;
    if (trap == 1) {
        value = 0.1f * battle->GetNowAccessAbs(0)->max + 0.1f * battle->GetNowAccessAbs(1)->max;
    }
    if (trap == 2) {
        const float rate = 1.0f;
        battle->AddHp_Rate(rate, 0, 0.0f);
        battle->SetAttr(0x6F, 1);
        applied = 1;
    }
    if (trap == 5) {
        battle->SetAttr(CHARA_STATUS_POISON, 0);
        applied = 1;
    }
    if (trap == 6) {
        float rate = float(-0.5);
        battle->AddHp_Rate(rate, 3, 0.0f);
        applied = 1;
    }
    if (trap == 9 || trap == 0xB) {
        COMMON_GAGE *gage = battle->GetNowAccessWHp(0);
        applied = 1;
        gage->now *= 0.5f;
    }
    if (trap == 8 || trap == 0xA) {
        COMMON_GAGE *gage = battle->GetNowAccessWHp(1);
        gage->now *= 0.5f;
        applied = 1;
    }
    return applied;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", StatusParamStep__16CBattleCharaInfoFPi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", Step__16CBattleCharaInfoFv);
CBattleCharaInfo *GetBattleCharaInfo(void) {
    return &BattleParamater;
}

void ConvertItemAttrToCharaAttr(s32 attr, s32 *add, s32 *cure) {
    s32 add_attr = 0;
    s32 cure_attr = 0;
    if (attr & 0x10000) {
        add_attr |= CHARA_STATUS_POISON;
    }
    if (attr & 0x100000) {
        add_attr |= CHARA_STATUS_UNK_2;
    }
    if (attr & 0x40000) {
        add_attr |= CHARA_STATUS_UNK_4;
    }
    if (attr & 0x4000) {
        add_attr |= CHARA_STATUS_UNK_8;
    }
    if (attr & 0x400000) {
        add_attr |= CHARA_STATUS_POWER;
    }
    if (attr & 0x02000000) {
        add_attr |= CHARA_STATUS_UNK_20;
    }
    if (attr & 0x08000000) {
        add_attr |= CHARA_STATUS_UNK_40;
    }
    if (attr & 0x20000) {
        cure_attr |= CHARA_STATUS_POISON;
    }
    if (attr & 0x200000) {
        cure_attr |= CHARA_STATUS_UNK_2;
    }
    if (attr & 0x80000) {
        cure_attr |= CHARA_STATUS_UNK_4;
    }
    if (attr & 0x8000) {
        cure_attr |= CHARA_STATUS_UNK_8;
    }
    if (attr & 0x04000000) {
        cure_attr |= CHARA_STATUS_UNK_20;
    }
    if (attr & 0x10000000) {
        cure_attr |= CHARA_STATUS_UNK_40;
    }
    if (add != NULL) {
        *add = add_attr;
    }
    if (cure != NULL) {
        *cure = cure_attr;
    }
}

s32 CheckBadStatus(s32 attr) {
    if ((attr & CHARA_STATUS_POISON) || (attr & CHARA_STATUS_UNK_2) ||
        (attr & CHARA_STATUS_UNK_4) || (attr & CHARA_STATUS_UNK_8) ||
        (attr & CHARA_STATUS_UNK_20) || (attr & CHARA_STATUS_UNK_40)) {
        return 1;
    }
    return 0;
}

unsigned int CheckWeaponAttribute(unsigned int weapon_attr, unsigned int attr) {
    static u32 attribute_pairs[12] = {0x2,  0x1,   0x0,  0x0,   0x0,   0x40,
                                      0x20, 0x100, 0x80, 0x400, 0x200, 0x0};

    int bit = 0;
    do {
        unsigned int pair = attribute_pairs[bit];
        unsigned int mask = 1 << bit;
        if (attribute_pairs[bit] != 0 && (weapon_attr & mask) && (attr & pair)) {
            weapon_attr &= ~mask;
            attr &= ~pair;
        }
        bit++;
    } while (bit < 12);
    weapon_attr |= attr;
    return weapon_attr;
}

int CheckBuildUpMonsterCondition(CDataWeapon *weapon) {
    int ok;
    int i;
    if (weapon == NULL) {
        return 1;
    }
    if (GetSaveData() == NULL) {
        return 1;
    }
    ok = 1;
    for (i = 0; i < 3; i++) {
        short monster = weapon->buildup_monster[i];
        if (0 <= monster && KillMonsterCount(monster, 0) <= 0) {
            ok = 0;
        }
    }
    return ok;
}

int KillMonsterCount(int monster_id, int mode) {
    CSaveData    *save = GetSaveData();
    CMonsterBook *book;
    if (save == NULL) {
        return 0;
    }
    book = &save->monster_book;
    if (book != NULL) {
        return book->CountKill(monster_id, mode);
    }
    return 0;
}

int SearchEquipType(int chara, int slot) {
    static s8 equip_type_tbl[3][5] = {{1, 2, 6, 7, 5}, {3, 4, 9, 10, 8}, {13, 12, 15, 14, 0}};

    if (chara < USER_CHARA_MAX || chara > USER_CHARA_ROBO) {
        return 0;
    }
    if (chara < USER_CHARA_MAX || chara >= USER_CHARA_MONSTER || slot < 0 || slot >= 5) {
        return 0;
    }
    return equip_type_tbl[chara][slot];
}

int IsItemtypeWhoisEquip(int item_no, int *slot) {
    int type = GetItemDataType(item_no);
    int category = -1;
    int found_slot = -1;
    int c;
    int s;
    for (c = USER_CHARA_MAX; c <= USER_CHARA_ROBO; c++) {
        for (s = 0; s < 5; s++) {
            if (type == SearchEquipType(c, s)) {
                found_slot = s;
                category = c;
                break;
            }
        }
    }
    if (slot != NULL) {
        *slot = found_slot;
    }
    return category;
}

int IsCheckParty(int chara) {
    int party = GetUserDataMan()->GetNowPartyMember();
    return (party & (1 << chara)) != 0;
}

char *GetAquariumFish0(int index) {
    CFishAquarium *aquarium = GetAquariumData();
    if (aquarium == NULL) {
        return 0;
    }
    if (index < 0 || index >= 6) {
        return 0;
    }

    if (0 < (int)aquarium->fish_tank[index].item_no) {
        return aquarium->fish_tank[index].GetName(1);
    }
    return 0;
}

int GetUserItemHaveNum(int item_no) {
    CUserDataManager *user_data;

    user_data = GetUserDataMan();
    if (user_data != NULL) {
        return user_data->GetNumSameItem(item_no);
    }
    return 0;
}

int CheckItemOver(void) {
    CUserDataManager *user_data = GetUserDataMan();
    int               count;
    int               slot;
    int               end;
    if (user_data == NULL) {
        return 0;
    }
    count = 0;
    slot = GetNowBagMax(0);
    end = GetNowBagMax(1);
    for (; slot < end; slot++) {
        if (user_data->used_data[slot].item_no > 0) {
            count++;
        }
    }
    return count;
}

int CheckItemLimmitOver(void) {
    CUserDataManager *user_data;

    user_data = GetUserDataMan();
    if (user_data != NULL) {
        return user_data->CheckItemLimmitOver();
    }
    return 0;
}

int CheckGetItemLimmitOver(int item_no, int num) {
    CUserDataManager *user_data;
    CDataCommon      *info;
    int               held;
    int               limit;
    int               take;
    int               bag_max;
    int               bag_room;
    int               i;
    CGameDataUsed    *entry;

    user_data = GetUserDataMan();
    if (user_data == NULL) {
        return 0;
    }
    held = user_data->GetNumSameItem(item_no);
    info = GetCommonItemData(item_no);
    limit = info->max_num - held;
    take = num;
    if (limit < num) {
        take = limit;
    }
    if (item_no == 0x132 || item_no == 0x131) {
        return num;
    }
    int type = ConvertUsedItemType(info->type);
    if (type == USED_ITEM_TYPE_ITEM ||
        (type == USED_ITEM_TYPE_ATTACH && item_no != 0xB9 && item_no != 0x17F)) {
        bag_max = GetNowBagMax(0);
        bag_room = 0;
        i = 0;
        if (0 < bag_max) {
            do {
                entry = &user_data->used_data[i];
                if (entry->item_no <= 0) {
                    bag_room += info->stack_num;
                } else if (item_no == entry->item_no) {
                    bag_room += entry->CheckStackRemain();
                }
                i++;
            } while (i < bag_max);
        }
        if (info->max_num < bag_room) {
            bag_room = info->max_num;
        }
        if (bag_room < take) {
            take = bag_room;
        }
        held = user_data->GetNumSameItem(item_no);
        if (info->max_num < take + held) {
            take = 0;
        }
    } else if (user_data->SearchSpaceUsedData() < 0) {
        take = 0;
    }
    return take;
}

int CheckGetItemRemainNum(int item_no) {
    CUserDataManager *manager = GetUserDataMan();
    int               held;
    if (manager == NULL) {
        return 0;
    }
    held = manager->GetNumSameItem(item_no);
    return GetCommonItemData(item_no)->max_num - held;
}

void CheckItemDngKey(void) {
    CUserDataManager *user_data = GetUserDataMan();
    CGameDataUsed    *item;
    int               bag_max;
    int               i;
    if (user_data != NULL) {
        item = user_data->GetUsedDataPtr(0);
        bag_max = GetNowBagMax(1);
        for (i = 0; i < bag_max; i++, item++) {
            if (item->item_type == 0x1A) {
                item->Init();
            }
        }
        if (user_data->GetHp(USER_CHARA_MAX) < 1.0f) {
            user_data->chara_data[USER_CHARA_MAX].hp.now = 1.0f;
        }
        if (user_data->GetHp(USER_CHARA_MONICA) < 1.0f) {
            user_data->chara_data[USER_CHARA_MONICA].hp.now = 1.0f;
        }
        user_data->SetCharaStatusAttirbute(USER_CHARA_MAX, CHARA_STATUS_POISON, 1);
        user_data->SetCharaStatusAttirbute(USER_CHARA_MONICA, CHARA_STATUS_POISON, 1);
    }
}

void PlayerPartyCure(void) {
    CUserDataManager *user_data = GetUserDataMan();
    CMonsterBox      *monster_box;
    if (user_data != NULL) {
        user_data->chara_data[USER_CHARA_MAX].hp.SetFillRate(1.0f);
        user_data->chara_data[USER_CHARA_MONICA].hp.SetFillRate(1.0f);
        user_data->SetCharaStatusAttirbute(USER_CHARA_MAX, CHARA_STATUS_ALL, 1);
        user_data->SetCharaStatusAttirbute(USER_CHARA_MONICA, CHARA_STATUS_ALL, 1);
        monster_box = &user_data->monster_box;
        if (monster_box != NULL) {
            monster_box->AllCure();
        }
    }
}

void UserDataRefresh(void) {
    CUserDataManager *user_data;

    user_data = GetUserDataMan();
    if (user_data != NULL) {
        user_data->RefreshParam();
    }
}

void DeleteErekiFish(void) {
    CFishAquarium *aquarium;
    CGameDataUsed *fish;
    int            i;

    aquarium = GetAquariumData();
    if (aquarium == NULL) {
        return;
    }
    fish = aquarium->GetAquariumFishTop(0);

    i = 0;
    for (; i < 6; i++) {
        if ((fish[i].item_no > 0) && (fish[i].data.fish.flags & BREEDFISH_FLAG_ELECTRIC)) {
            fish[i].Init();
            return;
        }
    }
}

int GetNowBagMax(int with_over) {
    return GetUserDataMan()->GetItemBoardMaxNum(with_over);
}

#ifdef NONMATCHING
void LeaveMonicaItemCheck(void) {

    CUserDataManager *user_data;
    int               num;
    int               i;
    int               item_no;
    CGameDataUsed    *bag_item;
    CGameDataUsed    *free_slot;
    CGameDataUsed    *active;
    CHARA_DATA       *monica;

    user_data = GetUserDataMan();
    if (user_data != NULL) {
        monica = user_data->GetCharaDataPtr(USER_CHARA_MONICA);
        i = 0;
        if (monica != NULL) {
            do {

                item_no = monica->active_item[i].item_no;
                active = &monica->active_item[i];
                num = active->GetNum();
                if ((item_no > 0) && (num > 0)) {
                    bag_item = user_data->SearchItemOnItemBrd(item_no, 0);
                    free_slot = user_data->SearchSpaceUsedDataPtr();
                    if (bag_item != NULL) {
                        if (bag_item->CheckTypeEnableStack() == 0) {
                            if (free_slot != NULL) {
                                free_slot->CopyGameData(active);
                                active->Init();
                            }
                        } else {
                            bag_item->AddNum(num, 1);
                            active->Init();
                        }
                    } else if (free_slot != NULL) {
                        free_slot->CopyGameData(active);
                        active->Init();
                    }
                }
                i += 1;
            } while (i < 3);
        }
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", LeaveMonicaItemCheck__Fv);
#endif
void AquaFishFatigueClear(void) {
    CUserDataManager *user_data;
    CGameDataUsed    *fish;
    int               tank;
    int               i;
    CGameDataUsed    *tank_fish;
    int               slot;
    CFishAquarium    *aquarium;

    user_data = GetUserDataMan();
    if (user_data == NULL) {
        return;
    }
    fish = user_data->GetUsedDataPtr(0);
    for (i = 0; i < 150; i++, fish++) {
        if (fish->item_no > 0 && fish->used_type == USED_ITEM_TYPE_FISH) {
            fish->data.fish.fatigue = 0;
            fish->data.fish.unk_3d = 0;
        }
    }
    aquarium = &user_data->aquarium;
    if (aquarium == NULL) {
        return;
    }
    tank = 0;
    for (; tank < 3; tank++) {
        tank_fish = aquarium->GetAquariumFishTop(tank);
        if (tank_fish != NULL) {
            for (slot = 0; slot < aquarium_fish_maxtbl[tank]; tank_fish++, slot++) {
                if (tank_fish->used_type == USED_ITEM_TYPE_FISH) {
                    tank_fish->data.fish.fatigue = 0;
                    tank_fish->data.fish.unk_3d = 0;
                }
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", DebugGetItem__FP16CUserDataManageri);

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/userdata", __sinit_userdata_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", mos_henge_param__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", basefish_1288__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", symbol_tbl_1338__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", magic_str_1462__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", strtbl_1505__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", htbl_1662__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", fish_record_dataindex_convert__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_3192__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", robo_nametable_3330__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_4196__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", weptbl_4503__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_table_5400__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", equip_type_tbl_5456__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", cureItemtable_5744__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", itemtbl_5745__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", start_tbl_5746__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", e3_town_5747__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", e3_dng_5748__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", e3_boss_5749__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", init_partytbl_5752__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", dbg_set2_5775__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", dbg_set3_5776__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", subgame1_5788__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_896__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_897__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_898__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_899__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_900__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_901__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_902__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_903__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_904__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_905__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_906__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_907__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_908__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_909__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_910__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_911__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_912__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_913__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_914__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_915__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_916__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_917__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_918__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_919__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_920__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_921__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_922__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_923__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_924__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_925__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_926__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_927__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_928__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_929__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_930__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_931__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_932__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_933__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_934__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_935__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_936__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_937__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_938__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_939__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_940__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_941__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1289__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1290__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1291__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1292__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1293__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1294__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1295__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1339__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1340__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1341__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1342__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1343__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1344__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1378__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1379__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1463__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1464__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1465__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1466__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1467__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1468__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1469__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1470__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1506__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1507__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1623__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1624__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_1637__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_2006__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_2007__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_2018__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_2019__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_3331__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_3332__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_3333__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_3334__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_4442__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", D_0037B004__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", f_2005__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", aquarium_fish_maxtbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", use_limmit_table_2558__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", lifetbl_2854__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", at_4695__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", tbl1_5167__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", tbl2_5168__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/userdata", dbg_set1_5774__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(FishGamePreEquip, 0x4);
INCLUDE_BSS(BattleParamater_Time, 0x4);
INCLUDE_BSS(BattleParamater_TimeBand, 0x4);
INCLUDE_BSS(at_5773, 0x8);

// Uninitialised data (.bss)
INCLUDE_BSS(word_1327, 0x70);
INCLUDE_BSS(temp_1510, 0x40);
INCLUDE_BSS(at_2061, 0x20);
INCLUDE_BSS(BattleParamater, 0x90);
