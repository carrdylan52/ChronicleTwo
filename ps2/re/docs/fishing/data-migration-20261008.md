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

## Matched-function resource strings

Twenty-six resource, animation and effect strings are inlined at their native
call sites. Shift-JIS literals preserve their bytes with hexadecimal escapes.
The cursor's native `{"NG", "OK"}` initializer also supplies its pointer
table, so `at_1444__3__DATA` is removed in the same step. Removing only the two
string markers leaves the assembly table's relocations unresolved; the combined
natural initializer passes. Strings consumed by guarded functions remain under
their original symbols.

Each function's string group passes PAL and 149/149 objects. After this group:
**77 / 2 markers**, **412 / 5145 matched_data**. The refresh is recorded in
`.private/dataD-r1/fishing-strings-{progress,metrics}.log`; final string validation
is in `fishing-fish-model-{build,objects}.log`, with preceding function groups
recorded separately. Protected bodies and unowned object hashes are unchanged.
