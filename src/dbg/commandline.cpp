#include "commandline.h"
#include "threading.h"
#include "memory.h"
#include "debugger.h"
#include "debugger_commands.h"

char commandLine[MAX_COMMAND_LINE_SIZE];

bool isCmdLineEmpty()
{
    return !strlen(commandLine);
}

char* getCommandLineArgs()
{
    char* commandLineArguments = NULL;
    char* extensionPtr = strchr(commandLine, '.');

    if (!extensionPtr)
        return NULL;

    commandLineArguments = strchr(extensionPtr, ' ');

    if (!commandLineArguments)
        return NULL;

    return (commandLineArguments + 1);

}

void CmdLineCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockCmdLine);

    // return if command line is empty
    if (!strlen(commandLine))
        return;

    // Create a JSON array to store each sub-object with a breakpoint
    const JSON jsonCmdLine = json_object();
    json_object_set_new(jsonCmdLine, "cmdLine", json_string(commandLine));
    json_object_set(Root, "commandLine", jsonCmdLine);

    // Notify garbage collector
    json_decref(jsonCmdLine);
}

void CmdLineCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockCmdLine);

    // Clear command line
    memset(commandLine, 0, MAX_COMMAND_LINE_SIZE);

    // Get a handle to the root object -> commandLine
    const JSON jsonCmdLine = json_object_get(Root, "commandLine");

    // Return if there was nothing to load
    if (!jsonCmdLine)
        return;

    const char *cmdLine = json_string_value(json_object_get(jsonCmdLine, "cmdLine"));

    strcpy_s(commandLine, cmdLine);
}

void copyCommandLine(const char* cmdLine)
{
    strcpy_s(commandLine, cmdLine);
}

CMDRESULT SetCommandLine()
{
    cmdline_error_t cmdline_error = { (cmdline_error_type_t)0, 0 };

    if (!dbgsetcmdline(commandLine, &cmdline_error))
    {
        showcommandlineerror(&cmdline_error);
        return STATUS_ERROR;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    return STATUS_CONTINUE;
}

