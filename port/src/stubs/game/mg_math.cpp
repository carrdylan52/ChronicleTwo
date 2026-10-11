#include "mg_math.hpp"

void mgFotI4(int *out, float *in) {
    PS2_UNIMPLEMENTED();
}

void mgCreateBox8(float (*corners)[4], float *max, float *min) {
    PS2_UNIMPLEMENTED();
}

int mgClipBoxVertex(float *point, float *max, float *min) {
    PS2_UNIMPLEMENTED();
}

int mgClipBox(float *max0, float *min0, float *max1, float *min1) {
    PS2_UNIMPLEMENTED();
}

int mgClipBoxW(float *max0, float *min0, float *max1, float *min1) {
    PS2_UNIMPLEMENTED();
}

int mgClipInBox(float *max0, float *min0, float *max1, float *min1) {
    PS2_UNIMPLEMENTED();
}

int mgClipInBoxW(float *max0, float *min0, float *max1, float *min1) {
    PS2_UNIMPLEMENTED();
}

void mgAddVector(float *vector, float *add) {
    PS2_UNIMPLEMENTED();
}

void mgSubVector(float *vector, float *sub) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMin(float *min, float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMin(float *min, float *a, float *b, float *c, float *d) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMaxMin(float *max, float *min, float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMaxMin(float *max, float *min, float *a, float *b, float *c) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMaxMin(float *max, float *min, float *a, float *b, float *c, float *d) {
    PS2_UNIMPLEMENTED();
}

void mgBoxMaxMin(mgVu0FBOX *box, mgVu0FBOX *other) {
    PS2_UNIMPLEMENTED();
}

void mgPlaneNormal(float *normal, float *v0, float *v1, float *v2) {
    PS2_UNIMPLEMENTED();
}

float mgDistVector(float *vector) {
    PS2_UNIMPLEMENTED();
}

float mgDistVectorXZ(float *vector) {
    PS2_UNIMPLEMENTED();
}

float mgDistVector2(float *vector) {
    PS2_UNIMPLEMENTED();
}

float mgDistVector(float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

float mgDistVectorXZ(float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

float mgDistVector2(float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

float mgDistVectorXZ2(float *a, float *b) {
    PS2_UNIMPLEMENTED();
}

void mgZeroMatrix(float (*matrix)[4]) {
    PS2_UNIMPLEMENTED();
}

void mgMulMatrix(float (*product)[4], float (*left_matrix)[4], float (*right_matrix)[4]) {
    PS2_UNIMPLEMENTED();
}

void mgInversMatrix(float (*inverse)[4], float (*matrix)[4]) {
    PS2_UNIMPLEMENTED();
}

PORT_OVERRIDE void mgRotMatrixXYZ(float (*matrix)[4], float *rotation) {
    PS2_UNIMPLEMENTED();
}

void mgApplyMatrixN(float (*out)[4], float (*matrix)[4], float (*in)[4], int count) {
    PS2_UNIMPLEMENTED();
}

void mgApplyMatrixN_MaxMin(float (*out)[4], float (*matrix)[4], float (*in)[4], int count, float *max, float *min) {
    PS2_UNIMPLEMENTED();
}

void mgVectorMinMaxN(float *max, float *min, float (*vectors)[4], int count) {
    PS2_UNIMPLEMENTED();
}
