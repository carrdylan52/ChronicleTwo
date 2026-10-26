#include "common.h"

#include <SDL3/SDL_events.h>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "camera_control_port.hpp"
#include "debug_camera_mouse.hpp"
#include "debug_camera_orbit.hpp"
#include "dng_main.hpp"
#include "editctrl_port.hpp"
#include "editloop.hpp"
#include "mainloop.hpp"
#include "menu_mouse.hpp"
#include "nowload.hpp"
#include "photo.hpp"
#include "platform/input.hpp"
#include "platform/mouse.hpp"
#include "platform/window.hpp"
#include "subgame.hpp"

namespace {
using Clock = std::chrono::steady_clock;
DebugCameraInputGate gate;
bool (*overlay_input_query)() = nullptr;
bool focused = true;
float pending_wheel = 0;
Clock::time_point wheel_time;
CCameraControl *sample_camera = nullptr;
CScene *sample_scene = nullptr;
int sample_map = -1, sample_submap = -1, sample_loop = -1;

void Discard() {
    pending_wheel = 0;
    gate.Reset();
}

CCameraControl *Owner() {
    if (!focused || !MouseInputLive() || InputDeveloperMenu() || MenuMouseCapturing() ||
        (overlay_input_query && overlay_input_query()) || DebugInfo.debug_camera != 1 ||
        GamePad__2.key_lock || mgCCamera::StopCamera || GetPauseFlag() || NowTakePhoto() || SubGameRunning()) return nullptr;
    // The loop goes first: the developer menu reuses the arena of the town camera MainScene still
    // lists, and this also runs from window events, so camera objects are read only in their own loop.
    auto *scene = GetMainScene();
    if (GetNowLoopNo() == LOOP_DUNGEON) {
        auto *area = BattleAreaScene;
        if (scene != DngMainScene || DngStatus.mode != DNG_STATUS_FIELD || DngStatus.eye_view ||
            PortDungeonDebugPause || !area || !MainChara__2 || area->camera_mode != DNG_CAMERA_FOLLOW ||
            (area->pause_flag & (DNG_PAUSE_PLAYER_STEP | DNG_PAUSE_PLAYER_CONTROL | DNG_PAUSE_FIELD_STEP | DNG_PAUSE_PAD_RESET)) ||
            area->script.event_no != -1 || area->script.running || MainChara__2->damage_req == ACTION_DAMAGE_REQ_DEAD)
            return nullptr;
    } else if (GetNowLoopNo() == LOOP_EDIT) {
        // Town retail explicitly ignores DebugInfo.debug_camera while DebugFlag is off.
        if (!DebugFlag || IsEditMode() || PortTownLoopMode != EDIT_LOOP_WALK ||
            PortTownControlMode != EDIT_CONTROL_PLAYER || PortTownLockChara || PortTownPause ||
            PortEditViewMode != EDIT_VIEW_MODE_WALK || PortEditLadderMode != EDIT_LADDER_MODE_NONE)
            return nullptr;
    } else {
        return nullptr;
    }
    if (!scene || scene->active_camera != 0 || scene->event_run || !scene->GetMap(scene->active_map)) return nullptr;
    auto *base = scene->GetCamera(0);
    auto *chara = scene->GetCharacter(scene->player_chara);
    if (!base || !chara || base->Iam() != CAMERA_KIND_CONTROL || base->suspended) return nullptr;
    auto *camera = static_cast<CCameraControl *>(base);
    if (camera->control_on || !camera->follow_on) return nullptr;
    return camera;
}

void NoteOwner(CCameraControl *camera) {
    auto *scene = GetMainScene();
    if (!camera || !scene) {
        sample_camera = nullptr;
        Discard();
        return;
    }
    if (sample_camera != camera || sample_scene != scene || sample_map != scene->now_map_no ||
        sample_submap != scene->now_sub_map_no || sample_loop != GetNowLoopNo()) {
        Discard();
        sample_camera = camera;
        sample_scene = scene;
        sample_map = scene->now_map_no;
        sample_submap = scene->now_sub_map_no;
        sample_loop = GetNowLoopNo();
    }
}

void Observe(const SDL_Event &event) {
    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
        focused = event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
        Discard();
        return;
    }
    auto *owner = Owner();
    NoteOwner(owner);
    if (!owner || event.type != SDL_EVENT_MOUSE_WHEEL || !std::isfinite(event.wheel.y)) return;
    const auto now = Clock::now();
    if (now - wheel_time > std::chrono::milliseconds(250)) pending_wheel = 0;
    const float turn = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
    pending_wheel = std::clamp(pending_wheel + turn, -8.0f, 8.0f);
    wheel_time = now;
}
} // namespace

void DebugCameraMouseSetOverlayInputQuery(bool (*query)()) {
    overlay_input_query = query;
    sample_camera = nullptr;
    Discard();
}

void DebugCameraMouseInstall() {
    focused = true;
    sample_camera = nullptr;
    Discard();
    WindowAddEventHook(Observe);
}

void DebugCameraMouseBeforeStep(CCameraControl &camera, int frames) {
    if (frames <= 0) return; // Camera placement/cuts and display-only draws never apply live input.
    auto *scene = GetMainScene();
    if (!scene || scene->GetCamera(scene->active_camera) != &camera) return;
    auto *owner = Owner();
    NoteOwner(owner);
    const auto &look = InputGetMouseLook();
    if (owner == &camera && !gate.Fresh(look.read)) return;
    const float wheel = Clock::now() - wheel_time <= std::chrono::milliseconds(250) ? pending_wheel : 0;
    const bool neutral = look.yaw == 0 && look.pitch == 0 && wheel == 0;
    // Always discard this batch, including during re-entry settling. The shared input remains intact.
    pending_wheel = 0;
    if (!gate.Read(look.read, owner == &camera, neutral)) return;
    DebugCameraOrbit pose{.angle = camera.next_angle, .distance = camera.distance, .height = camera.height};
    if (DebugCameraOrbitInput(pose, look.yaw, look.pitch, wheel)) {
        camera.SetAngle(pose.angle);
        camera.SetDistance(pose.distance);
        camera.SetHeight(pose.height);
    }
}
