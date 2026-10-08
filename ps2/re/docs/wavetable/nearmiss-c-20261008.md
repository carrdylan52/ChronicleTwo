# Ripple-grid near-miss assessment

Base `3d49d02`, image `chronicletwo_dev:sf-d8bf13c`, canonical MWCC
3.0-011126 and the checked-in compiler profile. `CWaveTable::Effect`
remains guarded at **27/324** relocation-masked words, body/extent
`0x510`. No source, header, pragma, or profile edit is retained.

Fresh `decompile.sh`/m2c output and the existing
[wave-table notes](notes.md) confirm the damped update, seam averaging,
two `24 x 24` planes, `0x900` plane stride, and `0x60` row stride.
The type remains `0x1208`: `height[2][24][24]`, signed `current` at
`0x1200`, and the virtual-table pointer at `0x1204`. Existing dependent
texture/draw declarations are unchanged.

The inherited close draft traverses rows through flattened float
pointers. It is not a basis for promotion. Its 18 coefficient-register
differences and nine seam-addition operand reversals remain exactly the
documented residual. The existing `WaveSample` helper trials and
coefficient/seam expression probes are not repeated.

## Signedness and bounded indexing

New unsigned row and column counters are tested independently and
together with bounded plane indexing, row pointers confined to their
own row, and direct member indexing. None restores retail's eight-cell
wave unroll:

| Bounded access form | Differing words / 324 | Body bytes |
| --- | ---: | ---: |
| Plane array pointers | 321 | `0x208` |
| Separate above/current/below/previous row pointers | 320 | `0x238` |
| Direct height members | 323 | `0x3F8` |

All four other native functions remain exact in these trials. The
unsigned bounds do not explain the flattening-dependent unroll.

New bounded current/previous sample references, genuine row pointers,
base/declaration-order changes, and reversing the self-product retain
the scalar inner wave loop (`0x208`, 321 words). No pointer induction,
row-crossing arithmetic, helper, or unrelated type view is introduced.

## Linear-storage diagnostic

Private owned-header variants declare the same-sized storage as either
two planes of 576 floats or one 1152-float array, updating all unit
accesses to use linear sample indices. These tests do not retain any
overlay or union and do not change the real header.

The signed sample-loop forms reproduce the inherited **27/324** Effect
score and unrolled extent, but regress three previously matching native
functions. The constructor grows from `0x90` to `0x100` with plane
storage or `0x168` with fully flat storage; `GetEffect` gives 56/84 words
and body `0x144`; `CreateTexture` gives 34/264 words
and body `0x418` with plane storage or 173/264 with fully flat storage.
Unsigned sample-loop forms give 34/324 in Effect. Direct indexed or
referenced-sample forms yield scalar Effect bodies `0x230` with 319/320
words. Only the destructor remains exact in the linear-layout variants.
This provides no supported type change or improvement to retain.

## Compiler unroll diagnostic

The compiler's own help records that canonical `-O3,p` enables loop
unrolling. Private scoped `opt_unroll_count 8` pragmas, alone or with
accepted `opt_unroll_instr_count 100`, preserve the scalar plane/row/
sample bodies and their 321/320/321-word differences. They do not make
bounded indexing match the retail unroll. Instruction limits 200 and
1000 are rejected as out of range; those failures are not optimizer
measurements. No pragma or build-option change is retained.

The reconsideration trigger remains a bounded source form that
reproduces the eight-cell/eight-row schedule and then resolves both
coefficient coloring and seam operand order. Effect has no call-argument
identity supporting an evaluate-first selector, and these tests do not
establish an alternative.

## Receipts

Receipts are under `.private/nearmiss-c/`:
`Effect__10CWaveTableFv.m2c.txt`, `wavetable-before/`,
`wave-width-*`, `wave-typed-points-*`, `wave-linear-*`,
`wave-policy.log`, `wave-policy-bounds.log`, `compiler-help.log`, and
the corresponding private sources, objects, profiles, comparisons, and
instruction listings. `ledger.jsonl` records successful native trials;
compile failures remain in per-case `compile.log` files. Full production
validation is in `final-{build,objects,progress}.log`,
`final-hashes.json`, and `final-validation.json`.
