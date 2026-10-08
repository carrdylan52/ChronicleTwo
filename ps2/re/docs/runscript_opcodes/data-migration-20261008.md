# runscript_opcodes data migration (2026-10-08)

The fresh round-2 baseline has 14 rodata markers, 4 BSS markers, and
`matched_data` 0 / `total_data` 2700. All functions in this unit already
match; the existing notes own their type and behavior analysis.

## Diagnostic and model literals

Nine string markers are removed. `_SET_BODY`, `_SET_DMG`, `_GET_MAPOBJ_POS`,
and `SetMonsterExtendTable` now use the exact diagnostic strings directly.
The range/monster-index diagnostics and the ridepod weapon name already
occur as literals in native functions, so their unused extern declarations
and markers are removed without changing those bodies. The Shift-JIS weapon
name already uses hexadecimal escapes.

The assembly words establish exact text, including the spelling
`mscript same ext_func_no!!!` and the lack of a newline in `ext func over!!`.
Validation: `.private/dataC-r2/opcodes-strings-{build,objects}.log` and
`opcodes-strings-metrics.json`; the image and all 149 objects pass and all
unowned objects retain their baseline hashes.
