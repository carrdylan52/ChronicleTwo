# menusys: header notes

Header: `ps2/include/menusys.hpp`. All offsets and sizes below were checked by compiling a test
against the header (offsets of the key fields and every `sizeof`).

## Includes
`userdata.hpp` at the top (CGameDataUsed by value in `CMenuKeyFunc`, `CMenuItemInfo`). userdata.hpp
no longer reaches inventmn.hpp (CInventUserData moved into userdata.hpp), so there is no cycle.
Header order (acyclic, each includes only what it needs by value): gamedata < userdata < menusys < savedata < memcard < inventmn.

## Classes and sizes
| Type | Size | Evidence |
|---|---|---|
| `CBaseMenuClass` | 0x110 | ctor `memset(this, 0, 0x110)`; derived classes' fields start at 0x110; vptr stored at 0x10C |
| `MENU_ASKMODE_PARA` | 0x94 | `Initialize` = `memset(this,0,0x94)`; symbol `MenuAskParam` size 0x94; base 0x58 + 0x94 = 0xEC (swap_info) |
| `MENU_SWAPITEM_INFO` | 8 | `Set` writes 0..6; followed by 0xF4 in base |
| `ITEMCMD_RET_PARA` | 0x14 | symbol `MenuItemCmdRet` size 0x14; accesses at 0,2(sb),3(sb),4,6,8,A(sh),C,10(sw) |
| `CMENU_USERPARAM` | 0x18 | `Initialize` clears 6 words; `MenuUserParam` size 0x18 |
| `CMenuKeyFunc` | 0x160 | `new(0x160)` in `MenuMainInit`; `Initialize` = `memset(this,0,0x160)` |
| `CMenuItemInfo` | 0x370 | `class_menu_item_info` size 0x370; `__sinit` builds base, stores vptr at +0x10C, `CGameDataUsed` ctor at +0x304 |
| `CItemSelect` | 0x450 | `new(0x450)` in `MenuItemSelectInit`; last used field 0x448, so `unk_44C` pads to the size |
| `MENU_INPUTKEY_ARG` | 0x24 | `item_menu_argtbl` (0x1B0 = 12 rows) indexed `key_arg_no * 0x24` |
| `MENU_ITEM_CURSOR_INFO` | 0xC | `MenuItemCursorInfo` size 0xC: bytes 0..6, int at 8 |
| `BUILDUP_WEAPON_INFO` | 0x44 | `BuildUpWeaponInfo` size 0x44 |

`MENU_INPUTKEY_ARG`, `MENU_ITEM_CURSOR_INFO` and `BUILDUP_WEAPON_INFO` are **not retail names**.
No symbol names these types, so the names are neutral ones taken from the tables and globals that use them.
`MENU_ASK_MODE`, `MENU_SELECT_KEY`, `MENU_PUSH_BUTTON` and `MENU_INPUTKEY_TYPE` are likewise our names.

## CBaseMenuClass
- vtable (`__vt__14CBaseMenuClass`, 0x20 = 2 header words + 6): IsCreateObject, IsMakeObject,
  IsAskExtend, ItemCmdAfter, InitEnd, ExitEnd. All six are inline (bodies `return 1` / `return 0` /
  empty) and are emitted in dngmenu (0x1F3D00..) and editmenu (InitEnd 0x1FF8D0). The vtable itself is in menusys.
  `CItemSelect` keeps all six. `CMenuItemInfo` overrides IsAskExtend, ItemCmdAfter and ExitEnd.
- ExtendCommand dispatches on `mode`. Mode 4 calls vt+0x14 (ItemCmdAfter) with `&MenuItemCmdRet`; mode 5
  calls vt+8, mode 6 vt+0xC and mode 12 vt+0x10. These give the `MENU_ASK_MODE` values 3..12.
  The ctor sets mode 1, `IsAskEnd` sets 0, and `CItemSelect::KeyStep` uses 1 (waiting for its background) and
  2 (closing).
- 0x04 u8 `opened`: `CMenuItemInfo::KeyStep` sets it once it has run the opening script (`ExeScript`).
- 0x08/0x0C: script pointer and size, from `GetPackFile(..., &size)` in `MenuItemInit` and passed to
  `MenuCommandAnalyze(char*, int, char*)`.
- 0x10 is set to 0x80 by the ctor and never read in menusys (`unk_10`). 0x06, 0xF8, 0xFC and 0x104 are
  never read here either.
- 0x14 s16 `key_arg_no`: index into `item_menu_argtbl` (`CMenuItemInfo`).
- 0x18 `int tex_block[16]`: SetTexBlock/DeleteTexBlock loop to 16 and stop at a value <= 0 or < 0.
- 0x58 `ask_para`; 0xEC `swap_info`; 0xF4 `cmd_arg_pos` (MenuItemCommnadSelectPrepare stores its arg).
- 0x100/0x108/0x10A: the make question of SelectMakeObject (count, cursor row byte, max s16).
- Ctor order in retail: vptr store, `ask_para` ctor, `swap_info.Set(-1,0,-1,0)`, `memset(this,0,0x110)`
  (this clears the vptr too; derived classes store theirs afterwards), field inits, 16x `tex_block=-1`,
  `cmd_arg_pos=-1`, `unk_F8=0`, `step=0`, `SetAskParam(NULL)`, `memset(&unk_FC,0,0x10)`.
  MENU_SWAPITEM_INFO has **no** constructor. Locals in EquipDirect and ItemCmdAfter are only `Set` once
  with other values, so the Set call belongs in the ctor body.

## MENU_ASKMODE_PARA
- 0x02 `mes_no` indexes `MenuDCMsg` / `MenuMesForm`, and 0x04 holds the command count.
- `GetItemCommandMsg(item, para, ...)` passes the arrays at 0x08 (int[8]), 0x28 (u32[8] colours;
  0x80202020 / 0x80303030 grey), 0x48 (s16[8]) and 0x58 (s16[8]; set to 1 in one case).
- `SetAskParam` copies 0x00..0x04, then a loop of **16** iterations over the three arrays at 0x08, 0x28 and
  0x48. With 8-entry arrays this overlaps the next array, so the retail loop overran its arrays. Reproduce it
  with a bound of 16 (unrolled by 8 → 2 iterations). Then it copies 0x68..0x70 (s16) and 0x74..0x88 (words).
  0x06, 0x8C and 0x90 are not copied.
- 0x68/0x6A: mode-dependent values (MoveItemCommand: chara number; SpectolBreak: `GetSameAdrressUserData`
  of both items). 0x78 form, 0x7C item, 0x80 second item (these come from stack copies in SetPreCmd*).
- The ctor sets 0x8C = -1 **before** calling Initialize (which zeroes it). Retail does this.

## CMenuKeyFunc (the object at `MenuCommonInfo`, owned by menumain)
- Construction (`MenuMainInit`): `mgRect<int>::Set(0,0,0,0)` on +0x90, `CGameDataUsed` ctor on +0xC0,
  `MENU_SWAPITEM_INFO::Set(-1,0,-1,0)` on +0x12C, then `Initialize()`. The rect Set comes before the member
  ctor of +0xC0. That suggests `mgRect` has a default ctor calling `Set(0,0,0,0)`, which `mg_tanime.hpp` does not
  declare. The same Set pair appears first thing in the inline `CItemSelect` construction. The header declares
  `CMenuKeyFunc() { have_swap.Set(-1,0,-1,0); Initialize(); }`. Check this against MenuMainInit when matching.
- 0x01 `key_enable` (gates every key read), 0x02 `key_input`, 0x04 select-key bits, 0x08 push-button bits.
- 0x0C `tex_block[16]` = `MENU_INIT_ARG::tex_block_top + i` (MenuMainInit). 0x4C set to -1 there.
- 0x50 s16 = `MENU_INIT_ARG::open_type`; 0x54 = running MenuModeID (`menu_keyfunctbl[...]`); 0x58 = next mode
  (-1 none); 0x60/0x64 = `MENU_INIT_ARG::pack/pack_size`; 0xA0 = `MenuUserDataManPtr`.
- 0x5C/0x5E: frame counters that light the "up"/"down" parts of the how-many board (MenuPosStep).
- 0x68 waku type (SetWakuType, parts "waku%d"). 0x70/0x74 cursor/top line, passed to MenuKeySelectCheck.
  0x78/0x7C are copies made by SelDataInit.
- 0x80 u8 set by ReturnItemMenu. 0x81..0x8F, 0x6C and 0xA4..0xBF are unused in menusys.
- 0x134 `MENU_INPUTKEY_ARG *` (assigned `item_menu_argtbl + n*0x24`). 0x138 form "cursor0", 0x13C "cur_waku0",
  0x140 "howmachbrd"; parts 0x144 "item0", 0x148 "item0sdw", 0x14C "item0num".
- 0x150..0x15A: BGM fade (saved volume, step, target s16, active s16).
- `MenuPosStop` and `MenuPosPlay` set `step_stop` on both cursor forms; the move-method
  setters write each form's `mtype`. `SelDataInit` clears key state and saves cursor and top
  line; `CheckKeyInput` discards button state when input is disabled and latches a nonzero
  direction into `key_input`. `FadeInMenuBGMVol` records a step, targets the saved volume,
  and marks the fade active.
- Return types: CheckAnalogKey returns float 1.0; CheckKeyInput returns the byte at +2; StepMenuBGM returns
  s16 +0x15A; the limmit_check functions return s16.

## MENU_INPUTKEY_ARG (item_menu_argtbl rows)
Seen in `menu_inputkey_limmit_check_line/glid`:
- 0x00: s16 step[4], the deltas for up/down/left/right.
- 0x08: int type, 0 for a line and 1 for a grid.
- 0x0C: s16 min. 0x0E: s16 max (a line). MenuItemDebugKey writes `MenuItemBoardTotalNum` into row 2's 0x0E.
- 0x10..0x13: bytes. A line uses 0x10 as its visible rows. A grid's up/down use 0x12 (max) and 0x10 (visible),
  and its left/right use 0x13 (max) and 0x11 (visible). 0x13 is also the column count used to split the
  cursor. MenuItemDebugKey writes `MenuItemBoardTotalLine` to row 2's 0x12.
- 0x14: s16 limit[4], the MenuKeySelectCheck mode. 0 clamps, 1 wraps, 2 clamps and returns 3 at an edge, 3
  returns 4 at an edge.
- 0x1C: s16 exit_no[4], returned when the edge result is 3.

## CMenuItemInfo (global `class_menu_item_info`, local; `CMenuItemInfoPt` points at it)
- No constructor symbol; `__sinit_menusys_cpp` constructs it inline (base ctor, vptr, `CGameDataUsed` ctor
  at 0x304). The implicit ctor gives exactly this.
- 0x110 `view_mode`: page. 0/1 = Max/Monica, 3 = ridepod, 4 = monster, and 2/5 are also seen.
  `GetActiveCharaIDForItemCmd` maps it.
- 0x114 `sub_view`, compared with 1 in GetActiveCharaNo. 0x116 holds a character number in CalcTex and
  ModelRead (`view_chara`). 0x118 is the load item list (CheckLoadItemNo), and 0x11A = `MenuUserDataManPtr+0x44D98`.
- 0x120 s16[8] / 0x130 u8[8] are the equipment lists (SetEquipListNo copies CHARA_DATA +0x172, +0x1DE, ... and
  ROBO_DATA +0x32...). 0x138 holds the loaded weapon number (CheckLoadInfo compares it with CHARA_DATA+0x1DE).
- 0x140/0x150: `float[4]` camera reference and position, filled by a CPosDataManage call (`at_6013/6014`,
  count 3) and used with `mgCCamera::SetRef/SetPos`. The header declares them `sceVu0FVECTOR` because 0x140 is
  16-aligned.
- 0x16E/0x16F: voices loaded / voices need loading (CheckSoundLoad, CheckLoadInfo).
- 0x17C `CGameDataUsed *` shown weapon (Save/CheckViewWeaponStatus).
- 0x180 `view_form[6]` from `ItemMenuFormNameTbl` = form_view00, form_view01, form_view1..4. 0x188 is
  form_view1 (weapon status), 0x18C form_view2, 0x190 form_view3, 0x194 form_view4.
- 0x19C "itembrd" (+0x1B4 its part "icon"), 0x1A4 "moneybrd", 0x1A8/0x1AC "poly_chr0/1", 0x1B0
  "mainform_filldmy".
- 0x1C8 `[2][16]` part pointers, stride 0x40 per character page. Twelve are used per page: wepfrm0 whp00 slu1
  whp01 wep0 batu0 / wepfrm1 whp10 slu2 whp11 wep1 batu1. MenuItemCharaActWepInfoDraw indexes it as
  `(i+4)*4 + 0x1B8`, which is the same array (0x1B8..0x1C7 itself is unused).
- 0x2A0 robo parts (wepfrm, whp00, slu1, whp01, wep0, batu0 of form_view2). 0x2B8 hp_bar[2]. 0x2C0 [2][3]
  (item0, item1, item2 via `local_over_flow_baseposname`). 0x2D8 [2][3] (item0num..item2num). 0x2F0 "VOICE".
- 0x2F4 is used as `int *` in MenuItemDraw. 0x2FE/0x300/0x304 are used by the debug key/draw (item number,
  flag, item).
- Unused here: 0x112, 0x11C, 0x13A (only cleared and read in CalcCursorPosition), 0x160 (flag), 0x161..0x16B,
  0x172..0x178, 0x198, 0x1A0, 0x248..0x29F, 0x2F8, 0x2FC. Most of these are set or tested somewhere but their
  meaning was not established.

## CItemSelect (heap object, `ItemSelectPtr`)
- `MenuItemSelectInit` attaches the caller's remaining stack, constructs the list in 0x47
  quadwords, sets its two screen rectangles, attaches the menu texture, and starts the background
  read. Modes 9 and 0x16 read separate menu files into the stack; the size is rounded up to
  quadwords before the message window is preset. The constructor and function drafts are guarded
  by `NONMATCHING`, leaving the retail assembly path intact.
- Built inline in MenuItemSelectInit: base ctor, vptr, two `mgRect<float>::Set(0,0,0,0)` (see the mgRect note),
  then the field clears, then `Set(120, mgScreenHeight-0x10A, 0, 200)` and `Set(list.x+20, list.y+370, 44, 55)`,
  then line_num = 1.0, CheckEnableHaveItemNum and SetPtrList. The header does not declare the ctor (no symbol).
- 0x110 count, 0x114 `CGameDataUsed *[150]` (from `MenuUserParam.used_data`, stride 0x6C, 150 entries),
  0x36C u8[150] (`menu_limmit_displayflag`), 0x402 alpha step (0xC in, -8 out), 0x404 alpha and 0x408
  background alpha (CalcMenuAdd), 0x430..0x438 eased cursor x/y and scroll, 0x43C texture, 0x440 cursor
  (5 columns), 0x444 top line (MenuCheckLine, 2 lines), and 0x448 float rows = count/5+1.

## Globals
Only non-local symbols get externs (the rest are `static` in the .cpp per `local_symbols.tsv`). Types:
- `menu_item_swap_sndtbl` is s16[8], indexed by the MenuSwapItem result.
- `SpectolInfo` is `CGameDataUsed*[2]` (SetSpectolInfo writes gp-0x6A80 and gp-0x6A7C).
- `MenuEffect` is `CMenuEffect*[2]` (passed as `CMenuEffect**` to SetEffectSpectolFusion; +0 and +4 loaded).
- `SpectolFusionTargetChara` is `CCharacter2*`. `trans_spectol_cnt` is float (sinf(cnt*...) and +1.0).
  `trans_spectol_pos` is int.
- `MenuItemUseTarget` is `CItemUseTarget` (sinit sets the first word to -1).
- `menu_chara_activeItem_limmit_check` (6 bytes) is never referenced, so it is declared `u8[6]`.
- `BuildUpNameXY` is s16[3][2] (x, y per name).
- `BUILDUP_WEAPON_INFO`: 0x00 s16 mode, 0x02 s8 select active, 0x03 s8 cursor, 0x08 int count. Its address
  +0x8, +0xC, +0x18 and +0x2C is also taken and +0x24 is read (not resolved: `unk_4`, `unk_C[0x38]`).
- `MENU_ITEM_CURSOR_INFO`: byte 0 enable, bytes 1..3 (loop of 3) and 4, 5 are arrows, byte 6 is the
  character mark, and the int at 8 is a counter that wraps above 0x18.

## Functions
- Local (static, keep in .cpp): DrawTrushMenuMessage, MenuHowMuchNumSelect, UpdataInfoSpectolBreakItem,
  SetConditionHowMuchBoard, CheckFishCondition, InitSpectol, AfterSpectolFusion, SpectolFrameCalc,
  TransSpectolDataSave, MenuDataSwap, both GetItemCommandMsg, MenuEquipCameraSetEnv,
  MenuPosFormValueSetWeapon, MenuFormUpdataAttachInfo, MenuPosFormValueSetFishingRod, MenuItemDebugKey,
  MenuItemDebugDraw, local_item_infoview_set, MenuItemCharaActWepInfoDraw, MenuItemCharaViewCheck,
  MenuPosFormValueSetCharaRobo, MenuPosFormValueSetMonster, BuildUpWeaponNameBoardDraw,
  MenuWeaponStatusInfoFormSet, MenuItemSelectDiffer, CheckTrushWeapon.
- `MenuItemSelectDiffer` has a 0x30-byte jump table at `at_7968` whose entries
  point to interior addresses in the function. Its active C++ matches the
  retail function and the linked jump-table targets.
- Return types come from m2c and Ghidra and were checked at call sites for SearchNowPosItemExist and
  GetExistThisPosData (both `CGameDataUsed *`) and GetGameDataUsedForSWAPINFO. Parameter names of the big
  functions (ModelReadStart, WeaponBuildCheck, KeyStepLocal, CheckSpectolFusion's int) are only partly
  established.
- Mangling of a sample of the declarations (ExchangeItemInfoMake, EnterDataMenu, MenuSwapItem,
  GetDebugInputKey, FadeInMenu, IsItemUseNum, MenuItemInit, ItemCmdAfter...) was checked by compiling stub
  definitions.

## First game
The first game has no counterpart to any of these classes (no CBaseMenuClass, CMenuKeyFunc or item menu
classes in `chronicle/ps2/include`).
# Build-up weapon transfer

`BuildUpWeaponTrans` changes an owned weapon to a new item number and type,
retaining its absorption gauge percentage as the next weapon's level-up
requirement replaces the maximum. It resets the weapon level, adds ten percent
of the next weapon's base status and attribute values, combines special ability
bits, limits parameters, and sets save bit 0x31. It replaces the custom name
only when the old name still equals the old item's default message. Its active
C++ matches retail.

## Question parameter copy layout

`CBaseMenuClass::SetAskParam` copies three overlapping runs from `MENU_ASKMODE_PARA`:
16 words beginning at `cmd_msg`, 16 words beginning at `cmd_color`, and 16 shorts
beginning at `unk_48`. The named overlay arrays in the header keep each run
inside its declared bounds while preserving the existing message, colour and
mark fields. The function separately copies the selected argument and item
pointer fields; it leaves `unk_6`, `unk_72`, `unk_8C` and `unk_90` untouched.

## Compiler flag cleanup

The four local `divbyzerocheck on`/`reset` pairs are redundant with the PS2
compiler flag. Removing them leaves every section and symbol in this unit's
object diff unchanged.

## Pending menu function matches

`CMenuItemInfo::MenuModeMalloc` retains a guarded C++ draft. Its object code
differs from the retail function, including its size, so the normal build uses
the retail assembly until its C++ form matches. `MenuItemInfoCursorSet` now
matches exactly as C++, including its linked image.
`CMenuItemInfo::PushKey` also retains a guarded draft: its raw C++ section is
0x1BB4 bytes against retail's 0x1BA0. Aligning the next function to sixteen
bytes would shift later linked text by 0x20 bytes.
`MenuWeaponBuildUpDraw` retains a guarded draft because MWCC assigns opposite
integer registers to the bottom bar Y coordinate and its X literal before
`PrimQuad`; the four resulting instructions differ from retail when the draft
is compiled alone. Compiling all `NONMATCHING` drafts changes MWCC's register
selection and yields an exact function, but that object contains other
nonmatching functions.

## October 2026 menu draft promotions

- `CBaseMenuClass::MenuItemCommandSelect` is an exact C++ match when compiled alone through mwccgap. Its dispatch selects an item command from a key and button pair, including ask mode handling.
- `MenuItemSelectDiffer` is an exact C++ match when compiled alone through mwccgap. It tests whether an item selection differs from the currently selected item.
- Both functions passed the isolated linked-image verification. `MenuWeaponBuildUpDraw` differs in four instructions in the linked image despite the whole-unit draft comparison reporting a match. `CMenuItemInfo::LRCheck` differs in two branch-delay-slot words at offsets 0x264 and 0x268; the compiler places the zero return value in the delay slot and skips the shared return-value assignment.

## Constructor-backed allocations

`NewMenuActionChara` now uses native placement construction of `CActionChara`; both callers (`IsAskExtend` and `MenuModeMalloc`) are guarded C++ drafts with retail assembly active.

## Remaining matching blockers

The [current matching status](matching-status-20261008.md) records the thirteen
guarded symbols, measured differences, and reconsideration triggers. The
whole-draft comparison and the game-build comparison are distinct:
`MenuWeaponBuildUpDraw` matches with all drafts enabled, but its isolated linked
image differs in four instructions at function offsets 0x960, 0x964, 0x968 and
0x970. The Y-coordinate addition and X-coordinate literal receive opposite
`v0`/`v1` temporaries. Reversing addition operands or naming the bottom Y float
does not resolve the isolated mismatch.

`CalcTex` has seven differing words at offsets 0x394, 0x398, 0x3A4, 0x3A8,
0x3B0, 0x3B8 and 0x3BC. Retail uses `s3` for the held item type and `s0` for the
equipment-search counter; the draft reverses them. This persists in the
isolated game build. Moving either variable's scope, grouping the declarations,
or giving the equipment loop a separate counter does not give a match.

`MenuPosFormValueSetCharaRobo` has nineteen differing words: the zero-WHP red
assignment at 0x238 and the eighteen colour stores for six parts at
0x24C..0x2B8. Retail uses `s3` for red and `s1` for green; the draft reverses
them. Changing initialization order, chained assignment order or declaration
scope does not resolve it. Its isolated game build has the same nineteen
differences. A section ending at 0x498 instead of the manifest extent 0x4A0
is only zero alignment padding, not missing behavior.

`CheckEnableHaveItemNum` has two register-allocation differences: the first
active-slot loop exchanges the `s1` counter and `s3` item pointer, while the
final flag loop exchanges `t0` and `a3` offsets. Reusing the earlier `j` counter
or replacing repeated active-item address expressions with the named pointer
does not resolve these thirteen differing words.

`MenuDataSwap` agrees through the swap behavior but differs in its final
presence flags and two-byte result-table scheduling. Retail's table at
`at_2512` contains `{0, 2}`; `ret_tbl1_2511` contains `{1, 3}`. Splitting the
mixed initializer into a constant initializer and an assignment produces the
same instructions. Swapping the presence-flag declaration order worsens the
comparison. `CommonSetMoveItemClass` differs in its source-row copy and first
equipment branch; the draft adds a `dsll32`/`dsra32` pair before comparing the
copied short with one. An unrolled four-field loop produces the same draft;
holding the short in a local worsens its layout.

The placement-new null-branch blocker occurs in `MenuItemSelectInit`,
`MenuModeMalloc`, `IsAskExtend` and `MenuItemDebugKey`. Retail branches on `v0`
and copies the returned pointer in the delay slot. The draft copies it before
branching on the saved register. Native action-character construction also
emits a shorter constructor sequence than retail. These functions require
the dedicated constructor investigation before further allocation experiments.

## Weapon buildup drawing under the verified profile

`MenuWeaponBuildUpDraw__FRi` passes an isolated production-wrapper build
with the existing Satan's Fiddle profile, GPR `0x30` / FPR `0`. The former
four-word bottom-bar argument discrepancy at +0x960..+0x970 is absent:
the binary32 256.0f (`0x43800000`) X argument and computed Y argument to
`PrimQuad__FP11mgCDrawPrimff9mgRect<i>` have the retail temporary registers.
No new selector or source-body change is required. Removing only the guard
passes all `0x1B0E8` allocated unit bytes and 5,764 resolved relocations.
