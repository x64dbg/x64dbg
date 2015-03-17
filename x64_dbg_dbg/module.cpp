#include "module.h"
#include "debugger.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"

static ModulesInfo modinfo;

bool ModLoad(uint base, uint size, const char* fullpath)
{
    //
    // Handle a new module being loaded
    //
    // TODO: Do loaded modules always require a path?
    if(!base or !size or !fullpath)
        return false;

    MODINFO info;

    //
    // Break the module path into a directory and file name
    //
    char dir[deflen];
    char* file;

    GetFullPathNameA(fullpath, deflen, dir, &file);

    // Make everything lowercase
    _strlwr(dir);

    // Copy the extension into the module struct
    {
        char* extensionPos = strrchr(file, '.');

        if(extensionPos)
        {
            extensionPos[0] = '\0';
            strcpy_s(info.extension, extensionPos + 1);
        }
    }

    // Copy the name to the module struct
    strcpy_s(info.name, file);

    //
    // Module base address/size/hash index
    //
    info.hash = ModHashFromName(info.name);
    info.base = base;
    info.size = size;

    //
    // Process module sections
    //
    info.sections.clear();

    WString wszFullPath = StringUtils::Utf8ToUtf16(fullpath);
    if(StaticFileLoadW(wszFullPath.c_str(), UE_ACCESS_READ, false, &info.Handle, &info.FileMapSize, &info.MapHandle, &info.FileMapVA))
    {
        // Get the entry point
        info.entry = GetPE32DataFromMappedFile(info.FileMapVA, 0, UE_OEP) + info.base;

        int sectionCount = (int)GetPE32DataFromMappedFile(info.FileMapVA, 0, UE_SECTIONNUMBER);
        for(int i = 0; i < sectionCount; i++)
        {
            MODSECTIONINFO curSection;

            curSection.addr             = GetPE32DataFromMappedFile(info.FileMapVA, i, UE_SECTIONVIRTUALOFFSET) + info.base;
            curSection.size             = GetPE32DataFromMappedFile(info.FileMapVA, i, UE_SECTIONVIRTUALSIZE);
            const char* sectionName     = (const char*)GetPE32DataFromMappedFile(info.FileMapVA, i, UE_SECTIONNAME);

            // Escape section name when needed
            strcpy_s(curSection.name, StringUtils::Escape(sectionName).c_str());

            // Add entry to the vector
            info.sections.push_back(curSection);
        }
    }

    // Add module to list
    EXCLUSIVE_ACQUIRE(LockModules);
    modinfo.insert(std::make_pair(Range(base, base + size - 1), info));
    EXCLUSIVE_RELEASE();

    SymUpdateModuleList();
    return true;
}

bool ModUnload(uint base)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    // Find the iterator index
    const auto found = modinfo.find(Range(base, base));

    if(found == modinfo.end())
        return false;

    // Remove it from the list
    modinfo.erase(found);

    // Unload everything from TitanEngine
    StaticFileUnloadW(nullptr, false, found->second.Handle, found->second.FileMapSize, found->second.MapHandle, found->second.FileMapVA);

    // Update symbols
    SymUpdateModuleList();
    return true;
}

void ModClear()
{
    // Remove all modules in the list
    EXCLUSIVE_ACQUIRE(LockModules);
    modinfo.clear();
    EXCLUSIVE_RELEASE();

    // Tell the symbol updater
    SymUpdateModuleList();
}

MODINFO* ModInfoFromAddr(uint addr)
{
    //
    // NOTE: THIS DOES _NOT_ USE LOCKS
    //
    auto found = modinfo.find(Range(addr, addr));

    // Was the module found with this address?
    if(found == modinfo.end())
        return nullptr;

    return &found->second;
}

bool ModNameFromAddr(uint addr, char* modname, bool extension)
{
    if(!modname)
        return false;

    SHARED_ACQUIRE(LockModules);

    // Get a pointer to module information
    auto module = ModInfoFromAddr(addr);

    if(!module)
        return false;

    // Zero buffer first
    memset(modname, 0, MAX_MODULE_SIZE);

    // Append the module path/name
    strcat_s(modname, MAX_MODULE_SIZE, module->name);

    // Append the extension
    if(extension)
        strcat_s(modname, MAX_MODULE_SIZE, module->extension);

    return true;
}

uint ModBaseFromAddr(uint addr)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(addr);

    if(!module)
        return 0;

    return module->base;
}

uint ModHashFromAddr(uint addr)
{
    //
    // Returns a unique hash from a virtual address
    //
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(addr);

    if(!module)
        return addr;

    return module->hash + (addr - module->base);
}

uint ModHashFromName(const char* mod)
{
    //
    // return MODINFO.hash (based on the name)
    //
    if(!mod || !mod[0])
        return 0;

    return murmurhash(mod, (int)strlen(mod));
}

uint ModBaseFromName(const char* modname)
{
    if(!modname || strlen(modname) >= MAX_MODULE_SIZE)
        return 0;

    SHARED_ACQUIRE(LockModules);

    for(auto itr = modinfo.begin(); itr != modinfo.end(); itr++)
    {
        char curmodname[MAX_MODULE_SIZE];
        sprintf(curmodname, "%s%s", itr->second.name, itr->second.extension);

        // Test with and without extension
        if(!_stricmp(curmodname, modname) || !_stricmp(itr->second.name, modname))
            return itr->second.base;
    }

    return 0;
}

uint ModSizeFromAddr(uint addr)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(addr);

    if(!module)
        return 0;

    return module->size;
}

bool ModSectionsFromAddr(uint addr, std::vector<MODSECTIONINFO>* sections)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(addr);

    if(!module)
        return false;

    // Copy vector <-> vector
    *sections = module->sections;
    return true;
}

uint ModEntryFromAddr(uint addr)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(addr);

    if(!module)
        return 0;

    return module->entry;
}

int ModPathFromAddr(duint addr, char* path, int size)
{
    auto module = ModInfoFromAddr(addr);

    if(!module)
        return 0;

    strcpy_s(path, size, module->path);
    return (int)strlen(path);
}

int ModPathFromName(const char* modname, char* path, int size)
{
    return ModPathFromAddr(ModBaseFromName(modname), path, size);
}