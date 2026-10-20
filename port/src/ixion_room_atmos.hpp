#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "gfx/gfx.hpp"

struct IxionRoom;
struct mgFOG_PARAM;

// The donor room's own atmosphere, read from the private atmosphere.json sidecar next to room.json
// (room.json v1 stays frozen). Music: a DC2-loadable pack of the donor floor's DC1 bank and
// sequences. Fog and light: the donor cfg's FOG, LIGHT_C, AMBIENT and BG_COL values, with
// distances already in DC2 units.
struct IxionRoomAtmosLight {
    int slot = 0; // 1..4, DC1's LIGHT_C slot
    std::array<float, 3> direction{};
    std::array<float, 3> color{}; // 0..255, 128 is unit intensity
};

struct IxionRoomAtmosphere {
    std::string donor_room;
    bool has_bgm = false;
    std::vector<uint32_t> bgm_pack; // PACK_ENTRY chain, word-aligned for sndLoadSound
    size_t bgm_pack_bytes = 0;
    int bgm_sequence = 0;
    int bgm_sequence_count = 0;
    std::string bgm_sequence_name;
    float fog_near = 0.0f;
    float fog_far = 0.0f;
    std::array<uint8_t, 3> fog_color{};
    float fog_far_value = 255.0f;
    float fog_near_value = 255.0f;
    std::vector<IxionRoomAtmosLight> lights;
    bool has_ambient = false;
    std::array<float, 3> ambient{};
    bool has_bg_color = false;
    std::array<float, 3> bg_color{};

    static bool Load(const std::filesystem::path &json_path, IxionRoomAtmosphere &out, std::string &error);
};

// Validates a PACK_ENTRY chain as DC2's music loader reads it: exactly one .hd, .bd, .cfg and
// .vol, and between one and 32 .sq members. Returns the sequence count or -1 with an error.
int IxionRoomAtmosValidateBgmPack(const uint8_t *data, size_t size, std::string &error,
                                  std::vector<std::string> *sequence_names = nullptr);
// Applies the parsed donor FOG fields using the native draw-environment arithmetic.
void IxionRoomAtmosApplyFog(const IxionRoomAtmosphere &atmosphere, mgFOG_PARAM &fog);

// Runtime seams called from ixion_room.cpp. Configure runs once after room.json loads; it reads
// DC2_IXION_ROOM_ATMOS (unset: atmosphere.json next to room.json when present; 0: off; else a
// sidecar path). Install and Retire bracket the room's installation.
void IxionRoomAtmosConfigure(const std::filesystem::path &room_json, const IxionRoom &room);
void IxionRoomAtmosInstall(const void *owner_scene, const void *owner_map);
void IxionRoomAtmosRetire(bool departing);
// Drop all scene/BGM claims without accessing the arena that has gone away.
void IxionRoomAtmosDiscard();
// Adds the active atmosphere's fog to the imported room's mesh draws.
void IxionRoomAtmosDecorateDraw(gfx::MeshConstants &constants, gfx::DrawState &state);
bool IxionRoomAtmosActive();
