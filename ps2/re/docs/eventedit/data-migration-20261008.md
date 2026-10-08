# Eventedit data migration (2026-10-08)

## Baseline and verification

Checkpoint `c9694e24` has 61 `INCLUDE_RODATA` markers and seven
`INCLUDE_BSS` reservations. All 13 functions match; refreshed objdiff reports
3,676/5,036 matching data bytes.

Every accepted step uses the pinned image, a full PAL build, the canonical
149-object checker and hashes of all 148 other objects against the previous
accepted checkpoint. Receipts are in `.private/dataA-r1/`.

## Native editor state and pointer templates

The six path editing counters/selections are native file-local `int` objects,
and `g_info` is native file-local `EventEditInfo` storage with its documented
0x40-byte layout. All seven BSS reservations and their externs are removed.
The existing native `CCameraPas g_cmr_pas` and `CCharaPas g_chara_pas`
objects are documented and defined before use; their duplicate externs and
an unused `EventScript` extern are removed.

The path objects retain their existing global linkage. Giving them file-local
linkage makes the linker reject the generated `vutext.data.s.o`, whose
`Vu_progmain` references both names. This compatibility requirement is
independent of the retail LOCAL binding recorded by earlier notes. The
rejected source snapshot is `.private/dataA-r1/failed-eventedit-storage.cpp`;
the accepted source keeps the baseline linkage and exact constructor order.

The three local five-pointer arrays in `DrawEventEdit` already have natural
initializers. They now supply their own `at_1208`, `at_1226__2`, and
`at_1242__2` data without markers, retaining all four labels plus the empty
fifth entry, and the two separate operation-name templates.

Receipts: `eventedit-storage`, `eventedit-at_1208`,
`eventedit-at_1226__2`, `eventedit-at_1242__2`, each with
`-{build,objects,hashes}.log`. Every accepted step passes PAL,
all 149 objects and all 148 unchanged other objects.
