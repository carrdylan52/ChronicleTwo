# PR #7 cleanup checkpoint - 2026-10-06 (update 2)

This is a verified takeover checkpoint for Adubbz, published at Dylan's request. It contains fakegenie's migration plus cleanup of **all 136 game source files the migration changed**, other than the protected `dng_main` files noted below. Full-branch final review is still yours.

Code checkpoint: `9a02dc7`. The fresh-clone check below was run on an identical tree except for one later review fix in `dynamicanime.cpp` (typed `sizeof` element sizes and upstream's local declaration order); the tree with that fix was rebuilt with a byte-identical progress report and unchanged draft results. The commit updating this document changes documentation only.

## What changed since the first checkpoint (`9160add`)

Fifty-one cleanup commits are added on top of the first checkpoint, one per source unit, each with its related header, notes and shared-owner changes. The published history through `9160add` is unchanged. The 51 files:

| Group | Files |
|---|---|
| Graphics engine | `mglib`, `mg_texture`, `mg_dataset`, `mg_tanime` |
| Extra modes | `vlgr_info`, `nameregi`, `pbuggy`, `gyorace`, `gyoracesim`, `subgame`, `pot` |
| Editor | `editmenu`, `editmode`, `editexception`, `editeff`, `editevent`, `editanalyze`, `editcoll`, `editdebug`, `editmapeffect` |
| Game utilities and small units | `gameutil`, `font`, `nowload`, `gamepad`, `cameracontrol`, `title`, `funcpoint`, `drawwin`, `villagermngr`, `main`, `padcontrol` |
| Sound, movies and event helpers | `movie`, `scenesnd`, `snd_mngr`, `event`, `eventedit`, `eventsprite`, `movieviewlp`, `ezbgm`, `convviewlp`, `ezmidi`, `wavetable`, `sound`, `snd_seseq` |
| Characters | `character`, `actionchara`, `dynamicanime`, `monster`, `sphida`, `swordeffect`, `visualmotion` |

Each group was reviewed on its own, with fixes folded into each file's commit; the combined tree then had its own integration review. Notable points:

- **Every unit's guarded drafts now compile.** The PR head had eight draft-compile failures; the first checkpoint had five; there are none now.
- `ps2/config/pal/migrated_units.txt` is now empty: `gameutil` joins `dngmenu` in building from its C++ and guarded fallbacks instead of whole-unit retail assembly.
- `sceGsTex0::operator=` is back to upstream's form: the SDK type has no declared assignment operator, and `__as__9sceGsTex0FRC9sceGsTex0` is retail assembly in `mg_texture`. That also lets `water`'s `CFireRaster::SetTexture` draft match.
- `EIntr` has its real SDK signature `int EIntr(void)`. The movie interrupt handlers no longer contain PR-only `sync`/`ei` inline assembly or a padding record shaped for the stack; their clean bodies are guarded drafts with retail fallbacks (`stepMain`, `vblankHandler`, `handler_endimage`).
- `sceCdlFILE` carries its real `flag` member; the extra `StrFile` word is gone and the layouts are unchanged.
- Invented helper types and integer pointer shuttles were removed (`mgCFrame::BoundCorners`, a texture pointer stored in an `int`, byte-offset list walks, an integer address subtraction in `gameutil`).
- Bug fixes in source: the editor's `DownLoadMes[6]` is now eight slots, covering the loop that reads it; a `printf("load sound %d\n")` call in `scenesnd` now passes its argument, as retail does.
- Shared return types follow retail callers: `CSaveData::GetBuildPartsNum`, `CheckNowTourEvent` and `CheckNowTourType` return `int`, and `PreLoadSync` returns `ReadBGSync()`'s result.
- `mgRect<short>::Set` stays declared out of line in `mg_tanime.hpp` and defined in `menuchr.cpp`; an inline header body would collide with that definition.

## Base and credit

- Upstream master: `a2578fe`.
- Migration: fakegenie's PR #7 head `3501e92`, locally merged as `a6e489b`.
- Fake Genie authored the migrated implementations. His original commits and authorship are preserved in this branch; the subsequent cleanup commits are Dylan's.
- The accepted PR build and post-processing changes are included unchanged by this cleanup.

**Important: Fake Genie subsequently pushed `06f56b3292deb2e39754d62cc0eddc8d7bdba266` ("wip", 2026-10-06 05:00:54 UTC), then `ca5f969c19b6fad5daf2f49d78872ad533d51b40` ("wip", 06:41:37 UTC, one more commit touching 48 files). The first of those changes 155 files, including code, headers and build/configuration files. Neither is merged into this checkpoint.** Compare the current PR head before combining branches; do not overwrite his newer additions with these older-base cleaned files.

`ca5f969` still lacks the script described next; it has not been built here. An independent fresh build of `06f56b3` failed with exit 1: the new CMake rules require `scripts/build/state.py`, which is absent from the tracked tree. No executable or real build progress report was produced, and zero sections were verified. A separate, explicitly diagnostic plain-MWCC compile of all 149 game units found **6,300 exact game-object scores: 606 gains and 3 losses against the old author head**. That diagnostic bypassed the missing state mechanism and does not establish matching linked code. The author also added 254 guarded drafts; draft compilation passed 147/149 units, failing `ezbgm` and `quest`.

To combine the work, retain Fake Genie's newer commits and reconcile by function in a fresh candidate. Preserve his additive coverage and the cleanup here. Resolve the missing state mechanism before accepting its build rules; at `ca5f969`, `state_units.txt` has 23 rows, including eight `drafts` modes whose behavior cannot be audited without the missing script. Shared constructor/header changes and the author's further `dng_main.cpp` changes must be resolved with its owner. Then repeat the linked-image, progress and all-drafts checks. Dylan has a detailed local reconciliation report and symbol-level receipts.

## What was done

- Restored upstream's names, comments, types and header layouts where they still match; merged matching implementations with upstream drafts rather than replacing those drafts wholesale.
- Replaced mangled callable aliases, label-based literals, invented object views and avoidable pointer arithmetic with ordinary C++ where the compiler permits it.
- Restored retail-local function linkage and moved numerous handler tables and data definitions into typed source; table strings, padding and initializers were checked against retail.
- Retested proposed hand-backs and recovered several matches during review. Remaining clean nonmatching forms retain `NONMATCHING` guards and retail assembly fallbacks.
- Fixed a guarded menu allocation bug: the repair manager now initializes all eight effect stacks, matching retail, rather than six.
- Removed `dngmenu` and `gameutil` from the whole-unit assembly switch.
- Reconciled shared save/user-data return types and transformation sound-bank layout. Kept main-loop assembly-storage declarations in their owning source to avoid collisions with edit-mode statics.

## Verification

A fresh clone of the code, with fresh generated build files, passed the PR's toolchain and retail verifier:

```text
SCES_511.90: OK (5758 perfect, 3 fuzzy, 2652 asm, 0 unmatched)
sections OK: 11; build exit 0
```

The eleven loaded executable sections match retail. This does not mean the entire ELF file is byte-identical, and no gameplay test is claimed.

Game-only progress, excluding SDK functions and internal `.L` labels:

| State | PR head 3501e92 | First checkpoint | This checkpoint |
|---|---:|---:|---:|
| Perfect | 5697 | 5659 | 5628 |
| Fuzzy | 6 | 5 | 3 |
| Assembly/unscored | 1169 | 1208 | 1241 |
| Total game functions | 6872 | 6872 | 6872 |

Against the PR head there are 38 newly perfect entries and 107 formerly perfect entries now assembly/unscored, for a net decrease of 69. The loss is deliberate: where the PR's matching body relied on inline assembly, invented types, pointer arithmetic or similar, the clean C++ is kept as a guarded draft with a retail assembly fallback. Each was retried with focused probes before being handed back. Unscored templates can still be compiled C++ with matching linked bytes. The exact formerly perfect entries are listed below.

All-unit draft compilation:

```text
units 149  match 5726  differ 343  no-draft 803
```

No unit fails to compile with drafts enabled. Guarded draft compilation and review provide evidence beyond the normal executable check, which does not execute guarded code.

Each cleanup commit was built on its own in its group's branch. The combined history was checked as a whole: each group's merged files were compared byte for byte with its reviewer's tested combination, and the final tree had a fresh-clone build and full draft check. Not every intermediate commit of the combined history was rebuilt; use the final checkpoint as the verified base.

## Open items

- `event_func`: its 697-row `ext_func_info__2` table remains assembly-backed and referenced by `extern`. Moving it to source caused fifteen same-named handler collisions across units. The remaining typed storage and handler cleanup is present.
- `mainloop`: the assembly initializer still owns six typed storage objects. `CEditData`'s constructor has an assembly slot because upstream's inline header placement otherwise leaves calls from assembly-backed code unresolved. Its clean inline definition remains in `editdata.hpp`.
- Compiler accommodations remain where probes failed, including password's file-wide optimization level 0 and some `global_optimizer`/schedule settings. `mgCCamera::GetCameraMatrix` retains the three required self-assignments. Some individually documented gotos/casts remain (for example two gotos in `swordeffect`).
- Vector-unit inline assembly matching upstream/retail remains. SDK source migrations and tool changes inherited from fakegenie's original PR have not been cleaned or removed; their scope still needs the maintainer's decision.
- This cleanup did not edit `ps2/src/dng_main.cpp` or `ps2/include/dng_main.hpp`: both are byte-for-byte the original PR head's versions. The PR's own differences from upstream are inherited and still need reconciliation with Plarpoon. `actscript`'s `_SET_ACCUME_FLAG` stays assembly because a clean draft needs field names in `dng_main.hpp`.
- The newer author update has not been reconciled, and full-branch final review is outstanding. This checkpoint is ready for takeover, not a claim that the complete migration is ready to merge.

## Taking over

```sh
git clone --branch handoff/pr7-cleanup-20261006 https://github.com/carrdylan52/ChronicleTwo.git
cd ChronicleTwo
git submodule update --init --recursive
```

Use your own checksum-matching PAL executable in the repository's expected ROM path and run the repository's build workflow. ROMs, generated binaries and verification logs are not included in the branch.

Start by comparing this checkpoint with the current author head. The per-unit source notes describe retained compiler forms and hand-backs. The original migration is credited to Fake Genie; preserve that credit if you squash or re-land this branch.

## Cleaned files

`actionchara`, `actscript`, `automap`, `cameracontrol`, `character`, `charasetup`, `collision`, `colprim`, `convviewlp`, `dataread`, `dbg_font`, `dng_debug`, `dng_effect`, `dng_event`, `dngfloor`, `dng_hud`, `dngmenu`, `dng_object`, `dng_status`, `drawwin`, `dynamicanime`, `editanalyze`, `editcoll`, `editctrl`, `editdata`, `editdebug`, `editeff`, `editevent`, `editexception`, `editinfo`, `editloop`, `editmap`, `editmap2`, `editmapeffect`, `editmenu`, `editmode`, `editparts`, `editriver`, `effect`, `effectlist`, `effscript`, `event`, `eventedit`, `event_func`, `eventsprite`, `ezbgm`, `ezmidi`, `fishing`, `fishingobj`, `font`, `funcpoint`, `gamedata`, `gamepad`, `gameutil`, `gyorace`, `gyoracesim`, `helpmes`, `intersection`, `inventmn`, `main`, `mainloop`, `maintex`, `map`, `mapinfo`, `mapjump`, `mapload`, `mapparts`, `mapselect`, `mapsky`, `mdslist`, `memcard`, `menuaqua`, `menucapt`, `menuchr`, `menucls1`, `menucommon`, `menudraw`, `menumain`, `menumap`, `menuop`, `menushop`, `menusys`, `menusystemdata`, `mg_camera`, `mg_dataset`, `mg_drawenv`, `mg_drawprim`, `mg_frame`, `mglib`, `mg_math`, `mg_memory`, `mg_shadow`, `mg_sprite`, `mg_tanime`, `mg_texture`, `mg_visual`, `monster`, `movie`, `movieviewlp`, `nameregi`, `nd_meswin`, `nowload`, `npccfg`, `object`, `occlusion`, `outline`, `padcontrol`, `password`, `pbuggy`, `photo`, `pot`, `quest`, `runscript`, `runscript_opcodes`, `savedata`, `savedatadungeon`, `scene`, `sceneevent`, `sceneload`, `sceneseq`, `scenesnd`, `scenevillager`, `snd_mngr`, `snd_seseq`, `sound`, `sphida`, `subgame`, `swordeffect`, `sysmes`, `title`, `userdata`, `villagermngr`, `visualmotion`, `vlgr_info`, `water`, `wavetable`.

## Formerly perfect entries now assembly/unscored

| File | Symbol | New progress state |
|---|---|---|
| `actionchara` | `GuardEffectSet__FP6CScenePf` | asm |
| `actionchara` | `HitEffectSet__FP6CScenePf` | asm |
| `actscript` | `_SET_ACCUME_FLAG__FP12RS_STACKDATAi` | asm |
| `automap` | `CreatTermParts__11CAutoMapGenFv` | asm |
| `cameracontrol` | `SetRotate__14CCameraControlFf` | asm |
| `character` | `__as__7CObjectFRC7CObject` | asm |
| `character` | `_CLOTH__FP9SPI_STACKi` | asm |
| `character` | `DeleteExtMotion__11CCharacter2Fv` | asm |
| `character` | `DrawDirect__11CCharacter2Fv` | asm |
| `character` | `_OBJECT_NAME__FP9SPI_STACKi` | asm |
| `character` | `_SHADOW_MODEL__FP9SPI_STACKi` | asm |
| `dng_debug` | `DBGCMD_ReloadEnemy__Fii` | asm |
| `dng_debug` | `dngDebugDraw__Fv` | asm |
| `dng_effect` | `Draw__18CMapEffectsManegerFP9mgCCamera` | asm |
| `dng_event` | `Switch__20CStartupEpisodeTitleFi` | asm |
| `dngmenu` | `Draw__11CDngFreeMapFv` | asm |
| `dngmenu` | `GetEntranceRoomGlid__11CDngFreeMapFv` | asm |
| `dngmenu` | `Init__6ClsMesFv` | asm |
| `dngmenu` | `Set__9mgRect_f_Fffff` | asm |
| `editcoll` | `Copy__14CEditCollisionFR14CEditCollisioniP9mgCMemory` | asm |
| `editcoll` | `DeleteVerticalPoly__14CEditCollisionFv` | asm |
| `editcoll` | `PickupVerticalPoly__14CEditCollisionFv` | asm |
| `editeff` | `EditPlaceEffect__FP10CEditPartsPf` | asm |
| `editeff` | `EditSetEffectBuffer__FP9mgCMemory` | asm |
| `editeff` | `EditSetPlaceAnime__FiP9CMapParts` | asm |
| `editexception` | `InitFirePowder__FiP6CSceneiP9mgCMemory` | asm |
| `editmap` | `ClearAllParts__8CEditMapFv` | asm |
| `editmap` | `GetSameParts__8CEditMapFi` | asm |
| `editmap2` | `GetSeSrcVolPan__8CEditMapFPiPfPfi` | asm |
| `effscript` | `AssignCharacter__16CEffectScriptManFP11_EFF_SCRIPTi` | asm |
| `effscript` | `CreateEffSpt__16CEffectScriptManFiii` | asm |
| `effscript` | `SetCharacter__16CEffectScriptManFP11CCharacter2ii` | asm |
| `event_func` | `_AMG_GET_ATTR_STATUS__FP12RS_STACKDATAi` | asm |
| `event_func` | `_DELETE_CHARA__FP12RS_STACKDATAi` | asm |
| `event_func` | `_DIST_VECTOR2__FP12RS_STACKDATAi` | asm |
| `event_func` | `_DIST_VECTOR__FP12RS_STACKDATAi` | asm |
| `event_func` | `_LINE_POINT_DIST__FP12RS_STACKDATAi` | asm |
| `event_func` | `_OBJS_SET_EOH_FRAME_POS__FP12RS_STACKDATAi` | asm |
| `fishing` | `InitSuccess__FP6CScene` | asm |
| `gameutil` | `CheckHits__FP13CollisionInfoPfPfiPiPA4_fii` | asm |
| `gameutil` | `MotionProc2__FP8mgCFrameP14tagMOTION_TYPEP12tagFRAME_INFP8Mot_List` | asm |
| `gameutil` | `MoveCheck__FPfPfPfP13MoveCheckInfoP6CCPolyii` | asm |
| `gyoracesim` | `grGetFishProgress__FP11grRACE_INFOifP15grRACE_PROGRESS` | asm |
| `inventmn` | `CheckPhotoFlag__Fv` | asm |
| `inventmn` | `GetInventUserDataPtr__Fv` | asm |
| `inventmn` | `HowMuchZairyouMakeItem__17CInventDataManageFiiPi` | asm |
| `mainloop` | `__ct__9CEditDataFv` | asm |
| `map` | `AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory` | asm |
| `map` | `CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi` | asm |
| `map` | `DrawWater__4CMapFP9mgCCameraP10mgCTextureP10mgCTexture` | asm |
| `map` | `GetCharaLight__4CMapFP9mgCObjectP10CFuncPointii` | asm |
| `mapinfo` | `OutputLightData__8CMapInfoFPc` | asm |
| `mapjump` | `LoadSubMap__FP6CSceneii` | asm |
| `mapjump` | `MapJump__FP6CSceneP17SCN_LOADMAP_INFO2i` | asm |
| `memcard` | `CheckOmakeFile__18CMemoryCardManagerFv` | asm |
| `menuaqua` | `_PRIZE_GROUP__FP9SPI_STACKi` | asm |
| `menuchr` | `KeyStep__12CMosBookMenuFv` | asm |
| `menuchr` | `LoadMenuData__15CMenuCostumeSelFP9mgCMemoryPi` | asm |
| `menuchr` | `MenuCharaChangeInit__FP9mgCMemoryPii` | asm |
| `menuchr` | `MenuNPCLoadCheck__FP12CActionCharaP9mgCMemoryi` | asm |
| `menuchr` | `Set__9mgRect_s_Fssss` | asm |
| `menudraw` | `PrimQuad_f___FP11mgCDrawPrim9mgRect_f_9mgRect_i_` | asm |
| `menudraw` | `Step__14CLevelUpEffectFv` | asm |
| `menuop` | `MenuSaveInit__FP9mgCMemoryPii` | asm |
| `menusys` | `CheckUse__11CItemSelectFP13CGameDataUsed` | asm |
| `menusys` | `menu_inputkey_limmit_check_line__12CMenuKeyFuncFi` | asm |
| `menusys` | `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | asm |
| `mg_dataset` | `CopyFrame__FP8mgCFrameP8mgCFrameP9mgCMemoryiPP8mgCFrame` | asm |
| `mg_dataset` | `CopyFrameSub__FP8mgCFrameP9mgCMemoryiPP8mgCFrame` | asm |
| `mg_dataset` | `CreateFrameVisual__FP8mgCFrameP9mgCMemoryP9mgCMemoryP8mgCFrameP13MDTOBJ_HEADERP10MDT_HEADERiP17mgCTextureManagerPUiiPP8mgCFramePA4_A4_f` | asm |
| `mg_dataset` | `End__13mgCMDTBuilderFP8mgCFrameP12mgCVisualMDTP10mgLoadData` | asm |
| `mg_dataset` | `htoi__FPc` | asm |
| `mg_dataset` | `mgCopyFrame__FP8mgCFrameP9mgCMemoryi` | asm |
| `mg_dataset` | `mgLoadMDSFile__FP10mgLoadData` | asm |
| `mg_dataset` | `mgSetFrameAttr__FP8mgCFramei` | asm |
| `mg_drawenv` | `GetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT` | asm |
| `mg_drawenv` | `SetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT` | asm |
| `mg_drawprim` | `Draw__14mgCDrawManagerFiP13sceVif1Packet` | asm |
| `mg_drawprim` | `Texture__11mgCDrawPrimFP10mgCTexture` | asm |
| `mglib` | `mgGetFogParam__FP11mgFOG_PARAM` | asm |
| `mglib` | `mgGetFrameBackBuffer__FP10mgCTexture` | asm |
| `mglib` | `mgGetFrameBuffer__FP10mgCTexture` | asm |
| `mg_tanime` | `NewTexAnimeData__15mgCTextureAnimeFP9mgCMemory` | asm |
| `mg_tanime` | `NewTexAnimeGroupData__15mgCTextureAnimeFiP9mgCMemory` | asm |
| `mg_texture` | `AddHash__17mgCTextureManagerFP10mgCTexture` | asm |
| `mg_texture` | `__as__9sceGsTex0FRC9sceGsTex0` | asm |
| `mg_texture` | `DelHash__17mgCTextureManagerFP10mgCTexture` | asm |
| `mg_texture` | `mgGetIMGHeader__FPci` | asm |
| `mg_texture` | `ReloadTexture__17mgCTextureManagerFiP13sceVif1Packet` | asm |
| `mg_visual` | `Copy__15mgCVisualFixMDTFP9mgCMemory` | asm |
| `mg_visual` | `CopyMaterial__FP10mgMaterialP13MDT_MATERIAL_P17mgCTextureManager` | asm |
| `mg_visual` | `CreatePacket__12mgCVisualMDTFP14mgCDrawManager` | asm |
| `mg_visual` | `CreatePacket__15mgCVisualFixMDTFP14mgCDrawManager` | asm |
| `monster` | `GuardEffectSet__FP6CScenePfi` | asm |
| `movie` | `handler_endimage__Fi` | asm |
| `movie` | `stepMain__FPv` | asm |
| `movie` | `vblankHandler__Fi` | asm |
| `nowload` | `CreateNowLoading__FP14NowLoadingInfo` | asm |
| `pbuggy` | `sgInitBuggy__FP11SubGameInfo` | asm |
| `runscript_opcodes` | `_GET_INDEXOBJ_SIZE__FP12RS_STACKDATAi` | asm |
| `runscript_opcodes` | `_SET_INDEXOBJ_SIZE__FP12RS_STACKDATAi` | asm |
| `scene` | `ClearStack__6CSceneFi` | asm |
| `scenevillager` | `CharaObjectOnOff__6CSceneFiP9mgCMemory` | asm |
| `userdata` | `GetNumSameItem__16CUserDataManagerFi` | asm |
| `userdata` | `LeaveMonicaItemCheck__Fv` | asm |
| `villagermngr` | `GetAppearVlgr__13CVillagerMngrFiiiPiPP18CVillagerPlaceInfo` | asm |
| `visualmotion` | `Copy__18mgCVisualMotionMDTFP9mgCMemory` | asm |

Treat this record as a checkpoint to verify against the branch and current PR state before changing it.
