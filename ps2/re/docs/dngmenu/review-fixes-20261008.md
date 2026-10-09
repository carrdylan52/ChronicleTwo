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

## DrawRoomOne overlay conversions

Replacing the four overlay `fptosi` calls with `(int)` casts in the same
source order fails matching. The native body shrinks from the retail
`0x914` symbol size to `0x90C`; the positional diagnostic has 236/584
word differences in the `0x920` aligned reservation. Cast lowering moves
conversions into the primitive batch: `Begin` is called at native `+0x61C`,
before the conversions at `+0x64C` and `+0x658`. Retail computes all four
integer corners before beginning that batch.

The complete checker reports 27 dngmenu problems and 148/149 passing
objects; the PAL verifier fails. The explicit calls are retained, with
no source-order or behavior change. Probe receipts:
`.private/fixes-r1b/receipts/room-casts-probe-{build,objects}.log` and
`room-casts-score.log` in the same directory. Restoration is verified by
`room-calls-restored-{build,objects}.log`.
