#pragma once

#include "common.h"

#include <libvu0.h>

#include "map.hpp"
#include "mapload.hpp"

/**
 * @file
 * Declares the description of an event that the player has reached in a
 * scene: the event point of a map, a villager to talk to or a game object,
 * which a scene keeps while it runs the event and hands to the event editor.
 */

/**
 * Two words of an event description, copied together.
 */
struct EventFloat2 {
    float v[2]; /**< Words of the group. */
};

/**
 * Four words of an event description, copied together.
 */
struct EventFloat4 {
    float v[4]; /**< Words of the group. */
};

/**
 * Two quadwords of an event description, copied together.
 */
struct EventVector2 {
    u_long128 v[2]; /**< Quadwords of the group. */
};

/**
 * Four quadwords of an event description, copied together.
 */
struct EventVector4 {
    u_long128 v[4]; /**< Quadwords of the group. */
};

#pragma push
#pragma cpp_extensions on
/**
 *
 * Event the player has reached, with the settings, placement and owner of the point that starts it.
 *
 */
struct CSceneEventData {
    union {
        struct {
            CFuncPoint::EventData event;      /**< Settings of the event point, or the event number of a villager or game object. */
            sceVu0FVECTOR         position;   /**< Position of the event point or game object. */
            sceVu0FVECTOR         rotation;   /**< Rotation of the event point, zero for a game object. */
            sceVu0FVECTOR         scale;      /**< Scale of the event point. */
            MapEventInfo          map_event;  /**< Event point found on a map, with the matrix that places it. */
            s32                   chara_no;   /**< Character number of the villager talked to. */
            s32                   chara_slot; /**< Scene character slot of the villager talked to. */
            s32                   gameobj_no; /**< Index of the game object position within its map's entry. */
            s32                   unk_cc;
        };
        struct {
            EventFloat4  head;                                   /**< Words 0x00 to 0x0F, copied as one group. */
            EventFloat4  group_1;                                /**< Words 0x10 to 0x1F, copied as one group. */
            EventFloat2  group_2;                                /**< Words 0x20 to 0x27, copied as one group. */
            EventFloat4  group_3 __attribute__((aligned(16)));   /**< Words 0x30 to 0x3F, copied as one group. */
            EventFloat4  group_4;                                /**< Words 0x40 to 0x4F, copied as one group. */
            EventFloat4  group_5;                                /**< Words 0x50 to 0x5F, copied as one group. */
            EventVector4 vectors_a;                              /**< Quadwords 0x60 to 0x9F, copied as one group. */
            EventVector2 vectors_b;                              /**< Quadwords 0xA0 to 0xBF, copied as one group. */
        };
    };
};
#pragma pop

STATIC_ASSERT(sizeof(CSceneEventData) == 0xD0);
