#include "commandline.h"
#include "threading.h"
#include "memory.h"
#include "debugger.h"
#include "console.h"

char commandLine[MAX_COMMAND_LINE_SIZE];

void showcommandlineerror(cmdline_error_t* cmdline_error)
{
    bool unkown = false;

    switch(cmdline_error->type)
    {
    case CMDL_ERR_ALLOC:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error allocating memory for cmdline"));
        break;
    case CMDL_ERR_CONVERTUNICODE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error converting UNICODE cmdline"));
        break;
    case CMDL_ERR_READ_PEBBASE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error reading PEB base addres"));
        break;
    case CMDL_ERR_READ_PROCPARM_CMDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error reading PEB -> ProcessParameters -> CommandLine UNICODE_STRING"));
        break;
    case CMDL_ERR_READ_PROCPARM_PTR:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error reading PEB -> ProcessParameters pointer address"));
        break;
    case CMDL_ERR_GET_PEB:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error Getting remote PEB address"));
        break;
    case CMDL_ERR_READ_GETCOMMANDLINEBASE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error Getting command line base address"));
        break;
    case CMDL_ERR_CHECK_GETCOMMANDLINESTORED:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error checking the pattern of the commandline stored"));
        break;
    case CMDL_ERR_WRITE_GETCOMMANDLINESTORED:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error writing the new command line stored"));
        break;
    case CMDL_ERR_GET_GETCOMMANDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error getting getcommandline"));
        break;
    case CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error allocating the page with UNICODE and ANSI command lines"));
        break;
    case CMDL_ERR_WRITE_ANSI_COMMANDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error writing the ANSI command line in the page"));
        break;
    case CMDL_ERR_WRITE_UNICODE_COMMANDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error writing the UNICODE command line in the page"));
        break;
    case CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE:
        dputs(QT_TRANSLATE_NOOP("DBG", "Error writing command line UNICODE in PEB"));
        break;
    default:
        unkown = true;
        dputs(QT_TRANSLATE_NOOP("DBG", "Error getting cmdline"));
        break;
    }

    if(!unkown)
    {
        if(cmdline_error->addr != 0)
            dprintf(QT_TRANSLATE_NOOP("DBG", " (Address: %p)"), cmdline_error->addr);
        dputs(QT_TRANSLATE_NOOP("DBG", ""));
    }
}

bool isCmdLineEmpty()
{
    return !strlen(commandLine);
}

char* getCommandLineArgs()
{
    char* commandLineArguments = NULL;
    char* extensionPtr = strchr(commandLine, '.');

    if(!extensionPtr)
        return NULL;

    commandLineArguments = strchr(extensionPtr, ' ');

    if(!commandLineArguments)
        return NULL;

    return (commandLineArguments + 1);

}

void CmdLineCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockCmdLine);

    // return if command line is empty
    if(!strlen(commandLine))
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
    if(!jsonCmdLine)
        return;

    const char* cmdLine = json_string_value(json_object_get(jsonCmdLine, "cmdLine"));

    strcpy_s(commandLine, cmdLine);

    json_decref(jsonCmdLine);
}

void copyCommandLine(const char* cmdLine)
{
    strcpy_s(commandLine, cmdLine);
}

bool SetCommandLine()
{
    cmdline_error_t cmdline_error = { (cmdline_error_type_t)0, 0 };

    if(!dbgsetcmdline(commandLine, &cmdline_error))
    {
        showcommandlineerror(&cmdline_error);
        return false;
    }

    //update the memory map
    MemUpdateMap();
    GuiUpdateMemoryView();

    return true;
}

