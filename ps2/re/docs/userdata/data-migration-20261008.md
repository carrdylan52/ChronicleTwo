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
