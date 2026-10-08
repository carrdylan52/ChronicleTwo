# menuaqua: reverse-engineering notes

`CAquarium::Draw` draws the fish, aquarium frames, bubbles, water reflection,
and menu overlays in retail order. Its C++ body remains a guarded draft.
`CAquarium::SettingAqua` constructs its `love_chara` member as a `CCharacter2`
and remains guarded, as do `CAquaFish::SetAdjustScale`, `DrawFishParam`,
`CAquarium::ColCheck`, `CAquarium::Step`, and `GyoraceMenuDraw`.

`GyoraceMenuKey` is active C++. Its existing draft is instruction-identical;
removing its guard passes the complete-unit linked-image comparison, PAL
verification, and all 149 complete-object comparisons. Its saved-race menu
handles loading, fish selection, tactics, name registration, and save prompts.
The retail switch labels split its objdiff function row, so coverage requires
the generated matching-assembly inventory to be refreshed with the supported
`disassemble` target after promotion. m2c currently cannot resolve that switch's
jump table; the retail assembly and existing types remain the analysis evidence.

## Remaining matching blockers

The following scores compare the complete C++ drafts with retail, including
zero-padding to each manifest extent. A shorter compiled body alone is not a
mismatch when the remaining retail words are padding.

- `SetAdjustScale__9CAquaFishFv`: **6/36 words**, compiled **0x8C**, retail extent
  **0x90**. All differences are float argument setup: retail materializes
  `0.95f` into `f13` before `0.6f` into `f12`; the draft reverses that order.
  Explicit float construction and named argument locals retain the compiler's
  ordering. Reconsider with a documented natural MWCC expression/scheduling
  explanation for that order, rather than repeating those forms.
- `SettingAqua__9CAquariumFv`: **2/752 words**, compiled **0xBB4**, retail extent
  **0xBC0**. At **+0xA00/+0xA04**, placement new for `CCharacter2` branches on
  `v0` and copies it to `s3` in the retail delay slot. MWCC instead copies first
  and branches on `s3`. Parked under the placement-new stop rule; reconsider
  when the dedicated constructor/null-branch lane supplies a natural solution.
- `Draw__9CAquariumFv`: **6/928 words**, compiled **0xE78**, retail extent
  **0xE80**. At **+0xD0C..+0xD28**, the debug panel's float arguments load in
  the wrong order: retail width `120`, height `242`, top `80`; draft top,
  width, height. Sharing the existing integer text `y = 0x50` with the panel
  leaves the same six-word difference. Reconsider with a supported float
  argument scheduling correction that also passes the game-object comparison.
- `GyoraceMenuDraw__Fv`: **0/624 words in the all-draft build**, but activating
  it produces **four differing bytes** in instructions at
  **0x0021E310..0x0021E324** (**+0x8D0..+0x8E4**). The game build reverses the
  final cursor call's `4.0f`/`2.0f` setup. Matching the earlier scroll call's
  integral snap-range conversion, or using explicit double-to-float literal
  conversions, leaves the game-build difference unchanged. Retain the guard.
  Reconsider when the all-draft/game-build translation-unit scheduling
  difference is understood or changed by another naturally matched body.
- `ColCheck__9CAquariumFi`: **60/448 words**, compiled **0x6F8**, retail extent
  **0x700**. The differing instructions exchange `s2` and `s3`: retail holds
  the selected fish in `s3` and uses `s2` for the fish-loop offset and obstacle
  count; draft allocates those registers oppositely. All other instructions
  and relocations agree. Giving each loop its own index produces **76**
  differing words; initializing the selected-fish pointer before the
  temporary declarations produces **89**. Reconsider with evidence for a
  natural local lifetime or type correction that yields the retail allocation.
- `DrawFishParam__FiiP10mgCTextureP13CGameDataUsed`: **652/704 words**, compiled
  **0xAA4**, retail extent **0xB00**. Retail uses a **0x450** stack frame;
  draft uses **0x430**. Fresh rectangle temporaries for the three unknown-weight
  glyphs restore the frame size but still leave **651** differing words and
  the same body length. Retail also preloads six dimension-table bytes before
  copying the width/height initializer templates; draft interleaves template
  copies and loads. Separate zero initialization and entry assignments leave
  **669** differing words with **0xAD4** compiled bytes. Reconsider with the
  original initializer/lifetime structure established from the retail loads,
  stores, and rectangle stack slots. Both probes are reverted.
- `Step__9CAquariumFv`: **1111/1732 words**, compiled **0x1B04**, retail extent
  **0x1B10**. Before **+0x660**, instruction differences are downstream branch
  targets. At **+0x664**, the draft schedules the tank-index shift into a
  branch delay slot where retail has a nop, then loads `menu_cursor` after
  the table-address work; retail loads it first. That one-word contraction
  shifts the following menu cases and inflates the aligned-word difference.
  Its out-of-line `CFishFood` constructor preserves the retail placement-new
  branch/copy-delay pattern and is not the inline constructor blocker above.
  m2c currently cannot resolve the menu switch jump table. Reconsider with
  jump-table recovery and a natural evaluation-order/type explanation for
  the menu-id lookup, then realign the comparison before chasing later blocks.

Aquarium menu (fish swim, eat food, fight, pair/breed), the gyorace (fish race) fish-select and
saved-race menus, fish race/fishing tournament prize scripts, and shared sub-game panel drawing.
No first-game counterpart (Dark Cloud has no aquarium); layouts below come from this game only.

## Class sizes (all asserted except CGyoraceFishData)
- CBubble 0x40: `__nw(0x40)` after `Alloc(6)` in `CAquarium::Initialize`/`SettingAqua`; battle
  bubbles indexed `AquaBattleBubble + n*0x40` in `CAquarium::Step`.
- AQUA_BUBBLE 0x30 (name invented): `Initialize` allocs `num*3` quadwords; stride 0x30 in
  Generate/Step/Draw.
- CAquaFishActionParam 0x40: `Initialize` is `memset(this,0,0x40)`; next field of CAquaFish at 0x700.
- CAquaFish 0x940: `__nw(0x940)` in `LoadFish` (Alloc 0x96 quadwords).
- CAquaFishEff 0x10: `__nw(0x10)` in `CAquarium::Initialize`.
- CFishFood 0x6A0: `__nw(0x6a0)` in `CAquarium::Step` (Alloc 0x6c).
- CAquaMes 0x64: member of CAquarium at 0xFC, next member (mgCMemory) at 0x160.
- CAquarium 0x3D0: `Aquarium` bss symbol extent; 16-aligned by `drop_pos`.
- NEXT_THINK_PARAM 0x20: stack copy `aNStack_80` in SettingAqua / `uStack_110` in Thinking; fields
  at 0x00 (vec), 0x10 (CAquaFishEff*), 0x14 (target CAquaFish*), 0x18 (s16 target no); 16-aligned.
- CGyoraceFishData: LoadData clears only 0x00..0x17 (s16[4] + ptr[4]); gyorace keeps it in a
  0x20 stack slot. Size 0x18 likely, not asserted.

## Vtables
`__vt__9CAquaFish` / `__vt__9CFishFood` (0xF8, in `.vtables` of this unit) equal
`__vt__11CCharacter2` except: CAquaFish slot 0x3C = `Initialize__9CAquaFishFv`; CFishFood slot
0xD4 = `Step__9CFishFoodFv`. No new virtuals. Both ctors are the inline CCharacter2 ctor chain
(mgCObject -> CObject -> CObjectFrame -> CCharacter2, shadow_link fields at 0x35C..0x364).
- CAquaFish ctor: after its vtable store, `action.Initialize()` three times, `data = NULL`, then
  virtual `Initialize()`. LoadFish also calls `action.Initialize()` three times in a row (some
  inline helper repeats it; unresolved).
- CFishFood ctor calls `CCharacter2::Initialize` non-virtually, then zeroes its own fields (pos.w=1).
- `love_chara` (CAquarium 0x390) is a plain `new CCharacter2` (inline ctor, Alloc 0x68, size 0x660)
  loaded from `menu/eff/haigou_love.chr`.

## CAquaFish (0x660..0x940)
- 0x660 target_pos, 0x670 move (added to position by ColCheck), 0x680 target_rot (x pitch,
  y yaw; LoadFish sets it from the initial rotation), 0x690 `turn`: zeroed as a vector by
  `mgZeroVector`; x = pitch step (passed to `mgAngleInterpolate` in ColCheck), y = yaw divisor
  (`PI / y`). z/w never used.
- 0x6A0 aqua_no (slot; indexes AquaFishEff/AquaFishBubble), 0x6A4 radius (`base scale*adj/0.6`
  in SetAdjustScale; *0.29/*0.26 in ColCheck), 0x6A8 think_timer, 0x6AC pair_no, 0x6AE
  think_mode (switch in Thinking, written by NextThink), 0x6B0 swim_mode, 0x6B2..0x6BF unused.
- 0x6C0 action: +0 phase (battle: 0 approach,1 start,2 swing; battle-rest: 3 go, 4 rest),
  +4 timer, +8 target_no, +0xC target (vtable call 0x18 = GetPosition), +0x10 speed / +0x14
  max_speed (round), +0x30 decel (0.8 rest, 0.7 food look), +0x34 hit_count (ColCheck adds 1, or
  6 at 8%; Thinking triggers pairing when > 0xA0 in mode 8; ParamStep raises a param when > 0xB4
  in mode 5).
- 0x700 union: round {s16 dir (0x700), float 0x704, 0x708, 0x70C} vs battle float 0x700
  (MoveActionBattle swing angle). Proven by short and float accesses at 0x700.
- 0x710 route[32] (0x710 + 32*0x10 = 0x910), 0x910 route_num (16..25), 0x912 route_no, 0x914
  route_time (>0x14 skips a point).
- 0x920 eat_item (food item no copied from food+0x680; 0x13B and 0x168 are special in
  ParamStep/ColCheck), 0x922 s8 (random 0..7 at init, reset to 0 when >7; meaning unknown),
  0x924 col_flags (cleared each ColCheck), 0x928 wall_time (round: >200 forces a turn),
  0x92C fatigue, 0x930 fatigue_max = `(data[+0x3C]/10 + rand(20) + 26) * 20`, 0x934 flash_count,
  0x938 data (CGameDataUsed*; BREEDFISH_USED is `data + 0x10`).
- ParamStep return bits: 2 = fish died (HP <= 0; 0x88 bubbles emitted), 8 = fish had flag 0x80,
  0x10/0x20 = special food 0x13B toggled sex byte (+0x15 of the breed data). ColCheck return:
  bit 1 = ate food 0x168, bit 4 = battle hit.
- Think modes (enum AQUA_FISH_THINK): seen in NextThink switch and Thinking switch. Mode 2 has no
  NextThink case and does nothing in Thinking. NextThink(1) always ends with swim_mode 2 (it
  writes 0 then 2), so the swim_mode 0 and 1 branches are reached only via route/think code.
- Motion name addresses used with SetMotion (vtable 0xB0): 0x36F140, 0x36F150; SetStep is 0xB8.

## CAquaFishEff
0 fish (vtable 0x18 GetPosition in Draw), 4 texture (`Tex_FishEffect`), 8 s16 type 1..5 (texture
rows in Draw; `max_tbl_1484` gives each type's time), 0xC timer (-1 = forever).

## CFishFood (0x660..0x6A0)
0x660 spin (x,z used), 0x670 pos, 0x680 item_no (s16), 0x684 fall_time, 0x688 sway, 0x68C
sway_phase, 0x690 state (FISH_FOOD_STATE: Thinking reads it via `piVar2[0x1a4]` byte; 2 makes
fish decide 3 vs 4, 3 lets ColCheck feed). 48.0 is the water surface; 19.6 the floor.

## CAquaMes
Field order from Initialize (clears) and the seven `new ClsMes` (0x2958 bytes, Alloc 0x298):
4 title, 0x10 menu, 0x38 guide, 0x2C question, 0x44 help, 0x4C info, 0x54 fish window.
0x38/0x3C: `CAquarium::Step` writes `mes.guide_id` (offset 0x138) then `MakeMesWin(guide_mes,
guide_id)`. 0x5C/0x60 set to -1 in Initialize, never read. Cursor: Step eases 0x24 toward 0x1C by
1/3 unless 0x19 set. ClsMes offsets used: 0x19C/0x1A0 position, 0x1E3C, 0x1E59 (name string
buffer, 0x32 per line), 0x225C/0x2278/0x228C (cursor), 0x258C/0x2590 widths.

## CAquarium
- 0x00 mode (Step switch); 0x04 load buffer (`stack + used*16` of the Initialize memory).
- 0x08/0x70/0xC8/0x160/0x194+n*0x30/0x394 are mgCMemory (ctor calls `Init` on each; Clear and
  others write their `lock` (+0x1C) and `stack_used` (+0x24) directly).
- 0x38 tex_block[13]: copied from the `int*` argument (12 entries) and terminated with -1;
  `MenuDeleteTextureBlock(tex_block)`. Slots handed out: [0]->0xB4, [1]->0xB0/0xB2/0xBC,
  [2]->0x190, [3]->0x324, [4]->0x386, [5..10]->0x2CC..0x2D6. `Aquarium_NameregistBlock =
  &tex_block[5]`.
- 0x6C user_data = `GetSaveData()+0x1D2A0` (passed as CUserDataManager* in Step);
  `m_aquarium_para` = that + 0x4958 (a CFishAquarium*; first s16 is the aquarium number 0..2).
- Pack files (`menu/aqua/pack/aqua%d.pak`): aqua.img->0xB4, aqua.mds->0xA8, ground.img->0xB0,
  ground.mds->0xA0, aqua_glass.img->0xB2, aqua_glass.mds->0xA4, water.img/water_ref.img->0xBC,
  aqua_mizu.mds->0xAC, suimen.mds->0xC0; `CreateWaterFrame` -> 0xB8; aqua_naka.mds -> 0xF8 (only
  aquarium 0, into 0xC8 memory).
- 0xC4 ripple: reset to 0.1, Draw counts it down by 0.01 and passes it to the water frame; Step
  sets 0.18 when food state is 2.
- 0x2D8 sel_fish, 0x2DA/0x2FA names (strcpy targets, 0x20 apart), 0x31A/0x31C fish slots for
  pairing/special food/messages, 0x31E draw flag for the selected fish's data.
- 0x320 food, 0x330 drop_pos (initialised (0,61,0,1)), 0x340 food_time (0xFA when dropped),
  0x384 drop-line draw flag (DrawEsaDropRoot).
- 0x388 love_phase (Step state machine: 1,2 combine, 3/0xB/0xC messages, 5 start, 10), 0x38A
  counter, 0x38C tex block (= tex_block slot 0x2D0 copy), 0x390 love_chara.
- Never accessed: 0x326 (only cleared), 0x328..0x32F (padding before drop_pos), 0x344..0x383,
  0x386 (only assigned), 0x3C4..0x3CF.

## Globals
- Header externs (global in retail): `Mitouroku` char*[7] (indexed by LanguageCode),
  `AquaDeadCheck` int, `GyoraceFish` CGameDataUsed* (aquarium fish top + n*0x6C), `GyoraceData`
  CGyoRaceData* (`CSubGameData::GetGyoRaceData(SubSaveData)`), `MenuDCMsg` CDC2Mes*[9] (0x24
  extent; inventmn indexes it; `MenuDCMsg[1]` is `DAT_01efba44`).
- Everything else in the unit is LOCAL in retail -> `static` in the .cpp. Notable types:
  `Aquarium` CAquarium; `AquaBubble` CBubble*[3]; `AquaFishBubble` CBubble*[6]; `AquaFishEff`
  CAquaFishEff*[6]; `AquaBattleBubble` CBubble* (array of battle emitters, stride 0x40);
  `AquaBattleBubble_Pos` float[4]; `aquarium_xz_table`/`aquarium_y_table` new float[0x3C][4];
  `aquarium_paul_table` new {float* xz; float* y}[0x3C] (60 grid points, 10 x 6);
  `Camera__2` mgCCameraFollow* (0xC0, Alloc 0xE); `aqua_old_env` float[4][4]* (12 quadwords:
  light matrices + point light + enable); `Tex_Aqualium`/`Tex_FishEffect` mgCTexture*;
  `m_aquarium_para` CFishAquarium*; `AquaScene` CScene*.
- Data tables (all local): `esa_info` rows of 10 bytes {s16 item; s8 +2 -> breed+0x3B counter;
  s8 +3 -> +0x2C; +4 -> +0x26; +5 -> +0x28; +6 -> +0x2A; s8 +7 unused; s16 +8 -> +0x30}
  (offsets relative to BREEDFISH_USED; ParamStep adds them), terminated by item <= 0; 10 rows.
  `aquafish_info` rows of 0xC {s16 item; char* name at +4; s8 colour at +8 and +9}, 19 rows.
  `aquafish_mixTable` s8[171][3] (parent1-0x136, parent2-0x136, child-0x136).
  `ColChkPoint`/`ColChkPoint2` 9 rows, `ColChkPoint3` 6 rows of 0x20 {float pos[4]; float
  radius; 12 bytes}; `ColChkPointNum` u8[3]. `aqua_bubble_generate_pos` float[3][3][4].
  `GyoracerIndexNo`/`GyoracerTacticsNo` s16[6]. `fish_save_present` FISH_PRIZE_INFO[4][3].
  Prize script data: `FishTournamentGoods` groups of 0x44 {int num; int [8] from script; 7 unused
  ints; ptr at 0x40 to num entries of 0x1C = {int; FISH_PRIZE_INFO[3]}}.
- No row-type structs were declared for these tables; they belong in the .cpp with the data.

## Functions
- File-scope `Aquarium_NameregistStack`, `Aquarium`, `GyoraceFishSelStack`, and
  `GyoraceStack` construct in that order. Native C++ definitions generate the retail
  `__sinit_menuaqua_cpp` instruction stream exactly; `CAquarium::CAquarium` also matches.
- `CAquaFish::Initialize` and `CFishFood::CFishFood` invoke the base
  `CCharacter2::Initialize` directly; qualified C++ calls match their retail bodies.
- Local (static, in .cpp): Get_aquarium_paul_table, Get_aquarium_paul_table_xz,
  local_aquarium_limmit_check (returns wall bits: 1/2 x, 4/8 y, 0x20/1 z), GetEsaInfo,
  GetChildFishNo, GetFishPath, CombineParam, _GYORACE_LISTNUM, _GYORACE_DATA, _PRIZE_LISTNUM,
  _PRIZE_GROUP, _PRIZE, GyoraceCFGAnalyze, SearchOmakeGyoracer, GyoracerListUpdate,
  OmakeGyoraceSelect, ForceSetGyoList.
- `P1` in `FishIMGReplace__FP1P...` and `LoadData__16CGyoraceFishDataFP9mgCMemoryP1` is
  `u_long128 *` (as in mg_memory). Ghidra treats LoadData as a free function; it is a member.
- GetOmakeGyoracer2 returns `CGyoRaceData::GetData(i)` (0xA0-byte entries) when its s16 at 0 is 6;
  typed `CGameDataUsed *` because callers use it as fish data (name at +0x10). Verify when savedata
  is done.
- Getters of char globals (GetGyoRace*) use `lb`; declared `int`.
- MenuAquaInit/MenuGyoraceFishSelInit/GyoraceMenuInit third parameter: meaning not established.

## AquaMode
Values 3, 4, 6, 7 disable fish battles/pairing (ColCheck, Thinking); full meaning in MenuAquaKey /
CAquarium::Step not yet worked out — candidate for an enum.

## Unresolved
- BREEDFISH_USED (owned by userdata, no header yet) is only forward-declared; its fields are
  touched at +0x05..+0x3B here (CalcFishParam sums the u16s at +0x26..+0x2E).
- FISH_PRIZE_INFO field meanings (event_func pushes both to the script stack).
- AQUA_BUBBLE byte 0 doubles as wobble-table row (0..4) while rising and countdown (10..19) while
  popping.

## Stable fish-food call argument order

The `menuaqua.cpp` profile row for `Step__9CFishFoodFv` selects `binary32`
`0x00000000` (0.0f) with `evaluate_first: true`. It applies to every
identical literal in that function, without an occurrence counter or callee
restriction. At `local_aquarium_limmit_check`, this prepares the zero final
argument before the 1.3f collision margin, preserving retail's float
argument registers. The function still integrates food movement, limits it
to the aquarium, updates its drop/entry/sink state, and submits its position
and rotation to the character.

The native 1,044-byte body has zero differing instruction words or relocation
fields after the canonical wrapper build and `fixup_sections.sh` pass. The
current consumer-hook validation checks `0x11C80` allocated unit bytes and
2,927 relocations; the unrelated existing `SetAdjustScale` and `MenuAquaInit`
differences were present in that earlier validation; the current zero selector
for `MenuAquaInit` is verified below.

## Aquarium camera constructor floating argument calibration

`MenuAquaInit__FP9mgCMemoryPii` uses stable binary32 `0x00000000` (0.0f)
`evaluate_first: true` to restore camera constructor argument materialization
in the order 40.0f, zero, 30.0f, 8.0f. The function now matches canonical bytes
and resolved relocations. With the division primer removed and helper masks
GPR `0x30` / FPR `0`, validation checks `0x11C50` bytes and 2,970 relocations.
The seven existing `DrawEsaDropRoot` issues remain; their complete masked
instruction bytes and resolved relocation targets/addends are unchanged.
