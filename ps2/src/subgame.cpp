#include "common.h"
#include "subgame.hpp"
#include "scene.hpp"
#include "mglib.hpp"
#include "scenesnd.hpp"
#include "fishing.hpp"
#include "gyorace.hpp"
#include "pbuggy.hpp"
#include "snd_mngr.hpp"
#include <cstdio>

static int SubGame;
static int MenuOpenFlag;
static int ItemOver;
static SubGameInfo GameInfo;

// Code (.text)
void InitSubGame(CScene *scene) {
    SubGame = SUBGAME_NONE;
    MenuOpenFlag = 0;
    ItemOver = 0;
    if (scene->GetCharacter(scene->player_chara) != NULL) {
        mgCTextureManager *tex_manager = &mgTexManager;
        for (int i = 0; i < scene->unk_3e6c; i++) {
            tex_manager->DeleteBlock(scene->unk_3e68 + i);
        }
        for (int j = 0; j < 0x28; j++) {
            scene->DeleteChara(j + 0x40);
        }
        scene->DeleteEffect(7);
    }
}

int SubGameRunning() {
    return SubGame != SUBGAME_NONE;
}

int GetSubGameNo() {
    return SubGame;
}

SubGameInfo *GetNowSubGameInfo() {
    return &GameInfo;
}

int sgMenuOpenEnable() {
    return SubGameRunning() != 0 ? MenuOpenFlag : 1;
}

void sgSetMenuOpenEnableFlag(int enable) {
    MenuOpenFlag = enable;
}

int sgGetItemOver() {
    return ItemOver;
}

void sgGetItemOverReset() {
    ItemOver = 0;
}

void sgGetItemOverFlagOn() {
    ItemOver = 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/subgame", sgInitSubGame__FiP11SubGameInfo);
int sgLoopSubGame() {
    int finished;

    if (SubGameRunning() == 0) {
        return 0;
    }
    finished = 0;
    switch (SubGame) {
        case SUBGAME_FISHING:
            finished = sgLoopFishing(&GameInfo);
            break;
        case SUBGAME_GYORACE:
            finished = sgLoopGyoRace(&GameInfo);
            break;
        case SUBGAME_BUGGY:
            finished = sgLoopBuggy(&GameInfo);
            break;
        case SUBGAME_UNK_4:
            finished = 1;
            break;
    }
    if (finished != 0) {
        SubGame = SUBGAME_NONE;
    }
    return 0;
}

int sgLoopSubGame2() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            sgLoopFishing2(&GameInfo);
            break;
        case SUBGAME_GYORACE:
        case SUBGAME_BUGGY:
        case SUBGAME_UNK_4:
            break;
    }
    return 0;
}

int sgExitSubGame() {
    int result = 0;
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            result = sgExitFishing(&GameInfo);
            break;
        case SUBGAME_GYORACE:
        case SUBGAME_BUGGY:
        case SUBGAME_UNK_4:
            break;
    }
    SubGame = SUBGAME_NONE;
    return result;
}

int sgRestartSubGame(SubGameInfo *info) {
    int result = 0;
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            result = sgRestartFishing(info);
            break;
        case SUBGAME_GYORACE:
        case SUBGAME_BUGGY:
        case SUBGAME_UNK_4:
            break;
    }
    return result;
}

int sgBreakSubGame() {
    int result = 0;
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            result = sgBreakFishing();
            break;
        case SUBGAME_GYORACE:
        case SUBGAME_BUGGY:
        case SUBGAME_UNK_4:
            break;
    }
    SubGame = SUBGAME_NONE;
    return result;
}

int sgDrawSubGameMap() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    if (SubGame != SUBGAME_GYORACE) {
        return 0;
    }
    return sgMapDrawGyoRace(&GameInfo);
}

int sgDrawSubGameCharaShadow() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
        case SUBGAME_GYORACE:
            break;
        case SUBGAME_BUGGY:
            return sgDrawShadowBuggy(&GameInfo);
        case SUBGAME_UNK_4:
            break;
    }
    return 0;
}

int sgDrawSubGameChara() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            return sgDrawFishing(&GameInfo);
        case SUBGAME_GYORACE:
            return sgCharaDrawGyoRace(&GameInfo);
        case SUBGAME_BUGGY:
            return sgDrawBuggy(&GameInfo);
        default:
        case SUBGAME_UNK_4:
            return 0;
    }
}

int sgDrawSubGameEffect() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_GYORACE:
            return sgEffectDrawGyoRace(&GameInfo);
        case SUBGAME_BUGGY:
            return sgEffectDrawBuggy(&GameInfo);
        default:
            return 0;
    }
}

int sgDrawSubGameSystem() {
    if (SubGameRunning() == 0) {
        return 0;
    }
    switch (SubGame) {
        case SUBGAME_FISHING:
            return sgSystemDrawFishing(&GameInfo);
        case SUBGAME_GYORACE:
            return sgSysDrawGyoRace(&GameInfo);
        case SUBGAME_BUGGY:
            return sgSystemDrawBuggy(&GameInfo);
        default:
        case SUBGAME_UNK_4:
            return 0;
    }
}

void sgCPlayVoice::Open(s32 file) {
    if (step > SG_PLAY_VOICE_IDLE) {
        Close();
    }
    step = SG_PLAY_VOICE_OPEN;
    file_no = file;
    play = 0;
}

void sgCPlayVoice::SetVol(float left, float right) {
    float left_volume = left;
    float right_volume = right;
    if (left_volume < 0.0f) {
        left_volume = 0.0f;
    }
    if (!(left_volume <= 1.0f)) {
        left_volume = 1.0f;
    }
    vol_l = left_volume;
    if (right_volume < 0.0f) {
        right_volume = left_volume;
    }
    if (!(right_volume <= 1.0f)) {
        right_volume = 1.0f;
    }
    vol_r = right_volume;
}

void sgCPlayVoice::Play(void) {
    play = 1;
}

int sgCPlayVoice::Step() {
    char name[0x80];
    if (step <= SG_PLAY_VOICE_IDLE) {
        return 0;
    }
    switch (step) {
        case SG_PLAY_VOICE_OPEN:
            sprintf(name, "%d.wav", file_no);
            sndStreamOpenFast(name);
            step++;
            break;
        case SG_PLAY_VOICE_OPENING:
            if (sndStreamOpenState() == 0) {
                sndStreamStandBy();
                step++;
            }
            break;
        case SG_PLAY_VOICE_STANDBY:
            if (sndStreamOpenState() == 0) {
                step++;
            }
            break;
        case SG_PLAY_VOICE_READY:
            if (play != 0) {
                sndStreamSetVol(vol_l, vol_r);
                sndStreamPlay();
                step++;
            }
            break;
        case SG_PLAY_VOICE_PLAYING:
            if (sndStreamGetState() != 0x1000) {
                sndStreamClose();
                step = SG_PLAY_VOICE_IDLE;
                return 0;
            }
            break;
    }
    return 1;
}

void sgCPlayVoice::Close() {
    if (step > SG_PLAY_VOICE_IDLE) {
        sndStreamClose();
        step = SG_PLAY_VOICE_IDLE;
    }
}

// Static initialiser (.init)


// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/subgame", at_985__3__DATA);

// Static initialiser table (.ctor)


// Small uninitialised data (.sbss)




// Uninitialised data (.bss)
