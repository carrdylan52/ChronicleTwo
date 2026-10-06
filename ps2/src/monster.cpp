#include "common.h"
#include "monster.hpp"

#include <cstdio>
#include <cstring>

#include "automap.hpp"
#include "character.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dng_effect.hpp"
#include "dng_event.hpp"
#include "dng_hud.hpp"
#include "dng_main.hpp"
#include "effscript.hpp"
#include "gameutil.hpp"
#include "mainloop.hpp"
#include "map.hpp"
#include "menucommon.hpp"
#include "mg_camera.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "userdata.hpp"

static char *dung_progtxt_notlift_mons[7] = { /**< Message shown when a monster cannot be lifted, per language. */
    "[(!)]\x82\xB1\x82\xCC\x83\x82\x83\x93\x83\x58\x83\x5E\x81\x5B\x82\xCD\x8E\x9D\x82\xBF\x8F\xE3\x82\xB0\x82\xE9\x82\xB1\x82\xC6\x82\xAA\x82\xC5\x82\xAB\x82\xC8\x82\xA2!",
    "[(!)]You can't lift this monster!",
    "[(!)]Tu ne peux soulever ce monstre!",
    "[(!)]Dieses Monster kannst du nicht hochheben!",
    "[(!)]Non puoi sollevare questo mostro!",
    "[(!)][UNI00a1]No puedes levantar al montruo!",
    "[(!)]You can't lift this monster!",
};

static int no_score_uv[7][4] = { /**< Texture rectangle of the no-damage mark, per language. */
    { 94, 186, 62, 16 },
    { 94, 184, 66, 18 },
    { 94, 182, 96, 20 },
    { 94, 184, 68, 18 },
    { 94, 184, 86, 18 },
    { 94, 184, 66, 18 },
    { 94, 184, 66, 18 },
};

static int guard_score_uv[7][4] = { /**< Texture rectangle of the guard mark, per language. */
    { 340, 192, 44, 18 },
    { 330, 192, 54, 16 },
    { 330, 192, 54, 16 },
    { 330, 192, 54, 16 },
    { 330, 192, 54, 16 },
    { 330, 192, 54, 16 },
    { 330, 192, 54, 16 },
};

static s16 gift_item_tbl[][3] = { /**< Gift item that each gift type takes, ended by a type of -1. */
    { 0, 269, 1 },
    { 34, 270, 1 },
    { 2, 318, 3 },
    { 37, 314, 3 },
    { 11, 289, 8 },
    { 59, 189, 10 },
    { 18, 225, 6 },
    { 38, 192, 6 },
    { 41, 211, 5 },
    { 31, 234, 7 },
    { 29, 232, 7 },
    { 30, 233, 7 },
    { 28, 235, 7 },
    { 32, 229, 7 },
    { -1, -1 },
};

static void HitEffectSet(CScene *scene, float *pos, int flags);
static void GuardEffectSet(CScene *scene, float *pos, int play_script);
static void HitScoreSet(float *pos, int type, int value);
static int  CheckGiftPack(CActiveMonster *monster, CColPrim *prim);
static int  _MONSTER_NAME(SPI_STACK *stack, int count);

static SPI_TAG_PARAM mos_data_anlyze_tag[] = { /**< Tags of the monster name file. */
    { "NAME", _MONSTER_NAME },
    { NULL, NULL },
};

// Code (.text)
int CActiveMonster::IsDraw(int alive_only) {
    if (chara_kind != ACTION_KIND_SCRIPT) {
        return 0;
    }
    if (life <= 0 && (alive_only & 1)) {
        return 0;
    }
    return 1;
}

void CActiveMonster::CheckStatusAttr() {
    sceVu0FVECTOR pos;
    int           old_life;
    int           damage;

    if (status.attr & MONSTER_STATUS_POISON) {
        status.poison_count++;
        if (status.poison_count >= 120) {
            status.poison_count = 0;
            pallet[0].SetAnim(0x60, 0x20, 0x60, 1, 30, 0);
            old_life = life;
            if (old_life > 0) {
                life = (int)((float)old_life - 0.04f * (float)max_life);
                if (life <= 0) {
                    life = 1;
                }
                damage = old_life - life;
                if (damage > 0) {
                    GetEntryObjectPos(0, 0, pos);
                    pos[1] += body_height;
                    HitScoreSet(pos, 0, damage);
                }
            }
        }
    }
    if (status.attr & (MONSTER_STATUS_UNK_8 | MONSTER_STATUS_UNK_20)) {
        if (status.grey_time % 45 == 0) {
            pallet[0].SetAnim(0x20, 0x20, 0x20, 1, 45, 0);
        }
        status.grey_time--;
        if (status.grey_time <= 0) {
            status.grey_time = 0;
            status.attr &= ~(MONSTER_STATUS_UNK_8 | MONSTER_STATUS_UNK_20);
        }
    }
    if (status.attr & MONSTER_STATUS_SLOW) {
        if (status.slow_time % 45 == 0) {
            pallet[0].SetAnim(0xA0, 0x40, 0xA0, 1, 45, 0);
        }
        status.slow_time--;
        if (status.slow_time <= 0) {
            status.slow_time = 0;
            status.attr &= ~MONSTER_STATUS_SLOW;
        }
    }
}

int CActiveMonster::CheckView(int priority_limit) {
    int rank;

    rank = priority;
    if (rank < 0) {
        rank = 99;
    }
    if (attrib & MONSTER_ATTRIB_ALWAYS_VIEW) {
        view_state = MONSTER_VIEW_IN;
        view_alpha = 1.0f;
        return view_state;
    }
    if (view_state == MONSTER_VIEW_INIT) {
        if (target_dist < clip_dist) {
            view_state = MONSTER_VIEW_IN;
            view_alpha = 1.0f;
        } else {
            view_state = MONSTER_VIEW_OUT;
            view_alpha = 1.0f;
        }
        return view_state;
    }
    if (view_state == MONSTER_VIEW_IN) {
        if (target_dist > 30.0f + clip_dist || rank >= priority_limit) {
            view_state = MONSTER_VIEW_FADE_OUT;
        }
    }
    if (view_state == MONSTER_VIEW_OUT && target_dist < clip_dist && rank < priority_limit) {
        view_state = MONSTER_VIEW_FADE_IN;
    }
    return view_state;
}

void CActiveMonster::Step() {
    CActionChara::Step();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", Copy__14CActiveMonsterFR14CActiveMonsterP9mgCMemory);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", __as__12CActionCharaFRC12CActionChara);

void CActiveMonster::Initialize() {
    int i;

    CActionChara::Initialize(NULL);
    refer_no = 0;
    monster_id = 0;
    req_prog = -1;
    now_prog = -1;
    target_no = 0;
    view_state = MONSTER_VIEW_INIT;
    view_alpha = 0.0f;
    camera_alpha = 1.0f;
    priority = 999;
    clip_dist = 500.0f;
    unk_1300 = 400.0f;
    unk_1304 = 300.0f;
    unk_1308 = 180;
    att_type = -1;
    stagger = 0;
    stagger_time = 0;
    piyori_mark = 0;
    piyori_time = 0;
    event_no = -1;
    reserv_img[1] = NULL;
    reserv_img[0] = NULL;
    for (i = 0; i < MONSTER_VAR_MAX; i++) {
        var[i].i = 0;
    }
    for (i = 0; i < MONSTER_VAR2_MAX; i++) {
        var2[i].i = 0;
    }
    max_life = 0;
    life = 0;
    state = ACTIVE_MONSTER_NONE;
    dead_alpha = 0;
    link_parts = NULL;
    link_type = 0;
    unk_1208 = 0;
    attack = 0;
    life_gage.Initialize(0);
    scoop.type = 0;
    scoop.ok = -1;
    gekirin = 0.0f;
    gekirin_time = 0;
    reward_exp = 0;
    reward_money = 0;
    unk_1322 = 0;
    whp = 0;
    defense = 0;
    unk_134c = -1;
    locate_param = 0;
    gate_key = -1;
    no_damage_cnt = 0;
    height = 0.0f;
    status.attr = 0;
    drop_badge = 0;
    next_pos[2] = 0.0f;
    next_pos[1] = 0.0f;
    next_pos[0] = 0.0f;
    next_pos[3] = 1.0f;
    move_speed = 0.0f;
    arrive_dist = 0.0f;
    unk_1488 = 0;
    unk_148c = 0;
    next_rot = 0.0f;
    rot_speed = 0.0f;
    attrib = 0;
}

BASE_MONSTER_TBL *GetMonsterTable(int id) {
    BASE_MONSTER_TBL *tbl = base_monster_define;

    while (tbl->name[0] != '\0') {
        if (tbl->id == id) {
            return tbl;
        }
        tbl++;
    }
    return NULL;
}

void CMonsterLocateInfo::SetPutFlag(int no, int put) {
    if (put != 0) {
        if (put_num < num) {
            put_flag |= 1 << no;
            put_num++;
        }
    } else if (put_num > 0) {
        put_flag &= ~(1 << no);
        put_num--;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", Initialize__11CMonsterManFP6CScene);

void CMonsterMan::DrawEffectScript() {
    int i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL) {
            active[i]->DrawEffect();
        }
    }
}

void CMonsterMan::StepEffectScript() {
    int i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL) {
            active[i]->StepEffect();
        }
    }
}

float CMonsterMan::IsBattleStyleDist() {
    int             i;
    int             user_mons_id = GetBattleCharaInfo()->unk_2;
    CActiveMonster *monster;
    float           dist;

    if (DngUserData->active_chr_no != 3 || user_mons_id == -1) {
        monster = GetPriorityLevelIndex(0, NULL);
        if (monster != NULL) {
            return monster->target_dist;
        }
        return 999999.0f;
    }
    dist = 999999.0f;
    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL && active[i]->IsDraw(1) && active[i]->tbl->user_mons_id != user_mons_id &&
            dist > active[i]->target_dist) {
            dist = active[i]->target_dist;
        }
    }
    return dist;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", CheckMonsterTolk__11CMonsterManFPf);

CActiveMonster *CMonsterMan::CheckThrowTarget(mgCFrame *frame) {
    sceVu0FVECTOR   frame_pos;
    sceVu0FVECTOR   monster_pos;
    int             i;
    CActiveMonster *monster;

    if (frame == NULL) {
        return NULL;
    }
    frame->GetWorldPosition0(frame_pos);
    frame_pos[3] = 1.0f;
    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL && active[i]->IsDraw(1) && active[i]->monster_id != 0x25A) {
            active[i]->GetPosition(monster_pos);
            monster_pos[3] = 1.0f;
            if (mgDistVector(monster_pos, frame_pos) <= 30.0f) {
                monster = active[i];
                if (monster->tbl->unk_98 & 1) {
                    MsgTaskMan.Print(dung_progtxt_notlift_mons[LanguageCode], 45, 8, 0);
                    return NULL;
                }
                monster->CObjectFrame::frame->SetReference(frame);
                active[i]->catch_frame = frame;
                active[i]->catch_state = 1;
                active[i]->no_hit_time = 0;
                active[i]->req_prog = 1100;
                return active[i];
            }
        }
    }
    return NULL;
}

int CMonsterMan::SearchBaseIndex(int id) {
    int i;

    for (i = 0; i < MONSTER_REFER_MAX; i++) {
        if (refer[i].id == id) {
            return i;
        }
    }
    return -1;
}

int CMonsterMan::GetMonsterNum(float dist) {
    int count = 0;
    int i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL && active[i]->chara_kind == ACTION_KIND_SCRIPT &&
            (active[i]->target_dist <= dist || dist < 0.0f)) {
            count++;
        }
    }
    return count;
}

BASE_MONSTER_TBL *CMonsterMan::GetReferPtr2(int id) {
    BASE_MONSTER_TBL *tbl = base_monster_define;

    while (tbl->name[0] != '\0') {
        if (tbl->id == id) {
            return tbl;
        }
        tbl++;
    }
    return NULL;
}

int CMonsterMan::SearchActiveMonsterBlock() {
    int             i;
    CActiveMonster *monster;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster == NULL) {
            return i;
        }
        if (monster->state == ACTIVE_MONSTER_NONE) {
            return i;
        }
    }
    return -1;
}

int CMonsterMan::SearchReferBlock() {
    int i;

    for (i = 0; i < MONSTER_REFER_MAX; i++) {
        if (refer[i].id == -1) {
            return i;
        }
    }
    return -1;
}

int CMonsterMan::EntryRefer(int id, mgCMemory *memory) {
    BASE_MONSTER_TBL *tbl;

    while (id != -1) {
        tbl = GetReferPtr2(id);
        if (tbl == NULL) {
            return 0;
        }
        if (!LoadReferMonsterFile(id, tbl, memory)) {
            return 0;
        }
        id = tbl->next_id;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", LoadReferMonsterFile__11CMonsterManFiP16BASE_MONSTER_TBLP9mgCMemory);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", SetActiveMonster__11CMonsterManFiPfPfi);

void CMonsterMan::DrawMiniMapSymbol(CMiniMapSymbol *symbol) {
    CBattleCharaInfo *battle_info;
    int               show_all;
    int               i;
    int               symbol_no;
    CActiveMonster   *monster;
    sceVu0FVECTOR     pos;

    if (symbol != NULL) {
        show_all = 0;
        if (scene->battle_area.unk_64 & 2) {
            show_all = 1;
        }
        battle_info = GetBattleCharaInfo();
        for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
            monster = active[i];
            if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT && monster->catch_state != 1 &&
                (monster->target_dist <= monster->clip_dist || show_all)) {
                symbol_no = 0;
                if (monster->gate_key > 0 && battle_info->GetNowNPC() == 9) {
                    symbol_no = 8;
                }
                active[i]->GetPosition(pos);
                symbol->DrawSymbol(pos, symbol_no);
            }
        }
    }
}

void CMonsterMan::DrawLifeGage(int hide_gekirin, int hide) {
    sceVu0FVECTOR   pos;
    sceVu0FVECTOR   boss_pos;
    CActionChara   *player;
    CActiveMonster *monster;
    int             boss_life;
    int             i;
    int             target_no;

    if (hide != 0) {
        return;
    }
    player = (CActionChara *)scene->GetCharacter(0);
    if (player == NULL) {
        return;
    }
    target_no = -1;
    if (player->lock_on != 0) {
        target_no = player->target_no;
    }
    boss_life = 0;
    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->state != ACTIVE_MONSTER_NONE && monster->target_dist <= monster->clip_dist) {
            if (i + MONSTER_ACTIVE_MAX == target_no) {
                monster->life_gage.SetView(1);
            } else {
                monster->life_gage.SetView(0);
            }
            active[i]->GetEntryObjectPos(0, 0, pos);
            pos[1] += active[i]->body_height;
            monster = active[i];
            if (monster->tbl->boss == 0) {
                s8 boss = monster->tbl->boss;
                monster->life_gage.Set(pos, monster->max_life, monster->life, (int)(0.9f + monster->gekirin),
                                       (s8)boss);
                active[i]->life_gage.Step();
                active[i]->life_gage.Draw(hide_gekirin);
            } else {
                boss_life += monster->life;
            }
        }
    }
    if (boss_life > 0 && boss_max_life > 0) {
        boss_life_gage.Set(boss_pos, boss_max_life, boss_life, 0, 1);
        boss_life_gage.Step();
        boss_life_gage.Draw(0);
    }
}

void CMonsterMan::DrawPiyori() {
    int             i;
    CActiveMonster *monster;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->catch_state != 1 && monster->state != ACTIVE_MONSTER_NONE) {
            monster->piyori.Draw();
            active[i]->gift_mark.Draw();
        }
    }
}

void CMonsterMan::DrawActMonster() {
    mgCTextureManager *textures = &mgTexManager;
    CActiveMonster    *monster;
    int                i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT && monster->view_state != MONSTER_VIEW_OUT &&
            monster->alpha >= 1.0f && monster->view_alpha >= 1.0f && monster->camera_alpha >= 1.0f) {
            textures->ReloadTexture(monster->refer_no + 40, (sceVif1Packet *)NULL);
            active[i]->DrawDirect();
        }
    }
}

void CMonsterMan::DrawInvisibleMonster() {
    mgCTextureManager *textures = &mgTexManager;
    int                i;
    CActiveMonster    *monster;
    float              alpha;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT) {
            float &current_alpha = monster->alpha;
            alpha = current_alpha;
            if ((alpha < 1.0f || monster->view_alpha < 1.0f || monster->camera_alpha < 1.0f) &&
                monster->view_alpha > 0.0f && monster->camera_alpha > 0.0f) {
                if (alpha >= 1.0f) {
                    current_alpha = monster->view_alpha;
                }
                active[i]->alpha *= active[i]->camera_alpha;
                textures->ReloadTexture(active[i]->refer_no + 40, (sceVif1Packet *)NULL);
                active[i]->DrawDirect();
                active[i]->alpha = alpha;
            }
        }
    }
}

void CMonsterMan::DrawShadowActMonster() {
    sceVu0FMATRIX   light_dir;
    sceVu0FMATRIX   light_color;
    int             i;
    CActiveMonster *monster;

    if (scene->GetMap(scene->active_map) == NULL) {
        return;
    }
    mgGetLight(light_dir, light_color);
    sceVu0FVECTOR shadow_dir = { light_dir[0][0], light_dir[1][0], light_dir[2][0] };
    shadow_dir[1] = shadow_dir[1] < 0.0f ? -shadow_dir[1] : shadow_dir[1];
    if (shadow_dir[1] < 0.8f) {
        shadow_dir[1] = 0.8f;
    }
    sceVu0FVECTOR normal = { 0.0f, 1.0f, 0.0f, 0.0f };
    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT && monster->catch_state != 1 &&
            monster->target_dist <= 0.6f * monster->clip_dist && monster->priority < priority_limit &&
            monster->alpha >= 0.6f) {
            sceVu0FVECTOR pos = { 0.0f, -10.0f, 0.0f, 0.0f };
            active[i]->GetEntryObjectPos(1, pos);
            pos[1] -= 20.0f;
            mgSetDropShadowMatrix(shadow_dir, pos, normal);
            active[i]->ShadowStep();
            active[i]->DrawShadowDirect();
        }
    }
}

void CMonsterMan::PriorityLevelCheck() {
    int order[MONSTER_ACTIVE_MAX];
    int count = 0;
    int i;
    int j;
    int swap;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL) {
            active[i]->priority = -1;
            if (active[i]->chara_kind == ACTION_KIND_SCRIPT && active[i]->state != ACTIVE_MONSTER_NONE) {
                order[count] = i;
                count++;
            }
        }
    }
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (active[order[i]]->target_dist > active[order[j]]->target_dist) {
                swap = order[i];
                order[i] = order[j];
                order[j] = swap;
            }
        }
    }
    for (i = 0; i < count; i++) {
        active[order[i]]->priority = i;
    }
}

CActiveMonster *CMonsterMan::GetPriorityLevelIndex(int priority, int *chara_no) {
    int i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] != NULL && active[i]->chara_kind == ACTION_KIND_SCRIPT && active[i]->priority == priority) {
            if (chara_no != NULL) {
                *chara_no = i + MONSTER_ACTIVE_MAX;
            }
            return active[i];
        }
    }
    return NULL;
}

int CMonsterMan::CheckPhoto(CScene::InScreenCharaInfo *info) {
    sceVu0FVECTOR       pos;
    sceVu0FVECTOR       rot;
    sceVu0FVECTOR       dir;
    sceVu0FMATRIX       object_matrix;
    sceVu0FMATRIX       photo_matrix;
    mgVu0FBOX           box;
    sceVu0FVECTOR       screen_max;
    sceVu0FVECTOR       screen_min;
    int                 selected = -1;
    float               nearest = 0.0f;
    float               dist;
    float               radius;
    CHARA_ENTRY_OBJECT *entry;
    int                 i;

    info->chara_no = -1;
    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        if (active[i] == NULL || active[i]->chara_kind != ACTION_KIND_SCRIPT || active[i]->alpha < 1.0f) {
            continue;
        }
        active[i]->GetRotation(rot);
        entry = active[i]->GetEntryObjectPos(0, 0, pos);
        radius = 5.0f * entry->unk_04;
        mgGetDirFromCamera(dir, pos);
        dist = mgDistVector(dir);
        if (dist > 300.0f && active[i]->tbl->boss == 0) {
            continue;
        }
        sceVu0Normalize(dir, dir);
        mgUnitMatrix(object_matrix);
        mgUnitMatrix(photo_matrix);
        sceVu0RotMatrixY(object_matrix, object_matrix, rot[1]);
        *(u_long128 *)object_matrix[3] = *(u_long128 *)pos;
        object_matrix[3][3] = 1.0f;
        *(u_long128 *)photo_matrix[3] = *(u_long128 *)pos;
        photo_matrix[3][3] = 1.0f;
        sceVu0InnerProduct(dir, object_matrix[2]);
        mgZeroVectorW(box.max);
        mgZeroVectorW(box.min);
        box.max[1] = radius;
        box.min[1] = -radius;
        box.max[0] = radius;
        box.min[0] = -radius;
        box.min[2] = -radius;
        box.max[2] = radius;
        if (mgInsideScreen(&box, photo_matrix, screen_max, screen_min) && screen_max[0] >= -50.0f &&
            screen_min[0] <= 50.0f && screen_max[1] >= -50.0f && screen_min[1] <= 50.0f &&
            (selected < 0 || nearest > dist)) {
            selected = i;
            nearest = dist;
        }
    }
    if (selected < 0) {
        return -1;
    }
    info->chara_no = active[selected]->tbl->id;
    info->dist = nearest - 10.0f;
    if (active[selected]->scoop.ok != 0) {
        return active[selected]->scoop.no;
    }
    return -1;
}

void CMonsterMan::SetNearAreaPiyori(float dist) {
    CActiveMonster *monster;
    int             i;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && monster->chara_kind == ACTION_KIND_SCRIPT && monster->target_dist <= dist &&
            monster->catch_state == 0 && monster->damage_time <= 0 && !(monster->attrib & MONSTER_ATTRIB_NO_DAMAGE)) {
            monster->req_prog = MONSTER_PROG_PIYORI;
            monster->piyori_time = 120;
        }
    }
}

int CMonsterMan::IsRunEvent() {
    int             i;
    CActiveMonster *monster;
    int             event_no;

    for (i = 0; i < MONSTER_ACTIVE_MAX; i++) {
        monster = active[i];
        if (monster != NULL && (event_no = monster->event_no) != -1) {
            monster->event_no = -1;
            return event_no;
        }
    }
    return -1;
}

void CMonsterMan::CollisionCheck(CActiveMonster *monster, float *pos, float *velocity, float *out_velocity) {
    sceVu0FVECTOR   next;
    sceVu0FVECTOR   center;
    sceVu0FVECTOR   other_pos;
    sceVu0FVECTOR   away;
    int             i;
    CActionChara   *other;
    u_int           se_id;
    float           scale;
    float           reach;
    float           dist;
    float           overlap;

    sceVu0CopyVector(out_velocity, velocity);
    next[0] = pos[0] + out_velocity[0];
    next[1] = pos[1] + out_velocity[1];
    next[2] = pos[2] + out_velocity[2];
    sceVu0CopyVector(center, next);
    center[1] = 0.0f;
    se_id = scene->se_battle_id;
    if (monster->catch_state == 1) {
        return;
    }
    for (i = 0; i < MONSTER_ACTIVE_MAX + 1; i++) {
        // Slot 0 is the player; the others are the monster slots.
        if (i == 0) {
            other = (CActionChara *)scene->GetCharacter(0);
        }
        if (i != 0) {
            other = active[i - 1];
        }
        if (other == NULL || other == monster || other->chara_kind != ACTION_KIND_SCRIPT ||
            (monster->catch_state == 2 && i == 0)) {
            continue;
        }
        other->GetPosition(other_pos);
        scale = 1.0f;
        if (monster->catch_state == 2 && i != 0) {
            other->GetEntryObjectPos(0, other_pos);
            scale = 2.0f;
        }
        if (next[1] + monster->body_height * scale < other_pos[1] ||
            next[1] > other_pos[1] + other->body_height * scale) {
            continue;
        }
        other_pos[1] = 0.0f;
        reach = 2.0f * other->body_width + 2.0f * monster->body_width * scale;
        dist = mgDistVector(other_pos, center);
        if (dist < reach) {
            if (monster->catch_state == 2 && i != 0) {
                // A thrown monster stuns both itself and the monster it hits.
                CActiveMonster *hit_monster = (CActiveMonster *)other;
                printf("HIT!!\n");
                hit_monster->req_prog = MONSTER_PROG_PIYORI;
                hit_monster->piyori_time = 120;
                sceVu0ScaleVectorXYZ(out_velocity, out_velocity, -0.8f);
                sceVu0CopyVector(other->blow_vec, out_velocity);
                other->blow_vec[1] = 0.0f;
                sceVu0Normalize(other->blow_vec, other->blow_vec);
                sceVu0ScaleVectorXYZ(other->blow_vec, other->blow_vec, -1.0f);
                other->blow_speed = 2.0f;
                other->blow_rate = 1.0f;
                other->blow_decel = 0.0f;
                other->blow_time = 3;
                HitEffectSet(scene, pos, 0);
                sndSePlay(se_id, 0x1C, 0);
                monster->req_prog = MONSTER_PROG_PIYORI;
                monster->piyori_time = 30;
                monster->catch_state = 0;
            }
            if (i != 0) {
                overlap = reach - dist;
                away[0] = center[0] - other_pos[0];
                away[1] = center[1] - other_pos[1];
                away[2] = center[2] - other_pos[2];
                away[3] = 1.0f;
                sceVu0Normalize(away, away);
                out_velocity[0] = (out_velocity[0] + away[0] * overlap) / 2.0f;
                out_velocity[1] = (out_velocity[1] + away[1] * overlap) / 2.0f;
                out_velocity[2] = (out_velocity[2] + away[2] * overlap) / 2.0f;
                out_velocity[3] = 1.0f;
                sceVu0ScaleVector(out_velocity, out_velocity, 1.5f);
            } else {
                out_velocity[2] = 0.0f;
                out_velocity[1] = 0.0f;
                out_velocity[0] = 0.0f;
                out_velocity[3] = 1.0f;
            }
        }
    }
}

float SearchArea(CScene *scene, float *from, float *to, float dist) {
    CCPoly        polys[128];
    mgVu0FBOX     box;
    sceVu0FVECTOR hit;
    CMap         *map = scene->GetMap(scene->active_map);

    if (map == NULL) {
        return dist;
    }
    float margin = 10.0f + dist;
    box.max[0] = margin + from[0];
    box.min[0] = from[0] - margin;
    box.max[1] = margin + from[1];
    box.min[1] = from[1] - margin;
    box.max[2] = margin + from[2];
    box.min[2] = from[2] - margin;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    int count = map->GetColPoly(polys, box, 128);
    if (CheckHit(polys, count, from, to, hit, 1, 2) < 0) {
        return dist;
    }
    return mgDistVector(hit, from);
}

/**
 * Starts the sparks and flash of a hit on a monster, moved towards the camera.
 */
#ifdef NONMATCHING
static void HitEffectSet(CScene *scene, float *pos, int flags) {
    sceVu0FVECTOR    to_camera;
    sceVu0FVECTOR    point;
    mgCCamera       *camera;
    CHitEffectImage *hit;
    CFlushEffect    *flush;

    camera = scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    sceVu0CopyVector(point, pos);
    camera->GetPos(to_camera);
    sceVu0SubVector(to_camera, to_camera, point);
    sceVu0Normalize(to_camera, to_camera);
    sceVu0ScaleVector(to_camera, to_camera, 20.0f);
    sceVu0AddVector(point, point, to_camera);
    sceVu0FVECTOR dir = { 0.0f, 1.0f, 0.0f, 1.0f };

    if (BattleFX.hit == NULL) {
        hit = NULL;
    } else {
        hit = &BattleFX.hit[BattleFX.hit_next];
        BattleFX.hit_next++;
        if (BattleFX.hit_next >= BattleFX.hit_num) {
            BattleFX.hit_next = 0;
        }
    }
    if (hit != NULL) {
        hit->SethitEffect(point, dir, 30.0f, 60.0f, 0.2f, 0.1f, 30, 32);
        hit->kind = 0;
        mgRect<int> rect(32, 0, 32, 32);
        hit->tex_rect = rect;
    }

    if (BattleFX.flush == NULL) {
        flush = NULL;
    } else {
        flush = &BattleFX.flush[BattleFX.flush_next];
        BattleFX.flush_next++;
        if (BattleFX.flush_next >= BattleFX.flush_num) {
            BattleFX.flush_next = 0;
        }
    }
    if (flags & 2) {
        if (flush != NULL) {
            sceVu0CopyVector(flush->pos, point);
            flush->fade_speed = 8.0f;
            flush->alpha = 160;
            flush->active = 1;
            flush->size = 16.0f;
            flush->grow = 2.0f;
            flush->tex_u = 128;
            flush->tex_v = 128;
            flush->tex_size = 128;
            flush->follow = NULL;
        }
    } else if (flush != NULL) {
        sceVu0CopyVector(flush->pos, point);
        flush->fade_speed = 10.0f;
        flush->alpha = 160;
        flush->active = 1;
        flush->size = 15.0f;
        flush->grow = 1.5f;
        flush->tex_u = 128;
        flush->tex_v = 128;
        flush->tex_size = 128;
        flush->follow = NULL;
    }
    if (BattleFX.hit == NULL) {
        hit = NULL;
    } else {
        hit = &BattleFX.hit[BattleFX.hit_next];
        BattleFX.hit_next++;
        if (BattleFX.hit_next >= BattleFX.hit_num) {
            BattleFX.hit_next = 0;
        }
    }
    if (hit != NULL) {
        hit->SethitEffect(point, dir, 60.0f, 45.0f, 0.0f, 0.0f, 15, 16);
        hit->kind = 2;
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", HitEffectSet__FP6CScenePfi);
#endif

/**
 * Starts the sparks and flash of a guarded hit, moved towards the camera, with an optional effect script.
 */
#ifdef NONMATCHING
static void GuardEffectSet(CScene *scene, float *pos, int play_script) {
    sceVu0FVECTOR    to_camera;
    sceVu0FVECTOR    point;
    mgCCamera       *camera;
    CHitEffectImage *hit;
    CFlushEffect    *flush;

    camera = scene->GetCamera(scene->active_camera);
    if (camera == NULL) {
        return;
    }
    sceVu0CopyVector(point, pos);
    camera->GetPos(to_camera);
    sceVu0SubVector(to_camera, to_camera, point);
    sceVu0Normalize(to_camera, to_camera);
    sceVu0ScaleVector(to_camera, to_camera, 20.0f);
    sceVu0AddVector(point, point, to_camera);
    sceVu0FVECTOR dir = { 0.0f, 1.0f, 0.0f, 1.0f };

    if (BattleFX.hit == NULL) {
        hit = NULL;
    } else {
        hit = &BattleFX.hit[BattleFX.hit_next];
        BattleFX.hit_next++;
        if (BattleFX.hit_next >= BattleFX.hit_num) {
            BattleFX.hit_next = 0;
        }
    }
    hit->SethitEffect(point, dir, 50.0f, 30.0f, 0.0f, 0.1f, 30, 32);
    hit->kind = 1;

    if (BattleFX.flush == NULL) {
        flush = NULL;
    } else {
        flush = &BattleFX.flush[BattleFX.flush_next];
        BattleFX.flush_next++;
        if (BattleFX.flush_next >= BattleFX.flush_num) {
            BattleFX.flush_next = 0;
        }
    }
    if (flush != NULL) {
        sceVu0CopyVector(flush->pos, point);
        flush->fade_speed = 16.0f;
        flush->alpha = 160;
        flush->active = 1;
        flush->size = 10.0f;
        flush->grow = 3.0f;
        flush->tex_u = 64;
        flush->tex_v = 192;
        flush->tex_size = 64;
        flush->follow = NULL;
    }
    if (play_script != 0) {
        FxScriptMan->CreateEffSpt("\x83\x4B\x81\x5B\x83\x68\x83\x47\x83\x74\x83\x46\x83\x4E\x83\x67\x82\x60", 0, 0);
        FxScriptMan->SetScriptVect1(point, 0, -1);
    }
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", GuardEffectSet__FP6CScenePfi);
#endif

/**
 * Shows a damage number, or the no-damage or guard mark, over a monster.
 */
static void HitScoreSet(float *pos, int type, int value) {
    int       *no_score = no_score_uv[LanguageCode];
    int       *guard_score = guard_score_uv[LanguageCode];
    static int dmg_sc_cnt = 0;

    switch (type) {
        case 0:
            DamageScoreMons[dmg_sc_cnt].SetValue(pos, value);
            break;
        case 1:
            DamageScoreMons[dmg_sc_cnt].SetSprite(pos, no_score[0], no_score[1], no_score[2], no_score[3]);
            break;
        case 2:
            DamageScoreMons[dmg_sc_cnt].SetSprite(pos, guard_score[0], guard_score[1], guard_score[2], guard_score[3]);
            break;
    }
    if (dmg_sc_cnt == 7) {
        dmg_sc_cnt = 0;
    } else {
        dmg_sc_cnt++;
    }
}

/**
 * Says whether a collision primitive carries the gift that a monster's gift type takes in every slot.
 */
static int CheckGiftPack(CActiveMonster *monster, CColPrim *prim) {
    int gift_type = monster->tbl->gift_type;
    int row = 0;
    int i;

    while (gift_item_tbl[row][0] != -1) {
        if (gift_item_tbl[row][0] == gift_type) {
            break;
        }
        row++;
    }
    if (gift_item_tbl[row][0] == -1) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (prim->gift[i] != gift_item_tbl[row][1]) {
            return 0;
        }
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", CheckDamage__11CMonsterManFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", MoveUnit__11CMonsterManFP14CActiveMonsterP6CCPolyi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/monster", ThinkHost__11CMonsterManFv);

/**
 * Sets the name of a monster kind from the monster name file.
 */
static int _MONSTER_NAME(SPI_STACK *stack, int count) {
    char              name[128];
    int               id;
    char             *text;
    BASE_MONSTER_TBL *tbl;

    id = spiGetStackInt(stack++);
    text = spiGetStackString(stack);
    tbl = GetMonsterTable(id);
    if (tbl == NULL) {
        return 0;
    }
    memset(name, 0, sizeof(name));

    if (LanguageCode >= 2 && LanguageCode < 6) {
        ConvertFontCode(text, name);
    } else {
        strcpy(name, text);
    }

    if (strlen(name) > 31) {
        return 0;
    }
    strcpy(tbl->name, name);
    return 1;
}

void LoadMonsterLanguage(int language) {
    char  text[0x4000];
    char  path[64];
    int   size;
    char *script = text;

    sprintf(path, "dungeon/cfg_file/mosdata%d.cfg", language);
    if (LoadFile2(path, script, &size, 0)) {
        CScriptInterpreter interpreter;

        interpreter.SetTag(mos_data_anlyze_tag);
        interpreter.SetScript(script, size);
        interpreter.Run();
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", base_monster_define__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", dung_progtxt_notlift_mons__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1707__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1724__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2031__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2079__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", no_score_uv__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", guard_score_uv__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", vs_attk_index__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", gift_item_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", react_tbl__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2183__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2294__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2699__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", mos_data_anlyze_tag__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1200__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1201__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1202__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1203__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1204__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1205__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1421__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1422__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1423__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1424__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1425__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1426__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1427__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1428__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1429__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1430__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1431__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_1999__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2100__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2485__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2486__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2487__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2488__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2588__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2589__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2802__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", at_2809__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/monster", __vt__14CActiveMonster__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(dmg_sc_cnt_2104, 0x4);
INCLUDE_BSS(init_2105, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1704, 0x10);
