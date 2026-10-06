#include "common.h"
#include "event_func.hpp"
#include "actionchara.hpp"
#include "automap.hpp"
#include "cameracontrol.hpp"
#include "character.hpp"
#include "collision.hpp"
#include "colprim.hpp"
#include "dataread.hpp"
#include "dng_event.hpp"
#include "dng_main.hpp"
#include "dngmenu.hpp"
#include "editdata.hpp"
#include "editevent.hpp"
#include "editinfo.hpp"
#include "editmap.hpp"
#include "editmenu.hpp"
#include "effscript.hpp"
#include "event.hpp"
#include "eventsprite.hpp"
#include "gamedata.hpp"
#include "gamepad.hpp"
#include "gameutil.hpp"
#include "gyorace.hpp"
#include "intersection.hpp"
#include "inventmn.hpp"
#include "mainloop.hpp"
#include "mapinfo.hpp"
#include "mapjump.hpp"
#include "mapparts.hpp"
#include "mapselect.hpp"
#include "menuaqua.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "menumain.hpp"
#include "mg_camera.hpp"
#include "mg_drawenv.hpp"
#include "mg_drawprim.hpp"
#include "mg_frame.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mglib.hpp"
#include "movie.hpp"
#include "nd_meswin.hpp"
#include "nowload.hpp"
#include "npccfg.hpp"
#include "padcontrol.hpp"
#include "pot.hpp"
#include "quest.hpp"
#include "runscript_opcodes.hpp"
#include "savedata.hpp"
#include "snd_mngr.hpp"
#include "sound.hpp"
#include "sphida.hpp"
#include "subgame.hpp"
#include "swordeffect.hpp"
#include "sysmes.hpp"
#include "userdata.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <libcdvd.h>

typedef int (*EventFunc)(RS_STACKDATA *, int);
static CEventScriptArg *nowScriptArg; /**< Argument builder used by the running argument script. */
extern RS_EXTFUNC_INFO ext_func_info__2[];
static EventFunc ext_func[0x5dc];
static u_long128 event_snd_buff[0x801];
static u_long128 event_snd2_buff[0x141];
static CEffectScriptMan *EventEffectScript; /**< Effect script used by the event. */
static CSWordAfterImage *SwordEffect; /**< Sword afterimage used by the event. */
const int hit_effect_num = 5;
const int event_sprite2_num = 0x30;
const int sprite_type_world = 1;
const float sprite_ground_offset = 32.0f;
const int status_no_shadow = 8;
const int script_stack_slots = 0x20;
const int script_call_slots = script_stack_slots;
const int script_func_slots = 3;
const int script_run_id = 0x64;
const float raster_max = 3.1415927f;
const int half_color = 0x80;
const int seq_node_num = 0x100;
const int hit_spark_num = 0x40;
const int event_snd_buffer_size = 0x801;
const int event_snd2_buffer_size = 0x141;
const int memory_name_max = 0x10;
const int paku_name_size = 0x40;
const int pack_file_max = 0x80;
const int type_loaded = 2;
const int event_func_slots = 0x5DC;
const int event_local_num = 64;
const int object_seq_num = 32;
const int exit_edit_mode = 17;
const int event_stream = 1;
const int stream_max_volume = 0x7FFF;
const int vpk_entry_count = 164;
const int menu_dng_map = 3;
const int menu_select_party = 4;
const int menu_use_item = 9;
const int menu_draw_chapter = 11;
const int exit_start_loop = EVENT_REQUEST_GOTO;
const int exit_enter_interior = EVENT_REQUEST_INTERIOR;
const int exit_leave_interior = EVENT_REQUEST_OUTSIDE;
const int exit_map_jump = EVENT_REQUEST_MAP_JUMP;
const int request_menu = EVENT_COMMAND_SUB_MODE;
const int request_door = EVENT_COMMAND_DOOR;

ED_EVENT_INFO EdEventInfo;
CEohMother EventObjHandleMother;
CEventSpriteMother esMother;
u32 EventLocalFlag[0x40];
int EventLocalCnt[0x40];
CRain EventRain;
CMarker EventMarker;
HIT_EFFECT_PARTICLE Hit_para[EVENT_HIT_EFFECT_NUM][EVENT_HIT_PARTICLE_NUM];
CHitEffectImage HitEffect[EVENT_HIT_EFFECT_NUM];
char PakuAnimName[0x40];
char PakuAnimName2[0x40];
char PakuMotionName[0x40];
char PakuMotionName2[0x40];
static mgCMemory BuffEventSnd;
static mgCMemory BuffEventSnd2;
static CDngFreeMap EventDngMap;
static CSceneCmrSeq CameraSeq;
static CSceneObjSeq ObjectSeq[32];
static CEventSprite2 EventSprite2[0x30];
static CEventScriptArg EventScriptArg;
CScreenEffect EventScreenEffect;
RS_STACKDATA *p_use_item;
int SetWorldCoordFlg;
int PakuAnimEohNo;
int PakuMotionEohNo;
int PakuMotionType;
int PakuMotionType2;

static void GetStackVector(float *out, RS_STACKDATA *stack);
static void FileNameConvLanguage(char *name);
static int GetStackInt(RS_STACKDATA *stack);
static float GetStackFloat(RS_STACKDATA *stack);
static char *GetStackString(RS_STACKDATA *stack);
static void SetStack(RS_STACKDATA *stack, int value);
static void SetStack(RS_STACKDATA *stack, float value);
static int _ID_OFFSET(RS_STACKDATA *stack, int argc);
static int GetArgInt(ARG_DATA *arg);
static float GetArgFloat(ARG_DATA *arg);
static char *GetArgString(ARG_DATA *arg);
static void GetArgVector(float *vec, ARG_DATA *arg);
static CSceneObjSeq *GetObjSeq(int index);
static int _GET_PADON(RS_STACKDATA *stack, int argc);
static int _GET_PADDOWN(RS_STACKDATA *stack, int argc);
static int _GET_PADUP(RS_STACKDATA *stack, int argc);
static int _GET_APAD(RS_STACKDATA *stack, int argc);
static int _GOTO_INTERIOR(RS_STACKDATA *stack, int argc);
static int _GOTO_OUTSIDE(RS_STACKDATA *stack, int argc);
static int _INITIALIZE(RS_STACKDATA *stack, int argc);
static int _CHARA_ACTIVE(RS_STACKDATA *stack, int argc);
static int _CLEAR_STACK(RS_STACKDATA *stack, int argc);
static int _ASSIGN_STACK(RS_STACKDATA *stack, int argc);
static int _SET_FLAG(RS_STACKDATA *stack, int argc);
static int _SET_CNT(RS_STACKDATA *stack, int argc);
static int _GET_CNT(RS_STACKDATA *stack, int argc);
static int _SET_CURRENT_DIR(RS_STACKDATA *stack, int argc);
static int _CHANGE_DIR(RS_STACKDATA *stack, int argc);
static int _DELETE_CHARA(RS_STACKDATA *stack, int argc);
static int _LOAD_MOTION(RS_STACKDATA *stack, int argc);
static int _MAP_JUMP(RS_STACKDATA *stack, int argc);
static int _SET_RAIN(RS_STACKDATA *stack, int argc);
static int _DEL_EXT_MOTION(RS_STACKDATA *stack, int argc);
static s32 _SET_MARKER(RS_STACKDATA *stack, int argc);
static void _FINISH(RS_STACKDATA *stack, int argc);
static int _GET_DUN_WORLD_COORD(RS_STACKDATA *stack, int argc);
static int _DEL_IMG(RS_STACKDATA *stack, int argc);
static int _SET_DNG_MAP(RS_STACKDATA *stack, int argc);
static int _LOAD_ITEM(RS_STACKDATA *stack, int argc);
static int _GOTO_USE_ITEM(RS_STACKDATA *stack, int argc);
static int _SET_LOCAL_FLAG(RS_STACKDATA *stack, int argc);
static int _GET_LOCAL_FLAG(RS_STACKDATA *stack, int argc);
static int _GOTO_SELECT_PARTY(RS_STACKDATA *stack, int argc);
static int _SET_LOADBG_FILE_MONS_TALK(RS_STACKDATA *stack, int argc);
static int _CHECK_LOADBG_FILE(RS_STACKDATA *stack, int argc);
static s32 _GET_TB_ITEMNO(RS_STACKDATA *stack, int argc);
static int _ADD_ITEM(RS_STACKDATA *stack, int argc);
static int _SUB_ITEM(RS_STACKDATA *stack, int argc);
static int _GET_ITEM_TYPE(RS_STACKDATA *stack, int argc);
static int _GET_ITEM_SPACE(RS_STACKDATA *stack, int argc);
static s32 _INIT_LOCAL_CNT(RS_STACKDATA *stack, s32 argc);
static int _SET_TIME(RS_STACKDATA *stack, int argc);
static int _SET_ACTIVE_LIGHT(RS_STACKDATA *stack, int argc);
static s32 _RESET_PAKU_ANIM(RS_STACKDATA *stack, s32 argc);
static int _TRG_PAKU_ANIM(RS_STACKDATA *stack, int argc);
static int _GET_ACTIVE_CHR_NO(RS_STACKDATA *stack, int argc);
static int _SET_ACTIVE_CHR_NO(RS_STACKDATA *stack, int argc);
static int _DNG_SET_FLOOR_ID(RS_STACKDATA *stack, int argc);
static int _DNG_GET_FLOOR_ID(RS_STACKDATA *stack, int argc);
static int _SET_PAKU_MOTION(RS_STACKDATA *stack, int argc);
static s32 _RESET_PAKU_MOTION(RS_STACKDATA *stack, s32 argc);
static int _TRG_PAKU_MOTION(RS_STACKDATA *stack, int argc);
static int _GOTO_DNG_MAP(RS_STACKDATA *stack, int argc);
static int _GOTO_DNG(RS_STACKDATA *stack, int argc);
static int _GOTO_EDIT(RS_STACKDATA *stack, int argc);
static int _LOAD_CHARA_NPC(RS_STACKDATA *stack, int argc);
static int _AUTO_SET_MONSTER(RS_STACKDATA *stack, int argc);
static int _LOAD_MONSTER_FILE(RS_STACKDATA *stack, int argc);
static int _GET_NPC_STATUS(RS_STACKDATA *stack, int argc);
static int _SET_NPC_STATUS(RS_STACKDATA *stack, int argc);
static int _GET_NOW_PARTY_CHARA(RS_STACKDATA *stack, int argc);
static int _SET_LOCAL_CNT(RS_STACKDATA *stack, int argc);
static int _GET_LOCAL_CNT(RS_STACKDATA *stack, int argc);
static int _GET_LOCAL_CNT2(RS_STACKDATA *stack, int argc);
static int _GOTO_DRAW_CHAPTER(RS_STACKDATA *stack, int argc);
static int _SET_PROJECTION(RS_STACKDATA *stack, int argc);
static int _GET_PROJECTION(RS_STACKDATA *stack, int argc);
static int _DNG_DEBUG_COMMAND(RS_STACKDATA *stack, int argc);
static int _CD_SEEK(RS_STACKDATA *stack, int argc);
static int _SET_MOTION_BLUR(RS_STACKDATA *stack, int argc);
static int _SET_TALK_CAMERA(RS_STACKDATA *stack, int argc);
static int _HIT_EFFECT(RS_STACKDATA *stack, int argc);
static int _GET_START_BUTTON(RS_STACKDATA *stack, int argc);
static int _MOVE_INTERIOR(RS_STACKDATA *stack, int argc);
static int _GET_NOW_MAP_NO(RS_STACKDATA *stack, int argc);
static int _GET_NOW_SUBMAP_NO(RS_STACKDATA *stack, int argc);
static int _GET_OLD_MAP_NO(RS_STACKDATA *stack, int argc);
static int _GET_OLD_SUBMAP_NO(RS_STACKDATA *stack, int argc);
static int _SET_RAIN_CHARA_NO(RS_STACKDATA *stack, int argc);
static s32 _GET_CONTENTS_POS(RS_STACKDATA *stack, int argc);
static int _GET_BPOT_POS(RS_STACKDATA *stack, int argc);
static int _GET_BPOT_STATUS(RS_STACKDATA *stack, int argc);
static s32 _GET_PERSON_STATUS(RS_STACKDATA *stack, int argc);
static int _GET_CONTROL_CHRID(RS_STACKDATA *stack, int argc);
static int _SET_CAMERA_NEXT_REF(RS_STACKDATA *stack, int argc);
static int _GOTO_MENU(RS_STACKDATA *stack, int argc);
static s32 _GET_MENU_STATUS(RS_STACKDATA *stack, int argc);
static int _LOAD_EQUIP(RS_STACKDATA *stack, int argc);
static int _GET_EQUIP_ITEMNO(RS_STACKDATA *stack, int argc);
static int _SET_TIME_STEP_ENABLE(RS_STACKDATA *stack, int argc);
static s32 _SET_DOOR_MATERIAL(RS_STACKDATA *stack, int argc);
static s32 _INIT_DRAMA_SCENE(RS_STACKDATA *stack, s32 argc);
static int _SET_ACTIVE_CMRID(RS_STACKDATA *stack, int argc);
static int _SET_BEFORE_CMRID(RS_STACKDATA *stack, int argc);
static int _DNGMAP_LOAD(RS_STACKDATA *stack, int argc);
static int _DNGMAP_DELETE(RS_STACKDATA *stack, int argc);
static int _DNGMAP_MOVE_PIECE(RS_STACKDATA *stack, int argc);
static int _DNGMAP_ONOFF(RS_STACKDATA *stack, int argc);
static int _DNGMAP_SET_FADE(RS_STACKDATA *stack, int argc);
static int _GET_BEFORE_CAMERA_NEXT_POS(RS_STACKDATA *stack, int argc);
static int _GET_BEFORE_CAMERA_NEXT_REF(RS_STACKDATA *stack, int argc);
static int _SET_CAMERA_NEXT_POS(RS_STACKDATA *stack, int argc);
static int _SET_FCAMERA_FOLLOW_FLAG(RS_STACKDATA *stack, int argc);
static int _FCAMERA_STEP(RS_STACKDATA *stack, int argc);
static int _GET_REF_ANGLE(RS_STACKDATA *stack, int argc);
static int _DNG_SET_STAGE_ID(RS_STACKDATA *stack, int argc);
static int _DNG_GET_STAGE_ID(RS_STACKDATA *stack, int argc);
static int _SET_CAMERA_CTRL(RS_STACKDATA *stack, int argc);
static int _GET_FCAMERA_ANGLE(RS_STACKDATA *stack, int argc);
static int _GET_FCAMERA_HEIGHT(RS_STACKDATA *stack, int argc);
static int _GET_FCAMERA_DIST(RS_STACKDATA *stack, int argc);
static int _GET_INVENTION_ID(RS_STACKDATA *stack, int argc);
static int _FUNCTION_MAP_JUMP(RS_STACKDATA *stack, int argc);
static int _FUNCTION_DOOR_MODE(RS_STACKDATA *stack, int argc);
static int _GET_MONEY(RS_STACKDATA *stack, int argc);
static int _ADD_MONEY(RS_STACKDATA *stack, int argc);
static int _GET_ITEM_NUM(RS_STACKDATA *stack, int argc);
static int _CHECK_BUTTON(RS_STACKDATA *stack, int argc);
static int _GET_LANGUAGE(RS_STACKDATA *stack, int argc);
static int _CHECK_INVENT_ITEM(RS_STACKDATA *stack, int argc);
static int _SET_AI(RS_STACKDATA *stack, int argc);
static int _GET_PHOTO_NUM(RS_STACKDATA *stack, int argc);
static s32 _SET_CONTENTS_ETC(RS_STACKDATA *stack, int argc);
static int _GOTO_SUBGAME(RS_STACKDATA *stack, int argc);
static int _GET_GYORACE_ETC(RS_STACKDATA *stack, int argc);
static int _SET_SAVEDATA_ETC(RS_STACKDATA *stack, int argc);
static void _GEORAMA_FUNC(RS_STACKDATA *stack, int argc);
static int _GOTO_EDITMODE(RS_STACKDATA *stack, int argc);
static int _GET_CHAPTER(RS_STACKDATA *stack, int argc);
static int _EYE_VIEW_DRAW_ON_OFF(RS_STACKDATA *stack, int argc);
static int _GET_QUEST_ETC(RS_STACKDATA *stack, int argc);
static int _GET_OLD_INTERIOR_MAP_NO(RS_STACKDATA *stack, int argc);
static int _SET_EVENT_DATA(RS_STACKDATA *stack, int argc);
static int _SET_FUNC_ETC(RS_STACKDATA *stack, int argc);
static CCharacter2 *GetChara(int id);
static int _GET_CHARA_POS(RS_STACKDATA *stack, int argc);
static int _GET_CHARA_TALK_POS(RS_STACKDATA *stack, int argc);
static int _TURN_CHARA(RS_STACKDATA *stack, int argc);
static int _GET_CHARA_ROT(RS_STACKDATA *stack, int argc);
static int _SET_TEX_ANIM(RS_STACKDATA *stack, int argc);
static int _SET_REFERENCE(RS_STACKDATA *stack, int argc);
static int _DEL_REFERENCE(RS_STACKDATA *stack, int argc);
static int _SHADOW_CLIP_OFF(RS_STACKDATA *stack, int argc);
static int _GET_CHARA_WIDTH(RS_STACKDATA *stack, int argc);

static int _DATA(RS_STACKDATA *stack, int argc);

static RS_EXTFUNC_INFO esa_ext_func_info[] = {
    {_DATA, 0},
    {_ID_OFFSET, 1},
    {NULL, 0}
};


static int _GET_CHARA_HEIGHT(RS_STACKDATA *stack, int argc);
static int _GET_CHARA_WEIGHT(RS_STACKDATA *stack, int argc);
static int _CHARA_DA_ENABLE(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_EX_SOUNDID(RS_STACKDATA *stack, int argc);
static int _ACTCHR_SOUND_INFO_COPY(RS_STACKDATA *stack, int argc);
static ClsMes *GetMes(int id);
static int _MES_CLOSE(RS_STACKDATA *stack, int argc);
static int _MES_NEXTPAGE(RS_STACKDATA *stack, int argc);
static int _SET_MES_AUTOSET(RS_STACKDATA *stack, int argc);
static int _SET_MES_SHIPPO(RS_STACKDATA *stack, int argc);
static int _SET_MES_POS(RS_STACKDATA *stack, int argc);
static int _SET_MES_CURSOR(RS_STACKDATA *stack, int argc);
static int _SET_MES_OKURI(RS_STACKDATA *stack, int argc);
static int _SET_MES_WIN_FLAG(RS_STACKDATA *stack, int argc);
static int _CHECK_MES_COMPLETE(RS_STACKDATA *stack, int argc);
static int _CHECK_MES_WAIT(RS_STACKDATA *stack, int argc);
static int _CHECK_MES(RS_STACKDATA *stack, int argc);
static int _SET_MES_FUKIDASHI(RS_STACKDATA *stack, int argc);
static int _SET_MES_WINDOW_MODE(RS_STACKDATA *stack, int argc);
static int _SET_MES_PRESET(RS_STACKDATA *stack, int argc);
static int _SET_MES_ITEM_DIRECT(RS_STACKDATA *stack, int argc);
static int _SET_MES_ITEM(RS_STACKDATA *stack, int argc);
static int _SET_MES_VALUE(RS_STACKDATA *stack, int argc);
static int _GET_MES_STATUS(RS_STACKDATA *stack, int argc);
static int _GET_PARTY_CHARA_MES_NO(RS_STACKDATA *stack, int argc);
static int _MES_SET_BUFF(RS_STACKDATA *stack, int argc);
static int _GET_MES_WINDOW_MODE(RS_STACKDATA *stack, int argc);
static int _GET_MES_VOICE(RS_STACKDATA *stack, int argc);
static int _SET_MES_QUESTION_GYOU(RS_STACKDATA *stack, int argc);
static int _GET_MES_QUESTION_GYOU(RS_STACKDATA *stack, int argc);
static int _SET_MES_CLOSE_CNT(RS_STACKDATA *stack, int argc);
static int _GET_MES_OKURI(RS_STACKDATA *stack, int argc);
static int _GET_OMAKE_FLAG(RS_STACKDATA *stack, int argc);
static int _SET_OMAKE_FLAG(RS_STACKDATA *stack, int argc);
static int _GET_CAMERA_POS(RS_STACKDATA *stack, int argc);
static int _GET_CAMERA_REF(RS_STACKDATA *stack, int argc);
static int _CAMERA_STEP(RS_STACKDATA *stack, int argc);
static int _GET_BEFORE_CAMERA_POS(RS_STACKDATA *stack, int argc);
static int _GET_BEFORE_CAMERA_REF(RS_STACKDATA *stack, int argc);
static s32 _ASQ_INIT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_SYNC_CHARA(RS_STACKDATA *stack, int argc);
static s32 _ASQ_SET_POS(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOVE(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOVE_STEP(RS_STACKDATA *stack, int argc);
static s32 _ASQ_ROT_REF(RS_STACKDATA *stack, int argc);
static s32 _ASQ_ROT_ANGLE(RS_STACKDATA *stack, int argc);
static s32 _ASQ_CLEAR_ROT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_WAIT_ROT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_ROT_MOVE(RS_STACKDATA *stack, int argc);
static s32 _ASQ_SET_ROT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_DELAY_ROT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOTION_TRG(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOTION_PLAY(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOTION_STOP(RS_STACKDATA *stack, int argc);
static s32 _ASQ_MOTION_NEXT(RS_STACKDATA *stack, int argc);
static s32 _ASQ_ANIME_TRG(RS_STACKDATA *stack, int argc);
static s32 _ASQ_ANIME(RS_STACKDATA *stack, int argc);
static s32 _ASQ_SE_PLAY(RS_STACKDATA *stack, int argc);
static void _IMG_SET_DRAW(RS_STACKDATA *stack, int argc);
static void _IMG_SET_GET(RS_STACKDATA *stack, int argc);
static void _IMG_SET_PUT(RS_STACKDATA *stack, int argc);
static void _IMG_SET_MOVE(RS_STACKDATA *stack, int argc);
static void _IMG_SET_FADE(RS_STACKDATA *stack, int argc);
static void _IMG_SET_COLOR(RS_STACKDATA *stack, int argc);
static int _SPRITE_INIT(RS_STACKDATA *stack, int argc);
static int _SPRITE_SET_DRAW(RS_STACKDATA *stack, int argc);
static int _SPRITE_SET_TYPE(RS_STACKDATA *stack, int argc);
static int _SPRITE_SET_ALPHAB(RS_STACKDATA *stack, int argc);
static int _CMRS_CHECK(RS_STACKDATA *stack, int argc);
static int _CMRS_INIT(RS_STACKDATA *stack, int argc);
static int _CMRS_PRDELAY(RS_STACKDATA *stack, int argc);
static int _CMRS_AHDDELAY(RS_STACKDATA *stack, int argc);
static int _CMRS_SET_ANGLE(RS_STACKDATA *stack, int argc);
static int _CMRS_SET_HEIGHT(RS_STACKDATA *stack, int argc);
static int _CMRS_SET_DIST(RS_STACKDATA *stack, int argc);
static int _CMRS_INIT_PAS(RS_STACKDATA *stack, int argc);
static int _CMRS_SET_PAS_FRM(RS_STACKDATA *stack, int argc);
static int _CMRS_START_PAS(RS_STACKDATA *stack, int argc);
static int _CMRS_PR_KEEP(RS_STACKDATA *stack, int argc);
static int _CMRS_PR_RETURN(RS_STACKDATA *stack, int argc);
static int _CMRS_RELEASE_OBJ(RS_STACKDATA *stack, int argc);
static int _CMRS_AHD_KEEP(RS_STACKDATA *stack, int argc);
static int _CMRS_AHD_RETURN(RS_STACKDATA *stack, int argc);
static int _CMRS_FADE_DELAY(RS_STACKDATA *stack, int argc);
static int _CMRS_FADE_INIT(RS_STACKDATA *stack, int argc);
static int _CMRS_QUAKE_DELAY(RS_STACKDATA *stack, int argc);
static int _CMRS_CHARA_DELAY(RS_STACKDATA *stack, int argc);
static int _CMRS_CHARA_ATTACH(RS_STACKDATA *stack, int argc);
static int _OBJS_CHECK(RS_STACKDATA *stack, int argc);
static int _OBJS_INIT(RS_STACKDATA *stack, int argc);
static int _OBJS_SYNC_OBJ(RS_STACKDATA *stack, int argc);
static int _OBJS_POS_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_SET_POS(RS_STACKDATA *stack, int argc);
static int _OBJS_INIT_PAS(RS_STACKDATA *stack, int argc);
static int _OBJS_SET_PAS_FRM(RS_STACKDATA *stack, int argc);
static int _OBJS_ADD_PAS(RS_STACKDATA *stack, int argc);
static int _OBJS_START_PAS(RS_STACKDATA *stack, int argc);
static int _OBJS_SET_EOH_FRAME_POS(RS_STACKDATA *stack, int argc);
static int _OBJS_ATTACH_CAMERA(RS_STACKDATA *stack, int argc);
static int _OBJS_ROT_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_SET_ROT(RS_STACKDATA *stack, int argc);
static int _OBJS_MOTION_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_MOTION_WAIT(RS_STACKDATA *stack, int argc);
static int _OBJS_SEQ_MOT_TRG(RS_STACKDATA *stack, int argc);
static int _OBJS_SEQ_MOT_TRG_WAIT(RS_STACKDATA *stack, int argc);
static int _OBJS_RESET_MOTION(RS_STACKDATA *stack, int argc);
static int _OBJS_TEXA_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_COLOR_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_SET_COLOR(RS_STACKDATA *stack, int argc);
static int _OBJS_SCALE_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_SE_DELAY(RS_STACKDATA *stack, int argc);
static int _OBJS_RESET_DA_POSITION(RS_STACKDATA *stack, int argc);
static int _OBJS_NORMAL_DRIVE(RS_STACKDATA *stack, int argc);
static s32 _ASQ_CHECK(RS_STACKDATA *stack, int argc);
static int _SND_INIT_PORT(RS_STACKDATA *stack, int argc);
static int _SND_SE_PAUSE(RS_STACKDATA *stack, int argc);
static int _SND_SE_PLAY(RS_STACKDATA *stack, int argc);
static int _SND_SE_STOP(RS_STACKDATA *stack, int argc);
static int _SND_SET_SE_VOL(RS_STACKDATA *stack, int argc);
static int _SND_SET_SE_PAN(RS_STACKDATA *stack, int argc);
static int _SND_SET_SE_PITCH(RS_STACKDATA *stack, int argc);
static int _SND_SE_ALL_STOP(RS_STACKDATA *stack, int argc);
static int _LOAD_BGM(RS_STACKDATA *stack, int argc);
static int _PLAY_BGM(RS_STACKDATA *stack, int argc);
static int _STOP_BGM(RS_STACKDATA *stack, int argc);
static int _STREAM_PLAY(RS_STACKDATA *stack, int argc);
static int _STREAM_STOP(RS_STACKDATA *stack, int argc);
static int _STREAM_STANDBY(RS_STACKDATA *stack, int argc);
static int _STREAM_GET_STATUS(RS_STACKDATA *stack, int argc);
static int _GET_SYS_SND_ID(RS_STACKDATA *stack, int argc);
static int _STREAM_OPEN_CHECK(RS_STACKDATA *stack, int argc);
static int _LOAD_SE_ENV(RS_STACKDATA *stack, int argc);
static int _PLAY_ENV_BGM(RS_STACKDATA *stack, int argc);
static int _SYS_SE_PLAY(RS_STACKDATA *stack, int argc);
static int _INIT_SE_SRC(RS_STACKDATA *stack, int argc);
static int _INIT_SE_ENV(RS_STACKDATA *stack, int argc);
static int _INIT_SE_BAS(RS_STACKDATA *stack, int argc);
static int _LOAD_SE_SRC(RS_STACKDATA *stack, int argc);
static s32 _LOAD_SE_FOOT(RS_STACKDATA *stack, int argc);
static s32 _LOAD_SE_DOOR(RS_STACKDATA *stack, int argc);
static s32 _LOAD_SE_BOX(RS_STACKDATA *stack, int argc);
static int _LOAD_SE_BATTLE(RS_STACKDATA *stack, int argc);
static int _SND_DELETE_PORT(RS_STACKDATA *stack, int argc);
static int _FADE_IN_BGM(RS_STACKDATA *stack, int argc);
static int _FADE_OUT_BGM(RS_STACKDATA *stack, int argc);
static int _STOP_ENV_BGM(RS_STACKDATA *stack, int argc);
static s32 _SND_SET_REVERB(RS_STACKDATA *stack, int argc);
static int _SND_SET_ENV_VOL(RS_STACKDATA *stack, int argc);
static int _STREAM_SILENT_CHECK(RS_STACKDATA *stack, int argc);
static int _AUTO_CHANGE_ENV(RS_STACKDATA *stack, int argc);
static int _BGM_LOAD_CANCEL(RS_STACKDATA *stack, int argc);
static int _SOUND_LOAD_CANCEL(RS_STACKDATA *stack, int argc);
static int _BGM_LOAD_ENABLE(RS_STACKDATA *stack, int argc);
static int _SOUND_LOAD_ENABLE(RS_STACKDATA *stack, int argc);
static int _LOAD_SE_BASE(RS_STACKDATA *stack, int argc);
static int _LOAD_SOUND(RS_STACKDATA *stack, int argc);
static int _STREAM_CLOSE(RS_STACKDATA *stack, int argc);
static int _STREAM_OPEN2(RS_STACKDATA *stack, int argc);
static int _LOAD_BGM_PACK(RS_STACKDATA *stack, int argc);
static int _GET_BGM_NO(RS_STACKDATA *stack, int argc);
static int _GET_MASTER_VOL(RS_STACKDATA *stack, int argc);
static int _SET_MASTER_VOL(RS_STACKDATA *stack, int argc);
static int _GET_BTL_BGM_VOL(RS_STACKDATA *stack, int argc);
static int _SND_IN_REVERB(RS_STACKDATA *stack, int argc);
static int _SND_STOP_SRC(RS_STACKDATA *stack, int argc);
static int _SND_PAUSE_BGM(RS_STACKDATA *stack, int argc);
static int _STREAM_OPEN3(RS_STACKDATA *stack, int argc);
static int _GET_ACTIVE_BGM_STATUS(RS_STACKDATA *stack, int argc);
static int _SET_ACTIVE_BGM_STATUS(RS_STACKDATA *stack, int argc);
static int _GET_BGM_STATUS_NOW_NO(RS_STACKDATA *stack, int argc);
static int _GET_SE_STATUS(RS_STACKDATA *stack, int argc);
static int _SE_ALL_STOP(RS_STACKDATA *stack, int argc);
static int _SOUND_ALL_STOP(RS_STACKDATA *stack, int argc);
static int _BGM_PLAY_CANCEL(RS_STACKDATA *stack, int argc);
static int _BGM_PLAY_ENABLE(RS_STACKDATA *stack, int argc);
static int _REGISTER_VILLAGER2(RS_STACKDATA *stack, int argc);
static int _SET_FISHINGTOURNAMENT_ETC(RS_STACKDATA *stack, int argc);
static int _EOH_SYNC_CHARA(RS_STACKDATA *stack, int argc);
static int _EOH_SYNC_SPRITE(RS_STACKDATA *stack, int argc);
static int _EOH_SET_POS(RS_STACKDATA *stack, int argc);
static int _EOH_SET_ROT(RS_STACKDATA *stack, int argc);
static void _EOH_GET_POS(RS_STACKDATA *stack, int argc);
static void _EOH_GET_ROT(RS_STACKDATA *stack, int argc);
static void _EOH_SET_STEP(RS_STACKDATA *stack, int argc);
static void _EOH_SET_SHOW(RS_STACKDATA *stack, int argc);
static void _EOH_GET_SHOW(RS_STACKDATA *stack, int argc);
static void _EOH_SET_FRAME_SHOW(RS_STACKDATA *stack, int argc);
static void _EOH_SET_SHADOW(RS_STACKDATA *stack, int argc);
static void _EOH_SET_FOOT_SOUND_ID(RS_STACKDATA *stack, int argc);
static void _EOH_SET_FRAME_STATUS(RS_STACKDATA *stack, int argc);
static void _EOH_SET_SOUND_ID(RS_STACKDATA *stack, int argc);
static void _EOH_SET_FADE_FLAG(RS_STACKDATA *stack, int argc);
static void _EOH_RESET_DA_POSITION(RS_STACKDATA *stack, int argc);
static void _EOH_SET_SHADOW_FRAME_STATUS(RS_STACKDATA *stack, int argc);
static void _EOH_SYNC_GEOSTONE(RS_STACKDATA *stack, int argc);
static int _EOH_SYNC_SEARCH_CHARA(RS_STACKDATA *stack, int argc);
static void _EOH_NORMAL_DRIVE(RS_STACKDATA *stack, int argc);
static void _EOH_SET_FOOT_SE_ID(RS_STACKDATA *stack, int argc);
static int _SPHIDA_INIT(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_UP(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_PLAY_FLAG(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_MINIMAP_FLAG(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_MM_LINE_FLAG(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_PIN_POS(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_BALL_POS(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_PIN_COL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_PIN_COL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_BALL_COL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_BALL_COL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_PAR_COUNT(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_PAR_COUNT(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_MINI_LEVEL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_TEXB(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_STATUS_FLAG(RS_STACKDATA *stack, int argc);
static int _SPHIDA_RESET_POWGAGE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_START_POWGAGE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_TRIGGER_POWGAGE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_SHOT_POW(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_POWGAGE_CODE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_POWGAGE_SAFE_LEVEL(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_CULB_NO(RS_STACKDATA *stack, int argc);
static int _SPHIDA_CALC_CARRY(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_PRIZE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_LAST_CHALLENGE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_LAST_CHALLENGE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_OMAKE_MODE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_NOW_HOLE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_NOW_HOLE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_SET_SCORE(RS_STACKDATA *stack, int argc);
static int _SPHIDA_GET_SCORE(RS_STACKDATA *stack, int argc);
static s32 _TEST(RS_STACKDATA *stack, int argc);
static void _MT_TEST(RS_STACKDATA *stack, int argc);
static int _NORMAL_VECTOR(RS_STACKDATA *stack, int argc);
static int _DIST_VECTOR(RS_STACKDATA *stack, int argc);
static int _DIST_VECTOR2(RS_STACKDATA *stack, int argc);
static int _SQRT(RS_STACKDATA *stack, int argc);
static int _ANGLE_LIMIT(RS_STACKDATA *stack, int argc);
static int _GET_RAND(RS_STACKDATA *stack, int argc);
static int _LINE_POINT_DIST(RS_STACKDATA *stack, int argc);
static int _CREATE_SWORD_EFFECT(RS_STACKDATA *stack, int argc);
static int _DELETE_SWORD_EFFECT(RS_STACKDATA *stack, int argc);
static int _SWORD_EFFECT_COLOR(RS_STACKDATA *stack, int argc);
static int _POST_TREASURE_BOX(RS_STACKDATA *stack, int argc);
static int _CTRLC_STEP(RS_STACKDATA *stack, int argc);
static int _CTRLC_SET_ROTATE(RS_STACKDATA *stack, int argc);
static int _CTRLC_ROT_BACK(RS_STACKDATA *stack, int argc);
static int _CTRLC_MOVE_CAMERA(RS_STACKDATA *stack, int argc);
static int _CTRLC_SET_ROT_CANCEL(RS_STACKDATA *stack, int argc);
static int _GET_NEAR_TBOX_POS(RS_STACKDATA *stack, int argc);
static s32 _CONV_CHRNO_S2L(RS_STACKDATA *stack, int argc);
static int _SWE_INIT(RS_STACKDATA *stack, int argc);
static int _SWE_SET_COLOR(RS_STACKDATA *stack, int argc);
static int _SWE_SET_TEXTURE(RS_STACKDATA *stack, int argc);
static int _SWE_START_EFFECT(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_TYPE(RS_STACKDATA *stack, int argc);
static int _GET_EVENT_DATA(RS_STACKDATA *stack, int argc);
static int _DNG_SET_PREV_FLOOR(RS_STACKDATA *stack, int argc);
static int _DNG_GET_PREV_FLOOR(RS_STACKDATA *stack, int argc);
static s32 _DNG_SET_FAST_FLOOR(RS_STACKDATA *stack, int argc);
static int _SET_FLOOR_INFO(RS_STACKDATA *stack, int argc);
static int _GET_FLOOR_INFO(RS_STACKDATA *stack, int argc);
static int _GET_NEXT_FLOOR(RS_STACKDATA *stack, int argc);
static int _PAD_AUTO_REPEAT_OFF(RS_STACKDATA *stack, int argc);
static int _PAD_SET_AUTO_REPEAT(RS_STACKDATA *stack, int argc);
static int _DNG_PAUSE(RS_STACKDATA *stack, int argc);
static int _DNG_CHECK_PAUSE(RS_STACKDATA *stack, int argc);
static int _DNG_RESET_TIMER(RS_STACKDATA *stack, int argc);
static int _DNG_GET_TIMER(RS_STACKDATA *stack, int argc);
static int _LOAD_SKIN(RS_STACKDATA *stack, int argc);
static int _RANDOM_CIRCLE_GET_POS(RS_STACKDATA *stack, int argc);
static int _RANDOM_CIRCLE_OFF(RS_STACKDATA *stack, int argc);
static int _DNG_XCHG_MAP_LIGHT(RS_STACKDATA *stack, int argc);
static int _GEOSTONE_ANIME_OFF(RS_STACKDATA *stack, int argc);
static int _GEOSTONE_SET_FLAG(RS_STACKDATA *stack, int argc);
static int _GEOSTONE_DEL_REFERENCE(RS_STACKDATA *stack, int argc);
static int _GET_ROBO_MOVE_TYPE(RS_STACKDATA *stack, int argc);
static int _SET_EXIT_FLAG(RS_STACKDATA *stack, int argc);
static int _GET_EXIT_FLAG(RS_STACKDATA *stack, int argc);
static int _GET_E3_VERSION(RS_STACKDATA *stack, int argc);
static int _CHK_PAD_CTRL(RS_STACKDATA *stack, int argc);
static int _CTRLC_STAY(RS_STACKDATA *stack, int argc);
static int _GET_RND_CIRCLE_TRAPID(RS_STACKDATA *stack, int argc);
static int _SET_RND_CIRCLE_STATUS(RS_STACKDATA *stack, int argc);
static int _MENU_CHARA_CHENGE(RS_STACKDATA *stack, int argc);
static int _GET_EVENT_INFO_SNDID(RS_STACKDATA *stack, int argc);
static int _GET_PARTS_POS(RS_STACKDATA *stack, int argc);
static s32 _CANCEL_DRAMA_SCENE(RS_STACKDATA *stack, s32 argc);
static int _GET_RNDC_MOT_NOWT(RS_STACKDATA *stack, int argc);
static int _CHARA_NORMAL_DRIVE(RS_STACKDATA *stack, int argc);
static int _CHARA_RESET_DA(RS_STACKDATA *stack, int argc);
static int _JOIN_PARTY_MEMBER(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_CHANGE_FLAG(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_CHANGE_MASK(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_EQUIP(RS_STACKDATA *stack, int argc);
static int _LOAD_PACK_FILE(RS_STACKDATA *stack, int argc);
static int _SET_BIT_CTRL(RS_STACKDATA *stack, int argc);
static int _GET_BIT_CTRL(RS_STACKDATA *stack, int argc);
static int _GET_ITEM_HAVE_NUM(RS_STACKDATA *stack, int argc);
static int _SET_SKIP_BOTTON(RS_STACKDATA *stack, int argc);
static int _SET_SKIP_FCOL(RS_STACKDATA *stack, int argc);
static int _GET_MAP_TYPE(RS_STACKDATA *stack, int argc);
static int _DNG_COLLISION_ALL_CLR(RS_STACKDATA *stack, int argc);
static int _SET_MAP_DRAW(RS_STACKDATA *stack, int argc);
static int _SET_NOW_MAP_NO(RS_STACKDATA *stack, int argc);
static int _CANCEL_LOAD_VILLAGER(RS_STACKDATA *stack, int argc);
static int _CANCEL_NOW_LOADING(RS_STACKDATA *stack, int argc);
static int _ESM_INIT_FIX(RS_STACKDATA *stack, int argc);
static int _ESM_CLEAR(RS_STACKDATA *stack, int argc);
static int _ESM_LOAD_BASE(RS_STACKDATA *stack, int argc);
static int _ESM_FINISH(RS_STACKDATA *stack, int argc);
static int _ESM_DELETE(RS_STACKDATA *stack, int argc);
static int _ESM_SET_TARGET_ID(RS_STACKDATA *stack, int argc);
static int _ESM_SET_VALUE(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_CONDITION(RS_STACKDATA *stack, int argc);
static int _ADD_WHP(RS_STACKDATA *stack, int argc);
static int _GET_TIME(RS_STACKDATA *stack, int argc);
static int _CHECK_GET_ITEM_LIMIT(RS_STACKDATA *stack, int argc);
static int _CHECK_ITEM_OVER(RS_STACKDATA *stack, int argc);
static int _GET_NOW_LOOP_NO(RS_STACKDATA *stack, int argc);
static int _IS_CLEAR_DESTROY(RS_STACKDATA *stack, int argc);
static int _IS_PLAY_SUB_GAME(RS_STACKDATA *stack, int argc);
static int _RESET_SUBJECT_COUNTER(RS_STACKDATA *stack, int argc);
static int _GET_TRIAL_VERSION(RS_STACKDATA *stack, int argc);
static int _SET_FLOOR_EPISODE(RS_STACKDATA *stack, int argc);
static int _ACTCHR_SET_DEF_MOTION(RS_STACKDATA *stack, int argc);
static int _ADD_FUSION_POINT(RS_STACKDATA *stack, int argc);
static int _GET_DEBUG_FLAG(RS_STACKDATA *stack, int argc);
static int _MINIMAP_DOOR_ENABLE(RS_STACKDATA *stack, int argc);
static int _DNG_CHECK_BOSS_MAP(RS_STACKDATA *stack, int argc);
static int _DNG_RUN_EVENT(RS_STACKDATA *stack, int argc);
static int _CHECK_ENABLE_CHARA_CHANGE(RS_STACKDATA *stack, int argc);
static int _INIT_SEPIA(RS_STACKDATA *stack, int argc);
static s32 _START_SEPIA(RS_STACKDATA *stack, s32 argc);
static s32 _END_SEPIA(RS_STACKDATA *stack, s32 argc);
static int _UNLOCK_STACK(RS_STACKDATA *stack, int argc);
static int _RESET_EVENT_TRG(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_NO(RS_STACKDATA *stack, int argc);
static int _GET_CHARA_NO(RS_STACKDATA *stack, int argc);
static int _SEARCH_CHARA_NO(RS_STACKDATA *stack, int argc);
static int _INIT_MONO_FLASH(RS_STACKDATA *stack, int argc);
static int _START_MONO_FLASH(RS_STACKDATA *stack, int argc);
static s32 _END_MONO_FLASH(RS_STACKDATA *stack, s32 argc);
static int _DELETE_VILLAGER(RS_STACKDATA *stack, int argc);
static int _DNG_SET_WEATHER(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_MAXHP(RS_STACKDATA *stack, int argc);
static int _SET_CHARA_DEFENCE(RS_STACKDATA *stack, int argc);
static int _GOTO_USE_ITEM2(RS_STACKDATA *stack, int argc);
static int _DBG_SET_ANALYZE_FLAG(RS_STACKDATA *stack, int argc);
static int _ATRAMIRIA_ON_OFF(RS_STACKDATA *stack, int argc);
static int _ADD_YARIKOMI_MEDAL(RS_STACKDATA *stack, int argc);
static int _SET_MAP_EFFECT_ID(RS_STACKDATA *stack, int argc);
static int _GET_MAP_EFFECT_ID(RS_STACKDATA *stack, int argc);
static int _DNG_FLOOR_INIT(RS_STACKDATA *stack, int argc);
static int _DNG_FLOOR_FINISH(RS_STACKDATA *stack, int argc);
static int _CLEAR_RND_STONE(RS_STACKDATA *stack, int argc);
static int _SET_FLOOR_STATUS(RS_STACKDATA *stack, int argc);
static int _AMG_GET_ATTR_STATUS(RS_STACKDATA *stack, int argc);
static int _SET_KEEP_TIME(RS_STACKDATA *stack, int argc);
static int _GET_KEEP_TIME(RS_STACKDATA *stack, int argc);
static int _CHECK_EQUEP_CHANGE(RS_STACKDATA *stack, int argc);
static int _DNG_EFFECT_ALL_CLEAR(RS_STACKDATA *stack, int argc);
static int _AUTO_CHENGE_BGM_VOL(RS_STACKDATA *stack, int argc);
static int _UDATA_ADD_WHP(RS_STACKDATA *stack, int argc);
static int _UDATA_ADD_ABS(RS_STACKDATA *stack, int argc);
static int _DNG_CREATE_EFFECT(RS_STACKDATA *stack, int argc);
static int _LEAVE_MONICA_ITEM_CHECK(RS_STACKDATA *stack, int argc);
static int _PAUSE_ENABLE_FLAG(RS_STACKDATA *stack, int argc);
static int _FORCE_BOOT_TOUR(RS_STACKDATA *stack, int argc);
static CCameraControl *GetCamera();
static CEventSprite2 *GetEventSprite(int no);

// Code (.text)
CEoh::CEoh(void) {
    type = EOH_TYPE_NONE;
    scene_no = -1;
    world_coord = 1;
    chara = NULL;
    object = NULL;
    sprite = NULL;
    frame = NULL;
    func_point = NULL;
}

int CEoh::Set(int type, CObject *object, int world_coord) {
    if (object == NULL) {
        return 0;
    }
    this->type = type;
    if (this->type != EOH_TYPE_OBJECT) {
        return 0;
    }
    this->object = object;
    this->world_coord = world_coord;
    return 1;
}

int CEoh::Set(int type, int scene_no, CCharacter2 *chara) {
    if (chara == 0) {
        return 0;
    }
    this->type = type;
    switch (this->type) {
        case EOH_TYPE_CHARA:
            this->scene_no = scene_no;
            this->chara = chara;
        return 1;
        default:
        return 0;
    }
}

int CEoh::Set(int type, CEventSprite2 *sprite) {
    if (sprite == 0) {
        return 0;
    }
    this->type = type;
    switch (this->type) {
        case EOH_TYPE_SPRITE:
            this->sprite = sprite;
        return 1;
        default:
        return 0;
    }
}

int CEoh::Set(int type, mgCFrame *frame) {
    if (frame == 0) {
        return 0;
    }
    this->type = type;
    switch (this->type) {
        case EOH_TYPE_FRAME:
            this->frame = frame;
        return 1;
        default:
        return 0;
    }
}

int CEoh::Set(int type, CFuncPoint *func_point) {
    if (func_point == 0) {
        return 0;
    }
    this->type = type;
    switch (this->type) {
        case EOH_TYPE_FUNC_POINT:
            this->func_point = func_point;
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Multiplies a direction by the rotational part of a matrix.
 */
void VectMatMul(float *out, float *in, float (*matrix)[4]) {
    sceVu0FVECTOR result;

    result[0] = in[0] * matrix[0][0] + in[1] * matrix[1][0] + in[2] * matrix[2][0];
    result[1] = in[0] * matrix[0][1] + in[1] * matrix[1][1] + in[2] * matrix[2][1];
    result[2] = in[0] * matrix[0][2] + in[1] * matrix[1][2] + in[2] * matrix[2][2];
    result[3] = 1.0f;
    sceVu0CopyVector(out, result);
}

void CalcPosWorldCoord(float *pos) {
    float rotation[4][4];
    sceVu0FVECTOR rotated;
    if (SetWorldCoordFlg != 0) {
        mgRotMatrixXYZ(rotation, EdEventInfo.world_coord_rot);
        VectMatMul(rotated, pos, rotation);
        sceVu0AddVector(pos, EdEventInfo.world_coord_pos, rotated);
    }
}

void CalcPosWorldCoordGyaku(float *pos) {
    sceVu0FVECTOR rot;
    float matrix[4][4];
    sceVu0FVECTOR local;
    if (SetWorldCoordFlg != 0) {
        rot[0] = 0.0f;
        rot[1] = -EdEventInfo.world_coord_rot[1];
        rot[2] = 0.0f;
        rot[3] = 0.0f;
        mgRotMatrixXYZ(matrix, rot);
        sceVu0SubVector(local, pos, EdEventInfo.world_coord_pos);
        VectMatMul(pos, local, matrix);
    }
}

void SetCamWorldCoord(mgCCamera *camera) {
    sceVu0FVECTOR pos;
    sceVu0FVECTOR ref;
    if ((SetWorldCoordFlg != 0) && (camera != NULL)) {
        camera->GetPos(pos);
        camera->GetRef(ref);
        CalcPosWorldCoord(pos);
        CalcPosWorldCoord(ref);
        camera->SetPos(pos);
        camera->SetRef(ref);
    }
}

void SetCamWorldCoordGyaku(mgCCamera *camera) {

    sceVu0FVECTOR pos;
    sceVu0FVECTOR ref;
    if ((SetWorldCoordFlg != 0) && (camera != NULL)) {
        camera->GetPos(pos);
        camera->GetRef(ref);
        CalcPosWorldCoordGyaku(pos);
        CalcPosWorldCoordGyaku(ref);
        camera->SetPos(pos);
        camera->SetRef(ref);
    }
}

CEohMother::CEohMother() {
    int i;

    for (i = 0; i < EOH_NUM; i++) {
        CEoh *handle = &eoh[i];
        handle->type = EOH_TYPE_NONE;
        handle->scene_no = -1;
        handle->world_coord = 1;

        handle->object = 0;
        handle->chara = 0;
        handle->sprite = 0;
        handle->frame = 0;
        handle->func_point = 0;
    }
}

int CEohMother::Set(int slot, int unused, CObject *object, int flag) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    return eoh[slot].Set(EOH_TYPE_OBJECT, object, flag);
}

int CEohMother::Set(int slot, int kind, int chara_no, CCharacter2 *chara) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    return eoh[slot].Set(kind, chara_no, chara);
}

int CEohMother::Set(int slot, int kind, CEventSprite2 *sprite) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    return eoh[slot].Set(kind, sprite);
}

int CEohMother::Set(int slot, int kind, mgCFrame *frame) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    return eoh[slot].Set(kind, frame);
}

int CEohMother::Set(int slot, int kind, CFuncPoint *funcPoint) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    return eoh[slot].Set(kind, funcPoint);
}

int CEohMother::SetPos(int no, float x, float y, float z) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            sceVu0FVECTOR pos;
            pos[0] = x;
            pos[1] = y;
            pos[2] = z;
            pos[3] = 1.0f;
            CalcPosWorldCoord(pos);
            x = pos[0];
            y = pos[1];
            z = pos[2];
            mgCObject *chara = (mgCObject *)eoh[no].object;
            if (chara == NULL) {
                return 0;
            }
            chara->SetPosition(x, y, z);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            if (handle->world_coord != 0) {
                sceVu0FVECTOR pos;
                pos[0] = x;
                pos[1] = y;
                pos[2] = z;
                pos[3] = 1.0f;
                CalcPosWorldCoord(pos);
                x = pos[0];
                y = pos[1];
                z = pos[2];
            }
            mgCObject *object = (mgCObject *)eoh[no].object;
            if (object == NULL) {
                return 0;
            }
            object->SetPosition(x, y, z);
            return 1;
        }
        case EOH_TYPE_SPRITE: {
            sceVu0FVECTOR pos;
            float *posY = &pos[1];
            pos[0] = x;
            *posY = y;
            pos[2] = z;
            pos[3] = 1.0f;
            CEventSprite2 *&sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            if (sprite->GetType() == 0) {
                *posY += sprite_ground_offset;
                sprite->SetPosition(pos);
            } else if (sprite->GetType() == sprite_type_world) {
                CalcPosWorldCoord(pos);
                sprite->SetPosition(pos);
            }
            return 1;
        }
        case EOH_TYPE_FRAME: {
            sceVu0FVECTOR pos;
            pos[0] = x;
            pos[1] = y;
            pos[2] = z;
            pos[3] = 1.0f;
            mgCObject *frame = (mgCObject *)handle->object;
            if (frame == NULL) {
                return 0;
            }
            frame->SetPosition(pos);
            return 1;
        }
        case EOH_TYPE_FUNC_POINT: {
            if (handle->world_coord != 0) {
                sceVu0FVECTOR pos;
                pos[0] = x;
                pos[1] = y;
                pos[2] = z;
                pos[3] = 1.0f;
                CalcPosWorldCoord(pos);
                CFuncPoint *funcPoint = eoh[no].func_point;
                if (funcPoint == NULL) {
                    return 0;
                }
                *(u_long128 *)funcPoint->position = *(u_long128 *)pos;
                funcPoint->frame.SetPosition(pos);
            }
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::SetRot(int no, float x, float y, float z) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            mgCObject *&chara = (mgCObject *&)handle->object;
            if (chara == NULL) {
                return 0;
            }
            x += EdEventInfo.world_coord_rot[0];
            y += EdEventInfo.world_coord_rot[1];
            z += EdEventInfo.world_coord_rot[2];
            y = mgAngleLimit(y);
            chara->SetRotation(x, y, z);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            mgCObject *&object = (mgCObject *&)handle->object;
            if (object == NULL) {
                return 0;
            }
            if (handle->world_coord != 0) {
                x += EdEventInfo.world_coord_rot[0];
                y += EdEventInfo.world_coord_rot[1];
                z += EdEventInfo.world_coord_rot[2];
                y = mgAngleLimit(y);
            }
            object->SetRotation(x, y, z);
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCObject *frame = (mgCObject *)handle->object;
            if (frame == NULL) {
                return 0;
            }
            frame->SetRotation(x, y, z);
            return 1;
        }
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            sprite->SetRotZ(z);
            return 1;
        }
        case EOH_TYPE_FUNC_POINT: {
            CFuncPoint *&funcPoint = handle->func_point;
            if (funcPoint == NULL) {
                return 0;
            }
            x += EdEventInfo.world_coord_rot[0];
            y += EdEventInfo.world_coord_rot[1];
            z += EdEventInfo.world_coord_rot[2];
            y = mgAngleLimit(y);
            sceVu0FVECTOR rot;
            rot[0] = x;
            rot[1] = y;
            rot[2] = z;
            rot[3] = 1.0f;
            CFuncPoint *target = funcPoint;
            *(u_long128 *)target->rotation = *(u_long128 *)rot;
            target->frame.SetRotation(rot);
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::GetPos(int no, float *pos) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            mgCObject *chara = (mgCObject *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->GetPosition(pos);
            CalcPosWorldCoordGyaku(pos);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            mgCObject *object = (mgCObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            object->GetPosition(pos);
            if (eoh[no].world_coord != 0) {
                CalcPosWorldCoordGyaku(pos);
            }
            return 1;
        }
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *&sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            sprite->GetPosition(pos);
            if (sprite->GetType() == sprite_type_world) {
                CalcPosWorldCoordGyaku(pos);
            } else {
                pos[1] -= sprite_ground_offset;
            }
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCObject *frame = (mgCObject *)handle->object;
            if (frame == NULL) {
                return 0;
            }
            frame->GetPosition(pos);
            return 1;
        }
        case EOH_TYPE_FUNC_POINT: {
            CFuncPoint *funcPoint = handle->func_point;
            if (funcPoint == NULL) {
                return 0;
            }
            *(u_long128 *)pos = *(u_long128 *)funcPoint->position;
            CalcPosWorldCoordGyaku(pos);
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::GetRot(int no, float *rot) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            mgCObject *chara = (mgCObject *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->GetRotation(rot);
            rot[1] -= EdEventInfo.world_coord_rot[1];
            rot[1] = mgAngleLimit(rot[1]);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            mgCObject *object = (mgCObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            object->GetRotation(rot);
            if (eoh[no].world_coord != 0) {
                rot[1] -= EdEventInfo.world_coord_rot[1];
                rot[1] = mgAngleLimit(rot[1]);
            }
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCObject *frame = (mgCObject *)handle->object;
            if (frame == NULL) {
                return 0;
            }
            frame->GetRotation(rot);
            return 1;
        }
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *&sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[3] = 0.0f;
            rot[2] = sprite->GetRotZ();
            return 1;
        }
        case EOH_TYPE_FUNC_POINT: {
            CFuncPoint *funcPoint = handle->func_point;
            if (funcPoint == NULL) {
                return 0;
            }
            *(u_long128 *)rot = *(u_long128 *)funcPoint->rotation;
            if (handle->world_coord != 0) {
                rot[1] -= EdEventInfo.world_coord_rot[1];
                rot[1] = mgAngleLimit(rot[1]);
            }
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::SetMotion(int no, char *name, int flag, float time) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CObject **held = &handle->object;
            CCharacter2 *chara = (CCharacter2 *)*held;
            if (chara == NULL) {
                return 0;
            }
            chara->SetMotion(name, flag);
            if (time != -1.0f) {
                ((CCharacter2 *)*held)->NormalDrive();
                ((CCharacter2 *)*held)->SetStep(time);
                chara = (CCharacter2 *)*held;
                chara->frame = (float)chara->now_key->start_frame;
            }
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::CheckMotionEnd(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            if (chara->seq_mode == 0) {
                return ((CCharacter2 *)chara)->CheckMotionEnd();
            }
            if (chara->seq_state == 4) {
                return ((CCharacter2 *)chara)->CheckMotionEnd();
            }
            return 0;
        }
        default:
        return 0;
    }
}

int CEohMother::SetMotionTrg(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            if (chara->seq_mode != 1) {
                break;
            }
            chara->seq_advance = 1;
            break;
        }
        default:
        return 0;
    }
    return 0;
}

int CEohMother::GetSeqStatus(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            if (chara->seq_mode == 1) {
                return chara->seq_state;
            }
            return 0;
        }
    }
    return 0;
}

int CEohMother::SetStep(int no, float step) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->SetStep(step);
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::SetChangeStep(int no, float step) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            chara->blend_speed = step;
            if (step >= 1.0f) {
                chara->blend = 1.0f;
            }
            break;
        }
        default:
        return 0;
    }
    return 1;
}

int CEohMother::ResetMotion(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->ResetMotion();
            break;
        }
        default:
        return 0;
    }
    return 1;
}

int CEohMother::SetTexAnim(int no, int on, char *name) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            int texb = EventScene->GetCharaTexb(handle->scene_no);
            if (texb < 0) {
                return 0;
            }
            mgCTextureManager *manager = &mgTexManager;
            if (on != 0) {
                manager->TexAnimeOn(texb, name);
            } else if (name != NULL) {
                manager->TexAnimeOff(texb, name);
            } else {
                manager->TexAnimeAllOff(texb);
            }
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetScale(int no, float x, float y, float z) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            mgCObject *chara = (mgCObject *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->SetScale(x, y, z);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            mgCObject *object = (mgCObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            object->SetScale(x, y, z);
            return 1;
        }
        case EOH_TYPE_SPRITE:
            handle->sprite->SetScale(x, y);
        return 1;
        case EOH_TYPE_FRAME:
            ((mgCObject *)handle->frame)->SetScale(x, y, z);
        return 1;
        case EOH_TYPE_FUNC_POINT: {
            CFuncPoint *&funcPoint = handle->func_point;
            if (funcPoint == NULL) {
                return 0;
            }
            sceVu0FVECTOR scale;
            scale[0] = x;
            scale[1] = y;
            scale[2] = z;
            scale[3] = 1.0f;
            CFuncPoint *target = funcPoint;
            *(u_long128 *)target->scale = *(u_long128 *)scale;
            target->frame.SetScale(scale);
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::GetScale(int no, float *scale) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            mgCObject *chara = (mgCObject *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->GetScale(scale);
            return 1;
        }
        case EOH_TYPE_OBJECT: {
            mgCObject *object = (mgCObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            object->GetScale(scale);
            return 1;
        }
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            float sizeX;
            float sizeY;
            sprite->GetScale(&sizeX, &sizeY);
            scale[0] = sizeX;
            scale[1] = sizeY;
            scale[2] = 0.0f;
            scale[3] = 0.0f;
            return 1;
        }
        case EOH_TYPE_FUNC_POINT: {
            CFuncPoint *funcPoint = handle->func_point;
            if (funcPoint == NULL) {
                return 0;
            }
            *(u_long128 *)scale = *(u_long128 *)funcPoint->scale;
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::SetShow(int no, int show) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_OBJECT: {
            CObject *object = (CObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            object->Show(show);
            return 1;
        }
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->Show(show);
            return 1;
        }
        case EOH_TYPE_FUNC_POINT:
            if (handle->func_point == NULL) {
            return 0;
            }
            handle->func_point->enable = show;
        return 1;
        default:
        return 0;
    }
}

int CEohMother::GetShow(int no, int *show) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_OBJECT: {
            CObject *object = (CObject *)handle->object;
            if (object == NULL) {
                return 0;
            }
            *show = object->GetShow();
            return 1;
        }
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            *show = chara->GetShow();
            return 1;
        }
        case EOH_TYPE_FUNC_POINT:
            if (handle->func_point == NULL) {
            return 0;
            }
            *show = handle->func_point->enable;
        return 1;
        default:
        return 0;
    }
}
mgCFrame *CEohMother::SearchFrame(int slot, char *name) {
    if (slot < 0 || slot >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[slot];
    mgCFrame *result = 0;
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            result = chara->CObjectFrame::frame->SearchFrame(name);
            break;
        }
    }
    return result;
}

int CEohMother::SetFrameShow(int no, char *name, int show) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            mgCFrame *frame = chara->CObjectFrame::frame->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr == NULL) {
                return 0;
            }
            attr->draw = show;
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCFrame *frame = handle->frame;
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr == NULL) {
                return 0;
            }
            attr->draw = show;
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetShadow(int no, int on) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA:
            if (on != 0) {
                EventScene->ResetStatus(1, handle->scene_no, status_no_shadow);
            } else {
                EventScene->SetStatus(1, handle->scene_no, status_no_shadow);
            }
        return 1;
    }
    return 0;
}

int CEohMother::SetShadowFrameShow(int no, char *name, int show) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            mgCFrame *shadow = (mgCFrame *)chara->shadow_frame;
            if (shadow == NULL) {
                return 0;
            }
            mgCFrame *frame = shadow->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr == NULL) {
                return 0;
            }
            attr->draw = show;
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetTranslate(int no, float *translate) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            mgCFrame *frame = chara->CObjectFrame::frame;
            if (frame == NULL) {
                return 0;
            }
            frame->trans_matrix[3][0] = translate[0];
            frame->trans_matrix[3][1] = translate[1];
            frame->trans_matrix[3][2] = translate[2];
            frame->changed = 1;
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCFrame *frame = handle->frame;
            if (frame == NULL) {
                return 0;
            }
            frame->trans_matrix[3][0] = translate[0];
            frame->trans_matrix[3][1] = translate[1];
            frame->trans_matrix[3][2] = translate[2];
            frame->changed = 1;
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetColor(int no, float *color) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            sprite->SetColor(color);
            return 1;
        }
    }
    return 0;
}

int CEohMother::GetColor(int no, float *color) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_SPRITE: {
            CEventSprite2 *sprite = handle->sprite;
            if (sprite == NULL) {
                return 0;
            }
            sprite->GetColor(color);
            return 1;
        }
    }
    return 0;
}

char *CEohMother::GetNowMotionName(int slot) {
    if (slot < 0 || slot >= EOH_NUM) {
        return NULL;
    }
    CEoh *handle = &eoh[slot];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara != NULL) {
                return chara->GetNowMotionName();
            }
            return NULL;
        }
    }
    return NULL;
}

int CEohMother::GetNowMotionStatus(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara != NULL) {
                return chara->GetMotionStatus();
            }
            return 0;
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", SetMotionNowTime__10CEohMotherFif);
int CEohMother::SetMotionWaitTime(int no, float rate) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CObject **held = &handle->object;
            CCharacter2 *chara = (CCharacter2 *)*held;
            if (chara == NULL) {
                return 0;
            }
            CHRINFO_KEY_SET *motion = chara->now_key;
            if (motion != NULL) {
                float length = (float)(motion->end_frame - motion->start_frame);
                length *= rate;
                chara->frame = length + (float)motion->start_frame;
                ((CCharacter2 *)*held)->NormalDrive();
                chara = (CCharacter2 *)*held;
                chara->frame = length + (float)chara->now_key->start_frame;
            }
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::SetFootSoundID(int no, int id) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            chara->foot_sound_id = id;
            return 1;
        }
    }
    return 0;
}

int CEohMother::GetFramePos(int no, char *name, float *pos) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            if (chara->CObjectFrame::frame == NULL) {
                return 0;
            }
            mgCFrame *frame = chara->CObjectFrame::frame->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            frame->GetWorldPosition0(pos);
            CalcPosWorldCoordGyaku(pos);
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetSoundID(int no, unsigned int id) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            chara->se_bank = id;
            return 1;
        }
    }
    return 0;
}

int CEohMother::GetFrameShow(int no, char *name) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            mgCFrame *frame = chara->CObjectFrame::frame->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr != NULL) {
                return attr->draw;
            }
            return 0;
        }
        case EOH_TYPE_FRAME: {
            mgCFrame *frame = handle->frame;
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr != NULL) {
                return attr->draw;
            }
            return 0;
        }
    }
    return 0;
}

int CEohMother::SetFadeFlag(int no, int flag) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->SetFadeFlag(flag);
            return 1;
        }
        default:
        return 0;
    }
}

int CEohMother::ResetDAPosition(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 **chara = (CCharacter2 **)&handle->object;
            if (*chara == NULL) {
                return 0;
            }
            (*chara)->ResetDAPosition();
            (*chara)->StepDA(10);
            return 1;
        }
    }
    return 0;
}

int CEohMother::NormalDrive(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = (CCharacter2 *)handle->object;
            if (chara == NULL) {
                return 0;
            }
            chara->NormalDrive();
            break;
        }
        default:
        return 0;
    }
    return 1;
}

int CEohMother::UpdatePosition(int no) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            chara->UpdatePosition();
            break;
        }
        default:
        return 0;
    }
    return 1;
}

int CEohMother::SetFrameObjAlpha(int no, char *name, float alpha) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            if (chara->CObjectFrame::frame == NULL) {
                return 0;
            }
            mgCFrame *frame = chara->CObjectFrame::frame->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            frame->SetAttrParamObjAlpha(alpha, 1);
            return 1;
        }
        case EOH_TYPE_FRAME: {
            mgCFrame *root = handle->frame;
            if (root == NULL) {
                return 0;
            }
            mgCFrame *frame = root->SearchFrame(name);
            if (frame == NULL) {
                return 0;
            }
            mgCFrameAttr *attr = frame->attr;
            if (attr == NULL) {
                return 0;
            }
            attr->obj_alpha = alpha;
            return 1;
        }
    }
    return 0;
}

int CEohMother::SetFootSeId(int no, int id) {
    if (no < 0 || no >= EOH_NUM) {
        return 0;
    }
    CEoh *handle = &eoh[no];
    switch (handle->type) {
        case EOH_TYPE_CHARA: {
            CCharacter2 *chara = handle->chara;
            if (chara == NULL) {
                return 0;
            }
            chara->foot_se_bank = id;
            break;
        }
        default:
        return 0;
    }
    return 1;
}

/**
 * Replaces the language code in supported resource filename extensions.
 */
static void FileNameConvLanguage(char *name) {
    char *extension[4] = {"txt", "img", "stb", ""};
    char marker[32];
    char *found;
    int i;

    for (i = 0; i < 3; i++) {
        sprintf(marker, "_1.%s", extension[i]);
        if ((found = strstr(name, marker)) != NULL) {
            switch (LanguageCode) {
                case 2:
                case 3:
                case 4:
                case 5:
                    sprintf(found, "_%d.%s", LanguageCode, extension[i]);
                    break;
            }
        }
    }
}

/**
 * Converts a floating-point script argument to an integer when needed.
 */
static int GetStackInt(RS_STACKDATA *stack) {
    if (stack->type == RS_FLOAT) {
        return (int)(stack->f);
    }
    return stack->i;
}

/**
 * Converts an integer script argument to floating point when needed.
 */
static float GetStackFloat(RS_STACKDATA *stack) {
    if (stack->type == RS_INT) {
        return (float)stack->i;
    }
    return stack->f;
}

/**
 * Reads three script values as a position vector.
 */
static void GetStackVector(float *out, RS_STACKDATA *stack) {
    out[0] = GetStackFloat(stack++);
    out[1] = GetStackFloat(stack++);
    out[2] = GetStackFloat(stack++);
    out[3] = 1.0f;
}
/**
 * Returns a script argument as a string.
 */
static char *GetStackString(RS_STACKDATA *stack) {
    return stack->s;
}

/**
 * Writes a value through a script pointer argument.
 */
static void SetStack(RS_STACKDATA *stack, int value) {
    if (stack->type == RS_PTR) {
        stack->p->i = value;
    }
}

/**
 * Writes a value through a script pointer argument.
 */
static void SetStack(RS_STACKDATA *stack, float value) {
    if (stack->type == RS_PTR) {
        stack->p->f = value;
    }
}

void CEventScriptArg::BuildArgData(unsigned int *program) {
    RS_STACKDATA stack[script_stack_slots];
    RS_CALLDATA calls[script_call_slots];
    int (*funcTable[script_func_slots])(RS_STACKDATA *, int);
    int count;
    int i;
    int j;

    funcTable[0] = NULL;
    funcTable[1] = NULL;
    funcTable[2] = NULL;
    count = 0;
    i = 0;
    for (;;) {
        if (esa_ext_func_info[i].func == NULL) {
            break;
        }
        for (j = 0; j < count; j++) {
            if (esa_ext_func_info[i].no == esa_ext_func_info[j].no) {
                printf("EvectScriptArg same ext_func_no!!!\n");
                while (1) {
                }
            }
        }
        if (esa_ext_func_info[i].no < 0 || esa_ext_func_info[i].no >= script_func_slots) {
            printf("EvectScriptArg ext func over!!\n");
        } else {
            funcTable[esa_ext_func_info[i].no] = esa_ext_func_info[i].func;
        }
        i++;
        count++;
    }
    nowScriptArg = this;
    CRunScript script;
    script.load((RS_PROG_HEADER *)program, stack, script_stack_slots, calls, script_call_slots);
    script.ext_func(funcTable, script_func_slots);
    script.run(script_run_id);
    nowScriptArg = NULL;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DATA__FP12RS_STACKDATAi);
/**
 * Id offset.
 */
static int _ID_OFFSET(RS_STACKDATA *stack, int argc) {
    if (nowScriptArg == 0) {
        return 0;
    }
    nowScriptArg->next_id = GetStackInt(stack);
    return 1;
}

/**
 * Returns an argument-list value as an integer.
 */
static int GetArgInt(ARG_DATA *arg) {
    if (arg == NULL) {
        printf("GetArgInt err[null_pointer]\n");
        return 0;
    }
    if (arg->type == RS_FLOAT) {
        return (int)(arg->f);
    }
    return arg->i;
}

/**
 * Returns an argument-list value as floating point.
 */
static float GetArgFloat(ARG_DATA *arg) {
    if (arg == NULL) {
        printf("GetArgFloat err[null_pointer]\n");
        return 0.0f;
    }
    if (arg->type == RS_INT) {
        return (float)arg->i;
    }
    return arg->f;
}

/**
 * Returns an argument-list value as a string.
 */
static char *GetArgString(ARG_DATA *arg) {
    if (arg == NULL) {
        printf("GetArgString err[null_pointer]\n");
        return 0;
    }
    return arg->s;
}

/**
 * Reads three consecutive vector components from an argument list.
 */
static void GetArgVector(float *vec, ARG_DATA *arg) {
    vec[0] = GetArgFloat(arg++);
    vec[1] = GetArgFloat(arg++);
    vec[2] = GetArgFloat(arg++);
    vec[3] = 1.0f;
}

void CRaster::Initialize(void) {
    state = RASTER_OFF;
    amplitude_step = 0.0f;
    amplitude = 0.0f;
    speed_step = 0.0f;
    speed = 0.0f;
    pitch_step = 0.0f;
    pitch = 0.0f;
    unk_20 = 0;
    phase = 0.0f;
    frames = -1;
    frame = 0;
}

void CRaster::SetParam(float amplitude, float speed, float pitch) {
    this->amplitude = amplitude;
    this->speed = speed;
    this->pitch = pitch;
}

void CRaster::StartRaster(float amplitude, float speed, float pitch, int frames) {
    this->frames = frames;
    frame = 0;
    if (this->frames > 1) {
        state = RASTER_START;
        if (amplitude != -1.0f) {
            amplitude_step = (amplitude - this->amplitude) / (float)this->frames;
        } else {
            amplitude_step = 0.0f;
        }
        if (speed != -1.0f) {
            speed_step = (speed - this->speed) / (float)this->frames;
        } else {
            speed_step = 0.0f;
        }
        if (pitch != -1.0f) {
            pitch_step = (pitch - this->pitch) / (float)this->frames;
            return;
        }
        pitch_step = 0.0f;
        return;
    }
    if (amplitude != -1.0f) {
        this->amplitude = amplitude;
    }
    if (speed != -1.0f) {
        this->speed = speed;
    }
    if (pitch != -1.0f) {
        this->pitch = pitch;
    }
    state = RASTER_ON;
}

void CRaster::StopRaster(float amplitude, float speed, float pitch, int frames) {
    this->frames = frames;
    frame = 0;
    if (this->frames > 1) {
        state = RASTER_STOP;
        if (amplitude != -1.0f) {
            amplitude_step = (amplitude - this->amplitude) / (float)this->frames;
        } else {
            amplitude_step = 0.0f;
        }
        if (speed != -1.0f) {
            speed_step = (speed - this->speed) / (float)this->frames;
        } else {
            speed_step = 0.0f;
        }
        if (pitch != -1.0f) {
            pitch_step = (pitch - this->pitch) / (float)this->frames;
            return;
        }
        pitch_step = 0.0f;
        return;
    }
    if (amplitude != -1.0f) {
        this->amplitude = amplitude;
    }
    if (speed != -1.0f) {
        this->speed = speed;
    }
    if (pitch != -1.0f) {
        this->pitch = pitch;
    }
    state = RASTER_OFF;
}

void CRaster::StepRaster(void) {
    switch (state) {
        case RASTER_START:
        case RASTER_STOP:
            amplitude += amplitude_step;
            if (amplitude < 0.0f) {
                amplitude = 0.0f;
            }
            speed += speed_step;
            if (speed > raster_max) {
                speed = raster_max;
            }
            if (speed < 0.0f) {
                speed = 0.0f;
            }
            pitch += pitch_step;
            if (pitch > raster_max) {
                pitch = raster_max;
            }
            if (pitch < 0.0f) {
                pitch = 0.0f;
            }
            frame++;
            if (frame >= this->frames) {
                frame = 0;
                this->frames = -1;
                if (state == RASTER_START) {
                    state = RASTER_ON;
                }
                if (state == RASTER_STOP) {
                    state = RASTER_OFF;
                }
            }
            break;
        case RASTER_ON:
        case RASTER_OFF:
            break;
    }
}

void CRaster::DrawRaster(void) {
    int width;
    int y;
    float shift;
    float current_phase;
    float nextY;

    if (state != RASTER_OFF) {
        mgCTexture screen;

        mgGetFrameBuffer(&screen);
        mgCDrawPrim slot;
        slot.Initialize(NULL, NULL);
        slot.DepthTestEnable(0);
        slot.AlphaTestEnable(0);
        slot.AlphaBlendEnable(0);
        slot.ZMask(-1);
        slot.TextureMapEnable(1);
        current_phase = phase;
        slot.Begin(MG_PRIM_SPRITE);
        slot.Texture(&screen);
        slot.Color(half_color, half_color, half_color, half_color);
        for (y = 0; y < mgScreenHeight; y++) {
            shift = amplitude * sinf(current_phase);
            slot.TextureCrd(0, y);
            slot.Vertex(shift, (float)y, 0.0f);
            slot.TextureCrd(mgScreenWidth, y + 1);
            slot.Vertex(shift + (float)mgScreenWidth, nextY = 1.0f + (float)y, 0.0f);
            if (shift != 0.0f) {
                if (shift > 0.0f) {
                    width = mgScreenWidth;
                    slot.TextureCrd(width - (int)(shift), y);
                    slot.Vertex(0.0f, (float)y, 0.0f);
                    slot.TextureCrd(mgScreenWidth, y + 1);
                    slot.Vertex(shift, nextY, 0.0f);
                } else {
                    slot.TextureCrd(0, y);
                    slot.Vertex((float)mgScreenWidth + shift, (float)y, 0.0f);
                    slot.TextureCrd((int)(-shift), y + 1);
                    slot.Vertex((float)mgScreenWidth, nextY, 0.0f);
                }
            }
            current_phase += pitch;
            current_phase = mgAngleLimit(current_phase);
        }
        slot.End();
        phase += speed;
        phase = mgAngleLimit(phase);
    }
}

void CScreenEffect::Initialize(void) {
    raster.Initialize();
    sepia_texture = NULL;
    sepia = 0;
    mono_flash_texture[0] = NULL;
    mono_flash_texture[1] = NULL;
    mono_flash = 0;
    mono_flash_interval = 0;
    mono_flash_frame = 0;
    mono_flash_no = 0;
}

void CScreenEffect::Step(void) {
    raster.StepRaster();
}

void CScreenEffect::Draw(void) {

    mgCTextureManager *manager = &mgTexManager;

    if (sepia != 0 && sepia_texture != NULL) {
        manager->ReloadTexture(sepia_texture->block, (sceVif1Packet *)NULL);
        mgCDrawPrim sepia_prim;
        sepia_prim.Initialize(NULL, NULL);
        sepia_prim.DepthTestEnable(0);
        sepia_prim.AlphaTestEnable(0);
        sepia_prim.AlphaBlendEnable(0);
        sepia_prim.ZMask(-1);
        sepia_prim.TextureMapEnable(1);
        sepia_prim.Begin(MG_PRIM_SPRITE);
        sepia_prim.Texture(sepia_texture);
        sepia_prim.Color(half_color, half_color, half_color, half_color);
        sepia_prim.TextureCrd(0, 0);
        sepia_prim.Vertex(-1, -1, 0);
        sepia_prim.TextureCrd(mgScreenWidth, mgScreenHeight);
        sepia_prim.Vertex(mgScreenWidth, mgScreenHeight, 0);
        sepia_prim.End();
    }
    if (mono_flash != 0) {
        if (mono_flash_texture[0] != NULL && mono_flash_texture[1] != NULL) {
            manager->ReloadTexture(mono_flash_texture[mono_flash_no]->block, (sceVif1Packet *)NULL);
            mgCDrawPrim flash;
            flash.Initialize(NULL, NULL);
            flash.DepthTestEnable(0);
            flash.AlphaTestEnable(0);
            flash.AlphaBlendEnable(0);
            flash.ZMask(-1);
            flash.TextureMapEnable(1);
            flash.Begin(MG_PRIM_SPRITE);
            flash.Texture(mono_flash_texture[mono_flash_no]);
            flash.Color(half_color, half_color, half_color, half_color);
            flash.TextureCrd(0, 0);
            flash.Vertex(-1, -1, 0);
            flash.TextureCrd(mgScreenWidth, mgScreenHeight);
            flash.Vertex(mgScreenWidth, mgScreenHeight, 0);
            flash.End();
            mono_flash_frame++;
            if (mono_flash_frame >= mono_flash_interval) {
                mono_flash_no = !mono_flash_no;
                mono_flash_frame = 0;
            }
        }
    }
    raster.DrawRaster();
}

void CScreenEffect::InitRaster(float amplitude, float speed, float pitch) {
    raster.Initialize();
    raster.SetParam(amplitude, speed, pitch);
}

void CScreenEffect::StartRaster(float amplitude, float speed, float pitch, int frames) {
    raster.StartRaster(amplitude, speed, pitch, frames);
}

void CScreenEffect::StopRaster(float amplitude, float speed, float pitch, int frames) {
    raster.StopRaster(amplitude, speed, pitch, frames);
}

void CScreenEffect::SetSepiaTexture(mgCTexture *texture, u_long128 *image) {
    if (texture != NULL) {
        sepia_texture = texture;

        sepia_texture->image[0] = image;
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", CaptureSepiaScreen__13CScreenEffectFv);
void CScreenEffect::SetSepiaFlag(s32 enabled) {
    if (sepia_texture != NULL) {
        sepia = enabled;
        return;
    }
    sepia = 0;
}

void CScreenEffect::SetMonoFlashTexture(mgCTexture **texture, u_long128 **image) {
    if (texture[0] == NULL || texture[1] == NULL) {
        return;
    }
    mono_flash_texture[0] = texture[0];
    mono_flash_texture[0]->image[0] = image[0];
    mono_flash_texture[1] = texture[1];
    mono_flash_texture[1]->image[0] = image[1];
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", CaptureMonoFlashScreen__13CScreenEffectFv);
void CScreenEffect::SetMonoFlashFlag(s32 enabled, s32 interval) {
    if (mono_flash_texture[0] != NULL || mono_flash_texture[1] != NULL) {
        mono_flash = enabled;
    } else {
        mono_flash = 0;
    }
    mono_flash_interval = interval;
    mono_flash_frame = 0;
    mono_flash_no = 0;
}

void InitWorldCoord(void) {
    EdEventInfo.world_coord_pos[2] = 0.0f;
    EdEventInfo.world_coord_pos[1] = 0.0f;
    SetWorldCoordFlg = 0;
    EdEventInfo.world_coord_pos[3] = 1.0f;
    EdEventInfo.world_coord_pos[0] = 0.0f;
    EdEventInfo.world_coord_rot[3] = 0.0f;
    EdEventInfo.world_coord_rot[2] = 0.0f;
    EdEventInfo.world_coord_rot[1] = 0.0f;
    EdEventInfo.world_coord_rot[0] = 0.0f;
}

int GetLocalFlag(int no) {
    int bit;
    int inRange;
    int word;

    word = no >> 5;
    if (no < 0) {
        return 0;
    }
    inRange = word < event_local_num;
    if (no < 0) {
        word = (int)(no + 0x1F) >> 5;
        inRange = word < event_local_num;
    }
    bit = no & 0x1F;
    if (inRange == 0) {
        return 0;
    }
    if (no < 0) {
        if (bit != 0) {
            bit -= 0x20;
        }
    }
    int mask = 1 << bit;
    return (mask & EventLocalFlag[word]) != 0;
}

int SetLocalFlag(int no, int on) {

    int bit;
    int inRange;
    int word;
    int mask;
    u32 *flags;

    word = no >> 5;
    if (no < 0) {
        return 0;
    }
    inRange = word < event_local_num;
    if (no < 0) {
        word = (int)(no + 0x1F) >> 5;
        inRange = word < event_local_num;
    }
    bit = no & 0x1F;
    if (inRange == 0) {
        return 0;
    }
    if (no < 0) {
        if (bit != 0) {
            bit -= 0x20;
        }
    }
    mask = 1 << bit;
    flags = &EventLocalFlag[word];
    *flags &= ~mask;
    if (on != 0) {
        *flags |= mask;
    }
    return on;
}

int GetLocalCnt(int no) {
    if (no < 0 || no >= event_local_num) {
        return -1;
    }
    return EventLocalCnt[no];
}

int SetLocalCnt(int no, int value) {
    if (no < 0 || no >= event_local_num) {
        return 0;
    }
    EventLocalCnt[no] = value;
    return 1;
}

int GetLocalCnt2(int value) {
    int i;
    for (i = 0; i < event_local_num; i++) {
        if (value == EventLocalCnt[i]) {
            return i;
        }
    }
    return -1;
}

void InitLocalCnt(void) {
    int i;
    for (i = 0; i < event_local_num; i++) {
        EventLocalCnt[i] = 0;
    }
}

void EdEventInfoCommandInitialize(void) {
    int i;

    EdEventInfo.request = 0;
    EdEventInfo.command_mode = 0;
    EdEventInfo.skip_button = 15;
    EdEventInfo.env_bgm_volume = 1.0f;
    EdEventInfo.skip_state = 0;
    EdEventInfo.skip_fade_color[0] = 0;
    EdEventInfo.skip_fade_color[1] = 0;
    EdEventInfo.skip_fade_color[2] = 0;
    EdEventInfo.skip_fade_color[3] = 0;
    EdEventInfo.start_button = 0;
    EdEventInfo.unk_128 = 0;
    EdEventInfo.env_bgm_no = 0;
    EdEventInfo.stream_playing = 0;
    EdEventInfo.stream_from_fpl = 0;
    for (i = 0; i < 16; i++) {
        EdEventInfo.func_iparam[i] = 0;
        EdEventInfo.func_fparam[i] = 0;
    }
    EdEventInfo.door_type = 0;
    EdEventInfo.map_draw = 1;
    EdEventInfo.interior_entrance = 0;
    EdEventInfo.pack_loaded = 0;
    EdEventInfo.stream_reading = 0;
}

void EventSeqInit(void) {
    int i;
    for (i = 0; i < EOH_NUM; i++) {
        CEoh *handle = &EventObjHandleMother.eoh[i];
        handle->type = EOH_TYPE_NONE;
        handle->scene_no = -1;
        handle->world_coord = 1;

        handle->object = 0;
        handle->chara = 0;
        handle->sprite = 0;
        handle->frame = 0;
        handle->func_point = 0;
    }
    CameraSeq.Initialize(cmr_seq_tbl, seq_node_num);
    int j;
    for (j = 0; j < object_seq_num; j++) {
        ObjectSeq[j].Initialize(obj_seq_tbl, seq_node_num);
    }
    int k;
    for (k = 0; k < event_sprite2_num; k++) {
        EventSprite2[k].Initialize();
    }
    EventScreenEffect.Initialize();
    EventScriptArg.next_id = 0;
    EventScriptArg.list = 0;
    EventScriptArg.list_num = 0;
    EventScriptArg.memory = 0;
}

void EdEventInit(void) {
    int i;

    BuffEventSnd.stSetBuffer(event_snd_buff, event_snd_buffer_size);
    if (strlen("Event Snd Buffer") < memory_name_max) {
        strcpy(BuffEventSnd.name, "Event Snd Buffer");
    }
    BuffEventSnd.stack_used = 0;
    BuffEventSnd.lock = 0;
    BuffEventSnd2.stSetBuffer(event_snd2_buff, event_snd2_buffer_size);
    if (strlen("Event Snd2 Buffer") < memory_name_max) {
        strcpy(BuffEventSnd2.name, "Event Snd2 Buffer");
    }
    BuffEventSnd2.stack_used = 0;
    BuffEventSnd2.lock = 0;
    InitReadBG();
    EventDngMap.Initialize();
    p_use_item = 0;
    SetWorldCoordFlg = 0;
    InitWorldCoord();
    EventScene->map_event_no = 0;
    PakuAnimEohNo = -1;
    memset(PakuAnimName, 0, paku_name_size);
    memset(PakuAnimName2, 0, paku_name_size);
    PakuMotionEohNo = -1;
    memset(PakuMotionName, 0, paku_name_size);
    PakuMotionType = 0;
    memset(PakuMotionName2, 0, paku_name_size);
    PakuMotionType2 = 0;
    EdEventInfoCommandInitialize();
    EventSeqInit();
    for (i = 0; i < event_sprite2_num; i++) {
        EventSprite2[i].Initialize();
    }

    HitEffect[0].live_num = 0;
    HitEffect[0].spark = (BattleEffectPrim *)Hit_para[0];
    HitEffect[0].spark_max = hit_spark_num;
    HitEffect[1].spark = (BattleEffectPrim *)Hit_para[1];
    SwordEffect = NULL;
    HitEffect[2].spark = (BattleEffectPrim *)Hit_para[2];
    EventEffectScript = 0;
    HitEffect[3].spark = (BattleEffectPrim *)Hit_para[3];
    HitEffect[4].spark = (BattleEffectPrim *)Hit_para[4];
    HitEffect[0].spark_num = 0;
    HitEffect[0].kind = 0;
    HitEffect[1].spark_max = hit_spark_num;
    HitEffect[1].live_num = 0;
    HitEffect[1].spark_num = 0;
    HitEffect[1].kind = 0;
    HitEffect[2].spark_max = hit_spark_num;
    HitEffect[2].live_num = 0;
    HitEffect[2].spark_num = 0;
    HitEffect[2].kind = 0;
    HitEffect[3].spark_max = hit_spark_num;
    HitEffect[4].spark_max = hit_spark_num;
    HitEffect[3].live_num = 0;
    HitEffect[3].spark_num = 0;
    HitEffect[3].kind = 0;
    HitEffect[4].live_num = 0;
    HitEffect[4].spark_num = 0;
    HitEffect[4].kind = 0;
    EventScriptArg.next_id = 0;
    EventScriptArg.list = 0;
    EventScriptArg.list_num = 0;
    EventScriptArg.memory = 0;
    EventScreenEffect.Initialize();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", EventTimeDraw__Fv);
void EdEventDraw(void) {
    int hit_no;
    int sprite_no;

    EventRain.Step();
    EventRain.Draw();
    for (hit_no = 0; hit_no < hit_effect_num; hit_no++) {
        HitEffect[hit_no].Draw();
    }
    if (SwordEffect != NULL) {
        SwordEffect->Draw();
    }
    if (EventEffectScript != NULL) {
        EventEffectScript->Draw();
    }
    EventDngMap.Step();
    EventDngMap.Draw();
    esMother.Step();
    esMother.Draw();
    for (sprite_no = 0; sprite_no < event_sprite2_num; sprite_no++) {
        EventSprite2[sprite_no].NormalDraw();
    }
    EventScreenEffect.Draw();
    DrawMenuDl(0x80);
    DrawDownLoadAnaunce();
}

void EdEventFirstDraw(void) {
    int i;

    for (i = 0; i < event_sprite2_num; i++) {
        EventSprite2[i].FirstDraw();
    }
}

int EdEventFinish(void) {
    mgCCamera *camera;
    int i;
    int j;
    int k;

    CameraSeq.Initialize(cmr_seq_tbl, seq_node_num);
    for (i = 0; i < object_seq_num; i++) {
        ObjectSeq[i].Initialize(obj_seq_tbl, seq_node_num);
    }
    for (j = 0; j < event_sprite2_num; j++) {
        EventSprite2[j].Initialize();
    }

    HitEffect[0].live_num = 0;
    HitEffect[0].spark = (BattleEffectPrim *)Hit_para[0];
    HitEffect[0].spark_max = hit_spark_num;
    HitEffect[1].spark = (BattleEffectPrim *)Hit_para[1];
    EventEffectScript = 0;
    HitEffect[2].spark = (BattleEffectPrim *)Hit_para[2];
    HitEffect[3].spark = (BattleEffectPrim *)Hit_para[3];
    HitEffect[4].spark = (BattleEffectPrim *)Hit_para[4];
    HitEffect[0].spark_num = 0;
    HitEffect[0].kind = 0;
    HitEffect[1].spark_max = hit_spark_num;
    HitEffect[1].live_num = 0;
    HitEffect[1].spark_num = 0;
    HitEffect[1].kind = 0;
    HitEffect[2].spark_max = hit_spark_num;
    HitEffect[2].live_num = 0;
    HitEffect[2].spark_num = 0;
    HitEffect[2].kind = 0;
    HitEffect[3].spark_max = hit_spark_num;
    HitEffect[4].spark_max = hit_spark_num;
    HitEffect[3].live_num = 0;
    HitEffect[3].spark_num = 0;
    HitEffect[3].kind = 0;
    HitEffect[4].live_num = 0;
    HitEffect[4].spark_num = 0;
    HitEffect[4].kind = 0;
    camera = (mgCCamera *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    if (SetWorldCoordFlg != 0) {
        SetCamWorldCoord(camera);
        SetWorldCoordFlg = 0;
    }
    esMother.Init();
    EventMarker.Init();
    p_use_item = 0;
    SetWorldCoordFlg = 0;
    for (k = 0; k < event_sprite2_num; k++) {
        EventSprite2[k].Initialize();
    }
    PakuAnimEohNo = -1;
    memset(PakuAnimName, 0, paku_name_size);
    memset(PakuAnimName2, 0, paku_name_size);
    PakuMotionEohNo = -1;
    memset(PakuMotionName, 0, paku_name_size);
    PakuMotionType = 0;
    memset(PakuMotionName2, 0, paku_name_size);
    PakuMotionType2 = 0;
    InitWorldCoord();
    EdEventInfo.skip_state = 0;
    EdEventInfo.pack_loaded = 0;
    EventScriptArg.next_id = 0;
    EventScriptArg.list = 0;
    EventScriptArg.list_num = 0;
    EventScriptArg.memory = 0;
    EventScreenEffect.Initialize();
    return 1;
}

int EdEventStep(void) {
    int seq_no;
    int hit_no;

    mgSetProjection(EdEventInfo.projection);
    for (seq_no = 0; seq_no < object_seq_num; seq_no++) {
        ObjectSeq[seq_no].Play();
    }
    CameraSeq.Play();
    if (Sphida != NULL) {
        Sphida->Step();
    }
    for (hit_no = 0; hit_no < hit_effect_num; hit_no++) {
        HitEffect[hit_no].Step();
    }
    if (SwordEffect != NULL) {
        SwordEffect->CreatPointList();
        SwordEffect->Step();
    }
    if (EventEffectScript != NULL) {
        EventEffectScript->Step();
    }
    EventScreenEffect.Step();
    ReadBG();
    return 1;
}

void InitDramaScene(void) {
    EdEventInfo.skip_fade_color[0] = 0;
    EdEventInfo.skip_fade_color[1] = 0;
    EdEventInfo.skip_state = 1;
    EdEventInfo.skip_button = 15;
    EdEventInfo.skip_fade_color[2] = 0;
    EdEventInfo.skip_fade_color[3] = 0;
}

void CancelDramaScene(void) {
    EdEventInfo.skip_state = 0;
}

void EdEventMenuExit(void) {
    if (p_use_item != 0) {
        p_use_item->i = MenuArg.result[0];
    }
    p_use_item = 0;
}

void EdEventLoopInit(void) {
    int i;
    for (i = 0; i < 12; i++) {
        EdEventInfo.snd_id[i] = 0;
    }
    EdEventInfo.last_snd_id = 0;
    memset(EdEventInfo.script_name, 0, sizeof(EdEventInfo.script_name));
    EdEventInfo.map_draw = 1;
    InitWorldCoord();
    EdEventInfoCommandInitialize();
    EventScreenEffect.Initialize();
}

void EdSetBrokenObject(void) {
}

void ResetMesFileBuffAll(void) {
    int i;
    ClsMes *message;

    for (i = 0; i < 8; i++) {
        message = GetEventMessage(i);
        if (message != NULL) {
            message->mes_data = NULL;
            message->mes_data_size = 0;
        }
    }
}

void EdEventMapInit(void) {
    ClsMes *message;
    int i;

    message = GetSystemMessage();
    if (message != NULL) {
        message->Preset(5);
    }
    for (i = 0; i < event_local_num; i++) {
        EventLocalFlag[i] = 0;
    }
    InitLocalCnt();
    message = GetSystemMessage(0);
    if (message != NULL) {
        message->draw_speed = message->GetDrawSpeedDef();
        message->mes_no = -1;
        message->unk_1e40 = 0;
        message->open = 0;
        message->fade = 0;
        message->fukidashi_centre_x = -1;
        message->fukidashi_centre_y = -1;
    }
    message = GetSystemMessage(1);
    if (message != NULL) {
        message->draw_speed = message->GetDrawSpeedDef();
        message->mes_no = -1;
        message->unk_1e40 = 0;
        message->open = 0;
        message->fade = 0;
        message->fukidashi_centre_x = -1;
        message->fukidashi_centre_y = -1;
    }
    message = GetSystemMessage(2);
    if (message != NULL) {
        message->draw_speed = message->GetDrawSpeedDef();
        message->mes_no = -1;
        message->unk_1e40 = 0;
        message->open = 0;
        message->fade = 0;
        message->fukidashi_centre_x = -1;
        message->fukidashi_centre_y = -1;
    }
    ResetMesFileBuffAll();
    EdSetBrokenObject();
    InitSphida();

    HitEffect[0].live_num = 0;
    HitEffect[0].spark = (BattleEffectPrim *)Hit_para[0];
    HitEffect[0].spark_max = hit_spark_num;
    SwordEffect = NULL;
    HitEffect[1].spark = (BattleEffectPrim *)Hit_para[1];
    EventEffectScript = 0;
    HitEffect[2].spark = (BattleEffectPrim *)Hit_para[2];
    HitEffect[3].spark = (BattleEffectPrim *)Hit_para[3];
    HitEffect[4].spark = (BattleEffectPrim *)Hit_para[4];
    HitEffect[0].spark_num = 0;
    HitEffect[0].kind = 0;
    HitEffect[1].spark_max = hit_spark_num;
    HitEffect[1].live_num = 0;
    HitEffect[1].spark_num = 0;
    HitEffect[1].kind = 0;
    HitEffect[2].spark_max = hit_spark_num;
    HitEffect[2].live_num = 0;
    HitEffect[2].spark_num = 0;
    HitEffect[2].kind = 0;
    HitEffect[3].spark_max = hit_spark_num;
    HitEffect[4].spark_max = hit_spark_num;
    HitEffect[3].live_num = 0;
    HitEffect[3].spark_num = 0;
    HitEffect[3].kind = 0;
    HitEffect[4].live_num = 0;
    HitEffect[4].spark_num = 0;
    HitEffect[4].kind = 0;
    EdEventInfo.stopwatch_start = 0;
    EdEventInfo.stopwatch_limit = 0;
    EdEventInfo.stopwatch_x = 0;
    EdEventInfo.stopwatch_y = 0;
    EdEventInfo.stopwatch_style = 0;
}

void EdEventTermination(void) {
    EdEventInfo.stream_playing = 0;
    if (EdEventInfo.stream_from_fpl == 1) {
        CSnd.StreamEND(1);
    }
    CSnd.StreamSetVol(1, EdEventInfo.stream_volume, EdEventInfo.stream_volume);
    CSnd.StreamClose(1);
    EdEventInfo.stream_reading = 0;
}

void EdEventEnd(void) {
    ResetMesFileBuffAll();
    EventSeqInit();
}

/**
 * Returns an object sequence by its event index.
 */
static CSceneObjSeq *GetObjSeq(int index) {
    if (index < 0 || index >= object_seq_num) {
        return NULL;
    }
    return &ObjectSeq[index];
}

/**
 * Returns the padon to the script.
 */
static int _GET_PADON(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad__2.GetPadOn());
    return 1;
}

/**
 * Returns the paddown to the script.
 */
static int _GET_PADDOWN(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad__2.GetPadDown());
    return 1;
}

/**
 * Returns the padup to the script.
 */
static int _GET_PADUP(RS_STACKDATA *stack, int argc) {
    if (argc <= 0) {
        return 0;
    }
    SetStack(stack, GamePad__2.GetPadUp());
    return 1;
}

/**
 * Returns the apad to the script.
 */
static int _GET_APAD(RS_STACKDATA *stack, int argc) {
    if (argc > 0) {
        SetStack(stack++, GamePad__2.GetLXf());
    }
    if (argc > 1) {
        SetStack(stack++, GamePad__2.GetLYf());
    }
    if (argc > 2) {
        SetStack(stack++, GamePad__2.GetRXf());
    }
    if (argc > 3) {
        SetStack(stack++, GamePad__2.GetRYf());
    }
    return 1;
}
u32 *CheckLoadedBGFile(char *name, int *size) {
    char path[0x80];
    BG_READ_INFO *info;

    GetCurrentDir(path);
    strcat(path, name);
    info = GetReadBGFile(path);
    if (info == NULL) {
        return 0;
    }
    if (info->busy == 0) {
        return 0;
    }
    *size = info->size;
    return (u32 *)info->buffer;
}
u32 *GetLoadBGBuff(char *name, int *size) {
    char path[0x8C];
    int loaded_size;
    u32 *buffer;

    strcpy(path, name);
    FileNameConvLanguage(path);
    buffer = (u32 *)CheckLoadedBGFile(path, &loaded_size);
    if (buffer != NULL) {
        printf("---- EVENT LOAD BG [%s] ----\n", path);
        if (size != NULL) {
            *size = loaded_size;
        }
    } else {
        if (EdEventInfo.pack_loaded == 1) {
            printf("---- EVENT LOAD PACK [%s] ----\n", path);
            buffer = GetPackFile((u32 *)read_buffer, path, &loaded_size);
            if (buffer != NULL) {
                if (size != NULL) {
                    *size = loaded_size;
                }
            } else {
                EdEventInfo.pack_loaded = 0;
            }
        }
        if (EdEventInfo.pack_loaded != 1) {
            if (EdEventInfo.stream_reading == 1) {
                printf("\n-------------------------------------------------------------------\n");
                printf("EVENT ERR [NOW STREAM OPEN!!! <FILE ACCESS ERROR>]\n");
                printf("\n-------------------------------------------------------------------\n");
                while (1) {
                }
            }
            printf("---- EVENT LOAD FILE [%s] ----\n", path);
            if (LoadFile2(path, read_buffer, &loaded_size, 0) == 0) {
                buffer = NULL;
            } else {
                buffer = (u32 *)read_buffer;
                if (size != NULL) {
                    *size = loaded_size;
                }
            }
        }
    }
    return buffer;
}

/**
 * Requests interior.
 */
static int _GOTO_INTERIOR(RS_STACKDATA *stack, int argc) {
    EdEventInfo.jump_point = GetStackInt(stack++);
    strcpy(EdEventInfo.jump_map_name, GetStackString(stack++));
    if (argc > 2) {
        EdEventInfo.event_no = GetStackInt(stack++);
    } else {
        EdEventInfo.event_no = 100;
    }
    if (argc > 3) {
        EdEventInfo.interior_entrance = GetStackInt(stack);
    } else {
        EdEventInfo.interior_entrance = 0;
    }
    EdEventInfo.request = 4;
    return 1;
}

/**
 * Requests outside.
 */
static int _GOTO_OUTSIDE(RS_STACKDATA *stack, int argc) {
    EdEventInfo.jump_point = GetStackInt(stack++);
    strcpy(EdEventInfo.jump_map_name, GetStackString(stack++));
    if (argc > 2) {
        EdEventInfo.event_no = GetStackInt(stack);
    } else {
        EdEventInfo.event_no = 100;
    }
    EdEventInfo.request = 7;
    return 1;
}

/**
 * Initialize .
 */
static int _INITIALIZE(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera;

    EdEventInit();
    camera = (mgCCameraFollow *)GetActiveCamera();
    if (camera == NULL) {
        return 0;
    }
    camera->FollowOff();
    return 1;
}

int _LOAD_CHARA_sub(int stack_no, char **name, int scene_no, unsigned int *data, int flag) {
    u32 *files[pack_file_max];
    int sizes[pack_file_max];
    char label[0x20];
    mgCMemory *stack;
    int texb;
    int result;
    mgCTextureManager *manager;

    stack = (mgCMemory *)EventScene->GetStack(stack_no);
    if (*name == NULL) {
        if (GetPackFileExt(data, "cfg", files, pack_file_max, sizes, name) <= 0) {
            return 0;
        }
    }
    texb = EventScene->GetCharaTexb(scene_no);
    if (texb < 0) {
        return 0;
    }
    manager = &mgTexManager;
    manager->DeleteBlock(texb);
    sprintf(label, "ev%d", scene_no);
    if (scene_no >= 8) {
        strcpy(manager->name_suffix, label);
    }
    result = EventScene->LoadChara(scene_no, data, *name, stack, stack, stack, texb, flag);
    if (scene_no >= 8) {
        manager->name_suffix[0] = 0;
    }
    EventScene->SetType(1, scene_no, type_loaded);
    return result;
}

int _LOAD_CHARA_sub(int stack_no, char **name, int scene_no, unsigned int *data) {
    return _LOAD_CHARA_sub(stack_no, name, scene_no, data, 0);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_CHARA__FP12RS_STACKDATAi);
/**
 * Character active.
 */
static int _CHARA_ACTIVE(RS_STACKDATA *stack, int argc) {
    int enable;
    int chara_no;

    switch (argc) {
        case 1:
            EventScene->SetActive(1, GetStackInt(stack));
        return 1;
        case 2:
            enable = GetStackInt(stack++);
            chara_no = GetStackInt(stack);
            if (enable != 0) {
                EventScene->SetActive(1, chara_no);
            } else {
                EventScene->ResetActive(1, chara_no);
            }
        return 1;
    }
    return 0;
}

/**
 * Clear stack.
 */
static int _CLEAR_STACK(RS_STACKDATA *stack, int argc) {
    EventScene->ClearStack(GetStackInt(stack));
    return 1;
}

/**
 * Assign stack.
 */
static int _ASSIGN_STACK(RS_STACKDATA *stack, int argc) {
    EventScene->AssignStack(GetStackInt(stack));
    return 1;
}

/**
 * Sets the flag from the script arguments.
 */
static int _SET_FLAG(RS_STACKDATA *stack, int argc) {
    CSaveData *saveData;
    int bit;
    int value;

    bit = GetStackInt(stack++);
    value = GetStackInt(stack);
    saveData = GetSaveData();
    if (saveData == NULL) {
        return 0;
    }
    saveData->SetBitFlag(bit, value);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_FLAG__FP12RS_STACKDATAi);
/**
 * Sets the cnt from the script arguments.
 */
static int _SET_CNT(RS_STACKDATA *stack, int argc) {
    CSaveData *saveData;
    int index;
    int value;

    index = GetStackInt(stack++);
    value = GetStackInt(stack);
    saveData = GetSaveData();
    if (saveData == NULL) {
        return 0;
    }
    saveData->SetShortFlag(index, value);
    return 1;
}

/**
 * Returns the cnt to the script.
 */
static int _GET_CNT(RS_STACKDATA *stack, int argc) {
    CSaveData *saveData;
    int index;

    index = GetStackInt(stack++);
    saveData = GetSaveData();
    if (saveData == NULL) {
        return 0;
    }
    SetStack(stack, saveData->GetShortFlag(index));
    return 1;
}

/**
 * Sets the current dir from the script arguments.
 */
static int _SET_CURRENT_DIR(RS_STACKDATA *stack, int argc) {
    char *dir = NULL;
    if (argc > 0) {
        dir = GetStackString(stack);
    }
    if (dir == NULL || strcmp(dir, "DEFAULT") == 0 || strcmp(dir, "") == 0) {
        SetCurrentDir(NULL);
    } else {
        SetCurrentDir(dir);
    }
    return 1;
}

/**
 * Change dir.
 */
static int _CHANGE_DIR(RS_STACKDATA *stack, int argc) {
    char *dir = NULL;
    if (argc > 0) {
        dir = GetStackString(stack);
    }
    if (dir == NULL || strcmp(dir, "DEFAULT") == 0 || strcmp(dir, "") == 0) {
        SetCurrentDir(NULL);
    } else {
        ChangeDir(dir);
    }
    return 1;
}

/**
 * Deletes the character.
 */
#ifdef NONMATCHING
static int _DELETE_CHARA(RS_STACKDATA *stack, int argc) {
    int chara_no;
    int delete_texture = 1;
    chara_no = GetStackInt(stack++);
    if (argc >= 2) {
        delete_texture = GetStackInt(stack);
    }
    int texb = EventScene->GetCharaTexb(chara_no);

    if (texb >= 0) {
        mgCTextureManager *manager = &mgTexManager;
        if (delete_texture == 1) {
            manager->DeleteBlock(texb);
        }
    }
    EventScene->DeleteChara(chara_no);
    int i;
    for (i = 0; i < EOH_NUM; i++) {
        CEoh *handle = &EventObjHandleMother.eoh[i];
        if (handle->type == EOH_TYPE_CHARA) {
            int *charaSlot = &handle->scene_no;
            if (chara_no == handle->scene_no) {
                handle->type = EOH_TYPE_NONE;
                *charaSlot = -1;
                handle->world_coord = 1;
                handle->chara = NULL;
                handle->object = NULL;
                handle->sprite = NULL;
                handle->frame = NULL;
                handle->func_point = NULL;
            }
        }
    }
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DELETE_CHARA__FP12RS_STACKDATAi);
#endif
int _LOAD_MOTION_sub(int stack_no, char *name, int scene_no, unsigned int *data) {
    char label[0x20];
    mgCMemory *stack = (mgCMemory *)EventScene->GetStack(stack_no);
    CCharacter2 *chara = EventScene->GetCharacter(scene_no);
    if (chara == NULL) {
        return 0;
    }
    int texb = EventScene->GetCharaTexb(scene_no);
    if (texb < 0) {
        return 0;
    }
    sprintf(label, "ev%d", scene_no);
    mgCTextureManager *manager = &mgTexManager;
    if (scene_no >= 8) {
        strcpy(manager->name_suffix, label);
    }
    chara->LoadPack(data, name, stack, stack, stack, texb, 0);
    if (scene_no >= 8) {
        manager->name_suffix[0] = 0;
    }
    return 1;
}

/**
 * Loads the motion.
 */
static int _LOAD_MOTION(RS_STACKDATA *stack, int argc) {
    int stack_no;
    int chara_no;
    char *pack_name;
    char *motion_name;
    u32 *pack;
    switch (argc) {
        case 1: {
            int key = GetStackInt(stack);
            int count;
            int i;
            ARG_LIST *node;
            ARG_DATA *args;

            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
                return 0;
            }
            stack_no = GetArgInt(args++);
            pack_name = GetArgString(args++);
            motion_name = GetArgString(args++);
            chara_no = GetArgInt(args);
            break;
        }
        case 4:
            stack_no = GetStackInt(stack++);
            pack_name = GetStackString(stack++);
            motion_name = GetStackString(stack++);
            chara_no = GetStackInt(stack);
            break;
        default:
        return 0;
    }
    pack = GetLoadBGBuff(pack_name, 0);
    if (pack == NULL) {
        return 0;
    }
    return _LOAD_MOTION_sub(stack_no, motion_name, chara_no, pack);
}

/**
 * Map jump.
 */
static int _MAP_JUMP(RS_STACKDATA *stack, int argc) {
    EdEventInfo.jump_point = GetStackInt(stack++);
    switch (stack->type) {
        case 0:
            strcpy(EdEventInfo.jump_map_name, GetMapName(GetStackInt(stack++), NULL));
            break;
        case 2:
            strcpy(EdEventInfo.jump_map_name, GetStackString(stack++));
            break;
        default:
        return 0;
    }
    if (argc > 2) {
        EdEventInfo.event_no = GetStackInt(stack);
    } else {
        EdEventInfo.event_no = 100;
    }
    EdEventInfo.request = exit_map_jump;
    return 1;
}

/**
 * Sets the rain from the script arguments.
 */
static int _SET_RAIN(RS_STACKDATA *stack, int argc) {
    if (GetStackInt(stack) != 0) {
        EventRain.Start();
    } else {
        EventRain.Stop();
    }
    return 1;
}

/**
 * Deletes the ext motion.
 */
static int _DEL_EXT_MOTION(RS_STACKDATA *stack, int argc) {
    CCharacter2 *character = EventScene->GetCharacter(GetStackInt(stack));
    if (character == NULL) {
        return 0;
    }
    character->DeleteExtMotion();
    return 1;
}

/**
 * Sets the marker from the script arguments.
 */
static s32 _SET_MARKER(RS_STACKDATA *stack, int argc) {
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_WORLD_COORD__FP12RS_STACKDATAi);
/**
 * Finish .
 */
static void _FINISH(RS_STACKDATA *stack, int argc) {
    EdEventFinish();
}

/**
 * Returns the dun world coordinates to the script.
 */
static int _GET_DUN_WORLD_COORD(RS_STACKDATA *stack, int argc) {

    sceVu0FVECTOR position;
    float indexed_position[6];
    float angle;
    float indexed_angle;

    if (argc == 4) {
        if (GetDungeonEventPoint(position, &angle, 0) == 0) {
            return 0;
        }
        SetStack(stack++, position[0]);
        SetStack(stack++, position[1]);
        SetStack(stack++, position[2]);
        SetStack(stack, angle);
        return 1;
    }
    if (argc == 5) {
        if (GetDungeonEventPoint(indexed_position, &indexed_angle, GetStackInt(stack + 4)) == 0) {
            return 0;
        }
        SetStack(stack++, indexed_position[0]);
        SetStack(stack++, indexed_position[1]);
        SetStack(stack++, indexed_position[2]);
        SetStack(stack, indexed_angle);
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_IMG__FP12RS_STACKDATAi);
/**
 * Deletes the image.
 */
static int _DEL_IMG(RS_STACKDATA *stack, int argc) {
    mgTexManager.DeleteBlock(EventScene->event_texb + GetStackInt(stack));
    return 1;
}

/**
 * Sets the dungeon map from the script arguments.
 */
static int _SET_DNG_MAP(RS_STACKDATA *stack, int argc) {
    GetStackInt(stack++);
    GetStackInt(stack);
    return 1;
}

/**
 * Loads the item.
 */
static int _LOAD_ITEM(RS_STACKDATA *stack, int argc) {
    char *name[32];
    int stack_no;
    int chara_no;
    int item_no;
    int index = 0;
    u32 *pack;
    switch (argc) {
        case 1: {
            int key = GetStackInt(stack);
            int count;
            int i;
            ARG_LIST *node;
            ARG_DATA *args;

            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            argc = node->arg_num;
            args = node->args;
        haveArgs:
            if (args == NULL) {
                return 0;
            }
            stack_no = GetArgInt(args++);
            item_no = GetArgInt(args++);
            name[0] = GetArgString(args++);
            chara_no = GetArgInt(args++);
            if (argc >= 5) {
                index = GetArgInt(args);
            }
            break;
        }
        case 4:
        case 5:
            stack_no = GetStackInt(stack++);
            item_no = GetStackInt(stack++);
            name[0] = GetStackString(stack++);
            chara_no = GetStackInt(stack++);
            if (argc >= 5) {
                index = GetStackInt(stack);
            }
            break;
        default:
        return 0;
    }
    pack = GetLoadBGBuff(GetItemFilePath(item_no, index), 0);
    if (pack != NULL) {
        return _LOAD_CHARA_sub(stack_no, name, chara_no, pack);
    }
    return 0;
}

/**
 * Requests use item.
 */
static int _GOTO_USE_ITEM(RS_STACKDATA *stack, int argc) {
    int arg_no;
    if (stack->type != RS_PTR) {
        return 0;
    }
    RS_STACKDATA *item_slot = stack->p;
    arg_no = 1;
    MenuArg.open_type = menu_use_item;
    stack++;
    MenuArg.param[0] = arg_no;
    p_use_item = item_slot;
    for (arg_no = 1; arg_no < argc; arg_no++) {
        MenuArg.param[arg_no] = GetStackInt(stack++);
    }
    MenuArg.param[arg_no] = 0;
    EdEventInfo.command_mode = request_menu;
    return 1;
}

/**
 * Sets the local flag from the script arguments.
 */
static int _SET_LOCAL_FLAG(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    SetLocalFlag(index, GetStackInt(stack));
    return 1;
}

/**
 * Returns the local flag to the script.
 */
static int _GET_LOCAL_FLAG(RS_STACKDATA *stack, int argc) {
    int result;
    int index = GetStackInt(stack++);

    result = GetLocalFlag(index);
    SetStack(stack, result);
    return 1;
}

/**
 * Requests select party.
 */
static int _GOTO_SELECT_PARTY(RS_STACKDATA *stack, int argc) {
    MenuArg.open_type = menu_select_party;
    EdEventInfo.command_mode = request_menu;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_LOADBG_FILE__FP12RS_STACKDATAi);
/**
 * Sets the loadbg file mons talk from the script arguments.
 */
static int _SET_LOADBG_FILE_MONS_TALK(RS_STACKDATA *stack, int argc) {
    char path[0x8C];
    int size;
    u_long128 *buffer = read_buffer;
    StartReadBG();
    sprintf(path, "dungeon/msg/mostalk%d_%d.txt", DngStatus.dungeon_no, LanguageCode);
    if (LoadFileBG(path, buffer, &size) == 0) {
        return 0;
    }
    EdEventInfo.pack_loaded = 0;
    return 1;
}

/**
 * Checks loadbg file and returns the result to the script.
 */
static int _CHECK_LOADBG_FILE(RS_STACKDATA *stack, int argc) {
    int result;
    result = ReadBGSync();
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the tb itemno to the script.
 */
static s32 _GET_TB_ITEMNO(RS_STACKDATA *stack, int argc) {
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_TB_STATUS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_TB_ANGLE__FP12RS_STACKDATAi);
/**
 * Adds to the item.
 */
static int _ADD_ITEM(RS_STACKDATA *stack, int argc) {
    int item_no = GetStackInt(stack++);
    int count = 1;
    if (argc == 2) {
        count = GetStackInt(stack);
    }
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    return user_data->GetItem(item_no, count);
}

/**
 * Sub item.
 */
static int _SUB_ITEM(RS_STACKDATA *stack, int argc) {

    int item_no;
    int count = 1;
    item_no = GetStackInt(stack++);
    if (argc >= 2) {
        count = GetStackInt(stack);
    }
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    if (user_data == NULL) {
        return 0;
    }
    user_data->DeleteItem(item_no, count);
    return 1;
}

/**
 * Returns the item type to the script.
 */
static int _GET_ITEM_TYPE(RS_STACKDATA *stack, int argc) {
    int itemType = GetItemDataType(GetStackInt(stack++));
    int category;
    if (itemType == 0) {
        return 0;
    }
    switch (itemType) {
        case 1:
            category = 1;
            break;
        case 2:
            category = 2;
            break;
        case 3:
            category = 3;
            break;
        case 4:
            category = 4;
            break;
        default:
            category = 0;
            break;
    }
    SetStack(stack, category);
    return 1;
}

/**
 * Returns the item space to the script.
 */
static int _GET_ITEM_SPACE(RS_STACKDATA *stack, int argc) {
    int result;
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    result = user_data->SearchSpaceUsedData();
    SetStack(stack, result);
    return 1;
}
char GetConfigCaptionOff() {
    s8 captionOff = 0;
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        SV_CONFIG_OPTION *config = &save->config;
        if (config != NULL) {
            captionOff = config->caption_off;
        }
    }
    return captionOff;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", LoadMovie__FPcP9mgCMemoryb);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_MOVIE__FP12RS_STACKDATAi);
/**
 * Clears the event local counters.
 */
static s32 _INIT_LOCAL_CNT(RS_STACKDATA *stack, s32 argc) {
    InitLocalCnt();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CROSSFADE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_ADJUST_POLYGON_SCALE__FP12RS_STACKDATAi);
/**
 * Sets the time from the script arguments.
 */
static int _SET_TIME(RS_STACKDATA *stack, int argc) {
    EventScene->SetTime(GetStackFloat(stack));
    return 1;
}

/**
 * Sets the active light from the script arguments.
 */
static int _SET_ACTIVE_LIGHT(RS_STACKDATA *stack, int argc) {

    CMap *maps[8];

    int lightNo = GetStackInt(stack);
    if (EventScene->GetActiveMap(maps, 8) <= 0) {
        return 0;
    }
    if (lightNo >= 0) {
        if (lightNo < maps[0]->lighting_info_num) {
            maps[0]->active_light_no = lightNo;
        }
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_PAKU_ANIM__FP12RS_STACKDATAi);
/**
 * Resets the paku animation.
 */
static s32 _RESET_PAKU_ANIM(RS_STACKDATA *stack, s32 argc) {
    PakuAnimEohNo = -1;
    memset(PakuAnimName, 0, sizeof(PakuAnimName));
    memset(PakuAnimName2, 0, sizeof(PakuAnimName2));
    return 1;
}

/**
 * Trg paku animation.
 */
static int _TRG_PAKU_ANIM(RS_STACKDATA *stack, int argc) {
    if (GetStackInt(stack) != 0) {
        if (strcmp(PakuAnimName2, "") != 0) {
            EventObjHandleMother.SetTexAnim( PakuAnimEohNo, 0, PakuAnimName2);
        }
        EventObjHandleMother.SetTexAnim( PakuAnimEohNo, 1, PakuAnimName);
    } else {
        EventObjHandleMother.SetTexAnim( PakuAnimEohNo, 0, PakuAnimName);
        if (strcmp(PakuAnimName2, "") != 0) {
            EventObjHandleMother.SetTexAnim( PakuAnimEohNo, 1, PakuAnimName2);
        }
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _RESET_CAMERA__FP12RS_STACKDATAi);
/**
 * Returns the active chr no to the script.
 */
static int _GET_ACTIVE_CHR_NO(RS_STACKDATA *stack, int argc) {
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save != NULL) {

        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    SetStack(stack, user_data->active_chr_no);
    return 1;
}

/**
 * Sets the active chr no from the script arguments.
 */
static int _SET_ACTIVE_CHR_NO(RS_STACKDATA *stack, int argc) {
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    user_data->SetActiveChrNo(GetStackInt(stack));
    return 1;
}

/**
 * Dungeon set floor id.
 */
static int _DNG_SET_FLOOR_ID(RS_STACKDATA *stack, int argc) {
    int floorId = GetStackInt(stack);
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    dungeon->SetFloorID(floorId);
    return 1;
}

/**
 * Dungeon get floor id.
 */
static int _DNG_GET_FLOOR_ID(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    SetStack(stack, dungeon->floor_id[dungeon->stage_id]);
    return 1;
}

/**
 * Sets the paku motion from the script arguments.
 */
static int _SET_PAKU_MOTION(RS_STACKDATA *stack, int argc) {
    int eohNo = GetStackInt(stack++);
    char *name = GetStackString(stack++);
    int type = GetStackInt(stack++);
    char *name2 = NULL;
    int type2 = 0;
    if (argc > 3) {
        name2 = GetStackString(stack++);
        type2 = GetStackInt(stack);
    }
    PakuMotionEohNo = eohNo;
    strcpy(PakuMotionName, name);
    PakuMotionType = type;
    if (name2 != NULL) {
        strcpy(PakuMotionName2, name2);
    } else {
        strcpy(PakuMotionName2, "");
    }
    PakuMotionType2 = type2;
    return 1;
}

/**
 * Resets the paku motion.
 */
static s32 _RESET_PAKU_MOTION(RS_STACKDATA *stack, s32 argc) {
    PakuMotionEohNo = -1;
    memset(PakuMotionName, 0, sizeof(PakuMotionName));
    memset(PakuMotionName2, 0, sizeof(PakuMotionName2));
    return 1;
}

/**
 * Trg paku motion.
 */
static int _TRG_PAKU_MOTION(RS_STACKDATA *stack, int argc) {
    int mode = GetStackInt(stack);
    if (mode == 0) {
        EventObjHandleMother.SetMotion( PakuMotionEohNo, PakuMotionName,
                                      PakuMotionType, -1.0f);
        return 1;
    }
    if (strcmp(PakuMotionName2, "") == 0) {
        return 1;
    }
    if (mode == 1) {
        if (strcmp(PakuMotionName,
                   EventObjHandleMother.GetNowMotionName( PakuMotionEohNo)) == 0) {
            if (EventObjHandleMother.GetNowMotionStatus( PakuMotionEohNo) == 0 ||
                EventObjHandleMother.GetNowMotionStatus( PakuMotionEohNo) == 4) {
                EventObjHandleMother.SetMotion( PakuMotionEohNo,
                                              PakuMotionName2, PakuMotionType2, -1.0f);
                return 1;
            }
            return 1;
        }
        EventObjHandleMother.SetMotion( PakuMotionEohNo, PakuMotionName2,
                                      PakuMotionType2, -1.0f);
        return 1;
    }
    if (mode == 2) {
        EventObjHandleMother.SetMotion( PakuMotionEohNo, PakuMotionName2,
                                      PakuMotionType2, -1.0f);
        return 1;
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_BG_COLOR__FP12RS_STACKDATAi);
/**
 * Requests dungeon map.
 */
static int _GOTO_DNG_MAP(RS_STACKDATA *stack, int argc) {
    MenuArg.open_type = menu_dng_map;
    MenuArg.param[0] = GetStackInt(stack);
    EdEventInfo.command_mode = request_menu;
    return 1;
}

/**
 * Requests dungeon.
 */
static int _GOTO_DNG(RS_STACKDATA *stack, int argc) {
    INIT_LOOP_ARG loopArg;
    if (EdEventFinish() == 0) {
        return 0;
    }
    memset(&loopArg, 0, sizeof(loopArg));
    loopArg.map_no = GetStackInt(stack++);
    loopArg.floor_no = -1;
    loopArg.event_no = -1;
    if (argc > 1) {
        loopArg.floor_no = GetStackInt(stack++);
    }
    if (argc > 2) {
        loopArg.event_no = GetStackInt(stack);
    }
    NextLoop(2, loopArg);
    EdEventInfo.request = exit_start_loop;
    return 1;
}

/**
 * Requests edit.
 */
static int _GOTO_EDIT(RS_STACKDATA *stack, int argc) {
    INIT_LOOP_ARG loopArg;
    if (EdEventFinish() == 0) {
        return 0;
    }
    memset(&loopArg, 0, sizeof(loopArg));
    loopArg.map_no = GetStackInt(stack++);
    loopArg.event_no = -1;
    if (argc > 1) {
        loopArg.event_no = GetStackInt(stack);
    }
    NextLoop(1, loopArg);
    EdEventInfo.request = exit_start_loop;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_MENU_PARAM__FP12RS_STACKDATAi);
/**
 * Loads the character npc.
 */
static int _LOAD_CHARA_NPC(RS_STACKDATA *stack, int argc) {
    char path[0x80];
    char *modelName[0x80];
    int kind = GetStackInt(stack++);
    int stack_no = GetStackInt(stack++);
    int chara_no = GetStackInt(stack++);
    char *name;
    u32 *pack;
    if (chara_no <= 0) {
        return 0;
    }
    if (chara_no >= 32) {
        return 0;
    }
    if (kind == 0) {
        name = GetPartyCharaModelName(chara_no, 0);
    } else {
        name = GetPartyCharaModelName(chara_no, 2);
    }
    if (name == NULL) {
        return 0;
    }
    strcpy(path, name);
    modelName[0] = GetPartyCharaModelName(chara_no, 1);
    if (modelName[0] == NULL) {
        return 0;
    }
    kind = GetStackInt(stack);
    pack = GetLoadBGBuff(path, 0);
    if (pack != NULL) {
        return _LOAD_CHARA_sub(stack_no, modelName, kind, pack);
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _AUTO_SET_TREASURE_BOX__FP12RS_STACKDATAi);
/**
 * Auto set monster.
 */
static int _AUTO_SET_MONSTER(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    sceVu0FVECTOR direction;
    int monsterNo;
    int param;
    if (GetNowLoopNo() != 2) {
        return 0;
    }
    if (argc < 2) {
        AutoSetMonster();
        return 1;
    }
    monsterNo = GetStackInt(stack++);
    GetStackVector(position, stack);
    stack += 3;
    mgZeroVectorW(direction);
    if (argc >= 7) {
        GetStackVector(direction, stack);
        stack += 3;
    }
    if (argc == 8) {
        param = GetStackInt(stack);
    }
    AutoSetMonster(monsterNo, position, direction, param);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_DUNGEON_MAP_FILE__FP12RS_STACKDATAi);
/**
 * Loads the monster file.
 */
static int _LOAD_MONSTER_FILE(RS_STACKDATA *stack, int argc) {
    if (GetNowLoopNo() != 2) {
        return 0;
    }
    switch (argc) {
        case 1:
            LoadMonsterFile();
            break;
        case 2: {
            int first = GetStackInt(stack++);
            LoadMonsterFile(first, GetStackInt(stack));
            break;
        }
        default:
        return 0;
    }
    return 1;
}

/**
 * Returns the npc status to the script.
 */
static int _GET_NPC_STATUS(RS_STACKDATA *stack, int argc) {
    int npcNo = GetStackInt(stack++);
    if (npcNo <= 0) {
        return 0;
    }
    CUserDataManager *user_data = NULL;
    if (npcNo > 0x20) {
        return 0;
    }
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    SetStack(stack, user_data->GetPartyCharaStatus(npcNo));
    return 1;
}

/**
 * Sets the npc status from the script arguments.
 */
static int _SET_NPC_STATUS(RS_STACKDATA *stack, int argc) {
    int npcNo = GetStackInt(stack++);
    int status = GetStackInt(stack);
    if (npcNo <= 0) {
        return 0;
    }
    CUserDataManager *user_data = NULL;
    if (npcNo > 0x20) {
        return 0;
    }
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    user_data->SetPartyCharaStatus(npcNo, status);
    return 1;
}

/**
 * Returns the now party character to the script.
 */
static int _GET_NOW_PARTY_CHARA(RS_STACKDATA *stack, int argc) {
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    SetStack(stack, user_data->NowPartyCharaID());
    return 1;
}

/**
 * Sets the local cnt from the script arguments.
 */
static int _SET_LOCAL_CNT(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    return SetLocalCnt(index, GetStackInt(stack)) > 0;
}

/**
 * Returns the local cnt to the script.
 */
static int _GET_LOCAL_CNT(RS_STACKDATA *stack, int argc) {
    int result;
    int index = GetStackInt(stack++);
    result = GetLocalCnt(index);
    if (result < 0) {
        return 0;
    }
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the first event local counter holding the requested value, or -1.
 */
static int _GET_LOCAL_CNT2(RS_STACKDATA *stack, int argc) {
    int result;
    int index = GetStackInt(stack++);

    result = GetLocalCnt2(index);
    SetStack(stack, result);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_TRAIN_NPC_POS__FP12RS_STACKDATAi);
/**
 * Requests drawing chapter.
 */
static int _GOTO_DRAW_CHAPTER(RS_STACKDATA *stack, int argc) {
    MenuArg.open_type = menu_draw_chapter;
    MenuArg.param[0] = GetStackInt(stack) - 1;
    EdEventInfo.command_mode = request_menu;
    return 1;
}

/**
 * Sets the projection from the script arguments.
 */
static int _SET_PROJECTION(RS_STACKDATA *stack, int argc) {
    EdEventInfo.projection = GetStackFloat(stack);
    return 1;
}

/**
 * Returns the projection to the script.
 */
static int _GET_PROJECTION(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EdEventInfo.projection);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FADE_IN__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FADE_OUT__FP12RS_STACKDATAi);
/**
 * Dungeon debug command.
 */
static int _DNG_DEBUG_COMMAND(RS_STACKDATA *stack, int argc) {
    ScriptDebugCommand(GetStackInt(stack));
    return 1;
}

/**
 * Cd seek.
 */
static int _CD_SEEK(RS_STACKDATA *stack, int argc) {

    sceCdlFILE *file_info;
    if (sceCdSearchFile(file_info, GetStackString(stack)) != 0) {
        return sceCdSeek(file_info->lsn);
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_ROT_LOOK_POS__FP12RS_STACKDATAi);
/**
 * Sets the motion blur from the script arguments.
 */
static int _SET_MOTION_BLUR(RS_STACKDATA *stack, int argc) {
    EventScene->motion_blur = GetStackInt(stack);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_SCRIPT__FP12RS_STACKDATAi);
/**
 * Sets the talk camera from the script arguments.
 */
static int _SET_TALK_CAMERA(RS_STACKDATA *stack, int argc) {
    static sceVu0FVECTOR vv[3] = {
        {-94.0f, 35.5f, -106.5f, 1.0f},
        {105.0f, 32.5f, -28.5f, 1.0f},
        {113.0f, 34.5f, 82.5f, 1.0f}
    };
    sceVu0FVECTOR middle;
    sceVu0FVECTOR offset;
    sceVu0FMATRIX rotation;
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    float angle;
    float *middle_y;
    from[0] = GetStackFloat(stack++);
    from[1] = GetStackFloat(stack++);
    from[2] = GetStackFloat(stack++);
    to[0] = GetStackFloat(stack++);
    to[1] = GetStackFloat(stack++);
    to[2] = GetStackFloat(stack++);
    angle = GetStackFloat(stack++);
    sceVu0AddVector(middle, from, to);
    sceVu0ScaleVector(middle, middle, 0.5f);
    middle_y = &middle[1];
    *middle_y += 30.0f;
    sceVu0UnitMatrix(rotation);
    sceVu0RotMatrixY(rotation, rotation, angle);
    sceVu0ApplyMatrix(offset, rotation, vv[1]);
    sceVu0AddVector(offset, middle, offset);
    SetStack(stack++, offset[0]);
    SetStack(stack++, offset[1]);
    SetStack(stack++, offset[2]);
    SetStack(stack++, middle[0]);
    SetStack(stack++, *middle_y);
    SetStack(stack, middle[2]);
    return 1;
}

/**
 * Hit effect.
 */
static int _HIT_EFFECT(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    float spread;
    float speed;
    float power;
    float gravity;
    int life;
    int count;
    int index = GetStackInt(stack++);
    position[0] = GetStackFloat(stack++);
    position[1] = GetStackFloat(stack++);
    position[2] = GetStackFloat(stack++);
    position[3] = 1.0f;
    sceVu0FVECTOR direction = {0.0f, 1.0f, 0.0f, 1.0f};
    spread = 50.0f;
    speed = 35.0f;
    power = 0.0f;
    gravity = 0.05f;
    life = 30;
    count = 32;
    if (argc >= 5) {
        GetStackVector(direction, stack);
        stack += 3;
        spread = GetStackFloat(stack++);
        speed = GetStackFloat(stack++);
        power = GetStackFloat(stack++);
        gravity = GetStackFloat(stack++);
        life = GetStackInt(stack++);
        count = GetStackInt(stack);
    }
    HitEffect[index].SethitEffect(position, direction, spread, speed, power, gravity, life, count);
    HitEffect[index].kind = 1;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _COPY_CHARA__FP12RS_STACKDATAi);
/**
 * Returns the start button to the script.
 */
static int _GET_START_BUTTON(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EdEventInfo.start_button);
    return 1;
}

/**
 * Move interior.
 */
static int _MOVE_INTERIOR(RS_STACKDATA *stack, int argc) {
    EdEventInfo.jump_point = GetStackInt(stack++);
    strcpy(EdEventInfo.jump_map_name, GetStackString(stack++));
    if (argc > 2) {
        EdEventInfo.event_no = GetStackInt(stack);
    } else {
        EdEventInfo.event_no = 100;
    }
    EdEventInfo.request = exit_enter_interior;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_MONSTER_TALK_DATA__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _FUNC_POINT_SHOW__FP12RS_STACKDATAi);
/**
 * Returns the now map no to the script.
 */
static int _GET_NOW_MAP_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->now_map_no);
    return 1;
}

/**
 * Returns the now submap no to the script.
 */
static int _GET_NOW_SUBMAP_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->now_sub_map_no);
    return 1;
}

/**
 * Returns the old map no to the script.
 */
static int _GET_OLD_MAP_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->old_map_no);
    return 1;
}

/**
 * Returns the old submap no to the script.
 */
static int _GET_OLD_SUBMAP_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->old_sub_map_no);
    return 1;
}

/**
 * Sets the rain character no from the script arguments.
 */
static int _SET_RAIN_CHARA_NO(RS_STACKDATA *stack, int argc) {
    EventRain.SetCharNo(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_EDIT_PARTS_POS__FP12RS_STACKDATAi);
/**
 * Returns the contents position to the script.
 */
static s32 _GET_CONTENTS_POS(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Returns the bpot position to the script.
 */
static int _GET_BPOT_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    sceVu0CopyVector(position, BTsubo.position);
    SetStack(stack++, position[0]);
    SetStack(stack++, position[1]);
    SetStack(stack, position[2]);
    return 1;
}

/**
 * Returns the bpot status to the script.
 */
static int _GET_BPOT_STATUS(RS_STACKDATA *stack, int argc) {
    SetStack(stack, BTsubo.state);
    return 1;
}

/**
 * Returns the person status to the script.
 */
static s32 _GET_PERSON_STATUS(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Returns the control chrid to the script.
 */
static int _GET_CONTROL_CHRID(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->player_chara);
    return 1;
}

/**
 * Sets the camera next reference from the script arguments.
 */
static int _SET_CAMERA_NEXT_REF(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR ref;
    mgCCameraFollow *camera;
    if ((camera = (mgCCameraFollow *)GetActiveCamera()) == NULL) {
        return 0;
}
    switch (argc) {
        case 1: {
            int key = GetStackInt(stack);
            int count;
            int i;
            ARG_LIST *node;
            ARG_DATA *args;

            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
                return 0;
            }
            GetArgVector(ref, args);
            break;
        }
        case 3:
            GetStackVector(ref, stack);
            break;
        default:
        return 0;
    }
    camera->SetNextRef(ref);
    return 1;
}

/**
 * Requests menu.
 */
static int _GOTO_MENU(RS_STACKDATA *stack, int argc) {
    MenuArg.open_type = GetStackInt(stack++);
    for (int arg_no = 0; arg_no < argc - 1; arg_no++) {
        MenuArg.param[arg_no] = GetStackInt(stack++);
    }
    EdEventInfo.command_mode = request_menu;
    return 1;
}

/**
 * Returns failure for the menu-status script query.
 */
static s32 _GET_MENU_STATUS(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Loads the equip.
 */
static int _LOAD_EQUIP(RS_STACKDATA *stack, int argc) {
    char label[0x20];
    char bone_name[0x2C];
    char *name;
    int stack_no;
    int member_kind;
    int equip_kind;
    int chara_no;
    int attach_no;
    switch (argc) {
        case 1: {
            int key = GetStackInt(stack);
            int count;
            int i;
            ARG_LIST *node;
            ARG_DATA *args;

            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            argc = node->arg_num;
            args = node->args;
        haveArgs:
            if (args == NULL) {
                return 0;
            }
            stack_no = GetArgInt(args++);
            member_kind = GetArgInt(args++);
            equip_kind = GetArgInt(args++);
            name = GetArgString(args++);
            chara_no = GetArgInt(args++);
            if (argc >= 6) {
                attach_no = GetArgInt(args);
            }
            break;
        }
        case 5:
        case 6:
            stack_no = GetStackInt(stack++);
            member_kind = GetStackInt(stack++);
            equip_kind = GetStackInt(stack++);
            name = GetStackString(stack++);
            chara_no = GetStackInt(stack++);
            if (argc >= 6) {
                attach_no = GetStackInt(stack);
            }
            break;
        default:
        return 0;
    }
    if (member_kind < 0 || member_kind > 1) {
        return 0;
    }
    if (equip_kind < 0 || equip_kind > 4) {
        return 0;
    }
    CUserDataManager *user_data = NULL;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    user_data = &save->user_data;
    if (user_data == NULL) {
        return 0;
    }
    char *path;
    CCharacter2 *chara;
    int texb;
    u32 *pack;
    if ((path = (char *)user_data->GetCharaEquipDataPath(member_kind, equip_kind)) == NULL) {
        return 0;
    }
    if ((chara = GetCharacter(chara_no)) == NULL) {
        return 0;
    }
    if (0 > (texb = EventScene->GetCharaTexb(chara_no))) {
        return 0;
    }
    if ((pack = GetLoadBGBuff(path, 0)) == NULL) {
        return 0;
    }
    if (equip_kind >= 3 || equip_kind >= 4) {
        mgCMemory *memory;
        if ((memory = (mgCMemory *)EventScene->GetStack(stack_no)) == NULL) {
            return 0;
        }
        sprintf(label, "ev%d", chara_no);
        mgCTextureManager *manager = &mgTexManager;
        if (chara_no >= 8) {
            strcpy(manager->name_suffix, label);
        }
        chara->LoadSkin(pack, name, "", memory, texb);
        if (chara_no >= 8) {
            manager->name_suffix[0] = 0;
        }
    } else {
        if (_LOAD_CHARA_sub(stack_no, &name, attach_no, pack) <= 0) {
            return 0;
        }
        if (member_kind == 0) {
            switch (equip_kind) {
                case 0:
                    strcpy(bone_name, "ef00");
                    break;
                case 1:
                    strcpy(bone_name, "gun_hand");
                    break;
                case 2:
                    strcpy(bone_name, "hat");
                    break;
            }
        } else if (member_kind == 1) {
            switch (equip_kind) {
                case 0:
                    strcpy(bone_name, "sword_hand");
                    break;
                case 1:
                    strcpy(bone_name, "wr");
                    break;
                case 2:
                    strcpy(bone_name, "ac");
                    break;
            }
        }
        CCharacter2 *attached;
        if ((attached = GetCharacter(attach_no)) == NULL) {
            return 0;
        }
        if (chara->CObjectFrame::frame == NULL) {
            return 0;
        }
        mgCFrame *bone;
        if ((bone = chara->CObjectFrame::frame->SearchFrame(bone_name)) == NULL) {
            return 0;
        }
        if (attached->CObjectFrame::frame == NULL) {
            return 0;
        }
        attached->CObjectFrame::frame->SetReference(bone);
    }
    return 1;
}

/**
 * Returns the equip itemno to the script.
 */
static int _GET_EQUIP_ITEMNO(RS_STACKDATA *stack, int argc) {
    int chara_no = GetStackInt(stack++);
    int slot = GetStackInt(stack++);
    if (chara_no < 0 || chara_no > 1) {
        return 0;
    }
    CUserDataManager *user_data;

    if (slot < 0 || (user_data = NULL, slot > 4)) {
        return 0;
    }
    CSaveData *save = GetSaveData();
    if (save != NULL) {
        user_data = &save->user_data;
    }
    if (user_data == NULL) {
        return 0;
    }
    CHARA_DATA *chara = user_data->GetCharaDataPtr(chara_no);
    if (chara == NULL) {
        return 0;
    }
    SetStack(stack, chara->equip[slot].item_no);
    return 1;
}

/**
 * Sets the time step enable from the script arguments.
 */
static int _SET_TIME_STEP_ENABLE(RS_STACKDATA *stack, int argc) {
    EventScene->time_step = GetStackInt(stack);
    return 1;
}

/**
 * Sets the door material from the script arguments.
 */
static s32 _SET_DOOR_MATERIAL(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Init drama scene.
 */
static s32 _INIT_DRAMA_SCENE(RS_STACKDATA *stack, s32 argc) {
    InitDramaScene();
    return 1;
}

/**
 * Sets the active cmrid from the script arguments.
 */
static int _SET_ACTIVE_CMRID(RS_STACKDATA *stack, int argc) {
    EventScene->active_camera = GetStackInt(stack);
    return 1;
}

/**
 * Sets the before cmrid from the script arguments.
 */
static int _SET_BEFORE_CMRID(RS_STACKDATA *stack, int argc) {
    EventScene->before_camera = GetStackInt(stack);
    return 1;
}

/**
 * Dngmap load.
 */
static int _DNGMAP_LOAD(RS_STACKDATA *stack, int argc) {
    int texBase;
    RS_STACKDATA *arg = stack + 1;
    mgCMemory *memory = (mgCMemory *)EventScene->GetStack(GetStackInt(stack));
    int fileIndex = GetStackInt(arg++);
    int texCount = EventScene->event_texb_num;
    texBase = EventScene->event_texb;
    if (texCount <= 0 || texCount < fileIndex) {
        return 0;
    }
    int param1 = GetStackInt(arg++);
    int param2 = GetStackInt(arg++);
    int param3 = GetStackInt(arg);
    memory->Align64();
    if (memory->stAlloc64(EventDngMap.LoadDngInfo(
            memory, texBase + fileIndex, param1, param2, param3)) == 0) {
        return 0;
    }

    EventDngMap.active = 0;
    return 1;
}

/**
 * Dngmap delete.
 */
static int _DNGMAP_DELETE(RS_STACKDATA *stack, int argc) {
    EventDngMap.DeleteTexBlock();
    return 1;
}

/**
 * Dngmap move piece.
 */
static int _DNGMAP_MOVE_PIECE(RS_STACKDATA *stack, int argc) {
    EventDngMap.SetKomaMove(GetStackInt(stack));
    return 1;
}

/**
 * Dngmap onoff.
 */
static int _DNGMAP_ONOFF(RS_STACKDATA *stack, int argc) {
    EventDngMap.active = (u8)(GetStackInt(stack) != 0);
    return 1;
}

/**
 * Dngmap set fade.
 */
static int _DNGMAP_SET_FADE(RS_STACKDATA *stack, int argc) {
    int fadeIn = GetStackInt(stack++);
    int duration = GetStackInt(stack);
    EventDngMap.active = 1;
    if (fadeIn != 0) {
        EventDngMap.FadeIn(duration);
    } else {
        EventDngMap.FadeOut(duration);
    }
    return 1;
}

/**
 * Returns the before camera next position to the script.
 */
static int _GET_BEFORE_CAMERA_NEXT_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    mgCCamera *camera = EventScene->GetCamera(EventScene->before_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->GetNextPos(position);
    SetStack(stack++, position[0]);
    SetStack(stack++, position[1]);
    SetStack(stack, position[2]);
    return 1;
}

/**
 * Returns the before camera next reference to the script.
 */
static int _GET_BEFORE_CAMERA_NEXT_REF(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    mgCCamera *camera = EventScene->GetCamera(EventScene->before_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->GetNextRef(position);
    SetStack(stack++, position[0]);
    SetStack(stack++, position[1]);
    SetStack(stack, position[2]);
    return 1;
}

/**
 * Sets the camera next position from the script arguments.
 */
static int _SET_CAMERA_NEXT_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    mgCCameraFollow *camera;
    if ((camera = (mgCCameraFollow *)GetActiveCamera()) == NULL) {
        return 0;
    }
    switch (argc) {
        case 1: {
            int key = GetStackInt(stack);
            int count;
            int i;
            ARG_LIST *node;
            ARG_DATA *args;

            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
                return 0;
            }
            GetArgVector(position, args);
            break;
        }
        case 3:
            GetStackVector(position, stack);
            break;
        default:
        return 0;
    }
    camera->SetNextPos(position);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHK_INTERSECTION_POINT__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHK_INTERSECTION_POINT_PIPE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_FOLLOW__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_FOLLOW_A__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_FOLLOW_OFS__FP12RS_STACKDATAi);
/**
 * Sets the follow camera follow flag from the script arguments.
 */
static int _SET_FCAMERA_FOLLOW_FLAG(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    if (GetStackInt(stack) == 1) {
        camera->FollowOn();
    } else {
        camera->FollowOff();
    }
    return 1;
}

/**
 * Follow camera step.
 */
static int _FCAMERA_STEP(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->Step(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_ANGLE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_HEIGHT__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_FCAMERA_DIST__FP12RS_STACKDATAi);
/**
 * Returns the reference angle to the script.
 */
static int _GET_REF_ANGLE(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR direction;
    float yaw;
    float pitch;
    float *directionZ;
    GetStackVector(from, stack);
    GetStackVector(direction, stack + 3);
    stack += 6;
    sceVu0SubVector(direction, direction, from);
    sceVu0Normalize(direction, direction);
    directionZ = &direction[2];
    yaw = atan2f(direction[0], *directionZ);
    pitch = -atan2f(direction[1], sqrtf(direction[0] * direction[0] + *directionZ * *directionZ));
    switch (argc) {
        case 7:
            SetStack(stack, yaw);
            break;
        case 9:
            SetStack(stack++, pitch);
            SetStack(stack++, yaw);
            SetStack(stack, 0.0f);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Dungeon set stage id.
 */
static int _DNG_SET_STAGE_ID(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    dungeon->stage_id = GetStackInt(stack);
    return 1;
}

/**
 * Dungeon get stage id.
 */
static int _DNG_GET_STAGE_ID(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    SetStack(stack, dungeon->stage_id);
    return 1;
}

/**
 * Sets the camera ctrl from the script arguments.
 */
static int _SET_CAMERA_CTRL(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    if (GetStackInt(stack) != 0) {
        camera->FollowOn();
        ((CCameraControl *)camera)->ControlOn();
    } else {
        camera->FollowOff();
        ((CCameraControl *)camera)->ControlOff();
    }
    return 1;
}

/**
 * Returns the follow camera angle to the script.
 */
static int _GET_FCAMERA_ANGLE(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    SetStack(stack, camera->GetAngle());
    return 1;
}

/**
 * Returns the follow camera height to the script.
 */
static int _GET_FCAMERA_HEIGHT(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    SetStack(stack, camera->GetHeight());
    return 1;
}

/**
 * Returns the follow camera dist to the script.
 */
static int _GET_FCAMERA_DIST(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)EventScene->GetCamera(EventScene->active_camera);
    if (camera == NULL) {
        return 0;
    }
    SetStack(stack, camera->GetDistance());
    return 1;
}

/**
 * Returns the invention id to the script.
 */
static int _GET_INVENTION_ID(RS_STACKDATA *stack, int argc) {
    int result;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    CInventUserData *invent_data = &user_data->invent_data;
    if (user_data == NULL) {
        return 0;
    }
    if (invent_data == NULL) {
        return 0;
    }
    int invention_id = GetStackInt(stack++);

    result = invent_data->IsAlreadyCreatedItem(invention_id);
    SetStack(stack, result);
    return 1;
}

/**
 * Function map jump.
 */
static int _FUNCTION_MAP_JUMP(RS_STACKDATA *stack, int argc) {

    CFuncPoint::EventData *request = &EventScene->event_data.event;
    if (!(request->flag & (FUNC_EVENT_ACTION | FUNC_EVENT_DOOR | FUNC_EVENT_UNK_100))) {
        return 0;
    }
    EdEventInfo.jump_point = -1;
    EdEventInfo.event_no = 100;
    if (strcmp(request->unk_38, "exit") == 0) {
        EdEventInfo.request = exit_leave_interior;
    } else {
        strcpy(EdEventInfo.jump_map_name, request->unk_38);
        EdEventInfo.request = exit_enter_interior;
    }
    return 1;
}

/**
 * Function door mode.
 */
static int _FUNCTION_DOOR_MODE(RS_STACKDATA *stack, int argc) {
    EdEventInfo.func_iparam[0] = 0;
    EdEventInfo.func_iparam[1] = EventScene->door_place_no[0];
    EdEventInfo.func_iparam[2] = EventScene->door_place_no[1];
    EdEventInfo.func_fparam[0] = EventScene->door_vec[0];
    EdEventInfo.func_fparam[1] = EventScene->door_vec[1];
    EdEventInfo.func_fparam[2] = EventScene->door_vec[2];
    EdEventInfo.func_fparam[3] = atan2f(EventScene->door_dir_x, EventScene->door_dir_z);
    EdEventInfo.command_mode = request_door;
    return 1;
}

/**
 * Returns the money to the script.
 */
static int _GET_MONEY(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    if (user_data == NULL) {
        return 0;
    }
    SetStack(stack, user_data->money);
    return 1;
}

/**
 * Adds to the money.
 */
static int _ADD_MONEY(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    if (user_data == NULL) {
        return 0;
    }
    user_data->AddMoney(GetStackInt(stack));
    return 1;
}

/**
 * Returns the item count to the script.
 */
static int _GET_ITEM_NUM(RS_STACKDATA *stack, int argc) {
    int result;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    if (user_data == NULL) {
        return 0;
    }
    int item_no = GetStackInt(stack++);

    result = user_data->GetNumSameItem(item_no);
    SetStack(stack, result);
    return 1;
}

/**
 * Checks button and returns the result to the script.
 */
static int _CHECK_BUTTON(RS_STACKDATA *stack, int argc) {
    int button = GetStackInt(stack++);
    int pressed = 0;
    int pad_down = GamePad__2.GetPadDown();
    switch (button) {
        case 0:
            if (LanguageCode == 0) {
                if (pad_down & 0x40) {
                    pressed = 1;
                }
            } else if (pad_down & 0x20) {
                pressed = 1;
            }
            break;
        case 1:
            if (LanguageCode == 0) {
                if (pad_down & 0x20) {
                    pressed = 1;
                }
            } else if (pad_down & 0x40) {
                pressed = 1;
            }
            break;
        case 2:
            if (pad_down & 0x80) {
                pressed = 1;
            }
            break;
        case 3:
            if (pad_down & 0x100) {
                pressed = 1;
            }
            break;
        default:
        return 0;
    }
    SetStack(stack, pressed);
    return 1;
}

/**
 * Returns the language to the script.
 */
static int _GET_LANGUAGE(RS_STACKDATA *stack, int argc) {
    SetStack(stack, LanguageCode);
    return 1;
}

/**
 * Checks invent item and returns the result to the script.
 */
static int _CHECK_INVENT_ITEM(RS_STACKDATA *stack, int argc) {
    int item = GetStackInt(stack++);
    SetStack(stack, CheckInventItem(item));
    return 1;
}
/**
 * Sets the ai from the script arguments.
 */
static int _SET_AI(RS_STACKDATA *stack, int argc) {
    int enabled = GetStackInt(stack++);
    int chara_no = GetStackInt(stack);
    if (enabled != 0) {
        EventScene->CancelStayVillager(chara_no);
    } else {
        EventScene->StayVillager(chara_no);
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHECK_INVENT_PHOTO__FP12RS_STACKDATAi);
/**
 * Returns the photo count to the script.
 */
static int _GET_PHOTO_NUM(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *user_data = &save->user_data;
    CInventUserData *invent_data = &user_data->invent_data;
    if (user_data == NULL) {
        return 0;
    }
    if (invent_data == NULL) {
        return 0;
    }
    SetStack(stack, invent_data->GetNowHavePictureNum());
    return 1;
}

/**
 * Sets the contents etc from the script arguments.
 */
static s32 _SET_CONTENTS_ETC(RS_STACKDATA *stack, int argc) {
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_STATUS__FP12RS_STACKDATAi__2);
/**
 * Requests subgame.
 */
static int _GOTO_SUBGAME(RS_STACKDATA *stack, int argc) {
    int type = GetStackInt(stack);
    SubGameInfo info;
    info.scene = EventScene;
    info.texb = EventScene->unk_3e68;
    info.texb_num = EventScene->unk_3e6c;
    return sgInitSubGame(type, &info) != 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_GYORACE_ETC__FP12RS_STACKDATAi);
/**
 * Returns the gyorace etc to the script.
 */
static int _GET_GYORACE_ETC(RS_STACKDATA *stack, int argc) {
    FISH_PRIZE_INFO info;
    int raceNo;

    switch (GetStackInt(stack++)) {
        case 0:
            SetStack(stack++, GetGyoRaceAquariumNo());
            break;
        case 1:
            SetStack(stack++, GetGyoRaceRanking());
            break;
        case 2:
            SetStack(stack++, GetGyoRaceClass());
            break;
        case 3:
            SetStack(stack++, GetGyoRaceNo());
            break;
        case 4:
            raceNo = GetStackInt(stack++);
            if (!GetFishPrize(raceNo, GetStackInt(stack++) - 1, &info)) {
            return 0;
            }
            SetStack(stack++, info.unk_0);
            SetStack(stack++, info.unk_4);
            break;
        case 5: {
            CSaveData *save = GetSaveData();
            if (save == NULL) {
                return 0;
            }
            SetStack(stack++, save->GetTourCountEtc());
            break;
        }
        default:
        return 0;
    }
    return 1;
}

/**
 * Sets the savedata etc from the script arguments.
 */
static int _SET_SAVEDATA_ETC(RS_STACKDATA *stack, int argc) {
    CSaveData *save;
    CUserDataManager *user;
    MOS_CHANGE_PARAM *bajji;

    switch (GetStackInt(stack++)) {
        case 0:
            save = GetSaveData();
            if (save == NULL) {
            return 0;
            }
            save->game_progress = GetStackInt(stack);
            break;
        case 1:
            save = GetSaveData();
            if (save == NULL) {
            return 0;
            }
            user = &save->user_data;
            if (user == NULL) {
            return 0;
            }
            bajji = user->GetMonsterBajjiDataPtrMosId(GetStackInt(stack));
            if (bajji == NULL) {
            return 0;
            }
            bajji->enable = 1;
            break;
        case 2:
            save = GetSaveData();
            if (save == NULL) {
            return 0;
            }
            user = &save->user_data;
            if (user == NULL) {
            return 0;
            }
            user->AllWeaponRepair();
            break;
        case 3:
            save = GetSaveData();
            if (save == NULL) {
            return 0;
            }
            save->unk_643C9 = GetStackInt(stack);
            break;
        case 4:
            DeleteErekiFish();
            break;
        default:
        return 0;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_SAVEDATA_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DEL_MONSTER__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MENU_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_MENU_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_ANALYZE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_DIORAMA_PERCENT__FP12RS_STACKDATAi);
/**
 * Georama func.
 */
static void _GEORAMA_FUNC(RS_STACKDATA *stack, int argc) {
    GeoFuncParam param;

    param.scene = (CScene *)EventScene;
    GeoramaFunc(&param, stack, argc);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_CHARA_ID__FP12RS_STACKDATAi);
/**
 * Requests editmode.
 */
static int _GOTO_EDITMODE(RS_STACKDATA *stack, int argc) {
    EdEventInfo.request = exit_edit_mode;
    return 1;
}

/**
 * Returns the chapter to the script.
 */
static int _GET_CHAPTER(RS_STACKDATA *stack, int argc) {
    CSaveData *save;

    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    SetStack(stack, GetNowChapter(save));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_NPC_TRAIN_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _REGISTER_VILLAGER__FP12RS_STACKDATAi);
/**
 * Eye view drawing on off.
 */
static int _EYE_VIEW_DRAW_ON_OFF(RS_STACKDATA *stack, int argc) {
    ((CScene *)EventScene)->EyeViewDrawOnOff(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_QUEST_ETC__FP12RS_STACKDATAi);
/**
 * Returns the quest etc to the script.
 */
static int _GET_QUEST_ETC(RS_STACKDATA *stack, int argc) {
    int command = GetStackInt(stack++);
    int quest_no = GetStackInt(stack++);
    switch (command) {
        case 0:
            SetStack(stack, GetQuestRequestStatus(quest_no));
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Returns the old interior map no to the script.
 */
static int _GET_OLD_INTERIOR_MAP_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, GetOldInteriorMapNo());
    return 1;
}

/**
 * Sets the event data from the script arguments.
 */
static int _SET_EVENT_DATA(RS_STACKDATA *stack, int argc) {
    CSceneEventData *eventData;
    RS_STACKDATA *value;

    eventData = &EventScene->event_data;
    if (eventData == NULL) {
        return 0;
    }
    value = stack + 1;
    switch (GetStackInt(stack)) {
        case 0:
            eventData->event.flag = GetStackInt(value);
            break;
        case 1:
            eventData->event.event_no = GetStackInt(value);
            break;
        case 2:
            eventData->event.point_no = GetStackInt(value);
            break;
        case 3:
            eventData->event.unk_2c = GetStackInt(value);
            break;
        case 4:
            eventData->event.unk_30 = GetStackInt(value);
            break;
        case 5:
            eventData->event.unk_34 = GetStackInt(value);
            break;
        case 6:
            eventData->map_event.check_type = GetStackInt(value);
            break;
        case 7:
            eventData->map_event.event_no = GetStackInt(value);
            break;
        case 13:
            eventData->chara_slot = GetStackInt(value);
            break;
        case 14:
            eventData->chara_no = GetStackInt(value);
            break;
        default:
        return 0;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _STOPWATCH__FP12RS_STACKDATAi);
/**
 * Sets the func etc from the script arguments.
 */
static int _SET_FUNC_ETC(RS_STACKDATA *stack, int argc) {
    u32 *flags;

    flags = &EventScene->map_jump_flags;
    if (flags == NULL) {
        return 0;
    }
    if (flags == NULL) {
        return 0;
    }
    switch (GetStackInt(stack)) {
        case 0:
            *flags |= 0x80;
            EdEventInfo.request = 0x13;
            break;
        case 1:
            EdEventInfo.request = 0x12;
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Returns a scene character by its event index.
 */
static CCharacter2 *GetChara(int id) {
    return GetCharacter(id);
}

/**
 * Returns the character position to the script.
 */
static int _GET_CHARA_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    RS_STACKDATA *out;
    CCharacter2 *chara;

    out = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    chara->GetPosition(pos);
    SetStack(out++, pos[0]);
    SetStack(out++, pos[1]);
    SetStack(out++, pos[2]);
    return 1;
}

/**
 * Returns the character talk position to the script.
 */
static int _GET_CHARA_TALK_POS(RS_STACKDATA *stack, int argc) {
    int screenPos[2];
    RS_STACKDATA *out;
    CCharacter2 *chara;

    out = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    GetScrPosFromChar(chara, screenPos);
    SetStack(out++, screenPos[0]);
    SetStack(out, screenPos[1]);
    return 1;
}

/**
 * Turn character.
 */
static int _TURN_CHARA(RS_STACKDATA *stack, int argc) {
    CCharacter2 *chara;
    sceVu0FVECTOR target;
    sceVu0FVECTOR pos;
    sceVu0FVECTOR rot;
    sceVu0FVECTOR diff;
    float rate;
    float angle;
    float *yaw;

    chara = GetChara(GetStackInt(stack++));
    if (chara == NULL) {
        return 0;
    }
    target[0] = GetStackFloat(stack++);
    target[1] = GetStackFloat(stack++);
    target[2] = GetStackFloat(stack++);
    rate = GetStackFloat(stack);
    chara->GetPosition(pos);
    chara->GetRotation(rot);
    sceVu0SubVector(diff, target, pos);
    angle = atan2f(diff[0], diff[2]);
    yaw = &rot[1];
    *yaw = mgAngleInterpolate(*yaw, angle, rate, 0);
    chara->SetRotation(rot);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CHARA_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CHARA_ROT__FP12RS_STACKDATAi);
/**
 * Returns the character rotation to the script.
 */
static int _GET_CHARA_ROT(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    RS_STACKDATA *out;
    CCharacter2 *chara;

    out = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    chara->GetRotation(pos);
    SetStack(out++, pos[0]);
    SetStack(out++, pos[1]);
    SetStack(out++, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MOTION__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_STEP__FP12RS_STACKDATAi);
/**
 * Sets the texture animation from the script arguments.
 */
static int _SET_TEX_ANIM(RS_STACKDATA *stack, int argc) {
    int charaId;
    int enable;
    char *name;
    int texBank;
    mgCTextureManager *manager;

    charaId = GetStackInt(stack++);
    enable = GetStackInt(stack++);
    name = NULL;
    if (argc > 2) {
        name = GetStackString(stack);
    }
    texBank = EventScene->GetCharaTexb(charaId);
    if (texBank < 0) {
        return 0;
    }
    manager = &mgTexManager;
    if (enable != 0) {
        manager->TexAnimeOn(texBank, name);
    } else if (name != NULL) {
        manager->TexAnimeOff(texBank, name);
    } else {
        manager->TexAnimeAllOff(texBank);
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_SCALE__FP12RS_STACKDATAi__2);
/**
 * Sets the reference from the script arguments.
 */
static int _SET_REFERENCE(RS_STACKDATA *stack, int argc) {
    int charaId;
    char *frameName;
    int referenceId;
    CCharacter2 *chara;
    CCharacter2 *referenceChara;
    mgCFrame *frame;
    mgCFrame *child;

    charaId = GetStackInt(stack++);
    frameName = GetStackString(stack++);
    referenceId = GetStackInt(stack);
    chara = GetChara(charaId);
    if (chara == NULL) {
        return 0;
    }
    referenceChara = GetChara(referenceId);
    if (referenceChara == NULL) {
        return 0;
    }
    frame = chara->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    child = frame->SearchFrame(frameName);
    if (child == NULL) {
        return 0;
    }
    frame = referenceChara->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    frame->SetReference(child);
    return 1;
}

/**
 * Deletes the reference.
 */
static int _DEL_REFERENCE(RS_STACKDATA *stack, int argc) {
    mgCFrame *frame;
    CCharacter2 *chara;

    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    frame = chara->CObjectFrame::frame;
    if (frame == NULL) {
        return 0;
    }
    frame->DeleteReference();
    return 1;
}

/**
 * Shadow clip off.
 */
static int _SHADOW_CLIP_OFF(RS_STACKDATA *stack, int argc) {
    int charaId;
    CCharacter2 *chara;
    int clipOff;

    clipOff = 1;
    charaId = GetStackInt(stack++);
    if (argc > 0) {
        clipOff = GetStackInt(stack);
    }
    chara = GetChara(charaId);
    if (chara == NULL) {
        return 0;
    }
    if (chara->shadow_frame == NULL) {
        return 0;
    }
    mgCFrameAttr attr;
    attr.no_cull = clipOff;
    chara->shadow_frame->SetAttrParam(attr, 1, 0x100000);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_COORDINATE_ANGLE__FP12RS_STACKDATAi);
/**
 * Returns the character width to the script.
 */
static int _GET_CHARA_WIDTH(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    CCharacter2 *chara;

    nextSlot = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    SetStack(nextSlot, chara->body_width);
    return 1;
}
/**
 * Returns the character height.
 */
static int _GET_CHARA_HEIGHT(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    CCharacter2 *chara;

    nextSlot = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    SetStack(nextSlot, chara->body_height);
    return 1;
}

/**
 * Returns the character weight.
 */
static int _GET_CHARA_WEIGHT(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    CCharacter2 *chara;

    nextSlot = stack + 1;
    chara = GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    SetStack(nextSlot, chara->body_depth);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CHARA_SHOW__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_CHARA_SHOW__FP12RS_STACKDATAi);
/**
 * Enables the character deformation animation.
 */
static int _CHARA_DA_ENABLE(RS_STACKDATA *stack, int argc) {
    CCharacter2 *chara;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    if ((chara = GetChara(GetStackInt(stack))) == NULL) {
        return 0;
    }
    chara->SetDAnimeEnable(GetStackInt(nextSlot));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_MOT_NOW_WAIT__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHECK_MOTION_END__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ACTCHR_SET_MOTION__FP12RS_STACKDATAi);
/**
 * Sets the character additional soundid.
 */
static int _SET_CHARA_EX_SOUNDID(RS_STACKDATA *stack, int argc) {
    CActionChara *chara;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    chara = (CActionChara *)GetChara(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    chara->se_bank_2 = GetStackInt(nextSlot);
    return 1;
}

/**
 * Copies the action character sound info.
 */
static int _ACTCHR_SOUND_INFO_COPY(RS_STACKDATA *stack, int argc) {
    CActionChara *chara;

    chara = (CActionChara *)GetCharacter(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    if (chara == NULL) {
        return 0;
    }
    chara->SetSoundInfoCopy();
    return 1;
}

/**
 * Gets an event message window by its identifier.
 */
static ClsMes *GetMes(int id) {
    return GetEventMessage(id);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _MES_MAKE__FP12RS_STACKDATAi);
/**
 * Closes the message.
 */
static int _MES_CLOSE(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;

    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->draw_speed = mes->GetDrawSpeedDef();
    mes->mes_no = -1;
    mes->unk_1e40 = 0;
    mes->open = 0;
    mes->fade = 0;
    mes->fukidashi_centre_x = -1;
    mes->fukidashi_centre_y = -1;
    return 1;
}

/**
 * Advances a message to its next page once scrolling has stopped and a page wait is active.
 */
static int _MES_NEXTPAGE(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;

    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    if (mes->scroll_wait != 0) {
        return 1;
    }
    if (mes->page_wait == 0) {
        return 1;
    }
    mes->GoNextPage();
    return 1;
}

/**
 * Sets the message autoset.
 */
static int _SET_MES_AUTOSET(RS_STACKDATA *stack, int argc) {
    int values[4];
    int charaValues[4];
    RS_STACKDATA *args;
    ClsMes *mes;
    CCharacter2 *chara1;
    CCharacter2 *chara2;
    int i;

    args = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    if (argc != 3) {
        if (argc != 5) {
            return 0;
        }
        for (i = 0; i < 4; i++) {
            values[i] = GetStackInt(args++);
        }
        mes->AutoSet(values);
    } else {
        chara1 = GetChara(GetStackInt(args++));
        if (chara1 == NULL) {
            return 0;
        }
        chara2 = GetChara(GetStackInt(args));
        if (chara2 == NULL) {
            return 0;
        }
        mes->AutoSetSub(chara1, chara2, charaValues);
        mes->AutoSet(charaValues);
    }
    return 1;
}

/**
 * Sets the message shippo.
 */
static int _SET_MES_SHIPPO(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;

    mes = GetMes(GetStackInt(stack++));
    if (mes == NULL) {
        return 0;
    }
    mes->tail_on = GetStackInt(stack++);
    if (argc > 2) {
        mes->tail_length = GetStackInt(stack++);
    }
    if (argc > 3) {
        mes->tail_half_w = GetStackInt(stack);
    }
    return 1;
}

/**
 * Sets the message position.
 */
static int _SET_MES_POS(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->fukidashi_pos = GetStackInt(nextSlot);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MES_DRAWSPEED__FP12RS_STACKDATAi);
/**
 * Sets the message cursor.
 */
static int _SET_MES_CURSOR(RS_STACKDATA *stack, int argc) {
    int cursor;
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    cursor = GetStackInt(nextSlot);
    if (mes->select < 0) {
        mes->cursor_time = 0;
    }
    mes->select = cursor;
    return 1;
}

/**
 * Sets the message okuri.
 */
static int _SET_MES_OKURI(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->push_button = GetStackInt(nextSlot);
    return 1;
}

/**
 * Sets the message win flag.
 */
static int _SET_MES_WIN_FLAG(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    if (GetStackInt(nextSlot) != 0) {
        mes->window_mode = 1;
    } else {
        mes->window_mode = 0;
    }
    return 1;
}

/**
 * Checks the message complete.
 */
static int _CHECK_MES_COMPLETE(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    SetStack(nextSlot, mes->State() == 3);
    return 1;
}

/**
 * Checks the message wait.
 */
static int _CHECK_MES_WAIT(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    SetStack(nextSlot, mes->State() == 5);
    return 1;
}

/**
 * Checks the message.
 */
static int _CHECK_MES(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    SetStack(nextSlot, mes->State() == 0);
    return 1;
}

/**
 * Sets the message fukidashi.
 */
static int _SET_MES_FUKIDASHI(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    GetStackInt(nextSlot);
    mes->SetWindowMode(1);
    return 1;
}

/**
 * Sets the message window mode.
 */
static int _SET_MES_WINDOW_MODE(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->SetWindowMode(GetStackInt(nextSlot));
    return 1;
}

/**
 * Sets the message preset.
 */
static int _SET_MES_PRESET(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->Preset(GetStackInt(nextSlot));
    return 1;
}

/**
 * Sets the message item direct.
 */
static int _SET_MES_ITEM_DIRECT(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *args;
    int slot;
    int value;

    args = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    slot = GetStackInt(args++);
    value = GetStackInt(args);
    if (slot - 1 >= 0 && slot - 1 < 0x10) {
        mes->item_mes[slot - 1] = value;
    }
    return 1;
}

/**
 * Sets the message item.
 */
static int _SET_MES_ITEM(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *args;
    int slot;
    int value;

    args = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    slot = GetStackInt(args++);
    value = GetItemMessageNo(GetStackInt(args), 1);
    if (slot - 1 >= 0 && slot - 1 < 0x10) {
        mes->item_mes[slot - 1] = value;
    }
    return 1;
}

/**
 * Sets the message value.
 */
static int _SET_MES_VALUE(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *args;
    int slot;
    int value;

    args = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    slot = GetStackInt(args++);
    value = GetStackInt(args);
    if (slot == 0) {
        mes->value = value;
    } else {
        mes->values[slot - 1] = value;
        mes->value_width[slot - 1] = 0;
    }
    return 1;
}

/**
 * Returns the message status.
 */
static int _GET_MES_STATUS(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    SetStack(nextSlot, mes->State());
    return 1;
}

/**
 * Returns the party character message number.
 */
static int _GET_PARTY_CHARA_MES_NO(RS_STACKDATA *stack, int argc) {
    int npcId;
    int kind;
    RS_STACKDATA *out;

    out = stack + 1;
    npcId = GetStackInt(stack);
    kind = GetStackInt(out++);
    SetStack(out++, GetPartyCharaMessage(npcId, kind, 1));
    if (kind == 4) {
        SetStack(out, (s8)GetPartyNPCData(npcId)->unk_31 + 3);
    }
    return 1;
}

/**
 * Sets the message buffer.
 */
static int _MES_SET_BUFF(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    int kind;
    RS_STACKDATA *args;

    args = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    kind = GetStackInt(args++);
    GetStackInt(args);
    if (kind == 0) {
        mes->SetBuff(GetSysMesBuffer());
    } else {
        mes->SetBuff_system(GetSystemMesBuffer());
    }
    return 1;
}

/**
 * Returns the message window mode.
 */
static int _GET_MES_WINDOW_MODE(RS_STACKDATA *stack, int argc) {
    int result;
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }

    result = mes->GetWindowMode();
    SetStack(nextSlot, result);
    return 1;
}

/**
 * Returns the message voice.
 */
static int _GET_MES_VOICE(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EdEventInfo.stream_playing);
    return 1;
}

/**
 * Sets the message question gyou.
 */
static int _SET_MES_QUESTION_GYOU(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->select_top = GetStackInt(nextSlot);
    return 1;
}

/**
 * Returns the message question gyou.
 */
static int _GET_MES_QUESTION_GYOU(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;
    ClsMes *mes;

    nextSlot = stack + 1;

    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    SetStack(nextSlot, mes->select_top);
    return 1;
}

/**
 * Sets the message close cnt.
 */
static int _SET_MES_CLOSE_CNT(RS_STACKDATA *stack, int argc) {
    ClsMes *mes;
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;
    mes = GetMes(GetStackInt(stack));
    if (mes == NULL) {
        return 0;
    }
    mes->close_time = GetStackInt(nextSlot);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MES_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_MES_ETC__FP12RS_STACKDATAi);
int _LOAD_MES_sub(char *name, int no, ClsMes *mes) {
    int size;
    u32 *src;
    mgCMemory *mem;
    char *dst;
    int t;

    if (name == NULL) {
        return 0;
    }
    if (mes == NULL) {
        return 0;
    }
    src = GetLoadBGBuff(name, &size);
    if (src == 0) {
        return 0;
    }
    mem = EventScene->GetStack(no);
    mem->Align64();
    dst = (char *)mem->stAllocTest(size / 16 + 1);
    if (dst == NULL) {
        return 0;
    }
    mem->stAlloc64(size / 16 + 1);
    memcpy(dst, src, size);
    t = size;
    mes->mes_data = dst;
    mes->mes_data_size = t;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_MES__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_MES_MONS_TALK__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _MES_SE_PLAY__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MES_STR__FP12RS_STACKDATAi);
/**
 * Returns the message okuri.
 */
static int _GET_MES_OKURI(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    ClsMes *mes = GetMes(GetStackInt(stack++));
    if (mes == NULL) {
        return 0;
    }
    SetStack(stack, mes->push_button);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_FISHINGTOURNAMENT_ETC__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CHARA_FAR_DIST__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MODEL_LIGHT_SWITCH__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MODEL_LIGHT_COLOR__FP12RS_STACKDATAi__2);
/**
 * Returns the extra flag.
 */
static int _GET_OMAKE_FLAG(RS_STACKDATA *stack, int argc) {
    SetStack(stack, OmakeFlag);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_WIND__FP12RS_STACKDATAi);
/**
 * Sets the extra flag.
 */
static int _SET_OMAKE_FLAG(RS_STACKDATA *stack, int argc) {
    OmakeFlag = GetStackInt(stack);
    return 1;
}

/**
 * Gets the active camera controller.
 */
static CCameraControl *GetCamera() {
    return (CCameraControl *)GetActiveCamera();
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CAMERA_POS__FP12RS_STACKDATAi);
/**
 * Returns the camera position.
 */
static int _GET_CAMERA_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    if (argc < 3) {
        return 0;
    }
    mgCCamera *camera = GetCamera();
    if (camera == NULL) {
        return 0;
    }
    camera->GetPos( pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CAMERA_REF__FP12RS_STACKDATAi);
/**
 * Returns the camera ref.
 */
static int _GET_CAMERA_REF(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    if (argc < 3) {
        return 0;
    }
    mgCCamera *camera = GetCamera();
    if (camera == NULL) {
        return 0;
    }
    camera->GetRef( pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CAMERA_SPEED__FP12RS_STACKDATAi__2);
/**
 * Moves the active follow camera by the script-specified number of steps.
 */
static int _CAMERA_STEP(RS_STACKDATA *stack, int argc) {
    mgCCameraFollow *camera = (mgCCameraFollow *)GetCamera();
    if (camera == NULL) {
        return 0;
    }
    camera->Step(GetStackInt(stack));
    return 1;
}

/**
 * Returns the before camera position.
 */
static int _GET_BEFORE_CAMERA_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    mgCCamera *camera = EventScene->GetCamera(EventScene->before_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->GetPos( pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Returns the before camera ref.
 */
static int _GET_BEFORE_CAMERA_REF(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    mgCCamera *camera = EventScene->GetCamera(EventScene->before_camera);
    if (camera == NULL) {
        return 0;
    }
    camera->GetRef( pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_INIT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_SYNC_CHARA(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_SET_POS(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOVE(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOVE_STEP(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_ROT_REF(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_ROT_ANGLE(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_CLEAR_ROT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_WAIT_ROT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_ROT_MOVE(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_SET_ROT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_DELAY_ROT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOTION_TRG(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOTION_PLAY(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOTION_STOP(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_MOTION_NEXT(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_ANIME_TRG(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_ANIME(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_SE_PLAY(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Shows or hides an event image.
 */
static void _IMG_SET_DRAW(RS_STACKDATA *stack, int argc) {
    int draw;

    draw = GetStackInt(stack++);
    esMother.SetDraw(GetStackInt(stack), draw);
}

/**
 * Sets the texture rectangle displayed by an event image.
 */
static void _IMG_SET_GET(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int x = GetStackInt(stack++);
    int y = GetStackInt(stack++);
    int width = GetStackInt(stack++);
    esMother.SetGet(index, x, y, width, GetStackInt(stack));
}

/**
 * Sets the screen rectangle occupied by an event image.
 */
static void _IMG_SET_PUT(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int x = GetStackInt(stack++);
    int y = GetStackInt(stack++);
    int width = GetStackInt(stack++);
    esMother.SetPut(index, x, y, width, GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _IMG_SET_NAME__FP12RS_STACKDATAi);
/**
 * Starts moving an event image to a screen position.
 */
static void _IMG_SET_MOVE(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int x = GetStackInt(stack++);
    int y = GetStackInt(stack++);
    esMother.SetMove(index, x, y, GetStackInt(stack));
}

/**
 * Starts fading an event image in or out over the requested number of frames.
 */
static void _IMG_SET_FADE(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int from = GetStackInt(stack++);
    esMother.SetFade(index, from, GetStackInt(stack));
}

/**
 * Sets the red, green, blue and alpha components of an event image colour.
 */
static void _IMG_SET_COLOR(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int r = GetStackInt(stack++);
    int g = GetStackInt(stack++);
    int b = GetStackInt(stack++);
    esMother.SetColor(index, r, g, b, GetStackInt(stack));
}

/**
 * Gets an event sprite by its slot number.
 */
static CEventSprite2 *GetEventSprite(int no) {
    if (no < 0 || no >= event_sprite2_num) {
        return NULL;
    }
    return &EventSprite2[no];
}

/**
 * Initializes the sprite.
 */
static int _SPRITE_INIT(RS_STACKDATA *stack, int argc) {
    CEventSprite2 *sprite;

    sprite = GetEventSprite(GetStackInt(stack));
    if (sprite == NULL) {
        return 0;
    }
    sprite->Initialize();
    return 1;
}

/**
 * Shows or hides an event sprite.
 */
static int _SPRITE_SET_DRAW(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int value = GetStackInt(stack);
    CEventSprite2 *sprite = GetEventSprite(index);
    if (sprite == NULL) {
        return 0;
    }
    sprite->SetDrawFlag(value);
    return 1;
}

/**
 * Sets the drawing type of an event sprite.
 */
static int _SPRITE_SET_TYPE(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int value = GetStackInt(stack);
    CEventSprite2 *sprite = GetEventSprite(index);
    if (sprite == NULL) {
        return 0;
    }
    sprite->SetSpriteType(value);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_TEXTURE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_PUTSIZE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_UVSIZE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_COLOR__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPRITE_SET_SCALE__FP12RS_STACKDATAi);
/**
 * Sets an event sprite alpha-blending mode.
 */
static int _SPRITE_SET_ALPHAB(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    int value = GetStackInt(stack);
    CEventSprite2 *sprite = GetEventSprite(index);
    if (sprite == NULL) {
        return 0;
    }
    sprite->SetAlphaBlend(value);
    return 1;
}

/**
 * Returns whether the camera position, angle, fade or shake tracks are still running.
 */
static int _CMRS_CHECK(RS_STACKDATA *stack, int argc) {
    SetStack(stack, (u8)((CameraSeq.CheckEnd() != 0) ^ 1));
    return 1;
}

/**
 * Clears the camera sequence tracks and camera state.
 */
static int _CMRS_INIT(RS_STACKDATA *stack, int argc) {
    CameraSeq.Clear();
    return 1;
}

/**
 * Queues a wait on the camera sequence position track.
 */
static int _CMRS_PRDELAY(RS_STACKDATA *stack, int argc) {
    CameraSeq.PRDelay(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_SET_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_SET_REF__FP12RS_STACKDATAi);
/**
 * Queues a wait on the camera sequence angle track.
 */
static int _CMRS_AHDDELAY(RS_STACKDATA *stack, int argc) {
    CameraSeq.AHDDelay(GetStackInt(stack));
    return 1;
}

/**
 * Sets the cmrs angle.
 */
static int _CMRS_SET_ANGLE(RS_STACKDATA *stack, int argc) {
    CameraSeq.SetAngle(GetStackFloat(stack));
    return 1;
}

/**
 * Sets the cmrs height.
 */
static int _CMRS_SET_HEIGHT(RS_STACKDATA *stack, int argc) {
    CameraSeq.SetHeight(GetStackFloat(stack));
    return 1;
}

/**
 * Sets the cmrs distance.
 */
static int _CMRS_SET_DIST(RS_STACKDATA *stack, int argc) {
    CameraSeq.SetDist(GetStackFloat(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_SET_AHD__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE2__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE_REF__FP12RS_STACKDATAi);
/**
 * Initializes the cmrs position animation.
 */
static int _CMRS_INIT_PAS(RS_STACKDATA *stack, int argc) {
    CameraSeq.InitPas();
    return 1;
}

/**
 * Sets the cmrs position animation frame.
 */
static int _CMRS_SET_PAS_FRM(RS_STACKDATA *stack, int argc) {
    CameraSeq.SetPasFrm(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_ADD_PAS__FP12RS_STACKDATAi);
/**
 * Queues running the camera sequence along its initialized path.
 */
static int _CMRS_START_PAS(RS_STACKDATA *stack, int argc) {
    CameraSeq.StartPas();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_PR_SLOWING__FP12RS_STACKDATAi);
/**
 * Queues a return point on the camera sequence position track.
 */
static int _CMRS_PR_KEEP(RS_STACKDATA *stack, int argc) {
    CameraSeq.PRKeep();
    return 1;
}

/**
 * Queues a jump to the camera sequence position track return point.
 */
static int _CMRS_PR_RETURN(RS_STACKDATA *stack, int argc) {
    CameraSeq.PRReturn();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE_AHD__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_SYNC_OBJ__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE_AHD2__FP12RS_STACKDATAi);
/**
 * Queues stopping the camera sequence from following its event object.
 */
static int _CMRS_RELEASE_OBJ(RS_STACKDATA *stack, int argc) {
    CameraSeq.ReleaseSyncObj();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_AHD_SLOWING__FP12RS_STACKDATAi);
/**
 * Queues a return point on the camera sequence angle track.
 */
static int _CMRS_AHD_KEEP(RS_STACKDATA *stack, int argc) {
    CameraSeq.AHDKeep();
    return 1;
}

/**
 * Queues a jump to the camera sequence angle track return point.
 */
static int _CMRS_AHD_RETURN(RS_STACKDATA *stack, int argc) {
    CameraSeq.AHDReturn();
    return 1;
}

/**
 * Queues a wait on the camera sequence fade track.
 */
static int _CMRS_FADE_DELAY(RS_STACKDATA *stack, int argc) {
    CameraSeq.FadeDelay(GetStackInt(stack));
    return 1;
}

/**
 * Initializes the cmrs fade.
 */
static int _CMRS_FADE_INIT(RS_STACKDATA *stack, int argc) {
    CameraSeq.FadeInit();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_FADE_IN__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_FADE_OUT__FP12RS_STACKDATAi);
/**
 * Queues a wait on the camera sequence shake track.
 */
static int _CMRS_QUAKE_DELAY(RS_STACKDATA *stack, int argc) {
    CameraSeq.QuakeDelay(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_QUAKE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_QUAKE2__FP12RS_STACKDATAi);
/**
 * Queues a wait on the camera sequence character track.
 */
static int _CMRS_CHARA_DELAY(RS_STACKDATA *stack, int argc) {
    CameraSeq.CharaDelay(GetStackInt(stack));
    return 1;
}

/**
 * Queues keeping a character the requested distance in front of the camera.
 */
static int _CMRS_CHARA_ATTACH(RS_STACKDATA *stack, int argc) {
    int kind;
    float factor;

    kind = GetStackInt(stack++);
    factor = GetStackFloat(stack++);
    CameraSeq.CharaAttach(kind, factor, GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CMRS_MOVE_POS__FP12RS_STACKDATAi);
/**
 * Checks the object sequence.
 */
static int _OBJS_CHECK(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack++));
    if (seq == NULL) {
        return 0;
    }

    SetStack(stack, (u8)((seq->CheckEnd() != 0) ^ 1));
    return 1;
}

/**
 * Initializes the object sequence.
 */
static int _OBJS_INIT(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->Clear();
    return 1;
}

/**
 * Selects the event object handle controlled by an object sequence.
 */
static int _OBJS_SYNC_OBJ(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetEohNo(frame);
    return 1;
}

/**
 * Queues a wait on an object sequence position track.
 */
static int _OBJS_POS_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->PosDelay(frame);
    return 1;
}

/**
 * Sets the object sequence position.
 */
static int _OBJS_SET_POS(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    sceVu0FVECTOR pos;
    int slot;
    int key;
    int count;
    int i;
    ARG_LIST *node;
    ARG_DATA *args;

    switch (argc) {
        case 1:

            key = GetStackInt(stack);
            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
            return 0;
            }
            slot = GetArgInt(args++);
            GetArgVector(pos, args);
            break;
        case 4:
            slot = GetStackInt(stack++);
            GetStackVector(pos, stack);
            break;
        default:
        return 0;
    }
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetPos(pos);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_MOVE__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_MOVE2__FP12RS_STACKDATAi);
/**
 * Initializes the object sequence position animation.
 */
static int _OBJS_INIT_PAS(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->InitPas();
    return 1;
}

/**
 * Sets the object sequence position animation frame.
 */
static int _OBJS_SET_PAS_FRM(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetPasFrm(frame);
    return 1;
}

/**
 * Adds the object sequence position animation.
 */
static int _OBJS_ADD_PAS(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    sceVu0FVECTOR pos;
    int slot;
    int key;
    int count;
    int i;
    ARG_LIST *node;
    ARG_DATA *args;

    switch (argc) {
        case 1:

            key = GetStackInt(stack);
            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
            return 0;
            }
            slot = GetArgInt(args++);
            GetArgVector(pos, args);
            break;
        case 4:
            slot = GetStackInt(stack++);
            GetStackVector(pos, stack);
            break;
        default:
        return 0;
    }
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->AddPas(pos);
    return 1;
}

/**
 * Starts the object sequence position animation.
 */
static int _OBJS_START_PAS(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int seqNo;
    int frame;

    frame = 0;
    seqNo = GetStackInt(stack++);
    if (argc >= 2) {
        frame = GetStackInt(stack);
    }
    seq = GetObjSeq(seqNo);
    if (seq == NULL) {
        return 0;
    }
    seq->StartPas(frame);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_JUMP__FP12RS_STACKDATAi);
/**
 * Sets the object sequence object handle frame position.
 */
#ifdef NONMATCHING
static int _OBJS_SET_EOH_FRAME_POS(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    sceVu0FVECTOR offset;
    int slot;
    int eohNo;
    int frames;
    char *frameName;

    frames = 0;
    mgZeroVector(offset);
    switch (argc) {
        case 3:
            slot = GetStackInt(stack++);
            eohNo = GetStackInt(stack++);
            frameName = GetStackString(stack);
            break;
        case 4:
            slot = GetStackInt(stack++);
            eohNo = GetStackInt(stack++);
            frameName = GetStackString(stack++);
            frames = GetStackInt(stack);
            break;
        case 6:
            slot = GetStackInt(stack++);
            eohNo = GetStackInt(stack++);
            frameName = GetStackString(stack++);
            GetStackVector(offset, stack);
            break;
        case 7:
            slot = GetStackInt(stack++);
            eohNo = GetStackInt(stack++);
            frameName = GetStackString(stack++);
            GetStackVector(offset, stack);

            stack += 3;
            frames = GetStackInt(stack);
            break;
    }
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetEohFramePos(eohNo, frameName, frames, offset);
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_EOH_FRAME_POS__FP12RS_STACKDATAi);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_ADD_POS__FP12RS_STACKDATAi);
/**
 * Queues keeping an object the requested distance in front of the camera.
 */
static int _OBJS_ATTACH_CAMERA(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    float rate;
    int frame;

    slot = GetStackInt(stack++);
    rate = GetStackFloat(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->AttachCamera(rate, frame);
    return 1;
}

/**
 * Queues a wait on an object sequence rotation track.
 */
static int _OBJS_ROT_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->RotDelay(frame);
    return 1;
}

/**
 * Sets the object sequence rotation.
 */
static int _OBJS_SET_ROT(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    sceVu0FVECTOR rot;
    int slot;
    int key;
    int count;
    int i;
    ARG_LIST *node;
    ARG_DATA *args;

    switch (argc) {
        case 1:

            key = GetStackInt(stack);
            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
            return 0;
            }
            slot = GetArgInt(args++);
            GetArgVector(rot, args);
            break;
        case 4:
            slot = GetStackInt(stack++);
            GetStackVector(rot, stack);
            break;
        default:
        return 0;
    }
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetRot(rot);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_ROTATION__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_ROTATION2__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_REFERENCE__FP12RS_STACKDATAi);
/**
 * Queues a wait on an object sequence motion track.
 */
static int _OBJS_MOTION_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->MotionDelay(frame);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_MOTION__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_NEXT_MOTION__FP12RS_STACKDATAi);
/**
 * Queues waiting for an object sequence motion to finish.
 */
static int _OBJS_MOTION_WAIT(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->MotionWait();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_STEP__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_CHENGE_STEP__FP12RS_STACKDATAi);
/**
 * Queues arming an object sequence motion trigger.
 */
static int _OBJS_SEQ_MOT_TRG(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->SetMotionTrg();
    return 1;
}

/**
 * Queues waiting for an object sequence motion trigger.
 */
static int _OBJS_SEQ_MOT_TRG_WAIT(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->MotionTrgWait();
    return 1;
}

/**
 * Resets the object sequence motion.
 */
static int _OBJS_RESET_MOTION(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->ResetMotion();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_MOTION_NOW_TIME__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_MOTION_WAIT_TIME__FP12RS_STACKDATAi);
/**
 * Queues a wait on an object sequence texture animation track.
 */
static int _OBJS_TEXA_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->TexAnimeDelay(frame);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_TEX_ANIME__FP12RS_STACKDATAi);
/**
 * Queues a wait on an object sequence colour track.
 */
static int _OBJS_COLOR_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->ColorDelay(frame);
    return 1;
}

/**
 * Sets the object sequence color.
 */
static int _OBJS_SET_COLOR(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    sceVu0FVECTOR color;
    int slot;
    int frames;

    frames = 0;
    slot = GetStackInt(stack++);
    color[0] = GetStackFloat(stack++);
    color[1] = GetStackFloat(stack++);
    color[2] = GetStackFloat(stack++);
    color[3] = GetStackFloat(stack++);
    if (argc >= 6) {
        frames = GetStackInt(stack);
    }
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SetColor(color, frames);
    return 1;
}

/**
 * Queues a wait on an object sequence scale track.
 */
static int _OBJS_SCALE_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->ScaleDelay(frame);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SET_SCALE__FP12RS_STACKDATAi);
/**
 * Queues a wait on an object sequence sound track.
 */
static int _OBJS_SE_DELAY(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;
    int slot;
    int frame;

    slot = GetStackInt(stack++);
    frame = GetStackInt(stack);
    seq = GetObjSeq(slot);
    if (seq == NULL) {
        return 0;
    }
    seq->SeDelay(frame);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _OBJS_SE_PLAY__FP12RS_STACKDATAi);
/**
 * Resets the object sequence deformation animation position.
 */
static int _OBJS_RESET_DA_POSITION(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->ResetDAPosition();
    return 1;
}

/**
 * Queues returning an event object to its normal behaviour.
 */
static int _OBJS_NORMAL_DRIVE(RS_STACKDATA *stack, int argc) {
    CSceneObjSeq *seq;

    seq = GetObjSeq(GetStackInt(stack));
    if (seq == NULL) {
        return 0;
    }
    seq->NormalDrive();
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _ASQ_CHECK(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Initializes the sound port.
 */
static int _SND_INIT_PORT(RS_STACKDATA *stack, int argc) {
    sndInitPort(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SND_LOAD_SOUND__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_SND_ID__FP12RS_STACKDATAi);
/**
 * Pauses the sound sound effect.
 */
static int _SND_SE_PAUSE(RS_STACKDATA *stack, int argc) {
    u32 seId;

    seId = GetStackInt(stack++);
    sndSePause(seId, GetStackInt(stack));
    return 1;
}

/**
 * Plays the sound sound effect.
 */
static int _SND_SE_PLAY(RS_STACKDATA *stack, int argc) {
    switch (argc) {
        case 2: {
            u32 seId = GetStackInt(stack++);
            int voice = GetStackInt(stack);
            sndSePlay(seId, voice, 0);
            return 1;
        }
        case 3: {
            u32 seId = GetStackInt(stack++);
            int voice = GetStackInt(stack++);
            int volume = GetStackInt(stack);
            sndSePlayV(seId, voice, volume, 0);
            return 1;
        }
        case 4: {
            u32 seId = GetStackInt(stack++);
            int voice = GetStackInt(stack++);
            int volume = GetStackInt(stack++);
            int pan = GetStackInt(stack);
            sndSePlayVP(seId, voice, volume, pan, 0);
            return 1;
        }
        default:
        return 0;
}
}

/**
 * Stops the sound sound effect.
 */
static int _SND_SE_STOP(RS_STACKDATA *stack, int argc) {
    u32 seId;

    seId = GetStackInt(stack++);
    sndSeStop(seId, GetStackInt(stack), 0);
    return 1;
}

/**
 * Sets the sound sound effect volume.
 */
static int _SND_SET_SE_VOL(RS_STACKDATA *stack, int argc) {
    u32 seId;
    int voice;

    seId = GetStackInt(stack++);
    voice = GetStackInt(stack++);
    switch (stack->type) {
        case 0:
            sndSetSeVol(seId, voice, GetStackInt(stack), 0);
            break;
        case 1:
            sndSetSeVolf(seId, voice, GetStackFloat(stack), 0);
            break;
        default:
        return 0;
}
    return 1;
}

/**
 * Sets the sound sound effect pan.
 */
static int _SND_SET_SE_PAN(RS_STACKDATA *stack, int argc) {
    int seId = GetStackInt(stack++);
    int pan = GetStackInt(stack++);
    int time = GetStackInt(stack);
    sndSetSePan(seId, pan, time, 0);
    return 1;
}

/**
 * Sets the sound sound effect pitch.
 */
static int _SND_SET_SE_PITCH(RS_STACKDATA *stack, int argc) {
    int seId = GetStackInt(stack++);
    int pitch = GetStackInt(stack++);
    int time = GetStackInt(stack);
    sndSetSePitch(seId, pitch, time, 0);
    return 1;
}

/**
 * Stops the sound sound effect all.
 */
static int _SND_SE_ALL_STOP(RS_STACKDATA *stack, int argc) {
    sndSeAllStop(GetStackInt(stack));
    return 1;
}

/**
 * Loads the background music.
 */
static int _LOAD_BGM(RS_STACKDATA *stack, int argc) {
    int bgmNo;

    bgmNo = GetStackInt(stack);
    if (EventScene->CheckLoadBGM(bgmNo) == 0) {
        return 0;
}
    return EventScene->LoadBGM(bgmNo, read_buffer);
}

/**
 * Plays the background music.
 */
static int _PLAY_BGM(RS_STACKDATA *stack, int argc) {
    int bgmNo;

    switch (argc) {
        case 1:
            EventScene->PlayBGM(GetStackInt(stack), -1, 1.0f);
        return 1;
        case 2:
            bgmNo = GetStackInt(stack++);
            EventScene->PlayBGM(bgmNo, GetStackInt(stack), 1.0f);
        return 1;
    }
    return 0;
}

/**
 * Stops the background music.
 */
static int _STOP_BGM(RS_STACKDATA *stack, int argc) {
    EventScene->StopBGM(GetStackInt(stack));
    return 1;
}

int CommandStreamOpenFromFPL(int port, char *pack, char *name) {
    char path[64];
    char baseName[64];

    strcpy(path, "cdrom0:\\V\\");
    strncat(path, pack, 3);
    strcat(path, "\\V");
    strcat(path, pack);
    strcat(path, ".VPK;1");
    strcpy(baseName, name);
    strcat(baseName, ".wav");
    CSnd.StreamOpenFromFPLFast(port, path, baseName);
    return 1;
}

int CommandStreamOpen(int port, char *name) {
    char path[64];

    strcpy(path, "cdrom0:\\VOICE\\");
    strcat(path, name);
    strcat(path, ".WV;1");
    CSnd.StreamOpenFast(port, path);
    return 1;
}

int VpkFileNameFromVoiceNo(char *name, int voice_no) {
    int group = voice_no / 10000;
    int kind = 0;

    switch (group) {
        case 1:
            if (voice_no >= 10705) {
                if (voice_no < 10721) {
                    kind = 1;
                }
            }
            if (voice_no >= 10300) {
                if (voice_no < 10701) {
                    kind = 2;
                }
            }
            break;
        case 105:
        case 108:
        case 109:
        case 255:
        case 600:
        case 640:
            break;
    }

    /**
     * A voice group and subdivision mapped to the two numbers in a pack name.
     */
    struct VoicePackEntry {
        int group; /**< Voice group from the upper decimal digits. */
        int kind;  /**< Subdivision within the voice group. */
        int id;    /**< First number in the pack name. */
        int sub;   /**< Second number in the pack name. */
    };

    VoicePackEntry table[164] = {
        {1, 0, 0, 100},
        {1, 1, 0, 106},
        {1, 2, 0, 105},
        {2, 0, 0, 110},
        {3, 0, 0, 115},
        {4, 0, 0, 120},
        {5, 0, 0, 125},
        {6, 0, 100, 100},
        {50, 0, 0, 130},
        {60, 0, 0, 180},
        {61, 0, 0, 185},
        {100, 0, 100, 105},
        {101, 0, 100, 110},
        {102, 0, 100, 115},
        {103, 0, 100, 120},
        {104, 0, 100, 125},
        {105, 0, 0, 135},
        {105, 1, 0, 140},
        {105, 2, 0, 145},
        {106, 0, 0, 150},
        {107, 0, 100, 130},
        {108, 0, 100, 135},
        {108, 1, 100, 136},
        {108, 2, 100, 137},
        {109, 0, 0, 155},
        {109, 1, 0, 160},
        {110, 0, 100, 140},
        {111, 0, 100, 145},
        {201, 0, 100, 150},
        {202, 0, 100, 155},
        {203, 0, 300, 100},
        {204, 0, 10, 100},
        {205, 0, 10, 105},
        {206, 0, 10, 110},
        {207, 0, 10, 115},
        {208, 0, 10, 120},
        {209, 0, 10, 125},
        {215, 0, 110, 100},
        {220, 0, 110, 105},
        {230, 0, 110, 110},
        {235, 0, 110, 115},
        {240, 0, 110, 120},
        {245, 0, 110, 125},
        {247, 0, 10, 130},
        {250, 0, 10, 135},
        {253, 0, 10, 140},
        {255, 0, 110, 130},
        {255, 1, 110, 135},
        {256, 0, 110, 140},
        {257, 0, 110, 145},
        {260, 0, 110, 150},
        {270, 0, 10, 145},
        {272, 0, 10, 150},
        {275, 0, 10, 155},
        {277, 0, 10, 160},
        {280, 0, 10, 165},
        {283, 0, 10, 170},
        {285, 0, 10, 180},
        {290, 0, 110, 155},
        {300, 0, 15, 100},
        {302, 0, 15, 105},
        {304, 0, 15, 110},
        {305, 0, 0, 165},
        {306, 0, 15, 115},
        {307, 0, 115, 100},
        {308, 0, 115, 105},
        {312, 0, 115, 110},
        {314, 0, 115, 115},
        {315, 0, 115, 120},
        {316, 0, 115, 125},
        {317, 0, 115, 130},
        {320, 0, 15, 120},
        {324, 0, 15, 125},
        {328, 0, 15, 130},
        {330, 0, 15, 135},
        {332, 0, 115, 135},
        {336, 0, 115, 140},
        {340, 0, 115, 145},
        {344, 0, 0, 170},
        {348, 0, 115, 150},
        {352, 0, 115, 155},
        {356, 0, 115, 160},
        {360, 0, 15, 140},
        {364, 0, 15, 145},
        {368, 0, 15, 150},
        {372, 0, 15, 155},
        {376, 0, 15, 160},
        {400, 0, 20, 100},
        {402, 0, 20, 105},
        {404, 0, 20, 110},
        {406, 0, 120, 100},
        {408, 0, 120, 105},
        {410, 0, 120, 110},
        {412, 0, 20, 115},
        {414, 0, 20, 120},
        {416, 0, 120, 115},
        {420, 0, 120, 120},
        {424, 0, 0, 175},
        {428, 0, 20, 125},
        {432, 0, 120, 125},
        {434, 0, 120, 130},
        {436, 0, 120, 135},
        {438, 0, 120, 140},
        {440, 0, 120, 145},
        {444, 0, 120, 150},
        {448, 0, 20, 130},
        {452, 0, 20, 135},
        {456, 0, 20, 140},
        {460, 0, 20, 145},
        {464, 0, 20, 150},
        {468, 0, 20, 155},
        {500, 0, 25, 100},
        {504, 0, 25, 105},
        {508, 0, 25, 110},
        {512, 0, 125, 100},
        {516, 0, 125, 105},
        {520, 0, 125, 110},
        {522, 0, 125, 115},
        {523, 0, 125, 120},
        {524, 0, 25, 115},
        {526, 0, 125, 125},
        {528, 0, 125, 130},
        {532, 0, 125, 135},
        {536, 0, 125, 140},
        {540, 0, 25, 120},
        {548, 0, 25, 125},
        {552, 0, 25, 130},
        {556, 0, 25, 135},
        {560, 0, 25, 140},
        {600, 0, 20, 160},
        {600, 1, 20, 161},
        {604, 0, 30, 100},
        {608, 0, 130, 100},
        {610, 0, 130, 105},
        {612, 0, 130, 110},
        {616, 0, 40, 100},
        {624, 0, 110, 160},
        {628, 0, 115, 165},
        {632, 0, 120, 155},
        {636, 0, 125, 145},
        {640, 0, 40, 105},
        {640, 1, 40, 106},
        {640, 2, 40, 107},
        {648, 0, 40, 110},
        {700, 0, 30, 105},
        {702, 0, 130, 115},
        {704, 0, 30, 110},
        {708, 0, 130, 120},
        {712, 0, 130, 125},
        {716, 0, 130, 130},
        {720, 0, 130, 135},
        {724, 0, 130, 140},
        {728, 0, 130, 145},
        {732, 0, 130, 150},
        {736, 0, 130, 160},
        {744, 0, 30, 115},
        {748, 0, 30, 120},
        {800, 0, 0, 190},
        {804, 0, 0, 195},
        {808, 0, 150, 100},
        {812, 0, 150, 105},
        {816, 0, 150, 110},
        {820, 0, 150, 115},
        {824, 0, 50, 100},
    };
    for (int i = 0; i < vpk_entry_count; i++) {
        if (group == table[i].group && kind == table[i].kind) {
            sprintf(name, "%03d_%03d", table[i].id, table[i].sub);
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _STREAM_OPEN__FP12RS_STACKDATAi);
int CommandStreamPlay(int port, int volume) {
    int reverb = sndGetReverbDepth(1);
    int scaled = (int)((double)volume - 256.0 * (1.5 * (double)reverb));
    printf("vol=%d,vol_0=%d\n", scaled, volume);
    EdEventInfo.stream_volume = volume;
    CSnd.StreamSetVol(port, scaled, scaled);
    CSnd.StreamPlay(port);
    return 1;
}

/**
 * Plays the stream.
 */
static int _STREAM_PLAY(RS_STACKDATA *stack, int argc) {
    EdEventInfo.stream_playing = 1;
    GetStackInt(stack++);
    switch (argc) {
        case 1:
        return CommandStreamPlay(event_stream, stream_max_volume);
        case 2:
        return CommandStreamPlay(event_stream, GetStackInt(stack));
    }
    return 0;
}

/**
 * Stops the stream.
 */
static int _STREAM_STOP(RS_STACKDATA *stack, int argc) {
    EdEventInfo.stream_playing = 0;
    GetStackInt(stack);
    if (EdEventInfo.stream_from_fpl == 1) {
        CSnd.StreamEND(event_stream);
    } else {
        CSnd.StreamSetVol(event_stream, EdEventInfo.stream_volume, EdEventInfo.stream_volume);
        CSnd.StreamClose(event_stream);
    }
    return 1;
}

/**
 * Readies the event voice stream and starts buffering it.
 */
static int _STREAM_STANDBY(RS_STACKDATA *stack, int argc) {
    GetStackInt(stack);
    CSnd.StreamStandBy(event_stream);
    return 1;
}

/**
 * Returns the stream status.
 */
static int _STREAM_GET_STATUS(RS_STACKDATA *stack, int argc) {
    int result;
    GetStackInt(stack++);

    result = CSnd.StreamGetState(event_stream);
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the sys sound id.
 */
static int _GET_SYS_SND_ID(RS_STACKDATA *stack, int argc) {
    int result;

    result = GetSystemSndID();
    SetStack(stack, result);
    return 1;
}

/**
 * Opens the stream check.
 */
static int _STREAM_OPEN_CHECK(RS_STACKDATA *stack, int argc) {
    SetStack(stack, CSnd.StreamOpenState());
    return 1;
}

/**
 * Loads the sound effect environment.
 */
static int _LOAD_SE_ENV(RS_STACKDATA *stack, int argc) {
    int bankNo;

    bankNo = GetStackInt(stack);
    if (EventScene->CheckLoadSeEnv(bankNo) == 0) {
        return 0;
    }
    EventScene->LoadSeEnv(bankNo, read_buffer);
    return 1;
}

/**
 * Plays the environment background music.
 */
static int _PLAY_ENV_BGM(RS_STACKDATA *stack, int argc) {
    int voice;
    int volume;

    voice = GetStackInt(stack++);
    EventScene->StopEnvBGM();
    if (voice == -1) {
        EventScene->AutoChangeEnvBGM(1);
        EventScene->SetEnvBGMVol(1.0f);
        return 1;
    }
    switch (argc) {
        case 1:
            EventScene->SetEnvBGMVol(1.0f);
            EventScene->PlayEnvBGM(voice, 1.0f);
            EdEventInfo.env_bgm_no = voice;
            EdEventInfo.env_bgm_volume = 1.0f;
        return 1;
        case 2:

            volume = (int)GetStackFloat(stack);
            EventScene->SetEnvBGMVol(volume);
            EventScene->PlayEnvBGM(voice, 1.0f);
            EdEventInfo.env_bgm_no = voice;
            EdEventInfo.env_bgm_volume = volume;
        return 1;
    }
    return 0;
}

/**
 * Plays the sys sound effect.
 */
static int _SYS_SE_PLAY(RS_STACKDATA *stack, int argc) {
    int seId;

    seId = GetStackInt(stack);
    if (seId < 0) {
        return 0;
    }
    sndSePlay(SystemSND_ID, seId, 0);
    return 1;
}

/**
 * Initializes the sound effect src.
 */
static int _INIT_SE_SRC(RS_STACKDATA *stack, int argc) {
    EventScene->InitSeSrc();
    return 1;
}

/**
 * Initializes the sound effect environment.
 */
static int _INIT_SE_ENV(RS_STACKDATA *stack, int argc) {
    EventScene->InitSeEnv();
    return 1;
}

/**
 * Initializes the sound effect bas.
 */
static int _INIT_SE_BAS(RS_STACKDATA *stack, int argc) {
    EventScene->InitSeBas();
    return 1;
}

/**
 * Loads the sound effect src.
 */
static int _LOAD_SE_SRC(RS_STACKDATA *stack, int argc) {
    int srcNo;

    srcNo = GetStackInt(stack);
    if (EventScene->CheckLoadSeSrc(srcNo) == 0) {
        return 0;
    }
    return EventScene->LoadSeSrc(srcNo, read_buffer);
}

/**
 * Reports failure without changing event state.
 */
static s32 _LOAD_SE_FOOT(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Reports failure without changing event state.
 */
static s32 _LOAD_SE_DOOR(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Reports failure without changing event state.
 */
static s32 _LOAD_SE_BOX(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Loads the sound effect battle.
 */
static int _LOAD_SE_BATTLE(RS_STACKDATA *stack, int argc) {
    int bankNo;

    bankNo = GetStackInt(stack);
    if (EventScene->CheckLoadSeBattle(bankNo) == 0) {
        return 0;
    }
    return EventScene->LoadSeBattle(bankNo, read_buffer);
}

/**
 * Deletes the sound port.
 */
static int _SND_DELETE_PORT(RS_STACKDATA *stack, int argc) {
    sndDeletePort(GetStackInt(stack));
    return 1;
}

/**
 * Fades in scene music over a duration converted from 60 Hz to 50 Hz frames.
 */
static int _FADE_IN_BGM(RS_STACKDATA *stack, int argc) {
    int frames60;
    int frames;

    frames60 = GetStackInt(stack);
    frames = frames60 * 50 / 60;
    if (frames <= 0) {
        frames = 1;
    }
    EventScene->FadeInBGM(frames);
    return 1;
}

/**
 * Fades out scene music over a duration converted from 60 Hz to 50 Hz frames.
 */
static int _FADE_OUT_BGM(RS_STACKDATA *stack, int argc) {
    int frames60;
    int frames;

    frames60 = GetStackInt(stack);
    frames = frames60 * 50 / 60;
    if (frames <= 0) {
        frames = 1;
    }
    EventScene->FadeOutBGM(frames);
    return 1;
}

/**
 * Stops the environment background music.
 */
static int _STOP_ENV_BGM(RS_STACKDATA *stack, int argc) {
    EventScene->StopEnvBGM();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_BGM_VOL__FP12RS_STACKDATAi);
/**
 * Reports failure without changing event state.
 */
static s32 _SND_SET_REVERB(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Sets the sound environment volume.
 */
static int _SND_SET_ENV_VOL(RS_STACKDATA *stack, int argc) {
    EventScene->SetEnvBGMVol(GetStackFloat(stack));
    return 1;
}

/**
 * Checks the stream silent.
 */
static int _STREAM_SILENT_CHECK(RS_STACKDATA *stack, int argc) {
    int level;
    int loud;

    GetStackInt(stack++);
    level = CSnd.StreamGetLevel( event_stream);
    loud = 1;

    if ((s16)(u16)level < 11 && (s16)(u16)level >= -10 && (s16)(level >> 16) < 11 &&
        (s16)(level >> 16) >= -10) {
        loud = 0;
    }
    SetStack(stack, loud);
    return 1;
}

/**
 * Changes the auto environment.
 */
static int _AUTO_CHANGE_ENV(RS_STACKDATA *stack, int argc) {
    EventScene->AutoChangeEnvBGM(GetStackInt(stack));
    return 1;
}

/**
 * Loads the background music cancel.
 */
static int _BGM_LOAD_CANCEL(RS_STACKDATA *stack, int argc) {
    EventScene->skip_load_bgm = 1;
    return 1;
}

/**
 * Loads the sound cancel.
 */
static int _SOUND_LOAD_CANCEL(RS_STACKDATA *stack, int argc) {
    EventScene->skip_load_sound = 1;
    return 1;
}

/**
 * Loads the background music enable.
 */
static int _BGM_LOAD_ENABLE(RS_STACKDATA *stack, int argc) {
    EventScene->skip_load_bgm = 0;
    return 1;
}

/**
 * Loads the sound enable.
 */
static int _SOUND_LOAD_ENABLE(RS_STACKDATA *stack, int argc) {
    EventScene->skip_load_sound = 0;
    return 1;
}

/**
 * Loads the sound effect base.
 */
static int _LOAD_SE_BASE(RS_STACKDATA *stack, int argc) {
    int bankNo;

    bankNo = GetStackInt(stack);
    if (EventScene->CheckLoadSeBase(bankNo) == 0) {
        return 0;
    }
    return EventScene->LoadSeBase(bankNo, read_buffer);
}

/**
 * Loads the sound.
 */
static int _LOAD_SOUND(RS_STACKDATA *stack, int argc) {
    EventScene->LoadSound(GetStackInt(stack), read_buffer);
    return 1;
}

/**
 * Closes the stream.
 */
static int _STREAM_CLOSE(RS_STACKDATA *stack, int argc) {
    EdEventInfo.stream_playing = 0;
    GetStackInt(stack);
    CSnd.StreamSetVol(event_stream, EdEventInfo.stream_volume, EdEventInfo.stream_volume);
    CSnd.StreamClose(event_stream);
    EdEventInfo.stream_reading = 0;
    return 1;
}

int CommandStreamOpen2(int port, char *name) {
    char path[64];
    strcpy(path, "cdrom0:\\VOICE\\");
    strcat(path, name);
    strcat(path, ".WV;1");
    return 1;
}

/**
 * Builds the voice-stream path and marks the event as reading the stream.
 */
static int _STREAM_OPEN2(RS_STACKDATA *stack, int argc) {
    int result;
    GetStackInt(stack++);
    EdEventInfo.stream_from_fpl = 0;
    result = CommandStreamOpen2(event_stream, GetStackString(stack));
    if (result == 1) {
        EdEventInfo.stream_reading = 1;
    }
    return result;
}

/**
 * Loads the background music pack.
 */
static int _LOAD_BGM_PACK(RS_STACKDATA *stack, int argc) {
    char fileName[64];
    int bgmNo;
    u32 *pack;

    bgmNo = GetStackInt(stack);
    if (EventScene->CheckLoadBGM(bgmNo) == 0) {
        return 0;
    }
    EventScene->GetBgmFile(fileName, bgmNo);
    pack = GetLoadBGBuff(fileName, NULL);
    if (pack != NULL) {
        return EventScene->LoadBGMPack(bgmNo, pack);
    }
    return 0;
}

/**
 * Returns the background music number.
 */
static int _GET_BGM_NO(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->GetActiveBgmInfo()->load_no);
    return 1;
}

/**
 * Returns the master volume.
 */
static int _GET_MASTER_VOL(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->GetActiveBgmInfo()->unk_c);
    return 1;
}

/**
 * Sets the master volume.
 */
static int _SET_MASTER_VOL(RS_STACKDATA *stack, int argc) {
    float volume;
    CScene *scene;

    volume = GetStackFloat(stack);
    scene = EventScene;
    scene->GetActiveBgmInfo()->unk_c = volume;
    scene->SetVolfBGM(scene->GetActiveBgmInfo()->volf);
    return 1;
}

/**
 * Returns the btl background music volume.
 */
static int _GET_BTL_BGM_VOL(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *battleBgm;

    battleBgm = &EventScene->battle_area;
    if (battleBgm == NULL) {
        return 0;
    }
    SetStack(stack, battleBgm->battle_bgm_vol);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_BTL_BGM_VOL__FP12RS_STACKDATAi);
/**
 * Enables or disables reverberation of the sound processor inputs.
 */
static int _SND_IN_REVERB(RS_STACKDATA *stack, int argc) {
    CSnd.SndInReverb(GetStackInt(stack) != 0);
    return 1;
}

/**
 * Stops the requested map-object sound effects.
 */
static int _SND_STOP_SRC(RS_STACKDATA *stack, int argc) {
    EventScene->StopSeSrc();
    return 1;
}

/**
 * Pauses the sound background music.
 */
static int _SND_PAUSE_BGM(RS_STACKDATA *stack, int argc) {
    EventScene->PauseBGM();
    return 1;
}

/**
 * Opens a numbered WAV file on the event voice stream and marks it as reading.
 */
static int _STREAM_OPEN3(RS_STACKDATA *stack, int argc) {
    char name[64];

    GetStackInt(stack++);
    EdEventInfo.stream_from_fpl = 2;
    sprintf(name, "%07d.wav", GetStackInt(stack));
    CSnd.StreamOpenFast(event_stream, name);
    EdEventInfo.stream_reading = 1;
    return 1;
}

/**
 * Returns the active background music status.
 */
static int _GET_ACTIVE_BGM_STATUS(RS_STACKDATA *stack, int argc) {
    EventScene->GetActiveBgmStatus(&EdEventInfo.bgm_status);
    return 1;
}

/**
 * Sets the active background music status.
 */
static int _SET_ACTIVE_BGM_STATUS(RS_STACKDATA *stack, int argc) {
    EventScene->SetActiveBgmStatus(&EdEventInfo.bgm_status);
    return 1;
}

/**
 * Returns the background music status now number.
 */
static int _GET_BGM_STATUS_NOW_NO(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, EdEventInfo.bgm_status.load_no);
    return 1;
}

/**
 * Returns the sound effect status.
 */
static int _GET_SE_STATUS(RS_STACKDATA *stack, int argc) {
    int result;
    if (argc != 3) {
        return 0;
    }
    int seId = GetStackInt(stack++);
    int voice = GetStackInt(stack++);

    result = sndGetSeStatus(seId, voice);
    SetStack(stack, result);
    return 1;
}

/**
 * Stops the sound effect all.
 */
static int _SE_ALL_STOP(RS_STACKDATA *stack, int argc) {
    EventScene->SeAllStop();
    return 1;
}

/**
 * Stops the sound all.
 */
static int _SOUND_ALL_STOP(RS_STACKDATA *stack, int argc) {
    EventScene->SoundAllStop();
    return 1;
}

/**
 * Disables background music playback.
 */
static int _BGM_PLAY_CANCEL(RS_STACKDATA *stack, int argc) {

    EventScene->skip_play_bgm = 1;
    return 1;
}

/**
 * Enables background music playback.
 */
static int _BGM_PLAY_ENABLE(RS_STACKDATA *stack, int argc) {
    EventScene->skip_play_bgm = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_DEF_BGM_NO__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MOVIE_CC__FP12RS_STACKDATAi);
/**
 * Registers a villager using the scene memory stack selected by the script.
 */
static int _REGISTER_VILLAGER2(RS_STACKDATA *stack, int argc) {
    int villagerNo;
    int mode;
    mgCMemory *memory;

    villagerNo = GetStackInt(stack++);
    mode = GetStackInt(stack++);
    memory = EventScene->GetStack(GetStackInt(stack));
    if (memory == NULL) {
        return 0;
    }
    EventScene->RegisterVillager(villagerNo, mode, memory);
    return 1;
}

/**
 * Sets the fishingtournament etc.
 */
static int _SET_FISHINGTOURNAMENT_ETC(RS_STACKDATA *stack, int argc) {
    CFishingTournament *tournament;
    RS_STACKDATA *next = stack + 1;

    switch (GetStackInt(stack)) {
        case 0:
            tournament = GetFishTournament();
            if (tournament == NULL) {
            return 0;
            }
            tournament->SetRank(GetStackInt(next));
            break;
        case 1:
            tournament = GetFishTournament();
            if (tournament == NULL) {
            return 0;
            }
            tournament->ResetRecord();
            break;
        case 2:
            InitFishPrize();
            LoadFishPrize(1);
            break;
        case 3:
            TuriTourCount();
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Assigns a scene character to an event object handle.
 */
static int _EOH_SYNC_CHARA(RS_STACKDATA *stack, int argc) {
    CCharacter2 *chara;
    int slot;
    int charaNo;

    slot = GetStackInt(stack++);
    charaNo = GetStackInt(stack);
    chara = GetChara(charaNo);
    if (chara != NULL) {
        return EventObjHandleMother.Set(slot, EOH_TYPE_CHARA, charaNo, chara);
    }
    return 0;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SYNC_OBJ__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SYNC_EDIT_OBJ__FP12RS_STACKDATAi);
/**
 * Assigns an event sprite to an event object handle.
 */
static int _EOH_SYNC_SPRITE(RS_STACKDATA *stack, int argc) {
    CEventSprite2 *sprite;
    int slot;

    slot = GetStackInt(stack++);
    sprite = GetEventSprite(GetStackInt(stack));
    if (sprite != NULL) {
        return EventObjHandleMother.Set(slot, EOH_TYPE_SPRITE, sprite);
    }
    return 0;
}

/**
 * Sets the object handle position.
 */
static int _EOH_SET_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    int slot;
    int key;
    int count;
    int i;
    ARG_LIST *node;
    ARG_DATA *args;

    switch (argc) {
        case 1:

            key = GetStackInt(stack);
            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
            return 0;
            }
            slot = GetArgInt(args++);
            GetArgVector(pos, args);
            break;
        case 4:
            slot = GetStackInt(stack++);
            GetStackVector(pos, stack);
            break;
        default:
        return 0;
    }
    return EventObjHandleMother.SetPos(slot, pos[0], pos[1], pos[2]);
}

/**
 * Sets the object handle rotation.
 */
static int _EOH_SET_ROT(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR rot;
    int slot;
    int key;
    int count;
    int i;
    ARG_LIST *node;
    ARG_DATA *args;

    switch (argc) {
        case 1:

            key = GetStackInt(stack);
            node = EventScriptArg.list;
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            count = EventScriptArg.list_num;
            i = 0;
            goto test;
        body:
            if (key == node->id) {
                goto done;
            }
            node = node->next;
            if (node != NULL) {
                i++;
                goto test;
            }
            args = NULL;
            goto haveArgs;
        test:
            if (i < count) {
                goto body;
            }
        done:
            if (node == NULL) {
                args = NULL;
                goto haveArgs;
            }
            args = node->args;
        haveArgs:
            if (args == NULL) {
            return 0;
            }
            slot = GetArgInt(args++);
            GetArgVector(rot, args);
            break;
        case 4:
            slot = GetStackInt(stack++);
            GetStackVector(rot, stack);
            break;
        default:
        return 0;
    }
    rot[0] = mgAngleLimit(rot[0]);
    rot[1] = mgAngleLimit(rot[1]);
    rot[2] = mgAngleLimit(rot[2]);
    return EventObjHandleMother.SetRot(slot, rot[0], rot[1], rot[2]);
}

/**
 * Returns the object handle position.
 */
static void _EOH_GET_POS(RS_STACKDATA *stack, int argc) {
    float pos[3];

    if (EventObjHandleMother.GetPos(GetStackInt(stack++), pos) != 0) {
        SetStack(stack++, pos[0]);
        SetStack(stack++, pos[1]);
        SetStack(stack, pos[2]);
    }
}

/**
 * Returns the object handle rotation.
 */
static void _EOH_GET_ROT(RS_STACKDATA *stack, int argc) {
    float rot[3];

    if (EventObjHandleMother.GetRot(GetStackInt(stack++), rot) != 0) {
        SetStack(stack++, rot[0]);
        SetStack(stack++, rot[1]);
        SetStack(stack, rot[2]);
    }
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SET_MOTION__FP12RS_STACKDATAi);
/**
 * Sets the object handle step.
 */
static void _EOH_SET_STEP(RS_STACKDATA *stack, int argc) {
    int slot = GetStackInt(stack++);
    EventObjHandleMother.SetStep(slot, GetStackFloat(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SET_TEX_ANIM__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SET_SCALE__FP12RS_STACKDATAi);
/**
 * Sets the object handle show.
 */
static void _EOH_SET_SHOW(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetShow(slot, GetStackInt(stack));
}

/**
 * Returns the object handle show.
 */
static void _EOH_GET_SHOW(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *nextSlot;

    nextSlot = stack + 1;

    int show;
    if (EventObjHandleMother.GetShow(GetStackInt(stack), &show) != 0) {
        SetStack(nextSlot, show);
    }
}

/**
 * Sets the object handle frame show.
 */
static void _EOH_SET_FRAME_SHOW(RS_STACKDATA *stack, int argc) {
    int slot;
    char *frameName;

    slot = GetStackInt(stack++);
    frameName = GetStackString(stack++);
    EventObjHandleMother.SetFrameShow(slot, frameName, GetStackInt(stack) != 0 ? 1 : 0);
}

/**
 * Sets the object handle shadow.
 */
static void _EOH_SET_SHADOW(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetShadow(slot, GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SET_TRANSLATE__FP12RS_STACKDATAi);
/**
 * Sets the object handle foot sound id.
 */
static void _EOH_SET_FOOT_SOUND_ID(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetFootSoundID(slot, GetStackInt(stack));
}

/**
 * Sets the object handle frame status.
 */
static void _EOH_SET_FRAME_STATUS(RS_STACKDATA *stack, int argc) {
    int slot;
    char *frameName;

    slot = GetStackInt(stack++);
    frameName = GetStackString(stack++);
    EventObjHandleMother.SetFrameShow(slot, frameName, GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_GET_FRAME_POS__FP12RS_STACKDATAi);
/**
 * Sets the object handle sound id.
 */
static void _EOH_SET_SOUND_ID(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetSoundID(slot, GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_GET_FRAME_STATUS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SYNC_CHROBJ__FP12RS_STACKDATAi);
/**
 * Sets the object handle fade flag.
 */
static void _EOH_SET_FADE_FLAG(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetFadeFlag(slot, GetStackInt(stack));
}

/**
 * Resets the object handle deformation animation position.
 */
static void _EOH_RESET_DA_POSITION(RS_STACKDATA *stack, int argc) {
    EventObjHandleMother.ResetDAPosition(GetStackInt(stack));
}

/**
 * Sets the object handle shadow frame status.
 */
static void _EOH_SET_SHADOW_FRAME_STATUS(RS_STACKDATA *stack, int argc) {
    int slot;
    char *frameName;

    slot = GetStackInt(stack++);
    frameName = GetStackString(stack++);
    EventObjHandleMother.SetShadowFrameShow(slot, frameName, GetStackInt(stack));
}

/**
 * Assigns the Geostone object to an event object handle.
 */
static void _EOH_SYNC_GEOSTONE(RS_STACKDATA *stack, int argc) {
    EventObjHandleMother.Set(GetStackInt(stack), EOH_TYPE_CHARA, -1, (CCharacter2 *)&GeoStone);
}

/**
 * Searches for the object handle sync character.
 */
static int _EOH_SYNC_SEARCH_CHARA(RS_STACKDATA *stack, int argc) {
    int slot;
    char *name;
    CActionChara *player;

    slot = GetStackInt(stack++);
    GetStackInt(stack++);
    name = GetStackString(stack);
    player = (CActionChara *)GetCharacter(0);
    if (player == NULL) {
        return 0;
    }
    return EventObjHandleMother.Set(slot, 0, -1, (CCharacter2 *)player->SearchChara(name));
}

/**
 * Returns an event object to its normal behaviour.
 */
static void _EOH_NORMAL_DRIVE(RS_STACKDATA *stack, int argc) {
    EventObjHandleMother.NormalDrive(GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SET_FRAME_ALPHA__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SYNC_FUNCP__FP12RS_STACKDATAi);
/**
 * Sets the object handle foot sound effect id.
 */
static void _EOH_SET_FOOT_SE_ID(RS_STACKDATA *stack, int argc) {
    int slot;

    slot = GetStackInt(stack++);
    EventObjHandleMother.SetFootSeId(slot, GetStackInt(stack));
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _EOH_SYNC_DOOR_PARTS__FP12RS_STACKDATAi);
/**
 * Initializes the sphida.
 */
static int _SPHIDA_INIT(RS_STACKDATA *stack, int argc) {
    InitSphida();
    return 1;
}

/**
 * Sets the sphida up.
 */
static int _SPHIDA_SET_UP(RS_STACKDATA *stack, int argc) {
    CSphida *sphida;
    mgCMemory *memory;
    int param;
    int kind;
    RS_STACKDATA *ptr;
    int slot;

    ptr = stack;
    kind = 0;
    slot = GetStackInt(ptr++);
    param = GetStackInt(ptr++);
    if (argc >= 3) {
        kind = GetStackInt(ptr++);
    }
    memory = EventScene->GetStack(slot);
    if (memory == NULL) {
        return 0;
    }
    sphida = new (memory->Alloc((sizeof(CSphida) + 15) / 16 + 2)) CSphida;
    Sphida = sphida;
    if (sphida == NULL) {
        return 0;
    }
    sphida->Initialize();
    switch (kind) {
        case 0:
            Sphida->SetUp(param);
            break;
        case 1:
            Sphida->s17_SetUp(param);
            break;
        case 2:
            Sphida->Omake_SetUp( GetStackInt(ptr), param);
            break;
    }
    return 1;
}

/**
 * Sets the sphida play flag.
 */
static int _SPHIDA_SET_PLAY_FLAG(RS_STACKDATA *stack, int argc) {
    int flag;

    flag = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->play_flag = flag;
    return 1;
}

/**
 * Sets the sphida minimap flag.
 */
static int _SPHIDA_SET_MINIMAP_FLAG(RS_STACKDATA *stack, int argc) {
    int flag;

    flag = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->minimap_flag = flag;
    return 1;
}

/**
 * Sets the sphida mm line flag.
 */
static int _SPHIDA_SET_MM_LINE_FLAG(RS_STACKDATA *stack, int argc) {
    int flag;

    flag = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->mm_line_flag = flag;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_SET_MM_LINE_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_SET_PIN_POS__FP12RS_STACKDATAi);
/**
 * Returns the sphida pin position.
 */
static int _SPHIDA_GET_PIN_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    if (Sphida == NULL) {
        return 0;
    }
    sceVu0CopyVector(pos, Sphida->pin_pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_SET_BALL_POS__FP12RS_STACKDATAi);
/**
 * Returns the sphida ball position.
 */
static int _SPHIDA_GET_BALL_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    if (Sphida == NULL) {
        return 0;
    }
    sceVu0CopyVector(pos, Sphida->ball_pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Sets the sphida pin col.
 */
static int _SPHIDA_SET_PIN_COL(RS_STACKDATA *stack, int argc) {
    int color;

    color = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->pin_col = color;
    return 1;
}

/**
 * Returns the sphida pin col.
 */
static int _SPHIDA_GET_PIN_COL(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->pin_col);
    return 1;
}

/**
 * Sets the sphida ball col.
 */
static int _SPHIDA_SET_BALL_COL(RS_STACKDATA *stack, int argc) {
    int color;

    color = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->ball_col = color;
    return 1;
}

/**
 * Returns the sphida ball col.
 */
static int _SPHIDA_GET_BALL_COL(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->ball_col);
    return 1;
}

/**
 * Sets the sphida par count.
 */
static int _SPHIDA_SET_PAR_COUNT(RS_STACKDATA *stack, int argc) {
    int parCount;

    parCount = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    if (DebugFlag == 0) {
        Sphida->par_count = parCount;
    }
    return 1;
}

/**
 * Returns the sphida par count.
 */
static int _SPHIDA_GET_PAR_COUNT(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    int count;
    if (DebugFlag != 0) {
        count = Sphida->par_count + 1;
    } else {
        count = Sphida->par_count;
    }
    SetStack(stack, count);
    return 1;
}

/**
 * Returns the sphida mini level.
 */
static int _SPHIDA_GET_MINI_LEVEL(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->mini_level);
    return 1;
}

/**
 * Returns the sphida texture block.
 */
static int _SPHIDA_GET_TEXB(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->tex_bank);
    return 1;
}

/**
 * Sets the sphida status flag.
 */
static int _SPHIDA_SET_STATUS_FLAG(RS_STACKDATA *stack, int argc) {
    int flag;

    flag = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->status_flag = flag;
    return 1;
}

/**
 * Resets the sphida powgage.
 */
static int _SPHIDA_RESET_POWGAGE(RS_STACKDATA *stack, int argc) {
    if (Sphida == 0) {
        return 0;
    }

    CSphida *sphida = Sphida;
    sphida->pow_gage.state = -1;
    sphida->pow_gage.reverse = 0;
    sphida->pow_gage.count = 0;
    sphida->pow_gage.power = 0;
    sphida->pow_gage.code = -10;
    return 1;
}

/**
 * Starts the sphida powgage.
 */
static int _SPHIDA_START_POWGAGE(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->pow_gage.state = 0;
    return 1;
}

/**
 * Advances the Sphida power gauge from state 1 to 2 or from state 3 to 4.
 */
static int _SPHIDA_TRIGGER_POWGAGE(RS_STACKDATA *stack, int argc) {

    CSphida *sphida = Sphida;
    if (sphida == NULL) {
        return 0;
    }
    switch (sphida->pow_gage.state) {
        case 1:
            sphida->pow_gage.state = 2;
            break;
        case 3:
            sphida->pow_gage.state = 4;
            break;
    }
    return 1;
}

/**
 * Returns the sphida shot pow.
 */
static int _SPHIDA_GET_SHOT_POW(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->pow_gage.power);
    return 1;
}

/**
 * Returns the sphida powgage code.
 */
static int _SPHIDA_GET_POWGAGE_CODE(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    SetStack(stack, Sphida->pow_gage.code);
    return 1;
}

/**
 * Sets the sphida powgage safe level.
 */
static int _SPHIDA_SET_POWGAGE_SAFE_LEVEL(RS_STACKDATA *stack, int argc) {
    int level;

    level = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    if (level <= 0) {
        level = 1;
    }
    if (level > 6) {
        level = 6;
    }
    Sphida->pow_gage.safe_level = level;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_GET_CULB_DEF__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_SET_SPIN_MARK_POS__FP12RS_STACKDATAi);
/**
 * Sets the sphida culb number.
 */
static int _SPHIDA_SET_CULB_NO(RS_STACKDATA *stack, int argc) {
    int clubNo;

    clubNo = GetStackInt(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->club_no = clubNo;
    return 1;
}

/**
 * Stores the script carry value in the Sphida state.
 */
static int _SPHIDA_CALC_CARRY(RS_STACKDATA *stack, int argc) {
    float carry = GetStackFloat(stack);
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->carry = carry;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_GET_PG_CURSOR_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SPHIDA_SET_COL_MODEL__FP12RS_STACKDATAi);
/**
 * Returns the sphida prize.
 */
static int _SPHIDA_GET_PRIZE(RS_STACKDATA *stack, int argc) {
    int prize1;
    int prize2;
    CDngFloorManager *floorManager;
    RS_STACKDATA *nextSlot;
    DNG_BATTLE_AREA *dngScene;

    dngScene = &(EventScene)->battle_area;
    floorManager = &dngScene->floor_manager;
    if (dngScene == 0) {
        return 0;
    }
    nextSlot = stack + 1;
    if (floorManager == NULL) {
        return 0;
    }
    floorManager->GetSphedaPrize(GetStackInt(stack), &prize1, &prize2);
    SetStack(nextSlot++, prize1);
    SetStack(nextSlot, prize2);
    return 1;
}

/**
 * Sets the sphida last challenge.
 */
static int _SPHIDA_SET_LAST_CHALLENGE(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL) {
        return 0;
    }
    Sphida->last_challenge = GetStackInt(stack);
    return 1;
}

/**
 * Returns the sphida last challenge.
 */
static int _SPHIDA_GET_LAST_CHALLENGE(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL || argc != 1) {
        return 0;
    }
    SetStack(stack, Sphida->last_challenge);
    return 1;
}

/**
 * Returns the sphida extra mode.
 */
static int _SPHIDA_GET_OMAKE_MODE(RS_STACKDATA *stack, int argc) {
    if (Sphida == NULL || argc != 1) {
        return 0;
    }
    SetStack(stack, Sphida->omake_mode);
    return 1;
}

/**
 * Sets the sphida now hole.
 */
static int _SPHIDA_SET_NOW_HOLE(RS_STACKDATA *stack, int argc) {
    CSubGameData *subGame = GetSubGameSaveData();
    if (subGame == NULL) {
        return 0;
    }
    CSphidaData *sphidaData;
    if ((sphidaData = subGame->GetSphidaData()) == NULL) {
        return 0;
    }
    sphidaData->SetHorl(GetStackInt(stack));
    return 1;
}

/**
 * Returns the sphida now hole.
 */
static int _SPHIDA_GET_NOW_HOLE(RS_STACKDATA *stack, int argc) {
    CSphidaData *sphidaData;
    CSubGameData *subGame;

    if (argc != 1) {
        return 0;
    }
    subGame = GetSubGameSaveData();
    if (subGame == NULL) {
        return 0;
    }
    sphidaData = subGame->GetSphidaData();
    if (sphidaData == NULL) {
        return 0;
    }
    SetStack(stack, sphidaData->GetNowHorl());
    return 1;
}

/**
 * Sets the sphida score.
 */
static int _SPHIDA_SET_SCORE(RS_STACKDATA *stack, int argc) {
    CSubGameData *subGame = GetSubGameSaveData();
    if (subGame == NULL) {
        return 0;
    }
    CSphidaData *sphidaData;
    if ((sphidaData = subGame->GetSphidaData()) == NULL) {
        return 0;
    }
    int slot = GetStackInt(stack++);
    int score = GetStackInt(stack);
    sphidaData->SetHorlScore(score, slot);
    return 1;
}

/**
 * Returns the sphida score.
 */
static int _SPHIDA_GET_SCORE(RS_STACKDATA *stack, int argc) {
    if (argc != 2) {
        return 0;
    }
    CSubGameData *subGame = GetSubGameSaveData();
    if (subGame == NULL) {
        return 0;
    }
    CSphidaData *sphidaData;
    if ((sphidaData = subGame->GetSphidaData()) == NULL) {
        return 0;
    }
    int slot = GetStackInt(stack++);
    SetStack(stack, sphidaData->GetHorlScore(slot));
    return 1;
}

/**
 * Reports success without changing event state.
 */
static s32 _TEST(RS_STACKDATA *stack, int argc) {
    return 1;
}

/**
 * Runs the memory test with the supplied stack arguments.
 */
static void _MT_TEST(RS_STACKDATA *stack, int argc) {
    mt_test(stack, argc);
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ZERO_VECTOR__FP12RS_STACKDATAi);
/**
 * Normalizes a referenced vector on the script stack.
 */
static int _NORMAL_VECTOR(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR vec;
    vec[0] = (stack[0].p)->f;
    vec[1] = (stack[1].p)->f;
    vec[2] = (stack[2].p)->f;
    vec[3] = 1.0f;
    sceVu0Normalize(vec, vec);
    SetStack(stack++, vec[0]);
    SetStack(stack++, vec[1]);
    SetStack(stack, vec[2]);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _COPY_VECTOR__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ADD_VECTOR__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SUB_VECTOR__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SCALE_VECTOR__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DIV_VECTOR__FP12RS_STACKDATAi__2);
/**
 * Returns the length of a vector through the script stack.
 */
#ifdef NONMATCHING
static int _DIST_VECTOR(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR vec;
    GetStackVector(vec, stack);

    stack += 3;
    SetStack(stack, mgDistVector(vec));
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DIST_VECTOR__FP12RS_STACKDATAi);
#endif

/**
 * Returns the distance between two vectors through the script stack.
 */
#ifdef NONMATCHING
static int _DIST_VECTOR2(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR from;
    sceVu0FVECTOR to;
    GetStackVector(from, stack);
    GetStackVector(to, &stack[3]);

    stack += 6;
    SetStack(stack, mgDistVector(from, to));
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DIST_VECTOR2__FP12RS_STACKDATAi);
#endif

/**
 * Returns the square root of a stack value.
 */
static int _SQRT(RS_STACKDATA *stack, int argc) {
    float value = GetStackFloat(stack++);
    SetStack(stack, (float)sqrt(value));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ATAN2F__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ANGLE_CMP__FP12RS_STACKDATAi__2);
/**
 * Normalizes an angle referenced by the script stack.
 */
static int _ANGLE_LIMIT(RS_STACKDATA *stack, int argc) {
    RS_STACKDATA *angle = stack->p;
    SetStack(stack, mgAngleLimit(angle->f));
    return 1;
}

/**
 * Returns a random integer or float below the supplied limit.
 */
static int _GET_RAND(RS_STACKDATA *stack, int argc) {
    int result;
    int intRange;
    if (stack->type == RS_FLOAT) {
        float range = GetStackFloat(stack++);
        SetStack(stack, range * (float)rand() / 2147483648.0f);
    } else {
        intRange = GetStackInt(stack++);
        result = (int)((float)intRange * (float)rand() / 2147483648.0f);
        SetStack(stack, result);
    }
    return 1;
}

/**
 * Returns a point distance from a line segment, or -1 when its projection lies outside the segment.
 */
#ifdef NONMATCHING
static int _LINE_POINT_DIST(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR segmentStart;
    sceVu0FVECTOR segmentEnd;
    sceVu0FVECTOR point;
    sceVu0FVECTOR toPoint;
    sceVu0FVECTOR direction;
    float segmentLength;
    float projection;
    GetStackVector(segmentStart, stack);
    GetStackVector(segmentEnd, &stack[3]);
    GetStackVector(point, &stack[6]);
    RS_STACKDATA *result = &stack[9];
    segmentLength = mgDistVector(segmentStart, segmentEnd);
    sceVu0SubVector(toPoint, point, segmentStart);
    sceVu0SubVector(direction, segmentEnd, segmentStart);
    sceVu0Normalize(direction, direction);
    projection = sceVu0InnerProduct(toPoint, direction);
    if (projection < 0.0f || projection > segmentLength) {
        SetStack(result, -1.0f);
        return 1;
    }
    sceVu0ScaleVector(direction, direction, projection);
    sceVu0AddVector(direction, segmentStart, direction);
    SetStack(result, mgDistVector(point, direction));
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LINE_POINT_DIST__FP12RS_STACKDATAi);
#endif

/**
 * Creates the sword effect.
 */
static int _CREATE_SWORD_EFFECT(RS_STACKDATA *stack, int argc) {
    int stackNo = GetStackInt(stack++);
    int initParam1 = GetStackInt(stack++);
    int initParam2 = GetStackInt(stack);
    mgCMemory *sceneStack;
    CSWordAfterImage *effect;
    sceneStack = EventScene->GetStack(stackNo);
    if (sceneStack == NULL) {
        return 0;
    }
    effect = new (sceneStack->Alloc((sizeof(CSWordAfterImage) + 15) / 16 + 2)) CSWordAfterImage;
    if (effect != NULL) {
        effect->edge_color[0] = 0x80;
        effect->edge_color[1] = 0x80;
        effect->edge_color[2] = 0x80;
        effect->edge_color[3] = 0x80;
        effect->back_color[0] = 0x80;
        effect->back_color[1] = 0x80;
        effect->back_color[2] = 0x80;
        effect->back_color[3] = 0x80;
    }
    SwordEffect = effect;
    if (SwordEffect == NULL) {
        return 0;
    }
    SwordEffect->Initialize( sceneStack, initParam1, initParam2);
    return 1;
}

/**
 * Deletes the sword effect.
 */
static int _DELETE_SWORD_EFFECT(RS_STACKDATA *stack, int argc) {
    SwordEffect = 0;
    return 1;
}

/**
 * Sets the edge and back colours of the sword afterimage.
 */
static int _SWORD_EFFECT_COLOR(RS_STACKDATA *stack, int argc) {
    if (SwordEffect == NULL) {
        return 0;
    }
    int aR = GetStackInt(stack++);
    int aG = GetStackInt(stack++);
    int aB = GetStackInt(stack++);
    int aA = GetStackInt(stack++);
    int bR = GetStackInt(stack++);
    int bG = GetStackInt(stack++);
    int bB = GetStackInt(stack++);
    int bA = GetStackInt(stack);
    CSWordAfterImage *effect = SwordEffect;
    effect->edge_color[0] = aR;
    effect->edge_color[1] = aG;
    effect->edge_color[2] = aB;
    effect->edge_color[3] = aA;
    effect->back_color[0] = bR;
    effect->back_color[1] = bG;
    effect->back_color[2] = bB;
    effect->back_color[3] = bA;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SWORD_EFFECT_ADD_POINT__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ADD_CHARA_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ADD_CHARA_ROT__FP12RS_STACKDATAi);
/**
 * Places a treasure box at the script position with its requested contents and facing.
 */
static int _POST_TREASURE_BOX(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;
    CTreasureBoxManager *chestManager;
    float angle;
    int kind;
    int count;
    DNG_BATTLE_AREA *dngScene;

    count = 1;
    GetStackVector(position, stack);
    stack += 3;
    angle = GetStackFloat(stack++);
    kind = GetStackInt(stack++);
    if (argc >= 6) {
        count = GetStackInt(stack);
    }
    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    chestManager = dngScene->treasure_box;
    if (chestManager == NULL) {
        return 0;
    }
    chestManager->PutTreasureBox(-1, position, angle, 0x41, kind, count, -1, 0);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_PARTS_ORIGIN__FP12RS_STACKDATAi);
/**
 * Moves the active camera controller one step.
 */
static int _CTRLC_STEP(RS_STACKDATA *stack, int argc) {
    CCameraControl *p = GetCamera();
    p->Step(1);
    return 1;
}

/**
 * Sets the camera controller rotate.
 */
static int _CTRLC_SET_ROTATE(RS_STACKDATA *stack, int argc) {
    float angle = GetStackFloat(stack);
    GetCamera()->SetRotate(angle);
    return 1;
}

/**
 * Swings the camera eye to the requested angle about its look-at point.
 */
static int _CTRLC_ROT_BACK(RS_STACKDATA *stack, int argc) {
    float angle = GetStackFloat(stack);
    GetCamera()->RotBack(angle);
    return 1;
}

/**
 * Moves the camera from controller input after collecting nearby camera collision polygons.
 */
static int _CTRLC_MOVE_CAMERA(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR charaPos;
    CCPoly polys[256];
    mgVu0FBOX box;
    sceVu0FVECTOR cameraRef;
    CCharacter2 *chara;
    CCameraControl *camera;
    CMap *map;
    int polyCount;
    float distance;

    if ((chara = GetCharacter(GetStackInt(stack))) == NULL) {
        return 0;
    }
    chara->GetRotation(charaPos);
    camera = GetCamera();
    distance = camera->GetDistance();
    map = (CMap *)(EventScene)->GetMap((EventScene)->active_map);
    camera->GetRef(cameraRef);
    camera->SetCheckRef(cameraRef);
    box.max[0] = cameraRef[0] + 1.2f * distance;
    box.min[0] = cameraRef[0] - 1.2f * distance;
    box.max[1] = cameraRef[1] + 1.2f * distance;
    box.min[1] = cameraRef[1] - 1.2f * distance;
    box.max[2] = cameraRef[2] + 1.2f * distance;
    box.min[2] = cameraRef[2] - 1.2f * distance;
    box.max[3] = 1.0f;
    box.min[3] = 1.0f;
    polyCount = map->GetCameraPoly(polys, box, 0x100);
    if (polyCount < 0) {
        return 0;
    }
    if (polyCount > 0x100) {
        printf("EVENT ERROR <CTRLC_MOVE_CAMERA camera poly over %d>\n", polyCount);
        return 0;
    }
    camera->MoveCamera(&PadCtrl, charaPos, polys, polyCount);
    return 1;
}

/**
 * Sets the camera controller rotation cancel.
 */
static int _CTRLC_SET_ROT_CANCEL(RS_STACKDATA *stack, int argc) {
    int mask;

    mask = GetStackInt(stack);
    GetCamera()->SetRotCameraCancel(mask);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CTRLC_MOVE_RANGE__FP12RS_STACKDATAi);
/**
 * Returns the nearest active treasure box within thirty units of a position.
 */
static int _GET_NEAR_TBOX_POS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR target;
    sceVu0FVECTOR boxPos;
    DNG_BATTLE_AREA *dngScene;
    int nearest;
    int i;
    CTreasureBoxManager *boxManager;
    float nearestDist;
    float dist;
    CTreasureBox *box;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    boxManager = dngScene->treasure_box;
    if (boxManager == NULL) {
        return 0;
    }
    nearest = -1;
    GetStackVector(target, stack);

    stack = stack + 3;
    nearestDist = 9999.0f;
    for (i = 0; i < 24; i++) {
        box = &boxManager->box[i];
        if (box != NULL && (box == NULL || box->state != 0)) {
            box->GetPosition(boxPos);
            dist = mgDistVector(boxPos, target);
            if (dist < 30.0f && nearestDist > dist) {
                nearestDist = dist;
                nearest = i;
            }
        }
    }
    boxPos[0] = 0.0f;
    boxPos[1] = 0.0f;
    boxPos[2] = 0.0f;
    boxPos[3] = 0.0f;
    if (nearest >= 0) {
        box = &boxManager->box[nearest];
        box->GetPosition(boxPos);
    }
    SetStack(stack++, boxPos[0]);
    SetStack(stack++, boxPos[1]);
    SetStack(stack++, boxPos[2]);
    SetStack(stack, nearest);
    return 1;
}

/**
 * Reports failure without changing event state.
 */
static s32 _CONV_CHRNO_S2L(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Initializes the swe.
 */
static int _SWE_INIT(RS_STACKDATA *stack, int argc) {
    int charaNo = GetStackInt(stack++);
    int slot = GetStackInt(stack++);
    int stackNo = GetStackInt(stack++);
    int initParam1 = GetStackInt(stack++);
    int initParam2 = GetStackInt(stack);
    CCharacter2 *chara;
    if ((chara = GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    mgCMemory *sceneStack;
    if ((sceneStack = EventScene->GetStack(stackNo)) == NULL) {
        return 0;
    }
    CSWordAfterEffect *effect = new (sceneStack->Alloc(12)) CSWordAfterEffect;
    chara->sword_effect[slot] = effect;
    if (chara->sword_effect[slot] == NULL) {
        return 0;
    }
    chara->sword_effect[slot]->Initialize(sceneStack, initParam1, initParam2);
    return 1;
}

/**
 * Sets the swe color.
 */
static int _SWE_SET_COLOR(RS_STACKDATA *stack, int argc) {
    int aR, aG, aB, aA, bR, bG, bB, bA;
    int charaNo, slot;
    CCharacter2 *chara;
    CSWordAfterEffect **effectSlot;
    CSWordAfterEffect **effects;
    charaNo = GetStackInt(stack++);
    slot = GetStackInt(stack++);
    if ((chara = GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    effects = chara->sword_effect;
    effectSlot = &effects[slot];
    if (*effectSlot == NULL) {
        return 0;
    }
    aR = GetStackInt(stack++);
    aG = GetStackInt(stack++);
    aB = GetStackInt(stack++);
    aA = GetStackInt(stack++);
    bR = GetStackInt(stack++);
    bG = GetStackInt(stack++);
    bB = GetStackInt(stack++);
    bA = GetStackInt(stack);
    CSWordAfterEffect *effect = *effectSlot;
    effect->color0[0] = aR;
    effect->color0[1] = aG;
    effect->color0[2] = aB;
    effect->color0[3] = aA;
    effect->color1[0] = bR;
    effect->color1[1] = bG;
    effect->color1[2] = bB;
    effect->color1[3] = bA;
    return 1;
}

/**
 * Sets the swe texture.
 */
static int _SWE_SET_TEXTURE(RS_STACKDATA *stack, int argc) {
    mgCTexture *texture;
    int blockNo;
    char *name;
    int u0;
    int v0;
    int u1;
    int v1;
    CSWordAfterEffect **effectSlot;
    CSWordAfterEffect **effects;
    CCharacter2 *chara;
    int slot;
    int charaNo;
    charaNo = GetStackInt(stack++);
    slot = GetStackInt(stack++);
    if ((chara = GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    effects = chara->sword_effect;
    effectSlot = &effects[slot];
    if (*effectSlot == NULL) {
        return 0;
    }
    blockNo = GetStackInt(stack++);
    name = GetStackString(stack++);
    u0 = GetStackInt(stack++);
    v0 = GetStackInt(stack++);
    u1 = GetStackInt(stack++);
    v1 = GetStackInt(stack);
    if ((texture = mgTexManager.GetTexture( name, blockNo)) == NULL) {
        return 0;
    }
    (*effectSlot)->SetTexture(blockNo, texture, u0, v0, u1, v1);
    return 1;
}

/**
 * Starts the swe effect.
 */
static int _SWE_START_EFFECT(RS_STACKDATA *stack, int argc) {
    int charaNo;
    int slot;
    CCharacter2 *chara;
    char *fromName;
    char *toName;
    int length;
    int fade_time;
    int hold_time;
    CSWordAfterEffect **effectSlot;
    CSWordAfterEffect **effects;
    mgCFrame *fromFrame;
    mgCFrame *toFrame;
    charaNo = GetStackInt(stack++);
    slot = GetStackInt(stack++);
    if ((chara = GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    effects = chara->sword_effect;
    effectSlot = &effects[slot];
    if (*effectSlot == NULL) {
        return 0;
    }
    fromName = GetStackString(stack++);
    toName = GetStackString(stack++);
    length = GetStackInt(stack++);
    fade_time = GetStackInt(stack++);
    hold_time = GetStackInt(stack);
    if (chara->CObjectFrame::frame == NULL) {
        return 0;
    }
    if ((fromFrame = chara->CObjectFrame::frame->SearchFrame(fromName)) == NULL) {
        return 0;
    }
    if (chara->CObjectFrame::frame == NULL) {
        return 0;
    }
    if ((toFrame = chara->CObjectFrame::frame->SearchFrame(toName)) == NULL) {
        return 0;
    }
    (*effectSlot)->StartEffect(fromFrame, toFrame, length, fade_time, hold_time);
    return 1;
}

/**
 * Sets the character type.
 */
static int _SET_CHARA_TYPE(RS_STACKDATA *stack, int argc) {
    int charaNo;

    charaNo = GetStackInt(stack++);
    (EventScene)->SetType(1, charaNo, GetStackInt(stack));
    return 1;
}

/**
 * Returns the event data.
 */
static int _GET_EVENT_DATA(RS_STACKDATA *stack, int argc) {
    CSceneEventData *eventData;

    eventData = &(EventScene)->event_data;
    if (eventData == NULL) {
        return 0;
    }
    switch (GetStackInt(stack++)) {
        case 0:
            SetStack(stack, (int)eventData->event.flag);
            break;
        case 1:
            SetStack(stack, eventData->event.event_no);
            break;
        case 2:
            SetStack(stack, eventData->event.point_no);
            break;
        case 3:
            SetStack(stack, eventData->event.unk_2c);
            break;
        case 4:
            SetStack(stack, eventData->event.unk_30);
            break;
        case 5:
            SetStack(stack, eventData->event.unk_34);
            break;
        case 6:
            SetStack(stack, eventData->map_event.check_type);
            break;
        case 7:
            SetStack(stack, eventData->map_event.event_no);
            break;
        case 8:
            SetStack(stack++, eventData->map_event.matrix[3][0]);
            SetStack(stack++, eventData->map_event.matrix[3][1]);
            SetStack(stack++, eventData->map_event.matrix[3][2]);
            SetStack(stack,
             atan2f(eventData->map_event.matrix[2][0], eventData->map_event.matrix[2][2]));
            break;
        case 9:
            SetStack(stack, eventData->map_event.parts_no);
            break;
        case 10:
            SetStack(stack++, eventData->position[0]);
            SetStack(stack++, eventData->position[1]);
            SetStack(stack, eventData->position[2]);
            break;
        case 11:
            switch (argc) {
                case 2:
                    SetStack(stack, eventData->rotation[1]);
                    break;
                case 4:
                    SetStack(stack++, eventData->rotation[0]);
                    SetStack(stack++, eventData->rotation[1]);
                    SetStack(stack, eventData->rotation[2]);
                    break;
                default:
                    break;
            }
            break;
        case 12:
            SetStack(stack++, eventData->scale[0]);
            SetStack(stack++, eventData->scale[1]);
            SetStack(stack, eventData->scale[2]);
            break;
        case 13:
            SetStack(stack, eventData->chara_slot);
            break;
        case 14:
            SetStack(stack, eventData->chara_no);
            break;
        case 15:
            SetStack(stack, eventData->gameobj_no);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Sets the dungeon prev floor.
 */
static int _DNG_SET_PREV_FLOOR(RS_STACKDATA *stack, int argc) {
    int floor = GetStackInt(stack);
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    dungeon->prev_floor_id[dungeon->stage_id] = floor;
    return 1;
}

/**
 * Returns the dungeon prev floor.
 */
static int _DNG_GET_PREV_FLOOR(RS_STACKDATA *stack, int argc) {
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    SetStack(stack, dungeon->prev_floor_id[dungeon->stage_id]);
    return 1;
}

/**
 * Reports failure without changing event state.
 */
static s32 _DNG_SET_FAST_FLOOR(RS_STACKDATA *stack, int argc) {
    return 0;
}

/**
 * Sets the floor info.
 */
static int _SET_FLOOR_INFO(RS_STACKDATA *stack, int argc) {
    int dungeonNo = GetStackInt(stack++);
    int floorNo = GetStackInt(stack++);
    int field = GetStackInt(stack++);
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    DNG_FLOOR_SAVE *info;
    if ((info = dungeon->GetFloorInfoPtr(dungeonNo, floorNo)) == NULL) {
        return 0;
    }
    switch (field) {
        case 0:
            info->unk_0 = (int)GetStackFloat(stack);
            break;
        case 1:
            info->fast_destroy_time = (int)GetStackFloat(stack);
            break;
        case 2:
            info->unk_8 = GetStackInt(stack);
            break;
        case 3:
            info->unk_a = GetStackInt(stack);
            break;
        case 4:
            info->spheda_clear = GetStackInt(stack);
            break;
        case 5:
            info->flag |= (u16)GetStackInt(stack);
            break;
        case 6:
            info->kill_count = GetStackInt(stack);
            break;
        case 7:
            info->visit_count = GetStackInt(stack);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Returns the floor info.
 */
static int _GET_FLOOR_INFO(RS_STACKDATA *stack, int argc) {
    int dungeonNo = GetStackInt(stack++);
    int floorNo = GetStackInt(stack++);
    int field = GetStackInt(stack++);
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CSaveDataDungeon *dungeon = &save->save_dungeon;
    if (dungeon == NULL) {
        return 0;
    }
    DNG_FLOOR_SAVE *info = dungeon->GetFloorInfoPtr(dungeonNo, floorNo);
    if (info == NULL) {
        return 0;
    }
    switch (field) {
        case 0:
            SetStack(stack, info->unk_0);
            break;
        case 1:
            SetStack(stack, info->fast_destroy_time);
            break;
        case 2:
            SetStack(stack, info->unk_8);
            break;
        case 3:
            SetStack(stack, info->unk_a);
            break;
        case 4:
            SetStack(stack, info->spheda_clear);
            break;
        case 5:
            SetStack(stack, info->flag);
            break;
        case 6:
            SetStack(stack, info->kill_count);
            break;
        case 7:
            SetStack(stack, info->visit_count);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Returns the next floor.
 */
static int _GET_NEXT_FLOOR(RS_STACKDATA *stack, int argc) {
    int result;
    int floor = GetStackInt(stack++);
    int route = GetStackInt(stack++);
    DNG_BATTLE_AREA *dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    CDngFloorManager *floorManager = &dngScene->floor_manager;
    if (floorManager == NULL) {
        return 0;
    }
    result = floorManager->GetDngMapNextFloorID(floor, route);
    SetStack(stack, result);
    return 1;
}

/**
 * Disables automatic repeating of pad input.
 */
static int _PAD_AUTO_REPEAT_OFF(RS_STACKDATA *stack, int argc) {
    GamePad__2.AutoRepeatOff();
    return 1;
}

/**
 * Sets the pad auto repeat.
 */
static int _PAD_SET_AUTO_REPEAT(RS_STACKDATA *stack, int argc) {
    int mask = GetStackInt(stack++);
    int delay = GetStackInt(stack++);
    int interval = GetStackInt(stack);
    GamePad__2.SetAutoRepeat(mask, delay, interval);
    return 1;
}

/**
 * Pauses the dungeon.
 */
static int _DNG_PAUSE(RS_STACKDATA *stack, int argc) {
    int mask = GetStackInt(stack++);
    int enable = GetStackInt(stack);
    DNG_BATTLE_AREA *dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    if (enable) {
        dngScene->pause_flag |= mask;
    } else {
        dngScene->pause_flag &= ~mask;
    }
    return 1;
}

/**
 * Checks the dungeon pause.
 */
static int _DNG_CHECK_PAUSE(RS_STACKDATA *stack, int argc) {
    int mask = GetStackInt(stack++);
    DNG_BATTLE_AREA *dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    int pauseFlags = dngScene->pause_flag;
    SetStack(stack, pauseFlags & mask);
    return 1;
}

/**
 * Resets the dungeon timer.
 */
static int _DNG_RESET_TIMER(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *dngScene;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    dngScene->timer = 0;
    return 1;
}

/**
 * Returns the dungeon timer.
 */
static int _DNG_GET_TIMER(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *dngScene;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    SetStack(stack, dngScene->timer);
    return 1;
}

/**
 * Loads the skin.
 */
static int _LOAD_SKIN(RS_STACKDATA *stack, int argc) {
    int stackNo;
    int imageBlock;
    int charaNo;
    char name[32];
    char *infoName;
    char *packFile;
    CCharacter2 *chara;
    mgCMemory *sceneStack;
    mgCTextureManager *texManager;
    stackNo = GetStackInt(stack++);
    switch (stack->type) {
        case RS_INT:
            packFile = (char *)GetItemFilePath(GetStackInt(stack++), 0);
            break;
        case RS_STR:
            packFile = GetStackString(stack++);
            break;
    }
    infoName = GetStackString(stack++);
    charaNo = GetStackInt(stack);
    if ((chara = GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    if (0 > (imageBlock = (EventScene)->GetCharaTexb(charaNo))) {
        return 0;
    }
    if ((sceneStack = EventScene->GetStack(stackNo)) == NULL) {
        return 0;
    }

    if ((packFile = (char *)GetLoadBGBuff(packFile, 0)) == 0) {
        return 0;
    }
    sprintf(name, "ev%d", charaNo);
    texManager = &mgTexManager;
    if (charaNo >= 8) {
        strcpy(texManager->name_suffix, name);
    }
    chara->LoadSkin((u32 *)packFile, infoName, "", sceneStack, imageBlock);
    if (charaNo >= 8) {
        texManager->name_suffix[0] = 0;
    }
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHK_CAMERA_COL__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_PARTS_FUNC_POS__FP12RS_STACKDATAi);
/**
 * Returns the random circle position.
 */
static int _RANDOM_CIRCLE_GET_POS(RS_STACKDATA *stack, int argc) {
    float pos[3];
    int circleId = GetStackInt(stack++);
    if (RandomCircle.GetPosition( pos, circleId) <= -1) {
        return 0;
    }
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Deactivates a selected random circle, or the current hit circle when the script passes -1.
 */
static int _RANDOM_CIRCLE_OFF(RS_STACKDATA *stack, int argc) {
    int circleId = GetStackInt(stack);
    if (circleId == -1) {
        int current = RandomCircle.hit;
        if (current != -1) {
            RandomCircle.active[current] = 0;
        }
    } else {
        RandomCircle.active[circleId] = 0;
    }
    return 1;
}

/**
 * Swaps the current map first eight light sets with the following eight.
 */
static int _DNG_XCHG_MAP_LIGHT(RS_STACKDATA *stack, int argc) {
    XChgMapLighting();
    return 1;
}

/**
 * Disables Geostone animation.
 */
static int _GEOSTONE_ANIME_OFF(RS_STACKDATA *stack, int argc) {
    GeoStone.anime = 0;
    return 1;
}

/**
 * Sets the geostone flag.
 */
static int _GEOSTONE_SET_FLAG(RS_STACKDATA *stack, int argc) {
    GeoStone.SetFlag(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GEOSTONE_SET_REFERENCE__FP12RS_STACKDATAi);
/**
 * Deletes the references of the Geostone model frame.
 */
static int _GEOSTONE_DEL_REFERENCE(RS_STACKDATA *stack, int argc) {
    if (GeoStone.CObjectFrame::frame == NULL) {
        return 0;
    }
    GeoStone.CObjectFrame::frame->DeleteReference();
    return 1;
}

/**
 * Returns the robo move type.
 */
static int _GET_ROBO_MOVE_TYPE(RS_STACKDATA *stack, int argc) {
    SetStack(stack, ((CActionChara *)GetCharacter(0))->move_type);
    return 1;
}

/**
 * Sets the exit flag.
 */
static int _SET_EXIT_FLAG(RS_STACKDATA *stack, int argc) {
    EventScene->exit_flag = GetStackInt(stack);
    return 1;
}

/**
 * Returns the exit flag.
 */
static int _GET_EXIT_FLAG(RS_STACKDATA *stack, int argc) {
    SetStack(stack, EventScene->exit_flag);
    return 1;
}

/**
 * Returns zero for the E3-version script query.
 */
static int _GET_E3_VERSION(RS_STACKDATA *stack, int argc) {
    SetStack(stack, 0);
    return 1;
}

/**
 * Returns the requested pad-control button state when two script arguments are present.
 */
static int _CHK_PAD_CTRL(RS_STACKDATA *stack, int argc) {
    if (argc != 2) {
        return 0;
    }
    int button = GetStackInt(stack++);
    SetStack(stack, PadCtrl.Btn(button));
    return 1;
}

/**
 * Stops the active camera controller from moving.
 */
static int _CTRLC_STAY(RS_STACKDATA *stack, int argc) {
    CCameraControl *camera = GetCamera();
    camera->Stay();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _BSCN_SET_BLIGHT_RATE__FP12RS_STACKDATAi);
/**
 * Returns the trap identifier of a selected random circle.
 */
static int _GET_RND_CIRCLE_TRAPID(RS_STACKDATA *stack, int argc) {
    int circleId = GetStackInt(stack++);
    SetStack(stack, GetRandomCircleTrapID(circleId));
    return 1;
}

/**
 * Sets the rnd circle status.
 */
static int _SET_RND_CIRCLE_STATUS(RS_STACKDATA *stack, int argc) {
    float result;
    int circleId = GetStackInt(stack++);
    SetRandamCircleStatus(circleId, result);
    SetStack(stack, result);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_STATUSBAR_SHOW__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_PULL_ITEM__FP12RS_STACKDATAi);
/**
 * Opens the dungeon party-character change menu and suspends the event script for it.
 */
static int _MENU_CHARA_CHENGE(RS_STACKDATA *stack, int argc) {
    GetStackInt(stack);
    MenuArg.open_type = MENU_OPEN_CHARA_CHANGE_DUNGEON;
    EdEventInfo.command_mode = EVENT_COMMAND_SUB_MODE;
    return 1;
}

/**
 * Returns the sound-bank handle stored in a selected event sound port.
 */
static int _GET_EVENT_INFO_SNDID(RS_STACKDATA *stack, int argc) {
    int index = GetStackInt(stack++);
    SetStack(stack, EdEventInfo.snd_id[index]);
    return 1;
}

/**
 * Returns the parts position.
 */
static int _GET_PARTS_POS(RS_STACKDATA *stack, int argc) {
    CMap *maps[8];
    sceVu0FVECTOR pos;
    CMapParts *parts;
    int mapCount;
    int i;
    int partsId;
    char *partsName;
    mapCount = (EventScene)->GetActiveMap(maps, 8);
    if (!(mapCount > 0)) {
        return 0;
    }
    switch (stack->type) {
        case 0:
            partsId = GetStackInt(stack++);
            for (i = 0; i < mapCount; i++) {
                parts = maps[i]->GetPlaceParts( partsId);
                if (parts != NULL) {
                    break;
                }
            }
            break;
        case 2:
            partsName = GetStackString(stack++);
            for (i = 0; i < mapCount; i++) {
                parts = maps[i]->GetPlaceParts( partsName);
                if (parts != NULL) {
                    break;
                }
            }
            break;
    }
    if (parts == NULL) {
        return 0;
    }
    parts->GetPosition(pos);
    SetStack(stack++, pos[0]);
    SetStack(stack++, pos[1]);
    SetStack(stack, pos[2]);
    return 1;
}

/**
 * Disables skipping of the running drama scene.
 */
static s32 _CANCEL_DRAMA_SCENE(RS_STACKDATA *stack, s32 argc) {
    CancelDramaScene();
    return 1;
}

/**
 * Returns the rndc motion nowt.
 */
static int _GET_RNDC_MOT_NOWT(RS_STACKDATA *stack, int argc) {
    float weight = RandomCircle.model.frame;
    SetStack(stack, weight);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_CHARA_MOT_NOWT__FP12RS_STACKDATAi);
/**
 * Returns a selected scene character to its normal behaviour.
 */
static int _CHARA_NORMAL_DRIVE(RS_STACKDATA *stack, int argc) {
    CCharacter2 *chara;
    chara = GetCharacter(GetStackInt(stack));
    if (chara == NULL) {
        return 0;
    }
    chara->NormalDrive();
    return 1;
}

/**
 * Resets the character deformation animation.
 */
static int _CHARA_RESET_DA(RS_STACKDATA *stack, int argc) {
    CCharacter2 *chara;

    if ((chara = GetCharacter(GetStackInt(stack))) == NULL) {
        return 0;
    }
    chara->ResetDAPosition();
    chara->StepDA(0xA);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _DNG_SETUP_MAIN_UNIT__FP12RS_STACKDATAi);
/**
 * Adds or removes a party character, or returns that character membership through a reference slot.
 */
static int _JOIN_PARTY_MEMBER(RS_STACKDATA *stack, int argc) {
    int charaNo;
    CUserDataManager *userData;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(stack++);
    switch (stack->type) {
        case RS_INT:
            if (GetStackInt(stack) == 1) {
                userData->JoinPartyMember(charaNo);
            } else {
                userData->LeavePartyMember(charaNo);
            }
            break;
        case RS_PTR: {
            int members = userData->GetNowPartyMember();
            SetStack(stack, (members & (1 << charaNo)) ? 1 : 0);
            break;
        }
        default:
        return 0;
    }
    return 1;
}

/**
 * Sets the character change flag.
 */
static int _SET_CHARA_CHANGE_FLAG(RS_STACKDATA *stack, int argc) {
    int charaNo;
    CUserDataManager *userData;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(stack++);
    if (GetStackInt(stack) == 1) {
        userData->EnableCharaChange(charaNo);
    } else {
        userData->DisableCharaChange(charaNo);
    }
    return 1;
}

/**
 * Sets the character change mask.
 */
static int _SET_CHARA_CHANGE_MASK(RS_STACKDATA *stack, int argc) {
    int charaNo;
    CUserDataManager *userData;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(stack++);
    if (GetStackInt(stack) == 1) {
        userData->EnableCharaChangeMask(charaNo);
    } else {
        userData->DisableCharaChangeMask(charaNo);
    }
    return 1;
}

/**
 * Sets the character equip.
 */
static int _SET_CHARA_EQUIP(RS_STACKDATA *stack, int argc) {
    int charaNo;
    int itemNo;
    CSaveData *save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    CUserDataManager *userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(stack++);
    itemNo = GetStackInt(stack);
    return userData->SetChrEquip(charaNo, itemNo);
}

/**
 * Loads the pack file.
 */
static int _LOAD_PACK_FILE(RS_STACKDATA *stack, int argc) {
    if (GetLoadBGBuff(GetStackString(stack), 0) == 0) {
        return 0;
    }
    EdEventInfo.pack_loaded = 1;
    return 1;
}

/**
 * Sets the bit ctrl.
 */
static int _SET_BIT_CTRL(RS_STACKDATA *stack, int argc) {
    CSaveData *save;
    int bit;

    if ((save = GetSaveData()) == NULL) {
        return 0;
    }
    bit = GetStackInt(stack++);
    if (GetStackInt(stack) == 1) {
        save->SetBitCtrl(bit);
    } else {
        save->ResetBitCtrl(bit);
    }
    return 1;
}

/**
 * Returns the bit ctrl.
 */
static int _GET_BIT_CTRL(RS_STACKDATA *stack, int argc) {
    CSaveData *save;
    int bit;
    int ctrl;

    if ((save = GetSaveData()) == NULL) {
        return 0;
    }
    bit = GetStackInt(stack++);
    ctrl = save->GetBitCtrl();
    SetStack(stack, (ctrl & bit) ? 1 : 0);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _LOAD_ARG__FP12RS_STACKDATAi);
/**
 * Returns the item have count.
 */
static int _GET_ITEM_HAVE_NUM(RS_STACKDATA *stack, int argc) {
    int result;
    int itemId = GetStackInt(stack++);
    result = GetUserItemHaveNum(itemId);
    SetStack(stack, result);
    return 1;
}

/**
 * Sets the skip botton.
 */
static int _SET_SKIP_BOTTON(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    EdEventInfo.skip_button = GetStackInt(stack);
    return 1;
}

/**
 * Sets the skip fade color.
 */
static int _SET_SKIP_FCOL(RS_STACKDATA *stack, int argc) {
    if (argc != 3) {
        return 0;
    }
    GetStackVector(EdEventInfo.skip_fade_color, stack);
    EdEventInfo.skip_fade_color[3] = 0;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_DEBUG_MODE__FP12RS_STACKDATAi);
/**
 * Returns the map type.
 */
static int _GET_MAP_TYPE(RS_STACKDATA *stack, int argc) {
    int mapNo;
    RS_STACKDATA *result;
    switch (stack->type) {
        case 0:
            result = stack + 1;
            mapNo = GetStackInt(stack);
            break;
        case 2:
            result = stack + 1;
            mapNo = SearchMapNo(GetStackString(stack));
            break;
        default:
        return 0;
    }
    SetStack(result, GetMapType(mapNo));
    return 1;
}

/**
 * Clears dungeon collision primitives and associates their manager with the dungeon scene.
 */
static int _DNG_COLLISION_ALL_CLR(RS_STACKDATA *stack, int argc) {
    ColPrimMan.Initialize(DngMainScene);
    return 1;
}

/**
 * Shows or hides the map behind the event.
 */
static int _SET_MAP_DRAW(RS_STACKDATA *stack, int argc) {
    EdEventInfo.map_draw = GetStackInt(stack);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _CHECK_MC_LOAD__FP12RS_STACKDATAi);
/**
 * Sets the now map number.
 */
static int _SET_NOW_MAP_NO(RS_STACKDATA *stack, int argc) {
    (EventScene)->SetNowMapNo(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_TBOX_PARAM__FP12RS_STACKDATAi);
/**
 * Loads the cancel villager.
 */
static int _CANCEL_LOAD_VILLAGER(RS_STACKDATA *stack, int argc) {
    CScene *scene = EventScene;
    scene->skip_load_sub_villager = 1;
    scene->skip_load_villager = 1;
    return 1;
}

/**
 * Suppresses the next loading-screen request.
 */
static int _CANCEL_NOW_LOADING(RS_STACKDATA *stack, int argc) {
    CancelNowLoading();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ESM_INITIALIZE__FP12RS_STACKDATAi);
/**
 * Allocates a work buffer in the selected scene stack for the event effect-script manager.
 */
static int _ESM_INIT_FIX(RS_STACKDATA *stack, int argc) {
    int stackNo;
    int heapSize;
    mgCMemory *sceneStack;
    mgCMemory *heap;
    if (EventEffectScript == 0) {
        return 0;
    }
    heapSize = 0x36B0;
    stackNo = GetStackInt(stack++);
    if (argc >= 2) {
        heapSize = GetStackInt(stack);
    }
    if ((sceneStack = EventScene->GetStack(stackNo)) == NULL) {
        return 0;
    }
    if ((heap = (mgCMemory *)operator new(sizeof(mgCMemory),
                                           (u_long128 *)sceneStack->Alloc(5))) != NULL) {
        heap->Init();
    }
    if (heap == NULL) {
        return 0;
    }
    heap->SetHeapMem((u_long128 *)sceneStack->stAlloc64(heapSize), heapSize);
    heap->stack_used = 0;
    heap->lock = 0;
    EventEffectScript->SetWorkBuffer(heap);
    return 1;
}

/**
 * Clears the effect script.
 */
static int _ESM_CLEAR(RS_STACKDATA *stack, int argc) {
    int clearedBlocks[68];
    mgCMemory *workBuffer;
    mgCTextureManager *texManager;
    int i;

    if (EventEffectScript == 0) {
        return 0;
    }
    EventEffectScript->ClearBaseFromLevel(0, clearedBlocks, 64);
    EventEffectScript->AllClearEffSpt();
    workBuffer = (mgCMemory *)EventEffectScript->work_memory;
    if (workBuffer != NULL) {
        workBuffer->stack_used = 0;
        workBuffer->lock = 0;
        workBuffer->ClearHeapMem();
    }
    texManager = &mgTexManager;
    EventEffectScript = 0;
    for (i = 0; i < 64; i++) {
        printf("[ESM_CLEAR] DEL TEXB = %d\n", clearedBlocks[i]);

        if (clearedBlocks[i] <= -1) {
            break;
        }
        texManager->DeleteBlock(clearedBlocks[i]);
    }
    return 1;
}

/**
 * Loads the effect script base.
 */
static int _ESM_LOAD_BASE(RS_STACKDATA *stack, int argc) {
    int ret;

    if (EventEffectScript == NULL) {
        return 0;
    }
    switch (stack->type) {
        case 0:
            ret = EventEffectScript->LoadBaseEffSpt(GetStackInt(stack), NULL, -1);
            break;
        case 2:
            ret = EventEffectScript->LoadBaseEffSpt(GetStackString(stack), NULL, -1);
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ESM_CREATE__FP12RS_STACKDATAi__2);
/**
 * Selects effect-script program 300 for the requested group and object.
 */
static int _ESM_FINISH(RS_STACKDATA *stack, int argc) {
    int group;
    RS_STACKDATA *nextSlot = stack + 1;

    if (EventEffectScript == NULL) {
        return 0;
    }
    group = GetStackInt(stack);
    EventEffectScript->SetScriptProgNo(0x12C, group, GetStackInt(nextSlot));
    return 1;
}

/**
 * Deletes the effect script.
 */
static int _ESM_DELETE(RS_STACKDATA *stack, int argc) {
    if (EventEffectScript == NULL) {
        return 0;
    }
    int group = GetStackInt(stack++);
    EventEffectScript->DeleteEffSpt(group, GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ESM_SET_VECT1__FP12RS_STACKDATAi__2);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ESM_SET_VECT2__FP12RS_STACKDATAi__2);
/**
 * Sets the effect script target id.
 */
static int _ESM_SET_TARGET_ID(RS_STACKDATA *stack, int argc) {
    int param1;
    int param2;
    int ret;

    if (EventEffectScript == NULL) {
        return 0;
    }
    switch (argc) {
        case 1:
            ret = EventEffectScript->SetScriptTargetId(GetStackInt(stack), -1, -1);
            break;
        case 3:
            param1 = GetStackInt(stack++);
            param2 = GetStackInt(stack++);
            ret = EventEffectScript->SetScriptTargetId(GetStackInt(stack), param1, param2);
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ESM_LOAD_BASE_PACK__FP12RS_STACKDATAi);
/**
 * Sets the effect script value.
 */
static int _ESM_SET_VALUE(RS_STACKDATA *stack, int argc) {
    int valueNo;
    int group;
    int slot;
    int ret;

    if (EventEffectScript == 0) {
        return 0;
    }
    switch (argc) {
        case 2:
            valueNo = GetStackInt(stack++);
            switch (stack->type) {
                case 0:
                    ret = EventEffectScript->SetValue(valueNo, GetStackInt(stack), -1, -1);
                    break;
                case 1:
                    ret = EventEffectScript->SetValue(valueNo, GetStackFloat(stack), -1, -1);
                    break;
                default:
            return 0;
            }
            break;
        case 4:
            group = GetStackInt(stack++);
            slot = GetStackInt(stack++);
            valueNo = GetStackInt(stack++);
            switch (stack->type) {
                case 0:
                    ret = EventEffectScript->SetValue(valueNo, GetStackInt(stack), group, slot);
                    break;
                case 1:
                    ret = EventEffectScript->SetValue(valueNo, GetStackFloat(stack), group, slot);
                    break;
                default:
            return 0;
            }
            break;
        default:
        return 0;
    }
    return ret;
}

/**
 * Sets the character condition.
 */
static int _SET_CHARA_CONDITION(RS_STACKDATA *stack, int argc) {
    switch (argc) {
        case 2: {
            CBattleCharaInfo *info;
            int chara;
            if ((info = GetBattleCharaInfo()) == 0) {
                return 0;
            }
            chara = GetStackInt(stack++);
            info->SetAttr(chara, GetStackInt(stack));
            break;
        }
        case 3: {
            CUserDataManager *userData;
            int attr;
            int charaNo;
            int value;
            if ((userData = GetUserDataMan()) == NULL) {
                return 0;
            }
            charaNo = GetStackInt(stack++);
            attr = GetStackInt(stack++);
            value = GetStackInt(stack);
            if (userData != NULL) {
                userData->SetCharaStatusAttirbute(charaNo, attr, value);
            }
            break;
        }
        default:
        return 0;
    }
    return 1;
}

/**
 * Adds the weapon health.
 */
static int _ADD_WHP(RS_STACKDATA *stack, int argc) {
    CBattleCharaInfo *info;
    if ((info = GetBattleCharaInfo()) == 0) {
        return 0;
    }
    int chara = GetStackInt(stack++);
    int amount = GetStackInt(stack);
    info->AddWhp(chara, amount);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ADD_HP_RATE__FP12RS_STACKDATAi);
/**
 * Returns the time.
 */
static int _GET_TIME(RS_STACKDATA *stack, int argc) {
    SetStack(stack, (EventScene)->time);
    return 1;
}

/**
 * Checks the get item limit.
 */
static int _CHECK_GET_ITEM_LIMIT(RS_STACKDATA *stack, int argc) {
    int result;
    int itemId;
    int count;
    switch (argc) {
        case 1:
            result = CheckItemLimmitOver();
            SetStack(stack, result);
            break;
        case 3:
            itemId = GetStackInt(stack++);
            count = GetStackInt(stack++);
            count = CheckGetItemRemainNum(itemId) - count < 0 ? 0 : count;
            SetStack(stack, count);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Checks the item over.
 */
static int _CHECK_ITEM_OVER(RS_STACKDATA *stack, int argc) {
    int result;
    result = CheckItemOver();
    SetStack(stack, result);
    return 1;
}

/**
 * Returns the now loop number.
 */
static int _GET_NOW_LOOP_NO(RS_STACKDATA *stack, int argc) {
    int result;
    result = GetNowLoopNo();
    SetStack(stack, result);
    return 1;
}

/**
 * Clears the is destroy.
 */
static int _IS_CLEAR_DESTROY(RS_STACKDATA *stack, int argc) {
    CDngFloorManager *floorManager;
    DNG_BATTLE_AREA *dngScene;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == 0) {
        return 0;
    }
    floorManager = &dngScene->floor_manager;
    if (floorManager == NULL) {
        return 0;
    }
    SetStack(stack, floorManager->IsClearMostFastDestroy());
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _IS_CLEAR_PRACTICE__FP12RS_STACKDATAi);
/**
 * Plays the is sub game.
 */
static int _IS_PLAY_SUB_GAME(RS_STACKDATA *stack, int argc) {
    CDngFloorManager *floorManager;
    DNG_BATTLE_AREA *dngScene;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == 0) {
        return 0;
    }
    floorManager = &dngScene->floor_manager;
    if (floorManager == NULL) {
        return 0;
    }
    SetStack(stack, floorManager->IsPlaySubGame());
    return 1;
}

/**
 * Resets the subject counter.
 */
static int _RESET_SUBJECT_COUNTER(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *dngScene;
    CSaveData *save;

    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    dngScene->subject_counter = (u32)save->play_time;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SCR_EFF_INIT_RASTER__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SCR_EFF_START_RASTER__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SCR_EFF_STOP_RASTER__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_MPCHARA_MOTION__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _FUNC_POINT_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _PARTS_NAME_STRCMP__FP12RS_STACKDATAi);
/**
 * Returns the trial version.
 */
static int _GET_TRIAL_VERSION(RS_STACKDATA *stack, int argc) {
    SetStack(stack, 0);
    return 1;
}

/**
 * Sets the floor episode.
 */
static int _SET_FLOOR_EPISODE(RS_STACKDATA *stack, int argc) {
    StartupEpisodeTitle.Switch(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _FUNC_POINT_GET_POS__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _FUNC_POINT_GET_ROT__FP12RS_STACKDATAi);
/**
 * Sets the action character def motion.
 */
static int _ACTCHR_SET_DEF_MOTION(RS_STACKDATA *stack, int argc) {
    int charaNo;
    int motionArg = 0;
    CActionChara *chara;
    charaNo = GetStackInt(stack++);
    if (argc >= 2) {
        motionArg = GetStackInt(stack);
    }
    if ((chara = (CActionChara *)GetCharacter(charaNo)) == NULL) {
        return 0;
    }
    char *defaultMotion = chara->default_motion;
    chara->ResetAction();
    chara->SetMotion(defaultMotion, motionArg, 1);
    return 1;
}

/**
 * Adds the fusion point.
 */
static int _ADD_FUSION_POINT(RS_STACKDATA *stack, int argc) {
    int group;
    int member;
    int points;
    CUserDataManager *userData;
    CSaveData *save;

    group = GetStackInt(stack++);
    member = GetStackInt(stack++);
    points = GetStackInt(stack);
    if ((group < 0) || (group > 1)) {
        return 0;
    }
    if ((member < 0) || (userData = NULL, (member > 1))) {
        return 0;
    }
    save = GetSaveData();
    if (save != 0) {
        userData = &save->user_data;
    }
    if (userData == NULL) {
        return 0;
    }
    userData->AddFusionPoint(group, member, points);
    return 1;
}

/**
 * Returns the debug flag.
 */
static int _GET_DEBUG_FLAG(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, DebugFlag);
    return 1;
}

/**
 * Enables the minimap door.
 */
static int _MINIMAP_DOOR_ENABLE(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR pos;
    if (argc != 3) {
        return 0;
    }
    GetStackVector(pos, stack);
    MinimapDoorEnable(pos);
    return 1;
}

/**
 * Checks the dungeon boss map.
 */
static int _DNG_CHECK_BOSS_MAP(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *dngScene;

    if (argc != 1) {
        return 0;
    }
    dngScene = &(EventScene)->battle_area;
    if (dngScene == NULL) {
        return 0;
    }
    SetStack(stack, dngScene->boss_map);
    return 1;
}

/**
 * Sets the dungeon battle area script event number.
 */
static int _DNG_RUN_EVENT(RS_STACKDATA *stack, int argc) {
    int eventNo = GetStackInt(stack);
    DNG_BATTLE_AREA *scene = &(EventScene)->battle_area;
    if (scene == NULL) {
        return 0;
    }
    s16 *slot = &scene->script.event_no;
    if (slot == NULL) {
        return 0;
    }
    *slot = (s16)eventNo;
    return 1;
}

/**
 * Checks the enable character change.
 */
static int _CHECK_ENABLE_CHARA_CHANGE(RS_STACKDATA *stack, int argc) {
    CUserDataManager *userData;
    int charaNo;
    CSaveData *save;
    RS_STACKDATA *nextSlot;

    if (argc != 2) {
        return 0;
    }
    nextSlot = stack + 1;
    charaNo = GetStackInt(stack);
    save = GetSaveData();
    if (save == 0) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    SetStack(nextSlot, userData->CheckEnableCharaChange(charaNo, NULL));
    return 1;
}

/**
 * Initializes the sepia.
 */
static int _INIT_SEPIA(RS_STACKDATA *stack, int argc) {
    int stackNo;
    int blockOffset;
    u_long128 *buffer;
    mgCMemory *sceneStack;
    int blockNo;
    int size;
    CScene *scene;
    int texCount;
    int texBase;

    blockOffset = 0;
    stackNo = GetStackInt(stack++);
    if (argc >= 2) {
        blockOffset = GetStackInt(stack);
    }
    scene = EventScene;
    texCount = scene->event_texb_num;
    texBase = scene->event_texb;
    if (texCount <= 0 || texCount < blockOffset) {
        return 0;
    }
    blockNo = texBase + blockOffset;
    if (stackNo >= 0) {
        if ((sceneStack = EventScene->GetStack(stackNo)) == NULL) {
            return 0;
        }
        size = mgScreenWidth * mgScreenHeight * mgScreenDepth;
        if ((buffer = sceneStack->stAlloc64(size / 8 / 16 + 1)) == 0) {
            return 0;
        }
    } else {
        buffer = read_buffer;
    }
    mgTexManager.DeleteBlock(blockNo);
    EventScreenEffect.SetSepiaTexture(mgTexManager.EnterTexture(
                                          blockNo, "event sepia", NULL, mgScreenWidth,
                                          mgScreenHeight, mgScreenDepth, 0, 0LL, 0),
                                      (u_long128 *)buffer);
    return 1;
}

/**
 * Starts the sepia.
 */
static s32 _START_SEPIA(RS_STACKDATA *stack, s32 argc) {
    EventScreenEffect.CaptureSepiaScreen();
    EventScreenEffect.SetSepiaFlag(1);
    return 1;
}

/**
 * Ends the sepia.
 */
static s32 _END_SEPIA(RS_STACKDATA *stack, s32 argc) {
    EventScreenEffect.SetSepiaFlag(0);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _COPY_MONS2SCNCHR__FP12RS_STACKDATAi);
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", __ct__7CObjectFRC7CObject);
/**
 * Unlocks the scene memory stack selected by the script.
 */
static int _UNLOCK_STACK(RS_STACKDATA *stack, int argc) {
    mgCMemory *sceneStack = EventScene->GetStack(GetStackInt(stack));

    if (sceneStack == NULL) {
        return 0;
    }
    sceneStack->lock = 0;
    return 1;
}

/**
 * Resets the event trg.
 */
static int _RESET_EVENT_TRG(RS_STACKDATA *stack, int argc) {
    EventScene->event_run = 0;
    return 1;
}

/**
 * Sets the character number.
 */
static int _SET_CHARA_NO(RS_STACKDATA *stack, int argc) {
    int slot = GetStackInt(stack++);

    EventScene->SetCharaNo(slot, GetStackInt(stack));
    return 1;
}

/**
 * Returns the character number.
 */
static int _GET_CHARA_NO(RS_STACKDATA *stack, int argc) {
    int id;
    RS_STACKDATA *args = stack;
    if (argc != 2) {
        return 0;
    }
    id = GetStackInt(args++);
    SetStack(args, EventScene->GetCharaNo(id));
    return 1;
}

/**
 * Searches for the character number.
 */
static int _SEARCH_CHARA_NO(RS_STACKDATA *stack, int argc) {
    int id;
    RS_STACKDATA *args = stack;
    if (argc != 2) {
        return 0;
    }
    id = GetStackInt(args++);
    SetStack(args, EventScene->SearchCharaID(id));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_NEAR_RANDOM_STONE_POS__FP12RS_STACKDATAi);
/**
 * Initializes the mono flash.
 */
static int _INIT_MONO_FLASH(RS_STACKDATA *stack, int argc) {
    u_long128 *buffers[2];
    mgCTexture *textures[2];
    mgCMemory *memory;
    int stackNo;
    int texBase;
    int texCount;

    stackNo = GetStackInt(stack);
    texCount = EventScene->event_texb_num;
    texBase = EventScene->event_texb;
    if (texCount <= 0 || texCount < 0) {
        return 0;
    }
    if (stackNo >= 0) {
        if ((memory = EventScene->GetStack(stackNo)) == NULL) {
            return 0;
        }
        if ((buffers[0] = (u_long128 *)memory->stAlloc64(
                 mgScreenDepth * (mgScreenWidth * mgScreenHeight) / 8 / 16 + 1)) == NULL) {
            return 0;
        }
        if ((buffers[1] = (u_long128 *)memory->stAlloc64(
                 mgScreenDepth * (mgScreenWidth * mgScreenHeight) / 8 / 16 + 1)) == NULL) {
            return 0;
        }
    } else {
        buffers[0] = (u_long128 *)read_buffer;
        buffers[1] =
            read_buffer + mgScreenDepth * (mgScreenWidth * mgScreenHeight) / 8 / 16;
    }
    mgTexManager.DeleteBlock(texBase);
    mgTexManager.DeleteBlock(texBase + 1);
    textures[0] = (mgCTexture *)mgTexManager.EnterTexture(
        texBase, "mono_flash1", NULL, mgScreenWidth, mgScreenHeight, mgScreenDepth, 0, 0, 0);
    textures[1] = (mgCTexture *)mgTexManager.EnterTexture(
        texBase + 1, "mono_flash2", NULL, mgScreenWidth, mgScreenHeight, mgScreenDepth, 0, 0,
        0);
    EventScreenEffect.SetMonoFlashTexture(textures, buffers);
    return 1;
}

/**
 * Starts the mono flash.
 */
static int _START_MONO_FLASH(RS_STACKDATA *stack, int argc) {
    int param = GetStackInt(stack);

    EventScreenEffect.CaptureMonoFlashScreen();
    EventScreenEffect.SetMonoFlashFlag(1, param);
    return 1;
}

/**
 * Ends the mono flash.
 */
static s32 _END_MONO_FLASH(RS_STACKDATA *stack, s32 argc) {
    EventScreenEffect.SetMonoFlashFlag(0, 0);
    return 1;
}

/**
 * Deletes the villager.
 */
static int _DELETE_VILLAGER(RS_STACKDATA *stack, int argc) {
    EventScene->DeleteVillager(GetStackInt(stack));
    return 1;
}

/**
 * Sets the dungeon weather.
 */
static int _DNG_SET_WEATHER(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *info = &EventScene->battle_area;
    int weather;
    if (info == NULL) {
        return 0;
    }
    weather = GetStackInt(stack);
    info->unk_8c = weather;
    if (weather == 2) {
        EventScene->AutoChangeEnvOffset(4);
    } else {
        EventScene->AutoChangeEnvOffset(0);
    }
    return 1;
}

/**
 * Sets the character maximum health.
 */
static int _SET_CHARA_MAXHP(RS_STACKDATA *stack, int argc) {
    int charaNo;
    RS_STACKDATA *args = stack;
    int maxHp;
    CUserDataManager *userData;
    CSaveData *save;
    CHARA_DATA *chara;

    charaNo = GetStackInt(args++);
    maxHp = GetStackInt(args);
    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }

    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    chara = userData->GetCharaDataPtr(charaNo);
    chara->hp.max = (float)maxHp;
    chara->hp.SetFillRate(1.0f);
    return 1;
}

/**
 * Sets the character defence.
 */
static int _SET_CHARA_DEFENCE(RS_STACKDATA *stack, int argc) {
    int charaNo;
    RS_STACKDATA *args = stack;
    int defence;
    CUserDataManager *userData;
    CSaveData *save;

    charaNo = GetStackInt(args++);
    defence = GetStackInt(args);
    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    userData->GetCharaDataPtr(charaNo)->defence = defence;
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _PLACE_PARTS_NAME_STRCMP__FP12RS_STACKDATAi);
/**
 * Opens the use item2.
 */
static int _GOTO_USE_ITEM2(RS_STACKDATA *stack, int argc) {
    int i;

    if (stack->type != RS_PTR) {
        return 0;
    }
    p_use_item = stack->p;
    stack++;
    MenuArg.open_type = 9;
    MenuArg.param[0] = GetStackInt(stack++);
    for (i = 1; i < argc - 1; i++) {
        MenuArg.param[i] = GetStackInt(stack++);
    }
    MenuArg.param[i] = 0;
    EdEventInfo.command_mode = 3;
    return 1;
}

/**
 * Sets the debug analyze flag.
 */
static int _DBG_SET_ANALYZE_FLAG(RS_STACKDATA *stack, int argc) {
    int area;
    RS_STACKDATA *args = stack;
    int entry;
    int flag;
    CSaveData *save;
    CEditData *editData;

    area = GetStackInt(args++);
    entry = GetStackInt(args++);
    flag = GetStackInt(args);
    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    editData = (CEditData *)save->GetEditData(area);
    if (editData == NULL) {
        return 0;
    }
    editData->dbgSetAnalyzeFlag(area, entry, flag);
    return 1;
}

/**
 * Shows or hides the selected Atlamillia model frames of a scene character.
 */
static int _ATRAMIRIA_ON_OFF(RS_STACKDATA *stack, int argc) {
    int mode;
    RS_STACKDATA *args = stack;
    int charaNo;
    int flag;

    mode = GetStackInt(args++);
    charaNo = GetStackInt(args++);
    flag = GetStackInt(args);
    AtraMiriaOnOff(mode, GetCharacter(charaNo), flag);
    return 1;
}

/**
 * Adds the completion medal.
 */
static int _ADD_YARIKOMI_MEDAL(RS_STACKDATA *stack, int argc) {
    CUserDataManager *userData;
    CSaveData *save = GetSaveData();

    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    userData->AddYarikomiMedal(GetStackInt(stack));
    return 1;
}

/**
 * Sets the map effect id.
 */
static int _SET_MAP_EFFECT_ID(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *info = &EventScene->battle_area;
    if (info == NULL) {
        return 0;
    }
    info->map_effect_id = GetStackInt(stack);
    return 1;
}

/**
 * Returns the map effect id.
 */
static int _GET_MAP_EFFECT_ID(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *info = &EventScene->battle_area;
    if (info == NULL) {
        return 0;
    }
    switch (argc) {
        case 1:
            SetStack(stack, info->map_effect_id);
        return 1;
    }
    return 0;
}

/**
 * Initializes the dungeon floor.
 */
static int _DNG_FLOOR_INIT(RS_STACKDATA *stack, int argc) {
    DungeonFloorInit();
    return 1;
}

/**
 * Invokes the inert dungeon floor-finish hook and returns success.
 */
static int _DNG_FLOOR_FINISH(RS_STACKDATA *stack, int argc) {
    DungeonFloorFinish();
    return 1;
}

/**
 * Clears the rnd stone.
 */
static int _CLEAR_RND_STONE(RS_STACKDATA *stack, int argc) {
    AutoMapGen.ClearRandomStone();
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_FLOOR_STATUS__FP12RS_STACKDATAi);
/**
 * Sets the floor status.
 */
static int _SET_FLOOR_STATUS(RS_STACKDATA *stack, int argc) {
    DNG_BATTLE_AREA *info = &EventScene->battle_area;
    RS_STACKDATA *args = stack;
    int mask;

    if (info == NULL) {
        return 0;
    }
    mask = GetStackInt(args++);
    if (GetStackInt(args) != 0) {
        info->floor_status |= mask;
    } else {
        info->floor_status &= ~mask;
    }
    return 1;
}

/**
 * Returns the map-generator attribute status at a script position.
 */
#ifdef NONMATCHING
static int _AMG_GET_ATTR_STATUS(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR position;

    if (argc != 4) {
        return 0;
    }
    GetStackVector(position, stack);

    stack += 3;
    SetStack(stack, AutoMapGen.GetAttrStatus(position));
    return 1;
}
#else
INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _AMG_GET_ATTR_STATUS__FP12RS_STACKDATAi);
#endif

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _SET_NEAR_DIST__FP12RS_STACKDATAi);
/**
 * Sets the keep time.
 */
static int _SET_KEEP_TIME(RS_STACKDATA *stack, int argc) {
    EdEventInfo.keep_time = GetStackFloat(stack);
    return 1;
}

/**
 * Returns the keep time.
 */
static int _GET_KEEP_TIME(RS_STACKDATA *stack, int argc) {
    if (argc != 1) {
        return 0;
    }
    SetStack(stack, EdEventInfo.keep_time);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _GET_DOOR_PARTS_ID__FP12RS_STACKDATAi);
/**
 * Checks the equep change.
 */
static int _CHECK_EQUEP_CHANGE(RS_STACKDATA *stack, int argc) {
    GetStackInt(stack);
    CheckEquipChange(1);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _ADD_HP_RATE2__FP12RS_STACKDATAi);
/**
 * Clears the dungeon effect all.
 */
static int _DNG_EFFECT_ALL_CLEAR(RS_STACKDATA *stack, int argc) {
    int i;

    RocketLauncher.Clear();
    for (i = 0; i < 16; i++) {
        MachineGun.active[i] = 0;
        MachineGun.col_prim_id[i] = -1;
    }
    MachineGun.index = 0;
    LaserGun.Clear();
    FxScriptMan->AllClearEffSpt();
    return 1;
}

/**
 * Sets whether scene music volume follows time-of-day lighting.
 */
static int _AUTO_CHENGE_BGM_VOL(RS_STACKDATA *stack, int argc) {
    EventScene->AutoChangeBGMVol(GetStackInt(stack));
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _UDATA_GET_WHP__FP12RS_STACKDATAi);
/**
 * Adds the user data weapon health.
 */
static int _UDATA_ADD_WHP(RS_STACKDATA *stack, int argc) {
    int charaNo;
    RS_STACKDATA *args = stack;
    CUserDataManager *userData;
    CSaveData *save;
    int itemNo;
    int amount;

    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(args++);
    itemNo = GetStackInt(args++);
    amount = GetStackInt(args);
    userData->AddWhp(charaNo, itemNo, amount);
    return 1;
}

INCLUDE_ASM("ps2/asm/pal/nonmatchings/event_func", _UDATA_GET_ABS__FP12RS_STACKDATAi);
/**
 * Adds the user data weapon experience.
 */
static int _UDATA_ADD_ABS(RS_STACKDATA *stack, int argc) {
    int charaNo;
    RS_STACKDATA *args = stack;
    CUserDataManager *userData;
    CSaveData *save;
    int itemNo;
    int amount;

    save = GetSaveData();
    if (save == NULL) {
        return 0;
    }
    userData = &save->user_data;
    if (userData == NULL) {
        return 0;
    }
    charaNo = GetStackInt(args++);
    itemNo = GetStackInt(args++);
    amount = GetStackInt(args);
    userData->AddAbs(charaNo, itemNo, amount);
    return 1;
}

/**
 * Creates the dungeon effect.
 */
static int _DNG_CREATE_EFFECT(RS_STACKDATA *stack, int argc) {
    sceVu0FVECTOR first;
    sceVu0FVECTOR second;
    int value;

    FxScriptMan->CreateEffSpt(GetStackString(stack++), -1, 0);
    switch (argc) {
        case 4:
            GetStackVector(first, stack);
            stack += 3;
            FxScriptMan->SetScriptVect1(first, -1, -1);
        case 5:
            GetStackVector(first, stack);
            value = GetStackInt(stack += 3);
            FxScriptMan->SetScriptVect1(first, -1, -1);
            FxScriptMan->SetValue(0, value, -1, -1);
            break;
        case 7:
            GetStackVector(first, stack);
            stack += 3;
            GetStackVector(second, stack);
            FxScriptMan->SetScriptVect1(first, -1, -1);
            FxScriptMan->SetScriptVect2(second, -1, -1);
            break;
        default:
        return 0;
    }
    return 1;
}

/**
 * Leaves the monica item check.
 */
static int _LEAVE_MONICA_ITEM_CHECK(RS_STACKDATA *stack, int argc) {
    LeaveMonicaItemCheck();
    return 1;
}

/**
 * Pauses the enable flag.
 */
static int _PAUSE_ENABLE_FLAG(RS_STACKDATA *stack, int argc) {
    PauseEnable(GetStackInt(stack));
    return 1;
}

/**
 * Forces the boot tour.
 */
static int _FORCE_BOOT_TOUR(RS_STACKDATA *stack, int argc) {
    CSaveData *saveData = GetSaveData();

    if (saveData == NULL) {
        return 0;
    }
    saveData->ForceBootTour(saveData->day, 1);
    return 1;
}

void SetEventFunc(CRunScript *script) {
    int i;
    int j;

    for (i = 0; i < event_func_slots; i++) {
        ext_func[i] = NULL;
    }
    i = 0;
    for (;;) {
        if (ext_func_info__2[i].func == NULL) {
            break;
        }
        for (j = 0; j < i; j++) {
            if (ext_func_info__2[i].no == ext_func_info__2[j].no) {
                printf("same ext_func_no!!!\n");
                while (1) {
                }
            }
        }
        if (ext_func_info__2[i].no < 0 || ext_func_info__2[i].no >= event_func_slots) {
            printf("ext func over!!");
        } else {
            ext_func[ext_func_info__2[i].no] = ext_func_info__2[i].func;
        }
        i++;
    }
    script->ext_func(ext_func, event_func_slots);
}


// Static initialiser (.init)


// Initialised data (.data)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1084__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", esa_ext_func_info__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3242__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", vv_3333__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3339__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4517__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6800__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", ext_func_info__2__DATA);

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1080__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1081__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1082__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1083__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1103__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1104__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1245__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1246__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1333__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1346__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1357__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1760__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1761__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1904__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1905__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1906__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1907__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1908__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1910__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_1909__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2245__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2246__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2247__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2248__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2249__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2291__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2292__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2333__4__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2334__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2393__3__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2664__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2836__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2837__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2838__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_2839__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3328__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3329__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3631__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3632__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3633__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3634__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3635__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3636__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3822__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3823__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_3884__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4072__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4261__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4262__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4263__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4264__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4265__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4266__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4267__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4268__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4269__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4270__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4271__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4274__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4273__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4272__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4291__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4360__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4437__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_4573__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5262__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5263__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5264__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5410__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5411__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5412__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5413__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5414__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5415__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5416__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5417__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5418__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5419__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5420__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5421__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5422__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5424__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5726__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_5736__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6703__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6773__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6774__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6775__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6776__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6781__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6782__2__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6816__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6834__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_6839__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_7117__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8230__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8406__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8458__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8480__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8902__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8903__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_8904__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_9148__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_9622__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_9744__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_9745__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_10100__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/event_func", at_10101__DATA);

// Static initialiser table (.ctor)


// Small uninitialised data (.sbss)
INCLUDE_BSS(EventMarker, 0x4);
INCLUDE_BSS(SwordEffect, 0x4);
INCLUDE_BSS(EventEffectScript, 0x4);
INCLUDE_BSS(p_use_item, 0x4);
INCLUDE_BSS(SetWorldCoordFlg, 0x4);
INCLUDE_BSS(PakuAnimEohNo, 0x4);
INCLUDE_BSS(PakuMotionEohNo, 0x4);
INCLUDE_BSS(PakuMotionType, 0x4);
INCLUDE_BSS(PakuMotionType2, 0x4);
INCLUDE_BSS(nowScriptArg, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(EdEventInfo, 0x12A0);
INCLUDE_BSS(EventObjHandleMother, 0x200);
INCLUDE_BSS(esMother, 0x440);
INCLUDE_BSS(EventLocalFlag, 0x100);
INCLUDE_BSS(EventLocalCnt, 0x100);
INCLUDE_BSS(EventRain, 0xABF0);
INCLUDE_BSS(Hit_para, 0x6400);
INCLUDE_BSS(HitEffect, 0x1E0);
INCLUDE_BSS(PakuAnimName, 0x40);
INCLUDE_BSS(PakuAnimName2, 0x40);
INCLUDE_BSS(PakuMotionName, 0x40);
INCLUDE_BSS(PakuMotionName2, 0x40);
INCLUDE_BSS(event_snd_buff, 0x8010);
INCLUDE_BSS(BuffEventSnd, 0x30);
INCLUDE_BSS(event_snd2_buff, 0x1410);
INCLUDE_BSS(BuffEventSnd2, 0x30);
INCLUDE_BSS(EventDngMap, 0x110);
INCLUDE_BSS(cmr_seq_tbl, 0x6000);
INCLUDE_BSS(CameraSeq, 0xB10);
INCLUDE_BSS(obj_seq_tbl, 0x5000);
INCLUDE_BSS(ObjectSeq, 0xBE00);
INCLUDE_BSS(EventSprite2, 0x1800);
INCLUDE_BSS(EventScriptArg, 0x10);
INCLUDE_BSS(EventScreenEffect, 0x50);
INCLUDE_BSS(ext_func__2, 0x1770);
