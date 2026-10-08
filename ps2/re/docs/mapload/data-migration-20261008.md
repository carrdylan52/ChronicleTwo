# Mapload data migration

## Baseline and validation

At `63f7a9e5`, all 118 mapload functions match. The unit has 90
`INCLUDE_RODATA` and 17 `INCLUDE_BSS` markers, with refreshed objdiff data
coverage of 888/3088 bytes. Earlier draft counts in `notes.md` describe an
older source state. Each accepted step passes the full 149-object comparison
with resolved relocations, leaves every unowned game object unchanged by
SHA-256, and produces `SCES_511.90: OK`.

## Script-loader storage

All 17 assembly reservations have types already established by the matched
code and existing headers. Retail-local state becomes file-static storage:

| Objects | Type | Purpose |
| --- | --- | --- |
| `mapFarDist` | `float` | Far clipping distance of the placed part. |
| `mapFarAlpha`, `mapShow` | `int` | Fade mode and visibility of the placed part. |
| `mapCameraInfoIdx`, `mapCameraRectIdx` | `int` | Current camera entry and next rectangle. |
| `mapFuncPointIdx` | `int` | Current parsed function-point index. |
| `mapNowFuncPoint` | `CFuncPoint *` | Function point being filled. |
| `mapAddMode` | `int` | Whether loading extends the current map. |
| `ReserveFuncFlag` | `int` | Whether configuration function-point storage is reserved. |
| `WaterIndex` | `int` | Next water-surface entry. |
| `cfgWater` | `CWaterFrame *` | Water frame being filled. |
| `mapPlacePartsName`, `mapMapPartsName` | `char[0x100]` | Instance and definition names of the placed part. |

Four native public definitions retain the existing declarations in
`mapload.hpp`: `char mapMapPartsGroupName[0x100]` and the three
`sceVu0FVECTOR` arrays `mapPos`, `mapRot`, and `mapScale`. The SDK vector type
provides the required 16-byte alignment. Existing native state stays in place;
all globals are ordered by their retail address within the storage groups and
have purpose comments. No layout or consumer declaration changes are needed.

The BSS step removes all 17 markers. The refreshed source-only objdiff data
metric remains 888/3088 bytes; that build omits final-object naming, ordering,
and padding fixups. Complete linked-object comparisons verify the migrated
storage and every resolved code reference.

## Script tag tables

The native tables use the existing documented `SPI_TAG_PARAM` type from
`scriptinterpreter.hpp`: a tag-name pointer and `int (*)(SPI_STACK *, int)`
callback, occupying eight bytes. `map_tag` is a local array of 88 records
(0x2C0 bytes); `cfg_tag` is a local array of 17 records (0x88 bytes). Both
terminate with `{NULL, NULL}`. Their definitions follow their callbacks and
precede the loader that installs them, avoiding extra forward declarations.
The tables stay mutable because that is the existing interpreter API.

All table ordinals and associations remain literal retail data. In particular:

- The first 35 map records are `{"d", mapDummy}`.
- `FUNC_FIRE_DATA` appears with both `mapFUNC_FIRE_DATA` and
  `mapFUNC_EFFECT_NAME`; the repeated spelling is preserved.
- Configuration `FUNC_DATA` appears with both `cfgFUNC_DATA` and
  `cfgFUNC_DATA_END`.

Sixty-three string objects belong to these initializers. Their literals pool
naturally when shared between records or tables. Uppercase tags remain
distinct from the lowercase selectors used by handlers. The cfg table's
additional eight assembly-piece bytes are zero alignment, not another row.
Removing the table and string markers leaves 25 RODATA markers and zero BSS
markers, with complete object and PAL validation passing. Refreshed source-only
objdiff data coverage remains 888/3088 bytes.

## Selector literals and sun template

Twenty-two ordinary string objects become literals at the existing `strcmp`
and `strncmp` uses. The affected handlers are `mapFIX_CAMERA_RECT`,
`mapFUNC_DATA`, `mapFUNC_EVENT_DATA`, `cfgFUNC_DATA`, `cfgFUNC_EVENT_DATA`,
and `cfgWATER_DRAW`. Shared `event` and `invent` selectors pool naturally;
markers and declarations remain until the last external use is replaced.
Each handler's change passes the full build and object checks independently.

The existing `CMap::GetSunPoint` local `sceVu0FVECTOR` initializer contains
`{0.0f, -1900.0f, 700.0f, 1.0f}`. It emits the 16-byte `at_438__2` template
itself, so that marker can be removed without changing the function or adding
an artifact global.

## Retained compiler vtables

The two remaining markers reserve compiler-generated `CList<CMapPiece>` and
`CList<CMapParts>` vtables. The existing placement constructions instantiate
both native tables automatically. Each real table is 12 bytes: two zero words
followed by an `Initialize` relocation at offset eight. Their retail addresses
are `0x37B608` and `0x37B618`, with callbacks at `0x1637D0` and `0x1631A0`.

Deleting both markers with the current tools produces exact native contents
and relocations, but the first table lacks its four-byte interior alignment
gap. The checker reports size 0xC versus a 0x10 retail piece, then identifies
the same gap before the second table. `postprocess_object.py::pad_data` omits
`.vtables` from its existing exact-declared-size and zero-padding policy.
Changing a native vtable's source contents to own padding would require
artificial scaffolding. Both markers therefore remain.

The private proposal `dataC-vtable-padding.patch` extends that policy to
`.vtables`. A private wrapper with the proposal passes the complete object
check: 0x4750 bytes, 945 relocations. Its terminal 12-byte section tail is
left to linker alignment. No instructions, special members, vtable writes,
or relocation fields need source changes. Tool-owner integration and full
PAL validation are required before deleting the remaining markers.

Final mapload markers are **2 RODATA / 0 BSS**, down from **90 / 17**.
Refreshed source-only objdiff data coverage remains **888/3088**. All 118
functions and the complete final object match; all 149 objects and the PAL
binary pass, and every unowned game object is unchanged.
