#include "common.h"
#include "effectlist.hpp"

#include <cstring>
#include <libvu0.h>

#include "dataread.hpp"
#include "effect.hpp"
#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_sprite.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"

static void DivSpriteScreen(mgCDrawPrim &prim);
static void DivSpriteScreen(mgCDrawPrim &prim, int left, int right, int jagged_left);

// Code (.text)
#ifdef NONMATCHING
void CEffectList::LoadEFPFile(char *name, u_int *pack, int block, mgCMemory *stack) {
    int           sizes[64];
    u_int        *files[64];
    char         *names[64];
    char          directory[32];
    char          basename[32];
    char          extension[16];
    int           particle_num;
    int           ctrl_num;
    int           image_num;
    int           i;
    u_int         bytes;
    u_int         quadwords;
    CEffect      *particles;
    CEffectCtrl  *ctrls;

    this->block = block;
    this->pack = pack;
    bytes = strlen(name) + 1;
    quadwords = bytes / 16;
    if (bytes % 16) {
        quadwords++;
    }
    this->name = (char *)stack->Alloc(quadwords);
    strcpy(this->name, name);

    image_num = GetPackFileExt(pack, "img", files, 64, sizes, names);
    for (i = 0; i < image_num; i++) {
        mgTexManager.EnterIMGFile((u_char *)files[i], block, NULL, NULL);
    }

    effect_num = GetPackFileExt(pack, "em", files, 64, sizes, names);
    bytes = effect_num * sizeof(CEffectManager);
    quadwords = bytes / 16;
    if (bytes % 16) {
        quadwords++;
    }
    managers = new (stack->Alloc(quadwords + 2)) CEffectManager[effect_num];
    bytes = effect_num * sizeof(mgC3DSprite);
    quadwords = bytes / 16;
    if (bytes % 16) {
        quadwords++;
    }
    sprites = new (stack->Alloc(quadwords + 2)) mgC3DSprite[effect_num];

    for (i = 0; i < effect_num; i++) {
        DivPathNameExt(names[i], directory, basename, extension);
        managers[i].GetBufferNums((char *)files[i], sizes[i], &particle_num, &ctrl_num);
        bytes = particle_num * sizeof(CEffect);
        quadwords = bytes / 16;
        if (bytes % 16) {
            quadwords++;
        }
        particles = new (stack->Alloc(quadwords + 2)) CEffect[particle_num];
        bytes = ctrl_num * sizeof(CEffectCtrl);
        quadwords = bytes / 16;
        if (bytes % 16) {
            quadwords++;
        }
        ctrls = new (stack->Alloc(quadwords + 2)) CEffectCtrl[ctrl_num];
        managers[i].EntryEffCtrls(particles, particle_num, ctrls, ctrl_num);
        managers[i].Initialize();
        managers[i].Load((char *)files[i], sizes[i]);
        strcpy(managers[i].name, basename);
        managers[i].Run();
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effectlist", LoadEFPFile__11CEffectListFPcPUiiP9mgCMemory);
#endif

#ifdef NONMATCHING
// Defined inline in mg_sprite.hpp.
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effectlist", __ct__11mgC3DSpriteFv);
#endif

int CEffectList::SaerchEffectIndex(char *name) {
    int i;

    for (i = 0; i < effect_num; i++) {
        if (strcmp(name, managers[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

mgC3DSprite *CEffectList::GetEffectVisual(int index) {
    if (index < 0 || index >= effect_num) {
        return NULL;
    }
    return &sprites[index];
}

void CEffectList::Step() {
    int i;

    for (i = 0; i < effect_num; i++) {
        managers[i].Ctrl();
        managers[i].Step(1);
    }
}

void CEffectList::CreatePacket() {
    int i;

    for (i = 0; i < effect_num; i++) {
        managers[i].CreatePacket(&sprites[i]);
    }
}

#ifdef NONMATCHING
void CEffectManager::CreatePacket(mgC3DSprite *sprite) {
    if (sprite == NULL) {
        return;
    }

    mgCDrawEnv    env = *mgGetpDrawEnv(0);
    sceVu0FVECTOR size = { 0.0f, 0.0f, 0.0f, 0.0f };
    sceVu0FVECTOR color = { 128.0f, 128.0f, 128.0f, 128.0f };
    sceVu0FVECTOR uv0;
    sceVu0FVECTOR uv1;
    mgCTexture   *last_texture;
    mgCTexture   *texture;
    CEffect      *particle;
    int           last_blend;
    int           blend;
    int           first;
    int           texture_changed;
    int           blend_changed;
    int           i;

    env.test.bits.zte = 1;
    env.test.bits.ztst = SCE_GS_ZGEQUAL;
    env.SetZBuf(MG_ZBUF_NO_WRITE);
    env.SetAlpha(MG_ALPHA_MACRO_ADD);
    sprite->BeginCreatePacket(MG_3DSPRITE_MODE_ROTATE, NULL);
    sprite->CPSetDrawEnv(&env);
    sprite->BeginCPSprite();
    mgZeroVector(uv0);
    mgZeroVector(uv1);
    last_texture = NULL;
    last_blend = MG_ALPHA_MACRO_ADD;
    first = 1;

    for (i = 0; i < effect_num; i++) {
        particle = &effects[i];
        if (particle->active) {
            texture = particle->param.texture;
            if (texture != NULL) {
                if (particle->param.alpha_blend == EFFECT_ALPHA_BLEND_ADD) {
                    blend = MG_ALPHA_MACRO_ADD;
                } else if (particle->param.alpha_blend == EFFECT_ALPHA_BLEND_SUB) {
                    blend = MG_ALPHA_MACRO_SUB;
                } else {
                    blend = MG_ALPHA_MACRO_OPAQUE;
                }
                size[0] = particle->param.width * particle->scale[0];
                size[1] = particle->param.height * particle->scale[0];
                color[3] = 128.0f * particle->alpha;
                texture_changed = 0;
                blend_changed = 0;
                if (last_texture != texture) {
                    texture_changed = 1;
                }
                if (first || last_blend != blend) {
                    blend_changed = 1;
                }
                first = 0;
                if (texture_changed || blend_changed) {
                    sprite->EndCPSprite();
                    env.SetAlpha(blend);
                    if (blend_changed) {
                        sprite->CPSetDrawEnv(&env);
                    }
                    sprite->CPSetTexture(texture);
                    sprite->BeginCPSprite();
                }
                last_texture = texture;
                last_blend = blend;
                uv0[0] = particle->tex_rect[0];
                uv0[1] = particle->tex_rect[1];
                uv1[0] = particle->tex_rect[0] + particle->tex_rect[2] - 1;
                uv1[1] = particle->tex_rect[1] + particle->tex_rect[3] - 1;
                particle->pos[3] = 1.0f;
                sprite->CPSetSprite(particle->pos, size, color, uv0, uv1);
            }
        }
    }
    sprite->EndCPSprite();
    sprite->EndCreatePacket();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effectlist", CreatePacket__14CEffectManagerFP11mgC3DSprite);
#endif

void CFadeInOut::Initialize(void) {
    alpha = 0.0f;
    b = 0.0f;
    g = 0.0f;
    r = 0.0f;
    mode = 0;
    speed = 0.0f;
    end = 0;
    cross = 0;
    cross_texture = NULL;
    blur_alpha = 0;
}
void CFadeInOut::ResetFade(void) {
    mode = 0;
    alpha = 0.0f;
    cross = 0;
}
void CFadeInOut::FadeIn(int frames, float r, float g, float b) {
    if (mode == FADE_MODE_NONE || frames < 0) {
        alpha = 128.0f;
    }
    mode = FADE_MODE_IN;
    end = 0;
    if (frames < 0) {
        speed = 0.0f;
    } else {
        speed = 128.0f / frames;
    }
    this->r = r;
    this->g = g;
    this->b = b;
    cross = 0;
}

void CFadeInOut::FadeIn(int frames) {
    if (mode >= FADE_MODE_NONE) {
        FadeIn(frames, 0.0f, 0.0f, 0.0f);
        return;
    }
    FadeIn(frames, r, g, b);
}

void CFadeInOut::FadeOut(int frames, float r, float g, float b) {
    if (mode == FADE_MODE_NONE || frames < 0) {
        alpha = 0.0f;
    }
    mode = FADE_MODE_OUT;
    end = 0;
    if (frames < 0) {
        speed = 0.0f;
    } else {
        speed = 128.0f / frames;
    }
    this->r = r;
    this->g = g;
    this->b = b;
    cross = 0;
}

void CFadeInOut::CrossFade(int duration, float alpha) {
    CrossFadeIn(0, duration, alpha);
}
void CFadeInOut::CrossFadeIn(int type, int frames, float alpha_rate) {
    cross_type = type;
    FadeIn(frames, 128.0f, 128.0f, 128.0f);
    cross = 1;
    cross_alpha_rate = alpha_rate;
}

void CFadeInOut::CrossFadeOut(int type, int frames, float alpha_rate) {
    cross_type = type;
    FadeOut(frames, 128.0f, 128.0f, 128.0f);
    alpha = 0.0f;
    cross = 1;
    cross_alpha_rate = alpha_rate;
}

int CFadeInOut::FadeCheck() { return this->end; }
s32 CFadeInOut::NowFade(void) {
    return mode != 0;
}
int CFadeInOut::FadeStep() {
    if (mode == FADE_MODE_NONE) {
        return 1;
    }
    if (mode > FADE_MODE_NONE) {
        alpha -= speed;
        if (alpha <= 0.0f) {
            alpha = 0.0f;
            mode = FADE_MODE_NONE;
            end = 1;
        }
    } else {
        alpha += speed;
        if (alpha >= 128.0f) {
            alpha = 128.0f;
            end = 1;
        }
    }
    if (end && cross && cross_type == CROSS_FADE_WIPE) {
        alpha = 0.0f;
    }
    return end;
}

void CFadeInOut::SetCrossTexture(mgCTexture *texture, u_long128 *buffer) {
    if (texture != NULL) {
        cross_texture = texture;
        cross_texture->image[0] = buffer;
    }
}

void CFadeInOut::CaptureScreen() {
    if (cross_texture == NULL || cross_texture->image[0] == NULL) {
        return;
    }
    mgCTexture backbuffer;

    mgGetFrameBackBuffer(&backbuffer);
    mgStoreImage(&backbuffer, cross_texture->image[0]);
}

#ifdef NONMATCHING
/**
 * Tiles the screen with textured rectangles of 64 by 32 pixels.
 */
static void DivSpriteScreen(mgCDrawPrim &prim) {
    sceVu0IVECTOR offset = { 0, 0, 0, 0 };
    sceVu0IVECTOR vertex = { 0, 0, 0, 0 };
    sceVu0IVECTOR uv = { 0, 0, 0, 0 };
    int           x;
    int           y;

    prim.BeginPrim2(MG_PRIM_SPRITE, 0x43, 0, 2);
    offset[0] = mgScreenOffx * 16;
    offset[1] = mgScreenOffy * 16;
    for (x = 0; x < mgScreenWidth; x += 64) {
        for (y = 0; y < mgScreenHeight; y += 32) {
            *(u_long128 *)vertex = *(u_long128 *)offset;
            uv[0] = x * 16;
            uv[1] = y * 16;
            prim.Data(uv);
            vertex[0] = uv[0] + offset[0];
            vertex[1] = uv[1] + offset[1];
            prim.Data(vertex);
            uv[0] = (x + 64) * 16;
            uv[1] = (y + 32) * 16;
            prim.Data(uv);
            vertex[0] = uv[0] + offset[0];
            vertex[1] = uv[1] + offset[1];
            prim.Data(vertex);
        }
    }
    prim.EndPrim2();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effectlist", DivSpriteScreen__FR11mgCDrawPrim);
#endif

#ifdef NONMATCHING
/**
 * Draws a strip across the screen with a jagged left or right edge.
 */
static void DivSpriteScreen(mgCDrawPrim &prim, int left, int right, int jagged_left) {
    sceVu0IVECTOR offset = { 0, 0, 0, 0 };
    sceVu0IVECTOR vertex = { 0, 0, 0, 0 };
    sceVu0IVECTOR uv = { 0, 0, 0, 0 };
    int           edge_offset[2] = { -10, 10 };
    int           row_height;
    int           row;

    prim.BeginPrim2(MG_PRIM_TRIANGLE_STRIP, 0x43, 0, 2);
    offset[0] = mgScreenOffx * 16;
    offset[1] = mgScreenOffy * 16;
    row_height = mgScreenHeight / 16;
    for (row = 0; row < 17; row++) {
        if (jagged_left) {
            uv[0] = (left + edge_offset[row % 2]) * 16;
        } else {
            uv[0] = left * 16;
        }
        if (uv[0] < 0) {
            uv[0] = 0;
        }
        uv[1] = row * row_height * 16;
        prim.Data(uv);
        vertex[0] = uv[0] + offset[0];
        vertex[1] = uv[1] + offset[1];
        prim.Data(vertex);
        if (!jagged_left) {
            uv[0] = (right + edge_offset[row % 2]) * 16;
        } else {
            uv[0] = right * 16;
        }
        if (uv[0] < 0) {
            uv[0] = 0;
        }
        uv[1] = row * row_height * 16;
        prim.Data(uv);
        vertex[0] = uv[0] + offset[0];
        vertex[1] = uv[1] + offset[1];
        prim.Data(vertex);
    }
    prim.EndPrim2();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effectlist", DivSpriteScreen__FR11mgCDrawPrimiii);
#endif

void CFadeInOut::Draw() {
    if (alpha > 0.0f) {
        mgCDrawPrim prim;

        prim.Initialize(NULL, NULL);
        prim.DepthTestEnable(0);
        prim.AlphaTestEnable(0);
        prim.AlphaBlendEnable(1);
        prim.AlphaBlend(MG_ALPHA_BLEND_NORMAL);
        prim.ZMask(MG_Z_MASK_MASKED);
        if (cross) {
            if (cross_texture != NULL) {
                cross_texture->tex0.bits.tcc = 0;
                mgTexManager.ReloadTexture(cross_texture->block, (sceVif1Packet *)NULL);
                prim.TextureMapEnable(1);
                if (cross_type == CROSS_FADE_WIPE) {
                    prim.Begin2();
                    prim.BeginPrim2(MG_PRIM_SPRITE);
                    prim.Texture(cross_texture);
                    prim.Direct(SCE_GS_TEXA, 0x8080UL | (0x80UL << 32));
                    prim.Color(128, 128, 128, 128);
                    prim.EndPrim2();
                    if (mode > FADE_MODE_NONE) {
                        DivSpriteScreen(prim, 0, (int)((alpha / 128.0f) * mgScreenWidth), 0);
                    } else {
                        DivSpriteScreen(prim, (int)((alpha / 128.0f) * mgScreenWidth), mgScreenWidth, 1);
                    }
                    prim.End2();
                } else {
                    prim.Begin2();
                    prim.BeginPrim2(MG_PRIM_SPRITE);
                    prim.Texture(cross_texture);
                    prim.Direct(SCE_GS_TEXA, 0x8080UL | (0x80UL << 32));
                    prim.Color((int)r, (int)g, (int)b, (int)(alpha * cross_alpha_rate));
                    prim.EndPrim2();
                    DivSpriteScreen(prim);
                    prim.End2();
                }
            }
        } else {
            prim.TextureMapEnable(0);
            prim.Begin2();
            prim.BeginPrim2(MG_PRIM_SPRITE);
            prim.Color((int)r, (int)g, (int)b, (int)alpha);
            prim.EndPrim2();
            DivSpriteScreen(prim);
            prim.End2();
        }
    }
    if (blur_alpha) {
        mgCDrawPrim prim;

        prim.Initialize(NULL, NULL);
        mgCTexture backbuffer;

        mgGetFrameBackBuffer(&backbuffer);
        backbuffer.tex0.bits.tcc = 0;
        prim.TextureMapEnable(1);
        prim.AlphaBlendEnable(1);
        prim.AlphaBlend(MG_ALPHA_BLEND_NORMAL);
        prim.DepthTestEnable(0);
        prim.ZMask(MG_Z_MASK_MASKED);
        prim.Begin(MG_PRIM_SPRITE);
        prim.Direct(SCE_GS_TEXA, 0x80UL | (0x80UL << 32));
        prim.Texture(&backbuffer);
        prim.Color(128, 128, 128, blur_alpha);
        prim.TextureCrd(0, 0);
        prim.Vertex(0, 0, 0);
        prim.TextureCrd(backbuffer.width, backbuffer.height);
        prim.Vertex(backbuffer.width, backbuffer.height, 0);
        prim.End();
    }
}


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effectlist", at_393__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effectlist", at_260__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effectlist", at_261__DATA);

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effectlist", at_589__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_392, 0x10);
INCLUDE_BSS(at_564__2, 0x10);
INCLUDE_BSS(at_565, 0x10);
INCLUDE_BSS(at_566, 0x10);
INCLUDE_BSS(at_586, 0x10);
INCLUDE_BSS(at_587, 0x10);
INCLUDE_BSS(at_588, 0x10);
