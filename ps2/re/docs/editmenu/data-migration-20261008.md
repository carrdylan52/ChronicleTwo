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

## Model vectors and paint palette

`old_menuparts_pos`, `old_menuparts_rot`, `now_menu_pos_mapparts` and
`georama_adjust_position` are four-float initialized vectors. They retain
zero components in initialized data, rather than BSS. `GeoramaColorList`
contains eight RGB paint colours followed by the negative unpainted sentinel.
The 93-entry `georama_parts_adjust_scaletable` and
`georama_parts_adjust_z_table` arrays supply per-definition model preview
scales and depth offsets. Their decimal float literals round-trip to the
retail binary32 values, including non-integral scale constants.

Each vector/table was migrated and checked separately. Receipts are under
`.private/dataB/receipts/editmenu-{original-position,original-rotation,
current-part-position,preview-offset,paint-colours,preview-scales,
preview-depths}-*`. Every check reports PAL OK and 149/149 objects, with
only the two owned unit hashes differing from baseline. This checkpoint
removes seven initialized-data markers (146 remain).

## Board and list tables

The paint item identifiers, view-to-list indices, active/inactive board and
scrollbar texture rectangles, language-specific analysis/house title offsets,
resident labels, message-column positions and analysis topic-button origins
now have typed initialized definitions. The existing drawing APIs consume
flat short rectangle tables, so these retain their declared flat interface;
two-dimensional tables retain rows with their actual dimensions.
`offset_2176` contains seven language entries, not the eight entries in its
former extern. `postbl_2175` has eight texture rectangles, including the
extra retail row. `tbl_957` retains -1 for views without a part list.

Each table was migrated separately and passed PAL and all object checks.
Receipts: `editmenu-paint-items-*`, `editmenu-list-indexes-*`,
`editmenu-active-board-*`, and `editmenu-table-<symbol>-*` beneath
`.private/dataB/receipts/`. All other 147 object hashes remain identical.
This group removes fourteen more markers (132 initialized markers remain).

## Inline strings: initialization and drawing

Form/table names, debug scripts, menu textures, the download blank name,
and placed-house/message blank names are inline literals at their use sites.
Shared literal markers disappear only after their last extern reference is
replaced. Every byte above ASCII is encoded as a three-digit octal escape,
so Shift-JIS strings are preserved independently of the editor encoding.

The ten function-sized steps from SetEditMenuEnv through
MenuGeoramaMessageMake each pass the full PAL and 149-unit checks. Receipts
are `editmenu-literals-<function>-*` in `.private/dataB/receipts/`.
This checkpoint has 118 initialized markers and 16 BSS markers.

## Inline strings: Georama class methods

The eight function-sized steps from InitEnd through CalcTex replace native
pack member names, Georama form and part names, cursor actions, build-dialog
scripts and formatted colour-cell names with inline strings. File traversal
still uses GetPackFile's typed serialized-buffer API. Literal contents,
including the Japanese house-information form name, are byte-identical.

Each step passes PAL and all 149 objects, with the other 147 object hashes
unchanged. Receipts are `editmenu-literals-CMenuGeorama-<method>-*` and
`editmenu-literals-MakeMsgPartsItemInfo-*`. Remaining initialized markers: 86.

## Inline strings: Georama input handlers

Base navigation, part placement/removal, part construction, placed-house
inspection, paint selection and input dispatch now use their inline script
actions and diagnostic messages. The six independent function steps from
MenuGeoramaBasePush through MenuGeoramaPushKey retain zero object differences.
The exact Japanese diagnostic bytes remain octal escaped.

Receipts: `editmenu-literals-MenuGeorama<handler>-*`; PAL and all 149 object
checks pass, and the other 147 object hashes remain unchanged. Remaining
initialized markers: 59.

## Inline strings: villager removal

KeyStep's villager-removal form/part names, script actions, pack members,
formatted colour cells and cursor/list templates are inline strings. The
removal initializer's background and form filenames are also inline.
`editmenu-literals-CRemovalMenu-KeyStep-*` and
`editmenu-literals-MenuRemovalInit-*` record PAL OK and 149/149 objects,
with all other 147 hashes unchanged. No string extern remains. The two
file-list string markers remain until their owning pointer tables migrate.
Twenty-two initialized markers remain at this checkpoint.

## Natural local aggregates

MenuGeoramaMakePush initializes an aligned `sceVu0FVECTOR` at the river
query with `{0,0,0,-1}`. Its compiler-created initialized template replaces
`at_3757`; the former float/quadword overlay type is unnecessary. Both
IsMakeObject item-name arrays use `{make_parts->edit_name}` directly instead
of loading a float through a cast into pointer storage. Their generated zero
templates still need the two anonymous BSS markers until tooling can name
them. DrawDownLoadAnaunce uses the existing `RGBAQ_TYPE` aggregate directly
for its two window colours, removing the redundant colour overlay union.
The redundant MakeBoardDrawInfo extern is removed; menudraw.hpp owns its
existing declaration.

The three function changes each pass complete object and PAL checks. The
combined cleanup passes 149/149, with all other 147 hashes unchanged:
`editmenu-{river-vector,item-arrays,window-colours,natural-aggregates}-*`.
Twenty-one initialized markers and sixteen BSS markers remain here.
