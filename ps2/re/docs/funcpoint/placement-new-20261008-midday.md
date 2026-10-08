# Placement-new lane — October 8 midday audit

The exclusive lane starts at `c79e57c`, the validated last-night integration
plus the mwccgap dependency fix. The local `origin/master` material at
`d8bf13c` is read without merging or rebasing. Measurements use MWCC
3.0-011126 with the canonical flags, existing source pragmas, and
`chronicletwo_dev:sf-d8bf13c`. All 29 outstanding functions in the thirteen
owned units are enumerated from fresh coverage and driven through
`decompile.sh`/m2c. Previously matched functions and recorded constructor
experiments are not reanalyzed.

The existing [constructor-classification report](placement-new.md#constructor-inline-classification)
remains the basis for the scalar-allocation park. Statement-inline class 3
can convert construction before the usual late expression lowering, while
the reconstructed small constructors retain expression-inline class 6.
The already validated menu constructors have genuine member arrays; no
corresponding original constructor array or conditional body is established
for the retained point/list, frame, character or collision constructors.
No artificial loop, destructor-bearing local, singleton array, identity
helper, hand-written vtable store or replacement assembly is introduced.

Upstream's generated map-lighting assignment and visual-assignment inlining
notes concern assignment binding/outlining. They do not establish a scalar
constructor conversion mechanism here. Ordinary generated frame-attribute
assignment does reproduce the old manual copy instructions, but leaves its
allocation branch unchanged. Nested float-argument selectors have no
validated application to these integer allocation-result null tests, and
no new compiler-profile row is added.

## Retained results and parks

Counts are differing 32-bit instruction words in the canonical native draft,
with relocation operands masked and identities checked separately. Zero
padding up to the retail extent is included. The oversize fishing function
uses the longer instruction stream for its diagnostic denominator.

| Unit / function | Baseline | Retained or admissible best | Remaining blocker |
|---|---:|---:|---|
| funcpoint `CFuncPointMngr::Add` | 2/40 | 2/40 | Null branch versus saved-pointer copy. |
| mg_dataset `CreateFrameVisual` | 10/444 | 6/444 | Six allocation-result branch aliases; bounds-query argument order is corrected. |
| mg_dataset `CopyFrame` | 1/208 | 1/208 | `a0` versus retail `v0` guard; generated attribute assignment reproduces the copy. |
| mg_dataset `CopyFrameSub` | 46/68 | 46/68 | Constructor result/copy lifetime shifts the remaining function. |
| mg_dataset builder `End(frame, visual, load)` | 1/96 | 1/96 | `a0` versus retail `v0` guard. |
| mg_tanime `TexAnime` | 660/1304 | 660/1304 | Four nop differences shift later words; inherited draft scaffolding also needs natural source recovery. |
| mg_tanime `NewTexAnimeData` | 6/32 | 6/32 | Success-only retained pointer and null-path/result join. |
| mdslist `CMapPiece::Copy` | 57/156 | 57/156 | Constructor guard/result copy adds a nop and shifts later words. |
| mdslist `CreateChara` | 2/76 | 2/76 | Null branch versus saved-pointer copy. |
| dynamicanime `dynCOLLISION` | 2/76 | 2/76 | Null branch versus saved-pointer copy. |
| water `CreateWaterFrame` | 82/104 | 82/104 | Frame-constructor guard/call shift and water-constructor result alias. |
| editmap `emapWATER_PARTS_NAME` | 2/72 | 2/72 | Null branch versus saved-pointer copy. |
| editmap `emapMASK_PARTS_NAME` | 2/96 | 2/96 | Null branch versus saved-pointer copy. |
| editmap `emapRIVER_PARTS_NAME` | 2/104 | 2/104 | Null branch versus saved-pointer copy. |
| event_func `_COPY_CHARA` | 2/176 | 2/176 | Null branch versus saved-pointer copy. |
| event_func `_ESM_INITIALIZE` | 2/76 with `Ident` | 11/76 without helper | Guard pair and saved-register lifetimes; inherited identity helper is inadmissible for promotion. |
| event_func `_COPY_MONS2SCNCHR` | 248/472 | 246/472 | Guard pair plus implicit `shadow_link` aggregate-copy code generation. |
| event_func `CObject(const CObject&)` | 0/52 | 0/52 | Natural emission depends on the still-guarded monster-copy caller. |
| sceneload `CScene::LoadChara` | 120/148 | 2/148 | Direct construction fixes base-constructor outlining; allocation branch pair remains. |
| sceneload `CScene::CopyChara` | 138/164 | 2/164 | Same direct-construction correction and remaining branch pair. |
| scenevillager `CScene::CharaObjectOnOff` | 6/112 | 6/112 | Hide-loop allocation; the identical show-loop constructor already matches. |
| effscript `BuildBase(int,...)` | 2/396 | 2/396 | Null branch versus saved-pointer copy. |
| effscript `CreateEffSpt(int,int,int)` | 206/320 | 206/320 | Script/member-constructor guard/call shift and subsequent character allocation. |
| effscript `AssignCharacter` | 2/108 | 2/108 | Null branch versus saved-pointer copy. |
| effscript `SetCharacter` | 24/168 | 24/168 | Allocation branches plus entry/character saved-register permutation. |
| editeff `EditSetPlaceAnime` | 2/156 | 2/156 | Null branch versus saved-pointer copy. |
| fishing `sgRestartFishing` | 2/344 | 2/344 | Null branch versus saved-pointer copy. |
| fishing `StepDataLoading` | 651/738 | 651/738 | 0xB88 body exceeds retail 0xB70; repeated construction adds instructions. |
| fishing `InitSuccess` | 25/280 | 25/280 | Player/fish saved-register permutation and allocation branch. |

The `TexAnime` sign-multiplication experiment gives 174/1304 positional
differences by replacing one retail negation with four instructions whose
length compensates for missing nops. It is rejected, as documented in the
unit note, and is not a retained improvement.

Unit evidence and negative probes are in the
[scene-loading note](../sceneload/placement-new-20261008.md),
[frame-copy/bounds note](../mg_dataset/placement-new-20261008-midday.md),
[event notes](../event_func/notes.md#remaining-allocation-checks-on-the-integrated-baseline),
[villager note](../scenevillager/placement-new-20261008.md),
[fishing note](../fishing/placement-new-20261008.md), and
[texture-animation note](../mg_tanime/placement-new-20261008-midday.md).

## Validation and handoff

No function is promoted, no header is changed, and no shared-file proposal is
produced. All retained runtime bodies continue to select their existing
assembly gaps. The normal build preserves all 149 game object-file SHA-256
hashes and retains 147/149 complete-object passes. The two failures remain
`nd_meswin::DrawMesWin` and `actscript::_SHOT`. PAL retains the baseline
0x26-byte `.text` difference; all other file sections and the 0x01F64A00 BSS
memory end pass. Coverage remains 6,686 matched / 169 guarded / 15 asm-only /
2 fuzzy, out of 6,872 game functions.

Important corrections for the next lane: monster copying is not currently
a two-word park; `Ident` is not a valid promotion mechanism; and the
compiler-generated object copy constructor cannot be independently emitted
by a dummy use. `CMap::CMap` in sceneload is already native and exact on this
baseline, despite the older unit note's constructor warning.

Private receipts are rooted at `.private/placenew-midday/`: `owned-queue.tsv`,
`m2c/`, `baseline-native/`, `probes/`, `baseline-build.log`,
`baseline-objects.log`, `baseline-object-hashes.json`, `final-build.log`,
`final-objects.log`, `final-objdiff-build.log`, `final-progress.log`, and
`final-hash-comparison.json`. Invocation files record each private source
hash, exact compiler arguments and compiler exit status. Diagnostic objdump
uses `-z` so runs of zero words do not disappear from the comparisons.
