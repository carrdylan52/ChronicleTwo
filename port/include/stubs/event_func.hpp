#pragma once

// Retail binds event_func's VectMatMul under the __2 name; eventedit's copy keeps the plain one.
#define VectMatMul VectMatMul__2

#include <event_func.hpp>
#include <runscript.hpp>
int GetStackInt(RS_STACKDATA *) asm("Toan_EventStackInt");
char *GetStackString(RS_STACKDATA *) asm("Toan_EventStackString");
ARG_DATA *FindArgData(int, int &) asm("Toan_FindArgData") __attribute__((used));
int _LOAD_EQUIP(RS_STACKDATA *, int) asm("Toan_LoadEquip");
