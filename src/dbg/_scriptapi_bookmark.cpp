#include "_scriptapi_bookmark.h"
#include "_scriptapi_module.h"
#include "bookmark.h"

SCRIPT_EXPORT bool Script::Bookmark::Set(duint addr, bool manual)
{
    return BookmarkSet(addr, manual);
}

SCRIPT_EXPORT bool Script::Bookmark::Set(const BookmarkInfo* info)
{
    if(!info)
        return false;
    auto base = Module::BaseFromName(info->mod);
    if(!base)
        return false;
    return Set(base + info->rva, info->manual);
}

SCRIPT_EXPORT bool Script::Bookmark::Get(duint addr)
{
    return BookmarkGet(addr);
}

SCRIPT_EXPORT bool Script::Bookmark::GetInfo(duint addr, BookmarkInfo* info)
{
    BOOKMARKSINFO comment;
    if(!BookmarkGetInfo(addr, &comment))
        return false;
    if(info)
    {
        strcpy_s(info->mod, comment.mod().c_str());
        info->rva = comment.addr;
        info->manual = comment.manual;
    }
    return true;
}

SCRIPT_EXPORT bool Script::Bookmark::Delete(duint addr)
{
    return BookmarkDelete(addr);
}

SCRIPT_EXPORT void Script::Bookmark::DeleteRange(duint start, duint end)
{
    BookmarkDelRange(start, end, false);
}

SCRIPT_EXPORT void Script::Bookmark::Clear()
{
    BookmarkClear();
}

SCRIPT_EXPORT bool Script::Bookmark::GetList(ListOf(BookmarkInfo) list)
{
    std::vector<BOOKMARKSINFO> bookmarkList;
    BookmarkGetList(bookmarkList);
    std::vector<BookmarkInfo> bookmarkScriptList;
    bookmarkScriptList.reserve(bookmarkList.size());
    for(const auto & bookmark : bookmarkList)
    {
        BookmarkInfo scriptComment;
        strcpy_s(scriptComment.mod, bookmark.mod().c_str());
        scriptComment.rva = bookmark.addr;
        scriptComment.manual = bookmark.manual;
        bookmarkScriptList.push_back(scriptComment);
    }
    return BridgeList<BookmarkInfo>::CopyData(list, bookmarkScriptList);
}