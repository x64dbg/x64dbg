#ifndef _MSGQUEUE_H
#define _MSGQUEUE_H

#include "_global.h"
#include <windows.h>
#include <agents.h>

// Message structure
struct MESSAGE
{
    int msg;
    uint param1;
    uint param2;
};

// Message stack structure.
// Supports an unlimited number of messages.
struct MESSAGE_STACK
{
	concurrency::unbounded_buffer<MESSAGE> FIFOStack;
};

MESSAGE_STACK* MsgAllocStack();
void MsgFreeStack(MESSAGE_STACK* Stack);
bool MsgSend(MESSAGE_STACK* Stack, int Msg, uint Param1, uint Param2);
bool MsgGet(MESSAGE_STACK* Stack, MESSAGE* Message);
void MsgWait(MESSAGE_STACK* Stack, MESSAGE* Message);

#endif // _MSGQUEUE_H
