# Event function data migration — 2026-10-08

## Baseline

Checkpoint `3221b488`: 116 `INCLUDE_RODATA` and 22 `INCLUDE_BSS` markers;
823/827 native functions; refreshed data comparison 4/235,364 bytes.
The warm PAL build and canonical checker pass, including all 149 objects.

## Event storage

All 22 zero-storage reservations have typed native definitions. The existing
header describes the event state, flags/counters, mouth-animation names,
particle buffers and camera/object sequence entries. Their exact extents are
unchanged. The two event sound buffers hold 0x801 and 0x141 quadwords;
the external-command dispatch array contains 0x5DC typed function pointers.
The script-argument pointer, effect manager, sword trail, sound buffers and
dispatch array have file-local linkage. `EventEffectScript` keeps its retail
symbol reachable for the guarded `_ESM_INITIALIZE` assembly body.
No guarded function or draft is edited.

Receipts: `.private/dataA-r3/event-storage-{global,local}-{build,objects,hashes}.log`.
Both stages pass PAL and 149/149 objects; every unowned object hash is unchanged.

## Retained markers

The initialized-data markers are pending the following migration topics.
