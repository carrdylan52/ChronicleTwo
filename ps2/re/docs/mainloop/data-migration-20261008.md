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

## Controller bindings

The mutable 46-entry button table uses `PAD_TABLE_ENTRY`, existing logical
button names where documented, `PadCtrlTrigger` values and `PadButton` masks.
The nine-entry analog table uses `ANALOG_TABLE_ENTRY` and `PadCtrlAxis` values.
Each retains its negative terminator. The two language-dependent button pairs
are emitted by the natural local initializers in `InitPadTable`. `SelectArg`
is a zero-initialized native `int[32]`, placed in retail's initialized-data
section by the existing named-object section mapping.

Each table and the caller's literals pass the complete main-loop object
check; the full build reports `SCES_511.90: OK`, and all 149 objects pass.
Receipt: `.private/dataB-r2/mainloop-button-literals-{build,check}.log`.

## Mode callback tables

`LoopInit`, `LoopMain` and `LoopExit` are native ten-entry arrays using the
existing `LOOP_INIT_FUNC`, `LOOP_MAIN_FUNC` and `LOOP_EXIT_FUNC` types and
`LOOP_MODE_NUM` bounds. Entries follow `MainLoopMode` order and reference
functions through their owning headers. The local menu callbacks retain local
linkage. Each 0x28-byte table receives the retail eight-byte alignment tail.
All three steps preserve the complete main-loop object and PAL executable;
all 149 objects pass after the final table. Receipts:
`.private/dataB-r2/mainloop-{init,main,exit}-table-{build,check}.log`.

## Configuration and debug-menu tables

The 23-entry `tag__3` script table uses `SPI_TAG_PARAM` with inline tag names.
The menu labels are native file-private tables under their retail storage
names: 15 pointers in `menu_1281` and 12 in `menu_1457`. The first ends at its
empty string, removing the extra source NULL that emitted a 64-byte copy of
the 60-byte retail table. `menu_sel_1452` is a native zero-initialized
`int[11]`. These tables only serve their respective debug-menu functions.
Their payloads and resolved references match retail after each migration.

Language names, item-set names/numbers, extra-game names and the event cursor
are produced by their natural local aggregate initializers. Menu format
strings, boot file names, font file names, config messages and option tag
names are inline literals. The complete PAL build and all 149 objects pass.
Receipt: `.private/dataB-r2/mainloop-option-literals-{build,check}.log`.

### Remaining debug cursor markers

`at_1317__2` holds the two cursor pointers to `at_1315` (" ") and `at_1316`
(">"). Their natural initializer is already in `MenuLoop`. Removing their
markers leaves one unidentified ROData piece and one unidentified SData piece:
the short space string has ambiguous byte matches, and its unresolved identity
prevents the native aggregate from being recognized. The PAL image fails.
The three markers are retained; restoring only this group resolves the full
object check. Receipts:
`.private/dataB-r2/mainloop-menu-literals-{build,check}.log` and
`.private/dataB-r2/mainloop-menu-literals-restored-{build,check}.log`.
