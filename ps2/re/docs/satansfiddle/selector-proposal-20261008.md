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

`control` is deliberately a semantic projection, not a unique source-call
identity. It omits the compared expression or variable, the selected constant's
formal index, and source location. Two different subjects compared against the
same integer set may satisfy the same row. The optional `expected_matches`
assertion detects additional selected call arguments without using a count to
select them. All five calibration rows require exactly one distinct call
argument per compiler invocation, independently in the two mwccgap passes.

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
Every case record retains its label even when its bounds cannot be represented
or its range exceeds 16 values. Either defect rejects the selected function's
context reconstruction. Cases sharing a label are grouped; regions stop at the
next case boundary or the terminal default target. The default is not assumed
to be the switch join. Only forward layouts with no case after the default
target are supported. Missing targets or a nonterminal default reject
compilation. An explicit default body and cases sharing its boundary receive
no switch context because their exit has not been reconstructed.

Compiler pointers and statement positions locate regions internally. Neither
appears in the profile, and neither identifies an occurrence. This implementation
supports these verified forward control forms, not arbitrary reconstruction of
all optimized C++ control flow. Unsupported switch descriptors in a function
requiring control reconstruction fail immediately. Unsupported condition forms
yield no context; a selector left unconsumed fails completion. The coarse
projection can match another equivalent context, so nonzero consumption alone
does not establish a unique original source site.

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

At the ordinary-walk entry, both ends of each constraint must have masked
argument category 1 or 2 at +0x3c, an unevaluated marker at +0x3a, and a false
expression evaluate-first flag at +5. The verified evaluator at `0x004a44db`
skips a nonzero +0x3a marker; the early walk sets it at `0x004a4afd`.
Those two additional opcode signatures accompany the compiler-image hash.
Already evaluated arguments and other evaluation categories are rejected
before any list is reordered.

All applicable rows are collected before applying a policy. The greatest
specificity wins, counting callee, nested identity, control, and sibling
argument fields. Conflicting `(evaluate_first, evaluate_before)` policies at
that specificity abort compilation independently of row order. Consistent
winning rows all apply; losing consumer rows gain neither consumption nor
cardinality. Unscoped annotation applications retain their own consumption.

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

## Round-0 validation (historical)

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

## Round-1 review resolution

The implementation revision starts at integrated commit
`24d3d21a9ab6afed8d2f5793f52ed65c3dc60498` on
`work/dc2-midday-fullmatch`. Round-0 comparisons above belong to their recorded
`0c33a7e` source baseline. The round-1 comparison uses the saved linked and
source-only objects from `24d3d21`, which also contains five integrated game-unit
changes. No source or header is edited in this revision.

1. **Switch boundaries/defaults:** each descriptor retains its label and either
   the accepted value range or an unsupported marker. Oversized or nonrepresentable
   bounds reject the function's reconstruction. Defaults are separately named;
   nonterminal, nonforward and missing targets fail before any context can be
   returned. Cases sharing a terminal default remain boundaries without gaining
   a selector region. Algorithm regressions cover omitted ranges and nonterminal
   defaults; a genuine unsigned-case fixture exercises unrepresentable bounds.
2. **Policy conflict/consumption:** the hook collects applicable rows, resolves
   greatest specificity, rejects conflicting winning policies, and consumes
   only winners. Tests reverse the two reviewed conflicting selectors and also
   reject a broad callee row completely shadowed by a condition row in either
   order. Equal specificity with a consistent policy consumes all winners.
3. **Cardinality/projection:** positive, callee-scoped `expected_matches` counts
   distinct selected `(function object, call-argument descriptor)` pairs. These
   addresses only deduplicate callbacks; they are not selector fields. Each of
   the five game rows asserts one match per invocation. A duplicated source
   call fails with an observed count of two. Documentation explicitly records
   the omitted comparison subject and selected slot.
4. **Compiler regressions/restoration:** precise hook decisions assert both
   shot branches and all six placements; MIPS call preparation windows assert
   that the two unselected placement calls and the mode-40 shot stay equal,
   while selected calls change. A volatile store through a nested call's returned
   pointer retains its lvalue evaluation inside the parent ordinary walk at
   `-O3,p`, exercising two simultaneous saved orders and readback of both restored
   lists and heads. A separate `hook-test-faults` binary attempts a real write to
   unmapped address zero during inner restoration; failure must preserve an
   existing output and publish no new one or temporary object. The production
   binary ignores that test-only fault request. Temporary-filename, repeated-code
   and stale-selector tests remain.
5. **Ordinary-walk participation:** both endpoints require category 1/2, a zero
   evaluated marker and a false early flag. Unit tests reject earlier categories
   and evaluation states; a genuine fixture makes screen-limit argument 1 early
   and requires failure instead of a no-effect reorder.
6. **Key ordering:** `evaluate_before` and `expected_matches` precede the final
   `evaluate_first`. Only the five owned rows change. Inherited rows and their
   seven pre-existing unit/function inversions retain their order.
7. **Docker flags:** stage-local `RUSTFLAGS` provide the trailing LLDB/C++ libraries
   for Cargo binaries, tests and dependency build scripts. The explicit search
   path supplies the LLVM-19 library location to targets that do not receive this
   package's `build.rs` directives. The production binary is saved before building
   the fault-enabled test variant. Recipe-level wrapper-byte equivalence is
   unmeasured and is not asserted; game-object comparisons establish the measured
   scope. Neither native flags nor fault enablement become runtime environment
   settings in the development image.

The deliberate remaining limits are the coarse semantic projection, forward
equality reconstruction, and terminal-default switch form. Count assertions do
not prove the comparison subject's identity or detect substitution of one
projected identity by another when cardinality stays equal. Unsupported switches
in any control-selected function fail closed, even when its row targets a
separate condition. Fault handling aborts compilation; it does not promise to
repair lists and resume after an error.

## Round-1 validation

The only image tag built in round 1 is `chronicletwo_dev:sf-d8bf13c-proto2`,
final ID
`sha256:ee4b36860ca508d90749c0b42ad147234a900d1e07fa8c1c22cc93045a968524`.
The control patch SHA-256 is
`5e5ef3e622fb7e5545764075015ea21e9d0ad01b97baa283a8c95ebe94febc61`.
Both patches apply to the pinned SF revision and reconstruct all eight modified
SF files. The protected `sf-d8bf13c` and round-0 `sf-d8bf13c-proto` image IDs
remain unchanged. No wrapper-byte comparison between Docker recipes was made.

The image build passes 29 nonignored Rust tests; all five genuine-MWCC-3.0
regressions pass with the production wrapper and the separate fault-enabled
binary. The 19 Python build tests include the four adapter tests. The standalone
mwccgap, tail-padding, postprocess and static-BSS algorithm checks pass, as does
the coverage unit test. The pre-existing legacy compiler regression remains
ignored because it additionally requires MWCC 2.3.3. An exploratory optional
static-BSS fixture invocation used an already postprocessed object and failed
its expectation of two still-unbound statics; that input is unsuitable for its
prebinding fixture. The supported algorithm checks pass without that argument,
and no unrelated test or source is edited.

Focused full-wrapper builds pass both complete target objects. All ten
cardinality readbacks are `expected=1 actual=1`: one per row in each mwccgap
pass. `_SHOT` and `DrawMesWin` retain zero differing masked words and their
`0x900` / `0xB80` sizes. No function is promoted or source/header edited.

The final image's `CLEAN=1 JOBS=4` build exits zero. Verifier output:

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

`check_objects.py` reports `149/149 units pass`; refreshed coverage reports
6,745 matched functions, zero fuzzy, 117 guarded drafts and 10 assembly-only
functions. All 149 linked game objects and all 149 source-only comparison
objects are whole-file byte-identical to the saved reviewed `24d3d21` baseline,
including both calibrated units and metadata. The linked executable is also
identical. This is a same-source game-artifact comparison, not wrapper-byte
equivalence or a cross-machine reproducibility claim.

Receipts under `.private/receipts/proto2/` include `docker-build.log`,
`image-id.txt`, `patch-check.log`, `compiler-selector-tests.log`, `build-tests.log`,
the standalone test logs, `focused.log`, `profile-validation.json`,
`shot-word-diff.json`, `message-word-diff.json`, `clean-build.log`,
`check-objects.log`, `coverage.log`, `object-comparison.json`,
`protected-images.json` and `validation.json`. Failed exploratory attempts are
retained separately. The five profile rows preserve their selectors and policies,
add only cardinality validation, and leave inherited row order untouched.
