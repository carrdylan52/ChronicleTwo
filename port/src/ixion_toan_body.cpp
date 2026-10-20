#include "common.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <cstdlib>
#include <cstring>

#include "actionchara.hpp"
#include "actscript.hpp"
#include "character.hpp"
#include "charasetup.hpp"
#include "dataread.hpp"
#include "dataread_port.hpp"
#include "ixion_room_logging.hpp"
#include "dng_main.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "mg_texture.hpp"
#include "gamedata.hpp"
#include "maintex.hpp"
#include "menuchr.hpp"
#include "menumain.hpp"
#include "menusys.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "mglib_port.hpp"
#include "scene.hpp"
#include "userdata.hpp"

#include "ixion_toan_body.hpp"
#include "ixion_toan_bridge.h"
#include "ixion_toan_combat.hpp"

// Ixion: Dark Cloud's Toan (the converted dungeon body ix_c01d.chr and the Dagger ix_c01w01.chr,
// private packs served through DC2_IXION_EXTRA_DATA) stands in for Max's base, costume and shoes.
// Toan keeps his own skeleton, skin (.wgt), motions (.mot/.bbp) and texture; DC2 drives him only
// through KEY names (the pack aliases Max's names onto Toan's motions) and named frames
// (ef00 weapon socket, keri_L floor point, bcol00/01 hurt spheres). Max's c01.stb still runs the
// controller. Max's costume and shoe LoadSkin are skipped (their same-named meshes would be
// copied over Toan's kao/kaban), and the gun and hat are never loaded, so SetupMints' gun_hand and
// hat re-binds find no part and do nothing.

// Ixion Goro R2: the same seams take Goro (DC2_IXION_PLAYER_BODY=goro): the converted c06b body (ix_c06b.chr,
// from the BodyProfile converter) and his hammer. Everything below is per selected row; nothing else differs.

namespace {

// One DC1 body DC2_IXION_PLAYER_BODY can select: the converter's pack names (monster_converter.py
// convert_player_body: "<prefix><stem>.chr" holding "<prefix><stem>.cfg") and the DC1 character id
// (IXION_DC1_CHARA_*) the combat bridge runs.
struct Dc1BodyRow {
    const char *name;
    int         character;
    const char *body_pack;
    const char *body_cfg;
    const char *weapon_pack;
    const char *weapon_cfg;
    // Xiao R1: the scene stack the expanded DC1 motion goes to. Max's costume stack 1 (40000 quadwords,
    // charasetup.cpp:55) holds Toan's (21697 used), Goro's and Ungaga's; Xiao's c04b.mot (1.34 MB, 568 frames) needs
    // 40536, so hers shares the base stack 0 (68500, about 8000 used by the model and texture).
    int         motion_stack = 1;
};
constexpr Dc1BodyRow kDc1Bodies[] = {
    {"toan", 0, "mainchr/ix_c01d.chr", "ix_c01d.cfg", "mainchr/ix_c01w01.chr", "ix_c01w01.cfg"},
    {"goro", 2, "mainchr/ix_c06b.chr", "ix_c06b.cfg", "mainchr/ix_c06w01.chr", "ix_c06w01.cfg"},
    // Ungaga R1: the converted c10b body (BodyProfile ungaga-c10b) and his Fighting Stick c10w01.
    {"ungaga", 4, "mainchr/ix_c10b.chr", "ix_c10b.cfg", "mainchr/ix_c10w01.chr", "ix_c10w01.cfg"},
    // Xiao R1: the converted c04b body (BodyProfile xiao-c04b) and her Wooden Slingshot c04w01, whose eff30 is her
    // shot origin.
    {"xiao", 1, "mainchr/ix_c04b.chr", "ix_c04b.cfg", "mainchr/ix_c04w01.chr", "ix_c04w01.cfg", 0},
};

const Dc1BodyRow *SelectedBody() {
    static const Dc1BodyRow *row = [] {
        const char *body = std::getenv("DC2_IXION_PLAYER_BODY");
        for (const Dc1BodyRow &r : kDc1Bodies) {
            if (body != nullptr && std::strcmp(body, r.name) == 0) return &r;
        }
        if (body != nullptr) std::fprintf(stderr,
            "ixion player body refused: DC2_IXION_PLAYER_BODY=%s; expected toan|goro|ungaga|xiao\n", body);
        return static_cast<const Dc1BodyRow *>(nullptr);
    }();
    return row;
}

const char *BodyPack() { return SelectedBody()->body_pack; }
const char *BodyCfg() { return SelectedBody()->body_cfg; }
const char *WeaponPack() { return SelectedBody()->weapon_pack; }
const char *WeaponCfg() { return SelectedBody()->weapon_cfg; }

// The last Toan body set up (traced), and every live one (the dungeon and event scenes each set
// one up through SetupMainUnit).
CCharacter2  *toan_body = nullptr;
CCharacter2  *toan_bodies[4] = {};
CActionChara *toan_daggers[4] = {};
const CSceneCharacter *toan_scene_slots[4] = {};
// Each Toan setup stamps its slot with a new generation: the combat bridge re-binds whenever the stamp changes,
// even when the scene slot (body pointer) is the same (quick change to Ridepod or Monica and back).
int toan_generations[4] = {};
int toan_setups = 0;

// What Toan's key list looked like when it was set up. The dungeon quick change (R3 to the Ridepod and back,
// L3 to Monica and back) reloads the scene slot through the menu loader (menuchr.cpp MenuItemCharaDataLoadPack),
// not SetupMainUnit, so the slot can hold native Max again at the same address. The content tells them apart.
struct ToanSignature {
    int key_num = 0;
    int key36_start = 0;
    int key36_end = 0;
    int key14_start = 0;
};
ToanSignature toan_signatures[4] = {};

ToanSignature SignatureOf(CCharacter2 *body) {
    ToanSignature sig;
    const CHRINFO_KEY_SET *keys = body->key_list[0];
    sig.key_num = body->key_num[0];
    if (keys != nullptr && sig.key_num > 36) {
        sig.key36_start = keys[36].start_frame;
        sig.key36_end = keys[36].end_frame;
        sig.key14_start = keys[14].start_frame;
    }
    return sig;
}

void Stamp(int i, CCharacter2 *body, CActionChara *dagger) {
    toan_bodies[i] = body;
    toan_daggers[i] = dagger;
    toan_generations[i] = toan_setups;
    toan_signatures[i] = SignatureOf(body);
}

void RememberToanBody(CCharacter2 *body, CActionChara *dagger) {
    toan_body = body;
    ++toan_setups;
    for (int i = 0; i < 4; i++) {
        if (toan_bodies[i] == body) {
            Stamp(i, body, dagger);
            return;
        }
    }
    for (int i = 0; i < 4; i++) {
        if (toan_bodies[i] == nullptr) {
            Stamp(i, body, dagger);
            return;
        }
    }
    Stamp(0, body, dagger);
}

// The slot of a live Toan body, or -1. A slot whose key list no longer matches Toan's (reloaded by the menu
// quick change) is forgotten here, so nothing keeps treating native Max as Toan.
int LiveToanSlot(CCharacter2 *body) {
    for (int i = 0; body != nullptr && i < 4; i++) {
        if (toan_bodies[i] != body) continue;
        const ToanSignature now = SignatureOf(body);
        const ToanSignature &was = toan_signatures[i];
        if (now.key_num == was.key_num && now.key36_start == was.key36_start && now.key36_end == was.key36_end && // gitleaks:allow - model signature comparison
            now.key14_start == was.key14_start) {
            return i;
        }
        std::fprintf(stderr,
                     "ixion toan body: frame=%d slot %d reloaded outside SetupMainUnit (keys %d, key36 %d-%d; Toan had "
                     "%d, %d-%d): no longer Toan, released\n",
                     mgPortFrameCount(), i, now.key_num, now.key36_start, now.key36_end, was.key_num, was.key36_start,
                     was.key36_end);
        IxionToanCombatRetire(body, "rig-signature-changed");
        toan_bodies[i] = nullptr;
        toan_daggers[i] = nullptr;
        toan_generations[i] = 0;
        toan_scene_slots[i] = nullptr;
        toan_signatures[i] = {};
        if (toan_body == body) toan_body = nullptr;
        return -1;
    }
    return -1;
}

void ForgetToanBody(CCharacter2 *body) {
    IxionToanCombatRetire(body, "body-retired");
    for (int i = 0; i < 4; i++) {
        if (toan_bodies[i] == body) {
            toan_generations[i] = 0;
            toan_bodies[i] = nullptr;
            toan_daggers[i] = nullptr;
            toan_scene_slots[i] = nullptr;
        }
    }
    if (toan_body == body) toan_body = nullptr;
}

bool IsToanBody(CCharacter2 *body) {
    return LiveToanSlot(body) >= 0;
}

bool ToanPacksPresent() {
    const bool body = DataReadFind(BodyPack()).has_value();
    const bool dagger = DataReadFind(WeaponPack()).has_value();
    if (!body || !dagger) {
        std::fprintf(stderr, "ixion toan body: pack missing body=%d dagger=%d; Max loads instead\n", body ? 1 : 0,
                     dagger ? 1 : 0);
    }
    return body && dagger;
}

int FrameIndex(mgCFrame *root, mgCFrame *frame) {
    for (int i = 0; frame != NULL && i < root->frame_num; i++) {
        if (root->frame_list[i] == frame) return i;
    }
    return -1;
}

// DC1's .bbp holds each frame's bind matrix relative to its parent: DC1's MotionProc2 chains
// parent.bone_base * base_matrices[frame] from the skin owner's parent (identity there,
// Chronicle port/src/gameutil_motion.cpp:87-89). DC2 reads the same table (copied over
// init_matrix by _MOTION, character.cpp:2359-2360) as model-space matrices (mg_dataset.cpp:760-766
// fills it from GetLWMatrix), and its skinning uses inv(base[skin]) * base[bone]
// (visualmotion.cpp CreateExtRenderInfoPacket). Chaining the DC1 locals down the frame tree gives
// DC2 that form. The skin owner's entry becomes its parent's, so the skin frame's own bind matrix
// drops out exactly as in DC1, which starts its chain at the skin's parent.
// The shadow model skins on the CPU (DeformMesh) from shadow_motion[0].base_matrices, a copy of
// ix_c01s.bbp with the same DC1 local form, so it is chained the same way.
int AccumulateDc1BindPose(mgCFrame *root, float (*matrices)[4][4], const int *skins, int skin_num) {
    if (root == NULL || matrices == NULL || root->frame_list == NULL || root->frame_num <= 0 ||
        root->frame_num > 256) {
        return 0;
    }
    static float local[256][4][4];
    const int    count = root->frame_num;
    std::memcpy(local, matrices, sizeof(float[4][4]) * count);
    int out_of_order = 0;
    for (int i = 0; i < count; i++) {
        const int parent = FrameIndex(root, root->frame_list[i]->parent);
        if (parent >= 0 && parent < i) {
            mgMulMatrix(matrices[i], matrices[parent], local[i]);
        } else {
            std::memcpy(matrices[i], local[i], sizeof(float[4][4]));
            out_of_order += parent >= i ? 1 : 0;
        }
    }
    for (int k = 0; k < skin_num; k++) {
        const int skin = skins[k];
        if (skin < 0 || skin >= count) continue;
        const int parent = FrameIndex(root, root->frame_list[skin]->parent);
        if (parent >= 0) {
            std::memcpy(matrices[skin], matrices[parent], sizeof(float[4][4]));
        }
        if (skin >= 0) {
            const float(*m)[4] = local[skin];
            std::fprintf(stderr,
                         "ixion toan body: skin frame=%d parent=%d dc1 bbp row0=(%.3f,%.3f,%.3f) row3=(%.3f,%.3f,%.3f)\n",
                         skin, parent, m[0][0], m[0][1], m[0][2], m[3][0], m[3][1], m[3][2]);
        }
    }
    return out_of_order == 0 ? count : -count;
}

void SetupToanBody(u_long128 *read_buffer, mgCMemory *memory, mgCMemory *stacks, int texture, CActionChara **parts,
                   CScene *scene, CUserDataManager *user_data, int edit_mode) {
    char path[64];
    GetCharaMemAllocPtr(memory, stacks, USER_CHARA_MAX, edit_mode);
    parts[0]->Initialize(stacks);
    int body_size = 0;
    std::snprintf(path, sizeof(path), "%s", BodyPack());
    LoadFile(path, read_buffer, &body_size);
    parts[0]->accume_effect = &AccumulateEffect;
    // Max's costume stack (1) is free here: the expanded DC1 motion goes there, the model and
    // texture into the base stack (0).
    parts[0]->LoadPack((unsigned int *) read_buffer, const_cast<char *>(BodyCfg()), &stacks[0],
                       &stacks[SelectedBody()->motion_stack], &stacks[0], texture, NULL);
    // Ixion R5: the fresh key list plays the donor KEYs at Dark Cloud's rate (NormalDrive's 1.2 cancelled),
    // once per load, so a reload into the same slot never inherits or doubles the rescale.
    // Goro R2: the rescaled KEYs are the selected character's own (the library table's donor_keys).
    IxionToanCombatRescaleKeys(parts[0]);
    parts[0]->SetPosition(0.0f, 0.0f, 200.0f);
    parts[0]->texture_block = texture;
    const char *bind = std::getenv("DC2_IXION_TOAN_BIND");
    int         bind_body = 0;
    int         bind_shadow = 0;
    if (bind == NULL || std::strcmp(bind, "raw") != 0) {
        mgCFrame *root = parts[0]->CObjectFrame::frame;
        int       skins[8];
        int       skin_num = 0;
        for (int k = 0; root != NULL && k < parts[0]->deform_frame_num && skin_num < 8; k++) {
            skins[skin_num++] = FrameIndex(root, parts[0]->deform_frame[k]);
        }
        bind_body = AccumulateDc1BindPose(root, root != NULL ? root->init_matrix : NULL, skins, skin_num);
        skin_num = 0;
        for (Mot_List *list = parts[0]->shadow_motion[0].skin_list; list != NULL && skin_num < 8; list = list->next) {
            bool seen = false;
            for (int k = 0; k < skin_num; k++) seen = seen || skins[k] == list->frame;
            if (!seen) skins[skin_num++] = list->frame;
        }
        bind_shadow = AccumulateDc1BindPose(parts[0]->shadow_frame, parts[0]->shadow_motion[0].base_matrices, skins,
                                            skin_num);
    }
    std::fprintf(stderr, "ixion toan body: dc1 bind pose chained body=%d shadow=%d (negative: out-of-order parents)\n",
                 bind_body, bind_shadow);

    // Ixion R6: in town (edit_mode != 0) Max carries no weapon and runs no action script: SetupMainUnit's town
    // path skips both (charasetup.cpp:383-407, :421-426), and EditControl drives the body by motion names. Toan
    // follows it: no Dagger, no sword blur, no c01.stb, so the DC1 combat bridge (which needs the Dagger) stays off.
    const bool town = edit_mode != 0;
    int        dagger_size = 0;
    int        dagger_attached = 0;
    if (!town) {
        parts[1]->Initialize(NULL);
        std::snprintf(path, sizeof(path), "%s", WeaponPack());
        LoadFile(path, read_buffer, &dagger_size);
        parts[1]->LoadPack((unsigned int *) read_buffer, const_cast<char *>(WeaponCfg()), &stacks[2], &stacks[2],
                           &stacks[2], texture, parts[0]);
        dagger_attached = parts[0]->SetRef(parts[1], const_cast<char *>("ef00"));
        SetSwordBlurEffect(parts[0], &stacks[2], USER_CHARA_MAX);

        int script_size = 0;
        LoadFile(const_cast<char *>("mainchr/c01.stb"), read_buffer, &script_size);
        parts[0]->LoadActionFile((char *) read_buffer, script_size, &stacks[6]);
        parts[0]->InitScript();
    }

    SetupUnitMan(scene, user_data, USER_CHARA_MAX, NULL);
    // SetupMints re-binds the melee part only when a melee weapon is equipped.
    CGameDataUsed *equip = user_data->GetCharaDataPtr(USER_CHARA_MAX)->equip;
    if (!town && equip[CHARA_EQUIP_MELEE].item_no <= 0 && parts[0]->SetRef(parts[1], const_cast<char *>("ef00"))) {
        std::strcpy(parts[1]->name, "sword");
        parts[1]->CopyOutLine(parts[0]);
    }
    parts[0]->chara_type = ACTION_CHARA_MAX;
    parts[0]->move_type = ACTION_MOVE_HUMAN;
    IxionToanBodyRegister(parts[0], town ? nullptr : parts[1], scene);

    std::fprintf(stderr,
                 "ixion toan body: loaded %s %s body=%s (%d B) dagger=%s (%d B) keys=%d poly=%d deform=%d scale=%.4f "
                 "ef00=%d keri_L=%d hat=%d gun_hand=%d dagger_attached=%d melee_item=%d sub_item=%d head_item=%d "
                 "stack0=%d/%d stack1=%d/%d stack2=%d/%d\n",
                 SelectedBody()->name, town ? "town" : "dungeon", BodyPack(), body_size, town ? "none" : WeaponPack(), dagger_size, parts[0]->key_num[0], parts[0]->poly_num,
                 parts[0]->deform_frame_num, parts[0]->base_scale[0],
                 parts[0]->SearchObject(const_cast<char *>("ef00")) != NULL ? 1 : 0,
                 parts[0]->SearchObject(const_cast<char *>("keri_L")) != NULL ? 1 : 0,
                 parts[0]->SearchObject(const_cast<char *>("hat")) != NULL ? 1 : 0,
                 parts[0]->SearchObject(const_cast<char *>("gun_hand")) != NULL ? 1 : 0, dagger_attached,
                 equip[CHARA_EQUIP_MELEE].item_no, equip[CHARA_EQUIP_SUB].item_no, equip[CHARA_EQUIP_HEAD].item_no,
                 stacks[0].stack_used, stacks[0].stack_size, stacks[1].stack_used, stacks[1].stack_size,
                 stacks[2].stack_used, stacks[2].stack_size);
}

} // namespace

bool IxionToanBodyEnabled() { return SelectedBody() != nullptr; }

int IxionDc1BodyCharacter() { return SelectedBody() != nullptr ? SelectedBody()->character : 0; }

void IxionToanBodyRegister(CCharacter2 *body, CActionChara *dagger, CScene *scene) {
    if (!IxionToanBodyEnabled() || body == nullptr) return;
    IxionToanCombatRetire(body, "rig-registered");
    RememberToanBody(body, dagger);
    if (scene != nullptr) {
        for (int i = 0; i < 4; ++i) {
            if (toan_bodies[i] == body) toan_scene_slots[i] = scene->GetSceneCharacter(0);
        }
    }
}

void IxionToanBodyRetireSceneSlot(const CSceneCharacter *slot) {
    for (int i = 0; i < 4; ++i) {
        if (toan_scene_slots[i] == slot) ForgetToanBody(toan_bodies[i]);
    }
}

namespace {
bool SlotOwns(int slot, CCharacter2 *target) {
    if (target == nullptr || toan_bodies[slot] == nullptr) return false;
    if (target == toan_bodies[slot] || target == toan_daggers[slot]) return true;
    // Setup registers action bodies; walk only their live attachment chain.
    auto *part = static_cast<CActionChara *>(toan_bodies[slot])->next;
    for (int n = 0; part != nullptr && n < 32; ++n, part = part->next) {
        if (part == target) return true;
    }
    return false;
}
} // namespace

bool IxionToanBodyOwns(CCharacter2 *target) {
    for (int i = 0; i < 4; ++i) {
        if (SlotOwns(i, target)) return true;
    }
    return false;
}

void IxionToanBodyRetire(CCharacter2 *target) {
    for (int i = 0; target != nullptr && i < 4; ++i) {
        if (SlotOwns(i, target)) {
            ForgetToanBody(toan_bodies[i]);
        }
    }
}

int IxionToanBodyGeneration(CCharacter2 *body) {
    const int slot = LiveToanSlot(body);
    return slot >= 0 ? toan_generations[slot] : 0;
}

CActionChara *IxionToanBodyDagger(CCharacter2 *body) {
    const int slot = LiveToanSlot(body);
    return slot >= 0 ? toan_daggers[slot] : nullptr;
}

void IxionToanBodyTrace(CCharacter2 *character) {
    if (character == nullptr || character != toan_body) {
        return;
    }
    static IxionRoomTraceLog       trace_log;
    const int                     now = mgPortFrameCount();
    const CHRINFO_KEY_SET        *key = character->now_key;
    if (!trace_log.Observe(now)) {
        return;
    }
    float position[4] = {};
    character->GetPosition(position);
    int index = -1;
    if (key != nullptr && character->key_list[character->now_set] != nullptr) {
        index = static_cast<int>(key - character->key_list[character->now_set]);
    }
    std::fprintf(stderr, "ixion toan body: frame=%d pos=(%.2f,%.2f,%.2f) key=%d range=%d-%d mframe=%.2f step=%.3f\n",
                 now, position[0], position[1], position[2], index, key ? key->start_frame : -1,
                 key ? key->end_frame : -1, character->frame, character->step);
}

// Verbatim ps2/src/charasetup.cpp SetupMainUnit, plus the opt-in Toan branch in place of Max's.
PORT_OVERRIDE int SetupMainUnit(u_long128 *read_buffer, mgCMemory *memory, mgCMemory *stacks, int image_block,
                                CScene *scene, CUserDataManager *user_data, int chara_type, int edit_mode) {
    CActionChara *parts[6];
    int           texture = image_block;

    for (int character_index = 0; character_index < 6; ++character_index) {
        parts[character_index] = (CActionChara *) scene->GetCharacter(character_index);

        if (parts[character_index] == NULL) {
            return false;
        }

        parts[character_index]->Initialize(NULL);
    }

    // Ixion R6: towns (edit_mode 1, editloop.cpp:847 and the town quick change) take Toan too.
    if (chara_type == USER_CHARA_MAX && IxionToanBodyEnabled() && ToanPacksPresent()) {
        SetupToanBody(read_buffer, memory, stacks, texture, parts, scene, user_data, edit_mode);
        chara_type = -1;
    } else {
        ForgetToanBody(parts[0]);
    }

    if (chara_type == USER_CHARA_MAX) {
        char path[32];
        char model_name[32];
        GetCharaMemAllocPtr(memory, stacks, USER_CHARA_MAX, edit_mode);
        parts[0]->Initialize(stacks);
        GetMainCharaModelName(USER_CHARA_MAX, model_name, edit_mode);

        if (edit_mode != 0) {
            sprintf(path, "chara/%s", model_name);
            LoadFile(path, read_buffer, NULL);
        } else {
            sprintf(path, "mainchr/%s", model_name);
            LoadFile(path, read_buffer, NULL);
        }

        parts[0]->accume_effect = &AccumulateEffect;
        parts[0]->LoadPack((unsigned int *) read_buffer, "info.cfg", stacks, stacks, stacks, texture, NULL);
        parts[0]->SetPosition(0.0f, 0.0f, 200.0f);
        parts[0]->texture_block = texture;
        CGameDataUsed *equip = user_data->GetCharaDataPtr(chara_type)->equip;
        char          *item_path;
        LoadFile(GetItemFilePath(equip[CHARA_EQUIP_CLOTHES].item_no, 0), read_buffer, NULL);
        parts[0]->LoadSkin((unsigned int *) read_buffer, "info.cfg", "", &stacks[1], texture);

        if (!edit_mode) {
            if (equip[CHARA_EQUIP_MELEE].item_no > 0) {
                item_path = GetItemFilePath(equip[CHARA_EQUIP_MELEE].item_no, 0);
                parts[1]->Initialize(NULL);
                LoadFile(item_path, read_buffer, NULL);
                parts[1]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[2], &stacks[2], &stacks[2], texture, parts[0]);

                if (!parts[0]->SetRef(parts[1], "ef00")) {
                    printf("err wep1\n");
                }
            }

            if (equip[CHARA_EQUIP_SUB].item_no > 0) {
                item_path = GetItemFilePath(equip[CHARA_EQUIP_SUB].item_no, 0);
                parts[2]->Initialize(NULL);
                LoadFile(item_path, read_buffer, NULL);
                parts[2]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[3], &stacks[3], &stacks[3], texture, parts[0]);

                if (!parts[0]->SetRef(parts[2], "gun_hand")) {
                    printf("err wep1\n");
                }
            }

            SetSwordBlurEffect(parts[0], &stacks[2], chara_type);
        }

        item_path = GetItemFilePath(equip[CHARA_EQUIP_HEAD].item_no, 0);
        parts[3]->Initialize(NULL);
        LoadFile(item_path, read_buffer, NULL);
        parts[3]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[4], &stacks[4], &stacks[4], texture, parts[0]);

        if (!parts[0]->SetRef(parts[3], "hat")) {
            printf("err wep1\n");
        }

        LoadFile(GetItemFilePath(equip[CHARA_EQUIP_FEET].item_no, 0), read_buffer, NULL);
        parts[0]->LoadSkin((unsigned int *) read_buffer, "info.cfg", "", &stacks[5], texture);

        if (!edit_mode) {
            int file_size;
            LoadFile("mainchr/c01.stb", read_buffer, &file_size);
            parts[0]->LoadActionFile((char *) read_buffer, file_size, &stacks[6]);
            parts[0]->InitScript();
        }

        SetupUnitMan(scene, user_data, USER_CHARA_MAX, NULL);
        parts[0]->chara_type = ACTION_CHARA_MAX;
        parts[0]->move_type = ACTION_MOVE_HUMAN;
    }

    if (chara_type == USER_CHARA_MONICA) {
        char path[32];
        char model_name[32];
        GetCharaMemAllocPtr(memory, stacks, USER_CHARA_MAX, edit_mode);
        parts[0]->Initialize(stacks);
        GetMainCharaModelName(USER_CHARA_MONICA, model_name, edit_mode);

        if (edit_mode != 0) {
            sprintf(path, "chara/%s", model_name);
            LoadFile(path, read_buffer, NULL);
        } else {
            sprintf(path, "mainchr/%s", model_name);
            LoadFile(path, read_buffer, NULL);
        }

        parts[0]->accume_effect = &AccumulateEffect;
        parts[0]->LoadPack((unsigned int *) read_buffer, "info.cfg", stacks, stacks, stacks, texture, NULL);
        parts[0]->SetPosition(0.0f, 0.0f, 200.0f);
        parts[0]->texture_block = texture;
        CGameDataUsed *equip = user_data->GetCharaDataPtr(chara_type)->equip;
        char          *item_path;
        LoadFile(GetItemFilePath(equip[CHARA_EQUIP_CLOTHES].item_no, 0), read_buffer, NULL);
        parts[0]->LoadSkin((unsigned int *) read_buffer, "info.cfg", "", &stacks[1], texture);

        if (!edit_mode) {
            item_path = GetItemFilePath(equip[CHARA_EQUIP_MELEE].item_no, 0);
            parts[1]->Initialize(NULL);
            LoadFile(item_path, read_buffer, NULL);
            parts[1]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[2], &stacks[2], &stacks[2], texture, parts[0]);

            if (!parts[0]->SetRef(parts[1], "sword_hand")) {
                printf("err wep1\n");
            }

            SetSwordBlurEffect(parts[0], &stacks[2], chara_type);
        }

        item_path = GetItemFilePath(equip[CHARA_EQUIP_SUB].item_no, 0);
        parts[2]->Initialize(NULL);
        LoadFile(item_path, read_buffer, NULL);
        parts[2]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[3], &stacks[3], &stacks[3], texture, parts[0]);

        if (!parts[0]->SetRef(parts[2], "wr")) {
            printf("err wep1\n");
        }

        item_path = GetItemFilePath(equip[CHARA_EQUIP_HEAD].item_no, 0);
        parts[3]->Initialize(NULL);
        LoadFile(item_path, read_buffer, NULL);
        parts[3]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[4], &stacks[4], &stacks[4], texture, parts[0]);

        if (!parts[0]->SetRef(parts[3], "ac")) {
            printf("err wep1\n");
        }

        LoadFile(GetItemFilePath(equip[CHARA_EQUIP_FEET].item_no, 0), read_buffer, NULL);
        parts[0]->LoadSkin((unsigned int *) read_buffer, "info.cfg", "", &stacks[5], texture);

        if (!edit_mode) {
            int file_size;
            LoadFile("mainchr/c02.stb", read_buffer, &file_size);
            parts[0]->LoadActionFile((char *) read_buffer, file_size, &stacks[6]);
            parts[0]->InitScript();
        }

        SetupUnitMan(scene, user_data, USER_CHARA_MONICA, NULL);
        parts[0]->chara_type = ACTION_CHARA_MONICA;
        parts[0]->move_type = ACTION_MOVE_HUMAN;
    }

    if (chara_type == USER_CHARA_ROBO) {
        char path[64];
        GetCharaMemAllocPtr(memory, stacks, USER_CHARA_ROBO, edit_mode);
        ROBO_INFO_DATA *robo_info = GetRoboPartsInfo(user_data);
        parts[0]->Initialize(NULL);
        sprintf(path, "dungeon/robo/%s.chr", robo_info->model_name[ROBO_MODEL_LEG]);
        LoadFile(path, read_buffer, NULL);
        parts[0]->LoadPack((unsigned int *) read_buffer, "info.cfg", stacks, stacks, stacks, texture, NULL);
        parts[0]->texture_block = texture;
        const int part_stack[5] = {0, 1, 2, 2, 3};

        for (int i = ROBO_MODEL_ARM; i < ROBO_MODEL_NUM; ++i) {
            parts[i]->Initialize(NULL);

            if (i != ROBO_MODEL_MINTS) {
                sprintf(path, "dungeon/robo/%s.chr", robo_info->model_name[i]);
            } else {
                sprintf(path, "dungeon/robo/%s", robo_info->model_name[i]);
            }

            LoadFile(path, read_buffer, NULL);
            parts[i]->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[part_stack[i]],
                               &stacks[part_stack[i]], &stacks[part_stack[i]], texture, parts[0]);

            if (i == ROBO_MODEL_ARM) {
                SetSwordBlurEffect(parts[0], &stacks[part_stack[i]], chara_type);
            }
        }

        CActionChara *hat = parts[5];
        hat->Initialize(NULL);
        LoadFile(robo_info->hat_file, read_buffer, NULL);
        hat->LoadPack((unsigned int *) read_buffer, "info.cfg", &stacks[2], &stacks[2], &stacks[2], texture, parts[0]);
        SetupUnitMan(scene, user_data, USER_CHARA_ROBO, robo_info);
        int file_size;
        LoadFile("dungeon/act_script/robo.stb", read_buffer, &file_size);
        parts[0]->LoadActionFile((char *) read_buffer, file_size, &stacks[4]);
        parts[0]->InitScript();
        parts[0]->move_type = robo_info->move_type;
        parts[0]->attack_type = robo_info->attack_type;
        parts[0]->chara_type = ACTION_CHARA_ROBO;
    }

    if (chara_type == USER_CHARA_MONSTER) {
        char monster_path[64];
        char monster_config[64];
        char monster_script[64];
        char monster_model[32];
        int  monster_id = user_data->monster_id;
        GetCharaMemAllocPtr(memory, stacks, USER_CHARA_MONSTER, edit_mode);
        GetMonsterModelFile(monster_id, MONSTER_FILE_MODEL, monster_model);
        sprintf(monster_path, "dungeon/monster/%s", monster_model);
        GetMonsterModelFile(monster_id, MONSTER_FILE_CONFIG, monster_config);
        GetMonsterModelFile(monster_id, MONSTER_FILE_SCRIPT, monster_model);
        sprintf(monster_script, "dungeon/act_script/%s", monster_model);
        LoadFile(monster_path, read_buffer, NULL);
        parts[0]->Initialize(NULL);
        parts[0]->LoadPack((unsigned int *) read_buffer, monster_config, stacks, stacks, stacks, texture, NULL);
        parts[0]->SetPosition(0.0f, 0.0f, 0.0f);
        parts[0]->texture_block = texture;
        SetupUnitMan(scene, user_data, USER_CHARA_MONSTER, NULL);
        int file_size;
        LoadFile(monster_script, read_buffer, &file_size);
        parts[0]->LoadActionFile((char *) read_buffer, file_size, &stacks[5]);
        parts[0]->InitScript();
        parts[0]->chara_type = ACTION_CHARA_MAX;
        parts[0]->move_type = ACTION_MOVE_MONSTER;
    }

    int i = 0;

    do {
        printf("[%d]STACK %d/%d\n", i, stacks[i].stack_used, stacks[i].stack_size);
        ++i;
    } while (i < CHARA_MEM_STACK_NUM);

    return true;
}

namespace {

std::uintmax_t PackBytes(const char *name) {
    const auto found = DataReadFind(name);
    std::error_code error;
    const std::uintmax_t size = found ? std::filesystem::file_size(*found, error) : 0;
    return error ? 0 : size;
}

// Ixion R6: the quick change (R3 to the Ridepod and back, L3 to Monica and back; dng_main.cpp:3044-3065) reloads the
// player's scene slot through the menu loader (InitMainCharaBG menuchr.cpp:6181, MenuItemCharaDataLoadEndCheck,
// static MenuItemCharaDataLoadPack), which never reaches SetupMainUnit. Once that loader has applied Max's last
// part (request_phase 2), the slot is set up again through SetupMainUnit, the same way the menu's manual-movie
// return does it (menuop.cpp:655-661): Toan's branch runs, with its own carve of the scene stacks (the layout
// InitMainCharaBG carved, menuchr.cpp:6253) and a fresh texture block, so Max's costume and shoe skins never merge
// into Toan and the combat bridge sees a new generation. Any other character releases slot 0.
void ReloadToanAfterQuickChange(int chara_no) {
    CActionChara *player = MenuMainScene != NULL ? (CActionChara *) MenuMainScene->GetCharacter(0) : NULL;
    if (player == NULL) {
        return;
    }
    if (chara_no != USER_CHARA_MAX) {
        if (LiveToanSlot(player) >= 0) {
            std::fprintf(stderr, "ixion toan body: frame=%d quick change to chara %d: Toan released\n",
                         mgPortFrameCount(), chara_no);
        }
        ForgetToanBody(player);
        return;
    }
    if (!ToanPacksPresent()) {
        return;
    }
    // The menu's read buffers are spent here: ReadMainCharaBG resets MenuCharaLoadStack right after (menuchr.cpp
    // phase 1), so its whole length is the read buffer.
    MenuCharaLoadStack.stReset();
    MenuCharaLoadStack.Align64();
    const std::uintmax_t rest = (std::uintmax_t) MenuCharaLoadStack.stGetRest() * sizeof(u_long128);
    const std::uintmax_t need =
        std::max({PackBytes(BodyPack()), PackBytes(WeaponPack()), PackBytes("mainchr/c01.stb")});
    if (need == 0 || rest < need) {
        std::fprintf(stderr, "ixion toan body: quick change read buffer %ju B < %ju B; Max stays\n", rest, need);
        MenuCharaLoadStack.stReset();
        return;
    }
    const int town = MenuLoadInfo.town_model ? 1 : 0;
    std::fprintf(stderr, "ixion toan body: frame=%d quick change back to Max (%s): setting Toan up again\n",
                 mgPortFrameCount(), town ? "town" : "dungeon");
    mgTexManager.DeleteBlock(MenuArg.chara_tex_block);
    SetupMainUnit(MenuCharaLoadStack.stGetTop(), MenuArg.chara_stack, MorattaStack, MenuArg.chara_tex_block,
                  MenuMainScene, MenuUserDataManPtr, USER_CHARA_MAX, town);
    MenuCharaLoadStack.stReset();
}

} // namespace

// Verbatim ps2/src/menuchr.cpp MenuItemCharaDataLoadEndCheckAfter, plus the opt-in Toan reload (marked Ixion) at the
// end of the scene load a quick change makes.
PORT_OVERRIDE void MenuItemCharaDataLoadEndCheckAfter(MENU_BGREAD_INFO2 **info, int chara_no) {
    /**
     *
     * Scene character slot supplied by each party member loading slot.
     *
     */
    static s8 convtbl[USER_CHARA_NUM][MENU_CHARA_LOAD_MAX] = {
        {0, 2,  3,  4,  5,  6,  -1},
        {0, 2,  3,  4,  5,  6,  -1},
        {0, 1,  2,  3,  4,  5,  -1},
        {0, -1, -1, -1, -1, -1, -1}
    };

    CUserDataManager *user_data = MenuUserDataManPtr;
    ROBO_INFO_DATA   *robo = GetRoboPartsInfo(user_data);

    if (MenuLoadInfo.update_scene) {
        SetupUnitMan(MenuMainScene, user_data, chara_no, robo);
    }

    switch (MenuLoadInfo.mode) {
        case MENU_LOAD_MODE_SCENE:
            // Ixion: Toan comes back when the quick change returns to Max.
            if (MenuLoadInfo.update_scene && IxionToanBodyEnabled()) {
                ReloadToanAfterQuickChange(chara_no);
            }
            return;
    }

    CScene       scene;
    CCharacter2 *chara;

    for (int i = 0; i < MENU_CHARA_LOAD_MAX; i++) {
        chara = NULL;
        int slot = convtbl[chara_no][i];

        if (0 <= slot && info[slot] != NULL) {
            chara = info[slot]->chara;
        }

        scene.DeleteChara(i);
        scene.AssignChara(i, chara, NULL);
    }

    SetupUnitMan(&scene, user_data, chara_no, robo);
}

// Verbatim ps2/src/event_func.cpp _LOAD_MOTION_sub, except that an event's extra motion pack is not
// loaded into a Toan body: those packs (e100_mints.chr and the like) are authored for Max's
// 124-frame skeleton, and MotionProc writes past Toan's 77 frames (crash in SetTransMatrix, seen at
// the e100 dungeon-entry event). Toan keeps his own motions; an event motion name then starts nothing.
PORT_OVERRIDE int _LOAD_MOTION_sub(int stack_no, char *name, int chara_no, u32 *pack) {
    char         label[0x20];
    mgCMemory   *stack = EventScene->GetStack(stack_no);
    CCharacter2 *chara = EventScene->GetCharacter(chara_no);

    if (chara == NULL) {
        return false;
    }

    if (IxionToanBodyOwns(chara)) {
        std::fprintf(stderr, "ixion toan body: skipped event motion pack cfg=%s chara=%d (Max skeleton)\n",
                     name != NULL ? name : "(null)", chara_no);
        return true;
    }

    int tex_block = EventScene->GetCharaTexb(chara_no);

    if (tex_block < 0) {
        return false;
    }

    sprintf(label, "ev%d", chara_no);
    mgCTextureManager *manager = &mgTexManager;

    if (chara_no >= 8) {
        strcpy(manager->name_suffix, label);
    }

    chara->LoadPack(pack, name, stack, stack, stack, tex_block, 0);

    if (chara_no >= 8) {
        manager->name_suffix[0] = 0;
    }

    return true;
}
