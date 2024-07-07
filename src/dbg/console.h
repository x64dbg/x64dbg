#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "_global.h"

void dputs(_In_z_ const char* Text);
void dprintf(_In_z_ _Printf_format_string_ const char* Format, ...);
void dprintf_args(_In_z_ _Printf_format_string_ const char* Format, va_list Args);
void dputs_untranslated(_In_z_ const char* Text);
void dprintf_untranslated(_In_z_ _Printf_format_string_ const char* Format, ...);
void dprintf_args_untranslated(_In_z_ _Printf_format_string_ const char* Format, va_list Args);
void dprint_untranslated_html(_In_z_ _Printf_format_string_ const char* Text);
void dprintf_html(_In_z_ _Printf_format_string_ const char* Format, ...);

#endif // _CONSOLE_H