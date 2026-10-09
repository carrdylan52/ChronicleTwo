# Compiler-native vtable producers

`native_vtables.py` declares a retail table's owner and its raw source-only
producer in `TABLE_PRODUCERS`. `object` receives `__vt__7CObject` from the
normal `map.cpp` objdiff compile. `ObjectLists.cmake` makes that raw producer
a dependency of the linked owner; the existing source-only rule tracks source,
headers, compiler and SF profile. Keep the dependency and manifest together
when adding a producer. No constructor or table initializer is synthesized.

A retained owner marker remains authoritative and excludes native comparison
credit. Once removed, a missing or stale producer fails the build. Inputs must
come from `objdiff/base`, never linked objects or prepared comparison copies;
the producer source and target section must contain no fallback for the table.

Import requires a unique global object occupying its entire native `.vtables`
section, exact declared retail size, valid address-compatible alignment,
allocated non-executable storage, no aliases, exact real R_MIPS_32 site/type
metadata, known native callbacks, and the complete resolved retail initializer.
Every incoming native consumer must match its complete retail function,
including all resolved relocations. Unknown or mismatching consumers reject
the donor. Recipient definitions and ambiguous callback names also reject it.

The importer copies only the verified compiler section and its relocation
links. Recipient code bytes, function metadata and serialized relocation indices
must remain unchanged. Normal data padding and section ordering follow import.
The comparison path uses the same verifier and includes raw donor contents,
producer source and the manifest implementation in cache fingerprints. A donor
change invalidates the prepared owner even when its own raw object is unchanged.

Run `python3 -m unittest discover -s scripts/build -p 'test_*.py'`. The native
table tests cover storage, extent, aliases, initializer bytes, relocation shape,
callbacks, all consumer instructions and calls, fallback provenance, freshness,
recipient code preservation, and donor-sensitive comparison caching. Whole PAL
and all-object checks remain required for accepting marker removals.
