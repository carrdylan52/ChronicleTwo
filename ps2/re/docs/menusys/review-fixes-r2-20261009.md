# October 9 round-two review fixes

## Native local statics (finding 22)

Each of the ten hand-written initialization guards is independently replaced
by a natural local static at the guard's original position. The declared
type, initial value and state accesses remain the same. MWCC supplies the
initialization latch and the current object postprocessor binds that local
state to its retail storage.

| State | Type / initializer | Consumer |
| --- | --- | --- |
| `cmd_counter` | `s8`, `0` | `CBaseMenuClass::MenuItemCommandSelect` |
| `sndflag` | `s8`, `0` | `CBaseMenuClass::IsSpectolFusion` |
| `count_time` | `s8`, `0` | `MenuFormUpdataAttachInfo` |
| `checkmoveFlag` | `s8`, `0` | `CMenuItemInfo::CalcTex` |
| `fusion_blinkcnt` | `s8`, `0` | `CMenuItemInfo::PushKey` |
| `diffent_weapon_dispflag` | `s8`, `0` | `CMenuItemInfo::PushKey` |
| `counter` | `float`, `0.0f` | `MenuPosFormValueSetCharaRobo` |
| `count` | `s8`, `0` | `MenuWeaponStatusInfoFormSet` |
| `old_viewmode` | `int`, `0` | `MenuItemKey` |
| `old_chrid` | `int`, `0` | `MenuItemKey` |

All ten independent complete-unit checks pass. The matching build still
selects the existing assembly gaps for the four constructor-dependent
functions; their draft-only state is outside this ten-site cleanup.

## Key and swap-result arrays (finding 22)

`MenuListKeyCheck` initializes its two-by-two direction-key array directly
with the named key bits. Its separate two-by-two wrap array starts at zero;
the existing count-minus-one assignments remain after initialization.
The `KeyPairTable` wrapper and both manually named templates are unnecessary.
Both plain-array substitutions independently pass the complete-unit check.

`MenuDataSwap` uses a two-byte `s8` result array initialized with
`MENU_SWAP_RESULT_FAILED` and `MENU_SWAP_RESULT_DESTINATION_OCCUPIED`.
The source-result lookup precedes array initialization, then replaces the
first entry before the destination-result lookup. This order reproduces
retail's loads and stores. The constant plain initializer independently
passes, so `MenuSwapResultTable` and its hand-defined template are unnecessary.

The combined ten-static and three-array candidate passes all `0x1B0B0`
allocated bytes and 5,996 resolved relocations. Individual and combined
compiler/checker results are saved under
`.private/fixes-r2b/menusys-probes/`; all thirteen substitutions are accepted.

The actual-source full build verifies `SCES_511.90: OK` and 149/149 complete
objects with the unchanged compiler profile. No function guard changes.
