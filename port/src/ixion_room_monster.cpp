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
#include "collision.hpp"
#include "colprim.hpp"
#include "ixion_ledger.hpp"
#include "dng_main.hpp"
#include "dngfloor.hpp"
#include "effscript.hpp"
#include "ixion_dc1_monster.hpp"
#include "ixion_monica_faithful.hpp"
#include "ixion_room.hpp"
#include "map.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "monster.hpp"
#include "mg_texture.hpp"
#include "runscript_opcodes.hpp"
#include "scene.hpp"
#include "snd_mngr.hpp"
#include "userdata.hpp"

namespace {

// Marks the native monster whose per-monster update is running, so map and
// navigation queries made inside it are attributed by identity, not proximity.
struct IxionRoomThinkingScope {
    explicit IxionRoomThinkingScope(const CActiveMonster *monster) { IxionRoomSetThinkingMonster(monster); }
    ~IxionRoomThinkingScope() { IxionRoomSetThinkingMonster(nullptr); }
    IxionRoomThinkingScope(const IxionRoomThinkingScope &) = delete;
    IxionRoomThinkingScope &operator=(const IxionRoomThinkingScope &) = delete;
};

} // namespace

// CMonsterMan::ThinkHost (ps2/src/monster.cpp) with two port additions for the
// opt-in Ixion imported room: suspended monsters are skipped and the running
// monster is published for identity-based query attribution. With no room
// installed IxionRoomMonsterSuspended is always false and the loop is native.
PORT_OVERRIDE void CMonsterMan::ThinkHost() {
    sceVu0FVECTOR    monster_pos;
    CCPoly           polys[128];
    sceVu0FVECTOR    camera_pos;
    sceVu0FVECTOR    target_pos;
    sceVu0FVECTOR    own_pos;
    mgVu0FBOX        box;
    sceVu0FVECTOR    item_pos;
    DNG_BATTLE_AREA *battle = &scene->battle_area;
    CMap            *map = scene->GetMap(scene->active_map);

    if (map == NULL) {
        return;
    }

    CCameraControl *camera = (CCameraControl *) scene->GetCamera(scene->active_camera);

    if (camera != NULL) {
        camera->GetPos(camera_pos);
    }

    const bool donor_paused = IxionDc1UpdatePause();
    IxionDc1StepShots(); // Ixion: DC1 shots outlive death but freeze during events
    IxionMonicaFaithfulStep(); // Ixion: opt-in player-owned goo spit (DC2_IXION_MONICA_FAITHFUL); no-op unset

    int user_monster = -1;

    if (DngUserData->active_chr_no == USER_CHARA_MONSTER) {
        user_monster = GetBattleCharaInfo()->user_mons_id;
    }

    for (int i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        CActiveMonster *monster = active[i];

        if (monster == NULL || monster->state == ACTIVE_MONSTER_NONE) {
            continue;
        }

        // Ixion room policy: while an imported room owns the floor every
        // non-selected native monster is suspended (no script, movement,
        // collision query, attack window or fade); the selected encounter
        // enemy runs natively and is identified by actor identity below.
        if (IxionRoomMonsterSuspended(monster)) {
            continue;
        }

        if (donor_paused && IxionDc1IsDonor(monster)) continue;

        // Ixion: an unrevealed DC1 mimic runs no script (DC1 monstorunit.cpp:1945); it keeps its
        // chest pose until the confirm press opens it (IxionDc1HiddenStep requests prog_50).
        if (IxionDc1HiddenStep(monster)) {
            monster->Step();
            continue;
        }

        // Observe accepted player attack motion starts before the donor's next VM resume.
        IxionDc1BeforeScript(monster);

        IxionRoomThinkingScope thinking(monster);

        monster->GetEntryObjectPos(CHARA_ENTRY_GROUP_TARGET, 0, monster->center_pos);
        monster->scoop.ok = false;
        MONSTER_SCOOP *scoop = &monster->scoop;

        if (monster->scoop.type & MONSTER_SCOOP_ALWAYS) {
            scoop->ok = true;
        }

        if (scoop->type != 0 && (scoop->type & MONSTER_SCOOP_MOTION)) {
            char *motion = monster->GetNowMotionName();

            if (motion != NULL && strcmp(motion, scoop->motion) == 0) {
                float frame = monster->GetNowFrameWait(NULL);

                if (!(frame < scoop->start) && frame < scoop->end) {
                    scoop->ok = true;
                }
            }
        }

        if (battle->pause_flag & DNG_PAUSE_ENEMY_STEP) {
            if (monster->tbl->user_mons_id == user_monster) {
                monster->Step();
            }

            continue;
        }

        if (monster->catch_state == ACTION_CATCH_HELD) {
            RunScript(i);
            monster->SetPosition(0.0f, -2.0f, 0.0f);
            monster->Step();
            ACTION_DAMAGE *damage = monster->damage;

            for (int n = 0; n < ACTION_DAMAGE_MAX; n++) {
                if (damage->use != 0) {
                    if (damage->prim != NULL) {
                        damage->prim->Delete(-1);
                        damage->prim = NULL;
                    }

                    damage++;
                }
            }

            continue;
        }

        monster->CheckStatusAttr();
        monster->GetPosition(monster_pos);
        BASE_MONSTER_TBL *tbl = monster->tbl;
        CActionChara     *target = NULL;

        if (monster->target_no != -1) {
            target = (CActionChara *) scene->GetCharacter(monster->target_no);
        }

        if (target != NULL) {
            target->GetPosition(target_pos);
            monster->GetPosition(own_pos);
            monster->target_dist = mgDistVector(target_pos, own_pos);

            if (AutoMapGen.GetNaviDistance(own_pos) < 0.0f) {
                monster->target_dist = 9999999.0f;
            }

            if (camera != NULL) {
                monster->camera_dist = mgDistVector(camera_pos, own_pos);
            }

            if (monster->state != ACTIVE_MONSTER_DEAD && monster->CheckView(priority_limit) == MONSTER_VIEW_OUT) {
                continue;
            }
        }

        box.max[0] = monster_pos[0] + 40.0f;
        box.min[0] = monster_pos[0] - 40.0f;
        box.max[2] = monster_pos[2] + 40.0f;
        box.min[2] = monster_pos[2] - 40.0f;
        box.max[1] = monster_pos[1] + 200.0f;
        box.min[1] = monster_pos[1] - 200.0f;
        box.max[3] = 1.0f;
        box.min[3] = 1.0f;
        int                  poly_num = map->GetColPoly(polys, box, 128);
        CTreasureBoxManager *treasure = battle->treasure_box;

        if (treasure != NULL) {
            poly_num += treasure->PickupCollision(monster_pos, &polys[poly_num], box, 128 - poly_num);
        }

        if (target != NULL && monster->state == ACTIVE_MONSTER_LIVE && target->murderous_time > 0 &&
            target->target_no == i + MONSTER_ACTIVE_MAX) {
            int chance = fptosi(100.0f * (float) rand() / 2147483648.0f);

            if (monster->mask_flag & 2) {
                chance = 100;
            }

            if (target->murderous == 0 && chance < tbl->escape_rate0) {
                monster->req_prog = MONSTER_PROG_ESCAPE_0;
            }

            if (target->murderous == 1 && chance < tbl->escape_rate1) {
                monster->req_prog = MONSTER_PROG_ESCAPE_1;
            }

            target->murderous_time = 0;
        }

        if (monster->damage_req == ACTION_DAMAGE_REQ_DEAD) {
            monster->req_prog = MONSTER_PROG_DEAD;
            monster->damage_req = ACTION_DAMAGE_REQ_NONE;
        }

        RunScript(i);
        IxionDc1AfterScript(monster); // Ixion: DC1 script's walk and turn into the native mover
        MoveUnit(monster, polys, poly_num);
        IxionDc1AfterMove(monster);

        for (int n = 0; n < ACTION_DAMAGE_MAX; n++) {
            ACTION_DAMAGE *damage = &monster->damage[n];

            if (monster->damage[n].use == 0) {
                continue;
            }

            float frame = monster->GetNowFrame(damage->chara);

            if (damage->prim == NULL) {
                if (!(frame < damage->start_frame) && frame < damage->end_frame) {
                    damage->prim = ColPrimMan.GetPrim();

                    if (damage->prim != NULL) {
                        damage->prim->SetDamage(damage->damage, monster->chara_type);
                        IxionRoomNoteMonsterPrim(damage->prim, monster); // N7: after native incarnation initialization
                        damage->prim->SetCoord(damage->frame0, damage->frame1, damage->radius);
                        damage->prim->damage = monster->attack;
                        IxionDc1AttackPrim(monster, damage); // Ixion: the DC1 sphere's own damage
                    }
                }
            } else if (frame < damage->start_frame || !(frame <= damage->end_frame)) {
                damage->prim->Delete(-1);
                damage->prim = NULL;
            }
        }

        if (monster->state == ACTIVE_MONSTER_DEAD && monster->dead_alpha > 0) {
            monster->dead_alpha -= 3;
            monster->alpha = (float) monster->dead_alpha / 128.0f;
            monster->damage_time = 0;

            if (monster->attrib & MONSTER_ATTRIB_QUICK_DEAD) {
                monster->alpha = 0.0f;
                monster->dead_alpha = 0;
                monster->damage_time = 0;
            }

            if (monster->dead_alpha <= 0) {
                KillMonsterCount(monster->tbl->id, 1);
                IxionLedgerNoteKillConfirmed(monster); // Ixion: snapshot the owned spawn before NONE/cleanup
                monster->dead_alpha = 0;
                monster->state = ACTIVE_MONSTER_NONE;
                monster->chara_kind = ACTION_KIND_NONE;

                if (NowFloorInfoPtr != NULL) {
                    NowFloorInfoPtr->kill_count++;
                }

                if (monster->gate_key > 0) {
                    CPullItem *item = PullItemMan.GetList(0);

                    if (item != NULL) {
                        float velocity[4] = {0.0f, 2.0f, 0.0f, 1.0f};
                        monster->GetEntryObjectPos(0, item_pos);
                        item_pos[1] += 20.0f;
                        item->SetItem(item_pos, velocity, 2);
                        item->item_no = monster->gate_key;
                    }
                }
                IxionDc1Removed(monster); // close private shots/windows at native slot removal
            }
        }

        monster->piyori.Step();
        monster->gift_mark.Step();

        if (monster->piyori_mark > 0) {
            monster->piyori_mark--;

            if (monster->piyori_mark <= 0) {
                monster->piyori_time = 0;
            }
        }

        if (monster->gekirin_time > 0) {
            monster->gekirin_time--;

            if (monster->gekirin_time <= 0) {
                monster->gekirin = monster->gekirin_num;
                monster->life_gage.ResetGekirin(monster->gekirin_num);
                monster->pallet[1].duration = 0;

                if (monster->tbl->boss == 0) {
                    monster->attack = monster->tbl->attack;
                }
            }
        }

        if (monster->camera_dist < 4.0f * monster->body_width) {
            if (!(monster->camera_alpha <= 0.0f)) {
                monster->camera_alpha -= 1.0f / 12.0f;

                if (monster->camera_alpha <= 0.0f) {
                    monster->camera_alpha = 0.0f;
                }
            }
        } else if (monster->camera_alpha < 1.0f) {
            monster->camera_alpha += 1.0f / 12.0f;

            if (!(monster->camera_alpha < 1.0f)) {
                monster->camera_alpha = 1.0f;
            }
        }

        if (monster->view_state == MONSTER_VIEW_FADE_OUT) {
            monster->view_alpha -= 0.05f;

            if (monster->view_alpha <= 0.0f) {
                monster->view_alpha = 0.0f;
                monster->view_state = MONSTER_VIEW_OUT;
            }
        }

        if (monster->view_state == MONSTER_VIEW_FADE_IN) {
            monster->view_alpha += 0.05f;

            if (!(monster->view_alpha < 1.0f)) {
                monster->view_alpha = 1.0f;
                monster->view_state = MONSTER_VIEW_IN;
            }
        }

        float step = IxionDc1MotionStep(monster, monster->GetDefaultStep()); // Ixion: DC1 motion speed

        if (monster->status.attr & MONSTER_STATUS_SLOW) {
            monster->SetStep(0.5f * step);
        } else {
            monster->SetStep(step);
        }

        monster->Step();
    }

    PriorityLevelCheck();
}

// CMonsterMan::CollisionCheck (ps2/src/monster.cpp) with one port addition for
// the opt-in Ixion imported room: a parked (suspended) native monster is not
// an "other" body, so it cannot invisibly push the selected enemy. With no
// room installed IxionRoomMonsterParked is always false and the loop is native.
PORT_OVERRIDE void CMonsterMan::CollisionCheck(CActiveMonster *monster, float *pos, float *move, float *push) {
    float           next[4];
    float           center[4];
    float           other_pos[4];
    float           away[4];
    int             i;
    CActiveMonster *other;
    u32             se_handle;
    float           height_scale;
    float           reach;
    float           dist;
    float           overlap;

    sceVu0CopyVector(push, move);
    next[0] = pos[0] + push[0];
    next[1] = pos[1] + push[1];
    next[2] = pos[2] + push[2];
    sceVu0CopyVector(center, next);
    center[1] = 0.0f;
    se_handle = scene->se_battle_id;

    if (monster->catch_state == ACTION_CATCH_HELD) {
        return;
    }

    for (i = 0; i < MONSTER_ACTIVE_MAX + 1; i++) {
        if (i == 0) {
            other = (CActiveMonster *) scene->GetCharacter(0);
        }

        if (i != 0) {
            other = active[i - 1];
        }

        if (other == NULL || other == monster || other->chara_kind != ACTION_KIND_SCRIPT ||
            (monster->catch_state == ACTION_CATCH_THROWN && i == 0) || IxionRoomMonsterParked(other)) {
            continue;
        }

        other->GetPosition(other_pos);
        height_scale = 1.0f;

        if (monster->catch_state == ACTION_CATCH_THROWN && i != 0) {
            other->GetEntryObjectPos(0, other_pos);
            height_scale = 2.0f;
        }

        if (next[1] + monster->body_height * height_scale < other_pos[1] ||
            !(next[1] <= other_pos[1] + other->body_height * height_scale)) {
            continue;
        }

        other_pos[1] = 0.0f;
        reach = 2.0f * other->body_width + 2.0f * monster->body_width * height_scale;
        dist = mgDistVector(other_pos, center);

        if (dist < reach) {
            if (monster->catch_state == ACTION_CATCH_THROWN && i != 0) {
                sceVu0ScaleVectorXYZ(push, push, -0.8f);
                // Ixion (precheck P3): a dead DC1 donor takes no stun or blow from a thrown monster;
                // every native target and the thrown monster's own reaction stay retail.
                if (!IxionDc1RefusesReaction(other)) {
                    other->req_prog = MONSTER_PROG_PIYORI;
                    other->piyori_time = 120;
                    sceVu0CopyVector(other->blow_vec, push);
                    other->blow_vec[1] = 0.0f;
                    sceVu0Normalize(other->blow_vec, other->blow_vec);
                    sceVu0ScaleVectorXYZ(other->blow_vec, other->blow_vec, -1.0f);
                    other->blow_speed = 2.0f;
                    other->blow_rate = 1.0f;
                    other->blow_decel = 0.0f;
                    other->blow_time = 3;
                }
                HitEffectSet(scene, pos, 0);
                sndSePlay(se_handle, 0x1C, 0);
                monster->req_prog = MONSTER_PROG_PIYORI;
                monster->piyori_time = 30;
                monster->catch_state = ACTION_CATCH_NONE;
                // The throw is consumed before its receipt: later overlap queries cannot repeat it.
                printf("HIT!!\n");
            }

            if (i != 0) {
                overlap = reach - dist;
                away[0] = center[0] - other_pos[0];
                away[1] = center[1] - other_pos[1];
                away[2] = center[2] - other_pos[2];
                away[3] = 1.0f;
                sceVu0Normalize(away, away);
                push[0] = (push[0] + away[0] * overlap) / 2.0f;
                push[1] = (push[1] + away[1] * overlap) / 2.0f;
                push[2] = (push[2] + away[2] * overlap) / 2.0f;
                push[3] = 1.0f;
                sceVu0ScaleVector(push, push, 1.5f);
            } else {
                push[2] = 0.0f;
                push[1] = 0.0f;
                push[0] = 0.0f;
                push[3] = 1.0f;
            }
        }
    }
}

PORT_OVERRIDE void CMonsterMan::Initialize(CScene *scene) {
    // N8: the retail body verbatim, after retiring an installed room while the
    // saved actor pointers still name the incarnation being reset.
    IxionRoomBeforeMonsterReset("monster-reset");
    mgCTextureManager *textures;
    int                i;
    DNG_BATTLE_AREA   *area = &scene->battle_area;
    int                texb;
    int                j;
    int                k;
    int                m;

    SetMonsterExtendTable();
    textures = &mgTexManager;
    this->scene = scene;
    effect_man = FxScriptMan;
    priority_limit = 5;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        active[i] = (CActiveMonster *) scene->GetCharacter(i + MONSTER_ACTIVE_MAX);
        active[i]->Initialize();
    }

    for (j = 0; j < MONSTER_REFER_MAX; j++) {
        textures->DeleteBlock(j + MONSTER_TEXB_REFER);
        refer[j].id = -1;
    }

    for (k = 0; k < MONSTER_SHARE_MAX; k++) {
        shared_script_vars[k].i = 0;
    }

    FxScriptMan->ClearBaseFromLevel(3, NULL, -1);

    for (texb = area->free_texb; texb < MONSTER_TEXB_FREE_END; texb++) {
        textures->DeleteBlock(texb);
    }

    boss_life_gage.Initialize(0);
    boss_max_life = 0;
    locate.num = 0;
    locate.put_num = 0;
    locate.put_flag = 0;

    for (m = 0; m < MONSTER_LOCATE_MAX; m++) {
        s16 *param = locate.param;
        s16 *id = locate.monster_id;
        param[m] = -1;
        id[m] = -1;
    }
}

// N11: retail CMonsterMan::SetActiveMonster verbatim plus two hooks. The slot
// it picks may be a NONE slot whose scene character address another
// incarnation (the killed selected enemy, a parked record) still names: the
// before hook starts a new incarnation and drops every old ownership record
// before the slot is overwritten; the after hook parks the new monster when
// it was spawned while the room is installed.
PORT_OVERRIDE CActiveMonster *CMonsterMan::SetActiveMonster(int refer_no, float pos[], sceVu0FVECTOR rot, int param) {
    int               slot;
    CActiveMonster   *monster;
    int               npc;
    BASE_MONSTER_TBL *tbl;
    int               i;
    DNG_BATTLE_AREA  *area;

    if (refer_no < 0 || refer_no >= MONSTER_REFER_MAX) {
        return NULL;
    }

    if (refer[refer_no].chara.GetFrame() == NULL) {
        return NULL;
    }

    slot = SearchActiveMonsterBlock();

    if (slot < 0) {
        return NULL;
    }

    area = &scene->battle_area;
    npc = GetBattleCharaInfo()->GetNowNPC();
    active[slot] = (CActiveMonster *) scene->GetCharacter(slot + MONSTER_ACTIVE_MAX);

    if (active[slot] == NULL) {
        return NULL;
    }

    IxionRoomBeforeMonsterSpawn(active[slot]);

    memory[slot].stReset();
    locate.SetPutFlag(slot, true);
    monster = active[slot];
    tbl = GetReferPtr2(refer[refer_no].id);

    if (tbl->share_model == 0) {
        refer[refer_no].chara.SetPosition(0.0f, 0.0f, 0.0f);
        refer[refer_no].chara.SetRotation(0.0f, 0.0f, 0.0f);
        refer[refer_no].chara.Copy(*monster, &memory[slot]);
    } else {
        *monster = refer[refer_no].chara;
        monster->main_frame_info = monster->motion[0].frame_info;
        monster->shadow_frame_info = monster->shadow_motion[0].frame_info;
        monster->now_key = 0;
    }

    if (tbl->boss) {
        area->boss_map = 1;
    }

    monster->ResetMotion();
    monster->SetPosition(pos);
    monster->SetRotation(rot);
    monster->UpdatePosition();
    sceVu0CopyVector(monster->place_pos, pos);
    monster->param = *tbl;
    monster->base_tbl = tbl;
    monster->tbl = &monster->param;
    monster->monster_id = refer[refer_no].id;
    monster->chara_type = slot + MONSTER_ACTIVE_MAX;
    monster->chara_kind = ACTION_KIND_SCRIPT;
    monster->Show(1, 1);
    monster->refer_no = refer_no;
    monster->view_state = MONSTER_VIEW_INIT;
    monster->view_alpha = 0.0f;
    monster->camera_alpha = 1.0f;
    monster->next_pos[0] = 0.0f;
    monster->next_pos[1] = 0.0f;
    monster->next_pos[2] = 0.0f;
    monster->next_pos[3] = 1.0f;
    monster->move_speed = 0.0f;
    monster->max_life = monster->life = tbl->life;
    monster->reward_exp = tbl->reward_exp;
    monster->reward_money = tbl->reward_money;
    monster->unk_1322 = tbl->unk_5a;
    monster->whp = tbl->whp;
    monster->attack = tbl->attack;
    monster->defense = tbl->defense;
    monster->gekirin_num = tbl->gekirin_num;

    if (npc == 8) {
        monster->gekirin_num += 2;
    }

    if (tbl->boss) {
        monster->gekirin = -1.0f;
        monster->life_gage.Initialize(0);
        boss_max_life += monster->max_life;
    } else {
        monster->gekirin = monster->gekirin_num;
        monster->life_gage.Initialize(fptosi(monster->gekirin));
    }

    monster->gekirin_time = 0;
    monster->piyori.Initialize();
    monster->gift_mark.Initialize();
    monster->drop_badge = false;
    monster->sound_info.loop_se = &scene->loop_se;
    monster->message_no = param;
    monster->locate_param = -1;
    monster->attrib = 0;
    monster->state = ACTIVE_MONSTER_LIVE;
    monster->target_no = 0;
    monster->target_dist = 999999.0f;
    monster->camera_dist = 999999.0f;
    monster->clip_dist = 800.0f;
    monster->alpha = 1.0f;
    monster->unk_1300 = 400.0f;
    monster->unk_1304 = 300.0f;
    monster->unk_1308 = 180;
    monster->stagger = 0;
    monster->stagger_time = 0;
    monster->target_dist = 0.0f;
    monster->priority = -1;

    for (i = 0; i < MONSTER_VAR_MAX; i++) {
        monster->script_vars[i].i = 0;
    }

    for (i = 0; i < MONSTER_VAR2_MAX; i++) {
        monster->secondary_script_vars[i].i = 0;
    }

    SetMonsterScript(&monster->mons_script, refer[refer_no].script, &memory[slot]);
    monster->req_prog = MONSTER_PROG_INIT;
    RunScript(slot);
    monster->req_prog = MONSTER_PROG_MAIN;
    IxionRoomAfterMonsterSpawn(active[slot]);
    return active[slot];
}
