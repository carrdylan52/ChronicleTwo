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

## Active script state

`nowScene`, `nowMonster`, `LastCInfo2`, and `ext_func` become typed definitions.
The scene and damage pointers and the 256-entry callback array have local
retail binding and are `static`; `nowMonster` retains its header-declared
global binding. `ACTION_DAMAGE` is the established return type of
`CActionChara::EntryDamage2`, resolving the older note's pointee question.
Their extents are 4, 4, 4, and 0x400 bytes, respectively.

Validation: `.private/dataC-r2/opcodes-state-{build,objects}.log` and
`opcodes-state-metrics.json`. All BSS markers are removed; all code bytes and
relocations remain exact, the complete image passes, and unowned objects
are unchanged.
