#ifndef IXION_DC1_TOAN_H
#define IXION_DC1_TOAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* One DC1 combat actor, private to the DC2 module. DC2 owns the body and clock.
 * No game headers or host-sized long/pointer fields cross this boundary in the records below.
 * The actor runs one of DC1's charakey.cpp handler pairs, chosen at reset by `character`:
 * Toan, Goro or Ungaga (dun/gameloop.cpp:3674-3725 dispatches the same pairs on UserStatus->cur_chara).
 * The ixion_dc1_toan_* entry points are the R1 Toan boundary and always run Toan. */
enum { IXION_DC1_TOAN_MOTIONS = 41, IXION_DC1_TOAN_HITS = 96 };
enum { IXION_DC1_MOTIONS = IXION_DC1_TOAN_MOTIONS, IXION_DC1_HITS = IXION_DC1_TOAN_HITS };
/* DC1 character ids (itemdata.hpp:486-491, CHARA_*). Xiao R1: Xiao (1) runs BattleActionOn_Jinn/BattleActionPlay_Jinn
 * (dun/gameloop.cpp:7655-7820, not charakey.cpp). Ruby and Osmond use other handlers (gameloop.cpp). */
enum { IXION_DC1_CHARA_TOAN = 0, IXION_DC1_CHARA_XIAO = 1, IXION_DC1_CHARA_GORO = 2, IXION_DC1_CHARA_UNGAGA = 4 };
/* The frame a named point is read from: getFramePos((CFrameVu1 *) NowWeapon->frame, ...) or CharaMain.frame. */
enum { IXION_DC1_FRAME_BODY = 0, IXION_DC1_FRAME_WEAPON = 1 };
typedef struct IxionDc1Motion {
    float start, end, step;
} IxionDc1Motion;
typedef struct IxionDc1ToanConfig {
    IxionDc1Motion motions[IXION_DC1_TOAN_MOTIONS]; /* The character's own KEY order (c01d, c06b, c10b). */
    int32_t attack, weapon_speed, element, weapon_flags;
    uint32_t ailments;
    int32_t charged_skill;
    float action_gauge;
    int32_t character; /* IXION_DC1_CHARA_*; zero is Toan. Ignored by ixion_dc1_toan_reset. */
    /* Ungaga R1: the main-effect runner (DC1's CSHOT_EFFECT, shot_effect.cpp:126-479). effect_keys are the KEY rows
     * of the character's effect pack (MyEntryEffect_*.model_name's cfg, e.g. c10a_ex "KEY 5, 25, 0.6", "30, 50,
     * 0.8", "55, 70, 0.6"), indexed by the effect's motion[phase]; effect_scale is host units per DC1 unit for
     * the flight. With effect_key_count == 0 the runner is off and shots are only reported through output.shot. */
    IxionDc1Motion effect_keys[4];
    int32_t effect_key_count;
    float effect_scale; /* 0 means 1. */
    /* Xiao R1: host units per DC1 unit for the 12-slot CSHOT pool (NowShotData). A new shot's vector is scaled once
     * when the handler fills its slot, so CSHOT::step's pos += vector moves it in host space. 0 means 1. */
    float shot_scale;
} IxionDc1ToanConfig;
typedef struct IxionDc1ToanInput {
    float time; /* Current absolute animation frame supplied by the host. */
    float position[4], weapon_tip[4]; /* weapon_tip answers every named point the resolver does not. */
    float yaw, move_x, move_z, ground_height;
    int32_t cross_down, cross_held, lock_on, cancel;
    /* Xiao R1. BtActStatus.target_position: the locked-on monster's point (read only with lock_on). */
    float target_position[4];
    /* viewAngleH__2/viewAngleV__2, the first-person view angles a hand-aimed shot flies along (gameloop.cpp:7764). */
    float view_yaw, view_pitch;
    /* Nonzero runs BattleActionPlay_Jinn(&CharaHand, 1) (gameloop.cpp:5133): time is then the hand rig's frame. */
    int32_t aimed;
} IxionDc1ToanInput;
typedef struct IxionDc1ToanHit {
    float position[4], radius;
    int32_t damage, life, target_mask, kind, element_flags, owner, attack_no;
    int32_t weapon_flags;
    float knockback_origin[4], knockback_speed, knockback_decay;
    int32_t knockback_mode;
    int32_t slot; /* CCollisionData record index: one per simultaneous hit (two for DC1's two-handed swings). */
} IxionDc1ToanHit;
/* DC1's main-effect shot: NowMainEffect->Set(position, vector, owner, sub_id, source, NULL, initial_phase) and
 * its Set* calls (Toan charakey.cpp:462-475, Goro 1009-1020, Ungaga 897-907). The effect itself
 * (CSHOT_EFFECT::Step) is the host's: DC1 never runs it inside the handler. The effect is the character's
 * MyEntryEffect_* entry (dun/gameloop.cpp:642-725): its name and radius are copied here. */
typedef struct IxionDc1ToanShot {
    int32_t spawned;
    float position[4];
    int32_t damage, wait, attribute, weapon_flags;
    float vector[4]; /* Set's second argument: Toan and Goro pass a near-vertical constant, Ungaga a target point. */
    int32_t owner, sub_id, source, initial_phase;
    char effect[16]; /* MyEntryEffect_*.name, e.g. "c01_fuusya", "c06a_tameex"; NUL-terminated. */
    float effect_radius; /* MyEntryEffect_*'s first radius, DC1 units (Toan 20, Goro 15, Ungaga 8). */
    int32_t slot; /* Ungaga R1: the effect-runner slot Set took (-1 with the runner off). */
} IxionDc1ToanShot;

/* Ungaga R1: one effect-runner slot after this tick's CSHOT_EFFECT::Step (shot_effect.cpp:126-292). The main
 * effect has 6 slots (MainChara_Effect's Entry2(..., 6), dun/gameloop.cpp:6857). */
enum { IXION_DC1_SHOT_SLOTS = 6 };
/* checkCollision's result (shot_utils.hpp SHOT_COLLISION_*), the host's collider answer. */
enum { IXION_DC1_SHOT_HIT_NONE = 0, IXION_DC1_SHOT_HIT_MAP = 1, IXION_DC1_SHOT_HIT_PLAYER = 2,
       IXION_DC1_SHOT_HIT_MONSTER = 3 };
/* Why a slot retired this tick. */
enum { IXION_DC1_SHOT_END_NONE = 0, IXION_DC1_SHOT_END_MOTION = 1, /* its last phase's KEY finished */
       IXION_DC1_SHOT_END_NO_MOTION = 2, /* the next phase has no KEY (motion -1) */
       IXION_DC1_SHOT_END_OFF = 3, /* ixion_dc1_off_effect */
       /* Xiao R1, CSHOT pool (CSHOT::step, shot_effect.cpp:54-87): */
       IXION_DC1_SHOT_END_LIFE = 4, /* life (0x78 = 120 steps) ran out */
       IXION_DC1_SHOT_END_HIT = 5 /* first contact: a record was laid and the slot freed */ };
/* Xiao R1: the CSHOT pool's 12 slots (shot_effect.hpp:235, BattleActionPlay_Jinn's `for (i = 0; i < 12; i++)`). */
enum { IXION_DC1_CSHOT_SLOTS = 12 };
typedef struct IxionDc1ShotStep {
    int32_t active;   /* Live after this tick. */
    int32_t spawned;  /* Set this tick. */
    int32_t ended;    /* IXION_DC1_SHOT_END_* when it retired this tick, else 0. */
    int32_t serial;   /* 1, 2, ... per Set since reset: tells a reused slot apart. */
    int32_t phase;    /* 0 start, 1 flight, 2 impact, 3 out of time. */
    int32_t age;      /* Steps run, counting this tick's. */
    int32_t collision; /* IXION_DC1_SHOT_HIT_* this step (asked only in phases 0 and 1). */
    float position[4]; /* After this step, host space. */
    float velocity[4]; /* CSHOT_EFFECT::velocity, DC1 units per step; position moves by velocity * effect_scale. */
    float effect_time; /* The effect model's KEY time after this tick. */
    int32_t phase_delay; /* Flight steps left before phase 3 (life_time from MyEntryEffect_*). */
    int32_t damage, owner, sub_id;
    /* The CCollisionData record this step laid (NowColData->Set, shot_effect.cpp:179-207), else record == 0. */
    int32_t record;
    float record_position[4], record_radius; /* radius in DC1 units */
    int32_t record_life, record_target, record_kind, record_flags, record_weapon_flags;
    float record_throw[4]; /* kind 3: SetVelocity's direction (the flight, or toward the player when not moving). */
    /* Xiao R1: CSHOT life left after this step (CSHOT::life[]); 0 for the effect runner's slots. */
    int32_t life;
} IxionDc1ShotStep;
typedef struct IxionDc1ToanOutput {
    int32_t action_on, action_no, motion_no, restart, combo_window, combo_queued;
    float time, yaw, action_gauge, charge_time;
    int32_t charge_level, cooldown;
    int32_t jumping; /* BtActStatus.jumping after the handler: velocity[1] is the whole vertical step. */
    int32_t invincible; /* BtActStatus.invincible_frames > 0: the jump attack and charged shot ignore hits. */
    float velocity[4]; /* Donor units per tick; the DC2 bridge applies room scale. */
    IxionDc1ToanShot shot; /* Spawned this tick by the handler, else zero. */
    int32_t hit_count;
    IxionDc1ToanHit hits[IXION_DC1_TOAN_HITS]; /* Raw per-frame records, not coalesced windows. */
    /* Additions after R5; zero for Toan's normal swings. */
    int32_t shot_ended; /* NowMainEffect->EndEffect() this tick (Ungaga's release end). */
    float camera_shake, camera_shake_decay; /* BtActStatus.camera_shake* written this tick (Goro 1017-1025). */
    int32_t camera_shake_frames;
    int32_t point_queries, point_fallbacks; /* getFramePos calls this tick; how many fell back to weapon_tip. */
    int32_t character; /* The character that ran. */
    /* Ungaga R1. */
    int32_t point_aliases; /* Named points answered through the row's shot_point_alias (weapon -> ef00). */
    int32_t shots_live;    /* Effect-runner slots live after this tick. */
    IxionDc1ShotStep shots[IXION_DC1_SHOT_SLOTS]; /* Indexed by slot; all zero with the runner off. */
    /* Xiao R1: the CSHOT pool after this tick's CSHOT::step (run verbatim after the handler and the effect runner,
     * as motionDrive steps NowShotData after NowMainEffect, dun/gameloop.cpp:6308-6309). Each hit's record (life 1,
     * radius 3, target 2, kind 2, owner 1 from SetUserID(1, 0), flags from best_elem) is also in hits[]. phase,
     * effect_time and phase_delay are 0; velocity is CSHOT::vector (host units per step after shot_scale). */
    int32_t cshots_live;
    int32_t point_missing; /* SearchFrame names the resolver declined this tick (no eff30: no shot). */
    IxionDc1ShotStep cshots[IXION_DC1_CSHOT_SLOTS];
} IxionDc1ToanOutput;
typedef IxionDc1ToanConfig IxionDc1Config;
typedef IxionDc1ToanInput IxionDc1Input;
typedef IxionDc1ToanOutput IxionDc1Output;
typedef IxionDc1ToanHit IxionDc1Hit;
typedef IxionDc1ToanShot IxionDc1Shot;

/* Per-character table, read from the handlers in charakey.cpp (cited in shim.cpp). Read-only data for the host:
 * which KEYs the handler drives, which one loops while charging, the gauge rules and the named points. */
enum { IXION_DC1_INFO_KEYS = 16 };
typedef struct IxionDc1CharacterInfo {
    int32_t character;
    char name[8];
    int32_t donor_keys[IXION_DC1_INFO_KEYS]; /* KEYs the handler selects (motion_no), ascending. */
    int32_t donor_key_count;
    int32_t combo_keys[5]; /* Swing KEYs in combo order. */
    int32_t combo_key_count;
    int32_t charge_loop_key; /* The host loops this KEY instead of holding at its end. */
    int32_t release_key; /* The charged release the loop leads to (Toan 0xF jump, Goro 14 smash, Ungaga 15). */
    float on_gauge_min; /* Gauge the first press needs (Goro 100, else 0). */
    int32_t on_zeroes_gauge; /* The first press empties the gauge (Goro). */
    float combo_gauge_min, combo_gauge_cost; /* Follow-up queue threshold and debit (clamped at 0). */
    float restart_gauge_cost; /* Goro's early charge release restarts the swing for 15, unclamped. */
    int32_t hit_owner; /* CCollisionData owner in this character's records (basic_damage's second argument). */
    char hit_point[8]; /* Weapon frame point the swing records sit on. */
    char hit_point_2[8]; /* Second simultaneous point (Ungaga's two fists), else empty. */
    char shot_point[8]; /* Point the shot spawns at: "" = CharaMain.pos. */
    int32_t shot_frame; /* IXION_DC1_FRAME_* of shot_point. */
    char effect[16];
    float effect_radius;
    /* Ungaga R1: the converted body's name for shot_point when the BodyProfile renamed it (c10b: DC1 "weapon" is
     * DC2 "ef00", tools/profiles/ungaga-c10b.json frame_renames); asked when the resolver declines shot_point. */
    char shot_point_alias[8];
    int32_t effect_stationary; /* MyEntryEffect_*.stationary */
    float effect_speed[4];     /* MyEntryEffect_*.speed, DC1 units per step */
    int32_t effect_life;       /* MyEntryEffect_*.life_time: flight steps before phase 3 */
    /* Xiao R1: CSHOT pool slots the handler fills (12 for Xiao), else 0: the host steps no straight shots. */
    int32_t cshot_pool;
} IxionDc1CharacterInfo;

/* Named-point resolution for getFramePos. Return nonzero after writing out[0..3] in the same space as
 * input.position. Called inside ixion_dc1_tick, only while the handler runs; it must not tick the library.
 * With no resolver, or when it returns zero, the point is input.weapon_tip (R1-R6 Toan behaviour). */
typedef int32_t (*IxionDc1PointResolver)(void *user, int32_t frame, const char *name, float out[4]);
void ixion_dc1_set_point_resolver(IxionDc1PointResolver resolver, void *user);

/* Ungaga R1: DC1's checkCollision (shot_utils.cpp:24-91) for the effect runner. position and movement are in host
 * space (movement = velocity * effect_scale), radius is MyEntryEffect_*.radius[phase] in DC1 units, target its
 * target mode (2 = monsters only). Return IXION_DC1_SHOT_HIT_* and, unless NONE, write the contact to hit_out.
 * Called inside ixion_dc1_tick; it must not tick the library. With no collider every step is NONE. */
typedef int32_t (*IxionDc1ShotCollider)(void *user, const float position[4], const float movement[4], int32_t target,
                                        float radius, float hit_out[4]);
void ixion_dc1_set_shot_collider(IxionDc1ShotCollider collider, void *user);
/* CSHOT_EFFECT::OffEffect(-1) (shot_effect.cpp:308-317): every runner slot retires at once, with no record.
 * For host-side ends DC1 has no handler for: the room retires, the character swaps out.
 * Xiao R1: it also empties the CSHOT pool (every NowShotData slot unused, no record). */
void ixion_dc1_off_effect(void);

/* Returns 0 for an unknown character. */
int32_t ixion_dc1_character_info(int32_t character, IxionDc1CharacterInfo *out);
/* Required before tick. Motion ranges come from the selected character's KEY order. Returns 0 (and leaves the
 * actor reset as Toan) for an unknown character. */
int32_t ixion_dc1_reset(const IxionDc1Config *config);
void ixion_dc1_tick(const IxionDc1Input *input, IxionDc1Output *output);

/* R1 Toan boundary: reset always selects Toan; tick runs whichever character the last reset selected. */
void ixion_dc1_toan_reset(const IxionDc1ToanConfig *config);
/* Cancel transient action/hit state without ticking or resetting combat resources. */
void ixion_dc1_toan_suspend(void);
void ixion_dc1_toan_tick(const IxionDc1ToanInput *input, IxionDc1ToanOutput *output);

#ifdef __cplusplus
}
#endif
#endif
