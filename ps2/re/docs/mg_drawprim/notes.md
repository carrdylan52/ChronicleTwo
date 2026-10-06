# mg_drawprim: reverse-engineering notes

## C++ status
All 51 functions have source coverage. 49 compile to retail, including four inline VU0 conversion
methods. Texture and mgCDrawManager::Draw remain guarded drafts. The draft-enabled unit compiles.

Classes: `mgCDrawPrim` (42 functions), `mgCDrawManager` (9 functions). No vtables, no statics,
no non-member functions. Only data: `at_369` (0x10 BSS, compiler-generated initializer template).
No first-game equivalent: the first game's headers have no draw-prim or draw-manager class.

Depends on `mg_drawenv.hpp` (`mgCDrawEnv`, by value; `mgRENDER_INFO`) and `mg_texture.hpp`
(`mgCTexture`, by value; `mgCTextureManager`). Both class size assertions hold with these headers.

## mgCDrawPrim (0x120)
Size: `MenuPrimFix` (menumain global, constructed with `__ct__11mgCDrawPrimFv`) has size 0x120;
mgEndFrame's stack object spans 0x120 bytes. Also embedded at `CMiniEffPrimMan+0x810` and treated
as the base of `CPreSprite` (prespr calls mgCDrawPrim methods on `this`, reads `this+0xdc`).

| Off | Field | Evidence |
|---|---|---|
| 0x00 | `mgCDrawManager *draw_manager` | Initialize: set to `&mgDrawManager` if NULL; Begin/Begin2 check `draw_manager->render_info` (+0x64) |
| 0x04 | `mgCMemory *memory` | Initialize (default `mgGetDataBuffer()`); Begin2 `stAllocTest(1)`, End2 `Alloc(used qw)` |
| 0x08 | `sceVif1Packet *vif_packet` | Initialize (default `mgVif1Packet`); Begin2 `sceVif1PkCall`, End2 `sceVif1PkTerminate` |
| 0x0C | `int detached` | Initialize sets 0. When 0, Begin2 calls the packet from the VIF1 packet and End2 writes a DMA RET tag (0x60000000). Never set non-zero in this unit; name is a description of the branch |
| 0x10 | `mgCDrawEnv draw_env` | ctor `__ct__10mgCDrawEnvFv(this+0x10)`, `Initialize(this+0x10,0)`, `SetZBuf`, copied by `__as__10mgCDrawEnvFR10mgCDrawEnv` into the packet (0x40 bytes). Inside: +0x10 TEST (prim+0x20), +0x20 ZBUF (prim+0x30, copied from `render_info+0xf40` in Begin2), +0x30 ALPHA (prim+0x40) |
| 0x50 | `sceGsPrim prim` | ctor stores 0x100 (FST) as one doubleword; Begin/BeginPrim2 set bits 0-2 (type); Shading bit 3, TextureMapEnable bit 4, FogEnable bit 5, AlphaBlendEnable bit 6, AntiAliasing bit 7; BeginDma copies the doubleword after the GIF tag; BeginPrim2(4 args) puts `prim & 0x7ff` in the GIF tag PRIM field |
| 0x58 | `mgCTexture texture` | ctor `__ct__10mgCTextureFv(this+0x58)`, Initialize `mgCTexture::Initialize`, Texture() copies a texture field by field (0x58..0xc4) then `mgCTexture::Bilinear(this+0x58, bilinear)`; texture +0x38 = TEX0, +0x40 = TEX1 (written with TEX0_1=0x06, TEX1_1=0x14). mgCTexture size 0x70 from `__construct_array(...,0x70,2)` in `__sinit_mglib_cpp` |
| 0xC8 | `int bilinear` | ctor 1; Bilinear() |
| 0xCC | `int z_mask` | ctor 1; ZMask(); Begin2 passes to `mgCDrawEnv::SetZBuf` (1 clears ZMSK, -1 sets it) |
| 0xD0 | `int disabled` | ctor/Initialize 1; Begin/Begin2 set 1, then 0 once memory, packet, manager and render info are present; End/End2 do nothing while set |
| 0xD4 | `u_long128 *packet_start` | Begin2: `stAllocTest(1)`, then `|= 0x20000000` (uncached); End2 claims `(write - packet_start)/16` qwords |
| 0xD8 | `u_long128 *packet_top` | Begin2 sets it to packet_start only if 0; never cleared in this unit (check other users before relying on it) |
| 0xDC | `u_long128 *write` | write cursor; every data function stores 1 qw and advances 0x10; DirectData advances `count*0x10` |
| 0xE0 | `u_long128 *dma_start` | BeginDma = write; EndDma: `(write-dma_start)/16 - 1` -> DMA tag qwc |
| 0xE4 | `u_long128 *direct_start` | BeginDma = dma_start; EndDma: `(write-direct_start)/16 - 1` -> VIF DIRECT count |
| 0xE8 | `u_int *giftag` | BeginDma/Begin2; EndDma writes `nloop | 0x8000` (EOP) |
| 0xEC | `u_int *dma_tag` | BeginDma: first word of the opening qw; EndDma `qwc | 0x10000000` (CNT) |
| 0xF0 | `u_int *direct_code` | BeginDma: word 3 of the opening qw; EndDma `count | 0x50000000` (VIF DIRECT) |
| 0xF4 | `unk_f4` | not referenced here |
| 0xF8 | `float q` | ctor/Begin/BeginPrim2 = 1.0f; Color puts its bits in RGBAQ high word |
| 0xFC | `int coord` | ctor 0; Coord(); GetOffset adds `mgScreenOffx/y << 4` only when 0 |
| 0x100 | `int packed` | BeginPrim2(int)=0, BeginPrim2(4 args)=1; EndPrim2 branches on it |
| 0x104 | `int nreg` | BeginPrim2(4 args) arg 4, GIF tag NREG; EndPrim2 divides the qw count by it for NLOOP |
| 0x108/0x10C | unk | not referenced here |
| 0x110/0x114 | `int offset_x/offset_y` | ctor 0; GetOffset adds them; written directly by `ClsMes::DrawFukidashi_sub` (`<< 4`) and `MenuMainInit` (0) -> public fields, 1/16 pixel units |
| 0x118/0x11C | unk | not referenced here |

### Packet format built
Begin2: `{DMA CNT qwc 7 | VIF DIRECT 7}`, GIF tag `0x8002, 0x10000000, 0xe` (EOP, NLOOP 2, NREG 1, A+D),
TEXFLUSH (0x3f), PRMODECONT (0x1a = 1), then the 0x40-byte mgCDrawEnv (4 A+D pairs).
BeginDma: `{DMA CNT | VIF DIRECT}` qw (filled by EndDma), GIF tag `0x8000, 0x10000000 (NREG 1), 0xe (A+D)`,
then PRIM. Data functions append A+D pairs: RGBAQ (1), UV (3), XYZ2 (5), any register via Direct(reg, data).
Texture(): TEXFLUSH, TEX1 (0x14), TEX0 (0x06). End2 (non-detached): DMA RET.

### Enums (values from code)
- Prim type (`mgPRIM_TYPE`): GS PRIM type 0-6; `Begin(6)` is used with two vertices per rectangle in mgEndFrame.
- `AlphaBlend(int)`: 1 -> 0x44 (Cs-Cd)*As+Cd; 2 -> 0x48 Cs*As+Cd; 3 -> 0x42 Cd-Cs*As;
  4 -> 0x80_0000002a (FIX 0x80, 0*..+Cs => source unchanged); 5 -> 0x80_00000068 Cs*FIX+Cd.
  Other values leave ALPHA unchanged. Names in `mgALPHA_BLEND` are descriptive.
- `DepthTest(int)`: always sets ZTE; 2 -> ZTST 3 (GREATER), 1 -> 2 (GEQUAL), -1 -> 1 (ALWAYS).
  `DepthTestEnable(0)` = ZTE 1 + ZTST ALWAYS; non-zero = `DepthTest(1)`.
- `ZMask`: 1 write, -1 masked (via `mgCDrawEnv::SetZBuf`).
- AlphaTest(method, ref): ATST bits 1-3 and AREF bits 4-11 of TEST. DAlphaTest(enable, mode): DATE bit 14, DATM bit 15.

### Signatures
- `Direct(unsigned long reg, unsigned long data)`: stores arg2 in the low doubleword and arg1 in the
  high one, i.e. arg1 is the A+D register address.
- `Vertex(float*)`/`Color(float*)`/`Data0`/`Data4` read a full quadword (lqc2), so the arrays are 4 floats.
- `Vertex(float,float,float)` builds `{x, y, z, <float const at 0x396eec>}` and calls `Vertex(float*)`.

## mgCDrawManager (0x80)
Size: `mgDrawManager` symbol size 0x80 (mglib). Constructed in `__sinit_mglib_cpp`.

| Off | Field | Evidence |
|---|---|---|
| 0x00 | `int *draw_order` | BeginDraw: copy of the caller's list, -1 terminated, else NULL; EndDraw picks group per index |
| 0x04 | `int *order_index` | BeginDraw: `group_max` entries set -1, then `order_index[order[i]] = i`; AddPacket/Draw/ReloadTexture map through it |
| 0x08 | `int group_max` | BeginDraw = `texture_manager+0xc`; AddPacket accepts `group < group_max` |
| 0x0C | `int group_num` | BeginDraw = `texture_manager+0xc`, or the order list length; loop bound of ClearTable/PreEndDraw/EndDraw |
| 0x10 | `mgSORT_PACKET ***packet_list` | BeginDraw allocs; PreEndDraw allocs per group `packet_num[g]` pointer arrays; Draw walks it from the end |
| 0x14 | `int *unk_14` | allocated in BeginDraw, zeroed in ClearTable, otherwise unused here |
| 0x18 | `int *packet_num` | AddPacket increments `packet_num[group]` |
| 0x1C | `int sort_num` | SetSortTable arg; ClearTable bound; BeginDraw `Alloc(sort_num)` for sort_table |
| 0x20 | `int sort_max` | `sort_num - 1` |
| 0x24 | `float sort_num_f` | `(float)sort_num` |
| 0x28/0x2C | `float near_clip/far_clip` | `render_info+0xe88` / `+0xe98` (defaults 1.0 / 2.0); these are the near/far params of `mgRENDER_INFO::SetRenderInfo` |
| 0x30 | `float clip_range` | far - near |
| 0x34-0x40 | unk | not referenced by any function found |
| 0x44/0x48/0x4c | `sort_near`, `sort_ratio`, `sort_scale` | near, near/far, sort_num_f. 0x40-0x4c looks like one quadword (VU constant?) with x unknown |
| 0x50 | `mgSORT_PACKET **sort_table` | AddPacket pushes onto `sort_table[0]`; PreEndDraw walks bucket 0 only (`for i < 1`) |
| 0x54 | `mgCMemory *memory` | BeginDraw arg, default `packet_memory` |
| 0x58 | `mgCTextureManager *texture_manager` | ReloadTexture calls `mgCTextureManager::ReloadTexture`; mgInit/mgBeginFrame store `&mgTexManager` at mgDrawManager+0x58 |
| 0x5c | `mgCMemory *packet_memory` | mgBeginPacket: `&packet_buf[mgDataID]` (0x30 stride = mgCMemory) |
| 0x60 | `mgCMemory *data_memory` | mgBeginPacket: `&data_buf[mgDataID]` (what `mgGetDataBuffer` returns) |
| 0x64 | `mgRENDER_INFO *render_info` | mgInit/mgBeginFrame store `&mgRenderInfo`; SetSortTable, mgCFrame::GetDrawRect, mgCDrawPrim::Begin2 (`+0xf40` ZBUF) |
| 0x68 | unk | ctor 0x40 |
| 0x6c/0x70 | unk | ctor 0 |
| 0x74 | `mgSORT_PACKET ***packet_cursor` | PreEndDraw: write cursor per group into packet_list |
| 0x78/0x7c | unk | not referenced |

## mgSORT_PACKET (0x10, name invented)
AddPacket allocates 1 qw: +0 common (arg 2), +4 packet (arg 3), +8 next (sort bucket list),
+0xc s16 group (position in draw order), +0xe s16 vu_program (arg 4). Draw: for each entry calls
`mgSendVuProg(dst, vu_program)`, emits a DMA CALL (0x50000000) to `common` when it differs from the
previous entry's, then a DMA CALL to `packet`.

## Mangling
`AddPacket__14mgCDrawManagerFiP1P1i`: `P1` is `u_long128*` (same as mg_memory's `FP1i`), so the
signature is `(int, u_long128*, u_long128*, int)`. The index's demangling "(int, P*, i)" is wrong.

## Unresolved
- EndDraw: retail leaves the group uninitialised when `draw_order` is NULL; see the instruction evidence below.
- `detached` (0x0C) is never set non-zero here; find the writer to confirm the meaning.
- Return values of ReloadTexture/Draw are 0/1 (int).
- `sceGsPrim` comes from `sce/libgraph.h` through `mg_drawenv.hpp`. Including `sce/libgraph.h`
  directly as well as `mg_drawenv.hpp` made MWCC redeclare `sceGifTag` (its `#pragma once` did not
  hold), so the header relies on `mg_drawenv.hpp` for it.
- The header compiles with the real texture and draw-environment declarations, and both size assertions hold.

## Drafting (job mg_drawprim.1)
- `AlphaBlend(int)` is a tail call to `mgCDrawEnv::SetAlpha(mode)` on `draw_env`; the switch
  Ghidra shows is SetAlpha inlined by the decompiler. `mgALPHA_BLEND` values equal `mgAlphaMacroID`.
- `render_info+0xe88/+0xe98` (SetSortTable) are `clip_min[2]`/`clip_max[2]`; `+0xf40` (Begin2)
  is `draw_env[0].zbuf`.
- `Vertex(float,float,float)` builds `{x, y, z, 0}` from the compiler's zero template `at_369`
  (BSS, 0x10); the promoted function binds to it.
- EndDraw: when `draw_order` is NULL the group passed to ReloadTexture/Draw is never assigned
  (asm confirms `$s3` is only loaded inside `if (draw_order)`): retail uses an uninitialised value.
- MWCC rejects the SDK's anonymous bitfield structs (`sceGsTest::ATE` etc.); use `.bits.<lower>`.
  The anonymous union member `value` works.
- The source includes `mglib.hpp` for the VIF packet, screen offsets, draw manager and packet helpers.
- The header now includes `<libpkt.h>` (not `"sce/libpkt.h"`), which otherwise redefines
  `sceVif1Packet` next to mglib's include.
- New enums `mgPACKET_CODE` (DMA tag IDs CNT/CALL/RET, VIF DIRECT, GIF tag EOP/PRE/field shifts,
  uncached bit) and `mgGS_CODE` (PRMODECONT, ZTST GREATER, PRIM FST) name constants the SDK shim lacks.
- Data0/Data4/Vertex(float*)/Color(float*) match their inline VU0 conversions.
  BeginDraw allocates `group_num/4+1` table quadwords without an order list and `(count+1)/4+1` with one.

## Drafting (job mg_drawprim.2)
- AddPacket: the `group < group_max` test is on the caller's group, before the order lookup; with
  no `order_index` a negative group is stored as is (no lower bound check). Draft logic agrees
  with the assembly. The matching source keeps the caller's group and resolved index separate.
- The source uses the owning `mglib.hpp` declarations.

## Inline VU0 matching status
Four methods use inline VU0 blocks and match retail:
- Data0: `lqc2 vf1`, `vftoi0.xyzw vf1`, `sqc2 vf1`.
- Data4: `lqc2 vf1`, `vftoi4.xyzw vf1`, `sqc2 vf1`.
- Vertex(float*): `lqc2 vf10`, `vftoi4.xy vf10`, `vftoi0.z vf10`, `sqc2 vf10`.
- Color(float*): `lqc2 vf10`, `vftoi0.xyzw vf10`, `sqc2 vf10`.

Scalar C++ casts emit scalar conversion instructions and different rounding and register traffic.
The SDK's vector conversion helpers are external calls and cannot reproduce these inline sequences
or Vertex's mixed component conversions.
