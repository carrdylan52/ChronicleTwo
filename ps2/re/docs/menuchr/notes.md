# menuchr: reverse-engineering notes

Header: `ps2/include/menuchr.hpp`. The unit has no first-game counterpart (no `menuchr` or
equivalent classes exist in `/home/adubbz/development/chronicle`).

## Classes

All four classes derive from `CBaseMenuClass` (`menusys.hpp`, size 0x110, vptr at 0x10C), so
derived fields start at 0x110. Each is constructed inline in its `*Init` function (no
constructor symbol), which is why the vtables are emitted in this unit.

### Vtables (`__vt__<class>`, 0x20 each, 2 header words + 6 slots)
Slot order is the base's: `IsCreateObject`, `IsMakeObject`, `IsAskExtend`, `ItemCmdAfter`,
`InitEnd`, `ExitEnd`. Only `CMosBookMenu` overrides anything (`InitEnd__12CMosBookMenuFv` in the
`InitEnd` slot). `CMenuChrCngMenu`, `CMenuMosSelect`, `CMenuCostumeSel` use the base entries.

### CMenuChrCngMenu (0x1F80)
- Size: `__nw__FUiP1(0x1F80, ...)` in `MenuCharaChangeInit`; instance kept in `ChrChangMenuPt`.
- Inline ctor in `MenuCharaChangeInit`: `Init` on 0x194/0x1C4 (two `mgCMemory`), clears/sets most
  fields (0x122 = -1 change_chara, 0x11C = -1 then 0x11E = 1, 0x24C/0x24E = -1 sub_menu(s),
  0x200 = -1 face_state, 0x202 = -1 face_chara), calls `InitStarInfo`, then
  `memset(this+0x1A80, 0, 0x500)`.
- 0x1A80 `clut[256]`: `EnterDataMenu` sets `MenuCharaChangeCLUT = this+0x1A80`, `memcpy`s 0x400
  bytes of the base texture's palette (`tex+0x60`) into it, darkens each of the 0x100 RGBA entries,
  and points the copied texture `MenuCharaChangeCLUT_Tex` (0x70-byte `mgCTexture` copy) at it.
- 0x1E80..0x1F80 (`unk_1E80[0x100]`): only covered by the 0x500 memset; no other access found in
  any unit.
- 0x280 `star[256]` of `CHR_CNG_STAR` (0x18 stride; 0x280 + 0x1800 = 0x1A80). Fields 0x10/0x14 of
  a star are unresolved.
- Field offsets in the header were checked with temporary `offsetof` asserts against the
  offsets above (all fields from 0x110 to 0x1E80).

### CMenuMosSelect (0x7670)
- Size: `__nw__FUiP1(0x7670, ...)` in `MenuMonsterBoxInit`; instance in `MenuMosSelectPtr`.
- Inline ctor: `CDC2Mes` ctor at 0x150 (0x2A50 -> 0x2BA0), `ClsMes` ctor at 0x2BA8 (0x2958 ->
  0x5500), a loop constructing `CActionChara`s (0x1030 stride) from 0x5590 until 0x65C0, i.e. one
  element (declared `monster[1]` to keep the array form the loop implies), then a single
  `CActionChara` at 0x65C0 (`effect`), `mgCMemory` Init at 0x75F0 and 0x7620.
- 0x5554/0x5558/0x555C = -1 (view/pick/load monster); 0x5508 = 1 (set_cursor); 0x5500 = 1
  (info_win_show); 0x140 = `GetMonsterBajjiDataPtr(.., 1)` (badges, `MOS_CHANGE_PARAM`).
- 0x110/0x120 are `sceVu0FVECTOR` (16-aligned) holding camera pos/ref; 0x5580 model_pos is a
  vector (hence the 16-byte alignment and the tail padding to 0x7670 after load_phase at 0x7662).
- Unresolved: `unk_148[8]`, `unk_2BA4`, `unk_5504` (zeroed in ctor), `unk_7620` (second stack),
  `unk_765C`.
- `KeyStep` returns `MOS_SELECT_RESULT` (1 close, 2 change) per the existing header; the
  meaning of 2 should be re-confirmed against `KeyStep`'s return sites when its body is written.

### CMenuCostumeSel (0x2D0)
- Size: `__nw__FUiP1(0x2D0, ...)` in `MenuCostumeInit`; instance in `MenuCosPtr`.
- Inline ctor: `mgCCameraFollow(40, 30, 0, 8)` at 0x110 (0xC0 -> 0x1D0), `mgCMemory` Init at 0x228,
  0x2C0 = `GetCharaDataPtr(.., 0)`, 0x260 = 15.0f, 0x268 = 4.0f (chara_pos x/z), zeroes
  0x1DE..0x1E2 (costume_select) and 0x284..0x2A4.
- `costume_list[3][8]` s16 at 0x1E4, `list[3]` s16* at 0x214.
- Unresolved: `unk_1D4`, `unk_220`, `unk_270[4]`, `unk_280`, `unk_2A8`, `unk_2AC`, `unk_2BC`.

### CMosBookMenu (0x980)
- Size: `__nw__FUiP1(0x980, ...)` in `MonsterBookInit`; instance in `MonsterBookPtr` /
  `MenuMosBookPtr`.
- Inline ctor: `mgCCamera(8.0f)` at 0x110 (0x70 -> 0x180), `mgCMemory` Init at 0x184, zeroes
  0x1B8..0x1E4 and 0x7E8; camera pos (0,0,100), ref (0,0,0).
- `SetMonsterInfo`: strcpy to 0x82C (name, from tbl+4), 0x7EC (area), 0x86C (type name from
  `monster_type_name`), 0x8AC strcpy/strcat (weak names from `monster_jyakuten`), 0x904/0x908 from
  u16 at tbl+0x56/+0x58 (hp, abs), 0x90C = `KillMonsterCount`, 0x910/0x914 OR'd with
  `stand_bit_5472` (resist >100 / weak <0x33), drop items at 0x918 stride 0x21 x3 (also seen in
  `Draw` at 0x918/0x939/0x95A). `list[0x180]` ints 0x1E8..0x7E8.
- Unresolved: 0x1BC..0x1CC (five words, zeroed in ctor).

### MENU_BGREAD_INFO2
- `InitMenuBGReadInfo2` clears 0x0, 0x20, 0x70 (s8) and 0x74 (pointer). 0x70 is tested as the
  "reading" flag in `MenuLoadFileCheck` and loop code; 0x74 holds a `CActionChara*`
  (`MenuItemCharaDataLoadEndCheck`). Allocated with `Alloc(stack, 8)` (8 x 16 bytes), which only
  bounds the size to <= 0x80; natural size is 0x78. No size assert is given.
- `CosutmeSelDefaultSet` searches the first five 16-bit costume IDs and returns the matching
  index, or zero when none match. The function is local to this unit.
- `CMosBookMenu::InitMonsterInfo` clears the first character of
  each display name and drop item, plus the numeric monster details. The offsets are inside
  the named arrays and fields in `CMosBookMenu`.

### CHR_CNG_STAR (0x18)
Stride from `InitStarInfo`/`CalcTex`/`MenuCharaChangeStarDraw`; 0x10/0x14 unresolved.

### mgRect<short>
Declared by the `mgRect<T>` template in `mg_tanime.hpp`; `Set__9mgRect_s_Fssss` is its inline
`Set` instantiated here. Size 0x8 is asserted in the header.

## Enums
- `CHR_CNG_PHASE` (change_phase 0x120): 0..3 from `CheckChrChange`.
- `CHR_CNG_SUB_MENU` (0x24C/0x24E): -1 none, 1 monster box.
- `MOS_SELECT_RESULT`: 1 close, 2 change (`CMenuMosSelect::KeyStep`).
- Table sizes enum: `MENU_CHARA_LOAD_MAX` 7 (loops `< 7` over `MenuCharaBuild2`,
  `MenuActionChara`; `MenuActionCharaBuffer` built with `__construct_array(..., 0x30, 7)` in
  `__sinit`), `MENU_LOAD_ITEM_MAX` 12 (`MenuLoadItemNo` 0x18 bytes), `MONSTER_PROGRESS_NUM` 19 and
  `MONSTER_PROGRESS_LEVEL_NUM` 4 (`monster_progress_tbl` 0xBE = 19 x 5 s16).

## Globals
Only symbols that are global in retail are declared in the header: `monster_progress_tbl`,
`MorattaStack`, `MenuLoadInfo`, `MenuCharaChangeBase_Tex`, `MenuCharaChangeCLUT_Tex`,
`MenuCharaChangeStar_Tex`, `CharaSndBuffer`, `MenuCharaBuild2`, `MenuActionChara`,
`MenuActionCharaBuffer`, `MenuLoadItemNo`, `MenuChangeNpcMemory`, `SwordEffectStack`.
Every other named datum in the unit (`menu_robo_memorytbl`, `menu_chr_memorytbl`,
`menu_infocfgname`, `MonsterDataPath`, `monster_type_name`, `monster_jyakuten`,
`monstere_file_template`, `MenuSoundCharaNo`, `menu_chara_chrtbl`, `menu_chara_cfg_chrtbl`,
`monster_load_id`, `NowReadMainChara*`, `MenuCharaChangeCLUT`, `ChrChangMenuPt`,
`MenuMosSelectPtr`, `MenuCosPtr`, `MonsterBookPtr`, `MenuMosBookPtr`, `Tex_MB*`, the
`MenuMos*Stack`/`MosBookStack`/`MenuChangeMemory`/`ChrChangeInitTextureStack` stacks,
`mos_effect_*`, `MenuMonsterBGInfo`, `script_file_name`, `NowMainRead*`, debug flags, etc.) is
LOCAL in `local_symbols.tsv` and belongs as `static` in the `.cpp`.
Note: `MenuActionChara` is 0x1C in main.symbols.txt (BSS slot 0x20 with padding) and
`MenuLoadItemNo` is 0x18 (slot 0x20).

## Local functions (static, not in header)
`CheckBattleLoop`, `MenuMemoryDivide`, `EditCharaPrepare`, `GetBajjiPosition`,
`CosutmeSelDefaultSet`, `GetMonsterBaseInfoForMonsterMemoIndex`.

## Return values
- `CMenuChrCngMenu::KeyChangeMain` always returns 0 (both paths set `$v0 = 0`); caller ignores it.
- `ConvertCharaLoadDataPhase` returns an s16 from `tbl_992[chara*5 + part]`.
- `get_gajji_id_from_monster_progress_table` returns s16 (row's first column) or -1.
- `MenuCharaSoundLoad`, `MenuItemChrLoad` return u32 sizes.

## C++ draft status

The unit has 88 retail functions. The normal image has 55 perfect functions and 33 assembly
entries. Four clean bodies remain guarded: `MenuCharaChangeInit`, `MenuNPCLoadCheck`,
`CMenuCostumeSel::LoadMenuData`, and `CMosBookMenu::KeyStep`.

`mgRect<short>::Set` is defined in C++ in this unit and supplies its retail bytes; the object
checker lists its template spelling as an assembly entry. The C++ definitions of the memory
managers generate the retail static initializer. The draft compile has 56 MATCH functions,
four DIFF functions, and 28 entries without a draft.

`MonsterEffectEnter` takes a scene, a read buffer, and a texture block. Its retail callers pass
three arguments. `MOS_HENGE_PARAM` holds the transformation script's file name at 0x8
(`GetMonsterModelFile` formats it with `"%s.stb"`) and four effect names at 0xC. The menus use
those members when reading and entering transformation effects.

## Party change initialization and monster book states

`MenuCharaChangePosDataCfgBuffer` is a retail data symbol, LOCAL, at 0x0037E230,
size 4 in `.sbss`. The ELF symbol has type OBJECT. The function-binding helper
does not include data symbols. `MenuCharaChangeInit` clears the item with
`sw $0, -0x64C0($gp)` at 0x002B926C, using retail `_gp = 0x003846F0`.

The party-change constructor clears `clut` (0x1A80..0x1E80) and `unk_1E80`
(0x1E80..0x1F80) separately. These cover the same 0x500 bytes retail clears in
one call. The first `set_cursor = 1` has no intervening reader or call before
`set_cursor = 0`; the constructor retains the final zero store.

`MenuCharaChangeInit` constructs `CRepairManager` with placement new. Its implicit
header constructor initializes the eight `mgCMemory` members at 0x24 with stride
0x30 and the model stack at 0x1B4, as retail does. The seven scene characters are
assigned to `MenuActionChara[0..6]` in order. The clean draft remains different
from retail, including allocation null-check scheduling, the separate buffer
clears, and register assignment. The draft comparison is 262 of 316 instructions
different; the linked image still uses retail assembly for this function.

`MOS_BOOK_MODE` describes the book's browsing, opening fade and closing fade
states (0, 1 and 2). `MOS_BOOK_MEMO_NUM` is the 0x119 entries scanned by `InitEnd`
and the debug completion command. The model read delay and show-counter cap are
both 20; book commands are 0xA to close and 0x64 to change the entry. These values
are unchanged in `KeyStep`.
