#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "breakpoint.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"
#include "lz4\lz4file.h"
#include "patches.h"

static ModulesInfo modinfo;
static CommentsInfo comments;
static LabelsInfo labels;
static BookmarksInfo bookmarks;
static FunctionsInfo functions;
static LoopsInfo loops;

//database functions
void dbsave()
{
    dprintf("saving database...");
    DWORD ticks = GetTickCount();
    JSON root = json_object();
    commentcachesave(root);
    labelcachesave(root);
    bookmarkcachesave(root);
    functioncachesave(root);
    loopcachesave(root);
    bpcachesave(root);
    WString wdbpath = StringUtils::Utf8ToUtf16(dbpath);
    if(json_object_size(root))
    {
        FILE* jsonFile = 0;
        if(_wfopen_s(&jsonFile, wdbpath.c_str(), L"wb"))
        {
            dputs("failed to open database file for editing!");
            json_decref(root); //free root
            return;
        }
        if(json_dumpf(root, jsonFile, JSON_INDENT(4)) == -1)
        {
            dputs("couldn't write JSON to database file...");
            json_decref(root); //free root
            return;
        }
        fclose(jsonFile);
        LZ4_compress_fileW(wdbpath.c_str(), wdbpath.c_str());
    }
    else //remove database when nothing is in there
        DeleteFileW(wdbpath.c_str());
    dprintf("%ums\n", GetTickCount() - ticks);
    json_decref(root); //free root
}

void dbload()
{
    if(!FileExists(dbpath)) //no database to load
        return;
    dprintf("loading database...");
    DWORD ticks = GetTickCount();
    WString wdbpath = StringUtils::Utf8ToUtf16(dbpath);
    LZ4_STATUS status = LZ4_decompress_fileW(wdbpath.c_str(), wdbpath.c_str());
    if(status != LZ4_SUCCESS && status != LZ4_INVALID_ARCHIVE)
    {
        dputs("\ninvalid database file!");
        return;
    }
    FILE* jsonFile = 0;
    if(_wfopen_s(&jsonFile, wdbpath.c_str(), L"rb"))
    {
        dputs("\nfailed to open database file!");
        return;
    }
    JSON root = json_loadf(jsonFile, 0, 0);
    fclose(jsonFile);
    if(status != LZ4_INVALID_ARCHIVE)
        LZ4_compress_fileW(wdbpath.c_str(), wdbpath.c_str());
    if(!root)
    {
        dputs("\ninvalid database file (JSON)!");
        return;
    }
    commentcacheload(root);
    labelcacheload(root);
    bookmarkcacheload(root);
    functioncacheload(root);
    loopcacheload(root);
    bpcacheload(root);
    json_decref(root); //free root
    dprintf("%ums\n", GetTickCount() - ticks);
}

void dbclose()
{
    dbsave();
    CriticalSectionLocker commentLocker(LockComments);
    CommentsInfo().swap(comments);

    CriticalSectionLocker labelLocker(LockLabels);
    LabelsInfo().swap(labels);

    CriticalSectionLocker bookmarkLocker(LockBookmarks);
    BookmarksInfo().swap(bookmarks);

    CriticalSectionLocker functionLocker(LockFunctions);
    FunctionsInfo().swap(functions);

    CriticalSectionLocker loopLocker(LockLoops);
    LoopsInfo().swap(loops);

    bpclear();
    patchclear();
}

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

///api functions
bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum)
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(fdProcessInfo->hProcess, (const void*)base, &mbi, sizeof(mbi));
    uint size = mbi.RegionSize;
    Memory<void*> buffer(size, "apienumexports:buffer");
    if(!memread(fdProcessInfo->hProcess, (const void*)base, buffer, size, 0))
        return false;
    IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)((uint)buffer + GetPE32DataFromMappedFile((ULONG_PTR)buffer, 0, UE_PE_OFFSET));
    uint export_dir_rva = pnth->OptionalHeader.DataDirectory[0].VirtualAddress;
    uint export_dir_size = pnth->OptionalHeader.DataDirectory[0].Size;
    IMAGE_EXPORT_DIRECTORY export_dir;
    memset(&export_dir, 0, sizeof(export_dir));
    memread(fdProcessInfo->hProcess, (const void*)(export_dir_rva + base), &export_dir, sizeof(export_dir), 0);
    unsigned int NumberOfNames = export_dir.NumberOfNames;
    if(!export_dir.NumberOfFunctions or !NumberOfNames) //no named exports
        return false;
    char modname[MAX_MODULE_SIZE] = "";
    modnamefromaddr(base, modname, true);
    uint original_name_va = export_dir.Name + base;
    char original_name[deflen] = "";
    memset(original_name, 0, sizeof(original_name));
    memread(fdProcessInfo->hProcess, (const void*)original_name_va, original_name, deflen, 0);
    char* AddrOfFunctions_va = (char*)(export_dir.AddressOfFunctions + base);
    char* AddrOfNames_va = (char*)(export_dir.AddressOfNames + base);
    char* AddrOfNameOrdinals_va = (char*)(export_dir.AddressOfNameOrdinals + base);
    for(DWORD i = 0; i < NumberOfNames; i++)
    {
        DWORD curAddrOfName = 0;
        memread(fdProcessInfo->hProcess, AddrOfNames_va + sizeof(DWORD)*i, &curAddrOfName, sizeof(DWORD), 0);
        char* cur_name_va = (char*)(curAddrOfName + base);
        char cur_name[deflen] = "";
        memset(cur_name, 0, deflen);
        memread(fdProcessInfo->hProcess, cur_name_va, cur_name, deflen, 0);
        WORD curAddrOfNameOrdinals = 0;
        memread(fdProcessInfo->hProcess, AddrOfNameOrdinals_va + sizeof(WORD)*i, &curAddrOfNameOrdinals, sizeof(WORD), 0);
        DWORD curFunctionRva = 0;
        memread(fdProcessInfo->hProcess, AddrOfFunctions_va + sizeof(DWORD)*curAddrOfNameOrdinals, &curFunctionRva, sizeof(DWORD), 0);

        if(curFunctionRva >= export_dir_rva and curFunctionRva < export_dir_rva + export_dir_size)
        {
            char forwarded_api[deflen] = "";
            memset(forwarded_api, 0, deflen);
            memread(fdProcessInfo->hProcess, (void*)(curFunctionRva + base), forwarded_api, deflen, 0);
            int len = (int)strlen(forwarded_api);
            int j = 0;
            while(forwarded_api[j] != '.' and j < len)
                j++;
            if(forwarded_api[j] == '.')
            {
                forwarded_api[j] = 0;
                HINSTANCE hTempDll = LoadLibraryExA(forwarded_api, 0, DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
                if(hTempDll)
                {
                    uint local_addr = (uint)GetProcAddress(hTempDll, forwarded_api + j + 1);
                    if(local_addr)
                    {
                        uint remote_addr = ImporterGetRemoteAPIAddress(fdProcessInfo->hProcess, local_addr);
                        cbEnum(base, modname, cur_name, remote_addr);
                    }
                }
            }
        }
        else
        {
            cbEnum(base, modname, cur_name, curFunctionRva + base);
        }
    }
    return true;
}

///comment functions
bool commentset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or text[0] == '\1' or strlen(text) >= MAX_COMMENT_SIZE - 1)
        return false;
    if(!*text) //NOTE: delete when there is no text
    {
        commentdel(addr);
        return true;
    }
    COMMENTSINFO comment;
    comment.manual = manual;
    strcpy(comment.text, text);
    modnamefromaddr(addr, comment.mod, true);
    comment.addr = addr - modbasefromaddr(addr);
    const uint key = modhashfromva(addr);
    CriticalSectionLocker locker(LockComments);
    if(!comments.insert(std::make_pair(key, comment)).second) //key already present
        comments[key] = comment;
    return true;
}

bool commentget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockComments);
    const CommentsInfo::iterator found = comments.find(modhashfromva(addr));
    if(found == comments.end()) //not found
        return false;
    strcpy(text, found->second.text);
    return true;
}

bool commentdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockComments);
    return (comments.erase(modhashfromva(addr)) == 1);
}

void commentdelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockComments);
    CommentsInfo::iterator i = comments.begin();
    while(i != comments.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            comments.erase(i++);
        else
            i++;
    }
}

void commentcachesave(JSON root)
{
    CriticalSectionLocker locker(LockComments);
    const JSON jsoncomments = json_array();
    const JSON jsonautocomments = json_array();
    for(CommentsInfo::iterator i = comments.begin(); i != comments.end(); ++i)
    {
        const COMMENTSINFO curComment = i->second;
        JSON curjsoncomment = json_object();
        json_object_set_new(curjsoncomment, "module", json_string(curComment.mod));
        json_object_set_new(curjsoncomment, "address", json_hex(curComment.addr));
        json_object_set_new(curjsoncomment, "text", json_string(curComment.text));
        if(curComment.manual)
            json_array_append_new(jsoncomments, curjsoncomment);
        else
            json_array_append_new(jsonautocomments, curjsoncomment);
    }
    if(json_array_size(jsoncomments))
        json_object_set(root, "comments", jsoncomments);
    json_decref(jsoncomments);
    if(json_array_size(jsonautocomments))
        json_object_set(root, "autocomments", jsonautocomments);
    json_decref(jsonautocomments);
}

void commentcacheload(JSON root)
{
    CriticalSectionLocker locker(LockComments);
    comments.clear();
    const JSON jsoncomments = json_object_get(root, "comments");
    if(jsoncomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsoncomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curComment.mod, mod);
            else
                *curComment.mod = '\0';
            curComment.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual = true;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curComment.text, text);
            else
                continue; //skip
            const uint key = modhashfromname(curComment.mod) + curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
    JSON jsonautocomments = json_object_get(root, "autocomments");
    if(jsonautocomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautocomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curComment.mod, mod);
            else
                *curComment.mod = '\0';
            curComment.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual = false;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curComment.text, text);
            else
                continue; //skip
            const uint key = modhashfromname(curComment.mod) + curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
}

bool commentenum(COMMENTSINFO* commentlist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!commentlist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockComments);
    if(!commentlist && cbsize)
    {
        *cbsize = comments.size() * sizeof(COMMENTSINFO);
        return true;
    }
    int j = 0;
    for(CommentsInfo::iterator i = comments.begin(); i != comments.end(); ++i, j++)
    {
        commentlist[j] = i->second;
        commentlist[j].addr += modbasefromname(commentlist[j].mod);
    }
    return true;
}

///label functions
bool labelset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text) >= MAX_LABEL_SIZE - 1 or strstr(text, "&"))
        return false;
    if(!*text) //NOTE: delete when there is no text
    {
        labeldel(addr);
        return true;
    }
    LABELSINFO label;
    label.manual = manual;
    strcpy(label.text, text);
    modnamefromaddr(addr, label.mod, true);
    label.addr = addr - modbasefromaddr(addr);
    uint key = modhashfromva(addr);
    CriticalSectionLocker locker(LockLabels);
    if(!labels.insert(std::make_pair(modhashfromva(key), label)).second) //already present
        labels[key] = label;
    return true;
}

bool labelfromstring(const char* text, uint* addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        if(!strcmp(i->second.text, text))
        {
            if(addr)
                *addr = i->second.addr + modbasefromname(i->second.mod);
            return true;
        }
    }
    return false;
}

bool labelget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    const LabelsInfo::iterator found = labels.find(modhashfromva(addr));
    if(found == labels.end()) //not found
        return false;
    if(text)
        strcpy(text, found->second.text);
    return true;
}

bool labeldel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockLabels);
    return (labels.erase(modhashfromva(addr)) > 0);
}

void labeldelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockLabels);
    LabelsInfo::iterator i = labels.begin();
    while(i != labels.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            labels.erase(i++);
        else
            i++;
    }
}

void labelcachesave(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    const JSON jsonlabels = json_array();
    const JSON jsonautolabels = json_array();
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        const LABELSINFO curLabel = i->second;
        JSON curjsonlabel = json_object();
        json_object_set_new(curjsonlabel, "module", json_string(curLabel.mod));
        json_object_set_new(curjsonlabel, "address", json_hex(curLabel.addr));
        json_object_set_new(curjsonlabel, "text", json_string(curLabel.text));
        if(curLabel.manual)
            json_array_append_new(jsonlabels, curjsonlabel);
        else
            json_array_append_new(jsonautolabels, curjsonlabel);
    }
    if(json_array_size(jsonlabels))
        json_object_set(root, "labels", jsonlabels);
    json_decref(jsonlabels);
    if(json_array_size(jsonautolabels))
        json_object_set(root, "autolabels", jsonautolabels);
    json_decref(jsonautolabels);
}

void labelcacheload(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    labels.clear();
    const JSON jsonlabels = json_object_get(root, "labels");
    if(jsonlabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonlabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = true;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curLabel.text, text);
            else
                continue; //skip
            int len = (int)strlen(curLabel.text);
            for(int i = 0; i < len; i++)
                if(curLabel.text[i] == '&')
                    curLabel.text[i] = ' ';
            const uint key = modhashfromname(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
    JSON jsonautolabels = json_object_get(root, "autolabels");
    if(jsonautolabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautolabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = false;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curLabel.text, text);
            else
                continue; //skip
            const uint key = modhashfromname(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
}

bool labelenum(LABELSINFO* labellist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!labellist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockLabels);
    if(!labellist && cbsize)
    {
        *cbsize = labels.size() * sizeof(LABELSINFO);
        return true;
    }
    int j = 0;
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i, j++)
    {
        labellist[j] = i->second;
        labellist[j].addr += modbasefromname(labellist[j].mod);
    }
    return true;
}

///bookmark functions
bool bookmarkset(uint addr, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    BOOKMARKSINFO bookmark;
    modnamefromaddr(addr, bookmark.mod, true);
    bookmark.addr = addr - modbasefromaddr(addr);
    bookmark.manual = manual;
    CriticalSectionLocker locker(LockBookmarks);
    if(!bookmarks.insert(std::make_pair(modhashfromva(addr), bookmark)).second)
        return bookmarkdel(addr);
    return true;
}

bool bookmarkget(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    if(bookmarks.count(modhashfromva(addr)))
        return true;
    return false;
}

bool bookmarkdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    return (bookmarks.erase(modhashfromva(addr)) > 0);
}

void bookmarkdelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockBookmarks);
    BookmarksInfo::iterator i = bookmarks.begin();
    while(i != bookmarks.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            bookmarks.erase(i++);
        else
            i++;
    }
}

void bookmarkcachesave(JSON root)
{
    CriticalSectionLocker locker(LockBookmarks);
    const JSON jsonbookmarks = json_array();
    const JSON jsonautobookmarks = json_array();
    for(BookmarksInfo::iterator i = bookmarks.begin(); i != bookmarks.end(); ++i)
    {
        const BOOKMARKSINFO curBookmark = i->second;
        JSON curjsonbookmark = json_object();
        json_object_set_new(curjsonbookmark, "module", json_string(curBookmark.mod));
        json_object_set_new(curjsonbookmark, "address", json_hex(curBookmark.addr));
        if(curBookmark.manual)
            json_array_append_new(jsonbookmarks, curjsonbookmark);
        else
            json_array_append_new(jsonautobookmarks, curjsonbookmark);
    }
    if(json_array_size(jsonbookmarks))
        json_object_set(root, "bookmarks", jsonbookmarks);
    json_decref(jsonbookmarks);
    if(json_array_size(jsonautobookmarks))
        json_object_set(root, "autobookmarks", jsonautobookmarks);
    json_decref(jsonautobookmarks);
}

void bookmarkcacheload(JSON root)
{
    CriticalSectionLocker locker(LockBookmarks);
    bookmarks.clear();
    const JSON jsonbookmarks = json_object_get(root, "bookmarks");
    if(jsonbookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonbookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curBookmark.mod, mod);
            else
                *curBookmark.mod = '\0';
            curBookmark.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual = true;
            const uint key = modhashfromname(curBookmark.mod) + curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
    JSON jsonautobookmarks = json_object_get(root, "autobookmarks");
    if(jsonautobookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautobookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curBookmark.mod, mod);
            else
                *curBookmark.mod = '\0';
            curBookmark.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual = false;
            const uint key = modhashfromname(curBookmark.mod) + curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
}

bool bookmarkenum(BOOKMARKSINFO* bookmarklist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!bookmarklist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    if(!bookmarklist && cbsize)
    {
        *cbsize = bookmarks.size() * sizeof(BOOKMARKSINFO);
        return true;
    }
    int j = 0;
    for(BookmarksInfo::iterator i = bookmarks.begin(); i != bookmarks.end(); ++i, j++)
    {
        bookmarklist[j] = i->second;
        bookmarklist[j].addr += modbasefromname(bookmarklist[j].mod);
    }
    return true;
}

///function database
bool functionadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end < start or !memisvalidreadptr(fdProcessInfo->hProcess, start))
        return false;
    const uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end)) //the function boundaries are not in the same module
        return false;
    if(functionoverlaps(start, end))
        return false;
    FUNCTIONSINFO function;
    modnamefromaddr(start, function.mod, true);
    function.start = start - modbase;
    function.end = end - modbase;
    function.manual = manual;
    CriticalSectionLocker locker(LockFunctions);
    functions.insert(std::make_pair(ModuleRange(modhashfromva(modbase), Range(function.start, function.end)), function));
    return true;
}

bool functionget(uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    uint modbase = modbasefromaddr(addr);
    CriticalSectionLocker locker(LockFunctions);
    const FunctionsInfo::iterator found = functions.find(ModuleRange(modhashfromva(modbase), Range(addr - modbase, addr - modbase)));
    if(found == functions.end()) //not found
        return false;
    if(start)
        *start = found->second.start + modbase;
    if(end)
        *end = found->second.end + modbase;
    return true;
}

bool functionoverlaps(uint start, uint end)
{
    if(!DbgIsDebugging() or end < start)
        return false;
    const uint modbase = modbasefromaddr(start);
    CriticalSectionLocker locker(LockFunctions);
    return (functions.count(ModuleRange(modhashfromva(modbase), Range(start - modbase, end - modbase))) > 0);
}

bool functiondel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    const uint modbase = modbasefromaddr(addr);
    CriticalSectionLocker locker(LockFunctions);
    return (functions.erase(ModuleRange(modhashfromva(modbase), Range(addr - modbase, addr - modbase))) > 0);
}

void functiondelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockFunctions);
    FunctionsInfo::iterator i = functions.begin();
    while(i != functions.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll or !(i->second.start <= end and i->second.end >= start))
            functions.erase(i++);
        else
            i++;
    }
}

void functioncachesave(JSON root)
{
    CriticalSectionLocker locker(LockFunctions);
    const JSON jsonfunctions = json_array();
    const JSON jsonautofunctions = json_array();
    for(FunctionsInfo::iterator i = functions.begin(); i != functions.end(); ++i)
    {
        const FUNCTIONSINFO curFunction = i->second;
        JSON curjsonfunction = json_object();
        json_object_set_new(curjsonfunction, "module", json_string(curFunction.mod));
        json_object_set_new(curjsonfunction, "start", json_hex(curFunction.start));
        json_object_set_new(curjsonfunction, "end", json_hex(curFunction.end));
        if(curFunction.manual)
            json_array_append_new(jsonfunctions, curjsonfunction);
        else
            json_array_append_new(jsonautofunctions, curjsonfunction);
    }
    if(json_array_size(jsonfunctions))
        json_object_set(root, "functions", jsonfunctions);
    json_decref(jsonfunctions);
    if(json_array_size(jsonautofunctions))
        json_object_set(root, "autofunctions", jsonautofunctions);
    json_decref(jsonautofunctions);
}

void functioncacheload(JSON root)
{
    CriticalSectionLocker locker(LockFunctions);
    functions.clear();
    const JSON jsonfunctions = json_object_get(root, "functions");
    if(jsonfunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonfunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curFunction.mod, mod);
            else
                *curFunction.mod = '\0';
            curFunction.start = (uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end = (uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual = true;
            const uint key = modhashfromname(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(modhashfromname(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
    JSON jsonautofunctions = json_object_get(root, "autofunctions");
    if(jsonautofunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautofunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curFunction.mod, mod);
            else
                *curFunction.mod = '\0';
            curFunction.start = (uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end = (uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual = true;
            const uint key = modhashfromname(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(modhashfromname(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
}

bool functionenum(FUNCTIONSINFO* functionlist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!functionlist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockFunctions);
    if(!functionlist && cbsize)
    {
        *cbsize = functions.size() * sizeof(FUNCTIONSINFO);
        return true;
    }
    int j = 0;
    for(FunctionsInfo::iterator i = functions.begin(); i != functions.end(); ++i, j++)
    {
        functionlist[j] = i->second;
        uint modbase = modbasefromname(functionlist[j].mod);
        functionlist[j].start += modbase;
        functionlist[j].end += modbase;
    }
    return true;
}

//loop database
bool loopadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end < start or !memisvalidreadptr(fdProcessInfo->hProcess, start))
        return false;
    const uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end)) //the function boundaries are not in the same mem page
        return false;
    int finaldepth;
    if(loopoverlaps(0, start, end, &finaldepth)) //loop cannot overlap another loop
        return false;
    LOOPSINFO loop;
    modnamefromaddr(start, loop.mod, true);
    loop.start = start - modbase;
    loop.end = end - modbase;
    loop.depth = finaldepth;
    if(finaldepth)
        loopget(finaldepth - 1, start, &loop.parent, 0);
    else
        loop.parent = 0;
    loop.manual = manual;
    CriticalSectionLocker locker(LockLoops);
    loops.insert(std::make_pair(DepthModuleRange(finaldepth, ModuleRange(modhashfromva(modbase), Range(loop.start, loop.end))), loop));
    return true;
}

//get the start/end of a loop at a certain depth and addr
bool loopget(int depth, uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    const uint modbase = modbasefromaddr(addr);
    CriticalSectionLocker locker(LockLoops);
    LoopsInfo::iterator found = loops.find(DepthModuleRange(depth, ModuleRange(modhashfromva(modbase), Range(addr - modbase, addr - modbase))));
    if(found == loops.end()) //not found
        return false;
    if(start)
        *start = found->second.start + modbase;
    if(end)
        *end = found->second.end + modbase;
    return true;
}

//check if a loop overlaps a range, inside is not overlapping
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth)
{
    if(!DbgIsDebugging())
        return false;

    const uint modbase = modbasefromaddr(start);
    uint curStart = start - modbase;
    uint curEnd = end - modbase;
    const uint key = modhashfromva(modbase);

    CriticalSectionLocker locker(LockLoops);

    //check if the new loop fits in the old loop
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        if(i->first.second.first != key) //only look in the current module
            continue;
        LOOPSINFO* curLoop = &i->second;
        if(curLoop->start < curStart and curLoop->end > curEnd and curLoop->depth == depth)
            return loopoverlaps(depth + 1, curStart, curEnd, finaldepth);
    }

    if(finaldepth)
        *finaldepth = depth;

    //check for loop overlaps
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        if(i->first.second.first != key) //only look in the current module
            continue;
        LOOPSINFO* curLoop = &i->second;
        if(curLoop->start <= curEnd and curLoop->end >= curStart and curLoop->depth == depth)
            return true;
    }
    return false;
}

//this should delete a loop and all sub-loops that matches a certain addr
bool loopdel(int depth, uint addr)
{
    return false;
}

void loopcachesave(JSON root)
{
    CriticalSectionLocker locker(LockLoops);
    const JSON jsonloops = json_array();
    const JSON jsonautoloops = json_array();
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i)
    {
        const LOOPSINFO curLoop = i->second;
        JSON curjsonloop = json_object();
        json_object_set_new(curjsonloop, "module", json_string(curLoop.mod));
        json_object_set_new(curjsonloop, "start", json_hex(curLoop.start));
        json_object_set_new(curjsonloop, "end", json_hex(curLoop.end));
        json_object_set_new(curjsonloop, "depth", json_integer(curLoop.depth));
        json_object_set_new(curjsonloop, "parent", json_hex(curLoop.parent));
        if(curLoop.manual)
            json_array_append_new(jsonloops, curjsonloop);
        else
            json_array_append_new(jsonautoloops, curjsonloop);
    }
    if(json_array_size(jsonloops))
        json_object_set(root, "loops", jsonloops);
    json_decref(jsonloops);
    if(json_array_size(jsonautoloops))
        json_object_set(root, "autoloops", jsonautoloops);
    json_decref(jsonautoloops);
}

void loopcacheload(JSON root)
{
    CriticalSectionLocker locker(LockLoops);
    loops.clear();
    const JSON jsonloops = json_object_get(root, "loops");
    if(jsonloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLoop.mod, mod);
            else
                *curLoop.mod = '\0';
            curLoop.start = (uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end = (uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth = (int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent = (uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual = true;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(modhashfromname(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
    JSON jsonautoloops = json_object_get(root, "autoloops");
    if(jsonautoloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautoloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy(curLoop.mod, mod);
            else
                *curLoop.mod = '\0';
            curLoop.start = (uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end = (uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth = (int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent = (uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual = false;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(modhashfromname(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
}

bool loopenum(LOOPSINFO* looplist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!looplist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockLoops);
    if(!looplist && cbsize)
    {
        *cbsize = loops.size() * sizeof(LOOPSINFO);
        return true;
    }
    int j = 0;
    for(LoopsInfo::iterator i = loops.begin(); i != loops.end(); ++i, j++)
    {
        looplist[j] = i->second;
        uint modbase = modbasefromname(looplist[j].mod);
        looplist[j].start += modbase;
        looplist[j].end += modbase;
    }
    return true;
}