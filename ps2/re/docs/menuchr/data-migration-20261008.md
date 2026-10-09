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

## Monster progression table

`monster_progress_tbl` is a native nineteen-by-five signed-halfword array:
each row contains its badge number followed by four monster forms. Its
190-byte declared extent matches the documented searches and header, and its
two-byte piece tail remains alignment. The existing consumers are unchanged.
After this step: **354 / 31 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass with unchanged unowned hashes; receipts are
`.private/nmchr-r3/menuchr-progress-table-{build,objects,progress,metrics}.log`.

## Party-menu resource and layout strings

The ten resource, form, palette-part, and message-file strings in
`CMenuChrCngMenu::EnterDataMenu` are inline at their uses. Their external
literal declarations and data markers are absent. Every string retains its
retail bytes and references, including shared uses in other native functions.
After this step: **344 / 31 markers**, **4 / 9726 matched_data**.
PAL and all 149 objects pass; receipts are
`.private/nmchr-r3/menuchr-enter-strings-{build,objects,progress,metrics}.log`.

## Party-menu zero aggregate templates

The two `SmallPair` locals in the party key handler now initialize their real
integer arrays with `{{0, 0}}`, supplying `at_2232` and `at_2289__2` naturally.
The star draw's existing center and UV aggregate initializers also supply
`at_2371__4` and `at_2372__4`, so those two fallbacks are removed.
After this step: **344 / 27 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass with unchanged unowned hashes; receipts are
`.private/nmchr-r3/menuchr-party-zero-{build,objects,progress,metrics}.log`.

The eleven party form/part strings in `SetFormPointInfo` are also inline,
including the health gauges, model panels, and formatted command/name parts.
After this step: **333 / 27 markers**, **4 / 9726 matched_data**.
Validation remains PAL OK and 149/149; receipts are
`.private/nmchr-r3/menuchr-party-parts-{build,objects,progress,metrics}.log`.

## Native switch tables and virtual tables

The existing native switches supply `at_3970` (equipment pack loading) and
`at_5197` (costume key handling). The existing monster-selection and
monster-book constructors supply their virtual tables. The native frame
lookup already contains the inline `"light"` string, supplying `at_4517__2`.
All five markers and the unused light-string declaration are removed.
The two virtual tables referenced by guarded initializers remain fallbacks.
After this step: **328 / 27 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-generated-{build,objects,progress,metrics}.log`.

## Work-stack adjustment data

`MenuMemoryAdjust` initializes its seven real memory pointers with a null
aggregate and uses the inline `"LOAD STACK"` name. These source forms supply
`at_1083__2` and `at_1104__4` without external template/string declarations.
After this step: **327 / 26 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-adjust-{build,objects,progress,metrics}.log`.

## Character equipment load phases

`tbl_992` is a documented native signed-halfword array with twenty entries:
five equipment phases for each `USER_CHARA`. Its exact forty-byte declared
extent includes the observed unused zero entries for monster mode, without
appended filler. `ConvertCharaLoadDataPhase` retains its existing typed indexing.
After this step: **326 / 26 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-phase-table-{build,objects,progress,metrics}.log`.

`CMenuMosSelect::CheckLoadBGMonster` now initializes its seven-pointer local
character-target array directly with null pointers, supplying `at_3054__2`.
After this step: **326 / 25 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-monster-target-{build,objects,progress,metrics}.log`.

## Localized monster-book page formats

`monstere_file_template` is a native seven-pointer table with inline space,
English, French, German, and Spanish strings. Japanese uses one space;
Italian and Chinese reuse English. The declared 28-byte extent excludes its
four-byte alignment tail. The current named-pointer-table literal binding
accepts the table and all five child strings, unlike the earlier rejected
probe recorded in `midday-book.md`. All seven resolved pointers match retail.
After this step: **320 / 25 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-book-format-{build,objects,progress,metrics}.log`.

`monster_type_name` is a native seven-by-twelve pointer table of monster
family labels, with every string inline in its initializer. Japanese has
twelve empty labels; English, French, German, and Spanish have their retail
labels; Italian and Chinese reuse English. The 336-byte declared extent,
all 84 pointers, and all 47 former child-string pieces match. Shared native
empty-string consumers now use the inline empty literal as well.
After this step: **272 / 25 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-book-types-{build,objects,progress,metrics}.log`.
