# Remaining main-menu guards

The current Satan's Fiddle translation-unit row uses GPR helper mask `0x30`
and FPR mask `0`. The existing `DrawMenuTopic` selector remains unchanged.
Both guarded functions retain their assembly fallbacks.

## MenuInternSelectDraw float-order calibration

`MenuInternSelectDraw__Fv` draws the active forms and message, topic ticker,
and optional debug status overlays. Its existing natural draft has the retail
`0x1C0` extent. With the current deterministic profile, 10 of 112 words differ:
six literal-materialization words at offsets `0x5C` through `0x84` in the first
`DrawMenuFillBox`, and four at `0x164` through `0x174` in the second.

Two unscoped expression selectors restore retail: translation unit
`menumain.cpp`, enclosing function `MenuInternSelectDraw__Fv`, value type
`binary32`, values `0x42A00000` (80.0f) and `0x43AF0000` (350.0f), each with
`evaluate_first: true`. The first is the height argument of the upper debug
box; the second is the y coordinate of the help box. No source rewrite is
needed. Each identity is consumed by the pinned compiler without stale-row
errors.

With both selectors and only this guard manually removed in a private source
copy, the canonical wrapper, section fixup, and complete object checker pass:
`0x4F98` allocated bytes, 1,396 relocations, zero byte or resolved-relocation
differences. The calibration rows are proposed for the profile-owning lane;
the shared profile is not changed here. Reconsider native promotion when those
rows are integrated and the complete unit and linked baseline are revalidated.

## MenuMainInit

`MenuMainInit__FP13MENU_INIT_ARG` configures menu work areas, camera and scene
state, message objects, and the requested initial menu mode. The current SF
draft emits `0xEB4` bytes against retail's padded `0xED0` extent, with 792 of
948 words differing. These counts include shifted instructions after the
constructor gaps; they are not independent semantic errors.

The first unsupported sequence is at offset `0xBC`: retail branches on the
placement allocator's `v0` result and copies it to `s1` in the delay slot;
native construction copies first and branches on `s1`. The typed
`MENU_DRAW_ENV` already constructs its camera with 8.0f. No missing camera
constructor or float-order selector is established at this point.

This function is parked under the documented
[placement-new blocker](../funcpoint/placement-new.md). Reconsider when the
dedicated compiler/constructor investigation supplies a natural fix, then
remeasure later constructors and argument scheduling rather than attributing
all downstream instruction shifts to that first branch.
