# PR #7 cleanup checkpoint - 2026-10-06

This is a verified takeover checkpoint for Adubbz, published at Dylan's request. It contains fakegenie's migration plus cleanup of **85 game source files**. It is a partial cleanup, with 51 planned files still outside this checkpoint.

Code checkpoint: `eb5d8c3560934da28b7e9f17a1d7a52623d9b114`. The commit adding this document changes documentation only.

## Base and credit

- Upstream master: `a2578fe`.
- Migration: fakegenie's PR #7 head `3501e92`, locally merged as `a6e489b`.
- Fake Genie authored the migrated implementations. His original commits and authorship are preserved in this branch; the subsequent cleanup commits are Dylan's.
- The accepted PR build and post-processing changes are included unchanged by this cleanup.

**Important: Fake Genie subsequently pushed `06f56b3292deb2e39754d62cc0eddc8d7bdba266` ("wip", 2026-10-06 05:00:54 UTC). That newer update changes 155 files, including code, headers and build/configuration files. It is not merged into this checkpoint.** Compare the current PR head before combining branches; do not overwrite his newer additions with these older-base cleaned files.

An independent fresh build of that author head failed with exit 1: the new CMake rules require `scripts/build/state.py`, which is absent from the tracked tree. No executable or real build progress report was produced, and zero sections were verified. A separate, explicitly diagnostic plain-MWCC compile of all 149 game units found **6,300 exact game-object scores: 606 gains and 3 losses against the old author head**. That diagnostic bypassed the missing state mechanism and does not establish matching linked code. The author also added 254 guarded drafts; draft compilation passed 147/149 units, failing `ezbgm` and `quest`.

To combine the work, retain Fake Genie's four new commits and reconcile by function in a fresh candidate. Preserve his additive coverage and the cleanup here. Resolve the missing state mechanism before accepting its build rules; the new `state_units.txt` has 21 rows, including six `drafts` modes whose behavior cannot be audited without the missing script. Shared constructor/header changes and the author's further `dng_main.cpp` changes must be resolved with its owner. Then repeat the linked-image, progress and all-drafts checks. Dylan has a detailed local reconciliation report and symbol-level receipts.

## What was done

- Restored upstream's names, comments, types and header layouts where they still match; merged matching implementations with upstream drafts rather than replacing those drafts wholesale.
- Replaced mangled callable aliases, label-based literals, invented object views and avoidable pointer arithmetic with ordinary C++ where the compiler permits it.
- Restored retail-local function linkage and moved numerous handler tables and data definitions into typed source; table strings, padding and initializers were checked against retail.
- Retested proposed hand-backs and recovered several matches during review. Remaining clean nonmatching forms retain `NONMATCHING` guards and retail assembly fallbacks.
- Fixed a guarded menu allocation bug: the repair manager now initializes all eight effect stacks, matching retail, rather than six.
- Removed `dngmenu` from the whole-unit assembly switch; its normal executable is now built from its C++ and guarded fallbacks.
- Reconciled shared save/user-data return types and transformation sound-bank layout. Kept main-loop assembly-storage declarations in their owning source to avoid collisions with edit-mode statics.

## Verification

A fresh clone of the code checkpoint, with fresh generated build files, passed the PR's toolchain and retail verifier:

```text
SCES_511.90: OK (5789 perfect, 5 fuzzy, 2619 asm, 0 unmatched)
sections OK: 11; build exit 0
```

The eleven loaded executable sections match retail. This does not mean the entire ELF file is byte-identical, and no gameplay test is claimed.

Game-only progress, excluding SDK functions and internal `.L` labels:

| State | PR head 3501e92 | Checkpoint |
|---|---:|---:|
| Perfect | 5697 | 5659 |
| Fuzzy | 6 | 5 |
| Assembly/unscored | 1169 | 1208 |
| Total game functions | 6872 | 6872 |

There are 23 newly perfect entries and 61 formerly perfect entries now assembly/unscored, for a net decrease of 38. The loss includes deliberate clean-source hand-backs and template/source-scoring changes; unscored templates can still be compiled C++ with matching linked bytes. The exact formerly perfect entries are listed below. Source bodies, guarded-draft scores and executable progress are distinct measurements.

All-unit draft compilation:

```text
NOT COMPILING: actionchara
NOT COMPILING: convviewlp
NOT COMPILING: ezbgm
NOT COMPILING: mglib
NOT COMPILING: snd_mngr
units 149  match 5530  differ 247  no-draft 808
```

Those five failures also existed at the original PR head. `quest`, `gamedata` and `mg_visual` are fixed in this checkpoint. No additional failing unit appears. Guarded draft compilation and review provide evidence beyond the normal executable check, which does not execute guarded code.

Cleanup commits were built individually in their isolated source branches. Shared-header integration exposed compile/link failures that were resolved before the final fresh-clone check. Not every intermediate commit of this combined history was rebuilt; use the final checkpoint as the verified base.

## Remaining work and boundaries

Implementation candidates for these 51 files exist locally, but are not incorporated or approved by this published checkpoint:

| Group | Files |
|---|---|
| Graphics engine | `mglib`, `mg_texture`, `mg_dataset`, `mg_tanime` |
| Characters | `character`, `actionchara`, `dynamicanime`, `monster`, `sphida`, `swordeffect`, `visualmotion` |
| Editor | `editmenu`, `editmode`, `editexception`, `editeff`, `editevent`, `editanalyze`, `editcoll`, `editdebug`, `editmapeffect` |
| Sound, movies and event helpers | `sound`, `snd_seseq`, `snd_mngr`, `ezmidi`, `ezbgm`, `scenesnd`, `wavetable`, `event`, `eventsprite`, `eventedit`, `movie`, `movieviewlp`, `convviewlp` |
| Game utilities and small units | `gameutil`, `font`, `nowload`, `gamepad`, `cameracontrol`, `title`, `funcpoint`, `drawwin`, `villagermngr`, `main`, `padcontrol` |
| Extra modes | `vlgr_info`, `nameregi`, `pbuggy`, `gyorace`, `gyoracesim`, `subgame`, `pot` |

Independent reviews of the graphics-engine, editor and extra-mode candidates are in progress. The other three groups still need independent review. No further implementation work is running. Please coordinate importing those results with Dylan to avoid repeating completed work.

Open items:

- `event_func`: its 697-row `ext_func_info__2` table remains assembly-backed and referenced by `extern`. Moving it to source caused fifteen same-named handler collisions across units. The remaining typed storage and handler cleanup is present.
- `mainloop`: the assembly initializer still owns six typed storage objects. `CEditData`'s constructor has an assembly slot because upstream's inline header placement otherwise leaves calls from assembly-backed code unresolved. Its clean inline definition remains in `editdata.hpp`.
- `gameutil`: still listed in `ps2/config/pal/migrated_units.txt`, so its executable uses whole-unit retail assembly while its C++ is separately scored by objdiff. The unpublished utility candidate removes that switch; it is outside this checkpoint.
- Compiler accommodations remain where probes failed, including password's file-wide optimization level 0 and some `global_optimizer`/schedule settings. `mgCCamera::GetCameraMatrix` retains the three required self-assignments. Some individually documented gotos/casts remain.
- Vector-unit inline assembly matching upstream/retail remains. SDK source migrations and tool changes inherited from fakegenie's original PR have not been cleaned or removed; their scope still needs the maintainer's decision.
- This cleanup did not edit `ps2/src/dng_main.cpp` or `ps2/include/dng_main.hpp`: both are byte-for-byte the original PR head's versions. The PR's own differences from upstream are inherited and still need reconciliation with Plarpoon.
- The newer author update has not been reconciled, and full-branch final review is outstanding. This checkpoint is ready for takeover, not a claim that the complete migration is ready to merge.

## Taking over

```sh
git clone --branch handoff/pr7-cleanup-20261006 https://github.com/carrdylan52/ChronicleTwo.git
cd ChronicleTwo
git submodule update --init --recursive
```

Use your own checksum-matching PAL executable in the repository's expected ROM path and run the repository's build workflow. ROMs, generated binaries, logs and private orchestration files are not included in the branch.

Start by comparing this checkpoint with the current author head, then decide which pending cleanup candidates to import. The per-unit source notes describe retained compiler forms and hand-backs. The original migration is credited to Fake Genie; preserve that credit if you squash or re-land this branch.

## Cleaned files

`actscript`, `automap`, `charasetup`, `collision`, `colprim`, `dataread`, `dbg_font`, `dng_debug`, `dng_effect`, `dng_event`, `dng_hud`, `dng_object`, `dng_status`, `dngfloor`, `dngmenu`, `editctrl`, `editdata`, `editinfo`, `editloop`, `editmap`, `editmap2`, `editparts`, `editriver`, `effect`, `effectlist`, `effscript`, `event_func`, `fishing`, `fishingobj`, `gamedata`, `helpmes`, `intersection`, `inventmn`, `mainloop`, `maintex`, `map`, `mapinfo`, `mapjump`, `mapload`, `mapparts`, `mapselect`, `mapsky`, `mdslist`, `memcard`, `menuaqua`, `menucapt`, `menuchr`, `menucls1`, `menucommon`, `menudraw`, `menumain`, `menumap`, `menuop`, `menushop`, `menusys`, `menusystemdata`, `mg_camera`, `mg_drawenv`, `mg_drawprim`, `mg_frame`, `mg_math`, `mg_memory`, `mg_shadow`, `mg_sprite`, `mg_visual`, `nd_meswin`, `npccfg`, `object`, `occlusion`, `outline`, `password`, `photo`, `quest`, `runscript`, `runscript_opcodes`, `savedata`, `savedatadungeon`, `scene`, `sceneevent`, `sceneload`, `sceneseq`, `scenevillager`, `sysmes`, `userdata`, `water`.

## Formerly perfect entries now assembly/unscored

| File | Symbol | New progress state |
|---|---|---|
| `actscript` | `_SET_ACCUME_FLAG__FP12RS_STACKDATAi` | asm |
| `automap` | `CreatTermParts__11CAutoMapGenFv` | asm |
| `dng_debug` | `DBGCMD_ReloadEnemy__Fii` | asm |
| `dng_debug` | `dngDebugDraw__Fv` | asm |
| `dng_effect` | `Draw__18CMapEffectsManegerFP9mgCCamera` | asm |
| `dng_event` | `Switch__20CStartupEpisodeTitleFi` | asm |
| `dngmenu` | `Draw__11CDngFreeMapFv` | asm |
| `dngmenu` | `GetEntranceRoomGlid__11CDngFreeMapFv` | asm |
| `dngmenu` | `Init__6ClsMesFv` | asm |
| `dngmenu` | `Set__9mgRect_f_Fffff` | asm |
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
| `menusys` | `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | asm |
| `menusys` | `menu_inputkey_limmit_check_line__12CMenuKeyFuncFi` | asm |
| `mg_drawenv` | `GetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT` | asm |
| `mg_drawenv` | `SetPlight__13mgRENDER_INFOFiP13mgPOINT_LIGHT` | asm |
| `mg_drawprim` | `Draw__14mgCDrawManagerFiP13sceVif1Packet` | asm |
| `mg_drawprim` | `Texture__11mgCDrawPrimFP10mgCTexture` | asm |
| `mg_visual` | `CopyMaterial__FP10mgMaterialP13MDT_MATERIAL_P17mgCTextureManager` | asm |
| `mg_visual` | `Copy__15mgCVisualFixMDTFP9mgCMemory` | asm |
| `mg_visual` | `CreatePacket__12mgCVisualMDTFP14mgCDrawManager` | asm |
| `mg_visual` | `CreatePacket__15mgCVisualFixMDTFP14mgCDrawManager` | asm |
| `runscript_opcodes` | `_GET_INDEXOBJ_SIZE__FP12RS_STACKDATAi` | asm |
| `runscript_opcodes` | `_SET_INDEXOBJ_SIZE__FP12RS_STACKDATAi` | asm |
| `scene` | `ClearStack__6CSceneFi` | asm |
| `scenevillager` | `CharaObjectOnOff__6CSceneFiP9mgCMemory` | asm |
| `userdata` | `GetNumSameItem__16CUserDataManagerFi` | asm |
| `userdata` | `LeaveMonicaItemCheck__Fv` | asm |

Treat this record as a checkpoint to verify against the branch and current PR state before changing it.
