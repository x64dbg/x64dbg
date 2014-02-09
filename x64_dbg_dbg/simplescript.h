#ifndef _SIMPLESCRIPT_H
#define _SIMPLESCRIPT_H

#include "command.h"

CMDRESULT cbScriptAddLine(int argc, char* argv[]);
CMDRESULT cbScriptClear(int argc, char* argv[]);
CMDRESULT cbScriptSetIp(int argc, char* argv[]);
CMDRESULT cbScriptError(int argc, char* argv[]);
CMDRESULT cbScriptSetTitle(int argc, char* argv[]);
CMDRESULT cbScriptSetInfoLine(int argc, char* argv[]);

#endif // _SIMPLESCRIPT_H
