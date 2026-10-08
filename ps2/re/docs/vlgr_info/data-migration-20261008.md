# Villager configuration data migration

## Baseline

Round 2 starts at `55cdb46c` with 35 `INCLUDE_RODATA` and 21 `INCLUDE_BSS`
markers; objdiff reports 7,252 / 7,836 matched data bytes.

## Schedule and parser storage

All 21 BSS markers have native definitions using the types established in
`notes.md`. Parser pointers and counters have file-local storage.
`ProgressInfo` retains external linkage because the generated VU data object
`vutext.data.s.o` refers to that symbol; making it static passes the unit
object check but fails the complete link. Its type is
`GAME_PROGRESS_INFO[GAME_PROGRESS_MAX]` and its size remains 0xC00.

The path, progress condition, posture, motion and file-name literals are
inline strings. The progress condition uses the retail Shift-JIS bytes
`88 C8 8C E3` for 以後, represented by octal escapes to preserve source encoding.
Repeated uses of "sit" share the compiler's string object.

The complete PAL build reports `SCES_511.90: OK`; all 149 objects pass.
Receipts: `.private/dataB-r2/vlgr-strings-{build,check}.log`.
