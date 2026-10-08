# editevent data migration (2026-10-08)

The fresh round-2 baseline has 20 rodata markers, no BSS markers, and
`matched_data` 0 / `total_data` 324. All nine functions already match; the
existing notes own the event/state and NPC-loading analysis.

## Event and NPC literals

Twelve diagnostic, motion, frame, and configuration strings are inlined at
their existing calls. The exact Shift-JIS motion bytes use hexadecimal
escapes. `at_1152` and `at_1154` are generated switch tables of `CEditEvent::Step`;
the native switches emit the same target offsets without handwritten data.

Acceptance: `.private/dataC-r2/editevent-literals-{build,objects}.log` and
`editevent-literals-metrics.json`. The full image and all 149 objects pass,
and all unowned object hashes remain unchanged. Six markers remain: the
four house suffix strings, their initializer template, and the menu pointer.

## House suffixes and menu arguments

The door branch initializes `char *suffix[4] = {"ia", "ib", "ic", "id"}`
at the point the original template is copied. This generates `at_920__4`
and the four string-pointer relocations naturally, replacing the quadword
union and reinterpretation. The default suffix remains the literal `"ia"`.
The menu state pointer becomes a documented file-local `MENU_INIT_ARG *`
initialized to `&MenuArg`, preserving its four-byte .sdata extent.

Final acceptance: `.private/dataC-r2/editevent-native-data-fixed-{build,objects}.log`
and `editevent-native-data-fixed-metrics.json`. No markers remain and native
data coverage is 324 / 324. The complete image, all 149 objects, and every
unowned object hash pass. No data or tooling proposal is parked.
