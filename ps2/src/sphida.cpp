#include "common.h"
#include "sphida.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "actionchara.hpp"
#include "automap.hpp"
#include "cameracontrol.hpp"
#include "collision.hpp"
#include "dng_main.hpp"
#include "event.hpp"
#include "mainloop.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "padcontrol.hpp"
#include "savedata.hpp"
#include "scenesnd.hpp"

// Code (.text)
GOLF_CLUB_DEF *GetSphidaClubDef(int club_no) {
    if (club_no < 9 || club_no > 14) {
        return NULL;
    }
    return &GolfClubDef[club_no - 9];
}

void DPrimEnterSprite(mgCDrawPrim *prim, int u, int v, int tex_w, int tex_h, float x, float y, float w, float h) {
    float half_h;
    float half_w;

    prim->TextureCrd(u, v);
    half_w = w / 2.0f;
    half_h = h / 2.0f;
    prim->Vertex(x - half_w, y - half_h, 0.0f);
    prim->TextureCrd(u + tex_w, v + tex_h);
    prim->Vertex(x + half_w, y + half_h, 0.0f);
}

void CPowGage::Initialize(void) {
    pos_y = 0.0f;
    pos_x = 0.0f;
    texture = NULL;
    power = 0.0f;
    safe_level = 2;
    code = -10;
    state = -1;
    reverse = 0;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sphida", Step__8CPowGageFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sphida", Draw__8CPowGageFv);
void InitSphida(void) {
    Sphida = 0;
}

CSphida *GetSphidaPtr() {
    return Sphida;
}

CSphida::CSphida() {
    memset(unk_198, 0x80, sizeof(unk_198));
    Initialize();
}

void CSphida::Initialize() {
    int i;
    int j;

    play_flag = 0;
    minimap_flag = 0;
    mm_line_flag = 0;
    status_flag = 0;
    for (i = 0; i < 5; i++) {
        mgZeroVector(mm_line_pos[i]);
    }
    mgZeroVector(pin_pos);
    mgZeroVector(ball_pos);
    pin_col = 0;
    ball_col = 0;
    par_count = 0;
    col_model = NULL;
    last_challenge = 0;
    omake_mode = 0;
    for (j = 0; j < 9; j++) {
        unk_210[j] = 0;
    }
}

void CSphida::SetUp(int tex_bank) {
    CMapParts           *parts[128];
    float                dists[128];
    sceVu0FVECTOR        chara_pos;
    CCPoly               polys[128];
    mgVu0FBOX            box;
    sceVu0FVECTOR        from;
    sceVu0FVECTOR        to;
    sceVu0FVECTOR        hit;
    CTreasureBoxManager *boxes;
    int                  count;
    int                  dng_no;
    CSaveData           *save;
    float                navi;

    DngMainScene->GetCharacter(DngMainScene->player_chara)->GetPosition(chara_pos);
    boxes = DngMainScene->battle_area.treasure_box;
    SearchMapEventParts(1, parts, dists, 128);

    // Pick flat spots clear of treasure boxes and other circles until the pin is away from the player.
    do {
    } while (!SearchMapFlatPosition(pin_pos, &AutoMapGen) || !boxes->CheckArea(pin_pos, 60.0f) ||
             !RandomCircle.CheckArea(pin_pos, 60.0f) || mgDistVector(chara_pos, pin_pos) < 60.0f);
    pin_pos[1] += 30.0f;

    do {
    } while (!SearchMapFlatPosition(ball_pos, &AutoMapGen) || !boxes->CheckArea(ball_pos, 40.0f) ||
             !RandomCircle.CheckArea(pin_pos, 60.0f) || mgDistVector(pin_pos, ball_pos) < 200.0f);

    box.max[0] = 20.0f + ball_pos[0];
    box.min[0] = ball_pos[0] - 20.0f;
    box.max[1] = 20.0f + ball_pos[1];
    box.min[1] = ball_pos[1] - 20.0f;
    box.max[2] = 20.0f + ball_pos[2];
    box.min[2] = ball_pos[2] - 20.0f;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;

    // Drop the ball onto the ground below it.
    *(u_long128 *)from = *(u_long128 *)ball_pos;
    *(u_long128 *)to = *(u_long128 *)ball_pos;
    from[1] += 18.0f;
    to[1] -= 18.0f;
    count = DngMainScene->GetColPoly(polys, box, 128);
    if (CheckHit(polys, count, from, to, hit, 1, 0xC) >= 0) {
        *(u_long128 *)ball_pos = *(u_long128 *)hit;
    }

    ball_pos[1] += 3.0f;
    pin_col = (int)(2.0f * (float)rand() / 2147483648.0f);
    ball_col = (int)(2.0f * (float)rand() / 2147483648.0f);

    dng_no = -1;
    save = GetSaveData();
    if (save != NULL) {
        CSaveDataDungeon *dungeon = &save->save_dungeon;
        if (dungeon != NULL) {
            dng_no = dungeon->stage_id;
        }
    }

    switch (dng_no) {
        case 0:
        case 3:
        case 4:
        case 5:
        case 6:
            AutoMapGen.UpdateNaviMap(ball_pos, 40);
            navi = AutoMapGen.GetNaviDistance(pin_pos);
            if (navi < 0.0f) {
                par_count = (int)(mgDistVector(pin_pos, ball_pos) / 600.0f) + 1;
                printf("SETUP_SFIDA[DIST %d]\n", par_count);
            } else {
                par_count = (int)(navi / 1000.0f) + 1;
                printf("SETUP_SFIDA[NAVI %d]\n", par_count);
            }
            break;
        case 1:
        case 2:
        default:
            par_count = (int)(mgDistVector(pin_pos, ball_pos) / 800.0f) + 1;
            break;
    }

    if (par_count > 99) {
        par_count = 99;
    }

    if (RedMarkModel != NULL) {
        memcpy(&red_mark, RedMarkModel, sizeof(red_mark));
    }

    this->tex_bank = tex_bank;
    InitStatusSprite();
    play_flag = 1;
}

void CSphida::s17_SetUp(int tex_bank) {
    pin_pos[0] = -1.09f;
    pin_pos[1] = 201.0f;
    pin_pos[2] = -478.03f;
    pin_pos[3] = 1.0f;
    ball_pos[0] = 0.0f;
    ball_pos[1] = 65.57f;
    ball_pos[2] = 1305.04f;
    ball_pos[3] = 1.0f;
    pin_col = 1;
    ball_col = 0;

    par_count = (int)(mgDistVector(pin_pos, ball_pos) / 800.0f) + 1;
    if (par_count > 99) {
        par_count = 99;
    }

    if (RedMarkModel != NULL) {
        memcpy(&red_mark, RedMarkModel, sizeof(red_mark));
    }

    this->tex_bank = tex_bank;
    InitStatusSprite();
    play_flag = 1;
}

void CSphida::Omake_SetUp(int course, int tex_bank) {
    switch (course) {
        case 0:
            pin_pos[0] = 0.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 0.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 640.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 1280.0f;
            ball_pos[3] = 1.0f;
            par_count = 3;
            break;
        case 1:
            pin_pos[0] = 960.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 960.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 640.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 1920.0f;
            ball_pos[3] = 1.0f;
            par_count = 3;
            break;
        case 2:
            pin_pos[0] = 1600.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 960.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 1600.0f;
            ball_pos[1] = 5.0f;
            ball_pos[2] = 2880.0f;
            ball_pos[3] = 1.0f;
            par_count = 3;
            break;
        case 3:
            pin_pos[0] = 1280.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 0.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 1280.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 3840.0f;
            ball_pos[3] = 1.0f;
            par_count = 4;
            break;
        case 4:
            pin_pos[0] = 2800.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 800.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 1600.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 5600.0f;
            ball_pos[3] = 1.0f;
            par_count = 15;
            break;
        case 5:
            pin_pos[0] = 0.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 4000.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 2800.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 800.0f;
            ball_pos[3] = 1.0f;
            par_count = 6;
            break;
        case 6:
            pin_pos[0] = 1600.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 1600.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 2800.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 2800.0f;
            ball_pos[3] = 1.0f;
            par_count = 10;
            break;
        case 7:
            pin_pos[0] = 1280.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 2560.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 1280.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 3200.0f;
            ball_pos[3] = 1.0f;
            par_count = 7;
            break;
        case 8:
            pin_pos[0] = 0.0f;
            pin_pos[1] = 30.0f;
            pin_pos[2] = 4480.0f;
            pin_pos[3] = 1.0f;
            ball_pos[0] = 4160.0f;
            ball_pos[1] = 3.0f;
            ball_pos[2] = 4480.0f;
            ball_pos[3] = 1.0f;
            par_count = 7;
            break;
        default:
            return;
    }
    AutoMapGen.MinimapAllVisible();
    pin_col = (int)(2.0f * (float)rand() / 2147483648.0f);
    ball_col = (int)(2.0f * (float)rand() / 2147483648.0f);
    if (RedMarkModel != NULL) {
        memcpy(&red_mark, RedMarkModel, sizeof(red_mark));
    }
    this->tex_bank = tex_bank;
    InitStatusSprite();
    play_flag = 1;
    omake_mode = 1;
}

int CSphida::Step() {
    sceVu0FVECTOR    chara_pos;
    CCharacter2     *chara;
    DNG_BATTLE_AREA *area;
    int              event_no;
    int              map_level;

    if (play_flag == 0) {
        return 0;
    }
    chara = DngMainScene->GetCharacter(DngMainScene->player_chara);
    chara->GetPosition(chara_pos);
    pow_gage.Step();
    if (((CActionChara *)chara)->CheckRunEvent() == 0) {
        red_mark.draw_request = 0;
        return 0;
    }
    if (DngStatus.eye_view != 0) {
        red_mark.draw_request = 0;
        return 0;
    }
    area = &DngMainScene->battle_area;
    if (area != NULL && area->script.event_no != -1) {
        red_mark.draw_request = 0;
        return 0;
    }
    if (DngMainScene->event_run != 0) {
        red_mark.draw_request = 0;
        return 0;
    }
    if (mgDistVector(chara_pos, ball_pos) <= 40.0f) {
        red_mark.SetPosition(chara_pos);
        red_mark.draw_request = 1;
        red_mark.Step();
        if (PadCtrl.Btn(0) != 0 && DngStatus.mode == DNG_STATUS_FIELD) {
            memcpy(&EventCamera__2, &MainCamera, sizeof(mgCCameraFollow));
            DngMainScene->active_camera = 1;
            InitEvent(DngMainScene);
            if (RunEvent(SPHIDA_EVENT_SHOT, DngMainScene) != 0) {
                DngStatus.debug_window = 0;
                DngStatus.mode = DNG_STATUS_EVENT;
                DngMainScene->before_camera = 0;
                sceVu0CopyVector(map_view_pos, ball_pos);
                map_level = DngSaveData->GetConfig()->map;
                if (map_level != 0) {
                    mini_level = map_level;
                }
                spin_mark_pos_y = 0.0f;
                spin_mark_pos_x = 0.0f;
                ((CActionChara *)chara)->RemoveThrowItem();
                red_mark.draw_request = 0;
                return 1;
            }
        }
    } else {
        red_mark.draw_request = 0;
    }
    if (GamePad__2.Down(PAD_SQUARE) != 0 && DngStatus.mode == DNG_STATUS_FIELD) {
        event_no = -1;
        if (DebugFlag != 0) {
            event_no = SPHIDA_EVENT_NEAR_BALL;
        } else if (mgDistVector(chara_pos, ball_pos) <= 160.0f && last_challenge == 0) {
            if (par_count < 2) {
                event_no = SPHIDA_EVENT_NEAR_BALL_LAST;
            } else {
                event_no = SPHIDA_EVENT_NEAR_BALL;
            }
        } else if (omake_mode == 1) {
            event_no = SPHIDA_EVENT_OMAKE_AWAY;
        }
        if (event_no >= 0) {
            memcpy(&EventCamera__2, &MainCamera, sizeof(mgCCameraFollow));
            DngMainScene->active_camera = 1;
            InitEvent(DngMainScene);
            if (RunEvent(event_no, DngMainScene) != 0) {
                DngStatus.debug_window = 0;
                DngStatus.mode = DNG_STATUS_EVENT;
                DngMainScene->before_camera = 0;
                ((CActionChara *)chara)->RemoveThrowItem();
                red_mark.draw_request = 0;
                return 1;
            }
        }
    }
    return 0;
}

void CSphida::InitStatusSprite() {
    mgCTexture *texture = mgTexManager.GetTexture("sphida_bar", -1);

    pow_gage.pos_x = 256.0f;
    pow_gage.pos_y = 406.4f;
    pow_gage.texture = texture;
}
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sphida", DrawStatusSprite__7CSphidaFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sphida", DrawParCounter__7CSphidaFv);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/sphida", Draw__7CSphidaFv);

int CSphida::SetCollisionModel(MDS_HEADER *model, mgCMemory *memory) {
    col_model = LoadCollisionFile(model, memory);
    return col_model != NULL;
}

int CSphida::PickupCollision(float *pos, CCPoly *poly, mgVu0FBOX box, int max) {
    int count;

    if (play_flag == 0 || col_model == NULL) {
        return 0;
    }
    count = 0;
    if (mgDistVector(pin_pos, pos) < 80.0f) {
        col_model->SetPosition(pin_pos);
        count += col_model->PickUpNearPoly(poly, box, max);
    }
    return count;
}

void CSphida::DrawMiniMapSymbol(CMiniMapSymbol *symbol) {
    int i;

    if (play_flag == 0) {
        return;
    }
    if (pin_col == 0) {
        symbol->DrawSymbol(pin_pos, MINIMAP_SYMBOL_SPHIDA_4);
    } else {
        symbol->DrawSymbol(pin_pos, MINIMAP_SYMBOL_SPHIDA_5);
    }
    if (ball_col == 0) {
        symbol->DrawSymbol(ball_pos, MINIMAP_SYMBOL_SPHIDA_6);
    } else {
        symbol->DrawSymbol(ball_pos, MINIMAP_SYMBOL_SPHIDA_7);
    }
    if (mm_line_flag == 1) {
        for (i = 0; i < 5; i++) {
            symbol->DrawSymbol(mm_line_pos[i], MINIMAP_SYMBOL_MONSTER);
        }
    }
}

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", GolfClubDef__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_940__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_1088__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_1089__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_1090__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_1138__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/sphida", at_1221__5__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(Sphida, 0x4);
