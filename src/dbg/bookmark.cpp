#include "bookmark.h"
#include "database_cb_batcher.h"

struct BookmarkSerializer : AddrInfoSerializer<BOOKMARKSINFO>
{
};

struct Bookmarks : AddrInfoHashMap<LockBookmarks, BOOKMARKSINFO, BookmarkSerializer>
{
    const char* jsonKey() const override
    {
        return "bookmarks";
    }

protected:
    bool populateDbOperation(DbOperation & op, const BOOKMARKSINFO & value) const override
    {
        op.itemType = DbItemTypeBookmark;
        op.manual = value.manual;
        op.modhash = value.modhash;
        op.address = value.addr;
        return true;
    }
};

static Bookmarks bookmarks;

bool BookmarkSet(duint Address, bool Manual)
{
    BOOKMARKSINFO bookmark;
    if(!bookmarks.PrepareValue(bookmark, Address, Manual))
        return false;
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
    DbCallbackBatcher batcher;
    bookmarks.DeleteRange(Start, End, Manual);
}

void BookmarkCacheSave(JSON Root)
{
    bookmarks.CacheSave(Root);
}

void BookmarkCacheLoad(JSON Root)
{
    DbCallbackBatcher batcher(true);
    bookmarks.CacheLoad(Root);
    bookmarks.CacheLoad(Root, "auto"); //legacy support
}

bool BookmarkEnum(BOOKMARKSINFO* List, size_t* Size)
{
    return bookmarks.Enum(List, Size);
}

void BookmarkClear(bool Terminating)
{
    DbCallbackBatcher batcher;
    bookmarks.Clear(Terminating);
}

void BookmarkGetList(std::vector<BOOKMARKSINFO> & list)
{
    bookmarks.GetList(list);
}

bool BookmarkGetInfo(duint Address, BOOKMARKSINFO* info)
{
    return bookmarks.GetInfo(Bookmarks::VaKey(Address), info);
}
