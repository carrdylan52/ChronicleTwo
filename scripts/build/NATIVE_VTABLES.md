# Compiler-native vtable producers

`native_vtables.py` can import a retail table's compiler-native definition
from a raw source-only producer named in `TABLE_PRODUCERS`. The manifest is
empty, and the importer does nothing.

MWCC emits a class's vtable in the translation unit that defines its first
non-inline virtual function. When a table's retail owner does not emit it,
look for that function first: retail binds inline members weak (binding 13)
in the units that emit their copies. `CObject` and `CMapParts` declare
`Draw`/`DrawDirect` inline, as retail's weak bindings in map show, so
`object.cpp` and `mapparts.cpp` emit their own tables.
