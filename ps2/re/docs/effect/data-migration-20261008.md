# Effect data migration

## Baseline

At lane checkpoint `ab376093`, all 75 functions match. The unit has 51
`INCLUDE_RODATA` markers and four `INCLUDE_BSS` markers. Refreshed objdiff
credits 44/1,124 data bytes. The warm build reports `SCES_511.90: OK` and
all 149 objects pass the canonical comparison.

## Script-loader storage

The existing matched code and type notes establish `g_tmp_effm` as a
`CEffectManager *`, `g_tmp_effc` as a `CEffectCtrl *`, `g_eff_entry_flag`
as an `int`, and `g_tmp_eff_name` as a 32-byte emitter-name buffer. Native
file-static definitions replace their reservations in retail address order.
No class layouts or public declarations change.

Acceptance receipts are `.private/dataC-r1/effect-storage-build.log`,
`effect-storage-objects.log`, and `effect-storage-metrics.log`: PAL OK,
149/149 objects, and no unowned object hash changes. Markers become 51/0.
