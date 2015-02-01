#include "bookmark.h"
#include "threading.h"
#include "module.h"
#include "debugger.h"
#include "memory.h"

typedef std::map<uint, BOOKMARKSINFO> BookmarksInfo;

static BookmarksInfo bookmarks;

bool bookmarkset(uint addr, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr))
        return false;
    BOOKMARKSINFO bookmark;
    modnamefromaddr(addr, bookmark.mod, true);
    bookmark.addr = addr - modbasefromaddr(addr);
    bookmark.manual = manual;
    CriticalSectionLocker locker(LockBookmarks);
    if(!bookmarks.insert(std::make_pair(modhashfromva(addr), bookmark)).second)
        return bookmarkdel(addr);
    return true;
}

bool bookmarkget(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    if(bookmarks.count(modhashfromva(addr)))
        return true;
    return false;
}

bool bookmarkdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    return (bookmarks.erase(modhashfromva(addr)) > 0);
}

void bookmarkdelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = modbasefromaddr(start);
    if(modbase != modbasefromaddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockBookmarks);
    BookmarksInfo::iterator i = bookmarks.begin();
    while(i != bookmarks.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            bookmarks.erase(i++);
        else
            i++;
    }
}

void bookmarkcachesave(JSON root)
{
    CriticalSectionLocker locker(LockBookmarks);
    const JSON jsonbookmarks = json_array();
    const JSON jsonautobookmarks = json_array();
    for(BookmarksInfo::iterator i = bookmarks.begin(); i != bookmarks.end(); ++i)
    {
        const BOOKMARKSINFO curBookmark = i->second;
        JSON curjsonbookmark = json_object();
        json_object_set_new(curjsonbookmark, "module", json_string(curBookmark.mod));
        json_object_set_new(curjsonbookmark, "address", json_hex(curBookmark.addr));
        if(curBookmark.manual)
            json_array_append_new(jsonbookmarks, curjsonbookmark);
        else
            json_array_append_new(jsonautobookmarks, curjsonbookmark);
    }
    if(json_array_size(jsonbookmarks))
        json_object_set(root, "bookmarks", jsonbookmarks);
    json_decref(jsonbookmarks);
    if(json_array_size(jsonautobookmarks))
        json_object_set(root, "autobookmarks", jsonautobookmarks);
    json_decref(jsonautobookmarks);
}

void bookmarkcacheload(JSON root)
{
    CriticalSectionLocker locker(LockBookmarks);
    bookmarks.clear();
    const JSON jsonbookmarks = json_object_get(root, "bookmarks");
    if(jsonbookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonbookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curBookmark.mod, mod);
            else
                *curBookmark.mod = '\0';
            curBookmark.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual = true;
            const uint key = modhashfromname(curBookmark.mod) + curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
    JSON jsonautobookmarks = json_object_get(root, "autobookmarks");
    if(jsonautobookmarks)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautobookmarks, i, value)
        {
            BOOKMARKSINFO curBookmark;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curBookmark.mod, mod);
            else
                *curBookmark.mod = '\0';
            curBookmark.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curBookmark.manual = false;
            const uint key = modhashfromname(curBookmark.mod) + curBookmark.addr;
            bookmarks.insert(std::make_pair(key, curBookmark));
        }
    }
}

bool bookmarkenum(BOOKMARKSINFO* bookmarklist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!bookmarklist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockBookmarks);
    if(!bookmarklist && cbsize)
    {
        *cbsize = bookmarks.size() * sizeof(BOOKMARKSINFO);
        return true;
    }
    int j = 0;
    for(BookmarksInfo::iterator i = bookmarks.begin(); i != bookmarks.end(); ++i, j++)
    {
        bookmarklist[j] = i->second;
        bookmarklist[j].addr += modbasefromname(bookmarklist[j].mod);
    }
    return true;
}

void bookmarkclear()
{
    CriticalSectionLocker locker(LockBookmarks);
    BookmarksInfo().swap(bookmarks);
}