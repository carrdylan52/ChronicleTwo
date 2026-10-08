# pbuggy: reverse-engineering notes

`sgInitBuggy` is currently supplied by retail assembly. Its effect-script
manager allocation initializes the embedded sprite through compiler-generated
class construction; no source-level virtual-table writes are retained.

Buggy sub game (sub game 3 in `subgame`'s dispatchers `sgInitSubGame`, `sgLoopSubGame`,
`sgDrawSubGameChara`, `sgDrawSubGameEffect`, `sgDrawSubGameCharaShadow`, `sgDrawSubGameSystem`).
The player drives a buggy with a gun and bombs and defends a train.

`CharaControl(CScene*, CPadControl*)` retains a C++ draft under `NONMATCHING`.
`InitBomb(CScene*)` uses the verified native body with the floating-point
calibration described below. Other native promotions are retained separately
from the construction fallback in `sgInitBuggy`.

## Types
- The unit owns no classes (`class_units.tsv` has no `pbuggy` rows) and declares no structs.
- `SubGameInfo` (owned by `subgame`) is forward-declared in `pbuggy.hpp`. Fields used here:
  `+0x0` scene, `+0x4` first texture block (`BuggyTexb` = it; Porcuss +1, Muccho +2, Bomb +3,
  Starbull +4, GunEff +5, Sys +6, Effect +7), `+0x8` number of texture blocks to delete
  (`DeleteBlock(info->unk_4 + i)` loop in sgInit/sgExit). Worth naming in `subgame.hpp`.

## Header contents
Only the 7 `sg*Buggy(SubGameInfo*)` entry points are global; all return `int`
(Ghidra `undefined4`/`long`, m2c `s32`; callers store it as the dispatcher's result).
- sgInitBuggy: 0 on any load failure, 1 on success.
- sgExitBuggy: 0 if the scene's player character (`GetCharacter(scene->+0x2E50)`) is null, else 1.
  Deletes charas 0x40..0x67, effect slot 7, stops BGM 0, closes `PolVoice`.
- sgLoopBuggy: 1 after the end fade completes (`RunEvent(RunEventNo)` then `sgExitBuggy`), else 0.
- sgDraw/sgEffectDraw/sgDrawShadow/sgSystemDraw: always 1.

## File-local (static in retail; belong in the .cpp, not the header)
All other functions are LOCAL in `local_symbols.tsv`: `CharaControl(CScene*, CPadControl*)`
(retail suffix `__3`; same-named statics exist in other units), `InitBuggy`, `BuggyDamage(int)`,
`PlayBuggyLoopSe(CScene*, int)`, `BuggyControl`, `InitBomb`, `TakeBombCheck`, `TakeBomb`,
`ThrowBomb(float*)`, `BombBomb`, `NowPutBomb`, `BombControl`, `BombCheck`.
Return types: TakeBombCheck/NowPutBomb `BombStatus == 3`; TakeBomb sets 4 and returns the check;
BombBomb `BombStatus == 6` (zeroes BombVelo, BombCount=0, BombHitObj=1); ThrowBomb returns 1.
Likely `int`/`bool`; confirm from asm when writing bodies.

Every data symbol is LOCAL too, so the header has no `extern`s. Data (gp = 0x3846F0):
- `.sbss` chara pointers (`CCharacter2*`-like, vtable calls): BuggyChara 0x40, PorcussChara 0x41,
  MucchoChara 0x42, BombChara 0x43, StarbullChara 0x44, GunFireEff 0x45, GunHitEff 0x46
  (scene chara slots). `+0x70` is the chara's root `mgCFrame*`.
- Texture block ints: BuggyTexb..SysTexb, EffectTexb (`EffectTexb__2`), EffectTexbNum (=5).
- EffectMan (`EffectMan__2`): `CEffectScriptMan*` (new 0x1190, assigned as scene effect 7).
- EffectBuff (.bss 0x30): `mgCMemory` (Init in `__sinit`, SetHeapMem 20000 bytes).
- PolVoice (.bss, extent 0x20, referenced size 0x14): `sgCPlayVoice` (Close/SetVol; `__sinit`
  zeroes +0, +8, sets +0xC/+0x10 = 1.0f).
- WorkBuff: `new[] 160000` buffer. BuggySndID: `sndLoadSound` id (-1 if none).
- RunEventNo (`RunEventNo__2`): -1, then 0x1F7 when BuggyHP <= 0, 0x1F6 when TrainHP <= 0.
- BuggyHP (.sdata int, init 1, set 3 in InitBuggy; decremented in BuggyDamage),
  BuggyHPf (float, init 3.0, eases toward BuggyHP by 0.05/frame in sgSystemDraw; gauge = /3),
  TrainHP (float, init 1.0; gauge width 173*TrainHP).
- BuggyVelo, BombVelo, StarbullPos (.bss 0x10 each): float[4] vectors.
- GunFireEffDraw/GunHitEffDraw: frame countdowns for sgEffectDrawBuggy.
- Function-local statics: `test_1254`, `init_1255` (BuggyControl), `reload_cnt_1350` (BombControl).

## State values seen (no names established)
- CharaStatus: 0..4 cycle in CharaControl (0->1->2->3->4->0).
- BuggyStatus: 0, 1, 2 (damage; BuggyDamage ignored while 1), 3. BuggySidePos: 0/1 toggled in state 1.
- BombStatus: 1->2->3 (InitBomb sets 3; held/placed), 4 (taken), 6 (thrown), 7 -> 1.
An enum for these, if wanted, belongs in the .cpp since all users are file-local.

## First game
No counterpart in `/home/adubbz/development/chronicle`.

## Bomb initialization calibration

`InitBomb__FP6CScene` marks the bomb available, activates its scene object,
positions the bomb and Starbull character, sets Starbull's rotation to
`(0.0f, pi, 0.0f)`, and starts its motion.

The verified MWCC 3.0 row selects `pbuggy.cpp`, `InitBomb__FP6CScene`,
`binary32`, IEEE bits `0x40490fdb` (the float pi literal), and
`evaluate_first: true`. It restores the retail order of the two differing
instructions in rotation setup. The selector uses the mangled function and
literal bits, with no occurrence indices or source edits.

Validation used the full production mwccgap wrapper, normal section fixup,
and the canonical object checker. All function bytes and resolved relocations
match. The earlier unit comparison had two `PolVoice` alignment-tail issues
(size 0x14 versus a 0x20 retail extent); the verified generic BSS padding now
handles that tail without enlarging the class. Merged surrounding promotions
need a fresh whole-unit comparison. The calibration's native body is preserved.

See [MWCC matching notes](../../../../docs/MWCC.md) for the compiler-state
rationale and verification workflow.
