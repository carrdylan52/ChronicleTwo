#include "common.h"
#include "actionchara.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <libvu0.h>

#include "actscript.hpp"
#include "automap.hpp"
#include "cameracontrol.hpp"
#include "character.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dng_effect.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "dng_status.hpp"
#include "effect.hpp"
#include "effscript.hpp"
#include "gamedata.hpp"
#include "gameutil.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "map.hpp"
#include "mapparts.hpp"
#include "mdslist.hpp"
#include "menucommon.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "monster.hpp"
#include "padcontrol.hpp"
#include "runscript.hpp"
#include "savedata.hpp"
#include "scene.hpp"
#include "sceneload.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "sphida.hpp"
#include "swordeffect.hpp"
#include "userdata.hpp"

#ifdef NONMATCHING
static float old_angle; /**< Previous stick direction used by the movement acceleration ramp. */
#endif

// Code (.text)
void CActionChara::ResetAccele(void) {
    accele.accele[2] = 0;
    accele.accele[1] = 0;
    accele.accele[0] = 0;
    accele.speed = 0;
}
void CActionChara::ResetAction() {
    int index;

    AllDeleteDamage();
    for (index = 0; index < 3; index++) {
        if (sword_effect[index] != NULL) {
            sword_effect[index]->Clear();
        }
    }
    hold_type = ACTION_HOLD_NONE;
    add_speed = 0.0f;
    add_time = 0;
    blow_speed = 0.0f;
    blow_time = 0;
    accume.frame = NULL;
    action_info.chara = this;
    action_info.env = NULL;
    if (script.check_program(ACTION_PROG_RESET) != 0) {
        script.run(ACTION_PROG_RESET);
    }
    prog_no = ACTION_PROG_MAIN;
}

void CActionChara::ResetScript() {
    int j;
    int index;
    int l;
    int m;

    for (index = 0; index < 8; index++) {
        object[index].frame = NULL;
    }
    for (j = 0; j < 16; j++) {
        body_col[j].type = 0;
        body_col[j].unk_20 = -1;
    }
    for (index = 0; index < 11; index++) {
        damage[index].use = 0;
        damage[index].chara = NULL;
        damage[index].frame0 = NULL;
        damage[index].frame1 = NULL;
        damage[index].damage = NULL;
        damage[index].prim = NULL;
    }
    damage_num = 0;
    for (l = 0; l < 10; l++) {
        sound[l].se_no = -1;
    }
    for (m = 0; m < 3; m++) {
        if (sword_effect[m] != NULL) {
            sword_effect[m]->Clear();
        }
    }
    hold_type = ACTION_HOLD_NONE;
    add_speed = 0.0f;
    add_time = 0;
    blow_speed = 0.0f;
    blow_time = 0;
    accume.frame = NULL;
}

s32 CActionChara::CheckRunEvent(void) {
    s32 can_run = menu_flag;
    if (hold_type != 0) {
        can_run = 0;
    }
    return can_run;
}
void CActionChara::SetMaskFlag(int flag, int on) {
    if (on != 0) {
        mask_flag |= flag;
    } else {
        mask_flag &= ~flag;
    }
}

ACTION_OBJECT *CActionChara::EntryObject(char *name, int no) {
    mgCFrame      *entry_frame;
    ACTION_OBJECT *entry;
    int            index;

    entry_frame = SearchObject(name);
    if (entry_frame == NULL) {
        return NULL;
    }
    index = 0;
    if (no != -1) {

        object[no].frame = entry_frame;
        object[no].pos[2] = 0.0f;
        object[no].pos[1] = 0.0f;
        object[no].pos[0] = 0.0f;
        return &object[no];
    }
    for (index = 0 ; index < 8; index++) {
        if (object[index].frame == NULL) {
            object[index].frame = entry_frame;
            object[index].pos[2] = 0.0f;
            object[index].pos[1] = 0.0f;
            object[index].pos[0] = 0.0f;
            return &object[index];
        }
    }
    return NULL;
}

void CActionChara::CalcCollision(void) {
    ACTION_OBJECT *entry = object;
    s32 index = 0;
    do {
        mgCFrame *frame = entry->frame;
        if (frame != NULL) {
            frame->GetWorldPosition0(entry->pos);
        }
        index += 1;
        entry += 1;
    } while (index < 8);
}
ACTION_BODY_COL *CActionChara::EntryBodyCol(int object_no, float value) {
    int index;

    if (object_no < 0 || object_no >= 8) {
        return NULL;
    }
    if (object[object_no].frame == NULL) {
        return NULL;
    }
    for (index = 0; index < 16; index++) {
        if (body_col[index].type == 0) {
            body_col[index].type = 2;
            body_col[index].object = object_no;
            body_col[index].radius = value;
            return &body_col[index];
        }
    }
    return NULL;
}

ACTION_DAMAGE *CActionChara::EntryDamage2(char *frame_name_a, char *frame_name_b, char *damage_name, float power,
                                 char *motion, float start, float end, char *chara_name) {
    mgCFrame *first_frame;
    mgCFrame *second_frame;
    int i;
    float start_frame;
    float end_frame;

    if (damage_num >= 11) {
        return NULL;
    }
    first_frame = NULL;
    second_frame = NULL;
    if (frame_name_a != NULL) {
        first_frame = SearchObject(frame_name_a);
    }
    if (frame_name_b != NULL) {
        second_frame = SearchObject(frame_name_b);
    }
    for (i = 0; i < 11; i++) {
        if (damage[i].use == 0) {
            start_frame = GetWaitToFrame(motion, start, chara_name);
            end_frame = GetWaitToFrame(motion, end, chara_name);
            if (start_frame == 0.0f && end_frame == 0.0f) {
                return NULL;
            }
            damage[i].use = 1;
            damage[i].frame0 = first_frame;
            damage[i].frame1 = second_frame;
            damage[i].damage = damage_name;
            damage[i].start_frame = start_frame;
            damage[i].end_frame = end_frame;
            damage[i].chara = chara_name;
            damage[i].radius = power;
            damage[i].power_rate = 1.0f;
            damage_num++;
            return &damage[i];
        }
    }
    return NULL;
}

ACTION_DAMAGE *CActionChara::EntryDamage2(mgCFrame *frame_a, mgCFrame *frame_b, char *damage_name, float power,
                                 char *motion, float start, float end, char *chara_name) {
    int i;
    float start_frame;
    float end_frame;

    if (damage_num >= 11) {
        return NULL;
    }
    for (i = 0; i < 11; i++) {
        if (damage[i].use == 0) {
            start_frame = GetWaitToFrame(motion, start, chara_name);
            end_frame = GetWaitToFrame(motion, end, chara_name);
            if (start_frame == 0.0f && end_frame == 0.0f) {
                return NULL;
            }
            damage[i].use = 1;
            damage[i].frame0 = frame_a;
            damage[i].frame1 = frame_b;
            damage[i].damage = damage_name;
            damage[i].start_frame = start_frame;
            damage[i].end_frame = end_frame;
            damage[i].chara = chara_name;
            damage[i].radius = power;
            damage[i].power_rate = 1.0f;
            damage_num++;
            return &damage[i];
        }
    }
    return NULL;
}

void CActionChara::AllDeleteDamage() {
    int index;

    for (index = 0; index < damage_num; index++) {
        if (damage[index].use != 0 && damage[index].prim != NULL) {
            damage[index].prim->Delete(-1);
        }
    }
}

ACTION_SW_EFFECT *CActionChara::GetSwEffectPtr() {
    ACTION_SW_EFFECT *entry;
    int               index;

    entry = sw_effect;
    for (index = 0; index < 9; index++, entry++) {
        if (entry->motion == NULL) {
            return entry;
        }
    }
    return NULL;
}

void CActionChara::SetSoundInfoCopy() {
    CActionChara *part;
    u32 *info;

    info = &foot_se_bank;
    for (part = next; part != NULL; part = part->next) {
        memcpy(&part->foot_se_bank, info, 0x28);
    }
}

void CActionChara::SetFadeFlag(int flag) {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->fade = flag;
    }
}

void CActionChara::SetFarDist(float dist) {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->far_dist = dist;
    }
}

void CActionChara::SetNearDist(float dist) {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->near_dist = dist;
    }
}

float CActionChara::GetCameraDist() {
    if (parent != NULL) {
        return parent->CCharacter2::GetCameraDist();
    }
    return CCharacter2::GetCameraDist();
}

void CActionChara::Show(int visible, int all) {
    CActionChara *part;

    if (all == 0) {
        show = visible;
        return;
    }
    for (part = this; part != NULL; part = part->next) {
        part->show = visible;
    }
}

int CActionChara::GetShow(char *chara) {
    CActionChara *part;
    int show;

    show = 0;
    part = this;
    if (chara != NULL) {
        if (this != NULL) {
            do {
                if (strcmp(part->name, chara) == 0) {
                    return ((CObject *)part)->show;
                }
                part = part->next;
            } while (part != NULL);
        }
    } else {
        show = ((CObject *)this)->show;
    }
    return show;
}

int CActionChara::CheckKeri(char *name, int kick) {
    sceVu0FVECTOR position;
    mgCFrame     *frame;
    CMapParts    *stone;
    CMapPiece    *piece;
    float         radius;

    frame = SearchObject(name);
    if (frame == NULL) {
        return 0;
    }
    frame->GetWorldPosition0(position);
    position[3] = 1.0f;
    radius = 30.0f;
    if (kick != 0) {
        radius = 40.0f;
    }
    stone = AutoMapGen.SearchRandomStone(position, radius);
    if (stone != NULL) {
        if (kick != 0) {
            piece = stone->SearchPiece("rnd_obj01-a");
            if (piece != NULL) {
                piece->Show(0);
            }
            release_timing = 5;
            hold_parts = stone;
        }
        return 1;
    }
    return 0;
}

int CActionChara::CheckEnemyCatch(char *name) {
    mgCFrame *frame;
    CMapParts *stone;
    CMapPiece *piece;
    CActionChara *part;
    CBattleCharaInfo *battle;
    float position[4];
    float one;

    one = 1.0f;
    frame = SearchObject(name);
    if (frame == NULL) {
        return 0;
    }
    if (hold_type != ACTION_HOLD_NONE) {
        return 0;
    }
    if (ActiveMonster->CheckThrowTarget(frame) != NULL) {
        hold_type = ACTION_HOLD_ENEMY;
        release_timing = 1;
        part = SearchChara("sword");
        if (part != NULL) {
            part->Show(0, 0);
        }
        battle = GetBattleCharaInfo();
        if (battle->chr_no == USER_CHARA_MAX) {
            part = SearchChara("shot");
            if (part != NULL) {
                part->Show(0, 0);
            }
        }
        battle->AddHp_Rate(-0.05f, 3, 1.0f);
        return 1;
    }
    frame->GetWorldPosition0(position);
    position[3] = 1.0f;
    stone = AutoMapGen.SearchRandomStone(position, 30.0f);
    if (stone != NULL) {
        piece = stone->SearchPiece("rnd_obj01-a");
        if (piece != NULL) {
            piece->Show(0);
        }
        release_timing = 1;
        hold_frame = frame;
        hold_parts = stone;
        hold_type = ACTION_HOLD_STONE;
        part = SearchChara("sword");
        if (part != NULL) {
            part->Show(0, 0);
        }
        if ((GetBattleCharaInfo())->chr_no == USER_CHARA_MAX) {
            part = SearchChara("shot");
            if (part != NULL) {
                part->Show(0, 0);
            }
        }
        return 1;
    }
    return 0;
}

void CActionChara::ThrowItemObject() {
    sceVu0FVECTOR target;
    sceVu0FVECTOR position;
    CGameDataUsed *item;
    int index;

    if (hold_type != ACTION_HOLD_NONE) {
        if (throw_effect >= 0) {
            effect_man->SetScriptProgNo(300, 0, throw_effect);
            sceVu0CopyVector(target, front_vec);
            GetPosition(position);
            sceVu0ScaleVector(target, target, 120.0f);
            sceVu0AddVector(target, target, position);
            effect_man->SetScriptVect1(target, 0, throw_effect);
            hold_type = ACTION_HOLD_NONE;

            item = GetBattleCharaInfo()->GetActiveItemInfo(0);
            index = DngStatus.active_item;
            item = &item[index];
            item->DeleteNum(1);
        }
    }
}

int CActionChara::UsedItemAction() {
    CBattleCharaInfo *battle;
    CGameDataUsed    *item;
    CDataItem        *info;
    int              item_no;
    int              effect_type;

    battle = GetBattleCharaInfo();
    item = &battle->GetActiveItemInfo(0)[DngStatus.active_item];
    if (DngStatus.active_item == 3) {
        return 3;
    }
    item_no = item->item_no;
    info = GetItemInfoData(item_no);
    if (info != NULL) {
        if (info->status_flags & 0x6) {
            EntryThrowItem();
            return 2;
        }
        if (info->status_flags & 0x19) {
            if (battle->UseActiveItem(item) != 0) {
                if (info->status_flags & 0x18) {
                    effect_type = 0;
                    if (item_no == 274) {
                        effect_type = 1;
                    }
                    if (effect_type == 0) {
                        pallet[0].SetAnim(96, 180, 255, 1, 45, 0);
                    }
                    if (effect_type == 1) {
                        pallet[0].SetAnim(255, 220, 64, 1, 45, 0);
                    }
                    effect_man->CreateEffSpt("\x92\xCA\x8F\xED\x89\xF1\x95\x9C", 0, 0);
                    effect_man->SetScriptTargetId(0, -1, -1);
                    effect_man->SetValue(0, effect_type, 0, -1);
                }
            }
            return 1;
        }
    }
    return 0;
}

void CActionChara::EntryThrowItem() {
    CGameDataUsed *item;
    int i;
    int item_no;

    item = &GetBattleCharaInfo()->GetActiveItemInfo(0)[DngStatus.active_item];
    item_no = item->item_no;
    int item_numbers[] = {
        276, 280, 281, 282, 283, 284, 285, 286, 287, 288,
        289, 290, 291, 292, 304, 390, 391, 307, -1
    };
    i = 0;
    while (item_numbers[i] != -1) {
        if (item_no == item_numbers[i]) {
            break;
        }
        i++;
    }
    if (item_numbers[i] == -1) {
        i = 0;
    }
    throw_effect = effect_man->CreateEffSpt("\x8E\xE8\x93\x8A\x82\xB0\x94\x9A\x92\x65", 0, 1);
    if (throw_effect < 0) {
        printf("effect entry err\n");
    } else {
        effect_man->SetValue(0, 1, 0, throw_effect);
        effect_man->SetValue(1, item_no, 0, throw_effect);
        if (action_info.env != NULL) {
            effect_man->SetCharacter(
                &action_info.env->item_chara[i], 0, throw_effect);
            effect_man->SetTexb(action_info.env->texb, 0, throw_effect);
            i = 0;
            if (item_no == 304) {
                for (i = 0; i < 3; i++) {
                    effect_man->SetValue(i + 2, item->GetGiftBoxItemNo(i), 0, throw_effect);
                }
            }
        }
    }
    hold_type = ACTION_HOLD_ITEM;
}

void CActionChara::RemoveThrowItem() {
    if (hold_type != ACTION_HOLD_NONE) {
        GetBattleCharaInfo();
        if (hold_type == ACTION_HOLD_ITEM) {
            if (throw_effect >= 0) {
                effect_man->DeleteEffSpt(0, throw_effect);
            }
            hold_type = ACTION_HOLD_NONE;
        }
    }
}

float CActionChara::GetNowFrameWait(char *chara) {
    CActionChara *part;
    float wait;

    wait = 0.0f;
    part = this;
    if (chara != NULL) {
        for (part = this; part != NULL; part = part->next) {
            if (strcmp(part->name, chara) == 0) {
                return part->frame_ratio;
            }
        }
    } else {
        wait = CCharacter2::frame_ratio;
    }
    return wait;
}

float CActionChara::GetNowFrame(char *chara) {
    CActionChara *part;
    float frame;

    frame = 0.0f;
    part = this;
    if (chara != NULL) {
        for (part = this; part != NULL; part = part->next) {
            if (strcmp(part->name, chara) == 0) {
                return part->CCharacter2::frame;
            }
        }
    } else {
        frame = CCharacter2::frame;
    }
    return frame;
}

int CActionChara::CheckMotionEnd(char *chara) {
    CActionChara *part;
    int result;

    result = 0;
    part = this;
    if (chara != NULL) {
        for (; part != NULL; part = part->next) {
            if (strcmp(part->name, chara) == 0) {
                return part->CCharacter2::CheckMotionEnd();
            }
        }
    } else {
        result = CCharacter2::CheckMotionEnd();
    }
    return result;
}

int CActionChara::GetMotionStatus(char *chara) {
    CActionChara *part;
    int status;

    status = 0;
    part = this;
    if (chara != NULL) {
        if (this != NULL) {
            do {
                if (strcmp(part->name, chara) == 0) {
                    return part->motion_status;
                }
                part = part->next;
            } while (part != NULL);
        }
    } else {
        status = CCharacter2::motion_status;
    }
    return status;
}

float CActionChara::GetWaitToFrame(char *motion, float rate, char *chara) {
    CActionChara *part;
    CHRINFO_KEY_SET *key;
    float start;

    part = this;
    start = 0.0f;
    if (chara != NULL) {
        for (; part != NULL; part = part->next) {
            if (strcmp(part->name, chara) == 0) {
                key = part->GetKeyListPtr(motion, NULL);
                if (key != NULL) {
                    return (float)key->start_frame +
                           rate * ((float)key->end_frame - (float)key->start_frame);
                }
            }
        }
    } else {
        key = CCharacter2::GetKeyListPtr(motion, NULL);
        if (key != NULL) {
            start = key->start_frame + rate * (key->end_frame - (float)key->start_frame);
        }
    }
    return start;
}

void CActionChara::SetMotion(int no, int flags) {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->CCharacter2::SetMotion(no, flags);
    }
}

void CActionChara::SetMotion(char *motion, int flags, int all) {
    CActionChara *current;

    current = this;
    if (all == 0) {
        CCharacter2::SetMotion(motion, flags);
        return;
    }
    if (this != NULL) {
        do {
            current->CCharacter2::SetMotion(motion, flags);
            current = current->next;
        } while (current != NULL);
    }
}

void CActionChara::ResetMotion() {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->CCharacter2::ResetMotion();
    }
}

int CActionChara::Draw() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR old_position;
    sceVu0FVECTOR ambient;
    int result;
    CActionChara *part;

    part = this;
    GetPosition(position);
    GetPosition(old_position);
    if (damage_time > 6) {
        position[1] += 2.5f * mgRnd();
    }
    SetPosition(position);
    mgGetAmbient(ambient);
    if (this != NULL) {
        do {
            result = part->CCharacter2::Draw();
            part->CalcCollision();
            part = part->next;
        } while (part != NULL);
    }
    SetPosition(old_position);
    mgSetAmbient(ambient);
    return result;
}

int CActionChara::DrawDirect() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR old_position;
    sceVu0FVECTOR ambient;
    sceVu0FVECTOR flash;
    int result;
    CActionChara *part;
    int i;

    part = this;
    GetPosition(position);
    GetPosition(old_position);
    if (shake.time > 0) {
        position[1] += shake.offset;
    }
    SetPosition(position);
    mgGetAmbient(ambient);
    i = 0;
    do {
        if (pallet[i].CreatPallet(flash, ambient) != 0) {
            mgSetAmbient(flash);
            break;
        }
        i++;
    } while (i < 3);
    if (this != NULL) {
        do {
            result = part->CCharacter2::DrawDirect();
            part->CalcCollision();
            part = part->next;
        } while (part != NULL);
    }
    SetPosition(old_position);
    mgSetAmbient(ambient);
    return result;
}

int CActionChara::DrawShadowDirect() {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->CCharacter2::DrawShadowDirect();
    }
}

void CActionChara::DrawEffect() {
    CCharacter2::DrawEffect();
    if (effect_man != NULL) {
        effect_man->Draw();
    }
}

void CActionChara::StepEffect() {
    ACTION_SW_EFFECT *effect;
    CActionChara     *part;
    mgCFrame         *first_frame;
    mgCFrame         *second_frame;
    float             motion_frame;
    int               i;

    for (i = 0; i < sw_effect_num; i++) {
        effect = &sw_effect[i];
        if (effect->motion != NULL) {
            if (effect->wait > 0) {
                effect->wait--;
            } else {
                part = this;
                if (effect->chara != NULL) {
                    part = SearchChara(effect->chara);
                }
                if (part != NULL && part->GetNowMotionName() != NULL && strcmp(part->GetNowMotionName(), effect->motion) == 0) {
                    motion_frame = part->GetNowFrameWait(NULL);
                    if (motion_frame >= effect->start && motion_frame < effect->end) {
                        first_frame = part->SearchObject(effect->frame0);
                        second_frame = part->SearchObject(effect->frame1);
                        if (first_frame != NULL && second_frame != NULL) {
                            sword_effect[effect->sword_no]->StartEffect(first_frame, second_frame, effect->unk_1c, effect->fade_time, effect->unk_1d);
                            effect->wait = 5;
                        }
                    }
                }
            }
        }
    }
    CCharacter2::StepEffect();
}

CActionChara *CActionChara::SearchChara(char *chara) {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        if (strcmp(part->name, chara) == 0) {
            return part;
        }
    }
    return NULL;
}

mgCFrame *CActionChara::SearchObject(char *name) {
    CActionChara *current;
    mgCFrame *frame;
    mgCFrame *found;

    current = this;
    if (this != NULL) {
        do {
            frame = current->CObjectFrame::frame;
            if (frame == NULL) {
                current = current->next;
                continue;
            }
            found = frame->SearchFrame(name);
            if (found != NULL) {
                return found;
            }
            current = current->next;
        } while (current != NULL);
    }
    return NULL;
}

void CActionChara::ResetParent() {
    next = NULL;
    if (CObjectFrame::frame != NULL) {
        CObjectFrame::frame->DeleteReference();
    }
}

int CActionChara::SetRef(CActionChara *part, char *name) {
    mgCFrame *reference;
    mgCFrame *part_frame;
    CActionChara *tail;
    CActionChara *following;

    if (part == NULL) {
        return 0;
    }
    part_frame = part->CObjectFrame::frame;
    if (part_frame == NULL) {
        return 0;
    }
    reference = SearchObject(name);
    if (reference == NULL) {
        return 0;
    }
    part_frame->DeleteReference();
    part_frame->SetReference(reference);
    part->chara_kind = ACTION_KIND_PART;
    for (tail = this ;;) {
        following = tail->next;
        if (following == NULL) {
            tail->next = part;
            tail->next->parent = this;
            break;
        }
        tail = following;
    }
    return 1;
}

float CActionChara::GetTargetDist(CScene *scene) {
    CCharacter2   *target;
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  target_position;

    if (target_no != -1) {
        target = scene->GetCharacter(target_no);
        if (target != NULL) {
            GetPosition(position);
            target->GetPosition(target_position);
            return mgDistVector(position, target_position);
        }
    }
    return -1.0f;
}

/**
 * Finds a living monster that can be selected as a lock-on target.
 */
static int RockOn_TargetSel(CScene *scene, int target_no) {
    CActiveMonster *monster;
    int             count;

    if (target_no != -1) {
        target_no--;
        for (count = 0; count < 24; count++) {
            target_no++;
            if (target_no >= 48) {
                target_no = 24;
            }
            monster = (CActiveMonster *)scene->GetCharacter(target_no);
            if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT &&
                monster->state == ACTIVE_MONSTER_LIVE && monster->catch_state != 1 &&
                (monster->attrib & MONSTER_ATTRIB_NO_LOCK_ON) == 0) {
                return target_no;
            }
        }
        return -1;
    }

    for (count = 0; count < 24; count++) {
        monster = (CActiveMonster *)scene->GetCharacter(count + 24);
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT &&
            monster->state == ACTIVE_MONSTER_LIVE && monster->catch_state != 1 &&
            (monster->attrib & MONSTER_ATTRIB_NO_LOCK_ON) == 0) {
            return count + 24;
        }
    }
    return -1;
}

/**
 * Selects a monster in range, optionally by its rank among the nearest targets.
 */
static int DistCheck_Action2(CScene *scene, float facing_limit, float max_distance, float *out_distance, int rank, int *out_rank) {
    sceVu0FVECTOR direction;
    sceVu0FVECTOR player_position;
    sceVu0FVECTOR player_rotation;
    sceVu0FVECTOR monster_position;
    sceVu0FVECTOR front;
    float distances[MONSTER_ACTIVE_MAX];
    int targets[MONSTER_ACTIVE_MAX];
    CActionChara *player;
    int count;
    int index;
    CActionChara *monster;
    int nearest;
    float nearest_distance;
    float distance;
    int row;
    int min;
    int column;
    float best;
    int saved_index;
    float saved_distance;

    player = (CActionChara *)scene->GetCharacter(0);
    direction[3] = 1.0f;
    nearest_distance = max_distance;
    player->GetPosition(player_position);
    player->GetRotation(player_rotation);
    sceVu0CopyVector(front, player->front_vec);
    count = 0;
    nearest = -1;
    index = 0;
    do {
        monster = (CActiveMonster *)scene->GetCharacter(index + MONSTER_ACTIVE_MAX);
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT && ((CActiveMonster *)monster)->state == 1 &&
            ((CActiveMonster *)monster)->catch_state != 1 &&
            !(((CActiveMonster *)monster)->attrib & MONSTER_ATTRIB_NO_LOCK_ON)) {
            monster->GetEntryObjectPos(0, 0, monster_position);
            distance = ((CActiveMonster *)monster)->target_dist;
            if (distance < max_distance || ((CActiveMonster *)monster)->tbl->boss != 0) {
                if (nearest_distance > distance || ((CActiveMonster *)monster)->tbl->boss != 0) {
                    nearest = index;
                    nearest_distance = distance;
                }
                direction[0] = monster_position[0] - player_position[0];
                direction[1] = monster_position[1] - player_position[1];
                direction[2] = monster_position[2] - player_position[2];
                sceVu0Normalize(direction, direction);
                sceVu0InnerProduct(front, direction);
                distances[count] = distance;
                targets[count] = index;
                count++;
            }
        }
        index++;
    } while (index < MONSTER_ACTIVE_MAX);
    if (rank == 0 || count < 2) {
        if (nearest == -1) {
            return -1;
        }
        *out_distance = nearest_distance;
        return nearest + 24;
    }
    for (row = 0; row < count; row++) {
        min = row;
        for (column = row; column < count; column++) {
            if (column != row) {
                best = distances[column];
                if (best >= 0.0f && distances[min] > best) {
                    min = column;
                }
            }
        }
        if (row != min) {
            saved_distance = distances[row];
            saved_index = targets[row];
            distances[row] = distances[min];
            targets[row] = targets[min];
            distances[min] = saved_distance;
            targets[min] = saved_index;
        }
    }
    if (rank >= count) {
        rank = 0;
    }
    *out_distance = distances[rank];
    if (out_rank != NULL) {
        *out_rank = rank;
    }
    return targets[rank] + 24;
}

/**
 * Checks whether the current lock-on target is still alive and within range.
 */
static int Check_LockOn(CScene *scene, float max_distance, int target_no) {
    sceVu0FVECTOR  player_position;
    sceVu0FVECTOR  monster_position;
    CCharacter2   *player;
    CActiveMonster *monster;
    int farther;

    player = scene->GetCharacter(0);
    player->GetPosition(player_position);
    monster = (CActiveMonster *)scene->GetCharacter(target_no);
    if (monster == NULL) {
        return 0;
    }
    if (monster->chara_kind != ACTION_KIND_SCRIPT) {
        return 0;
    }
    if (monster->state != ACTIVE_MONSTER_LIVE) {
        return 0;
    }
    if ((monster->attrib & MONSTER_ATTRIB_NO_LOCK_ON) != 0) {
        return 0;
    }
    if (monster->catch_state == 1) {
        return 0;
    }
    if (monster->tbl->boss != 0) {
        return 1;
    }
    monster->GetEntryObjectPos(0, 0, monster_position);
    player_position[3] = 1.0f;
    monster_position[3] = 1.0f;
    farther = 1;
    if (((CActiveMonster *)monster)->target_dist <= max_distance) {
        farther = 0;
    }

    return farther = farther ^ 1;
}

#ifdef NONMATCHING
void CActionChara::CollisionCheck(float *pos, float *velocity, float *out_velocity) {
    sceVu0FVECTOR      next_position;
    sceVu0FVECTOR      flat_position;
    sceVu0FVECTOR      body_position;
    sceVu0FVECTOR      push_direction;
    CActiveMonster    *monster;
    CHARA_ENTRY_OBJECT *body;
    float              distance;
    float              separation;
    int                index;
    int                body_no;

    sceVu0CopyVector(out_velocity, velocity);
    next_position[0] = pos[0] + out_velocity[0];
    next_position[1] = 20.0f + (pos[1] + out_velocity[1]);
    next_position[2] = pos[2] + out_velocity[2];
    next_position[3] = 1.0f;
    sceVu0CopyVector(flat_position, next_position);
    flat_position[1] = 0.0f;

    for (index = 24; index < 48; index++) {
        monster = (CActiveMonster *)nowScene__2->GetCharacter(index);
        if (monster == NULL || monster->chara_kind != ACTION_KIND_SCRIPT ||
            monster->state == ACTIVE_MONSTER_NONE ||
            (monster->state == ACTIVE_MONSTER_DEAD && monster->alpha < 0.6f) ||
            monster->catch_state == 1 || monster->no_hit_time > 0 ||
            (monster->attrib & MONSTER_ATTRIB_NO_BODY_HIT) != 0) {
            continue;
        }
        body_no = 0;
        body = monster->GetEntryObjectPos(4, body_no, body_position);
        while (body != NULL) {
            if (body->enable != 0 &&
                next_position[1] + body->unk_04 >= body_position[1] - body_height &&
                next_position[1] - body->unk_04 <= body_position[1] + body_height) {
                body_position[1] = 0.0f;
                separation = 2.0f * body->unk_04 + 2.0f * body_width;
                distance = mgDistVector(body_position, flat_position);
                if (distance < separation) {
                    separation -= distance;
                    push_direction[0] = flat_position[0] - body_position[0];
                    push_direction[1] = flat_position[1] - body_position[1];
                    push_direction[2] = flat_position[2] - body_position[2];
                    push_direction[3] = 1.0f;
                    sceVu0Normalize(push_direction, push_direction);
                    out_velocity[0] = (out_velocity[0] + push_direction[0] * separation) / 2.0f;
                    out_velocity[1] = (out_velocity[1] + push_direction[1] * separation) / 2.0f;
                    out_velocity[2] = (out_velocity[2] + push_direction[2] * separation) / 2.0f;
                    out_velocity[3] = 1.0f;
                    sceVu0ScaleVector(out_velocity, out_velocity, 1.5f);
                }
            }
            body_no++;
            body = monster->GetEntryObjectPos(4, body_no, body_position);
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", CollisionCheck__12CActionCharaFPfPfPf);
#endif

void CActionChara::RockOn() {
    sceVu0FVECTOR  position;
    sceVu0FVECTOR  target_position;
    sceVu0FVECTOR  target_direction;
    sceVu0FVECTOR  move_direction;
    DNG_BATTLE_AREA *battle_area;
    CActiveMonster *monster;
    int            next_target;

    GetPosition(position);
    battle_area = &nowScene__2->battle_area;
    target_dot = 0.0f;
    if (lock_on != 0) {
        monster = (CActiveMonster *)nowScene__2->GetCharacter(target_no);
        if (monster != NULL) {
            monster->GetEntryObjectPos(0, 0, target_position);
            sceVu0SubVector(target_direction, target_position, position);
            sceVu0Normalize(target_direction, target_direction);
            sceVu0Normalize(move_direction, velocity);
            target_dot = sceVu0InnerProduct(target_direction, move_direction);
        }
    }
    if (PadCtrl.Btn(0x34) != 0) {
        if (lock_on != 0) {
            if (target_dot < -0.2f) {
                sndSePlay(SystemSND_ID, 0x1B, 0);
                lock_on = 0;
                return;
            }
            if (battle_area->unk_9e == 2) {
                if (target_no < 0) {
                    target_no = 24;
                } else {
                    target_no++;
                }
                if (target_no >= 48) {
                    target_no = 24;
                }
                target_no = RockOn_TargetSel(nowScene__2, target_no);
                return;
            }
            monster = (CActiveMonster *)nowScene__2->GetCharacter(target_no);
            if (monster != NULL) {
                if (ActiveMonster->GetPriorityLevelIndex(monster->priority + 1, &next_target) != NULL) {
                    sndSePlay(SystemSND_ID, 0x1A, 0);
                    target_no = next_target;
                    return;
                }
                target_no = 24;
            }
        } else if (target_no != -1) {
            sndSePlay(SystemSND_ID, 0x1A, 0);
            lock_on = 1;
        }
    }
}

#ifdef NONMATCHING
int CActionChara::HumanMoveIF() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR stick_vector = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FVECTOR target_position;
    CActionChara *target;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         move_speed;
    float         acceleration_step;
    float         acceleration;
    float         stick_direction;
    float         angle_change;
    float         turn_penalty;
    float         relative_angle;
    float         facing;
    float         motion_speed;
    float         abs_x;
    float         abs_z;
    s8            boss;

    GetPosition(position);
    GetRotation(rotation);
    sceVu0CopyVector(move_velocity, velocity);
    GetPosition(old_pos);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    if (GetBattleCharaInfo()->GetAttr() & 0x2) {
        move_x *= 0.5f;
        move_z *= 0.5f;
    }
    stick_vector[0] = move_x;
    stick_vector[2] = move_z;
    move_speed = 3.0f;
    if (lock_on != 0) {
        move_speed = 1.8f;
    }
    acceleration_step = move_speed / 8.0f;
    if (acceleration_step > 0.5f) {
        acceleration_step = 0.5f;
    }
    acceleration = move_accel + acceleration_step;
    if (acceleration > 1.0f) {
        acceleration = 1.0f;
    }
    move_accel = acceleration;
    stick_direction = atan2f(move_x, move_z);
    angle_change = old_angle - stick_direction;
    if (angle_change > 3.1415927f) {
        angle_change -= 6.2831855f;
    }
    if (angle_change < -3.1415927f) {
        angle_change += 6.2831855f;
    }
    if (angle_change < 0.0f) {
        angle_change = -angle_change;
    }
    old_angle = stick_direction;
    turn_penalty = 1.5f * (angle_change / 3.1415927f);
    if (turn_penalty > 1.0f) {
        turn_penalty = 1.0f;
    }
    acceleration -= turn_penalty;
    if (acceleration < 0.0f) {
        acceleration = 0.0f;
    }
    move_accel = acceleration;
    move_velocity[0] = acceleration * ((float)mgFrameRate * (move_x * move_speed));
    move_velocity[2] = acceleration * ((float)mgFrameRate * (move_z * move_speed));
    relative_angle = atan2f(move_x, move_z) - rotation[1];
    if (relative_angle > 3.1415927f) {
        relative_angle -= 6.2831855f;
    }
    if (relative_angle < -3.1415927f) {
        relative_angle += 6.2831855f;
    }
    angle_change = stick_angle - relative_angle;
    if (angle_change > 3.1415927f) {
        angle_change -= 6.2831855f;
    }
    if (angle_change < -3.1415927f) {
        angle_change += 6.2831855f;
    }
    if (angle_change < 0.0f) {
        angle_change = -angle_change;
    }
    if (angle_change / 3.1415927f < 0.3f) {
        stick_time++;
    } else {
        stick_time = 0;
        stick_angle = relative_angle;
    }
    if (GamePad__2.On(PAD_L2) != 0 && DebugInfo.chara_move > 0) {
        move_velocity[0] *= 2.0f;
        move_velocity[2] *= 2.0f;
    }
    target = NULL;
    boss = 0;
    if (lock_on != 0) {
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL) {
            boss = ((CActiveMonster *)target)->tbl->boss;
        }
    }
    if (move_x != 0.0f || move_z != 0.0f) {
        stand_flag = 0;
    } else {
        stand_flag = 1;
    }
    if (lock_on != 0 && boss == 0) {
        if (unk_75e != 0) {
            SetMotion("\x83o\x83g\x83\x8B\x97\xA7\x82\xBF", 0, 1);
        } else {
            SetMotion("\x97\xA7\x82\xBF", 0, 1);
        }
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetEntryObjectPos(0, 0, target_position);
            facing = unitRotation(CObjectFrame::frame, atan2f(target_position[0] - position[0], target_position[2] - position[2]), 5.0f);
            SetRotation(0.0f, facing, 0.0f);
            if (move_x != 0.0f || move_z != 0.0f) {
                relative_angle = facing - atan2f(move_x, move_z);
                if (relative_angle < -3.1415927f) {
                    relative_angle += 6.2831855f;
                }
                if (relative_angle > 3.1415927f) {
                    relative_angle -= 6.2831855f;
                }
                abs_x = move_x;
                if (abs_x < 0.0f) {
                    abs_x = -abs_x;
                }
                abs_z = move_z;
                if (abs_z < 0.0f) {
                    abs_z = -abs_z;
                }
                motion_speed = abs_x;
                if (abs_x <= abs_z) {
                    motion_speed = abs_z;
                }
                if (relative_angle > -1.0f && relative_angle < 1.0f) {
                    SetMotion("\x83o\x83g\x83\x8B\x95\xE0\x82\xAB\x81i\x91O\x81j", 0, 1);
                }
                if (relative_angle < -2.4f || relative_angle > 2.4f) {
                    SetMotion("\x83o\x83g\x83\x8B\x95\xE0\x82\xAB\x81i\x8C\xE3\x81j", 0, 1);
                }
                if (relative_angle > 1.0f && relative_angle < 2.4f) {
                    SetMotion("\x83o\x83g\x83\x8B\x95\xE0\x82\xAB\x81i\x89" "E\x81j", 0, 1);
                }
                if (relative_angle < -1.0f && relative_angle > -2.4f) {
                    SetMotion("\x83o\x83g\x83\x8B\x95\xE0\x82\xAB\x81i\x8D\xB6\x81j", 0, 1);
                }
                if (motion_speed > 0.6f) {
                    motion_speed = 0.6f;
                }
                SetStep(0.5f * motion_speed);
            }
        }
    } else if (move_x != 0.0f || move_z != 0.0f) {
        SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
        motion_speed = mgDistVector(stick_vector);
        if (motion_speed > 1.0f) {
            motion_speed = 1.0f;
        }
        if (motion_speed >= 0.65f) {
            SetMotion("\x91\x96\x82\xE8", 0, 1);
        } else {
            SetMotion("\x95\xE0\x82\xAB", 0, 1);
            SetStep(motion_speed);
        }
    } else {
        if (unk_75e != 0 || boss != 0) {
            SetMotion("\x83o\x83g\x83\x8B\x97\xA7\x82\xBF", 0, 1);
        } else {
            SetMotion("\x97\xA7\x82\xBF", 0, 1);
        }
        if (target != NULL && boss != 0 && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetEntryObjectPos(0, 0, target_position);
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(target_position[0] - position[0], target_position[2] - position[2]), 5.0f), 0.0f);
        }
    }
    menu_flag = 1;
    if (GamePad__2.Down(PAD_RIGHT) != 0) {
        sndSePlay(SystemSND_ID, 0, 0);
        if (DngStatus.active_item == 2) {
            DngStatus.active_item = 0;
        } else {
            DngStatus.active_item++;
        }
    }
    if (GamePad__2.Down(PAD_LEFT) != 0) {
        sndSePlay(SystemSND_ID, 0, 0);
        if (DngStatus.active_item == 0) {
            DngStatus.active_item = 2;
        } else {
            DngStatus.active_item--;
        }
    }
    if (move_check.landed == 0) {
        menu_flag = 0;
        if (move_velocity[1] <= -3.5f) {
            SetMotion("\x97\x8E\x89\xBA\x92\x86", 0, 1);
        }
    }
    if (DebugInfo.chara_move >= 2) {
        if (GamePad__2.Down(PAD_SQUARE) != 0) {
            move_velocity[1] = 8.0f;
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", HumanMoveIF__12CActionCharaFv);
#endif

#ifdef NONMATCHING
int CActionChara::HumanShrowMoveIF() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR target_position;
    CActionChara *target;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         motion_speed;
    float         abs_x;
    float         abs_z;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    if (GetBattleCharaInfo()->GetAttr() & 0x2) {
        move_x *= 0.5f;
        move_z *= 0.5f;
    }
    move_x *= 0.8f;
    move_z *= 0.8f;
    move_velocity[0] = 2.0f * move_x * (float)mgFrameRate;
    move_velocity[2] = 2.0f * move_z * (float)mgFrameRate;
    if (move_x != 0.0f || move_z != 0.0f) {
        stand_flag = 0;
    } else {
        stand_flag = 1;
    }
    SetMotion("\x8E\x9D\x82\xBF\x8F\xE3\x82\xB0\x92\xE2\x8E~", 0, 1);
    if (move_x != 0.0f || move_z != 0.0f) {
        SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
        abs_x = move_x;
        if (abs_x < 0.0f) {
            abs_x = -abs_x;
        }
        abs_z = move_z;
        if (abs_z < 0.0f) {
            abs_z = -abs_z;
        }
        motion_speed = abs_x;
        if (abs_x <= abs_z) {
            motion_speed = abs_z;
        }
        if (motion_speed > 0.5f) {
            motion_speed = 0.5f;
        }
        SetMotion("\x8E\x9D\x82\xBF\x8F\xE3\x82\xB0\x95\xE0\x82\xAB", 0, 1);
        SetStep(0.5f * motion_speed);
    } else if (lock_on != 0) {
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetEntryObjectPos(0, 0, target_position);
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(target_position[0] - position[0], target_position[2] - position[2]), 5.0f), 0.0f);
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", HumanShrowMoveIF__12CActionCharaFv);
#endif

int CActionChara::HumanTameMoveIF() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    move_x *= 0.4f;
    move_z *= 0.4f;
    move_velocity[0] = 2.0f * move_x * (float)mgFrameRate;
    move_velocity[2] = 2.0f * move_z * (float)mgFrameRate;
    if (move_x != 0.0f || move_z != 0.0f) {
        stand_flag = 0;
    } else {
        stand_flag = 1;
    }
    SetMotion("\x82\xBD\x82\xDF\x83\x8B\x81[\x83v", 0, 1);
    if (move_x != 0.0f || move_z != 0.0f) {
        SetMotion("\x82\xBD\x82\xDF\x88\xDA\x93\xAE", 0, 1);
    }
    sceVu0CopyVector(velocity, move_velocity);
    RockOn();
    return 1;
}

#ifdef NONMATCHING
int CActionChara::HumanGunMoveIF(char *stand_motion, char *move_motion) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR target_position;
    CActionChara *target;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    move_x *= 0.2f;
    move_z *= 0.2f;
    move_velocity[0] = 2.0f * move_x * (float)mgFrameRate;
    move_velocity[2] = 2.0f * move_z * (float)mgFrameRate;
    SetMotion(stand_motion, 0, 1);
    if (move_x != 0.0f || move_z != 0.0f) {
        stand_flag = 0;
    } else {
        stand_flag = 1;
    }
    if (lock_on != 0) {
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetEntryObjectPos(0, 0, target_position);
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(target_position[0] - position[0], target_position[2] - position[2]), 5.0f), 0.0f);
            if (move_x != 0.0f || move_z != 0.0f) {
                SetMotion(move_motion, 0, 1);
            }
        }
    } else if (move_x != 0.0f || move_z != 0.0f) {
        SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
        SetMotion(move_motion, 0, 1);
    }
    sceVu0CopyVector(velocity, move_velocity);
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", HumanGunMoveIF__12CActionCharaFPcPc);
#endif

#ifdef NONMATCHING
int CActionChara::RoboWalkMoveIF(int mode) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR movement;
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR rotation;
    CActionChara *target;
    CActionChara *arm;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         motion_speed;
    float         target_angle;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    move_x *= 2.0f * (float)mgFrameRate;
    move_z *= 2.0f * (float)mgFrameRate;
    move_velocity[0] = move_x;
    move_velocity[2] = move_z;
    stand_flag = 0;
    if (lock_on != 0) {
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x91\xAB", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x91\xAB", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x91\xAB", 0, mode);
        }
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetPosition(target_position);
            target_angle = atan2f(target_position[0] - position[0], target_position[2] - position[2]);
            GetRotation(rotation);
            target_angle -= rotation[1];
            if (target_angle < -3.1415927f) {
                target_angle += 6.2831855f;
            }
            if (target_angle > 3.1415927f) {
                target_angle -= 6.2831855f;
            }
            arm = SearchChara("arm");
            if (arm != NULL) {
                arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, target_angle, 3.0f), 0.0f);
            }
        }
    } else {
        arm = SearchChara("arm");
        if (arm != NULL) {
            arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, 0.0f, 16.0f), 0.0f);
        }
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 10.0f), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x91\xAB", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x91\xAB", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x91\xAB", 0, mode);
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    menu_flag = 1;
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", RoboWalkMoveIF__12CActionCharaFi);
#endif

#ifdef NONMATCHING
int CActionChara::RoboTankMoveIF(int mode) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR movement;
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR rotation;
    CActionChara *target;
    CActionChara *arm;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         motion_speed;
    float         target_angle;
    sceVu0FVECTOR wheel_rotation;
    mgCFrame     *wheel;
    float         wheel_step;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    move_x *= 2.0f * (float)mgFrameRate;
    move_z *= 2.0f * (float)mgFrameRate;
    if (move_type == ACTION_MOVE_ROBO_TANK) {
        loop_se->SeLoopPlayStop(se_bank, 14, 3, 12);
        if (move_x != 0.0f || move_z != 0.0f) {
            foot_effect_wait = 6;
        }
    }
    if (move_type == ACTION_MOVE_ROBO_TANK2) {
        move_x *= 1.3f;
        move_z *= 1.3f;
    }
    move_velocity[0] = move_x;
    move_velocity[2] = move_z;
    stand_flag = 0;
    if (lock_on != 0) {
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
        }
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetPosition(target_position);
            target_angle = atan2f(target_position[0] - position[0], target_position[2] - position[2]);
            GetRotation(rotation);
            target_angle -= rotation[1];
            if (target_angle < -3.1415927f) {
                target_angle += 6.2831855f;
            }
            if (target_angle > 3.1415927f) {
                target_angle -= 6.2831855f;
            }
            arm = SearchChara("arm");
            if (arm != NULL) {
                arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, target_angle, 3.0f), 0.0f);
            }
        }
    } else {
        arm = SearchChara("arm");
        if (arm != NULL) {
            arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, 0.0f, 16.0f), 0.0f);
        }
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 10.0f), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
        }
    }
    if (move_type == ACTION_MOVE_ROBO_TANK2) {
        wheel_step = 0.034906585f * -mgDistVector(move_velocity);
        wheel = SearchObject("rf_tire");
        if (wheel != NULL) {
            wheel->SetRotType(2);
            wheel->GetRotation(wheel_rotation);
            wheel_rotation[2] += wheel_step;
            wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
            wheel->SetRotation(wheel_rotation);
        }
        wheel = SearchObject("rb_tire");
        if (wheel != NULL) {
            wheel->SetRotType(2);
            wheel->GetRotation(wheel_rotation);
            wheel_rotation[2] += wheel_step;
            wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
            wheel->SetRotation(wheel_rotation);
        }
        wheel = SearchObject("lf_tire");
        if (wheel != NULL) {
            wheel->SetRotType(2);
            wheel->GetRotation(wheel_rotation);
            wheel_rotation[2] += wheel_step;
            wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
            wheel->SetRotation(wheel_rotation);
        }
        wheel = SearchObject("lb_tire");
        if (wheel != NULL) {
            wheel->SetRotType(2);
            wheel->GetRotation(wheel_rotation);
            wheel_rotation[2] += wheel_step;
            wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
            wheel->SetRotation(wheel_rotation);
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    menu_flag = 1;
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", RoboTankMoveIF__12CActionCharaFi);
#endif

#ifdef NONMATCHING
int CActionChara::RoboBikeMoveIF(int mode) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR leg_rotation;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR movement;
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR direction = { 0.0f, 0.0f, 1.0f, 1.0f };
    sceVu0FMATRIX matrix;
    CActionChara *leg;
    CActionChara *arm;
    CActionChara *target;
    CMap         *map;
    float         stick_x;
    float         stick_y;
    float         steering;
    float         target_angle;
    int           poly_count;
    CCPoly        polys[128];
    mgVu0FBOX     box;
    sceVu0FVECTOR wheel_rotation;
    sceVu0FVECTOR front_wheel_position;
    sceVu0FVECTOR back_wheel_position;
    sceVu0FVECTOR front_hit;
    sceVu0FVECTOR back_hit;
    sceVu0FVECTOR slope_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FVECTOR old_velocity;
    mgCFrame     *wheel;
    float         slope_angle;

    leg = SearchChara("leg");
    if (leg == NULL) {
        return 0;
    }
    GetPosition(position);
    leg->GetRotation(leg_rotation);
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    stand_flag = 0;
    if (move_check.width_result & 0x8) {
        if (accele.speed != 0.0f) {
            if (accele.speed < 0.0f) {
                accele.speed += 0.1f;
                if (accele.speed > 0.0f) {
                    accele.speed = 0.0f;
                }
            }
            if (accele.speed > 0.0f) {
                accele.speed -= 0.1f;
                if (accele.speed < 0.0f) {
                    accele.speed = 0.0f;
                }
            }
        }
    }
    if (stick_y < 0.0f) {
        accele.speed += -0.2f * stick_y;
    }
    if (stick_y > 0.0f) {
        accele.speed += -0.15f * stick_y;
    }
    if (accele.speed > 12.0f) {
        accele.speed = 12.0f;
    }
    if (accele.speed < -3.0f) {
        accele.speed = -3.0f;
    }
    if (accele.speed > 0.2f) {
        foot_effect_wait = 2;
        loop_se->SeLoopPlayStop(se_bank, 16, 3, 12);
    }
    if (GamePad__2.On(0x4) != 0) {
        steering = leg_rotation[1] + 0.8f * (0.034906585f * -stick_x * accele.speed);
    } else {
        steering = leg_rotation[1] + 0.2f * (0.034906585f * -stick_x * accele.speed);
    }
    leg_rotation[1] = steering;
    leg_rotation[1] = mgAngleLimit(leg_rotation[1]);
    leg->SetRotation(leg_rotation);
    if (leg != NULL) {
        leg->GetRotation(rotation);
        sceVu0UnitMatrix(matrix);
        sceVu0RotMatrixY(matrix, matrix, rotation[1]);
        sceVu0ApplyMatrix(movement, matrix, direction);
    } else {
        sceVu0CopyVector(movement, front_vec);
    }
    sceVu0ScaleVector(movement, movement, accele.speed);
    if (accele.speed != 0.0f) {
        SetMotion("\x95\xE0\x82\xAB-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
        if (stick_x > 0.5f) {
            SetMotion("\x89" "E\x90\xF9\x89\xF1", 0, mode);
        }
        if (stick_x < -0.5f) {
            SetMotion("\x8D\xB6\x90\xF9\x89\xF1", 0, mode);
        }
    } else {
        SetMotion("\x97\xA7\x82\xBF-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
    }
    if (lock_on != 0) {
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetPosition(target_position);
            target_angle = atan2f(target_position[0] - position[0], target_position[2] - position[2]);
            GetRotation(rotation);
            target_angle -= rotation[1];
            if (target_angle < -3.1415927f) {
                target_angle += 6.2831855f;
            }
            if (target_angle > 3.1415927f) {
                target_angle -= 6.2831855f;
            }
            arm = SearchChara("arm");
            if (arm != NULL) {
                arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, target_angle, 3.0f), 0.0f);
            }
        }
    } else {
        arm = SearchChara("arm");
        if (arm != NULL) {
            arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, 0.0f, 8.0f), 0.0f);
        }
    }
    map = nowScene__2->GetMap(nowScene__2->active_map);

    box.max[0] = 50.0f + position[0];
    box.min[0] = position[0] - 50.0f;
    box.max[1] = 50.0f + position[1];
    box.min[1] = position[1] - 50.0f;
    box.max[2] = 50.0f + position[2];
    box.min[2] = position[2] - 50.0f;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    poly_count = map->GetColPoly(polys, box, 128);
    wheel = SearchObject("f_tire");
    if (wheel != NULL) {
        wheel->SetRotType(2);
        wheel->GetRotation(wheel_rotation);
        wheel_rotation[2] += 0.034906585f * -accele.speed;
        wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
        wheel->SetRotation(wheel_rotation);
        wheel->GetWorldPosition0(front_wheel_position);
        front_wheel_position[1] = 50.0f + position[1];
        if (CheckHitVertical(polys, poly_count, front_wheel_position, -100.0f, front_hit, 1) < 0) {
            sceVu0CopyVector(front_hit, front_wheel_position);
            front_hit[1] = position[1] - 10.0f;
        }
        if (position[1] - front_hit[1] > 20.0f) {
            front_hit[1] = position[1] - 20.0f;
        }
    }
    wheel = SearchObject("b_tire");
    if (wheel != NULL) {
        wheel->SetRotType(2);
        wheel->GetRotation(wheel_rotation);
        wheel_rotation[2] += 0.034906585f * -accele.speed;
        wheel_rotation[2] = mgAngleLimit(wheel_rotation[2]);
        wheel->SetRotation(wheel_rotation);
        wheel->GetWorldPosition0(back_wheel_position);
        back_wheel_position[1] = 50.0f + position[1];
        if (CheckHitVertical(polys, poly_count, back_wheel_position, -100.0f, back_hit, 1) < 0) {
            sceVu0CopyVector(back_hit, back_wheel_position);
            back_hit[1] = position[1] - 10.0f;
        }
        if (position[1] - back_hit[1] > 20.0f) {
            back_hit[1] = position[1] - 20.0f;
        }
    }
    sceVu0SubVector(front_hit, back_hit, front_hit);
    sceVu0CopyVector(back_hit, front_hit);
    back_hit[3] = 1.0f;
    back_hit[1] = 0.0f;
    slope_angle = atan2f(front_hit[1], mgDistVector(back_hit));
    wheel = SearchObject("katamuki");
    if (wheel != NULL) {
        slope_rotation[0] = slope_angle;
        wheel->SetRotType(2);
        wheel->SetRotation(slope_rotation);
    }
    sceVu0CopyVector(old_velocity, velocity);
    movement[1] = old_velocity[1];
    sceVu0CopyVector(velocity, movement);
    menu_flag = 1;
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", RoboBikeMoveIF__12CActionCharaFi);
#endif

#ifdef NONMATCHING
int CActionChara::RoboAirMoveIF(int unk, int mode) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR movement;
    sceVu0FVECTOR target_position;
    sceVu0FVECTOR rotation;
    CActionChara *target;
    CActionChara *arm;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         motion_speed;
    float         target_angle;
    sceVu0FVECTOR propeller_rotation;
    mgCFrame     *propeller;
    float         turn_speed;
    float         speed_limit;

    GetPosition(position);
    sceVu0CopyVector(move_velocity, velocity);
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    stand_flag = 0;
    turn_speed = 8.0f;
    speed_limit = 7.0f;
    accele.accele[0] += move_x;
    accele.accele[2] += move_z;
    if (move_type == ACTION_MOVE_ROBO_AIR2) {
        speed_limit = 8.0f;
        turn_speed = 4.0f;
    }
    if (accele.accele[0] > speed_limit) {
        accele.accele[0] = speed_limit;
    }
    if (accele.accele[0] < -speed_limit) {
        accele.accele[0] = -speed_limit;
    }
    if (accele.accele[2] > speed_limit) {
        accele.accele[2] = speed_limit;
    }
    if (accele.accele[2] < -speed_limit) {
        accele.accele[2] = -speed_limit;
    }
    move_x = accele.accele[0];
    move_z = accele.accele[2];
    move_velocity[0] = move_x;
    move_velocity[2] = move_z;
    accele.accele[0] *= 0.96f;
    accele.accele[2] *= 0.96f;
    if (move_type == ACTION_MOVE_ROBO_AIR) {
        foot_effect_wait = 2;
        loop_se->SeLoopPlayStop(se_bank, 20, 3, 12);
        propeller = SearchObject("prop1");
        if (propeller != NULL) {
            propeller->GetRotation(propeller_rotation);
            propeller_rotation[1] -= 0.5067085f;
            propeller_rotation[1] = mgAngleLimit(propeller_rotation[1]);
            propeller->SetRotation(propeller_rotation);
        }
        propeller = SearchObject("prop2");
        if (propeller != NULL) {
            propeller->GetRotation(propeller_rotation);
            propeller_rotation[1] += 0.49087387f;
            propeller_rotation[1] = mgAngleLimit(propeller_rotation[1]);
            propeller->SetRotation(propeller_rotation);
        }
    }
    if (move_type == ACTION_MOVE_ROBO_AIR2) {
        foot_effect_wait = 2;
        loop_se->SeLoopPlayStop(se_bank, 21, 3, 12);
    }
    if (lock_on != 0) {
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), turn_speed), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
        }
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetPosition(target_position);
            target_angle = atan2f(target_position[0] - position[0], target_position[2] - position[2]);
            GetRotation(rotation);
            target_angle -= rotation[1];
            if (target_angle < -3.1415927f) {
                target_angle += 6.2831855f;
            }
            if (target_angle > 3.1415927f) {
                target_angle -= 6.2831855f;
            }
            arm = SearchChara("arm");
            if (arm != NULL) {
                arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, target_angle, 3.0f), 0.0f);
            }
        }
    } else {
        arm = SearchChara("arm");
        if (arm != NULL) {
            arm->SetRotation(0.0f, unitRotation(arm->CObjectFrame::frame, 0.0f, 16.0f), 0.0f);
        }
        if (move_x != 0.0f || move_z != 0.0f) {
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), turn_speed), 0.0f);
            movement[0] = move_x;
            movement[1] = 0.0f;
            movement[2] = move_z;
            motion_speed = mgDistVector(movement) / max_speed;
            if (motion_speed > 1.0f) {
                motion_speed = 1.0f;
            }
            if (motion_speed >= 0.8f) {
                SetMotion("\x91\x96\x82\xE8-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            } else {
                SetMotion("\x95\xE0\x82\xAB-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
            }
        } else {
            SetMotion("\x97\xA7\x82\xBF-\x83L\x83\x83\x83^\x83s\x83\x89", 0, mode);
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    menu_flag = 1;
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", RoboAirMoveIF__12CActionCharaFii);
#endif

#ifdef NONMATCHING
int CActionChara::MonsterMoveIF() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR move_velocity;
    sceVu0FVECTOR stick_vector = { 0.0f, 0.0f, 0.0f, 1.0f };
    sceVu0FVECTOR target_position;
    CActionChara *target;
    float         camera_angle;
    float         stick_x;
    float         stick_y;
    float         move_x;
    float         move_z;
    float         move_speed;
    float         acceleration_step;
    float         acceleration;
    float         stick_direction;
    float         angle_change;
    float         turn_penalty;
    float         relative_angle;
    float         motion_speed;
    float         abs_x;
    float         abs_z;

    GetPosition(position);
    GetRotation(rotation);
    sceVu0CopyVector(move_velocity, velocity);
    GetPosition(old_pos);
    stand_flag = 0;
    camera_angle = action_info.camera->GetAngle();
    stick_x = GamePad__2.GetLXf();
    stick_y = GamePad__2.GetLYf();
    move_x = stick_x * cosf(camera_angle) + stick_y * sinf(camera_angle);
    move_z = -stick_x * sinf(camera_angle) + stick_y * cosf(camera_angle);
    if (GetBattleCharaInfo()->GetAttr() & 0x2) {
        move_x *= 0.5f;
        move_z *= 0.5f;
    }
    stick_vector[0] = move_x;
    stick_vector[2] = move_z;
    move_speed = accele.move_speed;
    if (lock_on != 0) {
        move_speed *= 0.6f;
    }
    acceleration_step = move_speed / 8.0f;
    if (acceleration_step > 0.5f) {
        acceleration_step = 0.5f;
    }
    acceleration = move_accel + acceleration_step;
    if (acceleration > 1.0f) {
        acceleration = 1.0f;
    }
    move_accel = acceleration;
    stick_direction = atan2f(move_x, move_z);
    angle_change = old_angle - stick_direction;
    if (angle_change > 3.1415927f) {
        angle_change -= 6.2831855f;
    }
    if (angle_change < -3.1415927f) {
        angle_change += 6.2831855f;
    }
    if (angle_change < 0.0f) {
        angle_change = -angle_change;
    }
    old_angle = stick_direction;
    turn_penalty = 1.5f * (angle_change / 3.1415927f);
    if (turn_penalty > 1.0f) {
        turn_penalty = 1.0f;
    }
    acceleration -= turn_penalty;
    if (acceleration < 0.0f) {
        acceleration = 0.0f;
    }
    move_accel = acceleration;
    move_velocity[0] = acceleration * ((float)mgFrameRate * (move_x * move_speed));
    move_velocity[2] = acceleration * ((float)mgFrameRate * (move_z * move_speed));
    relative_angle = atan2f(move_x, move_z) - rotation[1];
    if (relative_angle > 3.1415927f) {
        relative_angle -= 6.2831855f;
    }
    if (relative_angle < -3.1415927f) {
        relative_angle += 6.2831855f;
    }
    angle_change = stick_angle - relative_angle;
    if (angle_change > 3.1415927f) {
        angle_change -= 6.2831855f;
    }
    if (angle_change < -3.1415927f) {
        angle_change += 6.2831855f;
    }
    if (angle_change < 0.0f) {
        angle_change = -angle_change;
    }
    if (angle_change / 3.1415927f < 0.3f) {
        stick_time++;
    } else {
        stick_time = 0;
        stick_angle = relative_angle;
    }
    if (GamePad__2.On(PAD_L2) != 0 && DebugInfo.chara_move > 0) {
        move_velocity[0] *= 2.0f;
        move_velocity[2] *= 2.0f;
    }
    if (move_x != 0.0f || move_z != 0.0f) {
        SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(move_x, move_z), 5.0f), 0.0f);
        motion_speed = mgDistVector(stick_vector);
        if (motion_speed > 1.0f) {
            motion_speed = 1.0f;
        }
        if (motion_speed < 0.45f) {
            motion_speed = 0.45f;
        }
        if (motion_speed >= 0.65f) {
            SetMotion("\x91\x96\x82\xE8", 0, 1);
        } else {
            SetMotion("\x95\xE0\x82\xAB", 0, 1);
            SetStep(motion_speed);
        }
    } else if (lock_on != 0) {
        SetMotion("\x97\xA7\x82\xBF", 0, 1);
        target = (CActionChara *)nowScene__2->GetCharacter(target_no);
        if (target != NULL && target->chara_kind == ACTION_KIND_SCRIPT) {
            target->GetEntryObjectPos(0, 0, target_position);
            SetRotation(0.0f, unitRotation(CObjectFrame::frame, atan2f(target_position[0] - position[0], target_position[2] - position[2]), 5.0f), 0.0f);
        }
    } else {
        SetMotion("\x97\xA7\x82\xBF", 0, 1);
    }
    if (DebugInfo.chara_move >= 2) {
        if (GamePad__2.Down(PAD_SQUARE) != 0) {
            move_velocity[1] = 8.0f;
        }
    }
    sceVu0CopyVector(velocity, move_velocity);
    menu_flag = 1;
    RockOn();
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", MonsterMoveIF__12CActionCharaFv);
#endif

/**
 * Places the sparks, flash and script effect of a guarded hit in front of the camera.
 */
#ifdef NONMATCHING
static void GuardEffectSet(CScene *scene, float *point) {
    float to_camera[4];
    float position[4];
    mgCCamera *camera;
    CHitEffectImage *hit;
    CFlushEffect *flush;

    camera = scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    sceVu0CopyVector(position, point);
    camera->GetPos(to_camera);
    sceVu0SubVector(to_camera, to_camera, position);
    sceVu0Normalize(to_camera, to_camera);
    sceVu0ScaleVector(to_camera, to_camera, 20.0f);
    sceVu0AddVector(position, position, to_camera);
    sceVu0FVECTOR spark_direction = { 0.0f, 1.0f, 0.0f, 1.0f };
    if (BattleFX.hit == NULL) {
        hit = NULL;
    } else {
        hit = BattleFX.hit + BattleFX.hit_next;
        BattleFX.hit_next++;
        if (BattleFX.hit_next >= BattleFX.hit_num) {
            BattleFX.hit_next = 0;
        }
    }
    float spread = 50.0f;
    float zero = 0.0f;
    hit->SethitEffect(position, spark_direction, spread, 30.0f, zero, 0.1f, 30, 32);
    hit->kind = HIT_EFFECT_SPARK_SHORT;
    if (BattleFX.flush == NULL) {
        flush = NULL;
    } else {
        flush = BattleFX.flush + BattleFX.flush_next;
        BattleFX.flush_next++;
        if (BattleFX.flush_next >= BattleFX.flush_num) {
            BattleFX.flush_next = 0;
        }
    }
    if (flush != NULL) {
        sceVu0CopyVector(flush->pos, position);
        flush->fade_speed = 16.0f;
        flush->alpha = 160;
        flush->active = 1;
        flush->size = 10.0f;
        flush->grow = 3.0f;
        flush->tex_u = 65;
        flush->tex_v = 193;
        flush->tex_size = 62;
        flush->follow = NULL;
    }
    if (FxScriptMan != NULL) {
        FxScriptMan->CreateEffSpt("\x83\x4B\x81\x5B\x83\x68\x83\x47\x83\x74\x83\x46\x83\x4E\x83\x67\x82\x60", 0, 0);
        FxScriptMan->SetScriptVect1(position, 0, -1);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", GuardEffectSet__FP6CScenePf);
#endif

/**
 * Places the sparks and flash of a damaging hit in front of the camera.
 */
#ifdef NONMATCHING
static void HitEffectSet(CScene *scene, float *pos) {
    sceVu0FVECTOR    effect_position;
    sceVu0FVECTOR    camera_direction;
    sceVu0FVECTOR    hit_position;
    sceVu0FVECTOR    spark_direction = { 0.0f, 1.0f, 0.0f, 1.0f };
    mgCCamera      *camera;
    CHitEffectImage *hit;
    CFlushEffect   *flush;

    camera = scene->GetCamera(scene->active_camera);
    if (camera != NULL) {
        sceVu0CopyVector(hit_position, pos);
        camera->GetPos(camera_direction);
        sceVu0SubVector(camera_direction, camera_direction, hit_position);
        sceVu0Normalize(camera_direction, camera_direction);
        sceVu0ScaleVector(camera_direction, camera_direction, 20.0f);
        sceVu0AddVector(effect_position, hit_position, camera_direction);
        if (BattleFX.hit == NULL) {
            hit = NULL;
        } else {
            hit = &BattleFX.hit[BattleFX.hit_next++];
            if (BattleFX.hit_next >= BattleFX.hit_num) {
                BattleFX.hit_next = 0;
            }
        }
        if (hit != NULL) {
            hit->SethitEffect(effect_position, spark_direction, 30.0f, 50.0f, 0.4f, 0.1f, 30, 32);
            hit->kind = HIT_EFFECT_BOARD;
        }
        if (BattleFX.flush == NULL) {
            flush = NULL;
        } else {
            flush = &BattleFX.flush[BattleFX.flush_next++];
            if (BattleFX.flush_next >= BattleFX.flush_num) {
                BattleFX.flush_next = 0;
            }
        }
        if (flush != NULL) {
            sceVu0CopyVector(flush->pos, hit_position);
            flush->fade_speed = 20.0f;
            flush->alpha = 160;
            flush->active = 1;
            flush->size = 10.0f;
            flush->grow = 2.0f;
            flush->tex_u = 64;
            flush->tex_v = 192;
            flush->tex_size = 64;
            flush->follow = NULL;
        }
        if (BattleFX.hit == NULL) {
            hit = NULL;
        } else {
            hit = &BattleFX.hit[BattleFX.hit_next++];
            if (BattleFX.hit_next >= BattleFX.hit_num) {
                BattleFX.hit_next = 0;
            }
        }
        if (hit != NULL) {
            hit->SethitEffect(effect_position, spark_direction, 40.0f, 40.0f, 0.0f, 0.05f, 32, 32);
            hit->kind = HIT_EFFECT_BOARD;
            hit->tex_rect = mgRect<int>(0, 80, 16, 16);
            hit->sprite_size = 2.0f;
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", HitEffectSet__FP6CScenePf);
#endif

/**
 * Checks an equipped amulet, using one up on a third of the hits it prevents.
 */
static int CheckAmuletAvoid(int item_no) {
    CBattleCharaInfo *battle;
    CGameDataUsed *item;
    int index;

    battle = GetBattleCharaInfo();
    switch (battle->chr_no) {
        case 1:
        case 0:
            item = battle->GetActiveItemInfo(0);
            index = 0;
            do {
                if (item_no == item->item_no) {
                    if (iRand(100) % 3 == 0) {
                        item->DeleteNum(1);
                    }
                    return 1;
                }
                index++;
                item++;
            } while (index < 3);
            return 0;
        default:
            return 1;
    }
}

/**
 * Uses one active item of the given kind when Max or Monica has it equipped.
 */
static int CheckEquipSetItem(int item_no) {
    CBattleCharaInfo *battle;
    CGameDataUsed *item;
    int index;

    battle = GetBattleCharaInfo();
    switch (battle->chr_no) {
        case 1:
        case 0:
            item = battle->GetActiveItemInfo(0);
            index = 0;
            do {
                if (item_no == item->item_no) {
                    item->DeleteNum(1);
                    return 1;
                }
                index++;
                item++;
            } while (index < 3);
            return 0;
        default:
            return 0;
    }
}

#ifdef NONMATCHING
int CActionChara::CheckDamage() {
    sceVu0FVECTOR    position;
    DNG_BATTLE_AREA *battle_area;
    CBattleCharaInfo *battle;
    CColPrim        *hit;
    float            damage;
    u32              battle_sound;
    u32              chara_sound;
    u32              attributes;
    int              max_hp;
    int              now_hp;
    int              damage_points;
    int              guarded;
    int              immobilized;
    int              reaction;
    int              handled;
    int              element;
    int              index;
    s16              strongest;

    if (nowScene__2 == NULL) {
        return 0;
    }
    battle_area = &nowScene__2->battle_area;
    battle_sound = nowScene__2->se_battle_id;
    chara_sound = se_bank;
    battle = GetBattleCharaInfo();
    GetPosition(position);
    if (damage_time > 0) {
        return 0;
    }
    if (muteki_time > 0) {
        return 0;
    }
    handled = 0;
    hit = ColPrimMan.CheckHit(0);
    if (hit != NULL) {
        immobilized = 0;
        attributes = battle->GetAttr();
        if ((attributes & (CHARA_STATUS_UNK_8 | CHARA_STATUS_UNK_20)) != 0) {
            immobilized = 1;
        }
        max_hp = battle->GetMaxHp_i();
        now_hp = battle->GetNowHp_i();
        damage = hit->damage * (1.0f + fRand(0.15f));
        damage -= battle->GetDefenceVol();
        if (damage <= 0.0f) {
            damage = 0.0f;
        }
        if (hit->param->hit_count >= 2) {
            damage /= hit->param->hit_count;
        }
        damage += 1.0f + fRand(2.0f);
        if ((hit->status & 0x1000) != 0) {
            damage = (int)(max_hp * (0.01f * hit->damage));
            if (now_hp - damage < 0.0f) {
                damage = now_hp - 1.0f;
            }
        }
        if ((hit->status & 0x2000) != 0) {
            damage = now_hp / 2;
            if (damage <= 1.0f) {
                damage = 1.0f;
            }
        }
        if ((int)damage <= 0) {
            damage = 0.0f;
        }
        guarded = guard_flag;
        if (guarded != 0) {
            damage *= 0.01f * hit->param->critical_rate;
        }
        if (((s16)hit->param->hit_flags & 0x8) != 0) {
            guard_flag = 0;
            guarded = 0;
        }
        if (damage >= 0.0f) {
            damage = GetDispVolumeForFloat(damage);
        }
        battle->AddHp_Point(-damage, 0.0f);
        damage_points = (int)damage;
        if (damage_points > 0) {
            if ((hit->status & 0x4) != 0 && iRand(100) < 30 &&
                (attributes & CHARA_STATUS_POISON) == 0 && CheckAmuletAvoid(0x101) == 0) {
                battle->SetAttr(CHARA_STATUS_POISON, 0);
                sndSePlay(battle_sound, 0x18, 0);
            }
            if ((hit->status & 0x10000) != 0 && battle_area->unk_8c != 2 && iRand(100) < 30 &&
                (attributes & CHARA_STATUS_UNK_2) == 0 && CheckAmuletAvoid(0x100) == 0) {
                battle->SetAttrVol(CHARA_STATUS_UNK_2, 3600);
                sndSePlay(battle_sound, 0x52, 0);
            }
            if ((hit->status & 0x8000) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_UNK_20) == 0 && CheckAmuletAvoid(0xFD) == 0) {
                battle->SetAttrVol(CHARA_STATUS_UNK_20, 900);
                immobilized = 1;
                sndSePlay(battle_sound, 0x53, 0);
            }
            if ((hit->status & 0x8) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_UNK_8) == 0 && CheckAmuletAvoid(0xFE) == 0) {
                battle->SetAttrVol(CHARA_STATUS_UNK_8, 300);
                immobilized = 1;
                sndSePlay(battle_sound, 0x54, 0);
            }
            if ((hit->status & 0x20000) != 0 && iRand(100) < 50 &&
                (attributes & CHARA_STATUS_UNK_4) == 0 && CheckAmuletAvoid(0xFF) == 0) {
                battle->SetAttr(CHARA_STATUS_UNK_4, 0);
                sndSePlay(battle_sound, 0x55, 0);
            }
            if ((hit->status & 0x100000) != 0 && iRand(100) < 50) {
                battle->SetAttr(CHARA_STATUS_UNK_40, 0);
                sndSePlay(battle_sound, 0x52, 0);
            }
        }
        reaction = 2;
        if (((s16)hit->param->hit_flags & 0x2) != 0) {
            reaction = 4;
        }
        if (((s16)hit->param->hit_flags & 0x4) != 0) {
            reaction = 1;
        }
        if (guarded != 0) {
            reaction = 0;
        }
        if (immobilized != 0) {
            reaction = 3;
        }
        if (battle->GetNowHp_i() <= 0) {
            if (CheckEquipSetItem(0x111) == 0) {
                reaction = 6;
            } else {
                battle->SetHpRate(0.5f);
                pallet[0].SetAnim(96, 180, 255, 1, 45, 0);
                effect_man->CreateEffSpt("\x92\xCA\x8F\xED\x89\xF1\x95\x9C", 0, 0);
                effect_man->SetScriptTargetId(0, -1, -1);
                effect_man->SetValue(0, 0, 0, -1);
                sndSePlay(SystemSND_ID, 0xA, 0);
            }
        }
        sceVu0CopyVector(blow_vec, hit->hit_vec);
        blow_speed = 4.0f;
        blow_rate = 1.0f;
        blow_decel = 0.0f;
        blow_time = 5;
        if (guarded != 0) {
            sndSePlay(battle_sound, 0x21, 0);
            if (immobilized == 0) {
                sndSePlay(chara_sound, 0x1D, 0);
            }
            GuardEffectSet(nowScene__2, hit->hit_pos);
            if (battle->chr_no == USER_CHARA_MONICA) {
                element = -1;
                if (battle->equip->data.weapon.status[1] >= 31) {
                    strongest = 0;
                    for (index = 0; index < 4; index++) {
                        if (strongest < hit->param->element[index]) {
                            strongest = hit->param->element[index];
                            element = index;
                        }
                    }
                    if (element >= 0) {
                        battle->SetMagicSwordPow(element, hit->damage);
                    }
                }
            }
            if (damage > 0.0f) {
                HitEffectSet(nowScene__2, hit->hit_pos);
                pallet[0].SetAnim(255, 128, 128, 1, 45, 0);
                shake.time = 6;
                DamageScore2.SetValue(0, damage_points, 0.0f);
                damage_time = hit->param->stun_time * 2;
                GamePad__2.SetVibration(0, 96, 6);
            } else {
                GamePad__2.SetVibration(0, 72, 6);
            }
        } else {
            if (reaction == 4) {
                damage_time = hit->param->stun_time * 7;
                pallet[0].SetAnim(255, 128, 128, 1, 90, 0);
                shake.time = 8;
                GamePad__2.SetVibration(1, 180, 30);
            } else {
                damage_time = hit->param->stun_time * 3;
                pallet[0].SetAnim(255, 128, 128, 1, 30, 0);
                shake.time = 8;
                GamePad__2.SetVibration(1, 128, 20);
            }
            if (immobilized == 0) {
                sndSePlay(chara_sound, 0x24, 0);
            }
            sndSePlay(battle_sound, 0x16, 0);
            HitEffectSet(nowScene__2, hit->hit_pos);
            DamageScore2.SetValue(0, damage_points, body_height);
        }
        switch (reaction) {
        case 0:
        case 1:
        case 5:
            break;
        case 3:
            handled = 1;
            damage_req = ACTION_DAMAGE_REQ_HOLD;
            break;
        case 2:
            stagger += hit->param->stagger;
            stagger_time = 60;
            if (stagger >= 2 || (menu_flag != 0 && stand_flag != 0)) {
                damage_req = ACTION_DAMAGE_REQ_SMALL;
            }
            handled = 1;
            break;
        case 4:
            handled = 1;
            damage_req = ACTION_DAMAGE_REQ_LARGE;
            break;
        case 6:
            handled = 1;
            break;
        }
    }
    if (handled != 0) {
        menu_flag = 0;
    }
    return handled;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", CheckDamage__12CActionCharaFv);
#endif

int CActionChara::LoadActionFile(char *data, int size, mgCMemory *memory) {
    SetActionExtendTable();
    chara_kind = ACTION_KIND_SCRIPT;
    script_buf = (char *)memory->stAlloc64(size / 16 + 1);
    memcpy(script_buf, data, size);
    SetActionScript(&script, script_buf, memory);
    return 1;
}

void CActionChara::InitScript() {
    ResetScript();
    action_info.chara = this;
    if (script.check_program(ACTION_PROG_INIT) != 0) {
        script.run(ACTION_PROG_INIT);
    }
    prog_no = ACTION_PROG_MAIN;
}

void CActionChara::SetHold() {
    if (script.check_program(ACTION_PROG_HOLD) != 0) {
        script.run(ACTION_PROG_HOLD);
        AllDeleteDamage();
        damage_req = ACTION_DAMAGE_REQ_NONE;
        prog_no = ACTION_PROG_RUNNING;
    }
}

#ifdef NONMATCHING
void CActionChara::RunScript(CScene *scene, RUN_SCRIPT_ENV *env) {
    sceVu0FVECTOR     adjusted_velocity = { 0.0f, 0.0f, 0.0f, 0.0f };
    sceVu0FVECTOR     old_velocity;
    sceVu0FVECTOR     position;
    sceVu0FVECTOR     rotation;
    sceVu0FVECTOR     new_position;
    CCPoly            polys[128];
    mgVu0FBOX         box;
    CBattleCharaInfo *battle;
    CTreasureBoxManager *treasure;
    CSphida          *sphida;
    CMap             *map;
    ACTION_DAMAGE    *entry;
    CColPrim         *reversed;
    float             frame;
    float             target_distance;
    int               count;
    int               foot;
    int               index;
    int               pallet_no;
    int               target;

    action_info.chara = this;
    nowScene__2 = scene;
    action_info.camera = (mgCCameraFollow *)scene->GetCamera(scene->GetCameraID("MainCam"));
    action_info.env = env;
    menu_flag = 0;
    dir_gun = 0;
    action_info.chara->pad_history |= PadCtrl.Btn(0x32);
    if (PadCtrl.Btn(0x38) != 0) {
        action_info.chara->acumu_pad++;
    } else {
        action_info.chara->acumu_pad = 0;
    }
    if (unk_bec != 0) {
        prog_no = 700;
        unk_bec = 0;
    }
    if (damage_req == ACTION_DAMAGE_REQ_DEAD) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DEAD;
        damage_req = ACTION_DAMAGE_REQ_NONE;
        scene->battle_area.script.event_no = 1200;
    }
    if (damage_req == ACTION_DAMAGE_REQ_HOLD) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_HOLD;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }
    if (damage_req == ACTION_DAMAGE_REQ_SMALL) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DAMAGE_SMALL;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }
    if (damage_req == ACTION_DAMAGE_REQ_LARGE) {
        AllDeleteDamage();
        prog_no = ACTION_PROG_DAMAGE_LARGE;
        damage_req = ACTION_DAMAGE_REQ_NONE;
    }
    if (prog_no != ACTION_PROG_RUNNING) {
        if (script.check_program(prog_no) != 0) {
            script.run(prog_no);
            prog_no = ACTION_PROG_RUNNING;
        }
    } else {
        script.resume();
        if (script.end != 0) {
            prog_no = ACTION_PROG_MAIN;
        }
    }
    GetPosition(position);
    GetRotation(rotation);
    sceVu0CopyVector(old_velocity, velocity);
    CollisionCheck(position, old_velocity, adjusted_velocity);
    map = nowScene__2->GetMap(nowScene__2->active_map);
    box.max[0] = 40.0f + position[0];
    box.min[0] = position[0] - 40.0f;
    box.max[1] = 40.0f + position[1];
    box.min[1] = position[1] - 40.0f;
    box.max[2] = 40.0f + position[2];
    box.min[2] = position[2] - 40.0f;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    count = map->GetColPoly(polys, box, 128);
    treasure = scene->battle_area.treasure_box;
    if (treasure != NULL) {
        count += treasure->PickupCollision(position, &polys[count], box, 128 - count);
    }
    sphida = GetSphidaPtr();
    if (sphida != NULL) {
        count += sphida->PickupCollision(position, &polys[count], box, 128 - count);
    }
    move_check.radius = 4.0f + 2.0f * body_width;
    MoveCheck(position, adjusted_velocity, new_position, &move_check, polys, count, 1);
    adjusted_velocity[0] = new_position[0] - position[0];
    adjusted_velocity[2] = new_position[2] - position[2];
    battle = GetBattleCharaInfo();
    if (move_check.landed != 0) {
        foot = move_check.ground_poly.foot_sound;
        if (foot == 0 && map != NULL) {
            foot = map->def_foot;
        }
        foot_sound_id = foot;
        adjusted_velocity[1] = 0.0f;
    } else {
        if (battle->chr_no == USER_CHARA_ROBO) {
            adjusted_velocity[1] -= 3.0f;
            if (adjusted_velocity[1] <= -5.0f) {
                adjusted_velocity[1] = -5.0f;
            }
        } else {
            adjusted_velocity[1] -= 0.5f;
            if (adjusted_velocity[1] <= -3.5f) {
                adjusted_velocity[1] = -3.5f;
            }
        }
        foot_sound_id = -1;
    }
    if (new_position[1] < -500.0f) {
        new_position[1] = 500.0f;
    }
    SetPosition(new_position);
    sceVu0CopyVector(velocity, adjusted_velocity);
    if (hold_type == ACTION_HOLD_NONE &&
        (battle->chr_no == USER_CHARA_MAX || battle->chr_no == USER_CHARA_MONICA) &&
        move_check.landed != 0 && old_velocity[1] <= -3.5f) {
        SetMotion("\x92\x85\x92\x6E", 6, 1);
        prog_no = ACTION_PROG_LAND;
    }
    if (move_check.landed == 0) {
        menu_flag = 0;
    }
    for (index = 0; index < 11; index++) {
        entry = &damage[index];
        if (entry->use != 0) {
            frame = GetNowFrame(entry->chara);
            if (entry->prim == NULL) {
                if (frame >= entry->start_frame && frame < entry->end_frame) {
                    entry->prim = ColPrimMan.GetPrim();
                    if (entry->prim != NULL) {
                        entry->prim->SetDamage(entry->damage, 0);
                        entry->prim->SetCoord(entry->frame0, entry->frame1, entry->radius);
                        SetDamageParam(entry->prim, 0);
                        entry->prim->damage = (int)(entry->prim->damage * entry->power_rate);
                    }
                }
            } else if (frame < entry->start_frame || frame > entry->end_frame) {
                entry->prim->Delete(-1);
                entry->prim = NULL;
            } else {
                reversed = ColPrimMan.IsReversVec(entry->prim);
                if (reversed != NULL) {
                    reversed->reversed = 1;
                    reversed->GetReversVec(reversed->revers_vec);
                }
            }
        }
    }
    pallet_no = battle->GetPalletNo(0);
    if (pallet_no >= 0) {
        for (index = 0; index < 3; index++) {
            if (sword_effect[index] != NULL) {
                sword_effect[index]->SetTexture((pallet_no % 2) * 64, (pallet_no / 2) * 32, 64, 32);
            }
        }
    }
    if (scene->battle_area.unk_9e == 2) {
        target_no = RockOn_TargetSel(scene, target_no);
    }
    if (scene->battle_area.unk_9e == 0) {
        if (lock_on == 0) {
            target_no = DistCheck_Action2(scene, 0.5f, 400.0f, &target_distance, 0, NULL);
        } else {
            lock_on = Check_LockOn(scene, 300.0f, target_no);
            if (lock_on == 0) {
                target = DistCheck_Action2(scene, 0.5f, 400.0f, &target_distance, 0, NULL);
                target_no = target;
                if (target != -1) {
                    lock_on = 1;
                    return;
                }
            }
        }
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", RunScript__12CActionCharaFP6CSceneP14RUN_SCRIPT_ENV);
#endif

int CActionChara::CheckReleaseTimming(int type) {
    if (type == -1) {
        return release_timing;
    }
    if (type == hold_type) {
        return release_timing;
    }
    return 0;
}

void CActionChara::StepParam() {
    sceVu0FVECTOR   movement;
    sceVu0FVECTOR   rotation;
    sceVu0FVECTOR   arm_rotation;
    sceVu0FVECTOR   forward;
    sceVu0FVECTOR   added_movement;
    sceVu0FVECTOR   knockback;
    sceVu0FMATRIX   facing_matrix;
    CActionChara   *arm;
    int             i;

    sceVu0CopyVector(movement, velocity);
    movement[0] = 0.0f;
    movement[2] = 0.0f;
    sceVu0CopyVector(velocity, movement);
    if (chara_type == ACTION_CHARA_ROBO) {
        arm = SearchChara("arm");
        if (arm != NULL) {
            GetRotation(rotation);
            arm->GetRotation(arm_rotation);
            rotation[1] += arm_rotation[1];
            if (rotation[1] > 3.1415927f) {
                rotation[1] -= 6.2831855f;
            }
            if (rotation[1] < -3.1415927f) {
                rotation[1] += 6.2831855f;
            }
            sceVu0FVECTOR forward = { 0.0f, 0.0f, 1.0f, 1.0f };
            sceVu0FMATRIX facing_matrix;
            sceVu0UnitMatrix(facing_matrix);
            sceVu0RotMatrixY(facing_matrix, facing_matrix, rotation[1]);
            sceVu0ApplyMatrix(front_vec, facing_matrix, forward);
        }
    } else {
        sceVu0FVECTOR self_rot2;
        GetRotation(self_rot2);
        sceVu0FVECTOR forward2 = { 0.0f, 0.0f, 1.0f, 1.0f };
        sceVu0FMATRIX matrix2;
        sceVu0UnitMatrix(matrix2);
        sceVu0RotMatrixY(matrix2, matrix2, self_rot2[1]);
        sceVu0ApplyMatrix(front_vec, matrix2, forward2);
    }
    add_vec[1] = 0.0f;
    if (add_speed > 0.0f && add_time != 0) {
        sceVu0FVECTOR added_movement;
        sceVu0ScaleVectorXYZ(added_movement, add_vec, add_speed);
        sceVu0AddVector(velocity, velocity, added_movement);
        if (add_time > 0) {
            if (add_speed > 0.0f) {
                add_speed -= add_decel;
            }
            add_time--;
            if (add_time <= 0) {
                add_speed = 0.0f;
            }
        }
    }
    if (blow_speed > 0.0f && blow_time != 0) {
        sceVu0FVECTOR knockback;
        sceVu0ScaleVectorXYZ(knockback, blow_vec, blow_speed);
        sceVu0AddVector(velocity, velocity, knockback);
        if (blow_time > 0) {
            if (blow_speed > 0.0f) {
                blow_speed -= blow_decel;
            }
            blow_time--;
            if (blow_time <= 0) {
                blow_speed = 0.0f;
            }
        }
    }
    if (catch_state == 2) {
        sceVu0CopyVector(velocity, blow_vec);
        blow_vec[1] -= 0.6f;
    }
    if (unk_75e != 0) {
        unk_760 += 0.016666668f;
        if (unk_760 >= 1.0f) {
            unk_760 = 1.0f;
        }
    } else {
        unk_760 -= 0.016666668f;
        if (unk_760 <= 0.0f) {
            unk_760 = 0.0f;
        }
    }
    if (stagger_time > 0) {
        stagger_time--;
        if (stagger_time <= 0 && stagger > 0) {
            stagger = 0;
        }
    }
    if (shot_wait > 0) {
        shot_wait--;
    }
    if (murderous_time > 0) {
        murderous_time--;
    }
    if (damage_time > 0) {
        damage_time--;
    }
    if (unk_be4 > 0) {
        unk_be4--;
    }
    if (muteki_time > 0) {
        muteki_time--;
    }
    if (no_hit_time > 0) {
        no_hit_time--;
    }
    if (shake.time > 0) {
        shake.offset = 2.5f * mgRnd();
        shake.time--;
    }
    for (i = 0; i < 3; i++) {
        pallet[i].Step();
    }
    release_timing = 0;
}

void CActionChara::Step() {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;
    sceVu0FVECTOR arm_position;
    sceVu0FVECTOR target_position;
    sceVu0FMATRIX arm_matrix;
    sceVu0FMATRIX aim_matrix;
    CActionChara *link;
    CCharacter2 *part;
    mgCFrame *arm;

    StepParam();
    if (se_positional != 2) {
        se_positional = 1;
    }
    CCharacter2::Step();
    link = next;
    if (link != NULL) {
        do {
            part = link;
            if (part->se_positional != 2) {
                part->se_positional = 0;
            }
            part->CCharacter2::Step();
            link = link->next;
        } while (link != NULL);
    }
    if (hold_type == ACTION_HOLD_STONE && hold_parts != NULL && hold_frame != NULL) {
        GetRotation(rotation);
        hold_frame->GetWorldPosition0(position);
        position[3] = 1.0f;
        position[1] -= 1.0f;
        hold_parts->SetPosition(position);
        hold_parts->SetRotation(rotation);
    }
    arm = SearchObject("L_arm");
    if (arm != NULL) {
        static float ang = 0.0f;

        arm->GetWorldPosition0(arm_position);
        if (lock_on != 0 && dir_gun != 0) {

            int monster_index = target_no - MONSTER_ACTIVE_MAX;
            ActiveMonster->active[monster_index]->GetEntryObjectPos(0, target_position);
            sceVu0SubVector(arm_position, target_position, arm_position);
            sceVu0CopyVector(target_position, arm_position);
            target_position[3] = 1.0f;
            target_position[1] = 0.0f;
            ang = -atan2f(arm_position[1], mgDistVector(target_position));
        } else {
            ang = 0.0f;
        }
        sceVu0CopyMatrix(arm_matrix, arm->trans_matrix);
        sceVu0UnitMatrix(aim_matrix);
        sceVu0RotMatrixZ(aim_matrix, aim_matrix, ang);
        sceVu0MulMatrix(arm_matrix, arm_matrix, aim_matrix);
        arm->SetTransMatrix(arm_matrix);
    }
}

void CActionChara::ShadowStep() {
    CActionChara *part;

    for (part = this; part != NULL; part = part->next) {
        part->CCharacter2::ShadowStep();
    }
}

void CActionChara::Initialize(mgCMemory *memory) {
    int i;
    int j;

    CCharacter2::Initialize();
    accume_effect = NULL;
    old_pos[2] = 0.0f;
    old_pos[1] = 0.0f;
    old_pos[0] = 0.0f;
    old_pos[3] = 1.0f;
    chara_kind = ACTION_KIND_NONE;
    script_buf = NULL;
    max_speed = 4.0f;
    prog = 0;
    pad_history = 0;
    parent = NULL;
    next = NULL;
    accele.speed = 0.0f;
    accele.move_speed = 3.0f;
    accele.accele[0] = 0.0f;
    accele.accele[1] = 0.0f;
    accele.accele[2] = 0.0f;
    accele.accele[3] = 0.0f;
    acumu_pad = 0;
    now_status = 0;
    unk_bec = 0;
    unk_75e = 0;
    unk_760 = 0.0f;
    muteki_time = 0;
    guard_flag = 0;
    menu_flag = 0;
    add_speed = 0.0f;
    add_time = 0;
    blow_vec[2] = 0.0f;
    blow_vec[1] = 0.0f;
    blow_vec[0] = 0.0f;
    blow_vec[3] = 1.0f;
    blow_speed = 0.0f;
    blow_time = 0;
    unk_be4 = 0;
    damage_time = 0;
    stagger = 0;
    stagger_time = 0;
    mask_flag = 0;
    dir_gun = 0;
    default_motion = "\x97\xA7\x82\xBF";
    shot_wait = 0;
    murderous = 0;
    murderous_time = 0;
    target_no = -1;
    lock_on = 0;
    damage_req = ACTION_DAMAGE_REQ_NONE;
    catch_frame = NULL;
    catch_state = 0;
    no_hit_time = 0;
    release_timing = 0;
    hold_parts = NULL;
    hold_frame = NULL;
    hold_type = ACTION_HOLD_NONE;
    unk_72a = -1;
    for (i = 0; i < 9; i++) {
        sw_effect[i].sword_no = 0;
        sw_effect[i].frame0 = NULL;
        sw_effect[i].frame1 = NULL;
        sw_effect[i].chara = NULL;
        sw_effect[i].motion = NULL;
        sw_effect[i].wait = 0;
        sw_effect_num = 0;
    }
    effect_man = NULL;
    shake.time = 0;
    for (j = 0; j < 3; j++) {
        pallet[j].Initialize();
    }
    ResetScript();
}

#ifdef NONMATCHING
void CActionChara::Copy(CActionChara &dest, mgCMemory *memory) {
    dest = *this;
    if (memory != NULL) {
        CCharacter2::Copy(dest, memory);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", Copy__12CActionCharaFR12CActionCharaP9mgCMemory);
#endif

#ifdef NONMATCHING
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/actionchara", __as__11CCharacter2FRC11CCharacter2);
#endif


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1398__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2048__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2543__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2586__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2720__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2818__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2846__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3289__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3291__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1325__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1357__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1358__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1394__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1427__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_1428__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2209__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2210__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2211__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2212__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2213__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2214__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2215__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2216__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2217__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2294__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2295__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2333__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2334__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2420__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2421__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2422__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2423__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2504__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2505__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2506__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2507__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2508__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2509__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2510__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2629__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2630__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2631__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2632__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2633__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2634__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2713__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2714__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_2840__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3085__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3262__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3263__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", at_3389__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/actionchara", __vt__12CActionChara__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(old_angle, 0x4);
INCLUDE_BSS(ang_3371, 0x4);
INCLUDE_BSS(init_3372, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_3107, 0x10);
