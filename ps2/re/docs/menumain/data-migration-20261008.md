# menumain data migration (2026-10-08)

Baseline `e1471bff`: **82 INCLUDE_RODATA / 54 INCLUDE_BSS**, with
**4 / 3490 matched_data** after the standard objdiff/progress refresh.
The unit retains 52 native functions and one guarded `MenuMainInit`.

## Named menu state

Forty-nine reservations are native definitions using the existing documented
header and consumer types. Public globals keep external linkage; retail-local
state keeps internal linkage. The saved camera vectors retain four float
components. `MENU_INIT_ARG`, `CMenuItemUse` and `CMenuInter` own 0x98, 0x1C and
0x18 bytes; their zero tails to 0xA0, 0x20 and 0x20 are alignment supplied by
the existing data postprocessor, rather than extra members. One-byte flags and
two-byte map/topic values likewise retain their actual declared sizes.
The player-data refresh counter and initialization flag are signed bytes,
with the existing 25-frame refresh behavior unchanged.

The exact `MenuArg`, `MenuItemUse` and saved-vector symbols referenced by the
guarded initializer remain defined and reachable. All guarded source and both
SF-calibrated function bodies compare equal to the baseline text. Full PAL and
149-object checks pass; the only changed object hashes are the two owned units
worked so far. Receipts: `.private/dataD-r1/menumain-state-{build,objects,progress,metrics}.log`.
