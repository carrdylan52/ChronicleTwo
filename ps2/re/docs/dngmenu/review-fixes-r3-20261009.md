# October 9 round-three review fixes

## M4: room-information function statics

`DrawDngRoomInfo` owns the five-halfword `medal_xytbl` table. Its seal block
owns `AlphaRate`, initialized to zero on its first execution; MWCC generates
the initialization guard. Both use real function-local static declarations,
with no hand-written guard or file-scope aliases. This is the P1/P12 source
form verified by the round-two review.

The full pinned-image build verifies `SCES_511.90: OK` and the canonical
checker passes 149/149 objects. Function selection and profile rows are
unchanged. Receipts: `.private/fixes-r3b/receipts/dng-statics-{build,objects}.log`.
