#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"

static sqlite3* db;
static sqlite3* internaldb;

///basic database functions
void dbinit()
{
    //initialize user database
    if(sqlite3_open(":memory:", &db))
    {
        dputs("failed to open database!");
        return;
    }
    dbload();
    char sql[deflen]="";
    char* errorText=0;
    strcpy(sql, "CREATE TABLE IF NOT EXISTS comments (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, addr INT64 NOT NULL, text TEXT NOT NULL)");
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
    }
    strcpy(sql, "CREATE TABLE IF NOT EXISTS labels (id INTEGER PRIMARY KEY AUTOINCREMENT, mod TEXT, addr INT64 NOT NULL, text TEXT NOT NULL)");
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
    }
    //initialize internal database
    if(sqlite3_open(":memory:", &internaldb))
    {
        dputs("failed to open database!");
        return;
    }
    strcpy(sql, "CREATE TABLE IF NOT EXISTS exports (id INTEGER PRIMARY KEY AUTOINCREMENT, base INT64 NOT NULL, mod TEXT, name TEXT NOT NULL, addr INT64 NOT NULL)");
    if(sqlite3_exec(internaldb, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
    }
}

static int loadOrSaveDb(sqlite3* memory, const char* file, bool save)
{
    //CREDIT: http://www.sqlite.org/backup.html
    int rc;
    sqlite3* pFile;
    sqlite3_backup* pBackup;
    sqlite3* pTo;
    sqlite3* pFrom;
    rc=sqlite3_open(file, &pFile);
    if(rc==SQLITE_OK)
    {
        pFrom=(save?memory:pFile);
        pTo=(save?pFile:memory);
        pBackup=sqlite3_backup_init(pTo, "main", pFrom, "main");
        if(pBackup)
        {
            sqlite3_backup_step(pBackup, -1);
            sqlite3_backup_finish(pBackup);
        }
        rc=sqlite3_errcode(pTo);
    }
    sqlite3_close(pFile);
    return rc;
}

bool dbload()
{
    if(loadOrSaveDb(db, dbpath, false)!=SQLITE_OK)
        return false;
    return true;
}

bool dbsave()
{
    DeleteFileA("internal.db");
    loadOrSaveDb(internaldb, "internal.db", true);

    CreateDirectoryA(sqlitedb_basedir, 0); //create database directory
    if(loadOrSaveDb(db, dbpath, true)!=SQLITE_OK)
        return false;
    return true;
}

void dbclose()
{
    dbsave();
    sqlite3_db_release_memory(db);
    sqlite3_close(db); //close program database
    sqlite3_db_release_memory(internaldb);
    sqlite3_close(internaldb); //close internal database
}

///module functions

static std::vector<MODINFO> modinfo;

bool modnamefromaddr(uint addr, char* modname)
{
    int total=modinfo.size();
    for(int i=0; i<total; i++)
    {
        if(addr>=modinfo.at(i).start and addr<modinfo.at(i).end)
        {
            strcpy(modname, modinfo.at(i).name);
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
        if(addr>=modinfo.at(i).start and addr<modinfo.at(i).end)
        {
            return modinfo.at(i).start;
        }
    }
    return 0;
}

static void cbExport(uint base, const char* mod, const char* name, uint addr)
{
    char sql[deflen]="";
    sprintf(sql, "INSERT INTO exports (base,mod,name,addr) VALUES (%"fext"d,'%s','%s',%"fext"d)", base, mod, name, addr);
    char* errorText=0;
    if(sqlite3_exec(internaldb, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
    }
}

bool modload(uint base, uint size, const char* name)
{
    if(!base or !size or !name or strlen(name)>=31)
        return false;
    MODINFO info;
    info.start=base;
    info.end=base+size;
    strcpy(info.name, name);
    _strlwr(info.name);
    modinfo.push_back(info);
    apienumexports(base, cbExport);
    return true;
}

bool modunload(uint base)
{
    if(!base)
        return false;
    int total=modinfo.size();
    for(int i=0; i<total; i++)
    {
        if(modinfo.at(i).start==base)
        {
            modinfo.erase(modinfo.begin()+i);
            return true;
        }
    }
    return false;
}

void modclear()
{
    modinfo.clear();
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
    char modname[256]="";
    modnamefromaddr(base, modname);
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
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_COMMENT_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return commentdel(addr);
    int len=strlen(text);
    char* newtext=(char*)emalloc(len+1, "commentset:newtext");
    *newtext=0;
    for(int i=0,j=0; i<len; i++)
    {
        if(text[i]=='\"' or text[i]=='\'')
            j+=sprintf(newtext+j, "''");
        else
            j+=sprintf(newtext+j, "%c", text[i]);
    }
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //comments without module
    {
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            sqlite3_finalize(stmt);
            efree(newtext, "commentset:newtext");
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod IS NULL AND addr=%"fext"d", newtext, addr);
        else //insert
            sprintf(sql, "INSERT INTO comments (addr,text) VALUES (%"fext"d,'%s')", addr, newtext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, rva);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            sqlite3_finalize(stmt);
            efree(newtext, "commentset:newtext");
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod='%s' AND addr=%"fext"d", newtext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO comments (mod,addr,text) VALUES ('%s',%"fext"d,'%s')", modname, rva, newtext);
    }
    sqlite3_finalize(stmt);
    efree(newtext, "commentset:newtext");
    char* errorText=0;
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
        return false;
    }
    GuiUpdateAllViews();
    dbsave();
    return true;
}

bool commentget(uint addr, char* text)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text)
        return false;
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //comments without module
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, addr-modbasefromaddr(addr));
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //there is a comment already
    {
        sqlite3_finalize(stmt);
        return false;
    }
    strcpy(text, (const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return true;
}

bool commentdel(uint addr)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //comments without module
        sprintf(sql, "SELECT id FROM comments WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM comments WHERE mod='%s' AND addr=%"fext"d", modname, rva);
    }
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //no comment to delete
    {
        sqlite3_finalize(stmt);
        return false;
    }
    int del_id=sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    char* errorText=0;
    sprintf(sql, "DELETE FROM comments WHERE id=%d", del_id);
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
        return false;
    }
    GuiUpdateAllViews();
    dbsave();
    return true;
}

///label functions
bool labelset(uint addr, const char* text)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_LABEL_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return labeldel(addr);
    int len=strlen(text);
    char* newtext=(char*)emalloc(len+1, "labelset:newtext");
    *newtext=0;
    for(int i=0,j=0; i<len; i++)
    {
        if(text[i]=='\"' or text[i]=='\'')
            j+=sprintf(newtext+j, "''");
        else
            j+=sprintf(newtext+j, "%c", text[i]);
    }
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //labels without module
    {
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            sqlite3_finalize(stmt);
            efree(newtext, "labelset:newtext");
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod IS NULL AND addr=%"fext"d", newtext, addr);
        else //insert
            sprintf(sql, "INSERT INTO labels (addr,text) VALUES (%"fext"d,'%s')", addr, newtext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, rva);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            sqlite3_finalize(stmt);
            efree(newtext, "labelset:newtext");
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod='%s' AND addr=%"fext"d", newtext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO labels (mod,addr,text) VALUES ('%s',%"fext"d,'%s')", modname, rva, newtext);
    }
    sqlite3_finalize(stmt);
    efree(newtext, "labelset:newtext");
    char* errorText=0;
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
        return false;
    }
    GuiUpdateAllViews();
    dbsave();
    return true;
}

bool labelget(uint addr, char* text)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text)
        return false;
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //labels without module
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, addr-modbasefromaddr(addr));
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //there is a label already
    {
        sqlite3_finalize(stmt);
        return false;
    }
    strcpy(text, (const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return true;
}

bool labeldel(uint addr)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    char modname[35]="";
    char sql[256]="";
    sqlite3_stmt* stmt;
    if(!modnamefromaddr(addr, modname)) //labels without module
        sprintf(sql, "SELECT id FROM labels WHERE mod IS NULL AND addr=%"fext"d", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM labels WHERE mod='%s' AND addr=%"fext"d", modname, rva);
    }
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //no label to delete
    {
        sqlite3_finalize(stmt);
        return false;
    }
    int del_id=sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    char* errorText=0;
    sprintf(sql, "DELETE FROM labels WHERE id=%d", del_id);
    if(sqlite3_exec(db, sql, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        dprintf("SQL Error: %s\n", errorText);
        sqlite3_free(errorText);
        return false;
    }
    dbsave();
    GuiUpdateAllViews();
    return true;
}
