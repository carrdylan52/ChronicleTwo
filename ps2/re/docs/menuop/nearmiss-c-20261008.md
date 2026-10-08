# Save-menu near-miss assessment

Base `3d49d02`, image `chronicletwo_dev:sf-d8bf13c`, canonical MWCC
3.0-011126 and the checked-in compiler profile. `CSaveMenuClass::KeyStep`
remains guarded at **24/1692** words, body `0x1A64` in extent `0x1A70`.
No source, header, or profile edit is retained.

## Storage and dependency review

The exact instruction comparison reproduces
[round 3's stack inventory](round3-20261008.md): all 24 differences are
frame adjustments or stack addresses. The frame is `0x160`, against
retail's `0x1A0`; locals preceding `0x128` already have retail offsets.
No observed access identifies an owner for retail `0x128..0x167`.

The existing documented CDC2Mes setters distinguish the caller's small
message arrays from the callee's `previous[MES_VALUE_MAX]` storage.
`SetMsgVolumeNoOne` uses a two-integer input and calls the regular setter
with count one. The regular overloads' 16-entry previous-value arrays
are their own locals, not evidence for an additional caller array.
The prior visible/inline-body trials already test and reject that caller
storage explanation; they are not repeated.

The local scrollbar helper is GLOBAL in retail and retains that linkage.
The save-card helper has no retail outline symbol and cannot supply an
evidenced missing stack object. No nine-pair coordinate array, dummy
buffer, enlarged unrelated object, or unsupported placement construction
is introduced. The guard remains dependent on an independently supported
owner for the 64 bytes, rather than a diagnostic way to reserve them.

## New counter probe

Changing only the thirteen-file row counter to `unsigned int` gives
**25/1692** words and leaves all 34 existing native matches exact. It adds
the signed/unsigned loop-comparison difference while preserving every
stack mismatch. The existing signed counter is retained. The previously
resolved transition predicates and shared transfer local are unchanged;
their old allocation probes are not repeated.

## Receipts

Receipts are in `.private/nearmiss-c/`: `menuop-before/`,
`save-row-counter-u32/`, `ledger.jsonl`, and
`KeyStep__14CSaveMenuClassFv.m2c.txt`. The fresh direct m2c invocation
reproduces the already documented jump-table-name failure. The complete
analysis-only input and unblocked m2c stack output remain at
`.private/menuui/save-analysis.s` and `.private/menuui-r3/save.m2c.txt`.
No generated assembly is changed. Baseline menuop checks
`0x7DE4` allocated bytes and 2,075 resolved relocations with zero problems;
the full receipts are `baseline-objects.log` and `baseline-build.log`.
