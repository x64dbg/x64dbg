#pragma once

bool KdDebugEnabled();
//CMDRESULT KdDebugInit(int argc, char **argv);
DWORD WINAPI KdDebugLoop(LPVOID lpArg);
bool KdDebugQueryInterfaces();
bool KdDebugSetOptions();
bool KdLoadSymbols();

void KdSetProcessContext(ULONG64 EProcess);

bool KdInitialDriverEnum(ULONG64 LdrEntry);
bool KdInitialProcessEnum(ULONG64 EProcess);

bool KdWalkListEntry(ULONG64 ListStart, PVOID UserData, bool (* Callback)(ULONG64, PVOID));