# menuchr data migration (2026-10-08)

Baseline `860067a4`, image `chronicletwo_dev:sf-63f7a9e`:
**355 INCLUDE_RODATA / 79 INCLUDE_BSS**, **4 / 9726 matched_data**.
The seven guarded drafts and all Satan's Fiddle rows remain unchanged.
Header declarations remain source-compatible; menudraw is outside this lane.

## Exported character-menu state

Nine reservations use the existing documented header types: `MorattaStack`,
`MenuLoadInfo`, the three party-ring texture pointers, `CharaSndBuffer`,
`MenuCharaBuild2`, `MenuActionChara`, and `MenuLoadItemNo`. The last three
arrays contain seven pointers, seven pointers, and twelve signed halfwords.
Their declared sizes are 28, 28, and 24 bytes. Alignment after the latter
two arrays belongs to the reservation extent, without filler fields.
`D_01F3C7FC` remains the distinct four-byte boundary after `MenuCharaBuild2`.

After this step: **355 / 70 markers**, **4 / 9726 matched_data**.
The full build passes PAL verification and all 149 objects pass with resolved
relocations. Only owned-unit raw object hashes change from the warm baseline.
Receipts: `.private/nmchr-r3/menuchr-exported-{build,objects,progress,metrics}.log`.

## File-local menu state

Thirty-nine named reservations are documented file-local definitions with
their existing consumer types. These hold party-menu palettes and selection
state, monster-menu resources, background character load/slide state, costume
state, book textures and pointers, and the four transformation-effect buffer
arrays. One-byte and two-byte flags retain their exact retail declared sizes;
the existing postprocessor supplies the larger reservation padding.
`MenuMonsterBGInfo` has seven request pointers, matching its 28-byte retail
symbol and the seven-slot initialization loop, rather than the old eight-entry
declaration. Its four-byte piece tail is alignment.

After this step: **355 / 31 markers**, **4 / 9726 matched_data**.
PAL verification and all 149 object checks pass, with all unowned hashes
unchanged. The guarded drafts still reference the same retail symbols.
Receipts: `.private/nmchr-r3/menuchr-local-state-{build,objects,progress,metrics}.log`.
