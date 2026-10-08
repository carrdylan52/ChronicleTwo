# Native data migration — 2026-10-08

## Monster definitions

`base_monster_define` is native mutable `BASE_MONSTER_TBL[344]` storage, matching
its documented 0xB8-byte layout and global binding. Every record is initialized
by its typed fields in retail order. All three fixed-size string arrays in each
record have zero tails, and all nine implicit padding bytes per record are zero.
The weapon-wear floats are finite and reproduced exactly as binary32 literals.
The final record is `{-1}`: every byte after its ID is zero and its empty name
terminates lookups. Mutation by the language loader requires mutable storage.

MWCC initializes an anonymous-union member directly with the member array’s
braces; another enclosing union brace rejects subsequent initializers. The
drop-item arrays use the accepted natural aggregate form. The header layout
and public declarations remain unchanged.

This step removes the main table marker, reducing initialized-data markers from
43 to 42. Refreshed objdiff still reports matched data 0/64936 (the field is absent
when zero). Native source coverage and objdiff’s data-symbol credit differ here;
resolved object comparison and the PAL verifier remain the acceptance checks.

Receipts:

- `.private/dataA-r2/monster-base-table.log`: whole unit passes, 0x167F8 bytes
  and 587 relocations.
- `.private/dataA-r2/monster-base-build.log`: `SCES_511.90: OK`, 6763 perfect,
  zero fuzzy, 109 assembly, zero unmatched.
- `.private/dataA-r2/monster-base-objects.log`: 149/149 units pass.
