# Shared header proposals — October 8 midday

Evaluation base: `93cbbea` on `work/dc2-shared-midday`. Compiler:
MWCC 3.0-011126, canonical `-O3,p` flags and the checked-in Satan's Fiddle
profile, image `chronicletwo_dev:sf-d8bf13c`, JOBS=4. No upstream merge,
new profile row, replacement assembly or synthetic emission site is used.

The fresh base has 6741 matched, 119 guarded, 10 asm-only and two fuzzy
functions. The complete checker passes 147/149 units, with only nd_meswin
and actscript failing. PAL has exactly 0x26 differing text bytes, passes
the other nine file-backed sections, and ends memory at 0x1F64A00.
All 149 base game objects and the linked executable are copied before trials.

Each proposal is applied independently to this base, compiled through the
canonical wrapper, linked, checked against retail, and compared with all
149 base object hashes. ClsMes and GLID evaluations use complete private
source/header copies and rebuild all 149 game objects before linking with
the unchanged SDK/runtime objects. The GLID trial does not edit the
unowned dngfloor.hpp in the working source tree.

The draft sweeps include header references inside conditional draft code,
not only the ordinary build's dependency files. Both plain-MWCC
`draft_check.py` comparisons and canonical-profile comparisons are saved.
Word counts below are canonical all-drafts measurements, with relocated
operands masked and zero/excess words retained. They are not isolated
promotion proofs. A size overrun changes the comparison denominator.

## Decisions

| Proposal | Decision | Whole-file object hash changes | Complete objects | Draft result |
|---|---|---|---|---|
| `mg_drawenv-fog-color-array.patch` | Reject; editor remains noncompliant | 1, sceneload `.strtab` names only; no loaded change | 147/149 | `LightingEdit` stays 0/1356 with raw address arithmetic; no changed draft in 38 affected units |
| `inline-map-treasure-box.patch` | Evaluate only; paired header/source patch saved | 1, map constructor binding only; no loaded change | 147/149 | Standalone ctor remains exact and emitted; `EditInit` worsens 1226/1776 → 1248/1783, body 0x1BDC over 0x1BC0; other 24 affected units unchanged |
| `rectangle-value-assignment.patch` | Reject | 3: screeneffect, menudraw, menumap, all loaded changes | 144/149 | `sgLoopGyoRace` worsens 490/1676 → 1189/1680; `DrawRoot` 754/844 → 790/844; `MenuFormDrawNormal` 13/1020 → 778/1034; no improvement among 31 affected units |
| `unsigned-race-fish-fatigue.patch` | Retain unsigned field and audited purpose | 0; all 149 file hashes identical | 147/149 | `sgInitGyoRace` improves 432/1154 → 431/1154; body remains 0x1208 over 0x1200; other 20 affected units unchanged |
| `menusys-menu-key-rect.patch` | Evaluate only; not retained | 0; all 149 file hashes identical | 147/149 | `MenuMainInit` worsens 792/948 → 816/948, body 0xECC within 0xED0; other 12 affected units unchanged |
| `draft-check-zero-disassembly.patch` | Retain diagnostic fix | 0; all 149 file hashes identical | 147/149 | Race-display diff restores 17 omitted zero rows: 1155 → 1172 words, with ten differences in both displays; raw draft scores unchanged |
| ClsMes out-of-line `Init` | Evaluate only; reject global adoption | 5: dngmenu, mainloop, sysmes, menuaqua, menumain, all loaded changes | 143/149 | `Init` itself matches 0/176 and dngmenu passes; `DngTreeMapInit` improves oversized 374/388 → 247/256; four existing callers break |
| `GLID_INFO::blink` as s8 and remove DrawTreeMap cast | Evaluate only; ready patch | 0; all 149 file hashes identical | 147/149 | Existing `DrawTreeMap` remains exact; no changed draft among 23 affected units |

The unchanged-verifier trials retain the baseline 0x26 text bytes and every
other section/extent pass. Rectangle assignment regresses PAL text to
0xDC35C differing bytes, fails eight file-backed sections, and moves memory
end to 0x1F64A40. ClsMes outlining regresses text to 0x165A54 differing
bytes, fails all ten file-backed sections and main length, and moves memory
end to 0x1F63000. No proposal produces an eligible function promotion.

## Retail/type evidence

- Fog remains 0x30 bytes, with unsigned RGB bytes at 8..10. The proposal's
  extra union leaves the invalid editor address expression untouched and
  supplies no newly matching typed access. sceneload's only section-data
  difference is nonallocated `.strtab`: unused `@211`/`at_211` become
  `@213`/`at_213`. See [editdebug's evaluation](../editdebug/shared-fog-evaluation-20261008.md).
- The 0x680 treasure-box class has a real inline constructor chain and an
  independently required standalone callback at 0x161970. Retail binding
  is 13; the proposal emits binding 13, converted to weak 2 by normal
  postprocessing. The former global binding is 1. Loaded bytes and
  relocations are identical. See [mapparts' evaluation](../mapparts/shared-constructor-evaluation-20261008.md).
- There is no retail standalone mgRect assignment symbol establishing a
  by-value user-written operator. The proposed special member changes
  already matching code and cannot be accepted from a temporary copy in
  one caller. See [the assignment evaluation](../mg_tanime/shared-assignment-evaluation-20261008.md).
- The fish counter uses `lhu` both when incremented at 0x30A584 and when
  read for stamina at 0x30A5A0. All other named consumers clear it; signed
  union/other-class reads at the same numeric offsets are different fields.
  Record sizes/offsets are unchanged. The aquarium increases battle strength,
  not this fatigue counter. See [userdata's audit](../userdata/fatigue-signedness-20261008.md).
- `CMenuKeyFunc` constructs its rectangle at zero before the owned-item
  member and its body initialization. The explicit four-argument member
  initializer is semantically plausible, but its measured guarded caller
  is worse and it gives no promotion.
- Retail `ClsMes::Init` at 0x1F38E0 has binding 13 and a 0x2B8 body in a
  0x2C0 reservation. The proposed ordinary definition has global binding 1.
  The native body is exact; removing its shared inline definition changes
  mainloop, sysmes, menuaqua and menumain, while dngmenu itself passes.
  Among 16 guarded units, `EditInit` worsens 1226 → 1260,
  `sgInitGyoRace` 432 → 1046, and `MenuMainInit` 792 → 886.
  The smaller `DngTreeMapInit` still differs by 247/256 words.
- GLID_INFO is 0x70 bytes with blink at 0x1C. `DrawTreeMap` at 0x1EF254
  uses `lb`; the two dngfloor consumers clear it and `InitEnd` writes one.
  The only named read is the already signed DrawTreeMap cast. Changing
  the member to s8 removes that cast with identical code, layout, symbols
  and relocations in every object.

## Private proposals and receipts

Evaluation-only patches and their dispositions are in
`.private/proposals-out/README.md`:
`inline-map-treasure-box.patch`, `menusys-menu-key-rect.patch`,
`clsmes-init-out-of-line.patch`, and `glid-info-signed-blink.patch`.
They are not staged. map, menusys, nd_meswin, dngmenu, editloop and dngfloor
working sources contain no retained evaluation edits.

- `.private/shared-eval/base/`: build, full object check, coverage, object
  hashes/copies, linked image and base draft measurements.
- `.private/shared-eval/experiments/<proposal>/`: applied patch, build/link
  log, object check, PAL verifier, all-object hashes, metadata differences,
  changed object copies and target draft measurements.
- `.private/shared-eval/guard-sweeps/<proposal>/results.json`: per-function
  before/after metrics for every affected guarded unit. Both compiler modes
  have successful comparisons; no unit fails to compile.
- `.private/shared-eval/m2c/`: decompile.sh/m2c outputs and stderr, retail
  symbol bindings, named fish/blink consumer inventories.
- `.private/shared-eval/layout.log`: MWCC compile-time record-size and field
  offset checks, including fish fatigue at 0x24 and blink at 0x1C.
- `.private/shared-eval/experiments/final/`: combined retained-change build,
  full object check, verifier, hashes and affected draft comparisons.

## Retained state

Only the unsigned fish counter and objdump `-z` fix are retained in code.
No guard, generated member, inline assembly, profile row or gitlink is added.
The final combined build retains all 149 object file hashes and the complete
PAL ELF exactly; its SHA-256 is
`06ed9457824db7b84cbef38674573a0080d13d457d217eea6247f26e9d8a4378`.
The checker stays 147/149, PAL stays at the sole 0x26 text-byte discrepancy,
and memory ends at 0x1F64A00. The branch has no promoted function. Coverage
remains 6741 matched / 119 guarded / 10 asm-only / two fuzzy. Structured
receipts include `experiments/final/validation-summary.json` and
`experiments/final/coverage.txt` under `.private/shared-eval/`.
