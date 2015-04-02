#ifndef _COMMENT_H
#define _COMMENT_H

#include "_global.h"

struct COMMENTSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_COMMENT_SIZE];
    bool manual;
};

bool commentset(uint addr, const char* text, bool manual);
bool commentget(uint addr, char* text);
bool commentdel(uint addr);
void commentdelrange(uint start, uint end);
void commentcachesave(JSON root);
void commentcacheload(JSON root);
bool commentenum(COMMENTSINFO* commentlist, size_t* cbsize);
void commentclear();

#endif //_COMMENT_H