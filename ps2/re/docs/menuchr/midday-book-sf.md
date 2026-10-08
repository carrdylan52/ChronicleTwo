# Monster-book title argument order

The `menuchr.cpp` expression selector for `Draw__12CMosBookMenuFv` chooses
binary32 16.0 (`0x41800000`) first for the real callee
`PrimQuad__FP11mgCDrawPrimff9mgRect<i>`, with `evaluate_first: true`.
This is MWCC's actual callee spelling; the normalized manifest spelling
with `_i_` does not identify the call and leaves the selector unconsumed.

The title calls `PrimQuad(prim, 18.0f, 16.0f, rectangle)`. Retail materializes
16.0 in f13 before 18.0 in f12. Without the selector, four words at +0x5B4,
+0x5B8, +0x5BC, and +0x5C4 reverse the immediate/register setup. Named float
coordinates and explicit `float(16.0)` leave those four words. The literal
is a direct argument; nested-call/variable disambiguation is unnecessary.

With the natural shared horizontal icon-position lifetime and this row,
private E118 and the retained `book-native/draft.diff` reach zero differing
words. `book-native/objects.log` accepts the whole unit with all six native
drawing tables, 0x11CC4 allocated bytes and 3,745 resolved relocations.
The row applies only to this unit, function, value, and real callee. No
private costume-draw selector is included.

See [native monster-book drawing](midday-book.md) for rendering behavior,
native data, negative probes, and integrated acceptance receipts. The row
and this evidence note form a separate profile commit.
