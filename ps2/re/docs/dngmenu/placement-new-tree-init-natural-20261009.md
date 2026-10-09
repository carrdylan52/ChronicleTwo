# Natural dungeon-tree initialization controls

`DngTreeMapInit__FP9mgCMemoryPiii` remains guarded at **55 differing positional
words**, native body `0x3F8`, with mismatching relocation positions. Retail's
GLOBAL/FUNC symbol at `0x1F34E0` is also `0x3F8`; its `0x400` next-address extent
includes eight zero padding bytes. No constructor selector or float row is
justified by these residuals.

The established types, message constructor loops, mode paths and earlier
negative controls are documented in [notes.md](notes.md),
[midday-pass-20261008.md](midday-pass-20261008.md) and the two round notes. This
study reads those findings first and adds actual filename-data hygiene and
bounded controls of the existing opening flag. The fresh required m2c run
succeeds; its receipt is
`.private/pntc/receipts/dngmenu-tree-init-m2c.log`.

## Actual construction and filename data

`CMenuTreeMap` is `0x2FBE0` bytes, requiring `0x2FC0` sixteen-byte arena blocks
under the real `sizeof(T) / 16 + 2` convention. `CDngFreeMap` is `0x110` bytes,
requiring `0x13` blocks. Their genuine message/rectangle array initialization
makes the measured constructors class 3. Both allocation-result checks already
match retail's `v0` check and saved-pointer delay slot. Their real loops remain
unchanged; a class-6 capability is not applicable to them.

The menu file request is an actual two-pointer filename array: the first pointer
names `dmap%d.pac` in the existing forty-byte filename buffer, and the second is
null. Replacing the external `at_3478` seed reference with either a genuine
zero-initialized aggregate or `char *names[2] = {NULL, NULL}` preserves every
selected masked instruction. Initializing the aggregate directly with the
filename yields the same complete native object as the zero-pair forms.

At function offset `0x3A8`, the genuine initializer uses the existing generated
`@2588` local eight-byte zero datum, followed by the same LD/SD and first-pointer
store. The canonical native object already contains that datum. All three
initializers, and the final combined natural source, have complete native SHA256
`7c5adf23ea0fa52caac2b99e66a330a5a17984a00e4332a4e2f6e6417c7ba51d`.
The canonical and sizeof-only objects instead share
`2658d9feaf06ccec6c83dbf093c65acb92e4f23b02b7c6afef3255e5794dded8`.

Retail `@3478` is a local object of eight bytes at `0x37D5D0`, in the layout's
`.sbss` zero-storage range. Native `@2588` is local, eight bytes, with eight-byte
section alignment and logical zero contents. It has exactly one function
consumer: this request's LD. The retail ELF presents a broad file section, so
its file slice does not contain these BSS bytes; the audit uses the established
layout's NOBITS region rather than treating an empty file slice as data.
The generated assembly marker remains necessary while the production fallback
is active; this study does not remove it or claim complete native-data migration.

## Bounded controls and residual

| Genuine source control | Words | Native body |
| --- | ---: | --- |
| Current 31-row production profile | 55 | `0x3F8` |
| Both sizeof-based reservations | 55 | `0x3F8` |
| Zero-initialized actual filename aggregate | 55 | `0x3F8` |
| Filename/null aggregate initializer | 55 | `0x3F8` |
| Actual two-pointer array | 55 | `0x3F8` |
| Existing integer flag set by mutually exclusive increments | 119 | `0x404` |
| Existing integer flag set by OR with one | 119 | `0x404` |
| Existing boolean flag set by OR with true | 119 | `0x404` |
| Combined sizes and actual filename array | 55 | `0x3F8` |

The flag controls add no state or calls: either mode 3 or mode 0 requests the
same existing separate-map path, at most once. They are bounded semantic
controls, not evidence of a recovered counting purpose. All three oversized
results are retained as negatives.

The first residual starts at `0x214`: retail uses a direct mode comparison and
branches for 3 or 0, while the native boolean lowering computes equality values.
The cursor-file prefix also forwards the just-published buffer pointer into
`LoadFileMenu`; retail reloads the global. The latter removes an instruction
while the boolean prefix adds one, leaving the same final body size. The
common UI suffix from `0x30C` through the return, including both actual float
arguments, matches. Prior condition/cursor-type/lifetime negatives are not
repeated, and no dummy pointer, flag or helper is introduced to force a reload.

## Isolation and review boundary

All nine controls preserve the 47 other manifest function scores, masked
bodies, sizes, normalized references and symbol bindings. They also preserve
the two emitted vtables and all 84 allocated noncode sections, including
logical BSS shapes. The selected seed identity change is recorded separately;
unchanged function scores do not by themselves prove object equality.

The current 31 accepted placement rows are retained unchanged. A dngmenu-only
compiler run consumes none of those other-unit rows. No image, source guard,
shared header or production profile changes. The private source proposal
`.private/proposals/dngmenu-tree-init-natural-source.patch` retains the guard;
`.private/proposals/dngmenu-tree-init-body-size.patch` corrects only the owning
header annotation to the actual `0x3F8` symbol size. Both pass apply checks.
Neither is a promotion or a complete wrapper acceptance claim.

Reproduction and exact source/profile/object hashes are in
`.private/pntc/dngmenu-tree-init-natural/freeze.json`. The study preserves
`controls`, `flags`, `natural-source/driver`, `final-audit-0` through
`final-audit-3`, and the explicit `selected-equality` log/exit receipts. Initial copied-auditor path and source-symbol
lookup failures remain saved; the corrected audits succeed without any game
code or assembly change.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](../satansfiddle/placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
