#include "common.h"
#include "scene.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "effscript.hpp"
#include "font.hpp"
#include "mainloop.hpp"
#include "map.hpp"
#include "mg_drawprim.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "savedata.hpp"
#include "scenesnd.hpp"

// Code (.text)
float f_rand(float min_value, float max_value) {
    return min_value + (((max_value - min_value) * (float) rand()) / 2147483648.0f);
}

int i_rand(int min, int max) {
    return (int)f_rand(min, max);
}

void InitVector(float *vec) {
    vec[0] = 0.0f;
    vec[1] = 0.0f;
    vec[2] = 0.0f;
    vec[3] = 1.0f;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", RandXYinViewArea__FfffPfPf);

int CRipple::Birth(float *position) {
    if (active != 0) {
        return 0;
    }
    active = 1;
    sceVu0CopyVector(pos, position);
    pos[1] = 5.0f;
    pos[3] = 1.0f;
    size = f_rand(8.0f, 12.0f);
    count = 0;
    return life = i_rand(20, 40);
}

s32 CRipple::Step(void) {
    if (active == 0) {
        return 0;
    }
    count++;
    if (count >= life) {
        active = 0;
        return -1;
    }
    return 1;
}

void CRipple::Draw(void) {
    float ripple_size = size * count / life;
    float alpha = (float)((life - count) * 40) / life;
    mgCDrawPrim prim;
    float corner[4][4];
    int vertex[4][4];
    RECT rect_a;
    RECT rect_b;
    int tex_no;
    float half;
    int u;
    int v;
    int w;
    int h;

    prim.Initialize(NULL, NULL);
    prim.AlphaBlendEnable(1);
    prim.AlphaBlend(1);
    prim.AlphaTestEnable(1);
    prim.AlphaTest(1, 0);
    prim.DepthTestEnable(0);
    prim.ZMask(-1);
    prim.Bilinear(0);
    prim.TextureMapEnable(1);
    prim.DepthTestEnable(1);
    prim.DepthTest(1);
    prim.Bilinear(1);
    prim.Coord(1);
    prim.AlphaBlend(2);
    prim.AlphaTestEnable(1);
    prim.AntiAliasing(1);
    half = ripple_size / 2.0f;
    corner[0][0] = pos[0] - half;
    corner[0][1] = pos[1];
    corner[0][2] = pos[2] - half;
    corner[0][3] = 1.0f;
    corner[1][0] = pos[0] + half;
    corner[1][1] = pos[1];
    corner[1][2] = pos[2] - half;
    corner[1][3] = 1.0f;
    corner[2][0] = pos[0] - half;
    corner[2][1] = pos[1];
    corner[2][2] = pos[2] + half;
    corner[2][3] = 1.0f;
    corner[3][0] = pos[0] + half;
    corner[3][1] = pos[1];
    corner[3][2] = pos[2] + half;
    corner[3][3] = 1.0f;
    if (mgTransWorldPrim(vertex[0], corner[0]) != 0 &&
        mgTransWorldPrim(vertex[1], corner[1]) != 0 &&
        mgTransWorldPrim(vertex[2], corner[2]) != 0 &&
        mgTransWorldPrim(vertex[3], corner[3]) != 0) {
        prim.Begin(3);
        if (LanguageCode == 0 || LanguageCode == 1) {
            rect_a = GetRectFontTex(GetFontNo("\x81\x9B"), &tex_no);
            u = rect_a.x;
            v = rect_a.y;
            w = rect_a.width;
            h = rect_a.height;
        } else {
            rect_b = GetRectFontTex(GetHalfFontNo('O'), &tex_no);
            u = rect_b.x;
            v = rect_b.y;
            w = rect_b.width;
            h = rect_b.height;
        }
        MySetTex(tex_no, &prim);
        prim.Color(0x80, 0x80, 0x80, (int)alpha);
        prim.TextureCrd(u, v);
        prim.Vertex4(vertex[0]);
        prim.TextureCrd(u, v + h);
        prim.Vertex4(vertex[1]);
        prim.TextureCrd(u + w, v);
        prim.Vertex4(vertex[2]);
        prim.TextureCrd(u, v + h);
        prim.Vertex4(vertex[1]);
        prim.TextureCrd(u + w, v);
        prim.Vertex4(vertex[2]);
        prim.TextureCrd(u + w, v + h);
        prim.Vertex4(vertex[3]);
        prim.End();
    }
}

void CRipple::Init(void) {
    active = 0;
    InitVector(pos);
    size = 0;
    count = 0;
    life = 0;
}

int CParticle::Birth(float *position, float *velocity) {
    if (active != 0) {
        return 0;
    }
    active = 1;
    accel[0] = 0.0f;
    accel[1] = -0.5f;
    accel[2] = 0.0f;
    accel[3] = 1.0f;
    speed[0] = velocity[0];
    speed[1] = velocity[1];
    speed[2] = velocity[2];
    speed[3] = 1.0f;
    pos[0] = position[0];
    pos[1] = position[1];
    pos[2] = position[2];
    pos[3] = 1.0f;
    base_y = position[1];
    return 1;
}

int CParticle::Step(void) {
    if (active == 0) {
        return 0;
    }
    if (pos[1] < base_y) {
        active = 0;
        return -1;
    }
    speed[0] += accel[0];
    speed[1] += accel[1];
    speed[2] += accel[2];
    pos[0] += speed[0];
    pos[1] += speed[1];
    pos[2] += speed[2];
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", Draw__9CParticleFv);

void CParticle::Init(void) {
    active = 0;
    InitVector(pos);
    InitVector(speed);
    InitVector(accel);
    base_y = 0;
}

void CRainDrop::Birth(int drop_type) {
    float view_angle = 0.7853982f;
    int i;

    if (active != 0) {
        return;
    }
    active = 1;
    type = drop_type;
    if (1 == type) {
        float far_dist = 800.0f;
        pos[0][1] = RandXYinViewArea(600.0f, far_dist, view_angle, pos[0], &pos[0][2]);
    } else {
        pos[0][1] = RandXYinViewArea(110.0f, 600.0f, view_angle, pos[0], &pos[0][2]);
    }
    pos[0][3] = 1.0f;
    for (i = 1; i < RAIN_DROP_TRAIL_NUM; i++) {
        sceVu0CopyVector(pos[i], pos[i - 1]);
    }
    speed[0] = f_rand(-2.0f, 2.0f);
    speed[1] = -15.0f;
    speed[2] = f_rand(-2.0f, 2.0f);
    speed[3] = 1.0f;
    if (type == 1) {
        color[0] = 128;
        color[1] = 128;
        color[2] = 128;
        color[3] = 32;
    } else {
        color[0] = 128;
        color[1] = 128;
        color[2] = 128;
        color[3] = 32;
    }
}

int CRainDrop::Step(void) {
    int i;

    if (active == 0) {
        return 0;
    }
    for (i = RAIN_DROP_TRAIL_NUM - 1; i > 0; i--) {
        sceVu0CopyVector(pos[i], pos[i - 1]);
    }
    sceVu0AddVector(pos[0], pos[0], speed);
    pos[0][3] = 1.0f;
    if (pos[0][1] < -100.0f) {
        return -2;
    }
    if (pos[0][1] < 0.0f) {
        return -1;
    }
    return 1;
}

void CRainDrop::Draw(void) {
    mgCDrawPrim prim;
    int vertex_a[4];
    int vertex_b[4];
    int i;

    prim.Initialize(0, 0);
    prim.AlphaBlendEnable(1);
    prim.AlphaBlend(1);
    prim.AlphaTestEnable(1);
    prim.AlphaTest(1, 0);
    prim.DepthTestEnable(0);
    prim.ZMask(-1);
    prim.Bilinear(0);
    prim.TextureMapEnable(0);
    prim.Coord(1);
    prim.Shading(1);
    prim.DepthTestEnable(1);
    prim.DepthTest(1);
    prim.AlphaBlend(2);
    prim.AntiAliasing(1);
    prim.Begin(1);
    for (i = RAIN_DROP_TRAIL_NUM - 1; i > 0; i -= 2) {
        if (mgTransWorldPrim(vertex_a, pos[i]) != 0 &&
            mgTransWorldPrim(vertex_b, pos[i - 1]) != 0) {
            prim.Color(color[0], color[1], color[2], 8);
            prim.Vertex4(vertex_a);
            prim.Color(color[0], color[1], color[2], 0x10);
            prim.Vertex4(vertex_b);
        }
    }
    prim.End();
}

void CRainDrop::Init(void) {
    int i;

    active = 0;
    type = 0;
    for (i = 0; i < RAIN_DROP_TRAIL_NUM; i++) {
        InitVector(pos[i]);
    }
    InitVector(speed);
    color[0] = 0x80;
    color[1] = 0x80;
    color[2] = 0x80;
    color[3] = 0x80;
}

void CRain::SetCharNo(int chara_no) {
    int i;

    this->chara_no = chara_no;
    if (chara_no == -1) {
        for (i = 0; i < RAIN_PARTICLE_NUM; i++) {
            particle[i].Init();
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", ParticleBirth__5CRainFPfi);

void CRain::Stop(void) {
    active = 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", Start__5CRainFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", Step__5CRainFv);

void CRain::Init(void) {
    int i;

    active = 0;
    chara_no = 0;
    for (i = 0; i < RAIN_DROP_NUM; i++) {
        drop[i].Init();
    }
    for (i = 0; i < RAIN_FAR_DROP_NUM; i++) {
        far_drop[i].Init();
    }
    for (i = 0; i < RAIN_PARTICLE_NUM; i++) {
        particle[i].Init();
    }
    for (i = 0; i < RAIN_RIPPLE_NUM; i++) {
        ripple[i].Init();
    }
}

void DrawScreenRain(void) {
    mgCDrawPrim prim;
    int i;

    prim.Initialize(NULL, NULL);
    prim.AlphaBlendEnable(1);
    prim.AlphaBlend(1);
    prim.AlphaTestEnable(1);
    prim.AlphaTest(1, 0);
    prim.DepthTestEnable(0);
    prim.ZMask(-1);
    prim.Bilinear(0);
    prim.TextureMapEnable(0);
    prim.AntiAliasing(1);
    prim.Shading(1);
    prim.Begin(1);
    for (i = 0; i < 50; i++) {
        float length = f_rand(mgScreenHeight / 8, mgScreenHeight / 4);
        float angle = f_rand(-0.0490873866f, 0.0490873866f);
        int x = i_rand(0, mgScreenWidth);
        int y = i_rand(0, mgScreenHeight);
        int end_x = (int)(length * sinf(angle));
        end_x += x;
        int end_y = (int)(length * cosf(angle));
        end_y += y;
        prim.Color(128, 128, 128, 0);
        prim.Vertex(x, y, 0);
        prim.Color(128, 128, 128, 32);
        prim.Vertex(end_x, end_y, 0);
    }
    prim.End();
}

void CRain::Draw(void) {
    int i;
    if (active != 0) {
        for (i = 0; i < RAIN_FAR_DROP_NUM; i++) {
            far_drop[i].Draw();
        }
        for (i = 0; i < RAIN_DROP_NUM; i++) {
            drop[i].Draw();
        }
        for (i = 0; i < RAIN_PARTICLE_NUM; i++) {
            particle[i].Draw();
        }
        for (i = 0; i < 100; i++) {
            ripple[i].Draw();
        }
        DrawScreenRain();
    }
}

void CSceneData::Initialize(void) {
    status = 0;
    name[0] = 0;
    stack = NULL;
    tex_block = -1;
    tex_block_num = 0;
    type = 0;
}

int CSceneCharacter::AssignData(CCharacter2 *character_data, char *character_name) {
    if (character_name == NULL || character_data == NULL) {
        return 0;
    }
    status = 0;
    chara = character_data;
    strcpy(name, character_name);
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

void CSceneCharacter::Initialize(void) {
    chara = NULL;
    texb = -1;
    chara_no = -1;
    CSceneData::Initialize();
}

void CSceneMap::Initialize(void) {
    map = NULL;
    CSceneData::Initialize();
}

int CSceneMap::AssignData(CMap *map_data, char *map_name) {
    if (map_name == NULL || map_data == NULL) {
        return 0;
    }
    Initialize();
    status = 0;
    map = map_data;
    strcpy(name, map_name);
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

void CSceneMessage::Initialize(void) {
    mes = NULL;
    CSceneData::Initialize();
}

int CSceneMessage::AssignData(ClsMes *message_data, char *message_name) {
    if (message_data == NULL) {
        return 0;
    }
    Initialize();
    status = 0;
    mes = message_data;
    if (message_name == NULL) {
        name[0] = 0;
    } else {
        strcpy(name, message_name);
    }
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

int CSceneCamera::AssignData(mgCCamera *camera_data, char *camera_name) {
    if (camera_data == NULL) {
        return 0;
    }
    Initialize();
    status = 0;
    camera = camera_data;
    if (camera_name == NULL) {
        name[0] = 0;
    } else {
        strcpy(name, camera_name);
    }
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

void CSceneCamera::Initialize(void) {
    camera = NULL;
    CSceneData::Initialize();
}

int CSceneSky::AssignData(CMapSky *sky_data, char *sky_name) {
    if (sky_data == NULL) {
        return 0;
    }
    Initialize();
    status = 0;
    sky = sky_data;
    if (sky_name == NULL) {
        name[0] = 0;
    } else {
        strcpy(name, sky_name);
    }
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

void CSceneSky::Initialize(void) {
    sky = NULL;
    CSceneData::Initialize();
}

void CSceneGameObj::Initialize(void) {
    CSceneCharacter::Initialize();
}

void CSceneEffect::Initialize(void) {
    effect = NULL;
    CSceneData::Initialize();
}

int CSceneEffect::AssignData(CEffectScriptMan *effect_data, char *effect_name) {
    if (effect_data == NULL) {
        return 0;
    }
    Initialize();
    status = 0;
    effect = effect_data;
    if (effect_name == NULL) {
        name[0] = 0;
    } else {
        strcpy(name, effect_name);
    }
    status |= SCENE_DATA_ASSIGNED;
    return 1;
}

void CScene::InitAllData() {
    Initialize();
    time = 0.0f;
    day = 0;
    save_data = NULL;
    now_map_no = -1;
    now_sub_map_no = -1;
    old_map_no = -1;
    old_sub_map_no = -1;
    skip_load_villager = 0;
    skip_load_sub_villager = 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", Initialize__6CSceneFv);

void CScene::SetStack(int no, mgCMemory *stack) {
    if (no < 0 || no >= stack_num) {
        return;
    }
    this->stack[no] = stack;
}

mgCMemory *CScene::GetStack(int no) {
    if (no < 0 || no >= stack_num) {
        return NULL;
    }
    return stack[no];
}

#ifdef NONMATCHING
void CScene::ClearStack(int no) {
    int i;

    for (i = no; i < stack_num; i++) {
        mgCMemory *memory = stack[i];
        if (memory != NULL) {
            memory->stack_used = 0;
            memory->lock = 0;
            if (no < i) {
                stack[i]->stSetBuffer(NULL, 0);
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", ClearStack__6CSceneFi);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", AssignStack__6CSceneFi);

CSceneCharacter *CScene::GetSceneCharacter(int no) {
    if (no < 0 || no >= chara_num) {
        return NULL;
    }
    return &chara[no];
}

CSceneMap *CScene::GetSceneMap(int no) {
    if (no < 0 || no >= map_num) {
        return NULL;
    }
    return &map[no];
}

CSceneMessage *CScene::GetSceneMessage(int no) {
    if (no < 0 || no >= message_num) {
        return NULL;
    }
    return &message[no];
}

CSceneCamera *CScene::GetSceneCamera(int no) {
    if (no < 0 || no >= camera_num) {
        return NULL;
    }
    return &camera[no];
}

CSceneSky *CScene::GetSceneSky(int no) {
    if (no < 0 || no >= sky_num) {
        return NULL;
    }
    return &sky[no];
}

CSceneGameObj *CScene::GetSceneGameObj(int no) {
    if (no < 0 || no >= gameobj_num) {
        return NULL;
    }
    return &gameobj[no];
}

CSceneEffect *CScene::GetSceneEffect(int no) {
    if (no < 0 || no >= effect_num) {
        return NULL;
    }
    return &effect[no];
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", CheckIMGName__6CSceneFiPc);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", CheckMDSName__6CSceneFiPc);

CSceneData *CScene::GetData(int kind, int no) {
    switch (kind) {
    case SCENE_DATA_CHARA:
        return GetSceneCharacter(no);
    case SCENE_DATA_MAP:
        return GetSceneMap(no);
    case SCENE_DATA_MESSAGE:
        return GetSceneMessage(no);
    case SCENE_DATA_CAMERA:
        return GetSceneCamera(no);
    case SCENE_DATA_GAMEOBJ:
        return GetSceneGameObj(no);
    case SCENE_DATA_EFFECT:
        return GetSceneGameObj(no);
    default:
        return NULL;
    }
}

int CScene::AssignCamera(int no, mgCCamera *camera, char *name) {
    static char noname[] = "no_name";
    CSceneCamera *slot;
    int i;

    if (no < 0) {
        for (i = 0; i < camera_num; i++) {
            slot = GetSceneCamera(i);
            if (slot == NULL) {
                continue;
            }
            int empty = (slot->status == 0);
            if (empty) {
                break;
            }
        }
        return -1;
    }
    slot = GetSceneCamera(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (active_camera < 0) {
        active_camera = no;
    }
    if (slot->AssignData(camera, name) != 0) {
        return no;
    }
    return -1;
}

int CScene::GetCameraID(char *name) {
    int no;

    if (name == NULL) {
        return -1;
    }
    for (no = 0; no < camera_num; no++) {
        CSceneCamera *slot = GetSceneCamera(no);
        if (slot == NULL) {
            continue;
        }
        int empty = (slot->status == 0);
        if (empty) {
            continue;
        }
        if (strcmp(name, slot->name) == 0) {
            return no;
        }
    }
    return -1;
}

mgCCamera *CScene::GetCamera(int no) {
    CSceneCamera *slot = GetSceneCamera(no);
    if (slot == NULL) {
        return NULL;
    }
    int empty = (slot->status == 0);
    if (empty) {
        return NULL;
    }
    return slot->camera;
}

int CScene::AssignMessage(int no, ClsMes *message, char *name) {
    static char noname[] = "no_name";
    CSceneMessage *slot;
    int i;

    if (no < 0) {
        for (i = 0; i < message_num; i++) {
            slot = GetSceneMessage(i);
            if (slot == NULL) {
                continue;
            }
            int empty = (slot->status == 0);
            if (empty) {
                break;
            }
        }
        return -1;
    }
    slot = GetSceneMessage(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (slot->AssignData(message, name) != 0) {
        return no;
    }
    return -1;
}

ClsMes *CScene::GetMessage(int no) {
    CSceneMessage *slot = GetSceneMessage(no);
    if (slot == NULL) {
        return NULL;
    }
    int empty = (slot->status == 0);
    if (empty) {
        return NULL;
    }
    return slot->mes;
}

int CScene::AssignChara(int no, CCharacter2 *chara, char *name) {
    static char noname[] = "no_name";
    CSceneCharacter *slot;
    int i;

    if (no < 0) {
        for (i = 0; i < chara_num; i++) {
            slot = GetSceneCharacter(i);
            if (slot == NULL) {
                continue;
            }
            int empty = (slot->status == 0);
            if (empty) {
                break;
            }
        }
        return -1;
    }
    slot = GetSceneCharacter(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (slot->AssignData(chara, name) != 0) {
        return no;
    }
    return -1;
}

void CScene::SetCharaNo(int no, int chara_no) {
    CSceneCharacter *chara = GetSceneCharacter(no);
    if (chara != NULL) {
        chara->chara_no = chara_no;
    }
}

int CScene::GetCharaNo(int no) {
    CSceneCharacter *chara;

    chara = GetSceneCharacter(no);
    if (chara != NULL) {
        return chara->chara_no;
    }
    return -1;
}

CCharacter2 *CScene::GetCharacter(int no) {
    CSceneCharacter *slot = GetSceneCharacter(no);
    if (slot == NULL) {
        return NULL;
    }
    int empty = (slot->status == 0);
    if (empty) {
        return NULL;
    }
    return slot->chara;
}

int CScene::AssignMap(int no, CMap *map, char *name) {
    static char noname[] = "no_name";
    CSceneMap *slot;
    int i;

    if (no < 0) {
        for (i = 0; i < map_num; i++) {
            slot = GetSceneMap(i);
            if (slot == NULL) {
                continue;
            }
            int empty = (slot->status == 0);
            if (empty) {
                break;
            }
        }
        return -1;
    }
    slot = GetSceneMap(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (active_map < 0) {
        active_map = no;
    }
    if (slot->AssignData(map, name) != 0) {
        return no;
    }
    return -1;
}

char *CScene::GetMapName(int no) {
    CSceneMap *slot;

    slot = GetSceneMap(no);
    if (slot != NULL) {
        return slot->name;
    }
    return NULL;
}

int CScene::GetMapID(char *name) {
    int no;

    if (name == NULL) {
        return -1;
    }
    for (no = 0; no < map_num; no++) {
        CSceneMap *slot = GetSceneMap(no);
        if (slot == NULL) {
            continue;
        }
        int empty = (slot->status == 0);
        if (empty) {
            continue;
        }
        if (strcmp(name, slot->name) == 0) {
            return no;
        }
    }
    return -1;
}

CMap *CScene::GetMap(int no) {
    CSceneMap *slot = GetSceneMap(no);
    if (slot == NULL) {
        return NULL;
    }
    int empty = (slot->status == 0);
    if (empty) {
        return NULL;
    }
    return slot->map;
}

CMapSky *CScene::GetSky(int no) {
    CSceneSky *slot = GetSceneSky(no);
    if (slot == NULL) {
        return NULL;
    }
    int empty = (slot->status == 0);
    if (empty) {
        return NULL;
    }
    return slot->sky;
}

int CScene::GetMainMapNo() {
    if (active_map == 0) {
        return now_map_no;
    }
    return now_sub_map_no;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/scene", InScreenFunc__6CSceneFP16InScreenFuncInfo);

void CScene::DrawScreenFunc(mgCFrame *frame) {
    int i;
    CMap *map;

    for (i = 0; i < map_num; i++) {
        map = GetMap(i);
        if (IsActive(SCENE_DATA_MAP, i) != 0 && map != NULL) {
            map->DrawScreenFunc(frame);
        }
    }
}

int CScene::AssignSky(int no, CMapSky *sky, char *name) {
    static char noname[] = "no_name";
    CSceneSky *slot;
    int i;

    if (no < 0) {
        for (i = 0; i < sky_num; i++) {
            slot = GetSceneSky(i);
            if (slot == NULL) {
                continue;
            }
            int empty = (slot->status == 0);
            if (empty) {
                break;
            }
        }
        return -1;
    }
    slot = GetSceneSky(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (slot->AssignData(sky, name) != 0) {
        return no;
    }
    return -1;
}

int CScene::DeleteSky(int no) {
    CSceneSky *slot;

    slot = GetSceneSky(no);
    if (slot == NULL) {
        return 0;
    }
    slot->Initialize();
    return 1;
}

int CScene::AssignEffect(int no, CEffectScriptMan *effect, char *name) {
    static char noname[] = "no_name";
    CSceneEffect *slot = GetSceneEffect(no);
    if (slot == NULL) {
        return -1;
    }
    if (name == NULL) {
        name = noname;
    }
    if (slot->AssignData(effect, name) != 0) {
        return no;
    }
    return -1;
}

void CScene::DeleteEffect(int no) {
    CSceneEffect *slot;

    slot = GetSceneEffect(no);
    if (slot != NULL) {
        slot->Initialize();
    }
}

CEffectScriptMan *CScene::GetEffect(int no) {
    CSceneEffect *slot = GetSceneEffect(no);
    if (slot != NULL) {
        return slot->effect;
    }
    return NULL;
}

void CScene::StepEffectScript(int no) {
    if (no < 0) {
        for (int i = 0; i < effect_num; i++) {
            CEffectScriptMan *effect = GetEffect(i);
            if (effect != NULL) {
                effect->Step();
            }
        }
    } else {
        CEffectScriptMan *effect = GetEffect(no);
        if (effect != NULL) {
            effect->Step();
        }
    }
}

void CScene::DrawEffectScript(int no) {
    if (no < 0) {
        for (int i = 0; i < effect_num; i++) {
            CEffectScriptMan *effect = GetEffect(i);
            if (effect != NULL) {
                effect->Draw();
            }
        }
    } else {
        CEffectScriptMan *effect = GetEffect(no);
        if (effect != NULL) {
            effect->Draw();
        }
    }
}

int CScene::IsActive(int kind, int no) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        return (data->status & (SCENE_DATA_ACTIVE | SCENE_DATA_ASSIGNED)) ==
               (SCENE_DATA_ACTIVE | SCENE_DATA_ASSIGNED);
    }
    return 0;
}

void CScene::SetActive(int kind, int no) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        data->status |= SCENE_DATA_ACTIVE;
    }
}

void CScene::ResetActive(int kind, int no) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        data->status &= ~SCENE_DATA_ACTIVE;
    }
}

void CScene::SetStatus(int kind, int no, int flag) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        data->status |= flag;
    }
}

void CScene::ResetStatus(int kind, int no, int flag) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        data->status &= ~flag;
    }
}

int CScene::GetStatus(int kind, int no) {
    CSceneData *data;

    data = GetData(kind, no);
    if (data != NULL) {
        return data->status;
    }
    return 0;
}

void CScene::SetType(int kind, int no, int type) {
    CSceneData *data = GetData(kind, no);
    if (data != NULL) {
        data->type = type;
    }
}

int CScene::GetType(int kind, int no) {
    CSceneData *data = GetData(kind, no);
    if (data != NULL) {
        return data->type;
    }
    return 0;
}

int CScene::GetActiveMap(CMap **maps, int max) {
    int i;
    int count = 0;
    for (i = 0; i < map_num; i++) {
        if (count >= max) {
            break;
        }
        if (IsActive(SCENE_DATA_MAP, i) != 0) {
            maps[count++] = GetMap(i);
        }
    }
    return count;
}

int CScene::GetCharaTexb(int no) {
    int texb;
    CSceneCharacter *chara;

    chara = GetSceneCharacter(no);
    if (chara == NULL) {
        return -1;
    }
    texb = chara->texb;
    if (texb >= 0) {
        return texb;
    }
    if (no < 8) {
        return chara_texb;
    }
    if (no - 8 >= villager_texb_num) {
        return -1;
    }
    return villager_texb + no - 8;
}

void CScene::SetCharaTexb(int no, int texb) {
    CSceneCharacter *chara = GetSceneCharacter(no);
    if (chara != NULL) {
        chara->texb = texb;
    }
}

void CScene::SetTime(float hours) {
    hours -= 24.0f * (int)(hours / 24.0f);
    if (hours < 0.0f) {
        hours += 24.0f;
    }
    time = hours;
    if (save_data != NULL) {
        save_data->now_time = time;
    }
}

void CScene::AddTime(float hours) {
    SetTime(hours + time);
}

void CScene::TimeStep(float frame_scale) {
    float previous_time;
    float step;

    if (time_step != 0) {
        previous_time = time;
        step = time_speed * frame_scale;
        AddTime(step);
        if (previous_time > 24.0f - step - 0.1f && time < 0.1f + step) {
            day++;
        }
        if (save_data != NULL) {
            save_data->day = day;
            save_data->CheckTourBoot(day);
        }
    }
}

void CScene::SetWind(float power, float *dir) {
    wind_power = power;
    sceVu0Normalize(wind_dir, dir);
}

void CScene::ResetWind() {
    wind_power = 0.0f;
}

float CScene::GetWind(float *dir) {
    *(u_long128 *)dir = *(u_long128 *)wind_dir;
    return wind_power;
}

void CScene::SetNowMapNo(int map_no) {
    int old = now_map_no;
    if (old != map_no) {
        old_map_no = old;
    }
    now_map_no = map_no;
}

void CScene::SetNowSubMapNo(int map_no) {
    int old = now_sub_map_no;
    if (old != map_no) {
        old_sub_map_no = old;
    }
    now_sub_map_no = map_no;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", at_1503__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", at_1504__3__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", at_853__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", at_1117__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", at_1171__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", __vt__6CScene__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1188__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1242__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1294__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1381__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1692__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/scene", noname_1709__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(init_1519, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(sun_func_1518, 0x1C0);
