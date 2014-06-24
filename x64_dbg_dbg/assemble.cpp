#include "assemble.h"
#include "memory.h"
#include "debugger.h"
#include "XEDParse\XEDParse.h"
#include "value.h"
#include "disasm_helper.h"
#include "console.h"

static bool cbUnknown(const char* text, ULONGLONG* value)
{
    if(!text or !value)
        return false;
    uint val;
    if(!valfromstring(text, &val))
        return false;
    *value=val;
    return true;
}

bool assembleat(uint addr, const char* instruction, char* error, bool fillnop)
{
    if(strlen(instruction)>=XEDPARSE_MAXBUFSIZE)
        return false;
    XEDPARSE parse;
    memset(&parse, 0, sizeof(parse));
#ifdef _WIN64
    parse.x64=true;
#else //x86
    parse.x64=false;
#endif
    parse.cbUnknown=cbUnknown;
    parse.cip=addr;
    strcpy(parse.instr, instruction);
    if(XEDParseAssemble(&parse)==XEDPARSE_ERROR)
    {
        if(error)
            strcpy(error, parse.error);
        return false;
    }

    //calculate the number of NOPs to insert
    int destSize=parse.dest_size;
    int origLen=disasmgetsize(addr);
    while(origLen<destSize)
        origLen+=disasmgetsize(addr+origLen);
    int nopsize=origLen-destSize;
    unsigned char nops[16];
    memset(nops, 0x90, sizeof(nops));

    bool ret=memwrite(fdProcessInfo->hProcess, (void*)addr, parse.dest, destSize, 0);
    if(ret && fillnop && nopsize)
        if(!memwrite(fdProcessInfo->hProcess, (void*)(addr+destSize), nops, nopsize, 0))
            ret=false;
    return true;
}
