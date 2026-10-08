# Villager frame attributes — October 8 midday

On baseline `c79e57c`, the canonical native `CharaObjectOnOff` draft differs
in 6/112 words, with a 0x1C0 body matching retail's extent. It finds the
villager's named show and hide frames, allocates missing `mgCFrameAttr`
objects, and sets their drawing state. Existing type and lookup-function
documentation remains applicable.

The show-loop allocation already matches every instruction and relocation
identity. The hide loop uses the same attribute constructor but moves the
allocation result to `a0` before testing it; retail tests `v0` and places
that move in the branch delay slot. The extra draft nop shifts the call and
result copy within `+0x158..+0x16C`, then both versions rejoin at `+0x170`.
This localized difference is not evidence that the attribute's layout or
constructor body is wrong.

| Natural caller-lifetime probe | Words different |
|---|---:|
| Retained separate loop counters and block-local attribute pointers | 6/112 |
| Reuse `i` for the hide loop | 14/112 |
| Declare one attribute pointer for both loops | 6/112 |
| Give the hide count a separate local | 12/112 |

None improves the allocation site, and no source change or profile row is
retained. The matching build continues to use the assembly gap. Measurements
use MWCC 3.0-011126, the existing flags/pragmas and
`chronicletwo_dev:sf-d8bf13c`; word comparisons mask relocated operands and
check their identities separately.

Receipts: `.private/placenew-midday/baseline-native/scenevillager/` and the
`villager-*` directories under `.private/placenew-midday/probes/`.
