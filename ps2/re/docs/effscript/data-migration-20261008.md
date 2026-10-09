# effscript native data migration (2026-10-08)

The refreshed starting unit contains 33 initialized-data markers and five BSS markers. Strict objdiff measures are `matched_data = 0`, `total_data = 25232`; 181 of 185 functions match. The four guarded drafts and assembly functions remain unchanged. Existing type sizes, resource paths, linked-list ownership and command signatures are documented in `notes.md`.

## Migrate effect script resource catalog

`eff_spt_base_def` is a mutable global array of 219 `EFF_SPT_BASE_DEF` rows, each 0x64 bytes: 32-byte name, `EffSptBaseType`, 32-byte resource filename and 32-byte script filename. Its declared extent is 0x558C; the retail data piece has four additional zero padding bytes. The first 173 rows select character resources, the next 45 select image resources, and the final empty-name row uses `EFF_SPT_BASE_END`. Fixed arrays preserve duplicate names and Shift-JIS bytes with hexadecimal escapes. The actual retail data bytes establish these strings; apparent pointer expressions in generated assembly are not relocations within these arrays.

Accepted steps: `base-definitions`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Define typed effect script command storage

`now_scene` and `EffScriptMan` are global pointers to the active scene and effect manager. `now_script` is a file-local pointer to the script executing external commands. `ext_func__4` is a mutable file-local array of 256 correctly typed `int (RS_STACKDATA *, int)` callbacks (0x400 bytes). Its retail spelling remains available to the guarded `CreateEffSpt` assembly and its unchanged draft. Definitions preserve existing header declarations and global linkage.

Accepted steps: `storage`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Initialize effect direction vectors naturally

`_GET_DIR_VECTOR` and `_CHR_GET_DIR_VECTOR` each initialize a four-float local direction as `{0.0f, 0.0f, 1.0f, 1.0f}` before transforming it. These initializers reproduce the two mutable 16-byte compiler templates (`at_2311` and `at_2498__2`) without the former quadword casts.

Accepted steps: `vector:at_2311`, `vector:at_2498__2`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Initialize effect sprite size locally

`DrawEffSptSprite` initializes its four-component size vector to zero before assigning sprite width and height. A native `sceVu0FVECTOR` initializer replaces the anonymous 16-byte BSS vector `at_2067`. Removing its marker and the two direction templates eliminates the `EffectVector` union scaffolding.

Accepted steps: `vector:at_2067`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Inline effect resource loading literals

Resource loading uses native `%s.chr`, `%s.img`, `%s.stb` pack suffixes and `dungeon/eff_script/` paths. `ClearBaseFromLevel` owns its texture-bank exhaustion diagnostic. Each literal is inline at its actual native use.

Accepted steps: `strings:at_943__3`, `strings:at_1127__2`, `strings:at_1128__3`, `strings:at_1129__2`, `strings:at_1143`, `strings:at_1144`, `strings:at_1145`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Inline effect script command diagnostics

Native diagnostics report sprite-work exhaustion, collision polygon limits, unavailable collision primitives, command coordinates, effect creation failures, duplicate command numbers and dispatch capacity exhaustion. Their byte content and null termination are preserved, including hexadecimal Shift-JIS escapes.

Accepted steps: `strings:at_1655__5`, `strings:at_3303__2`, `strings:at_3398`, `strings:at_3495`, `strings:at_3536`, `strings:at_3644`, `strings:at_3645`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

## Use native effect intersection switch data

The existing `_INTERSECTION_POINT` switch generates the nine-entry `at_3304__2` jump table from its native cases. Its 0x24-byte payload occupies a 0x30-byte aligned retail piece. Removing the marker preserves all target addresses and zero padding.

Accepted steps: `switch`. Each has full PAL, 149-object and unowned raw-object hash receipts under `.private/dataA-r3/eff-<topic>-{build,objects,hashes}.log`.

