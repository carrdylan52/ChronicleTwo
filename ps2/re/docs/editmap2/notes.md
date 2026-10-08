# editmap2 notes

## What the unit owns
- No class or struct is owned by editmap2 (`build/re/class_units.tsv` has no row for it).
- 19 of its 21 functions are `CEditMap` members; all 19 are already declared (with `@mangled`)
  in `ps2/include/editmap.hpp`, owned by editmap. `ps2/include/editmap2.hpp` therefore only
  includes `editmap.hpp`.
- Types used by the unit, all declared elsewhere: `CEditMap`, `EP_PLACE_INFO` (editmap.hpp,
  0x48), `CEditParts`/`CEditPartsInfo` (editparts.hpp), `InScreenFuncInfo` (mapparts.hpp, 0xC),
  `mgCFrame`, `mgVu0FBOX`, `CEditCollision` (editcoll.hpp).

## File-local functions (static, define in editmap2.cpp, not in the header)
Both are LOCAL in `build/re/local_symbols.tsv` and called only from this unit.
- `PlaneNormalXZ(float *out, float *p0, float *p1, float *p2)` @ 0x002F26F0: VU0 outer product
  of (p1 - p0) and (p2 - p0), stored to `out` (lqc2/vsub/vopmula/vopmsub/sqc2). Will need the
  VU0 inline asm form (see the first game's mg_math / docs/MWCC.md). Called by
  `CEditMap::GetEditPartsAlt(CEditPartsInfo*, float*, float, CEditParts**, int)`.
- `int CheckFenceChain(CEditParts *a, CEditParts *b)` @ 0x002F3C60: returns 1 when both parts
  have bound spheres that overlap (`CMapParts::GetBoundSphere`, distance <= sum of radii at
  `[3]`) and any pair of their fence end points (`CEditParts::GetFenceSide(float*, float*)`) is
  closer than 5.0; else 0. Null parts return 0. Called by `CEditMap::PaintFence(CEditParts*)`.

## Data
All data symbols are compiler-generated, so no `extern` is declared:
- `cnt_482`, `init_483` (.sbss, 4 bytes each): function-local static counter and its init guard
  in `CheckEditParts(..., EP_PLACE_INFO*, CEditParts**, int)`; `cnt = (cnt + 1) % 10`.
- `at_1050__2` (.bss 0x10): local static read as a 64-bit value at the top of
  `GroundBalance(int)`. Unrelated to `CEditMap` field offset 0x1050 (`balance_moved`).
- `at_796__4` (.data 0x10), `at_983__3` (.data 0xA), `at_1042..1043`, `at_1127..1130` (.rodata):
  function-local initialised arrays / float literals.

## PlaneNormalXZ matching status
The retail body is ten instructions: three `lqc2` loads, two VU0 zeroing
subtractions, two XZ subtractions, `vopmula.xyz`, `vopmsub.xyz`, and one
`sqc2` store. `decompile.sh PlaneNormalXZ__FPfPfPfPf` reports each VU0
instruction as unsupported rather than C expressions. The available
`libvu0.h` exposes callable SDK functions, including `sceVu0OuterProduct`;
a call introduces an ABI boundary and cannot reproduce this inline body.
Scalar C++ likewise emits scalar FPU instructions instead of the required
COP2 opcodes. The function now uses the narrow inline VU0 exception. The
MWCC body matches all 0x2C retail bytes, and the `editmap2` object passes
`check_objects.py` with 143 resolved relocations.

## GetEditPartsAlt with placed parts

`GetEditPartsAlt` transforms each candidate polygon into a placed part's
space, finds horizontal overlap, and raises the best floor height. In retail,
the `triangle` pointer passed in `a1` to `PlaneNormalXZ` remains in that register
for `CEditCollision::OverlapPoly3XZ`. Compiling the normal helper in this unit
preserves that register across the call. The resulting 0x264-byte function
matches objdiff exactly, and the `editmap2` object passes `check_objects.py`
with 143 resolved relocations.

## GetSeSrcVolPan

`CEditMap::GetSeSrcVolPan` gathers sound sources from placed parts and river
grids. The river pass walks `CEditMap::grid`, an array of `CEditGrid *` at
offset 0xF54; `sndGetVolPan(float *, float *, float *, float, float)` is a native
C++ overload declared in `snd_mngr.hpp`. Replacing the raw mangled call with
that overload leaves its call instructions unchanged.

The typed `this->grid[i]` lookup avoids the old byte offset cast. Reusing the
preceding placed-parts loop index `i` for the river-grid loop preserves the
retail register assignment; the complete function has a 100% object match.
Using a new loop index instead exchanges two saved registers in the river pass.

## GetEditPartsAlt native assessment

The m2c output and retail body confirm that this overload transforms each
`col_area1` triangle into each placed part's frame, computes its XZ normal,
and queries the part's `col_floor` for overlap. Existing `CEditPartsInfo`,
`CCPoly`, `CEditCollision`, and `mgVu0FBOX` declarations describe the accessed
fields, including the 0x50-byte polygon stride and the floor collision at
0x110. Position and rotation getters are virtual slots 0x18 and 0x24.

The retail call to `OverlapPoly3XZ` retains the triangle address in argument
register a1 across the preceding file-local VU0 helper. The helper's actual
body preserves that register; m2c marks it unset because the generic call ABI
clobbers it. A native candidate must preserve this call sequence and all
existing unit failures under the canonical comparator before promotion.

The existing native draft produces a shorter body and changes the
`OverlapPoly3XZ` call location under the deterministic profile. It remains
guarded; the baseline unit passes canonical comparison with that fallback.
No native promotion is claimed for this function or the COP2-only helper.
