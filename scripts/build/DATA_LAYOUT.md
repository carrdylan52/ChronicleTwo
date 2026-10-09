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
