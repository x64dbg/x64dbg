#ifndef _XSTRING_H
#define _XSTRING_H

#include "stl_allocator.h"
#include <string>

typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char>> xstring;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, stl_allocator<wchar_t>> xwstring;

#endif

