Unusual instructions
====================

Unusual instructions are the instruction which is either privileged, invalid, have no use in ordinary applications, or make attempts to access sensitive information.

To notify the user of their existence, unusual instructions are usually special-colored in the disassembly.

The following instructions are considered unusual:

*  All privileged instructions (including I/O instructions and RDMSR/WRMSR)
*  RDTSC,RDTSCP,RDRAND,RDSEED
*  CPUID
*  SYSENTER and SYSCALL
*  UD2 and UD2B