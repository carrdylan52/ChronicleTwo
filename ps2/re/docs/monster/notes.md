# monster: reverse-engineering notes

`HitEffectSet` is active matching C++; `GuardEffectSet` retains a C++ draft
under `NONMATCHING` and uses retail `INCLUDE_ASM` in the matching build.
`CMonsterMan::ThinkHost` also has an active C++ definition.

## Monster hit and guard effects

Both functions return immediately when the scene has no active camera. They
copy the supplied point, normalize the vector from it toward the camera,
and offset the effect origin by 20 units along that vector. Their direction
vectors are quadword copies of `at_2031` and `at_2079__2`, respectively.
The hit pool has 0x60-byte `CHitEffectImage` entries, and the flash pool has
0x40-byte `CFlushEffect` entries. Each acquisition advances the corresponding
index, wrapping to zero when it reaches the pool count.

`HitEffectSet` emits a kind-0 burst with spread/speed/power/gravity
30/60/0.2/0.1, life 30 and count 32, then sets its texture rectangle to
(32,0,32,32). It acquires a flash independently: flag bit 2 selects
fade speed/size/growth 8/16/2, otherwise 10/15/1.5. Both variants have alpha
160 and a 128-by-128 texture region at (128,128). Finally, a second acquired
hit entry receives a kind-2 burst with 60/45/0/0, life 15 and count 16.
Both hit bursts and the flash are skipped when their acquired entries are null.

The first hit call matches when the local `spread` declaration precedes
`speed`; reversing those declarations changes four constant-load/move
instructions at +0xFC,+0x110,+0x118,+0x120. The matching function emits 0x334
bytes of instructions within retail's padded 0x340-byte extent. The production
unit's linked-image check passes with this function active and guard assembly
retained.

`GuardEffectSet` calls the acquired hit entry without a null check, using
50/30/0/0.1, life 30 and count 32, and sets kind 1. Its optional flash uses
fade speed/size/growth 16/10/3, alpha 160, and a 64-by-64 texture region at
(64,192). Nonzero `play_script` starts `at_2100` and passes the shifted origin
to script vector 1.

The current direct-literal guard draft emits 0x21C bytes within a padded
0x220-byte extent and differs in 8/136 words, all float argument setup:
+0xF0,+0xF4,+0xF8,+0x100,+0x108,+0x110,+0x118,+0x11C. Retail starts by
materializing gravity in v0 and spread in v1, then sets f15 and f14 before
finishing f12/f13. The draft starts spread/speed in v0/v1, then finishes
gravity/power. Control flow and all subsequent instructions match.
Direct literals, an outer gravity assignment, a gravity reference, a nested
call scope, entry-scope parameter initialization, and the single spread local
used by the actionchara sibling all give this same schedule. Earlier
five-local declaration permutations also failed isolated promotion; a
whole-draft zero score from those experiments did not establish an isolated
production match.

Guard remains parked until a natural MWCC expression form reproduces the
specific constant setup, or centrally integrated header/compiler work changes
that lowering. No shared header or compiler-note edit is required by the hit
match.

No first-game counterpart: Dark Cloud has no `CActiveMonster`, `CMonsterMan` or `CMonsterLocateInfo`.
`CActiveMonster` derives from `CActionChara` (actionchara.hpp, size 0x1030).

## Unresolved / for the next agent
- `CMonsterMan::CheckPhoto(CScene::InScreenCharaInfo *)` (0x1DE5E0, size 0x2CC) is NOT declared in
  `monster.hpp`: it needs the complete `CScene` (nested type), whose header `scenesnd.hpp` does not
  exist yet. Add, once it does (include "scenesnd.hpp"):
  `int CheckPhoto(CScene::InScreenCharaInfo *info);` -- fills info->[0] (s32) with the photo
  number / monster kind (`*tbl` id) and info->[4] (float), returns the scoop number or -1.
- `__as__12CActionCharaFRC12CActionChara` (0x1DB710) lives in this unit: it is the implicit
  `CActionChara::operator=` emitted here because `CActiveMonster::Copy` and `SetActiveMonster`
  assign `CActiveMonster`s. Do not declare it; it calls `CObject::operator=`.
- `SetActiveMonster` has two copies of a table entry into `param` (0x1094): the inlined implicit
  `CActiveMonster::operator=` block-copies it (0x17 doubleword loop), but the later
  `param = *tbl`-like copy is field by field with the char arrays copied two bytes per iteration.
  Unclear how the source spelt the second one.
- Fields named `unk_*` in `CActiveMonster`: 0x1208/0x120C/0x1210 (stored from the killing hit in
  CheckDamage: colprim damage-type `->unk8->unk18`, `->unk2C`, `->unk8->unk3C`; read by
  `_SET_DEAD_OFF`), 0x1300/0x1304 (float, Initialize 400.0/300.0), 0x1308 (s16, 180),
  0x1322 (from tbl+0x5A), 0x1328 (from tbl+0x56, drop amount in `_SET_DEAD_OFF`, x1.2 with hit
  attr 0x800), 0x132C (from tbl+0x58, amount of pull items dropped in `_SET_DEAD_START`),
  0x134C (SetActiveMonster's 4th arg; read by `CLockOnModel::Draw`), 0x1358 (set on death by
  certain kinds/attrs: kinds 0xB0-0xB4 with attr 0x40000, kind 0xDC with 0x80000; read by
  `_SET_DEAD_START`), 0x1488/0x148C (only zeroed/copied).
- MONSTER_ATTRIB bits 0x2 (tested by `_SET_DEAD_OFF`) and 0x80 (CheckDamage) unexplained.
- MONSTER_STATUS bits 0x8/0x20: grey palette flash, timer `grey_time` 300/900 steps; meaning not
  named. Status set from colprim attr bits 0x8 / 0x8000 / 0x10000 unless tbl->resist_attr has them.
- `CMonsterLocateInfo` arrays: the two s16[32] arrays at CMonsterMan+0x10000/+0x10040 are placed
  inside CMonsterLocateInfo (making it 0x8C); they could equally be CMonsterMan members.
  SetPutFlag only touches +0/+4/+8.
- MONSTER_REFER, MONSTER_SCOOP, MONSTER_STATUS, MONSTER_MAN_SIZE and all enums are not retail
  names.

## CActiveMonster (size 0x14A0)
Size: `InitDungeonMain` does `__construct_new_array(..., __ct__14CActiveMonsterFv, 0, 0x14A0, 0x18)`
(the 24 scene characters 0x18..0x2F) and Initialize writes up to 0x1497.
Ctor (dng_main 0x1CF640): CActionChara ctor inlined, vtable store, CRunScript ctor at 0x1040,
memset(0x1360, 0, 0x110) = inlined `MoveCheckInfo::Initialize`. In InitDungeonMain's inlined copy
it calls `MoveCheckInfo::Initialize` out of line, hence `mons_move_check.Initialize()` in the body.

Member groups were found from the inlined implicit operator= in SetActiveMonster/Copy: scalars are
copied one by one, arrays/structs as 16-byte groups. That fixes 0x12A8..0x12C0 as one 0x18 struct
(16+8 copy) and 0x133C..0x1348 as one 0xC struct (0x1340/0x1344 copied as words).

| off | field | evidence |
|---|---|---|
| 0x1030 | place_pos vec | SetActiveMonster copies pos; _GET/_SET_PLACE_POS |
| 0x1040 | mons_script CRunScript | ctor; RunScript (CMonsterMan) resume/run; +0x3C (0x107C) checked |
| 0x1094 | param BASE_MONSTER_TBL | 0xB8 copy; `tbl` points here for placed monsters |
| 0x114C | base_tbl | = table entry; _GET_BOSS_FLAG, SET_*_RATE read originals |
| 0x1150 | tbl | refer chara: table entry (LoadReferMonsterFile); placed: &param |
| 0x1154 | refer_no | = refer slot; texture block 0x28+refer_no in Draw*; _GET_MONSTER_ID |
| 0x1156 | monster_id | = refer id; _GET_MONSTER_INDEX; 0x25A excluded from CheckThrowTarget |
| 0x1158/0x115A | req_prog / now_prog | RunScript: -1 resume, else run and copy to 0x115A |
| 0x115C | var[8] | _V_PUSH/_V_POP index < 8 |
| 0x117C | var2[32] | _V_PUSH2/_V_POP2 index < 0x20 |
| 0x11FC/0x1200/0x1204 | link_parts/link_piece/link_type | _LINK_MAP_TO_OBJECT (1), _LINK_OBJECT_TO_PIECE (2, SearchPiece), MoveUnit |
| 0x1214 | last_hit_attr | colprim +0xA0 attr of the killing hit; bits 1/2/0x800 in _SET_DEAD_* |
| 0x1220 | life_gage CEnemyLifeGage | Initialize(0); DrawLifeGage |
| 0x1270 | piyori CPiyori | SetActiveMonster CPiyori::Initialize; _SET_PIYORI_MARK |
| 0x1290 | gift_mark CGiftMark | CGiftMark::Initialize/Set/Step |
| 0x12A4 | att_type s16 | CheckDamage (s8 of hit); _GET_ATT_TYPE |
| 0x12A8 | scoop | _SET_SCOOP (1 arg: type 2 + no; 4 args: type 1, motion, start, end, no); ThinkHost sets ok (0x12BC); CheckPhoto reads ok/no |
| 0x12C0/0x12C8 | reserv_img[2] / sizes | _LOAD_RESERV_IMG, _TRANS_RESERV_IMG |
| 0x12D0 | center_pos | ThinkHost GetEntryObjectPos(0,0); effect positions in CheckDamage |
| 0x12E0 | event_no | _RUN_EVENT_SCRIPT; IsRunEvent takes and clears |
| 0x12E2 | target_no | _GET_TARGET_* GetCharacter |
| 0x12E4/0x12E8 | view_state / view_alpha | CheckView, ThinkHost fade +-0.05 |
| 0x12EC | camera_alpha | ThinkHost: fades out while camera_dist < 4*radius(0x10C) |
| 0x12F0 | priority | PriorityLevelCheck rank by target_dist; GetPriorityLevelIndex |
| 0x12F4/0x12F8 | target_dist / camera_dist | ThinkHost |
| 0x12FC | clip_dist | _SET_CLIP_DIST (x20); CheckView, DrawLifeGage |
| 0x130C | height | MoveUnit pos.y - ground; _GET_HIGH |
| 0x1310/0x1314 | max_life / life | tbl->life; _GET/_SET_*LIFE* |
| 0x1318 | attack u16 | tbl->attack, x1.3 on rage; colprim +0x88; _GET_BASE_ATTACK |
| 0x131A/0x131C/0x1320 | gekirin_num / gekirin / gekirin_time | tbl+0x60; rage at 0 sets time 900; ResetGekirin |
| 0x1324 | whp u16 | fptoui(tbl+0x5C); calcWeaponParamWhp |
| 0x1326 | defense u16 | tbl+0x68; CheckDamage `colprim->unk88 - defense`; _SET_DEF_RATE |
| 0x1330 | state | 0 free (SearchActiveMonsterBlock), 1 live, 3 dead (_SET_DEAD_START) |
| 0x1334 | dead_alpha | 0x80 at _SET_DEAD_OFF, ThinkHost -3/step, alpha(0x100) = /128 |
| 0x1338/0x133A | piyori_mark / piyori_time | _SET_PIYORI_MARK copies 0x133A->0x1338; SetNearAreaPiyori 120 |
| 0x133C | status MONSTER_STATUS | CheckStatusAttr, CheckDamage, _GET_STS_ATTR |
| 0x1348 | attrib | _SET_ATTRIB; MONSTER_ATTRIB |
| 0x1350 | locate_param | AutoSetMonster from locate.param; CheckMonsterTolk != -1; IsEventRun |
| 0x1354 | gate_key | AutoSetMonster GetGateKeyIndex; ThinkHost drops CPullItem with it |
| 0x1356 | no_damage_cnt | CheckDamage ++; _GET_NO_DAMAGE_CNT |
| 0x1360 | mons_move_check MoveCheckInfo | MoveUnit MoveCheck; radius = 0x10C (x2.5+5 when caught) |
| 0x1470 | next_pos | _SET_NEXT_POS, _CHK_MOVE_END |
| 0x1480/0x1484 | move_speed / arrive_dist | _SET_NEXT_POS (default 20.0), _RESET_MOVE |
| 0x1490/0x1494 | next_rot / rot_speed | _SET_NEXT_ROT, _SET_ROT, MoveUnit mgAngleInterpolate |

### Vtable (__vt__14CActiveMonster, 0x124 bytes)
Same as CActionChara's except: slot 0x3C `Initialize__14CActiveMonsterFv` (overrides
CCharacter2::Initialize()), 0xD4 `Step__14CActiveMonsterFv` (tail-calls CActionChara::Step), and a
new last slot 0x120 `Copy__14CActiveMonsterFR14CActiveMonsterP9mgCMemory`.
Copy = implicit `dest = *this` (operator=, calls `CActionChara::operator=`) then
`CActionChara::Copy(dest, memory)`.

## BASE_MONSTER_TBL (0xB8)
Stride 0x5C shorts in GetMonsterTable/GetReferPtr2; end when name[0] == 0.
`base_monster_define` is 0xF740 = 344 entries (global). Field evidence: id +0 (GetMonsterTable);
grade +2 (_GET_MONS_GRADE); name +4 char[32] (_MONSTER_NAME strlen < 0x20, GetMonsterName);
model +0x24 ("%s.cfg", "dungeon/monster/%s.chr", GetMonsterModelFile); script +0x34 (".stb");
gift_type +0x44 (CheckGiftPack); sound_no +0x48 (EN_%d.snd); life +0x50; user_mons_id +0x54
(s8, compared with battle-chara-info +2; ==9 special); +0x56/+0x58/+0x5A copied to 0x1328/0x132C/0x1322;
whp float +0x5C; gekirin_num +0x60; guard_rate +0x62; escape_rate +0x63/+0x64 (ThinkHost:
progs 800/900 by target's attack kind 0/1); attack +0x66; defense u8 +0x68; stagger +0x69
(compared with CActionChara::stagger 0xBF4); boss +0x6A; sw_effect_num +0x6B
(LoadReferMonsterFile CSWordAfterEffect loop into chara+0x570); ext_param s16[12] +0x7C
(_SET_EXT_PARAM_RATE, capped 100; damage `* ext_param[vs_attk_index[type]] / 100`);
+0x94 bit 1 = no stagger reaction, bit 4 = no knock reaction (CheckDamage); +0x98 bit 1 = cannot
be lifted (CheckThrowTarget prints dung_progtxt_notlift_mons); next_id +0x9C (EntryRefer loop);
drop_item +0xA0/+0xA2 (CheckDamage, _SET_DEAD_START); resist_attr +0xA8; +0xAE percentage in
damage formula; +0xB4: 0 -> SetActiveMonster uses virtual Copy with model, else operator=.

## CMonsterMan (0x100F0)
Size: `__nw__FUiP1(0x100F0, ...)` in InitDungeonMain (global `ActiveMonster`, dng_main, is a
`CMonsterMan *` despite the name). Inlined implicit ctor there: mgCMemory::Init for 24 x 0x30 at
+4, CActiveMonster ctor at +0x500 stride 0x14C0 x12.
- 0x0 scene; 0x4 memory[24] (SetMonsterScript stack per slot; SetActiveMonster zeroes +0x20/+0x28)
- 0x484 active[24] = scene chara 0x18+i (Initialize)
- 0x4E4..0x4F0 alignment padding
- 0x4F0 refer[12]: +0 id (-1 free), +0x10 chara, +0x14B0 script (stAlloc64 copy of .stb)
- 0xFDF0 share_var[128]: _V_PUSH/_V_POP index 8..0x87 at ActiveMonster+0xFDD0+4*i
- 0xFFF0 effect_man = FxScriptMan
- 0xFFF4 locate: num (_FLS resets, _FL appends), put_num, put_flag; +0xC monster_id[32],
  +0x4C param[32] (Initialize -1; _FL; AutoSetMonster; AutoSetTreasureBox for mimic ids 0xF5..0x10C)
- 0x10080 priority_limit (=5; _SET_PRIORITY_LIMMIT; CheckView arg; DrawShadowActMonster)
- 0x10090 boss_life_gage; 0x100E0 boss_max_life (+= max_life for boss in SetActiveMonster)

Texture blocks: refer slot i uses texture block 0x28+i (Initialize DeleteBlock, ReloadTexture in
DrawActMonster/DrawInvisibleMonster).

## Return types chosen
IsDraw int, CheckView int (returns lh of view_state), IsBattleStyleDist float, CheckMonsterTolk int
(param unused), CheckThrowTarget CActiveMonster*, GetReferPtr2/GetMonsterTable BASE_MONSTER_TBL*,
SetActiveMonster/GetPriorityLevelIndex CActiveMonster*, IsRunEvent int (s16 load), EntryRefer and
LoadReferMonsterFile int, SearchArea float, LoadMonsterLanguage void.

## Globals and local data
- Global: `base_monster_define` (extern in header), `__vt__14CActiveMonster`.
- Local (static in .cpp): dung_progtxt_notlift_mons (char*[7], per LanguageCode),
  no_score_uv / guard_score_uv (s32[7][4] = u, v, w, h per language, HitScoreSet),
  vs_attk_index (s16[26], attack type -> ext_param index), gift_item_tbl (s16 rows of 3:
  gift_type, item, ?; ends -1; symbol 0x58), react_tbl (s16 rows of 4 / 8 bytes, key = colprim
  damage type, ends 0xFFFF), mos_data_anlyze_tag (SPI_TAG_PARAM[2]: tag string at_2802 ->
  _MONSTER_NAME, then a NULL terminator; used by LoadMonsterLanguage on mosdata%d.cfg), function statics dmg_sc_cnt_2104 / init_2105 (HitScoreSet).
- Local functions (static, not in header): HitEffectSet, GuardEffectSet, HitScoreSet,
  CheckGiftPack, _MONSTER_NAME.
## Compiler flag cleanup

The local `divbyzerocheck on`/`reset` pair is redundant with the PS2
compiler flag. Removing it leaves every section and symbol in this unit's
object diff unchanged.
