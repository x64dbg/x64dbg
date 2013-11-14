#include "command.h"
#include "argument.h"
#include "console.h"
#include "debugger.h"
#include "math.h"

COMMAND* cmdfind(COMMAND* command_list, const char* name, COMMAND** link)
{
    COMMAND* cur=command_list;
    if(!cur->name)
        return 0;
    COMMAND* prev=0;
    while(cur)
    {
        if(arraycontains(cur->name, name))
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

COMMAND* cmdinit()
{
    COMMAND* cmd=(COMMAND*)emalloc(sizeof(COMMAND));
    memset(cmd, 0, sizeof(COMMAND));
    return cmd;
}

void cmdfree(COMMAND* cmd_list)
{
    COMMAND* cur=cmd_list;
    while(cur)
    {
        efree(cur->name);
        COMMAND* next=cur->next;
        efree(cur);
        cur=next;
    }
}

bool cmdnew(COMMAND* command_list, const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!command_list or !cbCommand or !name or !*name or cmdfind(command_list, name, 0))
        return false;
    COMMAND* cmd;
    bool nonext=false;
    if(!command_list->name)
    {
        cmd=command_list;
        nonext=true;
    }
    else
        cmd=(COMMAND*)emalloc(sizeof(COMMAND));
    memset(cmd, 0, sizeof(COMMAND));
    cmd->name=(char*)emalloc(strlen(name)+1);
    strcpy(cmd->name, name);
    cmd->cbCommand=cbCommand;
    cmd->debugonly=debugonly;
    COMMAND* cur=command_list;
    if(!nonext)
    {
        while(cur->next)
            cur=cur->next;
        cur->next=cmd;
    }
    return true;
}

COMMAND* cmdget(COMMAND* command_list, const char* cmd)
{
    char new_cmd[deflen]="";
    strcpy(new_cmd, cmd);
    int len=strlen(new_cmd);
    int start=0;
    while(new_cmd[start]!=' ' and start<len)
        start++;
    new_cmd[start]=0;
    COMMAND* found=cmdfind(command_list, new_cmd, 0);
    if(!found)
        return 0;
    return found;
}

CBCOMMAND cmdset(COMMAND* command_list, const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!cbCommand)
        return 0;
    COMMAND* found=cmdfind(command_list, name, 0);
    if(!found)
        return 0;
    CBCOMMAND old=found->cbCommand;
    found->cbCommand=cbCommand;
    found->debugonly=debugonly;
    return old;
}

bool cmddel(COMMAND* command_list, const char* name)
{
    COMMAND* prev=0;
    COMMAND* found=cmdfind(command_list, name, &prev);
    if(!found)
        return false;
    efree(found->name);
    if(found==command_list)
    {
        COMMAND* next=command_list->next;
        if(next)
        {
            memcpy(command_list, command_list->next, sizeof(COMMAND));
            command_list->next=next->next;
            efree(next);
        }
        else
            memset(command_list, 0, sizeof(COMMAND));
    }
    else
    {
        prev->next=found->next;
        efree(found);
    }
    return true;
}

/*
command_list:         command list
cbUnknownCommand:     function to execute when an unknown command was found
cbCommandProvider:    function that provides commands (fgets for example), does not return until a command was found
cbCommandFinder:      non-default command finder
error_is_fatal:       error return of a command callback stops the command processing
*/
CMDRESULT cmdloop(COMMAND* command_list, CBCOMMAND cbUnknownCommand, CBCOMMANDPROVIDER cbCommandProvider, CBCOMMANDFINDER cbCommandFinder, bool error_is_fatal)
{
    if(!cbUnknownCommand or !cbCommandProvider)
        return STATUS_ERROR;
    char* command=(char*)emalloc(deflen);
    memset(command, 0, deflen);
    bool bLoop=true;
    while(bLoop)
    {
        if(!cbCommandProvider(command, deflen))
            break;
        if(strlen(command))
        {
            argformat(command); //default formatting
            COMMAND* cmd;
            if(!cbCommandFinder) //'clean' command processing
                cmd=cmdget(command_list, command);
            else //'dirty' command processing
                cmd=cbCommandFinder(command_list, command);

            if(!cmd or !cmd->cbCommand) //unknown command
            {
                CMDRESULT res=cbUnknownCommand(command);
                if((error_is_fatal and res==STATUS_ERROR) or res==STATUS_EXIT)
                    bLoop=false;
            }
            else
            {
                if(cmd->debugonly and !IsFileBeingDebugged())
                {
                    dputs("this command is debug-only");
                    if(error_is_fatal)
                        bLoop=false;
                }
                else
                {
                    CMDRESULT res=cmd->cbCommand(command);
                    if((error_is_fatal and res==STATUS_ERROR) or res==STATUS_EXIT)
                        bLoop=false;
                }
            }
        }
    }
    efree(command);
    return STATUS_EXIT;
}

/*
- custom command formatting rules
*/
static void specialformat(char* string)
{
    int len=strlen(string);
    char* found=strstr(string, "=");
    char* str=(char*)emalloc(len*2);
    memset(str, 0, len*2);
    if(found) //contains =
    {
        char* a=(found-1);
        *found=0;
        found++;
        if(!*found)
        {
            *found='=';
            efree(str);
            return;
        }
        int flen=strlen(found); //n(+)=n++
        if((found[flen-1]=='+' and found[flen-2]=='+') or (found[flen-1]=='-' and found[flen-2]=='-')) //eax++/eax--
        {
            found[flen-2]=0;
            char op=found[flen-1];
            sprintf(str, "%s%c1", found, op);
            strcpy(found, str);
        }
        if(mathisoperator(*a)>2) //x*=3 -> x=x*3
        {
            char op=*a;
            *a=0;
            sprintf(str, "mov %s,%s%c%s", string, string, op, found);
        }
        else
            sprintf(str, "mov %s,%s", string, found);
        strcpy(string, str);
    }
    else if((string[len-1]=='+' and string[len-2]=='+') or (string[len-1]=='-' and string[len-2]=='-')) //eax++/eax--
    {
        string[len-2]=0;
        char op=string[len-1];
        sprintf(str, "mov %s,%s%c1", string, string, op);
        strcpy(string, str);
    }
    efree(str);
}

/*
- 'default' command finder, with some custom rules
*/
COMMAND* cmdfindmain(COMMAND* cmd_list, char* command)
{
    COMMAND* cmd=cmdfind(cmd_list, command, 0);
    if(!cmd)
    {
        specialformat(command);
        cmd=cmdget(cmd_list, command);
    }
    if(!cmd or !cmd->cbCommand)
        mathformat(command);
    return cmd;
}
