# Native object frame vtable

The existing unit notes establish the base `CObject` and derived `CObjectFrame`
virtual layouts. The already matched `CObjectFrame` methods now emit its vtable
without an assembly marker. No function body or shared type changes.

The base `CObject` marker remains: deleting it fails to link because the native
object does not emit `__vt__7CObject`, and constructors/copies in many other
units require it. Changing shared virtual declarations or introducing a dummy
object solely to force vtable emission is outside this unit's migration scope.
Receipt: `.private/dataB-r3/object-vtable-build.log`. The existing frame vtable
is emitted naturally; no manual vtable writes are introduced.


Markers: ROData **2 → 1**, BSS **0 → 0**.
Objdiff after the required refresh: matched_data **0 → 0** / total_data **244 → 244**.

Validation: `SCES_511.90: OK`, **149/149** complete objects including resolved relocations. No functions promoted. Receipts: `.private/dataB-r3/object-final-build.log`, `object-final-objects.log`, `object-progress.log`.

## Native base-table producer and build dependency

The source-only SF compile of `map.cpp` already emits `__vt__7CObject` as a
global object of size **0x74** in an allocated, non-executable `.vtables`
section. Its complete initializer and every relocation resolve to retail's
table. The linked `map` object intentionally discards that external table;
the retail owner remains `object`. Deleting the marker from `object.cpp`
alone therefore still leaves the definition absent from the link.

A private transfer of this compiler-produced table into the marker-free
`object` object passes its complete canonical check (**0x7F0** bytes and
**71** relocations). The recipient's code snapshot is unchanged. The proof
requires a unique whole-section native object, its exact declared extent,
matching section properties, identical relocation sites and kinds, and
complete resolved initializer bytes. It imports no function bytes or
assembly-provided data. This establishes a viable native producer without
changing the class declarations or forcing emission with extra game code.

An integrated transfer needs the raw source-only donor to precede the linked
recipient in the build graph. It also needs donor-aware comparison cache
inputs, native provenance checks, and rejection of missing, ambiguous or
mismatching donors. The CMake scheduling change is outside this lane's
ownership; its prerequisite diff is in
`.private/proposals/dtool-r4-native-vtable-dependency.patch`. The accompanying
proposal records the remaining importer and comparison requirements. The
marker is retained until that complete path exists, so assembly data earns
no additional source-only credit here.

Receipts: `.private/dtool-r4/vtable-transfer-proof.py`,
`.private/dtool-r4/vtable-transfer-objects.log`, and
`.private/dtool-r4/verify-only-pal.log`. The combined private PAL link includes
the verified donor transfer and the dataE/dataF candidates and reports
`SCES_511.90: OK`. Accepted source markers remain **1 ROData / 0 BSS** and
matched_data remains **0 / 244**.


## Round-5 verified native base table

The general native-table importer and its CMake dependency now replace the
base-table marker. The raw source-only `map.cpp` producer emits the exact
116-byte `__vt__7CObject` table. Both native constructor consumers match
their complete retail functions and resolved relocations. The importer
checks the declared extent, whole-section storage, alignment, callback
identities, real relocation metadata and complete initializer before
copying only data. Retained markers remain authoritative; no table
initializer, constructor, or special member is written by hand.

Linked and comparison objects use the same verified donor. Producer bytes,
source provenance and the manifest participate in comparison cache inputs.
Every recipient code byte and serialized code relocation is preserved.

Markers: RODATA **1 → 0**, BSS **0 → 0**. Refreshed matched data:
**0 → 244 / 244**. Only `object.cpp.o` changes relative to the preceding
accepted step. PAL is `SCES_511.90: OK`, all **149/149** objects pass, and
code metrics remain **6,780 functions / 1,854,796 bytes**. No function is
promoted. Receipts: `.private/dtool-r5/object-{build,objects,tests,metrics}.log`.
The pre-fix failure is `.private/dtool-r5/vtable-before.log`; cache regression
receipts are `vtable-tests-{before,after}.log` in the same directory.
