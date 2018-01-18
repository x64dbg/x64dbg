#pragma once

#include "command.h"

bool cbAddWatch(int argc, char* argv[]);
bool cbDelWatch(int argc, char* argv[]);
bool cbSetWatchdog(int argc, char* argv[]);
bool cbSetWatchExpression(int argc, char* argv[]);
bool cbSetWatchName(int argc, char* argv[]);
bool cbCheckWatchdog(int argc, char* argv[]);