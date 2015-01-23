#ifndef _LABEL_H
#define _LABEL_H

#include "_global.h"

struct LABELSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_LABEL_SIZE];
    bool manual;
};

bool labelset(uint addr, const char* text, bool manual);
bool labelfromstring(const char* text, uint* addr);
bool labelget(uint addr, char* text);
bool labeldel(uint addr);
void labeldelrange(uint start, uint end);
void labelcachesave(JSON root);
void labelcacheload(JSON root);
bool labelenum(LABELSINFO* labellist, size_t* cbsize);
void labelclear();

#endif //_LABEL_H