#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include "mg_math.hpp"
#include "mw_runtime.h"
#include "platform_fixture.hpp"

extern "C" [[noreturn]] void PortExit(int status);

namespace {

constexpr float kNan = std::numeric_limits<float>::quiet_NaN();

} // namespace

TEST(RuntimePort, HardwareStubExitsWith70) {
    KZ_ASSERT_EXIT([] { PS2_UNIMPLEMENTED(); }, 70);
}

TEST(RuntimePort, GameExitIsTheRuntimes) {
    KZ_ASSERT_EXIT([] { PortExit(9); }, 9);
}

TEST(RuntimePort, GameConversionsSaturateAsMwcc) {
    EXPECT_EQ(fptosi(-2.75f), -2);
    EXPECT_EQ(fptosi(3.0e9f), INT32_MAX);
    EXPECT_EQ(fptosi(-3.0e9f), INT32_MIN);
    EXPECT_EQ(fptosi(kNan), 0);
    EXPECT_EQ(fptoui(-0.5f), 0u);
    EXPECT_EQ(fptoui(-7.0f), 0u);
    EXPECT_EQ(fptoui(3.0e9f), 3000000000u);
    EXPECT_EQ(fptoui(5.0e9f), UINT32_MAX);
}

TEST(RuntimePort, ConstructNewArrayWritesMwccsHeader) {
    alignas(16) std::uint32_t storage[4 + 3 * 2] = {};
    void *elements = __construct_new_array(storage, nullptr, nullptr, 8, 3);
    EXPECT_EQ(elements, static_cast<void *>(&storage[4]));
    EXPECT_EQ(storage[0], 8u);
    EXPECT_EQ(storage[1], 3u);
    EXPECT_EQ(__construct_new_array(nullptr, nullptr, nullptr, 8, 3), nullptr);
}

// The game's static constructors call these before main.
TEST(RuntimePort, StaticConstructorMath) {
    float matrix[4][4];
    for (auto &row : matrix) {
        for (float &value : row) {
            value = kNan;
        }
    }
    mgUnitMatrix(matrix);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_EQ(matrix[i][j], i == j ? 1.0f : 0.0f);
        }
    }
    float vector[4] = {kNan, kNan, kNan, kNan};
    mgZeroVectorW(vector);
    EXPECT_EQ(vector[0], 0.0f);
    EXPECT_EQ(vector[1], 0.0f);
    EXPECT_EQ(vector[2], 0.0f);
    EXPECT_EQ(vector[3], 1.0f);
}
