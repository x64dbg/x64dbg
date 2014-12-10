/**
 @file console.cpp

 @brief Implements the console class.
 */

#include "console.h"

/**
 @brief The msg[ 66000].
 */

/**
 @fn void dputs(const char* text)

 @brief Dputs the given text.

 @param text The text.
 */

void dputs(const char* text)
{
    dprintf("%s\n", text);
}

/**
 @fn void dprintf(const char* format, ...)

 @brief Dprintfs the given format.

 @param format Describes the format to use.
 */

void dprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Memory<char*> msg(66000);
    vsnprintf(msg, msg.size(), format, args);
    GuiAddLogMessage(msg);
}
