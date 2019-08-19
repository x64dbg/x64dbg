#ifndef _XLIST_H
#define _XLIST_H

#include "stl_allocator.h"
#include <list>

template<class _Ty,
         class _Ax = stl_allocator<_Ty>>
class xlist
    : public std::list<_Ty, _Ax>
{
};

#endif

