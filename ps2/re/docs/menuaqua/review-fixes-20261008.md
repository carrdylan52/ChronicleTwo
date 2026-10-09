# October 8 night review fixes

## Retail function sizes

The header's `@size` values describe each function's declared retail symbol
size in `ps2/config/pal/main.symbols.txt`, excluding padding before the next
function. 81 padded tags are corrected. Every tagged symbol in this
header is present in that table, and the complete size audit has no mismatch.

The documentation changes preserve `SCES_511.90: OK` and 149/149 objects.
Receipts: `.private/fixes-r1b/receipts/header-sizes-{build,objects}.log`.

## Native switch tables and Step statics

Retail `at_5500` maps main-menu cursors 0 through 6 to
`.L0021CAC4`, `.L0021CC30`, `.L0021CB30`, `.L0021CB30`, `.L0021CB74`,
`.L0021CBA0`, and `.L0021CC04`. Those blocks register a fish name, open
the save menu, assign/delete a saved fish, withdraw entrants, display
tactics, and start a race. Commit `d5d1ca77` restores case labels to that
mapping while keeping the body order 0, 2/3, 4, 5, 6, 1. The prior native
labels placed withdrawal, tactics, race start and save at 1, 4, 5, 6;
the retail assembly table had hidden the differing native table.

The native table now resolves to retail's targets, so `at_5500` is
emitted entirely by C++. Step's `at_4299` and `at_4300` are also native
switch tables. Its existing local `sel_sift_fish` and
`sel_sift_fish_select` declarations naturally supply both BSS objects
and the first object's initialization guard; their separate markers
are unnecessary. All six removals preserve SCES_511.90 and 149/149
canonical objects.
