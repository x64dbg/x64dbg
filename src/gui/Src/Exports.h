#pragma once

#include "Imports.h"

/************************************************************************************
                            Exported Function Prototypes
************************************************************************************/

#ifdef BUILD_LIB
extern "C" __declspec(dllexport) int _gui_guiinit(int argc, char* argv[]);
extern "C" __declspec(dllexport) void* _gui_sendmessage(GUIMSG type, void* param1, void* param2);
extern "C" __declspec(dllexport) const char* _gui_translate_text(const char* source);
#endif
