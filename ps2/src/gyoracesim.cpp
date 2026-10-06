#include "common.h"
#include "gyoracesim.hpp"
#include "crandom.hpp"
#include <cstring>

/**
 * Species figures used by the race simulation.
 */
static grFISH_DATA fish_data[18] = {
    {310, 100.0f, 100.0f, {100.0f, 100.0f, 100.0f}, 14},
    {320, 98.0f, 102.0f, {98.0f, 102.0f, 100.0f}, 1},
    {321, 104.0f, 100.0f, {100.0f, 98.0f, 98.0f}, 2},
    {322, 98.0f, 100.0f, {100.0f, 102.0f, 100.0f}, 3},
    {323, 102.0f, 102.0f, {96.0f, 102.0f, 100.0f}, 4},
    {324, 102.0f, 98.0f, {98.0f, 100.0f, 102.0f}, 9},
    {325, 100.0f, 104.0f, {100.0f, 100.0f, 102.0f}, 18},
    {326, 95.0f, 98.0f, {100.0f, 98.0f, 100.0f}, 5},
    {327, 105.0f, 98.0f, {98.0f, 98.0f, 98.0f}, 16},
    {328, 90.0f, 105.0f, {105.0f, 102.0f, 102.0f}, 8},
    {329, 98.0f, 100.0f, {98.0f, 98.0f, 100.0f}, 10},
    {330, 96.0f, 100.0f, {105.0f, 95.0f, 98.0f}, 11},
    {331, 95.0f, 98.0f, {98.0f, 96.0f, 102.0f}, 12},
    {332, 105.0f, 98.0f, {102.0f, 98.0f, 100.0f}, 13},
    {333, 102.0f, 98.0f, {102.0f, 100.0f, 96.0f}, 17},
    {334, 100.0f, 102.0f, {98.0f, 96.0f, 102.0f}, 15},
    {335, 96.0f, 98.0f, {102.0f, 100.0f, 98.0f}, 6},
    {336, 105.0f, 105.0f, {98.0f, 98.0f, 104.0f}, 7},
};
/**
 * Table of race random numbers.
 */
static int ia[56];
/**
 * Current index into the race random number table.
 */
static int jrand;

static float FishDist(RACE_FISH_PARAM *fish, RACE_FISH_PARAM *other);
static int StepFish(int step, RACE_FISH_PARAM *fish);
static void LaneBattleStep(RACE_FISH_PARAM *fish, int count);
static void CollisionFish(RACE_FISH_PARAM *fish, int count);
static int StepGyoRace(RACE_FISH_PARAM *fish, grRACE_INFO *info);
static s32 GetRaceDivision(float distance);
static float GetRaceDivisionLength(int division);
static float GetCourseR(float position, float lane);
static void FishModifyParam(grFISH_PARAM *source, float *output, float average);
static void CharacterBonus(grFISH_PARAM *source, RACE_FISH_PARAM *fish, int count);
static void RndFishParam(RACE_FISH_PARAM *fish);
static void GetPaseRatio(int tactics, float *ratio);
static void SetRaceFishParam(RACE_FISH_PARAM *fish, grRACE_INFO *info);
static grFISH_DATA *GetFishData(int fish_no);
static void irn55(void);
static void init_rnd(unsigned int seed);
static int irnd(void);
static float rnd();
static float nrnd();
static float GetRandomNumber(float mean, float range);

// Code (.text)
int grGyoRaceSimulate(grRACE_INFO *info) {
    u32 sum = 0;
    u32 random = 0x3526D02F;
    for (int i = 0; i < info->fish_num; ++i) {
        grFISH_PARAM &source = info->fish[i];
        int length = strlen(source.name);
        for (int j = 0; j < length; ++j) {
            random = random * 0x5D588B65 + 1;
            signed char c = source.name[j];
            sum += c * random;
        }

        random = random * 0x5D588B65 + 1;
        sum += source.fish_no * random;
        random = random * 0x5D588B65 + 1;
        sum += source.affinity * random;
        random = random * 0x5D588B65 + 1;
        sum += source.bonus_type * random;
        random = random * 0x5D588B65 + 1;
        sum += source.power * random;
        random = random * 0x5D588B65 + 1;
        sum += source.stamina * random;
        random = random * 0x5D588B65 + 1;
        sum += source.speed[0] * random;
        random = random * 0x5D588B65 + 1;
        sum += source.speed[1] * random;
        random = random * 0x5D588B65 + 1;
        sum += source.speed[2] * random;
        random = random * 0x5D588B65 + 1;
        sum += source.tactics * random;
        random = random * 0x5D588B65 + 1;
        sum += source.lane * random;
    }
    if (info->seed == 0) {
        init_rnd(sum);
    } else {
        init_rnd(info->seed);
    }
    RACE_FISH_PARAM fish[6];
    SetRaceFishParam(fish, info);
    return StepGyoRace(fish, info);
}
#ifdef NONMATCHING
int grGetFishProgress(grRACE_INFO *info, int fish, float time, grRACE_PROGRESS *out) {
    grRACE_PROGRESS *record;
    int step;
    int next;
    float fraction;
    if (fish < 0 || fish >= info->fish_num) {
        return 0;
    }
    record = info->progress[fish];
    if (record == NULL) {
        return 0;
    }
    step = (int)time;
    next = step + 1;
    fraction = time - (float)step;
    if (next >= info->step_max) {
        return 0;
    }
    grRACE_PROGRESS *current = &record[step];
    out->pos = current->pos;
    out->lane = current->lane;
    out->lane_pos = current->lane_pos;
    out->state = current->state;
    out->battle = current->battle;
    out->battle_target = current->battle_target;
    out->battle_hits = current->battle_hits;
    if (out->state == GR_RACE_STATE_NONE) {
        return 0;
    }
    out->pos += fraction * (record[next].pos - out->pos);
    out->lane_pos += fraction * (record[next].lane_pos - out->lane_pos);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", grGetFishProgress__FP11grRACE_INFOifP15grRACE_PROGRESS);
#endif
/**
 * Gives the separation of two fish after their next step.
 */
static float FishDist(RACE_FISH_PARAM *fish, RACE_FISH_PARAM *other) {
    return (fish->pos + fish->velocity) - (other->pos + other->velocity);
}
/**
 * Advances one fish and records its position and race state.
 */
static int StepFish(int step, RACE_FISH_PARAM *fish) {
    int division;
    float pace_b;
    float target;
    float acceleration;
    float radius;
    if (fish->progress == NULL || step >= fish->progress_num) {
        return 1;
    }
    grRACE_PROGRESS &record = fish->progress[step];
    division = GetRaceDivision(fish->pos);
    if (division < 0) {
        fish->velocity -= 0.01f;
        if (fish->velocity < 0.0f) fish->velocity = 0.01f;
        fish->pos += fish->velocity;
    } else {
        pace_b = fish->accel[division];
        target = 0.1f + 0.00020000001f * fish->speed[division];
        if (fish->rank > 0 && fish->rank < 7) {
            target *= fish->rank_ratio[fish->rank - 1];
        }
        acceleration = pace_b - (fish->velocity - target) / 0.016f;
        if (fish->boost > 1.0f) {
            fish->boost = 1.0f;
        }
        if (fish->boost < -1.0f) {
            fish->boost = -1.0f;
        }
        acceleration += 1.25f * fish->boost;
        radius = GetCourseR(fish->pos, record.lane_pos);
        fish->velocity += 0.0016000001f * acceleration;
        if (fish->velocity < 0.01f) {
            fish->velocity = 0.01f;
        }
        fish->pos += fish->velocity * radius;
        if (fish->boost > 0.0f) {
            fish->boost -= 0.05f;
            if (fish->boost < 0.0f) fish->boost = 0.0f;
        } else if (fish->boost < 0.0f) {
            fish->boost += 0.05f;
            if (fish->boost > 0.0f) fish->boost = 0.0f;
        }
    }
    record.pos = fish->pos;
    record.state = fish->state;
    record.battle = fish->battle;
    record.battle_target = fish->battle_target;
    record.battle_hits = fish->battle_hits;
    record.lane = fish->lane;
    record.lane_pos = (float)fish->lane;
    if (record.pos >= 16.0f) {
        record.state = GR_RACE_STATE_GOAL;
        return 1;
    }
    return 0;
}
#ifdef NONMATCHING
/**
 * Updates lane changes and battles between neighboring fish.
 */
static void LaneBattleStep(RACE_FISH_PARAM *fish, int count) {
    int lane_count[6] = {0, 0, 0, 0, 0, 0};
    int lane_fish[6][6];
    int order[6];
    for (int i = 0; i < count; ++i) {
        order[i] = i;
        int lane = fish[i].lane;
        lane_fish[lane][lane_count[lane]++] = i;
    }
    for (int i = 0; i < 20; ++i) {
        int a = (irnd() >> 22) % count;
        int b = (irnd() >> 22) % count;
        int swap = order[a];
        order[a] = order[b];
        order[b] = swap;
    }
    for (int turn = 0; turn < count; ++turn) {
        int index = order[turn];
        RACE_FISH_PARAM &current = fish[index];
        int neighbor[2] = {-1, -1};
        bool crowded[2] = {false, false};
        float best_distance[2] = {-1.0f, -1.0f};
        for (int side = 0; side < 2; ++side) {
            int adjacent_lane = current.lane + (side == 0 ? -1 : 1);
            if (adjacent_lane < 0 || adjacent_lane >= 6) continue;
            for (int j = 0; j < lane_count[adjacent_lane]; ++j) {
                int other_index = lane_fish[adjacent_lane][j];
                RACE_FISH_PARAM &other = fish[other_index];
                float distance = FishDist(&other, &current);
                float magnitude = distance < 0.0f ? -distance : distance;
                if (magnitude < 0.075f) crowded[side] = true;
                if (magnitude < 0.05f && other.state != GR_RACE_STATE_BATTLE &&
                    (neighbor[side] < 0 || best_distance[side] < distance)) {
                    neighbor[side] = other_index;
                    best_distance[side] = distance;
                }
            }
        }
        bool fish_ahead = false;
        for (int j = 0; j < lane_count[current.lane]; ++j) {
            float distance = FishDist(&fish[lane_fish[current.lane][j]], &current);
            if (distance > 0.0f && distance < 0.1f) fish_ahead = true;
        }
        if (current.state == GR_RACE_STATE_BATTLE) {
            current.battle_time -= 1.0f;
            RACE_FISH_PARAM &other = fish[current.battle_target];
            float difference = current.power - other.power;
            if (difference > 30.0f) difference = 30.0f;
            if (difference < -30.0f) difference = -30.0f;
            int chance = (int)(((difference + 30.0f) / 60.0f) * 100.0f);
            if (chance < 1) chance = 1;
            if (chance > 100) chance = 100;
            if (rand_prob(chance)) ++current.battle_hits;
            if (current.battle_time < 0.0f) {
                RACE_FISH_PARAM *winner = rand_prob(chance) ? &current : &other;
                RACE_FISH_PARAM *loser = winner == &current ? &other : &current;
                winner->boost = 0.5f;
                loser->boost = -0.25f;
                current.battle_time = 0.0f;
                current.state = GR_RACE_STATE_SWIM;
                current.battle = 0;
                other.state = GR_RACE_STATE_SWIM;
                other.battle = 0;
                other.battle_time = 0.0f;
            }
        } else {
            float crowd_effect = 0.0f;
            float increment = 0.1f * GetRandomNumber(1.0f, 0.5f);
            if (!crowded[0] && !crowded[1]) crowd_effect = -increment;
            if (crowded[0]) crowd_effect += increment;
            if (crowded[1]) crowd_effect += increment;
            current.battle_urge += current.aggression * crowd_effect;
            if (current.battle_urge < 0.0f) current.battle_urge = 0.0f;
        }
        if (current.state == GR_RACE_STATE_BATTLE) continue;
        if (rand_prob(10) && !crowded[0]) {
            if (--current.lane < 0) current.lane = 0;
        } else if (fish_ahead && rand_prob(75)) {
            int change = 0;
            if (!crowded[0]) {
                if (!crowded[1]) change = rand_prob(80) ? 1 : -1;
                else change = -1;
            } else if (!crowded[1]) change = 1;
            current.lane += change;
            if (current.lane < 0) current.lane = 0;
            if (current.lane >= 6) current.lane = 5;
        } else if ((neighbor[0] >= 0 || neighbor[1] >= 0) && current.battle_urge > 1.0f) {
            int target = -1;
            if (crowded[0] && crowded[1]) target = rand_prob(50) ? neighbor[0] : neighbor[1];
            else if (crowded[0]) target = neighbor[0];
            else if (crowded[1]) target = neighbor[1];
            if (target >= 0) {
                RACE_FISH_PARAM &other = fish[target];
                float speed = current.velocity > other.velocity ? current.velocity : other.velocity;
                current.state = GR_RACE_STATE_BATTLE;
                current.battle = 1;
                current.battle_target = target;
                current.battle_hits = 0;
                current.battle_urge = 0.0f;
                current.battle_time = 5.0f;
                current.velocity = speed;
                other.state = GR_RACE_STATE_BATTLE;
                other.battle = 1;
                other.battle_target = index;
                other.battle_hits = 0;
                other.battle_urge = 0.0f;
                other.battle_time = 5.0f;
                other.velocity = speed;
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", LaneBattleStep__FP15RACE_FISH_PARAMi);
#endif
#ifdef NONMATCHING
/**
 * Keeps fish in the same lane separated.
 */
static void CollisionFish(RACE_FISH_PARAM *fish, int count) {
    int order[6];
    float distance[6];
    for (int i = 0; i < count; ++i) {
        order[i] = i;
        distance[i] = fish[i].pos - fish[i].velocity;
    }
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (distance[i] < distance[j]) {
                float old_distance = distance[i];
                distance[i] = distance[j];
                distance[j] = old_distance;
                int old_index = order[i];
                order[i] = order[j];
                order[j] = old_index;
            }
        }
    }
    int lane_count[6] = {0, 0, 0, 0, 0, 0};
    int lane_fish[6][6];
    for (int i = 0; i < count; ++i) {
        int index = order[i];
        int lane = fish[index].lane;
        lane_fish[lane][lane_count[lane]++] = index;
    }
    for (int lane = 0; lane < 6; ++lane) {
        if (lane_count[lane] == 0) continue;
        RACE_FISH_PARAM *ahead = &fish[lane_fish[lane][0]];
        for (int i = 1; i < lane_count[lane]; ++i) {
            RACE_FISH_PARAM *behind = &fish[lane_fish[lane][i]];
            float limit = ahead->pos - 0.05f;
            if (limit < behind->pos) behind->pos = limit;
            ahead = behind;
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", CollisionFish__FP15RACE_FISH_PARAMi);
#endif
#ifdef NONMATCHING
/**
 * Advances the race and records the final places and goal times.
 */
static int StepGyoRace(RACE_FISH_PARAM *fish, grRACE_INFO *info) {
    for (int i = 0; i < 6; ++i) {
        info->rank[i] = 0;
        info->goal_time[i] = 0.0f;
    }
    int step = 0;
    for (; step < info->step_max; ++step) {
        int finished[6];
        for (int i = 0; i < info->fish_num; ++i) {
            finished[i] = StepFish(step, &fish[i]);
            if (finished[i] && info->goal_time[i] == 0.0f) {
                info->goal_time[i] = (float)step - (fish[i].pos - 16.0f) / fish[i].velocity;
            }
        }
        for (int i = 0; i < info->fish_num; ++i) {
            fish[i].rank = 1;
            for (int j = 0; j < info->fish_num; ++j) {
                if (i != j && fish[i].pos < fish[j].pos) ++fish[i].rank;
            }
        }
        CollisionFish(fish, info->fish_num);
        LaneBattleStep(fish, info->fish_num);
        bool all_finished = true;
        for (int i = 0; i < info->fish_num; ++i) {
            if (!finished[i]) all_finished = false;
        }
        if (all_finished) break;
    }
    for (int i = 0; i < info->fish_num; ++i) {
        info->rank[i] = 1;
        for (int j = 0; j < info->fish_num; ++j) {
            if (i != j && info->goal_time[i] > info->goal_time[j]) ++info->rank[i];
        }
    }
    int next = step + 1;
    for (int extra = 0; extra <= info->after_goal_step && next < info->step_max; ++extra, ++next) {
        for (int i = 0; i < info->fish_num; ++i) StepFish(next, &fish[i]);
    }
    return next;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", StepGyoRace__FP15RACE_FISH_PARAMP11grRACE_INFO);
#endif
/**
 * Gives the course division at a distance.
 */
static s32 GetRaceDivision(float distance) {
    s32 division;

    if (distance < 2.0f) {
        return 0;
    }
    if (distance < 6.0f) {
        return 1;
    }
    if (distance < 10.0f) {
        return 2;
    }
    if (distance < 14.0f) {
        return 3;
    }
    division = -1;
    if (!(distance < 16.0f)) {
        return division;
    }
    division = 4;

    return division;
}
/**
 * Gives the length of a course division.
 */
static float GetRaceDivisionLength(int division) {
    if (division < 0) {
        return 0.0f;
    }
    if (division == 0) {
        return 2.0f;
    }
    if (division == 4) {
        return 2.0f;
    }
    return 4.0f;
}

/**
 * Gives the course radius for a position and lane.
 */
static float GetCourseR(float position, float lane) {
    // The retail branches all choose the same course radius.
    int phase = (int)position % 8;
    if (phase == 1 || phase == 2 || phase == 5 || phase == 6) {
        return 1.0f;
    }
    return 1.0f;
}
#ifdef NONMATCHING
/**
 * Applies species, name and tactics adjustments to race figures.
 */
static void FishModifyParam(grFISH_PARAM *source, float *output, float average) {
    output[0] = (float)source->stamina;
    for (int i = 0; i < 3; ++i) output[i + 1] = (float)source->speed[i];
    output[4] = (float)source->power;
    output[5] = 0.5f;
    grFISH_DATA *kind = GetFishData(source->fish_no);
    if (kind != NULL) {
        output[0] *= kind->stamina / 100.0f;
        for (int i = 0; i < 3; ++i) output[i + 1] *= kind->speed[i] / 100.0f;
        output[4] *= kind->power / 100.0f;
        if (source->affinity == kind->affinity) {
            for (int i = 0; i < 5; ++i) output[i] *= 1.1f;
        }
    }
    u32 seed = 1;
    int shift = 0;
    for (int i = 0; source->name[i] != '\0'; ++i) {
        seed += (signed char)source->name[i] << shift;
        shift = (shift + 4) % 28;
    }
    if (seed == 0) seed = 1;
    CRandom random;
    random.seed = seed;
    for (int i = 0; i < 1000; ++i) random.seed = random.seed * 0x5D588B65 + 1;
    for (int i = 0; i < 5; ++i) output[i] *= 1.0f + random.nget() * 0.03f;
    float noise = 25.0f * average / 100.0f;
    if (noise < 6.25f) noise = 6.25f;
    for (int i = 0; i < 4; ++i) {
        float variation = noise * nrnd();
        if (variation < 0.0f) variation = -variation;
        output[i] += variation;
        if (output[i] < 0.0f) output[i] = 0.0f;
    }
    output[5] = GetRandomNumber(0.5f, 0.5f);
    switch (source->tactics) {
    case 0: {
        float factor = GetRandomNumber(1.0f, 0.1f);
        output[5] -= 0.5f;
        for (int i = 1; i <= 3; ++i) output[i] *= factor;
        break;
    }
    case 1: {
        float factor = GetRandomNumber(1.0f, 0.2f);
        for (int i = 1; i <= 3; ++i) output[i] *= factor;
        break;
    }
    case 2:
        output[5] -= 0.3f;
        output[1] *= GetRandomNumber(1.5f, 0.2f);
        output[2] *= 0.873f;
        output[3] *= 0.5f;
        break;
    case 3:
        output[5] += 0.2f;
        output[1] *= 0.8f;
        output[2] *= 0.8f;
        output[3] *= GetRandomNumber(1.8f, 0.4f);
        break;
    case 4: {
        float factor = GetRandomNumber(1.0f, 0.2f);
        output[5] += 0.5f;
        for (int i = 1; i <= 3; ++i) output[i] *= factor;
        break;
    }
    case 5:
        output[5] += 0.1f;
        output[1] *= 0.8f;
        output[2] *= GetRandomNumber(1.3f, 0.3f);
        output[3] *= 0.8f;
        break;
    }
    for (int i = 0; i < 4; ++i) if (output[i] < 0.0f) output[i] = 0.0f;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", FishModifyParam__FP12grFISH_PARAMPff);
#endif
#ifdef NONMATCHING
/**
 * Sets the speed factors for each place in the race.
 */
static void CharacterBonus(grFISH_PARAM *source, RACE_FISH_PARAM *fish, int count) {
    float front = 1.0f;
    float back = 1.0f;
    for (int i = 0; i < 6; ++i) fish->rank_ratio[i] = 1.0f;
    switch (source->bonus_type) {
    case GR_CHARA_BONUS_FRONT: {
        float amount = GetRandomNumber(0.0f, 0.01f);
        if (amount < 0.0f) amount = -amount;
        front = 1.0f + amount;
        back = 1.0f - amount;
        break;
    }
    case GR_CHARA_BONUS_BACK: {
        float amount = GetRandomNumber(0.0f, 0.01f);
        if (amount < 0.0f) amount = -amount;
        front = 1.0f - 0.2f * amount;
        back = 1.0f + amount;
        break;
    }
    case GR_CHARA_BONUS_RANDOM:
        front = GetRandomNumber(1.0f, 0.01f);
        back = GetRandomNumber(1.0f, 0.01f);
        break;
    }
    for (int i = 0; i < count; ++i) {
        fish->rank_ratio[i] = front - ((float)i / (float)(count - 1)) * (front - back);
    }
    fish->rank_ratio[0] = 1.0f;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyoracesim", CharacterBonus__FP12grFISH_PARAMP15RACE_FISH_PARAMi);
#endif
/**
 * Varies the speed and acceleration of a fish.
 */
static void RndFishParam(RACE_FISH_PARAM *fish) {
    for (int i = 0; i < 5; ++i) {
        fish->speed[i] *= GetRandomNumber(1.0f, 0.5f);
        if (fish->speed[i] < 0.0f) fish->speed[i] = 0.0f;
        fish->accel[i] *= GetRandomNumber(1.0f, 0.5f);
        if (fish->accel[i] < 0.0f) fish->accel[i] = 0.0f;
    }
}
/**
 * Distributes the acceleration over the course divisions.
 */
static void GetPaseRatio(int tactics, float *ratio) {
    for (int division = 0; division < 5; ++division) {
        ratio[division] = 1.0f;
    }
    float total = 0.0f;
    for (int division = 0; division < 5; ++division) {
        total += ratio[division];
    }
    for (int division = 0; division < 5; ++division) {
        ratio[division] /= total;
    }
}
/**
 * Prepares the simulation state and progress buffer of each entrant.
 */
static void SetRaceFishParam(RACE_FISH_PARAM *fish, grRACE_INFO *info) {
    float average;
    int i;
    int k;

    average = 0.0f;
    for (i = 0; i < info->fish_num; i++) {
        average += (float)info->fish[i].stamina;
        average += (float)info->fish[i].speed[0];
        average += (float)info->fish[i].speed[1];
        average += (float)info->fish[i].speed[2];
    }
    average /= 4.0f * (float)info->fish_num;
    for (i = 0; i < info->fish_num; i++) {
        RACE_FISH_PARAM &dst = fish[i];
        memset(&dst, 0, sizeof(dst));
        grFISH_PARAM source = info->fish[i];
        float ratio[5];
        float modified[6];
        grFISH_PARAM *source_ptr = &source;
        FishModifyParam(source_ptr, modified, average);
        CharacterBonus(source_ptr, &dst, info->fish_num);
        float low = modified[1];
        float mid = modified[2];
        float speed = modified[0];
        float high = modified[3];
        float half = 0.5f * mid;
        dst.speed[0] = low;
        dst.speed[1] = (low + half) / 1.5f;
        dst.speed[2] = mid;
        dst.speed[3] = (high + half) / 1.5f;
        dst.speed[4] = high;
        GetPaseRatio(source.tactics, ratio);
        for (k = 0; k < 5; k++) {
            float scaled = speed * ratio[k];
            dst.accel[k] = scaled / (10.0f * GetRaceDivisionLength(k));
        }
        dst.power = modified[4];
        dst.aggression = modified[5];
        RndFishParam(&dst);
        dst.boost = 0.0f;
        dst.velocity = GetRandomNumber(0.02f, 0.02f);
        if (dst.velocity < 0.0f) {
            dst.velocity = 0.0f;
        }
        dst.battle_urge = 0.0f;
        dst.battle_time = 0.0f;
        dst.pos = 0.0f;
        dst.lane = source_ptr->lane;
        dst.state = GR_RACE_STATE_SWIM;
        dst.battle = 0;
        dst.progress_num = info->step_max;
        dst.progress = info->progress[i];
        memset(dst.progress, 0, info->step_max * sizeof(grRACE_PROGRESS));
    }
}
/**
 * Finds the racing figures of a fish species.
 */
static grFISH_DATA *GetFishData(int fish_no) {
    for (int fish_index = 0; fish_index < 18; ++fish_index) {
        if (fish_data[fish_index].fish_no == fish_no) {
            return &fish_data[fish_index];
        }
    }
    return NULL;
}

/**
 * Refreshes the race random number table.
 */
static void irn55(void) {
    int index;
    for (index = 1; index <= 24; index++) {
        int value = ia[index] - ia[index + 31];
        if (value < 0) {
            value += 1000000000;
        }
        ia[index] = value;
    }
    for (index = 25; index <= 55; index++) {
        int value = ia[index] - ia[index - 24];
        if (value < 0) {
            value += 1000000000;
        }
        ia[index] = value;
    }
}

/**
 * Seeds the race random number table.
 */
static void init_rnd(unsigned int seed) {
    int index;
    int next;
    for (index = 0; index < 56; index++) {
        ia[index] = 0;
    }
    ia[55] = seed;
    next = 1;
    for (index = 1; index <= 54; index++) {
        ia[21 * index % 55] = next;
        next = seed - next;
        if (next < 0) {
            next += 1000000000;
        }
        seed = ia[21 * index % 55];
    }
    irn55();
    irn55();
    irn55();
    jrand = 55;
}

/**
 * Gives the next integer from the race random number table.
 */
static int irnd(void) {
    int next = jrand + 1;
    jrand = next;
    if (next > 55) {
        irn55();
        jrand = 1;
    }
    return ia[jrand];
}
/**
 * Gives a random value between zero and one.
 */
static float rnd() {
    return (float)irnd() / 1000000000.0f;
}
/**
 * Gives a centered random value from twelve samples.
 */
static float nrnd() {
    float total = 0.0f;
    for (int sample = 0; sample < 12; ++sample) {
        total += rnd();
    }
    return total - 6.0f;
}
/**
 * Scales a centered random value to the given mean and range.
 */
static float GetRandomNumber(float mean, float range) {
    float value = nrnd();
    float scale = range / 3.0f;
    value *= scale;
    return mean + value;
}
int rand_prob(int percent) {
    return ((irnd() >> 12) % 100) < percent;
}

// Initialised data (.data)


// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyoracesim", at_1059__3__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyoracesim", at_483__2__DATA);

// Small uninitialised data (.sbss)


// Uninitialised data (.bss)
