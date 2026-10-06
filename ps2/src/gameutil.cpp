#include "common.h"
#include "gameutil.hpp"

#include <libvu0.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "dng_main.hpp"
#include "font.hpp"
#include "intersection.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_visual.hpp"
#include "mglib.hpp"

// Trap on division by zero for variable integer divisors.
#pragma divbyzerocheck on

/**
 *
 * Skinned frame whose bone matrices MotionProc2 and MotionProc3 last set up.
 *
 */
static mgCFrame *OldSkinFrame;

/**
 *
 * Skinned-vertex accumulator of the frame being skinned; w sums the weights.
 *
 */
static sceVu0FVECTOR def_vrtx[800];

/**
 *
 * Skinned-normal accumulator of the frame being skinned.
 *
 */
static sceVu0FVECTOR def_nml[1];

// Code (.text)
/**
 *
 * Interpolates between two quaternions, stored w first, along the shorter arc.
 *
 */
static void QuatSlerp(float *from, float *to, float t, float *out) {
    float cosine;
    float scale_from;
    float scale_to;
    float angle;
    float inv_sine;

    cosine = (from[3] * to[3]) + ((from[1] * to[1]) + (from[2] * to[2])) + (from[0] * to[0]);
    scale_to = t;
    if (cosine < 0.0f) {
        to[0] = -to[0];
        cosine = -cosine;
        to[1] = -to[1];
        to[2] = -to[2];
        to[3] = -to[3];
    }
    if (cosine < 0.01f) {
        out[1] = to[1];
        out[2] = to[2];
        out[3] = to[3];
        out[0] = to[0];
    } else {
        scale_from = 1.0f - t;
        scale_to = t;

        if (1.0f - cosine > 0.01f) {
            angle = acosf(cosine);
            inv_sine = 1.0f / sinf(angle);
            scale_from = inv_sine * sinf((1.0f - scale_to) * angle);
            scale_to = inv_sine * sinf(scale_to * angle);
        } else {
            scale_from = 1.0f - scale_to;
        }
        out[1] = (scale_from * from[1]) + (scale_to * to[1]);
        out[2] = (scale_from * from[2]) + (scale_to * to[2]);
        out[3] = (scale_from * from[3]) + (scale_to * to[3]);
        out[0] = (scale_from * from[0]) + (scale_to * to[0]);
    }
}

#ifdef NONMATCHING
Mot_List *MotionProc(mgCFrame *root, float time, Mot_List *list, mgCCamera *camera) {
    unsigned int  frame_no = (unsigned int) time;
    int           low = 0;
    int           high = list->key_count;
    int           middle;
    unsigned int  key;
    unsigned int  next;
    unsigned int  key_frame;
    float         t;
    mgCFrame     *frame;
    sceVu0FVECTOR value;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;

    while (low < high) {
        middle = (low + high) >> 1;

        if (list->key_frames[middle] <= frame_no) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }

    key = low - 1;
    next = low;

    if (next > list->key_count - 1) {
        next = key;
    }

    key_frame = list->key_frames[key];
    t = (time - (float) key_frame) / (float) (list->key_frames[next] - key_frame);
    frame = root->GetFrame(list->frame);

    switch (list->type) {
        case MOTION_KEY_ROTATION:
            sceVu0CopyVector(from, list->values[key]);
            sceVu0CopyVector(to, list->values[next]);

            if (t > 0.001f && t < 0.999f) {
                QuatSlerp(list->values[key], list->values[next], t, rotation);
                frame->SetTransMatrix(rotation);
            } else {
                if (t <= 0.001f) {
                    frame->SetTransMatrix(from);
                }

                if (t >= 0.999f) {
                    frame->SetTransMatrix(to);
                }
            }

            break;
        case MOTION_KEY_SCALE:
            if (t > 0.001f && t < 0.999f) {
                sceVu0InterVectorXYZ(value, list->values[next], list->values[key], t);
            } else {
                if (t <= 0.001f) {
                    sceVu0CopyVectorXYZ(value, list->values[key]);
                }

                if (t >= 0.999f) {
                    sceVu0CopyVectorXYZ(value, list->values[next]);
                }
            }

            frame->SetScale(value[0], value[1], value[2]);
            break;
        case MOTION_KEY_TRANSLATION:
            if (t > 0.001f && t < 0.999f) {
                sceVu0InterVectorXYZ(value, list->values[next], list->values[key], t);
            } else {
                if (t <= 0.001f) {
                    sceVu0CopyVectorXYZ(value, list->values[key]);
                }

                if (t >= 0.999f) {
                    sceVu0CopyVectorXYZ(value, list->values[next]);
                }
            }

            frame->trans_matrix[3][0] = value[0];
            frame->trans_matrix[3][1] = value[1];
            frame->trans_matrix[3][2] = value[2];
            frame->changed = 1;
            break;
        case MOTION_KEY_VERTEX: {
            sceVu0FVECTOR *vertices = ((mgCVisualMDT *) frame->visual)->vertex;
            int            driven = list->frame;

            if (t > 0.001f && t < 0.999f) {
                while (driven == list->frame) {
                    sceVu0InterVectorXYZ(value, list->values[next], list->values[key], t);
                    sceVu0CopyVectorXYZ(vertices[list->target - 1], value);
                    list = list->next;

                    if (list == NULL) {
                        return NULL;
                    }
                }

                return list;
            }

            if (t <= 0.001f) {
                while (driven == list->frame) {
                    sceVu0CopyVectorXYZ(vertices[list->target - 1], list->values[key]);
                    list = list->next;

                    if (list == NULL) {
                        return NULL;
                    }
                }
            }

            if (t < 0.999f) {
                return list;
            }

            while (driven == list->frame) {
                sceVu0CopyVectorXYZ(vertices[list->target - 1], list->values[next]);
                list = list->next;

                if (list == NULL) {
                    return NULL;
                }
            }

            return list;
        }
        case MOTION_KEY_CAMERA_POSITION:
            if (camera != NULL) {
                sceVu0InterVectorXYZ(value, list->values[next], list->values[key], t);
                root->GetWorldPosition(value, value);
                camera->SetPos(value[0], value[1], value[2]);
            }

            break;
        case MOTION_KEY_CAMERA_TARGET:
            // The look-at position is never interpolated from the keys here.
            if (camera != NULL) {
                root->GetWorldPosition(value, value);
                camera->SetRef(value[0], value[1], value[2]);
            }

            break;
        case MOTION_KEY_MATERIAL_ALPHA: {
            float       one_minus_t = 1.0f - t;
            mgMaterial *materials = frame->visual->GetpMaterial();

            materials[list->target].diffuse[3] = 1.0f - (t * list->values[next][0] + one_minus_t * list->values[key][0]);
            break;
        }
        case MOTION_KEY_MATERIAL_COLOR: {
            mgMaterial *materials = frame->visual->GetpMaterial();

            sceVu0InterVectorXYZ(materials[list->target].diffuse, list->values[next], list->values[key], t);
            frame->attr->unk_28 = 2;
            break;
        }
        case MOTION_KEY_CAMERA_ROLL:
            if (camera != NULL) {
                camera->SetRoll(-((t * list->values[next][0] + (1.0f - t) * list->values[key][0]) / 180.0f * 3.1415927f));
            }

            break;
        case MOTION_KEY_CAMERA_FOV:
            if (camera != NULL) {
                mgSetProjection(1.0f / tanf((t * list->values[next][0] + (1.0f - t) * list->values[key][0]) * 0.5f / 180.0f * 3.1415927f) * 480.0f * 0.5f);
            }

            break;
        case MOTION_KEY_VISIBLE:
            if (list->values[key][0] < 1.0f) {
                frame->attr->draw = 0;
            } else {
                frame->attr->draw = MG_FRAME_DRAW_VISIBLE | MG_FRAME_DRAW_SKIP_CHILDREN;
            }

            break;
        case MOTION_KEY_UNK_33:
            if (list->values[key][0] < 1.0f) {
                frame->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            } else {
                frame->attr->draw = MG_FRAME_DRAW_VISIBLE;
            }

            break;
    }

    return list->next;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", MotionProc__FP8mgCFramefP8Mot_ListP9mgCCamera);
#endif

Mot_List *MotionProc(mgCFrame *root, unsigned int from_frame, unsigned int to_frame, float blend, Mot_List *list, mgCCamera *camera) {
    int high;
    int low;
    int key;
    int next;
    mgCFrame *frame;
    float one_minus_blend;
    sceVu0FVECTOR value;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    {
        int high = list->key_count;
        int low = 0;
        int middle;
        while (low < high) {
            key = (middle = (low + high) >> 1);
            if (list->key_frames[key] <= from_frame) {
                low = key + 1;
            } else {
                high = key;
            }
        }
        key = low - 1;
    }
    {
        high = list->key_count;
        low = 0;
        while (low < high) {
            next = (low + high) >> 1;
            if (list->key_frames[next] <= to_frame) {
                low = next + 1;
            } else {
                high = next;
            }
        }
    }

    next = low - 1;
    frame = root->GetFrame(list->frame);

    switch (list->type) {
        case MOTION_KEY_ROTATION:
            sceVu0CopyVector(from, list->values[key]);
            sceVu0CopyVector(to, list->values[next]);

            if (blend > 0.0001f && blend < 0.9999f) {
                QuatSlerp(from, to, blend, rotation);
                frame->SetTransMatrix(rotation);
            } else {
                if (blend <= 0.0001f) {
                    frame->SetTransMatrix(from);
                }

                if (blend >= 0.9999f) {
                    frame->SetTransMatrix(to);
                }
            }

            break;
        case MOTION_KEY_SCALE:
            sceVu0InterVectorXYZ(value, list->values[next], list->values[key], blend);
            frame->SetScale(value[0], value[1], value[2]);
            break;
        case MOTION_KEY_TRANSLATION:
            sceVu0InterVectorXYZ(value, list->values[next], list->values[key], blend);
            frame->trans_matrix[3][0] = value[0];
            frame->trans_matrix[3][1] = value[1];
            frame->trans_matrix[3][2] = value[2];
            frame->changed = 1;
            break;
        case MOTION_KEY_VERTEX: {
            int vertex;
            int driven = list->frame;
            sceVu0FVECTOR *vertices = ((mgCVisualMDT *) frame->visual)->vertex;
            do {
                if (!(blend <= 0.0001f) && blend < 0.9999f) {
                    Mot_List *node = list;
                    while (driven == node->frame) {
                        vertex = list->target - 1;
                        sceVu0InterVectorXYZ(value, list->values[next], list->values[key], blend);
                        sceVu0CopyVectorXYZ(vertices[vertex], value);
                        list = list->next;
                        do {
                            if (list == NULL) {
                                break;
                            }
                            node = list;
                            goto blend_nonnull0;
                        } while (0);
                        return NULL;
                    blend_nonnull0:;
                    }
                    break;
                }
                if (blend <= 0.0001f) {
                    Mot_List *node = list;
                    while (driven == node->frame) {
                        sceVu0CopyVectorXYZ(vertices[list->target - 1], list->values[key]);
                        list = list->next;
                        do {
                            if (list == NULL) {
                                break;
                            }
                            node = list;
                            goto blend_nonnull1;
                        } while (0);
                        return NULL;
                    blend_nonnull1:;
                    }
                }
                if (!(blend < 0.9999f)) {
                    Mot_List *node = list;
                    while (driven == node->frame) {
                        sceVu0CopyVectorXYZ(vertices[list->target - 1], list->values[next]);
                        list = list->next;
                        do {
                            if (list == NULL) {
                                break;
                            }
                            node = list;
                            goto blend_nonnull2;
                        } while (0);
                        return NULL;
                    blend_nonnull2:;
                    }
                }
            } while (0);
            return list;
        }
        case MOTION_KEY_CAMERA_POSITION:
            if (camera != NULL) {
                sceVu0InterVectorXYZ(value, list->values[next], list->values[key], blend);
                root->GetWorldPosition(value, value);
                camera->SetPos(value[0], value[1], value[2]);
            }

            break;
        case MOTION_KEY_CAMERA_TARGET:
            if (camera != NULL) {
                sceVu0InterVectorXYZ(value, list->values[next], list->values[key], blend);
                root->GetWorldPosition(value, value);
                camera->SetRef(value[0], value[1], value[2]);
            }

            break;
        case MOTION_KEY_MATERIAL_ALPHA: {
            one_minus_blend = 1.0f - blend;
            mgMaterial *materials = frame->visual->GetpMaterial();

            materials[list->target].diffuse[3] = 1.0f - (one_minus_blend * list->values[key][0] + blend * list->values[next][0]);
            frame->attr->unk_28 = 2;
            break;
        }
        case MOTION_KEY_MATERIAL_COLOR: {
            mgMaterial *materials = frame->visual->GetpMaterial();

            sceVu0InterVectorXYZ(materials[list->target].diffuse, list->values[next], list->values[key], blend);
            frame->attr->unk_28 = 2;
            break;
        }
        case MOTION_KEY_CAMERA_ROLL:
            if (camera != NULL) {
                one_minus_blend = 1.0f - blend;
                camera->SetRoll(-((one_minus_blend * list->values[key][0] + blend * list->values[next][0]) / 180.0f * 3.1415927f));
            }

            break;
        case MOTION_KEY_CAMERA_FOV:
            if (camera != NULL) {
                one_minus_blend = 1.0f - blend;
                mgSetProjection(1.0f / tanf((one_minus_blend * list->values[key][0] + blend * list->values[next][0]) * 0.5f / 180.0f * 3.1415927f) * 480.0f * 0.5f);
            }

            break;
        case MOTION_KEY_VISIBLE:
            if (list->values[key][0] < 1.0f) {
                frame->attr->draw = 0;
            } else {
                frame->attr->draw = MG_FRAME_DRAW_VISIBLE | MG_FRAME_DRAW_SKIP_CHILDREN;
            }

            break;
        case MOTION_KEY_UNK_33:
            if (list->values[key][0] < 1.0f) {
                frame->attr->draw = MG_FRAME_DRAW_SKIP_CHILDREN;
            } else {
                frame->attr->draw = MG_FRAME_DRAW_VISIBLE;
            }

            break;
    }

    return list->next;
}

/**
 *
 * Adds a vertex moved by a bone matrix and scaled by its weight to an accumulated vertex, and writes the sum to both the accumulator and an output vertex.
 *
 */
static void testVUnew(float (*matrix)[4], float *vertex, float *weight, float *accum, float *out) {
    asm {
        lqc2 vf4, 0(matrix)
        lqc2 vf5, 0x10(matrix)
        lqc2 vf6, 0x20(matrix)
        lqc2 vf7, 0x30(matrix)
        lqc2 vf8, 0(vertex)
        vmulax.xyzw ACC, vf4, vf8x
        vmadday.xyzw ACC, vf5, vf8y
        vmaddaz.xyzw ACC, vf6, vf8z
        vmaddw.xyzw vf12, vf7, vf8w
        lqc2 vf4, 0(accum)
        lqc2 vf5, 0(weight)
        // Only xyz are weighted; w gains the third matrix row's w.
        vmulx.xyz vf6, vf12, vf5x
        vadd.xyzw vf6, vf4, vf6
        sqc2 vf6, 0(accum)
        sqc2 vf6, 0(out)
    }
}

#ifdef NONMATCHING
Mot_List *MotionProc2(mgCFrame *root, tagMOTION_TYPE *motion, tagFRAME_INF *frame_info, Mot_List *list) {
    static sceVu0FVECTOR *vert;
    static sceVu0FMATRIX  tmp_SkinMatrix;
    static sceVu0FMATRIX  tmp_SkinMatrix_inv;
    static sceVu0FMATRIX  tmp_ChrMatrix;
    static sceVu0FMATRIX  tmp_BaseSkinMatrix;
    static sceVu0FMATRIX  tmp_BaseSkinMatrix_inv;
    mgCFrame             *bone;
    mgCFrame             *skin;
    tagFRAME_INF         *info;
    sceVu0FMATRIX         bone_matrix;
    sceVu0FMATRIX         bone_base;
    sceVu0FMATRIX         bone_in_skin;
    sceVu0FMATRIX         bone_in_skin_inv;
    sceVu0FMATRIX         skin_bone;
    sceVu0FMATRIX         deform;
    sceVu0FVECTOR         moved;
    sceVu0FVECTOR         weight;
    unsigned int          i;
    int                   vertex;

    if (list->key_count == 0) {
        return list->next;
    }

    bone = root->GetFrame(list->target);
    skin = root->GetFrame(list->frame);
    info = &frame_info[list->frame];

    if (OldSkinFrame != skin) {
        OldSkinFrame = root->GetFrame(list->frame);
        vert = ((mgCVisualMDT *) skin->visual)->vertex;

        for (vertex = 0; vertex < (int) info->vertex_count; vertex++) {
            def_vrtx[vertex][0] = 0.0f;
            def_vrtx[vertex][1] = 0.0f;
            def_vrtx[vertex][2] = 0.0f;
            def_vrtx[vertex][3] = 1.0f;
        }

        skin->GetLWMatrix(tmp_SkinMatrix);
        root->GetLWMatrix(tmp_ChrMatrix);
        mgInversMatrix(tmp_SkinMatrix_inv, tmp_SkinMatrix);
        mgMulMatrix(tmp_BaseSkinMatrix, tmp_ChrMatrix, motion->base_matrices[list->frame]);
        mgInversMatrix(tmp_BaseSkinMatrix_inv, tmp_BaseSkinMatrix);
    }

    sceVu0UnitMatrix(bone_matrix);
    sceVu0UnitMatrix(tmp_SkinMatrix);
    bone->GetLWMatrix(bone_matrix);
    mgMulMatrix(bone_base, tmp_ChrMatrix, motion->base_matrices[list->target]);
    mgMulMatrix(bone_in_skin, tmp_BaseSkinMatrix_inv, bone_base);
    mgInversMatrix(bone_in_skin_inv, bone_in_skin);
    mgMulMatrix(skin_bone, tmp_SkinMatrix_inv, bone_matrix);
    mgMulMatrix(deform, skin_bone, bone_in_skin_inv);

    for (i = 0; i < list->key_count; i++) {
        weight[0] = list->values[i][0] * 0.01f;
        vertex = list->key_frames[i];

        if (list->type == MOTION_KEY_SKIN_WEIGHTED) {
            testVUnew(deform, info->base_vertices[vertex], weight, def_vrtx[vertex], vert[vertex]);
        } else {
            sceVu0ApplyMatrix(moved, deform, info->base_vertices[vertex]);
            moved[3] = 0.0f;
            def_vrtx[vertex][0] += moved[0];
            def_vrtx[vertex][1] += moved[1];
            def_vrtx[vertex][2] += moved[2];
            def_vrtx[vertex][0] /= def_vrtx[vertex][3];
            def_vrtx[vertex][1] /= def_vrtx[vertex][3];
            def_vrtx[vertex][2] /= def_vrtx[vertex][3];
            sceVu0CopyVectorXYZ(vert[vertex], def_vrtx[vertex]);
            def_vrtx[vertex][3] += 1.0f;
        }
    }

    return list->next;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", MotionProc2__FP8mgCFrameP14tagMOTION_TYPEP12tagFRAME_INFP8Mot_List);
#endif

Mot_List *MotionProc3(mgCFrame *root, tagMOTION_TYPE *motion, tagFRAME_INF *frame_info, Mot_List *list) {
    static sceVu0FVECTOR *vert;
    static sceVu0FVECTOR *nml;
    static sceVu0FMATRIX  tmp_SkinMatrix;
    static sceVu0FMATRIX  tmp_SkinMatrix_inv;
    static sceVu0FMATRIX  tmp_ChrMatrix;
    static sceVu0FMATRIX  tmp_BaseSkinMatrix;
    static sceVu0FMATRIX  tmp_BaseSkinMatrix_inv;

    sceVu0FMATRIX deform;
    sceVu0FMATRIX rotate;
    sceVu0FMATRIX bone_matrix;
    sceVu0FMATRIX bone_in_skin;
    sceVu0FMATRIX skin_bone;
    sceVu0FMATRIX bone_in_skin_inv;
    sceVu0FMATRIX bone_base;
    sceVu0FVECTOR normal;
    mgCFrame *bone;
    mgCFrame *skin;
    int vertex;
    unsigned int i;

    if (list->type != MOTION_KEY_SKIN_WEIGHTED) {
        return list->next;
    }

    bone = root->GetFrame(list->target);
    skin = root->GetFrame(list->frame);

    if (OldSkinFrame != skin) {
        OldSkinFrame = root->GetFrame(list->frame);
        mgCVisualMDT *visual = (mgCVisualMDT *)skin->visual;
        vert = visual->vertex;
        nml = visual->normal;

        if (frame_info[list->frame].vertex_count > 400) {
            printf("###### MAX_VERTX OVER %d/%d######\n", frame_info[list->frame].vertex_count, 400);
        }

        if (frame_info[list->frame].normal_count > 800) {
            printf("###### MAX_NORMAL OVER %d/%d######\n", frame_info[list->frame].normal_count, 800);
        }

        for (i = 0; i < frame_info[list->frame].vertex_count; i++) {
            def_vrtx[i][0] = 0.0f;
            def_vrtx[i][1] = 0.0f;
            def_vrtx[i][2] = 0.0f;
            def_vrtx[i][3] = 1.0f;
        }

        for (unsigned int normal_index = 0; normal_index < frame_info[list->frame].normal_count; normal_index++) {
            def_nml[normal_index][0] = 0.0f;
            def_nml[normal_index][1] = 0.0f;
            def_nml[normal_index][2] = 0.0f;
            def_nml[normal_index][3] = 1.0f;
        }

        skin->attr->unk_28 = 1;
        skin->GetLWMatrix(tmp_SkinMatrix);
        root->GetLWMatrix(tmp_ChrMatrix);
        sceVu0InversMatrix(tmp_SkinMatrix_inv, tmp_SkinMatrix);
        mgMulMatrix(tmp_BaseSkinMatrix, tmp_ChrMatrix, motion->base_matrices[list->frame]);
        mgInversMatrix(tmp_BaseSkinMatrix_inv, tmp_BaseSkinMatrix);
    }

    sceVu0UnitMatrix(bone_matrix);
    sceVu0UnitMatrix(tmp_SkinMatrix);
    bone->GetLWMatrix(bone_matrix);
    mgMulMatrix(bone_base, tmp_ChrMatrix, motion->base_matrices[list->target]);
    mgMulMatrix(bone_in_skin, tmp_BaseSkinMatrix_inv, bone_base);
    mgInversMatrix(bone_in_skin_inv, bone_in_skin);
    mgMulMatrix(skin_bone, tmp_SkinMatrix_inv, bone_matrix);
    mgMulMatrix(deform, skin_bone, bone_in_skin_inv);

    // Normals turn with the bone but do not move with it.
    sceVu0CopyMatrix(rotate, deform);
    rotate[3][0] = 0.0f;
    rotate[3][1] = 0.0f;
    rotate[3][2] = 0.0f;

    for (unsigned int index = 0; index < list->key_count; index++) {
        float weight[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        weight[0] = 0.01f * list->values[index][0];

        if (!(weight[0] <= 0.0f)) {
            vertex = list->key_frames[index];
            testVUnew(deform, frame_info[list->frame].base_vertices[vertex], weight, def_vrtx[vertex], vert[vertex]);
            sceVu0ApplyMatrix(normal, rotate, frame_info[list->frame].base_normals[vertex]);
            sceVu0InterVectorXYZ(nml[vertex], normal, frame_info[list->frame].base_normals[vertex], weight[0]);
        }
    }

    return list->next;
}

void SetMotionTime(mgCFrame *root, tagMOTION_TYPE *motion, float time, mgCCamera *camera) {
    Mot_List *list;

    for (list = motion->motion_list; list != NULL;) {
        list = MotionProc(root, time, list, camera);
    }
}

void ChangeMotion(mgCFrame *root, tagMOTION_TYPE *motion, unsigned int from_frame, unsigned int to_frame, float blend, mgCCamera *camera) {
    Mot_List *list;

    for (list = motion->motion_list; list != NULL;) {
        list = MotionProc(root, from_frame, to_frame, blend, list, camera);
    }
}

void DeformMesh(mgCFrame *root, tagMOTION_TYPE *motion, tagFRAME_INF *frame_info, bool with_normals) {
    Mot_List *list = motion->skin_list;

    if (with_normals) {
        while (list != NULL) {
            list = MotionProc3(root, motion, frame_info, list);
        }
    } else {
        while (list != NULL) {
            list = MotionProc2(root, motion, frame_info, list);
        }
    }

    OldSkinFrame = NULL;
}

/**
 *
 * Allocates a key list's values and key frames and fills them from the keys of a motion file.
 *
 */
static void SetKeyFrame(Mot_List *list, FRAME_VECTOR_EX_DATA *keys, mgCMemory *memory) {
    
    list->values = (sceVu0FVECTOR *)memory->Alloc((list->key_count * sizeof(sceVu0FVECTOR) / 16) + 1);
    list->key_frames = (u32 *)memory->Alloc((list->key_count * 4 >> 4) + 1);
    u32 i = 0;
    while (i < list->key_count) {
        float *value = list->values[i];
        u32 *time = &list->key_frames[i];
        i++;
        value[0] = keys->value[0];
        value[1] = keys->value[1];
        value[2] = keys->value[2];
        value[3] = keys->value[3];
        *time = keys->frame;
        keys++;
    }
}

void ChangeWeight(Mot_List *list, mgCMemory *memory, unsigned char *file, int frame, tagFRAME_INF *frame_info,
                  mgCVisualMDT *visual, mgCFrame *root, mgCFrame *skin_root) {
    Mot_List *last;
    Mot_File_List *header;
    Mot_List *built;
    FRAME_VECTOR_EX_DATA *data;
    Mot_List *reversed;
    Mot_List *node;
    Mot_List *following;
    FRAME_VECTOR_EX_DATA *keys;
    sceVu0FVECTOR *vertices;
    sceVu0FVECTOR *normals;
    int i;
    mgFACE_GROUP *group;
    mgCFace *face;
    int *indices;
    int index;
    int *refs;

    // Unlink the frame's old key lists, leaving last on the final list that remains.
    last = list;

    for (node = list; node != NULL; node = node->next) {
        if (frame == node->frame) {
            last->next = node->next;
        } else {
            last = node;
        }
    }
    data = (FRAME_VECTOR_EX_DATA *)file;
    built = NULL;
    do {
        header = (Mot_File_List *)data;
        node = (Mot_List *)memory->Alloc(sizeof(Mot_List) / 16 + 1);
        node->frame = frame;
        
        node->target =
            root->SearchFrameID(skin_root->GetFrame((u32)header->target)->name);
        node->key_count = header->key_count;
        node->type = header->type;
        data++;
        keys = data;
        data += node->key_count;
        SetKeyFrame(node, keys, memory);
        if (built == NULL) {
            node->next = NULL;
        } else {
            node->next = built;
        }
        built = node;
    } while ((u64)header->more != 0);
    reversed = NULL;
    if (node != NULL) {
        do {
            following = reversed;
            reversed = built;
            built = built->next;
            reversed->next = following;
        } while (built != NULL);
    }
    last->next = reversed;
    if (visual != NULL) {
        vertices = visual->vertex;
        normals = visual->normal;
        frame_info[frame].base_vertices = (float(*)[4])memory->Alloc((visual->vertex_num * 16U / 16) + 1);
        frame_info[frame].base_normals = (float(*)[4])memory->Alloc(((u32)visual->normal_num * 16 / 16) + 1);
        frame_info[frame].vertex_refs =
            (int(*)[12])memory->Alloc(((u32)(visual->vertex_num * 0x30) >> 4) + 1);
        frame_info[frame].vertex_count = visual->vertex_num;
        frame_info[frame].normal_count = visual->normal_num;
        memcpy(frame_info[frame].base_vertices, vertices, visual->vertex_num * 16);
        memcpy(frame_info[frame].base_normals, normals, visual->normal_num * 16);
        for (i = 0; i < visual->vertex_num; i++) {
            frame_info[frame].vertex_refs[i][0] = 0;
        }
        group = (mgFACE_GROUP *)visual->face_group;
        if (group != NULL) {
            do {
                face = group->face;
                if (face != NULL) {
                    do {
                        indices = face->index;
                        index = 0;
                        if (!(group->face->type & MG_FACE_NO_NORMAL)) {
                            while (index < face->index_num) {
                                int vertex = indices[index];
                                index++;
                                int normal = indices[index];
                                index += face->index_stride - 1;
                                refs = frame_info[frame].vertex_refs[vertex];
                                refs[refs[0] + 1] = normal;
                                refs = frame_info[frame].vertex_refs[vertex];
                                refs[0]++;
                            }
                            face = face->next;
                        } else {
                            break;
                        }
                    } while (face != NULL);
                }
                group = group->next;
            } while (group != NULL);
        }
    }
}

int CreateAnimeDataEX(tagMOTION_TYPE *motion, mgCMemory *memory, MOTION_FILE_INFO *files) {
    FRAME_VECTOR_EX_DATA *data;
    Mot_List *list;
    Mot_File_List *header;
    FRAME_VECTOR_EX_DATA *keys;
    FRAME_VECTOR_EX_DATA *skin_data;
    Mot_List *skin_list;
    Mot_File_List *skin_header;
    FRAME_VECTOR_EX_DATA *skin_keys;
    Mot_List *head;
    Mot_List *reversed;
    Mot_List *following;
    Mot_List *node;

    if (files[0].name != NULL) {
        motion->base_matrices = (sceVu0FMATRIX *) memory->Alloc(files[0].size / 16 + 1);
        memcpy(motion->base_matrices, files[0].data, files[0].size);
    }

    if (files[1].name != NULL) {
        data = (FRAME_VECTOR_EX_DATA *) files[1].data;
        motion->motion_list = NULL;
        do {
            header = (Mot_File_List *) data;
            list = (Mot_List *) memory->Alloc(sizeof(Mot_List) / 16 + 1);
            list->frame = header->frame;
            list->target = header->target;
            list->key_count = header->key_count;
            list->type = header->type;
            data++;
            keys = data;
            data += list->key_count;
            SetKeyFrame(list, keys, memory);
            head = motion->motion_list;
            if (head == NULL) {
                list->next = NULL;
            } else {
                list->next = motion->motion_list;
            }
            motion->motion_list = list;
        } while ((u64)header->more != 0); 
        reversed = NULL;
        while ((node = motion->motion_list) != NULL) {
            following = reversed;
            reversed = node;
            motion->motion_list = node->next;
            node->next = following;
        }
        motion->motion_list = reversed;
    }
    if (files[2].name != 0) {
        skin_data = (FRAME_VECTOR_EX_DATA *)files[2].data;
        motion->skin_list = NULL;
        do {
            skin_header = (Mot_File_List *)skin_data;
            skin_list = (Mot_List *)memory->Alloc(sizeof(Mot_List) / 16 + 1);
            skin_list->frame = skin_header->frame;
            skin_list->target = skin_header->target;
            skin_list->key_count = skin_header->key_count;
            skin_list->type = skin_header->type;
            skin_data++;
            skin_keys = skin_data;
            skin_data += skin_list->key_count;
            SetKeyFrame(skin_list, skin_keys, memory);
            head = motion->skin_list;
            if (head == NULL) {
                skin_list->next = NULL;
            } else {
                skin_list->next = head;
            }
            motion->skin_list = skin_list;
        } while ((u64)skin_header->more != 0);
        reversed = NULL;
        while ((node = motion->skin_list) != NULL) {
            following = reversed;
            reversed = node;
            motion->skin_list = node->next;
            node->next = following;
        }
        motion->skin_list = reversed;
    }
    return 1;
}

void AnimeDataInit(mgCFrame *root, tagMOTION_TYPE *motion, mgCMemory *memory, tagFRAME_INF **frame_info) {
    *frame_info = (tagFRAME_INF *) memory->stAlloc64((root->GetFrameNum() + 10) * sizeof(tagFRAME_INF) / 16 + 1);
    AnimeDataInit(root, motion, memory, *frame_info);
}

int AnimeDataInit(mgCFrame *root, tagMOTION_TYPE *motion, mgCMemory *memory,
                  tagFRAME_INF *frame_info) {
    Mot_List *list = motion->skin_list;
    int i;
    int count;
    int vertex;
    mgCVisualMDT *visual;
    tagFRAME_INF *info;
    mgFACE_GROUP *group;
    mgCFace *face;
    int *indices;
    int index;
    int *refs;
    sceVu0FVECTOR *vertices;
    sceVu0FVECTOR *normals;
    mgCFrame *skin;

    count = root->GetFrameNum();
    i = 0;
    if (i < count) {
        do {
            int parent =
                root->GetFrame(i)->parent - root;
            tagFRAME_INF *info = &frame_info[i];
            i++;
            info->parent = parent;
            info->vertex_count = 0;
            info->normal_count = 0;
        } while (i < count);
    }
    if (list != NULL) {
        do {
            if (list->type == MOTION_KEY_SKIN_WEIGHTED || list->type == MOTION_KEY_SKIN_AVERAGED) {
                root->GetFrame(list->target);
                skin = root->GetFrame(list->frame);
                // A frame's skinning data is built once, from the first key list that skins it.
                if (frame_info[list->frame].vertex_count == 0 && skin != NULL) {
                    visual = (mgCVisualMDT *)skin->visual;
                    
                    if (visual != NULL && visual != NULL) {
                        vertices = visual->vertex;
                        normals = visual->normal;
                        frame_info[list->frame].base_vertices =
                            (float(*)[4])memory->Alloc((visual->vertex_num * 16U / 16) + 1);
                        frame_info[list->frame].base_normals =
                            (float(*)[4])memory->Alloc(((u32)visual->normal_num * 16 / 16) + 1);
                        frame_info[list->frame].vertex_refs = (int(*)[12])memory->Alloc(
                            ((u32)(visual->vertex_num * 0x30) >> 4) + 1);
                        frame_info[list->frame].vertex_count = visual->vertex_num;
                        frame_info[list->frame].normal_count = visual->normal_num;
                        memcpy(frame_info[list->frame].base_vertices, vertices,
                               visual->vertex_num * 16);
                        memcpy(frame_info[list->frame].base_normals, normals, visual->normal_num * 16);
                        for (vertex = 0; vertex < visual->vertex_num; vertex++) {
                            frame_info[list->frame].vertex_refs[vertex][0] = 0;
                        }
                        group = (mgFACE_GROUP *)visual->face_group;
                        if (group != NULL) {
                            do {
                                face = group->face;
                                if (face != NULL) {
                                    do {
                                        indices = face->index;
                                        index = 0;
                                        if (!(group->face->type & MG_FACE_NO_NORMAL)) {
                                            while (index < face->index_num) {
                                                int vertex_index = indices[index];
                                                index++;
                                                int normal_index = indices[index];
                                                index += face->index_stride - 1;
                                                refs = frame_info[list->frame].vertex_refs[vertex_index];
                                                refs[refs[0] + 1] = normal_index;
                                                refs = frame_info[list->frame].vertex_refs[vertex_index];
                                                refs[0]++;
                                            }
                                            face = face->next;
                                        } else {
                                            break;
                                        }
                                    } while (face != NULL);
                                }
                                group = group->next;
                            } while (group != NULL);
                        }
                    }
                }
            }
            list = list->next;
        } while (list != NULL);
    }
    return 1;
}

int CheckHit(CCPoly *polys, int count, float *from, float *to, float *hit_point, int nearest, int ignore_mask) {
    CollisionInfo info;

    info.unk_08 = 0;
    info.unk_0C = 0;
    info.count = count;
    info.polys = polys;
    return CheckHit(&info, from, to, hit_point, nearest, ignore_mask);
}

// Keep collision-vector calculations in their stated order.
#pragma global_optimizer off
int CheckHit(CollisionInfo *info, float *from, float *to, float *hit_point, int nearest, int ignore_mask) {
    sceVu0FVECTOR point;
    sceVu0FVECTOR diff;
    sceVu0FVECTOR poly_min;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR line_max;
    sceVu0FVECTOR line_min;
    sceVu0FVECTOR offset;
    float best;
    float from_side;
    float to_side;
    float dist;
    int i;
    int hit;
    CCPoly *poly;
    int count;
    int found;

    if (info == NULL) {
        return 0;
    }
    hit = -1;
    found = 0;
    mgVectorMaxMin(line_max, line_min, from, to);
    
    {
        float *min_ptr;
        float *max_ptr;
        max_ptr = line_max;
        min_ptr = line_min;
        asm {
            lqc2 vf10, 0(max_ptr)
            lqc2 vf11, 0(min_ptr)
        }
    }
    poly = info->polys;
    count = info->count;
    if (poly == NULL || count == 0) {
        return -1;
    }
    for (i = 0; i < count; i++, poly++) {
        if (poly->ignore_mask & ignore_mask) {
            continue;
        }
        
        mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);
        if (line_max[0] < poly_min[0] || line_max[1] < poly_min[1] || line_max[2] < poly_min[2]) {
            continue;
        }

        if (line_min[0] > poly_max[0] || line_min[1] > poly_max[1] || line_min[2] > poly_max[2]) {
            continue;
        }
        
        sceVu0SubVector(offset, from, poly->vertex[0]);
        from_side = sceVu0InnerProduct(poly->normal, offset);
        sceVu0SubVector(offset, to, poly->vertex[0]);
        to_side = sceVu0InnerProduct(poly->normal, offset);

        if (from_side > 0.0f && to_side > 0.0f) {
            continue;
        }
        if (from_side < 0.0f && to_side < 0.0f) {
            continue;
        }
        if (mgIntersectionPoint_line_poly3(from, to, poly->vertex[0], poly->vertex[1],
                                           poly->vertex[2], poly->normal, point) == 0) {
            continue;
        }
        if (nearest == 0) {
            hit = i;
            sceVu0CopyVector(hit_point, point);
            break;
        }
        diff[0] = from[0] - point[0];
        diff[1] = from[1] - point[1];
        diff[2] = from[2] - point[2];
        dist = (diff[0] * diff[0]) + (diff[1] * diff[1]) + (diff[2] * diff[2]);
        if (found == 0) {
            best = dist;
            hit = i;
            sceVu0CopyVector(hit_point, point);
        } else if (!(best <= dist)) {
            best = dist;
            hit = i;
            sceVu0CopyVector(hit_point, point);
        }
        found = 1;
    }
    return hit;
}
#pragma global_optimizer reset

int CheckHitVertical(CCPoly *polys, int count, float *from, float height, float *hit_point, int ignore_mask) {
    CollisionInfo info;

    info.unk_08 = 0;
    info.unk_0C = 0;
    info.count = count;
    info.polys = polys;
    return CheckHitVertical(&info, from, height, hit_point, ignore_mask);
}

int CheckHitVertical(CollisionInfo *info, float *from, float height, float *hit_point, int ignore_mask) {
    float to[3];
    float best_y;
    int i;
    int best;
    CCPoly *poly;
    int count;

    if (info == NULL) {
        return -1;
    }
    to[0] = from[0];
    to[1] = from[1] + height;
    to[2] = from[2];
    poly = info->polys;
    count = info->count;
    best = -1;
    if (poly == NULL || count == 0) {
        return -1;
    }
    for (i = 0; i < count; i++, poly++) {
        if (!(poly->ignore_mask & ignore_mask) &&
            mgIntersectionPoint_line_poly3(from, to, poly->vertex[0], poly->vertex[1],
                                           poly->vertex[2], poly->normal, hit_point) != 0) {
            if (height <= 0.0f) {
                // Looking down: keep the highest polygon below the point.
                if (!(from[1] <= hit_point[1]) && (best < 0 || (best >= 0 && best_y <= hit_point[1]))) {
                    best = i;
                    best_y = hit_point[1];
                }
            } else {
                // Looking up: keep the lowest polygon above the point.
                if (from[1] < hit_point[1] && (best < 0 || (best >= 0 && !(best_y < hit_point[1])))) {
                    best = i;
                    best_y = hit_point[1];
                }
            }
        }
    }
    if (best >= 0) {
        hit_point[1] = best_y;
    }
    return best;
}

int CheckHits(CCPoly *polys, int count, float *from, float *to, int max_hits, int *hit_polys, float (*hit_points)[4], int sort, int ignore_mask) {
    CollisionInfo info;

    info.unk_08 = 0;
    info.unk_0C = 0;
    info.count = count;
    info.polys = polys;
    return CheckHits(&info, from, to, max_hits, hit_polys, hit_points, sort, ignore_mask);
}

#ifdef NONMATCHING
int CheckHits(CollisionInfo *info, float *from, float *to, int max_hits, int *hit_polys, float (*hit_points)[4], int sort, int ignore_mask) {
    sceVu0FVECTOR point;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR poly_min;
    sceVu0FVECTOR line_max;
    sceVu0FVECTOR line_min;
    sceVu0FVECTOR offset;
    sceVu0FVECTOR swap;
    CCPoly       *poly;
    int           count;
    int           i;
    int           j;
    int           hits;
    float         from_side;
    float         to_side;

    hits = 0;
    mgVectorMaxMin(line_max, line_min, from, to);
    poly = info->polys;
    count = info->count;

    for (i = 0; i < count; i++, poly++) {
        if (poly->ignore_mask & ignore_mask) {
            continue;
        }

        mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);

        if (poly_min[0] > line_max[0] || poly_min[1] > line_max[1] || poly_min[2] > line_max[2]) {
            continue;
        }

        if (line_min[0] > poly_max[0] || line_min[1] > poly_max[1] || line_min[2] > poly_max[2]) {
            continue;
        }

        sceVu0SubVector(offset, from, poly->vertex[0]);
        from_side = sceVu0InnerProduct(poly->normal, offset);
        sceVu0SubVector(offset, to, poly->vertex[0]);
        to_side = sceVu0InnerProduct(poly->normal, offset);

        if (from_side > 0.0f && to_side > 0.0f) {
            continue;
        }

        if (from_side < 0.0f && to_side < 0.0f) {
            continue;
        }

        if (mgIntersectionPoint_line_poly3(from, to, poly->vertex[0], poly->vertex[1], poly->vertex[2], poly->normal, point) == 0) {
            continue;
        }

        if (hits >= max_hits) {
            break;
        }

        hit_polys[hits] = i;
        sceVu0CopyVector(hit_points[hits], point);
        hit_points[hits][3] = mgDistVector(from, point);
        hits++;
    }

    if (sort != 0) {
        if (sort > 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (hit_points[j][3] < hit_points[i][3]) {
                        int index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }

        // A descending sort was never written; it sorts ascending as well.
        if (sort < 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (hit_points[j][3] < hit_points[i][3]) {
                        int index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }
    }

    return hits;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", CheckHits__FP13CollisionInfoPfPfiPiPA4_fii);
#endif

int CheckHitsPipeY(CCPoly *polys, int count, float *from, float height, int max_hits, int *hit_polys, sceVu0FVECTOR *hit_points, int sort, int ignore_mask) {
    sceVu0FVECTOR best;
    sceVu0FVECTOR poly_min;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR pipe_max;
    sceVu0FVECTOR pipe_min;
    sceVu0FVECTOR top;
    sceVu0FVECTOR points[11];
    sceVu0FVECTOR swap;
    CCPoly       *poly;
    float         radius;
    float         normal_y;
    int           point_count;
    int           j;
    int           found;
    int           hits;
    int           i;
    int index;

    hits = 0;
    radius = from[3];
    *(u_long128 *)top = *(u_long128 *)from;
    top[1] += height;
    mgVectorMaxMin(pipe_max, pipe_min, from, top);
    pipe_max[0] += radius;
    pipe_max[2] += radius;
    pipe_min[0] -= radius;
    pipe_min[2] -= radius;
    poly = polys;

    for (i = 0; i < count; i++, poly++) {
        if (poly->ignore_mask & ignore_mask) {
            continue;
        }

        // A wall that is nearly upright has no height under the pipe.
        normal_y = (poly->normal[1] < 0.0f) ? -poly->normal[1] : poly->normal[1];

        if (normal_y < 0.01f) {
            continue;
        }

        mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);

        if (pipe_max[0] < poly_min[0] || pipe_max[1] < poly_min[1] || pipe_max[2] < poly_min[2]) {
            continue;
        }

        if (pipe_min[0] > poly_max[0] || pipe_min[1] > poly_max[1] || pipe_min[2] > poly_max[2]) {
            continue;
        }

        point_count = IntersectionPipeYPoly3(from, poly->vertex, poly->normal, points);

        if (point_count <= 0) {
            continue;
        }

        // Keep the highest point that lies within the pipe's height.
        found = 0;

        for (j = 0; j < point_count; j++) {
            if (points[j][1] <= pipe_max[1] && !(points[j][1] < pipe_min[1])) {
                if (found == 0) {
                    *(u_long128 *)best = *(u_long128 *)points[j];
                    found = 1;
                } else if (!(points[j][1] <= best[1])) {
                    *(u_long128 *)best = *(u_long128 *)points[j];
                }
            }
        }

        if (found == 0) {
            continue;
        }

        if (hits >= max_hits) {
            break;
        }

        hit_polys[hits] = i;
        sceVu0CopyVector(hit_points[hits], best);
        hit_points[hits][3] = mgDistVector(from, best);
        hits++;
    }

    if (sort == 0) {
        return hits;
    }
    {
        if (sort > 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }

        // A descending sort was never written; it sorts ascending as well.
        if (sort < 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }
    }

    return hits;
}

int CheckHitsPipe(CCPoly *polys, int count, sceVu0FVECTOR from, float *to, int max_hits, int *hit_polys, sceVu0FVECTOR *hit_points, int sort, int ignore_mask) {
    sceVu0FVECTOR best;
    sceVu0FVECTOR poly_min;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR pipe_max;
    sceVu0FVECTOR pipe_min;
    sceVu0FVECTOR dir;
    sceVu0FVECTOR points[11];
    sceVu0FVECTOR offset;
    sceVu0FVECTOR swap;
    CCPoly       *poly;
    float         radius;
    float         length;
    float         along;
    int           point_count;
    int           j;
    int           found;
    int           hits;
    int           i;
    int index;

    hits = 0;
    radius = from[3];
    length = mgDistVector(from, to);
    mgVectorMaxMin(pipe_max, pipe_min, from, to);
    sceVu0SubVector(dir, to, from);
    sceVu0Normalize(dir, dir);
    pipe_max[0] += radius;
    pipe_max[1] += radius;
    pipe_max[2] += radius;
    pipe_min[0] -= radius;
    pipe_min[1] -= radius;
    pipe_min[2] -= radius;
    poly = polys;

    for (i = 0; i < count; i++, poly++) {
        if (poly->ignore_mask & ignore_mask) {
            continue;
        }

        mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);

        if (pipe_max[0] < poly_min[0] || pipe_max[1] < poly_min[1] || pipe_max[2] < poly_min[2]) {
            continue;
        }

        if (pipe_min[0] > poly_max[0] || pipe_min[1] > poly_max[1] || pipe_min[2] > poly_max[2]) {
            continue;
        }

        point_count = IntersectionPipePoly3(from, dir, poly->vertex, poly->normal, points);

        if (point_count <= 0) {
            continue;
        }

        // Keep the point nearest the pipe's start among those along its length; w holds the distance.
        found = 0;

        for (j = 0; j < point_count; j++) {
            sceVu0SubVector(offset, points[j], from);
            along = sceVu0InnerProduct(offset, dir);
            points[j][3] = along;

            if (along >= 0.0f && along <= length) {
                if (found == 0) {
                    *(u_long128 *)best = *(u_long128 *)points[j];
                    found = 1;
                } else if (points[j][3] < best[3]) {
                    *(u_long128 *)best = *(u_long128 *)points[j];
                }
            }
        }

        if (found == 0) {
            continue;
        }

        if (hits >= max_hits) {
            break;
        }

        hit_polys[hits] = i;
        sceVu0CopyVector(hit_points[hits], best);
        hits++;
    }

    if (sort == 0) {
        return hits;
    }
    {
        if (sort > 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }

        // A descending sort was never written; it sorts ascending as well.
        if (sort < 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }
    }

    return hits;
}

int CheckHitsSphere(CCPoly *polys, int count, float *sphere, int max_hits, int *hit_polys, sceVu0FVECTOR *hit_points, int sort, int ignore_mask) {
    sceVu0FVECTOR push;
    sceVu0FVECTOR poly_min;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR sphere_max;
    sceVu0FVECTOR sphere_min;
    sceVu0FVECTOR normal;
    sceVu0FVECTOR swap;
    int i;
    int hits;
    CCPoly *poly;
    int j;
    int index;

    hits = 0;
    float radius = sphere[3];
    *(u_long128 *)sphere_max = *(u_long128 *)sphere;
    *(u_long128 *)sphere_min = *(u_long128 *)sphere;
    sphere_max[0] += radius;
    sphere_max[1] += radius;
    sphere_max[2] += radius;
    sphere_min[0] -= radius;
    sphere_min[1] -= radius;
    sphere_min[2] -= radius;
    poly = polys;

    for (i = 0; i < count; i++, poly++) {
        if (poly->ignore_mask & ignore_mask) {
            continue;
        }

        mgVectorMaxMin(poly_max, poly_min, poly->vertex[0], poly->vertex[1], poly->vertex[2]);

        if (sphere_max[0] < poly_min[0] || sphere_max[1] < poly_min[1] || sphere_max[2] < poly_min[2]) {
            continue;
        }

        if (sphere_min[0] > poly_max[0] || sphere_min[1] > poly_max[1] || sphere_min[2] > poly_max[2]) {
            continue;
        }

        sceVu0Normalize(normal, poly->normal);

        if (IntersectionSpherePoly3(sphere, poly->vertex, normal, push) == SPHERE_POLY3_NONE) {
            continue;
        }

        if (hits >= max_hits) {
            break;
        }

        hit_polys[hits] = i;
        sceVu0CopyVector(hit_points[hits], push);
        hits++;
    }

    if (sort == 0) {
        return hits;
    }
    {
        if (sort > 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }

        // A descending sort was never written; it sorts ascending as well.
        if (sort < 0) {
            for (i = 0; i < hits - 1; i++) {
                for (j = i + 1; j < hits; j++) {
                    if (!(hit_points[i][3] <= hit_points[j][3])) {
                        index = hit_polys[i];

                        hit_polys[i] = hit_polys[j];
                        hit_polys[j] = index;
                        sceVu0CopyVector(swap, hit_points[i]);
                        sceVu0CopyVector(hit_points[i], hit_points[j]);
                        sceVu0CopyVector(hit_points[j], swap);
                    }
                }
            }
        }
    }

    return hits;
}

#ifdef NONMATCHING
int MoveCheck(float *pos, float *velocity, float *out_pos, MoveCheckInfo *info, CCPoly *polys, int count, int ignore_mask) {
    sceVu0FVECTOR ground;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    sceVu0FVECTOR extension;
    int           hit_polys[64];
    sceVu0FVECTOR hit_points[64];
    sceVu0FVECTOR extended_to;
    CCPoly        ground_poly;
    sceVu0FVECTOR ground_query;
    sceVu0FVECTOR wall_query;
    float         radius;
    float         landing_margin;
    int           retries;

    radius = info->radius;
    if (radius <= 0.0f) {
        radius = 15.0f;
    }
    out_pos[0] = pos[0];
    out_pos[1] = pos[1];
    out_pos[2] = pos[2];
    sceVu0Normalize(extension, velocity);
    sceVu0ScaleVector(extension, extension, 0.3f * radius);
    from[0] = pos[0];
    from[1] = 10.0f + pos[1];
    from[2] = pos[2];
    to[0] = from[0] + velocity[0];
    to[1] = from[1] + velocity[1];
    to[2] = from[2] + velocity[2];
    from[3] = 4.0f;
    sceVu0AddVector(extended_to, to, extension);
    retries = 0;

    while (1) {
        if (CheckHitsPipe(polys, count, from, to, 64, hit_polys, hit_points, 1, ignore_mask) <= 0) {
            from[0] = to[0];
            from[1] = to[1];
            from[2] = to[2];
            to[0] = from[0];
            to[1] = from[1] - 10.0f;
            to[2] = from[2];
            break;
        }
        retries++;
        velocity[0] *= 0.5f;
        velocity[2] *= 0.5f;
        to[0] = from[0] + velocity[0];
        to[1] = from[1] + velocity[1];
        to[2] = from[2] + velocity[2];
        if (retries >= 2) {
            break;
        }
    }

    info->ground_found = 0;
    info->landed = 0;
    landing_margin = 4.0f;
    if (velocity[1] > 0.1f) {
        landing_margin = 0.0f;
    }
    sceVu0CopyVector(ground_query, from);
    if (info->skip_ground == 0 && GetFootPoly(ground_query, 20.0f, &ground_poly, ground, polys, count, ignore_mask) != 0) {
        sceVu0Normalize(ground_poly.normal, ground_poly.normal);
        info->ground_poly = ground_poly;
        info->second_poly = ground_poly;
        info->ground_found = 1;
        info->landed = 0;
        *(u_long128 *)info->ground_point = *(u_long128 *)ground;
        if (ground[1] > ((from[1] + velocity[1]) - 10.0f) - landing_margin) {
            info->landed = 1;
        }
    }
    if (info->landed != 0) {
        out_pos[0] = ground[0];
        out_pos[1] = ground[1];
        out_pos[2] = ground[2];
    } else {
        out_pos[0] = to[0];
        out_pos[1] = to[1];
        out_pos[2] = to[2];
    }
    *(u_long128 *)wall_query = *(u_long128 *)out_pos;
    wall_query[1] += 5.0f;
    info->width_result = CheckWidth(polys, count, wall_query, radius, to, ignore_mask);
    if (info->width_result != 0) {
        wall_query[0] = to[0];
        wall_query[2] = to[2];
    }
    wall_query[3] = 4.0f;
    if (CheckWidthPipe(polys, count, wall_query, radius, to, ignore_mask) != 0) {
        out_pos[0] = to[0];
        out_pos[2] = to[2];
    } else {
        out_pos[0] = wall_query[0];
        out_pos[2] = wall_query[2];
    }
    if (info->skip_ground == 0) {
        sceVu0CopyVector(ground_query, from);
        if (GetFootPoly(ground_query, 20.0f, &ground_poly, ground, polys, count, ignore_mask) != 0) {
            *(u_long128 *)info->ground_point = *(u_long128 *)ground;
            if (ground[1] > ((from[1] + velocity[1]) - 10.0f) - landing_margin) {
                out_pos[1] = ground[1];
            }
        }
    }
    GetCPolyAttr(info, pos, out_pos, 34.0f, polys, count, ignore_mask);
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", MoveCheck__FPfPfPfP13MoveCheckInfoP6CCPolyii);
#endif

#ifdef NONMATCHING
int GetFootPoly(float *pos, float depth, CCPoly *found, float *ground, CCPoly *polys, int count, int ignore_mask) {
    s16           ground_kind;
    s16           foot_sound;
    s16           area_kind;
    s16           poly_ignore_mask;
    u16           parts_no;
    s16           attribute;
    float         attribute_value;
    int           hit_polys[32];
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    sceVu0FVECTOR hit_points[32];
    sceVu0FVECTOR normal;
    float         normal_y;
    CCPoly       *poly;
    int           hits;
    int           found_ground;
    int           i;

    sceVu0CopyVector(from, pos);
    sceVu0CopyVector(to, pos);
    from[3] = 4.0f;
    to[1] -= depth;
    hits = CheckHitsPipeY(polys, count, from, -depth, 32, hit_polys, hit_points, 1, ignore_mask);
    if (hits == 0) {
        return 0;
    }
    ground_kind = 0;
    foot_sound = 0;
    area_kind = 0;
    found_ground = 0;
    for (i = 0; i < hits; i++) {
        sceVu0Normalize(normal, polys[hit_polys[i]].normal);
        normal_y = normal[1];
        if (normal_y < 0.0f) {
            normal_y = -normal_y;
        }
        if (normal_y < 0.05f) {
            continue;
        }
        *found = polys[hit_polys[i]];
        sceVu0CopyVector(ground, hit_points[i]);
        found_ground = 1;
        ground[0] = from[0];
        ground[2] = from[2];
        ground_kind = found->ground_kind;
        foot_sound = found->foot_sound;
        area_kind = found->area_kind;
        poly_ignore_mask = found->ignore_mask;
        parts_no = found->parts_no;
        attribute = found->unk_4a;
        attribute_value = found->unk_4c;
        break;
    }
    for (i = 0; i < hits; i++) {
        poly = &polys[hit_polys[i]];
        if (ground_kind == 0) {
            ground_kind = poly->ground_kind;
        }
        if (foot_sound == 0) {
            foot_sound = poly->foot_sound;
        }
        if (area_kind == 0) {
            area_kind = poly->area_kind;
        }
    }
    found->ground_kind = ground_kind;
    found->foot_sound = foot_sound;
    found->area_kind = area_kind;
    found->ignore_mask = poly_ignore_mask;
    found->parts_no = parts_no;
    found->unk_4a = attribute;
    found->unk_4c = attribute_value;
    return found_ground;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", GetFootPoly__FPffP6CCPolyPfP6CCPolyii);
#endif

void GetCPolyAttr(MoveCheckInfo *info, float *from, float *to, float height, CCPoly *polys, int count,
                  int ignore_mask) {
    int hit_polys[32];
    sceVu0FVECTOR probe_from;
    sceVu0FVECTOR probe_to;
    sceVu0FVECTOR hit_points[64];
    int hits;
    int i;
    s16 kind;

    info->in_water = 0;
    info->crossed_area = 0;
    hits = CheckHits(polys, count, from, to, 32, hit_polys, hit_points, 1, 0);
    for (i = 0; i < hits; i++) {
        kind = polys[hit_polys[i]].area_kind;

        switch (kind) {
            case 1:
            case 7:
                info->crossed_area = 1;
                info->signed_distance = mgDistVector(from, to);
                if (!(from[1] <= to[1])) {
                    info->signed_distance *= -1.0f;
                }

                *(u_long128 *)info->crossed_point = *(u_long128 *)hit_points[i];
                break;
        }
    }
    sceVu0CopyVector(probe_from, to);
    sceVu0CopyVector(probe_to, to);
    probe_from[1] += height;
    hits = CheckHits(polys, count, probe_from, probe_to, 0x20, hit_polys, hit_points, 1, 0);
    if (hits == 0) {
        return;
    }
    for (i = 0; i < hits; i++) {
        kind = polys[hit_polys[i]].area_kind;
        switch (kind) {
            case 1:
            case 7:
                info->in_water = 1;
                *(u_long128 *)info->water_surface = *(u_long128 *)hit_points[i];
                break;
        }
    }
}

int CheckWidth(CCPoly *polys, int count, float *pos, float radius, float *out_pos, int ignore_mask) {
    sceVu0FVECTOR to;
    sceVu0FVECTOR positive_hit;
    sceVu0FVECTOR negative_hit;
    sceVu0FVECTOR from;
    sceVu0FVECTOR normal;
    float         diagonal_radius;
    int           sides;
    int           positive;
    int           negative;
    int           hit;

    sides = 0;
    diagonal_radius = radius / 1.4142135f;
    sceVu0CopyVector(from, pos);
    sceVu0CopyVector(out_pos, pos);
    negative = 0;
    positive = 0;
    to[0] = from[0] + diagonal_radius;
    to[1] = from[1];
    to[2] = from[2] + diagonal_radius;
    hit = CheckHit(polys, count, from, to, positive_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && normal[1] > -0.5f) {
            positive = 1;
            sides |= CHECK_WIDTH_SIDE_POS_X | CHECK_WIDTH_SIDE_POS_Z;
        }
    }
    to[0] = from[0] - diagonal_radius;
    to[1] = from[1];
    to[2] = from[2] - diagonal_radius;
    hit = CheckHit(polys, count, from, to, negative_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_X | CHECK_WIDTH_SIDE_NEG_Z;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[0] = 0.5f * (positive_hit[0] + negative_hit[0]);
        out_pos[2] = 0.5f * (positive_hit[2] + negative_hit[2]);
    } else {
        if (positive != 0) {
            out_pos[0] = positive_hit[0] - diagonal_radius;
            out_pos[2] = positive_hit[2] - diagonal_radius;
        }
        if (negative != 0) {
            out_pos[0] = negative_hit[0] + diagonal_radius;
            out_pos[2] = negative_hit[2] + diagonal_radius;
        }
    }
    sceVu0CopyVector(from, out_pos);
    negative = 0;
    positive = 0;
    to[0] = from[0] + diagonal_radius;
    to[1] = from[1];
    to[2] = from[2] - diagonal_radius;
    hit = CheckHit(polys, count, from, to, positive_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && normal[1] > -0.5f) {
            positive = 1;
            sides |= CHECK_WIDTH_SIDE_POS_X | CHECK_WIDTH_SIDE_NEG_Z;
        }
    }
    to[0] = from[0] - diagonal_radius;
    to[1] = from[1];
    to[2] = from[2] + diagonal_radius;
    hit = CheckHit(polys, count, from, to, negative_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_X | CHECK_WIDTH_SIDE_POS_Z;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[0] = 0.5f * (positive_hit[0] + negative_hit[0]);
        out_pos[2] = 0.5f * (positive_hit[2] + negative_hit[2]);
    } else {
        if (positive != 0) {
            out_pos[0] = positive_hit[0] - diagonal_radius;
            out_pos[2] = positive_hit[2] + diagonal_radius;
        }
        if (negative != 0) {
            out_pos[0] = negative_hit[0] + diagonal_radius;
            out_pos[2] = negative_hit[2] - diagonal_radius;
        }
    }
    sceVu0CopyVector(from, out_pos);
    negative = 0;
    positive = 0;
    to[0] = from[0] + radius;
    to[1] = from[1];
    to[2] = from[2];
    hit = CheckHit(polys, count, from, to, positive_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && normal[1] > -0.5f) {
            positive = 1;
            sides |= CHECK_WIDTH_SIDE_POS_X;
        }
    }
    to[0] = from[0] - radius;
    to[1] = from[1];
    to[2] = from[2];
    hit = CheckHit(polys, count, from, to, negative_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_X;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[0] = 0.5f * (positive_hit[0] + negative_hit[0]);
    } else {
        if (positive != 0) {
            out_pos[0] = positive_hit[0] - radius;
        }
        if (negative != 0) {
            out_pos[0] = negative_hit[0] + radius;
        }
    }
    sceVu0CopyVector(from, out_pos);
    negative = 0;
    positive = 0;
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2] + radius;
    hit = CheckHit(polys, count, from, to, positive_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_POS_Z;
            positive = 1;
        }
    }
    to[0] = from[0];
    to[1] = from[1];
    to[2] = from[2] - radius;
    hit = CheckHit(polys, count, from, to, negative_hit, 1, ignore_mask);
    if (hit >= 0) {
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_Z;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[2] = 0.5f * (positive_hit[2] + negative_hit[2]);
    } else {
        if (positive != 0) {
            out_pos[2] = positive_hit[2] - radius;
        }
        if (negative != 0) {
            out_pos[2] = negative_hit[2] + radius;
        }
    }
    return sides;
}

int CheckWidthPipe(CCPoly *polys, int count, float *pos, float radius, float *out_pos, int ignore_mask) {
    sceVu0FVECTOR to;
    sceVu0FVECTOR positive_hit;
    sceVu0FVECTOR negative_hit;
    sceVu0FVECTOR from;
    sceVu0FVECTOR normal;
    int hit_polys[32];
    sceVu0FVECTOR hit_points[32];
    int sides;
    int positive;
    int negative;
    float probe_radius;
    int hit;

    probe_radius = radius;
    probe_radius *= 0.8f;
    sides = 0;
    sceVu0CopyVector(from, pos);
    from[1] += 3.0f * pos[3];
    sceVu0CopyVector(out_pos, pos);
    to[3] = from[3];
    negative = 0;
    positive = 0;
    to[1] = from[1];
    to[0] = from[0] + probe_radius;
    to[2] = from[2];
    if (CheckHitsPipe(polys, count, from, to, 0x20, hit_polys, hit_points, 1, ignore_mask) > 0) {
        hit = hit_polys[0];
        *(u_long128 *)positive_hit = *(u_long128 *)hit_points[0];
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            positive = 1;
            sides |= CHECK_WIDTH_SIDE_POS_X;
        }
    }
    to[0] = from[0] - probe_radius;
    to[2] = from[2];
    if (CheckHitsPipe(polys, count, from, to, 0x20, hit_polys, hit_points, 1, ignore_mask) > 0) {
        hit = hit_polys[0];
        *(u_long128 *)negative_hit = *(u_long128 *)hit_points[0];
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_X;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[0] = 0.5f * (positive_hit[0] + negative_hit[0]);
    } else {
        if (positive != 0) {
            out_pos[0] = positive_hit[0] - probe_radius;
        }
        if (negative != 0) {
            out_pos[0] = negative_hit[0] + probe_radius;
        }
    }
    negative = 0;
    positive = 0;
    from[0] = out_pos[0];
    from[2] = out_pos[2];
    to[0] = from[0];
    to[2] = from[2] + probe_radius;
    if (CheckHitsPipe(polys, count, from, to, 0x20, hit_polys, hit_points, 1, ignore_mask) > 0) {
        hit = hit_polys[0];
        *(u_long128 *)positive_hit = *(u_long128 *)hit_points[0];
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_POS_Z;
            positive = 1;
        }
    }
    to[0] = from[0];
    to[2] = from[2] - probe_radius;
    if (CheckHitsPipe(polys, count, from, to, 0x20, hit_polys, hit_points, 1, ignore_mask) > 0) {
        hit = hit_polys[0];
        *(u_long128 *)negative_hit = *(u_long128 *)hit_points[0];
        sceVu0Normalize(normal, polys[hit].normal);
        if (normal[1] < 0.5f && !(normal[1] <= -0.5f)) {
            sides |= CHECK_WIDTH_SIDE_NEG_Z;
            negative = 1;
        }
    }
    if (positive != 0 && negative != 0) {
        out_pos[2] = 0.5f * (positive_hit[2] + negative_hit[2]);
    } else {
        if (positive != 0) {
            out_pos[2] = positive_hit[2] - probe_radius;
        }
        if (negative != 0) {
            out_pos[2] = negative_hit[2] + probe_radius;
        }
    }
    return sides;
}

int CreateCharaCPoly(CCPoly *polys, int max_polys, float *pos, float *target, float distance, float half_size) {
    sceVu0FVECTOR direction;
    sceVu0FVECTOR centre;
    sceVu0FVECTOR upper_left;
    sceVu0FVECTOR upper_right;
    sceVu0FVECTOR lower_left;
    sceVu0FVECTOR lower_right;
    float         wall_distance;

    if (max_polys < 2) {
        return 0;
    }
    sceVu0SubVector(direction, target, pos);
    direction[1] = 0.0f;
    wall_distance = mgDistVector(direction);
    if (wall_distance > distance) {
        wall_distance = distance;
    }
    if (wall_distance < distance) {
        wall_distance -= 1.0f;
    }
    sceVu0Normalize(direction, direction);
    upper_left[1] = half_size;
    upper_left[0] = -direction[2] * half_size;
    upper_left[2] = direction[0] * half_size;
    upper_left[3] = 1.0f;
    sceVu0CopyVector(upper_right, upper_left);
    upper_right[0] = -upper_left[0];
    upper_right[2] = -upper_left[2];
    sceVu0CopyVector(lower_left, upper_left);
    lower_left[1] = -half_size;
    sceVu0CopyVector(lower_right, upper_right);
    lower_right[1] = -half_size;
    sceVu0ScaleVector(centre, direction, wall_distance);
    sceVu0AddVector(centre, centre, pos);
    sceVu0AddVector(upper_left, upper_left, centre);
    sceVu0AddVector(upper_right, upper_right, centre);
    sceVu0AddVector(lower_left, lower_left, centre);
    sceVu0AddVector(lower_right, lower_right, centre);
    *(u_long128 *)&polys[0].ground_kind = 0;
    sceVu0CopyVector(polys[0].vertex[0], upper_left);
    sceVu0CopyVector(polys[0].vertex[1], upper_right);
    sceVu0CopyVector(polys[0].vertex[2], lower_left);
    sceVu0CopyVector(polys[0].normal, direction);
    *(u_long128 *)&polys[1].ground_kind = 0;
    sceVu0CopyVector(polys[1].vertex[0], lower_left);
    sceVu0CopyVector(polys[1].vertex[1], upper_right);
    sceVu0CopyVector(polys[1].vertex[2], lower_right);
    sceVu0CopyVector(polys[1].normal, direction);
    return 2;
}

float LinerInterpolation(float from, float to, float rate) {
    return from + (rate * (to - from));
}

int LinerInterpolationI(int from, int to, int step, int steps) {
    return from + step * (to - from) / steps;
}

void RollPos(float *centre, float *pos, float angle, float *out) {
    float x;
    float centre_x = centre[0];
    float centre_y = centre[1];
    x = pos[0];
    float y = pos[1];
    float component;
    float relative_x;
    component = (relative_x = x - centre_x) * cosf(angle);
    out[0] = centre_x + (component - (y -= centre_y) * sinf(angle));
    component = relative_x * sinf(angle);
    out[1] = centre_y - (component + y * cosf(angle));
}

s32 CheckPosInOutForRect(RECT *rect, s32 x, s32 y) {
    s32 left = rect->x;
    if (x < left) {
        return 0;
    }
    if ((left + rect->width) < x) {
        return 0;
    }
    s32 top = rect->y;
    if (y < top) {
        return 0;
    }
    return (top + rect->height) >= y;
}

float GetDisPosToRect(RECT *rect, int x, int y) {
    float distance_x;
    float distance_y;

    distance_x = rect->x + rect->width / 2 - x;
    distance_y = rect->y + rect->height / 2 - y;
    return sqrt(distance_x * distance_x + distance_y * distance_y);
}

s32 CheckPosInOutFor2P(float x0, float y0, float x1, float y1, float x, float y) {
    float min_x;
    float max_x;
    float max_y;
    float min_y;
    s32 outside;

    max_x = x1;
    max_y = y1;
    min_x = max_x;
    if (x0 < max_x) {
        min_x = x0;
    } else {
        max_x = x0;
    }
    min_y = max_y;
    if (y0 < max_y) {
        min_y = y0;
    } else {
        max_y = y0;
    }
    if (x < min_x) {
        return 0;
    }
    if (max_x < x) {
        return 0;
    }
    if (y < min_y) {
        return 0;
    }
    outside = 1;
    if (!(max_y < y)) {
        outside = 0;
    }
    return outside ^ 1;
}
#ifdef NONMATCHING
int CalcIntersectionPointLineAndLine(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1, float *out_x, float *out_y) {
    float slope_a;
    float slope_b;
    float relative_x;

    if (ax0 == ax1 && bx0 == bx1) {
        return 0;
    }
    if (ax0 != ax1) {
        slope_a = (ay1 - ay0) / (ax1 - ax0);
    }
    if (bx0 != bx1) {
        slope_b = (by1 - by0) / (bx1 - bx0);
    }
    if (slope_a == slope_b) {
        return 0;
    }
    if (ax0 == ax1) {
        *out_x = ax0;
        *out_y = slope_b * (ax0 - bx0);
        return 1;
    } else if (bx0 == bx1) {
        *out_x = bx0;
        *out_y = slope_a * (bx0 - ax0);
        return 1;
    } else {
        relative_x = ((by0 - ay0) - slope_b * (bx0 - ax0)) / (slope_a - slope_b);
        *out_x = relative_x + ax0;
        *out_y = (relative_x * slope_a) + ay0;
        return 1;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gameutil", CalcIntersectionPointLineAndLine__FffffffffPfPf);
#endif

s32 CalcIntersectionPoint2PAnd2P(float ax0, float ay0, float ax1, float ay1, float bx0, float by0, float bx1, float by1, float *out_x, float *out_y) {
    if (CalcIntersectionPointLineAndLine(ax0, ay0, ax1, ay1, bx0, by0, bx1, by1, out_x, out_y) == 0) {
        return 0;
    }
    if (CheckPosInOutFor2P(ax0, ay0, ax1, ay1, *out_x, *out_y) == 0) {
        return 0;
    }
    return CheckPosInOutFor2P(bx0, by0, bx1, by1, *out_x, *out_y) != 0;
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gameutil", at_966__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gameutil", at_967__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(vert_845, 0x10);
INCLUDE_BSS(vert_915, 0x10);
INCLUDE_BSS(nml_916, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(tmp_SkinMatrix_847, 0x40);
INCLUDE_BSS(tmp_SkinMatrix_inv_848, 0x40);
INCLUDE_BSS(tmp_ChrMatrix_849, 0x40);
INCLUDE_BSS(tmp_BaseSkinMatrix_851, 0x40);
INCLUDE_BSS(tmp_BaseSkinMatrix_inv_852, 0x40);
INCLUDE_BSS(tmp_SkinMatrix_917, 0x40);
INCLUDE_BSS(tmp_SkinMatrix_inv_918, 0x40);
INCLUDE_BSS(tmp_ChrMatrix_919, 0x40);
INCLUDE_BSS(tmp_BaseSkinMatrix_921, 0x40);
INCLUDE_BSS(tmp_BaseSkinMatrix_inv_922, 0x40);
INCLUDE_BSS(at_945, 0x10);
