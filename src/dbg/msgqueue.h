#ifndef _MSGQUEUE_H
#define _MSGQUEUE_H

#include "_global.h"
#include <windows.h>

#define MAX_MESSAGES 256

//message structure
struct MESSAGE
{
    int msg;
    duint param1;
    duint param2;
};

//message stack structure
struct MESSAGE_STACK
{
    CRITICAL_SECTION cr;
    int stackpos;
    MESSAGE* msg[MAX_MESSAGES];
};

//function definitions
MESSAGE_STACK* MsgAllocStack();
void MsgFreeStack(MESSAGE_STACK* msgstack);
bool MsgSend(MESSAGE_STACK* msgstack, int msg, duint param1, duint param2);
bool MsgGet(MESSAGE_STACK* msgstack, MESSAGE* msg);
void MsgWait(MESSAGE_STACK* msgstack, MESSAGE* msg, bool* bStop);

#endif // _MSGQUEUE_H
