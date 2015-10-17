#pragma once

#include "_global.h"

struct COMMENTSINFO
{
    char mod[MAX_MODULE_SIZE];
    uint addr;
    char text[MAX_COMMENT_SIZE];
    bool manual;
};

bool CommentSet(uint Address, const char* Text, bool Manual);
bool CommentGet(uint Address, char* Text);
bool CommentDelete(uint Address);
void CommentDelRange(uint Start, uint End);
void CommentCacheSave(JSON Root);
void CommentCacheLoad(JSON Root);
bool CommentEnum(COMMENTSINFO* List, size_t* Size);
void CommentClear();