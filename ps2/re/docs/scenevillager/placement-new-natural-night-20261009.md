# Villager attribute allocation: natural caller controls

The fresh pn15 `CharaObjectOnOff__6CSceneFiP9mgCMemory` control retains the
six-word hide-loop allocation residual described in the
[midday study](placement-new-20261008.md). Its actual GLOBAL/FUNC symbol and
retail extent are both 0x1c0, at 0x002ce4f0. The existing header annotation
is correct. No source, header, profile or guard change is activated.

The existing scene/villager, `vlgr_info`, `scenesnd`, memory and frame notes
establish the dependent types and lookup APIs. `CVillagerInfo` is 0x1c with
show/hide name-list pointers at 0xc/0x10; each lookup fills the real sixteen
element `mgCFrame *frames[]`. A frame's attribute pointer is at 0xf4.
`mgCFrameAttr` is 0x90 and its draw flags are at 0x18. The fresh mandatory
`decompile.sh` succeeds. Its inferred `fog` name for the 0x18 store is not
the established layout: the current typed header and frame analysis identify
that field as `draw`.

The function returns for a missing scene slot, character or villager record.
For each resolved nonnull frame, it creates an attribute when both the
attribute is missing and an allocation stack exists, stores the result back
on the frame, and writes drawing flags when the result is nonnull. The show
list writes `MG_FRAME_DRAW_VISIBLE`; the hide list writes
`MG_FRAME_DRAW_SKIP_CHILDREN`. A null placement result still replaces the
frame's missing attribute with null, preserving the original behavior.

The constructor is the genuine out-of-line
`__ct__12mgCFrameAttrFv`, called at retail 0x1361f0. It is outside the
class-6 inline-construction capability. No placement row or float argument
policy is applicable to this residual. The show path already matches.
In the hide path, retail tests the allocator's v0 at +0x158 and copies it
to a0 in the delay slot. Native code copies first, tests a0, then adds a nop.
The call and result copy shift until retail's join nop restores alignment
at +0x170. The six differing words cover +0x158 through +0x16c, with the
constructor relocation shifted from +0x160 to +0x164.

Ten fresh source controls retain the genuine logical translation unit,
pn15 compiler, production profile snapshot and all real function calls.
The old counter reuse, shared attribute variable, separate hide-count and
direct-owner assignment negatives are not repeated.

| Source control | Differing words | Native body |
| --- | ---: | ---: |
| Current canonical source | 6 | 0x1c0 |
| `sizeof(mgCFrameAttr) / 16 + 2` reservation and existing draw enums | 6 | 0x1c0 |
| Actual hide-frame array pointer walk | 48 | 0x1b8 |
| Actual pointer walks in both frame arrays | 109 | 0x1ac |
| Reference to the real hide-array entry | 6 | 0x1c0 |
| References to the real entries in both loops | 12 | 0x1c0 |
| Capture the hide frame itself | 19 | 0x1c0 |
| Capture the frame itself in both loops | 41 | 0x1c0 |
| Const allocation-stack pointer | 6 | 0x1c0 |
| Const villager-info pointer at its lookup | 17 | 0x1c0 |

The typed walks use the existing array and its real loop count, retaining
all null-frame paths. An entry reference retains the live frame-pointer
load after constructor calls; a captured frame pointer expresses the other
natural ownership lifetime. Neither adds a synthetic allocation observer,
helper, generated special member, dummy store or byte-address operation.
The allocation sizing retains all eleven blocks, including two reserves.

The hygiene and const-stack controls preserve the exact complete canonical
native object, SHA-256
`b7ed5902fd0fcda952c7287557920156042c9d79173118e4938f9dbda7afa466`.
All nine successful variants preserve the other 39 owned function bodies
and normalized relocation targets. Some source forms renumber one anonymous
reference; its literal bytes remain identical. The hygiene control also
preserves all 39 function symbol shapes, including bindings, sizes, values
and section alignment; no vtable is emitted in this unit. The object audit
is a native diagnostic check, not acceptance of the still-guarded function.

Private inputs, source/profile snapshots, ten compiler statuses, native
objects, scores, diffs and nonselected audits are under
`.private/pntc/scenevillager-onoff-natural/`. The fresh decompiler receipt is
`.private/pntc/receipts/scenevillager-onoff-m2c.log`. No accepted natural zero
is established and no production selector is proposed.
