#include "reference.h"
#include "debugger.h"
#include "memory.h"
#include "console.h"

int reffind(uint addr, uint size, CBREF cbRef, void* userinfo, bool silent, const char* name)
{
    uint start_addr;
    uint start_size;
    uint base;
    uint base_size;
    base = memfindbaseaddr(addr, &base_size, true);
    if(!base or !base_size)
    {
        if(!silent)
            dputs("invalid memory page");
        return 0;
    }

    if(!size) //assume the whole page
    {
        start_addr = base;
        start_size = base_size;
    }
    else //custom boundaries
    {
        start_addr = addr;
        uint maxsize = size - (start_addr - base);
        if(size < maxsize) //check if the size fits in the page
            start_size = size;
        else
            start_size = maxsize;
    }
    Memory<unsigned char*> data(start_size, "reffind:data");
    if(!memread(fdProcessInfo->hProcess, (const void*)start_addr, data, start_size, 0))
    {
        if(!silent)
            dputs("error reading memory");
        return 0;
    }
    DISASM disasm;
    memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
    disasm.Archi = 64;
#endif // _WIN64
    disasm.EIP = (UIntPtr)data;
    disasm.VirtualAddr = (UInt64)start_addr;
    uint i = 0;
    BASIC_INSTRUCTION_INFO basicinfo;
    REFINFO refinfo;
    memset(&refinfo, 0, sizeof(REFINFO));
    refinfo.userinfo = userinfo;
    char fullName[deflen] = "";
    char modname[MAX_MODULE_SIZE] = "";
    if(modnamefromaddr(start_addr, modname, true))
        sprintf_s(fullName, "%s (%s)", name, modname);
    else
        sprintf_s(fullName, "%s (%p)", name, start_addr);
    refinfo.name = fullName;
    cbRef(0, 0, &refinfo); //allow initializing
    while(i < start_size)
    {
        if(!(i % 0x1000))
        {
            double percent = (double)i / (double)start_size;
            GuiReferenceSetProgress((int)(percent * 100));
        }
        int len = Disasm(&disasm);
        if(len != UNKNOWN_OPCODE)
        {
            fillbasicinfo(&disasm, &basicinfo);
            basicinfo.size = len;
            if(cbRef(&disasm, &basicinfo, &refinfo))
                refinfo.refcount++;
        }
        else
            len = 1;
        disasm.EIP += len;
        disasm.VirtualAddr += len;
        i += len;
    }
    GuiReferenceSetProgress(100);
    GuiReferenceReloadData();
    return refinfo.refcount;
}
