#ifndef _MSGQUEUE_H
#define _MSGQUEUE_H

#include "_global.h"
#include <windows.h>

#define MAX_MESSAGES 256

//message structure
struct MESSAGE
{
    int msg;
    uint param1;
    uint param2;
};

//message stack structure
struct MESSAGE_STACK
{
    CRITICAL_SECTION cr;
    int stackpos;
    MESSAGE* msg[MAX_MESSAGES];
};

//function definitions
MESSAGE_STACK* msgallocstack();
void msgfreestack(MESSAGE_STACK* msgstack);
bool msgsend(MESSAGE_STACK* msgstack, int msg, uint param1, uint param2);
bool msgget(MESSAGE_STACK* msgstack, MESSAGE* msg);
void msgwait(MESSAGE_STACK* msgstack, MESSAGE* msg);

#endif // _MSGQUEUE_H
