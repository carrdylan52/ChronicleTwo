# Exact three-zero owner handoff

The three private native zeros now have one exact current34 activation
bundle. It contains five source/header changes, two placement rows and one
camera floating row. Its reconstructed bytes match the already accepted
private two-header composition. Shared headers remain unowned in this lane;
none of these changes is applied to canonical production34.

## Frozen activation vehicle

`.private/pntc/three-zero-activation-proposal/code-header-current34.patch`
changes only menuchr.cpp/menuchr.hpp, mg_visual.cpp/mg_visual.hpp and
visualmotion.cpp. The Costume family moves the authored constructor into
its owning unit, removes dead cursor/count initialization, uses its actual
typed lists and moves loading-phase/character lookup to the allocation guard.
The visual family removes the manual MDT assignment declaration/body and
uses ordinary generated derived assignment in both Copy functions.

| Exact caller | Qualified name | Retail address | Actual body |
| --- | --- | --- | --- |
| `MenuCostumeInit__FP9mgCMemoryPii` | `MenuCostumeInit(mgCMemory*, int*, int)` |`0x002C22B0`|`0x2D8`|
| `Copy__15mgCVisualFixMDTFP9mgCMemory` | `mgCVisualFixMDT::Copy(mgCMemory*)` |`0x00141260`|`0x190`|
| `Copy__18mgCVisualMotionMDTFP9mgCMemory` | `mgCVisualMotionMDT::Copy(mgCMemory*)` |`0x0028E930`|`0x210`|

`profile-current34.patch` carries the exact current34 profile delta.
Portable `policy-additions.json` contains the two after-inline rows, each
with allocator `__nw__FUiP1`, its ordinary derived constructor and one expected
match, plus MenuCostumeInit's binary32 zero/evaluate-first camera row with
callee `__ct__15mgCCameraFollowFffff` and one expected match. Costume's actual
constructor is class three and receives no placement row. `compose_policy.py`
merges into a supplied profile, preserves unrelated rows and rejects identity
conflicts; its stdout is a proposal rather than an applied profile.

The complete candidate has 36 placement identities /46 sites plus the
camera float row, representing 37 hypothetical native callers. Its fixture
SHA-256 is `21097e026a7bbd2e4ca31e5e314d6afbd3fb72f5d691ff0f1f898787bdf8ee42`.
Do not replace a newer canonical profile with this snapshot. The code/header
patch SHA-256 is
`b3ab4ea96b7c87655d7f54a61e4ee4ef5e01f867c2d4a732b72be2d319b8a57a`;
portable additions SHA-256 is
`34f0ae8d7dfe29566fe3b48131e7db13cf070965e690ce1f27b6be6d6b409afb`.
Both exact patches pass apply-check without being applied. All five
reconstructed source/header files equal the frozen composition inputs.

Root verifies all 43 members in manifest SHA-256
`c7036745a8989a4fca0507dc6da66a1e28f600dee6636f356a6931b175d5edd1`.
The [composition note](placement-new-three-zero-composition-20261009.md)
owns the actual one-unit two-overlay wrapper and composed 306-input MWLD/PAL
proof, including reused visual corpus scope, seven symbol metadata changes
and the selected GP relocation additions/order differences. This handoff
does not add a compiler invocation or claim a second whole-149 joint compile.

## Commit-policy erratum

The original frozen README line18 and DEPENDENCIES.json
`combined_commit_requirements` incorrectly demand a single source/profile
commit and prohibit a profile-only commit. Those directives conflict with
the binding user brief: common.md lines20–22 require small per-unit/topic
commits, and lines44–46 require SF rows in their own commit. The pntc brief's
sequence places rows before per-unit promotions.

The correct sequence is a separate reviewed SF-row commit, followed by
source commits by unit/topic. Semantic acceptance and validation still cover
the complete source/header/profile combination. The relevant header owner
coordinates coupled declaration/body removal within the appropriate unit
change. Commit separation does not permit an incomplete final activation.

`.private/pntc/three-zero-activation-erratum/ERRATUM.md` and
`commit-policy-override.json` explicitly supersede only those erroneous
history directives. The original 43-file freeze remains unchanged.
`frozen-README-unapplied.patch` proposes the minimal documentary replacement,
SHA-256 `c5c921ff06f6525227e31fe2583b663dc89b19ab87226b8208acb876c82af449`.
Root verifies all 22 erratum members under manifest
`082ddf6f89e7803ac55d48d815c5c48328bf62907517dee1d5e0dd4765970114`
and all 43 original members again. Its exact native-name evidence also
corrects the public shorthand Motion Copy to `mgCVisualMotionMDT::Copy`;
the frozen native selectors were already correct.

Root receipts are
`.private/pntc/receipts/three-zero-activation-{proposal,erratum}-root-verification.json`.
No prior frozen proposal, canonical source/header/profile or remote state
changes. Shared-header review remains a future owner action under the brief.
