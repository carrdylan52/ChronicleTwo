# Save conversion data migration (2026-10-08)

All thirty initialized-data markers are removed. Screen text, directory
masks, suffixes and diagnostic formats are inline strings. The conversion
loop's three 128-byte format/name arrays retain their ordinary C++ aggregate
initializers and the retail stack layout.

Named state pointers and integers now have typed definitions. The size
information table is `int SaveFileInfoTableSizeConvert[128]`: its retail
symbol size is 0x200, while the reset loop writes only 32 entries. This
symbol needs external source linkage because the remaining `Vu_progmain`
assembly references it, despite the retail symbol being LOCAL. A preliminary
static definition failed at link time and was corrected before acceptance.

The four packet/data buffers in `SVConvViewInit` are native function-local
`mgCMemory` statics. Their constructors and guards replace the manual `Init`
blocks. `DataBuffer__3` and `Stack_ReadBuff__3` remain file-local globals and
still emit exactly the two retail global initializer calls.

## Retained BSS pieces

The four local buffers and their four compiler-generated guards retain
storage markers. The current postprocessor can bind native local BSS while
markers remain, but cannot name their sections after those markers are
removed. No explicit guard variables or buffer externs remain in C++.

`ConvertResultDispTime` is an integer of declared retail size four at
0x0037EA8C. Its piece extends to 0x0037EAC0 and owns 0x30 additional bytes.
The existing native padding rule rejects gaps of 16 or more bytes, so this
one scalar keeps its extern and marker pending shared-tool support.

The accepted screen, conversion-literal and final state/local-buffer steps
pass the PAL verifier and all 149 object checks. Marker counts decrease
from 30 `INCLUDE_RODATA` and 19 `INCLUDE_BSS` to 0 and 9. Receipts:
`.private/dataB-r1/convviewlp-{screen,conversion-literals,local-buffers-linked}-{build,objects}.log`.

The refreshed progress report remains at matched_data 0 / total_data 2165
before and after this migration. The source-only data naming/configuration
limits make this metric distinct from the exact linked-object proof. The
final receipts are `.private/dataB-r1/final-{build,objects,progress}.log`.
`ConvertResultDispTime` is only written in this unit; its source comment
states the recorded display-time value without claiming countdown behavior.
