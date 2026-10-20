# Toan combat transplant, round 1

This opt-in static library compiles Chronicle's original `ps2/src/charakey.cpp`
inside `ixdc1`, directly from the checkout named by `IXION_DC1_SOURCE_DIR`.
It links into `ixion_c2` without adding DLL exports. It does not yet drive a
DC2 character, animation, or enemy collision primitive.

## Boundary and clock ownership

`port/include/ixion_dc1_toan.h` is valid C11 and carries fixed-width POD fields.
The module has one actor; call `ixion_dc1_toan_reset` before its first tick or
when retiring/replacing it. Supply the 41 `c01d` dungeon KEY ranges in their
original order, plus weapon stats, ailments and gauge. There is no baked asset
table in the library. Reset clears the combo, hit pool, frames, pad and status.

Each tick supplies the body's current absolute animation frame, world position,
weapon-tip position, yaw, world-space stick components, ground height and Cross
edge/held states. The host is responsible for suppressing input while paused,
dead, in an event, or otherwise locked. `cancel` takes precedence over Cross.

The output describes the handler before end-of-tick gauge/cooldown upkeep.
For a new motion or `restart`, restart that KEY in the host. Otherwise apply a
handler change to `time`. The initial `ToanKey_On` writes 136 even though the
following `ToanKey_Play` selects motion 36; the host's motion restart selects
820. The test covers this distinction and the authored 0.3-frame step.
The library does not advance animation or apply movement to the supplied body.
Apply `velocity.xz` once through DC2's normal collision path, scaled by the
room's donor-unit scale; DC2 still owns position, gravity and incoming damage.
The charged-jump donor velocity is exposed for later integration qualification.

`hits` contains raw DC1 records from this tick, exported before lifetime aging.
One swing can produce a record on every frame in its window. R3 must coalesce
by action/window, track weapon-tip position, and retire each DC2 primitive at
window end. The headless 0.5-frame fixture produces 33 records across five
windows, rather than five raw records. Monster effectiveness is currently
zero-initialized; its table and dynamic equipment/status updates need a host
adapter before gameplay qualification.

## Isolation

The library does not inherit `ct_options` or DC2's rewritten headers. It uses
DC1's `PORT/PAL`, forced `port.h`, raw game headers and Kazarov include path.
Standard/SCE declarations are parsed globally before importing game headers.
DC1's unused `dataalloc.hpp` also needs global parsing because C++ forbids its
placement `operator new` declarations inside a namespace; no allocator code
is linked. C-linkage globals are renamed. The seven used VU0 functions have
private C names and local scalar implementations. After SCE declarations,
`u_long` is explicitly 32-bit for the donor game types, checked at compile time.

The character/frame backing views use aligned host `sizeof` storage as scoped
by the spike. Only the handler's direct named-object/nonvirtual calls are
supported. No DC1 engine constructors or vtables are linked. Never hand these
views to general DC1 engine code. `helpers.json` records exact-byte helper
hashes, original blob IDs, source ranges and the donor commit.

`check_symbols.py` checks the archive for unprefixed strong definitions and
foreign game references. After DLL linking it compares donor definitions
against all game/host/plugin objects and both Kazarov archives, checks that
the Toan boundary/handlers remain, and requires the sole export
`ixion_get_api`. This gate is currently for Windows/COFF. The three negative
tests compile real objects and prove rejection of an unprefixed global, a
foreign game-function binding, and a prefixed donor/host collision.

## Focused development

Use an existing Release configuration with `IXION_BUILD_PLUGIN=ON`,
`IXION_API_DIR` pointing at Ixion's `include`, and `IXION_DC1_SOURCE_DIR`
pointing at Chronicle. Reuse local dependency sources for LAN-only setup.

```powershell
pwsh -File tools/windows/build.ps1 -Root <Windows-dependency-root> `
  -BuildDirectory port/build/toan-release -Targets ixdc1_toan_tests -Jobs 8
port/build/toan-release/ixdc1_toan_tests.exe
pwsh -File tools/windows/build.ps1 -Root <Windows-dependency-root> `
  -BuildDirectory port/build/toan-release -Targets ixion_c2 -Jobs 8
```

R2 supplies the converted skinned body, weapon, KEY table and loading seam.
R3 adds the DC2-only RunScript adapter, input masking, motion/velocity/yaw
application, and per-window collision primitives. The stubs in `shim.cpp`
cover sound, weapon trail, ambient animation, charged effects and durability;
they need event mappings or qualification in later rounds.

## Characters (Goro round 1)

`ixion_dc1_reset` selects the handler pair by `IxionDc1Config.character`
(`IXION_DC1_CHARA_TOAN` 0, `GORO` 2, `UNGAGA` 4), the same pairs that
`dun/gameloop.cpp:3674-3725` dispatches on `cur_chara`; `ixion_dc1_tick` runs
it. `ixion_dc1_toan_reset/tick` remain and always select Toan. Supply the
character's own KEY order (Goro: `c06b` base.cfg, 37 rows; his charge loop is
KEY 13 and the smash KEY 14). `ixion_dc1_character_info` returns the table row:
driven KEYs, combo KEYs, charge-loop and release KEYs, gauge rules, hit owner,
named points and the main effect's name/radius. Ungaga is selectable but untested;
his handler's static `wait` counter survives reset.

`getFramePos` asks the host through `ixion_dc1_set_point_resolver` (frame
body/weapon plus point name, e.g. Goro's `dcol0`). Unanswered names use
`weapon_tip`, which is Toan's R1 behaviour; the output counts queries and
fallbacks. Shots now carry the `Set` vector, owner, sub id, source, initial
phase and the character's effect name/radius; outputs also report `EndEffect`
and camera shake written this tick. The DLL keeps the boundary with
`--require-defined` (MinGW lld's `--undefined` is only "if available").
