# Map-part data migration

The `830e48ed` baseline has four RODATA markers, no BSS markers, and
0/420 matched data bytes.

`CCharacter2::SetPosition(float,float,float)` initializes its position array
as `{0,0,0,1}`. This supplies the 16-byte `at_244` aggregate directly and
removes its anonymous extern and quadword type-punned initializer copy.
The native compiler already emits the 0xF8-byte treasure-box vtable;
removing its marker uses that table with its original methods and symbols.
No guarded or assembly-only function changes.

Two vtable markers remain:

- `__vt__17CList_9CObjAnime___DATA`: assembly-only `AssignFuncAnime`
  explicitly relocates against this list vtable. The current native source
  does not emit the table.
- `__vt__9CMapParts__DATA`: the current native source does not emit this
  class vtable. Its retail table remains required by constructors in other
  units; creating a synthetic constructor or hand-written table would not
  be a natural replacement.

Both accepted steps pass PAL and all 149 object comparisons. Only migrated
units change their object-file hashes from the warm baseline.
Receipts: `.private/dataF/mapparts-position-template-{build,objects}.log`,
`mapparts-treasure-vtable-{build,objects}.log`, and
`mapparts-final-{refresh,coverage,metrics}.log`.

Final markers are **2 RODATA / 0 BSS**; matched data is **16/420**.
The vtable section remains incomplete in source-only coverage, so the
native treasure-box table does not earn aggregate section credit.
No function is promoted and no shared-file proposal is made.
