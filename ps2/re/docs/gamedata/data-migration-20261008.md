# Gamedata data migration

## Baseline

At `ab376093`, all 67 functions match. Baseline markers are 46 RODATA / 16
BSS, and refreshed objdiff matched data is 36/47,820 bytes. The warm build
reports PAL OK and all 149 objects pass.

## Catalog storage

Native definitions replace 15 reservations using the established item types:
common records (432), usable items (162), weapons (116), attachments (38),
ridepod parts (68), fish (20), guard values (35 signed halfwords), and the
512-halfword item-number lookup. The constructed arrays retain their retail
initialization order. Script cursors are typed pointers; the common count is
an `int`. The public `GameItemDataManage` and `Spi*` declarations remain
source-compatible. File-local scratch names retain their 32- and 128-byte
character-array types.

`local_guarddata` owns 0x46 declared bytes; its 0x50-byte piece includes
14 zero alignment bytes. The native 35-element array preserves that declared
extent, rather than treating its alignment as five additional entries.

The 0x2800-byte name buffer has a native static character-array definition,
but its marker remains. Removing it yields an unresolved public reference
from `vutext.data.s.o` to `gamedata_sysword_buffer_1073`. This is the same
unrelocated VU-word dependency documented for FontTblBinBuff in the previous
lane; the shared VU splitter correction belongs to the tooling lane. The
buffer must remain genuinely local rather than acquire invented public
linkage to satisfy a false dependency.

The failed removal receipt is `gamedata-storage-build.log`. Acceptance with
the single marker retained is `gamedata-storage-retained-buffer-{build,
objects,metrics}.log` under `.private/dataC-r1/`: PAL OK, 149/149 objects,
and every unowned object hash unchanged. Markers become 46/1.

## Spectrumising parameters

The public `etcitem_spectol_table` retains its existing signed-byte array
declaration and has a native initializer for all 425 parameter/value pairs.
Each line holds one pair in item-number order. The 0x352-byte declared object
has a 14-byte zero alignment tail; no extra pairs are introduced. Its existing
consumer interprets parameter indices below eight as attributes and indices
10 and above as status parameters.

`gamedata-spectrum-table-{build,objects,metrics}.log` accepts the table:
PAL OK, 149/149 objects, unowned hashes unchanged. Markers become 45/1.

## Script tags

The native `gamedata_tag[25]` contains 24 exact name/callback pairs and the
null terminator. It follows its callbacks and precedes the script loader,
using the existing `SPI_TAG_PARAM` type. Strings and callback associations
are copied from the actual retail relocation words; the guard callbacks
precede the fish callbacks in the table, regardless of source function order.
The declared 0xC8 bytes receive their eight-byte zero alignment tail.

`gamedata-tags-{build,objects,metrics}.log` accepts this group with PAL OK,
149/149 objects, and no unowned object changes. Markers become 20/1.
