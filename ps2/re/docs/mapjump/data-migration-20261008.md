# mapjump data migration (2026-10-08)

Baseline: `95f8fdd1`; 11 initialized-data
markers and 17 BSS markers. Every function already matches.
The pinned `chronicletwo_dev:sf-63f7a9e` image and full object/PAL checks
validate each accepted step. Public declarations remain compatible.

## Map and interior state

All seventeen BSS reservations are removed. Named state uses native file-local
integers, `mgCMemory *`, four 64-byte name/path arrays, four `sceVu0FVECTOR`
arrays and `CScene::BGM_STATUS`. The music status owns 0x1C bytes; its four-byte
piece tail is alignment, not an extra field. The existing `MapJumpMapInfo`
objects and their constructor order are unchanged.

`LoadMapScript` starts with an ordinary `char script[0x80] = ""`. m2c and
retail disassembly show four 32-byte copies from the zero BSS template
`at_912__4` into that local before path concatenation. The template's only
code references are that load; it is not a persistent mutable script prefix.
The native array emits the exact 0x80-byte anonymous zero template, identified
by declared extent and the real HI16/LO16 references. The old struct wrapper,
anonymous extern, storage marker and unnecessary array casts are removed.
This resolves the ambiguity recorded in the older notes.

Separate state/template receipts are
`.private/dataD/mapjump-state-{build,objects,metrics}.log` and
`.private/dataD/mapjump-script-template-{build,objects,metrics}.log`.
Both pass PAL, all 149 objects and unchanged unowned hashes. m2c evidence is
`.private/dataD/map-script-m2c.c`. Interim markers are 11/0 and matched data
572/753. No scheduling profile or other unit is edited.
