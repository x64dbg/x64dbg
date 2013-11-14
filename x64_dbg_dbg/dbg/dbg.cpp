#include "dbg.h"

// a sample exported function
void DLL_EXPORT dbg(const char* a)
{
    char b[256]="";
    strcpy(b,a);
}
