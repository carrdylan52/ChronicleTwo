#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "debug_camera_orbit.hpp"

TEST(DebugCameraOrbit, KeepsNativePoseWhenIdleAndBoundsMouseChanges) {
    DebugCameraOrbit pose{.angle = 0, .distance = 100, .height = 50};
    EXPECT_TRUE(DebugCameraOrbitInput(pose, .25f, 0, 1));
    EXPECT_TRUE(pose.angle == -.25f && pose.distance == 92);
    EXPECT_TRUE(std::fabs(pose.height / pose.distance - .5f) < .0001f);
    EXPECT_TRUE(DebugCameraOrbitInput(pose, 0, .1f, 0));
    EXPECT_TRUE(pose.height < 46 && pose.distance == 92);
    for (int i = 0; i < 20; ++i) DebugCameraOrbitInput(pose, 0, 0, 100);
    EXPECT_TRUE(pose.distance == 30);
    EXPECT_TRUE(DebugCameraOrbitInput(pose, 0, 0, -1));
    EXPECT_TRUE(pose.distance == 38); // Reverses immediately, without accumulating excess wheel.
    for (int i = 0; i < 20; ++i) DebugCameraOrbitInput(pose, 0, 0, -100);
    EXPECT_TRUE(pose.distance == 600);
    EXPECT_TRUE(DebugCameraOrbitInput(pose, 0, -100, 0));
    const float limit = 80 * std::numbers::pi_v<float> / 180;
    EXPECT_TRUE(std::fabs(std::atan2(pose.height, pose.distance) - limit) < .0001f);
    auto old = pose;
    EXPECT_TRUE(!DebugCameraOrbitInput(pose, 0, 0, 0));
    EXPECT_TRUE(pose.angle == old.angle && pose.height == old.height && pose.distance == old.distance);
    EXPECT_TRUE(!DebugCameraOrbitInput(pose, INFINITY, 0, 1));
    EXPECT_TRUE(pose.angle == old.angle && pose.height == old.height && pose.distance == old.distance);
    DebugCameraOrbit native{.angle = 10, .distance = 800, .height = -900};
    EXPECT_TRUE(!DebugCameraOrbitInput(native, 0, 0, 0)); // No mouse input must leave native pad values alone.
    EXPECT_TRUE(native.distance == 800 && native.height == -900);

}

TEST(DebugCameraOrbit, RequiresFreshNeutralReadsAfterOwnerChanges) {
    DebugCameraInputGate gate;
    EXPECT_TRUE(!gate.Read(1, false, false));
    EXPECT_TRUE(!gate.Read(2, true, false)); // Closing motion is discarded, not queued.
    EXPECT_TRUE(!gate.Read(3, true, true));
    EXPECT_TRUE(!gate.Read(3, true, true)); // Duplicate draw/step does not count as another neutral read.
    EXPECT_TRUE(!gate.Read(4, true, true));
    EXPECT_TRUE(gate.Read(5, true, false));
    EXPECT_TRUE(!gate.Read(5, true, false));
    EXPECT_TRUE(!gate.Read(6, false, true)); // Menu/release/photo/event owns input.
    EXPECT_TRUE(!gate.Read(7, true, true));
    EXPECT_TRUE(!gate.Read(8, true, false)); // A moving sample restarts neutral settling.
    EXPECT_TRUE(!gate.Read(9, true, true));
    EXPECT_TRUE(!gate.Read(10, true, true));
    EXPECT_TRUE(gate.Read(11, true, false));
    gate.Reset(); // Camera/map/loop identity changed.
    EXPECT_TRUE(!gate.Read(12, true, false));
}
