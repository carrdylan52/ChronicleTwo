# userdata data migration

## Baseline

The 63f7a9e5 unit contains 114 `INCLUDE_RODATA` markers and seven
`INCLUDE_BSS` markers. The refreshed progress report credits 4 / 4,264
data bytes. All functions already match.

## Native BSS

`FishGamePreEquip` is a file-local `CGameDataUsed *`.
`BattleParamater_Time` and `BattleParamater_TimeBand` are file-local
`float` and `int` caches. Their native zero-initialized definitions
replace the three four-byte assembly reservations.

`word_1327` is the 0x61-byte item-name buffer used by `GetName`; the
0x70-byte section piece includes the following 15-byte alignment gap.
`temp_1510` is the 0x40-byte spectrum description buffer used by
`GetMsgAddInfo`. Both are native file-local character arrays. The
postprocessor retains the name buffer's declared extent and its zero
alignment gap, following `docs/MWCC.md`.

Validation: `.private/dataC/userdata-bss-build.log` records
`SCES_511.90: OK`; `.private/dataC/userdata-bss-objects.log` records
149 / 149 passing objects. All other game object file hashes equal
the warm baseline. The marker counts are 114 / 2 after this step.

## Numeric tables and aggregate templates

Native signed-byte, halfword, word, and float tables replace
`htbl_1662`, `fish_record_dataindex_convert`, `weptbl_4503`,
`lifetbl_2854`, `use_limmit_table_2558`, `tbl1_5167`, `tbl2_5168`,
`equip_type_tbl_5456`, `at_table_5400`, and the public
`aquarium_fish_maxtbl`. Each step has a separate full build and object
receipt under `.private/dataC/userdata-<symbol>-*.log`, except the
equipment and ability tables use `userdata-equip-types` and
`userdata-attributes-data-only`.

The equipment table is 3 x 5 signed bytes; `SearchEquipType` uses
ordinary two-dimensional indexing with identical instructions.
`tbl1_5167` has four bytes `{1, 2, 3, 4}`, although the selection
function reads only its first three choices. `at_table_5400` is 14
words (0x38 bytes); the combination loop checks the first 12. Its
last two zero entries are part of the declared object, not alignment
padding.

The existing local ridepod item-number and weapon-rate initializers
emit `at_4196` and `at_4695` without assembly. The local party-bit
initializer emits `at_3192` directly; its final entry shares Monica's
party bit because monster form belongs to her.

A direct `at_table_5400[bit]` trial, with the pair declared either
before or after the mask, swaps the induction-offset and pair
registers (`$9` / `$10`) relative to retail. The data definition
matches independently; the existing byte-offset consumer remains
until a natural indexed form reproduces those registers. Evidence:
`userdata-attributes-failure.log`, `CheckWeaponAttribute.m2c.cpp`,
and `userdata-native.dump` under `.private/dataC/`.

The completed group has 101 data markers and two BSS markers.
`userdata-attributes-data-only-build.log` records PAL OK, its
object receipt records 149 / 149, and every unowned object file hash
is unchanged. The refreshed data measure remains 4 / 4,264 bytes:
objdiff credits complete matching sections, so partial migrations
do not necessarily increase that measure.
