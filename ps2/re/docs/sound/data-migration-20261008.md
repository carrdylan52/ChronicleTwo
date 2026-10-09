# Sound data migration, October 8 night, round 4

Baseline: `9eb8f660`, pinned `chronicletwo_dev:sf-63f7a9e` image,
canonical MWCC flags and unchanged Satan's Fiddle profile. Existing MIDI,
CSL and bank layouts in `notes.md` supply the type evidence.

## Native data

All 26 retained string markers duplicate the diagnostic literals already
in the matched functions. Removing one function's markers at a time keeps
every instruction and resolved relocation exact. `iopMSINBuffAddr`,
`bgm_info`, `iop_bd_addr`, `bd_size_total`, `msinCtx`, `msinBfGrp`,
`msinBfCtx` and `gBank` now have documented typed definitions. Only
`iop_bd_addr` has external C++ linkage, as declared by sound.hpp.

The definitions use their actual sizes, including the 0x14 CSL context,
0x48 context array and 0x44 bank descriptor. The postprocessor preserves
their retail alignment gaps. The native `CSound::Init` local static and
its compiler-generated guard supply `load_m_flg_351` and `init_352`;
both reservation markers are removed together.

## Retained markers

- `msinBf` (0x1200): the natural `static MSIN_BUFFER[9]` definition
  compiles, but linking fails because generated `lib/sce/libdev.s` contains
  `.word msinBf + 0x141` and `.word msinBf + 0x40` at retail addresses
  0x00363F5C and 0x00363FE4. These words are inside a library byte table,
  not live sound-buffer references. Retail binds msinBf LOCAL. Correcting
  those inferred SDK references requires splitter/tooling work; exporting
  a retail-local variable or editing generated assembly is not retained.
- `midi_state` (0x1270 reservation, 0x1240 actual type): its typed static
  definition preserves PAL bytes, but the object checker rejects the
  terminal 0x30 reservation gap (`size 0x1240, retail 0x1270`; run ends
  0x003F6490 instead of 0x003F64C0). No filler fields enlarge MIDI_STATE.
- `D_003F3F6C` (4 bytes): removing this explicit padding reservation after
  native msinCtx changes the linked BSS layout. The current native-BSS
  padding path does not absorb the remaining explicit piece boundary.
  No fake object is introduced to occupy the gap.

Failed candidates are restored. Detailed receipts are
`sound-bss-msinBf-build.log`, `sound-bss-midi_state-objects.log` and
`sound-context-padding-build.log` under `.private/dataB-r4/`.

## Measurement and validation

Markers (RODATA / BSS): **26 / 13 → 0 / 3**.
Fresh objdiff `matched_data / total_data`: **0 / 10950 → 1414 / 10950**.

`sound-final-build.log` prints `SCES_511.90: OK`; `sound-final-objects.log`
passes 149/149 objects. These receipts, the incremental checks, baseline
measurements and object-hash audit are under `.private/dataB-r4/`.
All objects outside the seven owned units retain their baseline file hashes.
No function is promoted and no assembly fallback or guarded function body changes.
