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
