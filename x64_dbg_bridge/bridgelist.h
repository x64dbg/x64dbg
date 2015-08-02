#ifndef _LIST_H
#define _LIST_H

typedef struct
{
    int count; //Number of element in the list.
    size_t size; //Size of list in bytes (used for type checking).
    void* data; //Pointer to the list contents. Must be deleted by the caller using BridgeFree (or List::Free).
} ListInfo;

#define ListOf(Type) ListInfo*

#ifdef __cplusplus

#include <vector>

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
    inline int count() const
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
            BridgeFree(_listInfo.data);
            _listInfo.data = nullptr;
        }
    }

    /**
    \brief Reference operator (cleans up the previous list)
    \return Pointer to the ListInfo.
    */
    inline ListInfo* operator&()
    {
        cleanup();
        return &_listInfo;
    }

    /**
    \brief Array indexer operator. This will crash if you try to access out-of-bounds.
    \param index Zero-based index of the item you want to get.
    \return Reference to a value at that index.
    */
    inline Type & operator[](size_t index) const
    {
        if(index >= size_t(count()))  //make sure the out-of-bounds access is caught as soon as possible.
            __debugbreak();
        return data()[index];
    }

    /**
    \brief Copies data to a ListInfo structure..
    \param [out] listInfo If non-null, information describing the list.
    \param listData Data to copy in the ListInfo structure.
    \return true if it succeeds, false if it fails.
    */
    static inline bool CopyData(ListInfo* listInfo, const std::vector<Type> & listData)
    {
        if (!listInfo)
            return false;
        listInfo->count = int(listData.size());
        listInfo->size = listInfo->count * sizeof(Type);
        if (listInfo->count)
        {
            listInfo->data = BridgeAlloc(listInfo->size);
            Type* curItem = reinterpret_cast<Type*>(listInfo->data);
            for (const auto & item : listData)
            {
                *curItem = item;
                ++curItem;
            }
        }
        else
            listInfo->data = nullptr;
        return true;
    }

private:
    ListInfo _listInfo;
};

#endif //__cplusplus

#endif //_LIST_H