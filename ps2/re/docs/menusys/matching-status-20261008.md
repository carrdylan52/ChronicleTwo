# menusys guarded-function status (historical morning snapshot)

This records the older `0abce376` baseline. See
[the midday status](matching-midday-20261008.md) for the `c79e57c` lane results.
The counts and guard descriptions below describe that earlier snapshot.

Reference: PAL SCES_511.90, base
`0abce376a0d178797f62ae7e0e67b407fb67bd62`, MWCC 3.0-011126 with `-O3,p`.
All thirteen functions below remain guarded. No source or header experiment
is retained; no shared-header patch is proposed. Unit coverage stays at
152 matched / 13 guarded, and repository coverage stays at 6,651 matched /
204 guarded / 17 asm-only out of 6,872 game functions.

The default `draft.sh menusys` compares all C++ drafts together. It reports
153 matches / 12 differences across 165 functions because one still-guarded
function, `MenuWeaponBuildUpDraw`, matches in that compiler context. That is
not a promotion receipt. The word counts mask relocation operands and include
zero padding to the retail manifest extent. A shorter zero-padded section
does not by itself imply missing instructions.

## Before and after measurements

Every final score equals its initial score. Sizes are C++ section size / retail
manifest extent; `same` means the checker reports no extent difference.

| Mangled symbol | Before → after | Size |
|---|---|---|
| `MenuWeaponBuildUpDraw__FRi` | whole draft MATCH → MATCH; isolated four differing words / four differing linked bytes | same, 0xE80 |
| `LRCheck__13CMenuItemInfoFi` | 2/220 → 2/220 differing words | same, 0x370 |
| `CalcTex__13CMenuItemInfoFv` | 7/832 → 7/832 differing words | 0xCF8 / 0xD00 |
| `CheckEnableHaveItemNum__Fv` | 13/212 → 13/212 differing words | 0x34C / 0x350 |
| `MenuPosFormValueSetCharaRobo__FP9ROBO_DATAi` | 19/296 → 19/296 differing words | 0x498 / 0x4A0 |
| `MenuItemDebugDraw__Fv` | 34/1260 → 34/1260 differing words | 0x13A4 / 0x13B0 |
| `MenuDataSwap__FP13CGameDataUsedP13CGameDataUsedi` | 36/228 → 36/228 differing words | same, 0x390 |
| `CommonSetMoveItemClass__FPA4_i` | 42/248 → 42/248 differing words | 0x3D8 / 0x3E0 |
| `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | 208/240 → 208/240 differing words | 0x31C / 0x3C0 |
| `IsAskExtend__13CMenuItemInfoFii` | 625/668 → 625/668 differing words | 0xA1C / 0xA70 |
| `MenuItemDebugKey__Fv` | 1207/1460 → 1207/1460 differing words | same, 0x16D0 |
| `MenuItemSelectInit__FP9mgCMemoryPii` | oversize → oversize, +0x14 bytes | 0x2A4 / 0x290 |
| `PushKey__13CMenuItemInfoFii` | oversize → oversize, +0x14 bytes | 0x1BB4 / 0x1BA0 |

## Parks and reconsideration triggers

- `MenuWeaponBuildUpDraw__FRi` — **compiler context / temporary allocation**.
  The isolated image differs at +0x960/+0x964/+0x968/+0x970: retail gives the
  bottom-bar Y addition `v1` and X literal `v0`; the draft gives them `v0` and
  `v1`. Reconsider after a natural earlier-function promotion changes the
  game-build compiler context, or a reduced reproducer explains this choice.
  Require an isolated image match even if the whole draft still says MATCH.
- `LRCheck__13CMenuItemInfoFi` — **branch-delay return scheduling**.
  At +0x264/+0x268 the draft branches to +0x34C with `move v0,zero` in its
  delay slot; retail branches to the shared zero assignment at +0x348 with
  a `nop` delay slot. A positive rejection branch leaves two differences;
  an m2c-style fallthrough switch grows the section to 0x374. Reconsider
  when a natural control-flow form preserves the shared assignment and
  retail-sized section in a reduced compiler probe.
- `CalcTex__13CMenuItemInfoFv` — **register allocation**.
  Seven words exchange `s3` held type and `s0` equipment counter. The isolated
  game build has seven differing bytes, first at 0x246615. Reconsider when a
  reduced equipment-search probe gives the retail register assignment without
  unused operations, or an earlier natural promotion changes this unit's
  compiler context.
- `CheckEnableHaveItemNum__Fv` — **register allocation in nested loops**.
  Differences at +0x12C/+0x144/+0x14C/+0x154/+0x178/+0x1B0/+0x1B4 exchange
  the active-slot counter and pointer; six words at +0x2AC..+0x30C exchange
  the final loop offsets. Reconsider when a reduced typed nested-loop probe
  resolves either group independently. The retail accumulated
  `chara = chara + i` behavior must be preserved.
- `MenuPosFormValueSetCharaRobo__FP9ROBO_DATAi` — **colour register allocation**.
  The nineteen differences are the red update at +0x238 and six unrolled
  RGB triples. The isolated image also differs by nineteen bytes, first at
  0x24CD5A. Reconsider after a reduced red/green warning-colour probe resolves
  the allocation or a natural earlier promotion changes compiler context.
- `MenuItemDebugDraw__Fv` — **argument and loop-register scheduling**.
  The differences comprise a commutative item-index addition at +0x190,
  float argument setup at +0x380/+0x950/+0x114C/+0x1258 and nearby words,
  and the three-name loop at +0xD4C..+0xD74. Reconsider after reproducing
  retail's float-call argument order and the loop's `s2` counter / `s1`
  offset separately. The current m2c entry point also needs jump-table input.
- `MenuDataSwap__FP13CGameDataUsedP13CGameDataUsedi` — **result-table scheduling**.
  Non-tail behavior agrees, apart from branches whose common tail target
  shifts by one word. The meaningful tail divergence begins at +0x31C;
  presence flags and table loads differ, and the epilogue starts one word
  later. Reconsider when a reduced `{0,2}` copy and `{1,3}` lookup probe
  reproduces the retail sequence and source/destination flag registers.
- `CommonSetMoveItemClass__FPA4_i` — **typed row-copy / short comparison**.
  The differing region is +0x5C..+0x108; subsequent instruction positions
  agree from +0x10C. The draft lacks retail's row-pointer copy and emits
  two 64-bit sign-extension shifts before its equipment-type comparison.
  Reconsider with a reduced copy/branch probe using the verified short
  layout; an array-copy loop and a named short alone do not solve it.
- `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` — **placement construction**.
  At +0x5C retail branches on `v0` with the pointer copy in the delay slot;
  the draft copies first and branches on `s2`. Its native action-character
  constructor sequence is also shorter. Reconsider only after the dedicated
  allocation/constructor lane supplies a compliant solution.
- `IsAskExtend__13CMenuItemInfoFii` — **placement construction and dispatch**.
  Retail's action-character allocation tests `v0` at +0x5B8 and copies it
  at +0x5BC; the draft copies at +0x5B4 and tests `s0` at +0x5B8. Earlier
  dispatch also differs, first at +0x5C. Reconsider after the dedicated
  allocation/constructor solution, then isolate the dispatch differences.
- `MenuItemDebugKey__Fv` — **placement construction and argument setup**.
  The action-character allocation at +0x408/+0x40C has the same prohibited
  copy-before-test mismatch. Float argument setup already differs at
  +0x3C8..+0x3DC, and the allocation shifts most following instruction
  positions by a word. Reconsider after the dedicated allocation solution,
  then compare the float setup independently.
- `MenuItemSelectInit__FP9mgCMemoryPii` — **placement construction and constructor
  boundary**. Retail's +0x60 branch tests `v0` and copies the pointer in
  the delay slot. The draft branches on `s2`. Retail places both screen
  rectangle setters and cursor/line initialization inside construction;
  the draft's rectangle setters run after construction. Reconsider after
  the dedicated null-branch solution and evidence for that constructor
  boundary. No constructor changes were attempted in this lane.
- `PushKey__13CMenuItemInfoFii` — **oversize command dispatch / tail**.
  Branch destinations reflect a shifted tail; the first non-branch mismatch
  is the argument register at +0x640, and command bodies diverge further
  thereafter. The final colour-reset loop also differs. Reconsider after
  resolving the command dispatch and final loop as separate typed probes;
  the raw +0x14 size excess must vanish before linked-layout validation.

## Analysis and validation evidence

All targets were inspected against retail assembly and `decompile.sh` was used.
The current m2c entry point fails to recognize jump tables for `CalcTex`
(assembly line 707), `PushKey` (530), `MenuItemDebugKey` (29), `IsAskExtend`
(159) and `MenuItemDebugDraw` (36). These failures are recorded in receipts;
the tables' current `at_*` names are not recognized as jump-table names.
The successful m2c outputs and the existing documented drafts remain useful
for the smaller functions. No generated assembly or tooling was edited.

Local experiment hypotheses, changes and results are in
`.private/experiments-menusys.md`. Complete logs are in `.private/receipts/`:

- `menusys-draft-before.log` and `menusys-draft-after.log` — all per-symbol scores.
- `coverage-before.log` and `coverage-after.log` — complete coverage inventory.
- `E06-promote.log`, `E07-promote.log`, `E08-promote.log` — isolated weapon-board failures.
- `E20-promote.log`, `E20-game-object-score.log` — isolated `CalcTex` evidence.
- `E21-promote.log`, `E21-game-object-score.log` — isolated ridepod evidence.
- `*-before.diff` and `*-m2c.log` — retail comparisons and decompiler output/errors.
- `menusys-promote-final.log`, `build-final.log`, `objects-final.log` — final
  guarded game-object link, full PAL verifier and complete-object checks.

The four differing weapon-board words are distinct from four differing bytes:
each instruction differs in one byte. The normal guarded build retains the
retail bytes; final validation therefore checks preservation, not promotion.
