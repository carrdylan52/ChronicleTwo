#pragma once

class CCharacter2;
class CActionChara;
class CScene;
class CSceneCharacter;

// Ixion opt-in (DC2_IXION_PLAYER_BODY=toan or goro, with the converted pack served through
// DC2_IXION_EXTRA_DATA): a Dark Cloud character's body and weapon replace Max's base and costume.
bool IxionToanBodyEnabled();

// The DC1 character id (IXION_DC1_CHARA_*: Toan 0, Goro 2) DC2_IXION_PLAYER_BODY selects; 0 when the opt-in is off.
int IxionDc1BodyCharacter();

// The weapon part (Toan's Dagger, Goro's hammer) loaded for a live DC1 body, or null when the body is not one.
CActionChara *IxionToanBodyDagger(CCharacter2 *body);

// The setup stamp of a live DC1 body (0 when the body is not one): every setup of a scene slot gives it a new
// one, so the combat bridge re-binds after a reload into the same slot.
int IxionToanBodyGeneration(CCharacter2 *body);

// Called from the per-character position boundary; logs the DC1 body's motion every 30 frames.
void IxionToanBodyTrace(CCharacter2 *character);

// Register a completed donor rig, including its weapon lifetime.
void IxionToanBodyRegister(CCharacter2 *body, CActionChara *dagger, CScene *scene = nullptr);

// Raw ownership queries run before loaders mutate the rig or its attached parts.
bool IxionToanBodyOwns(CCharacter2 *target);
void IxionToanBodyRetire(CCharacter2 *target);
void IxionToanBodyRetireSceneSlot(const CSceneCharacter *slot);
