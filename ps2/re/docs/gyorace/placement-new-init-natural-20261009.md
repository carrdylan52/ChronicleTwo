# GyoRace Init: natural resource and induction evidence

## October 8 pntc natural Init investigation

The fresh mandatory `./decompile.sh sgInitGyoRace__FP11SubGameInfo --no-cache`
run succeeds in `chronicletwo_dev:sf-63f7a9e-pn15`. The current31 control at
`bdff2461c53275eca40e66724a8467473d32e51e` measures 431/1,154 positional
words, including its eight-byte overrun. The actual retail symbol is
GLOBAL/FUNC at 0x309A00, size 0x1200; its next-address reservation is also
0x1200, with no tail gap. The native control body is 0x1208.

### Dependent objects and resource lifetimes

`SubGameInfo` supplies the existing `CScene*` and first character texture
block. `AssignStack(5)`/`GetStack(5)` provide the `mgCMemory` from which the
texture/work stacks, message window, effect arrays and six progress arrays
are allocated. `CGyoraceFishData` is the real 0x18-byte table of four counts
and four fish-array pointers; `LoadData` fills it and `GetRaceFish` selects
class/index entries. `BREEDFISH_USED` is the existing owned-fish record; its
accepted `fatigue` and five racing parameters are unsigned halfwords. This
supersedes the earlier signed-fatigue description, without a new header
proposal. The player's increment retains 16-bit storage wrapping, and its
penalty retains signed `(fatigue - 1)` arithmetic.

The only scalar placement construction is `ClsMes`, size 0x2958, with its
genuine out-of-line `__ct__6ClsMesFv`. Its null branch/pointer-copy delay slot
already match at +0x154/+0x158. Uniform class0 is excluded from the placement
policy; no row or class override is added. `BattleEffectPrim[96][32]` and
`grRACE_PROGRESS[1000]` are the real plain arrays. `CHitEffectImage[96]` uses
its real array cookie and `__construct_new_array` callback. The four size
expressions reproduce the existing Alloc counts 0x298, 0x3C02, 0x242 and
0x5DE, including their existing two extra quadwords. No character or texture
object is constructed directly by Init: the existing scene and
`mgTexManager` resource APIs load/register those objects.

`GetSystemMesBuffer` belongs to `sysmes.hpp`; `TEX_SystemEffect1` belongs to
`maintex.hpp`. The private specimen includes those owners instead of local
extern declarations. It uses `LOAD_FILE_READ`, the existing message preset
and window-mode enums, `SND_PORT_ENEMY` and `GYORACE_MODE_READY`. The unknown
sound port 2 is left unchanged.

### Natural source result

Inlining the twelve real string literals, using real sizeof-based counts,
and replacing only Init's inherited RaceVector views with typed
`sceVu0FVECTOR` locals retain 431 words. The retail template bytes are
`(0,-15,0,1)` and `(0,3.1415927410125732,0,1)`; both actual symbols are LOCAL
16-byte objects. Removing the inherited `grRACE_INFO*` base alias and using
`RaceInfo.fish[racer]` produces the best 421/1,152 result, body 0x11F8. It
recovers the retail s3 entry induction, s4 computed owner address and s8
fish-state induction. Ownership-header/enum cleanup leaves this object
unchanged. Init remains guarded, and no profile delta is proposed.

The new genuine callee-scoped evaluate-first controls for
`SetStep__11CCharacter2Ff`/0.3f and
`grGetFishProgress__FP11grRACE_INFOifP15grRACE_PROGRESS`/0.0f each satisfy
`expected_matches=1`, but the entire native object remains identical.
Previously documented camera-reference/pointer-loop negatives are not
replayed. A chosen-entry reference worsens to 684 words. Keeping the older
fish-data lifetime gives 438, direct texture-manager ownership 439, and a
shared entrant index 432. Direct entrant-slot access and a character-number
reference are no-ops.

The best still swaps the real game_data induction and chosen-slot registers
at +0x69C. Its stamina expression combines the two retail parameter loads,
removing two instructions and shifting the remainder eight bytes; character
progress/induction registers and final camera-reference scheduling still
differ. Raw positional differences are 622; independent relocation masking
gives 421. After also masking branch destinations and aligning words, 38
native and 40 retail words remain unmatched. Alignment is diagnostic and
does not establish a match. A future hypothesis is the real five-element
parameter-subarray lifetime; no fake alias or added state is justified.

### Complete audit and receipts

Every one of the ten nonselected native functions, including the weak
`mgRect<int>::Set` body, preserves binding, size, full masked bytes and
normalized relocations. All 41 named data objects preserve binding, size,
full bytes and normalized data relocations. The Loop switch table and two
DivSprite anonymous templates preserve their exact data/relocations despite
numeric renaming. Fourteen new target-only literals/templates each match
one actual LOCAL retail symbol by size, contents and binding. Thirteen
obsolete target data aliases disappear; the motion alias still used by Loop
is retained. Defined/undefined vtable sets are both unchanged and empty.
All 88 original storage markers and 18 captured accepted headers remain
unchanged. No shared header is edited.

The fresh guarded private wrapper runs the normal two-pass mwccgap,
postprocess and fixup pipeline and passes the complete checker: 0x5004
bytes, 979 relocations. Its object is identical to the genuine current31
wrapper control. This validates the retained guards and data, not the
nonzero native Init body. All 31 production placement rows remain byte
identical; no complete-game build is run by this worker.

Receipts are in `.private/pntc/gyorace-init-natural/`: `freeze.json`, the
exact `sources/frozen-best/gyorace.cpp` and `frozen-profile.json`,
`frozen-source.diff`, `frozen-profile.diff`, `m2c-receipt.json`,
`retail-data-census.json`, `float-controls.json`, `residual-metrics.json` and
`outputs/frozen-best/{provenance,audit,data-audit}.json`. The guarded wrapper
receipt is `outputs/frozen-guarded-wrapper/wrapped/`. Failed helper-name
controls and the initial private driver setup failure are retained; no new
quadword helper was introduced.

The exact coordinator source copy is `.private/proposals/gyorace-init-natural-source.patch`; its NONMATCHING fallback remains intact. No selector delta is proposed.

## Current design reference

This dated evidence retains its original measurement scope.
[The consolidated placement-conversion design](../satansfiddle/placement-new.md) owns the current
capability, activation rows, safety requirements, and accepted source status.
