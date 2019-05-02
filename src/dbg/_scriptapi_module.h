#ifndef _SCRIPTAPI_MODULE_H
#define _SCRIPTAPI_MODULE_H

#include "_scriptapi.h"

namespace Script
{
    namespace Module
    {
        struct ModuleInfo
        {
            duint base;
            duint size;
            duint entry;
            int sectionCount;
            char name[MAX_MODULE_SIZE];
            char path[MAX_PATH];
        };

        struct ModuleSectionInfo
        {
            duint addr;
            duint size;
            char name[MAX_SECTION_SIZE * 5];
        };

        struct ModuleExport
        {
            duint ordinal;
            duint rva;
            duint va;
            bool forwarded;
            char forwardName[MAX_STRING_SIZE];
            char name[MAX_STRING_SIZE];
            char undecoratedName[MAX_STRING_SIZE];
        };

        struct ModuleImport
        {
            duint iatRva;
            duint iatVa;
            duint ordinal; //equal to -1 if imported by name
            char name[MAX_STRING_SIZE];
            char undecoratedName[MAX_STRING_SIZE];
        };

        SCRIPT_EXPORT bool InfoFromAddr(duint addr, ModuleInfo* info);
        SCRIPT_EXPORT bool InfoFromName(const char* name, ModuleInfo* info);
        SCRIPT_EXPORT duint BaseFromAddr(duint addr);
        SCRIPT_EXPORT duint BaseFromName(const char* name);
        SCRIPT_EXPORT duint SizeFromAddr(duint addr);
        SCRIPT_EXPORT duint SizeFromName(const char* name);
        SCRIPT_EXPORT bool NameFromAddr(duint addr, char* name); //name[MAX_MODULE_SIZE]
        SCRIPT_EXPORT bool PathFromAddr(duint addr, char* path); //path[MAX_PATH]
        SCRIPT_EXPORT bool PathFromName(const char* name, char* path); //path[MAX_PATH]
        SCRIPT_EXPORT duint EntryFromAddr(duint addr);
        SCRIPT_EXPORT duint EntryFromName(const char* name);
        SCRIPT_EXPORT int SectionCountFromAddr(duint addr);
        SCRIPT_EXPORT int SectionCountFromName(const char* name);
        SCRIPT_EXPORT bool SectionFromAddr(duint addr, int number, ModuleSectionInfo* section);
        SCRIPT_EXPORT bool SectionFromName(const char* name, int number, ModuleSectionInfo* section);
        SCRIPT_EXPORT bool SectionListFromAddr(duint addr, ListOf(ModuleSectionInfo) list);
        SCRIPT_EXPORT bool SectionListFromName(const char* name, ListOf(ModuleSectionInfo) list);
        SCRIPT_EXPORT bool GetMainModuleInfo(ModuleInfo* info);
        SCRIPT_EXPORT duint GetMainModuleBase();
        SCRIPT_EXPORT duint GetMainModuleSize();
        SCRIPT_EXPORT duint GetMainModuleEntry();
        SCRIPT_EXPORT int GetMainModuleSectionCount();
        SCRIPT_EXPORT bool GetMainModuleName(char* name); //name[MAX_MODULE_SIZE]
        SCRIPT_EXPORT bool GetMainModulePath(char* path); //path[MAX_PATH]
        SCRIPT_EXPORT bool GetMainModuleSectionList(ListOf(ModuleSectionInfo) list); //caller has the responsibility to free the list
        SCRIPT_EXPORT bool GetList(ListOf(ModuleInfo) list); //caller has the responsibility to free the list
        SCRIPT_EXPORT bool GetExports(const ModuleInfo* mod, ListOf(ModuleExport) list); //caller has the responsibility to free the list
        SCRIPT_EXPORT bool GetImports(const ModuleInfo* mod, ListOf(ModuleImport) list); //caller has the responsibility to free the list
    }; //Module
}; //Script

#endif //_SCRIPTAPI_MODULE_H
