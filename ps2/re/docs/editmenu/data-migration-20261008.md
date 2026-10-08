# editmenu data migration (October 8, 2026)

Baseline: `63f7a9e5`, 153 `INCLUDE_RODATA`, 101 `INCLUDE_BSS`,
68/6,092 matched initialized data bytes. All 61 functions already match.
Public header declarations and layouts remain source-compatible.

## Named uninitialized state

The named menu-state reservations are typed C++ definitions. Types follow
the existing declarations and documented layouts, with array sizes corrected
against retail symbols and their actual accesses. `HouseChildPartInfo` has
22 pointers (house/resident plus up to twenty children), `DownLoadMes` has six
pointers, `GeoBoardListTitlePutOffset` has five pairs, and
`GeoramaMesMakeLine` has five shorts. Padding belongs to section pieces, not
to those arrays. `GeoramaPenkiNum` retains its declared 16-short object;
only the first eight entries are consumed as paint quantities here.

Most definitions have internal linkage. `HouseChildPartInfo` and
`PartsMakeOkTable` retain external linkage because the generated whole-unit
`ps2/asm/pal/data/main/vutext.data.s` contains cross-unit relocations naming
them. Making either static fails the link with `Symbol not found`; these
references belong to an unowned generated unit, so this lane preserves them
without editing generated data or linker tooling. Their object sizes are
still the exact retail 0x58 and 0x400 bytes.

The cached blank names and initialization flags retain their existing
once-only setup in the matched functions. `cnt_2177` counts the eighty-frame
placed-house arrow-blink cycle; it is not a resident count. No runtime
initialization function or codegen helper is introduced.

| Object | Type and purpose |
| --- | --- |
| `HouseDrawInfo` | `CEditHouse *HouseDrawInfo`: House whose residents and attached parts appear in the placed-house panel. |
| `HouseInfoFormGrobal` | `CMenuPosDataForm *HouseInfoFormGrobal`: Form containing the placed-house information panel. |
| `HousePartsID` | `int HousePartsID`: Placed part identifier of the house shown in the information panel. |
| `HouseInfoSelectLine` | `short HouseInfoSelectLine`: First visible row of the placed-house information list. |
| `HouseInfoSelectSelect` | `short HouseInfoSelectSelect`: Selected row of the placed-house information list. |
| `HouseInfoSelectMoveInit` | `signed char HouseInfoSelectMoveInit`: Whether to snap the placed-house list and cursor to their target positions. |
| `HouseInfoSelectY` | `int HouseInfoSelectY`: Vertical scrolling position of the placed-house information list. |
| `HouseInfoCursorAlphaOnOff` | `int HouseInfoCursorAlphaOnOff`: Whether the placed-house list cursor is visible. |
| `HouseInfoCursorAlpha` | `int HouseInfoCursorAlpha`: Opacity of the placed-house list cursor. |
| `HouseInfoCursorY` | `int HouseInfoCursorY`: Vertical position of the placed-house list cursor. |
| `HouseChildPartInfo` | `char *HouseChildPartInfo[22]`: House name, resident name and names of up to twenty attached parts. |
| `Dmy_2314` | `char *Dmy_2314`: Blank name used for missing placed-house information. |
| `init_2315` | `signed char init_2315`: Whether the blank placed-house name has been initialized. |
| `RemovalMenuPtr` | `CRemovalMenu *RemovalMenuPtr`: Active villager removal menu. |
| `DownLoadInfo` | `DownLoadEntry *DownLoadInfo`: First entry in the Geostone download announcement. |
| `DownLoadInfoNext` | `DownLoadEntry *DownLoadInfoNext`: Next announcement entry waiting for message construction. |
| `DownLoadInfoEndFlag` | `signed char DownLoadInfoEndFlag`: Whether every download entry has been announced. |
| `DownLoadInfoDrawFlag` | `signed char DownLoadInfoDrawFlag`: Whether the download announcement windows are drawn. |
| `DownLoadWinRect` | `DownLoadRect DownLoadWinRect`: Screen rectangle used by the download announcement windows. |
| `DownLoadDispNum` | `short DownLoadDispNum`: Number of entries displayed by the download announcement. |
| `DownLoadProgress` | `short DownLoadProgress`: Current progress of the download announcement. |
| `DownLoadMesMakeProgress` | `signed char DownLoadMesMakeProgress`: Progress state for constructing the next announcement message. |
| `DownLoadMesAlpha` | `int DownLoadMesAlpha`: Opacity of the download announcement message windows. |
| `DownLoadMesMakeNo` | `signed char DownLoadMesMakeNo`: Announcement message window selected for construction. |
| `DownLoadActiveMes` | `ClsMes *DownLoadActiveMes`: Message window currently revealing a download entry. |
| `DownLoadMesUpY` | `short DownLoadMesUpY`: Upward scrolling offset of announcement message windows. |
| `DownLoadMes` | `CDC2Mes *DownLoadMes[6]`: Message windows available to the Geostone download announcement. |
| `MenuGeoStoneDonwLoadFlag` | `signed char MenuGeoStoneDonwLoadFlag`: Whether a Geostone download is active in the Georama menu. |
| `MenuGeoStoneDownLoad_PartsNum` | `u16 MenuGeoStoneDownLoad_PartsNum`: Number of part entries supplied by the Geostone download. |
| `MenuGeoStoneDownLoad_Request` | `u16 MenuGeoStoneDownLoad_Request`: Number of request entries supplied by the Geostone download. |
| `MenuGeoStoneDownLoadTime` | `u32 MenuGeoStoneDownLoadTime`: Total steps represented by the staged Geostone count animation. |
| `MenuGeoStoneDmyCnt` | `GeoStoneDmyCnt *MenuGeoStoneDmyCnt`: First stage of the Geostone count animation. |
| `MenuGeoStoneDmyCnt_Now` | `GeoStoneDmyCnt *MenuGeoStoneDmyCnt_Now`: Current stage of the Geostone count animation. |
| `PartsMakeOkTableNum` | `int PartsMakeOkTableNum`: Number of part definitions already available for construction. |
| `PartsMakeOkTable` | `int PartsMakeOkTable[256]`: Part definition identifiers already available for construction. |
| `old_menuparts_pos_flag` | `u8 old_menuparts_pos_flag`: Whether the original displayed part position and rotation have been saved. |
| `NowPolyGonFormMoveFlag` | `u8 NowPolyGonFormMoveFlag`: Whether the displayed model form should snap to its target position. |
| `MenuMapPart` | `CMapParts *MenuMapPart`: Map part currently shown by the Georama menu. |
| `MenuGeoramaSystemData` | `MenuGeoramaSystemInfo *MenuGeoramaSystemData`: Saved selections and scrolling positions for the Georama menu lists. |
| `MenuGeoramaCursorForceSetFlag` | `u8 MenuGeoramaCursorForceSetFlag`: Whether the menu cursor must move immediately to its target. |
| `GeoRequestFlag` | `GeoRequestCheck *GeoRequestFlag`: Results of checking each Georama request condition. |
| `MenuPartsDrawStack` | `mgCMemory *MenuPartsDrawStack`: Memory stack used for the displayed Georama part. |
| `MenuMainMapInfo` | `CEditMap *MenuMainMapInfo`: Town edit map inspected by the Georama menu. |
| `CMenuGeoPt` | `CMenuGeorama *CMenuGeoPt`: Active Georama menu. |
| `MenuGeoramaViewNowPicNo` | `short MenuGeoramaViewNowPicNo`: Photograph selected for the Georama wall-picture preview. |
| `MenuGeoramaViewWallPic` | `mgCTexture *MenuGeoramaViewWallPic`: Texture of the selected Georama wall photograph. |
| `MenuEditAnalyzeSrc` | `EditAnalyzeSrc *MenuEditAnalyzeSrc`: Request source currently examined by the Georama analysis menu. |
| `MenuEditAnalyzeDataSrcNum` | `short MenuEditAnalyzeDataSrcNum`: Number of request entries in the Georama analysis list. |
| `MenuEditAnalyzeDataSrcListH` | `float MenuEditAnalyzeDataSrcListH`: Total vertical extent of the Georama analysis list. |
| `MenuEditAnalyzeDataSrcListH_Move` | `float MenuEditAnalyzeDataSrcListH_Move`: Target vertical position of the Georama analysis list. |
| `MenuEditAnalyzeDataSrcListLimmitNum` | `short MenuEditAnalyzeDataSrcListLimmitNum`: Number of selectable rows within the analysis list scroll limit. |
| `MenuAnalyzeData` | `EditDataAnalyze *MenuAnalyzeData`: Current analysis of the town's Georama state. |
| `GeoRequestBoardCheckPoint_P` | `int GeoRequestBoardCheckPoint_P[2]`: Integer position parameters of the request-board checkpoint indicator. |
| `Tex_Georama` | `mgCTexture *Tex_Georama`: Texture containing the Georama menu artwork. |
| `GeoramaParts_DrawWaitCnt` | `short GeoramaParts_DrawWaitCnt`: Frames to wait before drawing the displayed Georama part. |
| `GeoramaMesPosForceSetFlag` | `u8 GeoramaMesPosForceSetFlag`: Whether Georama message positions must snap to their targets. |
| `GeoramaMesMakeManner` | `signed char GeoramaMesMakeManner[5]`: Scroll direction used to populate each Georama list message. |
| `GeoramaReqMakeLine` | `short GeoramaReqMakeLine`: First request line used to populate the analysis message. |
| `GeoramaReqMakeManner` | `short GeoramaReqMakeManner`: Scroll direction used to populate the analysis message. |
| `GeoramaMesForceMakeFlag` | `u8 GeoramaMesForceMakeFlag`: Whether Georama list messages need immediate reconstruction. |
| `GeoramaMesForceMakeFlag_PaintVer` | `u8 GeoramaMesForceMakeFlag_PaintVer`: Whether the paint-list message needs reconstruction. |
| `GeoAnalyzeCheckPointScrlBarY` | `float GeoAnalyzeCheckPointScrlBarY`: Vertical scroll-bar position for the Georama analysis list. |
| `GeoAlpha_1199` | `int GeoAlpha_1199`: Fade opacity of the Georama model preview while the menu closes. |
| `init_1200` | `signed char init_1200`: Whether the Georama model-preview opacity has been initialized. |
| `menu_georama_title_pos` | `float menu_georama_title_pos[2]`: Screen position of the Georama menu title. |
| `GeoramaPenkiNum` | `short GeoramaPenkiNum[16]`: Cached quantities for the paint items displayed by the Georama menu. |
| `MenuEditAnalyzeDataSrcListHTable` | `float MenuEditAnalyzeDataSrcListHTable[16]`: Vertical starting position of each Georama analysis entry. |
| `MenuEditAnalyzeDataSrc` | `EditAnalyzeDataSrc *MenuEditAnalyzeDataSrc[32]`: Request data entries shown by the Georama analysis menu. |
| `GeoBoardListTitleTexRect` | `float GeoBoardListTitleTexRect[5][4]`: Texture rectangles of the Georama list titles. |
| `GeoBoardListTitlePutOffset` | `int GeoBoardListTitlePutOffset[5][2]`: Screen-position offsets of the Georama list titles. |
| `GeoRequestBoardCheckPoint` | `float GeoRequestBoardCheckPoint[4]`: Texture rectangle of the request-board checkpoint indicator. |
| `GeoramaMes` | `CDC2Mes *GeoramaMes[5]`: Message windows used by the Georama lists. |
| `GeoramaMesMakeLine` | `short GeoramaMesMakeLine[5]`: First source line used to populate each Georama list message. |
| `GeoramaReqMsgFont` | `CFont *GeoramaReqMsgFont[48]`: Font objects containing the individual Georama request messages. |
| `GeoramaReqMsgFontGyouNum` | `signed char GeoramaReqMsgFontGyouNum[48]`: Number of text lines in each request message font object. |
| `GeoramaReqMsgTexH` | `short GeoramaReqMsgTexH[48]`: Texture height reserved for each Georama request message. |
| `GeoramaReqMsgFontDrawFlag` | `signed char GeoramaReqMsgFontDrawFlag[48]`: Whether each request message font object should be drawn. |
| `cnt_2177` | `signed char cnt_2177`: Frame counter of the placed-house scroll-arrow blinking animation. |
| `init_2178` | `signed char init_2178`: Whether the placed-house scroll-arrow blink counter has been initialized. |
| `dmychar_3207` | `char *dmychar_3207`: Blank display name used for unused Georama list rows. |
| `init_3208` | `signed char init_3208`: Whether the blank Georama list name has been initialized. |
| `edparts_info_3580` | `CEditPartsInfo *edparts_info_3580`: Definition of the placed Georama part selected for removal. |
| `init_3581` | `signed char init_3581`: Whether the cached removal part definition has been initialized. |
| `DestroyMaxNum_3584` | `short DestroyMaxNum_3584`: Maximum quantity of the selected Georama part that can be removed. |
| `init_3585` | `signed char init_3585`: Whether the cached removal quantity limit has been initialized. |

Named-state checkpoint: 153 `INCLUDE_RODATA`, 16 `INCLUDE_BSS`, and
68/6,092 matched_data after the prescribed refresh. The other 85 BSS markers
are removed. `.private/dataB/receipts/editmenu-named-bss-build.log` records
PAL OK; `editmenu-named-bss-objects.log` records 149/149; the associated
changed-object receipt contains only this lane's two owned units. The other
147 object file hashes equal the warm-built baseline. The header remains
unchanged in this checkpoint. All 61 functions remain exact.
