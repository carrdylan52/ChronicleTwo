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
