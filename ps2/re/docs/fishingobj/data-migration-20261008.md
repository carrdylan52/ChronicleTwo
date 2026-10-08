# Native fishing data

The 28 BSS reservations are replaced by documented static definitions. The
sixteen scalar state values keep their established types, including the floating
`NowFishRot`. The larger objects use `FISH_POINT`, `FISH_ROD_SEGMENT`, frame
pointers, and four-float vectors according to the existing layout evidence in
`decompilation.md`.

`RodPoint` is five `FISH_POINT` entries, rather than a flat sixty-float buffer.
The rod tip and preceding mass are entries 4 and 3. Every former rod byte/float
offset is expressed through a point index and `pos`, `old_pos`, or `velo`.
The focused object remains exact after that replacement.

The lure, float, and hook objects require declaration order `LureObj`, `UkiObj`,
`HariObj`. Defining the hook first changes only the three initializer relocation
targets. Defining them in retail order preserves the native 80-byte static
initializer; all 0x3DE0 checked bytes and 1,042 resolved relocations pass.
The inactive alternate state declarations and unused point/bind helpers are
removed with their `NONMATCHING` block.

Receipts: `.private/dataA-r2/fishingobj-bss.log` (rejected constructor order),
`fishingobj-bss-order.log` (exact), `fishingobj-bss-build.log`, and
`fishingobj-bss-objects.log`.
