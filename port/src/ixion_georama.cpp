#include "common.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "cameracontrol.hpp"
#include "dataread.hpp"
#include "mainloop.hpp"
#include "nd_meswin.hpp"
#include "padcontrol.hpp"
#include "villagermngr.hpp"
#include "scenevillager.hpp"
#include "editanalyze.hpp"
#include "editctrl.hpp"
#include "editdata.hpp"
#include "editloop.hpp"
#include "editmap.hpp"
#include "editmode.hpp"
#include "editmenu.hpp"
#include "editriver.hpp"
#include "gamepad.hpp"
#include "ixion_georama.hpp"
#include "mapjump.hpp"
#include "mapselect.hpp"
#include "mdslist.hpp"
#include "menudraw.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "platform/files.hpp"
#include "platform/paths.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "vlgr_info.hpp"
#include "stubs/editmenu.hpp"

int NowEditModeChg();

extern int SandboxMapNo asm("EditLoop_MapNo");
extern int SandboxLoopMode asm("EditLoop_Mode");
extern int SandboxControlMode asm("EditLoop_Control");
extern int SandboxLockChara asm("EditLoop_PlayerLock");
extern int SandboxPause asm("EditLoop_Pause");

namespace {
bool                enabled = false;
CEditData           sandbox;
std::array<int, 16> before;
constexpr int       kSlot = 5;
constexpr char      kMapName[] = "gix";
// Ids the sandbox adds. Native g01 ids and the retail table never use these slots, and each has a
// zero entry in georama_parts_adjust_scaletable (ps2/src/editmenu.cpp:306), which collapses the preview.
constexpr int       kSandboxIds[] = {50, 51, 52, 54, 55, 56, 58};

int SandboxStockTotal(CEditData &data) {
    int total = 0;
    for (int id : kSandboxIds) {
        total += data.GetPartsNumID(id);
    }
    return total;
}

struct Sidecar {
    char                   magic[16] = "IxionGeoramaV1";
    std::uint32_t          size = sizeof(CEditData);
    std::uint32_t          checksum = 0;
    CEditData              data;
    std::array<short, 128> stock{};
};

static_assert(sizeof(CEditData) == 0x5510);

std::uint32_t Checksum(const Sidecar &record) {
    std::uint32_t value = 2166136261u;
    auto          add = [&](const void *bytes, std::size_t count) {
        auto *p = static_cast<const unsigned char *>(bytes);
        for (std::size_t i = 0; i < count; ++i) {
            value = (value ^ p[i]) * 16777619u;
        }
    };
    add(&record.data, sizeof(record.data));
    add(record.stock.data(), sizeof(record.stock));
    return value;
}

bool RecordSafe(const Sidecar &record) {
    const auto &data = record.data;
    if (std::memcmp(record.magic, "IxionGeoramaV1\0\0", 16) != 0 || record.size != sizeof(CEditData) ||
        record.checksum != Checksum(record) || data.parts_max != 300 || data.house_max != 32 ||
        data.save_count < 0 || data.save_count > 1000000) {
        return false;
    }
    int count = 0;
    for (const auto &part : data.parts) {
        if (part.id < 0 || part.id >= 128 || part.state < 0 || part.state > 2 ||
            part.angle < 0 || part.angle >= 24 || part.house_no < 0 || part.house_no > 32) {
            return false;
        }
        count += part.id != 0;
    }
    if (count > 256) {
        return false;
    }
    for (const auto &entry : data.place_log) {
        if (entry.parts_no < -1 || entry.parts_no >= count ||
            (entry.parts_no >= 0 && (entry.base_no < -1 || entry.base_no >= count))) {
            return false;
        }
    }
    // Validate the packed river records before retail LoadData walks them.
    std::size_t offset = 0;
    for (int i = 0; i < 4; ++i) {
        if (offset + sizeof(EditDataGrid) > sizeof(data.grid)) {
            return false;
        }
        EditDataGrid header;
        std::memcpy(&header, data.grid + offset, sizeof(header));
        if (!header.num_x && !header.num_z) {
            break;
        }
        const std::size_t cells = std::size_t(header.num_x) * header.num_z;
        if (!cells || cells > sizeof(data.grid) - offset - sizeof(header)) {
            return false;
        }
        offset = (offset + sizeof(header) + cells + 1) & ~std::size_t(1);
    }
    return std::all_of(record.stock.begin(), record.stock.end(), [](short n) { return n >= 0 && n <= 9999; });
}
} // namespace

bool IxionGeoramaRead(const std::filesystem::path &path, CEditData &data,
                      std::array<short, 128> &stock, std::string &error) {
    Sidecar         record;
    std::error_code ec;
    if (std::filesystem::file_size(path, ec) != sizeof(record) || ec ||
        FilesRead(path, &record, sizeof(record)) != sizeof(record) || !RecordSafe(record)) {
        error = "invalid or truncated georama sidecar";
        return false;
    }
    data = record.data;
    stock = record.stock;
    error.clear();
    return true;
}

bool IxionGeoramaWrite(const std::filesystem::path &path, const CEditData &data,
                       const std::array<short, 128> &stock) {
    Sidecar record;
    record.data = data;
    record.stock = stock;
    record.checksum = Checksum(record);
    return RecordSafe(record) && FilesWrite(path, &record, sizeof(record), true) == FilesResult::kWritten;
}

bool IxionGeoramaSaveSafe(const CEditMap &map) {
    if (map.edit_parts_max < 0 || map.edit_parts_max > 256 ||
        map.place_log_max < 0 || map.place_log_max > EDIT_DATA_PLACE_LOG_MAX ||
        map.grid_max < 0 || map.grid_max > EDIT_MAP_GRID_MAX ||
        (map.place_log_max > 0 && !map.place_log)) {
        return false;
    }
    std::size_t bytes = 0;
    for (int i = 0; i < map.grid_max; ++i) {
        const auto *grid = map.grid[i];
        if (!grid) {
            continue;
        }
        if (grid->num_x <= 0 || grid->num_z <= 0 || grid->num_x > 255 || grid->num_z > 255) {
            return false;
        }
        bytes += sizeof(EditDataGrid) + std::size_t(grid->num_x) * grid->num_z;
        bytes = (bytes + 1) & ~std::size_t(1);
        if (bytes > EDIT_DATA_GRID_SIZE) {
            return false;
        }
    }
    for (int i = 0; i < map.place_log_max; ++i) {
        const auto &entry = map.place_log[i];
        if (entry.parts_no >= map.edit_parts_max || entry.base_no >= map.edit_parts_max) {
            return false;
        }
    }
    return true;
}

bool IxionGeoramaRequested() {
    const char *value = std::getenv("DC2_IXION_GEORAMA_SANDBOX");
    return value && std::strcmp(value, "1") == 0;
}

bool IxionGeoramaEnabled() { return enabled; }

void IxionGeoramaPrepare(int map) {
    enabled = false;
    if (!IxionGeoramaRequested()) {
        return;
    }
    std::ifstream marker(PathsSaveRoot() / "ixion-georama-sandbox.marker");
    std::string   text;
    std::getline(marker, text);
    char *name = GetMapName(kSlot, nullptr);
    if (map != kSlot || !name || std::strcmp(name, kMapName) != 0 ||
        SearchMapNo(name) != kSlot || GetMapType(kSlot) != MAP_TYPE_GEORAMA ||
        text != "ixion-georama-sandbox-v1") {
        std::fprintf(stderr, "ixion georama: requires map 5 named gix and a disposable save marker\n");
        std::exit(2);
    }
    sandbox.Initialize();
    enabled = true;
    const auto sidecar = PathsSaveRoot() / "ixion-georama-v1.bin";
    if (std::filesystem::exists(sidecar)) {
        std::array<short, 128> stock;
        std::string            error;
        if (!IxionGeoramaRead(sidecar, sandbox, stock, error)) {
            std::fprintf(stderr, "ixion georama: %s; refusing to replace it\n", error.c_str());
            std::exit(2);
        }
        for (int i = 0; i < 128; ++i) {
            GetSaveData()->SetBuildPartsNum(i, stock[i]);
        }
        std::fprintf(stderr, "ixion georama: sidecar loaded saves=%d parts4=%d parts50=%d sandbox=%d\n",
                     sandbox.save_count, sandbox.GetPartsNumID(4), sandbox.GetPartsNumID(50), SandboxStockTotal(sandbox));
        for (int id : kSandboxIds) {
            if (sandbox.GetPartsNumID(id) > 0) {
                std::fprintf(stderr, "ixion georama: sidecar placed id=%d count=%d\n", id, sandbox.GetPartsNumID(id));
            }
        }
    } else {
        GetSaveData()->SetBuildPartsNum(4, 4);
        for (int id : kSandboxIds) {
            GetSaveData()->SetBuildPartsNum(id, 4);
        }
    }
    std::fprintf(stderr, "ixion georama: sandbox enabled slot=5 name=gix save=%s\n",
                 PathsDisplay(PathsSaveRoot()).c_str());
}

// CSaveData's five campaign records and its memory-card layout stay intact.
PORT_OVERRIDE CEditData *CSaveData::GetEditData(int index) {
    if (index >= 0 && index < 5) {
        return &edit_data[index];
    }
    return enabled && index == kSlot && this == GetSaveData() ? &sandbox : nullptr;
}

PORT_OVERRIDE void KeepEditAnalyze() {
    CEditData *data = GetSaveData()->GetEditData(SandboxMapNo);
    for (int i = 0; i < 16; ++i) {
        before[i] = data ? data->GetAnalyzeFlag(SandboxMapNo, i) : 0;
    }
}

PORT_OVERRIDE int EditAnalyzeChanged() {
    CSaveData *save = GetSaveData();
    CEditData *data = save->GetEditData(SandboxMapNo);
    if (!data || SandboxMapNo == kSlot || GetGameChapter(save->game_progress) >= 8) {
        return 0;
    }
    for (int i = 0; i < 16; ++i) {
        if (data->GetAnalyzeFlag(SandboxMapNo, i) != before[i]) {
            return 1;
        }
    }
    return 0;
}

// Native budget helpers otherwise return zero outside the five campaign towns.
PORT_OVERRIDE int GetMaxPolyn(int map) {
    constexpr int budgets[] = {4000, 6000, 6000, 6000, 4000};
    if (enabled && map == kSlot) {
        map = 0;
    }
    return map >= 0 && map < 5 ? budgets[map] : 0;
}

PORT_OVERRIDE int GetMaxDrawMem(int map) {
    constexpr int budgets[] = {48000, 50000, 54000, 54000, 48000};
    if (enabled && map == kSlot) {
        map = 0;
    }
    return map >= 0 && map < 5 ? budgets[map] : 0;
}

PORT_OVERRIDE MENUFORMPARTS_TYPE *CMenuPosDataForm::GetPartInfo(char *part_name) {
    if (!part_name || !parts) {
        return nullptr;
    }
    // The copied native Georama menu has five title sprites. Only the sandbox
    // uses Title5; reuse the source town's artwork without aliasing its save slot.
    if (enabled && SandboxMapNo == kSlot && std::strcmp(part_name, "Title5") == 0) {
        part_name = const_cast<char *>("Title0");
    }
    for (int i = 0; i < parts_num; ++i) {
        if (parts[i].name && std::strcmp(parts[i].name, part_name) == 0) {
            return &parts[i];
        }
    }
    return nullptr;
}

bool IxionGeoramaSandboxPart(int id) {
    return enabled && SandboxMapNo == kSlot && std::find(std::begin(kSandboxIds), std::end(kSandboxIds), id) != std::end(kSandboxIds);
}

// Called from the build copy of CMenuGeorama::LoadGeoramaPart (UNIT_FIXUPS editmenu.cpp).
// Native ids keep retail's 0.85 * table; only a zero entry of a sandbox id keeps the fit scale.
float PortGeoramaPreviewScale(int id, float fit_scale, float table_scale) {
    if (table_scale == 0.0f && IxionGeoramaSandboxPart(id)) {
        std::fprintf(stderr, "ixion georama: preview id=%d scale=%.4f\n", id, fit_scale);
        return fit_scale;
    }
    return 0.85f * table_scale;
}

PORT_OVERRIDE int MakeDownLoadAnaunce(int town, mgCMemory *stack, int *out_num, int *out_sub_num, int *out_height) {
    if (!enabled || town != kSlot) {
        return PortRetailMakeDownLoadAnaunce(town, stack, out_num, out_sub_num, out_height);
    }
    // No campaign requests, future unlocks, or Geostone consumption in this slot.
    // Later menu debug/UI writes must also point at the sidecar, never town 0.
    MenuAnalyzeData = &sandbox.analyze;
    MenuEditAnalyzeSrc = nullptr;
    MenuEditAnalyzeDataSrcNum = 0;
    std::fill(std::begin(MenuEditAnalyzeDataSrc), std::end(MenuEditAnalyzeDataSrc), nullptr);
    analyze_percent = 0;
    PartsMakeOkTableNum = 0;
    MenuGeoStoneDownLoad_PartsNum = 0;
    MenuGeoStoneDownLoad_Request = 0;
    MenuGeoStoneDownLoadTime = 0;
    DownLoadInfo = DownLoadInfoNext = nullptr;
    MenuGeoStoneDmyCnt = MenuGeoStoneDmyCnt_Now = nullptr;
    DownLoadMesMakeProgress = 2;
    if (out_num) {
        *out_num = 0;
    }
    if (out_sub_num) {
        *out_sub_num = 0;
    }
    if (out_height) {
        *out_height = 0;
    }
    std::fprintf(stderr, "ixion georama: sandbox menu analysis isolated; no campaign announcements\n");
    return 0;
}

PORT_OVERRIDE void EditDataSave() {
    if (InInterior()) {
        return;
    }
    auto *save = GetSaveData();
    auto *data = save->GetEditData(SandboxMapNo);
    auto *scene = GetMainScene();
    auto *map = static_cast<CEditMap *>(scene->GetMap(scene->active_map));
    if (!data || !map || std::strcmp(map->Iam(), "CEditMap") != 0) {
        return;
    }
    const bool private_slot = enabled && SandboxMapNo == kSlot;
    if (private_slot && !IxionGeoramaSaveSafe(*map)) {
        std::fprintf(stderr, "ixion georama: unsafe native save counts; sandbox not written\n");
        return;
    }
    map->SaveData(data);
    save->GetBitFlag(SAVE_FLAG_HEIM_FIRE_OUT);
    data->culture_point = map->CultureAnalyze(0);
    data->save_count += 1;
    map->GroundBalance(0);
    map->UpdateHouse();
    if (!private_slot && DebugInfo.georama_debug == 0 && GetMapType(SandboxMapNo) == MAP_TYPE_GEORAMA) {
        AnalyzeEditMap(SandboxMapNo, map);
    }
    if (private_slot) {
        std::array<short, 128> stock;
        for (int i = 0; i < 128; ++i) {
            stock[i] = save->GetBuildPartsNum(i);
        }
        bool written = IxionGeoramaWrite(PathsSaveRoot() / "ixion-georama-v1.bin", *data, stock);
        std::fprintf(stderr, "ixion georama: sidecar %s saves=%d parts4=%d parts50=%d sandbox=%d\n",
                     written ? "written" : "FAILED", data->save_count, data->GetPartsNumID(4), data->GetPartsNumID(50),
                     SandboxStockTotal(*data));
    }
}

void IxionGeoramaArrival() {
    if (!enabled || SandboxMapNo != kSlot) {
        return;
    }
    // Inventory belongs to InitSaveData's disposable session, not a campaign file.
    std::fprintf(stderr, "ixion georama: arrived map=%d stock native=4 donor=50\n", SandboxMapNo);
    auto *map = static_cast<CEditMap *>(GetMainScene()->GetMap(GetMainScene()->active_map));
    for (int id : kSandboxIds) {
        auto *info = map ? map->GetePartsInfoAtID(id) : nullptr;
        if (info) {
            std::fprintf(stderr, "ixion georama: donor registered id=%d model=%d area1=%.1f area5=%.1f\n",
                         id, info->parts != nullptr, info->col_area1.AreaXZ(), info->col_area5.AreaXZ());
        }
    }
    if (map) {
        for (int i = 0; i < map->edit_parts_max; ++i) {
            auto *part = map->GetePlaceParts(i);
            if (part && part->info && IxionGeoramaSandboxPart(part->info->id)) {
                sceVu0FVECTOR position;
                part->GetPosition(position);
                std::fprintf(stderr, "ixion georama: live donor id=%d slot=%d state=%d pos=%.1f,%.1f,%.1f\n",
                             part->info->id, i, part->state, position[0], position[1], position[2]);
                mgVu0FBOX box;
                for (int j = 0; j < 3; ++j) {
                    box.min[j] = position[j] - 100;
                    box.max[j] = position[j] + 100;
                }
                box.min[3] = box.max[3] = 1;
                CCPoly polys[128];
                int    count = part->GetPoly(MDS_TYPE_COLLISION, polys, box, 128);
                std::fprintf(stderr, "ixion georama: native world collision polygons=%d\n", count);
            }
        }
    }
}

PORT_OVERRIDE int CEditMap::CheckEditParts(CEditPartsInfo *info, float *pos, float angle, EP_PLACE_INFO *place) {
    if (!info) {
        return 0;
    }
    CEditParts *near_parts[512];
    int         count = GetNearParts(info, pos, angle, near_parts, 512);
    int         result = CheckEditParts(info, pos, angle, place, near_parts, count);
    if (enabled && area_no == kSlot && IxionGeoramaSandboxPart(info->id) && GamePad__2.Down(PAD_CROSS)) {
        std::fprintf(stderr, "ixion georama: donor placement check id=%d result=%d near=%d bases=%d blocked=%d pos=%.1f,%.1f,%.1f angle=%.3f\n",
                     info->id, result, count, place->num, place->unk_44, pos[0], pos[1], pos[2], angle);
    }
    return result;
}

PORT_OVERRIDE int CEditMap::BuildEditParts(int id) {
    auto *info = GetePartsInfoAtID(id);
    int   slot = info ? BuildEditParts(info->edit_name) : -1;
    if (enabled && area_no == kSlot && (id == 4 || IxionGeoramaSandboxPart(id))) {
        std::fprintf(stderr, "ixion georama: native build id=%d slot=%d\n", id, slot);
    }
    return slot;
}

// ---- DC1 Norune villagers at their Georama houses (GEORAMA-R3) ----------------------------------------------
// The roster is data: ix_villagers.txt (vlgr id, house part id, NPC spot in house-local units, yaw, model) and
// ix_vtext.txt (DC1 talk lines already converted to DC2 message text), both written next to the pack's own
// place.cfg / npc_place4.cfg rows by Ixion's tools/georama_villagers.py. DC2's real townsperson path then does the
// rest: CScene::LoadVillager -> GetLoadVillagerList -> CVillagerMngr::GetAppearVlgr (scenevillager.cpp:507,681)
// reads the NPC and NPC_PLACE rows for map 5 and loads chara/<model>.chr through LoadChara(..."info.cfg"...).
// The port only (a) drops a villager whose house is not placed and puts the NPC_PLACE position on the house's spot
// (as LoadGeoNPC does for the resident, editevent.cpp:747-759), and (b) shows the talk line, since the generic
// townsperson talk event (RunEvent(1000), editloop.cpp:1512) has no script for these ids.
namespace {
struct RosterEntry {
    int         id = 0;
    int         part = 0;
    float       spot[3] = {};
    float       yaw = 0;
    std::string model;
};
std::vector<RosterEntry>                    roster;
std::map<int, std::vector<std::string>>     talk_lines;
std::map<int, std::size_t>                  talk_next;
bool                                        roster_loaded = false;
int                                         talk_id = -1;
int                                         talk_frame = 0;
int                                         talk_slot = -1;

void LoadRoster() {
    if (roster_loaded) {
        return;
    }
    roster_loaded = true;
    static char buffer[0x10000];
    int         size = 0;
    char        villagers_name[] = "ix_villagers.txt";
    char        text_name[] = "ix_vtext.txt";
    if (LoadFile2(villagers_name, buffer, &size, 0) != 0 && size > 0 && size < int(sizeof(buffer)) - 1) {
        buffer[size] = 0;
        std::istringstream in(buffer);
        std::string        line;
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }
            std::istringstream row(line);
            RosterEntry        entry;
            if (row >> entry.id >> entry.part >> entry.spot[0] >> entry.spot[1] >> entry.spot[2] >> entry.yaw >> entry.model) {
                roster.push_back(entry);
            }
        }
    }
    if (!roster.empty() && LoadFile2(text_name, buffer, &size, 0) != 0 && size > 0 && size < int(sizeof(buffer)) - 1) {
        buffer[size] = 0;
        std::istringstream in(buffer);
        std::string        line;
        while (std::getline(in, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            const auto tab = line.find('\t');
            if (tab != std::string::npos) {
                talk_lines[std::atoi(line.c_str())].push_back(line.substr(tab + 1));
            }
        }
    }
    std::fprintf(stderr, "ixion georama: villager roster loaded entries=%zu talk=%zu\n", roster.size(), talk_lines.size());
}

const RosterEntry *FindRoster(int id) {
    for (const auto &entry : roster) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

// World position and yaw of the NPC spot on the live placed house, from its local-to-world matrix. The matrix is
// returned too, because the route nodes of the villager's place are house-local and take the same transform.
bool HousePose(const RosterEntry &entry, float *position, float &yaw, float (*matrix_out)[4] = nullptr) {
    CScene *scene = GetMainScene();
    auto   *map = scene ? static_cast<CEditMap *>(scene->GetMap(scene->active_map)) : nullptr;
    if (!map || std::strcmp(map->Iam(), "CEditMap") != 0) {
        return false;
    }
    for (int i = 0; i < map->edit_parts_max; ++i) {
        auto *part = map->GetePlaceParts(i);
        if (!part || !part->info || part->info->id != entry.part || part->state != 1) {
            continue;
        }
        float matrix[4][4], rotation[4];
        part->GetLWMatrix(matrix);
        position[0] = entry.spot[0];
        position[1] = entry.spot[1];
        position[2] = entry.spot[2];
        position[3] = 1.0f;
        sceVu0ApplyMatrix(position, matrix, position);
        part->GetRotation(rotation);
        yaw = mgAngleLimit(entry.yaw + rotation[1]);
        if (matrix_out) {
            std::memcpy(matrix_out, matrix, sizeof(matrix));
        }
        return true;
    }
    return false;
}

// The place table is parsed once and its MOVE_TO nodes are house-local (written next to the NPC spot by
// tools/georama_villagers.py). Each roster load rewrites the same node objects in world space from a copy of the
// local position, so loading the roster again (after the editor) never transforms a node twice.
std::map<const CVillagerPlaceInfo::Node *, std::array<float, 4>> route_local;

int PlaceHouseRoute(CVillagerPlaceInfo *place, const float (*matrix)[4]) {
    int moves = 0;
    for (auto *node = place->route; node; node = node->next) {
        if (node->type != VLGR_ROUTE_MOVE) {
            continue;
        }
        auto it = route_local.find(node);
        if (it == route_local.end()) {
            it = route_local.emplace(node, std::array<float, 4>{node->pos[0], node->pos[1], node->pos[2], node->pos[3]}).first;
        }
        sceVu0FVECTOR world = {it->second[0], it->second[1], it->second[2], 1.0f};
        sceVu0ApplyMatrix(world, const_cast<float(*)[4]>(matrix), world);
        node->pos[0] = world[0];
        node->pos[1] = world[1];
        node->pos[2] = world[2];
        ++moves;
    }
    return moves;
}

void CloseTalk(ClsMes *message) {
    if (message->select < 0) {
        message->cursor_time = 0;
    }
    message->select = -1;
    message->draw_speed = message->GetDrawSpeedDef();
    message->mes_no = -1;
    message->text_ptr = 0;
    message->open = 0;
    message->fade = 0.0f;
    message->fukidashi_centre_x = -1;
    message->fukidashi_centre_y = -1;
    message->fukidashi_pos = 0;
}

// Townspeople are never asked for the talk motion by DC2 (no event script runs for these ids) and a queued motion
// request is skipped while the villager is stayed, so the motion is set on the character directly and the villager is
// held with the same CScene::StayVillager the native talk events use. Both are undone when the window closes.
void TalkMotion(CScene *scene, int slot, int motion, const char *what) {
    CCharacter2 *chara = scene->GetCharacter(slot);
    if (!chara) {
        return;
    }
    SetCharaMotion(chara, motion, 0);
    std::fprintf(stderr, "ixion georama: talk %s slot=%d motion=%d now=%d\n", what, slot, motion,
                 GetMotionID(chara->GetNowMotionName()));
}

void TalkStep() {
    CScene *scene = GetMainScene();
    if (talk_id >= 0) {
        ClsMes *message = scene->GetMessage(1);
        SandboxLockChara = 1;
        if (!message) {
            talk_id = -1;
            SandboxLockChara = 0;
            return;
        }
        if (mgGetVSyncCount() - talk_frame > 20 && PadCtrl.Btn(PAD_BTN_CONFIRM)) {
            const int state = message->State();
            if (state == CLSMES_PAGE_WAIT) {
                message->GoNextPage();
            } else if (state == CLSMES_SHOWN) {
                CloseTalk(message);
                std::fprintf(stderr, "ixion georama: talk closed id=%d\n", talk_id);
                if (talk_slot >= 0) {
                    TalkMotion(scene, talk_slot, VILLAGER_MOTION_STAND, "motion end");
                    scene->CancelStayVillager(talk_slot);
                    talk_slot = -1;
                }
                talk_id = -1;
                SandboxLockChara = 0;
            }
        }
        return;
    }
    if (roster.empty() || SandboxPause || SandboxLockChara || SandboxControlMode != EDIT_CONTROL_PLAYER ||
        SandboxLoopMode != EDIT_LOOP_WALK || scene->event_run || !PadCtrl.Btn(PAD_BTN_CONFIRM)) {
        return;
    }
    CCharacter2 *player = scene->GetCharacter(scene->player_chara);
    if (!player) {
        return;
    }
    sceVu0FVECTOR   position, height;
    CSceneEventData event;
    player->GetPosition(position);
    player->GetPosition(height);
    position[3] = height[1];
    if (!scene->GetTalkEvent(position, &event) || !FindRoster(event.chara_no)) {
        return;
    }
    auto it = talk_lines.find(event.chara_no);
    ClsMes *message = scene->GetMessage(1);
    if (it == talk_lines.end() || it->second.empty() || !message) {
        return;
    }
    std::size_t &next = talk_next[event.chara_no];
    std::string  text = it->second[next++ % it->second.size()];
    message->Preset(4);
    message->SetWindowMode(4);
    message->MakeMesWin(text.data(), 1, 1);
    message->fukidashi_pos = 8;
    talk_id = event.chara_no;
    talk_frame = mgGetVSyncCount();
    SandboxLockChara = 1;
    talk_slot = event.chara_slot;
    scene->StayVillager(talk_slot);
    TalkMotion(scene, talk_slot, VILLAGER_MOTION_TALK, "motion start");
    std::fprintf(stderr, "ixion georama: talk opened id=%d slot=%d line=%zu text=\"%.60s\"\n", event.chara_no,
                 event.chara_slot, (next - 1) % it->second.size(), text.c_str());
}
} // namespace

// Called from CVillagerMngr::GetAppearVlgr (port/src/villagermngr.cpp) with the rows the NPC tables selected.
int IxionGeoramaFilterVillagers(int map_no, int *ids, CVillagerPlaceInfo **places, int found) {
    if (!enabled) {
        return found;
    }
    LoadRoster();
    int kept = 0;
    for (int i = 0; i < found; ++i) {
        const RosterEntry *entry = FindRoster(ids[i]);
        if (entry) {
            sceVu0FVECTOR position;
            float         yaw = 0;
            float         matrix[4][4];
            if (map_no != kSlot || !HousePose(*entry, position, yaw, matrix)) {
                std::fprintf(stderr, "ixion georama: villager id=%d (%s) skipped, house part %d not placed\n", entry->id,
                             entry->model.c_str(), entry->part);
                continue;
            }
            places[i]->pos[0] = position[0];
            places[i]->pos[1] = position[1];
            places[i]->pos[2] = position[2];
            places[i]->pos[3] = yaw;
            const int moves = PlaceHouseRoute(places[i], matrix);
            std::fprintf(stderr, "ixion georama: villager id=%d (%s) at house part %d pos=%.1f,%.1f,%.1f yaw=%.3f route_moves=%d\n",
                         entry->id, entry->model.c_str(), entry->part, position[0], position[1], position[2], yaw, moves);
        }
        ids[kept] = ids[i];
        places[kept] = places[i];
        ++kept;
    }
    return kept;
}

void IxionGeoramaStep() {
    if (enabled && SandboxMapNo == kSlot) {
        TalkStep();
        if (!roster.empty() && mgGetVSyncCount() % 50 == 0) {
            auto *scene = GetMainScene();
            for (int i = 0; i < scene->villager_mngr.data_num; ++i) {
                const CVillagerData *villager = scene->villager_mngr.GetData(i);
                if (!villager || villager->vlgr_id < 0 || !FindRoster(villager->vlgr_id)) {
                    continue;
                }
                CCharacter2 *chara = scene->GetCharacter(villager->chara_id);
                std::fprintf(stderr, "ixion georama: villager state frame=%d id=%d pos=%.1f,%.1f,%.1f stay=%d motion=%d route_time=%d\n",
                             mgGetVSyncCount(), villager->vlgr_id, villager->pos[0], villager->pos[1], villager->pos[2],
                             villager->stay, chara ? GetMotionID(chara->GetNowMotionName()) : -1, villager->route_time);
            }
        }
    }
    if (enabled && SandboxMapNo == kSlot && SandboxLoopMode == EDIT_LOOP_WALK && mgGetVSyncCount() % 50 == 0) {
        auto *scene = GetMainScene();
        auto *player = scene->GetCharacter(scene->player_chara);
        if (player) {
            sceVu0FVECTOR position;
            player->GetPosition(position);
            std::fprintf(stderr, "ixion georama: walk frame=%d pos=%.1f,%.1f,%.1f\n",
                         mgGetVSyncCount(), position[0], position[1], position[2]);
        }
    }
    if (!enabled || SandboxMapNo != kSlot || SandboxPause || SandboxLockChara ||
        SandboxControlMode != EDIT_CONTROL_PLAYER || !GamePad__2.Down(PAD_SELECT) ||
        GamePad__2.On(PAD_START) || GamePad__2.On(PAD_L2)) {
        return;
    }
    CScene      *scene = GetMainScene();
    CCharacter2 *player = scene->GetCharacter(scene->player_chara);
    if (!player || scene->event_run || NowEditModeChg()) {
        return;
    }
    sceVu0FVECTOR position;
    player->GetPosition(position);
    if (SandboxLoopMode == EDIT_LOOP_WALK && CheckWalkToEdit(scene, position)) {
        ResetViewMode(scene);
        scene->before_camera = scene->active_camera;
        scene->active_camera = 3;
        StartEditMode(scene);
        SandboxLoopMode = EDIT_LOOP_EDIT;
        KeepEditAnalyze();
        player->SetMotion("\x97\xA7\x82\xBF", 0);
        scene->map_event_no = 0;
        std::fprintf(stderr, "ixion georama: entered native editor\n");
    } else if (SandboxLoopMode == EDIT_LOOP_EDIT && CheckEditToWalk(scene, position)) {
        SandboxLoopMode = EDIT_LOOP_WALK;
        EndEditMode(scene, position);
        EditControlStatusInit(scene);
        CCameraControl *edit_camera = static_cast<CCameraControl *>(scene->GetCamera(scene->active_camera));
        CCameraControl *walk_camera = static_cast<CCameraControl *>(scene->GetCamera(0));
        if (edit_camera && walk_camera) {
            sceVu0FVECTOR eye, target;
            edit_camera->GetPos(eye);
            edit_camera->GetRef(target);
            eye[1] += 0.1f;
            walk_camera->FollowOff();
            walk_camera->SetRef(target);
            walk_camera->SetPos(eye);
            walk_camera->SetAngle(edit_camera->GetAngle());
            walk_camera->Step(-1);
            walk_camera->FollowOn();
        }
        scene->active_camera = 0;
        EditDataSave();
        // The roster is read at map entry; a house placed or moved this visit needs the NPC tables walked again.
        scene->LoadVillager(SandboxMapNo, 0x4E);
        std::fprintf(stderr, "ixion georama: left native editor pos=%.1f,%.1f,%.1f parts4=%d parts50=%d sandbox=%d\n",
                     position[0], position[1], position[2], sandbox.GetPartsNumID(4), sandbox.GetPartsNumID(50),
                     SandboxStockTotal(sandbox));
        for (const auto &part : sandbox.parts) {
            if (part.id == 4 || IxionGeoramaSandboxPart(part.id)) {
                std::fprintf(stderr, "ixion georama: placed id=%d pos=%d,%d,%d angle=%d\n",
                             part.id, part.pos[0], part.pos[1], part.pos[2], part.angle);
            }
        }
    }
}
