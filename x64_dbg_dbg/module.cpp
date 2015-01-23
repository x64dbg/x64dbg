#include "module.h"
#include "debugger.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"

static ModulesInfo modinfo;

///module functions
bool modload(uint base, uint size, const char* fullpath)
{
    if(!base or !size or !fullpath)
        return false;
    char name[deflen] = "";

    int len = (int)strlen(fullpath);
    while(fullpath[len] != '\\' and len)
        len--;
    if(len)
        len++;
    strcpy(name, fullpath + len);
    _strlwr(name);
    len = (int)strlen(name);
    name[MAX_MODULE_SIZE - 1] = 0; //ignore later characters
    while(name[len] != '.' and len)
        len--;
    MODINFO info;
    memset(&info, 0, sizeof(MODINFO));
    info.sections.clear();
    info.hash = modhashfromname(name);
    if(len)
    {
        strcpy(info.extension, name + len);
        name[len] = 0; //remove extension
    }
    info.base = base;
    info.size = size;
    strcpy(info.name, name);

    //process module sections
    HANDLE FileHandle;
    DWORD LoadedSize;
    HANDLE FileMap;
    ULONG_PTR FileMapVA;
    WString wszFullPath = StringUtils::Utf8ToUtf16(fullpath);
    if(StaticFileLoadW(wszFullPath.c_str(), UE_ACCESS_READ, false, &FileHandle, &LoadedSize, &FileMap, &FileMapVA))
    {
        info.entry = GetPE32DataFromMappedFile(FileMapVA, 0, UE_OEP) + info.base; //get entry point
        int SectionCount = (int)GetPE32DataFromMappedFile(FileMapVA, 0, UE_SECTIONNUMBER);
        if(SectionCount > 0)
        {
            for(int i = 0; i < SectionCount; i++)
            {
                MODSECTIONINFO curSection;
                curSection.addr = GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALOFFSET) + base;
                curSection.size = GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONVIRTUALSIZE);
                const char* SectionName = (const char*)GetPE32DataFromMappedFile(FileMapVA, i, UE_SECTIONNAME);
                //escape section name when needed
                int len = (int)strlen(SectionName);
                int escape_count = 0;
                for(int k = 0; k < len; k++)
                    if(SectionName[k] == '\\' or SectionName[k] == '\"' or !isprint(SectionName[k]))
                        escape_count++;
                strcpy_s(curSection.name, StringUtils::Escape(SectionName).c_str());
                info.sections.push_back(curSection);
            }
        }
        StaticFileUnloadW(wszFullPath.c_str(), false, FileHandle, LoadedSize, FileMap, FileMapVA);
    }

    //add module to list
    CriticalSectionLocker locker(LockModules);
    modinfo.insert(std::make_pair(Range(base, base + size - 1), info));
    symupdatemodulelist();
    return true;
}

bool modunload(uint base)
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(base, base));
    if(found == modinfo.end()) //not found
        return false;
    modinfo.erase(found);
    symupdatemodulelist();
    return true;
}

void modclear()
{
    CriticalSectionLocker locker(LockModules);
    ModulesInfo().swap(modinfo);
    symupdatemodulelist();
}

bool modnamefromaddr(uint addr, char* modname, bool extension)
{
    if(!modname)
        return false;
    *modname = '\0';
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(addr, addr));
    if(found == modinfo.end()) //not found
        return false;
    String mod = found->second.name;
    if(extension)
        mod += found->second.extension;
    strcpy_s(modname, MAX_MODULE_SIZE, mod.c_str());
    return true;
}

uint modbasefromaddr(uint addr)
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(addr, addr));
    if(found == modinfo.end()) //not found
        return 0;
    return found->second.base;
}

uint modhashfromva(uint va) //return a unique hash from a VA
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(va, va));
    if(found == modinfo.end()) //not found
        return va;
    return found->second.hash + (va - found->second.base);
}

uint modhashfromname(const char* mod) //return MODINFO.hash
{
    if(!mod or !*mod)
        return 0;
    int len = (int)strlen(mod);
    return murmurhash(mod, len);
}

uint modbasefromname(const char* modname)
{
    if(!modname or strlen(modname) >= MAX_MODULE_SIZE)
        return 0;
    CriticalSectionLocker locker(LockModules);
    for(ModulesInfo::iterator i = modinfo.begin(); i != modinfo.end(); ++i)
    {
        MODINFO* curMod = &i->second;
        char curmodname[MAX_MODULE_SIZE] = "";
        sprintf(curmodname, "%s%s", curMod->name, curMod->extension);
        if(!_stricmp(curmodname, modname)) //with extension
            return curMod->base;
        if(!_stricmp(curMod->name, modname)) //without extension
            return curMod->base;
    }
    return 0;
}

uint modsizefromaddr(uint addr)
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(addr, addr));
    if(found == modinfo.end()) //not found
        return 0;
    return found->second.size;
}

bool modsectionsfromaddr(uint addr, std::vector<MODSECTIONINFO>* sections)
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(addr, addr));
    if(found == modinfo.end()) //not found
        return false;
    *sections = found->second.sections;
    return true;
}

uint modentryfromaddr(uint addr)
{
    CriticalSectionLocker locker(LockModules);
    const ModulesInfo::iterator found = modinfo.find(Range(addr, addr));
    if(found == modinfo.end()) //not found
        return 0;
    return found->second.entry;
}

int modpathfromaddr(duint addr, char* path, int size)
{
    Memory<wchar_t*> wszModPath(size * sizeof(wchar_t), "modpathfromaddr:wszModPath");
    if(!GetModuleFileNameExW(fdProcessInfo->hProcess, (HMODULE)modbasefromaddr(addr), wszModPath, size))
    {
        *path = '\0';
        return 0;
    }
    strcpy_s(path, size, StringUtils::Utf16ToUtf8(wszModPath()).c_str());
    return (int)strlen(path);
}

int modpathfromname(const char* modname, char* path, int size)
{
    return modpathfromaddr(modbasefromname(modname), path, size);
}
