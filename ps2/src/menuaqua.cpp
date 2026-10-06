#include "common.h"
#include "menuaqua.hpp"
#include "snd_mngr.hpp"
#include "mg_memory.hpp"
#include "mainloop.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_texture.hpp"
#include "mg_camera.hpp"
#include "gamedata.hpp"
#include "userdata.hpp"
#include "menucls1.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "menumain.hpp"
#include "savedata.hpp"
#include "scriptinterpreter.hpp"
#include "dataread.hpp"
#include "gamepad.hpp"
#include "scene.hpp"
#include "sound.hpp"
#include "mglib.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>

/**
 * Effects stored in one aquarium food table row.
 */
struct aqua_food_info {
    s16 item_no;             /**< Item used as fish food. */
    s8 unk_2;
    s8 parameter_change[4];   /**< Changes to the fish's racing parameters, in table order. */
    s8 unk_7;
    s16 timer_change;         /**< Change to the fish's timer. */
};
STATIC_ASSERT(sizeof(aqua_food_info) == 0xA);

/**
 * Pointers to the horizontal and vertical coordinates of one tank grid point.
 */
struct aqua_grid_cell {
    float *xz; /**< Horizontal position. */
    float *y;  /**< Vertical position. */
};

/**
 * Model images and the colour choices of one breed of fish.
 */
struct aqua_fish_info {
    s16 item_no;     /**< Fish item number. */
    u8 unk_2[2];
    const char *img_path; /**< Name used to build the IMG path. */
    s8 color_male;   /**< Male image colour. */
    s8 color_female; /**< Female image colour. */
    u8 unk_a[2];
};

/**
 * Position and first visible row of the saved racer list.
 */
struct gyorace_list_select {
    int cursor; /**< Selected row. */
    int top;    /**< First visible row. */
};

struct fish_prize_record;

/**
 * The prize entries and script parameters for one tournament group.
 */
struct fish_prize_group {
    int prize_count; /**< Number of prize entries in the group. */
    int unk_4[8];
    u8 unk_24[0x1C];
    fish_prize_record *prizes; /**< Entries in this group. */
};

/**
 * The three ranks of one tournament prize entry.
 */
struct fish_prize_record {
    int unk_0;
    FISH_PRIZE_INFO rank[3]; /**< Prize data for each rank. */
};

enum {
    short_flag_tour_count = 0x15,
    short_flag_wins_class0 = 0x16,
    short_flag_wins_class1 = 0x17,
    short_flag_wins_class2 = 0x18,
    short_flag_wins_class3 = 0x19,
    bit_flag_tour_cycled = 0x3F,
    fish_flag_won_class0 = 0x04,
    fish_flag_won_class1 = 0x08,
    fish_flag_won_class2 = 0x10,
    fish_flag_won_class3 = 0x20,
    omake_racer_slot_count = 6,
    gyorace_type_omake_racer = 6,
};

static const int max_fish_fatigue = 10000000;

/**
 * Copies a supplied name into the first message-name slot.
 */
inline void copy_name(ClsMes *window, char *name) {
    if (name != NULL) {
        strcpy(window->name[0], name);
    }
}

/**
 * Returns the number of quadwords needed to hold a byte count.
 */
static inline unsigned int align16_blocks(unsigned int bytes) {
    if (bytes & 0xF) {
        return (bytes >> 4) + 1;
    }
    return bytes >> 4;
}

static aqua_grid_cell *Get_aquarium_paul_table(int index);
static aqua_grid_cell *Get_aquarium_paul_table_xz(int x, int z);
static int local_aquarium_limmit_check(float *pos, float radius, int check_y, float height);
static aqua_food_info *GetEsaInfo(int item_no);
static int GetFishPath(int item_no, char *out);
static int CombineParam(int a, int b);
static int _GYORACE_LISTNUM(SPI_STACK *stack, int arg_count);
static int _GYORACE_DATA(SPI_STACK *stack, int arg_count);
static int _PRIZE_LISTNUM(SPI_STACK *stack, int arg_count);
static int _PRIZE_GROUP(SPI_STACK *stack, int arg_count);
static int _PRIZE(SPI_STACK *stack, int arg_count);
static void GyoraceCFGAnalyze(char *command);
static int SearchOmakeGyoracer(int slot);
static int CheckSameRacerFish(int fish_no);
static void ForceSetGyoList(void);

static CAquaFishEff * AquaFishEff[6];

static int AQUA_TITLE_X = 10;

static int AQUA_TITLE_Y = 6;

static int AQUA_TITLE_W = 158;

static int AQUA_TITLE_H = 66;

static mgCCamera * AquaCamera;

static aqua_grid_cell * aquarium_paul_table;

static aqua_fish_info aquafish_info[19] = {
    {320, {0, 0}, "f01a", 2, 1, {0, 0}},
    {321, {0, 0}, "f02a", 3, 2, {0, 0}},
    {322, {0, 0}, "f03a", 4, 3, {0, 0}},
    {323, {0, 0}, "f04a", 5, 4, {0, 0}},
    {324, {0, 0}, "f05a", 6, 9, {0, 0}},
    {325, {0, 0}, "f06a", 7, 18, {0, 0}},
    {326, {0, 0}, "f07a", 8, 5, {0, 0}},
    {327, {0, 0}, "f08a", 9, 16, {0, 0}},
    {328, {0, 0}, "f10a", 10, 8, {0, 0}},
    {329, {0, 0}, "f11a", 11, 10, {0, 0}},
    {330, {0, 0}, "f12a", 12, 11, {0, 0}},
    {331, {0, 0}, "f13a", 13, 12, {0, 0}},
    {332, {0, 0}, "f14a", 14, 13, {0, 0}},
    {333, {0, 0}, "f15a", 15, 17, {0, 0}},
    {334, {0, 0}, "f16a", 16, 15, {0, 0}},
    {335, {0, 0}, "f17a", 17, 6, {0, 0}},
    {336, {0, 0}, "f18a", 18, 7, {0, 0}},
    {310, {0, 0}, "f19a", 1, 14, {0, 0}},
    {65535, {0, 0}, NULL, 0, 0, {0, 0}}
};

static aqua_food_info esa_info[10] = {
    {312, 1, {0, 0, 0, 1}, 0, 7200},
    {313, 0, {0, 0, 1, 0}, 0, 7200},
    {314, 1, {1, 0, 0, 0}, 0, 7200},
    {315, 2, {0, 1, 0, 0}, 0, 7200},
    {316, 1, {1, 0, 0, 0}, 0, 7200},
    {317, 1, {0, 1, 0, 0}, 0, 7200},
    {318, 1, {0, 0, 0, 1}, 0, 7200},
    {319, 1, {0, 0, 1, 0}, 0, 7200},
    {360, 2, {0, 0, 0, 0}, 0, 7200},
    {-1, 0, {0, 0, 0, 0}, 0, 0}
};

static int Aqua_SpSndID;

static signed char GyoRaceAquariumNo;

static signed char GyoRaceClass;

static signed char GyoRaceProgressNum;

static signed char GyoRaceRankingData;

static short FishTournamentGoodsNum;

static fish_prize_group * FishTournamentGoods;

static mgCMemory * fish_prize_buildstack;

static fish_prize_record * spi_fish_prize_info;

static signed char FishTournamentGoodsType;

static s16 GyoracerIndexNo[6] = {-1, -1, -1, -1, -1, -1};

static s16 GyoracerTacticsNo[6] = {-1, -1, -1, -1, -1, -1};

static mgCMemory * spi_gyorace_stack;

static CGyoraceFishData * spi_gyorace_data;

static CGameDataUsed * spi_nowanalyze_gyorace_data;

static short spi_nowanalyze_gyorace_limmit;

static short spi_gyorace_counter;

#ifdef NONMATCHING
static fish_prize_group * spiFishTournamentGoods;
#endif

static SPI_TAG_PARAM gyorace_tag[3] = {
    {"NUM", _GYORACE_LISTNUM},
    {"DAT", _GYORACE_DATA},
    {NULL, NULL}
};

static SPI_TAG_PARAM gyoprize_tag[4] = {
    {"NUM", _PRIZE_LISTNUM},
    {"GRP", _PRIZE_GROUP},
    {"PRIZE", _PRIZE},
    {NULL, NULL}
};

static char * filename_4899[2] = {"gyop.cfg", "uofp.cfg"};

static char * GyoraceExeCfgBuffer;

static int GyoraceExeCfgBufferSize;

static mgCTexture * Tex_Aqualium;

static short GyoraceFishSelTexBk;

static signed char GyoraceFishSelectMode;

static gyorace_list_select GyoraceFishHaveListSelect;

static fish_prize_record * save_fish_prize_list;

static FISH_PRIZE_INFO fish_save_present[4][3];

CGameDataUsed *GyoraceFish;

CGyoRaceData *GyoraceData;

// Code (.text)
/**
 * Returns one of the sixty tank grid points.
 */
static aqua_grid_cell *Get_aquarium_paul_table(int index) {
    if ((index < 0) || (index >= 0x3C)) {
        index = 0;
    }
    return &aquarium_paul_table[index];
}
/**
 * Looks up the tank grid point at a column and row.
 */
static aqua_grid_cell *Get_aquarium_paul_table_xz(int x, int z) {
    return Get_aquarium_paul_table(x + z * 10);
}
/**
 * Keeps a fish inside the tank and reports the walls it reached.
 */
static int local_aquarium_limmit_check(float *pos, float radius, int check_y, float height) {
    int hit = 0;

    if (pos[0] < -31.0f + radius) {
        pos[0] = -31.0f + radius;
        hit |= 2;
    } else if (pos[0] > 31.0f - radius) {
        pos[0] = 31.0f - radius;
        hit |= 1;
    }
    if (check_y != 0) {
        if (pos[1] < 19.6f + radius) {
            pos[1] = 19.6f + radius;
            hit |= 8;
        } else if (pos[1] > (48.0f - radius) - height) {
            pos[1] = (48.0f - radius) - height;
            hit |= 4;
        }
    }
    if (pos[2] < -18.0f + radius) {
        pos[2] = -18.0f + radius;
        return hit | 0x20;
    }
    if (pos[2] > 18.0f - radius) {
        pos[2] = 18.0f - radius;
        hit |= 1;
    }
    return hit;
}

int GetUseableEsaNo(int *out) {
    int count;
    int i;

    count = 0;
    for (i = 0; esa_info[i].item_no > 0; i++) {
        count++;
        out[i] = esa_info[i].item_no;
    }
    if (GetUserItemHaveNum(0x168) <= 0) {
        out[8] = -1;
        count--;
    }
    return count;
}
/**
 * Finds the effect row for a fish food item.
 */
static aqua_food_info *GetEsaInfo(int item_no) {
    int i;

    for (i = 0; esa_info[i].item_no > 0; i++) {
        if (item_no == esa_info[i].item_no) {
            return &esa_info[i];
        }
    }
    return NULL;
}

void CBubble::Generate(int index) {
    AQUA_BUBBLE *particle = bubble + index;

    particle->drift_x = GetRandF(2.0f) - 1.0f;
    particle->drift_z = GetRandF(2.0f) - 1.0f;
    particle->pos[0] = origin[0] + particle->drift_x;
    particle->pos[1] = origin[1] + GetRandF(2.0f);
    particle->pos[2] = origin[2] + particle->drift_z;
    particle->pos[3] = 1.0f;
    particle->state = AQUA_BUBBLE_RISE;
    particle->phase = GetRandF(3.1415927f);
    particle->pattern = GetRandI(5);
    particle->drift_x = 0.1f * particle->drift_x;
    particle->drift_z = 0.1f * particle->drift_z;
    particle->alpha = 32.0f;
}

int CBubble::Generate(float *start_pos) {

    if (active != 0 && bubble_num <= (unsigned int)generated) {
        return 0;
    }
    if (bubble_num <= (unsigned int)generated) {
        return 0;
    }
    *(u_long128 *)origin = *(u_long128 *)start_pos;
    Generate(generated);
    generated += 1;
    active = 1;
    return 1;
}

void CBubble::SetTexture(mgCTexture *image, s32 u, s32 v) {
    texture = image;
    tex_u = u;
    tex_v = v;
}

void CBubble::Step() {
    static float wobble_amounts[5][2] = {0.019999999552965164f, 0.019999999552965164f, 0.009999999776482582f, 0.019999999552965164f, 0.03999999910593033f, 0.009999999776482582f, 0.019999999552965164f, 0.009999999776482582f, 0.009999999776482582f, 0.03999999910593033f};
    static float rise_speeds[5] = {0.05000000074505806f, 0.10000000149011612f, 0.15000000596046448f, 0.20000000298023224f, 0.25f};
    unsigned int finished;
    unsigned int index;
    AQUA_BUBBLE *particle;
    float *amp;

    if (active == 0) {
        return;
    }
    finished = 0;
    index = 0;
    for (; index < bubble_num; index++) {
        particle = &bubble[index];

        if (particle->state == AQUA_BUBBLE_RISE) {
            float depth = (particle->pos[1] - origin[1]) / height;
            float wobble;
            float spread;
            float jitter;

            particle->pos[1] = particle->pos[1] + (0.18f * depth + rise_speeds[particle->pattern]);
            amp = wobble_amounts[particle->pattern];
            wobble = (0.5f * depth + GetRandF(1.0f)) * sinf(particle->phase);
            depth *= 0.14f;
            spread = depth;
            float dx = amp[0] * wobble + particle->drift_x * spread;
            float dz = amp[1] * wobble + particle->drift_z * spread;
            jitter = 1.0f + GetRandF(spread);
            particle->pos[0] = particle->pos[0] + dx * jitter;
            particle->pos[2] = particle->pos[2] + dz * jitter;
            particle->phase += 0.15707964f;
            mgAngleLimit(particle->phase);
            if (particle->pos[1] >= surface_y) {
                particle->state = AQUA_BUBBLE_POP;
                particle->pos[1] = particle->pos[1] - GetRandF(0.1f);
                particle->pattern = GetRandI(10) + 16;
            }
        }
        if (particle->state == AQUA_BUBBLE_POP) {
            particle->pos[0] += particle->drift_x;
            particle->pos[2] += particle->drift_z;
            particle->pattern -= 1;
            particle->alpha = particle->alpha - 1.0f;
            if ((int)particle->pattern <= 0) {
                if (one_shot == 0) {
                    Generate(index);
                } else {
                    particle->state = AQUA_BUBBLE_END;
                }
            }
        }
        if (particle->state == AQUA_BUBBLE_END) {
            finished += 1;
        }
    }
    if (one_shot == 1 && bubble_num <= finished) {
        active = 0;
        generated = 0;
    }
}

void CBubble::Draw() {
    if (active == 0) {
        return;
    }
    mgCDrawPrim prim;
    int left[4];
    int right[4];
    unsigned int index;
    AQUA_BUBBLE *particle;

    SetSpriteEnv(&prim, 4);
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.Begin(MG_PRIM_SPRITE);
    prim.Texture(texture);
    index = 0;
    for (; index < bubble_num; index++) {
        particle = &bubble[index];

        if (particle->state != AQUA_BUBBLE_END &&
            mgTransWorldPrim3DSprite(left, right, particle->pos, 0.4f, 0.4f, 0) != 0) {
            prim.Color(0x80, 0x80, 0x80, (int)particle->alpha);
            prim.TextureCrd(tex_u, tex_v);
            prim.Vertex4(left);
            prim.TextureCrd(tex_u + 0x10, tex_v + 0x10);
            prim.Vertex4(right);
        }
    }
    prim.End();
}

void CBubble::Initialize(mgCMemory *memory, float *origin, int num, float surface) {
    unsigned int bytes;
    unsigned int blocks;
    unsigned int i;

    bubble_num = num;
    surface_y = surface;
    this->origin[0] = origin[0];
    this->origin[1] = origin[1];
    this->origin[2] = origin[2];
    bytes = bubble_num * sizeof(AQUA_BUBBLE);
    if (bytes & 0xF) {
        blocks = (bytes >> 4) + 1;
    } else {
        blocks = bytes >> 4;
    }
    bubble = (AQUA_BUBBLE *)memory->Alloc(blocks);
    i = 0;
    height = surface_y - this->origin[1];
    for (; i < bubble_num; i++) {
        Generate(i);
        float rise = GetRandF(height);
        bubble[i].pos[1] = this->origin[1] + rise;
    }
    active = 1;
    one_shot = 0;
    generated = 0;
}

void CBubble::RunOff(void) {
    active = 0;
    generated = 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", GetChildFishNo__Fii);
float SetFishAdjustScale(int length, int item_no, float base_scale, float max_scale) {
    CDataBreedFish *record;
    float scale;
    float result;

    scale = base_scale;
    record = GetBreedFishInfoData(item_no);
    if (record != NULL) {
        scale *= (float)length / record->size;
    }
    result = scale;
    if (max_scale <= scale) {
        result = max_scale;
    }
    return result;
}

void CAquaFishActionParam::Initialize(void) {
    memset(this, 0, sizeof(*this));
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", __ct__9CAquaFishFv);
void CAquaFish::Initialize() {
    CCharacter2::Initialize();
    mgZeroVector(move);
    mgZeroVector(target_pos);
    mgZeroVector(target_rot);
    mgZeroVector(turn);
    turn[0] = 0.03141593f;
    think_mode = AQUA_FISH_THINK_REST;
    think_timer = GetRandI(0x29) + 0xA;
    if (data != NULL) {
        data->Init();
    }
    data = NULL;
    pair_no = -1;
    col_flags = 0;
    radius = 0;
    wall_time = 0;
    eat_item = 0;
    unk_922 = GetRandI(8);
    action.Initialize();
    flash_count = 0;
    aqua_no = -1;
}

void CAquaFish::SetLiveParam(CGameDataUsed *item) {

    u16 value;
    int base;

    data = item;
    value = data->data.fish.param[3];
    base = value / 10;
    fatigue_max = (base + (GetRandI(0x14) + 0x1A)) * 0x14;
    fatigue = 0;
}

void CAquaFish::SetAdjustScale() {
    float hi = 0.95f;
    float scale = SetFishAdjustScale(data->data.fish.size, data->item_no,
                                   0.6f, hi);
    SetScale(scale, scale, scale);
    radius = body_height * (scale / 0.6f);
}

int CAquaFish::AddFatigue(int add) {
    fatigue += add;
    if (fatigue < 0) {
        fatigue = 0;
    }

    if (max_fish_fatigue < fatigue) {
        fatigue = max_fish_fatigue;
    }
    return fatigue;
}

void CAquaFish::GetPosition2D(int *pos) {
    float view[4][4];
    float camera_pos[4];
    int screen_int[4];
    float screen[4];

    if (AquaCamera != NULL) {
        AquaCamera->GetCameraMatrix(view);
        AquaCamera->GetPos(camera_pos);
        mgSetViewMatrix(view, camera_pos);
        GetPosition(screen);
        mgTransWorldScreen(screen_int, screen);
        sceVu0ITOF4Vector(screen, screen_int);
        pos[0] = (int)screen[0];
        pos[1] = (int)screen[1];
    }
}

void CAquaFish::GetDirVect(float *dir) {
    float rot[4];
    sceVu0FVECTOR forward = {0.0f, 0.0f, 1.0f, 1.0f};
    float matrix[4][4];

    GetRotation(rot);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, rot[1]);
    sceVu0ApplyMatrix(dir, matrix, forward);
    dir[3] = 1.0f;
}

void CAquaFish::NormalGetNextVelo(float speed) {
    float to_target[4];

    GetPosition(to_target);
    sceVu0SubVector(to_target, target_pos, to_target);
    sceVu0Normalize(to_target, to_target);
    sceVu0ScaleVectorXYZ(to_target, to_target, speed);
    *(u_long128 *)move = *(u_long128 *)to_target;
    move[3] = 1.0f;
}

void CAquaFish::NormalGetNextRotY() {
    float dir[4];
    float pos[4];

    GetPosition(pos);
    sceVu0SubVector(dir, target_pos, pos);
    sceVu0Normalize(dir, dir);
    target_rot[1] = mgAngleLimit(atan2f(dir[0], dir[2]));
}

void CAquaFish::NormalGetNextRot() {
    float dir[4];
    float rot[4];
    float pos[4];

    GetRotation(rot);
    GetPosition(pos);
    sceVu0SubVector(dir, target_pos, pos);
    sceVu0Normalize(dir, dir);
    target_rot[0] = mgAngleLimit(atan2f(sqrt(dir[0] * dir[0] + dir[2] * dir[2]), dir[1]));
    if (0.0f < dir[1]) {
        target_rot[0] = -target_rot[0];
    }
    target_rot[1] = mgAngleLimit(atan2f(dir[0], dir[2]));
}

float CAquaFish::CalcMoveSpeed(float speed) {
    BREEDFISH_USED *stats;
    float ceiling;
    int parameter;

    if (data == NULL) {
        stats = NULL;
    } else {
        stats = &data->data.fish;
    }
    ceiling = 3.0f;
    switch (think_mode) {
        case AQUA_FISH_THINK_SWIM:
            parameter = GetRandI(3);
            speed *= 0.55f + 0.02f * (float) stats->param[parameter];
            break;
        case AQUA_FISH_THINK_BATTLE:
            speed *= 0.65f + 0.024f * (float) stats->param[0];
            ceiling = 4.4f;
            break;
        case AQUA_FISH_THINK_BATTLE_REST:
            speed *= 0.65f + 0.024f * (float) stats->param[0];
            break;
        case AQUA_FISH_THINK_LOVE_SEARCH:
        case AQUA_FISH_THINK_LOVE_CHASE:
            speed *= 0.4f + 0.02f * (float) stats->param[3];
            break;
    }
    if (ceiling < speed) {
        speed = ceiling;
    }
    return speed;
}

void CAquaFish::NextRootNormal() {
    int direction;
    int grid_x;
    int grid_z;
    int level;
    float sway;
    int i;
    aqua_grid_cell *cell;
    float dir[4];
    float pos[4];
    float rot[4];

    direction = GetRandI(2);
    grid_x = GetRandI(8) + 1;
    grid_z = GetRandI(5) + 1;
    level = GetRandI(4);
    sway = GetRandF(3.1415927f) - 1.5707964f;
    route_num = GetRandI(10) + 16;
    for (i = 0; i < route_num; i++) {
        cell = Get_aquarium_paul_table_xz(grid_x, grid_z);
        route[i][0] = cell->xz[0] + GetRandF(sinf(sway));
        route[i][2] = cell->xz[2] + GetRandF(sinf(sway));
        route[i][1] = cell->y[level];
        if (direction == 0) {
            grid_x += GetRandI(3) + 1;
            if (grid_x >= 10) {
                grid_x = 9;
                grid_z = GetRandI(2) + 4;
                direction ^= 1;
            }
        } else if (direction == 1) {
            grid_x -= GetRandI(3) + 1;
            if (grid_x <= 0) {
                grid_x = 0;
                grid_z = GetRandI(2);
                direction ^= 1;
            }
        }
        if (grid_z < 0) {
            grid_z = 0;
        }
        if (grid_z >= 6) {
            grid_z = 5;
        }
        level += GetRandI(3) - 1;
        if (level < 0) {
            level = 0;
        }
        if (level >= 4) {
            level = 3;
        }
    }
    route_no = 0;
    route_time = 0;
    *(u_long128 *)target_pos = *(u_long128 *)route[0];
    GetPosition(pos);
    GetRotation(rot);
    sceVu0SubVector(dir, target_pos, pos);
    sceVu0Normalize(dir, dir);
    target_rot[1] = mgAngleLimit(atan2f(dir[0], dir[2]));
}

void CAquaFish::MoveActionRound() {
    static float wall_turns[8] = {1.5707963705062866f, -3.1101768016815186f, 0.0f, -1.5707963705062866f, 0.0f, 1.5707963705062866f, -1.5707963705062866f, -3.1101768016815186f};
    float pos[4];
    float *turn;
    float yaw;

    GetPosition(pos);
    sceVu0FVECTOR dir = {0.0f, 0.0f, 0.0f, 1.0f};
    yaw = target_rot[1];
    turn = &wall_turns[round.dir * 4];
    if (pos[0] < -31.0f * round.width) {
        if (pos[2] < -18.0f * round.depth) {
            yaw = turn[0];
        } else if (18.0f * round.depth < pos[2]) {
            yaw = turn[1];
        }
    } else if (31.0f * round.width < pos[0]) {
        if (pos[2] < -18.0f * round.depth) {
            yaw = turn[2];
        } else if (18.0f * round.depth < pos[2]) {
            yaw = turn[3];
        }
    } else if (col_flags & AQUA_FISH_COL_WALL) {
        wall_time += 1;
        if (wall_time > 200) {
            if ((round.dir == 0 && 0.0f <= pos[2]) || (round.dir == 1 && pos[2] < 0.0f)) {
                yaw = -1.5707964f;
            } else {
                yaw = 1.5707964f;
            }
            wall_time = 0;
        }
    } else {
        wall_time = 0;
    }
    action.speed += 0.01f;
    if (action.max_speed <= action.speed) {
        action.speed = action.max_speed;
    }
    target_rot[1] = mgAngleLimit(yaw);
    GetDirVect(dir);
    sceVu0Normalize(dir, dir);
    round.wave += 0.05235988f;
    round.wave = mgAngleLimit(round.wave);
    dir[1] += 0.1f * sinf(round.wave);
    if (col_flags & AQUA_FISH_COL_OBJECT) {
        dir[1] += 0.2f;
    }
    sceVu0ScaleVectorXYZ(dir, dir, action.speed);
    *(u_long128 *)move = *(u_long128 *)dir;
}

void CAquaFish::MoveActionBattle() {
    CAquaFish *foe;
    CAquaFishEff *eff;
    float foe_pos[4];
    float pos[4];
    sceVu0FVECTOR offset;
    float amount;
    float angle;
    float delta;

    foe = action.target;
    if (foe != NULL && swim_mode == AQUA_FISH_SWIM_POINT) {
        foe->GetPosition(foe_pos);
        if (action.phase == 0) {
            *(u_long128 *)target_pos = *(u_long128 *)foe_pos;
            NormalGetNextVelo(CalcMoveSpeed(0.24f));
            NormalGetNextRot();
            think_timer -= 1;
        }
        if (action.phase == 1) {
            charge_angle = 0.0f;
            action.phase = 2;
            SetMotion("\203o\203g\203\213\201i\214\263\213C\201j", 0);
            SetStep(1.0f);
        }
        if (action.phase == 2) {
            angle = charge_angle;
            angle += 0.10471976f;
            charge_angle = angle;
            if (3.1415927f < angle) {
                charge_angle = 3.1415927f;
                *(u_long128 *)target_pos = *(u_long128 *)foe_pos;
            }
            if (charge_angle < 3.1415927f) {
                charge_angle += 0.10471976f;
                delta = charge_angle - 2.9321532f;
                if (delta < 0.0f) {
                    delta = -delta;
                }
                if (delta <= 0.10471976f) {
                    eff = AquaFishEff[aqua_no];
                    eff->type = 4;
                    eff->timer = 0xFA0;
                }
                amount = 0.5f * sinf(mgAngleLimit(1.5707964f + charge_angle));
                GetPosition(pos);
                sceVu0SubVector(offset, foe_pos, pos);
                sceVu0Normalize(offset, offset);
                sceVu0ScaleVectorXYZ(offset, offset, amount);
                offset[3] = 1.0f;
                sceVu0AddVector(target_pos, foe_pos, offset);
            } else if (col_flags & AQUA_FISH_COL_TARGET) {
                charge_angle = 0.0f;
                action.phase = 0;
                eff = AquaFishEff[aqua_no];
                eff->type = 0;
                eff->timer = -1;
                SetStep(0.5f);
            } else {
                NormalGetNextVelo(CalcMoveSpeed(0.3f));
                NormalGetNextRot();
                think_timer -= 1;
            }
        }
    }
    if (foe == NULL) {
        NextThink(AQUA_FISH_THINK_BATTLE, NULL);
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", NextThink__9CAquaFishFiP16NEXT_THINK_PARAM);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", ParamStep__9CAquaFishFv);
void CAquaFish::FishDraw() {
    if (data != NULL) {
        unsigned int hp = data->data.fish.hp;
        float saved[4];

        mgGetAmbient(saved);
        sceVu0FVECTOR bright = {140.0f, 64.0f, 64.0f, 128.0f};
        if (hp < 0x1E && flash_count < 0xB) {
            bright[0] = 172.0f;
            mgSetAmbient(bright);
        }
        DrawDirect();
        mgSetAmbient(saved);
    }
}

void CAquaFishEff::Initialize(void) {
    fish = NULL;
    texture = NULL;
    type = 0;
    timer = 0;
}

void CAquaFishEff::StartFishEffect(int type) {
    static int effect_times[6] = {0, 250, 250, 25000, 250, 0};
    this->type = (short)type;
    timer = effect_times[this->type];
}

void CAquaFishEff::Step() {
    if ((fish != 0) && (type != 0)) {
        timer -= 1;
        if (timer <= 0) {
            timer = 0;
            type = 0;
        }
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Draw__12CAquaFishEffFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", __ct__9CFishFoodFv);
void CFishFood::SetDropPosition(float *pos) {
    *(u_long128 *)this->pos = *(u_long128 *)pos;
    SetPosition(this->pos);
}

void CFishFood::Drop() {
    state = FISH_FOOD_DROP;
    sway = 2.5f + GetRandF(1.6f);
    fall_time = 0;
    sway_phase = 0;
    spin[0] = 0.015707964f + GetRandF(3.1415927f) / 34.0f;
    spin[2] = 0.015707964f + GetRandF(3.1415927f) / 34.0f;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Step__9CFishFoodFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", DrawEsaDropRoot__FP9CFishFoodf);
void AquaMesDispAdjustPos(ClsMes *mes, int *pos) {
    int width;
    int height;

    if (mes != NULL) {
        width = mes->line_w[0];
        height = mes->line_w[1];
        if (width < height) {
            width = height;
        }
        mes->abs_win.x = pos[0] - (width >> 1);
        mes->abs_win.y = pos[1];
        if (mes->abs_win.x < 0x2C) {
            mes->abs_win.x = 0x2C;
        }
        if (mes->abs_win.y < 0x2C) {
            mes->abs_win.y = 0x2C;
        }
        if (mgScreenWidth - 0x36 < mes->abs_win.x + width) {
            mes->abs_win.x = mgScreenWidth - 0x36 - width;
        }
        if (mgScreenHeight - 0x2C < mes->abs_win.y + 0x30) {
            mes->abs_win.y = mgScreenHeight - 0x5C;
        }
    }
}
CAquaMes::CAquaMes() {
    Initialize(NULL);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Initialize__8CAquaMesFP9mgCMemory);
void CAquaMes::SettingAquaMes(int aqua_no) {
    switch (aqua_no) {
        case 0:
            SetTitleId(0);
            menu_mes->MakeMesWin(0xA);
            break;
        case 1:
            SetTitleId(1);
            menu_mes->MakeMesWin(0xB);
            break;
        case 2:
            SetTitleId(2);
            menu_mes->MakeMesWin(0xC);
            break;
    }
    SetCtrlHelpId(0x32);
}

void CAquaMes::SetTitleId(int id) {
    ClsMes *window;
    int half_w;
    int half_h;

    title_id = id;
    title_mes->MakeMesWin(title_id);
    title_mes->Step();
    window = title_mes;
    half_w = AQUA_TITLE_W / 2;
    window->abs_win.x = (AQUA_TITLE_X + half_w) - (window->line_w[0] >> 1);
    window = title_mes;
    half_h = AQUA_TITLE_H / 2;
    window->abs_win.y = ((AQUA_TITLE_Y + half_h) - (window->font_h >> 1)) + 2;
}

int CAquaMes::AddMenuCursor(int add, int num) {
    int before = menu_cursor;

    menu_cursor = before + add;
    if (menu_cursor < 0) {
        menu_cursor = num - 1;
    }
    if (menu_cursor >= num) {
        menu_cursor = 0;
    }
    if (before != menu_cursor) {
        return 1;
    }
    return 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", SetQuestionId__8CAquaMesFiii);
int CAquaMes::AddQuestionCursor() {
    int step = 0;
    int before = question_cursor;
    int base;
    int now;
    ClsMes *window;

    if (GamePad.Down(0x1000)) {
        step--;
    } else if (GamePad.Down(0x4000)) {
        step++;
    }
    question_cursor += step;
    if (question_cursor < 0) {
        question_cursor = 0;
    }
    if (question_num <= question_cursor) {
        question_cursor = question_num - 1;
    }
    now = question_cursor;
    if (before != now) {
        return 1;
    }
    window = question_mes;
    base = window->select_top;
    if (base < 0) {
        base = 0;
    }
    now = base + now;
    if (window->select < 0) {
        window->cursor_time = 0;
    }
    window->select = now;
    return 0;
}

void CAquaMes::SetCtrlHelpId(int id) {
    int half_w;
    int bottom;
    ClsMes *window;

    help_mes->mes_no = -1;
    help_mes->MakeMesWin(id);
    help_mes->Step();
    half_w = mgScreenWidth >> 1;
    bottom = mgScreenHeight - 0x28;
    window = help_mes;
    window->line_pos[0][0] = (half_w - window->line_w[0]) - 0x14;
    window->line_pos[0][1] = bottom;
    window->line_pos_on[0] = 1;
    window = help_mes;
    bottom = mgScreenHeight - 0x28;
    window->line_pos[1][0] = half_w + 0x32;
    window->line_pos[1][1] = bottom;
    window->line_pos_on[1] = 1;
}

void CAquaMes::SetInfoMsgID(int id) {
    info_mes->mes_no = -1;
    info_mes->MakeMesWin(id);
    info_mes->Step();
}

void CAquaMes::EatMessage(int id, CAquaFish *fish) {
    char *name;
    int pos[2];

    name = fish->data->GetName(0);
    if (name != NULL) {
        copy_name(fish_mes, name);
    }
    fish_mes->mes_no = -1;
    fish_mes->MakeMesWin(id);
    fish_mes_time = 0xFA;
    fish->GetPosition2D(pos);
    AquaMesDispAdjustPos(fish_mes, pos);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", ChangeManMessage__8CAquaMesFP9CAquaFish);
void CAquaMes::DeadMessage(CAquaFish *fish) {
    char *name;
    int pos[2];

    if (fish != NULL) {
        name = fish->data->GetName(0);
        fish_mes->mes_no = -1;
        copy_name(fish_mes, name);
        fish_mes->MakeMesWin(0x134);
        fish_mes->Step();
        fish_mes_time = 0xFA;
        fish->GetPosition2D(pos);
        AquaMesDispAdjustPos(fish_mes, pos);
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Step__8CAquaMesFv);
void CAquaMes::Draw() {
    mgCTextureManager *manager;
    mgCTexture *cursor_texture;

    mgTexManager.ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)NULL);
    if (menu_mes != NULL && menu_draw != 0) {
        menu_mes->DrawMesWin();
    }
    if (guide_mes != NULL && guide_draw != 0) {
        guide_mes->DrawMesWin();
    }
    if (question_mes != NULL && question_draw != 0) {
        question_mes->DrawMesWin();
    }
    if (help_mes != NULL && help_draw != 0) {
        help_mes->DrawMesWin();
    }
    if (info_mes != NULL && info_draw != 0) {
        info_mes->DrawMesWin();
    }
    if (fish_mes != NULL && 0 < fish_mes_time) {
        fish_mes->DrawMesWin();
    }
    if (cursor_draw != 0) {
        manager = &mgTexManager;
        cursor_texture = manager->GetTexture("mnmain", -1);
        if (cursor_texture != NULL) {
            manager->ReloadTexture(cursor_texture->block, (sceVif1Packet *)NULL);
            MenuCursorDraw(cursor_texture, cursor_pos, 0.0f, 0x80);
        }
    }
}

void CAquaMes::DrawTitleMes() {
    ClsMes *window;

    mgTexManager.ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)NULL);
    window = title_mes;
    if ((window != NULL) && (title_draw != 0)) {
        window->DrawMesWin();
    }
}
/**
 * Builds the character model path for a fish item.
 */
static int GetFishPath(int item_no, char *out) {
    char *file_name;

    file_name = GetItemFileName(item_no, 1);
    if ((file_name == 0) || (out == NULL)) {
        return 1;
    }
    sprintf(out, "menu/aqua/fish/%s", file_name);
    return 0;
}

int GetFishImgPath(char *path, int fish_no, BREEDFISH_USED *fish) {
    aqua_fish_info *info;

    if (fish_no <= 0) {
        return 0;
    }
    if (fish == NULL) {
        return 0;
    }
    if (path == NULL) {
        return 0;
    }
    for (info = aquafish_info; info->item_no > 0; info++) {
        if (info->item_no == fish_no) {
            sprintf(path, "menu/aqua/fish/%s%d.img", info->img_path, fish->unk_3a);
            return 1;
        }
    }
    return 0;
}

int GetFishImageColor(int fish_no, int which) {
    aqua_fish_info *info = aquafish_info;

    for (; info->img_path != 0; info++) {
        if (which == 0 && info->item_no == fish_no) {
            return info->color_male;
        }
        if (which == 1 && info->item_no == fish_no) {
            return info->color_female;
        }
    }
    return 0;
}

int FishIMGReplace(u_long128 *buffer, CCharacter2 *chara, int fish_no, BREEDFISH_USED *fish) {
    int size;
    char path[0x80];
    char saved_dir[0x6C];
    u8 *texture_buffer;

    if (buffer == NULL || chara == NULL || fish == NULL) {
        return 0;
    }
    if (fish->flags & 2) {
        return 0;
    }
    if (GetFishImgPath(path, fish_no, fish) != 0) {
        GetCurrentDir(saved_dir);
        SetCurrentDir(NULL);
        if (LoadFile2(path, buffer, &size, 0) != 0) {
            texture_buffer = (u8 *)chara->images[0];
            mgTexManager.DeleteBlock(chara->texture_block);
            memcpy(texture_buffer, buffer, size);
            mgTexManager.EnterIMGFile( texture_buffer, chara->texture_block, NULL, NULL);
        }
        SetCurrentDir(saved_dir);
    }
    return 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", DrawFishParam__FiiP10mgCTextureP13CGameDataUsed);
CAquarium::CAquarium() {
    int i;

    for (i = 0; i < 13; i++) {
        tex_block[i] = -1;
    }
    Clear();
}

void CAquarium::Clear() {
    int i;
    mode = 0;
    aqua_stack.stack_used = 0;
    aqua_stack.lock = 0;
    ground_frame = 0;
    glass_frame = 0;
    aqua_frame = 0;
    mizu_frame = 0;
    ground_tex_block = -1;
    glass_tex_block = -1;
    aqua_tex_block = -1;
    mes_stack.stack_used = 0;
    mes_stack.lock = 0;
    menu_tex_block = -1;
    suimen_frame = 0;
    water = 0;
    water_tex_block = -1;
    ripple = 0.1f;
    food = NULL;
    food_tex_block = -1;
    unk_326 = 0;
    unk_386 = -1;
    drop_root_draw = 0;
    naka_stack.stack_used = 0;
    naka_stack.lock = 0;
    naka_frame = 0;
    food_stack.stack_used = 0;
    food_stack.lock = 0;
    MenuDeleteTextureBlock(tex_block);
    for (i = 0; i < 13; i++) {
        tex_block[i] = -1;
    }
    for (i = 0; i < 6; i++) {
        fish_stack[i].stack_used = 0;
        fish_stack[i].lock = 0;
        fish[i] = NULL;
        fish_tex_block[i] = -1;
    }
    sel_fish = 0;
    Aqua_SpSndID = 0;
    sndInitPort(8);
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Initialize__9CAquariumFP9mgCMemoryPi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", LoadFish__9CAquariumFiP13CGameDataUsed);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", SettingAqua__9CAquariumFv);
int CalcFishParam(BREEDFISH_USED *fish) {
    BREEDFISH_USED *body = fish;
    int sum;
    if (body == NULL) {
        return 0;
    }
    sum = 0;
    sum += body->param[0];
    sum += body->param[1];
    sum += body->param[2];
    sum += body->param[3];
    sum += body->param[4];
    return sum;
}
/**
 * Combines one pair of parent fish parameters.
 */
static int CombineParam(int a, int b) {
    int result;
    if (a >= b) {
        result = a + b / 7;
    } else {
        result = b + a / 7;
    }
    if (result < 0) {
        result = 0;
    }
    if (result >= 100) {
        result = 100;
    }
    return result;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", CombineFish__9CAquariumFii);
int CAquarium::GetBattleTarget(int no) {
    int round;
    int i;
    for (round = 0; round < 3; round++) {
        for (i = 0; i < 6; i++) {
            if (i != no && fish[i] != NULL && GetRandI(3) == 0) {
                return i;
            }
        }
    }
    return -1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Thinking__9CAquariumFi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", ColCheck__9CAquariumFi);
int CAquarium::InitSelFish() {
    int i;
    sel_fish = -1;
    mes.cursor_draw = 1;
    for (i = 0; i < 6; i++) {
        if (fish[i] != NULL) {
            float x, y;
            sel_fish = i;
            x = (float)mes.menu_mes->cursor_x;
            y = (float)mes.menu_mes->cursor_y;
            mes.cursor_pos[0] = x;
            mes.cursor_target[0] = x;
            mes.cursor_pos[1] = y;
            mes.cursor_target[1] = y;
            mes.cursor_snap = 1;
            SelFishSetCursor();
            return 0;
        }
    }
    mes.cursor_draw = 0;
    return 1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", SelectFish__9CAquariumFi);
void CAquarium::SelFishSetCursor() {
    float view[4][4];
    float camera_pos[4];
    float fish_pos[4];
    int screen_int[4];
    float screen[4];

    if (sel_fish >= 0 && fish[sel_fish] != NULL) {
        AquaCamera->GetCameraMatrix(view);
        AquaCamera->GetPos(camera_pos);
        mgSetViewMatrix(view, camera_pos);
        ((CAquaFish *)fish[sel_fish])->GetPosition(fish_pos);
        mgTransWorldScreen(screen_int, fish_pos);
        sceVu0ITOF4Vector(screen, screen_int);
        screen[0] -= 50.0f;
        float y = screen[1] - 12.0f;
        screen[1] = y;
        mes.cursor_target[0] = screen[0];
        mes.cursor_target[1] = y;
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Step__9CAquariumFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", Draw__9CAquariumFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", MenuAquaInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", MenuAquaKey__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", MenuAquaDraw__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", MenuGyoraceFishSelInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", MenuGyoraceFishSelKey__Fv);
void MenuGyoraceFishSelDraw() {
    mgCTextureManager *tex_manager = &mgTexManager;
    mgRect<int> dest;
    mgRect<int> source;
    tex_manager->ReloadTexture(MenuBGTextureBlock, (sceVif1Packet *)0);
    source.Set(0, 0, (int)mgScreenWidth >> 1, mgScreenHeight >> 1);
    dest.Set(0, 0, mgScreenWidth, mgScreenHeight);
    PrimQuad(MenuFrameTex, dest, source, 0x80, 0x80, 0x80, 0x80);
    if (Tex_Aqualium != 0) {
        tex_manager->ReloadTexture(GyoraceFishSelTexBk, (sceVif1Packet *)0);
        DrawFishParam(((int)mgScreenWidth - 0x14A >> 1) + 10,
                                                        mgScreenHeight - 0x8E, Tex_Aqualium,
                                                        GyoraceFish);
    }
    if (GyoraceFishSelectMode != 0) {
        tex_manager->ReloadTexture(MenuArg.mes_tex_block, (sceVif1Packet *)0);
        MenuDCMsg[0]->DrawMsg();
        MenuDCMsg[1]->DrawMsg();
    }
}

CGameDataUsed *GetGyoRaceFish() {
    return GyoraceFish;
}

void SetGyoRaceAquariumNo(int no) {
    GyoRaceAquariumNo = no;
}

int GetGyoRaceAquariumNo() {
    return GyoRaceAquariumNo;
}

void SetGyoRaceClass(int race_class) {
    GyoRaceClass = race_class;
}

int GetGyoRaceClass() {
    return GyoRaceClass;
}

void SetGyoRaceNo(int no) {
    GyoRaceProgressNum = no;
    if (GyoRaceProgressNum < 0) {
        GyoRaceProgressNum = 0;
    }
}

int GetGyoRaceNo() {
    return GyoRaceProgressNum;
}

void SetGyoRaceRanking(int rank) {
    int race_class;
    CSaveData *save_data;
    CGameDataUsed *fish;
    int flag_no;
    int wins;

    GyoRaceRankingData = rank;
    race_class = GetGyoRaceClass();
    save_data = GetSaveData();
    if (GetGyoRaceNo() != 2) {
        return;
    }
    fish = GetGyoRaceFish();
    if (rank <= 2) {

        fish->data.fish.unk_3d |= 1;
    }
    if (rank == 0 && fish != NULL) {
        flag_no = short_flag_wins_class0;
        if (race_class == 0) {
            fish->data.fish.flags |= fish_flag_won_class0;
        }
        if (race_class == 1) {
            fish->data.fish.flags |= fish_flag_won_class1;
            flag_no = short_flag_wins_class1;
        }
        if (race_class == 2) {
            fish->data.fish.flags |= fish_flag_won_class2;
            flag_no = short_flag_wins_class2;
        }
        if (race_class == 3) {
            fish->data.fish.flags |= fish_flag_won_class3;
            flag_no = short_flag_wins_class3;
        }
        wins = save_data->GetShortFlag(flag_no) + 1;
        if (wins > 2) {
            wins = 2;
        }
        save_data->SetShortFlag(flag_no, wins);
    }
}

int GetGyoRaceRanking() {
    return GyoRaceRankingData;
}
/**
 * Allocates the fish entries for one race class.
 */
static int _GYORACE_LISTNUM(SPI_STACK *stack, int arg_count) {
    int race_class;
    int count;
    unsigned int blocks;

    race_class = spiGetStackInt(stack++);
    spi_nowanalyze_gyorace_limmit = spiGetStackInt(stack);
    spi_gyorace_data->fish_num[race_class] = spi_nowanalyze_gyorace_limmit;
    count = spi_nowanalyze_gyorace_limmit;

    if (((unsigned int)count * sizeof(CGameDataUsed)) & 0xF) {
        blocks = (((unsigned int)count * sizeof(CGameDataUsed)) >> 4) + 1;
    } else {
        blocks = ((unsigned int)count * sizeof(CGameDataUsed)) >> 4;
    }

    spi_gyorace_data->fish[race_class] = new ((u_long128 *)spi_gyorace_stack->Alloc(blocks + 2)) CGameDataUsed[count];
    spi_nowanalyze_gyorace_data = spi_gyorace_data->fish[race_class];
    spi_gyorace_counter = 0;
    return 1;
}
/**
 * Fills the next race fish entry from the script arguments.
 */
static int _GYORACE_DATA(SPI_STACK *stack, int arg_count) {
    CGameDataUsed *entry;
    BREEDFISH_USED *fields;

    if (spi_nowanalyze_gyorace_limmit <= spi_gyorace_counter) {
        return 0;
    }
    entry = &spi_nowanalyze_gyorace_data[spi_gyorace_counter];
    entry->CopyDataFish(spiGetStackInt(stack++));

    fields = &entry->data.fish;
    entry->SetName(spiGetStackString(stack++));
    fields->unk_16 = spiGetStackInt(stack++);
    fields->param[4] = spiGetStackInt(stack++);
    fields->param[0] = spiGetStackInt(stack++);
    fields->param[1] = spiGetStackInt(stack++);
    fields->param[2] = spiGetStackInt(stack++);
    fields->param[3] = spiGetStackInt(stack++);
    fields->size = spiGetStackInt(stack);
    spi_gyorace_counter += 1;
    return 1;
}

int CGyoraceFishData::LoadData(mgCMemory *memory, u_long128 *buffer) {
    char saved_dir[0x80];
    char path[0x40];
    int size;

    if (memory == NULL) {
        return 0;
    }
    fish[0] = NULL;
    fish[1] = NULL;
    fish[2] = NULL;
    fish[3] = NULL;
    fish_num[0] = 0;
    fish_num[1] = 0;
    fish_num[2] = 0;
    fish_num[3] = 0;
    spi_gyorace_data = this;
    spi_gyorace_stack = memory;
    GetCurrentDir(saved_dir);
    SetCurrentDir(NULL);
    sprintf(path, "sg/gyo/rfd%d.cfg", LanguageCode);
    if (LoadFile2(path, buffer, &size, 0) != 0) {
        CScriptInterpreter interpreter;
        interpreter.SetTag(gyorace_tag);
        interpreter.SetScript((char *)buffer, size);
        interpreter.Run();
    }
    SetCurrentDir(saved_dir);
    return 1;
}

CGameDataUsed *CGyoraceFishData::GetRaceFish(int race_class, int no) {
    if (race_class < 0 || race_class > 3) {
        return NULL;
    }
    if (fish_num[race_class] <= 0 || fish_num[race_class] <= no) {
        return NULL;
    }
    if (fish[race_class] == NULL) {
        return NULL;
    }
    return &fish[race_class][no];
}
/**
 * Allocates the tournament prize groups.
 */
static int _PRIZE_LISTNUM(SPI_STACK *stack, int arg_count) {
    unsigned int bytes;

    bytes = (unsigned int)(spiGetStackInt(stack) * sizeof(fish_prize_group));
    FishTournamentGoods = (fish_prize_group *)operator new[](
        bytes, (u_long128 *)fish_prize_buildstack->Alloc(align16_blocks(bytes) + 2));
    FishTournamentGoodsNum = 0;
    return 1;
}
#ifdef NONMATCHING
/**
 * Fills the next tournament group and allocates its prize entries.
 */
static int _PRIZE_GROUP(SPI_STACK *stack, int arg_count) {
    int prize_count;
    int i;
    SPI_STACK *arg;
    unsigned int bytes;

    arg = stack + 1;
    spiFishTournamentGoods = &FishTournamentGoods[FishTournamentGoodsNum++];
    prize_count = spiGetStackInt(stack);
    spiFishTournamentGoods->prize_count = prize_count;
    for (i = 0; i < 8; i++) {

        spiFishTournamentGoods->unk_4[i] = spiGetStackInt(arg++);
    }
    bytes = prize_count * sizeof(fish_prize_record);
    spiFishTournamentGoods->prizes = (fish_prize_record *)operator new[](
        bytes, (u_long128 *)fish_prize_buildstack->Alloc(align16_blocks(bytes) + 2));
    spi_fish_prize_info = spiFishTournamentGoods->prizes;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", _PRIZE_GROUP__FP9SPI_STACKi);
#endif
/**
 * Fills the three ranks of the next tournament prize entry.
 */
static int _PRIZE(SPI_STACK *stack, int arg_count) {
    SPI_STACK *arg;

    arg = stack + 1;
    if (spi_fish_prize_info == NULL) {
        return 0;
    }
    spi_fish_prize_info->unk_0 = spiGetStackInt(stack);
    spi_fish_prize_info->rank[0].unk_0 = spiGetStackInt(arg++);
    spi_fish_prize_info->rank[0].unk_4 = spiGetStackInt(arg++);
    spi_fish_prize_info->rank[1].unk_0 = spiGetStackInt(arg++);
    spi_fish_prize_info->rank[1].unk_4 = spiGetStackInt(arg++);
    spi_fish_prize_info->rank[2].unk_0 = spiGetStackInt(arg++);
    spi_fish_prize_info->rank[2].unk_4 = spiGetStackInt(arg);
    spi_fish_prize_info++;
    return 1;
}

void InitFishPrize() {
    fish_prize_buildstack = 0;
    FishTournamentGoods = 0;
    FishTournamentGoodsNum = 0;
    spi_fish_prize_info = 0;
}

int LoadFishPrize(int type) {
    u8 buffer[0x2800];
    mgCMemory memory;
    memory.stSetBuffer((u_long128 *)buffer, 0x280);
    memory.Align64();
    LoadFishPrize(type, &memory);
    return 0;
}

int LoadFishPrize(int type, mgCMemory *memory) {
    u8 buffer[0x2800];
    int size;
    void *script;

    InitFishPrize();
    script = (void *)MenuCalcBufAlignment((u_long128 *)buffer);
    FishTournamentGoodsType = type;
    if (FishTournamentGoodsType < 0 || FishTournamentGoodsType >= 2) {
        FishTournamentGoodsType = 0;
    }
    if (LoadFile2(filename_4899[FishTournamentGoodsType], script, &size, 0) != 0) {
        fish_prize_buildstack = memory;
        CScriptInterpreter interpreter;
        interpreter.SetTag(gyoprize_tag);
        interpreter.SetScript((char *)script, size);
        interpreter.Run();
    }
    RefreshFishPrize();
    return 1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", RefreshFishPrize__Fv);
int GetFishPrize(int race_class, int rank, FISH_PRIZE_INFO *info) {
    if (info == NULL) {
        return 0;
    }
    if (race_class < 0) {
        race_class = 0;
    }
    if (race_class > 3) {
        race_class = 3;
    }
    if (rank < 0) {
        rank = 0;
    }
    if (rank > 2) {
        rank = 2;
    }
    if (save_fish_prize_list != NULL) {
        info->unk_0 = fish_save_present[race_class][rank].unk_0;
        info->unk_4 = fish_save_present[race_class][rank].unk_4;
    }
    return 1;
}

void TuriTourCount() {
    CSaveData *save_data;
    int count;
    int last_tour;

    save_data = GetSaveData();
    if (save_data == NULL) {
        return;
    }
    count = save_data->GetShortFlag(short_flag_tour_count) + 1;
    last_tour = 9;
    if (save_data->GetBitFlag(bit_flag_tour_cycled) != 0) {
        last_tour = 8;
    }
    if (count > last_tour) {
        count = 1;
        save_data->SetBitFlag(bit_flag_tour_cycled, count);
    }
    save_data->SetShortFlag(short_flag_tour_count, count);
}
/**
 * Analyzes the saved racer configuration command.
 */
static void GyoraceCFGAnalyze(char *command) {
    MenuCommandAnalyze(GyoraceExeCfgBuffer, GyoraceExeCfgBufferSize, command);
}
/**
 * Returns a saved racer index, or the first empty slot for a negative argument.
 */
static int SearchOmakeGyoracer(int slot) {
    int i;

    if (slot < 0) {
        for (i = 0; i < omake_racer_slot_count; i++) {
            if (0 > GyoracerIndexNo[i]) {
                return i;
            }
        }
        return -1;
    }
    return GyoracerIndexNo[slot];
}
/**
 * Gives the slot of the race list that holds a racer; -1 when it is not
 * listed.
 *
 * @mangled CheckSameRacerFish__Fi
 * @address 0x21BFE0
 * @size 0x50
 */
static int CheckSameRacerFish(int fish_no) {
    int i;

    for (i = 0; i < omake_racer_slot_count; i++) {
        if (fish_no == GyoracerIndexNo[i]) {
            return i;
        }
    }
    return -1;
}

CGameDataUsed *GetOmakeGyoracer2(int no) {
    int index;
    GYORACE_DATA *record;

    index = SearchOmakeGyoracer(no);
    if (index < 0) {
        return NULL;
    }
    record = GyoraceData->GetData(index);
    if (record == NULL) {
        return NULL;
    }

    if (record->fish.used_type == gyorace_type_omake_racer) {
        return &record->fish;
    }
    return NULL;
}

int GetOmakeGyoracerTactics(int no) {
    if (no < 0 || no > 5) {
        return -1;
    }
    return GyoracerTacticsNo[no];
}

void SetOmakeGyoracerTactics(int no, int tactics) {
    if (no < 0 || no > 5) {
        return;
    }
    GyoracerTacticsNo[no] = tactics;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", GyoracerListUpdate__Fv);
void GyoraceSubGameInitData() {
    GyoracerIndexNo[0] = -1;
    GyoracerTacticsNo[0] = -1;
    GyoracerIndexNo[1] = -1;
    GyoracerTacticsNo[1] = -1;
    GyoracerIndexNo[2] = -1;
    GyoracerTacticsNo[2] = -1;
    GyoracerIndexNo[3] = -1;
    GyoracerTacticsNo[3] = -1;
    GyoracerIndexNo[4] = -1;
    GyoracerTacticsNo[4] = -1;
    GyoracerIndexNo[5] = -1;
    GyoracerTacticsNo[5] = -1;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", GyoraceMenuInit__FP9mgCMemoryPii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", OmakeGyoraceSelect__Fi);
/**
 * Scrolls the list so the first empty saved racer slot is visible.
 */
static void ForceSetGyoList(void) {
    int slot = -1;
    if (GyoraceData->SearchSpaceData(&slot) == 0) {
        return;
    }
    while (slot < GyoraceFishHaveListSelect.top) {
        GyoraceFishHaveListSelect.top--;
    }
    while (slot >= GyoraceFishHaveListSelect.top + 9) {
        GyoraceFishHaveListSelect.top++;
    }
    while (GyoraceFishHaveListSelect.top + 9 <= GyoraceFishHaveListSelect.cursor) {
        GyoraceFishHaveListSelect.cursor--;
    }
    while (GyoraceFishHaveListSelect.cursor < GyoraceFishHaveListSelect.top) {
        GyoraceFishHaveListSelect.cursor++;
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", GyoraceMenuKey__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", GyoraceMenuDraw__Fv);
void DrawSubGameTitle(mgCTexture *tex, int size, int x, int y, int w) {
    static short large_title[12] = {172, 0, 24, 54, 196, 0, 10, 54, 206, 0, 24, 54};
    static short small_title[12] = {172, 54, 24, 46, 196, 54, 10, 46, 206, 54, 24, 46};
    mgRect<int> shadow;
    mgRect<int> frame;
    short *table = small_title;
    if (size == 1) {
        table = large_title;
    }
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Texture(tex);
    prim->Color(0, 0, 0, 0x40);
    shadow.Set(x + 4, y + 4, w, table[3]);
    Menu3DivideTextureDraw(prim, shadow, table, 1);
    prim->Color(0x80, 0x80, 0x80, 0x80);
    frame.Set(x, y, w, table[3]);
    Menu3DivideTextureDraw(prim, frame, table, 1);
    prim->End();
}

void DrawSubGameListFix(mgCTexture *tex, int x, int y, int w, int h) {
    static short frame_parts[36] = {0, 0, 28, 28, 28, 0, 10, 28, 38, 0, 28, 28, 0, 28, 28, 10, 28, 28, 10, 10, 38, 28, 28, 10, 0, 38, 28, 28, 28, 38, 10, 28, 38, 38, 28, 28};
    mgRect<int> shadowTop;
    mgRect<int> shadowMid;
    mgRect<int> shadowBottom;
    mgRect<int> frameTop;
    mgRect<int> frameMid;
    mgRect<int> frameBottom;
    int mid_height = h - 0x38;
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Texture(tex);
    prim->Color(0, 0, 0, 0x40);
    int shadow_x = x + 4;
    int shadow_y = y + 4;
    shadowTop.Set(shadow_x, shadow_y, w, frame_parts[3]);
    Menu3DivideTextureDraw(prim, shadowTop, frame_parts, 1);
    shadowMid.Set(shadow_x, shadow_y + frame_parts[3], w, mid_height);
    Menu3DivideTextureDraw(prim, shadowMid, frame_parts + 12, 1);
    shadowBottom.Set(shadow_x, mid_height + (shadow_y + frame_parts[3]), w, frame_parts[27]);
    Menu3DivideTextureDraw(prim, shadowBottom, frame_parts + 24, 1);
    prim->Color(0x80, 0x80, 0x80, 0x80);
    frameTop.Set(x, y, w, frame_parts[3]);
    Menu3DivideTextureDraw(prim, frameTop, frame_parts, 1);
    frameMid.Set(x, y + frame_parts[3], w, mid_height);
    Menu3DivideTextureDraw(prim, frameMid, frame_parts + 12, 1);
    frameBottom.Set(x, mid_height + (y + frame_parts[3]), w, frame_parts[27]);
    Menu3DivideTextureDraw(prim, frameBottom, frame_parts + 24, 1);
    prim->End();
}

void DrawSubGameScrlList(mgCTexture *tex, int *rect, int *scroll) {
    static short bar_parts[12] = {248, 0, 8, 10, 248, 10, 8, 10, 248, 20, 8, 10};
    static short frame_parts[36] = {0, 0, 28, 28, 28, 0, 10, 28, 66, 0, 40, 28, 0, 28, 28, 10, 28, 28, 10, 10, 66, 28, 40, 10, 0, 38, 28, 28, 28, 38, 10, 28, 66, 38, 40, 28};
    mgRect<int> unusedRect;
    mgRect<int> shadowTop;
    mgRect<int> shadowMid;
    mgRect<int> shadowBottom;
    mgRect<int> frameTop;
    mgRect<int> frameMid;
    mgRect<int> frameBottom;
    mgRect<int> barRect;
    int mid_height = rect[3] - 0x38;
    int x = rect[0];
    int y = rect[1];
    int width = rect[2];
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Texture(tex);
    prim->Color(0, 0, 0, 0x40);
    int shadow_x = x + 4;
    int shadow_y = y + 4;
    shadowTop.Set(shadow_x, shadow_y, width, frame_parts[3]);
    Menu3DivideTextureDraw(prim, shadowTop, frame_parts, 1);
    shadowMid.Set(shadow_x, shadow_y + frame_parts[3], width, mid_height);
    Menu3DivideTextureDraw(prim, shadowMid, frame_parts + 12, 1);
    shadowBottom.Set(shadow_x, mid_height + (shadow_y + frame_parts[3]), width, frame_parts[27]);
    Menu3DivideTextureDraw(prim, shadowBottom, frame_parts + 24, 1);
    prim->Color(0x80, 0x80, 0x80, 0x80);
    frameTop.Set(x, y, width, frame_parts[3]);
    Menu3DivideTextureDraw(prim, frameTop, frame_parts, 1);
    frameMid.Set(x, y + frame_parts[3], width, mid_height);
    Menu3DivideTextureDraw(prim, frameMid, frame_parts + 12, 1);
    frameBottom.Set(x, mid_height + (y + frame_parts[3]), width, frame_parts[27]);
    Menu3DivideTextureDraw(prim, frameBottom, frame_parts + 24, 1);
    unusedRect.Set(0xF8, 0, 8, 0x1E);
    barRect.Set((x + width) - 0xF, rect[1] + scroll[0] + 8, 8, scroll[1]);
    Menu3DivideTextureDraw(prim, barRect, bar_parts, 0);
    prim->End();
}

void DrawSubGameUnderLine(mgCTexture *tex, int x, int y, int w) {
    static short underline_parts[12] = {0, 66, 10, 6, 10, 66, 10, 6, 20, 66, 10, 6};
    mgRect<int> rect;
    mgCDrawPrim *prim = GetMenuPrim();
    SetSpriteEnv(prim, 0);
    prim->Begin(MG_PRIM_SPRITE);
    prim->Texture(tex);
    prim->Color(0x80, 0x80, 0x80, 0x80);
    rect.Set(x, y, w, underline_parts[3]);
    Menu3DivideTextureDraw(prim, rect, underline_parts, 1);
    prim->End();
}

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/menuaqua", __sinit_menuaqua_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", aquafish_mixTable__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ambient__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", light_dir__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", light_color__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ColChkPoint__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ColChkPoint2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ColChkPoint3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", esa_info__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", aqua_bubble_generate_pos__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", up_tbl_996__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", amptbl_997__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1160__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1241__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", dirtbl_1242__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1346__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1471__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", max_tbl_1484__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", aquafish_info__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", u_brdtbl_2493__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", get_paraxtbl_2494__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ptbl_2495__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", chrtbl_2503__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2742__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2935__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2975__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2976__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3016__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3290__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3291__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3310__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3311__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", langTbl_3630__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", menu_id_tbl_3721__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4306__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4352__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4363__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4364__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4369__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4370__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4371__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4372__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", v1orig_4373__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", v2orig_4374__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", v3orig_4375__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", v4orig_4376__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", t_4408__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4432__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", gyorace_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", gyoprize_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", GyoracerIndexNo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", GyoracerTacticsNo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", Mitouroku__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5229__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5230__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", vol_5253__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", pl_s_5630__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", pl_b_5631__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", tbl_5644__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", tbl_5669__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", bart_5670__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", pl_s_5699__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1323__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1387__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_1388__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2112__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2183__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2184__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2185__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2186__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2361__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2377__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2379__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2380__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2381__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2382__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2383__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2384__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2385__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2386__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2387__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2388__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2389__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2390__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2391__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2392__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2393__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2394__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2395__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2396__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2415__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2871__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2872__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2873__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2874__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2929__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_2930__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3150__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3151__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3152__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3153__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3154__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3155__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3156__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3157__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3158__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3159__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3160__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3161__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3162__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3163__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3164__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3429__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_3430__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4300__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4299__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4420__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4421__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4422__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4423__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4424__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4425__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4426__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4427__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4428__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4429__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4430__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4431__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4519__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4629__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4814__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4815__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4825__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4884__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4885__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4900__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4901__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4975__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4976__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4977__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4978__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4979__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_4980__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5140__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5487__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5488__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5489__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5490__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5491__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5492__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5493__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5494__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5495__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5496__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5497__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5498__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5499__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5500__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", D_0037B024__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", __vt__9CFishFood__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", __vt__9CAquaFish__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ColChkPointNum__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", AQUA_TITLE_X__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", AQUA_TITLE_Y__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", AQUA_TITLE_W__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", AQUA_TITLE_H__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", xtbl_2468__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", ytbl_2469__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", wtbl_2470__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", htbl_2471__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", coltbl_2472__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", offtbl_2496__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", poffset_2511__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", m_next_aqua_no__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", aqua_frame_sizetbl_2934__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", tbl_3505__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", another_aquarium_Notbl_3642__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", menu_max_tbl_3720__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", filename_4899__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/menuaqua", at_5309__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(AquaScene, 0x4);
INCLUDE_BSS(AquaMode, 0x4);
INCLUDE_BSS(Aquarium_NameregistBlock, 0x4);
INCLUDE_BSS(Aqua_SpSndID, 0x4);
INCLUDE_BSS(Aqua_SpSndBattleCount, 0x4);
INCLUDE_BSS(AquaCameraCtrlMode, 0x4);
INCLUDE_BSS(Camera__2, 0x4);
INCLUDE_BSS(aqua_old_env, 0x4);
INCLUDE_BSS(Tex_Aqualium, 0x4);
INCLUDE_BSS(Tex_FishEffect, 0x4);
INCLUDE_BSS(menu_debug_select, 0x10);
INCLUDE_BSS(aquarium_xz_table, 0x10);
INCLUDE_BSS(aquarium_y_table, 0x4);
INCLUDE_BSS(aquarium_paul_table, 0x4);
INCLUDE_BSS(AquaBattleBubble, 0x4);
INCLUDE_BSS(AquaBattleBubble_Generate_Wait, 0x4);
INCLUDE_BSS(AquaBattleBubble_Generate_Counter, 0x4);
INCLUDE_BSS(count_1612, 0x4);
INCLUDE_BSS(init_1613, 0x4);
INCLUDE_BSS(m_aquarium_para, 0x4);
INCLUDE_BSS(m_aquarium_limmit_adr, 0x4);
INCLUDE_BSS(AquaDeadCheck, 0x4);
INCLUDE_BSS(sel_sift_fish_3638, 0x4);
INCLUDE_BSS(init_3639, 0x4);
INCLUDE_BSS(sel_sift_fish_select_3641, 0x4);
INCLUDE_BSS(Auqa_Bgm_Volf, 0x4);
INCLUDE_BSS(GyoraceFish, 0x4);
INCLUDE_BSS(GyoraceFishSelectMode, 0x4);
INCLUDE_BSS(GyoraceFishSelectNo, 0x4);
INCLUDE_BSS(GyoraceFishSelTexBk, 0x4);
INCLUDE_BSS(GyoraceFishFrameImgTexNo, 0x4);
INCLUDE_BSS(GyoraceFishSelNum, 0x4);
INCLUDE_BSS(GyoraceFishSel, 0x8);
INCLUDE_BSS(GyoRaceFishReadPhase, 0x4);
INCLUDE_BSS(GyoRaceAquariumNo, 0x4);
INCLUDE_BSS(GyoRaceClass, 0x4);
INCLUDE_BSS(GyoRaceProgressNum, 0x4);
INCLUDE_BSS(GyoRaceRankingData, 0x4);
INCLUDE_BSS(spi_gyorace_stack, 0x4);
INCLUDE_BSS(spi_gyorace_data, 0x4);
INCLUDE_BSS(spi_nowanalyze_gyorace_data, 0x4);
INCLUDE_BSS(spi_nowanalyze_gyorace_limmit, 0x4);
INCLUDE_BSS(spi_gyorace_counter, 0x4);
INCLUDE_BSS(fish_prize_buildstack, 0x4);
INCLUDE_BSS(FishTournamentGoods, 0x4);
INCLUDE_BSS(FishTournamentGoodsNum, 0x4);
INCLUDE_BSS(FishTournamentGoodsType, 0x4);
INCLUDE_BSS(spiFishTournamentGoods, 0x4);
INCLUDE_BSS(spi_fish_prize_info, 0x4);
INCLUDE_BSS(save_fish_prize_list, 0x4);
INCLUDE_BSS(SubSaveData, 0x4);
INCLUDE_BSS(GyoraceData, 0x4);
INCLUDE_BSS(GyoracerActive, 0x4);
INCLUDE_BSS(GyoraceMes, 0x4);
INCLUDE_BSS(GyoraceMesDrawFlag, 0x4);
INCLUDE_BSS(GyoraceFishInfoDrawFlag, 0x4);
INCLUDE_BSS(GyoraceFishMes, 0x4);
INCLUDE_BSS(GyoraceFishHave, 0x4);
INCLUDE_BSS(GyoraceFishHaveDrawFlag, 0x4);
INCLUDE_BSS(GyoraceFishTacMes, 0x4);
INCLUDE_BSS(GyoraceFishTacMesDrawFlag, 0x4);
INCLUDE_BSS(GyoraceFishHaveListSelect, 0x8);
INCLUDE_BSS(GyoraceCursor, 0x4);
INCLUDE_BSS(GyoraceFishTex, 0x4);
INCLUDE_BSS(GyoraceNowMode, 0x4);
INCLUDE_BSS(GyoraceNowPhase, 0x4);
INCLUDE_BSS(GyoraceQuestionMsgDrawFlag, 0x4);
INCLUDE_BSS(GyoraceHaveFishCursorDrawFlag, 0x4);
INCLUDE_BSS(GyoraceHaveFishCursor, 0x4);
INCLUDE_BSS(Gyoracemenu_long_hand_count, 0x4);
INCLUDE_BSS(Gyoracemenu_CursorXY, 0x8);
INCLUDE_BSS(GyoraceExeCfgBuffer, 0x4);
INCLUDE_BSS(GyoraceExeCfgBufferSize, 0x4);
INCLUDE_BSS(GyoraceHaveFishListMakeLine, 0x4);
INCLUDE_BSS(GyoraceHaveFishListTopY, 0x4);
INCLUDE_BSS(GyoraceHaveFishListScrlBarY, 0x4);
INCLUDE_BSS(GyoraceHaveFishListScrlInit, 0x4);
INCLUDE_BSS(MenuLoadFishIsLoad, 0x4);
INCLUDE_BSS(MenuLoadFishSelect, 0x4);
INCLUDE_BSS(MenuLoadFishTopLine, 0x4);
INCLUDE_BSS(MenuLoadFishSelectData, 0x4);
INCLUDE_BSS(MenuLoadBoardTex, 0x4);
INCLUDE_BSS(MenuLoadFishBoardX, 0x4);
INCLUDE_BSS(local_gdata_5177, 0x4);
INCLUDE_BSS(init_5178, 0x4);
INCLUDE_BSS(save_now_space_racer_no_5180, 0x4);
INCLUDE_BSS(init_5181, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(Aquarium_NameregistStack, 0x30);
INCLUDE_BSS(AquaBubble, 0x10);
INCLUDE_BSS(AquaFishBubble, 0x20);
INCLUDE_BSS(AquaBattleBubble_Pos, 0x10);
INCLUDE_BSS(AquaFishEff, 0x20);
INCLUDE_BSS(at_2473, 0x20);
INCLUDE_BSS(at_2474, 0x20);
INCLUDE_BSS(at_2475, 0x20);
INCLUDE_BSS(at_4433, 0x30);
INCLUDE_BSS(Aquarium, 0x3D0);
INCLUDE_BSS(GyoraceFishSelStack, 0x30);
INCLUDE_BSS(fish_save_present, 0x60);
INCLUDE_BSS(GyoraceStack, 0x30);
INCLUDE_BSS(GyoraceTexBlock, 0x40);
INCLUDE_BSS(GyoraceMenuOptionBuff, 0x40);
INCLUDE_BSS(MenuDCMsg, 0x28);
