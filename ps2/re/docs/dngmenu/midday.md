# October 8 midday matching

Base: `fe60604`, with upstream `d8bf13c` already merged. The container image is
`chronicletwo_dev:sf-d8bf13c`, with canonical optimization and the existing
Satan's Fiddle profile. The starting coverage is 6,736 matched, 124 guarded,
10 asm-only, and 2 fuzzy functions. The object checker passes 147/149 units;
`nd_meswin::DrawMesWin` and `actscript::_SHOT` account for the unchanged PAL
`.text` discrepancy of 0x26 bytes. No assembly or shared header is changed.

## CMenuTreeMap::Draw

`Draw__12CMenuTreeMapFv` at `0x1F2D10` draws the floor map, animated save
prompt, selected-floor information and medal count, cursor, money board, and
jump question. Its retail extent is 0x730 bytes, including eight bytes of
alignment padding. The native body matches all 460 words with no new profile
row. The complete canonical unit passes: 0x8BC8 checked bytes and 1,182
relocations. The shorter checked-byte total excludes alignment padding that
the next function's alignment supplies; it is not missing retail instructions.

The previously unused numeral alpha is observable: after constructing
`CMenuFont`, the caller assigns `font.alpha`. Its default is zero; when the
information texture exists, the caller reloads twice the current backdrop
alpha after `GetNumberKeta`. The money digits also receive a second white
color setup after setting their source rectangle. Both behaviors are present
in the retail calls and stores and are restored in the native source.

The texture-manager pointer remains live across the drawing calls. Numeral
position and alpha defaults precede the medal lookup. The animated prompt
uses the existing `SetMovePosGyou` inline interface, retaining its X copy as
well as its Y and enable stores. Writing the wrap comparison as
`3.1415927f <= TreeMapSaveHopCount` preserves retail's ordered comparison,
store, self move, and branch sequence; the opposite operand spelling produces
a shorter inverted comparison in this compiler. Cursor opacity uses separate
mode-1 and mode-2 branches.

Retail's 0x1B0-byte frame has the following addressable locals:

| Offset | Purpose |
|---|---|
| 0x80 | Menu font, including numeral alpha at 0x110 |
| 0x140 | 32-byte numeral string |
| 0x160 | Named money-digit source rectangle |
| 0x170 | Medal-board shadow argument temporary |
| 0x180 | Medal-board foreground argument temporary |
| 0x190 | Money-board argument temporary |
| 0x1A8 / 0x1AC | Selected-cell cursor coordinates |

The font and numeral buffer precede a named digit rectangle; the three board
rectangles are separate value temporaries. A 64-byte numeral buffer adds
0x20 to the frame. Named medal-board rectangles occupy early slots and move
the font; making the digit rectangle another argument temporary changes both
its slot and the order of its Set and Color calls. A two-float coordinate
array supplies the final pair of slots. `DngAskMessageDrawFlag` is a signed
byte: retail's final equality check uses `lb`.

Diagnostic progression: 446/460 words initially, 355 after restoring the
missing rendering behavior, 37 after the wrap comparison correction, 25 after
the cursor branches and signed flag, 17 with the 32-byte numeral buffer, and
zero with the named digit rectangle. The guard is removed only after the
complete canonical object check. Existing documentation for `CFont`,
`CMenuFont`, `mgRect<int>`, message positioning, and floor projection supplies
the dependency types; none needs a shared type change.

Receipts are private under `.private/dnginv-midday/`: `TreeDraw.m2c.c`,
`tree-draw-number32-local/`, `tree-draw-exact-canonical.log`, and
`draw-build.log`. The actual-source complete object and baseline comparison
receipts accompany the final lane validation.

Actual-source validation: `draw-objects.log` passes dngmenu and retains the
same two failures (147/149 overall). `draw-compare.log` and `draw-compare.json`
compare allocated section bytes and relocation identities for all 149 saved
baseline objects: only dngmenu changes. All allocated sections of the linked
PAL ELF, including BSS address and size, are byte-identical to the baseline.
`draw-coverage.log`, refreshed through ctx/objdiff and the progress report,
records 6,737 matched / 123 guarded / 10 asm-only / 2 fuzzy functions.
