#include "common.h"
#include "fishing.hpp"
#include "mg_memory.hpp"
#include "mg_drawprim.hpp"
#include "mg_texture.hpp"
#include "mg_frame.hpp"
#include "mg_drawenv.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "actionchara.hpp"
#include "scene.hpp"
#include "object.hpp"
#include "padcontrol.hpp"
#include "cameracontrol.hpp"
#include "fishingobj.hpp"
#include "subgame.hpp"
#include "scenesnd.hpp"
#include "snd_mngr.hpp"
#include "savedata.hpp"
#include "nd_meswin.hpp"
#include "scriptinterpreter.hpp"
#include "editctrl.hpp"
#include "mainloop.hpp"
#include "gamepad.hpp"
#include "effscript.hpp"
#include "helpmes.hpp"
#include "sceneload.hpp"
#include "dataread.hpp"
#include "gamedata.hpp"
#include "dngfloor.hpp"
#include <eekernel.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include "intersection.hpp"
#include <cstdlib>
#include "sceneevent.hpp"

enum { kFishShapeCount = 5, kFishShapeCircle = 2 };
const int kFishPlaceValueCount = 5;
const int kFishPlaceMaxFish = 8;
enum {
    kUkiStart = 0,
    kUkiWaitBite = 1,
    kUkiPoke = 2,
    kUkiPull = 3,
    kUkiReeledIn = 4,
    kUkiCharge = 5,
    kUkiBite = 6,
};
enum { kFishingModeFloat = 1, kFishingModeLure = 2 };
const int kFishCameraState = 1000;
enum {
    kFishBtnCameraToggle = 6,
    kFishBtnAction = 0x78,
    kFishBtnReel = 0x79,
    kFishBtnReelFast = 0x7A,
};
enum {
    kFishAxisUp = 4,
    kFishAxisSide = 5,
};

enum { kPadButtonDebugJump = 1, kPadButtonCancel = 1, kPadButtonCast = 0x78 };

enum { kCameraSettled = 1000 };
enum { kLastFishParam = 18 };
enum { kLoadThreadPriority = 10 };
enum {
    kCharaModeReel = 6,
    kCharaModeReel2 = 7,
};
enum { kCaptureSeed = 0xC31AFF };

static FISH_PARAM *GetFishParam(int index);
static void EsaInit(void);
static void ReplayPrevBGM(CScene *scene);
static int LoadExMotionBG(SubGameInfo *info, u_long128 *buffer);
static int LoadExMotionStep(SubGameInfo *info, mgCMemory *memory);
static void SetNextMode(int mode);
static void ExitFishing(CScene *scene);
static int InitDataLoading(void);
static int switch_thread(void);
static int CreateLoadThread(mgCMemory *memory);
static int StepLoadThread(void);
static void DeleteLoadThread(void);
static void DrawNumber(mgCDrawPrim *prim, int digit, int x, int y);
static void CharaControl(CScene *scene, CPadControl *pad);
static int InitSelectCastingPoint(CScene *scene);
static int EndSelectCastingPoint(CScene *scene);
static void SelectCastingPoint(CScene *scene, CPadControl *pad);
static void DrawHamon(float *pos, float scale);
static void DrawSplash(float *pos, float scale);
static int GetMotionCount(CCharacter2 *chara, char *name, int min_count, int max_count, int min_speed);
static int InitCasting(CScene *scene);
static void CastingLoop(CScene *scene, CPadControl *pad);
static int InitUkiWait(CScene *scene);
static void UkiWaitLoop(CScene *scene, CPadControl *pad);
static int InitBattle(CScene *scene);
static void BattleLoop(CScene *scene, CPadControl *pad);
static float GetFishDist(CScene *scene);
static void DeleteEsa(void);
static int InitFalse(CScene *scene);
static void FalseLoop(CScene *scene, CPadControl *pad);
static int InitSuccess(CScene *scene);
static void SuccessLoop(CScene *scene, CPadControl *pad);
static int CheckFishing(float *pos, CCPoly *polys, int count);
static float GetRandamNumber(float center, float high, float floor);
static int GetUkiPokeTime(FISH_DATA *fish);
static s32 GetUkiPullTime(FISH_DATA *fish);
static int FishLoadBG(FISH_DATA *fish, u_long128 *buffer);
static int fpFISH_MAP_NUM(SPI_STACK *args, int arg_count);
static int fpFISH_MAP(SPI_STACK *args, int arg_count);
static int fpFISH_PLACE(SPI_STACK *args, int arg_count);
static int fpFISH(SPI_STACK *args, int arg_count);
static int fpFISH_MAP_END(SPI_STACK *args, int arg_count);
static int CheckCasting(CScene *scene, float *position, float *direction);
static int GetUkiWaitTime(FISH_DATA *fish, CScene *scene, float *position, int rod_no, int bait_no);
static void LineTensionStep(FISH_DATA *fish, int action);
static void StepDataLoading(void *arg);

static mgCMemory EsaStack;
static mgCMemory SndStack;
static CCameraControl CameraInfo;
static CCameraControl UkiCameraInfo;
static mgCMemory MotionBuff;
static mgCMemory ReadStack;
static mgCMemory FishingBuff;
static mgCMemory FishStack;

static FISHING_ROD_DATA RodData;
static FISH_DATA FishData;
static mgCMemory *fpStack;
static FISH_PLACE_MAP *fpNowFishPlaceMap;
static FISH_PLACE_MAP *FishPlaceMap;
static int FishPlaceMapNum;
static u_int fpNowFishPlaceMapNum;
static int RodActFlag;
static int UkiCameraFlag;
static int UkiMode;
static int UkiModeCnt;
static u_int FishSnd;
static int CastStep;
static int CastCount;
static int CastTime;
static int CastMotionCnt;
static sceVu0FVECTOR CastPoint;
static float UkiCamOldRot;
static int DrawHit;
static float LineTension;
static int addLineTension;
static int MinLineTension;
static float LineMaxLen;
static float LineMinLen;
static float FishMinLen;
static float FishMaxLen;
static int RodStatus;
static int RodStatusCnt;
static int ActionCount;
static int ActionDecCount;
static int WindReel;
static int BattleBgmCnt;
static int BattleCount;
static int FalseStep;
static int FalseMotionCount;
static int FavoredEsa;
static u_int EsaChara;
static int FalseStep2;
static int FishMesNo;
static float oldPadRx;
static float oldPadRy;
static int RunEventNo;
static int MardanEventMap;
static int MardanEventPlace;
static float CastDistSizeRate;
static int RodNo;
static int LocalEsaNo;
static int RodActionPoint;
static int LoadFishFlag;
static int FishFontH;
static CCharacter2 *FishChara;
static CCharacter2 *MainChara;
static int FishTexb;
static u_long128 *ReadBuffer;
static int GetItemRet;
static int DrawCongra;
#ifdef NONMATCHING
static u_int FanSnd;
#endif

static float CastDist;
static sceVu0FVECTOR CastPointCur;
static CObjectFrame *Lure;
static int LureFrame;
static int EsaNo;
static int LureNo;
static int NextCharaMode;
static int CharaMode;
static int RetCode;
static int CastOKFlag;
static int fgLoopMode;
static mgCFrame *RodHand;
static CObjectFrame *UkiRod;
static CScene::BGM_STATUS BgmStatus;
static int FishingTexb;
static int SystemTexb;
static int BgmReadFlag;
static int LoadExMotionFlag;
static u_long128 *ex_mtn_buff;
static int ThreadRunning;
static int step_end_flag;
static CEffectScriptMan *EffectMan;
static u_char *ThreadStack;
static int TheadID;
int stack_size;
static FISH_PARAM FishParam[19];
static SPI_TAG_PARAM tag[6] = {
    {"FISH_MAP_NUM", fpFISH_MAP_NUM},
    {"FISH_MAP", fpFISH_MAP},
    {"FISH_PLACE", fpFISH_PLACE},
    {"FISH", fpFISH},
    {"FISH_MAP_END", fpFISH_MAP_END},
    {NULL, NULL},
};

/**
 * Gives the unused quadwords in the memory stack.
 */
static inline int FreeSize(mgCMemory *memory) {
    return memory->stack_size - memory->stack_used;
}

/**
 * Gives the next allocation address in the memory stack.
 */
static inline u_char *FreeTop(mgCMemory *memory) {
    return (u_char *)&memory->stack[memory->stack_used];
}

// Code (.text)
/**
 * Gets the parameters of a fish kind, or null for an invalid index.
 */
static FISH_PARAM *GetFishParam(int index) {
    if (index < 0 || index > kLastFishParam) {
        return NULL;
    }
    return &FishParam[index];
}

/**
 * Clears the selected bait and lure and resets the lure model.
 */
static void EsaInit(void) {
    CObjectFrame *lure = Lure;
    EsaChara = 0;
    LureFrame = 0;
    EsaNo = -1;
    LocalEsaNo = -1;
    FavoredEsa = -1;
    LureNo = -1;
    lure->Initialize();
}

/**
 * Restores the music that played before fishing.
 */
static void ReplayPrevBGM(CScene *scene) {
    u_char *read_buff;
    if (BgmReadFlag == 0) {
        scene->SetActiveBgmStatus(&BgmStatus);
        return;
    }
    read_buff = (u_char *)scene->read_buff + 0x100000;
    if (BgmStatus.load_no < 0) {
        scene->StopBGM(0);
        scene->InitBGM();
        return;
    }
    scene->LoadBGM(BgmStatus.load_no, (u_long128 *)read_buff);
    scene->SetActiveBgmStatus(&BgmStatus);
}

/**
 * Starts loading the player character's fishing motions outside a dungeon.
 */
static int LoadExMotionBG(SubGameInfo *info, u_long128 *buffer) {
    LoadExMotionFlag = 0;
    if (info->dungeon != 0) {
        return 0;
    }
    StartReadBG();
    if (LoadFileBG("chara/c01_fishing.chr", buffer, 0) == 0) {
        return 0;
    }
    ex_mtn_buff = buffer;
    LoadExMotionFlag = 1;
    return 1;
}

/**
 * Installs the fishing motions when their background read finishes.
 */
static int LoadExMotionStep(SubGameInfo *info, mgCMemory *memory) {
    CCharacter2 *chara;
    CScene *scene;

    if (info->dungeon != 0) {
        return 0;
    }
    if (ReadBGSync() != 0) {
        return 1;
    }
    if (LoadExMotionFlag == 0) {
        return 0;
    }
    scene = info->scene;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara != NULL) {
        chara->LoadPack((u_int *)ex_mtn_buff, "info.cfg", memory, memory, memory, 0, NULL);
    }
    LoadExMotionFlag = 0;
    return 0;
}

/**
 * Selects the next fishing step.
 */
static void SetNextMode(int mode) {
    NextCharaMode = mode;
}

/**
 * Marks the fishing sub game for exit.
 */
static void ExitFishing(CScene *scene) {
    RetCode = 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", sgInitFishing__FP11SubGameInfo);

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", sgRestartFishing__FP11SubGameInfo);

/**
 * Prepares the music, memory and fishing state for loading.
 */
static int InitDataLoading(void) {
    SubGameInfo *info;
    CScene *scene;
    int bgm_no;
    mgCMemory *memory;

    info = GetNowSubGameInfo();
    memory = info->load_buff;
    scene = info->scene;
    if (memory != NULL) {
        memory->stack_used = 0;
        memory->lock = 0;
        memory->Align64();
    } else {
        scene->AssignStack(5);
        scene->GetStack(5);
    }
    bgm_no = scene->GetDefBgmNo(0x205);
    if (info->keep_bgm == 0) {
        scene->GetActiveBgmStatus(&BgmStatus);
        if (scene->CheckLoadBGM(bgm_no) != 0) {
            scene->StopBGM(0);
        }
    }
    EffectMan = (CEffectScriptMan *)scene->GetEffect(0);
    CharaMode = FISHING_CHARA_MODE_CONTROL;
    NextCharaMode = -1;
    RunEventNo = -1;
    CastOKFlag = 0;
    MardanEventMap = 0;
    MardanEventPlace = 0;
    RodNo = info->rod_no;
    EsaNo = info->esa_no;
    RetCode = 0;
    oldPadRy = 0;
    oldPadRx = 0;
    DrawCongra = 0;
    DrawHit = 0;
    return 1;
}

/**
 * Yields to another ready thread at the loading priority.
 */
static int switch_thread(void) {
    return RotateThreadReadyQueue(kLoadThreadPriority);
}

/**
 * Starts the thread that loads the fishing data.
 */
static int CreateLoadThread(mgCMemory *memory) {
    ThreadParam param;
    int misalignment;

    stack_size = 0x40000;

    ThreadStack = (u_char *)memory->Alloc(0x4001);
    misalignment = (u_int)ThreadStack & 0x3F;
    if (misalignment != 0) {
        ThreadStack = &ThreadStack[0x40 - misalignment];
    }
    param.entry = StepDataLoading;
    step_end_flag = 0;
    param.initPriority = kLoadThreadPriority;
    param.option = 0;
    param.gpReg = &_gp;
    param.stack = ThreadStack;
    param.stackSize = stack_size;
    TheadID = CreateThread(&param);
    ThreadRunning = 1;
    StartThread(TheadID, NULL);
    return 1;
}

/**
 * Yields while the fishing data thread is running.
 */
static int StepLoadThread(void) {
    if (ThreadRunning == 0) {
        return 0;
    }
    switch_thread();

    return !(step_end_flag != 0);
}

/**
 * Waits for and deletes the fishing data thread.
 */
static void DeleteLoadThread(void) {
    if (ThreadRunning != 0) {
        while (StepLoadThread() != 0) {
        }
        TerminateThread(TheadID);
        DeleteThread(TheadID);
        ThreadRunning = 0;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", StepDataLoading__FPv);

int sgBreakFishing(void) {
    DeleteLoadThread();
    sgExitFishing(GetNowSubGameInfo());
    return 1;
}

int sgExitFishing(SubGameInfo *info) {
    CCharacter2 *chara;
    CScene *scene;

    mgTexManager.DeleteBlock(FishingTexb);
    mgTexManager.DeleteBlock(FishTexb);
    mgTexManager.DeleteBlock(SystemTexb);
    ReEquipFishingGameWeapon();
    if (info != NULL && info->scene != NULL) {
        ReplayPrevBGM(info->scene);
    }
    scene = info->scene;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara != NULL) {
        chara->DeleteExtMotion();
        chara->SetMotion("\227\247\202\277", 0);
    }
    BgmReadFlag = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", sgLoopFishing__FP11SubGameInfo);

int sgLoopFishing2(SubGameInfo *info) {
    CScene *scene;

    u_long128 rod_state[0x1400];
    sceVu0FVECTOR hand_pos;
    CCharacter2 *chara;
    mgCFrame *frame;

    if (fgLoopMode == 0) {
        return 0;
    }
    scene = info->scene;
    MainChara->UpdatePosition();
    UkiRod->frame->SetReference(RodHand);
    RodStep(scene, rod_state);
    if (CharaMode == kCharaModeReel || CharaMode == kCharaModeReel2) {
        chara = scene->GetCharacter(scene->player_chara);
        if (chara != NULL) {
            frame = chara->CObjectFrame::frame;
            if (frame != NULL) {
                frame = frame->SearchFrame("item");
            }
            if (frame != NULL) {
                frame->GetWorldPosition0(hand_pos);
                if (CatchLine(hand_pos, 50.0f) == 0) {
                    SlowLineVelo(0.2f);
                }
            }
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", sgDrawFishing__FP11SubGameInfo);

/**
 * Draws one digit from the fishing display texture.
 */
static void DrawNumber(mgCDrawPrim *prim, int digit, int x, int y) {
    int tex_u = 0;
    tex_u += digit * 12;
    prim->TextureCrd(tex_u, 0x72);
    prim->Vertex(x, y, 0);
    prim->TextureCrd(tex_u + 12, 0x80);
    prim->Vertex(x + 12, y + 14, 0);
}

int sgSystemDrawFishing(SubGameInfo *info) {
    CScene *scene;
    mgCTexture *system_texture;
    mgCTexture *banner_texture;
    int top;
    int gauge_top;
    float reach;
    float line_length;
    int length;
    float fill;
    float bottom_y;
    float tension;
    float fish_dist;
    int hundreds;
    int tens;

    if (fgLoopMode == 0) {
        return 0;
    }

    mgTexManager.ReloadTexture(SystemTexb, (sceVif1Packet *)NULL);
    DrawFishingActionChance();
    scene = info->scene;
    system_texture = mgTexManager.GetTexture("linetens", SystemTexb);
    banner_texture = mgTexManager.GetTexture("turi_hit", SystemTexb);

    mgCDrawPrim prim;
    prim.Initialize(NULL, NULL);
    prim.AlphaBlendEnable(1);
    prim.DepthTestEnable(0);
    prim.Coord(0);
    prim.ZMask(-1);
    prim.Bilinear(1);
    prim.TextureMapEnable(0);
    if (CharaMode == 5) {
        top = mgScreenHeight - 0x47;
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x15, 0x29, 0x47, 0x60);
        prim.Vertex(0x156, top + 0x25, 0);
        prim.Vertex(0x1C4, top + 0x2A, 0);
        prim.End();
        fish_dist = GetFishDist(scene);
        reach = (FishMaxLen - fish_dist) / (FishMaxLen - FishMinLen);
        if (reach < 0.0f) {
            reach = 0.0f;
        }
        if (reach > 1.0f) {
            reach = 1.0f;
        }
        line_length = GetNowLineLength();
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x15, 0x29, 0xFF, 0x80);
        prim.Vertex(342.0f + 110.0f * reach, (float)(top + 0x25), 0.0f);
        prim.Vertex(0x1C4, top + 0x2A, 0);
        prim.End();
        prim.Bilinear(0);
        prim.TextureMapEnable(1);
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.Texture(system_texture);
        prim.TextureCrd(0, 0x34);
        prim.Vertex(0x146, top, 0);
        prim.TextureCrd(0xA8, 0x6A);
        prim.Vertex(0x1EE, top + 0x36, 0);
        prim.End();
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.Texture(system_texture);
        gauge_top = mgScreenHeight - 0x36;
        length = (int)(line_length / 2.0f);
        hundreds = length / 100;
        length %= 100;
        tens = length / 10;
        length %= 10;
        DrawNumber(&prim, hundreds, 0x18B, gauge_top);
        DrawNumber(&prim, tens, 0x194, gauge_top);
        DrawNumber(&prim, length, 0x1A3, gauge_top);
        prim.End();
        prim.Bilinear(1);
        prim.TextureMapEnable(0);
        top = mgScreenHeight - 0xD7;
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x15, 0x29, 0x47, 0x60);
        prim.Vertex(0x1D5, top + 0x14, 0);
        prim.Vertex(0x1E1, top + 0x96, 0);
        prim.End();
        tension = LineTension;
        fill = 130.0f * (1.0f - tension);
        sceVu0FVECTOR tension_end = {21.0f, 41.0f, 255.0f, 128.0f};
        sceVu0FVECTOR tension_color = {255.0f, 20.0f, 10.0f, 128.0f};
        sceVu0InterVectorXYZ(tension_color, tension_color, tension_end, tension);
        prim.Shading(1);
        prim.Begin(MG_PRIM_TRIANGLE_STRIP);
        prim.Color((int)(tension_end[0]), (int)(tension_end[1]), (int)(tension_end[2]), 0x80);
        bottom_y = (float)(top + 0x96);
        prim.Vertex(469.0f, bottom_y, 0.0f);
        prim.Color((int)(tension_end[0]), (int)(tension_end[1]), (int)(tension_end[2]), 0x80);
        prim.Vertex(481.0f, (float)(top + 0x96), 0.0f);
        prim.Color((int)(tension_color[0]), (int)(tension_color[1]), (int)(tension_color[2]),
                   0x80);
        fill = fill + (float)(top + 0x14);
        prim.Vertex(469.0f, fill, 0.0f);
        prim.Color((int)(tension_color[0]), (int)(tension_color[1]), (int)(tension_color[2]),
                   0x80);
        prim.Vertex(481.0f, fill, 0.0f);
        prim.End();
        prim.Begin(MG_PRIM_TRIANGLE_STRIP);
        prim.Color(0xFF, 0xFF, 0xFF, 0x20);
        prim.Vertex(0x1D5, top + 0x14, 0);
        prim.Color(0xFF, 0xFF, 0xFF, 0);
        prim.Vertex(0x1E1, top + 0x14, 0);
        prim.Color(0xFF, 0xFF, 0xFF, 0x20);
        prim.Vertex(0x1D5, top + 0x96, 0);
        prim.Color(0xFF, 0xFF, 0xFF, 0);
        prim.Vertex(0x1E1, top + 0x96, 0);
        prim.End();
        prim.Bilinear(0);
        prim.TextureMapEnable(1);
        prim.Begin(MG_PRIM_TRIANGLE_STRIP);
        prim.Texture(system_texture);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.TextureCrd(0, 0x34);
        prim.Vertex(0x1C3, top, 0);
        prim.TextureCrd(0, 0);
        prim.Vertex(0x1F6, top, 0);
        prim.TextureCrd(0xA8, 0x34);
        prim.Vertex(0x1C3, top + 0xA7, 0);
        prim.TextureCrd(0xA8, 0);
        prim.Vertex(0x1F6, top + 0xA7, 0);
        prim.End();
    }
    prim.TextureMapEnable(1);
    if (DrawCongra > 0) {
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.Texture(banner_texture);
        prim.TextureCrd(0, 0);
        prim.Vertex(0x80, 0xBD, 0);
        prim.TextureCrd(0x100, 0x26);
        prim.Vertex(0x180, 0xE3, 0);
        prim.End();
    }
    if (DrawHit > 0) {
        prim.Begin(MG_PRIM_SPRITE);
        prim.Color(0x80, 0x80, 0x80, 0x80);
        prim.Texture(banner_texture);
        prim.TextureCrd(0, 0x50);
        prim.Vertex(0x80, 0xB8, 0);
        prim.TextureCrd(0x100, 0x80);
        prim.Vertex(0x180, 0xE8, 0);
        prim.End();
    }
    return 1;
}

/**
 * Moves the player and checks casting and map events while holding the rod.
 */
static void CharaControl(CScene *scene, CPadControl *pad) {
    mgCCameraFollow *camera;
    CCharacter2 *chara;
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR velocity;
    sceVu0FMATRIX matrix;
    EditMoveCharaInfo idle_info;
    sceVu0FVECTOR turn_rotation;
    EditMoveCharaInfo move_info;
    float camera_angle;
    float stick_y;
    float stick_x;
    float move_x;
    float move_z;
    float target_angle;
    float turned_angle;
    float turn_delta;
    float stick_length;
    int can_cast;

    chara = scene->GetCharacter(scene->player_chara);
    if (chara != NULL) {
        camera = (mgCCameraFollow *)scene->GetCamera(scene->active_camera);
        if (camera != NULL) {
            switch (((mgCCamera *)camera)->Iam()) {
                case kCameraSettled:
                    break;
                default:
                    return;
            }
            chara->GetPosition(position);
            chara->GetRotation(rotation);
            mgCreateMatrixPY(matrix, position, rotation[1]);
            *(u_long128 *)velocity = *(u_long128 *)chara->velocity;
            if (pad == NULL) {
                memset(&idle_info.move_info, 0, sizeof(idle_info.move_info));
                memset(&idle_info, 0, sizeof(idle_info));
                velocity[1] -= 0.6f;
                EditMoveChara(scene, velocity, &idle_info);
                EditCameraControl(scene, NULL, NULL);
                return;
            }
            camera_angle = camera->GetAngle();
            stick_y = pad->Analog(5);
            stick_x = pad->Analog(4);
            move_x = stick_y * cosf(camera_angle) + stick_x * sinf(camera_angle);
            move_z = -stick_y * sinf(camera_angle) + stick_x * cosf(camera_angle);
            move_x *= 3.5f;
            move_z *= 3.5f;
            if (DebugInfo.chara_move != 0) {
                if (GamePad__2.On(1) != 0) {
                    move_x *= 3.0f;
                    move_z *= 3.0f;
                }
                if (pad->Btn(kPadButtonDebugJump) != 0) {
                    velocity[1] = 8.0f;
                }
            }
            velocity[0] = move_x;
            velocity[2] = move_z;
            velocity[1] -= 0.6f;
            if (move_x != 0.0f || move_z != 0.0f) {
                chara->GetRotation(turn_rotation);
                target_angle = atan2f(move_x, move_z);
                turned_angle = mgAngleInterpolate(turn_rotation[1], target_angle, 0.3f, 0);
                turn_delta = target_angle - turned_angle;
                if (turn_delta < 0.0f) {
                    turn_delta = -turn_delta;
                }
                if ((float)(int)(turn_delta) > 1.0f) {
                    velocity[0] *= 0.5f;
                    velocity[2] *= 0.5f;
                }
                chara->SetRotation(0.0f, turned_angle, 0.0f);
                stick_length = sqrtf(stick_y * stick_y + stick_x * stick_x);
                if (stick_length < 0.8f) {
                    chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\225\340\202\253", 0);
                    chara->SetStep(0.1f + stick_length / 0.8f);
                } else {
                    chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\221\226\202\350", 0);
                }
            } else {
                chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 0);
            }
            memset(&move_info.move_info, 0, sizeof(move_info.move_info));
            memset(&move_info, 0, sizeof(move_info));
            EditMoveChara(scene, velocity, &move_info);
            EditCameraControl(scene, pad, NULL);
            sceVu0FVECTOR cast_dir = {0.0f, 0.0f, 160.0f, 1.0f};
            sceVu0FVECTOR event_pos;
            CSceneEventData event_data;
            sceVu0ApplyMatrix(cast_dir, matrix, cast_dir);
            can_cast = move_info.move_info.landed;
            if (GetFishingMode() == 2 && LureNo < 0) {
                can_cast = 0;
            }
            if (can_cast != 0 && CheckCasting(scene, position, cast_dir) != 0) {
                ShowHelpMes(0x65, 1);
                if (pad->Btn(kPadButtonCast) != 0 && InitSelectCastingPoint(scene) != 0) {
                    SetNextMode(FISHING_CHARA_MODE_SELECT_POINT);
                }
            }
            if (GetNowSubGameInfo()->no_map_event == 0) {
                chara->GetPosition(event_pos);
                memset(&event_data, 0, sizeof(event_data));
                if (scene->GetMapEvent(event_pos, 0, &event_data) != 0) {
                    scene->RunEvent(event_data.event.point_no, &event_data);
                    ExitFishing(scene);
                }
                scene->map_event_no = 0;
            }
        }
    }
}

/**
 * Saves the camera settings and prepares the casting view.
 */
static int InitSelectCastingPoint(CScene *scene) {
    CCharacter2 *chara = scene->GetCharacter(scene->player_chara);
    mgCCamera *camera;
    sceVu0FVECTOR position;
    CameraCtrlParam *param;

    if (chara == NULL) {
        return 0;
    }
    camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL || camera->Iam() != kCameraSettled) {
        return 0;
    }
    chara->GetRotation(position);
    ((CCameraControl *)camera)->CopyParam(CameraInfo);
    param = ((CCameraControl *)camera)->GetActiveParam();
    param->max_dist = 120.0f;
    param->min_dist = 120.0f;
    param->near_height = 12.0f;
    param->far_height = 12.0f;
    if (GetCaptureMode() != 0) {
        srand(kCaptureSeed);
    }
    LoadExMotionBG(GetNowSubGameInfo(), ReadBuffer);
    return 1;
}

/**
 * Restores the camera and player after aiming a cast.
 */
static int EndSelectCastingPoint(CScene *scene) {
    mgCCamera *camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    CCharacter2 *chara;

    if (camera == NULL || camera->Iam() != kCameraSettled) {
        return 1;
    }
    CameraInfo.CopyParam(*(CCameraControl *)camera);
    if (GetNowSubGameInfo()->dungeon == 0) {
        chara = scene->GetCharacter(scene->player_chara);
        if (chara != NULL) {
            chara->DeleteExtMotion();
            chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 4);
        }
    }
    SetWaterLevel(-100000.0f);
    CastOKFlag = 0;
    return 1;
}

/**
 * Aims the cast with the stick and starts or cancels casting.
 */
static void SelectCastingPoint(CScene *scene, CPadControl *pad) {
    CCharacter2 *chara;
    CCameraControl *camera;
    sceVu0FMATRIX matrix;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR position;
    float angle_diff;
    float turn;
    float max_dist;
    float cast_distance;
    float ratio;
    float lean;
    float lean_abs;
    int saved_cancel;
    int castable;

    if (pad == NULL) {
        return;
    }
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return;
    }
    camera = (CCameraControl *)scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    switch (((mgCCamera *)camera)->Iam()) {
        case kCameraSettled:
            break;
        default:
            return;
    }
    chara->GetPosition(position);
    chara->GetRotation(rotation);
    mgCreateMatrixPY(matrix, position, rotation[1]);
    angle_diff = mgAngleLimit(rotation[1] - mgAngleLimit(camera->GetAngle() - 3.1415927f));
    turn = 0.0f;
    if (angle_diff > 0.4f) {
        turn = 0.2f * mgAngleLimit(angle_diff - 0.4f);
    }
    if (angle_diff < -0.4f) {
        turn = 0.2f * mgAngleLimit(0.4f + angle_diff);
    }
    camera->Rotate(turn);
    CastDist -= 5.0f * pad->Analog(4);
    max_dist = 240.0f * (0.5f * (1.0f + (float)RodData.status[0] / 100.0f));
    max_dist += 160.0f;
    if (CastDist < 160.0f) {
        CastDist = 160.0f;
    }
    if (CastDist > max_dist) {
        CastDist = max_dist;
    }
    cast_distance = CastDist;
    CastDistSizeRate = (cast_distance - 160.0f) / 240.0f;
    CastDistSizeRate = 0.8f + 0.4f * CastDistSizeRate;
    sceVu0FVECTOR cast_dir = {0.0f, 0.0f, 0.0f, 1.0f};
    sceVu0FVECTOR target;
    sceVu0FVECTOR follow_offset;
    sceVu0FVECTOR camera_pos;
    sceVu0FVECTOR diff;
    cast_dir[2] = cast_distance;
    sceVu0ApplyMatrix(cast_dir, matrix, cast_dir);
    camera->GetFollowOffset(follow_offset);
    sceVu0AddVector(target, cast_dir, follow_offset);
    camera->GetPos(camera_pos);
    ratio = mgDistVectorXZ(camera_pos, position);
    ratio = ratio / mgDistVectorXZ(camera_pos, cast_dir);
    sceVu0SubVector(diff, target, camera_pos);
    sceVu0ScaleVector(diff, diff, ratio);
    sceVu0AddVector(target, diff, camera_pos);
    sceVu0SubVector(target, target, follow_offset);
    target[0] = position[0];
    target[2] = position[2];
    saved_cancel = camera->rot_cancel;
    camera->BitSetRotCameraCancel(0x80);
    EditCameraControl(scene, pad, NULL);
    camera->SetRotCameraCancel(saved_cancel);
    castable = CheckCasting(scene, position, cast_dir);
    if (mgDistVectorXZ(camera_pos, position) < 30.0f) {
        castable = 0;
    }
    if (castable > 0) {
        CastOKFlag = 1;
    } else {
        CastOKFlag = 0;
    }
    *(u_long128 *)CastPoint = *(u_long128 *)cast_dir;
    *(u_long128 *)CastPointCur = *(u_long128 *)cast_dir;
    if (CastOKFlag != 0) {
        SetWaterLevel(cast_dir[1]);
        ShowHelpMes(0x66, 1);
    } else {
        SetWaterLevel(-100000.0f);
        ShowHelpMes(0x6A, 1);
        if (cast_dir[1] > position[1]) {
            cast_dir[1] = position[1];
        }
    }
    CastPoint[1] = cast_dir[1];
    CastPointCur[1] = cast_dir[1];
    lean = 0.05f * -pad->Analog(5);
    lean_abs = lean < 0.0f ? -lean : lean;
    if (lean_abs > 0.01f) {
        rotation[1] = mgAngleLimit(rotation[1] + lean);
        chara->SetRotation(rotation);
        chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\225\340\202\253", 0);
    } else {
        chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 0);
    }
    ReadBG();
    if (CastOKFlag != 0 && pad->Btn(kPadButtonCast) != 0) {
        if (InitCasting(scene) != 0) {
            SetNextMode(FISHING_CHARA_MODE_CASTING);
        }
    } else if (pad->Btn(kPadButtonCancel) != 0) {
        BreakReadBG();
        if (EndSelectCastingPoint(scene) != 0) {
            SetNextMode(FISHING_CHARA_MODE_CONTROL);
        }
    }
}

/**
 * Creates a ripple at the water surface.
 */
static void DrawHamon(float *pos, float scale) {
    if (EffectMan != NULL) {
        sceVu0FVECTOR pos_vec;
        sceVu0FVECTOR scale_vec = {0.0f, 0.0f, 0.0f, 0.0f};
        scale_vec[0] = scale;
        scale_vec[1] = scale;
        scale_vec[2] = scale;
        *(u_long128 *)pos_vec = *(u_long128 *)pos;
        pos_vec[1] = 0.5f + GetWaterLevel();
        EffectMan->CreateEffSpt("\221\253\224g\226\344", 0, 0);
        EffectMan->SetScriptVect1(pos_vec, -1, -1);
        EffectMan->SetScriptVect2(scale_vec, -1, -1);
    }
}

/**
 * Creates a splash at the water surface.
 */
static void DrawSplash(float *pos, float scale) {
    if (EffectMan != NULL) {
        sceVu0FVECTOR pos_vec;
        sceVu0FVECTOR scale_vec = {0.0f, 0.0f, 0.0f, 0.0f};
        scale_vec[0] = scale;
        scale_vec[1] = scale;
        scale_vec[2] = scale;
        *(u_long128 *)pos_vec = *(u_long128 *)pos;
        pos_vec[1] = 0.5f + GetWaterLevel();
        EffectMan->CreateEffSpt("\221\253\220\205\203p\203V\203\203", 0, 0);
        EffectMan->SetScriptVect1(pos_vec, -1, -1);
        EffectMan->SetScriptVect2(scale_vec, -1, -1);
    }
}

/**
 * Gets the duration of a character motion within the given bounds.
 */
static int GetMotionCount(CCharacter2 *chara, char *name, int min_count, int max_count, int min_speed) {
    CHRINFO_KEY_SET *list;
    float speed;
    float floor;
    int count;

    list = chara->GetKeyListPtr(name, NULL);
    if (list == NULL) {
        return min_count;
    }
    speed = list->step;
    floor = (float)min_speed;
    if (speed < floor) {
        speed = floor;
    }
    count = (int)((float)(list->end_frame - list->start_frame) / speed);
    if (count <= min_count) {
        count = min_count;
    }
    if (count >= max_count) {
        count = max_count;
    }
    return count;
}

/**
 * Installs the fishing motions and begins the casting animation.
 */
static int InitCasting(CScene *scene) {
    CCharacter2 *chara;

    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return 0;
    }
    MotionBuff.stack_used = 0;
    MotionBuff.lock = 0;
    while (LoadExMotionStep(GetNowSubGameInfo(), &MotionBuff) != 0) {
    }
    chara->SetMotion("ex\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 4);
    chara->Step();
    CastStep = 0;
    CastTime = 60;
    CastCount = 0;
    chara->SetMotion("\212\306\223\212\202\260", 2);
    CastMotionCnt = GetMotionCount(chara, "\212\306\223\212\202\260", 20, 300, 0);
    return 1;
}

/**
 * Advances the cast until the float is ready to wait for a bite.
 */
static void CastingLoop(CScene *scene, CPadControl *pad) {
    CCharacter2 *chara;
    sceVu0FVECTOR hari_pos;
    sceVu0FVECTOR hari_prev;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara != NULL) {
        EditCameraControl(scene, pad, NULL);
        GetHariPos(hari_pos, hari_prev);
        if (hari_pos[1] <= GetWaterLevel() && hari_prev[1] > GetWaterLevel()) {
            DrawHamon(hari_pos, 0.8f);
            sndSePlay(FishSnd, 4, 0);
        }
        if (CastStep == 0) {
            CastCount += 1;
            if (CastCount == 0x23) {
                sndSePlay(FishSnd, 0, 0);
                CastTime = CastingLure(CastPoint);
            }
            CastMotionCnt -= 1;
            if (CastMotionCnt <= 0) {
                CastCount = 0;
                CastStep = 1;
                chara->SetMotion("\212\306\202\275\202\347\202\265\227\247\202\277", 0);
                return;
            }
        } else if (CastStep == 1) {
            CastCount += 1;
            if (CastCount >= CastTime) {
                EndCastingLure();
                if (InitUkiWait(scene) != 0) {
                    SetNextMode(FISHING_CHARA_MODE_UKI_WAIT);
                }
            }
        }
    }
}

/**
 * Starts waiting for a fish with the float.
 */
static int InitUkiWait(CScene *scene) {
    RodActFlag = 0;
    UkiMode = kUkiStart;
    UkiModeCnt = 0;
    UkiCameraFlag = 0;
    return 1;
}

void ResetUkiCamera(CCameraControl *camera) {
    if (UkiCameraFlag != 0) {
        camera->RotBack(UkiCamOldRot);
        if (camera != NULL) {
            UkiCameraInfo.CopyParam(*camera);
        }
    }
    UkiCameraFlag = 0;
}

/**
 * Controls the rod, float and camera while waiting for a bite.
 */
static void UkiWaitLoop(CScene *scene, CPadControl *pad) {
    static int act_interval;
    static int charge_point;
    static int act_count;
    static int pull_uki_cnt;
    static int boze_cnt;
    int moved;
    int up_push;
    int right_push;
    int left_push;
    CCharacter2 *chara;
    CCameraControl *camera;
    int pushed;
    CameraCtrlParam *param;
    float stick_side;
    float stick_up;
    int up_now;
    u_char right_now;
    u_char left_now;
    u_char up_old;
    u_char right_old;
    u_char left_old;
    int reel_result;
    int reel_sound;
    int reel_held;
    int caught;
    sceVu0FVECTOR hari_pos;
    sceVu0FVECTOR hari_prev;
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR hari_now;
    sceVu0FVECTOR hari_now_prev;
    sceVu0FVECTOR uki_pos;
    sceVu0FVECTOR uki_prev;
    sceVu0FVECTOR chara_now;
    sceVu0FVECTOR camera_target[1];
    sceVu0FVECTOR uki_pos2;
    float dist;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return;
    }
    camera = (CCameraControl *)scene->GetCamera(scene->active_camera);
    if (UkiMode == kUkiReeledIn) {
        UkiModeCnt += 1;
        if (UkiModeCnt > 15) {
            EndSelectCastingPoint(scene);
            if (chara != NULL) {
                chara->SetMotion("\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 4);
            }
            SetNextMode(FISHING_CHARA_MODE_CONTROL);
        }
        return;
    }
    moved = 1;
    stick_side = pad->Analog(kFishAxisSide);
    stick_up = pad->Analog(kFishAxisUp);
    if (stick_up > 0.8f) {
        RodActFlag = 1;
        chara->SetMotion("\202\265\202\341\202\255\202\350\217\343", 6);
    } else if (stick_side > 0.8f) {
        RodActFlag = 1;
        chara->SetMotion("\202\265\202\341\202\255\202\350\211E", 6);
    } else if (stick_side < -0.8f) {
        RodActFlag = 1;
        chara->SetMotion("\202\265\202\341\202\255\202\350\215\266", 6);
    } else {
        chara->SetMotion("\212\306\202\275\202\347\202\265\227\247\202\277", 0);
        moved = 0;
    }
    right_now = stick_side > 0.8f;
    left_now = stick_side < -0.8f;
    up_old = oldPadRy > 0.8f;
    right_old = oldPadRx > 0.8f;
    left_old = oldPadRx < -0.8f;
    up_now = stick_up > 0.8f;
    up_push = up_now && !up_old;
    right_push = right_now && !right_old;
    left_push = left_now && !left_old;
    pushed = up_push || right_push || left_push;
    reel_result = 0;
    reel_held = 0;
    reel_sound = 7;
    if (pad->Btn(kFishBtnReel) != 0) {
        reel_sound = 7;
        reel_result = ExtendLine(-1.0f);
        moved = reel_held = 1;
    } else if (pad->Btn(kFishBtnReelFast) != 0) {
        reel_result = ExtendLine(-2.5f);
        moved = 1;
        reel_held = moved;
        reel_sound = 8;
    }
    if (reel_result < 0) {
        UkiMode = kUkiReeledIn;
        if (chara != NULL) {
            chara->SetMotion("ex\202\302\202\350\212\306\216\235\202\277\227\247\202\277", 0);
        }
        UkiModeCnt = 0;
        return;
    }
    if (reel_held != 0) {
        scene->loop_se.SeLoopPlayStop(FishSnd, reel_sound, 2, 13);
    }
    GetHariPos(hari_pos, hari_prev);
    chara->GetPosition(chara_pos);
    if (hari_pos[1] > GetWaterLevel() - 3.0f) {
        if (GetFishingMode() == kFishingModeFloat) {
            FishData.fish_no = -1;
        }
    }
    if (mgDistVectorXZ(chara_pos, hari_pos) < 160.0f) {
        FishData.fish_no = -1;
    }
    static int hamon_count = 0;
    caught = 0;
    GetHariPos(hari_now, hari_now_prev);
    GetUkiPos(uki_pos, uki_prev);
    chara->GetPosition(chara_now);
    dist = mgDistVectorXZ(chara_now, hari_now);
    if (dist < 160.0f) {
        dist = 160.0f;
    }
    if (dist > 400.0f) {
        dist = 400.0f;
    }
    CastDistSizeRate = (dist - 160.0f) / 240.0f;
    CastDistSizeRate = 0.8f + 0.4f * CastDistSizeRate;
    if (GetFishingMode() == kFishingModeFloat) {
        ShowHelpMes(0x67, 1);
        switch (UkiMode) {
            case kUkiStart:
                scene->ResetStatus(1, scene->player_chara, 0x20);
                UkiModeCnt = GetUkiWaitTime(&FishData, scene, hari_now, RodNo, LocalEsaNo);
                boze_cnt = 0;
                UkiMode = kUkiWaitBite;
                hamon_count = 10;
            case kUkiWaitBite:
                if (moved != 0 || boze_cnt > 0x258) {
                    UkiMode = kUkiStart;
                } else {
                    if (hamon_count <= 0) {
                        DrawHamon(uki_pos, 0.4f);
                        hamon_count = (int)(50.0f * mgRnd()) + 30;
                    }
                    hamon_count -= 1;
                    if (FishData.fish_no < 0) {
                        boze_cnt += 1;
                    } else if (UkiModeCnt <= 0) {
                        UkiMode = kUkiPoke;
                        UkiModeCnt = GetUkiPokeTime(&FishData);
                        pull_uki_cnt = 0;
                    }
                }
                break;
            case kUkiPoke:
                if (moved != 0) {
                    UkiMode = kUkiStart;
                } else {
                    if (UkiModeCnt <= 0) {
                        if (FishData.fish_no <= 0) {
                            UkiMode = kUkiStart;
                            break;
                        }
                        UkiMode = kUkiPull;
                        UkiModeCnt = GetUkiPullTime(&FishData);
                    }
                    if (pull_uki_cnt <= 0) {
                        PullUki(2.0f + 5.0f * mgRnd());
                        GamePad__2.SetVibration(1, 0x50, 10);
                        pull_uki_cnt = rand() % 10 + 5;
                        DrawHamon(uki_pos, 0.6f);
                    }
                    pull_uki_cnt -= 1;
                }
                break;
            case kUkiPull:
                if (FishData.fish_no <= 0) {
                    UkiMode = kUkiStart;
                } else {
                    scene->SetStatus(1, scene->player_chara, 0x20);
                    if (pushed != 0) {
                        caught = 1;
                    }
                    GamePad__2.SetVibration(1, 0x96, 4);
                    if (UkiModeCnt <= 0) {
                        UkiMode = kUkiStart;
                    } else {
                        PullUki(10.0f);
                    }
                }
                break;
        }
    } else {
        ShowHelpMes(0x68, 1);
        switch (UkiMode) {
            case kUkiStart:
                scene->ResetStatus(1, scene->player_chara, 0x20);
                RodActionPoint = GetUkiWaitTime(&FishData, scene, hari_now, RodNo, LocalEsaNo);
                act_count = 0;
                UkiMode = kUkiCharge;
                charge_point = 0;
                act_interval = 0;
            case kUkiCharge:
                if (act_count <= 0 && pushed != 0) {
                    RodActionPoint -= charge_point;
                    act_count = 0;
                    charge_point = 0;
                }
                if (pushed != 0 || pad->Btn(kFishBtnAction) != 0) {
                    if (act_interval < 3) {
                        RodActionPoint = RodActionPoint + (int)(3.0f * mgRnd());
                    } else {
                        charge_point += (int)(10.0f * mgRnd()) + 2;
                        RodActionPoint = RodActionPoint - (int)(3.0f * mgRnd());
                    }
                    act_interval = 0;
                    act_count = (int)(30.0f * mgRnd()) + 10;
                    DrawHamon(hari_now, 0.5f);
                    sndSePlay(FishSnd, 10, 0);
                }
                act_interval += 1;
                act_count -= 1;
                if (FishData.fish_no >= 0 && RodActionPoint < 0) {
                    UkiMode = kUkiBite;
                    UkiModeCnt = 0x23;
                    DrawHamon(hari_now, 0.8f);
                    DrawSplash(hari_now, 0.5f);
                    sndSePlay(FishSnd, 11, 0);
                }
                break;
            case kUkiBite:
                if (UkiModeCnt < 0x20 && pushed != 0) {
                    caught = 1;
                }
                scene->SetStatus(1, scene->player_chara, 0x20);
                GamePad__2.SetVibration(1, 0x96, 4);
                if (UkiModeCnt <= 0) {
                    UkiMode = kUkiStart;
                } else {
                    PullUki(10.0f);
                }
                break;
        }
    }
    UkiModeCnt -= 1;
    if (UkiModeCnt < 0) {
        UkiModeCnt = 0;
    }
    camera->SetRotCameraCancel(0x80);
    if (caught == 0 && GetFishingMode() == kFishingModeFloat) {
        GetUkiPos(camera_target[0], uki_pos2);
        if (camera_target[0][1] > 1.0f + GetWaterLevel()) {
            ResetUkiCamera(camera);
        }
        if (moved != 0) {
            ResetUkiCamera(camera);
        }
        if (UkiCameraFlag != 0) {
            camera_target[0][1] = GetWaterLevel() - 20.0f;
            EditCameraControl(scene, NULL, camera_target);
        } else {
            EditCameraControl(scene, pad, NULL);
        }
    } else {
        EditCameraControl(scene, pad, NULL);
    }
    if (GetFishingMode() == kFishingModeFloat) {
        if (pad->Btn(kFishBtnCameraToggle) != 0) {
            if (UkiCameraFlag == 0) {
                UkiCamOldRot = camera->GetAngle();
                UkiCameraFlag = 1;
                camera->CopyParam(UkiCameraInfo);
                param = camera->GetActiveParam();
                param->SetFixHeight(100.0f);
                param->SetFixDist(80.0f);
                param->no_check = 1;
            } else {
                ResetUkiCamera(camera);
            }
        }
    }
    camera->SetRotCameraCancel(0);
    if (caught != 0) {
        ResetUkiCamera(camera);
        scene->ResetStatus(1, scene->player_chara, 0x20);
        if (InitBattle(scene) != 0) {
            GamePad__2.SetVibration(0, 1, 5);
            SetNextMode(FISHING_CHARA_MODE_BATTLE);
            sndSePlay(FishSnd, 0x12, 0);
            if (MardanEventMap != 0 && MardanEventPlace != 0) {
                RunEventNo = 0x1F9;
                scene->fade.FadeOut(0x14, 0.0f, 0.0f, 0.0f);
            }
            LoadFishFlag = FishLoadBG(&FishData, (u_long128 *)ReadBuffer);
        }
    }
}

/**
 * Prepares the camera and line gauges for fighting a hooked fish.
 */
static int InitBattle(CScene *scene) {
    CCharacter2 *chara;
    mgCCamera *camera;
    CameraCtrlParam *param;
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR chara_pos;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return 0;
    }
    camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL || camera->Iam() != kFishCameraState) {
        return 0;
    }
    chara->GetPosition(chara_pos);
    chara->GetRotation(chara_rot);
    param = ((CCameraControl *)camera)->GetActiveParam();
    param->max_dist = 80.0f;
    param->min_dist = 80.0f;
    param->near_height = 12.0f;
    param->far_height = 12.0f;
    ((CCameraControl *)camera)->RotBack(mgAngleLimit(3.1415927f + chara_rot[1] - 0.2f));
    camera->Step(-1);
    InitFishBattle();
    LineTension = 0;
    addLineTension = 0;
    MinLineTension = 0;
    LineMaxLen = GetNowLineLength();
    LineMinLen = GetMinLineLength();
    FishMaxLen = GetFishDist(scene);
    FishMinLen = 90.0f;
    DrawHit = 60;
    RodStatus = 0;
    RodStatusCnt = 0;
    ActionCount = 0;
    ActionDecCount = 0;
    WindReel = 0;
    BattleBgmCnt = 0;
    scene->StopBGM(0);
    BattleCount = 0;
    return 1;
}

/**
 * Controls the rod and reel during the fight with a hooked fish.
 */
static void BattleLoop(CScene *scene, CPadControl *pad) {
    int pushed;
    int released;
    CCharacter2 *chara;
    float stick_side;
    float stick_up;
    u_char up_now;
    u_char right_now;
    u_char left_now;
    u_char up_old;
    u_char right_old;
    u_char left_old;
    int rod_dir;
    int rod_chance;
    int rod_sound;
    int reel_result;
    int finished;
    sceVu0FVECTOR hari_pos;
    sceVu0FVECTOR hari_prev;
    CCPoly polys[0x400];
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return;
    }
    EditCameraControl(scene, pad, NULL);
    ReadBG();
    ShowHelpMes(0x69, 1);
    if (RunEventNo > 0 && scene->fade.FadeCheck() != 0) {
        scene->StopBGM(0);
        scene->InitBGM();
        scene->RunEvent(RunEventNo, NULL);
        EsaInit();
        EsaChara = 0;
        GetSaveData()->user_data.DeleteBait();
        EndSelectCastingPoint(scene);
        ExitFishing(scene);
        return;
    }
    if (BattleBgmCnt == 20) {
        scene->PlayBGM(1, -1, 1.0f);
    }
    BattleBgmCnt += 1;
    stick_side = pad->Analog(kFishAxisSide);
    stick_up = pad->Analog(kFishAxisUp);
    up_now = stick_up > 0.8f;
    right_now = stick_side > 0.8f;
    left_now = stick_side < -0.8f;
    up_old = oldPadRy > 0.8f;
    right_old = oldPadRx > 0.8f;
    left_old = oldPadRx < -0.8f;
    pushed = (up_now && !up_old) || (right_now && !right_old) || (left_now && !left_old);
    released = (!up_now && up_old) || (!right_now && right_old) || (!left_now && left_old);
    rod_dir = 0;
    if (right_now) {
        rod_dir = 1;
    }
    if (left_now) {
        rod_dir = -1;
    }
    rod_chance = CheckRodActionChance(rod_dir, &rod_sound);
    if (rod_sound != 0) {
        sndSePlay(FishSnd, 0x11, 0);
    }
    static int snd_cnt = 0;
    if (pushed) {
        GetHariPos(hari_pos, hari_prev);
        sndSePlay(FishSnd, 0xB, 0);
        DrawSplash(hari_pos, 0.5f);
        if (snd_cnt == 0) {
            sndSePlay(FishSnd, 9, 0);
            snd_cnt = 10;
        }
    }
    snd_cnt -= 1;
    if (snd_cnt < 0) {
        snd_cnt = 0;
    }
    switch (RodStatus) {
        case 0:
            if (pushed) {
                RodStatus = 1;
                RodStatusCnt = 20;
            } else if (released) {
                RodStatus = 2;
                RodStatusCnt = 20;
            }
            break;
        case 1:
            if (RodStatusCnt == 0) {
                RodStatus = 0;
            } else if (released) {
                RodStatus = 2;
            }
            break;
        case 2:
            if (RodStatusCnt == 0) {
                RodStatus = 0;
            } else if (pushed) {
                RodStatus = 1;
            }
            break;
    }
    RodStatusCnt -= 1;
    BattleCount += 1;
    if (stick_up > 0.8f) {
        chara->SetMotion("\203q\203b\203g\216\236\212\306\217\343", 0);
    } else if (stick_side > 0.8f) {
        chara->SetMotion("\203q\203b\203g\216\236\212\306\211E", 0);
    } else if (stick_side < -0.8f) {
        chara->SetMotion("\203q\203b\203g\216\236\212\306\215\266", 0);
    } else {
        chara->SetMotion("\203q\203b\203g\216\236\227\247\202\277", 0);
    }
    reel_result = 0;
    if (pad->Btn(kFishBtnReel) != 0) {
        scene->loop_se.SeLoopPlayStop(FishSnd, 7, 2, 100);
        reel_result = ExtendLine(-1.0f);
        WindReel = 1;
    } else {
        WindReel = 0;
    }
    if (pushed || pad->Btn(kFishBtnAction) != 0) {
        ActionCount += 1;
        ActionDecCount = 10;
    }
    ActionDecCount -= 1;
    if (ActionDecCount <= 0) {
        ActionCount -= 1;
        if (ActionCount < 0) {
            ActionCount = 0;
        }
    }
    FishBattle(scene, polys, 0x400);
    LineTensionStep(&FishData, rod_chance);
    finished = 0;
    if (GetFishDist(scene) < FishMinLen || reel_result < 0) {
        finished = 1;
    }
    if (BattleCount < 100) {
        finished = 0;
    }
    if (MardanEventMap != 0 && MardanEventPlace != 0) {
        finished = 0;
    }
    if (LineTension >= 1.0f) {
        if (InitFalse(scene) != 0) {
            EndFishBattle();
            SetNextMode(FISHING_CHARA_MODE_FALSE);
            sndSePlay(FishSnd, 0x13, 0);
            scene->StopBGM(0);
        }
    } else if (finished != 0) {
        if (LoadFishFlag == 0) {
            if (InitFalse(scene) != 0) {
                EndFishBattle();
                SetNextMode(FISHING_CHARA_MODE_FALSE);
            }
        } else if (InitSuccess(scene) != 0) {
            EndFishBattle();
            SetNextMode(FISHING_CHARA_MODE_SUCCESS);
            sndSePlay(FishSnd, 0x14, 0);
            scene->StopBGM(0);
        }
    }
}

/**
 * Gets the ground-plane distance from the player to the hooked fish.
 */
static float GetFishDist(CScene *scene) {
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR fish_pos;
    sceVu0FVECTOR fish_velo;
    sceVu0FMATRIX matrix;
    CCharacter2 *chara = scene->GetCharacter(scene->player_chara);
    chara->GetPosition(chara_pos);
    chara->GetRotation(chara_rot);
    mgUnitMatrix(matrix);
    sceVu0RotMatrixY(matrix, matrix, chara_rot[1]);
    GetFishPosVelo(fish_pos, fish_velo);
    return mgDistVectorXZ(chara_pos, fish_pos);
}

/**
 * Removes the equipped bait after clearing its model.
 */
static void DeleteEsa(void) {
    CSaveData *saved = GetSaveData();
    EsaInit();
    EsaChara = 0;
    saved->user_data.DeleteBait();
}

/**
 * Starts the failed-catch animation and checks whether the bait is lost.
 */
static int InitFalse(CScene *scene) {
    CCharacter2 *chara;
    mgCCamera *camera;
    float roll;
    float chance;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return 0;
    }
    camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL || camera->Iam() != kFishCameraState) {
        return 0;
    }
    ExtendLine(-1000.0f);
    ExtendLine(20.0f);
    ResetLineVelo();
    FalseStep = 0;
    chara->SetMotion("\222\336\202\352\202\310\202\242\202\252\202\301\202\251\202\350", 6);
    FalseMotionCount = GetMotionCount(chara, "\222\336\202\352\202\310\202\242\202\252\202\301\202\251\202\350", 20, 300, 0);
    roll = mgRnd();
    chance = 0.3f;
    chance *= 1.0f - 0.5f * RodData.status4_rate;
    if (GetFishingMode() == kFishingModeLure) {
        chance *= 0.5f;
    }
    if (roll < chance) {
        DeleteEsa();
    }
    return 1;
}

/**
 * Advances the failed-catch display until the player returns to aiming.
 */
static void FalseLoop(CScene *scene, CPadControl *pad) {
    CCharacter2 *chara;
    mgCCamera *camera;
    CameraCtrlParam *param;
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR ref_pos;
    sceVu0FVECTOR cam_pos;
    if (pad == NULL) {
        return;
    }
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return;
    }
    camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    switch (camera->Iam()) {
        case kFishCameraState:
            break;
        default:
            return;
    }
    chara->GetPosition(chara_pos);
    chara->GetPosition(ref_pos);
    chara->GetRotation(chara_rot);
    param = ((CCameraControl *)camera)->GetActiveParam();
    param->max_dist = 80.0f;
    param->min_dist = 80.0f;
    param->near_height = -5.0f;
    param->far_height = -5.0f;
    param->min_height = -5.0f;
    ref_pos[1] += 15.0f;
    ((CCameraControl *)camera)->GetPos(cam_pos);
    cam_pos[1] = 5.0f + ref_pos[1];
    ((CCameraControl *)camera)->SetRef(ref_pos);
    ((CCameraControl *)camera)->SetPos(cam_pos);
    ((CCameraControl *)camera)->SetRotate(mgAngleLimit(chara_rot[1] - 0.2f));
    camera->Step(-1);
    FalseMotionCount -= 1;
    if (FalseStep == 0 && (FalseMotionCount <= 0 || chara->CheckMotionEnd() != 0)) {
        chara->SetMotion("\222\336\202\352\202\310\202\242\202\252\202\301\202\251\202\350L", 4);
        FalseStep = 1;
        FalseMotionCount = 40;
    }
    if (FalseStep > 0 && FalseMotionCount <= 0 && pad->Btn(kFishBtnAction) != 0) {
        ExtendLine(-1000.0f);
        ResetLineVelo();
        EndSelectCastingPoint(scene);
        SetNextMode(FISHING_CHARA_MODE_CONTROL);
        ((CCameraControl *)camera)->RotBack(mgAngleLimit(3.1415927f + chara_rot[1]));
        scene->PlayBGM(0, -1, 1.0f);
    }
}

/**
 * Loads the landed fish and awards its item, record and fishing points.
 */
#ifdef NONMATCHING
static int InitSuccess(CScene *scene) {
    mgCTextureManager *tex_manager;
    CCharacter2 *chara;
    CCharacter2 *fish_chara;
    ClsMes *message;
    CSaveData *save_data;
    FISH_PARAM *fish_param;
    int fish_item_no;
    float fish_size;
    float fish_weight;
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return 0;
    }
    mgCCamera *camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL || camera->Iam() != kFishCameraState) {
        return 0;
    }
    tex_manager = &mgTexManager;
    FishFontH = 0;
    if (LoadFishFlag != 0) {
        while (ReadBGSync() != 0) {
        }
        int size = FreeSize(&MotionBuff);
        FishStack.stSetBuffer((u_long128 *)FreeTop(&MotionBuff), size);
        FishStack.stack_used = 0;
        FishStack.lock = 0;

        fish_chara = new (FishStack.Alloc(0x68)) CCharacter2;
        FishChara = fish_chara;
        fish_chara->Initialize();
        tex_manager->DeleteBlock(FishTexb);
        FishChara->LoadPack((u_int *)ReadBuffer, "info.cfg", &FishStack, &FishStack, &FishStack,
                            FishTexb, NULL);
        FishChara->SetScale(FishData.width_scale, FishData.width_scale, FishData.length_scale);
        FishChara->SetMotion("\222\336\202\352\202\275\216\236", 0);
    }
    ExtendLine(-1000.0f);
    ExtendLine(20.0f);
    ResetLineVelo();
    FalseStep = 0;
    chara->SetMotion("\222\336\202\350\217\343\202\260\212\354\202\321", 6);
    FalseMotionCount = GetMotionCount(chara, "\222\336\202\350\217\343\202\260\212\354\202\321", 20, 300, 0);
    save_data = GetSaveData();
    if (GetFishingMode() == kFishingModeFloat) {
        DeleteEsa();
    }
    GetItemRet = 0;
    message = scene->GetMessage(1);
    fish_param = GetFishParam(FishData.fish_no);
    message->values[0] = 0;
    message->value_width[0] = 0;
    fish_item_no = -1;
    message->values[1] = 0;
    message->value_width[1] = 0;
    if (fish_param != NULL) {
        fish_size = FishData.size / 100.0f;
        fish_item_no = fish_param->item_no;
        fish_weight = FishData.weight;
        GetItemRet = save_data->user_data.GetFishInAquarium(fish_item_no, fish_size, fish_weight);
        save_data->user_data.AddFp(FishData.fishing_point);
        save_data->user_data.CheckFishRecordUpdate(fish_item_no, fish_size, fish_weight);
    }
    DrawCongra = 100;
    SetShowHari(0);
    if (fish_item_no >= 0) {
        message->Preset(4);
        message->SetWindowMode(4);
        message->item_mes[0] = GetItemMessageNo(fish_param->item_no, 1);
        message->values[0] = (int)(FishData.size);
        message->value_width[0] = 0;
        message->values[1] = FishData.fishing_point;
        message->value_width[1] = 0;
        if (LanguageCode > 0 && LanguageCode < 6) {
            message->value_half = 1;
        }
        FishFontH = message->font_h;
        message->font_h += 4;
        message->MakeMesWin(13);
        message->fukidashi_pos = 8;
        message->abs_win.height = message->font_h * 4 - message->font_h / 2 + 4;
    }
    sndSePlay(FanSnd, 0, 0);
    return 1;
}

#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", InitSuccess__FP6CScene);
#endif

/**
 * Advances the landed-fish messages and returns to aiming.
 */
static void SuccessLoop(CScene *scene, CPadControl *pad) {
    ClsMes *message;
    CCharacter2 *chara;
    mgCCamera *camera;
    CameraCtrlParam *param;
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR ref_pos;
    sceVu0FVECTOR cam_pos;
    if (pad == NULL) {
        return;
    }
    chara = scene->GetCharacter(scene->player_chara);
    if (chara == NULL) {
        return;
    }
    camera = (mgCCamera *)scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    switch (camera->Iam()) {
        case kFishCameraState:
            break;
        default:
            return;
    }
    message = scene->GetMessage(1);
    chara->GetPosition(chara_pos);
    chara->GetPosition(ref_pos);
    chara->GetRotation(chara_rot);
    param = ((CCameraControl *)camera)->GetActiveParam();
    param->max_dist = 80.0f;
    param->min_dist = 80.0f;
    param->near_height = -5.0f;
    param->far_height = -5.0f;
    param->min_height = -5.0f;
    ref_pos[1] += 15.0f;
    ((CCameraControl *)camera)->GetPos(cam_pos);
    cam_pos[1] = 5.0f + ref_pos[1];
    ((CCameraControl *)camera)->SetRef(ref_pos);
    ((CCameraControl *)camera)->SetPos(cam_pos);
    ((CCameraControl *)camera)->SetRotate(mgAngleLimit(chara_rot[1] - 0.2f));
    camera->Step(-1);
    FalseMotionCount -= 1;
    if (FalseStep == 0 && (FalseMotionCount <= 0 || chara->CheckMotionEnd() != 0)) {
        chara->SetMotion("\222\336\202\350\217\343\202\260\212\354\202\321L", 4);
        FalseStep2 = 0;
        FalseStep = 1;
        FalseMotionCount = 60;
    }
    if (FalseMotionCount <= 0) {
        FalseMotionCount = 0;
    }
    static int font_h = 0;
    if (FalseStep == 1) {
        if (FalseMotionCount <= 0 && pad->Btn(kFishBtnAction) != 0) {
            if (message->select < 0) {
                message->cursor_time = 0;
            }
            message->select = -1;
            message->draw_speed = message->GetDrawSpeedDef();
            message->mes_no = -1;
            message->unk_1e40 = 0;
            message->open = 0;
            message->fade = 0.0f;
            message->fukidashi_centre_x = -1;
            message->fukidashi_centre_y = -1;
            message->fukidashi_pos = 0;
            if (FishFontH > 0) {
                message->font_h = FishFontH;
            }
            message->abs_win.height = -1;
            sndSePlay(GetSystemSndID(), 0x19, 0);
            FalseStep = 3;
            if (GetSaveData()->GetBitFlag(0x3C) == 0) {
                FalseStep2 = 0;
                FalseStep = 2;
                FishMesNo = 0x7D1;
            }
            if (GetNowSubGameInfo()->record_check != 0 &&
                CheckFishingRecord(FishData.size / 100.0f) != 0) {
                FalseStep2 = 0;
                FalseStep = 2;
                FishMesNo = 0x7D2;
            }
            return;
        }
    }
    if (FalseStep == 2) {
        ClsMes *result_message = scene->GetMessage(1);
        int state = result_message->State();
        int confirm = pad->Btn(kFishBtnAction);
        switch (FalseStep2) {
            case 0:
                GetSaveData()->SetBitFlag(0x3C, 1);
                result_message->Preset(4);
                result_message->SetWindowMode(4);
                result_message->MakeMesWin(FishMesNo);
                result_message->fukidashi_pos = 8;
                FalseStep2 += 1;
                break;
            case 1:
                switch (state) {
                    case 5:
                        if (confirm != 0) {
                            result_message->GoNextPage();
                            sndSePlay(GetSystemSndID(), 0x19, 0);
                            if (FishMesNo == 0x7D2) {
                                sndSePlay(GetSystemSndID(), 0x12, 0);
                            }
                        }
                        break;
                    case 3:
                        if (confirm != 0) {
                            FalseStep2 += 1;
                            sndSePlay(GetSystemSndID(), 0x19, 0);
                        }
                        break;
                    case 0:
                        FalseStep2 += 1;
                        break;
                }
                break;
            case 2:
                if (result_message->select < 0) {
                    result_message->cursor_time = 0;
                }
                result_message->select = -1;
                result_message->draw_speed = result_message->GetDrawSpeedDef();
                result_message->mes_no = -1;
                result_message->unk_1e40 = 0;
                result_message->open = 0;
                result_message->fade = 0.0f;
                result_message->fukidashi_centre_x = -1;
                result_message->fukidashi_centre_y = -1;
                result_message->fukidashi_pos = 0;
                FalseStep = 3;
                break;
        }
    }
    if (FalseStep >= 3) {
        ExtendLine(-1000.0f);
        ResetLineVelo();
        EndSelectCastingPoint(scene);
        SetNextMode(FISHING_CHARA_MODE_CONTROL);
        FishChara = NULL;
        mgTexManager.DeleteBlock(FishTexb);
        ((CCameraControl *)camera)->RotBack(mgAngleLimit(3.1415927f + chara_rot[1]));
        SetShowHari(1);
        scene->PlayBGM(0, -1, 1.0f);
        if (GetItemRet != 0) {
            sgGetItemOverFlagOn();
        }
    }
}

/**
 * Checks whether the nearest surface below the position is fishing water.
 */
static int CheckFishing(float *pos, CCPoly *polys, int count) {
    sceVu0FVECTOR end;
    int hit_index[32];
    sceVu0FVECTOR hit_point[32];
    int hits;
    int i;
    *(u_long128 *)end = *(u_long128 *)pos;
    end[1] -= 140.0f;
    hits = CheckHits(polys, count, pos, end, 32, hit_index, hit_point, 1, 0);
    i = 0;
    if (hits <= 0) {
        return 0;
    }
    for (; i < 1; i++) {
        int index = hit_index[i];
        float height = hit_point[i][1];
        pos[1] = height;
        if (polys[index].area_kind == 7) {
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", CheckCasting__FP6CScenePfPf);

/**
 * Chooses a normally distributed value with a randomized lower bound.
 */
static float GetRandamNumber(float center, float high, float floor) {
    float value = mgNRnd();
    value = center + value * ((high - center) / 3.0f);
    if (value < floor) {
        value = floor + (center - floor) * mgRnd();
    }
    return value;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", GetUkiWaitTime__FP9FISH_DATAP6CScenePfii);

/**
 * Chooses the duration of a fish nibbling according to its bait affinity.
 */
static int GetUkiPokeTime(FISH_DATA *fish) {
    switch (FavoredEsa) {
        case 0:
            fish->fish_no = -1;
            return 5;
        case 1:
            return (int)(60.0f * mgRnd()) + 100;
        case 2:
            return (int)(50.0f * mgRnd()) + 50;
        case 3:
            return (int)(30.0f * mgRnd()) + 30;
    }
    return 0;
}

/**
 * Gets the duration of a fish pulling the float.
 *
 * @mangled GetUkiPullTime__FP9FISH_DATA
 * @address 0x3083E0
 * @size 0x10
 */
static s32 GetUkiPullTime(FISH_DATA *fish) {
    return 30;
}

/**
 * Starts loading the hooked fish's model.
 */
static int FishLoadBG(FISH_DATA *fish, u_long128 *buffer) {
    char path[0x40];
    if (fish->fish_no < 0) {
        return 0;
    }
    StartReadBG();
    FISH_PARAM *param = GetFishParam(fish->fish_no);
    if (param == NULL) {
        return 0;
    }
    sprintf(path, "sg/fish/%sa.chr", param->file_name);
    return LoadFileBG(path, buffer, 0) != 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", LineTensionStep__FP9FISH_DATAi);

int GetAppearFish(int map_no, float *position, FISH_PLACE *place, int place_num) {
    FISH_PLACE_MAP *map;
    int i;
    int count;
    map = FishPlaceMap;
    for (i = 0; i < place_num; i++) {
        place[i].fish_no = -1;
        place[i].wait_bias = 0;
        place[i].rate = 0;
    }
    for (i = 0; i < FishPlaceMapNum; i++, map++) {
        if (map_no == map->map_no && map->CheckFishPlace(position) != 0) {
            map->SetFishPlace(place, place_num, map->exclusive);
            if (map->exclusive != 0) {
                break;
            }
        }
    }
    for (count = 0; count < place_num; count++) {
        if (place[count].fish_no < 0) {
            break;
        }
    }
    if (map_no >= 0 && count <= 0) {
        return GetAppearFish(-1, position, place, place_num);
    }
    return count;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/fishing", SetFishPlace__14FISH_PLACE_MAPFP10FISH_PLACEii);

int FISH_PLACE_MAP::CheckFishPlace(float *position) {
    sceVu0FVECTOR center;
    mgZeroVector(center);
    switch (area_type) {
        case FISH_PLACE_AREA_CIRCLE:
            center[0] = area_param[0];
            center[2] = area_param[1];
            float dist = mgDistVectorXZ(center, position);
            if (dist > area_param[2]) {
                return 0;
            }
            return 1;
    }
    return 1;
}

/**
 * Allocates the fish appearance map table.
 */
static int fpFISH_MAP_NUM(SPI_STACK *args, int arg_count) {
    FishPlaceMapNum = spiGetStackInt(args);
    u_int blocks;
    if (((u_int)FishPlaceMapNum * sizeof(FISH_PLACE_MAP)) & 0xF) {
        blocks = (((u_int)FishPlaceMapNum * sizeof(FISH_PLACE_MAP)) >> 4) + 1;
    } else {
        blocks = ((u_int)FishPlaceMapNum * sizeof(FISH_PLACE_MAP)) >> 4;
    }
    u_long128 *block = fpStack->Alloc(blocks + 2);
    FishPlaceMap = new (block) FISH_PLACE_MAP[FishPlaceMapNum];
    fpNowFishPlaceMapNum = 0;
    fpNowFishPlaceMap = FishPlaceMap;
    return 1;
}

/**
 * Starts a fish appearance map entry.
 */
static int fpFISH_MAP(SPI_STACK *args, int arg_count) {
    fpNowFishPlaceMap = NULL;
    if ((int)fpNowFishPlaceMapNum >= FishPlaceMapNum) {
        return 0;
    }
    fpNowFishPlaceMap = &FishPlaceMap[fpNowFishPlaceMapNum];
    memset(fpNowFishPlaceMap, 0, sizeof(FISH_PLACE_MAP));
    fpNowFishPlaceMap->map_no = spiGetStackInt(args++);
    if (arg_count >= 2) {
        fpNowFishPlaceMap->exclusive = spiGetStackInt(args);
    }
    return 1;
}

/**
 * Sets the area and name of the current fishing place.
 */
static int fpFISH_PLACE(SPI_STACK *args, int arg_count) {
    int area_type;
    char *name;
    int i;
    if (fpNowFishPlaceMap == 0) {
        return 0;
    }
    area_type = spiGetStackInt(args++);
    if (area_type < 0 || area_type >= kFishShapeCount) {
        area_type = 0;
    }
    fpNowFishPlaceMap->area_type = area_type;
    name = spiGetStackString(args++);

    if (name != NULL && *name != 0) {
        fpNowFishPlaceMap->name = mgCopyString(name, fpStack);
    }
    for (i = 0; i < kFishPlaceValueCount; i++) {
        fpNowFishPlaceMap->area_param[i] = spiGetStackFloat(args++);
    }
    return 1;
}

/**
 * Adds a fish to the current fishing place.
 */
static int fpFISH(SPI_STACK *args, int arg_count) {
    FISH_PLACE *entry;
    int index;
    int *count_ptr;
    if (fpNowFishPlaceMap == 0) {
        return 0;
    }
    count_ptr = &fpNowFishPlaceMap->fish_num;
    index = *count_ptr;
    if (index >= kFishPlaceMaxFish) {
        return 0;
    }
    *count_ptr = index + 1;
    entry = &fpNowFishPlaceMap->fish[index];
    entry->fish_no = -1;
    entry->wait_bias = 0;
    entry->rate = 0;
    entry->fish_no = spiGetStackInt(args++);
    entry->rate = spiGetStackFloat(args++);
    entry->wait_bias = spiGetStackFloat(args);
    return 1;
}

/**
 * Advances to the next fish appearance map entry.
 */
static int fpFISH_MAP_END(SPI_STACK *args, int arg_count) {
    if (fpNowFishPlaceMap == 0) {
        return 0;
    }
    fpNowFishPlaceMapNum += 1;
    return 1;
}

void LoadFishPlaceData(char *script, int size, mgCMemory *memory) {
    fpStack = memory;
    FishPlaceMapNum = 0;
    FishPlaceMap = NULL;
    CScriptInterpreter interpreter;
    interpreter.SetTag(tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
}

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", lure_file__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", EsaInfo__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", FishParam__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_993__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1430__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1490__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1491__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1536__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1631__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", tag__8__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_832__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_833__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_834__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_835__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_845__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_846__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_847__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_848__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_849__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_850__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_851__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_852__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_853__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_854__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_855__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_856__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_857__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_858__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_859__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_860__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_861__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_862__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_863__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_864__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_865__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_866__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_867__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_868__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_869__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_870__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_871__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_872__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_873__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_874__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_875__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_876__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_877__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_878__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_879__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_880__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_881__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_882__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_917__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_932__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_979__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_980__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1058__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1304__8__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1305__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1306__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1307__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1308__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1309__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1310__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1311__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1312__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1313__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1314__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1315__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1316__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1397__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1399__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1398__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1424__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1442__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1443__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1508__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1509__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1576__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1577__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1683__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1691__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1722__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1723__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1749__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1920__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1921__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1922__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2057__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2058__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2059__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2060__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2099__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2127__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2197__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2198__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2272__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2461__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2670__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2671__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2672__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2673__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_2674__2__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/fishing", at_1444__3__DATA);

// Small uninitialised data (.sbss)

static INCLUDE_BSS(FishSnd, 0x4);
static INCLUDE_BSS(FanSnd, 0x4);
static INCLUDE_BSS(UkiRod, 0x4);
static INCLUDE_BSS(LureRod, 0x4);
static INCLUDE_BSS(Uki, 0x4);
static INCLUDE_BSS(Lure, 0x4);
static INCLUDE_BSS(Hari, 0x4);
static INCLUDE_BSS(FishingTexb, 0x4);
static INCLUDE_BSS(FishTexb, 0x4);
static INCLUDE_BSS(SystemTexb, 0x4);
static INCLUDE_BSS(EsaTexb, 0x4);
static INCLUDE_BSS(MainChara, 0x4);
static INCLUDE_BSS(FishChara, 0x4);
static INCLUDE_BSS(EsaChara, 0x4);
static INCLUDE_BSS(CursorChara, 0x8);
static INCLUDE_BSS(RodHand, 0x4);
static INCLUDE_BSS(UkiFrame, 0x4);
static INCLUDE_BSS(LureFrame, 0x4);
static INCLUDE_BSS(HariFrame, 0x4);
static INCLUDE_BSS(oldPadRx, 0x4);
static INCLUDE_BSS(oldPadRy, 0x4);
static INCLUDE_BSS(CastOKFlag, 0x4);
static INCLUDE_BSS(RunEventNo, 0x4);
static INCLUDE_BSS(MardanEventMap, 0x4);
static INCLUDE_BSS(MardanEventPlace, 0x4);
static INCLUDE_BSS(CharaMode, 0x4);
static INCLUDE_BSS(NextCharaMode, 0x4);
static INCLUDE_BSS(fgLoopMode, 0x4);
static INCLUDE_BSS(fgLoopStep, 0x4);
static INCLUDE_BSS(fgLoopCnt, 0x4);
static INCLUDE_BSS(RodNo, 0x4);
static INCLUDE_BSS(EsaNo, 0x4);
static INCLUDE_BSS(LocalEsaNo, 0x4);
static INCLUDE_BSS(FavoredEsa, 0x4);
static INCLUDE_BSS(LureNo, 0x4);
static INCLUDE_BSS(CastDist, 0x4);
static INCLUDE_BSS(CastDistSizeRate, 0x4);
static INCLUDE_BSS(UkiMode, 0x4);
static INCLUDE_BSS(UkiModeCnt, 0x4);
static INCLUDE_BSS(RodActionPoint, 0x4);
static INCLUDE_BSS(ReadBuffer, 0x4);
static INCLUDE_BSS(LoadFishFlag, 0x4);
static INCLUDE_BSS(LoadExMotionFlag, 0x4);
static INCLUDE_BSS(LineTension, 0x4);
static INCLUDE_BSS(addLineTension, 0x4);
static INCLUDE_BSS(MinLineTension, 0x4);
static INCLUDE_BSS(LineMaxLen, 0x4);
static INCLUDE_BSS(LineMinLen, 0x4);
static INCLUDE_BSS(FishMaxLen, 0x4);
static INCLUDE_BSS(FishMinLen, 0x4);
static INCLUDE_BSS(BattleCount, 0x4);
static INCLUDE_BSS(WindReel, 0x4);
static INCLUDE_BSS(RodStatus, 0x4);
static INCLUDE_BSS(RodStatusCnt, 0x4);
static INCLUDE_BSS(ActionCount, 0x4);
static INCLUDE_BSS(ActionDecCount, 0x4);
static INCLUDE_BSS(RetCode, 0x4);
static INCLUDE_BSS(DrawHit, 0x4);
static INCLUDE_BSS(DrawCongra, 0x4);
static INCLUDE_BSS(BgmReadFlag, 0x4);
static INCLUDE_BSS(BattleBgmCnt, 0x4);
static INCLUDE_BSS(ex_mtn_buff, 0x4);
INCLUDE_BSS(stack_size, 0x4);
static INCLUDE_BSS(ThreadStack__2, 0x4);
static INCLUDE_BSS(TheadID__2, 0x4);
static INCLUDE_BSS(ThreadRunning, 0x4);
static INCLUDE_BSS(step_end_flag, 0x4);
static INCLUDE_BSS(CastStep, 0x4);
static INCLUDE_BSS(CastCount, 0x4);
static INCLUDE_BSS(CastTime, 0x4);
static INCLUDE_BSS(CastMotionCnt, 0x4);
static INCLUDE_BSS(RodActFlag, 0x4);
static INCLUDE_BSS(UkiCameraFlag, 0x4);
static INCLUDE_BSS(UkiCamOldRot, 0x4);
static INCLUDE_BSS(hamon_count_1798, 0x4);
static INCLUDE_BSS(init_1799, 0x4);
static INCLUDE_BSS(boze_cnt_1801, 0x4);
static INCLUDE_BSS(pull_uki_cnt_1808, 0x4);
static INCLUDE_BSS(act_count_1838, 0x4);
static INCLUDE_BSS(charge_point_1839, 0x4);
static INCLUDE_BSS(act_interval_1840, 0x4);
static INCLUDE_BSS(snd_cnt_1960, 0x4);
static INCLUDE_BSS(init_1961, 0x4);
static INCLUDE_BSS(FalseStep, 0x4);
static INCLUDE_BSS(FalseStep2, 0x4);
static INCLUDE_BSS(FalseMotionCount, 0x4);
static INCLUDE_BSS(GetItemRet, 0x4);
static INCLUDE_BSS(FishMesNo, 0x4);
static INCLUDE_BSS(FishFontH, 0x4);
static INCLUDE_BSS(font_h_2216, 0x4);
static INCLUDE_BSS(init_2217, 0x4);
static INCLUDE_BSS(snd_cnt_2495, 0x4);
static INCLUDE_BSS(init_2496, 0x4);
static INCLUDE_BSS(FishPlaceMapNum, 0x4);
static INCLUDE_BSS(FishPlaceMap, 0x4);
static INCLUDE_BSS(fpStack, 0x4);
static INCLUDE_BSS(fpNowFishPlaceMap, 0x4);
static INCLUDE_BSS(fpNowFishPlaceMapNum, 0x4);

// Uninitialised data (.bss)
static INCLUDE_BSS(EsaStack, 0x30);
static INCLUDE_BSS(SndStack, 0x30);
static INCLUDE_BSS(CameraInfo, 0x1F0);
static INCLUDE_BSS(UkiCameraInfo, 0x1F0);
static INCLUDE_BSS(CastPoint, 0x10);
static INCLUDE_BSS(CastPointCur, 0x10);
static INCLUDE_BSS(RodData, 0x20);
static INCLUDE_BSS(FishData, 0x30);
static INCLUDE_BSS(MotionBuff, 0x30);
static INCLUDE_BSS(ReadStack, 0x30);
static INCLUDE_BSS(FishingBuff__2, 0x30);
static INCLUDE_BSS(FishStack, 0x30);
static INCLUDE_BSS(BgmStatus, 0x20);
static INCLUDE_BSS(at_1681__2, 0x10);
static INCLUDE_BSS(at_1689, 0x10);
