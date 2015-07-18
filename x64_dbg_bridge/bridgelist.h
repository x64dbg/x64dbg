#ifndef _LIST_H
#define _LIST_H

typedef struct
{
    size_t count; //Number of element in the list.
    size_t size; //Size of list in bytes (used for type checking).
    void* data; //Pointer to the list contents. Must be deleted by the caller using BridgeFree (or List::Free).
} ListInfo;

#ifdef __cplusplus

/**
\brief A list object. This object is NOT thread safe.
\tparam Type List contents type.
*/
template<typename Type>
class List
{
public:
    /**
    \brief List constructor.
    \param _freeData (Optional) the free function.
    */
    explicit inline List()
    {
        memset(&_listInfo, 0, sizeof(_listInfo));
    }

    /**
    \brief List destructor.
    */
    inline ~List()
    {
        cleanup();
    }

    /**
    \brief Gets the list data.
    \return Returns ListInfo->data. Can be null if the list was never initialized. Will be destroyed once this object goes out of scope!
    */
    inline Type* data() const
    {
        return reinterpret_cast<Type*>(_listInfo.data);
    }

    /**
    \brief Gets the number of elements in the list. This will crash the program if the data is not consistent with the specified template argument.
    \return The number of elements in the list.
    */
    inline size_t count() const
    {
        if(_listInfo.size != _listInfo.count * sizeof(Type))  //make sure the user is using the correct type.
            __debugbreak();
        return _listInfo.count;
    }

    /**
    \brief Cleans up the list, freeing the list data when it is not null.
    */
    inline void cleanup()
    {
        if(_listInfo.data)
        {
            Free(_listInfo.data);
            _listInfo.data = nullptr;
        }
    }

    /**
    \brief Allocates memory with the given size.
    \param size The size to allocate.
    \return A pointer to the allocated data. Cannot return null.
    */
    static void* Alloc(size_t size)
    {
        return BridgeAlloc(size);
    }

    /**
    \brief Frees data allocated by List::Alloc.
    \param [in] The data to free.
    */
    static void Free(void* ptr)
    {
        BridgeFree(ptr);
    }

    /**
    \brief Reference operator.
    \return Pointer to the ListInfo.
    */
    ListInfo* operator&()
    {
        return &_listInfo;
    }

    /**
    \brief Array indexer operator. This will crash if you try to access out-of-bounds.
    \param index Zero-based index of the item you want to get.
    \return Reference to a value at that index.
    */
    inline Type & operator[](size_t index) const
    {
        if(index >= count())  //make sure the out-of-bounds access is caught as soon as possible.
            __debugbreak();
        return data()[index];
    }

private:
    ListInfo _listInfo;
};

#endif //__cplusplus

#endif //_LIST_H