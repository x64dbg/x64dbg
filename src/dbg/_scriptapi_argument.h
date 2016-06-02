#ifndef _SCRIPTAPI_ARGUMENT_H
#define _SCRIPTAPI_ARGUMENT_H

#include "_scriptapi.h"

namespace Script
{
    namespace Argument
    {
        struct ArgumentInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rvaStart;
            duint rvaEnd;
            bool manual;
            duint instructioncount;
        };

        SCRIPT_EXPORT bool Add(duint start, duint end, bool manual, duint instructionCount = 0);
        SCRIPT_EXPORT bool Add(const ArgumentInfo* info);
        SCRIPT_EXPORT bool Get(duint addr, duint* start = nullptr, duint* end = nullptr, duint* instructionCount = nullptr);
        SCRIPT_EXPORT bool GetInfo(duint addr, ArgumentInfo* info);
        SCRIPT_EXPORT bool Overlaps(duint start, duint end);
        SCRIPT_EXPORT bool Delete(duint address);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end, bool deleteManual = false);
        SCRIPT_EXPORT void Clear();
        SCRIPT_EXPORT bool GetList(ListOf(ArgumentInfo) list); //caller has the responsibility to free the list
    }; //Argument
}; //Script

#endif //_SCRIPTAPI_ARGUMENT_H