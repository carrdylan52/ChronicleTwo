# Inline treasure-box constructor evaluation

The proposed move of `CMapTreasureBox() { Initialize(); }` from map.cpp to
mapparts.hpp is evaluated alone on 93cbbea. The existing base constructors
initialize the object, then the derived constructor calls its virtual
`Initialize`; no replacement vtable stores or synthetic emission site is
introduced. The 0x680 class layout and 0x161970 constructor address are
unchanged.

Retail has a standalone 0xB8-byte constructor, within its 0xC0 reservation,
with processor-specific symbol binding 13. `CMap::CreateTrBox` needs its
address as an array-construction callback, so the inline proposal still
emits that standalone symbol with the exact instructions and relocations.
The source-only object has binding 13; normal postprocessing converts
coalesced binding 13 to weak binding 2. The old out-of-line definition has
global binding 1.

The full checker remains 147/149 and PAL retains only the 0x26 text bytes.
All allocated bytes and relocation identities equal the baseline. map is
the only whole-file hash change, due to the constructor's binding metadata;
the other 148 object hashes are identical. The ctor is already source
supplied, so this does not promote a function.

A sweep of 25 affected guarded units changes only `EditInit`. The canonical
all-drafts baseline is 1226/1776 words with a 0x1B78 body; the inline header
produces 1248/1783 with a 0x1BDC body, exceeding the 0x1BC0 reservation.
It exposes the required constructor chain but does not make that caller
match. These are all-drafts measurements, distinct from earlier isolated
caller results in editloop's notes.

This proposal also requires deleting the definition in map.cpp, which is
evaluation-only for the shared lane. Neither hunk is retained. The paired
patch is `.private/proposals-out/inline-map-treasure-box.patch`; applying
only its header hunk would leave a duplicate definition.

Receipts: `.private/shared-eval/experiments/inline-treasure-box/`, including
`ctor-symbol.txt`, and `.private/shared-eval/guard-sweeps/inline-treasure-box/`.
