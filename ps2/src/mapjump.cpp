#include "common.h"
#include "mapjump.hpp"

#include <cstdio>
#include <cstring>

#include "dataread.hpp"
#include "editmap.hpp"
#include "event.hpp"
#include "mainloop.hpp"
#include "mapselect.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "sceneload.hpp"
#include "scenesnd.hpp"
#include "vlgr_info.hpp"

static int NowMainMapNo;
static int NowSubMapNo;
static int NowInteriorMapNo;
static int OldInteriorMapNo;
static mgCMemory * ScriptBuffer;
static int InteriorFlag;
static MapJumpMapInfo MainMapInfo;
static MapJumpMapInfo SubMapInfo;
static char now_script_file[0x40];
static char old_mapname[0x40];
static char PrevInterior[0x40];
static char NowInterior[0x40];

// Code (.text)
int GetMainMapNo(void) {
    return NowMainMapNo;
}

int GetSubMapNo(void) {
    return NowSubMapNo;
}

void ClearSubMapNo(void) {
    NowSubMapNo = -1;
}

MapJumpMapInfo::MapJumpMapInfo() {
    memset(this, 0, sizeof(*this));
}

void SetMainMapInfo(MapJumpMapInfo *info) {
    MainMapInfo.map_no = info->map_no;
    MainMapInfo.tex_block = info->tex_block;
    MainMapInfo.stack_no = info->stack_no;
    MainMapInfo.efp_tex_block = info->efp_tex_block;
    MainMapInfo.sky_tex_block = info->sky_tex_block;
    MainMapInfo.load_buf = info->load_buf;
}

void SetSubMapInfo(MapJumpMapInfo *info) {
    SubMapInfo.map_no = info->map_no;
    SubMapInfo.tex_block = info->tex_block;
    SubMapInfo.stack_no = info->stack_no;
    SubMapInfo.efp_tex_block = info->efp_tex_block;
    SubMapInfo.sky_tex_block = info->sky_tex_block;
    SubMapInfo.load_buf = info->load_buf;
}

void SetScriptBuffer(mgCMemory *buffer) {
    ScriptBuffer = buffer;
}

void PreLoadSync(void) {
    ReadBG();
    ReadBGSync();
}

#ifdef NONMATCHING
int MapJump(CScene *scene, SCN_LOADMAP_INFO2 *info, int map_no) {
    char *map_name = GetMapName(map_no, NULL);
    if (map_name == NULL) {
        printf("not found map %d\n", map_no);
        return 0;
    }
    scene->StopSeSrc();
    sndSeAllStop(1);
    int chara_index;

    CSaveData *save_data = GetSaveData();
    save_data->prev_map_no = NowMainMapNo;
    mgWaitFrame();
    mgInitLighting();
    scene->DeleteMap(SubMapInfo.map_no, 1);
    scene->DeleteMap(MainMapInfo.map_no, 1);
    NowMainMapNo = -1;
    NowSubMapNo = -1;
    for (chara_index = 0; chara_index < 0x38; chara_index++) {
        scene->DeleteChara(chara_index + 8);
    }
    scene->ClearStack(1);
    NowMainMapNo = SearchMapNo(map_name);
    scene->SetNowMapNo(NowMainMapNo);
    save_data->map_no = NowMainMapNo;
    save_data->prev_sub_map_no = -1;
    save_data->sub_map_no = -1;
    int area_no = GetMapAreaNo(NowMainMapNo);
    if (area_no > 0) {
        save_data->area_no = area_no;
    }
    info->load_sky = 1;
    if (scene->LoadMap(MainMapInfo.map_no, info, 0) < 0) {
        return 0;
    }
    scene->SetActive(2, MainMapInfo.map_no);
    scene->active_map = MainMapInfo.map_no;
    CEditMap *map = (CEditMap *)scene->GetMap(scene->active_map);
    if (map != NULL) {
        map->now_time = scene->time;
    }
    LoadMapScript(map_name);
    InitInterior();
    NowInteriorMapNo = -1;
    OldInteriorMapNo = -1;
    GetSaveData()->ResetBitCtrl(1);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mapjump", MapJump__FP6CSceneP17SCN_LOADMAP_INFO2i);
#endif

int GetLoadMapInfo(SCN_LOADMAP_INFO2 *info, int map_no) {
    char map_path[64];
    char file_name[32];
    char add_path[128];
    char add_directory[64];
    char *map_name = GetMapName(map_no, NULL);
    if (map_name == NULL) {
        printf("not found map %d\n", map_no);
        return 0;
    }
    info->tex_block = MainMapInfo.tex_block;
    info->stack_no = MainMapInfo.stack_no;
    info->load_buf = MainMapInfo.load_buf;
    info->efp_tex_block = MainMapInfo.efp_tex_block;
    info->sky_tex_block = MainMapInfo.sky_tex_block;
    if (info->place_parts_max <= 0) {
        info->place_parts_max = 0x140;
    }
    GAME_PROGRESS_INFO *progress = GetGameProgressInfo(GetSaveData()->game_progress);
    GetMapPath(map_path, map_name);
    DivPathName(map_path, info->files[0].dir, file_name);
    info->files[0].enable = 1;
    strcpy(info->files[0].map_name, file_name);
    strcpy(info->files[0].cfg_name, file_name);
    strcpy(info->files[0].mpk_name, file_name);
    strcpy(info->files[0].ipk_name, file_name);
    strcpy(info->files[0].efp_name, file_name);
    strcpy(info->files[0].sky_name, file_name);
    strcpy(info->files[0].def_sky_name, "def");
    if (progress != NULL) {
        s16 chapter = progress->chapter;
        if (chapter >= 8 && chapter < 10) {
            strcat(info->files[0].sky_name, "b");
            strcat(info->files[0].def_sky_name, "b");
        }
    }
    strcpy(info->name, file_name);
    char *add_map_path = GetAddMapPath(map_no);
    if (add_map_path != NULL && *add_map_path != 0) {
        strcpy(add_path, add_map_path);
        info->files[1].enable = 1;
        if (progress != NULL) {
            s16 chapter = progress->chapter;
            if (chapter >= 6) {
                if (chapter < 8 && strcmp(add_path, "trn/train") == 0) {
                    strcpy(add_path, "trn2/s36");
                }
            }
        }
        DivPathName(add_path, add_directory, file_name);
        strcpy(info->files[1].dir, "map/cmn/");
        strcat(info->files[1].dir, add_directory);
        strcpy(info->files[1].map_name, file_name);
        strcpy(info->files[1].cfg_name, file_name);
        strcpy(info->files[1].mpk_name, file_name);
        strcpy(info->files[1].ipk_name, file_name);
        strcpy(info->files[1].efp_name, file_name);
    }
    return 1;
}

#ifdef NONMATCHING
int LoadSubMap(CScene *scene, int map_no, int background) {
    SCN_LOADMAP_INFO2 info;
    char map_path[64];
    char file_name[32];
    char *map_name = GetMapName(map_no, NULL);
    if (map_name == NULL) {
        printf("not found map %d\n", map_no);
        return 0;
    }
    mgWaitFrame();
    scene->DeleteMap(SubMapInfo.map_no, 1);
    scene->DeleteSubVillager();
    info.Initialize();
    info.tex_block = SubMapInfo.tex_block;
    info.stack_no = SubMapInfo.stack_no;
    info.load_buf = SubMapInfo.load_buf;
    info.efp_tex_block = SubMapInfo.efp_tex_block;
    strcpy(info.name, map_name);
    if (info.place_parts_max <= 0) {
        info.place_parts_max = 0x140;
    }
    GetMapPath(map_path, map_name);
    DivPathName(map_path, info.files[0].dir, file_name);
    info.files[0].enable = 1;
    strcpy(info.files[0].map_name, file_name);
    strcpy(info.files[0].cfg_name, file_name);
    strcpy(info.files[0].mpk_name, file_name);
    strcpy(info.files[0].ipk_name, file_name);
    strcpy(info.files[0].efp_name, file_name);
    strcpy(info.name, file_name);
    if (scene->LoadMap(SubMapInfo.map_no, &info, background) < 0) {
        return 0;
    }
    CSaveData *save_data = GetSaveData();

    save_data->prev_sub_map_no = NowSubMapNo;
    scene->SetNowSubMapNo(map_no);
    NowSubMapNo = map_no;
    save_data->sub_map_no = map_no;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mapjump", LoadSubMap__FP6CSceneii);
#endif

void LoadMapScript(char *map_name) {
    char map_path[0x80];
    char script[0x80] = "";
    GetMapPath(map_path, map_name);
    strcat(script, map_path);
    strcat(script, ".stb");
    LoadScript(script);
    strcpy(now_script_file, script);
}

void ReloadMapScript(void) {
    if (now_script_file[0] != '\0') {
        LoadScript(now_script_file);
    }
}

void LoadScript(char *file_name) {
    char localized_path[0x100];
    char language_suffix[0x1C];
    int file_size;

    {
        mgCMemory *memory = ScriptBuffer;
        memory->stReset();
    }
    ScriptBuffer->Align64();
    u8 *buffer = (u8 *)ScriptBuffer->stGetTop();
    int length = strlen(file_name);
    if (length >= 5) {
        strncpy(localized_path, file_name, length - 4);
        localized_path[length - 4] = 0;
        sprintf(language_suffix, "_%d.stb", LanguageCode);
        strcat(localized_path, language_suffix);
        if (LoadFile2(localized_path, buffer, &file_size, 0) != 0) {
            u32 blocks;
            if (file_size & 0xF) {
                blocks = ((u32)file_size >> 4) + 1;
            } else {
                blocks = (u32)file_size >> 4;
            }
            ScriptBuffer->Alloc(blocks);
            SetEventScript((char *)buffer, NULL, ScriptBuffer);
            return;
        }

        if (LoadFile2(file_name, buffer, &file_size, 0) != 0) {
            u32 blocks;
            if (file_size & 0xF) {
                blocks = ((u32)file_size >> 4) + 1;
            } else {
                blocks = (u32)file_size >> 4;
            }
            ScriptBuffer->Alloc(blocks);
            SetEventScript((char *)buffer, NULL, ScriptBuffer);
            return;
        }
        SetEventScript(NULL, NULL, NULL);
    }
}

int GetOldInteriorMapNo(void) {
    if (InInterior() != 0) {
        return -1;
    }
    return OldInteriorMapNo;
}

void InitInterior(void) {
    old_mapname[0] = 0;
    InteriorFlag = 0;
    PrevInterior[0] = 0;
    NowInterior[0] = 0;
}

int InInterior(void) {
    return InteriorFlag;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mapjump", SaveBeforeInterior__FP6CScene);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mapjump", SetInteriorDoorPos__FP6CScene);
void GotoInterior(CScene *scene, int map_no) {
    char *map_name = GetMapName(map_no, NULL);
    if (map_name != NULL && InInterior() == 0) {
        mgWaitFrame();
        SaveBeforeInterior(scene);
        if (LoadSubMap(scene, map_no, 0) != 0) {
            scene->SetActive(2, SubMapInfo.map_no);
            scene->ResetActive(2, MainMapInfo.map_no);
            scene->active_map = SubMapInfo.map_no;
            SetInteriorDoorPos(scene);
        }
        if (GetMapType(map_no) == 2) {
            LoadMapScript("g00");
        } else {
            LoadMapScript(map_name);
        }
        scene->SetNowMapNo(-1);
        scene->SetNowMapNo(NowMainMapNo);
        OldInteriorMapNo = NowInteriorMapNo;
        NowInteriorMapNo = NowMainMapNo;
        CEditMap *map = (CEditMap *)scene->GetMap(scene->active_map);
        if (map != NULL) {
            map->now_time = scene->time;
        }
        strcpy(NowInterior, map_name);
        PrevInterior[0] = 0;
        InteriorFlag = 1;
    }
}

void DeleteInterior(CScene *scene) {
    if (InInterior() != 0) {
        mgWaitFrame();
        scene->DeleteMap(SubMapInfo.map_no, 1);
        scene->DeleteSubVillager();
        NowSubMapNo = -1;
        s16 *sub_map_no = &GetSaveData()->sub_map_no;
        *sub_map_no = -1;
    }
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/mapjump", ExitInterior__FP6CScenePi);
int InteriorMapJump(CScene *scene, int map_no) {
    if (LoadSubMap(scene, map_no, 0) != 0) {
        scene->SetActive(2, SubMapInfo.map_no);
        scene->ResetActive(2, MainMapInfo.map_no);
        scene->active_map = SubMapInfo.map_no;
        char *map_name = GetMapName(map_no, NULL);
        strcpy(PrevInterior, NowInterior);
        if (map_name != NULL) {
            strcpy(NowInterior, map_name);
        } else {
            NowInterior[0] = 0;
        }
        SetInteriorDoorPos(scene);
        LoadMapScript(map_name);
        OldInteriorMapNo = NowInteriorMapNo;
        NowInteriorMapNo = map_no;
        return 1;
    }
    return 0;
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_997__4__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_863__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_890__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_891__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_892__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_893__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_894__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_914__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_950__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_1047__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/mapjump", at_1091__2__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(NowMainMapNo, 0x4);
INCLUDE_BSS(NowSubMapNo, 0x4);
INCLUDE_BSS(NowInteriorMapNo, 0x4);
INCLUDE_BSS(OldInteriorMapNo, 0x4);
INCLUDE_BSS(ScriptBuffer, 0x4);
INCLUDE_BSS(InteriorFlag, 0x4);
INCLUDE_BSS(old_bgm_no, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(now_script_file, 0x40);
INCLUDE_BSS(MainMapInfo__2, 0x20);
INCLUDE_BSS(SubMapInfo, 0x20);
INCLUDE_BSS(at_912__4, 0x80);
INCLUDE_BSS(old_mapname, 0x40);
INCLUDE_BSS(OldPos, 0x10);
INCLUDE_BSS(OldRot, 0x10);
INCLUDE_BSS(OldCamPos, 0x10);
INCLUDE_BSS(OldCamRef, 0x10);
INCLUDE_BSS(PrevInterior, 0x40);
INCLUDE_BSS(NowInterior, 0x40);
INCLUDE_BSS(OldBgmStatus, 0x20);
