#pragma once

// menumap.cpp:1438 writes SphidaMenuTexbk[15] of an int[8], into whatever the PS2 linker placed after it.
#pragma clang diagnostic ignored "-Warray-bounds"
