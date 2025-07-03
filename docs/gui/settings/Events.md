# Events

This page contains a list of debug events. You can specify whether the program should pause when the debug events happen.

## System Breakpoint

This event happens when the process is being initialized but have not begun to execute user code yet.

## TLS Callbacks

Set a single-shoot breakpoint on the TLS callbacks when a module is loaded to pause at the TLS callback.

## Entry Breakpoint

Set a single-shoot breakpoint on the entry of the EXE module to pause at the entry point.

## DLL Entry

Set a single-shoot breakpoint on the entry of the DLL module to pause at the entry point.

## Attach Breakpoint

This event happens when the process is successfully attached.

## Thread Entry

Set a single-shoot breakpoint on the entry of the thread when a thread is about to run.

## DLL Load

Pause when a DLL is mapped to the address space.

## DLL Unload

Pause when a DLL is unmapped from the address space.

## Thread Start

Pause when a new thread is about to run.

## Thread End

Pause when a thread has exited.

## Debug Strings

Pause when a debug string is emitted by the debuggee.
