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

## Remaining strings and generated switches

The external-function diagnostics and SB2 magic string are inlined at their
calls. The overflow and boolean-type diagnostics keep their exact Shift-JIS
bytes with hexadecimal escapes. `at_341__2` and `at_686` through `at_696`
already have natural string expressions in the matched functions; their
unused declarations and fallback markers can be removed directly.

`at_697`, `at_698`, and `at_699` are compiler-generated switch tables of
`CRunScript::exe`: the opcode switch and its integer/floating comparison
switches. Their R_MIPS_32 entries point to the same function offsets emitted
by the existing native switches. No handwritten table is required.

Final validation: `.private/dataC-r2/runscript-final-{build,objects}.log` and
`runscript-final-metrics.json`. All data markers are gone and native data
coverage is 1180 / 1180. The full image, all 149 objects, and all unowned
object hashes pass. No marker is parked and no shared-tool proposal is
needed for this unit.
