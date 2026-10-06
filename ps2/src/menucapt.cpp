#include "common.h"
#include "menucapt.hpp"
#include "mg_memory.hpp"
#include "mg_texture.hpp"
#include "mg_drawprim.hpp"
#include "mg_tanime.hpp"
#include "mglib.hpp"
#include "menucommon.hpp"
#include "menudraw.hpp"
#include "menumain.hpp"
#include "mainloop.hpp"
#include "scenesnd.hpp"
#include "sound.hpp"
#include "snd_mngr.hpp"
#include "dataread.hpp"

#include <cstdio>
#include <cstring>

static int MenuChapterMode;
static MENU_CHAPTER_INFO *MenuChapterInfo;
static mgCTexture *MenuChapterBG;
static mgCTexture *MenuChapter_Logo;
static unsigned int MenuChapterSnd_ID;
static int menu_snd_counter;
static int menu_chap_error_check_cnt;
static mgCMemory MenuChapterStack;

// Code (.text)
void MenuChapterInit(mgCMemory *stack, int *tex_block, int open_type, int chapter) {
    static char *chapter_voice[8] = {
        (char *)"0060600.wav", (char *)"2070310.wav", (char *)"3060260.wav", (char *)"4020120.wav",
        (char *)"5000010.wav", (char *)"6000360.wav", (char *)"7000010.wav", (char *)"8000140.wav"
    };
    char image_path[96];
    u_int file_size;
    int remaining = stack->stGetRest();
    u_long128 *buffer = stack->stack + stack->stack_used;
    MenuChapterStack.stSetBuffer(buffer, remaining);
    MenuChapterInfo = (MENU_CHAPTER_INFO *)MenuChapterStack.Alloc(2);
    MenuChapterInfo->tex_block[0] = tex_block[0];
    MenuChapterInfo->tex_block[1] = tex_block[1];
    MenuChapterInfo->logo_alpha = 0.0f;
    sprintf(image_path, "chap%d.img", chapter);
    MenuChapterStack.Align64();
    u_long128 *image_buffer = MenuChapterStack.stack + MenuChapterStack.stack_used;
    file_size = LoadFileMenu(image_path, image_buffer, 1);
    if ((int)file_size <= 0) {
        file_size = LoadFileMenu((char *)"chap0.img", image_buffer, 1);
    }
    u_int blocks;
    if (file_size & 0xF) {
        blocks = (file_size >> 4) + 1;
    } else {
        blocks = file_size >> 4;
    }
    MenuChapterStack.Alloc(blocks);
    mgTexManager.EnterIMGFile((unsigned char *)image_buffer, MenuChapterInfo->tex_block[0], 0, 0);
    MenuChapterBG = mgTexManager.GetTexture("chapbg", -1);
    MenuChapter_Logo = mgTexManager.GetTexture("chaplogo", -1);

    mgCMemory sound_memory;
    char voice_path[140];
    sound_memory.stSetBuffer(MenuChapterStack.stack + MenuChapterStack.stack_used, 0x280);
    MenuChapterStack.Alloc(0x280);
    MenuChapterStack.Align64();
    menu_snd_counter = 0;
    unsigned int *sound_buffer = (unsigned int *)(MenuChapterStack.stack + MenuChapterStack.stack_used);
    LoadFile2((char *)"snd2/sp/SP_007.snd", sound_buffer, (int *)&file_size, 0);
    if (file_size & 0xF) {
        blocks = (file_size >> 4) + 1;
    } else {
        blocks = file_size >> 4;
    }
    MenuChapterStack.Alloc(blocks);
    sndInitPort(8);
    MenuChapterSnd_ID = sndLoadSound(8, sound_buffer, &sound_memory);
    strcpy(voice_path, chapter_voice[chapter]);
    CSnd.StreamOpenFast(1, voice_path);
    if (CSnd.StreamOpenState() != 0) {
        while (CSnd.StreamOpenState() != 0) {}
    }
    CSnd.StreamStandBy(1);
    if (CSnd.StreamOpenState() != 0) {
        while (CSnd.StreamOpenState() != 0) {}
    }
    MenuChapterMode = MENU_CHAPTER_MODE_FADE_IN;
    MenuMainScene->fade.FadeIn(30);
}

int MenuChapterKey() {
    int fade_done;
    int stream_state;
    int finished = 0;
    CFadeInOut *fade;

    static int wait_count = 0;
    static int voice_done = 0;
    fade = &MenuMainScene->fade;
    fade_done = fade->FadeCheck();
    switch (MenuChapterMode) {
        case MENU_CHAPTER_MODE_FADE_IN:
            if (fade_done != 0) {
                ++menu_snd_counter;
                if (menu_snd_counter == 2) {
                    CSnd.StreamSetVol(1, 32767, 32767);
                    CSnd.StreamPlay(1);
                    wait_count = 0;
                }

                if (CalcMenuAdd(&MenuChapterInfo->logo_alpha, 3.0f, 128.0f) != 0) {
                    MenuChapterMode = MENU_CHAPTER_MODE_SHOW;
                    MenuChapterInfo->show_cnt = 0;
                    menu_snd_counter = 0;
                    menu_chap_error_check_cnt = 0;
                    voice_done = 0;
                }
            }
            break;
        case MENU_CHAPTER_MODE_SHOW:
            ++MenuChapterInfo->show_cnt;
            ++menu_chap_error_check_cnt;
            stream_state = CSnd.StreamGetState(1);
            if (stream_state == 0x8000 || menu_chap_error_check_cnt > 1500) {
                voice_done = 1;
            }
            if ((voice_done != 0) && (stream_state == 0)) {
                if (menu_snd_counter == 0) {
                    CSnd.StreamStop(1);
                    CSnd.StreamClose(1);
                }
                ++menu_snd_counter;
            }
            if (menu_snd_counter == 36) {
                sndSePlay(MenuChapterSnd_ID, 0, 0);
            }
            if ((MenuChapterInfo->show_cnt > 300) &&
                (menu_snd_counter >= 346)) {
                MenuMainScene->fade.FadeOut(60, 0.0f, 0.0f, 0.0f);
                MenuChapterMode = MENU_CHAPTER_MODE_FADE_OUT;
            }
            break;
        case MENU_CHAPTER_MODE_FADE_OUT:
            if (fade_done != 0) {
                finished = 1;
            }
            break;
    }
    return finished;
}

void MenuChapterDraw() {
    mgTexManager.ReloadTexture(MenuChapterInfo->tex_block[0], (sceVif1Packet *)0);
    DrawMenuFillBox(128, 0, 0, 0);
    mgCDrawPrim prim;
    mgRect<int> screen;
    mgRect<int> source;
    mgRect<int> title;
    prim.offset_x = 0;
    prim.offset_y = 0;
    SetSpriteEnv(&prim, 0);
    prim.Begin(MG_PRIM_SPRITE);
    if (MenuChapterBG != 0) {
        prim.Texture(MenuChapterBG);
        prim.Color(128, 128, 128, 128);
        source.Set(0, 0, 512, 448);
        screen.Set(0, 0, 512, mgScreenHeight);
        PrimQuad(&prim, screen, source);
    }
    if (MenuChapter_Logo != 0) {
        prim.Texture(MenuChapter_Logo);
        prim.Color(128, 128, 128, (int)MenuChapterInfo->logo_alpha);
        mgRect<int> title(0, 0, 512, 64);
        PrimQuad(&prim, 0.0f, (float)mgScreenHeight / 2.0f - 32.0f - 12.0f, title);
        prim.Color(128, 128, 128, 128);
        mgRect<int> overlay(0, 64, 512, 64);
        PrimQuad(&prim, 0.0f, 0.0f, overlay);
    }
    prim.End();
}

// Static initialiser (.init)

// Initialised data (.data)

// Constants (.rodata)

// Static initialiser table (.ctor)

// Small uninitialised data (.sbss)

// Uninitialised data (.bss)
