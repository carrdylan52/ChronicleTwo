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

Investigation in progress. No new SF policy or promotion is established by
this initial record.
