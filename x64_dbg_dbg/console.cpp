#include "console.h"

void dputs(const char* text)
{
    dprintf("%s\n", text);
}

void dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Memory<char*> msg(66000);
    vsnprintf(msg, msg.size(), format, args);
    GuiAddLogMessage(msg);
}
