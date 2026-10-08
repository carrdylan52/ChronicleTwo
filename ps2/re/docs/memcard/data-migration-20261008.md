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

## Costume and browser tables

`cosbit_table` is a file-static array of 34 `COSBIT_INFO` rows. Item numbers
0x6f–0x86 map to bits 0–23; 0x102–0x10b map to bits 24–33. The fourth byte
of every row is zero. The 0x88-byte table receives eight piece-padding
bytes. Both costume lookups now advance typed rows directly.

`MCBrowsetName` is a native three-region, four-category pointer table. Its
Japanese, American, and European titles retain their original Shift-JIS
bytes through octal escapes. `MCBrowserName_Offset` is a matching native
3×4 halfword array, with eight alignment-tail bytes after its 24-byte
payload. Regional selection and the formatted `%s` titles are unchanged.

Receipts `memcard-costume-table`, `memcard-browser-titles-fixed`, and
`memcard-browser-offsets` under `.private/dataC-r1/` each report PAL OK,
149/149 objects, and no unowned object changes. Markers become 30/7.
The rejected first browser-title substitution interpreted octal escapes
inside a regular-expression replacement; regenerating the source with
literal backslashes resolves that script error without changing values.

## Local aggregate templates

`MakeMemoryCardFileName` now initializes its 20-byte directory buffer and
19-byte filename buffer directly with their retail formats. `DeleteFile`
initializes its 64-byte local `darkcloud%d` buffer inside the successful
synchronization branch. `GetAllSaveFileInfo` initializes its 128-byte
save-directory search pattern at the original copy point. The anonymous
aggregate objects, extern declarations, and byte-array wrapper types are
removed; native compiler templates supply identical data and relocations.

`SetIconData` uses the SDK's `sceMcColor`, `sceMcVu0FVECTOR`, and
`sceMcColorF` types for four background colours, three light directions,
three light colours, and ambient colour. Their initialized templates remain
64/48/48/16 bytes. The retail routine copies just 16 bytes from each
array into the cleared icon descriptor, leaving its remaining entries zero;
that behavior is preserved. Icon filename fields are accessed by their
real `name` members instead of structure-address casts.

Receipts `memcard-filename-templates`, `memcard-icon-templates`,
`memcard-delete-template`, and `memcard-directory-template` each report
PAL OK, 149/149 objects, and unchanged unowned hashes. Markers become 22/7.
