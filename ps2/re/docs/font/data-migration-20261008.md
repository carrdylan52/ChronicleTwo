# Font data migration

## Baseline and validation

The lane starts at `63f7a9e5`: all 45 font functions match, with 97
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
`FontTblBinBuff + 0x2FD` or `+ 0xAFE` despite having no retail relocation.
Removing the marker gives an exact font object but fails the full link because
the natural static definition correctly has local binding. Keeping the native
typed declaration with its marker lets the existing binder select the retained
reservation and preserves the link. Shared-tool proposals and focused checks
are stored privately; no tool or generated-file changes belong to this lane.

## Glyph, tag, and code tables

The existing header types and public declarations describe the native tables
without changing any consumers:

- `GaijiDataTbl`: 51 `GAIJI_DATA` rows of 14 bytes (714 declared bytes).
  Each row contains a 16-bit code and six signed 16-bit texture/offset values.
  Codes run from `0xFD00` to `0xFD31`; the final row has code `0xFFFF`.
  Ten trailing bytes of its assembly piece are alignment, not extra rows.
- `FconvCodeTbl`: 46 `FCONV_CODE` rows of 12 bytes (552 declared bytes),
  comprising 44 named tags and two zero rows. The eight-byte tail of the
  assembly piece is alignment. Tags such as `[select]`, `[start]`, and `[L1]`
  are string literals in their table rows.
- `FontGaijiConvTbl`: 24 `FCONV_CODE` rows (288 bytes), for codes `0xFDE0`
  through `0xFDF7`. Its two-byte tag strings preserve the retail Shift-JIS
  bytes with fixed-width octal escapes.
- `alphabetical_chara_tbl`: 63 five-byte rows (315 bytes). Each row holds
  a four-character tag payload plus its NUL; five trailing piece bytes are
  alignment, not a 64th row.

The half-width and wide kana lookup functions initialize their existing local
wrapper types directly: 63 single-byte codes, 63 16-bit codes, and 24 16-bit
font-gaiji codes respectively. Their declared extents are 63, 126, and 48
bytes. They remain local copies rather than new global arrays.

The three outline offset templates are emitted by the existing four-, eight-,
and twelve-point local `int[][2]` initializers in `set2DSprite_Fuchi`. Their
redundant markers can be removed without changing code or relocations.

After these table steps, font has 19 RODATA markers and four BSS markers.
The refreshed source-only objdiff metric remains 0/7344; that build does not
run the final object's literal naming, piece ordering, and padding fixups.
The final linked-object comparison verifies every migrated byte and relocation.

## Inline literals

`MySetTexMini` uses the two retail small-font texture names directly.
`LoadFontTblBin` inlines its Japanese/other-language paths and oversize-file
diagnostic, and `CFont::SetStr` inlines its capacity diagnostic. The decimal
character comparisons in `GetHalfFontNo` use the exact two-byte code literals
also present in `FontGaijiConvTbl`; ordinary compiler pooling preserves their
shared identities. `GetAlphabeticalFontNo_cp` inlines the bracket prefix and
five normalization pairs, and indexes `&text[5]` for the payload. `DrawGaiji`
passes `"gaiji"` directly without a cast. Every function's literal replacement
was checked separately with the full build and object comparison.

These steps remove all font string extern declarations. The nine-entry branch table `at_1448__3` comes from the existing outline-style
switch. Removing its marker also passes without adding a definition or changing
the switch. No RODATA markers remain; four BSS markers remain as described
above. The refreshed data metric is still 0/7344.

## Primitive builder offsets

`CFont::DrawDirect` uses `mgCDrawPrim::offset_x` and `offset_y` directly at
`0x110` and `0x114`. Its local union retains only the genuine primitive-builder
member, which is initialized by `MySetPrim` for this draw. This preserves the
retail initialization without an automatic constructor call. The padding/size
overlay is unnecessary; removing it changes no code or relocations.

Final font markers are **0 RODATA / 4 BSS**, down from **97 / 4**. All 45
functions remain matched, the full 149-object check passes, unowned objects
remain unchanged, and the PAL verifier reports OK.
