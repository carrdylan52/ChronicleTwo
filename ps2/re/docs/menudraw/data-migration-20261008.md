# Menu drawing data migration

The lane starts at `73f8129d` (checkpoint m22), with the canonical
`chronicletwo_dev:sf-63f7a9e` image. Its warm PAL build is byte-identical,
all 149 complete objects pass, and menudraw has 197 matched native
functions plus the unchanged guarded `CRepairManager::GeneratePoly`.
The refreshed baseline has 79 `INCLUDE_RODATA` markers, 93 `INCLUDE_BSS`
markers, and 4 / 5,741 `matched_data` bytes.

## State objects and extents

The existing header and [unit notes](notes.md) establish the exported
message forms, item pointers, gift-box state, creation-board coordinates,
menu layout manager and captured background texture. Their definitions
use the declared element counts: nine forms, 150 item pointers, three
pairs of gift-box item coordinates, five creation-board floats and 156
display-limit bytes. Local animation counters, texture pointers, colours,
positions and particle arrays have the widths used by their retail
consumers and LOCAL retail binding.

Retail symbol extents are distinct from marker reservations. In
particular, byte flags retain one byte, `use_trans_rect` and the boiled
fish counters retain two bytes, `MenuMainFrame_MoveRate_Cnt` and
`MenuWakuRotCnt` retain one float, `MenuPosData` retains one pointer,
`MakeBoardDrawInfo` retains five floats, and `CommonBoardDrawInfo`
retains its documented 0x2C-byte layout. Native BSS reservation handling
supplies the gap through the next canonical piece; no filler members,
enlarged arrays or fake objects are needed.

Each candidate is checked with a complete PAL build and
`check_objects.py`. SHA-256 comparison against the warm-build object
set additionally requires all 148 other units to stay unchanged.
Only successful candidates are retained.

## Existing compiler templates

The source already provides natural owners for the following markers.
A read-only comparison of the baseline source-only object confirms the
exact declared extents, initialized bytes where applicable, and every
incoming code reference's relocation kind and instruction operands.

| Marker | Native owner | Declared extent |
| --- | --- | --- |
| `at_2265` | `DrawMenuWakuStep`: `float move[6]` | 0x18 |
| `at_4494__2`, `at_4495` | `Func_MenuItemIconSetEffectOne`: `short first[10]`, `second[10]` | 0x14 each |
| `at_4442__2` | `Func_MenuItemBrdPosStep`: `int pos[2] = {0, 24}` | 0x8 |
| `at_2596__2` | Integer `GetPutPosXY` overload: `float pos[2]` | 0x8 |
| `at_3325` | `GetNextMovePos`: `int now[2]` | 0x8 |
| `at_3384` | `Menu3DivideTextureDraw`: `int size[2][4]` | 0x20 |
| `at_4496` | `Func_MenuItemIconSetEffectOne`: `short sparkle[10] = {0}` | 0x14 |
| `at_4727` through `at_4730` | `MenuFrameImageDraw`: four corner RGBA arrays | 0x4 each |

The four corner arrays hold actual `gray` and `alpha` components. The
last element of each `size` row is an implicit zero used for the final
step. Marker sizes of 0x20 for the six-float and ten-short templates
include alignment beyond the declared objects.

## Validation receipts

All receipts are under `.private/dataC-r3/` in the assigned worktree.
`warm-build.log`, `baseline-objects.log`, `baseline-progress.log`,
`baseline-coverage.txt`, `baseline-object-hashes.json` and
`before-metrics.json` establish the initial state. Per-candidate build
and object logs and the migration ledgers record accepted and rejected
steps. The source comparison's `matched_data` metric credits complete
aggregate sections, so it can remain unchanged while individual objects
are migrated.

## State migration checkpoint

All 71 named state reservations are replaced by native typed definitions.
`fish_boiled_effect_tex` is a `mgCTexture *`; `InitFishBoiledEffect` assigns
the texture directly and `DrawFishBoiledEffect` passes it without an
integer-to-pointer cast. Their instructions and relocations remain exact.
The 22 remaining BSS markers belong to compiler templates and the
source-compatibility exception discussed below. RODATA remains at 79
markers, and matched data remains 4 / 5,741 bytes because the aggregate
sections are still incomplete. Native function coverage stays 197 / 198.

`state-final-build.log`, `state-final-objects.log`, `state-progress.log`,
`state-coverage.txt`, `state-metrics.json` and
`state-final-summary.txt` record the passing PAL verifier, 149 / 149
complete objects, and unchanged hashes of all 148 other objects.
`bss-retry.log` records the successful rerun of a private-wrapper
interruption, rather than a source mismatch. `typed-fish-texture` receipts
validate the corrected texture type. The header and the complete guarded
`GeneratePoly` block are byte-for-byte unchanged as text.

`MenuCursorReverseFlag` retains its marker: retail's declared symbol is
one byte, while the shared header exposes `extern int`. Other units use
that declaration. Defining an int would lose the exact declared extent;
changing the public declaration would exceed this lane's source-
compatibility constraint. No unowned header or caller is edited.

## Named initialized tables

The numeric tables use native static definitions in retail order. The
spectrum tables retain 16 triples of short coordinate pairs and 16 rows
of six exact binary32 direction angles. The two item-icon rectangles and
the download-panel's 3-by-3 rectangles use the existing 8-byte
`MENU_SHORT_RECT`; their final two shorts are width and height. The
header's comments describe those dimensions without changing any name,
type, declaration, size or alignment.

The paint table retains nine integer RGBA rows, and the spectrum raster
retains 40 signed-byte offsets. Gift-box window texture coordinates retain
36 shorts. Material-board row rectangles retain two groups of three
four-short rectangles; the button glyph coordinates retain signed bytes,
as retail uses `lb`. The frame-mode count table retains eight floats, the
star colour table nine RGB components, and the fish-bounce rate table
three floats. Icon base offsets retain 23 pairs of shorts; language-group
movement offsets retain three groups of 18 pairs.

`MENU_ITEM_FRAME_PART` names the twelve rectangles constructed by
`MenuItemBrdFrameDraw`: the four corners and paired tiles for each edge.
The top/bottom and side index tables retain 16 and 10 short entries.
The repeated-part primitive table uses `MG_PRIM_SPRITE` and
`MG_PRIM_TRIANGLE_STRIP`, with their existing two- and four-vertex counts.
Level-up colours, gift-box/item-board icon colours, frame centre and main
icon coordinate arrays retain their documented component widths and
counts. Float literals preserve retail's actual approximations rather
than replacing them with ideal angles.

The Geostone text table contains seven pairs of string pointers. Its
native definition owns the exact download/completion literals, including
the `[UNI00e9]` notation, shared space string, and shared English strings.
Native pointer relocations identify each compiler-owned string without
manual pointer words, padding fields or separately named literal objects.

The table checkpoint removes 37 RODATA markers: 25 numeric tables and
the Geostone pointer table plus its eleven strings. Counts are now
42 RODATA / 22 BSS, with matched data still 4 / 5,741 and 197 / 198
native functions. `numeric-ledger.json` and `localization-result.json`
record every successful candidate. The final table receipts are
`tables-final-{build,objects}.log`, `tables-final-summary.txt`,
`tables-progress.log`, `tables-coverage.txt` and `tables-metrics.json`.
The rebuild after the header comment correction passes PAL and all 149
objects; all 148 other object hashes equal the baseline. The header diff
contains comments only.
