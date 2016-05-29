#ifndef _SCRIPTAPI_COMMENT_H
#define _SCRIPTAPI_COMMENT_H

#include "_scriptapi.h"

namespace Script
{
    namespace Comment
    {
        struct CommentInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rva;
            char text[MAX_LABEL_SIZE];
            bool manual;
        };

        SCRIPT_EXPORT bool Set(duint addr, const char* text, bool manual = false);
        SCRIPT_EXPORT bool Set(const CommentInfo* info);
        SCRIPT_EXPORT bool Get(duint addr, char* text); //text[MAX_COMMENT_SIZE]
        SCRIPT_EXPORT bool GetInfo(duint addr, CommentInfo* info);
        SCRIPT_EXPORT bool Delete(duint addr);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end);
        SCRIPT_EXPORT void Clear();
        SCRIPT_EXPORT bool GetList(ListOf(CommentInfo) list); //caller has the responsibility to free the list
    }; //Comment
}; //Script

#endif //_SCRIPTAPI_COMMENT_H