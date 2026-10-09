# menusys review corrections — October 8, 2026

Lane base `4831afdc`, MWCC 3.0-011126 with the production profile in
`chronicletwo_dev:sf-63f7a9e`. The warm build passes `SCES_511.90: OK` and
149/149 complete objects. Receipts are under `.private/fixes-r1a/`.

## Retail function sizes (finding 2)

All 142 existing `@size` tags in `menusys.hpp` resolve by `@mangled` to
`ps2/config/pal/main.symbols.txt`. Of those, 113 described the aligned section
extent rather than the symbol's declared size; all now use the declared size.
The other 29 tags already agree. This includes declarations for inline
functions emitted in another unit and for the four guarded menu functions;
no function body or guard changes.

The promoted functions singled out by the review have declared sizes
`CMenuItemInfo::CalcTex` **0xCF8**, `CheckEnableHaveItemNum` **0x34C**, and
`CommonSetMoveItemClass` **0x3D8**. Their padded section extents remain 0xD00,
0x350, and 0x3E0 respectively. Symbol sizes exclude the zero alignment tail.

The before/after inventories are `size-audit-before.json` and
`size-audit-after.json`; full acceptance is recorded in `sizes-build.log` and
`sizes-objects.log` (PAL OK, 149/149 objects).
