# 64-bit layout audit

Every record the PS2 code checks with `STATIC_ASSERT(sizeof)`, every pointer the PS2 code keeps in 32
bits, every arena allocation sized by a PS2 literal and every byte offset into a pointer-bearing
record, with the boot-trace position where the game first relies on it. The tables below the marker
are written by `scripts/port/layout_audit.py`; `kind`, `fix`, `state` and `note` of a row come from
`scripts/port/layout_decisions.tsv` where the defaults are not the decision. `scripts/port/layout_state.py`
checks the tables against a milestone.

## Vocabulary

- `first_use`: `boot:<n>` is the first step of `scripts/port/boot_trace.md` that reaches a function
  relying on the row. A step followed by deeper steps counts only its own functions; a leaf step
  counts everything its functions call, and in a `→` chain only the last function is expanded.
  Otherwise `debug`, `menu` or `mode` by unit, as in the hardware inventory, or `dead` for a function
  nothing references.
- `state`: `repaired` (the port does the right thing on the host), `guarded-unreached` (the code
  stops the game before it can misbehave, and no milestone trace has reached it yet), `open`, or
  `n/a` (the row does not misbehave on the host: a record nobody reads in its PS2 byte layout, or a
  cast that keeps only low bits or carries a small integer).
- Guards: `override [unit:function] (WP)` is the asserting `PORT_OVERRIDE` T3b gives every inventory
  `stub` row; `callee [unit:function]` is a stub the truncated value only reaches; `guard` is
  `PortPtr32` (stops the game when the pointer is above 4 GiB, which host pointers are) or
  `PortLayoutGuard` at the site.

## Milestones

`layout_state.py --milestone` uses the last boot step each milestone's trace reaches: M1 `boot:2`
(the trace stops at the first stub in `init`), M3 `boot:11` (`mgInit`), M4 `boot:30` (at the
latest, before `sndInitMngr`), M6 `boot:66` (the title screen). Other positions go through
`--position boot:<n>`; `--trace` fails on any guarded row whose function the trace names. Every row
at or before the position must be `repaired` or `n/a`. At T2f, M1 passes; M3 needs the T5
`InitCDFile` and T6 `MainLoop` overrides to flip their cast rows to `repaired`.

## Records and STATIC_ASSERT (interface C)

`port/include/layouts.h` redefines `STATIC_ASSERT` for the port: the assertion stays active for every
record whose host layout equals the PS2 one (no pointer, vtable or plain `long` inside, and the same
size), and is skipped for the records listed in the generated `port/include/layout_records.h`. The
plan's per-record `DC2_LAYOUT_<Record>` macros in the stub headers could not express this: the records
live in shared headers, and the preprocessor cannot take the record name out of the assertion.

Records the game reads or writes in their PS2 byte layout are converted at the boundary rather than
re-laid out:

| record | host form |
|---|---|
| `DATA_HEADER` (DATA.HD4) | `port/src/records/data_hd4.hpp`: 12-byte disc entries, names resolved beside them; plain types, for the asset tools too |
| `SAVEDATA_FORMAT`, `CSaveData`, `CUserDataManager`, `CInventUserData`, `USER_PICTURE_INFO`, `SAVE_CONVERT_WORK` | `port/src/records/save_image.hpp`: the card image spliced around the 30 photo pointers and `costume_bit` (8 bytes on the card, `long` on the host: 4 bytes on Windows) |
| `CDC2AlbumData` | `port/src/records/save_image.hpp`: the album image, photo pointers written as 0 and relinked on load |
| `funcdata` (script files) | `name` is a `u32` under `PORT`, since the record is read in place |

The MDS, MDT, IMG and pack headers (`MDS_HEADER`, `MDT_HEADER`, `MDTOBJ_HEADER`, `mgIMG_FILE_HEADER`,
`mgIMG_HEADER`, `PACK_ENTRY`) hold offsets, not pointers, and keep their PS2 layout on the host; their
assertions stay active. The TIM2 picture header (`TM2_PICTURE`) has no assertion and only fixed-width
members.

## Source repairs

Repairs in `ps2/src` that the generated tables do not list.

| site | function | kind | fix | first_use | state | note |
|---|---|---|---|---|---|---|
| ps2/src/actscript.cpp:475 | _CHECK_PAUSE | argument order | `#ifdef PORT` | mode | repaired | MWCC passes `stack` to `GetStackInt` and then increments it, so `SetStack` writes the next slot |
| ps2/src/actscript.cpp:662 | _GET_BTN | argument order | `#ifdef PORT` | mode | repaired | as `_CHECK_PAUSE` |
| ps2/src/event_func.cpp:13975 | _STREAM_OPEN | argument order | `#ifdef PORT` | mode | repaired | MWCC evaluates left to right: the pack name is this slot, the path the next |
| ps2/src/runscript.cpp:90 | print | `%s` given an int | `#ifdef PORT` | mode | repaired | passes the slot's string |
| ps2/src/movie.cpp:1210 | strFileOpen | `%s` given an uninitialised pointer | override [movie:strFileOpen] (T13e) | boot:61 | guarded-unreached | the `-Wuninitialized` pragma in `port/include/stubs/movie.hpp` goes with the stripped body |
| ps2/src/menumap.cpp:57 | SphidaMenuInit | out-of-bounds write | `PORT_FIX(+ 8)` | menu | repaired | sixteen texture blocks written into an eight-entry table; on the PS2 they land in menuchr's data |
| ps2/src/dngmenu.cpp:1698 | CDngFreeMap::Draw | `sprintf` overflow | `PORT_FIX(+ 64)` | mode | repaired | the line needs more than 32 bytes |
| ps2/src/dataread.cpp:926 | InitFileCache | pointer alignment | `#ifdef PORT` | boot:10 | repaired | aligns through `uintptr_t` |
| ps2/src/menucommon.cpp:864 | MenuCalcBufAlignment | pointer alignment | `#ifdef PORT` | boot:28 | repaired | aligns through `uintptr_t` |
| ps2/src/mg_dataset.cpp:187 | htoi | pointer arithmetic in int | `#ifdef PORT` | boot:49 | repaired | |
| ps2/src/mg_texture.cpp:644 | mgCTextureManager::SearchHash | byte offset | `#ifdef PORT` | boot:19 | repaired | indexes `hash_table` |
| ps2/src/mg_visual.cpp:422 | mgCVisualMDT::CopyMDTDataPointer | pointer arithmetic in int | `#ifdef PORT` | boot:49 | repaired | `intptr_t` addresses |
| ps2/src/snd_seseq.cpp:99 | sndCSeSeqData::LoadSMF | pointer arithmetic in int | `#ifdef PORT` | boot:38 | repaired | |
| ps2/src/scene.cpp:899 | CScene::ClearStack | byte offset | `#ifdef PORT` | mode | repaired | indexes `stack` |
| ps2/src/character.cpp:1450 | CCharacter2::DeleteExtMotion | byte offset | `#ifdef PORT` | mode | repaired | indexes `images` |

## Limits

- The offset detector sees a byte-pointer cast of a pointer-bearing record (or a pointer) followed by
  arithmetic; offsets written through `int` arithmetic or through a struct copy are found only where
  `port_truncations.py` sees the cast.
- The allocation detector sees literal `Alloc`/`stAlloc` counts beside a record or pointer-array type
  in the same expression. `PortBlocks` takes the larger of the PS2 count and the host size plus the
  two spare blocks; arena budgets (`mgCMemory` buffers set from literal sizes) stay PS2-sized and
  overflow as "stack over" if the host records outgrow them.
- Records reached only through a stub row are left to the WP that replaces the stub.

<!-- Generated by scripts/port/layout_audit.py below this line. -->

## Records

598 rows: n/a 589, repaired 9.

| site | record | kind | ps2 | host | carries | first_use | function | fix | state | note | key |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ps2/include/actionchara.hpp:212 | ACCUME_EFFECT | runtime record | 0x330 | 0x338 | frame:pointer | mode | CommonStageClassInit | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACCUME_EFFECT |
| ps2/include/actionchara.hpp:224 | RUN_SCRIPT_ENV | runtime record | 0x8 | 0x10 | item_chara:pointer | mode | CActionChara::EntryThrowItem | assert neutralised | n/a |  | ps2/include/actionchara.hpp RUN_SCRIPT_ENV |
| ps2/include/actionchara.hpp:245 | ACTION_SW_EFFECT | runtime record | 0x20 | 0x38 | chara:pointer, frame0:pointer, frame1:pointer, motion:pointer | boot:52 | CActionChara::Initialize | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACTION_SW_EFFECT |
| ps2/include/actionchara.hpp:265 | ACTION_DAMAGE | runtime record | 0x28 | 0x40 | chara:pointer, damage:pointer, frame0:pointer, frame1:pointer +1 | boot:52 | CActionChara::ResetScript | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACTION_DAMAGE |
| ps2/include/actionchara.hpp:278 | ACTION_OBJECT | runtime record | 0x20 | 0x30 | frame:pointer | boot:52 | CActionChara::ResetScript | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACTION_OBJECT |
| ps2/include/actionchara.hpp:293 | ACTION_BODY_COL | faithful | 0x24 | 0x24 |  | boot:52 | CActionChara::ResetScript | assert active | n/a |  | ps2/include/actionchara.hpp ACTION_BODY_COL |
| ps2/include/actionchara.hpp:308 | ACTION_SOUND | runtime record | 0x14 | 0x18 | chara:pointer | boot:52 | CActionChara::ResetScript | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACTION_SOUND |
| ps2/include/actionchara.hpp:323 | ACTION_ACCELE | faithful | 0x20 | 0x20 |  | boot:52 | CActionChara::Initialize | assert active | n/a |  | ps2/include/actionchara.hpp ACTION_ACCELE |
| ps2/include/actionchara.hpp:336 | ACTION_ACCUME | runtime record | 0x8 | 0x10 | frame:pointer | boot:52 | CActionChara::ResetScript | assert neutralised | n/a |  | ps2/include/actionchara.hpp ACTION_ACCUME |
| ps2/include/actionchara.hpp:348 | ACTION_SHAKE | faithful | 0x8 | 0x8 |  | boot:52 | CActionChara::Initialize | assert active | n/a |  | ps2/include/actionchara.hpp ACTION_SHAKE |
| ps2/include/actionchara.hpp:1098 | CActionChara | runtime record | 0x1030 | 0x16A0 | accume:pointer, accume_effect:pointer, catch_frame:pointer, damage:pointer +40 | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/actionchara.hpp CActionChara |
| ps2/include/actscript.hpp:130 | ACTION_INFO | runtime record | 0x10 | 0x20 | camera:pointer, chara:pointer, env:pointer | mode | CActionChara::EntryThrowItem | assert neutralised | n/a |  | ps2/include/actscript.hpp ACTION_INFO |
| ps2/include/automap.hpp:145 | AUTOMAP_PARTS_INFO | runtime record | 0x18 | 0x20 | name:pointer | mode | CAutoMapGen::CreatFixedMap | assert neutralised | n/a |  | ps2/include/automap.hpp AUTOMAP_PARTS_INFO |
| ps2/include/automap.hpp:157 | MINIMAP_INFO | faithful | 0x290 | 0x290 |  | mode | CMiniMapSymbol::SetMapInfo | assert active | n/a |  | ps2/include/automap.hpp MINIMAP_INFO |
| ps2/include/automap.hpp:175 | MINIMAP_SYMBOL_INFO | faithful | 0x10 | 0x10 |  | mode | CMiniMapSymbol::DrawSymbol | assert active | n/a |  | ps2/include/automap.hpp MINIMAP_SYMBOL_INFO |
| ps2/include/automap.hpp:191 | AUTOMAP_ROOM_INFO | runtime record | 0x18 | 0x20 | table:pointer | mode | CAutoMapGen::Build | assert neutralised | n/a |  | ps2/include/automap.hpp AUTOMAP_ROOM_INFO |
| ps2/include/automap.hpp:206 | AUTOMAP_ROOM | faithful | 0x14 | 0x14 |  | mode | AutoSetTreasureBox | assert active | n/a |  | ps2/include/automap.hpp AUTOMAP_ROOM |
| ps2/include/automap.hpp:245 | CAutoMapParts | runtime record | 0x1C | 0x20 | parts:pointer | mode | CAutoMapGen::CreatDoorRoom | assert neutralised | n/a |  | ps2/include/automap.hpp CAutoMapParts |
| ps2/include/automap.hpp:338 | CMiniMapSymbol | runtime record | 0x180 | 0x1F0 | grid:pointer, info:pointer, map:pointer, parts_table:pointer +2 | mode | CGeoStone::DrawMiniMapSymbol | assert neutralised | n/a |  | ps2/include/automap.hpp CMiniMapSymbol |
| ps2/include/automap.hpp:374 | CHealingPoint | faithful | 0x8 | 0x8 |  | mode | CAutoMapGen::SearchHealingPoint | assert active | n/a |  | ps2/include/automap.hpp CHealingPoint |
| ps2/include/automap.hpp:704 | CAutoMapGen | runtime record | 0x2A0 | 0x360 | gio_parts:pointer, grid:pointer, mini_map:pointer, pot_parts:pointer +2 | mode | AutoSetTreasureBox | assert neutralised | n/a |  | ps2/include/automap.hpp CAutoMapGen |
| ps2/include/cameracontrol.hpp:101 | CameraCtrlParam | faithful | 0x2C | 0x2C |  | mode | CCameraControl::CCameraControl | assert active | n/a |  | ps2/include/cameracontrol.hpp CameraCtrlParam |
| ps2/include/cameracontrol.hpp:411 | CCameraControl | runtime record | 0x1F0 | 0x1F0 | vptr | mode | CCameraControl::AutoMove | assert neutralised | n/a |  | ps2/include/cameracontrol.hpp CCameraControl |
| ps2/include/character.hpp:141 | CHRINFO_KEY_SET | faithful | 0x30 | 0x30 |  | mode | CActionChara::GetWaitToFrame | assert active | n/a |  | ps2/include/character.hpp CHRINFO_KEY_SET |
| ps2/include/character.hpp:156 | CHRINFO_SEQ | faithful | 0x2C | 0x2C |  | mode | CCharacter2::Step | assert active | n/a |  | ps2/include/character.hpp CHRINFO_SEQ |
| ps2/include/character.hpp:170 | CHRINFO_SEQ_HEADER | runtime record | 0x30 | 0x40 | next:pointer, seq:pointer | mode | CCharacter2::GetSeqHeaderPtr | assert neutralised | n/a |  | ps2/include/character.hpp CHRINFO_SEQ_HEADER |
| ps2/include/character.hpp:186 | CHRINFO_SE | faithful | 0x10 | 0x10 |  | mode | CCharacter2::Copy | assert active | n/a |  | ps2/include/character.hpp CHRINFO_SE |
| ps2/include/character.hpp:206 | CHARA_EFFECT_MANAGER | runtime record | 0x1F0 | 0x210 | ctrls:pointer, effects:pointer, emitter_pool:pointer, particle_pool:pointer | mode | CCharacter2::CtrlEffect | assert neutralised | n/a |  | ps2/include/character.hpp CHARA_EFFECT_MANAGER |
| ps2/include/character.hpp:219 | CHRINFO_EFFECT | runtime record | 0x28 | 0x30 | effect:pointer, next:pointer | mode | CCharacter2::DrawEffect | assert neutralised | n/a |  | ps2/include/character.hpp CHRINFO_EFFECT |
| ps2/include/character.hpp:232 | CHRINFO_EFFECT_IMAGE | runtime record | 0x28 | 0x30 | data:pointer, next:pointer | mode | _EFFECT | assert neutralised | n/a |  | ps2/include/character.hpp CHRINFO_EFFECT_IMAGE |
| ps2/include/character.hpp:259 | CHARA_ENTRY_OBJECT | runtime record | 0x10 | 0x18 | frame:pointer | boot:52 | CCharacter2::Initialize | assert neutralised | n/a |  | ps2/include/character.hpp CHARA_ENTRY_OBJECT |
| ps2/include/character.hpp:272 | CHARA_ENTRY_EFFECT | runtime record | 0xC | 0x10 | effect:pointer | boot:52 | CCharacter2::InitEffect | assert neutralised | n/a |  | ps2/include/character.hpp CHARA_ENTRY_EFFECT |
| ps2/include/character.hpp:308 | CCharaFrameMatching | runtime record | 0xC | 0x18 | dst_frame:pointer, src_frame:pointer | boot:52 | CCharacter2::Initialize | assert neutralised | n/a |  | ps2/include/character.hpp CCharaFrameMatching |
| ps2/include/character.hpp:339 | CHARA_SOUND_INFO | runtime record | 0x28 | 0x30 | loop_se:pointer | boot:52 | CCharacter2::Initialize | assert neutralised | n/a |  | ps2/include/character.hpp CHARA_SOUND_INFO |
| ps2/include/character.hpp:366 | CCharaLOD | runtime record | 0x18 | 0x28 | frame:pointer, link:pointer | mode | CCharaLOD::CCharaLOD | assert neutralised | n/a |  | ps2/include/character.hpp CCharaLOD |
| ps2/include/character.hpp:1139 | CCharacter2 | runtime record | 0x660 | 0x9C0 | deform_frame:pointer, dynamic_anime:pointer, effect_image_list:pointer, effect_list:pointer +25 | boot:52 | CActionChara::Initialize | assert neutralised | n/a |  | ps2/include/character.hpp CCharacter2 |
| ps2/include/charasetup.hpp:53 | ROBO_INFO_BODY | faithful | 0x25 | 0x25 |  | mode | GetRoboPartsInfo | assert active | n/a |  | ps2/include/charasetup.hpp ROBO_INFO_BODY |
| ps2/include/charasetup.hpp:68 | ROBO_INFO_DATA | runtime record | 0x24 | 0x40 | arm_name:pointer, hat_file:pointer, model_name:pointer | mode | GetRoboPartsInfo | assert neutralised | n/a |  | ps2/include/charasetup.hpp ROBO_INFO_DATA |
| ps2/include/collision.hpp:79 | CCPoly | faithful | 0x50 | 0x50 |  | mode | BattleLoop | assert active | n/a |  | ps2/include/collision.hpp CCPoly |
| ps2/include/collision.hpp:180 | CCollision | runtime record | 0x40 | 0x30 | vptr | mode | CColFrame::SetCollision | assert neutralised | n/a |  | ps2/include/collision.hpp CCollision |
| ps2/include/collision.hpp:259 | CCollisionMDT | runtime record | 0x50 | 0x40 | poly:pointer, vptr | mode | CCollisionMDT::CCollisionMDT | assert neutralised | n/a |  | ps2/include/collision.hpp CCollisionMDT |
| ps2/include/collision.hpp:357 | CColFrame | runtime record | 0x120 | 0x140 | attr:pointer, bound:pointer, brother:pointer, child:pointer +8 | mode | CColFrame::CColFrame | assert neutralised | n/a |  | ps2/include/collision.hpp CColFrame |
| ps2/include/colprim.hpp:153 | DAMAGE_PARAM | faithful | 0x48 | 0x48 |  | mode | CActionChara::CheckDamage | assert active | n/a |  | ps2/include/colprim.hpp DAMAGE_PARAM |
| ps2/include/colprim.hpp:318 | CColPrim | runtime record | 0x110 | 0x120 | frame:pointer, param:pointer | mode | CActionChara::CheckDamage | assert neutralised | n/a |  | ps2/include/colprim.hpp CColPrim |
| ps2/include/colprim.hpp:412 | CColPrimMan | runtime record | 0x4410 | 0x4810 | prim:pointer, scene:pointer | mode | CColPrimMan::ActivePrimNum | assert neutralised | n/a |  | ps2/include/colprim.hpp CColPrimMan |
| ps2/include/convviewlp.hpp:83 | SAVE_CONVERT_WORK | save record | 0x659C0 | 0x65AB0 | save_data:long, save_data:pointer | debug | SVConvViewInit | host record port/src/records/save_image.hpp | repaired | holds a SAVEDATA_FORMAT | ps2/include/convviewlp.hpp SAVE_CONVERT_WORK |
| ps2/include/crandom.hpp:33 | CRandom | faithful | 0x4 | 0x4 |  | mode | CRandom::nget | assert active | n/a |  | ps2/include/crandom.hpp CRandom |
| ps2/include/dataread.hpp:77 | DATA_HEADER | disk record | 0xC | 0x10 | DATA_HEADER::(anonymous union at /work/chronicletwo/ps2/include/dataread.hpp:68:5):pointer | boot:6 | InitCDFile | host record port/src/records/data_hd4.hpp | repaired | DATA.HD4 entries are 12 bytes with a 32-bit name offset; the override of InitCDFile (T5) reads them through the host record | ps2/include/dataread.hpp DATA_HEADER |
| ps2/include/dataread.hpp:102 | BG_READ_INFO | runtime record | 0x120 | 0x128 | buffer:pointer | boot:39 | LoadFileBG | assert neutralised | n/a |  | ps2/include/dataread.hpp BG_READ_INFO |
| ps2/include/dataread.hpp:118 | FILE_CACHE | runtime record | 0x40 | 0x48 | address:pointer | boot:10 | InitFileCache | assert neutralised | n/a |  | ps2/include/dataread.hpp FILE_CACHE |
| ps2/include/dataread.hpp:133 | PACK_ENTRY | faithful | 0x4C | 0x4C |  | boot:38 | GetPackFile | assert active | n/a |  | ps2/include/dataread.hpp PACK_ENTRY |
| ps2/include/dbg_font.hpp:132 | dbgCJISFont | runtime record | 0x8B0 | 0x8B0 | prev_serno:long | mode | DebugMainDraw | assert neutralised | n/a | plain long: 32-bit on LLP64 | ps2/include/dbg_font.hpp dbgCJISFont |
| ps2/include/dng_debug.hpp:67 | DNG_DEBUG_INFO | faithful | 0x20 | 0x20 |  | mode | BattleAreaBGMCtrl | assert active | n/a |  | ps2/include/dng_debug.hpp DNG_DEBUG_INFO |
| ps2/include/dng_effect.hpp:203 | BattleEffectPrim | faithful | 0x50 | 0x50 |  | mode | BattleEffectMan::AllocEffect | assert active | n/a |  | ps2/include/dng_effect.hpp BattleEffectPrim |
| ps2/include/dng_effect.hpp:219 | BattleEffectChara | runtime record | 0x1C | 0x20 | chara:pointer | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp BattleEffectChara |
| ps2/include/dng_effect.hpp:241 | CHILL_AFTER_HIT_PIECE | faithful | 0x50 | 0x50 |  | mode | CChillAfterHit::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CHILL_AFTER_HIT_PIECE |
| ps2/include/dng_effect.hpp:307 | CChillAfterHit | faithful | 0x7A0 | 0x7A0 |  | mode | CChillAfterHit::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CChillAfterHit |
| ps2/include/dng_effect.hpp:325 | FIRE_AFTER_HIT_FLAME | faithful | 0x30 | 0x30 |  | mode | CFireAfterHit::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp FIRE_AFTER_HIT_FLAME |
| ps2/include/dng_effect.hpp:338 | FIRE_AFTER_HIT_TRAIL | faithful | 0x14 | 0x14 |  | mode | CFireAfterHit::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp FIRE_AFTER_HIT_TRAIL |
| ps2/include/dng_effect.hpp:405 | CFireAfterHit | faithful | 0x940 | 0x940 |  | mode | CFireAfterHit::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CFireAfterHit |
| ps2/include/dng_effect.hpp:421 | TORNADO_PIECE | faithful | 0x30 | 0x30 |  | mode | CTornado::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp TORNADO_PIECE |
| ps2/include/dng_effect.hpp:478 | CTornado | runtime record | 0x380 | 0x390 | model:pointer | mode | CTornado::Draw | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CTornado |
| ps2/include/dng_effect.hpp:495 | THUNDER_SPARK | faithful | 0x40 | 0x40 |  | mode | CThunder::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp THUNDER_SPARK |
| ps2/include/dng_effect.hpp:553 | CThunder | runtime record | 0xDC0 | 0xDE0 | frame:pointer, frame:vptr | mode | CThunder::Draw | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CThunder |
| ps2/include/dng_effect.hpp:602 | CSparcEffect | runtime record | 0xB0 | 0xC0 | model:pointer | mode | CSparcEffect::Draw | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CSparcEffect |
| ps2/include/dng_effect.hpp:658 | CMiniEffPrim | faithful | 0x20 | 0x20 |  | mode | CMiniEffPrim::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CMiniEffPrim |
| ps2/include/dng_effect.hpp:713 | CMiniEffPrimMan | runtime record | 0x940 | 0x990 | draw_prim:pointer | mode | CMiniEffPrimMan::CreatPrim | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CMiniEffPrimMan |
| ps2/include/dng_effect.hpp:770 | CPalletAnime | faithful | 0xE | 0xE |  | boot:52 | CPalletAnime::Initialize | assert active | n/a |  | ps2/include/dng_effect.hpp CPalletAnime |
| ps2/include/dng_effect.hpp:787 | HEALING_LIGHT | faithful | 0x30 | 0x30 |  | mode | CHealingEffectMan::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp HEALING_LIGHT |
| ps2/include/dng_effect.hpp:853 | CHealingEffectMan | faithful | 0x330 | 0x330 |  | mode | CHealingEffectMan::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CHealingEffectMan |
| ps2/include/dng_effect.hpp:890 | CSwordLuminous | runtime record | 0x18 | 0x28 | root_frame:pointer, tip_frame:pointer | mode | CSwordLuminous::Draw | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CSwordLuminous |
| ps2/include/dng_effect.hpp:968 | CSWordAfterImage | runtime record | 0x60 | 0x78 | back_point:pointer, edge_point:pointer, life:pointer, smooth_back:pointer +2 | mode | CSWordAfterImage::AddPoint | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CSWordAfterImage |
| ps2/include/dng_effect.hpp:1036 | CAfterWire | faithful | 0x120 | 0x120 |  | mode | CAfterWire::CAfterWire | assert active | n/a |  | ps2/include/dng_effect.hpp CAfterWire |
| ps2/include/dng_effect.hpp:1129 | CHitEffectImage | runtime record | 0x60 | 0x70 | spark:pointer | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CHitEffectImage |
| ps2/include/dng_effect.hpp:1169 | CFlushEffect | runtime record | 0x40 | 0x40 | follow:pointer | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CFlushEffect |
| ps2/include/dng_effect.hpp:1237 | CPowerLine | runtime record | 0x80 | 0x90 | prim:pointer, source:pointer | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CPowerLine |
| ps2/include/dng_effect.hpp:1298 | CDeadEffect | runtime record | 0x40 | 0x40 | prim:pointer | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CDeadEffect |
| ps2/include/dng_effect.hpp:1348 | CMapEffect_Sprite | faithful | 0x50 | 0x50 |  | mode | CMapEffect_Sprite::Draw | assert active | n/a |  | ps2/include/dng_effect.hpp CMapEffect_Sprite |
| ps2/include/dng_effect.hpp:1394 | CMapEffectsManeger | runtime record | 0x14 | 0x20 | sprite:pointer | mode | CMapEffectsManeger::Draw | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CMapEffectsManeger |
| ps2/include/dng_effect.hpp:1453 | BattleEffectMan | runtime record | 0x48 | 0x70 | chara:pointer, chara_slot:pointer, dead:pointer, dead_prim:pointer +5 | mode | BattleEffectMan::AllocEffect | assert neutralised | n/a |  | ps2/include/dng_effect.hpp BattleEffectMan |
| ps2/include/dng_effect.hpp:1651 | CWeaponElement | runtime record | 0x7C0 | 0x7C0 | origin:pointer | mode | CMonsterMan::CheckDamage | assert neutralised | n/a |  | ps2/include/dng_effect.hpp CWeaponElement |
| ps2/include/dng_event.hpp:136 | TRESURE_BOX_ITEM | faithful | 0xC | 0xC |  | mode | AutoSetTreasureBox | assert active | n/a |  | ps2/include/dng_event.hpp TRESURE_BOX_ITEM |
| ps2/include/dng_event.hpp:149 | TRESURE_BOX_GROUP | faithful | 0x488 | 0x488 |  | mode | PickupRandomItem | assert active | n/a |  | ps2/include/dng_event.hpp TRESURE_BOX_GROUP |
| ps2/include/dng_event.hpp:161 | TRESURE_BOX_FLOOR | faithful | 0x104 | 0x104 |  | mode | PickupRandomItem | assert active | n/a |  | ps2/include/dng_event.hpp TRESURE_BOX_FLOOR |
| ps2/include/dng_event.hpp:177 | TRESURE_BOX_FLOOR_INFO | faithful | 0x1A40C | 0x1A40C |  | mode | AutoSetTreasureBox | assert active | n/a |  | ps2/include/dng_event.hpp TRESURE_BOX_FLOOR_INFO |
| ps2/include/dng_event.hpp:235 | CStartupEpisodeTitle | runtime record | 0x18 | 0x20 | mes:pointer | mode | CStartupEpisodeTitle::DrawEpisode | assert neutralised | n/a |  | ps2/include/dng_event.hpp CStartupEpisodeTitle |
| ps2/include/dng_event.hpp:252 | MESSAGE_TASK | runtime record | 0x90 | 0x98 | message:pointer, next:pointer | mode | MessageTaskManager::Clear | assert neutralised | n/a |  | ps2/include/dng_event.hpp MESSAGE_TASK |
| ps2/include/dng_event.hpp:317 | MessageTaskManager | runtime record | 0x36C | 0x3A8 | mes:pointer, task:pointer, top:pointer | mode | InitDungeonMain | assert neutralised | n/a |  | ps2/include/dng_event.hpp MessageTaskManager |
| ps2/include/dng_event.hpp:367 | CRedMarkModel | runtime record | 0x90 | 0x80 | frame:pointer, vptr | mode | CRedMarkModel::Draw | assert neutralised | n/a |  | ps2/include/dng_event.hpp CRedMarkModel |
| ps2/include/dng_event.hpp:442 | CGeoStone | runtime record | 0x670 | 0x9C0 | deform_frame:pointer, dynamic_anime:pointer, effect_image_list:pointer, effect_list:pointer +25 | mode | AutoSetTreasureBox | assert neutralised | n/a |  | ps2/include/dng_event.hpp CGeoStone |
| ps2/include/dng_event.hpp:549 | CRandomCircle | runtime record | 0x6A0 | 0xA00 | model:pointer, model:vptr | mode | CRandomCircle::CheckArea | assert neutralised | n/a |  | ps2/include/dng_event.hpp CRandomCircle |
| ps2/include/dng_event.hpp:607 | CTreasureBox | runtime record | 0x70 | 0x80 | frame:pointer, lid_frame:pointer, model:pointer, vptr | mode | CTreasureBox::Draw | assert neutralised | n/a |  | ps2/include/dng_event.hpp CTreasureBox |
| ps2/include/dng_event.hpp:737 | CTreasureBoxManager | runtime record | 0xAA0 | 0xC30 | box:pointer, box:vptr, col_frame:pointer, model:pointer | mode | AutoSetTreasureBox | assert neutralised | n/a |  | ps2/include/dng_event.hpp CTreasureBoxManager |
| ps2/include/dng_hud.hpp:140 | CLevelupInfo | faithful | 0x38 | 0x38 |  | mode | CLevelupInfo::Draw | assert active | n/a |  | ps2/include/dng_hud.hpp CLevelupInfo |
| ps2/include/dng_hud.hpp:218 | CPiyori | runtime record | 0x20 | 0x28 | target:pointer | mode | CPiyori::Draw | assert neutralised | n/a |  | ps2/include/dng_hud.hpp CPiyori |
| ps2/include/dng_hud.hpp:274 | CGiftMark | runtime record | 0x14 | 0x18 | chara:pointer | mode | CGiftMark::Draw | assert neutralised | n/a |  | ps2/include/dng_hud.hpp CGiftMark |
| ps2/include/dng_hud.hpp:307 | CEnemyGekirin | faithful | 0x2 | 0x2 |  | mode | CEnemyGekirin::Draw | assert active | n/a |  | ps2/include/dng_hud.hpp CEnemyGekirin |
| ps2/include/dng_hud.hpp:386 | CEnemyLifeGage | faithful | 0x50 | 0x50 |  | mode | CEnemyLifeGage::Draw | assert active | n/a |  | ps2/include/dng_hud.hpp CEnemyLifeGage |
| ps2/include/dng_hud.hpp:480 | CDamageScore | faithful | 0x90 | 0x90 |  | mode | CDamageScore::CDamageScore | assert active | n/a |  | ps2/include/dng_hud.hpp CDamageScore |
| ps2/include/dng_hud.hpp:530 | CDamageScore2 | faithful | 0x28 | 0x28 |  | mode | CDamageScore2::Draw | assert active | n/a |  | ps2/include/dng_hud.hpp CDamageScore2 |
| ps2/include/dng_hud.hpp:596 | CLockOnModel | runtime record | 0xB0 | 0xB0 | frame:pointer, mes:pointer, name:pointer, scene:pointer +1 | mode | CLockOnModel::Draw | assert neutralised | n/a |  | ps2/include/dng_hud.hpp CLockOnModel |
| ps2/include/dng_hud.hpp:631 | CWarningGage2 | faithful | 0x20 | 0x20 |  | mode | CWarningGage2::Draw | assert active | n/a |  | ps2/include/dng_hud.hpp CWarningGage2 |
| ps2/include/dng_main.hpp:140 | DNG_STATUS | faithful | 0x1C | 0x1C |  | mode | CActionChara::EntryThrowItem | assert active | n/a |  | ps2/include/dng_main.hpp DNG_STATUS |
| ps2/include/dng_object.hpp:146 | CRocketLauncher | runtime record | 0x190 | 0x1A0 | model:pointer, trail_texture:pointer | mode | CRocketLauncher::Draw | assert neutralised | n/a |  | ps2/include/dng_object.hpp CRocketLauncher |
| ps2/include/dng_object.hpp:208 | CRocketLauncherMan | runtime record | 0x2580 | 0x2700 | rocket:pointer | mode | CRocketLauncherMan::Clear | assert neutralised | n/a |  | ps2/include/dng_object.hpp CRocketLauncherMan |
| ps2/include/dng_object.hpp:247 | CMachineGun | faithful | 0x390 | 0x390 |  | mode | CMachineGun::Set | assert active | n/a |  | ps2/include/dng_object.hpp CMachineGun |
| ps2/include/dng_object.hpp:339 | CLaserGun | runtime record | 0x130 | 0x140 | model:pointer, trail_texture:pointer | mode | CLaserGun::Draw | assert neutralised | n/a |  | ps2/include/dng_object.hpp CLaserGun |
| ps2/include/dng_object.hpp:401 | CLaserGunMan | runtime record | 0x1300 | 0x1400 | laser:pointer | mode | CLaserGunMan::Clear | assert neutralised | n/a |  | ps2/include/dng_object.hpp CLaserGunMan |
| ps2/include/dng_object.hpp:535 | CPullItem | faithful | 0x80 | 0x80 |  | mode | CMonsterMan::CheckDamage | assert active | n/a |  | ps2/include/dng_object.hpp CPullItem |
| ps2/include/dng_object.hpp:568 | CPullItemManager | runtime record | 0x8 | 0x10 | list:pointer | mode | CPullItemManager::Clear | assert neutralised | n/a |  | ps2/include/dng_object.hpp CPullItemManager |
| ps2/include/dng_object.hpp:651 | CRoboVoiceSystem | faithful | 0x18 | 0x18 |  | mode | CRoboVoiceSystem::SetStatus | assert active | n/a |  | ps2/include/dng_object.hpp CRoboVoiceSystem |
| ps2/include/dng_status.hpp:27 | SP_RGBA | faithful | 0x10 | 0x10 |  | mode | DrawMainUnitStatusBord | assert active | n/a |  | ps2/include/dng_status.hpp SP_RGBA |
| ps2/include/dngfloor.hpp:157 | DNGMAP_ROOT_INFO | faithful | 0x5 | 0x5 |  | mode | CDngFloorManager::CheckDrawGlidInfo | assert active | n/a |  | ps2/include/dngfloor.hpp DNGMAP_ROOT_INFO |
| ps2/include/dngfloor.hpp:199 | DNGMAP_ROOM_INFO | runtime record | 0x50 | 0x58 | title:pointer, unk_0:pointer | mode | CDngFloorManager::CheckDrawGlidInfo | assert neutralised | n/a |  | ps2/include/dngfloor.hpp DNGMAP_ROOM_INFO |
| ps2/include/dngfloor.hpp:224 | GLID_INFO | runtime record | 0x70 | 0x90 | GLID_INFO::(anonymous union at /work/chronicletwo/ps2/include/dngfloor.hpp:218:5):pointer, link_glid:pointer | mode | CDngFloorManager::CheckDrawGlidInfo | assert neutralised | n/a |  | ps2/include/dngfloor.hpp GLID_INFO |
| ps2/include/dngfloor.hpp:454 | CDngFloorManager | runtime record | 0x10 | 0x18 | glid_info:pointer | boot:0 | CDngFloorManager::Initialize | assert neutralised | n/a |  | ps2/include/dngfloor.hpp CDngFloorManager |
| ps2/include/dngmenu.hpp:183 | DNGMAP_KOMA_POS | runtime record | 0xC | 0x10 | next:pointer | menu | CDngFreeMap::DrawPlayer | assert neutralised | n/a |  | ps2/include/dngmenu.hpp DNGMAP_KOMA_POS |
| ps2/include/dngmenu.hpp:510 | CDngFreeMap | runtime record | 0x110 | 0x150 | floor_manager:pointer, koma_now:pointer, koma_path:pointer, koma_tex:pointer +6 | mode | _DNGMAP_LOAD | assert neutralised | n/a |  | ps2/include/dngmenu.hpp CDngFreeMap |
| ps2/include/dngmenu.hpp:511 | mgRect<float> | faithful | 0x10 | 0x10 |  | menu | CDngFreeMap::CheckIsViewMove | assert active | n/a |  | ps2/include/dngmenu.hpp mgRect<float> |
| ps2/include/dngmenu.hpp:599 | CMenuTreeMap | runtime record | 0x2FBE0 | 0x2FD10 | ask_para:pointer, mes:pointer, mes_data:pointer, script:pointer +2 | menu | CMenuTreeMap::CMenuTreeMap | assert neutralised | n/a |  | ps2/include/dngmenu.hpp CMenuTreeMap |
| ps2/include/dynamicanime.hpp:40 | DA_FRAME_POSE | runtime record | 0x10 | 0x18 | vertex_id:pointer | mode | CDynamicAnime::FramePose | assert neutralised | n/a |  | ps2/include/dynamicanime.hpp DA_FRAME_POSE |
| ps2/include/dynamicanime.hpp:55 | DA_FIX_VERTEX | faithful | 0x20 | 0x20 |  | mode | CDynamicAnime::NewFixVertexTable | assert active | n/a |  | ps2/include/dynamicanime.hpp DA_FIX_VERTEX |
| ps2/include/dynamicanime.hpp:68 | DA_BIND_VERTEX | faithful | 0x10 | 0x10 |  | mode | CDynamicAnime::NewBindVertexTable | assert active | n/a |  | ps2/include/dynamicanime.hpp DA_BIND_VERTEX |
| ps2/include/dynamicanime.hpp:81 | DA_BOUNDING_BOX | faithful | 0x30 | 0x30 |  | mode | CDynamicAnime::NewBoundingBoxTable | assert active | n/a |  | ps2/include/dynamicanime.hpp DA_BOUNDING_BOX |
| ps2/include/dynamicanime.hpp:128 | CDACollision | runtime record | 0xD0 | 0xC0 | frame:pointer, vptr | mode | CDAColPipe::CheckHit | assert neutralised | n/a |  | ps2/include/dynamicanime.hpp CDACollision |
| ps2/include/dynamicanime.hpp:169 | CDAColPipe | runtime record | 0xE0 | 0xD0 | frame:pointer, vptr | mode | CDAColPipe::CheckHit | assert neutralised | n/a |  | ps2/include/dynamicanime.hpp CDAColPipe |
| ps2/include/dynamicanime.hpp:565 | CDynamicAnime | runtime record | 0x90 | 0xE0 | bbox:pointer, bind_vertex:pointer, collision:pointer, draw_frame:pointer +9 | mode | CCharacter2::Copy | assert neutralised | n/a |  | ps2/include/dynamicanime.hpp CDynamicAnime |
| ps2/include/editcoll.hpp:118 | CEditCollision | runtime record | 0x50 | 0x40 | poly:pointer, vptr | debug | CEditCollision::ApplyMatrix | assert neutralised | n/a |  | ps2/include/editcoll.hpp CEditCollision |
| ps2/include/editctrl.hpp:119 | EditMoveCharaInfo | runtime record | 0x130 | 0x140 | polys:pointer | mode | CharaControl | assert neutralised | n/a |  | ps2/include/editctrl.hpp EditMoveCharaInfo |
| ps2/include/editdata.hpp:128 | EditAnalyzeDataSrc | runtime record | 0x1C | 0x28 | message:pointer, off_parts:pointer, on_parts:pointer | mode | FutureMapSelect | assert neutralised | n/a |  | ps2/include/editdata.hpp EditAnalyzeDataSrc |
| ps2/include/editdata.hpp:163 | EditAnalyzeSrc | runtime record | 0x340 | 0x500 | condition:pointer, data:pointer | debug | CEditData::GetAnalyzeSrc | assert neutralised | n/a |  | ps2/include/editdata.hpp EditAnalyzeSrc |
| ps2/include/editdata.hpp:190 | EditDataParts | faithful | 0x24 | 0x24 |  | boot:8 | CEditData::InitPlaceData | assert active | n/a |  | ps2/include/editdata.hpp EditDataParts |
| ps2/include/editdata.hpp:205 | EditDataGrid | faithful | 0x18 | 0x18 |  | debug | CEditMap::LoadData | assert active | n/a |  | ps2/include/editdata.hpp EditDataGrid |
| ps2/include/editdata.hpp:226 | EditDataHouse | faithful | 0x10 | 0x10 |  | boot:8 | CEditData::InitPlaceData | assert active | n/a |  | ps2/include/editdata.hpp EditDataHouse |
| ps2/include/editdata.hpp:250 | EditDataAnalyze | faithful | 0xD0 | 0xD0 |  | mode | gcALL_GEO_PARTS | assert active | n/a |  | ps2/include/editdata.hpp EditDataAnalyze |
| ps2/include/editdata.hpp:262 | EditPlaceLog | faithful | 0x4 | 0x4 |  | boot:8 | CEditData::InitPlaceData | assert active | n/a |  | ps2/include/editdata.hpp EditPlaceLog |
| ps2/include/editdata.hpp:441 | CEditData | faithful | 0x5510 | 0x5510 |  | boot:8 | CEditData::InitPlaceData | assert active | n/a |  | ps2/include/editdata.hpp CEditData |
| ps2/include/editdebug.hpp:136 | EditDebugInfo | runtime record | 0x3C | 0x50 | edit_data:pointer, load_buff:pointer, menu_buff:pointer, scene:pointer | debug | EditDebugLoop | assert neutralised | n/a |  | ps2/include/editdebug.hpp EditDebugInfo |
| ps2/include/editeff.hpp:87 | EditStarParticle | faithful | 0x20 | 0x20 |  | debug | CStarEffect::Draw | assert active | n/a |  | ps2/include/editeff.hpp EditStarParticle |
| ps2/include/editeff.hpp:155 | CStarEffect | runtime record | 0x100 | 0x130 | particle:pointer, sprite:pointer, sprite:vptr, texture:pointer +1 | debug | CStarEffect::CStarEffect | assert neutralised | n/a |  | ps2/include/editeff.hpp CStarEffect |
| ps2/include/editeff.hpp:222 | CPaintEffect | runtime record | 0x400 | 0x440 | sprite:pointer, sprite:vptr, texture:pointer, vptr | debug | CPaintEffect::CPaintEffect | assert neutralised | n/a |  | ps2/include/editeff.hpp CPaintEffect |
| ps2/include/editeff.hpp:281 | CPlaceAnime | runtime record | 0x90 | 0xA0 | parts:pointer | debug | CPlaceAnime::Draw | assert neutralised | n/a |  | ps2/include/editeff.hpp CPlaceAnime |
| ps2/include/editevent.hpp:199 | CEditEvent | faithful | 0x150 | 0x150 |  | debug | CEditEvent::Draw | assert active | n/a |  | ps2/include/editevent.hpp CEditEvent |
| ps2/include/editevent.hpp:222 | GeoFuncParam | runtime record | 0x4 | 0x8 | scene:pointer | mode | _GEORAMA_FUNC | assert neutralised | n/a |  | ps2/include/editevent.hpp GeoFuncParam |
| ps2/include/editexception.hpp:42 | FirePowder | faithful | 0x20 | 0x20 |  | debug | DrawFirePowder | assert active | n/a |  | ps2/include/editexception.hpp FirePowder |
| ps2/include/editexception.hpp:72 | CGeyserEffectPoint | faithful | 0x30 | 0x30 |  | debug | CGeyserEffect::CreatePacket | assert active | n/a |  | ps2/include/editexception.hpp CGeyserEffectPoint |
| ps2/include/editexception.hpp:153 | CGeyserEffect | runtime record | 0x80 | 0xC0 | point:pointer, sprite:pointer, sprite:vptr, texture:pointer | debug | CGeyserEffect::CGeyserEffect | assert neutralised | n/a |  | ps2/include/editexception.hpp CGeyserEffect |
| ps2/include/editinfo.hpp:29 | ePlaceData | faithful | 0x20 | 0x20 |  | debug | CEditInfoMngr::SeteFixPartsTable | assert active | n/a |  | ps2/include/editinfo.hpp ePlaceData |
| ps2/include/editinfo.hpp:135 | CEditInfoMngr | runtime record | 0x18 | 0x30 | fix_parts:pointer, init_parts:pointer, parts_info:pointer | debug | CEditInfoMngr::GetePartsInfo | assert neutralised | n/a |  | ps2/include/editinfo.hpp CEditInfoMngr |
| ps2/include/editmap.hpp:70 | EP_PLACE_INFO | faithful | 0x48 | 0x48 |  | debug | CEditMap::BurnEditParts | assert active | n/a |  | ps2/include/editmap.hpp EP_PLACE_INFO |
| ps2/include/editmap.hpp:1065 | CEditMap::RemoveInfo | runtime record | 0x494 | 0x4A0 | color:pointer, paint_num:pointer | debug | BurnEditParts | assert neutralised | n/a |  | ps2/include/editmap.hpp CEditMap::RemoveInfo |
| ps2/include/editmap.hpp:1066 | CEditMap | runtime record | 0x10F0 | 0x13D0 | balance_parts:pointer, camera_info:pointer, draw_parts:pointer, draw_rect:pointer +27 | boot:49 | CEditMap::ClearHouse | assert neutralised | n/a |  | ps2/include/editmap.hpp CEditMap |
| ps2/include/editmenu.hpp:126 | GEORAMA_PARTS_LIST_ITEM | faithful | 0x38 | 0x38 |  | debug | CMenuGeorama::ArrangePartsList | assert active | n/a |  | ps2/include/editmenu.hpp GEORAMA_PARTS_LIST_ITEM |
| ps2/include/editmenu.hpp:138 | GEORAMA_LIST_INFO | faithful | 0x8 | 0x8 |  | debug | CMenuGeorama::CalcCursorPosition | assert active | n/a |  | ps2/include/editmenu.hpp GEORAMA_LIST_INFO |
| ps2/include/editmenu.hpp:450 | CMenuGeorama | runtime record | 0x1B900 | 0x1B990 | analyze_form:pointer, analyze_percent_form:pointer, ask_para:pointer, cpview_form:pointer +12 | debug | CMenuGeorama::ArrangePartsList | assert neutralised | n/a |  | ps2/include/editmenu.hpp CMenuGeorama |
| ps2/include/editmenu.hpp:554 | CRemovalMenu | runtime record | 0x1500 | 0x1C20 | ask_para:pointer, chara:pointer, chara:vptr, chara_stack:pointer +13 | debug | CRemovalMenu::CRemovalMenu | assert neutralised | n/a |  | ps2/include/editmenu.hpp CRemovalMenu |
| ps2/include/editmode.hpp:78 | UNDO_DATA | faithful | 0x30 | 0x30 |  | debug | ClearUndoFlag | assert active | n/a |  | ps2/include/editmode.hpp UNDO_DATA |
| ps2/include/editparts.hpp:92 | EditPartsMaterial | faithful | 0x8 | 0x8 |  | debug | CEditPartsInfo::GetMaterial | assert active | n/a |  | ps2/include/editparts.hpp EditPartsMaterial |
| ps2/include/editparts.hpp:227 | CEditPartsInfo | runtime record | 0x280 | 0x250 | col_area1:pointer, col_area1:vptr, col_area3:pointer, col_area3:vptr +10 | debug | AnalyzeBenietio | assert neutralised | n/a |  | ps2/include/editparts.hpp CEditPartsInfo |
| ps2/include/editparts.hpp:259 | CEditHouse | faithful | 0x10 | 0x10 |  | boot:49 | CEditMap::ClearHouse | assert active | n/a |  | ps2/include/editparts.hpp CEditHouse |
| ps2/include/editparts.hpp:494 | CEditParts::WallInfo | faithful | 0x40 | 0x40 |  | debug | CEditMap::CheckWallEditParts | assert active | n/a |  | ps2/include/editparts.hpp CEditParts::WallInfo |
| ps2/include/editparts.hpp:495 | CEditParts | runtime record | 0x330 | 0x390 | allocation_address:pointer, anime_list:pointer, frame:pointer, frame:vptr +8 | mode | _EOH_SYNC_EDIT_OBJ | assert neutralised | n/a |  | ps2/include/editparts.hpp CEditParts |
| ps2/include/editriver.hpp:63 | CGridData | faithful | 0x14 | 0x14 |  | debug | CEditGrid::Clear | assert active | n/a |  | ps2/include/editriver.hpp CGridData |
| ps2/include/editriver.hpp:262 | CEditGrid | runtime record | 0x130 | 0x140 | data:pointer | debug | CEditGrid::Check | assert neutralised | n/a |  | ps2/include/editriver.hpp CEditGrid |
| ps2/include/effect.hpp:140 | EFFECT_PARAM | runtime record | 0x1B0 | 0x1C0 | texture:pointer | boot:49 | InitEffectParam | assert neutralised | n/a |  | ps2/include/effect.hpp EFFECT_PARAM |
| ps2/include/effect.hpp:223 | CEffect | runtime record | 0x200 | 0x210 | param:pointer | boot:49 | CEffect::CEffect | assert neutralised | n/a |  | ps2/include/effect.hpp CEffect |
| ps2/include/effect.hpp:394 | CEffectCtrl | runtime record | 0x310 | 0x310 | texture:pointer | boot:49 | CEffectCtrl::CEffectCtrl | assert neutralised | n/a |  | ps2/include/effect.hpp CEffectCtrl |
| ps2/include/effect.hpp:558 | CEffectManager | runtime record | 0x184 | 0x190 | ctrls:pointer, effects:pointer | boot:49 | CEffectList::LoadEFPFile | assert neutralised | n/a |  | ps2/include/effect.hpp CEffectManager |
| ps2/include/effectlist.hpp:118 | CEffectList | runtime record | 0x18 | 0x28 | managers:pointer, name:pointer, pack:pointer, sprites:pointer | boot:49 | CEffectList::LoadEFPFile | assert neutralised | n/a |  | ps2/include/effectlist.hpp CEffectList |
| ps2/include/effectlist.hpp:301 | CFadeInOut | runtime record | 0x30 | 0x38 | cross_texture:pointer | boot:0 | CFadeInOut::Initialize | assert neutralised | n/a |  | ps2/include/effectlist.hpp CFadeInOut |
| ps2/include/effscript.hpp:222 | EFF_SPT_BASE_DEF | faithful | 0x64 | 0x64 |  | mode | CEffectScriptMan::BuildBase | assert active | n/a |  | ps2/include/effscript.hpp EFF_SPT_BASE_DEF |
| ps2/include/effscript.hpp:239 | EFF_SPT_BASE | runtime record | 0x1C | 0x28 | chara:pointer, script:pointer | mode | CEffectScriptMan::BuildBase | assert neutralised | n/a |  | ps2/include/effscript.hpp EFF_SPT_BASE |
| ps2/include/effscript.hpp:277 | _ES_SPRITE | faithful | 0x110 | 0x110 |  | mode | CEffectScriptMan::AssignSprite | assert active | n/a |  | ps2/include/effscript.hpp _ES_SPRITE |
| ps2/include/effscript.hpp:326 | _EFF_SCRIPT | runtime record | 0x150 | 0x1D0 | chara:pointer, chara_work:pointer, colprim:pointer, next:pointer +6 | mode | CEffectScriptMan::AllClearEffSpt | assert neutralised | n/a |  | ps2/include/effscript.hpp _EFF_SCRIPT |
| ps2/include/effscript.hpp:866 | CEffectScriptMan | runtime record | 0x1190 | 0x22E0 | base:pointer, head:pointer, load_buffer:pointer, memory:pointer +6 | mode | CActionChara::DrawEffect | assert neutralised | n/a |  | ps2/include/effscript.hpp CEffectScriptMan |
| ps2/include/event_func.hpp:1148 | ED_EVENT_INFO | runtime record | 0x12A0 | 0x12B0 | dng_event_parts:pointer, npc_talk_text:pointer | boot:57 | ClsMes::DrawMesWin | assert neutralised | n/a |  | ps2/include/event_func.hpp ED_EVENT_INFO |
| ps2/include/event_func.hpp:1231 | CEoh | runtime record | 0x10 | 0x18 | CEoh::(anonymous union at /work/chronicletwo/ps2/include/event_func.hpp:1162:5):pointer | mode | CEoh::Set | assert neutralised | n/a |  | ps2/include/event_func.hpp CEoh |
| ps2/include/event_func.hpp:1674 | CEohMother | runtime record | 0x200 | 0x300 | eoh:pointer | mode | CEohMother::CEohMother | assert neutralised | n/a |  | ps2/include/event_func.hpp CEohMother |
| ps2/include/event_func.hpp:1691 | ARG_DATA | runtime record | 0x8 | 0x10 | ARG_DATA::(anonymous union at /work/chronicletwo/ps2/include/event_func.hpp:1684:5):pointer | mode | FindArgData | assert neutralised | n/a |  | ps2/include/event_func.hpp ARG_DATA |
| ps2/include/event_func.hpp:1705 | ARG_LIST | runtime record | 0x10 | 0x20 | args:pointer, next:pointer | mode | FindArgData | assert neutralised | n/a |  | ps2/include/event_func.hpp ARG_LIST |
| ps2/include/event_func.hpp:1738 | CEventScriptArg | runtime record | 0x10 | 0x20 | list:pointer, memory:pointer | mode | CEventScriptArg::BuildArgData | assert neutralised | n/a |  | ps2/include/event_func.hpp CEventScriptArg |
| ps2/include/event_func.hpp:1828 | CRaster | faithful | 0x2C | 0x2C |  | mode | CRaster::DrawRaster | assert active | n/a |  | ps2/include/event_func.hpp CRaster |
| ps2/include/event_func.hpp:1975 | CScreenEffect | runtime record | 0x4C | 0x60 | mono_flash_texture:pointer, sepia_texture:pointer | mode | CScreenEffect::CScreenEffect | assert neutralised | n/a |  | ps2/include/event_func.hpp CScreenEffect |
| ps2/include/eventedit.hpp:72 | EventEditInfo | runtime record | 0x40 | 0x50 | memory:pointer | debug | ChkEventEditStart | assert neutralised | n/a |  | ps2/include/eventedit.hpp EventEditInfo |
| ps2/include/eventsprite.hpp:80 | CMarker | faithful | 0x4 | 0x4 |  | mode | CMarker::Draw | assert active | n/a |  | ps2/include/eventsprite.hpp CMarker |
| ps2/include/eventsprite.hpp:216 | CEventSprite | faithful | 0x88 | 0x88 |  | mode | CEventSprite::Draw | assert active | n/a |  | ps2/include/eventsprite.hpp CEventSprite |
| ps2/include/eventsprite.hpp:345 | CEventSpriteMother | faithful | 0x440 | 0x440 |  | mode | CEventSpriteMother::Draw | assert active | n/a |  | ps2/include/eventsprite.hpp CEventSpriteMother |
| ps2/include/eventsprite.hpp:593 | CEventSprite2 | faithful | 0x80 | 0x80 |  | mode | CEoh::Set | assert active | n/a |  | ps2/include/eventsprite.hpp CEventSprite2 |
| ps2/include/fishing.hpp:100 | FISH_PARAM | runtime record | 0x54 | 0x60 | file_name:pointer, name:pointer | mode | FishLoadBG | assert neutralised | n/a |  | ps2/include/fishing.hpp FISH_PARAM |
| ps2/include/fishing.hpp:112 | FISHING_ROD_DATA | faithful | 0x18 | 0x18 |  | mode | GetUkiWaitTime | assert active | n/a |  | ps2/include/fishing.hpp FISHING_ROD_DATA |
| ps2/include/fishing.hpp:131 | FISH_DATA | faithful | 0x24 | 0x24 |  | mode | FishLoadBG | assert active | n/a |  | ps2/include/fishing.hpp FISH_DATA |
| ps2/include/fishing.hpp:144 | FISH_PLACE | faithful | 0xC | 0xC |  | mode | FISH_PLACE_MAP::SetFishPlace | assert active | n/a |  | ps2/include/fishing.hpp FISH_PLACE |
| ps2/include/fishing.hpp:183 | FISH_PLACE_MAP | runtime record | 0x88 | 0x90 | name:pointer | mode | FISH_PLACE_MAP::CheckFishPlace | assert neutralised | n/a |  | ps2/include/fishing.hpp FISH_PLACE_MAP |
| ps2/include/fishingobj.hpp:40 | FISH_POINT | faithful | 0x30 | 0x30 |  | mode | BindFishObj | assert active | n/a |  | ps2/include/fishingobj.hpp FISH_POINT |
| ps2/include/fishingobj.hpp:54 | FISH_BIND | runtime record | 0x10 | 0x18 | point0:pointer, point1:pointer | mode | CFishObj::BindStep | assert neutralised | n/a |  | ps2/include/fishingobj.hpp FISH_BIND |
| ps2/include/fishingobj.hpp:68 | FISH_FLOAT | runtime record | 0x10 | 0x18 | point0:pointer, point1:pointer | mode | CFishObj::FloatPoint | assert neutralised | n/a |  | ps2/include/fishingobj.hpp FISH_FLOAT |
| ps2/include/fishingobj.hpp:82 | FISH_ROD_SEGMENT | faithful | 0x10 | 0x10 |  | mode | InitRodPoint | assert active | n/a |  | ps2/include/fishingobj.hpp FISH_ROD_SEGMENT |
| ps2/include/fishingobj.hpp:147 | CFishObj | runtime record | 0x3D0 | 0x4E0 | bind:pointer, float_info:pointer | mode | BindFishObj | assert neutralised | n/a |  | ps2/include/fishingobj.hpp CFishObj |
| ps2/include/font.hpp:28 | RECT | faithful | 0x10 | 0x10 |  | boot:29 | ClsMes::Init | assert active | n/a |  | ps2/include/font.hpp RECT |
| ps2/include/font.hpp:43 | RGBAQ_TYPE | faithful | 0x8 | 0x8 |  | boot:39 | CFont::Init | assert active | n/a |  | ps2/include/font.hpp RGBAQ_TYPE |
| ps2/include/font.hpp:129 | GAIJI_DATA | faithful | 0xE | 0xE |  | boot:54 | GetGaijiW | assert active | n/a |  | ps2/include/font.hpp GAIJI_DATA |
| ps2/include/font.hpp:142 | FCONV_CODE | runtime record | 0xC | 0x10 | str:pointer | boot:54 | GetFontGaijiFontNo | assert neutralised | n/a |  | ps2/include/font.hpp FCONV_CODE |
| ps2/include/font.hpp:158 | FONT_TBL_BIN | faithful | 0x1000 | 0x1000 |  | boot:54 | GetHalfFontNum | assert active | n/a |  | ps2/include/font.hpp FONT_TBL_BIN |
| ps2/include/font.hpp:371 | CFont | faithful | 0xB8 | 0xB8 |  | boot:39 | CFont::Init | assert active | n/a |  | ps2/include/font.hpp CFont |
| ps2/include/funcpoint.hpp:166 | CObjAnime | runtime record | 0x30 | 0x40 | frame:pointer, func_point:pointer, parts:pointer, piece:pointer | boot:49 | CMap::AssignFuncPoint | assert neutralised | n/a |  | ps2/include/funcpoint.hpp CObjAnime |
| ps2/include/funcpoint.hpp:179 | CList<CObjAnime> | runtime record | 0x50 | 0x60 | data:pointer, next:pointer, prev:pointer | mode | CList::Initialize | assert neutralised | n/a |  | ps2/include/funcpoint.hpp CList<CObjAnime> |
| ps2/include/funcpoint.hpp:393 | CFuncPointMngr | runtime record | 0x34 | 0x68 | list:pointer, now:pointer, vptr | boot:49 | CFuncPointMngr::Get | assert neutralised | n/a |  | ps2/include/funcpoint.hpp CFuncPointMngr |
| ps2/include/gamedata.hpp:317 | CDataCommon | runtime record | 0x2C | 0x30 | name:pointer | boot:8 | CGameData::GetAttachData | assert neutralised | n/a |  | ps2/include/gamedata.hpp CDataCommon |
| ps2/include/gamedata.hpp:342 | CDataItem | faithful | 0x10 | 0x10 |  | boot:8 | CGameData::GetItemData | assert active | n/a |  | ps2/include/gamedata.hpp CDataItem |
| ps2/include/gamedata.hpp:366 | CDataAttach | faithful | 0x18 | 0x18 |  | boot:8 | CGameData::GetAttachData | assert active | n/a |  | ps2/include/gamedata.hpp CDataAttach |
| ps2/include/gamedata.hpp:404 | CDataWeapon | faithful | 0x4C | 0x4C |  | boot:8 | CGameData::GetWeaponData | assert active | n/a |  | ps2/include/gamedata.hpp CDataWeapon |
| ps2/include/gamedata.hpp:437 | CDataRoboPart | faithful | 0x24 | 0x24 |  | boot:8 | CGameData::GetRoboData | assert active | n/a |  | ps2/include/gamedata.hpp CDataRoboPart |
| ps2/include/gamedata.hpp:467 | CDataBreedFish | faithful | 0x14 | 0x14 |  | boot:8 | CGameData::GetFishData | assert active | n/a |  | ps2/include/gamedata.hpp CDataBreedFish |
| ps2/include/gamedata.hpp:543 | COMMON_GAGE | faithful | 0x8 | 0x8 |  | boot:8 | CGameDataUsed::CopyDataRoboPart | assert active | n/a |  | ps2/include/gamedata.hpp COMMON_GAGE |
| ps2/include/gamedata.hpp:1324 | CGameDataUsed | faithful | 0x6C | 0x6C |  | boot:8 | CBattleCharaInfo::RefreshParamater | assert active | n/a |  | ps2/include/gamedata.hpp CGameDataUsed |
| ps2/include/gamedata.hpp:1358 | CItemUseTarget | runtime record | 0x8 | 0x10 | target:pointer | mode | CBattleCharaInfo::UseActiveItem | assert neutralised | n/a |  | ps2/include/gamedata.hpp CItemUseTarget |
| ps2/include/gamedata.hpp:1405 | BASE_MONSTER_TBL | faithful | 0xB8 | 0xB8 |  | boot:30 | CBattleCharaInfo::SetChrNo | assert active | n/a |  | ps2/include/gamedata.hpp BASE_MONSTER_TBL |
| ps2/include/gamedata.hpp:1562 | CGameData | runtime record | 0x30 | 0x50 | attach_data:pointer, common_data:pointer, fish_data:pointer, guard_data:pointer +3 | boot:8 | CGameData::GetAttachData | assert neutralised | n/a |  | ps2/include/gamedata.hpp CGameData |
| ps2/include/gamepad.hpp:144 | PAD_STATUS | faithful | 0x4C | 0x4C |  | boot:9 | CGamePad::Init | assert active | n/a |  | ps2/include/gamepad.hpp PAD_STATUS |
| ps2/include/gamepad.hpp:160 | PAD_REPEAT | faithful | 0x188 | 0x188 |  | boot:9 | CGamePad::Init | assert active | n/a |  | ps2/include/gamepad.hpp PAD_REPEAT |
| ps2/include/gamepad.hpp:175 | PAD_CAPTURE_FRAME | faithful | 0x6 | 0x6 |  | boot:18 | CGamePad::Capture | assert active | n/a |  | ps2/include/gamepad.hpp PAD_CAPTURE_FRAME |
| ps2/include/gamepad.hpp:675 | CGamePad | faithful | 0x478 | 0x478 |  | boot:9 | CGamePad::Init | assert active | n/a |  | ps2/include/gamepad.hpp CGamePad |
| ps2/include/gameutil.hpp:72 | FRAME_VECTOR_EX_DATA | faithful | 0x20 | 0x20 |  | mode | ChangeWeight | assert active | n/a |  | ps2/include/gameutil.hpp FRAME_VECTOR_EX_DATA |
| ps2/include/gameutil.hpp:89 | Mot_File_List | faithful | 0x20 | 0x20 |  | mode | ChangeWeight | assert active | n/a |  | ps2/include/gameutil.hpp Mot_File_List |
| ps2/include/gameutil.hpp:107 | Mot_List | runtime record | 0x20 | 0x30 | key_frames:pointer, next:pointer, values:pointer | mode | AnimeDataInit | assert neutralised | n/a |  | ps2/include/gameutil.hpp Mot_List |
| ps2/include/gameutil.hpp:119 | FrameLinkRecord | faithful | 0x30 | 0x30 |  | mode | AnimeDataInit | assert active | n/a |  | ps2/include/gameutil.hpp FrameLinkRecord |
| ps2/include/gameutil.hpp:136 | tagFRAME_INF | runtime record | 0x20 | 0x30 | base_normals:pointer, base_vertices:pointer, vertex_refs:pointer | mode | AnimeDataInit | assert neutralised | n/a |  | ps2/include/gameutil.hpp tagFRAME_INF |
| ps2/include/gameutil.hpp:151 | tagMOTION_TYPE | runtime record | 0x14 | 0x28 | base_matrices:pointer, frame_info:pointer, motion_list:pointer, skin_list:pointer | mode | AnimeDataInit | assert neutralised | n/a |  | ps2/include/gameutil.hpp tagMOTION_TYPE |
| ps2/include/gameutil.hpp:164 | MOTION_FILE_INFO | runtime record | 0xC | 0x18 | data:pointer, name:pointer | mode | CreateAnimeDataEX | assert neutralised | n/a |  | ps2/include/gameutil.hpp MOTION_FILE_INFO |
| ps2/include/gameutil.hpp:178 | CollisionInfo | runtime record | 0x10 | 0x18 | polys:pointer | mode | CheckHit | assert neutralised | n/a |  | ps2/include/gameutil.hpp CollisionInfo |
| ps2/include/gameutil.hpp:222 | MoveCheckInfo | faithful | 0x110 | 0x110 |  | mode | CActionChara::HumanMoveIF | assert active | n/a |  | ps2/include/gameutil.hpp MoveCheckInfo |
| ps2/include/gyorace.hpp:80 | GYORACE_FISH_INF | faithful | 0x2C | 0x2C |  | mode | AutoCam | assert active | n/a |  | ps2/include/gyorace.hpp GYORACE_FISH_INF |
| ps2/include/gyorace.hpp:94 | GYORACE_RESULT | faithful | 0x24 | 0x24 |  | mode | _SET_GYORACE_ETC | assert active | n/a |  | ps2/include/gyorace.hpp GYORACE_RESULT |
| ps2/include/gyoracesim.hpp:64 | grFISH_PARAM | faithful | 0x40 | 0x40 |  | mode | CharacterBonus | assert active | n/a |  | ps2/include/gyoracesim.hpp grFISH_PARAM |
| ps2/include/gyoracesim.hpp:82 | grRACE_PROGRESS | faithful | 0x18 | 0x18 |  | mode | Jikkyou | assert active | n/a |  | ps2/include/gyoracesim.hpp grRACE_PROGRESS |
| ps2/include/gyoracesim.hpp:101 | grRACE_INFO | runtime record | 0x1DC | 0x1F8 | progress:pointer | mode | Jikkyou | assert neutralised | n/a |  | ps2/include/gyoracesim.hpp grRACE_INFO |
| ps2/include/gyoracesim.hpp:131 | RACE_FISH_PARAM | runtime record | 0xA0 | 0xA8 | progress:pointer | mode | CharacterBonus | assert neutralised | n/a |  | ps2/include/gyoracesim.hpp RACE_FISH_PARAM |
| ps2/include/gyoracesim.hpp:146 | grFISH_DATA | faithful | 0x1C | 0x1C |  | mode | FishModifyParam | assert active | n/a |  | ps2/include/gyoracesim.hpp grFISH_DATA |
| ps2/include/helpmes.hpp:42 | HELP_MES_INFO | faithful | 0x1C | 0x1C |  | mode | CreateHelpMes | assert active | n/a |  | ps2/include/helpmes.hpp HELP_MES_INFO |
| ps2/include/inventmn.hpp:415 | SCOOP_DATA | runtime record | 0x14 | 0x18 | text:pointer | menu | CMenuQuestView::KeyStep | assert neutralised | n/a |  | ps2/include/inventmn.hpp SCOOP_DATA |
| ps2/include/inventmn.hpp:428 | PIC_NAME_INFO | runtime record | 0x8 | 0x10 | name:pointer | menu | CMenuInvent::UpdataNetaMemoStr | assert neutralised | n/a |  | ps2/include/inventmn.hpp PIC_NAME_INFO |
| ps2/include/inventmn.hpp:441 | INVENT_MATERIAL | faithful | 0x4 | 0x4 |  | menu | CInventDataManage::DeleteUserUsedItem | assert active | n/a |  | ps2/include/inventmn.hpp INVENT_MATERIAL |
| ps2/include/inventmn.hpp:454 | INVENT_MATERIAL_LIST | runtime record | 0x8 | 0x10 | material:pointer | menu | CInventDataManage::DeleteUserUsedItem | assert neutralised | n/a |  | ps2/include/inventmn.hpp INVENT_MATERIAL_LIST |
| ps2/include/inventmn.hpp:476 | MakeItemNeeds | faithful | 0x24 | 0x24 |  | menu | CInventDataManage::CheckMakeItem | assert active | n/a |  | ps2/include/inventmn.hpp MakeItemNeeds |
| ps2/include/inventmn.hpp:495 | INVENT_DATA_INFO | runtime record | 0x24 | 0x30 | materials:pointer | menu | CInventDataManage::CheckInventEnable | assert neutralised | n/a |  | ps2/include/inventmn.hpp INVENT_DATA_INFO |
| ps2/include/inventmn.hpp:556 | CDC2AlbumData | save record | 0x64CB0 | 0x64E40 | photo:pointer | menu | CDC2AlbumData::DeletePhotoData | host record port/src/records/save_image.hpp | repaired | photo is a 32-bit PS2 address on disk | ps2/include/inventmn.hpp CDC2AlbumData |
| ps2/include/inventmn.hpp:639 | CInventDataManage | runtime record | 0x8 | 0x10 | table:pointer | menu | CInventDataManage::CheckInventEnable | assert neutralised | n/a |  | ps2/include/inventmn.hpp CInventDataManage |
| ps2/include/inventmn.hpp:1154 | CMenuInvent | runtime record | 0xF30 | 0x1190 | album_big_form:pointer, album_sw_form:pointer, album_tex:pointer, ask_para:pointer +34 | menu | CMenuInvent::AttachFormInfo | assert neutralised | n/a |  | ps2/include/inventmn.hpp CMenuInvent |
| ps2/include/mainloop.hpp:223 | INIT_LOOP_ARG | faithful | 0x50 | 0x50 |  | boot:7 | MainLoop | assert active | n/a |  | ps2/include/mainloop.hpp INIT_LOOP_ARG |
| ps2/include/mainloop.hpp:240 | DEBUG_INFO | faithful | 0x14 | 0x14 |  | boot:7 | MainLoop | assert active | n/a |  | ps2/include/mainloop.hpp DEBUG_INFO |
| ps2/include/mainloop.hpp:253 | PAD_TABLE_ENTRY | faithful | 0xC | 0xC |  | boot:28 | InitPadTable | assert active | n/a |  | ps2/include/mainloop.hpp PAD_TABLE_ENTRY |
| ps2/include/mainloop.hpp:265 | ANALOG_TABLE_ENTRY | faithful | 0x8 | 0x8 |  | boot:28 | InitPadTable | assert active | n/a |  | ps2/include/mainloop.hpp ANALOG_TABLE_ENTRY |
| ps2/include/map.hpp:84 | CMapFlagData | faithful | 0x10 | 0x10 |  | mode | CMap::DeleteTrBox | assert active | n/a |  | ps2/include/map.hpp CMapFlagData |
| ps2/include/map.hpp:102 | PartsGroupData | runtime record | 0x4 | 0x8 | parts:pointer | mode | CList::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp PartsGroupData |
| ps2/include/map.hpp:103 | CList<PartsGroupData> | runtime record | 0x10 | 0x20 | data:pointer, next:pointer, prev:pointer | mode | CList::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp CList<PartsGroupData> |
| ps2/include/map.hpp:104 | CList<CMapParts *> | runtime record | 0x10 | 0x20 | data:pointer, next:pointer, prev:pointer | mode | CList::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp CList<CMapParts *> |
| ps2/include/map.hpp:141 | CPartsGroup | runtime record | 0x10 | 0x18 | list:pointer, name:pointer | boot:49 | CPartsGroup::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp CPartsGroup |
| ps2/include/map.hpp:161 | MapDrawOffRect | runtime record | 0x30 | 0x30 | parts:pointer | boot:49 | CMap::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp MapDrawOffRect |
| ps2/include/map.hpp:187 | MapEventInfo | faithful | 0x60 | 0x60 |  | mode | CMap::GetEvent | assert active | n/a |  | ps2/include/map.hpp MapEventInfo |
| ps2/include/map.hpp:234 | CMapWater | runtime record | 0xA0 | 0xA0 | frame:pointer, parts:pointer, parts_name:pointer, vptr | boot:49 | CMap::PlacePartsEnd | assert neutralised | n/a |  | ps2/include/map.hpp CMapWater |
| ps2/include/map.hpp:1073 | CMap | runtime record | 0xD10 | 0xF40 | camera_info:pointer, draw_parts:pointer, draw_rect:pointer, effect_list:pointer +14 | boot:49 | CEditMap::Initialize | assert neutralised | n/a |  | ps2/include/map.hpp CMap |
| ps2/include/mapinfo.hpp:81 | CCameraInfo | runtime record | 0xD0 | 0xE0 | rect:pointer | mode | CCameraInfo::CCameraInfo | assert neutralised | n/a |  | ps2/include/mapinfo.hpp CCameraInfo |
| ps2/include/mapinfo.hpp:233 | CMapInfo | runtime record | 0x100 | 0x1A0 | add_map_file:pointer, img_name:pointer, lighting_info:pointer, map_file:pointer +1 | boot:49 | CMap::LoadData | assert neutralised | n/a |  | ps2/include/mapinfo.hpp CMapInfo |
| ps2/include/mapjump.hpp:41 | MapJumpMapInfo | runtime record | 0x18 | 0x20 | load_buf:pointer | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/mapjump.hpp MapJumpMapInfo |
| ps2/include/mapload.hpp:123 | CMapLightingInfo | faithful | 0x1D0 | 0x1D0 |  | boot:49 | CMapInfo::LoadMapInfo | assert active | n/a |  | ps2/include/mapload.hpp CMapLightingInfo |
| ps2/include/mapload.hpp:143 | CFuncPointCheck | faithful | 0x8 | 0x8 |  | boot:49 | CMapParts::Initialize | assert active | n/a |  | ps2/include/mapload.hpp CFuncPointCheck |
| ps2/include/mapload.hpp:340 | CFuncPoint | runtime record | 0x1C0 | 0x1F0 | CFuncPoint::(anonymous union at /work/chronicletwo/ps2/include/mapload.hpp:262:5):pointer, frame:pointer, frame:vptr, name:pointer | boot:49 | CFuncPointMngr::Get | assert neutralised | n/a |  | ps2/include/mapload.hpp CFuncPoint |
| ps2/include/mapload.hpp:376 | PieceMaterial | runtime record | 0x20 | 0x30 | frame:pointer, material:pointer | mode | CMapParts::GetDefColor | assert neutralised | n/a |  | ps2/include/mapload.hpp PieceMaterial |
| ps2/include/mapload.hpp:409 | CCameraDrawInfo | faithful | 0x8 | 0x8 |  | mode | CCameraDrawInfo::Initialize | assert active | n/a |  | ps2/include/mapload.hpp CCameraDrawInfo |
| ps2/include/mapparts.hpp:48 | InScreenFuncInfo | faithful | 0xC | 0xC |  | mode | CMap::InScreenFunc | assert active | n/a |  | ps2/include/mapparts.hpp InScreenFuncInfo |
| ps2/include/mapparts.hpp:509 | CMapParts | runtime record | 0x310 | 0x350 | anime_list:pointer, frame:pointer, frame:vptr, func_point_mngr:pointer +3 | boot:49 | CMap::AssignFuncPoint | assert neutralised | n/a |  | ps2/include/mapparts.hpp CMapParts |
| ps2/include/mapparts.hpp:570 | CMapTreasureBox | runtime record | 0x680 | 0x9E0 | deform_frame:pointer, dynamic_anime:pointer, effect_image_list:pointer, effect_list:pointer +27 | mode | CMap::CreateTrBox | assert neutralised | n/a |  | ps2/include/mapparts.hpp CMapTreasureBox |
| ps2/include/mapselect.hpp:143 | MAP_NAME_INFO | runtime record | 0x1C | 0x28 | add_path:pointer, name:pointer, title:pointer | mode | GetAddMapPath | assert neutralised | n/a |  | ps2/include/mapselect.hpp MAP_NAME_INFO |
| ps2/include/mapselect.hpp:160 | EVENT_VIEW_INFO | runtime record | 0x1C | 0x28 | detail:pointer, name:pointer | mode | EventViewLoop | assert neutralised | n/a |  | ps2/include/mapselect.hpp EVENT_VIEW_INFO |
| ps2/include/mapsky.hpp:39 | MAP_SKY_INFO | faithful | 0x740 | 0x740 |  | boot:49 | CMapSky::LoadPack | assert active | n/a |  | ps2/include/mapsky.hpp MAP_SKY_INFO |
| ps2/include/mapsky.hpp:123 | CMapSky::AnimeFrame | runtime record | 0x8 | 0x10 | frame:pointer | boot:49 | CMapSky::Initialize | assert neutralised | n/a |  | ps2/include/mapsky.hpp CMapSky::AnimeFrame |
| ps2/include/mapsky.hpp:124 | CMapSky | runtime record | 0x108 | 0x1C0 | anime:pointer, bg:pointer, bg_visual:pointer, sky:pointer +2 | boot:49 | CMapSky::Initialize | assert neutralised | n/a |  | ps2/include/mapsky.hpp CMapSky |
| ps2/include/mdslist.hpp:82 | CMdsInfo | runtime record | 0x20 | 0x38 | chara:pointer, frame:pointer, name:pointer, vptr | boot:49 | CMdsInfo::CMdsInfo | assert neutralised | n/a |  | ps2/include/mdslist.hpp CMdsInfo |
| ps2/include/mdslist.hpp:137 | CMdsList | runtime record | 0x10 | 0x20 | list:pointer, name:pointer | boot:0 | CMdsListSet::Initialize | assert neutralised | n/a |  | ps2/include/mdslist.hpp CMdsList |
| ps2/include/mdslist.hpp:160 | CIMGList | runtime record | 0x8 | 0x10 | info:pointer, name:pointer | boot:0 | CMdsListSet::Initialize | assert neutralised | n/a |  | ps2/include/mdslist.hpp CIMGList |
| ps2/include/mdslist.hpp:445 | CMapPiece | runtime record | 0xB0 | 0xB0 | chara:pointer, frame:pointer, material:pointer, name:pointer +1 | boot:49 | CMapParts::SearchPiece | assert neutralised | n/a |  | ps2/include/mdslist.hpp CMapPiece |
| ps2/include/memcard.hpp:166 | MC_ICON_DATA | runtime record | 0x28 | 0x30 | data:pointer | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/memcard.hpp MC_ICON_DATA |
| ps2/include/memcard.hpp:184 | MC_CARD_INFO | faithful | 0x20 | 0x20 |  | boot:48 | CMemoryCardManager::Initialize | assert active | n/a |  | ps2/include/memcard.hpp MC_CARD_INFO |
| ps2/include/memcard.hpp:199 | MC_ERROR_INFO | faithful | 0x14 | 0x14 |  | boot:56 | CMemoryCardManager::CheckAlbum | assert active | n/a |  | ps2/include/memcard.hpp MC_ERROR_INFO |
| ps2/include/memcard.hpp:225 | SAVEDATA_INFO | faithful | 0x40 | 0x40 |  | boot:48 | CMemoryCardManager::InitPlayDataInfo | assert active | n/a |  | ps2/include/memcard.hpp SAVEDATA_INFO |
| ps2/include/memcard.hpp:258 | SAVEDATA_FORMAT | save record | 0x659C0 | 0x65AB0 | save_data:long, save_data:pointer | boot:48 | CMemoryCardManager::Initialize | host record port/src/records/save_image.hpp | repaired | the memory-card image | ps2/include/memcard.hpp SAVEDATA_FORMAT |
| ps2/include/memcard.hpp:271 | COSBIT_INFO | faithful | 0x4 | 0x4 |  | boot:30 | CUserDataManager::GetCostume | assert active | n/a |  | ps2/include/memcard.hpp COSBIT_INFO |
| ps2/include/memcard.hpp:758 | CMemoryCardManager | runtime record | 0x1100 | 0x1130 | album_buffer:pointer, icon:pointer, read_buffer:pointer, save_buffer:pointer +2 | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/memcard.hpp CMemoryCardManager |
| ps2/include/menuaqua.hpp:295 | AQUA_BUBBLE | faithful | 0x30 | 0x30 |  | menu | CBubble::Draw | assert active | n/a |  | ps2/include/menuaqua.hpp AQUA_BUBBLE |
| ps2/include/menuaqua.hpp:389 | CBubble | runtime record | 0x40 | 0x50 | bubble:pointer, texture:pointer | menu | CAquarium::Initialize | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CBubble |
| ps2/include/menuaqua.hpp:433 | CAquaFishActionParam | runtime record | 0x40 | 0x48 | target:pointer | menu | CAquaFish::MoveActionBattle | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CAquaFishActionParam |
| ps2/include/menuaqua.hpp:447 | AQUA_FISH_ROUND | faithful | 0x10 | 0x10 |  | menu | CAquaFish::MoveActionRound | assert active | n/a |  | ps2/include/menuaqua.hpp AQUA_FISH_ROUND |
| ps2/include/menuaqua.hpp:502 | CAquaFishEff | runtime record | 0x10 | 0x18 | fish:pointer, texture:pointer | menu | CAquaFish::MoveActionBattle | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CAquaFishEff |
| ps2/include/menuaqua.hpp:516 | NEXT_THINK_PARAM | runtime record | 0x20 | 0x30 | effect:pointer, target:pointer | menu | CAquaFish::NextThink | assert neutralised | n/a |  | ps2/include/menuaqua.hpp NEXT_THINK_PARAM |
| ps2/include/menuaqua.hpp:729 | CAquaFish | runtime record | 0x940 | 0xCB0 | action:pointer, data:pointer | menu | CAquaFish::AddFatigue | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CAquaFish |
| ps2/include/menuaqua.hpp:788 | CFishFood | runtime record | 0x6A0 | 0xA00 | deform_frame:pointer, dynamic_anime:pointer, effect_image_list:pointer, effect_list:pointer +25 | menu | CAquarium::ColCheck | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CFishFood |
| ps2/include/menuaqua.hpp:976 | CAquaMes | runtime record | 0x64 | 0x90 | fish_mes:pointer, guide_mes:pointer, help_mes:pointer, info_mes:pointer +4 | menu | CAquaMes::AddMenuCursor | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CAquaMes |
| ps2/include/menuaqua.hpp:1172 | CAquarium | runtime record | 0x3D0 | 0x550 | aqua_frame:pointer, aqua_stack:pointer, fish:pointer, fish_stack:pointer +15 | boot:0 | CAquarium::CAquarium | assert neutralised | n/a |  | ps2/include/menuaqua.hpp CAquarium |
| ps2/include/menuaqua.hpp:1216 | FISH_PRIZE_INFO | faithful | 0x8 | 0x8 |  | mode | _GET_FISHINGTOURNAMENT_ETC | assert active | n/a |  | ps2/include/menuaqua.hpp FISH_PRIZE_INFO |
| ps2/include/menucapt.hpp:40 | MENU_CHAPTER_INFO | faithful | 0x20 | 0x20 |  | menu | MenuChapterDraw | assert active | n/a |  | ps2/include/menucapt.hpp MENU_CHAPTER_INFO |
| ps2/include/menuchr.hpp:418 | CHR_CNG_STAR | faithful | 0x18 | 0x18 |  | menu | CMenuChrCngMenu::CalcTex | assert active | n/a |  | ps2/include/menuchr.hpp CHR_CNG_STAR |
| ps2/include/menuchr.hpp:432 | CHR_CNG_CLUT_COLOR | faithful | 0x4 | 0x4 |  | menu | CMenuChrCngMenu::EnterDataMenu | assert active | n/a |  | ps2/include/menuchr.hpp CHR_CNG_CLUT_COLOR |
| ps2/include/menuchr.hpp:647 | CMenuChrCngMenu | runtime record | 0x1F80 | 0x2040 | ask_para:pointer, chara_pos:pointer, cmd_part:pointer, face_img:pointer +17 | menu | CMenuChrCngMenu::AttachForm | assert neutralised | n/a |  | ps2/include/menuchr.hpp CMenuChrCngMenu |
| ps2/include/menuchr.hpp:766 | CMenuMosSelect | runtime record | 0x7670 | 0x83F0 | ask_para:pointer, badge:pointer, badge_form:pointer, effect:pointer +15 | menu | CMenuMosSelect::AttachForm | assert neutralised | n/a |  | ps2/include/menuchr.hpp CMenuMosSelect |
| ps2/include/menuchr.hpp:855 | CMenuCostumeSel | runtime record | 0x2D0 | 0x330 | ask_para:pointer, camera:vptr, chara_data:pointer, cursor_tex:pointer +5 | menu | CMenuCostumeSel::CMenuCostumeSel | assert neutralised | n/a |  | ps2/include/menuchr.hpp CMenuCostumeSel |
| ps2/include/menuchr.hpp:974 | CMosBookMenu | runtime record | 0x980 | 0x9D0 | ask_para:pointer, camera:vptr, monster:pointer, monster_info:pointer +3 | menu | CMosBookMenu::CMosBookMenu | assert neutralised | n/a |  | ps2/include/menuchr.hpp CMosBookMenu |
| ps2/include/menuchr.hpp:1690 | MENU_LOAD_INFO | faithful | 0x8 | 0x8 |  | menu | CMenuChrCngMenu::CheckChrChange | assert active | n/a |  | ps2/include/menuchr.hpp MENU_LOAD_INFO |
| ps2/include/menucls1.hpp:59 | CMenuFont | faithful | 0xB8 | 0xB8 |  | mode | TitleHDDInstallDraw | assert active | n/a |  | ps2/include/menucls1.hpp CMenuFont |
| ps2/include/menucls1.hpp:397 | CDC2Mes | runtime record | 0x2A50 | 0x2A70 | buff:pointer, buff_system:pointer, mes_data:pointer, text_ptr:pointer | mode | AdjustWaku | assert neutralised | n/a |  | ps2/include/menucls1.hpp CDC2Mes |
| ps2/include/menucls1.hpp:413 | MENU_ITEM_MOVE_INFO | runtime record | 0x7C | 0x88 | dest:pointer | menu | CMenuMoveItem::CheckMove | assert neutralised | n/a |  | ps2/include/menucls1.hpp MENU_ITEM_MOVE_INFO |
| ps2/include/menucls1.hpp:476 | CMenuMoveItem | runtime record | 0x104 | 0x128 | form:pointer, info:pointer | menu | CMenuItemInfo::KeyStepLocal | assert neutralised | n/a |  | ps2/include/menucls1.hpp CMenuMoveItem |
| ps2/include/menucls1.hpp:531 | CMenuItemUse | faithful | 0x1C | 0x1C |  | menu | CMenuItemUse::CheckItemUseEnable | assert active | n/a |  | ps2/include/menucls1.hpp CMenuItemUse |
| ps2/include/menucommon.hpp:73 | MENU_SPI_ANALYZE_STRUCT1 | runtime record | 0x8 | 0x10 | name:pointer | menu | _MENU_EXE_MSGENV | assert neutralised | n/a |  | ps2/include/menucommon.hpp MENU_SPI_ANALYZE_STRUCT1 |
| ps2/include/menucommon.hpp:96 | MENU_COMMAND_ANALYZE_INFO | runtime record | 0x68 | 0x88 | mes_buff:pointer, system_mes_buff:pointer | menu | CMenuChrCngMenu::EnterDataMenu | assert neutralised | n/a |  | ps2/include/menucommon.hpp MENU_COMMAND_ANALYZE_INFO |
| ps2/include/menudraw.hpp:312 | MENU_SHORT_RECT | faithful | 0x8 | 0x8 |  | mode | TitleModeDraw | assert active | n/a |  | ps2/include/menudraw.hpp MENU_SHORT_RECT |
| ps2/include/menudraw.hpp:330 | MENU_BASETEXINFO | runtime record | 0x20 | 0x30 | name:pointer, tex_name:pointer | menu | CMenuPosDataForm::GetNowPosRGBA | assert neutralised | n/a |  | ps2/include/menudraw.hpp MENU_BASETEXINFO |
| ps2/include/menudraw.hpp:345 | MENU_PARTS_EFFECT_STRUCT1 | faithful | 0x24 | 0x24 |  | menu | CMenuInvent::GradationSet | assert active | n/a |  | ps2/include/menudraw.hpp MENU_PARTS_EFFECT_STRUCT1 |
| ps2/include/menudraw.hpp:381 | MENUFORMPARTS_TYPE | runtime record | 0x48 | 0x58 | effect:pointer, name:pointer, tex:pointer | menu | CBaseMenuClass::EffectDrawCheck | assert neutralised | n/a |  | ps2/include/menudraw.hpp MENUFORMPARTS_TYPE |
| ps2/include/menudraw.hpp:396 | MENU_FORM_ACTION_MOVE | faithful | 0x14 | 0x14 |  | menu | CMenuPosDataForm::GetNextMovePos | assert active | n/a |  | ps2/include/menudraw.hpp MENU_FORM_ACTION_MOVE |
| ps2/include/menudraw.hpp:408 | MENU_FORM_ACTION | runtime record | 0x14 | 0x18 | move:pointer | menu | CMenuPosDataForm::GetNextMovePos | assert neutralised | n/a |  | ps2/include/menudraw.hpp MENU_FORM_ACTION |
| ps2/include/menudraw.hpp:420 | MENU_ETCINFO | runtime record | 0xC | 0x10 | name:pointer | menu | CMenuChrCngMenu::CalcTex | assert neutralised | n/a |  | ps2/include/menudraw.hpp MENU_ETCINFO |
| ps2/include/menudraw.hpp:432 | MENU_ETCINFO2 | runtime record | 0x14 | 0x18 | name:pointer | menu | CPosDataManage::EtcTbl2Clear | assert neutralised | n/a |  | ps2/include/menudraw.hpp MENU_ETCINFO2 |
| ps2/include/menudraw.hpp:446 | MENUFORM_MAKEBRD_LINE | faithful | 0x6 | 0x6 |  | menu | CMenuInvent::CalcMakeBrd | assert active | n/a |  | ps2/include/menudraw.hpp MENUFORM_MAKEBRD_LINE |
| ps2/include/menudraw.hpp:462 | MENUFORM_MAKEBRD_INFO | faithful | 0x2C | 0x2C |  | menu | CMenuInvent::CalcMakeBrd | assert active | n/a |  | ps2/include/menudraw.hpp MENUFORM_MAKEBRD_INFO |
| ps2/include/menudraw.hpp:576 | MENU_EFFECT_INFO | faithful | 0x40 | 0x40 |  | menu | CBaseMenuClass::IsSpectolTrans | assert active | n/a |  | ps2/include/menudraw.hpp MENU_EFFECT_INFO |
| ps2/include/menudraw.hpp:596 | REPAIR_EFFECT_PARTICLE | faithful | 0x30 | 0x30 |  | menu | CRepairEffect::Draw | assert active | n/a |  | ps2/include/menudraw.hpp REPAIR_EFFECT_PARTICLE |
| ps2/include/menudraw.hpp:895 | CMenuPosDataForm | runtime record | 0x80 | 0xA0 | action:pointer, chara:pointer, name:pointer, next:pointer +2 | menu | CBaseMenuClass::CheckSpectolFusion | assert neutralised | n/a |  | ps2/include/menudraw.hpp CMenuPosDataForm |
| ps2/include/menudraw.hpp:1157 | CPosDataManage | runtime record | 0x20 | 0x40 | etc_tbl2:pointer, etc_tbl:pointer, form:pointer, tex_info:pointer | menu | CPosDataManage::ClearPos | assert neutralised | n/a |  | ps2/include/menudraw.hpp CPosDataManage |
| ps2/include/menudraw.hpp:1262 | CMenuPosDataManage | runtime record | 0x5BC | 0x630 | common_tex:pointer, effect_tex:pointer, etc_tbl2:pointer, etc_tbl:pointer +6 | menu | CItemSelect::Draw | assert neutralised | n/a |  | ps2/include/menudraw.hpp CMenuPosDataManage |
| ps2/include/menudraw.hpp:1322 | CRepairEffect | runtime record | 0x24 | 0x30 | particle:pointer, sub_tex:pointer, tex:pointer | menu | CRepairEffect::Draw | assert neutralised | n/a |  | ps2/include/menudraw.hpp CRepairEffect |
| ps2/include/menudraw.hpp:1465 | CRepairManager | runtime record | 0x1EC | 0x2F8 | data:pointer, effect:pointer, effect_stack:pointer, model:pointer +3 | menu | CMenuItemInfo::MenuModeMalloc | assert neutralised | n/a |  | ps2/include/menudraw.hpp CRepairManager |
| ps2/include/menudraw.hpp:1542 | CLevelUpEffect | runtime record | 0x30 | 0x30 | chara:pointer, tex:pointer | menu | CLevelUpEffect::Draw | assert neutralised | n/a |  | ps2/include/menudraw.hpp CLevelUpEffect |
| ps2/include/menudraw.hpp:1616 | CLevelUpEffectManager | runtime record | 0x190 | 0x190 | effect:pointer, label_tex:pointer, spark_tex:pointer | menu | CLevelUpEffectManager::Draw | assert neutralised | n/a |  | ps2/include/menudraw.hpp CLevelUpEffectManager |
| ps2/include/menudraw.hpp:1671 | CStarDust | faithful | 0xC | 0xC |  | menu | CMenuInvent::PhotoNetaEnter | assert active | n/a |  | ps2/include/menudraw.hpp CStarDust |
| ps2/include/menudraw.hpp:1723 | CEffVerticalLine | faithful | 0x40 | 0x40 |  | menu | CEffVerticalLine::Draw | assert active | n/a |  | ps2/include/menudraw.hpp CEffVerticalLine |
| ps2/include/menudraw.hpp:1846 | CMenuEffect | runtime record | 0x38 | 0x48 | info:pointer, tex:pointer | menu | CBaseMenuClass::EffectDrawCheck | assert neutralised | n/a |  | ps2/include/menudraw.hpp CMenuEffect |
| ps2/include/menumain.hpp:225 | MENU_INIT_ARG | runtime record | 0x98 | 0xB8 | base_chara_stack:pointer, chara_stack:pointer, pack:pointer, scene:pointer +2 | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/menumain.hpp MENU_INIT_ARG |
| ps2/include/menumain.hpp:252 | MENU_DRAW_ENV | runtime record | 0xD0 | 0xD0 | camera:vptr | menu | CMenuCostumeSel::LoadMenuData | assert neutralised | n/a |  | ps2/include/menumain.hpp MENU_DRAW_ENV |
| ps2/include/menumain.hpp:264 | MENU_ETC_INFO | runtime record | 0x8 | 0x10 | tex:pointer | menu | CMenuInter::InitEnd | assert neutralised | n/a |  | ps2/include/menumain.hpp MENU_ETC_INFO |
| ps2/include/menumain.hpp:324 | CMenuInter | runtime record | 0x18 | 0x20 | mode_list:pointer | menu | CMenuInter::InitEnd | assert neutralised | n/a |  | ps2/include/menumain.hpp CMenuInter |
| ps2/include/menumap.hpp:130 | WMAP_POS_DATA | runtime record | 0x14 | 0x18 | name:pointer | menu | CWorldMapMenu::KeyStep | assert neutralised | n/a |  | ps2/include/menumap.hpp WMAP_POS_DATA |
| ps2/include/menumap.hpp:153 | WMAP_AREA_DATA | runtime record | 0x4C | 0x78 | name:pointer, pos:pointer | menu | CWorldMapMenu::Draw | assert neutralised | n/a |  | ps2/include/menumap.hpp WMAP_AREA_DATA |
| ps2/include/menumap.hpp:241 | CWorldMapMenu | runtime record | 0xBA4 | 0xCF8 | anim_tex:pointer, ask_para:pointer, capture_tex:pointer, cursor_tex:pointer +10 | menu | CWorldMapMenu::CWorldMapMenu | assert neutralised | n/a |  | ps2/include/menumap.hpp CWorldMapMenu |
| ps2/include/menuop.hpp:27 | SaveIconSet | runtime record | 0x78 | 0x90 | file:pointer | menu | SetMCIconData | assert neutralised | n/a |  | ps2/include/menuop.hpp SaveIconSet |
| ps2/include/menuop.hpp:310 | CManualMenu | runtime record | 0x178 | 0x1A8 | ask_para:pointer, movie_stack:pointer, script:pointer, vptr | menu | CManualMenu::CManualMenu | assert neutralised | n/a |  | ps2/include/menuop.hpp CManualMenu |
| ps2/include/menuop.hpp:405 | CMenuOption | runtime record | 0x384 | 0x4E0 | ask_para:pointer, button:pointer, script:pointer, value:pointer +1 | menu | CMenuOption::CMenuOption | assert neutralised | n/a |  | ps2/include/menuop.hpp CMenuOption |
| ps2/include/menuop.hpp:518 | CSaveMenuClass | runtime record | 0x1A4 | 0x1E8 | ask_para:pointer, cursor_form:pointer, dl_tex:pointer, list_form:pointer +6 | menu | CSaveMenuClass::CSaveMenuClass | assert neutralised | n/a |  | ps2/include/menuop.hpp CSaveMenuClass |
| ps2/include/menushop.hpp:107 | DONY_SHOP_ITEM | faithful | 0x4 | 0x4 |  | menu | GetDonyShopLineUp | assert active | n/a |  | ps2/include/menushop.hpp DONY_SHOP_ITEM |
| ps2/include/menushop.hpp:119 | SHOP_PRICE_INFO | faithful | 0x8 | 0x8 |  | menu | CShopMenu::KeyStep | assert active | n/a |  | ps2/include/menushop.hpp SHOP_PRICE_INFO |
| ps2/include/menushop.hpp:235 | CShop | faithful | 0x120C | 0x120C |  | menu | CShop::AddMoney | assert active | n/a |  | ps2/include/menushop.hpp CShop |
| ps2/include/menushop.hpp:413 | CShopMenu | runtime record | 0x210 | 0x258 | ask_para:pointer, exp_brd:pointer, item_brd:pointer, item_list:pointer +10 | menu | CShopMenu::AttachForm | assert neutralised | n/a |  | ps2/include/menushop.hpp CShopMenu |
| ps2/include/menushop.hpp:473 | CMenuQuestView | runtime record | 0x190 | 0x1A8 | ask_para:pointer, script:pointer, vptr | menu | CMenuQuestView::CMenuQuestView | assert neutralised | n/a |  | ps2/include/menushop.hpp CMenuQuestView |
| ps2/include/menusys.hpp:393 | MENU_SWAPITEM_INFO | faithful | 0x8 | 0x8 |  | menu | CBaseMenuClass::SetAskHowMuchItemNum | assert active | n/a |  | ps2/include/menusys.hpp MENU_SWAPITEM_INFO |
| ps2/include/menusys.hpp:469 | MENU_ASKMODE_PARA | runtime record | 0x94 | 0xA0 | form:pointer, item2:pointer, item:pointer | menu | CBaseMenuClass::CheckSpectolFusion | assert neutralised | n/a |  | ps2/include/menusys.hpp MENU_ASKMODE_PARA |
| ps2/include/menusys.hpp:488 | ITEMCMD_RET_PARA | runtime record | 0x14 | 0x20 | item2:pointer, item:pointer | menu | CBaseMenuClass::ItemCmdAfter | assert neutralised | n/a |  | ps2/include/menusys.hpp ITEMCMD_RET_PARA |
| ps2/include/menusys.hpp:531 | CMENU_USERPARAM | runtime record | 0x18 | 0x30 | chara:pointer, monster1:pointer, monster:pointer, robo:pointer +1 | menu | CBaseMenuClass::MenuItemCommandSelect | assert neutralised | n/a |  | ps2/include/menusys.hpp CMENU_USERPARAM |
| ps2/include/menusys.hpp:551 | MENU_INPUTKEY_ARG | faithful | 0x24 | 0x24 |  | menu | CMenuKeyFunc::CheckMoveSelect | assert active | n/a |  | ps2/include/menusys.hpp MENU_INPUTKEY_ARG |
| ps2/include/menusys.hpp:880 | CBaseMenuClass | runtime record | 0x110 | 0x128 | ask_para:pointer, script:pointer, vptr | mode | CNameRegiMenu::CNameRegiMenu | assert neutralised | n/a |  | ps2/include/menusys.hpp CBaseMenuClass |
| ps2/include/menusys.hpp:1318 | CMenuKeyFunc | runtime record | 0x160 | 0x190 | cursor_form:pointer, have_icon:pointer, have_num:pointer, have_shadow:pointer +5 | mode | CNameRegiMenu::KeyStep | assert neutralised | n/a |  | ps2/include/menusys.hpp CMenuKeyFunc |
| ps2/include/menusys.hpp:1723 | CMenuItemInfo | runtime record | 0x370 | 0x4B0 | ask_para:pointer, build_up_chara:pointer, chara_poly_form:pointer, fill_form:pointer +13 | menu | CBaseMenuClass::MenuItemCommandSelect | assert neutralised | n/a |  | ps2/include/menusys.hpp CMenuItemInfo |
| ps2/include/menusys.hpp:1817 | CItemSelect | runtime record | 0x450 | 0x6E0 | ask_para:pointer, item_list:pointer, script:pointer, texture:pointer +1 | menu | CItemSelect::CItemSelect | assert neutralised | n/a |  | ps2/include/menusys.hpp CItemSelect |
| ps2/include/menusys.hpp:1832 | MENU_ITEM_CURSOR_INFO | faithful | 0xC | 0xC |  | menu | CMenuItemInfo::KeyStep | assert active | n/a |  | ps2/include/menusys.hpp MENU_ITEM_CURSOR_INFO |
| ps2/include/menusys.hpp:1854 | BUILDUP_WEAPON_INFO | runtime record | 0x44 | 0x60 | weapon:pointer, weapon_data:pointer | menu | CMenuItemInfo::CalcCursorPosition | assert neutralised | n/a |  | ps2/include/menusys.hpp BUILDUP_WEAPON_INFO |
| ps2/include/menusystemdata.hpp:21 | MENU_SYSTEM_LIST_CURSOR | faithful | 0x4 | 0x4 |  | menu | CMenuInvent::ExitEnd | assert active | n/a |  | ps2/include/menusystemdata.hpp MENU_SYSTEM_LIST_CURSOR |
| ps2/include/menusystemdata.hpp:33 | MENU_SYSTEM_GHOBI | faithful | 0x4 | 0x4 |  | menu | CMenuSystemData::CheckGetAlready | assert active | n/a |  | ps2/include/menusystemdata.hpp MENU_SYSTEM_GHOBI |
| ps2/include/menusystemdata.hpp:108 | CMenuSystemData | faithful | 0x308 | 0x308 |  | boot:8 | CMenuSystemData::MenuSystemDataInit | assert active | n/a |  | ps2/include/menusystemdata.hpp CMenuSystemData |
| ps2/include/mg_camera.hpp:299 | mgCCamera | runtime record | 0x70 | 0x70 | vptr | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/mg_camera.hpp mgCCamera |
| ps2/include/mg_camera.hpp:547 | mgCCameraFollow | runtime record | 0xC0 | 0xC0 | vptr | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/mg_camera.hpp mgCCameraFollow |
| ps2/include/mg_dataset.hpp:71 | MDS_HEADER | faithful | 0x10 | 0x10 |  | boot:49 | CMapSky::LoadPack | assert active | n/a |  | ps2/include/mg_dataset.hpp MDS_HEADER |
| ps2/include/mg_dataset.hpp:87 | MDTOBJ_HEADER | faithful | 0x70 | 0x70 |  | boot:49 | CreateFrameVisual | assert active | n/a |  | ps2/include/mg_dataset.hpp MDTOBJ_HEADER |
| ps2/include/mg_dataset.hpp:99 | mgCreateVisualType | runtime record | 0x8 | 0x10 | name:pointer | boot:49 | CMapSky::LoadPack | assert neutralised | n/a |  | ps2/include/mg_dataset.hpp mgCreateVisualType |
| ps2/include/mg_dataset.hpp:117 | mgLoadData | runtime record | 0x40 | 0x60 | matrix:pointer, mds:pointer, memory:pointer, texture_manager:pointer +3 | boot:49 | mgLoadMDSFile | assert neutralised | n/a |  | ps2/include/mg_dataset.hpp mgLoadData |
| ps2/include/mg_dataset.hpp:292 | mgCMDTBuilder | runtime record | 0x90 | 0xB0 | header:pointer, memory:pointer, mgCMDTBuilder::(anonymous union at /work/chronicletwo/ps2/include/mg_dataset.hpp:129:5):pointer, mgCMDTBuilder::(anonymous union at /work/chronicletwo/ps2/include/mg_dataset.hpp:134:5):pointer +3 | mode | mgCMDTBuilder::AddFace | assert neutralised | n/a |  | ps2/include/mg_dataset.hpp mgCMDTBuilder |
| ps2/include/mg_drawenv.hpp:65 | mgVu0FBOX | faithful | 0x20 | 0x20 |  | boot:49 | CMap::Initialize | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgVu0FBOX |
| ps2/include/mg_drawenv.hpp:96 | mgPOINT_LIGHT | faithful | 0x30 | 0x30 |  | mode | CScene::UpDateMapInfo | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgPOINT_LIGHT |
| ps2/include/mg_drawenv.hpp:110 | mgLIGHT_INFO | faithful | 0x150 | 0x150 |  | mode | mgCFrame::Draw | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgLIGHT_INFO |
| ps2/include/mg_drawenv.hpp:149 | mgFOG_PARAM | faithful | 0x30 | 0x30 |  | boot:20 | mgFlushRenderInfo | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgFOG_PARAM |
| ps2/include/mg_drawenv.hpp:234 | mgCDrawEnv | faithful | 0x40 | 0x40 |  | boot:0 | mgCDrawEnv::Initialize | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgCDrawEnv |
| ps2/include/mg_drawenv.hpp:272 | mgCVisualAttr | faithful | 0x18 | 0x18 |  | boot:49 | CMapSky::LoadPack | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgCVisualAttr |
| ps2/include/mg_drawenv.hpp:337 | mgCFrameAttr | faithful | 0x90 | 0x90 |  | boot:49 | CMapSky::LoadPack | assert active | n/a |  | ps2/include/mg_drawenv.hpp mgCFrameAttr |
| ps2/include/mg_drawenv.hpp:621 | mgRENDER_INFO | runtime record | 0x1020 | 0x1030 | attr:pointer | boot:11 | mgInit | assert neutralised | n/a |  | ps2/include/mg_drawenv.hpp mgRENDER_INFO |
| ps2/include/mg_drawprim.hpp:123 | mgSORT_PACKET | runtime record | 0x10 | 0x20 | common:pointer, next:pointer, packet:pointer | mode | mgCDrawManager::AddPacket | assert neutralised | n/a |  | ps2/include/mg_drawprim.hpp mgSORT_PACKET |
| ps2/include/mg_drawprim.hpp:265 | mgCDrawManager | runtime record | 0x80 | 0xB8 | data_memory:pointer, draw_order:pointer, memory:pointer, order_index:pointer +8 | boot:11 | mgInit | assert neutralised | n/a |  | ps2/include/mg_drawprim.hpp mgCDrawManager |
| ps2/include/mg_drawprim.hpp:777 | mgCDrawPrim | runtime record | 0x120 | 0x170 | draw_manager:pointer, giftag:pointer, memory:pointer, mgCDrawPrim::(anonymous union at /work/chronicletwo/ps2/include/mg_drawprim.hpp:288:5):pointer +8 | boot:7 | MainLoop | assert neutralised | n/a |  | ps2/include/mg_drawprim.hpp mgCDrawPrim |
| ps2/include/mg_frame.hpp:250 | mgCObject | runtime record | 0x50 | 0x50 | vptr | boot:49 | CMapParts::UpDatePosition | assert neutralised | n/a |  | ps2/include/mg_frame.hpp mgCObject |
| ps2/include/mg_frame.hpp:277 | mgCFrameBase | runtime record | 0x50 | 0x50 | vptr | dead |  | assert neutralised | n/a |  | ps2/include/mg_frame.hpp mgCFrameBase |
| ps2/include/mg_frame.hpp:774 | mgCFrame::BoundInfo | faithful | 0xB0 | 0xB0 |  | boot:49 | CreateFrameVisual | assert active | n/a |  | ps2/include/mg_frame.hpp mgCFrame::BoundInfo |
| ps2/include/mg_frame.hpp:775 | mgCFrame | runtime record | 0x110 | 0x130 | attr:pointer, bound:pointer, brother:pointer, child:pointer +7 | boot:49 | CMapSky::LoadPack | assert neutralised | n/a |  | ps2/include/mg_frame.hpp mgCFrame |
| ps2/include/mg_memory.hpp:36 | mgMEMORY_BLOCK | runtime record | 0x10 | 0x18 | data:pointer, next:pointer | mode | mgCMemory::EndStackMode | assert neutralised | n/a |  | ps2/include/mg_memory.hpp mgMEMORY_BLOCK |
| ps2/include/mg_memory.hpp:245 | mgCMemory | runtime record | 0x30 | 0x48 | heap:pointer, heap_top:pointer, mgCMemory::(anonymous union at /work/chronicletwo/ps2/include/mg_memory.hpp:52:5):pointer, stack_block:pointer | boot:0 | CAquarium::Clear | assert neutralised | n/a |  | ps2/include/mg_memory.hpp mgCMemory |
| ps2/include/mg_shadow.hpp:81 | mgCShadowMDT | runtime record | 0x50 | 0x80 | colour:pointer, draw_env:pointer, face_group:pointer, material:pointer +5 | boot:49 | CreateFrameVisual | assert neutralised | n/a |  | ps2/include/mg_shadow.hpp mgCShadowMDT |
| ps2/include/mg_shadow.hpp:93 | mgCShadowFixMDT | runtime record | 0x50 | 0x80 | colour:pointer, draw_env:pointer, face_group:pointer, material:pointer +5 | boot:49 | CreateFrameVisual | assert neutralised | n/a |  | ps2/include/mg_shadow.hpp mgCShadowFixMDT |
| ps2/include/mg_sprite.hpp:100 | mg3DSpriteRenderInfo | faithful | 0x280 | 0x280 |  | dead |  | assert active | n/a |  | ps2/include/mg_sprite.hpp mg3DSpriteRenderInfo |
| ps2/include/mg_sprite.hpp:340 | mgC3DSprite | runtime record | 0x50 | 0x80 | batch_giftag:pointer, batch_header:pointer, batch_tag:pointer, draw_env:pointer +6 | boot:49 | CEffectList::LoadEFPFile | assert neutralised | n/a |  | ps2/include/mg_sprite.hpp mgC3DSprite |
| ps2/include/mg_tanime.hpp:131 | mgRect<int> | faithful | 0x10 | 0x10 |  | boot:42 | PrimQuad | assert active | n/a |  | ps2/include/mg_tanime.hpp mgRect<int> |
| ps2/include/mg_texture.hpp:144 | mgIMG_FILE_HEADER | faithful | 0x10 | 0x10 |  | boot:19 | mgCTextureManager::EnterIMGFile | assert active | n/a |  | ps2/include/mg_texture.hpp mgIMG_FILE_HEADER |
| ps2/include/mg_texture.hpp:157 | mgIMG1_HEADER | faithful | 0x30 | 0x30 |  | boot:19 | mgCTextureManager::EnterIMGFile | assert active | n/a |  | ps2/include/mg_texture.hpp mgIMG1_HEADER |
| ps2/include/mg_texture.hpp:193 | mgIMG_HEADER | faithful | 0x40 | 0x40 |  | boot:19 | mgCTextureManager::EnterIMGFile | assert active | n/a |  | ps2/include/mg_texture.hpp mgIMG_HEADER |
| ps2/include/mg_texture.hpp:207 | mgCEnterIMGInfo | faithful | 0x100 | 0x100 |  | boot:19 | mgCTextureManager::EnterIMGFile | assert active | n/a |  | ps2/include/mg_texture.hpp mgCEnterIMGInfo |
| ps2/include/mg_texture.hpp:283 | mgCTexture | runtime record | 0x70 | 0x88 | clut:pointer, image:pointer, next:pointer | boot:13 | SCElogoFade | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgCTexture |
| ps2/include/mg_texture.hpp:344 | mgCTexAnimeData | runtime record | 0x34 | 0x40 | dest_tex:pointer, src_tex:pointer | boot:42 | mgCTextureAnime::TexAnime | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgCTexAnimeData |
| ps2/include/mg_texture.hpp:345 | CList<mgCTexAnimeData> | runtime record | 0x40 | 0x58 | data:pointer, next:pointer, prev:pointer | boot:42 | mgCTextureAnime::TexAnime | assert neutralised | n/a |  | ps2/include/mg_texture.hpp CList<mgCTexAnimeData> |
| ps2/include/mg_texture.hpp:512 | mgCTextureAnime | runtime record | 0x1E4 | 0x308 | list:pointer, name:pointer, now:pointer | boot:19 | mgCTextureManager::LoadCFGFile | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgCTextureAnime |
| ps2/include/mg_texture.hpp:569 | mgCTextureBlock | runtime record | 0x10 | 0x18 | anime:pointer, texture:pointer | boot:16 | mgCTextureBlock::Initialize | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgCTextureBlock |
| ps2/include/mg_texture.hpp:582 | mgTEXTURE_HASH | runtime record | 0x8 | 0x10 | next:pointer, texture:pointer | boot:16 | mgCTextureManager::Initialize | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgTEXTURE_HASH |
| ps2/include/mg_texture.hpp:946 | mgCTextureManager | runtime record | 0x21C | 0x3E8 | blocks:pointer, fix_block:pointer, hash_buf:pointer, hash_stack:pointer +4 | boot:13 | SCElogoFade | assert neutralised | n/a |  | ps2/include/mg_texture.hpp mgCTextureManager |
| ps2/include/mg_visual.hpp:69 | MDT_HEADER | faithful | 0x40 | 0x40 |  | boot:49 | CreateFrameVisual | assert active | n/a |  | ps2/include/mg_visual.hpp MDT_HEADER |
| ps2/include/mg_visual.hpp:86 | MDT_MATERIAL_ | faithful | 0x60 | 0x60 |  | boot:49 | CopyMaterial | assert active | n/a |  | ps2/include/mg_visual.hpp MDT_MATERIAL_ |
| ps2/include/mg_visual.hpp:100 | MDT_FACES | faithful | 0x10 | 0x10 |  | boot:49 | mgCVisualMDT::DataAssignMDT | assert active | n/a |  | ps2/include/mg_visual.hpp MDT_FACES |
| ps2/include/mg_visual.hpp:125 | mgMaterial | runtime record | 0x30 | 0x30 | texture:pointer | boot:49 | CopyMaterial | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgMaterial |
| ps2/include/mg_visual.hpp:148 | mgCFace | runtime record | 0x30 | 0x30 | index:pointer, next:pointer | boot:49 | mgCVisualMDT::CreateFace | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgCFace |
| ps2/include/mg_visual.hpp:166 | mgFACE_GROUP | runtime record | 0x20 | 0x38 | face:pointer, next:pointer, packet:pointer | boot:49 | mgCVisualMDT::CreateFace | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgFACE_GROUP |
| ps2/include/mg_visual.hpp:322 | mgCVisual | runtime record | 0x20 | 0x30 | draw_env:pointer, texture_manager:pointer, vptr | boot:49 | mgC3DSprite::Initialize | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgCVisual |
| ps2/include/mg_visual.hpp:559 | mgCVisualMDT | runtime record | 0x50 | 0x80 | colour:pointer, draw_env:pointer, face_group:pointer, material:pointer +5 | boot:49 | CMapSky::LoadPack | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgCVisualMDT |
| ps2/include/mg_visual.hpp:634 | mgCVisualFixMDT | runtime record | 0x50 | 0x80 | colour:pointer, draw_env:pointer, face_group:pointer, material:pointer +5 | boot:49 | CreateFrameVisual | assert neutralised | n/a |  | ps2/include/mg_visual.hpp mgCVisualFixMDT |
| ps2/include/mg_visual.hpp:879 | mgVisualGifTag | faithful | 0x10 | 0x10 |  | mode | mgCVisualMDT::SetPModeRef | assert active | n/a |  | ps2/include/mg_visual.hpp mgVisualGifTag |
| ps2/include/mglib.hpp:98 | MG_PICKZ | faithful | 0x10 | 0x10 |  | boot:23 | mgEndFrame | assert active | n/a |  | ps2/include/mglib.hpp MG_PICKZ |
| ps2/include/monster.hpp:181 | MONSTER_SCOOP | runtime record | 0x18 | 0x20 | motion:pointer | mode | CActiveMonster::Initialize | assert neutralised | n/a |  | ps2/include/monster.hpp MONSTER_SCOOP |
| ps2/include/monster.hpp:195 | MONSTER_STATUS | faithful | 0xC | 0xC |  | mode | CActiveMonster::CheckStatusAttr | assert active | n/a |  | ps2/include/monster.hpp MONSTER_STATUS |
| ps2/include/monster.hpp:209 | MONSTER_REACT | faithful | 0x8 | 0x8 |  | mode | CMonsterMan::CheckDamage | assert active | n/a |  | ps2/include/monster.hpp MONSTER_REACT |
| ps2/include/monster.hpp:371 | CActiveMonster | runtime record | 0x14A0 | 0x1B80 | accume:pointer, accume_effect:pointer, base_tbl:pointer, catch_frame:pointer +49 | mode | AutoSetMonster | assert neutralised | n/a |  | ps2/include/monster.hpp CActiveMonster |
| ps2/include/monster.hpp:384 | MONSTER_REFER | runtime record | 0x14C0 | 0x1BA0 | chara:pointer, chara:vptr, script:pointer | mode | CMonsterMan::Initialize | assert neutralised | n/a |  | ps2/include/monster.hpp MONSTER_REFER |
| ps2/include/monster.hpp:410 | CMonsterLocateInfo | faithful | 0x8C | 0x8C |  | mode | AutoSetMonster | assert active | n/a |  | ps2/include/monster.hpp CMonsterLocateInfo |
| ps2/include/monster.hpp:731 | CMonsterMan | runtime record | 0x100F0 | 0x15610 | active:pointer, effect_man:pointer, memory:pointer, refer:pointer +2 | mode | AutoSetMonster | assert neutralised | n/a |  | ps2/include/monster.hpp CMonsterMan |
| ps2/include/movie.hpp:64 | TimeStamp | runtime record | 0x18 | 0x18 | dts:long, pts:long | boot:61 | CMovie::videoDecCreate | assert neutralised | n/a | plain long: 32-bit on LLP64; the movie code is replaced in T13e | ps2/include/movie.hpp TimeStamp |
| ps2/include/movie.hpp:76 | QWORD | runtime record | 0x10 | 0x10 | l:long | boot:61 | scTag2 | assert neutralised | n/a | plain long: 32-bit on LLP64; the movie code is replaced in T13e | ps2/include/movie.hpp QWORD |
| ps2/include/movie.hpp:106 | ViBuf | runtime record | 0x60 | 0x70 | ViBuf::(anonymous union at /work/chronicletwo/ps2/include/movie.hpp:85:5):pointer, tag:pointer, total_bytes:long, ts:pointer | boot:61 | getFIFOindex | assert neutralised | n/a |  | ps2/include/movie.hpp ViBuf |
| ps2/include/movie.hpp:123 | VideoDec | runtime record | 0xB8 | 0xC8 | mpeg:long, mpeg:pointer, vibuf:long, vibuf:pointer | boot:61 | CMovie::videoDecCreate | assert neutralised | n/a |  | ps2/include/movie.hpp VideoDec |
| ps2/include/movie.hpp:134 | VoData | faithful | 0xE0000 | 0xE0000 |  | boot:61 | CMovie::Load | assert active | n/a |  | ps2/include/movie.hpp VoData |
| ps2/include/movie.hpp:148 | VoTag | runtime record | 0x48 | 0x50 | v:pointer | boot:61 | CMovie::Load | assert neutralised | n/a |  | ps2/include/movie.hpp VoTag |
| ps2/include/movie.hpp:164 | VoBuf | runtime record | 0x18 | 0x28 | data:pointer, ring_tag:pointer, tag:pointer | boot:61 | voBufCreate | assert neutralised | n/a |  | ps2/include/movie.hpp VoBuf |
| ps2/include/movie.hpp:180 | StrFile | runtime record | 0x34 | 0x38 | iop_buf:pointer | boot:61 | CMovie::Load | assert neutralised | n/a |  | ps2/include/movie.hpp StrFile |
| ps2/include/movie.hpp:218 | AudioDec | runtime record | 0x5C | 0x60 | data:pointer | boot:61 | audioDecBeginPut | assert neutralised | n/a |  | ps2/include/movie.hpp AudioDec |
| ps2/include/movie.hpp:443 | CMovie | runtime record | 0x23940 | 0x23978 | image_tag:pointer, mpeg_work:pointer, time_stamp:long, vi_buf_data:pointer +3 | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/movie.hpp CMovie |
| ps2/include/movieviewlp.hpp:23 | MOVIE_LIST_ENTRY | runtime record | 0xC | 0x18 | file_name:pointer, name:pointer | debug | MovieViewInit | assert neutralised | n/a |  | ps2/include/movieviewlp.hpp MOVIE_LIST_ENTRY |
| ps2/include/nameregi.hpp:77 | NAMEREGI_TARGET_INFO | runtime record | 0x48 | 0x50 | item:pointer | mode | CNameRegiMenu::KeyStep | assert neutralised | n/a |  | ps2/include/nameregi.hpp NAMEREGI_TARGET_INFO |
| ps2/include/nameregi.hpp:114 | NAMEREGI_KANJI_INDEX | runtime record | 0x10 | 0x18 | list:pointer | mode | CheckChronicleKanjiFont | assert neutralised | n/a |  | ps2/include/nameregi.hpp NAMEREGI_KANJI_INDEX |
| ps2/include/nameregi.hpp:294 | CNameRegiMenu | runtime record | 0xBC8 | 0xBE0 | ask_para:pointer, script:pointer, vptr | mode | CNameRegiMenu::CNameRegiMenu | assert neutralised | n/a |  | ps2/include/nameregi.hpp CNameRegiMenu |
| ps2/include/nd_meswin.hpp:194 | MES_WIN_TBL | faithful | 0x10 | 0x10 |  | boot:29 | ClsMes::InitMesWinTbl | assert active | n/a |  | ps2/include/nd_meswin.hpp MES_WIN_TBL |
| ps2/include/nd_meswin.hpp:1066 | ClsMes | runtime record | 0x2958 | 0x2970 | buff:pointer, buff_system:pointer, mes_data:pointer, text_ptr:pointer | boot:29 | ClsMes::GetDrawSpeedDef | assert neutralised | n/a |  | ps2/include/nd_meswin.hpp ClsMes |
| ps2/include/nowload.hpp:53 | NowLoadingInfo | runtime record | 0x3C | 0x58 | memory:pointer | mode | CreateNowLoading | assert neutralised | n/a |  | ps2/include/nowload.hpp NowLoadingInfo |
| ps2/include/nowload.hpp:66 | PAUSE_INFO | runtime record | 0x8 | 0x10 | scene:pointer | mode | LoopDungeonMain | assert neutralised | n/a |  | ps2/include/nowload.hpp PAUSE_INFO |
| ps2/include/npccfg.hpp:62 | NPC_BASE_DATA | faithful | 0x36 | 0x36 |  | boot:30 | CUserDataManager::JoinPartyChara | assert active | n/a |  | ps2/include/npccfg.hpp NPC_BASE_DATA |
| ps2/include/object.hpp:210 | CObject | runtime record | 0x70 | 0x70 | vptr | boot:49 | CMapParts::Initialize | assert neutralised | n/a |  | ps2/include/object.hpp CObject |
| ps2/include/object.hpp:327 | CObjectFrame | runtime record | 0x80 | 0x70 | frame:pointer, vptr | boot:49 | CObjAnime::AssignFuncAnime | assert neutralised | n/a |  | ps2/include/object.hpp CObjectFrame |
| ps2/include/occlusion.hpp:53 | COcclusion | faithful | 0xC0 | 0xC0 |  | boot:49 | CMap::Initialize | assert active | n/a |  | ps2/include/occlusion.hpp COcclusion |
| ps2/include/outline.hpp:90 | COutLineDraw | runtime record | 0x70 | 0x80 | frame:pointer, next:pointer, texture:pointer | mode | CCharacter2::AddOutLine | assert neutralised | n/a |  | ps2/include/outline.hpp COutLineDraw |
| ps2/include/padcontrol.hpp:124 | PAD_CTRL_BTN | faithful | 0x8 | 0x8 |  | boot:28 | CPadControl::Initialize | assert active | n/a |  | ps2/include/padcontrol.hpp PAD_CTRL_BTN |
| ps2/include/padcontrol.hpp:136 | PAD_CTRL_ANALOG | faithful | 0x8 | 0x8 |  | boot:28 | CPadControl::Initialize | assert active | n/a |  | ps2/include/padcontrol.hpp PAD_CTRL_ANALOG |
| ps2/include/padcontrol.hpp:213 | CPadControl | faithful | 0x510 | 0x510 |  | boot:28 | CPadControl::Initialize | assert active | n/a |  | ps2/include/padcontrol.hpp CPadControl |
| ps2/include/pot.hpp:129 | CFragment | runtime record | 0x60 | 0x60 | frame:pointer | mode | CBPot::SetObject2 | assert neutralised | n/a |  | ps2/include/pot.hpp CFragment |
| ps2/include/pot.hpp:200 | CBPot | runtime record | 0xC50 | 0xC50 | fragment:pointer, frame:pointer, offset:pointer, parts:pointer +1 | mode | CBPot::Clash | assert neutralised | n/a |  | ps2/include/pot.hpp CBPot |
| ps2/include/pot.hpp:311 | CPot | runtime record | 0x80 | 0x80 | parts:pointer | mode | CPot::Bakuhatsu | assert neutralised | n/a |  | ps2/include/pot.hpp CPot |
| ps2/include/prespr.hpp:79 | CPreSprite | runtime record | 0x130 | 0x180 | draw_manager:pointer, giftag:pointer, memory:pointer, mgCDrawPrim::(anonymous union at /work/chronicletwo/ps2/include/mg_drawprim.hpp:288:5):pointer +8 | boot:65 | TitleCopyRightDraw | assert neutralised | n/a |  | ps2/include/prespr.hpp CPreSprite |
| ps2/include/quest.hpp:50 | QUEST_INFO | faithful | 0x3D0 | 0x3D0 |  | mode | CQuestManager::GetQuestInfo | assert active | n/a |  | ps2/include/quest.hpp QUEST_INFO |
| ps2/include/quest.hpp:100 | CQuestManager | runtime record | 0x8 | 0x10 | info:pointer | mode | CQuestManager::GetQuestInfo | assert neutralised | n/a |  | ps2/include/quest.hpp CQuestManager |
| ps2/include/quest.hpp:113 | QUEST_PLAY_DATA | faithful | 0x10 | 0x10 |  | mode | CQuestData::GetPlayQuestData | assert active | n/a |  | ps2/include/quest.hpp QUEST_PLAY_DATA |
| ps2/include/quest.hpp:173 | CQuestData | faithful | 0x480 | 0x480 |  | boot:8 | CQuestData::Initialize | assert active | n/a |  | ps2/include/quest.hpp CQuestData |
| ps2/include/quest.hpp:186 | MONSTER_BOOK_ENTRY | faithful | 0xC | 0xC |  | mode | CMonsterBook::CountKill | assert active | n/a |  | ps2/include/quest.hpp MONSTER_BOOK_ENTRY |
| ps2/include/quest.hpp:208 | CMonsterBook | faithful | 0x1200 | 0x1200 |  | mode | CMonsterBook::CountKill | assert active | n/a |  | ps2/include/quest.hpp CMonsterBook |
| ps2/include/runscript.hpp:148 | RS_STACKDATA | runtime record | 0x8 | 0x10 | val:pointer | mode | CEventScriptArg::BuildArgData | assert neutralised | n/a | 16 bytes on the host; strides the PS2 code writes as literals use PortSize | ps2/include/runscript.hpp RS_STACKDATA |
| ps2/include/runscript.hpp:166 | vmcode_t | faithful | 0xC | 0xC |  | mode | CRunScript::call_func | assert active | n/a |  | ps2/include/runscript.hpp vmcode_t |
| ps2/include/runscript.hpp:186 | funcdata | disk record | 0x10 | 0x10 |  | mode | CRunScript::call_func | name is a u32 under PORT | repaired | read in place from the script file | ps2/include/runscript.hpp funcdata |
| ps2/include/runscript.hpp:199 | RS_CALLDATA | runtime record | 0xC | 0x18 | frame:pointer, func:pointer, ret:pointer | mode | CEventScriptArg::BuildArgData | assert neutralised | n/a |  | ps2/include/runscript.hpp RS_CALLDATA |
| ps2/include/runscript.hpp:211 | RS_PROGDATA | faithful | 0x8 | 0x8 |  | mode | CRunScript::check_program | assert active | n/a |  | ps2/include/runscript.hpp RS_PROGDATA |
| ps2/include/runscript.hpp:455 | CRunScript | runtime record | 0x54 | 0x90 | call:pointer, call_end:pointer, call_sp:pointer, code:pointer +9 | mode | CActionChara::RunScript | assert neutralised | n/a |  | ps2/include/runscript.hpp CRunScript |
| ps2/include/runscript_opcodes.hpp:210 | RS_EXTFUNC_INFO | runtime record | 0x8 | 0x10 | func:pointer | mode | CEventScriptArg::BuildArgData | assert neutralised | n/a |  | ps2/include/runscript_opcodes.hpp RS_EXTFUNC_INFO |
| ps2/include/savedata.hpp:188 | SV_CONFIG_OPTION | faithful | 0x40 | 0x40 |  | boot:7 | MainLoop | assert active | n/a |  | ps2/include/savedata.hpp SV_CONFIG_OPTION |
| ps2/include/savedata.hpp:205 | SAVE_TOUR_INFO | faithful | 0x1C | 0x1C |  | boot:8 | CSaveData::Initialize | assert active | n/a |  | ps2/include/savedata.hpp SAVE_TOUR_INFO |
| ps2/include/savedata.hpp:502 | CSaveData | save record | 0x65930 | 0x65A20 | user_data:long, user_data:pointer | boot:7 | MainLoop | host record port/src/records/save_image.hpp | repaired | inside SAVEDATA_FORMAT | ps2/include/savedata.hpp CSaveData |
| ps2/include/savedata.hpp:521 | SPHIDA_PLAYER_DATA | faithful | 0x50 | 0x50 |  | mode | CSphidaData::ClearPlayerScore | assert active | n/a |  | ps2/include/savedata.hpp SPHIDA_PLAYER_DATA |
| ps2/include/savedata.hpp:639 | CSphidaData | faithful | 0x1848 | 0x1848 |  | boot:56 | CSphidaData::Initialize | assert active | n/a |  | ps2/include/savedata.hpp CSphidaData |
| ps2/include/savedata.hpp:671 | GYORACE_DATA | faithful | 0xA0 | 0xA0 |  | mode | CGyoRaceData::GetData | assert active | n/a |  | ps2/include/savedata.hpp GYORACE_DATA |
| ps2/include/savedata.hpp:734 | CGyoRaceData | faithful | 0x2C28 | 0x2C28 |  | boot:56 | CGyoRaceData::Initialize | assert active | n/a |  | ps2/include/savedata.hpp CGyoRaceData |
| ps2/include/savedata.hpp:802 | CSubGameData | faithful | 0x5470 | 0x5470 |  | boot:7 | MainLoop | assert active | n/a |  | ps2/include/savedata.hpp CSubGameData |
| ps2/include/savedatadungeon.hpp:56 | DNG_FLOOR_SAVE | faithful | 0x14 | 0x14 |  | boot:8 | CSaveDataDungeon::GetFloorInfoPtr | assert active | n/a |  | ps2/include/savedatadungeon.hpp DNG_FLOOR_SAVE |
| ps2/include/savedatadungeon.hpp:103 | CSaveDataDungeon | faithful | 0xCE4 | 0xCE4 |  | boot:8 | CSaveDataDungeon::GetFloorInfoPtr | assert active | n/a |  | ps2/include/savedatadungeon.hpp CSaveDataDungeon |
| ps2/include/scene.hpp:225 | CRipple | faithful | 0x30 | 0x30 |  | mode | CRain::Start | assert active | n/a |  | ps2/include/scene.hpp CRipple |
| ps2/include/scene.hpp:295 | CParticle | faithful | 0x50 | 0x50 |  | mode | CParticle::Birth | assert active | n/a |  | ps2/include/scene.hpp CParticle |
| ps2/include/scene.hpp:363 | CRainDrop | faithful | 0xB0 | 0xB0 |  | mode | CRain::Step | assert active | n/a |  | ps2/include/scene.hpp CRainDrop |
| ps2/include/scene.hpp:464 | CRain | faithful | 0xABF0 | 0xABF0 |  | mode | CRain::Draw | assert active | n/a |  | ps2/include/scene.hpp CRain |
| ps2/include/scene.hpp:494 | CSceneData | runtime record | 0x34 | 0x38 | stack:pointer | boot:0 | CSceneCamera::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneData |
| ps2/include/scene.hpp:531 | CSceneCharacter | runtime record | 0x40 | 0x48 | chara:pointer, stack:pointer | boot:0 | CSceneCharacter::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneCharacter |
| ps2/include/scene.hpp:566 | CSceneMap | runtime record | 0x38 | 0x40 | map:pointer, stack:pointer | boot:0 | CSceneMap::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneMap |
| ps2/include/scene.hpp:601 | CSceneMessage | runtime record | 0x38 | 0x40 | mes:pointer, stack:pointer | boot:0 | CSceneMessage::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneMessage |
| ps2/include/scene.hpp:636 | CSceneCamera | runtime record | 0x38 | 0x40 | camera:pointer, stack:pointer | boot:0 | CSceneCamera::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneCamera |
| ps2/include/scene.hpp:671 | CSceneSky | runtime record | 0x38 | 0x40 | sky:pointer, stack:pointer | boot:0 | CSceneSky::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneSky |
| ps2/include/scene.hpp:693 | CSceneGameObj | runtime record | 0x40 | 0x48 | chara:pointer, stack:pointer | boot:0 | CSceneGameObj::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneGameObj |
| ps2/include/scene.hpp:728 | CSceneEffect | runtime record | 0x38 | 0x40 | effect:pointer, stack:pointer | boot:0 | CSceneEffect::Initialize | assert neutralised | n/a |  | ps2/include/scene.hpp CSceneEffect |
| ps2/include/sceneevent.hpp:97 | CSceneEventData | faithful | 0xD0 | 0xD0 |  | mode | CScene::GetGameObjectEvent | assert active | n/a |  | ps2/include/sceneevent.hpp CSceneEventData |
| ps2/include/sceneload.hpp:105 | SCN_LOADMAP_INFO2::MapFiles | runtime record | 0xB4 | 0xD8 | cfg_data:pointer, efp_data:pointer, ipk_data:pointer, map_data:pointer +2 | boot:49 | CScene::LoadMapFromMemory | assert neutralised | n/a |  | ps2/include/sceneload.hpp SCN_LOADMAP_INFO2::MapFiles |
| ps2/include/sceneload.hpp:106 | SCN_LOADMAP_INFO2 | runtime record | 0x1A8 | 0x1F8 | files:pointer, load_buf:pointer, stack:pointer | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/sceneload.hpp SCN_LOADMAP_INFO2 |
| ps2/include/sceneload.hpp:156 | mgCObjectStack<CList<EMAP_MESSAGE> > | faithful | 0x14 | 0x14 |  | dead | mgCObjectStack::Initialize | assert active | n/a |  | ps2/include/sceneload.hpp mgCObjectStack<CList<EMAP_MESSAGE> > |
| ps2/include/sceneseq.hpp:182 | SPLINE_KEY | faithful | 0x38 | 0x38 |  | mode | C3DSpline::SetUpSpline | assert active | n/a |  | ps2/include/sceneseq.hpp SPLINE_KEY |
| ps2/include/sceneseq.hpp:262 | C3DSpline | faithful | 0x39C | 0x39C |  | mode | C3DSpline::GetNowXYZ | assert active | n/a |  | ps2/include/sceneseq.hpp C3DSpline |
| ps2/include/sceneseq.hpp:417 | CCameraPas | faithful | 0x950 | 0x950 |  | mode | CCameraPas::AddCameraPas | assert active | n/a |  | ps2/include/sceneseq.hpp CCameraPas |
| ps2/include/sceneseq.hpp:571 | CCharaPas | faithful | 0x4B0 | 0x4B0 |  | mode | CCharaPas::AddCharaPas | assert active | n/a |  | ps2/include/sceneseq.hpp CCharaPas |
| ps2/include/sceneseq.hpp:607 | _SEN_CMR_SEQ | runtime record | 0x60 | 0x70 | next:pointer | mode | CSceneCmrSeq::AHDDelay | assert neutralised | n/a |  | ps2/include/sceneseq.hpp _SEN_CMR_SEQ |
| ps2/include/sceneseq.hpp:644 | _SEN_OBJ_SEQ | runtime record | 0x50 | 0x60 | next:pointer | mode | CSceneObjSeq::AddPas | assert neutralised | n/a |  | ps2/include/sceneseq.hpp _SEN_OBJ_SEQ |
| ps2/include/sceneseq.hpp:1213 | CSceneCmrSeq | runtime record | 0xB10 | 0xB50 | ahd_keep:pointer, ahd_last:pointer, ahd_seq:pointer, chara_last:pointer +9 | mode | CSceneCmrSeq::AHDDelay | assert neutralised | n/a |  | ps2/include/sceneseq.hpp CSceneCmrSeq |
| ps2/include/sceneseq.hpp:1825 | CSceneObjSeq | runtime record | 0x5F0 | 0x630 | anm_last:pointer, anm_seq:pointer, col_last:pointer, col_seq:pointer +11 | mode | CSceneObjSeq::AddPas | assert neutralised | n/a |  | ps2/include/sceneseq.hpp CSceneObjSeq |
| ps2/include/scenesnd.hpp:219 | DNG_BATTLE_AREA | runtime record | 0xA8 | 0xC0 | battle_effect:pointer, floor_manager:pointer, treasure_box:pointer | boot:0 | CScene::Initialize | assert neutralised | n/a |  | ps2/include/scenesnd.hpp DNG_BATTLE_AREA |
| ps2/include/scenesnd.hpp:240 | SND_FILE_INFO | faithful | 0x24 | 0x24 |  | boot:39 | CScene::LoadSndFileInfo | assert active | n/a |  | ps2/include/scenesnd.hpp SND_FILE_INFO |
| ps2/include/scenesnd.hpp:254 | SND_REV_INFO | faithful | 0x8 | 0x8 |  | boot:39 | CScene::LoadSndFileInfo | assert active | n/a |  | ps2/include/scenesnd.hpp SND_REV_INFO |
| ps2/include/scenesnd.hpp:268 | SE_SRC_PLAY_INFO | faithful | 0x88 | 0x88 |  | boot:37 | CScene::PrePlaySeSrc | assert active | n/a |  | ps2/include/scenesnd.hpp SE_SRC_PLAY_INFO |
| ps2/include/scenesnd.hpp:2354 | CScene::BGM_INFO | runtime record | 0x460 | 0x480 | stack:pointer | boot:37 | CScene::BGM_INFO::Init | assert neutralised | n/a |  | ps2/include/scenesnd.hpp CScene::BGM_INFO |
| ps2/include/scenesnd.hpp:2355 | CScene::BGM_STATUS | faithful | 0x1C | 0x1C |  | boot:55 | TitleLoop | assert active | n/a |  | ps2/include/scenesnd.hpp CScene::BGM_STATUS |
| ps2/include/scenesnd.hpp:2356 | CScene::InScreenCharaInfo | faithful | 0xC | 0xC |  | mode | CMonsterMan::CheckPhoto | assert active | n/a |  | ps2/include/scenesnd.hpp CScene::InScreenCharaInfo |
| ps2/include/scenesnd.hpp:2357 | CScene | runtime record | 0x10550 | 0x10F50 | CScene::(anonymous union at /work/chronicletwo/ps2/include/scenesnd.hpp:358:5):pointer, battle_area:pointer, bg_load_info:pointer, bgm:pointer +20 | boot:0 | CScene::InitAllData | assert neutralised | n/a |  | ps2/include/scenesnd.hpp CScene |
| ps2/include/scenevillager.hpp:110 | GAMEOBJ_PLACE | faithful | 0x10 | 0x10 |  | mode | CScene::DrawGameObject | assert active | n/a |  | ps2/include/scenevillager.hpp GAMEOBJ_PLACE |
| ps2/include/scenevillager.hpp:126 | GAMEOBJ_INFO | faithful | 0x50 | 0x50 |  | mode | CScene::DrawGameObject | assert active | n/a |  | ps2/include/scenevillager.hpp GAMEOBJ_INFO |
| ps2/include/scriptinterpreter.hpp:113 | input_str | runtime record | 0xC | 0x10 | buffer:pointer | boot:19 | CScriptInterpreter::CScriptInterpreter | assert neutralised | n/a |  | ps2/include/scriptinterpreter.hpp input_str |
| ps2/include/scriptinterpreter.hpp:144 | SPI_STACK | runtime record | 0x8 | 0x10 | value:pointer | boot:19 | CScriptInterpreter::GetArg | assert neutralised | n/a |  | ps2/include/scriptinterpreter.hpp SPI_STACK |
| ps2/include/scriptinterpreter.hpp:164 | SPI_TAG_PARAM | runtime record | 0x8 | 0x10 | function:pointer, name:pointer | boot:19 | CScriptInterpreter::GetNextTAG | assert neutralised | n/a |  | ps2/include/scriptinterpreter.hpp SPI_TAG_PARAM |
| ps2/include/scriptinterpreter.hpp:178 | SPI_TAG_HASH | runtime record | 0x10 | 0x18 | name:pointer, next:pointer | boot:19 | CScriptInterpreter::SearchCommand | assert neutralised | n/a |  | ps2/include/scriptinterpreter.hpp SPI_TAG_HASH |
| ps2/include/scriptinterpreter.hpp:339 | CScriptInterpreter | runtime record | 0xED0 | 0x1488 | buffer:pointer, hash_buckets:pointer, hash_entries:pointer, hash_table:pointer +4 | boot:19 | CScriptInterpreter::CScriptInterpreter | assert neutralised | n/a |  | ps2/include/scriptinterpreter.hpp CScriptInterpreter |
| ps2/include/snd_mngr.hpp:162 | SND_LOOP_SE_SEQ | faithful | 0x14 | 0x14 |  | boot:37 | CLoopSeMngr::Clear | assert active | n/a |  | ps2/include/snd_mngr.hpp SND_LOOP_SE_SEQ |
| ps2/include/snd_mngr.hpp:270 | CLoopSeMngr | runtime record | 0x8 | 0x10 | loop_se:pointer | boot:37 | CLoopSeMngr::Clear | assert neutralised | n/a |  | ps2/include/snd_mngr.hpp CLoopSeMngr |
| ps2/include/snd_mngr.hpp:298 | sndSeInfo | faithful | 0xC | 0xC |  | boot:37 | sndBankInfo::GetSe | assert active | n/a |  | ps2/include/snd_mngr.hpp sndSeInfo |
| ps2/include/snd_mngr.hpp:365 | sndBankInfo | runtime record | 0x1C | 0x30 | se:pointer, seseq:pointer, sq_name:pointer | boot:0 | sndInitPort | assert neutralised | n/a |  | ps2/include/snd_mngr.hpp sndBankInfo |
| ps2/include/snd_mngr.hpp:389 | sndPortSeSeq | faithful | 0x8 | 0x8 |  | boot:0 | sndInitPort | assert active | n/a |  | ps2/include/snd_mngr.hpp sndPortSeSeq |
| ps2/include/snd_mngr.hpp:514 | sndPortInfo | runtime record | 0x29C | 0x3E0 | bank:pointer | boot:0 | sndInitPort | assert neutralised | n/a |  | ps2/include/snd_mngr.hpp sndPortInfo |
| ps2/include/snd_seseq.hpp:74 | sndSeSeqEvent | faithful | 0x6 | 0x6 |  | boot:38 | sndCSeSeqData::LoadSMF | assert active | n/a |  | ps2/include/snd_seseq.hpp sndSeSeqEvent |
| ps2/include/snd_seseq.hpp:96 | sndSeSeqVoice | faithful | 0x4 | 0x4 |  | boot:37 | sndCSeSeq::TrackNoteOff | assert active | n/a |  | ps2/include/snd_seseq.hpp sndSeSeqVoice |
| ps2/include/snd_seseq.hpp:143 | sndCSeSeqData | runtime record | 0x10 | 0x18 | event:pointer, name:pointer | boot:38 | sndBankInfo::SearchSeq | assert neutralised | n/a |  | ps2/include/snd_seseq.hpp sndCSeSeqData |
| ps2/include/snd_seseq.hpp:256 | sndTrack | faithful | 0x10 | 0x10 |  | boot:37 | sndCSeSeq::TrackNoteOff | assert active | n/a |  | ps2/include/snd_seseq.hpp sndTrack |
| ps2/include/snd_seseq.hpp:459 | sndCSeSeq | runtime record | 0xB0 | 0xC0 | data:pointer, event:pointer, loop_event:pointer | boot:37 | GetSeSeq | assert neutralised | n/a |  | ps2/include/snd_seseq.hpp sndCSeSeq |
| ps2/include/sound.hpp:53 | MIDI_FADE | faithful | 0x10 | 0x10 |  | boot:32 | CSound::Init | assert active | n/a |  | ps2/include/sound.hpp MIDI_FADE |
| ps2/include/sound.hpp:83 | MIDI_PORT | runtime record | 0x124 | 0x190 | bank:pointer, resident_sequence:pointer, sequence:pointer | boot:32 | CSound::Init | assert neutralised | n/a |  | ps2/include/sound.hpp MIDI_PORT |
| ps2/include/sound.hpp:94 | MIDI_STATE | runtime record | 0x1240 | 0x1900 | port:pointer | boot:32 | CSound::Init | assert neutralised | n/a |  | ps2/include/sound.hpp MIDI_STATE |
| ps2/include/sound.hpp:112 | MIDI_BANK | runtime record | 0x44 | 0x50 | bd_address:pointer, hd_address:pointer | boot:38 | CSound::LoadHdBd2 | assert neutralised | n/a |  | ps2/include/sound.hpp MIDI_BANK |
| ps2/include/sound.hpp:126 | MSIN_BUFFER | faithful | 0x200 | 0x200 |  | boot:32 | CSound::Init | assert active | n/a |  | ps2/include/sound.hpp MSIN_BUFFER |
| ps2/include/sound.hpp:138 | STREAM_PACK_REQUEST | faithful | 0x40 | 0x40 |  | mode | CSound::StreamOpenFromFPLFast | assert active | n/a |  | ps2/include/sound.hpp STREAM_PACK_REQUEST |
| ps2/include/sound.hpp:525 | CSound | faithful | 0x1 | 0x1 |  | boot:0 | CSound::Stop | assert active | n/a |  | ps2/include/sound.hpp CSound |
| ps2/include/sphida.hpp:72 | GOLF_CLUB_DEF | faithful | 0xC | 0xC |  | mode | CSphida::DrawStatusSprite | assert active | n/a |  | ps2/include/sphida.hpp GOLF_CLUB_DEF |
| ps2/include/sphida.hpp:132 | CPowGage | runtime record | 0x24 | 0x28 | texture:pointer | mode | CPowGage::Draw | assert neutralised | n/a |  | ps2/include/sphida.hpp CPowGage |
| ps2/include/sphida.hpp:315 | CSphida | runtime record | 0x240 | 0x250 | col_model:pointer, pow_gage:pointer, red_mark:pointer, red_mark:vptr | mode | CActionChara::RunScript | assert neutralised | n/a |  | ps2/include/sphida.hpp CSphida |
| ps2/include/subgame.hpp:83 | SubGameInfo | runtime record | 0x30 | 0x40 | load_buff:pointer, menu_buff:pointer, scene:pointer | mode | AutoCam | assert neutralised | n/a |  | ps2/include/subgame.hpp SubGameInfo |
| ps2/include/subgame.hpp:164 | sgCPlayVoice | faithful | 0x14 | 0x14 |  | mode | sgCPlayVoice::Close | assert active | n/a |  | ps2/include/subgame.hpp sgCPlayVoice |
| ps2/include/swordeffect.hpp:190 | CSWordAfterEffect | runtime record | 0xA0 | 0xD0 | frame0:pointer, frame1:pointer, point0:pointer, point1:pointer +3 | boot:52 | CSWordAfterEffect::Clear | assert neutralised | n/a |  | ps2/include/swordeffect.hpp CSWordAfterEffect |
| ps2/include/title.hpp:286 | TITLE_INFO | runtime record | 0x194 | 0x210 | chara_stack:pointer | boot:44 | TitleInit | assert neutralised | n/a |  | ps2/include/title.hpp TITLE_INFO |
| ps2/include/title.hpp:303 | RUSH_INFO | faithful | 0x18 | 0x18 |  | boot:55 | TitleLoop | assert active | n/a |  | ps2/include/title.hpp RUSH_INFO |
| ps2/include/title.hpp:325 | HDD_INFO | runtime record | 0x24 | 0x28 | work:pointer | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/title.hpp HDD_INFO |
| ps2/include/userdata.hpp:252 | CHARA_DATA | faithful | 0x38C | 0x38C |  | boot:8 | CBattleCharaInfo::RefreshParamater | assert active | n/a |  | ps2/include/userdata.hpp CHARA_DATA |
| ps2/include/userdata.hpp:306 | ROBO_DATA | faithful | 0x220 | 0x220 |  | boot:8 | CGameDataUsed::CopyGameData | assert active | n/a |  | ps2/include/userdata.hpp ROBO_DATA |
| ps2/include/userdata.hpp:322 | MOS_HENGE_PARAM | runtime record | 0x1C | 0x30 | effect_name:pointer, script_name:pointer | boot:8 | GetMonsterHengeParam | assert neutralised | n/a |  | ps2/include/userdata.hpp MOS_HENGE_PARAM |
| ps2/include/userdata.hpp:396 | MOS_CHANGE_PARAM | faithful | 0xBC | 0xBC |  | boot:8 | CMonsterBox::GetMonsterBajjiData | assert active | n/a |  | ps2/include/userdata.hpp MOS_CHANGE_PARAM |
| ps2/include/userdata.hpp:468 | CMonsterBox | faithful | 0x2F00 | 0x2F00 |  | boot:8 | CMonsterBox::GetMonsterBajjiData | assert active | n/a |  | ps2/include/userdata.hpp CMonsterBox |
| ps2/include/userdata.hpp:577 | CFishAquarium | faithful | 0x530 | 0x530 |  | boot:8 | CFishAquarium::Initialize | assert active | n/a |  | ps2/include/userdata.hpp CFishAquarium |
| ps2/include/userdata.hpp:593 | FISH_RECORD | faithful | 0x20 | 0x20 |  | mode | CFishingRecord::CheckRecordFish | assert active | n/a |  | ps2/include/userdata.hpp FISH_RECORD |
| ps2/include/userdata.hpp:636 | CFishingRecord | faithful | 0x340 | 0x340 |  | mode | CFishingRecord::CheckRecordFish | assert active | n/a |  | ps2/include/userdata.hpp CFishingRecord |
| ps2/include/userdata.hpp:650 | FISH_TOURNAMENT_ENTRY | faithful | 0x8 | 0x8 |  | mode | CFishingTournament::CalcTopWeight | assert active | n/a |  | ps2/include/userdata.hpp FISH_TOURNAMENT_ENTRY |
| ps2/include/userdata.hpp:752 | CFishingTournament | faithful | 0x70 | 0x70 |  | mode | CFishingTournament::CalcTopWeight | assert active | n/a |  | ps2/include/userdata.hpp CFishingTournament |
| ps2/include/userdata.hpp:766 | PARTY_CHARA_INFO | faithful | 0xC | 0xC |  | boot:8 | CUserDataManager::Initialize | assert active | n/a |  | ps2/include/userdata.hpp PARTY_CHARA_INFO |
| ps2/include/userdata.hpp:785 | USER_PICTURE_INFO | save record | 0x18 | 0x20 | image:pointer | boot:8 | CInventUserData::ResetAddress | host record port/src/records/save_image.hpp | repaired | image is a 32-bit PS2 address on disk | ps2/include/userdata.hpp USER_PICTURE_INFO |
| ps2/include/userdata.hpp:797 | INVENT_CREATED_ITEM | faithful | 0x4 | 0x4 |  | boot:8 | CInventUserData::Initialize | assert active | n/a |  | ps2/include/userdata.hpp INVENT_CREATED_ITEM |
| ps2/include/userdata.hpp:810 | SCOOP_INFO | faithful | 0x4 | 0x4 |  | menu | CMenuQuestView::KeyStep | assert active | n/a |  | ps2/include/userdata.hpp SCOOP_INFO |
| ps2/include/userdata.hpp:1131 | CInventUserData | save record | 0x3CE60 | 0x3CF50 | photo:pointer | boot:8 | CInventUserData::Initialize | host record port/src/records/save_image.hpp | repaired | holds the photo album | ps2/include/userdata.hpp CInventUserData |
| ps2/include/userdata.hpp:2168 | CUserDataManager | save record | 0x457A0 | 0x45890 | costume_bit:long, invent_data:pointer | boot:7 | MainLoop | host record port/src/records/save_image.hpp | repaired | costume_bit is 64-bit on disk; long is 32-bit on LLP64 | ps2/include/userdata.hpp CUserDataManager |
| ps2/include/userdata.hpp:2182 | BATTLE_WEAPON_PARAM | faithful | 0x1C | 0x1C |  | boot:8 | CBattleCharaInfo::RefreshParamater | assert active | n/a |  | ps2/include/userdata.hpp BATTLE_WEAPON_PARAM |
| ps2/include/userdata.hpp:2612 | CBattleCharaInfo | runtime record | 0x90 | 0xA0 | active_item:pointer, chara_data:pointer, equip:pointer, hp:pointer | boot:8 | CBattleCharaInfo::RefreshParamater | assert neutralised | n/a |  | ps2/include/userdata.hpp CBattleCharaInfo |
| ps2/include/villagermngr.hpp:141 | CVillagerPlaceInfo::Node | runtime record | 0x20 | 0x30 | next:pointer | mode | CVillagerMngr::Step | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerPlaceInfo::Node |
| ps2/include/villagermngr.hpp:142 | CVillagerPlaceInfo | runtime record | 0x40 | 0x50 | route:pointer | mode | CScene::GetLoadVillagerList | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerPlaceInfo |
| ps2/include/villagermngr.hpp:186 | CVillagerPlace::ProgressInfo | runtime record | 0x28 | 0x48 | place:pointer | boot:29 | LoadNPCInfo | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerPlace::ProgressInfo |
| ps2/include/villagermngr.hpp:187 | CVillagerPlace | runtime record | 0x8 | 0x10 | prog_info:pointer | boot:29 | LoadNPCInfo | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerPlace |
| ps2/include/villagermngr.hpp:239 | CVillagerData | runtime record | 0x70 | 0x80 | place:pointer, route:pointer | mode | CScene::CancelStayVillager | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerData |
| ps2/include/villagermngr.hpp:394 | CVillagerMngr | runtime record | 0xE10 | 0x1010 | data:pointer | mode | CScene::CancelStayVillager | assert neutralised | n/a |  | ps2/include/villagermngr.hpp CVillagerMngr |
| ps2/include/visualmotion.hpp:37 | mgVertexWeight | faithful | 0x20 | 0x20 |  | boot:49 | mgCVisualMotionMDT::CreateVertexWeight | assert active | n/a |  | ps2/include/visualmotion.hpp mgVertexWeight |
| ps2/include/visualmotion.hpp:54 | mgCVMotionData | runtime record | 0x14 | 0x28 | base_matrix:pointer, frame:pointer, weight_data:pointer | boot:49 | CreateFrameVisual | assert neutralised | n/a |  | ps2/include/visualmotion.hpp mgCVMotionData |
| ps2/include/visualmotion.hpp:203 | mgCVisualMotionMDT | runtime record | 0x110 | 0x140 | base_matrix:pointer, colour:pointer, draw_env:pointer, face_group:pointer +8 | boot:49 | CreateFrameVisual | assert neutralised | n/a |  | ps2/include/visualmotion.hpp mgCVisualMotionMDT |
| ps2/include/vlgr_info.hpp:51 | GAME_PROGRESS_INFO | runtime record | 0xC | 0x10 | name:pointer | boot:29 | LoadGameInfo | assert neutralised | n/a |  | ps2/include/vlgr_info.hpp GAME_PROGRESS_INFO |
| ps2/include/vlgr_info.hpp:79 | CVillagerInfo | runtime record | 0x1C | 0x30 | hide_frames:pointer, model_name:pointer, show_frames:pointer | mode | CScene::CharaObjectOnOff | assert neutralised | n/a |  | ps2/include/vlgr_info.hpp CVillagerInfo |
| ps2/include/water.hpp:42 | FireRasterParticle | faithful | 0x20 | 0x20 |  | mode | CFireRaster::Draw | assert active | n/a |  | ps2/include/water.hpp FireRasterParticle |
| ps2/include/water.hpp:105 | CFireRaster | runtime record | 0x2F0 | 0x310 | texture:pointer | boot:0 | CFireRaster::Initialize | assert neutralised | n/a |  | ps2/include/water.hpp CFireRaster |
| ps2/include/water.hpp:262 | CWater | runtime record | 0x80 | 0xA0 | draw_env:pointer, height:pointer, height_a:pointer, height_b:pointer +3 | mode | CWater::CWater | assert neutralised | n/a |  | ps2/include/water.hpp CWater |
| ps2/include/water.hpp:377 | CWaterFrame | runtime record | 0x120 | 0x140 | attr:pointer, bound:pointer, brother:pointer, child:pointer +7 | mode | CMap::DrawWater | assert neutralised | n/a |  | ps2/include/water.hpp CWaterFrame |
| ps2/include/wavetable.hpp:91 | CWaveTable | runtime record | 0x1208 | 0x1210 | vptr | boot:47 | TitleBootInit | assert neutralised | n/a |  | ps2/include/wavetable.hpp CWaveTable |
| ps2/include/sce/libdma.h:39 | sceDmaChcr | faithful | 0x4 | 0x4 |  | boot:11 | mgInit | assert active | n/a |  | ps2/include/sce/libdma.h sceDmaChcr |
| ps2/include/sce/libdma.h:40 | sceDmaChan | faithful | 0x90 | 0x90 |  | boot:11 | mgInit | assert active | n/a |  | ps2/include/sce/libdma.h sceDmaChan |
| ps2/include/sce/libdma.h:58 | sceDmaEnv | runtime record | 0x14 | 0x18 | mem:pointer | boot:11 | mgInit | assert neutralised | n/a |  | ps2/include/sce/libdma.h sceDmaEnv |
| ps2/include/sce/libmc.h:57 | sceMcColor | faithful | 0x10 | 0x10 |  | mode | CMemoryCardManager::SetIconData | assert active | n/a |  | ps2/include/sce/libmc.h sceMcColor |
| ps2/include/sce/libmc.h:89 | sceMcIconSys | faithful | 0x3C4 | 0x3C4 |  | boot:56 | CMemoryCardManager::MakeDir | assert active | n/a |  | ps2/include/sce/libmc.h sceMcIconSys |
| ps2/include/sce/libmc.h:104 | sceMcStDateTime | faithful | 0x8 | 0x8 |  | dead |  | assert active | n/a |  | ps2/include/sce/libmc.h sceMcStDateTime |
| ps2/include/sce/libmc.h:120 | sceMcTblGetDir | faithful | 0x40 | 0x40 |  | boot:48 | CMemoryCardManager::InitSaveFileInfoTable | assert active | n/a |  | ps2/include/sce/libmc.h sceMcTblGetDir |
| ps2/src/dngmenu.cpp:3917 | DngTreeReadNames | runtime record | 0x8 | 0x10 | name:pointer | menu | DngTreeMapInit | assert neutralised | n/a |  | ps2/src/dngmenu.cpp DngTreeReadNames |
| ps2/src/editexception.cpp:132 | EditGsTest | faithful | 0x8 | 0x8 |  | debug | CGeyserEffect::CreatePacket | assert active | n/a |  | ps2/src/editexception.cpp EditGsTest |
| ps2/src/editmenu.cpp:184 | GeoRequestCheck | faithful | 0x440 | 0x440 |  | debug | InitDownLoadAnaunce | assert active | n/a |  | ps2/src/editmenu.cpp GeoRequestCheck |
| ps2/src/inventmn.cpp:1185 | INVENT_CREATED_ITEM_OLD | faithful | 0xC | 0xC |  | boot:56 | TranslateInventUserData | assert active | n/a |  | ps2/src/inventmn.cpp INVENT_CREATED_ITEM_OLD |
| ps2/src/menuaqua.cpp:105 | aqua_food_info | faithful | 0xA | 0xA |  | menu | CAquaFish::ParamStep | assert active | n/a |  | ps2/src/menuaqua.cpp aqua_food_info |
| ps2/src/menuaqua.cpp:147 | fish_breed_pair | faithful | 0x3 | 0x3 |  | menu | GetChildFishNo | assert active | n/a |  | ps2/src/menuaqua.cpp fish_breed_pair |
| ps2/src/menuaqua.cpp:161 | aqua_fish_info | runtime record | 0xC | 0x18 | img_path:pointer | menu | GetFishImageColor | assert neutralised | n/a |  | ps2/src/menuaqua.cpp aqua_fish_info |
| ps2/src/menuaqua.cpp:205 | aqua_col_point | faithful | 0x20 | 0x20 |  | menu | CAquarium::ColCheck | assert active | n/a |  | ps2/src/menuaqua.cpp aqua_col_point |
| ps2/src/mg_visual.cpp:41 | mgVISUAL_SETUP_PACKET | faithful | 0x140 | 0x140 |  | mode | mgCVisualMDT::CreateRenderInfoPacket | assert active | n/a |  | ps2/src/mg_visual.cpp mgVISUAL_SETUP_PACKET |
| ps2/src/prespr.cpp:15 | SpriteGsPacket | faithful | 0x10 | 0x10 |  | mode | CPreSprite::SetAlphaBlend | assert active | n/a |  | ps2/src/prespr.cpp SpriteGsPacket |
| ps2/src/water.cpp:47 | WaterRenderPacket | faithful | 0x260 | 0x260 |  | mode | CWater::CreateRenderInfoPacket | assert active | n/a |  | ps2/src/water.cpp WaterRenderPacket |
| ps2/src/water.cpp:60 | WaterStripPacket | faithful | 0x30 | 0x30 |  | mode | CWater::CreatePacket | assert active | n/a |  | ps2/src/water.cpp WaterStripPacket |
| ps2/src/water.cpp:73 | WaterDmaTag | faithful | 0x10 | 0x10 |  | mode | CWater::CreatePacket | assert active | n/a |  | ps2/src/water.cpp WaterDmaTag |
| ps2/src/water.cpp:86 | WaterFinishPacket | faithful | 0x30 | 0x30 |  | mode | CWater::CreatePacket | assert active | n/a |  | ps2/src/water.cpp WaterFinishPacket |

## Casts

114 rows: guarded-unreached 106, n/a 8.

| site | function | kind | expression | first_use | fix | state | note | key |
|---|---|---|---|---|---|---|---|---|
| ps2/src/character.cpp:813 | CCharacter2::SetMotionPara | value test | `(int)sequence` | mode | - | n/a | the pointer argument carries a small integer selector | ps2/src/character.cpp CCharacter2::SetMotionPara (int)sequence |
| ps2/src/dataread.cpp:532 | InitCDFile | escapes cast | `(int)header_buff` | boot:6 | override [dataread:InitCDFile] (T5) | guarded-unreached |  | ps2/src/dataread.cpp InitCDFile (int)header_buff |
| ps2/src/editdata.cpp:303 | CEditMap::SaveData | escapes cast | `(int)out` | debug | override [editdata:CEditMap::SaveData] (T16b) | guarded-unreached |  | ps2/src/editdata.cpp CEditMap::SaveData (int)out |
| ps2/src/editdata.cpp:494 | CEditMap::LoadData | low-bits cast | `(int)in` | debug | - | n/a | tests the low bit only | ps2/src/editdata.cpp CEditMap::LoadData (int)in |
| ps2/src/ezmidi.cpp:79 | ezTransToIOP2 | resolved cast | `(u32)ee_address` | boot:37 | override [ezmidi:ezTransToIOP2] (T11) | guarded-unreached |  | ps2/src/ezmidi.cpp ezTransToIOP2 (u32)ee_address |
| ps2/src/fishing.cpp:1327 | CreateLoadThread | escapes cast | `(int)memory->Alloc(0x4001)` | mode | override [fishing:CreateLoadThread] (T6) | guarded-unreached |  | ps2/src/fishing.cpp CreateLoadThread (int)memory->Alloc(0x4001) |
| ps2/src/mainloop.cpp:556 | MainLoop | escapes cast | `(int)(read_memory->stack + read_memory->stack_used)` | boot:7 | override [mainloop:MainLoop] (T6) | guarded-unreached |  | ps2/src/mainloop.cpp MainLoop (int)(read_memory->stack + read_memory->stack_used) |
| ps2/src/mainloop.cpp:1450 | LoadFontTexture | low-bits cast | `(u32)buffer` | boot:29 | - | n/a | keeps the low two bits only | ps2/src/mainloop.cpp LoadFontTexture (u32)buffer |
| ps2/src/menusys.cpp:6861 | CMenuItemInfo::ExitEnd | low-bits cast | `(int)*(void * *)& MenuCommonInfo->cursor` | menu | - | n/a | reads the int cursor through a pointer-sized load; the low 32 bits are the cursor on a little-endian host | ps2/src/menusys.cpp CMenuItemInfo::ExitEnd (int)*(void * *)& MenuCommonInfo->cursor |
| ps2/src/mg_dataset.cpp:686 | mgLoadMDSFile | low-bits cast | `(int)mds` | boot:49 | - | n/a | alignment test only | ps2/src/mg_dataset.cpp mgLoadMDSFile (int)mds |
| ps2/src/mg_drawprim.cpp:137 | mgCDrawPrim::Begin2 | escapes cast | `(u_int)packet_start` | boot:22 | override [mg_drawprim:mgCDrawPrim::Begin2] (T9) | guarded-unreached |  | ps2/src/mg_drawprim.cpp mgCDrawPrim::Begin2 (u_int)packet_start |
| ps2/src/mg_drawprim.cpp:699 | mgCDrawManager::Draw | escapes cast | `(u_int)(* entry)->common` | mode | override [mg_drawprim:mgCDrawManager::Draw] (T9) | guarded-unreached |  | ps2/src/mg_drawprim.cpp mgCDrawManager::Draw (u_int)(* entry)->common |
| ps2/src/mg_drawprim.cpp:707 | mgCDrawManager::Draw | escapes cast | `(u_int)(* entry)->packet` | mode | override [mg_drawprim:mgCDrawManager::Draw] (T9) | guarded-unreached |  | ps2/src/mg_drawprim.cpp mgCDrawManager::Draw (u_int)(* entry)->packet |
| ps2/src/mg_memory.cpp:234 | mgCMemory::stAlign64 | low-bits cast | `(u_int)& stack[stack_used]` | boot:42 | - | n/a | keeps the low six bits only | ps2/src/mg_memory.cpp mgCMemory::stAlign64 (u_int)& stack[stack_used] |
| ps2/src/mg_memory.cpp:252 | mgCMemory::Align64 | low-bits cast | `(u_int)& stack[stack_used]` | boot:42 | - | n/a | keeps the low six bits only | ps2/src/mg_memory.cpp mgCMemory::Align64 (u_int)& stack[stack_used] |
| ps2/src/mg_shadow.cpp:63 | mgCShadowMDT::CreateFacePacket | escapes cast | `(u_int)packet` | mode | override [mg_shadow:mgCShadowMDT::CreateFacePacket] (V4) | guarded-unreached |  | ps2/src/mg_shadow.cpp mgCShadowMDT::CreateFacePacket (u_int)packet |
| ps2/src/mg_shadow.cpp:222 | mgCShadowMDT::CreatePacket | resolved cast | `(int)& face_memory->stack[face_memory->stack_used]` | mode | override [mg_shadow:mgCShadowMDT::CreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_shadow.cpp mgCShadowMDT::CreatePacket (int)& face_memory->stack[face_memory->stack_used] |
| ps2/src/mg_shadow.cpp:259 | mgCShadowMDT::CreatePacket | escapes cast | `(int)packet_start` | mode | override [mg_shadow:mgCShadowMDT::CreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_shadow.cpp mgCShadowMDT::CreatePacket (int)packet_start |
| ps2/src/mg_sprite.cpp:239 | mgC3DSprite::Draw | resolved cast | `(int)data_memory->stAllocTest(0x3C)` | mode | override [mg_sprite:mgC3DSprite::Draw] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgC3DSprite::Draw (int)data_memory->stAllocTest(0x3C) |
| ps2/src/mg_sprite.cpp:250 | mgC3DSprite::Draw | escapes cast | `(u_int)packet` | mode | override [mg_sprite:mgC3DSprite::Draw] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgC3DSprite::Draw (u_int)packet |
| ps2/src/mg_sprite.cpp:266 | mgC3DSprite::BeginCreatePacket | escapes cast | `(u_int)packet` | mode | override [mg_sprite:mgC3DSprite::BeginCreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgC3DSprite::BeginCreatePacket (u_int)packet |
| ps2/src/mg_sprite.cpp:524 | mgCSprite::CreatePacket | escapes cast | `(int)(end_tag + 4)` | mode | override [mg_sprite:mgCSprite::CreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgCSprite::CreatePacket (int)(end_tag + 4) |
| ps2/src/mg_sprite.cpp:524 | mgCSprite::CreatePacket | escapes cast | `(int)start` | mode | override [mg_sprite:mgCSprite::CreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgCSprite::CreatePacket (int)start |
| ps2/src/mg_sprite.cpp:526 | mgCSprite::CreatePacket | escapes cast | `(u_int)start` | mode | override [mg_sprite:mgCSprite::CreatePacket] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgCSprite::CreatePacket (u_int)start |
| ps2/src/mg_sprite.cpp:539 | mgCSprite::Draw | resolved cast | `(int)memory->stAllocTest(0x3C)` | mode | override [mg_sprite:mgCSprite::Draw] (V4) | guarded-unreached |  | ps2/src/mg_sprite.cpp mgCSprite::Draw (int)memory->stAllocTest(0x3C) |
| ps2/src/mg_texture.cpp:539 | mgCTextureManager::Initialize | escapes cast | `(int)blocks` | boot:16 | override [mg_texture:mgCTextureManager::Initialize] (T8) | guarded-unreached |  | ps2/src/mg_texture.cpp mgCTextureManager::Initialize (int)blocks |
| ps2/src/mg_texture.cpp:546 | mgCTextureManager::Initialize | escapes cast | `(int)this` | boot:16 | override [mg_texture:mgCTextureManager::Initialize] (T8) | guarded-unreached |  | ps2/src/mg_texture.cpp mgCTextureManager::Initialize (int)this |
| ps2/src/mg_texture.cpp:1421 | mgLoadImage | escapes cast | `(u_int)image` | boot:42 | override [mg_texture:mgLoadImage] (T8) | guarded-unreached |  | ps2/src/mg_texture.cpp mgLoadImage (u_int)image |
| ps2/src/mg_texture.cpp:1478 | mgCTextureManager::ReloadTexture | escapes cast | `(int)base` | boot:22 | override [mg_texture:mgCTextureManager::ReloadTexture] (T8) | guarded-unreached |  | ps2/src/mg_texture.cpp mgCTextureManager::ReloadTexture (int)base |
| ps2/src/mg_texture.cpp:1570 | mgCTextureManager::ReloadTexture | escapes cast | `(int)texture` | boot:22 | override [mg_texture:mgCTextureManager::ReloadTexture] (T8) | guarded-unreached |  | ps2/src/mg_texture.cpp mgCTextureManager::ReloadTexture (int)texture |
| ps2/src/mg_visual.cpp:566 | mgCVisualMDT::CreateFace | value test | `(int)out_face` | boot:49 | - | n/a | the pointer argument carries a small integer selector | ps2/src/mg_visual.cpp mgCVisualMDT::CreateFace (int)out_face |
| ps2/src/mg_visual.cpp:656 | mgCVisualMDT::Draw | escapes cast | `(int)buffer` | mode | override [mg_visual:mgCVisualMDT::Draw] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::Draw (int)buffer |
| ps2/src/mg_visual.cpp:664 | mgCVisualMDT::Draw | escapes cast | `(u_int)node->packet` | mode | override [mg_visual:mgCVisualMDT::Draw] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::Draw (u_int)node->packet |
| ps2/src/mg_visual.cpp:699 | mgCVisualMDT::CreatePacket | round-trip cast | `(u_int)(packet_memory->stack + packet_memory->stack_used)` | mode | override [mg_visual:mgCVisualMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreatePacket (u_int)(packet_memory->stack + packet_memory->stack_used) |
| ps2/src/mg_visual.cpp:701 | mgCVisualMDT::CreatePacket | escapes cast | `(u_int)(data_memory->stack + data_memory->stack_used)` | mode | override [mg_visual:mgCVisualMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreatePacket (u_int)(data_memory->stack + data_memory->stack_used) |
| ps2/src/mg_visual.cpp:752 | mgCVisualMDT::CreatePacket | escapes cast | `(int)cursor` | mode | override [mg_visual:mgCVisualMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreatePacket (int)cursor |
| ps2/src/mg_visual.cpp:752 | mgCVisualMDT::CreatePacket | escapes cast | `(int)node->packet` | mode | override [mg_visual:mgCVisualMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreatePacket (int)node->packet |
| ps2/src/mg_visual.cpp:756 | mgCVisualMDT::CreatePacket | escapes cast | `(int)cursor` | mode | override [mg_visual:mgCVisualMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreatePacket (int)cursor #2 |
| ps2/src/mg_visual.cpp:774 | mgCVisualFixMDT::CreatePacket | escapes cast | `(u_int)(data_memory->stack + data_memory->stack_used)` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (u_int)(data_memory->stack + data_memory->stack_used) |
| ps2/src/mg_visual.cpp:818 | mgCVisualFixMDT::CreatePacket | escapes cast | `(int)cursor` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (int)cursor |
| ps2/src/mg_visual.cpp:818 | mgCVisualFixMDT::CreatePacket | escapes cast | `(int)node->packet` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (int)node->packet |
| ps2/src/mg_visual.cpp:822 | mgCVisualFixMDT::CreatePacket | escapes cast | `(int)cursor` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (int)cursor #2 |
| ps2/src/mg_visual.cpp:822 | mgCVisualFixMDT::CreatePacket | escapes cast | `(int)(u_int *)manager` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (int)(u_int *)manager |
| ps2/src/mg_visual.cpp:824 | mgCVisualFixMDT::CreatePacket | escapes cast | `(u_int)manager` | mode | override [mg_visual:mgCVisualFixMDT::CreatePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualFixMDT::CreatePacket (u_int)manager |
| ps2/src/mg_visual.cpp:1109 | mgCVisualMDT::CreateFacePacket | escapes cast | `(u_int)packet` | mode | override [mg_visual:mgCVisualMDT::CreateFacePacket] (V2) | guarded-unreached |  | ps2/src/mg_visual.cpp mgCVisualMDT::CreateFacePacket (u_int)packet |
| ps2/src/mglib.cpp:563 | mgInitVif1Packet | escapes cast | `(int)packetbuf[0]` | boot:14 | override [mglib:mgInitVif1Packet] (T7) | guarded-unreached |  | ps2/src/mglib.cpp mgInitVif1Packet (int)packetbuf[0] |
| ps2/src/mglib.cpp:569 | mgInitVif1Packet | escapes cast | `(int)packetbuf[1]` | boot:14 | override [mglib:mgInitVif1Packet] (T7) | guarded-unreached |  | ps2/src/mglib.cpp mgInitVif1Packet (int)packetbuf[1] |
| ps2/src/mglib.cpp:1910 | mgSendVuProg | escapes cast | `(u_int)packet` | mode | callee [mglib:mgGetVuProgPacket] (V4) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/mglib.cpp mgSendVuProg (u_int)packet |
| ps2/src/movie.cpp:759 | getFIFOindex | escapes cast | `(u32)buf->tag` | boot:61 | override [movie:getFIFOindex] (T13e) | guarded-unreached |  | ps2/src/movie.cpp getFIFOindex (u32)buf->tag |
| ps2/src/movie.cpp:763 | getFIFOindex | escapes cast | `(u32)addr` | boot:61 | override [movie:getFIFOindex] (T13e) | guarded-unreached |  | ps2/src/movie.cpp getFIFOindex (u32)addr |
| ps2/src/movie.cpp:763 | getFIFOindex | escapes cast | `(u32)buf->data` | boot:61 | override [movie:getFIFOindex] (T13e) | guarded-unreached |  | ps2/src/movie.cpp getFIFOindex (u32)buf->data |
| ps2/src/movie.cpp:790 | viBufCreate | escapes cast | `(u32)tags` | boot:61 | override [movie:viBufCreate] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufCreate (u32)tags |
| ps2/src/movie.cpp:827 | viBufReset | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufReset] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufReset (u32)buf->data |
| ps2/src/movie.cpp:828 | viBufReset | escapes cast | `(u32)buf->tag` | boot:61 | override [movie:viBufReset] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufReset (u32)buf->tag |
| ps2/src/movie.cpp:982 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data |
| ps2/src/movie.cpp:984 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #2 |
| ps2/src/movie.cpp:985 | viBufRestartDMA | escapes cast | `(u32)DmaAddr(buf->tag)` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)DmaAddr(buf->tag) |
| ps2/src/movie.cpp:987 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #3 |
| ps2/src/movie.cpp:987 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #4 |
| ps2/src/movie.cpp:997 | viBufRestartDMA | escapes cast | `(u32)DmaAddr(& buf->tag[index])` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)DmaAddr(& buf->tag[index]) |
| ps2/src/movie.cpp:998 | viBufRestartDMA | escapes cast | `(u32)& buf->data_bytes[index << 11]` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)& buf->data_bytes[index << 11] |
| ps2/src/movie.cpp:999 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #5 |
| ps2/src/movie.cpp:999 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #6 |
| ps2/src/movie.cpp:1000 | viBufRestartDMA | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufRestartDMA] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufRestartDMA (u32)buf->data #7 |
| ps2/src/movie.cpp:1159 | viBufGetTs | escapes cast | `(u32)buf->data` | boot:61 | override [movie:viBufGetTs] (T13e) | guarded-unreached |  | ps2/src/movie.cpp viBufGetTs (u32)buf->data |
| ps2/src/movie.cpp:1234 | strFileOpen | escapes cast | `(u32)file->iop_buf` | boot:61 | override [movie:strFileOpen] (T13e) | guarded-unreached |  | ps2/src/movie.cpp strFileOpen (u32)file->iop_buf |
| ps2/src/movie.cpp:1357 | audioDecCreate | escapes cast | `(int)sceSifAllocIopHeap(iop_size)` | boot:61 | override [movie:audioDecCreate] (T13e) | guarded-unreached |  | ps2/src/movie.cpp audioDecCreate (int)sceSifAllocIopHeap(iop_size) |
| ps2/src/movie.cpp:1366 | audioDecCreate | escapes cast | `(int)sceSifAllocIopHeap(0x800)` | boot:61 | override [movie:audioDecCreate] (T13e) | guarded-unreached |  | ps2/src/movie.cpp audioDecCreate (int)sceSifAllocIopHeap(0x800) |
| ps2/src/snd_mngr.cpp:1014 | sndLoadSound | escapes cast | `(int)hd` | boot:38 | callee [sound:CSound::LoadHdBd2] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/snd_mngr.cpp sndLoadSound (int)hd |
| ps2/src/snd_mngr.cpp:1014 | sndLoadSound | escapes cast | `(int)bd` | boot:38 | callee [sound:CSound::LoadHdBd2] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/snd_mngr.cpp sndLoadSound (int)bd |
| ps2/src/snd_mngr.cpp:1026 | sndLoadSound | escapes cast | `(int)hd` | boot:38 | callee [sound:TransHdBd] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/snd_mngr.cpp sndLoadSound (int)hd #2 |
| ps2/src/snd_mngr.cpp:1026 | sndLoadSound | escapes cast | `(int)bd` | boot:38 | callee [sound:TransHdBd] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/snd_mngr.cpp sndLoadSound (int)bd #2 |
| ps2/src/snd_mngr.cpp:1055 | sndLoadSound | escapes cast | `(int)sq[i]` | boot:38 | callee [sound:CSound::LoadSeq] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/snd_mngr.cpp sndLoadSound (int)sq[i] |
| ps2/src/sound.cpp:423 | CSound::DEL_PORT | escapes cast | `(int)midi_state.port[port].resident_sequence` | boot:37 | override [sound:CSound::DEL_PORT] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::DEL_PORT (int)midi_state.port[port].resident_sequence |
| ps2/src/sound.cpp:444 | CSound::DEL_PORT | escapes cast | `(int)midi_state.port[midi_state.port[port].linked_port].resident_sequence` | boot:37 | override [sound:CSound::DEL_PORT] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::DEL_PORT (int)midi_state.port[midi_state.port[port].linked_port].resident_sequence |
| ps2/src/sound.cpp:469 | CSound::DEL_PORT | escapes cast | `(int)midi_state.port[dependent_port].resident_sequence` | boot:37 | override [sound:CSound::DEL_PORT] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::DEL_PORT (int)midi_state.port[dependent_port].resident_sequence |
| ps2/src/sound.cpp:494 | CSound::SQ_Play | escapes cast | `(int)sequence` | boot:52 | callee [ezmidi:ezMidi] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/sound.cpp CSound::SQ_Play (int)sequence |
| ps2/src/sound.cpp:726 | CSound::LoadHdBd2 | escapes cast | `(int)& gBank` | boot:38 | override [sound:CSound::LoadHdBd2] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::LoadHdBd2 (int)& gBank |
| ps2/src/sound.cpp:730 | CSound::LoadHdBd2 | escapes cast | `(int)& gBank` | boot:38 | override [sound:CSound::LoadHdBd2] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::LoadHdBd2 (int)& gBank #2 |
| ps2/src/sound.cpp:742 | CSound::LoadHdBd2 | escapes cast | `(int)midi_state.port[port].resident_sequence` | boot:38 | override [sound:CSound::LoadHdBd2] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::LoadHdBd2 (int)midi_state.port[port].resident_sequence |
| ps2/src/sound.cpp:767 | CSound::LoadHdBd2 | escapes cast | `(int)midi_state.port[midi_state.port[port].linked_port].resident_sequence` | boot:38 | override [sound:CSound::LoadHdBd2] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::LoadHdBd2 (int)midi_state.port[midi_state.port[port].linked_port].resident_sequence |
| ps2/src/sound.cpp:847 | CSound::LoadHdBdAdd | escapes cast | `(int)& gBank` | boot:38 | callee [ezmidi:ezMidi] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/sound.cpp CSound::LoadHdBdAdd (int)& gBank |
| ps2/src/sound.cpp:850 | CSound::LoadHdBdAdd | escapes cast | `(int)& gBank` | boot:38 | callee [ezmidi:ezMidi] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/sound.cpp CSound::LoadHdBdAdd (int)& gBank #2 |
| ps2/src/sound.cpp:886 | CSound::LoadSeq | escapes cast | `(int)sequence` | boot:38 | override [sound:CSound::LoadSeq] (T11) | guarded-unreached |  | ps2/src/sound.cpp CSound::LoadSeq (int)sequence |
| ps2/src/sound.cpp:932 | CSound::StreamOpenFast | escapes cast | `(int)file_name` | mode | callee [ezbgm:ezBgm] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/sound.cpp CSound::StreamOpenFast (int)file_name |
| ps2/src/sound.cpp:941 | CSound::StreamOpenFromFPLFast | escapes cast | `(int)& request` | mode | callee [ezbgm:ezBgm] (T11) | guarded-unreached | the truncated address only reaches the stubbed callee | ps2/src/sound.cpp CSound::StreamOpenFromFPLFast (int)& request |
| ps2/src/visualmotion.cpp:558 | mgCVisualMotionMDT::CreateFaceMotionPacket | escapes cast | `(u_int)packet` | boot:49 | override [visualmotion:mgCVisualMotionMDT::CreateFaceMotionPacket] (V3) | guarded-unreached |  | ps2/src/visualmotion.cpp mgCVisualMotionMDT::CreateFaceMotionPacket (u_int)packet |
| ps2/src/water.cpp:595 | CWater::Draw | resolved cast | `(int)data_memory->stAllocTest(0x3C)` | mode | override [water:CWater::Draw] (V4) | guarded-unreached |  | ps2/src/water.cpp CWater::Draw (int)data_memory->stAllocTest(0x3C) |
| ps2/src/water.cpp:735 | CWater::CreatePacket | escapes cast | `(u_int)memory->stGetTop()` | mode | override [water:CWater::CreatePacket] (V4) | guarded-unreached |  | ps2/src/water.cpp CWater::CreatePacket (u_int)memory->stGetTop() |
| ps2/src/water.cpp:797 | CWater::CreatePacket | escapes cast | `(u_int)strip` | mode | override [water:CWater::CreatePacket] (V4) | guarded-unreached |  | ps2/src/water.cpp CWater::CreatePacket (u_int)strip |
| ps2/src/automap.cpp:1489 | CAutoMapGen::CreatTermParts | escapes cast | `PortPtr32(cells)` | mode | guard | guarded-unreached |  | ps2/src/automap.cpp CAutoMapGen::CreatTermParts PortPtr32(cells) |
| ps2/src/editriver.cpp:393 | CEditGrid::GetFast | escapes cast | `PortPtr32(data)` | debug | guard | guarded-unreached |  | ps2/src/editriver.cpp CEditGrid::GetFast PortPtr32(data) |
| ps2/src/effscript.cpp:372 | CEffectScriptMan::LoadBaseEffSpt | escapes cast | `PortPtr32(load_buffer)` | mode | guard | guarded-unreached | file offsets are rebased onto the buffer as 32-bit addresses | ps2/src/effscript.cpp CEffectScriptMan::LoadBaseEffSpt PortPtr32(load_buffer) |
| ps2/src/effscript.cpp:384 | CEffectScriptMan::LoadBaseEffSpt | escapes cast | `PortPtr32(load_buffer)` | mode | guard | guarded-unreached | file offsets are rebased onto the buffer as 32-bit addresses | ps2/src/effscript.cpp CEffectScriptMan::LoadBaseEffSpt PortPtr32(load_buffer) #2 |
| ps2/src/event_func.cpp:16556 | _DIST_VECTOR | escapes cast | `PortPtr32(stack)` | mode | guard | guarded-unreached |  | ps2/src/event_func.cpp _DIST_VECTOR PortPtr32(stack) |
| ps2/src/event_func.cpp:16572 | _DIST_VECTOR2 | escapes cast | `PortPtr32(stack)` | mode | guard | guarded-unreached |  | ps2/src/event_func.cpp _DIST_VECTOR2 PortPtr32(stack) |
| ps2/src/event_func.cpp:17175 | _SWE_SET_COLOR | escapes cast | `PortPtr32(chara)` | mode | guard | guarded-unreached |  | ps2/src/event_func.cpp _SWE_SET_COLOR PortPtr32(chara) |
| ps2/src/event_func.cpp:17225 | _SWE_SET_TEXTURE | escapes cast | `PortPtr32(chara)` | mode | guard | guarded-unreached |  | ps2/src/event_func.cpp _SWE_SET_TEXTURE PortPtr32(chara) |
| ps2/src/event_func.cpp:17270 | _SWE_START_EFFECT | escapes cast | `PortPtr32(chara)` | mode | guard | guarded-unreached |  | ps2/src/event_func.cpp _SWE_START_EFFECT PortPtr32(chara) |
| ps2/src/menudraw.cpp:6397 | CRepairManager::Generate | escapes cast | `PortPtr32(this)` | menu | guard | guarded-unreached |  | ps2/src/menudraw.cpp CRepairManager::Generate PortPtr32(this) |
| ps2/src/mg_drawenv.cpp:401 | mgRENDER_INFO::GetPlight | escapes cast | `PortPtr32(GetpLightInfo())` | mode | guard | guarded-unreached |  | ps2/src/mg_drawenv.cpp mgRENDER_INFO::GetPlight PortPtr32(GetpLightInfo()) |
| ps2/src/mg_tanime.cpp:675 | mgCTextureAnime::NewTexAnimeGroupData | escapes cast | `PortPtr32(this)` | mode | guard | guarded-unreached |  | ps2/src/mg_tanime.cpp mgCTextureAnime::NewTexAnimeGroupData PortPtr32(this) |
| ps2/src/mg_visual.cpp:626 | mgCVisualFixMDT::DataAssignMDT | round-trip cast | `PortPtr32(&memory->stack[memory->stack_used])` | mode | guard | guarded-unreached | VU packet build; replaced with the renderer (V2) | ps2/src/mg_visual.cpp mgCVisualFixMDT::DataAssignMDT PortPtr32(&memory->stack[memory->stack_used]) |
| ps2/src/movie.cpp:186 | DmaAddr | escapes cast | `PortPtr32(addr)` | boot:61 | guard | guarded-unreached | movie decoder; replaced in T13e | ps2/src/movie.cpp DmaAddr PortPtr32(addr) |
| ps2/src/movie.cpp:190 | UncAddr | escapes cast | `PortPtr32(addr)` | boot:61 | guard | guarded-unreached | movie decoder; replaced in T13e | ps2/src/movie.cpp UncAddr PortPtr32(addr) |
| ps2/src/movie.cpp:785 | scTag2 | escapes cast | `PortPtr32(addr)` | boot:61 | guard | guarded-unreached | movie decoder; replaced in T13e | ps2/src/movie.cpp scTag2 PortPtr32(addr) |
| ps2/src/movie.cpp:1704 | audioDecBeginPut | escapes cast | `PortPtr32(dec->hdr)` | boot:61 | guard | guarded-unreached | movie decoder; replaced in T13e | ps2/src/movie.cpp audioDecBeginPut PortPtr32(dec->hdr) |
| ps2/src/runscript_opcodes.cpp:4481 | _SET_INDEXOBJ_SIZE | round-trip cast | `PortPtr32(objects)` | mode | guard | guarded-unreached |  | ps2/src/runscript_opcodes.cpp _SET_INDEXOBJ_SIZE PortPtr32(objects) |
| ps2/src/runscript_opcodes.cpp:4504 | _GET_INDEXOBJ_SIZE | round-trip cast | `PortPtr32(&monster->entry_object)` | mode | guard | guarded-unreached |  | ps2/src/runscript_opcodes.cpp _GET_INDEXOBJ_SIZE PortPtr32(&monster->entry_object) |
| ps2/src/sceneload.cpp:177 | LoadMapData | escapes cast | `PortPtr32(load_buf)` | mode | guard | guarded-unreached |  | ps2/src/sceneload.cpp LoadMapData PortPtr32(load_buf) |
| ps2/src/villagermngr.cpp:430 | CVillagerMngr::GetAppearVlgr | escapes cast | `PortPtr32(schedule)` | mode | guard | guarded-unreached |  | ps2/src/villagermngr.cpp CVillagerMngr::GetAppearVlgr PortPtr32(schedule) |
| ps2/src/villagermngr.cpp:470 | CVillagerMngr::GetAppearVlgr | escapes cast | `PortPtr32(schedule)` | mode | guard | guarded-unreached |  | ps2/src/villagermngr.cpp CVillagerMngr::GetAppearVlgr PortPtr32(schedule) #2 |
| ps2/src/visualmotion.cpp:246 | mgCVisualMotionMDT::DataAssignMotionMDT | escapes cast | `PortPtr32(memory->stGetTop())` | boot:49 | guard | guarded-unreached | VU packet build; replaced with the renderer (V2) before M6 | ps2/src/visualmotion.cpp mgCVisualMotionMDT::DataAssignMotionMDT PortPtr32(memory->stGetTop()) |
| ps2/src/vlgr_info.cpp:464 | niNPC_INFO_NUM | round-trip cast | `PortPtr32(new (niStack->Alloc(blocks + 2)) CVillagerInfo[count])` | mode | guard | guarded-unreached |  | ps2/src/vlgr_info.cpp niNPC_INFO_NUM PortPtr32(new (niStack->Alloc(blocks + 2)) CVillagerInfo[count]) |

## Allocations

53 rows: repaired 53.

| site | function | record | detail | kind | first_use | fix | state | note | key |
|---|---|---|---|---|---|---|---|---|---|
| ps2/src/character.cpp:2945 | _EFFECT | CHARA_EFFECT_MANAGER | PS2 0x20 blocks; host 1 × 0x210 bytes takes 0x23 | allocation | mode | PortBlocks | repaired |  | ps2/src/character.cpp _EFFECT PortBlocks<CHARA_EFFECT_MANAGER>(0x20) |
| ps2/src/character.cpp:3796 | CCharacter2::Copy | CSWordAfterEffect | PS2 0xC blocks; host 1 × 0xD0 bytes takes 0xF | allocation | mode | PortBlocks | repaired |  | ps2/src/character.cpp CCharacter2::Copy PortBlocks<CSWordAfterEffect>(0xC) |
| ps2/src/dngmenu.cpp:3947 | DngTreeMapInit | CMenuTreeMap | PS2 0x2FC0 blocks; host 1 × 0x2FD10 bytes takes 0x2FD3 | allocation | menu | PortBlocks | repaired |  | ps2/src/dngmenu.cpp DngTreeMapInit PortBlocks<CMenuTreeMap>(0x2FC0) |
| ps2/src/dngmenu.cpp:3949 | DngTreeMapInit | CDngFreeMap | PS2 0x13 blocks; host 1 × 0x150 bytes takes 0x17 | allocation | menu | PortBlocks | repaired |  | ps2/src/dngmenu.cpp DngTreeMapInit PortBlocks<CDngFreeMap>(0x13) |
| ps2/src/editeff.cpp:82 | EditSetEffectBuffer | CPaintEffect | PS2 0x42 blocks; host 1 × 0x440 bytes takes 0x46 | allocation | debug | PortBlocks | repaired |  | ps2/src/editeff.cpp EditSetEffectBuffer PortBlocks<CPaintEffect>(0x42) |
| ps2/src/editeff.cpp:554 | EditSetPlaceAnime | CMapParts | PS2 0x33 blocks; host 1 × 0x350 bytes takes 0x37 | allocation | debug | PortBlocks | repaired |  | ps2/src/editeff.cpp EditSetPlaceAnime PortBlocks<CMapParts>(0x33) |
| ps2/src/editexception.cpp:390 | InitFirePowder | mgC3DSprite | PS2 0x7 blocks; host 1 × 0x80 bytes takes 0xA | allocation | debug | PortBlocks | repaired |  | ps2/src/editexception.cpp InitFirePowder PortBlocks<mgC3DSprite>(0x7) |
| ps2/src/editexception.cpp:696 | InitGeyserEffect | CGeyserEffect | PS2 0x22 blocks; host 4 × 0xC0 bytes takes 0x32 | allocation | debug | PortBlocks | repaired |  | ps2/src/editexception.cpp InitGeyserEffect PortBlocks<CGeyserEffect>(0x22) |
| ps2/src/editmenu.cpp:1162 | MenuGeoramaInit | CMenuGeorama | PS2 0x1B92 blocks; host 1 × 0x1B990 bytes takes 0x1B9B | allocation | debug | PortBlocks | repaired |  | ps2/src/editmenu.cpp MenuGeoramaInit PortBlocks<CMenuGeorama>(0x1B92) |
| ps2/src/editmenu.cpp:5766 | MenuRemovalInit | CRemovalMenu | PS2 0x152 blocks; host 1 × 0x1C20 bytes takes 0x1C4 | allocation | debug | PortBlocks | repaired |  | ps2/src/editmenu.cpp MenuRemovalInit PortBlocks<CRemovalMenu>(0x152) |
| ps2/src/editmode.cpp:676 | LoadEditCursor | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | debug | PortBlocks | repaired |  | ps2/src/editmode.cpp LoadEditCursor PortBlocks<CCharacter2>(0x68) |
| ps2/src/editmode.cpp:695 | LoadEditCursor | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | debug | PortBlocks | repaired |  | ps2/src/editmode.cpp LoadEditCursor PortBlocks<CCharacter2>(0x68) #2 |
| ps2/src/editmode.cpp:710 | LoadEditCursor | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | debug | PortBlocks | repaired |  | ps2/src/editmode.cpp LoadEditCursor PortBlocks<CCharacter2>(0x68) #3 |
| ps2/src/effscript.cpp:560 | CEffectScriptMan::BuildBase | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/effscript.cpp CEffectScriptMan::BuildBase PortBlocks<CCharacter2>(0x68) |
| ps2/src/effscript.cpp:1294 | CEffectScriptMan::AssignCharacter | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/effscript.cpp CEffectScriptMan::AssignCharacter PortBlocks<CCharacter2>(0x68) |
| ps2/src/effscript.cpp:5564 | SetEffectScript | RS_CALLDATA | PS2 0x1 blocks; host 1 × 0x18 bytes takes 0x4 | allocation | mode | PortBlocks | repaired |  | ps2/src/effscript.cpp SetEffectScript PortBlocks<RS_CALLDATA>(0x1) |
| ps2/src/event_func.cpp:6080 | _COPY_CHARA | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/event_func.cpp _COPY_CHARA PortBlocks<CCharacter2>(0x68) |
| ps2/src/event_func.cpp:17140 | _SWE_INIT | CSWordAfterEffect | PS2 0xC blocks; host 1 × 0xD0 bytes takes 0xF | allocation | mode | PortBlocks | repaired |  | ps2/src/event_func.cpp _SWE_INIT PortBlocks<CSWordAfterEffect>(0xC) |
| ps2/src/fishing.cpp:1197 | sgRestartFishing | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp sgRestartFishing PortBlocks<CCharacter2>(0x68) |
| ps2/src/fishing.cpp:1420 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) |
| ps2/src/fishing.cpp:1422 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #2 |
| ps2/src/fishing.cpp:1424 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #3 |
| ps2/src/fishing.cpp:1426 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #4 |
| ps2/src/fishing.cpp:1428 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #5 |
| ps2/src/fishing.cpp:1430 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #6 |
| ps2/src/fishing.cpp:1432 | StepDataLoading | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/fishing.cpp StepDataLoading PortBlocks<CCharacter2>(0x68) #7 |
| ps2/src/funcpoint.cpp:495 | CFuncPointMngr::Add | CList<CFuncPoint> | PS2 0x20 blocks; host 1 × 0x210 bytes takes 0x23 | allocation | mode | PortBlocks | repaired |  | ps2/src/funcpoint.cpp CFuncPointMngr::Add PortBlocks<CList<CFuncPoint>>(0x20) |
| ps2/src/maintex.cpp:283 | SetSwordBlurEffect | CSWordAfterEffect | PS2 0xC blocks; host 1 × 0xD0 bytes takes 0xF | allocation | mode | PortBlocks | repaired |  | ps2/src/maintex.cpp SetSwordBlurEffect PortBlocks<CSWordAfterEffect>(0xC) |
| ps2/src/mapselect.cpp:441 | InitMapSelect | void * | PS2 0x22 blocks; host 128 × 0x8 bytes takes 0x42 | allocation | mode | PortBlocks | repaired |  | ps2/src/mapselect.cpp InitMapSelect PortBlocks<void *>(0x22) |
| ps2/src/mapselect.cpp:998 | LoadEventViewData | EVENT_VIEW_INFO | PS2 0x382 blocks; host 512 × 0x28 bytes takes 0x502 | allocation | mode | PortBlocks | repaired |  | ps2/src/mapselect.cpp LoadEventViewData PortBlocks<EVENT_VIEW_INFO>(0x382) |
| ps2/src/mdslist.cpp:245 | CMapPiece::Copy | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/mdslist.cpp CMapPiece::Copy PortBlocks<CCharacter2>(0x68) |
| ps2/src/mdslist.cpp:803 | CreateChara | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/mdslist.cpp CreateChara PortBlocks<CCharacter2>(0x68) |
| ps2/src/menuchr.cpp:1189 | CMenuChrCngMenu::LoadBGNPCModel | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menuchr.cpp CMenuChrCngMenu::LoadBGNPCModel PortBlocks<CActionChara>(0x105) |
| ps2/src/menuchr.cpp:6620 | CMenuCostumeSel::LoadMenuData | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menuchr.cpp CMenuCostumeSel::LoadMenuData PortBlocks<CActionChara>(0x105) |
| ps2/src/menuchr.cpp:7242 | MenuCostumeInit | CMenuCostumeSel | PS2 0x2F blocks; host 1 × 0x330 bytes takes 0x35 | allocation | menu | PortBlocks | repaired |  | ps2/src/menuchr.cpp MenuCostumeInit PortBlocks<CMenuCostumeSel>(0x2F) |
| ps2/src/menuchr.cpp:7828 | CMosBookMenu::KeyStep | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menuchr.cpp CMosBookMenu::KeyStep PortBlocks<CActionChara>(0x105) |
| ps2/src/menuchr.cpp:7876 | MonsterBookInit | CMosBookMenu | PS2 0x9A blocks; host 1 × 0x9D0 bytes takes 0x9F | allocation | menu | PortBlocks | repaired |  | ps2/src/menuchr.cpp MonsterBookInit PortBlocks<CMosBookMenu>(0x9A) |
| ps2/src/menudraw.cpp:6359 | CRepairManager::GeneratePoly | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menudraw.cpp CRepairManager::GeneratePoly PortBlocks<CActionChara>(0x105) |
| ps2/src/menushop.cpp:1868 | MenuShopInit | CShopMenu | PS2 0x23 blocks; host 1 × 0x258 bytes takes 0x28 | allocation | menu | PortBlocks | repaired |  | ps2/src/menushop.cpp MenuShopInit PortBlocks<CShopMenu>(0x23) |
| ps2/src/menusys.cpp:6955 | CMenuItemInfo::MenuModeMalloc | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menusys.cpp CMenuItemInfo::MenuModeMalloc PortBlocks<CActionChara>(0x105) |
| ps2/src/menusys.cpp:6965 | CMenuItemInfo::MenuModeMalloc | CActionChara | PS2 0x105 blocks; host 1 × 0x16A0 bytes takes 0x16C | allocation | menu | PortBlocks | repaired |  | ps2/src/menusys.cpp CMenuItemInfo::MenuModeMalloc PortBlocks<CActionChara>(0x105) #2 |
| ps2/src/menusys.cpp:6979 | CMenuItemInfo::MenuModeMalloc | CRepairManager | PS2 0x21 blocks; host 1 × 0x2F8 bytes takes 0x32 | allocation | menu | PortBlocks | repaired |  | ps2/src/menusys.cpp CMenuItemInfo::MenuModeMalloc PortBlocks<CRepairManager>(0x21) |
| ps2/src/mg_drawprim.cpp:757 | mgCDrawManager::AddPacket | mgSORT_PACKET | PS2 0x1 blocks; host 1 × 0x20 bytes takes 0x4 | allocation | mode | PortBlocks | repaired |  | ps2/src/mg_drawprim.cpp mgCDrawManager::AddPacket PortBlocks<mgSORT_PACKET>(0x1) |
| ps2/src/mg_tanime.cpp:1081 | texTEX_ANIME_DATA_END | mgCTextureAnime | PS2 0x21 blocks; host 1 × 0x308 bytes takes 0x33 | allocation | mode | PortBlocks | repaired |  | ps2/src/mg_tanime.cpp texTEX_ANIME_DATA_END PortBlocks<mgCTextureAnime>(0x21) |
| ps2/src/monster.cpp:1109 | CMonsterMan::LoadReferMonsterFile | CSWordAfterEffect | PS2 0xC blocks; host 1 × 0xD0 bytes takes 0xF | allocation | mode | PortBlocks | repaired |  | ps2/src/monster.cpp CMonsterMan::LoadReferMonsterFile PortBlocks<CSWordAfterEffect>(0xC) |
| ps2/src/movieviewlp.cpp:203 | MovieViewInit | CMovie | PS2 0x2396 blocks; host 1 × 0x23978 bytes takes 0x239A | allocation | debug | PortBlocks | repaired |  | ps2/src/movieviewlp.cpp MovieViewInit PortBlocks<CMovie>(0x2396) |
| ps2/src/movieviewlp.cpp:205 | MovieViewInit | MOVIE_LIST_ENTRY | PS2 0x32 blocks; host 64 × 0x18 bytes takes 0x62 | allocation | debug | PortBlocks | repaired |  | ps2/src/movieviewlp.cpp MovieViewInit PortBlocks<MOVIE_LIST_ENTRY>(0x32) |
| ps2/src/sceneload.cpp:187 | CScene::LoadChara | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/sceneload.cpp CScene::LoadChara PortBlocks<CCharacter2>(0x68) |
| ps2/src/sceneload.cpp:242 | CScene::CopyChara | CCharacter2 | PS2 0x68 blocks; host 1 × 0x9C0 bytes takes 0x9E | allocation | mode | PortBlocks | repaired |  | ps2/src/sceneload.cpp CScene::CopyChara PortBlocks<CCharacter2>(0x68) |
| ps2/src/sceneload.cpp:325 | CScene::LoadMapFromMemory | CEditMap | PS2 0x111 blocks; host 1 × 0x13D0 bytes takes 0x13F | allocation | boot:49 | PortBlocks | repaired |  | ps2/src/sceneload.cpp CScene::LoadMapFromMemory PortBlocks<CEditMap>(0x111) |
| ps2/src/sceneload.cpp:401 | CScene::LoadMapFromMemory | CMapSky | PS2 0x13 blocks; host 1 × 0x1C0 bytes takes 0x1E | allocation | boot:49 | PortBlocks | repaired |  | ps2/src/sceneload.cpp CScene::LoadMapFromMemory PortBlocks<CMapSky>(0x13) |
| ps2/src/title.cpp:661 | TitleInit | TITLE_INFO | PS2 0x1C blocks; host 1 × 0x210 bytes takes 0x23 | allocation | boot:44 | PortBlocks | repaired |  | ps2/src/title.cpp TitleInit PortBlocks<TITLE_INFO>(0x1C) |
| ps2/src/title.cpp:686 | TitleBootInit | CMemoryCardManager | PS2 0x112 blocks; host 1 × 0x1130 bytes takes 0x115 | allocation | boot:47 | PortBlocks | repaired |  | ps2/src/title.cpp TitleBootInit PortBlocks<CMemoryCardManager>(0x112) |

## Offsets

23 rows: guarded-unreached 7, n/a 5, repaired 11.

| site | function | record | expression | kind | first_use | fix | state | note | key |
|---|---|---|---|---|---|---|---|---|---|
| ps2/src/automap.cpp:1466 | CAutoMapGen::CreatTermParts | CAutoMapParts | `( u8 * ) cells + y_off` | byte offset | mode | - | n/a | offset computed from sizeof | ps2/src/automap.cpp CAutoMapGen::CreatTermParts ( u8 * ) cells + y_off |
| ps2/src/collision.cpp:488 | LoadCollisionFile | CColFrame | `( u_char * ) frames + offset` | byte offset | mode | - | n/a | offset computed from sizeof | ps2/src/collision.cpp LoadCollisionFile ( u_char * ) frames + offset |
| ps2/src/dng_debug.cpp:342 | DBGCMD_ReloadEnemy | CMonsterMan | `manager = ( u8 * ) ActiveMonster` | byte offset | debug | - | n/a | offsets computed from member addresses | ps2/src/dng_debug.cpp DBGCMD_ReloadEnemy manager = ( u8 * ) ActiveMonster |
| ps2/src/dng_event.cpp:2420 | LoadMonsterFile | CMonsterMan | `u8 * monster_man = ( u8 * ) ActiveMonster` | byte offset | mode | #ifdef PORT indexes memory[] | repaired |  | ps2/src/dng_event.cpp LoadMonsterFile u8 * monster_man = ( u8 * ) ActiveMonster |
| ps2/src/dng_object.cpp:622 | CRocketLauncherMan::Draw | CRocketLauncherMan | `( u8 * ) this + offset` | byte offset | mode | PortSize<CRocketLauncher> | repaired |  | ps2/src/dng_object.cpp CRocketLauncherMan::Draw ( u8 * ) this + offset |
| ps2/src/dng_object.cpp:646 | CRocketLauncherMan::Initialize | CRocketLauncherMan | `( u8 * ) this + offset` | byte offset | mode | PortSize<CRocketLauncher> | repaired |  | ps2/src/dng_object.cpp CRocketLauncherMan::Initialize ( u8 * ) this + offset |
| ps2/src/dng_object.cpp:1217 | CLaserGunMan::Draw | CLaserGunMan | `( u8 * ) this + offset` | byte offset | mode | PortSize<CLaserGun> | repaired |  | ps2/src/dng_object.cpp CLaserGunMan::Draw ( u8 * ) this + offset |
| ps2/src/dng_object.cpp:1241 | CLaserGunMan::Initialize | CLaserGunMan | `( u8 * ) this + offset` | byte offset | mode | PortSize<CLaserGun> | repaired |  | ps2/src/dng_object.cpp CLaserGunMan::Initialize ( u8 * ) this + offset |
| ps2/src/editmap.cpp:469 | CEditMap::GetePlaceIDList | CEditParts | `( s8 * ) edit_parts + offset` | byte offset | debug | PortLayoutGuard | guarded-unreached | reads byte 0x70 of each 0x330-byte CEditParts | ps2/src/editmap.cpp CEditMap::GetePlaceIDList ( s8 * ) edit_parts + offset |
| ps2/src/event_func.cpp:3991 | _DELETE_CHARA | CEohMother | `( u8 * ) & EventObjHandleMother + offset` | byte offset | mode | - | n/a | offset computed from sizeof | ps2/src/event_func.cpp _DELETE_CHARA ( u8 * ) & EventObjHandleMother + offset |
| ps2/src/event_func.cpp:12605 | _OBJS_SET_EOH_FRAME_POS | RS_STACKDATA | `( u8 * ) stack + vector_bytes` | byte offset | mode | PortSize<RS_STACKDATA[3]> | repaired |  | ps2/src/event_func.cpp _OBJS_SET_EOH_FRAME_POS ( u8 * ) stack + vector_bytes |
| ps2/src/event_func.cpp:20714 | _AMG_GET_ATTR_STATUS | RS_STACKDATA | `( u8 * ) stack + PORT_FIX ( PortSize < RS_STACKDATA [ 3 ] > ) ( 0x18 )` | byte offset | mode | PortSize<RS_STACKDATA[3]> | repaired |  | ps2/src/event_func.cpp _AMG_GET_ATTR_STATUS ( u8 * ) stack + PORT_FIX ( PortSize < RS_STACKDATA [ 3 ] > ) ( 0x18 ) |
| ps2/src/inventmn.cpp:1568 | CheckPhotoFlag | USER_PICTURE_INFO | `( u8 * ) photos + offset` | byte offset | menu | - | n/a | offset computed from sizeof | ps2/src/inventmn.cpp CheckPhotoFlag ( u8 * ) photos + offset |
| ps2/src/map.cpp:1304 | CMap::DrawWater | pointer | `( u8 * ) overlay_water -> parts + overlay_parts_offset` | byte offset | mode | PortSize<CMapParts *> | repaired |  | ps2/src/map.cpp CMap::DrawWater ( u8 * ) overlay_water -> parts + overlay_parts_offset |
| ps2/src/memcard.cpp:1087 | CMemoryCardManager::SaveToMc | SAVEDATA_FORMAT | `write_buffer = ( char * ) save_buffer` | byte offset | boot:56 | override [memcard:CMemoryCardManager::SaveToMc] (T10) | guarded-unreached | the override writes the disk image through port/src/records/save_image.hpp | ps2/src/memcard.cpp CMemoryCardManager::SaveToMc write_buffer = ( char * ) save_buffer |
| ps2/src/memcard.cpp:1231 | CMemoryCardManager::LoadFromMc | SAVEDATA_FORMAT | `read_buffer = ( char * ) save_buffer` | byte offset | boot:56 | override [memcard:CMemoryCardManager::LoadFromMc] (T10) | guarded-unreached | the override reads the disk image through port/src/records/save_image.hpp | ps2/src/memcard.cpp CMemoryCardManager::LoadFromMc read_buffer = ( char * ) save_buffer |
| ps2/src/menucommon.cpp:1026 | MenuCommonReadData | pointer | `( u8 * ) names + i` | byte offset | menu | PortSize<char *> | repaired |  | ps2/src/menucommon.cpp MenuCommonReadData ( u8 * ) names + i |
| ps2/src/menucommon.cpp:1027 | MenuCommonReadData | pointer | `( u8 * ) names + i` | byte offset | menu | PortSize<char *> | repaired |  | ps2/src/menucommon.cpp MenuCommonReadData ( u8 * ) names + i #2 |
| ps2/src/menuop.cpp:1617 | CMenuOption::UpdateOptionForm | pointer | `( u8 * ) loop_row + offset` | byte offset | menu | PortSize<MENUFORMPARTS_TYPE *> | repaired |  | ps2/src/menuop.cpp CMenuOption::UpdateOptionForm ( u8 * ) loop_row + offset |
| ps2/src/mg_drawprim.cpp:682 | mgCDrawManager::Draw | pointer | `( u_char * ) packet_list + offset` | byte offset | mode | override [mg_drawprim:mgCDrawManager::Draw] (T9) | guarded-unreached |  | ps2/src/mg_drawprim.cpp mgCDrawManager::Draw ( u_char * ) packet_list + offset |
| ps2/src/mg_drawprim.cpp:689 | mgCDrawManager::Draw | pointer | `( u_char * ) packet_list + offset` | byte offset | mode | override [mg_drawprim:mgCDrawManager::Draw] (T9) | guarded-unreached |  | ps2/src/mg_drawprim.cpp mgCDrawManager::Draw ( u_char * ) packet_list + offset #2 |
| ps2/src/villagermngr.cpp:437 | CVillagerMngr::GetAppearVlgr | pointer | `( u8 * ) places + output_position` | byte offset | mode | guard [villagermngr:430] | guarded-unreached | the PortPtr32 guard on schedule earlier in the loop stops first | ps2/src/villagermngr.cpp CVillagerMngr::GetAppearVlgr ( u8 * ) places + output_position |
| ps2/src/villagermngr.cpp:473 | CVillagerMngr::GetAppearVlgr | pointer | `( u8 * ) places + fallback_output_position` | byte offset | mode | guard [villagermngr:470] | guarded-unreached | the PortPtr32 guard on schedule earlier in the loop stops first | ps2/src/villagermngr.cpp CVillagerMngr::GetAppearVlgr ( u8 * ) places + fallback_output_position |
