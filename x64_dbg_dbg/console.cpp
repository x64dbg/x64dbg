#include "console.h"

static char msg[66000];

void dputs(const char* text)
{
    dprintf("%s\n", text);
}

void dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    *msg = 0;
    vsnprintf(msg, sizeof(msg), format, args);
    GuiAddLogMessage(msg);
}
