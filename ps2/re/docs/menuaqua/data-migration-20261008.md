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
