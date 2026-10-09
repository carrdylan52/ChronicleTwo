# Accepted placement-family source audit

An independent review of all 34 activated callers across 21 translation
units finds no executable source-hygiene blocker. The review covers the
accepted bodies, selected constructor chains, declarations and compiler
generated members, typed array and serialized-buffer walks, literal/data
ownership, source/header purpose comments, symbol bindings, and the exact
authorized profile/source deltas. Existing unrelated source is outside
this audit's activation scope.

Nine inherited purpose-comment omissions occur in four owning source units:
two placement-animation constants in `editeff`, the fishing mode enum,
the invention preview enum, and three monster-book constants plus two
monster-book enums in `menuchr`. The exact reviewed changes add only comments;
all noncomment C++ tokens remain unchanged. Each declaration's purpose is
recorded in its owning unit's `placement-new-purpose-comments-20261009.md`.
No shared header, function body, policy row, generated special member or
compiler implementation changes.

The baseline is the accepted 34-caller state at `c655d012`, rather than
the original upstream baseline. Before editing, 306 assembled and 149
source-only object hashes and the complete linked ELF hash were frozen.
After the pn15 rebuild every one of those 455 object hashes is identical.
The whole ELF remains SHA-256
`6352338ce8ff3fef0df4d54215ab1ca9348ae4db098c1a35cb7da359ba17b82d`.
This whole-ELF identity applies to this comment-only comparison; prior
source promotions require the separately documented resolved retail
section checks and may change ELF metadata.

PAL verification prints `SCES_511.90: OK`; all 149 complete objects pass.
Explicit context/objdiff/progress refresh followed by native coverage remains
6,783 matched / 82 guarded / 7 assembly-only / 0 fuzzy. Production policy
remains 34 placement rows and 44 eligible constructions, profile SHA-256
`f6353a6cb5facbf60a0f48d2be2db95354573f58a4c85a0295b14ea78c3cafe0`.

Private evidence is frozen in
`.private/pntc/promotion-hygiene-final/{review.md,findings.json}` and
`.private/pntc/source-hygiene-34/{before.json,identity.json}`. The four exact
comment patches retain their reviewed old/new source hashes. Explicit
zero-status receipts are
`.private/pntc/receipts/source-hygiene-34-{build,objects,identity,progress,coverage}.log`
and matching `.exit` files. These checks preserve the accepted production
boundary; private shared-header proposals remain inactive.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
