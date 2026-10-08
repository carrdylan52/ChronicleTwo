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
