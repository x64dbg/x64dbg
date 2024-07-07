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
        dprintf_html(QT_TRANSLATE_NOOP("DBG", "Not enough arguments! At least %d argument(s) must be specified. Arguments are <a href=\"https://help.x64dbg.com/en/latest/commands/index.html\">comma-separated</a>."), minimumCount - 1);
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
    auto split = StringUtils::Split(name, ',');
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
    while(start < len && new_cmd[start] != ' ')
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

void cmdsplit(const char* cmd, StringList & commands)
{
    commands.clear();
    auto len = strlen(cmd);
    auto inquote = false;
    auto inescape = false;
    std::string split;
    split.reserve(len);
    for(size_t i = 0; i < len; i++)
    {
        auto ch = cmd[i];
        switch(ch) //simple state machine to determine if the ';' separator is in quotes
        {
        case '\"':
            if(!inescape)
                inquote = !inquote;
            inescape = false;
            break;
        case '\\':
            inescape = !inescape;
            break;
        default:
            inescape = false;
        }
        if(ch == ';' && !inquote)
        {
            if(!split.empty())
                commands.push_back(split);
            split.clear();
        }
        else
            split.push_back(ch);
    }
    if(!split.empty())
        commands.push_back(split);
}

bool cmdexeccallback(COMMAND* cmd, const std::string & command)
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
    auto res = cmd->cbCommand(argcount + 1, argv);
    for(int i = 0; i < argcount; i++)
        efree(argv[i + 1], "cmdloop:argv[i+1]");
    efree(argv, "cmdloop:argv");
    return res;
}

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
        cmdsplit(command_, commands);
        for(auto & command : commands)
        {
            command = StringUtils::Trim(command);
            if(command.empty()) //skip empty commands
                continue;

            COMMAND* found = cmdget(command.c_str());
            if(!found || !found->cbCommand) //unknown command
            {
                char* argv[1];
                *argv = (char*)command.c_str();
                if(!cbBadCmd(1, argv)) //stop processing on non-value commands
                    break;
                continue;
            }

            if(found->debugonly && !DbgIsDebugging()) //stop processing on debug-only commands
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "The command \"%s\" is debug-only\n"), command.c_str());
                break;
            }

            //execute command callback
            if(!cmdexeccallback(found, command))
                break;
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
    cmdsplit(cmd, commands);
    for(auto & command : commands)
    {
        command = StringUtils::Trim(command);
        if(command.empty()) //skip empty commands
            continue;

        COMMAND* found = cmdget(command.c_str());
        if(!found || !found->cbCommand) //unknown command
        {
            ExpressionParser parser(command);
            duint result;
            if(!parser.Calculate(result, valuesignedcalc(), true, false)) //stop processing on non-value commands
                return false;
            varset("$ans", result, true);
            continue;
        }

        if(found->debugonly && !DbgIsDebugging()) //stop processing on debug-only commands
        {
            dprintf(QT_TRANSLATE_NOOP("DBG", "The command \"%s\" is debug-only\n"), command.c_str());
            return false;
        }

        //execute command callback
        if(!cmdexeccallback(found, command))
            return false;
    }
    return true;
}
