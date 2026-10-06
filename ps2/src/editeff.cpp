#include "common.h"
#include "editeff.hpp"
#include "mg_texture.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mapparts.hpp"
#include "editparts.hpp"
#include "mg_drawenv.hpp"
#include "mglib.hpp"

#include <cmath>

static u32 EffectFlag;
static u32 EffectState;
static CPaintEffect *PaintEffect;
static CStarEffect _StarEffect[EDIT_STAR_EFFECT_MAX];
static mgCMemory CurPartsBuff;
CPlaceAnime PlaceAnime[EDIT_PLACE_ANIME_MAX];

// Code (.text)
#ifdef NONMATCHING
void EditSetEffectBuffer(mgCMemory *memory) {
    mgCTexture *texture = mgTexManager.GetTexture("haichi_eff", -1);
    int i;

    for (i = 0; i < EDIT_STAR_EFFECT_MAX; i++) {
        CStarEffect *star = &_StarEffect[i];
        u32 bytes;
        u32 blocks;
        star->CObject::Initialize();
        star->particle_max = 64;
        star->particle_num = 0;
        bytes = star->particle_max << 5;
        if (bytes & 0xF) {
            blocks = (bytes >> 4) + 1;
        } else {
            blocks = bytes >> 4;
        }
        star->particle = new (memory->Alloc(blocks + 2)) EditStarParticle[star->particle_max];
        star->texture = texture;
    }
    PaintEffect = new (memory->Alloc(0x42)) CPaintEffect[EDIT_PAINT_EFFECT_MAX];
    CurPartsBuff.stSetBuffer(memory->Alloc(1500),
                             1500);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editeff", EditSetEffectBuffer__FP9mgCMemory);
#endif

CPaintEffect::CPaintEffect() {}

void EditInitPlaceEffect(void) {
    int i;
    CPaintEffect *paint;

    for (i = 0; i < EDIT_STAR_EFFECT_MAX; i++) {
        _StarEffect[i].state = EDIT_EFFECT_STATE_FREE;
    }
    EffectFlag = 0;
    EffectState = 0;
    paint = PaintEffect;
    if (paint != NULL) {
        paint->state = EDIT_EFFECT_STATE_FREE;
        paint->shape = 0;
    }
}

#ifdef NONMATCHING
int EditPlaceEffect(CEditParts *parts, float *position) {
    CStarEffect *effect = NULL;
    int oldest_frame = 0;
    int index = 0;
    for (; index < EDIT_STAR_EFFECT_MAX; ++index) {
        CStarEffect *candidate = &_StarEffect[index];
        if (candidate->state == EDIT_EFFECT_STATE_FREE) {
            effect = &_StarEffect[index];
            break;
        }
        if (candidate->frame > oldest_frame) {
            oldest_frame = candidate->frame;
            effect = candidate;
        }
    }
    if (effect == NULL) {
        return 0;
    }
    EffectFlag = 1;
    mgVu0FBOX bounds;
    float spread[4];
    if (parts == NULL || !parts->GetBBox(&bounds)) {
        return 0;
    }
    sceVu0SubVector(spread, bounds.max, bounds.min);
    spread[0] = 0.5f * spread[0];
    for (int axis = 1; axis < 3; ++axis) {
        spread[axis] = 0.5f * spread[axis];
    }
    float radius = mgDistVectorXZ(spread);
    if (radius > 300.0f) {
        radius = 300.0f;
    }
    int particle_count = 10;
    particle_count += radius / (300.0f / 54.0f);
    spread[0] = radius;
    spread[2] = radius;
    effect->ParamInit(spread, particle_count);
    effect->SetPosition(position);
    effect->size = 1.0f + radius / 300.0f;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editeff", EditPlaceEffect__FP10CEditPartsPf);
#endif

int EditPaintEffect(CEditParts *parts, float *position, float *color, int shape) {
    CPaintEffect *paint;
    mgCTexture *texture;
    float size;
    int i;
    int j;

    if (PaintEffect == NULL) {
        return 0;
    }
    paint = NULL;
    for (i = 0; i < EDIT_PAINT_EFFECT_MAX; i++) {
        if (PaintEffect[i].state == EDIT_EFFECT_STATE_FREE) {
            paint = &PaintEffect[i];
        }
    }
    if (paint == NULL) {
        return 0;
    }
    texture = mgTexManager.GetTexture("haichi_eff", -1);
    size = 1.0f;
    if (shape != 0) {
        size = 2.0f;
    }
    EffectFlag = 1;
    paint->ParamInit(size);
    paint->SetPosition(position);
    *(u_long128 *)paint->color = *(u_long128 *)color;
    sceVu0ScaleVector(paint->color, paint->color, 1.2f);
    for (j = 0; j < 3; j++) {
        if (paint->color[j] > 255.0f) {
            paint->color[j] = 255.0f;
        }
    }
    if (shape != 0) {
        paint->shape = 1;
        paint->wait = 0;
    }
    paint->texture = texture;
    return 1;
}

void EditPEffectStep(void) {
    int i;
    int j;

    if (EffectFlag != 0) {
        for (i = 0; i < EDIT_STAR_EFFECT_MAX; i++) {
            _StarEffect[i].Step();
        }

        for (j = 0; j < EDIT_PAINT_EFFECT_MAX; j++) {
            PaintEffect[j].Step();
        }
    }
}

void EditPEffectDraw(int unused) {
    int i;
    int j;

    if (EffectFlag == 0) {
        return;
    }
    for (i = 0; i < EDIT_STAR_EFFECT_MAX; i++) {
        _StarEffect[i].Draw();
    }
    for (j = 0; j < EDIT_PAINT_EFFECT_MAX; j++) {

        PaintEffect[j].Draw();
    }
}

int EditGetPEffectState(void) {
    int result = EDIT_EFFECT_STATE_FREE;
    int i;
    for (i = 0; i < EDIT_STAR_EFFECT_MAX; i++) {
        int state = _StarEffect[i].state;
        if (state != EDIT_EFFECT_STATE_FREE) {
            if (state != EDIT_EFFECT_STATE_END) {
                return 1;
            }
            result = EDIT_EFFECT_STATE_END;
        }
    }
    return result;
}

s32 EditPEffectEndCheck(void) {
    if (EditGetPEffectState() == 3) {
        EditInitPlaceEffect();
        return 3;
    }
    return (EditGetPEffectState() == 0) ^ 1;
}

void CStarEffect::ParamInit(float *spread, int count) {
    int i;
    float radius;
    float theta;
    sceVu0FVECTOR p;
    u_long128 q;

    sceVu0ScaleVector(spread, spread, 0.12f);
    state = EDIT_EFFECT_STATE_PLAY;
    for (i = 0; i < particle_max; i++) {
        radius = 0.5f * spread[0] * (1.0f + mgRnd());
        theta = 6.2831855f * mgRnd();
        p[0] = radius * sinf(theta);
        p[1] = spread[1] * mgRnd();
        p[2] = radius * cosf(theta);
        p[3] = 1.0f;

        q = *(u_long128 *)p;
        *(u_long128 *)particle[i].position = q;
        particle[i].shape = i % 2;
    }
    scale[0] = scale[1] = scale[2] = 1.0f;
    mgZeroVector(rotation);
    position[1] = 0.0f;
    spin_speed = 0.0f;
    rise_speed = 0.0f;
    frame = 0;
    alpha = 1.0f;
    star_angle = 0.0f;
    particle_num = count;
    if (particle_num >= particle_max) {
        particle_num = particle_max;
    }
    size = 1.0f;
}

void CStarEffect::Step() {
    float current;
    float next;

    if (state != EDIT_EFFECT_STATE_FREE) {
        current = scale[0];
        next = current + (10.0f - current) / 9.0f;
        scale[2] = next;
        scale[1] = next;
        scale[0] = next;
        if (frame > 10) {
            if (frame < 30) {
                spin_speed += 0.008f;
                rise_speed += 0.21f;
            }
            alpha -= 0.05f;
            if (alpha < 0.0f) {
                alpha = 0.0f;
                state = EDIT_EFFECT_STATE_END;
            }
        }
        rotation[1] = mgAngleLimit(rotation[1] + spin_speed);
        star_angle += mgAngleLimit(0.2f + rotation[1]);
        position[1] += rise_speed;
        frame += 1;
    }
}

int CStarEffect::Draw() {
    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END) {
        return 0;
    }
    sprite.Initialize();
    mgC3DSprite *billboard = &sprite;
    mgCDrawEnv draw_env = *mgGetpDrawEnv(0);
    sceGsTest *test = &draw_env.test;
    test->bits.zte = 1;
    test->bits.ztst = 2;
    draw_env.SetZBuf(MG_ZBUF_NO_WRITE);
    draw_env.SetAlpha(MG_ALPHA_MACRO_ADD);
    billboard->BeginCreatePacket(MG_3DSPRITE_MODE_ROTATE, NULL);
    billboard->CPSetDrawEnv(&draw_env);
    billboard->CPSetTexture(texture);
    billboard->BeginCPSprite();
    float star_size[2][4] = {{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.0f}};
    star_size[0][0] = 25.0f * size;
    star_size[0][1] = 25.0f * size;
    star_size[0][2] = star_angle;
    star_size[1][0] = 15.0f * size;
    star_size[1][1] = 15.0f * size;
    star_size[1][2] = 0.5f * star_angle;
    float uv_start[2][4] = {{0.0f, 0.0f, 0.0f, 0.0f}, {32.0f, 5.0f, 0.0f, 0.0f}};
    float uv_end[2][4] = {{32.0f, 31.0f, 0.0f, 0.0f}, {48.0f, 15.0f, 0.0f, 0.0f}};
    float star_color[4] = {128.0f, 128.0f, 128.0f, 0.0f};
    star_color[3] = 128.0f * alpha;
    for (int index = 0; index < particle_num; ++index) {
        EditStarParticle *star = &particle[index];
        sceVu0FVECTOR star_position;
        sceVu0MulVector(star_position, star->position, scale);
        star_position[3] = 1.0f;
        billboard->CPSetSprite(star_position, star_size[star->shape], star_color,
                               uv_start[star->shape], uv_end[star->shape]);
    }
    billboard->EndCPSprite();
    billboard->EndCreatePacket();
    float matrix[4][4];
    mgCreateMatrixPY(matrix, position, rotation[1]);
    int fog_enabled = mgGetFogEnable();
    mgFogEnable(0);
    int result = mgDrawDirect(billboard, matrix);
    mgFogEnable(fog_enabled);
    return result;
}

void CPaintEffect::ParamInit(float size) {
    int i;

    state = EDIT_EFFECT_STATE_PLAY;
    for (i = 0; i < EDIT_PAINT_DROP_NUM; i++) {
        mgZeroVector(drop[i]);
        drop[i][3] = size * (0.5f + 0.5f * mgRnd());
        drop[i][1] = 10.0f;
        drop_speed[i][0] = 8.0f * (mgRnd() - 0.5f);
        drop_speed[i][1] = 2.0f;
        drop_speed[i][2] = 8.0f * (mgRnd() - 0.5f);
        drop_speed[i][3] = 0.0f;
    }
    alpha = 1.0f;
    wait = 10;
}

void CPaintEffect::Step() {
    int i;

    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END) {
        return;
    }
    wait--;
    if (wait > 0) {
        return;
    }
    state = EDIT_EFFECT_STATE_SCATTER;
    for (i = 0; i < EDIT_PAINT_DROP_NUM; i++) {
        drop_speed[i][1] -= 0.3f;
        mgAddVector(drop[i], drop_speed[i]);
    }
    alpha -= 0.05f;
    if (alpha < 0.0f) {
        state = EDIT_EFFECT_STATE_FREE;
    }
}

int CPaintEffect::Draw() {
    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END || state == EDIT_EFFECT_STATE_PLAY) {
        return 0;
    }
    sprite.Initialize();
    mgC3DSprite *billboard = &sprite;
    mgCDrawEnv draw_env = *mgGetpDrawEnv(0);
    sceGsTest *test = &draw_env.test;
    test->bits.zte = 1;
    test->bits.ztst = 2;
    draw_env.SetZBuf(MG_ZBUF_NO_WRITE);
    draw_env.SetAlpha(MG_ALPHA_MACRO_BLEND);
    billboard->BeginCreatePacket(MG_3DSPRITE_MODE_UPRIGHT, NULL);
    billboard->CPSetDrawEnv(&draw_env);
    billboard->CPSetTexture(texture);
    billboard->BeginCPSprite();
    float uv_start[2][4] = {{0.0f, 32.0f, 0.0f, 0.0f}, {32.0f, 32.0f, 0.0f, 0.0f}};
    float uv_end[2][4] = {{32.0f, 63.0f, 0.0f, 0.0f}, {63.0f, 63.0f, 0.0f, 0.0f}};
    sceVu0FVECTOR drop_color;
    *(u_long128 *)drop_color = *(u_long128 *)color;
    drop_color[3] = 128.0f * alpha;
    for (int index = 0; index < EDIT_PAINT_DROP_NUM; ++index) {
        float drop_position[4];
        float drop_scale = drop[index][3];
        *(u_long128 *)drop_position = *(u_long128 *)drop[index];
        drop_position[3] = 1.0f;
        sceVu0FVECTOR drop_size = {0.0f, 0.0f, 0.0f, 0.0f};
        drop_size[0] = 10.0f * drop_scale;
        drop_size[1] = drop_size[0];
        billboard->CPSetSprite(drop_position, drop_size, drop_color, uv_start[shape], uv_end[shape]);
    }
    billboard->EndCPSprite();
    billboard->EndCreatePacket();
    float matrix[4][4];
    mgCreateMatrixPY(matrix, position, rotation[1]);
    int fog_enabled = mgGetFogEnable();
    mgFogEnable(0);
    int result = mgDrawDirect(billboard, matrix);
    mgFogEnable(fog_enabled);
    return result;
}

void EditInitPlaceAnime(void) {
    int i;

    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        PlaceAnime[i].state = EDIT_EFFECT_STATE_FREE;
        PlaceAnime[i].type = EDIT_PLACE_ANIME_NONE;
        PlaceAnime[i].parts = NULL;
    }
}

int EditNowPlaceAnime(void) {
    int i;
    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        int state = PlaceAnime[i].state;
        if (state != EDIT_EFFECT_STATE_END && state != EDIT_EFFECT_STATE_FREE) {
            return 1;
        }
    }
    return 0;
}

#ifdef NONMATCHING
int EditSetPlaceAnime(int kind, CMapParts *parts) {
    CPlaceAnime *slot;
    CMapParts *target;
    int oldest;
    int i;
    CPlaceAnime *candidate;
    if (parts == NULL || kind == EDIT_PLACE_ANIME_NONE) {
        return 0;
    }
    slot = NULL;
    target = parts;
    if (kind == EDIT_PLACE_ANIME_REMOVE) {
        for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
            candidate = &PlaceAnime[i];
            if (candidate->state == EDIT_EFFECT_STATE_FREE) {
                slot = candidate;
                if (candidate->type == EDIT_PLACE_ANIME_REMOVE) {
                    break;
                }
            }
        }
        if (slot != NULL) {
            slot->state = 0;
            slot->type = EDIT_PLACE_ANIME_NONE;
            slot->parts = NULL;
            CurPartsBuff.stReset();
            target = new (CurPartsBuff.Alloc(0x33)) CMapParts;
            if (target == NULL) {
                return 0;
            }
            parts->Copy(*target, &CurPartsBuff);
        }
    } else {
        oldest = 0;
        for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
            candidate = &PlaceAnime[i];
            if (candidate->state == EDIT_EFFECT_STATE_FREE) {
                slot = &PlaceAnime[i];
                break;
            }
            if (oldest < candidate->frame) {
                oldest = candidate->frame;
                slot = candidate;
            }
        }
    }
    if (slot == NULL) {
        return 0;
    }
    slot->state = 1;
    slot->type = kind;
    slot->parts = target;
    slot->parts->GetPosition(slot->position);
    slot->parts->GetRotation(slot->rotation);
    slot->parts->GetScale(slot->scale);
    slot->frame = 0;
    slot->power = 1.0f;
    slot->height_speed = 6.0f;
    slot->height = 0.0f;
    slot->phase = 0;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editeff", EditSetPlaceAnime__FiP9CMapParts);
#endif

void EditPlaceAnime(void) {
    int i;

    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        PlaceAnime[i].Step();
    }
}

void EditPlaceAnime2(void) {
    int i;

    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        PlaceAnime[i].Step2();
    }
}

void EditPlaceAnimeDraw(void) {
    int i;

    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        PlaceAnime[i].Draw();
    }
}

void CPlaceAnime::Step() {
    int finished;
    float squash;

    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END) {
        return;
    }
    if (parts == NULL || type == EDIT_PLACE_ANIME_NONE) {
        return;
    }
    parts->GetPosition(base_position);
    parts->GetRotation(base_rotation);
    parts->GetScale(base_scale);
    finished = 0;
    if (type == EDIT_PLACE_ANIME_SWAY) {
        height_speed = height_speed - 1.2f;
        height += height_speed;
        if (height < 0.0f) {
            height = 0.0f;
            height_speed = 0.0f;
        }
        rotation[0] = 0.4f * power * sinf(6.2831855f * ((float)frame / 30.0f));
        rotation[2] =
            0.4f * power * cosf(6.2831855f * ((float)frame / 30.0f) + 1.5707964f * power);
        frame++;
        power = 0.92f * power;
        if (frame > 60) {
            finished = 1;
        }
    } else if (type == EDIT_PLACE_ANIME_SQUASH) {
        switch (phase) {
            case 0:
                height_speed = height_speed - 1.2f;
                height += height_speed;
                if (height < 0.0f) {
                    height = 0.0f;
                    phase++;

                    case 1:
                        height = 0.0f;
                        squash = 1.0f + 0.2f * power * sinf(6.2831855f * ((float)frame / 30.0f));
                        scale[2] = squash;
                        scale[0] = squash;
                        scale[1] = 2.0f - squash;
                        frame++;
                        power = 0.97f * power;
                        if (frame > 15) {
                            finished = 1;
                        }
                }
                break;
        }
    } else if (type == EDIT_PLACE_ANIME_REMOVE) {
        height += 20.0f;
        scale[0] -= 0.1f;
        scale[2] -= 0.1f;
        if (scale[0] < 0.0f) {
            scale[0] = 0.0f;
        }
        if (scale[2] < 0.0f) {
            scale[2] = 0.0f;
        }
        frame++;
        if (frame > 10) {
            finished = 1;
        }
    } else {
        finished = 1;
    }
    if (finished != 0) {
        state = EDIT_EFFECT_STATE_FREE;
        return;
    }
    position[1] = base_position[1] + height;
    parts->SetPosition(position);
    parts->SetRotation(rotation);
    parts->SetScale(scale);
}

void CPlaceAnime::Step2() {
    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END) {
        return;
    }
    if (parts == NULL || type == EDIT_PLACE_ANIME_NONE) {
        return;
    }
    parts->SetPosition(base_position);
    parts->SetRotation(base_rotation);
    parts->SetScale(base_scale);
}

void CPlaceAnime::Draw() {
    if (state == EDIT_EFFECT_STATE_FREE || state == EDIT_EFFECT_STATE_END) {
        return;
    }
    if (parts == NULL || type != EDIT_PLACE_ANIME_REMOVE) {
        return;
    }
    parts->Draw();
}

int EditGetPlaceAnimeState(void) {
    int result = EDIT_EFFECT_STATE_FREE;
    int i;
    for (i = 0; i < EDIT_PLACE_ANIME_MAX; i++) {
        int state = PlaceAnime[i].state;
        if (state != EDIT_EFFECT_STATE_FREE) {
            if (state != EDIT_EFFECT_STATE_END) {
                return 1;
            }
            result = EDIT_EFFECT_STATE_END;
        }
    }
    return result;
}

s32 EditPlaceAnimeEndCheck(void) {
    if (EditGetPlaceAnimeState() == 3) {
        EditInitPlaceAnime();
        return 3;
    }
    return (EditGetPlaceAnimeState() == 0) ^ 1;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_1038__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_1039__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_1040__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_1106__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_1107__4__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", at_821__5__DATA);

// Static initialiser table (.ctor)

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", __vt__12CPaintEffect__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editeff", __vt__11CStarEffect__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(EffectFlag, 0x4);
INCLUDE_BSS(EffectState, 0x4);
INCLUDE_BSS(PaintEffect, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(_StarEffect, 0x300);
INCLUDE_BSS(CurPartsBuff, 0x30);
INCLUDE_BSS(at_1037__6, 0x20);
INCLUDE_BSS(at_1112__3, 0x10);
INCLUDE_BSS(PlaceAnime, 0x1B0);
