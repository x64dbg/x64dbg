#pragma once

void CrashDumpInitialize();
LONG CALLBACK CrashDumpExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);
void InvalidParameterHandler(const wchar_t* Expression, const wchar_t* Function, const wchar_t* File, unsigned int Line, uintptr_t Reserved);
void TerminateHandler();
void AbortHandler(int Signal);