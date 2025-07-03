Call Stack
==========

Call stack view displays the call stack of the current thread. It has 6 columns.

**Address** is the base address of the stack frame.

**To** is the address of the code that is going to return to.

**From** is the probable address of the routine that is going to return.

**Size** is the size of the call stack frame, in bytes.

**Comment** is a brief description of the call stack frame.

**Party** describes whether the procedure that is going to return to, is a user module or a system module.

When **Show suspected call stack frame** option in the context menu in call stack view is active, it will search through the entire stack for possible return addresses. When it is inactive, it will use standard stack walking algorithm to get the call stack. It will typically get more results when **Show suspected call stack frame** option is active, but some of which may not be actual call stack frames.