#include "common.h"
#include "actscript.hpp"
#include "runscript_opcodes.hpp"
#include "padcontrol.hpp"
#include "dng_object.hpp"
#include "runscript.hpp"
#include "monster.hpp"
#include "scenesnd.hpp"
#include "event_func.hpp"
#include "character.hpp"
#include "actionchara.hpp"
#include "cameracontrol.hpp"
#include "effscript.hpp"
#include "colprim.hpp"
#include "dng_main.hpp"
#include "userdata.hpp"
#include "maintex.hpp"
#include "dng_hud.hpp"
#include "dng_effect.hpp"
#include "mg_frame.hpp"
#include "mg_memory.hpp"
#include "mg_math.hpp"
#include "map.hpp"
#include "mapparts.hpp"
#include "object.hpp"
#include "mainloop.hpp"
#include "dataread.hpp"
#include "gamepad.hpp"
#include "sound.hpp"
#include "snd_mngr.hpp"
#include "menucommon.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <libvu0.h>

// Integer health division traps when its divisor is zero.
#pragma divbyzerocheck on

CScene *nowScene; /**< Scene of the action character. */
static ACTION_DAMAGE *LastCInfo2; /**< Last damage entry created by the action script. */
ACTION_INFO action_info; /**< Environment of the running action character. */
static int (*ext_func[256])(RS_STACKDATA *, int); /**< Action handlers indexed by script number. */

static int GetStackInt(RS_STACKDATA *slot);
static float GetStackFloat(RS_STACKDATA *slot);
static char *GetStackString(RS_STACKDATA *slot);
static void SetStack(RS_STACKDATA *slot, int value);
static void SetStack(RS_STACKDATA *slot, float value);
static int _INIT_SCRIPT(RS_STACKDATA *stack, int argc);
static int _PROG_SET(RS_STACKDATA *stack, int argc);
static int _PROG_GET(RS_STACKDATA *stack, int argc);
static int _GET_ATTK_TYPE(RS_STACKDATA *stack, int argc);
static int _GET_MOVE_TYPE(RS_STACKDATA *stack, int argc);
static int _CHECK_PAUSE(RS_STACKDATA *stack, int argc);
static int _GET_PADON(RS_STACKDATA *stack, int argc);
static int _GET_PADDOWN(RS_STACKDATA *stack, int argc);
static int _GET_PADUP(RS_STACKDATA *stack, int argc);
static int _GET_BTN(RS_STACKDATA *stack, int argc);
static int _GET_PAD_HISTORY(RS_STACKDATA *stack, int argc);
static int _RESET_PAD_HISTORY(RS_STACKDATA *stack, int argc);
static int _GET_ACUMU_PAD(RS_STACKDATA *stack, int argc);
static int _RESET_ACUMU_PAD(RS_STACKDATA *stack, int argc);
static int _RUN_MAIN_MOVE(RS_STACKDATA *stack, int argc);
static int _RUN_SHROW_MOVE(RS_STACKDATA *stack, int argc);
static int _RUN_TAME_MOVE(RS_STACKDATA *stack, int argc);
static int _RUN_HOLD_MOVE(RS_STACKDATA *stack, int argc);
static int _RUN_ROBO_MOVE(RS_STACKDATA *stack, int argc);
static int _SET_MENU_FLAG(RS_STACKDATA *stack, int argc);
static int _GET_POS(RS_STACKDATA *stack, int argc);
static int _GET_ROT(RS_STACKDATA *stack, int argc);
static int _CHECK_FRONT_KEY(RS_STACKDATA *stack, int argc);
static int _CHECK_BACK_KEY(RS_STACKDATA *stack, int argc);
static int _SET_BLOW_ANGLE(RS_STACKDATA *stack, int argc);
static int _SET_BLOW_MOVE(RS_STACKDATA *stack, int argc);
static int _BLOW_START(RS_STACKDATA *stack, int argc);
static int _SET_DMG2(RS_STACKDATA *stack, int argc);
static int _SET_OBJ(RS_STACKDATA *stack, int argc);
static int _SET_BODY(RS_STACKDATA *stack, int argc);
static int _SW_EFFECT(RS_STACKDATA *stack, int argc);
static int _SET_SND(RS_STACKDATA *stack, int argc);
static int _SET_ACCUME_FX(RS_STACKDATA *stack, int argc);
static int _GET_MONSTER_NOWSTS(RS_STACKDATA *stack, int argc);
static int _SET_MURDEROUS(RS_STACKDATA *stack, int argc);
static int _GET_TRG_DISTANCE(RS_STACKDATA *stack, int argc);
static int _SET_GUARD_FLAG(RS_STACKDATA *stack, int argc);
static int _SET_MUTEKI(RS_STACKDATA *stack, int argc);
static int _CHECK_HAND_OBJ(RS_STACKDATA *stack, int argc);
static int _SET_ITEM_USED(RS_STACKDATA *stack, int argc);
static int _THROW_HAND_OBJECT(RS_STACKDATA *stack, int argc);
static int _CHECK_CATCH(RS_STACKDATA *stack, int argc);
static int _RELEASE_OBJ(RS_STACKDATA *stack, int argc);
static void ShotMonicaMagic(float *position, float *direction, float scale);
static void ShotNormalGun(float *position, float *direction);
static void ShotMachineGun(float *position, float *direction, char *damage_name, float damage);
static void ShotGrenadGun(float *position, float *direction);
static void ShotLaserGun(float *position, float *direction, int type);
static int _SET_SPECIAL_SHOT(RS_STACKDATA *stack, int argc);
static int _GET_OBJECT_POS(RS_STACKDATA *stack, int argc);
static int _SET_DIR_GUN(RS_STACKDATA *stack, int argc);
static int _GET_NOW_HP_RATE(RS_STACKDATA *stack, int argc);
static int _SET_BOMB(RS_STACKDATA *stack, int argc);
static int _GET_ACTION_CODE(RS_STACKDATA *stack, int argc);
static int _GET_ATTK_POINT(RS_STACKDATA *stack, int argc);
static int _GET_RING_COLOR(RS_STACKDATA *stack, int argc);
static int _SET_MOS(RS_STACKDATA *stack, int argc);
static int _CHECK_MOS_END(RS_STACKDATA *stack, int argc);
static int _NOW_MOS_WAIT(RS_STACKDATA *stack, int argc);
static int _NOW_MOS_CHGWAIT(RS_STACKDATA *stack, int argc);
static int _GET_MOS_STATUS(RS_STACKDATA *stack, int argc);
static int _SET_XCHG_STEP(RS_STACKDATA *stack, int argc);
static int _SET_MOS_STEP(RS_STACKDATA *stack, int argc);
static int _TRG_ON_MOS(RS_STACKDATA *stack, int argc);
static int _RESET_MOS(RS_STACKDATA *stack, int argc);
static int _SET_DEFAULT_MOS(RS_STACKDATA *stack, int argc);
static int _SET_NEBA2(RS_STACKDATA *stack, int argc);
static int _ESM_CREATE(RS_STACKDATA *stack, int argc);
static int _ESM_SET_VECT1(RS_STACKDATA *stack, int argc);
static int _ESM_SET_VECT2(RS_STACKDATA *stack, int argc);
static int _ESM_FINISH(RS_STACKDATA *stack, int argc);
static int _ESM_DELETE(RS_STACKDATA *stack, int argc);
static int _ESM_SET_VALUE(RS_STACKDATA *stack, int argc);
static int _SET_MOVE_SPEED(RS_STACKDATA *stack, int argc);
static int _SET_PALLET(RS_STACKDATA *stack, int argc);
static int _CHECK_EQUIP(RS_STACKDATA *stack, int argc);
static int _CAMERA_QUAKE(RS_STACKDATA *stack, int argc);
static int _GET_STATUS_ATTR(RS_STACKDATA *stack, int argc);
static int _SE_PLAY(RS_STACKDATA *stack, int argc);
static int _SE_LOOP_PLAY(RS_STACKDATA *stack, int argc);
static int _GET_SHOT_TYPE(RS_STACKDATA *stack, int argc);
static int _GET_MONS_ID(RS_STACKDATA *stack, int argc);
static int _GET_FRONT_VEC(RS_STACKDATA *stack, int argc);
static int _SET_ACCUME_FLAG(RS_STACKDATA *stack, int argc);
static int _SET_TRG_ANGLE(RS_STACKDATA *stack, int argc);
static int _SET_SHOT(RS_STACKDATA *stack, int argc);
static int _SHOT(RS_STACKDATA *stack, int argc);
static void ParabolicInitialVector(float *result, float *from, float *to, float height, float gravity);

/** Associates action-script function numbers with their handlers. */
static RS_EXTFUNC_INFO ext_func_info[] = {
    { _INIT_SCRIPT, 0 },
        { _PROG_SET, 1 },
        { _PROG_GET, 2 },
        { _GET_ATTK_TYPE, 3 },
        { _GET_MOVE_TYPE, 4 },
        { _SET_MOVE_SPEED, 5 },
        { _SET_PALLET, 30 },
        { _CHECK_EQUIP, 31 },
        { _CAMERA_QUAKE, 32 },
        { _CHECK_PAUSE, 33 },
        { _GET_STATUS_ATTR, 34 },
        { _SE_PLAY, 35 },
        { _SE_LOOP_PLAY, 36 },
        { _GET_SHOT_TYPE, 37 },
        { _GET_MONS_ID, 38 },
        { _GET_FRONT_VEC, 39 },
        { _GET_PADON, 40 },
        { _GET_PADDOWN, 41 },
        { _GET_PADUP, 42 },
        { _GET_BTN, 43 },
        { _GET_PAD_HISTORY, 45 },
        { _RESET_PAD_HISTORY, 46 },
        { _GET_ACUMU_PAD, 47 },
        { _RESET_ACUMU_PAD, 48 },
        { _RUN_MAIN_MOVE, 49 },
        { _RUN_SHROW_MOVE, 50 },
        { _RUN_TAME_MOVE, 51 },
        { _RUN_HOLD_MOVE, 52 },
        { _SET_MENU_FLAG, 59 },
        { _GET_POS, 53 },
        { _GET_ROT, 61 },
        { _CHECK_FRONT_KEY, 54 },
        { _CHECK_BACK_KEY, 55 },
        { _SET_BLOW_ANGLE, 56 },
        { _SET_BLOW_MOVE, 57 },
        { _BLOW_START, 58 },
        { _RUN_ROBO_MOVE, 60 },
        { _SET_DMG2, 71 },
        { _SET_OBJ, 72 },
        { _SET_BODY, 73 },
        { _SW_EFFECT, 75 },
        { _SET_SND, 76 },
        { _SET_ACCUME_FX, 77 },
        { _SET_ACCUME_FLAG, 78 },
        { _GET_MONSTER_NOWSTS, 90 },
        { _SET_MURDEROUS, 91 },
        { _GET_TRG_DISTANCE, 92 },
        { _SET_TRG_ANGLE, 93 },
        { _SET_GUARD_FLAG, 94 },
        { _SET_MUTEKI, 95 },
        { _CHECK_HAND_OBJ, 96 },
        { _SET_ITEM_USED, 97 },
        { _THROW_HAND_OBJECT, 98 },
        { _CHECK_CATCH, 99 },
        { _RELEASE_OBJ, 100 },
        { _SET_SHOT, 101 },
        { _SET_SPECIAL_SHOT, 105 },
        { _SHOT, 102 },
        { _GET_OBJECT_POS, 103 },
        { _SET_DIR_GUN, 104 },
        { _GET_NOW_HP_RATE, 106 },
        { _SET_BOMB, 107 },
        { _GET_ACTION_CODE, 108 },
        { _GET_ATTK_POINT, 109 },
        { _GET_RING_COLOR, 110 },
        { _SET_MOS, 130 },
        { _CHECK_MOS_END, 131 },
        { _NOW_MOS_WAIT, 132 },
        { _GET_MOS_STATUS, 133 },
        { _SET_XCHG_STEP, 134 },
        { _SET_MOS_STEP, 135 },
        { _TRG_ON_MOS, 136 },
        { _RESET_MOS, 137 },
        { _SET_DEFAULT_MOS, 138 },
        { _SET_NEBA2, 140 },
        { _NOW_MOS_CHGWAIT, 139 },
        { _ESM_CREATE, 150 },
        { _ESM_SET_VECT1, 151 },
        { _ESM_SET_VECT2, 152 },
        { _ESM_FINISH, 153 },
        { _ESM_DELETE, 154 },
        { _ESM_SET_VALUE, 155 },
        { NULL, -1 },
};

// Code (.text)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", ParabolicInitialVector__FPfPfPfff);
/**
 * Returns a stack value as an integer.
 */
static int GetStackInt(RS_STACKDATA *slot) {
    if (slot->type == RS_FLOAT) {
        return (int)slot->f;
    }
    return slot->i;
}

/**
 * Returns a stack value as a float.
 */
static float GetStackFloat(RS_STACKDATA *slot) {
    if (slot->type == RS_INT) {
        return (float)slot->i;
    }
    return slot->f;
}

/**
 * Returns the string held by a stack value.
 */
static char *GetStackString(RS_STACKDATA *slot) {
    return slot->s;
}

/**
 * Stores a value through a stack reference.
 */
static void SetStack(RS_STACKDATA *slot, int value) {
    if (slot->type == RS_PTR) {
        slot->p->i = value;
    }
}

/**
 * Stores a value through a stack reference.
 */
static void SetStack(RS_STACKDATA *slot, float value) {
    if (slot->type == RS_PTR) {
        slot->p->f = value;
    }
}

/**
 * Resets the action character script state.
 */
static int _INIT_SCRIPT(RS_STACKDATA *stack, int argc) {
    action_info.chara->ResetScript();
    return 1;
}

/**
 * Sets the requested program of the action character.
 */
static int _PROG_SET(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->prog = GetStackInt(stack);
    return 1;
}

/**
 * Returns the requested program of the action character.
 */
static int _PROG_GET(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, action_info.chara->prog);
    return 1;
}

/**
 * Returns the attack type of the action character.
 */
static int _GET_ATTK_TYPE(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, action_info.chara->attack_type);
    return 1;
}

/**
 * Returns the movement type of the action character.
 */
static int _GET_MOVE_TYPE(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, action_info.chara->move_type);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SET_MOVE_SPEED__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SET_PALLET__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _CHECK_EQUIP__FP12RS_STACKDATAi);
/**
 * Shakes the scene camera and reduces its strength over the requested duration.
 */
static int _CAMERA_QUAKE(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *battle;
    RS_STACKDATA    *duration;
    float            power;
    int              frames;

    battle = &nowScene->battle_area;
    duration = &stack[1];

    if (battle == NULL) {
        return 0;
    }
    power = GetStackFloat(stack);
    frames = GetStackInt(duration);
    battle->quake_power = power;
    battle->quake_step = battle->quake_power / (float)frames;
    battle->quake_count = frames;
    return 1;
}
/**
 * Returns the requested pause flags from the scene battle area.
 */
static int _CHECK_PAUSE(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *pause;

    if (argc != 2) {
        return 0;
    }
    pause = &nowScene->battle_area;
    if (pause == NULL) {
        return 0;
    }
    SetStack(stack, (int)(pause->pause_flag & GetStackInt(stack++)));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _GET_STATUS_ATTR__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SE_PLAY__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SE_LOOP_PLAY__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _GET_SHOT_TYPE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _GET_MONS_ID__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _GET_FRONT_VEC__FP12RS_STACKDATAi);
/**
 * Returns the controller buttons held down.
 */
static int _GET_PADON(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad.GetPadOn());
    return 1;
}

/**
 * Returns the controller buttons pressed this frame.
 */
static int _GET_PADDOWN(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad.GetPadDown());
    return 1;
}

/**
 * Returns the controller buttons released this frame.
 */
static int _GET_PADUP(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad.GetPadUp());
    return 1;
}

/**
 * Returns the state of a logical controller button.
 */
static int _GET_BTN(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, PadCtrl.Btn(GetStackInt(stack++)));
    return 1;
}

/**
 * Returns the button history of the action character.
 */
static int _GET_PAD_HISTORY(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, (int)action_info.chara->pad_history);
    return 1;
}

/**
 * Clears the button history of the action character.
 */
static int _RESET_PAD_HISTORY(RS_STACKDATA *stack, int argc) {
    action_info.chara->pad_history = 0;
    return 1;
}

/**
 * Returns the accumulated charging input.
 */
static int _GET_ACUMU_PAD(RS_STACKDATA *stack, int argc) {
    SetStack(stack, action_info.chara->acumu_pad);
    return 1;
}

/**
 * Clears the accumulated charging input.
 */
static int _RESET_ACUMU_PAD(RS_STACKDATA *stack, int argc) {
    action_info.chara->acumu_pad = 0;
    return 1;
}

/**
 * Runs the human or monster movement controller.
 */
static int _RUN_MAIN_MOVE(RS_STACKDATA *stack, int argc) {
    int chara_type;

    chara_type = action_info.chara->move_type;
    switch (chara_type) {
        case 0:
            action_info.chara->HumanMoveIF();
            break;
        case 3:
            action_info.chara->MonsterMoveIF();
            break;
    }
    return 1;
}

/**
 * Runs the throwing movement controller.
 */
static int _RUN_SHROW_MOVE(RS_STACKDATA *stack, int argc) {
    action_info.chara->HumanShrowMoveIF();
    return 1;
}

/**
 * Runs the charging movement controller.
 */
static int _RUN_TAME_MOVE(RS_STACKDATA *stack, int argc) {
    action_info.chara->HumanTameMoveIF();
    return 1;
}

/**
 * Runs the human gun movement controller with two named motions.
 */
static int _RUN_HOLD_MOVE(RS_STACKDATA *stack, int argc) {
    char *first;

    if (argc != 2) {
        return 0;
    }
    first = GetStackString(stack++);
    action_info.chara->HumanGunMoveIF(first, GetStackString(stack));
    return 1;
}

/**
 * Runs the movement controller for the current ridepod configuration.
 */
static int _RUN_ROBO_MOVE(RS_STACKDATA *stack, int argc) {
    int input;

    input = GetStackInt(stack);
    switch (action_info.chara->move_type) {
        case 1:
        case 4:
        action_info.chara->RoboWalkMoveIF(input);
        break;
        case 2:
        case 5:
        action_info.chara->RoboTankMoveIF(input);
        break;
        case 3:
        action_info.chara->RoboBikeMoveIF(input);
        break;
        case 6:
        case 7:
        action_info.chara->RoboAirMoveIF(1, input);
        break;
    }
    return 1;
}

/**
 * Sets the menu flag of the action character.
 */
static int _SET_MENU_FLAG(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->menu_flag = (s8)GetStackInt(stack);
    return 1;
}

/**
 * Returns the position of the action character.
 */
static int _GET_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;

    if (argc != 3) {
        return 0;
    }
    action_info.chara->GetPosition(pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Returns the rotation of the action character.
 */
static int _GET_ROT(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR rot;

    if (argc != 3) {
        return 0;
    }
    action_info.chara->GetRotation(rot);
    SetStack(stack++, rot[0]);
    SetStack(stack++, rot[1]);
    SetStack(stack, rot[2]);
    return 1;
}

/**
 * Returns the alignment of the stick direction with the character facing direction.
 */
static int _CHECK_FRONT_KEY(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR facing;
    sceVu0FVECTOR stick;
    float         stick_x;
    float         stick_y;
    float         camera_angle;

    if (argc != 1) {
        return 0;
    }
    camera_angle = action_info.camera->GetAngle();
    sceVu0CopyVector(facing, action_info.chara->front_vec);
    stick_x = GamePad.GetLXf();
    stick_y = GamePad.GetLYf();
    stick[0] = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    stick[2] = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    stick[3] = 1.0f;
    stick[1] = 0.0f;
    sceVu0Normalize(stick, stick);
    sceVu0Normalize(facing, facing);
    SetStack(stack, sceVu0InnerProduct(facing, stick));
    return 1;
}

/**
 * Returns whether locked-on stick input points behind the character.
 */
static int _CHECK_BACK_KEY(RS_STACKDATA *stack, int argc) {
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         x;
    float         z;
    float         back;
    sceVu0FVECTOR rot;

    if (argc != 1) {
        return 0;
    }
    if (action_info.chara->lock_on == 0) {
        SetStack(stack, 0);
        return 1;
    }
    action_info.chara->GetRotation(rot);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad.GetLXf();
    stick_y = GamePad.GetLYf();
    x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    back = rot[1] - 3.1415927f;
    if (back < -3.1415927f) {
        back += 6.2831855f;
    }
    if (x != 0.0f && z != 0.0f && mgAngleCmp(back, atan2f(x, z), 1.2566371f) == 0) {
        SetStack(stack, 1);
        return 1;
    }
    SetStack(stack, 0);
    return 1;
}

/**
 * Turns the action character to face opposite its knockback vector.
 */
static int _SET_BLOW_ANGLE(RS_STACKDATA *stack, int argc) {
    CActionChara *chara;
    float         angle;

    if (argc != 0) {
        return 0;
    }
    chara = action_info.chara;
    angle = atan2f(-chara->blow_vec[0], -chara->blow_vec[2]);
    action_info.chara->SetRotation(0.0f, angle, 0.0f);
    return 1;
}

/**
 * Sets the additional movement vector, speed and duration after knockback.
 */
static int _SET_BLOW_MOVE(RS_STACKDATA *stack, int argc) {
    float         yaw;
    sceVu0FVECTOR rot;

    if (argc > 4) {
        return 0;
    }
    action_info.chara->add_speed = GetStackFloat(stack++);
    action_info.chara->add_decel = GetStackFloat(stack++);
    action_info.chara->add_time = GetStackInt(stack++);
    yaw = 0.0f;
    if (argc == 4) {
        yaw = 0.017453292f * GetStackFloat(stack);
    }
    action_info.chara->GetRotation(rot);
    yaw += rot[1];
    if (yaw > 3.1415927f) {
        yaw -= 6.2831855f;
    }
    if (yaw < -3.1415927f) {
        yaw += 6.2831855f;
    }
    sceVu0FVECTOR dir = { 0.0f, 0.0f, 1.0f, 1.0f };
    sceVu0FMATRIX matrix;
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, yaw);
    sceVu0ApplyMatrix(dir, matrix, dir);
    sceVu0CopyVector(action_info.chara->add_vec, dir);
    return 1;
}

/**
 * Sets the knockback speed, deceleration and duration.
 */
static int _BLOW_START(RS_STACKDATA *stack, int argc) {
    if (argc != 3) {
        return 0;
    }
    action_info.chara->blow_speed = GetStackFloat(stack++);
    action_info.chara->blow_speed =
        action_info.chara->blow_speed * action_info.chara->blow_rate;
    action_info.chara->blow_decel = GetStackFloat(stack++);
    action_info.chara->blow_time = GetStackInt(stack);
    return 1;
}

/**
 * Creates a named attack entry and assigns its power rate.
 */
static int _SET_DMG2(RS_STACKDATA *stack, int argc) {
    char *first;
    char *second;
    char *attack;
    float damage;
    float rate;
    char *hit_effect;
    float knockback;
    float lift;
    char *extra;

    if (argc < 8 || argc > 9) {
        return 0;
    }
    first = GetStackString(stack++);
    second = GetStackString(stack++);
    attack = GetStackString(stack++);
    damage = 2.0f * GetStackFloat(stack++);
    rate = GetStackFloat(stack++);
    hit_effect = GetStackString(stack++);
    knockback = GetStackFloat(stack++);
    lift = GetStackFloat(stack++);
    extra = NULL;
    if (argc == 9) {
        extra = GetStackString(stack);
    }
    LastCInfo2 = (ACTION_DAMAGE *)action_info.chara->EntryDamage2(
        first, second, attack, damage, hit_effect, knockback, lift, extra);
    if (LastCInfo2 == NULL) {
        printf("CACT:DMG_ENTRY_ERR %s\n", attack);
        return 0;
    }
    LastCInfo2->power_rate = rate;
    return 1;
}

/**
 * Assigns a named object to the requested character slot.
 */
static int _SET_OBJ(RS_STACKDATA *stack, int argc) {
    int   number;
    char *name;

    if (argc != 2) {
        return 0;
    }
    number = GetStackInt(stack++);
    name = GetStackString(stack);
    if (action_info.chara->EntryObject(name, number) == 0) {
        printf("not found %s\n", name);
        return 0;
    }
    return 1;
}

/**
 * Registers a body collision with the requested slot and radius.
 */
static int _SET_BODY(RS_STACKDATA *stack, int argc) {
    int number;

    if (argc != 2) {
        return 0;
    }
    number = GetStackInt(stack++);
    return action_info.chara->EntryBodyCol(number, 2.0f * GetStackFloat(stack)) != 0;
}

/**
 * Schedules a sword effect between named frames during a motion.
 */
static int _SW_EFFECT(RS_STACKDATA *stack, int argc) {
    ACTION_SW_EFFECT *effect;
    int               slot;
    char             *name;
    float             start;
    float             end;
    char             *first;
    char             *second;
    int               flag_a;
    int               flag_b;
    int               flag_c;
    char             *extra;

    if (argc < 9 || argc > 10) {
        return 0;
    }
    effect = (ACTION_SW_EFFECT *)action_info.chara->GetSwEffectPtr();
    if (effect == NULL) {
        return 0;
    }
    slot = GetStackInt(stack++);
    if (slot < 0 || slot > 2) {
        return 0;
    }
    if (action_info.chara->sword_effect[slot] == NULL) {
        return 0;
    }
    name = GetStackString(stack++);
    start = GetStackFloat(stack++);
    end = GetStackFloat(stack++);
    first = GetStackString(stack++);
    second = GetStackString(stack++);
    flag_a = GetStackInt(stack++);
    flag_b = GetStackInt(stack++);
    flag_c = GetStackInt(stack++);
    extra = NULL;
    if (argc == 10) {
        extra = GetStackString(stack);
    }
    effect->sword_no = slot;
    effect->motion = name;
    effect->chara = extra;
    effect->start = start;
    effect->end = end;
    effect->frame0 = first;
    effect->frame1 = second;
    effect->unk_1c = flag_a;
    effect->unk_1d = flag_b;
    effect->fade_time = flag_c;
    effect->wait = 0;
    action_info.chara->sw_effect_num++;
    return 1;
}

/**
 * Schedules a sound between two motion times.
 */
static int _SET_SND(RS_STACKDATA *stack, int argc) {
    int   sound_id;
    char *motion;
    float start_time;
    float end_time;
    char *wait;
    int   i;

    sound_id = GetStackInt(stack++);
    motion = GetStackString(stack++);
    start_time = GetStackFloat(stack++);
    end_time = GetStackFloat(stack++);
    wait = NULL;
    if (argc > 4) {
        wait = GetStackString(stack);
    }
    for (i = 0; i < 10; i++) {
        if (action_info.chara->sound[i].se_no == -1) {
            action_info.chara->sound[i].se_no = sound_id;
            action_info.chara->sound[i].start_frame =
                action_info.chara->GetWaitToFrame(motion, start_time, wait);
            action_info.chara->sound[i].end_frame =
                action_info.chara->GetWaitToFrame(motion, end_time, wait);
            action_info.chara->sound[i].chara = wait;
            action_info.chara->sound[i].unk_c = 0;
            return 1;
        }
    }
    return 0;
}

/**
 * Selects the object frame and effect number used when charging.
 */
static int _SET_ACCUME_FX(RS_STACKDATA *stack, int argc) {
    int       index;
    int       effect_no;
    mgCFrame *effect;

    if (argc != 2) {
        return 0;
    }
    if (action_info.chara->accume_effect == NULL) {
        return 0;
    }
    index = GetStackInt(stack++);
    effect_no = GetStackInt(stack);
    effect = action_info.chara->object[index].frame;
    if (effect == NULL) {
        return 0;
    }
    action_info.chara->accume.frame = effect;
    action_info.chara->accume.unk_4 = effect_no;
    action_info.chara->accume.active = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SET_ACCUME_FLAG__FP12RS_STACKDATAi);
/**
 * Returns the current status of the targeted monster.
 */
static int _GET_MONSTER_NOWSTS(RS_STACKDATA *stack, int argc) {
    int           status;
    int           monster_no;
    CActionChara *monster;

    if (argc != 1) {
        return 0;
    }
    status = 0;
    monster_no = action_info.chara->target_no;
    if (monster_no != -1) {
        monster = (CActionChara *)nowScene->GetCharacter(monster_no);
        if (monster != NULL) {
            status = monster->now_status;
        }
    }
    SetStack(stack, status);
    return 1;
}

/**
 * Sets the action character murderous timer and flag.
 */
static int _SET_MURDEROUS(RS_STACKDATA *stack, int argc) {
    if (argc != 2) {
        return 0;
    }
    action_info.chara->murderous = GetStackInt(stack++);
    action_info.chara->murderous_time = GetStackInt(stack);
    return 1;
}

/**
 * Returns the distance to the current target.
 */
static int _GET_TRG_DISTANCE(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, action_info.chara->GetTargetDist(nowScene));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SET_TRG_ANGLE__FP12RS_STACKDATAi);
/**
 * Sets the guard flag of the action character.
 */
static int _SET_GUARD_FLAG(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->guard_flag = GetStackInt(stack);
    return 1;
}

/**
 * Sets the action character invulnerability timer.
 */
static int _SET_MUTEKI(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->muteki_time = GetStackInt(stack);
    return 1;
}

/**
 * Returns the kind of object held by the action character.
 */
static int _CHECK_HAND_OBJ(RS_STACKDATA *stack, int argc) {
    SetStack(stack, action_info.chara->hold_type);
    return 1;
}

/**
 * Runs the item-use action and returns its result.
 */
static int _SET_ITEM_USED(RS_STACKDATA *stack, int argc) {
    SetStack(stack, action_info.chara->UsedItemAction());
    return 1;
}

/**
 * Throws the item object held by the action character.
 */
static int _THROW_HAND_OBJECT(RS_STACKDATA *stack, int argc) {
    action_info.chara->ThrowItemObject();
    return 1;
}

/**
 * Checks a named enemy for catching or kicking.
 */
static int _CHECK_CATCH(RS_STACKDATA *stack, int argc) {
    char *name;
    int   target;
    int   caught;

    name = GetStackString(stack++);
    if (argc == 1) {
        action_info.chara->CheckEnemyCatch(name);
    }
    if (argc == 3) {
        target = GetStackInt(stack++);
        caught = action_info.chara->CheckKeri(name, target);
        SetStack(stack, caught);
    }
    return 1;
}

/**
 * Releases or throws the held object or monster.
 */
static int _RELEASE_OBJ(RS_STACKDATA *stack, int argc) {
    float            distance;
    DNG_BATTLE_AREA *input;
    const float      height = 0.6f;
    const float      gravity = 10.0f;
    sceVu0FVECTOR    held_pos;
    sceVu0FVECTOR    start_pos;
    sceVu0FVECTOR    direction;
    sceVu0FVECTOR    target_pos;
    int              throw_it = 0;

    if (argc == 1) {
        throw_it = GetStackInt(stack);
    }
    if (nowScene != NULL && (input = &nowScene->battle_area) != NULL &&
        !(input->pause_flag & 0x2000)) {
        action_info.chara->Show(1, 1);
    }
    if (action_info.chara->hold_type == 1 && throw_it == 0) {
        action_info.chara->RemoveThrowItem();
    }
    int chara_no = 0x18;
    if (action_info.chara->hold_type == 3) {
        do {
            CActionChara *held = (CActionChara *)nowScene->GetCharacter(chara_no);
            if (held != NULL && held->catch_state == 1) {
                held->catch_frame->GetWorldPosition0(held_pos);
                held->CObjectFrame::frame->DeleteReference();
                held->SetPosition(held_pos);
                if (throw_it == 0) {
                    held->catch_frame = NULL;
                    held->catch_state = 0;
                    held->no_hit_time = 5;
                    ((CActiveMonster *)held)->req_prog = 0x4B0;
                } else {

                    action_info.chara->GetPosition(start_pos);
                    sceVu0CopyVector(direction, action_info.chara->front_vec);
                    direction[3] = 1.0f;
    distance = 100.0f;
                    if (action_info.chara->lock_on != 0) {
                        CActionChara *target = (CActionChara *)nowScene->GetCharacter(
                            action_info.chara->target_no);
                        if (target != NULL) {
                            target->GetEntryObjectPos(0, 0, target_pos);
                            start_pos[3] = 1.0f;
                            target_pos[3] = 1.0f;
                            distance = mgDistVector(start_pos, target_pos);
                            if (distance > 120.0f) {
                                distance = 120.0f;
                            }
                        }
                    }
                    sceVu0Normalize(direction, direction);
                    sceVu0ScaleVectorXYZ(direction, direction, distance);
                    sceVu0AddVector(direction, direction, start_pos);

                    ParabolicInitialVector(held->blow_vec, start_pos, direction, height, gravity);
                    held->catch_frame = NULL;
                    held->catch_state = 2;
                    held->no_hit_time = 5;
                    held->damage_req = 6;
                    sceVu0FVECTOR offset = { 0.0f, 0.0f, 0.0f, 1.0f };
                    sceVu0CopyVector(held->velocity, offset);
                    action_info.chara->release_timing = 2;
                }
            }
            chara_no++;
        } while (chara_no <= 0x2F);
    }
    if (action_info.chara->hold_type == 4) {
        action_info.chara->hold_parts = 0;
        action_info.chara->hold_frame = 0;
        action_info.chara->release_timing = 3;
    }
    action_info.chara->hold_type = 0;
    return 1;
}

/**
 * Starts the equipped ring magic and its collision primitive.
 */
static void ShotMonicaMagic(float *position, float *direction, float scale) {
    float     tint;
    CColPrim *prim;
    char     *effect_name;
    char     *unused_name;
    int       effect_power;

    (&GetBattleCharaInfo()->equip[1])
    ->GetEffectReadType(&effect_name, &unused_name, &effect_power);
    action_info.chara->effect_man->CreateEffSpt(effect_name, 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    action_info.chara->effect_man->SetScriptVect2(direction, 0, -1);
    tint = (float)effect_power / 255.0f;
    tint *= 1.5f;
    action_info.chara->effect_man->SetValue(0, tint, -1, -1);
    action_info.chara->effect_man->SetScriptTargetId(action_info.chara->target_no, -1, -1);
    prim = ColPrimMan.GetPrim();
    if (prim != NULL) {
        prim->SetDamage("\203\202\203j\203J\226\202\226@", 0);
        prim->range = 500.0f;
        SetDamageParam(prim, 1);
        prim->damage = (int)((float)prim->damage * scale);
        action_info.chara->effect_man->SetColPrim(prim, -1, -1);
        calcWeaponParam2(5, prim->param->hit_count);
    }
    sndSePlay(action_info.chara->se_bank, 13, 0);
}

/**
 * Fires a normal gun projectile and its muzzle effect.
 */
static void ShotNormalGun(float *position, float *direction) {
    CColPrim *prim;

    action_info.chara->effect_man->CreateEffSpt("\217e\222e", 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    sceVu0ScaleVector(direction, direction, 20.0f);
    action_info.chara->effect_man->SetScriptVect2(direction, 0, -1);
    prim = ColPrimMan.GetPrim();
    if (prim != NULL) {
        prim->SetDamage("\203\206\203\212\203X\217e\215U\214\202", 0);
        prim->range = 300.0f;
        SetDamageParam(prim, 1);
        action_info.chara->effect_man->SetColPrim(prim, -1, -1);
        calcWeaponParam2(1, prim->param->hit_count);
    }
    action_info.chara->effect_man->CreateEffSpt("\203}\203Y\203\213\203t\203\211\203b\203V\203\205", 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    sndSePlay(action_info.chara->se_bank, 5, 0);
}

/**
 * Starts a machine-gun projectile and its collision primitive.
 */
static void ShotMachineGun(float *position, float *direction, char *damage_name, float damage) {
    CColPrim     *prim;
    int           col_prim_id;
    CActionChara *owner;

    MachineGun.Set(position, direction);
    prim = ColPrimMan.GetPrim();
    col_prim_id = -1;
    if (prim != NULL) {
        prim->SetDamage(damage_name, 0);
        prim->SetCoord(position, position, 5.0f);
        prim->range = damage;
        SetDamageParam(prim, 1);
        col_prim_id = prim->id;
        calcWeaponParam2(1, prim->param->hit_count);
    }
    MachineGun.col_prim_id[MachineGun.index] = col_prim_id;
    action_info.chara->effect_man->CreateEffSpt("\203}\203Y\203\213\203t\203\211\203b\203V\203\205", 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    owner = action_info.chara;
    if (owner->loop_se != NULL) {
        owner->loop_se->SeLoopPlayStop(owner->se_bank, 5, 5, 13);
    }
}

/**
 * Launches a grenade toward the given direction.
 */
static void ShotGrenadGun(float *position, float *direction) {
    CRocketLauncher *launcher;
    sceVu0FVECTOR    muzzle;
    CColPrim        *prim;
    int              col_prim_id;

    sceVu0ScaleVector(muzzle, direction, 500.0f);
    sceVu0AddVector(muzzle, position, muzzle);
    direction[1] += 0.1f;
    launcher = RocketLauncher.Get();
    if (launcher != NULL) {
        launcher->SetPos(position, muzzle, direction);
        launcher->target_chara = action_info.chara->target_no;
        launcher->speed = 20.0f;
        launcher->homing_delay = 4;
        launcher->homing_time = 30;
        prim = ColPrimMan.GetPrim();
        col_prim_id = -1;
        if (prim != NULL) {
            prim->SetDamage("\203O\203\214\203l\201[\203hG", 0);
            prim->SetCoord(position, 5.0f);
            prim->range = 500.0f;
            SetDamageParam(prim, 1);
            col_prim_id = prim->id;
            calcWeaponParam2(1, prim->param->hit_count);
        }
        launcher->col_prim_id = col_prim_id;
    }
    action_info.chara->effect_man->CreateEffSpt("\203}\203Y\203\213\203t\203\211\203b\203V\203\205", 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    sndSePlay(action_info.chara->se_bank, 5, 0);
}

/**
 * Starts a laser projectile with the requested visual type.
 */
static void ShotLaserGun(float *position, float *direction, int type) {
    CLaserGun    *laser;
    sceVu0FVECTOR muzzle;
    sceVu0FVECTOR target;
    CColPrim     *prim;
    int           col_prim_id;
    float         color_d;
    float         color_b;
    float         color_c;
    float         color_a;

    sceVu0ScaleVector(muzzle, direction, 20.0f);
    sceVu0AddVector(muzzle, position, muzzle);
    sceVu0ScaleVector(target, direction, 500.0f);
    sceVu0AddVector(target, muzzle, target);
    laser = LaserGun.Get();
    if (laser != NULL) {
        laser->SetPos(muzzle, target, direction);
        laser->target_chara = action_info.chara->target_no;
        laser->speed = 30.0f;
        laser->homing_delay = 99999;
        laser->homing_time = 0;
        laser->SetVisualCode(type);
        prim = ColPrimMan.GetPrim();
        col_prim_id = -1;
        if (prim != NULL) {
            prim->SetDamage("\203\214\201[\203U\201[G", 0);
            prim->SetCoord(muzzle, 5.0f);
            prim->range = 500.0f;
            SetDamageParam(prim, 1);
            col_prim_id = prim->id;
            calcWeaponParam2(1, prim->param->hit_count);
        }
        laser->col_prim_id = col_prim_id;
    }
    action_info.chara->effect_man->CreateEffSpt("\203}\203Y\203\213\203t\203\211\203b\203V\203\205", 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    action_info.chara->effect_man->SetValue(0, 1, 0, -1);
    if (type == 0) {
        color_a = 64.0f;
        color_b = 128.0f;
        color_c = color_a;
        color_d = color_b;
    }
    if (type == 1) {
        color_a = 64.0f;
        color_c = 128.0f;
        color_b = color_a;
        color_d = color_c;
    }
    if (type == 2) {
        color_a = 128.0f;
        color_b = 32.0f;
        color_d = 180.0f;
        color_c = color_a;
    }
    action_info.chara->effect_man->SetValue(1, color_a, 0, -1);
    action_info.chara->effect_man->SetValue(2, color_b, 0, -1);
    action_info.chara->effect_man->SetValue(3, color_c, 0, -1);
    action_info.chara->effect_man->SetValue(4, color_d, 0, -1);
    sndSePlay(action_info.chara->se_bank, 5, 0);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SET_SHOT__FP12RS_STACKDATAi);
/**
 * Starts a charged magic-sword projectile and its collision primitive.
 */
static int _SET_SPECIAL_SHOT(RS_STACKDATA *stack, int argc) {
    char             *object_name;
    CBattleCharaInfo *info;
    CColPrim         *prim;
    sceVu0FVECTOR     facing;
    sceVu0FVECTOR     position;
    sceVu0FVECTOR     direction;
    mgCFrame         *object;

    if (argc != 1) {
        return 0;
    }
    object_name = GetStackString(stack);
    info = GetBattleCharaInfo();
    if (info->GetMagicSwordCounterNow() <= 0) {
        return 1;
    }
    if (action_info.chara->shot_wait > 0) {
        return 1;
    }
    action_info.chara->shot_wait = 5;
    object = action_info.chara->SearchObject(object_name);
    if (object == NULL) {
        return 0;
    }
    sceVu0CopyVector(facing, action_info.chara->front_vec);
    object->GetWorldPosition0(position);
    sceVu0CopyVector(direction, action_info.chara->front_vec);
    char *effects[4] = {
        "\203\202\203j\203J\226\202\226@\201|\211\316",
        "\203\202\203j\203J\226\202\226@\201|\225X",
        "\203\202\203j\203J\226\202\226@\201|\227\213",
        "\203\202\203j\203J\226\202\226@\201|\225\227",
    };
    action_info.chara->effect_man->CreateEffSpt(effects[info->GetMagicSwordElem()], 0, 0);
    action_info.chara->effect_man->SetScriptVect1(position, 0, -1);
    action_info.chara->effect_man->SetScriptVect2(direction, 0, -1);
    action_info.chara->effect_man->SetValue(0, 0.0f, -1, -1);
    action_info.chara->effect_man->SetScriptTargetId(action_info.chara->target_no, -1, -1);
    prim = ColPrimMan.GetPrim();
    if (prim != NULL) {
        prim->SetDamage("\203\202\203j\203J\226\202\226@", 0);
        prim->damage = info->GetMagicSwordPow();
        prim->element[info->GetMagicSwordElem()] = 100;
        prim->element[info->GetMagicSwordElem()] = 100;
        action_info.chara->effect_man->SetColPrim(prim, -1, -1);
    }
    info->ClearMagicSwordPow();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/actscript", _SHOT__FP12RS_STACKDATAi);
/**
 * Returns the world position of a named character object.
 */
static int _GET_OBJECT_POS(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *next;
    sceVu0FVECTOR pos;
    mgCFrame     *object;

    if (argc != 4) {
        return 0;
    }
    next = stack + 1;
    object = action_info.chara->SearchObject(GetStackString(stack));
    if (object == NULL) {
        return 0;
    }
    object->GetWorldPosition0(pos);
    SetStack(next++, pos[0]);
    SetStack(next++, pos[1]);
    SetStack(next, pos[2]);
    return 1;
}

/**
 * Enables directional gun aiming.
 */
static int _SET_DIR_GUN(RS_STACKDATA *stack, int argc) {
    action_info.chara->dir_gun = 1;
    return 1;
}

/**
 * Returns the integer health ratio as a floating stack value.
 */
static int _GET_NOW_HP_RATE(RS_STACKDATA *stack, int argc) {
    CBattleCharaInfo *info;
    int               now_hp;
    int               rate;

    if (argc != 1) {
        return 0;
    }
    info = GetBattleCharaInfo();
    now_hp = info->GetNowHp_i();
    rate = now_hp / info->GetMaxHp_i();
    SetStack(stack, (float)rate);
    return 1;
}

/**
 * Sets player health to five percent of its maximum.
 */
static int _SET_BOMB(RS_STACKDATA *stack, int argc) {
    GetBattleCharaInfo()->SetHpRate(0.05f);
    return 1;
}

/**
 * Returns the equipped weapon model number.
 */
static int _GET_ACTION_CODE(RS_STACKDATA *stack, int argc) {
    int model_no;

    if (argc != 1) {
        return 0;
    }
    model_no = GetBattleCharaInfo()->GetEquipTablePtr(0)->GetModelNo();
    SetStack(stack, model_no);
    return 1;
}

/**
 * Returns the requested weapon attack parameter.
 */
static int _GET_ATTK_POINT(RS_STACKDATA *stack, int argc) {
    int                  index;
    BATTLE_WEAPON_PARAM *slots;

    if (argc != 2) {
        return 0;
    }
    index = GetStackInt(stack++);
    slots = GetBattleCharaInfo()->weapon_param;
    SetStack(stack, slots[index].status[0]);
    return 1;
}

/**
 * Returns the colour associated with the equipped ring effect.
 */
static int _GET_RING_COLOR(RS_STACKDATA *stack, int argc) {
    char *effect_name;
    char *unused_name;
    int   effect_power;

    if (argc != 3) {
        return 0;
    }
    int type = (&GetBattleCharaInfo()->equip[1])
    ->GetEffectReadType(&effect_name, &unused_name, &effect_power);
    if (type < 0 || type > 3) {
        return 0;
    }
    int colors[4][3] = { { 255, 64, 64 }, { 128, 255, 255 }, { 128, 64, 255 }, { 96, 255, 160 } };
    SetStack(stack++, colors[type][0]);
    SetStack(stack++, colors[type][1]);
    SetStack(stack, colors[type][2]);
    return 1;
}

/**
 * Sets a named motion with optional speed, flags and target character.
 */
static int _SET_MOS(RS_STACKDATA *stack, int argc) {
    char         *motion;
    char         *chara_name;
    int           flag;
    float         speed;
    CActionChara *target;

    motion = NULL;
    chara_name = NULL;
    flag = 0;
    speed = -1.0f;
    if (argc <= 0 || argc > 4) {
        return 0;
    }
    if (argc > 0) {
        motion = GetStackString(stack++);
    }
    if (argc >= 2) {
        speed = GetStackFloat(stack++);
    }
    if (argc >= 3) {
        flag = GetStackInt(stack++);
    }
    if (argc == 4) {
        chara_name = GetStackString(stack);
    }
    if (motion == NULL) {
        return 0;
    }
    target = (CActionChara *)action_info.chara;
    if (chara_name != NULL) {
        target = target->SearchChara(chara_name);
        if (target == NULL) {
            return 0;
        }
    }
    target->SetMotion(motion, flag, 1);
    if (speed > 0.0f) {
        target->SetStep(speed);
    }
    return 1;
}

/**
 * Returns whether the current or named motion has ended.
 */
static int _CHECK_MOS_END(RS_STACKDATA *stack, int argc) {
    float result;
    char *name;

    if (argc == 1) {
        result = action_info.chara->CheckMotionEnd(0);
    }
    if (argc == 2) {
        name = GetStackString(stack + 1);
        if (name == NULL) {
            return 0;
        }
        result = action_info.chara->CheckMotionEnd(name);
    }
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the current frame wait of the current or named motion.
 */
static int _NOW_MOS_WAIT(RS_STACKDATA *stack, int argc) {
    float result;
    char *name;

    if (argc == 1) {
        result = action_info.chara->GetNowFrameWait(0);
    }
    if (argc == 2) {
        name = GetStackString(stack + 1);
        if (name == NULL) {
            return 0;
        }
        result = action_info.chara->GetNowFrameWait(name);
    }
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the motion transition wait.
 */
static int _NOW_MOS_CHGWAIT(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, action_info.chara->GetChgStepWait());
    return 1;
}

/**
 * Returns the current or named motion status.
 */
static int _GET_MOS_STATUS(RS_STACKDATA *stack, int argc) {
    int   status;
    char *name;

    if (argc == 1) {
        status = action_info.chara->GetMotionStatus(NULL);
    }
    if (argc == 2) {
        name = GetStackString(stack + 1);
        if (name == NULL) {
            return 0;
        }
        status = action_info.chara->GetMotionStatus(name);
    }
    SetStack(stack, status);
    return 1;
}

/**
 * Sets motion blending speed and completes the blend at unit speed.
 */
static int _SET_XCHG_STEP(RS_STACKDATA *stack, int argc) {
    float         value;
    CActionChara *chara;

    if (argc != 1) {
        return 0;
    }
    value = GetStackFloat(stack);
    chara = action_info.chara;
    chara->blend_speed = value;
    if (value >= 1.0f) {
        chara->blend = 1.0f;
    }
    return 1;
}

/**
 * Sets the motion frame step.
 */
static int _SET_MOS_STEP(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->SetStep(GetStackFloat(stack));
    return 1;
}

/**
 * Requests advancement of the motion sequence.
 */
static int _TRG_ON_MOS(RS_STACKDATA *stack, int argc) {
    action_info.chara->seq_advance = 1;
    return 1;
}

/**
 * Resets the action character motion.
 */
static int _RESET_MOS(RS_STACKDATA *stack, int argc) {
    action_info.chara->ResetMotion();
    return 1;
}

/**
 * Sets the default motion name.
 */
static int _SET_DEFAULT_MOS(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    action_info.chara->default_motion = GetStackString(stack);
    return 1;
}

/**
 * Reduces the default motion step when the player attribute is set.
 */
static int _SET_NEBA2(RS_STACKDATA *stack, int argc) {
    if ((GetBattleCharaInfo())->GetAttr() & 2) {
        action_info.chara->SetStep(0.7f * action_info.chara->GetDefaultStep());
    }
    return 1;
}

/**
 * Starts a named character effect and optionally returns its slot.
 */
static int _ESM_CREATE(RS_STACKDATA *stack, int argc) {
    char *name;
    int   id;

    if (action_info.chara->effect_man == NULL) {
        return 0;
    }
    name = GetStackString(stack++);
    switch (argc) {
        case 1:
            action_info.chara->effect_man->CreateEffSpt(name, 0, 0);
            break;
        case 2: {
                id = action_info.chara->effect_man->CreateEffSpt(name, 0, 1);
                if (id <= -1) {
                    return 0;
                }
                SetStack(stack, id);
                break;
            }
    }
    return 1;
}

/**
 * Sets the first work vector of a character effect.
 */
static int _ESM_SET_VECT1(RS_STACKDATA *stack, int argc) {
    int           index;
    sceVu0FVECTOR vect;

    if (action_info.chara->effect_man == NULL) {
        return 0;
    }
    index = GetStackInt(stack++);
    vect[0] = GetStackFloat(stack++);
    vect[1] = GetStackFloat(stack++);
    vect[2] = GetStackFloat(stack);
    vect[3] = 1.0f;
    if (index >= 0) {
        return action_info.chara->effect_man->SetScriptVect1(vect, 0, index);
    }
    return action_info.chara->effect_man->SetScriptVect1(vect, 0, -1);
}

/**
 * Sets the second work vector of a character effect.
 */
static int _ESM_SET_VECT2(RS_STACKDATA *stack, int argc) {
    int           index;
    sceVu0FVECTOR vect;

    if (action_info.chara->effect_man == NULL) {
        return 0;
    }
    index = GetStackInt(stack++);
    vect[0] = GetStackFloat(stack++);
    vect[1] = GetStackFloat(stack++);
    vect[2] = GetStackFloat(stack);
    vect[3] = 1.0f;
    if (index >= 0) {
        return action_info.chara->effect_man->SetScriptVect2(vect, 0, index);
    }
    return action_info.chara->effect_man->SetScriptVect2(vect, 0, -1);
}

/**
 * Selects the finish program of a character effect.
 */
static int _ESM_FINISH(RS_STACKDATA *stack, int argc) {
    CEffectScriptMan *effect_script;
    int               effect_id;

    effect_id = GetStackInt(stack);
    if (effect_id < 0) {
        return 0;
    }
    effect_script = action_info.chara->effect_man;
    if (effect_script == NULL) {
        return 0;
    }
    effect_script->SetScriptProgNo(0x12C, 0, effect_id);
    return 1;
}

/**
 * Deletes a character effect from its slot.
 */
static int _ESM_DELETE(RS_STACKDATA *stack, int argc) {
    CEffectScriptMan *effect_script;
    int               effect_id;

    effect_id = GetStackInt(stack);
    if (effect_id < 0) {
        return 0;
    }
    effect_script = action_info.chara->effect_man;
    if (effect_script == NULL) {
        return 0;
    }
    effect_script->DeleteEffSpt(0, effect_id);
    return 1;
}

/**
 * Sets an integer or floating value slot in a character effect.
 */
static int _ESM_SET_VALUE(RS_STACKDATA *stack, int argc) {
    int prog_no;
    int value_no;
    int result;

    if (argc != 3) {
        return 0;
    }
    prog_no = GetStackInt(stack++);
    value_no = GetStackInt(stack++);
    switch (stack->type) {
        case RS_INT:
            result =
                action_info.chara->effect_man->SetValue(value_no, GetStackInt(stack), 0, prog_no);
            break;
        case RS_FLOAT:
            result =
                action_info.chara->effect_man->SetValue(value_no, GetStackFloat(stack), 0, prog_no);
            break;
        default:
            return 0;
    }
    return result;
}

int SetActionScript(CRunScript *script, char *program, mgCMemory *memory) {
    RS_STACKDATA *stack;
    RS_CALLDATA  *call_data;

    stack = (RS_STACKDATA *)memory->Alloc(64);
    call_data = (RS_CALLDATA *)memory->Alloc(384);
    script->load((RS_PROG_HEADER *)program, stack, 0x80, call_data,
        0x200);
    script->ext_func(ext_func, 0x100);
    return 1;
}

void SetActionExtendTable(void) {
    int i;
    int j;

    for (i = 0; i < 256; i++) {
        ext_func[i] = NULL;
    }
    for (i = 0;; i++) {
        if (ext_func_info[i].func == NULL) {
            break;
        }
        if (0 < i) {
            j = 0;
            do {
                if (ext_func_info[i].no == ext_func_info[j].no) {
                    printf("chr]same ext_func_no!!!\n");
                    while (1) {
                    }
                }
                j++;
            } while (j < i);
        }
        if (ext_func_info[i].no < 0 || ext_func_info[i].no >= 256) {
            printf("ext func over!!");
        } else {
            ext_func[ext_func_info[i].no] = ext_func_info[i].func;
        }
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1181__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1417__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1597__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1645__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1774__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1118__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1202__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1211__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1304__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1450__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1458__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1459__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1460__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1487__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1517__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1579__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1580__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1581__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1593__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1594__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1595__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1596__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1637__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1638__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1639__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1640__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1641__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1642__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1643__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1644__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1725__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1726__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1727__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1728__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1729__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_1730__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_2004__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actscript", at_2005__3__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(nowScene__2, 0x4);
INCLUDE_BSS(LastCInfo2__2, 0x4);
INCLUDE_BSS(sw_1617, 0x4);
INCLUDE_BSS(init_1618, 0x4);
INCLUDE_BSS(canon_slot_1620, 0x4);
INCLUDE_BSS(init_1621, 0x4);
INCLUDE_BSS(cnt_1661, 0x4);
INCLUDE_BSS(init_1662, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(action_info, 0x10);
INCLUDE_BSS(ext_func__3, 0x400);
