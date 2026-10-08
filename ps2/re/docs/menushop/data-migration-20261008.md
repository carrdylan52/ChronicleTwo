# menushop data migration (2026-10-08)

Baseline `e1471bff`: **102 INCLUDE_RODATA / 35 INCLUDE_BSS**, with
**4 / 1885 matched_data** after the normal objdiff/progress refresh.
The unit has 29 native functions and one guarded initializer; none is promoted.

## Named shop and quest state

Twenty-eight reservations are native, documented file-local definitions with
retail names. Their types follow the existing header and matched consumers:
shop/quest objects and message pointers, signed short shop identifiers/counts,
float cursor/scrolling positions, signed-byte memo mode and byte comment flag.
`ScmFlagCtrl` points to the selected `SCOOP_DATA`, rather than the signed-short
pointer asserted by the older notes. `QuestListTopY` and `QuestCommentWinX` are
floats, as their arithmetic and loads establish. `QuestTilePatternXY` owns one
float, used for both screen axes, and is expressed as a scalar. Its four-byte
piece tail is alignment, not a second coordinate. `QuestCommentMes` contains
three pointers, with four bytes of alignment after its twelve-byte extent.

`MenuLocalStack` retains its native construction and now has the documented
file-local binding. The guarded `MenuNPCQuestViewInit` and both vtable markers
are unchanged. Its named state remains reachable by the exact retail symbols.

After this group: **102 / 7 markers**, **64 / 1885 matched_data**.
Full PAL verification is OK and all 149 objects pass; only menushop's object
file hash changes from the warm baseline. Receipts:
`.private/dataD-r1/menushop-state-{build,objects,progress,metrics}.log`.

## Form, resource and message strings

Sixty-one anonymous strings are inline in their matched callers: shop form and
part names, resource pack members, purchase/sale commands and quantity labels,
texture names, startup form exclusions, and quest/scoop configuration paths.
Shift-JIS strings use hexadecimal escapes for each original byte. Repeated
strings remain compiler pooled. Every function group passes the full build and
149-object comparison independently, with unchanged unowned object hashes.
Receipts are `menushop-{forms,scroll,resources,key,texture,shop-init,quest-init,quest-cursor}`
under `.private/dataD-r1/`; the refreshed checkpoint is `menushop-strings`.
