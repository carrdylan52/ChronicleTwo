#include "common.h"
#include "effscript.hpp"
#include "mg_memory.hpp"
#include "mg_drawprim.hpp"
#include "mg_texture.hpp"
#include "mg_frame.hpp"
#include "mg_drawenv.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "actionchara.hpp"
#include "scene.hpp"
#include "scenesnd.hpp"
#include "object.hpp"
#include "padcontrol.hpp"
#include "cameracontrol.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include "character.hpp"
#include "colprim.hpp"
#include "mainloop.hpp"
#include "dataread.hpp"
#include "snd_mngr.hpp"
#include "event_func.hpp"
#include "dng_main.hpp"
#include "runscript_opcodes.hpp"

static void SetEffectScriptFunc();
static _EFF_SCRIPT *now_script; /**< Effect whose script is running. */
CScene *now_scene; /**< Scene used by the effect script manager. */
CEffectScriptMan *EffScriptMan; /**< Manager of the running effect. */
static int (*ext_func[256])(RS_STACKDATA *, int); /**< Effect handlers indexed by script number. */

static EFF_SPT_BASE_DEF *GetEffSptBaseDefPtr(int index);
static _ES_SPRITE *GetSpritePtr(_EFF_SCRIPT *script, int index);
static int GetStackInt(RS_STACKDATA *slot);
static float GetStackFloat(RS_STACKDATA *slot);
static void GetStackVector(float *vector, RS_STACKDATA *slot);
static char *GetStackString(RS_STACKDATA *slot);
static void SetStack(RS_STACKDATA *slot, int value);
static void SetStack(RS_STACKDATA *slot, float value);
static int _ZERO_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _NORMAL_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _COPY_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _ADD_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _SUB_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _SCALE_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _DIV_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _DIST_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _DIST_VECTOR2(RS_STACKDATA *stack, int argument_count);
static int _SQRT(RS_STACKDATA *stack, int argument_count);
static int _ATAN2F(RS_STACKDATA *stack, int argument_count);
static int _ANGLE_CMP(RS_STACKDATA *stack, int argument_count);
static int _ANGLE_LIMIT(RS_STACKDATA *stack, int argument_count);
static int _GET_RAND(RS_STACKDATA *stack, int argument_count);
static int _GET_REF_ROT(RS_STACKDATA *stack, int argument_count);
static int _GET_DIR_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _SET_ORIGIN(RS_STACKDATA *stack, int argument_count);
static int _GET_ORIGIN(RS_STACKDATA *stack, int argument_count);
static int _AUTO_SET_OFFSET(RS_STACKDATA *stack, int argument_count);
static int _GET_WORK_VECT1(RS_STACKDATA *stack, int argument_count);
static int _GET_WORK_VECT2(RS_STACKDATA *stack, int argument_count);
static int _GET_TARGET_ID(RS_STACKDATA *stack, int argument_count);
static int _GET_USER_ID(RS_STACKDATA *stack, int argument_count);
static int _GET_VALUE(RS_STACKDATA *stack, int argument_count);
static int _SET_VALUE(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_SHOW(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_SHOW(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_POS(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_POS(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_ROT(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_ROT(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_SCALE(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_SCALE(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_MOTION(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_MOT_STEP(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_MOT_WAIT(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_DIR_VECTOR(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_REF_ROT(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_POS(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_ROT(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_SCALE(RS_STACKDATA *stack, int argument_count);
static int _CHR_COPY_CHARA(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_POS2(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_ROT2(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_SCALE2(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_MOTION2(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_POS2(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_ROT2(RS_STACKDATA *stack, int argument_count);
static int _CHR_ADD_SCALE2(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_SHOW2(RS_STACKDATA *stack, int argument_count);
static int _CHR_GET_FRAME_POS(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_FRAME_SHOW(RS_STACKDATA *stack, int argument_count);
static int _CHR_CHK_MOT_END(RS_STACKDATA *stack, int argument_count);
static int _CHR_SET_LIGHT_COLOR(RS_STACKDATA *stack, int argument_count);
static int _SPT_ASSIGN_SPRITE(RS_STACKDATA *stack, int argument_count);
static int _SPT_DELETE_SPRITE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_TEXNAME(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_ALPHAB(RS_STACKDATA *stack, int argument_count);
static int _SPT_INIT_SPRITE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_DRAW_FLAG(RS_STACKDATA *stack, int argument_count);
static int _SPT_GET_DRAW_FLAG(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_UV_SIZE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_PUT_SIZE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_GET_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_ROTZ(RS_STACKDATA *stack, int argument_count);
static int _SPT_GET_ROTZ(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_SCALE(RS_STACKDATA *stack, int argument_count);
static int _SPT_GET_SCALE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_COLOR(RS_STACKDATA *stack, int argument_count);
static int _SPT_GET_COLOR(RS_STACKDATA *stack, int argument_count);
static int _SPT_ADD_ROTZ(RS_STACKDATA *stack, int argc);
static int _SPT_WORLD_ROT(RS_STACKDATA *stack, int argc);
static s32 _SPT_SET_LIFE(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_VELO_ROTZ(RS_STACKDATA *stack, int argc);
static int _SPT_SET_ACC_ROTZ(RS_STACKDATA *stack, int argc);
static int _SPT_SET_VELO_SCL(RS_STACKDATA *stack, int argc);
static int _SPT_SET_ACC_SCL(RS_STACKDATA *stack, int argc);
static int _SPT_SCALE_CONV(RS_STACKDATA *stack, int argc);
static int _SCN_GET_CHR_POS(RS_STACKDATA *stack, int argc);
static int _SCN_GET_CHR_ROT(RS_STACKDATA *stack, int argc);
static int _SCN_GET_CHR_FRM_POS(RS_STACKDATA *stack, int argc);
static int _SCN_GET_ENTRY_OBJ_POS(RS_STACKDATA *stack, int argc);
static int _MON_SE_PLAY(RS_STACKDATA *stack, int argc);
static int _MON_SE_STOP(RS_STACKDATA *stack, int argc);
static int _BTL_SE_PLAY(RS_STACKDATA *stack, int argc);
static int _BTL_SE_STOP(RS_STACKDATA *stack, int argc);
static int _BSE_SE_PLAY(RS_STACKDATA *stack, int argc);
static int _BSE_SE_STOP(RS_STACKDATA *stack, int argc);
static int _MON_SE_PLAY2(RS_STACKDATA *stack, int argc);
static int _MON_SE_STOP2(RS_STACKDATA *stack, int argc);
static int _SET_LIGHT_FLAG(RS_STACKDATA *stack, int argc);
static int _SCN_GET_CHR_ENTOBJ_POS(RS_STACKDATA *stack, int argc);
static int _CREATE_DAMAGE(RS_STACKDATA *stack, int argc);
static s32 _DELETE_DAMAGE(RS_STACKDATA *stack, int argument_count);
static s32 _DMG_SET_POS(RS_STACKDATA *stack, int argument_count);
static s32 _DMG_SET_FRONT_VECT(RS_STACKDATA *stack, int argument_count);
static int _DMG_SET_DAMAGE(RS_STACKDATA *stack, int argc);
static int _COLPRIM_CREATE(RS_STACKDATA *stack, int argc);
static int _COLPRIM_SET_COORD(RS_STACKDATA *stack, int argc);
static int _COLPRIM_DELETE(RS_STACKDATA *stack, int argc);
static int _COLPRIM_GET_HITCNT(RS_STACKDATA *stack, int argc);
static int _COLPRIM_GET_GIFT(RS_STACKDATA *stack, int argc);
static int _COLPRIM_GET_REVCNT(RS_STACKDATA *stack, int argc);
static int _COLPRIM_SET_DAMAGE(RS_STACKDATA *stack, int argc);
static int _COLPRIM_GET_HIT_POS(RS_STACKDATA *stack, int argc);
static int _ES_CREATE(RS_STACKDATA *stack, int argc);
static int _ES_SET_VECT1(RS_STACKDATA *stack, int argc);
static int _ES_SET_VECT2(RS_STACKDATA *stack, int argc);
static int _ES_SET_TARGET_ID(RS_STACKDATA *stack, int argc);
static int _ES_SET_VALUE(RS_STACKDATA *stack, int argc);
static int _ES_SET_COLPRIM(RS_STACKDATA *stack, int argc);
static int _GET_EOH_POS(RS_STACKDATA *stack, int argc);
static int SetEffectScript(CRunScript *script, char *program, mgCMemory *memory);
static int _SPT_VAN_SET_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_VAN_SET_ROT(RS_STACKDATA *stack, int argument_count);
static int _SPT_ADD_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_ADD_COLOR(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_VELO_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_ACC_POS(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_VELO_COL(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_ACC_COL(RS_STACKDATA *stack, int argument_count);
static int _SPT_SET_BLINKING(RS_STACKDATA *stack, int argument_count);
static int _SPT_VAN_SET_COL(RS_STACKDATA *stack, int argument_count);
static int _SPT_VAN_SET_SCL(RS_STACKDATA *stack, int argument_count);
static int _SPT_COLOR_CONV(RS_STACKDATA *stack, int argument_count);
static int _INTERSECTION_POINT(RS_STACKDATA *stack, int argument_count);
static int _SCN_GET_CHR_FRM_DIR(RS_STACKDATA *stack, int argument_count);
static int _SCN_GET_CHR_FRM_ROT(RS_STACKDATA *stack, int argument_count);

/** Associates effect-script function numbers with their handlers. */
static RS_EXTFUNC_INFO ext_func_info[] = {
    { _ZERO_VECTOR, 0 },
    { _NORMAL_VECTOR, 1 },
    { _COPY_VECTOR, 2 },
    { _ADD_VECTOR, 3 },
    { _SUB_VECTOR, 4 },
    { _SCALE_VECTOR, 5 },
    { _DIV_VECTOR, 6 },
    { _DIST_VECTOR, 7 },
    { _DIST_VECTOR2, 8 },
    { _SQRT, 9 },
    { _ATAN2F, 10 },
    { _ANGLE_CMP, 11 },
    { _ANGLE_LIMIT, 12 },
    { _GET_RAND, 13 },
    { _GET_REF_ROT, 14 },
    { _GET_DIR_VECTOR, 15 },
    { _SET_ORIGIN, 50 },
    { _GET_ORIGIN, 51 },
    { _AUTO_SET_OFFSET, 52 },
    { _GET_WORK_VECT1, 53 },
    { _GET_WORK_VECT2, 54 },
    { _GET_TARGET_ID, 55 },
    { _GET_USER_ID, 56 },
    { _GET_VALUE, 57 },
    { _SET_VALUE, 58 },
    { _CHR_SET_SHOW, 100 },
    { _CHR_GET_SHOW, 101 },
    { _CHR_SET_POS, 102 },
    { _CHR_GET_POS, 103 },
    { _CHR_SET_ROT, 104 },
    { _CHR_GET_ROT, 105 },
    { _CHR_SET_SCALE, 106 },
    { _CHR_GET_SCALE, 107 },
    { _CHR_SET_MOTION, 108 },
    { _CHR_SET_MOT_STEP, 109 },
    { _CHR_GET_DIR_VECTOR, 110 },
    { _CHR_GET_REF_ROT, 111 },
    { _CHR_ADD_POS, 112 },
    { _CHR_ADD_ROT, 113 },
    { _CHR_ADD_SCALE, 114 },
    { _CHR_COPY_CHARA, 125 },
    { _CHR_SET_POS2, 126 },
    { _CHR_SET_ROT2, 127 },
    { _CHR_SET_SCALE2, 128 },
    { _CHR_SET_MOTION2, 129 },
    { _CHR_ADD_POS2, 130 },
    { _CHR_ADD_ROT2, 131 },
    { _CHR_ADD_SCALE2, 132 },
    { _CHR_GET_MOT_WAIT, 133 },
    { _CHR_SET_SHOW2, 134 },
    { _CHR_GET_FRAME_POS, 135 },
    { _CHR_SET_FRAME_SHOW, 252 },
    { _CHR_CHK_MOT_END, 253 },
    { _CHR_SET_LIGHT_COLOR, 136 },
    { _SPT_ASSIGN_SPRITE, 150 },
    { _SPT_DELETE_SPRITE, 151 },
    { _SPT_SET_TEXNAME, 152 },
    { _SPT_SET_ALPHAB, 153 },
    { _SPT_INIT_SPRITE, 154 },
    { _SPT_SET_UV_SIZE, 157 },
    { _SPT_SET_PUT_SIZE, 158 },
    { _SPT_SET_DRAW_FLAG, 155 },
    { _SPT_GET_DRAW_FLAG, 156 },
    { _SPT_SET_POS, 159 },
    { _SPT_GET_POS, 160 },
    { _SPT_SET_ROTZ, 161 },
    { _SPT_GET_ROTZ, 162 },
    { _SPT_SET_SCALE, 163 },
    { _SPT_GET_SCALE, 164 },
    { _SPT_SET_COLOR, 165 },
    { _SPT_GET_COLOR, 166 },
    { _SPT_VAN_SET_POS, 167 },
    { _SPT_VAN_SET_ROT, 168 },
    { _SPT_ADD_POS, 169 },
    { _SPT_ADD_ROTZ, 170 },
    { _SPT_ADD_COLOR, 171 },
    { _SPT_WORLD_ROT, 172 },
    { _SPT_SET_LIFE, 173 },
    { _SPT_SET_VELO_POS, 174 },
    { _SPT_SET_ACC_POS, 175 },
    { _SPT_SET_VELO_ROTZ, 176 },
    { _SPT_SET_ACC_ROTZ, 177 },
    { _SPT_SET_VELO_COL, 178 },
    { _SPT_SET_ACC_COL, 179 },
    { _SPT_SET_BLINKING, 180 },
    { _SPT_VAN_SET_COL, 181 },
    { _SPT_SET_VELO_SCL, 182 },
    { _SPT_SET_ACC_SCL, 183 },
    { _SPT_VAN_SET_SCL, 184 },
    { _SPT_SCALE_CONV, 185 },
    { _SPT_COLOR_CONV, 186 },
    { _SCN_GET_CHR_POS, 200 },
    { _SCN_GET_CHR_ROT, 201 },
    { _SCN_GET_CHR_FRM_POS, 202 },
    { _INTERSECTION_POINT, 203 },
    { _MON_SE_PLAY, 220 },
    { _MON_SE_STOP, 221 },
    { _BTL_SE_PLAY, 222 },
    { _BTL_SE_STOP, 223 },
    { _BSE_SE_PLAY, 224 },
    { _BSE_SE_STOP, 225 },
    { _MON_SE_PLAY2, 226 },
    { _MON_SE_STOP2, 227 },
    { _SET_LIGHT_FLAG, 230 },
    { _SCN_GET_CHR_ENTOBJ_POS, 231 },
    { _SCN_GET_CHR_FRM_DIR, 232 },
    { _SCN_GET_CHR_FRM_ROT, 233 },
    { _SCN_GET_ENTRY_OBJ_POS, 234 },
    { _CREATE_DAMAGE, 204 },
    { _DELETE_DAMAGE, 205 },
    { _DMG_SET_POS, 206 },
    { _DMG_SET_FRONT_VECT, 207 },
    { _DMG_SET_DAMAGE, 208 },
    { _COLPRIM_CREATE, 210 },
    { _COLPRIM_SET_COORD, 211 },
    { _COLPRIM_DELETE, 212 },
    { _COLPRIM_GET_HITCNT, 213 },
    { _COLPRIM_SET_DAMAGE, 214 },
    { _COLPRIM_GET_HIT_POS, 215 },
    { _COLPRIM_GET_GIFT, 216 },
    { _COLPRIM_GET_REVCNT, 217 },
    { _ES_CREATE, 240 },
    { _ES_SET_VECT1, 241 },
    { _ES_SET_VECT2, 242 },
    { _ES_SET_TARGET_ID, 243 },
    { _ES_SET_COLPRIM, 244 },
    { _ES_SET_VALUE, 245 },
    { _GET_EOH_POS, 251 },
    { NULL, -1 },
};

static inline u_int align16_blocks(u_int size) {
    if (size & 0xF) {
        return (size >> 4) + 1;
    }
    return size >> 4;
}

// Code (.text)
void CEffectScriptMan::Initialize(mgCMemory *memory, int texb_start, int texb_num) {
    int                i;
    int                j;
    mgCTextureManager *manager;
    int                bank;

    this->memory = memory;
    load_buffer = NULL;
    work_memory = NULL;
    level = 0;
    for (i = 0; i < EFF_SPT_BASE_MAX; i++) {
        base[i] = NULL;
    }
    base_num = 0;
    for (i = 0; i < EFF_SPT_OWNER_MAX; i++) {
        for (j = 0; j < EFF_SPT_OWNER_SLOT_MAX; j++) {
            slot[i][j] = NULL;
        }
    }
    now = NULL;
    this->texb_start = texb_start;
    this->texb_num = texb_num;
    texb_used = 0;
    tail = NULL;
    head = NULL;
    now_scene = GetMainScene();
    SetEffectScriptFunc();
    level_texb_used[0] = 0;
    level_texb_used[1] = 0;
    level_texb_used[2] = 0;
    level_texb_used[3] = 0;
    manager = &mgTexManager;
    for (bank = this->texb_start; bank < this->texb_start + this->texb_num; bank++) {
        manager->DeleteBlock(bank);
    }
}

void CEffectScriptMan::SetWorkBuffer(mgCMemory *memory) {
    if (memory != NULL) {
        work_memory = memory;
    }
}

s32 CEffectScriptMan::SearchBaseNo(char *name) {
    int               index = 0;
    EFF_SPT_BASE_DEF *base;

    while (true) {
        base = GetEffSptBaseDefPtr(index);
        if (base == NULL) {
            return -1;
        }
        if (strcmp(base->name, name) == 0) {
            return index;
        }
        index++;
    }
}

s32 CEffectScriptMan::LoadBaseEffSpt(int base_no, mgCMemory *memory, int level) {
    char path[0x80];
    char pack[0x80];
    int  path_size;
    int  pack_size;
    u8  *path_buffer;
    u8  *pack_buffer;
    int  i;
    int  rest;
    int  pad;

    for (i = 0; i < EFF_SPT_BASE_MAX; i++) {
        if (base[i] != NULL && base[i]->base_no == base_no) {
            return 0;
        }
    }
    path_buffer = (u8 *)load_buffer;
    if (path_buffer == NULL) {
        return -1;
    }
    if (GetNeedFilePath(base_no, path, pack) == 0) {
        return -1;
    }
    if (LoadFile2(path, path_buffer, &path_size, 0) == 0) {
        path_size = 0;
        pack_buffer = (u8 *)load_buffer;
        path_buffer = NULL;
    } else {
        rest = path_size & 0x3F;
        pad = rest != 0 ? 0x40 - rest : 0;
        pack_buffer = &path_buffer[(path_size + pad) & -0x10];
    }
    if (LoadFile2(pack, pack_buffer, &pack_size, 0) == 0) {
        return -1;
    }
    return BuildBase(base_no, (u_long128 *)path_buffer, path_size, (u_long128 *)pack_buffer, pack_size, memory,
                     level);
}

s32 CEffectScriptMan::LoadBaseEffSpt(char *name, mgCMemory *memory, int level) {
    return LoadBaseEffSpt(SearchBaseNo(name), memory, level);
}

void CEffectScriptMan::ClearBaseFromLevel(int level, int *cleared, int max) {
    int count;
    int i;

    ClearEffectFromLevel(level);
    count = 0;
    for (i = 0; i < EFF_SPT_BASE_MAX; i++) {
        if (base[i] != NULL && base[i]->level == level) {
            if (cleared != 0 && base[i]->texb_owned != 0 && count < max) {
                cleared[count++] = base[i]->texb;
            }
            if (base[i]->texb_owned != 0) {
                texb_used--;
            }
            base[i] = NULL;
        }
    }
    if (level > 0 && level < 4) {
        texb_used -= level_texb_used[level];
        level_texb_used[level] = 0;
    }
    if (texb_used < 0) {
        texb_used = 0;
    }
    if (cleared != 0 && count < max) {
        cleared[count] = -1;
    } else if (cleared != 0) {
        printf("--- effect script err (ClearBaseFromLevel TexbTable Over)!!! ---\n");
        cleared[count - 1] = -1;
    }
}

CCharacter2 *CEffectScriptMan::GetBaseChara(int base_no) {
    int               i;
    EFF_SPT_BASE_DEF *current;
    EFF_SPT_BASE_DEF *wanted;

    for (i = 0; i < EFF_SPT_BASE_MAX; i++) {
        if (base[i] != NULL) {
            current = GetEffSptBaseDefPtr(base[i]->base_no);
            if (current == NULL) {
                return NULL;
            }
            wanted = GetEffSptBaseDefPtr(base_no);
            if (wanted == NULL) {
                return NULL;
            }
            if (current->type == 0 && strcmp(current->file, wanted->file) == 0) {
                return base[i]->chara;
            }
        }
    }
    return NULL;
}

CCharacter2 *CEffectScriptMan::GetBaseChara(char *name) {
    return GetBaseChara(SearchBaseNo(name));
}

s32 CEffectScriptMan::GetNotUsedTexb(void) {
    s32 used = texb_used;

    if (used >= texb_num) {
        return -1;
    }
    return texb_start + used;
}

void CEffectScriptMan::AddTexb() {
    int count = texb_used;

    if (count < texb_num) {
        texb_used = count + 1;
        level_texb_used[level] = level_texb_used[level] + 1;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", BuildBase__16CEffectScriptManFiP1iP1iP9mgCMemoryi);
s32 CEffectScriptMan::BuildBase(char *name, u_long128 *path_file, int path_size, u_long128 *pack_file,
                                 int pack_size, mgCMemory *memory, int level) {
    return BuildBase(SearchBaseNo(name), path_file, path_size, pack_file, pack_size, memory, level);
}

s32 CEffectScriptMan::BuildPack(int base_no, u_int *pack, mgCMemory *memory, int level) {
    char              path[0x20];
    char              pack_path[0x20];
    int               path_size;
    int               pack_size;
    EFF_SPT_BASE_DEF *base = GetEffSptBaseDefPtr(base_no);
    u_int            *path_file;
    u_int            *pack_file;

    if (base == NULL) {
        return -1;
    }
    switch (base->type) {
        case 0:
            sprintf(path, "%s.chr", base->file);
            break;
        case 1:
            sprintf(path, "%s.img", base->file);
            break;
    }
    sprintf(pack_path, "%s.stb", base->script);
    path_file = GetPackFile(pack, path, &path_size);
    pack_file = GetPackFile(pack, pack_path, &pack_size);
    return BuildBase(base_no, (u_long128 *)path_file, path_size, (u_long128 *)pack_file, pack_size, memory, level);
}

s32 CEffectScriptMan::BuildPack(char *name, u_int *pack, mgCMemory *memory, int level) {
    return BuildPack(SearchBaseNo(name), pack, memory, level);
}

s32 CEffectScriptMan::GetNeedFilePath(int base_no, char *path, char *pack) {
    EFF_SPT_BASE_DEF *base = GetEffSptBaseDefPtr(base_no);

    if (base == NULL) {
        return 0;
    }
    switch (base->type) {
        case 0:
            sprintf(path, "dungeon/eff_script/%s.chr", base->file);
            break;
        case 1:
            sprintf(path, "dungeon/eff_script/%s.img", base->file);
            break;
    }
    sprintf(pack, "dungeon/eff_script/%s.stb", base->script);
    return 1;
}

s32 CEffectScriptMan::GetNeedFilePath(char *name, char *path, char *pack) {
    return GetNeedFilePath(SearchBaseNo(name), path, pack);
}

#ifdef NONMATCHING
_EFF_SCRIPT *CEffectScriptMan::CreateEffSpt(int base_no, int group, int register_in_group) {
    EFF_SPT_BASE *base;
    int           slot;
    _EFF_SCRIPT  *script;
    u_long128    *token;

    base = NULL;
    if (base_num <= 0) {
        printf("--- effect script (non base)!!! ---\n");
        return NULL;
    }
    for (int i = 0; i < EFF_SPT_BASE_MAX; i++) {
        if (this->base[i] != NULL && this->base[i]->base_no == base_no) {
            base = this->base[i];
            break;
        }
    }
    if (base == NULL) {
        printf("--- effect script (not load base[%d])!!! ---\n");
        now = NULL;
        return NULL;
    }
    if (work_memory == NULL) {
        printf("--- effect script (non work stack)!!! ---\n");
        now = NULL;
        return NULL;
    }
    slot = -1;
    if (register_in_group == 1) {
        slot = 0;
        if (group <= -1) {
            return NULL;
        }
        for (; slot < EFF_SPT_OWNER_SLOT_MAX; slot++) {
            if (this->slot[group][slot] == NULL) {
                break;
            }
        }
        if (slot == EFF_SPT_OWNER_SLOT_MAX) {
            printf("--- effect script (ent_tbl max)!!! ---\n");
            now = NULL;
            return NULL;
        }
    }
    token = work_memory->StartStackMode(3, base->work_size);
    if (token == NULL) {
        printf("--- effect script work max[%d]!!! ---\n", work_memory->stack_size - work_memory->stack_used);
        now = NULL;
        return NULL;
    }
    script = new (work_memory->Alloc(sizeof(_EFF_SCRIPT) / 16 + 2)) _EFF_SCRIPT;
    script->work = token;
    script->texb = base->texb;
    script->level = base->level;
    script->sprite = NULL;
    script->sprite_num = 0;
    strcpy(script->tex_name, "");
    script->chara_work = NULL;
    if (base->chara != NULL) {
        CCharacter2 *chara;
        chara = new (work_memory->Alloc(sizeof(CCharacter2) / 16 + 2)) CCharacter2;
        script->chara = chara;
        script->chara->Initialize();
        base->chara->Copy(*script->chara, work_memory);
        base->work_size = base->chara->GetCopySize();
        base->work_size = base->work_size + 0x7D;
        base->work_size = base->work_size + 0x24;
        script->chara->SetPosition(0.0f, -10000.0f, 0.0f);
        script->chara->SetRotation(0.0f, 0.0f, 0.0f);
    } else {
        script->chara = NULL;
    }
    script->run.ext_func(ext_func, 0x100);
    SetEffectScript(&script->run, base->script, work_memory);
    script->prog_no = 200;
    script->user_id = group;
    script->slot = slot;
    script->work_vect1[0] = 0.0f;
    script->work_vect1[1] = 0.0f;
    script->work_vect1[2] = 0.0f;
    script->work_vect1[3] = 1.0f;
    script->work_vect2[0] = 0.0f;
    script->work_vect2[1] = 0.0f;
    script->work_vect2[2] = 0.0f;
    script->work_vect2[3] = 1.0f;
    script->target_id = -1;
    script->origin[0] = 0.0f;
    script->origin[1] = 0.0f;
    script->origin[2] = 0.0f;
    script->origin[3] = 0.0f;
    script->auto_offset = 0;
    memset(script->offset_frame, 0, 0x20);
    for (int i = 0; i < EFF_SPT_VALUE_MAX; i++) {
        script->value[i].i = 0;
    }
    script->sub_chara[0] = NULL;
    script->sub_chara[1] = NULL;
    script->sub_chara[2] = NULL;
    script->sub_chara[3] = NULL;
    script->sub_chara_work = NULL;
    script->colprim = NULL;
    script->light_flag = 0;
    script->state = 0;
    script->next = NULL;
    script->prev = NULL;
    work_memory->stAlign64();
    work_memory->EndStackMode();
    if (register_in_group == 1) {
        this->slot[group][slot] = script;
    }
    _EFF_SCRIPT *cursor = head;
    if (cursor == NULL) {
        head = script;
        tail = script;
        tail->next = NULL;
        tail->prev = NULL;
        head->next = NULL;
        head->prev = NULL;
    } else if (cursor != NULL) {
        do {
            if (cursor->texb > script->texb) {
                script->prev = cursor->prev;
                script->next = cursor;
                if (cursor->prev != NULL) {
                    cursor->prev->next = script;
                } else {
                    head = script;
                }
                cursor->prev = script;
                break;
            } else {
                _EFF_SCRIPT *following = cursor->next;
                if (following == NULL) {
                    cursor->next = script;
                    script->prev = cursor;
                    tail = script;
                    break;
                }
                cursor = following;
            }
        } while (cursor != NULL);
    }
    now = script;
    return script;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", CreateEffSpt__16CEffectScriptManFiii);
#endif

s32 CEffectScriptMan::CreateEffSpt(char *name, s32 user_id, s32 use_slot) {
    _EFF_SCRIPT *effect = CreateEffSpt(SearchBaseNo(name), user_id, use_slot);

    if (effect != NULL) {
        return effect->slot;
    }
    return -1;
}

void CEffectScriptMan::ClearEffectFromChrid(int chrid) {
    _EFF_SCRIPT *script = head;
    _EFF_SCRIPT *doomed;

    while (script != NULL) {
        if (script->user_id == chrid) {
            doomed = script;
            script = script->next;
            DeleteEffSpt(doomed);
        } else {
            script = script->next;
        }
    }
}

void CEffectScriptMan::ClearEffectFromLevel(int level) {
    _EFF_SCRIPT *script = head;
    _EFF_SCRIPT *doomed;

    while (script != NULL) {
        if (script->level == level) {
            doomed = script;
            script = script->next;
            DeleteEffSpt(doomed);
        } else {
            script = script->next;
        }
    }
}

void CEffectScriptMan::DeleteEffSpt(_EFF_SCRIPT *script) {
    if (script == NULL || work_memory == NULL) {
        return;
    }
    if (script->prev != NULL) {
        script->prev->next = script->next;
    } else {
        head = script->next;
        if (head != NULL) {
            head->prev = NULL;
        }
    }
    if (script->next != NULL) {
        script->next->prev = script->prev;
    } else {
        tail = script->prev;
        if (tail != NULL) {
            tail->next = NULL;
        }
    }
    if (now != NULL && script->work == now->work) {
        now = NULL;
    }
    if (script->slot >= 0) {
        slot[script->user_id][script->slot] = NULL;
    }
    if (script->colprim != NULL) {
        script->colprim->Delete(script->user_id);
    }
    DeleteSprite(script->sprite);
    if (script->sub_chara_work != NULL) {
        work_memory->Free(script->sub_chara_work);
    }
    if (script->chara_work != NULL) {
        work_memory->Free(script->chara_work);
    }
    work_memory->Free(script->work);
}

s32 CEffectScriptMan::DeleteEffSpt(int group, int slot) {
    if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot < 0 || slot >= EFF_SPT_OWNER_SLOT_MAX) {
        return 0;
    }
    DeleteEffSpt(this->slot[group][slot]);
    return 1;
}

void CEffectScriptMan::AllClearEffSpt() {
    _EFF_SCRIPT *script = tail;
    _EFF_SCRIPT *prev;
    int          group;
    int          slot;

    if (script != NULL) {
        while (script->prev != NULL) {
            prev = script->prev;
            script = prev;
            DeleteEffSpt(prev->next);
        }
        DeleteEffSpt(script);
        tail = NULL;
        head = NULL;
        for (group = 0; group < EFF_SPT_OWNER_MAX; group++) {
            for (slot = 0; slot < EFF_SPT_OWNER_SLOT_MAX; slot++) {
                this->slot[group][slot] = NULL;
            }
        }
        now = NULL;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", Step__16CEffectScriptManFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", Draw__16CEffectScriptManFv);
_ES_SPRITE *CEffectScriptMan::AssignSprite(int count) {
    u_int size;
    u_int blocks;

    if (work_memory == NULL) {
        return NULL;
    }
    size = count * sizeof(_ES_SPRITE);
    blocks = align16_blocks(size) + 3;
    if (work_memory->StartStackMode(3, blocks) == 0) {
        printf("------- es work max!! (assign sprite[%d]) ---------\n", blocks);
        return NULL;
    }

    _ES_SPRITE *sprite = (_ES_SPRITE *)operator new[](
        size, work_memory->Alloc(align16_blocks(size) + 2));

    memset(sprite, 0, blocks);
    work_memory->stAlign64();
    work_memory->EndStackMode();
    return sprite;
}

void CEffectScriptMan::DeleteSprite(_ES_SPRITE *sprite) {
    mgCMemory *memory = work_memory;

    if (memory == NULL || sprite == NULL) {
        return;
    }
    memory->Free((u_long128 *)sprite);
}

#ifdef NONMATCHING
s32 CEffectScriptMan::AssignCharacter(_EFF_SCRIPT *script, int count) {
    if (count > EFF_SPT_SUB_CHARA_MAX) {
        return 0;
    }
    int size = count * (script->chara->GetCopySize() + sizeof(CCharacter2) / 16 + 2);
    u_long128 *token = work_memory->StartStackMode(3, size);
    if (token == NULL) {
        printf("------- es work max!! (assign character[%d]) ---------\n", size);
        return 0;
    }
    for (int i = 0; i < count; i++) {
        CCharacter2 *chara;
        chara = new (work_memory->Alloc(sizeof(CCharacter2) / 16 + 2)) CCharacter2;
        script->sub_chara[i] = chara;
        script->chara->Copy(*script->sub_chara[i], work_memory);
    }
    script->sub_chara_work = token;
    work_memory->stAlign64();
    work_memory->EndStackMode();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", AssignCharacter__16CEffectScriptManFP11_EFF_SCRIPTi);
#endif

s32 CEffectScriptMan::SetScriptProgNo(int prog_no, int group, int slot) {
    _EFF_SCRIPT *script;

    if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot < 0 || slot >= EFF_SPT_OWNER_SLOT_MAX) {
        return 0;
    }
    script = this->slot[group][slot];
    if (script == NULL) {
        return 0;
    }
    script->prog_no = prog_no;
    return 1;
}

s32 CEffectScriptMan::Pause(int state, int group, int slot) {
    _EFF_SCRIPT *script;

    if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot < 0 || slot >= EFF_SPT_OWNER_SLOT_MAX) {
        return 0;
    }
    script = this->slot[group][slot];
    if (script == NULL) {
        return 0;
    }
    script->state = state;
    return 1;
}

void CEffectScriptMan::PauseFromLevel(int level, int state) {
    for (_EFF_SCRIPT *script = head; script != NULL; script = script->next) {
        if (script->level == level) {
            script->state = state;
        }
    }
}

s32 CEffectScriptMan::SetScriptVect1(float *vect, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        *(u_long128 *)script->work_vect1 = *(u_long128 *)vect;
        return 1;
    }
    first = now;
    if (first != NULL) {
        *(u_long128 *)first->work_vect1 = *(u_long128 *)vect;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::GetScriptVect1(float *vect, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        *(u_long128 *)vect = *(u_long128 *)script->work_vect1;
        return 1;
    }
    first = now;
    if (first != NULL) {
        *(u_long128 *)vect = *(u_long128 *)first->work_vect1;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetScriptVect2(float *vect, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        *(u_long128 *)script->work_vect2 = *(u_long128 *)vect;
        return 1;
    }
    first = now;
    if (first != NULL) {
        *(u_long128 *)first->work_vect2 = *(u_long128 *)vect;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::GetScriptVect2(float *vect, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        *(u_long128 *)vect = *(u_long128 *)script->work_vect2;
        return 1;
    }
    first = now;
    if (first != NULL) {
        *(u_long128 *)vect = *(u_long128 *)first->work_vect2;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetScriptTargetId(int target_id, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->target_id = target_id;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->target_id = target_id;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::GetScriptTargetId(int &target_id, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        target_id = script->target_id;
        return 1;
    }
    first = now;
    if (first != NULL) {
        target_id = first->target_id;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetScriptUserId(int user_id, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->user_id = user_id;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->user_id = user_id;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::GetScriptUserId(int &user_id, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        user_id = script->user_id;
        return 1;
    }
    first = now;
    if (first != NULL) {
        user_id = first->user_id;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetColPrim(CColPrim *colprim, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->colprim = colprim;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->colprim = colprim;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetValue(int index, int value, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (index < 0 || index >= EFF_SPT_VALUE_MAX) {
        return 0;
    }
    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->value[index].i = value;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->value[index].i = value;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetValue(int index, float value, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (index < 0 || index >= EFF_SPT_VALUE_MAX) {
        return 0;
    }
    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->value[index].f = value;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->value[index].f = value;
        return 1;
    }
    return 0;
}

s32 CEffectScriptMan::SetOrigin(float *vect, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        *(u_long128 *)script->origin = *(u_long128 *)vect;
        return 1;
    }
    first = now;
    if (first != NULL) {
        *(u_long128 *)first->origin = *(u_long128 *)vect;
        return 1;
    }
    return 0;
}

CCharacter2 *CEffectScriptMan::GetCharacter(int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return NULL;
        }
        script = this->slot[group][slot];
        if (script != NULL) {
            return script->chara;
        }
        return NULL;
    }
    first = now;
    if (first != NULL) {
        return first->chara;
    }
    return NULL;
}

#ifdef NONMATCHING
s32 CEffectScriptMan::SetCharacter(CCharacter2 *source, int group, int slot) {
    int        chara_blocks = source->GetCopySize() + sizeof(CCharacter2) / 16 + 2;
    u_long128 *token = work_memory->StartStackMode(3, chara_blocks);

    if (token == NULL) {
        printf("------- es work max!! (set character) ---------\n");
        return 0;
    }
    CCharacter2 *chara;
    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }

        _EFF_SCRIPT **entry = &this->slot[group][slot];
        if (*entry == NULL) {
            return 0;
        }
        chara = new (work_memory->Alloc(sizeof(CCharacter2) / 16 + 2)) CCharacter2;
        (*entry)->chara = chara;
        source->Copy(*(*entry)->chara, work_memory);
        (*entry)->chara_work = token;
    } else {
        if (now != NULL) {
            chara = new (work_memory->Alloc(sizeof(CCharacter2) / 16 + 2)) CCharacter2;
            now->chara = chara;
            source->Copy(*now->chara, work_memory);
            now->chara_work = token;
        } else {
            return 0;
        }
    }
    work_memory->stAlign64();
    work_memory->EndStackMode();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", SetCharacter__16CEffectScriptManFP11CCharacter2ii);
#endif

s32 CEffectScriptMan::SetTexb(int texb, int group, int slot) {
    _EFF_SCRIPT *script;
    _EFF_SCRIPT *first;

    if (slot >= 0) {
        if (group < 0 || group >= EFF_SPT_OWNER_MAX || slot >= EFF_SPT_OWNER_SLOT_MAX) {
            return 0;
        }
        script = this->slot[group][slot];
        if (script == NULL) {
            return 0;
        }
        script->texb = texb;
        return 1;
    }
    first = now;
    if (first != NULL) {
        first->texb = texb;
        return 1;
    }
    return 0;
}

/**
 * Returns an effect base definition unless the index reaches its terminator.
 */
static EFF_SPT_BASE_DEF *GetEffSptBaseDefPtr(int index) {
    EFF_SPT_BASE_DEF *base;

    if (index < 0) {
        return NULL;
    }
    base = &eff_spt_base_def[index];
    return strcmp(base->name, "") == 0 ? NULL : base;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", DrawEffSptSprite__FP11_EFF_SCRIPTP10mgCTexturePfP11mgC3DSpriteP16CMapLightingInfo);
/**
 * Returns an effect sprite when its index is in range.
 */
static _ES_SPRITE *GetSpritePtr(_EFF_SCRIPT *script, int index) {
    if (script == NULL || index >= script->sprite_num) {
        return NULL;
    }
    return &script->sprite[index];
}

/**
 * Returns a stack value as an integer.
 */
static int GetStackInt(RS_STACKDATA *slot) {
    if (slot->type == RS_FLOAT) {
        return (int)slot->f;
    }
    return slot->i;
}

/**
 * Returns a stack value as a float.
 */
static float GetStackFloat(RS_STACKDATA *slot) {
    if (slot->type == RS_INT) {
        return (float)slot->i;
    }
    return slot->f;
}

/**
 * Copies three stack values into a homogeneous vector.
 */
static void GetStackVector(float *vector, RS_STACKDATA *slot) {
    vector[0] = GetStackFloat(slot++);
    vector[1] = GetStackFloat(slot++);
    vector[2] = GetStackFloat(slot);
    vector[3] = 1.0f;
}

/**
 * Returns the string address held by a stack value.
 */
static char *GetStackString(RS_STACKDATA *slot) {
    return slot->s;
}

/**
 * Stores a value through a stack reference.
 */
static void SetStack(RS_STACKDATA *slot, int value) {
    if (slot->type == RS_PTR) {
        slot->p->i = value;
    }
}

/**
 * Stores a value through a stack reference.
 */
static void SetStack(RS_STACKDATA *slot, float value) {
    if (slot->type == RS_PTR) {
        slot->p->f = value;
    }
}

/**
 * Clears a referenced vector.
 */
static int _ZERO_VECTOR(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 3) {
        return 0;
    }
    SetStack(stack++, 0.0f);
    SetStack(stack++, 0.0f);
    SetStack(stack, 0.0f);
    return 1;
}

/**
 * Normalizes a vector in the script stack.
 */
static int _NORMAL_VECTOR(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (argument_count != 3) {
        return 0;
    }
    vector[0] = stack->p->f;
    vector[1] = (stack + 1)->p->f;
    vector[2] = (stack + 2)->p->f;
    vector[3] = 1.0f;
    sceVu0Normalize(vector, vector);
    SetStack(stack++, vector[0]);
    SetStack(stack++, vector[1]);
    SetStack(stack, vector[2]);
    return 1;
}

/**
 * Copies a vector into three referenced stack values.
 */
static int _COPY_VECTOR(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (argument_count != 6) {
        return 0;
    }
    GetStackVector(vector, stack + 3);
    SetStack(stack++, vector[0]);
    SetStack(stack++, vector[1]);
    SetStack(stack, vector[2]);
    return 1;
}

/**
 * Adds a vector to three referenced stack values.
 */
static int _ADD_VECTOR(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (argument_count != 6) {
        return 0;
    }
    GetStackVector(vector, stack + 3);
    SetStack(stack, stack->p->f + vector[0]);
    SetStack(stack + 1, (stack + 1)->p->f + vector[1]);
    SetStack(stack + 2, (stack + 2)->p->f + vector[2]);
    return 1;
}

/**
 * Subtracts a vector from three referenced stack values.
 */
static int _SUB_VECTOR(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (argument_count != 6) {
        return 0;
    }
    GetStackVector(vector, stack + 3);
    SetStack(stack, stack->p->f - vector[0]);
    SetStack(stack + 1, (stack + 1)->p->f - vector[1]);
    SetStack(stack + 2, (stack + 2)->p->f - vector[2]);
    return 1;
}

/**
 * Scales three referenced vector components.
 */
static int _SCALE_VECTOR(RS_STACKDATA *stack, int argument_count) {
    float scale;

    if (argument_count != 4) {
        return 0;
    }
    scale = GetStackFloat(stack + 3);
    SetStack(stack, stack->p->f * scale);
    SetStack(stack + 1, (stack + 1)->p->f * scale);
    SetStack(stack + 2, (stack + 2)->p->f * scale);
    return 1;
}

/**
 * Divides three referenced vector components by a nonzero value.
 */
static int _DIV_VECTOR(RS_STACKDATA *stack, int argument_count) {
    float divisor;

    if (argument_count != 4) {
        return 0;
    }
    divisor = GetStackFloat(stack + 3);
    if (divisor == 0.0f) {
        return 0;
    }
    SetStack(stack, stack->p->f / divisor);
    SetStack(stack + 1, (stack + 1)->p->f / divisor);
    SetStack(stack + 2, (stack + 2)->p->f / divisor);
    return 1;
}

/**
 * Returns the length of a vector.
 */
static int _DIST_VECTOR(RS_STACKDATA *stack, int argument_count) {
    float vector[3];

    if (argument_count != 4) {
        return 0;
    }
    GetStackVector(vector, stack);
    stack += 3;
    SetStack(stack++, mgDistVector(vector));
    return 1;
}

/**
 * Returns the distance between two vectors.
 */
static int _DIST_VECTOR2(RS_STACKDATA *stack, int argument_count) {
    float from[3];
    float to[3];

    if (argument_count != 7) {
        return 0;
    }
    GetStackVector(from, stack);
    GetStackVector(to, stack + 3);
    stack += 6;
    SetStack(stack++, mgDistVector(from, to));
    return 1;
}

/**
 * Returns the square root of a stack value.
 */
static int _SQRT(RS_STACKDATA *stack, int argument_count) {
    float value;

    if (argument_count != 2) {
        return 0;
    }
    value = GetStackFloat(stack++);
    SetStack(stack, (float)sqrt(value));
    return 1;
}

/**
 * Returns the angle of two vector components.
 */
static int _ATAN2F(RS_STACKDATA *stack, int argument_count) {
    float y;
    float x;

    if (argument_count != 3) {
        return 0;
    }
    y = GetStackFloat(stack++);
    x = GetStackFloat(stack++);
    SetStack(stack, atan2f(y, x));
    return 1;
}

/**
 * Compares angles within the given tolerance.
 */
static int _ANGLE_CMP(RS_STACKDATA *stack, int argument_count) {
    float a;
    float b;
    float c;

    if (argument_count != 4) {
        return 0;
    }
    a = GetStackFloat(stack++);
    b = GetStackFloat(stack++);
    c = GetStackFloat(stack++);
    SetStack(stack, mgAngleCmp(a, b, c));
    return 1;
}

/**
 * Wraps a referenced angle.
 */
static int _ANGLE_LIMIT(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 1) {
        return 0;
    }
    SetStack(stack, mgAngleLimit(stack->p->f));
    return 1;
}

/**
 * Returns a random value within the requested range.
 */
static int _GET_RAND(RS_STACKDATA *stack, int argument_count) {
    float float_range;
    int   range;
    int   value;

    if (argument_count != 2) {
        return 0;
    }
    if (stack->type == RS_FLOAT) {
    float_range = GetStackFloat(stack++);
        SetStack(stack, float_range * (float)rand() / 2147483648.0f);
        return 1;
    }
    range = GetStackInt(stack++);
    value = (int)((float)range * (float)rand() / 2147483648.0f);
    SetStack(stack, value);
    return 1;
}

/**
 * Computes the rotation from one point toward another.
 */
static int _GET_REF_ROT(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR dir;
    float        *z;
    float         yaw;
    float         pitch;

    if (argument_count != 7 && argument_count != 9) {
        return 0;
    }
    GetStackVector(from, stack);
    GetStackVector(dir, stack + 3);
    stack += 6;
    sceVu0SubVector(dir, dir, from);
    sceVu0Normalize(dir, dir);
    z = &dir[2];
    yaw = atan2f(dir[0], *z);
    pitch = -atan2f(dir[1], sqrtf(dir[0] * dir[0] + *z * *z));
    switch (argument_count) {
        case 7:
            SetStack(stack, yaw);
            break;
        case 9:
            SetStack(stack++, pitch);
            SetStack(stack++, yaw);
            SetStack(stack, 0.0f);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Computes a direction vector from a rotation.
 */
static int _GET_DIR_VECTOR(RS_STACKDATA *stack, int argument_count) {
    float         matrix[4][4];
    sceVu0FVECTOR rot;
    float        *y;
    float        *z;

    if (argument_count != 6) {
        return 0;
    }
    sceVu0FVECTOR dir = { 0.0f, 0.0f, 1.0f, 1.0f };
    GetStackVector(rot, stack);
    stack += 3;
    rot[0] = mgAngleLimit(rot[0]);
    y = &rot[1];
    *y = mgAngleLimit(*y);
    z = &rot[2];
    *z = mgAngleLimit(*z);
    sceVu0UnitMatrix(matrix);
    sceVu0RotMatrixX(matrix, matrix, rot[0]);
    sceVu0RotMatrixY(matrix, matrix, *y);
    sceVu0ApplyMatrix(dir, matrix, dir);
    SetStack(stack++, dir[0]);
    SetStack(stack++, dir[1]);
    SetStack(stack, dir[2]);
    return 1;
}

/**
 * Sets the origin for the running effect.
 */
static int _SET_ORIGIN(RS_STACKDATA *stack, int argument_count) {
    now_script->origin[0] = GetStackFloat(stack++);
    now_script->origin[1] = GetStackFloat(stack++);
    now_script->origin[2] = GetStackFloat(stack);
    now_script->origin[3] = 0.0f;
    return 1;
}

/**
 * Returns the origin for the running effect.
 */
static int _GET_ORIGIN(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 3) {
        return 0;
    }
    SetStack(stack++, now_script->origin[0]);
    SetStack(stack++, now_script->origin[1]);
    SetStack(stack, now_script->origin[2]);
    return 1;
}

/**
 * Selects a target character or named frame for the effect origin offset.
 */
static int _AUTO_SET_OFFSET(RS_STACKDATA *stack, int argument_count) {
    char *name = NULL;
    int   offset = GetStackInt(stack++);

    if (argument_count >= 2) {
        name = GetStackString(stack);
    }
    now_script->auto_offset = offset;
    if (name != NULL) {
        strcpy(now_script->offset_frame, name);
    } else {
        strcpy(now_script->offset_frame, "");
    }
    return 1;
}

/**
 * Returns the work first work vector for the running effect.
 */
static int _GET_WORK_VECT1(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 3) {
        return 0;
    }
    SetStack(stack++, now_script->work_vect1[0]);
    SetStack(stack++, now_script->work_vect1[1]);
    SetStack(stack, now_script->work_vect1[2]);
    return 1;
}

/**
 * Returns the work second work vector for the running effect.
 */
static int _GET_WORK_VECT2(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 3) {
        return 0;
    }
    SetStack(stack++, now_script->work_vect2[0]);
    SetStack(stack++, now_script->work_vect2[1]);
    SetStack(stack, now_script->work_vect2[2]);
    return 1;
}

/**
 * Returns the target id for the running effect.
 */
static int _GET_TARGET_ID(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 1) {
        return 0;
    }
    SetStack(stack, now_script->target_id);
    return 1;
}

/**
 * Returns the user id for the running effect.
 */
static int _GET_USER_ID(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 1) {
        return 0;
    }
    SetStack(stack, now_script->user_id);
    return 1;
}

/**
 * Returns the value for the running effect.
 */
static int _GET_VALUE(RS_STACKDATA *stack, int argument_count) {
    RS_STACKDATA *result_slot;
    int           index;
    _EFF_SCRIPT  *script;

    if (argument_count != 2) {
        return 0;
    }
    result_slot = stack + 1;
    index = GetStackInt(stack);
    if (result_slot->type != RS_PTR) {
        return 0;
    }
    switch (result_slot->p->type) {
        case RS_INT: {
            script = now_script;
            SetStack(result_slot, script->value[index].i);
            break;
        }
        case RS_FLOAT: {
            script = now_script;
            SetStack(result_slot, script->value[index].f);
            break;
        }
        default:
            return 0;
    }
    return 1;
}

/**
 * Sets the value for the running effect.
 */
static int _SET_VALUE(RS_STACKDATA *stack, int argument_count) {
    int           index;
    RS_STACKDATA *value_slot;
    int           int_value;
    _EFF_SCRIPT  *script;
    float         float_value;

    if (argument_count != 2) {
        return 0;
    }
    value_slot = stack + 1;
    index = GetStackInt(stack);
    if (value_slot->type == RS_PTR) {
        return 0;
    }
    switch (value_slot->type) {
        case RS_INT: {
            int_value = GetStackInt(value_slot);
            script = now_script;
            script->value[index].i = int_value;
            break;
        }
        case RS_FLOAT: {
            float_value = GetStackFloat(value_slot);
            script = now_script;
            script->value[index].f = float_value;
            break;
        }
        default:
            return 0;
    }
    return 1;
}

/**
 * Sets the character show for the running effect.
 */
static int _CHR_SET_SHOW(RS_STACKDATA *stack, int argument_count) {
    int           show;
    int           fade;
    RS_STACKDATA *next;

    if (now_script->chara == NULL) {
        return 0;
    }
    fade = 0;
    next = stack + 1;
    show = GetStackInt(stack);
    if (argument_count >= 2) {
        fade = GetStackInt(next++);
        if (argument_count == 3) {
            GetStackFloat(next);
        }
    }
    now_script->chara->Show(show);
    now_script->chara->fade = 1;
    now_script->chara->fade_speed = 0.1f;
    if (fade == 1 && show == 1) {
        now_script->chara->fade_alpha = 0.0001f;
    } else if (fade == 1 && show == 0) {
        now_script->chara->fade_alpha = 1.0f;
    }
    return 1;
}

/**
 * Returns the character show for the running effect.
 */
static int _CHR_GET_SHOW(RS_STACKDATA *stack, int argument_count) {
    if (argument_count != 1 && argument_count != 2) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    SetStack(stack++, now_script->chara->GetShow());
    if (argument_count == 2) {
        SetStack(stack, now_script->chara->fade);
    }
    return 1;
}

/**
 * Sets the character position for the running effect.
 */
static int _CHR_SET_POS(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(vector, stack);
    now_script->chara->SetPosition(vector);
    return 1;
}

/**
 * Returns the character position for the running effect.
 */
static int _CHR_GET_POS(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR pos;

    if (argument_count != 3) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    now_script->chara->GetPosition(pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Sets the character rotation for the running effect.
 */
static int _CHR_SET_ROT(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR rot;
    sceVu0FVECTOR current;
    int           steps;
    float        *y;
    float        *z;

    if (now_script->chara == NULL) {
        return 0;
    }
    steps = 1;
    GetStackVector(rot, stack);
    stack += 3;
    if (argument_count >= 4) {
        steps = GetStackInt(stack++);
    }
    now_script->chara->GetRotation(current);
    sceVu0SubVector(rot, rot, current);
    rot[0] = mgAngleLimit(rot[0]);
    y = &rot[1];
    *y = mgAngleLimit(*y);
    z = &rot[2];
    *z = mgAngleLimit(*z);
    sceVu0DivVector(rot, rot, (float)steps);
    sceVu0AddVector(rot, rot, current);
    rot[0] = mgAngleLimit(rot[0]);
    *y = mgAngleLimit(*y);
    *z = mgAngleLimit(*z);
    rot[3] = 1.0f;
    now_script->chara->SetRotation(rot);
    return 1;
}

/**
 * Returns the character rotation for the running effect.
 */
static int _CHR_GET_ROT(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR rot;

    if (argument_count != 3) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    now_script->chara->GetRotation(rot);
    SetStack(stack++, rot[0]);
    SetStack(stack++, rot[1]);
    SetStack(stack, rot[2]);
    return 1;
}

/**
 * Sets the character scale for the running effect.
 */
static int _CHR_SET_SCALE(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR vector;

    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(vector, stack);
    now_script->chara->SetScale(vector);
    return 1;
}

/**
 * Returns the character scale for the running effect.
 */
static int _CHR_GET_SCALE(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR scale;

    if (argument_count != 3) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    now_script->chara->GetScale(scale);
    SetStack(stack++, scale[0]);
    SetStack(stack++, scale[1]);
    SetStack(stack, scale[2]);
    return 1;
}

/**
 * Sets the character motion for the running effect.
 */
static int _CHR_SET_MOTION(RS_STACKDATA *stack, int argument_count) {
    int   mode;
    float step;
    char *name;

    if (now_script->chara == NULL) {
        return 0;
    }
    mode = 0;
    step = -1.0f;
    name = GetStackString(stack++);
    if (argument_count >= 2) {
        step = GetStackFloat(stack++);
    }
    if (argument_count >= 3) {
        mode = GetStackInt(stack);
    }
    now_script->chara->SetMotion(name, mode);
    if (step >= 0.0f) {
        now_script->chara->SetStep(step);
    }
    return 1;
}

/**
 * Sets the character motion step for the running effect.
 */
static int _CHR_SET_MOT_STEP(RS_STACKDATA *stack, int argument_count) {
    if (now_script->chara == NULL) {
        return 0;
    }
    now_script->chara->SetStep(GetStackFloat(stack));
    return 1;
}

/**
 * Returns the character motion wait for the running effect.
 */
static int _CHR_GET_MOT_WAIT(RS_STACKDATA *stack, int argument_count) {
    CCharacter2 *chara;

    if (argument_count != 1) {
        return 0;
    }
    chara = now_script->chara;
    if (chara == NULL) {
        return 0;
    }
    SetStack(stack, chara->GetNowFrameWait());
    return 1;
}

/**
 * Returns the forward direction of the effect character.
 */
static int _CHR_GET_DIR_VECTOR(RS_STACKDATA *stack, int argument_count) {
    float         matrix[4][4];
    sceVu0FVECTOR rot;

    if (argument_count != 3) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    sceVu0FVECTOR dir = { 0.0f, 0.0f, 1.0f, 1.0f };
    sceVu0UnitMatrix(matrix);
    now_script->chara->GetRotation(rot);
    sceVu0RotMatrixX(matrix, matrix, rot[0]);
    sceVu0RotMatrixY(matrix, matrix, rot[1]);
    sceVu0ApplyMatrix(dir, matrix, dir);
    SetStack(stack++, dir[0]);
    SetStack(stack++, dir[1]);
    SetStack(stack, dir[2]);
    return 1;
}

/**
 * Computes the rotation from the effect character toward a point.
 */
static int _CHR_GET_REF_ROT(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR dir;
    sceVu0FVECTOR pos;
    float        *z;
    float         yaw;
    float         pitch;

    if (argument_count != 4 && argument_count != 6) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(dir, stack);
    stack += 3;
    now_script->chara->GetPosition(pos);
    sceVu0SubVector(dir, dir, pos);
    sceVu0Normalize(dir, dir);
    z = &dir[2];
    yaw = atan2f(dir[0], *z);
    pitch = -atan2f(dir[1], sqrtf(dir[0] * dir[0] + *z * *z));
    switch (argument_count) {
        case 4:
            SetStack(stack, yaw);
            break;
        case 6:
            SetStack(stack++, pitch);
            SetStack(stack++, yaw);
            SetStack(stack, 0.0f);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Adds the character position for the running effect.
 */
static int _CHR_ADD_POS(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR offset;
    sceVu0FVECTOR pos;

    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(offset, stack);
    now_script->chara->GetPosition(pos);
    sceVu0AddVector(pos, pos, offset);
    now_script->chara->SetPosition(pos);
    return 1;
}

/**
 * Adds the character rotation for the running effect.
 */
static int _CHR_ADD_ROT(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR offset;
    sceVu0FVECTOR rot;
    float        *y;
    float        *z;

    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(offset, stack);
    now_script->chara->GetRotation(rot);
    sceVu0AddVector(rot, rot, offset);
    rot[0] = mgAngleLimit(rot[0]);
    y = &rot[1];
    *y = mgAngleLimit(*y);
    z = &rot[2];
    *z = mgAngleLimit(*z);
    rot[3] = 1.0f;
    now_script->chara->SetRotation(rot);
    return 1;
}

/**
 * Adds the character scale for the running effect.
 */
static int _CHR_ADD_SCALE(RS_STACKDATA *stack, int argument_count) {
    sceVu0FVECTOR offset;
    sceVu0FVECTOR scale;

    if (now_script->chara == NULL) {
        return 0;
    }
    GetStackVector(offset, stack);
    now_script->chara->GetScale(scale);
    sceVu0AddVector(scale, scale, offset);

    now_script->chara->GetScale(scale);
    return 1;
}

/**
 * Allocates extra character copies for the running effect.
 */
static int _CHR_COPY_CHARA(RS_STACKDATA *stack, int argument_count) {
    if (now_script->chara == NULL) {
        return 0;
    }
    return EffScriptMan->AssignCharacter(now_script, GetStackInt(stack));
}

/**
 * Sets the character pos2 for the running effect.
 */
static int _CHR_SET_POS2(RS_STACKDATA *stack, int argument_count) {
    int           index = GetStackInt(stack++);
    sceVu0FVECTOR vector;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(vector, stack);
    now_script->sub_chara[index]->SetPosition(vector);
    return 1;
}

/**
 * Sets the character rot2 for the running effect.
 */
static int _CHR_SET_ROT2(RS_STACKDATA *stack, int argument_count) {
    int           index = GetStackInt(stack++);
    sceVu0FVECTOR vector;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(vector, stack);
    now_script->sub_chara[index]->SetRotation(vector);
    return 1;
}

/**
 * Sets the character scale2 for the running effect.
 */
static int _CHR_SET_SCALE2(RS_STACKDATA *stack, int argument_count) {
    int           index = GetStackInt(stack++);
    sceVu0FVECTOR vector;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(vector, stack);
    now_script->sub_chara[index]->SetScale(vector);
    return 1;
}

/**
 * Sets the character motion2 for the running effect.
 */
static int _CHR_SET_MOTION2(RS_STACKDATA *stack, int argument_count) {
    int   index = GetStackInt(stack++);
    int   mode;
    float step;
    char *name;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    mode = 0;
    step = -1.0f;
    name = GetStackString(stack++);
    if (argument_count >= 2) {
        step = GetStackFloat(stack++);
    }
    if (argument_count >= 3) {
        mode = GetStackInt(stack);
    }
    now_script->sub_chara[index]->SetMotion(name, mode);
    if (step >= 0.0f) {
        now_script->sub_chara[index]->SetStep(step);
    }
    return 1;
}

/**
 * Adds the character pos2 for the running effect.
 */
static int _CHR_ADD_POS2(RS_STACKDATA *stack, int argument_count) {
    int           index = GetStackInt(stack++);
    sceVu0FVECTOR offset;
    sceVu0FVECTOR pos;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(offset, stack);
    now_script->sub_chara[index]->GetPosition(pos);
    sceVu0AddVector(pos, pos, offset);
    now_script->sub_chara[index]->SetPosition(pos);
    return 1;
}

/**
 * Adds the character rot2 for the running effect.
 */
static int _CHR_ADD_ROT2(RS_STACKDATA *stack, int argument_count) {
    int           index;
    RS_STACKDATA *next = stack + 1;
    sceVu0FVECTOR delta;
    sceVu0FVECTOR rot;
    float        *y;
    float        *z;

    index = GetStackInt(stack);
    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(delta, next);
    now_script->sub_chara[index]->GetRotation(rot);
    sceVu0AddVector(rot, rot, delta);
    rot[0] = mgAngleLimit(rot[0]);
    y = &rot[1];
    *y = mgAngleLimit(*y);
    z = &rot[2];
    *z = mgAngleLimit(*z);
    rot[3] = 1.0f;
    now_script->sub_chara[index]->SetRotation(rot);
    return 1;
}

/**
 * Adds the character scale2 for the running effect.
 */
static int _CHR_ADD_SCALE2(RS_STACKDATA *stack, int argument_count) {
    int           index = GetStackInt(stack++);
    sceVu0FVECTOR offset;
    sceVu0FVECTOR scale;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    GetStackVector(offset, stack);
    now_script->sub_chara[index]->GetScale(scale);
    sceVu0AddVector(scale, scale, offset);
    now_script->sub_chara[index]->SetScale(scale);
    return 1;
}

/**
 * Sets the character show2 for the running effect.
 */
static int _CHR_SET_SHOW2(RS_STACKDATA *stack, int argument_count) {
    int index = GetStackInt(stack++);
    int show;
    int fade;

    if (now_script->sub_chara[index] == NULL) {
        return 0;
    }
    fade = 0;
    show = GetStackInt(stack++);
    if (argument_count >= 3) {
        fade = GetStackInt(stack++);
        if (argument_count == 4) {
            GetStackFloat(stack);
        }
    }
    now_script->sub_chara[index]->Show(show);
    now_script->sub_chara[index]->fade = 1;
    now_script->sub_chara[index]->fade_speed = 0.1f;
    if (fade == 1 && show == 1) {
        now_script->sub_chara[index]->fade_alpha = 0.0001f;
    } else if (fade == 1 && show == 0) {
        now_script->sub_chara[index]->fade_alpha = 1.0f;
    }
    return 1;
}

/**
 * Returns the world position of a named frame of the effect character.
 */
static int _CHR_GET_FRAME_POS(RS_STACKDATA *stack, int argument_count) {
    RS_STACKDATA *result_slot;
    mgCFrame     *character_frame;
    mgCFrame     *frame;
    char         *name;
    sceVu0FVECTOR pos;

    if (argument_count != 4) {
        return 0;
    }
    if (now_script->chara == NULL) {
        return 0;
    }
    result_slot = stack + 1;
    name = GetStackString(stack);
    if ((character_frame = now_script->chara->CObjectFrame::frame) == NULL) {
        return 0;
    }
    if ((frame = character_frame->SearchFrame(name)) == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(pos);
    SetStack(result_slot++, pos[0]);
    SetStack(result_slot++, pos[1]);
    SetStack(result_slot, pos[2]);
    return 1;
}

/**
 * Sets the drawing attributes of a named frame of the effect character.
 */
static int _CHR_SET_FRAME_SHOW(RS_STACKDATA *stack, int argument_count) {
    char     *name;
    int       show;
    int       attr_mask;
    mgCFrame *character_frame;
    mgCFrame *frame;

    if (argument_count != 3) {
        return 0;
    }
    name = GetStackString(stack++);
    show = GetStackInt(stack++);
    attr_mask = GetStackInt(stack);
    character_frame = now_script->chara->CObjectFrame::frame;
    if (character_frame == NULL) {
        return 0;
    }
    if ((frame = character_frame->SearchFrame(name)) == NULL) {
        return 0;
    }
    mgCFrameAttr attr;
    attr.draw = show;
    frame->SetAttrParam(attr, attr_mask, 1);
    return 1;
}

/**
 * Returns whether the effect character motion has ended.
 */
static int _CHR_CHK_MOT_END(RS_STACKDATA *stack, int argument_count) {
    CCharacter2 *chara;

    if (argument_count != 1) {
        return 0;
    }
    chara = now_script->chara;
    if (chara == NULL) {
        return 0;
    }
    SetStack(stack, chara->CheckMotionEnd());
    return 1;
}

/**
 * Sets the character light color for the running effect.
 */
static int _CHR_SET_LIGHT_COLOR(RS_STACKDATA *stack, int argument_count) {
    int       flags;
    mgCFrame *frame;
    float     red;
    float     green;
    float     blue;
    float     alpha;

    if (argument_count != 5 && argument_count != 4) {
        return 0;
    }
    mgCFrameAttr attr;
    if (now_script == NULL) {
        return 0;
    }
    frame = now_script->chara->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    red = GetStackFloat(stack++);
    green = GetStackFloat(stack++);
    blue = GetStackFloat(stack++);
    alpha = GetStackFloat(stack++);
    attr.color[0] = red;
    attr.color[1] = green;
    attr.color[2] = blue;
    attr.color[3] = alpha;
    flags = MG_FRAME_ATTR_COLOR;
    if (argument_count == 5) {
        flags |= MG_FRAME_ATTR_NO_LIGHT;
        attr.no_light = GetStackInt(stack);
    }
    frame->SetAttrParam(attr, 1, flags);
    return 1;
}

/**
 * Allocates the requested billboard sprites for the running effect.
 */
static int _SPT_ASSIGN_SPRITE(RS_STACKDATA *stack, int argument_count) {
    int         count;
    _ES_SPRITE *sprite;

    if (argument_count != 1 && argument_count != 2) {
        return 0;
    }
    count = GetStackInt(stack++);
    if (now_script->sprite != NULL) {
        return 0;
    }
    sprite = EffScriptMan->AssignSprite(count);
    if (sprite == NULL) {
        if (argument_count >= 2) {
            SetStack(stack, 0);
        }
        return 0;
    }
    if (argument_count >= 2) {
        SetStack(stack, 1);
    }
    now_script->sprite = sprite;
    now_script->sprite_num = count;
    return 1;
}

/**
 * Deletes the billboard sprites of the running effect.
 */
static int _SPT_DELETE_SPRITE(RS_STACKDATA *stack, int argument_count) {
    _ES_SPRITE *sprite = now_script->sprite;

    if (sprite == NULL) {
        return 0;
    }
    EffScriptMan->DeleteSprite(sprite);
    now_script->sprite = NULL;
    now_script->sprite_num = 0;
    return 1;
}

/**
 * Sets the sprite texture name for the running effect.
 */
static int _SPT_SET_TEXNAME(RS_STACKDATA *stack, int argument_count) {
    strcpy(now_script->tex_name, GetStackString(stack));
    return 1;
}

/**
 * Sets the sprite alpha blending mode for the running effect.
 */
static int _SPT_SET_ALPHAB(RS_STACKDATA *stack, int argument_count) {
    int         first;
    int         alpha;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    alpha = GetStackInt(stack++);
    if (argument_count >= 3) {
        count = GetStackInt(stack);
    }

    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->alpha = alpha;
    }
    return 1;
}

/**
 * Initializes a range of billboard sprites.
 */
static int _SPT_INIT_SPRITE(RS_STACKDATA *stack, int argument_count) {
    int         first;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    if (argument_count >= 2) {
        count = GetStackInt(stack);
    }
    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->draw_flag = 0;
        sprite->alpha = 1;
        sprite->pos[0] = 0;
        sprite->pos[1] = 0;
        sprite->pos[2] = 0;
        sprite->pos[3] = 1.0f;
        sprite->uv[0] = 0;
        sprite->uv[1] = 0;
        sprite->uv[2] = 0;
        sprite->uv[3] = 0;
        sprite->color[0] = 128.0f;
        sprite->color[1] = 128.0f;
        sprite->color[2] = 128.0f;
        sprite->color[3] = 128.0f;
        sprite->scale[1] = 1.0f;
        sprite->scale[0] = 1.0f;
        sprite->put_size[1] = 0;
        sprite->put_size[0] = 0;
        sprite->rotz = 0;
        sprite->velo_pos[0] = 0;
        sprite->velo_pos[1] = 0;
        sprite->velo_pos[2] = 0;
        sprite->velo_pos[3] = 0;
        sprite->acc_pos[0] = 0;
        sprite->acc_pos[1] = 0;
        sprite->acc_pos[2] = 0;
        sprite->acc_pos[3] = 0;
        sprite->velo_col[0] = 0;
        sprite->velo_col[1] = 0;
        sprite->velo_col[2] = 0;
        sprite->velo_col[3] = 0;
        sprite->acc_col[0] = 0;
        sprite->acc_col[1] = 0;
        sprite->acc_col[2] = 0;
        sprite->acc_col[3] = 0;
        sprite->color_target[0] = 0;
        sprite->color_target[1] = 0;
        sprite->color_target[2] = 0;
        sprite->color_target[3] = 0;
        sprite->color_conv_div = -1.0f;
        sprite->acc_rotz = 0;
        sprite->velo_rotz = 0;
        sprite->velo_scl[1] = 0;
        sprite->velo_scl[0] = 0;
        sprite->acc_scl[1] = 0;
        sprite->acc_scl[0] = 0;
        sprite->scale_target[1] = 0;
        sprite->scale_target[0] = 0;
        sprite->scale_conv_div = -1.0f;
        sprite->blink_amp[0] = 0;
        sprite->blink_amp[1] = 0;
        sprite->blink_amp[2] = 0;
        sprite->blink_amp[3] = 0;
        sprite->blink_speed = 0;
        sprite->blink_phase = 0;
    }
    return 1;
}

/**
 * Sets the sprite draw flag for the running effect.
 */
static int _SPT_SET_DRAW_FLAG(RS_STACKDATA *stack, int argument_count) {
    int         first;
    int         draw_flag;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    draw_flag = GetStackInt(stack++);
    if (argument_count >= 3) {
        count = GetStackInt(stack);
    }

    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->draw_flag = draw_flag;
    }
    return 1;
}

/**
 * Returns the sprite draw flag for the running effect.
 */
static int _SPT_GET_DRAW_FLAG(RS_STACKDATA *stack, int argument_count) {
    RS_STACKDATA *result_slot;
    _ES_SPRITE   *sprite;

    if (argument_count != 2) {
        return 0;
    }
    result_slot = stack + 1;
    sprite = GetSpritePtr(now_script, GetStackInt(stack));
    if (sprite == NULL) {
        return 0;
    }
    SetStack(result_slot, sprite->draw_flag);
    return 1;
}

/**
 * Sets the sprite uv size for the running effect.
 */
static int _SPT_SET_UV_SIZE(RS_STACKDATA *stack, int argument_count) {
    int           first;
    sceVu0FVECTOR value;
    int           count;
    int           i;
    _ES_SPRITE   *sprite;

    count = 1;
    first = GetStackInt(stack++);
    value[0] = GetStackFloat(stack++);
    value[1] = GetStackFloat(stack++);
    value[2] = GetStackFloat(stack++);
    value[3] = GetStackFloat(stack++);
    if (argument_count >= 6) {
        count = GetStackInt(stack);
    }
    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        *(u_long128 *)sprite->uv = *(u_long128 *)value;
    }
    return 1;
}

/**
 * Sets the sprite put size for the running effect.
 */
static int _SPT_SET_PUT_SIZE(RS_STACKDATA *stack, int argument_count) {
    int         first;
    float       put_size[2];
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    put_size[0] = GetStackFloat(stack++);
    put_size[1] = GetStackFloat(stack++);
    if (argument_count >= 4) {
        count = GetStackInt(stack);
    }

    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->put_size[0] = put_size[0];
        sprite->put_size[1] = put_size[1];
    }
    return 1;
}

/**
 * Sets the sprite position for the running effect.
 */
static int _SPT_SET_POS(RS_STACKDATA *stack, int argument_count) {
    int           first;
    sceVu0FVECTOR value;
    int           count;
    int           i;
    _ES_SPRITE   *sprite;

    count = 1;
    first = GetStackInt(stack++);
    GetStackVector(value, stack);
    stack += 3;
    if (argument_count >= 5) {
        count = GetStackInt(stack++);
    }
    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        *(u_long128 *)sprite->pos = *(u_long128 *)value;
    }
    return 1;
}

/**
 * Returns the sprite position for the running effect.
 */
static int _SPT_GET_POS(RS_STACKDATA *stack, int argument_count) {
    _ES_SPRITE *sprite;

    if (argument_count != 4) {
        return 0;
    }
    sprite = GetSpritePtr(now_script, GetStackInt(stack++));
    if (sprite == NULL) {
        return 0;
    }
    SetStack(stack++, sprite->pos[0]);
    SetStack(stack++, sprite->pos[1]);
    SetStack(stack, sprite->pos[2]);
    return 1;
}

/**
 * Sets the sprite rotz for the running effect.
 */
static int _SPT_SET_ROTZ(RS_STACKDATA *stack, int argument_count) {
    int         first;
    float       rotz;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    rotz = GetStackFloat(stack++);
    if (argument_count >= 3) {
        count = GetStackInt(stack);
    }

    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->rotz = rotz;
    }
    return 1;
}

/**
 * Returns the sprite rotz for the running effect.
 */
static int _SPT_GET_ROTZ(RS_STACKDATA *stack, int argument_count) {
    RS_STACKDATA *result_slot;
    _ES_SPRITE   *sprite;

    if (argument_count != 2) {
        return 0;
    }
    result_slot = stack + 1;
    sprite = GetSpritePtr(now_script, GetStackInt(stack));
    if (sprite == NULL) {
        return 0;
    }
    SetStack(result_slot, sprite->rotz);
    return 1;
}

/**
 * Sets the sprite scale for the running effect.
 */
static int _SPT_SET_SCALE(RS_STACKDATA *stack, int argument_count) {
    int         first;
    float       scale[2];
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    scale[0] = GetStackFloat(stack++);
    scale[1] = GetStackFloat(stack++);
    if (argument_count >= 4) {
        count = GetStackInt(stack);
    }

    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->scale[0] = scale[0];
        sprite->scale[1] = scale[1];
    }
    return 1;
}

/**
 * Returns the sprite scale for the running effect.
 */
static int _SPT_GET_SCALE(RS_STACKDATA *stack, int argument_count) {
    _ES_SPRITE *sprite;

    if (argument_count != 3) {
        return 0;
    }
    sprite = GetSpritePtr(now_script, GetStackInt(stack++));
    if (sprite == NULL) {
        return 0;
    }
    SetStack(stack++, sprite->scale[0]);
    SetStack(stack, sprite->scale[1]);
    return 1;
}

/**
 * Sets the sprite color for the running effect.
 */
static int _SPT_SET_COLOR(RS_STACKDATA *stack, int argument_count) {
    int           first;
    sceVu0FVECTOR value;
    int           count;
    int           i;
    _ES_SPRITE   *sprite;

    count = 1;
    first = GetStackInt(stack++);
    value[0] = GetStackFloat(stack++);
    value[1] = GetStackFloat(stack++);
    value[2] = GetStackFloat(stack++);
    value[3] = GetStackFloat(stack++);
    if (argument_count >= 6) {
        count = GetStackInt(stack);
    }
    for (i = first; i < count + first; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        *(u_long128 *)sprite->color = *(u_long128 *)value;
    }
    return 1;
}

/**
 * Returns the sprite color for the running effect.
 */
static int _SPT_GET_COLOR(RS_STACKDATA *stack, int argument_count) {
    _ES_SPRITE *sprite;

    if (argument_count != 5) {
        return 0;
    }
    sprite = GetSpritePtr(now_script, GetStackInt(stack++));
    if (sprite == NULL) {
        return 0;
    }
    SetStack(stack++, sprite->color[0]);
    SetStack(stack++, sprite->color[1]);
    SetStack(stack++, sprite->color[2]);
    SetStack(stack, sprite->color[3]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_VAN_SET_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_VAN_SET_ROT__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_VAN_SET_COL__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_VAN_SET_SCL__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_ADD_POS__FP12RS_STACKDATAi);
/**
 * Adds the sprite rotz for the running effect.
 */
static int _SPT_ADD_ROTZ(RS_STACKDATA *stack, int argc) {
    int         first;
    float       angle;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    angle = GetStackFloat(stack++);
    if (argc >= 3) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->rotz += angle;
        sprite->rotz = mgAngleLimit(sprite->rotz);
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_ADD_COLOR__FP12RS_STACKDATAi);
/**
 * Rotates sprite positions and adds the effect origin.
 */
static int _SPT_WORLD_ROT(RS_STACKDATA *stack, int argc) {
    int         first;
    float       angle;
    int         count;
    int         i;
    _ES_SPRITE *sprite;
    float       matrix[4][4];

    count = 1;
    first = GetStackInt(stack++);
    angle = GetStackFloat(stack++);
    if (argc >= 3) {
        count = GetStackInt(stack);
    }
    sceVu0UnitMatrix(matrix);
    mgRotMatrixY(matrix, angle);
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sceVu0ApplyMatrix(sprite->pos, matrix, sprite->pos);
    }
    return 1;
}

/**
 * Sets the sprite life for the running effect.
 */
static s32 _SPT_SET_LIFE(RS_STACKDATA *stack, int argument_count) {
    return 0;

}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_SET_VELO_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_SET_ACC_POS__FP12RS_STACKDATAi);
/**
 * Sets the sprite velocity rotz for the running effect.
 */
static int _SPT_SET_VELO_ROTZ(RS_STACKDATA *stack, int argc) {
    int         first;
    float       value;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    value = GetStackFloat(stack++);
    if (argc >= 3) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->velo_rotz = value;
    }
    return 1;
}

/**
 * Sets the sprite acceleration rotz for the running effect.
 */
static int _SPT_SET_ACC_ROTZ(RS_STACKDATA *stack, int argc) {
    int         first;
    float       value;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    value = GetStackFloat(stack++);
    if (argc >= 3) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->acc_rotz = value;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_SET_VELO_COL__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_SET_ACC_COL__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_SET_BLINKING__FP12RS_STACKDATAi);
/**
 * Sets the sprite velocity scale for the running effect.
 */
static int _SPT_SET_VELO_SCL(RS_STACKDATA *stack, int argc) {
    int         first;
    float       x;
    float       y;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    x = GetStackFloat(stack++);
    y = GetStackFloat(stack++);
    if (argc >= 4) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->velo_scl[0] = x;
        sprite->velo_scl[1] = y;
    }
    return 1;
}

/**
 * Sets the sprite acceleration scale for the running effect.
 */
static int _SPT_SET_ACC_SCL(RS_STACKDATA *stack, int argc) {
    int         first;
    float       x;
    float       y;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    x = GetStackFloat(stack++);
    y = GetStackFloat(stack++);
    if (argc >= 4) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->acc_scl[0] = x;
        sprite->acc_scl[1] = y;
    }
    return 1;
}

/**
 * Sets the scale target and convergence divisor of a range of sprites.
 */
static int _SPT_SCALE_CONV(RS_STACKDATA *stack, int argc) {
    int         first;
    float       time;
    float       target_x;
    float       target_y;
    int         count;
    int         i;
    _ES_SPRITE *sprite;

    count = 1;
    first = GetStackInt(stack++);
    target_x = GetStackFloat(stack++);
    target_y = GetStackFloat(stack++);
    time = GetStackFloat(stack++);
    if (argc >= 5) {
        count = GetStackInt(stack);
    }
    for (i = first; i < first + count; i++) {
        sprite = GetSpritePtr(now_script, i);
        if (sprite == NULL) {
            return 0;
        }
        sprite->scale_target[0] = target_x;
        sprite->scale_target[1] = target_y;
        sprite->scale_conv_div = time;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SPT_COLOR_CONV__FP12RS_STACKDATAi);
/**
 * Returns the scene character position for the running effect.
 */
static int _SCN_GET_CHR_POS(RS_STACKDATA *stack, int argc) {
    float        pos[3];
    CCharacter2 *chara;

    if (argc != 4) {
        return 0;
    }
    chara = now_scene->GetCharacter(GetStackInt(stack++));
    if (chara == NULL) {
        return 0;
    }
    chara->GetPosition(pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Returns the scene character rotation for the running effect.
 */
static int _SCN_GET_CHR_ROT(RS_STACKDATA *stack, int argc) {
    float        rot[3];
    CCharacter2 *chara;

    if (argc != 2 && argc != 4) {
        return 0;
    }
    chara = now_scene->GetCharacter(GetStackInt(stack++));
    if (chara == NULL) {
        return 0;
    }
    chara->GetRotation(rot);
    switch (argc) {
        case 2:
            SetStack(stack, rot[1]);
            break;
        case 4:
            SetStack(stack++, rot[0]);
            SetStack(stack++, rot[1]);
            SetStack(stack, rot[2]);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Returns the scene character frame position for the running effect.
 */
static int _SCN_GET_CHR_FRM_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    int           chara_slot;
    char         *frame_name;
    CCharacter2  *chara;
    mgCFrame     *frame;

    if (argc != 5) {
        return 0;
    }
    chara_slot = GetStackInt(stack++);
    frame_name = GetStackString(stack++);
    chara = now_scene->GetCharacter(chara_slot);
    if (chara == NULL) {
        return 0;
    }
    if (chara->CObjectFrame::frame == NULL) {
        return 0;
    }
    frame = chara->CObjectFrame::frame->SearchFrame(frame_name);
    if (frame == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SCN_GET_CHR_FRM_DIR__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _SCN_GET_CHR_FRM_ROT__FP12RS_STACKDATAi);
/**
 * Returns the scene entry obj position for the running effect.
 */
static int _SCN_GET_ENTRY_OBJ_POS(RS_STACKDATA *stack, int argc) {
    float        pos[3];
    int          chara_slot;
    int          entry_index;
    CCharacter2 *chara;

    if (argc != 5) {
    return 0;
}
    chara_slot = GetStackInt(stack++);
    entry_index = GetStackInt(stack++);
    if (entry_index < 0 || entry_index > 1) {
    return 0;
}
    chara = now_scene->GetCharacter(chara_slot);
    if (chara == NULL) {
    return 0;
}
    chara->GetEntryObjectPos(entry_index, pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", _INTERSECTION_POINT__FP12RS_STACKDATAi);
/**
 * Plays the monster sound for the running effect.
 */
static int _MON_SE_PLAY(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    float         volume;
    float         pan;
    CCharacter2  *owner;
    int           se_id;
    u_int         se_handle;
    int           slot = now_script->user_id;

    if (slot <= -1) {
        return 0;
    }
    owner = now_scene->GetCharacter(slot);
    if (owner == NULL) {
        return 0;
    }
    se_handle = owner->se_bank;
    se_id = GetStackInt(stack++);
    switch (argc) {
        case 1:
            sndSePlay(se_handle, se_id, 0);
            break;
        case 4:
            GetStackVector(position, stack);
            sndGetVolPan(&volume, &pan, position, 160.0f, 1200.0f);
            sndSePlayVPf(se_handle, se_id, volume, pan, 0);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Stops the monster sound for the running effect.
 */
static int _MON_SE_STOP(RS_STACKDATA *stack, int argc) {
    CCharacter2 *owner;
    int          slot = now_script->user_id;
    u_int        se_handle;

    if (slot <= -1) {
        return 0;
    }
    owner = now_scene->GetCharacter(slot);
    if (owner == NULL) {
        return 0;
    }
    se_handle = owner->se_bank;
    sndSeStop(se_handle, GetStackInt(stack), 0);
    return 1;
}

/**
 * Plays the battle sound for the running effect.
 */
static int _BTL_SE_PLAY(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    float         volume;
    float         pan;
    float         near_distance = 160.0f;
    float         far_distance = 1200.0f;
    int           se_id;
    u_int         se_handle = now_scene->se_battle_id;

    se_id = GetStackInt(stack++);
    switch (argc) {
        case 1:
            sndSePlay(se_handle, se_id, 0);
            break;
        case 4:
            GetStackVector(position, stack);
            sndGetVolPan(&volume, &pan, position, near_distance, far_distance);
            sndSePlayVPf(se_handle, se_id, volume, pan, 0);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Stops the battle sound for the running effect.
 */
static int _BTL_SE_STOP(RS_STACKDATA *stack, int argc) {
    u_int se_handle;

    se_handle = now_scene->se_battle_id;
    sndSeStop(se_handle, GetStackInt(stack), 0);
    return 1;
}

/**
 * Plays the base sound for the running effect.
 */
static int _BSE_SE_PLAY(RS_STACKDATA *stack, int argc) {
    u_int se_handle;

    se_handle = now_scene->se_base_id;
    sndSePlay(se_handle, GetStackInt(stack), 0);
    return 1;
}

/**
 * Stops the base sound for the running effect.
 */
static int _BSE_SE_STOP(RS_STACKDATA *stack, int argc) {
    u_int se_handle;

    se_handle = now_scene->se_base_id;
    sndSeStop(se_handle, GetStackInt(stack), 0);
    return 1;
}

/**
 * Handles mon se play2 for the running effect.
 */
static int _MON_SE_PLAY2(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    float         volume;
    float         pan;
    float         near_distance = 160.0f;
    float         far_distance = 1200.0f;
    CCharacter2  *owner;
    int           se_id;
    u_int         se_handle;

    if (argc != 2 && argc != 5) {
        return 0;
    }
    owner = now_scene->GetCharacter(GetStackInt(stack++));
    if (owner == NULL) {
        return 0;
    }
    se_handle = owner->se_bank;
    se_id = GetStackInt(stack++);
    switch (argc) {
        case 2:
            sndSePlay(se_handle, se_id, 0);
            break;
        case 5:
            GetStackVector(position, stack);
            sndGetVolPan(&volume, &pan, position, near_distance, far_distance);
            sndSePlayVPf(se_handle, se_id, volume, pan, 0);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Handles mon se stop2 for the running effect.
 */
static int _MON_SE_STOP2(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *second = stack + 1;
    CCharacter2  *owner;
    u_int         se_handle;

    owner = now_scene->GetCharacter(GetStackInt(stack));
    if (owner == NULL) {
        return 0;
    }
    se_handle = owner->se_bank;
    sndSeStop(se_handle, GetStackInt(second), 0);
    return 1;
}

/**
 * Selects scene lighting for the running effect sprites.
 */
static int _SET_LIGHT_FLAG(RS_STACKDATA *stack, int argc) {
    now_script->light_flag = GetStackInt(stack);
    return 1;
}

/**
 * Returns the scene character entry object position for the running effect.
 */
static int _SCN_GET_CHR_ENTOBJ_POS(RS_STACKDATA *stack, int argc) {
    float        pos[3];
    int          chara_slot;
    int          entry_index;
    CCharacter2 *chara;

    if (argc != 5) {
        return 0;
    }
    chara_slot = GetStackInt(stack++);
    entry_index = GetStackInt(stack++);
    chara = now_scene->GetCharacter(chara_slot);
    if (chara == NULL) {
        return 0;
    }
    chara->GetEntryObjectPos(entry_index, pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Reports that scripted damage creation is unavailable.
 */
static int _CREATE_DAMAGE(RS_STACKDATA *stack, int argc) {
    printf("effect script err   \214\303\202\242\203R\203\212\203W\203\207\203\223\203N\203\211\203X\202\315\216g\202\301\202\277\202\341\203_\203\201\202\346\201I\201I\n");
    return 0;
}

/**
 * Handles delete damage for the running effect.
 */
static s32 _DELETE_DAMAGE(RS_STACKDATA *stack, int argument_count) {
    return 0;

}

/**
 * Sets the dmg position for the running effect.
 */
static s32 _DMG_SET_POS(RS_STACKDATA *stack, int argument_count) {
    return 0;

}

/**
 * Sets the dmg front vect for the running effect.
 */
static s32 _DMG_SET_FRONT_VECT(RS_STACKDATA *stack, int argument_count) {
    return 0;

}

/**
 * Reports that scripted damage assignment is unavailable.
 */
static int _DMG_SET_DAMAGE(RS_STACKDATA *stack, int argc) {
    printf("effect script err   \214\303\202\242\203R\203\212\203W\203\207\203\223\203N\203\211\203X\202\315\216g\202\301\202\277\202\341\203_\203\201\202\346\201I\201I\n");
    return 0;
}

/**
 * Creates a collision primitive owned by the running effect.
 */
static int _COLPRIM_CREATE(RS_STACKDATA *stack, int argc) {
    CColPrim *colprim;
    int       owner;
    char     *damage_name;

    if (now_script->colprim != NULL) {
        now_script->colprim->Delete(now_script->user_id);
    }
    colprim = ColPrimMan.GetPrim();
    now_script->colprim = colprim;
    if (colprim == NULL) {
        return 0;
    }
    owner = now_script->user_id;
    damage_name = GetStackString(stack++);
    if (argc >= 2) {
        owner = GetStackInt(stack);
    }
    now_script->colprim->SetDamage(damage_name, owner);
    return 1;
}

/**
 * Sets the coordinates and radius of the effect collision primitive.
 */
static int _COLPRIM_SET_COORD(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR start;
    sceVu0FVECTOR end;
    float         radius;
    mgCFrame     *root;
    mgCFrame     *frame;
    char         *start_name;
    char         *end_name;
    mgCFrame     *first_frame;
    mgCFrame     *second_frame;

    if (now_script->colprim == NULL) {
        return 0;
    }
    switch (argc) {
        case 4:
            GetStackVector(start, stack);
            radius = GetStackFloat(stack += 3);
            now_script->colprim->SetCoord(start, radius);
            break;
        case 7:
            GetStackVector(start, stack);
            GetStackVector(end, stack + 3);
            radius = GetStackFloat(stack += 6);
            now_script->colprim->SetCoord(start, end, radius);
            break;
        case 2:
            if (now_script->chara == NULL) {
                return 0;
            }
            start_name = GetStackString(stack++);
            radius = GetStackFloat(stack);
            root = now_script->chara->CObjectFrame::frame;
            if (root == NULL) {
                return 0;
            }
            frame = root->SearchFrame(start_name);
            if (frame == NULL) {
                return 0;
            }
            now_script->colprim->SetCoord(frame, radius);
            break;
        case 3:
            if (now_script->chara == NULL) {
                return 0;
            }
            start_name = GetStackString(stack++);
            end_name = GetStackString(stack++);
            radius = GetStackFloat(stack);
            root = now_script->chara->CObjectFrame::frame;
            if (root == NULL) {
                return 0;
            }

            if ((first_frame = root->SearchFrame(start_name)) == NULL) {
                return 0;
            }
            if ((second_frame = root->SearchFrame(end_name)) == NULL) {
                return 0;
            }
            now_script->colprim->SetCoord(first_frame, second_frame, radius);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Deletes the collision primitive of the running effect.
 */
static int _COLPRIM_DELETE(RS_STACKDATA *stack, int argc) {
    CColPrim *colprim;

    if (now_script == NULL) {
        return 0;
    }
    GetStackInt(stack);
    colprim = now_script->colprim;
    if (colprim == NULL) {
        return 0;
    }
    colprim->Delete(-1);
    now_script->colprim = NULL;
    return 1;
}

/**
 * Returns the hit count of the effect collision primitive.
 */
static int _COLPRIM_GET_HITCNT(RS_STACKDATA *stack, int argc) {
    CColPrim *colprim;

    if (argc != 1) {
        return 0;
    }
    colprim = now_script->colprim;
    if (colprim == NULL) {
        return 0;
    }
    SetStack(stack, colprim->hit_num);
    return 1;
}

/**
 * Sets the item, count and rate of a gift from the effect collision primitive.
 */
static int _COLPRIM_GET_GIFT(RS_STACKDATA *stack, int argc) {
    int           item_id;
    int           count;
    int           rate;
    RS_STACKDATA *next_slot;
    CColPrim     *colprim;

    if (argc != 3) {
        return 0;
    }
    next_slot = stack + 1;
    if (now_script->colprim == NULL) {
        return 0;
    }
    item_id = GetStackInt(stack);
    count = GetStackInt(next_slot++);
    rate = GetStackInt(next_slot);
    colprim = now_script->colprim;
    colprim->gift[0] = item_id;
    colprim->gift[1] = count;
    colprim->gift[2] = rate;
    colprim->has_gift = 1;
    printf("%d,%d,%d\n", item_id, count, rate);
    return 1;
}

/**
 * Returns the reverse count and optional vector of the effect collision primitive.
 */
static int _COLPRIM_GET_REVCNT(RS_STACKDATA *stack, int argc) {
    if (argc != 4 && argc != 1) {
        return 0;
    }
    if (now_script->colprim == NULL) {
        return 0;
    }
    SetStack(stack++, now_script->colprim->reversed);
    if (argc == 4) {
        SetStack(stack++, now_script->colprim->revers_vec[0]);
        SetStack(stack++, now_script->colprim->revers_vec[1]);
        SetStack(stack, now_script->colprim->revers_vec[2]);
    }
    return 1;
}

/**
 * Sets the damage of the effect collision primitive.
 */
static int _COLPRIM_SET_DAMAGE(RS_STACKDATA *stack, int argc) {
    if (now_script->colprim == NULL) {
        return 0;
    }
    now_script->colprim->damage = GetStackInt(stack);
    return 1;
}

/**
 * Returns the hit position of the effect collision primitive.
 */
static int _COLPRIM_GET_HIT_POS(RS_STACKDATA *stack, int argc) {
    if (argc != 3) {
        return 0;
    }
    if (now_script->colprim == NULL) {
        return 0;
    }
    SetStack(stack++, now_script->colprim->hit_pos[0]);
    SetStack(stack++, now_script->colprim->hit_pos[1]);
    SetStack(stack, now_script->colprim->hit_pos[2]);
    return 1;
}

/**
 * Starts an effect and returns its slot.
 */
static int _ES_CREATE(RS_STACKDATA *stack, int argc) {
    int   handle = -1;
    char *name = GetStackString(stack++);
    int   user_id;

    switch (argc) {
        case 1:
            EffScriptMan->CreateEffSpt(name, now_script->user_id, 0);
            break;
        case 2: {
            user_id = now_script->user_id;
            if (user_id >= 0) {
                handle = EffScriptMan->CreateEffSpt(name, user_id, 1);
            }
            SetStack(stack, handle);
            if (handle <= -1) {
                printf("ES_CREATE error [%s][%d][%d]", name, now_script->user_id);
            }
            break;
        }
        default:
            return 0;
    }
    return 1;
}

/**
 * Sets the first work vector of an effect.
 */
static int _ES_SET_VECT1(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR vect;
    int           target_id;

    switch (argc) {
        case 3:
            GetStackVector(vect, stack);
            EffScriptMan->SetScriptVect1(vect, now_script->user_id, -1);
            break;
        case 4:
            target_id = GetStackInt(stack++);
            GetStackVector(vect, stack);
            EffScriptMan->SetScriptVect1(vect, now_script->user_id, target_id);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Sets the second work vector of an effect.
 */
static int _ES_SET_VECT2(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR vect;
    int           target_id;

    switch (argc) {
        case 3:
            GetStackVector(vect, stack);
            EffScriptMan->SetScriptVect2(vect, now_script->user_id, -1);
            break;
        case 4:
            target_id = GetStackInt(stack++);
            GetStackVector(vect, stack);
            EffScriptMan->SetScriptVect2(vect, now_script->user_id, target_id);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Sets the target character of an effect.
 */
static int _ES_SET_TARGET_ID(RS_STACKDATA *stack, int argc) {
    int source_id;

    switch (argc) {
        case 1:
            EffScriptMan->SetScriptTargetId(GetStackInt(stack), now_script->user_id, -1);
            break;
        case 2: {
            source_id = GetStackInt(stack++);
            EffScriptMan->SetScriptTargetId(GetStackInt(stack), now_script->user_id, source_id);
            break;
        }
        default:
            return 0;
    }
    return 1;
}

/**
 * Stores an integer or float in an effect value slot.
 */
static int _ES_SET_VALUE(RS_STACKDATA *stack, int argc) {
    int target_id = -1;
    int index;

    switch (argc) {
        case 2:
            break;
        case 3:
            target_id = GetStackInt(stack++);
            break;
        default:
            return 0;
    }
    index = GetStackInt(stack++);
    switch (stack->type) {
        case RS_INT:
            EffScriptMan->SetValue(index, GetStackInt(stack), now_script->user_id, target_id);
            break;
        case RS_FLOAT:
            EffScriptMan->SetValue(index, GetStackFloat(stack), now_script->user_id, target_id);
            break;
        default:
            return 0;
    }
    return 1;
}

/**
 * Assigns the running effect collision primitive to the current effect.
 */
static int _ES_SET_COLPRIM(RS_STACKDATA *stack, int argc) {
    CColPrim *colprim;

    colprim = now_script->colprim;
    if (colprim == NULL) {
        return 0;
    }
    EffScriptMan->SetColPrim(colprim, now_script->user_id, -1);
    return 1;
}

/**
 * Returns the position of an event object handle.
 */
static int _GET_EOH_POS(RS_STACKDATA *stack, int argc) {
    float pos[3];

    if (argc != 4) {
        return 0;
    }
    EventObjHandleMother.GetPos(GetStackInt(stack++), pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Assigns a program and arena-backed stacks to an effect interpreter.
 */
static int SetEffectScript(CRunScript *script, char *program, mgCMemory *memory) {
    RS_STACKDATA *stack = (RS_STACKDATA *)memory->Alloc(0x20);
    RS_CALLDATA  *callData = (RS_CALLDATA *)memory->Alloc(1);

    script->load((RS_PROG_HEADER *)program, stack, 0x40, callData, 2);
    script->ext_func(ext_func, 0x100);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/effscript", SetEffectScriptFunc__Fv);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", eff_spt_base_def__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_2311__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_2498__2__DATA);


// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_943__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1099__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1100__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1101__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1102__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1103__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1104__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1127__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1128__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1129__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1143__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1144__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1145__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1336__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1337__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1338__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1339__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1340__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1341__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1655__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_1705__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_2025__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3303__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3304__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3398__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3495__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3536__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3644__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/effscript", at_3645__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(now_scene, 0x4);
INCLUDE_BSS(EffScriptMan, 0x4);
INCLUDE_BSS(now_script, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(ext_func__4, 0x400);
INCLUDE_BSS(at_2067, 0x10);
