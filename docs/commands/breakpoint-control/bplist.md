# bplist

Get a list of breakpoints. This list includes their state (enabled/disabled), their type, their address and (optionally) their names.

## arguments

This command has no arguments.

## result

This command does not set any result variables. A list entry has the following format:

STATE:TYPE:ADDRESS\[:NAME\]

STATEcan be 0 or 1. 0 means disabled, 1 means enabled. Only singleshoot and 'normal' breakpoints can be disabled.

TYPEcan be one of the following values: BP, SS, HW and GP. BP stands for a normal breakpoint (set using the SetBPX command), SS stands for SINGLESHOT, HW stands for HARDWARE and GP stand for Guard Page, the way of setting memory breakpoints.

ADDRESSis the breakpoint address, given in 32 and 64 bits for the x32 and x64 debugger respectively.

NAMEis the name assigned to the breakpoint.
