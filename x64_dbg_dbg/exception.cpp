#include <unordered_map>
#include "exception.h"

std::unordered_map<unsigned int, const char*> ExceptionNames;

void ExceptionCodeInit()
{
    ExceptionNames.clear();
    ExceptionNames.insert(std::make_pair(0x000006BA, "RPC_S_SERVER_UNAVAILABLE"));
    ExceptionNames.insert(std::make_pair(0x0000071A, "RPC_S_CALL_CANCELLED"));
    ExceptionNames.insert(std::make_pair(0x04242420, "CLRDBG_NOTIFICATION_EXCEPTION_CODE"));
    ExceptionNames.insert(std::make_pair(0x40000005, "STATUS_SEGMENT_NOTIFICATION"));
    ExceptionNames.insert(std::make_pair(0x4000001C, "STATUS_WX86_UNSIMULATE"));
    ExceptionNames.insert(std::make_pair(0x4000001D, "STATUS_WX86_CONTINUE"));
    ExceptionNames.insert(std::make_pair(0x4000001E, "STATUS_WX86_SINGLE_STEP"));
    ExceptionNames.insert(std::make_pair(0x4000001F, "STATUS_WX86_BREAKPOINT"));
    ExceptionNames.insert(std::make_pair(0x40000020, "STATUS_WX86_EXCEPTION_CONTINUE"));
    ExceptionNames.insert(std::make_pair(0x40000021, "STATUS_WX86_EXCEPTION_LASTCHANCE"));
    ExceptionNames.insert(std::make_pair(0x40000022, "STATUS_WX86_EXCEPTION_CHAIN"));
    ExceptionNames.insert(std::make_pair(0x40000028, "STATUS_WX86_CREATEWX86TIB"));
    ExceptionNames.insert(std::make_pair(0x40010003, "DBG_TERMINATE_THREAD"));
    ExceptionNames.insert(std::make_pair(0x40010004, "DBG_TERMINATE_PROCESS"));
    ExceptionNames.insert(std::make_pair(0x40010005, "DBG_CONTROL_C"));
    ExceptionNames.insert(std::make_pair(0x40010006, "DBG_PRINTEXCEPTION_C"));
    ExceptionNames.insert(std::make_pair(0x40010007, "DBG_RIPEXCEPTION"));
    ExceptionNames.insert(std::make_pair(0x40010008, "DBG_CONTROL_BREAK"));
    ExceptionNames.insert(std::make_pair(0x40010009, "DBG_COMMAND_EXCEPTION"));
    ExceptionNames.insert(std::make_pair(0x406D1388, "MS_VC_EXCEPTION"));
    ExceptionNames.insert(std::make_pair(0x80000001, "EXCEPTION_GUARD_PAGE"));
    ExceptionNames.insert(std::make_pair(0x80000002, "EXCEPTION_DATATYPE_MISALIGNMENT"));
    ExceptionNames.insert(std::make_pair(0x80000003, "EXCEPTION_BREAKPOINT"));
    ExceptionNames.insert(std::make_pair(0x80000004, "EXCEPTION_SINGLE_STEP"));
    ExceptionNames.insert(std::make_pair(0x80000026, "STATUS_LONGJUMP"));
    ExceptionNames.insert(std::make_pair(0x80000029, "STATUS_UNWIND_CONSOLIDATE"));
    ExceptionNames.insert(std::make_pair(0x80010001, "DBG_EXCEPTION_NOT_HANDLED"));
    ExceptionNames.insert(std::make_pair(0xC0000005, "EXCEPTION_ACCESS_VIOLATION"));
    ExceptionNames.insert(std::make_pair(0xC0000006, "EXCEPTION_IN_PAGE_ERROR"));
    ExceptionNames.insert(std::make_pair(0xC0000008, "EXCEPTION_INVALID_HANDLE"));
    ExceptionNames.insert(std::make_pair(0xC000000D, "STATUS_INVALID_PARAMETER"));
    ExceptionNames.insert(std::make_pair(0xC0000017, "STATUS_NO_MEMORY"));
    ExceptionNames.insert(std::make_pair(0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION"));
    ExceptionNames.insert(std::make_pair(0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION"));
    ExceptionNames.insert(std::make_pair(0xC0000026, "EXCEPTION_INVALID_DISPOSITION"));
    ExceptionNames.insert(std::make_pair(0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
    ExceptionNames.insert(std::make_pair(0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND"));
    ExceptionNames.insert(std::make_pair(0xC000008E, "EXCEPTION_FLT_DIVIDE_BY_ZERO"));
    ExceptionNames.insert(std::make_pair(0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT"));
    ExceptionNames.insert(std::make_pair(0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION"));
    ExceptionNames.insert(std::make_pair(0xC0000091, "EXCEPTION_FLT_OVERFLOW"));
    ExceptionNames.insert(std::make_pair(0xC0000092, "EXCEPTION_FLT_STACK_CHECK"));
    ExceptionNames.insert(std::make_pair(0xC0000093, "EXCEPTION_FLT_UNDERFLOW"));
    ExceptionNames.insert(std::make_pair(0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO"));
    ExceptionNames.insert(std::make_pair(0xC0000095, "EXCEPTION_INT_OVERFLOW"));
    ExceptionNames.insert(std::make_pair(0xC0000096, "EXCEPTION_PRIV_INSTRUCTION"));
    ExceptionNames.insert(std::make_pair(0xC00000FD, "EXCEPTION_STACK_OVERFLOW"));
    ExceptionNames.insert(std::make_pair(0xC0000135, "STATUS_DLL_NOT_FOUND"));
    ExceptionNames.insert(std::make_pair(0xC0000138, "STATUS_ORDINAL_NOT_FOUND"));
    ExceptionNames.insert(std::make_pair(0xC0000139, "STATUS_ENTRYPOINT_NOT_FOUND"));
    ExceptionNames.insert(std::make_pair(0xC000013A, "STATUS_CONTROL_C_EXIT"));
    ExceptionNames.insert(std::make_pair(0xC0000142, "STATUS_DLL_INIT_FAILED"));
    ExceptionNames.insert(std::make_pair(0xC000014A, "STATUS_ILLEGAL_FLOAT_CONTEXT"));
    ExceptionNames.insert(std::make_pair(0xC0000194, "EXCEPTION_POSSIBLE_DEADLOCK"));
    ExceptionNames.insert(std::make_pair(0xC00001A5, "STATUS_INVALID_EXCEPTION_HANDLER"));
    ExceptionNames.insert(std::make_pair(0xC00002B4, "STATUS_FLOAT_MULTIPLE_FAULTS"));
    ExceptionNames.insert(std::make_pair(0xC00002B5, "STATUS_FLOAT_MULTIPLE_TRAPS"));
    ExceptionNames.insert(std::make_pair(0xC00002C5, "STATUS_DATATYPE_MISALIGNMENT_ERROR"));
    ExceptionNames.insert(std::make_pair(0xC00002C9, "STATUS_REG_NAT_CONSUMPTION"));
    ExceptionNames.insert(std::make_pair(0xC0000409, "STATUS_STACK_BUFFER_OVERRUN"));
    ExceptionNames.insert(std::make_pair(0xC0000417, "STATUS_INVALID_CRUNTIME_PARAMETER"));
    ExceptionNames.insert(std::make_pair(0xC000041D, "STATUS_USER_CALLBACK"));
    ExceptionNames.insert(std::make_pair(0xC0000420, "STATUS_ASSERTION_FAILURE"));
    ExceptionNames.insert(std::make_pair(0xE0434352, "CLR_EXCEPTION"));
    ExceptionNames.insert(std::make_pair(0xE06D7363, "CPP_EH_EXCEPTION"));
}

const char* ExceptionCodeToName(unsigned int ExceptionCode)
{
    if(ExceptionNames.find(ExceptionCode) == ExceptionNames.end())
        return nullptr;

    return ExceptionNames[ExceptionCode];
}