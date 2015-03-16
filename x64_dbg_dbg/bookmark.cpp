#include "bookmark.h"
#include "threading.h"
#include "module.h"
#include "debugger.h"
#include "memory.h"

typedef std::map<uint, BOOKMARKSINFO> BookmarksInfo;

static BookmarksInfo bookmarks;

bool bookmarkset(uint addr, bool manual)
{
    // CHECK: Export call
    if(!DbgIsDebugging())
        return false;

    // Validate the incoming address
    if(!memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;

    BOOKMARKSINFO bookmark;
    ModNameFromAddr(addr, bookmark.mod, true);
    bookmark.addr   = addr - ModBaseFromAddr(addr);
    bookmark.manual = manual;

    // Exclusive lock to insert new data
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    if(!bookmarks.insert(std::make_pair(ModHashFromAddr(addr), bookmark)).second)
        return bookmarkdel(addr);

    return true;
}

bool bookmarkget(uint addr)
{
    SHARED_ACQUIRE(LockBookmarks);
    return (bookmarks.count(ModHashFromAddr(addr)) > 0);
}

bool bookmarkdel(uint addr)
{
    // CHECK: Export call
    if(!DbgIsDebugging())
        return false;

    EXCLUSIVE_ACQUIRE(LockBookmarks);
    return (bookmarks.erase(ModHashFromAddr(addr)) > 0);
}

void bookmarkdelrange(uint start, uint end)
{
    // CHECK: Export call
    if(!DbgIsDebugging())
        return;

    // Are all bookmarks going to be deleted?
    if(start == 0x00000000 && end == 0xFFFFFFFF)
    {
        EXCLUSIVE_ACQUIRE(LockBookmarks);
        bookmarks.clear();
    }
    else
    {
        // Make sure 'start' and 'end' reference the same module
        uint modbase = ModBaseFromAddr(start);

        if(modbase != ModBaseFromAddr(end))
            return;

        start   -= modbase;
        end     -= modbase;

        EXCLUSIVE_ACQUIRE(LockBookmarks);
        for(auto itr = bookmarks.begin(); itr != bookmarks.end();)
        {
            // Ignore manually set entries
            if(itr->second.manual)
            {
                itr++;
                continue;
            }

            if(itr->second.addr >= start && itr->second.addr < end)
                bookmarks.erase(itr);

            itr++;
        }
    }
}

void bookmarkcachesave(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    const JSON jsonbookmarks        = json_array();
    const JSON jsonautobookmarks    = json_array();

    // Save to the JSON root
    for(auto itr = bookmarks.begin(); itr != bookmarks.end(); itr++)
    {
        JSON curjsonbookmark = json_object();

        json_object_set_new(curjsonbookmark, "module", json_string(itr->second.mod));
        json_object_set_new(curjsonbookmark, "address", json_hex(itr->second.addr));

        if(itr->second.manual)
            json_array_append_new(jsonbookmarks, curjsonbookmark);
        else
            json_array_append_new(jsonautobookmarks, curjsonbookmark);
    }

    if(json_array_size(jsonbookmarks))
        json_object_set(root, "bookmarks", jsonbookmarks);

    if(json_array_size(jsonautobookmarks))
        json_object_set(root, "autobookmarks", jsonautobookmarks);

    json_decref(jsonbookmarks);
    json_decref(jsonautobookmarks);
}

void bookmarkcacheload(JSON root)
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);

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

            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(bookmarkInfo.mod, mod);

            // Load address and set auto-generated flag
            bookmarkInfo.addr   = (uint)json_hex_value(json_object_get(value, "address"));
            bookmarkInfo.manual = Manual;

            const uint key = ModHashFromName(bookmarkInfo.mod) + bookmarkInfo.addr;
            bookmarks.insert(std::make_pair(key, bookmarkInfo));
        }
    };

    // Remove existing entries
    bookmarks.clear();

    const JSON jsonbookmarks        = json_object_get(root, "bookmarks");
    const JSON jsonautobookmarks    = json_object_get(root, "autobookmarks");

    // Load user-set bookmarks
    if(jsonbookmarks)
        AddBookmarks(jsonbookmarks, true);

    // Load auto-set bookmarks
    if(jsonautobookmarks)
        AddBookmarks(jsonbookmarks, false);
}

bool bookmarkenum(BOOKMARKSINFO* bookmarklist, size_t* cbsize)
{
    // The array container must be set, or the size must be set, or both
    if(!bookmarklist && !cbsize)
        return false;

    SHARED_ACQUIRE(LockBookmarks);

    // Return the size if set
    if(cbsize)
    {
        *cbsize = bookmarks.size() * sizeof(BOOKMARKSINFO);
        return true;
    }

    // TODO: only ModBaseFromName seems wrong
    for(auto itr = bookmarks.begin(); itr != bookmarks.end(); itr++, bookmarklist++)
    {
        *bookmarklist       = itr->second;
        bookmarklist->addr  += ModBaseFromName(bookmarklist->mod);
    }

    return true;
}

void bookmarkclear()
{
    EXCLUSIVE_ACQUIRE(LockBookmarks);

    bookmarks.clear();
}