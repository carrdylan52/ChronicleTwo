#include "common.h"
#include "convviewlp.hpp"
#include "gamepad.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "font.hpp"
#include "gaiji.hpp"
#include "scenesnd.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "mglib.hpp"
#include "snd_mngr.hpp"
#include <libmc.h>

#ifdef NONMATCHING
static CScene *MovieScene;
static SAVE_CONVERT_WORK *SAVEDATA_BUFFER;
static int ConvMode;
static int SlotSelect;
#endif
static SAVE_CONVERT_FILE_INFO *SaveFileInfoTablePtr;
static int SaveFileInfoTableSizeConvert[128];
static int FileListNum;
static int ConvertPhase;
static int ConvertFileNum;
static int ConvertResult;
static int ConvertResultDispTime;
static mgCMemory DataBuffer;
static mgCMemory Stack_ReadBuff;

/**
 * Resets the conversion counters and the first 32 file records.
 */
static void InitSaveFileInfoTablePtr();
/**
 * Converts the save directories on the selected memory card.
 */
static int SaveDataConvertLoop();

// Code (.text)
#ifdef NONMATCHING
void SVConvViewInit(INIT_LOOP_ARG arg) {
    MovieScene = GetMainScene();
    MovieScene->Initialize();
    mgInitFont();
    mgCMemory *main_stack = GetMainStack();
    main_stack->stack_used = 0;
    main_stack->lock = 0;
    static mgCMemory buf0;
    static mgCMemory buf1;
    static mgCMemory dbuf0;
    static mgCMemory dbuf1;
    u_long128 *vif0 = main_stack->stAlloc64(10000);
    u_long128 *vif1 = main_stack->stAlloc64(10000);
    mgInitVif1Packet(vif0, vif1, 10000);
    buf0.stSetBuffer(main_stack->stAlloc64(30000), 30000);
    buf1.stSetBuffer(main_stack->stAlloc64(30000), 30000);
    dbuf0.stSetBuffer(main_stack->stAlloc64(60000), 60000);
    dbuf1.stSetBuffer(main_stack->stAlloc64(60000), 60000);
    DataBuffer.stSetBuffer(main_stack->stAlloc64(100000), 100000);
    mgSetPacketBuffer(&buf0, &buf1);
    mgSetDataBuffer(&dbuf0, &dbuf1, 1);
    mgSetBackGround(0.0f, 0.0f, 0.0f, 128.0f);
    SetTextureTable(100, 20, &DataBuffer);
    mgTexManager.EnterIMGFile(GetGaijiImgPtr(), 0, NULL, NULL);
    ReLoadFontTexture(0);
    mgTexManager.EnterIMGFile(GetFontTex2ImgPtr(), 0, NULL, NULL);
    sceMcInit();
    main_stack->Align64();
    SaveFileInfoTablePtr = new ((u_long128 *)main_stack->Alloc(0x82)) SAVE_CONVERT_FILE_INFO[32];
    main_stack->Align64();
    SAVEDATA_BUFFER = (SAVE_CONVERT_WORK *)main_stack->Alloc(0x659E);
    if (SAVEDATA_BUFFER != NULL) {
        for (int i = 0; i < SAVE_EDIT_DATA_MAX; ++i)
            new ((u_long128 *)&SAVEDATA_BUFFER->save_data.edit_data[i]) CEditData;
        new ((u_long128 *)&SAVEDATA_BUFFER->save_data.user_data) CUserDataManager;
        SAVEDATA_BUFFER->save_data.quest_data.Initialize();
        new ((u_long128 *)&SAVEDATA_BUFFER->save_data.menu_system_data) CMenuSystemData;
    }
    main_stack->Align64();
    Stack_ReadBuff.stSetBuffer(main_stack->stAlloc(0), 0);
    Stack_ReadBuff.stack_used = 0;
    Stack_ReadBuff.lock = 0;
    Stack_ReadBuff.Align64();
    InitSaveFileInfoTablePtr();
    ConvMode = SV_CONV_MODE_SELECT;
    SlotSelect = 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/convviewlp", SVConvViewInit__F13INIT_LOOP_ARG);
#endif
void SVConvViewExit() {
    sceMcEnd();
    GamePad__2.AutoRepeatOff();
    GamePad__2.MenuModeOff();
    sndSeAllStop(-1);
    mgCloseFont();
}
#ifdef NONMATCHING
int SVConvViewLoop() {
    if (ConvMode == SV_CONV_MODE_SELECT &&
        (GamePad__2.Down(PAD_START) || GamePad__2.Down(PAD_TRIANGLE))) {
        return 1;
    }
    switch (ConvMode) {
    case SV_CONV_MODE_SELECT:
        if (GamePad__2.Down(PAD_LEFT)) SlotSelect = 0;
        if (GamePad__2.Down(PAD_RIGHT)) SlotSelect = 1;
        if (GamePad__2.Down(PAD_CROSS)) {
            ConvMode = SV_CONV_MODE_CONVERT;
            InitSaveFileInfoTablePtr();
        }
        break;
    case SV_CONV_MODE_CONVERT:
        ConvertResultDispTime = 30;
        if (SaveDataConvertLoop()) ConvMode = SV_CONV_MODE_RESULT;
        break;
    case SV_CONV_MODE_RESULT:
        if (GamePad__2.Down(PAD_CIRCLE)) {
            ConvertResult = SAVEDATA_CONVERT_RESULT_NONE;
            ConvertResultDispTime = 0;
            ConvMode = SV_CONV_MODE_SELECT;
        }
        break;
    }
    mgTexManager.ReloadTexture(0, (sceVif1Packet *)NULL);
    CFont font;
    font.Init();
    font.SetClearance(16, 20);
    font.SetFuchi(5);
    font.SetColor(0x80686A6B);
    font.DrawDirect("SaveData Convert", 40, 20);
    if (ConvMode == SV_CONV_MODE_SELECT) font.DrawDirect("Exit: Start or (A)", 200, 20);
    char line[256];
    sprintf(line, "Now Slot : %d", SlotSelect);
    font.SetStr(line);
    font.SetPos(40, 42);
    font.DrawDirect(font.str, font.pos_x, font.pos_y);
    int y = 72;
    if (ConvMode == SV_CONV_MODE_SELECT) {
        font.DrawDirect("Slot Select    : Left or Right", 40, 72);
        y = 96;
        font.DrawDirect("Check & Convert: (O)", 40, 96);
    }
    if (ConvMode == SV_CONV_MODE_CONVERT) {
        char *phase_text = "Checking MemoryCard";
        if (ConvertPhase == SAVEDATA_CONVERT_PHASE_READ_DIR) phase_text = "Now Check DataFile";
        if (ConvertPhase == SAVEDATA_CONVERT_PHASE_CONVERT) phase_text = "Now Convert Data ....";
        font.SetStr(phase_text);
        font.SetPos(40, y);
        font.DrawDirect(font.str, font.pos_x, font.pos_y);
        y += 24;
        font.SetStr("Don't remove memory card (PS2).");
        font.SetPos(40, y);
        font.DrawDirect(font.str, font.pos_x, font.pos_y);
    }
    if (ConvMode == SV_CONV_MODE_RESULT) {
        if (ConvertResult == SAVEDATA_CONVERT_RESULT_CARD_ERROR) {
            font.SetStr("Failed Access memory card(PS2)");
            font.SetPos(40, y);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            y += 24;
        } else if (ConvertResult == SAVEDATA_CONVERT_RESULT_NO_FILES) {
            font.SetStr("Not Exist Convert Files ");
            font.SetPos(40, y);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            y += 24;
        } else {
            font.DrawDirect("End Convert", 40, y);
            sprintf(line, "Need Convert Files: %d", FileListNum);
            font.SetStr(line); font.SetPos(40, y + 24);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            sprintf(line, "Converted Files: %d", ConvertFileNum);
            font.SetStr(line); font.SetPos(40, y + 48);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            sprintf(line, "Not Convert Files: %d", FileListNum - ConvertFileNum);
            font.SetStr(line); font.SetPos(40, y + 72);
            font.DrawDirect(font.str, font.pos_x, font.pos_y);
            y += 96;
        }
        font.DrawDirect("Return to SlotSelect : (X)", 40, y);
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/convviewlp", SVConvViewLoop__Fv);
#endif
static void InitSaveFileInfoTablePtr() {
    FileListNum = 0;
    ConvertPhase = SAVEDATA_CONVERT_PHASE_CHECK_CARD;
    ConvertFileNum = 0;
    ConvertResult = SAVEDATA_CONVERT_RESULT_NONE;
    ConvertResultDispTime = 0;
    for (int i = 0; i < 32; i++) {
        SaveFileInfoTablePtr[i].name[0] = '\0';
        SaveFileInfoTableSizeConvert[i] = 0;
    }
}
#ifdef NONMATCHING
static int SaveDataConvertLoop() {
    int command, result;
    if (ConvertPhase == SAVEDATA_CONVERT_PHASE_CHECK_CARD) {
        int card_type, free_size, formatted;
        sceMcSync(0, NULL, NULL);
        sceMcGetInfo(SlotSelect, 1, &card_type, &free_size, &formatted);
        sceMcSync(0, &command, &result);
        if (card_type != sceMcTypePS2 || formatted != 1 || result < -1) {
            ConvertResult = SAVEDATA_CONVERT_RESULT_CARD_ERROR;
            return 1;
        }
        ConvertPhase = SAVEDATA_CONVERT_PHASE_READ_DIR;
        return 0;
    }
    if (ConvertPhase == SAVEDATA_CONVERT_PHASE_READ_DIR) {
        SAVE_CONVERT_FILE_INFO entries[64];
        memset(entries, 0, sizeof(entries));
        char mask[128];
        strcpy(mask, "BASCUS-97213*");
        memset(entries, 0, sizeof(entries));
        sceMcGetDir(SlotSelect, 1, mask, 0, 64, entries);
        sceMcSync(0, &command, &result);
        for (int i = 0; i < result && i < 64; i++) {
            memcpy(&SaveFileInfoTablePtr[FileListNum], &entries[i], sizeof(SAVE_CONVERT_FILE_INFO));
            FileListNum++;
        }
        if (FileListNum <= 0) {
            ConvertResult = SAVEDATA_CONVERT_RESULT_NO_FILES;
            return 1;
        }
        ConvertPhase = SAVEDATA_CONVERT_PHASE_CONVERT;
        return 0;
    }
    if (ConvertPhase != SAVEDATA_CONVERT_PHASE_CONVERT) return 0;

    char dkcl_name[128] = "BESCES-51190dkcl%d";
    char album_name[128] = "BESCES-51190dc2album";
    char omake_name[128] = "BESCES-51190dc2omake";
    for (int file = 0; file < FileListNum; file++) {
        SAVE_CONVERT_FILE_INFO existing[64];
        char old_name[128], new_name[128], previous_dir[128];
        char existing_names[64][128];
        int existing_numbers[64];
        strcpy(old_name, "BESCES-51190*");
        memset(existing, 0, sizeof(existing));
        sceMcGetDir(SlotSelect, 1, old_name, 0, 64, existing);
        int existing_count = 0;
        sceMcSync(0, &command, &existing_count);
        for (int i = 0; i < 64; i++) {
            existing_numbers[i] = -1;
            existing_names[i][0] = '\0';
        }
        for (int i = 0; i < existing_count; i++) {
            strcpy(existing_names[i], existing[i].name);
            int number = atoi(&existing[i].name[16]);
            existing_numbers[i] = existing[i].name[16] ? number : 32;
            printf("file[%d] : %d\n", i, number);
        }
        printf("\n");
        SAVE_CONVERT_FILE_INFO &candidate = SaveFileInfoTablePtr[file];
        SAVEDATA_CONVERT_TYPE type = SAVEDATA_CONVERT_TYPE_NONE;
        if (strncmp(&candidate.name[12], "dkcl", 4) == 0) type = SAVEDATA_CONVERT_TYPE_GAME;
        if (strncmp(&candidate.name[12], "dc2album", 8) == 0) type = SAVEDATA_CONVERT_TYPE_ALBUM;
        if (strncmp(&candidate.name[12], "dc2omake", 8) == 0) type = SAVEDATA_CONVERT_TYPE_OMAKE;
        if (type == SAVEDATA_CONVERT_TYPE_NONE) {
            printf("not convert type : %s\n", candidate.name);
            continue;
        }
        char *target = NULL;
        int number = -1;
        bool duplicate = false;
        if (type == SAVEDATA_CONVERT_TYPE_ALBUM) target = album_name;
        if (type == SAVEDATA_CONVERT_TYPE_OMAKE) target = omake_name;
        if (type == SAVEDATA_CONVERT_TYPE_GAME) {
            number = atoi(&candidate.name[16]);
            if (number >= 0) {
                for (int i = 0; i < existing_count; i++) {
                    if (number == existing_numbers[i]) { duplicate = true; break; }
                }
            }
        } else {
            for (int i = 0; i < existing_count; i++) {
                if (strcmp(existing_names[i], target) == 0) duplicate = true;
            }
        }
        if (duplicate) {
            printf("error already exist ... convert no : %d\n", number);
            continue;
        }
        sceMcChdir(SlotSelect, 1, candidate.name, previous_dir);
        sceMcSync(0, NULL, NULL);
        strcpy(old_name, candidate.name);
        SAVE_CONVERT_FILE_INFO inside[16];
        sceMcGetDir(SlotSelect, 1, old_name, 0, 16, inside);
        sceMcSync(0, &command, &result);
        if (type == SAVEDATA_CONVERT_TYPE_GAME) {
            if (result > 0) {
                sprintf(new_name, dkcl_name, number);
                sceMcRename(SlotSelect, 1, old_name, new_name);
                sceMcSync(0, NULL, NULL);
                sceMcChdir(SlotSelect, 1, "/", NULL);
                sceMcSync(0, NULL, NULL);
                sceMcRename(SlotSelect, 1, old_name, new_name);
                sceMcSync(0, NULL, NULL);
                ConvertFileNum++;
            } else {
                printf("ERROR : not exist file,,,\n");
            }
        } else {
            sceMcRename(SlotSelect, 1, old_name, target);
            sceMcSync(0, NULL, NULL);
            sceMcChdir(SlotSelect, 1, "/", NULL);
            sceMcSync(0, NULL, NULL);
            sceMcRename(SlotSelect, 1, old_name, target);
            sceMcSync(0, NULL, NULL);
            ConvertFileNum++;
        }
        sceMcChdir(SlotSelect, 1, "/", NULL);
        sceMcSync(0, NULL, NULL);
    }
    ConvertPhase = SAVEDATA_CONVERT_PHASE_END;
    ConvertResult = SAVEDATA_CONVERT_RESULT_DONE;
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/convviewlp", SaveDataConvertLoop__Fv);
#endif

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1072__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1073__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1074__5__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1016__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1017__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1018__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1019__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1020__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1021__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1022__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1023__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1024__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1025__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1026__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1027__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1028__10__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1029__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1030__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1031__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1159__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1160__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1161__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1162__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1163__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1164__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1165__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1166__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1167__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1168__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/convviewlp", at_1169__DATA);

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)
INCLUDE_BSS(MovieScene__2, 0x4);
INCLUDE_BSS(ConvMode, 0x4);
INCLUDE_BSS(SlotSelect, 0x4);
INCLUDE_BSS(FileListNum, 0x4);
INCLUDE_BSS(ConvertPhase, 0x4);
INCLUDE_BSS(ConvertFileNum, 0x4);
INCLUDE_BSS(ConvertResult, 0x4);
INCLUDE_BSS(ConvertResultDispTime, 0x34);
INCLUDE_BSS(SaveFileInfoTablePtr, 0x4);
INCLUDE_BSS(SAVEDATA_BUFFER, 0x4);
INCLUDE_BSS(init_817, 0x4);
INCLUDE_BSS(init_820, 0x4);
INCLUDE_BSS(init_823, 0x4);
INCLUDE_BSS(init_826, 0x1);

// Uninitialised data (.bss)
INCLUDE_BSS(DataBuffer__3, 0x30);
INCLUDE_BSS(Stack_ReadBuff__3, 0x30);
INCLUDE_BSS(SaveFileInfoTableSizeConvert, 0x200);
INCLUDE_BSS(buf0_816, 0x30);
INCLUDE_BSS(buf1_819, 0x30);
INCLUDE_BSS(dbuf0_822, 0x30);
INCLUDE_BSS(dbuf1_825, 0x30);
