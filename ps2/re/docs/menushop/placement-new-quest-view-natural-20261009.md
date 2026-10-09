# NPC quest-view construction and the implicit-constructor identity limit

`MenuNPCQuestViewInit__FP9mgCMemoryPii` opens the NPC quest list or scoop memo,
reserves its local allocator from the caller stack, constructs the viewer and
quest manager, then binds save-data services and finishes viewer initialization.
This investigation retains both the guarded fallback and the production profile.
No new placement or floating-point row is activated.

## Retail body and established types

The retail ELF has a GLOBAL FUNC symbol at `0x299620`, size `0x124`, info `0x12`.
Its manifest extent is `0x130`; the following twelve bytes are zero padding.
The owning-header size currently describes that extent. The exact comment-only
correction is `.private/proposals/menushop-quest-view-body-size.patch`; it passes
`git apply --check`. The shared header remains unchanged, SHA-256
`7fbd76b95fc3a4b86b326a98ab27b8851beb3480f7ecd9d994df7d40be42ac07`.

Existing [type notes](notes.md) establish `CMenuQuestView` as `0x190` bytes,
with its `CBaseMenuClass` base occupying `0x110`, the base vtable pointer at
`0x10C`, selection and top indices at `0x110`/`0x114`, and thirty photograph
indices beginning at `0x118`. Its implicit constructor calls the real base
constructor and installs the derived vtable; it does not clear the photograph
array. `InitEnd` performs the later photograph initialization. No clear or
handwritten special member is added.

The [quest type notes](../quest/notes.md) establish the real eight-byte
`CQuestManager`, whose existing declared constructor calls `Initialize`.
The guarded draft instead spells an allocator call and a null-checked
`Initialize` call. Replacing that pair with an ordinary placement-new expression
uses the existing constructor without inventing an implementation or helper.
The viewer reservation is `sizeof(CMenuQuestView) / 16 + 2` (`0x1B` quadwords).
The smaller manager requires `(sizeof(CQuestManager) + 15) / 16 + 2` (three
quadwords); floor division would incorrectly reserve two.

The allocator capacity and top are real values obtained after `Align64` and
passed to `stSetBuffer`. The original source order preserves the capacity
read before the top read. Existing typed save-data getters supply `quest_data`
and the nested inventory scoop data. Mode zero selects quests; mode one selects
scoops. These are the established `QUEST_VIEW_MODE_QUEST` and
`QUEST_VIEW_MODE_SCOOP` values. The final texture-block call is direct; `InitEnd`
is a virtual call through the viewer's actual base vtable.

Fresh mandatory `decompile.sh` succeeds. Its allocator and nested-field guesses
are checked against the retail instructions and the established headers rather
than adopted as alternate layouts.

## Bounded natural C++ controls

The frozen input is production31 at `af88553f`, image
`chronicletwo_dev:sf-63f7a9e-pn15`. All six source-only controls retain all
31 selected placement callers. Word differences are masked at actual native
relocation offsets; relocation-map equality is checked separately.

| Control | Words | Native body | Relocation map equal |
| --- | ---: | ---: | --- |
| Canonical guarded draft | 46 | `0x128` | no |
| Real size/reservation expressions and mode enums | 46 | `0x128` | no |
| Existing genuine quest-manager constructor | 44 | `0x12C` | no |
| Immutable viewer pointer binding | 44 | `0x12C` | no |
| Immutable manager pointer binding | 44 | `0x12C` | no |
| Capacity local with top read directly in `stSetBuffer` | 44 | `0x12C` | no |

The canonical and hygiene objects are byte-identical, SHA-256
`85a9723232b542efbabd0b7bc42079a029c8aad5c433cb3e6abbc7b5c8e77b85`.
All four genuine-constructor/lifetime controls are byte-identical, SHA-256
`3e7379d98d8b3c177c42a776c38799c87cd94aef90a8d4d215734f4bea24f5bd`.
The viewer's first mismatch remains the branch/copy at `+0x70`; the native
implicit-constructor guard adds one instruction. The genuine quest constructor
exposes a second natural inline guard with another extra instruction. Those
changes shift the remaining suffix; these positional totals are not counts
of independent semantic errors.

All 29 nonselected owned scores, masked bodies, normalized relocation targets,
and symbol shapes stay unchanged in every control. Both emitted vtables retain
their bytes, symbol shapes and relocations. The genuine-constructor controls
renumber one drawing literal from `@1532` to `@1536` at relocation offsets
`0x898`/`0x8A0`; the literal size and bytes are identical. The audits distinguish
that naming change from a changed referenced value.

## Normal identity observation and fail-closed scoped controls

A read-only plain-wibo LLDB observer records actual construction roots and the
compiler's cached names and normal mangler returns. It neither calls the mangler
manually nor writes guest data. The pinned compiler SHA is
`0e16a5d6205101f840f85c02664f21cd63b39a0dec2dff417b3a61b4477f0e00`;
signatures at the construction and normal-return observation points are verified.
The observer completes normal compilation with exit zero.

The canonical source has one directly observed class-6 construction root.
Its allocator is normally named `__nw__FUiP1`, and the caller's normal return
supplies `MenuNPCQuestViewInit__FP9mgCMemoryPii`. The implicit viewer constructor
has raw name `__ct`, no cached link name, and no later observed normal mangler
return. The genuine quest-manager source has two directly observed class-6
roots. Its manager root has a complete normal identity,
`__ct__13CQuestManagerFv`; the viewer root remains unresolved. The raw observation
also retains revisits whose already-inlined constructor child is a compound
node; these are not counted as additional direct construction roots.

Exact before-inline and after-inline rows for the witnessed manager use positive
`expected_matches: 1`. Both fail with `selected placement construction has
unresolved mangled identities` because the same caller still contains the
unwitnessed implicit viewer root. Neither reports a successful conversion.
Each test starts with a real previous canonical object at its output path;
both failures preserve its complete SHA-256. No guessed viewer name, manual
mangler call, dummy address-taking use, constructor rewrite, or policy bypass
is attempted. This result documents the existing capability's conservative
boundary; it does not justify weakening that boundary.

## Receipts and disposition

Private root: `.private/pntc/menushop-questview-natural/`. `inputs.json` freezes
source/profile hashes, variants and head; each case contains its source, profile,
compiler log, native object, scores and target diff. `function-audit.json`,
`all-binding-audit.json` and `score-audit.json` record unchanged nonselected
functions and vtables. `retail-symbol.json` records body size and padding.
Normal observations are in `observe-normal-witness-{canonical,natural-quest-construction}/`.
`natural-quest-{before,after}/rejection-receipt.json` records failure and previous
output preservation. Fresh m2c: `.private/pntc/receipts/menushop-questview-m2c.log`.

The best admissible private draft is 44 words. The source and production profile
remain unchanged, and the initializer remains supplied by assembly. The global
request4-all alternative remains measured separately in the
[toolchain proposal](../satansfiddle/placement-new-proposal-20261009.md); this
study does not replace its recorded whole-build results with a scoped success.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](../satansfiddle/placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
