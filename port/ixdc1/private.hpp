#pragma once

/* DC1's port.h and standard/SCE headers are parsed at global scope, before
 * retail headers enter ixdc1. This target never inherits ct_options. */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "ixion_dc1_toan.h"

// The small VU0 closure is private too: never bind against DC2's SCE shim.
#define sceVu0CopyVector ixdc1_sceVu0CopyVector
#define sceVu0ScaleVectorXYZ ixdc1_sceVu0ScaleVectorXYZ
#define sceVu0UnitMatrix ixdc1_sceVu0UnitMatrix
#define sceVu0RotMatrixX ixdc1_sceVu0RotMatrixX
#define sceVu0RotMatrixY ixdc1_sceVu0RotMatrixY
#define sceVu0MulMatrix ixdc1_sceVu0MulMatrix
#define sceVu0ApplyMatrix ixdc1_sceVu0ApplyMatrix
// Xiao R1: BattleActionPlay_Jinn's lock-on aim (dun/gameloop.cpp:7721).
#define sceVu0Normalize ixdc1_sceVu0Normalize
#include <libvu0.h>
#include <libdma.h>
#include <libgraph.h>
#include <libpkt.h>

// This unused allocator header declares global placement operator new, which
// C++ forbids inside a namespace. Parse it globally once; no allocator body or
// instance is part of the transplant. The post-link gate checks that closure.
#define __ct__14CDataAlloc2_1_Fi ixdc1___ct__14CDataAlloc2_1_Fi
#include "dataalloc.hpp"

// DC1's Windows force-include maps u_long to u64 for GS bitfields. Those SCE
// declarations are now complete; game records retain DC1's raw 32-bit long.
#undef u_long
#define u_long u32
static_assert(sizeof(u_long) == 4);

// C linkage ignores the namespace. Rename every C-linkage global exposed by
// this header closure, including declarations that the handler doesn't use.
#define CharaMain ixdc1_CharaMain
#define BtActStatus ixdc1_BtActStatus
#define BtEventInfo ixdc1_BtEventInfo
#define DebugStatus ixdc1_DebugStatus
#define DngMessMan ixdc1_DngMessMan
#define StealItem ixdc1_StealItem
#define ItemVolumeStep ixdc1_ItemVolumeStep
#define OzumondShotEffect ixdc1_OzumondShotEffect
#define GeneratedCharacterVtable ixdc1_GeneratedCharacterVtable
#define StatusColor ixdc1_StatusColor
#define WorkBuffer__2 ixdc1_WorkBuffer__2
#define FrameObjAnimCnt ixdc1_FrameObjAnimCnt
#define mgRenderInfo ixdc1_mgRenderInfo

namespace ixdc1 {
#include "btactstatus.hpp"
#include "btmisc.hpp"
#include "character.hpp"
#include "clothread.hpp"
#include "collisiondata.hpp"
#include "dataread.hpp"
#include "dun/gameloop.hpp"
#include "frame.hpp"
#include "framevu1.hpp"
#include "gamepad.hpp"
#include "itemdata.hpp"
#include "motionmodel.hpp"
#include "shot_effect.hpp"
#include "shot_freefuncs.hpp"
#include "shot_utils.hpp"
#include "snd.hpp"
#include "userstatus.hpp"
#include "weaponeffect.hpp"
void ToanKey_On();
void ToanKey_Play();
void GoroKey_On();
void GoroKey_Play();
void UngagaKey_On();
void UngagaKey_Play();
}
