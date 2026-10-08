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

## CDngFreeMap::Draw

`Draw__11CDngFreeMapFv` at `0x1EF730` draws the background, room map, player,
queued room marks, and optional debug readout. The native body matches all
388 words of its 0x610-byte retail extent, including eight bytes of padding.
With both drawing functions native, the canonical dngmenu object passes
0x8BC0 checked bytes and 1,187 relocations. No compiler-profile row is added.

The texture manager and map texture are retained before converting opacity;
the missing texture shares the function exit. Each queued mark constructs
its source rectangle as an argument temporary. Debug drawing retains a
`CFont` pointer to the local menu font, while the first print uses the local
object directly. That first print reads positions from stack offsets 0x114
and 0x118; subsequent prints use offsets 0x94 and 0x98 from the saved font
pointer. Selected-room and floor-save work uses positive nested conditions,
preserving retail's direct branches to the common exit.

The debug panel has a starting Y of 110, retained in `s1`. Room description,
next-floor IDs, root list, and visit count are positioned at Y + 2, +22, +102,
and +122. The flag list advances that Y by 142, then by 20 per row. Replacing
this live base with unrelated absolute positions frees a register too early
and shrinks the frame from retail's 0x280 to 0x270. Restoring the coordinate
relationship reproduces the saved registers and all addressable local slots:
font at 0x80, 256-byte detail text at 0x140, 32-byte line text at 0x240, mark
rectangle temporary at 0x260, and texture-block cache at 0x27C.

The four next-floor queries execute in normal/sun/moon/star order. The first
three have named results; the fourth remains the final sprintf argument.
The locals are declared moon, sun, normal before constructing the font and
assigned in call order. This changes four differing words to zero by
assigning normal to `s2`, sun to `s4`, and moon to `s6`. Declaring them in call
order or at function entry leaves normal and moon exchanged. The existing
dngfloor documentation establishes these as passage-type queries, not grid
direction queries; the named `DNGMAP_ROOT_TYPE` constants follow the retail
debug labels and keep the numeric query values 0 through 3.

ON and OFF use separate strcat calls. A conditional string argument factors
those calls and removes four instructions, shifting the final flag loop and
epilogue. The heading and root-prefix literals preserve their exact retail
Shift-JIS bytes. The final unused `strcat(detail, "NONE")` is retained because
retail executes it after the flag loop.

Progression after restoring the live Y coordinate: 63/388 differing words,
then 7 with separate ON/OFF branches, 4 with direct first-font access, and
zero with the query-result declaration order. Relevant private receipts are
`FreeMapDraw.m2c.c`, `free-query-decls-reversed/`,
`free-exact-canonical.log`, and `free-exact-style.log`; the latter also checks
the named passage constants and inline string bytes.

Actual-source receipts `free-build.log`, `free-objects.log`,
`free-compare.log`/`.json`, and `free-coverage.log` confirm the two drawing
promotions together. The object checker remains 147/149; dngmenu passes
0x8BC0 bytes and 1,187 relocations. Only dngmenu differs from the 149 saved
object fingerprints. Every allocated PAL section is identical to the base,
with the same 0x26 retail `.text` discrepancy and BSS end at 0x1F64A00.
Coverage is 6,738 matched / 122 guarded / 10 asm-only / 2 fuzzy functions.
