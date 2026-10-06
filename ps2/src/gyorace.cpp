#include "common.h"
#include "gyorace.hpp"
#include "gyoracesim.hpp"
#include "subgame.hpp"
#include "scenesnd.hpp"
#include "character.hpp"
#include "mg_math.hpp"
#include "mglib.hpp"
#include "mg_drawprim.hpp"
#include "mg_memory.hpp"
#include "dng_effect.hpp"
#include "nd_meswin.hpp"
#include "snd_mngr.hpp"
#include "menuaqua.hpp"
#include "userdata.hpp"
#include <cstring>

static unsigned int gyore_snd_id;
float race_cnt;
#ifdef NONMATCHING
int race_proc_cnt;
#endif
#ifdef NONMATCHING
int race_mode;
#endif
#ifdef NONMATCHING
int time_max;
#endif
static int rank_count;
#ifdef NONMATCHING
static mgCTexture *EffectTex;
#endif
#ifdef NONMATCHING
static mgCTexture *EffectTex2;
#endif
#ifdef NONMATCHING
static mgCTexture *wind_tex;
#endif
static int hero_no;
static u_char water_cam;
static int cam_no;
static float win_alpha;
#ifdef NONMATCHING
static int effect_cnt;
#endif
static int mes_count;
static int jyunkai_flg;
#ifdef NONMATCHING
static int hantei_flg;
#endif
#ifdef NONMATCHING
static int goal_cnt;
#endif
static CHitEffectImage *battle_effect;
#ifdef NONMATCHING
static BattleEffectPrim battle_EffectPara[96][32];
#endif
int camera_id;
#ifdef NONMATCHING
int race_rank[2];
#endif
ClsMes *gyo_mes;
#ifdef NONMATCHING
static int CharaTexb;
#endif
#ifdef NONMATCHING
static int WindowTexb;
#endif
static int EffectTexb;
static float raster_offset;
static s8 raster_initialized;
#ifdef NONMATCHING
GYORACE_RESULT fish_game_data[6];
#endif
grRACE_INFO RaceInfo;
#ifdef NONMATCHING
grRACE_PROGRESS old_prog[6];
#endif
static int fish_rank[6];
static int old_fish_rank[6];
#ifdef NONMATCHING
static CGameDataUsed *game_data[8];
#endif
#ifdef NONMATCHING
static float old_ambient[4];
#endif
static mgCMemory BuffTextureData;
static mgCMemory BuffWorkData;
mgCCamera camera0(8.0f);
GYORACE_FISH_INF fish_inf[6];
static int old_cam_no = -1;

static void DivSpriteScreen(mgCDrawPrim &prim);

// Code (.text)
#ifdef NONMATCHING
int sgInitGyoRace(SubGameInfo *info) {
    if (info == NULL || info->scene == NULL) return 0;
    race_rank[0] = GetGyoRaceClass();
    race_rank[1] = GetGyoRaceNo();
    race_mode = GYORACE_MODE_READY;
    race_proc_cnt = 75;
    race_cnt = 0.0f;
    hero_no = 0;
    goal_cnt = 0;
    rank_count = 0;
    mes_count = 0;
    jyunkai_flg = 0;
    hantei_flg = 0;
    old_cam_no = -1;
    win_alpha = 128.0f;
    CharaTexb = info->texb;
    WindowTexb = CharaTexb + 1;
    EffectTexb = WindowTexb + 1;
    RaceInfo.fish_num = 6;
    RaceInfo.seed = 0;
    CGameDataUsed *hero_fish = GetGyoRaceFish();
    for (int fish = 0; fish < 6; ++fish) {
        CGameDataUsed *entrant = fish == hero_no ? hero_fish : NULL;
        game_data[fish] = entrant;
        fish_inf[fish].lane = fish;
        fish_inf[fish].chara_no = 0x40 + fish;
        fish_inf[fish].fish_no = -1;
        fish_inf[fish].rank = fish + 1;
        fish_inf[fish].lap = 0;
        fish_inf[fish].time = 0.0f;
        RaceInfo.fish[fish].lane = fish;
        if (entrant != NULL) {
            RaceInfo.fish[fish].fish_no = entrant->item_no;
            strncpy(RaceInfo.fish[fish].name, entrant->data.fish.name, sizeof(RaceInfo.fish[fish].name));
        }
    }
    time_max = grGyoRaceSimulate(&RaceInfo);
    camera0.SetPos(270.0f, -40.0f, -10.0f);
    camera0.SetNextPos(270.0f, -40.0f, -10.0f);
    camera0.SetRef(192.0f, 0.0f, 0.0f);
    camera0.SetNextRef(192.0f, 0.0f, 0.0f);
    camera_id = info->scene->AssignCamera(0, &camera0, NULL);
    AutoCam(info);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyorace", sgInitGyoRace__FP11SubGameInfo);
#endif
#ifdef NONMATCHING
int sgLoopGyoRace(SubGameInfo *info) {
    if (info == NULL || info->scene == NULL) return 0;
    switch (race_mode) {
    case GYORACE_MODE_READY:
        if (--race_proc_cnt <= 0) {
            race_mode = GYORACE_MODE_GATE_OPEN;
            race_proc_cnt = 15;
        }
        break;
    case GYORACE_MODE_GATE_OPEN:
        if (--race_proc_cnt <= 0) {
            race_mode = GYORACE_MODE_RACE;
            race_cnt = 0.0f;
        }
        break;
    case GYORACE_MODE_RACE:
    case GYORACE_MODE_FINISH:
        race_cnt += 0.1f;
        goal_cnt = 0;
        for (int fish = 0; fish < 6; ++fish) {
            grRACE_PROGRESS progress;
            if (!grGetFishProgress(&RaceInfo, fish, race_cnt, &progress)) continue;
            GYORACE_FISH_INF &state = fish_inf[fish];
            unsigned int lap = (unsigned int)(progress.pos / 8.0f);
            if (lap > state.lap && lap < 3) {
                state.lap = lap;
                state.lap_start = race_cnt;
            }
            if (progress.state == GR_RACE_STATE_GOAL) {
                ++goal_cnt;
                state.rank = RaceInfo.rank[fish];
                state.time = RaceInfo.goal_time[fish] * 20.0f;
            }
            old_prog[fish] = progress;
        }
        AutoCam(info);
        Jikkyou(info);
        if (race_mode == GYORACE_MODE_RACE && goal_cnt == 6) {
            race_mode = GYORACE_MODE_FINISH;
            race_proc_cnt = 120;
        }
        if (race_mode == GYORACE_MODE_FINISH && --race_proc_cnt <= 0) race_mode = GYORACE_MODE_END;
        break;
    case GYORACE_MODE_GOAL_VIEW:
        if (--race_proc_cnt <= 0) race_mode = GYORACE_MODE_FINISH;
        break;
    case GYORACE_MODE_END:
        for (int fish = 0; fish < 6; ++fish) {
            int place = RaceInfo.rank[fish] - 1;
            if (place < 0 || place >= 6) continue;
            GYORACE_RESULT &result = fish_game_data[place];
            memset(result.name, ' ', sizeof(result.name));
            memcpy(result.name, RaceInfo.fish[fish].name, sizeof(result.name));
            result.time = RaceInfo.goal_time[fish] * 20.0f;
            result.fish_no = fish_inf[fish].fish_no;
            result.race_class = race_rank[0];
        }
        return 1;
    }
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyorace", sgLoopGyoRace__FP11SubGameInfo);
#endif
void AutoCam(SubGameInfo *info) {
    float nearest;
    CScene *scene = info->scene;
    CCharacter2 *hero = scene->GetCharacter(fish_inf[hero_no].chara_no);
    sceVu0FVECTOR hero_pos;
    hero->GetPosition(hero_pos);
    nearest = 9999.0f;
    cam_no = 0;
    for (int camera = 0; camera < 5; camera++) {
        float distance = mgDistVector(hero_pos, cam_pos[camera]);
        if (distance < nearest) {
            nearest = distance;
            cam_no = camera;
        }
    }
    if (old_cam_no != cam_no) {
        if (cam_pos[cam_no][1] < 0.0f) {
            sndSetSeVol(gyore_snd_id, 2, sndGetSeDefVol(gyore_snd_id, 2), 0);
        } else {
            sndSetSeVol(gyore_snd_id, 2, 0, 0);
        }
        camera0.SetPos(cam_pos[cam_no]);
        camera0.SetNextPos(cam_pos[cam_no]);
        mgDistVector(hero_pos, cam_pos[cam_no]);
        scene->active_camera = camera_id;
        camera0.SetRef(hero_pos);
        camera0.SetNextRef(hero_pos);
        camera0.SetSpeed(0.0f, 0.0f);
        win_alpha = 128.0f;
    } else {
        camera0.SetSpeed(9999.0f, 20.0f);
        mgDistVector(hero_pos, cam_pos[cam_no]);
        camera0.SetNextRef(hero_pos);
    }
    old_cam_no = cam_no;
}
s32 sgMapDrawGyoRace(SubGameInfo *info) {
    return 0;
}

int sgCharaDrawGyoRace(SubGameInfo *info) {
    CScene *scene;
    scene = info->scene;
    for (int fish = 0; fish < 6; ++fish) {
        scene->DrawChara(fish_inf[fish].chara_no, 1);
    }
    mgTexManager.ReloadTexture(EffectTexb, (sceVif1Packet *)NULL);
    for (int effect = 0; effect < 96; ++effect) {
        CHitEffectImage *image = battle_effect + effect;
        if (image != NULL) {
            image->Step();
            image->Draw();
        }
    }
    return 0;
}

/**
 * Draws the screen in distorted horizontal strips.
 */
static void DivSpriteScreen(mgCDrawPrim &prim) {
    int strip_height;
    int screen_width;
    int y;
    int y16;
    int x;
    int x16;
    int height16;
    int width16;

    if (!raster_initialized) {
        raster_offset = 0.0f;
        raster_initialized = true;
    }
    prim.BeginPrim2(MG_PRIM_TRIANGLE_STRIP, 0x43, 0, 2);
    int origin[4] = {mgScreenOffx << 4, mgScreenOffy << 4, 0, 0};
    screen_width = mgScreenWidth;
    int screen_height = mgScreenHeight;
    strip_height = screen_height / 48;
    sceVu0FVECTOR step = {0, 0, 0, 1};
    step[1] = (float)((strip_height / 3) << 4);
    sceVu0FMATRIX matrix;
    sceVu0UnitMatrix(matrix);
    x = 0;
    x16 = 0;
    width16 = screen_width << 4;
    while (x < mgScreenWidth) {
        y = 0;
        y16 = 0;
        height16 = strip_height << 4;
        while (y < mgScreenHeight) {
            sceVu0RotMatrixZ(matrix, matrix, raster_offset);
            sceVu0ApplyMatrix(step, matrix, step);
            int uv[4] = {0, 0, 0, 0};
            int xy[4] = {0, 0, 0, 0};
            *(u_long128 *)uv = *(u_long128 *)origin;
            xy[0] = x16;
            xy[1] = y16;
            prim.Data(xy);
            uv[0] = xy[0] + origin[0] - (int)step[0];
            uv[1] = xy[1] + origin[1];
            prim.Data(uv);
            xy[0] = (x + screen_width) << 4;
            xy[1] = (y + strip_height) << 4;
            prim.Data(xy);
            uv[0] = (int)step[0] + (xy[0] + origin[0]);
            uv[1] = xy[1] + origin[1];
            prim.Data(uv);
            y += strip_height;
            y16 += height16;
        }
        x += screen_width;
        x16 += width16;
    }
    raster_offset += 0.0004363323f;
    if (raster_offset > 6.2831855f) raster_offset = -6.2831855f;
    prim.EndPrim2();
}

int sgEffectDrawGyoRace(SubGameInfo *info) {
    if (!water_cam) return 0;
    mgCDrawPrim prim;
    prim.Initialize(NULL, NULL);
    mgCTexture frame;
    mgGetFrameBuffer(&frame);
    frame.tex0.bits.tcc = 0;
    prim.DepthTestEnable(0);
    prim.AlphaTestEnable(0);
    prim.AlphaBlendEnable(0);
    prim.ZMask(MG_Z_MASK_MASKED);
    prim.TextureMapEnable(1);
    prim.Begin2();
    prim.BeginPrim2(MG_PRIM_SPRITE);
    prim.Texture(&frame);
    prim.Direct(0x3B, 0x8000008080ULL);
    prim.Color(128, 128, 128, 128);
    prim.EndPrim2();
    DivSpriteScreen(prim);
    prim.End2();
    return 0;
}
#ifdef NONMATCHING
int sgSysDrawGyoRace(SubGameInfo *info) {
    if (gyo_mes != NULL) {
        gyo_mes->Step();
        gyo_mes->DrawMesWin();
    }
    for (int fish = 0; fish < 6; ++fish) {
        grRACE_PROGRESS progress;
        if (!grGetFishProgress(&RaceInfo, fish, race_cnt, &progress)) continue;
        fish_inf[fish].lap = (unsigned int)(progress.pos / 8.0f);
        fish_inf[fish].rank = RaceInfo.rank[fish];
        if (fish_inf[fish].lap > 1) {
            float elapsed = race_cnt * 20.0f;
            fish_inf[fish].lap_time[1] = elapsed - fish_inf[fish].lap_time[0];
        }
    }
    if (win_alpha > 0.0f) win_alpha -= 0.5f;
    return 0;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/gyorace", sgSysDrawGyoRace__FP11SubGameInfo);
#endif
int Jikkyou(SubGameInfo *info) {
    CFont font;
    grRACE_PROGRESS lead;
    grRACE_PROGRESS progress[6];
    grRACE_PROGRESS fish;
    float leading_position;
    int index;
    int fish_no;
    char *name;
    ClsMes *message;

    font.Init();
    font.Preset(4);
    font.SetFuchi(3);
    font.SetDrawSize(16, 20);
    const float race_time = race_cnt;
    if ((double)race_time <= 0.0) {
        return -1;
    }
    mes_count--;
    if (mes_count < 0) {
        mes_count = 0;
    }
    if (mes_count > 0) {
        return -1;
    }
    grGetFishProgress(&RaceInfo, fish_rank[0], race_time, &lead);
    leading_position = lead.pos;
    if (leading_position > 0.0f && leading_position <= 1.0f) {
        gyo_mes->MakeMesWin(5);
        mes_count = 60;
        return 0;
    }
    if (leading_position > 1.0f && leading_position <= 2.0f) {
        grGetFishProgress(&RaceInfo, fish_rank[0], 0.5f, &progress[0]);
        grGetFishProgress(&RaceInfo, fish_rank[5], 0.5f, &progress[1]);
        if (progress[0].pos - progress[1].pos > 0.01f) {
            gyo_mes->MakeMesWin(6);
            mes_count = 60;
            return 0;
        }
        gyo_mes->MakeMesWin(7);
        mes_count = 60;
        return 0;
    }
    if ((leading_position > 2.0f && leading_position < 8.0f) || (leading_position > 9.0f && leading_position < 16.0f)) {
        if (!jyunkai_flg) {
            jyunkai_flg = 1;
            index = 0;
            do {
                grGetFishProgress(&RaceInfo, index, race_cnt, &fish);
                if ((u_char)fish.state == GR_RACE_STATE_BATTLE) {
                    int lane = RaceInfo.fish[index].lane + 1;
                    message = gyo_mes;
                    message->values[0] = lane;
                    message->value_width[0] = 0;
                    name = RaceInfo.fish[index].name;
                    ClsMes *name_mes = gyo_mes;
                    if (name != NULL) {
                        strcpy(name_mes->name[0], name);
                    }
                    gyo_mes->MakeMesWin(29);
                    mes_count = 60;
                    jyunkai_flg = 0;
                    sndSePlay(gyore_snd_id, 0x16, 0);
                    break;
                }
                index++;
            } while (index < 6);
            return 0;
        }
        fish_no = fish_rank[rank_count];
        int lane = RaceInfo.fish[fish_no].lane + 1;
        message = gyo_mes;
        message->values[0] = lane;
        message->value_width[0] = 0;
        name = RaceInfo.fish[fish_no].name;
        ClsMes *name_mes = gyo_mes;
        if (name != NULL) {
            strcpy(name_mes->name[0], name);
        }
        if (rank_count == 0) {
            gyo_mes->MakeMesWin(RaceInfo.fish[fish_no].bonus_type + 9);
        } else if (rank_count < 5) {
            if (fish_rank[rank_count] == old_fish_rank[rank_count - 1]) {
                fish_no = fish_rank[rank_count - 1];
                int lane = RaceInfo.fish[fish_no].lane + 1;
                message = gyo_mes;
                message->values[0] = lane;
                message->value_width[0] = 0;
                name = RaceInfo.fish[fish_no].name;
                ClsMes *name_mes = gyo_mes;
                if (name != NULL) {
                    strcpy(name_mes->name[0], name);
                }
                gyo_mes->MakeMesWin(RaceInfo.fish[fish_no].bonus_type + 13);
            } else {
                gyo_mes->MakeMesWin(RaceInfo.fish[fish_no].bonus_type + 17);
            }
        } else {
            gyo_mes->MakeMesWin(RaceInfo.fish[fish_no].bonus_type + 21);
        }
        old_fish_rank[rank_count] = fish_rank[rank_count];
        rank_count++;
        if (leading_position > 9.0f) {
            if (rank_count > 1) {
                rank_count = 0;
            }
        } else if (rank_count > 5) {
            rank_count = 0;
        }
        mes_count = 60;
        return 0;
    }
    if (leading_position > 8.0f && leading_position <= 9.0f) {
        jyunkai_flg = 0;
        rank_count = 0;
        gyo_mes->MakeMesWin(30);
        sndSePlay(gyore_snd_id, 0x16, 0);
        mes_count = 60;
        return 0;
    }
    if (leading_position >= 16.0f) {
        message = gyo_mes;
        message->values[0] = 1;
        message->value_width[0] = 0;
        name = RaceInfo.fish[fish_rank[0]].name;
        ClsMes *name_mes = gyo_mes;
        if (name != NULL) {
            strcpy(name_mes->name[0], name);
        }
        gyo_mes->MakeMesWin(31);
        mes_count = 6000;
        sndSePlay(gyore_snd_id, 0x16, 0);
        sndSePlay(gyore_snd_id, 0x17, 0);
    }
    return 0;
}

// Static initialiser (.init)


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", fish_name__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", cam_pos__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1027__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1028__9__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1481__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1524__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1547__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1548__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1766__3__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_903__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_904__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_905__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_906__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_907__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_908__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_909__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_910__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_911__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_912__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_913__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_914__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_915__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_916__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_917__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_918__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_919__7__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_920__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1373__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1374__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1375__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1376__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1377__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1378__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1379__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1380__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1381__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1382__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1383__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1384__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1696__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1697__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1698__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1699__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1700__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1701__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1702__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/gyorace", at_1703__DATA);

// Static initialiser table (.ctor)

// Small initialised data (.sdata)

// Small uninitialised data (.sbss)
#ifndef NONMATCHING
INCLUDE_BSS(race_proc_cnt, 0x4);
INCLUDE_BSS(race_mode, 0x4);
INCLUDE_BSS(time_max, 0x4);
INCLUDE_BSS(EffectTex, 0x4);
INCLUDE_BSS(EffectTex2, 0x4);
INCLUDE_BSS(wind_tex, 0x4);
INCLUDE_BSS(effect_cnt, 0x4);
INCLUDE_BSS(hantei_flg, 0x4);
INCLUDE_BSS(goal_cnt, 0x4);
INCLUDE_BSS(battle_EffectPara, 0x4);
INCLUDE_BSS(race_rank, 0x8);
INCLUDE_BSS(CharaTexb, 0x4);
INCLUDE_BSS(WindowTexb, 0x4);
INCLUDE_BSS(ras_off_1762, 0x4);
INCLUDE_BSS(init_1763, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(fish_game_data, 0xE0);
INCLUDE_BSS(old_prog, 0x90);
INCLUDE_BSS(game_data, 0x20);
INCLUDE_BSS(old_ambient, 0x10);
INCLUDE_BSS(at_1765__2, 0x10);
INCLUDE_BSS(at_1775, 0x10);
INCLUDE_BSS(at_1776, 0x10);
INCLUDE_BSS(lap_inf_1798, 0x30);
INCLUDE_BSS(lap_inf2_1799, 0x50);
#endif
