#ifndef _SCRIPTAPI_BOOKMARK_H
#define _SCRIPTAPI_BOOKMARK_H

#include "_scriptapi.h"

namespace Script
{
    namespace Bookmark
    {
        struct BookmarkInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rva;
            bool manual;
        };

        SCRIPT_EXPORT bool Set(duint addr, bool manual = false);
        SCRIPT_EXPORT bool Set(const BookmarkInfo* info);
        SCRIPT_EXPORT bool Get(duint addr);
        SCRIPT_EXPORT bool GetInfo(duint addr, BookmarkInfo* info);
        SCRIPT_EXPORT bool Delete(duint addr);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end);
        SCRIPT_EXPORT void Clear();
        SCRIPT_EXPORT bool GetList(ListOf(BookmarkInfo) list); //caller has the responsibility to free the list
    }; //Bookmark
}; //Script

#endif //_SCRIPTAPI_BOOKMARK_H