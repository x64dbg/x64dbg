#ifndef _STL_ALLOCATOR_H
#define _STL_ALLOCATOR_H

// See http://www.codeproject.com/Articles/1089905/A-Custom-STL-std-allocator-Replacement-Improves-Performance-

#include "xallocator.h"
#include "Fault.h"

template <typename T> class stl_allocator;
template <> class stl_allocator<void>
{
public:
    typedef void* pointer;
    typedef const void* const_pointer;
    // reference to void members are impossible.
    typedef void value_type;

    template <class U>
    struct rebind { typedef stl_allocator<U> other; };
};

/// @brief stl_allocator is STL-compatible allocator used to provide fixed
/// block allocations.
/// @details The default allocator for the STL is the global heap. The
/// stl_allocator is custom allocator where xmalloc/xfree is used to obtain
/// and release memory.
template <typename T>
class stl_allocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T & reference;
    typedef const T & const_reference;
    typedef T value_type;

    /// Constructor
    stl_allocator() {}

    /// Destructor
    ~stl_allocator() {}

    /// Copy constructor
    template <class U> stl_allocator(const stl_allocator<U> &) {}

    template <class U>
    struct rebind { typedef stl_allocator<U> other; };

    /// Return reference address.
    /// @return Pointer to T memory.
    pointer address(reference x) const {return &x;}

    /// Return reference address.
    /// @return Const pointer to T memory.
    const_pointer address(const_reference x) const {return &x;}

    /// Get the maximum size of memory.
    /// @return Max memory size in bytes.
    size_type max_size() const throw() {return size_t(-1) / sizeof(value_type);}

    /// Allocates a fixed block of memory
    /// @param[in] n - size of memory to allocate in bytes
    /// @param[in] hint
    /// @return Pointer to the allocated memory.
    pointer allocate(size_type n, stl_allocator<void>::const_pointer hint = 0)
    {
        return static_cast<pointer>(xmalloc(n * sizeof(T)));
    }

    /// Deallocate a previously allocated fixed memory block.
    /// @param[in] p - pointer to the memory block
    /// @param[in] n - size of memory in bytes
    void deallocate(pointer p, size_type n)
    {
        xfree(p);
    }

    /// Constructs a new instance.
    /// @param[in] p - pointer to the memory where the instance is constructed
    ///     using placement new.
    /// @param[in] val - instance of object to copy construct.
    void construct(pointer p, const T & val)
    {
        new(static_cast<void*>(p)) T(val);
    }

    /// Create a new object instance using placement new.
    /// @param[in] p - pointer to the memory where the instance is constructed
    ///     using placement new.
    void construct(pointer p)
    {
        new(static_cast<void*>(p)) T();
    }

    /// Destroys an instance. Objects created with placement new must
    /// explicitly call the destructor.
    /// @param[in] p - pointer to object instance.
    void destroy(pointer p)
    {
        p->~T();
    }
};

template <typename T, typename U>
inline bool operator==(const stl_allocator<T> &, const stl_allocator<U>) {return true;}

template <typename T, typename U>
inline bool operator!=(const stl_allocator<T> &, const stl_allocator<U>) {return false;}

// For VC6/STLPort 4-5-3 see /stl/_alloc.h, line 464
// "If custom allocators are being used without member template classes support :
// user (on purpose) is forced to define rebind/get operations !!!"
#ifdef _WIN32
#define STD_ALLOC_CDECL __cdecl
#else
#define STD_ALLOC_CDECL
#endif

namespace std
{
    template <class _Tp1, class _Tp2>
    inline stl_allocator<_Tp2> & STD_ALLOC_CDECL
    __stl_alloc_rebind(stl_allocator<_Tp1> & __a, const _Tp2*)
    {
        return (stl_allocator<_Tp2> &)(__a);
    }

    template <class _Tp1, class _Tp2>
    inline stl_allocator<_Tp2> STD_ALLOC_CDECL
    __stl_alloc_create(const stl_allocator<_Tp1> &, const _Tp2*)
    {
        return stl_allocator<_Tp2>();
    }
}

#endif
