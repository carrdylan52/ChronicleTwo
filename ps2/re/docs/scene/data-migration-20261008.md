# scene data migration (2026-10-08)

Baseline: `95f8fdd1`; 12 initialized-data
markers and 2 BSS markers. Every function already matches.
The pinned `chronicletwo_dev:sf-63f7a9e` image and full object/PAL checks
validate each accepted step. Public declarations remain compatible.

## Native initialized data

The six writable eight-byte `noname_*` arrays are file-local `char[8]`
definitions containing `"no_name"`, in retail order. Each array supplies the
default name for its corresponding camera, message, character, map, sky or
effect slot. Their distinct storage and writable interface are preserved.

`CRipple::Draw` inlines the two-byte Shift-JIS circle glyph with hexadecimal
escapes; `CRain::Step` inlines the `"hat"` frame name. `CScene::GetData`
already uses a native switch; its case labels now use the existing scene-kind
enum, preserving retail's effect-kind call to `GetSceneGameObj`. MWCC emits
the switch table and `CScene` vtable without their redundant markers.

The two sixteen-byte initialized templates are the existing `InScreenFunc`
local bounds arrays `{50, 50, 0, 0}` and `{-50, -50, 0, 0}`. Removing their
markers retains the native array copies and the function's scheduling profile.

Initialized-data markers fall from **12 to 0**; the two BSS markers remain.
Refreshed `matched_data` increases from **0/589** to **140/589**. All 97
functions remain matched; none is promoted. The recovered state passes PAL
and all 149 objects with unchanged unowned object hashes. Receipts:
`.private/dataD/resumed-scene-{build,objects,metrics}.log` and
`resumed-scene-{refresh,coverage}.log`, plus the individual
`scene-{vtable,data-switch,noname_*,ripple,hat,screen-bounds}` receipts.

## Remaining local-static BSS

`InScreenFunc` already contains the native `static CFuncPoint sun_func`.
Its 0x1C0-byte object and one-byte constructor guard are compiler-generated,
but the current literal binder only names anonymous `at_*` BSS objects.
Removing both markers leaves `sun_func_1223` and `init_1224` unresolved,
with twelve complete-object errors and no function instruction differences.
The canonical source retains both markers.

`.private/proposals/dataD-local-static-bss.patch` extends the existing
extent and relocation evidence to local BSS names. It also refuses to create
a second live object with an existing retail name. On private raw copies
compiled without the two markers, the proposed postprocessor passes the
complete scene object: 0x3794 initialized bytes and 349 resolved relocations.
The unchanged postprocessor fails on the same raw object. No shared tool or
profile file is edited. Receipts:
`.private/dataD/scene-native-bss-{raw-build,baseline-check,proposed-check}.log`.
