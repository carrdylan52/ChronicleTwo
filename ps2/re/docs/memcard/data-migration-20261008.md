# Memory-card data migration

## Baseline

At `ab376093`, all 49 functions match. Baseline markers are 46 RODATA / 9
BSS. Refreshed objdiff credits 0/1,792 data bytes (`matched_data` is absent
from this unit's measures, meaning zero). The warm build reports PAL OK and
all 149 objects pass.

## Named operation state

`DngTreeSaveFlag` is a native file-static signed halfword; its two declared
bytes receive two zero piece-padding bytes. `NowProgramLoopNo` is the public
signed halfword initialized to -1, also with a two-byte alignment tail.
`SubGameOmakeTempBuffer` is the existing public byte-buffer pointer. Their
header declarations stay unchanged.

`memcard-named-state-{build,objects,metrics}.log` under `.private/dataC-r1/`
records PAL OK, 149/149 objects, and unowned hashes unchanged. Markers become
45/7.
