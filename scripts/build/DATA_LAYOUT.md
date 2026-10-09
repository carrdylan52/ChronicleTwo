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

Anonymous native BSS may also be referenced by a file-local function whose
retail name has a duplicate suffix. An unambiguous in-unit function row can
anchor its incoming references only when that entire native consumer matches
retail, including every instruction, declared extent and resolved relocation.
No function or code relocation is renamed. A second identity pass after
literal mapping allows the complete proof to use those established literal
addresses. The suffix path handles anonymous numeric compiler templates only;
named statics retain their existing identity pass and string-table ordering.
Missing, ambiguous or incomplete consumers supply no identity.
`test_bss_consumers.py` covers late literal resolution and rejects unmatched
calls, body bytes, extents, aliases, rows and additional unknown consumers.

## Data extents and alignment

Retail symbol sizes describe objects, while the split section pieces include
the alignment gap before the next symbol or referenced address. Once data
sections are assigned alignment one for linking, their bytes must retain that
gap. The postprocessor extends a correctly sized native initialized object by
fewer than 16 bytes to its piece boundary; initialized padding must be zero
in retail. An exactly sized native NOBITS object reserves storage only when
its canonical piece ends at a power-of-two alignment of at most 4,096 bytes.
A reference cut may divide an alignment gap before that end. For an unaligned
cut, the proof follows contiguous canonical `D_<address>` fragments with no
retail declaration to the next declared object and checks that object's start
against the same alignment bound. Only storage through the original cut is
reserved; the fragments remain separate. Missing, malformed, declared or
unaligned gap evidence supplies no additional reservation proof. Terminal
detection uses the retail section kind, so a compiler section name cannot
bypass the declared terminal extent.
Anonymous initialized literal naming requires both the original payload and
symbol extent to equal the declared retail size. Appended bytes must be
complete zero retail bytes with no relocation fields. The linked literal pass
can retain a larger verified terminal zero tail; comparison preparation trims
it at the linker's `contents_end`. Internal initialized gaps stay below 16 bytes.
An object with a size different from its declared retail size is not padded.
The same policy covers compiler-generated vtables; their final section tail
belongs to linker alignment.
A terminal datum retains its declared extent when its end equals the generated
linker script’s `contents_end`. The checker accepts larger linker-owned tails
only with no retail relocations and complete zero initialized bytes.
Literal identity uses declared objects; padding additionally uses the canonical
reference boundaries, so an alignment tail cannot swallow a separately
referenced word. Referenced interior addresses and explicit `D_<address>` source identifiers
remain separate piece boundaries. Every game C++ source is a split prerequisite,
so adding or removing an identifier refreshes other units' cuts before linking.
A negative-addend table access cannot bind the table to a placeholder for the
preceding word. Its original addend remains intact, and an established native
name must agree with the inferred retail base. If the table itself has a
retained placeholder, that base may supply it while preserving the negative
addend. Binding only repoints equal-offset symbols; native instruction fields
remain unchanged.
Every incoming reference must infer the same exact placeholder base, with the
retail relocation kind and non-immediate operands. Initialized copies require
resolved native bytes and the complete real relocation shape to equal retail;
NOBITS copies require the exact declared extent. Invalid copies remain live.
A rejected parent also prevents discarding its dependent child.

Native BSS templates, local statics and their guards need an exact declared
extent and agreement from every live incoming code reference. Each reference
must match the retail relocation kind and instruction operands outside the
immediate. HI16/LO16 pairs follow ELF relocation order, which can differ from
instruction order; orphan pairs, unknown consumers, out-of-object addends and
competing live definitions reject naming. The literal loop also leaves a
candidate untouched when another live symbol already owns the chosen name.
Zero contents and compiler counters alone establish no identity. Named local pointer tables can establish literal
identities when their exact declared extent, all code consumers, nonpointer
bytes, real relocation shape and native target bytes agree with retail. Their
validated identities are available before names are written in native symbol
order. Ambiguous initialized literals can then be named through real
R_MIPS_32 pointers in native data, subtracting the compiled addend and
target-symbol offset; conflicting references reject the binding. Anonymous
initialized templates retain the existing literal matcher and naming order.
BSS and pointer-table naming currently uses reference and byte evidence without
a source-family gate; map's `points` versus retail's `ft_1248` needs the source
owner's naming correction before strict family equality can be required.
Named initialized local tables without pointers additionally require the same
source base name, exact declared size and section kind, and exact retail bytes.
Locals sharing a consumer are inferred together; a rejected peer invalidates
every remaining claim that depends on it. Every live consumer must be a complete
retail function with the declared extent,
all instruction operands and all resolved relocation targets matching. Unknown
consumers, changed calls, duplicate relocation sites and orphan HI16/LO16 groups
reject naming. This pass changes only the data identity; existing bounded padding
supplies its alignment gap afterward.
Equal declared initialized extents distinguish a literal from a larger object's
byte prefix; established code or native-data destinations still reject competing
identities. Discarding a fallback parent removes a compiler-owned child only when
that child's retail storage also has a retained placeholder. Native children
remain available for naming and comparison.

VU instructions and initialized game or library words that resemble addresses
remain numeric when retail has no relocation. The splitter checks their emitted
byte comments against retail before replacing an inferred expression; real
relocations remain intact.

A terminal function may end before the next unit's address when the generated
linker script supplies the intervening alignment. The canonical checker permits
this only at the exact `contents_end` established by the script and only for an
all-zero retail tail. Objdiff target symbol metadata records declared retail
function sizes so the same linker padding is excluded from function scores.

Objdiff uses separate comparison copies of the raw source-only and reference
objects. Data references come from retail relocation metadata, never splat's
address guesses; switch-table pointers use their enclosing function and interior
addend. Raw compiler `@NNN` objects are kept distinct from explicit source `at_NNN`
identities before either linked or comparison naming. Anonymous names are
established by bytes and real references, never by a coincident compiler number.
Native pieces retain verified internal padding. A native comparison tail is
trimmed only when the original compiler symbol size equals the declared retail
size and every removed byte is zero with no relocation fields; oversized native
objects remain visible. Both sides exclude the verified terminal tails owned by
the linker. BSS symbol extents include
that verified piece padding consistently with initialized objects.

Reservation arrays and every retained data-marker piece are excluded from the
source comparison, including coincidental compiler copies. No fallback payload
is imported. Function bytes, declared sizes and relocation fields remain intact;
function names use the existing template/initializer projection. CMake tracks
both raw sides, sources, retail metadata and preparation tools; comparison copies
and receipts are declared byproducts. Linked C++ objects also track the
postprocessor's shared data proofs in `objdiff_data.py` and linker extents in
`lcf.py`. Missing raw inputs fail, and a failed refresh removes the stale
configuration. Cache fingerprints include the ELF
reader, length-delimited proof inputs, global cuts, raw objects and source
provenance. A malformed receipt or mismatched output hash requires preparation
from the raw inputs.

`objdiff.json` explicitly sets `combineDataSections: true`. The pinned CLI
combines automap's 297 raw `.rodata` sections into one with this option, versus
297 with it disabled. In a raw self-comparison, changing one `.rodata` byte
removes all 2,468 bytes of that combined section from data credit while code
credit stays unchanged. `matched_data` credits a complete combined data section
only when native bytes, extents and relocations match retail exactly. It is a
lower bound on migrated native data: an exact typed object receives no section
credit while a reservation or unmapped piece leaves the same aggregate section
incomplete. Removing the reservation preserves
the native object but cannot restore credit until the section is complete.
This metric is independent of executable matching.
