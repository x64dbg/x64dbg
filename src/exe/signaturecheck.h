#pragma once

#include <Windows.h>

#define SIGNATURE_EXPORT extern "C" __declspec(dllexport)

bool InitializeSignatureCheck();
SIGNATURE_EXPORT HMODULE WINAPI LoadLibraryCheckedW(const wchar_t* szDll, bool allowFailure);
SIGNATURE_EXPORT HMODULE WINAPI LoadLibraryCheckedA(const char* szDll, bool allowFailure);