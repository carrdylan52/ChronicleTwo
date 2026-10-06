# editmap2 notes

## C++ draft status
The matching build has 16 perfect functions and 5 assembly functions.
`CEditMap::GetSeSrcVolPan` has a clean guarded draft; its grid accesses use
a typed iterator. `PlaneNormalXZ` retains its vector-unit block. Both
file-local helpers have static linkage. The draft check prints 16 matches,
1 differing draft and 4 functions without a draft.

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
COP2 opcodes. The function therefore remains an inline-assembly match, not
a decompiled C++ match.

To return it to undecompiled status, replace its whole source definition with
`INCLUDE_ASM("ps2/asm/pal/nonmatchings/editmap2", PlaneNormalXZ__FPfPfPfPf);`
and run the normal `scripts/build/cmake.sh` setup/build path. The split must
regenerate the per-symbol assembly under `ps2/asm`; changing that generated
file by hand is inappropriate. Then check the marker's object with `diff.sh`
and verify both PS2 builds.
