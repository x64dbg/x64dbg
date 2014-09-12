#include "command.h"
#include "argument.h"
#include "value.h"
#include "console.h"
#include "debugger.h"
#include "math.h"

COMMAND* cmdfind(COMMAND* command_list, const char* name, COMMAND** link)
{
    COMMAND* cur = command_list;
    if(!cur->name)
        return 0;
    COMMAND* prev = 0;
    while(cur)
    {
        if(arraycontains(cur->name, name))
        {
            if(link)
                *link = prev;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

COMMAND* cmdinit()
{
    COMMAND* cmd = (COMMAND*)emalloc(sizeof(COMMAND), "cmdinit:cmd");
    memset(cmd, 0, sizeof(COMMAND));
    return cmd;
}

void cmdfree(COMMAND* cmd_list)
{
    COMMAND* cur = cmd_list;
    while(cur)
    {
        efree(cur->name, "cmdfree:cur->name");
        COMMAND* next = cur->next;
        efree(cur, "cmdfree:cur");
        cur = next;
    }
}

bool cmdnew(COMMAND* command_list, const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!command_list or !cbCommand or !name or !*name or cmdfind(command_list, name, 0))
        return false;
    COMMAND* cmd;
    bool nonext = false;
    if(!command_list->name)
    {
        cmd = command_list;
        nonext = true;
    }
    else
        cmd = (COMMAND*)emalloc(sizeof(COMMAND), "cmdnew:cmd");
    memset(cmd, 0, sizeof(COMMAND));
    cmd->name = (char*)emalloc(strlen(name) + 1, "cmdnew:cmd->name");
    strcpy(cmd->name, name);
    cmd->cbCommand = cbCommand;
    cmd->debugonly = debugonly;
    COMMAND* cur = command_list;
    if(!nonext)
    {
        while(cur->next)
            cur = cur->next;
        cur->next = cmd;
    }
    return true;
}

COMMAND* cmdget(COMMAND* command_list, const char* cmd)
{
    char new_cmd[deflen] = "";
    strcpy_s(new_cmd, cmd);
    int len = (int)strlen(new_cmd);
    int start = 0;
    while(new_cmd[start] != ' ' and start < len)
        start++;
    new_cmd[start] = 0;
    COMMAND* found = cmdfind(command_list, new_cmd, 0);
    if(!found)
        return 0;
    return found;
}

CBCOMMAND cmdset(COMMAND* command_list, const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!cbCommand)
        return 0;
    COMMAND* found = cmdfind(command_list, name, 0);
    if(!found)
        return 0;
    CBCOMMAND old = found->cbCommand;
    found->cbCommand = cbCommand;
    found->debugonly = debugonly;
    return old;
}

bool cmddel(COMMAND* command_list, const char* name)
{
    COMMAND* prev = 0;
    COMMAND* found = cmdfind(command_list, name, &prev);
    if(!found)
        return false;
    efree(found->name, "cmddel:found->name");
    if(found == command_list)
    {
        COMMAND* next = command_list->next;
        if(next)
        {
            memcpy(command_list, command_list->next, sizeof(COMMAND));
            command_list->next = next->next;
            efree(next, "cmddel:next");
        }
        else
            memset(command_list, 0, sizeof(COMMAND));
    }
    else
    {
        prev->next = found->next;
        efree(found, "cmddel:found");
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
    char command[deflen] = "";
    bool bLoop = true;
    while(bLoop)
    {
        if(!cbCommandProvider(command, deflen))
            break;
        if(strlen(command))
        {
            argformat(command); //default formatting
            COMMAND* cmd;
            if(!cbCommandFinder) //'clean' command processing
                cmd = cmdget(command_list, command);
            else //'dirty' command processing
                cmd = cbCommandFinder(command_list, command);

            if(!cmd or !cmd->cbCommand) //unknown command
            {
                char* argv[1];
                *argv = command;
                CMDRESULT res = cbUnknownCommand(1, argv);
                if((error_is_fatal and res == STATUS_ERROR) or res == STATUS_EXIT)
                    bLoop = false;
            }
            else
            {
                if(cmd->debugonly and !DbgIsDebugging())
                {
                    dputs("this command is debug-only");
                    if(error_is_fatal)
                        bLoop = false;
                }
                else
                {
                    int argcount = arggetcount(command);
                    char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmdloop:argv");
                    argv[0] = command;
                    for(int i = 0; i < argcount; i++)
                    {
                        argv[i + 1] = (char*)emalloc(deflen, "cmdloop:argv[i+1]");
                        *argv[i + 1] = 0;
                        argget(command, argv[i + 1], i, true);
                    }
                    CMDRESULT res = cmd->cbCommand(argcount + 1, argv);
                    for(int i = 0; i < argcount; i++)
                        efree(argv[i + 1], "cmdloop:argv[i+1]");
                    efree(argv, "cmdloop:argv");
                    if((error_is_fatal and res == STATUS_ERROR) or res == STATUS_EXIT)
                        bLoop = false;
                }
            }
        }
    }
    return STATUS_EXIT;
}

/*
- custom command formatting rules
*/

static bool isvalidexpression(const char* expression)
{
    uint value;
    return valfromstring(expression, &value);
}

static void specialformat(char* string)
{
    int len = (int)strlen(string);
    char* found = strstr(string, "=");
    char* str = (char*)emalloc(len * 2, "specialformat:str");
    char* backup = (char*)emalloc(len + 1, "specialformat:backup");
    strcpy(backup, string); //create a backup of the string
    memset(str, 0, len * 2);
    if(found) //contains =
    {
        char* a = (found - 1);
        *found = 0;
        found++;
        if(!*found)
        {
            *found = '=';
            efree(str, "specialformat:str");
            efree(backup, "specialformat:backup");
            return;
        }
        int flen = (int)strlen(found); //n(+)=n++
        if((found[flen - 1] == '+' and found[flen - 2] == '+') or (found[flen - 1] == '-' and found[flen - 2] == '-')) //eax++/eax--
        {
            found[flen - 2] = 0;
            char op = found[flen - 1];
            sprintf(str, "%s%c1", found, op);
            strcpy(found, str);
        }
        if(mathisoperator(*a) > 2) //x*=3 -> x=x*3
        {
            char op = *a;
            *a = 0;
            if(isvalidexpression(string))
                sprintf(str, "mov %s,%s%c%s", string, string, op, found);
            else
                strcpy(str, backup);
        }
        else
        {
            if(isvalidexpression(found))
                sprintf(str, "mov %s,%s", string, found);
            else
                strcpy(str, backup);
        }
        strcpy(string, str);
    }
    else if((string[len - 1] == '+' and string[len - 2] == '+') or (string[len - 1] == '-' and string[len - 2] == '-')) //eax++/eax--
    {
        string[len - 2] = 0;
        char op = string[len - 1];
        if(isvalidexpression(string))
            sprintf(str, "mov %s,%s%c1", string, string, op);
        else
            strcpy(str, backup);
        strcpy(string, str);
    }
    efree(str, "specialformat:str");
    efree(backup, "specialformat:backup");
}

/*
- 'default' command finder, with some custom rules
*/
COMMAND* cmdfindmain(COMMAND* cmd_list, char* command)
{
    COMMAND* cmd = cmdfind(cmd_list, command, 0);
    if(!cmd)
    {
        specialformat(command);
        cmd = cmdget(cmd_list, command);
    }
    if(!cmd or !cmd->cbCommand)
        mathformat(command);
    return cmd;
}

CMDRESULT cmddirectexec(COMMAND* cmd_list, const char* cmd)
{
    if(!cmd or !strlen(cmd))
        return STATUS_ERROR;
    char command[deflen] = "";
    strcpy(command, cmd);
    argformat(command);
    COMMAND* found = cmdfindmain(cmd_list, command);
    if(!found or !found->cbCommand)
        return STATUS_ERROR;
    if(found->debugonly and !DbgIsDebugging())
        return STATUS_ERROR;
    int argcount = arggetcount(command);
    char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmddirectexec:argv");
    argv[0] = command;
    for(int i = 0; i < argcount; i++)
    {
        argv[i + 1] = (char*)emalloc(deflen, "cmddirectexec:argv[i+1]");
        *argv[i + 1] = 0;
        argget(command, argv[i + 1], i, true);
    }
    CMDRESULT res = found->cbCommand(argcount + 1, argv);
    for(int i = 0; i < argcount; i++)
        efree(argv[i + 1], "cmddirectexec:argv[i+1]");
    efree(argv, "cmddirectexec:argv");
    return res;
}
