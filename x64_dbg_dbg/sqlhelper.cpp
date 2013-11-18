#include "sqlhelper.h"
#include "console.h"

static char lasterror[deflen]="";

const char* sqllasterror()
{
    return lasterror;
}

bool sqlexec(sqlite3* db, const char* query)
{
    char* errorText=0;
    if(sqlite3_exec(db, query, 0, 0, &errorText)!=SQLITE_OK) //error
    {
        strcpy(lasterror, errorText);
        sqlite3_free(errorText);
        return false;
    }
    *lasterror=0;
    return true;
}

bool sqlhasresult(sqlite3* db, const char* query)
{
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

bool sqlgettext(sqlite3* db, const char* query, char* result)
{
    if(!result)
        return false;
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    strcpy(result, (const char*)sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    return true;
}

bool sqlgetint(sqlite3* db, const char* query, int* result)
{
    if(!result)
        return false;
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    *result=sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return true;
}

bool sqlgetuint(sqlite3* db, const char* query, uint* result)
{
    if(!result)
        return false;
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
#ifdef _WIN64
    *result=sqlite3_column_int64(stmt, 0);
#else
    *result=sqlite3_column_int(stmt, 0);
#endif // _WIN64
    sqlite3_finalize(stmt);
    return true;
}

void sqlstringescape(const char* string, char* escaped_string)
{
    if(!string or !escaped_string)
        return;
    int len=strlen(string);
    *escaped_string=0;
    for(int i=0,j=0; i<len; i++)
    {
        if(string[i]=='\"' or string[i]=='\'')
            j+=sprintf(escaped_string+j, "''");
        else
            j+=sprintf(escaped_string+j, "%c", string[i]);
    }
}

bool sqlloadsavedb(sqlite3* memory, const char* file, bool save)
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
    return (rc==SQLITE_OK);
}

int sqlrowcount(sqlite3* db, const char* query)
{
    int rowcount=0;
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, query, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    while(sqlite3_step(stmt)==SQLITE_ROW)
        rowcount++;
    return rowcount;
}
