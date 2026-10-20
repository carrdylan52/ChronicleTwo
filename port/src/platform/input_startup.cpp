#include "input_startup.hpp"

#include <SDL3/SDL.h>
#include <cstdio>

#include "platform/input.hpp"
#include "platform/settings.hpp"

void InputInitForRun(bool headless) {
    if (headless) {
        // InputPoll and script overrides work without physical devices or window event hooks.
        // Apply the same bindings and mouse settings without opening SDL's gamepad subsystem.
        InputApplyConfig(SettingsGet());
    } else {
        InputInit();
        int count = 0;
        SDL_JoystickID *pads = SDL_GetGamepads(&count);
        std::fprintf(stderr, "input: gamepads=%d\n", count);
        for (int i = 0; pads != nullptr && i < count; ++i) {
            const char *name = SDL_GetGamepadNameForID(pads[i]);
            std::fprintf(stderr, "input: gamepad id=%u name=%s\n", static_cast<unsigned>(pads[i]),
                         name != nullptr ? name : "(unknown)");
        }
        SDL_free(pads);
    }
}
