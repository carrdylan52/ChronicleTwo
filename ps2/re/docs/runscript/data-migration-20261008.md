# runscript data migration (2026-10-08)

The round-2 baseline has 27 `INCLUDE_RODATA` markers, no `INCLUDE_BSS`
markers, and `matched_data` 0 / `total_data` 1180 after a warm build and
objdiff refresh. All 28 functions are already native matches; no function
promotion or repeat function analysis is needed.

## Runtime error and value formats

`at_168`, `at_173`, `at_183__2`, `at_197`, `at_202`, and `at_223` through
`at_225` are null-terminated strings used by the runtime error helpers and
value printer. Their assembly words establish the exact text, including
capitalization and newline bytes. Inlining each string at its existing call
preserves every code byte and resolved relocation. The eight marker-backed
arrays and their extern declarations are removed.

Validation receipts: `.private/dataC-r2/runscript-errors-{build,objects}.log`.
The executable remains `SCES_511.90: OK`, all 149 objects pass, and every
unowned object hash equals the warm baseline. The unit now has 19 rodata
markers; its incomplete native rodata section still reports 0 / 1180 data
bytes matched.
