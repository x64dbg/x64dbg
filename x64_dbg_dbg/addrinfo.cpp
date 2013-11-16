#include "addrinfo.h"
#include "debugger.h"
#include "console.h"
#include "memory.h"

static sqlite3* db;

void dbinit()
{
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
    CreateDirectoryA(sqlitedb_basedir, 0); //create database directory
    if(loadOrSaveDb(db, dbpath, true)!=SQLITE_OK)
        return false;
    return true;
}

void dbclose()
{
    dbsave();
    sqlite3_close(db);
}

bool modnamefromaddr(uint addr, char* modname)
{
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(!SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)addr, &modInfo) or !modname)
        return false;
    _strlwr(modInfo.ModuleName);
    strcpy(modname, modInfo.ModuleName);
    return true;
}

uint modbasefromaddr(uint addr)
{
    IMAGEHLP_MODULE64 modInfo;
    memset(&modInfo, 0, sizeof(modInfo));
    modInfo.SizeOfStruct=sizeof(IMAGEHLP_MODULE64);
    if(!SymGetModuleInfo64(fdProcessInfo->hProcess, (DWORD64)addr, &modInfo))
        return 0;
    return (uint)modInfo.BaseOfImage;
}

bool commentset(uint addr, const char* text)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_COMMENT_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return commentdel(addr);
    int len=strlen(text);
    char* newtext=(char*)emalloc(len+1);
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
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"u", addr);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod IS NULL AND addr=%"fext"u", newtext, addr);
        else //insert
            sprintf(sql, "INSERT INTO comments (addr,text) VALUES (%"fext"u,'%s')", addr, newtext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"u", modname, rva);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a comment already
            sprintf(sql, "UPDATE comments SET text='%s' WHERE mod='%s' AND addr=%"fext"u", newtext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO comments (mod,addr,text) VALUES ('%s',%"fext"u,'%s')", modname, rva, newtext);
    }
    sqlite3_finalize(stmt);
    efree(newtext);
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
        sprintf(sql, "SELECT text FROM comments WHERE mod IS NULL AND addr=%"fext"u", addr);
    else
        sprintf(sql, "SELECT text FROM comments WHERE mod='%s' AND addr=%"fext"u", modname, addr-modbasefromaddr(addr));
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //there is a comment already
    {
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
        sprintf(sql, "SELECT id FROM comments WHERE mod IS NULL AND addr=%"fext"u", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM comments WHERE mod='%s' AND addr=%"fext"u", modname, rva);
    }
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //no comment to delete
        return false;
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

bool labelset(uint addr, const char* text)
{
    if(!IsFileBeingDebugged() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or strlen(text)>=MAX_LABEL_SIZE-1)
        return false;
    if(!*text) //NOTE: delete when there is no text
        return labeldel(addr);
    int len=strlen(text);
    char* newtext=(char*)emalloc(len+1);
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
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"u", addr);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod IS NULL AND addr=%"fext"u", newtext, addr);
        else //insert
            sprintf(sql, "INSERT INTO labels (addr,text) VALUES (%"fext"u,'%s')", addr, newtext);
    }
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"u", modname, rva);
        if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
        {
            return false;
        }
        if(sqlite3_step(stmt)==SQLITE_ROW) //there is a label already
            sprintf(sql, "UPDATE labels SET text='%s' WHERE mod='%s' AND addr=%"fext"u", newtext, modname, rva);
        else //insert
            sprintf(sql, "INSERT INTO labels (mod,addr,text) VALUES ('%s',%"fext"u,'%s')", modname, rva, newtext);
    }
    sqlite3_finalize(stmt);
    efree(newtext);
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
        sprintf(sql, "SELECT text FROM labels WHERE mod IS NULL AND addr=%"fext"u", addr);
    else
        sprintf(sql, "SELECT text FROM labels WHERE mod='%s' AND addr=%"fext"u", modname, addr-modbasefromaddr(addr));
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //there is a label already
    {
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
        sprintf(sql, "SELECT id FROM labels WHERE mod IS NULL AND addr=%"fext"u", addr);
    else
    {
        uint modbase=modbasefromaddr(addr);
        uint rva=addr-modbase;
        sprintf(sql, "SELECT id FROM labels WHERE mod='%s' AND addr=%"fext"u", modname, rva);
    }
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW) //no label to delete
        return false;
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
