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
306/0; refreshed matched data becomes 188/21,300.
