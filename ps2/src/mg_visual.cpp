#include "common.h"
#include "mg_visual.hpp"

#include <cstring>

#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"

#ifdef NONMATCHING
/**
 * Initial transform and lighting upload of an MDT visual's setup packet.
 */
struct mgVISUAL_SETUP_PACKET {
    u_int          dma[4];        /**< DMA count tag for the initial upload. */
    u_int          vif[4];        /**< Buffer layout and upload commands. */
    u_long128      unk_20;
    sceVu0FMATRIX  model_screen;  /**< Transform from model space to GS screen space. */
    sceVu0FMATRIX  model_world;   /**< Transform from model space to world space. */
    sceVu0FVECTOR  light_dir[3];  /**< Three directional-light vectors sent to VU1. */
    sceVu0FMATRIX  light_color;   /**< Directional-light colours. */
    sceVu0FVECTOR  ambient;       /**< Ambient light with the object's alpha factor. */
    sceVu0FVECTOR  object_color;  /**< Object colour with the object's alpha factor. */
};
STATIC_ASSERT(sizeof(mgVISUAL_SETUP_PACKET) == 0x140);

static u_long128 *SetData0(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData1(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData2(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData3(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData4(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData5(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData6(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);
static u_long128 *SetData7(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour);

static u_long128 *(*set_data_func[8])(int, int, int **, u_long128 *, u_long128 *, u_long128 *, u_long128 *, u_long128 *) = {
    SetData0, SetData1, SetData2, SetData3, SetData4, SetData5, SetData6, SetData7
}; /**< Vertex upload writers selected by the face attributes. */

#endif

static void CopyMaterial(mgMaterial *material, MDT_MATERIAL_ *source, mgCTextureManager *texture_manager);

static u_int texflush_dma[3][4] __attribute__((aligned(16))) = {
    {0x10000002, 0, 0, 0x50000002},
    {0x8001, 0x10000000, 0xE, 0},
    {0, 0, SCE_GS_TEXFLUSH, 0}
}; /**< DMA chain that flushes the GS texture cache. */

// Code (.text)
u_int *GetScrPad() {
    return (u_int *)(buff_id != 0 ? 0x70002000 : 0x70000000);
}

void SendDMA(void *packet, int size) {
    packet = (void *)((u_int)packet & 0x0FFFFFFF);
    if (start_dma != 0) {
        asm {
        dma_wait:
            nop
            nop
            nop
            nop
            nop
            nop
            bc0f dma_wait
            nop
        }
        start_dma = 0;
    }
    *(volatile u_int *)0x1000E010 = 0x100;
    DmaCH8->sadr = (u_int)GetScrPad() & 0x0FFFFFFF;
    DmaCH8->madr = (u_int)packet;
    DmaCH8->qwc = size;
    DmaCH8->chcr.STR = 1;
    start_dma = 1;
    buff_id = !buff_id;
}

int mgSetPkTEX0(u_int *packet, u_long tex0, u_long tex1) {
    *(u_long128 *)&packet[0] = *(u_long128 *)&set_tex0_dma;
    *(u_long128 *)&packet[4] = *(u_long128 *)&set_tex0_giftag;
    *(u_long *)&packet[8] = tex1;
    *(u_long *)&packet[10] = SCE_GS_TEX1_1;
    *(u_long *)&packet[12] = tex0;
    *(u_long *)&packet[14] = SCE_GS_TEX0_1;
    return 4;
}

int mgSetPkTEX0(u_int *packet, u_long tex0, u_long tex1, u_long texa) {
    *(u_long128 *)&packet[0] = *(u_long128 *)&set_texa_dma;
    *(u_long128 *)&packet[4] = *(u_long128 *)&set_texa_giftag;
    *(u_long *)&packet[8] = tex1;
    *(u_long *)&packet[10] = SCE_GS_TEX1_1;
    *(u_long *)&packet[12] = tex0;
    *(u_long *)&packet[14] = SCE_GS_TEX0_1;
    *(u_long *)&packet[16] = texa;
    *(u_long *)&packet[18] = SCE_GS_TEXA;
    return 5;
}

int mgSetPkTexFlush_TagCnt(u_int *packet) {
    if (packet == NULL) {
        return 3;
    }
    *(u_long128 *)&packet[0] = *(u_long128 *)texflush_dma[0];
    *(u_long128 *)&packet[4] = *(u_long128 *)texflush_dma[1];
    *(u_long128 *)&packet[8] = *(u_long128 *)texflush_dma[2];
    return 3;
}

/**
 * Writes the two point-light matrices into a VU1 upload packet.
 */
static int SetPointLight(u_int *packet, float (*matrix0)[4], float (*matrix1)[4]) {
    packet[0] = 0x10000008;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = MG_VIF_UNPACK_V4_32 | (8 << MG_VIF_NUM_SHIFT) | 0x002D;
    *(u_long128 *)&packet[4] = *(u_long128 *)matrix0[0];
    *(u_long128 *)&packet[8] = *(u_long128 *)matrix0[1];
    *(u_long128 *)&packet[12] = *(u_long128 *)matrix0[2];
    *(u_long128 *)&packet[16] = *(u_long128 *)matrix0[3];
    *(u_long128 *)&packet[20] = *(u_long128 *)matrix1[0];
    *(u_long128 *)&packet[24] = *(u_long128 *)matrix1[1];
    *(u_long128 *)&packet[28] = *(u_long128 *)matrix1[2];
    *(u_long128 *)&packet[32] = *(u_long128 *)matrix1[3];
    return 9;
}

#ifdef NONMATCHING
int mgCVisualMDT::SetMaterialRef(u_long128 *packet, mgMaterial *material, int flags) {
    mgCTexture *texture;

    texture = material->texture;
    if (texture == NULL) {
        packet[0] = mat_vif;
        packet[1] = *(u_long128 *)material->diffuse;
        packet[2] = *(u_long128 *)material->unk_10;
        packet[3] = 0;
        packet[4] = mat_pw;
        prev_tex = NULL;
        return 5;
    }
    if (flags & 0x1) {
        packet[0] = mat_vif;
        packet[1] = *(u_long128 *)material->diffuse;
        packet[2] = *(u_long128 *)material->unk_10;
        packet[3] = 0;
        packet[4] = 3;
        packet[5] = mat_vif_d_tex;
        *(u_long *)&packet[6] = (u_long)0x30000000 << 32 | 0x8001;
        ((u_long *)&packet[6])[1] = 0x86E;
        *(u_long *)&packet[7] = *(u_long *)&texture->tex1;
        ((u_long *)&packet[7])[1] = SCE_GS_TEX1_1;
        *(u_long *)&packet[8] = texture->tex0.value;
        *(u_long *)&packet[9] = *(u_long *)&texture->clamp;
        prev_tex = texture;
        return 10;
    } else {
        packet[0] = mat_vif_dif;
        packet[1] = *(u_long128 *)material->diffuse;
        packet[2] = mat_vif_d_tex;
        *(u_long *)&packet[3] = (u_long)0x30000000 << 32 | 0x8001;
        ((u_long *)&packet[3])[1] = 0x86E;
        *(u_long *)&packet[4] = *(u_long *)&texture->tex1;
        ((u_long *)&packet[4])[1] = SCE_GS_TEX1_1;
        *(u_long *)&packet[5] = texture->tex0.value;
        *(u_long *)&packet[6] = *(u_long *)&texture->clamp;
        prev_tex = texture;
        return 7;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetMaterialRef__12mgCVisualMDTFP1P10mgMateriali);
#endif

int mgCVisualMDT::SetPModeRef(u_long128 *packet, int type) {
    int mode;

    mode = prmode;
    if (type & MG_FACE_NO_TEXTURE) {
        mode &= ~0x10;
    }
    if (type & MG_FACE_FLAT) {
        mode &= ~0x8;
    }
    packet[0] = *(u_long128 *)&mat_vif_d;
    giftag[0] = 0x8001;
    packet[1] = *(u_long128 *)&giftag;
    *(u_long *)&packet[2] = mode;
    ((u_long *)&packet[2])[1] = SCE_GS_PRMODE;
    return 3;
}

void mgCVisualAttr::Initialize() {
    memset(this, 0, sizeof(mgCVisualAttr));
    alpha_ref = -1;
    z_write = MG_ZBUF_WRITE;
    dest_alpha_test = MG_DEST_ALPHA_TEST_OFF;
}

mgCVisualAttr::mgCVisualAttr() {
    Initialize();
}

mgCTextureManager *mgCVisual::GetTextureManager() {
    if (texture_manager != NULL) {
        return texture_manager;
    }
    return &mgTexManager;
}

int mgCVisual::SetDrawEnvGifTag(u_long128 *packet, mgRENDER_INFO *info, mgCDrawEnv *base) {
    mgCDrawEnv *env;

    env = (mgCDrawEnv *)packet;
    *env = *base;
    if (info->attr->alpha_ref >= 0) {
        env->test.bits.aref = info->attr->alpha_ref;
    }
    if (info->attr->z_test != 0) {
        env->test.bits.zte = 1;
        if (info->attr->z_test == -1) {
            env->test.bits.ztst = 1;
        }
        if (info->attr->z_test == 1) {
            env->test.bits.ztst = 2;
        }
        if (info->attr->z_test == 2) {
            env->test.bits.ztst = 3;
        }
    }
    if (info->attr->alpha_test > 0) {
        env->test.bits.ate = 1;
        env->test.bits.atst = info->attr->alpha_test;
    } else if (info->attr->alpha_test == -1) {
        env->test.bits.ate = 0;
    }
    if (info->attr->dest_alpha_test != 0) {
        if (info->attr->dest_alpha_test == MG_DEST_ALPHA_TEST_OFF) {
            env->test.bits.date = 0;
        } else if (info->attr->dest_alpha_test == MG_DEST_ALPHA_TEST_ZERO) {
            env->test.bits.date = 1;
            env->test.bits.datm = 0;
        } else if (info->attr->dest_alpha_test == MG_DEST_ALPHA_TEST_ONE) {
            env->test.bits.date = 1;
            env->test.bits.datm = 1;
        }
    }
    if (info->attr->alpha_blend != 0) {
        env->SetAlpha(info->attr->alpha_blend);
    }
    env->SetZBuf(info->attr->z_write);
    return 4;
}

void mgCVisualMDT::Initialize(void) {
    vertex_num = 0;
    vertex = NULL;
    normal_num = 0;
    normal = NULL;
    colour_num = 0;
    colour = NULL;
    uv_num = 0;
    uv = NULL;
    material_num = 0;
    material = NULL;
    face_group = NULL;
    unk_00 = 0;
    draw_env = NULL;
    texture_manager = NULL;
    vu1_offset = 0;
    vu1_base = 0;
    vu1_base = 60;
    vu1_offset = 180;
}

/**
 * Copies an MDT material's colours and resolves its texture name.
 */
#ifdef NONMATCHING
static void CopyMaterial(mgMaterial *material, MDT_MATERIAL_ *source, mgCTextureManager *texture_manager) {
    material->diffuse[0] = source->diffuse[0];
    material->diffuse[1] = source->diffuse[1];
    material->diffuse[2] = source->diffuse[2];
    material->diffuse[3] = source->diffuse[3];
    material->unk_10[0] = source->unk_10[0];
    material->unk_10[1] = source->unk_10[1];
    material->unk_10[2] = source->unk_10[2];
    material->unk_10[3] = source->unk_10[3];
    material->texture = texture_manager->GetTexture(source->texture, -1);
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CopyMaterial__FP10mgMaterialP13MDT_MATERIAL_P17mgCTextureManager);
#endif

#ifdef NONMATCHING
void mgCVisualMDT::CopyMDTData(MDT_HEADER *header, mgCMemory *memory) {
    mgCTextureManager *textures;
    sceVu0FVECTOR     *source_vertex;
    sceVu0FVECTOR     *source_normal;
    sceVu0FVECTOR     *source_colour;
    sceVu0FVECTOR     *source_uv;
    MDT_MATERIAL_     *source_material;
    int                i;

    textures = GetTextureManager();
    source_vertex = (sceVu0FVECTOR *)((u_char *)header + header->vertex_ofs);
    source_normal = (sceVu0FVECTOR *)((u_char *)header + header->normal_ofs);
    source_colour = (sceVu0FVECTOR *)((u_char *)header + header->colour_ofs);
    source_uv = (sceVu0FVECTOR *)((u_char *)header + header->uv_ofs);
    source_material = (MDT_MATERIAL_ *)((u_char *)header + header->material_ofs);
    vertex_num = header->vertex_num;
    normal_num = header->normal_num;
    colour_num = header->colour_num;
    uv_num = header->uv_num;
    material_num = header->material_num;
    vertex = (sceVu0FVECTOR *)memory->Alloc(vertex_num);
    normal = (sceVu0FVECTOR *)memory->Alloc(normal_num);
    uv = (sceVu0FVECTOR *)memory->Alloc(uv_num);
    colour = (sceVu0FVECTOR *)memory->Alloc(colour_num);
    material = (mgMaterial *)memory->Alloc(material_num * (int)sizeof(mgMaterial) / 16);
    if (vertex != NULL) {
        for (i = 0; i < vertex_num; i++) {
            sceVu0CopyVector(vertex[i], source_vertex[i]);
        }
    }
    if (normal != NULL) {
        for (i = 0; i < normal_num; i++) {
            sceVu0CopyVector(normal[i], source_normal[i]);
        }
    }
    if (colour != NULL) {
        for (i = 0; i < colour_num; i++) {
            sceVu0CopyVector(colour[i], source_colour[i]);
        }
    }
    if (uv != NULL) {
        for (i = 0; i < uv_num; i++) {
            sceVu0CopyVector(uv[i], source_uv[i]);
        }
    }
    if (material != NULL) {
        for (i = 0; i < material_num; i++) {
            CopyMaterial(&material[i], &source_material[i], textures);
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CopyMDTData__12mgCVisualMDTFP10MDT_HEADERP9mgCMemory);
#endif

void mgCVisualMDT::CopyMDTDataPointer(MDT_HEADER *header, mgCMemory *memory) {
    mgCTextureManager *textures;
    MDT_MATERIAL_     *source_material;
    int                i;

    textures = GetTextureManager();
    sceVu0FVECTOR *vertex_data = (sceVu0FVECTOR *)((u_char *)header + header->vertex_ofs);
    sceVu0FVECTOR *normal_data = (sceVu0FVECTOR *)((u_char *)header + header->normal_ofs);
    sceVu0FVECTOR *colour_data = (sceVu0FVECTOR *)((u_char *)header + header->colour_ofs);
    sceVu0FVECTOR *uv_data = (sceVu0FVECTOR *)((u_char *)header + header->uv_ofs);
    source_material = (MDT_MATERIAL_ *)((u_char *)header + header->material_ofs);
    vertex_num = header->vertex_num;
    normal_num = header->normal_num;
    colour_num = header->colour_num;
    uv_num = header->uv_num;
    material_num = header->material_num;
    vertex = vertex_data;
    normal = normal_data;
    colour = colour_data;
    uv = uv_data;
    material = (mgMaterial *)memory->Alloc(material_num * (int)sizeof(mgMaterial) / 16);
    if (material != NULL) {
        for (i = 0; i < material_num; i++) {
            CopyMaterial(&material[i], &source_material[i], textures);
        }
    }
}

mgMaterial *mgCVisualMDT::GetMaterial(int index) {
    if (material == NULL) {
        return NULL;
    }
    if (index < 0 || index >= material_num) {
        return NULL;
    }
    return &material[index];
}

sceVu0FVECTOR *mgCVisualMDT::GetColor(int *num) {
    *num = colour_num;
    return colour;
}

int mgCVisualMDT::CreateBBox(float *max, float *min, float (*matrix)[4]) {
    if (vertex_num <= 0) {
        return 0;
    }
    if (vertex == NULL) {
        return 0;
    }
    mgVectorMinMaxN(max, min, vertex, vertex_num);
    return 1;
}

#ifdef NONMATCHING
FACES_ID *mgCVisualMDT::CreateFace(FACES_ID *faces, mgCMemory *memory, mgCMemory *index_memory, mgCFace **out_face) {
    mgCFace      *face;
    mgCFace      *last_face;
    mgFACE_GROUP *group;
    mgFACE_GROUP *previous;
    int          *indices;
    int          *write;
    int           i;

    GetTextureManager();
    face = (mgCFace *)memory->Alloc(3);
    face->vertex_num = (u_short)faces->face_num;
    face->type = faces->type;
    face->index_stride = 3;
    if (face->type & MG_FACE_COLOUR) {
        face->index_stride++;
    }
    if (face->type & MG_FACE_NO_NORMAL) {
        face->index_stride--;
    }
    if (face->type & MG_FACE_NO_TEXTURE) {
        face->index_stride--;
    }
    face->index_num = face->vertex_num * face->index_stride;
    face->material = (u_short)faces->material;
    indices = faces->index;
    write = (int *)index_memory->Alloc(face->index_num / 4 + 1);
    face->index = write;
    for (i = 0; i < face->index_num; i++) {
        *write++ = *indices++;
    }
    face->next = NULL;
    previous = face_group;
    if (previous == NULL) {
        group = new (memory->Alloc(4)) mgFACE_GROUP;
        if (group != NULL) {
            memset(group, 0, sizeof(mgFACE_GROUP));
        }
        group->next = NULL;
        group->face = NULL;
        group->material = face->material;
        face_group = group;
    } else {
        while (previous->next != NULL) {
            if (previous->material == face->material && previous->vu_program == 0) {
                break;
            }
            previous = previous->next;
        }
        group = previous;
        if (previous->next == NULL) {
            group = new (memory->Alloc(4)) mgFACE_GROUP;
            if (group != NULL) {
                memset(group, 0, sizeof(mgFACE_GROUP));
            }
            previous->next = group;
            group->next = NULL;
            group->face = NULL;
            group->material = face->material;
            group->vu_program = 0;
        }
    }
    last_face = group->face;
    if (last_face == NULL) {
        group->face = face;
    } else {
        while (last_face->next != NULL) {
            last_face = last_face->next;
        }
        last_face->next = face;
    }
    if (out_face != NULL) {
        *out_face = face;
    }
    return (FACES_ID *)indices;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreateFace__12mgCVisualMDTFP8FACES_IDP9mgCMemoryP9mgCMemoryPP7mgCFace);
#endif

int mgCVisualMDT::DataAssignMDT(MDT_HEADER *header, mgCMemory *memory, mgCTextureManager *textures) {
    MDT_FACES *section;
    FACES_ID  *faces;
    int        count;
    int        i;

    if (header == NULL) {
        return 0;
    }
    if (textures == NULL) {
        textures = &mgTexManager;
    }
    texture_manager = textures;
    CopyMDTData(header, memory);
    face_group = NULL;
    section = (MDT_FACES *)((u_char *)header + header->faces_ofs);
    count = section->prim_num;
    faces = (FACES_ID *)(section + 1);
    for (i = 0; i < count; i++) {
        faces = CreateFace(faces, memory, memory, NULL);
    }
    return 1;
}

int mgCVisualFixMDT::DataAssignMDT(MDT_HEADER *header, mgCMemory *memory, mgCTextureManager *textures) {
    u_long128  index_buffer[0x4B00];
    mgCMemory  index_memory;
    MDT_FACES *section;
    mgCFace   *face;
    u_int     *packet;
    int        count;
    int        size;
    int        i;

    index_memory.stSetBuffer(index_buffer, 0x4B00);
    if (textures == NULL) {
        textures = &mgTexManager;
    }
    texture_manager = textures;
    CopyMDTDataPointer(header, memory);
    face_group = NULL;
    section = (MDT_FACES *)((u_char *)header + header->faces_ofs);
    count = section->prim_num;
    FACES_ID *faces = (FACES_ID *)(section + 1);
    for (int i = 0; i < count; i++) {
        index_memory.stack_used = 0;
        index_memory.lock = 0;
        faces = CreateFace(faces, memory, &index_memory, &face);
        u_int *packet = (u_int *)&memory->stack[memory->stack_used];
        size = CreateFacePacket(packet, face);
        ((u_int *)&face->packet_tag)[0] = size | 0x30000000;
        ((u_int *)&face->packet_tag)[1] = (u_int)packet;
        ((u_int *)&face->packet_tag)[2] = 0;
        ((u_int *)&face->packet_tag)[3] = 0;
        memory->Alloc(size);
    }
    return 1;
}

int mgCVisualMDT::Draw(u_int *packet, float (*matrix)[4], mgCDrawManager *draw_manager) {
    mgCMemory    *memory;
    u_long128    *common;
    mgFACE_GROUP *group;
    mgCTexture   *texture;
    u_int        *write;

    if (draw_manager == NULL) {
        draw_manager = &mgDrawManager;
    }
    mgRENDER_INFO *info = draw_manager->render_info;
    texture_manager = draw_manager->texture_manager;
    prev_tex = NULL;
    memory = draw_manager->data_memory;
    common = &memory->stack[memory->stack_used];
    memory->stack_used += CreateRenderInfoPacket((u_int *)common, matrix, info);
    CreatePacket(draw_manager);
    if (packet != NULL) {
        packet[0] = MG_DMA_CALL;
        packet[1] = (u_int)common;
        packet[2] = 0;
        packet[3] = 0;
        write = packet + 4;
        for (group = face_group; group != NULL; group = group->next) {
            write += mgSendVuProg(write, group->vu_program);
            write[0] = MG_DMA_CALL;
            write[1] = (u_int)group->packet;
            write[2] = 0;
            write[3] = 0;
            write += 4;
        }
        return (write - packet) / 4;
    }
    for (group = face_group; group != NULL; group = group->next) {
        texture = material[group->material].texture;
        if (texture != NULL) {
            draw_manager->AddPacket(texture->block, common, group->packet, group->vu_program);
        } else {
            draw_manager->AddPacket(-1, common, group->packet, group->vu_program);
        }
    }
    return 0;
}

#ifdef NONMATCHING
u_int mgCVisualMDT::CreatePacket(mgCDrawManager *draw_manager) {
    mgCMemory     *packet_memory;
    mgCMemory     *data_memory;
    mgRENDER_INFO *info;
    mgFACE_GROUP  *group;
    mgCFace       *face;
    u_long128     *packet_start;
    u_long128     *data_start;
    u_long128     *packet;
    u_long128     *data;
    u_int         *tag;
    u_short        previous_type;
    int            size;

    GetTextureManager();
    packet_memory = draw_manager->packet_memory;
    data_memory = draw_manager->data_memory;
    info = draw_manager->render_info;
    packet_start = &packet_memory->stack[packet_memory->stack_used];
    data_start = &data_memory->stack[data_memory->stack_used];
    packet = packet_start;
    data = data_start;
    prev_tex = NULL;
    for (group = face_group; group != NULL; group = group->next) {
        group->packet = packet;
        size = SetMaterialRef((u_long128 *)((u_int)data | MG_UNCACHED), &material[group->material], info->attr->program_mode);
        ((u_int *)packet)[0] = size | 0x30000000;
        ((u_int *)packet)[1] = (u_int)data;
        ((u_int *)packet)[2] = 0;
        ((u_int *)packet)[3] = 0;
        packet++;
        data += size;
        previous_type = 0xFFFF;
        for (face = group->face; face != NULL; face = face->next) {
            if (previous_type != face->type) {
                SetPModeRef((u_long128 *)((u_int)data | MG_UNCACHED), face->type);
                ((u_int *)packet)[0] = 0x30000003;
                ((u_int *)packet)[1] = (u_int)data;
                ((u_int *)packet)[2] = 0;
                ((u_int *)packet)[3] = 0;
                packet++;
                data += 3;
                previous_type = face->type;
            }
            tag = (u_int *)packet++;
            tag[0] = 0x30000000;
            tag[1] = (u_int)data;
            tag[2] = 0;
            tag[3] = 0;
            size = CreateFacePacket((u_int *)((u_int)data | MG_UNCACHED), face);
            data += size;
            tag[0] |= size;
        }
        packet += mgSetPkTexFlush_TagCnt((u_int *)packet);
        ((u_int *)packet)[0] = MG_DMA_RET;
        ((u_int *)packet)[1] = 0;
        ((u_int *)packet)[2] = 0;
        ((u_int *)packet)[3] = 0;
        packet++;
        group->packet_size = packet - group->packet;
    }
    packet_memory->stack_used += packet - packet_start;
    data_memory->stack_used += data - data_start;
    return (u_int)packet_start & 0x0FFFFFFF;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreatePacket__12mgCVisualMDTFP14mgCDrawManager);
#endif

#ifdef NONMATCHING
u_int mgCVisualFixMDT::CreatePacket(mgCDrawManager *draw_manager) {
    mgCMemory     *packet_memory;
    mgCMemory     *data_memory;
    mgRENDER_INFO *info;
    mgFACE_GROUP  *group;
    mgCFace       *face;
    u_long128     *packet_start;
    u_long128     *data_start;
    u_long128     *packet;
    u_long128     *data;
    u_short        previous_type;
    int            size;

    GetTextureManager();
    packet_memory = draw_manager->packet_memory;
    data_memory = draw_manager->data_memory;
    info = draw_manager->render_info;
    packet_start = &packet_memory->stack[packet_memory->stack_used];
    data_start = &data_memory->stack[data_memory->stack_used];
    packet = packet_start;
    data = data_start;
    packet += mgSetPkTexFlush_TagCnt((u_int *)packet);
    prev_tex = NULL;
    for (group = face_group; group != NULL; group = group->next) {
        group->packet = packet;
        size = SetMaterialRef((u_long128 *)((u_int)data | MG_UNCACHED), &material[group->material], info->attr->program_mode);
        ((u_int *)packet)[0] = size | 0x30000000;
        ((u_int *)packet)[1] = (u_int)data;
        ((u_int *)packet)[2] = 0;
        ((u_int *)packet)[3] = 0;
        packet++;
        data += size;
        previous_type = 0xFFFF;
        for (face = group->face; face != NULL; face = face->next) {
            if (previous_type != face->type) {
                SetPModeRef((u_long128 *)((u_int)data | MG_UNCACHED), face->type);
                ((u_int *)packet)[0] = 0x30000003;
                ((u_int *)packet)[1] = (u_int)data;
                ((u_int *)packet)[2] = 0;
                ((u_int *)packet)[3] = 0;
                packet++;
                data += 3;
                previous_type = face->type;
            }
            *packet++ = face->packet_tag;
        }
        packet += mgSetPkTexFlush_TagCnt((u_int *)packet);
        ((u_int *)packet)[0] = MG_DMA_RET;
        ((u_int *)packet)[1] = 0;
        ((u_int *)packet)[2] = 0;
        ((u_int *)packet)[3] = 0;
        packet++;
        group->packet_size = packet - group->packet;
    }
    packet_memory->stack_used += packet - packet_start;
    data_memory->stack_used += data - data_start;
    return (u_int)packet_start;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreatePacket__15mgCVisualFixMDTFP14mgCDrawManager);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, normal, uv streams for one vertex batch.
 */
static u_long128 *SetData0(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *normal_out;
    u_long128 *uv_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = count;
    ((int *)packet)[2] = count;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    normal_out = vertex_out + count;
    uv_out = normal_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *normal_out++ = normal[cursor[1]];
        *uv_out++ = uv[cursor[2]];
        cursor += 3;
    }
    *index = cursor;
    return uv_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData0__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, normal, uv, colour streams for one vertex batch.
 */
static u_long128 *SetData1(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *normal_out;
    u_long128 *uv_out;
    u_long128 *colour_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = count;
    ((int *)packet)[2] = count;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    normal_out = vertex_out + count;
    uv_out = normal_out + count;
    colour_out = uv_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *normal_out++ = normal[cursor[1]];
        *uv_out++ = uv[cursor[2]];
        *colour_out++ = colour[cursor[3]];
        cursor += 4;
    }
    *index = cursor;
    return colour_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData1__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, normal streams for one vertex batch.
 */
static u_long128 *SetData2(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *normal_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = count;
    ((int *)packet)[2] = count;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    normal_out = vertex_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *normal_out++ = normal[cursor[1]];
        cursor += 2;
    }
    *index = cursor;
    return normal_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData2__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, normal, colour streams for one vertex batch.
 */
static u_long128 *SetData3(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *normal_out;
    u_long128 *colour_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = count;
    ((int *)packet)[2] = 0;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    normal_out = vertex_out + count;
    colour_out = normal_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *normal_out++ = normal[cursor[1]];
        *colour_out++ = colour[cursor[2]];
        cursor += 3;
    }
    *index = cursor;
    return colour_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData3__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, uv streams for one vertex batch.
 */
static u_long128 *SetData4(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *uv_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = 0;
    ((int *)packet)[2] = count;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    uv_out = vertex_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *uv_out++ = uv[cursor[1]];
        cursor += 2;
    }
    *index = cursor;
    return uv_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData4__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, uv, colour streams for one vertex batch.
 */
static u_long128 *SetData5(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *uv_out;
    u_long128 *colour_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = 0;
    ((int *)packet)[2] = count;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    uv_out = vertex_out + count;
    colour_out = uv_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *uv_out++ = uv[cursor[1]];
        *colour_out++ = colour[cursor[2]];
        cursor += 3;
    }
    *index = cursor;
    return colour_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData5__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex streams for one vertex batch.
 */
static u_long128 *SetData6(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = 0;
    ((int *)packet)[2] = 0;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        cursor += 1;
    }
    *index = cursor;
    return vertex_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData6__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
/**
 * Writes the indexed vertex, colour streams for one vertex batch.
 */
static u_long128 *SetData7(int count, int type, int **index, u_long128 *packet, u_long128 *vertex, u_long128 *normal, u_long128 *uv, u_long128 *colour) {
    int       *cursor;
    u_long128 *vertex_out;
    u_long128 *colour_out;

    ((int *)packet)[0] = count;
    ((int *)packet)[1] = 0;
    ((int *)packet)[2] = 0;
    ((int *)packet)[3] = type;
    cursor = *index;
    vertex_out = packet + 1;
    colour_out = vertex_out + count;
    while (count > 0) {
        count--;
        *vertex_out++ = vertex[cursor[0]];
        *colour_out++ = colour[cursor[1]];
        cursor += 2;
    }
    *index = cursor;
    return colour_out;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", SetData7__FiiPPiP1P1P1P1P1);
#endif

#ifdef NONMATCHING
int mgCVisualMDT::CreateFacePacket(u_int *packet, mgCFace *face) {
    static u_int prog_vif[4] __attribute__((aligned(16))) = {0, 0, 0, MG_VIF_MSCAL | 0x2};
    static u_int progf_vif[4] __attribute__((aligned(16))) = {0, 0, 0, MG_VIF_MSCNT};
    sceGifTag  batch_tag;
    sceGifTag  end_tag;
    u_int      finish[4] __attribute__((aligned(16))) = {MG_VIF_FLUSHA, 0, 0, 0};
    int       *indices;
    u_int     *destination;
    u_int     *write;
    u_int     *buffer_start;
    u_int     *unpack;
    u_long128 *end;
    short      remaining;
    short      batch_limit;
    short      count;
    int        variant;
    int        primitive;
    int        use_scratchpad;
    int        started;
    int        words;

    if (face == NULL) {
        return 0;
    }
    use_scratchpad = 0;
    if (((u_int)packet & 0xF0000000) == MG_UNCACHED) {
        use_scratchpad = 1;
    }
    remaining = face->vertex_num;
    primitive = face->type & MG_FACE_PRIM_MASK;
    indices = face->index;
    variant = 0;
    started = 0;
    batch_limit = (vu1_offset - 2) / 3 / 3 * 3;
    if (face->type & MG_FACE_COLOUR) {
        variant = 1;
        batch_limit = (vu1_offset - 2) / 4 / 3 * 3;
    }
    if (face->type & MG_FACE_NO_TEXTURE) {
        variant += 2;
    }
    if (face->type & MG_FACE_NO_NORMAL) {
        variant += 4;
    }
    *(u_long128 *)&batch_tag = 0;
    batch_tag.EOP = 1;
    batch_tag.PRE = 1;
    end_tag = batch_tag;
    if (primitive == MG_PRIM_TRIANGLE_STRIP) {
        batch_tag.PRIM = 0x5C;
    } else {
        batch_tag.PRIM = 0x5B;
    }
    batch_tag.NREG = 3;
    batch_tag.REGS0 = 2;
    batch_tag.REGS1 = 1;
    batch_tag.REGS2 = 4;
    end_tag.PRIM = 0x5D;
    end_tag.NREG = 3;
    end_tag.REGS0 = 2;
    end_tag.REGS1 = 1;
    end_tag.REGS2 = 4;
    packet[0] = 0;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = MG_VIF_UNPACK_V4_32 | (1 << MG_VIF_NUM_SHIFT) | 0x0027;
    *(u_long128 *)&packet[4] = *(u_long128 *)&end_tag;
    destination = packet + 8;
    write = use_scratchpad ? GetScrPad() : destination;
    buffer_start = write;
    while (remaining > 0) {
        count = batch_limit;
        if (remaining < batch_limit) {
            count = remaining;
        }
        write[0] = 0;
        write[1] = 0;
        write[2] = 0;
        write[3] = 0;
        unpack = write + 3;
        batch_tag.NLOOP = count | 0x8000;
        *(u_long128 *)&write[4] = *(u_long128 *)&batch_tag;
        end = set_data_func[variant](count, face->type, &indices, (u_long128 *)&write[8],
                                    (u_long128 *)vertex, (u_long128 *)normal, (u_long128 *)uv, (u_long128 *)colour);
        *unpack = (((u_int *)end - (write + 4)) / 4 << MG_VIF_NUM_SHIFT) | MG_VIF_UNPACK_V4_32 | MG_VIF_UNPACK_FLG;
        if (started == 0) {
            started = 1;
            *end = *(u_long128 *)prog_vif;
        } else {
            *end = *(u_long128 *)progf_vif;
        }
        write = (u_int *)(end + 1);
        if (primitive == MG_PRIM_TRIANGLE_STRIP && batch_limit < remaining) {
            remaining += 2;
            indices -= face->index_stride * 2;
        }
        words = write - buffer_start;
        if (words > 0x514) {
            if (use_scratchpad != 0) {
                SendDMA(destination, words / 4);
            }
            destination += words;
            write = use_scratchpad ? GetScrPad() : destination;
            buffer_start = write;
        }
        remaining -= batch_limit;
    }
    words = write - buffer_start;
    if (use_scratchpad != 0 && words > 0) {
        SendDMA(destination, words / 4);
    }
    destination += words;
    *(u_long128 *)destination = *(u_long128 *)finish;
    destination += 4;
    return (destination - packet) / 4;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreateFacePacket__12mgCVisualMDTFPUiP7mgCFace);
#endif

#ifdef NONMATCHING
int mgCVisualMDT::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    mgVISUAL_SETUP_PACKET *setup;
    mgLIGHT_INFO          *lighting;
    mgCDrawEnv            *environment;
    u_int                 *start;
    u_int                 *write;
    sceVu0FMATRIX          projection;
    sceVu0FMATRIX          world_screen;
    sceVu0FMATRIX          inverse;
    sceVu0FMATRIX          model_clip;
    sceVu0FMATRIX          point_position;
    sceVu0FMATRIX          point_colour;
    sceVu0FVECTOR          boosted_ambient;
    sceVu0FVECTOR          eye;
    int                    flags;
    int                    fog_colour;
    int                    size;
    int                    i;

    if (info->attr == NULL) {
        packet[0] = MG_DMA_RET;
        packet[1] = 0;
        packet[2] = 0;
        packet[3] = 0;
        return 1;
    }
    start = GetScrPad();
    setup = (mgVISUAL_SETUP_PACKET *)start;
    lighting = info->GetpLightInfo();
    *(u_long128 *)setup->model_world[0] = *(u_long128 *)matrix[0];
    *(u_long128 *)setup->model_world[1] = *(u_long128 *)matrix[1];
    *(u_long128 *)setup->model_world[2] = *(u_long128 *)matrix[2];
    *(u_long128 *)setup->model_world[3] = *(u_long128 *)matrix[3];
    if (info->attr->depth_bias > 1.0f) {
        sceVu0CopyMatrix(projection, info->screen);
        projection[3][2] *= 1.005f;
        mgMulMatrix(world_screen, projection, info->world_view);
        mgMulMatrix(setup->model_screen, world_screen, setup->model_world);
    } else {
        mgMulMatrix(setup->model_screen, info->world_screen, setup->model_world);
    }
    setup->dma[0] = MG_DMA_CNT;
    setup->dma[1] = 0;
    setup->dma[2] = 0;
    setup->dma[3] = 0;
    setup->vif[0] = 0;
    setup->vif[1] = vu1_base | MG_VIF_BASE;
    setup->vif[2] = vu1_offset | MG_VIF_OFFSET;
    *(u_long128 *)setup->light_dir[0] = *(u_long128 *)lighting->light_dir[0];
    *(u_long128 *)setup->light_dir[1] = *(u_long128 *)lighting->light_dir[1];
    *(u_long128 *)setup->light_dir[2] = *(u_long128 *)lighting->light_dir[2];
    sceVu0CopyMatrix(setup->light_color, lighting->light_color);
    *(u_long128 *)setup->ambient = *(u_long128 *)lighting->ambient;
    setup->ambient[3] *= info->attr->obj_alpha;
    if (info->attr->ambient_boost != 0) {
        sceVu0ScaleVector(boosted_ambient, lighting->light_color[0], 0.3f);
        mgAddVector(boosted_ambient, lighting->ambient);
        *(u_long128 *)setup->object_color = *(u_long128 *)boosted_ambient;
        setup->object_color[3] = info->object_color[3];
    } else {
        *(u_long128 *)setup->object_color = *(u_long128 *)info->object_color;
    }
    setup->object_color[3] *= info->attr->obj_alpha;
    write = (u_int *)(setup + 1);
    setup->vif[3] = (((u_int)(write - (start + 4)) / 4 - 1) << MG_VIF_NUM_SHIFT) | MG_VIF_UNPACK_V4_32 | 0x0003;
    if (info->attr->program_mode != 0) {
        write[0] = 0;
        write[1] = 0;
        write[2] = 0;
        write[3] = MG_VIF_UNPACK_V4_32 | (1 << MG_VIF_NUM_SHIFT) | 0x0018;
        eye[0] = info->camera_pos[0];
        eye[1] = info->camera_pos[1];
        eye[2] = info->camera_pos[2];
        eye[3] = 1.0f;
        sceVu0CopyMatrix(inverse, matrix);
        sceVu0InversMatrix(inverse, inverse);
        sceVu0ApplyMatrix(eye, inverse, eye);
        *(u_long128 *)&write[4] = *(u_long128 *)eye;
        write += 8;
    }
    if (info->scissor != 0) {
        write[0] = 0;
        write[1] = 0;
        write[2] = 0;
        write[3] = MG_VIF_UNPACK_V4_32 | (8 << MG_VIF_NUM_SHIFT) | 0x0019;
        mgMulMatrix(model_clip, info->world_clip, matrix);
        sceVu0CopyMatrix((float (*)[4])&write[4], model_clip);
        *(u_long128 *)&write[20] = *(u_long128 *)info->clip_screen[0];
        *(u_long128 *)&write[24] = *(u_long128 *)info->clip_screen[1];
        *(u_long128 *)&write[28] = *(u_long128 *)info->clip_screen[2];
        *(u_long128 *)&write[32] = *(u_long128 *)info->clip_screen[3];
        if (info->attr->depth_bias > 1.0f) {
            ((float *)&write[32])[2] *= 1.0000685f;
        }
        write += 36;
    }
    setup->dma[0] |= (write - (start + 4)) / 4;
    if (info->plight_hit != 0 && info->unk_fac != 0) {
        for (i = 0; i < 4; i++) {
            sceVu0SubVector(point_position[i], lighting->point_light[i].pos, matrix[3]);
            point_position[i][3] = lighting->point_light[i].power;
            sceVu0CopyVector(point_colour[i], lighting->point_light[i].color);
        }
        write += SetPointLight(write, point_position, point_colour) * 4;
    }
    if (info->attr->program_mode & 0x2) {
        write[0] = MG_DMA_CNT | 4;
        write[1] = 0;
        write[2] = 0;
        write[3] = MG_VIF_UNPACK_V4_32 | (4 << MG_VIF_NUM_SHIFT) | 0x0019;
        mgMulMatrix((float (*)[4])&write[4], info->view, matrix);
        write += 20;
    }
    flags = 0;
    if ((info->clip | info->scissor) != 0) {
        flags |= 0x1;
    }
    if (info->scissor != 0) {
        flags |= 0x2;
    }
    if (info->attr->program_mode != 0) {
        if (info->attr->program_mode & 0x1) {
            flags |= 0x8;
        }
        if (info->attr->program_mode & 0x2) {
            flags |= 0x100;
        }
    }
    if (info->attr->program_option != 0) {
        flags |= 0x4;
    }
    if (info->plight_hit != 0) {
        flags |= 0x10;
    }
    if (info->motion != 0) {
        flags |= 0x40;
    }
    if (info->attr->no_light != 0 || info->unk_fac == 0 || info->attr->ambient_boost != 0) {
        flags |= 0x20;
    }
    if (info->attr->unk_84 == 1) {
        flags |= 0x80;
    }
    write[0] = MG_DMA_CNT | 10;
    write[1] = 0;
    write[2] = 0;
    write[3] = MG_VIF_UNPACK_V4_32 | (1 << MG_VIF_NUM_SHIFT) | 0x0026;
    write[4] = flags;
    write[5] = 0;
    write[6] = 0;
    write[7] = 0;
    write[8] = 0;
    write[9] = 0;
    write[10] = MG_VIF_MSCAL;
    write[11] = MG_VIF_DIRECT | 8;
    write[12] = 0x8003;
    write[13] = 0x10000000;
    write[14] = 0xE;
    write[15] = 0;
    write[16] = 0;
    write[17] = 0;
    write[18] = MG_GS_PRMODECONT;
    write[19] = 0;
    prmode = ((info->attr->fog != 0 && info->fog_enable != 0) << 5) | 0x58;
    write[20] = prmode;
    write[21] = 0;
    write[22] = SCE_GS_PRMODE;
    write[23] = 0;
    fog_colour = info->fog.r | (info->fog.g << 8) | (info->fog.b << 16);
    if (info->attr->fog >= 2) {
        if (info->attr->fog == 2) {
            fog_colour = 0;
        }
        if (info->attr->fog == 3) {
            fog_colour = 0xFFFFFF;
        }
    }
    write[24] = fog_colour;
    write[25] = 0;
    write[26] = 0x3D;
    write[27] = 0;
    write += 28;
    environment = draw_env;
    if (environment == NULL) {
        environment = &info->draw_env[0];
    }
    write += SetDrawEnvGifTag((u_long128 *)write, info, environment) * 4;
    write += CreateExtRenderInfoPacket(write, matrix, info) * 4;
    write[0] = MG_DMA_RET;
    write[1] = 0;
    write[2] = 0;
    write[3] = 0;
    write += 4;
    size = (write - start) / 4;
    SendDMA(packet, size);
    return size;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreateRenderInfoPacket__12mgCVisualMDTFPUiPA4_fP13mgRENDER_INFO);
#endif

int mgCVisualMDT::CreateExtRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) { return 0; }

#ifdef NONMATCHING
mgCVisual *mgCVisualFixMDT::Copy(mgCMemory *memory) {
    mgCVisualFixMDT *copy;
    u_int            bytes;
    u_int            quadwords;
    int              i;

    copy = new (memory->Alloc(7)) mgCVisualFixMDT;
    if (copy == NULL) {
        return NULL;
    }
    (mgCVisualMDT &)*copy = (mgCVisualMDT &)*this;
    if (material_num > 0) {
        bytes = material_num * sizeof(mgMaterial);
        quadwords = bytes / 16;
        if (bytes & 0xF) {
            quadwords = bytes / 16 + 1;
        }
        copy->material = new (memory->Alloc(quadwords + 2)) mgMaterial[material_num];
    }
    for (i = 0; i < material_num; i++) {
        copy->material[i] = material[i];
    }
    return copy;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", Copy__15mgCVisualFixMDTFP9mgCMemory);
#endif

mgCVisualMDT &mgCVisualMDT::operator=(const mgCVisualMDT &other) {
    unk_00 = other.unk_00;
    draw_env = other.draw_env;
    texture_manager = other.texture_manager;
    prmode = other.prmode;
    vu1_base = other.vu1_base;
    vu1_offset = other.vu1_offset;
    unk_18 = other.unk_18;
    vertex_num = other.vertex_num;
    normal_num = other.normal_num;
    colour_num = other.colour_num;
    uv_num = other.uv_num;
    vertex = other.vertex;
    normal = other.normal;
    colour = other.colour;
    uv = other.uv;
    material_num = other.material_num;
    material = other.material;
    face_group = other.face_group;
    return *this;
}

/**
 * Applies a visual's alpha and depth settings over a base draw environment.
 */
static void SetDrawEnv(mgCDrawEnv *env, mgCVisualAttr *attr, mgCDrawEnv *base) {
    *env = *base;
    if (attr->alpha_ref >= 0) {
        env->test.bits.aref = attr->alpha_ref;
    }
    env->test.bits.zte = 1;
    if (attr->z_test != 0) {
        if (attr->z_test == -1) {
            env->test.bits.ztst = 1;
        }
        if (attr->z_test == 1) {
            env->test.bits.ztst = 2;
        }
        if (attr->z_test == 2) {
            env->test.bits.ztst = 3;
        }
    }
    if (attr->alpha_test != 0) {
        if (attr->alpha_test == -1) {
            env->test.bits.ate = 0;
        } else {
            env->test.bits.ate = 1;
        }
        env->test.bits.atst = attr->alpha_test;
    }
    if (attr->dest_alpha_test != 0) {
        if (attr->dest_alpha_test == MG_DEST_ALPHA_TEST_OFF) {
            env->test.bits.date = 0;
        } else if (attr->dest_alpha_test == MG_DEST_ALPHA_TEST_ZERO) {
            env->test.bits.date = 1;
            env->test.bits.datm = 0;
        } else if (attr->dest_alpha_test == MG_DEST_ALPHA_TEST_ONE) {
            env->test.bits.date = 1;
            env->test.bits.datm = 1;
        }
    }
    if (attr->z_write > 0) {
        env->zbuf.bits.zmsk = 0;
    }
    if (attr->z_write < 0) {
        env->zbuf.bits.zmsk = 1;
    }
    if (attr->alpha_blend != 0) {
        env->SetAlpha(attr->alpha_blend);
    }
}

#ifdef NONMATCHING
int mgCVisualPrim::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    u_int      *start;
    u_int      *write;
    mgCDrawEnv *environment;
    u_int       tag[4] __attribute__((aligned(16))) = {0x10000007, 0, 0, 0x50000007};
    int         size;

    start = GetScrPad();
    *(u_long128 *)start = *(u_long128 *)tag;
    giftag[0] = 0x8002;
    *(u_long128 *)&start[4] = *(u_long128 *)&giftag;
    *(u_long *)&start[8] = 1;
    *(u_long *)&start[10] = MG_GS_PRMODECONT;
    *(u_long *)&start[12] = 0;
    *(u_long *)&start[14] = SCE_GS_TEXFLUSH;
    environment = (mgCDrawEnv *)&start[16];
    if (draw_env != NULL) {
        *environment = *draw_env;
    } else {
        SetDrawEnv(environment, &attr, &info->draw_env[0]);
    }
    write = (u_int *)(environment + 1);
    write[0] = MG_DMA_RET;
    write[1] = 0;
    write[2] = 0;
    write[3] = 0;
    write += 4;
    size = ((u_long128 *)write - (u_long128 *)start);
    SendDMA(packet, size);
    return size;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_visual", CreateRenderInfoPacket__13mgCVisualPrimFPUiPA4_fP13mgRENDER_INFO);
#endif

void mgCVisualPrim::Initialize() {
    unk_00 = 0;
    draw_env = NULL;
    texture_manager = NULL;
    vu1_offset = 0;
    vu1_base = 0;
    attr.Initialize();
}

int mgCVisualFixMDT::Iam() {
    return MG_VISUAL_KIND_FIX_MDT;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", giftag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", set_tex0_dma__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", set_tex0_giftag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", set_texa_dma__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", set_texa_giftag__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", texflush_dma__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", mat_vif__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", mat_vif_dif__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", mat_vif_d__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", mat_pw__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", mat_vif_d_tex__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", set_data_func__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", prog_vif_730__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", progf_vif_731__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", at_769__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", __vt__13mgCVisualPrim__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", __vt__15mgCVisualFixMDT__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_visual", __vt__12mgCVisualMDT__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(start_dma, 0x4);
INCLUDE_BSS(buff_id, 0x4);
INCLUDE_BSS(prev_tex, 0x4);
