# Header-defined constructor identity evidence

Captured at 2026-10-08T21:42:55.404062+00:00. This audit only reads census/trace evidence and allowed current source; writes are private notes and this driver. No builds, containers, tracked edits or image operations.

The conservative allowlist contains **46 exact identities**, all copied from force-new-all compiler traces and joined to actual header constructor bodies. 36 had original inline class 6; 31 appeared in an active construction walk. The pn4 probe should still require the direct scalar placement allocator/constructor and original class 6. The JSON `allowlist` is ready for exact-string membership; it does not encode per-site overrides.

The census-allocated-owner subset is `allowlist_census_allocated_types`; member/base-only owners are included in the full header-body identity set and identified separately below. New-depth observations from the earlier broad hook cannot establish direct-constructor ownership because it also walks nested construction. Trace caller labels are a known stale backend context and are not used as evidence of the caller.

A header class definition or constructor declaration is insufficient. `CMapParts` is excluded: its declaration is in `mapparts.hpp`, while the known bodies are in `editeff.cpp` and `editmap.cpp`. Other inline cpp bodies and in-class cpp bodies are excluded too. Implicit constructors are unresolved; the audit invents no identity for them.

## Counts

| Population | Count |
| --- | ---: |
| Retail scalar placement sites | 216 |
| Distinct census allocated types | 99 |
| Exact traced constructor identities | 56 |
| Included explicit header bodies | 46 |
| Included and original class 6 | 36 |
| Included owners present as census allocated types | 33 |
| Excluded traced cpp constructor identities | 10 |
| Unknown traced constructor identities | 0 |

## Observed constructor identities

| Exact compiler identity | Decision | Original class | Census sites / walk depth | Definition evidence |
| --- | --- | --- | --- | --- |
| `__ct__10CCollisionFv` | included | 6 | 1 / 0 | `ps2/include/collision.hpp:77` |
| `__ct__10CDAColPipeFv` | included | 6 | 1 / 0,1 | `ps2/include/dynamicanime.hpp:166` |
| `__ct__10CFuncPointFv` | included | 6 | 0 / 0 | `ps2/include/mapload.hpp:269` |
| `__ct__10TITLE_INFOFv` | included | 3 | 1 / 0,1 | `ps2/include/title.hpp:268` |
| `__ct__11CCharacter2Fv` | included | 6 | 26 / 0,1 | `ps2/include/character.hpp:438` |
| `__ct__11CDngFreeMapFv` | included | 3 | 1 / 0,1 | `ps2/include/dngmenu.hpp:144` |
| `__ct__11CItemSelectFv` | excluded | 6 | 1 / 0,1 | `ps2/src/menusys.cpp:11535` |
| `__ct__11CManualMenuFv` | included | 6 | 1 / 0,1 | `ps2/include/menuop.hpp:185` |
| `__ct__11CMenuInventFv` | excluded | 3 | 1 / 0,1 | `ps2/src/inventmn.cpp:5781` |
| `__ct__11CMenuOptionFv` | included | 3 | 1 / 0,1 | `ps2/include/menuop.hpp:255` |
| `__ct__11CWaterFrameFv` | included | 6 | 1 / 0,1 | `ps2/include/water.hpp:283` |
| `__ct__11mgC3DSpriteFv` | included | 6 | 1 / 0,1 | `ps2/include/mg_sprite.hpp:216` |
| `__ct__12CActionCharaFv` | included | 6 | 16 / 0,1 | `ps2/include/actionchara.hpp:337` |
| `__ct__12CDACollisionFv` | included | 6 | 0 / 0 | `ps2/include/dynamicanime.hpp:125` |
| `__ct__12CMenuGeoramaFv` | included | 3 | 1 / 0,1 | `ps2/include/editmenu.hpp:211` |
| `__ct__12CMenuKeyFuncFv` | included | 6 | 1 / 0,1 | `ps2/include/menusys.hpp:690` |
| `__ct__12CMenuTreeMapFv` | excluded | 3 | 1 / 0,1 | `ps2/src/dngmenu.cpp:2576` |
| `__ct__12CMosBookMenuFv` | included | 3 | 1 / 0,1 | `ps2/include/menuchr.hpp:614` |
| `__ct__12CObjectFrameFv` | included | 6 | 0 / 0 | `ps2/include/object.hpp:32` |
| `__ct__12CRemovalMenuFv` | included | 3 | 1 / 0,1 | `ps2/include/editmenu.hpp:501` |
| `__ct__12mgCVisualMDTFv` | included | 6 | 1 / 0,1 | `ps2/include/mg_visual.hpp:163` |
| `__ct__13CCollisionMDTFv` | included | 6 | 1 / 0,1 | `ps2/include/collision.hpp:174` |
| `__ct__13CDC2AlbumDataFv` | included | 6 | 1 / 0,1 | `ps2/include/inventmn.hpp:187` |
| `__ct__13CMenuMoveItemFv` | included | 3 | 3 / 0,1 | `ps2/include/menucls1.hpp:433` |
| `__ct__13CNameRegiMenuFv` | excluded | 3 | 1 / 0,1 | `ps2/src/nameregi.cpp:499` |
| `__ct__13CWorldMapMenuFv` | excluded | 3 | 1 / 0,1 | `ps2/src/menumap.cpp:895` |
| `__ct__13DownLoadEntryFv` | excluded | 6 | 2 / 0,1 | `ps2/src/editmenu.cpp:141` |
| `__ct__13MENU_DRAW_ENVFv` | included | 6 | 1 / 0,1 | `ps2/include/menumain.hpp:217` |
| `__ct__14CFuncPointMngrFv` | included | 6 | 0 / 0 | `ps2/include/funcpoint.hpp:215` |
| `__ct__14CMenuMosSelectFv` | excluded | 3 | 1 / 0,1 | `ps2/src/menuchr.cpp:3838` |
| `__ct__14CSaveMenuClassFv` | included | 3 | 1 / 0,1 | `ps2/include/menuop.hpp:366` |
| `__ct__14GeoStoneDmyCntFv` | excluded | 6 | 2 / 0,1 | `ps2/src/editmenu.cpp:178` |
| `__ct__14PartsGroupDataFv` | included | 6 | 0 / 0 | `ps2/include/map.hpp:110` |
| `__ct__15CFuncPointCheckFv` | included | 6 | 0 / 0 | `ps2/include/funcpoint.hpp:89` |
| `__ct__15CMenuChrCngMenuFv` | excluded | 6 | 1 / 0,1 | `ps2/src/menuchr.cpp:45` |
| `__ct__15CMenuCostumeSelFv` | included | 6 | 1 / 0,1 | `ps2/include/menuchr.hpp:487` |
| `__ct__15mgCVisualFixMDTFv` | included | 6 | 2 / 0,1 | `ps2/include/mg_visual.hpp:390` |
| `__ct__16CEffectScriptManFv` | included | 6 | 4 / 0,1 | `ps2/include/effscript.hpp:215` |
| `__ct__17CSWordAfterEffectFv` | included | 6 | 4 / 0,1 | `ps2/include/swordeffect.hpp:78` |
| `__ct__18CList<P9CMapParts>Fv` | included | 6 | 1 / 0,1 | `ps2/include/mg_tanime.hpp:183` |
| `__ct__18mgCVisualMotionMDTFv` | included | 6 | 2 / 0,1 | `ps2/include/visualmotion.hpp:81` |
| `__ct__19CList<10CFuncPoint>Fv` | included | 6 | 1 / 0,1 | `ps2/include/mg_tanime.hpp:183` |
| `__ct__23CList<14PartsGroupData>Fv` | included | 6 | 1 / 0,1 | `ps2/include/mg_tanime.hpp:183` |
| `__ct__24CList<15mgCTexAnimeData>Fv` | included | 6 | 1 / 0,1 | `ps2/include/mg_tanime.hpp:183` |
| `__ct__7CObjectFv` | included | 6 | 0 / 0 | `ps2/include/map.hpp:212` |
| `__ct__8CEditMapFv` | included | 3 | 1 / 0,1 | `ps2/include/editmap.hpp:148` |
| `__ct__9CMapPartsFv` | excluded | 6 | 1 / 0,1 | `ps2/src/editeff.cpp:513`, `ps2/src/editmap.cpp:312` |
| `__ct__9CMapPieceFv` | included | 6 | 3 / 0,1 | `ps2/include/mdslist.hpp:296` |
| `__ct__9CShopMenuFv` | included | 3 | 1 / 0,1 | `ps2/include/menushop.hpp:289` |
| `__ct__9mgCMemoryFv` | included | 6 | 1 / 0 | `ps2/include/mg_memory.hpp:71` |
| `__ct__9mgCObjectFv` | included | 6 | 0 / 0 | `ps2/include/mg_frame.hpp:298` |
| `__ct__9mgCVisualFv` | included | 6 | 0 / 0 | `ps2/include/mg_dataset.hpp:214` |
| `__ct__9mgRect<f>Fffff` | included | 6 | 0 / 0 | `ps2/include/mg_tanime.hpp:88` |
| `__ct__9mgRect<f>Fv` | included | 6 | 0 / 0 | `ps2/include/mg_tanime.hpp:151` |
| `__ct__9mgRect<i>Fiiii` | included | 6 | 0 / 0 | `ps2/include/mg_tanime.hpp:88` |
| `__ct__9mgRect<i>Fv` | included | 6 | 0 / 0 | `ps2/include/mg_tanime.hpp:137` |

All included bodies were checked by exact source text against current files. JSON stores full body text, current file hashes, and each raw trace line/path/line number. Class 3 header constructors stay listed as header-defined but are filtered by the original-class-6 hook condition. Four mgRect identities use their actual header primary-template or default specialization bodies; none is a census scalar allocated type.

## Exhaustive allocation-type decisions

| Allocated type | Decision | Sites | Construction modes | Observed identities | Body/declaration/type evidence |
| --- | --- | ---: | --- | --- | --- |
| `ARG_LIST` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/event_func.hpp:683` |
| `CActionChara` | included | 16 | inline (16) | `__ct__12CActionCharaFv` | `ps2/include/actionchara.hpp:337` |
| `CAquaFish` | excluded | 1 | out of line (1) |  | `ps2/include/menuaqua.hpp:312` |
| `CAquaFishEff` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/menuaqua.hpp:482` |
| `CBubble` | unknown | 2 | trivial; no implicit construction guard (2) |  | `ps2/include/menuaqua.hpp:116` |
| `CCameraControl` | excluded | 5 | out of line (5) |  | `ps2/include/cameracontrol.hpp:143` |
| `CCharacter2` | included | 26 | inline (26) | `__ct__11CCharacter2Fv` | `ps2/include/character.hpp:438` |
| `CColFrame` | excluded | 1 | out of line (1) |  | `ps2/include/collision.hpp:256` |
| `CCollision` | included | 1 | out of line (1) | `__ct__10CCollisionFv` | `ps2/include/collision.hpp:77` |
| `CCollisionMDT` | included | 1 | inline (1) | `__ct__13CCollisionMDTFv` | `ps2/include/collision.hpp:174` |
| `CDAColPipe` | included | 1 | inline (1) | `__ct__10CDAColPipeFv` | `ps2/include/dynamicanime.hpp:166` |
| `CDC2AlbumData` | included | 1 | inline (1) | `__ct__13CDC2AlbumDataFv` | `ps2/include/inventmn.hpp:187` |
| `CDC2Mes` | excluded | 17 | out of line (17) |  | `ps2/include/menucls1.hpp:91` |
| `CDngFreeMap` | included | 1 | inline (1) | `__ct__11CDngFreeMapFv` | `ps2/include/dngmenu.hpp:144` |
| `CEditGrid` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/editriver.hpp:70` |
| `CEditInfoMngr` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/editinfo.hpp:50` |
| `CEditMap` | included | 1 | inline (1) | `__ct__8CEditMapFv` | `ps2/include/editmap.hpp:148` |
| `CEffectScriptMan` | included | 4 | inline (4) | `__ct__16CEffectScriptManFv` | `ps2/include/effscript.hpp:215` |
| `CFishFood` | excluded | 1 | out of line (1) |  | `ps2/include/menuaqua.hpp:556` |
| `CItemSelect` | excluded | 1 | inline (1) | `__ct__11CItemSelectFv` | `ps2/src/menusys.cpp:11535` |
| `CList<CFuncPoint>` | included | 1 | inline (1) | `__ct__19CList<10CFuncPoint>Fv` | `ps2/include/mg_tanime.hpp:183` |
| `CList<CMapParts *>` | included | 1 | inline (1) | `__ct__18CList<P9CMapParts>Fv` | `ps2/include/mg_tanime.hpp:183` |
| `CList<CMapParts>` | unknown | 1 | out of line (1) |  | `ps2/include/mg_tanime.hpp:183` |
| `CList<CMapPiece>` | unknown | 2 | out of line (1), inline (1) |  | `ps2/include/mg_tanime.hpp:183` |
| `CList<CObjAnime>` | unknown | 1 | inline (1) |  | `ps2/include/mg_tanime.hpp:183` |
| `CList<PartsGroupData>` | included | 1 | inline (1) | `__ct__23CList<14PartsGroupData>Fv` | `ps2/include/mg_tanime.hpp:183` |
| `CList<mgCTexAnimeData>` | included | 1 | inline (1) | `__ct__24CList<15mgCTexAnimeData>Fv` | `ps2/include/mg_tanime.hpp:183` |
| `CManualMenu` | included | 1 | inline (1) | `__ct__11CManualMenuFv` | `ps2/include/menuop.hpp:185` |
| `CMapParts` | excluded | 1 | inline (1) | `__ct__9CMapPartsFv` | `ps2/src/editeff.cpp:513`, `ps2/src/editmap.cpp:312` |
| `CMapPiece` | included | 3 | inline (3) | `__ct__9CMapPieceFv` | `ps2/include/mdslist.hpp:296` |
| `CMapSky` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/mapsky.hpp:77` |
| `CMapTreasureBox` | excluded | 1 | inline (1) |  | `ps2/include/mapparts.hpp:534` |
| `CMemoryCardManager` | excluded | 4 | out of line (4) |  | `ps2/include/memcard.hpp:285` |
| `CMenuChrCngMenu` | excluded | 1 | inline (1) | `__ct__15CMenuChrCngMenuFv` | `ps2/src/menuchr.cpp:45` |
| `CMenuCostumeSel` | included | 1 | inline (1) | `__ct__15CMenuCostumeSelFv` | `ps2/include/menuchr.hpp:487` |
| `CMenuEffect` | unknown | 4 | not invoked by allocation (4) |  | `ps2/include/menudraw.hpp:1476` |
| `CMenuFont` | excluded | 2 | out of line (2) |  | `ps2/include/menucls1.hpp:57` |
| `CMenuGeorama` | included | 1 | inline (1) | `__ct__12CMenuGeoramaFv` | `ps2/include/editmenu.hpp:211` |
| `CMenuInvent` | excluded | 1 | inline (1) | `__ct__11CMenuInventFv` | `ps2/src/inventmn.cpp:5781` |
| `CMenuKeyFunc` | included | 1 | inline (1) | `__ct__12CMenuKeyFuncFv` | `ps2/include/menusys.hpp:690` |
| `CMenuMosSelect` | excluded | 1 | inline (1) | `__ct__14CMenuMosSelectFv` | `ps2/src/menuchr.cpp:3838` |
| `CMenuMoveItem` | included | 3 | inline (3) | `__ct__13CMenuMoveItemFv` | `ps2/include/menucls1.hpp:433` |
| `CMenuOption` | included | 1 | inline (1) | `__ct__11CMenuOptionFv` | `ps2/include/menuop.hpp:255` |
| `CMenuPosDataManage` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/menudraw.hpp:908` |
| `CMenuQuestView` | unknown | 1 | inline (1) |  | `ps2/include/menushop.hpp:421` |
| `CMenuTreeMap` | excluded | 1 | inline (1) | `__ct__12CMenuTreeMapFv` | `ps2/src/dngmenu.cpp:2576` |
| `CMonsterMan` | unknown | 1 | inline (1) |  | `ps2/include/monster.hpp:439` |
| `CMosBookMenu` | included | 1 | inline (1) | `__ct__12CMosBookMenuFv` | `ps2/include/menuchr.hpp:614` |
| `CMovie` | unknown | 3 | not invoked by allocation (1), trivial; no implicit construction guard (2) |  | `ps2/include/movie.hpp:227` |
| `CNameRegiMenu` | excluded | 1 | inline (1) | `__ct__13CNameRegiMenuFv` | `ps2/src/nameregi.cpp:499` |
| `COutLineDraw` | unknown | 2 | not invoked by allocation (2) |  | `ps2/include/outline.hpp:41` |
| `CQuestManager` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/quest.hpp:67` |
| `CRedMarkModel` | unknown | 1 | inline (1) |  | `ps2/include/dng_event.hpp:282` |
| `CRemovalMenu` | included | 1 | inline (1) | `__ct__12CRemovalMenuFv` | `ps2/include/editmenu.hpp:501` |
| `CRepairEffect` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/menudraw.hpp:1014` |
| `CRepairManager` | unknown | 2 | inline (2) |  | `ps2/include/menudraw.hpp:1074` |
| `CSWordAfterEffect` | included | 4 | inline (3), not invoked by allocation (1) | `__ct__17CSWordAfterEffectFv` | `ps2/include/swordeffect.hpp:78` |
| `CSWordAfterImage` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/dng_effect.hpp:868` |
| `CSaveMenuClass` | included | 1 | inline (1) | `__ct__14CSaveMenuClassFv` | `ps2/include/menuop.hpp:366` |
| `CShop` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/menushop.hpp:144` |
| `CShopMenu` | included | 1 | inline (1) | `__ct__9CShopMenuFv` | `ps2/include/menushop.hpp:289` |
| `CSphida` | excluded | 1 | out of line (1) |  | `ps2/include/sphida.hpp:182` |
| `CSubGameData` | excluded | 1 | out of line (1) |  | `ps2/include/savedata.hpp:717` |
| `CTreasureBoxManager` | unknown | 1 | inline (1) |  | `ps2/include/dng_event.hpp:565` |
| `CVillagerPlaceInfo` | excluded | 1 | not invoked by allocation (1) |  | `ps2/include/villagermngr.hpp:117` |
| `CVillagerPlaceInfo::Node` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/villagermngr.hpp:81` |
| `CWater` | excluded | 1 | out of line (1) |  | `ps2/include/water.hpp:228` |
| `CWaterFrame` | included | 1 | inline (1) | `__ct__11CWaterFrameFv` | `ps2/include/water.hpp:283` |
| `CWaveTable` | excluded | 1 | out of line (1) |  | `ps2/include/wavetable.hpp:41` |
| `CWorldMapMenu` | excluded | 1 | inline (1) | `__ct__13CWorldMapMenuFv` | `ps2/src/menumap.cpp:895` |
| `ClsMes` | excluded | 12 | out of line (12) |  | `ps2/include/nd_meswin.hpp:496` |
| `DownLoadEntry` | excluded | 2 | inline (2) | `__ct__13DownLoadEntryFv` | `ps2/src/editmenu.cpp:141` |
| `EFF_SPT_BASE` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/effscript.hpp:86` |
| `GeoRequestCheck` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/src/editmenu.cpp:208` |
| `GeoStoneDmyCnt` | excluded | 2 | inline (2) | `__ct__14GeoStoneDmyCntFv` | `ps2/src/editmenu.cpp:178` |
| `MDT_HEADER` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/mg_dataset.hpp:69` |
| `MENU_DRAW_ENV` | included | 1 | inline (1) | `__ct__13MENU_DRAW_ENVFv` | `ps2/include/menumain.hpp:217` |
| `SAVEDATA_FORMAT` | unknown | 1 | inline (1) |  | `ps2/include/memcard.hpp:189` |
| `SAVE_CONVERT_WORK` | unknown | 1 | inline (1) |  | `ps2/include/convviewlp.hpp:67` |
| `TITLE_INFO` | included | 1 | inline (1) | `__ct__10TITLE_INFOFv` | `ps2/include/title.hpp:268` |
| `TRESURE_BOX_FLOOR_INFO` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/dng_event.hpp:126` |
| `_EFF_SCRIPT` | unknown | 1 | inline (1) |  | `ps2/include/effscript.hpp:152` |
| `mgC3DSprite` | included | 1 | inline (1) | `__ct__11mgC3DSpriteFv` | `ps2/include/mg_sprite.hpp:216` |
| `mgCCamera` | excluded | 1 | out of line (1) |  | `ps2/include/mg_camera.hpp:296` |
| `mgCCameraFollow` | excluded | 3 | out of line (3) |  | `ps2/include/mg_camera.hpp:544` |
| `mgCEnterIMGInfo` | unknown | 1 | not invoked by allocation (1) |  | `ps2/include/mg_texture.hpp:206` |
| `mgCFace` | unknown | 1 | trivial; no implicit construction guard (1) |  | `ps2/include/mg_visual.hpp:67` |
| `mgCFrame` | excluded | 3 | out of line (3) |  | `ps2/include/mg_frame.hpp:462` |
| `mgCFrame::BoundInfo` | unknown | 5 | trivial; no implicit construction guard (2), not invoked by allocation (3) |  | `ps2/include/mg_frame.hpp:357` |
| `mgCFrameAttr` | excluded | 8 | out of line (8) |  | `ps2/include/mg_frame.hpp:121` |
| `mgCMemory` | included | 1 | not invoked by allocation (1) | `__ct__9mgCMemoryFv` | `ps2/include/mg_memory.hpp:71` |
| `mgCShadowFixMDT` | unknown | 1 | inline (1) |  | `ps2/include/mg_shadow.hpp:93` |
| `mgCShadowMDT` | unknown | 1 | inline (1) |  | `ps2/include/mg_shadow.hpp:27` |
| `mgCTexture` | excluded | 4 | out of line (4) |  | `ps2/include/mg_texture.hpp:258` |
| `mgCTextureAnime` | excluded | 1 | out of line (1) |  | `ps2/include/mg_tanime.hpp:323` |
| `mgCVisualFixMDT` | included | 2 | inline (2) | `__ct__15mgCVisualFixMDTFv` | `ps2/include/mg_visual.hpp:390` |
| `mgCVisualMDT` | included | 1 | inline (1) | `__ct__12mgCVisualMDTFv` | `ps2/include/mg_visual.hpp:163` |
| `mgCVisualMotionMDT` | included | 2 | inline (2) | `__ct__18mgCVisualMotionMDTFv` | `ps2/include/visualmotion.hpp:81` |
| `mgFACE_GROUP` | unknown | 2 | not invoked by allocation (2) |  | `ps2/include/mg_visual.hpp:86` |

“Unknown” has two distinct causes, retained in JSON: an explicit header body exists but no compiler inline identity was observed, or the type has no explicit indexed constructor and construction may be implicit/trivial/synthesized. Explicit operator-new sites and out-of-line retail constructor calls are retained in this table; their presence does not imply a pn4-convertible inline new-expression.

Retail constructor calls in JSON are exact assembly observations, but are not used to guess the compiler frontend identity. In particular, the three unobserved CList instantiations remain unresolved despite sharing the generic header body.

## Limitations

This tests a conservative observed-identity subset of the header-defined hypothesis. It cannot establish how unobserved or synthesized constructors would respond to a complete TU/profile setting. PCH/prefix tests previously preserved constructor classes and object bytes; header-body location is a semantic filter for this experiment, not evidence of a compiler-state defect. The protected dng_main source/header was neither read nor edited.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
