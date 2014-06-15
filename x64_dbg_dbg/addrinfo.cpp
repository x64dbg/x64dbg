#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"
#include "sqlhelper.h"
#include "breakpoint.h"
#include "threading.h"
#include "symbolinfo.h"

sqlite3* userdb;
static ModulesInfo modinfo;
static CommentsInfo comments;
static LabelsInfo labels;
static BookmarksInfo bookmarks;
static FunctionsInfo functions;
static LoopsInfo loops;

///basic database functions
void dbinit()
{
    //initialize user database
    lock(WAITID_USERDB);
    if(sqlite3_open(":memory:", &userdb))
    {
        unlock(WAITID_USERDB);
        dputs("failed to open database!");
        return;
    }
    unlock(WAITID_USERDB);
    sqlloadsavedb(userdb, dbpath, false);
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS labels (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, addr INT64 NOT NULL, text TEXT NOT NULL)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS comments (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, addr INT64 NOT NULL, text TEXT NOT NULL)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS bookmarks (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, addr INT64 NOT NULL)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS breakpoints (id INTEGER PRIMARY KEY AUTOINCREMENT, addr INT64 NOT NULL, enabled INT NOT NULL, singleshoot INT NOT NULL, oldbytes INT NOT NULL, type INT NOT NULL, titantype INT NOT NULL, mod TEXT, name TEXT)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS functions (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, start INT64 NOT NULL, end INT64 NOT NULL, manual BOOL NOT NULL)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    if(!sqlexec(userdb, "CREATE TABLE IF NOT EXISTS loops (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, start INT64 NOT NULL, end INT64 NOT NULL, parent INT, depth INT NOT NULL, manual BOOL NOT NULL)"))
        dprintf("SQL Error: %s\n", sqllasterror());
    bpenumall(0); //update breakpoint list
    GuiUpdateBreakpointsView();
}

bool dbload()
{
    if(!FileExists(dbpath))
    {
        dbinit();
        return true;
    }
    return sqlloadsavedb(userdb, dbpath, false);
}

bool dbsave()
{
    CreateDirectoryA(sqlitedb_basedir, 0); //create database directory
    return sqlloadsavedb(userdb, dbpath, true);
}

void dbclose()
{
    //NOTE: remove breakpoints without module
    if(!sqlexec(userdb, "DELETE FROM breakpoints WHERE mod IS NULL"))
        dprintf("SQL Error: %s\n", sqllasterror());
    //NOTE: remove singleshoot breakpoints (mostly temporary breakpoints)
    if(!sqlexec(userdb, "DELETE FROM breakpoints WHERE singleshoot=1 AND type=0"))
        dprintf("SQL Error: %s\n", sqllasterror());
    dbsave();
    wait(WAITID_USERDB); //wait for the SQLite operation to complete before closing
    lock(WAITID_USERDB);
    sqlite3_db_release_memory(userdb);
    sqlite3_close(userdb); //close user database
    unlock(WAITID_USERDB);
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
    return true;
}

bool modunload(uint base)
{
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
bool commentset(uint addr, const char* text)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_COMMENT_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return commentdel(addr);
    COMMENTSINFO info;
    sqlstringescape(text, info.text);
    modnamefromaddr(addr, info.mod, true);
    info.addr=addr-modbasefromaddr(addr);
    if(comments.count(addr)) //contains addr
        comments[addr]=info;
    else
        comments.insert(std::make_pair(addr, info));
    return true;
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_COMMENT_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return commentdel(addr);
    char commenttext[MAX_COMMENT_SIZE]="";
    sqlstringescape(text, commenttext);
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //comments without module
    {
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
        if(sqlhasresult(userdb, sql)) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod IS NULL AND addr=%"fext"d", commenttext, addr);
        else //insert
            sprintf(sql, "INSERT INTO comments (addr,text) VALUES (%"fext"d,'%s')", addr, commenttext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, rva);
        if(sqlhasresult(userdb, sql)) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod='%s' AND addr=%"fext"d", commenttext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO comments (mod,addr,text) VALUES ('%s',%"fext"d,'%s')", modname, rva, commenttext);
    }
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text)
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //comments without module
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, addr-modbasefromaddr(addr));
    return sqlgettext(userdb, sql, text);
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //comments without module
        sprintf(sql, "SELECT id FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, rva);
    }
    int del_id=0;
    if(!sqlgetint(userdb, sql, &del_id))
        return false;
    sprintf(sql, "DELETE FROM comments WHERE id=%d", del_id);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
}

///label functions
bool labelset(uint addr, const char* text)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_LABEL_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return labeldel(addr);
    LABELSINFO label;
    sqlstringescape(text, label.text);
    modnamefromaddr(addr, label.mod, true);
    label.addr=addr-modbasefromaddr(addr);
    if(labels.count(addr)) //contains
        labels[addr]=label;
    else
        labels.insert(std::make_pair(addr, label));
    return true;
    /*
    if(!modnamefromaddr(addr, modname, true)) //labels without module
    {
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
        if(sqlhasresult(userdb, sql)) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod IS NULL AND addr=%"fext"d", labeltext, addr);
        else //insert
            sprintf(sql, "INSERT INTO labels (addr,text) VALUES (%"fext"d,'%s')", addr, labeltext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, rva);
        if(sqlhasresult(userdb, sql)) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod='%s' AND addr=%"fext"d", labeltext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO labels (mod,addr,text) VALUES ('%s',%"fext"d,'%s')", modname, rva, labeltext);
    }
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
}

bool labelfromstring(const char* text, uint* addr)
{
    if(!DbgIsDebugging())
        return false;
    char labeltext[MAX_LABEL_SIZE]="";
    sqlstringescape(text, labeltext);
    for(LabelsInfo::iterator i=labels.begin(); i!=labels.end(); ++i)
    {
        if(!strcmp(i->second.text, labeltext))
        {
            if(addr)
                *addr=i->first;
            return true;
        }
    }
    return false;
    /*
    if(!text or !strlen(text) or !addr)
        return 0;
    char labeltext[MAX_LABEL_SIZE]="";
    sqlstringescape(text, labeltext);
    char sql[deflen]="";
    sprintf(sql, "SELECT addr,mod FROM labels WHERE text='%s'", labeltext);
    sqlite3_stmt* stmt;
    lock(WAITID_USERDB);
    if(sqlite3_prepare_v2(userdb, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
#ifdef _WIN64
    *addr=sqlite3_column_int64(stmt, 0); //addr
#else
    *addr=sqlite3_column_int(stmt, 0); //addr
#endif // _WIN64
    const char* modname=(const char*)sqlite3_column_text(stmt, 1); //mod
    if(!modname)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return true;
    }
    //TODO: fix this
    *addr+=modbasefromname(modname);
    sqlite3_finalize(stmt);
    unlock(WAITID_USERDB);
    return true;
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text)
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //labels without module
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, addr-modbasefromaddr(addr));
    return sqlgettext(userdb, sql, text);
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //labels without module
        sprintf(sql, "SELECT id FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, rva);
    }
    int del_id=0;
    if(!sqlgetint(userdb, sql, &del_id))
        return false;
    sprintf(sql, "DELETE FROM labels WHERE id=%d", del_id);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
}

///bookmark functions
bool bookmarkset(uint addr)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    BOOKMARKSINFO bookmark;
    modnamefromaddr(addr, bookmark.mod, true);
    bookmark.addr=addr-modbasefromaddr(addr);
    bookmarks.insert(std::make_pair(addr, bookmark));
    return true;
    /*
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //bookmarks without module
    {
        sprintf(sql, "SELECT * FROM bookmarks WHERE mod IS NULL AND addr=%"fext"d", addr);
        if(sqlhasresult(userdb, sql)) //there is a bookmark already
            return true;
        else //insert
            sprintf(sql, "INSERT INTO bookmarks (addr) VALUES (%"fext"d)", addr);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT * FROM bookmarks WHERE mod='%s' AND addr=%"fext"d", modname, rva);
        if(sqlhasresult(userdb, sql)) //there is a bookmark already
            return true;
        else //insert
            sprintf(sql, "INSERT INTO bookmarks (mod,addr) VALUES ('%s',%"fext"d)", modname, rva);
    }
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
}

bool bookmarkget(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    if(bookmarks.count(addr))
        return true;
    return false;
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //bookmarks without module
        sprintf(sql, "SELECT * FROM bookmarks WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
        sprintf(sql, "SELECT * FROM bookmarks WHERE mod='%s' AND addr=%"fext"d", modname, addr-modbasefromaddr(addr));
    return sqlhasresult(userdb, sql);
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //bookmarks without module
        sprintf(sql, "SELECT id FROM bookmarks WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM bookmarks WHERE mod='%s' AND addr=%"fext"d", modname, rva);
    }
    int del_id=0;
    if(!sqlgetint(userdb, sql, &del_id))
        return false;
    sprintf(sql, "DELETE FROM bookmarks WHERE id=%d", del_id);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
}

///function database
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    uint modbase=0;
    if(!modnamefromaddr(addr, modname, true))
        sprintf(sql, "SELECT start,end FROM functions WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d", addr, addr);
    else
    {
        modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT start,end FROM functions WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d", modname, rva, rva);
    }
    sqlite3_stmt* stmt;
    lock(WAITID_USERDB);
    if(sqlite3_prepare_v2(userdb, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
#ifdef _WIN64
    uint dbstart=sqlite3_column_int64(stmt, 0)+modbase; //start
    uint dbend=sqlite3_column_int64(stmt, 1)+modbase; //end
#else
    uint dbstart=sqlite3_column_int(stmt, 0)+modbase; //addr
    uint dbend=sqlite3_column_int(stmt, 1)+modbase; //end
#endif // _WIN64
    sqlite3_finalize(stmt);
    if(start)
        *start=dbstart;
    if(end)
        *end=dbend;
    unlock(WAITID_USERDB);
    return true;
    */
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
    /*
    char sql[deflen]="";
    char modname[MAX_MODULE_SIZE]="";
    //check for function overlaps
    if(!modnamefromaddr(start, modname, true))
        sprintf(sql, "SELECT manual FROM functions WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d", end, start);
    else
    {
        uint modbase=modbasefromaddr(start);
        sprintf(sql, "SELECT manual FROM functions WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d", modname, end-modbase, start-modbase);
    }
    if(sqlhasresult(userdb, sql)) //functions overlap
        return true;
    return false;
    */
}

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
    /*
    char sql[deflen]="";
    char modname[MAX_MODULE_SIZE]="";
    uint modbase=0;
    //check for function overlaps
    if(!modnamefromaddr(start, modname, true))
        sprintf(sql, "SELECT manual FROM functions WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d", end, start);
    else
    {
        modbase=modbasefromaddr(start);
        sprintf(sql, "SELECT manual FROM functions WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d", modname, end-modbase, start-modbase);
    }
    if(sqlhasresult(userdb, sql)) //functions overlap
        return false;
    if(modbase)
        sprintf(sql, "INSERT INTO functions (mod,start,end,manual) VALUES('%s',%"fext"d,%"fext"d,%d)", modname, start-modbase, end-modbase, manual);
    else
        sprintf(sql, "INSERT INTO functions (start,end,manual) VALUES(%"fext"d,%"fext"d,%d)", start, end, manual);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
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
    /*
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true))
        sprintf(sql, "DELETE FROM functions WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d", addr, addr);
    else
    {
        uint rva=addr-modbasefromaddr(addr);
        sprintf(sql, "DELETE FROM functions WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d", modname, rva, rva);
    }
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    return true;
    */
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
    /*
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[MAX_MODULE_SIZE]="";
    char sql[deflen]="";
    uint modbase=0;
    if(!modnamefromaddr(addr, modname, true))
        sprintf(sql, "SELECT start,end FROM loops WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d AND depth=%d", addr, addr, depth);
    else
    {
        modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT start,end FROM loops WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d AND depth=%d", modname, rva, rva, depth);
    }
    sqlite3_stmt* stmt;
    lock(WAITID_USERDB);
    if(sqlite3_prepare_v2(userdb, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return false;
    }
#ifdef _WIN64
    uint dbstart=sqlite3_column_int64(stmt, 0)+modbase; //start
    uint dbend=sqlite3_column_int64(stmt, 1)+modbase; //end
#else
    uint dbstart=sqlite3_column_int(stmt, 0)+modbase; //addr
    uint dbend=sqlite3_column_int(stmt, 1)+modbase; //end
#endif // _WIN64
    sqlite3_finalize(stmt);
    if(start)
        *start=dbstart;
    if(end)
        *end=dbend;
    unlock(WAITID_USERDB);
    return true;
    */
}

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
    /*
    char sql[deflen]="";
    char modname[MAX_MODULE_SIZE]="";

    //check if the new loop fits in the old loop
    if(!modnamefromaddr(start, modname, true))
        sprintf(sql, "SELECT manual FROM loops WHERE mod IS NULL AND start<%"fext"d AND end>%"fext"d AND depth=%d", start, end, depth);
    else
    {
        uint modbase=modbasefromaddr(start);
        sprintf(sql, "SELECT manual FROM loops WHERE mod='%s' AND start<%"fext"d AND end>%"fext"d AND depth=%d", modname, start-modbase, end-modbase, depth);
    }
    if(sqlhasresult(userdb, sql)) //new loop fits in the old loop
        return loopoverlaps(depth+1, start, end); //check the next depth
    
    //check for loop overlaps
    if(!modnamefromaddr(start, modname, true))
        sprintf(sql, "SELECT manual FROM loops WHERE mod IS NULL AND start<=%"fext"d AND end>=%"fext"d AND depth=%d", end, start, depth);
    else
    {
        uint modbase=modbasefromaddr(start);
        sprintf(sql, "SELECT manual FROM loops WHERE mod='%s' AND start<=%"fext"d AND end>=%"fext"d AND depth=%d", modname, end-modbase, start-modbase, depth);
    }
    if(finaldepth)
        *finaldepth=depth;
    if(sqlhasresult(userdb, sql)) //loops overlap
        return true;
    return false;
    */
}

bool loopdel(int depth, uint addr)
{
    return false;
}
