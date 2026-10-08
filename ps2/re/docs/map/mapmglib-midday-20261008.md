# Map matching pass at 0c33a7e

This pass uses the pinned `chronicletwo_dev:sf-d8bf13c` image, canonical
MWCC 3.0-011126 `-O3,p` flags, and the checked-in Satan's Fiddle profile.
The base has 6,741 matched, 119 guarded, 10 assembly-only and 2 fuzzy
functions. Previous constructor findings in `notes.md`,
`midday-20261008.md` and `../funcpoint/placement-new.md` remain applicable.
Both targets were inspected through `decompile.sh`/m2c; existing native
functions were compared rather than re-analyzed.

## Target measurements

Word counts mask relocation operands for diagnosis and include retail
alignment padding. A guarded draft's score does not establish a native match.

| Function | Base draft | Retained draft | Native / retail extent |
| --- | ---: | ---: | --- |
| `AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory` | 19/65 | 19/65 | 0x104 / 0x100 |
| `CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi` | 50/112 | 112/116 | 0x1D0 / 0x1C0 |

`AddPartsGroup`, at 0x15DA50, retains its source. The meaningful constructor
alternative `PartsGroupData() : parts(0) {}` produces exactly the same 19/65
result as assigning `parts` in the body, and is not retained. Removing the
caller's duplicate data clear was already measured at 26/64 and was not
repeated. The allocation-result copy/null-branch scheduling and extra caller
clear remain the boundaries. `PartsGroupData` remains a single pointer;
there is no evidence for changing it to an array or adding initialization
helpers. The shared `CList` header is unchanged.

`CreateDrawRect`, at 0x15E210, formerly advanced a raw `CMapParts *` once per
iteration. The retained draft captures `place_parts` and indexes that typed
array by `j`. Capturing the base preserves the traversal even if a called
routine could change the map's base pointer; the existing count is still
read for each loop condition. Direct `place_parts[j]` produced 113/117 words
and 0x1D4 bytes; the captured base produced 112/116 and 0x1D0. The latter
adds saved registers for the base and displacement relative to the earlier
pointer walk, so the cleanup increases the positional difference. It is
required source compliance, not a promotion or an improved score.

The selection rectangle is a `MapDrawOffRect` with the existing 0x30-byte
layout; parts and bounds use `CMapParts` and `mgVu0FBOX`. Existing documentation
owns these layouts. `GetBoundBox`, `mgClipInBox` and native box assignment are
the real dependencies. Retail contains no VU0 instructions in this target.
The list-construction null branch remains an additional mismatch after the
array allocation differences. Reconsider only with natural constructor and
array-index allocation evidence; replacement assembly, pointer induction
and synthetic constructor arrays are not acceptable solutions.

## Verification and receipts

The final native diagnostic has 83 exact functions and the two guarded
targets. The complete guarded object compares with retail at 0x4734 checked
bytes and 418 resolved relocations. Neither guard is removed.

Receipts are under `.private/mapmglib-receipts/`:

- `baseline-map/` and `final-draft-map/`: canonical native objects, scores and
  instruction comparisons.
- `m2c-AddPartsGroup__4CMapFPcP9CMapPartsP9mgCMemory.log` and
  `m2c-CreateDrawRect__4CMapFP9mgCMemoryP9mgVu0FBOXP9mgVu0FBOXi.log`: m2c output.
- `map-member-initializer/`, `map-indexed/`, `map-captured-array/`: rejected
  constructor spelling and typed-index measurements.
- `final-units.log`, `final-objects.log`, `final-build.log`,
  `final-hash-comparison.json`, `final-coverage.log`: final acceptance state.

The final acceptance state retains 147/149 object passes and the existing
PAL `.text` difference of 0x26 bytes; all other PAL file-backed sections and
the 0x01F64A00 memory end pass. The two existing object failures remain
`nd_meswin/DrawMesWin` and `actscript/_SHOT`. All 149 objects retain identical
allocated sections; 148 also retain identical complete-file SHA-256 values.
Only mglib's full-file hash changes with the tested local symbol binding.
`final-allocated-comparison.json` records that unit's 192 unchanged allocated
sections and BSS extents against a reproduction of its exact baseline object.
