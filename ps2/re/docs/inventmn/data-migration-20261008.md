# October 8 data migration

Baseline: `7c7edc2f`, MWCC 3.0/Satan's Fiddle through
`chronicletwo_dev:sf-63f7a9e`. The warm build verifies PAL OK and
149/149 canonical objects. Initial markers: 261 RODATA / 51 BSS.
Refreshed matched data: 4 / 18656 bytes.

## Named storage

Thirty-four named objects now use their existing native types and LOCAL
linkage: menu/session pointers, photo and recipe parsing state, notebook
arrays, drawing and command state, photo-effect state, album slot state,
debug state, and work buffers. The recipe manager is eight bytes and has
no constructor; its definition adds no initialization code.

`CMenuInventPt` and `InventSubDataReadBGInfo` are four-byte objects despite
eight-byte reservations. Byte and halfword flags keep their declared
widths. `temp_1728` is a 33-byte string; the fifteen bytes before the next
symbol are alignment, not characters. Notebook arrays contain 512 idea
names and 512 signed-halfword identifiers; the photo-name work buffer is
0x2480 bytes. Guarded blocks and profile rows remain unchanged.

Markers: 261 / 17. Matched data remains 4 / 18656 because the aggregate
sections still contain reservations. `invent-state-build.log` and
`invent-state-objects.log` under `.private/nminv-r2/` pass the full PAL
verifier and 149/149 objects.

## Naturally emitted templates and tables

The existing `found[3]`, CalcTex background and clipping arrays, card-origin
pair, and picture-position pair generate their own zero templates. Six
matched switch tables and the CMenuInvent vtable also come from existing
C++. Nine separately validated steps remove their markers without changing
any function body. `at_1965` is three bytes, while the four coordinate and
clip templates are eight bytes each. Pointer casts and dummy storage are
not needed.

Markers: 254 / 12; matched data: 36 / 18656 bytes. Each
`invent-emitted-<step>-build.log` / `-objects.log` in
`.private/nminv-r2/` reports PAL OK and 149/149 objects.

## String literals

Twenty function groups now use 116 native literals, including the photo
parser paths, form/texture identifiers, Shift-JIS prompts, debug formats,
and item-menu messages. Shared strings were replaced at all unguarded
uses together. CalcTex's profile selectors and generated code remain
unchanged. Literals used only by frozen drafts remain addressable under
their retail symbols.

Markers: 138 / 12; matched data: 36 / 18656 bytes. All twenty
`invent-string-<step>-build.log` / `-objects.log` receipts in
`.private/nminv-r2/` verify PAL OK and 149/149 objects.

## Local aggregate initializers

Sixteen templates now come from their actual local aggregates: recipe
match flags, record message types, grade rows and gradation steps, cursor
coordinates, item board positions and names, gift position, blank name,
card colour, grid overlay codes, and creation/exit message arguments.
The record-message call uses the aggregate's typed array member.
The cursor seed is four integers `{10, 10, 0, 0}`, not float data.

Markers: 130 / 4; matched data: 36 / 18656 bytes. Eleven successful
`invent-templates-<step>-build.log` / `-objects.log` receipts verify PAL OK
and 149/149 objects. `at_3739` and `at_3765` require a separate scope check;
`at_2776` and `at_5642` belong to frozen drafts.

## Named initialized tables

Nineteen tables now use typed native definitions with LOCAL linkage:
icon prefixes, grade form names, cursor frames, confirmation modes,
next-mode selections, album overlays, message digit widths, four grid
dimension pairs, the 53 mutable scoop records, twelve photo-command rows,
effect colours, allocation options, model lighting, sound timings, and
Japanese name lengths. Command/confirmation IDs use the existing enums.
The scoop record order and gaps are retained, including scoop 1015 last;
its zero trailing runtime state uses ordinary aggregate initialization.
The four grid dimensions are actual two-integer arrays, passed directly
to MoveCursor instead of taking the address of scalar declarations.

Markers: 106 / 4; matched data: 36 / 18656 bytes. Twelve successful
`invent-table-<step>` and seven successful `invent-more-<step>` receipt
pairs verify PAL OK and 149/149 objects.

The three native handler-table probes and five frozen-only pointer-table
probes failed canonical data identity/layout validation. In particular,
the scoop-handler probe leaves the native `at_3509` zero template unmapped,
then reports unresolved small-data targets (340 object-check problems).
The table definitions have the correct retail extents and relocations;
no tooling change, fake use, or draft edit is made to bypass the rejection.
Their markers and literal children remain intact. The rejected natural
probes are logged as `invent-table-{scoop-tags,photo-tags,recipe-tags}` and
`invent-more-{model-assets,sound-banks,sound-waves,question-endings,newcomer-labels}`.

The confirmation buffer scope probe changes 15 linked text bytes in
IsAskExtend, beginning at the prologue, and is reverted. It adds no accepted
source changes. `invent-templates-ask-scopes-{build,objects}.log` records
that rejection.

## Final inventory verification

Native-only inlining of shared motion, stop, album and constructor-icon
literals fails identity validation with their frozen consumers retained.
The four `invent-shared-<step>-build.log` probes are reverted. Value
initialization of the two confirmation aggregates also fails matching and
is reverted (`invent-templates-ask-valueinit-build.log`). Neither probe
introduces accepted helpers or dummy locals.

Final inventory markers: 106 RODATA / 4 BSS (initially 261 / 51).
Matched data: 36 / 18656 bytes (initially 4 / 18656). Existing enum names
identify the known next-mode values. `invent-final-{build,objects}.log`
verifies PAL OK and 149/149 objects after all reverts. All guarded blocks
and headers remain identical to the checkpoint.

## Retained markers

Each row names one remaining marker; marker suffix `__DATA` is omitted.

| Symbol | Reason |
|---|---|
| `menu_scoop_str_tag` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `pic_tag` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `invent_teigi_func` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `Tb_2819` | Separately referenced interior address at the table alignment boundary; frozen consumer must remain unchanged. |
| `D_003532DF` | Separately referenced interior address at the table alignment boundary; frozen consumer must remain unchanged. |
| `at_2913` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `wavname_2960` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `NewComer_5648` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `at_1537` | Literal target of retained `menu_scoop_str_tag`. |
| `at_1655__2` | Literal target of retained `pic_tag`. |
| `at_1656__2` | Literal target of retained `pic_tag`. |
| `at_1947` | Literal target of retained `invent_teigi_func`. |
| `at_1948` | Literal target of retained `invent_teigi_func`. |
| `at_2244` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2245` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2246` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2247` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2248` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2249` | Frozen `LoadCharaCheck__11CMenuInventFv`, `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2250` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2251` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2252` | Frozen `LoadCharaCheck__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2253` | Frozen `LoadCharaCheck__11CMenuInventFv`, `MenuInventInit__FP9mgCMemoryPii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_2820` | Literal target of retained `Tb_2819`. |
| `at_2821` | Literal target of retained `Tb_2819`, `Tb_2819`, `Tb_2819`, `Tb_2819`, `Tb_2819`, `Tb_2819`. |
| `at_2848` | Literal target of retained `gobitbl_2847`. |
| `at_2849` | Literal target of retained `gobitbl_2847`. |
| `at_2929` | Literal target of retained `getfilename_2928`. |
| `at_2930` | Literal target of retained `getfilename_2928`. |
| `at_2952__2` | Literal target of retained `sndfileName_2951`. |
| `at_2953__2` | Literal target of retained `sndfileName_2951`. |
| `at_2961` | Literal target of retained `wavname_2960`. |
| `at_2962` | Literal target of retained `wavname_2960`. |
| `at_2963` | Literal target of retained `wavname_2960`. |
| `at_3113` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3114` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3115` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3116` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3117` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3118` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3119` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3120` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3121` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3122` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3123` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3124` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3125` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3126` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3127` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3128` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3129` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3130` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3131` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3132` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3133` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3134` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3135` | Frozen `IsCreateObject__11CMenuInventFii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_3138` | Jump table generated by a frozen switch; removing it requires changing its frozen body. |
| `at_4354` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4355` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4356` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4357` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4358` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4359` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4360` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4361` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4362` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4363` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4364` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4365` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4366` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4367__2` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4368__2` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4369` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4370` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4371` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4372` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4373` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4374` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4375` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4376` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4377` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4378` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4379` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_4380` | Frozen `IsAccessAlbum__11CMenuInventFv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5011` | Shared constructor/MenuInventInit literal; native-only inlining fails identity validation while frozen references remain. |
| `at_5012` | Shared constructor/MenuInventInit literal; native-only inlining fails identity validation while frozen references remain. |
| `at_5013` | Shared constructor/MenuInventInit literal; native-only inlining fails identity validation while frozen references remain. |
| `at_5014` | Frozen `MenuInventInit__FP9mgCMemoryPii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5015` | Frozen `MenuInventInit__FP9mgCMemoryPii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5016` | Frozen `MenuInventInit__FP9mgCMemoryPii`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5649` | Literal target of retained `NewComer_5648`. |
| `at_5650` | Literal target of retained `NewComer_5648`, `NewComer_5648`. |
| `at_5651` | Literal target of retained `NewComer_5648`. |
| `at_5652` | Literal target of retained `NewComer_5648`. |
| `at_5653` | Literal target of retained `NewComer_5648`. |
| `at_5654` | Literal target of retained `NewComer_5648`. |
| `at_5742` | Frozen `MenuInventKey__Fv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5743` | Frozen `MenuInventKey__Fv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5744` | Frozen `MenuInventKey__Fv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5745` | Frozen `MenuInventKey__Fv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5746` | Frozen `MenuInventKey__Fv`: initializer/literal/switch cannot be emitted by editing its body. |
| `at_5747` | Jump table generated by a frozen switch; removing it requires changing its frozen body. |
| `gobitbl_2847` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `getfilename_2928` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `sndfileName_2951` | Native pointer-table identity probe fails; retain exact table and literal targets. |
| `at_3739` (BSS) | Confirmation buffer template; natural scope/value-initialization probes fail matching. |
| `at_3765` (BSS) | Confirmation buffer template; natural scope/value-initialization probes fail matching. |
| `at_5642` (BSS) | Frozen local zero template. |
| `at_2776` (BSS) | Frozen local zero template. |

## Pointer-table identity anchor

The scoop-handler table is now native and matches. Its earlier failure
was a temporary projected-name collision, not a table-layout problem:
adding the handler literal shifts the PushKey switch to compiler name
`@3509` (48-byte RODATA). Before that switch is identified as `at_5560`,
its projected name blocks the eight-byte gift template's `at_3509`
identity. One missing BSS piece then causes the canonical checker to
leave that entire section run unresolved.

Retaining only `INCLUDE_BSS(at_3509, 0x8)` anchors the gift template while
its natural local initializer remains unchanged. No table type, handler
body, declaration placement or tool is altered to bypass the problem.
The scoop table and literal remove two RODATA markers. Current inventory
checkpoint: 104 / 5, matched data 36 / 18656. The raw native switch and
fixed object are saved privately; `invent-pointer-anchor-{build,objects}.log`
verifies PAL OK and 149/149 objects.
