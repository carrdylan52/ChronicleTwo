# Guarded-function sweep — October 8, 2026

Baseline: `6be9e34`, branch `work/dc2-sweep-midday`. Image: `chronicletwo_dev:sf-d8bf13c`; MWCC 3.0-011126 with the repository flags. Coverage is **6,743 matched / 117 guarded / 10 assembly-only / 2 fuzzy**, out of 6,872 functions. Complete objects pass **147/149**; only `nd_meswin` and `actscript` fail. PAL differs by **0x26 `.text` bytes**, first at `0x0015C5AD`; all other file-backed sections and memory end `0x01F64A00` match.

`manifest.py` was refreshed and `scripts/re/draft_check.py` was run for every one of the **40 units** with guarded functions; all compiled. The plain helper uses wibo directly. A second complete sweep uses the checked-in Satan's Fiddle adapter/profile with the same flags and both draft macros, then the helper's byte/relocation-mask comparison. All 40 profile-backed compiles also succeed. The table ranks **all 117 guarded functions by the current profile-backed differing-word count**, then retail extent, unit and symbol; the plain-helper count is retained for comparison.

Counts include zero padding through the complete retail manifest extent. The size delta is native body bytes minus that extent, so negative values can be valid alignment padding. For oversized bodies, the extra native words are compared against zero and counted as well; the denominator remains the retail extent and can be smaller than the difference count. Relocated fields are masked using the helper's rules. A zero diagnostic count does not prove resolved relocation identities, complete-unit acceptance, natural source compliance or admissible independent emission.

The recent objdump `-z` change repairs side-by-side instruction offsets and denominators; it does **not** change the helper's raw-byte scores. In particular, the 10/1172 gyorace result remains the same. The extra profile sweep distinguishes compiler state from that diagnostic fix.

The three zero-word drafts remain guarded: the object copy constructor depends on the monster-copy caller for natural emission; `StepGyoRace` belongs to the excluded ctxrows lane; `LightingEdit` retains prohibited fog-channel address arithmetic. No guard is removed on score alone. Off-limits units are read and rescored only. Previously exhaustive placement, integer-allocation and typed-layout trials are not replayed; VU0/asm targets are excluded.

## Ranked baseline

| Rank | Unit | Retail function symbol | SF words / extent | Plain words / extent | Size delta (bytes) | Documented blocker | Sweep disposition |
| ---: | --- | --- | ---: | ---: | ---: | --- | --- |
| 1 | `event_func` | `__ct__7CObjectFRC7CObject` | 0/52 | 0/52 | -0x8 | [Natural copy-constructor emission depends on guarded monster-copy caller](../event_func/notes.md) | Documented park; no repeated probes |
| 2 | `gyoracesim` | `StepGyoRace__FP15RACE_FISH_PARAMP11grRACE_INFO` | 0/204 | 0/204 | -0x4 | [Exact draft; joint complete-unit promotion still fails CollisionFish](../gyoracesim/notes.md) | Off limits; rescore only |
| 3 | `editdebug` | `LightingEdit__FP6CScene` | 0/1356 | 14/1356 | -0xC | [Source compliance: inherited fog-channel raw byte-address arithmetic](../editdebug/notes.md) | Documented park; no repeated probes |
| 4 | `mg_dataset` | `End__13mgCMDTBuilderFP8mgCFrameP12mgCVisualMDTP10mgLoadData` | 1/96 | 1/96 | 0 | [Placement-new allocation-result/null-branch scheduling](../mg_dataset/notes.md) | Documented park; no repeated probes |
| 5 | `mg_dataset` | `CopyFrame__FP8mgCFrameP8mgCFrameP9mgCMemoryiPP8mgCFrame` | 1/208 | 1/208 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../mg_dataset/notes.md) | Documented park; no repeated probes |
| 6 | `movie` | `handler_endimage__Fi` | 2/16 | 2/16 | -0x8 | [sync/ei instructions lack a native compiler expression](../movie/midday-20261008.md) | VU0/asm exclusion |
| 7 | `funcpoint` | `Add__14CFuncPointMngrFiP9mgCMemory` | 2/40 | 2/40 | 0 | [Placement-new allocation-result/null-branch scheduling](../funcpoint/notes.md) | Documented park; no repeated probes |
| 8 | `editmap` | `emapWATER_PARTS_NAME__FP9SPI_STACKi` | 2/72 | 2/72 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../editmap/notes.md) | Documented park; no repeated probes |
| 9 | `dynamicanime` | `dynCOLLISION__FP9SPI_STACKi` | 2/76 | 2/76 | 0 | [Placement-new allocation-result/null-branch scheduling](../dynamicanime/notes.md) | Documented park; no repeated probes |
| 10 | `event_func` | `_ESM_INITIALIZE__FP12RS_STACKDATAi` | 2/76 | 2/76 | -0x8 | [Placement-new scheduling; inherited Ident helper is inadmissible](../event_func/notes.md) | Documented park; no repeated probes |
| 11 | `mdslist` | `CreateChara__FPUiPcP9mgCMemory` | 2/76 | 2/76 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../mdslist/notes.md) | Documented park; no repeated probes |
| 12 | `editmap` | `emapMASK_PARTS_NAME__FP9SPI_STACKi` | 2/96 | 2/96 | -0xC | [Placement-new allocation-result/null-branch scheduling](../editmap/notes.md) | Documented park; no repeated probes |
| 13 | `editmap` | `emapRIVER_PARTS_NAME__FP9SPI_STACKi` | 2/104 | 2/104 | 0 | [Placement-new allocation-result/null-branch scheduling](../editmap/notes.md) | Documented park; no repeated probes |
| 14 | `effscript` | `AssignCharacter__16CEffectScriptManFP11_EFF_SCRIPTi` | 2/108 | 2/108 | -0x8 | [Placement-new allocation-result/null-branch scheduling](../effscript/notes.md) | Documented park; no repeated probes |
| 15 | `menuchr` | `LoadBGNPCModel__15CMenuChrCngMenuFi` | 2/120 | 2/120 | 0 | [Placement-new allocation-result/null-branch scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 16 | `sceneload` | `LoadChara__6CSceneFiPUiPcP9mgCMemoryP9mgCMemoryP9mgCMemoryii` | 2/148 | 2/148 | -0xC | [Placement-new allocation-result/null-branch scheduling](../sceneload/notes.md) | Documented park; no repeated probes |
| 17 | `editeff` | `EditSetPlaceAnime__FiP9CMapParts` | 2/156 | 2/156 | -0xC | [Placement-new allocation-result/null-branch scheduling](../editeff/notes.md) | Documented park; no repeated probes |
| 18 | `sceneload` | `CopyChara__6CSceneFiiP9mgCMemory` | 2/164 | 2/164 | -0xC | [Placement-new allocation-result/null-branch scheduling](../sceneload/notes.md) | Documented park; no repeated probes |
| 19 | `event_func` | `_COPY_CHARA__FP12RS_STACKDATAi` | 2/176 | 2/176 | -0xC | [Placement-new allocation-result/null-branch scheduling](../event_func/notes.md) | Documented park; no repeated probes |
| 20 | `menuchr` | `MenuItemCharaDataLoadEndCheckAfter__FPP17MENU_BGREAD_INFO2i` | 2/220 | 2/220 | -0x8 | [Temporary-scene model-list constructor argument scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 21 | `menusys` | `LRCheck__13CMenuItemInfoFi` | 2/220 | 2/220 | 0 | [Shared zero-return assignment versus branch delay-slot lowering](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 22 | `menuchr` | `LoadMenuData__15CMenuCostumeSelFP9mgCMemoryPi` | 2/228 | 2/228 | 0 | [Placement-new allocation-result/null-branch scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 23 | `menuchr` | `KeyStep__12CMosBookMenuFv` | 2/340 | 2/340 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 24 | `fishing` | `sgRestartFishing__FP11SubGameInfo` | 2/344 | 12/344 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../fishing/notes.md) | Documented park; no repeated probes |
| 25 | `effscript` | `BuildBase__16CEffectScriptManFiP1iP1iP9mgCMemoryi` | 2/396 | 2/396 | 0 | [Placement-new allocation-result/null-branch scheduling](../effscript/notes.md) | Documented park; no repeated probes |
| 26 | `mg_tanime` | `NewTexAnimeData__15mgCTextureAnimeFP9mgCMemory` | 6/32 | 6/32 | -0x4 | [Placement-new result lifetime and null-path join](../mg_tanime/notes.md) | Documented park; no repeated probes |
| 27 | `scenevillager` | `CharaObjectOnOff__6CSceneFiP9mgCMemory` | 6/112 | 6/112 | 0 | [Hide-loop placement-new argument/result scheduling](../scenevillager/notes.md) | Documented park; no repeated probes |
| 28 | `mg_dataset` | `CreateFrameVisual__FP8mgCFrameP9mgCMemoryP9mgCMemoryP8mgCFrameP13MDTOBJ_HEADERP10MDT_HEADERiP17mgCTextureManagerPUiiPP8mgCFramePA4_A4_f` | 6/444 | 6/444 | -0x8 | [Six placement-new allocation-result branch aliases](../mg_dataset/notes.md) | Documented park; no repeated probes |
| 29 | `dngmenu` | `MsgInit__12CMenuTreeMapFv` | 7/116 | 7/116 | 0 | [Integer screen-dimension and line-position load/store scheduling](../dngmenu/parks.md) | Documented park; no repeated probes |
| 30 | `dng_event` | `SearchMapFlatPosition__FPfP11CAutoMapGen` | 7/260 | 7/260 | 0 | [Receiver lifetimes and stack slots; inherited helpers/scalar alignment](../dng_event/notes.md) | Documented park; no repeated probes |
| 31 | `menusys` | `CalcTex__13CMenuItemInfoFv` | 7/832 | 7/832 | -0x8 | [Held-type/equipment-search saved-register permutation](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 32 | `gyoracesim` | `CollisionFish__FP15RACE_FISH_PARAMi` | 8/360 | 8/360 | -0x4 | [Inner fish-count/index saved-register allocation](../gyoracesim/notes.md) | Off limits; rescore only |
| 33 | `inventmn` | `MenuInventKey__Fv` | 8/524 | 10/524 | -0xC | [Negative-card/name/coordinate induction register allocation](../inventmn/notes.md) | Documented park; no repeated probes |
| 34 | `editloop` | `EditDraw__Fv` | 8/860 | 8/860 | -0x4 | [Texture-list count null-test/copy order and forward-loop allocation](../editloop/notes.md) | Documented park; no repeated probes |
| 35 | `movie` | `viBufRestartDMA__FP5ViBuf` | 9/200 | 9/200 | 0 | [DMA wrap count/address-mask register allocation](../movie/midday-20261008.md) | Documented park; no repeated probes |
| 36 | `gyorace` | `sgSysDrawGyoRace__FP11SubGameInfo` | 10/1172 | 10/1172 | -0xC | [Lap counter and digit-row pointer saved-register permutation](../gyorace/notes.md) | Documented park; no repeated probes |
| 37 | `gyoracesim` | `FishModifyParam__FP12grFISH_PARAMPff` | 11/480 | 11/480 | -0x8 | [Conflicting direct-call random-range float argument identities](../gyoracesim/notes.md) | Off limits; rescore only |
| 38 | `sound` | `Init__6CSoundFiiii` | 12/480 | 12/480 | -0x4 | [Integer MIDI-port kind/configuration store scheduling; no float selector](../sound/notes.md) | New per-port grouping probes; retain 12 words |
| 39 | `inventmn` | `ResetAddress__15CInventUserDataFv` | 13/48 | 13/48 | 0 | [Photo-work invariant base lifetime and unrolled-loop remainder](../inventmn/notes.md) | Documented park; no repeated probes |
| 40 | `menusys` | `CheckEnableHaveItemNum__Fv` | 13/212 | 13/212 | -0x4 | [Active-item counter/pointer and final flag offset allocation](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 41 | `menudraw` | `MenuFormDrawNormal__16CMenuPosDataFormFiiffRi` | 13/1020 | 13/1020 | -0x8 | [Integer texture-rectangle copy register allocation](../menudraw/remaining-guards-20261008.md) | Off limits; rescore only |
| 42 | `menuaqua` | `SettingAqua__9CAquariumFv` | 16/752 | 2/752 | -0xC | [47.0f argument scheduling plus character placement-new branch pair](../menuaqua/midday-20261008.md) | Documented park; no repeated probes |
| 43 | `map` | `AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory` | 19/64 | 19/64 | +0x4 | [List placement-new scheduling; redundant payload clear](../map/midday-20261008.md) | Documented park; no repeated probes |
| 44 | `menusys` | `MenuPosFormValueSetCharaRobo__FP9ROBO_DATAi` | 19/296 | 128/296 | -0x8 | [Form-color register allocation and additional source reconstruction](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 45 | `mglib` | `VSyncCallBack__Fi` | 20/32 | 20/32 | -0x10 | [sync/ei instructions lack a native compiler expression](../mglib/notes.md) | Off limits; rescore only |
| 46 | `effscript` | `SetCharacter__16CEffectScriptManFP11CCharacter2ii` | 24/168 | 24/168 | -0x4 | [Placement-new branches and saved-register permutation; inherited raw indexing](../effscript/notes.md) | Documented park; no repeated probes |
| 47 | `fishing` | `InitSuccess__FP6CScene` | 25/280 | 25/280 | -0x8 | [Player/fish saved-register permutation and placement-new branch](../fishing/notes.md) | Documented park; no repeated probes |
| 48 | `wavetable` | `Effect__10CWaveTableFv` | 27/324 | 27/324 | 0 | [Arithmetic coefficient/seam operand allocation; bounded grid indexing](../wavetable/notes.md) | Documented park; no repeated probes |
| 49 | `menuop` | `KeyStep__14CSaveMenuClassFv` | 28/1692 | 28/1692 | -0xC | [Save-state frame/local layout and integer predicate scheduling](../menuop/notes.md) | Off limits; rescore only |
| 50 | `mg_math` | `mgClipInBoxW__FPfPfPfPf` | 29/20 | 29/20 | +0x30 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 51 | `menudraw` | `CommonBoardDraw__FPfRi` | 29/888 | 52/888 | -0x8 | [Board coordinate/line-pointer integer allocation with direct typed access](../menudraw/remaining-guards-20261008.md) | Off limits; rescore only |
| 52 | `menusys` | `MenuItemDebugDraw__Fv` | 32/1260 | 34/1260 | -0xC | [Float argument setup, item-index addition and build-up-name loop registers](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 53 | `menusys` | `MenuDataSwap__FP13CGameDataUsedP13CGameDataUsedi` | 34/228 | 34/228 | -0x8 | [Presence registers, result-array setup and tail scheduling](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 54 | `mg_math` | `mgVectorMinMaxN__FPfPfPA4_fi` | 42/24 | 42/24 | +0x64 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 55 | `movie` | `vblankHandler__Fi` | 42/56 | 42/56 | -0x14 | [sync/ei instructions lack a native compiler expression](../movie/midday-20261008.md) | VU0/asm exclusion |
| 56 | `menusys` | `CommonSetMoveItemClass__FPA4_i` | 42/248 | 42/248 | -0x8 | [Input-row copy scheduling and narrowed-short sign extension](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 57 | `mg_math` | `mgApplyMatrixN__FPA4_fPA4_fPA4_fi` | 46/20 | 46/20 | +0x80 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 58 | `mg_dataset` | `CopyFrameSub__FP8mgCFrameP9mgCMemoryiPP8mgCFrame` | 46/68 | 46/68 | -0x8 | [Placement-new allocation-result/null-branch scheduling](../mg_dataset/notes.md) | Documented park; no repeated probes |
| 59 | `menushop` | `MenuNPCQuestViewInit__FP9mgCMemoryPii` | 46/76 | 46/76 | -0x8 | [Quest/menu native constructor expansion and allocation-result scheduling](../menushop/notes.md) | Documented park; no repeated probes |
| 60 | `mg_math` | `MulMatrix3__FPA4_fPA4_fPA4_f` | 49/52 | 49/52 | -0x84 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 61 | `dngmenu` | `DngTreeMapInit__FP9mgCMemoryPiii` | 55/256 | 55/256 | -0x8 | [Opening-mode boolean lowering and cursor-buffer store forwarding](../dngmenu/parks.md) | Documented park; no repeated probes |
| 62 | `title` | `TitleBootInit__Fv` | 56/676 | 56/676 | -0xC | [Boot/member construction and argument scheduling](../title/notes.md) | Off limits; rescore only |
| 63 | `mdslist` | `Copy__9CMapPieceFR9CMapPieceP9mgCMemory` | 57/156 | 57/156 | -0x4 | [Placement-new allocation-result/null-branch scheduling](../mdslist/notes.md) | Documented park; no repeated probes |
| 64 | `mg_math` | `mgApplyMatrixN_MaxMin__FPA4_fPA4_fPA4_fiPfPf` | 60/36 | 60/36 | +0x78 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 65 | `menuaqua` | `ColCheck__9CAquariumFi` | 60/448 | 60/448 | -0x8 | [Selected-fish/displacement/count saved-register lifetimes](../menuaqua/midday-20261008.md) | Documented park; no repeated probes |
| 66 | `mg_math` | `mgMulMatrix__FPA4_fPA4_fPA4_f` | 65/32 | 65/32 | +0x84 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 67 | `menuchr` | `Draw__15CMenuCostumeSelFv` | 71/568 | 71/568 | -0x10 | [Pulse merge, integer conversion and arrow-shadow argument schedule](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 68 | `water` | `CreateWaterFrame__FiiPfPfP9mgCMemory` | 82/104 | 82/104 | 0 | [Placement-new allocation-result/null-branch scheduling](../water/notes.md) | Documented park; no repeated probes |
| 69 | `editmode` | `EditMode__FP6CScene` | 87/1912 | 74/1912 | -0x4 | [Quarter-turn argument order, axis registers and ground-query scheduling](../editmode/notes.md) | Off limits; rescore only |
| 70 | `map` | `CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi` | 112/112 | 112/112 | +0x10 | [Visibility-list construction and placement-new result scheduling](../map/midday-20261008.md) | Documented park; no repeated probes |
| 71 | `menudraw` | `GeneratePoly__14CRepairManagerFPfi` | 117/144 | 117/144 | -0x2C | [Action-character base construction and placement-new result schedule](../menudraw/remaining-guards-20261008.md) | Off limits; rescore only |
| 72 | `menusys` | `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | 127/240 | 127/240 | -0x4 | [Native member construction and placement-new scheduling](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 73 | `mg_math` | `mgInversMatrix__FPA4_fPA4_f` | 140/64 | 140/64 | +0x130 | [VU0 vector/matrix/status instructions; scalar draft cannot match](../mg_math/notes.md) | VU0/asm exclusion |
| 74 | `menusys` | `MenuItemSelectInit__FP9mgCMemoryPii` | 146/164 | 146/164 | -0x4 | [Native member construction and placement-new scheduling](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 75 | `editexception` | `InitFirePowder__FiP6CSceneiP9mgCMemory` | 150/216 | 150/216 | -0x4 | [Sprite placement-new branch/copy schedule shifts construction/arithmetic tail](../editexception/notes.md) | Documented park; no repeated probes |
| 76 | `menuchr` | `MenuCostumeInit__FP9mgCMemoryPii` | 161/184 | 165/184 | -0x24 | [Menu/camera/character native constructor expansion](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 77 | `mglib` | `mgInit__Fii` | 168/652 | 168/652 | 0 | [SDK register/structure and frame-end source scheduling (see unit notes)](../mglib/notes.md) | Off limits; rescore only |
| 78 | `mg_frame` | `Draw__8mgCFrameFPUi` | 182/236 | 182/236 | +0x4 | [VU0 helper call retains output pointers across a nonstandard register contract](../mg_frame/notes.md) | VU0/asm exclusion |
| 79 | `inventmn` | `LoadCharaCheck__11CMenuInventFv` | 190/312 | 190/312 | -0x54 | [Shared character/album construction and placement-new scheduling](../inventmn/notes.md) | Documented park; no repeated probes |
| 80 | `sphida` | `DrawStatusSprite__7CSphidaFv` | 193/1096 | 168/1096 | -0x4 | [Context-dependent sprite argument scheduling and carry-digit extent](../sphida/notes.md) | Off limits; rescore only |
| 81 | `gameutil` | `MotionProc2__FP8mgCFrameP14tagMOTION_TYPEP12tagFRAME_INFP8Mot_List` | 206/224 | 206/224 | -0x40 | [VU0 skinning instructions/register state](../gameutil/notes.md) | VU0/asm exclusion |
| 82 | `effscript` | `CreateEffSpt__16CEffectScriptManFiii` | 206/320 | 206/320 | 0 | [Placement-new/member construction and subsequent character allocation](../effscript/notes.md) | Documented park; no repeated probes |
| 83 | `dngmenu` | `DrawGeoramaMateria__FiPciPii` | 224/256 | 224/256 | +0x4 | [Title/column/page lifetimes and dimension scratch slots](../dngmenu/parks.md) | Documented park; no repeated probes |
| 84 | `mglib` | `mgEndFrame__FP14mgCDrawManager` | 232/672 | 232/672 | -0x4C | [SDK register/structure and frame-end source scheduling (see unit notes)](../mglib/notes.md) | Off limits; rescore only |
| 85 | `menuchr` | `MenuCharaChangeInit__FP9mgCMemoryPii` | 236/316 | 236/316 | 0 | [Menu/camera/character native constructor expansion](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 86 | `event_func` | `_COPY_MONS2SCNCHR__FP12RS_STACKDATAi` | 246/472 | 246/472 | -0x10 | [Placement-new branch and shadow-link aggregate copy/type evidence](../event_func/notes.md) | Documented park; no repeated probes |
| 87 | `gameutil` | `CheckHits__FP13CollisionInfoPfPfiPiPA4_fii` | 252/264 | 252/264 | +0x28 | [VU0 segment-bound register state across polygon helpers](../gameutil/notes.md) | VU0/asm exclusion |
| 88 | `mglib` | `mgSetPkFrameBuffer__Fiiii` | 252/424 | 252/424 | -0x20 | [SDK register/structure and frame-end source scheduling (see unit notes)](../mglib/notes.md) | Off limits; rescore only |
| 89 | `editmode` | `LoadEditCursor__FP9mgCMemoryi` | 270/368 | 270/368 | 0 | [Cursor/character placement-new construction](../editmode/notes.md) | Off limits; rescore only |
| 90 | `menuop` | `MenuManualInit__FP9mgCMemoryPii` | 285/324 | 285/324 | +0x8 | [Manual-menu native construction/placement-new scheduling](../menuop/notes.md) | Off limits; rescore only |
| 91 | `menuchr` | `MenuCharaChangeStarDraw__Fv` | 285/368 | 285/368 | -0x3C | [Center/rectangle storage, manager and floating register lifetimes](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 92 | `gameutil` | `MotionProc__FP8mgCFramefP8Mot_ListP9mgCCamera` | 289/596 | 289/596 | -0x1C | [Camera/key saved-register allocation and vertex-key control flow](../gameutil/notes.md) | Eligible for new vertex/lifetime evidence |
| 93 | `menuchr` | `EnterDataMenu__15CMenuChrCngMenuFPUc` | 376/388 | 376/388 | +0x10 | [Palette/command array indexing and NPC index/stride allocation](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 94 | `title` | `TitleModeKey__Fv` | 379/624 | 15/624 | -0x8 | [Card-readiness snapshots and context-dependent fade scheduling](../title/notes.md) | Off limits; rescore only |
| 95 | `inventmn` | `IsCreateObject__11CMenuInventFii` | 380/1380 | 380/1380 | -0x28 | [Shared character/album construction and placement-new scheduling](../inventmn/notes.md) | Documented park; no repeated probes |
| 96 | `gyorace` | `sgInitGyoRace__FP11SubGameInfo` | 431/1152 | 434/1152 | +0x8 | [Character/texture construction, entrant lifetimes and oversized body](../gyorace/notes.md) | Documented park; no repeated probes |
| 97 | `dngmenu` | `DrawRoomOne__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOM_INFOUiif` | 459/584 | 460/584 | -0x5C | [Picture/glyph/tint lifetimes and frame/room-flag scheduling](../dngmenu/parks.md) | Documented park; no repeated probes |
| 98 | `gyorace` | `sgLoopGyoRace__FP11SubGameInfo` | 490/1676 | 1222/1676 | 0 | [Race state-machine branches and register/float scheduling](../gyorace/notes.md) | Documented park; no repeated probes |
| 99 | `menusys` | `IsAskExtend__13CMenuItemInfoFii` | 528/668 | 528/668 | -0x8 | [Native member construction and placement-new scheduling](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 100 | `inventmn` | `MenuInventInit__FP9mgCMemoryPii` | 606/1044 | 606/1044 | -0x38 | [Shared character/album construction and placement-new scheduling](../inventmn/notes.md) | Documented park; no repeated probes |
| 101 | `dngmenu` | `DrawDngRoomInfo__FP16DNGMAP_ROOM_INFO` | 610/712 | 610/712 | +0x80 | [Panel temporaries, message branches and integer allocation](../dngmenu/parks.md) | Documented park; no repeated probes |
| 102 | `menuaqua` | `DrawFishParam__FiiP10mgCTextureP13CGameDataUsed` | 648/704 | 648/704 | -0x5C | [Edge-load/template-copy order, background loop and later argument preparation](../menuaqua/midday-20261008.md) | Documented park; no repeated probes |
| 103 | `fishing` | `StepDataLoading__FPv` | 651/732 | 651/732 | +0x18 | [Repeated member construction and oversized native body](../fishing/notes.md) | Documented park; no repeated probes |
| 104 | `mg_tanime` | `TexAnime__15mgCTextureAnimeFiP13sceVif1Packet` | 660/1304 | 660/1304 | -0x10 | [Missing nops shift later code; inherited draft scaffolding](../mg_tanime/notes.md) | Documented park; no repeated probes |
| 105 | `dngmenu` | `DrawRoot__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOT_INFOiUii` | 708/844 | 708/844 | -0x44 | [Shape dispatch/loops and color/mark/root saved-register allocation](../dngmenu/parks.md) | Documented park; no repeated probes |
| 106 | `menumain` | `MenuMainInit__FP13MENU_INIT_ARG` | 792/948 | 792/948 | -0x1C | [Menu/scene/camera native construction and placement-new result scheduling](../menumain/notes.md) | Documented park; no repeated probes |
| 107 | `menuchr` | `KeyStep__14CMenuMosSelectFv` | 843/1660 | 843/1660 | 0 | [Menu state-machine reconstruction, branch and register scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |
| 108 | `dngmenu` | `LoadDngInfo__11CDngFreeMapFP9mgCMemoryiiii` | 914/1016 | 914/1016 | -0x78 | [Remapping/control flow, candidate-loop scheduling and native table migration](../dngmenu/parks.md) | Documented park; no repeated probes |
| 109 | `inventmn` | `IsAccessAlbum__11CMenuInventFv` | 1080/1276 | 1080/1276 | -0x20 | [Shared character/album construction and placement-new scheduling](../inventmn/notes.md) | Documented park; no repeated probes |
| 110 | `menuaqua` | `Step__9CAquariumFv` | 1111/1732 | 1111/1732 | -0xC | [Menu/state switches and question/simulation register scheduling](../menuaqua/midday-20261008.md) | Documented park; no repeated probes |
| 111 | `dng_status` | `DrawMainUnitStatusBord__Ff` | 1117/1192 | 1122/1192 | -0x58 | [Sprite/output stack slots, position/spill lifetimes and board branches](../dng_status/notes.md) | Documented park; no repeated probes |
| 112 | `editloop` | `EditLoop__Fv` | 1180/2228 | 1179/2228 | -0x8 | [Loop state/control-flow and saved/local lifetimes](../editloop/notes.md) | Documented park; no repeated probes |
| 113 | `menusys` | `MenuItemDebugKey__Fv` | 1208/1460 | 1207/1460 | 0 | [Native member construction and placement-new scheduling](../menusys/matching-round1-20261008.md) | Off limits; rescore only |
| 114 | `editloop` | `EditInit__F13INIT_LOOP_ARG` | 1226/1776 | 1230/1776 | -0x48 | [Constructor expansion, allocation and initialization scheduling](../editloop/notes.md) | Documented park; no repeated probes |
| 115 | `inventmn` | `CalcTex__11CMenuInventFv` | 1228/1256 | 1228/1256 | +0x10 | [Clipping aggregate/field-address lifetimes and oversized frame/body](../inventmn/notes.md) | Documented park; no repeated probes |
| 116 | `dngmenu` | `Step__12CMenuTreeMapFv` | 1488/1548 | 1488/1548 | -0xFC | [Mode/key/message lifetimes, stack frame and later state branches](../dngmenu/parks.md) | Documented park; no repeated probes |
| 117 | `menuchr` | `KeyChangeMain__15CMenuChrCngMenuFv` | 1792/2152 | 1792/2152 | -0x8 | [Menu state-machine reconstruction, branch and register scheduling](../menuchr/midday-r1-assessment.md) | Off limits; rescore only |

## Receipts

All paths are relative to the assigned worktree. Private receipts are not staged:

- `.private/sweep-midday/baseline-build.log`, `baseline-objects.log` and `baseline-progress-manifest.log`: initial build, complete objects, coverage preparation and refreshed 6,872-function manifest.
- `.private/sweep-midday/baseline-coverage.log`, `baseline-image.sha256` and `baseline-fingerprints.json`: function classifications, linked ELF hash and allocated-section/relocation inventories of all 149 units.
- `.private/sweep-midday/rescore-plain.log`, `drafts/*.log` and `guarded-scores-plain-baseline.json`: every requested plain draft check.
- `.private/sweep-midday/rescore-sf.log`, `drafts-sf/*.log` and `guarded-scores-sf-baseline.json`: the matching compiler-profile rescore.
- `.private/sweep-midday/guarded-functions-baseline.json`, `guarded-units.txt` and `guarded-ranked-baseline.json`: exact classified queue, unit list and structured ranked table.

The m2c outputs, source hypotheses and final acceptance measurements for any worked unit are documented in its own notes. No tooling, compiler profile, off-limits source or shared header is changed by this sweep.
