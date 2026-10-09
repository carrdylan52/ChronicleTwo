# October 8 data migration

Baseline: `7c7edc2f`, MWCC 3.0/Satan's Fiddle through
`chronicletwo_dev:sf-63f7a9e`. Warm build verifies `SCES_511.90: OK`;
the canonical checker passes 149/149 objects.

The initial source contains 99 `INCLUDE_RODATA` and 42 `INCLUDE_BSS`
markers. Refreshed objdiff data coverage is 4 / 3159 bytes.

## Menu state

The named menu pointers, drawing flags, counters, save-prompt state, and
eight message-window pointers now use their existing documented native
types. The declarations retain byte and halfword widths and retail linkage:
only `TreeMapSaveFlag`, `TreeMapSaveNum`, `TreeMapCallDungeonSubMap`, and
`TreeMapCalledWorldMap` are externally visible. Other definitions are static.

`DngInfoDrawAlpha` is four bytes despite its eight-byte reservation.
Byte and halfword reservations similarly include alignment owned by the
postprocessor; definitions do not represent that padding as extra fields.
Guarded function bodies and declarations are unchanged.

This step removes 29 BSS markers (99 / 13 remain). Data coverage remains
4 / 3159 because anonymous seeds still make the aggregate sections incomplete.

Receipts: `.private/nminv-r2/warm-build.log`, `warm-objects.log`,
`dng-state-build.log`, and `dng-state-objects.log`.

The seal pulse phase and the tree-step direction, previous cell, destination
cell, and four signed-byte initialization flags also have typed static
storage. They retain their retail symbol spellings because guarded assembly
refers to those symbols directly. This removes eight more BSS markers;
99 / 5 remain. `dng-local-state-build.log` and
`dng-local-state-objects.log` confirm the full PAL and 149/149 objects.

## Naturally emitted literals and vtable

The four texture names in `SetTextureInfo`, fourteen debug strings in
`CDngFreeMap::Draw`, four special map destinations, two tree-opening
filenames, the tree cursor name, and the tree-menu vtable now come solely
from their existing C++ definitions. No function body changes are needed.
Each function's marker group and the vtable were built and checked
separately; all six receipts report PAL OK and 149/149 objects.

Markers are now 73 / 5, with
matched data 36 / 3159 bytes. Receipts use
`dng-textures`, `dng-debug-literals`, `dng-jump-literals`,
`dng-opening-literals`, `dng-cursor-literal`, and `dng-vtable` prefixes
under `.private/nminv-r2/`.

## Drawing tables

The passage mark offsets, passage icon coordinates, flat visited-room glyph
rectangles and their destination offsets, special dungeon-six mark offset,
mark animation speeds, dungeon number, player position, floor limits, and
two floor-frame texture tables now have typed native definitions.
Their declared extents match retail, including the five icon coordinate
rows, sixteen glyph halfwords, and seven floor-limit bytes. The flat glyph
layout preserves the matched shared induction in `DrawRoomOne`.

Each of these eleven definitions passes a separate full build and 149/149
object check; receipts use `dng-table-<symbol>` prefixes. The first private
mark-offset trial placed its definition before its source-local row type;
placing definitions at their existing declaration sites resolves that
compile-only issue. No unmatched candidate is retained.

Markers: 62 / 5; matched data: 36 / 3159 bytes.

## Event movement tables

The ten passage paths contain 21 signed-halfword coordinate pairs each,
and the four room paths contain 11 pairs each. The last pair is `{-1,-1}`;
the loader consumes only the first twenty or ten points. Pointer lists
contain ten or four paths followed by a real null terminator. Their declared
extents are 44 and 20 bytes, rather than their 48- and 32-byte pieces.
The passage direction table is 11 by 4 signed bytes; the room selector and
room traversal tables are eight signed bytes each. `DngRoutePointOrder`
describes unavailable, forward, and reverse traversal in the direction data.

All nineteen data definitions and the direction-enum cleanup pass separate
full builds and canonical checks. The bare native names retain retail
symbols used by the guarded `LoadDngInfo` assembly. That draft's existing
`__DATA` declaration spellings remain untouched. Receipts use
`dng-route-<symbol>` and `dng-route-orders` prefixes.

Markers: 43 / 5; matched data: 36 / 3159 bytes.
