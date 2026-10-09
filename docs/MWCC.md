# MWCC 3.0 matching notes

ChronicleTwo game source uses MWCC 3.0-011126 with `-O3,p`, readonly strings,
exceptions and RTTI disabled, and division checks enabled. The
[Satan's Fiddle integration](../scripts/build/SATANSFIDDLE.md) makes verified
compiler state explicit without changing game source. The executable hash and
hook opcode signatures must match; a version string alone cannot authorize a
memory hook.

## Verified compiler state

**Helper-call history.** Compiler helpers accumulate masks of argument registers
they read. These masks survive translation-unit boundaries during a combined
compile, changing interference and register allocation in later functions.
The profile seeds integer and floating helper masks at translation-unit entry;
normal helper calls still accumulate history afterward. These masks describe
helper arguments, not a general register reservation policy.

Direct measurements in this pinned 3.0 compiler establish GPR mask `0x30` for
unsigned 64-bit division and `0x10` for double-to-float conversion, with FPR mask
zero in both cases. The calibrated unit rows use `0x30`, except `nd_meswin.cpp`
uses `0x10`. Do not insert non-retail functions in discarded sections to seed
this history. Replacing those functions with profile rows preserves every
allocated byte and resolved relocation, including existing unmatched functions.

**Floating argument evaluation order.** A floating constant's internal
evaluate-first byte can retain compiler-arena contents. Call lowering uses
that byte to decide whether to materialize an argument in its early walk.
Initializing the annotation path alone is insufficient: lowering can create
fresh constant nodes afterward. The verified argument-consumer hook initializes
direct constants before the read and reapplies stable overrides. Verified
assignment wrappers and compiler-registered pooled literal loads are recognized
for explicit selectors; ordinary variable expressions keep normal annotation.

Expression identities comprise source basename, mangled enclosing function,
binary32/binary64 type and exact IEEE bits. An optional mangled `callee` resolves
different requirements for the same value in one function. Scoped rows apply at
consumption and take precedence over unscoped rows. No occurrence counts,
ordinals, instruction addresses or compiler-arena addresses select expressions.
Unmatched selectors are errors, so source changes cannot silently leave stale
calibration behind. Signed zero and NaN payloads remain distinct identities.

**Nested call arguments.** Some calls in one function need opposite schedules
despite sharing the outer callee and constant. At argument consumption, the
verified 3.0 call AST exposes sibling call expressions. A scoped selector may
identify a nested call by its mangled callee and either a typed constant at a
formal argument index or a nonliteral variable load at that index. The index
is a source argument position, not a call occurrence. `RoboWalkMoveIF` uses
`unitRotation`'s 16.0f argument to preserve zero across that call;
`RoboAirMoveIF` instead selects the call whose angle is a local variable.
The pinned Satan's Fiddle source patch implements and tests this generic form.

**Pooled literal aliasing.** The separate bug that treats a literal's value buffer
as variable alias metadata is verified for MWCC 2.3.3. No affected alias path is
validated for this 3.0 image. It can pool constants under other optimization
options, including `-O2`; that does not establish the alias bug. The 3.0 profile
therefore omits literal-reload policy settings.

## Retail calibration examples

| Translation unit and function | Verified scheduling policy |
|---|---|
| `mapjump.cpp`, `ExitInterior__FP6CScenePi` | binary32 zero (`0x00000000`) first restores the retail stack frame and float preservation across the angle-limit call. |
| `pbuggy.cpp`, `InitBomb__FP6CScene` | binary32 pi (`0x40490fdb`) first restores the retail instruction order. |
| `dngmenu.cpp`, `Initialize__11CDngFreeMapFv` | binary32 286 (`0x438f0000`) first restores the initial rectangle argument order; the whole unit passes after native promotions. |
| `dngmenu.cpp`, `CheckIsViewMove__11CDngFreeMapFiiRfRf` | GPR helper-history seed `0x30` preserves coordinate-copy order and the final displacement after the floating branch; the whole unit passes with the native function. |
| `menumain.cpp`, `MenuInternSelectDraw__Fv` | binary32 80 (`0x42A00000`) and 350 (`0x43AF0000`) evaluate first for `DrawMenuFillBox` only; direct arguments and inline strings pass the whole-unit check. |
| `event_func.cpp`, `_SET_CROSSFADE__FP12RS_STACKDATAi` | binary32 one (`0x3f800000`) first only for `CrossFadeOut__10CFadeInOutFiif`; sibling `CrossFadeIn` and `CrossFade` calls retain false. |
| `scenesnd.cpp`, `SePlayFoot__6CSceneFiiPf` | binary32 1200 (`0x44960000`) first emits it before 160, as retail does. |
| `gyoracesim.cpp`, `CharacterBonus__FP12grFISH_PARAMP15RACE_FISH_PARAMi` | zero and 0.01 (`0x3c23d70a`) first preserve both the earlier zero/0.01 calls and the later call's 0.01-before-one materialization. |
| `menuaqua.cpp`, `Draw__9CAquariumFv` | binary32 120 (`0x42f00000`) and 242 (`0x43720000`) evaluate first for `DrawMenuFillBox` only, preserving the retail debug-panel width/height preparation before its top coordinate. |
| `menuchr.cpp`, `Draw__15CMenuCostumeSelFv` | binary32 36 (`0x42100000`) first for `DrawMenuFillBox` only emits the help box's X before its Y subtraction, as retail does; the whole unit passes. |
| `actionchara.cpp`, `RoboWalkMoveIF__12CActionCharaFi` and `RoboAirMoveIF__12CActionCharaFii` | Nested `unitRotation` argument identity selects the zero load order for only the differing rotation calls; the complete unit passes. |

These rows were accepted through the canonical object comparison. They establish
the listed functions' bytes and resolved relocations, not whole-unit matching
when other source or data-layout failures remain.

`mg_texture.cpp` needs one shared `#pragma optimization_level 2` region around
its three hash-table methods. Direct indexing yields the retail table access;
whole-unit level 2 changes unrelated functions. The scoped region preserves
the exact 0x3674-byte object and 160 resolved relocations.
Unit-specific evidence is in the tracked mapjump and event_func RE notes and
[pbuggy calibration notes](../ps2/re/docs/pbuggy/notes.md).

## Optimization levels and deferred template code

`optimization_level 2` enables global common-subexpression elimination without
strength reduction or loop rotation, as observed in
[mg_tanime](../ps2/re/docs/mg_tanime/nmmisc-20261008.md). Level 4 runs the IR
optimizer twice. Second-round propagation can remove first-round constant
temporaries; CSE recreates them with the lowest available virtual-register
numbers, below the surviving temporaries
([movie](../ps2/re/docs/movie/nmmisc-20261008.md)).

Functions using class templates can be compiled when the next top-level
declaration begins. Pragmas between their closing brace and that declaration
therefore govern them; template-free functions in the same probe use the state
at their definition ([mg_tanime](../ps2/re/docs/mg_tanime/nmmisc-20261008.md)).

## Source matching and verification

Read unit documentation first and analyze retail with `./decompile.sh SYMBOL`
using m2c. Establish dependency types and layouts before changing expressions.
An early `mtc1`, changed saved register or changed stack frame can be a compiler
state difference; test the deterministic profile before changing source to
imitate incidental allocation. Use correctly typed literals and real field
layouts rather than pointer arithmetic or instruction-shaped source.

Virtual-register numbering and live-range webs determine colouring; declaration
position alone does not predict a physical register. A later meaningful reuse
of an existing variable or an optimizer common subexpression can account for a
value coloured after the early locals. Named class locals take frame
slots in declaration order before call-argument temporaries.
`x = x < 0.0f ? -x : x` and `if (x < 0.0f) x = -x;` schedule the
surrounding loads differently. Probes of `((int)left)` in `CommonBoardDraw`
showed that a value-preserving cast can change virtual-register colouring. A
repeated
expression written inline (`top + heights[row]`) becomes one temporary
coloured after earlier temporaries, where a named local would be coloured
with the declared locals. `MenuCharaChangeStarDraw` and
`CMenuCostumeSel::Draw` (menuchr) and `CommonBoardDraw` (menudraw) show
these between them.

For each calibration, copy the profile privately, compile with the repository
wrapper and canonical flags, run `scripts/build/fixup_sections.sh`, then run
`scripts/build/check_objects.py`. Accept a row only when the target has zero
byte and resolved-relocation differences and existing unit failures are
preserved or resolved. Fixup is required: mwccgap's temporary `.dead` sections
are removed by the normal build stage.

Objdiff source-only objects use the same Satan's Fiddle profile as linked
objects; native template names are mapped structurally to retail identities.
The objdiff target preparation localizes explicit switch `jlabel .LXXXXXXXX`
symbols so they do not split native functions into artificial report rows.
This changes only the comparison object's symbol metadata; the linked game
objects retain the exported labels required by separately assembled tables.
A fuzzy percentage is diagnostic, not proof of an exact match. `INCLUDE_ASM`
and inline assembly do not qualify as matched native decompilation. Internal
class initializers must be generated naturally by the compiler.

## Branch delay slots

A conditional branch may take the first instruction of its target block into
its delay slot (and retarget past it) only if no earlier branch has already
taken that instruction; in matched retail code no target loses the same
instruction to two conditional branches. An unconditional jump to the block
also counts, and once filled it looks the same as an early return
(`b exit; move v0,zero`). When retail shows a `nop` where MWCC steals a shared
`return 0`, look for an earlier natural jump to that block, such as a
`switch` default falling out to the final return.
`CMenuItemInfo::LRCheck` (menusys) and `CheckOmakeVtuto` (menuop) are the
references. In LRCheck, four sparse case labels sharing one body compare in
the reverse of their written order.

The conditional absolute value `p = p < 0.0f ? -p : p;` reproduces retail's
`nop` placement at the four `TexAnime` joins; equivalent `if` forms move those
slots ([mg_tanime](../ps2/re/docs/mg_tanime/nmmisc-20261008.md)).

## Register colouring of loop and snapshot locals

A loop's own index, or the first use of a variable, is coloured before the
loop optimizer's offset values; a later use of a variable first used
elsewhere (for example an unbraced `case` index reused by a later case) is
coloured after them. Declaration position at function scope does not change
this. Constant-indexed two-element local arrays kept in registers colour
differently from separate scalars; `u8` snapshots compared after calls
follow their uses rather than their declaration order. `MenuItemDebugDraw`
(menusys) and `TitleModeKey` (title) are the references.

A meaningful later assignment to an index can change its register web,
including a strength-reduced loop web
([dngmenu](../ps2/re/docs/dngmenu/night-20261008.md)). Direct indexing and
reuse of a real pointer recover retail lifetimes; a named slot pointer can
keep an address alive earlier than the corresponding indexed CSE
([editloop](../ps2/re/docs/editloop/night-20261008.md)).

## Register allocation order

GPR simplify scans virtual registers in increasing number and pushes a node
when its current degree is below 25, lowering its neighbours' degrees at once;
passes repeat until no node is pushed. Then the remaining node with the lowest
cost divided by current degree is pushed (ties keep the latest-scanned node) and
simplify resumes. Colouring pops the stack and takes the first free register in
`v0 v1 a0–a3 t0–t7 t8 t9 s0–s7` order. Within one pass, a higher-numbered node
is therefore coloured first and receives the lower register. Scheduling
precedes allocation, so a statement order that schedules identically can still
change interference.

Observed numbering, highest first: single-web locals in reverse declaration
order, induction and invariant temporaries, extra live-range webs of reused
variables (per variable, in program order), load CSE temporaries (per source
branch, earlier branches higher) and materialized constants. Reusing a C-style
loop variable across disjoint loops is a natural way to move a counter below
the optimizer's temporaries. Indexing an array directly, rather than through a
named row pointer, makes the row address a low-numbered CSE temporary.
`CollisionFish`, `StepGyoRace` and `sgSysDrawGyoRace` match only with these
forms ([gyoracesim](../ps2/re/docs/gyoracesim/night-20261008.md),
[gyorace](../ps2/re/docs/gyorace/night-20261008.md)).

A value that retail keeps in a callee-saved register across calls but
colours after the named locals can be a CSE of a repeated expression rather
than a local: writing a register expression in full at each use lets MWCC
share its common left-associated prefix as a low-numbered temporary.
`mgEndFrame` (mglib) matches only with its three DISPLAY values written out
in full; a conditional expression for one of its locals (`a = c ? 4 : 3`)
also numbers differently from `if`/`else` ([mglib](../ps2/re/docs/mglib/night-20261008.md)).

Named locals take stack slots in declaration order, including block-scoped
ones, before argument temporaries. Temporaries such as `mgRect<int>(…)`
arguments are built right to left after them, so a call that retail builds
left to right in rising slots needs named locals.

## Instruction scheduling order

The pre-allocation list scheduler issues ready instructions cycle by cycle.
Critical nodes (latest start at or before the cycle) come first, then nodes
that unblock more successors, then greater height, then source order. Under
register pressure it instead picks the smallest pressure change: a store that
ends a value's live range beats a zero store, which beats a new constant.
Constants therefore issue in source order, and independent stores move with
the constants they consume. `CSound::Init` matches only when its configuration
assignments follow the same port order as its other field groups ([sound](../ps2/re/docs/sound/night-20261008.md)).

Assignment order inside a branch decides which values are still live when
the branch joins, which changes register allocation and lets the scheduler
move later loads. `CDngFreeMap::DrawRoot` (dngmenu) matches only with the
mark colour assigned before red in both branches. Satan's Fiddle
evaluate-first rows apply to call arguments, not to plain assignment
constants.

Equivalent integer comparisons such as `value > 2` and `value >= 3` can
change scratch-register allocation
([editloop](../ps2/re/docs/editloop/night-20261008.md)).

## Copy propagation and spilled pointers

The IR optimizer propagates a copy between two locals of the same type into
every later use of the destination, so `map = active;` makes `active` the one
long-lived value and removes `map`. Propagation stops when the source is
redefined later or when the types differ by a top-level `const`. A local
assigned from a call whose uses all come before the next call gets no virtual
register and reads the return register directly. In `if ((map = f()) == NULL)`
the test reads the return register, but later uses of a spilled `map` reload
it. Retail code that tests and uses `v0` while storing a spill slot therefore
comes from a separate `const` lookup local that is copied into the persistent
pointer (`SearchMapFlatPosition`, [dng_event](../ps2/re/docs/dng_event/night-20261008.md)). A
`T *const p = array;` copy keeps an array's address in a base register, so
`p[0]` and an indexed loop share it, where direct indexing folds `[0]` into a
symbol load (`mgEndFrame`). `u_int x = load; x &= mask;` makes the AND result
share the load's register; `x = load & mask` lets the mask share it. A
`sceVu0FVECTOR` parameter keeps its 16-byte alignment, so its spill slot takes
16 bytes of the frame.

After spills, a reloaded value's derived temporary receives a high backend
virtual-register number; a declared block-local value retains an early, low
number. This can change colouring after an otherwise matching first allocation
([mg_tanime](../ps2/re/docs/mg_tanime/nmmisc-20261008.md)).

## Data extents and alignment

MWCC gives native data objects their own extents and alignment; retail symbols
exclude the gaps between objects. Keep natural definitions exactly sized and
retain their original compiler alignment as evidence. See the split, padding
and comparison rules in
[Data layout](../scripts/build/DATA_LAYOUT.md).

## Natural C++ definitions

MWCC generates constructor vtable writes and C++ symbol names from class
definitions. Keep member functions and constructors in C++ form so the compiler
emits those symbols. A local `divbyzerocheck` pragma needs demonstrated code
generation evidence because that option is enabled by the shared flags.

Retail compiler-derived copy assignments have processor-specific symbol binding
13, unlike user-written assignments with global binding. Their outline decision
depends on inline depth: scoped `inline_depth(0)` makes the implicit
`CMapLightingInfo` assignment appear at the retail address and matches its
callers, while default depth inlines it. The same depth outlines the implicit
`sceGsTex0` and `mgCVisualMDT` assignments, but changes their callers or nested
base/constructor calls; those units still need exact source and type work.
`dont_inline` does not outline the implicit TEX0 assignment in the tested
compiler. Do not hand-write these generated assignments or compensate with
function-specific compiler hooks.

Compare complete objects as well as individual functions: emitted inline
helpers, static initializers and data sizes can change the containing unit.
The PAL executable verifier checks the final linked layout afterward.
Relocation-masked word counts hide call-target changes: a constructor chain
one level deeper than the inline depth calls an emitted WEAK constructor
where retail calls its body. `MenuItemCharaDataLoadEndCheckAfter` (menuchr)
needs scoped `inline_depth(8)` for its local `CScene`, and a single-case
`switch` rather than an equivalent `if` for its early return. The `if` lets
MWCC fill the next loop's branch delay slot from the following call setup.
