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
