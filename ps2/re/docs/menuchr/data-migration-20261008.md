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

`monster_jyakuten` is a native seven-by-eight pointer table of weakness
labels. Its 224 bytes and all 56 pointers match retail, and thirty additional
child-string markers are absent. Non-ASCII strings preserve every retail
byte with hexadecimal escapes. Unsupported translated rows use the same
English fallbacks as retail.
After this step: **241 / 25 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-book-weakness-{build,objects,progress,metrics}.log`.

## Monster grid dimensions

`max_3170` and `viewnum_3171` are two-integer arrays, rather than scalars.
Their values are four columns and three rows for both total and visible
badge-grid size. Documented grid dimension enums name those values, and
`KeyNormalMode` passes the arrays directly to the existing pointer interface.
Both eight-byte objects match without padding or type reinterpretation.
After this step: **239 / 25 markers**, **4 / 9726 matched_data**.
PAL and 149/149 objects pass; receipts are
`.private/nmchr-r3/menuchr-grid-{build,objects,progress,metrics}.log`.

## Monster selection, model and book strings

Thirty-two additional strings are inline in their native consumers: monster
selection form/part names, box resource/configuration names, monster badge
labels, monster model/sound filename formats, book resources, and its debug
label. Every per-function group passes PAL and all 149 objects independently.
Shared strings retain their single retail identity and all original references.
After this group: **207 / 25 markers**, **4 / 9726 matched_data**.
Receipts: `.private/nmchr-r3/menuchr-monster-strings-00` through `-07`,
with `-{build,objects,progress,metrics}.log`, and `monster-string-batch.log`.

## Party menu strings

Thirty-nine party-menu form, resource, part, and debug strings are inline in
native consumers. This includes the character display resources and shared
empty-page state labels. Ten per-function groups independently pass PAL and
all 149 object checks; guarded draft bodies are unchanged.
After this group: **168 / 25 markers**, **4 / 9726 matched_data**.
Receipts: `.private/nmchr-r3/menuchr-party-strings-00` through `-09`,
with `-{build,objects,progress,metrics}.log`, and `party-string-batch.log`.

## Character loading resource strings

Seventeen resource strings are inline in twelve native consumer groups: sun
and moon frame names, the weapon frame, sound/debug labels, character and
ridepod resource names, model suffixes, outline texture formats, and background
load configuration names. Shared suffix users still resolve to one retail
literal. Each group passes PAL and all 149 object checks independently.
After this group: **151 / 25 markers**, **4 / 9726 matched_data**.
Receipts: `.private/nmchr-r3/menuchr-loading-strings-00` through `-11`,
with `-{build,objects,progress,metrics}.log`, and `loading-string-batch.log`.

## Party menu numeric tables

Five native tables replace their fallbacks: seven background-request flags,
five cursor reversal bytes, the five-by-eight command transition table, four
signed-halfword character message numbers, and three outcome sound IDs.
The transition, message and sound tables remain reachable by their exact
retail names from the frozen character-change draft. All declared extents,
payloads and resolved relocations match without source padding.
After this group: **146 / 25 markers**, **4 / 9726 matched_data**.
Each table passes PAL and 149/149 objects independently; receipts are
`.private/nmchr-r3/menuchr-party-tables-00` through `-04`, with
`-{build,objects,progress,metrics}.log`, and `party-table-batch.log`.

## Monster numeric tables

Six native definitions supply monster-selection and book background-request
flags, four badge-grid edge overrides, ten parameter display indices, ten
growth title message numbers, and fourteen resistance/weakness display masks.
The zero-valued override array retains the retail initialized section. The
mask table's fourteen entries are its declared 56-byte payload; its separate
eight-byte tail remains alignment. Frozen monster-selection code still
resolves the parameter and title tables under their retail names.
After this group: **140 / 25 markers**, **4 / 9726 matched_data**.
Each table independently passes PAL and 149/149 objects; receipts are
`.private/nmchr-r3/menuchr-monster-tables-00` through `-05`, with
`-{build,objects,progress,metrics}.log`, and `monster-table-batch.log`.

## Costume numeric tables

Five native definitions supply seven costume background-request flags, three
load phases, the four-byte tile colour, three equipment slot indices, and
seven localized help-panel widths. Mutable table definitions preserve the
retail loads; the previous const equipment-table probe changes those loads
and is not repeated. Guarded costume loading still resolves its request table
by the retail name.
After this group: **135 / 25 markers**, **4 / 9726 matched_data**.
Each table passes PAL and 149/149 objects independently; receipts are
`.private/nmchr-r3/menuchr-costume-tables-00` through `-04`, with
`-{build,objects,progress,metrics}.log`, and `costume-table-batch.log`.

## Background load index tables

Three native signed-byte tables supply alternate model directory selection,
six ridepod load phases, and the four-by-seven mapping from party load slots
to temporary scene character slots. The scene conversion table has a genuine
28-byte declared payload, including its negative absent-slot sentinels.
After this group: **132 / 25 markers**, **4 / 9726 matched_data**.
Each table passes PAL and 149/149 objects independently; receipts are
`.private/nmchr-r3/menuchr-loading-tables-00` through `-02`, with
`-{build,objects,progress,metrics}.log`, and `loading-table-batch.log`.

## Native local aggregate initializers

Ten accepted conversions supply nine initialized objects and nine BSS
templates from natural local initializers:

- Character load completion initializes two seven-pointer scene lists and
  one eight-pointer load-target list.
- Ridepod completion initializes its six-pointer scene and stack lists and
  seven-pointer menu list. The initialized menu stack pointers select
  `MenuActionCharaBuffer` entries 0, 1, 2, 2, 3 and 2.
- Character change initializes the genuine 64-byte filename buffer with
  `"chrchg0.pac"`; character loading initializes seven path-category bytes.
- Quick-change loading initializes its nine-integer wanted list.
- Monster completion initializes seven scene pointers, three target pointers,
  and three stack pointers. Its two non-null stack pointers both address
  `MenuActionCharaBuffer[5]`, matching retail's `+0xF0` relocations.
- Sound entry initializes a four-byte signed map `{3, 3, 1, 0}` directly,
  replacing the float copy cast without changing instructions.
- NPC positioning initializes the existing SDK `sceVu0FVECTOR` with
  `{14.0f, 0.0f, 0.0f, 1.0f}`. The SDK's vector alignment matches retail,
  and the `MenuPositionVector` copy overlay is absent.
- Costume defaults initialize four integer IDs `{0, 0, 0, -1}` directly.
  The former quadword union is absent; the fourth value is inside the real
  16-byte declared initializer, rather than appended source padding.
- Monster book information initializes its two 32-byte area names and
  eight-integer weakness list directly. Both quadword-copy overlay types
  and their casts are absent.

Named array order is preserved when a declaration moves to its initializer.
After this group: **123 / 16 markers**, **4 / 9726 matched_data**.
Every accepted conversion passes PAL and 149/149 independently; the final
restored state also passes. Receipts use `.private/nmchr-r3/menuchr-` plus
`character-targets`, `ridepod-targets`, `character-menu-file`,
`character-paths`, `load-wanted`, `monster-targets`, `sound-stacks`,
`npc-position`, `worn-costumes`, or `monster-book-info`, with
`-{build,objects,progress,metrics}.log`. The group receipt is
`aggregate-batch.log`; the final receipt is `menuchr-aggregates-final`.

The grouped debug-buffer conversion preserves native instruction words but
fails BSS data naming: an eight-byte `at_970` template stays unnamed where
`at_2232` is required. The first two-name pointer-array probe also leaves
instructions unchanged, but its first pointer is wrong: retail requires a
single-space string (`at_2286`), followed by the empty string (`at_2287`);
the probe incorrectly uses two empty strings. Both attempts are restored.
Failed source checkpoints, object snapshots, focused checks, PAL section
differences and instruction comparisons are under `menuchr-debug-templates-*`
and `menuchr-party-name-pair-*`. Instruction masking cannot validate data
pointer destinations; the resolved-relocation check rejects the wrong pair.

## Named pointer tables and their child literals

Nine native pointer tables supply the party gauge label parts, localized
monster-selection help, badge numeric and item-number label parts, character
and monster resource directories, localized costume help, and the two main
characters' model/configuration filenames. Forty-three child string markers
are also absent; their bytes and `R_MIPS_32` destinations come from the native
inline initializers. The badge tables remain reachable under their retail
symbols from the frozen monster-selection method.
After this group: **71 / 16 markers**, **4 / 9726 matched_data**.
Each accepted table passes PAL and 149/149 independently, and the restored
final state passes. Receipts are `.private/nmchr-r3/menuchr-pointer-tables-01`
through `-09`, with `-{build,objects,progress,metrics}.log`, plus
`menuchr-pointer-tables-final` and `pointer-table-batch.log`.

The four-pointer townsperson command-cost table `tbl_1233` is restored.
Its natural file-scope definition leaves all native instruction words intact,
but reproduces the unnamed `at_2232` BSS template seen in the debug-buffer
attempt. Receipt `menuchr-pointer-tables-00-*` records the original source,
linked/native objects, section differences and the focused object check.


## Initialized menu state and camera defaults

Four native scalar definitions retain their signed `-1` sentinel: the current
sound character, pending monster model, pending main character, and pending
main-character monster. The monster selection camera position and reference
are native SDK vectors `{0.0f, 0.0f, 100.0f, 1.0f}` and
`{0.0f, 9.0f, 0.0f, 1.0f}`. The named information configuration buffer has
its genuine ten-byte declared capacity and inline `"info.cfg"` initializer.
All seven definitions keep the existing interfaces and retail symbol names.
After this group: **64 / 16 markers**, **4 / 9726 matched_data**.
Every definition independently passes PAL and 149/149; receipts use
`.private/nmchr-r3/menuchr-named-` plus its symbol name, with
`-{build,objects,progress,metrics}.log`, and `named-data-batch.log`.

## Item name pointer initializer

The native two-pointer array in `MenuLocalLoop` initializes with `{" ", ""}`,
matching the retail `R_MIPS_32` targets `at_2286` and `at_2287`. The first
pointer is later replaced by the item's name when the item exists. The
`NamePair` wrapper and its generic fields are absent; the existing setter
receives the real pointer array directly.
After this step: **63 / 16 markers**, **123 / 9726 matched_data**.
The complete small initialized-data section now receives progress credit.
PAL and all 149 objects pass; receipts are
`.private/nmchr-r3/menuchr-party-pair-corrected-{build,objects,progress,metrics}.log`.

## Debug text buffer initializers

The three 128-byte debug lines use nested zero initializers `{{0}}`. The
512-byte character-name block and 256-byte townsperson text block initialize
with their actual inline text and retain their full declared buffer capacities.
All five fallback templates are absent. The zero buffers were checked one at
a time, separately from the earlier grouped string-spelling probe.
After this group: **61 / 13 markers**, **123 / 9726 matched_data**.
PAL, all 149 objects, unchanged unowned hashes and frozen-source hashes pass.
Receipts: `.private/nmchr-r3/menuchr-debug-one-at_2674`, `-at_2675`,
`-at_2676`, and `-at_2691`, plus `menuchr-debug-complete`, with
`-{build,objects,progress,metrics}.log`. The last text conversion's original
checks also pass; its validation-summary command failed independently, so
`menuchr-debug-complete` supplies the complete final receipt.
