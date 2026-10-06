# font: reverse-engineering notes

The short CFont setters write the named size, clearance, position, RGBA and outline fields.
`Init` clears only the 0x80-byte string, then assigns the default outline, opaque grey colour,
alpha, position, spacing and glyph dimensions individually. `Preset` selects one of three
colour/outline combinations; preset pairs 0/1 and 2/3 share settings.

Unit range `0x2D8AA0`-`0x2DAE90`, 45 functions. Owns `CFont` (only class in `class_units.tsv`
for this unit). No first-game equivalent (`/home/adubbz/development/chronicle` has `RECT` in
`rect.hpp` with the same layout, but no `CFont` or `RGBAQ_TYPE`).

39 functions are exact C++ matches. Six retain assembly fallbacks without a C++
draft: `GetGaijiFontNo`, `GetFontGaijiFontNo`, `GetFontGaijiHankaku`, `GetFontNo`,
`set2DSprite_Fuchi`, and the integer-font-number `CFont::DrawChar` overload.

## CFont (size 0xB8, no vtable)
Size: retail symbols `Font` (0x3FAF50, mainloop) and `MovieCCFont` (0x3F0510, nd_meswin) are both
`size:0xb8`. `ClsMes` (nd_meswin) derives from `CFont`; its ctor runs `CFont::Init` twice (inline
`CFont()` ctor then an explicit `Init()`), and its own fields start at 0xB8.

| Off | Field | Evidence |
|---|---|---|
| 0x00 | `char str[0x80]` | `SetStr`/`Init`: `memset(this,0,0x80)`, `strlen >= 0x80` -> printf `at_936`, else `strcpy(this, s)` |
| 0x80 | `s32 fuchi` | `SetFuchi`; `DrawChar` passes it to `set2DSprite_Fuchi`; `Init` sets 3 |
| 0x84 | `unk_84` | never accessed in font or nd_meswin |
| 0x88 | `RGBAQ_TYPE color` | `SetColor(iiii)` byte stores 0x88-0x8B; `DrawChar(prim,char*,..)` passes the 8-byte value with `ld 0x88` |
| 0x90 | `s32 alpha` | `Init` = 0x80; `DrawChar(char*)` passes its low byte as the `unsigned char alpha` arg; `DrawChar` scales `color.a` by `alpha >> 7` |
| 0x94/0x98 | `pos_x/pos_y` | `SetPos`; `DrawDirect` calls `SetPos(x,y)` |
| 0x9C/0xA0 | `clearance_w/h` | `SetClearance`; `Init` 15/24; `DrawGaiji` passes 0xA0 as line height |
| 0xA4/0xA8 | `draw_w/h` | `SetDrawSize`; `Init` 16/20; `DrawChar` uses them as the screen rect size, halving w for half-width |
| 0xAC | `s32 mini` | `DrawChar`: non-zero -> `GetRectFontTexMini` + `MySetTexMini` |
| 0xB0/0xB4 | `float unk_b0/unk_b4` | `Init` zeroes; `DrawDirect` does `fptosi` on both and shifts << 4 (used as a 12.4 offset); meaning not established |

Verified by a trial compile (reverted): `Init`, `DrawChar(mgCDrawPrim*,char*,int,int)`,
`SetColor(RGBAQ_TYPE)` and `GetRectFontTexMini` MATCH with this layout.

Inline ctor `CFont() { Init(); }` is declared because `__sinit_mainloop_cpp` and `ClsMes::ClsMes`
call `Init` directly where a ctor would be.

## RGBAQ_TYPE (0x8, aligned 8) / RECT (0x10)
Declared here (nd_meswin includes font.hpp for them; gameutil forward-declares RECT).
`RGBAQ_TYPE` = r,g,b,a bytes + float q. It is passed by value in a single GPR (`sd $5` in
`SetColor(RGBAQ_TYPE)`, `ld $5` in `SetColor(unsigned)`), which needs the `aligned(8)` attribute;
`SetColor(RGBAQ_TYPE)` matches with it. `RECT` is returned by hidden pointer from
`GetRectFontTex`/`GetRectFontTexMini` and passed by value (pointer to copy) to `set2DSprite_Fuchi`.
`SetColor(unsigned)` = `SetColor(RgbqToUint(c))` (RgbqToUint in nd_meswin) but the trivial form
DIFFs: retail copies the returned value through a second stack temp (0x28 -> 0x20) before `ld`.

## Functions
- All 45 are global (none in `local_symbols.tsv`). `FontTblBinBuff` (0x1F455C0, 0x1000) is local:
  a static byte buffer of `FONT_TBL_BIN_SIZE` in font.cpp, read as `FONT_TBL_BIN`.
- `GetRectFontTex(font_no, &tex_no)`: if font_no is a font gaiji code (0xFDE0..0xFDF7) and
  LanguageCode is French/German/Italian/Spanish (2..5), converts via
  `GetFontNoFromFontGaijiCode`. Negative or >= 0x980 returns a zero-initialized RECT.
  Pages: <0x260 page 0 (n unchanged); <0x4C0 page 1, n -= 0x260; <0x720 page 2,
  n -= 0x4C0; <0x980 page 3, n -= 0x720. u = (n % 32) * 16,
  v = (n / 32) * 20, w 16, h 20. `MySetTex` accepts only pages 0 and 1.
- `GetRectFontTexMini` returns a zero-initialized RECT (stub; tex_no unused).
- `MySetTex(int)`: tex 0/1 only -> `GetFontTexture(tex)` (another unit). `MySetTexMini`:
  0 -> "FontTex_s_0", else "FontTex_s_1" via `mgTexManager.GetTexture(name,-1)`.
- `DrawGaiji` sets texture "gaiji" (`at_1543`) and draws with colour 0x80808080, line h = clearance_h.
- `LoadFontTblBin`: LanguageCode == LANG_ENGLISH -> "meswin/fonttbl_1.bin", else
  "meswin/fonttbl_2.bin"; returns 0 and prints "FontTblBinBuff OVER" if size > 0x1000.
- `GetFontNo`: '\n' -> -2; font gaiji tag -> its code; otherwise binary search of the big-endian
  Shift-JIS pair in `yoyaku_tbl` (count `yoyaku_num`) -> index or -1.
- `GetHalfFontNo(c)`: accented char -> font no; else `GetFontNo` of {c, 0x20}.
- `CheckKanjiFont`: false for LANG_CHINESE (6); else kanji_top_no != 0 && kanji_top_no <= n < yoyaku_num.
- `CheckHalfFont`: 0xFF02 -> true; non-English: 0x5E..0x9C true, 0x9D..0xB4 false; else 0 <= n < half_font_num.
- `GetFontNoFromFontGaijiCode`: not in English; searches the 24-entry u16 table `at_1137__2`
  (copied to stack) for the code, returns 0x9D + index.
- `GetFontGaijiHankaku`: true for 0xFDF3..0xFDF7.
- `GetGaijiW/H`: `GaijiDataTbl[code - 0xFD00].w/h` (offsets 6/8) for 0xFD00 <= code < 0xFD32.
- `My_strncpy`: copies n characters, a '[' tag copied whole up to ']'; returns dst.

## Enums / constants (header anonymous enum, FontFuchi, FontPreset)
- `FontFuchi` 0..8 from `set2DSprite_Fuchi` switch; `Init` default 3 (outline).
- `FontPreset` from `Preset`: 0/1 -> 0x80202020 + fuchi 2; 2/3 -> 0x80686A6B + fuchi 8; 4 -> 0x80686A6B + fuchi 5.
- `FONT_NO_HALF_SPACE` (0xFF02) is a neutral-ish name: only `CheckHalfFont` shows it is always half-width.
- Language checks use `LanguageCode`/`LanguageCodeNo` from mainloop.hpp (not included here).

## Data
| Symbol | Addr | Size | Type |
|---|---|---|---|
| GaijiDataTbl | 0x35AC80 | 0x2CA | `GAIJI_DATA[51]` (0xE stride; w at +6, h at +8) |
| FconvCodeTbl | 0x35AF50 | 0x228 | `FCONV_CODE[46]` (0xC stride) |
| FontGaijiConvTbl | 0x35B180 | 0x120 | `FCONV_CODE[24]` |
| alphabetical_chara_tbl | 0x35B2A0 | 0x13B | `char[63][5]` |
| FontTblBinBuff (static) | 0x1F455C0 | 0x1000 | `FONT_TBL_BIN` |
| at_784__2 / at_817__4 / at_1466__6 | .bss | 0x10 each | function-local static RECTs (compiler-named) |
| at_1137__2 | 0x35B4A0 | 0x30 | u16[24] font gaiji codes (local aggregate initialiser) |

## Unresolved
- `unk_84`, `unk_b0`, `unk_b4` meaning (b0/b4 are a float offset used by `DrawDirect`).
- `FONT_TBL_BIN::unk_6` never read.
