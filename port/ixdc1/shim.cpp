#include "private.hpp"

// Deliberately data-only backing views, as scoped by the spike: charakey uses
// direct calls on the named CharaMain and nonvirtual frame methods. No engine
// constructors, vtables, rendering, or ownership cross into DC2. Never pass
// these views to other DC1 engine code.
extern "C" {
alignas(ixdc1::CCharacter) unsigned char ixdc1_chara_storage[sizeof(ixdc1::CCharacter)]
    __asm__("ixdc1_CharaMain");
}
namespace ixdc1 {
BT_ACT_STATUS BtActStatus{};
CCollisionData collision{};
CCollisionData *NowColData = &collision;
WEAPON_HAVE weapon_have{};
WEAPON_HAVE *NowWeaponHave = &weapon_have;
CUserStatus user_status{};
CUserStatus *UserStatus = &user_status;
CWeaponEffect CWeaponFx{};
CGamePad GamePad{};
s32 PadInput_OK = PAD_CROSS;
s32 driveNoInterpolate = 0;
s32 lockOnTargetFlag = 0;
alignas(CCharacter) static unsigned char weapon_storage[sizeof(CCharacter)];
CCharacter *NowWeapon = reinterpret_cast<CCharacter *>(weapon_storage);
alignas(CFrameVu1) static unsigned char body_frame_storage[sizeof(CFrameVu1)];
alignas(CFrameVu1) static unsigned char weapon_frame_storage[sizeof(CFrameVu1)];
alignas(CSHOT_EFFECT) static unsigned char effect_storage[sizeof(CSHOT_EFFECT)];
CSHOT_EFFECT *NowMainEffect = reinterpret_cast<CSHOT_EFFECT *>(effect_storage);
static MOTION_INFO motions[IXION_DC1_TOAN_MOTIONS];
static float weapon_tip[4];
static int held_buttons;
// Xiao R1: the dun/gameloop.cpp globals BattleActionPlay_Jinn reads (gameloop.cpp:1005, 1008, 1199, 1217) and the
// 12-slot CSHOT pool NowShotData points at (gameloop.cpp:1525 `NowShotData = &ShotData`).
float viewAngleH__2;
float viewAngleV__2;
s32 gameTask;
static CSHOT shot_data{};
CSHOT *NowShotData = &shot_data;

// Verbatim donor functions and element table, with exact-byte provenance.
#include "helpers.inc"

void CObject::GetRotation(float *out) { CharaMain.frame->GetRotation(out); }
// Xiao R1: `CharaMain.GetRotation()->y` (gameloop.cpp:7714), the CVector3_f_ overload, reads the same rotation.
// A data-only view (no constructor, no guarded local static), like the other backing views here.
alignas(CVector3_f_) static unsigned char rotation_view_storage[sizeof(CVector3_f_)];
CVector3_f_ *CObject::GetRotation() {
    CVector3_f_ *view = reinterpret_cast<CVector3_f_ *>(rotation_view_storage);
    float r[4];
    CharaMain.frame->GetRotation(r);
    view->x = r[0]; view->y = r[1]; view->z = r[2]; view->w = r[3];
    return view;
}
void CFrame::GetRotation(float *out) { sceVu0CopyVector(out, rotation); }
void CFrame::SetRotation(float x, float y, float z) {
    rotation[0] = x; rotation[1] = y; rotation[2] = z; rotation[3] = 1.0f;
}
// Named points come from the host's resolver; anything it does not answer is the tick's weapon tip, which is
// exactly the R1-R6 behaviour for Toan (who only reads "dcol1" on the weapon frame, charakey.cpp:119).
static IxionDc1PointResolver point_resolver;
static void *point_user;
static int point_queries, point_fallbacks, point_aliases;
static const char *point_alias_of(const char *name);
void getFramePos(CFrameVu1 *frame, char *name, float *out) {
    ++point_queries;
    const int32_t kind = reinterpret_cast<CFrame *>(frame) == CharaMain.frame ? IXION_DC1_FRAME_BODY
                                                                               : IXION_DC1_FRAME_WEAPON;
    float point[4];
    if (point_resolver != nullptr && point_resolver(point_user, kind, name, point) != 0) {
        sceVu0CopyVector(out, point);
        return;
    }
    // Ungaga R1: the converted body may carry the point under its BodyProfile name (c10b's "weapon" is "ef00").
    const char *alias = point_alias_of(name);
    if (point_resolver != nullptr && alias != nullptr && point_resolver(point_user, kind, alias, point) != 0) {
        ++point_aliases;
        sceVu0CopyVector(out, point);
        return;
    }
    ++point_fallbacks;
    sceVu0CopyVector(out, weapon_tip);
}
// Toan's "dcol1" on the body frame is the weapon frame, as in R1 (charakey.cpp:238, 285, 331, 377). Xiao R1: any other
// name is asked of the host on the frame it is searched under (BattleActionPlay_Jinn's
// NowWeapon->frame->SearchFrame("eff30"), gameloop.cpp:7708, 7760: her sling's eff30). An answered name returns a
// private frame whose GetWorldPosition gives that point; a declined one returns NULL, so DC1 fires no shot.
alignas(CFrame) static unsigned char named_frame_storage[sizeof(CFrame)];
static float named_point[4];
static int point_missing;
CFrame *CFrame::SearchFrame(char *name) {
    if (std::strcmp(name, "dcol1") == 0) return NowWeapon->frame;
    ++point_queries;
    const int32_t kind = this == CharaMain.frame ? IXION_DC1_FRAME_BODY : IXION_DC1_FRAME_WEAPON;
    float point[4];
    if (point_resolver != nullptr && point_resolver(point_user, kind, name, point) != 0) {
        sceVu0CopyVector(named_point, point);
        return reinterpret_cast<CFrame *>(named_frame_storage);
    }
    ++point_missing;
    return nullptr;
}
// Both call sites pass a zero local offset (gameloop.cpp:7707, 7759); it is added unrotated.
void CFrame::GetWorldPosition(float *world, float *local) {
    const float *base = this == reinterpret_cast<CFrame *>(named_frame_storage) ? named_point : weapon_tip;
    for (int k = 0; k < 3; k++) world[k] = base[k] + local[k];
    world[3] = 1.0f;
}
int CGamePad::On(int mask) { return held_buttons & mask; }

// Presentation and weapon-durability hooks are intentional R1 stubs.
void SndSePlay(int, int, int) {}
int SndSeSeqPlayStop(int, int, int) { return 0; }
void CWeaponEffect::Set(int, float, float) {}
void setUnitAmbientAnime(float, float, float, float, float) {}
void SwordDmgCheck1(float, int) {}
// The per-character table. Handler pairs follow the cur_chara switch at dun/gameloop.cpp:3674-3725. The info
// rows are read from each handler: Toan charakey.cpp:55-602, Ungaga 604-934, Goro 936-1157; effect names and
// radii from MyEntryEffect_* (dun/gameloop.cpp:642-648, 662-667, 722-725).
struct HandlerEntry {
    void (*on)();
    void (*play)();
    IxionDc1CharacterInfo info;
    const BT_SHOT_EFFECT *effect; // The verbatim MyEntryEffect_* row (helpers.inc) the effect runner plays.
};
// Xiao R1: the table's handlers take no arguments; Play_Jinn's (chara, aimed) come from the tick. The host's time
// stands for the CharaHand frame when aimed (gameloop.cpp:5133) and for CharaMain's otherwise (3716).
static int jinn_aimed;
static void JinnPlay() { BattleActionPlay_Jinn(&CharaMain, jinn_aimed); }
// Xiao's handler never touches NowMainEffect; her row's effect is empty and the runner stays off.
static const BT_SHOT_EFFECT no_main_effect{};
static const HandlerEntry characters[] = {
    {ToanKey_On, ToanKey_Play,
     {IXION_DC1_CHARA_TOAN, "toan", {3, 14, 15, 16, 17, 24, 25, 36, 37, 38, 39, 40}, 12, {36, 37, 38, 39, 40}, 5,
      14, 15, 0.0f, 0, 20.0f, 25.0f, 0.0f, 0, "dcol1", "", "", IXION_DC1_FRAME_BODY, "c01_fuusya", 20.0f},
     &MyEntryEffect_TOAN},
    // Goro: one swing (36), held through 95.0-95.5 into the charge loop 13 (1063-1067), released after 45 ticks
    // into the smash 14 (1125-1134). On needs a full gauge and empties it (938-944); an early release restarts
    // the swing for 15, unclamped (1106-1118). Records use owner 2 (985-1006); the shot spawns at dcol0 (1010).
    {GoroKey_On, GoroKey_Play,
     {IXION_DC1_CHARA_GORO, "goro", {13, 14, 36}, 3, {36}, 1, 13, 14, 100.0f, 1, 0.0f, 0.0f, 15.0f, 2, "dcol0", "",
      "dcol0", IXION_DC1_FRAME_WEAPON, "c06a_tameex", 15.0f},
     &MyEntryEffect_GORO},
    // Ungaga: three swings 0x25-0x27 queued at gauge 40 for 20 (630-638, 740-744, 792-797), two records per tick
    // at the stick's dcol0 and dcol1 (648-689). Held through swing 1's end-1..end it enters 13 (754-759); a
    // release there ends the action (855-863); 13's end-2..end enters the loop 14 (865-871), which fires c10a_ex
    // from the body's "weapon" point toward his facing every 30 ticks (881-909). A release goes to 15, whose
    // end-2..end ends the action and the effect (911-932). Its static wait counter (881-887) survives reset.
    {UngagaKey_On, UngagaKey_Play,
     {IXION_DC1_CHARA_UNGAGA, "ungaga", {13, 14, 15, 37, 38, 39}, 6, {37, 38, 39}, 3, 14, 15, 0.0f, 0, 40.0f,
      20.0f, 0.0f, 4, "dcol0", "dcol1", "weapon", IXION_DC1_FRAME_BODY, "c10a_ex", 8.0f, "ef00"},
     &MyEntryEffect_UNGAGA},
    // Xiao R1: BattleActionOn_Jinn needs a full gauge and empties it (gameloop.cpp:7655-7665). Play (7667-7820):
    // KEY 11 draws; a release before 11's end-2..end goes to 13, a hold to 12 until released; 13 fires one straight
    // CSHOT from the sling's eff30 at time 251-252 (5.0 at the lock-on target or along the facing; 3.5 along the
    // view angles when hand-aimed, 21.0-21.8 under PAL_TIMING) and ends at 13's end-2..end. Damage is the weapon's
    // attack, doubled under AILMENT_STAMINA. No melee records and no main effect: CSHOT::step lays owner-1 records.
    {BattleActionOn_Jinn, JinnPlay,
     {IXION_DC1_CHARA_XIAO, "xiao", {11, 12, 13}, 3, {}, 0, 12, 13, 100.0f, 1, 0.0f, 0.0f, 0.0f, 1, "eff30", "",
      "eff30", IXION_DC1_FRAME_WEAPON, "", 2.0f, "", 0, {}, 0, IXION_DC1_CSHOT_SLOTS},
     &no_main_effect},
};
static const HandlerEntry *active_entry = &characters[0];
static const HandlerEntry *find_character(int32_t id) {
    for (const HandlerEntry &c : characters)
        if (c.info.character == id) return &c;
    return nullptr;
}
static const char *point_alias_of(const char *name) {
    const IxionDc1CharacterInfo &info = active_entry->info;
    if (info.shot_point_alias[0] == '\0' || std::strcmp(name, info.shot_point) != 0) return nullptr;
    return info.shot_point_alias;
}

// The main effect's description: a per-reset copy of the character's MyEntryEffect_* row, because SetAttribute
// writes hit_flags into the shared description (shot_effect.cpp:542-546).
static BT_SHOT_EFFECT effect_desc;
static IxionDc1Motion effect_keys[4];
static int effect_key_count;
static float effect_scale = 1.0f;
static IxionDc1ShotCollider shot_collider;
static void *shot_user;

// Ungaga R1: DC1's main-effect slots, re-homed from CSHOT_EFFECT (shot_effect.cpp:126-479). The effect model is
// not drawn here; only its KEY clock is kept, as CCharacter::Step advances it from Draw (shot_effect.cpp:98):
// a SetMotion with mode 4 restarts the KEY on the next advance, mode 2 holds before the end (character.cpp PAL).
struct EffectSlot {
    int active, phase, motion, restart, hold;
    float pos[4], velocity[4], time;
    int damage, wait, wait_state, phase_delay, life_time, owner, sub_id, weapon_status, age, serial;
};
static EffectSlot slots[IXION_DC1_SHOT_SLOTS];
static int current_slot = -1, shot_serial;
static IxionDc1ShotStep shot_steps[IXION_DC1_SHOT_SLOTS];

static bool runner_on() { return effect_key_count > 0; }
static const IxionDc1Motion *effect_key(int motion) {
    return motion >= 0 && motion < effect_key_count ? &effect_keys[motion] : nullptr;
}
// CCharacter::SetMotion(no, mode) and the time write that follows it at every call site.
static void effect_set_motion(EffectSlot &s, int motion, int mode) {
    s.motion = motion;
    s.restart = (mode & 4) != 0;
    s.hold = (mode & 2) != 0;
    const IxionDc1Motion *key = effect_key(motion);
    s.time = key != nullptr ? key->start : 0.0f;
}
static void normalize_scale(float *v, float speed) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    const float k = length > 0.0f ? speed / length : 0.0f;
    v[0] *= k; v[1] *= k; v[2] *= k;
}

// The main-effect shot is recorded for the host as before; with the runner on it also takes a slot.
static IxionDc1ToanShot pending_shot;
static int pending_shot_ended;
int CSHOT_EFFECT::Set(float *position, float *vector, int owner, int sub_id, int source, CFrame *, int phase) {
    pending_shot = {};
    pending_shot.slot = -1;
    current_slot = -1;
    if (runner_on()) {
        // shot_effect.cpp:402-478, for parent == NULL. No free slot: nothing spawns.
        int slot = -1;
        for (int i = 0; i < IXION_DC1_SHOT_SLOTS; i++) {
            if (slots[i].active == 0) { slot = i; break; }
        }
        if (slot == -1) return slot;
        EffectSlot &s = slots[slot];
        s = {};
        s.phase = effect_desc.motion[0] == -1 ? 1 : 0;
        if (phase != -1) s.phase = phase;
        if (effect_desc.stationary == 0) {
            position[3] = 1.0f;
            vector[3] = 1.0f;
            for (int k = 0; k < 3; k++) s.velocity[k] = vector[k] - position[k];
            normalize_scale(s.velocity, effect_desc.speed[s.phase]);
        }
        s.velocity[3] = 0.0f;
        if (s.phase == 0) effect_set_motion(s, effect_desc.motion[0], 6);
        else effect_set_motion(s, effect_desc.motion[1], 4);
        sceVu0CopyVector(s.pos, position);
        s.pos[3] = 1.0f;
        s.damage = effect_desc.damage;
        s.phase_delay = effect_desc.life_time;
        s.active = 1;
        s.owner = owner;
        s.sub_id = sub_id;
        s.weapon_status = 1;
        s.life_time = -1;
        s.wait = s.wait_state = 0;
        s.serial = ++shot_serial;
        current_slot = slot;
        shot_steps[slot] = {};
        shot_steps[slot].spawned = 1;
        pending_shot.slot = slot;
    }
    pending_shot.spawned = 1;
    std::memcpy(pending_shot.position, position, sizeof(pending_shot.position));
    std::memcpy(pending_shot.vector, vector, sizeof(pending_shot.vector));
    pending_shot.owner = owner;
    pending_shot.sub_id = sub_id;
    pending_shot.source = source;
    pending_shot.initial_phase = phase;
    std::memcpy(pending_shot.effect, active_entry->info.effect, sizeof(pending_shot.effect));
    pending_shot.effect_radius = active_entry->info.effect_radius;
    return current_slot;
}
void CSHOT_EFFECT::SetVsMonster(char *) {}
void CSHOT_EFFECT::SetWepStatus(int flags) {
    pending_shot.weapon_flags = flags;
    if (current_slot != -1) slots[current_slot].weapon_status = flags;
}
void CSHOT_EFFECT::SetDmg(int damage) {
    pending_shot.damage = damage;
    if (current_slot != -1) slots[current_slot].damage = damage;
}
void CSHOT_EFFECT::SetWait(int wait) {
    pending_shot.wait = wait;
    if (current_slot != -1) {
        slots[current_slot].wait = static_cast<u8>(wait);
        slots[current_slot].wait_state = 0;
    }
}
void CSHOT_EFFECT::SetAttribute(int attribute) {
    pending_shot.attribute = attribute;
    if (current_slot != -1) effect_desc.hit_flags = attribute;
}
// shot_effect.cpp:294-306: every slot still starting or flying goes to its impact phase.
void CSHOT_EFFECT::EndEffect() {
    pending_shot_ended = 1;
    for (EffectSlot &s : slots) {
        if (s.active != 0 && (s.phase == 1 || s.phase == 0)) {
            s.phase = 2;
            if (effect_desc.motion[s.phase] != -1) effect_set_motion(s, effect_desc.motion[s.phase], 6);
        }
    }
}

static void retire(int slot, int why) {
    slots[slot].active = 0;
    shot_steps[slot].ended = why;
}

// CSHOT_EFFECT::Step (shot_effect.cpp:126-292) for one slot, then the KEY advance CSHOT_EFFECT::Draw makes
// (shot_effect.cpp:98). Sounds and the bomb end effect (none of the three rows has one) are not run.
static void step_slot(int slot) {
    EffectSlot &s = slots[slot];
    IxionDc1ShotStep &out = shot_steps[slot];
    ++s.age;
    const IxionDc1Motion *key = effect_key(effect_desc.motion[s.phase]);
    const float end = key != nullptr ? key->end : 0.0f;
    // loop[] stays -1: no handler calls SetLoop on the main effect.
    if (s.phase == 0 && s.time >= end - 1.0f && s.time <= end) {
        s.phase++;
        if (effect_desc.motion[s.phase] == -1) {
            retire(slot, IXION_DC1_SHOT_END_NO_MOTION);
            return;
        }
        effect_set_motion(s, effect_desc.motion[s.phase], 4);
    }
    float hit_position[4], position[4];
    sceVu0CopyVector(position, s.pos);
    sceVu0CopyVector(hit_position, s.pos);
    int result = IXION_DC1_SHOT_HIT_NONE;
    const float movement[4] = {s.velocity[0] * effect_scale, s.velocity[1] * effect_scale,
                               s.velocity[2] * effect_scale, 0.0f};
    if (s.phase < 2 && shot_collider != nullptr) {
        float contact[4];
        result = shot_collider(shot_user, position, movement, effect_desc.target, effect_desc.radius[s.phase],
                               contact);
        if (result != IXION_DC1_SHOT_HIT_NONE) sceVu0CopyVector(hit_position, contact);
    }
    out.collision = result;
    if (s.life_time > 0) s.life_time--;
    if (!(effect_desc.radius[s.phase] <= 0.0f) && s.life_time != 0) {
        if (s.wait_state <= 0) {
            // NowColData->Set(hit_position, damage, 2, radius, 1.0f, target, hit_kind, hit_flags, 0) and its
            // SetUserID/weapon_flags writes, reported instead of entering the swing records.
            out.record = 1;
            sceVu0CopyVector(out.record_position, hit_position);
            out.record_radius = effect_desc.radius[s.phase];
            out.record_life = 2;
            out.record_target = effect_desc.target;
            out.record_kind = effect_desc.hit_kind;
            out.record_flags = effect_desc.hit_flags;
            out.record_weapon_flags = s.weapon_status;
            if (effect_desc.hit_kind == 3) {
                s.velocity[3] = 1.0f;
                if (effect_desc.speed[s.phase] <= 0.0f) {
                    float direction[4] = {CharaMain.pos[0] - position[0], 0.0f, CharaMain.pos[2] - position[2], 0.0f};
                    normalize_scale(direction, 1.0f);
                    sceVu0CopyVector(out.record_throw, direction);
                } else {
                    sceVu0CopyVector(out.record_throw, s.velocity);
                }
            }
            s.wait_state = s.wait;
        } else {
            s.wait_state--;
        }
    }
    if (result == IXION_DC1_SHOT_HIT_NONE) {
        for (int k = 0; k < 3; k++) s.pos[k] = position[k] + movement[k];
        // A flying effect that runs out of time skips to its last phase.
        if (s.phase == 1 && s.phase_delay != -1) {
            s.phase_delay--;
            if (s.phase_delay == -1) {
                s.phase += 2;
                if (effect_desc.motion[s.phase] == -1) {
                    retire(slot, IXION_DC1_SHOT_END_NO_MOTION);
                    return;
                }
                effect_set_motion(s, effect_desc.motion[s.phase], 6);
                normalize_scale(s.velocity, effect_desc.speed[s.phase]);
            }
        }
    } else if (s.phase == 1) {
        // A flying effect that hits something moves on to its impact phase.
        s.phase++;
        if (effect_desc.motion[s.phase] == -1) {
            retire(slot, IXION_DC1_SHOT_END_NO_MOTION);
            return;
        }
        effect_set_motion(s, effect_desc.motion[s.phase], 6);
        normalize_scale(s.velocity, effect_desc.speed[s.phase]);
    }
    // The effect ends when the KEY of its impact or last phase finishes (end is the one read at the top).
    if ((s.phase == 2 || s.phase == 3) && s.time >= end - 1.0f && s.time < end) {
        retire(slot, IXION_DC1_SHOT_END_MOTION);
        return;
    }
    // Draw's model->Step: restart (mode 4), then advance by the KEY step; mode 2 holds before the end (PAL).
    const IxionDc1Motion *now = effect_key(s.motion);
    if (now != nullptr) {
        if (s.restart) {
            s.time = now->start;
            s.restart = 0;
        }
        float speed = now->step;
        if (s.hold && !(s.time + speed < now->end)) speed = 0.0f;
        s.time += speed;
        if (!s.hold && s.time > now->end) s.time = now->start;
    }
}

// motionDrive's NowMainEffect->Step() (dun/gameloop.cpp:6308), once per tick after the handler.
// shot_steps was cleared at the start of the tick; Set marked the spawns and EndEffect needs no mark.
static void step_effects() {
    for (int slot = 0; slot < IXION_DC1_SHOT_SLOTS; slot++) {
        const bool was_active = slots[slot].active != 0;
        if (was_active) step_slot(slot);
        const EffectSlot &s = slots[slot];
        IxionDc1ShotStep &out = shot_steps[slot];
        if (!was_active && out.ended == 0) continue;
        out.active = s.active;
        out.serial = s.serial;
        out.phase = s.phase;
        out.age = s.age;
        sceVu0CopyVector(out.position, const_cast<float *>(s.pos));
        sceVu0CopyVector(out.velocity, const_cast<float *>(s.velocity));
        out.effect_time = s.time;
        out.phase_delay = s.phase_delay;
        out.damage = s.damage;
        out.owner = s.owner;
        out.sub_id = s.sub_id;
    }
}

// Xiao R1: the CSHOT pool. BattleActionPlay_Jinn fills NowShotData's slots itself (gameloop.cpp:7727-7741) and the
// verbatim CSHOT::step (helpers.inc, shot_effect.cpp:54-87) moves them, asks checkCollision and lays the records.
// This wrapper only scales a new slot's vector into host units once, tracks serial/age, and reports each slot.
static float shot_scale = 1.0f;
static s32 cshot_used_before[IXION_DC1_CSHOT_SLOTS];
static int cshot_serials[IXION_DC1_CSHOT_SLOTS], cshot_ages[IXION_DC1_CSHOT_SLOTS], cshot_serial;
static int cshot_collision[IXION_DC1_CSHOT_SLOTS];
static IxionDc1ShotStep cshot_steps[IXION_DC1_CSHOT_SLOTS];

// DC1's checkCollision (shot_utils.cpp:24-91), answered by the host's collider in host space. With no collider, or
// none found, hit_position is the position (shot_utils.cpp:89). CSHOT::step passes its slot's pos[] and vector[].
SHOT_COLLISION_RESULT checkCollision(float *hit_position, float *position, float *movement, s32 target_mode,
                                     float radius) {
    sceVu0CopyVector(hit_position, position);
    int32_t result = IXION_DC1_SHOT_HIT_NONE;
    if (shot_collider != nullptr) {
        float contact[4];
        result = shot_collider(shot_user, position, movement, target_mode, radius, contact);
        if (result != IXION_DC1_SHOT_HIT_NONE) sceVu0CopyVector(hit_position, contact);
    }
    const long slot = static_cast<long>((position - NowShotData->pos[0]) / 4);
    if (slot >= 0 && slot < IXION_DC1_CSHOT_SLOTS) cshot_collision[slot] = result;
    return static_cast<SHOT_COLLISION_RESULT>(result);
}

// motionDrive's NowShotData->step() (dun/gameloop.cpp:6309), once per tick after the main effect.
static void step_cshots() {
    CSHOT &shots = *NowShotData;
    float before[IXION_DC1_CSHOT_SLOTS][4];
    s32 was_used[IXION_DC1_CSHOT_SLOTS];
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        if (shots.used[i] != 0 && cshot_used_before[i] == 0) {
            // A slot the handler filled this tick: its vector is DC1 units per step (5.0 or 3.5).
            for (int k = 0; k < 3; k++) shots.vector[i][k] *= shot_scale;
            cshot_serials[i] = ++cshot_serial;
            cshot_ages[i] = 0;
            cshot_steps[i].spawned = 1;
        }
        was_used[i] = shots.used[i];
        sceVu0CopyVector(before[i], shots.pos[i]);
        cshot_collision[i] = IXION_DC1_SHOT_HIT_NONE;
    }
    int had_record[96];
    for (int k = 0; k < 96; k++) had_record[k] = collision.active[k];
    NowShotData->step();
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        if (was_used[i] == 0) continue;
        IxionDc1ShotStep &out = cshot_steps[i];
        out.active = shots.used[i] != 0;
        out.serial = cshot_serials[i];
        out.age = ++cshot_ages[i];
        out.collision = cshot_collision[i];
        sceVu0CopyVector(out.position, shots.pos[i]);
        sceVu0CopyVector(out.velocity, shots.vector[i]);
        out.damage = shots.damage[i];
        out.owner = 1; // SetUserID(1, 0) on every CSHOT record (shot_effect.cpp:71)
        out.life = shots.life[i];
        if (!out.active) out.ended = out.collision != IXION_DC1_SHOT_HIT_NONE ? IXION_DC1_SHOT_END_HIT
                                                                              : IXION_DC1_SHOT_END_LIFE;
    }
    // The record each hit laid sits at the slot's position before the step (Set(pos[shot], ...), :70).
    for (int k = 0; k < 96; k++) {
        if (collision.active[k] == 0 || had_record[k] != 0) continue;
        const auto &hit = collision.hit[k];
        for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
            IxionDc1ShotStep &out = cshot_steps[i];
            if (was_used[i] == 0 || out.collision == IXION_DC1_SHOT_HIT_NONE || out.record != 0 ||
                std::memcmp(before[i], hit.pos, 3 * sizeof(float)) != 0) continue;
            out.record = 1;
            sceVu0CopyVector(out.record_position, const_cast<float *>(hit.pos));
            out.record_radius = hit.radius;
            out.record_life = hit.life;
            out.record_target = hit.target_mask;
            out.record_kind = hit.kind;
            out.record_flags = hit.flags;
            out.record_weapon_flags = hit.weapon_flags;
            break;
        }
    }
}

// Re-homed donor upkeep, gameloop.cpp:6044-6070,6089-6175; party/HUD/camera bookkeeping is omitted.
// Ungaga R1: the swinging damp is Toan's and Ungaga's only (gameloop.cpp:6113-6135). Goro never had it, so his gauge
// refills at the full weapon rate (R1/R2 damped him by 0.25 too).
static void upkeep() {
    for (int i = 0; i < 96; ++i) {
        if (collision.active[i] == 0) continue;
        if (collision.hit[i].phase > 0) --collision.hit[i].phase;
        else collision.hit[i].phase = collision.hit[i].ready_phase;
        --collision.hit[i].life;
        if (collision.hit[i].life <= 0) {
            collision.active[i] = 0;
            collision.hit[i].scale_rate = 0.0f;
            continue;
        }
        collision.hit[i].radius += collision.hit[i].unk_40;
        if (collision.hit[i].scale_rate != 0.0f)
            sceVu0ScaleVectorXYZ(collision.hit[i].base_point, collision.hit[i].base_point,
                                 collision.hit[i].scale_rate);
    }
    if (BtActStatus.special_cooldown > 0) --BtActStatus.special_cooldown;
    if (static_cast<int>(BtActStatus.action_gauge) < 100) {
        if (BtActStatus.refill_frames < 10) ++BtActStatus.refill_frames;
        float gain = static_cast<float>(NowWeaponHave->speed) / 30.0f;
        if (StatusErrCheck(AILMENT_GOO)) gain /= 2.0f;
        if (StatusErrCheck(AILMENT_STAMINA)) gain *= 2.0f;
        if (gain < 1.0f) gain = 1.0f;
        if (UserStatus->cur_chara == CHARA_TOAN || UserStatus->cur_chara == CHARA_UNGAGA) {
            gain = BtActStatus.swinging == 0 ? gain * 0.25f : 0.0f;
            BtActStatus.swinging = 0;
        }
        BtActStatus.action_gauge += gain;
        if (!(static_cast<int>(BtActStatus.action_gauge) < 100)) {
            BtActStatus.action_gauge = 100.0f;
            BtActStatus.gauge_flash = 10;
            BtActStatus.gauge_exhausted = 0;
        }
    } else if (BtActStatus.gauge_flash > 0) --BtActStatus.gauge_flash;
    if (++BtActStatus.frames_since_attack >= 3600) BtActStatus.frames_since_attack = 3600;
}
}

extern "C" void ixion_dc1_set_point_resolver(IxionDc1PointResolver resolver, void *user) {
    ixdc1::point_resolver = resolver;
    ixdc1::point_user = user;
}

extern "C" int32_t ixion_dc1_character_info(int32_t character, IxionDc1CharacterInfo *out) {
    const ixdc1::HandlerEntry *c = ixdc1::find_character(character);
    if (c == nullptr) return 0;
    *out = c->info;
    out->effect_stationary = c->effect->stationary;
    for (int k = 0; k < 4; k++) out->effect_speed[k] = c->effect->speed[k];
    out->effect_life = c->effect->life_time;
    return 1;
}

extern "C" void ixion_dc1_set_shot_collider(IxionDc1ShotCollider collider, void *user) {
    ixdc1::shot_collider = collider;
    ixdc1::shot_user = user;
}

extern "C" void ixion_dc1_off_effect(void) {
    using namespace ixdc1;
    for (EffectSlot &s : slots) s.active = 0;
    current_slot = -1;
    // Xiao R1: the CSHOT pool goes too (DC1 has no host-side end for it either).
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) NowShotData->used[i] = 0;
}

extern "C" int32_t ixion_dc1_reset(const IxionDc1Config *config) {
    using namespace ixdc1;
    active_entry = find_character(config->character);
    const int32_t known = active_entry != nullptr;
    if (!known) active_entry = &characters[0];
    BtActStatus = {};
    collision = {};
    weapon_have = {};
    user_status = {};
    std::memset(ixdc1_chara_storage, 0, sizeof(ixdc1_chara_storage));
    std::memset(weapon_storage, 0, sizeof(weapon_storage));
    std::memset(body_frame_storage, 0, sizeof(body_frame_storage));
    std::memset(weapon_frame_storage, 0, sizeof(weapon_frame_storage));
    CharaMain.frame = reinterpret_cast<CFrame *>(body_frame_storage);
    NowWeapon->frame = reinterpret_cast<CFrame *>(weapon_frame_storage);
    CharaMain.motion_type.motion_info = motions;
    for (int i = 0; i < IXION_DC1_TOAN_MOTIONS; ++i) {
        motions[i] = {static_cast<int>(config->motions[i].start),
                      static_cast<int>(config->motions[i].end), config->motions[i].step, 0};
    }
    NowWeaponHave->attack = config->attack;
    NowWeaponHave->speed = config->weapon_speed;
    NowWeaponHave->best_elem = config->element;
    NowWeaponHave->flags = config->weapon_flags;
    // StatusErrCheck reads ailments[cur_chara] (shot_freefuncs.cpp:963); each skill flag is the character's own.
    const int chara = active_entry->info.character;
    UserStatus->cur_chara = chara;
    UserStatus->ailments[chara] = config->ailments;
    UserStatus->skill_owned[chara] = config->charged_skill;
    BtActStatus.action_gauge = config->action_gauge;
    BtActStatus.can_act = 1;
    held_buttons = 0;
    driveNoInterpolate = 0;
    pending_shot = {};
    pending_shot_ended = 0;
    // Ungaga R1: the effect runner starts empty with a fresh copy of the character's MyEntryEffect_* row.
    effect_desc = *active_entry->effect;
    effect_key_count = config->effect_key_count < 0 ? 0 : config->effect_key_count > 4 ? 4 : config->effect_key_count;
    for (int k = 0; k < 4; k++) effect_keys[k] = k < effect_key_count ? config->effect_keys[k] : IxionDc1Motion{};
    effect_scale = config->effect_scale > 0.0f ? config->effect_scale : 1.0f;
    for (EffectSlot &s : slots) s = {};
    for (IxionDc1ShotStep &s : shot_steps) s = {};
    current_slot = -1;
    shot_serial = 0;
    // Xiao R1: an empty CSHOT pool at the host's scale.
    shot_data = {};
    shot_scale = config->shot_scale > 0.0f ? config->shot_scale : 1.0f;
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        cshot_used_before[i] = 0;
        cshot_serials[i] = cshot_ages[i] = 0;
        cshot_steps[i] = {};
    }
    cshot_serial = 0;
    jinn_aimed = 0;
    viewAngleH__2 = viewAngleV__2 = 0.0f;
    gameTask = 0;
    return known;
}

extern "C" void ixion_dc1_toan_reset(const IxionDc1ToanConfig *config) {
    IxionDc1Config toan = *config;
    toan.character = IXION_DC1_CHARA_TOAN;
    ixion_dc1_reset(&toan);
}

extern "C" void ixion_dc1_toan_suspend(void) {
    using namespace ixdc1;
    // DC1's R2 entry clears the action/motion and movement (gameloop.cpp:4354-4371).
    // Gauge, cooldown and their upkeep counters belong to the loaded character.
    BtActStatus.action_on = BtActStatus.action_no = BtActStatus.action_step = 0;
    BtActStatus.motion_no = 0;
    BtActStatus.combo_window = BtActStatus.combo_queued = 0;
    BtActStatus.charge_time = 0.0f;
    BtActStatus.charge_level = 0;
    BtActStatus.move_power = BtActStatus.move_power_decay = 0.0f;
    BtActStatus.move_x = BtActStatus.move_z = BtActStatus.stick_strength = 0.0f;
    BtActStatus.jump_velocity_x = BtActStatus.jump_velocity_y = BtActStatus.jump_velocity_z = 0.0f;
    BtActStatus.jumping = BtActStatus.action_busy = BtActStatus.invincible_frames = 0;
    BtActStatus.can_act = 1;
    collision = {};
    pending_shot = {};
    held_buttons = 0;
    driveNoInterpolate = 0;
}

extern "C" void ixion_dc1_toan_tick(const IxionDc1ToanInput *input, IxionDc1ToanOutput *output) {
    ixion_dc1_tick(input, output);
}

extern "C" void ixion_dc1_tick(const IxionDc1Input *input, IxionDc1Output *output) {
    using namespace ixdc1;
    *output = {};
    driveNoInterpolate = 0;
    pending_shot = {};
    pending_shot_ended = 0;
    point_queries = point_fallbacks = point_aliases = 0;
    point_missing = 0;
    for (IxionDc1ShotStep &s : shot_steps) s = {};
    current_slot = -1;
    // Xiao R1: lock-on target, view angles and the aimed flag Play_Jinn reads; the pool's occupancy before the handler.
    sceVu0CopyVector(BtActStatus.target_position, const_cast<float *>(input->target_position));
    viewAngleH__2 = input->view_yaw;
    viewAngleV__2 = input->view_pitch;
    jinn_aimed = input->aimed != 0;
    gameTask = jinn_aimed ? GAME_TASK_EYE_CAMERA : 0;
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        cshot_used_before[i] = NowShotData->used[i];
        cshot_steps[i] = {};
    }
    // Camera shake is an event here: report what the handler writes this tick (no DC1 camera runs).
    BtActStatus.camera_shake = BtActStatus.camera_shake_decay = 0.0f;
    BtActStatus.camera_shake_frames = 0;
    held_buttons = input->cross_held ? PadInput_OK : 0;
    lockOnTargetFlag = input->lock_on;
    CharaMain.motion_type.state.time = input->time;
    sceVu0CopyVector(CharaMain.pos, const_cast<float *>(input->position));
    sceVu0CopyVector(weapon_tip, const_cast<float *>(input->weapon_tip));
    CharaMain.frame->SetRotation(0.0f, input->yaw, 0.0f);
    BtActStatus.move_x = input->move_x;
    BtActStatus.move_z = input->move_z;
    BtActStatus.stick_strength = std::sqrt(input->move_x * input->move_x + input->move_z * input->move_z);
    BtActStatus.ground_height = input->ground_height;
    if (StatusErrCheck(AILMENT_FREEZE)) BtActStatus.stick_strength = 0.0f;
    if (input->cancel) {
        BtActStatus.action_on = 0;
        BtActStatus.motion_no = 0;
        BtActStatus.combo_window = BtActStatus.combo_queued = 0;
        BtActStatus.move_power = BtActStatus.move_power_decay = 0.0f;
        BtActStatus.jumping = BtActStatus.action_busy = BtActStatus.invincible_frames = 0;
        BtActStatus.can_act = 1;
        collision = {};
    } else {
        if (input->cross_down) active_entry->on();
        if (BtActStatus.action_on == 1) active_entry->play();
    }
    // Ungaga R1: the main effect steps after the handler (motionDrive, dun/gameloop.cpp:6308), also on a cancel.
    if (runner_on()) step_effects();
    // Xiao R1: then the CSHOT pool (gameloop.cpp:6309), every tick like DC1's motionDrive; a no-op when it is empty.
    step_cshots();
    // Export fresh raw records before their one-frame lifetime is aged away.
    for (int i = 0; i < 96; ++i) {
        if (!collision.active[i]) continue;
        const auto &hit = collision.hit[i];
        auto &out = output->hits[output->hit_count++];
        std::memcpy(out.position, hit.pos, sizeof(out.position));
        std::memcpy(out.knockback_origin, hit.knockback_origin, sizeof(out.knockback_origin));
        out.radius = hit.radius; out.damage = hit.damage; out.life = hit.life;
        out.target_mask = hit.target_mask; out.kind = hit.kind; out.element_flags = hit.flags;
        out.owner = hit.owner; out.attack_no = hit.attack_no; out.weapon_flags = hit.weapon_flags;
        out.knockback_speed = hit.knockback_speed; out.knockback_decay = hit.knockback_decay;
        out.knockback_mode = hit.knockback_mode;
        out.slot = i;
    }
    output->shot = pending_shot;
    output->shot_ended = pending_shot_ended;
    output->camera_shake = BtActStatus.camera_shake;
    output->camera_shake_decay = BtActStatus.camera_shake_decay;
    output->camera_shake_frames = BtActStatus.camera_shake_frames;
    output->point_queries = point_queries;
    output->point_fallbacks = point_fallbacks;
    output->character = active_entry->info.character;
    output->point_aliases = point_aliases;
    for (int slot = 0; slot < IXION_DC1_SHOT_SLOTS; slot++) {
        output->shots[slot] = shot_steps[slot];
        output->shots_live += slots[slot].active != 0;
    }
    output->point_missing = point_missing;
    for (int i = 0; i < IXION_DC1_CSHOT_SLOTS; i++) {
        output->cshots[i] = cshot_steps[i];
        output->cshots_live += NowShotData->used[i] != 0;
    }
    // DunMoveChara push/jump, port/src/dun/movechara.cpp:1245-1262.
    if (BtActStatus.jumping == 1) {
        output->velocity[0] += BtActStatus.jump_velocity_x;
        output->velocity[1] += BtActStatus.jump_velocity_y;
        output->velocity[2] += BtActStatus.jump_velocity_z;
        BtActStatus.jump_velocity_y -= 0.1f;
    }
    if (BtActStatus.move_power > 0.0f) {
        output->velocity[0] += BtActStatus.move_vector[0] * BtActStatus.move_power;
        output->velocity[2] += BtActStatus.move_vector[2] * BtActStatus.move_power;
        BtActStatus.move_power -= BtActStatus.move_power_decay;
        if (BtActStatus.move_power <= 0.0f) BtActStatus.move_power = 0.0f;
    }
    output->action_on = BtActStatus.action_on; output->action_no = BtActStatus.action_no;
    output->motion_no = BtActStatus.motion_no; output->restart = driveNoInterpolate;
    output->time = CharaMain.motion_type.state.time; output->yaw = CharaMain.frame->rotation[1];
    output->combo_window = BtActStatus.combo_window; output->combo_queued = BtActStatus.combo_queued;
    output->charge_time = BtActStatus.charge_time; output->charge_level = BtActStatus.charge_level;
    // Gauge/cooldown report the handler's immediate outputs. Upkeep belongs to
    // the end of this tick, so the next press sees retail's replenished gauge.
    output->action_gauge = BtActStatus.action_gauge;
    output->cooldown = BtActStatus.special_cooldown;
    output->jumping = BtActStatus.jumping;
    output->invincible = BtActStatus.invincible_frames > 0;
    upkeep();
}
