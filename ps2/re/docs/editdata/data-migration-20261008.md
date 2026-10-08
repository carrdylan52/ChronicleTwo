# editdata data migration (2026-10-08)

The round-2 baseline has 16 rodata markers and 6 BSS markers, with
`matched_data` 4 / `total_data` 16884. All functions already match; the
existing notes document the saved layouts, analysis classes, tag handlers,
and loading functions.

## Layout diagnostics and analysis filenames

The six standalone format strings now occur directly in `SaveData`,
`LoadData`, `Analyze`, and `LoadEditAnalyzeData`. Assembly words establish
the exact diagnostic text, including `infinty loop!!!`, the extra
exclamation marks on the parts limit, the grid message's lack of a newline,
and `GeoData Remain = %dkbyte`. The filename remains `geo%d.cfg`.
All six extern string declarations and markers are removed.

Each function's literals were checked separately. Acceptance receipts:
`.private/dataC-r2/editdata-{save-strings,load-strings,analysis-string,loader-strings}-{build,objects}.log`.
Every full image and all 149 objects pass; every unowned object hash is
unchanged. The unit has 10 rodata / 6 BSS markers after this step.

## Parser tags and active analysis state

The nine named analysis tags and their terminator become a documented
file-local `SPI_TAG_PARAM[10]`. Each name is inlined in its row and the
function pointers use their established `int (SPI_STACK *, int)` signatures.
The handlers and script-running overload have retail-local binding and
receive `static` declarations. All ten remaining rodata markers disappear.

A natural function-local table has exact bytes and produces a matching
PAL image, but the object checker cannot map the changed `tag_<suffix>`
name to `tag__6` without a marker. File-local `tag` is equally consistent
with the established table semantics and lets the existing duplicate-name
projection establish its retail identity. Failed receipt:
`editdata-tag-table-objects.log`; accepted receipt:
`editdata-file-tag-table-objects.log`, under `.private/dataC-r2/`.

`AnalyzeSrc` retains its five established 0x340-byte instances with local
binding. `eaAnaSrc`, `eaAnaData`, and `eaStack` become documented typed
file-local pointers with four-byte extents. Their three BSS markers and
extern declarations are removed; the global constructor stays exact.

## Reused loading buffer and memory manager

`LoadEditAnalyzeData(int, u_long128*)` owns `static u_long128 buff[0x300]`
and `static mgCMemory Stack`. The manager's existing inline constructor
calls `Init`; MWCC emits the same one-time byte guard and initialization
instructions as retail. The explicit `init_1273` conditional and all three
extern declarations disappear. The buffer is 0x3000 bytes and the manager
is 0x30 bytes; neither needs a replacement storage type or helper.

### Named local BSS identity blocker

Without the three markers, the generated local names do not bind to retail:
`buff_983`, `Stack_984`, and `init_985` versus `buff_1271`, `Stack_1272`,
and `init_1273` in this particular trial. The suffixes are compiler-generated
and must not become source or tooling selectors. The new loader instructions
have zero masked word differences, but its unresolved references leave
78 differing linked bytes. Receipt:
`.private/dataC-r2/editdata-local-stack-{build,objects}.log` and
`editdata-local-stack-native.dump`.

The guard has a one-byte declared extent and a four-byte internal section
piece; its final three bytes are alignment before `eaAnaSrc`. Existing
`bind_named_static_bss` needs explicit markers to identify named local
storage, and the marker-free anonymous binder currently recognizes only
`at_<number>` templates. The three markers stay until a generic named
local/guard identity mapper uses source names, exact declared extents, and
consistent opcode-matched references, including the manager's member
addends +0x28 and +0x24. No shared tool was edited.

Final acceptance: `.private/dataC-r2/editdata-state-final-{build,objects}.log`
and `editdata-state-final-metrics.json`. All 149 objects, the complete PAL
image, and every unowned object hash pass. The unit has 0 rodata / 3 BSS
markers and native data coverage 372 / 16884.

## Direct saved-house indexing trial

The inherited `EditHouseIndex` inline wrapper was tested as the direct
`part->house - house + 1` expression and with a meaningful local
`house_index`. Both are semantically identical but swap the division
result's v0/v1 registers in six instruction words (seven differing linked
bytes). An explicit `static_cast<int>` worsens the schedule and spills,
expanding `SaveData` to 0x5D8 bytes from the 0x5D0-byte retail extent.
No new helper or dummy local is retained. The accepted source is restored.
Receipts: `.private/dataC-r2/editdata-house-difference-build.log`,
`editdata-house-difference.dump`, `owned-natural-fields-{build,objects}.log`,
and `editdata-house-index-local-build.log`.
