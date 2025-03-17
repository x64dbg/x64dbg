#ifndef _MSGQUEUE_H
#define _MSGQUEUE_H

#include "_global.h"
#include "concurrentqueue/blockingconcurrentqueue.h"

#define MAX_MESSAGES 256

// Message structure
struct MESSAGE
{
    int msg = -1;
    duint param1 = 0;
    duint param2 = 0;
};

// Message stack structure
class MESSAGE_STACK
{
public:
    moodycamel::BlockingConcurrentQueue<MESSAGE> msgs;

    int WaitingCalls = 0; // Number of threads waiting
    bool Destroy = false; // Destroy stack as soon as possible
};

// Function definitions
MESSAGE_STACK* MsgAllocStack();
void MsgFreeStack(MESSAGE_STACK* Stack);
bool MsgSend(MESSAGE_STACK* Stack, int Msg, duint Param1, duint Param2);
bool MsgGet(MESSAGE_STACK* Stack, MESSAGE* Msg);
void MsgWait(MESSAGE_STACK* Stack, MESSAGE* Msg);

#endif // _MSGQUEUE_H
