#ifndef _COMMAND_H
#define _COMMAND_H

#include "_global.h"
#include "console.h"

bool IsArgumentsLessThan(int argc, int minimumCount);

//typedefs

struct COMMAND;

enum CMDRESULT
{
    STATUS_ERROR = false,
    STATUS_CONTINUE = true,
    STATUS_EXIT = 2,
    STATUS_PAUSE = 3
};

typedef CMDRESULT(*CBCOMMAND)(int, char**);
typedef bool (*CBCOMMANDPROVIDER)(char*, int);
typedef COMMAND* (*CBCOMMANDFINDER)(char*);

struct COMMAND
{
    std::vector<String>* names;
    CBCOMMAND cbCommand;
    bool debugonly;
    COMMAND* next;
};

//functions
COMMAND* cmdinit();
void cmdfree();
COMMAND* cmdfind(const char* name, COMMAND** link);
bool cmdnew(const char* name, CBCOMMAND cbCommand, bool debugonly);
COMMAND* cmdget(const char* cmd);
CBCOMMAND cmdset(const char* name, CBCOMMAND cbCommand, bool debugonly);
bool cmddel(const char* name);
CMDRESULT cmdloop(CBCOMMAND cbUnknownCommand, CBCOMMANDPROVIDER cbCommandProvider, CBCOMMANDFINDER cbCommandFinder, bool error_is_fatal);
CMDRESULT cmddirectexec(const char* cmd);

#endif // _COMMAND_H
