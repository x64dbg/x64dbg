#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "breakpoint.h"
#include "threading.h"
#include "symbolinfo.h"

static ModulesInfo modinfo;
static CommentsInfo comments;
static LabelsInfo labels;
static BookmarksInfo bookmarks;
static FunctionsInfo functions;
static LoopsInfo loops;

//database functions
void dbsave()
{
    JSON root=json_object();
    commentcachesave(root);
    json_dump_file(root, dbpath, JSON_INDENT(4)|JSON_SORT_KEYS);
    json_decref(root); //free root
}

void dbload()
{
    JSON root=json_load_file(dbpath, 0, 0);
    if(!root)
        return;
    commentcacheload(root);
    json_decref(root); //free root
}

void dbupdate()
{
    dbsave(); //flush cache to disk
    dbload(); //load database to cache (and update the module bases + VAs)
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
    len=strlen(name);
    name[MAX_MODULE_SIZE-1]=0; //ignore later characters
    while(name[len]!='.' and len)
        len--;
    MODINFO info;
    memset(&info, 0, sizeof(MODINFO));
    if(len)
    {
        strcpy(info.extension, name+len);
        _strlwr(info.extension);
        name[len]=0; //remove extension
    }
    info.base=base;
    info.size=size;
    strcpy(info.name, name);
    _strlwr(info.name);
    modinfo.push_back(info);
    symupdatemodulelist();
    dbupdate();
    return true;
}

bool modunload(uint base)
{
    dbupdate();
    int total=modinfo.size();
    for(int i=0; i<total; i++)
    {
        if(modinfo.at(i).base==base)
        {
            modinfo.erase(modinfo.begin()+i);
            symupdatemodulelist();
            return true;
        }
    }
    return false;
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
    *modname=0;
    int total=modinfo.size();
    for(int i=0; i<total; i++)
    {
        uint modstart=modinfo.at(i).base;
        uint modend=modstart+modinfo.at(i).size;
        if(addr>=modstart and addr<modend) //found module
        {
            strcpy(modname, modinfo.at(i).name);
            if(extension)
                strcat(modname, modinfo.at(i).extension); //append extension
            return true;
        }
    }
    return false;
}

uint modbasefromaddr(uint addr)
{
    int total=modinfo.size();
    for(int i=0; i<total; i++)
    {
        uint modstart=modinfo.at(i).base;
        uint modend=modstart+modinfo.at(i).size;
        if(addr>=modstart and addr<modend) //found module
            return modstart;
    }
    return 0;
}

uint modbasefromname(const char* modname)
{
    if(!modname)
        return 0;
    int total=modinfo.size();
    int modname_len=strlen(modname);
    if(modname_len>=MAX_MODULE_SIZE)
        return 0;
    for(int i=0; i<total; i++)
    {
        char curmodname[MAX_MODULE_SIZE]="";
        sprintf(curmodname, "%s%s", modinfo.at(i).name, modinfo.at(i).extension);
        if(!_stricmp(curmodname, modname)) //with extension
            return modinfo.at(i).base;
        if(!_stricmp(modinfo.at(i).name, modname)) //without extension
            return modinfo.at(i).base;
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
    if(comments.count(addr)) //contains addr
        comments[addr]=comment;
    else
        comments.insert(std::make_pair(addr, comment));
    return true;
}

bool commentget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    if(comments.count(addr)) //contains
    {
        strcpy(text, comments[addr].text);
        return true;
    }
    return false;
}

bool commentdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(comments.count(addr)) //contains
    {
        comments.erase(addr);
        return true;
    }
    return false;
}

void commentcachesave(JSON root)
{
    JSON jsoncomments=json_array();
    JSON jsonautocomments=json_array();
    for(CommentsInfo::iterator i=comments.begin(); i!=comments.end(); ++i)
    {
        COMMENTSINFO curComment=i->second;
        JSON curjsoncomment=json_object();
        if(*curComment.mod)
            json_object_set_new(curjsoncomment, "module", json_string(curComment.mod));
        else
            json_object_set_new(curjsoncomment, "module", json_null());
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
    JSON jsoncomments=json_object_get(root, "comments");
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
            curComment.addr=json_hex_value(json_object_get(value, "address"));
            if(!curComment.addr)
                continue; //skip
            curComment.manual=true;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curComment.text, text);
            else
                continue; //skip
            uint modbase=modbasefromname(curComment.mod);
            comments.insert(std::make_pair(curComment.addr+modbase, curComment));
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
            curComment.addr=json_hex_value(json_object_get(value, "address"));
            if(!curComment.addr)
                continue; //skip
            curComment.manual=false;
            const char* text=json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy(curComment.text, text);
            else
                continue; //skip
            uint modbase=modbasefromname(curComment.mod);
            comments.insert(std::make_pair(curComment.addr+modbase, curComment));
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
    if(labels.count(addr)) //contains
        labels[addr]=label;
    else
        labels.insert(std::make_pair(addr, label));
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
                *addr=i->first;
            return true;
        }
    }
    return false;
}

bool labelget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    if(labels.count(addr)) //contains
    {
        strcpy(text, labels[addr].text);
        return true;
    }
    return false;
}

bool labeldel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(labels.count(addr))
    {
        labels.erase(addr);
        return true;
    }
    return false;
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
    bookmarks.insert(std::make_pair(addr, bookmark));
    return true;
}

bool bookmarkget(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(bookmarks.count(addr))
        return true;
    return false;
}

bool bookmarkdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(bookmarks.count(addr))
    {
        bookmarks.erase(addr);
        return true;
    }
    return false;
}

///function database
bool functionadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end<start or memfindbaseaddr(fdProcessInfo->hProcess, start, 0)!=memfindbaseaddr(fdProcessInfo->hProcess, end, 0)!=0) //the function boundaries are not in the same mem page
        return false;
    if(functionoverlaps(start, end))
        return false;
    FUNCTIONSINFO function;
    modnamefromaddr(start, function.mod, true);
    function.modbase=modbasefromaddr(start);
    function.start=start-function.modbase;
    function.end=end-function.modbase;
    function.manual=manual;
    functions.push_back(function);
    return true;
}

bool functionget(uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging())
        return false;
    for(FunctionsInfo::iterator i=functions.begin(); i!=functions.end(); ++i)
    {
        uint curAddr=addr-i->modbase;
        if(i->start<=curAddr and i->end>=curAddr)
        {
            if(start)
                *start=i->start+i->modbase;
            if(end)
                *end=i->end+i->modbase;
            return true;
        }
    }
    return false;
}

bool functionoverlaps(uint start, uint end)
{
    if(!DbgIsDebugging())
        return false;
    for(FunctionsInfo::iterator i=functions.begin(); i!=functions.end(); ++i)
    {
        uint curStart=start-i->modbase;
        uint curEnd=end-i->modbase;
        if(i->start<=curEnd and i->end>=curStart)
            return true;
    }
    return false;
}

bool functiondel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    for(FunctionsInfo::iterator i=functions.begin(); i!=functions.end(); ++i)
    {
        uint curAddr=addr-i->modbase;
        if(i->start<=curAddr and i->end>=curAddr)
        {
            functions.erase(i);
            return true;
        }
    }
    return false;
}

//loop database
bool loopadd(uint start, uint end, bool manual)
{
    if(!DbgIsDebugging() or end<start or memfindbaseaddr(fdProcessInfo->hProcess, start, 0)!=memfindbaseaddr(fdProcessInfo->hProcess, end, 0)!=0) //the function boundaries are not in the same mem page
        return false;
    int finaldepth;
    if(loopoverlaps(0, start, end, &finaldepth)) //loop cannot overlap another loop
        return false;
    LOOPSINFO loop;
    modnamefromaddr(start, loop.mod, true);
    loop.modbase=modbasefromaddr(start);
    loop.start=start-loop.modbase;
    loop.end=end-loop.modbase;
    loop.depth=finaldepth;
    if(finaldepth)
        loop.parent=finaldepth-1;
    else
        loop.parent=0;
    loop.manual=manual;
    return false;
}

bool loopget(int depth, uint addr, uint* start, uint* end)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        uint curAddr=addr-i->modbase;
        if(i->start<=curAddr and i->end>=curAddr and i->depth==depth)
        {
            if(start)
                *start=i->start+i->modbase;
            if(end)
                *end=i->end+i->modbase;
            return true;
        }
    }
    return false;
}

//check if a loop overlaps a range, inside is not overlapping
bool loopoverlaps(int depth, uint start, uint end, int* finaldepth)
{
    if(!DbgIsDebugging())
        return false;
    //check if the new loop fits in the old loop
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        uint curStart=start-i->modbase;
        uint curEnd=end-i->modbase;
        if(i->start<curStart and i->end>curEnd and i->depth==depth)
            return loopoverlaps(depth+1, start, end, finaldepth);
    }

    if(finaldepth)
        *finaldepth=depth;

    //check for loop overlaps
    for(LoopsInfo::iterator i=loops.begin(); i!=loops.end(); ++i)
    {
        uint curStart=start-i->modbase;
        uint curEnd=end-i->modbase;
        if(i->start<=curEnd and i->end>=curStart and i->depth==depth)
            return true;
    }
    return false;
}

bool loopdel(int depth, uint addr)
{
    return false;
}
