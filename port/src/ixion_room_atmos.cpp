#include "common.h"

#include "ixion_room_atmos.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <libvu0.h>
#include <nlohmann/json.hpp>
#include <string_view>

#include "dng_main.hpp"
#include "ixion_room.hpp"
#include "map.hpp"
#include "mapload.hpp"
#include "mg_drawenv.hpp"
#include "mglib.hpp"
#include "platform/paths.hpp"
#include "scenesnd.hpp"
#include "snd_mngr.hpp"

namespace {

using Json = nlohmann::json;
constexpr std::uintmax_t kSidecarLimit = 1u * 1024u * 1024u;
constexpr std::uintmax_t kPackLimit = 16u * 1024u * 1024u;
constexpr size_t kPackEntryHeader = 0x4C; // PACK_ENTRY: name[64], offset, size, next
constexpr size_t kMaxPackEntries = 256;
// A load number no DC2 music file uses, so CheckLoadBGM reloads the native file afterwards.
constexpr int kAtmosBgmLoadNo = 0x1C100;

bool Fail(std::string &error, const std::string &message) {
    error = message;
    return false;
}

bool ReadFileBytes(const std::filesystem::path &path, std::uintmax_t limit, std::vector<uint8_t> &out) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec || size == 0 || size > limit) return false;
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    out.resize(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(out.size()));
    return static_cast<bool>(stream);
}

bool SafeRelative(const std::string &text, std::filesystem::path &out) {
    if (text.empty() || text.size() > 240 || text.find(':') != std::string::npos ||
        text.find('\\') != std::string::npos)
        return false;
    const std::filesystem::path path(text);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
    for (const auto &part : path)
        if (part == ".." || part == ".") return false;
    out = path;
    return true;
}

bool Finite(const Json &value, float &out) {
    if (!value.is_number()) return false;
    const double number = value.get<double>();
    if (!std::isfinite(number) || std::fabs(number) > 1.0e7) return false;
    out = static_cast<float>(number);
    return true;
}

template <size_t N>
bool FiniteVector(const Json &value, std::array<float, N> &out, float lo, float hi) {
    if (!value.is_array() || value.size() != N) return false;
    for (size_t i = 0; i < N; ++i) {
        if (!Finite(value[i], out[i]) || out[i] < lo || out[i] > hi) return false;
    }
    return true;
}

bool EndsWithNoCase(std::string_view text, std::string_view suffix) {
    if (text.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(text[text.size() - suffix.size() + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

bool ParseSidecar(const Json &root, const std::filesystem::path &dir, IxionRoomAtmosphere &out, std::string &error) {
    if (!root.is_object() || root.value("format", std::string()) != "ixion-room-atmosphere" ||
        !root.contains("version") || !root["version"].is_number_integer() || root["version"].get<int>() != 1)
        return Fail(error, "atmosphere.json is not ixion-room-atmosphere version 1");
    const Json &room = root.value("room", Json::object());
    if (!room.is_object() || !room.contains("donor_room") || !room["donor_room"].is_string())
        return Fail(error, "atmosphere.json room.donor_room missing");
    out.donor_room = room["donor_room"].get<std::string>();

    if (root.contains("bgm")) {
        const Json &bgm = root["bgm"];
        if (!bgm.is_object() || !bgm.contains("pack") || !bgm["pack"].is_string())
            return Fail(error, "atmosphere.json bgm.pack missing");
        std::filesystem::path relative;
        if (!SafeRelative(bgm["pack"].get<std::string>(), relative))
            return Fail(error, "atmosphere.json bgm.pack is not a safe relative path");
        if (!bgm.contains("sequence") || !bgm["sequence"].is_number_integer())
            return Fail(error, "atmosphere.json bgm.sequence missing");
        out.bgm_sequence = bgm["sequence"].get<int>();
        std::vector<uint8_t> bytes;
        if (!ReadFileBytes(dir / relative, kPackLimit, bytes))
            return Fail(error, "cannot read bgm pack or it exceeds 16 MiB");
        std::vector<std::string> names;
        const int sequences = IxionRoomAtmosValidateBgmPack(bytes.data(), bytes.size(), error, &names);
        if (sequences < 0) return false;
        if (out.bgm_sequence < 0 || out.bgm_sequence >= sequences)
            return Fail(error, "atmosphere.json bgm.sequence is outside the pack's sequences");
        out.bgm_sequence_count = sequences;
        out.bgm_sequence_name = names[static_cast<size_t>(out.bgm_sequence)];
        out.bgm_pack.assign((bytes.size() + 3) / 4, 0u);
        std::memcpy(out.bgm_pack.data(), bytes.data(), bytes.size());
        out.bgm_pack_bytes = bytes.size();
        out.has_bgm = true;
    }

    const Json &fog = root.value("fog", Json());
    if (!fog.is_object()) return Fail(error, "atmosphere.json fog missing");
    std::array<float, 3> fog_color{};
    if (!fog.contains("near") || !fog.contains("far") || !Finite(fog["near"], out.fog_near) ||
        !Finite(fog["far"], out.fog_far) || out.fog_near < 0.0f || out.fog_far <= out.fog_near)
        return Fail(error, "atmosphere.json fog needs 0 <= near < far");
    if (!fog.contains("color") || !FiniteVector(fog["color"], fog_color, 0.0f, 255.0f))
        return Fail(error, "atmosphere.json fog.color needs three values 0..255");
    for (size_t i = 0; i < 3; ++i) out.fog_color[i] = static_cast<uint8_t>(fog_color[i]);
    if (!fog.contains("far_value") || !fog.contains("near_value") || !Finite(fog["far_value"], out.fog_far_value) ||
        !Finite(fog["near_value"], out.fog_near_value) || out.fog_far_value < 0.0f || out.fog_far_value > 255.0f ||
        out.fog_near_value < 0.0f || out.fog_near_value > 255.0f)
        return Fail(error, "atmosphere.json fog far_value/near_value need 0..255");

    const Json &lights = root.value("lights", Json::array());
    if (!lights.is_array() || lights.size() > 4) return Fail(error, "atmosphere.json lights needs at most four entries");
    bool used[5] = {};
    for (const Json &entry : lights) {
        IxionRoomAtmosLight light;
        if (!entry.is_object() || !entry.contains("slot") || !entry["slot"].is_number_integer())
            return Fail(error, "atmosphere.json light slot missing");
        light.slot = entry["slot"].get<int>();
        if (light.slot < 1 || light.slot > 4 || used[light.slot])
            return Fail(error, "atmosphere.json light slots must be unique 1..4");
        used[light.slot] = true;
        if (!entry.contains("direction") || !FiniteVector(entry["direction"], light.direction, -1.0e6f, 1.0e6f))
            return Fail(error, "atmosphere.json light direction needs three finite values");
        const float length = std::sqrt(light.direction[0] * light.direction[0] + light.direction[1] * light.direction[1] +
                                       light.direction[2] * light.direction[2]);
        if (!(length > 1.0e-6f)) return Fail(error, "atmosphere.json light direction is zero");
        for (float &c : light.direction) c /= length;
        if (!entry.contains("color") || !FiniteVector(entry["color"], light.color, 0.0f, 255.0f))
            return Fail(error, "atmosphere.json light color needs three values 0..255");
        out.lights.push_back(light);
    }
    if (root.contains("ambient")) {
        if (!FiniteVector(root["ambient"], out.ambient, 0.0f, 255.0f))
            return Fail(error, "atmosphere.json ambient needs three values 0..255");
        out.has_ambient = true;
    }
    if (root.contains("bg_color")) {
        if (!FiniteVector(root["bg_color"], out.bg_color, 0.0f, 255.0f))
            return Fail(error, "atmosphere.json bg_color needs three values 0..255");
        out.has_bg_color = true;
    }
    return true;
}

// ---- runtime -------------------------------------------------------------------------------

struct AtmosRuntime {
    bool configured = false;
    bool loaded = false;
    IxionRoomAtmosphere atmosphere;
    bool installed = false;
    const void *owner_scene = nullptr;
    const void *owner_map = nullptr;
    bool bgm_swapped = false;
    int saved_load_no = -1;
    int saved_play_no = -1;
    int saved_vol = -1;
    float saved_volf = 1.0f;
    bool lighting_logged = false;
};

AtmosRuntime &Runtime() {
    static AtmosRuntime runtime;
    return runtime;
}

void SwapInDonorBgm(AtmosRuntime &rt) {
    if (!rt.atmosphere.has_bgm || DngMainScene == nullptr) return;
    CScene::BGM_INFO *info = DngMainScene->GetActiveBgmInfo();
    rt.saved_load_no = info->load_no;
    rt.saved_play_no = info->play_no;
    rt.saved_vol = info->vol;
    rt.saved_volf = info->volf;
    if (!DngMainScene->LoadBGMPack(kAtmosBgmLoadNo, reinterpret_cast<u32 *>(rt.atmosphere.bgm_pack.data()))) {
        std::fprintf(stderr, "ixion atmos bgm load failed: sndLoadSound rejected the donor pack; native bgm load_no=%d kept\n",
                     rt.saved_load_no);
        if (rt.saved_load_no >= 0 && DngMainScene->LoadBGM(rt.saved_load_no, BuffReadData))
            DngMainScene->PlayBGM(std::max(rt.saved_play_no, 0), -1, 1.0f);
        return;
    }
    rt.bgm_swapped = true;
    DngMainScene->PlayBGM(rt.atmosphere.bgm_sequence, -1, 1.0f);
    std::fprintf(stderr,
                 "ixion atmos bgm started: donor=%s sequence=%d (%s) of %d snd_id=0x%08x load_no=0x%x play_no=%d vol=%d "
                 "state=%d native_load_no=%d native_play_no=%d\n",
                 rt.atmosphere.donor_room.c_str(), rt.atmosphere.bgm_sequence, rt.atmosphere.bgm_sequence_name.c_str(),
                 rt.atmosphere.bgm_sequence_count, static_cast<unsigned>(info->snd_id), info->load_no, info->play_no,
                 info->vol, DngMainScene->GetBGMState(), rt.saved_load_no, rt.saved_play_no);
}

void RestoreNativeBgm(AtmosRuntime &rt, bool departing) {
    if (!rt.bgm_swapped) return;
    rt.bgm_swapped = false;
    if (departing || DngMainScene == nullptr || DngMainScene != rt.owner_scene) {
        // The next floor's own start-up loads its music: load_no no longer matches any native file.
        std::fprintf(stderr, "ixion atmos bgm released on departure; next scene loads its native music (native_load_no=%d)\n",
                     rt.saved_load_no);
        return;
    }
    CScene::BGM_INFO *info = DngMainScene->GetActiveBgmInfo();
    DngMainScene->StopBGM(0);
    if (rt.saved_load_no < 0 || !DngMainScene->LoadBGM(rt.saved_load_no, BuffReadData)) {
        std::fprintf(stderr, "ixion atmos bgm restore failed: native load_no=%d\n", rt.saved_load_no);
        return;
    }
    const int play_no = rt.saved_play_no >= 0 ? rt.saved_play_no : 0;
    DngMainScene->PlayBGM(play_no, rt.saved_vol, rt.saved_volf);
    std::fprintf(stderr, "ixion atmos bgm restored native: load_no=%d play_no=%d snd_id=0x%08x vol=%d state=%d\n",
                 info->load_no, info->play_no, static_cast<unsigned>(info->snd_id), info->vol,
                 DngMainScene->GetBGMState());
}

void ApplyFog(const IxionRoomAtmosphere &a, mgFOG_PARAM &fog) {
    // mgRENDER_INFO::SetFogParam's arithmetic (ps2/src/mg_drawenv.cpp:424-443), so every reader of
    // the lighting info sees consistent coefficients.
    const float range = a.fog_far_value - a.fog_near_value;
    const float distance = a.fog_far - a.fog_near;
    fog.near_dist = a.fog_near;
    fog.far_dist = a.fog_far;
    fog.r = a.fog_color[0];
    fog.g = a.fog_color[1];
    fog.b = a.fog_color[2];
    fog.offset = (a.fog_far_value + a.fog_near_value + range * (a.fog_far + a.fog_near) / distance) / 2.0f;
    fog.far_value = a.fog_far_value;
    fog.near_value = a.fog_near_value;
    fog.scale = -a.fog_far * a.fog_near * range / distance;
    fog.coef[0] = fog.offset;
    fog.coef[1] = fog.far_value;
    fog.coef[2] = fog.near_value;
    fog.coef[3] = fog.scale;
}

void ApplyLighting(AtmosRuntime &rt, CMapLightingInfo *info) {
    const IxionRoomAtmosphere &a = rt.atmosphere;
    if (!rt.lighting_logged) {
        std::fprintf(stderr,
                     "ixion atmos lighting native: fog_enable=%d near=%.1f far=%.1f rgb=%d,%d,%d far_value=%.1f near_value=%.1f "
                     "ambient=%.1f,%.1f,%.1f light_color0=%.1f,%.1f,%.1f light_color1=%.1f,%.1f,%.1f plight=%d\n",
                     info->fog_enable, info->fog.near_dist, info->fog.far_dist, info->fog.r, info->fog.g, info->fog.b,
                     info->fog.far_value, info->fog.near_value, info->ambient[0], info->ambient[1], info->ambient[2],
                     info->light_color[0][0], info->light_color[0][1], info->light_color[0][2], info->light_color[1][0],
                     info->light_color[1][1], info->light_color[1][2], info->plight_enable);
    }
    // DC1 lights the floor with only its LIGHT_C slots (ps2/src/itemdata.cpp:731-746 in Chronicle);
    // slots the cfg leaves out stay dark.
    for (int light = 0; light < 4; ++light) {
        for (int c = 0; c < 4; ++c) info->light_color[light][c] = 0.0f;
    }
    for (const IxionRoomAtmosLight &light : a.lights) {
        const int column = light.slot - 1;
        for (int c = 0; c < 3; ++c) {
            info->light_dir[c][column] = light.direction[c];
            info->light_color[column][c] = light.color[c];
        }
    }
    if (a.has_ambient) {
        for (int c = 0; c < 3; ++c) info->ambient[c] = a.ambient[c];
    }
    if (a.has_bg_color) {
        for (int c = 0; c < 3; ++c) info->bg_color[c] = info->bg_color2[c] = a.bg_color[c];
    }
    info->plight_enable = 0;
    info->fog_enable = 1;
    IxionRoomAtmosApplyFog(a, info->fog);
    if (!rt.lighting_logged) {
        rt.lighting_logged = true;
        std::fprintf(stderr,
                     "ixion atmos lighting applied: fog near=%.1f far=%.1f rgb=%d,%d,%d far_value=%.1f near_value=%.1f "
                     "offset=%.3f scale=%.3f ambient=%.1f,%.1f,%.1f lights=%zu light_color1=%.1f,%.1f,%.1f\n",
                     info->fog.near_dist, info->fog.far_dist, info->fog.r, info->fog.g, info->fog.b, info->fog.far_value,
                     info->fog.near_value, info->fog.offset, info->fog.scale, info->ambient[0], info->ambient[1],
                     info->ambient[2], a.lights.size(), info->light_color[1][0], info->light_color[1][1],
                     info->light_color[1][2]);
    }
}

bool LightingOwnedBy(const AtmosRuntime &rt, const CMap *map) {
    return rt.installed && rt.loaded && map != nullptr && map == rt.owner_map && DngMainScene != nullptr &&
           DngMainScene == rt.owner_scene && DngMainScene->GetMap(DngMainScene->active_map) == map;
}

} // namespace

void IxionRoomAtmosApplyFog(const IxionRoomAtmosphere &atmosphere, mgFOG_PARAM &fog) {
    ApplyFog(atmosphere, fog);
}

int IxionRoomAtmosValidateBgmPack(const uint8_t *data, size_t size, std::string &error,
                                  std::vector<std::string> *sequence_names) {
    int hd = 0, bd = 0, cfg = 0, vol = 0, sq = 0;
    size_t position = 0;
    for (size_t entries = 0;; ++entries) {
        if (entries > kMaxPackEntries) return Fail(error, "bgm pack has too many entries"), -1;
        if (position > size || size - position < kPackEntryHeader) return Fail(error, "bgm pack entry header truncated"), -1;
        const char *name = reinterpret_cast<const char *>(data + position);
        const size_t length = strnlen(name, 64);
        if (length == 0) break;
        if (length == 64) return Fail(error, "bgm pack member name is not terminated"), -1;
        int32_t offset = 0, bytes = 0, next = 0;
        std::memcpy(&offset, data + position + 64, 4);
        std::memcpy(&bytes, data + position + 68, 4);
        std::memcpy(&next, data + position + 72, 4);
        if (offset < static_cast<int32_t>(kPackEntryHeader) || bytes < 0 || next < static_cast<int32_t>(kPackEntryHeader) ||
            static_cast<size_t>(offset) > size - position || static_cast<size_t>(bytes) > size - position - offset ||
            static_cast<size_t>(next) > size - position)
            return Fail(error, "bgm pack member range leaves the file"), -1;
        const std::string_view member(name, length);
        if (EndsWithNoCase(member, ".hd")) ++hd;
        else if (EndsWithNoCase(member, ".bd")) ++bd;
        else if (EndsWithNoCase(member, ".cfg")) ++cfg;
        else if (EndsWithNoCase(member, ".vol")) ++vol;
        else if (EndsWithNoCase(member, ".sq")) {
            ++sq;
            if (sequence_names != nullptr) sequence_names->emplace_back(member);
        }
        position += static_cast<size_t>(next);
    }
    if (hd != 1 || bd != 1 || cfg != 1 || vol != 1)
        return Fail(error, "bgm pack needs exactly one .hd, .bd, .cfg and .vol"), -1;
    if (sq < 1 || sq > 32) return Fail(error, "bgm pack needs 1..32 .sq sequences"), -1;
    return sq;
}

bool IxionRoomAtmosphere::Load(const std::filesystem::path &json_path, IxionRoomAtmosphere &out, std::string &error) {
    std::vector<uint8_t> bytes;
    if (!ReadFileBytes(json_path, kSidecarLimit, bytes)) return Fail(error, "cannot read atmosphere.json or it exceeds 1 MiB");
    try {
        const Json document = Json::parse(bytes.begin(), bytes.end());
        IxionRoomAtmosphere candidate;
        if (!ParseSidecar(document, json_path.parent_path(), candidate, error)) return false;
        out = std::move(candidate);
        error.clear();
        return true;
    } catch (const std::exception &exception) {
        error = std::string("invalid atmosphere JSON: ") + exception.what();
        return false;
    }
}

void IxionRoomAtmosConfigure(const std::filesystem::path &room_json, const IxionRoom &room) {
    AtmosRuntime &rt = Runtime();
    if (rt.configured) return;
    rt.configured = true;
    const char *setting = std::getenv("DC2_IXION_ROOM_ATMOS");
    std::filesystem::path sidecar;
    if (setting != nullptr && std::string_view(setting) == "0") {
        std::fprintf(stderr, "ixion atmos disabled by DC2_IXION_ROOM_ATMOS=0; native atmosphere\n");
        return;
    }
    if (setting != nullptr && *setting != '\0') {
        sidecar = PathsFromUtf8(setting);
    } else {
        sidecar = room_json.parent_path() / "atmosphere.json";
        std::error_code ec;
        if (!std::filesystem::exists(sidecar, ec)) {
            std::fprintf(stderr, "ixion atmos absent: no atmosphere.json beside room.json; native atmosphere\n");
            return;
        }
    }
    std::string error;
    IxionRoomAtmosphere atmosphere;
    if (!IxionRoomAtmosphere::Load(sidecar, atmosphere, error)) {
        std::fprintf(stderr, "ixion atmos rejected: %s; native atmosphere\n", error.c_str());
        return;
    }
    if (atmosphere.donor_room != room.donor_room) {
        std::fprintf(stderr, "ixion atmos rejected: sidecar donor %s does not match room donor %s; native atmosphere\n",
                     atmosphere.donor_room.c_str(), room.donor_room.c_str());
        return;
    }
    rt.atmosphere = std::move(atmosphere);
    rt.loaded = true;
    std::fprintf(stderr,
                 "ixion atmos loaded: donor=%s bgm=%d pack_bytes=%zu sequence=%d (%s) of %d fog=%.1f..%.1f lights=%zu\n",
                 rt.atmosphere.donor_room.c_str(), rt.atmosphere.has_bgm ? 1 : 0, rt.atmosphere.bgm_pack_bytes,
                 rt.atmosphere.bgm_sequence, rt.atmosphere.bgm_sequence_name.c_str(), rt.atmosphere.bgm_sequence_count,
                 rt.atmosphere.fog_near, rt.atmosphere.fog_far, rt.atmosphere.lights.size());
}

void IxionRoomAtmosInstall(const void *owner_scene, const void *owner_map) {
    AtmosRuntime &rt = Runtime();
    if (!rt.loaded || rt.installed) return;
    rt.installed = true;
    rt.owner_scene = owner_scene;
    rt.owner_map = owner_map;
    rt.lighting_logged = false;
    SwapInDonorBgm(rt);
}

void IxionRoomAtmosRetire(bool departing) {
    AtmosRuntime &rt = Runtime();
    if (!rt.installed) return;
    RestoreNativeBgm(rt, departing);
    rt.installed = false;
    rt.owner_scene = nullptr;
    rt.owner_map = nullptr;
    std::fprintf(stderr, "ixion atmos retired: departing=%d; native lighting resumes with the next GetLightInfo\n",
                 departing ? 1 : 0);
}

bool IxionRoomAtmosActive() {
    const AtmosRuntime &rt = Runtime();
    return rt.installed && rt.loaded;
}

void IxionRoomAtmosDiscard() {
    AtmosRuntime &rt = Runtime();
    rt.installed = false;
    rt.owner_scene = nullptr;
    rt.owner_map = nullptr;
    rt.bgm_swapped = false;
    rt.saved_load_no = rt.saved_play_no = rt.saved_vol = -1;
    rt.saved_volf = 1.0f;
    rt.lighting_logged = false;
}

void IxionRoomAtmosDecorateDraw(gfx::MeshConstants &constants, gfx::DrawState &state) {
    if (!IxionRoomAtmosActive() || !mgGetFogEnable()) return;
    // As water.cpp: mesh.vert clamps to [lo, hi] where the VU takes max(far, min(near, F)).
    const mgFOG_PARAM &fog = mgRenderInfo.fog;
    constants.flags |= gfx::kMeshFog;
    constants.fog[0] = fog.offset;
    constants.fog[1] = fog.scale;
    constants.fog[2] = fog.far_value;
    constants.fog[3] = std::max(fog.near_value, fog.far_value);
    state.fog = true;
    state.fog_color[0] = fog.r;
    state.fog_color[1] = fog.g;
    state.fog_color[2] = fog.b;
}

// ps2/src/mapload.cpp's CMap::GetLightInfo, unchanged, followed by the donor atmosphere while the
// imported room owns this map. DngMainDraw (ps2/src/dng_main.cpp:1198-1236) applies the result
// with mgSetFogParam, mgSetLight and mgSetAmbient every frame, so retirement needs no restore.
PORT_OVERRIDE void CMap::GetLightInfo(CMapLightingInfo *out_info) {
    if (out_info == NULL) {
        return;
    }

    int num = map_info.time_light_num;

    if (GetActiveLightNo() >= num || (!GetTimeEnable() && !map_info.fixed_time_enable)) {
        CMapLightingInfo *info = GetLightingInfo(GetActiveLightNo());

        if (info != NULL) {
            *out_info = *info;
            if (LightingOwnedBy(Runtime(), this)) ApplyLighting(Runtime(), out_info);
            return;
        }
    }

    CMapLightingInfo *list[8];
    float             ratio[8];
    sceVu0FVECTOR     sun;

    int band = GetNowTimeLightBand();

    for (int i = 0; i < num; i++) {
        list[i] = map_info.GetLightingInfo(i);

        if (list[i] == NULL) {
            return;
        }
    }

    *out_info = *list[band];

    if (map_info.time_light_blend) {
        GetLightInfo(out_info, ratio, GetTimeLightingRatio(ratio));

        GetSunPoint(sun);
        sceVu0Normalize(sun, sun);

        if (sun[1] < 0.2f) {
            sun[1] = 0.2f;
            sceVu0Normalize(sun, sun);
        }

        out_info->light_dir[0][0] = sun[0];
        out_info->light_dir[1][0] = sun[1];
        out_info->light_dir[2][0] = sun[2];
    }

    if (LightingOwnedBy(Runtime(), this)) ApplyLighting(Runtime(), out_info);
}
