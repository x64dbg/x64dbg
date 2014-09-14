/**
 @file msgqueue.cpp

 @brief Implements the msgqueue class.
 */

#include "msgqueue.h"
#include <stdio.h>

/**
 @fn static MESSAGE* msgalloc()

 @brief allocate a message (internal)

 @return null if it fails, else a MESSAGE*.
 */

static MESSAGE* msgalloc()
{
    return (MESSAGE*)emalloc(sizeof(MESSAGE), "msgalloc:msg");
}

/**
 @fn static void msgfree(MESSAGE* msg)

 @brief free a message (internal)

 @param [in,out] msg If non-null, the message.
 */

static void msgfree(MESSAGE* msg)
{
    efree(msg, "msgfree:msg");
}

/**
 @fn MESSAGE_STACK* msgallocstack()

 @brief allocate a message stack.

 @return null if it fails, else a MESSAGE_STACK*.
 */

MESSAGE_STACK* msgallocstack()
{
    MESSAGE_STACK* msgstack = (MESSAGE_STACK*)emalloc(sizeof(MESSAGE_STACK), "msgallocstack:msgstack");
    if(!msgstack)
        return 0;
    memset(msgstack, 0, sizeof(MESSAGE_STACK));
    InitializeCriticalSection(&msgstack->cr);
    return msgstack;
}

/**
 @fn void msgfreestack(MESSAGE_STACK* msgstack)

 @brief free a message stack.

 @param [in,out] msgstack If non-null, the msgstack.
 */

void msgfreestack(MESSAGE_STACK* msgstack)
{
    DeleteCriticalSection(&msgstack->cr);
    int stackpos = msgstack->stackpos;
    for(int i = 0; i < stackpos; i++) //free all messages left in stack
        msgfree(msgstack->msg[i]);
    efree(msgstack, "msgfreestack:msgstack");
}

/**
 @fn bool msgsend(MESSAGE_STACK* msgstack, int msg, uint param1, uint param2)

 @brief add a message to the stack.

 @param [in,out] msgstack If non-null, the msgstack.
 @param msg               The message.
 @param param1            The first parameter.
 @param param2            The second parameter.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn bool msgget(MESSAGE_STACK* msgstack, MESSAGE* msg)

 @brief get a message from the stack (will return false when there are no messages)

 @param [in,out] msgstack If non-null, the msgstack.
 @param [in,out] msg      If non-null, the message.

 @return true if it succeeds, false if it fails.
 */

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

/**
 @fn void msgwait(MESSAGE_STACK* msgstack, MESSAGE* msg)

 @brief wait for a message on the specified stack.

 @param [in,out] msgstack If non-null, the msgstack.
 @param [in,out] msg      If non-null, the message.
 */

void msgwait(MESSAGE_STACK* msgstack, MESSAGE* msg)
{
    while(!msgget(msgstack, msg))
        Sleep(1);
}
