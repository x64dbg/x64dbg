#pragma once

#define MSR_IA32_SYSENTER_CS	0x00000174 // SYSCALL code segment
#define MSR_IA32_SYSENTER_ESP	0x00000175 // SYSCALL stack pointer address
#define MSR_IA32_SYSENTER_EIP	0x00000176 // SYSCALL execution address

// x86-64 specific MSRs
// Keep these defined in 32/64 compilation for consistency
#define MSR_IA32_EFER			0xC0000080 // Extended Feature Enable
#define MSR_IA32_STAR			0xC0000081 // SYSCALL Target Address
#define MSR_IA32_LSTAR			0xC0000082 // Long Mode SYSCALL Target Address
#define MSR_IA32_CSTAR			0xC0000083 // Compatibility Mode SYSCALL Target Address
#define MSR_IA32_FMASK			0xC0000084 // SYSCALL Flag Mask
#define MSR_IA32_FS_BASE		0xC0000100 // FS Base
#define MSR_IA32_GS_BASE		0xC0000101 // GS Base
#define MSR_IA32_KERNEL_GS_BASE	0xC0000102 // Kernel GS Base
#define MSR_TSC_AUX				0xC0000103 // Auxiliary TSC

union IA32_STAR_BITS
{
	ULONG64 All;

	struct
	{
		ULONG SyscallEip32 : 32;// 32-bit SYSCALL Target EIP
		ULONG SyscallCS : 16;	// SYSCALL CS and SS
		ULONG SysretCS : 16;	// SYSRET CS and SS
	};
};

union IA32_FMASK_BITS
{
	ULONG64 All;

	struct
	{
		ULONG SyscallFlagMask : 32;	// SYSCALL EFLAGS Mask
		ULONG Reserved : 32;		// Unused
	};
};

ULONG64 KdMsrRead	(ULONG Msr);
bool	KdMsrSet	(ULONG Msr, ULONG64 Value);