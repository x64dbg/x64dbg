#include "bookmark.h"
#include "module.h"
#include "memory.h"

struct BookmarkSerializer : JSONWrapper<BOOKMARKSINFO>
{
    bool Save(const BOOKMARKSINFO & value) override
    {
        setString("module", value.mod);
        setHex("address", value.addr);
        setBool("manual", value.manual);
        return true;
    }

    bool Load(BOOKMARKSINFO & value) override
    {
        value.manual = true;
        getBool("manual", value.manual); //legacy support
        return getString("module", value.mod) &&
               getHex("address", value.addr);
    }
};

struct Bookmarks : SerializableModuleHashMap<LockBookmarks, BOOKMARKSINFO, BookmarkSerializer>
{
    void AdjustValue(BOOKMARKSINFO & value) const override
    {
        value.addr += ModBaseFromName(value.mod);
    }

protected:
    const char* jsonKey() const override
    {
        return "bookmarks";
    }

    duint makeKey(const BOOKMARKSINFO & value) const override
    {
        return ModHashFromName(value.mod) + value.addr;
    }
};

static Bookmarks bookmarks;

bool BookmarkSet(duint Address, bool Manual)
{
    // Validate the incoming address
    if(!MemIsValidReadPtr(Address))
        return false;

    BOOKMARKSINFO bookmark;
    if(!ModNameFromAddr(Address, bookmark.mod, true))
        *bookmark.mod = '\0';
    bookmark.addr = Address - ModBaseFromAddr(Address);
    bookmark.manual = Manual;

    auto key = Bookmarks::VaKey(Address);
    if(bookmarks.Contains(key))
        return bookmarks.Delete(key);
    return bookmarks.Add(bookmark);
}

bool BookmarkGet(duint Address)
{
    return bookmarks.Contains(Bookmarks::VaKey(Address));
}

bool BookmarkDelete(duint Address)
{
    return bookmarks.Delete(Bookmarks::VaKey(Address));
}

void BookmarkDelRange(duint Start, duint End, bool Manual)
{
    bookmarks.DeleteRange(Start, End, [Manual](duint start, duint end, const BOOKMARKSINFO & value)
    {
        if(Manual ? !value.manual : value.manual)  //ignore non-matching entries
            return false;
        return value.addr >= start && value.addr < end;
    });
}

void BookmarkCacheSave(JSON Root)
{
    bookmarks.CacheSave(Root);
}

void BookmarkCacheLoad(JSON Root)
{
    bookmarks.CacheLoad(Root);
    bookmarks.CacheLoad(Root, false, "auto"); //legacy support
}

bool BookmarkEnum(BOOKMARKSINFO* List, size_t* Size)
{
    return bookmarks.Enum(List, Size);
}

void BookmarkClear()
{
    bookmarks.Clear();
}

void BookmarkGetList(std::vector<BOOKMARKSINFO> & list)
{
    bookmarks.GetList(list);
}

bool BookmarkGetInfo(duint Address, BOOKMARKSINFO* info)
{
    return bookmarks.GetInfo(Bookmarks::VaKey(Address), info);
}
