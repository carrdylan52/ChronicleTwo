# Main-loop data migration

## Baseline

At `55cdb46c`: 126 `INCLUDE_RODATA`, 51 `INCLUDE_BSS` markers;
4 / 27,902,952 matched data bytes in objdiff.

## Main-loop state

The scalar state, font-page pointers, resource arenas, controller objects and
thread priority use native definitions of their documented types.
`GamePad__2` has the declared `CGamePad` size of 0x478 and receives the retail
eight-byte alignment tail. `PadCtrl` remains 0x510. The main arena is
`u_long128[0x1A0000]`; system sound and info arenas contain 400 and 5,000
quadwords; font image storage is `u8[0xD000]`.

The four resource arenas retain external linkage: generated SDK and VU
assembly objects have relocations to `main_buffer`, `SystemSeBuff`, `InfoBuff`
and `font_buff`. Local linkage passes the main-loop object check but fails
those link references. Other scalar state remains file-local. Public
variables retain the declarations and types in `mainloop.hpp`.

The PAL build is byte-identical and all 149 objects pass. Receipts:
`.private/dataB-r2/mainloop-controller-{build,check}.log` and
`.private/dataB-r2/mainloop-state-{build,check}.log`.
