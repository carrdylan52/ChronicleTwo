#pragma once
#include <dngmenu.hpp>
extern CDngFreeMap  *MenuDngMap asm("MenuMouse_DngMap");
extern CMenuTreeMap *CMenuTreePt asm("MenuMouse_Tree");
extern short         DngTreeMode asm("MenuMouse_TreeMode");

// The floor map's cull edges and line width; see header_fixups.UNIT_FIXUPS.
float PortDngMapVisibleRight();
float PortDngMapVisibleBottom();
void  mgPortPrimLogicalLines(mgCDrawPrim &prim);
