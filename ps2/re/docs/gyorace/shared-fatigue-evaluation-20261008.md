# Shared fatigue correction

The shared header now declares `BREEDFISH_USED::fatigue` as u16, consistent
with retail's `lhu` at `sgInitGyoRace +0xB84` and its later unsigned stamina
read. The complete consumer audit and layout evidence are recorded in
[userdata's signedness notes](../userdata/fatigue-signedness-20261008.md).
The earlier signed-field/shared-audit blocker in [notes.md](notes.md) is
resolved; the initialization function's other blockers remain.

On 93cbbea the canonical all-drafts score improves from 432 to 431 of 1154
words, including two native words beyond retail's 0x1200 reservation. The
body remains 0x1208. `sgLoopGyoRace` remains 490/1676 and
`sgSysDrawGyoRace` remains 10/1172. No source expression or guard is changed.
All 149 active object file hashes are identical in the header-only trial;
the full checker remains 147/149 and PAL retains the known 0x26 text bytes.

The separate by-value rectangle proposal worsens the loop to 1189/1680
with a 0x1A40 body and breaks three active units. It is rejected; see
[the shared assignment evaluation](../mg_tanime/shared-assignment-evaluation-20261008.md).

Receipts: `.private/shared-eval/experiments/unsigned-fatigue/` and
`.private/shared-eval/experiments/rectangle-by-value/`.
