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

## Source matching and verification

Read unit documentation first and analyze retail with `./decompile.sh SYMBOL`
using m2c. Establish dependency types and layouts before changing expressions.
An early `mtc1`, changed saved register or changed stack frame can be a compiler
state difference; test the deterministic profile before changing source to
imitate incidental allocation. Use correctly typed literals and real field
layouts rather than pointer arithmetic or instruction-shaped source.

Declared locals are coloured in declaration order. Compiler temporaries,
cached loads and later assignments of a reused variable are coloured after
them, so retail's last-coloured value is often a reused variable or a
common subexpression rather than a new local. Named class locals take frame
slots in declaration order before call-argument temporaries.
`x = x < 0.0f ? -x : x` and `if (x < 0.0f) x = -x;` schedule the
surrounding loads differently. A no-op cast such as `((int)left)` on an
`int` makes that variable coloured after the temporaries. A repeated
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

## Register colouring of loop and snapshot locals

A loop's own index, or the first use of a variable, is coloured before the
loop optimizer's offset values; a later use of a variable first used
elsewhere (for example an unbraced `case` index reused by a later case) is
coloured after them. Declaration position at function scope does not change
this. Constant-indexed two-element local arrays kept in registers colour
differently from separate scalars; `u8` snapshots compared after calls
follow their uses rather than their declaration order. `MenuItemDebugDraw`
(menusys) and `TitleModeKey` (title) are the references.

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
forms (see the gyoracesim and gyorace night notes).

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
assignments follow the same port order as its other field groups (see the
sound night notes).

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
pointer (`SearchMapFlatPosition`, see the dng_event night notes). A
`sceVu0FVECTOR` parameter keeps its 16-byte alignment, so its spill slot takes
16 bytes of the frame.

## Data extents and alignment

Retail symbol sizes describe objects, while the split section pieces include
the alignment gap before the next symbol or referenced address. Once data
sections are assigned alignment one for linking, their bytes must retain that
gap. The postprocessor extends a correctly sized native initialized object by
fewer than 16 bytes to its piece boundary; initialized padding must be zero
in retail. A native NOBITS object with its exact declared size owns its full
reservation through the next canonical piece boundary, including larger gaps.
Referenced interior addresses stop that reservation, and terminal tails remain
linker-owned.
An initialized object's original payload and symbol extent must both match its
known declared retail size before naming or padding. Appended bytes must be
complete zero retail bytes with no relocation fields. The linked literal pass
can retain a larger verified terminal zero tail; comparison preparation trims
it at the linker's `contents_end`. Internal initialized gaps stay below 16 bytes.
An object with a size different from its declared retail size is not padded.
The same policy covers compiler-generated vtables; their final section tail
belongs to linker alignment.
A terminal datum retains its declared extent when its end equals the generated
linker script’s `contents_end`. The checker accepts larger linker-owned tails
only with no retail relocations and complete zero initialized bytes.
Referenced interior addresses and explicit
`D_<address>` source identifiers remain separate piece boundaries.

Native BSS templates, local statics and their guards need an exact declared
extent and agreement from every live incoming code reference. Each reference
must match the retail relocation kind and instruction operands outside the
immediate. HI16/LO16 pairs follow ELF relocation order, which can differ from
instruction order; orphan pairs, unknown consumers, out-of-object addends and
competing live definitions reject naming. Zero contents and compiler counters
alone establish no identity. Named local pointer tables can establish literal
identities when their exact declared extent, all code consumers, nonpointer
bytes, real relocation shape and native target bytes agree with retail. Their
validated identities are available before names are written in native symbol
order. Ambiguous initialized literals can then be named through real
R_MIPS_32 pointers in native data, subtracting the compiled addend and
target-symbol offset; conflicting references reject the binding. Anonymous
initialized templates retain the existing literal matcher and naming order.
Named initialized local tables without pointers additionally require the same
source base name, exact declared size and section kind, and exact retail bytes.
Every live consumer must be a complete retail function with the declared extent,
all instruction operands and all resolved relocation targets matching. Unknown
consumers, changed calls, duplicate relocation sites and orphan HI16/LO16 groups
reject naming. This pass changes only the data identity; existing bounded padding
supplies its alignment gap afterward.
Equal declared initialized extents distinguish a literal from a larger object's
byte prefix; established code or native-data destinations still reject competing
identities. Discarding a fallback parent removes a compiler-owned child only when
that child's retail storage also has a retained placeholder. Native children
remain available for naming and comparison.

VU instructions and initialized game or library words that resemble addresses
remain numeric when retail has no relocation. The splitter checks their emitted byte comments against retail
before replacing an inferred expression; real relocations remain intact.

A terminal function may end before the next unit's address when the generated
linker script supplies the intervening alignment. The canonical checker permits
this only at the exact `contents_end` established by the script and only for an
all-zero retail tail. Objdiff target symbol metadata records declared retail
function sizes so the same linker padding is excluded from function scores.

Objdiff uses separate comparison copies of the raw source-only and reference
objects. Data references come from retail relocation metadata, never splat's
address guesses; switch-table pointers use their enclosing function and interior
addend. Native anonymous names are established by bytes and real references,
not compiler numbering. Native pieces retain verified internal padding and both
sides exclude terminal zero tails owned by the linker. BSS symbol extents include
that verified piece padding consistently with initialized objects.

Reservation arrays and every retained data-marker piece are excluded from the
source comparison, including coincidental compiler copies. No fallback payload
is imported. Function bytes, declared sizes and relocation fields remain intact;
function names use the existing template/initializer projection. The build and
GUI refresh these copies when inputs or preparation tools change. `matched_data`
credits complete aggregate sections whose native bytes, extents and relocations
match retail exactly. It is a lower bound on migrated native data: an exact
typed object receives no section credit while a reservation or unmapped piece
leaves the same aggregate section incomplete. Removing the reservation preserves
the native object but cannot restore credit until the section is complete.
This metric is independent of executable matching.

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
