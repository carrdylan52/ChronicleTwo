#include "common.h"
#include "helpmes.hpp"
#include "mglib.hpp"
#include "mainloop.hpp"
#include "snd_mngr.hpp"
#include "nd_meswin.hpp"
#include "mg_texture.hpp"
#include "dataread.hpp"
#include "mg_memory.hpp"
#include <cstdio>
#include <cstring>

static ClsMes        HelpMes;              /**< Help message window. */
static HELP_MES_INFO HelpMesInfo;          /**< Request displayed by the help window. */
static int           ShowOffOnce;          /**< Non-zero to skip drawing the window once. */
static int           WindowMode;           /**< Style requested for the help window. */
static char          HelpMesBuff[0x1000];   /**< Loaded help message text. */
static int           InitFlag;             /**< Non-zero once help message text has loaded. */

// Code (.text)
void LoadHelpMes(u_long128 *buffer) {
    char path[76];
    int size;

    sprintf(path, "etc/help%d.mes", LanguageCode);
    if (LoadFile2(path, buffer, &size, 0) != 0) {
        if (size > (int)sizeof(HelpMesBuff)) {
            printf("HMes Buffer Over!!(%d/%dbyte)", size, sizeof(HelpMesBuff));
            return;
        }
        memcpy(HelpMesBuff, buffer, size);
        InitFlag = 1;
    }
}

/**
 * Returns the request for the help message window.
 */
static HELP_MES_INFO *GetHepMesInfo() {
    return &HelpMesInfo;
}

void CreateHelpMes(int tex_no) {
    if (!InitFlag) {
        return;
    }
    HelpMes.Init();
    HelpMes.Preset(MES_WIN_VERSATILE_1);
    HelpMes.SetWindowMode(MES_WIN_NONE);
    HelpMes.SetBuff((short *)HelpMesBuff);
    HelpMes.texture_block = tex_no;
    ShowOffOnce = 0;
    HelpMesInfo.time = 0;
    HelpMesInfo.mes_no = -1;
    HelpMesInfo.fukidashi_pos = -1;
    HelpMesInfo.show = 0;
    HelpMesInfo.y = 0;
    HelpMesInfo.x = 0;
    HelpMesInfo.created = 0;
}

void StepHelpMes() {
    ClsMes *message = &HelpMes;
    HELP_MES_INFO *info = GetHepMesInfo();
    if (info != NULL) {
        int hidden = !info->show;
        if (hidden) {
            return;
        }
    } else {
        return;
    }
    if (!info->created) {
        message->Preset(4);
        message->SetWindowMode(WindowMode);
        message->MakeMesWin(info->mes_no);
        message->fade_speed = 1.0f;
        if (info->fukidashi_pos < 0) {
            message->abs_win.x = info->x;
            message->abs_win.y = info->y;
        } else {
            message->fukidashi_pos = info->fukidashi_pos;
        }
        info->created = 1;
    }
    message->Step();
    if (info->time > 0) {
        info->time--;
        if (info->time == 0) {
            info->time = 0;
            info->mes_no = -1;
            info->show = 0;
            info->y = 0;
            info->x = 0;
            info->created = 0;
            info->fukidashi_pos = -1;
        }
    }
}

void ShowOffOnceHelpMes() {
    ShowOffOnce = 1;
}

void DrawHelpMes() {
    if (DebugInfo.param_off != 0) {
        return;
    }
    ClsMes *message = &HelpMes;
    HELP_MES_INFO *info = GetHepMesInfo();
    if (info != NULL) {
        int hidden = !info->show;
        if (hidden) {
            return;
        }
    } else {
        return;
    }
    if (ShowOffOnce != 0) {
        ShowOffOnce = 0;
        return;
    }
    mgTexManager.ReloadTexture(HelpMes.texture_block, (sceVif1Packet *)NULL);
    message->DrawMesWin();
}

void ShowHelpMes(int mes_no, int time) {
    HELP_MES_INFO *info = GetHepMesInfo();
    if (info == NULL) {
        return;
    }
    if (info->mes_no != mes_no) {
        info->time = 0;
        info->mes_no = -1;
        info->show = 0;
        info->y = 0;
        info->x = 0;
        info->created = 0;
        info->fukidashi_pos = -1;
    }
    info->show = 1;
    info->mes_no = mes_no;
    info->time = time > 0 ? time + 1 : time;
    info->x = 18;
    info->y = mgScreenHeight - 31;
    info->fukidashi_pos = -1;
    WindowMode = 0;
}

void ShowErrorHelpMes(int mes_no, int time) {
    HELP_MES_INFO *info = GetHepMesInfo();
    if (info == NULL) {
        return;
    }
    if (info->mes_no != mes_no) {
        info->time = 0;
        info->mes_no = -1;
        info->show = 0;
        info->y = 0;
        info->x = 0;
        info->created = 0;
        info->fukidashi_pos = -1;
    }
    info->show = 1;
    info->mes_no = mes_no;
    info->time = time > 0 ? time + 1 : time;
    info->fukidashi_pos = 8;
    WindowMode = 4;
    sndSePlay(GetSystemSndID(), 28, 0);
}

// Constants (.rodata)
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/helpmes", at_799__6__DATA);
INCLUDE_RODATA("ps2/asm/pal/nonmatchings/helpmes", at_800__5__DATA);

// Small uninitialised data (.sbss)
INCLUDE_BSS(InitFlag__2, 0x4);
INCLUDE_BSS(WindowMode, 0x4);
INCLUDE_BSS(ShowOffOnce, 0x4);

// Uninitialised data (.bss)
INCLUDE_BSS(HelpMesBuff, 0x1000);
INCLUDE_BSS(HelpMes, 0x295C);
INCLUDE_BSS(D_01F628BC, 0x4);
INCLUDE_BSS(HelpMesInfo, 0x20);
