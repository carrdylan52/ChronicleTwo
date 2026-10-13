// The Ixion module for Dark Chronicle: the game built as a shared library that exports only
// ixion_get_api. The host loads it, runs it on a fiber of its own, and switches games at frame ends.
// Built only with IXION_BUILD_PLUGIN; the executable does not contain this file.

#include "ixion_api.h"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "actionchara.hpp"
#include "dng_main.hpp"
#include "mainloop.hpp"
#include "mapselect.hpp"
#include "mglib_port.hpp"
#include "platform/audio.hpp"
#include "platform/clock.hpp"
#include "platform/embed.hpp"
#include "platform/mouse.hpp"
#include "platform/window.hpp"
#include "savedatadungeon.hpp"
#include "scenesnd.hpp"

// main.cpp: the executable's run path, without the process entry.
int RunGame(int argc, const char **argv);

namespace {

constexpr std::uint32_t kGameId = IXION_GAME_DARK_CHRONICLE;

static_assert(static_cast<int>(kEmbedFrameEnd) == static_cast<int>(IXION_CHECKPOINT_FRAME_END),
              "the embed checkpoint reason is the ABI's frame end");

const IxionHost *g_host = nullptr;

bool PluginPollEvent(void *, SDL_Event *event) {
    if (g_host == nullptr || g_host->poll_event == nullptr) {
        return false;
    }
    return g_host->poll_event(g_host->user, kGameId, event) != 0;
}

bool PluginCheckpoint(void *, int reason) {
    if (g_host == nullptr || g_host->checkpoint == nullptr) {
        return false;
    }
    return g_host->checkpoint(g_host->user, kGameId, static_cast<std::uint32_t>(reason)) != 0;
}

int PluginBind(const IxionHost *host) {
    if (host == nullptr || host->abi_version != IXION_ABI_VERSION) {
        return 1;
    }
    g_host = host;
    return 0;
}

int PluginRun(int argc, char **argv) {
    EmbedHooks hooks;
    hooks.poll_event = PluginPollEvent;
    hooks.checkpoint = PluginCheckpoint;
    // Before RunGame: WindowInit reads the embedded state.
    EmbedEnable(hooks);

    // The game's argv ends with a null pointer, as the executable's does.
    std::vector<const char *> args(argv, argv + argc);
    args.push_back(nullptr);
    return RunGame(argc, args.data());
}

void PluginActivate(int active) {
    AudioOutputPause(active == 0);
    // A parked game hands the cursor back. Its window regains focus when it is shown, and the focus
    // event takes the capture again.
    if (active == 0 && MouseCaptured()) {
        MouseStop();
    }
}

void *PluginWindow() {
    return static_cast<void *>(WindowHandle());
}

int PluginInspect(IxionReceipt *out) {
    if (out == nullptr || out->size < sizeof(IxionReceipt)) {
        return 1;
    }
    out->game_id = kGameId;
    out->logic_frames = static_cast<std::uint64_t>(mgPortFrameCount());
    out->clock_ticks = static_cast<std::uint64_t>(ClockTickCount());
    const int mode = GetNowLoopNo();
    out->mode = mode;
    out->map = -1;
    out->floor = -1;
    out->character = -1;
    out->player_pos[0] = out->player_pos[1] = out->player_pos[2] = 0.0f;
    out->place[0] = '\0';

    // The scene and the characters are only valid once the town or dungeon loop has set them up.
    if (mode != LOOP_EDIT && mode != LOOP_DUNGEON) {
        return 0;
    }
    CScene *scene = GetMainScene();
    if (scene == nullptr) {
        return 0;
    }
    // GUESS: the main scene's map number. For a dungeon this is the scene's own map, not the dungeon id.
    out->map = scene->GetNowMapNo();
    // In a dungeon, the floor the save records for the current dungeon.
    if (mode == LOOP_DUNGEON && DngSaveDataDungeon != nullptr && DngSaveDataDungeon->stage_id >= 0 &&
        DngSaveDataDungeon->stage_id < SAVE_DUNGEON_NUM) {
        out->floor = DngSaveDataDungeon->floor_id[DngSaveDataDungeon->stage_id];
    }
    // The active character. In a dungeon it is the game's own player character (MainChara__2). In town it is
    // the scene's player slot, which EditInit places at the map's start position. LoadChara puts plain
    // CCharacter2 objects in slots, so the kind is read only when dynamic_cast finds a CActionChara.
    CCharacter2 *chara = MainChara__2;
    if (mode == LOOP_EDIT) {
        chara = scene->player_chara >= 0 ? scene->GetCharacter(scene->player_chara) : nullptr;
    }
    if (chara != nullptr) {
        if (auto *action = dynamic_cast<CActionChara *>(chara); action != nullptr) {
            out->character = action->chara_type;
        }
        float pos[4] = {};
        chara->GetPosition(pos);
        out->player_pos[0] = pos[0];
        out->player_pos[1] = pos[1];
        out->player_pos[2] = pos[2];
    }
    if (out->map >= 0) {
        if (const char *name = GetMapName(out->map, nullptr); name != nullptr) {
            std::snprintf(out->place, sizeof out->place, "%s", name);
        }
    }
    return 0;
}

const IxionApi kApi = {
    sizeof(IxionApi),
    IXION_ABI_VERSION,
    kGameId,
    "Dark Chronicle",
    PluginBind,
    PluginRun,
    PluginActivate,
    PluginWindow,
    PluginInspect,
};

} // namespace

IXION_EXPORT const IxionApi *ixion_get_api(uint32_t abi_version) {
    return abi_version == IXION_ABI_VERSION ? &kApi : nullptr;
}
