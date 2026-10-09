# menusys data migration (2026-10-08)

Lane base `91af9829` (checkpoint m24), canonical MWCC 3.0-011126 flags,
production Satan's Fiddle profile, image `chronicletwo_dev:sf-63f7a9e`.
The warm build passes `SCES_511.90: OK` and **149/149 objects**.
The refreshed baseline has **360 INCLUDE_RODATA / 126 INCLUDE_BSS**,
**4 / 10,332 matched_data**, and **161 / 165 native functions**.

The four guarded drafts are preserved verbatim, and the SF-controlled
`MenuItemDebugDraw` retains byte-identical code under the unchanged profile. No function promotion, profile change or toolchain change
is part of this migration. Every accepted step uses the normal full build,
the byte and resolved-relocation object checker, protected-body comparison,
and SHA-256 comparison of all 149 linked game objects. Only menusys's object
metadata may change; every unowned object hash remains equal to the warm build.
Receipts are under `.private/dataD-r2/`.

## Object extents and existing types

Retail declared sizes are distinct from the splitter's padded reservation
lengths. Native definitions use the actual object extents; the existing
postprocessor supplies only the verified retail alignment tail. Important
extents are `ITEMCMD_RET_PARA` 0x14, `MenuCharaReadBuffers` 0xC,
`MENU_ITEM_CURSOR_INFO` 0xC, `BUILDUP_WEAPON_INFO` 0x44,
`CLevelUpEffectManager` 0x190, and `BuildUpNameXY` 0xC.
The saved fusion parameters have twelve signed shorts, and both build-up
part-pointer arrays have twelve entries. Their old ten-element source-only
extern declarations understate the retail symbols.

`CLevelUpEffectManager` and its eight effect records have no constructor or
destructor. Their fields are scalar values, pointers and vector arrays, so
native uninitialized storage does not introduce another static initializer.
The existing header types and global declarations remain source-compatible.

The same size distinction applies to initialized tables: the ridepod equipment
mapping has four signed bytes, confirm/cancel mappings are two rows of two
integers, and active-weapon part names have three rows of two pointers.
The palette-effect table is two rows of five integers describing RGB, pulse
count and duration. It does not hold motion identifiers.

## Native zero-initializer templates

The matched source already creates the following zero templates. Removing a
marker exposes that native compiler object; obsolete external declarations do
not establish a type for these anonymous initializers.

| Retail symbol | Native consumer and purpose |
| --- | --- |
| `at_1385__2` | `SetItemCmdMsgPos`, initial message screen coordinates. |
| `at_2345__2`, `at_2346__2` | `MenuGlidKeyCheck`, movement and step arrays. |
| `at_2564` | `CMenuKeyFunc::MenuPosStep`, initial cursor coordinates. |
| `at_3407` | `CMenuKeyFunc::CheckAnalogKey`, four analog input components. |
| `at_3791__2`, `at_3792__2` | `MenuPosFormValueSetWeapon`, gauge rates and status values. |
| `at_4365__2`, `at_4406`, `at_4423__2`, `at_4510` | `CMenuItemInfo::ItemCmdAfter`, name, movement and value arrays. |
| `at_5026` | `CommonSetMoveItemClass`, item-movement pointers. |
| `at_5532`, `at_5573` | `CMenuItemInfo::CalcTex`, monster values and name substitution. |
| `at_5769`, `at_5774`, `at_5782`, `at_5829` | `CMenuItemInfo::CalcCursorPosition`, position, offset and form arrays. |
| `at_7021` | `CMenuItemInfo::PushKey`, one null item-name pointer. |
| `at_7650`, `at_7688` | `MenuWeaponBuildUpDraw`, name positions and monster-name substitutions. |
| `at_9093` | `CItemSelect::Draw`, selection display coordinates. |

`at_7021` is a pointer-array initializer, despite the obsolete float extern;
its actual load/store through an FPR does not imply a floating-point value.
The existing PushKey analysis establishes that typed initializer.

## Named menu state

The ordinary state objects use the types established by the matched consumers
and existing headers: controller/model/message/form/texture pointers, signed
short quantities and positions, byte visibility/initialization flags, floating
animation counters and vectors, and the documented cursor/build-up records.
Retail LOCAL objects retain file-local linkage; existing public globals retain
their header-compatible declarations. Primitive persistent counters and their
one-byte initialization latches preserve the existing runtime initialization
and exact retail symbol names.

## Validation and retained markers

The final measurements and complete retained-marker inventory follow the
completed migration checks below.

### Zero-template checkpoint

All twenty-two zero-template removals pass independently. Markers are
**360 RODATA / 104 BSS**, with **4 / 10,332 matched_data**. PAL remains OK,
all 149 objects pass, and the four guards and compiler profile are unchanged.
The step ledger is `.private/dataD-r2/generated-bss-ledger.log`; per-template
receipts use `generated-<symbol>-{build,objects,metrics}.log`, with the first
check recorded as `generated-at_1385`.

### Named-state checkpoint

All sixty-five ordinary named reservations have native, documented C++
definitions. The native sizes and bindings pass PAL and all 149 object checks,
including the twelve-element fusion and form-pointer arrays. The state groups
are recorded by `named-bss-ledger.log` and
`bss-state-{1,2,3,4}-{build,objects,metrics}.log`.
Markers are **360 RODATA / 39 BSS** and data credit remains
**4 / 10,332** while the remaining template pieces keep aggregate sections
incomplete. The guarded drafts and unowned object hashes remain unchanged.

### Persistent-state checkpoint

All twenty-nine counter, pointer and latch reservations have native file-local
primitive definitions under their exact retail names. The existing runtime
initialization is unchanged, including the guarded effect/debug counters and
the saved preview fields whose stores must remain present. No artificial read,
volatile qualifier or additional constructor is needed.

Markers are **360 RODATA / 10 BSS**, with **4 / 10,332 matched_data**.
All thirteen counter groups pass PAL, 149/149 objects, protected guards and
unowned hashes independently. Receipts are `persistent-bss-ledger.log` and
`bss-<group>-{build,objects,metrics}.log` under `.private/dataD-r2/`.

### Natural local initializer checkpoint

The spectrumisation name substitution is a one-pointer automatic array, the
fusion name substitution has two pointer slots, and the breakdown values are
four integers initialized to zero. Their native aggregate templates replace
`at_1545`, `at_1685` and `at_1557`. The accepted `KeyPairTable` copy convention
retains its typed, file-local zero wrap-target object at `at_2333__3`.
The old name/position/breakdown wrapper types and the `MenuEffect` declaration
macro are unnecessary and absent.

`MenuEquipCameraSetEnv` uses two automatic four-float arrays initialized to
`{0, 0, 0, 1}`. They are declared after a camera attachment is found, followed
by the name and lookup buffers, preserving the original initializer execution
point and stack allocation order. This replaces the two external aggregate
load casts and both `at_3771`/`at_3772` markers. A separate ordinary `memcpy`
trial for the existing output-vector copies fails PAL layout verification and
is reverted; those unrelated output operations retain their validated source.
Its negative receipt is `copy-camera-output-build.log`.

All accepted forms and the final restored source pass PAL and 149/149 objects.
The complete native `.bss` section now receives data credit: markers are
**358 RODATA / 6 BSS**, and **2,884 / 10,332 matched_data**.
Receipts are `natural-copy-ledger.log`, `copy-<group>-{build,objects,metrics}.log`,
and `copy-final-{build,objects,metrics}.log`. No guarded draft changes.
