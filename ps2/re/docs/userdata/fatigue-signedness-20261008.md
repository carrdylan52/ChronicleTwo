# Race fatigue signedness

`BREEDFISH_USED::fatigue` is an unsigned 16-bit race counter at fish offset
0x24, or offset 0x34 in `CGameDataUsed`. The fish record remains 0x5C bytes
and the enclosing owned-item record remains 0x6C bytes. The neighbouring
health, parameter and timer fields retain their offsets.

`sgInitGyoRace` increments the player's counter when the configured race
number is zero and the race is not an omake race. Retail 0x0030A584 loads
the counter with `lhu`; 0x0030A58C writes the increment with `sh`. Retail
0x0030A5A0 reads it again with `lhu` before the stamina calculation. The
penalty uses the integer expression `fatigue - 1`, then floating-point
conversion; unsigned storage does not make that integer expression unsigned.
The existing cast on the second read already expresses unsigned interpretation.

The remaining named field consumers only clear it: `CGameDataUsed::CopyDataFish`,
both inventory/aquarium paths in `AquaFishFatigueClear`, and the child's
initialization in `HaigouFish` in menuaqua. There is no signed read of this
field in those paths. `CFishAquarium::RefreshParam` increases the separate
`param[4]` field, not race fatigue: its `lhu`/`sh` at 0x0019BBF8/0x0019BC0C
access aquarium +0x2CA, equal to sub-tank +0x28C plus owned-item +0x3E.
Signed halfword accesses at owned-item offset 0x34 in weapon/status and
robot-part functions address other members of the owned-item union. Likewise,
the tournament's offset-0x34 read addresses a tournament record. Neither
contradicts the fish field's unsigned interpretation. The signed integer
fatigue in `AQUA_FISH` is a different battle-state counter.

The independent header-only trial on 93cbbea reproduces all 149 complete
game-object file hashes exactly. The complete checker remains 147/149,
failing only nd_meswin and actscript, and PAL differs only in the existing
0x26 text bytes. The canonical all-drafts comparison corrects precisely
the increment load in `sgInitGyoRace`: 432/1154 differing words become
431/1154. Its 0x1208 body still exceeds the 0x1200 reservation, so the guard
remains. No guard or source expression is changed by this correction.

A sweep of 21 affected guarded units, including headers included only under
draft guards, finds no other changed draft. gyoracesim's simulation records
receive integer parameters and do not read this owned-item field directly.

Evidence: `.private/shared-eval/m2c/sgInitGyoRace__FP11SubGameInfo.cpp`,
`RefreshParam__13CFishAquariumFv.cpp`, `fatigue-uses.txt`,
`.private/shared-eval/experiments/unsigned-fatigue/`, and
`.private/shared-eval/guard-sweeps/unsigned-fatigue/results.json`.
