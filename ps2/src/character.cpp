#include "common.h"
#include "character.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <libvu0.h>

#include "dataread.hpp"
#include "dynamicanime.hpp"
#include "effect.hpp"
#include "gameutil.hpp"
#include "mg_dataset.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_sprite.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "outline.hpp"
#include "scriptinterpreter.hpp"
#include "snd_mngr.hpp"
#include "swordeffect.hpp"
#include "visualmotion.hpp"

static CCharacter2       *nowChr; /**< Character whose info file is being read. */
static CCharacter2       *parent_chr; /**< Character whose outline texture is shared. */
static int                outline_flag; /**< Enables outlines while loading the info file. */
static int                now_motion_id; /**< Motion set receiving the current script entries. */
static CHRINFO_KEY_SET    *now_key_ptr; /**< Next key record in the motion list. */
static CHRINFO_SEQ_HEADER *now_seqhd_ptr; /**< Sequence header being filled. */
static CHRINFO_SEQ        *now_seq_ptr; /**< Next step record in the sequence. */
static int                now_cloth_id; /**< Dynamic animation receiving the cloth script. */
static int                outline_start; /**< Records whether the shared outline texture is ready. */
static mgCTexture        *outline_start_tex; /**< Texture shared by the character outlines. */
static int                outline_tex_id; /**< Texture identifier returned when entering an archive. */
static int                alloc_vertex_num; /**< Number of model frames whose vertices are allocated. */
static int                alloc_shadow_vertex_num; /**< Number of shadow frames whose vertices are allocated. */
static mgCMemory         *base_stack; /**< Memory for the base character model. */
static mgCMemory         *ext_stack; /**< Memory for extension motion sets. */
static mgCMemory         *img_stack; /**< Memory for character image archives. */
static mgCMemory         *now_stack; /**< Memory receiving the current script records. */
static int                set_imgblock; /**< Texture block receiving character images. */
static unsigned int      *pack_file; /**< Pack containing the current info file assets. */
static CHRINFO_SE        *now_se_header; /**< Next sound record in the motion sound list. */
static char               alloc_vertex[25][16]; /**< Names of model frames with allocated vertices. */
static mgIMG_FILE_HEADER *img_ptr[CHARA_IMAGE_MAX]; /**< Copied image archives awaiting entry. */

static void ScanInfoFile(CCharacter2 *character, unsigned int *pack, char *name, mgCMemory *model_stack, mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, CCharacter2 *parent, int outline);
static int _V2(SPI_STACK *stack, int count);
static int _NAME(SPI_STACK *stack, int count);
static int _BODY_SIZE(SPI_STACK *stack, int count);
static int _SCALE(SPI_STACK *stack, int count);
static int _MATERIAL_ANIME(SPI_STACK *stack, int count);
static int _POLY_NUM(SPI_STACK *stack, int count);
static int _IMG(SPI_STACK *stack, int count);
static int _IMG_END(SPI_STACK *stack, int count);
static int _OUTLINE(SPI_STACK *stack, int count);
static int _MODEL(SPI_STACK *stack, int count);
static int _SHADOW_MODEL(SPI_STACK *stack, int count);
static int _OBJECT_NAME(SPI_STACK *stack, int count);
static int _OBJECT_NAME2(SPI_STACK *stack, int count);
static int _MOTION(SPI_STACK *stack, int count);
static int _SHADOW_MOTION(SPI_STACK *stack, int count);
static int _VERTEX_ANIME(SPI_STACK *stack, int count);
static int _SHAPE_ANIME(SPI_STACK *stack, int count);
static int _KEY_START(SPI_STACK *stack, int count);
static int _KEY(SPI_STACK *stack, int count);
static int _KEY_END(SPI_STACK *stack, int count);
static int _SEQ_START(SPI_STACK *stack, int count);
static int _SEQ(SPI_STACK *stack, int count);
static int _SEQ_END(SPI_STACK *stack, int count);
static int _CLOTH_START(SPI_STACK *stack, int count);
static int _CLOTH(SPI_STACK *stack, int count);
static int _CLOTH_END(SPI_STACK *stack, int count);
static int _POSITION(SPI_STACK *stack, int count);
static int _ROTATION(SPI_STACK *stack, int count);
static int _SE_START(SPI_STACK *stack, int count);
static int _SE(SPI_STACK *stack, int count);
static int _SELP(SPI_STACK *stack, int count);
static int _SE_END(SPI_STACK *stack, int count);
static int _MOTION_END(SPI_STACK *stack, int count);
static int _EFFECT_START(SPI_STACK *stack, int count);
static int _EFFECT(SPI_STACK *stack, int count);
static int _EFFECT_END(SPI_STACK *stack, int count);
static int _LOD_MODEL_START(SPI_STACK *stack, int count);
static int _LOD_MODEL(SPI_STACK *stack, int count);
static int _LOD_MODEL_END(SPI_STACK *stack, int count);

static SPI_TAG_PARAM tag[] = { /**< Tags that build the character from its info file. */
    { "V2", _V2 },
    { "NAME", _NAME },
    { "IMG", _IMG },
    { "IMG_END", _IMG_END },
    { "BODY_SIZE", _BODY_SIZE },
    { "SCALE", _SCALE },
    { "MATERIAL_ANIME", _MATERIAL_ANIME },
    { "POLY_NUM", _POLY_NUM },
    { "MODEL", _MODEL },
    { "SHADOW_MODEL", _SHADOW_MODEL },
    { "OBJECT_NAME", _OBJECT_NAME },
    { "OBJECT_NAME2", _OBJECT_NAME2 },
    { "MOTION", _MOTION },
    { "SHADOW_MOTION", _SHADOW_MOTION },
    { "OUTLINE", _OUTLINE },
    { "VERTEX_ANIME", _VERTEX_ANIME },
    { "SHAPE_ANIME", _SHAPE_ANIME },
    { "KEY_START", _KEY_START },
    { "KEY", _KEY },
    { "KEY_END", _KEY_END },
    { "SEQ_START", _SEQ_START },
    { "SEQ", _SEQ },
    { "SEQ_END", _SEQ_END },
    { "CLOTH_START", _CLOTH_START },
    { "CLOTH", _CLOTH },
    { "CLOTH_END", _CLOTH_END },
    { "POSITION", _POSITION },
    { "ROTATION", _ROTATION },
    { "SE_START", _SE_START },
    { "SE", _SE },
    { "SELP", _SELP },
    { "SE_END", _SE_END },
    { "MOTION_END", _MOTION_END },
    { "EFFECT_START", _EFFECT_START },
    { "EFFECT", _EFFECT },
    { "EFFECT_END", _EFFECT_END },
    { "LOD_MODEL_START", _LOD_MODEL_START },
    { "LOD_MODEL", _LOD_MODEL },
    { "LOD_MODEL_END", _LOD_MODEL_END },
    { NULL, NULL }
};

static mgCFrame      *root_skin_frame; /**< Replacement hierarchy used to rebind the skin. */
static mgCFrame      *skin_frame; /**< Replacement visual frame used by shape animation. */
static char          *skin_name_ptr; /**< Main-model frame selected for replacement. */
static unsigned int  *eff_pack_ptr; /**< Pack containing the current effect definitions. */
static int            eff_pack_size; /**< Size in bytes of the current effect pack. */
static unsigned char *load_img_ptr; /**< Skin texture archive awaiting entry. */
static int            load_img_size; /**< Size in bytes of the skin texture archive. */
static char           skin_mds_name[64]; /**< Replacement model file named by the skin script. */

static void ScanInfoSkinFile(CCharacter2 *character, unsigned int *pack, char *name, char *skin_name, mgCMemory *memory, int image_block);
static int _SKIN_IMG(SPI_STACK *stack, int count);
static int _SKIN_IMG_END(SPI_STACK *stack, int count);
static int _SKIN_MODEL(SPI_STACK *stack, int count);
static mgCFrame *CreateChangeFrame(mgLoadData *load, mgCFrame *root);
static int _SKIN_MOTION(SPI_STACK *stack, int count);

static SPI_TAG_PARAM skin_tag[] = { /**< Tags that exchange the character's skin. */
    { "IMG", _SKIN_IMG },
    { "IMG_END", _SKIN_IMG_END },
    { "MODEL", _SKIN_MODEL },
    { "MOTION", _SKIN_MOTION },
    { NULL, NULL },
};

/**
 * Rounds a byte count up to 16-byte memory blocks.
 */
static inline u32 DynAnimeAlign16Blocks(u32 n) {
    if (n & 0xF) {
        return (n >> 4) + 1;
    }
    return n >> 4;
}

// Code (.text)
void CCharacter2::SetPosition(float *position) {
    mgCObject::SetPosition(position);
}

void CCharacter2::AddOutLine(char *frame_name, COutLineDraw *new_outline) {
    mgCFrame *target;
    COutLineDraw *current;
    COutLineDraw *last;
    COutLineDraw *q;
    if (frame_name != NULL) {
        target = CObjectFrame::frame;
        if (*frame_name != '\0') {
            target = target->SearchFrame(frame_name);
            if (target == NULL) {
                for (current = outline; current != NULL; current = current->next) {
                    target = current->frame->SearchFrame(frame_name);
                    if (target != NULL) {
                        break;
                    }
                }
            }
        }
        if (target != NULL) {
            if (target->parent != NULL) {
                mgCFrameAttr *attr = target->attr;
                if (attr != NULL) {
                    target-> attr->draw |= MG_FRAME_DRAW_SKIP_BY_PARENT;
                }
            }
            new_outline->SetFrame(target);
            last = outline;
            if (last == NULL) {
                outline = new_outline;
                return;
            }
            if (last != NULL) {
                do {
                    q = last->next;
                    if (q == NULL) {
                        break;
                    }
                    last = q;
                } while (q != NULL);
            }
            last->next = new_outline;
        }
    }
}

void CCharacter2::CopyOutLine(CCharacter2 *source) {
    COutLineDraw *current;
    mgCTexture *texture;

    if (source == NULL) {
        return;
    }
    if (source->outline == NULL) {
        return;
    }
    texture = source->outline->texture;
    if (texture == NULL) {
        return;
    }
    for (current = outline; current != NULL; current = current->next) {
        current->texture = texture;
    }
}

int CCharacter2::Draw() {
    sceVu0FVECTOR  world_position;
    float          draw_alpha;
    float          clip_alpha;
    int            count;
    int            i;

    if (!CheckDraw()) {
        return 0;
    }
    draw_alpha = alpha;
    if (!FarClip(mgGetDistFromCamera(position), &clip_alpha)) {
        return 0;
    }
    draw_alpha *= clip_alpha;
    if (CObjectFrame::frame != NULL) {
        CObjectFrame::frame->SetPosition(position);
        CObjectFrame::frame->SetRotation(rotation);
        CObjectFrame::frame->SetScale(scale);
        CObjectFrame::frame->GetWorldPosition0(world_position);
    }
    SetDeformMesh();
    count = 0;
    if (CObjectFrame::frame != NULL) {
        CObjectFrame::frame->SetAttrParamObjAlpha(draw_alpha, 1);
    }
    count += mgDraw(CObjectFrame::frame);
    for (i = 0; i < dynamic_anime_num; i++) {
        count += dynamic_anime[i].DrawSub(0);
    }
    return count;
}

void CCharacter2::SetDeformMesh() {
    sceVu0FVECTOR  max;
    sceVu0FVECTOR  min;
    int            i;

    for (i = 0; i < deform_frame_num; i++) {
        if (deform_frame[i] != NULL) {
            deform_frame[i]->RemakeBBox(max, min);
        }
    }
}

void CCharacter2::DrawStep() {
    float  clip_alpha;

    FarClip(GetCameraDist(), &clip_alpha);
}

float CCharacter2::GetCameraDist() {
    sceVu0FVECTOR  head_position;
    sceVu0FVECTOR  camera_position;
    sceVu0FVECTOR  closest_position;
    float          height;

    height = body_height;
    if (height < 1.0f) {
        height = 34.0f;
    }
    *(u_long128 *)head_position = *(u_long128 *)position;
    head_position[1] += height;
    mgGetCameraPos(camera_position);
    return mgDistLinePoint(camera_position, position, head_position, closest_position);
}

#ifdef NONMATCHING
int CCharacter2::DrawDirect() {
    sceVu0FVECTOR  world_position;
    sceVu0FVECTOR  outline_position;
    sceVu0FVECTOR  view_position;
    mgCFrame      *lod_frame;
    COutLineDraw  *current;
    COutLineDraw  *draw_outline;
    float          clip_alpha;
    float          draw_alpha;
    float          distance;
    float          outline_scale;
    int            level;
    int            count;
    int            i;

    draw_alpha = alpha;
    distance = GetCameraDist();
    if (!FarClip(distance, &clip_alpha)) {
        return 0;
    }
    draw_alpha *= clip_alpha;
    if (!CheckDraw()) {
        return 0;
    }
    if (CObjectFrame::frame != NULL) {
        CObjectFrame::frame->SetPosition(position);
        CObjectFrame::frame->SetRotation(rotation);
        CObjectFrame::frame->SetScale(scale);
        CObjectFrame::frame->GetWorldPosition0(world_position);
    }
    level = 0;
    for (i = 0; i < lod_num - 1; i++) {
        if (lod[i].distance < distance) {
            level = i + 1;
        }
    }
    if (level >= lod_num) {
        level = lod_num - 1;
    }
    lod_frame = ChangeLOD(level);
    draw_outline = outline;
    if (lod_frame != NULL && draw_outline == NULL) {
        return mgDrawDirect(lod_frame);
    }
    SetDeformMesh();
    count = 0;
    if (draw_outline != NULL) {
        COutLineDraw lod_outline = *draw_outline;

        if (lod_frame != NULL) {
            lod_outline.next = NULL;
            lod_outline.SetFrame(lod_frame);
            draw_outline = &lod_outline;
        }
        outline_scale = 1.0f;
        if (entry_frame[0] != NULL) {
            GetEntryObjectPos(0, outline_position);
        } else {
            *(u_long128 *)outline_position = *(u_long128 *)world_position;
            outline_position[3] = 1.0f;
        }
        mgTransWorldView(view_position, outline_position);
        if (!(view_position[2] <= 10.0f)) {
            outline_scale = 1.0f - (view_position[2] - 10.0f) / 300.0f;
            if (outline_scale < 0.0f) {
                outline_scale = 0.0f;
            }
        }
        if (draw_alpha < 1.0f) {
            for (current = outline; current != NULL; current = current->next) {
                if (current->frame != NULL && current->frame->attr != NULL) {
                    current->frame->attr->draw &= ~MG_FRAME_DRAW_SKIP_BY_PARENT;
                }
            }
            current = outline;
            count += current->Draw(position, outline_scale, draw_alpha);
            for (; current != NULL; current = current->next) {
                if (current->frame != NULL && current->frame->attr != NULL) {
                    current->frame->attr->draw |= MG_FRAME_DRAW_SKIP_BY_PARENT;
                }
            }
        } else {
            while (draw_outline != NULL) {
                count += draw_outline->Draw(position, outline_scale, draw_alpha);
                draw_outline = draw_outline->next;
            }
        }
    } else {
        if (CObjectFrame::frame != NULL) {
            CObjectFrame::frame->SetAttrParamObjAlpha(draw_alpha, 1);
        }
        count += mgDrawDirect(CObjectFrame::frame);
    }
    for (i = 0; i < dynamic_anime_num; i++) {
        count += dynamic_anime[i].DrawSub(1);
    }
    return count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", DrawDirect__11CCharacter2Fv);
#endif

int CCharacter2::DrawShadowDirect() {
    if (!show) {
        return 0;
    }
    if (!CheckDraw()) {
        return 0;
    }
    if (shadow_frame == NULL) {
        return 0;
    }
    CCharacter2::ShadowStep();
    shadow_frame->SetPosition(position);
    shadow_frame->SetRotation(rotation);
    shadow_frame->SetScale(scale);
    if (now_set >= 0 && now_set < CHARA_MOTION_SET_MAX) {
        DeformMesh(shadow_frame, &shadow_motion[now_set], shadow_frame_info, false);
    }
    return mgDrawDirect(shadow_frame);
}

void CCharacter2::UpdatePosition() {
    if (CObjectFrame::frame != NULL) {
        CObjectFrame::frame->SetPosition(position);
        CObjectFrame::frame->SetRotation(rotation);
        CObjectFrame::frame->SetScale(scale);
    }
    if (shadow_frame != NULL) {
        shadow_frame->SetPosition(position);
        shadow_frame->SetRotation(rotation);
        shadow_frame->SetScale(scale);
    }
}

void CCharacter2::ResetDAPosition() {
    int  i;

    UpdatePosition();
    if (dynamic_anime_num == 0 || dynamic_anime == NULL) {
        return;
    }
    for (i = 0; i < dynamic_anime_num; i++) {
        dynamic_anime[i].ResetPosition();
    }
}

float CCharacter2::GetDefaultStep() {
    if (now_key != NULL) {
        return now_key->step;
    }
    return 0.0f;
}

void CCharacter2::SetStep(float frame_step) {
    step = frame_step;
}

void CCharacter2::ResetMotion() {
    now_key = NULL;
    next_key = NULL;
    now_seq = NULL;
    next_seq = NULL;
}

float CCharacter2::GetChgStepWait() {
    if (motion_status != CHARA_MOTION_STATUS_BLEND) {
        return -1.0f;
    }
    return blend;
}

int CCharacter2::CheckMotionEnd() {
    float  end_frame;

    if (now_key == NULL) {
        return 1;
    }
    float margin = 1.2f * step;
    if (frame <= now_key->end_frame && frame + 2.0f * margin >= now_key->end_frame) {
        return 1;
    }
    return 0;
}

void CCharacter2::SetMotion(int no, int flags) {
    CHRINFO_KEY_SET *key;
    int              set;

    key = GetKeyListIndexPtr(no, &set);
    if (key != NULL) {
        next_key = key;
        next_flags = flags;
        next_set = set;
        blend_speed = 0.2f;
        seq_mode = 0;
        now_seq = NULL;
        seq_state = CHARA_SEQ_STATE_NONE;
    }
}

void CCharacter2::SetMotion(char *name, int flags) {
    SetMotionPara(name, flags, -1);
}

void CCharacter2::SetNowFrameWeight(float weight) {
    float range;

    if (now_key != NULL) {
        if (weight < 0.0f) {
            weight = 0.0f;
        }
        if (weight > 1.0f) {
            weight = 1.0f;
        }
        range = (float)(now_key->end_frame - now_key->start_frame);
        SetNowFrame((float)now_key->start_frame + range * weight);
    }
}

#ifdef NONMATCHING
void CCharacter2::SetMotionPara(char *name, int flags, int keep_seq) {
    CHRINFO_KEY_SET    *key;
    CHRINFO_SEQ_HEADER *sequence;
    int                 set;

    key = GetKeyListPtr(name, &set);
    if (key != NULL) {
        next_key = key;
        next_flags = flags;
        next_set = set;
        blend_speed = 0.2f;
        if (keep_seq == -1 || keep_seq == 0) {
            seq_mode = 0;
            now_seq = NULL;
            seq_state = CHARA_SEQ_STATE_NONE;
        }
    } else {
        sequence = GetSeqHeaderPtr(name, &set);
        if (sequence != NULL) {
            next_seq = sequence;
            seq_mode = 1;
            seq_flags = flags;
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", SetMotionPara__11CCharacter2FPcii);
#endif

void CCharacter2::SetDAnimeEnable(int enable) {
    if (enable != 0) {
        dynamic_anime_flags &= ~CHARA_DYNAMIC_ANIME_DISABLE;
    } else {
        dynamic_anime_flags |= CHARA_DYNAMIC_ANIME_DISABLE;
    }
}

CHRINFO_SE *CCharacter2::GetSoundInfoCopy(mgCMemory *memory) {
    u32 size;
    u32 quadwords;
    void *sounds;
    void *copy;

    if (se_num[0] <= 0) {
        return NULL;
    }
    size = se_num[0] * sizeof(CHRINFO_SE);
    if (size & 0xF) {
        quadwords = (size >> 4) + 1;
    } else {
        quadwords = size >> 4;
    }
    sounds = memory->Alloc(quadwords + 2);
    copy = operator new[](se_num[0] * (int)sizeof(CHRINFO_SE), (u_long128 *)sounds);
    memcpy(copy, se_list[0], se_num[0] * sizeof(CHRINFO_SE));
    return (CHRINFO_SE *)copy;
}

int CCharacter2::CheckFootEffect() {
    if (foot_effect_wait <= 0) {
        return -1;
    }
    return foot_sound_id;
}

void CCharacter2::SePlay() {
    CHRINFO_SE *sound;
    float position[4];
    float last_frame;
    float first_frame;
    float high;
    int index;

    if (foot_effect_wait > 0) {
        foot_effect_wait--;
    }
    float now = frame;
    last_frame = 1.6f * (1.2f * step);
    sound = (CHRINFO_SE *)se_list[now_set];
    first_frame = now - last_frame;
    high = now + last_frame;
    if (sound == NULL) {
        return;
    }
    se_volume = 1.0f;
    se_pan = 0.0f;
    if (se_positional == 1) {
        GetEntryObjectPos(0, position);
        float far_dist = 1200.0f;
        float near_dist = 160.0f;
        sndGetVolPan(&se_volume, &se_pan, position, 160.0f, 1200.0f);
    }
    for (index = 0; index < se_num[now_set]; sound++, index++) {
        if (sound->loop_slot > 0) {
            if (frame >= sound->frame && frame <= sound->end_frame) {
                if (sound->kind == CHRINFO_SE_SOUND && loop_se != NULL) {
                    loop_se->SeLoopPlayStop(se_bank, sound->se_no, sound->loop_slot, 13);
                }
                if (sound->kind == CHRINFO_SE_SOUND_2 && loop_se != NULL) {
                    loop_se->SeLoopPlayStop(se_bank_2, sound->se_no, sound->loop_slot, 13);
                }
            }
            sound->wait = 0;
        } else if (first_frame < sound->frame && high > sound->frame && sound->wait == 0) {
            if (sound->kind < CHRINFO_SE_SOUND && foot_sound_enable != 0) {
                if (foot_sound_id >= 0) {
                    sndSePlayVPf(foot_se_bank, sound->kind + foot_sound_id * 2, se_volume, se_pan, 0);
                }
                foot_effect_wait = 1;
                sound->wait = 6;
            }
            if (sound->kind == CHRINFO_SE_SOUND) {
                sndSePlayVPf(se_bank, sound->se_no, se_volume, se_pan, 0);
                sound->wait = 6;
            }
            if (sound->kind == CHRINFO_SE_SOUND_FOOT) {
                sndSePlayVPf(se_bank, sound->se_no, se_volume, se_pan, 0);
                sound->wait = 6;
                foot_effect_wait = 1;
            }
            if (sound->kind == CHRINFO_SE_SOUND_2) {
                sndSePlayVPf(se_bank_2, sound->se_no, se_volume, se_pan, 0);
                sound->wait = 6;
            }
        }
        if (sound->wait > 0) {
            sound->wait--;
        }
    }
}

void CCharacter2::Step() {
    sceVu0FMATRIX  matrix;
    float          angle;
    int            flags;
    int            advance;
    int            status;
    int            reset;

    if (CheckDraw() == 0) {
        return;
    }
    UpdatePosition();
    if (motion_enable == 0) {
        return;
    }
    SePlay();
    if (seq_mode == 0) {
        NormalDrive();
    }
    if (seq_mode == 1) {
        if (next_seq != now_seq) {
            now_seq = next_seq;
            seq_step = NULL;
            seq_loop = 0;
            seq_advance = 0;
            seq_state = CHARA_SEQ_STATE_NONE;
            if (now_seq != NULL) {
                seq_step = now_seq->seq;
                if (seq_step != NULL) {
                    seq_state = CHARA_SEQ_STATE_START;
                    flags = seq_flags;
                    switch (seq_step->type) {
                        case 1:
                            seq_loop = seq_step->loop_count;
                            break;
                        default:
                        case 0:
                        case 2:
                            flags |= CHARA_MOTION_HOLD;
                            break;
                        case 3:
                        case 7:
                            break;
                    }
                    SetMotionPara(seq_step->name, flags, 1);
                    blend_speed = seq_step->blend_speed;
                    if (blend_speed >= 1.0f) {
                        blend = 1.0f;
                    }
                    NormalDrive();
                    return;
                }
            }
        }
        if (now_seq == NULL) {
            return;
        }
        if (seq_step == NULL) {
            return;
        }
        seq_state = CHARA_SEQ_STATE_PLAY;
        advance = 0;
        status = GetMotionStatus();
        switch (seq_step->type) {
            case CHRINFO_SEQ_ONCE:
                if (status == CHARA_MOTION_STATUS_END) {
                    advance = 1;
                }
                break;
            case CHRINFO_SEQ_LOOP:
                if (status == CHARA_MOTION_STATUS_END) {
                    seq_loop--;
                    if (seq_loop <= 0) {
                        advance = 1;
                    }
                }
                break;
            case CHRINFO_SEQ_HOLD_WAIT:
            case CHRINFO_SEQ_LOOP_WAIT:
                if (status == 4) {
                    seq_state = CHARA_SEQ_STATE_WAIT;
                    if (seq_advance != 0) {
                        advance = 1;
                    }
                }
                break;
            case CHRINFO_SEQ_WAIT:
                seq_state = CHARA_SEQ_STATE_WAIT;
                if (seq_advance != 0) {
                    advance = 1;
                }
                break;
        }
        if (advance != 0) {
            if (seq_step[1].name[0] == '\0') {
                seq_state = CHARA_SEQ_STATE_END;
                return;
            }
            seq_step++;
            seq_loop = 0;
            seq_advance = 0;
            if (seq_step != NULL) {
                flags = 0;
                switch (seq_step->type) {
                    case 1:
                        seq_loop = seq_step->loop_count;
                        break;
                    case 0:
                    case 2:
                        flags = CHARA_MOTION_HOLD;
                        break;
                    case 3:
                    case 7:
                        break;
                }
                SetMotionPara(seq_step->name, flags, 1);
                blend_speed = seq_step->blend_speed;
                if (blend_speed >= 1.0f) {
                    blend = 1.0f;
                }
            }
        }
        NormalDrive();
    }
    reset = 0;
    GetEntryObjectPos(0, matrix);
    if (!(mgDistVector(matrix[3], entry_matrix[3]) <= 20.0f)) {
        reset = 1;
    }
    if (reset == 0) {
        angle = atan2f(matrix[2][0], matrix[2][2]);
        if (mgAngleCmp(angle, atan2f(entry_matrix[2][0], entry_matrix[2][2]), 0.7853982f) != 0) {
            reset = 1;
        }
    }
    *(u_long128 *)entry_matrix[0] = *(u_long128 *)matrix[0];
    *(u_long128 *)entry_matrix[1] = *(u_long128 *)matrix[1];
    *(u_long128 *)entry_matrix[2] = *(u_long128 *)matrix[2];
    *(u_long128 *)entry_matrix[3] = *(u_long128 *)matrix[3];
    if (reset != 0) {
        ResetDAPosition();
    }
    StepDA(1);
    CtrlEffect();
}

void CCharacter2::StepDA(int count) {
    int index;
    int iteration;

    if ((dynamic_anime_flags & CHARA_DYNAMIC_ANIME_DISABLE) != 0) {
        return;
    }
    if (dynamic_anime_num == 0 || dynamic_anime == NULL) {
        return;
    }
    for (index = 0; index < dynamic_anime_num; index++) {
        if (count < 0) {
            dynamic_anime[index].ResetPosition();
        } else {
            for (iteration = 0; iteration < count; iteration++) {
                dynamic_anime[index].Step();
            }
        }
    }
}

void CCharacter2::SetWind(float power, float *dir) {
    int  index;

    for (index = 0; index < dynamic_anime_num; index++) {
        dynamic_anime[index].SetWind(power, dir);
    }
}

void CCharacter2::ResetWind() {
    int  index;

    for (index = 0; index < dynamic_anime_num; index++) {
        dynamic_anime[index].ResetWind();
    }
}

void CCharacter2::SetFloor(float y) {
    int  index;

    for (index = 0; index < dynamic_anime_num; index++) {
        dynamic_anime[index].SetFloor(y);
    }
}

void CCharacter2::ResetFloor() {
    int  index;

    for (index = 0; index < dynamic_anime_num; index++) {
        dynamic_anime[index].ResetFloor();
    }
}

#ifdef NONMATCHING
void CCharacter2::NormalDrive() {
    float  frame_step;

    if (next_key != now_key && next_key != NULL) {
        posed_key = now_key;
        prev_flags = now_flags;
        prev_set = now_set;
        prev_frame = frame;
        now_key = next_key;
        step = now_key->step;
        now_set = next_set;
        now_flags = next_flags;
        blend = 0.1f;
        motion_status = CHARA_MOTION_STATUS_START;
        if ((next_flags & CHARA_MOTION_RESTART) != 0) {
            posed_key = next_key;
            frame = now_key->start_frame;
        }
        ExecEntryEffect(now_key);
    } else if (now_flags != next_flags && next_key != NULL) {
        now_flags = next_flags;
        if ((next_flags & CHARA_MOTION_RESTART) != 0) {
            motion_status = CHARA_MOTION_STATUS_START;
            posed_key = next_key;
            frame = now_key->start_frame;
        }
    }
    if (now_key == NULL) {
        return;
    }
    if (posed_key == now_key) {
        frame_step = step * 1.2f;
        if ((now_flags & CHARA_MOTION_PAUSE) != 0) {
            frame_step = 0.0f;
        }
        frame += frame_step;
        if (!(frame_step <= 0.0f)) {
            motion_status = CHARA_MOTION_STATUS_PLAY;
        }
        if (frame < now_key->start_frame + step * 1.2f && !(frame < now_key->start_frame)) {
            frame = now_key->start_frame;
            motion_status = CHARA_MOTION_STATUS_START;
        }
        if (!(frame + step * 1.2f <= now_key->end_frame)) {
            if ((now_flags & CHARA_MOTION_HOLD) != 0) {
                frame = now_key->end_frame;
                motion_status = CHARA_MOTION_STATUS_END;
                step = 0.0f;
            } else {
                frame = now_key->start_frame;
                motion_status = CHARA_MOTION_STATUS_END;
                ExecEntryEffect(now_key);
            }
        }
    }
    if (posed_key == now_key) {
        frame_ratio = now_key->end_frame - now_key->start_frame;
        frame_ratio = (frame - now_key->start_frame) / frame_ratio;
        SetMotionTime(CObjectFrame::frame, &motion[now_set], frame, NULL);
        return;
    }
    if (!(blend < 1.0f)) {
        posed_key = now_key;
        frame = now_key->start_frame;
        motion_status = CHARA_MOTION_STATUS_START;
        return;
    }
    frame_ratio = 0.0f;
    motion_status = CHARA_MOTION_STATUS_BLEND;
    ChangeMotion(CObjectFrame::frame, &motion[now_set], (int)(0.9f + frame), now_key->start_frame, blend, NULL);
    blend += blend_speed;
    if (!(blend < 1.0f)) {
        posed_key = now_key;
        frame = now_key->start_frame;
        motion_status = CHARA_MOTION_STATUS_START;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", NormalDrive__11CCharacter2Fv);
#endif

void CCharacter2::ShadowStep() {
    int index;
    mgCFrame *source;
    mgCFrame *model;
    mgCFrame *shadow;
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR scale;

    if (shadow_frame != NULL && CheckDraw() != 0 && motion_enable != 0 && now_key != NULL) {
        model = CObjectFrame::frame;
        for (index = 0; index < shadow_link_num; index++) {
            source = model->GetFrame(shadow_link_model[index]);
            if (source != NULL) {
                shadow = shadow_frame->GetFrame(shadow_link_shadow[index]);
                if (shadow != NULL) {
                    source->GetScale(scale);
                    sceVu0CopyMatrix(matrix, source->trans_matrix);
                    shadow->SetTransMatrix(matrix);
                    shadow->SetScale(scale);
                }
            }
        }
    }
}

CHRINFO_KEY_SET *CCharacter2::GetKeyListIndexPtr(int no, int *out_set) {
    int number;
    int set;
    int motion_no;
    CHRINFO_KEY_SET *key;

    number = 0;
    set = 0;
    do {
        key = key_list[set];
        if (key != NULL) {
            for (motion_no = 0; motion_no < key_num[set]; motion_no++) {
                if (key->name[0] == 0) {
                    break;
                }
                if (number == no) {
                    if (out_set != NULL) {
                        *out_set = set;
                    }
                    return key;
                }
                key++;
                number++;
            }
        }
        set++;
    } while (set < 8);
    return NULL;
}

CHRINFO_KEY_SET *CCharacter2::GetKeyListPtr(char *name, int *out_set) {
    CHRINFO_KEY_SET *key;
    int set = 0;
    int index;

    do {
        key = key_list[set];
        if (key != NULL) {
            for (index = 0; index < key_num[set]; index++) {
                if (key->name[0] == 0) {
                    break;
                }
                if (strcmp(key->name, name) == 0) {
                    if (out_set != NULL) {
                        *out_set = set;
                    }
                    return key;
                }
                key++;
            }
        }
        set++;
    } while (set < 8);
    return 0;
}

CHRINFO_SEQ_HEADER *CCharacter2::GetSeqHeaderPtr(char *name, int *out_set) {
    CHRINFO_SEQ_HEADER *sequence;
    int set = 0;

    do {
        sequence = seq_list[set];
        if (sequence != 0) {
            while (sequence != 0) {
                if (strcmp(sequence->name, name) == 0) {
                    if (out_set != NULL) {
                        *out_set = set;
                    }
                    return sequence;
                }
                sequence = sequence->next;
            }
        }
        set++;
    } while (set < 8);
    return 0;
}

#ifdef NONMATCHING
void CCharacter2::DeleteExtMotion() {
    CHRINFO_KEY_SET   *key;
    mgIMG_FILE_HEADER *image;
    mgIMG_HEADER      *texture;
    unsigned int       texture_index;
    int                set;
    int                group;
    int                archive;

    for (set = 1; set < CHARA_MOTION_SET_MAX; set++) {
        motion[set].frame_info = NULL;
        shadow_motion[set].frame_info = NULL;
        key_list[set] = NULL;
        key_num[set] = 0;
        seq_list[set] = NULL;
        key = GetKeyListIndexPtr(0, NULL);
        if (key != NULL) {
            SetMotion(key->name, CHARA_MOTION_RESTART);
        }
        if (next_key != NULL) {
            posed_key = next_key;
            frame = next_key->start_frame;
        }
    }
    if (tex_anime_group_start > 0) {
        for (group = tex_anime_group_start; group < tex_anime_group_num; group++) {
            mgTexManager.DeleteTexAnimeGroup(texture_block, group);
        }
    } else {
        mgTexManager.DeleteTexAnime(texture_block);
    }
    for (archive = 1; archive < CHARA_IMAGE_MAX; archive++) {
        image = images[archive];
        if (image != NULL) {
            texture = (mgIMG_HEADER *)(image + 1);
            for (texture_index = 0; texture_index < image->num3; texture_index++, texture++) {
                if (texture->name[0] != '#') {
                    mgTexManager.DeleteTexture(texture->name, texture_block);
                }
            }
            images[archive] = NULL;
        }
    }
    for (set = 1; set < CHARA_MOTION_SET_MAX; set++) {
        se_list[set] = NULL;
        se_num[set] = 0;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", DeleteExtMotion__11CCharacter2Fv);
#endif

void CCharacter2::DeleteImage() {
    mgCTextureManager *tex = &mgTexManager;
    int group_num;
    int group;
    mgIMG_HEADER *texture;
    u32 i;
    mgIMG_FILE_HEADER *image;

    mgTexManager.GetGroupNameList(texture_block, &group_num);
    if (tex_anime_group_start > 0) {
        for (group = tex_anime_group_start ; group < tex_anime_group_num; group++) {
            tex->DeleteTexAnimeGroup(texture_block, group);
        }
    }
    image = images[0];

    texture = (mgIMG_HEADER *)(image + 1);
    if (image != NULL) {
        i = 0;
        while (i < image->num3) {

            if (*(u8 *)texture->name != '#') {
                tex->DeleteTexture(texture->name, texture_block);
            }
            texture++;
            i += 1;
        }
        images[0] = NULL;
    }
}

int CCharacter2::GetEntryObjectPos(int no, float *out_position) {
    mgCFrame *entry;

    GetPosition(out_position);
    if (no < 0 || no > 2) {
        return 0;
    }
    entry = entry_frame[no];
    if (entry == NULL) {
        return 0;
    }
    entry->GetWorldPosition0(out_position);
    return 1;
}

int CCharacter2::GetEntryObjectPos(int no, float (*out_matrix)[4]) {
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  rotation;
    mgCFrame      *entry;

    if (no < 0 || no > 2) {
        no = 0;
    }
    entry = entry_frame[no];
    if (entry == NULL) {
        GetPosition(position);
        GetRotation(rotation);
        mgCreateMatrixPY(out_matrix, position, rotation[1]);
        return 0;
    }
    entry->GetLWMatrix(out_matrix);
    return 1;
}

CHARA_ENTRY_OBJECT *CCharacter2::GetEntryObjectPos(int group, int no, float *out_position) {
    int group_index;
    int index;
    int offset;

    GetPosition(out_position);
    if (no < 0 || no > 0x18) {
        return NULL;
    }
    group_index = -1;
    for (index = 0; index < CHARA_ENTRY_OBJECT_MAX; index++) {
        if (entry_object[index].frame != NULL && entry_object[index].group == group) {
            group_index++;
        }
        if (group_index == no) {
            entry_object[index].frame->GetWorldPosition0(out_position);
            return &entry_object[index];
        }
    }
    return NULL;
}

float CCharacter2::GetWaitToFrame(char *name, float ratio) {
    float result = 0.0f;

    CHRINFO_KEY_SET *key = nowChr->GetKeyListPtr(name, NULL);
    if (key != NULL) {
        result = key->start_frame + ratio * (key->end_frame - (float)key->start_frame);
    }
    return result;
}

void CCharacter2::LoadSkin(unsigned int *pack, char *name, char *skin_name, mgCMemory *stack, int image_block) {
    ScanInfoSkinFile(this, pack, name, skin_name, stack, image_block);
}

void CCharacter2::LoadPack(unsigned int *pack, char *name, mgCMemory *model_stack, mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, CCharacter2 *parent) {
    LoadChrFile(pack, name, model_stack, motion_stack, image_stack, image_block, parent, 1);
}

void CCharacter2::LoadPackNoLine(unsigned int *pack, char *name, mgCMemory *model_stack, mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, CCharacter2 *parent) {
    LoadChrFile(pack, name, model_stack, motion_stack, image_stack, image_block, parent, 0);
}

void CCharacter2::LoadChrFile(unsigned int *pack, char *name, mgCMemory *model_stack, mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, CCharacter2 *parent, int outline) {
    CHRINFO_KEY_SET *first_key;
    CHRINFO_KEY_SET *key;

    first_key = GetKeyListIndexPtr(0, NULL);
    ScanInfoFile(this, pack, name, model_stack, motion_stack, image_stack, image_block, parent, outline);
    if (first_key == NULL) {
        key = GetKeyListIndexPtr(0, NULL);
        if (key != NULL) {
            SetMotion(key->name, CHARA_MOTION_RESTART);
        }
        if (next_key != NULL) {
            posed_key = next_key;
            frame = next_key->start_frame;
            step = next_key->step;
        }
    }
}

void CCharacter2::Initialize() {
    int j;
    int index;

    CObjectFrame::Initialize();
    velocity[2] = 0.0f;
    velocity[1] = 0.0f;
    velocity[0] = 0.0f;
    velocity[3] = 1.0f;
    base_scale[3] = 1.0f;
    base_scale[2] = 1.0f;
    base_scale[1] = 1.0f;
    base_scale[0] = 1.0f;
    move_accel = 0;
    alpha = 1.0f;
    mgUnitMatrix(entry_matrix);
    CObjectFrame::frame = NULL;
    load_size = 0;
    copy_size = 0;
    poly_num[1] = 0;
    poly_num[0] = 0;
    dynamic_anime_flags = 0;
    this->outline = NULL;
    shadow_frame = NULL;
    frame = 0.0f;
    unk_500 = 0;
    motion_status = CHARA_MOTION_STATUS_NONE;
    shape_anime = 0;
    foot_sound_id = -1;
    foot_sound_enable = 1;
    se_positional = 1;
    se_volume = 1.0f;
    se_pan = 0.0f;
    foot_effect_wait = -1;
    loop_se = NULL;
    for (index = 0; index < CHARA_MOTION_SET_MAX; index++) {
        se_list[index] = NULL;
        se_num[index] = 0;
    }
    sword_effect[0] = NULL;
    sword_effect[1] = NULL;
    sword_effect[2] = NULL;
    dynamic_anime_num = 0;
    dynamic_anime = NULL;
    for (index = 0; index < CHARA_IMAGE_MAX; index++) {
        images[index] = NULL;
    }
    tex_anime_group_num = 0;
    tex_anime_group_start = 0;
    entry_frame[0] = NULL;
    entry_frame[1] = NULL;
    for (index = 0; index < CHARA_ENTRY_OBJECT_MAX; index++) {
        entry_object[index].frame = NULL;
        entry_object[index].unk_04 = 0.0f;
        entry_object[index].group = -1;
        entry_object[index].enable = 0;
    }
    for (j = 0; j < CHARA_DEFORM_FRAME_MAX; j++) {
        deform_frame[j] = NULL;
    }
    deform_frame_num = 0;
    for (index = 0; index < CHARA_MOTION_SET_MAX; index++) {
        key_list[index] = NULL;
        key_num[index] = 0;
        seq_list[index] = NULL;
    }
    memset(motion, 0, sizeof(motion));
    memset(shadow_motion, 0, sizeof(shadow_motion));
    now_set = 0;
    next_set = 0;
    next_key = NULL;
    now_key = NULL;
    posed_key = NULL;
    next_seq = NULL;
    now_seq = NULL;
    seq_step = NULL;
    unk_500 = 0;
    shadow_frame_info = NULL;
    lod_num = 0;
    lod = NULL;
    lod_no = -1;
    motion_enable = 1;
    shadow_link_num = 0;
    shadow_link_shadow = NULL;
    shadow_link_model = NULL;
    InitEffect();
}

/**
 * Builds a character by running its tagged info file from the pack.
 */
static void ScanInfoFile(CCharacter2 *character, unsigned int *pack, char *name, mgCMemory *model_stack,
                  mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, CCharacter2 *parent,
                  int outline) {
    int script_size;
    char *script;
    int free_blocks = model_stack->stack_size - model_stack->stack_used;
    CScriptInterpreter interpreter;

    ext_stack = motion_stack;
    img_ptr[2] = NULL;
    outline_flag = outline;
    img_ptr[3] = NULL;
    pack_file = pack;
    img_ptr[4] = NULL;
    img_stack = image_stack;
    set_imgblock = image_block;
    parent_chr = parent;
    base_stack = model_stack;
    nowChr = character;
    now_seq_ptr = NULL;
    now_seqhd_ptr = NULL;
    now_key_ptr = NULL;
    alloc_vertex_num = 0;
    alloc_shadow_vertex_num = 0;
    now_cloth_id = 0;
    outline_start = 0;
    outline_start_tex = NULL;
    for (int i = 0; i < CHARA_IMAGE_MAX; i++) {
        img_ptr[i] = NULL;
    }
    script = (char *)GetPackFile(pack, name, &script_size);
    if (script == NULL) {
        printf("not found %s\n", name);
        return;
    }
    interpreter.SetTag(tag);
    interpreter.SetScript(script, script_size);
    now_motion_id = 0;
    now_stack = model_stack;
    interpreter.Run();
    free_blocks -= model_stack->stack_size - model_stack->stack_used;
    character->load_size = free_blocks;
}

/**
 * Accepts the version marker of a character info file.
 */
static int _V2(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Accepts the name marker of a character info file.
 */
static int _NAME(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Reads the character's height, width and depth.
 */
static int _BODY_SIZE(SPI_STACK *stack, int count) {
    if (nowChr == NULL) {
        return 0;
    }
    nowChr->body_height = spiGetStackInt(stack++);
    nowChr->body_width = spiGetStackInt(stack++);
    nowChr->body_depth = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the base scale of the character and its shadow.
 */
static int _SCALE(SPI_STACK *stack, int count) {
    if (nowChr == NULL) {
        return 0;
    }
    nowChr->base_scale[0] = spiGetStackFloat(stack++);
    nowChr->base_scale[1] = spiGetStackFloat(stack++);
    nowChr->base_scale[2] = spiGetStackFloat(stack);
    nowChr->base_scale[3] = 1.0f;
    if (nowChr->CObjectFrame::frame != NULL) {
        nowChr->SetScale(nowChr->base_scale);
    }
    if (nowChr->shadow_frame != NULL) {
        nowChr->shadow_frame->SetScale(nowChr->base_scale);
    }
    return 1;
}

/**
 * Accepts the material animation marker of a character info file.
 */
static int _MATERIAL_ANIME(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Records the polygon counts of the character's models.
 */
static int _POLY_NUM(SPI_STACK *stack, int count) {
    nowChr->poly_num[0] = 0;
    nowChr->poly_num[1] = 0;
    if (count > 0) {
        nowChr->poly_num[0] += spiGetStackInt(stack++);
    }
    if (count == 2) {
        nowChr->poly_num[1] += spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Copies an image archive from the pack into image memory.
 */
static int _IMG(SPI_STACK *stack, int count) {
    int   index;
    int   size;
    void *data;

    if (img_stack == NULL) {
        return 0;
    }
    index = spiGetStackInt(stack++);
    if (index < 0 || index >= CHARA_IMAGE_MAX) {
        return 0;
    }
    data = GetPackFile(pack_file, spiGetStackString(stack), &size);
    if (data == NULL) {
        return 0;
    }
    img_ptr[index] = (mgIMG_FILE_HEADER *)img_stack->stAlloc64(size / 16 + 1);
    memcpy(img_ptr[index], data, size);
    return 1;
}

/**
 * Enters the copied image archives and remembers their animation groups.
 */
static int _IMG_END(SPI_STACK *stack, int count) {
    int index;
    mgCTextureManager *tex;
    char **groups;

    tex = &mgTexManager;
    nowChr->texture_block = set_imgblock;
    for (index = 0; index < CHARA_IMAGE_MAX; index++) {
        if (img_ptr[index] != NULL) {
            nowChr->images[index] = img_ptr[index];
            outline_tex_id = tex->EnterIMGFile((u8 *)img_ptr[index], set_imgblock, img_stack, NULL);
            if (index == 0) {
                groups = tex->GetGroupNameList(set_imgblock, &nowChr->tex_anime_group_num);
                if (groups != NULL) {
                    nowChr->tex_anime_group_start = 0;
                    while (groups[nowChr->tex_anime_group_start] != NULL) {
                        nowChr->tex_anime_group_start ++;
                    }
                }
            }
        }
    }
    return 1;
}

/**
 * Attaches an outline to a named frame, sharing its screen texture.
 */
static int _OUTLINE(SPI_STACK *stack, int count) {
    char frame_name[64];
    char texture_name[32];
    SPI_STACK *arg = stack + 1;
    float width;
    mgCTextureManager *tex_manager;
    COutLineDraw *outline;
    mgCTexture *texture;

    if (outline_flag == 0) {
        return 1;
    }
    strcpy(frame_name, spiGetStackString(stack++ ));
    width = spiGetStackFloat(stack);
    tex_manager = &mgTexManager;
    if (&mgTexManager == NULL || set_imgblock == -1) {
        return 0;
    }
    if ((outline = (COutLineDraw *)operator new(0x70, (u_long128 *)base_stack->Alloc(9))) != 0) {
        outline->next = 0;
        outline->Initialize();
    }
    outline->Initialize();
    outline->width = width;
    if (outline_start == 0) {
        if (parent_chr != NULL) {
            if (parent_chr->outline == NULL) {
                return 0;
            }
            if ((outline_start_tex = parent_chr->outline->texture) == NULL) {
                return 0;
            }
        } else {
            static int outline_num = 1;
            outline_num += 1;
            sprintf(texture_name, "out_line2%d", outline_num);
            nowChr->outline_tex_no = outline_num;
            texture = tex_manager->EnterTexture(
                 set_imgblock, texture_name, NULL, mgScreenWidth, mgScreenHeight,
                mgScreenDepth, NULL, 0, 0);
            outline_start_tex = texture;
            outline_start = 1;
            if (outline_start_tex == NULL) {
                return 0;
            }
        }
    }
    outline->texture = outline_start_tex;
    nowChr->AddOutLine(frame_name, outline);
    return 1;
}

/**
 * Loads the model with the visual classes needed by its animated vertices.
 */
static int _MODEL(SPI_STACK *stack, int count) {
    char                weight_name[64];
    char               *model_name;
    MDS_HEADER         *model;
    u_int              *weight;
    mgCreateVisualType  visual_type[26];
    mgLoadData          load;
    u_long128           work_buffer[6400];
    mgCFrame           *deform_frame;
    int                 index;
    char byte;

    model_name = spiGetStackString(stack);
    model = (MDS_HEADER *)GetPackFile(pack_file, model_name, NULL);
    if (model == NULL) {
        printf("not found %s\n", model_name);
        return 0;
    }
    for (index = 0; (byte = model_name[index]) != '\0' && byte != '.'; index++) {
        weight_name[index] = byte;
    }
    weight_name[index] = '\0';
    strcat(weight_name, ".wgt");
    weight = GetPackFile(pack_file, weight_name, NULL);
    if (alloc_vertex_num > 0) {
        char *empty_name = "";
        mgCreateVisualType *visual = visual_type;
        int vertex_index = 0;
        for (; vertex_index < alloc_vertex_num; vertex_index++) {
            if (nowChr->shape_anime == 0) {
                visual->type = MG_VISUAL_CREATE_MOTION_MDT;
            } else {
                visual->type = MG_VISUAL_CREATE_MDT;
            }
            visual->name = alloc_vertex[vertex_index];
            visual++;
        }
        visual->type = MG_VISUAL_CREATE_END;
        visual->name = empty_name;
        memset(&load, 0, sizeof(load));
        mgCMemory work_memory;
        work_memory.stSetBuffer(work_buffer, 6400);
        load.mds = model;
        load.work_memory = &work_memory;
        load.weight = weight;
        load.visual_type = visual_type;
        load.memory = base_stack;
        nowChr->CObjectFrame::frame = mgLoadMDSFile(&load);
    } else {
        nowChr->CObjectFrame::frame = mgLoadMDSFile(model, base_stack, NULL, NULL);
    }
    nowChr->deform_frame_num = 0;
    int frame_index = 0;
    for (; frame_index < alloc_vertex_num; frame_index++) {
        if (nowChr->CObjectFrame::frame != NULL) {
            deform_frame = nowChr->CObjectFrame::frame->SearchFrame(alloc_vertex[frame_index]);
            if (deform_frame != NULL) {
                nowChr->deform_frame[nowChr->deform_frame_num] = deform_frame;
                nowChr->deform_frame_num++;
            }
        }
    }
    return 1;
}

/**
 * Loads a shadow model and links its named frames to the character model.
 */
#ifdef NONMATCHING
static int _SHADOW_MODEL(SPI_STACK *stack, int count) {
    mgCreateVisualType visual_type[2] = {{MG_VISUAL_CREATE_SHADOW_MDT, ""}, {MG_VISUAL_CREATE_END, NULL}};
    char         *name;
    MDS_HEADER   *model;
    mgCFrame     *frame;
    mgCFrame     *shadow;
    mgCFrame     *shadow_part;
    unsigned int  size;
    unsigned int  quadwords;
    int           index;
    int           frame_id;
    int           link_num;

    name = spiGetStackString(stack);
    model = (MDS_HEADER *)GetPackFile(pack_file, name, NULL);
    if (model == NULL) {
        printf("not found %s\n", name);
        return 0;
    }
    nowChr->shadow_frame = mgLoadMDSFile(model, base_stack, visual_type, NULL);
    frame = nowChr->CObjectFrame::frame;
    shadow = nowChr->shadow_frame;
    if (shadow != NULL && frame != NULL && shadow->frame_num != 0) {
        size = shadow->frame_num * sizeof(int);
        quadwords = size >> 4;
        if (size & 0xF) {
            quadwords++;
        }
        nowChr->shadow_link_model = new (base_stack->Alloc(quadwords + 2)) int[shadow->frame_num];
        nowChr->shadow_link_shadow = new (base_stack->Alloc(quadwords + 2)) int[shadow->frame_num];
        if (nowChr->shadow_link_model != NULL && nowChr->shadow_link_shadow != NULL) {
            link_num = 0;
            for (index = 0; index < shadow->frame_num; index++) {
                shadow_part = shadow->frame_list[index];
                if (shadow_part != NULL && frame->frame_list[index] != NULL && shadow_part->name != NULL) {
                    frame_id = frame->SearchFrameID(shadow_part->name);
                    if (frame_id >= 0) {
                        nowChr->shadow_link_model[link_num] = frame_id;
                        nowChr->shadow_link_shadow[link_num] = index;
                        link_num++;
                    }
                }
            }
            nowChr->shadow_link_num = link_num;
        }
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", _SHADOW_MODEL__FP9SPI_STACKi);
#endif

/**
 * Names the next available primary entry frames and object slots.
 */
#ifdef NONMATCHING
static int _OBJECT_NAME(SPI_STACK *stack, int count) {
    char      name[64];
    mgCFrame *frame;
    mgCFrame *entry;
    int       frame_slot;
    int       object_slot;
    int       index;

    frame_slot = -1;
    for (index = 0; index < CHARA_ENTRY_FRAME_MAX; index++) {
        if (nowChr->entry_frame[index] == NULL) {
            frame_slot = index;
            break;
        }
    }
    if (frame_slot == -1) {
        return 0;
    }
    object_slot = -1;
    for (index = 0; index < CHARA_ENTRY_OBJECT_MAX; index++) {
        if (nowChr->entry_object[index].frame == NULL) {
            object_slot = index;
            break;
        }
    }
    if (object_slot == -1) {
        return 0;
    }
    frame = nowChr->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    for (index = 0; index < count; index++) {
        if (frame_slot >= CHARA_ENTRY_FRAME_MAX) {
            return 0;
        }
        strcpy(name, spiGetStackString(stack++));
        entry = frame->SearchFrame(name);
        if (entry != NULL) {
            nowChr->entry_frame[frame_slot++] = entry;
            nowChr->entry_object[object_slot].frame = entry;
            nowChr->entry_object[object_slot].unk_04 = 0.0f;
            nowChr->entry_object[object_slot].group = object_slot;
            nowChr->entry_object[object_slot].enable = 1;
            object_slot++;
        }
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", _OBJECT_NAME__FP9SPI_STACKi);
#endif

/**
 * Names entry objects in a group, with an optional value per frame.
 */
static int _OBJECT_NAME2(SPI_STACK *stack, int count) {
    char name[64];
    int slot = -1;
    int id;
    mgCFrame *root;
    int object_num;
    int index;
    mgCFrame *entry;
    float value;
    int i;

    i = 0;
    do {
        if (nowChr->entry_object[i].frame == NULL) {
            slot = i;
            break;
        }
        i++;
    } while (i < 0x18);
    if (slot == -1) {
        return 0;
    }
    id = spiGetStackInt(stack++);
    root = nowChr->CObjectFrame::frame;
    if (root == NULL) {
        return 0;
    }
    object_num = (count - 1) / 2;
    if (count < 3) {
        object_num = 1;
    }
    for (index = 0; index < object_num; index++) {
        if (slot >= CHARA_ENTRY_OBJECT_MAX) {
            return 0;
        }
        strcpy(name, spiGetStackString(stack++));
        entry = root->SearchFrame(name);
        value = 0.0f;
        if (id >= CHARA_ENTRY_FRAME_MAX) {
            value = spiGetStackFloat(stack++);
        } else if (count >= 3) {
            value = spiGetStackFloat(stack++);
        }
        if (entry != NULL) {
            if (id < CHARA_ENTRY_FRAME_MAX) {
                nowChr->entry_frame[id] = entry;
            }
            nowChr->entry_object[slot].frame = entry;
            nowChr->entry_object[slot].unk_04 = value;
            nowChr->entry_object[slot].group = id;
            nowChr->entry_object[slot++].enable = 1;
        }
    }
    return 1;
}

/**
 * Loads the motion files of one model motion set.
 */
static int _MOTION(SPI_STACK *stack, int count) {
    tagMOTION_TYPE *motion;
    char *motion_name;
    mgCFrame *root;
    SPI_STACK *arg;
    char *matrix_name;
    char *skin_name;
    MOTION_FILE_INFO files[3];

    root = nowChr->CObjectFrame::frame;
    arg = stack + 1;
    if (root == NULL) {
        return 0;
    }
    now_motion_id = spiGetStackInt(stack++);
    if (now_motion_id < 0 || now_motion_id >= CHARA_MOTION_SET_MAX) {
        return 0;
    }
    motion = &nowChr->motion[now_motion_id];
    memset(motion, 0, sizeof(*motion));
    motion_name = spiGetStackString(stack++);
    matrix_name = spiGetStackString(stack++);
    skin_name = spiGetStackString(stack);
    files[1].name = motion_name;
    files[0].name = matrix_name;
    files[2].name = skin_name;
    files[0].size = 0;
    files[1].size = 0;
    files[2].size = 0;
    files[0].data = GetPackFile(pack_file, matrix_name, &files[0].size);
    files[1].data = GetPackFile(pack_file, motion_name, &files[1].size);
    files[2].data = NULL;
    if (files[0].size == 0) {
        files[0].name = NULL;
    }
    if (files[1].size == 0) {
        files[1].name = NULL;
    }
    if (files[2].size == 0) {
        files[2].name = NULL;
    }
    if (files[0].data != 0) {
        int count = root->frame_num;
        if (root->init_matrix != NULL) {
            memcpy(root->init_matrix, files[0].data, count << 6);
        }
    }
    CreateAnimeDataEX(motion, ext_stack, files);
    if (now_motion_id > 0) {
        tagMOTION_TYPE *source = &nowChr->shadow_motion[0];
        tagMOTION_TYPE *dest = &nowChr->shadow_motion[now_motion_id];
        dest->base_matrices = source->base_matrices;
        dest->motion_list = source->motion_list;
        dest->skin_list = source->skin_list;
        dest->unk_0C = source->unk_0C;
    }
    return 1;
}

/**
 * Loads a shadow motion set and shares its frame skinning information.
 */
static int _SHADOW_MOTION(SPI_STACK *stack, int count) {
    tagMOTION_TYPE   *motion;
    MOTION_FILE_INFO  files[3];
    char             *motion_name;
    char             *matrix_name;
    char             *skin_name;

    if (nowChr->shadow_frame == NULL) {
        return 0;
    }
    if (now_motion_id < 0 || now_motion_id >= CHARA_MOTION_SET_MAX) {
        return 0;
    }
    motion = &nowChr->shadow_motion[now_motion_id];
    memset(motion, 0, sizeof(*motion));
    motion_name = spiGetStackString(stack++);
    matrix_name = spiGetStackString(stack++);
    skin_name = spiGetStackString(stack);
    files[0].size = 0;
    files[1].size = 0;
    files[2].size = 0;
    files[1].name = motion_name;
    files[0].name = matrix_name;
    files[2].name = skin_name;
    files[0].data = GetPackFile(pack_file, matrix_name, &files[0].size);
    files[1].data = GetPackFile(pack_file, motion_name, &files[1].size);
    files[2].data = GetPackFile(pack_file, skin_name, &files[2].size);
    if (matrix_name[0] == '\0') {
        files[0].name = NULL;
    }
    if (skin_name[0] == '\0') {
        files[2].name = NULL;
    }
    if (files[0].data == NULL && files[1].data == NULL && files[2].data == NULL) {
        return 0;
    }
    files[1].name = NULL;
    files[1].data = NULL;
    CreateAnimeDataEX(motion, ext_stack, files);
    if (nowChr->shadow_frame_info == NULL) {
        AnimeDataInit(nowChr->shadow_frame, motion, base_stack, &nowChr->shadow_frame_info);
    }
    motion->frame_info = nowChr->shadow_frame_info;
    return 1;
}

/**
 * Records the names of frames whose vertices will animate.
 */
static int _VERTEX_ANIME(SPI_STACK *stack, int count) {
    int   index;
    int   slot;
    char *name;

    for (index = 0; index < count; index++) {
        if (alloc_vertex_num >= CHARA_DEFORM_FRAME_MAX) {
            return 0;
        }
        name = spiGetStackString(stack++);
        slot = alloc_vertex_num++;
        strcpy(alloc_vertex[slot], name);
    }
    return 1;
}

/**
 * Chooses whether animated vertex frames use shape animation.
 */
static int _SHAPE_ANIME(SPI_STACK *stack, int count) {
    if (count != 1) {
        return 0;
    }
    nowChr->shape_anime = spiGetStackInt(stack);
    return 1;
}

/**
 * Begins the motion key list at the current memory position.
 */
static int _KEY_START(SPI_STACK *stack, int count) {
    now_key_ptr = (CHRINFO_KEY_SET *)&now_stack->stack[now_stack->stack_used];
    nowChr->key_list[now_motion_id] = now_key_ptr;
    nowChr->key_num[now_motion_id] = 0;
    return 1;
}

/**
 * Adds a named frame range and default step to the motion key list.
 */
static int _KEY(SPI_STACK *stack, int count) {
    if (count < 4) {
        return 0;
    }
    if (now_key_ptr == NULL) {
        return 0;
    }
    strcpy(now_key_ptr->name, spiGetStackString(stack++));
    now_key_ptr->start_frame = spiGetStackInt(stack++);
    now_key_ptr->end_frame = spiGetStackInt(stack++);
    now_key_ptr->step = spiGetStackFloat(stack);
    now_key_ptr++;
    nowChr->key_num[now_motion_id]++;
    return 1;
}

/**
 * Terminates the motion key list and claims its memory.
 */
static int _KEY_END(SPI_STACK *stack, int count) {
    if (now_key_ptr == NULL) {
        return 0;
    }
    now_key_ptr->name[0] = '\0';
    now_key_ptr->start_frame = -1;
    now_key_ptr->end_frame = -1;
    now_key_ptr->step = 0.0f;
    now_key_ptr++;
    nowChr->key_num[now_motion_id]++;
    now_stack->stAlloc64((nowChr->key_num[now_motion_id] * sizeof(CHRINFO_KEY_SET) >> 4) + 1);
    return 1;
}

#ifdef NONMATCHING
/**
 * Starts a named motion sequence and links it after the previous sequence.
 */
static int _SEQ_START(SPI_STACK *stack, int count) {
    CHRINFO_SEQ_HEADER *previous;

    if (count <= 0) {
        return 0;
    }
    if (now_stack == NULL) {
        return 0;
    }
    previous = now_seqhd_ptr;
    now_seqhd_ptr = (CHRINFO_SEQ_HEADER *)now_stack->stAlloc64(4);
    now_seq_ptr = (CHRINFO_SEQ *)&now_stack->stack[now_stack->stack_used];
    if (previous == NULL) {
        nowChr->seq_list[now_motion_id] = now_seqhd_ptr;
    } else {
        previous->next = now_seqhd_ptr;
    }
    strcpy(now_seqhd_ptr->name, spiGetStackString(stack));
    now_seqhd_ptr->next = NULL;
    now_seqhd_ptr->seq = now_seq_ptr;
    now_seqhd_ptr->seq_num = 0;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", _SEQ_START__FP9SPI_STACKi);
#endif

/**
 * Adds a motion step with its blend speed and ending conditions.
 */
static int _SEQ(SPI_STACK *stack, int count) {
    if (now_seq_ptr == NULL || now_seqhd_ptr == NULL) {
        return 0;
    }
    now_seq_ptr->type = CHRINFO_SEQ_ONCE;
    now_seq_ptr->loop_count = -1;
    now_seq_ptr->blend_speed = -1.0f;
    strcpy(now_seq_ptr->name, spiGetStackString(stack++));
    if (count >= 2) {
        now_seq_ptr->blend_speed = spiGetStackFloat(stack++);
    }
    if (count >= 3) {
        now_seq_ptr->type = spiGetStackInt(stack++);
    }
    if (count == 4) {
        now_seq_ptr->loop_count = spiGetStackInt(stack);
    }
    now_seq_ptr++;
    now_seqhd_ptr->seq_num++;
    return 1;
}

/**
 * Terminates the motion sequence and claims its step memory.
 */
static int _SEQ_END(SPI_STACK *stack, int count) {
    if (now_stack == NULL) {
        return 0;
    }
    now_seq_ptr->name[0] = '\0';
    now_seq_ptr->blend_speed = -1.0f;
    now_seq_ptr->type = CHRINFO_SEQ_ONCE;
    now_seq_ptr->loop_count = -1;
    now_seq_ptr++;
    now_seqhd_ptr->seq_num++;
    now_stack->stAlloc64((now_seqhd_ptr->seq_num * sizeof(CHRINFO_SEQ) >> 4) + 1);
    return 1;
}

/**
 * Allocates the character's cloth and hair animations.
 */
static int _CLOTH_START(SPI_STACK *stack, int count) {
    int           animation_num;

    animation_num = spiGetStackInt(stack);
    if (animation_num <= 0) {
        return 0;
    }
    void *block = (void *)now_stack->Alloc(DynAnimeAlign16Blocks((u32)animation_num * sizeof(CDynamicAnime)) + 2);
    nowChr->dynamic_anime = new ((u_long128 *)block) CDynamicAnime[animation_num];
    if (nowChr->dynamic_anime == NULL) {
        return 0;
    }
    nowChr->dynamic_anime_num = animation_num;
    return 1;
}

/**
 * Loads cloth animations and detaches shadow frames of detached model frames.
 */
#ifdef NONMATCHING
static int _CLOTH(SPI_STACK *stack, int count) {
    char     *name;
    char     *data;
    int       size;
    int       index;
    int       animation_id;
    mgCFrame *model;
    mgCFrame *shadow;
    mgCFrame *model_part;
    mgCFrame *shadow_part;

    for (index = 0; index < count; index++) {
        if (now_cloth_id >= nowChr->dynamic_anime_num) {
            return 0;
        }
        name = spiGetStackString(stack++);
        if (name != NULL) {
            data = (char *)GetPackFile(pack_file, name, &size);
            if (data != NULL) {
                animation_id = now_cloth_id++;
                nowChr->dynamic_anime[animation_id].Load(data, size, nowChr->CObjectFrame::frame, now_stack);
            }
        }
    }
    model = nowChr->CObjectFrame::frame;
    shadow = nowChr->shadow_frame;
    for (index = 0; index < nowChr->shadow_link_num; index++) {
        model_part = model->GetFrame(nowChr->shadow_link_model[index]);
        if (model_part != NULL) {
            shadow_part = shadow->GetFrame(nowChr->shadow_link_shadow[index]);
            if (shadow_part != NULL && model_part->parent == NULL && shadow_part->parent != NULL) {
                shadow_part->DeleteParent();
            }
        }
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", _CLOTH__FP9SPI_STACKi);
#endif

/**
 * Accepts the end of the cloth animation list.
 */
static int _CLOTH_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Moves the character to the position in the info file.
 */
static int _POSITION(SPI_STACK *stack, int count) {
    float  x;
    float  y;
    float  z;

    x = spiGetStackFloat(stack++);
    y = spiGetStackFloat(stack++);
    z = spiGetStackFloat(stack);
    nowChr->SetPosition(x, y, z);
    return 1;
}

/**
 * Turns the character to the rotation in the info file.
 */
static int _ROTATION(SPI_STACK *stack, int count) {
    float  x;
    float  y;
    float  z;

    x = spiGetStackFloat(stack++);
    y = spiGetStackFloat(stack++);
    z = spiGetStackFloat(stack);
    nowChr->SetRotation(x, y, z);
    return 1;
}

/**
 * Allocates the sounds belonging to the current motion set.
 */
static int _SE_START(SPI_STACK *stack, int count) {

    now_se_header = NULL;
    nowChr->se_num[now_motion_id] = 0;
    if (count != 1) {
        return 0;
    }
    nowChr->se_num[now_motion_id] = spiGetStackInt(stack);
    nowChr->se_list[now_motion_id] = (CHRINFO_SE *)operator new[](
        nowChr->se_num[now_motion_id] * sizeof(CHRINFO_SE),
        (u_long128 *)now_stack->Alloc(DynAnimeAlign16Blocks(nowChr->se_num[now_motion_id] * sizeof(CHRINFO_SE)) + 2));
    now_se_header = nowChr->se_list[now_motion_id];
    return 1;
}

/**
 * Adds a sound that plays once at a frame of the named motion.
 */
static int _SE(SPI_STACK *stack, int count) {
    char  *motion_name;
    int    kind;
    int    sound_no;
    float  frame;

    if (now_se_header == NULL) {
        return 0;
    }
    if (count != 4) {
        return 0;
    }
    motion_name = spiGetStackString(stack++);
    kind = spiGetStackInt(stack++);
    sound_no = spiGetStackInt(stack++);
    frame = spiGetStackFloat(stack);
    if (frame <= 1.0f) {
        frame = nowChr->GetWaitToFrame(motion_name, frame);
    }
    if (frame <= 0.0f) {
        return 0;
    }
    now_se_header->frame = frame;
    now_se_header->end_frame = 0.0f;
    now_se_header->kind = kind;
    now_se_header->se_no = sound_no;
    now_se_header->loop_slot = 0;
    now_se_header->wait = 0;
    now_se_header++;
    return 1;
}

/**
 * Adds a sound that loops over a frame range of the named motion.
 */
static int _SELP(SPI_STACK *stack, int count) {
    char *motion_name;
    SPI_STACK *arg;
    int se_no;
    int sound_no;
    float start;
    float end;
    int loop_flag;
    float start_time;
    float end_time;

    if (now_se_header == NULL) {
        return 0;
    }
    arg = stack + 1;
    if (count != 6) {
        return 0;
    }
    motion_name = spiGetStackString(stack++ );
    se_no = spiGetStackInt(arg++);
    sound_no = spiGetStackInt(arg++);
    start = spiGetStackFloat(arg++);
    end = spiGetStackFloat(arg++);
    loop_flag = spiGetStackInt(arg);
    if (start <= 1.0f) {
        start_time = nowChr->GetWaitToFrame(motion_name, start);
    } else {
        start_time = start;
    }
    if (end <= 1.0f) {
        end_time = nowChr->GetWaitToFrame(motion_name, end);
    } else {
        end_time = end;
    }
    if (start_time <= 0.0f || end_time <= 0.0f) {
        return 0;
    }
    now_se_header->frame = start_time;
    now_se_header->end_frame = end_time;
    now_se_header->kind = se_no;
    now_se_header->se_no = sound_no;
    now_se_header->loop_slot = loop_flag;
    now_se_header->wait = 0;
    now_se_header++;
    return 1;
}

/**
 * Accepts the end of the motion sound list.
 */
static int _SE_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Accepts the end of the motion definition.
 */
static int _MOTION_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Opens the effect pack used by the character's following effect entries.
 */
static int _EFFECT_START(SPI_STACK *stack, int count) {
    eff_pack_ptr = (unsigned int *)GetPackFile(pack_file, spiGetStackString(stack), &eff_pack_size);
    return eff_pack_ptr != NULL;
}

/**
 * Loads an effect, its particle and emitter pools, and its motion attachment.
 */
static int _EFFECT(SPI_STACK *stack, int count) {
    char                   *script;
    CHRINFO_EFFECT         *entry;
    CHARA_EFFECT_MANAGER   *manager;
    CHRINFO_EFFECT         *last;
    unsigned char          *image;
    CHRINFO_EFFECT_IMAGE  **image_entry;
    char                   *file_name;
    char                   *effect_name;
    char                   *frame_name;
    char                   *motion_name;
    sceVu0FVECTOR           offset;
    float                   start_ratio;
    int                     local_draw;
    int                     script_size;
    int                     particle_count;
    int                     emitter_count;
    int                     image_size;
    int                     enter_image;
    int                     index;

    if (eff_pack_ptr == NULL) {
        return 0;
    }
    file_name = spiGetStackString(stack++);
    effect_name = spiGetStackString(stack++);
    local_draw = spiGetStackInt(stack++);
    frame_name = spiGetStackString(stack++);
    offset[0] = spiGetStackFloat(stack++);
    offset[1] = spiGetStackFloat(stack++);
    offset[2] = spiGetStackFloat(stack++);
    offset[3] = 1.0f;
    motion_name = NULL;
    start_ratio = 0.0f;
    if (count >= 8) {
        motion_name = spiGetStackString(stack++);
        start_ratio = spiGetStackFloat(stack++);
    }
    script = (char *)GetPackFile(eff_pack_ptr, file_name, &script_size);
    if (script == NULL) {
        return 0;
    }
    entry = (CHRINFO_EFFECT *)now_stack->stAlloc64(3);
    manager = (CHARA_EFFECT_MANAGER *)now_stack->stAlloc64(32);
    manager->Initialize();
    manager->GetBufferNums(script, script_size, &particle_count, &emitter_count);
    manager->particle_pool = (CEffect *)now_stack->Alloc(particle_count * sizeof(CEffect) / 16 + 1);
    for (index = 0; index < particle_count; index++) {
        manager->particle_pool[index].Initialize();
    }
    manager->emitter_pool = (CEffectCtrl *)now_stack->Alloc(emitter_count * sizeof(CEffectCtrl) / 16 + 1);
    for (index = 0; index < emitter_count; index++) {
        manager->emitter_pool[index].Initialize();
    }
    manager->EntryEffCtrls(manager->particle_pool, particle_count, manager->emitter_pool, emitter_count);
    manager->Load(script, script_size);
    if (nowChr->effect_image_load != 0) {
        image = (unsigned char *)GetPackFile(eff_pack_ptr, manager->img_name, &image_size);
        if (image != NULL) {
            mgCTextureManager *textures = &mgTexManager;
            image_entry = &nowChr->effect_image_list;
            enter_image = 1;
            if (*image_entry != NULL) {
                while (*image_entry != NULL) {
                    if (strcmp((*image_entry)->name, manager->img_name) == 0) {
                        enter_image = 0;
                        break;
                    }
                    image_entry = &(*image_entry)->next;
                }
            }
            if (enter_image != 0) {
                *image_entry = (CHRINFO_EFFECT_IMAGE *)now_stack->stAlloc64(40);
                (*image_entry)->next = NULL;
                strcpy((*image_entry)->name, manager->img_name);
                (*image_entry)->data = (unsigned char *)img_stack->Alloc(image_size / 16 + 1);
                memcpy((*image_entry)->data, image, image_size);
                textures->EnterIMGFile(image, set_imgblock, img_stack, NULL);
            }
        }
    }
    manager->local_draw = local_draw;
    sceVu0CopyVector(manager->offset, offset);
    manager->start_ratio = start_ratio;
    if (frame_name != NULL) {
        strcpy(manager->frame_name, frame_name);
    } else {
        strcpy(manager->frame_name, "");
    }
    if (motion_name != NULL) {
        strcpy(manager->motion_name, motion_name);
    } else {
        strcpy(manager->motion_name, "");
    }
    entry->effect = manager;
    strcpy(entry->name, effect_name);
    entry->next = NULL;
    last = nowChr->effect_list;
    if (last != NULL) {
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = entry;
    } else {
        nowChr->effect_list = entry;
    }
    return 1;
}

/**
 * Closes the current effect pack.
 */
static int _EFFECT_END(SPI_STACK *stack, int count) {
    if (eff_pack_ptr == NULL) {
        return 0;
    }
    eff_pack_ptr = NULL;
    eff_pack_size = 0;
    return 1;
}

void CCharacter2::InitEffect() {
    int  index;

    for (index = 0; index < CHARA_ENTRY_EFFECT_MAX; index++) {
        entry_effect[index].effect = NULL;
        entry_effect[index].active = 0;
        entry_effect[index].running = 0;
    }
    effect_list = NULL;
    effect_enable = 1;
    effect_image_list = NULL;
    effect_image_load = 1;
}

void CCharacter2::ExecEntryEffect(CHRINFO_KEY_SET *key) {
    int index;
    int count;
    CHRINFO_EFFECT *node;

    for (index = 0; index < CHARA_ENTRY_EFFECT_MAX; index++) {
        if (entry_effect[index].active != 0 && entry_effect[index].running != 0) {
            entry_effect[index].effect->Stop();
        }
        entry_effect[index].effect = NULL;
        entry_effect[index].active = 0;
        entry_effect[index].running = 0;
    }
    if (effect_enable == 0) {
        return;
    }
    node = effect_list;
    if (key == 0) {
        return;
    }
    count = 0;
    while (node != 0) {

        if (strcmp((char *)node->effect ->motion_name, now_key->name ) == 0) {
            entry_effect[count].effect = node->effect;
            entry_effect[count].active = 1;
            count++;
        }
        node = node->next;
    }
}

void CCharacter2::CtrlEffect() {
    for (int index = 0; index < CHARA_ENTRY_EFFECT_MAX; index++) {
        if (entry_effect[index].active == 0) {
            continue;
        }
        if (entry_effect[index].running != 0) {
            continue;
        }
        CHARA_EFFECT_MANAGER *manager = entry_effect[index].effect;
        CHRINFO_KEY_SET *motion = now_key;
        float progress = (frame - (float)motion->start_frame) /
                       ((float)motion->end_frame - (float)motion->start_frame);

        if (progress > manager->start_ratio) {
            entry_effect[index].effect->Run();
            entry_effect[index].running = 1;
        }
    }
}

void CCharacter2::StepEffect() {
    CHRINFO_EFFECT *effect;
    int             index;

    for (index = 0; index < CHARA_SWORD_EFFECT_MAX; index++) {
        if (sword_effect[index] != NULL) {
            sword_effect[index]->Step();
            sword_effect[index]->CreatPointList();
        }
    }
    for (effect = effect_list; effect != NULL; effect = effect->next) {
        effect->effect->Ctrl();
        effect->effect->Step(1);
    }
}

void CCharacter2::DrawEffect() {
    CHRINFO_EFFECT  *effect;
    mgCFrame        *effect_frame;
    sceVu0FMATRIX    world_matrix;
    sceVu0FVECTOR    world_position;
    int              index;

    for (index = 0; index < CHARA_SWORD_EFFECT_MAX; index++) {
        if (sword_effect[index] != NULL) {
            sword_effect[index]->Draw();
        }
    }
    for (effect = effect_list; effect != NULL; effect = effect->next) {
        mgC3DSprite sprite;
        sceVu0FMATRIX world_matrix;
        sceVu0FVECTOR world_position;
        mgUnitMatrix(world_matrix);
        world_position[3] = 0.0f;
        world_position[2] = 0.0f;
        world_position[1] = 0.0f;
        world_position[0] = 0.0f;
        if (strcmp(effect->effect->frame_name, "") != 0) {
            effect_frame = CObjectFrame::frame->SearchFrame(effect->effect->frame_name);
            if (effect_frame != NULL) {
                effect_frame->GetWorldPosition0(world_position);
                sceVu0TransMatrix(world_matrix, world_matrix, effect->effect->offset);
                sceVu0RotMatrix(world_matrix, world_matrix, rotation);
                sceVu0TransMatrix(world_matrix, world_matrix, world_position);
            }
        }
        if (effect->effect->local_draw != 0) {
            effect->effect->CreatePacket(&sprite);
            mgDrawDirect(&sprite, world_matrix);
        } else {
            effect->effect->SetOrigin(world_matrix[3]);
            effect->effect->Draw();
        }
    }
}

/**
 * Runs a skin info file to exchange parts of the current character's model.
 */
static void ScanInfoSkinFile(CCharacter2 *character, unsigned int *pack, char *name, char *skin_name, mgCMemory *memory, int image_block) {
    CScriptInterpreter  script;
    char               *script_data;
    int                 script_size;

    base_stack = memory;
    pack_file = pack;
    set_imgblock = image_block;
    nowChr = character;
    skin_name_ptr = skin_name;
    root_skin_frame = NULL;
    skin_frame = NULL;
    script_data = (char *)GetPackFile(pack, name, &script_size);
    if (script_data == NULL) {
        printf("not found %s\n", name);
        return;
    }
    script.SetTag(skin_tag);
    script.SetScript(script_data, script_size);
    script.Run();
}

/**
 * Selects the texture archive to load when the skin's motion is processed.
 */
static int _SKIN_IMG(SPI_STACK *stack, int count) {
    if (base_stack == NULL) {
        return 0;
    }
    spiGetStackInt(stack++);
    load_img_ptr = (unsigned char *)GetPackFile(pack_file, spiGetStackString(stack), &load_img_size);
    return load_img_ptr != NULL;
}

/**
 * Ends the skin's texture archive declaration.
 */
static int _SKIN_IMG_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Records the replacement model file after checking it exists in the pack.
 */
static int _SKIN_MODEL(SPI_STACK *stack, int count) {
    char *name;

    if (count != 1) {
        return 0;
    }
    name = spiGetStackString(stack);
    if (GetPackFile(pack_file, name, NULL) == NULL) {
        printf("not found %s\n", name);
        return 0;
    }
    strcpy(skin_mds_name, name);
    return 1;
}

/**
 * Loads a replacement model and binds its skinned visuals to the main skeleton.
 */
static mgCFrame *CreateChangeFrame(mgLoadData *load, mgCFrame *root) {
    mgCFrame *replacement = (mgCFrame *)mgLoadMDSFile(load);
    char **name;
    int original_id;
    mgCVisualMotionMDT *motion;
    mgCFrame **frame_list;
    float(*base_matrix)[4][4];
    int index;
    mgCreateVisualType *list;
    mgCFrame *source_frame;
    mgCreateVisualType *entry;
    int replacement_id;

    if (replacement == NULL) {
        return NULL;
    }
    list = load->visual_type;
    index = 0;
    for (;;) {
        entry = &list[index];
        if (entry->type == MG_VISUAL_CREATE_END) {
            break;
        }
        name = &entry->name;
        if (root->SearchFrame(entry->name) != NULL) {
            source_frame = replacement->SearchFrame(*name);
            if (source_frame != NULL) {
                motion = (mgCVisualMotionMDT *) source_frame->visual;
                if (motion != NULL && motion->Iam() == MG_VISUAL_KIND_MOTION_MDT) {
                    original_id = root->SearchFrameID(*name);
                    replacement_id = replacement->SearchFrameID(*name);
                    frame_list = root->frame_list;
                    base_matrix = root->init_matrix;
                    if (load->matrix != NULL) {
                        memcpy(base_matrix[original_id], load->matrix[replacement_id], sizeof(sceVu0FMATRIX));
                    }
                    motion->ChangeWeight(frame_list, base_matrix, original_id);
                }
            }
        }
        index++;
    }
    return replacement;
}

#ifdef NONMATCHING
/**
 * Loads a replacement skin and exchanges the matching model visuals and bounds.
 */
static int _SKIN_MOTION(SPI_STACK *stack, int count) {
    mgCMemory           work_memory;
    mgCreateVisualType  visual_type[64];
    mgIMG_HEADER        image_header;
    mgLoadData          load;
    u_long128           work_buffer[6400];
    sceVu0FVECTOR       box_max;
    sceVu0FVECTOR       box_min;
    mgCFrame           *root;
    mgCFrame           *source_frame;
    mgCFrame           *dest_frame;
    mgCVisualMDT       *visual;
    char               *matrix_name;
    char               *weight_name;
    unsigned char      *weight_file;
    unsigned char      *matrix_file;
    unsigned char      *model_file;
    unsigned char      *image;
    char              **group_names;
    int                 index;
    int                 visual_count;
    int                 deform_index;
    int                 image_count;

    if (nowChr->CObjectFrame::frame == NULL) {
        return 0;
    }
    now_motion_id = spiGetStackInt(stack++);
    spiGetStackString(stack++);
    matrix_name = spiGetStackString(stack++);
    weight_name = spiGetStackString(stack++);
    weight_file = (unsigned char *)GetPackFile(pack_file, weight_name, NULL);
    matrix_file = (unsigned char *)GetPackFile(pack_file, matrix_name, NULL);
    if (weight_file == NULL) {
        printf("not found %s\n", weight_name);
        return 0;
    }
    if (nowChr->shape_anime == 0) {
        model_file = (unsigned char *)GetPackFile(pack_file, skin_mds_name, NULL);
        visual_count = 0;
        deform_index = 0;
        for (index = 0; index < nowChr->deform_frame_num; index++) {
            if (nowChr->deform_frame[deform_index] != NULL) {
                visual_type[visual_count].type = MG_VISUAL_CREATE_MOTION_MDT;
                visual_type[visual_count].name = nowChr->deform_frame[deform_index]->name;
                visual_count++;
                deform_index++;
            }
        }
        visual_type[visual_count].type = MG_VISUAL_CREATE_END;
        visual_type[visual_count].name = NULL;
        if (load_img_ptr != NULL) {
            image = (unsigned char *)base_stack->stAlloc64(load_img_size / 16 + 1);
            memcpy(image, load_img_ptr, load_img_size);
            image_count = mgGetIMGHeaderNum((char *)image);
            for (index = 0; index < image_count; index++) {
                image_header = mgGetIMGHeader((char *)image, index);
                mgTexManager.DeleteTexture(image_header.name, set_imgblock);
            }
            mgTexManager.EnterIMGFile(image, set_imgblock, base_stack, NULL);
            group_names = mgTexManager.GetGroupNameList(set_imgblock, &nowChr->tex_anime_group_num);
            if (group_names != NULL) {
                nowChr->tex_anime_group_start = 0;
                while (group_names[nowChr->tex_anime_group_start] != NULL) {
                    nowChr->tex_anime_group_start++;
                }
            }
        }
        memset(&load, 0, sizeof(load));

        work_memory.stSetBuffer(work_buffer, 6400);
        load.mds = (MDS_HEADER *)model_file;
        load.weight = (unsigned int *)weight_file;
        load.work_memory = &work_memory;
        load.matrix = (float (*)[4][4])matrix_file;
        load.visual_type = visual_type;
        load.memory = base_stack;
        root_skin_frame = CreateChangeFrame(&load, nowChr->CObjectFrame::frame);
        if (root_skin_frame == NULL) {
            return 0;
        }
        root = nowChr->CObjectFrame::frame;
        for (index = 0; index < root_skin_frame->frame_num; index++) {
            source_frame = root_skin_frame->frame_list[index];
            if (source_frame != NULL && source_frame->visual != NULL) {
                source_frame->GetBBox(box_max, box_min);
                dest_frame = root->SearchFrame(source_frame->name);
                if (dest_frame != NULL) {
                    dest_frame->SetVisual(source_frame->visual);
                    dest_frame->SetBBox(box_max, box_min);
                }
            }
        }
    } else {
        if (skin_frame != NULL) {
            root = nowChr->CObjectFrame::frame;
            if (root != NULL) {
                visual = (mgCVisualMDT *)skin_frame->visual;
                ChangeWeight(nowChr->motion[0].skin_list, base_stack, weight_file, root->SearchFrameID(skin_name_ptr), nowChr->motion[0].frame_info, visual, root, root_skin_frame);
                dest_frame = root->SearchFrame(skin_name_ptr);
                if (dest_frame != NULL) {
                    dest_frame->SetVisual(visual);
                }
            }
        }
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", _SKIN_MOTION__FP9SPI_STACKi);
#endif

/**
 * Allocates the character's level-of-detail records.
 */
static int _LOD_MODEL_START(SPI_STACK *stack, int count) {
    int  level_count;

    level_count = spiGetStackInt(stack);
    if (level_count <= 0) {
        return 0;
    }
    void *block = (void *)base_stack->Alloc(DynAnimeAlign16Blocks((u32)level_count * 0x18) + 2);
    nowChr->lod = new ((u_long128 *)block) CCharaLOD[level_count];
    if (nowChr->lod != NULL) {
        nowChr->lod_num = level_count;
    }
    return 1;
}

CCharaLOD::CCharaLOD() {
    link_num = 0;
    link = NULL;
    frame = NULL;
    distance = 0.0f;
    standalone = 0;
    motion = 0;
}

/**
 * Builds a level-of-detail model and the frame links used to exchange its visuals.
 */
static int _LOD_MODEL(SPI_STACK *stack, int count) {
    mgCreateVisualType  visual_type[64];
    mgLoadData          load;
    u_long128           work_buffer[6400];
    CCharaLOD          *level;
    int                 index;
    mgCFrame           *source_frame;
    char               *model_name;
    MDS_HEADER         *model_file;
    unsigned int       *weight_file;
    int               (*link)[2];
    float             (*matrix_file)[4][4];
    int                 frame_index;
    int                 frame_count;
    int                 visual_count;
    mgCFrame           *root;
    mgCFrame          **frame_list;
    char               *weight_name;
    char               *matrix_name;
    int                 level_index;
    int                 deform_index;
    int                 quadwords;

    level_index = spiGetStackInt(stack++);
    if (level_index < 0 || level_index >= nowChr->lod_num) {
        return 0;
    }
    level = &nowChr->lod[level_index];
    model_name = spiGetStackString(stack++);
    weight_name = spiGetStackString(stack++);
    matrix_name = spiGetStackString(stack++);
    level->distance = spiGetStackFloat(stack++);
    model_file = (MDS_HEADER *)GetPackFile(pack_file, model_name, NULL);
    weight_file = (unsigned int *)GetPackFile(pack_file, weight_name, NULL);
    matrix_file = (float (*)[4][4])GetPackFile(pack_file, matrix_name, NULL);
    if (model_file == NULL) {
        return 0;
    }
    root = nowChr->CObjectFrame::frame;
    if (root == NULL) {
        return 0;
    }
    visual_count = 0;
    memset(&load, 0, sizeof(load));

    mgCMemory work_memory;
    work_memory.stSetBuffer(work_buffer, 6400);
    load.mds = model_file;
    load.work_memory = &work_memory;
    load.visual_type = visual_type;
    load.memory = base_stack;
    if (weight_file == NULL || matrix_file == NULL) {
        visual_type[0].name = NULL;
        visual_type[0].type = MG_VISUAL_CREATE_END;
        level->motion = 0;
        level->standalone = 1;
        level->frame = mgLoadMDSFile(&load);
        if (level->frame == NULL) {
            return 0;
        }
        return 1;
    }
    level->motion = 1;
    deform_index = 0;
    for (index = 0; index < nowChr->deform_frame_num; index++) {
        if (nowChr->deform_frame[deform_index] != NULL) {
            visual_type[visual_count].type = MG_VISUAL_CREATE_MOTION_MDT;
            visual_type[visual_count].name = nowChr->deform_frame[deform_index]->name;
            deform_index++;
            visual_count++;
        }
    }
    visual_type[visual_count].type = MG_VISUAL_CREATE_END;
    visual_type[visual_count].name = NULL;
    load.weight = weight_file;
    load.matrix = matrix_file;
    level->frame = CreateChangeFrame(&load, root);
    if (level->frame == NULL) {
        return 0;
    }
    frame_count = level->frame->frame_num;
    quadwords = DynAnimeAlign16Blocks(frame_count * sizeof(*level->link));
    level->link = new (base_stack->Alloc(quadwords + 2)) int[frame_count][2];
    level->link_num = 0;
    frame_list = level->frame->frame_list;
    if (frame_list == NULL) {
        return 0;
    }
    link = level->link;
    for (frame_index = 0; frame_index < frame_count; frame_index++) {
        source_frame = frame_list[frame_index];
        if (source_frame != NULL) {
            link[0][1] = frame_index;
            link[0][0] = root->SearchFrameID(source_frame->name);
            if (link[0][0] >= 0) {
                link++;
                level->link_num++;
            }
        }
    }
    return 1;
}

/**
 * Marks the level of detail as needing its first exchange.
 */
static int _LOD_MODEL_END(SPI_STACK *stack, int count) {
    nowChr->lod_no = -1;
    return 1;
}

mgCFrame *CCharacter2::ChangeLOD(int no) {
    CCharaLOD *level;
    mgCFrame *root;
    mgCFrame *level_frame;
    mgCFrame **root_frames;
    int (*pair)[2];
    int index;
    mgCFrame *dest_frame;
    mgCFrame *source_frame;
    mgCVisual *motion;
    sceVu0FVECTOR box_min;
    sceVu0FVECTOR box_max;

    if (lod_no < 0 && no != 0) {
        ChangeLOD(0);
    }
    if (no < 0 || no >= lod_num) {
        return NULL;
    }
    level = &lod[no];
    motion_enable = level->motion;
    if (level->standalone == 0 && no == lod_no) {
        return NULL;
    }
    lod_no = no;
    root = CObjectFrame::frame;
    level_frame = level->frame;
    if (root == NULL || level_frame == NULL) {
        return NULL;
    }
    level_frame->SetPosition(position);
    level_frame->SetRotation(rotation);
    level_frame->SetScale(scale);
    if (level->standalone != 0) {
        return level_frame;
    }
    root_frames = root->frame_list;
    pair = level->link;
    for (index = 0; index < level->link_num; index++, pair++) {
        dest_frame = root->GetFrame((*pair)[0]);
        if (dest_frame != NULL) {
            source_frame = level_frame->GetFrame((*pair)[1]);
            if (source_frame != NULL) {
                motion = source_frame->visual;
                if (motion != NULL && motion->Iam() == MG_VISUAL_KIND_MOTION_MDT) {
                    ((mgCVisualMotionMDT *)motion)->frame = root_frames;
                }
                dest_frame->SetVisual(motion);
                source_frame->GetBBox(box_min, box_max);
                dest_frame->SetBBox(box_min, box_max);
            }
        }
    }
    return NULL;
}

#ifdef NONMATCHING
void CCharacter2::Copy(CCharacter2 &dest, mgCMemory *memory) {
    COutLineDraw *source_outline;
    COutLineDraw *dest_outline;
    CHRINFO_SE   *sounds;
    int           free_space;
    int           quadwords;
    int           index;
    int           component;
    int           row;

    free_space = memory->stack_size - memory->stack_used;
    (CObject &)dest = *this;
    dest.CObjectFrame::frame = CObjectFrame::frame;
    for (component = 0; component < 4; component++) {
        dest.velocity[component] = velocity[component];
        dest.base_scale[component] = base_scale[component];
    }
    dest.move_accel = move_accel;
    for (row = 0; row < 4; row++) {
        for (component = 0; component < 4; component++) {
            dest.entry_matrix[row][component] = entry_matrix[row][component];
        }
    }
    for (index = 0; index < 16; index++) {
        dest.name[index] = name[index];
    }
    dest.alpha = alpha;
    dest.poly_num[0] = poly_num[0];
    dest.poly_num[1] = poly_num[1];
    dest.body_width = body_width;
    dest.body_height = body_height;
    dest.body_depth = body_depth;
    dest.load_size = load_size;
    dest.copy_size = copy_size;
    dest.dynamic_anime_flags = dynamic_anime_flags;
    dest.outline = outline;
    dest.outline_tex_no = outline_tex_no;
    dest.dynamic_anime_num = dynamic_anime_num;
    dest.dynamic_anime = dynamic_anime;
    dest.shape_anime = shape_anime;
    for (index = 0; index < CHARA_ENTRY_FRAME_MAX; index++) {
        dest.entry_frame[index] = entry_frame[index];
    }
    for (index = 0; index < CHARA_ENTRY_OBJECT_MAX; index++) {
        dest.entry_object[index] = entry_object[index];
    }
    dest.shadow_frame = shadow_frame;
    for (index = 0; index < CHARA_IMAGE_MAX; index++) {
        dest.images[index] = images[index];
    }
    dest.tex_anime_group_num = tex_anime_group_num;
    dest.tex_anime_group_start = tex_anime_group_start;
    dest.texture_block = texture_block;
    for (index = 0; index < CHARA_DEFORM_FRAME_MAX; index++) {
        dest.deform_frame[index] = deform_frame[index];
    }
    dest.deform_frame_num = deform_frame_num;
    dest.lod_num = lod_num;
    dest.lod = lod;
    dest.lod_no = lod_no;
    dest.motion_enable = motion_enable;
    dest.shadow_link_num = shadow_link_num;
    dest.shadow_link_model = shadow_link_model;
    dest.shadow_link_shadow = shadow_link_shadow;
    dest.next_key = next_key;
    dest.next_flags = next_flags;
    dest.next_set = next_set;
    dest.now_key = now_key;
    dest.seq_mode = seq_mode;
    dest.now_flags = now_flags;
    dest.now_set = now_set;
    dest.motion_status = motion_status;
    dest.frame = frame;
    dest.frame_ratio = frame_ratio;
    dest.step = step;
    dest.posed_key = posed_key;
    dest.prev_flags = prev_flags;
    dest.prev_set = prev_set;
    dest.prev_frame = prev_frame;
    dest.next_seq = next_seq;
    dest.now_seq = now_seq;
    dest.seq_step = seq_step;
    dest.seq_flags = seq_flags;
    dest.seq_loop = seq_loop;
    dest.seq_state = seq_state;
    dest.seq_advance = seq_advance;
    for (index = 0; index < CHARA_MOTION_SET_MAX; index++) {
        dest.motion[index] = motion[index];
        dest.shadow_motion[index] = shadow_motion[index];
    }
    dest.unk_500 = unk_500;
    dest.shadow_frame_info = shadow_frame_info;
    dest.blend = blend;
    dest.blend_speed = blend_speed;
    for (index = 0; index < CHARA_MOTION_SET_MAX; index++) {
        dest.key_list[index] = key_list[index];
        dest.key_num[index] = key_num[index];
        dest.seq_list[index] = seq_list[index];
    }
    for (index = 0; index < CHARA_SWORD_EFFECT_MAX; index++) {
        dest.sword_effect[index] = sword_effect[index];
    }
    dest.foot_se_bank = foot_se_bank;
    dest.foot_sound_id = foot_sound_id;
    dest.foot_sound_enable = foot_sound_enable;
    dest.se_bank = se_bank;
    dest.se_bank_2 = se_bank_2;
    dest.se_positional = se_positional;
    dest.se_volume = se_volume;
    dest.se_pan = se_pan;
    dest.foot_effect_wait = foot_effect_wait;
    dest.loop_se = loop_se;
    for (index = 0; index < CHARA_MOTION_SET_MAX; index++) {
        dest.se_list[index] = se_list[index];
        dest.se_num[index] = se_num[index];
    }
    dest.effect_image_load = effect_image_load;
    dest.effect_list = effect_list;
    for (index = 0; index < CHARA_ENTRY_EFFECT_MAX; index++) {
        dest.entry_effect[index] = entry_effect[index];
    }
    dest.effect_image_list = effect_image_list;
    dest.effect_enable = effect_enable;
    if (memory != NULL) {
        dest.CObjectFrame::frame = mgCopyFrame(CObjectFrame::frame, memory, 1);
        if (dest.CObjectFrame::frame != NULL) {
            dest.shadow_frame = shadow_frame;
            if (outline_tex_no > 0 && outline != NULL) {
                dest.outline = NULL;
                dest.outline_tex_no = 0;
                for (source_outline = outline; source_outline != NULL; source_outline = source_outline->next) {
                    dest_outline = new (memory->Alloc(9)) COutLineDraw;
                    if (dest_outline == NULL) {
                        break;
                    }
                    *dest_outline = *source_outline;
                    dest_outline->next = NULL;
                    if (source_outline->frame != NULL) {
                        dest.AddOutLine(source_outline->frame->name, dest_outline);
                    }
                }
            }
            if (dynamic_anime_num > 0 && dynamic_anime != NULL) {
                quadwords = (dynamic_anime_num * sizeof(CDynamicAnime) + 15) / 16;
                dest.dynamic_anime = new (memory->Alloc(quadwords + 2)) CDynamicAnime[dynamic_anime_num];
                for (index = 0; index < dynamic_anime_num; index++) {
                    dynamic_anime[index].Copy(dest.dynamic_anime[index], dest.CObjectFrame::frame, memory);
                }
            }
            for (index = 0; index < CHARA_ENTRY_FRAME_MAX; index++) {
                if (entry_frame[index] == NULL) {
                    dest.entry_frame[index] = NULL;
                } else {
                    dest.entry_frame[index] = dest.CObjectFrame::frame->SearchFrame(entry_frame[index]->name);
                }
            }
            for (index = 0; index < CHARA_ENTRY_OBJECT_MAX; index++) {
                if (entry_object[index].frame == NULL) {
                    dest.entry_object[index].frame = NULL;
                } else {
                    dest.entry_object[index].frame = dest.CObjectFrame::frame->SearchFrame(entry_object[index].frame->name);
                    dest.entry_object[index].unk_04 = entry_object[index].unk_04;
                    dest.entry_object[index].group = entry_object[index].group;
                    dest.entry_object[index].enable = entry_object[index].enable;
                }
            }
            for (index = 0; index < deform_frame_num; index++) {
                if (deform_frame[index] == NULL) {
                    dest.deform_frame[index] = NULL;
                } else {
                    dest.deform_frame[index] = dest.CObjectFrame::frame->SearchFrame(deform_frame[index]->name);
                }
            }
            if (lod_num > 0) {
                quadwords = (lod_num * sizeof(CCharaLOD) + 15) / 16;
                dest.lod = new (memory->Alloc(quadwords + 2)) CCharaLOD[lod_num];
                if (dest.lod == NULL) {
                    return;
                }
                dest.lod_num = lod_num;
                for (index = 0; index < lod_num; index++) {
                    dest.lod[index] = lod[index];
                    dest.lod[index].frame = mgCopyFrame(lod[index].frame, memory, 1);
                }
            }
            lod_no = -1;
            sounds = GetSoundInfoCopy(memory);
            if (sounds != NULL) {
                dest.se_list[0] = sounds;
                dest.se_num[0] = se_num[0];
            }
            for (index = 0; index < CHARA_SWORD_EFFECT_MAX; index++) {
                if (sword_effect[index] != NULL) {
                    dest.sword_effect[index] = new (memory->Alloc(12)) CSWordAfterEffect;
                    sword_effect[index]->Copy(*dest.sword_effect[index], memory);
                }
            }
            copy_size = free_space - (memory->stack_size - memory->stack_used);
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", Copy__11CCharacter2FR11CCharacter2P9mgCMemory);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/character", __as__7CObjectFRC7CObject);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", skin_tag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_1575__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_281__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_282__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_283__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_284__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_285__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_286__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_287__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_288__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_289__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_290__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_291__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_292__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_293__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_294__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_295__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_296__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_297__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_298__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_299__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_300__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_301__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_302__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_303__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_304__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_305__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_306__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_307__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_308__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_309__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_310__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_311__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_312__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_313__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_314__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_315__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_316__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_317__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_318__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_319__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_1395__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_1522__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_1570__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", at_1571__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/character", __vt__11CCharacter2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(root_skin_frame, 0x4);
INCLUDE_BSS(skin_frame, 0x4);
INCLUDE_BSS(skin_name_ptr, 0x4);
INCLUDE_BSS(nowChr, 0x4);
INCLUDE_BSS(parent_chr, 0x4);
INCLUDE_BSS(outline_flag, 0x4);
INCLUDE_BSS(now_motion_id, 0x4);
INCLUDE_BSS(now_key_ptr, 0x4);
INCLUDE_BSS(now_seqhd_ptr, 0x4);
INCLUDE_BSS(now_seq_ptr, 0x4);
INCLUDE_BSS(now_cloth_id, 0x4);
INCLUDE_BSS(outline_start, 0x4);
INCLUDE_BSS(outline_start_tex, 0x4);
INCLUDE_BSS(outline_tex_id, 0x4);
INCLUDE_BSS(alloc_vertex_num, 0x4);
INCLUDE_BSS(alloc_shadow_vertex_num, 0x4);
INCLUDE_BSS(base_stack, 0x4);
INCLUDE_BSS(ext_stack, 0x4);
INCLUDE_BSS(img_stack, 0x4);
INCLUDE_BSS(now_stack, 0x4);
INCLUDE_BSS(set_imgblock, 0x4);
INCLUDE_BSS(pack_file, 0x4);
INCLUDE_BSS(outline_num_1499, 0x4);
INCLUDE_BSS(init_1500, 0x4);
INCLUDE_BSS(now_se_header, 0x4);
INCLUDE_BSS(eff_pack_ptr, 0x4);
INCLUDE_BSS(eff_pack_size, 0x4);
INCLUDE_BSS(load_img_ptr, 0x4);
INCLUDE_BSS(load_img_size, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(alloc_vertex, 0x190);
INCLUDE_BSS(img_ptr, 0x20);
INCLUDE_BSS(skin_mds_name, 0x40);
