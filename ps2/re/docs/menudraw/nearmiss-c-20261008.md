# Material-board near-miss assessment

Base `3d49d02`, image `chronicletwo_dev:sf-d8bf13c`, canonical MWCC
3.0-011126 and the existing menudraw Satan's Fiddle profile.
`CommonBoardDraw` remains guarded at **29/888** relocation-masked words,
body `0xDD8` in extent `0xDE0`. No source, header, or profile edit is retained.

Fresh `decompile.sh`/m2c output confirms the documented background passes,
four material rows, counts, buttons, and quantity controls. The existing
16 coordinate/UV-pointer exchanges and 13 material-line/top-coordinate
exchanges remain the entire inherited residual. The inherited `BoardLine`
wrapper is not a basis for promotion; direct typed line indexing has
43 differing words before this wave's changes.

## New hypotheses

Retail identifies `CommonBoardDrawInfo`, `Tex_CommonBoard`, and
`make_object_husoku_number_blink` as LOCAL objects. Private static source
definitions, separately and together, leave 29 words with the inherited
wrapper and 43 with direct line indexing. There is no linkage-driven
allocation correction analogous to the already promoted form drawer.

| New variant | Inherited words / 888 | Direct-index words / 888 |
| --- | ---: | ---: |
| Each local-data linkage correction, or all three | 29 | 43 |
| Unsigned 32-bit background-row counter | 30 | 44 |
| Unsigned pass counter, with either row-counter type | Body exceeds retail | Body exceeds retail |
| Unsigned 32-bit pass-colour or button-colour channels | 29 | 43 |

The row counter's extra mismatch is the signed/unsigned loop comparison.
The byte UV coordinates, byte line-kind/button selectors, and signed
halfword material counts already agree with retail load widths; their
existing declarations are preserved.

New direct-index probes move the three used material pixel-coordinate
declarations before line selection in all six orders. Each gives 52 words.
Moving only the line or board-UV pointer declaration immediately before
the material loop or before the line-width computation preserves 43.
An actual board-info reference or reference to its four-line array gives
872 words at function scope and 577 immediately before the material loop.
These aliases alter address/induction code and do not resolve the retained
saved-register exchanges. Earlier row/UV-segment alias probes are not
repeated.

The guard's reconsideration trigger remains a natural direct-index form
that explains both saved-register exchanges. None of these measurements
supports a new floating-argument selector, helper, or type overlay.

## Receipts

Receipts under `.private/nearmiss-c/` include fresh
`CommonBoardDraw__FPfRi.m2c.txt`, `menudraw-before/`, `width-trials.log`,
`board-linkage.log`, `board-order.log`, `ledger.jsonl`, and the per-case
source, object, inventory and instruction comparisons. The baseline
menudraw complete object passes with `0x14050` allocated bytes and
2,598 resolved relocations; all-object and PAL receipts are
`baseline-objects.log` and `baseline-build.log`.
