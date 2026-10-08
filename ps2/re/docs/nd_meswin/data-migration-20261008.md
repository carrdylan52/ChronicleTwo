# Message-window data migration

## Baseline

At `ab376093`, all 98 functions match. Baseline markers are 71 RODATA / 8
BSS, and refreshed objdiff matched data is 20/9,068 bytes. The warm build
reports PAL OK and all 149 objects pass. `DrawMesWin` retains its complete
source body and the existing four control-context scheduling rows.

## Message and caption storage

The public definitions use the established header types: `s32` drawing offset
and caption counters, `short NameRegistTbl[8][11]`, two 20-element frame arrays,
and `char MovieCCStr[20][350]`. `MovieCCFont` remains the existing native
`CFont`. The caption strings own 7,000 declared bytes; their eight-byte zero
alignment tail is supplied by the existing postprocessor. No header or caller
changes are required.

`nd-movie-storage-{build,objects,metrics}.log` under `.private/dataC-r1/`
accepts this step: PAL OK, 149/149 objects, no unowned hash changes. Markers
become 71/0.
