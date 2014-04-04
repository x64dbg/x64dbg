#include "reference.h"
#include "debugger.h"
#include "memory.h"
#include "console.h"

int reffind(uint addr, CBREF cbRef, void* userinfo, bool silent)
{
    uint size=0;
    uint base=memfindbaseaddr(fdProcessInfo->hProcess, addr, &size);
    if(!base or !size)
    {
        if(!silent)
            dputs("invalid memory page");
        return 0;
    }
    unsigned char* data=(unsigned char*)emalloc(size);
    if(!memread(fdProcessInfo->hProcess, (const void*)base, data, size, 0))
    {
        if(!silent)
            dputs("error reading memory");
        efree(data);
        return 0;
    }
    DISASM disasm;
    memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
    disasm.Archi=64;
#endif // _WIN64
    disasm.EIP=(UIntPtr)data;
    disasm.VirtualAddr=(UInt64)base;
    uint i=0;
    BASIC_INSTRUCTION_INFO basicinfo;
    cbRef(&disasm, &basicinfo, 0); //allow initializing
    REFINFO refinfo;
    memset(&refinfo, 0, sizeof(REFINFO));
    refinfo.userinfo=userinfo;
    while(i<size)
    {
        if(!(i%0x1000))
        {
            double percent=(double)i/(double)size;
            GuiReferenceSetProgress((int)(percent*100));
        }
        int len=Disasm(&disasm);
        if(len!=UNKNOWN_OPCODE)
        {
            fillbasicinfo(&disasm, &basicinfo);
            if(cbRef(&disasm, &basicinfo, &refinfo))
                refinfo.refcount++;
        }
        else
            len=1;
        disasm.EIP+=len;
        disasm.VirtualAddr+=len;
        i+=len;
    }
    GuiReferenceSetProgress(100);
    GuiReferenceReloadData();
    efree(data);
    return refinfo.refcount;
}