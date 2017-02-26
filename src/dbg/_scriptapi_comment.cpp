#include "_scriptapi_comment.h"
#include "_scriptapi_module.h"
#include "comment.h"

SCRIPT_EXPORT bool Script::Comment::Set(duint addr, const char* text, bool manual)
{
    return CommentSet(addr, text, manual);
}

SCRIPT_EXPORT bool Script::Comment::Set(const CommentInfo* info)
{
    if(!info)
        return false;
    auto base = Module::BaseFromName(info->mod);
    if(!base)
        return false;
    return Set(base + info->rva, info->text, info->manual);
}

SCRIPT_EXPORT bool Script::Comment::Get(duint addr, char* text)
{
    return CommentGet(addr, text);
}

SCRIPT_EXPORT bool Script::Comment::GetInfo(duint addr, CommentInfo* info)
{
    COMMENTSINFO comment;
    if(!CommentGetInfo(addr, &comment))
        return false;
    if(info)
    {
        strcpy_s(info->mod, comment.mod().c_str());
        info->rva = comment.addr;
        strcpy_s(info->text, comment.text.c_str());
        info->manual = comment.manual;
    }
    return true;
}

SCRIPT_EXPORT bool Script::Comment::Delete(duint addr)
{
    return CommentDelete(addr);
}

SCRIPT_EXPORT void Script::Comment::DeleteRange(duint start, duint end)
{
    CommentDelRange(start, end, false);
}

SCRIPT_EXPORT void Script::Comment::Clear()
{
    CommentClear();
}

SCRIPT_EXPORT bool Script::Comment::GetList(ListOf(CommentInfo) list)
{
    std::vector<COMMENTSINFO> commentList;
    CommentGetList(commentList);
    std::vector<CommentInfo> commentScriptList;
    commentScriptList.reserve(commentList.size());
    for(const auto & comment : commentList)
    {
        CommentInfo scriptComment;
        strcpy_s(scriptComment.mod, comment.mod().c_str());
        scriptComment.rva = comment.addr;
        strcpy_s(scriptComment.text, comment.text.c_str());
        scriptComment.manual = comment.manual;
        commentScriptList.push_back(scriptComment);
    }
    return BridgeList<CommentInfo>::CopyData(list, commentScriptList);
}