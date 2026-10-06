# dngmenu: reverse-engineering notes

Unit: dungeon floor map (`CDngFreeMap`) and the dungeon menu's tree map (`CMenuTreeMap`).
No first-game counterpart (Dark Cloud 1 has no class of either name; nothing equivalent found
in `/home/adubbz/development/chronicle/ps2/include`).

## Header dependencies
- `CMenuTreeMap : CBaseMenuClass` -> `menusys.hpp` (did not exist when this header was written).
- `CDC2Mes mes[8]` by value -> `menucls1.hpp` (exists; asserts `sizeof(CDC2Mes) == 0x2A50`).
- `TRESURE_BOX_FLOOR_INFO tresure` by value -> `dng_event.hpp` (did not exist). Its size must be
  0x1A40C: `AutoSetTreasureBox` (dng_event) does `__nw(0x1A40C)` for one.
- Forward-declared only: `GLID_INFO`, `DNGMAP_ROOM_INFO`, `DNGMAP_ROOT_INFO` (dngfloor owns
  them: `CDngFloorManager` returns/takes `GLID_INFO*`), `CDngFloorManager`, `CSaveDataDungeon`,
  `mgCMemory`, `mgCTexture`.
- The header was verified to compile, and `dngmenu.cpp` to compile with it, against stub
  versions of the three missing types with the sizes above (CBaseMenuClass 0x110 with its
  vptr at 0x10C).

## CDngFreeMap (size 0x110)
Size: `DngTreeMapInit` `__nw(0x110)`; symbol `EventDngMap` (event_func) size 0x110.
Constructor is inline (emitted in `DngTreeMapInit` and `__sinit_event_func_cpp`): it calls
`mgRect<float>::Set(0,0,0,0)` on the rect at 0x20 and on each of the 8 rects at 0x40, then
`Initialize()`. That means `mgRect<T>` has a default constructor `{ Set(0,0,0,0); }` (and the
globals in `__sinit_dngmenu_cpp` show a 4-argument constructor calling `Set`). `mgRect` lives in
`mg_tanime.hpp` with no constructors; they have to be added there before the constructor matches.
`mgRect<float>::Set` (0x1F3D50) is emitted in this unit and comes from that template.

| Off | Type | Name | Evidence |
|---|---|---|---|
| 0x00 | CSaveDataDungeon* | save_dungeon | DngTreeMapInit stores MenuSaveDataDungeonPtr; LoadDngInfo `GetSaveData()+0x1C5B4` |
| 0x04 | CDngFloorManager* | floor_manager | `menu_GetBattleAreaScene()+0x14`; GetRoomGlid/GetNextGlid call through it |
| 0x08 | u8 | active | Initialize =1; Step/Draw return when 0 |
| 0x09 | u8 | unk_9 | only zeroed in Initialize |
| 0x0A | s16 | dng_no | LoadDngInfo param 3; DngTreeMapInit; compared 1..6 in LoadDngInfo, 6 in DrawRoot |
| 0x0C | s16 | mode | 0 Initialize (menu), 1 LoadDngInfo (event); indexes `stepCntTbl_1501[2]` |
| 0x10 | float | back_scroll | DrawBackPattern tile offset, +0.5 per frame |
| 0x14-0x1F | | unk_14 | never accessed |
| 0x20 | mgRect<float> | view_rect | Initialize Set(120,138,420,286); LoadDngInfo Set(60,40,W-40,H-40); CheckIsViewMove |
| 0x30 | s32 | mark_num | DrawRoomOne appends to 0x40[], Step zeroes, Draw iterates |
| 0x34-0x3F | | unk_34 | never accessed |
| 0x40 | mgRect<float>[8] | mark_rect | DrawRoomOne writes `this+0x40+n*0x10`; Draw draws each with name_tex sprite (0xC0,0xD2,0x40,0x2E) |
| 0xC0 | s16 | user_room_no | LoadDngInfo param 4; Initialize -1 |
| 0xC2 | s16 | next_room_no | LoadDngInfo param 5; Initialize -1; heavily adjusted in LoadDngInfo per dungeon |
| 0xC4 | GLID_INFO* | user_glid | SetUserGlid; DrawPlayer |
| 0xC8 | s16 | blink_cnt | Step ++ wrap at 100; DrawTreeMap `%25 < 14` blink |
| 0xCC | GLID_INFO* | select_glid | CMenuTreeMap::Step copies its select_glid here; Draw debug readout; CMenuTreeMap::Draw |
| 0xD0 | s16 | tex_block | InitTexture -1; LoadDngInfo param 2; DeleteTexBlock |
| 0xD4 | mgCTexture* | name_tex | "dtname" / "dtname_dn" |
| 0xD8 | mgCTexture* | map_tex | "dt" / "dt_dn"; Draw reloads its block `*(s16*)tex` |
| 0xDC | mgCTexture* | last_tex | "dtbg"; DrawLast full screen |
| 0xE0 | mgCTexture* | koma_tex | "dngop" / "dngop_dn"; DrawPlayer |
| 0xE4 | DNGMAP_KOMA_POS* | koma_path | LoadDngInfo builds the list |
| 0xE8 | DNGMAP_KOMA_POS* | koma_now | SetKomaMove sets `koma_path->next`; DrawPlayer advances |
| 0xEC | s16 | koma_move | SetKomaMove param |
| 0xF0 | float | alpha | Initialize 128.0; Step clamps 0..128 |
| 0xF4 | s32 | fade_time | FadeIn/FadeOut param; Initialize -1 |
| 0xF8 | float | fade_step | +-128/time |
| 0xFC | s32 | fade_mode | -1 Initialize, 0 FadeIn, 1 FadeOut |
| 0x100/0x104 | float | pos_x/pos_y | scroll; eased towards 0x108/0x10C by 1/5 in Step |
| 0x108/0x10C | float | next_pos_x/y | Initialize 200.0; ResetDngMapPos `256-x`, `208-y` |

`DNGMAP_KOMA_POS` is an invented name (no retail symbol): `{float x, y; next}` nodes allocated
with `mgCMemory::Alloc(1)` (one 16-byte unit; only 0xC used). Built from
`RootHokanTable*`/`RoomHokanTable*` (10 s16 x/y pairs each) offsets in LoadDngInfo.

Not a virtual class (no vtable symbol, no vptr store).

## CMenuTreeMap (size 0x2FBE0)
Size: `DngTreeMapInit` `__nw(0x2FBE0)` (after `Alloc(0x2FC0)` units). Base `CBaseMenuClass` is
0x110 (its ctor does `memset(this, 0, 0x110)`, vptr at 0x10C). Constructor inline in
`DngTreeMapInit`: base ctor, vptr = `__vt__12CMenuTreeMap`, 8x `CDC2Mes` ctor, then
0x11A=0, 0x120=0, 0x153B0=0, base 0x14=0, 0x153C0=1, 0x153B8=0, 0x153BC=0, 0x153C4=0, then
for i<8: `MenuDngMes[i] = &mes[i]; ClsMes::Init(); SetBuff_system(GetSystemMesBuffer())`, then
`MenuDngMes[1]+0x224C = 0x10`. `MenuDngMes` is a file-local static, so the constructor body has
to be defined in dngmenu.cpp (declared only in the header).

| Off | Type | Name | Evidence |
|---|---|---|---|
| 0x110 | float[2] | cursor_pos | passed as `float*` to MenuCursorDraw; eased in Draw |
| 0x118 | s16 | dng_no | DngTreeMapInit param 4 (clamped 0..6) |
| 0x11A | s16 | unk_11a | only ever set 0; tested in FadeInOutMenu and Draw |
| 0x11C | s16 | jump_pay | set 1 when `BattleAreaScene+0x5C == 0 && MenuCommonInfo+0x50 == 1` and not sub map; jump then `AddMoney(-money/2)` |
| 0x120 | GLID_INFO* | select_glid | InitEnd/Step |
| 0x124-0x12F | | unk_124 | never accessed |
| 0x130 | CDC2Mes[8] | mes | stride 0x2A50 to 0x153B0; Step uses mes[1],[3]..[6] (0x2B80, 0x8020, 0xAA70, 0xD4C0, 0xFF10) |
| 0x153B0 | short* | mes_data | "systree.mes" pack file; SetMessData |
| 0x153B4 | s32 | cursor_view | Draw draws cursor when set |
| 0x153B8 | s32 | cursor_reset | Draw snaps cursor_pos and clears |
| 0x153BC | s32 | money_view | Draw: PrimDrawNumber of user money (`UserDataMan+0x44D9C`) |
| 0x153C0 | s32 | help_view | ctor 1; Draw gates the shared help window |
| 0x153C4 | u8 | tresure_loaded | InitEnd sets 1 after CreatTresuarBoxInfo |
| 0x153C8 | TRESURE_BOX_FLOOR_INFO | tresure | CreatTresuarBoxInfo / CheckGeoramaMateria |
| 0x2F7D4 | s32[0x103] | georama_materia | CheckGeoramaMateria output; DrawGeoramaMateria input. Count 0x103 is only the remaining bytes (0x40C) up to the class size; no bound is visible in the code |

Vtable `__vt__12CMenuTreeMap` (0x37BF70, 0x20): `0, 0, IsCreateObject, IsMakeObject,
IsAskExtend, ItemCmdAfter, InitEnd (CMenuTreeMap), ExitEnd` -- all but InitEnd are
`CBaseMenuClass`'s, emitted here as weak inline copies (IsCreateObject returns 1, the other
three return 0, ExitEnd empty). Step calls InitEnd through the vtable at +0x18.

`*(s16*)this` (base) is the menu state: 0 running, 1 opening (calls InitEnd when the fade and
BG read finish), 2 closing, 0xC returning from the save menu. Base 0x14 is the sub-screen
(0 map, 1 floor info, 2 georama list); base 0x2 is a sub-state used with 0xC.

## Enums
- `DNGMAP_MODE` (CDngFreeMap::mode): 0 menu, 1 event (see above).
- `DNGMAP_FADE`: -1/0/1 (Initialize, FadeIn, FadeOut).
- `DNG_TREE_MAP_RESULT`: CMenuTreeMap::Step returns 2 when `DAT_01efc64c == 5` (jump chosen),
  1 otherwise on close, 0 while open; DngTreeMapKey passes it through; menumap's WorldMoveKey
  tests 1 and 2.
- `DNG_TREE_MODE`: `DngTreeMode` static, 0 tree map, 1 save menu (DngTreeMapKey/Draw).
- `CheckDngTreeMapFuncType` gives 2 when `MenuCommonInfo+0x50 == 3` (menumain opens the tree
  map with mode 3 from a save point, setting TreeMapSaveFlag), 1 when it is 1 or
  `TreeMapCallDungeonSubMap`, else 0. No enum: the meaning of mode 1 is not established.

## Globals
Only four of the unit's data symbols are global (others are LOCAL in retail, so `static` in the
.cpp): `TreeMapSaveFlag` (u8, lbu), `TreeMapSaveNum` (s16, lh; menuop increments per save),
`TreeMapCallDungeonSubMap` (u8; menumain SetCommonMenuModeID), `TreeMapCalledWorldMap` (u8;
menumap). Statics of note: `MenuDngMap` (CDngFreeMap*), `CMenuTreePt` (CMenuTreeMap*),
`MenuDngMes` (CDC2Mes*[8]), `MenuTreeMapStack` (mgCMemory, 0x30), `treemap_root_put`
(mgRect<float>), `Floor_Info`, `dng_light_circle`, `dngfreemap_num` (mgRect<int>),
`dng_player_pos` (float[2]), `Floor_InfoTex` (mgCTexture*, "dngfibrd"), `DngTreeMode` (s16),
`GeoramaMateriaNum` (s16), `DngInfoStageNo` (u8).

## Non-members
`CheckGeoramaMateria`, `DrawDngRoomInfo`, `DrawGeoramaMateria` are LOCAL in retail -> static in
the .cpp, not in the header. `ClsMes::Init` (0x1F38E0) is emitted here but declared in
`nd_meswin.hpp`.

## GLID_INFO / DNGMAP_ROOM_INFO as seen from here (for dngfloor's header)
GLID_INFO stride 0x70 (CDngFloorManager +4 array, +8 count, +0xC/+0xE grid width/height):
+0 s16 kind (0 passage, 1 room), +2 s16 grid x, +4 s16 grid y, +0xC GLID_INFO*[4] neighbours,
+0x1C u8 blink, +0x20 DNGMAP_ROOM_INFO / DNGMAP_ROOT_INFO. Room info (relative to +0x20):
+4 char* georama list, +8 s8 floor id, +0xC u32 flags (2 entrance, 4, 8, 0x10 special floors),
+0x18 s16 message no, +0x3E/+0x40 s16 draw offset, +0x42 s8 picture, +0x45 u8 (cleared/visited;
also read as glid+0x65 in DrawGlidCheck), +0x46 u8 mark (bobbing mark, glid+0x66 in InitEnd),
+0x48 float mark phase (DrawRoomOne).

## Current bodies

The unit has 48 functions: 25 matching drafts, three differing drafts and 20 without a draft.
`CheckIsViewMove`, `GetEntranceRoomGlid` and `CDngFreeMap::Draw` retain their C++ drafts under
`NONMATCHING` and use retail assembly in the normal build. The normal build links the unit's
C++ source; it is absent from `migrated_units.txt`.

`ClsMes::Init` remains inline in `nd_meswin.hpp`. Its page, name, item, value and line tables
are cleared through typed array members, with a separate loop index for each table.
The standalone copy at 0x1F38E0 is assembly-backed in `dngmenu.cpp`: its caller in this unit
is assembly-only, so the compiler does not emit the unused inline member here.
`mgRect<float>::Set` is an explicit C++ template specialization. It matches in the draft check
and links at 0x1F3D50, but objdiff does not pair the compiler's `Set__9mgRect<f>Fffff` with
the target's sanitized `Set__9mgRect_f_Fffff`: the normal report lists 24 perfect, zero fuzzy
and 24 assembly functions despite the linked image matching retail in all 11 sections.
