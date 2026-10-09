# mg_dataset residuals and scoped-selector boundary

Isolated investigation, 2026-10-08, PN14. No tracked source/header/profile
changes, image builds, selector extensions, or promotions. All measurements
use the mandated container wrapper, canonical `draft_check.CC_FLAGS`, C++,
`UNMATCHING` and `NONMATCHING`, canonical library includes, logical
`mg_dataset.cpp`, and verification enabled. `run.py` and each variant's
`scores.json` record exact commands, source/profile/object hashes and timings.

## Correct function identities

The initially abbreviated member identities are absent from the manifest.
These are the actual guarded retail functions:

| Identity | Address | Native body / retail extent | Fresh PN14 words |
|---|---:|---:|---:|
| `CopyFrame__FP8mgCFrameP8mgCFrameP9mgCMemoryiPP8mgCFrame` | 0x133A80 | 0x33C / 0x340 | 1 |
| `End__13mgCMDTBuilderFP8mgCFrameP12mgCVisualMDTP10mgLoadData` | 0x134290 | 0x180 / 0x180 | 1 |
| `CreateFrameVisual__FP8mgCFrameP9mgCMemoryP9mgCMemoryP8mgCFrameP13MDTOBJ_HEADERP10MDT_HEADERiP17mgCTextureManagerPUiiPP8mgCFramePA4_A4_f` | 0x132D20 | 0x6E8 / 0x6F0 | 6 |

The fourth guarded diagnostic, `CopyFrameSub__FP8mgCFrameP9mgCMemoryiPP8mgCFrame`,
remains 46 words, body 0x108 / extent 0x110, relocation offset/type maps unequal.
The other three fresh control maps are equal. These metrics mask relocation
operands and compare maps only; they do not prove resolved targets, bindings,
or whole-object acceptance.

All mg_dataset notes were read first: `notes.md`, `matching-20261008.md`,
`placement-new-20261008-midday.md`. Relevant frame, visual, shadow, memory,
math, motion type notes, headers, MWCC and current selector documentation were
also read. Prior reference, combined construction/copy, direct field store,
whole BoundInfo assignment, pointer/propagation/lifetime, peephole, and vertex
argument-order negatives were not repeated. Mandated fresh m2c receipts are
`copy-m2c.*`, `end-m2c.*`, and `create-m2c.*`.

## Constructor eligibility is not uniform in this unit

| Site | Actual allocated type and definition | Retained evidence | Current boundary |
|---|---|---|---|
| CopyFrame +0x12C | `mgCFrameAttr`, `ps2/include/mg_frame.hpp:89`, ctor declaration :121 | Direct out-of-line `__ct__12mgCFrameAttrFv`; archived root inline class 0 | Ineligible for class-6 selector. Fresh exact row expects 1, matches 0, fails. |
| End +0x138 | same `mgCFrameAttr` | Direct out-of-line ctor; archived root class 0 | Ineligible; one-word branch remains. |
| CreateFrameVisual +0x174 | same `mgCFrameAttr` | Direct out-of-line ctor; archived root class 0 | Ineligible. |
| CreateFrameVisual +0x300 | `mgCVisualMDT`, `ps2/include/mg_visual.hpp:143`, inline ctor :163 | Root and direct-inline class 6 with normal exact ctor name | Eligible class, but the caller's raw implicit roots prevent unique/witnessed selection. |
| CreateFrameVisual +0x374 | `mgCVisualFixMDT`, `ps2/include/mg_visual.hpp:382`, inline ctor :390 | Root and direct-inline class 6 with normal exact ctor name | Same identity boundary. |
| CreateFrameVisual +0x408 | `mgCVisualMotionMDT`, `ps2/include/visualmotion.hpp:65` | Root and direct-inline class 6 with normal exact ctor name | Same identity boundary. |
| CreateFrameVisual +0x4BC | `mgCShadowMDT`, `ps2/include/mg_shadow.hpp:27` | Implicit default ctor root raw `__ct`, class 6 | Class eligible, normal mangled ctor witness unavailable. |
| CreateFrameVisual +0x53C | `mgCShadowFixMDT`, `ps2/include/mg_shadow.hpp:93` | Implicit default ctor root raw `__ct`, class 6 | Class eligible, normal mangled ctor witness unavailable. |
| CopyFrame +0x27C; End +0xD4 | `mgCFrame::BoundInfo`, `ps2/include/mg_frame.hpp:357`, size 0xB0 | Explicit placement operator call, no nontrivial ctor | Genuine POD/storage exclusion; never infer class 3/6. |

CreateFrameVisual also obtains BoundInfo storage directly from Alloc; that
site has no placement allocator call or nontrivial constructor. Pool buffer
casts and MDT serialized-file offset traversal are real storage/file format
operations, distinct from type-punning named object fields.

`mgCShadowMDT` derives from `mgCVisualMDT`. It declares five virtual rendering
and loading overrides and no constructor. Its asserted size is 0x50 (:85).
`mgCShadowFixMDT` derives from **mgCShadowMDT**, not mgCVisualFixMDT; its body is
empty and its asserted size is also 0x50 (:97). Both therefore use compiler
generated default constructors. Their actual base constructor is the inline
`mgCVisualMDT() { Initialize(); }`, with the inline mgCVisual base ctor in
`ps2/include/mg_dataset.hpp:214`. No generated special-member definition was
added to any header or source.

The complete old request4 trace is copied locally. Its lines 3-11 record the
three named class-6 direct roots; lines 12-15 record both raw `__ct` root
constructors as **class 6**, `selected=false`. Source switch order and retail
construction sites identify the two shadow types; the raw trace alone does
not provide their normal link names. Later expanded base ctor reads are
`direct=false` and are not substitutes for a root constructor witness.

Fresh PN14 `selected-after-control` and `selected-before-control` include the
three exact named constructor rows, expected one each. Both fail safely:
`placement raw identities cannot uniquely select a conversion row`.
Each single exact row (`after-mdt-only`, `after-fix-only`, `after-motion-only`)
also fails safely: `selected placement construction has unresolved mangled
identities`. Each log contains its exact named root plus two cached=false raw
`__ct` candidates. Production's uniform class-6 test occurs before this log;
the implicit roots are thus not class-0/3 exclusions. Their provisional raw
names alias any selected default constructor, and final normal mangled
witnesses remain unavailable. No ordinal, address, predicted-name bypass,
source helper, or capability broadening was attempted.

Consequently there is no genuine accepted selected control for this caller
under PN14. Historical request4/headers-after results of three words, and
before results of 107 words, are global diagnostics only. They are not
current scoped-selector or promotion evidence. Historical CopyFrame and End
both remain one word. `historical-scores.json` retains the seven mode results.

## Bounded natural source trials

| Variant | CopyFrame | End | CreateFrameVisual | Meaning |
|---|---:|---:|---:|---|
| canonical-control | 1 | 1 | 6 | Unchanged genuine private guarded control |
| typed-pod-allocs | 1 | 1 | 6 | Natural POD placement expressions, unnecessary bound cast removed, existing motion-visual enum |
| copy-block-memcpy | 47 | 1 | 6 | Real complete POD block copy; maps differ, CopyFrame body 0x2C4 |
| copy-named-array-loops | 54 | 1 | 6 | Natural named corner/max/min/center/radius loops; body 0x358 exceeds extent |
| copy-piece-memcpy | 47 | 1 | 6 | Separate legal array-sized copies plus named radius; maps differ, body 0x308 |
| create-typed-string | 1 | 1 | 6 | Inline literal and ordinary signed-char value conversion; body remains 0x6E8 |
| create-one-clear | 1 | 1 | 66 | Removes one actual retail clear; maps differ, body 0x6D4 |

All 41 native manifest rows were measured for every successful source trial.
All 37 unrelated rows and CopyFrameSub are exactly unchanged in these
diagnostic metrics. `nonselected-comparison.json` stores the complete
comparison and explicitly marks the absence of an accepted selected control.
Each directory contains the source, exact source diff, profile, compiler log,
object, disassembly, function diffs, and score receipt.

CopyFrame's inherited BoundCorners/mgVec4 casts reinterpret float arrays as
different aggregate objects. The center cast also spans the separate radius
field. These are real hygiene blockers; the score-neutral typed-POD variant
still contains them and is not a clean promotion candidate. The three new
copy alternatives remove every such cast; none reaches zero. Retail copies
the first 0x80 bytes with paired word loads/stores in an eight-byte loop,
then copies max, min, and center/radius with grouped four-component float
loads and stores. Natural C array loops and actual memcpy calls do not
reproduce that schedule with the current unchanged type definition.

End's explicit POD operator call becomes a natural placement-new expression
without changing its diagnostic score. Its remaining byte traversal is MDT
file-format addressing through `vertex_ofs`, not arithmetic on named game
object fields. Its out-of-line attribute constructor guard remains the
single mismatch at +0x144 (`a0` versus retail `v0`).

CreateFrameVisual's `at_550` bytes at 0x366D28 decode `mgLoadMDSFile\0` with
padding. The exclusive declaration and INCLUDE_RODATA are removed in the
private literal trial. The former signed-byte pointer reinterpretation
becomes `(s8)*name`, an ordinary value conversion. Existing meaningful
visual type enums are already used; CopyFrame's Iam literal becomes the
documented `MG_VISUAL_KIND_MOTION_MDT`. No new enum or helper was invented.

The two `memset(&motion, 0, 0x14)` calls are both present in retail and m2c
(m2c lines 165-166; assembly +0x5F8 and +0x60C). `mgCVMotionData` currently has
five named/unknown members and no constructor (`visualmotion.hpp:48`, size
0x14). A missing original inline constructor is a possible explanation,
not established type evidence. Deleting the redundant clear worsens the
score; inventing a ctor to hide the duplicate would violate this task.
This source/type question remains open before any clean promotion.

## Disposition

No zero. Keep all guards and add no mg_dataset production selector rows.
No whole promoted-object validation was run, because there is no clean zero
or accepted selected control to validate. Natural score-neutral cleanups are
private evidence only. The class-0 attributes and class-6 implicit-constructor
identity boundary are separate limitations, and neither authorizes expanding
the selector. Proposed negative fixture receipts will be kept in a separate
private subdirectory below this investigation.

Complete raw receipts and the real-header implicit-constructor sentinel fixture
are retained in `.private/pntc/mg-dataset-natural/`. This note records a
capability boundary of the [proposal](../satansfiddle/placement-new-proposal-20261009.md),
not an accepted source change or a class-3 exclusion.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](../satansfiddle/placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
