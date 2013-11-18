#include "breakpoint.h"
#include "debugger.h"
#include "addrinfo.h"
#include "sqlhelper.h"
#include "console.h"

static BREAKPOINT bpall[1000];
static int bpcount=0;

int bpgetlist(BREAKPOINT** list)
{
    if(list)
        *list=bpall;
    return bpcount;
}

bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, int titantype, const char* name)
{
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    char sql[deflen]="";
    uint modbase=modbasefromaddr(addr);
    if(bpget(addr, type, name, 0)) //breakpoint found
        return false;
    char bpname[MAX_BREAKPOINT_NAME]="";
    if(name and *name)
    {
        sqlstringescape(name, bpname);
        sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype,mod,name) VALUES (%"fext"d,%d,%d,%d,%d,%d,'%s','%s')", addr-modbase, enabled, singleshoot, oldbytes, type, titantype, modname, bpname);
    }
    else
        sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype,mod) VALUES (%"fext"d,%d,%d,%d,%d,%d,'%s')", addr-modbase, enabled, singleshoot, oldbytes, type, titantype, modname);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    return true;
}

bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp)
{
    char sql[deflen]="";
    char modname[256]="";
    char bpname[MAX_BREAKPOINT_NAME]="";
    uint modbase=0;
    if(!modnamefromaddr(addr, modname)) //no module
    {
        if(bp)
            *bp->mod=0;
        if(name and *name)
        {
            sqlstringescape(name, bpname);
            sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE (addr=%"fext"d AND type=%d AND mod IS NULL) OR name='%s'", addr, type, bpname);
        }
        else
            sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE (addr=%"fext"d AND type=%d AND mod IS NULL)", addr, type);
    }
    else
    {
        if(bp)
            strcpy(bp->mod, modname);
        modbase=modbasefromaddr(addr);
        if(name and *name)
        {
            sqlstringescape(name, bpname);
            sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE (addr=%"fext"d AND type=%d AND mod='%s') OR name='%s'", addr-modbase, type, modname, bpname);
            puts(sql);
        }
        else
            sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE (addr=%"fext"d AND type=%d AND mod='%s')", addr-modbase, type, modname);
    }
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(userdb, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(!bp) //just check if a breakpoint exists
    {
        sqlite3_finalize(stmt);
        return true;
    }
    if(!modbase)
    {
        const char* mod=(const char*)sqlite3_column_text(stmt, 6);
        if(mod)
            modbase=modbasefromname(mod);
    }
    //addr
#ifdef _WIN64
    bp->addr=sqlite3_column_int64(stmt, 0)+modbase; //addr
#else
    bp->addr=sqlite3_column_int(stmt, 0)+modbase; //addr
#endif // _WIN64
    //enabled
    if(sqlite3_column_int(stmt, 1))
        bp->enabled=true;
    else
        bp->enabled=false;
    //singleshoot
    if(sqlite3_column_int(stmt, 2))
        bp->singleshoot=true;
    else
        bp->singleshoot=false;
    //oldbytes
    bp->oldbytes=(short)(sqlite3_column_int(stmt, 3)&0xFFFF);
    //type
    bp->type=(BP_TYPE)sqlite3_column_int(stmt, 4);
    //titantype
    bp->titantype=sqlite3_column_int(stmt, 5);
    //name
    const char* bpname_=(const char*)sqlite3_column_text(stmt, 7);
    if(bpname_)
        strcpy(bp->name, bpname_);
    else
        *bp->name=0;
    sqlite3_finalize(stmt);
    return true;
}

bool bpdel(uint addr, BP_TYPE type)
{
    BREAKPOINT found;
    if(!bpget(addr, type, 0, &found))
        return false;
    char modname[256]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname)) //no module
        sprintf(sql, "DELETE FROM breakpoints WHERE addr=%"fext"d AND IS NULL AND type=%d", addr, type);
    else
        sprintf(sql, "DELETE FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    return true;
}

bool bpenable(uint addr, BP_TYPE type, bool enable)
{
    BREAKPOINT found;
    if(!bpget(addr, type, 0, &found))
        return false;
    char modname[256]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname)) //no module
        sprintf(sql, "UPDATE breakpoints SET enabled=%d WHERE addr=%"fext"d AND mod IS NULL AND type=%d", enable, addr, type);
    else
        sprintf(sql, "UPDATE breakpoints SET enabled=%d WHERE addr=%"fext"d AND mod='%s' AND type=%d", enable, addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    dbsave();
    bpenumall(0); //update breakpoint list
    return true;
}

bool bpsetname(uint addr, BP_TYPE type, const char* name)
{
    if(!name)
        return false;
    char modname[256]="";
    char sql[deflen]="";
    char bpname[MAX_BREAKPOINT_NAME]="";
    sqlstringescape(name, bpname);
    if(!modnamefromaddr(addr, modname)) //no module
        sprintf(sql, "UPDATE breakpoints SET name='%s' WHERE addr=%"fext"d AND mod IS NULL AND type=%d", bpname, addr, type);
    else
        sprintf(sql, "UPDATE breakpoints SET name='%s' WHERE addr=%"fext"d AND mod='%s' AND type=%d", bpname, addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    dbsave();
    return true;
}

bool bpenumall(BPENUMCALLBACK cbEnum, const char* module)
{
    bool retval=true;
    if(!cbEnum)
        bpcount=0;
    char sql[deflen]="";
    if(!module)
        sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints");
    else
        sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE mod='%s'", module);
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(userdb, sql, -1, &stmt, 0)!=SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    if(sqlite3_step(stmt)!=SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return false;
    }
    BREAKPOINT curbp;
    do
    {
#ifdef _WIN64
        uint rva=sqlite3_column_int64(stmt, 0); //addr
#else
        uint rva=sqlite3_column_int(stmt, 0); //addr
#endif // _WIN64
        if(sqlite3_column_int(stmt, 1)) //enabled
            curbp.enabled=true;
        else
            curbp.enabled=false;
        if(sqlite3_column_int(stmt, 2)) //singleshoot
            curbp.singleshoot=true;
        else
            curbp.singleshoot=false;
        curbp.oldbytes=(short)(sqlite3_column_int(stmt, 3)&0xFFFF); //oldbytes
        curbp.type=(BP_TYPE)sqlite3_column_int(stmt, 4); //type
        curbp.titantype=sqlite3_column_int(stmt, 5); //titantype
        const char* modname=(const char*)sqlite3_column_text(stmt, 6); //mod
        const char* bpname=(const char*)sqlite3_column_text(stmt, 7); //name
        if(bpname)
            strcpy(curbp.name, bpname);
        else
            *curbp.name=0;
        uint modbase=modbasefromname(modname);
        if(!modbase) //module not loaded
            *curbp.mod=0;
        curbp.addr=modbase+rva;
        if(cbEnum)
        {
            if(!cbEnum(&curbp))
                retval=false;
        }
        else if(bpcount<1000)
        {
            memcpy(&bpall[bpcount], &curbp, sizeof(BREAKPOINT));
            bpcount++;
        }
    }
    while(sqlite3_step(stmt)==SQLITE_ROW);
    sqlite3_finalize(stmt);
    return retval;
}

bool bpenumall(BPENUMCALLBACK cbEnum)
{
    return bpenumall(cbEnum, 0);
}

int bpgetcount(BP_TYPE type)
{
    char sql[deflen]="";
    sprintf(sql, "SELECT * FROM breakpoints WHERE type=%d", type);
    return sqlrowcount(userdb, sql);
}
