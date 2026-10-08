# Remaining guarded drawing functions

The current MWCC 3.0-011126 Satan's Fiddle profile selects GPR helper mask
`0x30`, FPR mask `0` for `menudraw.cpp`. Measurements below use that profile,
canonical `-O3,p` flags, and relocation-masked instruction comparisons.
They supersede older draft percentages recorded with different compiler state.
All three functions retain their assembly fallbacks.

The [round-1](round1-20261008.md) and [round-2](round2-20261008.md) notes
give subsequent direct-access and allocation findings. The measurements
below include the earlier mid-day experiment ledger.

## CommonBoardDraw

`CommonBoardDraw__FPfRi` draws the material board's background, four material
rows, required and missing quantities, confirmation buttons, and quantity
controls. Its draft emits `0xDD8` bytes against the padded `0xDE0` retail
extent, with 29 of 888 words differing. The differences are integer saved
register assignments in the row background and material-line blocks, rather
than floating argument order.

Replacing the inherited `BoardLine` wrapper with direct typed indexing
increases the difference to 43 words. Widening the left-coordinate local's
scope does not alter that result; binding the line as a C++ reference also
leaves 43 words. These alternatives are not retained as closer drafts.
No floating-point profile selector is supported by the observed differences.

The blocker is natural source expression/lifetime and integer allocation.
Reconsider when compiler allocation evidence explains both coordinate and
line-pointer choices using direct typed access; the inherited helper wrapper
is not a basis for native promotion.

## CMenuPosDataForm::MenuFormDrawNormal

`MenuFormDrawNormal__16CMenuPosDataFormFiiffRi` dispatches visible form parts
by draw type, applies vibration and effects, reloads texture state, and emits
textured, numeric, filled, or memo primitives. Its draft emits `0xFE8` bytes
against the padded `0xFF0` retail extent. All 13 differing words are in the
integer block at offsets `0x43C` through `0x488`, after `GetNowPosRGBA`:
retail uses `v1/a2` where the draft uses `v0/v1`. The second texture-rectangle
copy in the number branch already matches.

Spelling the first rectangle copy as four assignments to its named fields
produces the same instructions and the same 13 differences. Removing the
inherited `PlusF`, `FormPart`, and `PartTex` wrappers in favor of an explicit
float cast, typed array indexing, and the named texture field initially gives
173 words. Round 1 combines genuine local lifetimes and the shared horizontal
float offset to recover 13 words with all three wrappers removed. That
direct-access draft is retained.

Retail `GetNowPosRGBA` explicitly returns zero or one. Its existing `int`
return declaration is supported; changing it to `void` is unsupported.
The signed shadow offset, unsigned vibration bytes, signed vibration counts,
colour bytes, and effect-array stride agree with the current header. There
is no evidenced shared-header change or float-order profile proposal.

The blocker is integer register allocation in this local copy block. Reconsider when
compiler allocation evidence explains the block without changing supported
field types or introducing source-level steering helpers.

## CRepairManager::GeneratePoly

`GeneratePoly__14CRepairManagerFPfi` constructs the repair action character,
loads and scales its model, positions and starts its motion, and adjusts
frame depth testing. Its draft emits `0x214` bytes against retail's `0x240`,
with 117 of 144 words differing. Independent canonical compilation and
section fixup report 16 complete-object problems, confined to this function.

At offset `0x60`, retail tests the placement allocator's `v0` result and
copies it into `s1` in the branch delay slot. Native construction copies first
and tests `s1`. Its base-constructor sequence also differs from retail's
inlined vtable/virtual-initialization sequence. This is the established
[placement-new park](../funcpoint/placement-new.md); no new constructor
spelling or shared-header experiment is warranted here. Reconsider after the
dedicated constructor/compiler investigation validates a natural solution
for the null branch and the base construction sequence.

## Mid-day lane at c79e57c

Coverage lists exactly these three guarded drafts, with 195 matched functions
and no asm-only functions in the 198-function unit. Canonical SF measurements
reproduce the 29/888, 13/1020 and 117/144 word differences above. The build
object passes with 0x14058 allocated bytes and 2,584 resolved relocations because
the three fallbacks remain active.

### Drawing expression experiments

All experiments use private source copies, the canonical compiler adapter and
profile, and the complete native function inventory. No drawing edit is retained.

| MenuFormDrawNormal expression/lifetime change | Differing words / 1020 |
| --- | --- |
| Remove only PlusF; cast the signed shadow offset to float | 21 |
| Remove only FormPart; index the typed parts array directly | 118 |
| Remove only PartTex; read the named texture member | 165 |
| Remove all three wrappers | 173 |
| Direct version with part, texture and texture-info locals declared outside the loop | 173 |
| Direct version fetching texture after texture-info lookup | 163 |
| Direct version with promoted integer shadow-offset locals | 173 |
| Direct version naming the typed parts-array base | 173 |
| Direct version binding the UV source as a const rectangle reference | 173 |
| Direct version binding the part as a reference | 173 |
| Direct version capturing draw type before texture-info lookup | 175 |
| Direct version advancing a typed base pointer by the element index | 174 |

The cast-only experiment adds two four-word FPR operand swaps in the shadow
x-coordinate calculations at +0x5AC and +0x880. The array and texture access
experiments also change the saved-register assignments across the draw-type
dispatch. The named array-base, reference, and promoted-offset forms do not
recover the inherited inline-call lowering. None resolves the original local
copy block's v1/a2 allocation. An unsupported return-type change to
GetNowPosRGBA is not used.

| CommonBoardDraw change after direct line-array indexing | Differing words / 888 |
| --- | --- |
| Scope row_top to its row loop | 43 |
| Scope row_bottom to its pass loop | 48 |
| Use a typed line-array base followed by element advancement | 213 |
| Give each pass loop its own index | 52 |
| Convert the material-row top before the first rectangle construction | 527 |
| Assign row_bottom in the second Vertex argument | 43 |
| Copy pass colors before converting the row coordinates | 63 |
| Select the material line after calculating its floating coordinates | 217 |

All board variants retain 0xDD8 bytes except the separate top-conversion
variant, which emits 0xDD0. The first background block still exchanges the
s1/s4 coordinate and color-pointer assignments. Direct line indexing also
exchanges s2/s3 in the material-row rectangle and coordinate code. These
are integer source/lifetime differences; nested floating-argument selectors
have no supported use here. The best retained draft remains 29/888.

GeneratePoly retains its placement-new and base-construction park. The
CActionChara constructor chain has no supported homogeneous inline array
initialization to replace in an owned header; the accepted shop/save array
technique supplies no new experiment for this function.

Private receipts: `.private/menuui/native-before/`,
`.private/menuui/form-*/`, `.private/menuui/board-*/`, and the three m2c
outputs `form.m2c.txt`, `board.m2c.txt`, and `repair.m2c.txt`.
