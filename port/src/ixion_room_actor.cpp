#include "common.h"

#include "character.hpp"
#include "ixion_room.hpp"
#include "ixion_toan_body.hpp"

#include <algorithm>

// CCharacter2::SetPosition is the final native action-position write. The
// room adapter commits authored markers here after native movement/collision
// has completed its update, then immediately returns ownership to that code.
PORT_OVERRIDE void CCharacter2::SetPosition(float *position) {
    float applied[4]{};
    std::copy_n(position,4,applied);
    IxionRoomRedirectPosition(this,applied);
    mgCObject::SetPosition(applied);
    IxionRoomActionPositionBoundary(this);
    IxionToanBodyTrace(this);
}
