#include "common.h"
#include "eventedit.hpp"
#include "character.hpp"
#include "dataread.hpp"
#include "dbg_font.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "gamepad.hpp"
#include "mainloop.hpp"
#include "mg_camera.hpp"
#include "mg_drawprim.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "sceneseq.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <libvu0.h>
#include <sifdev.h>

static EventEditInfo g_info;
static CCameraPas g_cmr_pas;
static CCharaPas g_chara_pas;
static int g_cp_mode;
static int g_cp_cursor;
static int g_cp_selno;
static int g_chara_pas_mode;
static int g_chara_pas_cursor;
static int g_chara_pas_selno;

/**
 * Draws the edges of a box after transforming its corners to the screen.
 */
static void DrawBox(float (*corners)[4], int r, int g, int b);
/**
 * Moves an event character with the pad.
 */
static void MoveChara(CCharacter2 *chara, mgCCamera *camera, mgCMemory *memory);

// Code (.text)
/**
 * Writes the current character and camera settings as event script commands.
 */
static void OutPutFile() {
    char text[0x100];
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR eye_pos;
    sceVu0FVECTOR look_pos;
    sceVu0FVECTOR view_dir;
    sceVu0FVECTOR flat_dir;
    float angle;
    int i;
    int file;
    CCharacter2 *chara;
    mgCCamera *camera;

    file = sceOpen("host0:debug.txt", SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (file < 0) {
        return;
    }
    sprintf(text, "character\r\n");
    sceWrite(file, text, strlen(text));
    chara = GetCharacter(g_info.chara_no);
    sprintf(text, "select_chara %d \n", g_info.chara_no);
    sceWrite(file, text, strlen(text));
    sprintf(text, "collision %d\n", g_info.collision);
    sceWrite(file, text, strlen(text));
    chara->GetPosition(chara_pos);
    chara->GetRotation(chara_rot);
    CalcPosWorldCoordGyaku(chara_pos);
    chara_rot[0] -= EdEventInfo.world_coord_rot[0];
    chara_rot[1] -= EdEventInfo.world_coord_rot[1];
    chara_rot[2] -= EdEventInfo.world_coord_rot[2];
    sprintf(text, "pos = %1.2f, %1.2f, %1.2f\n", chara_pos[0], chara_pos[1], chara_pos[2]);
    sceWrite(file, text, strlen(text));
    sprintf(text, "rot = %1.2f, %1.2f, %1.2f\n", chara_rot[0], chara_rot[1], chara_rot[2]);
    sceWrite(file, text, strlen(text));
    sprintf(text, "\ncamera\n");
    sceWrite(file, text, strlen(text));
    camera = GetActiveCamera();
    camera->GetPos(eye_pos);
    camera->GetRef(look_pos);
    sceVu0SubVector(view_dir, look_pos, eye_pos);
    flat_dir[1] = 0.0f;
    flat_dir[0] = view_dir[0];
    flat_dir[2] = view_dir[2];
    flat_dir[3] = 0.0f;
    sceVu0Normalize(flat_dir, flat_dir);
    angle = atan2f(-flat_dir[0], -flat_dir[2]);
    CalcPosWorldCoordGyaku(eye_pos);
    CalcPosWorldCoordGyaku(look_pos);
    sprintf(text, "CMRS_SET_POS\t%1.2f, %1.2f, %1.2f;\n", eye_pos[0], eye_pos[1], eye_pos[2]);
    sceWrite(file, text, strlen(text));
    sprintf(text, "CMRS_SET_REF\t%1.2f, %1.2f, %1.2f;\n", look_pos[0], look_pos[1], look_pos[2]);
    sceWrite(file, text, strlen(text));
    angle -= EdEventInfo.world_coord_rot[1];
    if (angle > 3.1415927f) {
        angle -= 6.2831855f;
    } else if (angle <= -3.1415927f) {
        angle += 6.2831855f;
    }
    sprintf(text, "angle = %1.2f\n", angle);
    sceWrite(file, text, strlen(text));
    sprintf(text, "height = %1.2f\n", (eye_pos[1] - look_pos[1]));
    sceWrite(file, text, strlen(text));
    eye_pos[1] = 0.0f;
    look_pos[1] = 0.0f;
    sprintf(text, "distance = %1.2f\n", mgDistVector(eye_pos, look_pos));
    sceWrite(file, text, strlen(text));
    sprintf(text, "projection = %1.1f\n", EdEventInfo.projection);
    sceWrite(file, text, strlen(text));
    sprintf(text, "\ncamera pas\n");
    sceWrite(file, text, strlen(text));
    sprintf(text, "pointnum = %d\n", g_cmr_pas.pas_num);
    sceWrite(file, text, strlen(text));
    sceWrite(file, "CMRS_INIT_PAS;\n", strlen("CMRS_INIT_PAS;\n"));
    sprintf(text, "CMRS_SET_PAS_FRM\t%d;\n", g_cmr_pas.GetFrame());
    sceWrite(file, text, strlen(text));
    for (i = 0; i < g_cmr_pas.pas_num; i++) {
        g_cmr_pas.GetCameraPas(i, eye_pos, look_pos);
        CalcPosWorldCoordGyaku(eye_pos);
        CalcPosWorldCoordGyaku(look_pos);
        sprintf(text, "CMRS_ADD_PAS\t%1.2f,%1.2f,%1.2f,\t%1.2f,%1.2f,%1.2f;\n", eye_pos[0], eye_pos[1], eye_pos[2],
                look_pos[0], look_pos[1], look_pos[2]);
        sceWrite(file, text, strlen(text));
    }
    sceWrite(file, "CMRS_START_PAS;\n", strlen("CMRS_START_PAS;\n"));
    sprintf(text, "\nchara pas\n");
    sceWrite(file, text, strlen(text));
    sprintf(text, "pointnum = %d\n", g_chara_pas.pas_num);
    sceWrite(file, text, strlen(text));
    sceWrite(file, "OBJS_INIT_PAS\t\tid;\n", strlen("OBJS_INIT_PAS\t\tid;\n"));
    sprintf(text, "OBJS_SET_PAS_FRM\tid, %d;\n", g_chara_pas.GetFrame());
    sceWrite(file, text, strlen(text));
    for (i = 0; i < g_chara_pas.pas_num; i++) {
        g_chara_pas.GetCharaPas(i, chara_pos);
        CalcPosWorldCoordGyaku(chara_pos);
        sprintf(text, "OBJS_ADD_PAS\t\tid, %1.2f, %1.2f, %1.2f;\n", chara_pos[0], chara_pos[1], chara_pos[2]);
        sceWrite(file, text, strlen(text));
    }
    sceWrite(file, "OBJS_START_PAS\t\tid;\n", strlen("OBJS_START_PAS\t\tid;\n"));
    sceClose(file);
}

/**
 * Draws the edges of a box in world coordinates.
 */
static void DrawBox(float *max, float *min, int r, int g, int b) {
    float corners[8][4];
    sceVu0FVECTOR lo;
    sceVu0FVECTOR hi;

    *(u_long128 *)lo = *(u_long128 *)min;
    *(u_long128 *)hi = *(u_long128 *)max;
    corners[0][0] = lo[0];
    corners[0][1] = lo[1];
    corners[0][2] = lo[2];
    corners[0][3] = 1.0f;
    corners[1][0] = hi[0];
    corners[1][1] = lo[1];
    corners[1][2] = lo[2];
    corners[1][3] = 1.0f;
    corners[2][0] = lo[0];
    corners[2][1] = hi[1];
    corners[2][2] = lo[2];
    corners[2][3] = 1.0f;
    corners[3][0] = hi[0];
    corners[3][1] = hi[1];
    corners[3][2] = lo[2];
    corners[3][3] = 1.0f;
    corners[4][0] = lo[0];
    corners[4][1] = lo[1];
    corners[4][2] = hi[2];
    corners[4][3] = 1.0f;
    corners[5][0] = hi[0];
    corners[5][1] = lo[1];
    corners[5][2] = hi[2];
    corners[5][3] = 1.0f;
    corners[6][0] = lo[0];
    corners[6][1] = hi[1];
    corners[6][2] = hi[2];
    corners[6][3] = 1.0f;
    corners[7][0] = hi[0];
    corners[7][1] = hi[1];
    corners[7][2] = hi[2];
    corners[7][3] = 1.0f;
    DrawBox(corners, r, g, b);
}

/**
 * Draws the edges of a box in world coordinates.
 */
static void DrawBox(float (*corners)[4], int r, int g, int b) {
    mgCDrawPrim prim;
    int vertex[8][4];
    int i;
    int visible;

    prim.Initialize(0, 0);
    prim.DepthTestEnable(1);
    prim.DepthTest(1);
    prim.AlphaTestEnable(0);
    prim.AlphaBlendEnable(1);
    prim.TextureMapEnable(0);
    prim.Coord(1);
    prim.Begin(MG_PRIM_LINE);
    prim.Color(r, g, b, 0x80);
    visible = 1;
    for (i = 0; i < 8; i++) {
        corners[i][3] = 1.0f;
        visible &= mgTransWorldPrim(vertex[i], corners[i]);
    }
    if (visible != 0) {
        prim.Vertex4(vertex[0]);
        prim.Vertex4(vertex[1]);
        prim.Vertex4(vertex[1]);
        prim.Vertex4(vertex[5]);
        prim.Vertex4(vertex[5]);
        prim.Vertex4(vertex[4]);
        prim.Vertex4(vertex[4]);
        prim.Vertex4(vertex[0]);
        prim.Vertex4(vertex[2]);
        prim.Vertex4(vertex[3]);
        prim.Vertex4(vertex[3]);
        prim.Vertex4(vertex[7]);
        prim.Vertex4(vertex[7]);
        prim.Vertex4(vertex[6]);
        prim.Vertex4(vertex[6]);
        prim.Vertex4(vertex[2]);
        prim.Vertex4(vertex[0]);
        prim.Vertex4(vertex[2]);
        prim.Vertex4(vertex[4]);
        prim.Vertex4(vertex[6]);
        prim.Vertex4(vertex[5]);
        prim.Vertex4(vertex[7]);
        prim.Vertex4(vertex[1]);
        prim.Vertex4(vertex[3]);
    }
    prim.End();
}

/**
 * Multiplies a vector by the upper three rows of a matrix.
 */
static void VectMatMul(float *out, float *vec, float (*mat)[4]) {
    sceVu0FVECTOR result;

    result[0] = vec[0] * mat[0][0] + vec[1] * mat[1][0] + vec[2] * mat[2][0];
    result[1] = vec[0] * mat[0][1] + vec[1] * mat[1][1] + vec[2] * mat[2][1];
    result[2] = vec[0] * mat[0][2] + vec[1] * mat[1][2] + vec[2] * mat[2][2];
    result[3] = 1.0f;
    sceVu0CopyVector(out, result);
}

/**
 * Loads the event editor font into its texture bank.
 */
static void evLoadDebugFont(int texture_id, mgCMemory *memory) {

    mgCTextureManager *texManager = &mgTexManager;
    int file_size;
    u8 *buffer;

    memory->Align64();
    buffer = (u8 *)memory->stAllocTest(1);
    if (LoadFile2("img/font3.tm2", buffer, &file_size, 0) != 0) {
        memory->Alloc(file_size / 16 + 1);
        texManager->EnterTexture(texture_id, "font3", (TM2_head *)buffer, 0, 0);
    }
    JisFont.Initialize();
    JisFont.InitTexture(-1, "", -1, "", texture_id, "font3");
    JisFont.Clear();
    JisFont.shadow_enable = 1;
}

/**
 * Moves the camera with the pad, optionally moving its reference point too.
 */
static void MoveCamera(float *pos, float *ref) {
    sceVu0FVECTOR dir;
    sceVu0FVECTOR move;
    float dist;
    float angle;
    float right;
    float up;
    float forward;

    sceVu0SubVector(dir, ref, pos);
    dist = sqrtf(dir[0] * dir[0] + dir[2] * dir[2]);
    angle = atan2f(dir[0], dir[2]);
    right = -GamePad__2.GetLXf();
    if (GamePad__2.On(PAD_R1) != 0) {
        right = 0.04f * dist;
    }
    if (GamePad__2.On(PAD_L1) != 0) {
        right = 0.04f * -dist;
    }
    up = -GamePad__2.GetRYf();
    forward = -GamePad__2.GetLYf();
    move[0] = right * cosf(angle) + forward * sinf(angle);
    move[1] = up;
    move[2] = forward * cosf(angle) - right * sinf(angle);
    if (GamePad__2.On(PAD_CROSS) != 0) {
        sceVu0ScaleVector(move, move, 6.0f);
    }
    sceVu0AddVector(pos, pos, move);
    if (GamePad__2.On(PAD_SQUARE) != 0 && GamePad__2.On(PAD_L1 | PAD_R1) == 0) {
        sceVu0AddVector(ref, ref, move);
    }
}

/**
 * Moves the camera reference point with the pad.
 */
static void MoveCameraRef(float *pos, float *ref) {
    sceVu0FVECTOR dir;
    sceVu0FVECTOR move;
    float dist;
    float angle;
    float right;
    float up;
    float forward;

    sceVu0SubVector(dir, ref, pos);
    dist = sqrtf(dir[0] * dir[0] + dir[2] * dir[2]);
    angle = atan2f(dir[0], dir[2]);
    right = -GamePad__2.GetLXf();
    if (GamePad__2.On(PAD_R1) != 0) {
        right = 0.04f * -dist;
    }
    if (GamePad__2.On(PAD_L1) != 0) {
        right = 0.04f * dist;
    }
    up = GamePad__2.GetRYf();
    forward = -GamePad__2.GetLYf();
    move[0] = right * cosf(angle) + forward * sinf(angle);
    move[1] = up;
    move[2] = forward * cosf(angle) - right * sinf(angle);
    if (GamePad__2.On(PAD_CROSS) != 0) {
        sceVu0ScaleVector(move, move, 6.0f);
    }
    sceVu0AddVector(ref, ref, move);
    if (GamePad__2.On(PAD_SQUARE) != 0 && GamePad__2.On(PAD_L1 | PAD_R1) == 0) {
        sceVu0AddVector(pos, pos, move);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/eventedit", MoveChara__FP11CCharacter2P9mgCCameraP9mgCMemory);
void InitEventEdit(int texb, mgCMemory *memory) {
    g_info.memory = memory;
    g_info.texb = texb;
    g_info.disp = 1;
    g_info.active = 0;
    g_info.mode = EVENT_EDIT_MODE_CAMERA_MOVE;
    g_info.chara_no = 0;
    g_info.collision = 0;
}

int ChkEventEditStart() {
    mgCCamera *camera;

    if (DebugFlag != 1) {
        return 0;
    }
    camera = GetActiveCamera();

    if (GamePad__2.Down(PAD_L3) != 0) {
        g_info.active = 1;
        EdEventInfo.projection = mgGetProjection();
        mgCCamera::StopCamera = 1;
        camera->GetPos(g_info.camera_pos);
        camera->GetRef(g_info.camera_ref);
        evLoadDebugFont(g_info.texb, g_info.memory);
        g_cmr_pas.Initialize();
        g_cmr_pas.SetFrame(200);
        g_cp_cursor = EVENT_EDIT_PAS_ITEM_EDIT_MODE;
        g_cp_mode = EVENT_EDIT_PAS_OP_ADDITION;
        g_cp_selno = 0;
        g_chara_pas.Initialize();
        g_chara_pas.SetFrame(200);
        g_chara_pas_cursor = EVENT_EDIT_PAS_ITEM_EDIT_MODE;
        g_chara_pas_mode = EVENT_EDIT_PAS_OP_ADDITION;
        g_chara_pas_selno = 0;
        GamePad__2.MenuModeOff();
        return 1;
    }
    return 0;
}

int EventEdit(mgCMemory *memory) {
    sceVu0FVECTOR cam_pos;
    sceVu0FVECTOR cam_ref;
    sceVu0FVECTOR focus_pos;
    sceVu0FVECTOR path_eye;
    sceVu0FVECTOR path_look;
    sceVu0FVECTOR chara_pos;
    sceVu0FVECTOR chara_rot;
    sceVu0FVECTOR add_pos;
    sceVu0FVECTOR set_pos;
    mgCCamera *camera;
    CCharacter2 *chara;
    mgCMemory *memoryHeap;
    int frame;
    int index;

    if (DebugFlag != 1) {
        return 0;
    }
    camera = GetActiveCamera();
    if (GamePad__2.Down(PAD_L3) != 0) {
        g_info.active = 0;
        camera->SetPos(g_info.camera_pos);
        camera->SetRef(g_info.camera_ref);
        mgCCamera::StopCamera = 0;
        memoryHeap = g_info.memory;
        memoryHeap->stack_used = 0;
        memoryHeap->lock = 0;
        GamePad__2.MenuModeOff();
        mgTexManager.DeleteBlock(g_info.texb);
    }
    if (g_info.active == 0) {
        return 0;
    }
    mgSetProjection(EdEventInfo.projection);
    camera->GetPos(cam_pos);
    camera->GetRef(cam_ref);
    switch (g_info.mode) {
        case EVENT_EDIT_MODE_CAMERA_MOVE:
            if (GamePad__2.On(PAD_L2) != 0) {
                MoveCameraRef(cam_pos, cam_ref);
            } else {
                MoveCamera(cam_pos, cam_ref);
            }
            camera->SetPos(cam_pos);
            camera->SetRef(cam_ref);
            if (GamePad__2.On(PAD_RIGHT) != 0) {
                EdEventInfo.projection += 1.0f;
            }
            if (GamePad__2.On(PAD_LEFT) != 0) {
                EdEventInfo.projection -= 1.0f;
            }
            if (EdEventInfo.projection < 100.0f) {
                EdEventInfo.projection = 100.0f;
            }
            if (EdEventInfo.projection > 2000.0f) {
                EdEventInfo.projection = 2000.0f;
            }
            break;
        case EVENT_EDIT_MODE_CHARACTER:
            if (GamePad__2.On(PAD_R2) == 0) {
                chara = GetCharacter(g_info.chara_no);
                if (GamePad__2.Down(PAD_RIGHT) != 0) {
                    chara = NULL;
                    for (index = g_info.chara_no + 1; index < 0x80; index++) {
                        chara = GetCharacter(index);
                        if (chara != NULL) {
                            break;
                        }
                    }
                    if (chara == NULL) {
                        chara = GetCharacter(g_info.chara_no);
                    } else {
                        g_info.chara_no = index;
                    }
                }
                if (GamePad__2.Down(PAD_LEFT) != 0) {
                    chara = NULL;
                    for (index = g_info.chara_no - 1; index >= 0; index--) {
                        chara = GetCharacter(index);
                        if (chara != NULL) {
                            break;
                        }
                    }
                    if (chara == NULL) {
                        chara = GetCharacter(g_info.chara_no);
                    } else {
                        g_info.chara_no = index;
                    }
                }
                if (GamePad__2.Down(PAD_SQUARE) != 0) {
                    g_info.collision = !(bool)g_info.collision;
                }
                if (GamePad__2.Down(PAD_TRIANGLE) != 0) {
                    chara->GetPosition(focus_pos);
                    sceVu0CopyVector(cam_ref, focus_pos);
                    cam_ref[1] += 0.7f * chara->body_height;
                    camera->SetRef(cam_ref);
                }
                MoveChara(chara, camera, memory);
            } else {
                if (GamePad__2.On(PAD_L2) != 0) {
                    MoveCameraRef(cam_pos, cam_ref);
                } else {
                    MoveCamera(cam_pos, cam_ref);
                }
                camera->SetPos(cam_pos);
                camera->SetRef(cam_ref);
            }
            break;
        case EVENT_EDIT_MODE_CAMERA_PAS:
            if (GamePad__2.On(PAD_L2) != 0) {
                MoveCameraRef(cam_pos, cam_ref);
            } else {
                MoveCamera(cam_pos, cam_ref);
            }
            camera->SetPos(cam_pos);
            camera->SetRef(cam_ref);
            switch (g_cp_cursor) {
                case EVENT_EDIT_PAS_ITEM_EDIT_MODE:
                    if (GamePad__2.Down(PAD_LEFT) != 0) {
                        g_cp_mode -= 1;
                        if (g_cp_mode < 0) {
                            g_cp_mode = EVENT_EDIT_PAS_OP_ADDITION;
                        }
                    } else if (GamePad__2.Down(PAD_RIGHT) != 0) {
                        g_cp_mode += 1;
                        if (g_cp_mode >= EVENT_EDIT_PAS_OP_COUNT) {
                            g_cp_mode = EVENT_EDIT_PAS_OP_DELETE;
                        }
                    }
                    break;
                case EVENT_EDIT_PAS_ITEM_SELECT_NO:
                    if (GamePad__2.Down(PAD_LEFT) != 0) {
                        g_cp_selno -= 1;
                        if (g_cp_selno < 0) {
                            g_cp_selno = 0;
                        }
                    } else if (GamePad__2.Down(PAD_RIGHT) != 0) {
                        g_cp_selno += 1;
                        if (g_cp_selno >= 0x10) {
                            g_cp_selno = 0xF;
                        }
                    }
                    break;
                case EVENT_EDIT_PAS_ITEM_FRAME:
                    frame = g_cmr_pas.GetFrame();
                    if (GamePad__2.On(PAD_LEFT) != 0) {
                        frame -= 1;
                        if (frame < 0) {
                            frame = 0;
                        }
                    } else if (GamePad__2.On(PAD_RIGHT) != 0) {
                        frame += 1;
                    }
                    g_cmr_pas.SetFrame(frame);
                    break;
            }
            if (GamePad__2.Down(PAD_UP) != 0) {
                g_cp_cursor -= 1;
                if (g_cp_cursor < 0) {
                    g_cp_cursor = EVENT_EDIT_PAS_ITEM_EDIT_MODE;
                }
            } else if (GamePad__2.Down(PAD_DOWN) != 0) {
                g_cp_cursor += 1;
                if (g_cp_cursor >= EVENT_EDIT_PAS_ITEM_COUNT) {
                    g_cp_cursor = EVENT_EDIT_PAS_ITEM_FRAME;
                }
            }
            if (GamePad__2.Down(PAD_R2) != 0 && g_cp_selno < g_cmr_pas.pas_num) {
                g_cmr_pas.GetCameraPas(g_cp_selno, path_eye, path_look);
                camera->SetPos(path_eye);
                camera->SetRef(path_look);
            }
            if (GamePad__2.Down(PAD_CIRCLE) != 0) {
                camera->GetPos(cam_pos);
                camera->GetRef(cam_ref);
                switch (g_cp_mode) {
                    case EVENT_EDIT_PAS_OP_ADDITION:
                        g_cmr_pas.AddCameraPas(cam_pos, cam_ref);
                        break;
                    case EVENT_EDIT_PAS_OP_INSERT:
                        g_cmr_pas.InsCameraPas(g_cp_selno, cam_pos, cam_ref);
                        break;
                    case EVENT_EDIT_PAS_OP_OVERWRITE:
                        g_cmr_pas.SetCameraPas(g_cp_selno, cam_pos, cam_ref);
                        break;
                    case EVENT_EDIT_PAS_OP_DELETE:
                        g_cmr_pas.DelCameraPas(g_cp_selno);
                        break;
                }
            }
            if (g_cmr_pas.CheckEnd() == 0) {
                g_cmr_pas.Step(cam_pos, cam_ref);
                camera->SetPos(cam_pos);
                camera->SetRef(cam_ref);
            }
            if (GamePad__2.Down(PAD_TRIANGLE) != 0) {
                g_cmr_pas.Setup();
                g_cmr_pas.Run();
            }
            break;
        case EVENT_EDIT_MODE_CHARA_PAS:
            if (GamePad__2.Down(PAD_TRIANGLE) != 0) {
                g_chara_pas.Setup();
                g_chara_pas.Run();
            }
            if (g_chara_pas.CheckEnd() == 0) {
                chara = GetCharacter(g_info.chara_no);
                chara->GetPosition(chara_pos);
                chara->GetRotation(chara_rot);
                g_chara_pas.Step(chara_pos, &chara_rot[1]);
                chara->SetPosition(chara_pos);
                chara->SetRotation(chara_rot);
            } else {
                if (GamePad__2.On(PAD_R2) == 0) {
                    MoveChara(GetCharacter(g_info.chara_no), camera, memory);
                } else {
                    if (GamePad__2.On(PAD_L2) != 0) {
                        MoveCameraRef(cam_pos, cam_ref);
                    } else {
                        MoveCamera(cam_pos, cam_ref);
                    }
                    camera->SetPos(cam_pos);
                    camera->SetRef(cam_ref);
                }
                if (GamePad__2.Down(PAD_UP) != 0) {
                    g_chara_pas_cursor -= 1;
                    if (g_chara_pas_cursor < 0) {
                        g_chara_pas_cursor = EVENT_EDIT_PAS_ITEM_EDIT_MODE;
                    }
                } else if (GamePad__2.Down(PAD_DOWN) != 0) {
                    g_chara_pas_cursor += 1;
                    if (g_chara_pas_cursor >= EVENT_EDIT_PAS_ITEM_COUNT) {
                        g_chara_pas_cursor = EVENT_EDIT_PAS_ITEM_FRAME;
                    }
                }
                switch (g_chara_pas_cursor) {
                    case EVENT_EDIT_PAS_ITEM_EDIT_MODE:
                        if (GamePad__2.Down(PAD_LEFT) != 0) {
                            g_chara_pas_mode -= 1;
                            if (g_chara_pas_mode < 0) {
                                g_chara_pas_mode = EVENT_EDIT_PAS_OP_ADDITION;
                            }
                        } else if (GamePad__2.Down(PAD_RIGHT) != 0) {
                            g_chara_pas_mode += 1;
                            if (g_chara_pas_mode >= EVENT_EDIT_PAS_OP_COUNT) {
                                g_chara_pas_mode = EVENT_EDIT_PAS_OP_DELETE;
                            }
                        }
                        break;
                    case EVENT_EDIT_PAS_ITEM_SELECT_NO:
                        if (GamePad__2.Down(PAD_LEFT) != 0) {
                            g_chara_pas_selno -= 1;
                            if (g_chara_pas_selno < 0) {
                                g_chara_pas_selno = 0;
                            }
                        } else if (GamePad__2.Down(PAD_RIGHT) != 0) {
                            g_chara_pas_selno += 1;
                            if (g_chara_pas_selno >= 0x10) {
                                g_chara_pas_selno = 0xF;
                            }
                        }
                        break;
                    case EVENT_EDIT_PAS_ITEM_FRAME:
                        frame = g_chara_pas.GetFrame();
                        if (GamePad__2.On(PAD_LEFT) != 0) {
                            frame -= 1;
                            if (frame < 0) {
                                frame = 0;
                            }
                        } else if (GamePad__2.On(PAD_RIGHT) != 0) {
                            frame += 1;
                        }
                        g_chara_pas.SetFrame(frame);
                        break;
                }
                if (GamePad__2.Down(PAD_CIRCLE) != 0) {
                    chara = GetCharacter(g_info.chara_no);
                    chara->GetPosition(add_pos);
                    switch (g_chara_pas_mode) {
                        case EVENT_EDIT_PAS_OP_ADDITION:
                            g_chara_pas.AddCharaPas(add_pos);
                            break;
                        case EVENT_EDIT_PAS_OP_INSERT:
                            g_chara_pas.InsCharaPas(g_chara_pas_selno, add_pos);
                            break;
                        case EVENT_EDIT_PAS_OP_OVERWRITE:
                            g_chara_pas.SetCharaPas(g_chara_pas_selno, add_pos);
                            break;
                        case EVENT_EDIT_PAS_OP_DELETE:
                            g_chara_pas.DelCharaPas(g_chara_pas_selno);
                            break;
                    }
                }
                if (GamePad__2.Down(PAD_SQUARE) != 0 &&
                    g_chara_pas_selno < g_chara_pas.pas_num) {
                    g_chara_pas.GetCharaPas(g_chara_pas_selno, set_pos);
                    chara = GetCharacter(g_info.chara_no);
                    chara->SetPosition(set_pos);
                }
            }
            break;
    }
    if (GamePad__2.Down(PAD_SELECT) != 0) {
        g_info.mode += 1;
        if (g_info.mode > EVENT_EDIT_MODE_CHARA_PAS) {
            g_info.mode = EVENT_EDIT_MODE_CAMERA_MOVE;
        }
    }
    if (GamePad__2.Down2(PAD_L3) != 0) {
        g_info.disp = !g_info.disp;
    }
    if (GamePad__2.Down(PAD_START) != 0) {
        OutPutFile();
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/eventedit", DrawEventEdit__Fv);

// Static initialiser (.init)

// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1208__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1226__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1242__2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_809__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_810__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_811__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_812__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_813__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_814__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_815__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_816__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_817__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_818__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_819__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_820__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_821__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_822__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_823__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_824__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_825__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_826__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_827__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_828__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_829__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_830__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_831__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_832__5__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_889__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_890__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_891__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_979__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1204__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1205__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1206__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1207__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1222__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1223__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1224__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1225__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1382__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1383__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1384__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1385__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1386__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1387__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1388__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1389__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1390__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1391__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1392__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1393__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1394__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1395__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1396__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1397__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1398__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1399__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1400__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1401__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1402__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/eventedit", at_1403__2__DATA);

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)
INCLUDE_BSS(g_cp_mode, 0x4);
INCLUDE_BSS(g_cp_cursor, 0x4);
INCLUDE_BSS(g_cp_selno, 0x4);
INCLUDE_BSS(g_chara_pas_mode, 0x4);
INCLUDE_BSS(g_chara_pas_cursor, 0x4);
INCLUDE_BSS(g_chara_pas_selno, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(g_cmr_pas, 0x950);
INCLUDE_BSS(g_chara_pas, 0x4B0);
INCLUDE_BSS(g_info, 0x40);
