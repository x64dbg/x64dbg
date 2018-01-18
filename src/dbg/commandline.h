#ifndef _COMMANDLINE_H
#define _COMMANDLINE_H

#include "_global.h"
#include "command.h"
#include "jansson/jansson_x64dbg.h"

typedef enum
{
    CMDL_ERR_READ_PEBBASE = 0,
    CMDL_ERR_READ_PROCPARM_PTR,
    CMDL_ERR_READ_PROCPARM_CMDLINE,
    CMDL_ERR_CONVERTUNICODE,
    CMDL_ERR_ALLOC,
    CMDL_ERR_GET_PEB,
    CMDL_ERR_READ_GETCOMMANDLINEBASE,
    CMDL_ERR_CHECK_GETCOMMANDLINESTORED,
    CMDL_ERR_WRITE_GETCOMMANDLINESTORED,
    CMDL_ERR_GET_GETCOMMANDLINE,
    CMDL_ERR_ALLOC_UNICODEANSI_COMMANDLINE,
    CMDL_ERR_WRITE_ANSI_COMMANDLINE,
    CMDL_ERR_WRITE_UNICODE_COMMANDLINE,
    CMDL_ERR_WRITE_PEBUNICODE_COMMANDLINE

} cmdline_error_type_t;

typedef enum
{
    NO_QOUTES = 0,
    QOUTES_AROUND_EXE,
    QOUTES_AT_BEGIN_AND_END,
    NO_CLOSE_QUOTE_FOUND

} cmdline_qoutes_placement_t_enum;

typedef struct
{
    cmdline_qoutes_placement_t_enum posEnum;
    size_t firstPos;
    size_t secondPos;
} cmdline_qoutes_placement_t;

typedef struct
{
    cmdline_error_type_t type;
    duint addr;
} cmdline_error_t;

void showcommandlineerror(cmdline_error_t* cmdline_error);
bool isCmdLineEmpty();
char* getCommandLineArgs();
void CmdLineCacheSave(JSON Root, const String & cacheFile);
void CmdLineCacheLoad(JSON Root);
void copyCommandLine(const char* cmdLine);
bool setCommandLine();

#endif // _COMMANDLINE_H