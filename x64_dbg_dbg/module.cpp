#include "module.h"
#include "debugger.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"

static ModulesInfo modinfo;

bool ModLoad(uint Base, uint Size, const char* FullPath)
{
    //
    // Handle a new module being loaded
    //
    // TODO: Do loaded modules always require a path?
    if(!Base || !Size || !FullPath)
        return false;

    MODINFO info;

    // Break the module path into a directory and file name
    char dir[deflen];
    char* file;

    if(GetFullPathNameA(FullPath, ARRAYSIZE(dir), dir, &file) == 0)
        return false;

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

    // Module base address/size/hash index
    info.hash = ModHashFromName(info.name);
    info.base = Base;
    info.size = Size;

    // Process module sections
    info.sections.clear();

    WString wszFullPath = StringUtils::Utf8ToUtf16(FullPath);
    if(StaticFileLoadW(wszFullPath.c_str(), UE_ACCESS_READ, false, &info.Handle, &info.FileMapSize, &info.MapHandle, &info.FileMapVA))
    {
        // Get the entry point
        info.entry = GetPE32DataFromMappedFile(info.FileMapVA, 0, UE_OEP) + info.base;

        // Enumerate all PE sections
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
    modinfo.insert(std::make_pair(Range(Base, Base + Size - 1), info));
    EXCLUSIVE_RELEASE();

    SymUpdateModuleList();
    return true;
}

bool ModUnload(uint Base)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    // Find the iterator index
    const auto found = modinfo.find(Range(Base, Base));

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
    EXCLUSIVE_ACQUIRE(LockModules);
    modinfo.clear();
    EXCLUSIVE_RELEASE();

    // Tell the symbol updater
    SymUpdateModuleList();
}

MODINFO* ModInfoFromAddr(uint Address)
{
    //
    // NOTE: THIS DOES _NOT_ USE LOCKS
    //
    auto found = modinfo.find(Range(Address, Address));

    // Was the module found with this address?
    if(found == modinfo.end())
        return nullptr;

    return &found->second;
}

bool ModNameFromAddr(uint Address, char* Name, bool Extension)
{
    if(!Name)
        return false;

    SHARED_ACQUIRE(LockModules);

    // Get a pointer to module information
    auto module = ModInfoFromAddr(Address);

    if(!module)
        return false;

    // Copy initial module name
    strcpy_s(Name, MAX_MODULE_SIZE, module->name);

    if(Extension)
        strcat_s(Name, MAX_MODULE_SIZE, module->extension);

    return true;
}

uint ModBaseFromAddr(uint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->base;
}

uint ModHashFromAddr(uint Address)
{
    //
    // Returns a unique hash from a virtual address
    //
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return Address;

    return module->hash + (Address - module->base);
}

uint ModHashFromName(const char* Module)
{
    //
    // return MODINFO.hash (based on the name)
    //
    if(!Module || Module[0] == '\0')
        return 0;

    return murmurhash(Module, (int)strlen(Module));
}

uint ModBaseFromName(const char* Module)
{
    if(!Module || strlen(Module) >= MAX_MODULE_SIZE)
        return 0;

    SHARED_ACQUIRE(LockModules);

    for(auto itr = modinfo.begin(); itr != modinfo.end(); itr++)
    {
        char curmodname[MAX_MODULE_SIZE];
        sprintf(curmodname, "%s%s", itr->second.name, itr->second.extension);

        // Test with and without extension
        if(!_stricmp(curmodname, Module) || !_stricmp(itr->second.name, Module))
            return itr->second.base;
    }

    return 0;
}

uint ModSizeFromAddr(uint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->size;
}

bool ModSectionsFromAddr(uint Address, std::vector<MODSECTIONINFO>* Sections)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return false;

    // Copy vector <-> vector
    *Sections = module->sections;
    return true;
}

uint ModEntryFromAddr(uint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->entry;
}

int ModPathFromAddr(duint Address, char* Path, int Size)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    strcpy_s(Path, Size, module->path);
    return (int)strlen(Path);
}

int ModPathFromName(const char* Module, char* Path, int Size)
{
    return ModPathFromAddr(ModBaseFromName(Module), Path, Size);
}