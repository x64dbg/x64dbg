#pragma once

#include "_global.h"
#include "debugger.h"

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void ThreadExit(DWORD dwThreadId);
void ThreadClear();
int ThreadGetCount();
void ThreadGetList(THREADLIST* list);
bool ThreadIsValid(DWORD dwThreadId);
bool ThreadSetName(DWORD dwTHreadId, const char* name);
bool ThreadGetTeb(uint TEBAddress, TEB* Teb);
int ThreadGetSuspendCount(HANDLE Thread);
THREADPRIORITY ThreadGetPriority(HANDLE Thread);
THREADWAITREASON ThreadGetWaitReason(HANDLE Thread);
DWORD ThreadGetLastError(uint tebAddress);
bool ThreadSetName(DWORD dwThreadId, const char* name);
HANDLE ThreadGetHandle(DWORD dwThreadId);
DWORD ThreadGetId(HANDLE hThread);
int ThreadSuspendAll();
int ThreadResumeAll();