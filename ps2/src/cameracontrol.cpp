#include "common.h"
#include "cameracontrol.hpp"
#include "collision.hpp"
#include "gameutil.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "padcontrol.hpp"

#include <cmath>

// Code (.text)
void CameraCtrlParam::SetFixHeight(float height) {
    max_height = height;
    min_height = height;
    near_height = height;
    far_height = height;
    rest_max_height = height;
    rest_min_height = height;
}

void CameraCtrlParam::SetFixDist(float distance) {
    max_dist = distance;
    min_dist = distance;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/cameracontrol", __ct__14CCameraControlFv);
CameraCtrlParam *CCameraControl::GetActiveParam() {
    return &param[active_param];
}

void CCameraControl::SetRotCameraCancel(s32 mask) {
    rot_cancel = mask;
}

void CCameraControl::BitSetRotCameraCancel(s32 mask) {
    rot_cancel |= mask;
}

void CCameraControl::BitResetRotCameraCancel(s32 mask) {
    rot_cancel &= ~mask;
}

void CCameraControl::InitStatus() {
    rot_cancel = 0;
    rot_back = 0;
    rot_back_angle = 0.0f;
    mgZeroVector(dir_offset);
}

void CCameraControl::ControlOn() {
    CameraCtrlParam *p;

    if (control_on == 0) {
        *(u_long128 *)next_ref = *(u_long128 *)ref;
        *(u_long128 *)next_pos = *(u_long128 *)pos;
        p = GetActiveParam();
        p->height = next_pos[1] - next_ref[1];
        if (p->height < p->min_height) {
            p->height = p->min_height;
        }
        if (p->height > p->max_height) {
            p->height = p->max_height;
        }
    }
    control_on = 1;
}

void CCameraControl::ControlOff() {
    control_on = 0;
}

void CCameraControl::Stay() {
    if (control_on == 0) {
        mgCCameraFollow::Stay();
    } else {
        mgCCamera::Stay();
    }
}

void CCameraControl::Step(int steps) {
    sceVu0FVECTOR offset;

    if (control_on == 0) {
        mgCCameraFollow::Step(steps);
        return;
    }
    if (steps < 0) {
        if (rot_back != 0) {
            SetRotate(rot_back_angle);
        }
    }
    mgCCamera::Step(steps);
    sceVu0SubVector(offset, next_ref, next_pos);
    distance = mgDistVector(offset);
    height = -offset[1];
    next_angle = atan2f(-offset[0], -offset[2]);
    sceVu0SubVector(offset, ref, pos);
    angle = atan2f(-offset[0], -offset[2]);
}

void CCameraControl::MoveCamera(CPadControl *pad, float *rot, CCPoly *polys, int poly_count) {
    Control control;

    control.height = 0.0f;
    control.rot = 0.0f;
    control.rot_back = 0;
    if (pad != NULL) {
        float turn = 0.0f;

        if (!(rot_cancel & CAMERA_ROT_CANCEL_ANALOG)) {
            turn = 0.05f * -pad->Analog(6);
        }
        if (!(rot_cancel & CAMERA_ROT_CANCEL_BUTTON)) {
            if (pad->Btn(3) != 0) {
                turn = 0.05f;
            }
            if (pad->Btn(2) != 0) {
                turn = -0.05f;
            }
        }
        if (rot_reverse != 0) {
            turn = -turn;
        }
        control.rot = turn;
        control.height = 2.0f * -pad->Analog(7);
        bool rot_back_requested = pad->Btn(4) != 0;
        if (rot_back_requested == 0) {
            rot_back_requested = pad->Btn(1) != 0;
        }
        control.rot_back = rot_back_requested;
    }
    MoveCamera(&control, rot, polys, poly_count);
}

void CCameraControl::MoveCamera(Control *control, float *rot, CCPoly *polys, int poly_count) {
    CameraCtrlParam *param;
    sceVu0FVECTOR follow;
    sceVu0FVECTOR follow_offset;
    sceVu0FVECTOR to_target;
    sceVu0FVECTOR direction;
    sceVu0FVECTOR correction;
    float distance;
    float turn;
    float zoom;

    if (control_on == 0) {
        return;
    }
    param = GetActiveParam();
    GetFollow(follow);
    GetFollowOffset(follow_offset);
    sceVu0AddVector(next_ref, follow, follow_offset);
    sceVu0SubVector(to_target, next_ref, next_pos);
    to_target[1] = 0.0f;
    sceVu0Normalize(direction, to_target);
    mgZeroVector(correction);
    distance = mgDistVector(to_target);
    if (distance <= 0.0f) {
        distance = 1.0f;
        to_target[2] = 1.0f;
        direction[2] = 1.0f;
    }
    if (distance < param->min_dist) {
        sceVu0ScaleVector(correction, direction, distance - param->min_dist);
    }
    if (distance > param->max_dist) {
        sceVu0ScaleVector(correction, direction, distance - param->max_dist);
    }
    {
        float effective;
        float min_dist = param->min_dist;
        effective = min_dist;
        if (distance < min_dist) {
            effective = distance;
        }
        turn = (control->rot * min_dist) / effective;
    }
    if (turn != 0.0f) {
        Rotate(turn);
    }
    zoom = control->height;
    param->height += zoom;
    if (param->height < param->min_height) {
        param->height = param->min_height;
    }
    if (param->height > param->max_height) {
        param->height = param->max_height;
    }
    if (zoom == 0.0f) {
        if (param->rest_min_height > param->height) {
            param->height += (param->rest_min_height - param->height) / 20.0f;
        }
        if (param->rest_max_height < param->height) {
            param->height += (param->rest_max_height - param->height) / 20.0f;
        }
    }
    {
        float near_distance = param->min_dist;
        float far_distance = param->max_dist;
        float near_height = param->near_height;
        float far_height = param->far_height;
        float ratio = (distance - near_distance) / (far_distance - near_distance);
        correction[1] = near_height + ratio * (far_height - near_height);
    }
    next_pos[1] = next_ref[1] + param->height;
    mgAddVector(next_pos, correction);
    if (control->rot_back != 0 && !(rot_cancel & CAMERA_ROT_CANCEL_ROT_BACK)) {
        RotBack(rot[1] - 3.1415927f);
    }
    if (rot_back != 0) {
        float angle = mgAngleInterpolate(GetAngle(), rot_back_angle, 1.0f, 0);
        SetRotate(angle);
        if (mgAngleCmp(angle, rot_back_angle, 0.1f) == 0) {
            rot_back = 0;
        }
    }
    if (param->no_check == 0) {
        CheckGround(polys, poly_count);
        if (turn != 0.0f || (rot_cancel & CAMERA_ROT_CANCEL_AUTO_MOVE)) {
            CheckCollision(polys, poly_count);
            return;
        }
        CheckCollision(polys, poly_count);
        AutoMove(polys, poly_count);
    }
}

void CCameraControl::Rotate(float angle) {
    sceVu0FVECTOR offset;
    sceVu0FMATRIX matrix;

    sceVu0SubVector(offset, next_pos, next_ref);
    offset[3] = 0.0f;
    mgUnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, mgAngleLimit(angle));
    sceVu0ApplyMatrix(offset, matrix, offset);
    sceVu0AddVector(next_pos, next_ref, offset);
}

#ifdef NONMATCHING
void CCameraControl::SetRotate(float angle) {
    static sceVu0FVECTOR base;
    sceVu0FVECTOR offset;
    sceVu0FMATRIX matrix;
    float distance;
    float height;

    distance = mgDistVectorXZ(next_ref, next_pos);

    height = next_pos[1] - next_ref[1];
    sceVu0CopyVector(offset, base);
    offset[1] = height;
    offset[2] = distance;
    mgUnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, mgAngleLimit(angle));
    sceVu0ApplyMatrix(offset, matrix, offset);
    sceVu0AddVector(next_pos, next_ref, offset);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/cameracontrol", SetRotate__14CCameraControlFf);
#endif

void CCameraControl::SetHeight(float height) {
    CameraCtrlParam *p;

    if (control_on == 0) {
        mgCCameraFollow::SetHeight(height);
    }

    p = GetActiveParam();
    next_pos[1] = next_ref[1] + height;
    p->height = height;
}

void CCameraControl::RotBack(float angle) {
    rot_back = 1;
    rot_back_angle = angle;
}

void CCameraControl::CancelRotBack() {
    rot_back = 0;
}

void CCameraControl::SetCheckRef(float *ref) {
    check_ref_on = 1;
    *(u_long128 *)check_ref = *(u_long128 *)ref;
}

void CCameraControl::SetCheckRef(float x, float y, float z) {
    sceVu0FVECTOR ref = {0.0f, 0.0f, 0.0f, 1.0f};

    ref[0] = x;
    ref[1] = y;
    ref[2] = z;
    SetCheckRef(ref);
}

void CCameraControl::CheckCollision(CCPoly *polys, int poly_count) {
    sceVu0FVECTOR to_camera;
    sceVu0FVECTOR side_dir;
    sceVu0FVECTOR view_dir;
    sceVu0FVECTOR hit;
    sceVu0FVECTOR unused_hit;
    sceVu0FVECTOR view_end;
    sceVu0FVECTOR side_end;
    sceVu0FVECTOR target;
    sceVu0FVECTOR margin;
    sceVu0FVECTOR push;
    int hit_index;
    int slid;
    float old_dist;

    GetActiveParam();
    if (check_ref_on != 0) {
        *(u_long128 *)target = *(u_long128 *)check_ref;
        target[1] += follow_offset[1];
    } else {
        *(u_long128 *)target = *(u_long128 *)next_ref;
    }
    sceVu0SubVector(to_camera, target, next_pos);
    sceVu0Normalize(view_dir, to_camera);
    sceVu0SubVector(view_end, target, to_camera);
    to_camera[1] = 0.0f;
    sceVu0SubVector(side_end, target, to_camera);
    sceVu0Normalize(side_dir, to_camera);
    mgDistVector(to_camera);
    sceVu0ScaleVector(margin, side_dir, 5.0f);
    sceVu0SubVector(side_end, side_end, margin);
    sceVu0ScaleVector(margin, view_dir, 5.0f);
    sceVu0SubVector(view_end, view_end, margin);
    slid = 0;
    hit_index = CheckHit(polys, poly_count, target, view_end, hit, 1, 0);
    if (hit_index < 0) {
        hit_index = -1;
        slid = 1;
        *(u_long128 *)hit = *(u_long128 *)unused_hit;
        *(u_long128 *)view_end = *(u_long128 *)side_end;
    }
    if (hit_index >= 0) {
        sceVu0SubVector(push, target, view_end);
        sceVu0Normalize(push, push);
        sceVu0ScaleVector(push, push, 5.0f);
        mgAddVector(hit, push);
        if (slid != 0) {
            old_dist = mgDistVectorXZ(next_pos, target);
            float ratio = mgDistVectorXZ(hit, target) / old_dist;
            hit[1] += ratio * (next_pos[1] - target[1]);
        }
        *(u_long128 *)next_pos = *(u_long128 *)hit;
        old_dist = mgDistVector(target, next_pos);
        if (old_dist - mgDistVector(target, hit) > 5.0f) {
            *(u_long128 *)pos = *(u_long128 *)hit;
        }
    }
}

int CCameraControl::AutoMove(CCPoly *polys, int poly_count) {
    sceVu0FVECTOR target;
    sceVu0FVECTOR to_target;
    sceVu0FVECTOR plane;
    sceVu0FVECTOR to_pos;
    sceVu0FVECTOR start_pos;
    sceVu0FVECTOR dir_copy;
    sceVu0FVECTOR ray_end;
    sceVu0FVECTOR rot_a;
    sceVu0FVECTOR rot_b;
    sceVu0FVECTOR ray_start;
    sceVu0FVECTOR candidate;
    sceVu0FMATRIX turn_a;
    sceVu0FMATRIX turn_b;
    sceVu0FVECTOR probe;
    int hit_count;
    int found;
    int step;
    int result;
    float unit = 1.0f;
    float radius;
    float margin;

    GetActiveParam();
    if (check_ref_on != 0) {
        *(u_long128 *)target = *(u_long128 *)check_ref;
        target[1] += follow_offset[1];
    } else {
        *(u_long128 *)target = *(u_long128 *)next_ref;
    }
    radius = 4.0f;
    sceVu0SubVector(to_target, target, next_pos);
    sceVu0SubVector(to_pos, target, pos);
    mgDistVectorXZ(to_pos);
    margin = 0.9f;
    margin *= unit;
    radius *= unit;
    *(u_long128 *)ray_end = *(u_long128 *)target;
    *(u_long128 *)start_pos = *(u_long128 *)next_pos;
    *(u_long128 *)dir_copy = *(u_long128 *)to_target;
    sceVu0ScaleVector(ray_start, dir_copy, margin);
    sceVu0SubVector(ray_start, target, ray_start);
    ray_start[3] = radius;
    if (CheckHitsPipe(polys, poly_count, ray_start, ray_end, 1, &hit_count, &plane, 0, 0) <= 0) {
        return 1;
    }
    *(u_long128 *)rot_a = *(u_long128 *)dir_copy;
    rot_a[3] = 0.0f;
    *(u_long128 *)rot_b = *(u_long128 *)dir_copy;
    rot_b[3] = 0.0f;
    mgUnitMatrix(turn_a);
    mgUnitMatrix(turn_b);
    sceVu0RotMatrixY(turn_a, turn_a, 0.01636246219277382f);
    sceVu0RotMatrixY(turn_b, turn_b, -0.01636246219277382f);
    found = 0;
    *(u_long128 *)candidate = *(u_long128 *)start_pos;
    for (step = 0; step < 0x20; step++) {
        sceVu0ApplyMatrix(rot_a, turn_a, rot_a);
        sceVu0SubVector(candidate, ray_end, rot_a);
        sceVu0ScaleVector(probe, rot_a, margin);
        sceVu0SubVector(probe, ray_end, probe);
        probe[3] = radius;
        if (CheckHitsPipe(polys, poly_count, probe, ray_end, 1, &hit_count, &plane, 0, 0) <= 0) {
            found = 1;
            break;
        }
        sceVu0ApplyMatrix(rot_b, turn_b, rot_b);
        sceVu0SubVector(candidate, ray_end, rot_b);
        sceVu0ScaleVector(probe, rot_b, margin);
        sceVu0SubVector(probe, ray_end, probe);
        probe[3] = radius;
        if (CheckHitsPipe(polys, poly_count, probe, ray_end, 1, &hit_count, &plane, 0, 0) <= 0) {
            found = 1;
            break;
        }
    }
    result = 0;
    if (found != 0) {
        result = 1;
        *(u_long128 *)next_pos = *(u_long128 *)candidate;
    }
    return result;
}

void CCameraControl::CheckGround(CCPoly *polys, int poly_count) {
    int ceiling_index;
    sceVu0FVECTOR target;
    int indices[0x20];
    sceVu0FVECTOR from;
    sceVu0FVECTOR line_high;
    sceVu0FVECTOR line_low;
    sceVu0FVECTOR hits[0x20];
    sceVu0FVECTOR hit_info;
    sceVu0FVECTOR floor_normal;
    sceVu0FVECTOR ceiling_normal;
    CameraCtrlParam *param;
    float top;
    float bottom;
    int floor_index;
    int count;
    int i;

    param = GetActiveParam();
    if (check_ref_on != 0) {
        *(u_long128 *)target = *(u_long128 *)check_ref;
        target[1] += follow_offset[1];
    } else {
        *(u_long128 *)target = *(u_long128 *)next_ref;
    }
    *(u_long128 *)from = *(u_long128 *)next_pos;
    *(u_long128 *)line_high = *(u_long128 *)next_pos;
    *(u_long128 *)line_low = *(u_long128 *)next_pos;
    line_high[1] = 20.0f + (target[1] + param->max_height);
    line_low[1] = (target[1] + param->min_height) - param->ground_space;
    top = line_high[1];
    bottom = line_low[1];
    if (CheckHit(polys, poly_count, from, line_high, hit_info, 1, 0) >= 0) {
        top = hit_info[1];
    }
    if (CheckHit(polys, poly_count, from, line_low, hit_info, 1, 0) >= 0) {
        bottom = hit_info[1];
    }
    ceiling_index = -1;
    floor_index = ceiling_index;
    line_high[1] = 2.0f + top;
    line_low[1] = bottom - 2.0f;
    count = CheckHits(polys, poly_count, line_high, line_low, 0x20, indices, hits, 1, 0);
    for (i = 0; i < count; i++) {
        if (hits[i][1] <= 1.0f + top) {
            sceVu0Normalize(floor_normal, polys[indices[i]].normal);
            if (next_pos[1] - hits[i][1] < param->ground_space) {
                if (floor_normal[1] > 0.5f) {
                    floor_index = i;
                    break;
                }
            }
        }
    }
    for (count--; count >= 0; count--) {
        if (hits[count][1] >= bottom - 1.0f) {
            sceVu0Normalize(ceiling_normal, polys[indices[count]].normal);
            if (hits[count][1] - next_pos[1] < 20.0f) {
                if (ceiling_normal[1] < -0.5f) {
                    ceiling_index = count;
                    break;
                }
            }
        }
    }
    if (floor_index >= 0 && ceiling_index >= 0) {
        next_pos[1] =
            0.5f * (param->ground_space + hits[floor_index][1] + hits[ceiling_index][1] - 20.0f);
    } else {
        if (floor_index >= 0) {
            next_pos[1] = param->ground_space + hits[floor_index][1];
        }
        if (ceiling_index >= 0) {
            next_pos[1] = hits[ceiling_index][1] - 20.0f;
        }
    }
}

void CCameraControl::GetCameraMatrix(float (*matrix)[4]) {
    sceVu0FVECTOR dir;
    sceVu0FVECTOR up;
    sceVu0SubVector(dir, ref, pos);
    mgAddVector(dir, dir_offset);
    dir[0] = dir[0];
    dir[1] = dir[1];
    dir[2] = dir[2];
    up[0] = dir[0] * dir[1];
    up[1] = -(dir[0] * dir[0] + dir[2] * dir[2]);
    up[2] = dir[1] * dir[2];
    up[3] = 1.0f;
    sceVu0Normalize(up, up);
    sceVu0Normalize(dir, dir);
    sceVu0CameraMatrix(matrix, pos, dir, up);
}

void CCameraControl::CopyParam(CCameraControl &dest) {
    CameraCtrlParam *src = GetActiveParam();
    CameraCtrlParam *dst = dest.GetActiveParam();
    dst->min_dist = src->min_dist;
    dst->max_dist = src->max_dist;
    dst->near_height = src->near_height;
    dst->far_height = src->far_height;
    dst->height = src->height;
    dst->max_height = src->max_height;
    dst->min_height = src->min_height;
    dst->rest_max_height = src->rest_max_height;
    dst->rest_min_height = src->rest_min_height;
    dst->ground_space = src->ground_space;
    dst->no_check = src->no_check;
    dest.rot_cancel = rot_cancel;
    *(u_long128 *)dest.follow_offset = *(u_long128 *)follow_offset;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/cameracontrol", at_396__3__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/cameracontrol", __vt__14CCameraControl__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_373__3, 0x10);
