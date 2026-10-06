#include "common.h"
#include "sceneevent.hpp"
#include "scenesnd.hpp"
#include "scenevillager.hpp"
#include "collision.hpp"
#include "mapsky.hpp"
#include "mg_camera.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "mg_texture.hpp"
#include "mg_tanime.hpp"
#include "screeneffect.hpp"
#include <cstring>

#include <cstdio>

// Code (.text)
void CScene::UpDateMapInfo() {
    CMap *maps[4];
    CMapLightingInfo *lighting;
    int count = GetActiveMap(maps, 4);
    int index;
    for (index = 0; index < count; ++index) {
        if (maps[index] != NULL) maps[index]->now_time = time;
    }
    CMap *map = GetMap(active_map);
    if (map == NULL) return;
    CMapLightingInfo lighting_info;
    map->GetLightInfo(lighting = &lighting_info);
    mgFogEnable(lighting->fog_enable);
    if (lighting->fog_enable) {
        mgSetFogParam(lighting->fog.near_dist, lighting->fog.far_dist, lighting->fog.r,
                      lighting->fog.g, lighting->fog.b, lighting->fog.far_value, lighting->fog.near_value);
    }
    mgSetRenderInfo(lighting->projection, 3.0f, 50000.0f);
    mgSetLight(lighting->light_dir, lighting->light_color);
    mgSetAmbient(lighting->ambient);
    mgResetPlight();
    if (lighting->plight_enable) {
        mgPlightEnable(1);
        for (int light = 0; light < 4; ++light) mgSetPlight(light, &lighting->point_light[light]);
    }
    mgSetBackGround(lighting->bg_color);
}

int CScene::GetColPoly(CCPoly *polys, mgVu0FBOX &box, int max) {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    int total = 0;
    for (int index = 0; index < count; ++index) {
        int found = maps[index]->GetColPoly(polys, box, max);
        total += found;
        polys += found;
        max -= found;
        if (max < 0) {
            break;
        }
    }
    return total;
}

int CScene::GetCameraPoly(CCPoly *polys, mgVu0FBOX &box, int max) {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    int total = 0;
    for (int index = 0; index < count; ++index) {
        int found = maps[index]->GetCameraPoly(polys, box, max);
        total += found;
        polys += found;
        max -= found;
        if (max < 0) {
            break;
        }
    }
    return total;
}

void CScene::RunEvent(int requested_event_no, CSceneEventData *data) {
    if (event_run) {
        printf("start event running!!\n");
        if (event_no == 100) {
            return;
        }
    }
    event_no = requested_event_no;
    if (data != NULL) {
        event_data = *data;
    }
    event_run = 1;
}

int CScene::GetMapEvent(float *pos, int check_type, CSceneEventData *data) {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    int last_event_no = 0;
    for (int index = 0; index < count; ++index) {
        MapEventInfo event_info;
        CFuncPoint *point = maps[index]->GetEvent(pos, check_type, &event_info);
        if (event_info.event_no != 0) last_event_no = event_info.event_no;
        if (point == NULL) continue;
        if (data != NULL) {
            *(u_long128 *)data->position = *(u_long128 *)point->position;
            *(u_long128 *)data->rotation = *(u_long128 *)point->rotation;
            *(u_long128 *)data->scale = *(u_long128 *)point->scale;
            data->event = point->event;
            data->map_event = event_info;
        }
        return 1;
    }
    map_event_no = last_event_no;
    int object_slot = GetGameObjectEvent(pos, data);
    if (object_slot < 0) return 0;
    map_event_no = 1;
    if (check_type != 1) {
        return 0;
    }
    if (object_slot == SCENE_GAMEOBJ_SLOT_TG) {
        data->event.point_no = 300;
    } else if (object_slot == SCENE_GAMEOBJ_SLOT_SAVEPOINT) {
        data->event.point_no = 400;
    }
    return 1;
}

int CScene::GetFixCameraPos(float *pos, float *camera) {
    sceVu0FVECTOR raised;
    *(u_long128 *)raised = *(u_long128 *)pos;
    raised[1] += 1.0f;
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    for (int index = 0; index < count; ++index) {
        if (maps[index]->GetFixCameraPos(raised, camera)) {
            return 1;
        }
    }
    return 0;
}

void CScene::FixCameraPartsOnOff(float *pos) {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    for (int index = 0; index < count; ++index) {
        maps[index]->FixCameraPartsOnOff(pos);
    }
}

void CScene::EyeViewDrawOnOff(int on) {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    for (int index = 0; index < count; ++index) {
        CPartsGroup *shown = maps[index]->SearchPartsGroup("eyeview_on");
        CPartsGroup *hidden = maps[index]->SearchPartsGroup("eyeview_off");
        if (shown != NULL) shown->off = !on;
        if (hidden != NULL) hidden->off = on;
    }
}

void CScene::GetSunPosition(float *pos) {
    sceVu0FVECTOR camera_pos;
    mgZeroVector(camera_pos);
    mgCCamera *camera = GetCamera(active_camera);
    if (camera != NULL) camera->GetPos(camera_pos);
    CMap *map = GetMap(active_map);
    if (map == NULL) return;
    map->GetSunPoint(pos);
    sceVu0Normalize(pos, pos);
    sceVu0ScaleVector(pos, pos, 5000.0f);
    pos[0] += camera_pos[0];
    pos[1] += map->unk_dc;
    pos[2] += camera_pos[2];
}

void CScene::GetMoonPosition(float *pos) {
    GetSunPosition(pos);
    pos[1] *= -1.0f;
}

void CScene::DrawSky(int no) {
    CMapSky *sky;
    if (no < 0) {
        sky = GetSky(1);
        if (sky == NULL) sky = GetSky(0);
    } else {
        sky = GetSky(no);
    }
    if (sky != NULL) {
        sceVu0FVECTOR camera_pos;
        mgZeroVector(camera_pos);
        mgCCamera *camera = GetCamera(active_camera);
        if (camera != NULL) {
            camera->GetPos(camera_pos);
            camera_pos[3] = camera->GetAngleH();
        }
        CMap *map = GetMap(active_map);
        if (map != NULL && map->sky_info) {
            camera_pos[1] = map->unk_dc;
            sceVu0FVECTOR sun_pos;
            sceVu0FVECTOR moon_pos;
            float ratio[8];
            float sun_ratio[8];
            CMapLightingInfo lighting;
            map->GetLightInfo(&lighting);
            map->GetLightingRatio(ratio);
            map->GetLightingSunRatio(sun_ratio);
            GetSunPosition(sun_pos);
            GetMoonPosition(moon_pos);
            sceVu0FVECTOR color0;
            sceVu0FVECTOR color1;
            *(u_long128 *)color0 = *(u_long128 *)lighting.bg_color;
            sceVu0ScaleVector(color0, color0, 0.0078125f);
            color0[3] = 1.0f;
            *(u_long128 *)color1 = *(u_long128 *)lighting.bg_color2;
            sceVu0ScaleVector(color1, color1, 0.0078125f);
            color1[3] = 1.0f;
            sky->DrawSkyBack(camera_pos, color0, color1);
            sky->DrawSky(camera_pos, sun_pos, moon_pos, map->GetNowTimeBand(), ratio, sun_ratio);
        }
    }
}

void CScene::DrawLensFlare(int texb, char *name0, char *name1) {
    static float col[4][4] = {
        {128.0f, 128.0f, 128.0f, 128.0f},
        {192.0f, 96.0f, 0.0f, 128.0f},
        {0.0f, 0.0f, 0.0f, 0.0f},
        {128.0f, 128.0f, 128.0f, 128.0f},
    };
    CMap *map = GetMap(active_map);
    if (map == NULL || !map->sky_info || !map->lens_flare) return;
    float ratio[4];
    map->GetLightingFlareRatio(ratio);
    if (ratio[0] == 0.0f && ratio[1] == 0.0f && ratio[3] == 0.0f) return;
    sceVu0FVECTOR color = { 0.0f, 0.0f, 0.0f, 78.0f };
    for (int band = 0; band < 4; ++band) {
        color[0] += col[band][0] * ratio[band];
        color[1] += col[band][1] * ratio[band];
        color[2] += col[band][2] * ratio[band];
    }
    sceVu0FVECTOR sun_pos;
    GetSunPosition(sun_pos);
    sun_pos[3] = 1.0f;
    int screen[4];
    if (mgTransWorldScreen(screen, sun_pos)) {
        screen[2] = mgTransZPrim(10000.0f);
        LensFlare(screen, color, texb, name0, name1);
    }
}

void CScene::EffectStep() {
    CMap *maps[4];
    int count = GetActiveMap(maps, 4);
    for (int index = 0; index < count; ++index) {
        maps[index]->EffectStep();
    }
    fire_raster.Step();
}

void CScene::DrawEffect(int mode) {
    CMap *maps[4];
    mgCTextureManager *tex_manager = &mgTexManager;
    int count = GetActiveMap(maps, 4);
    for (int index = 0; index < count; ++index) {
        int block = maps[index]->effect_list.block;
        if (block >= 0) {
            tex_manager->ReloadTexture(block, (sceVif1Packet *)NULL);
            maps[index]->DrawEffect();
        }
    }
    tex_manager->ReloadTexture(mode, (sceVif1Packet *)NULL);
    for (int index = 0; index < count; ++index) {
        if (maps[index] != NULL) {
            maps[index]->fire_raster = &fire_raster;
            maps[index]->DrawFireEffect(mode);
            maps[index]->fire_raster = NULL;
        }
    }
    mgCTexture *texture = tex_manager->GetTexture("fire_work", mode);
    fire_raster.SetTexture(texture);
    mgCTexture frame;
    mgGetFrameBuffer(&frame);
    mgRect<int> frame_rect(0, 0, (mgScreenWidth - 1) * 16, (mgScreenHeight - 1) * 16);
    mgSetPkMoveImage(&frame, frame_rect, texture, 0, 0, 0);
    for (int index = 0; index < count; ++index) {
        if (maps[index] != NULL) {
            maps[index]->fire_raster = &fire_raster;
            maps[index]->DrawFireRaster();
            maps[index]->fire_raster = NULL;
        }
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", col_1003__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", at_1013__4__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", at_858__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", at_958__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", at_959__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sceneevent", at_1093__DATA);
