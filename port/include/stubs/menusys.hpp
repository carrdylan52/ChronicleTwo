#pragma once
#include <menusys.hpp>
extern CItemSelect *ItemSelectPtr asm("MenuMouse_ItemSelect");

extern s16 MenuItem_ItemBoardTopLine asm("Toan_MenuItemBoardTopLine");
extern s16 MenuItem_ItemBoardTopSelect asm("Toan_MenuItemBoardTopSelect");
struct MenuCharaReadBuffers;
extern MenuCharaReadBuffers MainCharaReadBuffer asm("Toan_MenuCharaReadBuffers");
extern int Robo_Sound_ID_Save asm("Toan_RoboSoundId");
