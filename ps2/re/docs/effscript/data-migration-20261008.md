# effscript native data migration (2026-10-08)

The refreshed starting unit contains 33 initialized-data markers and five BSS markers. Strict objdiff measures are `matched_data = 0`, `total_data = 25232`; 181 of 185 functions match. The four guarded drafts and assembly functions remain unchanged. Existing type sizes, resource paths, linked-list ownership and command signatures are documented in `notes.md`.

## Migrate effect script resource catalog

`eff_spt_base_def` is a mutable global array of 219 `EFF_SPT_BASE_DEF` rows, each 0x64 bytes: 32-byte name, `EffSptBaseType`, 32-byte resource filename and 32-byte script filename. Its declared extent is 0x558C; the retail data piece has four additional zero padding bytes. The first 173 rows select character resources, the next 45 select image resources, and the final empty-name row uses `EFF_SPT_BASE_END`. Fixed arrays preserve duplicate names and Shift-JIS bytes with hexadecimal escapes. The actual retail data bytes establish these strings; apparent pointer expressions in generated assembly are not relocations within these arrays.

Accepted steps: `base-definitions`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

