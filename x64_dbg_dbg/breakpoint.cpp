#include "breakpoint.h"
#include "debugger.h"

BREAKPOINT* bpinit(BREAKPOINT* breakpoint_list)
{
    bool bNext=true;
    if(!breakpoint_list)
        bNext=false;
    BREAKPOINT* cur=breakpoint_list;
    while(bNext)
    {
        BREAKPOINT* next=cur->next;
        bpdel(breakpoint_list, 0, cur->addr, BPNORMAL);
        cur=next;
        if(!cur)
            bNext=false;
    }
    BREAKPOINT* bp;
    if(!breakpoint_list)
        bp=(BREAKPOINT*)emalloc(sizeof(BREAKPOINT));
    else
        bp=breakpoint_list;
    memset(bp, 0, sizeof(BREAKPOINT));
    return bp;
}

BREAKPOINT* bpfind(BREAKPOINT* breakpoint_list, const char* name, uint addr, BREAKPOINT** link, BP_TYPE type)
{
    BREAKPOINT* cur=breakpoint_list;
    if(!cur or !cur->addr)
        return 0;
    BREAKPOINT* prev=0;
    while(cur)
    {
        BP_TYPE bptype=cur->type;
        if(bptype==BPSINGLESHOOT)
            bptype=BPNORMAL;
        BP_TYPE realtype=type;
        if(realtype==BPSINGLESHOOT)
            realtype=BPNORMAL;
        if(((name and arraycontains(cur->name, name)) or cur->addr==addr) and (type==BPNOTYPE or bptype==realtype))
        {
            if(link)
                *link=prev;
            return cur;
        }
        prev=cur;
        cur=cur->next;
    }
    return 0;
}

bool bpnew(BREAKPOINT* breakpoint_list, const char* name, uint addr, short oldbytes, BP_TYPE type)
{
    if(!breakpoint_list or !addr or bpfind(breakpoint_list, name, addr, 0, type))
        return false;
    BREAKPOINT* bp;
    bool nonext=false;
    if(!breakpoint_list->addr)
    {
        bp=breakpoint_list;
        nonext=true;
    }
    else
        bp=(BREAKPOINT*)emalloc(sizeof(BREAKPOINT));
    memset(bp, 0, sizeof(BREAKPOINT));
    if(name and *name)
    {
        bp->name=(char*)emalloc(strlen(name)+1);
        strcpy(bp->name, name);
    }
    bp->addr=addr;
    bp->type=type;
    bp->oldbytes=oldbytes;
    bp->enabled=true;
    BREAKPOINT* cur=breakpoint_list;
    if(!nonext)
    {
        while(cur->next)
            cur=cur->next;
        cur->next=bp;
    }
    return true;
}

bool bpsetname(BREAKPOINT* breakpoint_list, uint addr, const char* name)
{
    //TODO: fix this BPNOTYPE, it's bullshit
    if(!name or !*name or !addr or bpfind(breakpoint_list, name, 0, 0, BPNOTYPE))
        return false;
    BREAKPOINT* found=bpfind(breakpoint_list, 0, addr, 0, BPNOTYPE);
    if(!found)
        return false;
    efree(found->name); //free previous name
    found->name=(char*)emalloc(strlen(name)+1);
    strcpy(found->name, name);
    return true;
}

bool bpdel(BREAKPOINT* breakpoint_list, const char* name, uint addr, BP_TYPE type)
{
    BREAKPOINT* prev=0;
    BREAKPOINT* found=bpfind(breakpoint_list, name, addr, &prev, type);
    if(!found)
        return false;
    if(found->name)
        efree(found->name);
    if(found==breakpoint_list)
    {
        BREAKPOINT* next=breakpoint_list->next;
        if(next)
        {
            memcpy(breakpoint_list, breakpoint_list->next, sizeof(BREAKPOINT));
            breakpoint_list->next=next->next;
            efree(next);
        }
        else
            memset(breakpoint_list, 0, sizeof(BREAKPOINT));
    }
    else
    {
        prev->next=found->next;
        efree(found);
    }
    return true;
}
