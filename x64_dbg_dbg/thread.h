#ifndef _THREAD_H
#define _THREAD_H

#include "_global.h"
#include "debugger.h"

//functions
void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void threadexit(DWORD dwThreadId);
void threadclear();
void threadgetlist(THREADLIST* list);
bool threadisvalid(DWORD dwThreadId);
bool threadsetname(DWORD dwTHreadId, const char* name);
HANDLE threadgethandle(DWORD dwThreadId);
DWORD threadgetid(HANDLE hThread);
int threadgetcount();
int threadsuspendall();
int threadresumeall();

#endif //_THREAD_H
