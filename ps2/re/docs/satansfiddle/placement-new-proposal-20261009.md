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
graph as empty. The ordinary statement-return copier has a verified NULL path;
the generic expression-body copier does not.

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
