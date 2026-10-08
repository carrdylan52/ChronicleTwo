# Nameregi data migration (2026-10-08)

## Baseline and verification

Lane checkpoint `c9694e24` contains 57 `INCLUDE_RODATA` markers and 18
`INCLUDE_BSS` reservations. Refreshed objdiff reports 36/3,648 matching data
bytes. All 30 functions already match. The warm build prints
`SCES_511.90: OK`; the canonical checker passes all 149 units.

Every accepted step uses the pinned `chronicletwo_dev:sf-63f7a9e` image,
checks the full PAL build and all objects, and compares all 148 other object
hashes against the preceding accepted checkpoint. Private receipts are under
`.private/dataA-r1/`. Objdiff data scores have the preparation limitations
recorded in `../automap/data-migration-20261008.md`; markers and canonical
bytes/relocations establish the actual migration.

## Name entry storage

Nine file-local variables now have documented native definitions:
`NameRegiCode` (`s8`), `NameRegiMenuPtr` (`CNameRegiMenu*`),
`OldReloadTexNumber` (`int`), the five texture pointers, and
`NameRegiTopic` (`char[0x40]`). `Nameregi_Target` retains its public
`NAMEREGI_TARGET_INFO` definition and header declaration.

The one-byte code occupies a four-byte piece, the final texture pointer has
four trailing alignment bytes, and the 0x48-byte target record occupies a
0x50-byte piece. Native definitions use the actual declared extents; existing
postprocessing supplies the verified zero padding. No function changes.

Receipts: `01-nameregi-storage-build.log`,
`01-nameregi-storage-objects.log`, `01-nameregi-storage-hashes.log`.
PAL is OK, all 149 objects pass, and all 148 other objects are unchanged.
Markers: 57 read-only/data markers and eight BSS reservations.

## Character conversion and board tables

`NameStrSelectModeTable` uses the documented font modes for its seven
language rows. `NameRegiSearchKanjiIndexTable` holds 46 native 16-byte
records: 45 code boundaries and an all-zero last row. The loops process
44 readings and use the next row as a code boundary.

`testchar` contains 46 Shift-JIS pairs plus its terminator (93 bytes), and
`txt_table2` contains 58 pairs plus its terminator (117 bytes). Both are
native signed-character strings, indexed as pairs without overextending the
objects. `txt_table` retains its exact 59-byte ASCII string.

The remaining named native tables are `LimmitTable_1360[5]`,
`Convtable2_1382[2][5][8]`, `addTable_1510[5][4]`,
`nameregist_baseboard_upper_table[6][12]`, `colt_1808[2]`,
`table_1819[7][5]`, `tex_commtbl_1822[7][6]`, `gettbl0_2012[12]`,
`NameRegistGyouLimmitTable[5]`, `convTbl_1579[5]`, `convtbl_1792`,
and `get_Htable_1806[4]`. Board points, colours and rectangles retain their
existing documented typed layouts. Notable declaration corrections are the
five rows of `addTable_1510`, five bytes of `convTbl_1579`, and four slice
heights `{100,16,32,0}`; the preceding externs were incorrectly sized.

Each table was built and checked separately. Receipt prefixes are
`02-nameregi-font-mode`, `nameregi-<table name>`, and
`03-nameregi-enum-modes`, each with `-build.log`, `-objects.log` and
`-hashes.log`. Every accepted step passes PAL, all 149 objects and the 148
unchanged-other-object hashes.
