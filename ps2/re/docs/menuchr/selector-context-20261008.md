# Costume-draw selector boundary

The proto-image baseline at `24d3d21` confirms 71/568 differing words in
`Draw__15CMenuCostumeSelFv`, body `0x8D0` in retail extent `0x8E0`.
The existing camera, costume, rectangle, font and primitive analyses are
retained; fresh `decompile.sh` output is `.private/ctxrows/menuchr/m2c.c`.

The documented callee-only binary32 36.0f (`0x42100000`) evaluate-first row
for `DrawMenuFillBox__Fffffiiii` again gives **18/568 words**, body `0x8D4`.
The complete-wrapper probe checks `0x11CB8` bytes and 3,752 relocations,
with exactly one byte problem at `0x002C1C7C` (`+0x2AC`). All other unit
functions, data and resolved relocations pass.

The residual is still `+0x2AC..+0x2F8`: the absolute-wave merge's nop,
the right-origin integer-to-float conversion and the two arrow-shadow
coordinate preparations. The four-pixel constants are operands of
arithmetic expressions, rather than the direct constant arguments or
recognized constant assignment wrappers accepted by the consumer hook.
`control` and `argument` narrow existing call identities; they do not add
an arithmetic-operand identity. `evaluate_before` likewise needs an eligible
constant argument. The earlier rejected arithmetic selector and source
rewrites are not repeated.

The function remains guarded, source is unchanged, and **the partial 36.0f
row is not committed**. No new selector capability is required for its
already reproducible help-box schedule; the remainder needs natural source
expression/lifetime work outside this promotion-only task.

Exact row: `.private/ctxrows/menuchr-box-rows.json`.
Receipts: `.private/ctxrows/menuchr/baseline/`, `menuchr-box/`,
and `costume-best-full/`. Final guarded validation:
`.private/ctxrows/receipts/final/`.

The final `CLEAN=1 JOBS=4` proto build passes `SCES_511.90: OK` and
149/149 complete objects. The production menuchr object passes `0x11CC4`
bytes and 3,745 relocations; its linked and source-only SHA-256 hashes
equal the baseline. The sole promotion in this lane is FishModifyParam.
