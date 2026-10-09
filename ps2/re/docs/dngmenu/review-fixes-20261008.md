# October 8 night review fixes

## Retail function sizes

The header's `@size` values describe each function's declared retail symbol
size in `ps2/config/pal/main.symbols.txt`, excluding padding before the next
function. 27 padded tags are corrected. Every tagged symbol in this
header is present in that table, and the complete size audit has no mismatch.

The documentation changes preserve `SCES_511.90: OK` and 149/149 objects.
Receipts: `.private/fixes-r1b/receipts/header-sizes-{build,objects}.log`.

## Declarations and drawing parameters

`DrawRoot` names its mark mask and opacity consistently in the header and
source. `DrawRoomOne` likewise names the unused flag argument, opacity,
and brightness consistently. Their parameter types and retail ABI are
unchanged.

`MsgInit` uses the native `TreeMapSaveDispY` definition and the header's
`TreeMapSaveFlag` declaration. The duplicate `MenuTreeMapStack` declaration
before `DngTreeMapKey` is removed. One forward declaration remains in the
source file responsible for its definition: keeping the definition after
the four rectangle globals preserves their retail static-initialization
order. Data migration already removed the repeated `MenuDngMap`,
`CMenuTreePt` and `MenuCursorDataBuff` externs.

The complete build verifies `SCES_511.90: OK` and 149/149 objects.
Receipts: `.private/fixes-r1b/receipts/dungeon-declarations-{build,objects}.log`.
