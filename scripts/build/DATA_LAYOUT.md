# Verified native data layout

The splitter uses retail relocation metadata for initialized data references.
A `D_<address>` expression in assembly data owns a boundary only when its word
has a real R_MIPS_32 relocation and its complete emitted byte comment agrees
with retail. Unrelocated numeric words that splat interprets as addresses do
not introduce boundaries. Code references and explicit source identifiers
still own their existing cuts. Unsupported data expressions, wrong relocation
kinds, unaligned sites and conflicting bytes reject reference inference.

This rule also applies to splat's `data/` fragments, including fragments of
game units whose linked data comes from C++. Those fragments must not create
phantom alignment pieces from packed numeric contents. For example, the
unrelocated word `0x003F3F6C` at `0x00361600` in memcard data does not point
into sound's BSS. Once its explicit padding marker is removed, the correctly
sized native MIDI context owns its complete alignment tail.

`test_data_references.py` checks positive pointer evidence, unrelocated words,
complete byte verification, unsupported expressions, code references and source
identities. Initialized-byte emission is separately covered by
`test_raw_data_words.py`. Canonical object checks and whole PAL verification
remain necessary; layout normalization must preserve code and every resolved
relocation.

The native-data pipeline also preserves independently referenced cuts inside a
compiler-required alignment gap. It snapshots the original whole-object sizes
and section alignments before identity mapping can append padding. Both
neighboring native objects must have unique, exact declared retail extents,
compatible section kinds and valid alignments. Rounding the first object's end
to the second object's original alignment must give its exact retail start.
Only a gap smaller than 16 bytes with no declared object, relocation field or
live competing definition can supply fragments. Initialized gaps must have
complete zero retail bytes and zero existing native prefix padding; BSS gaps
must be NOBITS. Every cut must be the contiguous canonical `D_<address>` label.

Such fragments contain only the zero storage the native alignment requires.
Their symbols are address labels rather than C++ objects. Existing undefined
labels keep their symbol indices, preserving every serialized code relocation.
No terminal reservation, missing object contents or nonzero initializer is
supplied. Held source markers remain authoritative and receive no native data
credit. `test_data_padding.py` covers positive initialized/BSS and multiple-cut
layouts, original extent evidence, unchanged code and rejection of incomplete,
aliased, relocated, nonzero, misaligned or marker-held candidates.

Source-only comparison keeps each function's original identity and serialized
code relocations. When the existing local-name mapper identifies a duplicate
retail suffix, an initialized callback table may instead reference an undefined
canonical alias. This requires a uniquely named file-local native function,
one in-unit retail function row, and a complete function-byte/relocation match.
The table must be a uniquely owned native object with the original exact
declared extent. Each changed pointer needs a real R_MIPS_32 site, zero addend
and the exact retail callback address. Only data relocations change; aliases
append without moving existing symbol indices. Unknown, ambiguous, incomplete,
interior, competing or marker-held references retain their original form.
`test_data_callbacks.py` checks this identity path and code preservation.
