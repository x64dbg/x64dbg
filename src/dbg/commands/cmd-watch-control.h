#pragma once

#include "command.h"

CMDRESULT cbAddWatch(int argc, char* argv[]);
CMDRESULT cbDelWatch(int argc, char* argv[]);
CMDRESULT cbSetWatchdog(int argc, char* argv[]);
CMDRESULT cbSetWatchExpression(int argc, char* argv[]);
CMDRESULT cbSetWatchName(int argc, char* argv[]);
CMDRESULT cbCheckWatchdog(int argc, char* argv[]);