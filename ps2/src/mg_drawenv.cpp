#include "common.h"
#include "mg_drawenv.hpp"

#include <cmath>
#include <cstring>

#include "mg_math.hpp"

// Code (.text)
mgCDrawEnv::mgCDrawEnv() {
    Initialize(0);
}

mgCDrawEnv &mgCDrawEnv::operator=(mgCDrawEnv &other) {
    *(u_long128 *)&giftag = *(u_long128 *)&other.giftag;
    *(u_long128 *)&test = *(u_long128 *)&other.test;
    *(u_long128 *)&zbuf = *(u_long128 *)&other.zbuf;
    *(u_long128 *)&alpha = *(u_long128 *)&other.alpha;
    return *this;
}

void mgCDrawEnv::Initialize(int context) {
    *(u_long *)&giftag = 0;
    giftag.NLOOP = 3;
    giftag.EOP = 1;
    giftag.NREG = 1;
    giftag.REGS0 = SCE_GIF_PACKED_AD;
    test.value = SCE_GS_SET_TEST(1, SCE_GS_GEQUAL, 0, 0, 0, 0, 1, SCE_GS_ZGEQUAL);
    alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_CD, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0);
    if (context == 0) {
        test_addr = SCE_GS_TEST_1;
        zbuf_addr = SCE_GS_ZBUF_1;
        alpha_addr = SCE_GS_ALPHA_1;
    } else {
        test_addr = SCE_GS_TEST_2;
        zbuf_addr = SCE_GS_ZBUF_2;
        alpha_addr = SCE_GS_ALPHA_2;
    }
}

void mgCDrawEnv::SetAlpha(int mode) {
    switch (mode) {
    case MG_ALPHA_MACRO_NONE:
        break;
    case MG_ALPHA_MACRO_BLEND:
        alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_CD, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0);
        break;
    case MG_ALPHA_MACRO_ADD:
        alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0);
        break;
    case MG_ALPHA_MACRO_SUB:
        alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_CS, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0);
        break;
    case MG_ALPHA_MACRO_OPAQUE:
        alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_FIX, SCE_GS_ALPHA_CS, 0x80);
        break;
    case MG_ALPHA_MACRO_ADD_FIX:
        alpha.value = SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_FIX, SCE_GS_ALPHA_CD, 0x80);
        break;
    }
}

int mgCDrawEnv::GetAlphaMacroID() {
    if (alpha.value == SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0)) {
        return MG_ALPHA_MACRO_ADD;
    }
    if (alpha.value == SCE_GS_SET_ALPHA(SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_CS, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0)) {
        return MG_ALPHA_MACRO_SUB;
    }
    if (alpha.value == SCE_GS_SET_ALPHA(SCE_GS_ALPHA_CS, SCE_GS_ALPHA_CD, SCE_GS_ALPHA_AS, SCE_GS_ALPHA_CD, 0)) {
        return MG_ALPHA_MACRO_BLEND;
    }
    if (alpha.value == SCE_GS_SET_ALPHA(SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_ZERO, SCE_GS_ALPHA_FIX, SCE_GS_ALPHA_CS, 0x80)) {
        return MG_ALPHA_MACRO_OPAQUE;
    }
    return MG_ALPHA_MACRO_NONE;
}

void mgCDrawEnv::SetZBuf(int mode) {
    switch (mode) {
    case MG_ZBUF_NO_WRITE:
        zbuf.bits.zmsk = 1;
        break;
    case MG_ZBUF_WRITE:
        zbuf.bits.zmsk = 0;
        break;
    }
}

void mgRENDER_INFO::Initialize() {
    draw_env[0].Initialize(0);
    draw_env[1].Initialize(1);
    fog_enable = 0;
    plight_enable = 0;
    unk_fac = 1;
    motion = 0;
    all_scissor = 0;
    light_changed = 1;
}

#ifdef NONMATCHING
void mgRENDER_INFO::SetRenderInfo(float projection, int width, int height, float near_dist,
                                  float far_dist, int zdepth, float aspect_y) {
    float z_range = 1.67e7f;
    if (zdepth == 16) {
        z_range = 65000.0f;
    }
    if (near_dist < 0.0f) {
        near_dist = clip_min[2];
    }
    if (far_dist < 0.0f) {
        far_dist = clip_max[2];
    }
    float depth_offset = (near_dist * far_dist * (z_range - 0.0f)) / (far_dist - near_dist);

    if (projection > 0.0f) {
        this->projection = projection;
    }

    clip_min[0] = 0.0f;
    clip_min[1] = 0.0f;
    clip_min[2] = near_dist;
    clip_max[0] = 4095.9f;
    clip_max[1] = 4095.9f;
    clip_max[2] = far_dist;

    float guard_half_w = (float)width * 0.55f;
    float guard_half_h = (float)height * 0.55f;
    guard_min[0] = 2048.0f - guard_half_w;
    guard_min[1] = 2048.0f - guard_half_h;
    guard_min[2] = 0.0f;
    guard_min[3] = clip_min[2];
    guard_max[0] = guard_half_w + 2048.0f;
    guard_max[1] = guard_half_h + 2048.0f;
    guard_max[2] = 0.0f;
    guard_max[3] = clip_max[2];

    full_min[0] = 1.0f;
    full_min[1] = 1.0f;
    full_min[2] = 0.0f;
    full_min[3] = clip_min[2];
    full_max[0] = 4095.0f;
    full_max[1] = 4095.0f;
    full_max[2] = 0.0f;
    full_max[3] = clip_max[2];

    screen_box_min[0] = (float)(-width / 2);
    screen_box_min[1] = (float)(-height / 2);
    screen_box_min[2] = 0.0f;
    screen_box_min[3] = clip_min[2];
    screen_box_max[0] = screen_box_min[0] + (float)width;
    screen_box_max[1] = screen_box_min[1] + (float)height;
    screen_box_max[2] = 0.0f;
    screen_box_max[3] = clip_max[2];

    gs_box_min[0] = -2047.0f;
    gs_box_min[1] = -2047.0f;
    gs_box_min[2] = 0.0f;
    gs_box_min[3] = clip_min[2];
    gs_box_max[0] = 2047.0f;
    gs_box_max[1] = 2047.0f;
    gs_box_max[2] = 0.0f;
    gs_box_max[3] = clip_max[2];

    sceVu0UnitMatrix(aspect);
    aspect[0][0] = 1.0f;
    aspect[1][1] = aspect_y;
    sceVu0CopyMatrix(aspect, aspect);

    // Perspective projection onto the guard area, then onto the whole GS coordinate range.
    sceVu0FMATRIX proj;
    sceVu0UnitMatrix(proj);
    float near_z = clip_min[2];
    float eye = this->projection;
    float clip_half_w = (near_z * guard_half_w) / eye;
    float clip_half_full = (near_z * 2047.0f) / eye;
    proj[0][0] = near_z / clip_half_w;
    float far_z = clip_max[2];
    float clip_half_h = (near_z * guard_half_h) / eye;
    proj[1][1] = near_z / clip_half_h;
    float depth_scale = (far_z + near_z) / (far_z - near_z);
    float depth_bias = (far_z * near_z * -2.0f) / (far_z - near_z);
    proj[2][2] = depth_scale;
    proj[2][3] = 1.0f;
    proj[3][2] = depth_bias;
    proj[3][3] = 0.0f;
    mgMulMatrix(view_clip, proj, aspect);

    sceVu0UnitMatrix(proj);
    proj[0][0] = near_z / clip_half_full;
    proj[1][1] = proj[0][0];
    proj[2][2] = depth_scale;
    proj[2][3] = 1.0f;
    proj[3][2] = depth_bias;
    proj[3][3] = 0.0f;
    mgMulMatrix(view_clip_full, proj, aspect);

    sceVu0UnitMatrix(clip_screen);
    float eye_scale = eye * 1.0f;
    clip_screen[0][0] = (clip_half_w * eye_scale) / near_z;
    clip_screen[1][1] = (clip_half_h * eye_scale) / near_z;
    clip_screen[2][2] = (-z_range + 0.0f) / 2.0f;
    clip_screen[3][2] = (z_range + 0.0f) / 2.0f;
    clip_screen[3][0] = 2048.0f;
    clip_screen[3][1] = 2048.0f;
    clip_screen[3][3] = 1.0f;
    sceVu0CopyMatrix(clip_screen_full, clip_screen);
    float full_scale = (clip_half_full * eye_scale) / near_z;
    clip_screen_full[0][0] = full_scale;
    clip_screen_full[1][1] = full_scale;

    // Direct projection to GS screen coordinates: the screen centre is added in proportion to depth.
    sceVu0FMATRIX persp;
    sceVu0UnitMatrix(persp);
    persp[0][0] = this->projection;
    persp[1][1] = persp[0][0];
    persp[2][0] = 2048.0f;
    persp[2][1] = 2048.0f;
    persp[2][2] = -(z_range * near_dist - far_dist * 0.0f) / (far_dist - near_dist);
    persp[2][3] = 1.0f;
    persp[3][2] = depth_offset;
    persp[3][3] = 0.0f;
    sceVu0CopyMatrix(screen, persp);

    SetViewMatrix(view, camera_pos);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_drawenv", SetRenderInfo__13mgRENDER_INFOFfiiffif);
#endif

void mgRENDER_INFO::SetViewMatrix(float (*view)[4], float *camera_pos) {
    sceVu0CopyVector(this->camera_pos, camera_pos);
    this->camera_pos[3] = 1.0f;
    sceVu0TransposeMatrix(camera_pose, view);
    sceVu0CopyVector(camera_pose[3], this->camera_pos);
    camera_pose[0][3] = 0.0f;
    camera_pose[1][3] = 0.0f;
    camera_pose[2][3] = 0.0f;
    camera_pose[3][3] = 1.0f;

    sceVu0CopyMatrix(this->view, view);
    mgMulMatrix(world_view, aspect, view);
    mgMulMatrix(world_screen, screen, world_view);
    mgMulMatrix(view_screen, screen, aspect);

    // The screen projection without the screen centre offset.
    sceVu0FMATRIX screen_rel;
    sceVu0CopyMatrix(screen_rel, screen);
    screen_rel[2][1] = 0.0f;
    screen_rel[2][0] = 0.0f;
    mgMulMatrix(world_screen_rel, screen_rel, world_view);
    mgMulMatrix(world_clip, view_clip, this->view);
}

void mgRENDER_INFO::SetDropShadowMatrix(float *light_dir, float *plane_pos, float *plane_normal) {
    sceVu0CopyVector(shadow_plane_pos, plane_pos);
    sceVu0CopyVector(shadow_plane_normal, plane_normal);
    sceVu0CopyVector(shadow_light_dir, light_dir);
    mgShadowMatrix(shadow, light_dir, plane_pos, plane_normal);
}

int mgRENDER_INFO::ActiveLighting(int index, int copy) {
    int previous = active_light;
    if (index == previous) {
        return index;
    }
    if (index < 0 || index >= 8) {
        return previous;
    }
    light_changed = 1;
    previous = active_light;
    active_light = index;
    if (copy != 0) {
        light_info[active_light] = light_info[previous];
    }
    return previous;
}

mgLIGHT_INFO *mgRENDER_INFO::GetpLightInfo() {
    return &light_info[active_light];
}

void mgRENDER_INFO::InitActiveLighting() {
    light_changed = 1;
    memset(GetpLightInfo(), 0, sizeof(mgLIGHT_INFO));
}

void mgRENDER_INFO::InitLighting() {
    light_changed = 1;
    for (int i = 0; i < 8; i++) {
        memset(&light_info[i], 0, sizeof(mgLIGHT_INFO));
    }
}

void mgRENDER_INFO::SetLight(float (*light_dir)[4], float (*light_color)[4]) {
    light_changed = 1;
    mgLIGHT_INFO *info = GetpLightInfo();
    sceVu0CopyMatrix(info->light_dir, light_dir);
    sceVu0CopyMatrix(info->light_color, light_color);
    info->light_color[0][3] = 0.0f;
    info->light_color[1][3] = 0.0f;
    info->light_color[2][3] = 0.0f;
    info->light_color[3][3] = 0.0f;
    info->light_dir[3][0] = 0.0f;
    info->light_dir[3][1] = 0.0f;
    info->light_dir[3][2] = 0.0f;
    info->light_dir[3][3] = 0.0f;
}

void mgRENDER_INFO::GetLight(float (*light_dir)[4], float (*light_color)[4]) {
    mgLIGHT_INFO *info = GetpLightInfo();
    sceVu0CopyMatrix(light_dir, info->light_dir);
    sceVu0CopyMatrix(light_color, info->light_color);
}

void mgRENDER_INFO::SetLight(int index, float *dir, float *color) {
    light_changed = 1;
    if (index < 0 || index >= 4) {
        return;
    }
    mgLIGHT_INFO *info = GetpLightInfo();
    info->light_dir[0][index] = dir[0];
    info->light_dir[1][index] = dir[1];
    info->light_dir[2][index] = dir[2];
    info->light_dir[3][index] = 0.0f;
    info->light_color[index][0] = color[0];
    info->light_color[index][1] = color[1];
    info->light_color[index][2] = color[2];
    info->light_color[index][3] = 0.0f;
}

void mgRENDER_INFO::SetAmbient(float *color) {
    sceVu0CopyVector(GetpLightInfo()->ambient, color);
}

void mgRENDER_INFO::GetAmbient(float *color) {
    sceVu0CopyVector(color, GetpLightInfo()->ambient);
}

void mgRENDER_INFO::SetPlight(int index, float *pos, float *color, float power, float range) {
    mgPOINT_LIGHT light;
    sceVu0CopyVector(light.pos, pos);
    sceVu0CopyVector(light.color, color);
    light.power = power;
    light.range = range;
    SetPlight(index, &light);
}

#ifdef NONMATCHING
void mgRENDER_INFO::SetPlight(int index, mgPOINT_LIGHT *light) {
    if (index < 0 || index >= 4) {
        return;
    }
    mgLIGHT_INFO *info = GetpLightInfo();
    if (light == NULL) {
        info->point_light[index].power = 0.0f;
        return;
    }

    // Without a range of its own, the light reaches as far as its brightest colour component allows.
    float range = light->range;
    if (range <= 0.0f) {
        float brightest;
        if (light->color[0] > light->color[1]) {
            brightest = light->color[0] > light->color[2] ? light->color[0] : light->color[2];
        } else {
            brightest = light->color[1] > light->color[2] ? light->color[1] : light->color[2];
        }
        range = light->power * sqrtf(brightest);
    }

    mgPOINT_LIGHT *dest = &info->point_light[index];
    *dest = *light;
    dest->pos[3] = 1.0f;
    dest->range = range;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_drawenv", SetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT);
#endif

#ifdef NONMATCHING
void mgRENDER_INFO::GetPlight(int index, mgPOINT_LIGHT *light) {
    if (index >= 0 && index < 4) {
        if (light == NULL) {
            return;
        }
        *light = GetpLightInfo()->point_light[index];
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_drawenv", GetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT);
#endif

void mgRENDER_INFO::FogEnable(int enable) {
    fog_enable = enable;
}

int mgRENDER_INFO::GetFogEnable() {
    return fog_enable;
}

void mgRENDER_INFO::PlightEnable(int enable) {
    plight_enable = enable;
}

int mgRENDER_INFO::GetPlightEnable() {
    return plight_enable;
}

void mgRENDER_INFO::SetFogParam(float near_dist, float far_dist, u_char r, u_char g, u_char b,
                                float far_value, float near_value) {
    float value_range = far_value - near_value;
    float dist_range = far_dist - near_dist;
    fog.near_dist = near_dist;
    fog.far_dist = far_dist;
    fog.offset = (far_value + near_value + (value_range * (far_dist + near_dist)) / dist_range) / 2.0f;
    fog.far_value = far_value;
    fog.near_value = near_value;
    float scale = -far_dist * near_dist;
    scale *= value_range;
    fog.scale = scale / dist_range;
    fog.coef[0] = fog.offset;
    fog.coef[1] = fog.far_value;
    fog.coef[2] = fog.near_value;
    fog.coef[3] = fog.scale;
    fog.r = r;
    fog.g = g;
    fog.b = b;
}

mgVu0FBOX &mgVu0FBOX::operator=(mgVu0FBOX &other) {
    *(u_long128 *)max = *(u_long128 *)other.max;
    *(u_long128 *)min = *(u_long128 *)other.min;
    return *this;
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_drawenv", at_184__DATA);
