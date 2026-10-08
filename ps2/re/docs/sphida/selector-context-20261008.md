# Status-sprite context selectors

Base: `24d3d21`, image `chronicletwo_dev:sf-d8bf13c-proto`, canonical
MWCC 3.0-011126 flags and the existing GPR/FPR `0x30`/`0` history.
`DrawStatusSprite__7CSphidaFv` at `0x002EF390` draws the two language
layouts, par and pin-distance digits, spin marker, club label and simulated
carry-distance digits. The existing class, gauge, primitive, language and
club analysis is retained; fresh `decompile.sh` output is
`.private/ctxrows/sphida/m2c.c`. No dependent type is changed.

The isolated baseline reproduces **193/1096** differing words. The previous
eleven value-only rows reproduce **134/1096**, including the displaced
carry-digit call. Scoped condition/sibling rows restore every relocation
offset and reduce the function to **20/1096**, body `0x111C` in the retail
`0x1120` extent. The complete-wrapper check still fails only target bytes
at `0x002EF854`; all other unit functions, data and resolved relocations
pass (`0x3130` checked bytes, 439 relocations). No promotion or profile
rows are accepted for this function.

## Exact private candidate

Every row below has `translation_unit: sphida.cpp`,
`function: DrawStatusSprite__7CSphidaFv`, `value_type: binary32`, and
`callee: DPrimEnterSprite__FP11mgCDrawPrimiiiiffff`. The argument column
means `argument: {argument_index: N, value_type: binary32, value_bits: B}`.
Its formal indices are 0 = primitive, 1/2 = texture U/V, 3/4 = texture
width/height, and 5/6/7/8 = screen X/Y/width/height.

`JP` means `control: {kind: condition, values: [0]}`, the real
`LanguageCode == LANG_JAPANESE` body. `Any` omits `control`; the sibling
may still identify a call present in only one language branch. `First` means
`evaluate_first: true`. `Before N` means `evaluate_before: N` and
`evaluate_first: false`. Distinct rows select different floating values or
disjoint call/context identities; no broad policy is overridden by a
conflicting narrower policy. All argument markers name a sibling slot,
including Y=384 selected by width=58.

| Selected value / bits | Language context | Sibling argument / bits | Policy |
| --- | --- | --- | --- |
| 33 / `0x42040000` | Any | 5 = 438 / `0x43db0000` | First |
| 438 / `0x43db0000` | Any | 6 = 33 / `0x42040000` | First |
| 18 / `0x41900000` | Any | 5 = 438 / `0x43db0000` | First |
| 454 / `0x43e30000` | Any | 6 = 33 / `0x42040000` | First |
| 18 / `0x41900000` | Any | 5 = 454 / `0x43e30000` | First |
| 22 / `0x41b00000` | Any | 5 = 380 / `0x43be0000` | Before 7 |
| 60 / `0x42700000` | Any | 5 = 474 / `0x43ed0000` | First |
| 22 / `0x41b00000` | Any | 5 = 474 / `0x43ed0000` | First |
| 54 / `0x42580000` | JP | 6 = 304 / `0x43980000` | Before 6 |
| 16 / `0x41800000` | JP | 6 = 304 / `0x43980000` | Before 6 |
| 336 / `0x43a80000` | Any | 7 = 30 / `0x41f00000` | First |
| 16 / `0x41800000` | Any | 7 = 30 / `0x41f00000` | First |
| 16 / `0x41800000` | JP | 6 = 406.4 / `0x43cb3333` | Before 6 |
| 24 / `0x41c00000` | Any | 5 = 334 / `0x43a70000` | First |
| 34 / `0x42080000` | Any | 5 = 334 / `0x43a70000` | First |
| 20 / `0x41a00000` | Any | 5 = 430 / `0x43d70000` | Before 7 |
| 355 / `0x43b18000` | Any | 6 = 33 / `0x42040000` | First |
| 20 / `0x41a00000` | Any | 5 = 355 / `0x43b18000` | First |
| 371 / `0x43b98000` | Any | 6 = 33 / `0x42040000` | First |
| 20 / `0x41a00000` | Any | 5 = 371 / `0x43b98000` | First |
| 74 / `0x42940000` | Any | 5 = 442 / `0x43dd0000` | First |
| 22 / `0x41b00000` | Any | 5 = 442 / `0x43dd0000` | First |
| 34 / `0x42080000` | Any | 5 = 40 / `0x42200000` | First |
| 52 / `0x42500000` | Any | 6 = 382.4 / `0x43bf3333` | First |
| 16 / `0x41800000` | Any | 7 = 52 / `0x42500000` | First |
| 384 / `0x43c00000` | Any | 7 = 58 / `0x42680000` | First |
| 58 / `0x42680000` | Any | 6 = 384 / `0x43c00000` | First |
| 16 / `0x41800000` | JP | 6 = 384 / `0x43c00000` | First |
| 460 / `0x43e60000` | JP | 7 = 68 / `0x42880000` | First |
| 406.4 / `0x43cb3333` | Any | 7 = 68 / `0x42880000` | First |
| 68 / `0x42880000` | Any | 6 = 406.4 / `0x43cb3333` | First |
| 28 / `0x41e00000` | Any | 7 = 68 / `0x42880000` | First |
| 482 / `0x43f10000` | JP | 6 = 406.4 / `0x43cb3333` | First |
| 406.4 / `0x43cb3333` | Any | 5 = 482 / `0x43f10000` | First |
| 22 / `0x41b00000` | Any | 5 = 482 / `0x43f10000` | First |
| 382.4 / `0x43bf3333` | Any | 7 = 66 / `0x42840000` | First |

The profile has 36 rows. Exact JSON and compiled profile are retained in
`.private/ctxrows/sphida-20-rows.json` and
`.private/ctxrows/sphida/sprite-best-full/profile.json`. The table is the
portable evidence record; private paths are not staged.

## Remaining scheduling boundaries

| Retail call / source identity | Differing words | Retail materialization order |
| --- | ---: | --- |
| Japanese `(60, 384, 58, 16)` screen rectangle, call `+0x4F4` | 4 | X=60 precedes Y=384; the other-language call `+0xCA8` needs Y before X and matches. |
| Japanese `(460, 406.4, 68, 28)` gauge rectangle, call `+0x62C` | 6 | Width=68 precedes X=460 and Y=406.4; the other-language call `+0xDE4` begins with Y and matches. |
| Japanese `(482, 406.4, 22, 22)` distance suffix, call `+0x930` | 6 | Width=22 precedes X=482 and Y=406.4; the other-language call `+0x10E8` begins with Y and matches. |
| Other-language carry-digit call `+0x109C`, Y=406.4 and width=16 | 4 | Height=20 precedes width=16; the Japanese call `+0x8E8` needs width before Y and matches. |

The residual offsets are `+0x4C4`, `+0x4CC`, `+0x4E4`, `+0x4F0`;
`+0x5F8`, `+0x600`, `+0x608`, `+0x610`, `+0x618`, `+0x620`;
`+0x900`, `+0x908`, `+0x910`, `+0x918`, `+0x920`, `+0x928`;
and `+0x108C`, `+0x1090`, `+0x1094`, `+0x1098`.
The two suffix texture locations are already correct in source and retail:
Japanese `(234, 20)`, other languages `(178, 74)`.

The prototype reconstructs the equal-controlled Japanese body, not an
else/complement selector. Current sibling constants are identical at these
paired calls. Combining a broad early policy and a conflicting Japanese
ordinary policy is deliberately excluded from candidates. These observations
describe the tested identities, not a proof that every possible policy fails.
No occurrence, instruction address, source-line identity or invented nested
call is used.

## Bounded policy tests

The first context profile gives 36 words. Correcting the Japanese 52-pixel
label, shared 384-Y label, gauge rectangle and distance suffix reaches 24;
leaving only 382.4 early on the other-language 66-pixel label reaches 20.
The corresponding ledgers are `label52-ledger.json`,
`shared384-ledger.json`, `gauge68-ledger.json`, `end482-ledger.json`, and
`label66-ledger.json` under `.private/ctxrows/`.

The early-policy sweeps choose default, Japanese-only first or both-language
first for each eligible constant. The ordinary-walk suffix sweep compares
priorities before sibling slots 5 and 6. It does not improve the six-word
paired-call residual; cyclic dependencies fail compilation and are excluded.
Five explicit false-policy probes also fail to improve 20. Seven carry-digit
height/width priorities worsen the function to 34–56 words and displace the
Japanese call. The final 104 single/pair substitutions at integer setup
boundaries (before argument 0 or 4, Japanese-only or both languages) retain
a best 20. Receipts: `end482-ordinary-ledger.json`, `false-probes.log`,
`digit-probes.log`, `targeted-int-tasks.json`, and `targeted-int-probes.log`
under `.private/ctxrows/`.

All three new features have useful partial evidence: `argument` separates
many rectangles, `control` selects Japanese schedules, and `evaluate_before`
places the 380-X rectangle's height before its width and the Japanese
carry-digit width before Y without forcing it into the early phase.
The source remains unchanged and guarded. All partial rows remain private.

Complete-unit receipt: `.private/ctxrows/sphida/sprite-best-full/`.
Final clean guarded validation: `.private/ctxrows/receipts/final/`.

The final `CLEAN=1 JOBS=4` proto build passes `SCES_511.90: OK` and
149/149 complete objects. The production sphida object passes `0x3134`
bytes and 438 relocations; its linked and source-only SHA-256 hashes
equal the baseline. The sole promotion in this lane is FishModifyParam.

## Proto2 round-1 cardinality and carry-priority audit

Base `202d02d`, image `chronicletwo_dev:sf-d8bf13c-proto2`. All 36 private
rows above have source-derived `expected_matches` assertions. The full
wrapper reports **72 successful readbacks**, one per row in each compiler
pass. Most rows select one argument. The repeated identities are:

- 22 at screen X=474 and 24 at X=334 each select two equal-valued slots.
- Y=384 selected by width=58, and width=58 selected by Y=384, each select
  one slot in each language branch (two total).
- Gauge Y=406.4, width=68, and height=28 each select two language calls.
- Suffix Y=406.4 at X=482 selects two calls; suffix 22 at X=482 selects
  both width and height in both calls (**four total**).

The last count confirms the documented selector projection: the selected
formal slot is absent from the identity. A count assertion validates these
four arguments; it does not select a width-only subset. The Japanese
condition `[0]` remains the available language context. No complement/else
identity, source position, or conflicting broad/narrow policy is added.

The candidate remains **20/1096 words**, body `0x111C` in extent `0x1120`,
with matching relocation offsets. The single complete-unit problem is
target bytes at `0x002EF854`; `0x3130` bytes and 439 relocations are checked.
The four residual call regions and paired language schedules above are
unchanged.

Twenty new carry-digit priority probes use the real Y=406.4 sibling to
isolate the two carry calls. Each adds an exact one- or two-argument count
and uses either the Japanese context or both language branches. They keep
all other best policies fixed; width substitutions replace the previous
width row rather than creating competing policies. Their policy sets are
compared against the saved round-0 candidates before compilation.

| Selected value and priority | Both languages | Japanese only |
| --- | ---: | ---: |
| Height 20 before U/V/texture width (formal 1/2/3) | 45 words | 32 words |
| Height 20 before screen width (7) | 28 words | 32 words |
| Width 16 before U/V/texture width (1/2/3) | 34 words | 20 words |
| Width 16 before screen height (8) | 37 words | 37 words |
| Height 20 or width 16 before screen X (5) | Rejected | Rejected |

Sixteen probes compile. The four X-target probes fail because argument 5
does not participate in the ordinary walk, preserving proto2's validation
rule. No trial improves 20; several move the Japanese relocation and are
rejected as matching candidates. The shared width/height/Y identities and
the absent else context still limit disjoint language policies. This is a
bounded negative result, not an exhaustive impossibility claim.

The source and guard remain unchanged; no partial row is committed.
Receipts: `.private/ctxrows-r1/sphida/cardinal-best/`,
`.private/ctxrows-r1/sphida-best-rows.json`,
`.private/ctxrows-r1/carry-priority-ledger.json`, and per-probe profiles,
compiler logs and scores under `.private/ctxrows-r1/sphida/`.

## Proto2 round-1 final acceptance

The `CLEAN=1 JOBS=4` build passes all ten initialized PAL sections, the
`0x01F64A00` memory end, and `SCES_511.90: OK`. The complete checker passes
**149/149 units**. All **149 linked game-object hashes**, all **149
source-only object hashes**, and the complete linked ELF hash equal the
`202d02d` baseline. Game source and headers are unchanged.

Freshly regenerated `progress/report.json` and coverage retain **6,746
matched / 116 guarded / 10 assembly-only / zero fuzzy**. No function is
promoted in round 1; the only profile edits add count assertions to the
two already accepted FishModifyParam rows. Receipts:
`.private/ctxrows-r1/final/clean-build.log`, `check-objects.log`,
`coverage.txt`, `hashes.json`, `report.json`, and `comparison.json`.
