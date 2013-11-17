#include "breakpoint.h"
#include "debugger.h"
#include "addrinfo.h"
#include "sqlhelper.h"
#include "console.h"

static uint bpaddrs[1000];
static int bptitantype[1000];
static int bpcount=0;

int bpgetlist(uint** list, int** type)
{
    if(!list or !type)
        return bpcount;
    *list=bpaddrs;
    *type=bptitantype;
    return bpcount;
}

bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, int titantype)
{
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    char sql[256]="";
    uint modbase=modbasefromaddr(addr);
    sprintf(sql, "SELECT * FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbase, modname, type);
    if(sqlhasresult(userdb, sql)) //no breakpoint set
        return false;
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

bool bpget(uint addr, BP_TYPE type, BREAKPOINT* bp)
{
    char sql[256]="";
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    uint modbase=modbasefromaddr(addr);
    sprintf(sql, "SELECT enabled,singleshoot,oldbytes,type,titantype,name FROM breakpoints WHERE addr=%"fext"d AND type=%d AND mod='%s'", addr-modbase, type, modname);
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
    //addr
    bp->addr=addr;
    //enabled
    if(sqlite3_column_int(stmt, 0))
        bp->enabled=true;
    else
        bp->enabled=false;
    //singleshoot
    if(sqlite3_column_int(stmt, 1))
        bp->singleshoot=true;
    else
        bp->singleshoot=false;
    //oldbytes
    bp->oldbytes=(short)(sqlite3_column_int(stmt, 2)&0xFFFF);
    //type
    bp->type=(BP_TYPE)sqlite3_column_int(stmt, 3);
    //titantype
    bp->titantype=sqlite3_column_int(stmt, 4);
    //name
    const char* name=(const char*)sqlite3_column_text(stmt, 5);
    if(name)
        strcpy(bp->name, name);
    else
        *bp->name=0;
    sqlite3_finalize(stmt);
    return true;
}

bool bpdel(uint addr, BP_TYPE type)
{
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    uint modbase=modbasefromaddr(addr);
    char sql[256]="";
    sprintf(sql, "SELECT * FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbase, modname, type);
    if(!sqlhasresult(userdb, sql)) //no breakpoint
        return false;
    sprintf(sql, "DELETE FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbase, modname, type);
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
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    uint modbase=modbasefromaddr(addr);
    char sql[256]="";
    sprintf(sql, "SELECT * FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbase, modname, type);
    if(!sqlhasresult(userdb, sql)) //no breakpoint
        return false;
    sprintf(sql, "UPDATE breakpoints SET enabled=%d WHERE addr=%"fext"d AND mod='%s' AND type=%d", enable, addr-modbase, modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    dbsave();
    return true;
}

bool bpsetname(uint addr, BP_TYPE type, const char* name)
{
    if(!name)
        return false;
    char modname[256]="";
    if(!modnamefromaddr(addr, modname)) //no module
        return false;
    uint modbase=modbasefromaddr(addr);
    char sql[256]="";
    sprintf(sql, "SELECT * FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbase, modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\n", sqllasterror());
        return false;
    }
    char bpname[MAX_BREAKPOINT_NAME]="";
    sqlstringescape(name, bpname);
    sprintf(sql, "UPDATE breakpoints SET name='%s' WHERE addr=%"fext"d AND mod='%s' AND type=%d", bpname, addr-modbase, modname, type);
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
    if(!cbEnum)
        bpcount=0;
    char sql[256]="";
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
            continue;
        curbp.addr=modbase+rva;
        if(cbEnum)
            cbEnum(&curbp);
        else if(bpcount<1000 and curbp.type==BPNORMAL)
        {
            bpaddrs[bpcount]=curbp.addr;
            bptitantype[bpcount]=curbp.titantype;
            bpcount++;
        }
    }
    while(sqlite3_step(stmt)==SQLITE_ROW);
    sqlite3_finalize(stmt);
    return true;
}

bool bpenumall(BPENUMCALLBACK cbEnum)
{
    return bpenumall(cbEnum, 0);
}
