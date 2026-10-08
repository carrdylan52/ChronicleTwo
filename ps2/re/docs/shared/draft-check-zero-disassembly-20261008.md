# Zero words in draft disassembly

`draft_check.py --diff` compares an instruction list parsed from objdump's
text output. Without `-z`, objdump collapses runs of zero words, omitting
real nops and shifting the display's inferred offsets. Both retail and
draft objdump calls now include `-z`.

The unmodified base's plain-MWCC `sgSysDrawGyoRace` display reports 10
differing instructions out of 1155. With `-z`, it reports the same ten
differences out of the complete 1172-word reservation. The additional 17
rows are the omitted zero words. The raw-byte comparator, relocation
comparison and promotion logic are unchanged.

The independent full build preserves all 149 game-object file hashes,
147/149 object passes, and the known 0x26 PAL text bytes. Canonical draft
scores for gyorace and mg_tanime are unchanged. This is a diagnostic fix,
not a game-function promotion.

Receipts: `.private/shared-eval/base/drafts/gyorace/plain-diff-before.txt`,
`.private/shared-eval/experiments/draft-check-zero/drafts/gyorace/plain-diff-after.txt`,
and `.private/shared-eval/experiments/draft-check-zero/`.
