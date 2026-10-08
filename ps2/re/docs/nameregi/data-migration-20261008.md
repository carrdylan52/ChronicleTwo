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
