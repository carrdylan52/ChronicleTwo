# Item-menu work-memory construction

`CMenuItemInfo::MenuModeMalloc` partitions the menu stack, creates its action
characters and movement/repair objects, initializes effects, then exposes the
remaining work memory. The existing complete type analysis remains applicable;
the guard is removed by hand.

The exact `menusys.cpp` row selects `MenuModeMalloc__13CMenuItemInfoFP9mgCMemory`,
`__nw__FUiP1`, `__ct__12CActionCharaFv`, `after_constructor_inline` and
`expected_matches: 2`. These are the one action-character loop expression
and the separate `SpectolFrame` expression. The loop's seven executions do
not increase the static count. The move-item/repair constructors and two
explicit effect allocations are outside the class-6 constructor identity.

Retail is GLOBAL/FUNC at 0x00245EC0, size 0x3B8 in extent 0x3C0. Its selected
clean draft reaches zero with the correct relocation map. The real constructor
chain retains its existing depth-8 pragma. The stack additions walk actual
`u_long128` elements; they are typed buffer boundaries rather than byte
access to object fields. The texture name `menueff0` is inline; its shared
alias/data remains for another user. The whole body contains no scalar
type-pun, identity helper or dummy lifetime variable. The shared-header exact
size correction is saved privately. Private receipts are the
`natural-menu-clean-literals/menusys.o` object, compiler log and `scores.json`,
plus `.private/pntc/receipts/menusys-literal-recovery.json`.

The canonical baseline has 127 differing words; the semantic policy also
restores constructor expansion scheduling where the residual is larger than
the allocator branch pair. All 164 other diagnostic rows equal the selected
control. Full pn14 integration passes `SCES_511.90: OK`, 149/149 complete
objects, and baseline hashes outside the 16 promoted units in both the full
306 assembled-object census and 149 source-only base objects. The linked
main/game bytes and loaded memory end match. Refreshed coverage is 6,773
matched / 89 guarded / 10 assembly-only / 0 fuzzy. Full receipts are
`.private/pntc/receipts/promote-twenty-four-build.log` and `.exit`,
`promote-twenty-four-objects.log`, `promote-twenty-four-artifacts.json`,
`promote-twenty-four-progress.log`, and `promote-twenty-four-coverage.log`.
The [toolchain proposal](../satansfiddle/placement-new-proposal-20261009.md)
states the intentional policy and its evidence limits.
