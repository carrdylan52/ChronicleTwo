# dng_status notes

Dungeon status board (the HUD panels at the top/bottom of the screen). Seven functions, no classes
owned (`class_units.tsv` lists none for this unit). No first-game counterpart (Dark Cloud has no
`dng_status`; its HUD code is elsewhere).

## Types
- `SP_RGBA` (declared here; used by no other unit, no owner in `class_units.tsv`): four `int`s
  r, g, b, a, size 0x10. Evidence: `PrintV` reads `color+0/4/8/0xC` with `lw` and passes them to
  `mgCDrawPrim::Color(int,int,int,int)`; callers build it as four consecutive stack ints
  (e.g. `iStack_3a0..iStack_394` in `DrawMainUnitStatusBord`, `uStack_60..54` in the monster
  board). NULL means 0x80,0x80,0x80,0x80.
- `mgRect<int>` (from `mg_tanime.hpp`) is passed by value to `PrintV` as the glyph of digit 0:
  `left,top` = texture u,v; `right` = glyph width; `bottom` = glyph height (drawn `bottom + 1`
  high). Digit n is at u = `left + right * n`. Every caller uses `Set(0, 0xE8, 0xC, 0xC)`.
  The `mg_tanime` field names (`right`/`bottom`, "inclusive") do not fit this use; not changed here.

## Functions
- `PrintV(x, y, value, texture, glyph, digits, align_right, pitch, color)`: splits `value` into
  `digits` decimal digits (local `int[8]`, first six set to -1), drops leading zeros (at least one
  digit kept). `pitch < 0` -> glyph width. If `align_right != 0`, x += pitch * (digits - shown).
  Draws with `mgCDrawPrim` + `CPreSprite::SetIRect`, Begin(6). Callers pass 1 for "current" values
  and 0 for "max" values, pitch 10, digits 2 or 5.
- `DrawDrumCounter(x, y, value)`: five digits always (10000s..1s, leading zeros kept), 12x12
  glyphs at v=0xE8 of `TEX_SystenFrame`, 15 px apart. Only caller: robot board, value is
  `CBattleCharaInfo::GetNowAbs(0, &out)` first int.
- `DrawActiveItemCursor(int, int, float)` is LOCAL in retail (`local_symbols.tsv`, 0x1BD360), so it
  is not in the header; it must be `static` in the `.cpp`. Draws a rotating cursor (Begin(3),
  alpha blend 2, bilinear) around the active item slot; float = fade 0..1 (alpha = f*128).
  Uses function statics `cur_ang_1005` (float, starts -PI, +1 deg/frame, wraps by -8PI when > PI)
  and guard `init_1006`.
- `DrawMainUnitStatusBord(rate)` / `DrawRoboUnitStatusBord(rate)` / `DrawMonsterUnitStatusBord(rate)`:
  `rate` is `*(float*)(BattleAreaScene + 0x4C)` (passed by `DrawStatusBord`); boards are offset by
  `rate*80` vertically, main bottom panel x by `0x244 - rate*300`; main also uses `rate*128` as alpha
  for the magic sword / status icon rows; monster board draws nothing unless `rate >= 1`.
  Main board: `SubGameRunning()` forces the fully-shown position. Function statics `palanim_1023`
  / `palanim_1222` (float phase, += PI/16 per frame, -PI when > 0; sinf drives the red flash of low
  gauges) with guards `init_1024` / `init_1223`.
  Data: `at_1048__2` (0x38 = 7 x {int x, int y}, magic-sword pip positions), `at_1049` (0x20 =
  4 x {int u, int v}, pip glyph per magic-sword element), `at_1058__2` (0x1C = 7 x u32 attribute
  bit masks tested against `CBattleCharaInfo::GetAttr()`), `at_1059__2` (0x1C = 7 x {s16 u, s16 v}
  icon coordinates in `TEX_StatusIcon`). All are compiler-generated local-array initialisers, so no
  extern declarations.
- `DrawStatusBord()`: called from `DngMainDraw` (dng_main). Clears `WarningGage2` (dng_main,
  0x1ECEF60, 0x20 bytes) words +0x0, +0x4, +0x8 and `LockOnModel + 0xAC` (Ghidra `DAT_01ecef5c`;
  `LockOnModel` is 0x1ECEEB0, size 0xB0), then switches on `*(s16*)(DngUserData + 0x44D96)` =
  active character number (`CUserDataManager::SetActiveChrNo`, userdata unit): 0 or 1 -> main
  board, 2 -> robot, 3 -> monster. An enum for these values belongs in the userdata header (not
  declared here). The same values are tested in dng_main (`LoopDungeonMain`, `DngStep`) and
  dng_hud (`CLockOnModel::Draw`).
- The boards write `WarningGage2` (when shown fully): +0x0/+0x4/+0x8 low-gauge flags (hp < 0.3,
  weapon 0/1 whp < 0.2), +0x10/+0x14/+0x18 the three ratios (float), +0x1C = 1 for robot, 0
  otherwise. `DngStatus` (dng_main, 0x1ECE1E0, 0x1C): +0xC (Ghidra `DAT_01ece1ec`, int active item
  slot index; cursor x = idx*42 + 0x44) and +0x10 (`DAT_01ece1f0`, float cursor fade, +1/6 per
  frame while `CheckRunEvent` of character 0 is set, -1/3 otherwise, clamped 0..1).

## Externals used
`TEX_SystenFrame`, `TEX_DummyIcon1`, `TEX_DummyIcon2`, `TEX_StatusIcon` (maintex, `mgCTexture*`),
`DngMainScene`, `DngUserData`, `BattleAreaScene`, `DngStatus`, `WarningGage2`, `LockOnModel`
(dng_main), `GetBattleCharaInfo()`, `CBattleCharaInfo` getters, `CGameDataUsed::GetNum` (stride
0x6C between the three active item entries), `SubGameRunning()`, `CScene::GetCharacter`,
`CActionChara::CheckRunEvent`.

## Current C++ status

Six functions have active C++ definitions. `DrawMainUnitStatusBord__Ff` remains
behind `NONMATCHING` with its assembly fallback. `PrintV` uses a stack digit
array, `CPreSprite`, and a glyph rectangle passed by value. The durability and
absorption getters write current and maximum values into adjacent integers.
The boards use typed battle, status, gauge, and user-data records.

The main-board draft's two native sprite constructors belong immediately after
the flash `sinf`, at retail +0x1E4/+0x1EC, before the first Initialize. Moving
the declarations to this point reduces its checked-in-profile differences from
1126/1192 words to 1124/1192, with code size still 0x1240 versus retail 0x12A0.
This closer natural correction is retained with the guard. Scalar flash and
durability ratios, or moving render-only glyph/table/absorption buffers after
the sprites, do not improve that result and are not retained.

Remaining differences begin at +0x74 with position-register allocation and
include different spills, branch scheduling and sprite stack slots. The current
sprite locals follow the early buffers in the frame; retail uses sp+0x110 and
sp+0x240, with durability values at sp+0x480. This is a local-lifetime and
stack-layout blocker; no isolated float-order selector has been demonstrated.
Reconsider when a natural rendering scope and buffer declaration arrangement
recovers those retail slots and lifetimes without fake scalar alignments or
constructor-suppression helpers. The linked unit continues to pass with the
main-board assembly fallback.

## Number glyph calls

All status-board callers pass `mgRect<int>` glyph bounds to `PrintV` by value. The MWCC ABI passes the aggregate through its address, so native `PrintV(..., rect, ...)` calls reproduce the same code as the former linker-name aliases.

## Division-check pragma

The unit-level `divbyzerocheck` pragma was redundant with the global MWCC flag; removing it left the full compiled object identical in objdiff.

## Native primitive constructors

The former `MG_DRAWPRIM_MANUAL_CTOR` macro suppressed normal `mgCDrawPrim` construction throughout this unit. Removing it and the explicit constructor aliases makes `PrintV`, `DrawDrumCounter`, the active-item cursor, and the status boards construct their `CPreSprite` locals through C++. `DrawRoboUnitStatusBord` declares its two sprites immediately before first use, after calculating gauge colours, so the calls retain retail order; the already active functions continue to match; the main board retains its guarded checkpoint above.
