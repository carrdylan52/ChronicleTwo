# Native menu helper data

`MenuBigNum` is a typed ten-pointer definition with the full-width zero-through-nine
strings inlined in its initializer. Its header already documents the global.
The forty-byte table and its ten padded glyph strings pass the complete object.
The three item-use result globals use the established `int`, `u32`, and `int`
types documented in `menucls1.hpp`.

The question-mark strings, empty string, and form/part names are inlined at their
uses. The existing native message-preset switch, two-element volume initializer,
and four-element unavailable-item initializer emit their data without markers.
`GetHatena` contains the two real local pointer statics rather than explicit
external variables and guard tests.

`SetMenuBigNum` contains its native local ten-pointer ASCII digit table. Its
glyph pointers use `char *`, matching the strings and `GetMenuBigNum` result;
the former signed-character pointer casts are unnecessary for the copied bytes.
`MenuUseItemCheckFunc` contains its seven-entry local status-bit table. Both
local-table replacements retain every instruction byte and resolved relocation
with the existing storage markers.

## Remaining reservations

Twelve initialized-data markers remain: `sn_944`, its ten ASCII digit strings,
and `st_bittable_1654`. Seven BSS reservations remain: the two question-mark
pointer statics and their guards, plus the three anonymous zero initializers
for the message argument arrays.

Removing these markers produces correctly generated source objects, but the
current postprocessor does not assign retail names to the two generated named
local tables or the generated BSS objects. The checker rejects their pieces and
unresolved references. Keeping the `sn_944` placeholder also causes the native
digit strings reachable only from its shadow table to be discarded, so its ten
string markers cannot be removed independently. This is a binding limitation;
no fake compiler-counter names or file-scope substitutes are added.

The retained source passes 0x2B04 checked bytes and 304 resolved relocations.
Focused receipts under `.private/dataA-r2/`: `menucls1-used-strings.log`,
`menucls1-existing-initializers.log`, `menucls1-fullwidth-digits.log`,
`menucls1-question-statics.log`, `menucls1-local-tables.log`, and
`menucls1-native-storage.log` (rejected unbound native storage).

The full build retains `SCES_511.90: OK`, and all 149 objects pass. Refreshed
`matched_data / total_data` is 12 / 548 (baseline 12 / 548).
Whole-project receipts: `.private/dataA-r2/menucls1-build.log` and
`menucls1-objects.log`.

## Status-bit names and digit pointer types

The local status table uses the existing `CHARA_STATUS_ATTR` enumerators in
ascending bit order. These are the same seven masks consumed by the item
add/cure loop; no shared enum or header changes are required. `SetMenuBigNum2`
uses `char *` for the full-width glyph returned by `GetMenuBigNum`, removing
its two signed-character pointer casts. The two question-mark statics and both
local initialized tables have purpose comments at their definitions.

The focused object passes all 0x2B04 bytes and 304 resolved relocations.
The full build retains `SCES_511.90: OK`, and all 149 objects pass.
Receipts: `.private/dataA-r2/menucls1-status-enum.log`,
`menucls1-status-enum-build.log`, and `menucls1-status-enum-objects.log`.
The reservation counts and data coverage remain 12 initialized-data markers,
7 BSS markers, and 12/548 matched data bytes.

## Native status-table section

Retail `st_bittable_1654` is a mutable 0x1C-byte object in `.data`.
A native `static const u32[7]` instead emits `.rodata`; the retained marker
had hidden that difference. Its source definition is `static u32[7]`, retaining
the same enumerator initializers and item-use instructions while matching the
retail storage kind. The stricter proposed local-data mapper rejects a native
section-kind mismatch rather than rewriting it into the retail kind.

Both the retained-marker object (0x2B04 bytes) and the fresh marker-free
proposal object (0x2B00 bytes) pass all 304 resolved relocations. The private
proposal leaves the final four zero alignment bytes to the linker. Full stock
build and object receipts: `.private/dataA-r2/menucls1-mutable-status-build.log`
and `menucls1-mutable-status-objects.log`; focused receipt:
`menucls1-mutable-status.log`. The fresh proposal receipt is
`.private/dataA-r2/resume-proposal/menucls1-check-v4.log`.
