#include "common.h"

#include "occlusion.hpp"
#include "mg_math.hpp"

// Code (.text)

void COcclusion::Setup(sceVu0FMATRIX view_matrix) {
    if (enable == 0) {
        return;
    }

    setup = 1;
    sceVu0FVECTOR view_vertex[4];
    sceVu0FVECTOR origin;
    mgApplyMatrixN(view_vertex, view_matrix, vertex, 4);
    mgVectorMin(view_min, view_vertex[0], view_vertex[1], view_vertex[2], view_vertex[3]);
    mgZeroVector(origin);

    mgPlaneNormal(plane, view_vertex[2], view_vertex[1], view_vertex[0]);
    sceVu0Normalize(plane, plane);
    plane[3] = -sceVu0InnerProduct(plane, view_vertex[0]);

    if (plane[3] > 0.0f) {
        mgPlaneNormal(plane, view_vertex[0], view_vertex[1], view_vertex[2]);
        sceVu0Normalize(plane, plane);
        plane[3] = -sceVu0InnerProduct(plane, view_vertex[0]);
        mgPlaneNormal(side_plane[0], origin, view_vertex[1], view_vertex[2]);
        mgPlaneNormal(side_plane[1], origin, view_vertex[3], view_vertex[0]);
        mgPlaneNormal(side_plane[2], origin, view_vertex[0], view_vertex[1]);
        mgPlaneNormal(side_plane[3], origin, view_vertex[2], view_vertex[3]);
    } else {
        mgPlaneNormal(side_plane[0], origin, view_vertex[2], view_vertex[1]);
        mgPlaneNormal(side_plane[1], origin, view_vertex[0], view_vertex[3]);
        mgPlaneNormal(side_plane[2], origin, view_vertex[1], view_vertex[0]);
        mgPlaneNormal(side_plane[3], origin, view_vertex[3], view_vertex[2]);
    }

    sceVu0Normalize(side_plane[0], side_plane[0]);
    side_plane[0][3] = 0.0f;
    sceVu0Normalize(side_plane[1], side_plane[1]);
    side_plane[1][3] = 0.0f;
    sceVu0Normalize(side_plane[2], side_plane[2]);
    side_plane[2][3] = 0.0f;
    sceVu0Normalize(side_plane[3], side_plane[3]);
    side_plane[3][3] = 0.0f;
}

int COcclusion::CheckSphere(sceVu0FVECTOR sphere) {
    if (enable == 0 || setup == 0) {
        return 0;
    }
    if (view_min[2] > sphere[2] - sphere[3]) {
        return 0;
    }
    float distance = sceVu0InnerProduct(plane, sphere);
    distance += plane[3];
    if (distance < sphere[3]) {
        return 0;
    }
    if (sceVu0InnerProduct(side_plane[0], sphere) < sphere[3]) {
        return 0;
    }
    if (sceVu0InnerProduct(side_plane[1], sphere) < sphere[3]) {
        return 0;
    }
    if (sceVu0InnerProduct(side_plane[2], sphere) < sphere[3]) {
        return 0;
    }
    if (sceVu0InnerProduct(side_plane[3], sphere) < sphere[3]) {
        return 0;
    }
    return 1;
}
