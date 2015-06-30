#include "_scriptapi_module.h"
#include "module.h"
#include "debugger.h"

SCRIPT_EXPORT bool Script::Module::InfoFromAddr(duint addr, Script::Module::ModuleInfo* info)
{
    MODINFO* modInfo = ModInfoFromAddr(addr);
    if(!info || !modInfo)
        return false;
    info->base = modInfo->base;
    info->size = modInfo->size;
    info->entry = modInfo->entry;
    info->sectionCount = (int)modInfo->sections.size();
    strcpy_s(info->name, modInfo->name);
    strcat_s(info->name, modInfo->extension);
    strcpy_s(info->path, modInfo->path);
    return true;
}

SCRIPT_EXPORT bool Script::Module::InfoFromName(const char* name, Script::Module::ModuleInfo* info)
{
    return Module::InfoFromAddr(Module::BaseFromName(name), info);
}

SCRIPT_EXPORT duint Script::Module::BaseFromAddr(duint addr)
{
    return ModBaseFromAddr(addr);
}

SCRIPT_EXPORT duint Script::Module::BaseFromName(const char* name)
{
    return ModBaseFromName(name);
}

SCRIPT_EXPORT duint Script::Module::SizeFromAddr(duint addr)
{
    return ModSizeFromAddr(addr);
}

SCRIPT_EXPORT duint Script::Module::SizeFromName(const char* name)
{
    return Module::SizeFromAddr(Module::BaseFromName(name));
}

SCRIPT_EXPORT bool Script::Module::NameFromAddr(duint addr, char* name)
{
    return ModNameFromAddr(addr, name, true);
}

SCRIPT_EXPORT bool Script::Module::PathFromAddr(duint addr, char* path)
{
    return !!ModPathFromAddr(addr, path, MAX_PATH);
}

SCRIPT_EXPORT bool Script::Module::PathFromName(const char* name, char* path)
{
    return Module::PathFromAddr(Module::BaseFromName(name), path);
}

SCRIPT_EXPORT duint Script::Module::EntryFromAddr(duint addr)
{
    return ModEntryFromAddr(addr);
}

SCRIPT_EXPORT duint Script::Module::EntryFromName(const char* name)
{
    return Module::EntryFromAddr(Module::BaseFromName(name));
}

SCRIPT_EXPORT int Script::Module::SectionCountFromAddr(duint addr)
{
    MODINFO* modInfo = ModInfoFromAddr(addr);
    return modInfo ? (int)modInfo->sections.size() : 0;
}

SCRIPT_EXPORT int Script::Module::SectionCountFromName(const char* name)
{
    return Module::SectionCountFromAddr(Module::BaseFromName(name));
}

SCRIPT_EXPORT bool Script::Module::SectionFromAddr(duint addr, int number, ModuleSectionInfo* section)
{
    MODINFO* modInfo = ModInfoFromAddr(addr);
    if(!section || !modInfo || number < 0 || number >= (int)modInfo->sections.size())
        return false;
    const MODSECTIONINFO & secInfo = modInfo->sections.at(number);
    section->addr = secInfo.addr;
    section->size = secInfo.size;
    strcpy_s(section->name, secInfo.name);
    return true;
}

SCRIPT_EXPORT bool Script::Module::SectionFromName(const char* name, int number, ModuleSectionInfo* section)
{
    return Module::SectionFromAddr(Module::BaseFromName(name), number, section);
}

SCRIPT_EXPORT bool Script::Module::GetMainModuleInfo(ModuleInfo* info)
{
    return Module::InfoFromAddr(Module::GetMainModuleBase(), info);
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleBase()
{
    return dbggetdebuggedbase();
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleSize()
{
    return Module::SizeFromAddr(Module::GetMainModuleBase());
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleEntry()
{
    return Module::EntryFromAddr(Module::GetMainModuleBase());
}

SCRIPT_EXPORT int Script::Module::GetMainModuleSectionCount()
{
    return Module::SectionCountFromAddr(Module::GetMainModuleBase());
}

SCRIPT_EXPORT bool Script::Module::GetMainModuleName(char* name)
{
    return Module::NameFromAddr(Module::GetMainModuleBase(), name);
}

SCRIPT_EXPORT bool Script::Module::GetMainModulePath(char* path)
{
    return Module::PathFromAddr(Module::GetMainModuleBase(), path);
}