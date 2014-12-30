#ifndef _VALUE_H
#define _VALUE_H

#include "_global.h"

//functions
bool valuesignedcalc();
void valuesetsignedcalc(bool a);
bool valapifromstring(const char* name, uint* value, int* value_size, bool printall, bool silent, bool* hexonly);
bool valfromstring(const char* string, uint* value, bool silent = true, bool baseonly = false, int* value_size = 0, bool* isvar = 0, bool* hexonly = 0);
bool valflagfromstring(uint eflags, const char* string);
bool valtostring(const char* string, uint* value, bool silent);
bool valmxcsrflagfromstring(uint mxcsrflags, const char* string);
bool valx87statuswordflagfromstring(uint statusword, const char* string);
bool valx87controlwordflagfromstring(uint controlword, const char* string);
unsigned short valmxcsrfieldfromstring(uint mxcsrflags, const char* string);
unsigned short valx87statuswordfieldfromstring(uint statusword, const char* string);
unsigned short valx87controlwordfieldfromstring(uint controlword, const char* string);
uint valfileoffsettova(const char* modname, uint offset);
uint valvatofileoffset(uint va);

#endif // _VALUE_H
