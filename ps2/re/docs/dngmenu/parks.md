# dngmenu remainder after r2 — October 8

Current lane source: 39 native functions, 8 guarded drafts, 1 asm-only
function. Upstream supplies the exact `CheckIsViewMove` implementation and
its translation-unit helper seed; its former park is resolved.
The r2 baseline at `2f71f10` was 34/13/1. All assigned targets were
remeasured with the merged SF profile before experiments. Rows below use SF
diagnostic differing words (including retail padding), or body/retail bytes
when the body exceeds retail. Detailed disassemblies are saved privately.
Only complete-unit canonical checks authorize the two native promotions;
see [r2.md](r2.md).

## Assigned targets

| Target | SF baseline → retained result | Status and concrete reconsideration trigger |
|---|---|---|
| `CheckGeoramaMateria__FP22TRESURE_BOX_FLOOR_INFOiPi` | 6/112 → exact | Promoted. Reusing the completed group-search index for item traversal fixes the induction allocation. Complete-unit byte/relocation check passes. |
| `MsgInit__12CMenuTreeMapFv` | 7/116 → unchanged | Guarded. Height/width/first-line width loads at +0x140..+0x15C remain reordered. Actual-unit removal fails bytes and the height relocation. Reconsider with demonstrated integer argument/store scheduling evidence; floating annotation rows cannot select this integer block. Named first-line X, dimension snapshots, and existing line-position inline calls do not help. |
| `LoadDngInfo__11CDngFreeMapFP9mgCMemoryiiii` | 957/1016, 0xDD0/0xFE0 → 914/1016, 0xF68/0xFE0 | Guarded. Full-width candidate arrays restore arena spB0, filename spE0 and coordinate sp14C..sp15C layout. Signed tables, independent dungeon checks and distinct forward/reverse point loops are corrected. Initial negative-test encoding, remapping branches and loop scheduling still differ; native table migration is also required before canonical activation. See midday.md. |
| `DrawRoot__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOT_INFOiUii` | 781/844, 0xD08/0xD30 → 708/844, 0xCEC/0xD30 | Guarded. Event tints now share retail's single branch. Shapes 4 and 5 are excluded from the five-pixel adjustment, and dispatch is an ordered comparison chain. Root/marks/opacity and color/mark saved registers and individual shape loops still differ. Reconsider with a case-zero lifetime map and the default/event color register map before changing the other shapes. |
| `DrawRoomOne__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOM_INFOUiif` | 553/584, 0x8A8/0x920 → 459/584, 0x8C4/0x920 | Guarded. Missing glyph destination rectangle and pre-draw overlay geometry restored. SF frame is 0x110 versus retail 0x120. Reconsider after mapping separate picture, texture, special-overlay, glyph and tint lifetimes; room flag paths and texture-number signedness still require attention. |
| `DrawDngRoomInfo__FP16DNGMAP_ROOM_INFO` | 0xBEC/0xB20 (742/763 detailed words) → 0xBA0/0xB20 (611/744) | Guarded. Separate panel temporaries and simultaneous seal choices restore the 0x150 frame and f20–f23 saves. Message null checks, activity branches, region placement and typed room pointer are corrected. Integer register allocation and scheduling remain; see midday.md. |
| `Step__12CMenuTreeMapFv` | 1506/1548, 0x1734/0x1830 → 1488/1548, same sizes | Guarded. Natural mode switch, question-message timing and integer selection-change flag improve the draft. Frame remains 0x110 versus retail 0x130, with one fewer saved GPR; the correctly sized 72-byte time-text buffer stays intact. Reconsider the key/message/result lifetimes and later state branches; see midday.md. |
| `InitEnd__12CMenuTreeMapFv` | 198/224, 0x364/0x380 → exact | Promoted. Cached floor bound, loader locals, sequential bounds, separate sub/boss checks, coordinate pair updates, output-size lifetime, and cursor state now match all 224 words and the complete unit. |
| `DrawGeoramaMateria__FiPciPii` | 0x404/0x400 → unchanged (224/257 detailed words) | Guarded. Retail spills the right column at spA0; draft spills the panel-left position. Earlier declaration of right column does not change this. Reconsider with a demonstrated title/column/page lifetime map that produces the correct spill and message scratch slots. No profile row was attempted for this broad allocation remainder. |
| `Draw__11CDngFreeMapFv` | 0x628/0x610 (353/394 detailed words) → exact | Promoted in the midday lane. Retained texture/manager, temporary mark rectangle, local-font access, typed room pointer, positive conditions, debug Y base, ordered query calls with moon/sun/normal declarations, and separate ON/OFF calls reproduce the complete unit; see [midday.md](midday.md). |
| `Draw__12CMenuTreeMapFv` | 446/460, 0x6E0/0x730 → exact | Promoted in the midday lane. Restored numeral alpha and money-digit Color, manager lifetime, ordered wrap comparison, 32-byte numeral buffer, separate board temporaries, named digit rectangle, coordinate pair, cursor branches, and signed question flag. Complete-unit byte/relocation check passes; see [midday.md](midday.md). |

## Constructor and emission remainders

`DngTreeMapInit__FP9mgCMemoryPiii` remains guarded at 55/256 words,
0x3F8/0x400 bytes. Its natural constructor and real message-member array
now reproduce both placement-new branch/copy pairs. The opening-mode
comparison and cursor-buffer store forwarding remain different.
Reconsider those two blocks; see [midday.md](midday.md).

`Init__6ClsMesFv` remains assembly-supplied by assignment. The natural
constructor causes exact standalone emission from the unchanged shared
header. The private canonical checker reports only tree-initialization
problems, with none for Init. Marker removal is a conditional private
proposal after tree initialization is exact; see
[clsmes-init-proposal.md](clsmes-init-proposal.md).

No VU0/COP2 inline-code blocker was identified in this unit. No ledger rows
or compiler-profile rows were added in r2. One-hypothesis experiment logs are
private at `.private/experiments-dngmenu-r2.md`; receipts for final acceptance
are at `.private/receipts/r2-final/`.
