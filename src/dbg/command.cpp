/**
 @file command.cpp

 @brief Implements the command class.
 */

#include "command.h"
#include "value.h"
#include "console.h"
#include "commandparser.h"

COMMAND* cmd_list = 0;

/**
\brief Finds a ::COMMAND in a command list.
\param [in] command list.
\param name The name of the command to find.
\param [out] Link to the command.
\return null if it fails, else a ::COMMAND*.
*/
COMMAND* cmdfind(const char* name, COMMAND** link)
{
    COMMAND* cur = cmd_list;
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

/**
\brief Initialize a command list.
\return a ::COMMAND*
*/
COMMAND* cmdinit()
{
    cmd_list = (COMMAND*)emalloc(sizeof(COMMAND), "cmdinit:cmd");
    memset(cmd_list, 0, sizeof(COMMAND));
    return cmd_list;
}

/**
\brief Clear a command list.
\param [in] cmd_list Command list to clear.
*/
void cmdfree()
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

/**
\brief Creates a new command and adds it to the list.
\param [in,out] command_list Command list. Cannot be null.
\param name The command name.
\param cbCommand The command callback.
\param debugonly true if the command can only be executed in a debugging context.
\return true if the command was successfully added to the list.
*/
bool cmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if (!cmd_list || !cbCommand || !name || !*name || cmdfind(name, 0))
        return false;
    COMMAND* cmd;
    bool nonext = false;
    if (!cmd_list->name)
    {
        cmd = cmd_list;
        nonext = true;
    }
    else
        cmd = (COMMAND*)emalloc(sizeof(COMMAND), "cmdnew:cmd");
    memset(cmd, 0, sizeof(COMMAND));
    cmd->name = (char*)emalloc(strlen(name) + 1, "cmdnew:cmd->name");
    strcpy(cmd->name, name);
    cmd->cbCommand = cbCommand;
    cmd->debugonly = debugonly;
    COMMAND* cur = cmd_list;
    if(!nonext)
    {
        while(cur->next)
            cur = cur->next;
        cur->next = cmd;
    }
    return true;
}

/**
\brief Gets a ::COMMAND from the command list.
\param [in] command_list Command list.
\param cmd The command to get from the list.
\return null if the command was not found. Otherwise a ::COMMAND*.
*/
COMMAND* cmdget(const char* cmd)
{
    char new_cmd[deflen] = "";
    strcpy_s(new_cmd, deflen, cmd);
    int len = (int)strlen(new_cmd);
    int start = 0;
    while(new_cmd[start] != ' ' && start < len)
        start++;
    new_cmd[start] = 0;
    COMMAND* found = cmdfind(new_cmd, 0);
    if(!found)
        return 0;
    return found;
}

/**
\brief Sets a new command callback and debugonly property in a command list.
\param [in] command_list Command list.
\param name The name of the command to change.
\param cbCommand The new command callback.
\param debugonly The new debugonly value.
\return The old command callback.
*/
CBCOMMAND cmdset(const char* name, CBCOMMAND cbCommand, bool debugonly)
{
    if(!cbCommand)
        return 0;
    COMMAND* found = cmdfind(name, 0);
    if(!found)
        return 0;
    CBCOMMAND old = found->cbCommand;
    found->cbCommand = cbCommand;
    found->debugonly = debugonly;
    return old;
}

/**
\brief Deletes a command from a command list.
\param [in] command_list Command list.
\param name The name of the command to delete.
\return true if the command was deleted.
*/
bool cmddel(const char* name)
{
    COMMAND* prev = 0;
    COMMAND* found = cmdfind(name, &prev);
    if(!found)
        return false;
    efree(found->name, "cmddel:found->name");
    if (found == cmd_list)
    {
        COMMAND* next = cmd_list->next;
        if(next)
        {
            memcpy(cmd_list, cmd_list->next, sizeof(COMMAND));
            cmd_list->next = next->next;
            efree(next, "cmddel:next");
        }
        else
            memset(cmd_list, 0, sizeof(COMMAND));
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

/**
\brief Initiates a command loop. This function will not return until a command returns ::STATUS_EXIT.
\param [in] command_list Command list to use for the command lookups.
\param cbUnknownCommand The unknown command callback.
\param cbCommandProvider The command provider callback.
\param cbCommandFinder The command finder callback.
\param error_is_fatal true if commands that return ::STATUS_ERROR terminate the command loop.
\return A CMDRESULT, will always be ::STATUS_EXIT.
*/
CMDRESULT cmdloop(CBCOMMAND cbUnknownCommand, CBCOMMANDPROVIDER cbCommandProvider, CBCOMMANDFINDER cbCommandFinder, bool error_is_fatal)
{
    if(!cbUnknownCommand || !cbCommandProvider)
        return STATUS_ERROR;
    char command[deflen] = "";
    bool bLoop = true;
    while(bLoop)
    {
        if(!cbCommandProvider(command, deflen))
            break;
        if(strlen(command))
        {
            strcpy_s(command, StringUtils::Trim(command).c_str());
            COMMAND* cmd;
            if(!cbCommandFinder) //'clean' command processing
                cmd = cmdget(command);
            else //'dirty' command processing
                cmd = cbCommandFinder(command);

            if(!cmd || !cmd->cbCommand) //unknown command
            {
                char* argv[1];
                *argv = command;
                CMDRESULT res = cbUnknownCommand(1, argv);
                if((error_is_fatal && res == STATUS_ERROR) || res == STATUS_EXIT)
                    bLoop = false;
            }
            else
            {
                if(cmd->debugonly && !DbgIsDebugging())
                {
                    dputs("this command is debug-only");
                    if(error_is_fatal)
                        bLoop = false;
                }
                else
                {
                    Command commandParsed(command);
                    int argcount = commandParsed.GetArgCount();
                    char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmdloop:argv");
                    argv[0] = command;
                    for(int i = 0; i < argcount; i++)
                    {
                        argv[i + 1] = (char*)emalloc(deflen, "cmdloop:argv[i+1]");
                        *argv[i + 1] = 0;
                        strcpy_s(argv[i + 1], deflen, commandParsed.GetArg(i).c_str());
                    }
                    CMDRESULT res = cmd->cbCommand(argcount + 1, argv);
                    for(int i = 0; i < argcount; i++)
                        efree(argv[i + 1], "cmdloop:argv[i+1]");
                    efree(argv, "cmdloop:argv");
                    if((error_is_fatal && res == STATUS_ERROR) || res == STATUS_EXIT)
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

/**
\brief Query if a string is a valid expression.
\param expression The expression to check.
\return true if the string is a valid expression.
*/
static bool isvalidexpression(const char* expression)
{
    duint value;
    return valfromstring(expression, &value);
}

/**
\brief Check if a character is a mathematical operator. Used to determine stuff like "a *= b"
\param ch The character to check.
\return true if the character is an operator, false otherwise.
*/
static bool mathisoperator(const char ch)
{
    switch(ch)
    {
    case '*':
    case '`':
    case '/':
    case '%':
    case '+':
    case '-':
    case '<':
    case '>':
    case '&':
    case '^':
    case '|':
        return true;
    default:
        return false;
    }
}

/**
\brief Special formats a given command. Used as a little hack to support stuff like 'x++' and 'x=y'
\param [in,out] string String to format.
*/
static void specialformat(char* string)
{
    int len = (int)strlen(string);
    char* found = strstr(string, "=");
    char str[deflen] = "";
    char backup[deflen] = "";
    strcpy_s(backup, string); //create a backup of the string
    if(found) //contains =
    {
        char* a = (found - 1);
        *found = 0;
        found++;
        if(!*found)
        {
            *found = '=';
            return;
        }

        if(mathisoperator(*a)) //x*=3 -> x=x*3
        {
            char op = *a;
            *a = 0;
            if(isvalidexpression(string))
                sprintf_s(str, "mov %s,%s%c%s", string, string, op, found);
            else
                strcpy_s(str, backup);
        }
        else //x=y
        {
            if(isvalidexpression(found))
                sprintf_s(str, "mov %s,%s", string, found);
            else
                strcpy_s(str, backup);
        }
        strcpy_s(string, deflen, str);
    }
    else if((string[len - 1] == '+' && string[len - 2] == '+') || (string[len - 1] == '-' && string[len - 2] == '-')) //eax++/eax--
    {
        string[len - 2] = 0;
        char op = string[len - 1];
        if(isvalidexpression(string))
            sprintf_s(str, "mov %s,%s%c1", string, string, op);
        else
            strcpy_s(str, backup);
        strcpy_s(string, deflen, str);
    }
}

/*
- 'default' command finder, with some custom rules
*/

/**
\brief Default command finder. It uses specialformat() and mathformat() to make sure the command is optimally checked.
\param [in] cmd_list Command list.
\param [in] command Command name.
\return null if it fails, else a COMMAND*.
*/
COMMAND* cmdfindmain(char* command)
{
    COMMAND* cmd = cmdfind(command, 0);
    if(!cmd)
    {
        specialformat(command);
        cmd = cmdget(command);
    }
    return cmd;
}

/**
\brief Directly execute a command.
\param [in,out] cmd_list Command list.
\param cmd The command to execute.
\return A CMDRESULT.
*/
CMDRESULT cmddirectexec(const char* cmd, ...)
{
    // Fail on null strings
    ASSERT_NONNULL(cmd);

    // Don't allow anyone to send in empty strings
    if(strlen(cmd) <= 0)
        return STATUS_ERROR;

    char command[deflen];
    va_list ap;
    va_start(ap, cmd);
    _vsnprintf_s(command, _TRUNCATE, cmd, ap);
    va_end(ap);

    strcpy_s(command, StringUtils::Trim(cmd).c_str());
    COMMAND* found = cmdfindmain(command);
    if(!found || !found->cbCommand)
        return STATUS_ERROR;
    if(found->debugonly && !DbgIsDebugging())
        return STATUS_ERROR;
    Command cmdParsed(command);
    int argcount = cmdParsed.GetArgCount();
    char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmddirectexec:argv");
    argv[0] = command;
    for(int i = 0; i < argcount; i++)
    {
        argv[i + 1] = (char*)emalloc(deflen, "cmddirectexec:argv[i+1]");
        *argv[i + 1] = 0;
        strcpy_s(argv[i + 1], deflen, cmdParsed.GetArg(i).c_str());
    }
    CMDRESULT res = found->cbCommand(argcount + 1, argv);
    for(int i = 0; i < argcount; i++)
        efree(argv[i + 1], "cmddirectexec:argv[i+1]");
    efree(argv, "cmddirectexec:argv");
    return res;
}
