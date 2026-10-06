#include "common.h"
#include "savedata.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "menuaqua.hpp"

// Code (.text)
void InitSV_CONFIG_OPTION(SV_CONFIG_OPTION *config) {
    if (config != NULL) {
        memset(config, 0, sizeof(SV_CONFIG_OPTION));
        config->map = 1;
    }
}

void CSaveData::Initialize() {
    int i;

    printf("size : %d\n", SAVE_BIT_FLAG_MAX / 32);
    memset(this, 0, sizeof(CSaveData));
    now_time = 12.0f;
    game_progress = 1;
    for (i = 0; i < SAVE_BIT_FLAG_MAX / 32; i++) {
        bit_flag[i] = 0;
    }
    for (i = 0; i < SAVE_SHORT_FLAG_MAX; i++) {
        short_flag[i] = 0;
    }
    map_no = -1;
    sub_map_no = -1;
    prev_map_no = -1;
    prev_sub_map_no = -1;
    area_no = -1;
    for (i = 0; i < SAVE_EDIT_DATA_MAX; i++) {
        edit_data[i].Initialize();
    }
    save_dungeon.Initialize();
    InitSV_CONFIG_OPTION(&config);
    user_data.Initialize();
    menu_system_data.MenuSystemDataInit();
    quest_data.Initialize();
    memset(&monster_book, 0, sizeof(monster_book));
    InitBitCtrl();
    memset(&tour, 0, sizeof(tour));
    tour.base_day = -1;
}

int CSaveData::CheckBitFlagNo(int no) {
    if (no < 0 || no >= SAVE_BIT_FLAG_MAX) {
        return 0;
    }
    return 1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/savedata", SetBitFlag__9CSaveDataFii);

int CSaveData::GetBitFlag(int no) {
    u32 mask;
    u32 *word;

    if (CheckBitFlagNo(no) == 0) {
        return 0;
    }
    mask = 1;
    mask <<= no % 32;
    word = &bit_flag[no / 32];
    return (mask & *word) != 0;
}

s16 CSaveData::SetShortFlag(int no, s16 value) {
    s16 old;

    if (no < 0 || no >= SAVE_SHORT_FLAG_MAX) {
        return 0;
    }
    old = short_flag[no];
    short_flag[no] = value;
    return old;
}

s16 CSaveData::GetShortFlag(int no) {
    if (no < 0 || no >= SAVE_SHORT_FLAG_MAX) {
        return 0;
    }
    return short_flag[no];
}

void CSaveData::SetBuildPartsNum(int parts_no, int num) {
    if (parts_no < 0 || parts_no >= SAVE_BUILD_PARTS_MAX) {
        return;
    }
    build_parts_num[parts_no] = num;
    s16 &slot = build_parts_num[parts_no];
    if (slot < 0) {
        slot = 0;
    }
    if (slot > SAVE_BUILD_PARTS_NUM_MAX) {
        slot = SAVE_BUILD_PARTS_NUM_MAX;
    }
}

s16 CSaveData::GetBuildPartsNum(int parts_no) {
    if (parts_no < 0 || parts_no >= SAVE_BUILD_PARTS_MAX) {
        return 0;
    }
    return build_parts_num[parts_no];
}

s16 CSaveData::AddBuildPartsNum(int parts_no, int add) {
    if (parts_no < 0 || parts_no >= SAVE_BUILD_PARTS_MAX) {
        return 0;
    }
    build_parts_num[parts_no] += add;
    s16 &slot = build_parts_num[parts_no];
    if (slot < 0) {
        slot = 0;
    }
    if (slot > SAVE_BUILD_PARTS_NUM_MAX) {
        slot = SAVE_BUILD_PARTS_NUM_MAX;
    }
    return slot;
}

CEditData *CSaveData::GetEditData(int index) {
    if (index < 0 || index >= SAVE_EDIT_DATA_MAX) {
        return NULL;
    }
    return &edit_data[index];
}

int CSaveData::GetPlaceEditPartsNum(int parts_id) {
    int total = 0;
    for (int i = 0; i < SAVE_EDIT_DATA_MAX; i++) {
        total += edit_data[i].GetPartsNumID(parts_id);
    }
    return total;
}

CMapFlagData *CSaveData::GetMapFlag(int index) {
    if (index < 0 || index >= SAVE_MAP_FLAG_MAX) {
        return NULL;
    }
    return &map_flag[index];
}

void CSaveData::InitBitCtrl() {
    bit_ctrl = 0;
}

u8 CSaveData::SetBitCtrl(int bits) {
    u8 previous = bit_ctrl;
    bit_ctrl |= bits;
    return previous;
}

void CSaveData::ResetBitCtrl(int bits) {
    bit_ctrl &= ~bits;
}

int CSaveData::GetBitCtrl() { return this->bit_ctrl; }

int CSaveData::GetItem(int item_no, int num) {
    return user_data.GetItem(item_no, num);
}

void CSaveData::ForceBootTour(int day, int type) {
    tour.base_day = day;
    tour.start_day = day;
    tour.finish_day = day - SAVE_TOUR_CYCLE;
    tour.now_event = 1;
    tour.type = type;
    tour.count = 0;
}

int CSaveData::CheckEventDay(int day) {
    if (tour.base_day < 0) {
        return -1;
    }
    return day - tour.base_day;
}

void CSaveData::CheckTourBoot(int day) {
    int next_type;
    int elapsed;
    if (tour.base_day >= 0) {
        elapsed = CheckEventDay(day);
        if (tour.now_event == 1) {
            if (elapsed % SAVE_TOUR_CYCLE > 2) {
                tour.now_event = 0;
                tour.finish_day = elapsed;
            }
            return;
        }
        if (elapsed % SAVE_TOUR_CYCLE > 2) {
            tour.now_event = 0;
            return;
        }
        if (0 <= tour.base_day) {
            int start = tour.start_day;
            int previous = tour.finish_day;
            if (start <= previous && previous < start + SAVE_TOUR_DAYS && elapsed < start + SAVE_TOUR_DAYS) {
                tour.now_event = 0;
                return;
            }
        }
        tour.start_day = elapsed;
        tour.now_event = 1;
        tour.count = 0;
        next_type = tour.type + 1;
        if (GetBitFlag(0x1A8) != 0) {
            if (next_type >= 3) {
                next_type = 1;
            }
        } else {
            next_type = 1;
            if (GetBitFlag(0x158) == 0) {
                next_type = 0;
            }
        }
        tour.type = next_type;
        if (tour.type == 1) {
            CUserDataManager *user = &user_data;
            if (user != NULL) {
                user->fish_tournament.ResetRecord();
            }
        }
        if (tour.type == 2) {
            AquaFishFatigueClear();
        }
    }
}

s16 CSaveData::CheckNowTourEvent() {
    return tour.now_event;
}

s8 CSaveData::CheckNowTourType() {
    return tour.type;
}

s8 CSaveData::AddTourCountEtc(int add) {
    tour.count += add;
    if (tour.count < 0) {
        tour.count = 0;
    }
    if (tour.count > SAVE_TOUR_COUNT_MAX) {
        tour.count = SAVE_TOUR_COUNT_MAX;
    }
    return tour.count;
}

int CSaveData::GetTourCountEtc() {
    return tour.count;
}

void CSaveData::FinishTour() {
    tour.finish_day = day - tour.base_day;
    tour.now_event = 0;
    tour.count = 0;
}

void CSphidaData::Initialize(void) {
    memset(this, 0, sizeof(*this));
}

void CSphidaData::SetHorl(s32 hole) {
    now_hole = hole;
}

void CSphidaData::SetHorlScore(int score, int hole) {
    if (hole == -1) {
        hole = this->now_hole;
    }
    if (hole < 0 || hole > SPHIDA_HOLE_MAX - 1) {
        return;
    }
    this->hole_score[hole] = score;
}

int CSphidaData::GetNowHorl(void) {
    return now_hole;
}

int CSphidaData::GetHorlScore(int hole) {
    if (hole == -1) {
        int total = 0;
        for (int i = 0; i < SPHIDA_HOLE_MAX; i++) {
            total += hole_score[i];
        }
        return total;
    }
    if (hole > SPHIDA_HOLE_MAX - 1) {
        return 0;
    }
    return hole_score[hole];
}

void CSphidaData::ClearPlayerScore(int no) {
    SPHIDA_PLAYER_DATA *dst;
    SPHIDA_PLAYER_DATA *src;

    if (no < 0 || no >= SPHIDA_PLAYER_MAX) {
        return;
    }
    memset(&player[no], 0, sizeof(SPHIDA_PLAYER_DATA));
    dst = GetPlayerData(no);
    src = GetPlayerData(no + 1);
    if (src != NULL && dst != NULL) {
        memcpy(dst, src, sizeof(SPHIDA_PLAYER_DATA));
        ClearPlayerScore(no + 1);
    }
}

int CSphidaData::EnterScore() {
    int rank = -1;
    int i;
    int total_score;
    SPHIDA_PLAYER_DATA *record;

    total_score = GetHorlScore(-1);
    for (i = 0; i < SPHIDA_PLAYER_MAX; i++) {
        record = GetPlayerData(i);
        if (record->total_score <= total_score) {
            for (int j = SPHIDA_PLAYER_MAX - 2; j >= i; j--) {
                SPHIDA_PLAYER_DATA *moved = GetPlayerData(j);
                memcpy(&moved[1], moved, sizeof(SPHIDA_PLAYER_DATA));
            }
            rank = i;
            strcpy(record->name, player_name);
            record->unk_38 = 1;
            record->total_score = total_score;
            for (int k = 0; k < SPHIDA_HOLE_MAX; k++) {
                record->hole_score[k] = hole_score[k];
            }
            record->password_key = rand() % 255;
            break;
        }
    }
    if (rank < 0) {
        return 0x64;
    }
    return rank;
}

SPHIDA_PLAYER_DATA *CSphidaData::GetPlayerData(int no) {
    if (no < 0 || no >= SPHIDA_PLAYER_MAX) {
        return NULL;
    }
    return &player[no];
}

void CSphidaData::InitPlay(void) {
    now_hole = 0;
    memset(hole_score, 0, sizeof(hole_score) + sizeof(unk_145A));
    memset(player_name, 0, sizeof(player_name));
}

int GYORACE_DATA::IsUsed() {
    return (fish.item_no < 2) ^ 1;
}

void GYORACE_DATA::Init(void) {
    memset(this, 0, sizeof(*this));
}

void CGyoRaceData::Initialize(void) {
    memset(this, 0, sizeof(*this));
}

int CGyoRaceData::SearchSpace() {
    for (int i = 0; i < GYORACE_DATA_MAX; i++) {
        if (!(0 < data[i].fish.item_no)) {
            return i;
        }
    }
    return -1;
}

GYORACE_DATA *CGyoRaceData::SearchSpaceData(int *no) {
    int index = SearchSpace();
    if (index < 0) {
        return NULL;
    }
    if (no) {
        *no = index;
    }
    return &data[index];
}

GYORACE_DATA *CGyoRaceData::GetData(int no) {
    if (no < 0 || no >= GYORACE_DATA_MAX) {
        return NULL;
    }
    return &data[no];
}

CSubGameData::CSubGameData() {
    Initialize();
    sphida.Initialize();
    gyorace.Initialize();
}

void CSubGameData::Initialize(void) {
    memset(this, 0, sizeof(*this));
}

void CSubGameData::PlayEnable(int bits, int enable) {
    if (enable == 1) {
        play_enable |= bits;
    } else {
        play_enable &= ~bits;
    }
}

CSphidaData *CSubGameData::GetSphidaData() {
    return &this->sphida;
}

CGyoRaceData *CSubGameData::GetGyoRaceData() {
    return &this->gyorace;
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/savedata", at_453__DATA);
