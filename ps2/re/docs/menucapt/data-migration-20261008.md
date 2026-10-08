# menucapt data migration (2026-10-08)

Baseline: `95f8fdd1`; 14 initialized-data
markers and 11 BSS markers. Every function already matches.
The pinned `chronicletwo_dev:sf-63f7a9e` image and full object/PAL checks
validate each accepted step. Public declarations remain compatible.

## Chapter state

The eleven reservations are native file-local definitions with the existing
word, pointer and signed-byte types. The two initialization flags have declared
size one; their three-byte alignment gaps remain piece padding, rather than
additional state. `MenuChapterStack` remains a native `mgCMemory`, so its
constructor and the unit's initializer retain the retail call.

`wait_cnt_918` is only reset, never advanced or read by the title state
machine. `voiceflag_921` records narration completion or the 1,500-frame
timeout. Their existing initialization guards and runtime instructions remain
unchanged. The typed data definitions do not introduce helper code or locals.

The `menucapt-state-{build,objects,metrics}.log` receipts in `.private/dataD/`
record PAL OK, 149/149 objects and unchanged unowned hashes. BSS markers
fall from eleven to zero; interim matched data is 93/336.
