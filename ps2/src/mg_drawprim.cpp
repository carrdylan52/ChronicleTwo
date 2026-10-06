#include "common.h"
#include "mg_drawprim.hpp"

#include "mg_memory.hpp"
#include "mglib.hpp"


// Code (.text)
mgCDrawPrim::mgCDrawPrim() {
    memory = NULL;
    vif_packet = NULL;
    draw_manager = NULL;
    disabled = 1;
    q = 1.0f;
    *(u_long *)&prim = MG_GS_PRIM_FST;
    bilinear = 1;
    z_mask = MG_Z_MASK_WRITE;
    coord = 0;
    offset_x = 0;
    offset_y = 0;
}

void mgCDrawPrim::Initialize(mgCMemory *memory, sceVif1Packet *vif_packet) {
    if (vif_packet == NULL) {
        vif_packet = mgVif1Packet;
    }
    if (memory == NULL) {
        memory = mgGetDataBuffer();
    }
    this->memory = memory;
    this->vif_packet = vif_packet;
    detached = 0;
    if (draw_manager == NULL) {
        draw_manager = &mgDrawManager;
    }
    disabled = 1;
    texture.Initialize();
    draw_env.Initialize(0);
}

void mgCDrawPrim::Begin(int type) {
    disabled = 1;
    if (memory == NULL || vif_packet == NULL || draw_manager == NULL) {
        return;
    }
    if (draw_manager->render_info == NULL) {
        return;
    }
    disabled = 0;
    prim.PRIM = type;
    q = 1.0f;
    Begin2();
    BeginDma();
}

void mgCDrawPrim::BeginDma() {
    u_int *tag;
    u_long *data;
    dma_start = write;
    direct_start = dma_start;
    tag = (u_int *)write;
    tag[0] = 0;
    tag[1] = 0;
    tag[2] = 0;
    tag[3] = 0;
    dma_tag = &tag[0];
    direct_code = &tag[3];
    write++;

    u_int *gif = (u_int *)write;
    giftag = gif;
    gif[0] = MG_GIFTAG_EOP;
    gif[1] = 1 << MG_GIFTAG_NREG_SHIFT;
    gif[2] = SCE_GIF_PACKED_AD;
    gif[3] = 0;
    write++;

    data = (u_long *)write;
    data[0] = *(u_long *)&prim;
    data[1] = SCE_GS_PRIM;
    write = (u_long128 *)(data + 2);
}

void mgCDrawPrim::EndDma() {
    int dma_qwc = (write - dma_start) - 1;
    int direct_qwc = (write - direct_start) - 1;
    int nloop = (write - (u_long128 *)giftag) - 1;

    if (dma_qwc < 1 || direct_qwc < 1) {
        // An empty run is dropped.
        write = dma_start;
    } else {
        *dma_tag = dma_qwc | MG_DMA_CNT;
        *direct_code = direct_qwc | MG_VIF_DIRECT;
        *giftag = nloop | MG_GIFTAG_EOP;
    }
}

void mgCDrawPrim::Flush() {
    EndDma();
    BeginDma();
}

void mgCDrawPrim::End() {
    if (disabled == 0) {
        EndDma();
        End2();
    }
}

void mgCDrawPrim::Begin2() {
    mgRENDER_INFO *render_info;
    u_int *tag;
    u_long *data;

    disabled = 1;
    if (memory == NULL || vif_packet == NULL || draw_manager == NULL) {
        return;
    }
    render_info = draw_manager->render_info;
    if (render_info == NULL) {
        return;
    }
    disabled = 0;
    packet_start = memory->stAllocTest(1);
    if (detached == 0) {
        sceVif1PkCall(vif_packet, packet_start, 0);
    }
    packet_start = (u_long128 *)((u_int)packet_start | MG_UNCACHED);
    if (packet_top == NULL) {
        packet_top = packet_start;
    }
    write = packet_start;
    sceGsZbuf zbuf __attribute__((aligned(4))) = sceGsZbuf(render_info->draw_env[0].zbuf);
    draw_env.zbuf = zbuf;
    draw_env.SetZBuf(z_mask);

    // DMA tag and VIF code for the seven quadwords of drawing state below.
    tag = (u_int *)write;
    tag[0] = MG_DMA_CNT | 7;
    tag[2] = 0;
    tag[1] = 0;
    tag[3] = MG_VIF_DIRECT | 7;
    write++;

    u_int *gif = (u_int *)write;
    giftag = gif;
    gif[0] = MG_GIFTAG_EOP | 2;
    gif[1] = 1 << MG_GIFTAG_NREG_SHIFT;
    gif[2] = SCE_GIF_PACKED_AD;
    gif[3] = 0;
    write++;

    data = (u_long *)write;
    data[0] = 0;
    data[1] = SCE_GS_TEXFLUSH;
    data[2] = 1;
    data[3] = MG_GS_PRMODECONT;
    write = (u_long128 *)(data + 4);

    *(mgCDrawEnv *)write = draw_env;
    write += sizeof(mgCDrawEnv) / sizeof(u_long128);
}

void mgCDrawPrim::BeginPrim2(int type) {
    packed = 0;
    prim.PRIM = type;
    q = 1.0f;
    BeginDma();
}


void mgCDrawPrim::BeginPrim2(int type, unsigned int regs_lo, unsigned int regs_hi, int nreg) {
    u_int *tag;

    packed = 1;
    prim.PRIM = type;
    q = 1.0f;
    dma_start = write;
    direct_start = dma_start;
    tag = (u_int *)write;
    tag[0] = 0;
    tag[1] = 0;
    tag[2] = 0;
    tag[3] = 0;
    dma_tag = &tag[0];
    direct_code = &tag[3];
    write++;

    u_int prim_bits = *(u_int *)&prim & 0x7FF;
    this->nreg = nreg;
    u_int *gif = (u_int *)write;
    giftag = gif;
    gif[0] = MG_GIFTAG_EOP;
    gif[1] = this->nreg << MG_GIFTAG_NREG_SHIFT | prim_bits << MG_GIFTAG_PRIM_SHIFT | MG_GIFTAG_PRE;
    gif[2] = regs_lo;
    gif[3] = regs_hi;
    write++;
}

// Packed primitive runs require a nonzero register count.
#pragma divbyzerocheck on
void mgCDrawPrim::EndPrim2() {
    if (packed == 0) {
        EndDma();
    } else {
        int dma_qwc = (write - dma_start) - 1;
        int direct_qwc = (write - direct_start) - 1;
        int data_qwc = (write - (u_long128 *)giftag) - 1;

        if (dma_qwc < 1 || direct_qwc < 1) {
            write = dma_start;
        } else {
            *dma_tag = dma_qwc | MG_DMA_CNT;
            *direct_code = direct_qwc | MG_VIF_DIRECT;
            *giftag = data_qwc / nreg | MG_GIFTAG_EOP;
        }
    }
}
#pragma divbyzerocheck reset

void mgCDrawPrim::End2() {
    if (disabled == 0) {
        if (detached == 0) {
            u_int *tag = (u_int *)write;
            write++;
            tag[0] = MG_DMA_RET;
            tag[1] = 0;
            tag[2] = 0;
            tag[3] = 0;
            sceVif1PkTerminate(vif_packet);
        }
        memory->Alloc(write - packet_start);
    }
}

void mgCDrawPrim::Data0(float *data) {
    int *dst = (int *)write;
    write++;
    asm {
        lqc2 vf1, 0(data)
        vftoi0.xyzw vf1, vf1
        sqc2 vf1, 0(dst)
    }
}

void mgCDrawPrim::Data4(float *data) {
    int *dst = (int *)write;
    write++;
    asm {
        lqc2 vf1, 0(data)
        vftoi4.xyzw vf1, vf1
        sqc2 vf1, 0(dst)
    }
}

void mgCDrawPrim::Data(int *data) {
    u_long128 value = *(u_long128 *)data;
    data = (int *)write;
    write = (u_long128 *)data + 1;
    *(u_long128 *)data = value;
}

u_char *mgCDrawPrim::DirectData(int count) {
    u_char *packet = (u_char *)write;
    write += count;
    return packet;
}

void mgCDrawPrim::Vertex(int x, int y, int z) {
    Vertex4(x << 4, y << 4, z);
}

void mgCDrawPrim::Vertex(float x, float y, float z) {
    sceVu0FVECTOR pos = {x, y, z, 0.0f};
    Vertex(pos);
}

void mgCDrawPrim::Vertex(float *pos) {
    int xyz[4];
    int *dst = xyz;
    asm {
        lqc2 vf10, 0(pos)
        vftoi4.xy vf10, vf10
        vftoi0.z vf10, vf10
        sqc2 vf10, 0(dst)
    }
    Vertex4(xyz[0], xyz[1], xyz[2]);
}
void mgCDrawPrim::Vertex4(int x, int y, int z) {
    int offset_x = 0;
    int offset_y = 0;

    GetOffset(&offset_x, &offset_y);
    u_long *data = (u_long *)write;
    data[0] = ((long long)z << 32) | ((long long)(x + offset_x) | ((long long)(y + offset_y) << 16));
    data[1] = SCE_GS_XYZ2;
    write++;
}

void mgCDrawPrim::Vertex4(int *pos) {
    Vertex4(pos[0], pos[1], pos[2]);
}

void mgCDrawPrim::Color(int r, int g, int b, int a) {
    u_long *data = (u_long *)write;
    u_int q_bits = *(u_int *)&q;
    data[0] = ((u_long)q_bits << 32) | ((long long)r | ((long long)g << 8) | ((long long)b << 16) | ((long long)a << 24));
    data[1] = SCE_GS_RGBAQ;
    write++;
}

void mgCDrawPrim::Color(float *color) {
    int rgba[4];
    int *dst = rgba;
    asm {
        lqc2 vf10, 0(color)
        vftoi0.xyzw vf10, vf10
        sqc2 vf10, 0(dst)
    }
    Color(rgba[0], rgba[1], rgba[2], rgba[3]);
}
void mgCDrawPrim::TextureCrd4(int u, int v) {
    u_long *data = (u_long *)write;
    data[0] = (long)u | (long)v << 16;
    data[1] = SCE_GS_UV;
    write++;
}

void mgCDrawPrim::TextureCrd(int u, int v) {
    TextureCrd4(u << 4, v << 4);
}

void mgCDrawPrim::Direct(unsigned long reg, unsigned long data) {
    u_long *dst = (u_long *)write;
    dst[0] = data;
    dst[1] = reg;
    write++;
}

#ifdef NONMATCHING
void mgCDrawPrim::Texture(mgCTexture *texture) {
    if (texture != NULL) {
        u_long *data;

        this->texture = *texture;
        this->texture.Bilinear(bilinear);
        data = (u_long *)write;
        data[0] = 0;
        data[1] = SCE_GS_TEXFLUSH;
        data[2] = *(u_long *)&this->texture.tex1;
        data[3] = SCE_GS_TEX1_1;
        data[4] = this->texture.tex0.value;
        data[5] = SCE_GS_TEX0_1;
        write += 3;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_drawprim", Texture__11mgCDrawPrimFP10mgCTexture);
#endif

void mgCDrawPrim::AlphaBlendEnable(int enable) {
    prim.ABE = enable;
}

void mgCDrawPrim::AlphaBlend(int mode) {
    draw_env.SetAlpha(mode);
}

void mgCDrawPrim::AlphaTestEnable(int enable) {
    draw_env.test.bits.ate = enable;
}

void mgCDrawPrim::AlphaTest(int method, int ref) {
    draw_env.test.bits.atst = method;
    draw_env.test.bits.aref = ref;
}

void mgCDrawPrim::DAlphaTest(int enable, int mode) {
    draw_env.test.bits.date = enable;
    draw_env.test.bits.datm = mode;
}

void mgCDrawPrim::DepthTestEnable(int enable) {
    sceGsTest *test = &draw_env.test;

    if (enable == 0) {
        test->bits.zte = 1;
        test->bits.ztst = SCE_GS_ALWAYS;
    } else {
        DepthTest(MG_DEPTH_TEST_GEQUAL);
    }
}

void mgCDrawPrim::DepthTest(int method) {
    sceGsTest *test = &draw_env.test;

    test->bits.zte = 1;
    switch (method) {
    case MG_DEPTH_TEST_ALWAYS:
        test->bits.ztst = SCE_GS_ALWAYS;
        break;
    case MG_DEPTH_TEST_GEQUAL:
        test->bits.ztst = SCE_GS_ZGEQUAL;
        break;
    case MG_DEPTH_TEST_GREATER:
        test->bits.ztst = MG_GS_ZGREATER;
        break;
    }
}

void mgCDrawPrim::ZMask(int mask) {
    z_mask = mask;
}

void mgCDrawPrim::TextureMapEnable(int enable) {
    prim.TME = enable;
}

void mgCDrawPrim::Bilinear(int enable) {
    bilinear = enable;
}

void mgCDrawPrim::Shading(int enable) {
    prim.IIP = enable;
}

void mgCDrawPrim::AntiAliasing(int enable) {
    prim.AA1 = enable;
}

void mgCDrawPrim::FogEnable(int enable) {
    prim.FGE = enable;
}

void mgCDrawPrim::Coord(int coord) {
    this->coord = coord;
}

void mgCDrawPrim::GetOffset(int *x, int *y) {
    *y = 0;
    *x = 0;
    if (coord == 0) {
        *x = mgScreenOffx << 4;
        *y = mgScreenOffy << 4;
    }
    *x += offset_x;
    *y += offset_y;
}

// Packet groups are collected before they are sent.
#pragma schedule off
mgCDrawManager::mgCDrawManager() {
    unk_68 = 0x40;
    unk_6c = 0;
    unk_70 = 0;
}
#pragma schedule reset

#pragma schedule off
void mgCDrawManager::SetSortTable(int num) {
    float near_dist = 1.0f;
    float far_dist = 2.0f;

    if (render_info != NULL) {
        near_dist = render_info->clip_min[2];
        far_dist = render_info->clip_max[2];
    }
    sort_num = num;
    sort_max = sort_num - 1;
    sort_num_f = (float)num;
    near_clip = near_dist;
    far_clip = far_dist;
    clip_range = far_dist - near_clip;
    sort_near = near_clip;
    sort_ratio = near_clip / far_clip;
    sort_scale = sort_num_f;
}
#pragma schedule reset

#pragma schedule off
#pragma opt_loop_invariants off
#pragma global_optimizer off
void mgCDrawManager::BeginDraw(mgCMemory *memory, int *order) {
    int table_size;
    int i;
    this->memory = memory;
    if (this->memory == NULL) {
        this->memory = packet_memory;
    }
    group_num = texture_manager->block_max;
    group_max = group_num;
    table_size = group_num / 4 + 1;
    order_index = NULL;
    draw_order = NULL;
    if (order != NULL) {
        int count;
        int *entry;

        order_index = (int *)this->memory->Alloc(group_max / 4 + 1);
        for (i = 0; i < group_max; i++) {
            order_index[i] = -1;
        }
        entry = order;
        count = 0;
        while (*entry >= 0) {
            count++;
            entry++;
        }
        draw_order = (int *)this->memory->Alloc((count + 1) / 4 + 1);
        for (i = 0; i < count; i++) {
            draw_order[i] = order[i];
            order_index[draw_order[i]] = i;
        }
        draw_order[i] = -1;
        group_num = count;
        table_size = (group_num + 1) / 4 + 1;
    }
    packet_list = (mgSORT_PACKET ***)this->memory->Alloc(table_size);
    unk_14 = (int *)this->memory->Alloc(table_size);
    packet_num = (int *)this->memory->Alloc(table_size);
    sort_table = (mgSORT_PACKET **)this->memory->Alloc(sort_num);
    ClearTable();
}
#pragma global_optimizer reset
#pragma opt_loop_invariants reset
#pragma schedule reset

#pragma optimization_level 1
void mgCDrawManager::ClearTable() {
    mgSORT_PACKET ***packets = packet_list;
    int *indices = unk_14;
    int *counts = packet_num;
    mgSORT_PACKET **sort = sort_table;

    for (int i = 0; i < group_num; i++) {
        *packets++ = NULL;
        *indices++ = 0;
        *counts++ = 0;
    }
    for (int j = 0; j < sort_num; j++) {
        *sort++ = NULL;
    }
}
#pragma optimization_level reset

#pragma schedule off
#pragma global_optimizer off
void mgCDrawManager::PreEndDraw() {
    int i;

    packet_cursor = (mgSORT_PACKET ***)memory->Alloc(group_num / 4 + 1);
    int *sizes = packet_num;
    for (int i = 0; i < group_num; i++) {
        int num = *sizes;
        sizes++;
        if (num > 0) {
            packet_list[i] = (mgSORT_PACKET **)memory->Alloc(num / 4 + 1);
            packet_cursor[i] = packet_list[i];
        }
    }
    // Only the first sort bucket is used.
    for (i = 0; i < 1; i++) {
        mgSORT_PACKET *entry = sort_table[i];
        if (entry != NULL) {
            for (; entry != NULL; entry = entry->next) {
                *packet_cursor[entry->group] = entry;
                packet_cursor[entry->group]++;
            }
        }
    }
}
#pragma global_optimizer reset
#pragma schedule reset

#pragma schedule off
int mgCDrawManager::ReloadTexture(int group, sceVif1Packet *vif_packet) {
    mgCTextureManager *manager = texture_manager;
    int index;

    if (group < 0 || group >= manager->block_max) {
        return 0;
    }
    index = group;
    if (draw_order != NULL) {
        index = order_index[group];
    }
    if (index < 0) {
        return 0;
    }
    sceVif1PkTerminate(vif_packet);
    manager->ReloadTexture(group, vif_packet);
    return 1;
}
#pragma schedule reset

#ifdef NONMATCHING
int mgCDrawManager::Draw(int group, sceVif1Packet *vif_packet) {
    u_int *start;
    u_int *tag;
    mgSORT_PACKET **entry;
    u_long128 *common;
    int i;

    if (group < 0 || group >= texture_manager->block_max) {
        return 0;
    }
    if (draw_order != NULL) {
        group = order_index[group];
    }
    if (group < 0) {
        return 0;
    }
    if (packet_list[group] == NULL) {
        return 1;
    }
    sceVif1PkTerminate(vif_packet);
    start = (u_int *)vif_packet->pCurrent;
    entry = &packet_list[group][packet_num[group] - 1];
    common = NULL;
    tag = start;
    // Packets are called in the reverse of their registration order.
    for (i = 0; i < packet_num[group]; i++) {
        if (*entry != NULL) {
            tag += mgSendVuProg(tag, (*entry)->vu_program);
            if (common != (*entry)->common) {
                tag[0] = MG_DMA_CALL;
                tag[1] = (u_int)(*entry)->common;
                tag[2] = 0;
                tag[3] = 0;
                tag += 4;
                common = (*entry)->common;
            }
            tag[0] = MG_DMA_CALL;
            tag[1] = (u_int)(*entry)->packet;
            tag[2] = 0;
            tag[3] = 0;
            tag += 4;
            entry--;
        }
    }
    sceVif1PkReserve(vif_packet, tag - start);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_drawprim", Draw__14mgCDrawManagerFiP13sceVif1Packet);
#endif

#pragma schedule off
#pragma global_optimizer off
void mgCDrawManager::EndDraw(sceVif1Packet *vif_packet) {
    int i;
    int group;

    PreEndDraw();
    for (i = 0; i < group_num; i++) {
        // Without a draw order the group is left as it was.
        if (draw_order != NULL) {
            group = draw_order[i];
        }
        ReloadTexture(group, vif_packet);
        Draw(group, vif_packet);
    }
}
#pragma global_optimizer reset
#pragma schedule reset

#pragma schedule off
void mgCDrawManager::AddPacket(int group, u_long128 *common, u_long128 *packet, int vu_program) {
    int index;
    mgSORT_PACKET *entry;

    if (group >= group_max) {
        return;
    }
    index = group;
    if (order_index != NULL) {
        if (group < 0) {
            index = order_index[*draw_order];
        } else {
            index = order_index[group];
        }
        if (index < 0) {
            return;
        }
    }
    entry = (mgSORT_PACKET *)memory->Alloc(1);
    entry->next = *sort_table;
    *sort_table = entry;
    entry->common = common;
    entry->packet = packet;
    entry->group = index;
    entry->vu_program = vu_program;
    packet_num[index]++;
}
#pragma schedule reset

// Uninitialised data (.bss)
INCLUDE_BSS(at_369, 0x10);
