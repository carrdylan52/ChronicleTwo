# menucommon data migration (October 8, 2026)

Baseline: `63f7a9e5`, 186 `INCLUDE_RODATA` markers, 22 `INCLUDE_BSS`
markers, 0/3,816 matched initialized data bytes. All 129 functions are native
and exact. This work preserves the existing public header declarations.

## Uninitialized menu state

All 22 reservations are native definitions. The three public globals retain
their header types: `MenuSePlayUsedFlag` is `u8`, `MenuSpiStack` is
`mgCMemory *`, and `MenuCommandAnalyzeInfo` is `MENU_COMMAND_ANALYZE_INFO`
(size 0x68). The other definitions have internal linkage, matching retail.

| State | Type and purpose |
| --- | --- |
| `SndPortVol_Ob`, `SndPortVol_Base`, `SndPortVol_Event`, `SndPortVol_Env` | `float`, saved sound-port/environment volumes |
| `SndPortCheck_EventPort` | `int`, whether to restore event-port volume |
| `MenuTexPosNo`, `MenuTexPosNo_local` | `u16`, first texture slot and block offset |
| `menu_analyze_texblock` | `short`, current texture block |
| `Menu_Target_No`, `Menu_Target_No_local` | `u16`, first miscellaneous-information slot and block offset |
| `menu_formPt` | `CMenuPosDataForm *`, current form |
| `menu_form_part` | `MENUFORMPARTS_TYPE *`, current part |
| `menu_parts_effect_ptr` | `MENU_PARTS_EFFECT_STRUCT1 *`, next effect entry |
| `menu_analyze_formno`, `menu_analyze_formno_offset` | `short`, selected form and form-index offset |
| `menu_form_partsno` | `int`, next part index |
| `menu_spi_form_action_info` | `MENU_FORM_ACTION *`, next action entry |
| `SpiMenuExeCommandFlag` | `u8`, selected command-block execution flag |
| `MenuSpiTextureName` | `char[0x20]`, name copied by `_MENU_TEXNAME` and consumed by `_MENU_TEXDATA` |

`MenuSpiTextureName` is a character buffer, so its `strcpy` and `mgCopyString`
uses need no cast. The scalar/object sizes come from the existing documented
layout and the retail symbols; reservation extents include alignment gaps.
The existing postprocessor supplies those gaps without expanding the C++
types. In particular, the flag has size 1 rather than its four-byte reservation,
and the command object has size 0x68 rather than its 0x70 reservation.

Validation: `.private/dataB/receipts/menucommon-bss-final-build.log` records
`SCES_511.90: OK`; `menucommon-bss-final-objects.log` records 149/149 objects.
`menucommon-bss-changed-objects.json` lists only `menucommon.cpp.o`; the other
148 file hashes equal the warm-built baseline. Refreshed objdiff keeps all
129 functions exact. The marker counts become 186/0; initialized-data coverage
remains 0/3,816 because BSS does not contribute to that measurement.
