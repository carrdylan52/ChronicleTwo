#include "common.h"
#include "mg_shadow.hpp"

#include <libgraph.h>
#include <libvu0.h>

#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_visual.hpp"

// Code (.text)

/**
 * Writes the packet that loads the shadow projection matrix into VU1 memory, followed by the GS
 * packets the shadow microprogram sends, and returns its length in quadwords.
 */
// Keep the matrix and GIF packet stores in order.
#pragma schedule off
static int SetShadowData(u_int *packet, float (*matrix)[4]) {
    // DMA CNT of eight quadwords, then VIF UNPACK of the matrix and the two GIF packets.
    packet[0] = 0x10000008;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = 0x6C080028;
    ((u_long128 *)packet)[1] = ((u_long128 *)matrix)[0];
    ((u_long128 *)packet)[2] = ((u_long128 *)matrix)[1];
    ((u_long128 *)packet)[3] = ((u_long128 *)matrix)[2];
    ((u_long128 *)packet)[4] = ((u_long128 *)matrix)[3];
    packet[20] = 0x8001;
    packet[21] = 0x102E8000;
    packet[22] = SCE_GIF_PACKED_AD;
    packet[23] = 0;
    packet[24] = 0x68;
    packet[25] = 0x80;
    packet[26] = 0x42;
    packet[27] = 0;
    packet[28] = 0x8001;
    packet[29] = 0x102E8000;
    packet[30] = SCE_GIF_PACKED_AD;
    packet[31] = 0;
    packet[32] = 0x62;
    packet[33] = 0x80;
    packet[34] = 0x42;
    packet[35] = 0;
    return 9;
}
#pragma schedule reset

#ifdef NONMATCHING
int mgCShadowMDT::CreateFacePacket(u_int *packet, mgCFace *face) {
    // VIF MSCAL that starts the shadow microprogram on each batch.
    static u_int prog_vif[4] __attribute__((aligned(16))) = {0, 0, 0, 0x14000002};

    if (face == NULL) {
        return 0;
    }

    // A packet in uncached memory is built in the scratchpad and copied out by DMA.
    int scratchpad = 0;
    if (((u_int)packet & 0xF0000000) == 0x20000000) {
        scratchpad = 1;
    }

    u_int *start = packet;
    int remain = face->vertex_num;
    int *index = face->index;

    sceGifTag tag;
    *(u_long128 *)&tag = 0;
    tag.EOP = 1;
    tag.PRE = 1;
    if ((face->type & MG_FACE_PRIM_MASK) != MG_PRIM_TRIANGLE) {
        return 0;
    }
    tag.PRIM = SCE_GS_SET_PRIM(MG_PRIM_TRIANGLE_FAN, 1, 1, 0, 1, 0, 0, 0, 0);
    tag.NREG = 2;
    tag.REGS0 = SCE_GS_RGBAQ;
    tag.REGS1 = SCE_GS_XYZF2;

    u_int *write = scratchpad ? GetScrPad() : packet;
    u_int *block = write;
    for (; remain > 0; remain -= 42) {
        int num = 42;
        if (remain < 42) {
            num = remain;
        }

        write[0] = 0;
        write[1] = 0;
        write[2] = 0;
        write[3] = 0;
        u_int *unpack = &write[3];
        u_int *data = &write[4];
        tag.NLOOP = num;
        ((u_long128 *)write)[1] = *(u_long128 *)&tag;
        write[8] = num;
        write[9] = num;
        write[10] = face->type;

        sceVu0FVECTOR *vertex = this->vertex;
        u_long128 *out = &((u_long128 *)write)[3];
        for (; num > 0; num--) {
            out[0] = *(u_long128 *)vertex[index[0]];
            out[1] = *(u_long128 *)vertex[index[1]];
            out[2] = *(u_long128 *)vertex[index[2]];
            index += 3;
            out += 3;
        }
        *unpack = (((u_int)((u_int *)out - data) / 4) << 16) | 0x6C008000;
        *out = *(u_long128 *)prog_vif;
        write = (u_int *)(out + 1);

        int size = write - block;
        if (size > 0x514) {
            if (scratchpad) {
                SendDMA(packet, size / 4);
            }
            packet += size;
            write = scratchpad ? GetScrPad() : packet;
            block = write;
        }
    }

    int size = write - block;
    if (scratchpad && size > 0) {
        SendDMA(packet, size / 4);
    }
    packet += size;

    // VIF FLUSHA.
    u_int flush[4] = {0x13000000, 0, 0, 0};
    *(u_long128 *)packet = *(u_long128 *)flush;
    return (packet + 4 - start) / 4;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_shadow", CreateFacePacket__12mgCShadowMDTFPUiP7mgCFace);
#endif

// Preserve the signed face counts while allocating and copying their index data.
#pragma schedule off
#pragma global_optimizer off
FACES_ID *mgCShadowMDT::CreateFace(FACES_ID *faces, mgCMemory *memory, mgCMemory *index_memory,
                               mgCFace **face_out) {
    mgCFace *face = new (memory->Alloc(5)) mgCFace;
    face->vertex_num = faces->face_num / 3;
    face->type = faces->type_low;
    face->index_stride = 3;
    face->index_num = face->vertex_num * face->index_stride;
    // Only the position index of each of a triangle's three vertices is kept.
    face->material = faces->material_low;
    int *vertex;
    faces = (FACES_ID *)(vertex = faces->index);
    int *index = (int *)index_memory->Alloc(face->index_num * 4 / 16 + 16);
    face->index = index;
    for (int i = 0; i < face->vertex_num; i++) {
        index[0] = vertex[0];
        index[1] = vertex[3];
        index[2] = vertex[6];
        index += 3;
        vertex += 9;
    }
    face->next = NULL;
    mgFACE_GROUP *group = face_group;
    if (group == NULL) {
        group = (mgFACE_GROUP *)memory->Alloc(0x12);
        group->next = NULL;
        group->face = NULL;
        group->material = face->material;
        group->vu_program = 1; // MG_VU_PROG_SHADOW
        face_group = group;
    }
    mgCFace *last = group->face;
    if (last == NULL) {
        group->face = face;
    } else {
        mgCFace *following;
        while ((following = last->next) != NULL) {
            last = following;
        }
        last->next = face;
    }
    if (face_out != NULL) {
        *face_out = face;
    }
    return (FACES_ID *)vertex;
}
#pragma global_optimizer reset
#pragma schedule reset

// Emit each face reference before advancing the packet.
#pragma schedule off
#pragma global_optimizer off
u_int mgCShadowMDT::CreatePacket(mgCDrawManager *draw_manager) {
    GetTextureManager();

    mgFACE_GROUP *group = face_group;
    mgCMemory *packet_memory = draw_manager->packet_memory;
    mgCMemory *data_memory = draw_manager->data_memory;
    u_long128 *packet_start = &packet_memory->stack[packet_memory->stack_used];
    u_long128 *data_start = &data_memory->stack[data_memory->stack_used];
    u_long128 *data = data_start;
    u_int *packet = (u_int *)packet_start;
    u_int *tag;
    u_int *end_tag;

    for (; group != NULL; group = group->next) {
        group->packet = (u_long128 *)packet;
        mgCFace *face = group->face;
        for (; face != NULL;) {
            // DMA REF to the face's packet, built through the uncached mirror of the data memory.
            tag = packet;
            packet += 4;
            tag[0] = 0x30000000;
            tag[1] = (u_int)data;
            tag[2] = 0;
            tag[3] = 0;
            int size = CreateFacePacket((u_int *)((u_int)data | 0x20000000), face);
            data += size;
            face = face->next;
            tag[0] |= size;
        }
        packet += mgSetPkTexFlush_TagCnt(packet) * 4;

        end_tag = packet;
        packet += 4;
        // DMA RET.
        end_tag[0] = 0x60000000;
        end_tag[1] = 0;
        end_tag[2] = 0;
        end_tag[3] = 0;
        group->packet_size = (u_long128 *)packet - group->packet;
    }

    packet_memory->Alloc((u_long128 *)packet - packet_start);
    data_memory->Alloc(data - data_start);
    return (u_int)packet_start & 0x0FFFFFFF;
}
#pragma global_optimizer reset
#pragma schedule reset

// Copy the model data before walking its face sections.
#pragma schedule off
#pragma global_optimizer off
int mgCShadowMDT::DataAssignMDT(MDT_HEADER *header, mgCMemory *memory, mgCTextureManager *texture_manager) {
    if (header == NULL) {
        return 0;
    }

    this->texture_manager = texture_manager;
    // A shadow needs neither normals nor texture coordinates.
    header->uv_num = 0;
    header->normal_num = 0;
    CopyMDTData(header, memory);
    face_group = NULL;

    MDT_FACES *section = (MDT_FACES *)((char *)header + header->faces_ofs);
    FACES_ID *faces = (FACES_ID *)(section + 1);
    int prim_num = section->prim_num;
    for (int i = 0; i < prim_num; i++) {
        faces = CreateFace(faces, memory, memory, NULL);
    }
    return 1;
}
#pragma global_optimizer reset
#pragma schedule reset

#ifdef NONMATCHING
int mgCShadowMDT::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    u_int zero[4] = {0, 0, 0, 0};
    sceVu0FMATRIX world_screen;
    mgMulMatrix(world_screen, info->world_screen, matrix);

    u_int *start = GetScrPad();
    u_int *write = start;
    info->GetpLightInfo();

    // DMA CNT, its quadword count filled in below; VIF NOP, BASE, OFFSET and UNPACK.
    write[0] = 0x10000000;
    write[1] = 0;
    write[2] = 0;
    write[3] = 0;
    write[4] = 0;
    write[5] = vu1_base | 0x03000000;
    write[6] = vu1_offset | 0x02000000;

    u_long128 *vu = (u_long128 *)write;
    vu[2] = *(u_long128 *)zero;
    vu[3] = *(u_long128 *)zero;
    vu[4] = *(u_long128 *)zero;
    write[20] = info->unk_fb0[3];
    write[21] = info->unk_fb0[0];
    write[22] = info->unk_fb0[1];
    write[23] = info->unk_fb0[2];
    sceVu0CopyMatrix((sceVu0FVECTOR *)&vu[6], world_screen);
    sceVu0CopyMatrix((sceVu0FVECTOR *)&vu[10], matrix);
    vu[14] = *(u_long128 *)zero;
    vu[15] = *(u_long128 *)zero;
    vu[16] = *(u_long128 *)zero;
    ((float *)write)[56] = info->shadow_light_dir[0];
    ((float *)write)[60] = info->shadow_light_dir[1];
    ((float *)write)[64] = info->shadow_light_dir[2];
    vu[23] = *(u_long128 *)info->full_max;
    vu[24] = *(u_long128 *)info->full_min;

    sceVu0FMATRIX view_clip;
    mgMulMatrix(view_clip, info->view_clip_full, info->view);
    mgMulMatrix(view_clip, view_clip, matrix);
    vu[27] = *(u_long128 *)view_clip[0];
    vu[28] = *(u_long128 *)view_clip[1];
    vu[29] = *(u_long128 *)view_clip[2];
    vu[30] = *(u_long128 *)view_clip[3];
    vu[31] = *(u_long128 *)info->clip_screen_full[0];
    vu[32] = *(u_long128 *)info->clip_screen_full[1];
    vu[33] = *(u_long128 *)info->clip_screen_full[2];
    vu[34] = *(u_long128 *)info->clip_screen_full[3];
    write[7] = ((((u_int)((u_int *)&vu[35] - &write[4]) / 4) - 1) << 16) | 0x6C000000;

    // VIF MSCAL of the shadow microprogram.
    write[140] = 0;
    write[141] = 0;
    write[142] = 0;
    write[143] = 0x14000000;
    write[0] |= ((u_int *)&vu[36] - &write[4]) / 4;

    // DMA CNT and VIF DIRECT of the GS packet: PRMODECONT, PRMODE, RGBAQ, then the draw environment.
    write = (u_int *)&vu[36];
    write[0] = 0x10000008;
    write[1] = 0;
    write[2] = 0;
    write[3] = 0x50000008;
    write[4] = 0x8003;
    write[5] = 0x10000000;
    write[6] = SCE_GIF_PACKED_AD;
    write[7] = 0;
    u_long *ad = (u_long *)&write[8];
    ad[0] = 0;
    ad[1] = SCE_GS_PRMODECONT;
    ad[2] = 0x40;
    ad[3] = SCE_GS_PRMODE;
    ad[4] = SCE_GS_SET_RGBAQ(1, 1, 1, 0x80, 0);
    ad[5] = SCE_GS_RGBAQ;

    mgCDrawEnv *env = (mgCDrawEnv *)&ad[6];
    if (draw_env != NULL) {
        *env = *draw_env;
    } else {
        *env = info->draw_env[0];
    }
    env->SetZBuf(MG_ZBUF_NO_WRITE);
    env->test.bits.zte = 1;
    env->test.bits.ztst = SCE_GS_ZGEQUAL;
    env->test.bits.ate = 0;
    env->test.bits.afail = 0;
    env->test.bits.date = 0;
    write = (u_int *)(env + 1);

    sceVu0FMATRIX shadow;
    mgMulMatrix(shadow, info->shadow, matrix);
    mgMulMatrix(shadow, info->view, shadow);
    mgMulMatrix(shadow, info->view_clip_full, shadow);
    write += SetShadowData(write, shadow) * 4;

    // DMA RET.
    write[0] = 0x60000000;
    write[1] = 0;
    write[2] = 0;
    write[3] = 0;

    int size = (write + 4 - start) / 4;
    SendDMA(packet, size);
    return size;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_shadow", CreateRenderInfoPacket__12mgCShadowMDTFPUiPA4_fP13mgRENDER_INFO);
#endif

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_shadow", prog_vif_208__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_shadow", at_243__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_shadow", __vt__12mgCShadowMDT__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(at_353, 0x10);
