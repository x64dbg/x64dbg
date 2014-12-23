/**
\file console.cpp
\brief Implements the console class.
*/

#include "console.h"
#include "threading.h"

static char msg[66000] = "";

/**
\brief Print a line with text, terminated with a newline to the console.
\param text The text to print.
*/
void dputs(const char* text)
{
    dprintf("%s\n", text);
}

/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
*/
void dprintf(const char* format, ...)
{
    CriticalSectionLocker locker(LockDprintf);
    va_list args;
    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    GuiAddLogMessage(msg);
}
