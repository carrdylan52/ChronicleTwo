#include "common.h"
#include "dynamicanime.hpp"
#include <cstdio>

#include <cstring>

#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "scriptinterpreter.hpp"

static CDynamicAnime *dynNowDA; /**< Animation receiving the script tags. */
static mgCMemory     *dynStack; /**< Storage used by the script tables. */
static mgCFrame      *dynTopFrame; /**< Root frame used to resolve script frame names. */
static int           dynFrameCount; /**< Next frame table entry to fill. */
static int           dynVertexCount; /**< Next simulated vertex to load. */
static int           dynFixVertexCount; /**< Fixed vertex script counter. */
static int           dynBindVertexCount; /**< Next bind vertex entry to fill. */
static int           dynBBoxCount; /**< Next bounding box entry to fill. */
static int           dynColCount; /**< Next collision volume entry to fill. */

/**
 * Returns the number of quadwords needed to hold a byte count.
 */
static inline u_int align16_blocks(u_int size) {
    if (size & 15) {
        return (size >> 4) + 1;
    }
    return size >> 4;
}

static int dynCOLLISION(SPI_STACK *stack, int count);

// Code (.text)
/**
 * Moves two vertices towards their prescribed separation, sharing the correction by rate.
 */
static void BindPosition(float *a, float *b, float length, float rate) {
    sceVu0FVECTOR  difference;
    sceVu0FVECTOR  correction_a;
    sceVu0FVECTOR  correction_b;
    float          distance;
    float          error;

    sceVu0SubVector(difference, a, b);
    distance = mgDistVector(difference);
    error = distance - length;
    sceVu0ScaleVector(correction_a, difference, ((1.0f - rate) * error) / distance);
    sceVu0ScaleVector(correction_b, difference, (rate * error) / distance);
    mgSubVector(a, correction_a);
    mgAddVector(b, correction_b);
}

void CDynamicAnime::ResetPosition() {
    sceVu0FMATRIX  matrix;
    int            i;

    if (top_frame != NULL) {
        top_frame->GetLWMatrix(matrix);
        mgApplyMatrixN(now_vertex, matrix, init_vertex, vertex_num);
    }
    for (i = 0; i < vertex_num; i++) {
        mgZeroVector(velocity[i]);
        *(u_long128 *)old_vertex[i] = *(u_long128 *)now_vertex[i];
    }
}

#ifdef NONMATCHING
void CDynamicAnime::Step() {
    sceVu0FMATRIX   matrix;
    sceVu0FVECTOR   pull;
    sceVu0FVECTOR   max;
    sceVu0FVECTOR   min;
    sceVu0FVECTOR   wind;
    DA_FIX_VERTEX  *fixed;
    DA_BIND_VERTEX *bound;
    CDACollision   *volume;
    mgCFrame       *fixed_frame;
    float           stiffness;
    float           friction;
    int             hit;
    int             i;
    int             j;
    int             iteration;

    if (vertex_num <= 0) {
        return;
    }
    stiffness = k;
    if (top_frame != NULL) {
        top_frame->GetLWMatrix(matrix);
    } else {
        stiffness = 0.0f;
    }
    if (stiffness > 0.0f) {
        mgApplyMatrixN(world_init_vertex, matrix, init_vertex, vertex_num);
    }
    for (i = 0; i < vertex_num; i++) {
        mgAddVector(velocity[i], gravity);
        velocity[i][3] = 0.0f;
        mgAddVector(now_vertex[i], velocity[i]);
    }
    for (iteration = 0; iteration < 6; iteration++) {
        for (i = 0; i < bind_vertex_num; i++) {
            bound = &bind_vertex[i];
            BindPosition(now_vertex[bound->vertex_id[0]], now_vertex[bound->vertex_id[1]], bound->length, bound->rate);
        }
        for (i = 0; i < vertex_num; i++) {
            fixed = &fix_vertex[i];
            if (fixed->weight >= 1.0f) {
                fixed_frame = GetFrame(fixed->frame_id);
                if (fixed_frame == NULL) {
                    return;
                }
                fixed_frame->GetWorldPosition(now_vertex[i], fixed->position);
            }
        }
    }
    sceVu0CopyVector(max, now_vertex[0]);
    sceVu0CopyVector(min, now_vertex[0]);
    PreCollision();
    for (i = 0; i < vertex_num; i++) {
        sceVu0SubVector(velocity[i], now_vertex[i], old_vertex[i]);
        *(u_long128 *)old_vertex[i] = *(u_long128 *)now_vertex[i];
        fixed = &fix_vertex[i];
        if (fixed->weight < 1.0f && fixed->weight > 0.0f) {
            fixed_frame = GetFrame(fixed->frame_id);
            if (fixed_frame != NULL) {
                fixed_frame->GetWorldPosition(pull, fixed->position);
                mgSubVector(pull, now_vertex[i]);
                sceVu0ScaleVector(pull, pull, fixed->weight);
                mgAddVector(now_vertex[i], pull);
                sceVu0ScaleVector(pull, pull, fixed->velocity_rate);
                mgSubVector(velocity[i], pull);
            }
        }
        friction = 1.0f;
        hit = 0;
        if (fixed->weight < 1.0f) {
            for (j = 0; j < collision_num; j++) {
                volume = collision[j];
                if (volume != NULL) {
                    hit |= volume->CheckHit(now_vertex[i]);
                    if (friction > volume->friction) {
                        friction = volume->friction;
                    }
                }
            }
            if (hit != 0) {
                sceVu0ScaleVector(velocity[i], velocity[i], friction);
            }
        }
        if (floor_enable != 0) {
            if (now_vertex[i][1] < floor_y) {
                now_vertex[i][1] = floor_y;
                sceVu0ScaleVector(velocity[i], velocity[i], 0.3f);
            }
        }
        if (wind_power != 0.0f) {
            wind_seed = wind_seed * 0x10DCD + 1;
            wind_gust += 0.5f * ((float)wind_seed / -2147483648.0f - 0.5f);
            if (wind_gust > 1.0f) {
                wind_gust = 1.0f;
            }
            if (wind_gust < 0.0f) {
                wind_gust = 0.0f;
            }
            sceVu0ScaleVector(wind, wind_dir, wind_scale * (wind_power * wind_gust));
            mgAddVector(velocity[i], wind);
        }
        mgVectorMaxMin(max, min, max, min, now_vertex[i]);
    }
    for (i = 0; i < frame_num; i++) {
        FramePose(frame[i], &frame_pose[i]);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dynamicanime", Step__13CDynamicAnimeFv);
#endif

int CDACollision::CheckHit(float *position) { return 0; }
void CDynamicAnime::SetWind(float power, float *direction) {
    wind_power = power;
    sceVu0Normalize(wind_dir, direction);
}
void CDynamicAnime::ResetWind(void) {
    wind_power = 0.0f;
}
void CDynamicAnime::SetFloor(float height) {
    floor_enable = 1;
    floor_y = height;
}
void CDynamicAnime::ResetFloor(void) {
    floor_enable = 0;
}
#ifdef NONMATCHING
void CDynamicAnime::FramePose(mgCFrame *frame, DA_FRAME_POSE *pose) {
    sceVu0FMATRIX  matrix;
    sceVu0FMATRIX  bone_parent_matrix;
    sceVu0FMATRIX  corner_parent_matrix;
    sceVu0FVECTOR  origin;
    sceVu0FVECTOR  end;
    sceVu0FVECTOR  across;
    sceVu0FVECTOR  along;
    float         *v0;
    float         *v1;
    float         *v2;
    float         *v3;
    int            cross_axis;
    int            along_axis;
    int            across_axis;
    int            first_axis;
    int            second_axis;

    if (frame == NULL) {
        return;
    }
    across_axis = 0;
    cross_axis = 1;
    along_axis = 2;
    first_axis = 2;
    second_axis = 0;
    switch (pose->type) {
    case DA_FRAME_POSE_BONE_YX:
        cross_axis = 2;
        first_axis = 0;
        along_axis = 1;
        second_axis = 1;
        // The same construction with the long axis along y.
    case DA_FRAME_POSE_BONE:
        v0 = now_vertex[pose->vertex_id[0]];
        v1 = now_vertex[pose->vertex_id[1]];
        v2 = now_vertex[pose->vertex_id[2]];
        v3 = now_vertex[pose->vertex_id[3]];
        sceVu0AddVector(origin, v0, v1);
        sceVu0ScaleVector(origin, origin, 0.5f);
        sceVu0AddVector(end, v2, v3);
        sceVu0ScaleVector(end, end, 0.5f);
        sceVu0SubVector(along, v1, v0);
        sceVu0Normalize(matrix[along_axis], along);
        matrix[along_axis][3] = 0.0f;
        sceVu0SubVector(across, end, origin);
        sceVu0Normalize(matrix[across_axis], across);
        matrix[across_axis][3] = 0.0f;
        sceVu0OuterProduct(matrix[cross_axis], matrix[first_axis], matrix[second_axis]);
        matrix[cross_axis][3] = 0.0f;
        sceVu0OuterProduct(matrix[first_axis], matrix[second_axis], matrix[cross_axis]);
        sceVu0Normalize(matrix[first_axis], matrix[first_axis]);
        sceVu0CopyVector(matrix[3], origin);
        matrix[3][3] = 1.0f;
        if (pose->local != 0 && frame->parent != NULL) {
            frame->parent->GetLWMatrix(bone_parent_matrix);
            mgInversMatrix(bone_parent_matrix, bone_parent_matrix);
            mgMulMatrix(matrix, bone_parent_matrix, matrix);
        }
        frame->SetTransMatrix(matrix);
        return;
    case DA_FRAME_POSE_B_CDLR:
        v0 = now_vertex[pose->vertex_id[0]];
        v1 = now_vertex[pose->vertex_id[1]];
        v2 = now_vertex[pose->vertex_id[2]];
        v3 = now_vertex[pose->vertex_id[3]];
        sceVu0SubVector(matrix[0], v1, v0);
        matrix[0][3] = 0.0f;
        sceVu0Normalize(matrix[0], matrix[0]);
        sceVu0SubVector(along, v2, v3);
        along[3] = 0.0f;
        sceVu0Normalize(matrix[2], along);
        sceVu0OuterProduct(matrix[1], matrix[2], matrix[0]);
        matrix[1][3] = 0.0f;
        sceVu0CopyVector(matrix[3], v0);
        matrix[3][3] = 1.0f;
        if (pose->local != 0 && frame->parent != NULL) {
            frame->parent->GetLWMatrix(corner_parent_matrix);
            mgInversMatrix(corner_parent_matrix, corner_parent_matrix);
            mgMulMatrix(matrix, corner_parent_matrix, matrix);
        }
        frame->SetTransMatrix(matrix);
        break;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dynamicanime", FramePose__13CDynamicAnimeFP8mgCFrameP13DA_FRAME_POSE);
#endif

void CDynamicAnime::PreCollision() {
    int i;
    CDACollision *volume;

    for (i = 0; i < collision_num; i++) {
        volume = collision[i];
        if (volume != NULL) {
            volume->frame = GetFrame(volume->frame_id);
            if (volume->frame != NULL) {
                volume->frame->GetLWMatrix(volume->lw_matrix);
                mgInversMatrix(volume->inverse_matrix, volume->lw_matrix);
            }
        }
    }
}

void CDynamicAnime::Initialize() {
    top_frame = NULL;
    frame_num = 0;
    frame = NULL;
    frame_pose = NULL;
    vertex_num = 0;
    init_vertex = NULL;
    now_vertex = NULL;
    old_vertex = NULL;
    velocity = NULL;
    fix_vertex_num = 0;
    fix_vertex = NULL;
    draw_frame_num = 0;
    draw_frame = NULL;
    bind_vertex_num = 0;
    bind_vertex = NULL;
    bbox_num = 0;
    bbox = NULL;
    collision_num = 0;
    collision = NULL;
    mgZeroVector(gravity);
    gravity[1] = -0.6f;
    k = 0.0f;
    wind_power = 0.0f;
    mgZeroVector(wind_dir);
    wind_seed = 0x1E69D;
    wind_gust = 0.0f;
    wind_scale = 1.0f;
    floor_enable = 0;
    floor_y = -100000.0f;
}

void CDynamicAnime::NewFrameTable(int num, mgCMemory *stack) {
    int    i;

    frame_num = num;
    frame = (mgCFrame **)stack->Alloc(align16_blocks(frame_num * sizeof(mgCFrame *)));
    frame_pose = (DA_FRAME_POSE *)stack->Alloc(align16_blocks(frame_num * sizeof(DA_FRAME_POSE)));
    for ( i = 0; i < frame_num; i++) {
        frame[i] = NULL;
        memset(&frame_pose[i], 0, sizeof(DA_FRAME_POSE));
    }
}

void CDynamicAnime::NewVertexTable(int num, mgCMemory *stack) {
    int    i;

    vertex_num = num;
    init_vertex = (sceVu0FVECTOR *)stack->Alloc(align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)));
    now_vertex = (sceVu0FVECTOR *)stack->Alloc(align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)));
    old_vertex = (sceVu0FVECTOR *)stack->Alloc(align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)));
    velocity = (sceVu0FVECTOR *)stack->Alloc(align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)));
    world_init_vertex = (sceVu0FVECTOR *)stack->Alloc(align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)));
    for ( i = 0; i < vertex_num; i++) {
        mgZeroVector(init_vertex[i]);
        mgZeroVector(now_vertex[i]);
        mgZeroVector(old_vertex[i]);
        mgZeroVector(velocity[i]);
    }
}

void CDynamicAnime::NewFixVertexTable(int num, mgCMemory *stack) {
    int    i;

    fix_vertex_num = num;
    fix_vertex = (DA_FIX_VERTEX *)stack->Alloc(
        align16_blocks(fix_vertex_num * sizeof(DA_FIX_VERTEX)));
    for ( i = 0; i < vertex_num; i++) {
        memset(&fix_vertex[i], 0, sizeof(DA_FIX_VERTEX));
    }
}

void CDynamicAnime::NewDrawFrameTable(int num, mgCMemory *stack) {
    int    i;

    draw_frame_num = num;
    draw_frame = (int *)stack->Alloc(align16_blocks(draw_frame_num * sizeof(int)));
    for ( i = 0; i < draw_frame_num; i++) {
        draw_frame[i] = -1;
    }
}

void CDynamicAnime::NewBindVertexTable(int num, mgCMemory *stack) {
    int    i;

    bind_vertex_num = num;
    bind_vertex = (DA_BIND_VERTEX *)stack->Alloc(
        align16_blocks(bind_vertex_num * sizeof(DA_BIND_VERTEX)));
    for ( i = 0; i < bind_vertex_num; i++) {
        memset(&bind_vertex[i], 0, sizeof(DA_BIND_VERTEX));
    }
}

void CDynamicAnime::NewBoundingBoxTable(int num, mgCMemory *stack) {
    int    i;

    bbox_num = num;
    bbox = (DA_BOUNDING_BOX *)stack->Alloc(
        align16_blocks(bbox_num * sizeof(DA_BOUNDING_BOX)));
    for ( i = 0; i < bind_vertex_num; i++) {
        memset(&bbox[i], 0, sizeof(DA_BOUNDING_BOX));
        bbox[i].frame_id = -1;
    }
}

void CDynamicAnime::NewCollisionTable(int num, mgCMemory *stack) {
    int    i;

    collision_num = num;
    collision = (CDACollision **)stack->Alloc(
        align16_blocks(collision_num * sizeof(CDACollision *)));
    for ( i = 0; i < collision_num; i++) {
        collision[i] = NULL;
    }
}

void CDynamicAnime::SetFrame(int index, mgCFrame *frame) {
    if (index < 0 || index >= frame_num) {
        return;
    }
    this->frame[index] = frame;
}

mgCFrame *CDynamicAnime::GetFrame(int index) {
    if (index < 0 || index >= frame_num) {
        return NULL;
    }
    return frame[index];
}

DA_FRAME_POSE *CDynamicAnime::pGetFramePose(int index) {
    if (index < 0 || index >= frame_num) {
        return NULL;
    }
    return &frame_pose[index];
}

int CDynamicAnime::CheckVertexID(int index) {
    if (index < 0 || index >= vertex_num) {
        return 0;
    }
    return 1;
}

void CDynamicAnime::SetInitVertex(int index, float *position) {
    if (CheckVertexID(index) != 0) {
        *(u_long128 *)init_vertex[index] = *(u_long128 *)position;
    }
}

void CDynamicAnime::GetInitVertex(int index, float *out_position) {
    if (CheckVertexID(index) != 0) {
        *(u_long128 *)out_position = *(u_long128 *)init_vertex[index];
    }
}

void CDynamicAnime::SetNowVertex(int index, float *position) {
    if (CheckVertexID(index) != 0) {
        *(u_long128 *)now_vertex[index] = *(u_long128 *)position;
    }
}

void CDynamicAnime::SetOldVertex(int index, float *position) {
    if (CheckVertexID(index) != 0) {
        *(u_long128 *)old_vertex[index] = *(u_long128 *)position;
    }
}

DA_FIX_VERTEX *CDynamicAnime::pGetFixVertex(int index) {
    if (index < 0 || index >= fix_vertex_num) {
        return NULL;
    }
    return &fix_vertex[index];
}

void CDynamicAnime::SetDrawFrame(int index, int frame_id) {
    if (index < 0 || index >= draw_frame_num) {
        return;
    }
    this->draw_frame[index] = frame_id;
}

mgCFrame *CDynamicAnime::GetDrawFrame(int index) {
    if (index < 0 || index >= draw_frame_num) {
        return NULL;
    }
    return GetFrame(draw_frame[index]);
}

DA_BIND_VERTEX *CDynamicAnime::pGetBindVertex(int index) {
    if (index < 0 || index >= bind_vertex_num) {
        return NULL;
    }
    return &bind_vertex[index];
}

DA_BOUNDING_BOX *CDynamicAnime::pGetBoundingBox(int index) {
    if (index < 0 || index >= bbox_num) {
        return NULL;
    }
    return &bbox[index];
}

void CDynamicAnime::SetCollision(int index, CDACollision *collision) {
    if (index < 0 || index >= collision_num) {
        return;
    }
    this->collision[index] = collision;
}

int CDynamicAnime::DrawSub(int direct) {
    int  total;
    int  i;

    total = 0;
    for (i = 0; i < draw_frame_num; i++) {
        if (direct != 0) {
            total += mgDrawDirect(GetDrawFrame(i));
        } else {
            total += mgDraw(GetDrawFrame(i));
        }
    }
    return total;
}

void CDynamicAnime::Copy(CDynamicAnime &dest, mgCFrame *root, mgCMemory *stack) {
    int i;
    dest = *this;
    dest.top_frame = root;
    if (root == NULL) {
        return;
    }
    if (frame_num > 0 && frame != NULL) {
        dest.frame = new ((u_long128 *)stack->Alloc(
            align16_blocks(frame_num * sizeof(mgCFrame *)) + 2)) mgCFrame *[frame_num];
        if (dest.frame == NULL) {
            return;
        }
        for (i = 0; i < frame_num; i++) {
            dest.frame[i] = NULL;
            if (frame[i] != NULL) {
                dest.frame[i] = root->GetFrame(root->SearchFrameID(frame[i]->name));
            }
        }
    }
    if (vertex_num > 0) {
        dest.init_vertex = new ((u_long128 *)stack->Alloc(
            align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)) + 2)) sceVu0FVECTOR[vertex_num];
        dest.now_vertex = new ((u_long128 *)stack->Alloc(
            align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)) + 2)) sceVu0FVECTOR[vertex_num];
        dest.old_vertex = new ((u_long128 *)stack->Alloc(
            align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)) + 2)) sceVu0FVECTOR[vertex_num];
        dest.velocity = new ((u_long128 *)stack->Alloc(
            align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)) + 2)) sceVu0FVECTOR[vertex_num];
        dest.world_init_vertex = new ((u_long128 *)stack->Alloc(
            align16_blocks(vertex_num * sizeof(sceVu0FVECTOR)) + 2)) sceVu0FVECTOR[vertex_num];
        for (i = 0; i < vertex_num; i++) {
            *(u_long128 *)dest.init_vertex[i] = *(u_long128 *)init_vertex[i];
            *(u_long128 *)dest.now_vertex[i] = *(u_long128 *)now_vertex[i];
            *(u_long128 *)dest.old_vertex[i] = *(u_long128 *)old_vertex[i];
            *(u_long128 *)dest.velocity[i] = *(u_long128 *)velocity[i];
            *(u_long128 *)dest.world_init_vertex[i] = *(u_long128 *)world_init_vertex[i];
        }
    }
}

/**
 * Allocates the script frame and pose tables.
 */
static int dynFRAME_START(SPI_STACK *stack, int count) {
    dynNowDA->NewFrameTable(spiGetStackInt(stack), dynStack);
    return 1;
}

/**
 * Adds the named model frame to the animation frame table.
 */
static int dynFRAME(SPI_STACK *stack, int count) {
    char     *name;
    mgCFrame *frame;

    name = spiGetStackString(stack);
    if (name == NULL) {
        return 0;
    }
    frame = dynTopFrame->SearchFrame(name);
    if (frame == NULL) {
        printf("not found %s\n", name);
    }
    dynNowDA->SetFrame(dynFrameCount++, frame);
    return 1;
}

/**
 * Finishes the frame table.
 */
static int dynFRAME_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Allocates the simulated vertex tables.
 */
static int dynVERTEX_START(SPI_STACK *stack, int count) {
    dynNowDA->NewVertexTable(spiGetStackInt(stack), dynStack);
    return 1;
}

/**
 * Loads a vertex offset from a frame world position.
 */
static int dynVERTEX(SPI_STACK *stack, int count) {
    sceVu0FVECTOR  offset;
    sceVu0FVECTOR  position;
    mgCFrame      *frame;
    int            frame_id;

    frame_id = spiGetStackInt(stack++);
    spiGetStackVector(offset, stack);
    frame = dynNowDA->GetFrame(frame_id);
    if (frame != NULL) {
        frame->GetWorldPosition0(position);
        position[3] = 1.0f;
        position[0] += offset[0];
        position[1] += offset[1];
        position[2] += offset[2];
        dynNowDA->SetInitVertex(dynVertexCount++, position);
    }
    return 1;
}

/**
 * Loads a vertex given in the space of a frame.
 */
static int dynVERTEX_L(SPI_STACK *stack, int count) {
    sceVu0FVECTOR  local;
    sceVu0FVECTOR  position;
    mgCFrame      *frame;
    int            frame_id;

    frame_id = spiGetStackInt(stack++);
    spiGetStackVector(local, stack);
    frame = dynNowDA->GetFrame(frame_id);
    if (frame != NULL) {
        local[3] = 1.0f;
        frame->GetWorldPosition(position, local);
        position[3] = 1.0f;
        dynNowDA->SetInitVertex(dynVertexCount++, position);
    }
    return 1;
}

/**
 * Starts the current and previous positions at the loaded vertices.
 */
static int dynVERTEX_END(SPI_STACK *stack, int count) {
    sceVu0FVECTOR  position;
    int            vertex_num;
    int            i;

    count = dynNowDA->vertex_num;
    for (i = 0; i < count; i++) {
        dynNowDA->GetInitVertex(i, position);
        dynNowDA->SetOldVertex(i, position);
        dynNowDA->SetNowVertex(i, position);
    }
    return 1;
}

/**
 * Allocates a fix record for every simulated vertex.
 */
static int dynFIX_VERTEX_START(SPI_STACK *stack, int count) {
    spiGetStackInt(stack);
    dynNowDA->NewFixVertexTable(dynNowDA->vertex_num, dynStack);
    return 1;
}

/**
 * Builds the frame-local attachment point for a fixed vertex.
 */
static DA_FIX_VERTEX *dynFixVertex(SPI_STACK *stack, int count) {
    int frame_id;
    int vertex_id;
    DA_FIX_VERTEX *fixed;
    mgCFrame *frame;
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR init;

    frame_id = spiGetStackInt(stack++);
    vertex_id = spiGetStackInt(stack);
    fixed = dynNowDA->pGetFixVertex(vertex_id);

    fixed->frame_id = frame_id;
    if (fixed == NULL) {
        return NULL;
    }
    frame = dynNowDA->GetFrame(fixed->frame_id);
    if (frame == NULL || dynNowDA->CheckVertexID(vertex_id) == 0) {
        fixed->frame_id = -1;
        return NULL;
    }
    dynNowDA->GetInitVertex(vertex_id, init);
    init[3] = 1.0f;
    frame->GetInverseMatrix(matrix);
    sceVu0ApplyMatrix(fixed->position, matrix, init);
    fixed->position[3] = 1.0f;
    fixed->weight = 1.0f;
    fixed->velocity_rate = 0.0f;
    fixed->unk_1c = 1.0f;
    return fixed;
}

/**
 * Sets the attachment weight and response of a fixed vertex.
 */
static int dynFIX_VERTEX(SPI_STACK *stack, int count) {
    DA_FIX_VERTEX *fixed;
    SPI_STACK     *parameter;

    fixed = dynFixVertex(stack, count);
    stack += 2;
    if (fixed == NULL) {
        return 0;
    }
    if (count >= 3) {
        fixed->weight = spiGetStackFloat(stack++);
    }
    if (count >= 4) {
        fixed->unk_1c = spiGetStackFloat(stack);
    }
    fixed->velocity_rate = 0.0f;
    return 1;
}

/**
 * Sets the attachment weight and response of a fixed vertex.
 */
static int dynFIX_VERTEX_C(SPI_STACK *stack, int count) {
    DA_FIX_VERTEX *fixed;
    SPI_STACK     *parameter;

    fixed = dynFixVertex(stack, count);
    stack += 2;
    if (fixed == NULL) {
        return 0;
    }
    if (count >= 3) {
        fixed->weight = spiGetStackFloat(stack++);
    }
    if (count >= 4) {
        fixed->unk_1c = spiGetStackFloat(stack);
    }
    fixed->velocity_rate = 1.0f;
    return 1;
}

/**
 * Sets the attachment weight and response of a fixed vertex.
 */
static int dynFIX_VERTEX_S(SPI_STACK *stack, int count) {
    DA_FIX_VERTEX *fixed;
    SPI_STACK     *parameter;

    fixed = dynFixVertex(stack, count);
    stack += 2;
    if (fixed == NULL) {
        return 0;
    }
    if (count >= 3) {
        fixed->weight = spiGetStackFloat(stack++);
    }
    if (count >= 4) {
        fixed->unk_1c = spiGetStackFloat(stack);
    }
    fixed->velocity_rate = -1.0f;
    return 1;
}

/**
 * Finishes the fixed vertex table.
 */
static int dynFIX_VERTEX_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Reads a frame pose kind and the four vertices that determine its transform.
 */
static DA_FRAME_POSE *FRAME_POSE_Sub(SPI_STACK *stack, int count) {
    DA_FRAME_POSE *pose;
    char          *kind;
    SPI_STACK     *vertex;
    int            i;

    pose = dynNowDA->pGetFramePose(spiGetStackInt(stack++));
    kind = spiGetStackString(stack++);
    if (pose == NULL || kind == NULL) {
        return NULL;
    }
    pose->type = DA_FRAME_POSE_NONE;
    if (strcmp(kind, "bone") == 0) {
        if (count < 6) {
            return NULL;
        }
        pose->type = DA_FRAME_POSE_BONE;
        pose->vertex_num = 4;
    } else if (strcmp(kind, "bone_yx") == 0) {
        if (count < 6) {
            return NULL;
        }
        pose->type = DA_FRAME_POSE_BONE_YX;
        pose->vertex_num = 4;
    } else if (strcmp(kind, "b_cdlr") == 0) {
        if (count < 6) {
            return NULL;
        }
        pose->type = DA_FRAME_POSE_B_CDLR;
        pose->vertex_num = 4;
    } else {
        return NULL;
    }
    pose->vertex_id = (int *)dynStack->Alloc(1);
    for (i = 0; i < pose->vertex_num; i++) {
        pose->vertex_id[i] = spiGetStackInt(stack++);
        if (dynNowDA->CheckVertexID(pose->vertex_id[i]) == 0) {
            printf("error vertex no %d!!\n", pose->vertex_id[i]);
            pose->type = DA_FRAME_POSE_NONE;
            return NULL;
        }
    }
    return pose;
}

/**
 * Sets whether the frame pose is relative to its parent.
 */
static int dynFRAME_POSE_L(SPI_STACK *stack, int count) {
    DA_FRAME_POSE *pose;
    mgCFrame      *frame;

    pose = FRAME_POSE_Sub(stack, count);
    frame = dynNowDA->GetFrame(spiGetStackInt(stack));
    if (pose == NULL || frame == NULL) {
        return 0;
    }
    pose->local = 1;
    return 1;
}

/**
 * Sets whether the frame pose is relative to its parent.
 */
static int dynFRAME_POSE(SPI_STACK *stack, int count) {
    DA_FRAME_POSE *pose;
    mgCFrame      *frame;

    pose = FRAME_POSE_Sub(stack, count);
    frame = dynNowDA->GetFrame(spiGetStackInt(stack));
    if (pose == NULL || frame == NULL) {
        return 0;
    }
    pose->local = 0;
    frame->DeleteParent();
    return 1;
}

/**
 * Reads the list of frames drawn by the animation.
 */
static int dynDRAW_FRAME(SPI_STACK *stack, int count) {
    int  i;

    dynNowDA->NewDrawFrameTable(count, dynStack);
    for (i = 0; i < count; i++) {
        dynNowDA->SetDrawFrame(i, spiGetStackInt(stack++));
    }
    return 1;
}

/**
 * Allocates the bindvertex table.
 */
static int dynBIND_VERTEX_START(SPI_STACK *stack, int count) {
    dynNowDA->NewBindVertexTable(spiGetStackInt(stack), dynStack);
    return 1;
}

/**
 * Binds a pair of vertices at their loaded separation.
 */
static int dynBIND_VERTEX(SPI_STACK *stack, int count) {
    DA_BIND_VERTEX *bound;
    int vertex_a;
    int vertex_b;
    sceVu0FVECTOR a;
    sceVu0FVECTOR b;
    float weight;

    bound = dynNowDA->pGetBindVertex(dynBindVertexCount++);
    if (bound == NULL) {
        return 0;
    }
    vertex_a = spiGetStackInt(stack++);
    vertex_b = spiGetStackInt(stack++);
    if (dynNowDA->CheckVertexID(vertex_a) == 0 || dynNowDA->CheckVertexID(vertex_b) == 0) {
        printf("error vertex no %d-%d!!!\n", vertex_a, vertex_b);
        return 0;
    }
    bound->rate = 0.5f;
    if (count >= 3) {
        weight = spiGetStackFloat(stack);
        bound->rate = weight;
        if (weight > 1.0f || weight < 0.0f) {
            bound->rate = 0.5f;
        }
    }
    bound->vertex_id[0] = vertex_a;
    bound->vertex_id[1] = vertex_b;
    dynNowDA->GetInitVertex(vertex_a, a);
    dynNowDA->GetInitVertex(vertex_b, b);
    bound->length = mgDistVector(a, b);
    return 1;
}

/**
 * Finishes the bindvertex table.
 */
static int dynBIND_VERTEX_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Allocates the boundingbox table.
 */
static int dynBOUNDING_BOX_START(SPI_STACK *stack, int count) {
    dynNowDA->NewBoundingBoxTable(spiGetStackInt(stack), dynStack);
    return 1;
}

/**
 * Reads a frame index and the vectors of a bounding box.
 */
static int dynBOUNDING_BOX(SPI_STACK *stack, int count) {
    DA_BOUNDING_BOX *box;
    SPI_STACK       *parameter;

    box = dynNowDA->pGetBoundingBox(dynBBoxCount++);
    if (box == NULL) {
        return 0;
    }
    box->frame_id = spiGetStackInt(stack++);
    if (count >= 4) {
        spiGetStackVector(box->unk_0, stack);
        stack += 3;
        *(u_long128 *)box->unk_10 = *(u_long128 *)box->unk_0;
    }
    if (count >= 7) {
        spiGetStackVector(box->unk_10, stack);
    }
    return 1;
}

/**
 * Finishes the boundingbox table.
 */
static int dynBOUNDING_BOX_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Allocates the collision table.
 */
static int dynCOLLISION_START(SPI_STACK *stack, int count) {
    dynNowDA->NewCollisionTable(spiGetStackInt(stack), dynStack);
    return 1;
}

#ifdef NONMATCHING
/**
 * Creates the pipe collision volume described by a script tag.
 */
static int dynCOLLISION(SPI_STACK *stack, int count) {
    char       *kind;
    CDAColPipe *pipe;

    kind = spiGetStackString(stack);
    if (kind == NULL) {
        return 0;
    }
    if (strcmp(kind, "pipe") == 0) {
        pipe = new (dynStack->Alloc(16)) CDAColPipe;
        if (pipe == NULL) {
            return 0;
        }
        pipe->frame_id = spiGetStackInt(stack + 1);
        spiGetStackVector(pipe->center, stack + 2);
        spiGetStackVector(pipe->radius, stack + 5);
        pipe->axis = spiGetStackInt(stack + 8);
        if (count >= 10) {
            pipe->friction = spiGetStackFloat(stack + 9);
        }
        dynNowDA->SetCollision(dynColCount++, pipe);
        return 1;
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dynamicanime", dynCOLLISION__FP9SPI_STACKi);
#endif

void CDAColPipe::Initialize() {
    axis = 0;
    mgZeroVector(center);
    mgZeroVector(radius);
    friction = 0.8f;
}

void CDACollision::Initialize() {
    mgZeroVector(center);
    mgZeroVector(radius);
    friction = 0.8f;
}

/**
 * Finishes the collision table.
 */
static int dynCOLLISION_END(SPI_STACK *stack, int count) {
    return 1;
}

/**
 * Sets the velocity added to the vertices each step.
 */
static int dynGRAVITY(SPI_STACK *stack, int count) {
    spiGetStackVector(dynNowDA->gravity, stack);
    dynNowDA->gravity[3] = 0.0f;
    return 1;
}

/**
 * Sets the stiffness parameter of the animation.
 */
static int dynK(SPI_STACK *stack, int count) {
    dynNowDA->k = spiGetStackFloat(stack);
    return 1;
}

/**
 * Sets the scale applied to the animation wind.
 */
static int dynWind(SPI_STACK *stack, int count) {
    dynNowDA->wind_scale = spiGetStackFloat(stack);
    return 1;
}

/**
 * Tags understood by the dynamic animation script loader.
 */
static SPI_TAG_PARAM dynmc_tag[] = {
    { "FRAME_START", dynFRAME_START },
    { "FRAME", dynFRAME },
    { "FRAME_END", dynFRAME_END },
    { "VERTEX_START", dynVERTEX_START },
    { "VERTEX", dynVERTEX },
    { "VERTEX_L", dynVERTEX_L },
    { "VERTEX_END", dynVERTEX_END },
    { "FIX_VERTEX_START", dynFIX_VERTEX_START },
    { "FIX_VERTEX", dynFIX_VERTEX },
    { "FIX_VERTEX_C", dynFIX_VERTEX_C },
    { "FIX_VERTEX_S", dynFIX_VERTEX_S },
    { "FIX_VERTEX_END", dynFIX_VERTEX_END },
    { "FRAME_POSE", dynFRAME_POSE },
    { "FRAME_POSE_L", dynFRAME_POSE_L },
    { "DRAW_FRAME", dynDRAW_FRAME },
    { "BIND_VERTEX_START", dynBIND_VERTEX_START },
    { "BIND_VERTEX", dynBIND_VERTEX },
    { "BIND_VERTEX_END", dynBIND_VERTEX_END },
    { "BOUNDING_BOX_START", dynBOUNDING_BOX_START },
    { "BOUNDING_BOX", dynBOUNDING_BOX },
    { "BOUNDING_BOX_END", dynBOUNDING_BOX_END },
    { "COLLISION_START", dynCOLLISION_START },
    { "COLLISION", dynCOLLISION },
    { "COLLISION_END", dynCOLLISION_END },
    { "GRAVITY", dynGRAVITY },
    { "K", dynK },
    { "WIND", dynWind },
    { NULL, NULL },
};

void CDynamicAnime::Load(char *script, int size, mgCFrame *top_frame, mgCMemory *stack) {
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  rotation;
    sceVu0FVECTOR  scale;

    Initialize();
    dynStack = stack;
    dynNowDA = this;
    dynTopFrame = top_frame;
    dynFrameCount = 0;
    dynVertexCount = 0;
    dynFixVertexCount = 0;
    dynBindVertexCount = 0;
    dynBBoxCount = 0;
    dynColCount = 0;
    if (top_frame != NULL) {
        this->top_frame = top_frame;
        dynTopFrame->GetPosition(position);
        dynTopFrame->GetRotation(rotation);
        dynTopFrame->GetScale(scale);
        dynTopFrame->SetPosition(0.0f, 0.0f, 0.0f);
        dynTopFrame->SetRotation(0.0f, 0.0f, 0.0f);
        dynTopFrame->SetScale(1.0f, 1.0f, 1.0f);

        CScriptInterpreter interpreter;

        interpreter.SetTag(dynmc_tag);
        interpreter.SetScript(script, size);
        interpreter.Run();
        dynTopFrame->SetPosition(position);
        dynTopFrame->SetRotation(rotation);
        dynTopFrame->SetScale(scale);
    }
}

int CDAColPipe::CheckHit(float *position) {
    sceVu0FVECTOR  displacement;
    sceVu0FVECTOR  local;
    float          axial_position;

    position[3] = 1.0f;
    sceVu0ApplyMatrix(local, inverse_matrix, position);
    sceVu0SubVector(displacement, local, center);
    displacement[0] /= radius[0];
    displacement[1] /= radius[1];
    displacement[2] /= radius[2];
    if (1.0f < displacement[axis]) {
        return 0;
    }
    if (-1.0f > displacement[axis]) {
        return 0;
    }
    displacement[axis] = 0.0f;
    if (mgDistVector(displacement) >= 1.0f) {
        return 0;
    }
    sceVu0Normalize(displacement, displacement);
    displacement[0] *= radius[0];
    displacement[1] *= radius[1];
    displacement[2] *= radius[2];
    axial_position = local[axis];
    sceVu0AddVector(local, center, displacement);
    local[axis] = axial_position;
    local[3] = 1.0f;
    sceVu0ApplyMatrix(position, lw_matrix, local);
    return 1;
}



// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", dynmc_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_816__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_817__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_818__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_819__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_820__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_821__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_822__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_823__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_824__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_825__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_826__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_827__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_828__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_829__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_830__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_831__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_832__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_833__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_834__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_835__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_836__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_837__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_838__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_839__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_840__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_841__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_842__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_855__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_976__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_977__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_978__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_979__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_1025__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", at_1074__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", __vt__10CDAColPipe__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dynamicanime", __vt__12CDACollision__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(dynNowDA, 0x4);
INCLUDE_BSS(dynStack, 0x4);
INCLUDE_BSS(dynTopFrame, 0x4);
INCLUDE_BSS(dynFrameCount, 0x4);
INCLUDE_BSS(dynVertexCount, 0x4);
INCLUDE_BSS(dynFixVertexCount, 0x4);
INCLUDE_BSS(dynBindVertexCount, 0x4);
INCLUDE_BSS(dynBBoxCount, 0x4);
INCLUDE_BSS(dynColCount, 0x4);
