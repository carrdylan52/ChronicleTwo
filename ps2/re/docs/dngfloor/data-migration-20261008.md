# Native dungeon floor data

The established unit notes describe the grid, room option flags, script tags,
challenge tables, search directions and special forest title. All functions
already match at checkpoint `96cbdc31`; no functions are promoted here.

The six script-state slots are typed, documented file-static definitions.
`tree_map_tag` holds its eleven callbacks and terminator; callbacks have their
retail local linkage. Room options use a local aggregate of the established
`MENU_SPI_ANALYZE_STRUCT1` records and `DNGMAP_ROOM_FLAG` values.

`_ROOM_TITLE` initializes its four-byte fallback as `char empty[4] = "err"`,
removing the inherited float copy/type-pun. m2c evidence is saved in
`.private/dataB-r3/dngfloor-room-title-m2c.log`; the initializer preserves
retail's float load/store lowering and all object bytes. The three script paths
are literals at use.

The three practice-condition tables retain their signed-byte/unsigned-halfword
layouts. Search tables use `GLID_DIR` values. `GetNextRoom` initializes a
four-by-three integer array directly, removing the unused quadword union and
anonymous external template. The floor-count initializer and practice switch
emit their data naturally. The special forest titles preserve the Japanese
Shift-JIS bytes through hexadecimal escapes.

## Retained boundary markers

Seven `INCLUDE_RODATA` markers remain; BSS has none:

- `at_886__4` and its four keyword pieces: the natural 40-byte room option
  aggregate emits a 48-byte compiler template. Retail's piece ends at 44 bytes,
  followed by the separately referenced `D_0036178C` boundary. Removing the
  marker fails with native extent `0x30`, retail `0x2C`, and overlapping pieces.
  Removing only keyword markers then fails their retained-table relocations.
  Natural aggregate source is present; no external template declaration remains.
- `offsetTable_911` and `D_0036178C`: native source defines the five integer
  texture bases `{0, 4, 8, 12, 16}` and indexes `[texture_group - 1]`. This keeps
  retail's instruction lowering. Removing the table marker loses the table
  because the current identity preparation binds the negative-addend reference
  to the preceding `D_0036178C` marker. It needs an addend-aware native identity
  for the actual table. The preceding word is alignment padding, so it is not
  replaced with an invented source variable.

Failures are recorded in `dngfloor-options-failure-check.log` and
`dngfloor-texture-offsets-failure-check.log` under `.private/dataB-r3/`.
No build scripts are changed. The source retains natural initializers and only
these required boundary markers. No zero-valued filler fields or fake objects
are introduced.

Markers: ROData **35 → 7**, BSS **6 → 0**.
Objdiff after refresh: matched_data **0 → 38** / total_data **836 → 836**.

Every accepted incremental step passes `SCES_511.90: OK` and 149/149 full object
checks including resolved relocations. Receipts are
`.private/dataB-r3/dngfloor-*-build.log` and `dngfloor-*-objects.log`;
`dngfloor-progress.log` records the required source-only refresh.
