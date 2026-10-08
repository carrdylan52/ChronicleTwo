# Charasetup data migration (2026-10-08)

## Baseline and verification

Checkpoint `c9694e24` has 65 `INCLUDE_RODATA` markers and three
`INCLUDE_BSS` reservations. All ten functions already match; refreshed
objdiff attributes 0/1,824 data bytes to matching source.

Every accepted step uses the pinned image, a full PAL build and the canonical
149-object checker. All 148 other object hashes must match the preceding
accepted checkpoint. Receipts are in `.private/dataA-r1/`.

## Native storage and named tables

The three BSS objects are native typed definitions: `ROBO_INFO_DATA robo_dat`,
`char r_robo_pname_1282[4][16]`, and `char fname_1290[64]`. The two character
buffers have file-local linkage. `robo_dat` retains its existing public header
declaration for source compatibility, although retail binds its symbol LOCAL.
Its declared size is 0x24; the next object's 16-byte alignment adds 12 piece
bytes. The header's former `@size 0x30` described that padded piece and is
corrected to the actual object size.

`mem_table[4][7]` contains the exact four rows of memory stack capacities.
`robo_info_body[11]` has the documented 37-byte row type and exact 0x197-byte
extent. Retail's first row is `{"body01.chr","arm1"}`; all ten remaining
rows are `{"body02.chr","arm2"}`. The zero tail of the 0x1A0-byte piece is
alignment padding, not another row.

Receipts: `charasetup-storage`, `charasetup-mem_table`, and
`charasetup-robo_info_body`, each with `-{build,objects,hashes}.log`.
All pass PAL, all 149 objects and 148 unchanged other objects.
