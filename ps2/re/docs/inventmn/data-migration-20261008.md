# October 8 data migration

Baseline: `7c7edc2f`, MWCC 3.0/Satan's Fiddle through
`chronicletwo_dev:sf-63f7a9e`. The warm build verifies PAL OK and
149/149 canonical objects. Initial markers: 261 RODATA / 51 BSS.
Refreshed matched data: 4 / 18656 bytes.

## Named storage

Thirty-four named objects now use their existing native types and LOCAL
linkage: menu/session pointers, photo and recipe parsing state, notebook
arrays, drawing and command state, photo-effect state, album slot state,
debug state, and work buffers. The recipe manager is eight bytes and has
no constructor; its definition adds no initialization code.

`CMenuInventPt` and `InventSubDataReadBGInfo` are four-byte objects despite
eight-byte reservations. Byte and halfword flags keep their declared
widths. `temp_1728` is a 33-byte string; the fifteen bytes before the next
symbol are alignment, not characters. Notebook arrays contain 512 idea
names and 512 signed-halfword identifiers; the photo-name work buffer is
0x2480 bytes. Guarded blocks and profile rows remain unchanged.

Markers: 261 / 17. Matched data remains 4 / 18656 because the aggregate
sections still contain reservations. `invent-state-build.log` and
`invent-state-objects.log` under `.private/nminv-r2/` pass the full PAL
verifier and 149/149 objects.

## Naturally emitted templates and tables

The existing `found[3]`, CalcTex background and clipping arrays, card-origin
pair, and picture-position pair generate their own zero templates. Six
matched switch tables and the CMenuInvent vtable also come from existing
C++. Nine separately validated steps remove their markers without changing
any function body. `at_1965` is three bytes, while the four coordinate and
clip templates are eight bytes each. Pointer casts and dummy storage are
not needed.

Markers: 254 / 12; matched data: 36 / 18656 bytes. Each
`invent-emitted-<step>-build.log` / `-objects.log` in
`.private/nminv-r2/` reports PAL OK and 149/149 objects.

## String literals

Twenty function groups now use 116 native literals, including the photo
parser paths, form/texture identifiers, Shift-JIS prompts, debug formats,
and item-menu messages. Shared strings were replaced at all unguarded
uses together. CalcTex's profile selectors and generated code remain
unchanged. Literals used only by frozen drafts remain addressable under
their retail symbols.

Markers: 138 / 12; matched data: 36 / 18656 bytes. All twenty
`invent-strings-<step>-build.log` / `-objects.log` receipts in
`.private/nminv-r2/` verify PAL OK and 149/149 objects.
