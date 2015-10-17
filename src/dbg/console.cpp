/**
\file console.cpp
\brief Implements the console class.
*/

#include "console.h"

/**
\brief Print a line with text, terminated with a newline to the console.
\param text The text to print.
*/
void dputs(const char* Text)
{
    dprintf("%s\n", Text);
}

/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
*/
void dprintf(const char* Format, ...)
{
    va_list args;

    va_start(args, Format);
    dprintf_args(Format, args);
    va_end(args);
}

/**
\brief Print a formatted string to the console.
\param format The printf format to use (see documentation of printf for more information).
\param Args The argument buffer passed to the string parser.
*/
void dprintf_args(const char* Format, va_list Args)
{
    char buffer[16384];
    vsnprintf_s(buffer, _TRUNCATE, Format, Args);

    GuiAddLogMessage(buffer);
}
