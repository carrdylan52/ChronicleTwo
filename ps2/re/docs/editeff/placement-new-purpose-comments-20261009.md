# Placement animation support declarations

`effect_idle` is the zero state of a placement-animation slot available for
reuse. `place_anime_count` is the three-slot count of the real `PlaceAnime`
array. These existing declarations support the accepted
`EditSetPlaceAnime__FiP9CMapParts` body described in
[its owning note](placement-new-night-20261009.md).

Their source declarations now have the required purpose comments. No value,
type, storage, function body or policy row changes. The accepted 34-caller
pn15 rebuild passes PAL verification and 149/149 complete object checks;
all 306 assembled and 149 source-only objects retain identical hashes, as
does the whole accepted-34 ELF. The shared
[source audit](../satansfiddle/placement-new-source-hygiene-20261009.md)
indexes the independent review and explicit identity receipts.
