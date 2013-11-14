#include "simplescript.h"
#include "command.h"
#include "console.h"
#include "instruction.h"

static int total=0;
static char found[1024][1024];
static COMMAND* command_list=0;

static CMDRESULT cbRet(const char* cmd)
{
    return STATUS_EXIT;
}

static bool cbCommandProvider(char* cmd, int maxlen)
{
    fgets(cmd, maxlen, stdin);
    cmd[strlen(cmd)-1]=0;
    return true;
}

static CMDRESULT cbCollect(const char* cmd)
{
    strcpy(found[total], cmd);
    total++;
    if(total>=1024)
        return STATUS_EXIT;
    return STATUS_CONTINUE;
}

void scriptSetList(COMMAND* cmd_list)
{
    command_list=cmd_list;
}

static int i=0;

static bool provider(char* cmd, int size)
{
    strcpy(cmd, found[i]);
    i++;
    if(i>total)
        return false;
    return true;
}

CMDRESULT cbScript(const char* cmd)
{
    total=0;
    i=0;
    COMMAND* cmd_list=cmdinit();
    cmdnew(cmd_list, "ret", cbRet, false);
    cmdloop(cmd_list, cbCollect, cbCommandProvider, 0, false);
    cmdfree(cmd_list);
    cmdloop(command_list, cbBadCmd, provider, cmdfindmain, true);
    return STATUS_CONTINUE;
}
