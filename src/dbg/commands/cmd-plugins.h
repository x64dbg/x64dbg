#pragma once

#include "command.h"

bool cbDebugStartScylla(int argc, char* argv[]);
bool cbInstrPluginLoad(int argc, char* argv[]);
bool cbInstrPluginUnload(int argc, char* argv[]);
bool cbInstrPluginReload(int argc, char* argv[]);