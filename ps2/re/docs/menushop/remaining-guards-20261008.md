# Native quest-view drawing and remaining initializer guards

The current Satan's Fiddle row for `menushop.cpp` uses GPR helper mask `0x30`
and FPR mask `0`. No floating-point expression override or header change is
needed for the drawing function.

## MenuNPCQuestViewDraw

`MenuNPCQuestViewDraw__Fv` draws the quest/scoop memo backing, list fonts,
completion and photograph marks, cursor and scrollbar, optional comments,
and debug record overlay. Its existing natural C++ draft matches all 924
instruction words under the pinned profile, with the retail `0xE70` extent.
Plain-wibo draft checking reports four differences; the canonical wrapper's
compiler state is authoritative.

Removing only this function's guard and retaining both initializer fallbacks
passes canonical compilation, section fixup, and the complete object checker:
`0x5A68` allocated bytes, 1,304 relocations, zero byte or resolved-relocation
differences. The drawing function is supplied by native C++.

## MenuShopInit and MenuNPCQuestViewInit

`MenuShopInit__FP9mgCMemoryPii` constructs the shop menu and records, initializes
messages and common forms, and loads the shop pack. Its unchanged current SF
draft has 281 of 312 words differing and emits `0x4DC` bytes against retail's
padded `0x4E0` extent. The first gap is at offset `0x58`: retail tests `v0`
after placement allocation and copies it to `s1` in the delay slot, while
native construction copies first and tests `s1`.

`MenuNPCQuestViewInit__FP9mgCMemoryPii` selects quest/scoop mode, constructs its
menu and quest manager, attaches save-data records, and sets texture blocks.
Its unchanged SF draft has 46 of 76 words differing and emits `0x128` bytes
against retail's padded `0x130` extent. The first null-branch gap is at offset
`0x70`. Independent canonical object measurements confirm both gaps; the
instruction counts include downstream shifts, and supersede older percentages
recorded under different compiler state.

Both initializers remain guarded under the established
[placement-new park](../funcpoint/placement-new.md). No new constructor
experiment is justified in this lane. Reconsider after the dedicated
compiler/constructor investigation validates natural lowering of the returned
allocation pointer, then remeasure all subsequent differences rather than
assuming every shifted instruction is fixed by that first branch.
