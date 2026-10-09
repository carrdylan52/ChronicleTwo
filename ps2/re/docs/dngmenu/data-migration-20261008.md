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
