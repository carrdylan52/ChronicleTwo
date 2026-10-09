# Fish-race data migration, October 8 night, round 4

Baseline: `9eb8f660`, pinned SF image, canonical flags and unchanged profile.
The existing race, camera, effect and fish layouts supply the types. Neither
sgInitGyoRace nor sgLoopGyoRace, including their guarded drafts, is changed.

## Native tables and state

`fish_name` is the exported 18-entry character-model pointer table with
inline filenames. The compiler emits its 18 string objects; verified piece
padding supplies the eight bytes before `cam_pos`. No extra entries are
introduced. `cam_pos` is five SDK float vectors, each with homogeneous
component 1.0. The previous-camera sentinel is `static int old_cam_no = -1`.
Each table is migrated and checked independently.

The race counters, commentary state, sound handle, textures, effect pointers,
camera identifier, race selection and entrant arrays become documented typed
definitions under their retail symbols. File-local state remains static;
the published symbols retain their header declarations. In particular,
`battle_EffectPara` points to arrays of 32 BattleEffectPrim objects.

The existing draft's `game_data[8]` agrees with the retail 0x20-byte symbol;
the older six-pointer claim in notes.md is superseded. Only six entrants are
used by this race, while the allocated pointer array has eight slots. The
result, simulation and fish-display arrays retain their established element
types and six-entry counts. Alignment gaps do not become source fields.

## Retained assembly-owned constants

The following markers remain because the guarded assembly functions access
their retail symbols directly. Compiler-generated anonymous identities from
inactive draft bodies cannot replace these live references. No literal is
renamed, exported artificially or moved into a dummy object.

| Consumer | Retained markers |
|---|---|
| sgInitGyoRace vector templates | `at_1027__4`, `at_1028__9` |
| sgInitGyoRace filenames, formats and names | `at_1373__3`, `at_1374__2`, `at_1375__2`, `at_1376__2`, `at_1377__4`, `at_1378__3`, `at_1379__3`, `at_1380__2`, `at_1381`, `at_1382__2`, `at_1383__3`, `at_1384__2` |
| sgLoopGyoRace vector and rectangle templates | `at_1481__4`, `at_1524__2`, `at_1547`, `at_1548` |
| sgLoopGyoRace names and diagnostics | `at_1696__2`, `at_1697__3`, `at_1698__3`, `at_1699__3`, `at_1700__2`, `at_1701`, `at_1702`, `at_1703` |

`at_1380__2` is shared by Init and Loop; `at_1384__2` also shares text with
the native commentary path. Their assembly consumers still require the
retail symbols even where a native caller already uses an inline literal.

## Drawing data and retained BSS markers

The existing `float step[4] = {0, 0, 0, 1}` supplies `at_1766__3` without
its marker. The existing `lap_inf[2][5]` supplies its BSS template and verified
piece padding. The raster phase and its initialization flag become typed
file-local float and signed-char definitions under their existing symbols;
the active helper's initialization and arithmetic remain unchanged.

A natural function-static `float ras_off = 0.0f` reproduces the complete PAL
image, but check_objects rejects its two anonymous small-BSS identities
(missing ras_off_1762/init_1763, two unexpected unnamed pieces). The exact
symbol definitions pass both checks. This is a naming/tooling limitation,
not a reason to change flags, the profile or the retail function.

| Retained BSS marker | Reason |
|---|---|
| `at_1765__2` | The existing origin-array initialization alone does not preserve the unit initializer's reference: removing the marker changes `__sinit_gyorace_cpp+0x10`. Retained pending template identity handling. |
| `at_1775`, `at_1776` | SDK integer-vector zero initializers with memcpy of the origin compile, but change the strip helper and linked initializer layout. The exact original body is restored; no new pun or copy helper is retained. |
| `D_01F5971C` | This four-byte explicit boundary follows the fish-rank reservation. Removing it changes the linked unit initializer reference at +0x10. No filler object is introduced to preserve the boundary. |
| `lap_inf2_1799` | The natural five-int local has a 0x14-byte payload and a 0x50-byte terminal reservation. The PAL image matches without its marker, but check_objects rejects the BSS run ending at 0x01F599C4 instead of 0x01F59A00. The 0x3C-byte terminal gap requires tooling support. |

All failed variants and their receipts remain in `.private/dataB-r4/gyorace-*`.
The retained source uses neither a new alignment filler nor an artificial
initializer/vtable helper. No header or other unit is changed.

## Measurement and validation

Markers (RODATA / BSS): **48 / 40 → 26 / 5**.
Fresh objdiff `matched_data / total_data`: **4 / 2729 → 125 / 2729**.

`gyorace-final-build.log` prints `SCES_511.90: OK`; `gyorace-final-objects.log`
passes 149/149 objects. These receipts, the incremental checks, baseline
measurements and object-hash audit are under `.private/dataB-r4/`.
All objects outside the seven owned units retain their baseline file hashes.
No function is promoted and no assembly fallback or guarded function body changes.
