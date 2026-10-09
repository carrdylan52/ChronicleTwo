# Placement-new conversion investigation (2026-10-08/09)

## Baseline and scope

The isolated `work/dc2-night-pntc` lane starts at `63f7a9e5`, with
`chronicletwo_dev:sf-63f7a9e`. The coordinator's clean baseline verifies
`SCES_511.90: OK`, 149/149 objects, and 6,749 matched / 113 guarded /
10 assembly-only / 0 fuzzy functions. The lane's worktree is
`/home/dylan/projects/chronicletwo-night-pntc`; the cutoff is October 9
at 06:15 America/New_York. Compiler experiments and proposed toolchain
changes stay on this branch and new `sf-63f7a9e-pnN` image tags.

No shared game header, reserved dungeon-main file, existing image tag,
remote Git ref, PR, issue, or external comment is part of this investigation.
Private drivers and raw receipts are kept in `.private/pntc/`.

## Established evidence carried forward

The full [placement-new study](../funcpoint/placement-new.md), the
[guarded sweep](../shared/guarded-sweep-20261008.md),
[MWCC notes](../../../../docs/MWCC.md), and
[accepted control-context proposal](selector-proposal-20261008.md) were
read before experiments. Earlier source-spelling and helper-mask probes
are not new hypotheses and will not be repeated as matching proposals.

For MWCC 3.0-011126, constructor inline classification distinguishes
statement-capable class 3 from expression-capable class 6. The classifier
has initialized inputs derived from retained statements. Early conversion
puts the allocator assignment inside the construction condition; late
IroLinearForm separates assignment from a condition loading the object.
Those paths explain the observed guard operands before register allocation.
The prior study establishes no affected uninitialized state read.

Class A here means the construction guard tests the allocator's `v0` result;
class B means it tests a saved copy. Copy scheduling is recorded separately,
including the schedule-off and argument-register-alias cases. Explicit
`operator new` plus initialization, trivial construction, and array-new
are not evidence for an inlined nontrivial scalar constructor.

## Questions and acceptance criteria

1. Does any retail scalar site use B with an inlined class-6 constructor
   that the current source reproduces exactly? Inventory every allocator
   call, including matched sites and explicit-allocation exclusions.
2. Does forcing early constructor conversion across a translation unit or
   the profile preserve every existing matched function? Measure constructor,
   template, and header-defined subsets separately.
3. Can prefix/PCH handling, inline options, header order, or another natural
   build setting explain that same result without a compiler hook?
4. If a policy is still necessary, what semantic identity and fail-closed
   cardinality are sufficient? A matching result alone is not evidence of a
   compiler state defect. Report any remaining justification gap explicitly.

Any implementation must pass the canonical wrapper, clean PAL verification,
149/149 complete objects, unchanged objects outside promoted units, and
refreshed native coverage. Function promotion also requires a natural-source
and resolved-relocation audit, and is performed by hand in per-unit commits.

## Results

The [complete census](placement-new-census-20261008.md) records all 216
scalar placement allocator calls. All 108 nontrivial inline-constructor
sites use retail shape A; 68 of 70 out-of-line sites use A and the remaining
two use B. Runtime classification is measured for 91 inline sites: 74
class 6 and 17 class 3. The other 17 are explicitly unobserved, not inferred
classifier readings. Matched class-6 A sites demonstrate that pointer lifetime
and register pressure can produce A without an early-conversion policy.

The [natural controls](placement-new-natural-controls-20261008.md) contain
44 successful funcpoint tests. Valid text-prefix and genuine PCH variants,
prefix compilation state, header order, ordinary inliner choices, debugging,
RTTI, and exceptions do not remove the guard difference. Canonical text/PCH,
header-order, and several option controls reproduce the entire native object.
Exceptions introduce tables absent from retail. No tested setting establishes
an uninitialized-state defect or a natural global explanation.

The [global experiments](placement-new-global-controls-20261008.md) compile
all 149 native draft units successfully under each policy. A direct class-6
conversion request after expression inlining gives 25 newly zero draft
functions and loses no previous zero score. A before-inline class-3 request
gives 24 but regresses an existing matched function; restricting that request
to template constructors gives two gains without losing an existing zero.
The union is 26 guarded candidates, each requiring source hygiene and complete
object validation before promotion. This is measured lowering behavior, not
proof that the retail compiler used an undocumented global option.

A clean after-inline global build with all source guards retained passes
`SCES_511.90: OK` and 149/149 resolved game objects. All 306 assembled objects
in the baseline tree, including non-game inputs, and the linked PAL image have
identical whole-file hashes. Of 149 source-only objdiff base objects, 147 have
identical hashes; two differ only in compiler-generated local symbol strings.
This distinction matters when comparing source-only diagnostics with accepted
assembled artifacts.

The [constructor/header subset controls](placement-new-global-subsets-20261008.md)
complete the 149-unit comparison. Converting every class-6 constructor inline
read, including ordinary stack/member construction, loses 13 existing zero
scores. The observed-header before-inline subset loses one and gives 23 guarded
zeros. The header after-inline subset loses none and gives 24, missing the
cpp-defined `CMapParts` allocation in `EditSetPlaceAnime`. Both header controls
select 21 exact observed header identities in 67 trace events; those event
counts are not unique construction counts. None explains the template caller
that needs the other timing.

The scoped implementation compiles all 17 candidate units on `sf-63f7a9e-pn12`.
Its profile asserts 26 caller/constructor rows and 37 eligible constructions;
all 26 callers reach diagnostic zero, while no other scored function changes
in those units. The exact semantic scope is narrower than the global probes:
root call-node identity, actual initialized class read, exact normal mangled
witnesses, verified single-construction regions, and observed ordinary lowering.
Natural-source cleanup and promotion acceptance remain separate checks.

After-inline global conversion is empirically safe for the established zero
scores in this corpus. The proposal deliberately limits activation to callers
being promoted, adds fail-closed semantic scope/count checks absent from the
global driver, and uses before-inline timing only where separately measured.
This is a maintainer proposal for intentional lowering policy. No tested build
setting or memory-state defect establishes that retail used such a policy.

## Stronger global alternative: request4-all

`request4-all` is the stronger alternative explanation for the widespread
retail pattern: one profile-level after-inline conversion policy accounts for
25 guarded diagnostic zeros without losing any established zero. It deserves
maintainer consideration alongside the implemented caller-scoped proposal;
the investigation does not reject it in favor of per-caller rows.

The pn4 private driver captures the direct scalar allocator and constructor,
records the original inline class, performs ordinary expression inlining,
then requests the compiler's normal enclosing statement conversion for a
class-6 construction. It leaves the classifier result and retained constructor
metadata unchanged. Its allocator-name filter accepts `__nw__FUiP1` and the
provisional `__nw` alias. This is a measured global experiment, not a supported
global switch in the production patch.

The historical driver also requires the captured constructor name to start
with `__ct__`. This excludes a raw, unmangled `__ct` even when its measured
class is 6. The actual `request4-all/menushop.log` records the implicit
`CMenuQuestView` root as `ctor=__ct class=6 selected=false`; its initializer
remains 46 words in that corpus. This is the measured scope of the 25-gain
alternative, not evidence that every implicit constructor was converted.
The exact predicate is preserved in `.private/pntc/tools/make_probe4.py` and
`.private/pntc/receipts/satansfiddle-placement-probe-pn11.patch`. A new
[quest-view observation](../menushop/placement-new-quest-view-natural-20261009.md)
confirms that this implicit constructor still has no normal link-name witness;
the scoped production capability rejects such a caller rather than silently
skipping an unresolved eligible root.

| Measurement against the canonical all-drafts corpus | request4-all |
| --- | ---: |
| Successful native translation units | 149 / 149 |
| Common scored retail function identities | 6,773 |
| Canonical score-zero functions | 6,654 |
| Previous score-zero functions losing zero | 0 |
| Newly zero guarded functions | 25 |
| Changed scored functions | 44, all guarded |
| Changed whole native objects | 25 / 149 |

These counts require zero masked words, equal relocation offset/type sets,
and a body within its retail extent. They do not compare resolved relocation
targets or establish admissible source. Two changed native objects,
`editmenu` and `monster`, change only compiler-generated local symbol strings;
whole-file counts include that metadata. The full tables, exact changed rows,
object hashes and trace evidence are in
[global controls](placement-new-global-controls-20261008.md) and
[the extended comparison](placement-new-global-subsets-20261008.md), backed by
`.private/pntc/natural/global-comparison.json`,
`extended-global-comparison.json`, and the complete
`.private/pntc/experiments/request4-all/` corpus. The corresponding compile
receipt is `.private/pntc/receipts/corpus-request4-all.log`.

The clean global build with the original guards retained also passes PAL
verification and 149/149 resolved game objects. All 306 recursive assembled
objects and the linked ELF are byte-identical to the accepted baseline.
That validates preservation of existing native code; guards still supply
retail assembly for the newly zero drafts. It is not acceptance of 25 new
native functions. The source-only comparison has 147 exact object hashes
and the two metadata-only differences above. Receipts are
`global-request-clean-build.log`, `global-request-objects.log`, and the
timestamped artifact census in `global-comparison.json`.

The global result is stronger than overriding every constructor inline class,
which loses 13 existing zeros, or applying the earlier broad before-inline
new-expression policy, which loses one. The observed-header after-inline
subset preserves existing zeros but yields 24 gains, missing the cpp-defined
`CMapParts` construction. Thus header location is not necessary for the useful
global after-inline behavior. Conversely, `request4-all` leaves
`NewTexAnimeData` at its six-word baseline; the separately measured
before-inline template policy supplies that additional candidate.

A production global capability would still need a precise direct-call/allocator
boundary, handling for implicit constructors, bounded enclosing-expression
scope, and genuine failure-path tests. The private driver compares captured
constructor names rather than exact root call-node identity and does not
provide the scoped patch's exact live witnesses or completion assertions.
The existing global evidence supports evaluating such a profile-level option,
but does not establish that the original source or compiler enabled it.
The current committed capability supplies a narrower, validated artifact
while that design choice remains explicit for the maintainer.

## Proposed semantic boundary

The proposal is an intentional request for MWCC's normal early conversion of
an enclosing expression containing a scalar placement construction. It does
not alter emitted MIPS, stored constructor inline metadata, allocator results,
or arbitrary optimizer operands. The supported compiler is identified by its
full executable hash and exact hook opcode signatures.

Each row requires the logical translation-unit name, exact mangled caller,
scalar allocator signature, direct constructor identity, explicit conversion
timing, and a positive expected count of distinct eligible compiler constructions.
Eligibility requires the measured expression-inline class 6, uniformly before
raw or cached-name filtering. Class 0 and class 3 sites are outside the capability;
a row with no eligible sites fails its count. This prevents an uncached generic
`__ct` of an unrelated non-inline or implicit constructor from being treated as
a selected class-6 candidate.
The constructor projects the allocated type from the compiler's direct
scalar-construction child. No address, construction ordinal, or match count
selects a site. Count is a completion assertion; two otherwise identical
constructions receive the same policy.

Frontend names can still be raw at the conversion decision. A raw stem is
only a provisional filter. Every changed construction must obtain exact live
caller, allocator, and direct constructor witnesses from normal cached link
names or the compiler's ordinary name-mangler return. A wrong provisional
overload, multiple possible rows, or a missing exact witness rejects the entire
compilation before publishing its temporary output. Alias traversal is bounded
and follows the measured kind-6 function-object representation.

Conversion affects the enclosing expression, including its other evaluation
and scheduling decisions. To bound construction effects, the initial support
requires a single scalar construction in that region. It checks both the
initial walk and the final AST after expression inlining, then observes actual
ordinary construction lowering. A helper introducing an unvisited sibling or
nested construction is therefore rejected. Both timings audit retained inline
callees for deferred constructions; before-inline timing also audits the direct
constructor body before changing its current inline request. Transitive latent
constructions, unsupported indirect calls, body forms, and cleanup metadata reject
conservatively. An empty void return is accepted only as a metadata-free latent
body record with the canonical void return type. The audit treats its expression
graph as empty. The ordinary statement-return copier has a verified NULL path.
The ordinary expression inliner also tests a kind-8 return's expression for
NULL before invoking its generic copier; the generic copier itself has no
NULL guard. The retained empty-return regression exercises these caller checks.

Internal pointer bookkeeping has compiler-arena epochs. A measured AST arena
reset requires all active conversions and exact identity witnesses to have
completed before its pointers can be reused. Compiler teardown provides the
same live completion boundary. Completion after process exit reads only saved
host-side witnesses. The existing transactional object publisher preserves a
pre-existing destination on any hook, identity, scope, or count failure.

These restrictions are availability limits: a valid source may be rejected
if exact names arrive too late, raw overloads collide, or its enclosing expression
contains another construction. They are not evidence of a source defect.
The original study's recommendation against claiming an unsupported state
repair remains valid; this branch is a maintainer proposal authorized by the
isolated investigation brief.


## Isolated implementation and baseline validation

The proposed patch is `scripts/build/patches/satansfiddle-placement-new.patch`,
applied after the existing nested-argument and control-context patches in the
Dockerfile. The Python adapter filters placement rows by the original logical
translation-unit name, including mwccgap's temporary physical C inputs.

The production executable in `sf-63f7a9e-pn12` and `sf-63f7a9e-pn13` has the same
SHA-256, `a9da1a7739987952f838db26e420c40c17606125c3ab1ef53a44a29587471b26`.
pn13 adds genuine class-0 exclusion and mixed same-caller constructor fixtures;
all ten runnable 3.0 compiler integration tests pass, including the five placement
tests. The container's Cargo suite and four host adapter tests pass. The existing
dual-compiler integration test needs an unavailable 2.3.3 executable and is not
claimed as run. Placement hooks reject compiler versions other than the verified
3.0 executable (`0e16a5d6205101f840f85c02664f21cd63b39a0dec2dff417b3a61b4477f0e00`)
and validate exact instruction signatures before mutation.

With no placement rows and every original game guard retained, a clean pn12
build exits zero, verifies `SCES_511.90: OK`, and passes 149/149 complete game
objects. All 306 recursive assembled objects, all 149 source-only base objects,
and the linked PAL image have their exact baseline hashes. This stronger empty-
profile check establishes that adding the capability leaves the baseline intact.

Receipts are `.private/pntc/receipts/production-empty-clean-build.log`, its `.exit`,
`production-empty-objects.log`, `production-empty-artifact-comparison.json`,
`semantic-pn13-tests.log` and `.exit`, and `semantic12-score-comparison.json`.
Image build logs are `image-pnN.log`; the pinned original Git-fetch stage remains
cached in each build. Only this lane's new pn1 through pn13 tags were created;
none of the existing tags or remote Git refs was changed. Private global probes
are retained as receipts and are not part of the committed production patch.

## Retained-body and shared-ancestor regressions

The pn14 patch adds genuine compiler fixtures for retained cleanup metadata
and an empty void return, plus a pure AST test for a shared ancestor containing
a construction. No production policy logic changes in this test extension.
All 30 Rust unit/config tests pass during the image build. The genuine pinned
3.0 suite passes 12 tests; the existing dual-compiler test remains excluded
because this lane has no 2.3 executable. Receipt:
`.private/pntc/receipts/semantic-pn14-tests.log` and `.exit`.

The cleanup fixture has a class-6 selected constructor calling an inline
helper with a real automatic object and destructor. Observational LLDB reads
confirm nonzero cleanup metadata in its retained records. Both policy timings
reject specifically at that metadata audit, preserve the destination sentinel
and leave no temporary object. This tests the cleanup restriction directly,
rather than reaching a class-3 eligibility failure first.

The void fixture's class-6 constructor calls `notify()`, which retains a
kind-4 direct side-effect expression followed by a kind-8 NULL return,
metadata zero and canonical void result type 0x531F90. Both timings complete
one selected construction and publish an ELF object. Raw dispatch table
0x536F54 is indexed by record kind minus four and contains
`[0x4630A3, 0x4630D4, 0x4630D4, 0x4630D4, 0x463040]`.
Kind eight therefore takes the NULL-tested return path at 0x463040; earlier
private review notes reversed this mapping and are corrected. The production
audit remains consistent with the actual supported empty-return behavior.

The AST test shares a unary ancestor containing a kind-3A construction,
rather than sharing the construction node directly. It verifies propagation
of construction-bearing status to ancestors. A matching graph with a leaf
instead of the construction supplies the construction-free sharing control.
Evidence and exact raw-body observations are indexed in
`.private/pntc/selector-review/regression-notes.md`.

pn14's production wrapper SHA-256 is
`8f506132538b65e828c5677d21d218b898fe8206ee276075240ff1f653790f67`.
Its hash differs from pn13 despite this being a test-only source extension;
wrapper hash equality is not asserted. Real compiler behavior and complete
game artifacts provide the equivalence checks. The image's pinned Git-fetch
layer is cached; no Git network operation is executed.

## Accepted eighteen-caller source group

The local pn14 clean build accepts 18 manually promoted callers in 12 units
under 18 semantic rows, asserting 26 eligible static constructions. The group
contains 17 after-inline rows and the measured before-inline texture-list row.
Natural source cleanup is required independently of a diagnostic zero: the
placement-animation searches now use typed indexing, the cursor loader uses a
real signed file-size output with value-only unsigned shifts, and recovered
resource literals are compiler literals. Per-unit notes record complete bodies,
exact symbol sizes, binding, alignment and data/relocation evidence.

The clean image verifies `SCES_511.90: OK` and all 149 complete game objects.
Every assembled object outside the 12 promoted units is byte-identical to the
baseline in the full recursive 306-object census; the same holds outside those
units for all 149 source-only base objects. The linked main/game section and
loaded memory end retain their baseline values. The complete ELF differs only
in symbol/metadata representation. Restoring untouched source data markers to
their original sections changes fishing's assembled object metadata hash but
preserves the complete resolved-unit check and linked game bytes.

pn14 also passes all 12 genuine compiler CLI regressions, the 30 Rust unit/config
tests, and four host adapter tests. Its production binary SHA256 is
`8f506132538b65e828c5677d21d218b898fe8206ee276075240ff1f653790f67`; a test-source
change altered the binary build fingerprint, so equality with pn13 is not
asserted. Fresh context/objdiff and native coverage report 6,767 matched /
95 guarded / 10 assembly-only / 0 fuzzy. Receipts are
`.private/pntc/receipts/promote-eighteen-pn14-clean-*`,
`promote-eighteen-final-*`, `promote-eighteen-progress.log`,
`promote-eighteen-coverage.log`, `semantic-pn14-tests.log`, and
`adapter-pn14-tests.log`.

The inherited invention-menu allocation helper and event ESM identity helper
remain guarded. Their initial natural controls are nonzero and are not promoted
merely because inherited scaffolding had a diagnostic zero.

## Accepted twenty-four-caller source group

Six naturally cleaned menu callers add six semantic rows and seven static
constructions. The production profile now has 24 rows / 33 constructions,
all manually promoted, across 16 units. In particular, the costume loader's
float field receives 2.0f without an integer alias, the book has no
self-assignment steering, and recovered file/motion strings are compiler
literals. Array-bound and domain enums are used only for their actual owning
values; an unrelated same-valued enum is not used for the manual preset.

The full pn14 build passes `SCES_511.90: OK` and 149/149 resolved objects.
Every object outside these 16 units retains its exact baseline hash in the
306 assembled objects and 149 source-only objects. Main/game payload bytes
and loaded memory end remain baseline-identical. Fresh context/objdiff and
coverage give 6,773 matched / 89 guarded / 10 assembly-only / 0 fuzzy.
Receipts are `.private/pntc/receipts/promote-twenty-four-*`, including the
explicit refresh and coverage logs. Complete per-unit notes carry the exact
body-versus-extent and binding checks.

A separate aquarium caller becomes diagnostic zero with the existing
documented 47.0f floating-evaluation selector plus a semantic placement row.
Its natural local initializer cleanup is zero after moving real fish/think
declarations to their uses, but promotion still awaits complete acceptance.
The dataset investigation also exposes a fail-closed capability boundary:
implicit class-6 shadow constructors retain only raw `__ct` identities with
no normal cached mangled witness. Multirow raw ambiguity and single-row
unresolved witnesses are rejected; they are not bypassed or mislabeled as
class-3 exclusions. Frame attributes there are genuinely class 0/out-of-line.

## Accepted twenty-five-caller source group

`CAquarium::SettingAqua` is accepted with one class-6 after-inline row and
two existing-capability floating rows, each selecting one real 47.0f argument
by exact callee with expected count one. All resource names and four actual
array initializer templates are ordinary C++; the function uses plain int/float
arrays, typed buffer boundaries and real local lifetimes. Its exact native
body is 0xBB4 in extent 0xBC0, and all 123 other diagnostic rows remain stable.

The pn14 **clean** build now accepts 25 manually promoted callers across
17 units under 25 placement rows asserting 34 static constructions. All 149
resolved objects and PAL verification pass. The full recursive 306-object
census and 149 source-only objects retain baseline whole-file hashes outside
those units; linked main/game bytes and memory end agree. Explicit refreshed
coverage is 6,774 matched / 88 guarded / 10 assembly-only / 0 fuzzy. Receipts
are `.private/pntc/receipts/promote-twenty-five-clean-*`,
`promote-twenty-five-progress.log` and `promote-twenty-five-coverage.log`.

Further genuine identity-boundary evidence has a standalone no-include C++
fixture with ordinary virtual base constructors and two implicitly generated
shadow constructors. With no placement rows it compiles; each exact selected
after-inline row reaches the class-6 candidate gate, lacks a normal ctor link
name witness and fails specifically for unresolved identity. Both preserve
the original sentinel output exactly. This supplements the stronger numeric
game-root traces; it does not classify all implicit constructors. Integration
as an additional genuine regression is accepted on `sf-63f7a9e-pn15`:
all 13 runnable pinned-3.0 CLI regressions pass, including both implicit
class-6 roots. The image build also passes the 30 Rust unit/config tests.
pn15's production wrapper has the same SHA-256 as pn14,
`8f506132538b65e828c5677d21d218b898fe8206ee276075240ff1f653790f67`.
The extension changes only the genuine compiler test input and assertions.
Receipts are `.private/pntc/receipts/semantic-pn15-tests.log` and `.exit`,
`image-pn15.log` and `.exit`, and `image-pn15-production-sha256.log`.

The resumed pn15 clean build accepts the same 25-caller group: PAL verification
and 149/149 complete objects pass. The recursive 306 assembled objects and
149 source-only objects retain exact baseline hashes outside the same 17
promoted units. Linked main/game bytes and the loaded memory end remain equal.
Explicit context/objdiff refresh gives 6,774 matched / 88 guarded /
10 assembly-only / 0 fuzzy. Receipts are
`.private/pntc/receipts/resume-pn15-clean-build.log`, `resume-pn15-objects.log`,
`resume-pn15-artifacts.json`, and `resume-pn15-progress.log`, with explicit
exit files for the build, object check, artifact comparison, and refresh.

## Accepted twenty-seven- and twenty-nine-caller source groups

Natural `IsCreateObject` and `MenuItemSelectInit` add three static
constructions under two exact placement rows, together with their verified
floating argument policies. The 27-caller/37-construction group passes PAL,
all 149 complete objects and baseline whole-file hash checks outside eighteen
promoted units. Explicit refresh reports 6,776 matched / 86 guarded /
ten assembly-only / zero fuzzy. The complete object check catches and resolves
real initializer-storage requirements that zero instruction scores alone miss;
the affected unit notes retain those failures and the final storage evidence.

`MenuItemDebugKey` and `CMap::AddPartsGroup` then add two exact rows and
two static constructions. The **pn15 clean 29-caller group** now has 39
asserted constructions across nineteen promoted units. All 149 complete
objects pass, and the PAL verifier prints `SCES_511.90: OK`. All 306 recursive
assembled objects and 149 source-only objects outside those units retain
exact baseline hashes. Linked main bytes retain SHA-256
`a103b0461a88e443a3af684cf150c05b5bd355e5a97ab2dc029c4d872bed0811`;
the loaded memory end remains 0x01f64a00. Whole ELF metadata changes are
reported separately from those equal game bytes.

Explicit context/objdiff refresh reports 6,778 matched / 84 guarded /
ten assembly-only / zero fuzzy. DebugKey's final two `LOAD_FILE_READ` enum
arguments are independently whole-native-object identical to the clean zero;
the subsequent full game build, all-object check and unrelated-artifact
comparison pass again. Its ordinary unsigned value conversion and genuine
ridepod status subrange need no shared-header edit. AddPartsGroup uses its
real data constructor's clear and an ordinary sizeof-based allocation.

Receipts are `.private/pntc/receipts/promote-twenty-seven-*`,
`promote-twenty-nine-clean-build.log`, `promote-twenty-nine-{objects,artifacts,progress,coverage}`
and `promote-twenty-nine-final-{build,objects,artifacts}`, with explicit zero
statuses and artifact JSONs. Per-unit notes contain the exact body/extent,
binding, literal, vtable and relocation audits. Production wrapper semantics
and pn15's hash are unchanged from the previously tested image.

## Genuine constructor-source correction remains a separate alternative

The private `MenuCostumeInit` investigation reaches a natural zero by fixing
its actual constructor initialization: source-owned phase reset and character
lookup inside construction, real column-wise costume-list clearing, and
removal of extra inherited stores absent from retail. Its capacity argument
has a genuine value lifetime, and one existing camera argument policy restores
the remaining float order. No Costume placement row belongs to that zero.

The original constructor is observed as class 6. The corrected typed loop's
ordinary lowering reports class 3 in the corresponding diagnostic; the old
positive-one class-6 row fails with zero eligible matches. This supports the
capability's conservative eligibility boundary and demonstrates why a genuine
source correction must be considered independently of compiler timing. It
does not revise the initial-source census or establish a global build-state
defect. Exact source/header/profile proposals remain private because the
owning header is outside this lane; complete object/PAL acceptance is still
required. The evidence is in
[the costume constructor note](../menuchr/placement-new-costume-natural-20261009.md).

## Accepted thirty-one-caller source group

`CMenuInvent::LoadCharaCheck` and `CMap::CreateDrawRect` add two after-inline
rows and two static constructions. The complete production profile now has
31 manually promoted callers / 41 eligible constructions across nineteen
units. LoadCharaCheck uses a direct existing action-character placement
expression, removing an extra helper inline level, plus two semantic
SetPosition dependencies with asserted counts two and one. CreateDrawRect
walks the real typed placed-parts array and constructs the existing list node
with its genuine sizeof-based reservation. Neither requires a shared-header
change. The map unit has no guarded functions remaining.

The pn15 clean game build passes PAL verification and 149/149 resolved unit
checks. The complete inventory unit is 0xff1c bytes with 2,826 relocations;
map is 0x4728 bytes with 418 relocations. All 306 assembled objects and 149
source-only base objects outside the nineteen promoted units remain
byte-identical to baseline. Linked main bytes retain SHA-256
`a103b0461a88e443a3af684cf150c05b5bd355e5a97ab2dc029c4d872bed0811` and
loaded memory ends at 0x01f64a00. Whole ELF metadata may differ.
Explicit context/objdiff refresh followed by host coverage reports
6,780 matched / 82 guarded / ten assembly-only / zero fuzzy.

Receipts are `.private/pntc/receipts/promote-thirty-one-{clean-build,objects,artifacts,progress,coverage}`
with logs, explicit zero statuses and the artifact JSON. The owning
[loader](../inventmn/placement-new-loadchara-night-20261009.md) and
[visibility-list](../map/placement-new-drawrect-night-20261009.md) notes record
native symbol sizes, data and nonselected checks, actual policy counts and
complete acceptance. The smaller two-float loader policy and a three-row
control produce an identical complete native object. Production patch/image
semantics are unchanged.

The separate effect-script investigation confirms another conservative normal
identity limit: a selected caller also contains an implicit script constructor
with no ordinary mangled witness. Its character-scoped row fails closed before
producing an object. No constructor name or eligibility is fabricated to bypass
that rejection; see [the effect-script note](../effscript/placement-new-create-natural-20261009.md).
The party-change machine-zero control likewise stays guarded because its
transient cursor store lacks a supported source/default boundary; the legitimate
initializer alternative retains 26 words. These results do not enlarge the
accepted promotion count or alter the documented stronger global alternative.

## Accepted thirty-three-caller source group

Fresh typed `sgInitBuggy` and `CMapParts::AssignFuncAnime` add two witnessed
expected-one after-inline rows and two static scalar constructions. Both
start as assembly-only callers; each genuine source-only control differs
in just the allocation-result copy and null-guard pair, and each exact row
resolves that pair. The production profile now has 33 manually promoted
callers /43 eligible constructions across 21 units. Neither zero requires a new helper, handwritten generated special member,
dummy local, manual vtable or float selector.

Buggy uses the existing effect-manager/sprite constructor chain and actual
resource/scene APIs; the model frame is the qualified +0x70 base member, the
gun outline flags are 1, the message position is +0x158, and the image buffer
is a real serialized-file view. Animation binding consumes the manager's
actual Get result in its while condition and constructs/initializes the
real CList<CObjAnime> payload before appending it through typed links.
Actual function bodies are 0x994 and0x138; their 0x9a0/0x140 reservations
include independently checked twelve/eight zero padding bytes. Shared
header annotation corrections remain exact private proposals.

Independent source/row/witness/private-wrapper review finds no blocking
objection. The pn15 CLEAN build passes PAL verification and 149/149 resolved
unit checks. Pbuggy is 0x34b0 bytes with 834 relocations; mapparts is 0x2310
bytes with 264. All 306 assembled and 149 source-only objects outside the 21
promoted units remain byte-identical to upstream. Linked main retains
SHA-256 `a103b0461a88e443a3af684cf150c05b5bd355e5a97ab2dc029c4d872bed0811`
and memory end 0x01f64a00; whole ELF metadata is a separate comparison.
Explicit context/objdiff refresh followed by coverage reports 6,782 matched /
82 guarded /8 assembly-only /0 fuzzy.

Complete receipts are `.private/pntc/receipts/promote-thirty-three-{clean-build,objects,artifacts,progress,coverage}`,
with logs/zero exits and artifact JSON, plus
`.private/pntc/promote33/acceptance.json`. The independent review is frozen
in `.private/pntc/promotion-review-33/`. Owning details are in the
[Buggy initialization](../pbuggy/placement-new-init-natural-20261009.md) and
[animation-list](../mapparts/placement-new-assign-natural-20261009.md) notes.
Production image/patch semantics remain pn15; the 13 genuine compiler and 30
Rust configuration tests already pass, and pn15's production wrapper remains
byte-identical to pn14. The stronger global request4-all alternative above
still records 25 guarded gains /0 matched regressions and its actual name
filter limitation; the new scoped acceptance does not revise that result.
