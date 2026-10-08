# Mapselect data migration (2026-10-08)

## Baseline and verification

Checkpoint `c9694e24` contains 52 `INCLUDE_RODATA` markers and 21
`INCLUDE_BSS` reservations. All 23 functions already match; refreshed
objdiff reports 32,832/33,980 matching data bytes.

Every accepted step uses the pinned image, a full PAL build, the canonical
149-object checker and hashes of all 148 other objects against the preceding
accepted checkpoint. Private receipts are under `.private/dataA-r1/`.

## Native map and viewer state

Nineteen named BSS reservations now have documented native definitions.
The counters, selections and buffer positions use `int`; `map_name` points
to the existing 0x1C-byte `MAP_NAME_INFO` records; `CharBuff` points to their
following strings; `MenuStack` is an `mgCMemory` pointer. The per-category
lists are `char **SelectMapList[MAP_SEL_TYPE_NUM]` with parallel integer counts.
`EventInfo` and `BossBattleSelFlag` retain their public header declarations.
The dead `NONMATCHING` state block and reservation guards are removed.

`MapNameBuff` is a 0x8000-byte arena containing a dynamic number of map
records followed by variable-length strings. It is a native character array,
not a fixed record array or a table of padded records. The existing serialized
arena offsets remain valid. Earlier notes' `u_long128[0x800]` description does
not match the byte-oriented source traversal.

The arena retains baseline global linkage because generated `Vu_progmain`
data references `MapNameBuff` by name at multiple interior offsets. A
file-local definition passes the isolated object comparison but makes the
full linker reject these references. The rejected source is preserved as
`.private/dataA-r1/failed-mapselect-storage.cpp`; generated data is unchanged.

`MapTypeSelect` now declares its natural function-local `static int select = 0`.
The compiler supplies the initialization guard and assignments formerly
written explicitly. The `select_1009` and `init_1010` externs are removed;
the two reservations remain pending native BSS identity support.

Receipts: `mapselect-storage` and `mapselect-guard`, each with
`-{build,objects,hashes}.log`. Both accepted steps pass PAL, all 149 objects
and all 148 other object hashes. Markers are now 52/2.
