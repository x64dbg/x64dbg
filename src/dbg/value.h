#ifndef _VALUE_H
#define _VALUE_H

#include "_global.h"

//functions
bool valuesignedcalc();
void valuesetsignedcalc(bool a);
bool valapifromstring(const char* name, duint* value, int* value_size, bool printall, bool silent, bool* hexonly);
bool valfromstring_noexpr(const char* string, duint* value, bool silent = true, bool baseonly = false, int* value_size = 0, bool* isvar = 0, bool* hexonly = 0);
bool valfromstring(const char* string, duint* value, bool silent = true, bool baseonly = false, int* value_size = 0, bool* isvar = 0, bool* hexonly = 0);
bool valflagfromstring(duint eflags, const char* string);
bool valtostring(const char* string, duint value, bool silent);
bool valmxcsrflagfromstring(duint mxcsrflags, const char* string);
bool valx87statuswordflagfromstring(duint statusword, const char* string);
bool valx87controlwordflagfromstring(duint controlword, const char* string);
unsigned short valmxcsrfieldfromstring(duint mxcsrflags, const char* string);
unsigned short valx87statuswordfieldfromstring(duint statusword, const char* string);
unsigned short valx87controlwordfieldfromstring(duint controlword, const char* string);
duint valfileoffsettova(const char* modname, duint offset);
duint valvatofileoffset(duint va);
bool setregister(const char* string, duint value);
bool setflag(const char* string, bool set);

#endif // _VALUE_H
