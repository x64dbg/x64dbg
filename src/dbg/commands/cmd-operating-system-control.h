#pragma once

#include "command.h"

bool cbGetPrivilegeState(int argc, char* argv[]);
bool cbEnablePrivilege(int argc, char* argv[]);
bool cbDisablePrivilege(int argc, char* argv[]);
bool cbHandleClose(int argc, char* argv[]);
bool cbEnableWindow(int argc, char* argv[]);
bool cbDisableWindow(int argc, char* argv[]);