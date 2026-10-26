#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

// The native debug camera is a player-following orbit, not a free-flying camera.
struct DebugCameraOrbit {
    float angle, distance, height;
};

inline bool DebugCameraOrbitInput(DebugCameraOrbit &pose, float yaw, float pitch, float wheel) {
    if (!std::isfinite(pose.angle) || !std::isfinite(pose.distance) || !std::isfinite(pose.height) ||
        !std::isfinite(yaw) || !std::isfinite(pitch) || !std::isfinite(wheel) ||
        (yaw == 0 && pitch == 0 && wheel == 0)) return false;
    constexpr float turn = 2 * std::numbers::pi_v<float>;
    constexpr float limit = 80 * std::numbers::pi_v<float> / 180;
    if (yaw != 0) pose.angle = std::remainder(pose.angle - yaw, turn);
    if (pitch != 0 || wheel != 0) {
        const float distance = std::clamp(pose.distance, 30.0f, 600.0f);
        const float elevation = std::clamp(std::atan2(pose.height, distance) - pitch, -limit, limit);
        pose.distance = std::clamp(distance - 8 * std::clamp(wheel, -8.0f, 8.0f), 30.0f, 600.0f);
        pose.height = std::tan(elevation) * pose.distance;
    }
    return true;
}

// One motion sample per logic read. After losing ownership, require two fresh neutral
// samples before accepting input again; none of the discarded samples is queued.
class DebugCameraInputGate {
public:
    bool Fresh(std::uint64_t read) const { return read_ != read; }
    bool Read(std::uint64_t read, bool owns, bool neutral) {
        if (!owns) {
            neutral_ = 0;
            read_ = read;
            return false;
        }
        if (read_ == read) return false;
        read_ = read;
        if (neutral_ < 2) {
            neutral_ = neutral ? neutral_ + 1 : 0;
            return false;
        }
        return true;
    }
    void Reset() { neutral_ = 0; }

private:
    std::uint64_t read_ = 0;
    int neutral_ = 0;
};
