# Title data migration

Round-5 baseline: `8a85b297`, canonical MWCC 3.0-011126 flags and
`chronicletwo_dev:sf-63f7a9e`. The warm build passes `SCES_511.90: OK` and
149/149 complete objects. Fresh objdiff measures are 61 RODATA markers,
76 BSS markers and **4 / 2064 matched_data / total_data**.

## Ownership boundary

The guarded TitleBootInit block, its data declarations and its markers are
unchanged. The protected set is the union of its draft identifiers and the
retail ELF relocation targets. Both sets agree for title's retained markers.
Existing title.hpp declarations and all other units remain source-compatible.
The existing title investigation notes and the SF profile are unchanged.

## Title state storage

Typed definitions replace the reservations for the first-boot attract flag,
camera animation state, debug unlock byte, demo movie selection, extras and
costume bits, active memory card port, memory card message pointer, title
phase and prompt pulse direction, trial fade state, copyright/logo phase and
counter, logo skip flag, E3 trial state, START indicator, title extra flag,
boot card-check state and attract movie state.

The byte and halfword definitions use their documented retail access widths;
the four-byte marker slots include alignment rather than additional fields.
`RushInfo` is the existing 0x18-byte RUSH_INFO, with its complete 0x20-byte
piece supplied by verified NOBITS reservation padding. MasterDebugModeOn and
CostumeOptionEnv retain external linkage; unit-owned state has file linkage.
The existing header defines and documents all required types.

Each datum has its own incremental PAL build, all-object check and audit under
`.private/dataB-r5/bss-<name>-{build,objects,audit}.log` (audit files use `.json`).
The audit requires every object outside title to retain its baseline file hash
and verifies the protected declarations, markers and complete boot block.

## Title state validation

RODATA / BSS markers: **61 / 76 → 61 / 49**. Fresh objdiff data measures
remain **4 / 2064**. Aggregate section credit remains incomplete while the
remaining title reservations are present. No function is promoted.

Receipts: `.private/dataB-r5/state-final-{build,objects}.log`,
`state-final-audit.json`, `state-progress.log` and `state-metrics.json`.

## Installer and language storage

Fourteen typed installer definitions supply the phase, confirmation action,
slideshow selection, drawing flags, texture and message pointers, menu
selection, ten texture pointers and ten image alphas. Both arrays have the
retail 0x28-byte declared extent; the eight bytes through each 0x30-byte piece
are alignment. No extra array entries or filler objects are declared.

Six definitions supply the language selection, phase, cursor position, fade
alpha, cursor animation count and texture pointer. The language phase is an
int, even though its marker reserves eight bytes. Its exact four-byte declared
extent owns the verified gap to the next piece.

TitleHDDInstallDraw uses a natural `static int count = 0` inside the controls
branch. MWCC emits both the counter and its one-time initialization guard,
replacing count_2647/init_2648 and their manual initialization scaffolding.
All function bytes and resolved relocation targets remain exact.

RODATA / BSS markers after this group: **61 / 27**. Fresh objdiff data
measures are **468 / 2064**. The large BSS section is now fully native.
Receipts: `.private/dataB-r5/bss-hdd-steps.log`, `bss-lang-steps.log`,
`cursor-static-{build,objects}.log`, `storage-final-{build,objects}.log`
and `storage-metrics.json`.
