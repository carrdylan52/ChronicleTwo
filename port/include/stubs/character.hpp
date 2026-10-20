#pragma once
#include <character.hpp>
void ScanInfoSkinFile(CCharacter2 *, u32 *, char *, char *, mgCMemory *, int) asm("Toan_RetailScanInfoSkinFile");
// Parser outputs also let production-path tests observe a valid native skin script.
extern int load_img_size asm("Toan_SkinImageSize");
