#ifndef _SYMBOLINFO_H
#define _SYMBOLINFO_H

#include "_global.h"
#include "addrinfo.h"

void symbolloadmodule(MODINFO* modinfo);
void symbolunloadmodule(uint base);
void symbolclear();
void symbolupdategui();

#endif //_SYMBOLINFO_H
