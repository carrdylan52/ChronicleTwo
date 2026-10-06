#include "common.h"
#include "editinfo.hpp"
#include "scriptinterpreter.hpp"
#include "mg_memory.hpp"
#include "editparts.hpp"
#include "editmap.hpp"
#include "mg_math.hpp"
#include "menucommon.hpp"
#include <cstring>

static CEditInfoMngr *emapInfo__2; /**< Manager being filled by the script. */
static mgCMemory *emapStack__2; /**< Heap holding the part definitions and their text. */
static int emapIdx__2; /**< Index of the next part definition. */
static int emapMatID; /**< Index of the next material of the current part. */
static CEditPartsInfo *emapNowInfo__2; /**< Part definition being filled. */
static void *emapRect__2; /**< Rectangle table being filled. */
static int emapRectNum__2; /**< Number of rectangles in the current table. */
static int emapRectIdx__2; /**< Index of the next rectangle. */
static int emapFixNum__2; /**< Number of fixed parts. */
static int emapFixIdx__2; /**< Index of the next fixed part. */

static int emapEDIT_PARTS_NUM(SPI_STACK *stack, int argument_count);
static int emapEDIT_PARTS(SPI_STACK *stack, int argument_count);
static int emapID(SPI_STACK *stack, int argument_count);
static int emapPARTS_NAME(SPI_STACK *stack, int argument_count);
static int emapPARTS_ATR(SPI_STACK *stack, int argument_count);
static int emapPARTS_MATERIAL(SPI_STACK *stack, int argument_count);
static int emapPARTS_COMMENT(SPI_STACK *stack, int argument_count);
static int emapGROUND_PARTS(SPI_STACK *stack, int argument_count);
static int emapBLOCK_PARTS(SPI_STACK *stack, int argument_count);
static int emapRIVER_PARTS(SPI_STACK *stack, int argument_count);
static int emapFENCE_PARTS(SPI_STACK *stack, int argument_count);
static int emapCPOINT(SPI_STACK *stack, int argument_count);
static int emapWEIGHT(SPI_STACK *stack, int argument_count);
static int emapGEO_STONE(SPI_STACK *stack, int argument_count);
static int emapMAX_NUM(SPI_STACK *stack, int argument_count);
static int emapPAINT_NUM(SPI_STACK *stack, int argument_count);
static int emapPAINT_USED(SPI_STACK *stack, int argument_count);
static int emapPARTS_TYPE(SPI_STACK *stack, int argument_count);
static int emapPLACE_EPS(SPI_STACK *stack, int argument_count);
static int emapMAP_NO(SPI_STACK *stack, int argument_count);
static int emapPOLYN(SPI_STACK *stack, int argument_count);
static int emapRECT(SPI_STACK *stack, int argument_count);
static int emapPLACE_RECT(SPI_STACK *stack, int argument_count);
static int emapPLACE_RECT_END(SPI_STACK *stack, int argument_count);
static int emapPARTS_RECT(SPI_STACK *stack, int argument_count);
static int emapPARTS_RECT_END(SPI_STACK *stack, int argument_count);
static int emapPUT_RECT(SPI_STACK *stack, int argument_count);
static int emapPUT_RECT_END(SPI_STACK *stack, int argument_count);
static int emapEDIT_PARTS_END(SPI_STACK *stack, int argument_count);

/**
 * Associates edit information commands with their handlers.
 */
static SPI_TAG_PARAM emap_tag__2[] = {
    {"EDIT_PARTS_NUM", emapEDIT_PARTS_NUM},
    {"EDIT_PARTS", emapEDIT_PARTS},
    {"ID", emapID},
    {"PARTS_NAME", emapPARTS_NAME},
    {"PARTS_ATR", emapPARTS_ATR},
    {"PARTS_MATERIAL", emapPARTS_MATERIAL},
    {"PARTS_COMMENT", emapPARTS_COMMENT},
    {"GROUND_PARTS", emapGROUND_PARTS},
    {"BLOCK_PARTS", emapBLOCK_PARTS},
    {"RIVER_PARTS", emapRIVER_PARTS},
    {"FENCE_PARTS", emapFENCE_PARTS},
    {"CPOINT", emapCPOINT},
    {"WEIGHT", emapWEIGHT},
    {"GEO_STONE", emapGEO_STONE},
    {"MAX_NUM", emapMAX_NUM},
    {"PAINT_NUM", emapPAINT_NUM},
    {"PAINT_USED", emapPAINT_USED},
    {"PARTS_TYPE", emapPARTS_TYPE},
    {"PLACE_EPS", emapPLACE_EPS},
    {"MAP_NO", emapMAP_NO},
    {"POLYN", emapPOLYN},
    {"RECT", emapRECT},
    {"PLACE_RECT", emapPLACE_RECT},
    {"PLACE_RECT_END", emapPLACE_RECT_END},
    {"PARTS_RECT", emapPARTS_RECT},
    {"PARTS_RECT_END", emapPARTS_RECT_END},
    {"PUT_RECT", emapPUT_RECT},
    {"PUT_RECT_END", emapPUT_RECT_END},
    {"EDIT_PARTS_END", emapEDIT_PARTS_END},
    {NULL, NULL},
};

/**
 * Rounds a byte count up to the number of sixteen-byte heap blocks.
 */
static inline u32 align16_blocks(u32 size) {
    if (size & 0xF) {
        return (size >> 4) + 1;
    }
    return size >> 4;
}

// Code (.text)
void CEditInfoMngr::Initialize(void) {
    parts_info_num = 0;
    parts_info = NULL;
    fix_parts_num = 0;
    fix_parts = NULL;
    init_parts_num = 0;
    init_parts = NULL;
}

void CEditInfoMngr::SetePartsInfoTable(CEditPartsInfo *table, s32 num) {
    parts_info_num = num;
    parts_info = table;
}

void CEditInfoMngr::SeteFixPartsTable(ePlaceData *table, s32 num) {
    fix_parts_num = num;
    fix_parts = table;
}

CEditPartsInfo *CEditInfoMngr::GetePartsInfo(int no) {
    if (no < 0 || no >= parts_info_num) {
        return NULL;
    }
    return parts_info + no;
}

CEditPartsInfo *CEditInfoMngr::GetePartsInfo(char *name) {
    int index = 0;
    CEditPartsInfo *part = parts_info;
    for (; index < parts_info_num; index++, part++) {
        if (part->edit_name != NULL && strcmp(part->edit_name, name) == 0) {
            return part;
        }
    }
    return NULL;
}

CEditPartsInfo *CEditInfoMngr::GetePartsInfoAtID(int id) {
    if (id < 0) {
        return NULL;
    }
    CEditPartsInfo *part = parts_info;
    if (part == NULL) {
        return NULL;
    }
    for (int index = 0; index < parts_info_num; index++, part++) {
        if (part->id == id) {
            return part;
        }
    }
    return NULL;
}

CEditPartsInfo *CEditInfoMngr::GetePartsInfoAtType(int type) {
    int index = 0;
    CEditPartsInfo *part = parts_info;
    for (; index < parts_info_num; index++, part++) {
        if (type == part->GetPartsType()) {
            return part;
        }
    }
    return NULL;
}

/**
 * Allocates the table of part definitions.
 */
static int emapEDIT_PARTS_NUM(SPI_STACK *stack, int argument_count) {
    int parts_count = spiGetStackInt(stack);
    if (parts_count <= 0) {
        return 0;
    }
    CEditPartsInfo *table = new (emapStack__2->Alloc(
        align16_blocks(parts_count * sizeof(CEditPartsInfo)) + 2)) CEditPartsInfo[parts_count];
    emapInfo__2->SetePartsInfoTable(table, parts_count);
    return 1;
}

/**
 * Begins a part definition with its editor name.
 */
static int emapEDIT_PARTS(SPI_STACK *stack, int argument_count) {
    char converted_name[256];
    char *name;
    char *name_buffer;
    emapNowInfo__2 = emapInfo__2->GetePartsInfo(emapIdx__2++);
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    name = spiGetStackString(stack);
    ConvertFontCode(name, converted_name);
    name_buffer = (char *)emapStack__2->Alloc(align16_blocks(strlen(converted_name) + 1));
    if (name != NULL && name_buffer != NULL) {
        strcpy(name_buffer, converted_name);
        emapNowInfo__2->edit_name = name_buffer;
    }
    emapMatID = 0;
    return 1;
}

/**
 * Sets the identifier of the current part.
 */
static int emapID(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->id = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the model name of the current part.
 */
static int emapPARTS_NAME(SPI_STACK *stack, int argument_count) {
    char *text;
    char *buffer;

    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    text = spiGetStackString(stack);
    buffer = (char *)emapStack__2->Alloc(align16_blocks(strlen(text) + 1));
    if (text != NULL) {
        if (buffer != NULL) {
            strcpy(buffer, text);
            emapNowInfo__2->parts_name = buffer;
        }
    }
    return 1;
}

/**
 * Adds script attributes to the current part.
 */
static int emapPARTS_ATR(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->attr |= spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the item and quantity of the next material for the current part.
 */
static int emapPARTS_MATERIAL(SPI_STACK *stack, int argument_count) {
    int index;
    EditPartsMaterial *material;

    if ((emapNowInfo__2 == NULL) || (argument_count < 2)) {
        return 0;
    }
    index = emapMatID;
    emapMatID = index + 1;
    material = emapNowInfo__2->GetMaterial(index);
    if (material == NULL) {
        return 0;
    }
    material->item_no = spiGetStackInt(stack++);
    material->num = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the description of the current part.
 */
static int emapPARTS_COMMENT(SPI_STACK *stack, int argument_count) {
    char *text;
    char *buffer;

    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    text = spiGetStackString(stack);
    buffer = (char *)emapStack__2->Alloc(align16_blocks(strlen(text) + 1));
    if (text != NULL) {
        if (buffer != NULL) {
            strcpy(buffer, text);
            emapNowInfo__2->comment = buffer;
        }
    }
    return 1;
}

/**
 * Sets the two culture point values of the current part.
 */
static int emapCPOINT(SPI_STACK *stack, int argument_count) {
    SPI_STACK *second;

    second = stack + 1;
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->cpoint[0] = spiGetStackInt(stack);
    emapNowInfo__2->cpoint[1] = spiGetStackInt(second);
    return 1;
}

/**
 * Sets the weight of the current part.
 */
static int emapWEIGHT(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->weight = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the geostone the current part belongs to.
 */
static int emapGEO_STONE(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->geo_stone = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets how many copies of the current part can be placed.
 */
static int emapMAX_NUM(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->max_num = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets how many colours of the current part can be painted.
 */
static int emapPAINT_NUM(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->paint_num = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the amount of paint the current part takes to paint.
 */
static int emapPAINT_USED(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->paint_used = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the type of the current part.
 */
static int emapPARTS_TYPE(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->parts_type = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets the fraction of the part base that must rest on ground for placement.
 */
static int emapPLACE_EPS(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->place_eps = spiGetStackFloat(stack);
    return 1;
}

/**
 * Sets the map number of the current part.
 */
static int emapMAP_NO(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->map_no = spiGetStackInt(stack);
    return 1;
}

/**
 * Sets up to three polygon values of the current part.
 */
static int emapPOLYN(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->polyn[0] = spiGetStackInt(stack++);
    if (argument_count >= 2) {
        emapNowInfo__2->polyn[1] = spiGetStackInt(stack++);
    }
    if (argument_count >= 3) {
        emapNowInfo__2->polyn[2] = spiGetStackInt(stack);
    }
    return 1;
}

/**
 * Adds ground attributes to the current part.
 */
static int emapGROUND_PARTS(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->attr |= EDIT_PARTS_ATR_GROUND;
    return 1;
}

/**
 * Adds block attributes to the current part.
 */
static int emapBLOCK_PARTS(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->attr |= EDIT_PARTS_ATR_BLOCK;
    return 1;
}

/**
 * Adds river attributes to the current part.
 */
static int emapRIVER_PARTS(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->attr |= EDIT_PARTS_ATR_RIVER;
    return 1;
}

/**
 * Adds fence attributes to the current part.
 */
static int emapFENCE_PARTS(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapNowInfo__2->attr |= EDIT_PARTS_ATR_FENCE;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editinfo", emapRECT__FP9SPI_STACKi);

/**
 * Sets the placement rectangle count and resets the rectangle index.
 */
static int emapPLACE_RECT(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapRectNum__2 = spiGetStackInt(stack);
    emapRectIdx__2 = 0;
    return 1;
}

/**
 * Clears the placement rectangle table.
 */
static int emapPLACE_RECT_END(SPI_STACK *stack, int argument_count) {
    emapRect__2 = NULL;
    return 1;
}

/**
 * Sets the part rectangle count and resets the rectangle index.
 */
static int emapPARTS_RECT(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapRectNum__2 = spiGetStackInt(stack);
    emapRectIdx__2 = 0;
    return 1;
}

/**
 * Clears the part rectangle table.
 */
static int emapPARTS_RECT_END(SPI_STACK *stack, int argument_count) {
    emapRect__2 = NULL;
    return 1;
}

/**
 * Sets the put rectangle count and resets the rectangle index.
 */
static int emapPUT_RECT(SPI_STACK *stack, int argument_count) {
    if (emapNowInfo__2 == NULL) {
        return 0;
    }
    emapRectNum__2 = spiGetStackInt(stack);
    emapRectIdx__2 = 0;
    return 1;
}

/**
 * Clears the put rectangle table.
 */
static int emapPUT_RECT_END(SPI_STACK *stack, int argument_count) {
    emapRect__2 = NULL;
    return 1;
}

/**
 * Ends the current part definition.
 */
static int emapEDIT_PARTS_END(SPI_STACK *stack, int argument_count) {
    emapNowInfo__2 = NULL;
    return 1;
}

void CEditInfoMngr::LoadEditInfo(char *script, int size, mgCMemory *stack) {
    emapInfo__2 = this;
    emapStack__2 = stack;
    emapIdx__2 = 0;
    emapNowInfo__2 = NULL;
    emapRect__2 = NULL;
    emapRectNum__2 = 0;
    emapRectIdx__2 = 0;
    emapFixNum__2 = 0;
    emapFixIdx__2 = 0;
    CScriptInterpreter interpreter;
    interpreter.SetTag(emap_tag__2);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

CFuncPoint *CEditMap::GetEvent(float *pos, int check_type, MapEventInfo *info) {
    if (info != NULL) {
        info->event_no = 0;
        mgUnitMatrix(info->matrix);
        info->point_no = -1;
        info->parts_no = -1;
    }
    CFuncPoint *point = CMap::GetEvent(pos, check_type, info);
    if (point != NULL) {
        return point;
    }
    int index = 0;
    CEditParts *part = edit_parts;
    for (; index < edit_parts_max; index++, part++) {
        if (part->func_point_mngr.flag & FUNC_POINT_MNGR_EVENT) {
            int unnamed = part->name[0] == 0;
            if (unnamed == 0 && part->GetShow() != 0 &&
                part->state != EDIT_PARTS_STATE_NONE && part->info != NULL) {
                CFuncPointMngr *manager = &part->func_point_mngr;
                manager->GetStart(FUNC_POINT_EVENT);
                int accepted;
                CFuncPoint *point;
                if ((point = manager->Get()) != NULL) {
                    do {
                        point->frame.SetReference(&part->frame);
                        accepted = CheckFuncEvent(point, pos, check_type, info, NULL);
                        point->frame.DeleteReference();
                        if (accepted != 0) {
                            info->parts_no = index;
                            return point;
                        }
                    } while ((point = manager->Get()) != NULL);
                }
            }
        }
    }
    return NULL;
}

// Small uninitialised data (.sbss)
INCLUDE_BSS(emapRectType, 0x4);
