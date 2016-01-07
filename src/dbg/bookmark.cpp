#include "bookmark.h"
#include "threading.h"
#include "module.h"
#include "memory.h"

std::unordered_map<duint, BOOKMARKSINFO> bookmarks;

bool BookmarkSet(duint Address, bool Manual)
{
    ASSERT_DEBUGGING("Export call");

    // Validate the incoming address
    if(!MemIsValidReadPtr(Address))
        return false;

    BOOKMARKSINFO bookmark;
    ModNameFromAddr(Address, bookmark.mod, true);
    bookmark.addr = Address - ModBaseFromAddr(Address);
    bookmark.manual = Manual;

    // Exclusive lock to insert new data
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    if(!bookmarks.insert(std::make_pair(ModHashFromAddr(Address), bookmark)).second)
    {
        EXCLUSIVE_RELEASE();
        return BookmarkDelete(Address);
    }

    return true;
}

bool BookmarkGet(duint Address)
{
    ASSERT_DEBUGGING("Export call");
    SHARED_ACQUIRE(LockBookmarks);

    return (bookmarks.count(ModHashFromAddr(Address)) > 0);
}

bool BookmarkDelete(duint Address)
{
    ASSERT_DEBUGGING("Export call");
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    return (bookmarks.erase(ModHashFromAddr(Address)) > 0);
}

void BookmarkDelRange(duint Start, duint End)
{
    ASSERT_DEBUGGING("Export call");

    // Are all bookmarks going to be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        BookmarkClear();
    }
    else
    {
        // Make sure 'Start' and 'End' reference the same module
        duint moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        // Virtual -> relative offset
        Start -= moduleBase;
        End -= moduleBase;

        EXCLUSIVE_ACQUIRE(LockBookmarks);
        for(auto itr = bookmarks.begin(); itr != bookmarks.end();)
        {
            const auto & currentBookmark = itr->second;

            // Ignore manually set entries
            if(currentBookmark.manual)
            {
                ++itr;
                continue;
            }

            // [Start, End)
            if(currentBookmark.addr >= Start && currentBookmark.addr < End)
                itr = bookmarks.erase(itr);
            else
                ++itr;
        }
    }
}

void BookmarkCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    const JSON jsonBookmarks = json_array();
    const JSON jsonAutoBookmarks = json_array();

    // Save to the JSON root
    for(auto & itr : bookmarks)
    {
        JSON currentBookmark = json_object();

        json_object_set_new(currentBookmark, "module", json_string(itr.second.mod));
        json_object_set_new(currentBookmark, "address", json_hex(itr.second.addr));

        if(itr.second.manual)
            json_array_append_new(jsonBookmarks, currentBookmark);
        else
            json_array_append_new(jsonAutoBookmarks, currentBookmark);
    }

    if(json_array_size(jsonBookmarks))
        json_object_set(Root, "bookmarks", jsonBookmarks);

    if(json_array_size(jsonAutoBookmarks))
        json_object_set(Root, "autobookmarks", jsonAutoBookmarks);

    json_decref(jsonBookmarks);
    json_decref(jsonAutoBookmarks);
}

void BookmarkCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    // Inline lambda to parse each JSON entry
    auto AddBookmarks = [](const JSON Object, bool Manual)
    {
        size_t i;
        JSON value;

        json_array_foreach(Object, i, value)
        {
            BOOKMARKSINFO bookmarkInfo;
            memset(&bookmarkInfo, 0, sizeof(BOOKMARKSINFO));

            // Load the module name
            const char* mod = json_string_value(json_object_get(value, "module"));

            if(mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(bookmarkInfo.mod, mod);

            // Load address and set auto-generated flag
            bookmarkInfo.addr = (duint)json_hex_value(json_object_get(value, "address"));
            bookmarkInfo.manual = Manual;

            const duint key = ModHashFromName(bookmarkInfo.mod) + bookmarkInfo.addr;
            bookmarks.insert(std::make_pair(key, bookmarkInfo));
        }
    };

    // Remove existing entries
    bookmarks.clear();

    const JSON jsonBookmarks = json_object_get(Root, "bookmarks");
    const JSON jsonAutoBookmarks = json_object_get(Root, "autobookmarks");

    // Load user-set bookmarks
    if(jsonBookmarks)
        AddBookmarks(jsonBookmarks, true);

    // Load auto-set bookmarks
    if(jsonAutoBookmarks)
        AddBookmarks(jsonAutoBookmarks, false);
}

bool BookmarkEnum(BOOKMARKSINFO* List, size_t* Size)
{
    // The array container must be set, or the size must be set, or both
    ASSERT_FALSE(!List && !Size);
    SHARED_ACQUIRE(LockBookmarks);

    // Return the size if set
    if(Size)
    {
        *Size = bookmarks.size() * sizeof(BOOKMARKSINFO);

        if(!List)
            return true;
    }

    // Copy struct and adjust the relative offset to a virtual address
    for(auto & itr : bookmarks)
    {
        *List = itr.second;
        List->addr += ModBaseFromName(List->mod);

        List++;
    }

    return true;
}

void BookmarkClear()
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);
    bookmarks.clear();
}

void BookmarkGetList(std::vector<BOOKMARKSINFO> & list)
{
    SHARED_ACQUIRE(LockBookmarks);
    list.clear();
    list.reserve(bookmarks.size());
    for(const auto & itr : bookmarks)
        list.push_back(itr.second);
}

bool BookmarkGetInfo(duint Address, BOOKMARKSINFO* info)
{
    SHARED_ACQUIRE(LockBookmarks);
    auto found = bookmarks.find(Address);
    if(found == bookmarks.end())
        return false;
    if(info)
        memcpy(info, &found->second, sizeof(BOOKMARKSINFO));
    return true;
}
