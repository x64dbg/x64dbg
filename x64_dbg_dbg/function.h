#ifndef _FUNCTION_H
#define _FUNCTION_H

#include "addrinfo.h"

struct FUNCTIONSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    bool manual;
};

bool functionadd(uint start, uint end, bool manual);
bool functionget(uint addr, uint* start, uint* end);
bool functionoverlaps(uint start, uint end);
bool functiondel(uint addr);
void functiondelrange(uint start, uint end);
void functioncachesave(JSON root);
void functioncacheload(JSON root);
bool functionenum(FUNCTIONSINFO* functionlist, size_t* cbsize);
void functionclear();

#endif //_FUNCTION_H