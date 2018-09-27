#pragma once

/*
MIT License

Copyright (c) 2018 Duncan Ogilvie

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <string>

typedef char* pchar_t;
typedef const char* pcchar_t;
typedef char* (*GetParameter_t)(long n);
using Alloc_t = decltype(malloc);
using Free_t = decltype(free);

enum
{
    X_UNDNAME_COMPLETE = 0x0, //Enables full undecoration.
    X_UNDNAME_NO_LEADING_UNDERSCORES = 0x1, //Removes leading underscores from Microsoft extended keywords.
    X_UNDNAME_NO_MS_KEYWORDS = 0x2, //Disables expansion of Microsoft extended keywords.
    X_UNDNAME_NO_FUNCTION_RETURNS = 0x4, //Disables expansion of return type for primary declaration.
    X_UNDNAME_NO_ALLOCATION_MODEL = 0x8, //Disables expansion of the declaration model.
    X_UNDNAME_NO_ALLOCATION_LANGUAGE = 0x10, //Disables expansion of the declaration language specifier.
    X_UNDNAME_NO_MS_THISTYPE = 0x20, //NYI Disable expansion of MS keywords on the 'this' type for primary declaration.
    X_UNDNAME_NO_CV_THISTYPE = 0x40, //NYI Disable expansion of CV modifiers on the 'this' type for primary declaration/
    X_UNDNAME_NO_THISTYPE = 0x60, //Disables all modifiers on the this type.
    X_UNDNAME_NO_ACCESS_SPECIFIERS = 0x80, //Disables expansion of access specifiers for members.
    X_UNDNAME_NO_THROW_SIGNATURES = 0x100, //Disables expansion of "throw-signatures" for functions and pointers to functions.
    X_UNDNAME_NO_MEMBER_TYPE = 0x200, //Disables expansion of static or virtual members.
    X_UNDNAME_NO_RETURN_UDT_MODEL = 0x400, //Disables expansion of the Microsoft model for UDT returns.
    X_UNDNAME_32_BIT_DECODE = 0x800, //Undecorates 32-bit decorated names.
    X_UNDNAME_NAME_ONLY = 0x1000, //Gets only the name for primary declaration; returns just [scope::]name. Expands template params.
    X_UNDNAME_TYPE_ONLY = 0x2000, //Input is just a type encoding; composes an abstract declarator.
    X_UNDNAME_HAVE_PARAMETERS = 0x4000, //The real template parameters are available.
    X_UNDNAME_NO_ECSU = 0x8000, //Suppresses enum/class/struct/union.
    X_UNDNAME_NO_IDENT_CHAR_CHECK = 0x10000, //Suppresses check for valid identifier characters.
    X_UNDNAME_NO_PTR64 = 0x20000, //Does not include ptr64 in output.
};

#if _MSC_VER != 1800
#error unDNameEx is undocumented and possibly unsupported on your runtime! Uncomment this line if you understand the risks and want continue regardless...
#endif //_MSC_VER

//undname.cxx
extern "C" pchar_t __cdecl __unDNameEx(_Out_opt_z_cap_(maxStringLength) pchar_t outputString,
                                       pcchar_t name,
                                       int maxStringLength,    // Note, COMMA is leading following optional arguments
                                       Alloc_t pAlloc,
                                       Free_t pFree,
                                       GetParameter_t pGetParameter,
                                       unsigned long disableFlags
                                      );

template<size_t MaxSize = 512>
inline bool undecorateName(const std::string & decoratedName, std::string & undecoratedName)
{
    //TODO: undocumented hack to have some kind of performance while undecorating names
    auto mymalloc = [](size_t size) { return emalloc(size, "symbolundecorator::undecoratedName"); };
    auto myfree = [](void* ptr) { return efree(ptr, "symbolundecorator::undecoratedName"); };

    undecoratedName.resize(max(MaxSize, decoratedName.length() * 2));
    if(!__unDNameEx((char*)undecoratedName.data(),
                    decoratedName.c_str(),
                    (int)undecoratedName.size(),
                    mymalloc,
                    myfree,
                    nullptr,
                    X_UNDNAME_COMPLETE))
    {
        undecoratedName.clear();
        return false;
    }
    else
    {
        undecoratedName.resize(strlen(undecoratedName.c_str()));
        if(decoratedName == undecoratedName)
            undecoratedName = ""; //https://stackoverflow.com/a/18299315
        return true;
    }
}