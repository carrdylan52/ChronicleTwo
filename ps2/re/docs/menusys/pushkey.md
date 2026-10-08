# Item-menu input and weapon tuning

`CMenuItemInfo::PushKey(int pad, int trigger)` is the input dispatcher at PAL
`0x24A890`, with a `0x1BA0`-byte manifest extent. It returns zero when menu keys
are disabled and one after processing enabled input. `pad` carries held
navigation bits; `trigger` carries newly pressed action bits. The existing
item-menu and tuning enums identify the dispatcher states and stages.

The browsing state handles cursor movement, item commands, equipment changes,
and transitions into spectrumisation, fusion and build-up tuning. The modal
states dismiss an over-limit message or handle tuning adjustment and its save
or discard questions. The common tail restores the ten parameter-label colour
triples when the menu leaves tuning. The early modal exit shares the return
label with that tail; replacing its existing jump with a separate return changes
the code layout and increases the raw section past the retail extent.

## Sorting and model-reload locals

The bag-sort case constructs a normal `CGameDataUsed` automatic object before
preserving the viewed weapon with `CopyGameData`. After sorting the inventory,
it searches for the saved record with `memcmp(sizeof(CGameDataUsed))` and restores
the viewed slot. `CGameDataUsed` is the documented `0x6C`-byte tagged item record.
Retail directly calls its constructor with the address of this local; it does
not call the placement-allocation overload. A function-scope union followed by
placement construction introduces that extra call and changes the stack layout.

The model path and full path buffers belong to the equipped-model reload
branch. Their extents are `0x40` and `0x60` bytes. Keeping them local to that
branch puts the saved item at retail's stack offset `0xE0` and the two buffers at
`0x150` and `0x190`. A normal item object with function-scope path buffers instead
places the buffers first and the saved item at `0x180`.

## Tuning record layout and lifetimes

`CGameDataUsed::data.weapon` begins at item offset `0x10`. The existing
`WEAPON_USED` definition supplies the two relevant members:

| Member | Weapon offset | Type and role |
|---|---|---|
| `attribute` | `0x16` | Eight signed halfword attribute values; tuning visits the first five. |
| `fusion_point` | `0x2C` | Signed halfword points available to spend on attributes. |

The saved snapshot is `SpectolInfoStay.data.weapon`; the current record is the
viewed weapon's `data.weapon`. Retail keeps these weapon bases in `s1` and `s2`
through the adjustment sound calls and the five-element confirmation loop.
Keeping typed pointers to both records across those regions avoids reloading
the global snapshot in the confirmation loop. The earlier whole-item and
halfword-pointer draft added two loads there, shifting every later instruction
by eight bytes.

Adjustment selects one of the first five attributes. Left can undo increases
above the saved value, refunding 100 fusion points for one attribute point.
Right spends 100 fusion points if the attribute is below 100 and the point
budget allows it; the resulting budget is clamped to zero if negative. The
available-step count is computed before the left adjustment, as in retail.
Confirm scans those five attributes for an increase and opens the save question
only when a value exceeds its snapshot. These accesses use the existing typed
members rather than treating the entire weapon as an array of shorts.

Declaring the available-step count before the selected index gives the retail
register assignments. Directly reading and updating `weapon->attribute[selected]`
also gives the retail operand order at `+0x12C4`; a separate pointer to that
single attribute reverses the operands of its commutative `addu` and leaves one
word different. The direct form contains no pointer arithmetic or additional
code-generation helper.

## Matching evidence

The October 8 midday isolated production-wrapper compile reports an exact
function and a passing whole-unit object: `0x1B0DC` bytes, 5,872 relocations.
The raw C++ function section is `0x1B94` bytes. The remaining twelve bytes of
the retail manifest extent are zero padding, and the object's function bytes,
padding and relocations all pass `check_objects.py`.

Private receipts in the implementation worktree:

- `.private/menusys-midday/PushKey__13CMenuItemInfoFii-m2c.log`: the required
  `decompile.sh` attempt; m2c cannot recognize the existing `at_*` jump-table
  labels. The already documented dispatcher and retail disassembly provide
  the local-layout and tuning comparisons above.
- `.private/menusys-midday/push-direct-attribute/{scores,check}.log`: exact
  isolated function and whole-object comparison.
- `.private/menusys-midday/push-probes3.log` through `push-probes5.log`: typed
  base lifetimes, declaration order, and direct attribute access trials.
- `.private/menusys-midday/push-align.log`: branch-offset-normalized alignment
  showing that the remaining mismatch region was weapon tuning.

The function's `NONMATCHING` guard is removed. Full-build validation preserves
the baseline PAL result: only `.text` differs by `0x26` bytes, first at
`0x0015C5AD` in `nd_meswin::DrawMesWin`; all other file-backed sections pass and
the memory end stays `0x01F64A00`. The complete object check remains 147/149,
with `menusys` passing. SHA-256 comparison of all 149 game objects changes only
`menusys.cpp.o`; every other game object is byte-identical to the lane baseline.
Receipts are `.private/menusys-midday/push-build.log`, `push-objects.log`, and
`push-hash-compare.json`. The separate `push-progress.log` refreshes objdiff
because the expected PAL verifier failure stops the ordinary build before its
progress stage. No Satan's Fiddle override is needed for this function.
