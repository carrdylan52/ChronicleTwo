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

Private receipts are under `.private/receipts/`: `r2-final-build.log`,
`r2-final-objects.log`, `r2-final-funcpoint.log`, `r2-final-mg_tanime.log`,
`r2-coverage-after.log`, `r2-final-<other-unit>.log`, per-experiment `r2-e27`
through `r2-e33` logs, `r2-matched-placement-sites.log`, and the two
`r2-entry-helper-<unit>.log` audits. Hypotheses are in the private experiment
logs. `r2-e27-rejected.patch` and `r2-e33-rejected.patch` preserve rejected
shared/source experiments; they are **not proposals for integration**.
