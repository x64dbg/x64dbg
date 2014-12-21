#include "msgqueue.h"
#include <stdio.h>

//allocate a message (internal)
static MESSAGE* msgalloc()
{
    return (MESSAGE*)emalloc(sizeof(MESSAGE), "msgalloc:msg");
}

//free a message (internal)
static void msgfree(MESSAGE* msg)
{
    efree(msg, "msgfree:msg");
}

//allocate a message stack
MESSAGE_STACK* msgallocstack()
{
    MESSAGE_STACK* msgstack = (MESSAGE_STACK*)emalloc(sizeof(MESSAGE_STACK), "msgallocstack:msgstack");
    if(!msgstack)
        return 0;
    memset(msgstack, 0, sizeof(MESSAGE_STACK));
    InitializeCriticalSection(&msgstack->cr);
    return msgstack;
}

//free a message stack
void msgfreestack(MESSAGE_STACK* msgstack)
{
    DeleteCriticalSection(&msgstack->cr);
    int stackpos = msgstack->stackpos;
    for(int i = 0; i < stackpos; i++) //free all messages left in stack
        msgfree(msgstack->msg[i]);
    efree(msgstack, "msgfreestack:msgstack");
}

//add a message to the stack
bool msgsend(MESSAGE_STACK* msgstack, int msg, uint param1, uint param2)
{
    CRITICAL_SECTION* cr = &msgstack->cr;
    EnterCriticalSection(cr);
    int stackpos = msgstack->stackpos;
    if(stackpos >= MAX_MESSAGES)
    {
        LeaveCriticalSection(cr);
        return false;
    }
    MESSAGE* newmsg = msgalloc();
    if(!newmsg)
    {
        LeaveCriticalSection(cr);
        return false;
    }
    newmsg->msg = msg;
    newmsg->param1 = param1;
    newmsg->param2 = param2;
    msgstack->msg[stackpos] = newmsg;
    msgstack->stackpos++; //increase stack pointer
    LeaveCriticalSection(cr);
    return true;
}

//get a message from the stack (will return false when there are no messages)
bool msgget(MESSAGE_STACK* msgstack, MESSAGE* msg)
{
    CRITICAL_SECTION* cr = &msgstack->cr;
    EnterCriticalSection(cr);
    int stackpos = msgstack->stackpos;
    if(!msgstack->stackpos) //no messages to process
    {
        LeaveCriticalSection(cr);
        return false;
    }
    msgstack->stackpos--; //current message is at stackpos-1
    stackpos--;
    MESSAGE* stackmsg = msgstack->msg[stackpos];
    memcpy(msg, stackmsg, sizeof(MESSAGE));
    msgfree(stackmsg);
    msgstack->msg[stackpos] = 0;
    LeaveCriticalSection(cr);
    return true;
}

//wait for a message on the specified stack
void msgwait(MESSAGE_STACK* msgstack, MESSAGE* msg)
{
    while(!msgget(msgstack, msg))
        Sleep(1);
}
