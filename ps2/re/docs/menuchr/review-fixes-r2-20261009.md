# October 9 round-two review fixes

## Memory and read-record names (finding 12)

`MenuMemoryAdjust` writes the stack label through `mgCMemory::name`.
`MenuItemChrLoad` constructs the read-record label through
`MENU_BGREAD_INFO2::name`. Both buffers begin at offset zero; the typed
member expressions retain the existing loads, stores and string calls.

The `tbl_992` character/phase table stays flat. The reviewed two-dimensional
form changes three complete-object checks in `ConvertCharaLoadDataPhase`,
including its address calculations and relocated destinations.
`MenuItemChrLoad` retains `(char *) &info->path` at its two path uses:
replacing it with `info->path` changes six instruction bytes at function
offset `+0xAC` and fails one complete-object check. These are the P15/P15b
negative probes, separate from the matching name-member substitutions.

The typed name-member substitutions pass `SCES_511.90: OK` and all 149
complete-object checks. Neither protected method's guarded block changes.
