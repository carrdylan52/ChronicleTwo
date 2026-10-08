# sceneevent data migration (2026-10-08)

Baseline: `d56248a7`; 6 `INCLUDE_RODATA` / 0
`INCLUDE_BSS` markers; matched_data 0/146.

The event-running diagnostic, eye-view group names, and fire-raster texture
name are inline literals (`sceneevent-literals`). The writable time-band
flare palette is `static float col_1003[4][4]` with its exact four rows and
16-byte alignment (`sceneevent-flare-colors`). Its 64 bytes are native data.

One marker remains: `at_1013__4`, the `{0, 0, 0, 78}` base flare-color
initializer in DrawLensFlare. Natural float-array, aligned-vector, and
existing union aggregate initializers preserve data but change register
allocation: the compiler retains ratio[0] across the aggregate copy,
where retail reloads it. Moving the following sun/screen declarations into
the conditional scope restores stack order but does not restore that
allocation (`sceneevent-flare-vector`, `-aligned`, `-aggregate`, `-order`).
Copying a native static union generates an extra implicit assignment
function (`sceneevent-flare-vector-copy`), so it is rejected. Ordinary
memcpy from a static native base-color array, with and without SDK vector
alignment, preserves the function extent but changes source/destination
address registers: the best candidate differs in **29/156 relocation-masked
words**, native extent 0x26C within the retail 0x270-byte piece
(`sceneevent-flare-vector-memcpy`, `-memcpy-aligned`). No failing source,
new type pun, helper, dummy local, compiler pragma, or tool/profile edit is
retained. The existing marker/copy remains pending a natural exact form.

`sceneevent-final` revalidates the accepted source after all probes are
reverted. The best-score and disassembly receipts are
`sceneevent-flare-vector-best-score.log` and
`sceneevent-flare-vector-memcpy-aligned-disassembly.log`.

Final: 1 rodata / 0 BSS markers; matched_data
66/146 after the standard objdiff/progress refresh.
Every accepted step passes the full PAL build (`SCES_511.90: OK`) and
all 149 complete objects, including resolved relocations. The 148 other
object hashes are unchanged at each step. Function coverage stays
6,770 matched / 93 guarded / nine assembly-only / zero fuzzy.

Validation receipts are under `.private/dataE/`, with the step prefixes
listed above and `-build.log`, `-objects.log`, and `-hashes.log` suffixes.
Final refresh: `sceneevent-final-progress.log`, `sceneevent-final-coverage.log`,
and `sceneevent-final-metrics.json`.
