#include "bookmark.h"
#include "serializablemap.h"

struct BookmarkSerializer : AddrInfoSerializer<BOOKMARKSINFO>
{
};

struct Bookmarks : AddrInfoHashMap<LockBookmarks, BOOKMARKSINFO, BookmarkSerializer>
{
    const char* jsonKey() const override
    {
        return "bookmarks";
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
    bookmarks.DeleteRange(Start, End, Manual);
}

void BookmarkCacheSave(rapidjson::Document & Root)
{
    bookmarks.CacheSave(Root);
}

void BookmarkCacheLoad(rapidjson::Document & Root)
{
    bookmarks.CacheLoad(Root);
    bookmarks.CacheLoad(Root, "auto"); //legacy support
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
