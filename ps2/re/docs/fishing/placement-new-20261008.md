# Fishing character construction — October 8 midday

The native-draft measurements below use baseline `c79e57c`, MWCC
3.0-011126, canonical flags and existing pragmas, and
`chronicletwo_dev:sf-d8bf13c`. Relocation operands are masked in word counts;
relocation identities are checked separately. The three functions remain
guarded, and no source or profile change is retained.

| Function | Native words different | Body / retail extent |
|---|---:|---:|
| `sgRestartFishing__FP11SubGameInfo` | 2/344 | 0x55C / 0x560 |
| `StepDataLoading__FPv` | 651/738 | 0xB88 / 0xB70 |
| `InitSuccess__FP6CScene` | 25/280 | 0x458 / 0x460 |

`sgRestartFishing` resets the selected fishing equipment and bait. Its
remaining pair is the allocation-result null branch and saved-pointer copy.

`StepDataLoading` loads fishing resources and constructs the rod, float,
lure, hook, and two cursor characters before attaching them to the player.
Its seven constructions reuse a local `CCharacter2*`. Changing those sites
to direct global assignments, or giving each constructed character its own
named local, retains the 0xB88 body and 651/738 diagnostic word count. The
body exceeds the retail extent by 0x18 bytes; the canonical comparison
therefore reports an oversize function rather than an ordinary in-extent
word miss. At the first construction, `+0x138`, the copied allocation
register is tested instead of retail `v0`, and argument preparation adds
instructions that shift the following constructor chain. Its larger score
does not indicate a different initialization algorithm.

`InitSuccess` prepares the caught-fish character, fishing rewards, and the
success message. Retail keeps the player character in `s2` and constructs
the fish in `s1`; the draft exchanges those saved registers, in addition to
moving the allocation-result null branch. Moving the fish local into the
load block, reordering its declaration, assigning the new object directly
to `FishChara`, or using a validated player reference each retains 25/280.
Initializing the texture-manager pointer before the early returns gives
53/280; declaring it at its existing assignment gives 28/280; using the
global texture manager directly gives 247/280. None fixes the lifetime
permutation or the allocation branch.

The character constructor chain and its `shadow_link` member are already
documented in the owning headers and the placement-new report. These caller
probes add no original array or conditional-construction evidence to that
chain. No artificial constructor control flow is introduced.

Receipts: `.private/placenew-midday/baseline-native/fishing/` and the `fish-*`
directories under `.private/placenew-midday/probes/`.
