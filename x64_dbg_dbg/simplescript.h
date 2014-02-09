#ifndef _SIMPLESCRIPT_H
#define _SIMPLESCRIPT_H

#include "command.h"

bool scriptload(const char* filename);
void scriptunload();
void scriptrun(int destline);
void scriptstep();
bool scriptbptoggle(int line);
bool scriptbpget(int line);
bool scriptcmdexec(const char* command);
void scriptabort();

CMDRESULT cbScriptAddLine(int argc, char* argv[]);
CMDRESULT cbScriptClear(int argc, char* argv[]);
CMDRESULT cbScriptSetIp(int argc, char* argv[]);
CMDRESULT cbScriptError(int argc, char* argv[]);
CMDRESULT cbScriptSetTitle(int argc, char* argv[]);
CMDRESULT cbScriptSetInfoLine(int argc, char* argv[]);

#endif // _SIMPLESCRIPT_H
