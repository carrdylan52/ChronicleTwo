# Character data migration

## Baseline

At `55cdb46c`: 47 `INCLUDE_RODATA`, 32 `INCLUDE_BSS` markers;
0 / 1,788 matched data bytes in objdiff.

## Model parser storage and tables

The 30 file-scope BSS markers now have native definitions using the previously
documented parser types. `alloc_vertex` is `char[25][16]`, `img_ptr` is the
six-entry `mgIMG_FILE_HEADER *` array, and `skin_mds_name` is `char[64]`.
The six image pointers occupy 0x18 bytes; the retail BSS piece has eight more
bytes of zero alignment padding, supplied by the existing postprocessor.

The character and skin tag tables are native `SPI_TAG_PARAM[40]` and
`SPI_TAG_PARAM[5]` arrays. Their callbacks have file-local linkage, matching
retail. Names are inline strings. The skin table's 0x28 declared size receives
eight zero bytes before the next piece.

`_SHADOW_MODEL` initializes a two-entry `mgCreateVisualType` array to shadow
MDT for the empty object name followed by the end sentinel. The quadword union
and extern aggregate are removed. `CCharacter2` emits its own vtable.
Diagnostic formats, the outline texture format, the weight-file suffix and
empty names are inline literals. All 47 ROData markers are removed.

Both `_MODEL` walks access `alloc_vertex[index]`. This preserves the complete
object while removing the integer byte offsets and casts. A typed pointer
walk in the second loop differed in four checked words/relocation sites;
indexed access matches without additional variables.

## Outline counter

`_OUTLINE` uses a natural `static int outline_num = 1`, followed by its normal
increment. It generates the same guarded initialization that the explicit
`outline_num_1499` / `init_1500` scaffolding represented. The two BSS markers
remain because the postprocessor currently needs them to identify compiler-
generated local-static names; the corresponding editctrl failure and tooling
proposal cover this limitation.

Final validation: `SCES_511.90: OK`, all 149 objects pass; character retains
102 matched functions, 0x6BE4 checked bytes and 906 resolved relocations.
Receipts: `.private/dataB-r2/character-final-{build,check,progress}.log`.

After refreshing progress: 0 / 1788 matched data bytes; markers 0 ROData, 2 BSS.
