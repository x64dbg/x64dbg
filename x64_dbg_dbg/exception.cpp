#include "exception.h"
#include <map>

static std::map<unsigned int, const char*> exceptionNames;

void exceptioninit()
{
    exceptionNames.insert(std::make_pair(0x40000005, "STATUS_SEGMENT_NOTIFICATION"));
    exceptionNames.insert(std::make_pair(0x4000001C, "STATUS_WX86_UNSIMULATE"));
    exceptionNames.insert(std::make_pair(0x4000001D, "STATUS_WX86_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x4000001E, "STATUS_WX86_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x4000001F, "STATUS_WX86_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x40000020, "STATUS_WX86_EXCEPTION_CONTINUE"));
    exceptionNames.insert(std::make_pair(0x40000021, "STATUS_WX86_EXCEPTION_LASTCHANCE"));
    exceptionNames.insert(std::make_pair(0x40000022, "STATUS_WX86_EXCEPTION_CHAIN"));
    exceptionNames.insert(std::make_pair(0x40000028, "STATUS_WX86_CREATEWX86TIB"));
    exceptionNames.insert(std::make_pair(0x40010003, "DBG_TERMINATE_THREAD"));
    exceptionNames.insert(std::make_pair(0x40010004, "DBG_TERMINATE_PROCESS"));
    exceptionNames.insert(std::make_pair(0x40010005, "DBG_CONTROL_C"));
    exceptionNames.insert(std::make_pair(0x40010006, "DBG_PRINTEXCEPTION_C"));
    exceptionNames.insert(std::make_pair(0x40010007, "DBG_RIPEXCEPTION"));
    exceptionNames.insert(std::make_pair(0x40010008, "DBG_CONTROL_BREAK"));
    exceptionNames.insert(std::make_pair(0x40010009, "DBG_COMMAND_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0x80000001, "EXCEPTION_GUARD_PAGE"));
    exceptionNames.insert(std::make_pair(0x80000002, "EXCEPTION_DATATYPE_MISALIGNMENT"));
    exceptionNames.insert(std::make_pair(0x80000003, "EXCEPTION_BREAKPOINT"));
    exceptionNames.insert(std::make_pair(0x80000004, "EXCEPTION_SINGLE_STEP"));
    exceptionNames.insert(std::make_pair(0x80000026, "STATUS_LONGJUMP"));
    exceptionNames.insert(std::make_pair(0x80000029, "STATUS_UNWIND_CONSOLIDATE"));
    exceptionNames.insert(std::make_pair(0x80010001, "DBG_EXCEPTION_NOT_HANDLED"));
    exceptionNames.insert(std::make_pair(0xC0000005, "EXCEPTION_ACCESS_VIOLATION"));
    exceptionNames.insert(std::make_pair(0xC0000006, "EXCEPTION_IN_PAGE_ERROR"));
    exceptionNames.insert(std::make_pair(0xC0000008, "EXCEPTION_INVALID_HANDLE"));
    exceptionNames.insert(std::make_pair(0xC000000D, "STATUS_INVALID_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000017, "STATUS_NO_MEMORY"));
    exceptionNames.insert(std::make_pair(0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xC0000026, "EXCEPTION_INVALID_DISPOSITION"));
    exceptionNames.insert(std::make_pair(0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
    exceptionNames.insert(std::make_pair(0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND"));
    exceptionNames.insert(std::make_pair(0xC000008E, "EXCEPTION_FLT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT"));
    exceptionNames.insert(std::make_pair(0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION"));
    exceptionNames.insert(std::make_pair(0xC0000091, "EXCEPTION_FLT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000092, "EXCEPTION_FLT_STACK_CHECK"));
    exceptionNames.insert(std::make_pair(0xC0000093, "EXCEPTION_FLT_UNDERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO"));
    exceptionNames.insert(std::make_pair(0xC0000095, "EXCEPTION_INT_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000096, "EXCEPTION_PRIV_INSTRUCTION"));
    exceptionNames.insert(std::make_pair(0xC00000FD, "EXCEPTION_STACK_OVERFLOW"));
    exceptionNames.insert(std::make_pair(0xC0000135, "STATUS_DLL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000138, "STATUS_ORDINAL_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC0000139, "STATUS_ENTRYPOINT_NOT_FOUND"));
    exceptionNames.insert(std::make_pair(0xC000013A, "STATUS_CONTROL_C_EXIT"));
    exceptionNames.insert(std::make_pair(0xC0000142, "STATUS_DLL_INIT_FAILED"));
    exceptionNames.insert(std::make_pair(0xC000014A, "STATUS_ILLEGAL_FLOAT_CONTEXT"));
    exceptionNames.insert(std::make_pair(0xC0000194, "EXCEPTION_POSSIBLE_DEADLOCK"));
    exceptionNames.insert(std::make_pair(0xC00002B4, "STATUS_FLOAT_MULTIPLE_FAULTS"));
    exceptionNames.insert(std::make_pair(0xC00002B5, "STATUS_FLOAT_MULTIPLE_TRAPS"));
    exceptionNames.insert(std::make_pair(0xC00002C5, "STATUS_DATATYPE_MISALIGNMENT_ERROR"));
    exceptionNames.insert(std::make_pair(0xC00002C9, "STATUS_REG_NAT_CONSUMPTION"));
    exceptionNames.insert(std::make_pair(0xC0000409, "STATUS_STACK_BUFFER_OVERRUN"));
    exceptionNames.insert(std::make_pair(0xC0000417, "STATUS_INVALID_CRUNTIME_PARAMETER"));
    exceptionNames.insert(std::make_pair(0xC0000420, "STATUS_ASSERTION_FAILURE"));
    exceptionNames.insert(std::make_pair(0x04242420, "CLRDBG_NOTIFICATION_EXCEPTION_CODE"));
    exceptionNames.insert(std::make_pair(0xE0434352, "CLR_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xE06D7363, "CPP_EH_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0x406D1388, "MS_VC_EXCEPTION"));
    exceptionNames.insert(std::make_pair(0xC00001A5, "STATUS_INVALID_EXCEPTION_HANDLER"));
}

const char* exceptionnamefromcode(unsigned int ExceptionCode)
{
    if(!exceptionNames.count(ExceptionCode))
        return 0;
    return exceptionNames[ExceptionCode];
}