#include "breakpoint.h"
#include "debugger.h"
#include "addrinfo.h"
#include "sqlhelper.h"
#include "console.h"
#include "memory.h"
#include "threading.h"

static BREAKPOINT bpall[1000]; //TODO: fix this size
static int bpcount=0;

int bpgetlist(BREAKPOINT** list)
{
    if(list)
        *list=bpall;
    return bpcount;
}

bool bpnew(uint addr, bool enabled, bool singleshoot, short oldbytes, BP_TYPE type, DWORD titantype, const char* name)
{
    if(bpget(addr, type, name, 0)) //breakpoint found
        return false;
    char modname[256]="";
    char sql[deflen]="";
    char bpname[MAX_BREAKPOINT_SIZE]="";
    if(modnamefromaddr(addr, modname, true)) //no module
    {
        uint modbase=modbasefromaddr(addr);
        if(name and *name)
        {
            sqlstringescape(name, bpname);
            sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype,mod,name) VALUES (%"fext"d,%d,%d,%d,%d,%d,'%s','%s')", addr-modbase, enabled, singleshoot, oldbytes, type, titantype, modname, bpname);
        }
        else
            sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype,mod) VALUES (%"fext"d,%d,%d,%d,%d,%d,'%s')", addr-modbase, enabled, singleshoot, oldbytes, type, titantype, modname);
    }
    else
    {
        if(name and *name)
        {
            sqlstringescape(name, bpname);
            sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype,name) VALUES (%"fext"d,%d,%d,%d,%d,%d,'%s')", addr, enabled, singleshoot, oldbytes, type, titantype, bpname);
        }
        else
            sprintf(sql, "INSERT INTO breakpoints (addr,enabled,singleshoot,oldbytes,type,titantype) VALUES (%"fext"d,%d,%d,%d,%d,%d)", addr, enabled, singleshoot, oldbytes, type, titantype);
    }
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    GuiUpdateBreakpointsView();
    return true;
}

bool bpget(uint addr, BP_TYPE type, const char* name, BREAKPOINT* bp)
{
    char sql[deflen]="";
    char modname[256]="";
    char bpname[MAX_BREAKPOINT_SIZE]="";
    uint modbase=0;
    if(!modnamefromaddr(addr, modname, true)) //no module
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
        }
        else
            sprintf(sql, "SELECT addr,enabled,singleshoot,oldbytes,type,titantype,mod,name FROM breakpoints WHERE (addr=%"fext"d AND type=%d AND mod='%s')", addr-modbase, type, modname);
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
    if(!bp) //just check if a breakpoint exists
    {
        sqlite3_finalize(stmt);
        unlock(WAITID_USERDB);
        return true;
    }
    memset(bp, 0, sizeof(BREAKPOINT));
    if(!modbase)
    {
        const char* mod=(const char*)sqlite3_column_text(stmt, 6); //mod
        if(mod)
            modbase=modbasefromname(mod);
    }
#ifdef _WIN64
    bp->addr=sqlite3_column_int64(stmt, 0)+modbase; //addr
#else
    bp->addr=sqlite3_column_int(stmt, 0)+modbase; //addr
#endif // _WIN64
    if(sqlite3_column_int(stmt, 1)) //enabled
        bp->enabled=true;
    else
        bp->enabled=false;
    if(sqlite3_column_int(stmt, 2)) //singleshoot
        bp->singleshoot=true;
    else
        bp->singleshoot=false;
    bp->oldbytes=(short)(sqlite3_column_int(stmt, 3)&0xFFFF); //oldbytes
    bp->type=(BP_TYPE)sqlite3_column_int(stmt, 4); //type
    bp->titantype=sqlite3_column_int(stmt, 5); //titantype
    const char* bpname_=(const char*)sqlite3_column_text(stmt, 7); //name
    if(bpname_)
        strcpy(bp->name, bpname_);
    else
        *bp->name=0;
    //TODO: fix this
    if(memisvalidreadptr(fdProcessInfo->hProcess, bp->addr))
        bp->active=true;
    sqlite3_finalize(stmt);
    unlock(WAITID_USERDB);
    return true;
}

bool bpdel(uint addr, BP_TYPE type)
{
    BREAKPOINT found;
    if(!bpget(addr, type, 0, &found))
        return false;
    char modname[256]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //no module
        sprintf(sql, "DELETE FROM breakpoints WHERE addr=%"fext"d AND mod IS NULL AND type=%d", addr, type);
    else
        sprintf(sql, "DELETE FROM breakpoints WHERE addr=%"fext"d AND mod='%s' AND type=%d", addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    GuiUpdateBreakpointsView();
    return true;
}

bool bpenable(uint addr, BP_TYPE type, bool enable)
{
    BREAKPOINT found;
    if(!bpget(addr, type, 0, &found))
        return false;
    char modname[256]="";
    char sql[deflen]="";
    if(!modnamefromaddr(addr, modname, true)) //no module
        sprintf(sql, "UPDATE breakpoints SET enabled=%d WHERE addr=%"fext"d AND mod IS NULL AND type=%d", enable, addr, type);
    else
        sprintf(sql, "UPDATE breakpoints SET enabled=%d WHERE addr=%"fext"d AND mod='%s' AND type=%d", enable, addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    GuiUpdateBreakpointsView();
    return true;
}

bool bpsetname(uint addr, BP_TYPE type, const char* name)
{
    if(!name)
        return false;
    char modname[256]="";
    char sql[deflen]="";
    char bpname[MAX_BREAKPOINT_SIZE]="";
    sqlstringescape(name, bpname);
    if(!modnamefromaddr(addr, modname, true)) //no module
        sprintf(sql, "UPDATE breakpoints SET name='%s' WHERE addr=%"fext"d AND mod IS NULL AND type=%d", bpname, addr, type);
    else
        sprintf(sql, "UPDATE breakpoints SET name='%s' WHERE addr=%"fext"d AND mod='%s' AND type=%d", bpname, addr-modbasefromaddr(addr), modname, type);
    if(!sqlexec(userdb, sql))
    {
        dprintf("SQL Error: %s\nSQL Query: %s\n", sqllasterror(), sql);
        return false;
    }
    bpenumall(0); //update breakpoint list
    dbsave();
    GuiUpdateBreakpointsView();
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
        if(modname)
            strcpy(curbp.mod, modname);
        else
            *curbp.mod=0;
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
    unlock(WAITID_USERDB);
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

void bpfixmemory(uint addr, unsigned char* dest, uint size)
{
    uint start=addr;
    uint end=addr+size;
    unsigned char oldbytes[2];
    for(int i=0; i<bpcount; i++)
    {
        if(!bpall[i].enabled or bpall[i].type!=BPNORMAL)
            continue;
        memcpy(oldbytes, &bpall[i].oldbytes, sizeof(short));
        uint cur_addr=bpall[i].addr;
        if(cur_addr>=start and cur_addr<end) //breakpoint is in range of current memory
        {
            uint index=cur_addr-start;
            dest[index]=oldbytes[0];
            if(size>1 and index!=(size-1)) //restore second byte
                dest[index+1]=oldbytes[1];
        }
    }
}

void bptobridge(const BREAKPOINT* bp, BRIDGEBP* bridge)
{
    if(!bp or !bridge)
        return;
    memset(bridge, 0, sizeof(BRIDGEBP));
    bridge->active=bp->active;
    bridge->addr=bp->addr;
    bridge->enabled=bp->enabled;
    strcpy(bridge->mod, bp->mod);
    strcpy(bridge->name, bp->name);
    bridge->singleshoot=bp->singleshoot;
    switch(bp->type)
    {
    case BPNORMAL:
        bridge->type=bp_normal;
        break;
    case BPHARDWARE:
        bridge->type=bp_hardware;
        break;
    case BPMEMORY:
        bridge->type=bp_memory;
    default:
        bridge->type=bp_none;
    }
}
