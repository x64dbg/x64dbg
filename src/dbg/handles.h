#pragma once

#include "_global.h"
#include "_dbgfunctions.h"

bool HandlesEnum(std::vector<HANDLEINFO> & handlesList);
bool HandlesGetName(HANDLE remoteHandle, String & name, String & typeName);
bool HandlesEnumWindows(std::vector<WINDOW_INFO> & windowsList);
bool HandlesEnumHeaps(std::vector<HEAPINFO> & heapList);
String LoadedAntiCheatDrivers();
