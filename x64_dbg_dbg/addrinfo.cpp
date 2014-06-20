#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "breakpoint.h"
#include "threading.h"
#include "symbolinfo.h"
#include "murmurhash.h"

//TODO: use modinfo.hash+rva as key for the maps for "instant" lookup

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
    DWORD ticks=GetTickCount();
    JSON root=json_object();
    commentcachesave(root);
    labelcachesave(root);
    bookmarkcachesave(root);
    functioncachesave(root);
    loopcachesave(root);
    bpcachesave(root);
    if(json_object_size(root))
        json_dump_file(root, dbpath, JSON_INDENT(4));
    json_decref(root); //free root
    dprintf("%ums\n", GetTickCount()-ticks);
}

void dbload()
{
    dprintf("loading database...");
    DWORD ticks=GetTickCount();
    JSON root=json_load_file(dbpath, 0, 0);
    if(!root)
    {
        dputs("");
        return;
    }
    commentcacheload(root);
    labelcacheload(root);
    bookmarkcacheload(root);
    functioncacheload(root);
    loopcacheload(root);
    bpcacheload(root);
    json_decref(root); //free root
    dprintf("%ums\n", GetTickCount()-ticks);
}

void dbclose()
{
    dbsave();
    CommentsInfo().swap(comments);
    LabelsInfo().swap(labels);
    BookmarksInfo().swap(bookmarks);
    FunctionsInfo().swap(functions);
    LoopsInfo().swap(loops);
}

///module functions
bool modload(uint base, uint size, const char* fullpath)
{
    if(!base or !size or !fullpath)
        return false;
    char name[deflen]="";
    int len=strlen(fullpath);
    while(fullpath[len]!='\\' and len)
        len--;
    if(len)
        len++;
    strcpy(name, fullpath+len);
    _strlwr(name);
    len=strlen(name);
    name[MAX_MODULE_SIZE-1]=0; //ignore later characters
    while(name[len]!='.' and len)
        len--;
    MODINFO info;
    memset(&info, 0, sizeof(MODINFO));
    info.hash=modhashfromname(name);
    if(len)
    {
        strcpy(info.extension, name+len);
        name[len]=0; //remove extension
    }
    info.base=base;
    info.size=size;
    strcpy(info.name, name);
    modinfo.insert(std::make_pair(Range(base, base+size-1), info));
    symupdatemodulelist();
    return true;
}

bool modunload(uint base)
{
    const ModulesInfo::iterator found=modinfo.find(Range(base, base));
    if(found==modinfo.end()) //not found
        return false;
    modinfo.erase(found);
    symupdatemodulelist();
    return true;
}

void modclear()
{
    ModulesInfo().swap(modinfo);
    symupdatemodulelist();
}

bool modnamefromaddr(uint addr, char* modname, bool extension)
{
    if(!modname)
        return false;
    *modname='\0';
    const ModulesInfo::iterator found=modinfo.find(Range(addr, addr));
    if(found==modinfo.end()) //not found
        return false;
    strcpy(modname, found->second.name);
    if(extension)
        strcat(modname, found->second.extension); //append extension
    return true;
}

uint modbasefromaddr(uint addr)
{
    const ModulesInfo::iterator found=modinfo.find(Range(addr, addr));
    if(found==modinfo.end()) //not found
        return 0;
    return found->second.base;
}

uint modhashfromva(uint va) //return a unique hash from a VA
{
    const ModulesInfo::iterator found=modinfo.find(Range(va, va));
    if(found==modinfo.end()) //not found
        return va;
    return found->second.hash+(va-found->second.base);
}

uint modhashfromname(const char* mod) //return MODINFO.hash
{
    if(!mod or !*mod)
        return 0;
    int len=strlen(mod);
    return murmurhash(mod, len);
}

uint modbasefromname(const char* modname)
{
    if(!modname or strlen(modname)>=MAX_MODULE_SIZE)
        return 0;
    for(ModulesInfo::iterator i=modinfo.begin(); i!=modinfo.end(); ++i)
    {
        MODINFO* curMod=&i->second;
        char curmodname[MAX_MODULE_SIZE]="";
        sprintf(curmodname, "%s%s", curMod->name, curMod->extension);
        if(!_stricmp(curmodname, modname)) //with extension
            return curMod->base;
        if(!_stricmp(curMod->name, modname)) //without extension
            return curMod->base;
    }
    return 0;
}

///api functions
bool apienumexports(uint base, EXPORTENUMCALLBACK cbEnum)
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQueryEx(fdProcessInfo->hProcess, (const void*)base, &mbi, sizeof(mbi));
    uint size=mbi.RegionSize;
    void* buffer=emalloc(size, "apienumexports:buffer");
    if(!memread(fdProcessInfo->hProcess, (const void*)base, buffer, size, 0))
    {
        efree(buffer, "apienumexports:buffer");
        return false;
    }
    IMAGE_NT_HEADERS* pnth=(IMAGE_NT_HEADERS*)((uint)buffer+GetPE32DataFromMappedFile((ULONG_PTR)buffer, 0, UE_PE_OFFSET));
    uint export_dir_rva=pnth->OptionalHeader.DataDirectory[0].VirtualAddress;
    uint export_dir_size=pnth->OptionalHeader.DataDirectory[0].Size;
    efree(buffer, "apienumexports:buffer");
    IMAGE_EXPORT_DIRECTORY export_dir;
    memset(&export_dir, 0, sizeof(export_dir));
    memread(fdProcessInfo->hProcess, (const void*)(export_dir_rva+base), &export_dir, sizeof(export_dir), 0);
    unsigned int NumberOfNames=export_dir.NumberOfNames;
    if(!export_dir.NumberOfFunctions or !NumberOfNames) //no named exports
        return false;
    char modname[MAX_MODULE_SIZE]="";
    modnamefromaddr(base, modname, true);
    uint original_name_va=export_dir.Name+base;
    char original_name[deflen]="";
    memset(original_name, 0, sizeof(original_name));
    memread(fdProcessInfo->hProcess, (const void*)original_name_va, original_name, deflen, 0);
    char* AddrOfFunctions_va=(char*)(export_dir.AddressOfFunctions+base);
    char* AddrOfNames_va=(char*)(export_dir.AddressOfNames+base);
    char* AddrOfNameOrdinals_va=(char*)(export_dir.AddressOfNameOrdinals+base);
    for(DWORD i=0; i<NumberOfNames; i++)
    {
        DWORD curAddrOfName=0;
        memread(fdProcessInfo->hProcess, AddrOfNames_va+sizeof(DWORD)*i, &curAddrOfName, sizeof(DWORD), 0);
        char* cur_name_va=(char*)(curAddrOfName+base);
        char cur_name[deflen]="";
        memset(cur_name, 0, deflen);
        memread(fdProcessInfo->hProcess, cur_name_va, cur_name, deflen, 0);
        WORD curAddrOfNameOrdinals=0;
        memread(fdProcessInfo->hProcess, AddrOfNameOrdinals_va+sizeof(WORD)*i, &curAddrOfNameOrdinals, sizeof(WORD), 0);
        DWORD curFunctionRva=0;
        memread(fdProcessInfo->hProcess, AddrOfFunctions_va+sizeof(DWORD)*curAddrOfNameOrdinals, &curFunctionRva, sizeof(DWORD), 0);

        if(curFunctionRva>=export_dir_rva and curFunctionRva<export_dir_rva+export_dir_size)
        {
            char forwarded_api[deflen]="";
            memset(forwarded_api, 0, deflen);
            memread(fdProcessInfo->hProcess, (void*)(curFunctionRva+base), forwarded_api, deflen, 0);
            int len=strlen(forwarded_api);
            int j=0;
            while(forwarded_api[j]!='.' and j<len)
                j++;
            if(forwarded_api[j]=='.')
            {
                forwarded_api[j]=0;
                HINSTANCE hTempDll=LoadLibraryExA(forwarded_api, 0, DONT_RESOLVE_DLL_REFERENCES|LOAD_LIBRARY_AS_DATAFILE);
                if(hTempDll)
                {
                    uint local_addr=(uint)GetProcAddress(hTempDll, forwarded_api+j+1);
                    if(local_addr)
                    {
                        uint remote_addr=ImporterGetRemoteAPIAddress(fdProcessInfo->hProcess, local_addr);
                        cbEnum(base, modname, cur_name, remote_addr);
                    }
                }
            }
        }
        else
        {
            cbEnum(base, modname, cur_name, curFunctionRva+base);
        }
    }
    return true;
}

///comment functions
bool commentset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_COMMENT_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return commentdel(addr);
    COMMENTSINFO comment;
    comment.manual=manual;
    strcpy(comment.text, text);
    modnamefromaddr(addr, comment.mod, true);
    comment.addr=addr-modbasefromaddr(addr);
    const uint key=modhashfromva(addr);
    if(!comments.insert(std::make_pair(key, comment)).second) //key already present
        comments[key]=comment;
    return true;
}

bool commentget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    const CommentsInfo::iterator found=comments.find(modhashfromva(addr));
    if(found==comments.end()) //not found
        return false;
    strcpy(text, found->second.text);
    return true;
}

bool commentdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    return (comments.erase(modhashfromva(addr))==1);
}

void commentcachesave(JSON root)
{
    const JSON jsoncomments=json_array();
    const JSON jsonautocomments=json_array();
    for(CommentsInfo::iterator i=comments.begin(); i!=comments.end(); ++i)
    {
        const COMMENTSINFO curComment=i->second;
        JSON curjsoncomment=json_object();
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
    comments.clear();
    const JSON jsoncomments=json_object_get(root, "comments");
    if(jsoncomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsoncomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curComment.mod, mod);
            else
                *curComment.mod='\0';
            curComment.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual=true;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curComment.text, text);
            else
                continue; //skip
            const uint key=modhashfromname(curComment.mod)+curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
    JSON jsonautocomments=json_object_get(root, "autocomments");
    if(jsonautocomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautocomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curComment.mod, mod);
            else
                *curComment.mod='\0';
            curComment.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual=false;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curComment.text, text);
            else
                continue; //skip
            const uint key=modhashfromname(curComment.mod)+curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
}

///label functions
bool labelset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_LABEL_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return labeldel(addr);
    LABELSINFO label;
    label.manual=manual;
    strcpy(label.text, text);
    modnamefromaddr(addr, label.mod, true);
    label.addr=addr-modbasefromaddr(addr);
    uint key=modhashfromva(addr);
    if(!labels.insert(std::make_pair(modhashfromva(key), label)).second) //already present
        labels[key]=label;
    return true;
}

bool labelfromstring(const char* text, uint* addr)
{
    if(!DbgIsDebugging())
        return false;
    for(LabelsInfo::iterator i=labels.begin(); i!=labels.end(); ++i)
    {
        if(!strcmp(i->second.text, text))
        {
            if(addr)
                *addr=i->second.addr+modbasefromname(i->second.mod);
            return true;
        }
    }
    return false;
}

bool labelget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    const LabelsInfo::iterator found=labels.find(modhashfromva(addr));
    if(found==labels.end()) //not found
        return false;
    if(text)
        strcpy(text, found->second.text);
    return true;
}

bool labeldel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    return (labels.erase(modhashfromva(addr))>0);
}

void labelcachesave(JSON root)
{
    const JSON jsonlabels=json_array();
    const JSON jsonautolabels=json_array();
    for(LabelsInfo::iterator i=labels.begin(); i!=labels.end(); ++i)
    {
        const LABELSINFO curLabel=i->second;
        JSON curjsonlabel=json_object();
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
    labels.clear();
    const JSON jsonlabels=json_object_get(root, "labels");
    if(jsonlabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonlabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod='\0';
            curLabel.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual=true;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curLabel.text, text);
            else
                continue; //skip
            const uint key=modhashfromname(curLabel.mod)+curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
    JSON jsonautolabels=json_object_get(root, "autolabels");
    if(jsonautolabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautolabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curLabel.mod, mod);
            else
                *curLabel.mod='\0';
            curLabel.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual=false;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curLabel.text, text);
            else
                continue; //skip
            const uint key=modhashfromname(curLabel.mod)+curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
}

///bookmark functions
bool bookmarkset(uint addr, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    BOOKMARKSINFO bookmark;
    modnamefromaddr(addr, bookmark.mod, true);
    bookmark.addr=addr-modbasefromaddr(addr);
    bookmark.manual=manual;
    if(!bookmarks.insert(std::make_pair(modhashfromva(addr), bookmark)).second)
        return bookmarkdel(addr);
    return true;
}

bool bookmarkget(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(bookmarks.count(modhashfromva(addr)))
        return true;
    return false;
}

bool bookmarkdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    return (bookmarks.erase(modhashfromva(addr))>0);
}

void bookmarkcachesave(JSON root)
{
    const JSON jsonbookmarks=json_array();
    const JSON jsonautobookmarks=json_array();
    for(BookmarksInfo::iterator i=bookmarks.begin(); i!=bookmarks.end(); ++i)
    {
        const BOOKMARKSINFO curBookmark=i->second;
        JSON curjsonbookmark=json_object();
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
    bookmarks.clear();
    const JSON jsonbookmarks=json_object_get(root, "bookmarks");
    if(jsonbookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonbookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curBookmark.mod, mod);
            else
                *curBookmark.mod='\0';
            curBookmark.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual=true;
            const uint key=modhashfromname(curBookmark.mod)+curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
    JSON jsonautobookmarks=json_object_get(root, "autobookmarks");
    if(jsonautobookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautobookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curBookmark.mod, mod);
            else
                *curBookmark.mod='\0';
            curBookmark.addr=(uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual=false;
            const uint key=modhashfromname(curBookmark.mod)+curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
}

///function database
bool functionadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end<start)
        return false;
    uint page=memfindbaseaddr(fdProcessInfo->hProcess, start, 0);
    if(!page or page!=memfindbaseaddr(fdProcessInfo->hProcess, end, 0)) //the function boundaries are not in the same mem page
        return false;
    if(functionoverlaps(start, end))
        return false;
    FUNCTIONSINFO function;
    modnamefromaddr(start, function.mod, true);
    uint modbase=modbasefromaddr(start);
    function.start=start-modbase;
    function.end=end-modbase;
    function.manual=manual;
    functions.insert(std::make_pair(ModuleRange(modhashfromva(modbase), Range(function.start, function.end)), function));
    return true;
}

bool functionget(uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    uint modbase=modbasefromaddr(addr);
    const FunctionsInfo::iterator found=functions.find(ModuleRange(modhashfromva(modbase), Range(addr-modbase, addr-modbase)));
    if(found==functions.end()) //not found
        return false;
    if(start)
        *start=found->second.start+modbase;
    if(end)
        *end=found->second.end+modbase;
    return true;
}

bool functionoverlaps(uint start, uint end)
{
    if(!DbgIsDebugging() or end<start)
        return false;
    const uint modbase=modbasefromaddr(start);
    return (functions.count(ModuleRange(modhashfromva(modbase), Range(start-modbase, end-modbase)))>0);
}

bool functiondel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    const uint modbase=modbasefromaddr(addr);
    return (functions.erase(ModuleRange(modhashfromva(modbase), Range(addr-modbase, addr-modbase)))>0);
}

void functioncachesave(JSON root)
{
    const JSON jsonfunctions=json_array();
    const JSON jsonautofunctions=json_array();
    for(FunctionsInfo::iterator i=functions.begin(); i!=functions.end(); ++i)
    {
        const FUNCTIONSINFO curFunction=i->second;
        JSON curjsonfunction=json_object();
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
    functions.clear();
    const JSON jsonfunctions=json_object_get(root, "functions");
    if(jsonfunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonfunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curFunction.mod, mod);
            else
                *curFunction.mod='\0';
            curFunction.start=(uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end=(uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual=true;
            const uint key=modhashfromname(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(modhashfromname(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
    JSON jsonautofunctions=json_object_get(root, "autofunctions");
    if(jsonautofunctions)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautofunctions, i, value)
        {
            FUNCTIONSINFO curFunction;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curFunction.mod, mod);
            else
                *curFunction.mod='\0';
            curFunction.start=(uint)json_hex_value(json_object_get(value, "start"));
            curFunction.end=(uint)json_hex_value(json_object_get(value, "end"));
            if(curFunction.end < curFunction.start)
                continue; //invalid function
            curFunction.manual=true;
            const uint key=modhashfromname(curFunction.mod);
            functions.insert(std::make_pair(ModuleRange(modhashfromname(curFunction.mod), Range(curFunction.start, curFunction.end)), curFunction));
        }
    }
}

//loop database
bool loopadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end<start)
        return false;
    uint page=memfindbaseaddr(fdProcessInfo->hProcess, start, 0);
    if(!page or page!=memfindbaseaddr(fdProcessInfo->hProcess, end, 0)) //the function boundaries are not in the same mem page
        return false;
    int finaldepth;
    if(loopoverlaps(0, start, end, &finaldepth)) //loop cannot overlap another loop
        return false;
    LOOPSINFO loop;
    modnamefromaddr(start, loop.mod, true);
    const uint modbase=modbasefromaddr(start);
    loop.start=start-modbase;
    loop.end=end-modbase;
    loop.depth=finaldepth;
    if(finaldepth)
        loopget(finaldepth-1, start, &loop.parent, 0);
    else
        loop.parent=0;
    loop.manual=manual;
    loops.insert(std::make_pair(DepthModuleRange(finaldepth, ModuleRange(modhashfromva(modbase), Range(loop.start, loop.end))), loop));
    return true;
}

bool loopget(int depth, uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    const uint modbase=modbasefromaddr(addr);
    LoopsInfo::iterator found=loops.find(DepthModuleRange(depth, ModuleRange(modhashfromva(modbase), Range(addr-modbase, addr-modbase))));
    if(found==loops.end()) //not found
        return false;
    if(start)
        *start=found->second.start+modbase;
    if(end)
        *end=found->second.end+modbase;
    return true;
}

//check if a loop overlaps a range, inside is not overlapping
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth)
{
    if(!DbgIsDebugging())
        return false;

    const uint modbase=modbasefromaddr(start);
    uint curStart=start-modbase;
    uint curEnd=end-modbase;
    const uint key=modhashfromva(modbase);

    //check if the new loop fits in the old loop
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        if(i->first.second.first!=key) //only look in the current module
            continue;
        LOOPSINFO* curLoop=&i->second;
        if(curLoop->start<curStart and curLoop->end>curEnd and curLoop->depth==depth)
            return loopoverlaps(depth+1, curStart, curEnd, finaldepth);
    }

    if(finaldepth)
        *finaldepth=depth;

    //check for loop overlaps
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        if(i->first.second.first!=key) //only look in the current module
            continue;
        LOOPSINFO* curLoop=&i->second;
        if(curLoop->start<=curEnd and curLoop->end>=curStart and curLoop->depth==depth)
            return true;
    }
    return false;
}

bool loopdel(int depth, uint addr)
{
    return false;
}

void loopcachesave(JSON root)
{
    const JSON jsonloops=json_array();
    const JSON jsonautoloops=json_array();
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        const LOOPSINFO curLoop=i->second;
        JSON curjsonloop=json_object();
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
    loops.clear();
    const JSON jsonloops=json_object_get(root, "loops");
    if(jsonloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curLoop.mod, mod);
            else
                *curLoop.mod='\0';
            curLoop.start=(uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end=(uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth=(int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent=(uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual=true;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(modhashfromname(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
    JSON jsonautoloops=json_object_get(root, "autoloops");
    if(jsonautoloops)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautoloops, i, value)
        {
            LOOPSINFO curLoop;
            const char* mod=json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod)<MAX_MODULE_SIZE)
                strcpy(curLoop.mod, mod);
            else
                *curLoop.mod='\0';
            curLoop.start=(uint)json_hex_value(json_object_get(value, "start"));
            curLoop.end=(uint)json_hex_value(json_object_get(value, "end"));
            curLoop.depth=(int)json_integer_value(json_object_get(value, "depth"));
            curLoop.parent=(uint)json_hex_value(json_object_get(value, "parent"));
            if(curLoop.end < curLoop.start)
                continue; //invalid loop
            curLoop.manual=false;
            loops.insert(std::make_pair(DepthModuleRange(curLoop.depth, ModuleRange(modhashfromname(curLoop.mod), Range(curLoop.start, curLoop.end))), curLoop));
        }
    }
}