#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

__declspec(dllexport) char* LLVMDemangle(const char* MangledName);
__declspec(dllexport) void LLVMDemangleFree(char* DemangledName);

#ifdef __cplusplus
}
#endif
