#pragma once

#include <Windows.h>

bool InitializeSignatureCheck();
HMODULE WINAPI LoadLibraryCheckedW(const wchar_t* szDll, bool allowFailure);
HMODULE WINAPI LoadLibraryCheckedA(const char* szDll, bool allowFailure);