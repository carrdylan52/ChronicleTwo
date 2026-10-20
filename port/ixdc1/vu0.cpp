#include "private.hpp"

extern "C" {
void sceVu0CopyVector(float *out, float *in) { std::memcpy(out, in, 4 * sizeof(float)); }
void sceVu0ScaleVectorXYZ(float *out, float *in, float scale) {
    for (int i = 0; i < 3; ++i) out[i] = in[i] * scale;
    out[3] = in[3];
}
void sceVu0UnitMatrix(float (*out)[4]) {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) out[i][j] = i == j ? 1.0f : 0.0f;
}
void sceVu0MulMatrix(float (*out)[4], float (*a)[4], float (*b)[4]) {
    float result[4][4]{};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) result[i][j] += a[k][j] * b[i][k];
    std::memcpy(out, result, sizeof(result));
}
void sceVu0RotMatrixY(float (*out)[4], float (*in)[4], float yaw) {
    float rotation[4][4]; sceVu0UnitMatrix(rotation);
    const float c = std::cos(yaw), s = std::sin(yaw);
    rotation[0][0] = c; rotation[0][2] = -s;
    rotation[2][0] = s; rotation[2][2] = c;
    sceVu0MulMatrix(out, rotation, in);
}
void sceVu0RotMatrixX(float (*out)[4], float (*in)[4], float pitch) {
    float rotation[4][4]; sceVu0UnitMatrix(rotation);
    const float c = std::cos(pitch), s = std::sin(pitch);
    rotation[1][1] = c; rotation[1][2] = s;
    rotation[2][1] = -s; rotation[2][2] = c;
    sceVu0MulMatrix(out, rotation, in);
}
// Xiao R1: as Kazarov's port/sce/libvu0.cpp:189-194 (DC1's own port): xyz / |xyz|, w 0, FLT_MAX scale at length 0.
void sceVu0Normalize(float *out, float *in) {
    const float length = std::sqrt(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);
    const float scale = length == 0.0f ? 3.402823466e+38f : 1.0f / length;
    const float result[4] = {in[0] * scale, in[1] * scale, in[2] * scale, 0.0f};
    std::memcpy(out, result, sizeof(result));
}
void sceVu0ApplyMatrix(float *out, float (*matrix)[4], float *in) {
    float result[4]{};
    for (int j = 0; j < 4; ++j)
        for (int i = 0; i < 4; ++i) result[j] += matrix[i][j] * in[i];
    std::memcpy(out, result, sizeof(result));
}
}
