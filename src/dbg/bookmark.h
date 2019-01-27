#ifndef _BOOKMARK_H
#define _BOOKMARK_H

#include "_global.h"
#include "addrinfo.h"

struct BOOKMARKSINFO : AddrInfo
{
};

bool BookmarkSet(duint Address, bool Manual);
bool BookmarkGet(duint Address);
bool BookmarkDelete(duint Address);
void BookmarkDelRange(duint Start, duint End, bool Manual);
void BookmarkCacheSave(rapidjson::Document & Root);
void BookmarkCacheLoad(rapidjson::Document & Root);
bool BookmarkEnum(BOOKMARKSINFO* List, size_t* Size);
void BookmarkClear();
void BookmarkGetList(std::vector<BOOKMARKSINFO> & list);
bool BookmarkGetInfo(duint Address, BOOKMARKSINFO* info);

#endif // _BOOKMARK_H