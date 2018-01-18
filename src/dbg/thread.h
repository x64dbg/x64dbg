#ifndef _THREAD_H
#define _THREAD_H

#include "_global.h"
#include "ntdll/ntdll.h"

void ThreadCreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void ThreadExit(DWORD ThreadId);
void ThreadClear();
int ThreadGetCount();
void ThreadGetList(THREADLIST* list);
void ThreadGetList(std::vector<THREADINFO> & list);
bool ThreadGetInfo(DWORD ThreadId, THREADINFO & info);
bool ThreadIsValid(DWORD ThreadId);
bool ThreadSetName(DWORD ThreadId, const char* name);
bool ThreadGetTib(duint TEBAddress, NT_TIB* Tib);
bool ThreadGetTeb(duint TEBAddress, TEB* Teb);
int ThreadGetSuspendCount(HANDLE Thread);
THREADPRIORITY ThreadGetPriority(HANDLE Thread);
DWORD ThreadGetLastErrorTEB(ULONG_PTR ThreadLocalBase);
DWORD ThreadGetLastError(DWORD ThreadId);
NTSTATUS ThreadGetLastStatusTEB(ULONG_PTR ThreadLocalBase);
NTSTATUS ThreadGetLastStatus(DWORD ThreadId);
bool ThreadSetName(DWORD dwThreadId, const char* name);
bool ThreadGetName(DWORD ThreadId, char* Name);
HANDLE ThreadGetHandle(DWORD ThreadId);
DWORD ThreadGetId(HANDLE Thread);
int ThreadSuspendAll();
int ThreadResumeAll();
ULONG_PTR ThreadGetLocalBase(DWORD ThreadId);
ULONG64 ThreadQueryCycleTime(HANDLE hThread);
void ThreadUpdateWaitReasons();

#endif // _THREAD_H