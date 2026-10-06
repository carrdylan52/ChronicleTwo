# mg_math notes

Engine maths helpers (`mg` prefix). No class is owned by this unit (`class_units.tsv`); the header
declares 60 free functions, two enums and three globals. First-game counterpart: the un-prefixed
helpers in `chronicle/ps2/include/mathutil.hpp` / `src/mathutil.cpp` (`VectorMaxMin`, `DistVector`,
`AngleInterpolate`, `Sinf`, ...). This game's versions are mostly VU0 inline asm (`lqc2`/`sqc2`,
`vmax`/`vmini`, `ctc2`/`cfc2` on the status flag) rather than C.

## Types used
- `mgVu0FBOX` is owned by `mg_drawenv` (declared in `mg_drawenv.hpp`); forward-declared here. Layout seen
  from this unit and its callers: two quadwords, **max corner at 0x00, min corner at 0x10**
  (mgBoxMaxMin stores vmax to +0, vmini to +0x10; mgApplyMatrix callers in mapparts pass
  `box, box+0x10` as `max, min`; mgInsideScreen passes `box, box+0x10` to mgCreateBox8(max, min)).
- `mgInterpolateMode` (name not retail): mode argument of mgVectorInterpolate / mgAngleInterpolate.
  0 = fixed step (snap when closer than step), 1 = gap / step. Other values: VectorInterpolate
  writes nothing; AngleInterpolate returns `from` wrapped. Parameter stays `int` for mangling.
- `mgPointPoly3Result` (name not retail): return of Check_Point_Poly3 / mgCheckPointPoly3_XZ.
  With cross products c0 (edge v0->v1), c1 (v1->v2), c2 (v2->v0): out of bounding rectangle -> 0;
  c0 == 0 -> 2; c1 == 0 -> 3; c2 == 0 -> 4; all > 0 or all < 0 -> 1; else 0.
  mgCheckPointPoly3_XYZ returns only 0/1 (sign test of three cross products dotted with normal;
  all >= 0 or all <= 0 -> 1).

## Globals (data stays in asm)
- `sin_table_num` (.sdata 0x37C700, float): set to 1024.0 by mgCreateSinTable, used as divisor.
- `sin_table_unit_1` (.sdata 0x37C704, float): set to 0x4322F983 = 162.97466 = 1024 / (2*pi);
  mgSinf multiplies the angle by it.
- `SinTable` (.bss 0x395EC0, 0x1000 = float[1024]): sin(2*pi*i/1024).
- The header declares `sin_table_num`, `sin_table_unit_1` and `SinTable` as extern. Their
  reconstructed names are absent from the retail symbol-binding table.

## Function details (for the body writer)
- Sizes in the header are retail symbol sizes (main.symbols.txt), not the padded manifest sizes.
- Clip functions return the result of `(cfc2 status & 0x80) == 0` (sticky sign flag; `ctc2 $0`
  clears status first). Return type taken as `int`; callers only test != 0.
  - mgClipBoxVertex(p, max, min): vsub.xyz max-p, p-min.
  - mgClipBox(max0,min0,max1,min1): vsub.xyz max0-min1, max1-min0 (overlap).
  - mgClipInBox: vsub.xyz max1-max0, min0-min1 (box0 inside box1).
  - W variants use `.xyw` masks (screen-space boxes, mg_frame).
- mgZeroVector: `sq $zero` (all four zero). mgZeroVectorW: `sqc2 vf0` -> (0,0,0,1).
- mgCreateBox8(out[8], max, min): out[0]=min, out[7]=max, others mixed via vaddx.x/.y/.z with vf0.
- mgDistVector*/XZ: vmul then vmr32 sums; non-squared ones use vsqrt + vwaitq, result via
  `cfc2 vi22` (Q). XZ sums x and z only. Squared ones via qmfc2.
- mgDistPlanePoint(n, on_plane, p) = n . (p - on_plane) (sceVu0SubVector + sceVu0InnerProduct).
  Ghidra shows it void; mgReflectionPlane consumes its $f0, so it returns float.
- mgReflectionPlane: d = DistPlanePoint; out = (on_plane - p) - n*(-2d); returns 2d.
- mgDistLinePoint(p, a, b, nearest): parameter t of projection onto segment; outside [0,1] picks
  the nearer endpoint (sceVu0CopyVector) and returns its distance.
- mgIntersectionSphereLine0(r, from, to, hits): sphere at origin, solves quadratic; returns 0, 1
  or 2. mgIntersectionSphereLine(sphere, ...): radius = sphere[3]; translates into sphere space,
  calls the 0 variant, adds the centre back to each hit with mgAddVector.
- mgIntersectionPoint_line_poly3(from, to, v0, v1, v2, normal, hit): 0 if line parallel to plane,
  else hit = from + (to-from)*t and returns mgCheckPointPoly3_XYZ(hit, v0, v1, v2, normal).
- mgCheckPointPoly3_XZ: tail-jumps to Check_Point_Poly3(p.x, p.z, v0.x, v0.z, ...).
- MulMatrix3(m, a, b): loads m, a, b; computes (m*a) then that *b with the same vmula/vmadda
  pattern as mgMulMatrix, stores into m. Used by mgRotMatrixXYZ(m, rot): Z(m), Y, X ->
  MulMatrix3(m, Y, X).
- mgRotMatrixX/Y/Z start from mgUnitMatrix then set cos/sin with libm `cosf`/`sinf` (not mgSinf).
- mgCreateMatrixPY: unit, sceVu0RotMatrixY(m, m, angle), m[3] = position with w = 1.
- mgInversMatrix: VU0 cofactor inverse of the 3x3 part, divided by determinant (vdiv), translation
  row = -(t * R^-1), w = 1.
- mgLookAtMatrixZ: two matrices (pitch about x using y component, yaw about y using xz length),
  multiplied with mgMulMatrix.
- mgShadowMatrix(m, light_dir, on_plane, normal): light_dir xyz with w=0, normalised; if
  normal . on_plane == 0, on_plane -= normal*0.1 first; builds planar projection along light.
- mgApplyMatrixN(out, m, in, n): n vectors. mgApplyMatrixN_MaxMin: same plus vmax/vmini bounds.
- mgVectorMinMaxN(max, min, v, n): for n >= 2, bounds include v[0] through v[n-1], with v[1]
  folded twice. The final preload reads v[n] without folding it. Counts <= 1 still execute the
  loop and access v[1]; callers require a readable preload beyond the bounded vectors.
- mgApplyMatrix(max, min, m, box_max, box_min) = CreateBox8 + ApplyMatrixN_MaxMin(.., 8, ..)
  in a 0x80-byte stack buffer.
- mgAngleCmp(a, b, tol): d = a-b wrapped to (-pi, pi]; 0 if d == 0; 1 if d > tol; -1 if d < -tol.
- mgAngleLimit: if |a| >= pi, a -= (int)(a / 2pi) * 2pi then one more wrap step.
- mgRnd: rand() / 2147483648.0f. mgNRnd: sum of 12 mgRnd() - 6.0.
- mgSinf: index = (int)(|a| * sin_table_unit_1) % 1024 (signed modulo code), negated for a < 0.
  mgCosf: mgSinf(a + pi/2) (tail call).

## Unresolved
- Clip functions could return `bool` instead of `int`; codegen (sltiu) is the same either way.

## Job mg_math.1 (first 50 functions)
- Binding (local_symbols.tsv): `MulMatrix3` and `Check_Point_Poly3` are file-local -> `static` in
  the .cpp, removed from the header (Check_Point_Poly3 has a static prototype at the top because
  mgCheckPointPoly3_XZ precedes it). `sin_table_num`, `sin_table_unit_1`, `SinTable` are not
  found under those reconstructed names in the retail binding table; the header declares them extern.
- mgVectorMinMaxN seeds bounds with v[0], preloads v[1] and folds v[1] twice. For count >= 2,
  the last folded vector is v[count-1]; the final unused preload reads v[count].
- VU0 functions are written as `asm { }` blocks on the raw argument registers ($4..$9), in the
  style of the first game's chararead/bound; they match byte-for-byte and are promoted. Float ones
  return through `mtc1 $2, $f0` with no C return (MWCC warns "return value expected").
  Clip functions read the status into a `register int` and return
  `(status & MG_VU0_STATUS_SIGN_STICKY) == 0` (new enum `mgVu0Status`); this matches.
- Three pipelined VU loops (`mgApplyMatrixN`, `mgApplyMatrixN_MaxMin`, `mgVectorMinMaxN`)
  use their retail instruction sequences in asm blocks. Together with the 30 existing VU blocks,
  all 60 functions compile and match. No optimization or scheduling pragma is present.
- mgShadowMatrix: the plane normal is scaled by 1 / (normal . point) so the plane is n.x = 1;
  with l the normalised light and d = n.l, m[i][j] = -(n_i l_j - [i==j] d) / d, row 3 = l / d,
  m[3][3] = 1, column 3 rows 0-2 = 0.
- mgLookAtMatrixZ calls mgDistVector once (the first game's version called it twice).

## Job mg_math.2 (last 10 functions)
- All ten are plain C++ and promoted; they follow the first game's mathutil.cpp except where noted.
- mgVectorInterpolate STEP mode differs from the first game: it measures the whole gap with
  mgDistVector, snaps with a quadword copy (`*(u_long128 *) out = *(u_long128 *) to`, lq/sq), else
  Normalize + ScaleVector by step + AddVector. FRACTION mode is per-component from + gap / step.
- mgAngleLimit calls fptosi once (`angle -= 2pi * (int) (angle / 2pi)`); the first game's
  version truncated twice.
- mgCosf is `mgSinf(1.5707964f + angle)` (tail jump). `sin_table_unit_1 = 162.97466f` gives
  retail's 0x4322F983.
- The 30 original VU0 `asm { }` function bodies are compiled source and recorded as perfect.

## Current source coverage

All 60 functions have compiled source bodies and match retail. Thirty asm blocks have the same
instructions and text as upstream; three pipelined-loop blocks have no upstream block.
`Check_Point_Poly3` uses upstream's already matched normal definition and has static linkage.
