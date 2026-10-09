# October 8 data migration

Baseline: `7c7edc2f`, MWCC 3.0/Satan's Fiddle through
`chronicletwo_dev:sf-63f7a9e`. The warm build verifies PAL OK and
149/149 canonical objects. Initial markers: 178 RODATA / 99 BSS.
Refreshed matched data: 4 / 8293 bytes.

## Naturally emitted templates, statics and vtables

Existing native C++ supplies the fish parameter zero templates, wall
vertices, aquarium debug format and value arrays, race debug arrays,
race tactics sentinel, drop counter and its initialization guard, race
menu local statics and guards, three switch tables, and the CAquaFish and
CFishFood vtables. The vtables retain their real sixty function pointers
and compiler-generated identity. The race debug byte array is binary
14-byte data, not a string. Local statics retain their native one-byte
initialization guards and actual object widths.

Ten separately validated steps remove 25 RODATA and ten BSS markers.
Markers: 153 / 89; matched data: 508 / 8293 bytes. Every
`aqua-emitted-<step>-build.log` / `-objects.log` receipt in
`.private/nminv-r2/` verifies PAL OK and 149/149 objects.

## Named zero storage

Eighty-six named reservations now have documented native definitions:
25 aquarium objects and 61 race-menu objects. They retain retail linkage,
existing pointer types, byte/halfword flags, arrays and save-state records.
Public GyoraceFish, GyoraceData, AquaDeadCheck and MenuDCMsg definitions
remain compatible with the existing header declarations. No constructor
objects or initialization order are added.

Actual object extents exclude reservation gaps: the six race selection
flags occupy six bytes, the three tank bubble pointers twelve bytes,
the six fish bubble/effect pointers twenty-four bytes, and MenuDCMsg's
nine pointers thirty-six bytes. Camera and position-table state are
pointers, not storage for the pointed-to objects. The four existing
native global constructor objects remain in place.

Markers: 153 / 3; matched data: 2128 / 8293 bytes.
`aqua-state-core-{build,objects}.log` and
`aqua-state-race-{build,objects}.log` verify PAL OK and 149/149 objects.
The three remaining BSS markers are CAquarium::Step's frozen local statics
and initialization guard; their body and retail references stay unchanged.

## Native literals

Sixteen separately validated groups replace 34 external literal references
with strings at their native uses: message/question formats, tank and fish
paths, fish motions, race paths and key messages, fish pack/image formats,
and reflection/screen texture names. Shift-JIS bytes use hex escapes.
The two fish path formats now have ordinary char-string arguments without
casting their former byte-array declarations.

The shared `info.cfg` and screen texture literals also pass with both
frozen bodies intact; generated native data retains their exact retail
identities for the assembly consumers. Draw's existing profile selectors
and resulting code remain unchanged.

Markers: 119 / 3; matched data: 2128 / 8293 bytes. Every
`aqua-string-<step>-{build,objects}.log` receipt verifies PAL OK and
149/149 objects.

## Native vector initializers

Seven four-float templates now come from their real local aggregates:
direction, fish brightness, bubble origin, two ambient colours and two
reflection axes. Five successful `aqua-template-<step>` receipt pairs
verify them individually. Draw's profile selectors are unchanged.

The declaration-site NextThink/Thinking probes change five/thirteen text
bytes respectively, and are reverted. The SDK-vector plus memcpy circling
probe changes 25 bytes at the final runtime vector copy, and is reverted.
Those logs are `aqua-template-{next-think,thinking-vectors,round-copy}-build.log`.
Markers at this checkpoint: 112 / 3; matched data: 2128 / 8293 bytes.
The subsequent full `invent-pointer-anchor-{build,objects}.log` also
verifies the restored aquarium source: PAL OK and 149/149 objects.

The circling direction seed also matches as a local sceVu0FVECTOR
initializer, removing its external template and seed-copy cast. The
existing runtime vector copy remains unchanged. `aqua-sdk-round-seed`
verifies PAL OK and 149/149 objects. SDK-array forms for NextThink and
Thinking retain the same five/thirteen-byte differences and are reverted.
Current markers: 111 / 3; matched data: 2128 / 8293 bytes.

## Fish and geometry tables

Fourteen initialized tables now have documented native definitions:
171 breeding combinations, ambient and directional lighting, three
collision-point arrays, nine food records plus their sentinel, bubble
origins/rise/wobble data, circling angles, fish effect durations, and
18 fish-image records plus their sentinel. All retain mutable LOCAL
storage and their exact retail dimensions.

The verified model mapping contains f01 through f08 and f10 through f19;
f09 is absent. f19 has item ID 310 and breeding code zero; the other
models have IDs 320 through 336 and codes ten through twenty-six. New
enums name these data identities without assigning unverified species names.

Natural type padding replaces unused byte members: food has one byte at
+7 and size ten; image records have padding at +2 and +10 and size twelve;
collision points use aligned sceVu0FVECTOR plus radius, with a natural
12-byte tail and size 32. ColChkPoint2 has nine real array slots and seven
active entries; its two unused slots are implicitly zero initialized.
Static assertions preserve all three sizes. All existing code, including
both frozen drafts, remains byte-identical.

Fourteen `aqua-table-<symbol>-{build,objects}.log` receipts verify PAL OK
and 149/149 objects. Eighteen image-string markers also come from the
native pointer targets. Markers: 79 / 3; matched data: 2128 / 8293 bytes.

## Menu and reflection tables

Eleven further tables now use native initializers: aquarium menu border
strips, seven parameter-gauge layouts, parameter labels, character-frame
rectangles, regional accept/cancel mappings, menu action IDs, four wall
origins and the title-frame strips. Each retains the retail element width,
array dimensions and mutable LOCAL linkage. The regional table uses the
existing PAD_CIRCLE and PAD_CROSS names.

Eleven `aqua-table-<symbol>-{build,objects}.log` receipts verify PAL OK
and 149/149 objects. Markers: 68 / 3; matched data: 2128 / 8293 bytes.

## Race script and drawing tables

Twelve race tables now use native definitions: the three-entry race tag
list, four-entry prize tag list, six saved-racer indices, six tactics
numbers, seven translated empty-slot labels, six tactics message IDs,
and six subgame texture-strip arrays. Tag lists retain real null
terminators and typed function pointers. Mitouroku remains externally
visible; both English slots still share their retail literal target.
Ten child-literal markers are also supplied by the native tables.

Each `aqua-table-<symbol>-{build,objects}.log` receipt verifies PAL OK
and 149/149 objects. Markers: 46 / 3; matched data: 2128 / 8293 bytes.

## Small aquarium tables

The final eighteen named definitions supply collision-point counts,
title coordinates, fish-stat icon rectangles and colours, parameter
layout offsets, the pending tank sentinel, tank frame sizes, menu layout
choices, tank navigation, per-mode menu limits and two race filenames.
The fish-stat offset list contains seven real bytes, rather than the old
eight-byte extern extent. The two filename literals also come from their
native pointer initializer.

All eighteen `aqua-table-<symbol>-{build,objects}.log` receipts verify
PAL OK and 149/149 objects. The complete 55-table migration has no failed
candidate. Markers: 26 / 3; matched data: 2260 / 8293 bytes.

## Retained markers

All 26 initialized markers and three BSS markers are listed below.
Both frozen bodies retain their exact checkpoint text and retail symbols.
The headers and INCLUDE_ASM function selections are unchanged.

| Marker | Reason |
|---|---|
| `at_1346__DATA` | NextThink vector seed; natural aggregate and SDK-array initializers change five text bytes and are reverted. |
| `at_2935__DATA` | Bubble-count template used only by frozen SettingAqua. |
| `at_2975__DATA` | Vector template used only by frozen SettingAqua. |
| `at_2976__DATA` | Vector template used only by frozen SettingAqua. |
| `at_3016__DATA` | Vector template used only by frozen SettingAqua. |
| `at_3290__DATA` | Thinking vector seed; natural aggregate and SDK-array initializers change thirteen text bytes and are reverted. |
| `at_3291__3__DATA` | Thinking vector seed; natural aggregate and SDK-array initializers change thirteen text bytes and are reverted. |
| `at_3310__DATA` | Thinking vector seed; natural aggregate and SDK-array initializers change thirteen text bytes and are reverted. |
| `at_3311__DATA` | Thinking vector seed; natural aggregate and SDK-array initializers change thirteen text bytes and are reverted. |
| `at_3150__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3151__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3152__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3153__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3154__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3155__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3156__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3157__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3158__2__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3159__2__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3160__2__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3161__2__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3163__2__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_3164__3__DATA` | Resource-name literal used only by frozen SettingAqua. |
| `at_4300__DATA` | Format literal used only by frozen CAquarium::Step. |
| `at_4299__DATA` | Format literal used only by frozen CAquarium::Step. |
| `at_5500__DATA` | GyoraceMenuKey switch table; native emission differs in four target words and cannot acquire the retail identity. |
| `sel_sift_fish_3638` (BSS) | Local static or its initialization guard used only by frozen CAquarium::Step. |
| `init_3639` (BSS) | Local static or its initialization guard used only by frozen CAquarium::Step. |
| `sel_sift_fish_select_3641` (BSS) | Local static or its initialization guard used only by frozen CAquarium::Step. |

The final race-table probe removes only at_5500's marker. Retail's
seven-word table has target offsets 0x334, 0x4A0, 0x3A0, 0x3A0, 0x3E4,
0x410 and 0x474; the native compiler emits 0x334, 0x3E4, 0x3A0, 0x3A0,
0x410, 0x474 and 0x4A0. Slots one, four, five and six differ. Its declared
extent is 28 bytes, with four alignment bytes in the assembly piece.
The canonical comparison cannot bind the differing native table to
at_5500; the full build fails and the object check reports 148/149.
The marker is restored without changing the function or profile.
Receipts: `aqua-final-race-jump-build.log`,
`aqua-final-race-jump-objects.log` and `aqua-jump-analysis.log`.

Final markers: **178 / 99 -> 26 / 3**. Refreshed matched data:
**4 / 8293 -> 2260 / 8293 bytes**. Coverage counts complete aggregate
sections, so retained pieces prevent credit for some native data.
No function is promoted, no profile row changes, and no foreign-file
proposal is needed. The final accepted source is verified by
`.private/nminv-r2/final-build.log` and `final-objects.log`.

Final validation uses the pinned image and original build tooling:
`final-build.log` verifies SCES_511.90 OK; `final-objects.log` reports
149/149 objects. `final-refresh.log` refreshes ctx/objdiff/progress before
`final-coverage.log` records 6779 matched, 84 guarded, nine assembly-only
and zero fuzzy functions. `final-audit.log` verifies frozen source and
headers, lane-only changes and the required commit author. Of 150 raw
object hashes, only the three owned units differ from the warm baseline;
all 147 unowned objects are unchanged. All receipts are under
`.private/nminv-r2/` in this lane's worktree.
