# Native object frame vtable

The existing unit notes establish the base `CObject` and derived `CObjectFrame`
virtual layouts. The already matched `CObjectFrame` methods now emit its vtable
without an assembly marker. No function body or shared type changes.

The base `CObject` marker remains: deleting it fails to link because the native
object does not emit `__vt__7CObject`, and constructors/copies in many other
units require it. Changing shared virtual declarations or introducing a dummy
object solely to force vtable emission is outside this unit's migration scope.
Receipt: `.private/dataB-r3/object-vtable-build.log`. The existing frame vtable
is emitted naturally; no manual vtable writes are introduced.


Markers: ROData **2 → 1**, BSS **0 → 0**.
Objdiff after the required refresh: matched_data **0 → 0** / total_data **244 → 244**.

Validation: `SCES_511.90: OK`, **149/149** complete objects including resolved relocations. No functions promoted. Receipts: `.private/dataB-r3/object-final-build.log`, `object-final-objects.log`, `object-progress.log`.
