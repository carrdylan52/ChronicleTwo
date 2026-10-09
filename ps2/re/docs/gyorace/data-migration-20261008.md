# Fish-race data migration, October 8 night

The round-4 state below is historical; the round-6 sections record the
data ownership after native race initialization.

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

Retail `game_data` is a six-pointer array: readelf gives its declared
symbol size as 0x18 at 0x01F59740. The 0x20-byte reservation includes eight
alignment bytes. The inactive draft's eight-entry declaration is not its
retail type; the native definition follows the six-entry layout already
recorded in notes.md. The result, simulation and fish-display arrays retain
their established element types and six-entry counts. Alignment gaps do not
become source fields.

The initial migration used the draft's eight-entry declaration and still
passed PAL/object checks, so those checks alone do not validate this source
extent. The final declaration is corrected to the ELF size and validated
again. This supersedes the initial migration's eight-entry note.

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
| `at_1775`, `at_1776` | SDK integer-vector zero initializers with memcpy of the origin change the strip helper and linked initializer layout. A typed aggregate containing an SDK integer vector, with ordinary value assignment, also changes linked code. Keeping these markers restores its startup initializer but leaves a text mismatch. The exact original body is restored; no new pun or copy helper is retained. |
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

## Round 6: native initialization vectors

Baseline `c935aa16` has native `sgInitGyoRace`, with documented SDK
`sceVu0FVECTOR` local initializers for position `{0, -15, 0, 1}` and
rotation `{0, 3.1415927f, 0, 1}`. They supply the two 0x10-byte templates
at `0x3621F0` (`at_1027__4`) and `0x362200` (`at_1028__9`). Neither has
an assembly or guarded-draft consumer now, so both markers are removed.
The initializers and every function body remain unchanged.

With a marker present, `postprocess_object.py::bind_local_data` redirects
native relocations to that marker's retail address, checks the redundant
native payload against retail and marks the unused copy `.dead`.
`fixup_sections.sh` removes it. This explains the matching baseline despite
the source containing both native literals and their markers. Without a
marker, `name_literal_data` names the native template through its payload
and real consumers; verified piece padding preserves the retail extent.
No replacement named template, filler or artificial initializer is needed.

The warm build passes PAL and 149/149 complete objects; gyorace checks
0x4FF8 bytes and 1,114 resolved relocations. Removing these two markers
also passes PAL and 149/149 objects. Marker counts become **24 RODATA /
5 BSS**, from **26 / 5**. Fresh objdiff `matched_data / total_data` remains
**125 / 2729**; marker removal does not change the existing native source
forms measured by objdiff. All other 148 object hashes equal the warm
baseline, and all function source is identical.

Receipts are in `.private/dataB-r6/`: `warm-build.log`, `warm-objects.log`,
`warm-gyorace-object.log`, `before-progress.log`, `before-snapshot.json`,
`vectors-build.log`, `vectors-objects.log`, `vectors-progress.log` and
`vectors-snapshot.json`.
