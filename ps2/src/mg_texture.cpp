#include "common.h"

#include <libgraph.h>
#include <libpkt.h>

#include <cstdio>
#include <cstring>

#include "mg_memory.hpp"
#include "mg_tanime.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"

/**
 * Gives the quadwords a buffer of the given bytes takes, rounded up.
 */
#define QWORDS(bytes) ((bytes) % 16 == 0 ? (bytes) / 16 : (bytes) / 16 + 1)

/**
 * Words of the DMA tags, VIF codes and GIF tags the upload chains are
 * built from.
 */
enum {
    DMA_ID_CNT = 0x10000000,   /**< DMA tag transferring the quadwords that follow it. */
    DMA_ID_REF = 0x30000000,   /**< DMA tag transferring quadwords from the address it holds. */
    VIF_DIRECT = 0x50000000,   /**< VIF code passing the quadwords that follow it to the GIF. */
    GIF_EOP = 0x8000,          /**< GIF tag lower word: last tag of the packet. */
    GIF_NREG_1 = 0x10000000,   /**< GIF tag upper word: one register descriptor. */
    GIF_FLG_IMAGE = 0x08000000 /**< GIF tag upper word: image data. */
};

static int Conv32To8(int width, int height, u_char *image);

/**
 * DMA chain flushing the GS texture cache, copied in front of and behind
 * every texture upload.
 */
static u_int texflush_dma[12] __attribute__((aligned(16))) = {
    DMA_ID_CNT | 2, 0, 0, VIF_DIRECT | 2,
    GIF_EOP | 1, GIF_NREG_1, SCE_GIF_PACKED_AD, 0,
    0, 0, SCE_GS_TEXFLUSH, 0,
};

/**
 * Gives the quadwords required for an unsigned byte count.
 */
static inline u_int align16_blocks(u_int bytes) {
    if (bytes & 0xF) {
        return (bytes >> 4) + 1;
    }
    return bytes >> 4;
}

// Keep texture-state updates and packet operations in statement order.

// Code (.text)
#pragma schedule off
/**
 * Gives the VRAM block address of the Z buffer and stores the GS blocks
 * it spans, which 8-bit textures can borrow while it holds nothing.
 */
static int GetZBufVram(int *size) {
    mgCTexture frame;

    mgGetTextureZ(0);
    mgGetFrameBuffer(&frame);
    *size = frame.width * frame.width * 4 / 256;
    return mgZBUF_1.bits.zbp << 5;
}

#pragma schedule reset
#pragma schedule off
/**
 * Tells whether a texture can be placed in the Z buffer's VRAM, which
 * holds unswizzled 8-bit textures only, and stores the blocks it needs.
 */
static int CheckCopyToZBufVram(mgCTexture *texture, int *size) {
    *size = 0;

    if (texture->swizzled == 0 && texture->bpp == 8 && texture->image[0] != NULL) {
        *size = texture->vram_size * 4;
        return 1;
    }

    return 0;
}

#pragma schedule reset
#pragma schedule off
mgCTexture::mgCTexture() {
    Initialize();
}

#pragma schedule reset
// Keep the image-table clearing loop intact.
#pragma optimization_level 2
void mgCTexture::Initialize() {
    int i;

    name[0] = '\0';
    block = -1;

    for (i = 0; i < MG_TEXTURE_LEVEL_MAX; i++) {
        image[i] = NULL;
    }

    clut = NULL;
    *(u_long *)&clamp = 0;
    *(u_long *)&tex1 = 0;
    tex0.value = 0;
    clamp.WMS = 1;
    clamp.WMT = 1;
    bpp = 0;
    height = 0;
    width = 0;
    swizzled = 0;
    vram_size = 0;
    image_blocks = 0;
    clut_size = 0;
    next = NULL;
}
#pragma optimization_level reset
#pragma schedule off
void mgCTexture::Bilinear(int mode) {
    if (tex1.MXL == 0) {
        switch (mode) {
            case MG_TEXTURE_FILTER_NEAREST:
                tex1.MMAG = 0;
                tex1.MMIN = 0;
                break;
            case MG_TEXTURE_FILTER_LINEAR:
            case MG_TEXTURE_FILTER_2:
                tex1.MMAG = 1;
                tex1.MMIN = 1;
                break;
        }
    } else {
        switch (mode) {
            case MG_TEXTURE_FILTER_NEAREST:
                tex1.MMAG = 0;
                tex1.MMIN = 2;
                break;
            case MG_TEXTURE_FILTER_LINEAR:
                tex1.MMAG = 1;
                tex1.MMIN = 4;
            case MG_TEXTURE_FILTER_2:
                tex1.MMAG = 1;
                tex1.MMIN = 5;
                break;
        }
    }
}

#pragma schedule reset
#pragma schedule off
mgCTextureBlock::mgCTextureBlock() {
    Initialize();
}

#pragma schedule reset
#pragma schedule off
void mgCTextureBlock::Initialize() {
    unk_4 = 0;
    unk_0 = 0;
    anime = NULL;
    texture = NULL;
}

#pragma schedule reset
#pragma schedule off
#pragma global_optimizer off
void mgCTextureBlock::Add(mgCTexture *texture) {
    mgCTexture *last;
    mgCTexture *next;

    texture->next = NULL;
    if (this->texture == NULL) {
        this->texture = texture;
    } else {
        for (last = this->texture; last != NULL; last = next) {
            next = last->next;
            if (next == NULL) {
                last->next = texture;
                break;
            }
        }
    }
}

#pragma global_optimizer reset
#pragma schedule reset
#pragma schedule off
#pragma global_optimizer off
void mgCTextureBlock::Delete(mgCTexture *texture) {
    mgCTexture *current = this->texture;
    mgCTexture *previous = NULL;

    for (current = this->texture; current != NULL; current = current->next) {
        if (current == texture) {
            if (previous == NULL) {
                this->texture = current->next;
            } else {
                previous->next = current->next;
            }
        }

        previous = current;
    }
}

#pragma global_optimizer reset
#pragma schedule reset
#pragma schedule off
mgCTextureManager::mgCTextureManager() : texture_max(0), hash_max(0) {
    block_max = 0;
    blocks = NULL;
}

#pragma schedule reset
#pragma schedule off
#pragma global_optimizer off
void mgCTextureManager::SetTableBuffer(int texture_count, int block_count, mgCMemory *memory) {
    int i;
    int hash_index;
    int count;
    u_int bytes;

    block_max = block_count;
    count = block_max;
    bytes = count * sizeof(mgCTextureBlock);
    blocks = new (memory->Alloc(align16_blocks(bytes) + 2)) mgCTextureBlock[count];

    if (blocks == NULL) {
        block_max = 0;
    }

    texture_max = texture_count;
    count = texture_max;
    bytes = count * sizeof(mgCTexture);
    texture_buf = new (memory->Alloc(align16_blocks(bytes) + 2)) mgCTexture[count];
    bytes = texture_max * sizeof(mgCTexture *);
    texture_stack = (mgCTexture **)memory->Alloc(align16_blocks(bytes));

    for (i = 0; i < texture_max; i++) {
        texture_stack[i] = &texture_buf[i];
    }

    texture_num = 0;
    hash_max = texture_count;
    bytes = hash_max * sizeof(mgTEXTURE_HASH);
    hash_buf = new (memory->Alloc(align16_blocks(bytes) + 2)) mgTEXTURE_HASH[hash_max];
    bytes = hash_max * sizeof(mgTEXTURE_HASH *);
    hash_stack = (mgTEXTURE_HASH **)memory->Alloc(align16_blocks(bytes));

    for (hash_index = 0; hash_index < hash_max; hash_index++) {
        hash_stack[hash_index] = &hash_buf[hash_index];
    }

    hash_num = 0;
}

#pragma global_optimizer reset
#pragma schedule reset
// Keep the separate texture and hash-table loops intact.
#pragma optimization_level 2
void mgCTextureManager::Initialize(int vram_top, int vram_fix) {
    int i;
    int texture_index;
    int bucket;
    int hash_index;

    this->vram_top = vram_top;
    this->vram_fix = vram_fix;

    if (this->vram_fix < 0) {
        this->vram_fix = MG_TEXTURE_VRAM_FIX_DEFAULT;
    }

    if (blocks != NULL) {
        for (i = 0; i < block_max; i++) {
            blocks[i].Initialize();
        }

        fix_block.Initialize();

        for (texture_index = 0; texture_index < texture_max; texture_index++) {
            texture_stack[texture_index] = &texture_buf[texture_index];
        }

        texture_num = 0;

        for (i = 0; i < block_max; i++) {
            blocks[i].unk_0 = this->vram_top;
            blocks[i].unk_4 = this->vram_top;
        }

        last_block = -1;
        name_suffix[0] = '\0';

        for (bucket = 0; bucket < MG_TEXTURE_HASH_SIZE; bucket++) {
            hash_table[bucket] = NULL;
        }

        for (hash_index = 0; hash_index < hash_max; hash_index++) {
            hash_stack[hash_index] = &hash_buf[hash_index];
        }

        hash_num = 0;
    }
}
int mgCTextureManager::hash(char *name) {
    u_char key = 0;

    for (; *name != '\0'; name++) {
        key = ((key << 8) + *name) % MG_TEXTURE_HASH_SIZE;
    }

    return key;
}

#pragma optimization_level reset
#ifdef NONMATCHING
void mgCTextureManager::AddHash(mgCTexture *texture) {
    mgTEXTURE_HASH *link;
    mgTEXTURE_HASH *last;
    int key;

    if (hash_num < hash_max) {
        link = hash_stack[hash_num++];
    } else {
        link = NULL;
    }

    if (link == NULL) {
        return;
    }

    link->next = NULL;
    link->texture = texture;
    key = hash(texture->name);

    if (hash_table[key] == NULL) {
        hash_table[key] = link;
        return;
    }

    for (last = hash_table[key]; last != NULL; last = last->next) {
        if (last->next == NULL) {
            last->next = link;
            return;
        }
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", AddHash__17mgCTextureManagerFP10mgCTexture);
#endif
#ifdef NONMATCHING
void mgCTextureManager::DelHash(mgCTexture *texture) {
    mgTEXTURE_HASH *previous;
    mgTEXTURE_HASH *link;
    int key;

    if (texture == NULL) {
        return;
    }

    key = hash(texture->name);
    previous = NULL;

    for (link = hash_table[key]; link != NULL; link = link->next) {
        if (link->texture == texture) {
            break;
        }

        previous = link;
    }

    if (link == NULL) {
        return;
    }

    if (previous == NULL) {
        hash_table[key] = link->next;
    } else {
        previous->next = link->next;
    }

    if (hash_num > 0) {
        hash_stack[--hash_num] = link;
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", DelHash__17mgCTextureManagerFP10mgCTexture);
#endif
// Keep hash-chain traversal in statement order.
#pragma optimization_level 2
mgCTexture *mgCTextureManager::SearchHash(char *name, int block) {
    mgTEXTURE_HASH *link;

    for (link = hash_table[hash(name)]; link != NULL; link = link->next) {
        if (strcmp(name, link->texture->name) == 0 && (block < 0 || link->texture->block == block)) {
            return link->texture;
        }
    }

    return NULL;
}
#pragma optimization_level reset
#pragma schedule off
mgCTexture *mgCTextureManager::SearchTextureName(char *name, int block) {
    char full_name[128];

    if (name_suffix[0] != '\0') {
        strcpy(full_name, name);
        strcat(full_name, name_suffix);
        name = full_name;
    }

    return SearchHash(name, block);
}

#pragma schedule reset
#pragma schedule off
mgCTexture *mgCTextureManager::SearchTexture(char *name) {
    mgCTexture *texture;

    if (texture_num >= texture_max) {
        texture = NULL;
    } else {
        texture = texture_stack[texture_num++];
    }

    if (SearchTextureName(name, -1) != NULL) {
        printf("%s is already used.\n", name);
    }

    return texture;
}

#pragma schedule reset
#pragma optimization_level 1
mgCTexture *mgCTextureManager::GetTexture(char *name, int block) {
    return SearchTextureName(name, block);
}

#pragma optimization_level reset
#pragma schedule off
mgCTextureBlock *mgCTextureManager::GetTextureBlock(int block) {
    if (block == MG_TEXTURE_BLOCK_FIX) {
        return &fix_block;
    }

    if (block < 0 || block >= block_max) {
        return NULL;
    }

    return &blocks[block];
}

#pragma schedule reset
#pragma schedule off
#pragma global_optimizer off
int mgCTextureManager::GetRemainVRAM(int block) {
    mgCTexture *texture;
    int vram;
    int zbuf_remain;
    int size;

    if (block < 0 || block >= block_max) {
        return 0;
    }

    texture = blocks[block].texture;
    vram = vram_top;
    GetZBufVram(&zbuf_remain);

    for (; texture != NULL; texture = texture->next) {
        if (CheckCopyToZBufVram(texture, &size) && size <= zbuf_remain) {
            zbuf_remain -= size;
        } else {
            vram += texture->vram_size;
        }

        if (texture->bpp <= 8) {
            vram += MG_TEXTURE_CLUT_BLOCKS;
        }
    }

    return vram_fix - vram;
}

#pragma global_optimizer reset
#pragma schedule reset
#ifdef NONMATCHING
mgCTexture *mgCTextureManager::EnterTexture(int block, char *name, u_long128 **image, int width, int height, int bpp,
                                            u_long128 *clut, u_long tex1, int swizzled) {
    char full_name[0x20];
    u_long128 *no_image[MG_TEXTURE_LEVEL_MAX];
    mgCTextureBlock *texture_block;
    mgCTexture *texture;
    sceGsTex1 *lod;
    int psm;
    int tw;
    int th;
    int tbw;
    int power;
    int i;
    int level_blocks;
    int last_level;
    int vram;

    strcpy(full_name, name);
    strcat(full_name, name_suffix);

    if (full_name[0] == '\0') {
        return NULL;
    }

    texture_block = GetTextureBlock(block);

    if (texture_block == NULL) {
        return NULL;
    }

    if (image == NULL) {
        image = no_image;

        for (i = 0; i < MG_TEXTURE_LEVEL_MAX; i++) {
            image[i] = NULL;
        }
    }

    texture = SearchTexture(full_name);

    if (texture == NULL) {
        return NULL;
    }

    texture->Initialize();

    switch (bpp) {
        case 32:
            psm = SCE_GS_PSMCT32;
            break;
        case 24:
            psm = SCE_GS_PSMCT24;
            break;
        case 16:
            psm = SCE_GS_PSMCT16;
            break;
        case 8:
            psm = SCE_GS_PSMT8;
            break;
        case 4:
            psm = SCE_GS_PSMT4;
            break;
        default:
            return NULL;
    }

    tw = 0;
    th = 0;

    for (power = width; power >= 2; power /= 2) {
        tw++;
    }

    for (power = 1, i = 0; i < tw; i++) {
        power <<= 1;
    }

    if (width != power) {
        tw++;
    }

    for (power = height; power >= 2; power /= 2) {
        th++;
    }

    for (power = 1, i = 0; i < th; i++) {
        power <<= 1;
    }

    if (height != power) {
        th++;
    }

    tbw = width / 64;

    if (width % 64 != 0) {
        tbw++;
    }

    if (tbw <= 0) {
        tbw = 1;
    }

    // 24-bit pixels occupy 32 bits each in VRAM.
    level_blocks = (bpp == 24 ? 32 : bpp) * width * height / 256 / 8;
    texture->image_blocks = 0;
    last_level = -1;

    if (psm == SCE_GS_PSMT4 || psm == SCE_GS_PSMT8 || psm == SCE_GS_PSMCT16 || psm == SCE_GS_PSMCT32 ||
        psm == SCE_GS_PSMCT24) {
        for (i = 0; i < MG_TEXTURE_LEVEL_MAX; i++) {
            if (image[i] == NULL) {
                last_level = i - 1;
                break;
            }
        }

        for (i = 0; i < last_level + 1; i++) {
            texture->image[i] = image[i];

            if (image[i] != NULL) {
                texture->image_blocks += level_blocks;
            }

            level_blocks /= 4;
        }

        if (image[0] == NULL) {
            texture->image_blocks = level_blocks;
        }

        if (psm == SCE_GS_PSMT8) {
            texture->clut = clut;
            texture->clut_size = MG_TEXTURE_CLUT_BLOCKS;
            texture->tex0.value = SCE_GS_SET_TEX0(0, tbw, psm, tw, th, 1, 0, 0, 0, 0, 0, 1);
        } else if (psm == SCE_GS_PSMT4) {
            texture->clut = clut;
            texture->clut_size = MG_TEXTURE_CLUT_BLOCKS;
            texture->tex0.value = SCE_GS_SET_TEX0(0, tbw, psm, tw, th, 1, 0, 0, 0, 0, 0, 1);
        } else {
            texture->clut = NULL;
            texture->tex0.value = SCE_GS_SET_TEX0(0, tbw, psm, tw, th, 1, 0, 0, 0, 0, 0, 0);
        }
    }

    texture->vram_size = texture->image_blocks;

    if (texture->vram_size % MG_TEXTURE_PAGE_BLOCKS != 0) {
        texture->vram_size += MG_TEXTURE_PAGE_BLOCKS - texture->vram_size % MG_TEXTURE_PAGE_BLOCKS;
    }

    strcpy(texture->name, full_name);
    texture->width = width;
    texture->height = height;
    texture->bpp = bpp;
    texture->block = block;
    texture->swizzled = swizzled;
    *(u_long *)&texture->tex1 = SCE_GS_SET_TEX1(1, 0, 1, 1, 1, 0, 0);

    if (bpp > 0) {
        if (last_level > 0) {
            *(u_long *)&texture->tex1 = SCE_GS_SET_TEX1(0, 2, 1, 5, 1, 0, -120);

            if (tex1 != 0) {
                lod = (sceGsTex1 *)&tex1;
                *(u_long *)&texture->tex1 = SCE_GS_SET_TEX1(0, last_level, 1, 5, 1, lod->L, lod->K);
            }
        } else {
            *(u_long *)&texture->tex1 = SCE_GS_SET_TEX1(0, 0, 1, 1, 1, 0, 0);

            if (tex1 != 0) {
                *(u_long *)&texture->tex1 = SCE_GS_SET_TEX1(0, 0, 1, 1, 1, 0, 0);
            }
        }
    }

    texture_block->Add(texture);
    AddHash(texture);

    // Fixed textures are allocated downwards from the top of VRAM, each palette above its pixels.
    if (block == MG_TEXTURE_BLOCK_FIX) {
        vram = vram_fix;

        if (texture->bpp <= 8) {
            vram -= MG_TEXTURE_CLUT_BLOCKS;
            texture->tex0.CBP = vram;
        }

        texture->tex0.TBP0 = vram - texture->vram_size;
        vram_fix = vram - texture->vram_size;
    }

    if (mgGetPerformanceMeterFlag()) {
        printf("b = %d,%s vram = %d\n", block, texture->name, texture->vram_size);
    }

    return texture;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", EnterTexture__17mgCTextureManagerFiPcPP1iiiP1Uli);
#endif
#pragma optimization_level 2
mgCTexture *mgCTextureManager::EnterTexture(int block, char *name, TM2_head *tm2, int swizzled, int no_image) {
    char tag[5];
    u_long128 *image[MG_TEXTURE_LEVEL_MAX];
    TM2_PICTURE *picture;
    int width;
    int height;
    int bpp;
    u_long128 *clut;
    u_char *level;
    u_char *first;
    int i;

    memcpy(tag, tm2->tag, 4);
    tag[4] = '\0';
    picture = &tm2->picture;
    width = picture->image_width;
    height = picture->image_height;
    switch (picture->image_type) {
        case TIM2_RGB16:
            bpp = 16;
            break;
        case TIM2_RGB24:
            bpp = 24;
            break;
        case TIM2_RGB32:
            bpp = 32;
            break;
        case TIM2_IDTEX4:
            bpp = 4;
            break;
        case TIM2_IDTEX8:
            bpp = 8;
            break;
        default:
            return NULL;
    }
    clut = NULL;
    for (int j = 0; j < MG_TEXTURE_LEVEL_MAX; j++) {
        image[j] = NULL;
    }

    // The pixels follow the picture header; the palette follows every mip level.
    if (no_image == 0) {
        image[0] = (u_long128 *)((u_char *)picture + picture->header_size);
        if (bpp <= 8) {
            clut = (u_long128 *)((u_char *)image[0] + picture->image_size);
        }
        if (picture->mipmap_textures > 1) {
            level = (u_char *)image[0] + picture->mipmap_size[0];
            for (i = 1; i < picture->mipmap_textures; i++) {
                image[i] = (u_long128 *)level;
                level += picture->mipmap_size[i];
            }
        }
    }
    if (swizzled != 0 && bpp == 8 && picture->mipmap_textures == 1) {
        first = (u_char *)image[0];
        if (Conv32To8(width, height, first)) {
            swizzled = 0;
        }
    }
    return EnterTexture(block, name, image, width, height, bpp, clut, picture->tex1, swizzled);
}

#pragma optimization_level reset
#ifdef NONMATCHING
int mgCTextureManager::EnterIMGFile(u_char *img, int block, mgCMemory *stack, mgCEnterIMGInfo *info) {
    mgIMG_FILE_HEADER *file = (mgIMG_FILE_HEADER *)img;
    mgIMG_HEADER swap;
    mgIMG_HEADER *entries;
    mgIMG_HEADER *entry;
    mgIMG1_HEADER *entry1;
    mgCTexture *texture;
    int not_im2;
    int not_im3;
    int spill;
    int current_block;
    int last_block_used;
    int texture_block;
    int swizzled;
    int offset;
    int i;
    u_int a;
    u_int b;

    if (img == NULL) {
        return 0;
    }

    if (memcmp(img, "IM", 2) != 0) {
        return 0;
    }

    not_im2 = memcmp(img, "IM2", 3);
    not_im3 = memcmp(img, "IM3", 3);

    if (info != NULL) {
        for (i = 0; i < MG_TEXTURE_IMG_GROUP_MAX; i++) {
            info->block[i] = -1;
            info->block_num[i] = 0;
        }
    }

    spill = 0;
    current_block = block;
    last_block_used = block;

    if (not_im3 == 0) {
        entries = (mgIMG_HEADER *)(file + 1);

        // Order the pictures by group, leaving texture animation scripts where they are.
        for (a = 0; a < file->num3 - 1; a++) {
            for (b = a + 1; b < file->num3; b++) {
                if (entries[a].block < 0) {
                    entries[a].block = 0;
                }

                if (entries[b].block < 0) {
                    entries[b].block = 0;
                }

                if (entries[b].block < entries[a].block && entries[b].name[0] != '#') {
                    memcpy(&swap, &entries[a], sizeof(mgIMG_HEADER));
                    memcpy(&entries[a], &entries[b], sizeof(mgIMG_HEADER));
                    memcpy(&entries[b], &swap, sizeof(mgIMG_HEADER));
                }
            }
        }

        entry = entries;

        for (a = 0; a < file->num3; a++, entry++) {
            offset = entry->offset;

            if (entry->name[0] == '#') {
                if (block >= 0 && stack != NULL) {
                    LoadCFGFile((char *)img + offset, entry->size, stack, NULL);
                }

                continue;
            }

            swizzled = entry->swizzled;
            texture_block = spill + block + entry->block;

            if (info != NULL && info->block[entry->block] < 0) {
                info->block[entry->block] = texture_block;
                info->block_num[entry->block] = 1;
            }

            texture = EnterTexture(texture_block, entry->name, (TM2_head *)(img + offset), swizzled, entry->no_image);

            if (texture != NULL) {
                entry->swizzled = texture->swizzled;
                texture->clamp = entry->clamp;

                // A block out of VRAM passes the texture on to the following block.
                if (texture_block < block_max - 1 && GetRemainVRAM(texture_block) < 0) {
                    if (info != NULL && info->block[entry->block] >= 0) {
                        info->block_num[entry->block]++;
                    }

                    spill++;
                    blocks[texture->block].Delete(texture);
                    texture->block++;

                    if (texture->block < block_max) {
                        blocks[texture->block].Add(texture);
                    } else {
                        if (texture_num > 0) {
                            texture_stack[--texture_num] = texture;
                        }

                        texture = NULL;
                    }

                    printf("texture over %d:%s\n", block, texture->name);
                }
            }

            if (texture != NULL && texture->block > last_block_used) {
                last_block_used = texture->block;
            }
        }
    } else {
        entry1 = (mgIMG1_HEADER *)(file + 1);

        for (a = 0; a < file->num; a++, entry1++) {
            texture = EnterTexture(current_block, entry1->name, (TM2_head *)(img + entry1->offset), not_im2 == 0, 0);

            if (texture != NULL && current_block < block_max - 1 && GetRemainVRAM(current_block) < 0) {
                current_block++;
                spill++;
                texture->block = current_block;
            }

            if (texture != NULL && texture->block > last_block_used) {
                last_block_used = texture->block;
            }
        }

        if (info != NULL) {
            info->block[0] = block;
            info->block_num[0] = spill + 1;
        }
    }

    return last_block_used - current_block;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", EnterIMGFile__17mgCTextureManagerFPUciP9mgCMemoryP15mgCEnterIMGInfo);
#endif
#pragma schedule off
/**
 * Identifies the archive format of an IMG texture archive from its
 * signature, or gives MG_IMG_VERSION_NONE when the data is not one.
 */
static int GetIMGVersion(char *img) {
    if (img == NULL) {
        return MG_IMG_VERSION_NONE;
    }

    if (memcmp(img, "IM", 2) != 0) {
        return MG_IMG_VERSION_NONE;
    }

    if (memcmp(img, "IMG", 3) == 0) {
        return MG_IMG_VERSION_IMG;
    }

    if (memcmp(img, "IM2", 3) == 0) {
        return MG_IMG_VERSION_IM2;
    }

    if (memcmp(img, "IM3", 3) == 0) {
        return MG_IMG_VERSION_IM3;
    }

    return MG_IMG_VERSION_NONE;
}

#pragma schedule reset
#pragma schedule off
int mgGetIMGHeaderNum(char *img) {
    mgIMG_FILE_HEADER *file = (mgIMG_FILE_HEADER *)img;
    int version;

    if (file == NULL) {
        return 0;
    }

    version = GetIMGVersion(img);

    if (version == MG_IMG_VERSION_NONE) {
        return 0;
    }

    if (version == MG_IMG_VERSION_IMG || version == MG_IMG_VERSION_IM2) {
        return file->num;
    }

    if (version == MG_IMG_VERSION_IM3) {
        return file->num3;
    }

    return 0;
}

#pragma schedule reset
#ifdef NONMATCHING
mgIMG_HEADER mgGetIMGHeader(char *img, int index) {
    mgIMG_HEADER header;
    mgIMG_FILE_HEADER *file = (mgIMG_FILE_HEADER *)img;
    char *entry;
    int version;
    int num;
    int stride;
    int i;

    memset(&header, 0, sizeof(mgIMG_HEADER));

    if (img == NULL) {
        return header;
    }

    version = GetIMGVersion(img);

    if (version == MG_IMG_VERSION_NONE) {
        return header;
    }

    entry = img;
    num = 0;
    stride = 0;

    if (version == MG_IMG_VERSION_IMG || version == MG_IMG_VERSION_IM2) {
        entry = (char *)(file + 1);
        num = file->num;
        stride = sizeof(mgIMG1_HEADER);
    }

    if (version == MG_IMG_VERSION_IM3) {
        entry = (char *)(file + 1);
        num = file->num3;
        stride = sizeof(mgIMG_HEADER);
    }

    for (i = 0; i < num; i++, entry += stride) {
        if (index != i) {
            continue;
        }

        switch (version) {
            case MG_IMG_VERSION_IM3:
                header = *(mgIMG_HEADER *)entry;
                break;
            case MG_IMG_VERSION_IM2:
                header.swizzled = 1;
            case MG_IMG_VERSION_IMG:
                memcpy(header.name, entry, sizeof(header.name));
                header.offset = ((mgIMG1_HEADER *)entry)->offset;
                break;
        }
    }

    return header;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", mgGetIMGHeader__FPci);
#endif
#pragma schedule off
void mgCTextureManager::DeleteTexture(mgCTexture *texture) {
    mgCTextureBlock *texture_block;

    if (texture != NULL) {

        texture_block = GetTextureBlock(texture->block);

        if (texture_block != NULL) {

            DelHash(texture);
            texture_block->Delete(texture);

            if (texture_num > 0) {
                texture_num--;
                texture_stack[texture_num] = texture;
            }

            texture->Initialize();
        }
    }
}

#pragma schedule reset
#pragma schedule off
void mgCTextureManager::DeleteTexture(char *name, int block) {
    mgCTexture *texture = GetTexture(name, block);

    if (texture != NULL) {
        DeleteTexture(texture);
    }
}

#pragma schedule reset
#pragma schedule off
#pragma global_optimizer off
void mgCTextureManager::DeleteBlock(int block) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTexture *texture;
    mgCTexture *next;

    if (texture_block == NULL) {
        return;
    }

    for (texture = texture_block->texture; texture != NULL; texture = next) {
        next = texture->next;
        DeleteTexture(texture);
    }

    texture_block->Initialize();
}

#pragma global_optimizer reset
#pragma schedule reset
#pragma optimization_level 1
void mgCTextureManager::EndEnterTexture(int block) {
    ReloadTexture(block, (u_int *)NULL);
}

#pragma optimization_level reset
#pragma optimization_level 2
int mgLoadImage(u_int *packet, int dbp, int dpsm, int dbw, u_long128 *image, int qwc,
                int x, int y, int w, int h) {
    u_int *start = packet;
    unsigned long long bitbltbuf;
    unsigned long long trxpos;
    unsigned long long trxreg;
    int count;
    // The transfer setup: TEXFLUSH, BITBLTBUF, TRXPOS, TRXREG and TRXDIR as A+D pairs.
    packet[0] = DMA_ID_CNT | 6;
    packet[1] = 0;
    packet[2] = 0;
    packet[3] = VIF_DIRECT | 6;
    packet[4] = 5;
    packet[5] = GIF_NREG_1;
    packet[6] = SCE_GIF_PACKED_AD;
    packet[7] = 0;
    packet[8] = 0;
    packet[9] = 0;
    packet[10] = SCE_GS_TEXFLUSH;
    packet[11] = 0;
    bitbltbuf = ((long long)dpsm << 56) | (((long long)dbp << 32) | ((long long)dbw << 48));
    packet[12] = (u_int)(bitbltbuf & 0xFFFFFFFFULL);
    packet[13] = (u_int)((bitbltbuf >> 32) & 0xFFFFFFFFULL);
    packet[14] = SCE_GS_BITBLTBUF;
    packet[15] = 0;
    trxpos = ((long long)x << 32) | ((long long)y << 48);
    packet[16] = (u_int)(trxpos & 0xFFFFFFFFULL);
    packet[17] = (u_int)((trxpos >> 32) & 0xFFFFFFFFULL);
    packet[18] = SCE_GS_TRXPOS;
    packet[19] = 0;
    trxreg = w | ((long long)h << 32);
    packet[20] = (u_int)(trxreg & 0xFFFFFFFFULL);
    packet[21] = (u_int)((trxreg >> 32) & 0xFFFFFFFFULL);
    packet[22] = SCE_GS_TRXREG;
    packet[23] = 0;
    packet[24] = SCE_GS_SET_TRXDIR(0);
    packet[25] = 0;
    packet[26] = SCE_GS_TRXDIR;
    packet[27] = 0;
    // The pixels follow by reference, in transfers of at most 0x4000 quadwords.
    packet += 28;
    while (qwc > 0) {
        count = 0x4000;
        if (qwc < 0x4000) {
            count = qwc;
        }
        packet[0] = DMA_ID_CNT | 1;
        packet[1] = 0;
        packet[2] = 0;
        packet[3] = VIF_DIRECT | 1;
        packet[4] = count | GIF_EOP;
        packet[5] = GIF_FLG_IMAGE;
        packet[6] = 0;
        packet[7] = 0;
        packet[8] = count | DMA_ID_REF;
        packet[9] = (u_int)image;
        packet[10] = 0;
        packet[11] = count | VIF_DIRECT;
        packet += 12;
        image += count;
        qwc -= 0x4000;
    }
    return packet - start;
}

#pragma optimization_level reset
#pragma schedule off
/**
 * Writes the DMA chain flushing the GS texture cache, when given a
 * buffer; gives the quadwords it takes either way.
 */
static int SetTexFlush_TagCnt(u_int *packet) {
    if (packet == NULL) {
        return 3;
    }
    u_long128 *cursor = (u_long128 *)packet;
    cursor[0] = *(u_long128 *)&texflush_dma[0];
    cursor[1] = *(u_long128 *)&texflush_dma[4];
    cursor[2] = *(u_long128 *)&texflush_dma[8];
    return 3;
}

#pragma schedule reset
#ifdef NONMATCHING
void mgCTextureManager::ReloadTexture(int block, sceVif1Packet *packet) {
    int loaded;
    int qwc;
    mgCTextureAnime *anime;

    if (block < 0 || block >= block_max) {
        last_block = -1;
        return;
    }

    loaded = last_block;

    if (packet == NULL) {
        packet = mgVif1Packet;
    }

    sceVif1PkTerminate(packet);
    qwc = ReloadTexture(block, packet->pCurrent);
    sceVif1PkReserve(packet, qwc * 16 / 4);

    if (loaded != block) {
        anime = blocks[block].anime;

        if (anime != NULL) {
            anime->TexAnime(block, packet);
        }
    }

    last_block = block;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", ReloadTexture__17mgCTextureManagerFiP13sceVif1Packet);
#endif
#ifdef NONMATCHING
int mgCTextureManager::ReloadTexture(int block, u_int *packet) {
    sceGsTex0 tex0;
    mgCTexture *texture;
    u_int *cursor;
    int vram;
    int fix;
    int zbuf;
    int zbuf_size;
    int zbuf_end;
    int size;
    int to_zbuf;
    int width;
    int height;
    int bpp;
    int level;

    if (packet != NULL && (block < 0 || block >= block_max)) {
        last_block = -1;
        return 0;
    }

    cursor = packet;

    if (cursor != NULL) {
        cursor += SetTexFlush_TagCnt(cursor) * 4;
    }

    vram = vram_top;
    fix = vram_fix;
    zbuf = GetZBufVram(&zbuf_size);
    zbuf_end = zbuf + zbuf_size;

    if (last_block != block) {
        for (texture = blocks[block].texture; texture != NULL; texture = texture->next) {
            width = texture->width;
            height = texture->height;
            bpp = texture->bpp;
            to_zbuf = 0;

            if (CheckCopyToZBufVram(texture, &size) && zbuf + size < zbuf_end) {
                to_zbuf = 1;
            }

            // An 8-bit texture in the Z buffer's VRAM sits in the upper byte of each 32-bit pixel.
            if (to_zbuf) {
                texture->tex0.TBP0 = zbuf;
                texture->tex0.PSM = SCE_GS_PSMT8H;
                zbuf += texture->vram_size * 4;
            } else {
                texture->tex0.TBP0 = vram;
                vram += texture->vram_size;

                if (bpp == 8) {
                    texture->tex0.PSM = SCE_GS_PSMT8;
                }
            }

            tex0 = texture->tex0;

            if (texture->bpp <= 8) {
                fix -= MG_TEXTURE_CLUT_BLOCKS;
                texture->tex0.CBP = fix;
            }

            // Swizzled 8-bit pixels are uploaded as a 32-bit image of half the size.
            if (texture->swizzled != 0 && bpp == 8) {
                width >>= 1;
                height >>= 1;
                bpp = 32;
                tex0.PSM = SCE_GS_PSMCT32;
                tex0.TBW = tex0.TBW >> 1;
            }

            if (cursor != NULL) {
                cursor += ReloadCLUT(texture, cursor);
            }

            for (level = 0; level < MG_TEXTURE_LEVEL_MAX && texture->image[level] != NULL; level++) {
                if (tex0.TBW == 0) {
                    tex0.TBW = 1;
                }

                if (cursor != NULL) {
                    cursor += mgLoadImage(cursor, tex0.TBP0, tex0.PSM, tex0.TBW, texture->image[level],
                                          bpp * width * height / 16 / 8, 0, 0, width, height);
                }

                tex0.TBP0 = tex0.TBP0 + bpp * width * height / 256 / 8;
                tex0.TBW = tex0.TBW >> 1;
                width >>= 1;
                height >>= 1;
            }
        }
    }

    if (cursor != NULL) {
        cursor += SetTexFlush_TagCnt(cursor) * 4;
        last_block = block;
    }

    return (cursor - packet) / 4;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", ReloadTexture__17mgCTextureManagerFiPUi);
#endif
#ifdef NONMATCHING
// sceGsTex0::operator= is the compiler-generated copy assignment of the SDK type (sce/libgraph.h).
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", __as__9sceGsTex0FRC9sceGsTex0);
#endif
#pragma schedule off
int mgCTextureManager::ReloadCLUT(mgCTexture *texture, u_int *packet) {
    int words;

    if (texture == NULL) {
        return 0;
    }

    if (texture->clut == NULL) {
        return 0;
    }

    words = 0;

    if (texture->bpp == 8) {
        words = mgLoadImage(packet, texture->tex0.CBP, texture->tex0.CPSM, 1, texture->clut, 0x40, 0, 0, 16, 16);
    } else if (texture->bpp == 4) {
        words = mgLoadImage(packet, texture->tex0.CBP, texture->tex0.CPSM, 1, texture->clut, 4, 0, 0, 8, 2);
    }

    return words;
}

#pragma schedule reset
#pragma schedule off
void mgCTextureManager::ReloadCLUT(mgCTexture *texture, sceVif1Packet *packet) {
    u_int *cursor;
    u_int *start;

    if (packet == NULL) {
        packet = mgVif1Packet;
    }

    sceVif1PkTerminate(packet);
    cursor = packet->pCurrent;
    start = cursor;
    cursor += ReloadCLUT(texture, cursor);
    cursor += SetTexFlush_TagCnt(cursor) * 4;
    sceVif1PkReserve(packet, cursor - start);
}

#pragma schedule reset
#pragma schedule off
void mgCTextureManager::TexAnimeOn(int block, char *group_name) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTextureAnime *anime;

    if (texture_block != NULL && (anime = texture_block->anime) != NULL) {
        anime->Enable(anime->SearchGroupName(group_name));
    }
}

#pragma schedule reset
#pragma schedule off
void mgCTextureManager::TexAnimeOff(int block, char *group_name) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTextureAnime *anime;

    if (texture_block != NULL && (anime = texture_block->anime) != NULL) {
        anime->Disable(anime->SearchGroupName(group_name));
    }
}

#pragma schedule reset
#pragma schedule off
void mgCTextureManager::TexAnimeAllOff(int block) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTextureAnime *anime;

    if (texture_block != NULL && (anime = texture_block->anime) != NULL) {
        anime->DisableAll();
    }
}

char **mgCTextureManager::GetGroupNameList(int block, int *num) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTextureAnime *anime;

    if (texture_block == NULL) {
        return NULL;
    }

    anime = texture_block->anime;

    if (anime == NULL) {
        return NULL;
    }

    *num = MG_TEX_ANIME_GROUP_MAX;
    return anime->name;
}

void mgCTextureManager::DeleteTexAnimeGroup(int block, int group) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);
    mgCTextureAnime *anime;

    if (texture_block != NULL && (anime = texture_block->anime) != NULL) {
        anime->DeleteGroup(group);
    }
}

void mgCTextureManager::DeleteTexAnime(int block) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);

    if (texture_block != NULL) {
        texture_block->anime = NULL;
    }
}

#pragma schedule reset
#pragma schedule off
mgCTextureAnime *mgCTextureManager::GetTexAnime(int block) {
    mgCTextureBlock *texture_block = GetTextureBlock(block);

    if (texture_block != NULL) {
        return texture_block->anime;
    }
    return NULL;
}

#pragma schedule reset
#pragma optimization_level 1
/**
 * Reorders one block of 8-bit pixels stored in 32-bit page order back
 * into linear order.
 */
static int BlockConv32to8(u_char *source, u_char *destination) {
    // Each block is four columns of 64 bytes; odd columns use the second half of the table.
    static u_char lut[128] = {
        0,  36, 8,  44, 1,  37, 9,  45, 2,  38, 10, 46, 3,  39, 11, 47, 4,  32, 12, 40, 5,  33,
        13, 41, 6,  34, 14, 42, 7,  35, 15, 43, 16, 52, 24, 60, 17, 53, 25, 61, 18, 54, 26, 62,
        19, 55, 27, 63, 20, 48, 28, 56, 21, 49, 29, 57, 22, 50, 30, 58, 23, 51, 31, 59, 4,  32,
        12, 40, 5,  33, 13, 41, 6,  34, 14, 42, 7,  35, 15, 43, 0,  36, 8,  44, 1,  37, 9,  45,
        2,  38, 10, 46, 3,  39, 11, 47, 20, 48, 28, 56, 21, 49, 29, 57, 22, 50, 30, 58, 23, 51,
        31, 59, 16, 52, 24, 60, 17, 53, 25, 61, 18, 54, 26, 62, 19, 55, 27, 63,
    };
    u_int row;
    u_int pixel;
    u_int column;
    int lut_index;
    int source_index;

    source_index = 0;

    for (column = 0; column < 4; column++) {
        lut_index = (column & 1) * 64;

        for (row = 0; row < 16; row++) {
            for (pixel = 0; pixel < 4; pixel++) {
                u_char destination_index = lut[lut_index];
                lut_index++;
                destination[destination_index] = source[source_index];
                source_index++;
            }
        }

        destination += 64;
    }

    return 0;
}

#pragma optimization_level reset
#pragma optimization_level 2
/**
 * Reorders one page of 8-bit pixels stored in 32-bit page order back
 * into linear order, block by block.
 */
static int PageConv32to8(int width, int height, u_char *source, u_char *destination) {
    static int block_table8[32] = {
        0, 1, 4,  5,  16, 17, 20, 21, 2,  3,  6,  7,  18, 19, 22, 23,
        8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31,
    };
    static int block_table32[32] = {
        0, 1, 4,  5,  16, 17, 20, 21, 2,  3,  6,  7,  18, 19, 22, 23,
        8, 9, 12, 13, 24, 25, 28, 29, 10, 11, 14, 15, 26, 27, 30, 31,
    };
    int block_column[32];
    int block_row[32];
    u_char work8[0x100];
    u_char work32[0x100];
    int entry;
    int i;
    int j;
    int k;
    int blocks_wide;
    u_char *work_cursor;
    u_char *source_cursor;
    u_char *destination_cursor;
    int blocks_high;
    int block_index;

    entry = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            block_column[block_table32[entry]] = j;
            block_row[block_table32[entry]] = i;
            entry++;
        }
    }
    blocks_wide = width / 16;
    blocks_high = height / 16;
    memset(work8, 0, sizeof(work8));
    memset(work32, 0, sizeof(work32));
    for (i = 0; i < blocks_high; i++) {
        for (j = 0; j < blocks_wide; j++) {
            block_index = block_table8[j + i * blocks_wide];
            work_cursor = work32;
            source_cursor = source + block_row[block_index] * 2048 + block_column[block_index] * 32;
            for (k = 0; k < 8; k++) {
                memcpy(work_cursor, source_cursor, 32);
                work_cursor += 32;
                source_cursor += 256;
            }
            BlockConv32to8(work32, work8);
            work_cursor = work8;
            destination_cursor = destination + i * 2048 + j * 0x10;
            for (k = 0; k < 0x10; k++) {
                memcpy(destination_cursor, work_cursor, 0x10);
                work_cursor += 0x10;
                destination_cursor += 0x80;
            }
        }
    }
    return 0;
}

#pragma optimization_level reset
#ifdef NONMATCHING
/**
 * Converts 8-bit pixels stored in 32-bit page order back into linear
 * order in place; gives 0 when the image is too large to convert.
 */
static int Conv32To8(int width, int height, u_char *image) {
    static u_char conv_work[0x10000];
    u_char work8[0x2000];
    u_char work32[0x2000];
    u_char *source_cursor;
    u_char *work_cursor;
    u_char *destination_cursor;
    int size;
    int pages_x;
    int pages_y;
    int row_bytes;
    int row_count;
    int i;
    int j;
    int k;

    size = width * height;

    if (size > 0x10000) {
        return 0;
    }

    memset(work8, 0, sizeof(work8));
    memset(work32, 0, sizeof(work32));
    pages_x = (width - 1) / 128 + 1;
    pages_y = (height - 1) / 64 + 1;

    if (pages_x == 1) {
        row_bytes = width * 2;
    } else {
        width = 128;
        row_bytes = 256;
    }

    if (pages_y == 1) {
        row_count = height / 2;
    } else {
        height = 64;
        row_count = 32;
    }

    for (i = 0; i < pages_y; i++) {
        for (j = 0; j < pages_x; j++) {
            source_cursor = image + row_bytes * j + i * pages_x * row_bytes * row_count;
            work_cursor = work32;

            for (k = 0; k < row_count; k++) {
                memcpy(work_cursor, source_cursor, row_bytes);
                source_cursor += row_bytes * pages_x;
                work_cursor += 256;
            }

            PageConv32to8(128, 64, work32, work8);
            destination_cursor = conv_work + width * j + i * pages_x * width * 64;
            work_cursor = work8;

            for (k = 0; k < height; k++) {
                memcpy(destination_cursor, work_cursor, width);
                destination_cursor += width * pages_x;
                work_cursor += 128;
            }
        }
    }

    memcpy(image, conv_work, size);
    return 1;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mg_texture", Conv32To8__FiiPUc);
#endif

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", texflush_dma__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", lut_1246__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", block_table8_1266__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", block_table32_1267__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_497__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_629__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_866__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_867__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_868__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_869__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mg_texture", at_884__DATA);

// Uninitialised data (.bss)
INCLUDE_BSS(conv_work_1306, 0x10000);
