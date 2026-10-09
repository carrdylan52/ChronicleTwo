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

## Retained markers

The initialized-data markers are pending the following migration topics.
