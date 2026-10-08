# Guarded menu construction

These constructor drafts still use retail assembly in the ordinary build. The
following results come from isolated production-wrapper compiles with canonical
flags and the October 8 `sf-d8bf13c` image. Each compare masks relocation operands
and pads the function to its manifest extent.

## Action-character chain

`IsAskExtend` creates the weapon build-up preview character from its local
memory stack. `MenuModeMalloc` creates seven menu action characters and the
spectrumisation frame from `MenuItemMemory2`. Each requests `0x105` quadwords.
`CActionChara` itself is the existing documented `0x1030`-byte type.

The native construction chain is `mgCObject`, `CObject`, `CObjectFrame`,
`CCharacter2`, then `CActionChara`. It performs the base/derived initialization
calls, the character shadow-link clears, constructs `CRunScript` at `+0x6BC`,
and clears the `0x110`-byte movement-check region at `+0x910` through the existing
constructor. A local `inline_depth(8)` setting exposes the deeper part of this
actual chain. It is reset inside each draft guard, so the normal assembly build
never sees the setting. Direct `new (stack.Alloc(...)) CActionChara` expressions
replace the unnecessary local allocation helper.

| Caller | Earlier draft | Retained draft | Raw size / retail extent |
|---|---|---|---|
| `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory` | 208/240 words | 127/240 words | `0x3BC / 0x3C0` |
| `IsAskExtend__13CMenuItemInfoFii` | 625/668 words | 528/668 words | `0xA68 / 0xA70` |

The first action-character construction in `MenuModeMalloc` now has the retail
operations and instruction positions throughout its body. Its remaining
`+0x5C/+0x60` mismatch copies the allocated pointer before testing it instead of
placing that copy in the branch delay slot. The second construction adds a nop
and shifts following instructions. Effect allocation and the final float
argument setup also remain to be resolved. `IsAskExtend` additionally has
question-dispatch and register-assignment differences before construction.

The placement analysis in
[funcpoint/placement-new.md](../funcpoint/placement-new.md) establishes that
straight-line constructor bodies remain in frontend class 6; a real loop or
other retained statement structure can produce class 3. This chain has no
supported inline array loop to substitute for the actual initialization calls.
Changing unrelated shared headers or inventing a loop would not follow the
observed construction. No shared-header proposal is made.

Applying depth eight to `MenuItemDebugKey`, which already has a depth-five draft,
does not improve its isolated 1208/1460-word result. Its existing depth remains.

## Item-selector constructor boundary

`MenuItemSelectInit` allocates `CItemSelect` in `0x47` quadwords; the existing
class extent is `0x450` bytes. Retail constructs its two rectangle members with
their default `mgRect<float>` constructors, then clears the selector animation,
count, coordinate and texture fields. The member default constructors already
call `Set(0,0,0,0)`; repeating those calls in the explicit constructor duplicates
initialization.

The explicit body then sets the list rectangle to
`(120, mgScreenHeight-0x10A, 0, 200)` and the item rectangle to
`(list.left+20, list.top+370, 44, 55)`, clears top line and cursor, sets one row,
refreshes owned-item limit flags, and builds the selectable-item pointer list.
All these operations lie inside the retail allocation-success branch. They
therefore belong to the inline constructor, before `SetTexBlock` in the caller.
The constructor declaration remains in the header; the guarded inline definition
sits immediately before its only allocating caller in `menusys.cpp`.

The caller then attaches textures, captures the background, attaches common
texture information, aligns its stack, and starts background reading. Mode 9
loads the first menu file and mode `0x16` loads the second. Retail uses separate
mode tests and leaves the file-size local uninitialized for other modes. It
rounds the selected byte count to quadwords through the same unsigned remainder
rule as the existing `QuadwordsFor` helper. The guarded draft preserves those
control-flow and rounding details.

The earlier whole-draft section exceeded the retail `0x290` extent by `0x14`.
The retained isolated draft fits at `0x28C`, with 146/164 words different.
A smaller 107/164-word trial initialized the size to zero, used an `else if`,
and used signed `(size+15)>>4`; it does not reproduce the retail caller tail
and is not retained. The fitting retained version still differs in its first
stack-buffer expression order, pointer copy/null-test scheduling, and rectangle
float argument order. Neither member is a homogeneous constructor array; no
supported loop conversion has been found.

A private evaluate-first profile for the actual native `Set__9mgRect<f>Fffff`
callee reduced the retained score to 145/164. Sanitized retail template names
and the standalone constructor identity did not consume those selectors; the
actual enclosing identity was the caller. The profile is not retained: the
function remains guarded and no exact whole-object match follows. There are no
new committed Satan's Fiddle rows.

## Receipts

In `.private/menusys-midday/`:

- `mode-probes.log`, `ask-probes.log`, `key-probes.log`: depth and direct-new
  trials before retaining the improvements.
- `select-probes.log`, `select-native-profile.log`: constructor boundary,
  caller-control and native-template profile trials.
- `retained-ctor-scores.log`: all three final retained isolated scores.
- `retained-<mangled symbol>/{check,scores}.log` and `diff.txt`:
  production wrapper output and detailed comparisons.

The ordinary guarded build must retain the already validated `PushKey` object
and leave all other units byte-identical. The complete validation receipt is
recorded in [the midday status](matching-midday-20261008.md).
