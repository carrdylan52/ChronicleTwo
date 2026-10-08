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

## Native identity proposal validation

The remaining twelve initialized-data and seven BSS markers can be removed without changing any function
instructions. The exact source cleanup is
`.private/proposals/menucls1-remove-native-data-markers.patch`; it requires
`.private/proposals/native-local-data-identities.patch` for the object
postprocessor. The retained source keeps its markers until that tooling change
is integrated. No build script or Satan's Fiddle profile is changed in this lane.

The proposal requires every live incoming BSS reference to belong to a complete,
byte-identical retail function with matching declared extent. It resolves the
other relocation destinations, validates HI16/LO16 pairs in emitted record
order, and requires one agreed destination, the same source base name, exact
retail object size, the correct section, and a sole zero-offset object definition.
For initialized named locals, exact symbol and section sizes and the retail
section kind are checked before naming; existing bounded alignment padding is
applied afterward. Pointer-table literals are named before their tables.

The exact marker-free source was freshly compiled privately through both
mwccgap passes with the pinned image and unchanged profile. Its final object
passes 0x2B00 bytes and 304 resolved relocations. Combining the four private
replacement objects with the unchanged stock objects passes 149/149 checks,
and the resulting linked PAL passes every section and memory-extent check.
This staged validation does not install the proposal or claim a fresh whole-tree
build with modified tooling.

Receipts: `.private/dataA-r2/resume-proposal/menucls1-final-check.log`,
`final-objects.log`, and `final-verify.log`. The private safety harness receipt
`.private/dataA-r2/proposal-safety-v2.log` contains 25 successful checks,
including rejection of changed instructions, truncated functions, changed calls,
non-code consumers, duplicate relocation sites, orphan lows, wrong base names,
object aliases, wrong sizes and sections, nonlocal bindings, and changed table
pointers. The untouched canonical control passes; both initialized-local
metadata counterexamples are rejected by the canonical checker.
