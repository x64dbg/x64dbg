#ifndef _ADDRINFO_H
#define _ADDRINFO_H

#include "_global.h"

void dbinit();
bool modnamefromaddr(uint addr, char* modname);
bool commentset(uint addr, const char* text);
bool commentget(uint addr, char* text);
bool commentdel(uint addr);
bool labelset(uint addr, const char* text);
bool labelget(uint addr, char* text);
bool labeldel(uint addr);

#endif // _ADDRINFO_H
