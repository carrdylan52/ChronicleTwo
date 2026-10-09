# October 8 night review fixes

## CalcTex gift coordinates

`CMenuInvent::CalcTex` initializes the gift-box coordinates as a real
`int[2]`, passes that array to `GetPosMenuItemOnItemBrd(int *, int, int)`,
and reads its two elements when placing the gift form. The callee writes
both output elements; a pointer to the first field of a `CursorPos` record
is not an array covering both fields.

The local zero initializer still emits the retail eight-byte template.
`INCLUDE_BSS(at_3509, 0x8)` remains the identity anchor required by the
unrelated PushKey switch-name collision described in
[data-migration-20261008.md](data-migration-20261008.md).
No external declaration of the gift template remains after data migration.

The complete PAL build verifies `SCES_511.90: OK` and all 149 objects pass
with the array form. Receipts:
`.private/fixes-r1b/receipts/gift-array-{build,objects}.log`.

## Retail function sizes

The header's `@size` values describe each function's declared retail symbol
size in `ps2/config/pal/main.symbols.txt`, excluding padding before the next
function. 56 padded tags are corrected. Every tagged symbol in this
header is present in that table, and the complete size audit has no mismatch.

The documentation changes preserve `SCES_511.90: OK` and 149/149 objects.
Receipts: `.private/fixes-r1b/receipts/header-sizes-{build,objects}.log`.
