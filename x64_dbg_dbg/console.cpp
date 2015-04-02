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
	dprintf_args(Format, args);
    va_end(args);
}

void dprintf_args(const char* Format, va_list Args)
{
	char buffer[16384];
	vsnprintf_s(buffer, _TRUNCATE, Format, Args);

	GuiAddLogMessage(buffer);
}