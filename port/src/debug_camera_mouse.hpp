#pragma once

class CCameraControl;

// Live-run observer only; does not drain shared mouse motion/wheel or open a device.
void DebugCameraMouseInstall();
void DebugCameraMouseBeforeStep(CCameraControl &camera, int frames);

// The mod overlay supplies its complete input ownership, including close settling.
// Install once from its init/open path; the query must not consume input or mutate camera state.
void DebugCameraMouseSetOverlayInputQuery(bool (*query)());
