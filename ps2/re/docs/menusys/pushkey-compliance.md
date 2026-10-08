# PushKey input compliance — October 8, 2026

`CMenuItemInfo::PushKey(int pad, int trigger)` dispatches held navigation and
new action buttons for item browsing, equipment, weapon tuning and spectrum
transformation. Its PAL address is `0x24A890`, its instruction size is `0x1B94`,
and its manifest extent including zero padding is `0x1BA0` (1,768 words).
The dispatcher and dependency layouts are documented in [pushkey.md](pushkey.md)
and [notes.md](notes.md).

**PushKey remains promoted.** The compliant body has zero differing words,
and the complete `menusys` object passes byte and resolved-relocation checks.
Only this function body and this note change from lane base `fd05c9d4b8e7a54b032ffbae7f177c91ea61b030`.
No header, assembly, compiler-profile or other function change is needed.

## Over-limit message name

The disputed store belongs to the one-entry item-name list passed to
`CDC2Mes::SetMsgItemNo(char **, int)`, rather than a floating-point game value:

- At `0x24B6D8`, retail loads four zero-initialized bytes from `at_7021`
  (`0x37DDD0`, a four-byte `.sbss` piece) through `f0`.
- At `0x24B6DC`, it takes the address of the stack slot at `sp + 0x204`.
  The `swc1` at `0x24B6E8` copies those bytes into that slot.
- The `sw` at `0x24B6F4` replaces the slot with the `char *` returned by
  `GetItemMessage(over_item_no)`.
- At `0x24B700`, that same slot's address becomes the `char **` argument;
  the following call supplies a count of one. The overload copies the string
  into the message's name substitutions, as documented in
  [menucls1/notes.md](../menucls1/notes.md).

The retained source uses `char *item_name[1] = {NULL}`, fills element zero,
and passes the array directly. A canonical MWCC compile of this typed
aggregate reproduces the floating load/store used for its initializer copy.
Thus those instructions do not require a float type-pun into pointer storage.
The body no longer references the old float declaration of `at_7021` outside
the permitted edit range. The ordinary placeholder-binding stage supplies
the existing retail initializer storage for the compiler's anonymous copy.

## Named addresses and output lifetimes

The ridepod selection now uses `&MenuUserParam.robo->parts[robo_slot]`.
The spectrumisation cursor update writes `MenuCommonInfo->top_line`, the
documented member at `0x74`, rather than indexing past `cursor` at `0x70`.

The main-character model, skin and outline loading addresses use
`MainCharaReadStack.stGetTop()`. The fusion sound load likewise uses
`load_stack->stGetTop()`. The existing `mgCMemory` method returns
`&stack[stack_used]`, where the buffer elements are sixteen-byte quadwords.
All six address-expression replacements preserve the exact function bytes
and relocations independently of the initializer correction.

`file_size` is an `int` output for the model, skin and outline file loads and
now lives in their model-reload branch. `fusion_file_size` is the corresponding
output for the fusion sound load and lives in that branch. With the typed
name array, these lifetimes preserve retail's stack slots: name at `0x204`,
model-load size at `0x208`, and fusion-load size at `0x20C`.

## Experiments and validation

Every compile uses MWCC 3.0-011126, canonical `-O3,p` flags and the production
mwccgap/Satan's Fiddle wrapper in `chronicletwo_dev:sf-d8bf13c`, followed by
`fixup_sections.sh`. The required `decompile.sh` attempt reproduces the
documented m2c jump-table failure at assembly line 530; generated assembly
and tools are unchanged. Retail instructions and the typed compiler output
resolve the initializer question above.

Receipts are under `.private/pushkey-compliance/` in the assigned worktree:

| Trial | Word differences | Complete object |
|---|---:|---|
| Fresh baseline (`baseline-score.json`) | 0/1768 | Pass |
| Named address replacements (`address-fixes/`) | 0/1768 | Pass |
| Branch-local name array with function-scope sizes (`pointer-array/`) | 11/1768 | PushKey fails |
| Name array and scoped size outputs (`scoped-sizes/`) | 0/1768 | Pass |
| Final ordinary build (`final-score.json`) | 0/1768 | Pass |

The eleven intermediate differences are only stack offsets: three name-slot
references, six model-size references and two fusion-size references.
No extra operation, dummy local, helper, type-pun or profile row is retained.
The final `menusys` object contains `0x1B0DC` bytes and 5,872 relocations.

`baseline-build.log` and `final-build.log` both exit one at the same existing
PAL verification failure: `.text` differs by `0x26` bytes, first at
`0x0015C5AD` in `nd_meswin::DrawMesWin`. The other nine file-backed sections
pass and `.bss` ends at `0x01F64A00`. `baseline-objects.log` and
`final-objects.log` both report 147/149 passing units; the only failures are
`nd_meswin::DrawMesWin` and `actscript::_SHOT`, at the same addresses.

`final-hash-compare.json` confirms that only `menusys.cpp.o` changes among
149 game-object file hashes; all 148 other objects are byte-identical.
The address-replacement object is byte-identical to the original baseline
object, allowing a private baseline relink whose hash reproduces the original
image. `final-image-section-compare.json` compares that image to the final
build: every allocated section is identical. Only the non-allocated
`.relmain`, `.strtab` and `.symtab` sections change, so the complete ELF file
hash changes without a loaded-byte change.

`baseline-progress.log`, `final-progress.log`, `baseline-coverage.log` and
`final-coverage.log` refresh source-only objdiff after the expected build
failure. Coverage remains 6,741 matched, 119 guarded drafts, 10 asm-only and
2 fuzzy functions out of 6,872; `menusys` remains 154 matched and 11 guarded.
No new function is promoted and there is no remaining PushKey blocker or
proposal for a nonowned file.
