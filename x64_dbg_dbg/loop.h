#ifndef _LOOP_H
#define _LOOP_H

#include "addrinfo.h"

struct LOOPSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint start;
    uint end;
    uint parent;
    int depth;
    bool manual;
};

bool loopadd(uint start, uint end, bool manual);
bool loopget(int depth, uint addr, uint* start, uint* end);
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth);
bool loopdel(int depth, uint addr);
void loopcachesave(JSON root);
void loopcacheload(JSON root);
bool loopenum(LOOPSINFO* looplist, size_t* cbsize);
void loopclear();

#endif //_LOOP_H