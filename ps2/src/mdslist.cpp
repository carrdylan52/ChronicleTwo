#include "common.h"
#include "mdslist.hpp"

#include <cstdio>
#include <cstring>
#include <libvu0.h>

#include "character.hpp"
#include "collision.hpp"
#include "dataread.hpp"
#include "mapload.hpp"
#include "mg_dataset.hpp"
#include "mg_frame.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mg_visual.hpp"
#include "scriptinterpreter.hpp"

static int        now_mds_num;   /**< Next entry the pack script fills. */
static int        max_mds_num;   /**< Number of entries allocated for the pack. */
static CMdsList  *pcpMdsList;    /**< Pack list the script is loading. */
static CMdsInfo  *pcpMdsInfo;    /**< Entries allocated for the pack. */
static CMdsInfo  *pcpNowMdsInfo; /**< Entry the current script tags describe. */
static mgCMemory *pcpStack;     /**< Memory used to load the pack's entries. */
static u_int     *pcp_file;      /**< Pack whose script is being read. */
static int        pcpAllScissor; /**< Non-zero enables clipping on every loaded frame. */

static int          pcpMDS(SPI_STACK *stack, int count);
static int          pcpTYPE(SPI_STACK *stack, int count);
static int          pcpFAR_CLIP(SPI_STACK *stack, int count);
static int          pcpMDS_END(SPI_STACK *stack, int count);
static CCharacter2 *CreateChara(u_int *pack, char *name, mgCMemory *stack);

static SPI_TAG_PARAM pcp_tag[] = { /**< Tags describing entries of a PCP pack. */
    { "MDS", pcpMDS },
    { "TYPE", pcpTYPE },
    { "FAR_CLIP", pcpFAR_CLIP },
    { "MDS_END", pcpMDS_END },
    { NULL, NULL },
    { NULL, NULL },
};

// Code (.text)
int CMapPiece::AssignMds(CMdsInfo *info) {
    if (info == NULL) {
        return 0;
    }
    name = info->name;
    chara = info->chara;
    type = info->type;
    frame = info->frame;
    far_dist = info->far_dist;
    fade = info->far_fade;
    return 1;
}

int CMapPiece::GetPoly(int type, CCPoly *poly, mgVu0FBOX &box, int max) {
    if (type != this->type) {
        return 0;
    }
    if (col_type != 0) {
        return 0;
    }
    if (!GetShow()) {
        return 0;
    }

    UpDatePosition();
    return ((CColFrame *)frame)->PickUpNearPoly(poly, box, max);
}

void CMapPiece::SetTimeBand(float start, float end) {
    time_start = start;
    time_end = end;
}

PieceMaterial *CMapPiece::GetMaterial(int index) {
    if (material == NULL) {
        return NULL;
    }
    if (index < 0 || index >= material_num) {
        return NULL;
    }
    return &material[index];
}

void CMapPiece::Step() {
    if (chara != NULL) {
        UpDatePosition();
        chara->Step();
    }
}

int CMapPiece::GetBoundBox(mgVu0FBOX *box) {
    if (frame == NULL) {
        return 0;
    }
    UpDatePosition();
    return frame->GetWorldBBox(box);
}

#ifdef NONMATCHING
int CMapPiece::DrawSub(int direct) {
    sceVu0FVECTOR  saved_color[4];
    PieceMaterial *piece_material;
    int            result;
    int            i;

    if (draw_enable == 0) {
        return 0;
    }
    if (type & 0x1) {
        return 0;
    }

    piece_material = material;
    if (piece_material != NULL) {
        for (i = 0; i < material_num; i++, piece_material++) {
            if (piece_material->material != NULL) {
                *(u_long128 *)saved_color[i] = *(u_long128 *)piece_material->material->diffuse;
                *(u_long128 *)piece_material->material->diffuse = *(u_long128 *)piece_material->color;
            }
        }
    }

    if (direct != 0) {
        result = CObjectFrame::DrawDirect();
    } else {
        result = CObjectFrame::Draw();
    }

    piece_material = material;
    if (piece_material != NULL) {
        for (i = 0; i < material_num; i++, piece_material++) {
            if (piece_material->material != NULL) {
                *(u_long128 *)piece_material->material->diffuse = *(u_long128 *)saved_color[i];
            }
        }
    }
    return result;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mdslist", DrawSub__9CMapPieceFi);
#endif

void CMapPiece::Copy(CMapPiece &dest, mgCMemory *stack) {
    int i;

    dest.Initialize();
    CObjectFrame::Copy(dest, stack);
    dest.name = name;
    dest.type = type;
    dest.draw_enable = draw_enable;
    dest.col_type = col_type;
    dest.time_start = time_start;
    dest.time_end = time_end;
    dest.material_num = material_num;

    if (stack == NULL) {
        dest.material = material;
    } else {
        dest.material = new (stack->Alloc(algn16_size(material_num * sizeof(PieceMaterial)) + 2)) PieceMaterial[material_num];
        if (dest.material == NULL) {
            dest.material_num = 0;
        }
        for (i = 0; i < dest.material_num; i++) {
            dest.material[i] = material[i];
        }
    }

    if (chara != NULL && stack != NULL) {
        dest.chara = new (stack->Alloc(algn16_size(sizeof(CCharacter2)) + 2)) CCharacter2;
        if (dest.chara != NULL) {
            chara->Copy(*dest.chara, stack);
            dest.frame = dest.chara->CObjectFrame::frame;
        }
    } else {
        dest.chara = chara;
    }
}

void CMapPiece::Initialize() {
    int i;

    CObjectFrame::Initialize();
    type = MDS_TYPE_MODEL;
    chara = NULL;
    draw_enable = 1;
    name = NULL;
    material_num = 0;
    col_type = 0;
    col_param = 0;
    for (i = 0; i < material_num; i++) {
        memset(&material[i], 0, sizeof(PieceMaterial));
    }
    time_end = 0.0f;
    time_start = 0.0f;
}

void CMdsInfo::Initialize(void) {
    name = NULL;
    type = MDS_TYPE_MODEL;
    frame = NULL;
    chara = NULL;
    far_dist = -1.0f;
    far_fade = 0;
}

CMdsList *CMdsListSet::SearchMdsList(char *name) {
    int i;

    if (name == NULL) {
        return NULL;
    }
    for (i = 0; i < mds_list_num; i++) {
        if (mds_list[i].name != NULL && strcmp(mds_list[i].name, name) == 0) {
            return &mds_list[i];
        }
    }
    return NULL;
}

CMdsList *CMdsListSet::GetMdsList(int index) {
    if (index < 0 || mds_list_num < index) {
        return NULL;
    }
    return &mds_list[index];
}

CMdsInfo *CMdsListSet::SearchMDS(char *name) {
    CMdsList *entry;
    CMdsInfo *info;
    int       i;

    for (i = 0; ; i++) {
        entry = GetMdsList(i);
        if (entry == NULL) {
            break;
        }
        info = entry->GetList(name);
        if (info != NULL) {
            return info;
        }
    }
    return NULL;
}

int CMdsListSet::LoadPCPFile(char *name, unsigned int *pack, mgCMemory *stack, int all_scissor) {
    CMdsList *entry;
    int i;
    int is_free;

    if (name == NULL) {
        return 0;
    }
    if (SearchMdsList(name) != NULL) {
        return 0;
    }
    entry = NULL;
    i = 0;
    for (; i < mds_list_num; i++) {
        is_free = !mds_list[i].list || !mds_list[i].name || mds_list[i].num == 0;
        if (is_free) {
            entry = &mds_list[i];
        }
    }
    if (entry == NULL) {
        return 0;
    }
    entry->LoadPCPFile(name, pack, stack, all_scissor);
    return 1;
}

s32 CMdsListSet::DeleteMdsList(char *name) {
    CMdsList *mds_list = SearchMdsList(name);
    if (mds_list == NULL) {
        return 0;
    }
    mds_list->name = NULL;
    mds_list->num = 0;
    mds_list->list = NULL;
    return 1;
}

int CMdsListSet::LoadIMGFile(char *name, mgCEnterIMGInfo *info, mgCMemory *stack) {
    CIMGList *entry;
    int i;

    if (name == NULL) {
        return 0;
    }
    if (SearchIMGList(name) != NULL) {
        return 0;
    }
    entry = NULL;
    i = 0;
    for (; i < img_list_num; i++) {
        if ((u_char)(!img_list[i].name) != 0) {
            entry = &img_list[i];
        }
    }
    if (entry == NULL) {
        return 0;
    }
    entry->LoadIMGFile(name, info, stack);
    return 1;
}

void CMdsListSet::DeleteIMG(char *name) {
    CIMGList *img_list = SearchIMGList(name);
    if (img_list != NULL) {
        img_list->name = NULL;
        img_list->info = NULL;
    }
}

CIMGList *CMdsListSet::SearchIMGList(char *name) {
    int i;

    for (i = 0; i < img_list_num; i++) {
        if ((u_char)(!img_list[i].name) == 0 && img_list[i].name != NULL && strcmp(img_list[i].name, name) == 0) {
            return &img_list[i];
        }
    }
    return NULL;
}

#ifdef NONMATCHING
int CMdsListSet::GetTextureBlockNo(int group, int *out_block, int max) {
    mgCEnterIMGInfo *info;
    int              count;
    int              first_block;
    int              block_count;
    int              i;
    int              block;

    count = 0;
    for (i = 0; i < img_list_num; i++) {
        if (img_list[i].name != NULL) {
            info = img_list[i].info;
            if (info != NULL) {
                first_block = -1;
                if (group >= 0 && group < MG_TEXTURE_IMG_GROUP_MAX) {
                    first_block = info->block[group];
                }
                if (first_block >= 0) {
                    block_count = 0;
                    if (group >= 0 && group < MG_TEXTURE_IMG_GROUP_MAX) {
                        block_count = info->block_num[group];
                    }
                    for (block = 0; block < block_count && count < max; block++) {
                        out_block[count++] = first_block + block;
                    }
                }
            }
        }
    }
    return count;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mdslist", GetTextureBlockNo__11CMdsListSetFiPii);
#endif

void CMdsListSet::Initialize() {
    int i;
    int j;

    mds_list_num = 8;
    for (i = 0; i < mds_list_num; i++) {
        mds_list[i].name = NULL;
        mds_list[i].num = 0;
        mds_list[i].list = NULL;
    }
    img_list_num = 16;
    for (j = 0; j < img_list_num; j++) {
        img_list[j].name = NULL;
        img_list[j].info = NULL;
    }
}

CMdsInfo *CMdsList::GetList(int index) {
    if (index < 0 || index >= num) {
        return NULL;
    }
    return &list[index];
}

int CMdsList::GetListID(char *name) {
    int i;

    if (name == NULL || *name == '\0') {
        return -1;
    }
    for (i = 0; i < num; i++) {
        if (list[i].name != NULL && strcasecmp(list[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

CMdsInfo *CMdsList::GetList(char *name) {
    int index;

    index = GetListID(name);
    if (index < 0) {
        return NULL;
    }
    return GetList(index);
}

int CIMGList::LoadIMGFile(char *name, mgCEnterIMGInfo *info, mgCMemory *stack) {
    if (name == NULL) {
        return 0;
    }
    this->name = NULL;
    this->name = (char *)stack->Alloc(algn16_size(strlen(name) + 1));
    strcpy(this->name, name);
    this->info = NULL;
    if (info != NULL) {
        this->info = new (stack->Alloc(algn16_size(sizeof(mgCEnterIMGInfo)) + 2)) mgCEnterIMGInfo;
        *this->info = *info;
    }
    return 1;
}
/**
 * Starts a named pack entry, rejecting duplicate names and entries beyond the allocated list.
 */
static int pcpMDS(SPI_STACK *stack, int count) {
    char         *name;
    char         *entry_name;
    unsigned int  name_size;

    if (now_mds_num >= max_mds_num) {
        pcpNowMdsInfo = NULL;
        return 0;
    }
    name = spiGetStackString(stack);
    if (name != NULL && pcpMdsList->GetList(name) != NULL) {
        printf("same mds %s\n", name);
        pcpNowMdsInfo = NULL;
        return 0;
    }
    pcpNowMdsInfo = &pcpMdsInfo[now_mds_num++];
    pcpNowMdsInfo->Initialize();
    if (name == NULL) {
        pcpNowMdsInfo->name = NULL;
    } else {
        name_size = strlen(name) + 1;
        entry_name = (char *)pcpStack->Alloc(algn16_size(name_size));
        strcpy(entry_name, name);
        pcpNowMdsInfo->name = entry_name;
    }
    return 1;
}
/**
 * Converts the script's entry type to the model, collision or character type stored in the list.
 */
static int pcpTYPE(SPI_STACK *stack, int count) {
    int type;

    if (pcpNowMdsInfo == NULL) {
        return 0;
    }
    type = spiGetStackInt(stack);
    switch (type) {
    case 0:
        type = MDS_TYPE_MODEL;
        break;
    case 1:
        type = MDS_TYPE_COLLISION;
        break;
    case 2:
        type = MDS_TYPE_CAMERA_COLLISION;
        break;
    case 3:
    case 4:
        type = MDS_TYPE_CHARA;
        break;
    }
    pcpNowMdsInfo->type = type;
    return 1;
}
/**
 * Sets the current entry's far distance and whether it fades at that distance.
 */
static int pcpFAR_CLIP(SPI_STACK *stack, int count) {
    SPI_STACK *fade_argument;

    fade_argument = &stack[1];
    if (pcpNowMdsInfo == NULL) {
        return 0;
    }
    pcpNowMdsInfo->far_dist = spiGetStackFloat(stack);
    pcpNowMdsInfo->far_fade = spiGetStackInt(fade_argument);
    return 1;
}
/**
 * Loads the current entry's file as its selected type and applies the pack's clipping setting.
 */
static int pcpMDS_END(SPI_STACK *stack, int count) {
    MDS_HEADER  *file;
    mgCFrame    *frame;
    CCharacter2 *chara;
    int          size;

    if (pcpNowMdsInfo == NULL) {
        return 0;
    }
    frame = NULL;
    file = (MDS_HEADER *)GetPackFile(pcp_file, pcpNowMdsInfo->name, &size);
    if (file == NULL) {
        return 0;
    }
    switch (pcpNowMdsInfo->type) {
    case MDS_TYPE_MODEL:
        frame = mgLoadMDSFile(file, pcpStack, NULL, NULL);
        break;
    case MDS_TYPE_CAMERA_COLLISION:
    case MDS_TYPE_COLLISION:
        frame = LoadCollisionFile(file, pcpStack);
        break;
    case MDS_TYPE_CHARA:
        chara = CreateChara((u_int *)file, "info.cfg", pcpStack);
        pcpNowMdsInfo->chara = chara;
        if (chara != NULL) {
            frame = chara->CObjectFrame::frame;
        }
        break;
    }
    if (pcpAllScissor != 0 && frame != NULL) {
        mgCFrameAttr attr;

        attr.clip_enable = 1;
        frame->SetAttrParam(attr, 1, MG_FRAME_ATTR_CLIP);
    }
    pcpNowMdsInfo->frame = frame;
    return 1;
}

void CMdsList::LoadPCPFile(char *name, unsigned int *pack, mgCMemory *stack, int all_scissor) {
    u_int *files[512];
    char  *file_names[512];
    char  *script;
    int    script_size;

    pcpMdsList = this;
    GetPackFileNum(pack);
    num = GetPackFileExt(pack, "mds", files, 512, NULL, file_names);
    num += GetPackFileExt(pack, "chr", files, 512, NULL, file_names);
    if (num >= 512) {
        printf("over pcp %d\n", num);
    }
    this->name = NULL;
    if (name != NULL) {
        this->name = (char *)stack->Alloc(algn16_size(strlen(name) + 1));
        strcpy(this->name, name);
    }
    list = new (stack->Alloc(algn16_size(num * sizeof(CMdsInfo)) + 2)) CMdsInfo[num];
    now_mds_num = 0;
    max_mds_num = num;
    pcpMdsInfo = list;
    pcpStack = stack;
    pcp_file = pack;
    pcpAllScissor = all_scissor;
    pcpNowMdsInfo = NULL;
    script = (char *)GetPackFile(pack, "info.cfg", &script_size);

    CScriptInterpreter interpreter;

    interpreter.SetTag(pcp_tag);
    interpreter.SetScript(script, script_size);
    interpreter.Run();
}
/**
 * Creates and initializes a character from a pack using the named configuration script.
 */
static CCharacter2 *CreateChara(u_int *pack, char *name, mgCMemory *stack) {
    CCharacter2 *chara;

    chara = new (stack->Alloc(algn16_size(sizeof(CCharacter2)) + 2)) CCharacter2;
    if (chara == NULL) {
        return NULL;
    }
    chara->Initialize();
    chara->LoadPackNoLine(pack, name, stack, stack, stack, -1, NULL);
    return chara;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", pcp_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_729__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_730__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_731__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_732__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_754__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_807__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_828__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_829__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", at_830__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", __vt__8CMdsInfo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mdslist", __vt__9CMapPiece__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(now_mds_num, 0x4);
INCLUDE_BSS(max_mds_num, 0x4);
INCLUDE_BSS(pcpMdsList, 0x4);
INCLUDE_BSS(pcpMdsInfo, 0x4);
INCLUDE_BSS(pcpNowMdsInfo, 0x4);
INCLUDE_BSS(pcpStack, 0x4);
INCLUDE_BSS(pcp_file, 0x4);
INCLUDE_BSS(pcpAllScissor, 0x4);
