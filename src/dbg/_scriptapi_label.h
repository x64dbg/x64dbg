#ifndef _SCRIPTAPI_LABEL_H
#define _SCRIPTAPI_LABEL_H

#include "_scriptapi.h"

namespace Script
{
    namespace Label
    {
        struct LabelInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rva;
            char text[MAX_LABEL_SIZE];
            bool manual;
        };

        SCRIPT_EXPORT bool Set(duint addr, const char* text, bool manual = false);
        SCRIPT_EXPORT bool Set(duint addr, const char* text, bool manual = false, bool temporary = false);
        SCRIPT_EXPORT bool Set(const LabelInfo* info);
        SCRIPT_EXPORT bool FromString(const char* label, duint* addr);
        SCRIPT_EXPORT bool Get(duint addr, char* text); //text[MAX_LABEL_SIZE]
        SCRIPT_EXPORT bool IsTemporary(duint addr);
        SCRIPT_EXPORT bool GetInfo(duint addr, LabelInfo* info);
        SCRIPT_EXPORT bool Delete(duint addr);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end);
        SCRIPT_EXPORT void Clear();
        SCRIPT_EXPORT bool GetList(ListOf(LabelInfo) list); //caller has the responsibility to free the list
    }; //Label
}; //Script

#endif //_SCRIPTAPI_LABEL_H