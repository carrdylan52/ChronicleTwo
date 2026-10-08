# Native action character data

All 47 initialized-data markers are replaced by source-emitted data. Strings
are inlined at their uses with exact retail bytes, including the Shift-JIS
motion and effect names. Existing movement-vector initializers, the
`CheckDamage` switch, and the compiler-generated `CActionChara` virtual table
emit their corresponding sections without the duplicate markers.

`EntryThrowItem` contains its nineteen-item aggregate initializer. The eighteen
item numbers select the corresponding item model slots, followed by the -1
terminator. The object is 76 bytes with a four-byte alignment gap.

`HitEffectSet` and `GuardEffectSet` initialize their four-float directions at
the former constant-copy sites. `StepParam` initializes its two forward vectors
in the branches where they are used. The associated matrices and knockback
buffers are declared in their use scopes, preserving the retail stack layout.
Every focused step passes all 0x8F80 checked bytes and 1,035 resolved relocations;
the Satan's Fiddle profile is unchanged.

The previous stick direction is a documented static `float old_angle` definition.
`Step` uses a native `static float ang = 0.0f` instead of an explicit external
angle and initialization guard. Its generated initialization instructions match
retail while the two corresponding BSS reservations remain.

## Remaining BSS reservations

Three markers remain: `ang_3371`, `init_3372`, and `at_3107`. The first pair
belongs to the native arm-angle local static; the final sixteen bytes are the
compiler-generated zero initializer for `RunScript`'s `adjusted_velocity`.
Removing the markers preserves instruction bytes but the current postprocessor
does not bind their generated BSS names to retail symbols. The canonical
checker rejects the anonymous BSS pieces and their unresolved relocation targets.
No compiler-generated names, manual guards, or substitute storage definitions
are invented to conceal this mapping limitation.

Receipts are under `.private/dataA-r2/`: `actionchara-rodata-strings.log`,
`actionchara-emitted-data.log`, `actionchara-stick-state.log`,
`actionchara-throw-items.log`, `actionchara-hit-direction.log`,
`actionchara-guard-direction.log`, `actionchara-forward-scopes.log`,
`actionchara-arm-static.log`, and `actionchara-native-bss.log` (rejected).

The full build retains `SCES_511.90: OK`, and all 149 objects pass. Refreshed
`matched_data / total_data` is 288 / 1124 (baseline 288 / 1124).
Whole-project receipts: `.private/dataA-r2/actionchara-build.log` and
`actionchara-objects.log`. No header or Satan's Fiddle row changes are needed.
