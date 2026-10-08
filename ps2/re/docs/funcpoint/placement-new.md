# Placement-new null checks in MWCC 3.0-011126

## Result

The tested natural constructor forms do not reproduce both retail allocation
sequences. Keep `Add__14CFuncPointMngrFiP9mgCMemory` and
`NewTexAnimeData__15mgCTextureAnimeFP9mgCMemory` guarded. No source transformation
or shared-header patch is validated for other units.

The retained natural drafts are:

```cpp
CFuncPoint *CFuncPointMngr::Add(int type, mgCMemory *stack) {
    CList<CFuncPoint> *node = new ((u_long128 *) stack->Alloc(0x20)) CList<CFuncPoint>;
    if (node == NULL) {
        return NULL;
    }
    node->data.Initialize();
    return Add(type, node);
}

CList<mgCTexAnimeData> *mgCTextureAnime::NewTexAnimeData(mgCMemory *stack) {
    CList<mgCTexAnimeData> *node;
    node = new (stack->Alloc(6)) CList<mgCTexAnimeData>;
    return node;
}
```

The allocator overload is the out-of-line `operator new(size_t, u_long128 *)`
in `mg_memory.hpp`. Non-trivial construction generates a null check of the
allocation result. The unresolved issue is how MWCC binds that result to the
persistent object pointer during inlining and register allocation.

## Retail instructions

`CFuncPointMngr::Add(int, mgCMemory*)`, address 0x002A11C0, extent 0xA0:

```asm
+0x2c  jal   __nw__FUiP1
+0x30  move  a1,v0
+0x34  beqz  v0,+0x60
+0x38  move  s0,v0             # branch delay slot
```

Construction writes the list vptr, constructs the frame at node+0x80, and
dispatches virtual list initialization. A separate check at +0x60 rejects
a NULL node. Only afterwards does +0x70 call point initialization on node+0x10;
the existing list-taking `Add` overload is called at +0x80.

The original draft copies `v0` to `s0` at +0x34 and branches on `s0` at +0x38:
2/40 instruction words differ. The other 33 unit functions match in the draft
compilation.

`mgCTextureAnime::NewTexAnimeData`, address 0x0013DA40, extent 0x80:

```asm
+0x24  jal   __nw__FUiP1
+0x28  nop
+0x2c  move  s0,v0
+0x30  beqz  v0,+0x64
+0x34  nop
+0x38  lui   v0,%hi(list_vtable)
+0x3c  addiu v0,v0,%lo(list_vtable)
+0x40  sw    v0,0x3c(s0)
```

This unit uses `schedule off`: the copy precedes the branch rather than
occupying its delay slot. The original draft branches on `v0` at +0x2C, copies
to `s0` at +0x34 only on the non-null path, and uses `v1` for the vtable write.
There are 6/32 differing words; its body is 0x7C bytes against a padded retail
extent of 0x80. Constructor specialization narrows this to one word,
`beqz s0` in place of `beqz v0`, without fixing it.

## Constructor evidence

`CList<T>` is defined in `mg_tanime.hpp`. Its constructor already calls virtual
`Initialize()`, after constructing its data member. It is not missing a
non-trivial constructor.

- Retail `__ct__10CFuncPointFv` at 0x0015F5D0 only constructs its `mgCFrame`
  member at +0x70. It does not call point initialization.
- Retail `__ct__19CList_10CFuncPoint_Fv` at 0x002A13E0, used by `Reserve`'s
  array construction, writes the list vptr, constructs the frame, and
  dispatches list initialization. It does not initialize the point's other
  fields either.
- The out-of-line `mgCTexAnimeData` constructor at 0x0013C340 calls its
  `Initialize`. Retail `NewTexAnimeData` calls that constructor at +0x48,
  followed by virtual list initialization.
- `Initialize__23CList_14PartsGroupData_Fv` at 0x0015DB50 and
  `Initialize__18CList_P9CMapParts_Fv` at 0x0015E3D0 only clear the links.
  They support the current list initialization operation; they do not imply
  additional initialization of the carried object.

Making `CFuncPoint()` call `Initialize()` and replacing the caller with
`return Add(type, new (stack->Alloc(0x20)) CList<CFuncPoint>);` contradicts these
retail constructors. It also moves point initialization before list
initialization and removes the caller's second null guard.

## Matching callers elsewhere

An active-matching-assembly scan found 70 scalar placement-allocator call sites
with a nearby `beqz v0`. This counts instruction patterns, not 70 validated
natural new-expressions.

`NameRegistInit__FP9mgCMemoryPii` is a genuine positive example. Its ordinary
`CNameRegiMenu` constructor is declared in a header and defined inline in the
source before the caller. A named local receives the new-expression, yet
the matching code tests `v0` and copies to a saved register in the delay slot.
Named locals alone therefore do not forbid the desired sequence.

`mapPARTS__FP9SPI_STACKi` genuinely constructs `CList<CMapParts>`, but has
`inline_depth(0)` and calls the constructor out of line. It does not establish
the lowering of an inlined list constructor. `CreateGrid` constructs trivial
`CEditGrid` with an explicit assignment condition, without an implicit
constructor check. Other apparent examples, including `_ESM_INIT_FIX`,
character outline, sky, and villager allocation, explicitly call `operator new`
and initialize afterwards; they do not establish a natural constructor idiom.

## Constructor-form results

These continuation hypotheses were tested after reading the earlier logs:

- Initialized `CFuncPoint` constructor plus direct-return caller: 27/40 words
  differ. The full PAL build fails; complete objects pass 144/149. `map`,
  `dng_main`, `scene`, `funcpoint`, and `scenevillager` fail, including changes
  to containing objects and static initialization.
- Compiler-generated `CFuncPoint` constructor instead of its empty declared
  body: unchanged 2/40 words; the array-node constructor still matches.
- Complete `CList<mgCTexAnimeData>` specialization before the first `sizeof`
  instantiation: compiles, unlike the earlier misplaced specialization, but
  leaves the wrong branch operand (1/32 words). Defining virtual initialization
  in the header also changes its scheduling and produces a mismatch.
- Generic out-of-class constructor without `inline`: constructor stays out
  of line. Funcpoint differs 27/40 words (0x8C body); mg_tanime 28/32 words
  (0x4C body).
- Empty declared list constructor with explicit caller list initialization:
  funcpoint differs 11/40 words and mg_tanime grows to 0x8C. The array-node
  constructor loses its link initialization and differs 9/20 words.
- Compiler-generated list constructor with the same explicit initialization:
  identical unsuccessful result to the empty constructor.
- Ordinary out-of-class inline constructors of complete specializations,
  defined in source before the callers like `CNameRegiMenu`: funcpoint remains
  2/40 words and mg_tanime 1/32 words. Both list initialization functions and
  the funcpoint array constructor match, isolating the unresolved caller branch.

All experimental source and header changes were restored.

## Rejected helper evidence

`AddNewFuncPoint(manager, type, new(...))` matches all 40 words. Its inline
formal parameter separates construction from the caller's null check and
point initialization. It is an invented code-generation helper and is not
admissible source. The exact interrupted source was saved privately and its
match was reproduced in the continuation.

`CheckedTexAnimeNode(new(...))` returns NULL or its input unchanged: a semantic
no-op, also inadmissible. The saved E25 receipt and a fresh check of the exact
interrupted source both report **0x88 bytes versus retail 0x80**, despite the
continuation brief describing it as a match. No mg_tanime helper match was
reproduced with baseline headers.

The funcpoint helper proves that this compiler can emit the desired sequence.
It does not prove that retail initialized the point inside its constructor or
that an additional source check is an acceptable universal fix.

## Cross-unit checks

No other guarded draft was fixed. Restored-header draft checks still report:

- scenevillager `CharaObjectOnOff__6CSceneFiP9mgCMemory`: 6/112 words.
- fishing `sgRestartFishing__FP11SubGameInfo`: 2/344 words, 0x55C/0x560;
  `InitSuccess__FP6CScene`: 25/280 words, 0x458/0x460;
  `StepDataLoading__FPv`: 0xB88/0xB70 bytes.
- mdslist `CreateChara__FPUiPcP9mgCMemory`: 2/76 words, 0x12C/0x130;
  `Copy__9CMapPieceFR9CMapPieceP9mgCMemory`: 57/156 words, 0x26C/0x270.
- sceneload `LoadChara__6CSceneFiPUiPcP9mgCMemoryP9mgCMemoryP9mgCMemoryii`:
  120/148 words, 0x218/0x250; `CopyChara__6CSceneFiiP9mgCMemory`:
  138/164 words, 0x258/0x290.
- dynamicanime `dynCOLLISION__FP9SPI_STACKi`: 2/76 words.
- water `CreateWaterFrame__FiiPfPfP9mgCMemory`: 82/104 words.

These are baseline scores, not proof that each entire difference is caused
by this idiom. The broader drafts have additional differences. No non-owned
source was changed during these probes.

## Validation, receipts, and reconsideration

After restoration the PAL build passes all ten file-backed sections and the
memory-end check. Complete objects pass 149/149. Both unit `draft.sh --promote`
checks pass with the original assembly guards active. Coverage remains 6,651
matched, 204 guarded drafts, and 17 asm-only functions out of 6,872. These
checks validate restoration, not promotion of either target.

Blocker category: constructor inlining/allocation-result register selection.
Reconsider when a genuine matched caller with comparable inlined virtual-list
construction and null-result flow establishes an untested source distinction,
or independently established retail type evidence requires another constructor.
A new helper match alone is not such evidence.

## Chronicle / Dark Cloud 1 comparison

### Compiler and evidence boundary

The comparison uses the read-only Chronicle checkout at
`/home/dylan/projects/Chronicle`, revision
`66051c26538644d3a73db3f7fab674f417fff5f6`.
`ps2/CMakeLists.txt:65,97` and `scripts/build/mwccgap.sh` select
`tools/compilers/mw/2.3.3/mwccmips.exe`, with `-O2`, exceptions/RTTI off,
read-only strings, and `divbyzerocheck on`. The existing NTSC/PAL
`compiler_flags.txt` files agree. The compiler's SHA-256 is
`6375fd27814a1cb7eddbc229ecbd15b57d83562f95be23804f0a06ad4814e2df`;
`tools/mwcc-debug/profiles/mwccmips-2.3.3-000921.json` identifies that exact
image as **2.3.3 build 1010, 2000-09-21**. This is an earlier compiler than
DC2's **3.0-011126, `-O3,p`**, not the same build. Chronicle also routes
mwccgap through `statefix-wibo.sh` / `statefix.py` for recovered compiler-state
bugs. DC1 addresses, node layouts, and state overrides are not DC2 settings.

A scan of all `new` tokens in `ps2/src` and `ps2/include`, excluding allocator
definitions/declarations and comments, finds **19 placement-new expressions**:
10 scalar and 9 array sites. None is a guarded draft. All are in generated
`ps2/asm/<region>/matchings/` files, and their 11 enclosing functions have
`cpp` provenance in the existing builds. A read-only comparison of every
instruction's recorded retail bytes with the corresponding virtual address
in the existing linked NTSC and PAL `SCUS_971.11` files finds zero differing
words for all 11 bodies in both regions. This establishes the examples against
existing builds; it is not a fresh Chronicle build. Chronicle was not edited,
built, or subjected to write-producing scripts.

For the scalar table, **A** means the implicit construction guard tests `$2`
(`v0`), with the result stored/copied after the guard. **B** means a copy into
a saved register precedes the guard and that saved register is tested.
There are **4 A and 6 B scalar sites**. All ten caller branch delay slots are
`nop`; **none reproduces DC2 funcpoint's allocation-result copy in the delay
slot**. DC2 mg_tanime's copy-before-`beqz v0` is also absent from these sites.
The array sites have no caller construction guard and are recorded separately
as **N**, rather than incorrectly counted as A or B.

### Scalar comparison table

Paths and line numbers below refer to Chronicle. `move` denotes its printed
`paddub dst,src,$0`. Allocator-call addresses identify the individual sites,
including the four arms of `CreateVisual`. Each quoted source statement is
the exact spelling at that site; `CBound` additionally declares `bound` in
that statement. Every scalar new-expression has an implicit compiler null
guard. The explicit caller tests are listed independently.

| Class; source location; NTSC / PAL allocator call | Exact source spelling | Constructor and destination | Caller test; following operations; pointer after calls |
| --- | --- | --- | --- |
| A; `clothread.cpp:144`, `InitCloth`; `0x13F9F4` / `0x13F9C4` | `pCloth = new ((u_long128 *) alloc->Alloc(0x856)) CCloth(16, 16, 1.0f);` | Buffer cast; user-defined out-of-line constructor with three arguments; global destination. | `beqz v0`; pass `v0` in `a0` to ctor; store ctor return to `pCloth` at the join. Then assign `ParentFrame`, clear `pBound`, test `frame`, explicitly test `pCloth`. Global reloaded for later cloth calls. |
| B; `clothread.cpp:279`, `CommandBOUND`; `0x13FDE8` / `0x13FDB8` | `CBound       *bound = new ((u_long128 *) DataBuffer->Alloc(0x14)) CBound(1.0f, 1.0f, 1.0f);` | Buffer cast; user-defined out-of-line constructor with three arguments; initialized local. | `move s1,v0; beqz s1`; pass saved pointer to ctor; replace `s1` with ctor return. Explicit `bound == NULL` test follows. Pointer survives `SearchFrame`, then is used by `SetDir`, extent stores and attachment. |
| B; `dataset.cpp:508`, `CreateVisual`, `attr & 2` and `attr & 8`; `0x1267E8` / same | `visual = new ((u_long128 *) alloc->Alloc(3)) CVisualShadow;` | Buffer cast; no arguments; compiler-generated derived ctor, user-defined base ctor; shared local assigned in four arms. | `move s0,v0; beqz s0`; call `CVisualMDTVu1` base ctor, then write derived vptr through the saved original allocation pointer. No explicit result test. Join calls virtual `visual->Initialize()`, then `Align64`; pointer used throughout later visual creation. |
| A; `dataset.cpp:510`, `CreateVisual`, `attr & 2` and not `attr & 8`; `0x126834` / same | `visual = new ((u_long128 *) alloc->Alloc(3)) CVisualMDTVu1;` | Buffer cast; no arguments; user-defined out-of-line ctor; same four-arm local. | `beqz v0`; ctor consumes `v0` in `a0`; `move s0,v0` at the null/ctor join consumes the expression/ctor return. Same virtual initialization, alignment and later uses as the shadow arms. No explicit result test. |
| B; `dataset.cpp:514`, `CreateVisual`, not `attr & 2` and `attr & 8`; `0x126880` / same | `visual = new ((u_long128 *) alloc->Alloc(3)) CVisualShadow;` | Buffer cast; no arguments; compiler-generated derived ctor, user-defined base ctor; same four-arm local. | Same B sequence and post-base-ctor derived-vptr write as line 508. Same following calls and uses; no explicit result test. |
| A; `dataset.cpp:516`, `CreateVisual`, neither flag; `0x1268CC` / same | `visual = (CVisualMDTVu1 *) new ((u_long128 *) alloc->Alloc(2)) CVisualVu1;` | Buffer cast and result cast; no arguments; user-defined out-of-line ctor; same four-arm local. | `beqz v0`; ctor consumes `v0`; `move s0,v0` at join. Same following virtual call/alignment and saved-pointer uses; no explicit result test. The result cast is unique to this arm and is not necessary for A (line 510 is also A). |
| B; `dataset.cpp:794`, `CreateCollisionMDT`; `0x1272CC` / same | `collision = new ((u_long128 *) alloc->Alloc(4)) CCollisionMDT;` | Buffer cast; no arguments; user-defined inline derived ctor with an implicit polymorphic base ctor; function local. | `move s4,v0; beqz s4`; both compiler vptr writes and three zero stores are inline, with no ctor call. Then assign `model`, call `CreateBBox`, build mesh/boxes; saved pointer remains needed. No explicit result test. B therefore does not require an out-of-line ctor. |
| B; `dataset.cpp:895`, `CopyFrameVu1`; `0x127658` / same | `copy = new ((u_long128 *) alloc->Alloc(sizeof(CFrameVu1) / 16)) CFrameVu1;` | Buffer cast; no arguments; user-defined out-of-line ctor; local. | `move s0,v0; beqz s0`; call ctor and replace `s0` with ctor return. Explicit test is only of input `frame`, before allocation. Then virtual `copy->Initialize()`, object assignment, recursive child copies/parent calls, return `copy`; pointer survives these calls. |
| B; `dataset.cpp:918`, `CopyFrame`; `0x127748` / same | `copy = new ((u_long128 *) alloc->Alloc(sizeof(CFrame) / 16)) CFrame;` | Buffer cast; no arguments; user-defined out-of-line ctor; local. | Same B sequence, ctor-return replacement, input-only guard, virtual initialization, assignment, recursion and returned saved pointer as `CopyFrameVu1`. |
| A; `editloop.cpp:768`, `EditInit`; `0x17869C` / `0x17AE6C` | `pEditGround = new ((u_long128 *) EtcDataBuffer.Alloc(0x2097)) CEditGround;` | Buffer cast; no arguments; user-defined out-of-line ctor; global destination. | `beqz v0`; ctor consumes `v0`; ctor/expression return stored to global at join. No explicit result guard here. Next allocate `ObjParts`, `MotionParts`, `RiverParts`, `RoadParts`; global reloaded later into `ground` and used for loading/grid calls. |

### Array comparison table

For N sites with non-trivial elements, `__nwa__FUiP1` is followed by
`a0 = v0` and `__construct_new_array`; its return is then saved/stored.
There is no explicit result-null test in any of these nine callers.
`__construct_new_array` itself guards its incoming `a0`, with `move s0,a0`
in that branch's delay slot at NTSC `0x122438` / `0x12243C`. This is a
runtime helper's argument check, not the caller's allocation-result check.
Its compiler/runtime settings and exception machinery differ from game code;
it does not establish the scalar new-expression lowering under investigation.

| Class; source location; NTSC / PAL allocator call | Exact source spelling | Construction; destination; following operations and call lifetime |
| --- | --- | --- |
| N; `dataset.cpp:399`, `LoadMDSFile`; `0x1263C0` / same | `frames = new (block) CFrameVu1[header->object_num];` | Named `u_long128 *block`, assigned by `(u_long128 *) alloc->Alloc(header->object_num * sizeof(CFrameVu1) / 16)`; no cast in placement operand. User-defined no-arg element ctor via runtime helper; local receives helper result in `s7`. Followed by per-object loop, initialization/transform calls and final `return frames`; pointer persists across calls. |
| N; `dataset.cpp:725`, `LoadCollisionFile`; `0x127008` / same | `frames = new (block) CFrameVu1[header->object_num];` | Named `u_long128 *block`, assigned by `(u_long128 *) alloc->Alloc(((MDS_HEADER *) data)->object_num * sizeof(CFrameVu1) / 16)`; no cast in placement operand. Same user-defined element ctor/helper; local receives helper return in `s0`. Followed by object loop and `node->Initialize()`/matrix setup; pointer used after calls and returned. |
| N; `fishing.cpp:123`, `FishingLoadFish`; `0x1A8950` / `0x1ABB30` | `Fish = new ((u_long128 *) alloc->Alloc(0xD8C)) CFish[6];` | Buffer cast; user-defined no-arg element ctor via helper; global assignment. Then store `fish_texb`, set `FishNum`, choose count/rarity and load fish data. Global array accessed after subsequent calls. |
| N; `editloop.cpp:767`, `EditInit`; `0x178650` / `0x17AE20` | `EditArea = new ((u_long128 *) EtcDataBuffer.Alloc(0x81C)) CEditArea[4];` | Buffer cast; user-defined no-arg element ctor via helper; global assignment. Next scalar `pEditGround` allocation and other array allocations; retained globally across calls. |
| N; `editloop.cpp:769`, `EditInit`; `0x1786D8` / `0x17AEA8` | `ObjParts = new ((u_long128 *) EtcDataBuffer.Alloc(0x3F1)) CMapParts[24];` | Buffer cast; user-defined no-arg element ctor via helper; global assignment. Next `MotionParts` allocation; later assigned to `EdEventInfo.edit_parts` after calls. |
| N; `editloop.cpp:770`, `EditInit`; `0x178720` / `0x17AEF0` | `MotionParts = new ((u_long128 *) EtcDataBuffer.Alloc(0x470)) CCharacter[4];` | Buffer cast; user-defined inline no-arg element ctor emitted as a routine for helper; global assignment. Next `RiverParts` allocation; retained globally across calls. |
| N; `editloop.cpp:771`, `EditInit`; `0x178768` / `0x17AF38` | `RiverParts = new ((u_long128 *) EtcDataBuffer.Alloc(0x2B0)) CMapParts[16];` | Buffer cast; same user-defined element ctor/helper as `ObjParts`; global assignment. Next `RoadParts` allocation; retained globally across calls. |
| N; `editloop.cpp:772`, `EditInit`; `0x1787B0` / `0x17AF80` | `RoadParts = new ((u_long128 *) EtcDataBuffer.Alloc(0x102)) CMapParts[6];` | Buffer cast; same user-defined element ctor/helper; global assignment. Next initialize `near_clip` and call `MGSetRenderInfo`; retained globally across calls. |
| N; `edit_in.cpp:1775`, `LoadData`; `0x19EF08` / `0x1A20E8` | `func_point = new ((u_long128 *) (EdNPCBuffer.base + EdNPCBuffer.used * 16)) EPARTS_FUNC_DATA[128];` | Buffer cast; trivial implicit element construction, no runtime ctor helper or implicit null branch. Directly store array allocator's `v0` to global. Then `EdNPCBuffer.Alloc(0x600)` reserves storage and a loop clears `kind`; global array reloaded after that call. This existing DC1 arithmetic is evidence only, not an admissible DC2 source proposal. |

No expression in this inventory assigns to an object member or directly
returns a new-expression. There is consequently no matched DC1 example here
for either of those destination forms. Every scalar inline placement-buffer
cast occurs in both A and B; all ten objects are later retained across calls,
either in a saved register or a global. Buffer cast, constructor arguments,
explicit caller null test, and eventual use after a call do not separately
predict the branch operand.

### What the comparison establishes

`CreateVisual` supplies the best controlled distinction: identical allocator,
function, saved destination, surrounding branches and later uses, but A for
the out-of-line base constructors and B for the implicit shadow constructor.
The latter must retain the original object while the base ctor runs, because
it writes the derived vptr afterwards. The former can carry the expression
through the ctor's ABI return and only bind the destination at the join.
This explanation follows the observed uses; it is not a proved universal
frontend rule. `CopyFrame`/`CopyFrameVu1` remain B even though their constructors
also return the object, and inline `CCollisionMDT` remains B without any
constructor call. A named local and an out-of-line ctor therefore do not
suffice. DC2's inlined list constructors still need the original list pointer
after member construction and virtual initialization, so the DC1 A examples
do not have the same internal live range.

### Relevant rules from the complete Chronicle MWCC notes

Chronicle `docs/MWCC.md` was read in full. Its relevant claims and their limits
for this problem are:

- Section 2: graph coloring picks the lowest free physical register; it is
  interference, not a register preference. A pointer live across an external
  call uses a saved register. This predicts the need for `s0` in DC2, but not
  whether the earlier guard reads the allocation temporary or the saved node.
- Section 2: declaration order affects saved-register numbering; ternary
  assignments can replace the local with a lower-numbered optimizer temporary.
  Separate block-scoped locals split graph nodes. These are destination/lifetime
  hypotheses, not a rule that selects `v0` for a constructor check.
- Section 2: removal/coloring order depends on graph degree and static
  reference count divided by degree; shared/reused locals and split loops
  alter it. Surviving copies arise from call arguments/results after local
  copy folding. These facts distinguish an ABI result copy from a C++ source
  assignment; no spill or 26-register-pressure evidence exists in these small
  allocation sequences. Dead assignments described in the DC1 note are not
  source proposals under this lane's natural-code rules.
- Section 2: an already-defined static callee calling nothing external can
  leave caller-saved values live across its call. This does not apply to the
  external placement allocator or the member/virtual ctor calls here.
- Section 3: naming a call argument changes evaluation order; no-op casts can
  change evaluation rank and CSE. Both scalar classes already share the same
  buffer cast. Neither rule promises the desired branch operand. No extra
  casts/comma constants or other semantic no-ops are proposed.
- Section 3: assignment folded into a condition can preserve a global call
  result instead of storing/reloading it; a shared local for two call results
  can keep an ABI copy that separate locals fold. Section 6's multi-definition
  example similarly prevents call-result folding. These depend on real data
  flow. Inventing dead definitions or repeating the previously tested allocation
  conditions would not supply missing retail source evidence.
- Section 7: declaration placement affects emitted initializers/stack slots;
  standalone initialization and assignment nested in a following call can
  produce different argument-register lifetimes. Section 8: returning a final
  assignment can reserve `v0` earlier. None states how an implicit placement-new
  construction condition is bound. DC1 has no direct-return site in this set.
- Sections 4 and 6: comparison spelling can choose `$at` versus a general
  register for an integer comparison result; a displaced statement changes a
  branch displacement. This does not equate testing `s0` with testing an ABI
  `v0` pointer, and all source comparison/layout variations already tested stay
  rejected as solutions.
- Section 1: persistent helper argument masks change interference, while
  float-node evaluation flags and literal aliasing can read unwritten state.
  Those observations concern the exact 2.3.3 image and DC1's statefix workflow.
  They provide no established integer allocation-result state bug in 3.0-011126.
  Sections 8/9 also distinguish virtual versus qualified calls and generated
  class operations; constructors must preserve the independently evidenced
  member/vptr/virtual-call sequence.

The compiler profile independently reports that the 2.3.3 scheduling pass is
entered only at optimization level 3 or higher; this project uses `-O2`.
Its scalar sites therefore cannot validate a scheduling rule for DC2's
scheduled funcpoint body. The runtime array helper's filled delay slot is a
different compilation case. No rule in the 414-line note directly predicts
the two-stage allocation-result binding needed by either DC2 target.

### DC2 probes and final state

Each probe used original constructors and one hypothesis at a time, with
`JOBS=3`. No shared header was changed. All source edits were local and restored.

| Probe | Sole distinction | Funcpoint | mg_tanime | Other guarded functions |
| --- | --- | --- | --- | --- |
| E34 | Name the `u_long128 *buffer` from `Alloc` before `new(buffer)`; compare argument/result lifetime with DC1's named-buffer arrays. | Unchanged 2/40 words. | Unchanged 6/32 words, 0x7C/0x80. | `dynCOLLISION` and `CreateChara` unchanged 2/76 words; both still copy before `beqz s0`. |
| E35 audit | `return new (stack->Alloc(6)) CList<mgCTexAnimeData>;`, without a helper. | Not applicable: point initialization must precede the existing `Add` consumer. | Unchanged 6/32 words. | None. Direct-return failures were already recorded in mg_tanime's unit notes; this run is a reproduction, not a new hypothesis. |
| E36 | Destination declaration lifetime: split funcpoint's declaration initializer; initialize mg_tanime's destination at declaration. | Unchanged 2/40 words. | Unchanged 6/32 words. | Narrow `pipe` into its using block and initialize it there; initialize `chara` at declaration. Both unchanged 2/76 words. |
| E37 | Compile unchanged source with 3.0-011126 at `-O2` instead of `-O3,p`; measure the optimization-level transfer separately from compiler version. | 0xC4 versus 0xA0; still copies to `s0` before `beqz s0`; 0/34 unit functions match. | Target unchanged 6/32; 25/33 unit functions match instead of 31/33. | None; no settings patch retained. |

The direct-return reproduction adds no new evidence. Previously tested
constructors, allocation conditions, pointer casts, inlining/analysis switches
and rejected helpers were not reintroduced. No source transformation matches,
and neither target is promoted. The restored full PAL build passes the ten
file-backed sections and memory-end check, complete objects pass **149/149**,
and both owned units pass `draft.sh --promote` with their original assembly
guards active. These validate restoration. Coverage remains **6,651 matched,
204 guarded drafts, 17 asm-only / 6,872 functions**.

The evidence still needed is either (1) an active natural 3.0-011126 caller
with comparable inlined list construction, a retained original allocation
pointer across member/virtual calls, and the desired `v0` guard, exposing a
source distinction not already tested; or (2) an independently recovered
3.0-011126 frontend/PCode trace locating where the allocation temporary and
persistent node become the same value. A before/after-inlining, copy-folding,
register-allocation and scheduling comparison with the positive
`NameRegistInit` caller would separate binding from scheduling. Physical
register/coloring advice alone cannot explain which virtual value the branch
tests. Reconsider on either piece of evidence, or new retail constructor/type
evidence requiring a different natural construction sequence. A DC1 A site
without the same live range, a runtime-helper delay slot, or another invented
helper match is insufficient. There is no recipe to send to other lanes yet.

Round-3 private receipts: `r3-dc1-inventory.json` contains both-region addresses,
full-body byte-comparison counts and allocation windows; `r3-e34-*`,
`r3-e35-mg_tanime.log`, `r3-e36-*`, and `r3-e37-*` contain focused diffs and
compiler-setting comparisons. `r3-final-build.log`, `r3-final-objects.log`,
`r3-final-funcpoint.log`, `r3-final-mg_tanime.log`, and
`r3-coverage-after.log` record restoration checks. Hypotheses and results are
in `.private/experiments-*.md`; the non-owned probe sources were restored
byte-for-byte.

Private receipts are under `.private/receipts/`: `r2-final-build.log`,
`r2-final-objects.log`, `r2-final-funcpoint.log`, `r2-final-mg_tanime.log`,
`r2-coverage-after.log`, `r2-final-<other-unit>.log`, per-experiment `r2-e27`
through `r2-e33` logs, `r2-matched-placement-sites.log`, and the two
`r2-entry-helper-<unit>.log` audits. Hypotheses are in the private experiment
logs. `r2-e27-rejected.patch` and `r2-e33-rejected.patch` preserve rejected
shared/source experiments; they are **not proposals for integration**.
