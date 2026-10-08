# nowload data migration (2026-10-08)

Baseline: `95f8fdd1`; 10 initialized-data
markers and 20 BSS markers. Every function already matches.
The pinned `chronicletwo_dev:sf-63f7a9e` image and full object/PAL checks
validate each accepted step. Public declarations remain compatible.

## Loading and pause state

The loading-thread ID, progress ratios/counter, cancellation/end flags,
pause flags/counter/texture block, saved master volume, stream/play-time
state and logo VSync count are native file-local scalars. `LoopStep` is an
initialized integer using `NOW_LOADING_STEP_NONE` (-1); it replaces the
four-byte initialized-data marker. Existing `LoadInfo` and `PauseInfo`
construction order and both runtime bodies remain unchanged.

The thread stack is `u8[0x1000]`; the skip-image buffer is `u8[0x2800]`.
`bgm_status` retains its existing seven-word representation: only its first
word is read, to decide whether `PauseEnd` replays music. The retail declared
extent is 0x1C; four following zero bytes are piece alignment. The other six
words have no established semantic names, so no speculative status struct
or field interpretations are introduced.

All twenty BSS markers and the `LoopStep` marker are removed. Receipt
`.private/dataD/nowload-state-{build,objects,metrics}.log` records PAL OK,
149/149 objects and unchanged unowned hashes. Interim data is 14,512/14,645.
