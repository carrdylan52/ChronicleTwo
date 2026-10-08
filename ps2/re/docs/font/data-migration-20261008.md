# Font data migration

## Baseline and validation

The lane starts at `63 f 7 a 9 e 5`: all 45 font functions match, with 97
`INCLUDE_RODATA` markers and 4 `INCLUDE_BSS` markers. The refreshed objdiff
report records 0/7344 matched data bytes. Each accepted step passes the full
149-object comparison with resolved relocations, produces `SCES_511.90: OK`,
and leaves every unowned game object unchanged by SHA-256.

## Font-file storage and rectangle templates

`FontTblBinBuff` is a 4096-byte local font-file buffer. Its existing
`FONT_TBL_BIN` type describes four 16-bit header words and 2044 two-byte
character codes. `LoadFontTblBin` passes its address directly to `LoadFile`;
count accessors read the named fields, and `GetYoyakuTblTop` returns the first
code row. Narrow `opt_propagation off` scopes on the accessors retain the
retail base-address relocation and separate field offset rather than folding
the offset into the relocation. The complete font object remains exact.

`GetRectFontTex` initializes and returns a real `RECT` aggregate, then sets
its position and extent. `GetRectFontTexMini` returns a zero `RECT`. Neither
needs an anonymous structure, a reinterpretation cast, or explicit alignment.
`CFont::DrawChar` already has a third zero `RECT` aggregate. The three 16-byte
BSS templates are compiler-generated initialization data, not persistent
rectangle globals.

The four BSS markers remain for shared-tool constraints. The existing
postprocessor does not assign retail identities to anonymous NOBITS aggregate
templates. The named buffer marker additionally preserves an exported symbol
required by generated `vutext.data.s`: eight raw VU words are expressed as
`FontTblBinBuff + 0x 2 FD` or `+ 0xAFE` despite having no retail relocation.
Removing the marker gives an exact font object but fails the full link because
the natural static definition correctly has local binding. Keeping the native
typed declaration with its marker lets the existing binder select the retained
reservation and preserves the link. Shared-tool proposals and focused checks
are stored privately; no tool or generated-file changes belong to this lane.
