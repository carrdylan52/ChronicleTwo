# menudraw: reverse-engineering notes

The migrated form helpers write the form's character, movement and named part data through
their existing members. `CMenuEffect::type` is a signed byte: its initializer stores -1, and
using an unsigned field changes MWCC's immediate instruction.
`CMenuPosDataForm::mtype` is an unsigned byte: `Initialize` stores 0xFF.
`MENU_PARTS_EFFECT_STRUCT1::type` is unsigned; the random-parameter helper tests it with
`lhu`. `MenuCursorReverseFlag` has a byte load in drawing code and a word store in
`CMenuChrCngMenu::MenuLocalLoop`. `CRepairManager::IsRun` checks
all eight effect pointers and the model pointer even after finding an active entry.

Header: `ps2/include/menudraw.hpp`. Every field offset below was checked with compile-time
offset asserts against the header. No class in this unit has a vtable (no `__vt__` symbol).

## Script keyword tables (source of enum names)
The menu layout script parser in `menucommon` maps keywords to values with
`MENU_SPI_ANALYZE_STRUCT1` tables; the strings give the retail names used for the enums.
- `tbl_1728` (form dtype, `_MENU_FORM_DTYPE`) -> `MENUFORM_DTYPE`. 0x16 "combrd", 0x29 "mosbaji",
  0x2B "wmap" have no case in `MenuFormDraw` (draw nothing).
- `tbl_1759` (form mtype, `_MENU_FORM_MTYPE`): "n","d","l","i","ir" -> index-1, so n=-1 ... ir=3
  -> `MENUFORM_MTYPE`. Stored as a byte at form+0x20; `Initialize` writes 0xFF (= n).
- `tbl_1994` (part dtype, `_MENU_PART_DTYPE`): clut_reload 0x4F, 発明ネタ 0x3C, アルバム 0x3D,
  ごちゃ線 0x4C, font 0x4E, ネタ帳 0x3E. `tbl_2060` bg 0x2D / beta 0x2E (`_MENU_FRMIMG`),
  `tbl_2074` trs 0x37 / neta 0x3B (`_MENU_ITEM`), `tbl_2090` sq_beta 0x41 (`_MENU_FILLBOX`).
  Fixed values: `_MENU_NORMAL` 0, `_MENU_NORMAL2` 1, `_MENU_CURSOR` 2, `_MENU_FUNCINFO` 3,
  `_MENU_NUMBER1` 5, `_MENU_NUMBER2` 6, `_MENU_WAKU_RECT` 0xD, `_MENU_WAKU_CIRCLE` 0xE,
  `_MENU_FORM` 0x19, `_MENU_ITEM_CHECKMARK` 0x39. -> `MENUFORMPARTS_DTYPE`.
- `tbl_2144` (`_MENU_PARTS_EFFECT`): blink 2, rot 3, huriko 4, stretch 6, stretch_rep 7,
  stretch_sin 8. Values 1, 9, 10, 11, 12, 100 appear only in code (`MenuPartsStep`,
  `GetNowPosRGBA`, `Func_MenuItemIconSetEffectOne`) -> `MENU_PARTS_EFFECT_UNK_*`.

## CPosDataManage (0x20)
From `Initialize`, `EtcTblClear` (stride 0xC), `EtcTbl2Clear` (stride 0x14), `TexGetInfoClear`
(stride 0x20), `FormInfoClear`/`GetFormInfo(int)` (stride 0x80). 0x1E: `FormStep` forces each
form's `step_stop` while it is set. Size 0x20 = where `CMenuPosDataManage`'s own fields begin.
Element types `MENU_ETCINFO` (char*, int[2]) and `MENU_ETCINFO2` (char*, float[4]) are not
retail names (named after the `_ETCINFO_MALLOC`/`_ETCINFO2_MALLOC` script tags).

## CMenuPosDataManage : CPosDataManage (0x5BC)
Size from `MenuMainInit` (`__nw(0x5BC)`). `InitializeCMenuPosDataManage` zeroes 0x30..0x60,
0x6C, 0x70, memsets 0x74 (600 bytes, 0), 0x2CC (600, 0), 0x524 (0x96, value 2). Nothing in
the decompiled corpus reads 0x40..0x48, 0x74, 0x2CC or 0x524 through `MenuPosData`; types
unknown. `MallocPallet` loops i=0..1: palettes at 0x20/0x28/0x30 + 4i (normal / grey
quantised / sepia copies of the source texture's clut), textures at 0x5C/0x64/0x6C + 4i built
by `memcpy` of the source `mgCTexture` at 0x54 + 4i. `GetMenuItemIconTexInfo(item, kind)`
reads {0x54,0x5C,0x64,0x6C}[kind] + `use_trans_rect`*4 -> `item_icon_tex[4][2]`.
`SearchTransPalletNo` writes s16 at 0x38 + 2i. `AttachCommonTexInfo`: 0x3C, 0x4C, 0x50,
0x54, 0x58 from `mgTexManager.GetTexture` (names at_4182..at_4186); 0x4C is passed to
`DrawItemIconEffect2`, 0x50 to `CMenuEffect::PresetEffect` (fusion), hence the field names
`icon_effect_tex`/`effect_tex` are guesses from use; rename freely.

## CMenuPosDataForm (0x80)
Stride 0x80 in `CPosDataManage`. Layout from `Initialize`, script tags in `menucommon`
(`_MENU_FORM_SET` 0x00/0x01/0x14, `_MENU_FORM_DRAWFLG` 0x01, `_MENU_FORM_VIBECNT` 0x08/0x0A,
`_MENU_FORM_PUTXY` 0x0C/0x10, `_MENU_FORM_MOVERATE` 0x2C/0x30, `_MENU_FORM_RGBA_BIT` 0x50
(r=1 g=2 b=4 a=8), `_CLIP_WH` 0x06, `menu_dtype_init` 0x1C, 0x40..0x4C), `SetNextMovePos`
(0x20,0x24,0x28), `SetActionCharaPtr` (0x34,0x36,0x38), `SetRGBACalcParam` (0x51+i, 0x59+i),
`MenuFormStep` (`CalcMenuAdd(&rgba[i], (s8)rgba_add[i], rgba_target[i])`), `SetAction`
(0x5E, 0x60, 0x62, 0x64 stride 0x14, name compared inline), `FormReLink` (0x70/0x74).
- 0x34: below 1 -> `mgDrawDirect(chara->frame)`, else reload that texture block and call a
  chara vfunc. 0x36 never read here. 0x3C and 0x78..0x7F never seen.
- `action_state` values: 1 set by `SetAction`, 4 once `MenuFormStep` reaches target.
- `MenuFormStep`/`StepMainMenuIconMove` return a comparison result (ghidra: bool); declared
  `int`. `GetNextMovePos` always returns 1.
- Action entry `MENU_FORM_ACTION` {char[0x10], ptr} and its `MENU_FORM_ACTION_MOVE` {s16 mtype,
  float x, y, rate_x, rate_y} (from `_MENU_ACTION_DEF` and `GetNextMovePos`) are not retail
  names.

## MENUFORMPARTS_TYPE (0x48)
Stride 0x48 everywhere. `MenuPosDataTypeInit` gives defaults. `_MENU_PARTVIBER` 0x0B/0x0C,
`_MENU_PARTVIBECNT` 0x0E/0x10, `_MENU_SHADOW_ONOFF` 0x46/0x47, `_MENU_PART_ETCINFO` writes
int 0x30 + 4*i, `_MENU_PARTS_EFF_NUM` 0x40/0x44, `menu_texdata_to_formpart_copy` 0x24/0x28
from the tex info w/h. 0x19 bit 0 -> Bilinear; 0x1A -> `AlphaBlend((s8))`. 0x45 = item icon
state bits (|1 in `Func_MenuIconDrawPrepare`, |2 in `CheckItemBoardFunc_...`), passed to
`DrawOneItem`. 0x2C float, init 1.0, passed to `PictureDraw` (meaning unknown). 0x0D, 0x12,
0x1B unseen. menuop `KeyStep__14CSaveMenuClassFv` reads s16 at +0x12 of something: not
verified to be a part.

## MENU_BASETEXINFO (0x20), MENU_PARTS_EFFECT_STRUCT1 (0x24)
BASETEXINFO: `MENU_BASETEXINFO_Init`, `_MENU_TEXDATA` (0x10 name, 0x14 tex name, 0x18 block,
0x1A table index), rect is x,y,w,h. EFFECT_STRUCT1: `_MENU_PARTS_EFFECT` sets 0=1, 1=1, 2=type,
4+ floats; `MenuPartsStep` clears byte 0 at cycle end only when byte 1 is 0. Allocated in
16-byte units: `(n*0x24 + 15) >> 4`.

## MENUFORM_MAKEBRD_INFO (0x2C)
`Init_` memsets 0x2C; `CalcCommonBrdDrawInfo` copies 0x2C into the local `CommonBoardDrawInfo`.
`CommonBoardDraw` reads 4 lines of stride 6 (u8 board-style index into `get_onoffbrdtbl_1789`
stride 0x18, u8 button index into `get_btntbl_1810`, s16 at +2 and +4), int 0x1C drawn as a
number, 0x20 index into a colour table, 0x24/0x28 tested > 0. editmenu `MakeMsgPartsItemInfo`
counts materials into 0x18. Line struct `MENUFORM_MAKEBRD_LINE` is not a retail name.
`MakeBoardDrawInfo` (global, 0x14) is float[5]: (x,y) of two buttons indexed `[i*2]` by
editmenu/inventmn, then the board width at +0x10.

## Effects
- `CRepairEffect` 0x24: `CRepairManager::Generate` does `__nw(0x24, ...)`. Particles stride 0x30
  (`__nwa(n*0x30)`), struct `REPAIR_EFFECT_PARTICLE` not retail. Particle 0x0/0x4/0x8 set to
  128/128/64 but never read; 0x14 set 0.
- `CRepairManager` 0x1EC: `MenuModeMalloc__13CMenuItemInfo` `__nw(0x1EC)` and constructs
  `mgCMemory` at 0x24..0x1A4 step 0x30 plus 0x1B4 (implicit ctor of the member array).
  0x1A4 copied into effect+0x1C, never set in this unit. 0x1B0 holds an inline-constructed
  `CActionChara` (`__nw(0x1030)`), driven by vtable calls. 0x1D0/0x1D8 writes in
  `GeneratePoly`/`Step` are fields of `model_stack`.
- `CLevelUpEffect` 0x30 / `CLevelUpEffectManager` 0x190 (`MenuLevelUpMan` symbol size). Effects
  start at manager+0x10, so the effect is 16-aligned: pos at 0x10 is `sceVu0FVECTOR` (filled by
  a CCharacter2 vfunc +0x18). Manager 0x0 = label texture (menusys `EnterDataMenu`), 0x4 = spark
  texture; 0x8..0xF padding. `IsRun` loads the byte at 0 and returns it as int; the
  manager callers consume the extended return without a second byte conversion.
  Spark state is in the local arrays `l_levelup_pos/vec/counter/generate_counter` (32 sparks,
  shared by all effects).
- `CStarDust` 0xC: inventmn `__construct_new_array(..., __ct__9CStarDustFv, 0, 0xC, n)`; ctor
  (declared in menudraw.hpp, defined in inventmn.cpp) clears byte 0xA.
- `CEffVerticalLine` 0x40: `__nwa(n << 6)`; `mgTransWorldPrim3DSprite(..., this, w, h)` so
  pos is a 4-float vector at 0; 0x34..0x3F unseen.
- `CMenuEffect` 0x38: `__nw(0x38)` in inventmn/menusys. Effect type byte 0x9 values seen:
  0, 1, 2, 4, 0xA, 0xB, 0xC, 0xF, 0x10, 0x12, 0x13 (spectrum break), 0x14, 0x15 (fusion item);
  no names known, so no enum. 0x14 is s16[16] copied from an int array (low halves).
  `MENU_EFFECT_INFO` (0x40, stride in `PresetInfoAll`) is 16 floats used per kind; only
  0xC/0x10 (screen x/y in `Draw`) named.

## Globals
Only non-local data is declared `extern` (checked against `local_symbols.tsv`). Types:
`MenuMesForm` 9 form pointers (`AttachMessageForm`), `MenuDrawItemInfo` 150 item pointers
(stride 0x6C items in `SetModeMenuDrawItemBoard`; loop bound 0x96), `menu_limmit_displayflag`
byte flags (symbol size 0x9C), `Pos_ItemInGiftBox` s16[3][2], `GiftBoxWindowPutPos` /
`menu_long_hand` mgRect<int> (set in `__sinit_menudraw_cpp` via `mgRect::Set`, so these are
dynamically initialised in source; same for local `MenuMainFrame_PutRect`,
`MenuMainIMG_PutRect`, `star_light`, `MenuItemBrdKomaRect`, `ItemBoardScrlBar1..3`,
`ItemBoardCursor`). `NowGiftBoxPtr` holds `SearchNowPosItemExist` results (declared
`CGameDataUsed*`; it reads s16 at 0 and +0x10). `MenuCursorReverseFlag` is declared
int for the word store in `CMenuChrCngMenu::MenuLocalLoop`; drawing code tests its
low byte. `MenuItemBrdCalcManner` and `GiftBoxViewFlag` are byte flags.

## Other
- File-local functions (static, not in the header): ConvMGIRECTtoINTtbl, ConvMGFRECTtoFLOATtbl,
  both SetPartEffectInfoRandFunc, PushPrimRepeat, MenuWindowHelp, SetMenuDrawNumberKeta,
  DrawMenuNumber, GenarateRandamLine, DrawRandamLine, MenuMainFrameImgDraw,
  MENU_BASETEXINFO_Init, DrawItemIconEffect2, MenuFrameImageDraw, InitInitBuildUpInfoEffectPos.
- `PrimQuad<f>`/`PrimQuad<i>` are instances of a function template
  `template <class T> void PrimQuad(mgCDrawPrim*, mgRect<T>, mgRect<int>)`; only declared in the
  header. Retail has one copy each, in this unit; other units call them.
- `GetMenuDlTexture` returns the texture (ghidra lost the return; callers cast the result).
  `GetMenuMainFrameCount` returns a float from `tbl_2072`; `GetMenuMainFrameLeftTopPos` returns
  `&MenuMainFrame_LeftTop_Pos` (float[2]) regardless of its argument.
- Parameter names of the large drawing functions are best guesses from their bodies; `unk`
  parameters were not traced.
- No first-game counterpart: the first game has no menudraw/CPosDataManage equivalent in
  `/home/adubbz/development/chronicle/ps2/include`.

## Compiled coverage

The object report records 137 perfect functions and 61 entries without a source-side
score. The normal C++ object has 150 retail-matching functions: 13, including
`PrimQuad<float>` and the other rectangle-typed helpers, have no source-side progress
score. The remaining 48 functions use assembly fallbacks.
`CLevelUpEffect::Step` has a typed-array draft; its loop differs in 13 of 140 instruction
words and uses retail assembly in the linked tree. `PrimQuad<float>` is explicitly
instantiated without a draft guard or assembly fallback. The source initializers
for the rectangles match
`__sinit_menudraw_cpp`.
