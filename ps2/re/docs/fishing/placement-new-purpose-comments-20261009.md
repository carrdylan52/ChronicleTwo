# Fishing mode declaration

The existing anonymous enum names fishing modes selected by float bait
(`kFishingModeFloat = 1`) and lures (`kFishingModeLure = 2`). The accepted
restart/loading callers use these mode values as described in
[their owning note](placement-new-night-20261009.md).

The enum now has the required purpose comment. No enumerator, type,
function body or policy row changes. The accepted 34-caller pn15 rebuild
passes PAL verification and 149/149 complete object checks; all 306
assembled and 149 source-only objects retain identical hashes, as does
the whole accepted-34 ELF. The shared
[source audit](../satansfiddle/placement-new-source-hygiene-20261009.md)
indexes the independent review and explicit identity receipts.
