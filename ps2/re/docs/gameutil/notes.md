# gameutil: reverse-engineering notes

## C++ draft status
The matching build supplies 31 of the 37 functions as native C++. Six remain
under `NONMATCHING` with retail assembly fallbacks. Raw draft comparisons use
the pinned deterministic compiler profile; promotion also requires complete
object bytes, resolved relocations, and the inherited linked-image baseline.

Header: `ps2/include/gameutil.hpp`. No class in `class_units.tsv` is owned by gameutil; the header
declares the plain structs and enums the unit's code uses.

## Linkage
- Local (static, keep in the .cpp, not in the header): `QuatSlerp__FPfPffPf`,
  `testVUnew__FPA4_fPfPfPfPf`, `SetKeyFrame__FP8Mot_ListP20FRAME_VECTOR_EX_DATAP9mgCMemory`
  (all in `local_symbols.tsv`). Every other function is global and declared in the header.
- All data of the unit is local (static): `OldSkinFrame` (mgCFrame*, last skinned frame; reset to 0
  by DeformMesh), `def_vrtx` (sceVu0FVECTOR[800], 0x3200: skinned-vertex accumulator, w = weight
  sum), `def_nml` (0x10 in retail although MotionProc3 writes up to 800 normals into it; it
  overruns into the tmp_* matrices), and the function-local statics `vert_845`/`vert_915`
  (sceVu0FVECTOR* into the visual's vertices), `nml_916` (normals), `tmp_*Matrix*_8xx/9xx`
  (sceVu0FMATRIX), `at_945` (16-byte vector constant), `at_966`/`at_967` (printf strings
  "MAX_VERTX OVER %d/%d" / "MAX_NORMAL OVER %d/%d", limits 400 and 800). No `extern`s in header.
- MotionProc2 / MotionProc3 / testVUnew use VU0 macro code (`lqc2`, `vmulabc`...).

## Types (offset -> evidence)
### tagMOTION_TYPE (0x14)
Size: CCharacter2 holds `tagMOTION_TYPE[8]` at 0x3C0 and another `[8]` at 0x460 (stride 0x14),
`memset(dest, 0, 0x14)` in `_MOTION`/`_SHADOW_MOTION` (character).
- 0x00 base_matrices: CreateAnimeDataEX memcpy of file 0; MotionProc2/3 index `* 0x40` by frame.
- 0x04 motion_list: built by CreateAnimeDataEX from file 1; walked by SetMotionTime/ChangeMotion.
- 0x08 skin_list: built from file 2; walked by DeformMesh, AnimeDataInit.
- 0x0C unknown (copied as a word by `_MOTION`).
- 0x10 frame_info: `_SHADOW_MOTION` stores the tagFRAME_INF* (chr+0x470); `_SKIN_MOTION` reads
  chr+0x3D0 as tagFRAME_INF*.
First game's tagMOTION_TYPE was 0x80 with an embedded MOTION_STATE; this game moved state into
CCharacter2 (0x368..0x3A0, 0x508).

### Mot_List (0x20, asserted)
Allocated by `mgCMemory::Alloc(3)` (quadwords, callers always pass `size/16 + 1`), so size is in
0x20..0x2F; 0x1C is the last used word, 0x20 matches the first game.
0x00 frame (GetFrame index), 0x04 target (vertex index+1 for type 12: `(target-1)*0x10`; material
index `*0x30` for types 40/41; bone frame for skin types), 0x08 type, 0x0C key_count (unsigned
loops in SetKeyFrame/MotionProc2/3, signed test in MotionProc's binary search), 0x10 values
(`Alloc(count+1)`, stride 0x10), 0x14 key_frames (`Alloc(count/4+1)`, stride 4, binary searched
with unsigned compares), 0x18 next. CreateAnimeDataEX / ChangeWeight build the list by prepending
and then reverse it in place.

### FRAME_VECTOR_EX_DATA (0x20) and Mot_File_List (0x20)
Motion file layout: a 0x20 header then key_count 0x20 keys, repeated while header+0x14 != 0.
Header: 0x00 frame, 0x04 target, 0x08 type, 0x10 key_count, 0x14 more. Key (retail type
FRAME_VECTOR_EX_DATA, SetKeyFrame's parameter): 0x00 frame -> key_frames[i], 0x10..0x1C value.
`Mot_File_List` is the first game's header name, not a retail symbol of this game.
In ChangeWeight the skin file is passed as `unsigned char *`; its header +0x04 is a frame index in
`skin_root`, mapped to `root` by name (`SearchFrameID(root, GetFrame(skin_root, idx)->name@0x50)`).

### tagFRAME_INF (0x20)
Stride `* 0x20` everywhere; AnimeDataInit(**) allocates `stAlloc64((frames + 10) * 2)` quadwords.
0x00 parent: `(GetFrame(i)->parent@0x54 - root) / 0x110` (so mgCFrame is 0x110 and frames are one
array). 0x04 vertex_count / 0x08 normal_count from visual (mgCVisualMDT) +0x20/+0x24.
0x0C vertex_refs: `Alloc(vcount*3+1)` quadwords = 0x30 per vertex: [0]=count, [1..11]=values; filled
from visual+0x48 primitive list (prim +4 -> strip list, strip +2 stride, +6 count, +0xC index
array, +0x10 next; prim->+4 ushort flag 0x200 skips): for each vertex index idx[l] it appends
idx[l+1]. Meaning of the appended value (probably normal index) not confirmed; never read in this
unit. 0x10 base_vertices / 0x14 base_normals: copies of visual +0x30/+0x34. 0x18..0x1F unused.

### MOTION_FILE_INFO (0xC)
Callers build `MOTION_FILE_INFO[3]` on the stack (name, data, size) for base matrices, motion keys,
skin keys; name is nulled when the file is missing and CreateAnimeDataEX tests name (+0), then uses
data (+4) and size (+8). `data` typed `void *` (pack data; first game used `unsigned int *`).

### CollisionInfo (0x10)
The CCPoly wrappers build `{count, polys, 0, 0}` on the stack (sp+0x10..0x1C) and pass its address.
Only +0 count and +4 polys are read.

### CCPoly (not declared here; forward-declared)
Not owned by any class in class_units; probably belongs in collision.hpp (CCollision's
PickUpNearPoly). Layout seen here matches the first game: 0x50 stride; vertex[3] at 0x00/0x10/0x20,
normal 0x30, s16 attributes 0x40, 0x42, 0x44 (GetCPolyAttr tests 0x44 == 7 or 1), 0x46 ignore mask
(`attr & ignore_mask` skips the polygon), 0x48, 0x4A, s32 0x4C. GetFootPoly merges the first
nonzero 0x40/0x42/0x44 over all hits. CreateCharaCPoly zeroes 0x40..0x4F.

### MoveCheckInfo (owned by dng_main; forward-declared)
At least 0x110 here (larger than the first game's 0xD0): 0x00 float radius (<=0 -> 15),
0x04 flag (nonzero skips ground search), 0x08 landed, 0x10 CCPoly ground poly (0x50),
0x60 ground found, 0x70 CCPoly (0x50), 0xC0 ground point vec4, 0xD0 CheckWidth result,
0xD4 flag + 0xE0 vec4 (special area above), 0xF0 flag + 0xF4 float signed distance +
0x100 vec4 (special area crossed).

### RECT (forward-declared; owner probably drawwin)
{x, y, w, h} ints: CheckPosInOutForRect uses x..x+w, y..y+h; GetDisPosToRect uses x+w/2, y+h/2.

## Enums
- MotionKeyType: values from MotionProc (0,1,2,12,30,31,32,33,40,41,50,51) and MotionProc2/3 /
  AnimeDataInit (20, 21). 20 = weighted skin (weight = value.x * 0.01, MotionProc3 only handles 20);
  21 = averaged skin in MotionProc2 (accumulate, divide by w). 1 calls mgCFrame vtable+0x2C,
  2 writes frame+0xE0 and sets frame+0x40=1, 50/51 set frame+0xF4 attr +0x18 (0/3 and 2/1),
  40/41 use visual vtable+0x10 material array (+0xC alpha = 1-value), 41 also sets attr +0x28=2.
- CheckWidthSide: CheckWidth's result bits; diagonal probes OR 5/10/9/6 (= +X|+Z, -X|-Z, +X|-Z,
  -X|+Z), axis probes 1/2/4/8.

## Function notes
- Return types: CheckHit/CheckHitVertical return polygon index or -1 (0 for NULL info in CheckHit,
  -1 in CheckHitVertical); CheckHits* return hit count; sort > 0 and sort < 0 both sort ascending by
  distance (stored in hit_points[i][3]). Pipe radius is from[3]; sphere radius is sphere[3].
- MoveCheck always returns 0. CreateCharaCPoly returns 0 if max_polys < 2, else 2.
- CheckPosInOutFor*/CalcIntersection* return 0/1 (declared int; `xori` result could also be bool).
- ChangeWeight: void (v0 is memcpy leftover). AnimeDataInit(*) returns 1; CreateAnimeDataEX 1.
- MotionProc (time): `fptoui(time)` then binary search; types 12 process consecutive lists with the
  same frame in one call.
  In the vertex-key case, each consumed list advances to the next entry; a null next entry
  returns null immediately. The guarded C++ draft can express this with an ordinary null
  check and assignment, without labels or jumps.

## Division-check pragma

The unit-level `divbyzerocheck` pragma was redundant with the global MWCC flag; removing it left the full compiled object identical in objdiff.

## Assembly gaps

`testVUnew`, `MotionProc2`, `CheckHit(CollisionInfo*, ...)`, and
`CheckHits(CollisionInfo*, ...)` retain C++ drafts under `NONMATCHING` and use
`INCLUDE_ASM` in retail builds. Their promoted versions contained VU0 assembly
inside C++ functions, so those promotions do not meet the source matching rule.

## Guarded collision-query remainders

`CheckHitsSphere` copies the complete sphere vector into both bounds, then
adds/subtracts the captured radius from xyz. Its first box rejection tests
`sphere_max < poly_min`; the second rejects `!(sphere_min <= poly_max)`.
Both positive and negative sort modes swap when `!(point_i.w <= point_j.w)`.
The unordered-comparison behavior therefore matters; replacing the latter
predicates with ordinary greater-than or reversed less-than changes NaN cases.
The zero-sort path returns before the two sorting blocks.

The retained sphere draft differs in 55 of 252 padded words, down from
246/252. Its body is 0x3E4 within the retail 0x3F0 extent. Its 0x140-byte
stack frame is 0x10 smaller than retail, the spilled argument/vector slots
are lower, and the scan index and polygon cursor use exchanged s2/s3 registers.
The sorting instructions otherwise line up. Ordinary versus aligned vector
typedefs, component-index lifetime, and function-scope scalar declarations
alone do not resolve this. Reconsider with evidence for the original scratch
lifetimes or an established compiler allocation policy.

## Movement query native match

`MoveCheck` copies polygon results as five float vectors, including the raw
surface-attribute representation, using the same `CCPolyCopy` view already
used by `GetFootPoly`. Its scratch view retains the real polygon's 16-byte
alignment at the typed query interface. The captured `CheckWidth` return
value is stored in `info->width_result` and tested without rereading the
member.

The native function has a 0x608-byte body within retail's 0x610 padded extent
and zero differing words or relocation fields. Capturing `float *query_pos =
ground_query` alongside the existing typed polygon output pointer, then using
both named pointers for both copies/ground queries, resolves the two pointer
loads at +0x540/+0x548. The paired input/output lifetimes reproduce retail's
position-before-polygon argument order around 20.0f. Capturing the input only
for the second query retains the two-word remainder; the shared capture is
required. No compiler-profile row is added.

The manually unguarded canonical wrapper/fixup check passes the complete unit:
0x5BE0 allocated bytes and 357 resolved relocations. Probe receipts are under
`.private/receipts/nearmiss-probes/gameutil/e25/` and the complete-object check
under `.private/receipts/nearmiss-canonical/gameutil/e25/`.

## Additional sphere layout negatives

The 55/252 sphere draft remains guarded. A sphere-bounds `mgVu0FBOX` aggregate
changes alias scheduling and differs by 240/252 words; separate quadword union
views differ by 247/252; a polygon-bounds box differs by 211/252. None restores
the missing 0x10 of frame space. Giving polygon scanning its own index,
independent of sorting, retains the 0x140 frame and adds ten register differences
(65/252). These candidates are reverted. Reconsider with evidence for the
actual scratch lifetime/allocation that produces the 0x150 frame, rather than
another vector typedef or bounds aggregate. Private sphere receipts are
`nearmiss-probes/gameutil/sphere-n1` through `sphere-n4`.

## Other guarded remainders

`MotionProc(float)` retains its original draft, 289/596 padded words and
0x934 bytes against retail 0x950. Its first differences exchange the saved
camera/key registers and retain a subtraction of key/next where retail
materializes -1. Direct unsigned upper-bound subtraction removes a word
and shifts later code; unsigned locals alone do not fix the difference.
Retail reads the motion type after `GetFrame`; m2c lifts this read in its
pseudocode, so moving it before the call is incorrect. Sequential vertex
time-region tests alone do not close the later control-flow differences. Reconsider
with original index-lifetime and vertex-case control-flow evidence.

## COP2 audit

`CheckHit(CollisionInfo*)` and `CheckHits(CollisionInfo*)` each load the query
bounds into vf10/vf11 using two direct `lqc2` instructions between ordinary
calls. `MotionProc2` clears each skin accumulator with a direct
`sqc2 vf0`, storing (0,0,0,1). These are instructions in the callers, not SDK
call bodies. The SDK header declares out-of-line operations and no existing
native inline helper reproduces these sequences. All three remain parked,
along with the VU0 `testVUnew`; reconsider only when an admissible existing
SDK/inline mechanism or a separately authorized VU implementation policy
covers those operations.

## Nearmiss final validation

The PAL verifier is identical to i12: only the inherited 0x2C text bytes differ;
main and BSS remain retail-sized. The failure set and every problem entry stay
mg_texture, nd_meswin and actscript (146/149 units pass). Coverage is now
6,676 matched / 176 guarded / 15 assembly-only / 5 fuzzy. All 148 other raw
object hashes are unchanged, including the default guarded editmenu object.
The native MoveCheck body removes eight object tail-padding bytes (gameutil
0x5BE8 to 0x5BE0); linked alignment supplies them before GetFootPoly, and the
verifier is unchanged. Final receipts:
`.private/receipts/nearmiss-final/comparison.json`, `objects-sha256.json`,
`image.json`, `check.log`, `objdiff.log`, `coverage.txt` and per-unit draft logs.
No shared header, compiler-profile row or promotion-ledger entry is changed.
