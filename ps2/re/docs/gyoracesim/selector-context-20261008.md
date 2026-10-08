# Fish tactics control-context calibration

Base: `24d3d21`, image `chronicletwo_dev:sf-d8bf13c-proto`, MWCC
3.0-011126, canonical `-O3,p` flags and GPR/FPR history `0x30`/`0`.
The baseline PAL image passes and all 149 game objects pass. Coverage is
6,745 matched, 117 guarded drafts, ten assembly-only functions and zero fuzzy.

`FishModifyParam__FP12grFISH_PARAMPff` at `0x00323710` builds the six
race-statistic outputs from the entrant's attributes, species, name-seeded
random variation and selected tactics. The already documented dependent
types remain unchanged. A fresh `decompile.sh` receipt confirms the known
m2c tactics-jump-table limitation; retail disassembly and the existing
analysis establish the actual case labels and call arguments.

Two accepted binary32 rows select `GetRandomNumber__Fff` arguments:

| Bits / value | Additional identity | Policy | Retail effect |
| --- | --- | --- | --- |
| `0x3e4ccccd` / 0.2f | `control: {kind: switch, values: [1]}` | `evaluate_first: true` | Fixes the five case-1 range/mean words at `+0x4F4..+0x504` without changing tactics 2 or 4. |
| `0x3e99999a` / 0.3f | Callee only | `evaluate_first: true` | Fixes the six case-5 range/mean words at `+0x68C..+0x6A8`. |

These rows use the real tactics value and floating arguments, without an
occurrence, instruction address, source position or invented call. There
are no overlapping conflicting policies. Only `control` is needed beyond
the existing callee selector; `argument` and `evaluate_before` are unnecessary.

The private isolated promotion passes at **0/480 words**, body `0x778` in
retail extent `0x780`. The complete unit has zero byte or resolved-relocation
problems: `0x3144` checked bytes and 87 relocations. The production guard is
removed manually. Seven unused float declarations are removed. RNG state
and the name-hash accumulator share their actual initial value through
`u32 seed = random.seed = 1`; the name hash subsequently reseeds the RNG
before its 1,000-step warm-up. This preserves retail's seed store at
`+0x154` and does not introduce a dummy object or store.

Deleting that initial state store changes 73 words and moves `strlen`;
aggregate initialization instead emits additional constant data and fails
the complete unit. Neither probe is retained. The chained scalar
initialization passes at zero with the unused declarations removed.
An explicit field read (`random.seed = 1; u32 seed = random.seed;`) emits
a stack reload and changes 390 words. Hashing through a seed reference or
the generator field instead emits an oversized `0x790` body (394 and 418
positional words respectively). Separate initializations of the RNG state
and accumulator also pass at zero. Those probes do not replace the
chained initialization, which directly initializes both from one value.
`CollisionFish` and `StepGyoRace` are outside this lane's function ownership
and retain their guards.

Receipts, relative to the worktree:

- `.private/ctxrows/gyoracesim/m2c.c` and `m2c.err`.
- `.private/ctxrows/gyoracesim/switch1-range03/`: original-draft zero probe.
- `.private/ctxrows/gyoracesim/promoted-seed-chain/`: hygienic source,
  exact profile, both-pass compiler log, word score and complete-unit result.
- `.private/ctxrows/seed-state-probes.log` and
  `gyoracesim/seed-initial-state-full/`: additional seed-lifetime checks.
- `.private/ctxrows/receipts/baseline/`: PAL, object checks, coverage and
  all 149 linked/source-only object hashes.
- `.private/ctxrows/receipts/final/`: final clean-build and object/hash checks.

## Final clean validation

`CLEAN=1 JOBS=4` with the pinned proto image passes all ten initialized
PAL sections, the `0x01F64A00` memory end, and `SCES_511.90: OK`.
`check_objects.py` passes **149/149**, including gyoracesim's `0x3144`
bytes and 87 resolved relocations. All **148 other linked object hashes**
and all **148 other source-only object hashes** equal the baseline;
gyoracesim is the sole changed file in each set.

Coverage is **6,746 matched / 116 guarded / 10 assembly-only / 0 fuzzy**,
one native promotion above the baseline. The entire loaded image is
byte-identical to retail: 2,608,512 bytes, entry `0x00100008`. Its SHA-256
is `a103b0461a88e443a3af684cf150c05b5bd355e5a97ab2dc029c4d872bed0811`.
The whole ELF hash changes from
`928cfd449b8ed99c709e54fe0e0d19b90a341fcd69cb98f7d44c4981b1100509`
to `fbcb34a5f31054885b37d164a110975b29394f897a26e95bac8e69950d528fd4`;
this is outside the identical loaded payload.

The final receipt directory contains `clean-build.log`, `check-objects.log`,
`coverage.txt`, `hashes.json`, `comparison.json`, `loaded-image.json`,
`target-probes.json`, `scope.log`, and the exact `image-id.txt`.
The profile-row and source-promotion commits are separate and should be
applied together.
