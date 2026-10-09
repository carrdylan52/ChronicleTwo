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
