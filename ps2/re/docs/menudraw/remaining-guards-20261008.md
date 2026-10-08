# Remaining guarded drawing functions

The current MWCC 3.0-011126 Satan's Fiddle profile selects GPR helper mask
`0x30`, FPR mask `0` for `menudraw.cpp`. Measurements below use that profile,
canonical `-O3,p` flags, and relocation-masked instruction comparisons.
They supersede older draft percentages recorded with different compiler state.
All three functions retain their assembly fallbacks.

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
float cast, typed array indexing, and the named texture field increases the
difference to 173 words. Neither experiment is retained.

Retail `GetNowPosRGBA` explicitly returns zero or one. Its existing `int`
return declaration is supported; changing it to `void` is unsupported.
The signed shadow offset, unsigned vibration bytes, signed vibration counts,
colour bytes, and effect-array stride agree with the current header. There
is no evidenced shared-header change or float-order profile proposal.

The blocker is integer register allocation in this local copy block, plus
natural removal of the inherited wrappers before promotion. Reconsider when
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
