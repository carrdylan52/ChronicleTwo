#include "common.h"
#include <cstring>
#include <cstdio>
#include "charasetup.hpp"
#include "actionchara.hpp"
#include "colprim.hpp"
#include "character.hpp"
#include "dng_main.hpp"
#include "event.hpp"
#include "event_func.hpp"
#include "mainloop.hpp"
#include "maintex.hpp"
#include "menuchr.hpp"
#include "menucommon.hpp"
#include "menumain.hpp"
#include "menusys.hpp"
#include "mg_math.hpp"
#include "mg_memory.hpp"
#include "mglib.hpp"
#include "scene.hpp"
#include "sceneload.hpp"
#include "scenesnd.hpp"
#include "savedata.hpp"
#include "runscript.hpp"
#include "userdata.hpp"
#include "stubs/character.hpp"
#include "stubs/menuchr.hpp"
#include "stubs/menusys.hpp"
#include "stubs/event_func.hpp"
#include "ixion_toan_body.hpp"
#include "ixion_toan_combat.hpp"

// Same pointer-only record as menusys.cpp; the stub binds the original storage.
struct MenuCharaReadBuffers { u_int *model; u_int *skin; u_int *outline; };

void PortRetailInitEyeCamera(CActionChara *) asm("Toan_RetailInitEyeCamera");
int PortRetailDngMainKey() asm("Toan_RetailDngMainKey");
int PortToanEventStackInt(RS_STACKDATA *) asm("Toan_EventStackInt");
char *PortToanEventStackString(RS_STACKDATA *) asm("Toan_EventStackString");
ARG_DATA *PortToanFindArgData(int, int &) asm("Toan_FindArgData");

// Keep native preview loading; its update_scene branch must not reset the donor's arenas.
PORT_OVERRIDE int MenuItemCharaDataLoadEndCheck(MENU_BGREAD_INFO2 **info, mgCMemory *stack,
                                               CActionChara **chara, int chara_no, int tex, int scene_tex) {
    CCharacter2 *body = MenuMainScene != nullptr ? MenuMainScene->GetCharacter(0) : nullptr;
    const bool protect = MenuLoadInfo.mode == MENU_LOAD_MODE_MENU && MenuLoadInfo.update_scene &&
                         IxionToanBodyOwns(body);
    const auto update = MenuLoadInfo.update_scene;
    if (protect) {
        IxionToanCombatRetire(body, "equipment-menu");
        MenuLoadInfo.update_scene = false;
    }
    const int result = PortRetailMenuItemCharaDataLoadEndCheck(info, stack, chara, chara_no, tex, scene_tex);
    MenuLoadInfo.update_scene = update;
    if (protect && result != 0) {
        // A new weapon/rig lifetime is explicit even when the protected body keys did not change.
        IxionToanBodyRegister(body, IxionToanBodyDagger(body));
    }
    return result;
}

// Called by the R2 path before its early return, including neutral first-person input.
PORT_OVERRIDE void InitEyeCamera(CActionChara *chara) {
    IxionToanCombatSuspend(chara, "first-person");
    PortRetailInitEyeCamera(chara);
}

PORT_OVERRIDE int DngMainKey() {
    const bool was_eye_view = DngStatus.eye_view;
    const int result = PortRetailDngMainKey();
    if (was_eye_view || DngStatus.mode != DNG_STATUS_FIELD || DngStatus.eye_view ||
        (DngMainScene != nullptr && DngMainScene->event_run) ||
        (BattleAreaScene != nullptr && (BattleAreaScene->pause_flag & DNG_PAUSE_PLAYER_CONTROL))) {
        IxionToanCombatSuspend(nullptr, "control-suspended");
    }
    return result;
}

// Verbatim retail bodies below, with one ownership/retirement seam each.
PORT_OVERRIDE void CCharacter2::LoadSkin(u32 *pack_file, char *info_name, char *skin_name, mgCMemory *memory,
                           int texture_block) {
    if (IxionToanBodyOwns(this)) return; // Ixion: native skeletons cannot merge into a donor.
    ScanInfoSkinFile(this, pack_file, info_name, skin_name, memory, texture_block);
}

PORT_OVERRIDE void CCharacter2::Initialize() {
    IxionToanBodyRetire(this); // Ixion: close combat before any frame/key storage is retired.
    int j;
    int i;

    CObjectFrame::Initialize();
    velocity[2] = 0.0f;
    velocity[1] = 0.0f;
    velocity[0] = 0.0f;
    velocity[3] = 1.0f;
    base_scale[3] = 1.0f;
    base_scale[2] = 1.0f;
    base_scale[1] = 1.0f;
    base_scale[0] = 1.0f;
    move_accel = 0.0f;
    alpha = 1.0f;
    mgUnitMatrix(entry_matrix);
    CObjectFrame::frame = NULL;
    load_size = 0;
    copy_size = 0;
    shadow_poly_num = 0;
    poly_num = 0;
    dynamic_anime_flags = 0;
    outline = NULL;
    shadow_frame = NULL;
    frame = 0;
    main_frame_info = NULL;
    motion_status = CHARA_MOTION_STATUS_NONE;
    shape_anime = 0;
    sound_info.foot_sound_id = -1;
    sound_info.foot_sound_enable = true;
    sound_info.se_positional = CHARA_SE_POSITIONAL_HEAD;
    sound_info.se_volume = 1.0f;
    sound_info.se_pan = 0.0f;
    sound_info.foot_effect_wait = -1;
    sound_info.loop_se = NULL;

    for (i = 0; i < CHARA_MOTION_SET_MAX; i++) {
        se_list[i] = NULL;
        se_num[i] = 0;
    }

    sword_effect[0] = NULL;
    sword_effect[1] = NULL;
    sword_effect[2] = NULL;
    dynamic_anime_num = 0;
    dynamic_anime = NULL;

    for (i = 0; i < CHARA_IMAGE_MAX; i++) {
        images[i] = NULL;
    }

    tex_anime_group_num = 0;
    tex_anime_group_start = 0;
    entry_frame[0] = NULL;
    entry_frame[1] = NULL;

    for (i = 0; i < CHARA_ENTRY_OBJECT_MAX; i++) {
        entry_object[i].frame = NULL;
        entry_object[i].size = 0.0f;
        entry_object[i].group = -1;
        entry_object[i].enable = false;
    }

    for (j = 0; j < CHARA_DEFORM_FRAME_MAX; j++) {
        deform_frame[j] = NULL;
    }

    deform_frame_num = 0;

    for (i = 0; i < CHARA_MOTION_SET_MAX; i++) {
        key_list[i] = NULL;
        key_num[i] = 0;
        seq_list[i] = NULL;
    }

    memset(motion, 0, sizeof(motion));
    memset(shadow_motion, 0, sizeof(shadow_motion));
    now_set = 0;
    next_set = 0;
    next_key = NULL;
    now_key = NULL;
    posed_key = NULL;
    next_seq = NULL;
    now_seq = NULL;
    seq_step = NULL;
    main_frame_info = NULL;
    shadow_frame_info = NULL;
    lod_num = 0;
    lod = NULL;
    lod_no = -1;
    motion_enable = true;
    shadow_link.num = 0;
    shadow_link.dst_frame = NULL;
    shadow_link.src_frame = NULL;
    InitEffect();
}

PORT_OVERRIDE CColPrim *CColPrimMan::GetID2Prim(int id) {
    if (id < 0 || id >= COLPRIM_MAX) {
        return NULL;
    }

    IxionToanCombatPrimInvalidated(&prim[id]);
    prim[id].id = id;
    return &prim[id];
}

PORT_OVERRIDE int CScene::LoadChara(int index, u_int *pack, char *name, mgCMemory *model_stack, mgCMemory *motion_stack, mgCMemory *image_stack, int image_block, int no_outline) {
    if (IxionToanBodyOwns(GetCharacter(index))) return index; // Ixion: preserve the owned body/part.
    u_int       *files[1];
    int          sizes[1];
    CCharacter2 *chara = new (model_stack->Alloc(0x68)) CCharacter2;

    if (chara == NULL) {
        return -1;
    }

    DeleteChara(index);
    int slot = AssignChara(index, chara, "chara");

    if (slot < 0) {
        return -1;
    }

    CSceneCharacter *scene_chara = GetSceneCharacter(slot);

    if (scene_chara == NULL) {
        return -1;
    }

    scene_chara->status |= SCENE_DATA_LOADED;

    if (name == NULL) {
        GetPackFileExt(pack, "cfg", files, 1, sizes, &name);
    }

    chara->Initialize();

    if (no_outline != 0) {
        chara->LoadPackNoLine(pack, name, model_stack, motion_stack, image_stack, image_block, NULL);
    } else {
        chara->LoadPack(pack, name, model_stack, motion_stack, image_stack, image_block, NULL);
    }

    SetCharaTexb(slot, image_block);
    chara->sound_info.foot_se_bank = se_base_id;
    chara->sound_info.foot_sound_id = 0;
    return slot;
}

PORT_OVERRIDE void CScene::DeleteChara(int index) {
    IxionToanBodyRetire(GetCharacter(index));
    CSceneCharacter *chara;

    chara = GetSceneCharacter(index);

    if (chara != NULL) {
        chara->Initialize();
    }
}

PORT_OVERRIDE void CSceneCharacter::Initialize() {
    // Slot identity also works during construction, without reading an uninitialized chara member.
    IxionToanBodyRetireSceneSlot(this);
    chara = 0;
    texb = -1;
    chara_no = -1;
    CSceneData::Initialize();
}

PORT_OVERRIDE int _LOAD_CHARA_sub(int stack_no, char **name, int chara_no, u32 *pack, int mode) {
    if (IxionToanBodyOwns(EventScene->GetCharacter(chara_no))) return chara_no;
    u32               *files[0x80];
    int                sizes[0x80];
    char               label[0x20];
    mgCMemory         *stack;
    int                tex_block;
    int                result;
    mgCTextureManager *manager;

    stack = EventScene->GetStack(stack_no);

    if (*name == NULL) {
        if (GetPackFileExt(pack, "cfg", files, 0x80, sizes, name) <= 0) {
            return 0;
        }
    }

    tex_block = EventScene->GetCharaTexb(chara_no);

    if (tex_block < 0) {
        return 0;
    }

    manager = &mgTexManager;
    manager->DeleteBlock(tex_block);
    sprintf(label, "ev%d", chara_no);

    if (chara_no >= 8) {
        strcpy(manager->name_suffix, label);
    }

    result = EventScene->LoadChara(chara_no, pack, *name, stack, stack, stack, tex_block, mode);

    if (chara_no >= 8) {
        manager->name_suffix[0] = 0;
    }

    EventScene->SetType(SCENE_DATA_CHARA, chara_no, SCENE_CHARA_TYPE_EVENT);
    return result;
}

PORT_OVERRIDE void CMenuItemInfo::ExitEnd() {
    CActionChara *field_chara;
    int           chara_no;
    CActionChara *chara;
    int           equip_list_no;
    mgCMemory    *stack;

    field_chara = (CActionChara *) MenuMainScene->GetCharacter(0);

    if (sound_loaded != 0) {
        MenuCharaSoundEnter(MenuMainScene, field_chara, 1);
    }

    // Ixion: the donor was preserved through menu loading. Native reload would replace its arenas.
    const bool donor_reload = chara_reload != 0 && IxionToanBodyOwns(field_chara);
    if (donor_reload) {
        field_chara->AllDeleteDamage();
        IxionToanCombatRetire(field_chara, "menu-exit-reload");
        IxionToanBodyRegister(field_chara, IxionToanBodyDagger(field_chara));
        field_chara->effect_man = FxScriptMan;
    }
    if (chara_reload != 0 && !donor_reload) {
        chara_no = GetActiveCharaNo();
        chara = (CActionChara *) MenuMainScene->GetCharacter(0);
        DeleteOutLineMenu(chara, 0);
        chara->DeleteImage();
        stack = MorattaStack;
        stack->stack_used = 0;
        stack->lock = 0;
        chara->AllDeleteDamage();
        chara->Initialize(MorattaStack);
        AccumulateEffect.frame = 0;
        AccumulateEffect.unk_320 = 0;
        AccumulateEffect.mode = 0;
        chara->accume_effect = &AccumulateEffect;
        chara->LoadPack(MainCharaReadBuffer.model, "info.cfg", MorattaStack, MorattaStack, MorattaStack,
                        MenuArg.chara_tex_block, 0);
        SwordEffectStack.stack_used = 0;
        SwordEffectStack.lock = 0;
        SetSwordBlurEffect(chara, &SwordEffectStack, chara_no);
        stack = MorattaStack;
        stack[1].stack_used = 0;
        stack[1].lock = 0;
        mgCTextureManager *textures = &mgTexManager;
        textures->DeleteTexAnime(MenuArg.chara_tex_block);
        chara->LoadSkin(MainCharaReadBuffer.skin, "info.cfg", "", &MorattaStack[1],
                       MenuArg.chara_tex_block);
        stack = MorattaStack;
        stack[5].stack_used = 0;
        stack[5].lock = 0;
        chara->LoadSkin(MainCharaReadBuffer.outline, "info.cfg", "skin3", &MorattaStack[5],
                       MenuArg.chara_tex_block);
        SetupUnitMan(MenuMainScene, GetUserDataMan(), chara_no, NULL);
        chara->effect_man = FxScriptMan;
    }

    equip_list_no = CheckEquipListNo(0);

    if ((equip_list_no != 0 || GetActiveCharaNo() == USER_CHARA_ROBO) && MenuMainScene != NULL &&
        GetMenuLoopType() == MENU_LOOP_DUNGEON) {
        if (field_chara != NULL) {
            if (equip_list_no != 0) {
                field_chara->InitScript();
            }
        }

        if (GetActiveCharaNo() == USER_CHARA_ROBO) {
            CharaSndBuffer = NULL;
            MenuCharaSoundEnter(MenuMainScene, field_chara, 0);
            field_chara->sound_info.se_bank = Robo_Sound_ID_Save;
            field_chara->effect_man = FxScriptMan;
        }
    }

    MenuSystemDataPtr->item_board_select = MenuItem_ItemBoardTopSelect;
    MenuSystemDataPtr->item_board_top = MenuItem_ItemBoardTopLine;
    MenuSystemDataPtr->unk_6 = 0;
    MenuSystemDataPtr->unk_4 = 0;
    MenuSystemDataPtr->item_key_arg_no = key_arg_no;
    MenuSystemDataPtr->item_cursor = (int) *(void **) &MenuCommonInfo->cursor;
    CopyActiveItemAndWeapon(MenuArg.active_chara_no, -1);
    ExeScript("EXITEND");
    MenuPosData->TexGetInfoClear(0x5A, 0x100);
    MenuPosData->EtcTblClear(0x1E, 0x60);
    MenuCommonInfo->have_item.Init();
    MenuMainFrameModeSet(MENU_MAIN_FRAME_OPEN, false);
}

PORT_OVERRIDE int _LOAD_EQUIP(RS_STACKDATA *stack, int argc) {
    char  label[0x20];
    char  bone_name[0x2C];
    char *name;
    int   stack_no;
    int   member_kind;
    int   equip_kind;
    int   chara_no;
    int   attach_no;

    switch (argc) {
        case 1: {
            ARG_DATA *args = PortToanFindArgData(PortToanEventStackInt(stack), argc);

            if (args == NULL) {
                return false;
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
            stack_no = PortToanEventStackInt(stack++);
            member_kind = PortToanEventStackInt(stack++);
            equip_kind = PortToanEventStackInt(stack++);
            name = PortToanEventStackString(stack++);
            chara_no = PortToanEventStackInt(stack++);

            if (argc >= 6) {
                attach_no = PortToanEventStackInt(stack);
            }

            break;
        default:
            return false;
    }

    if (member_kind < 0 || member_kind > 1) {
        return false;
    }

    if (equip_kind < 0 || equip_kind > 4) {
        return false;
    }

    // Ixion: refuse equipment attachment to the donor even when attach_no is an unowned new slot.
    if (IxionToanBodyOwns(GetCharacter(chara_no)) ||
        (equip_kind < 3 && argc >= 6 && IxionToanBodyOwns(GetCharacter(attach_no)))) return true;

    CUserDataManager *user_data = NULL;
    CSaveData        *save = GetSaveData();

    if (save == NULL) {
        return false;
    }

    user_data = &save->user_data;

    if (user_data == NULL) {
        return false;
    }

    char        *path;
    CCharacter2 *chara;
    int          tex_block;
    u32         *pack;

    if ((path = user_data->GetCharaEquipDataPath(member_kind, equip_kind)) == NULL) {
        return false;
    }

    if ((chara = GetCharacter(chara_no)) == NULL) {
        return false;
    }

    if (0 > (tex_block = EventScene->GetCharaTexb(chara_no))) {
        return false;
    }

    if ((pack = GetLoadBGBuff(path, NULL)) == NULL) {
        return false;
    }

    if (equip_kind >= 3 || equip_kind >= 4) {
        mgCMemory *memory;

        if ((memory = EventScene->GetStack(stack_no)) == NULL) {
            return false;
        }

        sprintf(label, "ev%d", chara_no);
        mgCTextureManager *manager = &mgTexManager;

        if (chara_no >= 8) {
            strcpy(manager->name_suffix, label);
        }

        chara->LoadSkin(pack, name, "", memory, tex_block);

        if (chara_no >= 8) {
            manager->name_suffix[0] = 0;
        }
    } else {
        if (_LOAD_CHARA_sub(stack_no, &name, attach_no, pack) <= 0) {
            return false;
        }

        if (member_kind == USER_CHARA_MAX) {
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
        } else if (member_kind == USER_CHARA_MONICA) {
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
            return false;
        }

        if (chara->CObjectFrame::frame == NULL) {
            return false;
        }

        mgCFrame *bone;

        if ((bone = chara->CObjectFrame::frame->SearchFrame(bone_name)) == NULL) {
            return false;
        }

        if (attached->CObjectFrame::frame == NULL) {
            return false;
        }

        attached->CObjectFrame::frame->SetReference(bone);
    }

    return true;
}
