# Scene character construction — October 8 midday

The lane baseline is `c79e57c`, compiled with MWCC 3.0-011126 and the
`chronicletwo_dev:sf-d8bf13c` adapter. Measurements below compare the native
`NONMATCHING` draft against retail, masking relocation operands while also
checking relocation identities. Complete-object validation uses the normal
matching build, where both character functions still select their assembly
gaps.

## Direct construction

`LoadChara` creates a character in the model stack, loads its model and motion
packs, and assigns it to the requested scene slot. `CopyChara` creates a
character in the supplied stack, copies an existing scene character, and
assigns its copied name, model, and scale to the destination slot. Both use
the already documented `CCharacter2` constructor chain in `character.hpp`
and `object.hpp`.

The source-local `NewSceneCharacter` wrapper consumes an additional inline
level. In the baseline native drafts, MWCC leaves the `CObject` constructor
out of line; retail inlines the complete chain. Removing the wrapper and
putting the placement-new expression directly in each caller reproduces the
four virtual-table stores, virtual initialization calls, and three
`shadow_link` initialization stores without modifying any shared header.

| Function | Baseline words different | Direct construction | Native body / retail extent |
|---|---:|---:|---:|
| `LoadChara__6CSceneFiPUiPcP9mgCMemoryP9mgCMemoryP9mgCMemoryii` | 120/148 | 2/148 | 0x244 / 0x250 |
| `CopyChara__6CSceneFiiP9mgCMemory` | 138/164 | 2/164 | 0x284 / 0x290 |

The remaining pair is the familiar allocation-result branch and register
copy. Retail tests `v0` and copies it to `s2` in the branch delay slot;
MWCC copies it first and tests `s2`. This occurs at `LoadChara +0x60/+0x64`
and `CopyChara +0x44/+0x48`. The functions remain guarded.

The other eleven native functions in this unit retain their previous match
results. The normal complete build preserves all 149 object-file SHA-256
hashes, retains 147/149 complete-object matches, and retains the baseline
PAL `.text` difference of 0x26 bytes. No constructor, profile, or shared-header
change is required.

Private receipts: `.private/placenew-midday/probes/scene-direct-new/`,
`draft-cleanup-build.log`, `draft-cleanup-objects.log`, and
`draft-cleanup-hash-comparison.json` under `.private/placenew-midday/`.
