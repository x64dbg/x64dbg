#include "patches.h"
#include "addrinfo.h"
#include "memory.h"
#include "debugger.h"

PatchesInfo patches;

bool patchset(uint addr, unsigned char oldbyte, unsigned char newbyte)
{
    if(!DbgIsDebugging() || !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    if(oldbyte==newbyte)
        return true; //no need to make a patch for a byte that is equal to itself
    PATCHINFO newPatch;
    newPatch.addr=addr-modbasefromaddr(addr);
    modnamefromaddr(addr, newPatch.mod, true);
    newPatch.oldbyte=oldbyte;
    newPatch.newbyte=newbyte;
    uint key=modhashfromva(addr);
    PatchesInfo::iterator found=patches.find(key);
    if(found!=patches.end()) //we found a patch on the specified address
    {
        if(found->second.oldbyte == newbyte) //patch is undone
        {
            patches.erase(found);
            return true;
        }
        else
        {
            newPatch.oldbyte=found->second.oldbyte; //keep the original byte from the previous patch
            found->second=newPatch;
        }
    }
    else
        patches.insert(std::make_pair(key, newPatch));
    return true;
}

bool patchget(uint addr, PATCHINFO* patch)
{
    if(!DbgIsDebugging())
        return false;
    PatchesInfo::iterator found=patches.find(modhashfromva(addr));
    if(found==patches.end()) //not found
        return false;
    if(patch)
    {
        *patch=found->second;
        patch->addr+=modbasefromaddr(addr);
        return true;
    }
    return (found->second.oldbyte != found->second.newbyte);
}

bool patchdel(uint addr, bool restore)
{
    if(!DbgIsDebugging())
        return false;
    PatchesInfo::iterator found=patches.find(modhashfromva(addr));
    if(found==patches.end()) //not found
        return false;
    if(restore)
        memwrite(fdProcessInfo->hProcess, (void*)(found->second.addr+modbasefromaddr(addr)), &found->second.oldbyte, sizeof(char), 0);
    patches.erase(found);
    return true;
}

void patchdelrange(uint start, uint end, bool restore)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll=(start==0 && end==~0); //0x00000000-0xFFFFFFFF
    uint modbase=modbasefromaddr(start);
    if(modbase!=modbasefromaddr(end))
        return;
    start-=modbase;
    end-=modbase;
    PatchesInfo::iterator i=patches.begin();
    while(i!=patches.end())
    {
        if(bDelAll || (i->second.addr>=start && i->second.addr<end))
        {
            if(restore)
                memwrite(fdProcessInfo->hProcess, (void*)(i->second.addr+modbase), &i->second.oldbyte, sizeof(char), 0);
            patches.erase(i++);
        }
        else
            i++;
    }
}

void patchclear(const char* mod)
{
    if(!mod or !*mod)
        patches.clear();
    else
    {
        PatchesInfo::iterator i=patches.begin();
        while(i!=patches.end())
        {
            if(!_stricmp(i->second.mod, mod))
                patches.erase(i++);
            else
                i++;
        }
    }
}

bool patchenum(PATCHINFO* patcheslist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!patcheslist && !cbsize)
        return false;
    if(!patcheslist && cbsize)
    {
        *cbsize=patches.size()*sizeof(LOOPSINFO);
        return true;
    }
    int j=0;
    for(PatchesInfo::iterator i=patches.begin(); i!=patches.end(); ++i,j++)
    {
        patcheslist[j]=i->second;
        uint modbase=modbasefromname(patcheslist[j].mod);
        patcheslist[j].addr+=modbase;
    }
    return true;
}