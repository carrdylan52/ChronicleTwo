# Shared fog colour proposal

The independent RGBA-overlay trial uses base 93cbbea, MWCC 3.0-011126,
the canonical profile, and `chronicletwo_dev:sf-d8bf13c` with JOBS=4.
`mgFOG_PARAM` stays 0x30 bytes, with near/far distances at 0/4 and the
colour bytes at 8..11. Retail's editor merges RGB cases 2..4 and accesses
the selected unsigned byte through its base-plus-index displacement of 6.
The renderer, fog setter, map lighting and script consumers use the named
RGB fields. The proposal adds an anonymous union alongside those fields;
it does not replace the editor's noncompliant address expression.

The full build and checker retain 147/149 and the sole PAL 0x26-byte text
discrepancy. No allocated bytes, layout or relocation identities change.
One of the 149 whole-file hashes changes: sceneload's nonallocated `.strtab`
renames unused `@211`/`at_211` strings to `@213`/`at_213`. All other section
data in that object are unchanged.

All 38 affected guarded units are compared with both plain MWCC and the
canonical profile; no canonical draft changes. `LightingEdit` remains
0/1356 differing words under the canonical profile because its inherited
raw-address expression is still present. This is not a compliant promotion.
The earlier typed-index results in [notes.md](notes.md) remain applicable
and are not repeated. The overlay supplies no new matching typed expression
or independently justified need for both views. It is not retained.

Receipts: `.private/shared-eval/experiments/fog-color-array/` and
`.private/shared-eval/guard-sweeps/fog-color-array/results.json`.
