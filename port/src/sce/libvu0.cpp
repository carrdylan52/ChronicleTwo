#include <libvu0.h>

#include "sce_core/vu0_math.hpp"

namespace vu0 = sce_core::vu0;

void sceVu0CopyVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1) {
    vu0::CopyVector(v0, v1);
}

void sceVu0CopyVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1) {
    vu0::CopyVectorXYZ(v0, v1);
}

void sceVu0FTOI4Vector(sceVu0IVECTOR v0, sceVu0FVECTOR v1) {
    vu0::FTOI4Vector(v0, v1);
}

void sceVu0ITOF4Vector(sceVu0FVECTOR v0, sceVu0IVECTOR v1) {
    vu0::ITOF4Vector(v0, v1);
}

void sceVu0ScaleVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s) {
    vu0::ScaleVector(v0, v1, s);
}

void sceVu0ScaleVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float s) {
    vu0::ScaleVectorXYZ(v0, v1, s);
}

void sceVu0AddVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2) {
    vu0::AddVector(v0, v1, v2);
}

void sceVu0SubVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2) {
    vu0::SubVector(v0, v1, v2);
}

void sceVu0MulVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2) {
    vu0::MulVector(v0, v1, v2);
}

void sceVu0InterVectorXYZ(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2, float r) {
    vu0::InterVectorXYZ(v0, v1, v2, r);
}

void sceVu0DivVector(sceVu0FVECTOR v0, sceVu0FVECTOR v1, float q) {
    vu0::DivVector(v0, v1, q);
}

float sceVu0InnerProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1) {
    return vu0::InnerProduct(v0, v1);
}

void sceVu0OuterProduct(sceVu0FVECTOR v0, sceVu0FVECTOR v1, sceVu0FVECTOR v2) {
    vu0::OuterProduct(v0, v1, v2);
}

void sceVu0Normalize(sceVu0FVECTOR v0, sceVu0FVECTOR v1) {
    vu0::Normalize(v0, v1);
}

void sceVu0ApplyMatrix(sceVu0FVECTOR v0, sceVu0FMATRIX m, sceVu0FVECTOR v1) {
    vu0::ApplyMatrix(v0, m, v1);
}

void sceVu0UnitMatrix(sceVu0FMATRIX m) {
    vu0::UnitMatrix(m);
}

void sceVu0CopyMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1) {
    vu0::CopyMatrix(m0, m1);
}

void sceVu0TransposeMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1) {
    vu0::TransposeMatrix(m0, m1);
}

void sceVu0MulMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FMATRIX m2) {
    vu0::MulMatrix(m0, m1, m2);
}

void sceVu0InversMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1) {
    vu0::InversMatrix(m0, m1);
}

void sceVu0RotMatrixX(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rx) {
    vu0::RotMatrixX(m0, m1, rx);
}

void sceVu0RotMatrixY(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float ry) {
    vu0::RotMatrixY(m0, m1, ry);
}

void sceVu0RotMatrixZ(sceVu0FMATRIX m0, sceVu0FMATRIX m1, float rz) {
    vu0::RotMatrixZ(m0, m1, rz);
}

void sceVu0RotMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FVECTOR rot) {
    vu0::RotMatrix(m0, m1, rot);
}

void sceVu0TransMatrix(sceVu0FMATRIX m0, sceVu0FMATRIX m1, sceVu0FVECTOR tv) {
    vu0::TransMatrix(m0, m1, tv);
}

void sceVu0CameraMatrix(sceVu0FMATRIX m, sceVu0FVECTOR p, sceVu0FVECTOR zd, sceVu0FVECTOR yd) {
    vu0::CameraMatrix(m, p, zd, yd);
}
