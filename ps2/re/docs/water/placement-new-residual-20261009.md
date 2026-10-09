# Natural water-frame factory residual

`CreateWaterFrame__FiiPfPfP9mgCMemory` remains guarded. Fresh pn15 canonical
compilation differs by 82 words. Selecting its one witnessed inline
CWaterFrame construction under either conversion timing leaves 46 words.
An ordinary initialized `CWater *const water` local, whose pointer is never
reassigned, reduces this to one word with the exact relocation offset/type
map. No constructor or field initialization is replaced by hand.

The actual retail GLOBAL/FUNC symbol is 0x198 bytes at 0x001872f0, within
extent 0x1a0. The best native body has the same size and binding. Its sole
residual is +0xf4: retail passes the saved s6 object pointer to the out-of-line
CWater constructor, while the draft passes the equal v0 value. The preceding
branch delay slot has already copied v0 to s6. All other instructions and
relocation positions agree, including both allocation failure exits. The
source is not promoted for an equal-valued but nonidentical instruction.

Existing water, frame, visual and memory documentation establishes all used
types. The factory constructs a 0x120 water frame, a 0x90 frame attribute and
a 0x80 water visual, then allocates trivial 0xb0 BoundInfo storage. The arena
requests retain size/16 plus two reserved quadwords: 20, 11, 10 and 13.
The frame receives disabled depth writes, GEQUAL depth testing, disabled
alpha testing and ordinary blending. The visual receives the grid size and
two corner vectors, is attached virtually, and supplies the frame's box.
The current header names the two visual surface parameters at +0x48/+0x4c
as surface_param0/surface_param1; the earlier notes' unknown names are stale.

The private selector uses logical water.cpp, exact caller, scalar allocator
`__nw__FUiP1`, constructor `__ct__11CWaterFrameFv`, and expected_matches one.
The CWater and mgCFrameAttr constructors remain out of line; BoundInfo is
trivial. They are excluded rather than broadening the scalar class-6 policy.
This caller passes pointer-valued corners and has no floating constant
argument for the existing float evaluation capability.

Eighteen successful genuine controls include the canonical source, both
scoped timings, initialized/immutable real pointers, allocation assignment
inside the null condition, positive branches, actual constructor value
initialization, unchanged parameter values and direct frame-attribute access.
Ordinary nonconst declaration/assignment forms retain 46. A nonconst positive
water branch reaches nine; constant-water positive/frame branches reach 41
and 71. Direct frame-attribute access reaches 63 and changes the body size.
Adding actual frame/attribute/parameter const qualifiers or using value
initialization retains one. Before timing on the one-word source also retains
one. No dummy locals, helper functions, byte walks, puns or float policies
are introduced. Every successful variant preserves all 24 nonselected scored
bodies and normalized relocation targets.

Fresh mandated m2c, exact source/profile specimens, explicit statuses,
objects, instruction diffs, nonselected audits and the actual ELF symbol
receipt are under `.private/pntc/water-create-natural/`. An initial private
preparation removed a same-named local in an unrelated function and failed;
its stopped batch is preserved separately, and every subsequent edit is
restricted to this factory. That failure is not a compiler matching result.
The private one-word source and selector remain uncommitted.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](../satansfiddle/placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
