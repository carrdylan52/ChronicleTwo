# editdebug notes

Town-building (georama) debug menu and lighting editor. Called from `editloop`: `EditInit` calls
`EditDebugInit`/`InitLightingEdit`; `EditLoop` (only when `DebugFlag != 0`) calls
`EditDebugMode`, `EditDebugLoop(MainScene, &EdDebugInfo)`, `EditDebugStart(0xD7, &MenuBuffer)`
(on pad 0x400) and `LightingEdit(MainScene)` every frame. No first-game counterpart (nothing named
EditDebug/LightingEdit/SubGameInfo in `/home/adubbz/development/chronicle`).

The unit owns no classes (`class_units.tsv` has none). Header declares `EditDebugInfo` and enums.

## Functions
| Function | Binding | Return | Notes |
|---|---|---|---|
| `EditDebugInit()` | global | void | `EditDebugFlag=0; Select=0; EditDebugTexb=-1;` |
| `EditDebugMode()` | global | int | `return EditDebugFlag;` |
| `EditDebugStart(int, mgCMemory*)` | global | void | `buffer->stack_used(0x24)=0; buffer->lock(0x1C)=0; EditDebugFlag=1; EditDebugTexb=texb;` |
| `PrintCursor(char*, int)` | **local** -> static in .cpp | int | two `sprintf` branches for `"->"` and `"  "`; the returned length advances the menu text pointer |
| `EditDebugLoop(CScene*, EditDebugInfo*)` | global | int | 0 while open, 1 after `EditDebugEnd` (pad 0x440 or an action that closes) |
| `EditDebugEnd()` | global | void | same body as `EditDebugInit` |
| `InitLightingEdit()` / `EndLightingEdit()` | global | void | `LEditFlag = 0` |
| `IsLightingEditMode()` | global | int | `return LEditFlag` |
| `LightingEdit(CScene*)` | global | void | opens itself (`LEditFlag=1`) on a pad combination |
| `tagGyoFish(SPI_STACK*, int)` | **local** in retail, globally named in C++ for the assembly data relocation | int (1/0) | script tag handler for `host:gyorace.cfg`; fills `GetOmakeGyoracer2(fish_num)` (+0x10 name strcpy, +0x02 s16, +0x4A u8, +0x26 s16, +0x3E/+0x3C/+0x36/+0x38/+0x3A s16) from args 0,1,2,3,5..9, arg 4 -> `SetOmakeGyoracerTactics(fish_num, v)`; `fish_num++` |
| `LoadGyorace()` | **local** -> static | void | `LoadFile2("host:gyorace.cfg", buf[0x4000], &size, 0)`, `fish_num=0`, builds a 0x10-byte `SPI_TAG_PARAM[]` from `at_1542` (`{"…", tagGyoFish}` + terminator), runs a stack `CScriptInterpreter` |

## Data (all LOCAL in retail -> `static` in the .cpp; no externs in the header)
`.data`: `SelMax` int[3] (0xC) = {7,5,2}; `SelData` int*[3][8] (0x60) = value each entry edits
(page0: `&DebugInfo[0]`, `&EventNo`, `&DebugInfo[2]`, `&DebugInfo[1]`, `&sg_type`, `&DebugInfo[3]`,
`&DebugInfo[4]`; page1: NULL, `&save_no`, `&load_no`, `&condition`, `&map_flag_no`; page2: `&map_jump`);
`SelText` char*[3][8] entry labels; `SelHelp` char*[3][8] help lines (page0 entry1 "○:run ▲:reload" (Shift-JIS),
entry3 "1:sp up 2:col off", rest ""); `LightSel` int[4] current row per lighting page;
`LightListNum` int[4] = {11,8,9,3} rows per lighting page. `.sdata`: `EventNo` int = 100.
`.sbss`: `EditDebugFlag`, `EditDebugTexb` (texture bank, -1 none, 0xD7 from EditLoop), `Select`
(row), `SelTAG` (page, EDIT_DEBUG_PAGE), `sg_type`, `map_jump`, `save_no`, `load_no`, `condition`,
`map_flag_no`, `LEditFlag`, `LightType` (LIGHTING_EDIT_PAGE), `DirLightNo` (0..), `fish_num`; all int.
`.bss at_1237` 0x10 is a function-local static of LightingEdit.

Values edited via `SelData` are clamped 0..99999; pad: 0x8000/0x2000 ±1, 4/8 ±10 (±10000 with
pads 1+2 held), 1/2 ±100; 0x100 next page; 0x4000/0x1000 next/prev row; 0x20 act; 0x10 alt-act.
After editing, `DebugInfo` words are clamped: [0],[2],[3] -> bool, [1] 0..2, [4] 0..1
(`DebugInfo` is a global int[5] at 0x3FAF30 owned elsewhere).

## Enums (values from SelText strings and EditDebugLoop's `SelTAG`/`Select` tests)
- `EDIT_DEBUG_PAGE`: 0 general ("Debug Camera","RunEvent","Georama Debug","CharaMove","SubGame",
  "ParamOff","InventDebug"), 1 edit data ("All Clear","Save File","Load File","Con","Map Flag"),
  2 map ("Map Jump","Load Gyorace"). `SelTAG` wraps after 2.
- Actions on 0x20: page0 row4 -> `sgInitSubGame(sg_type, info)` + `EditDebugEnd`; page0 row1 ->
  `CScene::RunEvent(EventNo, NULL)` (0x10 -> `ReloadMapScript`); page1 row0 `CEditMap::ClearAllParts`,
  row1 save `host0:geo_data/geo%d-%d.edt` (`WriteFile(..., info->edit_data, 0x5510)`), row2 load
  (`LoadFile2` into `info->edit_data`, then `EditDataLoad`), row3 toggle
  `CEditData::dbgSet/GetContintionFlag(edit_data_no, condition)` (0x10 toggles all 0x40), row4
  toggle `CMapFlagData::SetFlag(map_flag_no)`; page2 row0 `info->jump_map_no = map_jump` and close,
  row1 `LoadGyorace()`. Item enums: `EDIT_DEBUG_GENERAL_ITEM`, `EDIT_DEBUG_EDIT_DATA_ITEM`,
  `EDIT_DEBUG_MAP_ITEM` (names derived from the label strings).
- `LIGHTING_EDIT_PAGE` from `at_1231` labels: 0 "BG & AMB", 1 "Dir Light" (prints `DirLightNo`),
  2 "Fog" (NEAR/FAR/R/G/B/MIN/MAX), 3 "File" (save to `%sy:/dc2/build/light_info/%s.lgt`, prefix "host:").

## EditDebugInfo (size 0x3C)
Size from the `EdDebugInfo` global (editloop, 0x1ECDA20, size 0x3C).
- 0x00..0x2F: a `SubGameInfo`. Evidence: `EditDebugLoop` passes the pointer unchanged to
  `sgInitSubGame(int, SubGameInfo*)`, which reads 0x00 and 0x0C..0x2C; `__sinit_editloop_cpp` zeroes
  exactly +0x00,+0x10,+0x14,+0x18,+0x1C,+0x28,+0x2C of `EdDebugInfo`, the same set
  `__sinit_subgame_cpp` zeroes on `GameInfo` -> an inline `SubGameInfo` constructor. +0x00 is a
  `CScene*` (EditInit stores MainScene; fishing derefs it as CScene). EditInit also sets +0x04=0xB9,
  +0x08=0x15, +0x0C=0x9A, +0x20/+0x24 = same register value.
  Declared as a base class (`struct EditDebugInfo : public SubGameInfo`); a first member would
  compile identically. `SubGameInfo` belongs to `subgame`; `subgame.hpp` declares its 0x30-byte
  layout, and the size assertion for `EditDebugInfo` passes in the PS2 build.
- 0x30 `CEditData* edit_data`: set by `EditMapJump` = `CSaveData::GetEditData(n)`; NULL check and
  `dbgGetContintionFlag` this; save/load buffer (0x5510 bytes).
- 0x34 `int edit_data_no`: the `n` above (0..3 by town map), passed to `dbg*ContintionFlag`.
- 0x38 `int jump_map_no`: set from `map_jump`; EditInit sets -1; EditLoop calls
  `EditMapJump(jump_map_no)` when >= 0 and resets to -1.

## Earlier draft snapshot
All ten previously assembly-only named functions have C++ implementations.
`EditDebugInit`, `EditDebugMode`, `EditDebugStart`, `PrintCursor`,
`InitLightingEdit`, and `IsLightingEditMode` pass isolated linked-image
verification and are promoted. The four other drafts compile behind
`NONMATCHING`; their retail assembly remains the default. Their isolated
differences are `EditDebugLoop` 649/692 words, `LightingEdit` 1314/1356 words,
`tagGyoFish` 56/72 words, and `LoadGyorace` 2/32 words. Each received one
promotion attempt. The `tagGyoFish` attempt initially failed to link because
its C++ declaration was file-static while the retained `at_1542` assembly table
relocates against the named symbol. The draft now has external C++ linkage,
but the ledger prevents a second attempt this pass. m2c could not resolve the
`LightingEdit` jump table; its existing Ghidra export and the retail assembly
were used to establish the page and control flow after the m2c pass.

## LightingEdit on the 73f8e75 merged base

`LightingEdit__FP6CScene` is the unit's only guarded function. With the pinned
Satan's Fiddle profile, the entry draft has 46 differing words out of 1,356
(the plain-wibo draft helper reports 60). Four named projected endpoint arrays
and a two-dimension coordinate adjustment loop reproduce retail's endpoint
lifetimes. A single indexed matrix retains axis addresses across projection
calls and differs by 53 words. The named-array version has zero differing
words and identical relocation fields; its native body is 0x1524 bytes against
the 0x1530 retail extent, whose remaining bytes are zero padding.

The absolute value of the projected Y axis is a conditional expression. This
removes the non-retail `LightAbs` wrapper. An in-place `if` instead changes the
branch delay slot at function offsets 0x122C/0x1230. The projection declaration
comes from its owning `mglib.hpp` header. The BG/ambient, Fog and File page
labels include retail's trailing arrows; omitting them passes the instruction
comparison but fails canonical data binding at retail 0x0036A850 (`at_1230`).

A temporary, manually unguarded canonical compile with the wrapper and section
fixup passes the complete unit: 0x296C allocated bytes and 668 resolved
relocations. This is a matching candidate, but the assembly guard remains:
the inherited fog-channel expression still converts the object address to an
integer and adds the raw byte offset 6. Promotion must also satisfy the lane's
natural-source rule.

The proposed shared-header change overlays `mgFOG_PARAM::r/g/b/a` at offsets
8..11 with `u_char color[4]`, retaining sizeof 0x30. Typed access
`&fog->color[edit - 2]` adds an instruction at offset 0x79C, moving subsequent
code by four bytes and producing 873 differing words. Signed and unsigned edit
indices both do this. The proposal was tested privately and reverted; it is
layout evidence, not an accepted matching patch. No profile row is needed for
the zero-word candidate; the existing TU helper-mask row accounts for the 14
extra differences seen with plain wibo.

**Park category:** source compliance/shared layout. **Reconsider when:** an
approved fog-channel representation permits indexed C++ access without the
extra subtraction instruction, followed by a zero-word and whole-unit check.
The rest of the function's control flow, strings, locals and expressions have
matching evidence.

Local evidence is in `.private/receipts/bigfn-drafts/editdebug-final.log` and
`.private/receipts/bigfn-editdebug/final-probe/check.log`; the exact shared-header
proposal is `.private/bigfn/mg_drawenv-color-proposal.patch`. The final guarded
build comparison is `.private/receipts/bigfn-final/`.

Final guarded validation is identical to i9 in verifier, complete object-check
output and coverage. All three lane units pass; the inherited failing set stays
mg_texture, nd_meswin, actionchara and actscript (145/149 pass). Coverage stays
6,666 matched / 184 guarded / 15 assembly-only / 7 fuzzy. No target is promoted.
Comparison receipt: `.private/receipts/bigfn-final/comparison.json`.
