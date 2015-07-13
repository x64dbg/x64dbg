#pragma once

void CrashDumpInitialize();
LONG CALLBACK CrashDumpVectoredHandler(EXCEPTION_POINTERS* ExceptionInfo);