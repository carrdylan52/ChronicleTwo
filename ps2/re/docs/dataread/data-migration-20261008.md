# dataread data migration (2026-10-08)

The fresh round-2 baseline has 19 rodata markers and 4 BSS markers, with
`matched_data` 552 / `total_data` 339554. All functions already match and
the existing notes document the file/cache/device structures and behavior.

## File paths and diagnostics

All 19 string markers and their extern declarations are removed. File and
device names, current-directory reset strings, and load/cache diagnostics
are literals at their existing uses. The two background-read diagnostics
already occur as native literals and need only their markers removed.
The exact disc filenames keep the leading backslash and `;1` suffix.
The empty string and slash are distinct retail objects, emitted naturally
by the two default-device transitions.

Acceptance: `.private/dataC-r2/dataread-literals-{build,objects}.log` and
`dataread-literals-metrics.json`. The full image and all 149 objects pass,
and every unowned object hash is unchanged. No rodata markers remain;
coverage is 850 / 339554, pending the four local BSS templates.

## Local path and prefix templates

The three 256-byte paths and the 16-byte device prefix now use local
`char` arrays initialized with `""`. The path/prefix quadword unions and
all four extern template declarations are removed. Initializers stay at
the point the original template copy executes: after the background-read
argument checks, after the cache-hit return, after device selection, and
at the start of `WriteFile`, respectively.

MWCC assigns stack arrays in declaration order. Moving the initialized
background path below the existing scratch path swapped their stack slots
(seven differing bytes); declaring the scratch path after the initializer
restores the retail offsets. The same applies to `LoadFile2`'s `sce_stat`
local (14 differing bytes in the first trial). The accepted declarations
keep the full path before these independent scratch buffers.

Acceptance: `.private/dataC-r2/dataread-load-path-ordered-{build,objects}.log`
and `dataread-load-path-ordered-metrics.json`. The complete image and all
149 objects pass and every unowned object hash is unchanged. Three BSS
markers are removed; the unit has 0 rodata / 1 BSS marker, and native data
coverage remains 850 / 339554 while the final BSS piece is retained.

### Terminal BSS extent blocker

`at_845` has a declared retail extent of 0x100 at 0x003EC290. Its split
piece reaches the next unit's BSS start at 0x003EC3C0, adding 0x30 bytes.
The natural `WriteFile` initializer emits exactly 0x100 bytes and preserves
every code byte and resolved reference. Removing the final marker passes
the PAL verifier but fails the object checker with two extent-only
problems: size 0x100 versus piece 0x130, and BSS run end 0x003EC390 versus
0x003EC3C0. The checker currently accepts terminal data padding only when
the gap is fewer than 16 bytes. Receipt:
`.private/dataC-r2/dataread-write-path-{build,objects}.log`.

`INCLUDE_BSS(at_845, 0x130)` therefore remains to preserve the existing
canonical acceptance contract. The source uses the natural array even
with that fallback piece. Enlarging the local path to imitate section
padding would change the retail stack frame and is not a justified source
form. A generic linker-tail proposal belongs to the tooling lane.
