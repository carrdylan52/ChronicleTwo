#include "common.h"

#include "dng_main.hpp"
#include "ixion_room.hpp"
#include "ixion_room_arena.hpp"
#include "ixion_room_logging.hpp"
#include "mainloop.hpp"
#include "map.hpp"
#include "mglib.hpp"
#include "mglib_port.hpp"
#include "savedatadungeon.hpp"

#include <algorithm>
#include <cstdlib>

namespace {

void LogRoomGameplay(int dungeon,int floor) {
    // Qualification knob: DC2_IXION_ROOM_LOG_EVERY=N, floored at ten frames.
    static const int every=[] {
        const char *text=std::getenv("DC2_IXION_ROOM_LOG_EVERY");
        const int value=text!=nullptr?std::atoi(text):60;
        return IxionRoomLogInterval(value);
    }();
    static int last_frame=-60;
    const int frame=mgPortFrameCount();
    if (frame-last_frame<every || MainChara__2==nullptr) return;
    last_frame=frame;
    float player[4]={}; MainChara__2->GetPosition(player);
    const int character=DngUserData!=nullptr?DngUserData->GetActiveChrNo():-1;
    COMMON_GAGE *gage=DngUserData!=nullptr?DngUserData->GetCharaHpGage(character):nullptr;
    std::fprintf(stderr,"ixion room gameplay frame=%d player=(%.3f,%.3f,%.3f) player_hp=%.1f/%.1f live_native=%d",
        frame,player[0],player[1],player[2],gage?gage->now:-1.0f,gage?gage->max:-1.0f,
        IxionRoomRuntimeEnemyCount());
    if (CActiveMonster *enemy=IxionRoomRuntimeEnemy()) {
        float pos[4]={}; enemy->GetPosition(pos);
        std::fprintf(stderr," enemy=%d state=%d pos=(%.3f,%.3f,%.3f) hp=%d/%d target_dist=%.3f",
            enemy->monster_id,enemy->state,pos[0],pos[1],pos[2],enemy->life,enemy->max_life,enemy->target_dist);
    }
    std::fputc('\n',stderr);
    (void)dungeon;
    (void)floor;
}

} // namespace

PORT_OVERRIDE int CMap::GetColPoly(CCPoly *polygons, mgVu0FBOX &box, int max) {
    if (polygons == nullptr || max <= 0) {
        return 0;
    }

    IxionRoomObserveDungeonLoop();
    if (GetNowLoopNo() == LOOP_DUNGEON && DngSaveDataDungeon != nullptr && DngMainScene != nullptr &&
        DngMainScene->active_map==0 && DngMainScene->GetMap(0)==this) {
        const int dungeon = DngSaveDataDungeon->stage_id;
        if (dungeon >= 0 && dungeon < SAVE_DUNGEON_NUM) {
            const int floor = DngSaveDataDungeon->floor_id[dungeon];
            const int imported = IxionRoomAppendRuntimeCollision(polygons,max,box.min,box.max,dungeon,floor);
            if (IxionRoomRuntimeOwnedBy(DngMainScene,this)) {
                static int collision_receipts = 0;
                if (collision_receipts < 3) {
                    ++collision_receipts;
                    std::fprintf(stderr,"ixion room collision query %d: donor=%d native-suppressed=1 capacity=%d\n",
                                 collision_receipts,imported,max);
                }
                return imported;
            }
        }
    }
    return GetPoly(1, polygons, box, max);
}

PORT_OVERRIDE int CMap::GetCameraPoly(CCPoly *polygons, mgVu0FBOX &box, int max) {
    if (polygons==nullptr || max<=0) return 0;
    IxionRoomObserveDungeonLoop();
    if (DngMainScene!=nullptr && DngMainScene->active_map==0 && DngMainScene->GetMap(0)==this &&
        DngSaveDataDungeon!=nullptr) {
        const int dungeon=DngSaveDataDungeon->stage_id;
        if (dungeon>=0 && dungeon<SAVE_DUNGEON_NUM) {
            const int floor=DngSaveDataDungeon->floor_id[dungeon];
            if (IxionRoomRuntimeActive(dungeon,floor)) {
                // Ixion arena: DC1's PT_CAM camera walls first, then the room collision.
                const int camera=IxionRoomArenaCameraPoly(polygons,max,box.min,box.max);
                return camera+IxionRoomAppendRuntimeCollision(polygons+camera,max-camera,box.min,box.max,dungeon,floor);
            }
        }
    }
    return GetPoly(3,polygons,box,max);
}

PORT_OVERRIDE int CMap::DrawSub(int direct) {
    IxionRoomObserveDungeonLoop();
    if (IxionRoomRuntimeOwnedBy(DngMainScene,this)) {
        if (direct!=0) return 0;
        int dungeon=DngSaveDataDungeon->stage_id;
        int floor=DngSaveDataDungeon->floor_id[dungeon];
        const int drawn=std::max(0,IxionRoomDrawRuntime(dungeon,floor));
        LogRoomGameplay(dungeon,floor);
        return drawn;
    }
    int plight_enable = mgGetPlightEnable();
    int lighting = mgActiveLighting(2, 1);
    CFuncPointCheck check;
    check.time = 0;
    CreateFuncCheck(&check);
    GetNowTime();

    int total = 0;
    CMapParts **list = draw_parts;
    for (int i = 0; i < draw_parts_num; ++i, ++list) {
        CMapParts *parts = *list;
        parts->CopyFuncPointCheck(check);
        int light_count = 0;
        if (func_point.flag & FUNC_POINT_MNGR_LIGHT) {
            float sphere[4];
            parts->GetBoundSphere(sphere);
            light_count = SetFuncPLight(sphere, &check);
        }
        if (light_count > 0) {
            mgPlightEnable(1);
        }
        total += direct != 0 ? parts->DrawDirect() : parts->Draw();
        ResetFuncPLight(light_count);
    }
    mgPlightEnable(plight_enable);
    if (lighting >= 0) {
        mgActiveLighting(lighting, 0);
    }
    return total;
}
