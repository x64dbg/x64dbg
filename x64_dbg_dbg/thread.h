#ifndef _THREAD_H
#define _THREAD_H

#include "_global.h"
#include "debugger.h"

//functions
void threadcreate(CREATE_THREAD_DEBUG_INFO* CreateThread);
void threadexit(DWORD dwThreadId);
void threadclear();
void threadgetlist(THREADLIST* list);

#endif //_THREAD_H
