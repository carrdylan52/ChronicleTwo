# actscript: notes

Action-script (player, ridepod, monster `CActionChara`) external functions `_XXX(RS_STACKDATA *, int)`,
their argument helpers, gun/magic shot helpers, and the action external-function table. Same shape
as `runscript_opcodes` (monster scripts) and `event_func`; see `ps2/re/docs/runscript_opcodes/notes.md`
for the shared helper semantics. No first-game counterpart unit (the first game has no
`CActionChara`).

## Owned types
`class_units.tsv` lists no class owned by actscript. VM types come from `runscript.hpp`; the table
row type `RS_EXTFUNC_INFO` is already declared in `runscript_opcodes.hpp` (include it in the .cpp).

- `ACTION_INFO` (0x10, **name not retail**): type of global `action_info` (0x1F3D170, .bss, size
  0x10 from main.symbols.txt). Evidence (all absolute `%lo(action_info + n)` accesses in the
  binary; the struct is never addressed as a whole):
  - +0x0 `CActionChara *chara`: written by `CActionChara::InitScript`, `ResetAction`, `RunScript`
    (`sw this`); read by ~70 actscript opcodes and passed as `this` to CActionChara members.
  - +0x4 `mgCCameraFollow *camera`: written in `RunScript` from
    `CScene::GetCamera(scene, GetCameraID(scene, "MainCam"))`; read by `_CHECK_FRONT_KEY`,
    `_CHECK_BACK_KEY` and actionchara's *MoveIF functions, always as `this` for
    `mgCCameraFollow::GetAngle()`.
  - +0x8 `RUN_SCRIPT_ENV *env`: written in `RunScript` (its 2nd argument), zeroed in
    `ResetAction`; read in `CActionChara::EntryThrowItem` (`env->item_chara`, `env->texb` at +4).
    Not read in actscript itself.
  - +0xC: never accessed anywhere -> `unk_c[4]`. The 0x10 size could be bss padding before the
    following `ext_func` (0x1F3D180); kept at 0x10 to match the symbol extent.

## Globals
| Symbol (config) | Addr | Size | Binding | Type / meaning |
|---|---|---|---|---|
| `nowScene__2` (retail `nowScene`) | 0x37E44C | 4 | **global** | `CScene *`, extern in header. Set by `CActionChara::RunScript` (`sw $a1, -0x62A4($gp)`); read by actscript (`_CAMERA_QUAKE`: `nowScene+0x2F90` = dng_main's `DNG_BATTLE_AREA`, fields +0x70 float, +0x74 float, +0x78 s16; `_CHECK_PAUSE`, `_GET_MONSTER_NOWSTS`, `_GET_TRG_DISTANCE` -> `GetTargetDist(CScene*)`, ...) and by 12 actionchara members. Another `nowScene` (0x37D4E4) is a runscript_opcodes local. |
| `action_info` | 0x1F3D170 | 0x10 | **global** | `ACTION_INFO`, extern in header (actionchara writes it). |
| `LastCInfo2__2` (retail `LastCInfo2`) | 0x37E450 | 4 | local | `static ACTION_DAMAGE *LastCInfo2;` result of `CActionChara::EntryDamage2` in `_SET_DMG2` (null check -> printf "CACT:DMG_ENTRY_ERR %s\n" at_1202__2; writes +0x14). |
| `ext_func__3` (retail `ext_func`) | 0x1F3D180 | 0x400 | local | `static int (*ext_func[256])(RS_STACKDATA *, int);` |
| `ext_func_info__3` (retail `ext_func_info`) | 0x35A800 | 0x298 | local | `static RS_EXTFUNC_INFO ext_func_info[83]`: 82 entries + `{0, -1}` terminator (the data file has 8 more zero bytes after it: alignment). |
| `sw_1617`/`init_1618`, `canon_slot_1620`/`init_1621`, `cnt_1661`/`init_1662` | .sbss | 4 each | local | function-local statics of `_SHOT`: `static int sw = 1;` (toggles 0/1), `static int canon_slot = 0;` (cycles 0..3, indexes a pair table of 8 pointers on the stack), `static int cnt = 0;` (cycles 0..2). Initialised lazily (guard bytes `init_*`), i.e. non-constant-initialised statics. |
| `at_2004__4` | 0x375EE0 | | | "chr]same ext_func_no!!!\n" (duplicate-number printf). |
| `at_2005__3` | 0x375F00 | | | "ext func over!!" (number outside 0..255). |

Globals used but owned elsewhere: `DngUserData` (dng_main), `GamePad` (`GamePad__2`, mainloop),
`PadCtrl` (mainloop), `ColPrimMan`, `MachineGun`, `RocketLauncher`, `LaserGun` (dng_main).

## Functions
Global (in header): `SetActionScript`, `SetActionExtendTable`, both called only from
`CActionChara::LoadActionFile`. Every other function in the unit (all 82 opcodes, the helpers,
`ParabolicInitialVector`, the five `Shot*`) is LOCAL in retail -> `static` in the .cpp.

- `int SetActionScript(CRunScript*, char*, mgCMemory*)`: `Alloc(0x40)` stack (0x80 RS_STACKDATA),
  `Alloc(0x180)` call stack (0x200 RS_CALLDATA), `load(prog, stack, 0x80, call, 0x200)`,
  `ext_func(ext_func, 0x100)`, returns 1. Identical to `SetMonsterScript`.
- `void SetActionExtendTable()`: identical to `SetMonsterExtendTable` (zero table unrolled x8;
  per row until `func == 0`: duplicate-number check -> printf + `for(;;);`; range check
  0..255 -> printf, else `ext_func[no] = func`).
- Helpers (`__3` suffixed symbols): `int GetStackInt`, `float GetStackFloat`, `char *GetStackString`,
  `void SetStack(RS_STACKDATA*, int)`, `void SetStack(RS_STACKDATA*, float)`; semantics as in
  runscript_opcodes (RS_FLOAT/RS_INT conversion; store only through RS_PTR).
- `void ParabolicInitialVector(float *out, float *from, float *to, float gravity, float time)`:
  `out = {(to.x-from.x)/t, -((to.y-from.y)*2 - t*g*t)/(t*2), (to.z-from.z)/t, 1.0}`; used by `_RELEASE_OBJ`.
- `void ShotMonicaMagic(float*, float*, float)`, `ShotNormalGun(float*, float*)`,
  `ShotMachineGun(float*, float*, char*, float)`, `ShotGrenadGun(float*, float*)`,
  `ShotLaserGun(float*, float*, int)`: all return void; use `ColPrimMan` and the gun managers.
- Opcodes return `int`: most are `return argc == N` (Ghidra shows `bool`) after acting only when
  the argument count matches; others return 1 (or 0 on a null scene, e.g. `_CAMERA_QUAKE`).
- CActionChara offsets touched (named in actionchara.hpp): 0x588, 0x5A0, 0x690 (front vector),
  0x6A4 (attack type), 0x6A8, 0x712 (s16 prog number, `_PROG_SET/_GET`), 0x714, 0x764, 0x76C
  (`_SET_MENU_FLAG`, byte), 0x770, 0x772, 0x7A0 (blow vector), 0x7CC, 0x7D6, 0x7D8, 0x7DC (effect
  script manager), 0xC10 (+n*0x20, shot slots), 0xF54..0xF70 (sound slots, `_SET_SND`).

## External-function numbers (`ext_func_info` order, number=function)
0 INIT_SCRIPT, 1 PROG_SET, 2 PROG_GET, 3 GET_ATTK_TYPE, 4 GET_MOVE_TYPE, 5 SET_MOVE_SPEED,
30 SET_PALLET, 31 CHECK_EQUIP, 32 CAMERA_QUAKE, 33 CHECK_PAUSE, 34 GET_STATUS_ATTR, 35 SE_PLAY,
36 SE_LOOP_PLAY, 37 GET_SHOT_TYPE, 38 GET_MONS_ID, 39 GET_FRONT_VEC, 40 GET_PADON, 41 GET_PADDOWN,
42 GET_PADUP, 43 GET_BTN, 45 GET_PAD_HISTORY, 46 RESET_PAD_HISTORY, 47 GET_ACUMU_PAD,
48 RESET_ACUMU_PAD, 49 RUN_MAIN_MOVE, 50 RUN_SHROW_MOVE, 51 RUN_TAME_MOVE, 52 RUN_HOLD_MOVE,
59 SET_MENU_FLAG, 53 GET_POS, 61 GET_ROT, 54 CHECK_FRONT_KEY, 55 CHECK_BACK_KEY, 56 SET_BLOW_ANGLE,
57 SET_BLOW_MOVE, 58 BLOW_START, 60 RUN_ROBO_MOVE, 71 SET_DMG2, 72 SET_OBJ, 73 SET_BODY,
75 SW_EFFECT, 76 SET_SND, 77 SET_ACCUME_FX, 78 SET_ACCUME_FLAG, 90 GET_MONSTER_NOWSTS,
91 SET_MURDEROUS, 92 GET_TRG_DISTANCE, 93 SET_TRG_ANGLE, 94 SET_GUARD_FLAG, 95 SET_MUTEKI,
96 CHECK_HAND_OBJ, 97 SET_ITEM_USED, 98 THROW_HAND_OBJECT, 99 CHECK_CATCH, 100 RELEASE_OBJ,
101 SET_SHOT, 105 SET_SPECIAL_SHOT, 102 SHOT, 103 GET_OBJECT_POS, 104 SET_DIR_GUN,
106 GET_NOW_HP_RATE, 107 SET_BOMB, 108 GET_ACTION_CODE, 109 GET_ATTK_POINT, 110 GET_RING_COLOR,
130 SET_MOS, 131 CHECK_MOS_END, 132 NOW_MOS_WAIT, 133 GET_MOS_STATUS, 134 SET_XCHG_STEP,
135 SET_MOS_STEP, 136 TRG_ON_MOS, 137 RESET_MOS, 138 SET_DEFAULT_MOS, 140 SET_NEBA2,
139 NOW_MOS_CHGWAIT, 150 ESM_CREATE, 151 ESM_SET_VECT1, 152 ESM_SET_VECT2, 153 ESM_FINISH,
154 ESM_DELETE, 155 ESM_SET_VALUE; then `{0, -1}`.

## Unresolved
- Retail name of the `action_info` struct type (`ACTION_INFO` is ours); meaning of +0xC.
- Per-opcode argument meanings were not analysed (header task); offsets above are from m2c.

## Division-check pragma

The unit-level `divbyzerocheck` pragma was redundant with the global MWCC flag; removing it left the full compiled object identical in objdiff.

## `_SHOT` floating argument schedule

Both attack-type 40 and 90 branches finish their effect setup by calling
`CEffectScriptMan::SetValue(int, float, int, int)` with value index 4 and
160.0. m2c and retail assembly confirm the same overload and constant in both
branches. `_SHOT` is 0x900 bytes; the deterministic default policy differs
from retail only at +0x8B0/+0x8B4/+0x8B8/+0x8C4 in the later call. These four
words exchange the v0/v1 registers used for the constant and `action_info`
loads, without changing function size or relocation count.

A binary32 160 (`0x43200000`) evaluate-first row, including one scoped to
`SetValue__16CEffectScriptManFifii`, fixes the later call but creates the same
four-word exchange at the earlier +0x6C0/+0x6C4/+0x6C8/+0x6D4 call. A local
pointer or reference to the earlier effect manager does not resolve this
conflict; verified consumer logging still identifies two binary32 160
arguments to the same callee in each compiler pass.

Natural source/type trials establish these boundaries:

- Replacing the later `float(160.0)` with `float(160)` or `160.0f`, or passing
  a named float or integer value converted to float, preserves the complete
  default-policy object byte-for-byte.
- A named integer value at the earlier call also preserves the conflict
  under the binary32 evaluate-first row.
- Inline `float(160.0)` and a named `const double` value narrowed to float
  do not retain a selectable binary64 160 identity; that selector is rejected
  as unconsumed.
- A non-const double local does retain a binary64 identity, but the compiler
  emits `dptofp`, growing `_SHOT` to 0x908 and the unit's checked size from
  0x47FC to 0x4804, with 1112 rather than 1111 relocations. The extra call
  remains under both the default policy and a binary64 evaluate-first row.

No source form or policy above passes the complete unit. The remaining
reconsideration trigger is retail-supported source evidence for a real
expression distinction that survives optimization without a conversion call,
or a separately validated stable compiler identity that distinguishes those
expressions. Occurrence selectors and invented helper functions do not follow
from this evidence.
