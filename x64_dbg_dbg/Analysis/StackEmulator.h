#pragma once
#include "Meta.h"
#include "../_global.h"

namespace tr4ce
{

/* since some compilers don't use "push" to change the stack for arguments (see MingGW)
   we have to track all modifications to the stack for better analysis
   therefore we emulate the stack by only storing the virtual address from the last write-access

   -> advantage: we can track all modifications of the register for better placing of comments
*/

const unsigned int MAX_STACKSIZE = 50;

#define STACK_ERROR -1

class StackEmulator
{

    UInt64 mStack[MAX_STACKSIZE];
    unsigned int mStackpointer;

public:
    StackEmulator(void);
    ~StackEmulator(void);


    void pushFrom(UInt64 addr);
    void popFrom(UInt64 addr);
    void modifyFrom(int relative_offset, UInt64 addr);

    void moveStackpointerBack(int offset);
    unsigned int pointerByOffset(int offset) const;
    UInt64 lastAccessAtOffset(int offset) const;

    void emulate(const DISASM* disasm);
};



};