#pragma once

#include "_global.h"

struct BOOKMARKSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    bool manual;
};

bool BookmarkSet(uint Address, bool Manual);
bool BookmarkGet(uint Address);
bool BookmarkDelete(uint Address);
void BookmarkDelRange(uint Start, uint End);
void BookmarkCacheSave(JSON Root);
void BookmarkCacheLoad(JSON Root);
bool BookmarkEnum(BOOKMARKSINFO* List, size_t* Size);
void BookmarkClear();