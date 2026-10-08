# Native character-menu memory partitioning

October 8, 2026, midday menuchr lane. Base `c79e57c`, image
`chronicletwo_dev:sf-d8bf13c`, four build jobs.

## Behavior and dependency types

`MenuMemoryDivide__FP9mgCMemoryPP9mgCMemoryi` aligns the caller's memory
stack, partitions its quadword storage among character-menu work managers,
and returns the total quadword capacity assigned. `mgCMemory` is the
documented 0x30-byte manager: `stack` is at 0x20 and `stack_used` at 0x24.
`stGetTop()` addresses `stack[stack_used]`; `stSetBuffer()` accepts a
quadword pointer and an integer quadword count.

Max, Monica, and the ridepod use seven capacities. Every nonzero remainder
is rounded up to the next multiple of 64 quadwords. `SetMemoryName` copies
the formatted `stack %d\n` label only when its length fits the manager's
16-byte name. After attaching each buffer, the next buffer begins at that
manager's top plus its capacity. Monster mode attaches 0x6400 quadwords
to slot zero and 0x3C0 quadwords to slot five, returning 0x67C0. Unrecognized
character numbers return zero after alignment.

The character identifiers are the existing `USER_CHARA` values from
`userdata.hpp`. The two capacity tables are local constant `u16[7]` arrays:

| Table | Quadword capacities |
| --- | --- |
| `menu_robo_memorytbl` | 0x5240, 0x4514, 0x6E80, 0x1240, 0x05DC, 0, 0 |
| `menu_chr_memorytbl` | 0x1B80, 0x9AC0, 0x1C84, 0x11C0, 0x0BC0, 0x26C0, 0x0708 |

The first table occupies a 16-byte retail piece, the second a 24-byte piece;
their declared 14-byte arrays retain the zero alignment tails through normal
postprocessing. The tables and the inline format string replace their three
assembly data markers. No shared header changes are required.

## Matching evidence

m2c analysis uses `decompile.sh`, saved privately as
`.private/menuchr-midday/m2c-memory.c`. The baseline is 18 differing words
within a 0x1D0-byte extent. Its 0x1C4-byte body already has the correct
operations, but assigns the loop capacity to s1 and the buffer to s2.
Retail retains the buffer in s1, table/list induction in s2/s3, and capacity
in s4.

Reading the table when deciding and computing the rounded capacity, rather
than using the mutable capacity as the rounding input, restores those
lifetimes. The retained source initializes `size` from the table, tests the
table's remainder, and assigns `(64 - remainder) + table[i]` on the rounding
path. MWCC eliminates repeated table reads and emits the retail single
halfword load, signed remainder sequence, and three rounding instructions.
Private probes E52 and E53 both reach zero; E52 is retained.

An if/else formulation reaches three words but introduces a branch around
the merged result. A ternary keeps that branch. An unsigned-short capacity
adds truncations; a long capacity changes the instruction selection. Separate
raw-size/remainder locals, reference access to the input manager, and a
do/while loop do not resolve the original allocation. These probes are
E02, E07–E10, and E42–E53; none is retained.

The guard is removed manually. Canonical compilation with mwccgap, section
fixup, and complete-object verification accepts 0x11CD0 allocated bytes and
3,734 resolved relocations, including both native tables and the format
string. No Satan's Fiddle expression selector is needed.

## Acceptance receipts

Receipts remain under `.private/menuchr-midday/`:

- `baseline-build.log`, `baseline-objects.log`, and `baseline-hashes.json`.
- `memory-game/objects.log`: native function with assembly data.
- `memory-native-data/objects.log`: native function and native data.
- `memory-native-build.log`, `memory-native-objects.log`,
  `memory-native-coverage.txt`, and `memory-native-hashes.json`.

The integrated verifier retains the baseline's 0x26 differing `.text` bytes,
first at 0x0015C5AD in `nd_meswin/DrawMesWin__6ClsMesFv+0x6FD`. The other
nine file-backed sections and memory end 0x01F64A00 pass. Both known failing
objects remain `nd_meswin` and `actscript`; 147/149 complete units pass.
All other 148 object-file hashes are unchanged. Coverage rises from 6,686
to 6,687 matched functions; menuchr rises from 75 matched /13 guarded to
76 matched /12 guarded.
