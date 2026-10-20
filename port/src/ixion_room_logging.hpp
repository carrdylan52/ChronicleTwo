#pragma once

#include <algorithm>

// Remember the full skipped-loop state; a successful entry permits a new receipt.
struct IxionRoomLoopLog {
    bool skipped = false;
    int loop = -1, map = -1;
    const void *scene = nullptr;

    bool Observe(bool skip, int next_loop, const void *next_scene, int next_map) {
        const bool changed = skip && (!skipped || loop != next_loop || scene != next_scene || map != next_map);
        skipped = skip;
        loop = next_loop;
        scene = next_scene;
        map = next_map;
        return changed;
    }
};

inline int IxionRoomLogInterval(int requested) {
    return requested >= 1 && requested <= 60 ? std::max(10, requested) : 60;
}

struct IxionRoomRequestLog {
    bool reported = false;
    bool Observe(bool pending) {
        const bool emit = pending && !reported;
        reported = pending;
        return emit;
    }
};

struct IxionRoomTraceLog {
    int last_frame = -30;
    bool Observe(int frame) {
        if (frame >= last_frame && frame - last_frame < 30) return false;
        last_frame = frame;
        return true;
    }
};
