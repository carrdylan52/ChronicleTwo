# Title phase selectors

On `24d3d21` with `chronicletwo_dev:sf-d8bf13c-proto`, the isolated native
`TitleModeKey__Fv` baseline has 379/624 positional word differences because
of the previously documented four-byte contraction. The old private
zero/128 rows give 15/624. The existing m2c analysis and documented menu,
phase, card-manager and fade types are retained; the fresh m2c receipt is
`.private/ctxrows/title/m2c.c`.

Five disjoint binary32 rows for `CalcMenuAdd__FPfff` fix every fade call:

| Value / bits | Switch values | Policy |
| --- | --- | --- |
| 0 / `0x00000000` | `[0]` (push-start) | Evaluate first |
| 0 / `0x00000000` | `[1]` (menu) | Evaluate first |
| 128 / `0x43000000` | `[1]` | Evaluate first |
| 128 / `0x43000000` | `[10]` (extras menu) | Evaluate first |
| -8 / `0xc1000000` | `[10]` | Evaluate first |

Phase 0's 8/128 title-alpha call retains the default schedule, while phase
10's -8/zero menu-alpha call evaluates -8 first. The selectors reproduce
their differing real phase contexts without overriding one broad row with
a conflicting narrower row. No sibling-argument selector or ordinary-walk
priority is needed.

The best result is **5/624 words**, body `0x9BC` in extent `0x9C0`, with
all relocation offsets restored. The only remaining words are
`+0xC8`, `+0xCC`, `+0x188`, `+0x198`, and `+0x1AC`: the two captured card
bytes exchange s1/s2, including the second narrowing's reused register.
These are integer loads, masks and comparisons; there is no floating call
argument at those sites for the selectors to choose. Source lifetime/type
experiments already recorded in notes are not repeated.

The complete-wrapper probe has one problem, target bytes at `0x002A521A`;
all other unit bytes and relocations pass (`0x68B4` bytes, 2,200 relocations).
The function remains guarded and **none of these partial rows is committed**.
The source is unchanged. `TitleBootInit` is not attempted.

Exact candidate rows: `.private/ctxrows/title-context-rows.json`.
Source-only receipt: `.private/ctxrows/title/title-context/`.
Complete-unit receipt: `.private/ctxrows/title/title-best-full/`.
Final clean guarded validation: `.private/ctxrows/receipts/final/`.

The final `CLEAN=1 JOBS=4` proto build passes `SCES_511.90: OK` and
149/149 complete objects. The production title object passes `0x68B8`
bytes and 2,081 relocations; its linked and source-only SHA-256 hashes
equal the baseline. The sole promotion in this lane is FishModifyParam.
