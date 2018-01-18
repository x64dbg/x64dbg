#ifndef _COMMENT_H
#define _COMMENT_H

#include "_global.h"
#include "addrinfo.h"

struct COMMENTSINFO : AddrInfo
{
    std::string text;
};

bool CommentSet(duint Address, const char* Text, bool Manual);
bool CommentGet(duint Address, char* Text);
bool CommentDelete(duint Address);
void CommentDelRange(duint Start, duint End, bool Manual);
void CommentCacheSave(JSON Root);
void CommentCacheLoad(JSON Root);
bool CommentEnum(COMMENTSINFO* List, size_t* Size);
void CommentClear();
void CommentGetList(std::vector<COMMENTSINFO> & list);
bool CommentGetInfo(duint Address, COMMENTSINFO* info);

#endif // _COMMENT_H