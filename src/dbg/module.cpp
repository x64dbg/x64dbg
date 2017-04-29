#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"
#include "memory.h"
#include "label.h"
#include <algorithm>

std::map<Range, MODINFO, RangeCompare> modinfo;
std::unordered_map<duint, std::string> hashNameMap;

void GetModuleInfo(MODINFO & Info, ULONG_PTR FileMapVA)
{
    // Get the entry point
    duint moduleOEP = GetPE32DataFromMappedFile(FileMapVA, 0, UE_OEP);

    // Fix a problem where the OEP is set to zero (non-existent).
    // OEP can't start at the PE header/offset 0 -- except if module is an EXE.
    Info.entry = moduleOEP + Info.base;

    if(!moduleOEP)
    {
        WORD characteristics = (WORD)GetPE32DataFromMappedFile(FileMapVA, 0, UE_CHARACTERISTICS);

        // If this wasn't an exe, invalidate the entry point
        if((characteristics & IMAGE_FILE_DLL) == IMAGE_FILE_DLL)
            Info.entry = 0;
    }

    // Enumerate all PE sections
    Info.sections.clear();
    int sectionCount = (int)GetPE32DataFromMappedFile(FileMapVA, 0, UE_SECTIONNUMBER);

    for(int i = 0; i < sectionCount; i++)
    {
        MODSECTIONINFO curSection;
        memset(&curSection, 0, sizeof(MODSECTIONINFO));

        curSection.addr = GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALOFFSET) + Info.base;
        curSection.size = GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALSIZE);
        const char* sectionName = (const char*)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONNAME);

        // Escape section name when needed
        strcpy_s(curSection.name, StringUtils::Escape(sectionName).c_str());

        // Add entry to the vector
        Info.sections.push_back(curSection);
    }

    // Clear imports by default
    Info.imports.clear();
}

bool ModLoad(duint Base, duint Size, const char* FullPath)
{
    // Handle a new module being loaded
    if(!Base || !Size || !FullPath)
        return false;

    // Copy the module path in the struct
    MODINFO info;
    memset(&info, 0, sizeof(info));
    strcpy_s(info.path, FullPath);

    // Break the module path into a directory and file name
    char file[MAX_MODULE_SIZE];
    {
        char dir[MAX_PATH];
        memset(dir, 0, sizeof(dir));

        // Dir <- lowercase(file path)
        strcpy_s(dir, FullPath);
        _strlwr(dir);

        // Find the last instance of a path delimiter (slash)
        char* fileStart = strrchr(dir, '\\');

        if(fileStart)
        {
            strcpy_s(file, fileStart + 1);
            fileStart[0] = '\0';
        }
        else
            strcpy_s(file, FullPath);
    }

    // Calculate module hash from full file name
    info.hash = ModHashFromName(file);

    // Copy the extension into the module struct
    {
        char* extensionPos = strrchr(file, '.');

        if(extensionPos)
        {
            strcpy_s(info.extension, extensionPos);
            extensionPos[0] = '\0';
        }
    }

    // Copy information to struct
    strcpy_s(info.name, file);
    info.base = Base;
    info.size = Size;
    info.fileHandle = nullptr;
    info.loadedSize = 0;
    info.fileMap = nullptr;
    info.fileMapVA = 0;

    // Determine whether the module is located in system
    wchar_t sysdir[MAX_PATH];
    GetEnvironmentVariableW(L"windir", sysdir, _countof(sysdir));
    String Utf8Sysdir = StringUtils::Utf16ToUtf8(sysdir);
    Utf8Sysdir.append("\\");
    if(_memicmp(Utf8Sysdir.c_str(), FullPath, Utf8Sysdir.size()) == 0)
    {
        info.party = 1;
    }
    else
    {
        info.party = 0;
    }

    // Load module data
    bool virtualModule = strstr(FullPath, "virtual:\\") == FullPath;

    if(!virtualModule)
    {
        auto wszFullPath = StringUtils::Utf8ToUtf16(FullPath);

        // Load the physical module from disk
        if(StaticFileLoadW(wszFullPath.c_str(), UE_ACCESS_READ, false, &info.fileHandle, &info.loadedSize, &info.fileMap, &info.fileMapVA))
        {
            GetModuleInfo(info, info.fileMapVA);
        }
        else
        {
            info.fileHandle = nullptr;
            info.loadedSize = 0;
            info.fileMap = nullptr;
            info.fileMapVA = 0;
        }
    }
    else
    {
        // This was a virtual module -> read it remotely
        Memory<unsigned char*> data(Size);
        MemRead(Base, data(), data.size());

        // Get information from the local buffer
        GetModuleInfo(info, (ULONG_PTR)data());
    }

    // Add module to list
    EXCLUSIVE_ACQUIRE(LockModules);
    modinfo.insert(std::make_pair(Range(Base, Base + Size - 1), info));
    EXCLUSIVE_RELEASE();

    // Put labels for virtual module exports
    if(virtualModule)
    {
        if(info.entry >= Base && info.entry < Base + Size)
            LabelSet(info.entry, "EntryPoint", false);

        apienumexports(Base, [](duint base, const char* mod, const char* name, duint addr)
        {
            LabelSet(addr, name, false);
        });
    }

    SymUpdateModuleList();
    return true;
}

bool ModUnload(duint Base)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    // Find the iterator index
    const auto found = modinfo.find(Range(Base, Base));

    if(found == modinfo.end())
        return false;

    // Unload the mapped file from memory
    const auto & info = found->second;
    if(info.fileMapVA)
        StaticFileUnloadW(StringUtils::Utf8ToUtf16(info.path).c_str(), false, info.fileHandle, info.loadedSize, info.fileMap, info.fileMapVA);

    // Remove it from the list
    modinfo.erase(found);
    EXCLUSIVE_RELEASE();

    // Update symbols
    SymUpdateModuleList();
    return true;
}

void ModClear()
{
    {
        // Clean up all the modules
        EXCLUSIVE_ACQUIRE(LockModules);

        for(const auto & mod : modinfo)
        {
            // Unload the mapped file from memory
            const auto & info = mod.second;
            if(info.fileMapVA)
                StaticFileUnloadW(StringUtils::Utf8ToUtf16(info.path).c_str(), false, info.fileHandle, info.loadedSize, info.fileMap, info.fileMapVA);
        }

        modinfo.clear();
    }

    {
        // Clean up the reverse hash map
        EXCLUSIVE_ACQUIRE(LockModuleHashes);
        hashNameMap.clear();
    }

    // Tell the symbol updater
    GuiSymbolUpdateModuleList(0, nullptr);
}

MODINFO* ModInfoFromAddr(duint Address)
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

bool ModNameFromAddr(duint Address, char* Name, bool Extension)
{
    ASSERT_NONNULL(Name);
    SHARED_ACQUIRE(LockModules);

    // Get a pointer to module information
    auto module = ModInfoFromAddr(Address);

    if(!module)
    {
        Name[0] = '\0';
        return false;
    }

    // Copy initial module name
    strcpy_s(Name, MAX_MODULE_SIZE, module->name);

    if(Extension)
        strcat_s(Name, MAX_MODULE_SIZE, module->extension);

    return true;
}

duint ModBaseFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->base;
}

duint ModHashFromAddr(duint Address)
{
    // Returns a unique hash from a virtual address
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return Address;

    return module->hash + (Address - module->base);
}

duint ModContentHashFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    if(module->fileMapVA != 0 && module->loadedSize > 0)
        return murmurhash((void*)module->fileMapVA, module->loadedSize);
    else
        return 0;
}

duint ModHashFromName(const char* Module)
{
    // return MODINFO.hash (based on the name)
    ASSERT_NONNULL(Module);
    auto len = int(strlen(Module));
    if(!len)
        return 0;
    auto hash = murmurhash(Module, len);

    //update the hash cache
    SHARED_ACQUIRE(LockModuleHashes);
    auto hashInCache = hashNameMap.find(hash) != hashNameMap.end();
    SHARED_RELEASE();
    if(!hashInCache)
    {
        EXCLUSIVE_ACQUIRE(LockModuleHashes);
        hashNameMap[hash] = Module;
    }

    return hash;
}

duint ModBaseFromName(const char* Module)
{
    ASSERT_NONNULL(Module);
    auto len = int(strlen(Module));
    if(!len)
        return 0;
    ASSERT_TRUE(len < MAX_MODULE_SIZE);
    SHARED_ACQUIRE(LockModules);

    for(const auto & i : modinfo)
    {
        const auto & currentModule = i.second;
        char currentModuleName[MAX_MODULE_SIZE];
        strcpy_s(currentModuleName, currentModule.name);
        strcat_s(currentModuleName, currentModule.extension);

        // Test with and without extension
        if(!_stricmp(currentModuleName, Module) || !_stricmp(currentModule.name, Module))
            return currentModule.base;
    }

    return 0;
}

duint ModSizeFromAddr(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return 0;

    return module->size;
}

std::string ModNameFromHash(duint Hash)
{
    SHARED_ACQUIRE(LockModuleHashes);
    auto found = hashNameMap.find(Hash);
    if(found == hashNameMap.end())
        return std::string();
    return found->second;
}

bool ModSectionsFromAddr(duint Address, std::vector<MODSECTIONINFO>* Sections)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return false;

    // Copy vector <-> vector
    *Sections = module->sections;
    return true;
}

bool ModImportsFromAddr(duint Address, std::vector<MODIMPORTINFO>* Imports)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    if(!module)
        return false;

    // Copy vector <-> vector
    *Imports = module->imports;
    return true;
}

duint ModEntryFromAddr(duint Address)
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

void ModGetList(std::vector<MODINFO> & list)
{
    SHARED_ACQUIRE(LockModules);
    list.clear();
    for(const auto & mod : modinfo)
        list.push_back(mod.second);
}

bool ModAddImportToModule(duint Base, const MODIMPORTINFO & importInfo)
{
    SHARED_ACQUIRE(LockModules);

    if(!Base || !importInfo.addr)
        return false;

    auto module = ModInfoFromAddr(Base);

    if(!module)
        return false;

    // Search in Import Vector
    auto pImports = &(module->imports);
    auto it = std::find_if(pImports->begin(), pImports->end(), [&importInfo](const MODIMPORTINFO & currentImportInfo)->bool
    {
        return (importInfo.addr == currentImportInfo.addr);
    });

    // Import in the list already
    if(it != pImports->end())
        return false;

    // Add import to imports vector
    pImports->push_back(importInfo);

    return true;
}

int ModGetParty(duint Address)
{
    SHARED_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    // If the module is not found, it is an user module
    if(!module)
        return 0;

    return module->party;
}

void ModSetParty(duint Address, int Party)
{
    EXCLUSIVE_ACQUIRE(LockModules);

    auto module = ModInfoFromAddr(Address);

    // If the module is not found, it is an user module
    if(!module)
        return;

    module->party = Party;
}