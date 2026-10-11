#include "mg_math.hpp"

#include "sce_core/vu0_math.hpp"

// Static constructors call these before main, so they cannot wait for V3 with the rest of mg_math.

void mgZeroVectorW(float *vector) {
    vector[0] = 0.0f;
    vector[1] = 0.0f;
    vector[2] = 0.0f;
    vector[3] = 1.0f;
}

void mgUnitMatrix(float (*matrix)[4]) {
    sce_core::vu0::UnitMatrix(matrix);
}
