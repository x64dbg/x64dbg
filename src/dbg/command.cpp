/**
 @file command.cpp

 @brief Implements the command class.
 */

#include "command.h"
#include "value.h"
#include "console.h"
#include "commandparser.h"
#include "expressionparser.h"
#include "variable.h"
#include "cmd-undocumented.h"

COMMAND* cmd_list = 0;

static bool vecContains(std::vector<String>* names, const char* name)
{
    for(const auto & cmd : *names)
        if(!_stricmp(cmd.c_str(), name))
            return true;
    return false;
}

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
    if(!cur->names)
        return 0;
    COMMAND* prev = 0;
    while(cur)
    {
        if(vecContains(cur->names, name))
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

bool IsArgumentsLessThan(int argc, int minimumCount)
{
    if(argc < minimumCount)
    {
        dprintf(QT_TRANSLATE_NOOP("DBG", "Not enough arguments! At least %d arguments must be specified.\n"), minimumCount - 1);
        return true;
    }
    return false;
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
        delete cur->names;
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
    if(!cmd_list || !cbCommand || !name || !*name || cmdfind(name, 0))
        return false;
    COMMAND* cmd;
    bool nonext = false;
    if(!cmd_list->names)
    {
        cmd = cmd_list;
        nonext = true;
    }
    else
        cmd = (COMMAND*)emalloc(sizeof(COMMAND), "cmdnew:cmd");
    memset(cmd, 0, sizeof(COMMAND));
    cmd->names = new std::vector<String>;
    auto split = StringUtils::Split(name, '\1');
    for(const auto & s : split)
    {
        auto trimmed = StringUtils::Trim(s);
        if(trimmed.length())
            cmd->names->push_back(trimmed);
    }
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
    delete found->names;
    if(found == cmd_list)
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

bool cbCommandProvider(char* cmd, int maxlen);

/**
\brief Initiates the command loop. This function will not return until a command returns ::STATUS_EXIT.
\return A bool, will always be ::STATUS_EXIT.
*/
void cmdloop()
{
    char command_[deflen] = "";
    StringList commands;
    commands.reserve(100);
    while(true)
    {
        if(!cbCommandProvider(command_, deflen))
            break;
        StringUtils::Split(command_, ';', commands);
        for(auto & command : commands)
        {
            if(!command.empty())
            {
                command = StringUtils::Trim(command);
                COMMAND* cmd = cmdget(command.c_str());

                if(!cmd || !cmd->cbCommand)  //unknown command
                {
                    char* argv[1];
                    *argv = (char*)command.c_str();
                    cbBadCmd(1, argv);
                }
                else
                {
                    if(cmd->debugonly && !DbgIsDebugging())
                    {
                        dputs(QT_TRANSLATE_NOOP("DBG", "this command is debug-only"));
                    }
                    else
                    {
                        Command commandParsed(command);
                        int argcount = commandParsed.GetArgCount();
                        char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmdloop:argv");
                        argv[0] = (char*)command.c_str();
                        for(int i = 0; i < argcount; i++)
                        {
                            argv[i + 1] = (char*)emalloc(deflen, "cmdloop:argv[i+1]");
                            *argv[i + 1] = 0;
                            strcpy_s(argv[i + 1], deflen, commandParsed.GetArg(i).c_str());
                        }
                        cmd->cbCommand(argcount + 1, argv);
                        for(int i = 0; i < argcount; i++)
                            efree(argv[i + 1], "cmdloop:argv[i+1]");
                        efree(argv, "cmdloop:argv");
                    }
                }
            }
        }
    }
}

/**
\brief Directly execute a command.
\param [in,out] cmd_list Command list.
\param cmd The command to execute.
\return A bool.
*/
bool cmddirectexec(const char* cmd)
{
    // Don't allow anyone to send in empty strings
    if(!cmd)
        return false;

    StringList commands;
    StringUtils::Split(cmd, ';', commands);
    for(auto & command : commands)
    {
        command = StringUtils::Trim(command);
        COMMAND* found = cmdget(command.c_str());
        if(!found || !found->cbCommand)
        {
            ExpressionParser parser(command);
            duint result;
            if(!parser.Calculate(result, valuesignedcalc(), true, false))
                return false;
            varset("$ans", result, true);
            continue;
        }
        if(found->debugonly && !DbgIsDebugging())
            return false;
        Command cmdParsed(command);
        int argcount = cmdParsed.GetArgCount();
        char** argv = (char**)emalloc((argcount + 1) * sizeof(char*), "cmddirectexec:argv");
        argv[0] = (char*)command.c_str();
        for(int i = 0; i < argcount; i++)
        {
            argv[i + 1] = (char*)emalloc(deflen, "cmddirectexec:argv[i+1]");
            *argv[i + 1] = 0;
            strcpy_s(argv[i + 1], deflen, cmdParsed.GetArg(i).c_str());
        }
        auto res = found->cbCommand(argcount + 1, argv);
        for(int i = 0; i < argcount; i++)
            efree(argv[i + 1], "cmddirectexec:argv[i+1]");
        efree(argv, "cmddirectexec:argv");
        if(!res)
            return false;
    }
    return true;
}
