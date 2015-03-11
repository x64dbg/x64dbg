#ifndef _THREAD_H
#define _THREAD_H

#include "_global.h"
#include "debugger.h"

//functions
void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void threadexit(DWORD dwThreadId);
void threadclear();
void ThreadGetList(THREADLIST* list);
bool ThreadIsValid(DWORD dwThreadId);
bool ThreadSetName(DWORD dwTHreadId, const char* name);
HANDLE ThreadGetHandle(DWORD dwThreadId);
DWORD ThreadGetId(HANDLE hThread);
int ThreadGetCount();
int ThreadSuspendAll();
int ThreadResumeAll();

#endif //_THREAD_H
