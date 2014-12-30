#include "console.h"
#include "threading.h"

static char msg[66000] = "";

void dputs(const char* text)
{
    dprintf("%s\n", text);
}

void dprintf(const char* format, ...)
{
    CriticalSectionLocker locker(LockDprintf);
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    GuiAddLogMessage(msg);
}
