# movieviewlp data migration (2026-10-08)

Baseline: `95f8fdd1`; 14 initialized-data
markers and 20 BSS markers. Every function already matches.
The pinned `chronicletwo_dev:sf-63f7a9e` image and full object/PAL checks
validate each accepted step. Public declarations remain compatible.

## Viewer state and tag table

The twelve viewer-state reservations are native file-local scalar, pointer
and short-array definitions with the existing interfaces. `MovieLine`,
`MovieSelect` and `MovieSpecialMode` own two bytes each; their two-byte piece
tails remain alignment. `MovieSpecialModeInfo` owns three shorts, followed
by a two-byte alignment tail. The four buffer-manager objects and their
guards retain their eight markers.

`DataBuffer__2` and `Stack_ReadBuff__2` remain native `mgCMemory` objects,
now with file-local binding and documented purposes. Their constructor order
and the generated initializer are unchanged. `_MOVIE` has retail's file-local
binding, and `tag_movie` is a native two-entry `SPI_TAG_PARAM` array containing
the inline `"MOVIE"` tag and handler, followed by the null terminator.

Markers fall from **14 RODATA / 20 BSS** to **12 / 8**. Interim refreshed
matched data is **20/539**, from **4/539**. All five functions remain matched;
none is promoted. Each accepted group passes PAL and all 149 objects with
unchanged unowned object hashes. Receipts:
`.private/dataD/movieviewlp-{state,storage,tags}-{build,objects,metrics}.log`.

## Inline configuration and playback strings

All twelve remaining strings are inline at their existing uses: configuration
path, movie-work texture name, promotional list names and first-part paths,
list heading and row formats, and later promotional-part formats. The list
heading's Shift-JIS bytes use hexadecimal escapes. No argument order, stack
local or runtime control flow changes.

Initialized-data markers fall from **12 to 0**. Interim matched data is
**186/539**; the eight buffer-manager BSS markers remain. Receipts:
`.private/dataD/movieviewlp-{config,work,promo,list,parts}-{build,objects,metrics}.log`.
Every group passes PAL and all 149 objects with unchanged unowned object hashes.

## Buffer initialization flags

The four existing one-time initialization flags are native file-local signed
bytes. Their three-byte piece tails remain alignment padding. The four explicit
manager initialization paths are unchanged while their storage still comes from
markers. Markers fall from **0 RODATA / 8 BSS** to **0 / 4**; refreshed matched
data rises from **186/539** to **251/539**. Receipts:
`.private/dataD/movieviewlp-guards-final-{build,objects,metrics}.log`.

## Remaining buffer-manager storage

Native function-local `static mgCMemory` declarations reproduce all retail
instructions and constructor guards. The current data binder cannot name those
four manager objects or their compiler guards without storage markers. A raw
private copy using `buf0`, `buf1`, `dbuf0` and `dbuf1` has zero masked instruction
differences, but the unchanged postprocessor leaves the eight local identities
unmapped and the complete object fails. Four canonical manager markers remain.

The same generic proposal tested for scene,
`.private/proposals/dataD-local-static-bss.patch`, establishes each identity
from exact extent and consistent opcode-matched retail references. On that same
raw copy it names and pads the four individual guard slots, preserves the two
earlier native global buffers, and passes the complete unit: 0xD00 initialized
bytes and 361 resolved relocations. No shared tool or profile file is edited.
Receipts: `.private/dataD/movieviewlp-native-bss-{raw-build,baseline-check,proposed-check}.log`.

Final canonical markers: **0 RODATA / 4 BSS**, from **14 / 20**. Final
`matched_data` is **251/539**, from **4/539**. The unmatched 288-byte BSS
section contains the four retained 48-byte managers and the two native 48-byte
global buffers; objdiff's exact data credit applies to the complete section.
