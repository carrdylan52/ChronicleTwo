# October 8 data migration

Baseline: `7c7edc2f`, MWCC 3.0/Satan's Fiddle through
`chronicletwo_dev:sf-63f7a9e`. The warm build verifies PAL OK and
149/149 canonical objects. Initial markers: 261 RODATA / 51 BSS.
Refreshed matched data: 4 / 18656 bytes.

## Named storage

Thirty-four named objects now use their existing native types and LOCAL
linkage: menu/session pointers, photo and recipe parsing state, notebook
arrays, drawing and command state, photo-effect state, album slot state,
debug state, and work buffers. The recipe manager is eight bytes and has
no constructor; its definition adds no initialization code.

`CMenuInventPt` and `InventSubDataReadBGInfo` are four-byte objects despite
eight-byte reservations. Byte and halfword flags keep their declared
widths. `temp_1728` is a 33-byte string; the fifteen bytes before the next
symbol are alignment, not characters. Notebook arrays contain 512 idea
names and 512 signed-halfword identifiers; the photo-name work buffer is
0x2480 bytes. Guarded blocks and profile rows remain unchanged.

Markers: 261 / 17. Matched data remains 4 / 18656 because the aggregate
sections still contain reservations. `invent-state-build.log` and
`invent-state-objects.log` under `.private/nminv-r2/` pass the full PAL
verifier and 149/149 objects.
