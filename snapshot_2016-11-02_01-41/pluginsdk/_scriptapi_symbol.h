#ifndef _SCRIPTAPI_SYMBOL_H
#define _SCRIPTAPI_SYMBOL_H

#include "_scriptapi.h"

namespace Script
{
    namespace Symbol
    {
        enum SymbolType
        {
            Function,
            Import,
            Export
        };

        struct SymbolInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rva;
            char name[MAX_LABEL_SIZE];
            bool manual;
            SymbolType type;
        };

        SCRIPT_EXPORT bool GetList(ListOf(SymbolInfo) list); //caller has the responsibility to free the list
    }; //Symbol
}; //Script

#endif //_SCRIPTAPI_SYMBOL_H