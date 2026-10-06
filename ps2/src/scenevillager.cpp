#include "common.h"
#include "scenevillager.hpp"

#include <cstring>

#include "character.hpp"
#include "dataread.hpp"
#include "funcpoint.hpp"
#include "map.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "sceneevent.hpp"
#include "scenesnd.hpp"
#include "userdata.hpp"
#include "villagermngr.hpp"
#include "vlgr_info.hpp"

/**
 * Names of the villager motions, indexed by VILLAGER_MOTION.
 */
static char *motion_name[VILLAGER_MOTION_NUM + 1] = {
    "\x97\xA7\x82\xBF",
    "\x95\xE0\x82\xAB",
    "\x91\x96\x82\xE8",
    "\x89\xEF\x98\x62",
    "\x8D\xC0\x82\xE8",
    "\x83J\x83\x81\x83\x89\x93\xFC\x82\xE8",
    "\x83J\x83\x81\x83\x89",
    "\x83J\x83\x81\x83\x89\x96\xDF\x82\xE8",
    "\x93\xC1\x95\xCA",
    NULL,
};

/**
 * Game objects placed on each map, ended by an entry whose map number is below zero.
 */
static GAMEOBJ_INFO GameObjInfo[] = {
    {0, GAMEOBJ_TYPE_TG_RED, 1, 0, {{{-697.8f, -0.0f, -1087.4f}, 0.22f}}},
    {25, GAMEOBJ_TYPE_TG_BLUE, 1, 0, {{{107.5f, 24.7f, 1527.8f}, -2.85f}}},
    {1, GAMEOBJ_TYPE_TG_RED, 1, 0, {{{-741.2f, 1.0f, -709.9f}, -2.59f}}},
    {26, GAMEOBJ_TYPE_TG_BLUE, 1, 0, {{{-620.5f, 151.0f, -899.2f}, 0.5f}}},
    {2, GAMEOBJ_TYPE_TG_RED, 1, 0, {{{-138.5f, 166.0f, 1863.7f}, -3.08f}}},
    {82, GAMEOBJ_TYPE_TG_BLUE, 1, 0, {{{160.5f, 287.0f, 1850.2f}, 0.5f}}},
    {3, GAMEOBJ_TYPE_TG_RED, 1, 0, {{{1770.9f, 1.0f, -417.5f}, -1.7f}}},
    {102, GAMEOBJ_TYPE_TG_BLUE, 1, 0, {{{0.0f, 0.0f, 0.0f}, 0.0f}}},
    {10, GAMEOBJ_TYPE_SAVEPOINT, 4, 0, {{{559.3f, 4.1f, 120.1f}, 1.58f},
                                        {{2094.2f, 0.0f, 2066.3f}, 0.17f},
                                        {{-4241.7f, 353.0f, 2876.7f}, 3.04f},
                                        {{-3796.3f, 359.4f, -1639.8f}, 1.63f}}},
    {17, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{2100.2f, 0.0f, 2248.6f}, 2.89f}}},
    {22, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-112.3f, 104.0f, -160.7f}, -0.69f}}},
    {86, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{40.3f, 24.7f, -70.8f}, -2.25f}}},
    {34, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-240.6f, 35.8f, 177.2f}, -2.78f}}},
    {23, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{33.9f, 36.1f, -102.7f}, 3.07f}}},
    {16, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-769.7f, 0.0f, -78.7f}, 1.05f}}},
    {65, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{2805.7f, 0.0f, 1630.3f}, -1.77f}}},
    {54, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-387.7f, 0.0f, -209.7f}, -2.15f}}},
    {1, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{1961.2f, 0.0f, -189.0f}, 1.46f}}},
    {83, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-1861.1f, 29.2f, -336.5f}, 2.8f}}},
    {2, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{1835.6f, -6.3f, -425.0f}, 2.34f}}},
    {87, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{2394.5f, 0.0f, 1511.4f}, 2.76f}}},
    {3, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-1378.5f, 0.0f, -1369.0f}, -2.39f}}},
    {24, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{85.6f, -0.0f, 213.5f}, 3.04f}}},
    {76, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{94.9f, 38.0f, 2816.2f}, 3.14f}}},
    {81, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{184.3f, 57.9f, 1249.0f}, -1.62f}}},
    {61, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{971.7f, 49.3f, -15.5f}, -0.2f}}},
    {84, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{43.6f, -18.1f, 1007.0f}, -2.66f}}},
    {90, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{1875.3f, -0.0f, -338.6f}, 0.26f}}},
    {88, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-2268.1f, -0.0f, 2502.4f}, -2.91f}}},
    {92, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-88.1f, -30.0f, 1727.0f}, -1.82f}}},
    {187, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{70.1f, 140.0f, -865.0f}, 0.0f}}},
    {103, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{-71.1f, -268.0f, 3166.0f}, 0.0f}}},
    {72, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{96.0f, 0.0f, 3451.0f}, 0.0f}}},
    {4, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{78.0f, 0.0f, 1338.0f}, 3.14f}}},
    {109, GAMEOBJ_TYPE_SAVEPOINT, 1, 0, {{{54.0f, 280.0f, -607.0f}, 0.0f}}},
    {-1, GAMEOBJ_TYPE_NONE},
};

// Code (.text)
/**
 * Gives the memory a character pack needs once loaded: its model files take ten thirds of
 * their size, and the total is rounded up to a whole kilobyte.
 */
static int GetChrFileSize(u_int *pack, int file_size) {
    u32 *files[8];
    int sizes[8];
    int count = GetPackFileExt(pack, "mds", files, 8, sizes, NULL);
    int total = 0;
    int i;
    int size;
    for (i = 0; i < count; i++) {
        total += sizes[i];
    }
    size = (file_size - total) + (total / 3 + total * 3);
    if (size % 1024 != 0) {
        size = (size / 1024) * 1024 + 1024;
    }
    return size;
}

int CScene::CheckDrawChara(int no) {
    int status;

    if (IsActive(SCENE_DATA_CHARA, no) == 0) {
        return 0;
    }
    status = GetStatus(SCENE_DATA_CHARA, no);
    if (status & SCENE_CHARA_HIDE) {
        return 0;
    }
    if (status & SCENE_CHARA_NO_MODEL) {
        return 0;
    }
    return 1;
}

int CScene::CheckDrawCharaShadow(int no) {
    int status;

    if (IsActive(SCENE_DATA_CHARA, no) == 0) {
        return 0;
    }
    status = GetStatus(SCENE_DATA_CHARA, no);
    if (status & SCENE_CHARA_HIDE) {
        return 0;
    }
    if (status & SCENE_CHARA_NO_SHADOW) {
        return 0;
    }
    return 1;
}

int CScene::StepChara(int no) {
    float entry_pos[4];
    CCharacter2 *chara = GetCharacter(no);
    if (chara == NULL) {
        return 0;
    }
    chara->foot_se_bank = se_base_id;
    if (chara->CheckDraw() == 0) {
        return 0;
    }
    if (CheckDrawChara(no) == 0 && CheckDrawCharaShadow(no) == 0) {
        return 0;
    }
    chara->SetWind(wind_power, wind_dir);
    if (chara->GetEntryObjectPos(1, entry_pos) != 0) {
        chara->SetFloor(entry_pos[1]);
    }
    chara->Step();
    return 1;
}

void CScene::GetCharaLighting(float (*lights)[4], float *ambient) {
    CMap *map = GetMap(active_map);
    if (map != NULL) {
        float light_scale = 0.3f;
        float ambient_scale = 1.8f;
        float ambient_floor = 56.0f;
        int i;
        float lowest;
        if (map->chara_light_adjust != 0) {
            light_scale = map->chara_light_adjust_value[0];
            ambient_scale = map->chara_light_adjust_value[1];
            ambient_floor = 128.0f * map->chara_light_adjust_value[2];
        }
        sceVu0FVECTOR limit = {255.0f, 255.0f, 255.0f, 128.0f};
        for (i = 0; i < 4; i++) {
            sceVu0ScaleVectorXYZ(lights[i], lights[i], light_scale);
            mgVectorMin(lights[i], lights[i], limit);
        }
        sceVu0ScaleVectorXYZ(ambient, ambient, ambient_scale);
        lowest = ambient[0] < ambient[1] ? (ambient[0] < ambient[2] ? ambient[0] : ambient[2])
                                         : (ambient[1] < ambient[2] ? ambient[1] : ambient[2]);
        if (lowest < ambient_floor) {
            float lift = ambient_floor - lowest;
            ambient[0] += lift;
            ambient[1] += lift;
            ambient[2] += lift;
        }
        if (ambient[0] > 128.0f) {
            ambient[0] = 128.0f;
        }
        if (ambient[1] > 128.0f) {
            ambient[1] = 128.0f;
        }
        if (ambient[2] > 128.0f) {
            ambient[2] = 128.0f;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", DrawChara__6CSceneFii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", DrawCharaShadow__6CSceneFi);

void CScene::DrawExclamationMark(mgCFrame *frame) {
    float position[4];
    int index;
    if (frame != NULL) {
        for (index = 0; index < SCENE_CHARA_SLOT_NUM; index++) {
            if (CheckDrawChara(index) != 0) {
                CCharacter2 *chara = GetCharacter(index);
                if (chara != NULL && (GetStatus(SCENE_DATA_CHARA, index) & SCENE_CHARA_EXCLAMATION) != 0) {
                    chara->GetPosition(position);
                    if (chara->body_height > 0.0f) {
                        position[1] -= 32.0f;
                        position[1] += 2.0f * chara->body_height;
                    }
                    frame->SetPosition(position);
                    mgDrawDirect(frame);
                }
            }
        }
    }
}

int CScene::SearchCharaTexb(int chara_no) {
    int texb;
    int other;
    int i;
    int j;

    texb = GetCharaTexb(chara_no);
    if (texb < 0) {
        return -1;
    }
    for (i = 0; i < SCENE_VILLAGER_SLOT_NUM + SCENE_SUB_VILLAGER_SLOT_NUM; i++) {
        j = i + SCENE_VILLAGER_SLOT_TOP;
        if (j != chara_no) {
            other = GetCharaTexb(j);
            if (other >= 0 && other == texb) {
                return j;
            }
        }
    }
    return -1;
}

int CScene::PreLoadVillager(int map_no, u_long128 *cache) {
    int chara_ids[32];
    CVillagerPlaceInfo *places[32];
    char model_name[256];
    int count;
    int i;
    count = GetLoadVillagerList(map_no, chara_ids, places);
    InitFileCache(cache, 1);
    for (i = 0; i < count; i++) {
        if (GetVillagerModelName(chara_ids[i], model_name) != 0) {
            LoadFileCacheBG(model_name);
        }
    }
    return count;
}

void CScene::PreLoadVillagerEnd() {
    DeleteFileCache();
}

int CScene::DeleteVillager(int chara_no) {
    villager_mngr.DeleteCharaID(chara_no);
    return 1;
}

void CScene::DeleteSubVillager() {
    mgCTextureManager *tex_manager = &mgTexManager;
    int i;
    for (i = 0; i < SCENE_SUB_VILLAGER_SLOT_NUM; i++) {
        int texb = GetCharaTexb(i + SCENE_SUB_VILLAGER_SLOT_TOP);
        if (texb > 0 && SearchCharaTexb(i + SCENE_SUB_VILLAGER_SLOT_TOP) < 0) {
            tex_manager->DeleteBlock(texb);
        }
        DeleteChara(i + SCENE_SUB_VILLAGER_SLOT_TOP);
        villager_mngr.DeleteCharaID(i + SCENE_SUB_VILLAGER_SLOT_TOP);
    }
    ClearStack(SCENE_STACK_SUB_VILLAGER);
    sub_villager_time = -1;
}

void CScene::DeleteVillager() {
    mgCTextureManager *tex_manager = &mgTexManager;
    int i;
    for (i = 0; i < SCENE_VILLAGER_SLOT_NUM; i++) {
        int texb = GetCharaTexb(i + SCENE_VILLAGER_SLOT_TOP);
        if (texb > 0 && SearchCharaTexb(i + SCENE_VILLAGER_SLOT_TOP) < 0) {
            tex_manager->DeleteBlock(texb);
        }
        DeleteChara(i + SCENE_VILLAGER_SLOT_TOP);
        villager_mngr.DeleteCharaID(i + SCENE_VILLAGER_SLOT_TOP);
    }
    ClearStack(SCENE_STACK_VILLAGER);
    villager_time = -1;
}

int CScene::SearchCharaID(int chara_no) {
    int i = SCENE_VILLAGER_SLOT_TOP;
    do {
        int no = GetCharaNo(i);
        if (no == chara_no) {
            return i;
        }
        i++;
    } while (i < SCENE_TALK_SLOT_END);
    return -1;
}

int CScene::GetNowVillagerTime() {
    int is_night;

    float end_hour = 6.0f;
    float start_hour = 21.0f;

    is_night = 0;
    if (CheckTime(time, start_hour, end_hour) != 0) {
        is_night = 1;
    }
    return is_night;
}

int CScene::GetLoadVillagerList(int map_no, int *chara_no, CVillagerPlaceInfo **place) {
    CSaveData *save = save_data;
    int chapter;
    int night;
    int count;
    int i;
    CUserDataManager *user_data;
    if (save == NULL) {
        return 0;
    }
    if (map_no < 0) {
        return 0;
    }
    chapter = save->game_progress;
    night = GetNowVillagerTime();
    count = villager_mngr.GetAppearVlgr(chapter, night, map_no, chara_no, place);
    user_data = &save_data->user_data;
    if (user_data == NULL) {
        return count;
    }
    for (i = 0; i < count; i++) {
        if (user_data->GetPartyCharaStatus(chara_no[i]) > 0) {
            chara_no[i] += 1000;
        }
    }
    return count;
}

int CScene::SearchCopyModel(int chara_no) {
    CVillagerInfo *info = GetVillagerInfo(chara_no);
    int i;
    if (info == NULL) {
        return -1;
    }
    for (i = 0; i < SCENE_VILLAGER_SLOT_NUM + SCENE_SUB_VILLAGER_SLOT_NUM; i++) {
        int slot = i + SCENE_VILLAGER_SLOT_TOP;
        CSceneCharacter *scene_chara = GetSceneCharacter(slot);
        if (scene_chara != NULL && GetCharacter(slot) != NULL) {
            CVillagerInfo *other = GetVillagerInfo(scene_chara->chara_no);
            if (other != NULL && other->model_name != NULL && info->model_name != NULL &&
                strcmp(other->model_name, info->model_name) == 0) {
                return slot;
            }
        }
    }
    return -1;
}

/**
 * Finds the frames of a character named in a list separated by semicolons, giving the
 * number of names read.
 */
static int GetObjectNameList(char *names, CCharacter2 *chara, mgCFrame **frames, int max) {
    char name[64];
    char *cursor;
    int count;
    mgCFrame *model;
    int index;
    if (names == NULL || max <= 0 || chara == NULL) {
        return 0;
    }
    model = chara->CObjectFrame::frame;
    count = 0;
    if (model == NULL) {
        return 0;
    }
    index = 0;
    while (*names != 0) {
        if (count >= max) {
            return count;
        }
        cursor = name;
        for (;;) {
            if (*names == ';' || *names == 0) {
                break;
            }
            *cursor++ = *names++;
        }
        *cursor = 0;
        frames[index] = model->SearchFrame(name);
        if (*names == 0) {
            count++;
            break;
        }
        names++;
        if (frames[index] != NULL) {
            index++;
            count++;
        }
    }
    return count;
}

#ifdef NONMATCHING
void CScene::CharaObjectOnOff(int no, mgCMemory *memory) {
    mgCFrame *frames[16];
    CVillagerInfo *info;
    int i;
    CCharacter2 *chara;
    int j;
    mgCFrameAttr *attr;
    CSceneCharacter *scene_chara;
    int count;

    scene_chara = GetSceneCharacter(no);
    if (scene_chara == NULL) {
        return;
    }
    chara = scene_chara->chara;
    if (chara == NULL) {
        return;
    }
    info = GetVillagerInfo(scene_chara->chara_no);
    if (info == NULL) {
        return;
    }
    count = GetObjectNameList(info->show_frames, chara, frames, 16);
    for (i = 0; i < count; i++) {
        if (frames[i] != NULL) {
            attr = frames[i]->attr;
            if (attr == NULL && memory != NULL) {
                attr = new (memory->Alloc(11)) mgCFrameAttr;
                frames[i]->attr = attr;
            }
            if (attr != NULL) {
                attr->draw = 1;
            }
        }
    }
    count = GetObjectNameList(info->hide_frames, chara, frames, 16);
    for (j = 0; j < count; j++) {
        if (frames[j] != NULL) {
            attr = frames[j]->attr;
            if (attr == NULL && memory != NULL) {
                attr = new (memory->Alloc(11)) mgCFrameAttr;
                frames[j]->attr = attr;
            }
            if (attr != NULL) {
                attr->draw = 2;
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", CharaObjectOnOff__6CSceneFiP9mgCMemory);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", LoadVillager__6CSceneFii);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", LoadSubVillager__6CSceneFii);

void CScene::RegisterVillager(int no, int chara_no, int place_no) {
    RegisterVillager(no, chara_no, GetVlgrPlaceInfo(place_no));
}

int CScene::RegisterVillager(int no, int chara_no, CVillagerPlaceInfo *place) {
    return villager_mngr.Register(chara_no, no, place);
}

int CScene::RegisterVillager(int no, int chara_no, mgCMemory *memory) {
    float character_position[4];
    CCharacter2 *chara;
    CVillagerPlaceInfo *place_info;
    if ((place_info = (CVillagerPlaceInfo *)operator new(sizeof(CVillagerPlaceInfo),
                                                        memory->Alloc(6))) != NULL) {
        memset(place_info, 0, sizeof(CVillagerPlaceInfo));
        place_info->map_no = -1;
    }
    if (place_info == NULL) {
        return 0;
    }
    chara = GetCharacter(no);
    if (chara == NULL) {
        return 0;
    }
    chara->GetPosition(place_info->pos);
    chara->GetRotation(character_position);
    place_info->pos[3] = character_position[1];
    return RegisterVillager(no, chara_no, place_info);
}

int CScene::GetTalkEvent(float *pos, CSceneEventData *data) {
    float character_position[4];
    float character_rotation[4];
    float talk_rect[4];
    float rotation[4][4];
    CSceneEventData cleared_event;
    float range;
    int slot;
    for (slot = SCENE_VILLAGER_SLOT_TOP; slot < SCENE_TALK_SLOT_END; slot++) {
        range = 30.0f;
        if (IsActive(SCENE_DATA_CHARA, slot) != 0) {
            CCharacter2 *chara = GetCharacter(slot);
            if (chara != NULL) {
                int rect_kind;
                chara->GetPosition(character_position);
                chara->GetRotation(character_rotation);
                rect_kind = villager_mngr.GetTalkRect(slot, talk_rect);
                if (rect_kind >= 0) {
                    if (talk_rect[3] > 0.0f) {
                        range = talk_rect[3];
                    }
                    talk_rect[3] = 1.0f;
                    if (rect_kind != 0) {
                        mgUnitMatrix(rotation);
                        mgCreateMatrixPY(rotation, character_position, character_rotation[1]);
                        sceVu0ApplyMatrix(character_position, rotation, talk_rect);
                    }
                    if (mgDistVector(character_position, pos) < range) {
                        memset(&cleared_event, 0, sizeof(cleared_event));
                        data->chara_slot = slot;
                        data->chara_no = GetCharaNo(slot);
                        data->event.point_no = slot - SCENE_VILLAGER_SLOT_TOP;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * Gives the name of a villager motion; the standing motion for an unknown number.
 */
static char *GetMotionName(int motion_id) {
    switch (motion_id) {
    case VILLAGER_MOTION_WALK:
        return motion_name[1];
    case VILLAGER_MOTION_RUN:
        return motion_name[2];
    case VILLAGER_MOTION_TALK:
        return motion_name[3];
    case VILLAGER_MOTION_SIT:
        return motion_name[4];
    case VILLAGER_MOTION_CAMERA_IN:
        return motion_name[5];
    case VILLAGER_MOTION_CAMERA:
        return motion_name[6];
    case VILLAGER_MOTION_CAMERA_OUT:
        return motion_name[7];
    case VILLAGER_MOTION_SPECIAL:
        return motion_name[8];
    default:
        return motion_name[0];
    }
}

/**
 * Gives the villager motion with a name, or -1.
 */
static int GetMotionID(char *name) {
    int i;
    if (name == NULL) {
        return -1;
    }
    for (i = 0; motion_name[i] != NULL; i++) {
        if (strcmp(name, motion_name[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * Starts a villager motion on a character; standing replaces sitting for a character
 * without a sitting motion.
 */
static void SetCharaMotion(CCharacter2 *chara, int motion_id, int mode) {
    char *name = GetMotionName(motion_id);
    if (name != NULL) {
        if (motion_id == VILLAGER_MOTION_SIT && chara->GetKeyListPtr(name, NULL) == NULL) {
            name = GetMotionName(VILLAGER_MOTION_STAND);
        }
        if (name != NULL) {
            chara->SetMotion(name, mode);
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", StepVillager__6CSceneFv);

void CScene::StayNearVillager(float *pos, int *stay) {
    int count = villager_mngr.data_num;
    int i;
    if (villager_mngr.stop == 0) {
        for (i = 0; i < count; i++) {
            CVillagerData *villager;
            stay[i] = 0;
            villager = villager_mngr.GetData(i);
            if (villager != NULL) {
                u8 unusable = villager->vlgr_id < 0 || !(villager->place != NULL);
                if (unusable == 0 && villager->stay <= 0 &&
                    mgDistVectorXZ(pos, villager->pos) < pos[3]) {
                    CCharacter2 *chara;
                    villager_mngr.Stay(i);
                    chara = GetCharacter(villager->chara_id);
                    if (chara != NULL) {
                        int motion = GetMotionID(chara->GetNowMotionName());
                        if (chara != NULL && villager_mngr.CheckStay(i) != 0) {
                            CVillagerPlaceInfo *place = villager->place;
                            if (motion == place->move_motion) {
                                SetCharaMotion(chara, place->motion, 0);
                            }
                        }
                        stay[i] = 1;
                    }
                }
            }
        }
    }
}

void CScene::CancelStayVillager(int *stay) {
    int i;
    int count = villager_mngr.data_num;

    for (i = 0; i < count; i++) {
        if (stay[i] != 0) {
            villager_mngr.CancelStay(i);
        }
    }
}

void CScene::StayVillager(int chara_no) {
    int index = villager_mngr.SearchDataIDatCharaID(chara_no);
    if (index >= 0) {
        villager_mngr.Stay(index);
    }
}

void CScene::CancelStayVillager(int chara_no) {
    int index = villager_mngr.SearchDataIDatCharaID(chara_no);
    CVillagerData *villager = villager_mngr.GetData(index);
    if (index >= 0) {
        villager_mngr.CancelStay(index);
    }
    if (villager != NULL && villager->stay == 0) {
        CCharacter2 *chara = GetCharacter(villager->chara_id);
        if (chara != NULL) {
            chara->GetPosition(villager->pos);
            chara->GetRotation(villager->rot);
        }
    }
}

void CScene::ExModeVillager(int chara_no) {
    int index = villager_mngr.SearchDataIDatCharaID(chara_no);
    if (index >= 0) {
        villager_mngr.ExMode(index);
    }
}

void CScene::SetActiveVillager() {
    int no_map;
    int i;
    int count;
    no_map = active_map == 0;
    count = villager_mngr.data_num;
    for (i = 0; i < count; i++) {
        CVillagerData *villager = villager_mngr.GetData(i);
        if (villager != NULL) {
            u8 unused = villager->vlgr_id < 0 || !(villager->place != NULL);
            if (unused == 0) {
                if (no_map) {
                    SetActive(SCENE_DATA_CHARA, villager->chara_id);
                } else {
                    int chara_id = villager->chara_id;
                    if (chara_id >= SCENE_SUB_VILLAGER_SLOT_TOP) {
                        SetActive(SCENE_DATA_CHARA, chara_id);
                    } else {
                        ResetActive(SCENE_DATA_CHARA, chara_id);
                    }
                }
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scenevillager", InScreenChara__6CSceneFPQ26CScene17InScreenCharaInfoPf);

void CScene::LoadGameObject(int map_no, int texb, mgCMemory *memory) {
    GAMEOBJ_INFO *entry;
    int skip_objects;
    u32 *file_buffer;
    CSaveData *save;

    file_buffer = (u32 *)read_buff;
    entry = GameObjInfo;
    DeleteChara(SCENE_GAMEOBJ_SLOT_TG);
    DeleteChara(SCENE_GAMEOBJ_SLOT_TG_BASE);
    DeleteChara(SCENE_GAMEOBJ_SLOT_SAVEPOINT);
    DeleteChara(SCENE_GAMEOBJ_SLOT_BOOK);
    mgTexManager.DeleteBlock(texb);
    save = save_data;
    skip_objects = 0;
    if (save != NULL && GetGameChapter(save->game_progress) == 8) {
        skip_objects = 1;
    }
    for (;;) {
        if (entry->map_no < 0) {
            break;
        }
        if (entry->map_no == map_no) {
            switch (entry->type) {
            case GAMEOBJ_TYPE_SAVEPOINT:
                if (LoadFile2("effect/savepoint.chr", file_buffer, NULL, 0) != 0) {
                    LoadChara(SCENE_GAMEOBJ_SLOT_SAVEPOINT, file_buffer, NULL, memory, memory, memory,
                                  texb, 1);
                    SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_SAVEPOINT);
                    if (LoadFile2("effect/book.chr", file_buffer, NULL, 0) != 0) {
                        LoadChara(SCENE_GAMEOBJ_SLOT_BOOK, file_buffer, NULL, memory, memory, memory,
                                      texb, 1);
                        SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_BOOK);
                    }
                }
                break;
            case GAMEOBJ_TYPE_TG_RED:
                if (skip_objects == 0 &&
                        LoadFile2("effect/tg_maru_red.chr", file_buffer, NULL, 0) != 0) {
                    LoadChara(SCENE_GAMEOBJ_SLOT_TG, file_buffer, NULL, memory, memory, memory,
                                  texb, 1);
                    SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG);
                    if (LoadFile2("effect/tg_sita_red.chr", file_buffer, NULL, 0) != 0) {
                        LoadChara(SCENE_GAMEOBJ_SLOT_TG_BASE, file_buffer, NULL, memory, memory, memory,
                                      texb, 1);
                        SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG_BASE);
                    }
                }
                break;
            case GAMEOBJ_TYPE_TG_BLUE:
                if (skip_objects == 0 &&
                        LoadFile2("effect/tg_maru_blue.chr", file_buffer, NULL, 0) != 0) {
                    LoadChara(SCENE_GAMEOBJ_SLOT_TG, file_buffer, NULL, memory, memory, memory,
                                  texb, 1);
                    SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG);
                    if (LoadFile2("effect/tg_sita_blue.chr", file_buffer, NULL, 0) != 0) {
                        LoadChara(SCENE_GAMEOBJ_SLOT_TG_BASE, file_buffer, NULL, memory, memory, memory,
                                      texb, 1);
                        SetActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG_BASE);
                    }
                }
                break;
            }
        }
        entry++;
    }
}

int CScene::GetGameObjectEvent(float *pos, CSceneEventData *data) {
    GAMEOBJ_INFO *entry;
    int now_map_no;
    int i;
    float point[4];
    if (active_map != 0) {
        return -1;
    }
    now_map_no = GetMainMapNo();
    entry = GameObjInfo;
    for (;;) {
        if (entry->map_no < 0) {
            break;
        }
        if (entry->map_no == now_map_no) {
            for (i = 0; i < entry->place_num; i++) {
                *(u_long128 *)point = *(u_long128 *)entry->place[i].pos;
                point[3] = 1.0f;
                if (mgDistVector(point, pos) < 20.0f) {
                    *(u_long128 *)data->position = *(u_long128 *)point;
                    mgZeroVector(data->rotation);
                    switch (entry->type) {
                    case GAMEOBJ_TYPE_SAVEPOINT:
                        if (IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_SAVEPOINT) != 0 &&
                                IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_BOOK) != 0) {
                            data->gameobj_no = i;
                            return SCENE_GAMEOBJ_SLOT_SAVEPOINT;
                        }
                        break;
                    case GAMEOBJ_TYPE_TG_RED:
                    case GAMEOBJ_TYPE_TG_BLUE:
                        if (IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG) != 0 &&
                                IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG_BASE) != 0) {
                            return SCENE_GAMEOBJ_SLOT_TG;
                        }
                        break;
                    }
                }
            }
        }
        entry++;
    }
    return -1;
}

void CScene::DrawGameObject(int map_no) {
    GAMEOBJ_INFO *entry;
    CCharacter2 *first;
    CCharacter2 *second;
    int i;
    float first_point[4];
    float second_point[4];
    if (active_map != 0) {
        return;
    }
    entry = GameObjInfo;
    for (;;) {
        if (entry->map_no < 0) {
            break;
        }
        if (entry->map_no == map_no) {
            first = NULL;
            second = NULL;
            switch (entry->type) {
            case GAMEOBJ_TYPE_SAVEPOINT:
                if (IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_SAVEPOINT) != 0 &&
                    IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_BOOK) != 0) {
                    first = GetCharacter(SCENE_GAMEOBJ_SLOT_SAVEPOINT);
                    second = GetCharacter(SCENE_GAMEOBJ_SLOT_BOOK);
                }
                break;
            case GAMEOBJ_TYPE_TG_RED:
            case GAMEOBJ_TYPE_TG_BLUE:
                if (IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG) != 0 &&
                    IsActive(SCENE_DATA_CHARA, SCENE_GAMEOBJ_SLOT_TG_BASE) != 0) {
                    first = GetCharacter(SCENE_GAMEOBJ_SLOT_TG);
                    second = GetCharacter(SCENE_GAMEOBJ_SLOT_TG_BASE);
                }
                break;
            }
            for (i = 0; i < entry->place_num; i++) {
                *(u_long128 *)first_point = *(u_long128 *)entry->place[i].pos;
                first_point[3] = 1.0f;
                *(u_long128 *)second_point = *(u_long128 *)entry->place[i].pos;
                second_point[3] = 1.0f;
                if (entry->type == GAMEOBJ_TYPE_TG_RED || entry->type == GAMEOBJ_TYPE_TG_BLUE) {
                    first_point[1] += 60.0f;
                }
                if (second != NULL) {
                    second->SetPosition(second_point);
                    second->SetRotation(0.0f, entry->place[i].rot_y, 0.0f);
                    second->DrawDirect();
                }
                if (first != NULL) {
                    first->SetPosition(first_point);
                    first->SetRotation(0.0f, entry->place[i].rot_y, 0.0f);
                    first->DrawDirect();
                }
            }
        }
        entry++;
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_868__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_991__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_992__3__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_815__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1335__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1336__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1337__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1338__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1339__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1464__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1592__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1593__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1594__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1595__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1842__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1843__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1844__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1845__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1846__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scenevillager", at_1847__2__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_988__3, 0x10);
