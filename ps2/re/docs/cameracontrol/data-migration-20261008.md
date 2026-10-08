# Native camera control data

The established unit notes identify the homogeneous reference-vector initializer,
rotation zero template and camera vtable. `SetCheckRef(x, y, z)` now uses the
local float initializer `{0, 0, 0, 1}` rather than an external byte template and
quadword type-pun. The existing virtual methods emit the camera-control vtable.
The SF-calibrated constructor body is unchanged.

The rotate zero-template marker remains. A natural local float array initialized
at the existing copy point differs by **8/28 words** (native size 0x70);
explicit 16-byte alignment has the same result. The inherited matched body is
restored. Receipts: `cameracontrol-rotate-worddiff.log` and
`cameracontrol-rotate-aligned-build.log` under `.private/dataB-r3/`.


Markers: ROData **2 → 0**, BSS **1 → 1**.
Objdiff after the required refresh: matched_data **0 → 52** / total_data **68 → 68**.

Validation: `SCES_511.90: OK`, **149/149** complete objects including resolved relocations. No functions promoted. Receipts: `.private/dataB-r3/cameracontrol-final-build.log`, `cameracontrol-final-objects.log`, `cameracontrol-progress.log`.
