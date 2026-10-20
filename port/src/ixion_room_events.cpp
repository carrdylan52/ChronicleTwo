// Retail IsEventRun, EventScriptSetup (ps2/src/dng_main.cpp) and
// CTreasureBoxManager::PutTreasureBox (ps2/src/dng_event.cpp) verbatim, plus
// the imported room's retained native event policy hooks.
#include "common.h"
#include "mw_runtime.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "actionchara.hpp"
#include "automap.hpp"
#include "cameracontrol.hpp"
#include "character.hpp"
#include "charasetup.hpp"
#include "collision.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dbg_font.hpp"
#include "dng_debug.hpp"
#include "dng_effect.hpp"
#include "dng_event.hpp"
#include "dng_hud.hpp"
#include "dng_main.hpp"
#include "dng_object.hpp"
#include "dng_status.hpp"
#include "editexception.hpp"
#include "effscript.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "eventedit.hpp"
#include "font.hpp"
#include "funcpoint.hpp"
#include "gamedata.hpp"
#include "gamepad.hpp"
#include "helpmes.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "map.hpp"
#include "mapload.hpp"
#include "mapselect.hpp"
#include "menucommon.hpp"
#include "menumain.hpp"
#include "mg_camera.hpp"
#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "monster.hpp"
#include "nd_meswin.hpp"
#include "nowload.hpp"
#include "padcontrol.hpp"
#include "photo.hpp"
#include "pot.hpp"
#include "prespr.hpp"
#include "quest.hpp"
#include "savedata.hpp"
#include "savedatadungeon.hpp"
#include "sceneevent.hpp"
#include "scenesnd.hpp"
#include "screeneffect.hpp"
#include "snd_mngr.hpp"
#include "snd_seseq.hpp"
#include "sphida.hpp"
#include "subgame.hpp"
#include "sysmes.hpp"
#include "userdata.hpp"
#include "water.hpp"
#include "wavetable.hpp"
#include "ixion_room.hpp"
#include "ixion_room_props.hpp"

PORT_OVERRIDE void IsEventRun() {
    sceVu0FVECTOR     pos;
    CBattleCharaInfo *info;
    int               button;
    int               near;
    s16              *event_no;

    if (MainChara__2 == NULL) {
        return;
    }

    MainChara__2->GetPosition(pos);
    event_no = &BattleAreaScene->script.event_no;
    info = GetBattleCharaInfo();

    if (info == NULL) {
        return;
    }

    if (info->GetNowHp_i() <= 0) {
        return;
    }

    if (!MainChara__2->move_check.landed) {
        return;
    }

    static int npc_heal_cnt = 0;

    if (info->chr_no != USER_CHARA_ROBO) {
        npc_heal_cnt++;

        if (npc_heal_cnt > 150) {
            info->UseNPCPoint(-1);
            npc_heal_cnt = 0;
        }
    }

    if (MainChara__2->move_check.ground_poly.area_kind == COL_AREA_HEALING && info->chr_no != USER_CHARA_ROBO) {
        int max;
        int attr;

        attr = info->GetAttr() & (CHARA_STATUS_ALL & ~CHARA_STATUS_POWER);
        max = info->GetMaxHp_i();

        if ((max > info->GetNowHp_i() || attr) && AutoMapGen.healing_point.CheckHealingTime()) {
            float healing = 9999.0f;
            info->AddHp_Point(healing, 0.0f);
            info->SetAttr(CHARA_STATUS_ALL & ~CHARA_STATUS_POWER, 1);
            FxScriptMan->CreateEffSpt("\x92\xca\x8f\xed\x89\xf1\x95\x9c", 0, 0);
            FxScriptMan->SetScriptTargetId(0, -1, -1);
            CPalletAnime *pallet = &MainChara__2->script_pallet;
            pallet->red = 0x60;
            pallet->green = 0xB4;
            pallet->blue = 0xFF;
            pallet->pulse_num = 1;
            pallet->duration = 45;
            pallet->elapsed = 0;
            pallet->repeats = 0;
            sndSePlay(GetSystemSndID(), SYSTEM_SE_RECOVER, 0);
            BattleAreaScene->practice_actions |= DNG_PRACTICE_ACTION_HEAL;
        }
    }

    if (info->GetAttr() & (CHARA_STATUS_STOP | CHARA_STATUS_STONE)) {
        return;
    }

    // N10: a parked monster's queued request stays queued while the room owns the floor.
    int run = IxionRoomTakeMonsterEvent(ActiveMonster);

    if (run != -1) {
        *event_no = run;
        return;
    }

    int num = ActiveMonster->GetMonsterNum(-1.0f);
    num += TreasureBoxMan->MimicCount();

    if (num == 0 && !BattleAreaScene->battle_clear) {
        *event_no = DNG_EVENT_FLOOR_CLEAR;
        BattleAreaScene->battle_clear = true;
        ColPrimMan.Initialize(DngMainScene);
        BTsubo.Clear();
        BTsuboCol = NULL;
        return;
    }

    if (DngStatus.eye_view) {
        return;
    }

    if (!DngSaveData->GetBitFlag(SAVE_FLAG_GUIDE_MAGIC_SWORD) && info->GetMagicSwordCounterNow() > 0) {
        *event_no = DNG_EVENT_MAGIC_SWORD_GUIDE;
        DngSaveData->SetBitFlag(SAVE_FLAG_GUIDE_MAGIC_SWORD, 1);
        return;
    }

    // Retained native event policy: while the imported room owns the floor, the
    // position-keyed native checks below (random circle, map event points such
    // as the gate and doors, ridepod/level-up guides, random stone, geostone,
    // treasure box, monster talk) would compare the imported player position
    // with native-coordinate objects. They are gated until retirement.
    if (IxionRoomGatesNativeFieldEvents()) {
        // Imported chests use native pickup/scripts, but only our exact slot
        // may pass the field gate. Keep native-coordinate checks below gated.
        if (MainChara__2->CheckRunEvent() && TreasureBoxMan != nullptr) {
            const float dist = info->chr_no == USER_CHARA_ROBO ? 50.0f : 30.0f;
            const int slot = TreasureBoxMan->CheckEvent(pos, dist);
            const auto *target = ActiveMonster->GetPriorityLevelIndex(0, nullptr);
            const bool near_enemy = target != nullptr && target->target_dist < 200.0f;
            if (IxionRoomPropsOwnsSlot(TreasureBoxMan, slot) && !near_enemy) {
                if (PadCtrl.Btn(PAD_BTN_CONFIRM) && !PadCtrl.Btn(PAD_BTN_ACTION_SQUARE)) {
                    *event_no = DNG_EVENT_TREASURE_BOX;
                } else if (RedMarkModel != nullptr) {
                    RedMarkModel->draw_request = true;
                }
            }
        }
        return;
    }

    if (!MainChara__2->CheckRunEvent()) {
        return;
    }

    if (RandomCircle.CheckEvent(pos) != -1) {
        *event_no = DNG_EVENT_RANDOM_CIRCLE;
        MainChara__2->ResetAccele();
        return;
    }

    button = MAP_EVENT_CHECK_TOUCH;

    if (PadCtrl.Btn(PAD_BTN_CONFIRM)) {
        button = MAP_EVENT_CHECK_ACTION;
    }

    if (PadCtrl.Btn(PAD_BTN_ACTION_SQUARE)) {
        button = MAP_EVENT_CHECK_ITEM;
    }

    near = false;
    CActiveMonster *target = ActiveMonster->GetPriorityLevelIndex(0, NULL);

    if (target != NULL && target->target_dist < 200.0f) {
        near = true;
    }

    CSceneEventData data;

    if (DngMainScene->GetMapEvent(pos, button, &data)) {
        DngMainScene->RunEvent(data.event.point_no, &data);
        return;
    }

    if (DngMainScene->map_event_no) {
        RedMarkModel->draw_request = true;
    }

    if (!DngSaveData->GetBitFlag(SAVE_FLAG_ROBO_BIKE_EVENT_SEEN) && info->chr_no == USER_CHARA_ROBO && MainChara__2->move_type == ACTION_MOVE_ROBO_BIKE) {
        *event_no = DNG_EVENT_ROBO_BIKE_GUIDE;
        DngSaveData->SetBitFlag(SAVE_FLAG_ROBO_BIKE_EVENT_SEEN, 1);
    }

    if (!DngSaveData->GetBitFlag(SAVE_FLAG_GUIDE_RANDOM_STONE) && (info->chr_no == USER_CHARA_MAX || info->chr_no == USER_CHARA_MONICA)) {
        if (AutoMapGen.SearchRandomStone(pos, 30.0f)) {
            if (button == MAP_EVENT_CHECK_ACTION) {
                *event_no = DNG_EVENT_RANDOM_STONE;
                return;
            }

            RedMarkModel->draw_request = true;
        }
    }

    if (!DngSaveData->GetBitFlag(SAVE_FLAG_GUIDE_LEVEL_UP) && LevelupInfo.phase != 0) {
        *event_no = DNG_EVENT_LEVEL_UP_GUIDE;
        DngSaveData->SetBitFlag(SAVE_FLAG_GUIDE_LEVEL_UP, 1);
        return;
    }

    if (GeoStone.CheckEvent(pos)) {
        if (button == MAP_EVENT_CHECK_ACTION) {
            *event_no = DNG_EVENT_GEOSTONE;
            return;
        }

        RedMarkModel->draw_request = true;
    }

    float dist = 30.0f;

    if (info->chr_no == USER_CHARA_ROBO) {
        dist = 50.0f;
    }

    if (TreasureBoxMan->CheckEvent(pos, dist) >= 0 && !near) {
        if (button == MAP_EVENT_CHECK_ACTION) {
            *event_no = DNG_EVENT_TREASURE_BOX;
            return;
        }

        RedMarkModel->draw_request = true;
    }

    int talk = ActiveMonster->CheckMonsterTolk(pos);

    if (talk != -1) {
        if (button == MAP_EVENT_CHECK_ACTION || button == MAP_EVENT_CHECK_ITEM) {
            *event_no = DNG_EVENT_MONSTER_TALK;
            EdEventInfo.monster_talk[0] = talk + DNG_CHARA_SLOT_MONSTER;
            EdEventInfo.monster_talk[2] = ActiveMonster->active[talk]->locate_param;
            EdEventInfo.monster_talk[1] = ActiveMonster->active[talk]->monster_id;
        } else {
            RedMarkModel->draw_request = true;
        }
    }
}

// Retail EventScriptSetup (ps2/src/dng_main.cpp) verbatim, preceded by the
// retained native event policy before setup side effects. Direct starts from
// debug commands, script-loading continuations and Sphida are guarded at the
// common RunEvent VM boundary (port/src/event.cpp).
PORT_OVERRIDE int EventScriptSetup(SYSTEM_SCRIPT_INFO *script) {
    IxionRoomBeforeNativeEvent(script->event_no);
    InitEvent(DngMainScene);
    VoiceUnit.StopVoice(10);
    DngMainScene->fade.ResetFade();

    if (RunEvent(script->event_no, DngMainScene)) {
        script->running = true;
        printf("RUN SYS_SCRIPT %d\n", script->event_no);
        DngStatus.mode = DNG_STATUS_EVENT;
        DngMainScene->before_camera = 0;
        ResetEyeView(MainChara__2);
        memcpy(&EventCamera__2, &MainCamera, sizeof(CCameraControl));
        DngMainScene->active_camera = 1;
        MainChara__2->sound_info.foot_sound_id = -1;
        LoopSoundManager(0);
        BattleAreaScene->pause_flag |= DNG_PAUSE_PLAYER_STATUS;
        MainChara__2->RemoveThrowItem();
        MsgTaskMan.Clear();
        BattleAreaScene->script.event_no = -1;
        return true;
    }

    return false;
}

// Retail CTreasureBoxManager::PutTreasureBox (ps2/src/dng_event.cpp) verbatim,
// followed by the room's box invariant: a box posted while installed whose
// pickup radius reaches the donor bounds retires the room (boxes are never parked).
PORT_OVERRIDE void CTreasureBoxManager::PutTreasureBox(int index, float *pos, float rot_y, int flags, int item0, int num0, int item1, int num1) {
    int           i;
    CTreasureBox *chest;

    if (index == -1) {
        i = 0;

        do {
            chest = &this->box[i];

            if (chest->state == TREASURE_BOX_STATE_NONE) {
                index = i;
                break;
            }

            i += 1;
        } while (i < TREASURE_BOX_MAX);
    }

    if (index < 0 || index >= TREASURE_BOX_MAX) {
        return;
    }

    chest = &this->box[index];
    chest->state = TREASURE_BOX_STATE_UNOPENED;
    chest->SetPosition(pos);
    chest->SetRotation(0.0f, rot_y, 0.0f);
    chest->flags = flags;
    chest->item[0] = item0;
    chest->item[1] = item1;
    chest->num[0] = num0;
    chest->num[1] = num1;
    IxionRoomAfterTreasureBoxPosted(this, index);
}
