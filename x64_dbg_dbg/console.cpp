#include "console.h"

void dputs(const char* text)
{
    dprintf("%s\n", text);
}

void dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char msg[deflen]="";
    vsprintf(msg, format, args);
    GuiAddLogMessage(msg);
}
