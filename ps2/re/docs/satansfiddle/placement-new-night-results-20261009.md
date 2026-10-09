# Placement-new night results (2026-10-08/09)

The branch accepts **34 natural native callers in 21 units**. The final clean pn15 build passes PAL and **149/149 complete object checks**. Coverage is **6,783 matched /82 guarded /7 assembly-only /0 fuzzy**, from baseline 6,749 /113 /10 /0. Production has 34 placement rows, 44 eligible constructions and 81 floating-expression rows.

This checkpoint precedes the authorized **06:15 America/New_York cutoff on October 9**; work continues. The [chronological commit ledger](placement-new-night-commits-20261009.md) records each hash/subject. The [authoritative proposal](placement-new-proposal-20261009.md) owns full compiler reasoning, alternatives and staged acceptance.

## Accepted production callers

Every guard below is removed and its complete owning-unit bytes/resolved relocation targets and linked game bytes pass. Direct retail/accepted34 symbol inspection verifies all 34 addresses and actual body sizes. The binding column preserves the separate metadata facts: six retail-local symbols are global in accepted34, while other listed bindings agree. This does not assert full ELF identity against retail. Extents/padding remain separate in owning notes.

All rows use actual allocator `__nw__FUiP1`, exact caller/constructor identities and positive count assertions. Timing is after constructor inlining except the independently measured texture-animation before-inline case.

| Unit | Exact retail function | Address | Body | Retail→linked binding | Sites | Timing |
| --- | --- | ---: | ---: | --- | ---: | --- |
| dynamicanime | `dynCOLLISION__FP9SPI_STACKi` | `0x0017D0F0` | `0x130` | LOCAL | 1 | after |
| editeff | `EditSetPlaceAnime__FiP9CMapParts` | `0x00300C40` | `0x264` | GLOBAL | 1 | after |
| editexception | `InitFirePowder__FiP6CSceneiP9mgCMemory` | `0x002FCA30` | `0x358` | GLOBAL | 1 | after |
| editmap | `emapRIVER_PARTS_NAME__FP9SPI_STACKi` | `0x001B5800` | `0x1A0` | LOCAL→GLOBAL | 1 | after |
| editmap | `emapMASK_PARTS_NAME__FP9SPI_STACKi` | `0x001B59A0` | `0x174` | LOCAL→GLOBAL | 1 | after |
| editmap | `emapWATER_PARTS_NAME__FP9SPI_STACKi` | `0x001B5B20` | `0x11C` | LOCAL→GLOBAL | 1 | after |
| editmode | `LoadEditCursor__FP9mgCMemoryi` | `0x002DDB30` | `0x5B4` | GLOBAL | 3 | after |
| effscript | `BuildBase__16CEffectScriptManFiP1iP1iP9mgCMemoryi` | `0x002E5420` | `0x630` | GLOBAL | 1 | after |
| effscript | `AssignCharacter__16CEffectScriptManFP11_EFF_SCRIPTi` | `0x002E6FA0` | `0x1A8` | GLOBAL | 1 | after |
| event_func | `_COPY_CHARA__FP12RS_STACKDATAi` | `0x0026A900` | `0x2B4` | LOCAL→GLOBAL | 1 | after |
| fishing | `sgRestartFishing__FP11SubGameInfo` | `0x00301A00` | `0x55C` | GLOBAL | 1 | after |
| fishing | `StepDataLoading__FPv` | `0x003021C0` | `0xB70` | LOCAL→GLOBAL | 7 | after |
| funcpoint | `Add__14CFuncPointMngrFiP9mgCMemory` | `0x002A11C0` | `0xA0` | GLOBAL | 1 | after |
| inventmn | `LoadCharaCheck__11CMenuInventFv` | `0x002022A0` | `0x4D8` | GLOBAL | 1 | after |
| inventmn | `IsCreateObject__11CMenuInventFii` | `0x00203E90` | `0x1588` | GLOBAL | 2 | after |
| map | `AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory` | `0x0015DA50` | `0xFC` | GLOBAL | 1 | after |
| map | `CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi` | `0x0015E210` | `0x1B8` | GLOBAL | 1 | after |
| mapparts | `Copy__9CMapPartsFR9CMapPartsP9mgCMemory` | `0x00168EB0` | `0x6AC` | GLOBAL | 1 | after |
| mapparts | `AssignFuncAnime__9CMapPartsFP9mgCMemory` | `0x00169560` | `0x138` | GLOBAL | 1 | after |
| mdslist | `Copy__9CMapPieceFR9CMapPieceP9mgCMemory` | `0x00169C60` | `0x268` | GLOBAL | 1 | after |
| mdslist | `CreateChara__FPUiPcP9mgCMemory` | `0x0016ACD0` | `0x12C` | LOCAL→GLOBAL | 1 | after |
| menuaqua | `SettingAqua__9CAquariumFv` | `0x00215350` | `0xBB4` | GLOBAL | 1 | after |
| menuchr | `LoadBGNPCModel__15CMenuChrCngMenuFi` | `0x002B5240` | `0x1E0` | GLOBAL | 1 | after |
| menuchr | `LoadMenuData__15CMenuCostumeSelFP9mgCMemoryPi` | `0x002C0C60` | `0x390` | GLOBAL | 1 | after |
| menuchr | `KeyStep__12CMosBookMenuFv` | `0x002C3A60` | `0x54C` | GLOBAL | 1 | after |
| menudraw | `GeneratePoly__14CRepairManagerFPfi` | `0x0022FE30` | `0x240` | GLOBAL | 1 | after |
| menuop | `MenuManualInit__FP9mgCMemoryPii` | `0x002C4480` | `0x510` | GLOBAL | 1 | after |
| menusys | `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | `0x00245EC0` | `0x3B8` | GLOBAL | 2 | after |
| menusys | `MenuItemDebugKey__Fv` | `0x00247E10` | `0x16CC` | LOCAL | 1 | after |
| menusys | `MenuItemSelectInit__FP9mgCMemoryPii` | `0x002528E0` | `0x288` | GLOBAL | 1 | after |
| mg_tanime | `NewTexAnimeData__15mgCTextureAnimeFP9mgCMemory` | `0x0013DA40` | `0x7C` | GLOBAL | 1 | before |
| pbuggy | `sgInitBuggy__FP11SubGameInfo` | `0x00318B70` | `0x994` | GLOBAL | 1 | after |
| sceneload | `LoadChara__6CSceneFiPUiPcP9mgCMemoryP9mgCMemoryP9mgCMemoryii` | `0x00288F30` | `0x244` | GLOBAL | 1 | after |
| sceneload | `CopyChara__6CSceneFiiP9mgCMemory` | `0x002891B0` | `0x284` | GLOBAL | 1 | after |

The final source audit covers all 34 callers and all 21 affected units, with no executable hygiene blocker. Nine inherited declaration purpose-comment omissions were corrected in separate editeff, fishing, inventmn and menuchr commits. Their rebuild and final CLEAN rebuild preserve all 455 raw objects and the whole accepted34 ELF. The [source-audit note](placement-new-source-hygiene-20261009.md) owns the exact scope. Existing declarations and the original baseline already use GLOBAL for those six; read-only provenance checking records this inherited metadata difference without replaying any matching control.

## Validation and artifact identity

Production uses `chronicletwo_dev:sf-63f7a9e-pn15`. The final clean source snapshot is `2dee539d`; later commits are documentation only. All 149 resolved units pass. Every assembled/source-only object outside the 21 promoted units is raw-identical to tested upstream. All 455 raw objects and the whole linked ELF also equal the immutable accepted34 artifacts. Full ELF identity against upstream is not claimed.

| Artifact | Accepted value |
| --- | --- |
| Profile SHA-256 | `f6353a6cb5facbf60a0f48d2be2db95354573f58a4c85a0295b14ea78c3cafe0` |
| Accepted34 ELF SHA-256 | `6352338ce8ff3fef0df4d54215ab1ca9348ae4db098c1a35cb7da359ba17b82d` |
| Game/main SHA-256 | `a103b0461a88e443a3af684cf150c05b5bd355e5a97ab2dc029c4d872bed0811` |
| Main bytes / base / memory end | 2,608,512 / `0x00100000` / `0x01F64A00` |
| Object / PAL / refreshed coverage |149/149 / `SCES_511.90: OK` /6,783 /82 /7 /0 |

Final receipts are `.private/pntc/receipts/final-thirty-four-{clean-build,objects,artifacts,identity,progress,coverage}`, each with log and zero exit. `.private/pntc/promote34/{final-clean-acceptance,final-clean-identity}.json` pins the snapshot and direct comparisons. Context/objdiff refresh precedes progress and host coverage. Earlier per-promotion receipts remain in owning notes; raw whole-ELF metadata is kept distinct from loaded game bytes throughout.

## Census, global alternative and natural controls

The retail census covers 216 scalar placement allocator sites in 116 callers:178 nontrivial constructors, 21 explicit allocation/initialization sites and 17 trivial constructions. All 108 inline-constructor sites use retail allocator-result guard shape A. Of70 out-of-line constructors, 68 use A and two B. Measured runtime inline classes cover 91 sites:74 class 6 and 17 class 3; the other17 have no observed classifier value. Ordinary matched class6/A callers demonstrate that pointer lifetimes and register pressure can produce A; no compiler-state defect is established.

Historical **request4-all global after-inline conversion is the stronger alternative**:149/149 native units, 6,773 common scores, **25 newly zero guarded diagnostics and zero previous-zero regressions**. All 44 changed scores are guarded. Whole native objects change in 25 units;23 have actual payload differences and editmenu/monster change only generated local symbol strings. With guards retained, a clean build preserves all 306 raw assembled baseline inputs, full ELF/PAL and 149 resolved units;147/149 source-only hashes agree, with those two metadata exceptions. This is preservation and diagnostic-gain evidence, not acceptance of 25 new native functions.

Its historical filter accepts `__ct__` prefixes and skips raw `__ct`, including the actual implicit quest-view class 6 root. It uses provisional allocator/name matching without production's exact live identity, bounded-region or completion guarantees. Pure request4 leaves NewTexAnimeData six words different; before-template timing supplies the additional zero. Broad before-inline gives 24 gains/one prior-zero loss; before-template gives two/no loss; converting every constructor inline read loses 13; header-only after-inline gives 24/no loss but misses a cpp-defined constructor. See [global controls](placement-new-global-controls-20261008.md), [subset controls](placement-new-global-subsets-20261008.md) and the proposal.

Forty-four genuine natural funcpoint controls test text prefixes/PCH, prefix state, header order, ordinary inliner/deferred/depth choices, debug, RTTI and exceptions. None removes the guard difference. Several reproduce the complete native object; exceptions create tables absent in retail. No original retail global option or uninitialized-state repair is established. The [natural-control note](placement-new-natural-controls-20261008.md) owns exact variants.

The new **current34 hybrid migration** is still private. It reconstructs the exact historical driver, omits only the placement profile key, preserves 81 floating rows, uses after-inline in 148 units and before-template in mg_tanime. This mixes two policies; it is not one pure global switch. All 149 wrappers/postprocessors/fixups and resolved objects now pass. Full raw/selected/nonselected/data/metadata/trace auditing is ongoing; no current34 global link/PAL result is claimed here.

## Guarded attempts and blockers

These are the best documented admissible natural results in bounded investigations, except the explicitly historical last row. Every fallback stays active. Scores alone do not establish admissible source or complete object acceptance.

| Owning target | Best words | Remaining blocker / evidence |
| --- | ---: | --- |
| `_ESM_INITIALIZE` |2 | Base/sum copy coalescing at +0xE8/+0xF0; Ident-helper zero rejected. [note](../event_func/placement-new-residuals-20261009.md) |
| `MenuInventInit` |76 | Pack/texture registers and missing original buffer API; wrapper zero rejected. [note](../inventmn/placement-new-natural-night-20261009.md) |
| `InitSuccess` |24 | Fish/player s1/s2 exchange after valid allocation conversion. [note](../fishing/placement-new-residuals-20261009.md) |
| `CEffectScriptMan::SetCharacter` |23 | Typed slots:21 GPR/two addu words. Inherited byte addressing reaches21 but fails hygiene. [note](../effscript/placement-new-residuals-20261009.md) |
| `CreateWaterFrame` |1 | Out-of-line receiver uses v0 rather than equal saved s6. [note](../water/placement-new-residual-20261009.md) |
| `TitleBootInit` |19 | Map-top/icon/menu-argument lifetimes and relocation positions. [note](../title/placement-new-residual-20261009.md) |
| `MenuCharaChangeInit` |26 | Legitimate member-initializer schedule; extra cursor body-store zero rejected. [note](../menuchr/placement-new-change-natural-20261009.md) |
| `CScene::CharaObjectOnOff` |6 | Out-of-line class 0 attribute guard/copy schedule; no eligible row. [note](../scenevillager/placement-new-natural-night-20261009.md) |
| `CEffectScriptMan::CreateEffSpt(int,int,int)` |206 | Implicit class 6 script ctor lacks normal exact name; scoped row rejects. [note](../effscript/placement-new-create-natural-20261009.md) |
| `MenuNPCQuestViewInit` |44 | Implicit viewer identity unresolved; real manager ctor is insufficient. [note](../menushop/placement-new-quest-view-natural-20261009.md) |
| `_COPY_MONS2SCNCHR` |244 | Genuine by-value copy leaves shadow-link/GPR/FPR residuals. [note](../event_func/placement-new-copy-mons-night-20261009.md) |
| `CMenuInvent::IsAccessAlbum` |177 |154 GPR/13 stack/10 schedule words; no dummy0x10 frame storage. [note](../inventmn/placement-new-album-natural-20261009.md) |
| `CMenuItemInfo::IsAskExtend` |170 | Native0xA6C versus retail0xA68 and frame/control; older failed wrappers do not validate best. [note](../menusys/placement-new-ask-extend-natural-20261009.md) |
| `DngTreeMapInit` |55 | Class 0 constructors, filename/offset schedule, relocation positions. [note](../dngmenu/placement-new-tree-init-natural-20261009.md) |
| `MenuMainInit` |13 | Unowned real MenuKey rectangle initializer leaves schedule residual. [note](../menumain/placement-new-natural-night-20261009.md) |
| `CAquarium::Step` |1111 | Model/global reload/register residual; food ctor class 0 excluded. [note](../menuaqua/placement-new-step-natural-20261009.md) |
| `CMenuChrCngMenu::EnterDataMenu` |11 | Real three-byte palette walk leaves induction/scheduling; all targets exact. [note](../menuchr/placement-new-enter-data-natural-20261009.md) |
| `EditInit` |821 | Paired unowned chest ctor move; missing camera assignment and incoming uninitialized field. [note](../editloop/placement-new-init-natural-20261009.md) |
| `sgInitGyoRace` |421 | Real fish/slot induction and resource lifetimes; class 0 placement already matches. [note](../gyorace/placement-new-init-natural-20261009.md) |
| `CopyFrameSub` |46 | Class 0 frame construction and register/relocation residual. [note](../mg_dataset/placement-new-class0-copies-20261009.md) |
| `CopyFrame` / `mgCMDTBuilder::End` |1 /1 | a0 versus v0 branch operand; real local helper binding fix changes no payload. [note](../mg_dataset/placement-new-class0-copies-20261009.md) |
| `CreateFrameVisual` |historical6 /3 global | Incomplete unowned types/implicit ctor scope; manual-vptr specimen rejected. [note](../mg_dataset/placement-new-natural-night-20261009.md) |

New function analysis uses decompile.sh/m2c after existing owning/dependency notes. Recorded spelling and steering negatives are preserved, not replayed. Linked notes contain complete nonselected code/data/vtable/relocation audits and exact failure receipts.

## Unowned zero proposals and private impact

Three more genuine private-zero callers require shared headers and stay outside production34.

| Bundle | Native zero | Private acceptance / owner limit |
| --- | --- | --- |
| Costume authored-constructor correction | `MenuCostumeInit`,0x2D8 body /0x2E0 extent |Current34 all 149 wrappers/PAL pass;148 other raw objects agree. Class3 ctor needs no placement row; only one camera float row. [note](../menuchr/placement-new-costume-natural-20261009.md) |
| Generated MDT assignment / derived visual copies | `mgCVisualFixMDT::Copy`,0x190; `mgCVisualMotion::Copy`,0x210 |Remove manual assignment declaration/body; genuine native binding13 becomes ordinary WEAK2. All 149 accepted33→private35 wrappers/PAL pass;147 other objects agree. Fresh raw-identical MapParts bridge plus148 reused objects supplies current34→private36 PAL. [joint note](placement-new-visual-pair-impact-20261009.md) |

Root independently verifies 1,909 regular files in the Costume all 149 manifest and 1,932 in the visual four-stage aggregate, excluding passthrough symlink targets. Costume's raw ELF changes selected size/assembly marker, adds ten selected GP tuples and reorders12 existing positions; all 98,516 baseline named tuples survive and 98,482 nonselected records keep exact order. Visual links preserve all 98,516 ordered named tuples and record five symbol/owner metadata differences. All loaded bytes and memory extents remain exact.

Their new **two-header composition** passes one fresh menuchr wrapper, raw-identical to Costume-alone, and one 306-input actual MWLD/PAL link using the frozen visual private36 corpus. It preserves all 98,516 old named tuples and 98,482 ordered nonselected records, with the same ten selected GP additions/12 order positions. Measured symbol metadata differences are exactly seven names. Root verifies all 528 regular files and ten symlink identities without following them; the [composition note](placement-new-three-zero-composition-20261009.md) owns the frozen scope. Its profile has 36 placement rows/46 sites plus the sole Costume float row; it supports 37 hypothetical native callers, not 37 placement rows or production promotions.

Exact visual vehicle: `.private/proposals/visual-copies-generated-assignment-pair.patch` plus `visual-copies-row-delta.json`. Current34 Costume vehicle: `.private/pntc/costume-whole-impact/proposals/{header-current34,source-current34,profile-current34}.patch` and `profile-current34-delta.json`. Original `.private/proposals/menuchr-costume-*` patches remain provenance. Merge portable row deltas into current34; do not replace its profile with old35-row fixtures.

The [inventory](placement-new-proposal-inventory-20261009.md) preserves 73 originals: six zero-bundle files,two unowned nonzero controls, 35 primary comment files, 12 owned guarded diagnostics and 18 superseded/duplicates. Its deduplicated `.private/pntc/proposal-inventory/comment-only-actual-symbol-sizes.patch` changes42 actual-body @size lines across 25 unchanged headers.43 source records include one exact FBOX duplicate; root verifies all 42 against retail. The frozen MDT const-marker dependency-label typo has an erratum; exact selectors and patches are unaffected. No unowned header is active.

## Toolchain, tests, images and interruptions

The proposed `scripts/build/patches/satansfiddle-placement-new.patch` follows the accepted nested-argument/control-context patches in Dockerfile. The adapter uses genuine logical-unit identity. It requires measured class 6 scalar roots, exact ordinary caller/allocator/ctor witnesses, bounded regions and positive completion counts; ambiguous/unresolved identity, unsupported cleanup/graphs, signature changes and count failure reject transactionally. It never rewrites emitted MIPS or retained ctor metadata.

Production pn15 passes **13 genuine pinned MWCC3.0 CLI regressions**, including implicit-constructor rejection, cleanup/sentinel rejection and valid empty-return conversion. Separately, its image passes 30 Rust unit/config tests; four host adapter tests also pass. The unavailable2.3.3 dual-compiler test is not claimed. Frontend SHA `8f506132538b65e828c5677d21d218b898fe8206ee276075240ff1f653790f67` is equal pn14/pn15; compiler SHA is `0e16a5d6205101f840f85c02664f21cd63b39a0dec2dff417b3a61b4477f0e00`. Receipts: `.private/pntc/receipts/{semantic-pn15-tests,image-pn15,image-pn15-production-sha256}.*` and earlier adapter logs.

Only new own image tags `chronicletwo_dev:sf-63f7a9e-pn1` through`-pn15` were built for private probes/production; all original image-pnN.log receipts are preserved. New private pn16 adds the distinct historical driver while preserving canonical pn15 frontend. Its builder runs one available CLI-target test and ignores six compiler fixtures. The frozen source lacks exact example files, so full Rust/example tests are explicitly unrun; these counts do not replace pn15's genuine13/Rust30. Digest-store preflight and missing-example build failures remain preserved alongside the corrected private recipe.

The app restart cancelled the prior lane around19:17 EDT, not a coordinator stop. The unfinished pn15 test-only patch was reviewed, validated and committed on resumption; no partial source promotion was committed. Later pn16 mg_tanime completes both genuine passes before a login-PATH assembler failure; its authorized launcher-only `bash -c` retry succeeds, with the initial failure and repeated-pass counts preserved separately. No commit was rewritten. No reserved dungeon source/header, shared game header, foreign image tag or remote ref was changed. The expected dirty tools/mwccgap is unstaged. No push, PR, issue or external comment occurred.

## Remaining work before cutoff

Freeze and independently review the combined-header proof and current34 hybrid corpus, then record accurate outcomes. Keep unowned headers inactive, preserve negatives and stop new work at 06:15 EDT. The final checkpoint will state the stopping HEAD and update the complete commit ledger.
