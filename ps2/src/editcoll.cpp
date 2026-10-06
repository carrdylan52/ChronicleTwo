#include "common.h"
#include "editcoll.hpp"

#include <libvu0.h>

#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_frame.hpp"

static float OverlapPoly3AreaXZ(float (*clipped)[4], float (*clipper)[4], mgVu0FBOX *box);

// Code (.text)
int ClipBoxXZ(float *max_a, float *min_a, float *max_b, float *min_b) {
    register int status;

    // Only the sign flags of the two subtractions matter: any negative component means apart.
    asm {
        lqc2 $vf10, 0x0($4)
        lqc2 $vf11, 0x0($5)
        lqc2 $vf1, 0x0($6)
        lqc2 $vf2, 0x0($7)
        vnop
        vnop
        vnop
        ctc2.ni $0, $vi16
        vsub.xz $vf25, $vf10, $vf2
        vsub.xz $vf25, $vf1, $vf11
        vnop
        vnop
        vnop
        vnop
        vnop
        cfc2.ni status, $vi16
    }

    return (status & MG_VU0_STATUS_SIGN_STICKY) == 0;
}

/**
 * Returns the area of the region of the XZ plane covered by both of two
 * triangles, and when a box is given writes the bounds of that region
 * lifted onto the second triangle's plane.
 *
 * @mangled OverlapPoly3AreaXZ__FPA4_fPA4_fP9mgVu0FBOX
 * @address 0x1A40A0
 * @size 0x5E0
 */
#ifdef NONMATCHING
static float OverlapPoly3AreaXZ(float (*clipped)[4], float (*clipper)[4], mgVu0FBOX *box) {
    float vertices[2][7][4];
    int count = 3;
    for (int i = 0; i < 3; ++i) {
        for (int component = 0; component < 4; ++component) {
            vertices[0][i][component] = clipped[i][component];
        }
        vertices[0][i][1] = 0.0f;
    }
    int source = 0;
    for (int edge = 0; edge < 3; ++edge) {
        int target = source ^ 1;
        int output_count = 0;
        float edge_x = clipper[edge + 1 < 3 ? edge + 1 : 0][0] - clipper[edge][0];
        float edge_z = clipper[edge + 1 < 3 ? edge + 1 : 0][2] - clipper[edge][2];
        for (int i = 0; i < count; ++i) {
            float *first = vertices[source][i];
            float *second = vertices[source][i + 1 < count ? i + 1 : 0];
            float first_side = edge_x * (first[2] - clipper[edge][2]) - edge_z * (first[0] - clipper[edge][0]);
            float second_side = edge_x * (second[2] - clipper[edge][2]) - edge_z * (second[0] - clipper[edge][0]);
            if (first_side >= 0.0f) {
                for (int component = 0; component < 4; ++component)
                    vertices[target][output_count][component] = first[component];
                output_count++;
            }
            if ((first_side >= 0.0f) != (second_side >= 0.0f)) {
                float factor = first_side / (first_side - second_side);
                for (int component = 0; component < 4; ++component)
                    vertices[target][output_count][component] = first[component] + factor * (second[component] - first[component]);
                output_count++;
            }
        }
        count = output_count;
        source = target;
        if (count == 0) break;
    }
    if (count < 3) return 0.0f;
    float area = 0.0f;
    for (int i = 0; i < count; ++i) {
        float *first = vertices[source][i];
        float *second = vertices[source][i + 1 < count ? i + 1 : 0];
        area += second[0] * first[2] - first[0] * second[2];
    }
    if (box != NULL) {
        float e0x = clipper[1][0] - clipper[0][0];
        float e0y = clipper[1][1] - clipper[0][1];
        float e0z = clipper[1][2] - clipper[0][2];
        float e1x = clipper[2][0] - clipper[1][0];
        float e1y = clipper[2][1] - clipper[1][1];
        float e1z = clipper[2][2] - clipper[1][2];
        float nx = e0y * e1z - e0z * e1y;
        float ny = e0z * e1x - e0x * e1z;
        float nz = e0x * e1y - e0y * e1x;
        float distance = -(nx * clipper[0][0] + ny * clipper[0][1] + nz * clipper[0][2]);
        for (int i = 0; i < count; ++i) {
            float *vertex = vertices[source][i];
            vertex[1] = -(nx * vertex[0] + nz * vertex[2] + distance) / ny;
            if (i == 0) {
                for (int component = 0; component < 4; ++component)
                    box->max[component] = box->min[component] = vertex[component];
            } else {
                mgVectorMaxMin(box->max, box->min, box->max, box->min, vertex);
            }
        }
    }
    return area * 0.5f;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editcoll", OverlapPoly3AreaXZ__FPA4_fPA4_fP9mgVu0FBOX);
#endif

#ifdef NONMATCHING
void CEditCollision::Copy(CEditCollision &dest, int area_kind, mgCMemory *memory) {
    int count = 0;
    for (int i = 0; i < poly_count; ++i) {
        if (poly[i].area_kind == area_kind) ++count;
    }
    if (count <= 0 || memory == NULL) {
        dest.poly_count = 0;
        dest.poly = NULL;
        return;
    }
    unsigned int bytes = count * sizeof(CCPoly);
    dest.poly_count = count;
    dest.poly = new (memory->Alloc(((bytes + 15) / 16) + 2)) CCPoly[count];
    if (dest.poly != NULL) {
        int next = 0;
        for (int i = 0; i < poly_count; ++i) {
            if (poly[i].area_kind == area_kind) dest.poly[next++] = poly[i];
        }
    }
    CreateBBox();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editcoll", Copy__14CEditCollisionFR14CEditCollisioniP9mgCMemory);
#endif

float CEditCollision::AreaXZ() {
    float total_area;
    CCPoly *triangle;
    int index;
    int count;
    count = poly_count;
    triangle = poly;
    total_area = 0.0f;
    for (index = 0; index < count; index++) {
        float signed_area;
        float bx;
        float bz;
        float cz;
        float ax;
        float az;
        float cx;
        ax = triangle->vertex[0][0];
        bz = triangle->vertex[1][2];
        bx = triangle->vertex[1][0];
        az = triangle->vertex[0][2];
        cz = triangle->vertex[2][2];
        cx = triangle->vertex[2][0];
        float sum = -ax * bz + bx * az;
        sum += -bx * cz + cx * bz;
        sum += -cx * az + ax * cz;
        signed_area = 0.5f * sum;
        total_area += (signed_area < 0.0f) ? -signed_area : signed_area;
        triangle++;
    }
    return total_area;
}

int CEditCollision::OverlapPoly3XZ(float (*triangle)[4], float *area, mgVu0FBOX *box) {
    sceVu0FVECTOR tri_max;
    sceVu0FVECTOR tri_min;
    mgVu0FBOX     overlap_box;
    sceVu0FVECTOR poly_max;
    sceVu0FVECTOR poly_min;
    CCPoly       *p;
    float         overlap;
    float         total;
    int           overlap_count;
    int           i;

    p = poly;
    if (p == NULL) {
        return 0;
    }

    mgVectorMaxMin(tri_max, tri_min, triangle[0], triangle[1], triangle[2]);

    if (area != NULL) {
        *area = 0.0f;
    }

    if (box != NULL) {
        mgZeroVectorW(box->max);
        mgZeroVectorW(box->min);
    }

    if (!ClipBoxXZ(tri_max, tri_min, bbox.max, bbox.min)) {
        return 0;
    }

    total = 0.0f;
    overlap_count = 0;
    for (i = 0; i < poly_count; i++, p++) {
        mgVectorMaxMin(poly_max, poly_min, p->vertex[0], p->vertex[1], p->vertex[2]);
        if (ClipBoxXZ(tri_max, tri_min, poly_max, poly_min)) {
            overlap = OverlapPoly3AreaXZ(triangle, p->vertex, &overlap_box);
            overlap = overlap < 0.0f ? -overlap : overlap;
            total += overlap;

            if (overlap > 0.0) {
                if (box != NULL) {
                    if (overlap_count == 0) {
                        *box = overlap_box;
                    } else {
                        mgBoxMaxMin(box, &overlap_box);
                    }
                }
                overlap_count++;
            }
        }
    }

    if (area != NULL) {
        *area = total;
    }

    if (total > 0.0f) {
        return 1;
    }

    return 0;
}

float CEditCollision::OverlapXZ(CEditCollision &other, float (*matrix)[4], mgVu0FBOX *box) {
    float total;
    int overlaps;
    float transformed[3][4];
    float area;
    mgVu0FBOX overlap_box;
    CCPoly *polygon;
    int i;
    int count;
    i = 0;
    count = other.poly_count;
    total = 0.0f;
    polygon = other.poly;
    overlaps = 0;
    for (i = 0; i < count; i++) {
        mgApplyMatrixN(transformed, matrix, polygon->vertex, 3);
        if (OverlapPoly3XZ(transformed, &area, &overlap_box) != 0) {
            total += area;
            if (box != NULL) {
                if (overlaps == 0) {
                    overlaps = 1;
                    *box = overlap_box;
                } else {
                    mgBoxMaxMin(box, &overlap_box);
                }
            }
        }
        polygon++;
    }
    return total;
}

#ifdef NONMATCHING
int CEditCollision::OverlapPoly3XZ(float (*triangle)[4], float (*matrix)[4], float *area) {
    if (poly == NULL) return 0;
    float tri_max[4], tri_min[4], transformed_max[4], transformed_min[4];
    mgVectorMaxMin(tri_max, tri_min, triangle[0], triangle[1], triangle[2]);
    if (area != NULL) *area = 0.0f;
    mgApplyMatrix(transformed_max, transformed_min, matrix, bbox.max, bbox.min);
    if (!ClipBoxXZ(tri_max, tri_min, transformed_max, transformed_min)) return 0;
    float total = 0.0f;
    for (int i = 0; i < poly_count; ++i) {
        float transformed[3][4];
        mgApplyMatrixN(transformed, matrix, poly[i].vertex, 3);
        if (transformed[0][1] <= 0.1f && transformed[1][1] <= 0.1f && transformed[2][1] <= 0.1f) {
            float overlap = OverlapPoly3AreaXZ(triangle, transformed, NULL);
            if (overlap < 0.0f) overlap = -overlap;
            total += overlap;
        }
    }
    if (area != NULL) *area = total;
    return total > 0.0f;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editcoll", OverlapPoly3XZ__14CEditCollisionFPA4_fPA4_fPf);
#endif

void CEditCollision::ApplyMatrix(float (*matrix)[4]) {
    CCPoly *p;
    int     i;
    int     j;

    p = poly;
    if (p == NULL) {
        return;
    }

    for (i = 0; i < poly_count; i++, p++) {
        mgApplyMatrixN(p->vertex, matrix, p->vertex, 3);

        // Heights are rounded to the nearest whole unit.
        for (j = 0; j < 3; j++) {
            if (p->vertex[j][1] > 0.0f) {
                p->vertex[j][1] = (int)(p->vertex[j][1] + 0.5f);
            } else {
                p->vertex[j][1] = (int)(p->vertex[j][1] - 0.5f);
            }
        }

        mgPlaneNormal(p->normal, p->vertex[0], p->vertex[1], p->vertex[2]);
        sceVu0Normalize(p->normal, p->normal);
    }

    CreateBBox();
}

#ifdef NONMATCHING
void CEditCollision::DeleteVerticalPoly() {
    if (poly == NULL) return;
    for (int index = 0; index < poly_count; ++index) {
        sceVu0FVECTOR normal;
        sceVu0Normalize(normal, poly[index].normal);
        float vertical_component = normal[1];
        if (vertical_component < 0.0f) vertical_component = -vertical_component;
        if (vertical_component < 0.01f) {
            if (poly_count == 0) break;
            --poly_count;
            poly[index] = poly[poly_count];
            --index;
        }
    }
    CreateBBox();
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editcoll", DeleteVerticalPoly__14CEditCollisionFv);
#endif

#ifdef NONMATCHING
int CEditCollision::PickupVerticalPoly() {
    if (poly == NULL) return 0;
    for (int i = 0; i < poly_count; ++i) {
        sceVu0FVECTOR normal;
        sceVu0Normalize(normal, poly[i].normal);
        float y = normal[1] < 0.0f ? -normal[1] : normal[1];
        if (y > 0.01f) {
            if (poly_count == 0) break;
            --poly_count;
            poly[i] = poly[poly_count];
            --i;
        }
    }
    CreateBBox();
    short wall_count = 0;
    for (int i = 0; i < poly_count; ++i) {
        CCPoly *same_plane = NULL;
        for (int j = 0; j < i; ++j) {
            sceVu0FVECTOR first_normal, second_normal;
            sceVu0Normalize(first_normal, poly[i].normal);
            sceVu0Normalize(second_normal, poly[j].normal);
            if (mgDistVector(first_normal, second_normal) <= 0.01f) {
                float delta = sceVu0InnerProduct(first_normal, poly[i].vertex[0]) -
                              sceVu0InnerProduct(second_normal, poly[j].vertex[0]);
                if (delta < 0.0f) delta = -delta;
                if (delta <= 0.01f) {
                    same_plane = &poly[j];
                    break;
                }
            }
        }
        if (same_plane != NULL) poly[i].ignore_mask = same_plane->ignore_mask;
        else poly[i].ignore_mask = wall_count++;
    }
    return wall_count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editcoll", PickupVerticalPoly__14CEditCollisionFv);
#endif
