#include "common.h"
#include "dng_event.hpp"
#include "mg_drawprim.hpp"
#include "effscript.hpp"
#include "maintex.hpp"
#include "monster.hpp"
#include "font.hpp"
#include "mainloop.hpp"
#include "cameracontrol.hpp"
#include "event_func.hpp"
#include "event.hpp"
#include "menucommon.hpp"
#include "gameutil.hpp"
#include "dataread.hpp"
#include "quest.hpp"
#include "mapload.hpp"
#include "mglib.hpp"
#include "water.hpp"
#include "editriver.hpp"
#include "snd_mngr.hpp"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include "savedatadungeon.hpp"
#include "sceneevent.hpp"
#include "snd_seseq.hpp"
#include "mg_texture.hpp"
#include "mg_math.hpp"
#include "dng_effect.hpp"
#include "dng_status.hpp"
#include "dng_debug.hpp"
#include "dng_main.hpp"
#include "actionchara.hpp"
#include "automap.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "mapinfo.hpp"
#include "mapparts.hpp"
#include "mg_drawenv.hpp"
#include "mg_frame.hpp"
#include "mg_memory.hpp"
#include "nd_meswin.hpp"
#include "object.hpp"
#include "scenesnd.hpp"
#include "scriptinterpreter.hpp"
#include "userdata.hpp"
#include <cstring>


static void StatusWarningSnd(void);
static void BattleAreaBGMCtrl(void);
static int _GROUP_START(SPI_STACK *stack, int argc);
static int _GROUP(SPI_STACK *stack, int argc);
static int _ITEM(SPI_STACK *stack, int argc);
static int _FLOOR_START(SPI_STACK *stack, int argc);
static int _FLOOR(SPI_STACK *stack, int argc);
static void PickupRandomItemCheckMax(TRESURE_BOX_FLOOR_INFO *table, int floor_index);
static TRESURE_BOX_ITEM *PickupRandomItem(TRESURE_BOX_FLOOR_INFO *table, int floor_index, int value);
static int CheckObjectPutArea(float *pos);
static int _FLE(SPI_STACK *stack, int argc);
static void CreatMonsterFloorInfo(char *script, int length);

static int gatekey_index[7] = {0x151, 0x153, 0x155, 0x156, 0x158, 0x15C, 0x15E};
static int keydoor_key_index[7] = {0x152, 0x154, -1, 0x157, 0x15A, 0x15D, 0x15F};
static float xchg_rot_list[4] = {0.0f, -1.5707964f, 3.1415927f, 1.5707964f};
static int FLS_FLOOR_ID;
static TRESURE_BOX_FLOOR_INFO *nowTbFloor;
static int nowTboxGroup;
static int nowTboxItemCnt;

static int _FLS(SPI_STACK *stack, int argc);
static int _FL(SPI_STACK *stack, int argc);

static SPI_TAG_PARAM tag[6] = {
    {"GROUP_START", _GROUP_START}, {"GROUP", _GROUP}, {"ITEM", _ITEM},
    {"FLOOR_START", _FLOOR_START}, {"FLOOR", _FLOOR}, {NULL, NULL}
};
static SPI_TAG_PARAM tag2[4] = {
    {"FLS", _FLS}, {"FL", _FL}, {"FLE", _FLE}, {NULL, NULL}
};

// Code (.text)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", DrawEpisode__20CStartupEpisodeTitleFii);
#ifdef NONMATCHING
void CStartupEpisodeTitle::Switch(int state) {
    char   *title;
    ClsMes *current;
    int    title_width;
    int    floor_id;

    floor_id = DngSaveDataDungeon->floor_id[DngSaveDataDungeon->stage_id];

    if (state != 0) {
        mes->abs_win.x = 0x22;
        mes->abs_win.y = 0x154;
        mes->font_w = 0x12;
        title = DngMainScene->battle_area.floor_manager.GetFloorTitle(floor_id);
        mes->MakeMesWin(title, 1, 1);
        title_width = (s16)mes->GetStrWidth(title);
        width = title_width - 2;
        if (width < 0x9A) {
            width = 0x9A;
        }
        mes->abs_win.x = (width / 2 + 0x24) - title_width / 2;
        mes->open = 1;
        alpha = 0;
        reveal = 0;
        slide = 0;
        wait = 0x3C;
    } else if (this->state != 0) {
        current = mes;
        current->draw_speed = current->GetDrawSpeedDef();
        current->mes_no = -1;
        current->unk_1e40 = 0;
        current->open = 0;
        current->fade = 0.0f;
        current->fukidashi_centre_x = -1;
        current->fukidashi_centre_y = -1;
    }
    this->state = state;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", Switch__20CStartupEpisodeTitleFi);
#endif

void CStartupEpisodeTitle::Step(void) {
    ClsMes *mes_win;
    int    top;

    if (mes == NULL || !state) {
        return;
    }
    if (state == 1) {
        alpha += 0.033333335f;
        if (alpha >= 1.0f) {
            alpha = 1.0f;
        }
        reveal += 0.033333335f;
        if (reveal >= 1.0f) {
            reveal = 1.0f;
        }
        if (alpha >= 0.2f) {
            slide += 0.025f;
            if (slide >= 1.0f) {
                slide = 1.0f;
                wait = 0x3C;
                state = 2;
            }
        }
    }
    if (state == 2) {
        wait -= 1;
        if (wait < 0) {
            state = 3;
        }
    }
    if (state == 3) {
        reveal -= 0.05f;
        if (reveal <= 0.0f) {
            reveal = 0.0f;
        }
        slide -= 0.05f;
        if (slide <= 0.0f) {
            slide = 0.0f;
        }
        if (reveal < 0.5f) {
            alpha -= 0.04f;
            if (alpha <= 0.0f) {
                mes_win = mes;
                mes_win->draw_speed = mes_win->GetDrawSpeedDef();
                mes_win->mes_no = -1;
                mes_win->unk_1e40 = 0;
                mes_win->open = 0;
                mes_win->fade = 0.0f;
                mes_win->fukidashi_centre_x = -1;
                mes_win->fukidashi_centre_y = -1;
                state = 0;
            }
        }
    }
    top = mgScreenHeight - 0x4C;
    mes->abs_win.y = (top + 0x15) - (int)(22.0f * slide);
    mes_win = mes;
    mes_win->scissor_on = 2;
    mes_win->scissor.x = 1;
    mes_win->scissor.y = top - 2;
    mes_win->scissor.width = 0x1A0;
    mes_win->scissor.height = 0x15;
    mes->Step();
}

void CStartupEpisodeTitle::Initialize(void) {
    mes = NULL;
    state = 0;
    wait = 0;
}

void MessageTaskManager::Draw(void) {
    ClsMes *current;

    current = mes;
    if (current != NULL) {
        current->DrawMesWin();
    }
}

void MessageTaskManager::Step(void) {
    ClsMes       *current;
    MESSAGE_TASK *line;

    current = this->mes;
    if (current != NULL && !(this->flag & 1)) {
        line = this->top;
        if (line != NULL) {
            if (line->count <= 0) {
                current->fukidashi_pos = line->slot;
                this->mes->MakeMesWin(this->top->message,
                                         1, 1);
                this->mes->open = 1;
            }
            this->top->count += 1;
            this->top->time -= 1;
            if (this->top->time <= 0) {
                current = this->mes;
                current->draw_speed = current->GetDrawSpeedDef();
                current->mes_no = -1;
                current->unk_1e40 = 0;
                current->open = 0;
                current->fade = 0.0f;
                current->fukidashi_centre_x = -1;
                current->fukidashi_centre_y = -1;
                this->top->message = NULL;
                this->top = this->top->next;
            }
            this->mes->Step();
        }
    }
}

void MessageTaskManager::Print(char *message, int time, int slot, int priority) {
    MESSAGE_TASK *node;
    MESSAGE_TASK *head;
    MESSAGE_TASK *prev;
    MESSAGE_TASK *next;
    int          i;

    if (this->mes != NULL) {
        node = NULL;
        for (i = 0; i < 6; i++) {
            if (task[i].message == NULL) {
                node = &task[i];
                break;
            }
        }
        if (node != NULL) {
            strcpy(node->text, message);
            node->message = node->text;
            node->priority = priority;
            node->slot = slot;
            node->time = time;
            node->count = 0;
            head = this->top;
            if (head == NULL) {
                this->top = node;
                node->next = NULL;
            } else if (priority < head->priority) {
                node->next = head;
                this->top = node;
            } else {
                prev = head;
                while ((next = prev->next) != NULL) {
                    if (priority < next->priority) {
                        node->next = next;
                        prev->next = node;
                        return;
                    }
                    prev = next;
                }
                prev->next = node;
                node->next = NULL;
            }
        }
    }
}

void MessageTaskManager::Clear(void) {
    ClsMes       *current;
    MESSAGE_TASK *line;

    current = this->mes;
    if (current != NULL) {
        line = this->top;
        if (line != NULL) {
            if (line->time > 0) {
                current->draw_speed = current->GetDrawSpeedDef();
                current->mes_no = -1;
                current->unk_1e40 = 0;
                current->open = 0;
                current->fade = 0.0f;
                current->fukidashi_centre_x = -1;
                current->fukidashi_centre_y = -1;
                this->mes->Step();
            }
            this->task[0].message = 0;
            this->task[0].message = 0;
            this->task[0].message = 0;
            this->task[0].message = 0;
            this->task[0].message = 0;
            this->task[0].message = 0;
            this->top = 0;
            this->flag = 0;
        }
    }
}

void MessageTaskManager::Initialize(void) {
    mes = NULL;
    top = NULL;
    flag = 0;
    task[0].message = NULL;
    task[0].priority = 0;
    task[0].time = 0;
    task[0].slot = 8;
    task[0].next = NULL;
    task[1].message = NULL;
    task[1].priority = 0;
    task[1].time = 0;
    task[1].slot = 8;
    task[1].next = NULL;
    task[2].message = NULL;
    task[2].priority = 0;
    task[2].time = 0;
    task[2].slot = 8;
    task[2].next = NULL;
    task[3].message = NULL;
    task[3].priority = 0;
    task[3].time = 0;
    task[3].slot = 8;
    task[3].next = NULL;
    task[4].message = NULL;
    task[4].priority = 0;
    task[4].time = 0;
    task[4].slot = 8;
    task[4].next = NULL;
    task[5].message = NULL;
    task[5].priority = 0;
    task[5].time = 0;
    task[5].slot = 8;
    task[5].next = NULL;
}

void CRedMarkModel::Draw(void) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR saved_position;

    if (draw_request != 0) {
        GetPosition(position);
        GetPosition(saved_position);
        position[1] += 2.0f * sinf(angle);
        SetPosition(position);
        CObjectFrame::DrawDirect();
        SetPosition(saved_position);
        draw_request = 0;
    }
}

void CRedMarkModel::Step(void) {
    float next;

    angle += 0.19634955f;
    next = angle;
    if (next > 0.0f) {
        angle = next - 3.1415927f;
    }
}

void CGeoStone::GeoDraw(float *player_pos) {
    sceVu0FVECTOR home_position;
    sceVu0FVECTOR draw_position;

    if (this->flag != 0) {
        GetPosition(home_position);
        GetPosition(draw_position);
        if (mgDistVector(player_pos, draw_position) < 1000.0f || this->anime == 0) {
            if (this->anime != 0) {
                draw_position[1] += 3.0f * sinf(this->angle);
            }
            SetPosition(draw_position);
            CCharacter2::DrawDirect();
        }
        SetPosition(home_position);
    }
}

void CGeoStone::DrawMiniMapSymbol(CMiniMapSymbol *symbol_drawer) {
    sceVu0FVECTOR position;

    if (this->flag != 0) {
        GetPosition(position);
        symbol_drawer->DrawSymbol(position, MINIMAP_SYMBOL_GEOSTONE);
    }
}

void CGeoStone::SetFlag(int flag) {
    CMapParts *object;

    this->flag = flag;
    if (this->flag == 0 && (object = AutoMapGen.gio_parts) != NULL) {
        sceVu0FVECTOR hidden_position = {0.0f, -99999.0f, 0.0f, 1.0f};
        object->SetPosition(hidden_position);
    }
}

void CGeoStone::GeoStep(void) {
    float next;

    if (this->flag != 0) {
        CCharacter2::Step();
        this->angle += 0.05235988f;
        next = this->angle;
        if (next > 3.1415927f) {
            this->angle = next - 6.2831855f;
        }
    }
}

int CGeoStone::CheckEvent(float *pos) {
    sceVu0FVECTOR position;

    if (this->flag == 0) {
        return 0;
    }
    GetPosition(position);
    position[1] -= 20.0f;
    if (mgDistVector(pos, position) <= 30.0f) {
        return 1;
    }
    return 0;
}

void CGeoStone::Initialize(void) {
    CCharacter2::Initialize();
    this->flag = 0;
}

void CRandomCircle::Draw(float *view_pos) {
    int id;
    for (id = 0; id < 3; id++) {
        if (active[id] != 0 &&
            mgDistVector(view_pos, this->pos[id]) < 1000.0f) {
            model.SetPosition(this->pos[id]);
            model.SetRotation(0.0f, 0.0f, 0.0f);
            model.DrawDirect();
        }
    }
}

void CRandomCircle::Step() {
    model.Step();
}

void CRandomCircle::DrawSymbol(CMiniMapSymbol *mini_map) {
    int id;
    for (id = 0; id < 3; id++) {
        if (active[id] != 0) {
            mini_map->DrawSymbol(this->pos[id], MINIMAP_SYMBOL_RANDOM_CIRCLE);
        }
    }
}

int CRandomCircle::CheckArea(float *pos, float radius) {
    int id;
    for (id = 0; id < 3; id++) {
        if (active[id] != 0 &&
            mgDistVector(this->pos[id], pos) < radius) {
            return 0;
        }
    }
    return 1;
}

int CRandomCircle::GetPosition(float *out_pos, int index) {
    if (index == -1) {
        if (this->hit == -1) {
            return 0;
        }
        sceVu0CopyVector(out_pos, this->pos[hit]);
        return 1;
    }
    if (index < 0 || index >= 3) {
        return 0;
    }
    sceVu0CopyVector(out_pos, this->pos[index]);
    return 1;
}

int CRandomCircle::CheckEvent(float *pos) {
    int id;
    for (id = 0; id < 3; id++) {
        if ((active[id] != 0) &&
            (mgDistVector((this->pos[id]), pos) <= 20.0f)) {
            hit = id;
            return id;
        }
    }
    hit = -1;
    return -1;
}

int CRandomCircle::SetCircle(float *pos) {
    int id;
    for (id = 0; id < 3; id++) {
        if (active[id] == 0) {
            sceVu0CopyVector(this->pos[id], pos);
            this->pos[id][3] = 1.0f;
            this->active[id] = 1;
            return id;
        }
    }
    return -1;
}

void CRandomCircle::Clear() {
    active[0] = 0;
    active[1] = 0;
    active[2] = 0;
    hit = -1;
}

void CRandomCircle::Initialize() {
    model.Initialize();
    this->active[0] = 0;
    this->active[1] = 0;
    this->active[2] = 0;
    this->hit = -1;
}

void CTreasureBox::Draw(float *view_pos) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR rotation;

    if (this->frame != NULL) {
        this->GetPosition(position);
        this->GetRotation(rotation);
        this->lid_frame->SetRotation(45.0f * (-3.1415927f * this->lid_open / 180.0f), 0.0f, 0.0f);
        this->lid_frame->SetPosition(0.0f, 9.0f, -7.4f);
        if (mgDistVector(view_pos, position) < 1000.0f) {
            this->model->SetPosition(position);
            this->model->SetRotation(rotation);
            this->model->DrawDirect();
        }
    }
}

void CTreasureBox::DrawShadow(float *camera_pos, float *light_dir) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR shadow_position;
    sceVu0FVECTOR rotation;

    if (this->model != NULL) {
        sceVu0FVECTOR up = {0.0f, 1.0f, 0.0f, 0.0f};
        this->GetPosition(shadow_position);
        shadow_position[1] -= 20.0f;
        mgSetDropShadowMatrix(light_dir, shadow_position, up);
        this->GetPosition(position);
        position[1] += 2.0f;
        this->GetRotation(rotation);
        if (mgDistVector(camera_pos, position) < 1000.0f) {
            this->model->SetPosition(position);
            this->model->SetRotation(rotation);
            this->model->DrawShadowDirect();
        }
    }
}

void CTreasureBoxManager::SetLargeModel(CCharacter2 *model, int tex_block) {
    mgCFrame     *frame;
    mgCFrame     *found;
    CTreasureBox *entry;
    int          i;

    this->tex_block = tex_block;
    this->model = model;
    frame = model->CObjectFrame::frame;
    if (frame != NULL) {
        found = frame->SearchFrame("tbox1");
        if (found != NULL) {
            entry = box;
            for (i = 0; i < 0x18; i++) {
                entry->lid_frame = found;
                entry->frame = frame;
                entry->model = model;
                entry++;
            }
        }
    }
}

void CTreasureBoxManager::SetCollisionModel(u32 *pack, mgCMemory *memory) {
    col_frame = LoadCollisionFile((MDS_HEADER *)GetPackFile(pack, "tbox_a.mds", NULL), memory);
}

void CTreasureBoxManager::PutTreasureBox(int index, float *pos, float rot_y, int flags, int item0, int num0, int item1, int num1) {
    int          i;
    CTreasureBox *chest;

    if (index == -1) {
        for (i = 0; i < 0x18; i++) {
            chest = &box[i];
            if (chest->state == 0) {
                index = i;
                break;
            }
        }
    }
    if (index < 0 || index >= 0x18) {
        return;
    }
    chest = &box[index];
    chest->state = 1;
    chest->SetPosition(pos);
    chest->SetRotation(0.0f, rot_y, 0.0f);
    chest->flags = flags;
    chest->item[0] = item0;
    chest->item[1] = item1;
    chest->num[0] = num0;
    chest->num[1] = num1;
}

int CTreasureBoxManager::CheckArea(float *pos, float radius) {
    sceVu0FVECTOR chest_pos;
    int           i;
    CTreasureBox  *slot;
    for (i = 0; i < 24; i++) {
        slot = &this->box[i];
        if (slot->state != 0) {
            slot->GetPosition(chest_pos);
            if (mgDistVector(chest_pos, pos) < radius) {
                return 0;
            }
        }
    }
    return 1;
}

void CTreasureBoxManager::DrawMiniMapSymbol(CMiniMapSymbol *symbol_drawer) {
    sceVu0FVECTOR chest_pos;
    int           i;
    CTreasureBox  *slot;
    for (i = 0; i < 0x18; i++) {
        slot = &this->box[i];
        if (slot->state == 1) {
            slot->GetPosition(chest_pos);
            symbol_drawer->DrawSymbol(chest_pos, 1);
        }
    }
}

void CTreasureBoxManager::Draw(float *view_pos) {
    int          i;
    CTreasureBox *slot;
    for (i = 0; i < 0x18; i++) {
        slot = &this->box[i];
        if (slot->state != 0) {
            slot->Draw(view_pos);
        }
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", DrawShadow__19CTreasureBoxManagerFPf);
int CTreasureBoxManager::PickupCollision( float *pos, CCPoly *poly, mgVu0FBOX box, int max) {
    int           count;
    int           i;
    CTreasureBox  *slot;
    sceVu0FVECTOR chest_pos;
    sceVu0FVECTOR rotation;
    i = 0;
    count = 0;
    do {
        slot = &this->box[i];
        if (slot->state != 0) {
            slot->GetPosition(chest_pos);
            if (mgDistVector(chest_pos, pos) <= 40.0f) {
                this->col_frame->SetPosition(chest_pos);
                slot->GetRotation(rotation);
                this->col_frame->SetRotation(rotation);
                count += this->col_frame->PickUpNearPoly(&poly[count], box, max);
            }
        }
        i += 1;
    } while (i < 0x18);
    return count;
}

int CTreasureBoxManager::MimicCount() {
    int          count;
    int          i;
    CTreasureBox *slot;

    count = 0;
    for (i = 0; i < 0x18; i++) {
        slot = &box[i];
        if (slot->state == 1 && (slot->flags & 0x100)) {
            count += 1;
        }
    }
    return count;
}

int CTreasureBoxManager::CheckEvent(float *pos, float dist) {
    sceVu0FVECTOR chest_pos;
    float         nearest;
    int           i;
    CTreasureBox  *slot;
    float         distance;

    this->near_box = -1;
    nearest = 9999.0f;
    for (i = 0; i < 0x18; i++) {
        slot = &this->box[i];
        if (slot->state == 1) {
            slot->GetPosition(chest_pos);
            distance = mgDistVector(chest_pos, pos);
            if (distance < dist && nearest > distance) {
                nearest = distance;
                this->near_box = i;
            }
        }
    }
    return this->near_box;
}

int GetGateKeyIndex(int dungeon, int floor) {
    if (dungeon == 4 && floor >= 0x11) {
        return 0x159;
    }
    return gatekey_index[dungeon];
}

int GetKeyDoorIndex(int dungeon, int floor) {
    if (dungeon == 4 && floor >= 0x11) {
        return 0x15B;
    }
    return keydoor_key_index[dungeon];
}

int Lamb2WolfManager(void) {
    CBattleCharaInfo *info;
    int              form;
    CActionChara     *chara;
    mgCFrame         *wolf;
    mgCFrame         *lamb;
    mgCFrame         *object;
    sceVu0FVECTOR    rotation;

    info = GetBattleCharaInfo();
    if (info->chr_no != 1) {
        return -1;
    }
    form = info->equip->item_no;
    if (form != 0x38 && form != 0x58) {
        return -1;
    }
    chara = (CActionChara *)DngMainScene->GetCharacter(0);
    if (chara == NULL) {
        return -1;
    }
    if (form == 0x58) {
        object = chara->SearchObject("parts01");
        if (object != NULL) {
            object->GetRotation(rotation);
            rotation[1] += 0.27925268f;
            rotation[1] = mgAngleLimit(rotation[1]);
            object->SetRotation(rotation);
        }
        return 0;
    }
    lamb = chara->SearchObject("w15a");
    wolf = chara->SearchObject("w15b");
    if (lamb == NULL || wolf == NULL) {
        return -1;
    }
    if (GetTimeBand(DngMainScene->time) == 2) {
        lamb->SetAttrParamDraw(0, 0);
        wolf->SetAttrParamDraw(1, 0);
        return 1;
    }
    lamb->SetAttrParamDraw(1, 0);
    wolf->SetAttrParamDraw(0, 0);
    return 0;
}

void LoopSoundManager(s32 sound_id) {
    (void)sound_id;
}

void BattleSoundManager(void) {
    BattleAreaBGMCtrl();
    StatusWarningSnd();
}

/**
 * Plays a warning when the active character has little health.
 */
static void StatusWarningSnd(void) {
    static int       counter;
    CBattleCharaInfo *info;
    float            ratio;

    if (!(DngMainScene->battle_area.pause_flag & 0x400)) {
        if (counter < 0x14) {
            counter += 1;
        } else {
            counter = 0;
            info = GetBattleCharaInfo();
            ratio = (float)info->GetNowHp_i();
            ratio /= (float)info->GetMaxHp_i();
            if (ratio > 0.0f && ratio < 0.3f) {
                sndSePlay(DngMainScene->se_battle_id, 10, 0);
                if (ratio < 0.15f) {
                    counter = 10;
                }
            }
        }
    }
}

/**
 * Fades between the map and battle music as monsters approach.
 */
static void BattleAreaBGMCtrl(void) {
    CActionChara    *player;
    DNG_BATTLE_AREA *state;
    float           distance;
    int             phase;
    float           fade;
    float           rate;
    CScene          *scene;

    state = (DNG_BATTLE_AREA *)&DngMainScene->battle_area;
    player = (CActionChara *)DngMainScene->GetCharacter(0);
    if (dngGetDebugInfo()->sound_flag == 0) {
        sndSeStop(EdEventInfo.snd_id[4], 0, 0);
        return;
    }
    distance = 9999999.0f;
    if (ActiveMonster != NULL) {
        distance = ActiveMonster->IsBattleStyleDist();
    }
    if (distance <= 340.0f) {
        player->unk_75e = 1;
    } else {
        player->unk_75e = 0;
    }
    if (!(state->pause_flag & 0x4000) && state->boss_map == 0) {
        phase = state->battle_bgm_state;
        fade = state->battle_bgm_vol;
        if (phase == 0) {
            if (!(340.0f < distance)) {
                state->battle_bgm_state = 1;
            }
        }
        if (phase == 2) {
            if (400.0f <= distance) {
                state->battle_bgm_state = 3;
            }
        }
        switch (phase) {
            case 1:
                fade += 0.05f;
                if (!(fade < 1.0f)) {
                    state->battle_bgm_state = 2;
                    fade = 1.0f;
                    sndSePlay(EdEventInfo.snd_id[4], 0, 0);
                    DngMainScene->PauseBGM();
                }
                scene = DngMainScene;
                DngMainScene->GetActiveBgmInfo()->unk_c = 1.0f - fade;
                scene->SetVolfBGM(scene->GetActiveBgmInfo()->volf);
                state->battle_bgm_vol = fade;
                return;
            case 3:
                fade -= 0.016666668f;
                if (fade <= 0.0f) {
                    state->battle_bgm_state = 4;
                    fade = 0.0f;
                    sndSeStop(EdEventInfo.snd_id[4], 0, 0);
                    DngMainScene->RePlayBGM();
                    scene = DngMainScene;
                    DngMainScene->GetActiveBgmInfo()->unk_c = 0.0f;
                    scene->SetVolfBGM(scene->GetActiveBgmInfo()->volf);
                } else {
                    sndSetSeVolf(EdEventInfo.snd_id[4], 0, fade, 0);
                }
                state->battle_bgm_vol = fade;
                return;
            case 4:
                rate = DngMainScene->GetActiveBgmInfo()->unk_c;
                rate += 0.033333335f;
                if (!(rate < 1.0f)) {
                    state->battle_bgm_state = 0;
                    rate = 1.0f;
                }
                scene = DngMainScene;
                DngMainScene->GetActiveBgmInfo()->unk_c = rate;
                scene->SetVolfBGM(scene->GetActiveBgmInfo()->volf);
                break;
        }
    }
}

void ScriptDebugCommand(int command) {
    CBattleCharaInfo *info;

    info = GetBattleCharaInfo();
    switch (command) {
        case 0:
            info->AddHp_Point(512.0f, -1.0f);
            info->ForceSet();
            break;
    }
}

void XChgMapLighting(void) {
    u8       saved[sizeof(CMapLightingInfo)];
    int      i;
    CMapInfo *map;

    map = (CMapInfo *)DngMainScene->GetMap(DngMainScene->active_map);
    if ((map != NULL) && (map != NULL) && (map->lighting_info_num >= 0x10)) {
        memset(saved, 0, sizeof(saved));
        i = 0;
        CMapLightingInfo *lighting;
        do {
            lighting = &map->lighting_info[i];
            memcpy(saved, lighting, sizeof(saved));
            lighting = &map->lighting_info[i];
            memcpy(lighting, &lighting[8], sizeof(saved));
            lighting = &map->lighting_info[i];
            memcpy(&lighting[8], saved, sizeof(saved));
            i += 1;
        } while (i < 8);
    }
}

float XChgMapRotation(int rotation) {
    if (rotation < 0 || rotation > 3) {
        return 0.0f;
    }
    return xchg_rot_list[rotation];
}

int SearchMapEventParts(int kind, CMapParts **out_parts, float *out_rot, int max) {
    char      name[0x40];
    int       result;
    CMap      *map;
    CMapParts *found;
    int       i;

    result = 0;
    if ((map = DngMainScene->GetMap(DngMainScene->active_map)) == NULL) {
        return 0;
    }
    switch (kind) {
        case 0:
            for (i = 0; i < 4; i++) {
                sprintf(name, "way%d", i + 0x20);
                found = map->GetPlaceParts(name);
                if (found != NULL) {
                    out_parts[0] = found;
                    *out_rot = XChgMapRotation(i);
                    result = 1;
                    out_parts[1] = NULL;
                    break;
                }
            }
            break;
        case 2:
            for (i = 0; i < 0x10; i++) {
                sprintf(name, "way%d", i + 0x24);
                found = map->GetPlaceParts(name);
                if (found != NULL) {
                    out_parts[0] = found;
                    *out_rot = XChgMapRotation(i);
                    result = 1;
                    out_parts[1] = NULL;
                    break;
                }
            }
            break;
        case 1:
            break;
    }
    return result;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", SearchMapFlatPosition__FPfP11CAutoMapGen);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", GetDungeonEventPoint__FPfPfi);
/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _GROUP_START(SPI_STACK *stack, int argc) {
    spiGetStackInt(stack);
    nowTbFloor->group_num = -1;
    return 1;
}

/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _GROUP(SPI_STACK *stack, int argc) {
    int first;
    int second;

    first = spiGetStackInt(stack++);
    second = spiGetStackInt(stack);
    nowTbFloor->group_num += 1;
    nowTbFloor->group[nowTbFloor->group_num].group_id = first;
    nowTbFloor->group[nowTbFloor->group_num].item_num = second;
    nowTboxGroup = nowTbFloor->group_num;
    nowTboxItemCnt = 0;
    return 1;
}

/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _ITEM(SPI_STACK *stack, int argc) {
    int i;
    int id;
    int value;
    int weight;

    for (i = 0; i < argc / 3; i++) {
        id = spiGetStackInt(stack++);
        value = spiGetStackInt(stack++);
        weight = spiGetStackInt(stack++);
        nowTbFloor->group[nowTboxGroup].item[nowTboxItemCnt].item_no = id;
        nowTbFloor->group[nowTboxGroup].item[nowTboxItemCnt].rank = value;
        nowTbFloor->group[nowTboxGroup].item[nowTboxItemCnt].num = weight;
        nowTboxItemCnt += 1;
    }
    return 1;
}

/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _FLOOR_START(SPI_STACK *stack, int argc) {
    nowTbFloor->floor_start = spiGetStackInt(stack);
    return 1;
}

/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _FLOOR(SPI_STACK *stack, int argc) {
    int count;
    int i;
    int floor;

    floor = spiGetStackInt(stack++);
    count = spiGetStackInt(stack++);
    nowTbFloor->floor[floor].group_num = count;
    for (i = 0; i < count; i++) {
        nowTbFloor->floor[floor].group_id[i] = spiGetStackInt(stack++);
    }
    return 1;
}

void CreatTresuarBoxInfo(TRESURE_BOX_FLOOR_INFO *info, char *script, int size) {
    info->group_num = 0;
    info->floor_start = 0;
    info->rank_max = 0;
    info->rank_min = 100;
    nowTbFloor = info;
    CScriptInterpreter interpreter;

    interpreter.SetTag(tag);
    interpreter.SetScript(script, size);
    interpreter.Run();
    info->group_num += 1;
}

/**
 * Finds the rank bounds of the items offered on a floor.
 */
static void PickupRandomItemCheckMax(TRESURE_BOX_FLOOR_INFO *table, int floor_index) {
    TRESURE_BOX_GROUP *group;
    int               count;
    int               i;
    int               j;
    int               id;
    table->rank_max = 0;
    table->rank_min = 0;
    count = table->floor[floor_index].group_num;
    for (i = 0; i < count; i++) {
        id = table->floor[floor_index].group_id[i];
        group = table->group;
        while (1) {
            if (group->group_id == id) {
                break;
            }
            group++;
        }
        for (j = 0; j < group->item_num; j++) {
            if (group->item[j].rank > table->rank_max) {
                table->rank_max = group->item[j].rank;
            }
            if (group->item[j].rank < table->rank_min) {
                table->rank_min = group->item[j].rank;
            }
        }
    }
}

/**
 * Chooses a random item within the requested rank bounds.
 */
static TRESURE_BOX_ITEM *PickupRandomItem(TRESURE_BOX_FLOOR_INFO *table, int floor_index, int value) {
    int               want_higher;
    TRESURE_BOX_GROUP *group;
    TRESURE_BOX_ITEM  *entry;
    int               i;
    int               id;

    want_higher = 1;
    if (value < 0) {
        want_higher = 0;
        value = -value;
    }
    if (table->rank_max < value) {
        value = table->rank_max;
    }
    if (value < table->rank_min) {
        value = table->rank_min;
    }
    while (1) {
        id = table->floor[floor_index].group_id[iRand(table->floor[floor_index].group_num)];
        group = table->group;
        i = 0;
        do {
            if (group->group_id == id) {
                goto found;
            }
            i++;
            group++;
        } while (i < table->group_num);
        printf("ERR:GROUP_ID OVER!! %d\n", i);
        while (1) {
        }
    found:
        entry = &group->item[iRand(group->item_num)];
        if (want_higher) {
            if (entry->rank >= value) {
                return entry;
            }
        } else if (entry->rank <= value) {
            return entry;
        }
    }
}

/**
 * Reports whether a position is clear of dungeon objects.
 */
static int CheckObjectPutArea(float *pos) {
    sceVu0FVECTOR position;
    CMapParts     *object;

    if (!((CTreasureBoxManager *)DngMainScene->battle_area.treasure_box)->CheckArea(pos, 40.0f)) {
        return 0;
    }
    if (!RandomCircle.CheckArea(pos, 40.0f)) {
        return 0;
    }
    if ((object = AutoMapGen.gio_parts) != NULL) {
        object->GetPosition(position);
        position[3] = 1.0f;
        if (mgDistVector(position, pos) < 40.0f) {
            return 0;
        }
    }
    return (AutoMapGen.SearchRandomStone(pos, 40.0f) != NULL) ^ 1;
}

float ScanEyePoint(float *pos) {
    sceVu0FVECTOR position;
    float         hit[4];
    CCPoly        polys[0x80];
    mgVu0FBOX     box;

    float angle;
    CMap  *map;
    int   poly_count;
    int   i;
    float z;

    angle = 0.0f;
    sceVu0CopyVector(position, pos);
    position[1] += 20.0f;
    map = DngMainScene->GetMap(DngMainScene->active_map);
    if (map == NULL) {
        return angle;
    }
    box.max[0] = 200.0f + position[0];
    box.min[0] = position[0] - 200.0f;
    box.max[1] = 200.0f + position[1];
    box.min[1] = position[1] - 200.0f;
    z = position[2];
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    box.max[2] = 200.0f + z;
    box.min[2] = z - 200.0f;
    poly_count = map->GetColPoly(polys, box, 0x80);
    sceVu0FVECTOR direction = {0.0f, 20.0f, 80.0f, 1.0f};
    sceVu0FVECTOR rotated;
    sceVu0FMATRIX rotation;
    sceVu0FMATRIX identity;
    sceVu0UnitMatrix(identity);
    i = 0;
    while (1) {
        sceVu0RotMatrixY(rotation, identity, angle);
        sceVu0ApplyMatrix(rotated, rotation, direction);
        rotated[0] += position[0];
        rotated[2] += position[2];
        if (CheckHit(polys, poly_count, position, rotated, hit, 0, 0) < 0) {
            return angle;
        }
        printf("-------------------------> hit %.2f,%.2f\n", position[0], position[2]);
        angle += 0.3926991f;
        angle = mgAngleLimit(angle);
        i++;
        if (i >= 0x10) {
            return angle;
        }
    }
}

void AutoSetTreasureBox(int item_no, float *pos, float rot_y) {
    (DngMainScene->battle_area.treasure_box)->PutTreasureBox(-1, pos, rot_y, 0x41, item_no, 1, -1, 0);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", AutoSetTreasureBox__Fv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", _FLS__FP9SPI_STACKi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", _FL__FP9SPI_STACKi);
/**
 * Updates the treasure or monster floor settings from script arguments.
 */
static int _FLE(SPI_STACK *stack, int argc) {
    FLS_FLOOR_ID = -1;
    return 1;
}

/**
 * Reads the monster placement script of a dungeon.
 */
static void CreatMonsterFloorInfo(char *script, int length) {
    FLS_FLOOR_ID = -1;
    CScriptInterpreter interpreter;

    interpreter.SetTag(tag2);
    interpreter.SetScript(script, length);
    interpreter.Run();
}

void AutoSetMonster(void) {
    float          event_point[4];
    sceVu0FVECTOR  position;
    float          direction[4];
    float          event_extra[4];
    int            i;
    int            placed;
    CActiveMonster *monster;
    int            floor_no;
    int            floor_id;
    int            spawn_count;
    int            base_id;
    int            gate_key;

    if (ActiveMonster != NULL) {
        DngMainScene->battle_area.unk_5c = 0;
        floor_no = DngSaveDataDungeon->stage_id;
        spawn_count = ActiveMonster->locate.num;
        floor_id = DngSaveDataDungeon->floor_id[DngSaveDataDungeon->stage_id];
        if (spawn_count > 0x18) {
            spawn_count = 0x18;
        }
        GetDungeonEventPoint(event_point, &event_extra[3], 2);
        event_point[3] = 1.0f;
        for (i = 0; i < spawn_count; i++) {
            placed = 0;
            do {
                if (SearchMapFlatPosition(position, &AutoMapGen) != 0) {
                    rand();
                    direction[3] = 0.0f;
                    direction[2] = 0.0f;
                    direction[1] = 0.0f;
                    direction[0] = 0.0f;
                    if (mgDistVector(event_point, position) > 520.0f &&
                        CheckObjectPutArea(position) != 0) {
                        base_id = ActiveMonster->locate.monster_id[i];
                        if (base_id >= 0xF5 && base_id < 0x10D) {
                            placed = 1;
                        } else {
                            monster = ActiveMonster->SetActiveMonster(ActiveMonster->SearchBaseIndex(base_id), position, direction, -1);
                            if (monster != NULL) {
                                placed = 1;
                                monster->locate_param = ActiveMonster->locate.param[i];
                                if (i == 0) {
                                    gate_key = GetGateKeyIndex(floor_no, floor_id);
                                    if (gate_key != -1) {
                                        monster->gate_key = gate_key;
                                    }
                                }
                            }
                        }
                    }
                }
            } while (placed == 0);
        }
    }
}

void AutoSetMonster(int monster_no, float *pos, float *rot, int param) {
    int            index;
    CActiveMonster *monster;

    if (ActiveMonster != NULL) {
        DngMainScene->battle_area.unk_5c = 0;
        index = ActiveMonster->SearchBaseIndex(monster_no);
        if (index != -1) {
            monster = ActiveMonster->SetActiveMonster(index, pos, rot, -1);
            if (monster != NULL) {
                monster->locate_param = param;
            }
        }
    }
}

void DungeonFloorInit(void) {
}

void DungeonFloorFinish(void) {
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", LoadDungeonMapFile__FPcPci);
void MinimapDoorEnable(float *pos) {
    AutoMapGen.MinimapDoorOpen(pos);
    AutoMapGen.UpdateNaviMap(pos, 4);
}

void LoadMonsterFile() {
    CScene *scene = DngMainScene;
    if (ActiveMonster != NULL) {
        ActiveMonster->Initialize(scene);
        DngMainScene->AssignStack(3);
        DngMainScene->ClearStack(3);
        mgCMemory *memory = DngMainScene->GetStack(3);
        int slot_index;
        CMonsterMan *manager = ActiveMonster;
        for (slot_index = 0; slot_index < MONSTER_ACTIVE_MAX; slot_index++) {
            u_long128 *buffer = memory->stAlloc64(4000);
            mgCMemory *slot = &manager->memory[slot_index];
            slot->stSetBuffer(buffer, 4000);
            slot->stack_used = 0;
            slot->lock = 0;
        }
        sndInitPort(5);
        int locate_index;
        int stage_id = DngSaveDataDungeon->stage_id;
        CMonsterLocateInfo *locate = &ActiveMonster->locate;
        locate->num = 0;
        locate->put_num = 0;
        locate->put_flag = 0;
        for (locate_index = 0; locate_index < MONSTER_LOCATE_MAX; locate_index++) {
            locate->param[locate_index] = -1;
            locate->monster_id[locate_index] = -1;
        }
        char path[76];
        int  size;
        sprintf(path, "dungeon/cfg_file/mos_place%d.cfg", stage_id);
        LoadFile(path, BuffReadData, &size);
        CreatMonsterFloorInfo((char *)BuffReadData, size);
        int monster_count = ActiveMonster->locate.num;
        for (int entry = 0; entry < monster_count; entry++) {
            int monster_id = ActiveMonster->locate.monster_id[entry];
            if (ActiveMonster->SearchBaseIndex(monster_id) < 0) {
                ActiveMonster->EntryRefer(monster_id, memory);
            }
        }
    }
}

void LoadMonsterFile(int monster_no, int reset) {
    mgCMemory *memory;
    CScene *scene = DngMainScene;
    if (ActiveMonster != NULL) {
        if (reset != 0) {
            ActiveMonster->Initialize(scene);
            DngMainScene->AssignStack(3);
            DngMainScene->ClearStack(3);
            memory = DngMainScene->GetStack(3);
            if (memory != NULL) {
                int i = 0;
                CMonsterMan *manager = ActiveMonster;
                for (; i < MONSTER_ACTIVE_MAX; i++) {
                    u_long128 *buffer = memory->stAlloc64(4000);
                    mgCMemory *slot = &manager->memory[i];
                    slot->stSetBuffer(buffer, 4000);
                    slot->stack_used = 0;
                    slot->lock = 0;
                }
                sndInitPort(5);

                goto entry;
            }
        } else {
            memory = scene->GetStack(3);
            if (memory != NULL) {
            entry:
                if (ActiveMonster->SearchBaseIndex(monster_no) < 0) {
                    ActiveMonster->EntryRefer(monster_no, memory);
                }
            }
        }
    }
}

// Static initialiser (.init)
INCLUDE_ASM("ps2/asm/pal/nonmatchings/dng_event", __sinit_dng_event_cpp);

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1082__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1248__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", gatekey_index__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", keydoor_key_index__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", xchg_rot_list__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", tag__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1936__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", tag2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1274__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1279__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1466__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1467__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1468__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1645__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1732__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1825__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1826__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1827__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1828__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1829__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1905__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_1965__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2159__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2198__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2199__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2200__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2446__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2447__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2448__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2449__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2450__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2451__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2452__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2453__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2454__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2455__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2456__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", at_2529__DATA);

// Static initialiser table (.ctor)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", D_0037B044__DATA);

// Virtual tables (.vtables)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", __vt__9CGeoStone__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/dng_event", __vt__13CRedMarkModel__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(counter_1489, 0x4);
INCLUDE_BSS(nowTbFloor, 0x4);
INCLUDE_BSS(nowTboxGroup, 0x4);
INCLUDE_BSS(nowTboxItemCnt, 0x4);
INCLUDE_BSS(FLS_FLOOR_ID, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(at_1348, 0x10);
INCLUDE_BSS(MainMapInfo, 0x20);
