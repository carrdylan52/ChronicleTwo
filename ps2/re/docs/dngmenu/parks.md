# Remaining dngmenu matching work — October 8

The normal build continues to select retail assembly for every function
below. Scores are instruction words differing in `draft.sh` unless a size
only is shown; sizes include generated body length and retail manifest
padding as reported by that tool. Per-function `--diff` logs additionally
exclude trailing retail padding. Baseline: 29 matched, 18 guarded, 1 asm-only.
Current (after merging upstream, whose Satan's Fiddle profile matches `CDngFreeMap::Initialize`): 34 matched, 13 guarded, 1 asm-only.

## Guarded functions

| Mangled symbol | Before → after | Blocker and concrete reconsideration trigger |
|---|---|---|
| `CheckIsViewMove__11CDngFreeMapFiiRfRf` | 4/80 → unchanged | Scheduler: copy instructions at 0x44/0x54 exchange slots; X subtraction is duplicated into delay slot 0xDC and the final branch target advances by four bytes. Reconsider when a natural form fixes both copy ordering and final subtraction scheduling. Reversed declarations, delayed Y initialization and named/updated displacements did not help. |
| `CheckGeoramaMateria__FP22TRESURE_BOX_FLOOR_INFOiPi` | 105/112, 0x1A8/0x1C0 → 6/112, exact size | Induction register allocation: a2/a3 swap for counter and byte offset at 0xCC, 0xD4, 0xD8, 0xE0, 0xE4, 0x10C. Reconsider with a demonstrated typed item-loop form that keeps counter in a3 and offset in a2. |
| `MsgInit__12CMenuTreeMapFv` | 7/116 → unchanged | Scalar scheduling: height, width and first-line width loads and calculations at 0x140..0x15C use different order/registers. Reconsider with a natural line-position expression that emits height-first loads; dimension snapshots and existing SetMovePosGyou calls did not help. |
| `DrawGeoramaMateria__FiPciPii` | 0x404/0x400 → same sizes; 226/257 → 224/257 in detailed diff | Local allocation/scheduling: retail spills right column at sp0xA0; draft spills left column there. The corrected rectangle temporaries and signed page read still differ through the draw calls. Reconsider after deriving title/column/page local declaration lifetimes that reproduce retail's column spill and text scratch layout. |
| `Draw__11CDngFreeMapFv` | 0x614/0x610 → 0x630/0x610; 372/389 → 367/396 in detailed diff | Control flow/local allocation: frame now matches 0x280 and omitted NONE append is restored, but early returns, this/texture register assignment and debug traversal remain different. Reconsider after deriving the top-level nested active/alpha/texture condition and explicit order of the four floor-link queries. |
| `DrawRoot__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOT_INFOiUii` | 781/844, 0xD08/0xD30 → unchanged | Register lifetimes and shape branches: the 0xF0 frame matches, but root/marks/opacity saved registers differ from offset 0x2C and shape cases differ broadly. Reconsider with a case-by-case local-lifetime map for shape 0, then the shared color/mark setup. No shape rewrite attempted this round. |
| `DrawRoomOne__11CDngFreeMapF9mgRect_f_P16DNGMAP_ROOM_INFOUiif` | 553/584, 0x8A8/0x920 → unchanged | Local allocation and drawing branches: retail frame is 0xF0 versus draft 0xE0; retail preserves f22 and more integer state. Reconsider after mapping distinct picture, texture, special-overlay and glyph rectangle lifetimes before revising room flag paths. |
| `DrawDngRoomInfo__FP16DNGMAP_ROOM_INFO` | 0xBEC/0xB20 → unchanged; 742/763 detailed diff | Type/geometry and FP liveness: retail frame is 0x150 versus 0x140 and saves f23. The medal UV rectangle still uses the uncertain packed table described in notes.md. Reconsider when medal_xytbl_1736's typed UV layout is established and can replace the guessed highlight rectangle. |
| `LoadDngInfo__11CDngFreeMapFP9mgCMemoryiiii` | 957/1016 → 956/1016; 0xDD0/0xFE0 unchanged | Arena/local layout and path control flow: retail frame is 0x160 versus 0x1A0; stGetRest/stGetTop evaluation differs at 0x90..0xAC; later dungeon/path branches differ broadly. Reconsider after deriving the filename/direction/position local layout and arena argument order, then one dungeon path branch at a time. No path rewrite attempted. |
| `InitEnd__12CMenuTreeMapFv` | 198/224, 0x364/0x380 → unchanged | Local lifetimes: retail frame is 0xA0F0 versus 0xA0E0 and retains texture manager across the loader calls; selected/marked room state also differs. Reconsider with the manager and room-selection lifetimes established from 0x1F0FF0..0x1F1260. |
| `Step__12CMenuTreeMapFv` | 1506/1548, 0x1734/0x1830 → unchanged score | State-machine control flow and signed flags: frame is 0x110 versus retail 0x130; startup init flags use lbu where retail uses lb, and mode dispatch differs from offset 0xA8. Reconsider when the owned init flag types and mode-12 dispatch can be changed as one evidenced block before working through later states. |
| `Draw__12CMenuTreeMapFv` | 376/460, 0x6F0/0x730 → unchanged | Boolean control flow and local lifetimes: help visibility materializes booleans with sltu/xor instead of retail short-circuit branches; frame is 0x1A0 versus 0x1B0. Reconsider with explicit integer show_help initialization and a derived cursor/message scratch layout. |
| `DngTreeMapInit__FP9mgCMemoryPiii` | 0x610/0x400 → unchanged | Placement-new park: retail branches on v0 at 0x1F3564 with the pointer copy in the delay slot at 0x1F3568; the same pattern recurs at 0x1F3684/0x1F3688. Current draft also calls an undefined out-of-line CMenuTreeMap constructor and inlines ClsMes::Init. Stop constructor tuning here. Reconsider only after the dedicated placement-new lane supplies a natural matching form and coordinated constructor/Init emission work is available. |

## Standalone assembly function

`Init__6ClsMesFv`: NO DRAFT in the unmodified shared-header configuration.
The documented out-of-line proposal matches 174/174 nonpadding instructions
and passes the isolated unit-linked image check, but depends on moving the
shared inline definition. Reconsider with coordinated shared-header ownership
and verification of every affected caller; see clsmes-init-proposal.md.

No VU0/COP2 inline-code blocker was found in this unit's remaining assembly.
There is no new promotion-ledger reservation, and no shared patch is retained.
