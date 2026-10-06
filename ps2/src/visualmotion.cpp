#include "common.h"
#include "visualmotion.hpp"

#include <cstring>

#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"

// Code (.text)
void mgCVisualMotionMDT::Initialize() {
    int i;

    mgCVisualMDT::Initialize();
    for (i = 0; i < 32; i++) {
        bone[i] = -1;
    }
    vu1_base = 0x7C;
    vu1_offset = 0x94;
    weight_num = 0;
    weight = NULL;
    base_matrix = NULL;
    frame_id = 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", CreateVertexWeight__18mgCVisualMotionMDTFPUiiP9mgCMemory);

mgVertexWeight::mgVertexWeight() {
    memset(this, 0, sizeof(mgVertexWeight));
}

void mgCVisualMotionMDT::ChangeWeight(mgCFrame **frame, float (*base_matrix)[4][4], int frame_id) {
    int i;

    for (i = 0; i < 32; i++) {
        if (bone[i] < 0) {
            break;
        }
        bone[i] = frame[0]->SearchFrameID(this->frame[bone[i]]->name);
    }
    this->frame = frame;
    this->base_matrix = base_matrix;
    this->frame_id = frame_id;
}

int mgCVisualMotionMDT::DataAssignMotionMDT(MDT_HEADER *header, mgCVMotionData *motion, mgCMemory *memory, mgCMemory *work_memory, mgCTextureManager *texture_manager) {
    MDT_FACES *section;
    FACES_ID  *faces;
    mgCFace   *face;
    int        i;
    u_int     *packet;
    int        bone_num;
    int        count;
    int        size;

    if (work_memory == NULL) {
        return 0;
    }
    work_memory->stack_used = 0;
    work_memory->lock = 0;
    if (texture_manager == NULL) {
        texture_manager = &mgTexManager;
    }
    this->texture_manager = texture_manager;
    CopyMDTDataPointer(header, memory);
    frame = motion->frame;
    base_matrix = motion->base_matrix;
    frame_id = motion->frame_id;
    CreateVertexWeight(motion->weight_data, motion->frame_id, work_memory);
    for (bone_num = 0; bone_num < 32; bone_num++) {
        if (bone[bone_num] < 0) {
            break;
        }
    }
    vu1_base = bone_num * 4 + 0x3C;
    vu1_offset = 0xB4 - bone_num * 4 / 2;
    mgCMemory index_memory;
    size = work_memory->stack_size - work_memory->stack_used;
    index_memory.stSetBuffer(&work_memory->stack[work_memory->stack_used], size);
    face_group = NULL;
    section = (MDT_FACES *)((u_char *)header + header->faces_ofs);
    count = section->prim_num;
    faces = (FACES_ID *)(section + 1);
    for (i = 0; i < count; i++) {
        index_memory.stack_used = 0;
        index_memory.lock = 0;
        faces = CreateFace(faces, memory, &index_memory, &face);
        packet = (u_int *)&memory->stack[memory->stack_used];
        size = CreateFaceMotionPacket(packet, face, motion);
        ((u_int *)&face->packet_tag)[0] = size | 0x30000000;
        ((u_int *)&face->packet_tag)[1] = (u_int)packet;
        ((u_int *)&face->packet_tag)[2] = 0;
        ((u_int *)&face->packet_tag)[3] = 0;
        memory->Alloc(size);
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData0__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData1__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData2__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData3__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData4__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData5__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData6__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", SetData7__FiiPPiP1P1P1P1P1P14mgVertexWeight);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", CreateFaceMotionPacket__18mgCVisualMotionMDTFPUiP7mgCFaceP14mgCVMotionData);

int mgCVisualMotionMDT::CreateRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    int size;

    info->motion = 1;
    size = mgCVisualMDT::CreateRenderInfoPacket(packet, matrix, info);
    info->motion = 0;
    return size;
}

int mgCVisualMotionMDT::CreateExtRenderInfoPacket(u_int *packet, float (*matrix)[4], mgRENDER_INFO *info) {
    sceVu0FMATRIX inverse_matrix;
    sceVu0FMATRIX inverse_root;
    sceVu0FMATRIX root_local;
    sceVu0FMATRIX root_matrix;
    sceVu0FMATRIX bone_matrix;
    sceVu0FMATRIX bone_inverse;
    sceVu0FMATRIX frame_matrix;
    u_int        *write;
    int           bone_num;
    int           i;
    int           slot;

    if (frame == NULL) {
        return 0;
    }
    write = packet;
    if (base_matrix == NULL) {
        return 0;
    }
    for (bone_num = 0; bone_num < 32; bone_num++) {
        if (bone[bone_num] < 0) {
            break;
        }
    }
    if (bone_num == 0) {
        return 0;
    }
    mgInversMatrix(inverse_matrix, matrix);
    packet[0] = 0x10000000 | (bone_num << 2);
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = (bone_num << 18) | 0x6C00003C;
    write += 4;
    frame[0]->GetLWMatrix(root_matrix);
    mgMulMatrix(inverse_root, root_matrix, base_matrix[frame_id]);
    mgInversMatrix(root_local, inverse_root);
    for (i = 0; i < bone_num; i++) {
        slot = bone[i];
        if (frame[slot] != NULL) {
            frame[slot]->GetLWMatrix(frame_matrix);
            mgMulMatrix(bone_matrix, root_matrix, base_matrix[slot]);
            mgMulMatrix(bone_matrix, root_local, bone_matrix);
            mgInversMatrix(bone_inverse, bone_matrix);
            mgMulMatrix(frame_matrix, inverse_matrix, frame_matrix);
            mgMulMatrix((float (*)[4])write, frame_matrix, bone_inverse);
        }
        write += 16;
    }
    return (write - packet) / 4;
}

void mgCVisualMotionMDT::SetBaseBox(float *max, float *min) {
    *(u_long128 *)base_box.max = *(u_long128 *)max;
    max[3] = 1.0f;
    *(u_long128 *)base_box.min = *(u_long128 *)min;
    min[3] = 1.0f;
}

int mgCVisualMotionMDT::CreateBBox(float *max, float *min, float (*matrix)[4]) {
    sceVu0FVECTOR bound_max;
    sceVu0FVECTOR bound_min;
    sceVu0FVECTOR bone_max;
    sceVu0FVECTOR bone_min;
    sceVu0FVECTOR center;
    sceVu0FVECTOR corner[8];
    sceVu0FVECTOR moved[8];
    sceVu0FVECTOR origin[8];
    float        *box[4];
    sceVu0FMATRIX inverse_matrix;
    sceVu0FMATRIX inverse_root;
    sceVu0FMATRIX root_local;
    sceVu0FMATRIX root_matrix;
    sceVu0FMATRIX bone_matrix;
    sceVu0FMATRIX bone_inverse;
    sceVu0FMATRIX frame_matrix;
    int           bone_num;
    int           i;
    int           slot;
    int           j;

    for (bone_num = 0; bone_num < 32; bone_num++) {
        if (bone[bone_num] < 0) {
            break;
        }
    }
    if (bone_num == 0) {
        return 0;
    }
    sceVu0AddVector(center, base_box.max, base_box.min);
    sceVu0ScaleVector(center, center, 0.5f);
    box[0] = base_box.max;
    box[1] = base_box.min;
    for (j = 0; j < 8; j++) {
        corner[j][3] = 1.0f;
        corner[j][0] = box[(j & 1) != 0][0];
        corner[j][1] = box[(j & 2) != 0][1];
        corner[j][2] = box[(j & 4) != 0][2];
        mgZeroVector(origin[j]);
    }
    mgInversMatrix(inverse_matrix, matrix);
    frame[0]->GetLWMatrix(root_matrix);
    mgMulMatrix(inverse_root, root_matrix, base_matrix[frame_id]);
    mgInversMatrix(root_local, inverse_root);
    for (i = 0; i < bone_num; i++) {
        slot = bone[i];
        if (frame[slot] != NULL) {
            frame[slot]->GetLWMatrix(frame_matrix);
            mgMulMatrix(bone_matrix, root_matrix, base_matrix[slot]);
            mgMulMatrix(bone_matrix, root_local, bone_matrix);
            mgInversMatrix(bone_inverse, bone_matrix);
            mgMulMatrix(frame_matrix, inverse_matrix, frame_matrix);
            mgMulMatrix(frame_matrix, frame_matrix, bone_inverse);
            mgApplyMatrixN(moved, frame_matrix, corner, 8);
            if (i == 0) {
                mgVectorMinMaxN(bound_max, bound_min, moved, 8);
            } else {
                mgVectorMinMaxN(bone_max, bone_min, moved, 8);
                mgVectorMaxMin(bound_max, bound_min, bound_max, bound_min, bone_max, bone_min);
            }
        }
    }
    *(u_long128 *)max = *(u_long128 *)bound_max;
    *(u_long128 *)min = *(u_long128 *)bound_min;
    max[3] = 1.0f;
    min[3] = 1.0f;
    return 1;
}

#ifdef NONMATCHING
mgCVisual *mgCVisualMotionMDT::Copy(mgCMemory *memory) {
    mgCVisualMotionMDT *copy;
    u_int               bytes;
    u_int               quadwords;
    int                 i;

    copy = new (memory->Alloc(0x13)) mgCVisualMotionMDT;
    if (copy == NULL) {
        return NULL;
    }
    *copy = *this;
    if (material_num > 0) {
        bytes = material_num * sizeof(mgMaterial);
        quadwords = (bytes & 0xF) ? (bytes >> 4) + 1 : bytes >> 4;
        copy->material = new (memory->Alloc(quadwords + 2)) mgMaterial[material_num];
    }
    for (i = 0; i < material_num; i++) {
        copy->material[i] = material[i];
    }
    return copy;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/visualmotion", Copy__18mgCVisualMotionMDTFP9mgCMemory);
#endif

int mgCVisualMotionMDT::Iam() {
    return MG_VISUAL_KIND_MOTION_MDT;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", set_data_func__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", prog_vif_532__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", progf_vif_533__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", at_571__3__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", at_357__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", at_358__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/visualmotion", __vt__18mgCVisualMotionMDT__DATA);
