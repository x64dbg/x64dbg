#ifndef _BOOKMARK_H
#define _BOOKMARK_H

#include "_global.h"

struct BOOKMARKSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    bool manual;
};

bool bookmarkset(uint addr, bool manual);
bool bookmarkget(uint Address);
bool bookmarkdel(uint addr);
void bookmarkdelrange(uint start, uint end);
void bookmarkcachesave(JSON root);
void bookmarkcacheload(JSON root);
bool bookmarkenum(BOOKMARKSINFO* bookmarklist, size_t* cbsize);
void bookmarkclear();

#endif //_BOOKMARK_H