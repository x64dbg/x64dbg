#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "_global.h"

void dputs(const char* Text);
void dprintf(const char* Format, ...);
void dprintf_args(const char* Format, va_list Args);
void dputs_untranslated(const char* Text);
void dprintf_untranslated(const char* Format, ...);
void dprintf_args_untranslated(const char* Format, va_list Args);

#endif // _CONSOLE_H