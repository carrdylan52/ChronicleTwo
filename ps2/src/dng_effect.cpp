#include "common.h"
#include "dng_effect.hpp"
#include "std/runtime.hpp"
#include "maintex.hpp"
#include "mglib.hpp"
#include "mg_math.hpp"
#include "mg_camera.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

/**
 * Copies between three-component positions and homogeneous vectors.
 */
static void trans_float_to_sceVector(float *sce, float *vec, int to_vec);
/**
 * Projects a rotated sprite into four screen-space corners.
 */
static int LocalTransWorldPrimPos(int (*corners)[4], float *pos, float width, float height, float angle);

// Trap on division by zero for variable integer divisors.
#pragma divbyzerocheck on

// Code (.text)

float trans_effect_rate(int rate) {
    float f = (float)rate / 255.0f;
    if (1.0f < f) {
        f = 1.0f;
    }
    return f;
}

static void trans_float_to_sceVector(float *sce, float *vec, int to_vec) {
    if (to_vec == 0) {
        sce[0] = vec[0];
        sce[1] = vec[1];
        sce[2] = vec[2];
        sce[3] = 1.0f;
    } else {
        vec[0] = sce[0];
        vec[1] = sce[1];
        vec[2] = sce[2];
    }
}

void CChillAfterHit::Initialize() {
    active = 0;
    piece_num = 0;
    rate = 0;
    memset(piece, 0, sizeof(piece));
    mgZeroVector(center);
}

void CChillAfterHit::SetPos(float *pos, float size, int strength) {
    CHILL_AFTER_HIT_PIECE *p;
    int i;
    float speed_boost;
    float launch_speed;
    if (strength < 32) {
        return;
    }
    Initialize();
    active = 1;
    p = piece;
    rate = trans_effect_rate(strength);
    if (45.0f < size) {
        size = 45.0f;
    }
    piece_num = CHILL_AFTER_HIT_PIECE_MAX;
    if (rate <= 0.5f) {
        piece_num = (int)(16.0f * (0.5f + rate)) + 8;
        launch_speed = 3.5f + 2.5f * rate;
        speed_boost = 4.4f * rate;
    } else {
        launch_speed = 4.5f + 3.0f * rate;
        speed_boost = 5.6f * rate;
    }
    if (piece_num > CHILL_AFTER_HIT_PIECE_MAX) {
        piece_num = CHILL_AFTER_HIT_PIECE_MAX;
    }
    *(u_long128 *)center = *(u_long128 *)pos;
    size = 8.0f + size / 13.5f + speed_boost;
    for (i = 0; i < CHILL_AFTER_HIT_PIECE_MAX; i++, p++) {
        p->velocity[0] = fRand(1.2f) - 0.6f;
        p->velocity[2] = fRand(1.2f) - 0.6f;
        p->velocity[1] = fRand(0.65f) - 0.15f;
        sceVu0ScaleVector(p->velocity, p->velocity, size);
        sceVu0AddVector(p->pos, pos, p->velocity);
        p->velocity[3] = 1.0f;
        p->damping = 0.76f * (0.85f + fRand(0.21f));
        p->size = launch_speed * (0.8f + fRand(0.3f * rate));
        p->alpha = iRand(60) + 72;
        p->fade_speed = iRand(8) + 9;
        p->rect = iRand(5);
        p->move_time = iRand(8) + 16;
        p->angle = fRand(6.2831855f) - 3.1415927f;
        p->spin = 3.1415927f / (9.0f + 2.0f * fRand(rate));
        if (iRand(10) % 2 != 0) {
            p->spin = -p->spin;
        }
        p->trail_num = -1;
    }
}

void CChillAfterHit::Step() {
    int i;
    int any_alive;
    CHILL_AFTER_HIT_PIECE *p;
    if (active != 0) {
        any_alive = 0;
        p = piece;
        i = 0;
        while (i < piece_num) {
            if (p->alpha > 0) {
                sceVu0ScaleVector(p->velocity, p->velocity, p->damping);
                p->velocity[3] = 1.0f;
                if (mgDistVector(p->velocity) < 0.7f) {
                    mgZeroVector(p->velocity);
                    p->move_time = 0;
                }
                if (p->trail_num <= 0) {
                    p->trail_num += 1;
                }
                p->trail[1][0] = p->trail[0][0];
                p->trail[1][1] = p->trail[0][1];
                p->trail[1][2] = p->trail[0][2];
                p->trail[0][0] = p->pos[0];
                p->trail[0][1] = p->pos[1];
                p->trail[0][2] = p->pos[2];
                if (0 < p->move_time) {
                    sceVu0AddVector(p->pos, p->pos, p->velocity);
                    p->move_time -= 1;
                    p->angle = mgAngleLimit(p->angle + p->spin);
                    p->spin *= 0.6f;
                } else {
                    p->alpha -= p->fade_speed;
                    p->pos[1] = p->pos[1] - 0.2f;
                }
                p->pos[3] = 1.0f;
                if (0 < p->alpha) {
                    any_alive = 1;
                }
            }
            i++;
            p++;
        }
        if (any_alive == 0) {
            active = 0;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", LocalTransWorldPrimPos__FPA4_iPffff);

void CChillAfterHit::Draw() {
    sceVu0FVECTOR vec;
    int sprite0[4];
    int sprite1[4];
    int j;
    int i;
    CHILL_AFTER_HIT_PIECE *p;
    int *rect;
    int alpha;
    float size;
    int side;
    static int chill_tex_rect[6][3] = {
        {0, 0, 0x20}, {0x20, 0, 0x20}, {0, 0x20, 0x20},
        {0x20, 0x20, 0x20}, {0x40, 0, 0x40}, {0, 0x40, 0x40}
    };
    if (TEX_ExFx_ICE != 0 && active != 0) {
        CPreSprite prim;
        int quad[4][4];
        int trail_quad[4][4];
        prim.Initialize(0, 0);
        prim.Preset2D();
        prim.Coord(1);
        prim.DepthTestEnable(1);
        prim.ZMask(-1);
        prim.Bilinear(1);
        prim.TextureMapEnable(1);
        p = piece;
        prim.AlphaBlend(2);
        i = 0;
        while (i < piece_num) {
            if (p->alpha > 0) {
                size = p->size;
                if (LocalTransWorldPrimPos(quad, p->pos, size, size, p->angle) != 0) {
                    prim.Begin(5);
                    prim.Texture(TEX_ExFx_ICE);
                    alpha = p->alpha;
                    j = 0;
                    rect = chill_tex_rect[p->rect];
                    while (j < p->trail_num) {
                        trans_float_to_sceVector(vec, p->trail[j], 0);
                        size = p->size;
                        LocalTransWorldPrimPos(trail_quad, vec, size, size, p->angle);
                        prim.Color(0x80, 0x80, 0xB6, alpha);
                        prim.TextureCrd(rect[0], rect[1]);
                        prim.Vertex4(trail_quad[0]);
                        prim.TextureCrd(rect[0] + rect[2], rect[1]);
                        prim.Vertex4(trail_quad[1]);
                        side = rect[2];
                        prim.TextureCrd(rect[0] + side, rect[1] + side);
                        prim.Vertex4(trail_quad[2]);
                        prim.TextureCrd(rect[0], rect[1] + rect[2]);
                        prim.Vertex4(trail_quad[3]);
                        prim.Flush();
                        alpha = alpha >> 1;
                        j++;
                    }
                    prim.Color(0x80, 0x80, 0xB6, p->alpha);
                    prim.TextureCrd(rect[0], rect[1]);
                    prim.Vertex4(quad[0]);
                    prim.TextureCrd(rect[0] + rect[2], rect[1]);
                    prim.Vertex4(quad[1]);
                    side = rect[2];
                    prim.TextureCrd(rect[0] + side, rect[1] + side);
                    prim.Vertex4(quad[2]);
                    prim.TextureCrd(rect[0], rect[1] + rect[2]);
                    prim.Vertex4(quad[3]);
                    prim.End();
                    prim.Begin(6);
                    sceVu0ScaleVector(vec, p->velocity, 0.05f);
                    sceVu0SubVector(vec, p->pos, vec);
                    vec[3] = 1.0f;
                    size = 1.2f * p->size;
                    mgTransWorldPrim3DSprite(sprite0, sprite1, vec, size, size, 0);
                    prim.Color(0x80, 0x80, 0x94, p->alpha);
                    prim.TextureCrd(0x40, 0x40);
                    prim.Vertex4(sprite0);
                    prim.TextureCrd(0x80, 0x40);
                    prim.Vertex4(sprite1);
                    prim.End();
                }
            }
            i++;
            p++;
        }
    }
}

void CFireAfterHit::Initialize(void) {
    active = 0;
    flame_num = 0;
    time = 0;
    rate = 0.0f;
    memset(flame, 0, sizeof(flame));
    memset(trail, 0, sizeof(trail));
}

void CFireAfterHit::SetPos(float *pos, float size, int strength) {
    FIRE_AFTER_HIT_FLAME *e;
    FIRE_AFTER_HIT_TRAIL(*trails)[FIRE_AFTER_HIT_TRAIL_MAX];
    int i;
    if (strength < 32) {
        return;
    }
    Initialize();
    rate = trans_effect_rate(strength);
    if (45.0f < size) {
        size = 45.0f;
    }
    flame_num = (int)(4.0f + 10.0f * rate);
    if (flame_num > FIRE_AFTER_HIT_FLAME_MAX) {
        flame_num = FIRE_AFTER_HIT_FLAME_MAX;
    }
    e = flame;
    trails = trail;
    size = 8.0f + size / 14.0f;
    active = 1;
    i = 0;
    while (i < flame_num) {
        e->velocity[0] = fRand(1.0f) - 0.5f;
        e->velocity[2] = fRand(1.0f) - 0.5f;
        e->velocity[1] = 0.3f + fRand(0.3f);
        e->size = 1.0f;
        sceVu0ScaleVector(e->velocity, e->velocity, size);
        sceVu0AddVector(e->pos, pos, e->velocity);
        e->size = 7.5f * (0.8f + fRand(rate));
        e->alpha = iRand(40) + 97;
        e->age = (int)((1.3f - rate) * fRand(4.0f));

        trans_float_to_sceVector(e->pos, trails[i][0].pos, 1);
        e->trail_head += 1;
        e->fade_speed = iRand(10) + 4;
        e->delay = iRand(3);
        i++;
        e++;
    }
}

void CFireAfterHit::Step() {
    int i;
    int j;
    int any_alive;
    FIRE_AFTER_HIT_FLAME *e;
    FIRE_AFTER_HIT_TRAIL *row;
    int spawned;
    if (active != 0) {
        any_alive = 0;
        e = flame;
        i = 0;
        while (i < flame_num) {
            if (0 < e->delay) {
                e->delay -= 1;
            } else {
                row = trail[i];
                spawned = 0;
                for (j = 0; j < FIRE_AFTER_HIT_TRAIL_MAX; j++, row++) {
                    if (j == e->trail_head && spawned == 0 && e->age < 13) {
                        trans_float_to_sceVector(e->pos, row->pos, 1);
                        row->alpha = (int)(0.5f * (128.0f - (float)e->alpha)) + 32;
                        if (row->alpha < 32) {
                            row->alpha = 32;
                        }
                        if (row->alpha >= 128) {
                            row->alpha = 128;
                        }
                        row->size = 1.35f * e->size;
                        e->trail_head += 1;
                        spawned = 1;
                        if (e->trail_head >= FIRE_AFTER_HIT_TRAIL_MAX) {
                            e->trail_head = 0;
                        }
                    } else {
                        row->size += 0.2f;
                        row->alpha -= 3;
                        if (row->alpha <= 0) {
                            row->alpha = 0;
                        }
                    }
                    if (0 < row->alpha) {
                        any_alive = 1;
                    }
                }
                if (0 < e->alpha) {
                    any_alive = 1;
                    e->alpha -= e->fade_speed;
                }
                e->velocity[0] *= 0.94f;
                e->velocity[2] *= 0.94f;
                e->velocity[1] -= 0.5f;
                if (e->velocity[1] < -4.5f) {
                    e->velocity[1] = -4.5f;
                }
                e->size *= 0.94f;
                sceVu0AddVector(e->pos, e->pos, e->velocity);
                e->pos[3] = 1.0f;
                e->age += 1;
            }
            i++;
            e++;
        }
        time += 1;
        if (any_alive == 0) {
            active = 0;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw__13CFireAfterHitFv);

void CTornado::SetPos(float *pos, float size, float strength) {
    TORNADO_PIECE *p;
    int i;
    if (strength < 32.0f) {
        return;
    }
    active = 1;
    live_num = TORNADO_PIECE_MAX;
    pos[3] = 1.0f;
    sceVu0CopyVector(center, pos);
    rate = strength / 255.0f;
    live_num = (int)(14.0f * rate) + 4;
    printf("br = %.2f\n", (double)rate);
    p = piece;
    for (i = 0; i < TORNADO_PIECE_MAX; i++) {
        p[i].life = 0;
    }
    for (i = 0; i < live_num; i++) {
        sceVu0CopyVector(p->pos, center);
        p->pos[1] += fRand(0.5f * size);
        p->life = iRand(15) + 15;
        p->angle = fRand(6.2831855f) - 3.1415927f;
        p->alpha = 1.0f;
        p->scale = 0.1f * size;
        p->rise = 0.05f * size;
        p++;
    }
}

void CTornado::Draw() {
    TORNADO_PIECE *p;
    int i;
    if (active != 0 && model != 0) {
        mgCFrameAttr attr;

        attr.no_light = 1;
        attr.z_write = -1;
        attr.alpha_blend = 2;
        p = piece;
        for (i = 0; i < TORNADO_PIECE_MAX; i++) {
            if (p->life <= 0) {
                p++;
                continue;
            }
            model->SetPosition(p->pos);
            float size = p->scale;
            model->SetScale(size, 0.25f + 4.0f * rate, size);
            model->SetRotation(0.0f, p->angle, 0.0f);
            attr.color[0] = 180.0f;
            attr.color[1] = 180.0f;
            attr.color[2] = 180.0f;
            attr.color[3] = 250.0f * p->alpha;
            model->SetAttrParam(attr, 1, 0x10000);
            mgDrawDirect(model);
            p++;
        }
    }
}

void CTornado::Step() {
    TORNADO_PIECE *p;
    int i;
    if (active != 0) {
        p = piece;
        for (i = 0; i < TORNADO_PIECE_MAX; i++) {
            if (p->life <= 0) {
                p++;
                continue;
            }
            p->pos[1] += p->rise;
            p->angle += 0.19634955f;
            p->angle = mgAngleLimit(p->angle);
            if (p->life < 10) {
                p->scale += 0.2f;
                p->alpha = p->alpha - 0.1f;
                if (p->alpha <= 0.0f) {
                    p->alpha = 0.0f;
                }
            }
            p->life = p->life - 1;
            if (p->life <= 0) {
                live_num -= 1;
            }
            p++;
        }
        if (live_num <= 0) {
            active = 0;
        }
    }
}

void CTornado::Initialize() {
    TORNADO_PIECE *p;
    int i;
    active = 0;
    live_num = 0;
    p = piece;
    for (i = 0; i < TORNADO_PIECE_MAX; i++) {
        p->life = 0;
        p++;
    }
}

void CThunder::SetPos(float *pos, float size, float strength) {
    if (strength < 32.0f) {
        return;
    }
    active = 1;
    live_num = THUNDER_SPARK_MAX;
    float span = 2.0f * size;
    frame.SetPosition(pos);
    rate = strength / 255.0f;
    live_num = (int)(32.0f * rate) + 8;
    THUNDER_SPARK *bolt = spark;

    for (int i = 0; i < THUNDER_SPARK_MAX; i += 8) {
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
        bolt->life = 0.0f;
    }
    for (int i = 0; i < live_num; i++) {
        bolt->pos[0] = fRand(span) - span / 2.0f;
        bolt->pos[1] = fRand(span / 3.0f);
        bolt->pos[2] = fRand(span) - span / 2.0f;
        bolt->pos[3] = 1.0f;
        bolt->velocity[0] = fRand(1.0f) - 0.5f;
        bolt->velocity[1] = fRand(1.0f) - 0.5f;
        bolt->velocity[2] = fRand(1.0f) - 0.5f;
        bolt->velocity[3] = 0.0f;
        bolt->life = (float)(iRand(5) + 20);
        bolt->spin = fRand(0.3926991f);
        bolt->angle = fRand(3.1415927f);
        bolt->frame = iRand(6);
        bolt->scale = 1.0f;
        bolt++;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw__8CThunderFv);

void CThunder::Step(void) {
    if (active != 0) {
        THUNDER_SPARK *bolt = spark;
        if (live_num > 0) {
            for (int i = 0; i < THUNDER_SPARK_MAX; i++) {
                if (bolt->life <= 0.0f) {
                    bolt++;
                    continue;
                }
                sceVu0AddVector(bolt->pos, bolt->pos, bolt->velocity);
                bolt->frame = iRand(6);
                bolt->angle += bolt->spin;
                if (bolt->life < 10.0f) {
                    bolt->scale -= 0.1f;
                    if (bolt->scale < 0.0f) {
                        bolt->scale = 0.0f;
                    }
                }
                bolt->life -= 1.0f;
                if (bolt->life <= 0.0f) {
                    live_num--;
                }
                bolt++;
            }
            if (live_num <= 0) {
                active = 0;
            }
        }
    }
}

void CThunder::Initialize(void) {
    active = 0;
    live_num = 0;
    frame.attr = &attr;
}

void CSparcEffect::Draw(void) {
    if (state != SPARC_EFFECT_OFF && model[0] != 0) {
        mgCFrameAttr attr;
        float sx = 2.0f;
        float sz = 1.0f;

        attr.no_light = 1;
        switch (color) {
            case 0:
                attr.color[0] = 255.0f;
                attr.color[1] = 64.0f;
                attr.color[2] = 0.0f;
                attr.color[3] = alpha;
                break;
            case 1:
                attr.color[0] = 160.0f;
                attr.color[1] = 220.0f;
                attr.color[2] = 250.0f;
                attr.color[3] = alpha;
                break;
            case 2:
                attr.color[0] = 200.0f;
                attr.color[1] = 160.0f;
                attr.color[2] = 250.0f;
                attr.color[3] = alpha;
                break;
            case 3:
                attr.color[0] = 128.0f;
                attr.color[1] = 255.0f;
                attr.color[2] = 128.0f;
                attr.color[3] = alpha;
                break;
        }
        model[0]->SetAttrParam(attr, 1, MG_FRAME_ATTR_COLOR);
        model[1]->SetAttrParam(attr, 1, MG_FRAME_ATTR_COLOR);
        model[2]->SetAttrParam(attr, 1, MG_FRAME_ATTR_COLOR);
        model[0]->SetPosition(pos);
        model[0]->SetScale(sx, sx, sz);
        model[1]->SetPosition(pos);
        model[1]->SetScale(sx, sx, sz);
        model[2]->SetPosition(pos);
        model[2]->SetScale(sx, sx, sz);
        switch (pattern) {
            case 0:
                mgDrawDirect(model[0]);
                mgDrawDirect(model[1]);
                break;
            case 1:
                mgDrawDirect(model[1]);
                mgDrawDirect(model[2]);
                break;
            case 2:
                mgDrawDirect(model[0]);
                mgDrawDirect(model[2]);
                break;
        }
    }
}


void CSparcEffect::Step(void) {
    if (state != SPARC_EFFECT_OFF) {
        switch ((s64)state) {
            case SPARC_EFFECT_FADE_IN:
                alpha += alpha_max / 4.0f;
                if (alpha >= alpha_max) {
                    state = SPARC_EFFECT_FADE_OUT;
                }
                break;
            case SPARC_EFFECT_FADE_OUT:
                alpha -= alpha_max / 8.0f;
                if (alpha <= 0.0f) {
                    alpha = 0.0f;
                    state = SPARC_EFFECT_OFF;
                }
                break;
        }
    }
}

void CSparcEffect::Initialize(void) {
    state = 0;
}

void CMiniEffPrim::SetPrim(float *pos, int color) {
    sceVu0CopyVector(this->pos, pos);
    this->state = MINI_EFF_PRIM_RISING;
    this->alpha = 1.0f;
    this->size = 1.0f + fRand(2.0f);
    this->color = color;
}

void CMiniEffPrim::Draw(CPreSprite *prim) {
    int corner0[4];
    int corner1[4];
    int u0;
    int v0;
    int u1;
    int v1;
    if (state != MINI_EFF_PRIM_FREE && prim != 0) {
        if (color == 0) {
            prim->Color(0x80, 0xB4, 0x80, (int)(128.0f * alpha));
            u0 = 0x20;
            v0 = 0x40;
            u1 = 0x2F;
            v1 = 0x4F;
        }
        if (color == 1) {
            prim->Color(0xFA, 0xDC, 0x40, (int)(128.0f * alpha));
            u0 = 0x20;
            v0 = 0x40;
            u1 = 0x2F;
            v1 = 0x4F;
        }
        if (mgTransWorldPrim3DSprite(corner0, corner1, pos, size, size, 0) != 0) {
            prim->TextureCrd(u0, v0);
            prim->Vertex4(corner0);
            prim->TextureCrd(u1, v1);
            prim->Vertex4(corner1);
        }
    }
}

int CMiniEffPrim::Step(void) {
    if (state == MINI_EFF_PRIM_FREE) {
        return 0;
    }
    if (state == MINI_EFF_PRIM_RISING) {
        alpha -= 0.0625f;
        pos[1] -= 0.5f;
        if (alpha <= 0.0f) {
            state = MINI_EFF_PRIM_FREE;
            return 1;
        }
    }
    return 0;
}

void CMiniEffPrim::Initialize(void) {
    state = 0;
}

void CMiniEffPrimMan::CreatPrim(float *pos, int color) {
    for (int i = 0; i < MINI_EFF_PRIM_MAX; i++) {
        if (prim[i].state == MINI_EFF_PRIM_FREE) {
            prim[i].SetPrim(pos, color);
            active_num++;
            return;
        }
    }
}

void CMiniEffPrimMan::Draw(void) {
    if (active_num > 0) {
        draw_prim.Initialize(0, 0);
        draw_prim.Preset2D();
        draw_prim.Coord(1);
        draw_prim.DepthTestEnable(1);
        draw_prim.ZMask(-1);
        draw_prim.Bilinear(1);
        draw_prim.TextureMapEnable(1);
        draw_prim.AlphaBlend(2);
        draw_prim.Begin(6);
        draw_prim.Texture(TEX_SystemEffect1);
        for (int i = 0; i < MINI_EFF_PRIM_MAX; i++) {
            prim[i].Draw(&draw_prim);
        }
        draw_prim.End();
    }
}

void CMiniEffPrimMan::Step(void) {
    if (active_num > 0) {
        for (int i = 0; i < MINI_EFF_PRIM_MAX; i++) {
            if (prim[i].Step() != 0) {
                active_num--;
            }
        }
    }
}

void CMiniEffPrimMan::Initialize(void) {
    for (int i = 0; i < MINI_EFF_PRIM_MAX; i++) {
        prim[i].Initialize();
    }
    active_num = 0;
}

void CPalletAnime::SetAnim(s16 red, s16 green, s16 blue, s16 pulse_num, s16 duration, s16 repeats) {
    this->red = red;
    this->green = green;
    this->blue = blue;
    this->pulse_num = pulse_num;
    this->duration = duration;
    elapsed = 0;
    this->repeats = repeats;
}

int CPalletAnime::CreatPallet(float *out, float *base) {
    if (duration <= 0) {
        return 0;
    }
    int period = duration / this->pulse_num;
    if (period <= 0) {
        return 0;
    }
    float ratio = (float)(elapsed % period);
    ratio = ratio / (float)period;
    float blend = sinf(3.1415927f * ratio);
    out[0] = base[0] + blend * ((float)red - base[0]);
    out[1] = base[1] + blend * ((float)green - base[1]);
    out[2] = base[2] + blend * ((float)blue - base[2]);
    out[3] = 128.0f;
    return 1;
}

void CPalletAnime::Step(void) {
    s16 repeat_left;
    if (duration > 0) {
        elapsed++;
        if (elapsed < duration) {
            return;
        }
        repeat_left = repeats;
        if (repeat_left == -1) {
            elapsed = 0;
            return;
        }
        if (repeat_left > 0) {
            repeats = repeat_left - 1;
            elapsed = 0;
            return;
        }
        duration = 0;
    }
}

void CPalletAnime::Initialize(void) {
    duration = 0;
    elapsed = 0;
}

void CHealingEffectMan::Draw(mgCCamera *camera) {
    sceVu0FVECTOR camera_pos;
    int corner0[4];
    int corner1[4];
    float identity[4][4];
    float rotation[4][4];
    int glow_alpha;
    int halo_alpha;
    if (active != 0) {
        camera->GetPos(camera_pos);
        sceVu0UnitMatrix(identity);
        switch (mode) {
            case HEALING_EFFECT_IDLE:
                glow_alpha = 0x80;
                halo_alpha = 0x40;
                break;
            case HEALING_EFFECT_OFF:
                halo_alpha = 0;
                glow_alpha = 0;
                break;
            case HEALING_EFFECT_SPENT: {
                float fade_in = 1.0f - (brightness - 1.0f);
                glow_alpha = (int)(255.0f * fade_in);
                halo_alpha = (int)(180.0f * fade_in);
                break;
            }
            case HEALING_EFFECT_RECHARGE: {
                float fade_out = brightness;
                glow_alpha = (int)(128.0f * fade_out);
                halo_alpha = (int)(64.0f * fade_out);
                break;
            }
        }
        float distance = mgDistVector(camera_pos, center);
        if (distance <= 640.0f) {
            float distance_fade = 1.0f;
            if (distance > 320.0f) {
                distance_fade = 1.0f - (distance - 320.0f) / 320.0f;
            }
            glow_alpha = (int)((float)glow_alpha * distance_fade);
            halo_alpha = (int)((float)halo_alpha * distance_fade);

            CPreSprite prim;
            prim.Initialize(0, 0);
            prim.Preset2D();
            prim.Coord(1);
            prim.DepthTestEnable(1);
            prim.ZMask(-1);
            prim.Bilinear(1);
            prim.TextureMapEnable(1);
            prim.AlphaBlend(2);
            prim.Begin(6);
            prim.Texture(TEX_SystemEffect1);
            HEALING_LIGHT *particle = light;
            for (int i = 0; i < HEALING_LIGHT_MAX; i++, particle++) {
                particle->pos[0] = 0.0f;
                particle->pos[1] = particle->bob_height * sinf(particle->bob_phase);
                particle->pos[2] = particle->radius;
                sceVu0RotMatrixY(rotation, identity, particle->angle);
                sceVu0ApplyMatrix(particle->pos, rotation, particle->pos);
                sceVu0AddVector(particle->pos, particle->pos, center);
                particle->pos[1] += 8.0f;
                particle->pos[3] = 1.0f;
                float glow_size = 3.0f * brightness;
                if (mgTransWorldPrim3DSprite(corner0, corner1, particle->pos, glow_size, glow_size,
                                             0) != 0) {
                    prim.Color(0x5A, 0x80, 0x40, glow_alpha);
                    prim.TextureCrd(0, 0x20);
                    prim.Vertex4(corner0);
                    prim.TextureCrd(0x1F, 0x3F);
                    prim.Vertex4(corner1);
                }
                float halo_size = 30.0f * brightness;
                if (mgTransWorldPrim3DSprite(corner0, corner1, particle->pos, halo_size, halo_size,
                                             0) != 0) {
                    prim.Color(0x5A, 0x80, 0x5A, halo_alpha);
                    prim.TextureCrd(0x40, 0x40);
                    prim.Vertex4(corner0);
                    prim.TextureCrd(0x7F, 0x7F);
                    prim.Vertex4(corner1);
                }
            }
            prim.End();
        }
    }
}

void CHealingEffectMan::Step(void) {
    if (active != 0) {
        switch (mode) {
            case HEALING_EFFECT_SPENT:
                brightness += 0.033333335f;
                if (brightness >= 2.0f) {
                    mode = HEALING_EFFECT_OFF;
                    brightness = 0.0f;
                }
                break;
            case HEALING_EFFECT_RECHARGE:
                brightness += 0.033333335f;
                if (brightness >= 1.0f) {
                    mode = HEALING_EFFECT_IDLE;
                    brightness = 1.0f;
                }
                break;
        }
        HEALING_LIGHT *particle = light;
        for (int i = 0; i < HEALING_LIGHT_MAX; i++, particle++) {
            particle->angle += particle->spin;
            if (particle->angle > 3.1415927f) {
                particle->angle -= 6.2831855f;
            }
            if (particle->angle < -3.1415927f) {
                particle->angle += 6.2831855f;
            }
            particle->bob_phase += particle->bob_speed;
            if (particle->bob_phase > 3.1415927f) {
                particle->bob_phase -= 6.2831855f;
            }
        }
    }
}

void CHealingEffectMan::SetMode(s32 mode) {
    this->mode = mode;
}

void CHealingEffectMan::Set(float *pos) {
    sceVu0CopyVector(this->center, pos);
    this->active = 1;
    this->mode = HEALING_EFFECT_IDLE;
    this->brightness = 1.0f;
}

void CHealingEffectMan::Initialize(void) {
    active = 0;
    mode = HEALING_EFFECT_IDLE;
    HEALING_LIGHT *particle = light;
    for (int i = 0; i < HEALING_LIGHT_MAX; i++, particle++) {
        particle->radius = fRand(80.0f) - 40.0f;
        if (particle->radius < 0.0f) {
            particle->radius -= 20.0f;
        } else {
            particle->radius += 20.0f;
        }
        particle->angle = fRand(3.1415927f);
        particle->bob_height = fRand(8.0f);
        particle->bob_phase = fRand(3.1415927f / 32.0f);
        particle->bob_speed = fRand(3.1415927f / 32.0f);
        particle->spin = fRand(3.1415927f / 48.0f) - 3.1415927f / 96.0f;
    }
}

void CSwordLuminous::Draw(void) {
    sceVu0FVECTOR hilt_pos;
    sceVu0FVECTOR tip_pos;
    sceVu0FVECTOR step;
    if (mode == SWORD_LUMINOUS_OFF || tip_frame == 0 || root_frame == 0) {
        return;
    }
    tip_frame->GetWorldPosition0(hilt_pos);
    root_frame->GetWorldPosition0(tip_pos);
    sceVu0SubVector(step, hilt_pos, tip_pos);
    float length = mgDistVector(step);
    sceVu0Normalize(step, step);
    sceVu0ScaleVector(step, step, length / 16.0f);

    CPreSprite prim;
    int corner0[4];
    int corner1[4];
    prim.Initialize(0, 0);
    prim.Preset2D();
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.ZMask(-1);
    prim.Bilinear(1);
    prim.TextureMapEnable(1);
    prim.AlphaBlend(2);
    prim.Begin(6);
    prim.Texture(TEX_SystemEffect1);
    prim.Color(0x80, 0x80, 0xFF, 0x18);
    float size = 12.0f + 4.0f * sinf(pulse);
    int i;
    for (i = 0; i < 0x10; i++) {
        if (mgTransWorldPrim3DSprite(corner0, corner1, tip_pos, size, size, 0) != 0) {
            prim.TextureCrd(0x80, 0x40);
            prim.Vertex4(corner0);
            prim.TextureCrd(0xA0, 0x60);
            prim.Vertex4(corner1);
        }
        sceVu0AddVector(tip_pos, tip_pos, step);
    }
    prim.End();
}

void CSwordLuminous::Step(void) {
    if (mode == SWORD_LUMINOUS_OFF) {
        return;
    }
    switch (mode) {
        case SWORD_LUMINOUS_FADE_IN:
            fade += 0.0625f;
            if (fade > 1.0f) {
                fade = 1.0f;
            }
            break;
        case SWORD_LUMINOUS_ON:
            break;
        case SWORD_LUMINOUS_FADE_OUT:
            fade -= 0.03125f;
            if (fade <= 0.0f) {
                mode = SWORD_LUMINOUS_OFF;
            }
            break;
    }
    pulse += 0.10471976f;
    if (pulse > 3.1415927f) {
        pulse -= 3.1415927f;
    }
}

void CSWordAfterImage::Draw(void) {
    int screen[4];
    if (smooth_num > 0) {
        CPreSprite prim;
        sceVu0FVECTOR edge;
        sceVu0FVECTOR edge_end;
        prim.Initialize(0, 0);
        prim.Preset2D();
        prim.TextureMapEnable(0);
        prim.Coord(1);
        prim.Shading(1);
        prim.DepthTestEnable(1);
        prim.DepthTest(1);
        prim.AlphaBlend(2);
        prim.Begin(4);
        for (int i = 0; i < smooth_num; i++) {
            sceVu0SubVector(edge, smooth_back[i], smooth_edge[i]);
            sceVu0ScaleVector(edge, edge, 0.7f);
            sceVu0AddVector(edge_end, smooth_edge[i], edge);
            edge_end[3] = 1.0f;
            float alpha = smooth_life[i];
            if (alpha < 0.0f) {
                alpha = 0.0f;
            }
            if (mgTransWorldPrim(screen, smooth_edge[i]) != 0) {
                prim.Color(edge_color[0], edge_color[1], edge_color[2], (int)((float)edge_color[3] * alpha));
                prim.Vertex4(screen);
            }
            if (mgTransWorldPrim(screen, edge_end) != 0) {
                prim.Color(back_color[0], back_color[1], back_color[2], (int)((float)back_color[3] * alpha));
                prim.Vertex4(screen);
            }
        }
        prim.End();
    }
}

void CSWordAfterImage::CreatPointList(void) {
    if (point_num <= 0) {
        return;
    }
    smooth_num = CreatSmoothPass(smooth_edge, edge_point, point_num, division, head_index, point_max);
    CreatSmoothPass(smooth_back, back_point, point_num, division, head_index, point_max);
    if (smooth_num == 0) {
        return;
    }
    int out = 0;
    for (int i = 0; i < point_num - 1; i++) {
        int from = i + head_index;
        int to = i + head_index + 1;
        if (from >= point_max) {
            from -= point_max;
        }
        if (from < 0) {
            from += point_max;
        }
        if (to >= point_max) {
            to -= point_max;
        }
        if (to < 0) {
            to += point_max;
        }
        float fade_from = life[from];
        float fade_to = life[to];
        smooth_life[out] = fade_from;
        (&smooth_life[out])[division - 1] = fade_to;
        for (int j = 1; j < division; j++) {
            (&smooth_life[out])[j] = fade_from + (float)j * ((fade_to - fade_from) / (float)division);
        }
        out += division;
    }
}

void CSWordAfterImage::AddPoint(float *edge, float *back, float life) {
    sceVu0CopyVector(this->edge_point[this->write_index], edge);
    sceVu0CopyVector(this->back_point[this->write_index], back);
    this->life[this->write_index] = life;
    this->head_index = this->write_index;
    if (this->point_num < this->point_max) {
        this->point_num++;
    }
    this->write_index--;
    if (this->write_index < 0) {
        this->write_index = this->point_max - 1;
    }
    this->active = 1;
}

void CSWordAfterImage::Step(void) {
    if (active != 0) {
        int index = head_index;
        for (int i = 0; i < point_num; i++) {
            life[index] -= 0.1f;
            if (life[index] <= 0.0f) {
                point_num--;
            }
            index++;
            if (index >= point_max) {
                index -= point_max;
            }
            if (index < 0) {
                index += point_max;
            }
        }
        if (point_num <= 0) {
            active = 0;
        }
    }
}

// Keep the point and life allocations independent.
#pragma opt_propagation off
void CSWordAfterImage::Initialize(mgCMemory *memory, int point_max, int division) {
    int smooth_bytes;
    int smooth_capacity;
    int point_bytes;
    point_bytes = point_max << 4;
    smooth_capacity = point_max * (division + 2);
    smooth_bytes = smooth_capacity << 4;
    edge_point = (sceVu0FVECTOR *)memory->Alloc(point_bytes / 16 + 1);
    back_point = (sceVu0FVECTOR *)memory->Alloc(point_bytes / 16 + 1);
    int smooth_blocks = smooth_bytes / 16 + 1;
    smooth_edge = (sceVu0FVECTOR *)memory->Alloc(smooth_blocks);
    smooth_back = (sceVu0FVECTOR *)memory->Alloc(smooth_blocks);
    life = (float *)memory->Alloc(point_max * 4 / 16 + 1);
    smooth_life = (float *)memory->Alloc(smooth_capacity * 4 / 16 + 1);
    edge_color[0] = 96;
    edge_color[1] = 64;
    edge_color[2] = 48;
    edge_color[3] = 180;
    back_color[0] = 64;
    back_color[1] = 48;
    back_color[2] = 32;
    back_color[3] = 96;
    this->point_max = point_max;
    this->division = division;
    smooth_num = 0;
    point_num = 0;
    write_index = point_max - 1;
    head_index = point_max - 1;
    active = 0;
}

#pragma opt_propagation reset

void CAfterWire::SetMode(s32 mode) {
    this->mode = mode;
    write_index = 0;
    newest = 0;
    point_num = 0;
    oldest = 0;
}

void CAfterWire::SetPos(float *pos) {
    sceVu0CopyVector(point[write_index], pos);
    newest = write_index;
    write_index++;
    if (write_index > AFTER_WIRE_POINT_MAX - 1) {
        write_index = 0;
    }
    oldest = newest + 1;
    if (oldest > AFTER_WIRE_POINT_MAX) {
        oldest = 0;
    }
    if (point_num < AFTER_WIRE_POINT_MAX) {
        oldest = 0;
    }
    if (point_num < AFTER_WIRE_POINT_MAX) {
        point_num++;
    }
}

void CAfterWire::DrawWire(sceVu0FVECTOR *work) {
    int vertex[4];
    if (mode != 0 && point_num >= 2) {
        smooth_num = CreatSmoothPass(work, point, point_num, 4, oldest, AFTER_WIRE_POINT_MAX);

        CPreSprite prim;
        prim.Initialize(0, 0);
        prim.Preset2D();
        prim.TextureMapEnable(0);
        prim.Coord(1);
        prim.Shading(1);
        prim.DepthTestEnable(1);
        prim.DepthTest(1);
        prim.AlphaBlend(2);
        prim.Begin(2);
        float alpha = 0.0f;
        for (int i = 0; i < smooth_num; work++, i++) {
            if (mgTransWorldPrim(vertex, *work) != 0) {
                prim.Color(0x50, 0x50, 0xFF, (int)(128.0f * alpha));
                prim.Vertex4(vertex);
            }
            alpha += 1.0f / smooth_num;
        }
        prim.End();
    }
}

void CAfterWire::StepWire() {
    if (mode != 0 && point_num < 2) {
        return;
    }
}

void CHitEffectImage::SethitEffect(float *pos, float *direction, float spread, float distance,
                                 float slow, float gravity, int life, int spark_num) {
    sceVu0FVECTOR center;
    int num = spark_num;
    sceVu0CopyVector(origin, pos);
    sceVu0CopyVector(this->direction, direction);
    sceVu0Normalize(this->direction, this->direction);
    if (spark_max < num) {
        num = spark_max;
    }
    this->spread = spread;
    this->distance = distance;
    this->slow = slow;
    this->gravity = gravity;
    live_num = num;
    sceVu0ScaleVectorXYZ(center, this->direction, distance);
    center[0] += origin[0];
    center[1] += origin[1];
    center[2] += origin[2];
    this->spark_num = num;
    BattleEffectPrim *spark = this->spark;
    int i = 0;
    if (0 < num) {
        do {
            float x = (center[0] + 2.0f * (spread * mgRnd())) - spread;
            float y = (center[1] + 2.0f * (spread * mgRnd())) - spread;
            float z = (center[2] + 2.0f * (spread * mgRnd())) - spread;
            spark->velocity[0] = x - origin[0];
            spark->velocity[1] = y - origin[1];
            spark->velocity[2] = z - origin[2];
            spark->velocity[3] = 1.0f;
            sceVu0Normalize(spark->velocity, spark->velocity);
            spark->pos[0] = (origin[0] + 3.0f * mgRnd()) - 3.0f;
            spark->pos[1] = (origin[1] + 3.0f * mgRnd()) - 3.0f;
            spark->pos[2] = (origin[2] + 3.0f * mgRnd()) - 3.0f;
            spark->pos[3] = 1.0f;
            spark->life = life + (int)((float)life * mgRnd()) - life / 2;
            spark->alpha = 1.0f;
            spark->alpha_step = 1.0f / (float)spark->life;
            spark->speed = distance / (float)spark->life;
            spark->rate = slow * (0.5f * mgRnd());
            spark->size = 32.0f + 200.0f * mgRnd();
            spark++;
            i++;
        } while (i < num);
    }
    tex_rect.left = 0;
    tex_rect.top = 0;
    tex_rect.right = 0x1F;
    tex_rect.bottom = 0x1F;
    sprite_size = 5.0f;
}

void CHitEffectImage::Step(void) {
    if (live_num > 0) {
        BattleEffectPrim *spark = this->spark;
        for (int i = 0; i < spark_num; i++) {
            if (spark->life > 0) {
                sceVu0FVECTOR move;
                sceVu0ScaleVectorXYZ(move, spark->velocity, spark->speed);
                spark->pos[0] += move[0];
                spark->pos[1] += move[1];
                spark->pos[2] += move[2];
                spark->velocity[1] -= gravity;
                if (spark->speed > 0.0f) {
                    spark->speed -= spark->rate;
                }
                spark->life--;
                spark->alpha -= spark->alpha_step;
                if (spark->life <= 0) {
                    live_num--;
                }
                spark++;
            }
        }
    }
}

void CHitEffectImage::Draw(void) {
    if (live_num > 0) {
        switch (kind) {
        case HIT_EFFECT_BOARD:
            this->DrawBord();
            return;
        case HIT_EFFECT_SPARK_SHORT:
            this->DrawSpark(3.0f);
            return;
        case HIT_EFFECT_SPARK_LONG:
            this->DrawSpark(9.0f);
            break;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", DrawBord__15CHitEffectImageFv);

void CHitEffectImage::DrawSpark(float length) {
    CPreSprite prim;
    int tail_screen[4];
    int tip_screen[4];
    sceVu0FVECTOR tip;

    prim.Initialize(0, 0);
    prim.Preset2D();
    prim.TextureMapEnable(0);
    prim.Coord(1);
    prim.Shading(1);
    prim.DepthTestEnable(1);
    prim.DepthTest(1);
    prim.AlphaBlend(2);
    prim.Begin(1);

    BattleEffectPrim *spark = this->spark;
    for (int i = 0; i < spark_num; i++) {
        if (spark->life > 0) {
            prim.Color(0x80, 0x80, 0x80, 0x80);
            tip[0] = spark->pos[0] + spark->velocity[0] * length;
            tip[1] = spark->pos[1] + spark->velocity[1] * length;
            tip[2] = spark->pos[2] + spark->velocity[2] * length;
            tip[3] = 1.0f;
            int visible = mgTransWorldPrim(tail_screen, spark->pos);
            visible += mgTransWorldPrim(tip_screen, tip);
            if (visible == 2) {
                prim.Color(0x20, 0x20, 0, 0x20);
                prim.Vertex4(tail_screen);
                prim.Color(0xFF, 0xFF, 0x80, 0xFF);
                prim.Vertex4(tip_screen);
            }
            spark++;
        }
    }
    prim.End();
}

void CFlushEffect::Draw(void) {
    if (active != 0) {
        CPreSprite prim;
        int corner0[4];
        int corner_b_r[4];
        int corner_t_l[4];
        int corner1[4];
        prim.Initialize(0, 0);
        prim.Preset2D();
        prim.DepthTestEnable(1);
        prim.DepthTest(1);
        prim.Bilinear(1);
        prim.Coord(1);
        prim.AlphaBlend(2);
        prim.Begin(3);
        prim.Texture(TEX_SystemEffect2);
        prim.AlphaTestEnable(1);
        if (mgTransWorldPrim3DSprite(corner0, corner1, pos, size, size, 0) != 0) {
            corner_b_r[0] = corner1[0];
            corner_b_r[1] = corner0[1];
            corner_b_r[2] = corner0[2];
            corner_b_r[3] = corner0[3];
            corner_t_l[0] = corner0[0];
            corner_t_l[1] = corner1[1];
            corner_t_l[2] = corner1[2];
            corner_t_l[3] = corner1[3];
            prim.Color(0x80, 0x80, 0x80, alpha);
            prim.TextureCrd(tex_u, tex_v);
            prim.Vertex4(corner0);
            prim.TextureCrd(tex_u + tex_size, tex_v);
            prim.Vertex4(corner_b_r);
            prim.TextureCrd(tex_u, tex_v + tex_size);
            prim.Vertex4(corner_t_l);
            prim.TextureCrd(tex_u, tex_v + tex_size);
            prim.Vertex4(corner_t_l);
            prim.TextureCrd(tex_u + tex_size, tex_v);
            prim.Vertex4(corner_b_r);
            prim.TextureCrd(tex_u + tex_size, tex_v + tex_size);
            prim.Vertex4(corner1);
        }
        prim.End();
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Step__12CFlushEffectFv);

void CPowerLine::CreatPrim(void) {
    float range = radius;
    BattleEffectPrim *streak = &prim[next];
    streak->pos[0] = 2.0f * (range * mgRnd()) - range;
    streak->pos[1] = 0.2f * (height * mgRnd());
    range = radius;
    streak->pos[2] = 2.0f * (range * mgRnd()) - range;
    streak->speed = rise;
    float prim_size = this->prim_size;
    streak->size = prim_size / 2.0f + prim_size * mgRnd() / 2.0f;
    streak->life = prim_life;
    next++;
    if (next >= prim_max) {
        next = 0;
    }
    live_num++;
}

void CPowerLine::Step(void) {
    if ((duration > 0 || live_num > 0) && source != 0) {
        BattleEffectPrim *streak = this->prim;
        for (int i = 0; i < prim_max; i++) {
            if (streak->life > 0) {
                streak->pos[1] += streak->speed;
                streak->life--;
                if (streak->life <= 0) {
                    live_num--;
                }
            }
            streak++;
        }
        source->GetPosition(pos);
        if (duration > 0 && elapsed < duration) {
            elapsed++;
            if (elapsed >= duration) {
                duration = 0;
            }
            CreatPrim();
        }
    }
}

void CPowerLine::Draw(void) {
    if (duration > 0 || live_num > 0) {
        CPreSprite sprite;
        float center[4];
        int corner0[4];
        int corner_b_r[4];
        int corner_t_l[4];
        int corner1[4];
        sprite.Initialize(0, 0);
        sprite.Preset2D();
        sprite.DepthTestEnable(1);
        sprite.DepthTest(1);
        sprite.Bilinear(1);
        sprite.Coord(1);
        sprite.AlphaBlend(2);
        sprite.Begin(3);
        sprite.Texture(TEX_SystemEffect1);
        sprite.AlphaTestEnable(1);
        BattleEffectPrim *streak = this->prim;
        for (int i = 0; i < prim_max; i++) {
            if (streak->life <= 0) {
                streak++;
                continue;
            }
            int tex_u = tex_rect.left;
            int tex_v = tex_rect.top;
            float width = 20.0f;
            float sprite_height = 0.2f;
            sceVu0AddVector(center, &streak->pos[0], pos);
            center[3] = 1.0f;
            if (mgTransWorldPrim3DSprite(corner0, corner1, center, sprite_height, width, 0) != 0) {
                corner_b_r[0] = corner1[0];
                corner_b_r[1] = corner0[1];
                corner_b_r[2] = corner0[2];
                corner_b_r[3] = corner0[3];
                corner_t_l[0] = corner0[0];
                corner_t_l[1] = corner1[1];
                corner_t_l[2] = corner1[2];
                corner_t_l[3] = corner1[3];
                sprite.Color(color[0], color[1], color[2], color[3]);
                sprite.TextureCrd(tex_u, tex_v);
                sprite.Vertex4(corner0);
                sprite.TextureCrd(tex_u + 0xF, tex_v);
                sprite.Vertex4(corner_b_r);
                sprite.TextureCrd(tex_u, tex_v + 0x1F);
                sprite.Vertex4(corner_t_l);
                sprite.TextureCrd(tex_u, tex_v + 0x1F);
                sprite.Vertex4(corner_t_l);
                sprite.TextureCrd(tex_u + 0xF, tex_v);
                sprite.Vertex4(corner_b_r);
                sprite.TextureCrd(tex_u + 0xF, tex_v + 0x1F);
                sprite.Vertex4(corner1);
            }
            streak++;
        }
        sprite.End();
    }
}

void CDeadEffect::SetDeadEffect(float *pos, float radius, float height, float size, int duration) {
    sceVu0CopyVector(this->pos, pos);
    this->radius = radius;
    this->height = height;
    this->size = size;
    this->duration = duration;
    this->elapsed = 0;
}

void CDeadEffect::CreatPrim(int kind) {
    if (prim != NULL) {
        BattleEffectPrim *particle = &prim[next];
        particle->kind = kind;
        float range = radius;
        particle->pos[0] = 2.0f * (range * mgRnd()) - range;
        particle->pos[1] = 0.3f * (height * mgRnd());
        range = radius;
        particle->pos[2] = 2.0f * (range * mgRnd()) - range;
        particle->velocity[0] = 8.0f * mgRnd() - 4.0f;
        particle->velocity[1] = 1.0f + 0.5f * mgRnd();
        particle->velocity[2] = 8.0f * mgRnd() - 4.0f;
        if (kind == 0) {
            particle->life = (int)(10.0f * mgRnd()) + 20;
            particle->life_max = particle->life;
            particle->speed = 0.2f;
            particle->size = 1.5f + size * mgRnd();
            particle->rate = 32.0f + 8.0f * mgRnd();
        }
        if (kind == 1) {
            particle->life = (int)(10.0f * mgRnd()) + 20;
            particle->life_max = particle->life;
            particle->speed = 0.2f;
            particle->size = 0.1f + size * (0.8f * mgRnd());
            particle->rate = 32.0f + 64.0f * mgRnd();
        }
        next += 1;
        if (next >= prim_max) {
            next = 0;
        }
        live_num += 1;
    }
}

void CDeadEffect::Step(void) {
    if (duration > 0 || live_num > 0) {
        BattleEffectPrim *spark = prim;
        for (int i = 0; i < prim_max; spark++, i++) {
            if (spark->life > 0) {
                spark->pos[0] += spark->velocity[0];
                spark->pos[1] += spark->velocity[1];
                spark->pos[2] += spark->velocity[2];
                spark->velocity[0] = spark->velocity[0] - 0.2f * spark->velocity[0];
                spark->velocity[2] = spark->velocity[2] - 0.2f * spark->velocity[2];
                if (spark->kind == 1) {
                    spark->size = spark->size - 0.01f;
                }
                spark->life = spark->life - 1;
                if (spark->life <= 0) {
                    live_num -= 1;
                }
            }
        }
        if (duration > 0) {
            if (elapsed < duration) {
                elapsed++;
                if (elapsed >= duration) {
                    duration = 0;
                }
                if (elapsed == 1) {
                    for (int n = 0; n < 10; n++) {
                        CreatPrim(0);
                    }
                }
                CreatPrim(0);
                CreatPrim(1);
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw__11CDeadEffectFv);

void CMapEffect_Sprite::Set(float *pos) {
    sceVu0CopyVector(this->pos, pos);
    sceVu0CopyVector(target, pos);
    target[3] = 1.0f;
    this->pos[3] = 1.0f;
    target[0] = target[0] + 3.0f * ((600.0f * (float)rand()) / 2.1474836e9f - 300.0f);
    target[2] += 3.0f * ((600.0f * (float)rand()) / 2.1474836e9f - 300.0f);
    life = (int)(90.0f + (150.0f * (float)rand()) / 2.1474836e9f);
    life_max = life;
    bob_angle = (6.0f * (float)rand()) / 2.1474836e9f - 3.0f;
    bob_height = 0.4f + (2.0f * (float)rand()) / 2.1474836e9f;
    speed = 0.4f + (float)rand() / 2.1474836e9f;
    direction[0] = target[0] - this->pos[0];
    direction[1] = target[1] - this->pos[1];
    direction[2] = target[2] - this->pos[2];
    direction[3] = 1.0f;
    sceVu0Normalize(direction, direction);
}

void CMapEffect_Sprite::Step(mgCCamera *camera) {
    sceVu0FVECTOR camera_pos;
    sceVu0FVECTOR move;
    if (life > 0) {
        camera->GetPos(camera_pos);
        move[0] = target[0] - pos[0];
        move[1] = target[1] - pos[1];
        move[2] = target[2] - pos[2];
        move[3] = 1.0f;
        sceVu0Normalize(move, move);
        move[0] += (move[0] + direction[0]) / 16.0f;
        move[1] = move[1] + (move[1] + direction[1]) / 16.0f;
        move[2] = move[2] + (move[2] + direction[2]) / 16.0f;
        sceVu0ScaleVectorXYZ(move, move, speed);
        pos[0] += move[0];
        pos[1] += move[1];
        pos[2] += move[2];
        bob_angle += 0.05235988f;
        if (bob_angle > 3.1415927f) {
            bob_angle -= 6.2831855f;
        }
        if (mgDistVector(target, pos) < 2.0f) {
            target[0] = pos[0] + 2.0f * ((600.0f * (float)rand()) / 2.1474836e9f - 300.0f);
            target[2] = pos[2] + 2.0f * ((600.0f * (float)rand()) / 2.1474836e9f - 300.0f);
            speed = 0.4f + (0.8f * (float)rand()) / 2.1474836e9f;
        }
        if (mgDistVector(pos, camera_pos) > 600.0f) {
            life = 0;
        }
        life -= 1;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw__17CMapEffect_SpriteFP9mgCCameraP10CPreSprite);

void CMapEffectsManeger::Init_LightBoll(mgCMemory *memory, int sprite_num) {
    this->sprite_num = sprite_num;
    u32 blocks;
    if (((u32)sprite_num * (int)sizeof(CMapEffect_Sprite)) & 0xF) {
        blocks = (((u32)sprite_num * (int)sizeof(CMapEffect_Sprite)) >> 4) + 1;
    } else {
        blocks = ((u32)sprite_num * (int)sizeof(CMapEffect_Sprite)) >> 4;
    }
    void *block = memory->Alloc(blocks + 2);
    sprite = (CMapEffect_Sprite *)__construct_new_array(
        operator new[](sprite_num * (int)sizeof(CMapEffect_Sprite) + 0x10, (u_long128 *)block),
        NULL, NULL, sizeof(CMapEffect_Sprite), sprite_num);
    for (int i = 0; i < sprite_num; i++) {
        sprite[i].life = 0;
    }
}

void CMapEffectsManeger::Step(mgCCamera *camera) {
    sceVu0FVECTOR spawn;
    sceVu0FVECTOR camera_pos;
    sceVu0FVECTOR camera_ref;
    sceVu0FVECTOR forward;
    sceVu0FVECTOR to_spawn;
    if (type < MAP_EFFECT_D01 || type >= 3) {
        return;
    }
    camera->GetPos(camera_pos);
    camera->GetRef(camera_ref);
    forward[0] = camera_ref[0] - camera_pos[0];
    forward[1] = camera_ref[1] - camera_pos[1];
    forward[2] = camera_ref[2] - camera_pos[2];
    forward[3] = 1.0f;
    sceVu0Normalize(forward, forward);
    if (spawn_wait > 0) {
        spawn_wait--;
    }
    live_num = 0;
    for (int i = 0; i < sprite_num; i++) {
        if (sprite[i].life > 0) {
            live_num++;
        }
    }
    if (live_num < sprite_num && spawn_wait <= 0) {
        for (int i = 0; i < sprite_num; i++) {
            if (sprite[i].life <= 0) {
                spawn[0] = camera_pos[0] + ((800.0f * (float)rand()) / 2.1474836e9f - 400.0f);
                spawn[1] = 5.0f + (camera_pos[1] + (50.0f * (float)rand()) / 2.1474836e9f);
                spawn[2] = camera_pos[2] + ((800.0f * (float)rand()) / 2.1474836e9f - 400.0f);
                spawn[3] = 1.0f;
                to_spawn[0] = spawn[0] - camera_pos[0];
                to_spawn[1] = spawn[1] - camera_pos[1];
                to_spawn[2] = spawn[2] - camera_pos[2];
                to_spawn[3] = 1.0f;
                sceVu0Normalize(to_spawn, to_spawn);
                if (sceVu0InnerProduct(forward, to_spawn) > 0.25f) {
                    if (type == MAP_EFFECT_D03) {
                        spawn[1] -= 130.0f;
                    }
                    sprite[i].Set(spawn);
                    CMapEffect_Sprite *slot = &sprite[i];
                    slot->type = type;
                    spawn_wait = (int)(1.0f + (5.0f * (float)rand()) / 2.1474836e9f);
                }
                break;
            }
        }
    }
    for (int i = 0; i < sprite_num; i++) {
        if (sprite[i].life > 0) {
            sprite[i].Step(camera);
        }
    }
}


#ifdef NONMATCHING
void CMapEffectsManeger::Draw(mgCCamera *camera) {
    int index;
    mgCCamera *draw_camera = camera;
    CMapEffectsManeger *manager = this;
    CPreSprite primitive;
    if (manager->type < MAP_EFFECT_D01 || manager->type >= 3) {
        return;
    }
    primitive.Initialize(NULL, NULL);
    primitive.Preset2D();
    if (manager->type == MAP_EFFECT_D01) {
        primitive.DepthTestEnable(0);
    }
    if (manager->type == MAP_EFFECT_D02) {
        primitive.DepthTestEnable(1);
        primitive.DepthTest(1);
    }
    if (manager->type == MAP_EFFECT_D03) {
        primitive.DepthTestEnable(1);
        primitive.DepthTest(1);
    }
    primitive.Bilinear(1);
    primitive.Coord(1);
    primitive.AlphaBlend(2);
    primitive.AlphaTestEnable(1);
    primitive.Begin(MG_PRIM_TRIANGLE);
    primitive.Texture(TEX_SystemEffect1);
    index = 0;
    for (; index < manager->sprite_num; index++) {
        CMapEffect_Sprite *entry = &manager->sprite[index];
        if (entry->life > 0) {
            entry->Draw(draw_camera, &primitive);
        }
    }
    primitive.End();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw__18CMapEffectsManegerFP9mgCCamera);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", AllocEffect__15BattleEffectManFiP9mgCMemoryi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", __ct__11CCharacter2Fv);

CPowerLine::CPowerLine(void) {
    tex_rect.Set(0, 0, 0, 0);
    color[0] = 0x80;
    color[1] = 0x80;
    color[2] = 0x80;
    color[3] = 0x80;
}

CHitEffectImage::CHitEffectImage(void) {
    tex_rect.Set(0, 0, 0, 0);
}

void BattleEffectMan::Step(void) {
    int hit_no;
    int flush_no;
    int line_no;
    int dead_no;
    CHitEffectImage *hit;
    CFlushEffect *flush;
    CPowerLine *line;
    CDeadEffect *dead;

    hit = this->hit;
    if (hit != 0) {
        for (hit_no = 0; hit_no < hit_num; hit_no++) {
            hit->Step();
            hit++;
        }
    }
    flush = this->flush;
    if (flush != 0) {
        for (flush_no = 0; flush_no < flush_num; flush_no++) {
            flush->Step();
            flush++;
        }
    }
    line = power;
    if (line != 0) {
        for (line_no = 0; line_no < power_num; line_no++) {
            line->Step();
            line++;
        }
    }
    dead = this->dead;
    if (dead != 0) {
        for (dead_no = 0; dead_no < dead_num; dead_no++) {
            dead->Step();
            dead++;
        }
    }
}

void BattleEffectMan::Draw(void) {
    int hit_no;
    int flush_no;
    int line_no;
    int dead_no;
    CHitEffectImage *hit;
    CFlushEffect *flush;
    CPowerLine *line;
    CDeadEffect *dead;

    hit = this->hit;
    if (hit != 0) {
        for (hit_no = 0; hit_no < hit_num; hit_no++) {
            hit->Draw();
            hit++;
        }
    }
    flush = this->flush;
    if (flush != 0) {
        for (flush_no = 0; flush_no < flush_num; flush_no++) {
            flush->Draw();
            flush++;
        }
    }
    line = power;
    if (line != 0) {
        for (line_no = 0; line_no < power_num; line_no++) {
            line->Draw();
            line++;
        }
    }
    dead = this->dead;
    if (dead != 0) {
        for (dead_no = 0; dead_no < dead_num; dead_no++) {
            dead->Draw();
            dead++;
        }
    }
}

void CWeaponElement::Initialize(void) {
    int i;

    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        size[i] = 1.0f;
        alpha[i] = 1.0f;
    }
    on = 0;
}

void CWeaponElement::Set(sceVu0FVECTOR *origin, float *position, float power, int kind, float spread) {
    this->power = 0.01f * (1.0f + power);
    this->kind = kind;
    this->spread = spread;
    this->origin = origin;
    switch (kind) {
        case WEAPON_ELEMENT_COLD:
        default:
            Init_Cold(position);
            break;
        case WEAPON_ELEMENT_WIND:
            Init_Wind(position);
            break;
        case WEAPON_ELEMENT_FIRE:
            Init_Fire(position);
            break;
        case WEAPON_ELEMENT_THUNDER:
            Init_Thunder(position);
            break;
    }
    on = 1;
}

void CWeaponElement::Step(void) {
    if (on != 0) {
        switch (kind) {
        case WEAPON_ELEMENT_COLD:
        default:
            this->Step_Cold();
            return;
        case WEAPON_ELEMENT_WIND:
            this->Step_Wind();
            return;
        case WEAPON_ELEMENT_FIRE:
            this->Step_Fire();
            return;
        case WEAPON_ELEMENT_THUNDER:
            this->Step_Thunder();
            break;
        }
    }
}

void CWeaponElement::Draw(void) {
    if (on != 0) {
        switch (kind) {
        case WEAPON_ELEMENT_COLD:
        default:
            this->Draw_Cold();
            return;
        case WEAPON_ELEMENT_WIND:
            this->Draw_Wind();
            return;
        case WEAPON_ELEMENT_FIRE:
            this->Draw_Fire();
            return;
        case WEAPON_ELEMENT_THUNDER:
            this->Draw_Thunder();
            break;
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Init_Cold__14CWeaponElementFPf);

void CWeaponElement::Step_Cold(void) {
    int dead;
    int i;
    int j;

    dead = 0;
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        if (alpha[i] <= 0.0f) {
            dead += 1;
        } else {
            offset[i][1] -= 0.2f + (0.6f * (float)rand()) / 2.1474836e9f;
            shrink[i] -= 0.01f;
            if (shrink[i] <= 0.0f) {
                shrink[i] = 0.0f;
                alpha[i] = 0.0f;
            }
            if (frame_timer == 4) {
                frame[i] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
            }
            if (fading[i] != 0) {
                alpha[i] -= 12.0f;
                if (alpha[i] <= 0.0f) {
                    alpha[i] = 0.0f;
                }
            } else {
                alpha[i] += 24.0f;
                if (alpha[i] >= 128.0f) {
                    fading[i] = 1;
                }
            }
        }
    }
    frame_timer -= 1;
    if (frame_timer == 0) {
        frame_timer = 4;
    }
    if (spawn_budget > 0) {
        spawn_budget -= 2;
        spawn_delay -= 1;
        if (spawn_delay <= 0) {
            for (j = 0; j < WEAPON_ELEMENT_SPARK_MAX; j++) {
                if (alpha[j] == 0.0f) {
                    size[j] = 3.0f + (6.0f * (float)rand()) / 2.1474836e9f;
                    shrink[j] = 1.0f;
                    alpha[j] = 1.0f + (48.0f * (float)rand()) / 2.1474836e9f;
                    fading[j] = 0;
                    offset[j][0] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][1] = spread / 2.0f + (spread * (float)rand()) / 2.1474836e9f;
                    offset[j][2] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][3] = 1.0f;
                    frame[j] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
                    spawn_delay = (int)(((float)spawn_delay_max * (float)rand()) / 2.1474836e9f) + 1;
                    break;
                }
            }
        }
    }
    if (dead >= WEAPON_ELEMENT_SPARK_MAX) {
        on = 0;
    }
}

void CWeaponElement::Draw_Cold(void) {
    sceVu0FVECTOR base;
    sceVu0FVECTOR pos;
    mgCTexture *tex;
    int i;

    tex = mgTexManager.GetTexture("effect02", -1);
    sceVu0CopyVector(base, *origin);
    pos[3] = 1.0f;

    CPreSprite prim;
    int quad_a[4];
    int quad_b[4];
    prim.Initialize(NULL, NULL);
    prim.Preset2D();
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.ZMask(-1);
    prim.Bilinear(1);
    prim.TextureMapEnable(1);
    prim.AlphaBlend(2);
    prim.Begin(6);
    prim.Texture(tex);
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        int tex_row = frame[i];
        float *particle_alpha = &alpha[i];
        if (*particle_alpha > 0.0f) {
            float sprite_size = scale * (this->size[i] * shrink[i]);
            pos[0] = base[0] + offset[i][0];
            pos[1] = base[1] + offset[i][1];
            pos[2] = base[2] + offset[i][2];
            if (mgTransWorldPrim3DSprite(quad_a, quad_b, pos, sprite_size, sprite_size, 0) != 0) {
                prim.Color(0x80, 0x80, 0x80, (int)(*particle_alpha));
                prim.TextureCrd(0xA0, tex_row);
                prim.Vertex4(quad_a);
                prim.TextureCrd(0xD0, tex_row + 0x30);
                prim.Vertex4(quad_b);
            }
        }
    }
    prim.End();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Init_Wind__14CWeaponElementFPf);

void CWeaponElement::Step_Wind(void) {
    int dead;
    int i;
    int j;

    dead = 0;
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        if (alpha[i] <= 0.0f) {
            dead += 1;
        } else {
            if (fading[i] != 0) {
                alpha[i] -= 4.0f;
                if (alpha[i] <= 0.0f) {
                    alpha[i] = 0.0f;
                }
            } else {
                alpha[i] += 32.0f;
                if (alpha[i] >= 128.0f) {
                    fading[i] = 1;
                }
            }
            if (frame_timer == 4) {
                frame[i] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
            }
            spin[i] += spin_speed[i];
            if (spin[i] > 3.1415927f) {
                spin[i] -= 6.2831855f;
            }
            offset[i][0] += velocity[i][0];
            offset[i][1] += 0.2f;
            offset[i][2] += velocity[i][2];
        }
    }
    frame_timer -= 1;
    if (frame_timer == 0) {
        frame_timer = 4;
    }
    if (spawn_budget > 0) {
        spawn_budget -= 1;
        spawn_delay -= 1;
        if (spawn_delay <= 0) {
            for (j = 0; j < WEAPON_ELEMENT_SPARK_MAX; j++) {
                if (alpha[j] == 0.0f) {
                    size[j] = 2.0f + (4.0f * (float)rand()) / 2.1474836e9f;
                    shrink[j] = 1.0f;
                    alpha[j] = 1.0f + (48.0f * (float)rand()) / 2.1474836e9f;
                    fading[j] = 0;
                    offset[j][0] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][1] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][2] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][3] = 1.0f;
                    sceVu0CopyVector(&velocity[j][0], &offset[j][0]);
                    sceVu0Normalize(&velocity[j][0], &velocity[j][0]);
                    sceVu0ScaleVector(&velocity[j][0], &velocity[j][0],
                                      (0.3f * (float)rand()) / 2.1474836e9f);
                    spin[j] = (2.0f * (3.1415927f * (float)rand())) / 2.1474836e9f - 3.1415927f;
                    spin_speed[j] = 0.09817477f + (0.19634955f * (float)rand()) / 2.1474836e9f;
                    frame[j] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
                    spawn_delay = (int)(((float)spawn_delay_max * (float)rand()) / 2.1474836e9f) + 1;
                    break;
                }
            }
        }
    }
    if (dead >= WEAPON_ELEMENT_SPARK_MAX) {
        on = 0;
    }
}

void CWeaponElement::Draw_Wind(void) {
    sceVu0FVECTOR base;
    sceVu0FVECTOR pos;
    mgCTexture *tex;
    int i;

    tex = mgTexManager.GetTexture("effect02", -1);
    sceVu0CopyVector(base, *origin);
    pos[3] = 1.0f;

    CPreSprite prim;
    float identity[4][4];
    float rotation[4][4];
    int quad_a[4];
    int quad_b[4];
    prim.Initialize(NULL, NULL);
    prim.Preset2D();
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.ZMask(-1);
    prim.Bilinear(1);
    prim.TextureMapEnable(1);
    prim.AlphaBlend(2);
    prim.Begin(6);
    prim.Texture(tex);
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        int tex_row = frame[i];
        float *particle_alpha = &alpha[i];
        if (*particle_alpha > 0.0f) {
            float sprite_size = scale * (this->size[i] * shrink[i]);
            sceVu0UnitMatrix(identity);
            sceVu0RotMatrixY(rotation, identity, spin[i]);
            sceVu0ApplyMatrix(pos, rotation, &offset[i][0]);
            pos[0] += base[0];
            pos[1] += base[1];
            pos[2] += base[2];
            if (mgTransWorldPrim3DSprite(quad_a, quad_b, pos, sprite_size, sprite_size, 0) != 0) {
                prim.Color(0x80, 0x80, 0x80, (int)(*particle_alpha));
                prim.TextureCrd(0x60, tex_row);
                prim.Vertex4(quad_a);
                prim.TextureCrd(0xA0, tex_row + 0x30);
                prim.Vertex4(quad_b);
            }
        }
    }
    prim.End();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Init_Fire__14CWeaponElementFPf);

void CWeaponElement::Step_Fire(void) {
    int dead;
    int i;
    int j;

    dead = 0;
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        if (alpha[i] <= 0.0f) {
            dead += 1;
        } else {
            offset[i][1] += 0.02f + (1.2f * (float)rand()) / 2.1474836e9f;
            shrink[i] -= 0.01f;
            if (shrink[i] <= 0.0f) {
                shrink[i] = 0.0f;
                alpha[i] = 0.0f;
            }
            if (frame_timer == 4) {
                frame[i] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
            }
            if (fading[i] != 0) {
                alpha[i] -= 8.0f;
                if (alpha[i] <= 0.0f) {
                    alpha[i] = 0.0f;
                }
            } else {
                alpha[i] += 16.0f;
                if (alpha[i] >= 128.0f) {
                    fading[i] = 1;
                }
            }
        }
    }
    frame_timer -= 1;
    if (frame_timer == 0) {
        frame_timer = 4;
    }
    if (spawn_budget > 0) {
        spawn_budget -= 1;
        spawn_delay -= 1;
        if (spawn_delay <= 0) {
            for (j = 0; j < WEAPON_ELEMENT_SPARK_MAX; j++) {
                if (alpha[j] == 0.0f) {
                    size[j] = 2.0f + (6.0f * (float)rand()) / 2.1474836e9f;
                    shrink[j] = 1.0f;
                    alpha[j] = 1.0f + (48.0f * (float)rand()) / 2.1474836e9f;
                    fading[j] = 0;
                    offset[j][0] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][1] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][2] = (2.0f * (spread * (float)rand())) / 2.1474836e9f - spread;
                    offset[j][3] = 1.0f;
                    frame[j] = (int)((5.0f * (float)rand()) / 2.1474836e9f) * 0x30;
                    spawn_delay = (int)(((float)spawn_delay_max * (float)rand()) / 2.1474836e9f) + 1;
                    break;
                }
            }
        }
    }
    if (dead >= WEAPON_ELEMENT_SPARK_MAX) {
        on = 0;
    }
}

void CWeaponElement::Draw_Fire(void) {
    sceVu0FVECTOR base;
    sceVu0FVECTOR pos;
    mgCTexture *tex;
    int i;

    tex = mgTexManager.GetTexture("effect02", -1);
    sceVu0CopyVector(base, fire_pos);
    pos[3] = 1.0f;

    CPreSprite prim;
    int quad_a[4];
    int quad_b[4];
    prim.Initialize(NULL, NULL);
    prim.Preset2D();
    prim.Coord(1);
    prim.DepthTestEnable(1);
    prim.ZMask(-1);
    prim.Bilinear(1);
    prim.TextureMapEnable(1);
    prim.AlphaBlend(2);
    prim.Begin(6);
    prim.Texture(tex);
    for (i = 0; i < WEAPON_ELEMENT_SPARK_MAX; i++) {
        int tex_row = frame[i];
        float *particle_alpha = &alpha[i];
        if (*particle_alpha > 0.0f) {
            float sprite_size = scale * (this->size[i] * shrink[i]);
            pos[0] = base[0] + offset[i][0];
            pos[1] = base[1] + offset[i][1];
            pos[2] = base[2] + offset[i][2];
            if (mgTransWorldPrim3DSprite(quad_a, quad_b, pos, sprite_size, sprite_size, 0) != 0) {
                prim.Color(0x80, 0x80, 0x80, (int)(*particle_alpha));
                prim.TextureCrd(0x30, tex_row);
                prim.Vertex4(quad_a);
                prim.TextureCrd(0x60, tex_row + 0x30);
                prim.Vertex4(quad_b);
            }
        }
    }
    prim.End();
}

void CWeaponElement::Init_Thunder(float *position) {
    sceVu0FVECTOR scaled;
    sceVu0FVECTOR dir;
    int i;
    int j;

    count = (int)(18.0f * power) + 6;
    bolt_count = (int)(7.0f * power) + 1;
    if (count > WEAPON_ELEMENT_SPARK_MAX) {
        count = WEAPON_ELEMENT_SPARK_MAX;
    }
    if (bolt_count > WEAPON_ELEMENT_BOLT_MAX) {
        bolt_count = WEAPON_ELEMENT_BOLT_MAX;
    }

    spread *= (float)(0.8 + 0.4f * power);
    for (i = 0; i < count; i++) {
        velocity[i][0] = (8.0f * (float)rand()) / 2.1474836e9f - 4.0f;
        velocity[i][1] = (8.0f * (float)rand()) / 2.1474836e9f - 4.0f;
        velocity[i][2] = (8.0f * (float)rand()) / 2.1474836e9f - 4.0f;
        sceVu0Normalize(dir, &velocity[i][0]);
        sceVu0ScaleVectorXYZ(scaled, dir, spread);
        offset[i][0] = position[0] + velocity[i][0] + (scaled[0] * (float)rand()) / 2.1474836e9f;
        offset[i][1] = position[1] + velocity[i][1] + (scaled[1] * (float)rand()) / 2.1474836e9f;
        offset[i][2] = position[2] + velocity[i][2] + (scaled[2] * (float)rand()) / 2.1474836e9f;
        offset[i][3] = 1.0f;
        sceVu0ScaleVectorXYZ(&velocity[i][0], dir, (0.3f * (float)rand()) / 2.1474836e9f);
        size[i] = 0.5f + (2.5f * (float)rand()) / 2.1474836e9f;
        shrink[i] = 1.0f;
        alpha[i] = 96.0f + (float)(int)((64.0f * (float)rand()) / 2.1474836e9f);
    }
    for (j = 0; j < bolt_count; j++) {
        bolt_head[j] = (int)(((float)count * (float)rand()) / 2.1474836e9f);
        bolt_tail[j] = (int)(((float)count * (float)rand()) / 2.1474836e9f);
        bolt_timer[j] = (int)((6.0f * (float)rand()) / 2.1474836e9f) * 3 + 3;
        bolt_frame[j] = (int)((4.0f * (float)rand()) / 2.1474836e9f);
    }
}

void CWeaponElement::Step_Thunder(void) {
    int dead;
    int i;
    int j;

    dead = 0;
    for (i = 0; i < count; i++) {
        if (alpha[i] <= 0.0f) {
            dead += 1;
        } else {
            offset[i][0] += velocity[i][0];
            offset[i][1] += velocity[i][1];
            offset[i][2] += velocity[i][2];
            shrink[i] -= 0.01f;
            alpha[i] -= 4.0f;
            if (alpha[i] <= 3.0f) {
                alpha[i] = 0.0f;
            }
        }
    }
    if (dead >= count) {
        on = 0;
        return;
    }
    for (j = 0; j < bolt_count; j++) {
        bolt_timer[j] -= 1;
        if (bolt_timer[j] <= 0) {
            bolt_head[j] = (int)(((float)count * (float)rand()) / 2.1474836e9f);
            bolt_tail[j] = (int)(((float)count * (float)rand()) / 2.1474836e9f);
            bolt_timer[j] = (int)((6.0f * (float)rand()) / 2.1474836e9f) * 3 + 3;
            bolt_frame[j] = (int)((4.0f * (float)rand()) / 2.1474836e9f);
        } else if (bolt_timer[j] % 3 == 0) {
            bolt_frame[j] += 1;
            if (bolt_frame[j] >= 4) {
                bolt_frame[j] = 0;
            }
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", Draw_Thunder__14CWeaponElementFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_effect", CreatSmoothPass__FPA4_fPA4_fiiii);

float unitRotation(mgCFrame *frame, float target, float divide) {
    sceVu0FVECTOR rot;
    float diff;
    float abs_diff;

    frame->GetRotation(rot);
    diff = target - rot[1];
    abs_diff = diff;
    if (diff <= 0.0f) {
        abs_diff = -1.0f * diff;
    }
    if (abs_diff <= 3.1415927f) {
        if (abs_diff <= 3.1415927f / divide) {
            diff = 0.0f;
        }
    } else if (6.2831855f - abs_diff <= 3.1415927f / divide) {
        diff = 0.0f;
    }
    if (diff > 0.0f) {
        if (diff <= 3.1415927f) {
            rot[1] += 3.1415927f / (2.0f * divide);
        } else {
            rot[1] -= 3.1415927f / divide;
        }
    }
    if (diff < 0.0f) {
        if (diff >= -3.1415927f) {
            rot[1] -= 3.1415927f / (2.0f * divide);
        } else {
            rot[1] += 3.1415927f / divide;
        }
    }
    if (diff == 0.0f) {
        rot[1] = target;
    }
    if (rot[1] <= -3.1415927f) {
        rot[1] += 6.2831855f;
    }
    if (rot[1] >= 3.1415927f) {
        rot[1] -= 6.2831855f;
    }
    return rot[1];
}

s32 iRand(s32 limit) {
    return (s32) (((float) limit * (float) rand()) / 2.1474836e9f);
}

float fRand(float limit) {
    return (limit * (float) rand()) / 2.1474836e9f;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", chill_tex_rect_910__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", gb_tbl_1052__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", thn_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", thn_uv__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_1215__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_1216__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_3214__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_1107__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_1981__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_effect", at_2882__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1051, 0x10);
INCLUDE_BSS(at_1214__2, 0x10);
