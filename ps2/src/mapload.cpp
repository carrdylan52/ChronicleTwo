#include "common.h"
#include "mapload.hpp"

#include <cmath>
#include <cstring>
#include <libvu0.h>

#include "collision.hpp"
#include "map.hpp"
#include "mapinfo.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_visual.hpp"
#include "object.hpp"
#include "scriptinterpreter.hpp"
#include "water.hpp"

// The inline members of the header are called here, never expanded.
#pragma dont_inline on

// Check the divisor when wrapping a lighting-set index.
#pragma divbyzerocheck on

/** Non-zero when loading an additional map into the current map. */
static int mapAddMode;

/**
 *
 * Map the map script is loading into.
 *
 */
static CMap *mapMap;

/**
 *
 * Node of the map part the map script is building, or null outside a part.
 *
 */
static CList<CMapParts> *mapNowMapParts;

/**
 *
 * Node of the map piece the map script is building, or null outside a piece.
 *
 */
static CList<CMapPiece> *mapNowMapPiece;

/**
 *
 * Memory that everything the map script builds is taken from.
 *
 */
static mgCMemory *mapStack;

/**
 *
 * Far distance of the part being placed.
 *
 */
static float mapFarDist;

/**
 *
 * Fade flag of the part being placed.
 *
 */
static int mapFarAlpha;

/**
 *
 * Visibility of the part being placed.
 *
 */
static int mapShow;

/**
 *
 * Level of detail that the map script is giving pieces to.
 *
 */
static int mapLOD_ID;

/**
 *
 * Current fixed camera entry.
 *
 */
static int mapCameraInfoIdx;

/**
 *
 * Next collision shape of the current camera.
 *
 */
static int mapCameraRectIdx;

/**
 *
 * Index of the function point being built.
 *
 */
static int mapFuncPointIdx;

/**
 *
 * Function point currently being configured.
 *
 */
static CFuncPoint *mapNowFuncPoint;

/**
 *
 * Next entry of the current piece's materials that the map script fills in.
 *
 */
static int mapMatIdx;

/**
 *
 * Non-zero while function points of the map script go to the current map part rather than the map.
 *
 */
static int mapPtsFunc;

/**
 *
 * Non-zero after reserving configuration function points.
 *
 */
static int ReserveFuncFlag;

/**
 *
 * Next water surface slot filled by the configuration script.
 *
 */
static int WaterIndex;

/**
 *
 * Water surface currently being configured.
 *
 */
static CWaterFrame *cfgWater;

static int cfgDRAW_OFF_RECT(SPI_STACK *stack, int argument_count);
static int cfgFUNC_DATA(SPI_STACK *stack, int argument_count);
static int cfgFUNC_DATA_END(SPI_STACK *stack, int argument_count);
static int cfgFUNC_EVENT_DATA(SPI_STACK *stack, int argument_count);
static int cfgOCCLUSION_PLANE(SPI_STACK *stack, int argument_count);
static int cfgWATER_DRAW(SPI_STACK *stack, int argument_count);
static int cfgWATER_DRAW_NUM(SPI_STACK *stack, int argument_count);
static int cfgWATER_PARAM(SPI_STACK *stack, int argument_count);
static int cfgWATER_POS(SPI_STACK *stack, int argument_count);
static int cfgWATER_SHAKE(SPI_STACK *stack, int argument_count);
static int cfgWATER_SURFACE_END(SPI_STACK *stack, int argument_count);
static int cfgWATER_SURFACE_NUM(SPI_STACK *stack, int argument_count);
static int cfgWATER_SURFACE_START(SPI_STACK *stack, int argument_count);
static int cfgWATER_VERTEX(SPI_STACK *stack, int argument_count);
static int mapCAMERA_INFO(SPI_STACK *stack, int argument_count);
static int mapCAMERA_INFO_END(SPI_STACK *stack, int argument_count);
static int mapDummy(SPI_STACK *stack, int argument_count);
static int mapFAR_CLIP(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA_END(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA_OFF_GROUP(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA_POS(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA_POS2(SPI_STACK *stack, int argument_count);
static int mapFIX_CAMERA_RECT(SPI_STACK *stack, int argument_count);
static int mapFUNC_ANIME_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_DATA_END(SPI_STACK *stack, int argument_count);
static int mapFUNC_EFFECT_NAME(SPI_STACK *stack, int argument_count);
static int mapFUNC_EVENT_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_FIRE_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_FLAG(SPI_STACK *stack, int argument_count);
static int mapFUNC_INVENT_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_NAME(SPI_STACK *stack, int argument_count);
static int mapFUNC_PLIGHT_DATA(SPI_STACK *stack, int argument_count);
static int mapFUNC_POINT(SPI_STACK *stack, int argument_count);
static int mapFUNC_POINT_END(SPI_STACK *stack, int argument_count);
static int mapFUNC_POS(SPI_STACK *stack, int argument_count);
static int mapFUNC_SOUND_DATA(SPI_STACK *stack, int argument_count);
static int mapLIGHT_FLAG(SPI_STACK *stack, int argument_count);
static int mapLOD_BLEND(SPI_STACK *stack, int argument_count);
static int mapLOD_END(SPI_STACK *stack, int argument_count);
static int mapLOD_PIECE(SPI_STACK *stack, int argument_count);
static int mapLOD_START(SPI_STACK *stack, int argument_count);
static int mapMAP_FAR_CLIP(SPI_STACK *stack, int argument_count);
static int mapMAP_PARTS(SPI_STACK *stack, int argument_count);
static int mapMAP_PARTS_END(SPI_STACK *stack, int argument_count);
static int mapMOVE_FLAG(SPI_STACK *stack, int argument_count);
static int mapPARTS(SPI_STACK *stack, int argument_count);
static int mapPARTS_END(SPI_STACK *stack, int argument_count);
static int mapPARTS_GROUP(SPI_STACK *stack, int argument_count);
static int mapPARTS_NAME(SPI_STACK *stack, int argument_count);
static int mapPARTS_POS(SPI_STACK *stack, int argument_count);
static int mapPARTS_ROT(SPI_STACK *stack, int argument_count);
static int mapPARTS_SCALE(SPI_STACK *stack, int argument_count);
static int mapPIECE(SPI_STACK *stack, int argument_count);
static int mapPIECE_COL_TYPE(SPI_STACK *stack, int argument_count);
static int mapPIECE_END(SPI_STACK *stack, int argument_count);
static int mapPIECE_MATERIAL(SPI_STACK *stack, int argument_count);
static int mapPIECE_MATERIAL_END(SPI_STACK *stack, int argument_count);
static int mapPIECE_MATERIAL_START(SPI_STACK *stack, int argument_count);
static int mapPIECE_NAME(SPI_STACK *stack, int argument_count);
static int mapPIECE_POS(SPI_STACK *stack, int argument_count);
static int mapPIECE_ROT(SPI_STACK *stack, int argument_count);
static int mapPIECE_SCALE(SPI_STACK *stack, int argument_count);
static int mapPIECE_TIME(SPI_STACK *stack, int argument_count);
static int map_MAP_INFO_TOP(SPI_STACK *stack, int argument_count);

/**
 * Tags recognized by the map script loader.
 */
static SPI_TAG_PARAM map_tag[] = {
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"d", mapDummy},
    {"PARTS", mapPARTS},
    {"FAR_CLIP", mapFAR_CLIP},
    {"LIGHT_FLAG", mapLIGHT_FLAG},
    {"MOVE_FLAG", mapMOVE_FLAG},
    {"LOD_START", mapLOD_START},
    {"LOD_BLEND", mapLOD_BLEND},
    {"LOD_PIECE", mapLOD_PIECE},
    {"LOD_END", mapLOD_END},
    {"PIECE", mapPIECE},
    {"PIECE_POS", mapPIECE_POS},
    {"PIECE_NAME", mapPIECE_NAME},
    {"PIECE_ROT", mapPIECE_ROT},
    {"PIECE_SCALE", mapPIECE_SCALE},
    {"PIECE_MATERIAL_START", mapPIECE_MATERIAL_START},
    {"PIECE_MATERIAL", mapPIECE_MATERIAL},
    {"PIECE_MATERIAL_END", mapPIECE_MATERIAL_END},
    {"PIECE_COL_TYPE", mapPIECE_COL_TYPE},
    {"PIECE_TIME", mapPIECE_TIME},
    {"PIECE_END", mapPIECE_END},
    {"PARTS_END", mapPARTS_END},
    {"MAP_PARTS", mapMAP_PARTS},
    {"MAP_FAR_CLIP", mapMAP_FAR_CLIP},
    {"PARTS_NAME", mapPARTS_NAME},
    {"PARTS_GROUP", mapPARTS_GROUP},
    {"PARTS_POS", mapPARTS_POS},
    {"PARTS_ROT", mapPARTS_ROT},
    {"PARTS_SCALE", mapPARTS_SCALE},
    {"MAP_PARTS_END", mapMAP_PARTS_END},
    {"_MAP_INFO_TOP", map_MAP_INFO_TOP},
    {"CAMERA_INFO", mapCAMERA_INFO},
    {"FIX_CAMERA", mapFIX_CAMERA},
    {"FIX_CAMERA_POS", mapFIX_CAMERA_POS},
    {"FIX_CAMERA_POS2", mapFIX_CAMERA_POS2},
    {"FIX_CAMERA_OFF_GROUP", mapFIX_CAMERA_OFF_GROUP},
    {"FIX_CAMERA_RECT", mapFIX_CAMERA_RECT},
    {"FIX_CAMERA_END", mapFIX_CAMERA_END},
    {"CAMERA_INFO_END", mapCAMERA_INFO_END},
    {"FUNC_POINT", mapFUNC_POINT},
    {"FUNC_DATA", mapFUNC_DATA},
    {"FUNC_NAME", mapFUNC_NAME},
    {"FUNC_FLAG", mapFUNC_FLAG},
    {"FUNC_FIRE_DATA", mapFUNC_FIRE_DATA},
    {"FUNC_EFFECT_NAME", mapFUNC_EFFECT_NAME},
    {"FUNC_FIRE_DATA", mapFUNC_EFFECT_NAME},
    {"FUNC_PLIGHT_DATA", mapFUNC_PLIGHT_DATA},
    {"FUNC_ANIME_DATA", mapFUNC_ANIME_DATA},
    {"FUNC_INVENT_DATA", mapFUNC_INVENT_DATA},
    {"FUNC_EVENT_DATA", mapFUNC_EVENT_DATA},
    {"FUNC_POS", mapFUNC_POS},
    {"FUNC_DATA_END", mapFUNC_DATA_END},
    {"FUNC_POINT_END", mapFUNC_POINT_END},
    {"FUNC_SOUND_DATA", mapFUNC_SOUND_DATA},
    {NULL, NULL},
};

/**
 * Tags recognized by the map configuration script loader.
 */
static SPI_TAG_PARAM cfg_tag[] = {
    {"DRAW_OFF_RECT", cfgDRAW_OFF_RECT},
    {"OCCLUSION_PLANE", cfgOCCLUSION_PLANE},
    {"FUNC_DATA", cfgFUNC_DATA},
    {"FUNC_NAME", mapFUNC_NAME},
    {"FUNC_EVENT_DATA", cfgFUNC_EVENT_DATA},
    {"FUNC_POS", mapFUNC_POS},
    {"FUNC_DATA", cfgFUNC_DATA_END},
    {"WATER_SURFACE_NUM", cfgWATER_SURFACE_NUM},
    {"WATER_SURFACE_START", cfgWATER_SURFACE_START},
    {"WATER_VERTEX", cfgWATER_VERTEX},
    {"WATER_POS", cfgWATER_POS},
    {"WATER_PARAM", cfgWATER_PARAM},
    {"WATER_SHAKE", cfgWATER_SHAKE},
    {"WATER_SURFACE_END", cfgWATER_SURFACE_END},
    {"WATER_DRAW_NUM", cfgWATER_DRAW_NUM},
    {"WATER_DRAW", cfgWATER_DRAW},
    {NULL, NULL},
};
STATIC_ASSERT(sizeof(cfg_tag) == 0x88);

/**
 *
 * Name of the placement being built.
 *
 */
static char mapPlacePartsName[0x100];

/**
 *
 * Name of the part to place.
 *
 */
static char mapMapPartsName[0x100];

/**
 *
 * Name of the part group that the map part being placed by the map script joins, or empty for none.
 *
 */
static char mapMapPartsGroupName[0x100];

/**
 *
 * Position of the map part being placed by the map script.
 *
 */
static sceVu0FVECTOR mapPos;

/**
 *
 * Rotation of the map part being placed by the map script.
 *
 */
static sceVu0FVECTOR mapRot;

/**
 *
 * Scale of the map part being placed by the map script.
 *
 */
static sceVu0FVECTOR mapScale;

// Code (.text)
MAP_TIME_BAND GetTimeBand(float time) {
    MAP_TIME_BAND band = MAP_TIME_BAND_NIGHT;
    if (time >= 6.0f && time < 9.0f) {
        band = MAP_TIME_BAND_MORNING;
    }
    if (time >= 9.0f && time < 17.0f) {
        band = MAP_TIME_BAND_DAY;
    }
    if (time >= 17.0f && time < 21.0f) {
        band = MAP_TIME_BAND_EVENING;
    }
    return band;
}

float CMap::GetNowTime() {
    if (time_enable) {
        return now_time;
    }
    if (fixed_time_enable) {
        return fixed_time;
    }
    return 12.0f;
}

int CMap::GetNowTimeBand() {
    return GetTimeBand(GetNowTime());
}

int CMap::GetNowTimeLightBand() {
    int num = time_light_num;
    if (num < 2) {
        return 0;
    }
    if (num == MAP_TIME_BAND_NUM) {
        return GetNowTimeBand();
    }

    // The light sets divide the day evenly, the first starting at 9:00.
    float time = GetNowTime();
    time -= 9.0f;
    if (time < 0.0f) {
        time += 24.0f;
    }
    return (int)(time / (24.0f / num)) % num;
}

void CMap::GetLightingRatio(float *out_ratio) {
    float time = GetNowTime();
    out_ratio[MAP_TIME_BAND_DAY] = 0.0f;
    out_ratio[MAP_TIME_BAND_EVENING] = 0.0f;
    out_ratio[MAP_TIME_BAND_NIGHT] = 0.0f;
    out_ratio[MAP_TIME_BAND_MORNING] = 0.0f;

    // In the last hour of each band the light fades into the next band's.
    float blend = 0.0f;
    int band = GetNowTimeBand();
    int next = (band + 1) % MAP_TIME_BAND_NUM;
    switch (band) {
    case MAP_TIME_BAND_MORNING:
        if (time > 8.0f) {
            blend = time - 8.0f;
        }
        break;
    case MAP_TIME_BAND_DAY:
        if (time > 16.0f) {
            blend = time - 16.0f;
        }
        break;
    case MAP_TIME_BAND_EVENING:
        if (time > 20.0f) {
            blend = time - 20.0f;
        }
        break;
    case MAP_TIME_BAND_NIGHT:
        if (time < 6.0f && time > 5.0f) {
            blend = time - 5.0f;
        }
        break;
    }
    out_ratio[band] = 1.0f - blend;
    out_ratio[next] = blend;
}

void CMap::GetLightingFlareRatio(float *out_ratio) {
    GetLightingRatio(out_ratio);
    out_ratio[MAP_TIME_BAND_NIGHT] = 0.0f;
}

void CMap::GetLightingSunRatio(float *out_ratio) {
    float time = GetNowTime();
    GetLightingRatio(out_ratio);
    if (time < 6.0f) {
        if (time > 4.0f) {
            out_ratio[MAP_TIME_BAND_NIGHT] = 0.0f;
            out_ratio[MAP_TIME_BAND_MORNING] = 0.0f;
        } else if (time > 3.0f) {
            out_ratio[MAP_TIME_BAND_NIGHT] = 1.0f - (time - 3.0f);
        }
    }
    if (out_ratio[MAP_TIME_BAND_NIGHT] > 0.0f) {
        out_ratio[MAP_TIME_BAND_MORNING] = 0.0f;
        out_ratio[MAP_TIME_BAND_EVENING] = 0.0f;
    }
}

int CMap::GetTimeLightingRatio(float *out_ratio) {
    int num = time_light_num;
    int i;
    float time;
    float ratio;
    float length;
    float start;
    float remaining;
    int band;
    int next;

    if (num == MAP_TIME_BAND_NUM) {
        GetLightingRatio(out_ratio);
        return num;
    }
    time = GetNowTime();
    for (i = 0; i < num; i++) {
        out_ratio[i] = 0.0f;
    }
    if (num < 2) {
        out_ratio[0] = 1.0f;
        return num;
    }
    // The light set in use fades into the next over the last hour before the next starts.
    ratio = 1.0f;
    band = GetNowTimeLightBand();
    next = (band + 1) % num;
    length = 24.0f / num;
    start = 9.0f + band * length;
    if (start >= 24.0f) {
        start -= 24.0f;
    }
    remaining = start + length - time;
    if (remaining < 1.0f) {
        ratio = remaining;
    }
    out_ratio[band] = ratio;
    out_ratio[next] = 1.0f - ratio;
    return num;
}

void CMap::GetSunPoint(float *out_pos) {
    sceVu0FVECTOR sun = {0.0f, -1900.0f, 700.0f, 1.0f};
    sceVu0FMATRIX matrix;

    mgUnitMatrix(matrix);
    sceVu0RotMatrixZ(matrix, matrix, mgAngleLimit((GetNowTime() * 6.2831855f) / 24.0f));
    sceVu0RotMatrixY(matrix, matrix, sun_angle);
    sceVu0ApplyMatrix(out_pos, matrix, sun);
}

float CMap::GetLightNoTime(int light_no) {
    int num = time_light_num;
    float time;

    if (light_no >= num || GetTimeEnable() == 0) {
        return -1.0f;
    }
    if (num == MAP_TIME_BAND_NUM) {
        switch (light_no) {
        case MAP_TIME_BAND_MORNING:
            return 6.5f;
        case MAP_TIME_BAND_DAY:
            return 9.5f;
        case MAP_TIME_BAND_EVENING:
            return 17.5f;
        case MAP_TIME_BAND_NIGHT:
            return 21.5f;
        default:
            return -1.0f;
        }
    }
    time = (24.0f * light_no) / num;
    time += 9.5f;
    if (time < 0.0f) {
        time += 24.0f;
    }
    return time;
}

int CMap::GetTimeEnable() {
    return time_enable;
}

void CMap::GetLightInfo(CMapLightingInfo *out_info) {
    if (out_info == NULL) {
        return;
    }

    int num = time_light_num;
    if (GetActiveLightNo() >= num || (!GetTimeEnable() && !fixed_time_enable)) {
        CMapLightingInfo *info = GetLightingInfo(GetActiveLightNo());
        if (info != NULL) {
            *out_info = *info;
            return;
        }
    }

    CMapLightingInfo *list[8];
    float ratio[8];
    sceVu0FVECTOR sun;

    int band = GetNowTimeLightBand();
    for (int i = 0; i < num; i++) {
        list[i] = CMapInfo::GetLightingInfo(i);
        if (list[i] == NULL) {
            return;
        }
    }
    *out_info = *list[band];

    if (time_light_blend) {
        GetLightInfo(out_info, ratio, GetTimeLightingRatio(ratio));

        // The first directional light follows the sun, never lower than a fixed height.
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
}

CMapLightingInfo *CMap::GetLightingInfo(int no) {
    return CMapInfo::GetLightingInfo(no);
}

int CMap::GetActiveLightNo() {
    return CMapInfo::GetActiveLightNo();
}

void CMap::GetLightInfo(CMapLightingInfo *out_info, float *ratio, int num) {
    sceVu0FMATRIX light_dir;
    sceVu0FMATRIX light_color;
    sceVu0FVECTOR ambient;
    sceVu0FVECTOR bg_color;
    sceVu0FVECTOR bg_color2;
    sceVu0FVECTOR fog;
    sceVu0FVECTOR fog_color;
    CMapLightingInfo *list[8];
    sceVu0FVECTOR work;
    int fog_num = 0;
    int i;
    int j;
    int lighting_num = time_light_num;

    for (i = 0; i < lighting_num; i++) {
        list[i] = CMapInfo::GetLightingInfo(i);
        if (list[i] == NULL) {
            return;
        }
    }

    mgZeroVector(ambient);
    mgZeroVector(fog);
    mgZeroVector(fog_color);
    mgZeroVector(bg_color);
    mgZeroVector(bg_color2);
    mgZeroMatrix(light_dir);
    mgZeroMatrix(light_color);

    for (i = 0; i < num; i++) {
        if (ratio[i] > 0.0f) {
            CMapLightingInfo *info = list[i];
            sceVu0ScaleVector(work, info->ambient, ratio[i]);
            mgAddVector(ambient, work);
            sceVu0ScaleVector(work, info->bg_color, ratio[i]);
            mgAddVector(bg_color, work);
            sceVu0ScaleVector(work, info->bg_color2, ratio[i]);
            mgAddVector(bg_color2, work);

            // Only the sets that draw fog weigh in its colour and distances.
            if (info->fog_enable) {
                work[0] = info->fog.r;
                work[1] = info->fog.g;
                work[2] = info->fog.b;
                work[3] = info->fog.unk_b;
                sceVu0ScaleVector(work, work, ratio[i]);
                mgAddVector(fog_color, work);
                work[0] = info->fog.near_dist;
                work[1] = info->fog.far_dist;
                work[2] = info->fog.far_value;
                work[3] = info->fog.near_value;
                sceVu0ScaleVector(work, work, ratio[i]);
                mgAddVector(fog, work);
                fog_num++;
            }

            for (j = 0; j < 4; j++) {
                sceVu0ScaleVector(work, list[i]->light_dir[j], ratio[i]);
                mgAddVector(light_dir[j], work);
                sceVu0ScaleVector(work, list[i]->light_color[j], ratio[i]);
                mgAddVector(light_color[j], work);
            }
        }
    }

    // Each row now holds one light's direction, normalised unless the blend cancelled it out.
    sceVu0TransposeMatrix(light_dir, light_dir);
    for (i = 0; i < 4; i++) {
        if (mgDistVector(light_dir[i]) > 0.0f) {
            sceVu0Normalize(light_dir[i], light_dir[i]);
        }
    }

    if (mgAbs(ambient[3] - 128.0f) < 0.01f) {
        ambient[3] = 128.0f;
    }

    *(u_long128 *)out_info->ambient = *(u_long128 *)ambient;
    *(u_long128 *)out_info->bg_color = *(u_long128 *)bg_color;
    *(u_long128 *)out_info->bg_color2 = *(u_long128 *)bg_color2;
    *(u_long128 *)out_info->light_color[0] = *(u_long128 *)light_color[0];
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            out_info->light_dir[j][i] = light_dir[i][j];
        }
        *(u_long128 *)out_info->light_color[i] = *(u_long128 *)light_color[i];
    }
    out_info->fog.r = fog_color[0];
    out_info->fog.g = fog_color[1];
    out_info->fog.b = fog_color[2];
    out_info->fog.unk_b = fog_color[3];
    out_info->fog.near_dist = fog[0];
    out_info->fog.far_dist = fog[1];
    out_info->fog.far_value = fog[2];
    out_info->fog.near_value = fog[3];
    out_info->fog_enable = fog_num > 0;
}

/**
 *
 * Handles a map script tag that does nothing.
 *
 */
static int mapDummy(SPI_STACK *stack, int argument_count) {
    return 1;
}

/**
 *
 * Tells whether the map script being loaded adds to a map already loaded.
 *
 */
static int IsAddMode() {
    return mapAddMode;
}

/**
 *
 * Starts a map part of the name of the first argument, which the tags up to PARTS_END build.
 *
 */
static int mapPARTS(SPI_STACK *stack, int argument_count) {
    mapNowMapParts = new (mapStack->Alloc(algn16_size(sizeof(CList<CMapParts>)) + 2)) CList<CMapParts>;
    CMapParts *parts = mapNowMapParts->pGetData();
    char *name = spiGetStackString(stack);
    parts->SetName(name);
    parts->SetPartsName(name);
    mapLOD_ID = 0;
    mapPtsFunc = 1;
    return 1;
}

/**
 *
 * Gives the current map part its far clip distance and whether it fades out there.
 *
 */
static int mapFAR_CLIP(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    mapNowMapParts->pGetData()->far_dist = spiGetStackFloat(stack++);
    mapNowMapParts->pGetData()->fade = spiGetStackInt(stack);
    return 1;
}

/**
 *
 * Sets whether the current map part is drawn without the scene's lights and without point lights.
 *
 */
static int mapLIGHT_FLAG(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    mapNowMapParts->pGetData()->no_light = spiGetStackInt(stack++);
    mapNowMapParts->pGetData()->no_plight = spiGetStackInt(stack);
    return 1;
}

/**
 *
 * Sets the four move flags of the current map part, one per argument.
 *
 */
static int mapMOVE_FLAG(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    CMapParts *parts = mapNowMapParts->pGetData();
    parts->move_flag = 0;
    if (spiGetStackInt(stack++)) {
        parts->move_flag |= 1;
    }
    if (spiGetStackInt(stack++)) {
        parts->move_flag |= 2;
    }
    if (spiGetStackInt(stack++)) {
        parts->move_flag |= 4;
    }
    if (spiGetStackInt(stack)) {
        parts->move_flag |= 8;
    }
    return 1;
}

/**
 *
 * Gives the current map part four levels of detail at the standard distances.
 *
 */
static int mapLOD_START(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    float *dist = (float *)mapStack->Alloc(1);
    CMapParts *parts = mapNowMapParts->pGetData();
    dist[0] = 600.0f;
    dist[1] = 1000.0f;
    dist[2] = 1400.0f;
    dist[3] = 1800.0f;
    parts->SetLODDist(dist, 4);
    return 1;
}

/**
 *
 * Sets whether the current map part blends between its levels of detail.
 *
 */
static int mapLOD_BLEND(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    mapNowMapParts->pGetData()->SetLODBlend(spiGetStackInt(stack));
    return 1;
}

/**
 *
 * Puts a piece of the current map part into a level of detail, hiding it until that level is reached.
 *
 */
static int mapLOD_PIECE(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    CMapParts *parts = mapNowMapParts->pGetData();
    int level = spiGetStackInt(stack++);
    CMapPiece *piece = parts->SearchPiece(spiGetStackString(stack));
    if (piece != NULL) {
        if (level > 0) {
            piece->show = 0;
            piece->fade_alpha = 0.0f;
        }
        if (parts->GetLODBlend()) {
            piece->fade = 1;
        }
    }
    return 1;
}

/**
 *
 * Ends a level of detail, so that the next pieces go to the following level.
 *
 */
static int mapLOD_END(SPI_STACK *stack, int argument_count) {
    mapLOD_ID++;
    return 1;
}

/**
 *
 * Starts a piece of the current map part that uses the model data of the first argument, shown unless the second argument is zero.
 *
 */
static int mapPIECE(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    char *name = spiGetStackString(stack++);
    if (name == NULL) {
        return 0;
    }
    int show = 1;
    if (argument_count >= 2) {
        show = spiGetStackInt(stack);
    }
    if (mapMap->piece_load_skip & 1) {
        return 1;
    }
    mapNowMapPiece = new (mapStack->Alloc(algn16_size(sizeof(CList<CMapPiece>)) + 2)) CList<CMapPiece>;
    CMapPiece *piece = mapNowMapPiece->pGetData();
    int size = strlen(name) + 1;
    if (size % 16 != 0) {
        size = size / 16 + 1;
    } else {
        size = size / 16;
    }
    char *copy = (char *)mapStack->Alloc(size);
    strcpy(copy, name);
    piece->SetName(copy);
    piece->show = show;
    CMdsInfo *mds = mapMap->SearchMDS(copy);
    if (mds != NULL) {
        piece->AssignMds(mds);
    }
    return 1;
}

/**
 *
 * Renames the model data that the current piece uses.
 *
 */
static int mapPIECE_NAME(SPI_STACK *stack, int argument_count) {
    if (mapNowMapPiece == NULL) {
        return 0;
    }
    CMapPiece *piece = mapNowMapPiece->pGetData();
    char *name = spiGetStackString(stack);
    if (name != NULL) {
        char *copy = (char *)mapStack->Alloc(algn16_size(strlen(name) + 1));
        strcpy(copy, name);
        piece->SetName(copy);
    }
    return 1;
}

/**
 *
 * Moves the current piece to the position of the three arguments.
 *
 */
static int mapPIECE_POS(SPI_STACK *stack, int argument_count) {
    if (mapNowMapPiece == NULL) {
        return 0;
    }
    CMapPiece *piece = mapNowMapPiece->pGetData();
    if (piece == NULL) {
        return 0;
    }
    sceVu0FVECTOR position;
    spiGetStackVector(position, stack);
    piece->SetPosition(position);
    return 1;
}

/**
 *
 * Turns the current piece to the angles of the three arguments.
 *
 */
static int mapPIECE_ROT(SPI_STACK *stack, int argument_count) {
    if (mapNowMapPiece == NULL) {
        return 0;
    }
    CMapPiece *piece = mapNowMapPiece->pGetData();
    if (piece == NULL) {
        return 0;
    }
    sceVu0FVECTOR rotation;
    spiGetStackVector(rotation, stack);
    piece->SetRotation(rotation);
    return 1;
}

/**
 *
 * Scales the current piece by the three arguments.
 *
 */
static int mapPIECE_SCALE(SPI_STACK *stack, int argument_count) {
    if (mapNowMapPiece == NULL) {
        return 0;
    }
    CMapPiece *piece = mapNowMapPiece->pGetData();
    if (piece == NULL) {
        return 0;
    }
    sceVu0FVECTOR scale;
    spiGetStackVector(scale, stack);
    piece->SetScale(scale);
    return 1;
}

/**
 *
 * Gives the current piece as many material colour entries as the first argument, filled in by the PIECE_MATERIAL tags that follow.
 *
 */
static int mapPIECE_MATERIAL_START(SPI_STACK *stack, int argument_count) {
    if (mapNowMapPiece == NULL) {
        return 0;
    }
    mapMatIdx = 0;
    int num = spiGetStackInt(stack);
    if (num <= 0) {
        return 1;
    }
    PieceMaterial *material = new (mapStack->Alloc(algn16_size(num * sizeof(PieceMaterial)) + 2)) PieceMaterial[num];
    if (material != NULL) {
        mapNowMapPiece->pGetData()->SetMaterial(material, num);
    }
    return 1;
}

/**
 * Sets the next material colour override of the current piece.
 */
static int mapPIECE_MATERIAL(SPI_STACK *stack, int argument_count) {
    CMapPiece     *piece;
    PieceMaterial *material;
    mgCFrame      *frame;
    char          *name;

    if (mapNowMapPiece == NULL) {
        return 0;
    }
    piece = mapNowMapPiece->pGetData();
    if (piece == NULL) {
        return 0;
    }
    material = piece->GetMaterial(mapMatIdx++);
    if (material == NULL) {
        return 0;
    }
    frame = piece->GetFrame();
    name = spiGetStackString(stack++);
    if (name == NULL || frame == NULL) {
        return 0;
    }
    material->frame = frame->SearchFrame(name);
    if (material->frame == NULL) {
        return 0;
    }
    material->material_no = spiGetStackInt(stack++);
    material->material = material->frame->GetMaterial(material->material_no);
    material->color[0] = spiGetStackFloat(stack++);
    material->color[1] = spiGetStackFloat(stack++);
    material->color[2] = spiGetStackFloat(stack++);
    material->color[3] = spiGetStackFloat(stack++);
    material->unk_c = spiGetStackInt(stack);
    return 1;
}

mgMaterial *mgCFrame::GetMaterial(int index) {
    mgMaterial *material;

    material = NULL;
    if (visual != NULL) {
        material = visual->GetMaterial(index);
    }
    return material;
}

s32 mapPIECE_MATERIAL_END(SPI_STACK *stack, int argc) {
    return 1;
}
/**
 * Sets the collision kind and optional parameter of the current piece.
 */
static int mapPIECE_COL_TYPE(SPI_STACK *stack, int argument_count) {
    CMapPiece *piece;

    if (mapNowMapPiece == NULL) {
        return 0;
    }
    piece = mapNowMapPiece->pGetData();
    piece->col_type = spiGetStackInt(stack++);
    if (argument_count >= 2) {
        piece->col_param = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the interval of the day during which the current piece appears.
 */
static int mapPIECE_TIME(SPI_STACK *stack, int argument_count) {
    CMapPiece *piece;
    float      start;
    float      end;

    if (mapNowMapPiece == NULL) {
        return 0;
    }
    piece = mapNowMapPiece->pGetData();
    start = spiGetStackFloat(stack++);
    end = spiGetStackFloat(stack);
    piece->SetTimeBand(start, end);
    return 1;
}

/**
 * Appends the completed piece to the current part.
 */
static int mapPIECE_END(SPI_STACK *stack, int argument_count) {
    CMapParts *parts;

    if (mapNowMapParts == NULL || mapNowMapPiece == NULL) {
        return 0;
    }
    parts = mapNowMapParts->pGetData();
    if (parts == NULL) {
        return 0;
    }
    parts->AddPiece(mapNowMapPiece);
    return 1;
}

/**
 * Appends the completed part to the map and builds its bounds.
 */
static int mapPARTS_END(SPI_STACK *stack, int argument_count) {
    if (mapNowMapParts == NULL) {
        return 0;
    }
    mapMap->AddParts(mapNowMapParts);
    mapPtsFunc = 0;
    mapNowMapParts->pGetData()->CreateBoundBox();
    return 1;
}

/**
 * Starts a named placement with default position, scale and visibility.
 */
static int mapMAP_PARTS(SPI_STACK *stack, int argument_count) {
    char *name;

    name = spiGetStackString(stack++);
    mapNowMapParts = NULL;
    if (name == NULL) {
        return 0;
    }
    strcpy(mapPlacePartsName, name);
    mgZeroVector(mapPos);
    mgZeroVector(mapRot);
    mapScale[3] = 0.0f;
    mapScale[2] = 1.0f;
    mapScale[1] = 1.0f;
    mapScale[0] = 1.0f;
    mapFarDist = -1.0f;
    mapShow = 1;
    mapFarAlpha = 0;
    if (argument_count > 1) {
        mapShow = spiGetStackInt(stack);
    }
    mapMapPartsName[0] = 0;
    mapMapPartsGroupName[0] = 0;
    return 1;
}

/**
 * Sets the far distance and fading of the part being placed.
 */
static int mapMAP_FAR_CLIP(SPI_STACK *stack, int argument_count) {
    mapFarDist = spiGetStackFloat(stack++);
    mapFarAlpha = spiGetStackInt(stack);
    return 1;
}

/**
 * Selects the source part for the placement.
 */
static int mapPARTS_NAME(SPI_STACK *stack, int argument_count) {
    char *name;

    name = spiGetStackString(stack);
    if (name == NULL) {
        return 0;
    }
    strcpy(mapMapPartsName, name);
    return 1;
}

/**
 * Selects the group that the placement joins.
 */
static int mapPARTS_GROUP(SPI_STACK *stack, int argument_count) {
    char *name;

    name = spiGetStackString(stack);
    if (name == NULL) {
        return 0;
    }
    strcpy(mapMapPartsGroupName, name);
    return 1;
}

/**
 * Sets the position of the placement.
 */
static int mapPARTS_POS(SPI_STACK *stack, int argument_count) {
    spiGetStackVector(mapPos, stack);
    return 1;
}

/**
 * Sets the angles of the placement.
 */
static int mapPARTS_ROT(SPI_STACK *stack, int argument_count) {
    spiGetStackVector(mapRot, stack);
    return 1;
}

/**
 * Sets the scale of the placement.
 */
static int mapPARTS_SCALE(SPI_STACK *stack, int argument_count) {
    spiGetStackVector(mapScale, stack);
    return 1;
}

/**
 * Places the selected part and applies its name, distance and group.
 */
static int mapMAP_PARTS_END(SPI_STACK *stack, int argument_count) {
    CMapParts *parts;

    parts = mapMap->PlaceParts(mapMapPartsName, mapPos, mapRot, mapScale, mapStack);
    if (parts == NULL) {
        return 0;
    }
    parts->SetName(mapPlacePartsName);
    if (mapFarDist > 0.0f) {
        parts->far_dist = mapFarDist;
        parts->fade = mapFarAlpha;
    }
    parts->show = mapShow;
    if (mapMapPartsGroupName[0] != 0) {
        parts->group_no = mapMap->AddPartsGroup(mapMapPartsGroupName, parts, mapStack);
    }
    return 1;
}

s32 map_MAP_INFO_TOP(SPI_STACK *stack, int argc) {
    return 1;
}
/**
 * Allocates the fixed camera information table for the map.
 */
static int mapCAMERA_INFO(SPI_STACK *stack, int argument_count) {
    int          camera_count;
    CCameraInfo *camera_info;

    if (IsAddMode() != 0) {
        return 1;
    }
    camera_count = spiGetStackInt(stack);
    if (camera_count < 0) {
        return 0;
    }
    camera_info = new (mapStack->Alloc(algn16_size(camera_count * sizeof(CCameraInfo)) + 2)) CCameraInfo[camera_count];
    mapMap->SetCameraInfoTable(camera_info, camera_count);
    mapCameraInfoIdx = 0;
    return 1;
}

/**
 * Begins the collision shapes of the current fixed camera.
 */
static int mapFIX_CAMERA(SPI_STACK *stack, int argument_count) {
    if (IsAddMode() != 0) {
        return 1;
    }
    mapCameraRectIdx = 0;
    return 1;
}

/**
 * Sets the first position of the current fixed camera.
 */
static int mapFIX_CAMERA_POS(SPI_STACK *stack, int argument_count) {
    CCameraInfo *camera_info;

    if (IsAddMode() != 0) {
        return 1;
    }
    camera_info = mapMap->GetCameraInfo(mapCameraInfoIdx);
    if (camera_info == NULL) {
        return 0;
    }
    spiGetStackVector(camera_info->pos[0], stack);
    return 1;
}

/**
 * Sets an indexed position of the current fixed camera.
 */
static int mapFIX_CAMERA_POS2(SPI_STACK *stack, int argument_count) {
    CCameraInfo *camera_info;
    int          position_index;

    if (IsAddMode() != 0) {
        return 1;
    }
    camera_info = mapMap->GetCameraInfo(mapCameraInfoIdx);
    if (camera_info == NULL) {
        return 0;
    }
    position_index = spiGetStackInt(stack++);
    if (position_index < 0 || position_index >= 8) {
        return 0;
    }
    spiGetStackVector(camera_info->pos[position_index], stack);
    camera_info->pos_num = position_index + 1 < camera_info->pos_num ? camera_info->pos_num : position_index + 1;
    return 1;
}

/**
 * Assigns a part group and its draw setting to the current fixed camera.
 */
static int mapFIX_CAMERA_OFF_GROUP(SPI_STACK *stack, int argument_count) {
    CCameraInfo     *camera_info;
    CCameraDrawInfo *draw_info;
    char            *group_name;
    int              draw_index;

    if (IsAddMode() != 0) {
        return 1;
    }
    draw_index = spiGetStackInt(stack++);
    camera_info = mapMap->GetCameraInfo(mapCameraInfoIdx);
    if (camera_info == NULL) {
        return 0;
    }
    draw_info = camera_info->GetDrawInfo(draw_index);
    if (draw_info == NULL) {
        return 0;
    }
    group_name = spiGetStackString(stack++);
    if (group_name == NULL) {
        return 0;
    }
    if (group_name[0] == '\0') {
        return 1;
    }
    draw_info->unk_4 = spiGetStackInt(stack);
    draw_info->group_no = mapMap->SearchPartsGroupNo(group_name);
    return 1;
}

/**
 * Adds a collision shape marking where the current fixed camera applies.
 */
static int mapFIX_CAMERA_RECT(SPI_STACK *stack, int argument_count) {
    CCameraInfo  *camera_info;
    CColFrame    *frame;
    CCollision   *collision;
    char         *shape_name;
    SPI_STACK    *shape_data;
    sceVu0FVECTOR transform;

    if (IsAddMode() != 0) {
        return 1;
    }
    camera_info = mapMap->GetCameraInfo(mapCameraInfoIdx);
    if (camera_info == NULL) {
        return 0;
    }
    shape_name = spiGetStackString(stack++);
    if (shape_name == NULL) {
        return 0;
    }
    frame = new (mapStack->Alloc(algn16_size(sizeof(CColFrame)) + 2)) CColFrame;
    collision = NULL;
    if (strcmp(shape_name, "box") == 0) {
        collision = new (mapStack->Alloc(algn16_size(sizeof(CCollision)) + 2)) CCollision;
        spiGetStackVector(collision->bbox.min, stack);
        collision->bbox.min[3] = 1.0f;
        spiGetStackVector(collision->bbox.max, &stack[3]);
        collision->bbox.max[3] = 1.0f;
        stack += 6;
        if (frame != NULL) {
            if (argument_count >= 8) {
                spiGetStackVector(transform, stack);
                stack += 3;
                frame->SetPosition(transform);
            }
            if (argument_count >= 11) {
                spiGetStackVector(transform, stack);
                stack += 3;
                frame->SetRotation(transform);
            }
            if (argument_count >= 14) {
                spiGetStackVector(transform, stack);
                frame->SetScale(transform);
            }
        }
    }
    if (frame != NULL) {
        frame->SetCollision(collision);
    }
    if (mapCameraRectIdx < camera_info->rect_num) {
        camera_info->rect[mapCameraRectIdx] = frame;
        mapCameraRectIdx++;
    }
    return 1;
}

/**
 * Advances to the next fixed camera information entry.
 */
static int mapFIX_CAMERA_END(SPI_STACK *stack, int argument_count) {
    if (IsAddMode() != 0) {
        return 1;
    }
    mapCameraInfoIdx++;
    return 1;
}

s32 mapCAMERA_INFO_END(SPI_STACK *stack, s32 argument_count) {
    if (IsAddMode() != 0) {
        return 1;
    }
    return 1;
}
/**
 * Begins the function points of the map script.
 */
static int mapFUNC_POINT(SPI_STACK *stack, int argument_count) {
    spiGetStackInt(stack);
    mapFuncPointIdx = 0;
    return 1;
}

/**
 * Adds a function point of the script's named kind to the map or current part.
 */
static int mapFUNC_DATA(SPI_STACK *stack, int argument_count) {
    char *kind;
    int   type;

    mapNowFuncPoint = NULL;
    kind = spiGetStackString(stack++);
    if (kind == NULL) {
        return 0;
    }
    if (strcmp(kind, "effect") == 0) {
        type = FUNC_POINT_EFFECT;
    } else if (strcmp(kind, "fire") == 0) {
        type = FUNC_POINT_FIRE;
    } else if (strcmp(kind, "flare") == 0) {
        type = FUNC_POINT_FLARE;
    } else if (strcmp(kind, "plight") == 0) {
        type = FUNC_POINT_PLIGHT;
    } else if (strcmp(kind, "anime") == 0) {
        type = FUNC_POINT_ANIME;
    } else if (strcmp(kind, "invent") == 0) {
        type = FUNC_POINT_INVENT;
    } else if (strcmp(kind, "event") == 0) {
        type = FUNC_POINT_EVENT;
        mapMap->parts_event = 1;
    } else if (strcmp(kind, "sound") == 0) {
        type = FUNC_POINT_SOUND;
    } else if (strcmp(kind, "pos") == 0) {
        type = FUNC_POINT_POS;
    } else {
        return 0;
    }

    if (mapPtsFunc != 0) {
        if (mapNowMapParts == NULL) {
            return 0;
        }
        mapNowFuncPoint = mapNowMapParts->pGetData()->func_point_mngr.Add(type, mapStack);
    } else {
        mapNowFuncPoint = mapMap->func_point.Add(type, mapStack);
    }
    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    mapNowFuncPoint->type = type;
    mapNowFuncPoint->enable = spiGetStackInt(stack);
    return 1;
}

/**
 * Copies the current function point's search name into map memory.
 */
static int mapFUNC_NAME(SPI_STACK *stack, int argument_count) {
    char *name;
    char *copy;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    name = spiGetStackString(stack);
    if (name == NULL) {
        return 0;
    }
    copy = (char *)mapStack->Alloc(algn16_size(strlen(name) + 1));
    if (copy != NULL) {
        strcpy(copy, name);
    }
    mapNowFuncPoint->name = copy;
    return 1;
}

/**
 * Sets the current function point's flags and hours of operation.
 */
static int mapFUNC_FLAG(SPI_STACK *stack, int argument_count) {
    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    mapNowFuncPoint->unk_c = spiGetStackInt(stack++);
    mapNowFuncPoint->unk_8 = spiGetStackInt(stack++);
    mapNowFuncPoint->start = spiGetStackFloat(stack++);
    mapNowFuncPoint->end = spiGetStackFloat(stack);
    return 1;
}

/**
 * Sets the colour and drawing settings of a fire or flare function point.
 */
static int mapFUNC_FIRE_DATA(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR color;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    spiGetStackVector(color, stack);
    stack += 3;
    if (color[2] + (color[0] + color[1]) == 0.0f) {
        color[0] = 128.0f;
        color[1] = 90.0f;
        color[2] = 38.0f;
    } else {
        sceVu0ScaleVector(color, color, 128.0f);
    }
    color[3] = 128.0f;
    *(u_long128 *)mapNowFuncPoint->fire.color = *(u_long128 *)color;
    mapNowFuncPoint->fire.unk_30 = !spiGetStackInt(stack++);
    if (argument_count >= 5) {
        mapNowFuncPoint->fire.unk_34 = spiGetStackInt(stack++);
    }
    if (argument_count >= 6) {
        mapNowFuncPoint->fire.unk_38 = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the current point light's colour, range and optional flicker settings.
 */
static int mapFUNC_PLIGHT_DATA(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR color;
    float         max_color;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    mapNowFuncPoint->plight.power = spiGetStackFloat(stack++);
    spiGetStackVector(color, stack);
    stack += 3;
    sceVu0ScaleVector(color, color, 0.5f);
    color[3] = 0.0f;
    if (color[0] > color[1]) {
        max_color = color[0] > color[2] ? color[0] : color[2];
    } else {
        max_color = color[1] > color[2] ? color[1] : color[2];
    }
    mapNowFuncPoint->plight.range = 0.25f * (mapNowFuncPoint->plight.power * sqrt(max_color));
    *(u_long128 *)mapNowFuncPoint->plight.color = *(u_long128 *)color;
    mapNowFuncPoint->plight.unk_38 = spiGetStackInt(stack++);
    if (argument_count >= 6) {
        mapNowFuncPoint->plight.unk_3c = spiGetStackInt(stack++);
    }
    if (argument_count >= 7) {
        mapNowFuncPoint->plight.unk_40 = spiGetStackInt(stack++);
    }
    if (argument_count >= 8) {
        mapNowFuncPoint->plight.unk_44 = spiGetStackInt(stack++);
    }
    if (argument_count >= 9) {
        mapNowFuncPoint->plight.unk_48 = spiGetStackInt(stack++);
    }
    if (argument_count >= 10) {
        mapNowFuncPoint->plight.flicker_type = spiGetStackInt(stack++);
    }
    if (argument_count >= 11) {
        mapNowFuncPoint->plight.flicker_depth = spiGetStackFloat(stack++);
    }
    if (argument_count >= 12) {
        mapNowFuncPoint->plight.flicker_period = spiGetStackFloat(stack);
    }
    return 1;
}

/**
 * Sets the strings, parameters and optional flags of an animation point.
 */
static int mapFUNC_ANIME_DATA(SPI_STACK *stack, int argument_count) {
    CFuncPoint::AnimeData *anime;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    anime = &mapNowFuncPoint->anime;
    anime->parts_name = mgCopyString(spiGetStackString(stack++), mapStack);
    anime->piece_name = mgCopyString(spiGetStackString(stack++), mapStack);
    anime->frame_name = mgCopyString(spiGetStackString(stack++), mapStack);
    anime->unk_2c = spiGetStackInt(stack++);
    anime->unk_30 = spiGetStackInt(stack++);
    spiGetStackVector(anime->param, stack);
    spiGetStackVector(anime->unk_50, &stack[3]);
    spiGetStackVector(anime->unk_60, &stack[6]);
    anime->unk_34 = 0;
    stack += 9;
    if (argument_count >= 15) {
        anime->unk_34 = spiGetStackInt(stack++);
    }
    if (argument_count >= 16) {
        anime->unk_36 = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the box and angle covered by an invention function point.
 */
static int mapFUNC_INVENT_DATA(SPI_STACK *stack, int argument_count) {
    CFuncPoint::InventData *invent;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    invent = &mapNowFuncPoint->invent;
    invent->unk_20 = spiGetStackInt(stack++);
    spiGetStackVector(invent->box.min, stack);
    invent->box.min[3] = 1.0f;
    spiGetStackVector(invent->box.max, &stack[3]);
    stack += 6;
    invent->box.max[3] = 1.0f;
    invent->unk_24 = spiGetStackInt(stack++);
    invent->unk_28 = spiGetStackFloat(stack++);
    invent->angle = (3.1415927f * spiGetStackFloat(stack)) / 180.0f;
    return 1;
}

/**
 * Sets the event kind, event parameters and optional interaction flags of a point.
 */
static int mapFUNC_EVENT_DATA(SPI_STACK *stack, int argument_count) {
    char                  *kind;
    char                  *name;
    u32                    flag;
    CFuncPoint::EventData *event;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    kind = spiGetStackString(stack++);
    flag = 0;
    event = &mapNowFuncPoint->event;
    event->event_no = spiGetStackInt(stack++);
    event->point_no = spiGetStackInt(stack++);
    event->unk_2c = spiGetStackInt(stack++);
    event->unk_30 = spiGetStackInt(stack++);
    event->unk_34 = spiGetStackInt(stack++);
    if (kind != NULL) {
        if (strcmp(kind, "door") == 0) {
            flag = FUNC_EVENT_UNK_100 | FUNC_EVENT_DOOR | FUNC_EVENT_ACTION;
        } else if (strcmp(kind, "ed_door") == 0) {
            flag = FUNC_EVENT_UNK_100 | FUNC_EVENT_ED_DOOR | FUNC_EVENT_DOOR | FUNC_EVENT_ACTION;
        } else if (strcmp(kind, "lddr_b") == 0) {
            flag = FUNC_EVENT_LADDER_BOTTOM;
            event->event_no = 1;
        } else if (strcmp(kind, "lddr_t") == 0) {
            flag = FUNC_EVENT_LADDER_TOP;
            event->event_no = 1;
        } else if (strcmp(kind, "close_door") == 0) {
            flag = FUNC_EVENT_CLOSE_DOOR | FUNC_EVENT_DOOR | FUNC_EVENT_ACTION;
        } else if (strcmp(kind, "t_box") == 0) {
            flag = FUNC_EVENT_TREASURE_BOX | FUNC_EVENT_ACTION;
            event->event_no = 1;
        } else if (strcmp(kind, "book") == 0) {
            flag = FUNC_EVENT_BOOK | FUNC_EVENT_ACTION;
        }
    }
    event->flag = flag;
    if (argument_count >= 7) {
        name = spiGetStackString(stack++);
        if (name != NULL) {
            if (strlen(name) >= 16) {
                strncpy(event->unk_38, name, 15);
                event->unk_38[15] = '\0';
            } else {
                strcpy(event->unk_38, name);
            }
        }
    }
    if (argument_count >= 8) {
        if (spiGetStackInt(stack++) != 0) {
            event->flag |= FUNC_EVENT_ACTION;
        }
    }
    if (argument_count >= 9) {
        if (spiGetStackInt(stack++) != 0) {
            event->flag |= FUNC_EVENT_ITEM;
        }
    }
    if (argument_count >= 10) {
        if (spiGetStackInt(stack) != 0) {
            event->flag |= FUNC_EVENT_UNK_100;
        } else {
            event->flag &= ~FUNC_EVENT_UNK_100;
        }
    }
    return 1;
}

/**
 * Sets a sound function point's effect, distance parameters and line endpoints.
 */
static int mapFUNC_SOUND_DATA(SPI_STACK *stack, int argument_count) {
    CFuncPoint::SoundData *sound;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    sound = &mapNowFuncPoint->sound;
    sound->se_no = spiGetStackInt(stack++);
    sound->unk_24 = spiGetStackFloat(stack++);
    sound->unk_28 = spiGetStackFloat(stack++);
    sound->unk_2c = (float)spiGetStackInt(stack++);
    sound->shape = spiGetStackInt(stack++);
    spiGetStackVector(sound->start, stack);
    spiGetStackVector(sound->end, &stack[3]);
    sound->end[3] = 1.0f;
    sound->start[3] = 1.0f;
    return 1;
}

/**
 * Resolves the current effect point's effect and reserves its frame bound.
 */
static int mapFUNC_EFFECT_NAME(SPI_STACK *stack, int argument_count) {
    char *name;
    char *copy;
    int   index;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    name = spiGetStackString(stack);
    if (name == NULL) {
        return 0;
    }
    copy = (char *)mapStack->Alloc(algn16_size(strlen(name) + 1));
    if (copy != NULL) {
        strcpy(copy, name);
    }
    mapNowFuncPoint->effect.name = copy;
    index = mapMap->SaerchEffectIndex(copy);
    if (index >= 0) {
        mapNowFuncPoint->effect.index = index;
        mapNowFuncPoint->frame.SetBound(new (mapStack->Alloc(algn16_size(sizeof(mgCFrame::BoundInfo)) + 2)) mgCFrame::BoundInfo);
    } else {
        mapNowFuncPoint->type = FUNC_POINT_NONE;
    }
    return 1;
}

/**
 * Places the function point and adjusts the size and approach offset of event points.
 */
static int mapFUNC_POS(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR scale;
    sceVu0FVECTOR offset;
    sceVu0FMATRIX matrix;
    CFuncPoint::EventData *event;
    CFuncPoint   *point;
    int           scale_y;
    int           scale_z;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    spiGetStackVector(position, &stack[0]);
    position[3] = 1.0f;
    spiGetStackVector(rotation, &stack[3]);
    rotation[3] = 0.0f;
    spiGetStackVector(scale, &stack[6]);
    scale[3] = 0.0f;
    mapNowFuncPoint->SetPosition(position);
    rotation[0] = mgAngleLimit(rotation[0]);
    rotation[1] = mgAngleLimit(rotation[1]);
    rotation[2] = mgAngleLimit(rotation[2]);
    mapNowFuncPoint->SetRotation(rotation);
    mapNowFuncPoint->SetScale(scale);
    point = mapNowFuncPoint;
    if (point->type == FUNC_POINT_EVENT) {
        event = &point->event;
        if ((event->flag & FUNC_EVENT_LADDER_TOP) != 0 ||
            (event->flag & FUNC_EVENT_LADDER_BOTTOM) != 0) {
            if (scale[0] <= 1.1f && scale[1] <= 1.1f && scale[2] <= 1.1f) {
                scale[2] = 25.0f;
                scale[1] = 25.0f;
                scale[0] = 25.0f;
            }
        } else if (point->event.flag & FUNC_EVENT_TREASURE_BOX) {
            scale[2] = 20.0f;
            scale[1] = 20.0f;
            scale[0] = 20.0f;
        } else {
            scale_y = (int)(0.1f + scale[1]);
            scale_z = (int)(0.1f + scale[2]);
            if ((int)(0.1f + scale[0]) == 1 && scale_y == 1 && scale_z == 1) {
                scale[2] = 15.0f;
                scale[1] = 15.0f;
                scale[0] = 15.0f;
            }
        }
        point->SetScale(scale);
        mgZeroVector(offset);
        if (event->flag & FUNC_EVENT_DOOR) {
            offset[0] = -2.0f;
            offset[2] = -12.0f;
        }
        mgUnitMatrix(matrix);
        sceVu0RotMatrixY(matrix, matrix, rotation[1]);
        sceVu0ApplyMatrix(offset, matrix, offset);
        mgAddVector(position, offset);
        mapNowFuncPoint->SetPosition(position);
    }
    return 1;
}

void CFuncPoint::SetScale(float *scale) {
    *(u_long128 *)this->scale = *(u_long128 *)scale;
    frame.SetScale(scale);
}

void CFuncPoint::SetRotation(float *rotation) {
    *(u_long128 *)this->rotation = *(u_long128 *)rotation;
    frame.SetRotation(rotation);
}

void CFuncPoint::SetPosition(float *position) {
    *(u_long128 *)this->position = *(u_long128 *)position;
    frame.SetPosition(position);
}

/**
 * Ends the current function point and advances the script's point index.
 */
static int mapFUNC_DATA_END(SPI_STACK *stack, int argument_count) {
    mapNowFuncPoint = NULL;
    mapFuncPointIdx++;
    return 1;
}

/**
 * Rebuilds the status flags of the map or current part's function point manager.
 */
static int mapFUNC_POINT_END(SPI_STACK *stack, int argument_count) {
    CFuncPointMngr *manager;

    if (mapPtsFunc != 0) {
        if (mapNowMapParts == NULL) {
            return 0;
        }
        manager = &mapNowMapParts->pGetData()->func_point_mngr;
    } else {
        manager = &mapMap->func_point;
    }
    manager->UpdateStatus();
    return 1;
}

void CMap::LoadMapFile(char *script, int size, mgCMemory *stack, int add_mode) {
    mapStack = stack;
    mapAddMode = add_mode;
    mapMap = this;
    mapNowMapParts = NULL;
    mapNowMapPiece = NULL;
    mapCameraInfoIdx = 0;
    mapCameraRectIdx = 0;
    mapFuncPointIdx = 0;
    mapNowFuncPoint = NULL;
    mapPtsFunc = 0;
    SetPieceLoadSkip(0);

    CScriptInterpreter interpreter;
    interpreter.SetTag(map_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

void CMap::SetPieceLoadSkip(s32 skip) {
    piece_load_skip = skip;
}
/**
 * Adds a draw-off area and the box selecting the parts it hides.
 */
static int cfgDRAW_OFF_RECT(SPI_STACK *stack, int argument_count) {
    mgVu0FBOX area;
    mgVu0FBOX parts_box;

    spiGetStackVector(area.min, &stack[0]);
    spiGetStackVector(area.max, &stack[3]);
    spiGetStackVector(parts_box.min, &stack[6]);
    spiGetStackVector(parts_box.max, &stack[9]);
    mapMap->CreateDrawRect(mapStack, &area, &parts_box, 0);
    return 1;
}

/**
 * Adds an occlusion plane from four corners of the configuration script.
 */
static int cfgOCCLUSION_PLANE(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR corner[4];
    int           index;

    for (index = 0; index < 4; index++) {
        spiGetStackVector(corner[index], stack);
        corner[index][3] = 1.0f;
        stack += 3;
    }
    mapMap->CreateOcclusion(corner);
    return 1;
}

/**
 * Takes an event function point from the map's reserved configuration points.
 */
static int cfgFUNC_DATA(SPI_STACK *stack, int argument_count) {
    char *kind;

    if (ReserveFuncFlag == 0) {
        mapMap->func_point.Reserve(64, mapStack);
        ReserveFuncFlag = 1;
    }
    kind = spiGetStackString(stack);
    if (kind == NULL) {
        return 0;
    }
    if (strcmp(kind, "event") == 0) {
        mapMap->parts_event = 1;
    } else {
        return 0;
    }
    mapNowFuncPoint = mapMap->func_point.AddFromReserve(FUNC_POINT_EVENT);
    return mapNowFuncPoint != NULL;
}

/**
 * Sets the trigger parameters of the current configuration point.
 */
static int cfgFUNC_EVENT_DATA(SPI_STACK *stack, int argument_count) {
    char *kind;

    if (mapNowFuncPoint == NULL) {
        return 0;
    }
    mapNowFuncPoint->event.point_no = spiGetStackInt(stack++);
    if (argument_count > 1) {
        kind = spiGetStackString(stack);
        mapNowFuncPoint->event.event_no = 0;
        if (kind != NULL) {
            if (strcmp(kind, "every") == 0) {
                mapNowFuncPoint->event.flag = FUNC_EVENT_EVERY;
            } else if (strcmp(kind, "action") == 0) {
                mapNowFuncPoint->event.flag = FUNC_EVENT_ACTION;
            } else if (strcmp(kind, "check") == 0) {
                mapNowFuncPoint->event.flag = FUNC_EVENT_ACTION;
                mapNowFuncPoint->event.event_no = 1;
            } else if (strcmp(kind, "item") == 0) {
                mapNowFuncPoint->event.flag = FUNC_EVENT_ITEM;
                mapNowFuncPoint->event.event_no = 1;
            } else if (strcmp(kind, "invent") == 0) {
                mapNowFuncPoint->event.flag = FUNC_EVENT_ACTION;
                mapNowFuncPoint->event.event_no = 2;
            } else {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * Ends the current configuration function point.
 */
static int cfgFUNC_DATA_END(SPI_STACK *stack, int argument_count) {
    mapNowFuncPoint = NULL;
    return 1;
}

/**
 * Allocates and empties the map's table of water surfaces.
 */
static int cfgWATER_SURFACE_NUM(SPI_STACK *stack, int argument_count) {
    int index;

    mapMap->water_surface_num = spiGetStackInt(stack);
    if (mapMap->water_surface_num > 0) {
        mapMap->water_surface = new (mapStack->Alloc(algn16_size(mapMap->water_surface_num * sizeof(CWaterFrame *)) + 2)) CWaterFrame *[mapMap->water_surface_num];
        if (mapMap->water_surface == NULL) {
            mapMap->water_surface_num = 0;
        }
        for (index = 0; index < mapMap->water_surface_num; index++) {
            mapMap->water_surface[index] = NULL;
        }
    }
    return 1;
}

s32 cfgWATER_SURFACE_START(SPI_STACK *stack, int argc) {
    return 1;
}
/**
 * Creates the current water surface from its grid size and two corners.
 */
static int cfgWATER_VERTEX(SPI_STACK *stack, int argument_count) {
    int           rows;
    int           columns;
    sceVu0FVECTOR min;
    sceVu0FVECTOR max;

    rows = spiGetStackInt(stack++);
    columns = spiGetStackInt(stack++);
    spiGetStackVector(min, stack);
    spiGetStackVector(max, &stack[3]);
    max[3] = 1.0f;
    min[3] = 1.0f;
    cfgWater = CreateWaterFrame(rows, columns, min, max, mapStack);
    return 1;
}

/**
 * Moves the current water surface to the configuration position.
 */
static int cfgWATER_POS(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR position;

    if (cfgWater == NULL) {
        return 0;
    }
    spiGetStackVector(position, stack);
    cfgWater->SetPosition(position);
    return 1;
}

/**
 * Sets the current water surface's ripple and microprogram constants.
 */
static int cfgWATER_PARAM(SPI_STACK *stack, int argument_count) {
    float speed;
    float damping;
    float param_48;
    float param_4c;

    speed = spiGetStackFloat(stack++);
    damping = spiGetStackFloat(stack++);
    param_48 = spiGetStackFloat(stack++);
    param_4c = spiGetStackFloat(stack);
    cfgWater->SetParam(speed, damping, param_48, param_4c);
    return 1;
}

s32 cfgWATER_SHAKE(SPI_STACK *stack, int argc) {
    return 1;
}
/**
 * Stores the current water surface in the next map slot.
 */
static int cfgWATER_SURFACE_END(SPI_STACK *stack, int argument_count) {
    if (WaterIndex >= mapMap->water_surface_num) {
        return 0;
    }
    mapMap->water_surface[WaterIndex] = cfgWater;
    cfgWater = NULL;
    WaterIndex++;
    return 1;
}

/**
 * Allocates the map's water draw slots and their placed-parts tables.
 */
static int cfgWATER_DRAW_NUM(SPI_STACK *stack, int argument_count) {
    int index;

    mapMap->water_num = spiGetStackInt(stack);
    if (mapMap->water_num < 0) {
        return 0;
    }
    mapMap->water = new (mapStack->Alloc(algn16_size(mapMap->water_num * sizeof(CMapWater)) + 2)) CMapWater[mapMap->water_num];
    for (index = 0; index < mapMap->water_num; index++) {
        mapMap->water[index].Initialize();
        mapMap->water[index].parts_max = 16;
        mapMap->water[index].parts = new (mapStack->Alloc(algn16_size(mapMap->water[index].parts_max * sizeof(CMapParts *)) + 2)) CMapParts *[mapMap->water[index].parts_max];
        mapMap->water[index].Clear();
    }
    return 1;
}

CMapWater::CMapWater() {
}

/**
 * Attaches a water surface to a draw slot with parts or camera-follow axes.
 */
static int cfgWATER_DRAW(SPI_STACK *stack, int argument_count) {
    sceVu0FVECTOR position;
    CMapWater    *water;
    char         *parts_name;
    int           surface;
    int           index;

    surface = spiGetStackInt(stack++);
    if (surface < 0 || surface >= mapMap->water_surface_num) {
        return 0;
    }
    water = NULL;
    for (index = 0; index < mapMap->water_num; index++) {
        if (mapMap->water[index].frame == NULL) {
            water = &mapMap->water[index];
            break;
        }
    }
    if (water == NULL) {
        return 0;
    }
    water->frame = mapMap->water_surface[surface];
    parts_name = spiGetStackString(stack++);
    if (parts_name != NULL && parts_name[0] != '\0') {
        if (strncmp(parts_name, "#follow", 7) == 0) {
            water->follow[0] = parts_name[7] - '0';
            water->follow[1] = parts_name[8] - '0';
            water->follow[2] = parts_name[9] - '0';
        } else {
            water->parts_name = mgCopyString(parts_name, mapStack);
        }
    }
    if (argument_count >= 5) {
        spiGetStackVector(position, stack);
        water->SetPosition(position);
    }
    return 1;
}

void CMap::LoadCfgFile(char *script, int size, mgCMemory *stack) {
    mapMap = this;
    mapStack = stack;
    ReserveFuncFlag = 0;
    WaterIndex = 0;
    cfgWater = NULL;

    CScriptInterpreter interpreter;
    interpreter.SetTag(cfg_tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_438__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", map_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", cfg_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_611__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_612__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_613__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_614__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_615__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_616__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_617__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_618__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_619__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_620__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_621__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_622__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_623__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_624__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_625__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_626__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_627__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_628__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_629__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_630__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_631__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_632__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_633__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_634__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_635__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_636__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_637__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_638__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_639__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_640__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_641__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_642__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_643__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_644__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_645__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_646__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_647__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_648__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_649__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_650__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_651__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_652__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_653__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_654__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_655__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_656__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_657__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_658__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_659__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_660__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_661__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_662__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1064__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1128__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1129__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1130__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1131__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1132__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1133__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1134__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1135__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1136__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1278__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1279__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1280__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1281__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1282__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1283__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1284__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1370__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1371__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1372__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1373__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1374__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1375__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1376__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1377__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1378__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1379__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1380__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1436__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1437__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1438__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1439__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", at_1544__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", __vt__17CList_9CMapPiece___DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapload", __vt__17CList_9CMapParts___DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(mapMap, 0x4);
INCLUDE_BSS(mapNowMapParts, 0x4);
INCLUDE_BSS(mapNowMapPiece, 0x4);
INCLUDE_BSS(mapStack, 0x4);
INCLUDE_BSS(mapFarDist, 0x4);
INCLUDE_BSS(mapFarAlpha, 0x4);
INCLUDE_BSS(mapShow, 0x4);
INCLUDE_BSS(mapLOD_ID, 0x4);
INCLUDE_BSS(mapCameraInfoIdx, 0x4);
INCLUDE_BSS(mapCameraRectIdx, 0x4);
INCLUDE_BSS(mapFuncPointIdx, 0x4);
INCLUDE_BSS(mapNowFuncPoint, 0x4);
INCLUDE_BSS(mapMatIdx, 0x4);
INCLUDE_BSS(mapPtsFunc, 0x4);
INCLUDE_BSS(mapAddMode, 0x4);
INCLUDE_BSS(ReserveFuncFlag, 0x4);
INCLUDE_BSS(WaterIndex, 0x4);
INCLUDE_BSS(cfgWater, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(mapPlacePartsName, 0x100);
INCLUDE_BSS(mapMapPartsName, 0x100);
INCLUDE_BSS(mapMapPartsGroupName, 0x100);
INCLUDE_BSS(mapPos, 0x10);
INCLUDE_BSS(mapRot, 0x10);
INCLUDE_BSS(mapScale, 0x10);
