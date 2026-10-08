# collision: reverse-engineering notes

Header: `ps2/include/collision.hpp`. Depends on `mg_frame.hpp` (base `mgCFrame`) and
`mg_drawenv.hpp` (by-value `mgVu0FBOX`); neither existed in finished form when the header was
written, so it only compiles once those do. Checked against stubs (`mgVu0FBOX` = two
`sceVu0FVECTOR`, `max` then `min`; `mgCFrame` 0x110 bytes, 16-aligned): all asserts and the
offsets below hold.

## Local functions (static, belong in collision.cpp)
ELF binding LOCAL: `pre_trance_normal(float (*)[4])` (loads a matrix into vf10-vf13) and
`trance_normal(float*, float*, float*, float*)` (transforms 3 vertices by vf10-13 in place, writes
cross product of the edges as the normal). Both inline VU0 asm; used only by
`CColFrame::PickUpNearPoly`. `LoadCollisionFile` and `CreateCollisionMDT` are GLOBAL.

## CCPoly (0x50)
No member functions, so not in class_units.tsv; declared here because collision builds and
fills it. Size: stride 0x50 in every loop (`CreateCollisionMDT`, `PickUpNearPoly`, `CMap::GetPoly`),
`Alloc(n*5)` quadwords.
- 0x00/0x10/0x20 vertex[3], 0x30 normal (`mgPlaneNormal(p+0x30, v0, v1, v2)`).
- 0x40..0x46 s16 x4: from the MDT material (stride 0x60, table at header word 0xE):
  `(s16)(m[0]*0.7+0.01)`, `m[1]*..`, `m[2]*..`, `(s16)(1.0-m[3])`. Absent material -> 16 bytes
  zeroed. Names `ground_kind`, `foot_sound`, `area_kind`, `ignore_mask` come from the first game
  (same layout, same derivation). Corroborated here: 0x46 is ANDed with a query mode in
  gameutil `CheckHit*` (ignore mask); 0x44 compared to 7 and 1 in `GetCPolyAttr`. 0x40/0x42
  only copied (GetFootPoly) -- names unverified in this game.
- 0x48 u16 `parts_no`: `CMap::GetPoly` writes the map-part index; `CEditMap::GetPoly` writes
  `index | 0x1000`; copied with `lhu`.
- 0x4A s16, 0x4C float: only copied (lh/sh, lwc1/swc1).

## CCollision (0x40), first game `CCollision`
- 0x00 s32 `unk_00`: zeroed by Initialize, never read anywhere found.
- 0x10 `mgVu0FBOX bbox` (max at 0x10, min at 0x20): `InsidePoint` -> `mgClipBoxVertex(p, max,
  min)`; `Copy` uses `mgVu0FBOX::operator=`; `mapFIX_CAMERA_RECT` writes 0x20/0x10 directly.
- 0x30 vptr (data declared before the first virtual, as MWCC places it).
- Size 0x40: `__nw(0x40)` in `mapFIX_CAMERA_RECT` (mapload).
- Ctor (mapload, out-of-line copy): store vtable, call `Initialize__10CCollisionFv` directly.
- Vtable `__vt__10CCollision` 0x37B4E0: +8 CreateBBox, +C InsidePoint, +10 GetMaxY,
  +14 Intersection, +18 PickUpNearPoly, +1C Copy(CCollision&,mgCMemory*), +20 Initialize.
  (0x37B4E0/E4 are the usual two zero words.)
- `Copy(dest, mem)` copies THIS into `dest` (assigns `dest.bbox = this->bbox`).
- `InsidePoint` returns `mgClipBoxVertex(...) != 0` (sltu); declared int.
- Differs from the first game: first game had GetPolygon/GetVertexAddress and three
  PickUpNearPoly overloads, no Copy/InsidePoint, and no `unk_00`.

## CCollisionMDT (0x50)
- 0x40 `CCPoly *poly`, 0x44 `s32 poly_count` (Initialize, Copy, CreateCollisionMDT).
- Size 0x50: `__nw(0x50)` (Alloc 7 qw) in `CreateCollisionMDT`.
- Ctor is only inlined (in CreateCollisionMDT): CCollision vtable + inlined Initialize, then
  CCollisionMDT vtable + inlined CCollisionMDT::Initialize. Declared `{ Initialize(); }`.
- Vtable `__vt__13CCollisionMDT` 0x37B4B0: CreateBBox(MDT), InsidePoint(base), GetMaxY(MDT),
  Intersection(base), PickUpNearPoly(MDT), Copy(base), Initialize(MDT), +24 new virtual
  `Copy(CCollisionMDT&, mgCMemory*)`. `CEditCollision` (editmap) has an identical vtable.
- `Copy`: dest.bbox = bbox; dest.poly_count = poly_count; count < 1 -> dest.poly = 0; mem null
  -> shares `poly`; else `new (Alloc(count*5+2)) CCPoly[count]`-style `__nwa(count*0x50, ...)`
  and a 0x50-byte struct copy per element.
  The exact C++ copy indexes `poly[i]` and `dest.poly[i]`, then views each
  0x50-byte element as five `CollisionQuad` blocks for the retail quadword
  stores. This removes byte-offset arithmetic while retaining the exact
  object code; aggregate `CCPoly` assignment changes the copy sequence.
- `CreateBBox`: bbox = (0,0,0,1)/(0,0,0,1), then MaxMin over every triangle.
- `GetMaxY(pos)`: returns 0 if no polys or x/z outside bbox; casts the vertical line
  (x,0,z)->(x,1,z) with `mgIntersectionPoint_line_poly3`, keeps the greatest y (start -1e8),
  writes `pos[1]`, returns 1 if any hit.
- `PickUpNearPoly(out, box, max)`: rejects if box and bbox do not overlap; for each poly whose
  MaxMin box `mgClipBox`es the query box, copies it out; stops when `max` reached.
  A guarded C++ draft now follows this search and compiles, but its generated
  code still differs from retail.

## CColFrame (0x120) : mgCFrame
- mgCFrame is 0x110 (`mgCFrame::operator=` memcpy 0x110). 0x110 `u32 flags`, 0x114
  `CCollision *collision`; 0x118-0x11F alignment padding.
- Size 0x120: `__nw(0x120)` in `mapFIX_CAMERA_RECT`, array stride 0x120 in
  `LoadCollisionFile` (`__construct_new_array(..., __ct__9CColFrameFv, 0, 0x120, n)`).
- Ctor: mgCFrame ctor, store vtable, VIRTUAL call to Initialize (vtable +0x3C).
- `Initialize`: flags = 1, collision = 0, then `mgCFrame::Initialize()`.
- Vtable `__vt__9CColFrame` 0x37B450 = mgCFrame's (0x37B1C0) with Initialize (+0x3C) and
  GetWorldBBox (+0x40) overridden and two new virtuals appended: +0x4C
  `Draw(mgCDrawManager*)`, +0x50 `Draw(u_int*, mgCDrawManager*)`, both `return 0`.
- `InsidePoint` / `PickUpNearPoly` are non-virtual (called directly by CMap, CMapPiece,
  CTreasureBoxManager, CSphida).
- flags (enum `ColFrameFlag`), from `PickUpNearPoly`: `flags == 2` -> return 0; bit 1 and
  collision -> query own geometry (box corners transformed by inverse LW matrix, results
  transformed back via pre_/trance_normal); bit 2 -> skip children; bit 4 (tested on `this`
  inside the child loop) -> skip each child. Only value ever written: 1 (Initialize). Bit 4
  meaning beyond "skip children" unknown (`COL_FRAME_FLAG_UNK_4`).
- mgCFrame fields used: 0x58 first child, 0x5C next sibling (child `+0x5C`), 0xF0 pointer to a
  0xB0-byte bbox object (`+0x80`/`+0x90` max/min read by GetWorldBBox; allocated in
  LoadCollisionFile then `SetBBox`). These belong in mg_frame.hpp.
- `GetWorldBBox`: own bbox (from 0xF0 transformed by LW matrix) only if collision && 0xF0;
  merges children's `GetWorldBBox` (virtual +0x40); returns nonzero if any.

## LoadCollisionFile(MDS_HEADER*, mgCMemory*) -> CColFrame*
MDS_HEADER (owned elsewhere, forward-declared): +8 frame count, frame records from +0x10,
stride 0x70: +0x08 name (`SetName`), +0x28 offset of MDT data from header (0 = none),
+0x2C parent index (<0 none), +0x30 4x4 matrix (`SetTransMatrix`). Returns null when count 0.
For frames with MDT data: `collision = CreateCollisionMDT(...)`, bbox from its bbox (zero
vectors if null), `this+0xF0 = new(Alloc 0xD) [0xB0]`, `SetBBox(max, min)`.
Callers: mdslist `pcpMDS_END` (MDS kinds 1 and 3), CSphida/CTreasureBoxManager
`SetCollisionModel`.

## CreateCollisionMDT(u_int*, mgCMemory*) -> CCollisionMDT*
Reads raw MDT words (no MDT struct declared here): word 4 = vertex table offset (16-byte
vertices), word 10 = primitive-list block offset (block +8 = list count, lists from +0x10),
word 14 = material table offset (stride 0x60). Each list: [flags, index count, material],
then index words, three per triangle. Returns null if any list has `(flags & 7) == 4` or
`flags & 0x100`, or if the poly Alloc fails (the CCollisionMDT is leaked). Ends with virtual
CreateBBox (+8).

## Drafting (job collision.1)
- Inline-emitted copies: the tail block 0x1489E0-0x148A80 (both `CColFrame::Draw`,
  `CCollisionMDT::Initialize`, `CCollision::Copy/CreateBBox/GetMaxY/Initialize`) are class-body
  inline virtuals, now defined in collision.hpp. Evidence: `CCollision::Initialize` is inlined
  into `CCollisionMDT::Initialize` and into the inlined MDT ctor in `CreateCollisionMDT`.
  Their INCLUDE_ASM stays: the compiler does not emit them here while the vtables are
  INCLUDE_RODATA (removing them breaks the link: editmap, mapload and the vtables reference them).
- Promotion limits: defining a class's key function (first non-inline virtual) makes MWCC emit
  that class's vtable and inline virtuals here, which shifts the tail (CCollision:
  `InsidePoint`; CColFrame: `Initialize`, +0x80 bytes). The CColFrame ctor cannot be promoted
  because it references `__vt__9CColFrame`, which the game build only has as `__DATA`.
  Promoted: `CColFrame::InsidePoint`, `CCollision::Intersection`, `CCollision::PickUpNearPoly`.
- `pre_trance_normal` / `trance_normal` are whole-function VU0 blocks.
  `pre_trance_normal` keeps the world matrix in vf10-vf13 across calls;
  `trance_normal` transforms three contiguous vertices by that matrix, writes
  them through three destination pointers, and writes their unnormalised
  cross-product normal. Guarded C++ drafts keep the matrix in a file-local
  array and compute the same three vertices and xyz normal. VU0 leaves the
  normal's w lane unspecified; the draft writes zero there. `pre_trance_normal`
  now uses the narrow inline VU0 exception: its four matrix loads match all
  0x14 retail bytes, and the `collision` object passes `check_objects.py` with
  98 resolved relocations.
  `trance_normal` also uses the narrow inline VU0 exception. It reads three
  contiguous input vectors from its first pointer, transforms them with the
  matrix retained in vf10-vf13, stores the three results through separate
  destination pointers, and computes their cross-product normal. Its 0x64
  retail bytes match objdiff exactly; the `collision` object passes
  `check_objects.py` with 98 resolved relocations.
- `CCollisionMDT::PickUpNearPoly` loads the query box (w = 1) into vf10/vf11 with two `lqc2`
  before the loop; nothing in this function reads them. The typed volatile view of the
  box's maximum bounds makes MWCC reload its X lane after the early overlap tests, as
  retail does. The function's 0x238 bytes match objdiff exactly, and the full collision
  object passes `check_objects.py` with 98 resolved relocations.
- MDT layout: `CreateCollisionMDT` reads `MDT_HEADER` (mg_dataset.hpp) vertex_ofs/faces_ofs/
  material_ofs; `MDT_FACES::prim_num`, records from `faces + 1`; `FACES_ID::face_num` is used as
  an INDEX count here (triangles = face_num / 3; the next record is `&index[face_num]`), so
  mg_dataset.hpp's "Number of faces" comment is likely wrong. Material attrs come from
  `MDT_MATERIAL_::diffuse[0..3]`. The 16-byte zeroing of CCPoly 0x40-0x4F suggests the original
  had a 16-byte attribute struct there (first game: union `info`/`attr`).
- `LoadCollisionFile`: object records start at `header + 1` (not `object_ofs`) with fixed stride
  0x70 (`MDTOBJ_HEADER`, not its `size`); `frame->Initialize()` is a virtual call; bounds alloc
  is `mgCFrame::BoundInfo` (0xB0). Alloc sizes follow `size/16 + 2` quadwords for placement new.
- `CColFrame::PickUpNearPoly` explicitly forms eight corners from the query box,
  transforms them into collision space, queries its own polygons, transforms the
  returned triangles back with `pre_trance_normal` and `trance_normal`, then
  recursively queries child frames while capacity remains. Its 0x290 bytes
  match objdiff exactly; the collision object passes `check_objects.py` with
  98 resolved relocations.
- Unsure drafts (DIFF): LoadCollisionFile matrix copy loop shape;
  CreateCollisionMDT vertex copies.

## Native loader comparison

`LoadCollisionFile` was checked with typed MDTOBJ record advancement, indexed
CColFrame access, and byte-indexed MDT file-offset lookup. This preserves the
retail 0x230-byte size and all call relocations, but the original header, record
cursor, memory allocator and frames receive different saved registers. The
matrix column-copy loop and the call order are unchanged. Naming a captured
object count, sharing or separating the frame index, and moving the original
header declaration did not close the register difference. The fallback remains
active; no unmatched trial is promoted.

## Native MDT constructor comparison

`CreateCollisionMDT` uses signed division for the index count divided by three;
`FACES_ID::face_num` is declared unsigned, so its signed interpretation must be
preserved in this query. The counted value represents vertex indices rather
than triangles. Typed FACES_ID record advancement locates the next record at
`&index[face_num]`. Serialized vertex, material and face tables are relative byte
offsets from the original MDT header.
The native candidate's CCollisionMDT construction has the same two base-bound
clears and vtable transitions, but placement-new null-test scheduling differs.
The counting pass and subsequent primitive walk also differ from retail.
A class-body CCollisionMDT::Initialize with its constructor calling Initialize
was tested and changed existing inline emission; it was reverted. Seeding helper
masks for integer argument registers and float argument registers did not change
the remaining native differences. The original constructor/header and assembly
fallback remain in place while these source-shape issues are unresolved.

## CColFrame caller preservation

`CColFrame::PickUpNearPoly` now uses upstream's exact native caller and its
processor-specific `pre_trance_normal` / `trance_normal` implementations.
Their visible register preservation resolves the earlier five-word caller
park: the hit-count/world-matrix argument order and caller-saved triangle
counter now follow retail. The earlier scalar-helper-interface measurements
do not describe this merged implementation.
