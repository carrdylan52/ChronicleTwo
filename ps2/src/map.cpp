#include "common.h"
#include "map.hpp"

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "collision.hpp"
#include "dataread.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "mg_camera.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_sprite.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "water.hpp"

#include "mapload.hpp"

// Code (.text)
int CMapFlagData::SetFlag(int no, int on) {
    u32 mask;
    u32 old_flag;
    if (no < 0 || no >= MAP_FLAG_MAX) {
        return 0;
    }
    mask = 1;
    int index = no / 32;
    mask <<= no % 32;
    old_flag = flag[index];
    int was_set = (mask & old_flag) != 0;
    if (on) {
        flag[index] = mask | old_flag;
    } else {
        flag[index] = ~mask & old_flag;
    }
    return was_set;
}

int CMapFlagData::GetFlag(int no) {
    if (no < 0 || no >= MAP_FLAG_MAX) {
        return 0;
    }
    u32 mask = 1;
    mask <<= no % 32;
    return (mask & flag[no / 32]) != 0;
}

char *CMap::Iam() {
    return CMapName;
}

void CPartsGroup::Initialize() {
    name = NULL;
    camera_off = 0;
    off = 0;
    list = NULL;
}

void CPartsGroup::Add(CList<PartsGroupData> *entry) {
    CList<PartsGroupData> *last = list;
    CList<PartsGroupData> *next;
    if (last == 0) {
        list = entry;
        return;
    }
    if (last != 0) {
        do {
            next = last->next;
            if (next == 0) {
                break;
            }
            last = next;
        } while (next != 0);
    }
    last->next = entry;
    if (entry != NULL) {
        entry->prev = last;
    }
}

void CMapWater::Initialize() {
    frame = NULL;
    *(u_long128 *)follow = 0;
    parts = NULL;
    parts_max = 0;
    parts_num = 0;
    parts_name = NULL;
}

void CMapWater::Clear() {
    int i;

    parts_num = 0;
    if (parts != NULL) {
        for (i = 0; i < parts_max; i++) {
            parts[i] = NULL;
        }
    }
}

CPartsGroup *CMap::GetPartsGroup(int no) {
    if (no < 0 || no >= parts_group_max) {
        return NULL;
    }
    return &parts_group[no];
}

#ifdef NONMATCHING
int CMap::AddPartsGroup(char *name, CMapParts *parts, mgCMemory *stack) {
    CList<PartsGroupData> *entry;
    CPartsGroup          *group;
    char                 *group_name;
    int                   no;

    no = SearchPartsGroupNo(name);
    group_name = NULL;
    if (no < 0) {
        no = SerachEmptyPartsGroupNo();
        group_name = mgCopyString(name, stack);
    }
    group = GetPartsGroup(no);
    if (group == NULL) {
        return -1;
    }
    if (group_name != NULL) {
        group->name = group_name;
    }
    entry = new ((u_long128 *)stack->Alloc(3)) CList<PartsGroupData>;
    entry->data.parts = parts;
    group->Add(entry);
    return no;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory);
#endif

template <>
void CList<PartsGroupData>::Initialize() {
    prev = NULL;
    next = NULL;
}

CPartsGroup *CMap::SearchPartsGroup(char *name) {
    return GetPartsGroup(SearchPartsGroupNo(name));
}

int CMap::SearchPartsGroupNo(char *name) {
    int i;
    for (i = 0; i < parts_group_max; i++) {
        u8 used = !!parts_group[i].name ^ 1;
        if (!used && strcmp(parts_group[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int CMap::SerachEmptyPartsGroupNo() {
    int i;
    for (i = 0; i < parts_group_max; i++) {
        u8 empty = !!parts_group[i].name ^ 1;
        if (empty) {
            return i;
        }
    }
    return -1;
}

void CMap::Initialize() {
    int i;
    int j;
    int k;
    parts_list = NULL;
    effect_list.pack = NULL;
    effect_list.name = NULL;
    effect_list.block = -1;
    effect_list.effect_num = 0;
    effect_list.managers = NULL;
    effect_list.sprites = NULL;
    place_parts = NULL;
    place_parts_max = 0;
    draw_parts_num = 0;
    draw_parts = NULL;
    mds_list_set = NULL;
    camera_info_num = 0;
    camera_info = NULL;
    draw_rect_max = MAP_DRAW_RECT_MAX;
    for (i = 0; i < MAP_DRAW_RECT_MAX; i++) {
        draw_rect[i].outside = 0;
        draw_rect[i].used = 0;
        draw_rect[i].parts = NULL;
    }
    parts_group_max = MAP_PARTS_GROUP_MAX;
    for (j = 0; j < MAP_PARTS_GROUP_MAX; j++) {
        parts_group[j].Initialize();
    }
    parts_event = 0;
    func_point.CFuncPointMngr::Initialize();
    obj_anime_num = 0;
    obj_anime = NULL;
    tr_box_num = 0;
    tr_box = NULL;
    tr_box_texture = -1;
    tr_box_model = NULL;
    unk_30c = 0;
    now_time = 0.0f;
    water_surface_num = 0;
    water_surface = NULL;
    water_num = 0;
    water = NULL;
    fire_raster = NULL;
    anime_time = 0.0f;
    anime_frame = 0;
    occlusion_num = 0;
    for (k = 0; k < MAP_OCCLUSION_MAX; k++) {
        memset(&occlusion[k], 0, sizeof(COcclusion));
    }
    bbox_valid = 0;
    mgZeroVectorW(bbox.max);
    mgZeroVectorW(bbox.min);
    piece_load_skip = 0;
}


void CMap::SetPlacePartsBuff(mgCMemory *stack, int max) {
    unsigned int list_bytes;
    u_long128 *block = stack->Alloc(algn16_size((u_int)max * sizeof(CMapParts)) + 2);
    place_parts = new (block) CMapParts[max];
    list_bytes = max << 2;
    block = stack->Alloc(algn16_size(list_bytes) + 2);
    draw_parts = (CMapParts **)new (block) char[list_bytes];
    place_parts_max = max;
    ClearPlaceParts();
}

CMapParts::CMapParts() { Initialize(); }

CMapParts *CMap::GetPlacPartsTable(int *out_max) {
    *out_max = place_parts_max;
    return place_parts;
}

void CMap::SetCameraInfoTable(CCameraInfo *table, int num) {
    camera_info_num = num;
    camera_info = table;
}

CCameraInfo *CMap::GetCameraInfo(int no) {
    if (no < 0 || no >= camera_info_num) {
        return NULL;
    }
    return &camera_info[no];
}

CMapParts *CMap::NewPlaceParts() {
    for (int i = 0; i < place_parts_max; i++) {
        u8 unused = place_parts[i].name[0] == 0;
        if (unused) {
            return &place_parts[i];
        }
    }
    return NULL;
}

CMdsInfo *CMap::SearchMDS(char *name) {
    CMdsInfo *model;

    model = NULL;
    if (mds_list_set != NULL) {
        model = mds_list_set->SearchMDS(name);
    }
    return model;
}

void CMap::CreateEffect(unsigned int *pack, int tex_block, mgCMemory *stack) {
    effect_list.LoadEFPFile("test", pack, tex_block, stack);
}

int CMap::SaerchEffectIndex(char *name) {
    return effect_list.SaerchEffectIndex(name);
}

void CMap::AddParts(CList<CMapParts> *parts) {
    CList<CMapParts> *last;
    CList<CMapParts> *next;
    if (parts != 0) {
        last = parts_list;
        if (last != 0) {
            if (last != 0) {
                do {
                    next = last->next;
                    if (next == 0) {
                        break;
                    }
                    last = next;
                } while (next != 0);
            }
            last->next = parts;
            if (parts != 0) {
                parts->prev = last;
            }
        } else {
            parts_list = parts;
        }
    }
}

CMapParts *CMap::GetParts(char *name) {
    CList<CMapParts> *entry;
    CMapParts       *parts;

    if (name == NULL || name[0] == 0) {
        return NULL;
    }
    for (entry = parts_list; entry != NULL; entry = entry->next) {
        parts = &entry->data;
        if (parts != NULL && parts->name != NULL && strcasecmp(name, parts->name) == 0) {
            return parts;
        }
    }
    return NULL;
}

#ifdef NONMATCHING
void CMap::CreateDrawRect(mgCMemory *stack, mgVu0FBOX *area, mgVu0FBOX *parts_box, int outside) {
    mgVu0FBOX           bounds;
    MapDrawOffRect     *rect;
    CList<CMapParts *> *entry;
    CList<CMapParts *> *last;
    CMapParts          *parts;
    int                 i;

    rect = NULL;
    for (i = 0; i < draw_rect_max; i++) {
        if (!draw_rect[i].used) {
            rect = &draw_rect[i];
            break;
        }
    }
    area->max[3] = 1.0f;
    area->min[3] = 1.0f;
    parts_box->max[3] = 1.0f;
    parts_box->min[3] = 1.0f;
    if (rect != NULL) {
        rect->used = 1;
        rect->area = *area;
        rect->outside = outside;
        parts = place_parts;
        for (i = 0; i < place_parts_max; i++, parts++) {
            if (parts->name[0] != 0 && parts->GetBoundBox(&bounds) && mgClipInBox(bounds.max, bounds.min, parts_box->max, parts_box->min)) {
                entry = new ((u_long128 *)stack->Alloc(3)) CList<CMapParts *>;
                entry->data = parts;
                last = rect->parts;
                if (last == NULL) {
                    rect->parts = entry;
                } else {
                    while (last->next != NULL) {
                        last = last->next;
                    }
                    last->next = entry;
                    if (entry != NULL) {
                        entry->prev = last;
                    }
                }
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi);
#endif

template <>
void CList<CMapParts *>::Initialize() {
    prev = NULL;
    next = NULL;
}

void CMap::CreateOcclusion(float (*corner)[4]) {
    if (occlusion_num < MAP_OCCLUSION_MAX) {
        occlusion[occlusion_num].enable = 1;
        *(u_long128 *)occlusion[occlusion_num].vertex[0] = *(u_long128 *)corner[0];
        *(u_long128 *)occlusion[occlusion_num].vertex[1] = *(u_long128 *)corner[1];
        *(u_long128 *)occlusion[occlusion_num].vertex[2] = *(u_long128 *)corner[2];
        *(u_long128 *)occlusion[occlusion_num].vertex[3] = *(u_long128 *)corner[3];
        occlusion_num++;
    }
}

CMapParts *CMap::PlaceParts(char *name, float *pos, float *rot, float *scale, mgCMemory *stack) {
    CMapParts *model;
    CMapParts *parts;

    model = GetParts(name);
    if (model == NULL) {
        return NULL;
    }
    parts = NewPlaceParts();
    if (parts == NULL) {
        return NULL;
    }
    model->Copy(*parts, stack);
    parts->SetPosition(pos);
    parts->SetRotation(rot);
    parts->SetScale(scale);
    return parts;
}

void CMap::PlacePartsEnd() {
    mgVu0FBOX bounds; {
        int j;
        CMapWater *surface;
        for (j = 0; j < water_num; j++) {
            surface = &water[j];
            if (surface->frame != NULL && surface->parts_name == NULL) {
                surface->parts[surface->parts_num++] = NULL;
            }
        }
    }
    int i;
    int j;
    CMapWater *surface;
    CMapParts *parts;
    char *name;
    place_parts_num = place_parts_max;
    for (i = 0; i < place_parts_max; i++) {
        parts = &place_parts[i];
        u8 unused = *(s8 *)parts->name == 0;
        if (!unused) {
            place_parts_num = i + 1;
        }
        if (parts->GetBoundBox(&bounds)) {
            if (!bbox_valid) {
                bbox = bounds;
                bbox_valid = 1;
            } else {
                mgBoxMaxMin(&bbox, &bounds);
            }
        }
        name = parts->parts_name;
        if (name != NULL) {
            for (j = 0; j < water_num; j++) {
                surface = &water[j];
                if (surface->frame != NULL && surface->parts_name != NULL && strcmp(surface->parts_name, name) == 0) {
                    if (surface->parts_num < surface->parts_max) {
                        surface->parts[surface->parts_num++] = parts;
                    }
                }
            }
        }
    }
}

void CMap::ClearPlaceParts() {
    int i;
    int j;
    place_parts_num = place_parts_max;
    for (i = 0; i < place_parts_max; i++) {
        place_parts[i].Initialize();
        draw_parts[i] = NULL;
    }
    for (j = 0; j < water_num; j++) {
        water[j].Clear();
    }
}

CMapParts *CMap::GetPlaceParts(char *name) {
    int i;

    for (i = 0; i < place_parts_num; i++) {
        if (strcmp(name, place_parts[i].name) == 0) {
            return &place_parts[i];
        }
    }
    return NULL;
}

CMapParts *CMap::GetPlaceParts(int no) {
    if (no < 0 || place_parts_num < no) {
        return NULL;
    }
    return &place_parts[no];
}

int CMap::ConvertParts(CMapParts *parts) {
    int no;

    no = -1;
    if (parts != NULL) {
        no = parts - place_parts;
    }
    return no;
}

int CMap::GetPlaceParts(mgVu0FBOX *box, CMapParts **out_parts, int max) {
    mgVu0FBOX bounds;
    CMapParts *parts;
    int count;
    int i;
    int out_index;
    if (box == 0) {
        return 0;
    }
    parts = place_parts;
    count = 0;
    i = 0;
    out_index = 0;
    for (; i < place_parts_num; i++, parts++) {
        u8 unused = *(s8 *)parts->name == 0;
        if (unused) {
            continue;
        }
        if (parts->GetBoundBox(&bounds) == 0) {
            continue;
        }
        if (mgClipBox(bounds.max, bounds.min, box->max, box->min) == 0) {
            continue;
        }
        count++;
        out_parts[out_index++] = parts;
        if (count >= max) {
            break;
        }
    }
    return count;
}

int CMap::GetPlaceColParts(mgVu0FBOX *box, CMapParts **out_parts, int max) {
    CMapParts *parts;
    int count;
    int i;
    int out_index;
    if (box == 0) {
        return 0;
    }
    parts = place_parts;
    count = 0;
    i = 0;
    out_index = 0;
    for (; i < place_parts_num; i++, parts++) {
        u8 unused = *(s8 *)parts->name == 0;
        if (unused) {
            continue;
        }
        if (parts->CheckColBox(box) == 0) {
            continue;
        }
        count++;
        out_parts[out_index++] = parts;
        if (count >= max) {
            break;
        }
    }
    return count;
}

void CMap::CreateFuncCheck(CFuncPointCheck *check) {
    check->time = GetNowTime();
    check->anime_frame = anime_frame;
}

int CMap::GetBBox(mgVu0FBOX *out_box) {
    *out_box = bbox;
    return bbox_valid;
}

int CMap::PreDraw(float *view_pos) {
    CMapParts              *parts;
    MapDrawOffRect         *rect;
    CPartsGroup            *group;
    CList<CMapParts *>     *rect_entry;
    CList<PartsGroupData>  *group_entry;
    CList<CMapPiece>       *piece;
    int                     active_occlusion;
    int                     index;
    int                     hide;
    if (bbox_valid != 0 && mgInsideScreen(&bbox) == 0) {
        draw_parts_num = 0;
        return 0;
    }
    active_occlusion = 0;
    for (index = 0; index < occlusion_num; index++) {
        COcclusion *current = &occlusion[index];
        if (current->enable != 0) {
            current->Setup(mgRenderInfo.view);
            active_occlusion++;
        }
    }
    CFuncPointCheck check;
    CreateFuncCheck(&check);
    func_point.UpdateFlag(FUNC_POINT_FIRE, &check);
    func_point.UpdateFlag(FUNC_POINT_FLARE, &check);
    func_point.Step(FUNC_POINT_PLIGHT, &check);
    func_point.UpdateFlag(FUNC_POINT_EFFECT, &check);
    parts = place_parts;
    for (index = 0; index < place_parts_num; index++, parts++) {
        if (active_occlusion > 0) {
            parts->in_screen = parts->InsideScreen(occlusion, occlusion_num);
        } else {
            parts->in_screen = parts->InsideScreen();
        }
        if (parts->in_screen != 0) {
            parts->StepFuncPoint(check);
        }
    }
    rect = draw_rect;
    for (index = 0; index < draw_rect_max; index++, rect++) {
        if (rect->used != 0) {
            if (rect->outside == 0) {
                if (!(!(view_pos[0] < rect->area.min[0]) && !(view_pos[1] < rect->area.min[1]) && !(view_pos[2] < rect->area.min[2])
                && view_pos[0] <= rect->area.max[0] && view_pos[1] <= rect->area.max[1])) {
                    continue;
                }
                do {
                    if (!(view_pos[2] <= rect->area.max[2])) {
                        break;
                    }
                    goto hide_parts;
                } while (0);
                continue;
            }
            if (!(view_pos[0] <= rect->area.min[0]) && !(view_pos[1] <= rect->area.min[1]) && !(view_pos[2] <= rect->area.min[2])
            && view_pos[0] < rect->area.max[0] && view_pos[1] < rect->area.max[1] && view_pos[2] < rect->area.max[2]) {
                continue;
            }
            hide_parts: {
                for (rect_entry = rect->parts; rect_entry != NULL; rect_entry = rect_entry->next) {
                    if (rect_entry->data != NULL) {
                        rect_entry->data->in_screen = 0;
                    }
                }
            }
        }
    } {
        int group_num = parts_group_max;
        CPartsGroup *group = parts_group;
        int group_no = 0;
        CList<PartsGroupData> *group_entry;
        if (0 < group_num) {
            do {
                u8 unused = (group->name != NULL) ^ 1;
                if (!unused && (group->camera_off != 0 || group->off != 0)) {
                    for (group_entry = group->list; group_entry != NULL; group_entry = group_entry->next) {
                        if (group_entry->data.parts != NULL) {
                            group_entry->data.parts->in_screen = 0;
                        }
                    }
                    group->camera_off = 0;
                }
                group_no++;
                group++;
            } while (group_no < group_num);
        }
    }
    draw_parts_num = 0;
    if (draw_parts == NULL) {
        return 0;
    }
    parts = place_parts;
    for (index = 0; index < place_parts_num; index++, parts++) {
        if ((u8)(*(s8 *)parts->name == 0) == 0) {
            if (parts->in_screen != 0) {
                draw_parts[draw_parts_num] = parts;
                draw_parts_num++;
            } else {
                for (piece = parts->piece_list; piece != NULL; piece = piece->next) {
                    piece->data.fade_alpha = -1.0f;
                }
            }
        }
    }
    return 1;
}

#ifdef NONMATCHING
int CMap::GetCharaLight(mgCObject *chara, CFuncPoint *points, int max, int use_parts) {
    CFuncPoint       candidate;
    CFuncPoint       nearest;
    CMapParts       *parts;
    sceVu0FMATRIX    world_matrix;
    sceVu0FMATRIX    inverse_matrix;
    sceVu0FVECTOR    local_position;
    float            distance;
    int              nearest_distance;
    CFuncPointCheck  check;
    sceVu0FVECTOR    chara_position;
    sceVu0FVECTOR    direction;
    sceVu0FVECTOR    color;
    CFuncPoint      *point;
    float            attenuation;
    int              light_num;
    int              light_mode;
    int              index;

    if (max <= 0) {
        return 0;
    }

    CreateFuncCheck(&check);
    chara->GetPosition(chara_position);
    chara_position[3] = 0.0f;
    chara_position[1] += 20.0f;
    light_mode = 1;
    if (use_parts != 0) {
        light_mode |= 0x2;
    }

    light_num = func_point.GetLight(chara_position, points, max, &check, light_mode);
    if (light_num >= 3) {
        light_num = 2;
    }
    for (index = 0; index < light_num; index++) {
        point = &points[index];
        sceVu0SubVector(direction, point->position, chara_position);
        attenuation = point->plight.power * point->plight.power / mgDistVector2(direction);
        if (!(attenuation <= 1.0f)) {
            attenuation = 1.0f;
        }
        sceVu0ScaleVector(color, point->plight.color, 0.4f * (attenuation * GetLightAnimeWeight(point, anime_frame)));
        color[3] = 128.0f;
        sceVu0Normalize(direction, direction);
        mgSetLight(3 - index, direction, color);
    }

    if (use_parts != 0) {
        chara->GetPosition(chara_position);
        chara_position[3] = 1.0f;
        GetNowTime();

        nearest_distance = 0x4876E000;
        nearest.type = FUNC_POINT_NONE;
        parts = place_parts;
        for (index = 0; index < place_parts_num; index++, parts++) {
            if ((parts->func_point_mngr.flag & FUNC_POINT_MNGR_LIGHT) != 0 && parts->name[0] != '\0') {
                chara_position[3] = 1.0f;
                parts->GetLWMatrix(world_matrix);
                mgInversMatrix(inverse_matrix, world_matrix);
                sceVu0ApplyMatrix(local_position, inverse_matrix, chara_position);
                local_position[3] = 0.0f;
                if (parts->func_point_mngr.GetLight(local_position, &candidate, 1, &check, light_mode) > 0) {
                    distance = mgDistVector(candidate.position, local_position);
                    if (distance < nearest_distance) {
                        nearest_distance = (int)distance;
                        nearest = candidate;
                        nearest.position[3] = 1.0f;
                        sceVu0ApplyMatrix(nearest.position, world_matrix, nearest.position);
                    }
                }
            }
        }

        if (nearest.type == FUNC_POINT_PLIGHT) {
            sceVu0SubVector(direction, nearest.position, chara_position);
            attenuation = nearest.plight.power / mgDistVector(direction);
            attenuation *= attenuation;
            if (!(attenuation <= 1.0f)) {
                attenuation = 1.0f;
            }
            sceVu0ScaleVector(color, nearest.plight.color, 0.4f * (attenuation * GetLightAnimeWeight(&nearest, anime_frame)));
            color[3] = 128.0f;
            sceVu0Normalize(direction, direction);
            mgSetLight(2, direction, color);
        }
    }
    return light_num;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", GetCharaLight__4CMapFP9mgCObjectP10CFuncPointii);
#endif

int CMap::SetFuncPLight(float *pos, CFuncPointCheck *check) {
    static CFuncPoint points[8];
    sceVu0FVECTOR    color;
    CFuncPoint      *point;
    int              light_num;
    int              index;

    light_num = func_point.GetLight(pos, points, 3, check, 0);
    for (index = 0; index < light_num; index++) {
        point = &points[index];
        sceVu0ScaleVector(color, point->plight.color, GetLightAnimeWeight(point, anime_frame));
        mgSetPlight(3 - index, point->position, color, point->plight.power, point->plight.range);
    }
    return light_num;
}

void CMap::ResetFuncPLight(int num) {
    int index;

    for (index = 0; index < num; index++) {
        mgSetPlight(3 - index, NULL);
    }
}

int CMap::DrawSub(int direct) {
    int previous_plight = mgGetPlightEnable();
    int previous_lighting = mgActiveLighting(2, 1);
    CFuncPointCheck check;
    sceVu0FVECTOR sphere;
    CMapParts **list;
    CMapParts *parts;
    int draw_num;
    int light_num;
    int drawn;
    int index;
    check.time = 0;
    CreateFuncCheck(&check);
    draw_num = 0;
    list = draw_parts;
    GetNowTime();
    for (index = 0; index < draw_parts_num; index++, list++) {
        parts = *list;
        parts->CopyFuncPointCheck(check);
        if ((func_point.flag & FUNC_POINT_MNGR_LIGHT) != 0) {
            parts->GetBoundSphere(sphere);
            light_num = SetFuncPLight(sphere, &check);
        } else {
            light_num = 0;
        }
        if (light_num > 0) {
            mgPlightEnable(1);
        }
        if (direct != 0) {
            drawn = parts->DrawDirect();
        } else {
            drawn = parts->Draw();
        }
        draw_num += drawn;
        ResetFuncPLight(light_num);
    }
    mgPlightEnable(previous_plight);
    if (previous_lighting >= 0) {
        mgActiveLighting(previous_lighting, 0);
    }
    return draw_num;
}

#ifndef NONMATCHING
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", Draw__9CMapPartsFv);
#endif

#ifndef NONMATCHING
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", DrawDirect__9CMapPartsFv);
#endif

void CMap::DrawEffect() {
    CMapParts **list;
    CMapParts *parts;
    CFuncPoint *parts_point;
    CFuncPoint *point;
    int index;
    GetNowTime();
    effect_list.CreatePacket();
    CFuncPointCheck check;
    check.time = 0;
    CreateFuncCheck(&check);
    static mgCFrameAttr attr;
    attr.draw = MG_FRAME_DRAW_VISIBLE | MG_FRAME_DRAW_SKIP_CHILDREN;
    attr.no_cull = 1;
    attr.fog = 2;
    attr.no_cull = 1;
    attr.depth_bias = 1.015f;
    func_point.GetStart(FUNC_POINT_EFFECT);
    if ((point = func_point.Get()) != 0) {
        do {
            if (point->Check(&check) != 0) {
                point->frame.SetVisual(effect_list.GetEffectVisual(point->effect.index));
                point->frame.attr = &attr;
                mgDrawDirect(&point->frame);
            }
        } while ((point = func_point.Get()) != 0);
    }
    func_point.GetEnd();
    list = draw_parts;
    for (index = 0; index < draw_parts_num; index++, list++) {
        parts = *list;
        if (parts->CheckDraw() == 0) {
            continue;
        }
        parts->func_point_mngr.GetStart(FUNC_POINT_EFFECT);
        if ((parts_point = parts->func_point_mngr.Get()) != 0) {
            do {
                if (parts_point->active != 0) {
                    parts_point->frame.SetReference(&parts->frame);
                    parts_point->frame.SetVisual(effect_list.GetEffectVisual(parts_point->effect.index));
                    parts_point->frame.attr = &attr;
                    mgDrawDirect(&parts_point->frame);
                    parts_point->frame.DeleteReference();
                }
            } while ((parts_point = parts->func_point_mngr.Get()) != 0);
        }
    }
}

void CMap::DrawFireEffect(int tex_block) {
    CFuncPointCheck check;
    sceVu0FMATRIX world_matrix;
    mgCTexture *fire_texture;
    mgCTexture *light_texture;
    CMapParts **list;
    CMapParts *parts;
    int index;
    check.time = 0;
    CreateFuncCheck(&check);
    mgTexManager.ReloadTexture(tex_block, (sceVif1Packet *)NULL);
    fire_texture = mgTexManager.GetTexture("fire_wrk", tex_block);
    light_texture = mgTexManager.GetTexture("lightling", tex_block);
    mgUnitMatrix(world_matrix);
    ::DrawFireEffect(world_matrix, &func_point, &check, 1.0f, fire_texture, light_texture);
    list = draw_parts;
    if (list != 0) {
        for (index = 0; index < draw_parts_num; index++, list++) {
            parts = *list;
            if ((parts->func_point_mngr.flag & FUNC_POINT_MNGR_BURN) == 0) {
                continue;
            }
            if (parts->CheckDraw() == 0) {
                continue;
            }
            parts->GetLWMatrix(world_matrix);
            ::DrawFireEffect(world_matrix, &parts->func_point_mngr, &check,
            1.0f, fire_texture, light_texture);
        }
    }
}

void CMap::DrawFireRaster() {
    CFuncPointCheck check;
    sceVu0FMATRIX world_matrix;
    CMapParts **list;
    int index;
    CMapParts *parts;
    check.time = 0;
    CreateFuncCheck(&check);
    mgUnitMatrix(world_matrix);
    ::DrawFireRaster(world_matrix, &func_point, &check, fire_raster);
    list = draw_parts;
    if (list != 0) {
        for (index = 0; index < draw_parts_num; index++, list++) {
            parts = *list;
            u8 unused = *(s8 *)parts->name == 0;
            if (unused) {
                continue;
            }
            if (parts->CheckDraw() == 0) {
                continue;
            }
            parts->GetLWMatrix(world_matrix);
            ::DrawFireRaster(world_matrix, &parts->func_point_mngr, &check, fire_raster);
        }
    }
}

#ifdef NONMATCHING
void CMap::DrawWater(mgCCamera *camera, mgCTexture *screen, mgCTexture *overlay) {
    mgCDrawPrim    prim;
    sceVu0FVECTOR  overlay_position;
    sceVu0FVECTOR  overlay_rotation;
    sceVu0FVECTOR  overlay_scale;
    mgCTexture     framebuffer;
    mgRect<int>    screen_rect(0, 0, (mgScreenWidth - 1) * 16, (mgScreenHeight - 1) * 16);
    sceVu0FVECTOR  camera_position;
    sceVu0FVECTOR  camera_direction;
    sceVu0FVECTOR  camera_rotation;
    sceVu0FMATRIX  identity;
    sceVu0FMATRIX  parts_matrix;
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  rotation;
    sceVu0FVECTOR  scale;
    CMapWater     *placement;
    CWaterFrame   *surface;
    CMapParts     *parts;
    int            surface_no;
    int            placement_no;
    int            parts_no;
    int            ripple_row;
    int            ripple_column;

    if (water_surface_num <= 0) {
        return;
    }
    if (screen == NULL) {
        return;
    }
    if (water_num <= 0) {
        return;
    }

    mgZeroVector(camera_position);
    mgZeroVector(camera_rotation);
    if (camera != NULL) {
        camera->GetDir(camera_direction);
        camera->GetPos(camera_position);
        sceVu0Normalize(camera_direction, camera_direction);
        sceVu0ScaleVector(camera_direction, camera_direction, 400.0f);
        mgAddVector(camera_position, camera_direction);
        camera_rotation[1] = mgAngleLimit(atan2f(camera_direction[0], camera_direction[2]));
    }

    for (surface_no = 0; surface_no < water_surface_num; surface_no++) {
        if (water_surface[surface_no] != NULL) {
            water_surface[surface_no]->CreatePacket();
            water_surface[surface_no]->SetTexture(screen);
            ripple_row = (int)(48.0f * ((float)rand() / 2147483648.0f));
            ripple_column = (int)(32.0f * ((float)rand() / 2147483648.0f));
            water_surface[surface_no]->Shake(ripple_row, ripple_column, 0.1f);
            water_surface[surface_no]->SetParam(0.15f, 0.0045f, 0.0f, 16.0f);
            water_surface[surface_no]->Step();
            water_surface[surface_no]->SetColor(0x80, 0x80, 0x80, 0x80);
        }
    }

    mgTexManager.ReloadTexture(screen->block, (sceVif1Packet *)NULL);

    mgGetFrameBuffer(&framebuffer);
    mgSetPkMoveImage(&framebuffer, screen_rect, screen, 0, 0, 0);
    placement = water;
    mgUnitMatrix(identity);

    for (placement_no = 0; placement_no < water_num; placement_no++, placement++) {
        surface = placement->frame;
        if (surface != NULL) {
            placement->GetPosition(position);
            placement->GetRotation(rotation);
            placement->GetScale(scale);
            if (placement->follow[0] != 0) {
                position[0] = camera_position[0];
            }
            if (placement->follow[1] != 0) {
                position[1] = camera_position[1];
            }
            if (placement->follow[2] != 0) {
                position[2] = camera_position[2];
            }
            surface->SetPosition(position);
            surface->SetRotation(rotation);
            if (placement->follow[0] != 0 && placement->follow[2] != 0) {
                surface->SetRotation(camera_rotation);
            }
            surface->SetScale(scale);

            for (parts_no = 0; parts_no < placement->parts_num; parts_no++) {
                parts = placement->parts[parts_no];
                if (parts == NULL) {
                    mgDrawDirect(surface);
                } else {
                    parts->GetLWMatrix(parts_matrix);
                    surface->SetTransMatrix(parts_matrix);
                    mgDrawDirect(surface);
                    surface->SetTransMatrix(identity);
                }
            }
        }
    }

    if (overlay != NULL) {
        prim.Initialize(NULL, NULL);
        prim.DepthTestEnable(0);
        prim.ZMask(-1);
        prim.TextureMapEnable(1);
        prim.AlphaBlendEnable(0);
        prim.AlphaTestEnable(0);
        mgSetPkFrameBuffer(screen);
        prim.Begin(6);
        prim.Texture(overlay);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.TextureCrd(0, 0);
        prim.Vertex(0, 0, 0);
        prim.TextureCrd(0x80, 0x80);
        prim.Vertex(mgScreenWidth, mgScreenHeight, 0);
        prim.End();
        mgSetPkFrameBuffer(-1, -1, -1, -1);
        placement = water;

        for (placement_no = 0; placement_no < water_num; placement_no++, placement++) {
            surface = placement->frame;
            if (surface != NULL) {
                placement->GetPosition(overlay_position);
                placement->GetRotation(overlay_rotation);
                placement->GetScale(overlay_scale);
                placement->GetPosition(overlay_position);
                placement->GetRotation(overlay_rotation);
                placement->GetScale(overlay_scale);
                if (placement->follow[0] != 0) {
                    overlay_position[0] = camera_position[0];
                }
                if (placement->follow[1] != 0) {
                    overlay_position[1] = camera_position[1];
                }
                if (placement->follow[2] != 0) {
                    overlay_position[2] = camera_position[2];
                }
                surface->SetPosition(overlay_position);
                surface->SetRotation(overlay_rotation);
                if (placement->follow[0] != 0 && placement->follow[2] != 0) {
                    surface->SetRotation(camera_rotation);
                }
                surface->SetColor(0x80, 0x80, 0x80, 0x20);
                surface->SetParam(0.15f, 0.0045f, 0.0f, 300.0f);
                surface->SetScale(overlay_scale);

                for (parts_no = 0; parts_no < placement->parts_num; parts_no++) {
                    parts = placement->parts[parts_no];
                    if (parts == NULL) {
                        mgDrawDirect(surface);
                    } else {
                        parts->GetLWMatrix(parts_matrix);
                        surface->SetTransMatrix(parts_matrix);
                        mgDrawDirect(surface);
                        surface->SetTransMatrix(identity);
                    }
                }
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", DrawWater__4CMapFP9mgCCameraP10mgCTextureP10mgCTexture);
#endif

void CMap::DrawTrBox() {
    int plight_enable;
    int lighting;
    sceVu0FVECTOR light_sphere;
    CMapTreasureBox *box;
    int box_no;
    int light_num;
    if (tr_box_num == 0 || tr_box == 0) {
        return;
    }
    mgTexManager.ReloadTexture(tr_box_texture, (sceVif1Packet *)0);
    plight_enable = mgGetPlightEnable();
    lighting = mgActiveLighting(2, 1);
    CFuncPointCheck check;
    check.time = 0;
    CreateFuncCheck(&check);
    GetNowTime();
    box = tr_box;
    for (box_no = 0; box_no < tr_box_num; box_no++, box++) {
        if (box->active == 0) {
            continue;
        }
        CMapParts *link = box->parts;
        if (link != 0 && link->GetShow() == 0) {
            continue;
        }
        if (func_point.flag & FUNC_POINT_MNGR_LIGHT) {
            box->GetPosition(light_sphere);
            light_sphere[3] = 40.0f;
            light_num = SetFuncPLight(light_sphere, &check);
        } else {
            light_num = 0;
        }
        if (light_num > 0) {
            mgPlightEnable(1);
        }
        box->DrawDirect();
        ResetFuncPLight(light_num);
    }
    mgPlightEnable(plight_enable);
    if (lighting >= 0) {
        mgActiveLighting(lighting, 0);
    }
}

int CMap::GetPoly(int kind, CCPoly *polys, mgVu0FBOX &box, int max) {
    CMapParts *parts_table[128];
    int parts_count = GetPlaceColParts(&box, parts_table, 128);
    int count = 0;
    int i;
    int j;
    for (i = 0; i < parts_count; i++) {
        CMapParts *parts = parts_table[i];
        u8 unused = *(s8 *)parts->name == 0;
        if (unused) {
            continue;
        }
        if (parts->GetShow() == 0) {
            continue;
        }
        int copied = parts->GetPoly(kind, polys, box, max);
        if (0 < copied) {
            j = 0;
            do {
                j++;
                polys->parts_no = i;
                polys++;
            } while (j < copied);
        }
        max -= copied;
        count += copied;
        if (max <= 0) {
            return count;
        }
    }
    return count;
}

int CMap::GetColPoly(CCPoly *polys, mgVu0FBOX &box, int max) {
    return GetPoly(1, polys, box, max);
}

int CMap::GetCameraPoly(CCPoly *polys, mgVu0FBOX &box, int max) {
    return GetPoly(3, polys, box, max);
}

int CMap::GetTrBoxColPoly(CCPoly *polys, float *param, int max) {
    sceVu0FVECTOR position;
    int count = 0;
    CMapTreasureBox *box = tr_box;
    int i;
    int copied;
    for (i = 0; i < tr_box_num; i++, box++) {
        if (box->active == 0) {
            continue;
        }
        CMapParts *linked_parts = box->parts;
        if (linked_parts != 0 && linked_parts->GetShow() == 0) {
            continue;
        }
        box->GetWorldPosition(position);
        copied = CreateCharaCPoly(polys, max, position, param, 5.0f, 20.0f);
        count += copied;
        polys += copied;
        max -= copied;
        if (max < 0) {
            break;
        }
    }
    return count;
}

#ifdef NONMATCHING
int CMap::GetFixCameraPos(float *pos, float *out_camera_pos) {
    sceVu0FVECTOR projection[8];
    sceVu0FVECTOR direction;
    sceVu0FVECTOR offset;
    sceVu0FVECTOR projection_sum;
    CCameraInfo  *selected;
    CCameraInfo  *camera;
    float         segment_length2;
    float         weight;
    float         nearest_distance2;
    float         nearest_distance;
    float         distance;
    int           camera_no;
    int           rect_no;
    int           segment_no;
    int           projection_num;
    int           nearest_projection;
    int           point_no;

    selected = NULL;
    camera = camera_info;
    for (camera_no = 0; camera_no < camera_info_num; camera_no++, camera++) {
        if (camera->rect[0] == NULL) {
            selected = camera;
        }
    }
    camera = camera_info;
    for (camera_no = 0; camera_no < camera_info_num; camera_no++, camera++) {
        for (rect_no = 0; rect_no < camera->rect_num; rect_no++) {
            if (camera->rect[rect_no] == NULL) {
                break;
            }
            if (camera->rect[rect_no]->InsidePoint(pos) != 0) {
                selected = camera;
            }
        }
    }

    if (selected == NULL) {
        return MAP_FIX_CAMERA_NONE;
    }
    if (selected->pos_num >= 2) {
        projection_num = 0;
        nearest_projection = -1;
        for (segment_no = 0; segment_no < selected->pos_num - 1; segment_no++) {
            sceVu0SubVector(direction, selected->pos[segment_no + 1], selected->pos[segment_no]);
            sceVu0SubVector(offset, pos, selected->pos[segment_no]);
            segment_length2 = mgDistVector2(direction);
            weight = sceVu0InnerProduct(direction, offset) / segment_length2;
            if (!(weight < 0.0f) && weight <= 1.0f) {
                sceVu0ScaleVector(projection[projection_num], direction, weight);
                mgAddVector(projection[projection_num], selected->pos[segment_no]);
                if (nearest_projection < 0) {
                    nearest_projection = projection_num;
                } else {
                    nearest_distance2 = mgDistVector2(pos, projection[nearest_projection]);
                    if (mgDistVector2(pos, projection[projection_num]) < nearest_distance2) {
                        nearest_projection = projection_num;
                    }
                }
                projection_num++;
            }
        }

        mgZeroVector(projection_sum);
        for (point_no = 0; point_no < projection_num; point_no++) {
            mgAddVector(projection_sum, projection[point_no]);
        }
        point_no = 0;
        if (projection_num > 0) {
            *(u_long128 *)out_camera_pos = *(u_long128 *)projection[nearest_projection];
            nearest_distance = mgDistVector(out_camera_pos, pos);
        } else {
            nearest_distance = mgDistVector(selected->pos[0], pos);
            *(u_long128 *)out_camera_pos = *(u_long128 *)selected->pos[0];
            point_no = 1;
        }
        for (; point_no < selected->pos_num; point_no++) {
            distance = mgDistVector(pos, selected->pos[point_no]);
            if (distance < nearest_distance) {
                nearest_distance = distance;
                *(u_long128 *)out_camera_pos = *(u_long128 *)selected->pos[point_no];
            }
        }
        return MAP_FIX_CAMERA_PATH;
    }
    *(u_long128 *)out_camera_pos = *(u_long128 *)selected->pos[0];
    return MAP_FIX_CAMERA_POINT;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", GetFixCameraPos__4CMapFPfPf);
#endif

void CMap::FixCameraPartsOnOff(float *camera_pos) { {
        CCameraInfo *camera = camera_info;
        int camera_no;
        int draw_no;
        for (camera_no = 0; camera_no < camera_info_num; camera_no++, camera++) {
            for (draw_no = 0; draw_no < 4; draw_no++) {
                CCameraDrawInfo *draw_info = camera->GetDrawInfo(draw_no);
                if (draw_info != NULL) {
                    CPartsGroup *group = GetPartsGroup(draw_info->group_no);
                    if (group != NULL) {
                        group->camera_off = 0;
                    }
                }
            }
        }
    }
    CCameraInfo *selected = NULL;
    int camera_no;
    CCameraInfo *candidate = camera_info;
    for (camera_no = 0; camera_no < camera_info_num; camera_no++, candidate++) {
        if (mgDistVector(candidate->pos[0], camera_pos) < 10.0f) {
            selected = candidate;
            break;
        }
    }
    if (selected != NULL) {
        for (int draw_no = 0; draw_no < 4; draw_no++) {
            CCameraDrawInfo *draw_info = selected->GetDrawInfo(draw_no);
            if (draw_info != NULL) {
                CPartsGroup *group = GetPartsGroup(draw_info->group_no);
                if (group != NULL) {
                    group->camera_off = 1;
                }
            }
        }
    }
}

CFuncPoint *CMap::GetEvent(float *pos, int check_type, MapEventInfo *info) {
    MapEventInfo event_info;
    MapEventInfo    nearest_info;
    CFuncPoint     *nearest_point;
    MapEventInfo *current;
    CFuncPoint     *point;
    CMapParts      *parts;
    CFuncPointMngr *manager;
    float          nearest_distance;
    float          distance;
    int            last_event;
    int            parts_no;
    int            accepted;
    int            row;

    current = (MapEventInfo *)(u_long128 *)&event_info;
    nearest_point = NULL;
    current->event_no = 0;
    mgUnitMatrix(current->matrix);
    current->point_no = -1;
    current->parts_no = -1;
    func_point.GetStart(FUNC_POINT_EVENT);
    nearest_distance = 0.0f;
    last_event = 0;
    if ((point = func_point.Get()) != NULL) {
        do {
            if (CheckFuncEvent(point, pos, check_type, current, &distance) == 0) {
                if (current != NULL && current->event_no != 0) {
                    last_event = current->event_no;
                }
            } else {
                current->point_no = point->event.point_no;
                if (nearest_point == NULL || distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest_point = point;
                    nearest_info = *current;
                }
            }
        } while ((point = func_point.Get()) != NULL);
    }

    parts = place_parts;
    if (parts_event != 0) {
        for (parts_no = 0; parts_no < place_parts_max; parts_no++, parts++) {
            manager = &parts->func_point_mngr;
            if ((manager->flag & FUNC_POINT_MNGR_EVENT) && (u8)(*(s8 *)parts->name == 0) == 0 && parts->GetShow() != 0) {
                manager->GetStart(FUNC_POINT_EVENT);
                if ((point = manager->Get()) != NULL) {
                    do {
                        point->frame.SetReference(&parts->frame);
                        accepted = CheckFuncEvent(point, pos, check_type, current, &distance);
                        point->frame.DeleteReference();
                        if (current != NULL) {
                            current->parts_no = parts_no;
                            if (current->event_no != 0) {
                                last_event = current->event_no;
                            }
                        }
                        if (accepted != 0) {
                            current->point_no = point->event.point_no;
                            if (nearest_point == NULL || distance < nearest_distance) {
                                nearest_distance = distance;
                                nearest_point = point;
                                nearest_info = *current;
                            }
                        }
                    } while ((point = manager->Get()) != NULL);
                }
            }
        }
    }
    if (info != NULL) {
        *info = nearest_info;
        info->event_no = last_event;
    }
    return nearest_point;
}

CFuncPoint *CMap::InScreenFunc(InScreenFuncInfo *info) {
    CFuncPoint *nearest = 0;
    CMapParts *parts;
    int i;
    CFuncPoint *point;
    float point_info = 0.0f;
    float distance = 0.0f;
    parts = place_parts;
    for (i = 0; i < place_parts_max; i++, parts++) {
        u8 unused = *(s8 *)parts->name == 0;
        if (unused) {
            continue;
        }
        if (parts->CheckDraw() == 0) {
            continue;
        }
        point = parts->InScreenFunc(info);
        if (point == 0) {
            continue;
        }
        if (nearest != 0 && !(info->dist < distance)) {
            continue;
        }
        nearest = point;
        point_info = info->unk_04;
        distance = info->dist;
    }
    info->unk_04 = point_info;
    return nearest;
}

void CMap::DrawScreenFunc(mgCFrame *marker) {
    CMapParts *parts = place_parts;
    int i;
    for (i = 0; i < place_parts_max; i++, parts++) {
        u8 unused = *(s8 *)parts->name == 0;
        if (!unused && parts->CheckDraw() != 0) {
            parts->DrawScreenFunc(marker);
        }
    }
}

void CMap::EffectStep() {
    anime_time += 1.0f;
    anime_frame = (int)anime_time;
    effect_list.Step();
}

#ifdef NONMATCHING
void CMap::AnimeStep(CObjAnimeEnv *env) {
    CFuncPointCheck check;
    CObjAnime      *animation;
    int             i;

    CreateFuncCheck(&check);
    for (i = 0; i < place_parts_num; i++) {
        place_parts[i].AnimeStep(&check, env);
    }
    if (obj_anime_num > 0) {
        animation = obj_anime;
        if (animation == NULL) {
            return;
        }
        for (i = 0; i < obj_anime_num; i++, animation++) {
            if (animation->func_point != NULL && animation->func_point->Check(&check)) {
                animation->Step(env);
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", AnimeStep__4CMapFP12CObjAnimeEnv);
#endif

void CMap::Step() {
    CMapParts *parts = place_parts;
    int i;
    for (i = 0; i < place_parts_num; i++, parts++) {
        parts->Step();
    }
}

int CMap::GetSeSrcVolPan(int *se_no, float *vol, float *pan, int max) {
    CFuncPointCheck check;
    sceVu0FMATRIX  matrix;
    CMapParts     *parts;
    int            count;
    int            copied;
    int            remaining;
    int            i;

    CreateFuncCheck(&check);
    count = 0;
    mgUnitMatrix(matrix);
    copied = ::GetSeSrcVolPan((float(*)[4])matrix, &func_point, &check, se_no, vol, pan, max);
    count += copied;
    se_no += copied;
    max -= copied;
    vol += copied;
    pan += copied;
    parts = place_parts;
    for (i = 0; i < place_parts_max; i++, parts++) {
        u8 unused = *(s8 *)parts->name == 0;
        if (unused) {
            continue;
        }
        if (parts->CheckDraw() == 0) {
            continue;
        }
        if ((parts->func_point_mngr.flag & FUNC_POINT_MNGR_SOUND) == 0) {
            continue;
        }
        parts->GetLWMatrix(matrix);
        if (max <= 0) {
            return count;
        }
        copied = ::GetSeSrcVolPan((float(*)[4])matrix, &parts->func_point_mngr,
        &check, se_no, vol, pan, max);
        count += copied;
        se_no += copied;
        max -= copied;
        vol += copied;
        pan += copied;
    }
    return count;
}

void CMap::CreateMap(CMdsListSet *mds_list_set, mgCMemory *stack) {
    char *script;
    int   size;

    script = GetAddMapFile(&size);
    if (script != NULL && size > 0) {
        LoadMapFile(script, size, stack, 1);
    }
    script = GetMapFile(&size);
    LoadMapFile(script, size, stack, 0);
}

void CMap::AssignFuncPoint(mgCMemory *stack) {
    CObjAnime  *animation;
    CFuncPoint *point;
    CMapParts  *parts;
    obj_anime_num = func_point.GetNum(FUNC_POINT_ANIME);
    if (obj_anime_num > 0) {
        int count = obj_anime_num;
        u_int bytes = (u_int)count * sizeof(CObjAnime);
        unsigned int blocks;
        switch (bytes & 0xF) {
            default: blocks = (bytes >> 4) + 1;
            break;
            case 0: blocks = bytes >> 4;
            break;
        }
        obj_anime = new (stack->Alloc(blocks + 2)) CObjAnime[count];
        CFuncPoint *point;
        CObjAnime *animation = obj_anime;
        if (animation != NULL) {
            func_point.GetStart(FUNC_POINT_ANIME);
            if ((point = func_point.Get()) != NULL) {
                do {
                    animation->frame = NULL;
                    animation->piece = NULL;
                    animation->parts = NULL;
                    animation->func_point = NULL;
                    animation->back = 0;
                    animation->stop = 0;
                    animation->func_point = point;
                    parts = NULL;
                    if (point->anime.parts_name != NULL) {
                        parts = GetPlaceParts(point->anime.parts_name);
                    }
                    animation->AssignFuncAnime(point, parts);
                    animation++;
                } while ((point = func_point.Get()) != NULL);
            }
            func_point.GetEnd();
        }
    }
}

#ifndef NONMATCHING
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", __ct__9CObjAnimeFv);
#endif

void CMap::CreateTrBox(CMapTreasureBox *model, int tex_block, mgCMemory *stack) {
    mgCFrame        *top_frame;
    CMapParts       *parts;
    CFuncPointMngr  *manager;
    CFuncPoint      *point;
    int              parts_index;
    int              box_index;
    if (model == NULL || model->CObjectFrame::frame == NULL) {
        return;
    }
    top_frame = model->CObjectFrame::frame->SearchFrame("top");
    if (top_frame != NULL) {
        top_frame->SetRotType(2);
    }
    model->fade = 1;
    tr_box_texture = tex_block;
    tr_box_model = model;
    tr_box_num = func_point.GetEventNum(FUNC_EVENT_TREASURE_BOX);
    parts = place_parts;
    for (parts_index = 0; parts_index < place_parts_max; parts_index++, parts++) {
        if ((u8)(*(s8 *)parts->name == 0) == 0) {
            tr_box_num += parts->func_point_mngr.GetEventNum(FUNC_EVENT_TREASURE_BOX);
        }
    }
    int count = tr_box_num;
    u_int bytes = (u_int)count * sizeof(CMapTreasureBox);
    unsigned int blocks;
    switch (bytes & 0xF) {
        default: blocks = (bytes >> 4) + 1;
        break;
        case 0: blocks = bytes >> 4;
        break;
    }
    u_long128 *block = stack->Alloc(blocks + 2);
    tr_box = new (block) CMapTreasureBox[count];
    if (tr_box == NULL) {
        tr_box_num = 0;
    } {
        int box_no;
        CFuncPointMngr *current_manager;
        CFuncPoint *current_point;
        CMapParts *linked_parts;
        int parts_no;
        CMapParts *parts_cursor = place_parts;
        box_no = 0;
        for (parts_no = -1, parts_cursor--; parts_no < place_parts_max; parts_cursor++, parts_no++) {
            if (box_no >= tr_box_num) {
                break;
            }
            linked_parts = NULL;
            if (parts_no < 0) {
                current_manager = &func_point;
            } else {
                current_manager = &parts_cursor->func_point_mngr;
                linked_parts = parts_cursor;
            }
            current_manager->GetStart(FUNC_POINT_EVENT);
            if ((current_point = current_manager->Get()) != NULL) {
                do {
                    if ((current_point->event.flag & FUNC_EVENT_TREASURE_BOX) != 0) {
                        tr_box_model->Copy(tr_box[box_no], stack);
                        tr_box[box_no].AssignFuncPoint(current_point, linked_parts);
                        current_point->event.point_no = box_no;
                        box_no++;
                    }
                } while ((current_point = current_manager->Get()) != NULL);
            }
            current_manager->GetEnd();
        }
    }
}

#ifndef NONMATCHING
INCLUDE_ASM("ps2/asm/pal/nonmatchings/map", __ct__15CMapTreasureBoxFv);
#endif

CMapTreasureBox *CMap::GetTrBox(int no) {
    if (no < 0 || no > tr_box_num || tr_box == NULL) {
        return NULL;
    }
    return &tr_box[no];
}

void CMap::DeleteTrBox(int no, CMapFlagData *flags) {
    CMapTreasureBox *box;

    box = GetTrBox(no);
    if (box != NULL) {
        box->active = 0;
        if (box->flag_no > 0) {
            if (flags != NULL) {
                flags->SetFlag(box->flag_no, 1);
            }
            if (box->func_point != NULL) {
                box->func_point->enable = 0;
            }
        }
    }
}

void CMap::UpdateTrBoxFlag(CMapFlagData *flags) {
    CMapTreasureBox *box;
    int i;
    if (flags != NULL) {
        box = tr_box;
        for (i = 0; i < tr_box_num; i++, box++) {
            if (box->flag_no > 0) {
                box->active = !(flags->GetFlag(box->flag_no) != 0);
                if (box->func_point != NULL) {
                    box->func_point->enable = box->active;
                }
            }
        }
    }
}

void CMap::LoadData(unsigned int *pcp_pack, unsigned int *img_pack, int *tex_block, mgCMemory *stack) {
    mgCEnterIMGInfo info;
    int index;
    int block;
    char *name;
    unsigned int *file;
    int first_block;
    if (mds_list_set != NULL) {
        block = *tex_block;
        mgCTextureManager *manager = &mgTexManager;
        for (index = 0; ; index++) {
            name = GetImgName(index);
            if (name == NULL) {
                break;
            }
            file = GetPackFile(img_pack, name, NULL);
            printf("%x %s\n", file, name);
            if (file != NULL) {
                first_block = block;
                block += manager->EnterIMGFile((u_char *)file, block, stack, &info);
                block++;
                manager->EndEnterTexture(first_block);
                mds_list_set->LoadIMGFile(name, &info, stack);
            }
        }
        char *pcp_name;
        int pcp_index;
        for (pcp_index = 0; ; pcp_index++) {
            pcp_name = GetPCPName(pcp_index);
            if (pcp_name == NULL) {
                break;
            }
            file = GetPackFile(pcp_pack, pcp_name, NULL);
            if (file != NULL) {
                mds_list_set->LoadPCPFile(pcp_name, file, stack, all_scissor);
            }
        }
        *tex_block = block - *tex_block;
    }
}

int CheckFuncEvent(CFuncPoint *point, float *pos, int check_type, MapEventInfo *info, float *out_dist) {
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR world_position;
    sceVu0FVECTOR normalized_offset;
    float         scale_x;
    float         scale_y;
    float         scale_z;
    u32           flags;
    if (point->Check(NULL) == 0) {
        return 0;
    }
    point->frame.GetLWMatrix(matrix);
    *(u_long128 *)world_position = *(u_long128 *)matrix[3];
    scale_x = mgDistVector(matrix[0]);
    scale_y = mgDistVector(matrix[1]);
    scale_z = mgDistVector(matrix[2]);
    normalized_offset[0] = (world_position[0] - pos[0]) / scale_x;
    normalized_offset[1] = (world_position[1] - pos[1]) / scale_y;
    normalized_offset[2] = (world_position[2] - pos[2]) / scale_z;
    if (!(mgDistVector(normalized_offset) <= 1.0f)) {
        return 0;
    }
    if (info != NULL) {
        if (point->event.event_no > 0) {
            info->event_no = point->event.event_no;
        }
        info->check_type = check_type;
        flags = point->event.flag;
        if ((flags & FUNC_EVENT_ACTION) != 0 || (flags & FUNC_EVENT_ITEM) != 0) {
            if (check_type == 0) {
                if (flags & FUNC_EVENT_ACTION) {
                    return 0;
                }
                if (flags & FUNC_EVENT_ITEM) {
                    return 0;
                }
            } else if (check_type == 1) {
                if (!(flags & FUNC_EVENT_ACTION)) {
                    return 0;
                }
            } else if (check_type == 2) {
                if (!(flags & FUNC_EVENT_ITEM)) {
                    return 0;
                }
            }
        }
        *(u_long128 *)info->matrix[0] = *(u_long128 *)matrix[0];
        *(u_long128 *)info->matrix[1] = *(u_long128 *)matrix[1];
        *(u_long128 *)info->matrix[2] = *(u_long128 *)matrix[2];
        *(u_long128 *)info->matrix[3] = *(u_long128 *)matrix[3];
    }
    if (out_dist != NULL) {
        *out_dist = mgDistVector(world_position, pos);
    }
    return 1;
}

int CObject::Draw() { return 0; }

int CObject::DrawDirect() { return 0; }

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_327__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_574__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_1352__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_1353__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_1927__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", at_2008__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", __vt__4CMap__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", __vt__18CList_P9CMapParts___DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", __vt__23CList_14PartsGroupData___DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", __vt__9CMapWater__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/map", CMapName__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(init_1249, 0x4);
INCLUDE_BSS(init_1301, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(ft_1248, 0xE00);
INCLUDE_BSS(attr_1300, 0x90);
