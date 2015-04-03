/**
 @file msgqueue.cpp

 @brief Implements the msgqueue class.
 */

#include "msgqueue.h"

// Allocate a message stack
MESSAGE_STACK* MsgAllocStack()
{
	// Allocate memory for the structure
	PVOID memoryBuffer = emalloc(sizeof(MESSAGE_STACK), "MsgAllocStack:memoryBuffer");

	if (!memoryBuffer)
		return nullptr;

	// Use placement new to ensure all constructors are called correctly
    return new(memoryBuffer) MESSAGE_STACK;
}

// Free a message stack and all messages in the queue
void MsgFreeStack(MESSAGE_STACK* Stack)
{
	// Destructor must be called manually due to placement new
	Stack->FIFOStack.~unbounded_buffer();

	// Free memory
	efree(Stack, "MsgFreeStack:Stack");
}

// Add a message to the stack
bool MsgSend(MESSAGE_STACK* Stack, int Msg, uint Param1, uint Param2)
{
	MESSAGE messageInfo;
	messageInfo.msg		= Msg;
	messageInfo.param1	= Param1;
	messageInfo.param2	= Param2;

	// Asynchronous send. Return value doesn't matter.
	concurrency::asend(Stack->FIFOStack, messageInfo);
    return true;
}

// Get a message from the stack (will return false when there are no messages)
bool MsgGet(MESSAGE_STACK* Stack, MESSAGE* Message)
{
	return concurrency::try_receive(Stack->FIFOStack, *Message);
}

// Wait for a message on the specified stack
void MsgWait(MESSAGE_STACK* Stack, MESSAGE* Message)
{
	*Message = concurrency::receive(Stack->FIFOStack);
}
