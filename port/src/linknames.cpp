// Names the PS2 link supplies and the port's link otherwise lacks.

// cameracontrol.cpp calls the assignment its header declares for it alone; MWCC emitted the implicit
// one out of line in editloop.cpp, which no clang object defines.
#define CAMERA_CONTROL_USE_RETAIL_ASSIGNMENT
#include "cameracontrol.hpp"

CameraCtrlParam &CameraCtrlParam::operator=(const CameraCtrlParam &source) = default;
