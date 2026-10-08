# snd_mngr data migration (2026-10-08)

The fresh baseline has 23 rodata markers and 11 BSS markers, with
`matched_data` 4 / `total_data` 16808. The existing notes document all state
and classes used below; all unit functions already match.

## Compiler-generated literals and local templates

The native string expressions in the bank/sequence loaders, reverb parser,
and line reader emit their retail literals without the 17 string markers.
`at_816__2` is the compiler-generated game-port switch table of
`GetCSndPortNo`; its native switch emits the same code-target relocations.
The line reader's two-byte `{'\r', '\n'}` initializer has a two-byte retail
symbol and a four-byte section piece; it is not a null-terminated string.

The existing nine- and four-entry `char *col[]` local initializers emit
`at_1555` (0x24 bytes) and `at_1648` (0x10 bytes) as anonymous zero-filled
BSS templates. The first owns 12 bytes of alignment before the next piece.
The new shared tooling binds them by declared extent and the matching code
references, so no handwritten buffer or reinterpretation is required.

Acceptance: `.private/dataC-r2/snd-native-literals-{build,objects}.log` and
`snd-native-literals-metrics.json`. The whole image and all 149 objects pass,
and all unowned hashes are unchanged. Markers fall to 5 rodata / 9 BSS;
native data coverage is 250 / 16808.
