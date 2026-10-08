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
