#pragma once

#include "_global.h"
#include "debugger.h"

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void ThreadExit(DWORD ThreadId);
void ThreadClear();
int ThreadGetCount();
void ThreadGetList(THREADLIST* list);
bool ThreadIsValid(DWORD ThreadId);
bool ThreadSetName(DWORD ThreadId, const char* name);
bool ThreadGetTeb(duint TEBAddress, TEB* Teb);
int ThreadGetSuspendCount(HANDLE Thread);
THREADPRIORITY ThreadGetPriority(HANDLE Thread);
THREADWAITREASON ThreadGetWaitReason(HANDLE Thread);
DWORD ThreadGetLastErrorTEB(ULONG_PTR ThreadLocalBase);
DWORD ThreadGetLastError(DWORD ThreadId);
bool ThreadSetName(DWORD dwThreadId, const char* name);
HANDLE ThreadGetHandle(DWORD ThreadId);
DWORD ThreadGetId(HANDLE Thread);
int ThreadSuspendAll();
int ThreadResumeAll();