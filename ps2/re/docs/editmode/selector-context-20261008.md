# EditMode selector and hygiene boundary

On `24d3d21` with the proto image and canonical flags, the isolated native
`EditMode__FP6CScene` baseline is 87/1912 words, body `0x1DDC` in retail
extent `0x1DE0`. The existing editor/camera/collision type analysis is
retained; fresh m2c output is `.private/ctxrows/editmode/m2c.c`.

The documented callee-scoped binary32 quarter-turn tolerance
`0x3f490fdb` for `mgAngleCmp__Ffff`, evaluated first, again reaches
**74/1912 words**. All three angle-call materializations match. The
remaining axis arithmetic exchanges f22/f24, the `GetGeoCheckCamCol`
setup exchanges two integer argument preparations at `+0xB68/+0xB70`,
and the ground-query region begins diverging at `+0xD2C`.

Those regions contain integer/vector loads, stores, field assignments and
nonconstant arguments. The nested collision-query argument to `CheckHit`
has no floating constant, so neither sibling-argument nor control selectors
add an eligible source identity there. Ordinary-walk priority operates on
call arguments, not the separate axis or vector-assignment statements.
The already rejected box/pointer source trials are not repeated.

Promotion hygiene is an independent constraint: the inherited draft has
unused floats, casts two separate vector locals to `mgVu0FBOX`, and walks
the collision buffer with `next_poly += added`. These require natural typed
source work before any future promotion. This assignment permits promotion
of the existing function, not a new box/loop rewrite; no source or header
change is made.

The full wrapper has exactly the target byte problem at `0x002DF805`;
all sibling functions, data and resolved relocations pass (`0x637C` bytes,
1,400 relocations). The guard remains and **the partial row is not committed**.
`LoadEditCursor` is outside the assigned function and is not retried.

Exact row: `.private/ctxrows/editmode-tolerance-rows.json`.
Receipts: `.private/ctxrows/editmode/baseline/`, `editmode-tolerance/`,
and `editor-best-full/`. Final guarded validation:
`.private/ctxrows/receipts/final/`.

The final `CLEAN=1 JOBS=4` proto build passes `SCES_511.90: OK` and
149/149 complete objects. The production editmode object passes `0x6380`
bytes and 1,282 relocations; its linked and source-only SHA-256 hashes
equal the baseline. The sole promotion in this lane is FishModifyParam.
