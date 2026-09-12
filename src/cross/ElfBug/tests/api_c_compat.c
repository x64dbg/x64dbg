// Compiled as C11 so the public header stays usable from C.
#include <ElfBug/api/elfbug_api.h>

int elfbug_api_compiles_as_c(void)
{
    ElfBugThreadInfo info = {0};
    ElfBugCallbacks callbacks = {0};
    return (int)sizeof(info) + (int)sizeof(callbacks);
}
