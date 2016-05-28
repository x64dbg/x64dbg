#pragma once

#include "_global.h"
#include "_dbgfunctions.h"

bool HandlesEnum(duint pid, std::vector<HANDLEINFO> & handlesList);
bool HandlesGetName(HANDLE hProcess, HANDLE remoteHandle, String & name, String & typeName);