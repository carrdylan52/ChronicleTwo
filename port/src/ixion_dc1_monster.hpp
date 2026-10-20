#pragma once

// Ixion: a DC1 (Dark Cloud) monster running its own DC1 behaviour script inside DC2.
//
// Opt-in through DC2_IXION_ROOM_DONOR_MONSTER=<DC1 id>[,<id>...] with encounter enabled. e104a, e42a and e07a have
// built-in retail profiles; other ids supply dungeon/monster/ix_<id>.dc1.json in the private overlay.
// Descriptor: format="ixion-dc1-monster", version=1, name, life, defense, body_radius, no_blow,
// shot_effect (-1, 5, 9 or 29), reward_exp, reward_money, optional kind (normal, no_lock_on,
// mimic_small, mimic_large; DC1 MONSTER_KIND). Other profiles have no ordinary DC2 item drops.
// DC2_IXION_ROOM_DONOR_COUNT=1..15 sets how many monsters the room arranges from the listed kinds
// (default one per kind; IxionRoomArrange in ixion_room.hpp).
// Profiles select content/properties; they do not extend the native table or qualify another script.
// The monster is a native CActiveMonster in a native slot (life gauge, lock-on, damage and kill
// handling stay DC2's). Its body is the converted DC1 pack ix_<id> and its script is DC1's
// <id>.stb, both loaded through the game's own LoadFile from the private DC2_IXION_EXTRA_DATA
// overlay. DC2's CRunScript executes the DC1 program as it is; its EXT calls go to a DC1 native
// table (namespace ixion::dc1rt, DC1 numbering) that replaces DC2's table for this monster only.

class CActiveMonster;

// True when DC2_IXION_ROOM_DONOR_MONSTER names a supported DC1 monster.
bool IxionDc1DonorRequested();

// Scaling policy: 146 for friendly, 70 for faithful. Host defence is applied afterwards.
int IxionDc1ScaledDamage(int dc1_damage, float max_hp, float dc1_hp);
// The selected balance basis (DC2_IXION_DC1_BALANCE): the dc1_hp argument above.
float IxionDc1BalanceHp();

// Loads (once per floor) and places the requested DC1 monster at a DC2 world position, installs
// the DC1 native table and runs the DC1 set-up program. Returns null and logs on failure.
CActiveMonster *IxionDc1SpawnDonor(const float position[4]);
// Roster: DC2_IXION_ROOM_DONOR_MONSTER may list several ids (e03a,e06a,...). Kind i has its own
// private row 0x7000 + i, refer, patched program and native state; kind 0 is IxionDc1SpawnDonor.
int IxionDc1RosterSize();
const char *IxionDc1RosterId(int kind);
CActiveMonster *IxionDc1SpawnDonorKind(int kind, const float position[4]);
// True for a roster kind that DC1's ArrangementPos places at most once (MONSTER_KIND other than
// NORMAL and MIMIC_SMALL, Chronicle monstorunit.cpp:610-616); descriptor field "kind".
bool IxionDc1RosterKindUnique(int kind);

// Mimic reveal. A mimic-kind donor whose STB has DC1 prog_50 spawns hidden (no script, no damage,
// no lock-on). ThinkHost calls IxionDc1HiddenStep first: true means the donor stays hidden this
// tick; on the confirm press within reach it requests the DC2 program mapped to prog_50 and
// returns false. IxionDc1MimicOpens is the pure opening rule.
bool IxionDc1HiddenStep(CActiveMonster *monster);
bool IxionDc1MimicOpens(float distance_dc1, float body_radius_dc1, bool confirm_pressed);
// True for a dead donor (life gone or DC1 death program running): no new reaction applies.
bool IxionDc1RefusesReaction(const CActiveMonster *monster);

// True for a monster that runs a DC1 script through the DC1 native table.
bool IxionDc1IsDonor(const CActiveMonster *monster);
// Installs the private DC1 native table/state on an already loaded private donor slot.
void IxionDc1InstallNatives(CActiveMonster *monster);

// Called by ThinkHost right after RunScript: applies the DC1 movement and turning state to the
// native mover (next_pos, move_speed, rotation) and steps the DC1 alpha fade.
void IxionDc1AfterScript(CActiveMonster *monster);
// Advances guard-search attack age before the donor VM resumes (unpaused ticks only).
void IxionDc1BeforeScript(CActiveMonster *monster);
// Applies the DC1 ground snap/bounce policy after DC2's map mover (only when DC1 gravity natives ran).
void IxionDc1AfterMove(CActiveMonster *monster);

// Called by ThinkHost where it resets the motion step: the DC1 per-call motion speed.
float IxionDc1MotionStep(CActiveMonster *monster, float native_step);

// Forgets a monster (scene retirement or slot reuse).
void IxionDc1Forget(const CActiveMonster *monster);
// Native removal has already counted the kill; close every donor primitive immediately.
void IxionDc1Removed(CActiveMonster *monster);

// Called by ThinkHost right after it makes an attack window's collision primitive (and overwrites
// its damage with the row attack, as monster.cpp:2828-2830): the DC1 sphere's damage and row.
struct ACTION_DAMAGE;
void IxionDc1AttackPrim(CActiveMonster *monster, ACTION_DAMAGE *entry);

// Called once per ThinkHost: steps the DC1 shots (they outlive the monster's death).
void IxionDc1StepShots();

// Durable dungeon event/menu and pause state. Collision hooks call UpdatePause even when DC2
// skips ThinkHost: paused prims stay allocated, untargeted and unstepped until resumed.
bool IxionDc1PauseRequested(int mode, bool script_running, unsigned pause_flags);
bool IxionDc1UpdatePause();
class CColPrim;
bool IxionDc1PrimPaused(const CColPrim *prim);
// Player CheckDamage records accepted outgoing damage on the private donor owner (native 113).
void IxionDc1RecordPlayerDamage(const CColPrim *prim, int damage);
// Consumes a donor guard collision without HP loss or interrupting the DC1 guard script.
bool IxionDc1GuardHit(CActiveMonster *monster, CColPrim *prim);

// Player-owned goo (Monica faithful, ixion_monica_faithful.cpp): the DC1 shot code with the player as owner.
// Owner 0, so the ねば攻撃 row targets monsters only; status 0 (no goo on anyone); the damage is DC1's
// number scaled from the caster's maximum HP by the balance policy. The ball steps with the donor's
// shots (frozen by event pauses) and ends on a monster hit ("hit-monster"), a wall, its 180-frame life, or
// IxionDc1RetirePlayerShots. The donor's own lifecycle (respawn, despawn, forget) never ends it.
bool IxionDc1SpawnPlayerShot(const float from[4], const float target[4], int dc1_damage, float caster_max_hp);
int IxionDc1PlayerShotsLive();
void IxionDc1RetirePlayerShots(const char *why);

// DC1 world units to DC2 world units (the room and monster converters' k).
constexpr float kIxionDc1UnitScale = 1.5584790683f;
// The balance basis in force (DC2_IXION_DC1_BALANCE: friendly 146, faithful 70).
float IxionDc1BalanceHp();

// Monica's belly-flop window (round 2): a player-owned point prim of DC1's kind-3 blow row (吹っ飛び攻撃).
// Owner 0 so only monsters are hit; status 0; damage = IxionDc1ScaledDamage(dc1_damage, caster_max_hp, balance).
// `dc2_radius` is the DC2 prim radius (DC2 hits within 2 x (radius + the target's hit-entry size)).
// Null when no prim is free. The caller moves it each tick and closes it.
CColPrim *IxionDc1OpenPlayerBlow(const float position[4], float dc2_radius, int dc1_damage, float caster_max_hp);
void IxionDc1MovePlayerBlow(CColPrim *prim, const float position[4], float dc2_radius);
// Still the prim opened above (the engine frees prims on a scene change; a freed or reused one is not live).
bool IxionDc1PlayerBlowLive(const CColPrim *prim);
void IxionDc1ClosePlayerBlow(CColPrim *prim);

// Removes the DC1 monster from its slot without a kill (room retirement); ends its shots.
void IxionDc1Despawn(CActiveMonster *monster, const char *reason);
// DRAN-R2. 111 _RUN_SCRIPT: the port-owned id for a DC1 event a donor requests, or -1 (refused).
// Never 1200 (the one DC2 event_no the room accepts) and never the raw DC1 number.
int IxionDc1PortEventId(int dc1_event);
// 115: DC1 owner * 10 + attack for a DC2 attack kind (DAMAGE_KIND_*), -1 when DC1 has no owner.
int IxionDc1HitIdFor(int dc2_kind);
// 134: the percent of damage a DC1 attacker owner takes off the donor's body sphere `col`.
int IxionDc1BodyAttackerPercent(const CActiveMonster *monster, int col, int dc1_owner);
// 134: DC2 prim damage after the DC1 percent is applied to the net damage (prim damage - defence).
int IxionDc1PercentPrimDamage(int prim_damage, int defense, int percent);
// 115/134: records an accepted DC2 hit on a donor (kind, the HIT entry index or -1) and returns the
// prim damage to use; logs the DC1 owner/attack/percent.
int IxionDc1NoteBodyHit(CActiveMonster *monster, int dc2_kind, int entry, int prim_damage);
// Test seams.
int IxionDc1ColOffForTest(const CActiveMonster *monster);
bool IxionDc1ShotEffectForTest(int effect, float &speed, float &radius, int &life, int &damage, float &impact_radius);
