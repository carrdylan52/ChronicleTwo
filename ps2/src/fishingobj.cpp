#include "common.h"
#include "fishingobj.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <libvu0.h>
#include "mg_math.hpp"
#include "gameutil.hpp"
#include "mg_frame.hpp"
#include "mg_drawprim.hpp"
#include "mglib.hpp"
#include "scenesnd.hpp"
#include "dng_main.hpp"

static float WaterLevel;
static int LineTop;
static float LineTopDist;
static int CastingLureFlag;
static int CastingLureTime;
static int AddLineSpeed;
static int BattleFlag;
static float BattleLineDist;
static int ShowHari;
static int LureLessFlag;
static int NowMode;
static int NowFishSpeed;
static float NowFishRot;
static int ActionChanceNextCnt;
static int ActionChanceCnt;
static int ActionChanceDir;
static FISH_POINT RodPoint[5];
static FISH_ROD_SEGMENT RodPointDist[5];
static mgCFrame *SaoFrame[8];
static float SaoDist[8];
static FISH_POINT LinePoint[64];
static FISH_POINT LurePoint[3];
static FISH_POINT FlyingPoint;
static FISH_POINT FishPoint;
static sceVu0FVECTOR CastingPoint;
static sceVu0FVECTOR ReleasePoint;
static sceVu0FVECTOR BattleStartPos;
static CFishObj LureObj;
static CFishObj UkiObj;
static CFishObj HariObj;
static sceVu0FVECTOR ChanceBarPos;

static void GetTriPose(sceVu0FMATRIX matrix, sceVu0FVECTOR *points, int *axes);
static CFishObj *GetActiveHariObj();
static CFishObj *GetActiveUkiObj();
static int GetNextChanceCnt();
static void BindPosition(float *point0, float *point1, float length, float rate);
static void ParaBlend(float *out, float time, sceVu0FVECTOR *samples, int count);

#ifdef NONMATCHING
static void SetObjectPoint(FISH_POINT &point, float x, float y, float z) {
    point.pos[0] = x;
    point.pos[1] = y;
    point.pos[2] = z;
    point.pos[3] = 1.0f;
}

static void SetObjectBind(FISH_BIND &bind, FISH_POINT &first, FISH_POINT &second) {
    bind.point0 = &first;
    bind.point1 = &second;
    bind.rate = 0.5f;
    bind.length = mgDistVector(first.pos, second.pos);
}
#endif

// Code (.text)
void SetFishingMode(int mode) { NowMode = mode; }

int GetFishingMode() { return NowMode; }

void SetWaterLevel(float level) { WaterLevel = level; }

float GetWaterLevel() { return WaterLevel; }

static CFishObj *GetActiveHariObj() {
    return NowMode == FISHING_MODE_LURE ? &LureObj : &HariObj;
}

static CFishObj *GetActiveUkiObj() {
    return NowMode == FISHING_MODE_LURE ? 0 : &UkiObj;
}

int ExtendLine(float length) {
    if (BattleFlag != 0) {
        BattleLineDist += length;
        if (BattleLineDist < 20.0f) {
            BattleFlag = 0;
            LineTop = 59;
            LineTopDist = 5.0f;
            return -1;
        }
        return 0;
    }
    if (length > 0.0f) {
        LineTopDist += length;
        while (LineTopDist > 5.0f) {
            LineTopDist -= 5.0f;
            --LineTop;
            if (LineTop >= 0) {
                FISH_POINT *point = &LinePoint[LineTop];
                *(u_long128 *)point->pos = *(u_long128 *)RodPoint[4].pos;
                *(u_long128 *)point->old_pos = *(u_long128 *)RodPoint[4].pos;
                mgZeroVector(point->velo);
            }
        }
        if (LineTop < 0) {
            LineTopDist = 5.0f;
            LineTop = 0;
            return 1;
        }
    }
    if (length < 0.0f) {
        LineTopDist += length;
        while (LineTopDist < 0.0f) {
            LineTopDist += 5.0f;
            ++LineTop;
        }
        if (LineTop >= 59) {
            LineTop = 59;
            LineTopDist = 5.0f;
            return -1;
        }
    }
    return 0;
}

float GetNowLineLength() {
    if (BattleFlag != 0) {
        return BattleLineDist;
    }
    float length = 5.0f * (float)(63 - LineTop);
    length += LineTopDist;
    return length;
}

float GetMinLineLength(void) {
    return 25.0f;
}

void InitRodPoint(mgCFrame *reference, mgCFrame *rod) {
    int i;
    for (i = 0; i < 5; i++) {
        mgZeroVector(RodPoint[i].pos);
        mgZeroVector(RodPoint[i].old_pos);
        mgZeroVector(RodPoint[i].velo);
    }
    for (int i = 0; i < 64; i++) {
        mgZeroVector(LinePoint[i].pos);
        mgZeroVector(LinePoint[i].old_pos);
        mgZeroVector(LinePoint[i].velo);
    }
    for (i = 0; i < 3; i++) {
        mgZeroVector(LurePoint[i].pos);
        mgZeroVector(LurePoint[i].old_pos);
        mgZeroVector(LurePoint[i].velo);
    }
    SaoFrame[0] = rod->SearchFrame("sao");
    SaoFrame[1] = rod->SearchFrame("sao2");
    SaoFrame[2] = rod->SearchFrame("sao3");
    SaoFrame[3] = rod->SearchFrame("sao4");
    SaoFrame[4] = rod->SearchFrame("sao5");
    SaoFrame[5] = rod->SearchFrame("sao6");
    SaoFrame[6] = rod->SearchFrame("sao7");
    SaoFrame[7] = rod->SearchFrame("ito");
    sceVu0FVECTOR span;
    sceVu0FVECTOR root;
    sceVu0FVECTOR tip;
    sceVu0FVECTOR joint;
    sceVu0FVECTOR offset;
    sceVu0FVECTOR position;
    SaoFrame[0]->GetWorldPosition0(root);
    SaoFrame[7]->GetWorldPosition0(tip);
    for (i = 0; i < 8; i++) {
        SaoFrame[i]->GetWorldPosition0(joint);
        SaoDist[i] = mgDistVector(root, joint);
    }
    sceVu0SubVector(span, tip, root);
    for (i = 0; i < 5; i++) {
        sceVu0ScaleVector(offset, span, (float)i / 4.0f);
        sceVu0AddVector(position, root, offset);
        *(u_long128 *)RodPoint[i].pos = *(u_long128 *)position;
        *(u_long128 *)RodPoint[i].old_pos = *(u_long128 *)position;
        mgZeroVector(RodPoint[i].velo);
        if (i > 0) {
            RodPointDist[i - 1].length = mgDistVector(RodPoint[i].pos, RodPoint[i - 1].pos);
        }
        RodPointDist[i].stiffness = ((float)(5 - i) * 0.3f) / 5.0f + 0.3f;
        if (RodPointDist[i].stiffness > 1.0f) {
            RodPointDist[i].stiffness = 1.0f;
        }
        RodPointDist[i].damping = ((float)(5 - i) * 0.2f) / 5.0f + 0.2f;
    }
    ResetLine(tip);
    LineTop = 59;
    LineTopDist = 5.0f;
    EndCastingLure();
    BattleFlag = 0;
    NowMode = FISHING_MODE_BAIT;
    ShowHari = 1;
}
#ifdef NONMATCHING
static void GetTriPose(sceVu0FMATRIX pose, sceVu0FVECTOR points[3], int axes[3]) {
    int first_axis = axes[0] < 0 ? -axes[0] : axes[0];
    int second_axis = axes[1] < 0 ? -axes[1] : axes[1];
    int normal_axis = axes[2] < 0 ? -axes[2] : axes[2];
    sceVu0SubVector(pose[first_axis], points[1], points[0]);
    sceVu0Normalize(pose[first_axis], pose[first_axis]);
    mgPlaneNormal(pose[normal_axis], points[0], points[1], points[2]);
    sceVu0Normalize(pose[normal_axis], pose[normal_axis]);
    sceVu0OuterProduct(pose[second_axis], pose[normal_axis], pose[first_axis]);
    for (int axis = 0; axis < 3; axis++) {
        sceVu0Normalize(pose[axis], pose[axis]);
        if (axes[axis] < 0) {
            sceVu0ScaleVector(pose[-axes[axis]], pose[-axes[axis]], -1.0f);
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishingobj", GetTriPose__FPA4_fPA4_fPi);
#endif

void GetHariPos(float *pos, float *velo) {
    *(u_long128 *)pos = *(u_long128 *)LinePoint[63].pos;
    *(u_long128 *)velo = *(u_long128 *)LinePoint[63].old_pos;
}

void GetUkiPos(float *pos, float *velo) {
    *(u_long128 *)pos = *(u_long128 *)LinePoint[60].pos;
    *(u_long128 *)velo = *(u_long128 *)LinePoint[60].old_pos;
}

void PullUki(float power) { LinePoint[63].velo[1] -= power; }

void SetShowHari(int show) { ShowHari = show; }

int GetShowHari() {
    sceVu0FVECTOR pos;
    sceVu0FVECTOR velo;
    GetHariPos(pos, velo);
    if (pos[1] < GetWaterLevel() - 3.0f) {
        return 0;
    }
    return ShowHari;
}

int SetLurePose(mgCFrame *lure) {
    if (lure == 0) {
        return 0;
    }
    if (NowMode == FISHING_MODE_BAIT) {
        return 0;
    }
    sceVu0FVECTOR position;
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR points[3];
    *(u_long128 *)position = *(u_long128 *)LinePoint[63].pos;
    mgUnitMatrix(matrix);
    *(u_long128 *)points[0] = *(u_long128 *)LureObj.point[0].pos;
    *(u_long128 *)points[1] = *(u_long128 *)LureObj.point[1].pos;
    *(u_long128 *)points[2] = *(u_long128 *)LureObj.point[2].pos;
    int axes[3] = {0, 1, 2};
    GetTriPose(matrix, points, axes);
    lure->SetPosition(position);
    lure->SetTransMatrix(matrix);
    return 1;
}

int SetUkiPose(mgCFrame *uki, mgCFrame *hari) {
    if (uki == 0 || hari == 0) {
        return 0;
    }
    if (NowMode == FISHING_MODE_LURE) {
        return 0;
    }
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR position;
    sceVu0FVECTOR points[3];
    *(u_long128 *)position = *(u_long128 *)LinePoint[60].pos;
    mgUnitMatrix(matrix);
    sceVu0AddVector(points[1], UkiObj.point[1].pos, UkiObj.point[2].pos);
    mgAddVector(points[1], UkiObj.point[3].pos);
    *(u_long128 *)points[0] = *(u_long128 *)UkiObj.point[0].pos;
    sceVu0ScaleVector(points[1], points[1], 1.0f / 3.0f);
    *(u_long128 *)points[2] = *(u_long128 *)UkiObj.point[1].pos;
    int uki_axes[3] = {-1, 2, 0};
    GetTriPose(matrix, points, uki_axes);
    uki->SetTransMatrix(matrix);
    uki->SetPosition(position);
    *(u_long128 *)position = *(u_long128 *)LinePoint[63].pos;
    sceVu0AddVector(points[1], HariObj.point[1].pos, HariObj.point[2].pos);
    *(u_long128 *)points[0] = *(u_long128 *)HariObj.point[0].pos;
    sceVu0ScaleVector(points[1], points[1], 0.5f);
    *(u_long128 *)points[2] = *(u_long128 *)HariObj.point[2].pos;
    int hari_axes[3] = {-1, 0, 2};
    GetTriPose(matrix, points, hari_axes);
    hari->SetTransMatrix(matrix);
    hari->SetPosition(position);
    return 1;
}

int CastingLure(float *target) {
    sceVu0FVECTOR start;
    sceVu0FVECTOR direction;
    *(u_long128 *)start = *(u_long128 *)LinePoint[63].pos;
    *(u_long128 *)CastingPoint = *(u_long128 *)target;
    *(u_long128 *)ReleasePoint = *(u_long128 *)start;
    *(u_long128 *)FlyingPoint.pos = *(u_long128 *)start;
    *(u_long128 *)FlyingPoint.old_pos = *(u_long128 *)start;
    mgZeroVector(FlyingPoint.velo);
    sceVu0SubVector(direction, target, start);
    float distance;
    float horizontal_speed;
    float rise_speed;
    float unit = 1.0f;
    float lift_ratio = 0.8f;
    distance = 2.0f * mgDistVectorXZ(direction);
    rise_speed = sqrtf((distance * 0.6f) / 1.6f);
    horizontal_speed = rise_speed * unit;
    CastingLureTime = (int)(distance / horizontal_speed);
    direction[1] = 0.0f;
    sceVu0Normalize(direction, direction);
    sceVu0ScaleVector(FlyingPoint.velo, direction, horizontal_speed);
    FlyingPoint.velo[1] = rise_speed * lift_ratio;
    AddLineSpeed = 0;
    CastingLureFlag = 1;
    return CastingLureTime;
}

void EndCastingLure() {
    CastingLureFlag = 0;
    CastingLureTime = 0;
    AddLineSpeed = 0;
}

int CatchLine(float *target, float max_dist) {
    CFishObj *hari = GetActiveHariObj();
    sceVu0FVECTOR delta;
    sceVu0FVECTOR next_pos;
    sceVu0SubVector(delta, target, LinePoint[63].pos);
    if (mgDistVector(delta) < max_dist) {
        *(u_long128 *)LinePoint[63].pos = *(u_long128 *)target;
        *(u_long128 *)LinePoint[63].old_pos = *(u_long128 *)target;
        mgZeroVector(LinePoint[63].velo);
        if (hari != NULL) {
            *(u_long128 *)hari->point[0].pos = *(u_long128 *)target;
            *(u_long128 *)hari->point[0].old_pos = *(u_long128 *)target;
            mgZeroVector(hari->point[0].velo);
        }
        return 1;
    }
    sceVu0Normalize(delta, delta);
    sceVu0ScaleVector(delta, delta, max_dist);
    sceVu0AddVector(next_pos, LinePoint[63].pos, delta);
    *(u_long128 *)LinePoint[63].pos = *(u_long128 *)next_pos;
    *(u_long128 *)LinePoint[63].old_pos = *(u_long128 *)next_pos;
    mgZeroVector(LinePoint[63].velo);
    if (hari != 0) {
        *(u_long128 *)hari->point[0].pos = *(u_long128 *)next_pos;
        *(u_long128 *)hari->point[0].old_pos = *(u_long128 *)next_pos;
        mgZeroVector(hari->point[0].velo);
    }
    return 0;
}

void SlowLineVelo(float rate) {
    for (int i = LineTop; i < 64; i++) {
        sceVu0ScaleVector(LinePoint[i].velo, LinePoint[i].velo, rate);
    }
}

void ResetLineVelo() {
    sceVu0FVECTOR top_pos;
    *(u_long128 *)top_pos = *(u_long128 *)LinePoint[LineTop].pos;
    for (int i = LineTop; i < 64; i++) {
        sceVu0FVECTOR pos;
        *(u_long128 *)pos = *(u_long128 *)LinePoint[i].pos;
        pos[0] = top_pos[0];
        pos[2] = top_pos[2];
        *(u_long128 *)LinePoint[i].pos = *(u_long128 *)pos;
        *(u_long128 *)LinePoint[i].old_pos = *(u_long128 *)pos;
        mgZeroVector(LinePoint[i].velo);
    }
}

void ResetLine(float *pos) {
    LineTop = 59;
    FISH_POINT *last = &LinePoint[59];
    *(u_long128 *)last->pos = *(u_long128 *)pos;
    *(u_long128 *)last->old_pos = *(u_long128 *)pos;
    mgZeroVector(last->velo);
    for (int i = LineTop + 1; i < 64; i++) {
        sceVu0FVECTOR next_pos;
        *(u_long128 *)next_pos = *(u_long128 *)LinePoint[i - 1].pos;
        next_pos[0] = pos[0];
        next_pos[1] -= 5.0f;
        next_pos[2] = pos[2];
        *(u_long128 *)LinePoint[i].pos = *(u_long128 *)next_pos;
        *(u_long128 *)LinePoint[i].old_pos = *(u_long128 *)next_pos;
        mgZeroVector(LinePoint[i].velo);
    }
    *(u_long128 *)LureObj.point[0].pos = *(u_long128 *)LinePoint[63].pos;
    *(u_long128 *)LureObj.point[0].old_pos = *(u_long128 *)LinePoint[63].pos;
    mgZeroVector(LureObj.point[0].velo);
    *(u_long128 *)UkiObj.point[0].pos = *(u_long128 *)LinePoint[60].pos;
    *(u_long128 *)UkiObj.point[0].old_pos = *(u_long128 *)LinePoint[60].pos;
    mgZeroVector(UkiObj.point[0].velo);
    *(u_long128 *)HariObj.point[0].pos = *(u_long128 *)LinePoint[63].pos;
    *(u_long128 *)HariObj.point[0].old_pos = *(u_long128 *)LinePoint[63].pos;
    mgZeroVector(HariObj.point[0].velo);
}

s32 GetNextChanceCnt(void) {
    return (rand() % 80) + 0x3C;
}

int InitFishBattle() {
    u_long128 *last = (u_long128 *)LinePoint[63].pos;
    *(u_long128 *)BattleStartPos = *last;
    *(u_long128 *)FishPoint.pos = *last;
    *(u_long128 *)FishPoint.old_pos = *last;
    mgZeroVector(FishPoint.velo);
    BattleLineDist = mgDistVector(LinePoint[63].pos, RodPoint[4].pos);
    NowFishSpeed = 0;
    BattleFlag = 1;
    NowFishRot = 0;
    ActionChanceNextCnt = GetNextChanceCnt();
    ActionChanceCnt = 0;
    return 1;
}

int EndFishBattle() {
    BattleFlag = 0;
    ActionChanceNextCnt = 0;
    ActionChanceCnt = 0;
    return 1;
}

int CheckRodActionChance(int dir, int *just) {
    *just = 0;
    if (BattleFlag == 0 || ActionChanceCnt <= 0) {
        return 0;
    }
    int match = dir * ActionChanceDir;
    if (match > 0) {
        return 1;
    }
    if (match < 0) {
        return -1;
    }
    *just = ActionChanceCnt == 28;
    return 0;
}

int FishBattle(CScene *scene, CCPoly *poly_buffer, int poly_max) {
    if (BattleFlag == 0) {
        return 0;
    }
    CCharacter2 *player = scene->GetCharacter(scene->player_chara);
    sceVu0FVECTOR player_rot;
    sceVu0FVECTOR player_pos;
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR float_velo;
    sceVu0FVECTOR heading;
    sceVu0FVECTOR next_pos;
    player->GetPosition(player_pos);
    player->GetRotation(player_rot);
    mgUnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, player_rot[1]);
    float target_angle = player_rot[1] + (mgRnd() - 0.5f) * 2.5132742f;
    if (ActionChanceCnt <= 0) {
        --ActionChanceNextCnt;
        ActionChanceCnt = 0;
        if (ActionChanceNextCnt <= 0) {
            ActionChanceCnt = 30;
            if (rand() % 2 != 0) {
                ActionChanceDir = 1;
            } else {
                ActionChanceDir = -1;
            }
        }
    }
    if (ActionChanceCnt > 0) {
        target_angle = ActionChanceDir > 0 ? player_rot[1] - mgRnd() * 1.2566371f :
                                              player_rot[1] + mgRnd() * 1.2566371f;
        GetUkiPos(ChanceBarPos, float_velo);
        ChanceBarPos[1] = WaterLevel;
        --ActionChanceCnt;
        if (ActionChanceCnt <= 0) {
            ActionChanceCnt = 0;
            ActionChanceNextCnt = GetNextChanceCnt();
        }
    }
    NowFishRot = mgAngleInterpolate(NowFishRot, mgAngleLimit(target_angle), 0.1f, 0);
    NowFishSpeed = 8;
    FishPoint.velo[0] = (float)NowFishSpeed * sinf(NowFishRot);
    FishPoint.velo[2] = (float)NowFishSpeed * cosf(NowFishRot);
    FishPoint.velo[1] = 0.0f;
    FishPoint.pos[1] = WaterLevel - 10.0f;
    *(u_long128 *)heading = *(u_long128 *)matrix[2];
    next_pos[0] = FishPoint.pos[0] + FishPoint.velo[0];
    next_pos[1] = FishPoint.pos[1];
    next_pos[2] = FishPoint.pos[2] + FishPoint.velo[2];
    mgVu0FBOX query_box;
    *(u_long128 *)query_box.max = *(u_long128 *)FishPoint.pos;
    *(u_long128 *)query_box.min = *(u_long128 *)FishPoint.pos;
    for (int axis = 0; axis < 3; axis++) {
        query_box.max[axis] += 100.0f;
        query_box.min[axis] -= 100.0f;
    }
    int poly_count = scene->GetColPoly(poly_buffer, query_box, poly_max);
    MoveCheckInfo check;
    memset(&check, 0, sizeof(check));
    check.radius = 10.0f;
    MoveCheck(FishPoint.pos, FishPoint.velo, next_pos, &check, poly_buffer, poly_count, 0);
    sceVu0SubVector(heading, next_pos, player_pos);
    sceVu0Normalize(heading, heading);
    sceVu0InnerProduct(heading, matrix[2]);
    FishPoint.pos[0] = next_pos[0];
    FishPoint.pos[2] = next_pos[2];
    return 0;
}

void GetFishPosVelo(float *pos, float *velo) {
    *(u_long128 *)pos = *(u_long128 *)FishPoint.pos;
    *(u_long128 *)velo = *(u_long128 *)FishPoint.velo;
}

static void BindFishObj() {
    CFishObj *hari = GetActiveHariObj();
    CFishObj *uki = GetActiveUkiObj();
    FISH_POINT *top = &LinePoint[LineTop];
    *(u_long128 *)top->pos = *(u_long128 *)RodPoint[4].pos;
    *(u_long128 *)top->old_pos = *(u_long128 *)RodPoint[4].pos;
    mgZeroVector(top->velo);
    for (int step = 0; step < 4; step++) {
        for (int i = LineTop; i < 63; i++) {
            float rate = 0.5f;
            if (i >= 59) {
                rate = 0.52f;
            }
            float length = 5.0f;
            if (i == LineTop) {
                length = LineTopDist;
            }
            BindPosition(LinePoint[i].pos, LinePoint[i + 1].pos, length, rate);
        }
        if (CastingLureFlag != 0) {
            *(u_long128 *)LinePoint[63].pos = *(u_long128 *)FlyingPoint.pos;
        }
        if (LureLessFlag == 0) {
            BindPosition(LinePoint[63].pos, hari->point[0].pos, 0.0f, 0.45f);
        }
        hari->BindStep();
        if (uki != 0) {
            BindPosition(LinePoint[60].pos, uki->point[0].pos, 0.0f, 0.4f);
            uki->BindStep();
        }
        top = &LinePoint[LineTop];
        *(u_long128 *)top->pos = *(u_long128 *)RodPoint[4].pos;
        *(u_long128 *)top->old_pos = *(u_long128 *)RodPoint[4].pos;
        mgZeroVector(top->velo);
    }
}
#ifdef NONMATCHING
void RodStep(CScene *scene, u_long128 *poly_buffer) {
    CCPoly *polys = (CCPoly *)poly_buffer;
    CFishObj *hari = GetActiveHariObj();
    CFishObj *uki = GetActiveUkiObj();
    sceVu0FVECTOR frame_pos;
    SaoFrame[0]->GetWorldPosition0(frame_pos);
    sceVu0CopyVector(RodPoint[0].pos, frame_pos);
    sceVu0CopyVector(RodPoint[0].old_pos, frame_pos);
    mgZeroVector(RodPoint[0].velo);
    SaoFrame[1]->GetWorldPosition0(frame_pos);
    sceVu0CopyVector(RodPoint[1].pos, frame_pos);
    sceVu0CopyVector(RodPoint[1].old_pos, frame_pos);
    mgZeroVector(RodPoint[1].velo);

    if (CastingLureFlag != 0) {
        FlyingPoint.velo[1] -= 0.6f;
        float cast_distance = mgDistVectorXZ(ReleasePoint, CastingPoint);
        float flown_distance = mgDistVectorXZ(ReleasePoint, FlyingPoint.pos);
        sceVu0FVECTOR flight_step;
        sceVu0CopyVector(flight_step, FlyingPoint.velo);
        if (flown_distance > cast_distance * 0.8f) {
            float scale = (cast_distance - flown_distance) / (cast_distance * 0.2f);
            flight_step[0] *= scale;
            flight_step[2] *= scale;
            ExtendLine(mgDistVectorXZ(flight_step));
        } else {
            float top_gap = mgDistVector(LinePoint[LineTop].pos, LinePoint[LineTop + 1].pos);
            if (top_gap > LineTopDist) {
                ExtendLine(0.8f * mgDistVectorXZ(flight_step));
            }
            for (int i = LineTop + 1; i < 63; i++) {
                sceVu0FVECTOR pull;
                sceVu0SubVector(pull, FlyingPoint.pos, LinePoint[i].pos);
                sceVu0Normalize(pull, pull);
                sceVu0ScaleVector(pull, pull, 2.0f);
                mgAddVector(LinePoint[i].velo, pull);
            }
        }
        float remaining = mgDistVectorXZ(CastingPoint, FlyingPoint.pos);
        if (remaining < mgDistVectorXZ(flight_step)) {
            flight_step[0] = 0.0f;
            flight_step[2] = 0.0f;
            FlyingPoint.velo[0] = 0.0f;
            FlyingPoint.velo[2] = 0.0f;
            FlyingPoint.pos[0] = CastingPoint[0];
            FlyingPoint.pos[2] = CastingPoint[2];
        }
        mgAddVector(FlyingPoint.pos, flight_step);
        sceVu0CopyVector(LinePoint[63].pos, FlyingPoint.pos);
        sceVu0CopyVector(LinePoint[63].old_pos, FlyingPoint.pos);
        mgZeroVector(LinePoint[63].velo);
        --CastingLureTime;
    }

    for (int i = 2; i < 5; i++) {
        sceVu0CopyVector(RodPoint[i].old_pos, RodPoint[i].pos);
        if (BattleFlag == 0) {
            mgAddVector(RodPoint[i].pos, RodPoint[i].velo);
        }
    }
    mgVu0FBOX line_box;
    sceVu0CopyVector(line_box.max, LinePoint[LineTop].pos);
    sceVu0CopyVector(line_box.min, LinePoint[LineTop].pos);
    for (int i = LineTop; i < 64; i++) {
        FISH_POINT &point = LinePoint[i];
        sceVu0CopyVector(point.old_pos, point.pos);
        mgAddVector(point.pos, point.velo);
        point.pos[1] -= 0.36f;
        mgVectorMaxMin(line_box.max, line_box.min, line_box.max, line_box.min, point.pos);
    }
    hari->MovePoint();
    if (uki != 0) {
        uki->MovePoint();
    }

    // Keep the rod's four moving masses spaced between its fixed joints and tip.
    for (int pass = 0; pass < 2; pass++) {
        if (BattleFlag != 0) {
            BindPosition(RodPoint[4].pos, FishPoint.pos, BattleLineDist, 0.2f);
        } else {
            BindPosition(RodPoint[4].pos, LinePoint[LineTop].pos, 0.0f, 0.8f);
        }
        for (int i = 3; i >= 2; i--) {
            sceVu0FVECTOR across;
            sceVu0FVECTOR half_across;
            sceVu0FVECTOR segment;
            sceVu0FVECTOR bend;
            sceVu0SubVector(across, RodPoint[i + 1].pos, RodPoint[i - 1].pos);
            sceVu0ScaleVector(half_across, across, 0.5f);
            sceVu0SubVector(segment, RodPoint[i].pos, RodPoint[i - 1].pos);
            sceVu0SubVector(bend, half_across, segment);
            sceVu0ScaleVector(bend, bend, RodPointDist[i].damping);
            mgAddVector(segment, bend);
            sceVu0Normalize(segment, segment);
            sceVu0ScaleVector(segment, segment, RodPointDist[i].length);
            sceVu0AddVector(RodPoint[i].pos, RodPoint[i - 1].pos, segment);
        }
        for (int i = 1; i < 4; i++) {
            sceVu0FVECTOR direction;
            sceVu0FVECTOR desired;
            sceVu0FVECTOR actual;
            sceVu0FVECTOR error;
            sceVu0SubVector(direction, RodPoint[i].pos, RodPoint[i - 1].pos);
            sceVu0Normalize(direction, direction);
            sceVu0ScaleVector(desired, direction, RodPointDist[i].length);
            sceVu0SubVector(actual, RodPoint[i + 1].pos, RodPoint[i].pos);
            sceVu0SubVector(error, desired, actual);
            sceVu0ScaleVector(error, error, RodPointDist[i].stiffness);
            mgAddVector(actual, error);
            sceVu0Normalize(actual, actual);
            sceVu0ScaleVector(actual, actual, RodPointDist[i].length);
            sceVu0AddVector(RodPoint[i + 1].pos, RodPoint[i].pos, actual);
        }
    }
    if (BattleFlag != 0) {
        for (int pass = 0; pass < 4; pass++) {
            sceVu0CopyVector(hari->point[0].pos, FishPoint.pos);
            sceVu0CopyVector(hari->point[0].old_pos, FishPoint.pos);
            mgZeroVector(hari->point[0].velo);
            sceVu0CopyVector(LinePoint[63].pos, FishPoint.pos);
            sceVu0CopyVector(LinePoint[63].old_pos, FishPoint.pos);
            mgZeroVector(LinePoint[63].velo);
            hari->BindStep();
            if (uki != 0) {
                sceVu0FVECTOR float_pos;
                sceVu0SubVector(float_pos, FishPoint.pos, RodPoint[4].pos);
                sceVu0Normalize(float_pos, float_pos);
                sceVu0ScaleVector(float_pos, float_pos, 15.0f);
                sceVu0SubVector(float_pos, FishPoint.pos, float_pos);
                sceVu0CopyVector(LinePoint[60].pos, float_pos);
                sceVu0CopyVector(LinePoint[60].old_pos, float_pos);
                mgZeroVector(LinePoint[60].velo);
                sceVu0CopyVector(uki->point[0].pos, float_pos);
                sceVu0CopyVector(uki->point[0].old_pos, float_pos);
                mgZeroVector(uki->point[0].velo);
                uki->BindStep();
            }
        }
    } else {
        sceVu0CopyVector(LinePoint[LineTop].pos, RodPoint[4].pos);
        sceVu0CopyVector(LinePoint[LineTop].old_pos, RodPoint[4].pos);
        mgZeroVector(LinePoint[LineTop].velo);
        BindFishObj();
    }
    for (int i = 1; i < 5; i++) {
        sceVu0SubVector(RodPoint[i].velo, RodPoint[i].pos, RodPoint[i].old_pos);
        sceVu0ScaleVector(RodPoint[i].velo, RodPoint[i].velo, 0.6f);
        RodPoint[i].velo[1] -= 0.6f;
        RodPoint[i].pos[3] = 1.0f;
    }

    // Move the model's seven flexible rod joints along the solved rod curve.
    sceVu0FVECTOR curve[5];
    for (int i = 0; i < 5; i++) {
        sceVu0CopyVector(curve[i], RodPoint[i].pos);
    }
    for (int i = 1; i < 8; i++) {
        mgCFrame *joint = SaoFrame[i];
        mgCFrame *parent = joint->parent;
        sceVu0FMATRIX joint_matrix;
        sceVu0FMATRIX parent_world;
        sceVu0FMATRIX parent_inverse;
        sceVu0FMATRIX parent_basis;
        sceVu0FVECTOR before;
        sceVu0FVECTOR after;
        sceVu0FVECTOR forward;
        sceVu0CopyMatrix(joint_matrix, joint->trans_matrix);
        parent->GetLWMatrix(parent_world);
        mgInversMatrix(parent_inverse, parent_world);
        sceVu0CopyMatrix(parent_basis, parent->trans_matrix);
        float fraction = SaoDist[i] / SaoDist[7];
        ParaBlend(before, 0.99f * SaoDist[i - 1] / SaoDist[7], curve, 5);
        before[3] = 1.0f;
        ParaBlend(after, 0.99f * fraction, curve, 5);
        sceVu0SubVector(forward, after, before);
        forward[3] = 0.0f;
        sceVu0ApplyMatrix(joint_matrix[0], parent_inverse, forward);
        if (i != 1) {
            sceVu0ApplyMatrix(joint_matrix[3], parent_inverse, before);
        }
        sceVu0OuterProduct(joint_matrix[2], joint_matrix[0], parent_basis[0]);
        sceVu0OuterProduct(joint_matrix[1], joint_matrix[2], joint_matrix[0]);
        sceVu0Normalize(joint_matrix[0], joint_matrix[0]);
        sceVu0Normalize(joint_matrix[1], joint_matrix[1]);
        sceVu0Normalize(joint_matrix[2], joint_matrix[2]);
        joint->SetTransMatrix(joint_matrix);
    }

    for (int axis = 0; axis < 3; axis++) {
        line_box.max[axis] += 20.0f;
        line_box.min[axis] -= 20.0f;
    }
    line_box.max[3] = 1.0f;
    line_box.min[3] = 1.0f;
    int poly_count = scene->GetColPoly(polys, line_box, 1024);
    for (int i = 0; i < poly_count; i++) {
        if (polys[i].area_kind == 7) {
            polys[i].ignore_mask |= 8;
        }
    }
    for (int i = LineTop; i < 64; i++) {
        int previous = i - 1 < LineTop ? LineTop : i - 1;
        int following = i + 1 > 63 ? 63 : i + 1;
        float heights[3] = {LinePoint[i].pos[1], LinePoint[previous].pos[1], LinePoint[following].pos[1]};
        for (int first = 0; first < 2; first++) {
            for (int second = first + 1; second < 3; second++) {
                if (heights[first] < heights[second]) {
                    float exchange = heights[first];
                    heights[first] = heights[second];
                    heights[second] = exchange;
                }
            }
        }
        sceVu0FVECTOR from;
        sceVu0FVECTOR to;
        sceVu0FVECTOR hit;
        sceVu0CopyVector(from, LinePoint[i].pos);
        sceVu0CopyVector(to, LinePoint[i].pos);
        from[1] = heights[0] + 4.0f;
        to[1] = heights[2] - 1.0f;
        float damping = 0.95f;
        if (CheckHit(polys, poly_count, from, to, hit, 1, 9) >= 0 && hit[1] + 1.0f >= LinePoint[i].pos[1]) {
            LinePoint[i].pos[1] += 0.4f * (hit[1] + 1.0f - LinePoint[i].pos[1]);
            damping = 0.95f * (i == 63 ? 0.05f : 0.1f);
        }
        sceVu0SubVector(LinePoint[i].velo, LinePoint[i].pos, LinePoint[i].old_pos);
        sceVu0ScaleVector(LinePoint[i].velo, LinePoint[i].velo, damping);
    }
    if (CastingLureTime <= 0) {
        EndCastingLure();
    }
    if (CastingLureFlag != 0) {
        sceVu0CopyVector(LinePoint[63].pos, FlyingPoint.pos);
        if (LineTop < 62) {
            sceVu0ScaleVector(LinePoint[62].pos, FlyingPoint.velo, 0.8f);
        }
        if (LineTop < 61) {
            sceVu0ScaleVector(LinePoint[61].pos, FlyingPoint.velo, 0.5f);
        }
    }
    hari->Correct(polys, poly_count, LureLessFlag == 0 ? 1.0f : 0.4f);
    if (uki != 0) {
        uki->Correct(polys, poly_count, 1.0f);
    }
    float water = GetWaterLevel();
    for (int i = LineTop; i < 64; i++) {
        if (uki != 0 && i == 60) {
            continue;
        }
        if (i == 63) {
            continue;
        }
        FISH_POINT &point = LinePoint[i];
        if (point.pos[1] < water) {
            float lift = water - point.pos[1];
            if (lift > 0.61f) {
                lift = 0.61f;
            }
            if (point.pos[1] < water - 0.05f) {
                point.velo[0] *= 0.1f;
                point.velo[1] *= 0.1f;
                point.velo[2] *= 0.1f;
            }
            point.velo[1] += lift;
        }
    }
    hari->FloatPoint(water);
    if (uki != 0) {
        uki->FloatPoint(water);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishingobj", RodStep__FP6CSceneP1);
#endif

static void BindPosition(float *point0, float *point1, float length, float rate) {
    sceVu0FVECTOR difference;
    sceVu0FVECTOR correction0;
    sceVu0FVECTOR correction1;
    sceVu0SubVector(difference, point0, point1);
    float distance = mgDistVector(difference);
    float error = distance - length;
    sceVu0ScaleVector(correction0, difference, ((1.0f - rate) * error) / distance);
    sceVu0ScaleVector(correction1, difference, (rate * error) / distance);
    mgSubVector(point0, correction0);
    mgAddVector(point1, correction1);
}

void DrawFishingLine() {
    sceVu0FVECTOR start;
    sceVu0FVECTOR fish_pos;
    sceVu0FVECTOR offset;
    int i;
    int ok;
    FISH_POINT *point;
    *(u_long128 *)start = *(u_long128 *)RodPoint[4].pos;
    *(u_long128 *)offset = *(u_long128 *)RodPoint[3].pos;
    sceVu0SubVector(offset, offset, start);
    sceVu0ScaleVector(offset, offset, 0.1f);
    mgAddVector(start, offset);
    *(u_long128 *)fish_pos = *(u_long128 *)FishPoint.pos;
    start[3] = 1.0f;
    fish_pos[3] = 1.0f;
    mgCDrawPrim prim;
    int first_screen[4];
    int second_screen[4];
    prim.Initialize(NULL, NULL);
    prim.DepthTestEnable(1);
    prim.AlphaBlendEnable(1);
    prim.ZMask(MG_Z_MASK_WRITE);
    prim.TextureMapEnable(0);
    prim.Begin(MG_PRIM_LINE);
    prim.Color(220, 220, 220, 0x10);
    if (BattleFlag != 0) {
        ok = mgTransWorldScreen(first_screen, start);
        ok &= mgTransWorldScreen(second_screen, fish_pos);
        if (ok) {
            prim.Vertex4(first_screen);
            prim.Vertex4(second_screen);
        }
    } else {
        ok = mgTransWorldScreen(first_screen, start);
        LinePoint[LineTop + 1].pos[3] = 1.0f;
        ok &= mgTransWorldScreen(second_screen, LinePoint[LineTop + 1].pos);
        if (ok) {
            prim.Vertex4(first_screen);
            prim.Vertex4(second_screen);
        }
        for (i = LineTop + 1; i < 63; i++) {
            LinePoint[i].pos[3] = 1.0f;
            point = &LinePoint[i];
            ok = mgTransWorldScreen(first_screen, point->pos);
            LinePoint[i + 1].pos[3] = 1.0f;
            ok &= mgTransWorldScreen(second_screen, LinePoint[i + 1].pos);
            if (ok) {
                if (point->pos[1] < GetWaterLevel()) {
                    prim.Color(220, 220, 220, 0);
                }
                prim.Vertex4(first_screen);
                if (LinePoint[i + 1].pos[1] < GetWaterLevel()) {
                    prim.Color(220, 220, 220, 0);
                }
                prim.Vertex4(second_screen);
            }
        }
    }
    prim.End();
}

void DrawFishingActionChance() {
    mgCTextureManager *textures = &mgTexManager;
    sceVu0FVECTOR start;
    sceVu0FVECTOR end;
    sceVu0FVECTOR marker;
    sceVu0FVECTOR delta;
    *(u_long128 *)start = *(u_long128 *)RodPoint[4].pos;
    *(u_long128 *)delta = *(u_long128 *)RodPoint[3].pos;
    sceVu0SubVector(delta, delta, start);
    sceVu0ScaleVector(delta, delta, 0.1f);
    mgAddVector(start, delta);
    *(u_long128 *)end = *(u_long128 *)FishPoint.pos;
    start[3] = 1.0f;
    end[3] = 1.0f;
    mgCDrawPrim prim;
    sceVu0FVECTOR offset;
    int top_left[4];
    int bottom_right[4];
    prim.Initialize(NULL, NULL);
    prim.DepthTestEnable(1);
    prim.AlphaBlendEnable(1);
    prim.ZMask(MG_Z_MASK_WRITE);
    prim.TextureMapEnable(0);
    if (BattleFlag != 0 && ActionChanceCnt > 0) {
        sceVu0SubVector(offset, start, end);
        sceVu0ScaleVector(offset, offset, (WaterLevel - end[1]) / offset[1]);
        sceVu0AddVector(marker, end, offset);
        marker[3] = 1.0f;
        if (mgTransWorldPrim3DSprite(top_left, bottom_right, marker, 10.0f, 10.0f, 0) != 0) {
            int center_x = (top_left[0] + bottom_right[0]) / 2;
            int center_y = (top_left[1] + bottom_right[1]) / 2;
            bottom_right[0] = center_x + 256;
            bottom_right[1] = center_y + 224;
            top_left[0] = center_x - 256;
            top_left[1] = center_y - 224;
            prim.DepthTestEnable(0);
            prim.TextureMapEnable(1);
            prim.Coord(1);
            prim.ZMask(MG_Z_MASK_MASKED);
            prim.Begin(MG_PRIM_SPRITE);
            prim.Color(128, 128, 128, 128);
            prim.Texture(textures->GetTexture("fish_juji", -1));
            if (ActionChanceDir > 0) {
                bottom_right[0] += 384;
                top_left[0] += 384;
                prim.TextureCrd(26, 22);
                prim.Vertex4(top_left);
                prim.TextureCrd(52, 44);
                prim.Vertex4(bottom_right);
            }
            if (ActionChanceDir < 0) {
                bottom_right[0] -= 384;
                top_left[0] -= 384;
                prim.TextureCrd(0, 22);
                prim.Vertex4(top_left);
                prim.TextureCrd(26, 44);
                prim.Vertex4(bottom_right);
            }
            prim.End();
        }
    }
}

void InitLureObj(int lure_no, mgCFrame *lure) {
    sceVu0FVECTOR body_pos;
    sceVu0FVECTOR rod_tip;
    float body_length;
    int i;

    memset(&LureObj, 0, sizeof(LureObj));
    LureLessFlag = 0;
    if (lure_no < 0 || lure == NULL) {
        LureObj.point[0].pos[0] = 0.0f;
        LureLessFlag = 1;
        LureObj.point_num = 1;
        LureObj.point[0].pos[3] = 1.0f;
        LureObj.point[0].pos[1] = 0.0f;
        LureObj.point[0].pos[2] = 0.0f;
        return;
    }
    lure->SetPosition(0.0f, 0.0f, 0.0f);
    lure->SetRotation(0.0f, 0.0f, 0.0f);
    mgCFrame *body = lure->SearchFrame("obj1");
    body_length = 4.0f;
    if (lure_no == 0 && body != NULL) {
        body->GetWorldPosition0(body_pos);
        body_length = mgDistVector(body_pos);
    }
    LureObj.point_num = 5;
    for (i = 0; i < LureObj.point_num; i++) {
        FISH_POINT *point = &LureObj.point[i];
        mgZeroVector(point->pos);
        mgZeroVector(point->old_pos);
        mgZeroVector(point->velo);
    }
    LureObj.point[0].pos[0] = 0.0f;
    LureObj.point[0].pos[1] = 0.0f;
    LureObj.point[0].pos[2] = 0.0f;
    LureObj.point[0].pos[3] = 1.0f;
    LureObj.point[1].pos[0] = 0.0f;
    LureObj.point[1].pos[1] = 0.0f;
    LureObj.point[1].pos[2] = body_length;
    LureObj.point[1].pos[3] = 1.0f;
    LureObj.point[2].pos[0] = 0.0f;
    LureObj.point[2].pos[1] = -1.0f;
    LureObj.point[2].pos[2] = 0.5f * body_length;
    LureObj.point[2].pos[3] = 1.0f;
    LureObj.point[3].pos[0] = 0.0f;
    LureObj.point[3].pos[1] = 1.0f;
    LureObj.point[3].pos[3] = 1.0f;
    LureObj.point[4].pos[3] = 1.0f;
    LureObj.point[3].pos[2] = 2.0f + body_length;
    LureObj.point[4].pos[2] = 2.0f + body_length;
    LureObj.point[4].pos[0] = 0.0f;
    LureObj.point[4].pos[1] = -1.0f;
    FISH_POINT *second_point = &LureObj.point[1];
    float dist = mgDistVector(LureObj.point[0].pos, second_point->pos);
    LureObj.bind[0].point1 = second_point;
    LureObj.bind[0].point0 = &LureObj.point[0];
    LureObj.bind[0].length = dist;
    LureObj.bind[0].rate = 0.5f;
    FISH_POINT *third_point = &LureObj.point[2];
    dist = mgDistVector(second_point->pos, third_point->pos);
    LureObj.bind[1].point0 = second_point;
    LureObj.bind[1].rate = 0.5f;
    LureObj.bind[1].point1 = third_point;
    LureObj.bind[1].length = dist;
    dist = mgDistVector(LureObj.point[0].pos, third_point->pos);
    LureObj.bind[2].point0 = &LureObj.point[0];
    LureObj.bind[2].rate = 0.5f;
    LureObj.bind[2].point1 = third_point;
    LureObj.bind[2].length = dist;
    FISH_POINT *fourth_point = &LureObj.point[3];
    dist = mgDistVector(second_point->pos, fourth_point->pos);
    LureObj.bind[3].point0 = second_point;
    LureObj.bind[3].rate = 0.5f;
    LureObj.bind[3].point1 = fourth_point;
    LureObj.bind[3].length = dist;
    FISH_POINT *fifth_point = &LureObj.point[4];
    dist = mgDistVector(second_point->pos, fifth_point->pos);
    LureObj.bind[4].point0 = second_point;
    LureObj.bind[4].rate = 0.5f;
    LureObj.bind[4].point1 = fifth_point;
    LureObj.bind[4].length = dist;
    dist = mgDistVector(fourth_point->pos, fifth_point->pos);
    LureObj.bind[5].point0 = fourth_point;
    LureObj.bind[5].point1 = fifth_point;
    LureObj.bind[5].length = dist;
    LureObj.bind[5].rate = 0.5f;
    LureObj.bind_num = 6;
    LureObj.float_num = 2;
    LureObj.float_info[0].point1 = &LureObj.point[0];
    LureObj.float_info[1].point1 = second_point;
    LureObj.float_info[0].point0 = third_point;
    LureObj.float_info[1].point0 = third_point;
    LureObj.float_info[0].buoyancy = 2.5f;
    LureObj.float_info[1].buoyancy = 2.5f;
    LureObj.float_info[0].unk_8 = 0;
    LureObj.float_info[1].unk_8 = 0;
    SaoFrame[7]->GetWorldPosition0(rod_tip);
    for (int i = 0; i < LureObj.point_num; i++) {
        mgAddVector(LureObj.point[i].pos, rod_tip);
    }
}
#ifdef NONMATCHING
void InitUkiObj(int no, mgCFrame *uki, mgCFrame *hari) {
    sceVu0FVECTOR rod_tip;
    SaoFrame[7]->GetWorldPosition0(rod_tip);

    UkiObj.point_num = 4;
    for (int i = 0; i < UkiObj.point_num; i++) {
        mgZeroVector(UkiObj.point[i].pos);
        mgZeroVector(UkiObj.point[i].old_pos);
        mgZeroVector(UkiObj.point[i].velo);
    }
    SetObjectPoint(UkiObj.point[0], 0.0f, 2.0f, 0.0f);
    SetObjectPoint(UkiObj.point[1], 0.0f, -1.5f, 3.0f);
    SetObjectPoint(UkiObj.point[2], 2.5980763f, -1.5f, -1.5f);
    SetObjectPoint(UkiObj.point[3], -2.5980763f, -1.5f, -1.5f);
    for (int i = 0; i < UkiObj.point_num; i++) {
        mgAddVector(UkiObj.point[i].pos, rod_tip);
    }
    SetObjectBind(UkiObj.bind[0], UkiObj.point[0], UkiObj.point[1]);
    SetObjectBind(UkiObj.bind[1], UkiObj.point[0], UkiObj.point[2]);
    SetObjectBind(UkiObj.bind[2], UkiObj.point[0], UkiObj.point[3]);
    SetObjectBind(UkiObj.bind[3], UkiObj.point[1], UkiObj.point[2]);
    SetObjectBind(UkiObj.bind[4], UkiObj.point[2], UkiObj.point[3]);
    SetObjectBind(UkiObj.bind[5], UkiObj.point[3], UkiObj.point[1]);
    UkiObj.bind_num = 6;
    UkiObj.float_num = 3;
    for (int i = 0; i < UkiObj.float_num; i++) {
        UkiObj.float_info[i].point0 = &UkiObj.point[i + 1];
        UkiObj.float_info[i].point1 = &UkiObj.point[0];
        UkiObj.float_info[i].unk_8 = 0;
        UkiObj.float_info[i].buoyancy = 1.6f;
    }

    HariObj.point_num = 3;
    for (int i = 0; i < HariObj.point_num; i++) {
        mgZeroVector(HariObj.point[i].pos);
        mgZeroVector(HariObj.point[i].old_pos);
        mgZeroVector(HariObj.point[i].velo);
    }
    SetObjectPoint(HariObj.point[0], 0.0f, 0.0f, 0.0f);
    SetObjectPoint(HariObj.point[1], 1.0f, -4.0f, 0.0f);
    SetObjectPoint(HariObj.point[2], -1.0f, -4.0f, 0.0f);
    for (int i = 0; i < HariObj.point_num; i++) {
        mgAddVector(HariObj.point[i].pos, rod_tip);
    }
    SetObjectBind(HariObj.bind[0], HariObj.point[0], HariObj.point[1]);
    SetObjectBind(HariObj.bind[1], HariObj.point[0], HariObj.point[2]);
    SetObjectBind(HariObj.bind[2], HariObj.point[1], HariObj.point[2]);
    HariObj.bind_num = 3;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishingobj", InitUkiObj__FiP8mgCFrameP8mgCFrame);
#endif

void CFishObj::MovePoint() {
    for (int i = 0; i < point_num; i++) {
        *(u_long128 *)point[i].old_pos = *(u_long128 *)point[i].pos;
        mgAddVector(point[i].pos, point[i].velo);
        point[i].pos[1] -= 0.6f;
    }
}

void CFishObj::FloatPoint(float water_level) {
    for (int i = 0; i < float_num; i++) {
        float first_height = float_info[i].point0->pos[1];
        float second_height = float_info[i].point1->pos[1];
        float span = first_height - second_height;
        float abs_span = span < 0.0f ? -span : span;
        if (abs_span < 0.01f) {
            continue;
        }
        float depth;
        if (first_height > second_height) {
            depth = (water_level - second_height) / (span < 0.0f ? -span : span);
        } else {
            depth = (water_level - first_height) / (span < 0.0f ? -span : span);
        }
        if (depth < 0.0f) {
            continue;
        }
        if (depth > 1.0f) {
            depth = 1.0f;
        }

        float_info[i].point0->velo[0] *= 0.3f;
        float_info[i].point0->velo[2] *= 0.3f;
        float_info[i].point0->velo[1] += float_info[i].buoyancy * depth;
    }
    for (int i = 0; i < point_num; i++) {
        if (point[i].pos[1] < water_level) {
            point[i].velo[0] *= 0.1f;
            if (point[i].velo[1] < 0.0f) {
                point[i].velo[1] *= 0.1f;
            }
            point[i].velo[2] *= 0.1f;
        }
    }
}

void CFishObj::BindStep() {
    for (int i = 0; i < bind_num; i++) {
        BindPosition(bind[i].point0->pos, bind[i].point1->pos, bind[i].length, bind[i].rate);
    }
}

void CFishObj::Correct(CCPoly *poly, int poly_num, float damping) {
    for (int i = 0; i < point_num; i++) {
        float friction = 0.95f;
        float upper;
        float lower;
        float y = point[i].pos[1];
        float old_y = point[i].old_pos[1];
        if (y > old_y) {
            upper = y;
        } else {
            upper = old_y;
        }
        lower = y < old_y ? y : old_y;
        sceVu0FVECTOR from;
        sceVu0FVECTOR to;
        sceVu0FVECTOR hit;
        sceVu0FVECTOR correction;
        mgZeroVector(correction);
        u_long128 position = *(u_long128 *)point[i].pos;
        *(u_long128 *)from = position;
        *(u_long128 *)to = position;
        from[1] = upper + 4.0f;
        to[1] = lower - 1.0f;
        int hit_poly = CheckHit(poly, poly_num, from, to, hit, 1, 9);
        if (hit_poly >= 0 && hit[1] + 1.0f > point[i].pos[1]) {
            correction[1] = -point[i].velo[1] * 0.5f;
            point[i].pos[1] += hit[1] + 1.0f - point[i].pos[1];
            friction *= 0.2f;
        }
        sceVu0SubVector(point[i].velo, point[i].pos, point[i].old_pos);
        sceVu0ScaleVector(point[i].velo, point[i].velo, friction * damping);
        mgAddVector(point[i].velo, correction);
    }
}

static void ParaBlend(float *out, float time, sceVu0FVECTOR *samples, int count) {
    float interval = 1.0f / (float)(count - 1);
    int index = (int)(time / interval);
    int prev;
    int next;
    int next2;
    next = index + 1;
    prev = index - 1;
    next2 = next + 1;
    float fraction = time - (float)index * interval;
    fraction *= (float)(count - 1);
    if (prev < 0) {
        prev = 0;
    }
    if (index >= count) {
        index = count - 1;
    }
    if (next >= count) {
        next = count - 1;
    }
    if (next2 >= count) {
        next2 = count - 1;
    }
    sceVu0FMATRIX basis = {
        {-1.0f, 3.0f, -3.0f, 1.0f},
        {2.0f, -5.0f, 4.0f, -1.0f},
        {-1.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 2.0f, 0.0f, 0.0f}
    };
    sceVu0FMATRIX control;
    sceVu0TransposeMatrix(basis, basis);
    *(u_long128 *)control[0] = *(u_long128 *)samples[prev];
    control[0][3] = 0.0f;
    *(u_long128 *)control[1] = *(u_long128 *)samples[index];
    control[1][3] = 0.0f;
    *(u_long128 *)control[2] = *(u_long128 *)samples[next];
    control[2][3] = 0.0f;
    *(u_long128 *)control[3] = *(u_long128 *)samples[next2];
    control[3][3] = 0.0f;
    sceVu0TransposeMatrix(control, control);
    mgMulMatrix(basis, basis, control);
    sceVu0TransposeMatrix(basis, basis);
    sceVu0FVECTOR power = {0.0f, 0.0f, 0.0f, 1.0f};
    float square = fraction * fraction;
    power[0] = fraction * square;
    power[1] = square;
    power[2] = fraction;
    sceVu0ScaleVector(power, power, 0.5f);
    sceVu0ApplyMatrix(out, basis, power);
}

// Static initialiser (.init)
extern "C" void __sinit_fishingobj_cpp() {
    memset(&LureObj, 0, sizeof(LureObj));
    memset(&UkiObj, 0, sizeof(UkiObj));
    memset(&HariObj, 0, sizeof(HariObj));
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_975__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_985__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_986__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_1797__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_1798__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_896__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_897__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_898__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_899__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_900__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_901__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_902__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_903__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_1503__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", at_1564__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishingobj", D_0037B08C__DATA);

// Small uninitialised data (.sbss)

// Uninitialised data (.bss)
