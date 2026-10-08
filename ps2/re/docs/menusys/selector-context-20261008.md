# Debug-panel context and ordinary-walk selectors

`MenuItemDebugDraw__Fv` at `0x002494E0` draws the debug item grid and the
selected character/weapon information page. The established menu/font/item
types remain unchanged. `decompile.sh` again encounters the documented
jump-table limitation; the source and retail case labels provide the
already analyzed page identities. Base: `24d3d21`, proto image, canonical
flags and existing GPR/FPR `0x30`/`0` history.

The isolated baseline is 32/1260 words, body `0x13A4` in extent `0x13B0`.
Seven private binary32 rows for `DrawMenuFillBox__Fffffiiii` reproduce every
floating argument schedule:

| Value / bits | Source identity | Policy |
| --- | --- | --- |
| 200 / `0x43480000` | Switch `[2]`, sibling argument 1 = 330 (`0x43a50000`) | `evaluate_first: false`, `evaluate_before: 2` |
| 60 / `0x42700000` | Switch `[4]` | Evaluate first |
| 60 / `0x42700000` | Switch `[6]` | Evaluate first |
| 230 / `0x43660000` | Switch `[6]` | Evaluate first |
| 260 / `0x43820000` | Switch `[6]` | Evaluate first |
| 230 / `0x43660000` | Switch `[7]` | Evaluate first |
| 260 / `0x43820000` | Switch `[7]` | Evaluate first |

The 330-Y sibling separates page 2's lower panel from its other fill calls.
Height 200 must precede width argument 2 in the ordinary walk while leaving
20-X and 330-Y first. Boolean early evaluation moves height ahead of those
coordinates and leaves seven new setup differences. The ordinary policy
removes them. Pages 4, 6 and 7 need different combinations despite sharing
their 236-X, 60-Y and 230-width literals. All rows have unambiguous real
source identities.

The best result is **8/1260 words**, with unchanged body size and matching
relocation offsets. One word at `+0x190` reverses the operands of the
commutative item-index addition. Seven at `+0xD4C..+0xD74` exchange the
three-name loop's s1/s2 counter and four-byte stride. No floating argument
is consumed in either residual region. The prior source-form trials are
not repeated under this promotion-only assignment.

The function remains guarded and **none of the partial rows is committed**.
The complete-wrapper probe checks `0x1B0D0` bytes and 5,905 relocations,
with only target bytes failing at `0x00249672`. All other unit bytes and
resolved relocations pass.
The source and all other menu functions remain unchanged. This is additional
evidence for all three prototype features: `control`, `argument`, and
`evaluate_before`, rather than a native promotion.

Receipts: `.private/ctxrows/menusys/m2c.c`,
`.private/ctxrows/menusys/baseline/`, `menusys-context/`,
`menusys-ordinary/`, and `menusys-best-full/`.
Exact candidates: `.private/ctxrows/menusys-ordinary-rows.json`.
Final guarded validation: `.private/ctxrows/receipts/final/`.

The final `CLEAN=1 JOBS=4` proto build passes `SCES_511.90: OK` and
149/149 complete objects. The production menusys object passes `0x1B0DC`
bytes and 5,872 relocations; its linked and source-only SHA-256 hashes
equal the baseline. The sole promotion in this lane is FishModifyParam.
