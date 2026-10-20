#ifndef IXION_DC1_TOAN_MOTION_H
#define IXION_DC1_TOAN_MOTION_H

/* Pure DC1 <-> DC2 conversions for Toan's vertical motion. Shared by the DC2 bridge
 * (port/src/ixion_toan_combat.cpp) and ixdc1_toan_tests, so the landing rule is tested headless. */

namespace ixion_toan {

/* Reported while DC2 finds no ground under Toan (high in the air). Dark Cloud keeps its last value
 * instead; a stale value below 2.0 could land Toan in mid-air, so the bridge never reuses one. */
constexpr float kNoGround = 1000.0f;

/* Ticks Toan may wait in DC1 action 0x19 (falling after the slam) before the bridge reports the ground.
 * Dark Cloud waits forever; DC2 always lands within this on any floor, so this only fires on a fault. */
constexpr int kLandingGuardTicks = 90;

/* Dark Cloud's action 0x19 waits for BtActStatus.ground_height < 2.0 (charakey.cpp:576). Dark Cloud sets it
 * after its MoveCheck as pos[1] - ground_point[1], only when a ground polygon was found
 * (Chronicle port/src/dun/movechara.cpp:1368-1370). DC2's MoveCheck gives the same three facts
 * (gameutil.cpp:1792-1812). Returned in DC1 units: the DC2 height divided by the body's room scale.
 * R3 passed world Y here, which never drops below 2 on a floor above Y=2 and stranded Toan in 0x19. */
inline float GroundHeight(bool landed, bool ground_found, float y, float ground_y, float scale) {
    if (landed) return 0.0f;
    if (!ground_found) return kNoGround;
    const float height = (y - ground_y) / (scale > 0.0f ? scale : 1.0f);
    return height < 0.0f ? 0.0f : height;
}

/* One gravity: Dark Cloud zeroes the walk velocity while can_act == 0 (movechara.cpp:1182-1186) and adds
 * jump_velocity_y, so the whole vertical step of a tick is jump_velocity_y, which the handler lowers by 0.1
 * per tick (+2.0 at launch, apex about 21 DC1 units after 20 ticks). The bridge writes this step over DC2's
 * velocity[1] each tick; DC2's own -0.5/tick (terminal -3.5) is overwritten until the donor stops jumping. */
inline float VerticalStep(float dc1_velocity_y, float scale) {
    return dc1_velocity_y * scale;
}

struct LandingGuard {
    int  ticks = 0;
    bool fired = false;
    /* previous_action is the donor action seen last tick. */
    float Filter(int previous_action, float height) {
        if (previous_action != 0x19) {
            ticks = 0;
            return height;
        }
        if (++ticks > kLandingGuardTicks) {
            fired = true;
            return 0.0f;
        }
        return height;
    }
};

} // namespace ixion_toan

#endif
