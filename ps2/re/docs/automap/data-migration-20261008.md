# Automap data migration (2026-10-08)

## Baseline and verification

Baseline `63f7a9e5` has 307 `INCLUDE_RODATA` markers, seven `INCLUDE_BSS`
reservations, and 43 matching functions. The refreshed objdiff report attributes
28 of 21,300 data bytes to matching source.

Each accepted step passes the full build (`SCES_511.90: OK`) and all 149 units
in `check_objects.py`, which compares bytes and resolved relocations. SHA-256
comparison against the warm-build baseline also checks that every other compiled
object is unchanged. Private receipts are under `.private/dataA/`.

## Room-script and navigation storage

The seven four-byte `.sbss` objects are file-local typed storage, in retail order:
`auto_map` (`CAutoMapGen*`), `nowPrisetStack` (`mgCMemory*`), `nowPriset`
(`AUTOMAP_ROOM_INFO*`), `nowPrisetNum` (`int`), `nowPrisetTable` (`s16*`),
`cax` and `cay` (`int`). Their uses and layouts are established in `notes.md`.
Replacing the extern declarations and reservations with documented `static`
definitions preserves every instruction, data extent, and resolved relocation.

Receipts: `01-bss-build.log`, `01-bss-objects.log`. All 148 other object hashes
are unchanged. Marker counts become 307/0; matched data remains 28/21,300,
because the source-only object already inferred storage from the BSS markers.

## Mini map symbol appearance

`symbol_table` at `0x341C60` is a file-local writable array of ten
`MINIMAP_SYMBOL_INFO` rows. Nine rows specify existing `MINIMAP_SYMBOL`
values; the final row uses `MINIMAP_SYMBOL_END` and zero appearance fields.
The native aggregate retains every colour, dimension, blink flag and visibility
flag, and preserves the retail-local binding.

Receipts: `02-symbols-build.log`, `02-symbols-objects.log`. All 149 units pass;
PAL is OK; all 148 other object hashes remain unchanged. Marker counts become
306/0; refreshed matched data remains 28/21,300.

## Dungeon map tile tables

`MiniMapInfoData` at `0x33EE40` contains 18 writable `MINIMAP_INFO`
aggregates, each with its 16-byte map name and 320 signed tile indices.
All 18 names and all 5,760 tile indices are represented directly by the native
initializer. The definition retains the public name, 0x2E20-byte declared
extent, and existing header layout. The trailing 43 tile slots of each map
are zero; tile values include the retail negative entries unchanged.

Receipts: `03-tiles-build.log`, `03-tiles-objects.log`, `03-tiles-metrics.log`.
All 149 units pass; PAL is OK; all 148 other object hashes remain unchanged.
Marker counts become 305/0; matched data remains 28/21,300 while other pieces
of the source-only `.data` section are incomplete.

## Generated part catalog

`PartsInfoData` at `0x33D440` is 277 native `AUTOMAP_PARTS_INFO`
aggregates. Its names and documented kind/link flags are inline; the eight
auxiliary shorts of each row retain their existing unknown field name and exact
retail values. Those shorts range from -1 to 85 and are not uniformly repeated;
no consumer establishes their meaning. Bits 0x40 and 0x80 have neutral enum
names recording which stair and door variants carry them, without claiming a
behavior. All 276 nonempty part-name literals are emitted by the compiler.

The final row at `0x33EE20` points to the empty string at `0x36CFA8`.
Its name pointer is not NULL. Retail `SetMapInfo` advances the row offset by
0x18 at `0x1D5EE8`, then tests the name pointer at `0x1D5EFC`/`0x1D5F00`;
after the empty-name row it reads zero from `0x33EE38`, the eight alignment
bytes before the next table. The native 0x19F8-byte object receives that exact
zero tail through the documented data-padding postprocessor.

The empty-name literal retains `at_1054` and its marker. A fully inline `""`
initializer emits one unnamed byte of zero: `name_literal_data` finds several
matching zero-filled pieces, and currently distinguishes candidates only through
code relocations. This literal has only a pointer relocation in the catalog.
The probe leaves `at_1054` missing and an unexpected unnamed `.rodata` piece,
so it cannot pass the canonical checker. Keeping just that literal marker and
reference preserves exact source for the rest of the catalog. Negative receipts:
`04-parts-build.log`, `04-parts-probe-objects.log`.

Proposed shared-tool change, outside this lane's ownership:
`.private/proposals/automap-empty-literal-data-relocation.patch`. It extends
ambiguous-literal identification to existing R_MIPS_32 relocations in known
named native data objects; it does not modify data or instruction bytes. The
proposal is not applied or validated here. After accepting that tool change, the
last row can use `""` and the `at_1054` declaration/marker can be removed.

Accepted receipts: `04b-parts-build.log`, `04b-parts-objects.log`,
`04b-parts-metrics.log`. All 149 units pass; PAL is OK; all 148 other object
hashes remain unchanged. Marker counts become 28/0.

## Room script command table

`tag` is a file-local, writable eight-row `SPI_TAG_PARAM` array at
`0x341D00` (the split disambiguates its name as `tag__4`). Each of its seven
command names is inline with its callback; the last row is `{NULL, NULL}`.
The callbacks use `static` linkage, consistent with their retail LOCAL
binding. The existing documented callback signatures and script behavior are
unchanged. The split-name suffix is handled by the existing postprocessor.

Receipts: `05-tags-build.log`, `05-tags-objects.log`, `05-tags-metrics.log`.
All 149 units pass; PAL is OK; all 148 other object hashes remain unchanged.
Marker counts become 20/0.
