#include "console.h"

void dputs(const char* Text)
{
    dprintf("%s\n", Text);
}

void dprintf(const char* Format, ...)
{
    va_list args;
    char buffer[16384];

    va_start(args, Format);
    vsnprintf_s(buffer, _TRUNCATE, Format, args);
    va_end(args);

    GuiAddLogMessage(buffer);
}
