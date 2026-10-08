# Predicate and ordinary argument-walk prototype (2026-10-08)

## Source identity

The pinned Satan's Fiddle revision is
`365415fa2fd7bf69e899044b704b571a64ed4e6c`, followed by the existing
nested-arguments patch. The game-source baseline is
`0c33a7e1464e3c6d7a0a21152c9ed2d89fe3f327`, including upstream `d8bf13c`.
No game source or header change is required or permitted in this prototype.

The smallest distinguishing context is the integer value set in the enclosing
control statement, combined with the existing unit/function/callee/float
identity. `control` contains a `kind` (`condition` or `switch`) and `values`:
the attack-type-90 condition is `[90]`, the DQ switch cases are `[9, 10]`, the
final bottom/DQ condition is `[7, 9, 10]`, and the center condition is `[11]`.
These are the actual weapon and window-mode values present in source, not
positions in a sequence of calls. Repeated calls with the same resulting
semantic identity receive the same policy.

An optional `argument` selector identifies a sibling floating constant by its
formal argument position, type, and exact bits. At `CalcAutoPosSet`, screen
limit argument 1 distinguishes 512-wide X placement from 480-high Y placement.
This uses existing scalar arguments rather than introducing helpers or locals.

The existing callee and nested-call selectors cannot separate these direct calls.
Inline-helper identity would depend on names introduced by decompilation, while
source lines and occurrence counters would describe placement in a file rather
than game behavior. Integer control values and existing scalar argument values
are sufficient; no source parser, helper insertion, or backend instruction patch
is required.

## Verified compiler provenance

MWCC 3.0's statement lowering loop reaches `0x00437140` with the lowered
statement in ESI, immediately before publishing it as the current statement.
The statement list already retains conditional comparisons and switch cases;
inline helpers and optimized temporary assignments are inside these statements.
The prototype traverses that bounded list once for each selected function.

Statements contain their next link at +0, kind at +4, expression at +10, and
label/branch target/switch descriptor at +14. Kind 2 marks labels; kinds 6 and 7
branch on true and false. Equality and inequality expression kinds are `0x17`
and `0x18`. Integer literals have kind `0x32` and four 32-bit limbs in decreasing
significance order. Only properly sign-extended 32-bit values are accepted.

A forward unequal exit bounds the equality-controlled statement region. An
immediately preceding chain of equal jumps to that region's entry label supplies
the other terms of a short-circuit OR. This reconstructs `[7, 9, 10]` as one
control identity, rather than asserting that its body only runs for value 10.
A switch descriptor contains its case-list head at +0 and default label at +4;
case records contain their label at +4 and inclusive integer bounds at +8/+24.
Cases sharing a label are grouped; regions stop at the next case or switch end.

Compiler pointers and statement positions locate regions internally. Neither
appears in the profile, and neither identifies an occurrence. This implementation
supports these verified forward control forms, not arbitrary reconstruction of
all optimized C++ control flow. Unsupported contexts leave selectors unconsumed
and fail compilation.

## Third schedule

Center X's half ratio is an assignment to an optimizer temporary shared with
the later center-Y call. A Boolean early-walk policy cannot reproduce retail:
early evaluation also performs the floating transfer immediately. A private
probe instead moves that real argument before screen limit argument 1 during
the compiler's ordinary register-argument evaluation walk, and restores formal
order before the argument-transfer walk. It eliminates all four center-X word
differences without changing the `0xB80` function size.

`evaluate_before` expresses that separate scheduling policy using a formal
argument index. It requires `evaluate_first: false`. The verified walk starts
at `0x004a4bb9` and ends at `0x004a4bdc`; argument slots are already assigned.
Only linked-list evaluation order changes. AST expressions, formal slots,
register assignments, backend instructions, object bytes, and relocations are
not edited. The compiler emits the instructions naturally. Per-frame restoration
supports nested compiler evaluations, and incomplete restoration is an error.

## Accepted rows

- `_SHOT`, binary32 160, `SetValue__16CEffectScriptManFifii`, condition `[90]`:
  evaluate first.
- `DrawMesWin`, binary32 half, `CalcAutoPosSet__Fffff`, switch `[9, 10]`:
  evaluate first.
- `DrawMesWin`, binary32 0.95, same callee, condition `[7, 9, 10]`:
  evaluate first.
- `DrawMesWin`, binary32 half, same callee, condition `[11]`, argument 1 = 480:
  evaluate first.
- `DrawMesWin`, binary32 half, same callee, condition `[11]`, argument 1 = 512:
  ordinary evaluation before argument 1.

## Validation

Both builds run at game-source commit
`0c33a7e1464e3c6d7a0a21152c9ed2d89fe3f327`; game source and headers have no
working-tree changes. Baseline image `chronicletwo_dev:sf-d8bf13c` remains
`sha256:2756b29e98231a46505359d94343d5d3510a0d765b671b029076efca92b8f46b`.
Its clean-build receipts show 147/149 passing objects and `0x26` differing
`.text` bytes. The baseline profile is preserved except for the five owned rows.

The only built image tag is `chronicletwo_dev:sf-d8bf13c-proto`, image ID
`sha256:3b98a5d10e86ab2c285952105096a5fb7ab6bb374a42d87c86ccea0770b79eaf`.
The Docker build checks and applies the patch after nested arguments, passes
23 nonignored Rust tests, and includes the CLI compiler-regression runner.
The additional genuine-3.0 selector test passes, exercising grouped switch
cases, compound equality conditions, sibling constants, ordinary-walk priority,
repeated code generation, logical-unit identity with temporary filenames, and
stale-selector rejection. All four existing adapter tests also pass.

Focused full-wrapper builds log all five rows exactly once in each of mwccgap's
two passes. `_SHOT` improves from four differing words to zero, and `DrawMesWin`
from 17 to zero. The complete `actscript` object passes `0x47FC` checked bytes
and 1,111 relocations; `nd_meswin` passes `0xBF28` and 1,364 relocations. Function
sizes remain `0x900` and `0xB80` respectively. No function guard is promoted.

The final validation uses:

```sh
export CHRONICLETWO_IMAGE=chronicletwo_dev:sf-d8bf13c-proto
H=/home/dylan/.t3/worktrees/dark-cloud-3/t3-cc986570/scripts/chronicle-two/container.sh
CLEAN=1 JOBS=4 bash "$H" ./build.sh
bash "$H" python3 scripts/build/check_objects.py
```

The clean build exits zero. Verifier output:

```text
  .text     OK
  .vutext   OK
  .data     OK
  .vudata   OK
  .rodata   OK
  .init     OK
  .ctor     OK
  .vtables  OK
  .rdata    OK
  .sdata    OK
  .bss      OK (memory ends at 0x01f64a00)
SCES_511.90: OK
```

The complete checker reports `149/149 units pass`. Whole-file comparisons
against the saved baseline confirm all 147 other linked game objects and all
147 other source-only objdiff base objects are byte-identical, including
metadata. Only the two intended objects change. Their final linked-object
SHA-256 values are:

- `actscript`: `7bcb1994638f63d15b27a07502cb6d44b7ead724b95b41ebc88ae0e8712f1c92`.
- `nd_meswin`: `49d904d2ab6d18911dfa6062f2367a204b35c96151683c60efc4af2d2cca1638`.

Receipts are under `.private/receipts/baseline/` and
`.private/receipts/prototype/`: `docker-build.log`, `compiler-selector-tests.log`,
`adapter-tests.log`, `focused.log`, `shot-word-diff.json`,
`message-word-diff.json`, `clean-build.log`, `check-objects.log`,
`object-comparison.json`, and `image-id.txt`. All implementation changes are
confined to the assigned Dockerfile, patch, profile, and documentation files.
No outside-file proposal, unresolved matching blocker, or network write is
required.
