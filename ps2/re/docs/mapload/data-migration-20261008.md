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
