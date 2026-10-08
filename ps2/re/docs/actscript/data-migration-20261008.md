# Native data migration — 2026-10-08

## Result

The unit changes from 39 initialized-data markers and 10 BSS markers to 0 and 6.
Refreshed objdiff reports `matched_data` 1040/2416 before and after; anonymous
initializer data is not credited as a named matched object by this report.
All 95 functions remain matched. No Satan’s Fiddle profile rows change.

## Definitions and literals

`nowScene__2` and `action_info` use their documented header types.
The local collision-window pointer is `static ACTION_DAMAGE *LastCInfo2` and the
script dispatch slots are `static int (*ext_func[256])(RS_STACKDATA *, int)`.
The local dispatch description is a typed `RS_EXTFUNC_INFO[83]`: 82 handlers in
retail table order followed by `{NULL, ACTION_EXT_END}`. `ACTION_EXT_NUMBER`
documents each existing script external number. Static prototypes establish the
retail local linkage of the handlers before their addresses are initialized.
The table is 0x298 bytes; its former assembly piece carried another eight zero
alignment bytes, now supplied by the linker. Canonical comparison accepts this
zero tail and checks the same resolved function addresses.

Direct strings use their exact retail bytes, including Shift-JIS strings and
escaped adjacent literals where a hexadecimal escape would otherwise consume
an ASCII hex digit. The existing Ridepod movement switch emits its own jump table.
`_SET_BLOW_MOVE` initializes its forward vector at the original copy site and
places the matrix declaration after that vector. `_RELEASE_OBJ` initializes the
throw offset only in the throwing branch. `_SET_SPECIAL_SHOT` initializes four
typed effect-name pointers at their use, removing the integer pointer table and
quadword-copy cast. `_GET_RING_COLOR` initializes the four RGB rows directly.

`_SHOT` initializes `CanonObjectNames` inside its cannon branch. Its later target
buffers, weapon-health array and beam offset remain in their original declaration
order after that branch. This retains the exact frame layout, instructions,
relocations and float selector identities while emitting the eight frame names
and their pointer table naturally. The constant-copy-only `ScriptVector` union
is no longer needed.

## Remaining reservations

`sw_1617`, `init_1618`, `canon_slot_1620`, `init_1621`, `cnt_1661` and `init_1662`
reserve the native `_SHOT` statics and their compiler-generated guards. Source
already expresses these as `static int sw = 1`, `static int canon_slot = 0` and
`static int cnt = 0`. Removing the six markers preserves instructions but leaves
MWCC’s generated names (`sw_861`, `init_862`, `canon_slot_864`, `init_865`,
`cnt_905`, `init_906`) unmapped. The focused check reports missing/unexpected
`.sbss` pieces and unresolved relocations; it does not report instruction-byte
differences. The markers remain until the tooling lane supports native local
static/guard identity mapping. No synthetic locals or names are used to alter
MWCC’s generated counters.

## Verification receipts

- `.private/dataA-r2/actscript-native-dispatch.log`
- `.private/dataA-r2/actscript-native-vectors.log`
- `.private/dataA-r2/actscript-native-effects.log`
- `.private/dataA-r2/actscript-native-cannon.log`: whole unit passes,
  0x47F4 compared bytes and 1111 relocations.
- `.private/dataA-r2/actscript-unmarked-locals.log`: retained-reservation blocker.
- `.private/dataA-r2/actscript-build.log`: `SCES_511.90: OK`, 6763 perfect,
  zero fuzzy, 109 assembly, zero unmatched.
- `.private/dataA-r2/actscript-objects.log`: 149/149 units pass.
