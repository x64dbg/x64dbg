#include "assemble.h"
#include "memory.h"
#include "debugger.h"
#include "XEDParse\XEDParse.h"

bool assembleat(uint addr, const char* instruction)
{
    if(strlen(instruction)>=XEDPARSE_MAXBUFSIZE)
        return false;
    XEDPARSE parse;
    memset(&parse, 0, sizeof(parse));
    strcpy(parse.instr, instruction);
    if(XEDParseAssemble(&parse)==XEDPARSE_ERROR)
        return false;
    bool ret=memwrite(fdProcessInfo->hProcess, (void*)addr, parse.dest, parse.dest_size, 0);
    return ret;
}
