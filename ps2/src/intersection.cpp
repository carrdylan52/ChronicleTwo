#include "common.h"
#include "intersection.hpp"
#include "mg_math.hpp"
#include "mg_drawenv.hpp"

#include <libvu0.h>

// Code (.text)
#ifdef NONMATCHING
int IntersectionPipeYPoly3(float *pipe, float (*poly)[4], float *normal, float (*hits)[4]) {
    sceVu0FVECTOR axis = {0.0f, normal[1], 0.0f, 0.0f};
    sceVu0FVECTOR tangent;
    sceVu0FVECTOR offset;
    sceVu0FVECTOR side[2];
    sceVu0FVECTOR flat_poly[3];
    sceVu0FVECTOR flat_pipe;
    float radius = pipe[3];
    int count = 0;

    sceVu0OuterProduct(tangent, normal, axis);
    sceVu0OuterProduct(offset, normal, tangent);
    sceVu0Normalize(offset, offset);
    sceVu0ScaleVector(offset, offset, radius);
    sceVu0AddVector(side[0], pipe, offset);
    sceVu0SubVector(side[1], pipe, offset);
    for (int i = 0; i < 2; i++) {
        if (mgCheckPointPoly3_XZ(side[i], poly[0], poly[1], poly[2]) != 0) {
            sceVu0CopyVector(hits[count++], side[i]);
        }
    }
    sceVu0CopyVector(flat_pipe, pipe);
    flat_pipe[1] = 0.0f;
    for (int i = 0; i < 3; i++) {
        sceVu0CopyVector(flat_poly[i], poly[i]);
        flat_poly[i][1] = 0.0f;
        if (mgDistVectorXZ2(pipe, poly[i]) <= radius * radius) {
            sceVu0CopyVector(hits[count++], poly[i]);
        }
    }
    for (int i = 0; i < 3; i++) {
        count += mgIntersectionSphereLine(flat_pipe, flat_poly[i], flat_poly[(i + 1) % 3], hits + count);
    }
    if (count > 0) {
        float plane_height = sceVu0InnerProduct(normal, poly[0]);
        float inverse_y = 1.0f / normal[1];
        for (int i = 0; i < count; i++) {
            hits[i][1] = inverse_y * ((plane_height - normal[0] * hits[i][0]) - normal[2] * hits[i][2]);
        }
    }
    return count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/intersection", IntersectionPipeYPoly3__FPfPA4_fPfPA4_f);
#endif

int IntersectionPipePoly3(float *pipe, float *direction, float (*poly)[4], float *normal, float (*hits)[4]) {
    sceVu0FMATRIX basis;
    sceVu0FMATRIX inverse;
    sceVu0FVECTOR unit;
    sceVu0FVECTOR helper;
    sceVu0FVECTOR transformed[5];
    sceVu0FVECTOR local_hits[11];

    sceVu0Normalize(basis[1], direction);
    basis[1][3] = 0.0f;
    mgZeroVector(helper);
    float abs_y = basis[1][1] < 0.0f ? -basis[1][1] : basis[1][1];
    if (abs_y < 0.9f) {
        helper[1] = 1.0f;
    } else {
        helper[2] = 1.0f;
    }
    sceVu0OuterProduct(basis[0], basis[1], helper);
    basis[0][3] = 0.0f;
    sceVu0OuterProduct(basis[2], basis[0], basis[1]);
    basis[2][3] = 0.0f;
    mgZeroVectorW(basis[3]);
    sceVu0TransposeMatrix(inverse, basis);
    for (int i = 0; i < 3; i++) {
        *(u_long128 *)transformed[i] = *(u_long128 *)poly[i];
        transformed[i][3] = 1.0f;
    }
    *(u_long128 *)transformed[3] = *(u_long128 *)normal;
    transformed[3][3] = 0.0f;
    *(u_long128 *)transformed[4] = *(u_long128 *)pipe;
    transformed[4][3] = 1.0f;
    mgApplyMatrixN(transformed, inverse, transformed, 5);
    mgPlaneNormal(transformed[3], transformed[0], transformed[1], transformed[2]);
    sceVu0Normalize(transformed[3], transformed[3]);
    transformed[4][3] = pipe[3];
    int count = IntersectionPipeYPoly3(transformed[4], transformed, transformed[3], local_hits);
    if (count == 0) {
        return 0;
    }
    for (int i = 0; i < count; i++) {
        local_hits[i][3] = 1.0f;
    }
    mgApplyMatrixN(hits, basis, local_hits, count);
    return count;
}

int IntersectionSpherePoly3(float *sphere, float (*poly)[4], float *normal, float *push) {
    sceVu0FVECTOR offset;
    sceVu0FVECTOR projection;
    sceVu0FVECTOR edge_hits[2];
    float radius = sphere[3];
    float radius2 = radius * radius;
    sceVu0SubVector(offset, sphere, poly[0]);
    float signed_distance = sceVu0InnerProduct(normal, offset);
    float distance = signed_distance < 0.0f ? -signed_distance : signed_distance;
    if (distance > radius) {
        return SPHERE_POLY3_NONE;
    }
    *(u_long128 *)offset = *(u_long128 *)normal;
    sceVu0ScaleVector(offset, offset, -signed_distance);
    sceVu0ScaleVector(push, offset, -1.0f);
    float scale = signed_distance < 0.0f ? -signed_distance : signed_distance;
    push[3] = scale;
    mgAddVector(offset, sphere);
    if (mgCheckPointPoly3_XYZ(offset, poly[0], poly[1], poly[2], normal) != 0) {
        return SPHERE_POLY3_FACE;
    }
    if (mgDistVector2(sphere, poly[0]) <= radius2) {
        return SPHERE_POLY3_VERTEX;
    }
    if (mgDistVector2(sphere, poly[1]) <= radius2) {
        return SPHERE_POLY3_VERTEX;
    }
    if (mgDistVector2(sphere, poly[2]) <= radius2) {
        return SPHERE_POLY3_VERTEX;
    }
    if (mgIntersectionSphereLine(sphere, poly[0], poly[1], edge_hits) > 0) {
        return SPHERE_POLY3_EDGE;
    }
    if (mgIntersectionSphereLine(sphere, poly[1], poly[2], edge_hits) > 0) {
        return SPHERE_POLY3_EDGE;
    }
    if (mgIntersectionSphereLine(sphere, poly[2], poly[0], edge_hits) > 0) {
        return SPHERE_POLY3_EDGE;
    }
    return SPHERE_POLY3_NONE;
}
#ifdef NONMATCHING
int IntersectionBox(float *from, float *to, mgVu0FBOX *box, float (*hits)[4]) {
    sceVu0FVECTOR direction;
    sceVu0FVECTOR segment_max;
    sceVu0FVECTOR segment_min;
    sceVu0FVECTOR point;
    sceVu0FVECTOR candidates[6];
    int count = 0;
    sceVu0SubVector(direction, to, from);
    mgVectorMaxMin(segment_max, segment_min, from, to);
    for (int axis = 0; axis < 3; axis++) {
        for (int face = 0; face < 2; face++) {
            float plane = face == 0 ? box->min[axis] : box->max[axis];
            if (plane >= segment_max[axis] || plane <= segment_min[axis]) {
                continue;
            }
            sceVu0ScaleVector(point, direction, (plane - from[axis]) / direction[axis]);
            sceVu0AddVector(point, point, from);
            int side_axis = (axis + 1) % 3;
            int other_axis = (side_axis + 1) % 3;
            if (point[side_axis] < box->max[side_axis] && point[side_axis] > box->min[side_axis] &&
                point[other_axis] < box->max[other_axis] && point[other_axis] > box->min[other_axis]) {
                point[3] = mgDistVector(point, from);
                sceVu0CopyVector(candidates[count++], point);
            }
        }
    }
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (candidates[j][3] < candidates[i][3]) {
                sceVu0FVECTOR swap;
                sceVu0CopyVector(swap, candidates[i]);
                sceVu0CopyVector(candidates[i], candidates[j]);
                sceVu0CopyVector(candidates[j], swap);
            }
        }
    }
    int hit_count = count > 2 ? 2 : count;
    for (int i = 0; i < hit_count; i++) {
        sceVu0CopyVector(hits[i], candidates[i]);
    }
    return hit_count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/intersection", IntersectionBox__FPfPfP9mgVu0FBOXPA4_f);
#endif

int IntersectionBox(float *from, float *to, mgVu0FBOX *box, float (*matrix)[4], float (*hits)[4]) {
    sceVu0FVECTOR local_from;
    sceVu0FVECTOR local_to;
    sceVu0FVECTOR local_hits[2];
    sceVu0FMATRIX inverse;
    *(u_long128 *)local_from = *(u_long128 *)from;
    local_from[3] = 1.0f;
    *(u_long128 *)local_to = *(u_long128 *)to;
    local_to[3] = 1.0f;
    mgInversMatrix(inverse, matrix);
    sceVu0ApplyMatrix(local_from, inverse, local_from);
    sceVu0ApplyMatrix(local_to, inverse, local_to);
    int count = IntersectionBox(local_from, local_to, box, local_hits);
    for (int i = 0; i < count; i++) {
        local_hits[i][3] = 1.0f;
        sceVu0ApplyMatrix(hits[i], matrix, local_hits[i]);
    }
    return count;
}
s32 mt_test(RS_STACKDATA *stack, int argc) {
    return 1;
}

// Uninitialised data (.bss)
INCLUDE_BSS(at_161, 0x10);
