#include "_scriptapi_module.h"
#include "threading.h"
#include "module.h"
#include "debugger.h"

SCRIPT_EXPORT bool Script::Module::InfoFromAddr(duint addr, ModuleInfo* info)
{
    SHARED_ACQUIRE(LockModules);
    auto modInfo = ModInfoFromAddr(addr);
    if(!info || !modInfo)
        return false;
    info->base = modInfo->base;
    info->size = modInfo->size;
    info->entry = modInfo->entry;
    info->sectionCount = int(modInfo->sections.size());
    strcpy_s(info->name, modInfo->name);
    strcat_s(info->name, modInfo->extension);
    strcpy_s(info->path, modInfo->path);
    return true;
}

SCRIPT_EXPORT bool Script::Module::InfoFromName(const char* name, ModuleInfo* info)
{
    return InfoFromAddr(BaseFromName(name), info);
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
    return SizeFromAddr(BaseFromName(name));
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
    return PathFromAddr(BaseFromName(name), path);
}

SCRIPT_EXPORT duint Script::Module::EntryFromAddr(duint addr)
{
    return ModEntryFromAddr(addr);
}

SCRIPT_EXPORT duint Script::Module::EntryFromName(const char* name)
{
    return EntryFromAddr(BaseFromName(name));
}

SCRIPT_EXPORT int Script::Module::SectionCountFromAddr(duint addr)
{
    SHARED_ACQUIRE(LockModules);
    auto modInfo = ModInfoFromAddr(addr);
    return modInfo ? int(modInfo->sections.size()) : 0;
}

SCRIPT_EXPORT int Script::Module::SectionCountFromName(const char* name)
{
    return SectionCountFromAddr(BaseFromName(name));
}

SCRIPT_EXPORT bool Script::Module::SectionFromAddr(duint addr, int number, ModuleSectionInfo* section)
{
    SHARED_ACQUIRE(LockModules);
    auto modInfo = ModInfoFromAddr(addr);
    if(!section || !modInfo || number < 0 || number >= int(modInfo->sections.size()))
        return false;
    const auto & secInfo = modInfo->sections.at(number);
    section->addr = secInfo.addr;
    section->size = secInfo.size;
    strcpy_s(section->name, secInfo.name);
    return true;
}

SCRIPT_EXPORT bool Script::Module::SectionFromName(const char* name, int number, ModuleSectionInfo* section)
{
    return SectionFromAddr(BaseFromName(name), number, section);
}

SCRIPT_EXPORT bool Script::Module::SectionListFromAddr(duint addr, ListOf(ModuleSectionInfo) list)
{
    SHARED_ACQUIRE(LockModules);
    auto modInfo = ModInfoFromAddr(addr);
    if(!modInfo)
        return false;
    std::vector<ModuleSectionInfo> scriptSectionList;
    scriptSectionList.reserve(modInfo->sections.size());
    for(const auto & section : modInfo->sections)
    {
        ModuleSectionInfo scriptSection;
        scriptSection.addr = section.addr;
        scriptSection.size = section.size;
        strcpy_s(scriptSection.name, section.name);
        scriptSectionList.push_back(scriptSection);
    }
    return BridgeList<ModuleSectionInfo>::CopyData(list, scriptSectionList);
}

SCRIPT_EXPORT bool Script::Module::SectionListFromName(const char* name, ListOf(ModuleSectionInfo) list)
{
    return SectionListFromAddr(BaseFromName(name), list);
}

SCRIPT_EXPORT bool Script::Module::GetMainModuleInfo(ModuleInfo* info)
{
    return InfoFromAddr(GetMainModuleBase(), info);
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleBase()
{
    return dbgdebuggedbase();
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleSize()
{
    return SizeFromAddr(GetMainModuleBase());
}

SCRIPT_EXPORT duint Script::Module::GetMainModuleEntry()
{
    return EntryFromAddr(GetMainModuleBase());
}

SCRIPT_EXPORT int Script::Module::GetMainModuleSectionCount()
{
    return SectionCountFromAddr(GetMainModuleBase());
}

SCRIPT_EXPORT bool Script::Module::GetMainModuleName(char* name)
{
    return NameFromAddr(GetMainModuleBase(), name);
}

SCRIPT_EXPORT bool Script::Module::GetMainModulePath(char* path)
{
    return PathFromAddr(GetMainModuleBase(), path);
}

SCRIPT_EXPORT bool Script::Module::GetMainModuleSectionList(ListOf(ModuleSectionInfo) list)
{
    return SectionListFromAddr(GetMainModuleBase(), list);
}

SCRIPT_EXPORT bool Script::Module::GetList(ListOf(ModuleInfo) list)
{
    std::vector<ModuleInfo> modScriptList;
    ModEnum([&modScriptList](const MODINFO & mod)
    {
        ModuleInfo scriptMod;
        scriptMod.base = mod.base;
        scriptMod.size = mod.size;
        scriptMod.entry = mod.entry;
        scriptMod.sectionCount = int(mod.sections.size());
        strcpy_s(scriptMod.name, mod.name);
        strcat_s(scriptMod.name, mod.extension);
        strcpy_s(scriptMod.path, mod.path);
        modScriptList.push_back(scriptMod);
    });
    return BridgeList<ModuleInfo>::CopyData(list, modScriptList);
}