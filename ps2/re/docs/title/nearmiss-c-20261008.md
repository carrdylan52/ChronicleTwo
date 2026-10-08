# Title input near-miss assessment

Base `3d49d02`, image `chronicletwo_dev:sf-d8bf13c`, canonical MWCC
3.0-011126 flags and the checked-in Satan's Fiddle configuration.
`TitleModeKey` remains guarded; no source or profile change is retained.

## Production and control contexts

The fresh all-draft production-profile comparison is **379/624** words,
body `0x9B8` in retail extent `0x9C0`. The earlier documented private
zero/128.0f `CalcMenuAdd__FPfff` selectors restore body `0x9BC` and the
**15/624** residual. Thus 15 is the control-profile score, not the
checked-in-profile score reproduced here. The selectors remain private.
This agrees with the context distinction in [notes.md](notes.md).

## New linkage, width, and ordering probes

Retail records `DCTitleStep`, `TitleMCCheckInit`, `TitleMCCheckKey`,
`CalcPushAlpha`, and `TitleModeKey` as LOCAL functions. Private static
declarations and definitions, individually and together, leave 379/624
and all 35 existing native matches unchanged. They do not resolve the
snapshot allocation or fade schedules.

Private static definitions of the unit-local `TitleMCCheckInport`,
`TitleMCCheckNow`, `TitleMCCheck`, `TitleInfo`, and
`TitleMainMCCheckPhase` likewise leave the target's instructions unchanged.
Unsigned start flags, push mask, lost-card flag, completed-port result,
title step, and old selection do not improve the score. Capturing either
port in signed-byte storage and narrowing it explicitly at comparison
also preserves the original snapshot allocation in the control context.

| New source form | Production words / 624 | Control words / 624 |
| --- | ---: | ---: |
| Chained menu/cursor alpha stores, preserving retail store order | 379 | 15 |
| Chained four extras-selection clears, preserving retail store order | 379 | 15 |
| Both chains | 379 | 15 |
| Chained active-port/card-port clears | 554 | Body exceeds retail |
| Extras comparison as `select > count - 1` | 379 | 22 |
| Literal-left selection clamps and flag tests | 379 | 15 |
| Either signed-byte port snapshot with explicit unsigned narrowing | 379 | 15 |

No invented helper, volatile snapshot, altered field width, or shared
declaration is introduced. The documented signed halfword title fields
and unsigned byte port-state accesses remain supported by retail loads.
The guard's reconsideration trigger remains a natural source formulation
that resolves both the five snapshot operands and the context-dependent
fade argument order without the private profile.

## Receipts

All receipts are in `.private/nearmiss-c/`: fresh `TitleModeKey__Fv.m2c.txt`,
`title-before/`, `title-linkage.log`, `width-trials.log`, `title-order.log`,
and their per-case source, object, comparison, and disassembly files.
`ledger.jsonl` records successful canonical native compilations.
The baseline build, object check, refreshed progress, coverage, and all
149 object inventories are `baseline-{build,objects,progress}.log`,
`baseline-coverage.txt`, and `baseline-hashes.json`.
