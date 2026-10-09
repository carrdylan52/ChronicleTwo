# Event function data migration — 2026-10-08

## Baseline

Checkpoint `3221b488`: 116 `INCLUDE_RODATA` and 22 `INCLUDE_BSS` markers;
823/827 native functions; refreshed data comparison 4/235,364 bytes.
The warm PAL build and canonical checker pass, including all 149 objects.

## Event storage

All 22 zero-storage reservations have typed native definitions. The existing
header describes the event state, flags/counters, mouth-animation names,
particle buffers and camera/object sequence entries. Their exact extents are
unchanged. The two event sound buffers hold 0x801 and 0x141 quadwords;
the external-command dispatch array contains 0x5DC typed function pointers.
The script-argument pointer, effect manager, sword trail, sound buffers and
dispatch array have file-local linkage. `EventEffectScript` keeps its retail
symbol reachable for the guarded `_ESM_INITIALIZE` assembly body.
No guarded function or draft is edited.

Receipts: `.private/dataA-r3/event-storage-{global,local}-{build,objects,hashes}.log`.
Both stages pass PAL and 149/149 objects; every unowned object hash is unchanged.

## Local language extensions

`FileNameConvLanguage` initializes its four typed extension pointers at their
use and inlines the two formatting strings. The three nonempty extensions
are `txt`, `img`, and `stb`. The unused fourth pointer remains `at_1083`, the
shared empty-string symbol. Inlining that pointer produces an anonymous
empty literal that the stock mapper cannot identify through the initializer
relocation; the native extension template and literal stay unnamed. Retaining
the existing empty symbol lets the complete native pointer template match.
This removes six initialized markers without altering function instructions.

Receipts: `.private/dataA-r3/event-extensions-empty-symbol-{build,objects,hashes}.log`;
the failed fully inline form is recorded in `event-extensions-failure.log`.

## NPC and voice-pack initializer templates

The three local aggregates now use direct initializers: twelve NPC positions
and facing angles, 25 three-column NPC training rows, and 164 voice-pack
lookup rows. `_GET_TRAIN_NPC_POS` selects x/y/z/yaw by zero-based row, with y
forced to zero in map 120. `_GET_NPC_TRAIN_ETC` selects the requested column
and the one-based NPC row. `VpkFileNameFromVoiceNo` matches group/kind and
formats the selected resource id/subresource pair.
The native templates preserve their .data contents and local-copy instructions;
the integer training template's four-byte trailing alignment is supplied by
the existing bounded padding policy.

Receipts: `.private/dataA-r3/event-{train-pos,train-etc,voice-pack}-{build,objects,hashes}.log`.
Each topic passes PAL, all 149 objects and every unowned object hash.

## Hit-effect direction

`_HIT_EFFECT` initializes its default upward vector at the existing copy site,
after reading the position. The `{0, 1, 0, 1}` local array replaces the external
template and quadword type-pun. Its bytes and all caller instructions match.
Receipt: `.private/dataA-r3/event-hit-direction-corrected-{build,objects,hashes}.log`.

## Argument script dispatch definitions

The argument interpreter's three records are native mutable `EventScriptFunc`
rows, with typed `_DATA` and `_ID_OFFSET` function pointers and the null sentinel.
`EventArgumentCommand` names the two command numbers. The shared row type has
its documented eight-byte layout in `event_func.hpp`; both handlers receive
file-local declarations. Native padding retains the original .data piece.
Receipt: `.private/dataA-r3/event-arg-dispatch-{build,objects,hashes}.log`.

## External callback result types

`CRunScript::ext` tests each callback's integer result and diagnoses zero.
Twenty-two callbacks now explicitly return their final dependency call's
existing integer status; their previous void declarations hid that result:

`_FINISH`, `_IMG_SET_DRAW`, `_IMG_SET_GET`, `_IMG_SET_PUT`, `_IMG_SET_MOVE`, `_IMG_SET_FADE`, `_IMG_SET_COLOR`, `_GEORAMA_FUNC`, `_EOH_SET_STEP`, `_EOH_SET_SHOW`, `_EOH_SET_FRAME_SHOW`, `_EOH_SET_SHADOW`, `_EOH_SET_FOOT_SOUND_ID`, `_EOH_SET_FRAME_STATUS`, `_EOH_SET_SOUND_ID`, `_EOH_SET_FADE_FLAG`, `_EOH_RESET_DA_POSITION`, `_EOH_SET_SHADOW_FRAME_STATUS`, `_EOH_SYNC_GEOSTONE`, `_EOH_NORMAL_DRIVE`, `_EOH_SET_FOOT_SE_ID`, `_MT_TEST`.

`_EOH_GET_POS`, `_EOH_GET_ROT`, `_EOH_GET_SHOW`, and `_EOH_GET_FRAME_POS`
return the handle lookup's integer result. Their stack-output calls preserve
v0 in retail and in the native object. A named result expresses that live
status, with no extra instructions. The frame-position getter needs the
positive `if (result != 0)` body and one common return: the early-return form
shortens the native body by four bytes and changes branch/call placement.
Every retained result correction passes the whole-object and PAL checks.
No callback pointer cast or undefined missing return is used.

m2c receipts: `.private/dataA-r3/_EOH_GET_{POS,ROT,SHOW,FRAME_POS}-m2c.txt`
and `_FINISH-m2c.txt`. Canonical receipts are
`event-results-wrappers`, `event-results-_EOH_GET_{POS,ROT,SHOW}`, and
`event-results-frame-branch`, each with build/object/hash logs. The rejected
frame early-return form is in `event-results-_EOH_GET_FRAME_POS-failure.log`.

## Main event dispatch definitions

The main table has 696 typed handlers followed by `{NULL, EVENT_EXT_END}`.
`EventExternalCommand` names every retained retail command number, including
sparse and out-of-order identifiers. `ext_func_info[697]` is mutable file-local
.data storage; its 0x15C8 declared size and eight-byte zero tail match retail.
Three source declarations make the guarded `_COPY_CHARA`, `_ESM_INITIALIZE`,
and `_COPY_MONS2SCNCHR` assembly symbols available to these typed pointer
initializers. Their guarded bodies and drafts remain byte-for-byte unchanged.
Receipt: `.private/dataA-r3/event-dispatch-table-prototypes-{build,objects,hashes}.log`.
The source-only table retains references to assembly handlers without claiming
their code as native coverage.

## Conversation camera static offsets

`_SET_TALK_CAMERA` now owns the original mutable `float vv[3][4]` static
initializer. It transforms `vv[1]`; the other two retail rows remain in the
48-byte object. The external `vv_3333` declaration and flattened indexing
are removed. Keeping the marker passes the complete object and PAL.
Removing it also passes PAL, but the canonical checker reports an unnamed
.data piece: the compiler emits `vv_3159`, and the stock mapper cannot assign
the initialized local static's retail identity. The `vv_3333` marker therefore
remains until the queued local-storage tooling supports this data family.

Receipts: `.private/dataA-r3/event-talk-camera-{static,restored}-{build,objects,hashes}.log`;
`event-talk-camera-unmarked-failure.log` records the naming-only blocker.

## Inline event argument script diagnostics

The following literals are inline at their native uses: `at_1245`, `at_1246`, `at_1333`, `at_1346__2`, `at_1357__3`.

Each datum has a separate full PAL/object/hash receipt under
`.private/dataA-r3/event-string-<symbol>-{build,objects,hashes}.log`.
Every accepted form preserves instruction bytes, resolved addresses and all
unowned object hashes. Shift-JIS characters use hexadecimal byte escapes.

## Retained markers

The initialized-data markers are pending the following migration topics.
