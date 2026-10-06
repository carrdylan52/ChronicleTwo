#include "common.h"
#include "runscript_opcodes.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libvu0.h>

#include "actionchara.hpp"
#include "cameracontrol.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dng_effect.hpp"
#include "dng_main.hpp"
#include "dng_object.hpp"
#include "effscript.hpp"
#include "event_func.hpp"
#include "gameutil.hpp"
#include "map.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "mg_camera.hpp"
#include "mg_drawenv.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "monster.hpp"
#include "nd_meswin.hpp"
#include "savedata.hpp"
#include "scenesnd.hpp"
#include "snd_mngr.hpp"
#include "userdata.hpp"
#include "character.hpp"
#include "dng_hud.hpp"
#include "gamepad.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "menucommon.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "object.hpp"
#include "sound.hpp"

static CScene *nowScene; /**< Scene of the running monster. */
CActiveMonster *nowMonster; /**< Monster whose script is running. */
static ACTION_DAMAGE *LastCInfo2; /**< Last damage entry created by the monster script. */

static int _NORMAL_VECTOR(RS_STACKDATA *args, int argc);
static int _COPY_VECTOR(RS_STACKDATA *args, int argc);
static int _ADD_VECTOR(RS_STACKDATA *args, int argc);
static int _SUB_VECTOR(RS_STACKDATA *args, int argc);
static int _SCALE_VECTOR(RS_STACKDATA *args, int argc);
static int _DIV_VECTOR(RS_STACKDATA *args, int argc);
static int _ANGLE_CMP(RS_STACKDATA *args, int argc);
static int _ANGLE_LIMIT(RS_STACKDATA *args, int argc);
static int _SQRT(RS_STACKDATA *args, int argc);
static int _ATAN2F(RS_STACKDATA *args, int argc);
static int _ND_TEST(RS_STACKDATA *args, int argc);
static int _GET_DIST_VECTOR(RS_STACKDATA *args, int argc);
static int _GET_DIST_VECTOR2(RS_STACKDATA *args, int argc);
static int _CALC_IP_CIRCLE_LINE(RS_STACKDATA *args, int argc);
static int _GET_ANGLE_INNER(RS_STACKDATA *args, int argc);
static int _MY_SE_PLAY(RS_STACKDATA *args, int argc);
static int _MY_SE_STOP(RS_STACKDATA *args, int argc);
static int _MONS_SE_PLAY(RS_STACKDATA *args, int argc);
static int _MONS_SE_STOP(RS_STACKDATA *args, int argc);
static int _MONS_SE_LOOP(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_NEXT_REF(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_FOLLOW(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_NEXT_POS(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_MODE(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_SPEED(RS_STACKDATA *args, int argc);
static int _CAMERA_QUAKE(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_CTRL_PARAM1(RS_STACKDATA *args, int argc);
static int _SET_CAMERA_CTRL_PARAM2(RS_STACKDATA *args, int argc);
static int _RESET_CAMERA_CTRL_PARAM(RS_STACKDATA *args, int argc);
static int _GET_RND(RS_STACKDATA *args, int argc);
static int _GET_RNDF(RS_STACKDATA *args, int argc);
static int _V_PUSH(RS_STACKDATA *args, int argc);
static int _V_POP(RS_STACKDATA *args, int argc);
static int _GET_MONSTER_NUM(RS_STACKDATA *args, int argc);
static int _GET_MONSTER_INDEX(RS_STACKDATA *args, int argc);
static int _GET_MONSTER_ID(RS_STACKDATA *args, int argc);
static int _GET_USERID(RS_STACKDATA *args, int argc);
static int _GET_USER_MONS_ID(RS_STACKDATA *args, int argc);
static int _RESET_TIMER(RS_STACKDATA *args, int argc);
static int _GET_TIMER(RS_STACKDATA *args, int argc);
static int _CREATE_MONSTER(RS_STACKDATA *args, int argc);
static int _RUN_EVENT_SCRIPT(RS_STACKDATA *args, int argc);
static int _GET_FRAME_POS(RS_STACKDATA *args, int argc);
static int _GET_OBJ_POS(RS_STACKDATA *args, int argc);
static int _GET_MAPOBJ_POS(RS_STACKDATA *args, int argc);
static int _SET_PAUSE(RS_STACKDATA *args, int argc);
static int _CHECK_PAUSE(RS_STACKDATA *args, int argc);
static int _GET_BIT_FLAG(RS_STACKDATA *args, int argc);
static int _SET_BIT_FLAG(RS_STACKDATA *args, int argc);
static int _GET_ATT_TYPE(RS_STACKDATA *args, int argc);
static int _GET_USER_ATTR(RS_STACKDATA *args, int argc);
static int _TRANS_RESERV_IMG(RS_STACKDATA *args, int argc);
static int _GET_STS_ATTR(RS_STACKDATA *args, int argc);
static int _V_PUSH2(RS_STACKDATA *args, int argc);
static int _V_POP2(RS_STACKDATA *args, int argc);
static int _SET_LOCKON_MODE(RS_STACKDATA *args, int argc);
static int _SET_MOTION_BLUR(RS_STACKDATA *args, int argc);
static int _GET_EVENT_INFO(RS_STACKDATA *args, int argc);
static int _MONS_VOL_CTRL(RS_STACKDATA *args, int argc);
static int _GET_DIST(RS_STACKDATA *args, int argc);
static int _SEARCH_AREA(RS_STACKDATA *args, int argc);
static int _SEARCH_AREA2(RS_STACKDATA *args, int argc);
static int _GET_PLACE_POS(RS_STACKDATA *args, int argc);
static int _SET_PLACE_POS(RS_STACKDATA *args, int argc);
static int _GET_INDEX_POS(RS_STACKDATA *args, int argc);
static int _GET_POS(RS_STACKDATA *args, int argc);
static int _SET_POS(RS_STACKDATA *args, int argc);
static int _GET_ROT(RS_STACKDATA *args, int argc);
static int _SET_ROT(RS_STACKDATA *args, int argc);
static int _SET_NEXT_ROT(RS_STACKDATA *args, int argc);
static int _SET_NEXT_POS(RS_STACKDATA *args, int argc);
static int _CHK_MOVE_END(RS_STACKDATA *args, int argc);
static int _RESET_MOVE(RS_STACKDATA *args, int argc);
static int _GET_TARGET_POS(RS_STACKDATA *args, int argc);
static int _GET_TARGET_DIST(RS_STACKDATA *args, int argc);
static int _GET_TARGET_ANGLE(RS_STACKDATA *args, int argc);
static int _GET_TARGET_REF_POS(RS_STACKDATA *args, int argc);
static int _GET_REF_DIR(RS_STACKDATA *args, int argc);
static int _GET_REFANGLE_POS(RS_STACKDATA *args, int argc);
static int _GET_TARGET_ROT(RS_STACKDATA *args, int argc);
static int _GET_REF_ANGLE(RS_STACKDATA *args, int argc);
static int _GET_HIGH(RS_STACKDATA *args, int argc);
static int _GET_NEAR_MONS_POS(RS_STACKDATA *args, int argc);
static int _GET_TARGET_OLD_POS(RS_STACKDATA *args, int argc);
static int _GET_TARGET_SPEED(RS_STACKDATA *args, int argc);
static int _CALC_MOVE_NEXT_POS(RS_STACKDATA *args, int argc);
static int _GET_POSREF_ANGLE(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_POS(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_ROT(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_DIST(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_ANGLE(RS_STACKDATA *args, int argc);
static int _GET_REF_ROT(RS_STACKDATA *args, int argc);
static int _GET_REF_ROT2(RS_STACKDATA *args, int argc);
static int _FLYING_SEARCH_AREA(RS_STACKDATA *args, int argc);
static int _GET_HIGH2(RS_STACKDATA *args, int argc);
static int _GET_RANGE_MONS_ID(RS_STACKDATA *args, int argc);
static int _GET_ENTRY_OBJ_POS(RS_STACKDATA *args, int argc);
static int _SET_OBJ(RS_STACKDATA *args, int argc);
static int _SET_BODY(RS_STACKDATA *args, int argc);
static int _SET_DMG(RS_STACKDATA *args, int argc);
static int _SET_DMG2(RS_STACKDATA *args, int argc);
static int _LINK_MAP_TO_OBJECT(RS_STACKDATA *args, int argc);
static int _LINK_OBJECT_TO_PIECE(RS_STACKDATA *args, int argc);
static int _LOAD_EFFECT_SCRIPT(RS_STACKDATA *args, int argc);
static int _SET_SCOOP(RS_STACKDATA *args, int argc);
static int _LOAD_RESERV_IMG(RS_STACKDATA *args, int argc);
static int _SET_PRIORITY_LIMMIT(RS_STACKDATA *args, int argc);
static int _SET_MODEL_LIGHT_SWITCH(RS_STACKDATA *args, int argc);
static int _SET_MODEL_LIGHT_COLOR(RS_STACKDATA *args, int argc);
static int _SET_ALPHA(RS_STACKDATA *args, int argc);
static int _SET_SCALE(RS_STACKDATA *args, int argc);
static int _SET_INDEX_ALPHA(RS_STACKDATA *args, int argc);
static int _SET_PALLET_ANIM(RS_STACKDATA *args, int argc);
static int _RESET_PALLET_ANIM(RS_STACKDATA *args, int argc);
static int _SET_ATTRIB(RS_STACKDATA *args, int argc);
static int _SET_STATUS(RS_STACKDATA *args, int argc);
static int _SET_INT_FLAG(RS_STACKDATA *args, int argc);
static int _SET_ACT_STATUS(RS_STACKDATA *args, int argc);
static int _SET_MUTEKI(RS_STACKDATA *args, int argc);
static int _SET_GRAVITY(RS_STACKDATA *args, int argc);
static int _SET_COLLISION(RS_STACKDATA *args, int argc);
static int _GET_GEKIRIN(RS_STACKDATA *args, int argc);
static int _GET_PRIORITY(RS_STACKDATA *args, int argc);
static int _SET_CLIP_DIST(RS_STACKDATA *args, int argc);
static int _SET_PIYORI_MARK(RS_STACKDATA *args, int argc);
static int _CHECK_PIYORI(RS_STACKDATA *args, int argc);
static int _GET_SCALE(RS_STACKDATA *args, int argc);
static int _GET_MONS_WIDTH(RS_STACKDATA *args, int argc);
static int _BLOW_START(RS_STACKDATA *args, int argc);
static int _SET_DEAD_START(RS_STACKDATA *args, int argc);
static int _SET_DEAD_OFF(RS_STACKDATA *args, int argc);
static int _SET_SHROW_END(RS_STACKDATA *args, int argc);
static int _GET_BASE_ATTACK(RS_STACKDATA *args, int argc);
static int _SET_DEF_RATE(RS_STACKDATA *args, int argc);
static int _SET_MONSTER_LIFE(RS_STACKDATA *args, int argc);
static int _GET_MONSTER_LIFE(RS_STACKDATA *args, int argc);
static int _GET_NO_DAMAGE_CNT(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_LIFEI(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_LIFEF(RS_STACKDATA *args, int argc);
static int _SET_ACTIVE_MONS_LIFEI(RS_STACKDATA *args, int argc);
static int _SET_ACTIVE_MONS_LIFEF(RS_STACKDATA *args, int argc);
static int _GET_ACTIVE_MONS_MAX_LIFE(RS_STACKDATA *args, int argc);
static int _SET_DAMAGE_SCORE(RS_STACKDATA *args, int argc);
static int _GET_MONS_GRADE(RS_STACKDATA *args, int argc);
static int _SET_ESCAPE_RATE(RS_STACKDATA *args, int argc);
static int _SET_GUARD_RATE(RS_STACKDATA *args, int argc);
static int _SET_EXT_PARAM_RATE(RS_STACKDATA *args, int argc);
static int _GET_BOSS_FLAG(RS_STACKDATA *args, int argc);
static int _SET_INDEXOBJ_SIZE(RS_STACKDATA *args, int argc);
static int _GET_INDEXOBJ_SIZE(RS_STACKDATA *args, int argc);
static int _RESET_MOTION(RS_STACKDATA *args, int argc);
static int _SET_MOS(RS_STACKDATA *args, int argc);
static int _CHECK_MOS_END(RS_STACKDATA *args, int argc);
static int _NOW_MOS_WAIT(RS_STACKDATA *args, int argc);
static int _GET_MOS_STATUS(RS_STACKDATA *args, int argc);
static int _ESM_CREATE(RS_STACKDATA *args, int argc);
static int _ESM_FINISH(RS_STACKDATA *args, int argc);
static int _ESM_DELETE(RS_STACKDATA *args, int argc);
static int _ESM_SET_VECT1(RS_STACKDATA *args, int argc);
static int _ESM_GET_VECT1(RS_STACKDATA *args, int argc);
static int _ESM_SET_VECT2(RS_STACKDATA *args, int argc);
static int _ESM_GET_VECT2(RS_STACKDATA *args, int argc);
static int _ESM_SET_TARGET_ID(RS_STACKDATA *args, int argc);
static int _ESM_GET_TARGET_ID(RS_STACKDATA *args, int argc);
static int _ESM_SET_USER_ID(RS_STACKDATA *args, int argc);
static int _ESM_GET_USER_ID(RS_STACKDATA *args, int argc);
static int _ESM_SET_VALUE(RS_STACKDATA *args, int argc);
static int _SW_EFFECT(RS_STACKDATA *args, int argc);
static int _ESM_GET_NOTUESD_TEXB(RS_STACKDATA *args, int argc);
static int _ESM_ADD_TEXB(RS_STACKDATA *args, int argc);
static int _SHOT_ROCKET_LAUNCHER(RS_STACKDATA *args, int argc);
static int _ESM_ALL_CLEAR(RS_STACKDATA *args, int argc);
static int _SET_MAPOBJ_SHOW(RS_STACKDATA *args, int argc);

static int (*ext_func[256])(RS_STACKDATA *, int); /**< External functions indexed by script number. */

/**
 * Associates monster-script function numbers with their handlers.
 */
static RS_EXTFUNC_INFO ext_func_info[] = {
    { _NORMAL_VECTOR, 0 },
    { _COPY_VECTOR, 1 },
    { _ADD_VECTOR, 2 },
    { _SUB_VECTOR, 3 },
    { _SCALE_VECTOR, 4 },
    { _DIV_VECTOR, 5 },
    { _ANGLE_CMP, 6 },
    { _ANGLE_LIMIT, 7 },
    { _SQRT, 8 },
    { _ATAN2F, 9 },
    { _ND_TEST, 10 },
    { _GET_DIST_VECTOR, 11 },
    { _GET_DIST_VECTOR2, 12 },
    { _CALC_IP_CIRCLE_LINE, 13 },
    { _GET_ANGLE_INNER, 14 },
    { _MY_SE_PLAY, 21 },
    { _MY_SE_STOP, 22 },
    { _MONS_SE_PLAY, 23 },
    { _MONS_SE_STOP, 24 },
    { _MONS_SE_LOOP, 65 },
    { _SET_CAMERA_NEXT_REF, 25 },
    { _SET_CAMERA_FOLLOW, 26 },
    { _SET_CAMERA_NEXT_POS, 27 },
    { _SET_CAMERA_MODE, 28 },
    { _SET_CAMERA_SPEED, 29 },
    { _CAMERA_QUAKE, 30 },
    { _SET_CAMERA_CTRL_PARAM1, 31 },
    { _SET_CAMERA_CTRL_PARAM2, 32 },
    { _RESET_CAMERA_CTRL_PARAM, 33 },
    { _GET_RND, 35 },
    { _GET_RNDF, 36 },
    { _V_PUSH, 37 },
    { _V_POP, 38 },
    { _GET_MONSTER_NUM, 39 },
    { _GET_MONSTER_INDEX, 40 },
    { _GET_MONSTER_ID, 41 },
    { _GET_USERID, 42 },
    { _GET_USER_MONS_ID, 43 },
    { _RESET_TIMER, 44 },
    { _GET_TIMER, 45 },
    { _CREATE_MONSTER, 46 },
    { _RUN_EVENT_SCRIPT, 47 },
    { _GET_FRAME_POS, 48 },
    { _GET_OBJ_POS, 49 },
    { _GET_MAPOBJ_POS, 50 },
    { _SET_PAUSE, 51 },
    { _CHECK_PAUSE, 52 },
    { _GET_BIT_FLAG, 53 },
    { _SET_BIT_FLAG, 54 },
    { _GET_ATT_TYPE, 55 },
    { _GET_USER_ATTR, 56 },
    { _TRANS_RESERV_IMG, 57 },
    { _GET_STS_ATTR, 58 },
    { _V_PUSH2, 59 },
    { _V_POP2, 60 },
    { _SET_LOCKON_MODE, 61 },
    { _SET_MOTION_BLUR, 62 },
    { _GET_EVENT_INFO, 64 },
    { _MONS_VOL_CTRL, 66 },
    { _GET_DIST, 70 },
    { _SEARCH_AREA, 71 },
    { _SEARCH_AREA2, 108 },
    { _GET_PLACE_POS, 72 },
    { _SET_PLACE_POS, 73 },
    { _GET_INDEX_POS, 74 },
    { _GET_POS, 75 },
    { _SET_POS, 76 },
    { _GET_ROT, 77 },
    { _SET_ROT, 78 },
    { _SET_NEXT_ROT, 79 },
    { _SET_NEXT_POS, 80 },
    { _CHK_MOVE_END, 81 },
    { _RESET_MOVE, 82 },
    { _GET_TARGET_POS, 83 },
    { _GET_TARGET_DIST, 84 },
    { _GET_TARGET_ANGLE, 85 },
    { _GET_TARGET_REF_POS, 86 },
    { _GET_REF_DIR, 87 },
    { _GET_REFANGLE_POS, 88 },
    { _GET_TARGET_ROT, 89 },
    { _GET_REF_ANGLE, 90 },
    { _GET_HIGH, 91 },
    { _GET_NEAR_MONS_POS, 92 },
    { _GET_TARGET_OLD_POS, 93 },
    { _GET_TARGET_SPEED, 94 },
    { _CALC_MOVE_NEXT_POS, 95 },
    { _GET_POSREF_ANGLE, 96 },
    { _GET_ACTIVE_MONS_POS, 97 },
    { _GET_ACTIVE_MONS_ROT, 98 },
    { _GET_ACTIVE_MONS_DIST, 99 },
    { _GET_ACTIVE_MONS_ANGLE, 100 },
    { _GET_REF_ROT, 101 },
    { _GET_REF_ROT2, 102 },
    { _FLYING_SEARCH_AREA, 103 },
    { _GET_HIGH2, 104 },
    { _GET_RANGE_MONS_ID, 105 },
    { _GET_ENTRY_OBJ_POS, 106 },
    { _SET_OBJ, 110 },
    { _SET_BODY, 111 },
    { _SET_DMG, 112 },
    { _SET_DMG2, 113 },
    { _LINK_MAP_TO_OBJECT, 114 },
    { _LINK_OBJECT_TO_PIECE, 115 },
    { _LOAD_EFFECT_SCRIPT, 116 },
    { _SET_SCOOP, 117 },
    { _LOAD_RESERV_IMG, 118 },
    { _SET_PRIORITY_LIMMIT, 119 },
    { _SET_MODEL_LIGHT_SWITCH, 120 },
    { _SET_MODEL_LIGHT_COLOR, 121 },
    { _SET_ALPHA, 122 },
    { _SET_SCALE, 123 },
    { _SET_INDEX_ALPHA, 124 },
    { _SET_PALLET_ANIM, 125 },
    { _RESET_PALLET_ANIM, 126 },
    { _SET_ATTRIB, 127 },
    { _SET_STATUS, 128 },
    { _SET_INT_FLAG, 129 },
    { _SET_ACT_STATUS, 130 },
    { _SET_MUTEKI, 131 },
    { _SET_GRAVITY, 132 },
    { _SET_COLLISION, 133 },
    { _GET_GEKIRIN, 134 },
    { _GET_PRIORITY, 135 },
    { _SET_CLIP_DIST, 136 },
    { _SET_PIYORI_MARK, 137 },
    { _CHECK_PIYORI, 138 },
    { _GET_SCALE, 139 },
    { _GET_MONS_WIDTH, 140 },
    { _BLOW_START, 150 },
    { _SET_DEAD_START, 151 },
    { _SET_DEAD_OFF, 152 },
    { _SET_SHROW_END, 153 },
    { _GET_BASE_ATTACK, 160 },
    { _SET_DEF_RATE, 161 },
    { _SET_MONSTER_LIFE, 162 },
    { _GET_MONSTER_LIFE, 163 },
    { _GET_NO_DAMAGE_CNT, 164 },
    { _GET_ACTIVE_MONS_LIFEI, 165 },
    { _GET_ACTIVE_MONS_LIFEF, 166 },
    { _SET_ACTIVE_MONS_LIFEI, 167 },
    { _SET_ACTIVE_MONS_LIFEF, 168 },
    { _GET_ACTIVE_MONS_MAX_LIFE, 169 },
    { _SET_DAMAGE_SCORE, 170 },
    { _GET_MONS_GRADE, 171 },
    { _SET_ESCAPE_RATE, 173 },
    { _SET_GUARD_RATE, 172 },
    { _SET_EXT_PARAM_RATE, 174 },
    { _GET_BOSS_FLAG, 175 },
    { _SET_INDEXOBJ_SIZE, 176 },
    { _GET_INDEXOBJ_SIZE, 177 },
    { _RESET_MOTION, 180 },
    { _SET_MOS, 182 },
    { _CHECK_MOS_END, 183 },
    { _NOW_MOS_WAIT, 184 },
    { _GET_MOS_STATUS, 185 },
    { _ESM_CREATE, 191 },
    { _ESM_FINISH, 192 },
    { _ESM_DELETE, 193 },
    { _ESM_SET_VECT1, 194 },
    { _ESM_GET_VECT1, 195 },
    { _ESM_SET_VECT2, 196 },
    { _ESM_GET_VECT2, 197 },
    { _ESM_SET_TARGET_ID, 198 },
    { _ESM_GET_TARGET_ID, 199 },
    { _ESM_SET_USER_ID, 200 },
    { _ESM_GET_USER_ID, 201 },
    { _ESM_SET_VALUE, 202 },
    { _SW_EFFECT, 203 },
    { _ESM_GET_NOTUESD_TEXB, 204 },
    { _ESM_ADD_TEXB, 205 },
    { _SHOT_ROCKET_LAUNCHER, 206 },
    { _ESM_ALL_CLEAR, 207 },
    { _SET_MAPOBJ_SHOW, 63 },
    { NULL, -1 }
};

/**
 * Holds a vector whose components are copied together.
 */
union ScriptVector {
    sceVu0FVECTOR values; /**< Vector components. */
    u_long128 quadword;   /**< All components as one aligned value. */
};

// Code (.text)
void CMonsterMan::RunScript(int no) {
    s16 program;

    nowScene = scene;
    nowMonster = active[no];
    if (nowMonster != NULL) {
        program = nowMonster->req_prog;
        if (program != MONSTER_PROG_RUNNING) {
            if (nowMonster->mons_script.check_program(program)) {
                nowMonster->mons_script.run(program);
                nowMonster->now_prog = program;
                nowMonster->req_prog = MONSTER_PROG_RUNNING;
            }
        } else {
            nowMonster->mons_script.resume();
            if (nowMonster->mons_script.end) {
                nowMonster->req_prog = MONSTER_PROG_MAIN;
            }
        }
    }
}

/**
 * Reads an integer argument, converting a float value.
 */
static int GetStackInt(RS_STACKDATA *data) {
    if (data->type == RS_FLOAT) {
        return (int)data->f;
    }
    return data->i;
}

/**
 * Reads a float argument, converting an integer value.
 */
static float GetStackFloat(RS_STACKDATA *data) {
    if (data->type == RS_INT) {
        return (float)data->i;
    }
    return data->f;
}

/**
 * Reads a string argument.
 */
static char *GetStackString(RS_STACKDATA *data) {
    return data->s;
}

/**
 * Stores an int through an argument that references a stack slot.
 */
static void SetStack(RS_STACKDATA *data, int value) {
    if (data->type == RS_PTR) {
        data->p->i = value;
    }
}

/**
 * Stores a float through an argument that references a stack slot.
 */
static void SetStack(RS_STACKDATA *data, float value) {
    if (data->type == RS_PTR) {
        data->p->f = value;
    }
}

/**
 * Reads three components and advances the argument pointer.
 */
static void GetStackVector(float *vector, RS_STACKDATA **stack) {
    vector[0] = GetStackFloat((*stack)++);
    vector[1] = GetStackFloat((*stack)++);
    vector[2] = GetStackFloat((*stack)++);
    vector[3] = 1.0f;
}

/**
 * Stores three components and advances the argument pointer.
 */
static void SetStackVector(float *vector, RS_STACKDATA **stack) {
    SetStack((*stack)++, vector[0]);
    SetStack((*stack)++, vector[1]);
    SetStack((*stack)++, vector[2]);
}

/**
 * Stores the square root of a script argument.
 */
static int _SQRT(RS_STACKDATA *args, int argc) {
    float value;

    value = GetStackFloat(args++);
    SetStack(args, (float)sqrt(value));
    return 1;
}

/**
 * Stores the angle of two script arguments.
 */
static int _ATAN2F(RS_STACKDATA *args, int argc) {
    float y;
    float x;

    y = GetStackFloat(args++);
    x = GetStackFloat(args++);
    SetStack(args, atan2f(y, x));
    return 1;
}

s32 _ND_TEST(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Stores the rotation of the running monster's target.
 */
static int _GET_TARGET_ROT(RS_STACKDATA *args, int argc) {
    CCharacter2  *target;
    sceVu0FVECTOR rotation;

    if (argc != 3) {
        return 0;
    }
    target = nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetRotation(rotation);
    SetStack(args++, rotation[0]);
    SetStack(args++, rotation[1]);
    SetStack(args, rotation[2]);
    return 1;
}

/**
 * Stores the running monster's monster id.
 */
static int _GET_MONSTER_INDEX(RS_STACKDATA *args, int argc) {
    SetStack(args, (int)nowMonster->monster_id);
    return 1;
}

/**
 * Sets the running monster's life as an integer or a fraction of its maximum.
 */
static int _SET_MONSTER_LIFE(RS_STACKDATA *args, int argc) {
    int life;

    switch (args->type) {
        case RS_INT:
            life = GetStackInt(args);
            break;
        case RS_FLOAT:
            life = (int)(nowMonster->max_life * GetStackFloat(args));
            break;
        default:
            return 0;
    }
    if (life < 0) {
        life = 0;
    }
    if (life > nowMonster->max_life) {
        life = nowMonster->max_life;
    }
    nowMonster->life = life;
    if (0 < life) {
        nowMonster->state = ACTIVE_MONSTER_LIVE;
    }
    return 1;
}

/**
 * Stores the running monster's chara type.
 */
static int _GET_USERID(RS_STACKDATA *args, int argc) {
    SetStack(args, (int)nowMonster->chara_type);
    return 1;
}

/**
 * Stores the running monster's refer no.
 */
static int _GET_MONSTER_ID(RS_STACKDATA *args, int argc) {
    SetStack(args, (int)nowMonster->refer_no);
    return 1;
}

/**
 * Resets the running monster's motion.
 */
static int _RESET_MOTION(RS_STACKDATA *args, int argc) {
    nowMonster->ResetMotion();
    return 1;
}

/**
 * Stores the position of the monster of a given index.
 */
static int _GET_INDEX_POS(RS_STACKDATA *args, int argc) {
    int             index;
    int             count;
    int             slot;
    CActiveMonster *monster;
    sceVu0FVECTOR   position;

    index = GetStackInt(args++);
    count = ActiveMonster->GetMonsterNum(-1.0f);
    for (slot = 0; slot < count; slot++) {
        monster = ActiveMonster->active[slot];
        if (monster->monster_id == index) {
            monster->GetPosition(position);
            SetStack(args++, position[0]);
            SetStack(args++, position[1]);
            SetStack(args, position[2]);
            return 1;
        }
    }
    return 0;
}

/**
 * Sets the camera's next reference point.
 */
static int _SET_CAMERA_NEXT_REF(RS_STACKDATA *args, int argc) {
    CCameraControl *camera;
    float           x;
    float           y;
    float           z;

    camera = (CCameraControl *)nowScene->GetCamera(nowScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->FollowOff();
    camera->ControlOff();
    x = GetStackFloat(args++);
    y = GetStackFloat(args++);
    z = GetStackFloat(args);
    camera->SetNextRef(x, y, z);
    return 1;
}

/**
 * Sets the running monster's drawing alpha.
 */
static int _SET_ALPHA(RS_STACKDATA *args, int argc) {
    nowMonster->alpha = GetStackFloat(args);
    return 1;
}

/**
 * Sets the drawing alpha of the monster of a given index.
 */
static int _SET_INDEX_ALPHA(RS_STACKDATA *args, int argc) {
    int             index;
    int             count;
    int             slot;
    CActiveMonster *monster;

    index = GetStackInt(args++);
    count = ActiveMonster->GetMonsterNum(-1.0f);
    for (slot = 0; slot < count; slot++) {
        monster = ActiveMonster->active[slot];
        if (monster->monster_id == index) {
            monster->alpha = GetStackFloat(args);
            return 1;
        }
    }
    return 0;
}

/**
 * Enables or disables the camera's follow and control.
 */
static int _SET_CAMERA_FOLLOW(RS_STACKDATA *args, int argc) {
    CCameraControl *camera;

    camera = (CCameraControl *)nowScene->GetCamera(nowScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    if (GetStackInt(args)) {
        camera->FollowOn();
        camera->ControlOn();
    } else {
        camera->FollowOff();
        camera->ControlOff();
    }
    return 1;
}

/**
 * Sets the camera's next position.
 */
static int _SET_CAMERA_NEXT_POS(RS_STACKDATA *args, int argc) {
    CCameraControl *camera;
    float           x;
    float           y;
    float           z;

    camera = (CCameraControl *)nowScene->GetCamera(nowScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->FollowOff();
    camera->ControlOff();
    x = GetStackFloat(args++);
    y = GetStackFloat(args++);
    z = GetStackFloat(args);
    camera->SetNextPos(x, y, z);
    return 1;
}

/**
 * Stores the length of a vector.
 */
static int _GET_DIST_VECTOR(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;

    vector[0] = GetStackFloat(args++);
    vector[1] = GetStackFloat(args++);
    vector[2] = GetStackFloat(args++);
    vector[3] = 1.0f;
    SetStack(args, mgDistVector(vector));
    return 1;
}

/**
 * Stores the distance between two vectors.
 */
static int _GET_DIST_VECTOR2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;
    sceVu0FVECTOR other;

    vector[0] = GetStackFloat(args++);
    vector[1] = GetStackFloat(args++);
    vector[2] = GetStackFloat(args++);
    vector[3] = 1.0f;
    other[0] = GetStackFloat(args++);
    other[1] = GetStackFloat(args++);
    other[2] = GetStackFloat(args++);
    other[3] = 1.0f;
    SetStack(args, mgDistVector(vector, other));
    return 1;
}

/**
 * Sets the running monster's scale.
 */
static int _SET_SCALE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR scale;

    scale[0] = GetStackFloat(args++);
    scale[1] = GetStackFloat(args++);
    scale[2] = GetStackFloat(args);
    scale[3] = 1.0f;
    nowMonster->SetScale(scale);
    return 1;
}

/**
 * Sets the running monster's palette pulse.
 */
static int _SET_PALLET_ANIM(RS_STACKDATA *args, int argc) {
    int           red;
    int           green;
    int           blue;
    int           pulses;
    int           duration;
    int           repeats;
    CPalletAnime *pallet;

    repeats = 0;
    red = GetStackInt(args++);
    green = GetStackInt(args++);
    blue = GetStackInt(args++);
    pulses = GetStackInt(args++);
    duration = GetStackInt(args++);
    if (argc >= 6) {
        repeats = GetStackInt(args);
    }
    pallet = &nowMonster->unk_67c;
    pallet->red = red;
    pallet->green = green;
    pallet->blue = blue;
    pallet->pulse_num = pulses;
    pallet->duration = duration;
    pallet->elapsed = 0;
    pallet->repeats = repeats;
    return 1;
}

/**
 * Stops the running monster's palette pulse.
 */
static int _RESET_PALLET_ANIM(RS_STACKDATA *args, int argc) {
    CPalletAnime *pallet;

    pallet = &nowMonster->unk_67c;
    pallet->duration = 0;
    pallet->elapsed = 0;
    return 1;
}

/**
 * Stores the intersections of a ground-plane circle and a line.
 */
static int _CALC_IP_CIRCLE_LINE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR centre;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    sceVu0FVECTOR hit0;
    sceVu0FVECTOR hit1;
    float         radius;
    int           count;
    float         centre_x;
    float         centre_z;
    float         from_x;
    float         from_z;
    float         to_x;
    float         to_z;

    centre_x = GetStackFloat(args++);
    centre_z = GetStackFloat(args++);
    radius = GetStackFloat(args++);
    from_x = GetStackFloat(args++);
    from_z = GetStackFloat(args++);
    to_x = GetStackFloat(args++);
    to_z = GetStackFloat(args++);
    centre[0] = centre_x;
    centre[1] = 0.0f;
    centre[2] = centre_z;
    centre[3] = 1.0f;
    from[0] = from_x;
    from[1] = 0.0f;
    from[2] = from_z;
    from[3] = 1.0f;
    to[0] = to_x;
    to[1] = 0.0f;
    to[2] = to_z;
    to[3] = 1.0f;
    count = CalcIntersectionPointSphereAndLine(centre, radius, from, to, hit0, hit1);
    SetStack(args++, count);
    if (count == 2) {
        SetStack(args++, hit0[0]);
        SetStack(args++, hit0[2]);
        SetStack(args++, hit1[0]);
        SetStack(args++, hit1[2]);
    }
    if (count == 1) {
        SetStack(args++, hit0[0]);
        SetStack(args, hit0[2]);
    }
    return 1;
}

/**
 * Stores the yaw or full rotation from one position towards another.
 */
static int _GET_POSREF_ANGLE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR direction;
    float         yaw;
    float         pitch;

    from[0] = GetStackFloat(args++);
    from[1] = GetStackFloat(args++);
    from[2] = GetStackFloat(args++);
    from[3] = 1.0f;
    direction[0] = GetStackFloat(args++);
    direction[1] = GetStackFloat(args++);
    direction[2] = GetStackFloat(args++);
    direction[3] = 1.0f;
    sceVu0SubVector(direction, direction, from);
    sceVu0Normalize(direction, direction);
    yaw = atan2f(direction[0], direction[2]);
    pitch = -atan2f(direction[1], sqrtf(direction[0] * direction[0] + direction[2] * direction[2]));
    switch (argc) {
        case 7:
            SetStack(args, yaw);
            break;
        case 9:
            SetStack(args++, pitch);
            SetStack(args++, yaw);
            SetStack(args, 0.0f);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Normalizes a vector passed through stack references.
 */
static int _NORMAL_VECTOR(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;

    vector[0] = args[0].p->f;
    vector[1] = args[1].p->f;
    vector[2] = args[2].p->f;
    vector[3] = 1.0f;
    sceVu0Normalize(vector, vector);
    SetStack(args++, vector[0]);
    SetStack(args++, vector[1]);
    SetStack(args, vector[2]);
    return 1;
}

/**
 * Copies a vector into three stack references.
 */
static int _COPY_VECTOR(RS_STACKDATA *args, int argc) {
    float         x;
    float         y;
    float         z;
    RS_STACKDATA *source;

    source = args;
    source += 3;
    x = GetStackFloat(source++);
    y = GetStackFloat(source++);
    z = GetStackFloat(source);
    SetStack(args++, x);
    SetStack(args++, y);
    SetStack(args, z);
    return 1;
}

/**
 * Adds a vector to three stack references.
 */
static int _ADD_VECTOR(RS_STACKDATA *args, int argc) {
    float         x;
    float         y;
    float         z;
    RS_STACKDATA *source;

    source = args;
    source += 3;
    x = GetStackFloat(source++);
    y = GetStackFloat(source++);
    z = GetStackFloat(source);
    SetStack(args, args[0].p->f + x);
    SetStack(args + 1, args[1].p->f + y);
    SetStack(args + 2, args[2].p->f + z);
    return 1;
}

/**
 * Subtracts a vector from three stack references.
 */
static int _SUB_VECTOR(RS_STACKDATA *args, int argc) {
    float         x;
    float         y;
    float         z;
    RS_STACKDATA *source;

    source = args;
    source += 3;
    x = GetStackFloat(source++);
    y = GetStackFloat(source++);
    z = GetStackFloat(source);
    SetStack(args, args[0].p->f - x);
    SetStack(args + 1, args[1].p->f - y);
    SetStack(args + 2, args[2].p->f - z);
    return 1;
}

/**
 * Scales a vector passed through stack references.
 */
static int _SCALE_VECTOR(RS_STACKDATA *args, int argc) {
    float scale;

    scale = GetStackFloat(args + 3);
    SetStack(args, args[0].p->f * scale);
    SetStack(args + 1, args[1].p->f * scale);
    SetStack(args + 2, args[2].p->f * scale);
    return 1;
}

/**
 * Divides a vector passed through stack references by a nonzero scalar.
 */
static int _DIV_VECTOR(RS_STACKDATA *args, int argc) {
    float scale;

    scale = GetStackFloat(args + 3);
    if (scale == 0.0f) {
        return 0;
    }
    SetStack(args, args[0].p->f / scale);
    SetStack(args + 1, args[1].p->f / scale);
    SetStack(args + 2, args[2].p->f / scale);
    return 1;
}

/**
 * Stores whether two angles are within the requested tolerance.
 */
static int _ANGLE_CMP(RS_STACKDATA *args, int argc) {
    float first;
    float second;
    float tolerance;

    first = GetStackFloat(args++);
    second = GetStackFloat(args++);
    tolerance = GetStackFloat(args++);
    SetStack(args, mgAngleCmp(first, second, tolerance));
    return 1;
}

/**
 * Wraps an angle passed through a stack reference.
 */
static int _ANGLE_LIMIT(RS_STACKDATA *args, int argc) {
    SetStack(args, mgAngleLimit(args->p->f));
    return 1;
}

/**
 * Stores the previous position of the running monster's target.
 */
static int _GET_TARGET_OLD_POS(RS_STACKDATA *args, int argc) {
    CActionChara *target;
    sceVu0FVECTOR position;

    target = (CActionChara *)nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    sceVu0CopyVector(position, target->old_pos);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Stores the distance the running monster's target moved this step.
 */
static int _GET_TARGET_SPEED(RS_STACKDATA *args, int argc) {
    CActionChara *target;
    sceVu0FVECTOR position;
    sceVu0FVECTOR old_position;

    target = (CActionChara *)nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetPosition(position);
    sceVu0CopyVector(old_position, target->old_pos);
    SetStack(args, mgDistVector(position, old_position));
    return 1;
}

/**
 * Stores the next position on a move and whether the destination was reached.
 */
static int _CALC_MOVE_NEXT_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    sceVu0FVECTOR position;
    float         speed;
    int           arrived;

    from[0] = GetStackFloat(args++);
    from[1] = GetStackFloat(args++);
    from[2] = GetStackFloat(args++);
    to[0] = GetStackFloat(args++);
    to[1] = GetStackFloat(args++);
    to[2] = GetStackFloat(args++);
    speed = GetStackFloat(args++);
    arrived = CalcMoveNextPos(from, to, speed, position);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args++, position[2]);
    SetStack(args, arrived);
    return 1;
}

/**
 * Reads the running monster's life as a count or a share of its maximum.
 */
static int _GET_MONSTER_LIFE(RS_STACKDATA *args, int argc) {
    if (args->type != RS_PTR) {
        return 0;
    }
    if (args->p->type == RS_INT) {
        SetStack(args, nowMonster->life);
    } else if (args->p->type == RS_FLOAT) {
        SetStack(args, (float)nowMonster->life / (float)nowMonster->max_life);
    } else {
        return 0;
    }
    return 1;
}

/**
 * Reads the number of hits that did the running monster no damage.
 */
static int _GET_NO_DAMAGE_CNT(RS_STACKDATA *args, int argc) {
    SetStack(args, nowMonster->no_damage_cnt);
    return 1;
}

/**
 * Reads the remaining life of a selected monster.
 */
static int _GET_ACTIVE_MONS_LIFEI(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    SetStack(args, monster->life);
    return 1;
}

/**
 * Reads the share of maximum life left for a selected monster.
 */
static int _GET_ACTIVE_MONS_LIFEF(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    SetStack(args, (float)monster->life / (float)monster->max_life);
    return 1;
}

/**
 * Adds an integer amount to a selected monster's life within its limits.
 */
static int _SET_ACTIVE_MONS_LIFEI(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;
    int             change;
    int             life;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    change = GetStackInt(args);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    if (change < 0) {
        life = monster->life + change;
        if (life < 0) {
            monster->life = 0;
        } else {
            monster->life = life;
        }
    } else {
        life = monster->life + change;
        if (life > monster->max_life) {
            monster->life = monster->max_life;
        } else {
            monster->life = life;
        }
    }
    return 1;
}

/**
 * Adds a nonnegative share of maximum life to a selected monster.
 */
static int _SET_ACTIVE_MONS_LIFEF(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;
    float           rate;
    int             change;
    int             life;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    rate = GetStackFloat(args);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    change = (int)((float)monster->max_life * rate);
    if (change < 0) {
        change = 0;
    }
    life = monster->life + change;
    if (life > monster->max_life) {
        monster->life = monster->max_life;
    } else {
        monster->life = life;
    }
    return 1;
}

/**
 * Reads the maximum life of a selected monster.
 */
static int _GET_ACTIVE_MONS_MAX_LIFE(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    SetStack(args, monster->max_life);
    return 1;
}

/**
 * Shows a damage number above a selected monster, optionally in green.
 */
static int _SET_DAMAGE_SCORE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   position;
    int             color;
    int             chara_no;
    int             damage;
    CActiveMonster *monster;

    if (argc != 3 && argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    damage = GetStackInt(args++);
    if (argc == 3) {
        color = GetStackInt(args);
    }
    monster->GetPosition(position);
    position[1] += monster->body_height;
    if (color == 1) {
        DamageScore.SetColor(64, 128, 96);
    }
    DamageScore.SetValue(position, damage);
    return 1;
}

/**
 * Reads the grade of a selected monster.
 */
static int _GET_MONS_GRADE(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    SetStack(args, monster->tbl->grade);
    return 1;
}

/**
 * Scales a selected monster's two dodge chances from its base parameters.
 */
static int _SET_ESCAPE_RATE(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    float           rate;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    rate = GetStackFloat(args);
    monster->tbl->escape_rate[0] = (int)((float)monster->base_tbl->escape_rate[0] * rate);
    monster->tbl->escape_rate[1] = (int)((float)monster->base_tbl->escape_rate[1] * rate);
    if ((u8)monster->tbl->escape_rate[0] > 100) {
        monster->tbl->escape_rate[0] = 100;
    }
    if ((u8)monster->tbl->escape_rate[1] > 100) {
        monster->tbl->escape_rate[1] = 100;
    }
    return 1;
}

/**
 * Scales a selected monster's guard chance from its base parameters.
 */
static int _SET_GUARD_RATE(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             chara_no;

    float           rate;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    rate = GetStackFloat(args);
    monster->tbl->guard_rate = (int)((float)monster->base_tbl->guard_rate * rate);
    if ((u8)monster->tbl->guard_rate > 100) {
        monster->tbl->guard_rate = 100;
    }
    return 1;
}

/**
 * Scales the selected attack damage parameters of a monster from their base values.
 */
static int _SET_EXT_PARAM_RATE(RS_STACKDATA *args, int argc) {
    int             chara_no;
    int   mask;
    float rate;
    int   index;
    CActiveMonster *monster;

    if (argc != 3) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    mask = GetStackInt(args++);
    rate = GetStackFloat(args);
    if (chara_no != -1) {
        chara_no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[chara_no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    for (index = 0; index < 12; index++) {
        if (mask & (1 << index)) {
            monster->tbl->ext_param[index] = (int)((float)monster->base_tbl->ext_param[index] * rate);
            if (monster->tbl->ext_param[index] > 100) {
                monster->tbl->ext_param[index] = 100;
            }
        }
    }
    return 1;
}

/**
 * Reads whether the running monster's base parameters mark it as a boss.
 */
static int _GET_BOSS_FLAG(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, nowMonster->base_tbl->boss);
    return 1;
}

/**
 * Resets the dungeon timer.
 */
static int _RESET_TIMER(RS_STACKDATA *args, int argc) {
    DNG_BATTLE_AREA *battle;

    battle = &nowScene->battle_area;
    if (battle == NULL) {
        return 0;
    }
    battle->timer = 0;
    return 1;
}

/**
 * Reads the dungeon timer.
 */
static int _GET_TIMER(RS_STACKDATA *args, int argc) {
    DNG_BATTLE_AREA *battle;

    battle = &nowScene->battle_area;
    if (battle == NULL) {
        return 0;
    }
    SetStack(args, battle->timer);
    return 1;
}

/**
 * Reads the world position of a named frame of the running monster.
 */
static int _GET_FRAME_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    mgCFrame     *frame;
    char         *name;

    name = GetStackString(args++);
    frame = nowMonster->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    frame = frame->SearchFrame(name);
    if (frame == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(position);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Plays a sound from the running monster's bank at its world position.
 */
static int _MY_SE_PLAY(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    float         volume;
    float         pan;
    int           sound;
    unsigned int  bank;
    float         far;
    float         near;

    sound = GetStackInt(args);
    bank = nowMonster->se_bank;
    nowMonster->GetPosition(position);
    far = 1200.0f;
    near = 160.0f;
    sndGetVolPan(&volume, &pan, position, near, far);
    sndSePlayVPf(bank, sound, volume, pan, 0);
    return 1;
}

/**
 * Stops a sound from the running monster's bank.
 */
static int _MY_SE_STOP(RS_STACKDATA *args, int argc) {
    int sound;

    sound = GetStackInt(args);
    sndSeStop(nowMonster->se_bank, sound, 0);
    return 1;
}

/**
 * Reads the event stopwatch limit as an integer.
 */
static int _GET_EVENT_INFO(RS_STACKDATA *args, int argc) {
    int selector;

    selector = GetStackInt(args++);
    switch (selector) {
        case 0:
            SetStack(args, (int)EdEventInfo.stopwatch_limit);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Clears every monster effect belonging to a character number.
 */
static int _ESM_ALL_CLEAR(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    FxScriptMan->ClearEffectFromChrid(GetStackInt(args));
    return 1;
}

/**
 * Reads the inner product of the forward vectors at two horizontal angles.
 */
static int _GET_ANGLE_INNER(RS_STACKDATA *args, int argc) {
    static ScriptVector first_direction = { { 0.0f, 0.0f, 1.0f, 0.0f } };
    static ScriptVector second_direction = { { 0.0f, 0.0f, 1.0f, 0.0f } };
    ScriptVector first;
    ScriptVector second;
    sceVu0FMATRIX unit;
    sceVu0FMATRIX rotation;
    float         first_angle;
    float         second_angle;

    if (argc != 3) {
        return 0;
    }
    first_angle = GetStackFloat(args++);
    second_angle = GetStackFloat(args++);
    first = first_direction;
    second = second_direction;
    sceVu0UnitMatrix(unit);
    sceVu0RotMatrixY(rotation, unit, first_angle);
    sceVu0ApplyMatrix(first.values, rotation, first.values);
    sceVu0RotMatrixY(rotation, unit, second_angle);
    sceVu0ApplyMatrix(second.values, rotation, second.values);
    SetStack(args, sceVu0InnerProduct(first.values, second.values));
    return 1;
}

/**
 * Starts a dungeon camera quake that fades over a given number of frames.
 */
static int _CAMERA_QUAKE(RS_STACKDATA *args, int argc) {
    DNG_BATTLE_AREA *battle;
    float            power;
    int              frames;
    RS_STACKDATA    *next;

    battle = &nowScene->battle_area;
    next = args + 1;
    if (battle == NULL) {
        return 0;
    }
    power = GetStackFloat(args);
    frames = GetStackInt(next);
    battle->quake_power = power;
    battle->quake_step = battle->quake_power / (float)frames;
    battle->quake_count = frames;
    return 1;
}

/**
 * Changes the dungeon camera mode.
 */
static int _SET_CAMERA_MODE(RS_STACKDATA *args, int argc) {
    DNG_BATTLE_AREA *battle;
    int              mode;

    mode = GetStackInt(args);
    battle = &nowScene->battle_area;
    if (battle == NULL) {
        return 0;
    }
    battle->unk_54 = mode;
    return 1;
}

/**
 * Changes the active camera's position speed while keeping its reference speed.
 */
static int _SET_CAMERA_SPEED(RS_STACKDATA *args, int argc) {
    mgCCamera *camera;
    float      speed;

    camera = nowScene->GetCamera(nowScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    speed = GetStackFloat(args);
    camera->SetSpeed(speed, -1.0f);
    return 1;
}

#ifdef NONMATCHING
/**
 * Changes selected limits of the active controlled camera.
 */
static int _SET_CAMERA_CTRL_PARAM1(RS_STACKDATA *args, int argc) {
    CameraCtrlParam *param;
    float            value;

    if (argc > 0 && argc < 5) {
        return 0;
    }
    param = ((CCameraControl *)nowScene->GetCamera(nowScene->active_camera))->GetActiveParam();
    if (argc >= 1) {
        value = GetStackFloat(args + 0);
    }
    if (value != -99999.9 && argc >= 1) {
        param->min_dist = value;
    }
    if (argc >= 2) {
        value = GetStackFloat(args + 1);
    }
    if (value != -99999.9 && argc >= 2) {
        param->max_dist = value;
    }
    if (argc >= 3) {
        value = GetStackFloat(args + 2);
    }
    if (value != -99999.9 && argc >= 3) {
        param->near_height = value;
    }
    if (argc >= 4) {
        value = GetStackFloat(args + 3);
    }
    if (value != -99999.9 && argc >= 4) {
        param->far_height = value;
    }
    return 1;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SET_CAMERA_CTRL_PARAM1__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Changes selected limits of the active controlled camera.
 */
static int _SET_CAMERA_CTRL_PARAM2(RS_STACKDATA *args, int argc) {
    CameraCtrlParam *param;
    float            value;

    if (argc > 0 && argc < 7) {
        return 0;
    }
    param = ((CCameraControl *)nowScene->GetCamera(nowScene->active_camera))->GetActiveParam();
    if (argc >= 1) {
        value = GetStackFloat(args + 0);
    }
    if (value != -99999.9 && argc >= 1) {
        param->height = value;
    }
    if (argc >= 2) {
        value = GetStackFloat(args + 1);
    }
    if (value != -99999.9 && argc >= 2) {
        param->max_height = value;
    }
    if (argc >= 3) {
        value = GetStackFloat(args + 2);
    }
    if (value != -99999.9 && argc >= 3) {
        param->min_height = value;
    }
    if (argc >= 4) {
        value = GetStackFloat(args + 3);
    }
    if (value != -99999.9 && argc >= 4) {
        param->rest_max_height = value;
    }
    if (argc >= 5) {
        value = GetStackFloat(args + 4);
    }
    if (value != -99999.9 && argc >= 5) {
        param->rest_min_height = value;
    }
    if (argc >= 6) {
        value = GetStackFloat(args + 5);
    }
    if (value != -99999.9 && argc >= 6) {
        param->ground_space = value;
    }
    return 1;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SET_CAMERA_CTRL_PARAM2__FP12RS_STACKDATAi);
#endif

/**
 * Restores the active controlled camera's distance and height limits.
 */
static int _RESET_CAMERA_CTRL_PARAM(RS_STACKDATA *args, int argc) {
    CameraCtrlParam *param;

    param = ((CCameraControl *)nowScene->GetCamera(nowScene->active_camera))->GetActiveParam();
    param->min_dist = 100.0f;
    param->max_dist = 160.0f;
    param->near_height = 18.0f;
    param->far_height = 10.0f;
    param->max_height = 40.0f;
    param->min_height = -15.0f;
    param->rest_max_height = 20.0f;
    param->rest_min_height = -15.0f;
    param->height = -15.0f;
    param->ground_space = 25.0f;
    return 1;
}

/**
 * Reads a random integer scaled by the given limit.
 */
static int _GET_RND(RS_STACKDATA *args, int argc) {
    int limit;
    int value;

    if (argc != 2) {
        return 0;
    }
    limit = GetStackInt(args++);
    value = (int)(((float)limit * (float)rand()) / 2147483648.0f);
    SetStack(args, value);
    return 1;
}

/**
 * Reads a random whole-number float scaled by the given limit.
 */
static int _GET_RNDF(RS_STACKDATA *args, int argc) {
    float limit;

    if (argc != 2) {
        return 0;
    }
    limit = GetStackFloat(args++);
    SetStack(args, (float)(int)((limit * (float)rand()) / 2147483648.0f));
    return 1;
}

/**
 * Stores a monster script variable.
 */
static int _V_PUSH(RS_STACKDATA *args, int argc) {
    int index;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index < 0) {
        return 0;
    }
    if (args->type == RS_INT) {
        if (index < MONSTER_VAR_MAX) {
            nowMonster->var[index].i = GetStackInt(args);
        }
        if (index >= MONSTER_VAR_MAX && index < MONSTER_VAR_MAX + MONSTER_SHARE_MAX) {
            ActiveMonster->share_var[index - MONSTER_VAR_MAX].i = GetStackInt(args);
        }
        return 1;
    }
    if (args->type == RS_FLOAT) {
        if (index < MONSTER_VAR_MAX) {
            nowMonster->var[index].f = GetStackFloat(args);
        }
        if (index >= MONSTER_VAR_MAX && index < MONSTER_VAR_MAX + MONSTER_SHARE_MAX) {
            ActiveMonster->share_var[index - MONSTER_VAR_MAX].f = GetStackFloat(args);
        }
        return 1;
    }
    return 1;
}

/**
 * Reads a stored monster script variable.
 */
static int _V_POP(RS_STACKDATA *args, int argc) {
    int index;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index < 0) {
        return 0;
    }
    if (args->type != RS_PTR) {
        return 0;
    }
    if (args->p->type == RS_INT) {
        if (index < MONSTER_VAR_MAX) {
            SetStack(args, nowMonster->var[index].i);
        }
        if (index >= MONSTER_VAR_MAX && index < MONSTER_VAR_MAX + MONSTER_SHARE_MAX) {
            SetStack(args, ActiveMonster->share_var[index - MONSTER_VAR_MAX].i);
        }
        return 1;
    } else if (args->p->type == RS_FLOAT) {
        if (index < MONSTER_VAR_MAX) {
            SetStack(args, nowMonster->var[index].f);
        }
        if (index >= MONSTER_VAR_MAX && index < MONSTER_VAR_MAX + MONSTER_SHARE_MAX) {
            SetStack(args, ActiveMonster->share_var[index - MONSTER_VAR_MAX].f);
        }
        return 1;
    }
    return 1;
}

/**
 * Stores a monster script variable.
 */
static int _V_PUSH2(RS_STACKDATA *args, int argc) {
    int index;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index < 0) {
        return 0;
    }
    if (args->type == RS_INT) {
        if (index < MONSTER_VAR2_MAX) {
            nowMonster->var2[index].i = GetStackInt(args);
        } else {
            return 0;
        }
        return 1;
    } else if (args->type == RS_FLOAT) {
        if (index < MONSTER_VAR2_MAX) {
            nowMonster->var2[index].f = GetStackFloat(args);
        } else {
            return 0;
        }
        return 1;
    }
    return 1;
}

/**
 * Reads a stored monster script variable.
 */
static int _V_POP2(RS_STACKDATA *args, int argc) {
    int index;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index < 0) {
        return 0;
    }
    if (args->type != RS_PTR) {
        return 0;
    }
    if (args->p->type == RS_INT) {
        if (index < MONSTER_VAR2_MAX) {
            SetStack(args, nowMonster->var2[index].i);
        } else {
            return 0;
        }
        return 1;
    } else if (args->p->type == RS_FLOAT) {
        if (index < MONSTER_VAR2_MAX) {
            SetStack(args, nowMonster->var2[index].f);
        } else {
            return 0;
        }
        return 1;
    }
    return 1;
}

/**
 * Changes the dungeon's monster lock-on mode.
 */
static int _SET_LOCKON_MODE(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowScene->battle_area.unk_9e = GetStackInt(args);
    return 1;
}

/**
 * Reads the number of active monsters on the floor.
 */
static int _GET_MONSTER_NUM(RS_STACKDATA *args, int argc) {
    int count;

    if (argc != 1) {
        return 0;
    }
    count = ActiveMonster->GetMonsterNum(-1.0f);
    SetStack(args, count);
    return 1;
}

/**
 * Reads the distance from a point to the running monster.
 */
static int _GET_DIST(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR point;
    sceVu0FVECTOR position;

    if (argc != 4) {
        return 0;
    }
    point[0] = GetStackFloat(args++);
    point[1] = GetStackFloat(args++);
    point[2] = GetStackFloat(args++);
    nowMonster->GetPosition(position);
    SetStack(args, mgDistVector(point, position));
    return 1;
}

/**
 * Enters a named object of the running monster into a script object slot.
 */
static int _SET_OBJ(RS_STACKDATA *args, int argc) {
    int   index;
    char *name;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    name = GetStackString(args);
    return nowMonster->EntryObject(name, index) != NULL;
}

/**
 * Reports that the legacy body opcode is unavailable.
 */
static int _SET_BODY(RS_STACKDATA *args, int argc) {
    printf("NOT FOUND (_SET_BODY)\n");
    return 1;
}

/**
 * Reports that the legacy damage opcode is unavailable.
 */
static int _SET_DMG(RS_STACKDATA *args, int argc) {
    printf("NOT FOUND (_SET_DMG)\n");
    return 1;
}

/**
 * Enters damage between one or two numbered frames of the running monster.
 */
static int _SET_DMG2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR       position;
    CHARA_ENTRY_OBJECT *object;
    mgCFrame           *first;
    mgCFrame           *second;
    char               *damage;
    char               *motion;
    float               radius;
    float               start;
    float               end;
    int                 first_index;
    int                 second_index;

    damage = GetStackString(args++);
    radius = 2.0f * GetStackFloat(args++);
    motion = GetStackString(args++);
    start = GetStackFloat(args++);
    end = GetStackFloat(args++);
    first_index = GetStackInt(args++);
    second_index = -1;
    if (argc == 7) {
        second_index = GetStackInt(args);
    }
    second = NULL;
    object = nowMonster->GetEntryObjectPos(3, first_index, position);
    if (object == NULL) {
        return 0;
    }
    first = object->frame;
    if (radius <= 0.0f) {
        radius = object->unk_04;
    }
    if (second_index >= 0) {
        object = nowMonster->GetEntryObjectPos(3, second_index, position);
        if (object != NULL) {
            second = object->frame;
        }
    }
    LastCInfo2 = nowMonster->EntryDamage2(first, second, damage, radius, motion, start, end, NULL);
    return LastCInfo2 != NULL;
}

/**
 * Reads the world position of a named object on the monster or one of its parts.
 */
static int _GET_OBJ_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    char         *object_name;
    mgCFrame     *frame;
    CActionChara *chara;
    char         *chara_name;

    if (argc < 4 || argc > 5) {
        return 0;
    }
    object_name = GetStackString(args++);
    chara_name = NULL;
    if (argc == 5) {
        chara_name = GetStackString(args + 3);
    }
    if (argc == 5) {
        chara = nowMonster->SearchChara(chara_name);
        if (chara != NULL) {
            frame = chara->SearchObject(object_name);
        }
    } else {
        frame = nowMonster->SearchObject(object_name);
    }
    if (frame == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(position);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Reads the world position of a named frame of the linked map piece.
 */
static int _GET_MAPOBJ_POS(RS_STACKDATA *args, int argc) {
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR position;
    char         *name;
    mgCFrame     *frame;

    if (argc != 4) {
        return 0;
    }
    if (nowMonster->link_piece == NULL || nowMonster->link_parts == NULL) {
        return 0;
    }
    name = GetStackString(args++);
    nowMonster->link_piece->UpDatePosition();
    frame = nowMonster->link_piece->frame->SearchFrame(name);
    if (frame == NULL) {
        printf("ERR\n");
    }
    if (frame == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(position);
    nowMonster->link_parts->GetLWMatrix(matrix);
    sceVu0ApplyMatrix(position, matrix, position);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Makes a named map part follow the running monster.
 */
static int _LINK_MAP_TO_OBJECT(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    if (DngMainMap == NULL) {
        return 0;
    }
    nowMonster->link_parts = DngMainMap->GetPlaceParts(GetStackString(args));
    if (nowMonster->link_parts == NULL) {
        return 0;
    }
    nowMonster->link_type = MONSTER_LINK_PARTS;
    return 1;
}

/**
 * Links the running monster to a named piece of a map part.
 */
static int _LINK_OBJECT_TO_PIECE(RS_STACKDATA *args, int argc) {
    char *parts_name;
    char *piece_name;

    if (argc != 2) {
        return 0;
    }
    if (DngMainMap == NULL) {
        return 0;
    }
    parts_name = GetStackString(args++);
    piece_name = GetStackString(args);
    nowMonster->link_parts = DngMainMap->GetPlaceParts(parts_name);
    if (nowMonster->link_parts == NULL) {
        return 0;
    }
    nowMonster->link_piece = nowMonster->link_parts->SearchPiece(piece_name);
    if (nowMonster->link_piece == NULL) {
        return 0;
    }
    nowMonster->link_type = MONSTER_LINK_PIECE;
    return 1;
}

/**
 * Sets the running monster's photo scoop for all time or a motion interval.
 */
static int _SET_SCOOP(RS_STACKDATA *args, int argc) {
    if (argc != 1 && argc != 4) {
        return 0;
    }
    if (argc == 1) {
        nowMonster->scoop.type = MONSTER_SCOOP_ALWAYS;
        nowMonster->scoop.no = GetStackInt(args++);
    }
    if (argc == 4) {
        nowMonster->scoop.type = MONSTER_SCOOP_MOTION;
        nowMonster->scoop.motion = GetStackString(args++);
        nowMonster->scoop.start = GetStackFloat(args++);
        nowMonster->scoop.end = GetStackFloat(args++);
        nowMonster->scoop.no = GetStackInt(args);
    }
    return 1;
}

/**
 * Loads an image into one of the running monster's reserved image slots.
 */
static int _LOAD_RESERV_IMG(RS_STACKDATA *args, int argc) {
    int        size;
    int        index;
    mgCMemory *memory;
    void      *image;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index < 0 || index > 1) {
        return 0;
    }
    if (LoadFile2(GetStackString(args), BuffReadData, &size, LOAD_FILE_READ) == 0) {
        return 0;
    }
    memory = nowScene->GetStack(3);
    if (memory == NULL) {
        return 0;
    }
    image = memory->Alloc(size / 16 + 1);
    if (image == NULL) {
        return 0;
    }
    memcpy(image, BuffReadData, size);
    nowMonster->reserv_img[index] = image;
    nowMonster->reserv_img_size[index] = size;
    return 1;
}

/**
 * Limits the number of nearest monsters allowed to come into sight.
 */
static int _SET_PRIORITY_LIMMIT(RS_STACKDATA *args, int argc) {
    int limit;

    if (argc != 1) {
        return 0;
    }
    limit = GetStackInt(args);
    if (limit < 0 || limit >= MONSTER_ACTIVE_MAX) {
        return 0;
    }
    ActiveMonster->priority_limit = limit;
    return 1;
}

/**
 * Reads the position at which the running monster was placed.
 */
static int _GET_PLACE_POS(RS_STACKDATA *args, int argc) {
    if (argc != 3) {
        return 0;
    }
    SetStack(args++, nowMonster->place_pos[0]);
    SetStack(args++, nowMonster->place_pos[1]);
    SetStack(args, nowMonster->place_pos[2]);
    return 1;
}

/**
 * Changes the position at which the running monster was placed.
 */
static int _SET_PLACE_POS(RS_STACKDATA *args, int argc) {
    if (argc != 3) {
        return 0;
    }
    nowMonster->place_pos[0] = GetStackFloat(args++);
    nowMonster->place_pos[1] = GetStackFloat(args++);
    nowMonster->place_pos[2] = GetStackFloat(args);
    return 1;
}

/**
 * Reads how far a ray at an offset from the monster's facing reaches before hitting the map.
 */
static int _SEARCH_AREA(RS_STACKDATA *args, int argc) {
    static ScriptVector initial_end = { { 0.0f, 0.0f, 1.0f, 0.0f } };
    ScriptVector end;
    sceVu0FVECTOR start;
    sceVu0FVECTOR rotation;
    sceVu0FMATRIX matrix;
    float         distance;
    float         angle;

    if (argc != 3) {
        return 0;
    }
    distance = GetStackFloat(args++);
    angle = GetStackFloat(args++);
    end = initial_end;
    nowMonster->GetPosition(start);
    nowMonster->GetRotation(rotation);
    rotation[1] = mgAngleLimit(rotation[1] + angle);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, rotation[1]);
    sceVu0ApplyMatrix(end.values, matrix, end.values);
    sceVu0ScaleVector(end.values, end.values, distance);
    sceVu0AddVector(end.values, end.values, start);
    start[1] += 100.0f;
    end.values[1] += 100.0f;
    SetStack(args, SearchArea(nowScene, start, end.values, distance));
    return 1;
}

/**
 * Reports the map distance along a line between two script positions.
 */
static int _SEARCH_AREA2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    float         distance;

    if (argc != 7) {
        return 0;
    }
    GetStackVector(from, &args);
    GetStackVector(to, &args);
    distance = SearchArea(nowScene, from, to, mgDistVector(from, to));
    SetStack(args++, distance);
    return 1;
}

/**
 * Enables or disables scene lighting on the monster's model or a named frame.
 */
static int _SET_MODEL_LIGHT_SWITCH(RS_STACKDATA *args, int argc) {
    int           enabled;
    char         *name;
    mgCFrame     *frame;
    mgCFrameAttr *attr;

    if (argc < 1 || argc > 2) {
        return 0;
    }
    enabled = GetStackInt(args++);
    name = NULL;
    if (argc == 2) {
        name = GetStackString(args);
    }
    frame = nowMonster->CObjectFrame::frame;
    if (name != NULL) {
        frame = frame->SearchFrame(name);
    }
    if (frame == NULL) {
        return 0;
    }
    attr = frame->attr;
    if (enabled != 0) {
        attr->no_light = 0;
        frame->SetAttrParam(*attr, 1, MG_FRAME_ATTR_NO_LIGHT);
    } else {
        attr->no_light = 1;
        attr->color[0] = 128.0f;
        attr->color[1] = 128.0f;
        attr->color[2] = 128.0f;
        attr->color[3] = 128.0f;
        frame->SetAttrParam(*attr, 1, MG_FRAME_ATTR_NO_LIGHT | MG_FRAME_ATTR_COLOR);
    }
    return 1;
}

/**
 * Sets the unlit colour of the monster's model or a named frame.
 */
static int _SET_MODEL_LIGHT_COLOR(RS_STACKDATA *args, int argc) {
    float         red;
    float         green;
    float         blue;
    float         alpha;
    char         *name;
    mgCFrame     *frame;
    mgCFrameAttr *attr;

    if (argc < 4 || argc > 5) {
        return 0;
    }
    name = NULL;
    red = GetStackFloat(args++);
    green = GetStackFloat(args++);
    blue = GetStackFloat(args++);
    alpha = GetStackFloat(args++);
    if (argc == 5) {
        name = GetStackString(args);
    }
    frame = nowMonster->CObjectFrame::frame;
    if (name != NULL) {
        frame = frame->SearchFrame(name);
    }
    if (frame == NULL) {
        return 0;
    }
    attr = frame->attr;
    attr->no_light = 1;
    attr->color[0] = red;
    attr->color[1] = green;
    attr->color[2] = blue;
    attr->color[3] = alpha;
    frame->SetAttrParam(*attr, 1, MG_FRAME_ATTR_COLOR);
    return 1;
}

/**
 * Scales the monster's defence from its table value.
 */
static int _SET_DEF_RATE(RS_STACKDATA *args, int argc) {
    float rate;

    if (argc != 1) {
        return 0;
    }
    rate = GetStackFloat(args);
    nowMonster->defense = (unsigned int)(nowMonster->tbl->defense * rate);
    return 1;
}

/**
 * Writes the running monster's world position to the script.
 */
static int _GET_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;

    if (argc != 3) {
        return 0;
    }
    nowMonster->GetPosition(position);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Places the running monster at the script's world position.
 */
static int _SET_POS(RS_STACKDATA *args, int argc) {
    float x;
    float y;
    float z;

    if (argc != 3) {
        return 0;
    }
    x = GetStackFloat(args++);
    y = GetStackFloat(args++);
    z = GetStackFloat(args);
    nowMonster->SetPosition(x, y, z);
    return 1;
}

/**
 * Writes the rotation of the running monster or a numbered monster to the script.
 */
static int _GET_ROT(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR rotation;
    CCharacter2  *chara;
    int           index;

    if (argc < 3 || argc > 4) {
        return 0;
    }
    if (argc == 4) {
        index = GetStackInt(args++);
        chara = nowScene->GetCharacter(index + MONSTER_ACTIVE_MAX);
        if (chara == NULL) {
            return 0;
        }
        chara->GetRotation(rotation);
    } else {
        nowMonster->GetRotation(rotation);
    }
    SetStack(args++, rotation[0]);
    SetStack(args++, rotation[1]);
    SetStack(args, rotation[2]);
    return 1;
}

/**
 * Sets the monster's rotation and stops its scripted turn.
 */
static int _SET_ROT(RS_STACKDATA *args, int argc) {
    float x;
    float y;
    float z;

    if (argc != 3) {
        return 0;
    }
    x = GetStackFloat(args++);
    y = GetStackFloat(args++);
    z = GetStackFloat(args);
    nowMonster->SetRotation(x, y, z);
    nowMonster->rot_speed = 0.0f;
    return 1;
}

/**
 * Sets the angle and speed of the monster's next turn.
 */
static int _SET_NEXT_ROT(RS_STACKDATA *args, int argc) {
    if (argc != 2) {
        return 0;
    }
    nowMonster->next_rot = GetStackFloat(args++);
    nowMonster->rot_speed = GetStackFloat(args);
    return 1;
}

/**
 * Sets the monster's next walk position, speed and arrival distance.
 */
static int _SET_NEXT_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR next_position;
    sceVu0FVECTOR position;

    if (argc < 3 || argc > 5) {
        return 0;
    }
    next_position[0] = GetStackFloat(args++);
    next_position[1] = GetStackFloat(args++);
    next_position[2] = GetStackFloat(args++);
    nowMonster->next_pos[0] = next_position[0];
    nowMonster->next_pos[1] = next_position[1];
    nowMonster->next_pos[2] = next_position[2];
    nowMonster->move_speed = GetStackFloat(args++);
    nowMonster->arrive_dist = 20.0f;
    if (argc >= 5) {
        nowMonster->arrive_dist = GetStackFloat(args);
    }
    nowMonster->GetPosition(position);
    if (mgDistVector(next_position, position) < nowMonster->arrive_dist) {
        nowMonster->move_speed = 0.0f;
    }
    return 1;
}

/**
 * Reports whether the monster has reached the arrival distance of its next position.
 */
static int _CHK_MOVE_END(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    int           arrived;

    if (argc != 1) {
        return 0;
    }
    arrived = 0;
    nowMonster->GetPosition(position);
    if (mgDistVector(position, nowMonster->next_pos) < nowMonster->arrive_dist) {
        arrived = 1;
    }
    SetStack(args, arrived);
    return 1;
}

s32 _RESET_MOVE(RS_STACKDATA *stack, s32 argument_count) {
    nowMonster->move_speed = 0.0f;
    return 1;
}

/**
 * Writes the target's position and optionally its distance from the monster.
 */
static int _GET_TARGET_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR position;
    CCharacter2  *target;

    if (argc < 3 || argc > 4) {
        return 0;
    }
    target = nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetPosition(target_position);
    SetStack(args++, target_position[0]);
    SetStack(args++, target_position[1]);
    SetStack(args++, target_position[2]);
    if (argc == 4) {
        nowMonster->GetPosition(position);
        SetStack(args, mgDistVector(position, target_position));
    }
    return 1;
}

/**
 * Writes the distance between the monster and its target.
 */
static int _GET_TARGET_DIST(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR position;
    CCharacter2  *target;

    if (argc != 1) {
        return 0;
    }
    target = nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetPosition(target_position);
    nowMonster->GetPosition(position);
    SetStack(args, mgDistVector(target_position, position));
    return 1;
}

/**
 * Writes the world angle from the monster to its target.
 */
static int _GET_TARGET_ANGLE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR position;
    CCharacter2  *target;

    if (argc != 1) {
        return 0;
    }
    target = nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetPosition(target_position);
    nowMonster->GetPosition(position);
    sceVu0SubVector(target_position, target_position, position);
    SetStack(args, atan2f(target_position[0], target_position[2]));
    return 1;
}

/**
 * Writes a position offset from the target by a world angle and distance.
 */
static int _GET_TARGET_REF_POS(RS_STACKDATA *args, int argc) {
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR position;
    sceVu0FVECTOR direction;
    float         angle;
    float         distance;
    CCharacter2  *target;

    if (argc != 5) {
        return 0;
    }
    angle = GetStackFloat(args++);
    distance = GetStackFloat(args++);
    target = nowScene->GetCharacter(nowMonster->target_no);
    if (target == NULL) {
        return 0;
    }
    target->GetPosition(position);
    direction[0] = 0.0f;
    direction[1] = 0.0f;
    direction[2] = 1.0f;
    sceVu0Normalize(direction, direction);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, angle);
    sceVu0ApplyMatrix(direction, matrix, direction);
    sceVu0ScaleVector(direction, direction, distance);
    sceVu0AddVector(position, position, direction);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

#ifdef NONMATCHING
/**
 * Classifies a position's direction relative to the monster's facing.
 */
static int _GET_REF_DIR(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR position;
    sceVu0FVECTOR front;
    float         distance;
    float         front_angle;
    float         angle;
    int           direction;

    if (argc < 4 || argc > 5) {
        return 0;
    }
    target_position[0] = GetStackFloat(args++);
    target_position[1] = GetStackFloat(args++);
    target_position[2] = GetStackFloat(args++);
    target_position[3] = 1.0f;
    distance = GetStackFloat(args++);
    nowMonster->GetPosition(position);
    if (mgDistVector(position, target_position) < distance) {
        SetStack(args++, 0);
    }
    sceVu0SubVector(target_position, target_position, position);
    sceVu0Normalize(target_position, target_position);
    sceVu0CopyVector(front, nowMonster->front_vec);
    front_angle = atan2f(front[0], front[2]);
    angle = atan2f(target_position[0], target_position[2]) - front_angle;
    if (angle < -3.1415927f) {
        angle += 6.2831855f;
    }
    direction = 0;
    if (angle > -0.8f && angle < 0.8f) {
        direction = 1;
    }
    if (angle < -2.2f || angle > 2.2f) {
        direction = 2;
    }
    if (angle < -0.8f && angle > -3.0f) {
        direction = 3;
    }
    if (angle > 0.8f && angle < 3.0f) {
        direction = 4;
    }
    SetStack(args, direction);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _GET_REF_DIR__FP12RS_STACKDATAi);
#endif

/**
 * Writes a position offset from the monster's facing by an angle and distance.
 */
static int _GET_REFANGLE_POS(RS_STACKDATA *args, int argc) {
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR position;
    sceVu0FVECTOR direction;
    float         angle;
    float         distance;

    if (argc != 5) {
        return 0;
    }
    angle = GetStackFloat(args++);
    distance = GetStackFloat(args++);
    nowMonster->GetPosition(position);
    sceVu0CopyVector(direction, nowMonster->front_vec);
    sceVu0Normalize(direction, direction);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, angle);
    sceVu0ApplyMatrix(direction, matrix, direction);
    sceVu0ScaleVector(direction, direction, distance);
    sceVu0AddVector(position, position, direction);
    SetStack(args++, position[0]);
    SetStack(args++, position[1]);
    SetStack(args, position[2]);
    return 1;
}

/**
 * Writes the world angle from the monster to a script position.
 */
static int _GET_REF_ANGLE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR target_position;

    if (argc != 4) {
        return 0;
    }
    target_position[0] = GetStackFloat(args++);
    target_position[1] = GetStackFloat(args++);
    target_position[2] = GetStackFloat(args++);
    target_position[3] = 1.0f;
    nowMonster->GetPosition(position);
    sceVu0SubVector(target_position, target_position, position);
    SetStack(args, atan2f(target_position[0], target_position[2]));
    return 1;
}

/**
 * Writes the monster's height above the ground, or zero when landed.
 */
static int _GET_HIGH(RS_STACKDATA *args, int argc) {
    float height;

    if (argc != 1) {
        return 0;
    }
    if (nowMonster->mons_move_check.landed != 0) {
        height = 0.0f;
    } else {
        height = nowMonster->height;
    }
    SetStack(args, height);
    return 1;
}

/**
 * Writes a scene monster's position and optionally its distance from the running monster.
 */
static int _GET_ACTIVE_MONS_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   target_position;
    sceVu0FVECTOR   position;
    CActiveMonster *monster;
    int             chara_no;

    if (argc < 4 || argc > 5) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    chara_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[chara_no];
    if (monster == NULL) {
        return 0;
    }
    monster->GetPosition(target_position);
    SetStack(args++, target_position[0]);
    SetStack(args++, target_position[1]);
    SetStack(args++, target_position[2]);
    if (argc == 5) {
        nowMonster->GetPosition(position);
        SetStack(args, mgDistVector(target_position, position));
    }
    return 1;
}

/**
 * Writes a scene monster's rotation to the script.
 */
static int _GET_ACTIVE_MONS_ROT(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   rotation;
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 4) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    chara_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[chara_no];
    if (monster == NULL) {
        return 0;
    }
    monster->GetRotation(rotation);
    SetStack(args++, rotation[0]);
    SetStack(args++, rotation[1]);
    SetStack(args, rotation[2]);
    return 1;
}

/**
 * Writes the distance from the running monster to a scene monster.
 */
static int _GET_ACTIVE_MONS_DIST(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   position;
    sceVu0FVECTOR   target_position;
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    chara_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[chara_no];
    if (monster == NULL) {
        return 0;
    }
    nowMonster->GetPosition(position);
    monster->GetPosition(target_position);
    SetStack(args, mgDistVector(target_position, position));
    return 1;
}

/**
 * Writes the yaw of a scene monster to the script.
 */
static int _GET_ACTIVE_MONS_ANGLE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   rotation;
    CActiveMonster *monster;
    int             chara_no;

    if (argc != 2) {
        return 0;
    }
    chara_no = GetStackInt(args++);
    chara_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[chara_no];
    if (monster == NULL) {
        return 0;
    }
    monster->GetRotation(rotation);
    SetStack(args, rotation[1]);
    return 1;
}

/**
 * Writes the pitch and yaw that face a script position from the monster.
 */
static int _GET_REF_ROT(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR direction;
    sceVu0FVECTOR rotation;

    if (argc != 6) {
        return 0;
    }
    GetStackVector(direction, &args);
    nowMonster->GetPosition(position);
    sceVu0SubVector(direction, direction, position);
    sceVu0Normalize(direction, direction);
    rotation[1] = atan2f(direction[0], direction[2]);
    rotation[0] = -atan2f(direction[1], sqrtf(direction[0] * direction[0] + direction[2] * direction[2]));
    rotation[2] = 0.0f;
    SetStackVector(rotation, &args);
    return 1;
}

/**
 * Writes the yaw or normalised direction between two script positions.
 */
static int _GET_REF_ROT2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR direction;
    float         yaw;

    if (argc != 7 && argc != 9) {
        return 0;
    }
    GetStackVector(direction, &args);
    GetStackVector(position, &args);
    sceVu0SubVector(direction, direction, position);
    sceVu0Normalize(direction, direction);
    yaw = atan2f(direction[0], direction[2]);
    atan2f(direction[1], sqrtf(direction[0] * direction[0] + direction[2] * direction[2]));
    if (argc == 7) {
        SetStack(args++, yaw);
    }
    if (argc == 9) {
        SetStackVector(direction, &args);
    }
    return 1;
}

/**
 * Reports the map distance from the monster along a horizontal angle.
 */
static int _FLYING_SEARCH_AREA(RS_STACKDATA *args, int argc) {
    static ScriptVector initial_direction = { { 0.0f, 0.0f, 1.0f, 0.0f } };
    ScriptVector direction;
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FMATRIX matrix;
    float         distance;
    float         angle;

    if (argc != 3) {
        return 0;
    }
    distance = GetStackFloat(args++);
    angle = GetStackFloat(args++);
    direction = initial_direction;
    nowMonster->GetPosition(position);
    nowMonster->GetRotation(rotation);
    rotation[1] = mgAngleLimit(angle);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, rotation[1]);
    sceVu0ApplyMatrix(direction.values, matrix, direction.values);
    sceVu0ScaleVector(direction.values, direction.values, distance);
    sceVu0AddVector(direction.values, direction.values, position);
    SetStack(args, SearchArea(nowScene, position, direction.values, distance));
    return 1;
}

#ifdef NONMATCHING
/**
 * Writes the height above the nearest map polygon below a script position.
 */
static int _GET_HIGH2(RS_STACKDATA *args, int argc) {
    CCPoly        polygons[128];
    sceVu0FVECTOR position;
    sceVu0FVECTOR hit;
    mgVu0FBOX     box;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    CMap         *map;
    int           count;
    float         height;

    if (argc != 4) {
        return 0;
    }
    GetStackVector(position, &args);
    map = nowScene->GetMap(nowScene->active_map);
    if (map == NULL) {
        return 0;
    }
    box.max[0] = 40.0f + position[0];
    box.min[0] = position[0] - 40.0f;
    box.max[2] = 40.0f + position[2];
    box.min[2] = position[2] - 40.0f;
    box.max[1] = 300.0f + position[1];
    box.min[1] = position[1] - 300.0f;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    count = map->GetColPoly(polygons, box, 128);
    sceVu0CopyVector(from, position);
    sceVu0CopyVector(to, position);
    from[1] += 1.0f;
    to[1] -= 300.0f;
    if (CheckHit(polygons, count, from, to, hit, 1, 2) >= 0) {
        height = position[1] - hit[1];
    } else {
        height = -3.4028235e38f;
    }
    SetStack(args++, height);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _GET_HIGH2__FP12RS_STACKDATAi);
#endif

/**
 * Writes the scene number of a ranked monster within a script distance.
 */
static int _GET_RANGE_MONS_ID(RS_STACKDATA *args, int argc) {
    /**
     * Pairs a nearby monster's distance with its scene number.
     */
    struct MONSTER_RANGE_ENTRY {
        float distance; /**< Distance from the running monster. */
        int   no;       /**< Scene number of the monster. */
    };
    MONSTER_RANGE_ENTRY nearby[MONSTER_ACTIVE_MAX];
    sceVu0FVECTOR       position;
    sceVu0FVECTOR       target_position;
    int                 i;
    int                 monster_index;
    int                 j;
    int                 count;
    MONSTER_RANGE_ENTRY *entry;
    float               range;
    int                 rank;
    CActionChara       *monster;
    float               distance;
    float               swap_distance;
    int                 swap_no;

    entry = nearby;
    do {
        entry->distance = -1.0f;
        entry->no = -1;
        entry++;
    } while (entry < nearby + MONSTER_ACTIVE_MAX);
    range = GetStackFloat(args++);
    rank = GetStackInt(args++);
    nowMonster->GetPosition(position);
    mgZeroVector(target_position);
    for (monster_index = 0, count = 0; monster_index < MONSTER_ACTIVE_MAX; monster_index++) {
        if ((monster = (CActionChara *)nowScene->GetCharacter(
                 monster_index + MONSTER_ACTIVE_MAX)) != NULL &&
            monster->chara_kind == ACTION_KIND_SCRIPT &&
            nowMonster->chara_type != monster->chara_type) {
            monster->GetPosition(target_position);
            distance = mgDistVector(position, target_position);
            if (distance <= range) {
                nearby[count].distance = distance;
                nearby[count].no = monster->chara_type;
                count++;
            }
        }
    }
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (nearby[i].distance > nearby[j].distance) {
                swap_distance = nearby[i].distance;
                swap_no = nearby[i].no;
                nearby[i].distance = nearby[j].distance;
                nearby[i].no = nearby[j].no;
                nearby[j].distance = swap_distance;
                nearby[j].no = swap_no;
            }
        }
    }
    SetStack(args, nearby[rank].no);
    return 1;
}

/**
 * Writes an entered object's position from the running monster or a scene monster.
 */
static int _GET_ENTRY_OBJ_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   position;
    CActiveMonster *monster;
    int             no;
    int             object;

    if (argc != 5) {
        return 0;
    }
    no = GetStackInt(args++);
    object = GetStackInt(args++);
    if (no != -1) {
        no -= MONSTER_ACTIVE_MAX;
        monster = ActiveMonster->active[no];
        if (monster == NULL) {
            return 0;
        }
    } else {
        monster = nowMonster;
    }
    monster->GetEntryObjectPos(object, position);
    SetStackVector(position, &args);
    return 1;
}

/**
 * Starts the monster's knock-back with the script's speed, deceleration and duration.
 */
static int _BLOW_START(RS_STACKDATA *args, int argc) {
    if (argc != 3) {
        return 0;
    }
    nowMonster->blow_speed = GetStackFloat(args++);
    nowMonster->blow_speed *= nowMonster->blow_rate;
    nowMonster->blow_decel = GetStackFloat(args++);
    nowMonster->blow_time = GetStackInt(args);
    return 1;
}

/**
 * Sets the action status that other scripts read from the monster.
 */
static int _SET_ACT_STATUS(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowMonster->now_status = GetStackInt(args);
    return 1;
}

/**
 * Sets or clears the monster's script mask flags.
 */
static int _SET_INT_FLAG(RS_STACKDATA *args, int argc) {
    int mask;
    int enabled;

    if (argc != 2) {
        return 0;
    }
    mask = GetStackInt(args++);
    enabled = GetStackInt(args);
    nowMonster->SetMaskFlag(mask, enabled);
    return 1;
}

#ifdef NONMATCHING
/**
 * Starts the monster's death and scatters its money, badge and item drops.
 */
static int _SET_DEAD_START(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR     position;
    sceVu0FVECTOR     velocity;
    CBattleCharaInfo *battle;
    CPalletAnime     *pallet;
    CPullItem        *item;
    BASE_MONSTER_TBL *table;
    float             money_rate;
    int               total;
    int               large_coin;
    int               small_coin;
    int               roll;
    int               drop_index;
    int               drop_second;
    int               i;

    if (argc != 0) {
        return 0;
    }
    nowMonster->state = ACTIVE_MONSTER_DEAD;
    pallet = &nowMonster->unk_67c;
    pallet->duration = 0;
    pallet->elapsed = 0;
    if (nowMonster->reward_money <= 0) {
        return 1;
    }
    nowMonster->GetEntryObjectPos(0, position);
    money_rate = 1.0f;
    total = nowMonster->reward_money;
    battle = GetBattleCharaInfo();
    if (battle->GetNowNPC() == 17) {
        money_rate += 0.3f;
    }
    if (nowMonster->last_hit_attr & 0x1) {
        money_rate += 0.3f;
    }
    if (nowMonster->last_hit_attr & 0x2) {
        money_rate -= 0.3f;
    }
    total = (int)(total * money_rate);
    large_coin = (int)(0.7f * total);
    large_coin += (total - large_coin) % 5;
    if (large_coin > 0) {
        item = PullItemMan.GetList(2);
        if (item != NULL) {
            velocity[0] = 0.0f;
            velocity[1] = 4.0f;
            velocity[2] = 0.0f;
            velocity[3] = 1.0f;
            item->SetItem(position, velocity, PULL_ITEM_MONEY_LARGE);
            item->item_no = large_coin;
        }
    }
    printf("total = %d (%d)\n", total, large_coin);
    small_coin = (total - large_coin) / 5;
    printf("num = %d (%d)\n", 5, small_coin);
    for (i = 0; i < 5; i++) {
        item = PullItemMan.GetList(2);
        if (item != NULL) {
            velocity[0] = 0.3f + fRand(0.6f);
            velocity[1] = 2.0f + fRand(3.0f);
            velocity[2] = 0.3f + fRand(0.6f);
            if (iRand(100) < 50) {
                velocity[0] *= -1.0f;
            }
            if (iRand(100) < 50) {
                velocity[2] *= -1.0f;
            }
            velocity[3] = 1.0f;
            item->SetItem(position, velocity, PULL_ITEM_MONEY);
            item->item_no = small_coin;
        }
    }
    if (nowMonster->drop_badge != 0) {
        item = PullItemMan.GetList(1);
        if (item != NULL) {
            velocity[0] = fRand(0.5f) - 0.25f;
            velocity[2] = fRand(0.5f) - 0.25f;
            velocity[1] = 4.0f;
            velocity[3] = 1.0f;
            item->SetItem(position, velocity, PULL_ITEM_BADGE);
            item->item_no = nowMonster->tbl->user_mons_id;
        }
    }
    roll = iRand(100);
    if (roll % 6 == 0) {
        table = nowMonster->tbl;
        if (table->drop_item[0] > 0 || table->drop_item[1] > 0) {
            drop_index = 0;
            if (roll < 20 && table->drop_item[1] > 0) {
                drop_index = 1;
            }
            if (table->drop_item[0] <= 0) {
                drop_index = 1;
            }
            item = PullItemMan.GetList(2);
            if (item != NULL) {
                velocity[0] = fRand(0.5f) - 0.25f;
                velocity[2] = fRand(0.5f) - 0.25f;
                velocity[1] = 4.0f;
                velocity[3] = 1.0f;
                item->SetItem(position, velocity, PULL_ITEM_ITEM);
                item->item_no = nowMonster->tbl->drop_item[drop_index];
            }
        }
    }
    drop_second = 0;
    if (iRand(100) % 4 == 0) {
        drop_second = 1;
    }
    if (battle->GetNowNPC() == 2) {
        drop_second = 1;
    }
    if (drop_second != 0 && nowMonster->tbl->drop_item[2] > 0) {
        item = PullItemMan.GetList(2);
        if (item != NULL) {
            velocity[0] = fRand(1.0f) - 0.5f;
            velocity[2] = fRand(1.0f) - 0.5f;
            velocity[1] = 5.0f;
            velocity[3] = 1.0f;
            item->SetItem(position, velocity, PULL_ITEM_ITEM2);
            item->item_no = nowMonster->tbl->drop_item[2];
        }
    }
    sndSePlay(nowScene->se_battle_id, 1, 0);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SET_DEAD_START__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Starts the monster's death fade, death clouds and weapon-growth pickups.
 */
static int _SET_DEAD_OFF(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR velocity;
    CDeadEffect  *effect;
    CPullItem    *item;
    float         radius;
    float         height;
    float         size;
    float         growth;
    int           last_chara;
    int           last_source;
    int           experience;
    int           pickup_count;
    int           i;

    if (argc != 0) {
        return 0;
    }
    nowMonster->dead_alpha = 128;
    height = 2.0f * nowMonster->body_height;
    radius = 3.0f * nowMonster->body_width;
    if (height >= 60.0f) {
        height = 60.0f;
    }
    size = height / 32.0f;
    nowMonster->GetEntryObjectPos(0, position);
    if ((nowMonster->attrib & MONSTER_ATTRIB_UNK_2) == 0) {
        if (BattleFX.dead == NULL) {
            effect = NULL;
        } else {
            effect = &BattleFX.dead[BattleFX.dead_next++];
            if (BattleFX.dead_next >= BattleFX.dead_num) {
                BattleFX.dead_next = 0;
            }
        }
        if (effect != NULL) {
            effect->SetDeadEffect(position, radius, height, size, 35);
        }
        if (BattleFX.dead == NULL) {
            effect = NULL;
        } else {
            effect = &BattleFX.dead[BattleFX.dead_next++];
            if (BattleFX.dead_next >= BattleFX.dead_num) {
                BattleFX.dead_next = 0;
            }
        }
        if (effect != NULL) {
            effect->SetDeadEffect(position, 0.5f * radius, 0.5f * height, size, 35);
        }
    }
    last_chara = nowMonster->last_hit_chara;
    last_source = nowMonster->last_hit_source;
    experience = nowMonster->reward_exp;
    pickup_count = 0;
    if (nowMonster->last_hit_attr & 0x800) {
        experience = (int)(experience * 1.2f);
    }
    if (experience < 6 && experience > 0) {
        pickup_count = 6;
    }
    if (experience >= 6) {
        pickup_count = 8;
    }
    if (experience >= 50) {
        pickup_count = 10;
    }
    if (experience >= 200) {
        pickup_count = 12;
    }
    if (experience >= 500) {
        pickup_count = 16;
    }
    growth = (float)experience / (float)pickup_count;
    for (i = 0; i < pickup_count; i++) {
        item = PullItemMan.GetList(2);
        if (item != NULL) {
            velocity[0] = 0.3f + fRand(0.6f);
            velocity[1] = 2.0f + fRand(3.0f);
            velocity[2] = 0.3f + fRand(0.6f);
            if (iRand(100) < 50) {
                velocity[0] *= -1.0f;
            }
            if (iRand(100) < 50) {
                velocity[2] *= -1.0f;
            }
            velocity[3] = 1.0f;
            item->SetItem(position, velocity, PULL_ITEM_WEAPON_EXP);
            item->exp = growth;
            item->exp_param = last_chara;
            item->item_no = last_source;
        }
    }
    if (pickup_count > 0) {
        sndSePlay(nowScene->se_battle_id, 2, 0);
    }
    sndSePlay(nowScene->se_battle_id, 20, 0);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SET_DEAD_OFF__FP12RS_STACKDATAi);
#endif

s32 _SET_SHROW_END(RS_STACKDATA *stack, s32 argument_count) {
    if (argument_count != 0) return 0;
    nowMonster->catch_state = 0;
    return 1;
}

/**
 * Starts a named motion on the monster with optional speed and playback flags.
 */
static int _SET_MOS(RS_STACKDATA *args, int argc) {
    char *name;
    float speed;
    int   flags;

    speed = -1.0f;
    name = NULL;
    flags = 0;
    if (argc < 1 || argc > 3) {
        return 0;
    }
    if (argc > 0) {
        name = GetStackString(args++);
    }
    if (argc >= 2) {
        speed = GetStackFloat(args++);
    }
    if (argc == 3) {
        flags = GetStackInt(args);
    }
    if (name == NULL) {
        return 0;
    }
    nowMonster->SetMotion(name, flags, 1);
    if (speed > 0.0f) {
        nowMonster->SetStep(speed);
    }
    return 1;
}

/**
 * Reports whether the monster's motion or a named chained part's motion has ended.
 */
static int _CHECK_MOS_END(RS_STACKDATA *args, int argc) {
    char *name;
    int   ended;

    ended = argc;
    if (argc == 1) {
        ended = nowMonster->CheckMotionEnd(NULL);
    }
    if (argc == 2) {
        name = GetStackString(args + 1);
        if (name == NULL) {
            return 0;
        }
        ended = nowMonster->CheckMotionEnd(name);
    }
    SetStack(args, ended);
    return 1;
}

/**
 * Writes the motion wait of the monster or a named chained part.
 */
static int _NOW_MOS_WAIT(RS_STACKDATA *args, int argc) {
    char *name;
    float wait;

    if (argc == 1) {
        wait = nowMonster->GetNowFrameWait(NULL);
    }
    if (argc == 2) {
        name = GetStackString(args + 1);
        if (name == NULL) {
            return 0;
        }
        wait = nowMonster->GetNowFrameWait(name);
    }
    SetStack(args, wait);
    return 1;
}

/**
 * Writes the motion status of the monster or a named chained part.
 */
static int _GET_MOS_STATUS(RS_STACKDATA *args, int argc) {
    char *name;
    int   status;

    status = argc;
    if (argc == 1) {
        status = nowMonster->GetMotionStatus(NULL);
    }
    if (argc == 2) {
        name = GetStackString(args + 1);
        if (name == NULL) {
            return 0;
        }
        status = nowMonster->GetMotionStatus(name);
    }
    SetStack(args, status);
    return 1;
}

/**
 * Sets the duration for which the monster takes no damage.
 */
static int _SET_MUTEKI(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 1;
    }
    nowMonster->muteki_time = GetStackInt(args);
    return 1;
}

/**
 * Writes the monster's remaining hits before rage.
 */
static int _GET_GEKIRIN(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, nowMonster->gekirin);
    return 1;
}

/**
 * Writes the player's transformation number when it is this monster's kind.
 */
static int _GET_USER_MONS_ID(RS_STACKDATA *args, int argc) {
    int monster_id;

    if (argc != 1) {
        return 0;
    }
    monster_id = -1;
    if (DngUserData->active_chr_no == USER_CHARA_MONSTER) {
        monster_id = GetBattleCharaInfo()->unk_2;
        if (nowMonster->tbl->user_mons_id != monster_id) {
            monster_id = -1;
        }
    }
    SetStack(args, monster_id);
    return 1;
}

/**
 * Writes the monster's rank by distance to its target.
 */
static int _GET_PRIORITY(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, (int)nowMonster->priority);
    return 1;
}

#ifdef NONMATCHING
/**
 * Places a monster of a loaded kind with optional position and rotation.
 */
static int _CREATE_MONSTER(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR   position;
    sceVu0FVECTOR   rotation;
    CActiveMonster *monster;
    int             id;
    int             index;

    mgZeroVector(position);
    position[3] = 1.0f;
    mgZeroVector(rotation);
    rotation[3] = 1.0f;
    id = GetStackInt(args++);
    if (argc >= 2) {
        position[0] = GetStackFloat(args++);
        position[1] = GetStackFloat(args++);
        position[2] = GetStackFloat(args++);
    }
    if (argc == 5) {
        rotation[1] = GetStackFloat(args);
    } else if (argc == 7) {
        rotation[0] = GetStackFloat(args++);
        rotation[1] = GetStackFloat(args++);
        rotation[2] = GetStackFloat(args);
    }
    index = ActiveMonster->SearchBaseIndex(id);
    if (index < 0) {
        return 0;
    }
    monster = ActiveMonster->SetActiveMonster(index, position, rotation, -1);
    printf("mons_index = %d\n", index);
    return monster != NULL;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _CREATE_MONSTER__FP12RS_STACKDATAi);
#endif

/**
 * Sets the distance within which the monster is visible in units of twenty.
 */
static int _SET_CLIP_DIST(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowMonster->clip_dist = 20.0f * GetStackFloat(args);
    return 1;
}

s32 _SET_COLLISION(RS_STACKDATA *stack, int argc) {
    return 0;
}
s32 _SET_GRAVITY(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Sets or clears the monster's behaviour flags.
 */
static int _SET_ATTRIB(RS_STACKDATA *args, int argc) {
    int flags;

    if (argc != 2) {
        return 0;
    }
    flags = GetStackInt(args++);
    if (GetStackInt(args) != 0) {
        nowMonster->attrib |= flags;
    } else {
        nowMonster->attrib &= ~flags;
    }
    return 1;
}

/**
 * Writes the monster's scale to three script variables.
 */
static int _GET_SCALE(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR scale;

    if (argc != 3) {
        return 0;
    }
    nowMonster->GetScale(scale);
    SetStackVector(scale, &args);
    return 1;
}

/**
 * Writes the monster's movement radius, using the default for a nonpositive radius.
 */
static int _GET_MONS_WIDTH(RS_STACKDATA *args, int argc) {
    float radius;

    if (argc != 1 || nowMonster == NULL) {
        return 0;
    }
    radius = nowMonster->mons_move_check.radius;
    SetStack(args, (float)(radius <= 0.0 ? 15.0 : (double)radius));
    return 1;
}

/**
 * Starts a named effect for the monster and optionally returns its slot.
 */
static int _ESM_CREATE(RS_STACKDATA *args, int argc) {
    char *name;
    int   user_id;
    int   slot;

    name = GetStackString(args++);
    user_id = nowMonster->chara_type;
    switch (argc) {
        case 1:
            ActiveMonster->effect_man->CreateEffSpt(name, user_id, 0);
            break;
        case 2:
            slot = ActiveMonster->effect_man->CreateEffSpt(name, user_id, 1);
            if (slot <= -1) {
                return 0;
            }
            SetStack(args, slot);
            break;
    }
    return 1;
}

/**
 * Starts the finishing program of one of the monster's effects.
 */
static int _ESM_FINISH(RS_STACKDATA *args, int argc) {
    int user_id;
    int slot;

    user_id = nowMonster->chara_type;
    slot = GetStackInt(args);
    return ActiveMonster->effect_man->SetScriptProgNo(300, user_id, slot);
}

/**
 * Deletes one of the monster's effects.
 */
static int _ESM_DELETE(RS_STACKDATA *args, int argc) {
    int user_id;
    int slot;

    user_id = nowMonster->chara_type;
    slot = GetStackInt(args);
    return ActiveMonster->effect_man->DeleteEffSpt(user_id, slot);
}

#ifdef NONMATCHING
/**
 * Sets the first work vector of one of the monster's effects.
 */
static int _ESM_SET_VECT1(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;
    int           slot;

    slot = GetStackInt(args++);
    vector[0] = GetStackFloat(args++);
    vector[1] = GetStackFloat(args++);
    vector[2] = GetStackFloat(args);
    vector[3] = 1.0f;
    return ActiveMonster->effect_man->SetScriptVect1(vector, nowMonster->chara_type, slot);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_SET_VECT1__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Writes the first work vector of one of the monster's effects to script variables.
 */
static int _ESM_GET_VECT1(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;
    int           slot;
    int           result;

    if (argc != 4) {
        return 0;
    }
    slot = GetStackInt(args++);
    result = ActiveMonster->effect_man->GetScriptVect1(vector, nowMonster->chara_type, slot);
    SetStack(args++, vector[0]);
    SetStack(args++, vector[1]);
    SetStack(args, vector[2]);
    return result;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_GET_VECT1__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Sets the second work vector of one of the monster's effects.
 */
static int _ESM_SET_VECT2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;
    int           slot;

    if (argc != 4) {
        return 0;
    }
    slot = GetStackInt(args++);
    vector[0] = GetStackFloat(args++);
    vector[1] = GetStackFloat(args++);
    vector[2] = GetStackFloat(args);
    vector[3] = 1.0f;
    return ActiveMonster->effect_man->SetScriptVect2(vector, nowMonster->chara_type, slot);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_SET_VECT2__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Writes the second work vector of one of the monster's effects to script variables.
 */
static int _ESM_GET_VECT2(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR vector;
    int           slot;
    int           result;

    if (argc != 4) {
        return 0;
    }
    slot = GetStackInt(args++);
    result = ActiveMonster->effect_man->GetScriptVect2(vector, nowMonster->chara_type, slot);
    SetStack(args++, vector[0]);
    SetStack(args++, vector[1]);
    SetStack(args, vector[2]);
    return result;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_GET_VECT2__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Sets the target character of one of the monster's effects.
 */
static int _ESM_SET_TARGET_ID(RS_STACKDATA *args, int argc) {
    int slot;
    int id;

    slot = GetStackInt(args++);
    id = GetStackInt(args);
    return ActiveMonster->effect_man->SetScriptTargetId(id, nowMonster->chara_type, slot);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_SET_TARGET_ID__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Writes the target character of one of the monster's effects to a script variable.
 */
static int _ESM_GET_TARGET_ID(RS_STACKDATA *args, int argc) {
    int slot;
    int id;
    int result;

    slot = GetStackInt(args++);
    result = ActiveMonster->effect_man->GetScriptTargetId(id, nowMonster->chara_type, slot);
    SetStack(args, id);
    return result;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_GET_TARGET_ID__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Sets the owner of one of the monster's effects.
 */
static int _ESM_SET_USER_ID(RS_STACKDATA *args, int argc) {
    int slot;
    int id;

    slot = GetStackInt(args++);
    id = GetStackInt(args);
    return ActiveMonster->effect_man->SetScriptUserId(id, nowMonster->chara_type, slot);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_SET_USER_ID__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Writes the owner of one of the monster's effects to a script variable.
 */
static int _ESM_GET_USER_ID(RS_STACKDATA *args, int argc) {
    int slot;
    int id;
    int result;

    slot = GetStackInt(args++);
    result = ActiveMonster->effect_man->GetScriptUserId(id, nowMonster->chara_type, slot);
    SetStack(args, id);
    return result;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _ESM_GET_USER_ID__FP12RS_STACKDATAi);
#endif

/**
 * Sets an integer or float variable of one of the monster's effects.
 */
static int _ESM_SET_VALUE(RS_STACKDATA *args, int argc) {
    int slot;
    int index;
    int user_id;
    int result;

    user_id = nowMonster->chara_type;
    slot = GetStackInt(args++);
    index = GetStackInt(args++);
    switch (args->type) {
        case RS_INT:
            result =
                ActiveMonster->effect_man->SetValue(index, GetStackInt(args), user_id, slot);
            break;
        case RS_FLOAT:
            result =
                ActiveMonster->effect_man->SetValue(index, GetStackFloat(args), user_id, slot);
            break;
        default:
            return 0;
    }
    return result;
}

/**
 * Loads an effect base by number or name into the scene memory stack.
 */
static int _LOAD_EFFECT_SCRIPT(RS_STACKDATA *args, int argc) {
    int        result;
    int        texture_block;
    mgCMemory *memory;
    int        base_no;
    char      *name;

    FxScriptMan->level = 3;
    texture_block = -1;
    memory = nowScene->GetStack(3);
    switch (args->type) {
        case RS_INT:
            base_no = GetStackInt(args++);
            if (argc >= 2) {
                texture_block = GetStackInt(args++);
            }
            result = ActiveMonster->effect_man->LoadBaseEffSpt(base_no, memory, texture_block);
            break;
        case RS_STR:
            name = GetStackString(args++);
            if (argc >= 2) {
                texture_block = GetStackInt(args++);
            }
            result = ActiveMonster->effect_man->LoadBaseEffSpt(name, memory, texture_block);
            break;
        default:
            return 0;
    }
    if (argc >= 3) {
        if (result > 0) {
            SetStack(args, 1);
        } else {
            SetStack(args, 0);
        }
    }
    return result >= 0;
}

/**
 * Adds a sword after-image to the monster's motion.
 */
static int _SW_EFFECT(RS_STACKDATA *args, int argc) {
    ACTION_SW_EFFECT *effect;
    int               sword_no;
    char             *motion;
    float             start;
    float             end;
    char             *frame0;
    char             *frame1;
    int               mode0;
    int               mode1;
    int               fade_time;

    if (argc != 9) {
        return 0;
    }
    effect = nowMonster->GetSwEffectPtr();
    if (effect == NULL) {
        return 0;
    }
    sword_no = GetStackInt(args++);
    if (sword_no < 0 || sword_no > CHARA_SWORD_EFFECT_MAX - 1) {
        return 0;
    }
    if (nowMonster->sword_effect[sword_no] == NULL) {
        return 0;
    }
    motion = GetStackString(args++);
    start = GetStackFloat(args++);
    end = GetStackFloat(args++);
    frame0 = GetStackString(args++);
    frame1 = GetStackString(args++);
    mode0 = GetStackInt(args++);
    mode1 = GetStackInt(args++);
    fade_time = GetStackInt(args);
    effect->sword_no = sword_no;
    effect->motion = motion;
    effect->start = start;
    effect->end = end;
    effect->frame0 = frame0;
    effect->frame1 = frame1;
    effect->unk_1c = mode0;
    effect->unk_1d = mode1;
    effect->fade_time = fade_time;
    effect->wait = 0;
    nowMonster->sw_effect_num++;
    return 1;
}

/**
 * Writes the next free effect texture block to a script variable.
 */
static int _ESM_GET_NOTUESD_TEXB(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, ActiveMonster->effect_man->GetNotUsedTexb());
    return 1;
}

/**
 * Reserves another texture block for monster effects.
 */
static int _ESM_ADD_TEXB(RS_STACKDATA *args, int argc) {
    ActiveMonster->effect_man->AddTexb();
    return 1;
}

#ifdef NONMATCHING
/**
 * Fires a rocket from a script position and direction with optional homing and damage settings.
 */
static int _SHOT_ROCKET_LAUNCHER(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR    position;
    sceVu0FVECTOR    target;
    sceVu0FVECTOR    direction;
    CRocketLauncher *rocket;
    CColPrim        *primitive;
    float            speed;
    int              homing_delay;
    int              homing_time;
    int              damage;
    int              primitive_id;

    if (argc != 6 && argc != 10) {
        return 0;
    }
    GetStackVector(position, &args);
    GetStackVector(direction, &args);
    if (argc == 10) {
        speed = GetStackFloat(args++);
        homing_delay = GetStackInt(args++);
        homing_time = GetStackInt(args++);
        damage = GetStackInt(args++);
    } else {
        speed = 12.0f;
        homing_delay = 6;
        homing_time = 45;
        damage = nowMonster->attack;
    }
    sceVu0Normalize(direction, direction);
    sceVu0ScaleVector(target, direction, 500.0f);
    sceVu0AddVector(target, position, target);
    direction[1] += 0.1f;
    rocket = RocketLauncher.Get();
    if (rocket == NULL) {
        return 0;
    }
    rocket->SetPos(position, target, direction);
    rocket->target_chara = 0;
    rocket->speed = speed;
    rocket->homing_delay = homing_delay;
    rocket->homing_time = homing_time;
    primitive = ColPrimMan.GetPrim();
    primitive_id = -1;
    if (primitive != NULL) {
        primitive->SetDamage("\x83\x8d\x83\x7b\x83\x89\x83\x93\x83\x60\x83\x83", 2);
        primitive->SetCoord(position, 5.0f);
        primitive->damage = damage;
        primitive_id = primitive->id;
    }
    rocket->col_prim_id = primitive_id;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SHOT_ROCKET_LAUNCHER__FP12RS_STACKDATAi);
#endif

/**
 * Requests an event script from the monster.
 */
static int _RUN_EVENT_SCRIPT(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowMonster->event_no = GetStackInt(args);
    return 1;
}

/**
 * Sets or clears the scene status of the monster's character slot.
 */
static int _SET_STATUS(RS_STACKDATA *args, int argc) {
    int flags;
    int on;

    flags = GetStackInt(args++);
    on = 1;
    if (argc >= 2) {
        on = GetStackInt(args);
    }
    if (on != 0) {
        nowScene->SetStatus(SCENE_DATA_CHARA, nowMonster->chara_type, flags);
    } else {
        nowScene->ResetStatus(SCENE_DATA_CHARA, nowMonster->chara_type, flags);
    }
    return 1;
}

/**
 * Sets or clears the dungeon pause flags.
 */
static int _SET_PAUSE(RS_STACKDATA *args, int argc) {
    RS_STACKDATA    *next;
    DNG_BATTLE_AREA *area;
    int              flags;

    area = &nowScene->battle_area;
    next = args + 1;
    if (area == NULL) {
        return 0;
    }
    flags = GetStackInt(args);
    if (GetStackInt(next) != 0) {
        area->pause_flag |= flags;
    } else {
        area->pause_flag &= ~flags;
    }
    return 1;
}

/**
 * Writes the masked dungeon pause flags to a script variable.
 */
static int _CHECK_PAUSE(RS_STACKDATA *args, int argc) {
    DNG_BATTLE_AREA *area;
    int              flags;

    area = &nowScene->battle_area;
    if (area == NULL) {
        return 0;
    }
    flags = GetStackInt(args++);
    SetStack(args, (int)(area->pause_flag & flags));
    return 1;
}

/**
 * Writes a saved bit flag to a script variable.
 */
static int _GET_BIT_FLAG(RS_STACKDATA *args, int argc) {
    int flag;
    int value;

    if (argc != 2) {
        return 0;
    }
    flag = GetStackInt(args++);
    value = DngSaveData->GetBitFlag(flag);
    SetStack(args, value);
    return 1;
}

/**
 * Sets a saved bit flag from the script.
 */
static int _SET_BIT_FLAG(RS_STACKDATA *args, int argc) {
    int flag;
    int value;

    if (argc != 2) {
        return 0;
    }
    flag = GetStackInt(args++);
    value = GetStackInt(args);
    DngSaveData->SetBitFlag(flag, value);
    return 1;
}

/**
 * Writes the kind of attack that last hit the monster to a script variable.
 */
static int _GET_ATT_TYPE(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, (int)nowMonster->att_type);
    return 1;
}

/**
 * Writes the player's battle status attributes to a script variable.
 */
static int _GET_USER_ATTR(RS_STACKDATA *args, int argc) {
    int attr;

    if (argc != 1) {
        return 0;
    }
    attr = GetBattleCharaInfo()->GetAttr();
    SetStack(args, attr);
    return 1;
}

/**
 * Copies a reserved image over the monster's current image archive.
 */
static int _TRANS_RESERV_IMG(RS_STACKDATA *args, int argc) {
    int image_no;

    if (argc != 1) {
        return 0;
    }
    image_no = GetStackInt(args);
    if (image_no < 0 || image_no > 1) {
        return 0;
    }
    if (nowMonster->reserv_img[image_no] == NULL) {
        return 0;
    }
    memcpy(nowMonster->images[0], nowMonster->reserv_img[image_no], nowMonster->reserv_img_size[image_no]);
    return 1;
}

/**
 * Writes the monster's masked status attributes to a script variable.
 */
static int _GET_STS_ATTR(RS_STACKDATA *args, int argc) {
    int flags;

    if (argc != 2) {
        return 0;
    }
    flags = GetStackInt(args++);
    SetStack(args, (int)(nowMonster->status.attr & flags));
    return 1;
}

/**
 * Shows the monster's stun stars for the remaining stun time.
 */
static int _SET_PIYORI_MARK(RS_STACKDATA *args, int argc) {
    nowMonster->piyori_mark = nowMonster->piyori_time;
    nowMonster->piyori.Set(nowMonster, nowMonster->piyori_mark);
    return 1;
}

/**
 * Writes the remaining time of the monster's stun mark to a script variable.
 */
static int _CHECK_PIYORI(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(args, (int)nowMonster->piyori_mark);
    return 1;
}

/**
 * Writes the monster's attack power to a script variable.
 */
static int _GET_BASE_ATTACK(RS_STACKDATA *args, int argc) {
    SetStack(args, (int)nowMonster->attack);
    return 1;
}

/**
 * Writes the nearest other scripted monster's position and distance to script variables.
 */
static int _GET_NEAR_MONS_POS(RS_STACKDATA *args, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR nearest_position;
    sceVu0FVECTOR monster_position;
    CActionChara *monster;
    float         nearest_distance;
    float         distance;
    int           index;

    nearest_distance = -1.0f;
    nowMonster->GetPosition(position);
    mgZeroVector(nearest_position);
    for (index = 0; index < MONSTER_ACTIVE_MAX; index++) {
        if ((monster = (CActionChara *)nowScene->GetCharacter(
                 index + MONSTER_ACTIVE_MAX)) != NULL &&
            monster->chara_kind == ACTION_KIND_SCRIPT &&
            nowMonster->chara_type != monster->chara_type) {
            monster->GetPosition(monster_position);
            if (nearest_distance >= 0.0) {
                distance = mgDistVector(position, monster_position);
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    *(u_long128 *)nearest_position = *(u_long128 *)monster_position;
                }
            } else {
                *(u_long128 *)nearest_position = *(u_long128 *)monster_position;
                nearest_distance = mgDistVector(position, nearest_position);
            }
        }
    }
    SetStack(args++, nearest_position[0]);
    SetStack(args++, nearest_position[1]);
    SetStack(args++, nearest_position[2]);
    SetStack(args, nearest_distance);
    return 1;
}

#ifdef NONMATCHING
/**
 * Sets the collision radius of one of the monster's entered objects.
 */
static int _SET_INDEXOBJ_SIZE(RS_STACKDATA *args, int argc) {
    int   index;
    float size;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    size = GetStackFloat(args);
    if (index == -1) {
        index = 0;
    }
    nowMonster->entry_object[index].unk_04 = size;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _SET_INDEXOBJ_SIZE__FP12RS_STACKDATAi);
#endif

#ifdef NONMATCHING
/**
 * Writes the collision radius of one of the monster's entered objects to a script variable.
 */
static int _GET_INDEXOBJ_SIZE(RS_STACKDATA *args, int argc) {
    int index;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(args++);
    if (index == -1) {
        index = 0;
    }
    SetStack(args, nowMonster->entry_object[index].unk_04);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", _GET_INDEXOBJ_SIZE__FP12RS_STACKDATAi);
#endif

/**
 * Sets the alpha of the previous frame used for scene motion blur.
 */
static int _SET_MOTION_BLUR(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowScene->fade.blur_alpha = GetStackInt(args);
    return 1;
}

/**
 * Plays a sound in another monster's bank.
 */
static int _MONS_SE_PLAY(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             character_no;
    int             sound_no;

    if (argc != 2) {
        return 0;
    }
    character_no = GetStackInt(args++);
    sound_no = GetStackInt(args);
    character_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[character_no];
    if (monster == NULL) {
        return 0;
    }
    sndSePlay(monster->se_bank, sound_no, 0);
    return 1;
}

/**
 * Stops a sound in another monster's bank.
 */
static int _MONS_SE_STOP(RS_STACKDATA *args, int argc) {
    CActiveMonster *monster;
    int             character_no;
    int             sound_no;

    if (argc != 2) {
        return 0;
    }
    character_no = GetStackInt(args++);
    sound_no = GetStackInt(args);
    character_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[character_no];
    if (monster == NULL) {
        return 0;
    }
    sndSeStop(monster->se_bank, sound_no, 0);
    return 1;
}

/**
 * Starts or stops a looping sound in another monster's bank.
 */
static int _MONS_SE_LOOP(RS_STACKDATA *args, int argc) {
    int             character_no;
    int             sound_no;
    int             keep_time;
    CActiveMonster *monster;

    if (argc != 3) {
        return 0;
    }
    character_no = GetStackInt(args++);
    sound_no = GetStackInt(args++);
    keep_time = GetStackInt(args);
    character_no -= MONSTER_ACTIVE_MAX;
    monster = ActiveMonster->active[character_no];
    if (monster == NULL) {
        return 0;
    }
    monster->loop_se->SeLoopPlayStop((int)monster->se_bank, sound_no, keep_time, 13);
    return 1;
}

/**
 * Sets whether the monster's sound volume follows its position.
 */
static int _MONS_VOL_CTRL(RS_STACKDATA *args, int argc) {
    if (argc != 1) {
        return 0;
    }
    nowMonster->se_positional = GetStackInt(args);
    return 1;
}

/**
 * Shows or hides a named map part or one of its pieces.
 */
static int _SET_MAPOBJ_SHOW(RS_STACKDATA *args, int argc) {
    CMap      *maps[8];
    CMapParts *parts;
    int        map_count;
    char      *parts_name;
    char      *piece_name;
    CMapPiece *piece;
    int        on;
    int        index;

    if (argc != 2 && argc != 3) {
        return 0;
    }
    map_count = nowScene->GetActiveMap(maps, 8);
    if (map_count <= 0) {
        return 0;
    }
    parts_name = GetStackString(args++);
    if (argc == 3) {
        piece_name = GetStackString(args++);
    }
    on = GetStackInt(args);
    for (index = 0; index < map_count; index++) {
        parts = maps[index]->GetPlaceParts(parts_name);
        if (parts != NULL) {
            break;
        }
    }
    if (parts == NULL) {
        return 0;
    }
    if (argc == 3) {
        if ((piece = parts->SearchPiece(piece_name)) == NULL) {
            return 0;
        }
    }
    if (argc == 2) {
        parts->Show(on);
    } else {
        piece->Show(on);
    }
    return 1;
}

int SetMonsterScript(CRunScript *script, char *program, mgCMemory *memory) {
    RS_STACKDATA *stack;
    RS_CALLDATA  *call;

    stack = (RS_STACKDATA *)memory->Alloc(64);
    call = (RS_CALLDATA *)memory->Alloc(384);
    script->load((RS_PROG_HEADER *)program, stack, 128, call, 512);
    script->ext_func(ext_func, 256);
    return 1;
}

#ifdef NONMATCHING
void SetMonsterExtendTable() {
    int index;
    int earlier;
    int number;

    for (index = 0; index < 256; index++) {
        ext_func[index] = NULL;
    }
    for (index = 0; ext_func_info[index].func != NULL; index++) {
        for (earlier = 0; earlier < index; earlier++) {
            if (ext_func_info[index].no == ext_func_info[earlier].no) {
                printf("mscript same ext_func_no!!!\n");
                for (;;) {
                }
            }
        }
        number = ext_func_info[index].no;
        if (number < 0 || number >= 256) {
            printf("ext func over!!");
        } else {
            ext_func[number] = ext_func_info[index].func;
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/runscript_opcodes", SetMonsterExtendTable__Fv);
#endif

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1480__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1481__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1864__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_2160__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1728__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1733__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_1784__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_2398__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_2399__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_2580__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_2787__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_3078__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/runscript_opcodes", at_3079__DATA);

// Small uninitialised data (.sbss)

// Uninitialised data (.bss)
