# fishing data migration (2026-10-08)

Baseline `e1471bff`: **104 INCLUDE_RODATA / 106 INCLUDE_BSS**, with
**4 / 5145 matched_data** after the normal objdiff/progress refresh.
The unit retains 57 native functions and three guarded allocation functions.
The guarded drafts and `GetUkiWaitTime`'s SF-selected body remain unchanged.

## Typed fishing state

The 104 named reservations use documented native definitions with their
established pointer, integer, float, vector and header aggregate types. All
are file-local except the public `stack_size`. Retail symbol names remain
reachable, including every state object accessed by assembly fallbacks.
`RodData`, `FishData` and `BgmStatus` own 0x18, 0x24 and 0x1C bytes; their
0x20, 0x30 and 0x20 pieces include alignment supplied by the postprocessor.
The casting points are genuine four-float `Vec4` objects.

Primitive ripple/action/sound counters and their signed-byte initialization
flags use the accepted file-local counter form from menucapt, preserving all
existing initialization branches. `font_h_2216` is cleared once by `FalseLoop`
and never read or advanced; the native definition preserves that retail state
without adding a local or helper. Its guard is still a one-byte object.

The six memory managers and two camera-control objects retain their existing
constructor order, with documented file-local binding. Their native static
initializer is unchanged. After this group: **104 / 2 markers**,
**404 / 5145 matched_data**. PAL and 149/149 objects pass; unowned object file
hashes equal the warm baseline. Receipts:
`.private/dataD-r1/fishing-{state,storage}-{build,objects}.log` and
`fishing-state-{progress,metrics}.log`.
