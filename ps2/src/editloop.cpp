#include "common.h"
#include "editloop.hpp"
#include <cstring>

#include <cstdio>
#include <libvu0.h>

#include "actionchara.hpp"
#include "cameracontrol.hpp"
#include "charasetup.hpp"
#include "dataread.hpp"
#include "editanalyze.hpp"
#include "editctrl.hpp"
#include "editdebug.hpp"
#include "editeff.hpp"
#include "editevent.hpp"
#include "editexception.hpp"
#include "editmap.hpp"
#include "editmode.hpp"
#include "editparts.hpp"
#include "effectlist.hpp"
#include "effscript.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "eventedit.hpp"
#include "gaiji.hpp"
#include "gamedata.hpp"
#include "gamepad.hpp"
#include "helpmes.hpp"
#include "mainloop.hpp"
#include "mapjump.hpp"
#include "mapload.hpp"
#include "mapparts.hpp"
#include "mapselect.hpp"
#include "menumain.hpp"
#include "mg_dataset.hpp"
#include "mg_drawprim.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_visual.hpp"
#include "mglib.hpp"
#include "nd_meswin.hpp"
#include "nowload.hpp"
#include "padcontrol.hpp"
#include "photo.hpp"
#include "savedata.hpp"
#include "scenesnd.hpp"
#include "screeneffect.hpp"
#include "snd_mngr.hpp"
#include "sphida.hpp"
#include "subgame.hpp"
#include "sysmes.hpp"
#include "vlgr_info.hpp"
#include "wavetable.hpp"

#ifdef NONMATCHING
static mgCFrame         *WaterFrame;        /**< Frame of the water overlay. */
static mgCFrame         *RedBicMark;        /**< Red map marker frame. */
static mgCFrame         *BlueBicMark;       /**< Blue map marker frame. */
#endif
static CMapTreasureBox  *TreasureBox;       /**< Model used for the town's treasure chests. */
static int              MapNo;             /**< Main map number of the town. */
#ifdef NONMATCHING
static CCameraControl  *Camera;            /**< Walking camera. */
#endif
// dng_main.hpp declares the same name for its own object.
#ifdef NONMATCHING
namespace editloop {
    static CCameraControl *EventCamera;    /**< Camera used by town events. */
}

static CCameraControl  *FixCamera;         /**< Fixed-map camera. */
static CCameraControl  *EditCamera;        /**< Georama camera. */
static int              ActiveCharaNo;     /**< Current player character type. */
static int              ControlCharaID;    /**< Scene slot of the controlled character. */
#endif
static CCharacter2     *WalkChara;         /**< Character walking through the town. */
static int              LoopCounter;       /**< Frames spent in the town loop. */
static int              LoopMode;          /**< Phase of the town loop, an EditLoopMode. */
static int              ControlMode;       /**< Owner of player control, an EditControlMode. */
static int              SubMapLoadBG;      /**< Non-zero while a sub-map loads in the background. */
static int              now_load_map_no;   /**< Number of the background sub-map load. */
#ifdef NONMATCHING
static int              EventSquareJump;   /**< Event requested when entering a square. */
static int              EditDrawFlag;      /**< Non-zero when town drawing is enabled. */
#endif
static int              EditDrawCancelFlag; /**< Cancels one town draw. */
#ifdef NONMATCHING
static int              PauseFlag;         /**< Non-zero while the town is paused. */
#endif
static int              LockChara;         /**< Number of character control locks held. */
#ifdef NONMATCHING
static int              PreEditMenuCnt;    /**< Frames waiting for the Georama menu animation. */
#endif
static int              EditModeChgFlag;   /**< Non-zero while changing Georama mode. */
static int              EditModeChgCnt;    /**< Frames left in a mode change. */
static int              EditModeChgEvent;  /**< Event started after the mode-change delay. */
static CScene          *MainScene;         /**< Scene of the town loop. */
#ifdef NONMATCHING
static u_long128       *main_pkt1;         /**< First VIF packet buffer. */
static u_long128       *main_pkt2;         /**< Second VIF packet buffer. */
u_long128              *read_buffer_end;
static u_long128       *MenuDataBuf;       /**< Loaded menu configuration. */
static int              MenuDataSize;      /**< Byte size of the menu configuration. */
static int              FixCharaBuffSize;  /**< Quadwords reserved for character data. */
static u_long128       *CrossFadeBuff;     /**< Saved frame used for cross-fades. */
#endif
static int              DelMainNPCflag;    /**< Non-zero when the outdoor villagers were removed. */
#ifdef NONMATCHING
static MENU_INIT_ARG   *MenuInfo = &MenuArg;          /**< Parameters and result of the town menu. */
static int              DataPktMode = -1;  /**< Kind of packet buffers currently installed. */
#endif

static CWaveTable       WaveTable;         /**< Water ripple heights. */
static sceVu0FVECTOR    CharaOldPos;        /**< Player position saved before the menu. */
ClsMes                  EventMes1;
static mgCMemory        buf0;              /**< First drawing packet stack. */
static mgCMemory        buf1;              /**< Second drawing packet stack. */
static mgCMemory        data_buf[2];       /**< Map drawing data stacks. */
static mgCMemory        init_dbuf[2];      /**< Initial drawing data stacks. */
static mgCMemory        WorkBuffer;        /**< Temporary town work memory. */
static mgCMemory        MenuBuffer;        /**< Main menu memory. */
static mgCMemory        ChrEffBuffer;      /**< Character effect memory. */
mgCMemory               ScriptBuffer;
static mgCMemory        TotalDataBuff;     /**< Memory holding persistent town data. */
static mgCMemory        ControlCharaBuff;  /**< Player character memory. */
static mgCMemory        MainDataBuff;      /**< Main map memory. */
static mgCMemory        MainCharaBuff;     /**< Main map character memory. */
static mgCMemory        SubDataBuff;       /**< Sub-map memory. */
static mgCMemory        SubCharaBuff;      /**< Sub-map character memory. */
static mgCMemory        EventBuff[4];      /**< Scene event stacks. */
static mgCMemory        CharaBufs[8];      /**< Character stacks. */
static mgCMemory        FishingBuff;       /**< Fishing sub-game memory. */
static mgCMemory        SkyBuff;           /**< Sky memory. */
static CEditEvent       EditEvent;         /**< Town event controller. */
static EditDebugInfo    EdDebugInfo;       /**< Town debug-menu parameters. */
static mgCVisualMDT     TestVisual;        /**< Debug test model. */
static mgCFrame         TestFrame;         /**< Frame of the debug test model. */
static int              beforeAnalyze[16]; /**< Georama analysis flags before the last edit. */

static void InitSubMapLoadStep();
static void EditModeChgStep(CScene *scene);
static void LoadMap();
static void InitEditEvent();
static void ResetEditEvent();
static void RestartEditEvent();
static void UnLockCharaCtrl();
static void UpdateTrBoxFlag(int map_no);
static void editLoadSound(int map_no);

// Code (.text)
/**
 * Returns the player data in the current save, or NULL when no save is attached.
 */
static CUserDataManager *GetUserData() {
    CSaveData *save;

    save = GetSaveData();
    if (save != NULL) {
        return &save->user_data;
    }
    return NULL;
}

/**
 * Clears the town character control lock.
 */
static void InitLockCharaCtrl() {
    LockChara = 0;
}

/**
 * Adds a lock preventing player control of the town character.
 */
static void LockCharaCtrl() {
    LockChara++;
}

/**
 * Releases one character control lock, stopping at zero.
 */
static void UnLockCharaCtrl() {
    LockChara--;
    if (LockChara < 0) {
        LockChara = 0;
    }
}

#ifdef NONMATCHING
int IsEditMode() {
    if (LoopMode == EDIT_LOOP_EDIT) {
        return 1;
    }
    if (LoopMode == EDIT_LOOP_EDIT_PRE_MENU) {
        return 1;
    }
    return 0;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", IsEditMode__Fv);
#endif

/**
 * Clears the pending transition between walking and Georama editing.
 */
static void InitEditModeChg() {
    EditModeChgFlag = 0;
    EditModeChgCnt = 0;
    EditModeChgEvent = 0;
}

/**
 * Reports whether a Georama mode transition is still counting down.
 */
static int NowEditModeChg() {
    if (EditModeChgFlag != 0) {
        return EditModeChgCnt > 0;
    }
    return 0;
}

#ifdef NONMATCHING
/**
 * Locks character control for thirty frames before starting the transition event.
 */
static void EditModeChg(int event) {
    EditModeChgEvent = event;
    EditModeChgCnt = 30;
    EditModeChgFlag = 1;
    LockCharaCtrl();
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditModeChg__Fi);
#endif

#ifdef NONMATCHING
/**
 * Finishes the transition delay and starts its event when the scene is idle.
 */
static void EditModeChgStep(CScene *scene) {
    if (EditModeChgFlag != 0) {
        EditModeChgCnt--;
        if (EditModeChgCnt <= 0) {
            EditModeChgFlag = 0;
            EditModeChgCnt = 0;
            UnLockCharaCtrl();
            if (scene->event_run == 0 && EditModeChgEvent > 0) {
                scene->RunEvent(EditModeChgEvent, NULL);
            }
        }
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditModeChgStep__FP6CScene);
#endif

#ifdef NONMATCHING
void SetDataPacket(int mode) {
    u_long128 *buffer;
    int        size;

    if (mode == 0) {
        buffer = read_buffer - 5000;
        init_dbuf[0].stSetBuffer(buffer, 5000);
        init_dbuf[1].stSetBuffer(buffer - 5000, 5000);
        mgInitVif1Packet(main_pkt1, main_pkt2, 160000);
        mgSetPacketBuffer(&buf0, &buf1);
        mgSetDataBuffer(&init_dbuf[0], &init_dbuf[1], 1);
        DataPktMode = mode;
        return;
    }
    size = 70000;
    if (mode == 2) {
        size = 115000;
    }
    printf("Data Packet Size = %dkb\n", size * 16 / 1024);
    if (mode == DataPktMode) {
        ControlCharaBuff.stack_used = 0;
        ControlCharaBuff.lock = 0;
        ControlCharaBuff.Alloc(FixCharaBuffSize);
        ControlCharaBuff.Alloc(size);
        ControlCharaBuff.Alloc(size);
        ControlCharaBuff.lock = 1;
        return;
    }
    ControlCharaBuff.stack_used = 0;
    ControlCharaBuff.lock = 0;
    ControlCharaBuff.Alloc(FixCharaBuffSize);
    mgInitVif1Packet(main_pkt1, main_pkt2, 160000);
    mgSetPacketBuffer(&buf0, &buf1);
    data_buf[0].stSetBuffer(ControlCharaBuff.Alloc(size), size);
    data_buf[1].stSetBuffer(ControlCharaBuff.Alloc(size), size);
    ControlCharaBuff.lock = 1;
    mgSetDataBuffer(&data_buf[0], &data_buf[1], 1);
    printf("Data ADR %x,%x\n", data_buf[0].stack + data_buf[0].stack_used, data_buf[1].stack + data_buf[1].stack_used);
    DataPktMode = mode;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", SetDataPacket__Fi);
#endif

/**
 * Saves the town and stops its sub-game, sound, background reads and event.
 */
static void PreExitLoop(CScene *scene) {
    BurnEditParts();
    EditDataSave();
    sgBreakSubGame();
    scene->StopBGM(0);
    scene->InitBGM();
    scene->SeAllStop();
    BreakReadBG();
    sndStopVoice(1);
    ResetNpcTalkMes();
    EdEventTermination();
}

#ifdef NONMATCHING
void EditInit(INIT_LOOP_ARG arg) {
    mgCMemory         *main_stack;
    u_long128         *script_data;
    u_long128         *image_data;
    CEffectScriptMan  *effects;
    CActionChara      *characters;
    CCameraControl   *debug_camera;
    CCharacter2      *player;
    CMap             *map;
    mgCTexture       *cross_texture;
    mgCMDTBuilder     builder;
    mgLoadData        load;
    sceVu0FVECTOR     material = {0.5f, 0.0f, 0.0f, 0.2f};
    sceVu0FVECTOR     position;
    char              system_image[64] = "img/esystem.img";
    char             *menu_file;
    int               data_size;
    int               image_size;
    int               fire_size;
    int               water_size;
    u_int             water_qwords;
    int               event_no;
    int               active_chara_no;
    int               fishing_item;
    int               i;
    static sceVu0FVECTOR initial_position;

    DataPktMode = -1;
    MainScene = GetMainScene();
    DeleteFileCache();
    MapNo = -1;
    mgInitFont();
    PauseFlag = 0;
    LoopMode = EDIT_LOOP_WALK;
    ControlMode = EDIT_CONTROL_PLAYER;
    InitLockCharaCtrl();
    InitEditModeChg();
    LoopCounter = 0;
    EditDrawFlag = 0;
    EditDrawCancelFlag = 0;
    mgInitLighting();
    InitInterior();
    InitSubMapLoadStep();
    EventSquareJump = 0;
    mgActiveLighting(0, 0);
    mgInitActiveLighting();

    main_stack = GetMainStack();
    main_stack->stack_used = 0;
    main_stack->lock = 0;
    main_pkt1 = main_stack->stAlloc64(10000);
    main_pkt2 = main_stack->stAlloc64(10000);
    mgInitVif1Packet(main_pkt1, main_pkt2, 160000);
    buf0.stSetBuffer(main_stack->stAlloc64(35000), 35000);
    buf1.stSetBuffer(main_stack->stAlloc64(35000), 35000);
    mgSetPacketBuffer(&buf0, &buf1);
    script_data = main_stack->stAlloc64(20000);
    if (strlen("SsScript Buffer") < 16) {
        strcpy(ScriptBuffer.name, "SsScript Buffer");
    }
    ScriptBuffer.stSetBuffer(script_data, 20000);
    main_stack->Align64();
    data_size = main_stack->stack_size - main_stack->stack_used - 210128;
    TotalDataBuff.stSetBuffer(&main_stack->stack[main_stack->stack_used], data_size);
    printf("data memory size = %d kbyte", data_size * 16 / 1024);
    if (strlen("Total Data Buffer") < 16) {
        strcpy(TotalDataBuff.name, "Total Data Buffer");
    }
    TotalDataBuff.stack_used = 0;
    TotalDataBuff.lock = 0;
    main_stack->Alloc(data_size);
    read_buffer = main_stack->stAlloc64(200000);
    MenuBuffer.stSetBuffer(read_buffer, 200000);
    read_buffer_end = read_buffer + 200000;
    WorkBuffer.stSetBuffer(main_stack->stAlloc64(10000), 10000);
    mgTexManager.SetTableBuffer(350, 221, &TotalDataBuff);
    mgTexManager.Initialize(GetVramTopAddress(), -1);
    SetDataPacket(0);

    NowLoadingInfo loading;
    loading.tex_block = 206;
    loading.step_count = 15;
    loading.unk_4 = 1;
    loading.memory.stSetBuffer(read_buffer + 195000, 5000);
    CreateNowLoading(&loading);
    SetEnvUserDataMan(0);
    TotalDataBuff.Align64();
    MenuDataBuf = &TotalDataBuff.stack[TotalDataBuff.stack_used];
    menu_file = GetMenuCfgFileName(0, 0);
    SetCurrentDir(NULL);
    if (LoadFile2(menu_file, MenuDataBuf, &MenuDataSize, 0) != 0) {
        TotalDataBuff.Alloc(MenuDataSize / 16 + 1);
    }
    NowLoadingBarStep();

    builder.Begin(&TotalDataBuff);
    builder.BeginData(MG_MDT_DATA_VERTEX);
    builder.SetData(0.0f, 0.0f, 0.0f, 1.0f);
    builder.SetData(0.0f, 0.0f, 1.0f, 1.0f);
    builder.SetData(0.0f, 1.0f, 0.0f, 1.0f);
    builder.SetData(0.0f, 1.0f, 1.0f, 1.0f);
    builder.SetData(1.0f, 0.0f, 0.0f, 1.0f);
    builder.SetData(1.0f, 0.0f, 1.0f, 1.0f);
    builder.SetData(1.0f, 1.0f, 0.0f, 1.0f);
    builder.SetData(1.0f, 1.0f, 1.0f, 1.0f);
    builder.EndData();
    builder.BeginData(MG_MDT_DATA_MATERIAL);
    builder.SetMaterial(material, "");
    builder.EndData();
    builder.BeginFaces();
    builder.BeginPrim(0x214, 0);
    builder.AddFace(0);
    builder.AddFace(1);
    builder.AddFace(2);
    builder.AddFace(3);
    builder.AddFace(6);
    builder.AddFace(7);
    builder.AddFace(4);
    builder.AddFace(5);
    builder.EndPrim();
    builder.BeginPrim(0x214, 0);
    builder.AddFace(2);
    builder.AddFace(6);
    builder.AddFace(0);
    builder.AddFace(4);
    builder.AddFace(1);
    builder.AddFace(5);
    builder.AddFace(3);
    builder.AddFace(7);
    builder.EndPrim();
    builder.EndFaces();
    TestFrame.mgCFrame::Initialize();
    memset(&load, 0, sizeof(load));
    load.memory = &TotalDataBuff;
    load.work_memory = &WorkBuffer;
    builder.End(&TestFrame, &TestVisual, &load);
    if (TestFrame.attr != NULL) {
        TestFrame.attr->clip_enable = 1;
        TestFrame.attr->z_write = -1;
    }

    if (LoadFile2("etc/bikkuri_aka.mds", read_buffer, NULL, 0) != 0) {
        RedBicMark = mgLoadMDSFile((MDS_HEADER *)read_buffer, &TotalDataBuff, NULL, NULL);
        mgCFrameAttr attr;
        attr.billboard = MG_FRAME_BILLBOARD_Y;
        attr.no_light = 1;
        attr.color[3] = 128.0f;
        attr.color[2] = 255.0f;
        attr.color[1] = 255.0f;
        attr.color[0] = 255.0f;
        RedBicMark->SetAttrParam(attr, 1, 0);
    }
    LoadEditCursor(&TotalDataBuff, 163);
    EditSetEffectBuffer(&TotalDataBuff);
    TreasureBox = NULL;
    if (LoadFile2("map/itembox.chr", read_buffer, NULL, 0) != 0) {
        TreasureBox = new (TotalDataBuff.Alloc(sizeof(CMapTreasureBox) / 16 + 2)) CMapTreasureBox;
        mgTexManager.DeleteBlock(173);
        TreasureBox->LoadPackNoLine((u_int *)read_buffer, "info.cfg", &TotalDataBuff, &TotalDataBuff, &TotalDataBuff, 173, NULL);
    }
    ChrEffBuffer.SetHeapMem(TotalDataBuff.stAlloc64(6400), 6400);
    NowLoadingBarStep();
    NowLoadingBarStep();
    SetCurrentDir(NULL);

    TotalDataBuff.Align64();
    image_data = TotalDataBuff.stAllocTest(1);
    if (LanguageCode > 0) {
        sprintf(system_image, "img/esystem%d.img", LanguageCode);
    }
    if (LoadFile2(system_image, image_data, &image_size, 0) != 0) {
        mgTexManager.EnterIMGFile((u_char *)image_data, 162, &TotalDataBuff, NULL);
        TotalDataBuff.Alloc(image_size / 16 + 1);
        LoadTakePhoto(162, &TotalDataBuff, read_buffer);
    }
    TotalDataBuff.Align64();
    image_data = TotalDataBuff.stAllocTest(1);
    if (LoadFile2("effect/fire.img", image_data, &fire_size, 0) != 0) {
        TotalDataBuff.Alloc(fire_size / 16 + 1);
        mgTexManager.EnterIMGFile((u_char *)image_data, 66, &TotalDataBuff, NULL);
    }

    EventMes1.Init();
    EventMes1.Preset(0);
    EventMes1.texture_block = 154;
    ReLoadFontTexture(154);
    mgTexManager.EnterIMGFile(GetGaijiImgPtr(), 154, NULL, NULL);
    mgTexManager.EnterIMGFile(GetFontTex2ImgPtr(), 154, NULL, NULL);
    NowLoadingBarStep();
    MainScene->Initialize();
    MainScene->chara_texb = 70;
    MainScene->villager_texb = 78;
    MainScene->villager_texb_num = 56;
    MainScene->event_texb = 160;
    MainScene->event_texb_num = 2;
    MainScene->GetActiveBgmInfo()->unk_c = 1.0f;
    MainScene->SetVolfBGM(MainScene->GetActiveBgmInfo()->volf);
    MainScene->unk_3e68 = 185;
    MainScene->unk_3e6c = 21;
    effects = new (TotalDataBuff.Alloc(sizeof(CEffectScriptMan) / 16 + 2)) CEffectScriptMan;
    effects->Initialize(&TotalDataBuff, 174, 11);
    effects->load_buffer = read_buffer;
    effects->SetWorkBuffer(&ChrEffBuffer);
    effects->LoadBaseEffSpt("\x91\xab\x8d\xbb\x89\x8c", NULL, -1);
    effects->LoadBaseEffSpt("\x91\xab\x94\x67\x96\xe4", NULL, -1);
    effects->LoadBaseEffSpt("\x91\xab\x90\x85\x83\x70\x83\x56\x83\x83", NULL, -1);
    effects->LoadBaseEffSpt("\x91\xab\x8e\xc5\x90\xb6", NULL, -1);
    effects->LoadBaseEffSpt("\x8d\xbb\x89\x8c" "2", NULL, -1);
    MainScene->AssignEffect(0, effects, NULL);
    MainScene->read_buff = read_buffer;
    if (LoadFile2("img/water_ref.img", TotalDataBuff.stAllocTest(1), &water_size, 0) != 0) {
        water_qwords = (u_int)water_size / 16;
        if (water_size & 0xF) {
            water_qwords++;
        }
        mgTexManager.EnterIMGFile((u_char *)TotalDataBuff.Alloc(water_qwords), 158, NULL, NULL);
    }
    NowLoadingBarStep();

    characters = new (TotalDataBuff.Alloc(sizeof(CActionChara) * 8 / 16 + 2)) CActionChara[8];
    for (i = 0; i < 8; i++) {
        MainScene->AssignChara(i, &characters[i], NULL);
    }
    MainScene->AssignMessage(0, &EventMes1, NULL);
    GetSystemMessage()->texture_block = 154;
    MainScene->AssignMessage(1, GetSystemMessage(), NULL);
    GetSystemMessage(1)->texture_block = 154;
    MainScene->AssignMessage(2, GetSystemMessage(1), NULL);
    GetSystemMessage(2)->texture_block = 154;
    MainScene->AssignMessage(3, GetSystemMessage(2), NULL);
    Camera = new (TotalDataBuff.Alloc(sizeof(CCameraControl) / 16 + 2)) CCameraControl;
    editloop::EventCamera = new (TotalDataBuff.Alloc(sizeof(CCameraControl) / 16 + 2)) CCameraControl;
    FixCamera = new (TotalDataBuff.Alloc(sizeof(CCameraControl) / 16 + 2)) CCameraControl;
    EditCamera = new (TotalDataBuff.Alloc(sizeof(CCameraControl) / 16 + 2)) CCameraControl;
    debug_camera = new (TotalDataBuff.Alloc(sizeof(CCameraControl) / 16 + 2)) CCameraControl;
    MainScene->AssignCamera(0, Camera, NULL);
    MainScene->AssignCamera(1, editloop::EventCamera, NULL);
    MainScene->AssignCamera(2, FixCamera, NULL);
    MainScene->AssignCamera(3, EditCamera, NULL);
    MainScene->AssignCamera(7, debug_camera, NULL);
    MainScene->active_camera = 0;
    MainScene->before_camera = 0;
    Camera->GetActiveParam()->near_height = 10.0f;
    Camera->GetActiveParam()->far_height = 10.0f;
    Camera->GetActiveParam()->ground_space = 30.0f;
    Camera->default_param = *Camera->GetActiveParam();
    MainScene->player_chara = 0;

    mgTexManager.EnterTexture(156, "work", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    mgTexManager.EnterTexture(159, "shadow_work", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    mgTexManager.EnterTexture(158, "water_work", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    mgTexManager.EnterTexture(66, "fire_work", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    mgTexManager.EnterTexture(213, "test4", NULL, 64, 64, 16, NULL, 0, 0);
    mgTexManager.EnterTexture(157, "f_work", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    mgTexManager.EnterTexture(157, "f_work2", NULL, mgScreenWidth / 3, mgScreenHeight / 3, 32, NULL, 0, 0);
    cross_texture = mgTexManager.EnterTexture(213, "cross_f", NULL, mgScreenWidth, mgScreenHeight, 32, NULL, 0, 0);
    InitPause(207);
    CrossFadeBuff = read_buffer + 131072;
    MainScene->fade.SetCrossTexture(cross_texture, CrossFadeBuff);
    MainScene->unk_2e84 = 159;

    TotalDataBuff.Align64();
    TotalDataBuff.lock = 1;
    ControlCharaBuff.stSetBuffer(&TotalDataBuff.stack[TotalDataBuff.stack_used], TotalDataBuff.stack_size - TotalDataBuff.stack_used);
    ControlCharaBuff.stack_used = 0;
    ControlCharaBuff.lock = 0;
    MainScene->SetStack(0, &ControlCharaBuff);
    MainScene->SetStack(1, &MainDataBuff);
    MainScene->SetStack(2, &MainCharaBuff);
    MainScene->SetStack(3, &SubDataBuff);
    MainScene->SetStack(4, &SubCharaBuff);
    MainScene->SetStack(5, &EventBuff[0]);
    MainScene->SetStack(6, &EventBuff[1]);
    MainScene->SetStack(7, &EventBuff[2]);
    MainScene->SetStack(8, &EventBuff[3]);
    MainScene->work_stack = &WorkBuffer;
    ControlCharaBuff.stAlloc64(GetCharaMemAllocSize());
    ControlCharaBuff.Align64();
    FixCharaBuffSize = ControlCharaBuff.stack_used;
    ControlCharaBuff.lock = 1;
    active_chara_no = GetUserData()->active_chr_no;
    SetupMainUnit(read_buffer, &ControlCharaBuff, CharaBufs, 70, MainScene, GetUserData(), active_chara_no, 1);
    ActiveCharaNo = GetUserData()->active_chr_no;
    ControlCharaID = 0;
    MainScene->SetActive(1, 0);
    MainScene->player_chara = ControlCharaID;
    NowLoadingBarStep();

    MapJumpMapInfo main_map;
    main_map.stack_no = 1;
    main_map.map_no = 0;
    main_map.efp_tex_block = 64;
    main_map.tex_block = 0;
    main_map.sky_tex_block = 169;
    main_map.load_buf = (u_char *)read_buffer;
    SetMainMapInfo(&main_map);
    MapJumpMapInfo sub_map;
    sub_map.map_no = 1;
    sub_map.tex_block = 24;
    sub_map.stack_no = 3;
    sub_map.efp_tex_block = 65;
    sub_map.sky_tex_block = 169;
    sub_map.load_buf = (u_char *)read_buffer;
    SetSubMapInfo(&sub_map);
    SetScriptBuffer(&ScriptBuffer);
    MainScene->InitSeBas();
    if (arg.map_no < 0) {
        MainScene->LoadSound(0, read_buffer);
    }
    EditMapJump(SearchMapNo(GetMapName(arg.map_no, NULL)));
    NowLoadingBarStep();
    WaterFrame = NULL;
    InitEditFlag();
    SetCurrentDir(NULL);
    if (MapNo == 10) {
        LoadMap();
    }
    MainCharaBuff.Align64();
    MainCharaBuff.lock = 1;
    SubDataBuff.stSetBuffer(&MainCharaBuff.stack[MainCharaBuff.stack_used], MainCharaBuff.stack_size - MainCharaBuff.stack_used);
    SubDataBuff.stack_used = 0;
    SubDataBuff.lock = 0;
    SetCurrentDir(NULL);
    *(u_long128 *)position = *(u_long128 *)initial_position;
    player = MainScene->GetCharacter(MainScene->player_chara);
    map = MainScene->GetMap(0);
    if (player != NULL && map != NULL) {
        player->SetPosition(map->chara_pos);
        player->GetPosition(position);
    }
    Camera->SetPos(0.0f, 0.0f, 100.0f);
    Camera->Step(10);
    Camera->SetFollowOffset(0.0f, 30.0f, 0.0f);
    Camera->SetFollow(position[0], position[1], position[2]);
    Camera->SetDistance(130.0f);
    Camera->SetHeight(20.0f);
    Camera->SetSpeed(8.0f / (float)mgFrameRate, -1.0f);
    Camera->Step(-1);
    printf("Basic Data %dkbyte\n", TotalDataBuff.stack_used * 16 / 1024);
    printf("Main Data %dkbyte\n", MainDataBuff.stack_used * 16 / 1024);
    printf("Main Chara %dkbyte\n", MainCharaBuff.stack_used * 16 / 1024);
    printf("Sub Data %dkbyte\n", SubDataBuff.stack_used * 16 / 1024);
    printf("Remain %dkbyte\n", (SubDataBuff.stack_size - SubDataBuff.stack_used) * 16 / 1024);
    CreateHelpMes(154);
    InitEvent(MainScene);
    InitEventEdit(214, &MenuBuffer);
    EdEventLoopInit();
    EditControlInit(MainScene);
    MainScene->fade.FadeIn(30);
    MainScene->time_step = 1;
    event_no = arg.event_no;
    if (event_no <= 0) {
        event_no = 100;
    }
    if (event_no > 0 && RunEvent(event_no, MainScene) > 0) {
        ControlMode = EDIT_CONTROL_EVENT;
    }
    MenuInfo->stack = &MenuBuffer;
    MenuInfo->tex_block_top = 134;
    MenuInfo->tex_block_num = 16;
    MenuInfo->mes_tex_block = 154;
    MenuInfo->active_chara_no = ActiveCharaNo;
    MenuInfo->user_data = GetUserData();
    MenuInfo->chara_stack = &ControlCharaBuff;
    MenuInfo->base_chara_stack = CharaBufs;
    MenuInfo->chara_tex_block = 70;
    MenuInfo->pack = (u_int *)MenuDataBuf;
    MenuInfo->pack_size = MenuDataSize;
    EditDebugInit();
    InitLightingEdit();
    EditEvent.Reset();
    InitSubGame(MainScene);
    EdDebugInfo.rod_no = fishing_item;
    EdDebugInfo.menu_buff = NULL;
    EdDebugInfo.esa_no = fishing_item;
    EdDebugInfo.texb = 185;
    EdDebugInfo.texb_num = 21;
    EdDebugInfo.load_buff = NULL;
    EdDebugInfo.jump_map_no = -1;
    EdDebugInfo.unk_c = 154;
    EdDebugInfo.dungeon = 0;
    EdDebugInfo.no_map_event = 0;
    EdDebugInfo.record_check = 0;
    EdDebugInfo.keep_bgm = 0;
    EdDebugInfo.scene = MainScene;
    InitPauseMenu(154);
    NowLoadingBarSteEnd();
    DeleteNowLoading();
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditInit__F13INIT_LOOP_ARG);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", __as__15CameraCtrlParamFRC15CameraCtrlParam);

#ifdef NONMATCHING
// Defined inline in actionchara.hpp.
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", __ct__12CActionCharaFv);
#endif

void EditExit() {
    sndSeAllStop(1);
    MainScene->InitSeSrc();
    mgCloseFont();
    BreakReadBG();
    sndStopVoice(1);
    if (SubGameRunning() != 0) {
        sgExitSubGame();
    }
}

/**
 * Clears the pending background sub-map load.
 */
static void InitSubMapLoadStep() {
    SubMapLoadBG = 0;
    now_load_map_no = -1;
}

/**
 * Finishes a background sub-map load and attaches its villagers and events.
 */
static int SubMapLoadStep() {
    CScene *scene;
    char *name;

    if (MainScene->LoadMapBGStep(NULL) != 0) {
        if (SubMapLoadBG != 0) {
            SubMapLoadBG = 0;
            MainScene->SetActive(2, 1);
            MainScene->LoadSubVillager(GetSubMapNo(), 94);
            MainScene->PreLoadVillagerEnd();
            name = GetMapName(now_load_map_no, NULL);
            scene = MainScene;
            EditMapInitEvent(now_load_map_no, (CEditMap *)scene->GetMap(scene->GetMapID(name)));
            now_load_map_no = -1;
        }
        return 0;
    }
    return 1;
}

#ifdef NONMATCHING
int EditLoop() {
    static int          time_step;
    static int          show_time_step;
    static int          old_cm;
    static int          rain_flag;
    CMap               *map;
    CCharacter2        *chara;
    CCameraControl     *camera;
    CCameraControl     *walk_camera;
    CameraCtrlParam    *camera_param;
    CameraCtrlParam    *default_param;
    SubGameInfo        *current_subgame;
    PAUSE_INFO          menu_pause;
    PAUSE_INFO          pause;
    sceVu0FVECTOR       position;
    sceVu0FVECTOR       ground_position;
    sceVu0FVECTOR       closest;
    sceVu0FVECTOR       line_start;
    sceVu0FVECTOR       line_end;
    sceVu0FVECTOR       load_position = {1400.0f, -6.0f, -218.0f, 1.0f};
    sceVu0FVECTOR       talk_position;
    sceVu0FVECTOR       talk_height;
    sceVu0FVECTOR       villager_position;
    sceVu0FVECTOR       edit_position;
    sceVu0FVECTOR       return_position;
    sceVu0FVECTOR       camera_position;
    sceVu0FVECTOR       camera_reference;
    int                 stay[32];
    char               *map_name;
    float               time_rate;
    float               projection;
    float               distance;
    float               camera_angle;
    int                 game_progress;
    int                 light_band;
    int                 next_sub_map;
    int                 finish;
    int                 wait_for_map;
    int                 menu_mode;
    int                 open_menu;
    int                 change_mode;
    int                 return_to_player;
    int                 start_event;
    int                 pause_enabled;
    int                 event_result;
    int                 reset_event;
    int                 menu_requested;
    int                 menu_button;
    int                 quick_change;
    int                 next_chara;
    int                 menu_enabled;
    int                 debug_closed;
    int                 debug_move;
    int                 edit_enabled;
    int                 main_map_no;
    int                 change_event;
    int                 pause_result;

    finish = 0;
    mgSetAllScissorFlag(0);
    LoopCounter++;
    if (LoopCounter > 10000) {
        LoopCounter = 10000;
    }
    {
        static char init;

        if (init == 0) {
            time_step = 1;
            init = 1;
        }
    }
    {
        static char init;

        if (init == 0) {
            show_time_step = 0;
            init = 1;
        }
    }

    if (PauseFlag == 0 && IsLightingEditMode() == 0) {
        map = MainScene->GetMap(MainScene->active_map);
        if (map != NULL) {
            map->now_time = MainScene->time;
            light_band = map->GetNowTimeLightBand();
            if (GamePad.On2(PAD_RIGHT) != 0) {
                MainScene->AddTime(0.1f);
            }
            if (GamePad.On2(PAD_LEFT) != 0) {
                MainScene->AddTime(-0.1f);
            }
            if (GamePad.Down2(PAD_UP) != 0) {
                MainScene->SetTime(((int)MainScene->time / 2) * 2 + 2);
            }
            if (GamePad.Down2(PAD_DOWN) != 0) {
                show_time_step = 30;
                time_step = time_step == 0;
            }
            if (LoopMode == EDIT_LOOP_WAIT_READ) {
                if (ReadBGSync() == 0) {
                    LoopMode = EDIT_LOOP_WALK;
                }
            } else if (GetSaveData()->time_stop == 0 && time_step != 0 &&
                       ControlMode == EDIT_CONTROL_PLAYER && LoopMode == EDIT_LOOP_WALK) {
                time_rate = 1.0f;
                if (GetSaveData() != NULL && GetSaveData()->config.fast_time != 0) {
                    time_rate = 1.5f;
                }
                MainScene->TimeStep(time_rate);
            }
            map->now_time = MainScene->time;
            if (light_band != map->GetNowTimeLightBand() && map->time_cfade != 0) {
                MainScene->fade.CaptureScreen();
                MainScene->fade.CrossFade(10, 0.8f);
            }
        }
    }

    wait_for_map = 0;
    if (PadCtrl.Btn(0) != 0 || PadCtrl.Btn(5) != 0) {
        wait_for_map = 1;
    }
    map_name = MainScene->GetMapName(MainScene->active_map);
    if (LoopCounter >= 3 && LoopMode == EDIT_LOOP_WALK && map_name != NULL && strcmp(map_name, "m01") == 0) {
        chara = MainScene->GetCharacter(MainScene->player_chara);
        if (chara != NULL) {
            chara->GetPosition(position);
            *(u_long128 *)ground_position = *(u_long128 *)position;
            ground_position[1] = 0.0f;
            line_start[3] = 1.0f;
            line_end[3] = 1.0f;
            next_sub_map = -1;
            if (MainScene->LoadMapBGStep(NULL) == 0) {
                line_start[0] = 1900.0f;
                line_start[1] = 0.0f;
                line_start[2] = 1200.0f;
                line_end[0] = 1900.0f;
                line_end[1] = 0.0f;
                line_end[2] = 1700.0f;
                if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f) {
                    wait_for_map = 1;
                }
                line_start[0] = 1713.0f;
                line_start[1] = 0.0f;
                line_start[2] = -302.0f;
                line_end[0] = 1423.0f;
                line_end[1] = 0.0f;
                line_end[2] = -487.0f;
                if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f) {
                    wait_for_map = 1;
                }
                line_start[0] = -1380.0f;
                line_start[1] = 0.0f;
                line_start[2] = 151.0f;
                line_end[0] = -1380.0f;
                line_end[1] = 0.0f;
                line_end[2] = 444.0f;
                if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f) {
                    wait_for_map = 1;
                }
                line_start[0] = -3283.0f;
                line_start[1] = 0.0f;
                line_start[2] = 1362.0f;
                line_end[0] = -2996.0f;
                line_end[1] = 0.0f;
                line_end[2] = 1616.0f;
                if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f) {
                    wait_for_map = 1;
                }
                line_start[0] = -2218.0f;
                line_start[1] = 0.0f;
                line_start[2] = 628.0f;
                line_end[0] = -2041.0f;
                line_end[1] = 0.0f;
                line_end[2] = 660.0f;
                if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f) {
                    wait_for_map = 1;
                }
            }
            line_start[0] = 1340.0f;
            line_start[1] = 0.0f;
            line_start[2] = 79.0f;
            line_end[0] = 1125.0f;
            line_end[1] = 0.0f;
            line_end[2] = -245.0f;
            if (now_load_map_no != 11 &&
                mgDistLinePoint(ground_position, line_start, line_end, closest) < 20.0f && MainScene->GetMapID("m02") != 1) {
                next_sub_map = 11;
            }
            line_start[0] = 1500.0f;
            line_start[1] = 0.0f;
            line_start[2] = 1600.0f;
            line_end[0] = 1500.0f;
            line_end[1] = 0.0f;
            line_end[2] = 1100.0f;
            if (mgDistLinePoint(ground_position, line_start, line_end, closest) < 40.0f) {
                if (GetSquareEvent() != 0) {
                    if (ground_position[0] >= 1500.0f && CharaOldPos[0] < 1500.0f) {
                        MainScene->RunEvent(201, NULL);
                        if (SubGameRunning() != 0 && GetSubGameNo() == SUBGAME_FISHING) {
                            sgExitSubGame();
                        }
                    }
                } else if (now_load_map_no != 12 && MainScene->GetMapID("m03") != 1) {
                    next_sub_map = 12;
                }
            }
            game_progress = GetSaveData()->game_progress;
            if (game_progress != 2 && game_progress != 3) {
                line_start[0] = -520.0f;
                line_start[1] = 0.0f;
                line_start[2] = 243.0f;
                line_end[0] = -608.0f;
                line_end[1] = 0.0f;
                line_end[2] = -187.0f;
                if (now_load_map_no != 13 &&
                    mgDistLinePoint(ground_position, line_start, line_end, closest) < 50.0f && MainScene->GetMapID("m04") != 1) {
                    next_sub_map = 13;
                }
            }
            load_position[0] = -2259.0f;
            load_position[1] = 185.0f;
            load_position[2] = 851.0f;
            if (now_load_map_no != 13 && mgDistVectorXZ(position, load_position) < 150.0f && MainScene->GetMapID("m04") != 1) {
                next_sub_map = 13;
            }
            load_position[0] = -2700.0f;
            load_position[1] = 256.0f;
            load_position[2] = 1207.0f;
            if (now_load_map_no != 14 && mgDistVectorXZ(position, load_position) < 150.0f && MainScene->GetMapID("m05") != 1) {
                next_sub_map = 14;
            }
            line_start[0] = -2157.0f;
            line_start[1] = 0.0f;
            line_start[2] = 756.0f;
            line_end[0] = -3037.0f;
            line_end[1] = 0.0f;
            line_end[2] = 1531.0f;
            distance = mgDistLinePoint(ground_position, line_start, line_end, closest);
            camera = (CCameraControl *)MainScene->GetCamera(MainScene->active_camera);
            camera_param = camera->GetActiveParam();
            default_param = &camera->default_param;
            if (distance < 220.0f) {
                CancelEyeViewMode();
                camera_param->min_height += (100.0f - camera_param->min_height) / 8.0f;
                camera_param->max_height = camera_param->min_height;
            } else {
                camera_param->min_height += (default_param->min_height - camera_param->min_height) / 8.0f;
                camera_param->max_height += (default_param->max_height - camera_param->max_height) / 8.0f;
            }
            *(u_long128 *)CharaOldPos = *(u_long128 *)position;
            if (next_sub_map > 0) {
                now_load_map_no = next_sub_map;
                LoadSubMap(MainScene, next_sub_map, 1);
                MainScene->PreLoadVillager(next_sub_map, read_buffer_end);
                SubMapLoadBG = 1;
            }
        }
    }
    if (SubMapLoadStep() != 0) {
        while (wait_for_map != 0 && SubMapLoadStep() != 0) {
        }
    }

    if (LoopMode == EDIT_LOOP_WALK_MENU || LoopMode == EDIT_LOOP_EDIT_MENU) {
        menu_pause.scene = MainScene;
        menu_pause.event_skip = 0;
        if (PadCtrl.Btn(21) != 0) {
            PauseStart(&menu_pause);
        }
        if (MenuMainLoop() != 0) {
            MenuMainExit();
            if (LoopMode == EDIT_LOOP_WALK_MENU) {
                LoopMode = EDIT_LOOP_WALK;
                if (MenuInfo->end_code != 11 && SubGameRunning() != 0 && GetSubGameNo() == SUBGAME_FISHING) {
                    sgExitSubGame();
                }
                switch (MenuInfo->end_code) {
                    case 1:
                    case 21:
                        if (MenuInfo->end_code == 21) {
                            MainScene->fade.CaptureScreen();
                            MainScene->fade.CrossFade(20, 1.0f);
                        }
                        ActiveCharaNo = MenuInfo->result[0];
                        ActiveCharaNo = GetUserData()->active_chr_no;
                        chara = MainScene->GetCharacter(MainScene->player_chara);
                        if (chara != NULL) {
                            chara->UpdatePosition();
                            chara->ResetDAPosition();
                        }
                        EditControlStatusInit(MainScene);
                        break;
                    case 11: {
                        SubGameInfo fishing;

                        fishing.scene = MainScene;
                        fishing.rod_no = MenuInfo->result[0];
                        fishing.esa_no = MenuInfo->result[1];
                        FishingBuff.stSetBuffer(CharaBufs[0].stack + CharaBufs[0].stack_used,
                                                CharaBufs[0].stack_size - CharaBufs[0].stack_used);
                        fishing.menu_buff = &MenuBuffer;
                        fishing.load_buff = &FishingBuff;
                        MenuInfo->end_code = 0;
                        current_subgame = GetNowSubGameInfo();
                        if (SubGameRunning() != 0 && current_subgame->rod_no != fishing.rod_no) {
                            fishing.keep_bgm = 1;
                        }
                        if ((GetMenuEtcFlag() & 0x1) != 0 || SubGameRunning() == 0 || current_subgame->rod_no != fishing.rod_no) {
                            ResetViewMode(MainScene);
                            sgInitSubGame(SUBGAME_FISHING, &fishing);
                        } else {
                            sgRestartSubGame(&fishing);
                        }
                        break;
                    }
                    case 6:
                        SetEventScript(NULL, NULL, NULL);
                        if (ControlMode == EDIT_CONTROL_EVENT) {
                            MainScene->active_camera = MainScene->before_camera;
                            ControlMode = EDIT_CONTROL_PLAYER;
                        }
                        if (MenuInfo->result[0] != LOOP_EDIT) {
                            finish = 1;
                            INIT_LOOP_ARG next_loop;
                            memset(&next_loop, 0, sizeof(next_loop));
                            next_loop.map_no = MenuInfo->result[1];
                            next_loop.floor_no = MenuInfo->result[2];
                            next_loop.event_no = 1010;
                            NextLoop(MenuInfo->result[0], next_loop);
                        } else {
                            BurnEditParts();
                            EditMapJump(MenuInfo->result[1]);
                            MainScene->RunEvent(100, NULL);
                        }
                        break;
                }
            } else if (LoopMode == EDIT_LOOP_EDIT_MENU) {
                MainScene->GetMap(MainScene->active_map);
                MainScene->GetCharacter(MainScene->player_chara);
                LoopMode = EDIT_LOOP_EDIT;
                MainScene->before_camera = MainScene->active_camera;
                MainScene->active_camera = 3;
                StartEditModeFromMenu(MainScene, MenuInfo->end_code, MenuInfo->result);
            }
        }
        FadeOutForE3();
        MainScene->fade.FadeStep();
        MainScene->fade.Draw();
        if (finish != 0) {
            PreExitLoop(MainScene);
            return 1;
        }
        return TimeLimitCheck() != 0;
    }

    if (DebugFlag != 0 && EdDebugInfo.jump_map_no >= 0) {
        BurnEditParts();
        EditMapJump(EdDebugInfo.jump_map_no);
        MainScene->RunEvent(100, NULL);
        EdDebugInfo.jump_map_no = -1;
        return 0;
    }
    mgPlightEnable(0);
    MainScene->UpDateMapInfo();
    projection = mgGetProjection() + PhotoAddProjection();
    if (strcmp(MainScene->GetMapName(MainScene->active_map), "s32") == 0 ||
        strcmp(MainScene->GetMapName(MainScene->active_map), "s55") == 0) {
        projection = 250.0f;
    }
    if (projection < 300.0f) {
        projection = 300.0f;
    }
    if (projection > 1000.0f) {
        projection = 1000.0f;
    }
    mgSetRenderInfo(projection, 3.0f, 30000.0f);
    S51Thunder(MainScene);
    menu_mode = LoopMode;
    open_menu = 0;
    change_mode = 0;
    return_to_player = 0;
    start_event = -1;
    pause.scene = MainScene;
    pause.event_skip = 0;
    pause_enabled = 0;
    if (PauseFlag == 0) {
        switch (ControlMode) {
            case EDIT_CONTROL_PLAYER:
                pause_enabled = 1;
                pause.event_skip = 0;
                if (SubGameRunning() != 0) {
                    sgLoopSubGame();
                } else {
                    if (LockChara == 0 && IsEditMode() != 0) {
                        EditMode(MainScene);
                        GamePad.SetAutoRepeat2(PAD_L2 | PAD_R2, 13, 2);
                        GamePad.SetAutoRepeat2(PAD_UP | PAD_RIGHT | PAD_DOWN | PAD_LEFT, 13, 2);
                    }
                    event_result = EditEvent.Step(MainScene);
                    CSceneEventData event_data;
                    memset(&event_data, 0, sizeof(event_data));
                    reset_event = 0;
                    switch (event_result) {
                        case EDIT_EVENT_RESULT_END:
                            reset_event = 1;
                            EditDataSave();
                            break;
                        case EDIT_EVENT_RESULT_ENTER:
                        case EDIT_EVENT_RESULT_ENTER_HOUSE:
                            reset_event = 1;
                            event_data = EditEvent.data;
                            EditGotoInterior(SearchMapNo(EditEvent.map_name), event_result == EDIT_EVENT_RESULT_ENTER_HOUSE);
                            MainScene->RunEvent(100, &event_data);
                            break;
                        case EDIT_EVENT_RESULT_EXIT:
                            reset_event = 1;
                            event_data = EditEvent.data;
                            EditExitInterior(0);
                            MainScene->RunEvent(100, &event_data);
                            break;
                        case EDIT_EVENT_RESULT_MENU:
                            open_menu = 1;
                            menu_mode = EDIT_LOOP_WALK_MENU;
                            break;
                    }
                    if (reset_event != 0) {
                        EditEvent.Reset();
                        UnLockCharaCtrl();
                    }
                    if (LoopMode == EDIT_LOOP_WALK) {
                        if (LockChara == 0) {
                            EditControl(MainScene, &PadCtrl);
                        } else {
                            EditControl(MainScene, NULL);
                        }
                        GamePad.AutoRepeatOff();
                        MainScene->EyeViewDrawOnOff(IsWalkMode() == 0);
                        if (LockChara == 0 && MainScene->event_run == 0 && PadCtrl.Btn(0) != 0 && IsWalkMode() != 0) {
                            chara = MainScene->GetCharacter(MainScene->player_chara);
                            chara->GetPosition(talk_position);
                            chara->GetPosition(talk_height);
                            talk_position[3] = talk_height[1];
                            CSceneEventData talk_event;
                            memset(&talk_event, 0, sizeof(talk_event));
                            if (MainScene->GetTalkEvent(talk_position, &talk_event) != 0) {
                                MainScene->RunEvent(1000, &talk_event);
                                printf("chara No = %d\n", talk_event.chara_no);
                            }
                        }
                    }
                }
                break;
            case EDIT_CONTROL_EVENT:
                pause.event_skip = 1;
                pause_enabled = 1;
                if (CheckEventSkip() == 0) {
                    pause.event_skip = 0;
                }
                switch (EventLoop()) {
                    case 17:
                        change_mode = 1;
                        return_to_player = 1;
                        break;
                    case 18:
                        ResetEditEvent();
                        return_to_player = 1;
                        break;
                    case 19:
                        RestartEditEvent();
                        return_to_player = 1;
                        break;
                    case 1:
                        return_to_player = 1;
                        break;
                    case 2:
                        menu_mode = EDIT_LOOP_WALK_MENU;
                        open_menu = 1;
                        break;
                    case 3:
                        finish = 1;
                        break;
                    case 8:
                        BurnEditParts();
                        EditMapJump(SearchMapNo(EdEventInfo.jump_map_name));
                        start_event = EdEventInfo.event_no;
                        if (start_event < 0) {
                            start_event = 100;
                        }
                        printf("start_event = %d\n", start_event);
                        EdEventInfo.event_no = -1;
                        break;
                    case 4:
                        EditGotoInterior(SearchMapNo(EdEventInfo.jump_map_name), EdEventInfo.interior_entrance);
                        start_event = EdEventInfo.event_no;
                        if (start_event < 0) {
                            start_event = 100;
                        }
                        printf("start_event = %d\n", start_event);
                        EdEventInfo.event_no = -1;
                        break;
                    case 7:
                        EditExitInterior(0);
                        start_event = EdEventInfo.event_no;
                        if (start_event < 0) {
                            start_event = 100;
                        }
                        printf("start_event = %d\n", start_event);
                        EdEventInfo.event_no = -1;
                        break;
                    default:
                        if (DebugFlag != 0 && ChkEventEditStart() != 0) {
                            ControlMode = EDIT_CONTROL_EVENT_EDIT;
                        }
                        break;
                }
                if (return_to_player != 0) {
                    ControlMode = EDIT_CONTROL_PLAYER;
                    MainScene->active_camera = MainScene->before_camera;
                }
                if (start_event > 0) {
                    MainScene->RunEvent(start_event, NULL);
                }
                break;
            case EDIT_CONTROL_EVENT_EDIT:
                if (EventEdit(&WorkBuffer) == 0) {
                    ControlMode = EDIT_CONTROL_EVENT;
                }
                break;
        }
        if (pause_enabled != 0 && (PadCtrl.Btn(21) != 0 || GamePad.Connect() == 0)) {
            PauseStart(&pause);
        }
        WalkChara = MainScene->GetCharacter(MainScene->player_chara);
        if (WalkChara != NULL) {
            WalkChara->foot_se_bank = MainScene->se_base_id;
        }
        EditStep();
        if (WalkChara != NULL) {
            WalkChara->GetPosition(villager_position);
            villager_position[3] = 30.0f;
            MainScene->StayNearVillager(villager_position, stay);
        }
        if (LoopMode == EDIT_LOOP_WALK) {
            MainScene->StepVillager();
        }
        MainScene->CancelStayVillager(stay);
        sgLoopSubGame2();
        if (open_menu == 0 && LockChara == 0 && sgMenuOpenEnable() != 0 && change_mode == 0) {
            menu_requested = PadCtrl.Btn(5) != 0 || sgGetItemOver() != 0;
            menu_button = PadCtrl.Btn(5);
            quick_change = EditOnGround() != 0 && PadCtrl.Btn(23) != 0 && SubGameRunning() == 0;
            if (MainScene->fade.NowFade() != 0 && MainScene->fade.cross != 0) {
                quick_change = 0;
            }
            next_chara = GetUserData()->active_chr_no == 0;
            if ((GetUserData()->CheckQuickChange(next_chara, NULL) & 0x1) == 0) {
                quick_change = 0;
            }
            if (IsWalkMode() == 0 || GetPauseFlag() != 0) {
                quick_change = 0;
            }
            menu_enabled = 1;
            if (LoopMode != EDIT_LOOP_EDIT && SubGameRunning() == 0 && EditOnGround() == 0) {
                menu_enabled = 0;
            }
            if (menu_enabled != 0 && LoopCounter >= 3 && ControlMode == EDIT_CONTROL_PLAYER &&
                (menu_requested != 0 || quick_change != 0 || menu_button != 0)) {
                ShowOffOnceHelpMes();
                if (LoopMode == EDIT_LOOP_WALK) {
                    if (menu_requested != 0) {
                        if (NowTakePhoto() != 0) {
                            if (IsEnablePhotoMenu() != 0) {
                                MenuInfo->open_type = MENU_OPEN_INVENT;
                                HidePhoto();
                                menu_mode = EDIT_LOOP_WALK_MENU;
                                open_menu = 1;
                            }
                        } else {
                            menu_mode = EDIT_LOOP_WALK_MENU;
                            open_menu = 1;
                            MenuInfo->open_type = MENU_OPEN_MAIN_TOWN;
                        }
                    } else {
                        open_menu = 1;
                        menu_mode = EDIT_LOOP_WALK_MENU;
                        MenuInfo->open_type = MENU_OPEN_MAIN_CHARA_BG;
                        MenuInfo->param[0] = next_chara;
                    }
                } else if (LoopMode == EDIT_LOOP_EDIT && menu_button != 0) {
                    EditDataSave();
                    LoopMode = EDIT_LOOP_EDIT_PRE_MENU;
                    MenuInfo->open_type = MENU_OPEN_GEORAMA;
                    PreEditMenuCnt = 0;
                    EditModeControlLock();
                    if (menu_button != 0) {
                        MenuInfo->param[0] = -1;
                    } else {
                        EditPreMenuAnime(25);
                        MenuInfo->param[0] = GetSelPartsInfoID();
                    }
                }
            }
        }
        if (open_menu != 0) {
            EditDrawFlag |= 0x1;
        }
    }

    mgFlushRenderInfo();
    EditEvent.Draw(MainScene);
    EditDraw();
    debug_closed = 0;
    if (DebugFlag != 0) {
        if (EditDebugMode() != 0 && EditDebugLoop(MainScene, &EdDebugInfo) != 0) {
            ControlMode = old_cm;
            debug_closed = 1;
            if (ControlMode == EDIT_CONTROL_DEBUG) {
                ControlMode = EDIT_CONTROL_PLAYER;
            }
        }
        if (ControlMode != EDIT_CONTROL_DEBUG && GamePad.Down(PAD_R3) != 0 && debug_closed == 0) {
            EditDebugStart(215, &MenuBuffer);
            old_cm = ControlMode;
            ControlMode = EDIT_CONTROL_DEBUG;
        }
        LightingEdit(MainScene);
    }
    if (PauseFlag == 0) {
        if (LockChara == 0 && NowEditModeChg() == 0) {
            edit_enabled = 1;
            debug_move = DebugInfo.chara_move > 0;
            chara = MainScene->GetCharacter(MainScene->player_chara);
            if (debug_move == 0) {
                if ((GetSaveData()->GetBitCtrl() & 0x2) != 0) {
                    edit_enabled = 0;
                }
                main_map_no = MainScene->GetMainMapNo();
                if (main_map_no < 0 || main_map_no >= 5) {
                    edit_enabled = 0;
                }
            }
            if (PadCtrl.Btn(108) != 0) {
                change_mode = 1;
            }
            if (open_menu != 0) {
                change_mode = 0;
            }
            if (SubGameRunning() != 0) {
                change_mode = 0;
            }
            if (edit_enabled != 0 && chara != NULL && ControlMode == EDIT_CONTROL_PLAYER && change_mode != 0) {
                chara->GetPosition(edit_position);
                if (LoopMode == EDIT_LOOP_WALK) {
                    if (CheckWalkToEdit(MainScene, edit_position) != 0 || debug_move != 0) {
                        open_menu = 0;
                        ResetViewMode(MainScene);
                        MainScene->before_camera = MainScene->active_camera;
                        MainScene->active_camera = 3;
                        StartEditMode(MainScene);
                        LoopMode = EDIT_LOOP_EDIT;
                        KeepEditAnalyze();
                        chara->SetMotion("\x97\xA7\x82\xBF", 0);
                        MainScene->map_event_no = 0;
                    }
                } else if (LoopMode == EDIT_LOOP_EDIT && (CheckEditToWalk(MainScene, return_position) != 0 || debug_move != 0)) {
                    LoopMode = EDIT_LOOP_WALK;
                    open_menu = 0;
                    EndEditMode(MainScene, return_position);
                    EditControlStatusInit(MainScene);
                    camera = (CCameraControl *)MainScene->GetCamera(MainScene->active_camera);
                    camera_angle = 0.0f;
                    if (camera != NULL) {
                        camera_angle = camera->GetAngle();
                        camera->GetPos(camera_position);
                        camera->GetRef(camera_reference);
                    }
                    walk_camera = (CCameraControl *)MainScene->GetCamera(0);
                    if (walk_camera != NULL) {
                        walk_camera->FollowOff();
                        walk_camera->SetRef(camera_reference);
                        camera_position[1] += 0.1f;
                        walk_camera->SetPos(camera_position);
                        walk_camera->SetAngle(camera_angle);
                        walk_camera->Step(-1);
                        walk_camera->FollowOn();
                    }
                    MainScene->active_camera = 0;
                    EditDataSave();
                    GeoUpdateNpcPos(MainScene);
                    if (GetGameChapter(GetSaveData()->game_progress) < 8) {
                        change_event = -1;
                        if (MapNo == 0 && GetSaveData()->GetBitFlag(0x21) != 0 && GetSaveData()->GetBitFlag(0x22) == 0) {
                            change_event = 507;
                        }
                        if (change_event < 0 && EditAnalyzeChanged() != 0) {
                            change_event = 310;
                        }
                        if (change_event > 0) {
                            EditModeChg(change_event);
                        }
                    }
                }
            }
        }
        if (LoopMode == EDIT_LOOP_EDIT_PRE_MENU) {
            PreEditMenuCnt++;
            if (MenuInfo->param[0] < 0 || PreEditMenuCnt >= 25) {
                menu_mode = EDIT_LOOP_EDIT_MENU;
                MenuInfo->open_type = MENU_OPEN_GEORAMA;
                open_menu = 1;
                EditModeControlUnLock();
            }
        }
        if (open_menu != 0) {
            EditDrawFlag &= ~0x1;
            if (MainScene->bg_load_step <= 0) {
                if (MainScene->fade.NowFade() != 0 && MainScene->fade.cross != 0) {
                    MainScene->fade.FadeIn(0);
                }
                MenuInfo->scene = MainScene;
                if (MenuDataBuf != NULL) {
                    LoopMode = menu_mode;
                    MenuMainInit(MenuInfo);
                    if (sgGetItemOver() != 0) {
                        sgGetItemOverReset();
                    }
                }
            }
        }
        {
            static char init;

            if (init == 0) {
                rain_flag = 0;
                init = 1;
            }
        }
        if (GamePad.Down2(PAD_R2) != 0) {
            if (rain_flag == 0) {
                EventRain.Start();
            } else {
                EventRain.Stop();
            }
            rain_flag = rain_flag == 0;
        }
        MainScene->fade.FadeStep();
    }
    MainScene->fade.Draw();
    if (PauseFlag != 0) {
        pause_result = PauseMenu();
        if (pause_result == 1) {
            PauseFlag = 0;
        }
        if (pause_result == 2) {
            finish = 1;
        }
    }
    DrawEventEdit();
    FadeOutForE3();
    if (DebugFlag != 0) {
        static int start_bt_cnt;
        static int encount_flag;
        static int show_encount_cnt;
        static int next_encount;

        {
            static char init;

            if (init == 0) {
                start_bt_cnt = 0;
                init = 1;
            }
        }
        {
            static char init;

            if (init == 0) {
                encount_flag = 1;
                init = 1;
            }
        }
        {
            static char init;

            if (init == 0) {
                show_encount_cnt = 0;
                init = 1;
            }
        }
        {
            static char init;

            if (init == 0) {
                next_encount = -1;
                init = 1;
            }
        }

        if (show_time_step > 0 || show_encount_cnt > 0) {
            mgCDrawPrim prim;

            prim.Initialize(NULL, NULL);
            prim.DepthTestEnable(0);
            prim.AlphaBlendEnable(0);
            prim.ZMask(1);
            prim.TextureMapEnable(0);
            if ((show_time_step > 0 && time_step != 0) || (show_encount_cnt > 0 && encount_flag != 0)) {
                prim.Begin(MG_PRIM_TRIANGLE);
                if (show_encount_cnt > 0) {
                    prim.Color(255, 0, 0, 128);
                } else {
                    prim.Color(255, 255, 255, 128);
                }
                prim.Vertex(450, 10, 0);
                prim.Vertex(480, 25, 0);
                prim.Vertex(450, 40, 0);
                prim.End();
            } else {
                prim.Begin(MG_PRIM_SPRITE);
                if (show_encount_cnt > 0) {
                    prim.Color(255, 0, 0, 128);
                } else {
                    prim.Color(255, 255, 255, 128);
                }
                prim.Vertex(450, 10, 0);
                prim.Vertex(480, 40, 0);
                prim.End();
            }
        }
        show_time_step--;
        show_encount_cnt--;
        if (show_time_step < 0) {
            show_time_step = 0;
        }
        if (show_encount_cnt < 0) {
            show_encount_cnt = 0;
        }
    }
    if (DebugFlag != 0 && GamePad.On(PAD_SELECT) != 0 && GamePad.Down(PAD_START) != 0) {
        finish = 1;
    }
    if (MainScene->exit_flag != 0) {
        finish = 1;
    }
    if (finish != 0) {
        PauseCancel();
        PreExitLoop(MainScene);
        return 1;
    }
    return 0;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditLoop__Fv);
#endif

/**
 * Clears character locks and resets the town event controller.
 */
static void InitEditEvent() {
    InitLockCharaCtrl();
    EditEvent.Reset();
}

/**
 * Resets a running town event and releases its character lock.
 */
static void ResetEditEvent() {
    if (EditEvent.state == EDIT_EVENT_STATE_RUNNING) {
        EditEvent.Reset();
        UnLockCharaCtrl();
    }
}

/**
 * Restarts a running town event using the scene event description.
 */
static void RestartEditEvent() {
    if (EditEvent.state == EDIT_EVENT_STATE_RUNNING) {
        ResetEditEvent();
        if (EditEvent.StartEvent(&MainScene->event_data) != 0) {
            LockCharaCtrl();
        }
    }
}

int EditStep() {
    CMap          *maps[8];
    CObjAnimeEnv   environment;
    CScene        *scene;
    CCharacter2   *chara;
    CSceneEventData *event_data;
    mgCCamera     *camera;
    int            event_no;
    int            count;
    int            i;

    scene = MainScene;
    chara = scene->GetCharacter(scene->player_chara);
    WalkChara = chara;
    if (chara != NULL) {
        chara->foot_se_bank = MainScene->se_base_id;
    }
    EditModeChgStep(MainScene);
    scene = MainScene;
    if (scene->event_run != 0) {
        event_no = scene->event_no;
        printf("event = %d\n", event_no);
        event_data = &MainScene->event_data;
        if (event_no == 99999) {
            if (EditEvent.StartEvent(event_data) != 0) {
                camera = MainScene->GetCamera(MainScene->active_camera);
                if (camera == NULL || camera->Iam() != 1000) {
                    return 0;
                }
                if (event_data->event.flag & FUNC_EVENT_DOOR) {
                    if (event_data->event.unk_34 > 0) {
                        printf("   event = %d\n", event_data->event.unk_34);
                        if (RunEvent(event_data->event.unk_34, MainScene) > 0) {
                            ResetViewMode(MainScene);
                            ControlMode = EDIT_CONTROL_EVENT;
                        }
                    }
                }
                LockCharaCtrl();
            }
        } else {
            InitEvent(MainScene);
            MainScene->before_camera = MainScene->active_camera;
            if (RunEvent(event_no, MainScene) > 0) {
                ResetViewMode(MainScene);
                ControlMode = EDIT_CONTROL_EVENT;
            }
        }
        MainScene->event_run = 0;
    }
    if (GamePad.Down2(PAD_SQUARE) != 0) {
        InitEvent(MainScene);
        ReloadMapScript();
        MainScene->before_camera = MainScene->active_camera;
        if (RunEvent(150, MainScene) != 0) {
            ControlMode = EDIT_CONTROL_EVENT;
        }
    }
    count = MainScene->GetActiveMap(maps, 8);
    if (WalkChara != NULL) {
        *(u_long128 *)environment.chara_pos = *(u_long128 *)WalkChara->position;
        environment.time = MainScene->time;
        for (i = 0; i < count; i++) {
            if (maps[i] != NULL) {
                maps[i]->AnimeStep(&environment);
                maps[i]->Step();
            }
        }
    }
    MainScene->PrePlaySeSrc();
    MainScene->PlayMapSeSrc();
    if (LoopMode != EDIT_LOOP_EDIT) {
        EditStepChara(MainScene);
    }
    MainScene->EffectStep();
    MainScene->StepEffectScript(-1);
    if (InInterior() == 0) {
        StepFirePowder(MainScene);
        StepGeyserEffect(MainScene);
    }
    camera = MainScene->GetCamera(MainScene->active_camera);
    if (camera != NULL) {
        camera->Step(1);
    }
    EditExceptionStep(MapNo, MainScene);
    EventMes1.Step();
    GetSystemMessage()->Step();
    GetSystemMessage(1)->Step();
    GetSystemMessage(2)->Step();
    StepHelpMes();
    return 1;
}

#ifdef NONMATCHING
int EditDraw() {
    static int                 flag;
    static char                init;
    CMap                      *maps[8];
    CMap                      *map;
    CEditMap                  *edit_map;
    CPartsGroup               *ghost_group;
    CList<PartsGroupData>     *group_entry;
    CMapParts                 *parts;
    CList<CMapPiece>          *piece;
    CCharacter2               *chara;
    mgCCamera                 *camera;
    mgCTexture                *water;
    mgCTexture                *screen;
    mgCTexture                *overlay;
    CInventUserData           *invent;
    USER_PICTURE_INFO         *picture;
    CFuncPoint                *subject;
    CScene::InScreenCharaInfo  screen_chara;
    InScreenFuncInfo           screen_func;
    sceVu0FMATRIX             view_matrix;
    sceVu0FVECTOR             camera_pos = { 0.0f, 0.0f, 100.0f, 0.0f };
    sceVu0FVECTOR             camera_dir;
    sceVu0FVECTOR             walk_pos;
    sceVu0FVECTOR             player_pos;
    sceVu0FVECTOR             system_pos;
    sceVu0FVECTOR             screen_range;
    sceVu0FVECTOR             lighting;
    float                     blur_range[2];
    float                     photo_dist;
    int                       texture_order[65];
    int                       texture_blocks[128];
    int                       later_texture_blocks[128];
    int                       map_count;
    int                       map_draw;
    int                       ghost_visible;
    int                       water_block;
    int                       block_count;
    int                       map_index;
    int                       texture_group;
    int                       block_index;
    int                       block;
    int                       dof_off;
    int                       chara_no;
    int                       idea_no;
    int                       show_system;
    int                       main_map_no;
    int                       exit_flag;

    if (EditDrawCancelFlag != 0) {
        EditDrawCancelFlag = 0;
        return 0;
    }

    map_draw = EdEventInfo.map_draw;
    map_count = MainScene->GetActiveMap(maps, 8);
    mgSetPkTextureRepeat(0);
    MainScene->GetCamera(MainScene->active_camera);
    camera = MainScene->GetCamera(MainScene->active_camera);
    if (camera != NULL) {
        camera->GetCameraMatrix(view_matrix);
        camera->GetPos(camera_pos);
        camera->GetDir(camera_dir);
        mgSetViewMatrix(view_matrix, camera_pos);
        sndSetMicPos(camera_pos, camera_dir);
        MainScene->FixCameraPartsOnOff(camera_pos);
    }

    MainScene->DrawSky(-1);
    map = MainScene->GetMap(MainScene->active_map);
    edit_map = NULL;
    if (map != NULL && strcmp(map->Iam(), "CEditMap") == 0) {
        edit_map = (CEditMap *)map;
    }
    if (map_draw != 0 && edit_map != NULL) {
        edit_map->DrawRiverMask();
    }

    ghost_visible = GhostPhotoTiming();
    if (CheckTime(MainScene->time, 0.0f, 4.0f) == 0) {
        ghost_visible = 0;
    }
    for (map_index = 0; map_index < map_count; map_index++) {
        ghost_group = maps[map_index]->SearchPartsGroup("ghost");
        if (ghost_group != NULL) {
            for (group_entry = ghost_group->list; group_entry != NULL; group_entry = group_entry->next) {
                parts = group_entry->data.parts;
                if (parts != NULL) {
                    piece = parts->piece_list;
                    if (piece != NULL) {
                        piece->data.fade = 1;
                        piece->data.Show(ghost_visible);
                        if (ghost_visible != 0) {
                            piece->data.fade_alpha = 1.0f;
                        }
                        if ((double)piece->data.fade_alpha <= 0.0) {
                            parts->Show(0);
                        } else {
                            parts->Show(1);
                        }
                    }
                }
            }
        }
    }

    WorkBuffer.stack_used = 0;
    WorkBuffer.lock = 0;
    for (block_index = 0; block_index < 64; block_index++) {
        texture_order[block_index] = block_index;
    }
    texture_order[64] = -1;
    mgBeginDraw(&WorkBuffer, texture_order, NULL);

    if (map_draw != 0) {
        if (edit_map != NULL) {
            edit_map->DrawRiver();
        }
        EditPlaceAnime();
        for (map_index = 0; map_index < map_count; map_index++) {
            if (WalkChara != NULL) {
                WalkChara->GetPosition(walk_pos);
            }
            maps[map_index]->PreDraw(walk_pos);
            maps[map_index]->Draw();
        }
        mgSetPkTextureRepeat(1);
        if (IsEditMode() != 0 && edit_map != NULL) {
            DrawEditCursorParts(MainScene);
            EditPlaceAnimeDraw();
        }
        EditPlaceAnime2();
        water = mgTexManager.GetTexture("water", -1);
        water_block = -1;
        if (water != NULL) {
            water_block = water->block;
            WaveTable.GetEffect();
        }
        mgPreEndDraw(NULL);
        for (texture_group = 0; texture_group < 6; texture_group++) {
            block_count = MainScene->mds_list_set.GetTextureBlockNo(texture_group, texture_blocks, 128);
            for (block_index = 0; block_index < block_count; block_index++) {
                block = texture_blocks[block_count - block_index - 1];
                if (mgEndDrawReloadTexture(block, NULL) != 0 && water_block == block) {
                    WaveTable.CreateTexture(water);
                }
                mgEndDraw(block, NULL);
            }
        }
        mgSetPkTextureRepeat(0);
        sgDrawSubGameMap();
        map = MainScene->GetMap(MainScene->active_map);
        dof_off = 0;
        if (map != NULL) {
            map->GetNowTimeBand();
        }
        if (GetSaveData() != NULL) {
            dof_off = GetSaveData()->config.dof_off;
        }
        if (dof_off != 0 && IsEditMode() == 0) {
            blur_range[0] = 1000.0f;
            blur_range[1] = 2000.0f;
            screen = mgTexManager.GetTexture("work", 0x9C);
            mgTexManager.ReloadTexture(0x9C, (sceVif1Packet *)NULL);
            DepthOfField(2, blur_range, screen, 1.0f);
        } else {
            map = MainScene->GetMap(MainScene->active_map);
            if (map != NULL) {
                map->GetLightingRatio(lighting);
            }
            blur_range[0] = 3000.0f;
            blur_range[1] = 4000.0f;
            screen = mgTexManager.GetTexture("work", 0x9C);
            mgTexManager.ReloadTexture(0x9C, (sceVif1Packet *)NULL);
            DepthOfField(1, blur_range, screen, 1.0f);
        }
    }

    EdEventFirstDraw();
    if (TreasureBox != NULL) {
        map = MainScene->GetMap(MainScene->active_map);
        if (map != NULL) {
            map->DrawTrBox();
        }
    }
    if (LoopMode == EDIT_LOOP_WALK) {
        mgTexManager.ReloadTexture(0x9F, (sceVif1Packet *)NULL);
        screen = mgTexManager.GetTexture("shadow_work", 0x9F);
        mgBeginDrawShadow(screen, NULL);
        EditDrawShadowChara(MainScene);
        sgDrawSubGameCharaShadow();
        mgEndDrawShadow(screen, NULL);
        chara = MainScene->GetCharacter(MainScene->player_chara);
        if (chara != NULL) {
            chara->SetFadeFlag(1);
            chara->SetNearDist(25.0f);
        }
        EditDrawChara(MainScene);
    }
    sgDrawSubGameChara();
    mgSetPkTextureRepeat(1);
    if (map_draw != 0) {
        for (texture_group = 6; texture_group < 16; texture_group++) {
            block_count = MainScene->mds_list_set.GetTextureBlockNo(texture_group, later_texture_blocks, 128);
            for (block_index = 0; block_index < block_count; block_index++) {
                block = later_texture_blocks[block_index];
                mgEndDrawReloadTexture(block, NULL);
                mgEndDraw(block, NULL);
            }
        }
    }
    MainScene->DrawEffectScript(-1);
    if (InInterior() == 0) {
        DrawFirePowder(MainScene);
        DrawGeyserEffect(MainScene);
    }
    MainScene->DrawExclamationMark(RedBicMark);
    chara = MainScene->GetCharacter(MainScene->player_chara);
    if (chara != NULL) {
        chara->GetPosition(player_pos);
        if (BlueBicMark != NULL) {
            BlueBicMark->SetPosition(player_pos);
        }
        exit_flag = MainScene->map_event_no;
        if ((exit_flag & 0x1) != 0) {
            if (RedBicMark != NULL) {
                RedBicMark->SetPosition(0.0f, 0.0f, 0.0f);
                RedBicMark->SetPosition(player_pos);
                RedBicMark->SetRotation(0.0f, 0.0f, 0.0f);
            }
            mgDrawDirect(RedBicMark);
        }
        if ((exit_flag & 0x2) != 0) {
            mgDrawDirect(BlueBicMark);
        }
    }
    EditDrawEffectChara(MainScene);
    if (map_draw != 0) {
        screen = mgTexManager.GetTexture("water_work", 0x9E);
        overlay = mgTexManager.GetTexture("ref", 0x9E);
        camera = MainScene->GetCamera(MainScene->active_camera);
        for (map_index = 0; map_index < map_count; map_index++) {
            maps[map_index]->DrawWater(camera, screen, overlay);
        }
    }
    if (IsEditMode() != 0 && edit_map != NULL) {
        mgTexManager.ReloadTexture(0xA3, (sceVif1Packet *)NULL);
        DrawEditCursor(MainScene);
        EditPEffectStep();
        EditPEffectDraw(0xA3);
    }
    if (map_draw != 0) {
        MainScene->DrawEffect(0x42);
        mgTexManager.ReloadTexture(0xA4, (sceVif1Packet *)NULL);
        MainScene->DrawGameObject(MapNo);
    }
    sgDrawSubGameEffect();
    EdEventDraw();
    mgSetPkTextureRepeat(0);
    if (InInterior() == 0) {
        MainScene->DrawLensFlare(0x9D, "f_work", "f_work2");
    }
    sgDrawSubGameSystem();
    if (DebugFlag != 0 && DebugInfo.invent_debug == 1) {
        MainScene->DrawScreenFunc(&TestFrame);
    }

    if (NowTakePhoto() == 0) {
        InitNpcCameraReaction();
    } else {
        mgTexManager.ReloadTexture(0xA2, (sceVif1Packet *)NULL);
        mgTexManager.GetTexture("fix_work", -1);
        invent = NULL;
        if (GetUserData() != NULL) {
            invent = &GetUserData()->invent_data;
        }
        if (init == 0) {
            flag = 0;
            init = 1;
        }
        screen_chara.chara_no = -1;
        screen_chara.dist = 0.0f;
        screen_chara.in_center = 0;
        chara_no = MainScene->InScreenChara(&screen_chara, screen_range);
        chara = MainScene->GetCharacter(chara_no);
        if (chara_no >= 0 && chara != NULL && chara->GetKeyListPtr("\x83\x4A\x83\x81\x83\x89", NULL) != NULL) {
            MainScene->ExModeVillager(chara_no);
        }
        picture = invent->IsPhotoSpace(NULL);
        if (DrawTakePhoto(picture, &photo_dist) != 0 && picture != NULL) {
            screen_func.range = -1.0f;
            screen_func.unk_04 = 0;
            screen_func.dist = -1.0f;
            screen_func.range = photo_dist;
            subject = MainScene->InScreenFunc(&screen_func);
            picture->map_no = MainScene->GetMainMapNo();
            idea_no = -1;
            picture->neta_id = -1;
            picture->npc_no = -1;
            if (subject != NULL) {
                idea_no = subject->invent.unk_20;
            }
            if (idea_no == 196 && MainScene->GetMainMapNo() == 2) {
                idea_no = 2006;
            }
            if (screen_chara.dist < 5.0f + photo_dist) {
                picture->npc_no = screen_chara.chara_no;
                if (screen_chara.in_center != 0 && screen_chara.chara_no == 14) {
                    idea_no = 36;
                    picture->npc_no = -1;
                }
            }
            if (idea_no > 0) {
                picture->neta_id = idea_no;
                sndSePlay(GetSystemSndID(), 14, 0);
                printf("invent_no = %d\n", picture->neta_id);
            }
            SetTookPhotoData(picture);
        }
        if ((EditDrawFlag & 0x1) == 0) {
            DrawTakePhotoSystem(0x9A, invent);
        }
    }

    show_system = DebugInfo.param_off == 0;
    if ((GetSaveData()->GetBitCtrl() & 0x2) != 0) {
        show_system = 0;
    }
    main_map_no = MainScene->GetMainMapNo();
    if (main_map_no < 0 || main_map_no >= 5) {
        show_system = 0;
    }
    if (ControlMode != EDIT_CONTROL_PLAYER) {
        show_system = 0;
    }
    if (LoopMode != EDIT_LOOP_EDIT && LoopMode != EDIT_LOOP_WALK) {
        show_system = 0;
    }
    if (IsWalkMode() == 0) {
        show_system = 0;
    }
    if (SubGameRunning() != 0) {
        show_system = 0;
    }
    if (show_system != 0) {
        chara = MainScene->GetCharacter(MainScene->player_chara);
        if (chara != NULL) {
            chara->GetPosition(system_pos);
        }
        DrawEditSystem(0xA3, MainScene, system_pos, LoopMode == EDIT_LOOP_EDIT);
    }
    mgTexManager.ReloadTexture(0x9A, (sceVif1Packet *)NULL);
    EventMes1.DrawMesWin();
    GetSystemMessage()->DrawMesWin();
    GetSystemMessage(1)->DrawMesWin();
    GetSystemMessage(2)->DrawMesWin();
    DrawHelpMes();
    DrawEditHelpMes();
    EventTimeDraw();
    return 0;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditDraw__Fv);
#endif

/**
 * Removes opened town chests and chests whose dungeon floor has never been visited.
 */
static void UpdateTrBoxFlag(int map_no) {
    int i;

    CMapFlagData *flags = GetSaveData()->GetMapFlag(map_no);
    CMap *map = MainScene->GetMap(MainScene->active_map);
    map->UpdateTrBoxFlag(flags);
    int count = map->tr_box_num;
    CSaveDataDungeon *dungeon = &GetSaveData()->save_dungeon;
    for (i = 0; i < count; i++) {
        CMapTreasureBox *chest = map->GetTrBox(i);
        if (chest != NULL) {
            DNG_FLOOR_SAVE *floor = dungeon->GetFloorInfoPtr(chest->floor_id / 100 - 1, chest->floor_id % 100);
            if (floor != NULL && floor->visit_count <= 0) {
                map->DeleteTrBox(i, NULL);
            }
        }
    }
}

int BurnEditParts() {
    CEditMap::RemoveInfo removed;
    CEditMap           *map;
    CEditPartsInfo     *info;
    int                 i;
    int                 id;

    if (GetSaveData()->GetBitFlag(0x208) != 0) {
        return 0;
    }
    if (MainScene->GetMainMapNo() == 3) {
        map = (CEditMap *)MainScene->GetMap(MainScene->active_map);
        if (map != NULL) {
            memset(&removed, 0, sizeof(removed));
            map->BurnEditParts(&removed);
            for (i = 0; i < removed.house_num; i++) {
                GetSaveData()->user_data.LeaveHouse(removed.house_npc[i]);
            }
            for (id = 0; id < 256; id++) {
                info = map->GetePartsInfoAtID(id);
                if (info != NULL && !(info->attr & 0x8000) && !(info->attr & EDIT_PARTS_ATR_BURN)) {
                    if (removed.parts_num[id] > 0) {
                        GetSaveData()->AddBuildPartsNum(id, removed.parts_num[id]);
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

/**
 * Loads the sound set for a map and starts its default background music.
 */
static void editLoadSound(int map_no) {
    int sound_no;
    int bgm_no;

    sound_no = GetMapSndDataID(map_no);
    MainScene->LoadSound(sound_no, read_buffer);
    if (MainScene->skip_load_bgm == 0) {
        bgm_no = MainScene->GetDefBgmNo(sound_no);
        if (bgm_no == -1) {
            MainScene->StopBGM(0);
        }
        if (MainScene->CheckLoadBGM(bgm_no) == 0 || bgm_no == 9999) {
            MainScene->PlayBGM(0, -1, 1.0f);
            return;
        }
        MainScene->StopBGM(0);
        if (MainScene->LoadBGM(bgm_no, read_buffer) != 0) {
            MainScene->PlayBGM(0, -1, 1.0f);
            if (bgm_no == 0) {
                MainScene->AutoChangeBGMVol(1);
                MainScene->StepSnd();
                sndStep(2.0f);
            }
        }
    } else {
        MainScene->skip_load_bgm = 0;
    }
}

int EditMapJump(int map_no) {
    SCN_LOADMAP_INFO2 info;
    char             path[136];
    int              size;
    char             language[4];
    int              sub_map_no;
    char            *name;
    mgCMemory       *main_data;
    int              edit_data_no;
    CEditMap        *map;
    mgCTextureManager *textures;
    int              used;
    CCameraControl  *camera;
    CameraCtrlParam *param;
    CEditData       *data;
    int              loaded;
    int              free_quadwords;

    InitEditEvent();
    DeleteFileCache();
    textures = &mgTexManager;
    sub_map_no = map_no;
    if (map_no > 10 && map_no < 15) {
        map_no = 10;
    } else {
        sub_map_no = -1;
    }
    InitSubMapLoadStep();
    name = GetMapName(map_no, NULL);
    if (name == NULL) {
        printf("not found map %d\n", map_no);
        return 0;
    }
    MainScene->SeAllStop();
    EditDataSave();
    MainScene->ResetWind();
    InitSphida();
    mgWaitFrame();
    if (GetMapType(map_no) == 1) {
        SetDataPacket(2);
    } else {
        SetDataPacket(1);
    }
    free_quadwords = ControlCharaBuff.stack_size;
    ControlCharaBuff.lock = 1;
    MainDataBuff.stSetBuffer(ControlCharaBuff.stack + ControlCharaBuff.stack_used, free_quadwords - ControlCharaBuff.stack_used);
    MainDataBuff.stack_used = 0;
    MainDataBuff.lock = 0;
    printf("MAP DATA %x\n", MainDataBuff.stack + MainDataBuff.stack_used);
    info.Initialize();
    if (GetLoadMapInfo(&info, map_no) == 0) {
        return 0;
    }
    if (map_no == SearchMapNo("f01") && GetSaveData()->GetEditData(0)->GetAnalyzeFlag(0, 3) == 0) {
        strcat(info.files[0].map_name, "_2");
        strcat(info.files[0].mpk_name, "_2");
        strcat(info.files[0].ipk_name, "_2");
    }
    MainScene->DeleteVillager();
    MainScene->DeleteSubVillager();
    MapJump(MainScene, &info, map_no);
    EditMapInitEvent(map_no, (CEditMap *)MainScene->GetMap(0));
    main_data = &MainDataBuff;
    ResetNpcTalkMes();
    if (map_no == 10 || map_no == 34 || map_no == 121) {
        main_data->Align64();
        LoadNpcTalkMes(main_data);
    }
    NowLoadingBarStep();
    MapNo = SearchMapNo(name);
    MainScene->SetNowMapNo(MapNo);
    EdEventMapInit();
    if (GetMapType(MapNo) == 1) {
        used = main_data->stack_used;
        map = (CEditMap *)MainScene->GetMap(0);
        map->CreateTable(main_data, 256, 40960);
        GetMapPath(path, name);
        sprintf(language, "%d", LanguageCode);
        strcat(path, language);
        strcat(path, ".gpi");
        if (LoadFile2(path, read_buffer, &size, 0) != 0) {
            map->info_mngr.LoadEditInfo((char *)read_buffer, size, main_data);
        }
        GetMapPath(path, name);
        strcat(path, ".cfg");
        if (LoadFile2(path, read_buffer, &size, 0) != 0) {
            map->LoadEditInfo((char *)read_buffer, size, main_data);
        }
        map->area_no = MainScene->now_map_no;
        map->ClearAllParts();
        printf("edit data %dKB\n", (main_data->stack_used - used) * 16 / 1024);
        EditDataLoad();
        if (map_no == 0) {
            map->river_poly_margin = 15.0f;
        }
    }
    NowLoadingBarStep();
    MainScene->LoadGameObject(map_no, 164, main_data);
    if (GetMapType(MapNo) == 5) {
        edit_data_no = 0;
        SearchMapNo("f01");
        if (map_no == SearchMapNo("f02")) {
            edit_data_no = 1;
        }
        if (map_no == SearchMapNo("f03")) {
            edit_data_no = 2;
        }
        if (map_no == SearchMapNo("f04")) {
            edit_data_no = 3;
        }
        data = GetSaveData()->GetEditData(edit_data_no);
        map = (CEditMap *)MainScene->GetMap(MainScene->active_map);
        if (map != NULL) {
            map->PartsOnOff(edit_data_no, data);
        }
    }
    map = (CEditMap *)MainScene->GetMap(MainScene->active_map);
    if (map != NULL) {
        if (TreasureBox != NULL) {
            map->CreateTrBox(TreasureBox, 173, main_data);
            UpdateTrBoxFlag(MapNo);
        }
        map->now_time = MainScene->time;
    }
    InitFirePowder(map_no, MainScene, 208, main_data);
    InitGeyserEffect(map_no, MainScene, 209, main_data);
    EditControlInit(MainScene);
    NowLoadingBarStep();
    editLoadSound(map_no);
    NowLoadingBarStep();
    MainScene->DeleteVillager();
    MainScene->LoadVillager(MapNo, 78);
    NowLoadingBarStep();
    if (sub_map_no > 0) {
        loaded = LoadSubMap(MainScene, sub_map_no, 0);
        NowLoadingBarStep();
        if (loaded != 0) {
            MainScene->SetActive(2, 1);
            MainScene->LoadSubVillager(GetSubMapNo(), 94);
            EditMapInitEvent(sub_map_no, (CEditMap *)MainScene->GetMap(1));
        }
        NowLoadingBarStep();
    } else {
        NowLoadingBarStep();
        NowLoadingBarStep();
    }
    edit_data_no = MapNo;
    if (MapNo == SearchMapNo("f01")) {
        edit_data_no = 0;
    }
    if (MapNo == SearchMapNo("f02")) {
        edit_data_no = 1;
    }
    if (MapNo == SearchMapNo("f03")) {
        edit_data_no = 2;
    }
    if (MapNo == SearchMapNo("f04")) {
        edit_data_no = 3;
    }
    EdDebugInfo.edit_data_no = edit_data_no;
    EdDebugInfo.edit_data = GetSaveData()->GetEditData(edit_data_no);
    mgPlightEnable(0);
    MainScene->UpDateMapInfo();
    camera = (CCameraControl *)MainScene->GetCamera(MainScene->active_camera);
    if (camera != NULL) {
        param = camera->GetActiveParam();
        param->min_dist = camera->default_param.min_dist;
        param->max_dist = camera->default_param.max_dist;
        param->near_height = camera->default_param.near_height;
        param->far_height = camera->default_param.far_height;
        param->height = camera->default_param.height;
        param->max_height = camera->default_param.max_height;
        param->min_height = camera->default_param.min_height;
        param->rest_max_height = camera->default_param.rest_max_height;
        param->rest_min_height = camera->default_param.rest_min_height;
        param->ground_space = camera->default_param.ground_space;
        param->no_check = camera->default_param.no_check;
    }
    textures->ReloadTexture(-1, (sceVif1Packet *)NULL);
    LoopCounter = 0;
    EditDrawCancelFlag = 1;
    InitS51Thunder();
    return 1;
}

int EditGotoInterior(int map_no, int delete_villager) {
    sceVu0FVECTOR  position;
    mgCMemory     *stack;
    CMap          *map;

    InitEditEvent();
    DeleteFileCache();
    EditDataSave();
    MainScene->StopSeSrc();
    DelMainNPCflag = 0;
    if (delete_villager != 0) {
        MainScene->DeleteVillager();
        DelMainNPCflag = 1;
    }
    MainScene->DeleteSubVillager();
    if (InInterior() != 0) {
        InteriorMapJump(MainScene, map_no);
    } else {
        GotoInterior(MainScene, map_no);
    }
    editLoadSound(map_no);
    stack = MainScene->GetStack(3);
    map = MainScene->GetMap(MainScene->active_map);
    if (map != NULL && TreasureBox != NULL) {
        map->CreateTrBox(TreasureBox, 173, stack);
        UpdateTrBoxFlag(map_no);
    }
    MainScene->LoadSubVillager(map_no, 94);
    MainScene->SetActiveVillager();
    EditControlInit(MainScene);
    if (EditEvent.door_se >= 0) {
        MainScene->SePlayCloseDoor(EditEvent.door_se, position);
    }
    mgPlightEnable(0);
    MainScene->UpDateMapInfo();
    return 1;
}

int EditExitInterior(int arg) {
    sceVu0FVECTOR position;
    int           map_no;
    int           time;
    int           saved_time;

    InitEditEvent();
    DeleteFileCache();
    MainScene->StopSeSrc();
    MainScene->DeleteSubVillager();
    time = MainScene->GetNowVillagerTime();
    saved_time = MainScene->villager_time;
    if (DelMainNPCflag != 0 || time != saved_time) {
        DeleteInterior(MainScene);
        MainScene->DeleteVillager();
        MainScene->LoadVillager(GetMainMapNo(), 78);
    }
    map_no = -1;
    ExitInterior(MainScene, &map_no);
    EditMapInitEvent(map_no, (CEditMap *)MainScene->GetMap(1));
    MainScene->LoadSubVillager(GetSubMapNo(), 94);
    MainScene->SetActiveVillager();
    EditControlInit(MainScene);
    if (EditEvent.door_se >= 0) {
        MainScene->SePlayCloseDoor(EditEvent.door_se, position);
    }
    MainScene->LoadSound(GetMapSndDataID(GetMainMapNo()), read_buffer);
    mgPlightEnable(0);
    MainScene->UpDateMapInfo();
    return 1;
}

void EditDataSave() {
    CEditData *data;
    CMap      *base;
    CEditMap  *map;

    if (InInterior() == 0) {
        data = GetSaveData()->GetEditData(MapNo);
        if (data != NULL) {
            base = MainScene->GetMap(MainScene->active_map);
            if (base != NULL && strcmp(base->Iam(), "CEditMap") == 0 && base != NULL) {
                map = (CEditMap *)base;
                map->SaveData(data);
                GetSaveData()->GetBitFlag(0x208);
                data->culture_point = map->CultureAnalyze(0);
                data->save_count++;
                map->GroundBalance(0);
                map->UpdateHouse();
                if (DebugInfo.georama_debug == 0 && GetMapType(MapNo) == 1) {
                    AnalyzeEditMap(MapNo, map);
                }
            }
        }
    }
}

#ifdef NONMATCHING
void EditDataLoad() {
    EP_PLACE_INFO   placement;
    sceVu0FVECTOR  positions[2] = { { 54.0f, 0.0f, 454.0f, 0.0f }, { -103.0f, 0.0f, 397.0f, 0.0f } };
    sceVu0FVECTOR  rotation = { 0.0f, 0.0f, 0.0f, 0.0f };
    CMap          *base;
    CEditMap      *map;
    CEditData     *data;
    CSaveData     *save;
    CEditPartsInfo *info;
    int            slot;
    int            i;

    base = MainScene->GetMap(MainScene->active_map);
    if (base == NULL) {
        return;
    }
    data = GetSaveData()->GetEditData(MapNo);
    if (data == NULL || strcmp(base->Iam(), "CEditMap") != 0) {
        return;
    }
    map = (CEditMap *)base;
    map->ClearAllParts();
    map->LoadData(data);
    map->InitialPlaceParts(data);
    map->GroundBalance(0);
    map->UpdateHouse();
    if (DebugInfo.georama_debug == 0 && GetMapType(MapNo) == 1) {
        AnalyzeEditMap(MapNo, map);
    }
    save = GetSaveData();
    if (MapNo == 0 && save->GetBitFlag(0xFA) != 0 && save->GetBitFlag(0x3D) == 0) {
        info = map->GetePartsInfoAtID(19);
        for (i = 0; i < 2; i++) {
            if (map->CheckEditParts(info, positions[i], 0.0f, &placement) != 0) {
                slot = map->BuildEditParts(19);
                if (slot >= 0) {
                    map->PlaceEditParts(slot, &placement, positions[i], rotation, NULL);
                }
            }
        }
        save->SetBitFlag(0x3D, 1);
    }
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/editloop", EditDataLoad__Fv);
#endif

void KeepEditAnalyze() {
    CEditData *data;
    int        i;

    data = GetSaveData()->GetEditData(MapNo);
    for (i = 0; i < 16; i++) {
        beforeAnalyze[i] = data->GetAnalyzeFlag(MapNo, i);
    }
}

int EditAnalyzeChanged() {
    CEditData *data;
    int        i;
    int        flag;

    if (GetGameChapter(GetSaveData()->game_progress) >= 8) {
        return 0;
    }
    data = GetSaveData()->GetEditData(MapNo);
    for (i = 0; i < 16; i++) {
        flag = data->GetAnalyzeFlag(MapNo, i);
        if (flag != beforeAnalyze[i]) {
            return 1;
        }
    }
    return 0;
}

void LoadComVillaager(void) {
}

void LoadMap(void) {
    LoadComVillaager();
}

// Static initialiser (.init)
// Initializes WaveTable, EventMes1, the memory stacks, EditEvent, EdDebugInfo, TestVisual and TestFrame.

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1045__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1053__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1528__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2271__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_3040__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1032__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1033__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1395__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1396__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1397__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1398__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1399__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1400__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1401__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1402__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1403__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1404__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1405__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1406__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1407__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1408__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1409__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1410__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1411__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1412__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1413__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1414__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1415__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1416__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1417__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1418__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1419__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1420__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1421__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_1422__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2125__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2126__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2127__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2128__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2129__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2130__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2131__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2132__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2133__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2134__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2136__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2261__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2262__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2747__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2748__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2749__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2750__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2751__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2752__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2753__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2948__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2949__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2950__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2951__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2952__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2953__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2954__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2955__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2956__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2957__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2958__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", MenuInfo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", DataPktMode__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2346__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/editloop", at_2352__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(WaterFrame, 0x4);
INCLUDE_BSS(RedBicMark, 0x4);
INCLUDE_BSS(BlueBicMark, 0x4);
INCLUDE_BSS(TreasureBox, 0x4);
INCLUDE_BSS(MapNo, 0x4);
INCLUDE_BSS(Camera, 0x4);
static INCLUDE_BSS(EventCamera, 0x4);
INCLUDE_BSS(FixCamera, 0x4);
INCLUDE_BSS(EditCamera, 0x4);
INCLUDE_BSS(ActiveCharaNo, 0x4);
INCLUDE_BSS(ControlCharaID, 0x4);
INCLUDE_BSS(WalkChara, 0x4);
INCLUDE_BSS(LoopCounter, 0x4);
INCLUDE_BSS(LoopMode, 0x4);
INCLUDE_BSS(ControlMode, 0x4);
INCLUDE_BSS(SubMapLoadBG, 0x4);
INCLUDE_BSS(now_load_map_no, 0x4);
INCLUDE_BSS(EventSquareJump, 0x4);
INCLUDE_BSS(EditDrawFlag, 0x4);
INCLUDE_BSS(EditDrawCancelFlag, 0x4);
INCLUDE_BSS(PauseFlag, 0x4);
INCLUDE_BSS(LockChara, 0x4);
INCLUDE_BSS(PreEditMenuCnt, 0x4);
INCLUDE_BSS(EditModeChgFlag, 0x4);
INCLUDE_BSS(EditModeChgCnt, 0x4);
INCLUDE_BSS(EditModeChgEvent, 0x4);
INCLUDE_BSS(MainScene__2, 0x4);
INCLUDE_BSS(main_pkt1, 0x4);
INCLUDE_BSS(main_pkt2, 0x4);
INCLUDE_BSS(read_buffer_end, 0x4);
INCLUDE_BSS(MenuDataBuf, 0x4);
INCLUDE_BSS(MenuDataSize, 0x4);
INCLUDE_BSS(FixCharaBuffSize, 0x4);
INCLUDE_BSS(CrossFadeBuff, 0x4);
INCLUDE_BSS(time_step_1481, 0x4);
INCLUDE_BSS(init_1482, 0x4);
INCLUDE_BSS(show_time_step_1484, 0x4);
INCLUDE_BSS(init_1485, 0x4);
INCLUDE_BSS(old_cm_1772, 0x4);
INCLUDE_BSS(rain_flag_1849, 0x4);
INCLUDE_BSS(init_1850, 0x4);
INCLUDE_BSS(start_bt_cnt_1865, 0x4);
INCLUDE_BSS(init_1866, 0x4);
INCLUDE_BSS(encount_flag_1868, 0x4);
INCLUDE_BSS(init_1869, 0x4);
INCLUDE_BSS(show_encount_cnt_1871, 0x4);
INCLUDE_BSS(init_1872, 0x4);
INCLUDE_BSS(next_encount_1874, 0x4);
INCLUDE_BSS(init_1875, 0x4);
INCLUDE_BSS(flag_2408, 0x4);
INCLUDE_BSS(init_2409, 0x4);
INCLUDE_BSS(DelMainNPCflag, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_949, 0x10);
INCLUDE_BSS(WaveTable, 0x1210);
INCLUDE_BSS(CharaOldPos, 0x10);
INCLUDE_BSS(EventMes1, 0x2960);
INCLUDE_BSS(buf0, 0x30);
INCLUDE_BSS(buf1, 0x30);
INCLUDE_BSS(data_buf__2, 0x60);
INCLUDE_BSS(init_dbuf, 0x60);
INCLUDE_BSS(WorkBuffer, 0x30);
INCLUDE_BSS(MenuBuffer__2, 0x30);
INCLUDE_BSS(ChrEffBuffer, 0x30);
INCLUDE_BSS(ScriptBuffer__2, 0x30);
INCLUDE_BSS(TotalDataBuff, 0x30);
INCLUDE_BSS(ControlCharaBuff, 0x30);
INCLUDE_BSS(MainDataBuff, 0x30);
INCLUDE_BSS(MainCharaBuff, 0x30);
INCLUDE_BSS(SubDataBuff, 0x30);
INCLUDE_BSS(SubCharaBuff, 0x30);
INCLUDE_BSS(EventBuff, 0xC0);
INCLUDE_BSS(CharaBufs, 0x180);
INCLUDE_BSS(FishingBuff, 0x30);
INCLUDE_BSS(SkyBuff, 0x30);
INCLUDE_BSS(EditEvent, 0x150);
INCLUDE_BSS(EdDebugInfo, 0x40);
INCLUDE_BSS(TestVisual, 0x50);
INCLUDE_BSS(TestFrame, 0x110);
INCLUDE_BSS(at_1077, 0x10);
INCLUDE_BSS(at_3041, 0x10);
INCLUDE_BSS(beforeAnalyze, 0x40);
