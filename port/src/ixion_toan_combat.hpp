#pragma once

#include <cstdint>

class CCharacter2;
class CActionChara;

// Ixion: what Dark Cloud's combat bridge (ixion_toan_combat.cpp) exposes to the rest of the port.

struct IxionToanHud {
    float gauge;        // Dark Cloud's action gauge, 0-100 (BtActStatus.action_gauge).
    float charge_time;  // Charge time while charging (the character's charge-loop action), from 1.0; 0 otherwise.
    int   charge_level; // 0, 1 at charge_time 1.5, 2 at 2.5 with the event skill (Toan).
    int   action;       // Running donor action, or 0.
};

// True when Dark Cloud's combat drives the DC1 body (DC2_IXION_PLAYER_COMBAT=dc1 with a DC1 body, built with the
// donor): the body setup then rescales the donor KEYs once per load.
bool IxionToanCombatDrivesKeys();

// Called once per DC1 body load, right after LoadPack: rescales the KEYs the selected character's handler drives
// (the library table's donor_keys) so they play at Dark Cloud's rate under DC2's NormalDrive. No-op when off.
void IxionToanCombatRescaleKeys(CCharacter2 *body);

// Ungaga R1: the Ixion room retired (ixion_room.cpp RetireScene); any DC1 shot still flying in it ends. No-op when off.
void IxionToanCombatRoomRetired(const char *reason);

// True, with the gauge and charge, while Dark Cloud's combat drives the main character.
bool IxionToanCombatHud(IxionToanHud *out);

// Toan's status panel (ixion_toan_hud.cpp): DC2's HP/item half, Dark Cloud's gauge in the weapon half.
void IxionDrawToanUnitStatusBord(float rate, const IxionToanHud &hud);

#include "padcontrol.hpp"
#include "ixion_toan_bridge.h"

struct ToanCombatFrame {
    bool        active;
    ixion_toan::CrossTake cross_take;
    PAD_STATUS  saved_pad;
    PAD_STATUS  saved_previous;
    int         cross_down;
    int         cross_held;
    int         cancel;
    float       move_x;
    float       move_z;
    int         jumping; // Dark Cloud's jump owns vertical motion and landing this tick.
};

void ToanCombatBegin(CActionChara *self, ToanCombatFrame &frame);
void ToanCombatTick(CActionChara *self, ToanCombatFrame &frame);

class CColPrim;
// Control/camera suspension closes damaging records but keeps the loaded donor session.
void IxionToanCombatSuspend(CCharacter2 *body, const char *why);
void IxionToanCombatRetire(CCharacter2 *body, const char *why);
void IxionToanCombatPrimInvalidated(const CColPrim *prim);
bool IxionToanCombatOwnsPrim(const CColPrim *prim);
std::uint64_t IxionToanPrimIncarnation(const CColPrim *prim);
