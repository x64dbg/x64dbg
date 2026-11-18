// Keeps the public header valid C.
#include <ElfBug/api/elfbug_api.h>
#include <stddef.h>

int elfbug_api_compiles_as_c(void)
{
    ElfBugThreadInfo info = {0};
    ElfBugCallbacks callbacks = {0};
    ElfBugMemoryPage page = {0};
    ElfBugSignalFilter filter = {0};
    callbacks.onSignalFiltered = NULL;
    ElfBugSetSignalFilters(NULL, "");
    (void)ElfBugGetSignalFilter(NULL, 1, &filter);
    return (int)sizeof(info) + (int)sizeof(callbacks) + (int)sizeof(page) + (int)sizeof(filter);
}
