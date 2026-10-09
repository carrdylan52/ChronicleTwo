# menuop data migration — 2026-10-08

## Baseline and preservation

Checkpoint `1695f2eb`: 153 `INCLUDE_RODATA` and 59 `INCLUDE_BSS` markers;
34 native functions and two guarded drafts. Refreshed data comparison is
4/3,027 bytes. The warm PAL build prints `SCES_511.90: OK`, and all 149
canonical objects pass. Receipts are under `.private/menuop-data-r4/`.

`MenuManualInit` and `CSaveMenuClass::KeyStep` remain byte-for-byte unchanged
as source, including their guards. Their external data identities are retained.
The existing `MenuMapInfoSave` byte-array header declaration remains compatible.
No build scripts, SF rows, unowned units, or generated files are edited.

## Named storage

The first step replaces 34 BSS markers with their exact declared types and
extents. The retail ELF gives one-byte movie/menu flags, two-byte dungeon and
phase values, four-byte pointers and volumes, a six-byte texture-block array,
32-byte picture storage, 52-byte save-row pointers, a 12-byte map snapshot,
and a 28-byte BGM snapshot. Their larger marker extents include alignment gaps;
no source filler or fake object accounts for these gaps. Existing build tooling
supplies verified reservation padding. All unit-local objects use `static`.
The globally visible map snapshot keeps its existing name and type.

`MnOnePictTex` is a real eight-entry `mgCTexture *` array. Its matched consumers
now use pointers directly rather than storing textures as integer addresses.
The pointer and integer representations occupy the same retail storage, and
every changed consumer retains its exact instructions and relocation targets.

After this step: 153 RODATA / 25 BSS markers.
`named-storage-build.log` prints `SCES_511.90: OK`;
`named-storage-objects.log` records 149/149 matching objects.

The refreshed metric after named storage is 288/3,027 matched data bytes
(`named-storage-metrics.json`). Only `menuop.cpp.o` changes its raw file hash.

## Native initializer storage

The manual-movie fade counter is a genuine function-local `static short`
initialized to zero, so MWCC emits its variable and one-time guard naturally.
The manual cursor and option cursor/size pairs use `{0, 0}` initializers, and
the option cursor velocity uses `{-48, 0}`. Their anonymous native templates
replace the assembly reservations without adding source globals.

The existing first-page, turned-page, and mini-game message-value arrays
already generate their own zero templates. The existing `streams[6]` initializer
generates the global/local pointer template at `at_1315__2`; its runtime local
pointers retain the original stores. The save time formatter's existing
function-local `space` pointer generates its own BSS storage and guard.
The native option/save constructors generate their virtual tables directly.
Only their corresponding markers are removed; the guarded manual constructor's
virtual table remains.

This step removes 11 BSS and four RODATA markers: 149 RODATA / 14 BSS remain.
`native-initializers-{build,objects}.log` proves PAL OK and 149/149 exact
objects. No guarded draft is edited.

## Menu lookup tables

The manual unlock table is 47 signed 16-bit event IDs, including its terminal
-1. The boss-area table is 21 map names followed by NULL; it remains exported
locally under `submap_table_1022`, the name used by the guarded manual init.
The seven-entry `dngmap_2627` table contains each dungeon's entrance map name.
Its exact declared size is 28 bytes, with the following zero word belonging
to alignment rather than an eighth entry. The scrollbar-name table contains
three pointers, with its following zero word likewise outside the object.
All table strings are literals in their initializers, including the scrollbar
strings shared with native manual/option layout calls.

The picture-page help width table contains eight language widths. The five
manual list message-class IDs retain their signed-byte representation. Both
option row counters have retail initial value 16.0f; initialization later
selects 14.0f or 16.0f according to language. These are mutable variables,
not folded constants.

Each table step passes the complete build and all 149 canonical objects:
`scalar-tables`, `boss-maps`, `dungeon-maps`, and `scrollbar-names` receipts.
After these steps: 110 RODATA / 14 BSS markers.

## Save resources and guarded consumers

`tbl_2023` is a two-pointer slot cursor action table in small initialized data.
`tp_2083` is a three-pointer visible-row action table in initialized data.
Its former four-entry source declaration included the alignment word, whereas
retail declares only 12 bytes. The native definitions retain both exact
normalized retail names so the guarded save-menu draft and retail fallback
resolve them without source changes. Their five action strings are now
initializer literals.

The nine-byte dungeon-location map conversion table keeps its seven mapped
IDs and two implicit zero entries under `conv_2316`; the larger 24-byte marker
was its declared extent plus alignment. No padding array is introduced.
The icon metadata is a local `SaveIconSet` aggregate initialized by the three
resource filenames. Each existing 0x28-byte `MC_ICON_DATA` entry contains a
32-byte filename, loaded-data pointer and size; omitted members initialize
the pointer and size to zero before the resource lookup fills them. The
retail anonymous 0x78-byte template is emitted by that initializer.

`save-map-conversion`, `slot-actions`, `row-actions`, and `save-icons`
receipts each show PAL OK and 149/149 exact objects. All nine initialized-data
markers are now removed. RODATA / BSS markers are 101 / 14, and the refreshed
data comparison is 732/3,027 bytes (`tables-complete-metrics.json`).

## Manual and option literals

Native manual drawing/playback/layout/cursor functions and option key/layout/
initialization functions now pass their archive filenames, texture names,
form names, action names, format strings and debug labels as literal arguments.
Repeated strings pool at their original retail addresses. Shift-JIS text uses
hexadecimal escapes, with no generated named literal stand-ins. The integer
component pair's fields are `x` and `y`, identifying their horizontal/vertical
purposes without changing its layout.

Shared manual-initialization strings retain their assembly markers and extern
declarations for the untouched guarded draft; native consumers use literals.
The compiler's native copies do not replace those guarded identities.
Each individual function step passes PAL and all 149 objects. Receipts are
`literal-manual-draw`, `literal-CManualMenu-{KeyStep,CalcTex,CalcCursorPosition}`,
`literal-CMenuOption-{KeyStep,CalcTex}`, `literal-MenuOptionInit`, and the
combined `manual-option-literals` check. RODATA / BSS markers are 65 / 14.
