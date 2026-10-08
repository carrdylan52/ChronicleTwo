# dataread data migration (2026-10-08)

The fresh round-2 baseline has 19 rodata markers and 4 BSS markers, with
`matched_data` 552 / `total_data` 339554. All functions already match and
the existing notes document the file/cache/device structures and behavior.

## File paths and diagnostics

All 19 string markers and their extern declarations are removed. File and
device names, current-directory reset strings, and load/cache diagnostics
are literals at their existing uses. The two background-read diagnostics
already occur as native literals and need only their markers removed.
The exact disc filenames keep the leading backslash and `;1` suffix.
The empty string and slash are distinct retail objects, emitted naturally
by the two default-device transitions.

Acceptance: `.private/dataC-r2/dataread-literals-{build,objects}.log` and
`dataread-literals-metrics.json`. The full image and all 149 objects pass,
and every unowned object hash is unchanged. No rodata markers remain;
coverage is 850 / 339554, pending the four local BSS templates.
