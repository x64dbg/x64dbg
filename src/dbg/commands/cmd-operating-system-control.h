#pragma once

#include "command.h"

bool cbGetPrivilegeState(int argc, char* argv[]);
bool cbEnablePrivilege(int argc, char* argv[]);
bool cbDisablePrivilege(int argc, char* argv[]);
bool cbHandleClose(int argc, char* argv[]);