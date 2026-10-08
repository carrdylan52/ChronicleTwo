# mapsky data migration (2026-10-08)

Baseline: `d56248a7`; 11 `INCLUDE_RODATA` / 3
`INCLUDE_BSS` markers; matched_data 0/195.

The script state is native file-local storage: MAP_SKY_INFO *skyInfo and the
two integer animation counts (`mapsky-parser-storage`). LoadSkyPack uses a
native SPI_TAG_PARAM[8] command table with seven inline keywords in retail
order and a null row. Every callback retains its existing internal linkage
(`mapsky-command-table-retained-empty`).

The existing LoadPack aggregate supplies the 16-byte native
mgCreateVisualType[2] initializer and its pointer relocation; info.cfg is
already an inline literal. Their two markers are removed. One empty-string
marker remains: at_386. Inlining its empty string while removing the visual
initializer marker causes the native anonymous initializer and its string
target to lose piece identity (`mapsky-background-visual-template`,
`mapsky-command-table`; canonical detail in
`mapsky-background-visual-failed-objects.log`). The native initializer's
R_MIPS_32 points at a new compiler anonymous string; the current mapper
cannot establish both identities together. Restoring the empty-string
extern/marker gives a complete-unit match and permits the native aggregate,
configuration literal, command table and all other keyword strings to stay.
No arbitrary string or table-name padding is introduced.

Final: 1 rodata / 0 BSS markers; matched_data
92/195 after the standard objdiff/progress refresh.
Every accepted step passes the full PAL build (`SCES_511.90: OK`) and
all 149 complete objects, including resolved relocations. The 148 other
object hashes are unchanged at each step. Function coverage stays
6,770 matched / 93 guarded / nine assembly-only / zero fuzzy.

Validation receipts are under `.private/dataE/`, with the step prefixes
listed above and `-build.log`, `-objects.log`, and `-hashes.log` suffixes.
Final refresh: `mapsky-final-progress.log`, `mapsky-final-coverage.log`,
and `mapsky-final-metrics.json`.
