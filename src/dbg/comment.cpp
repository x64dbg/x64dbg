#include "comment.h"
#include "database_cb_batcher.h"

struct CommentSerializer : AddrInfoSerializer<COMMENTSINFO>
{
    bool Save(const COMMENTSINFO & value) override
    {
        AddrInfoSerializer::Save(value);
        setString("text", value.text);
        return true;
    }

    bool Load(COMMENTSINFO & value) override
    {
        return AddrInfoSerializer::Load(value) &&
               getString("text", value.text);
    }
};

struct Comments : AddrInfoHashMap<LockComments, COMMENTSINFO, CommentSerializer>
{
    const char* jsonKey() const override
    {
        return "comments";
    }

protected:
    bool populateDbOperation(DbOperation & op, const COMMENTSINFO & value) const override
    {
        op.itemType = DbItemTypeComment;
        op.manual = value.manual;
        op.modhash = value.modhash;
        op.address = value.addr;
        op.comment.text = value.text.c_str();
        return true;
    }
};

static Comments comments;

bool CommentSet(duint Address, const char* Text, bool Manual)
{
    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_COMMENT_SIZE - 1)
        return false;
    // Delete the comment if no text was supplied
    if(Text[0] == '\0')
    {
        CommentDelete(Address);
        return true;
    }
    // Fill in the structure + add to database
    COMMENTSINFO comment;
    if(!comments.PrepareValue(comment, Address, Manual))
        return false;
    comment.text = Text;
    return comments.Add(comment);
}

bool CommentGet(duint Address, char* Text)
{
    COMMENTSINFO comment;
    if(!comments.Get(Comments::VaKey(Address), comment))
        return false;
    if(comment.manual)
        strncpy_s(Text, MAX_COMMENT_SIZE, comment.text.c_str(), _TRUNCATE);
    else
        _snprintf_s(Text, MAX_COMMENT_SIZE, _TRUNCATE, "\1%s", comment.text.c_str());
    return true;
}

bool CommentDelete(duint Address)
{
    return comments.Delete(Comments::VaKey(Address));
}

void CommentDelRange(duint Start, duint End, bool Manual)
{
    DbCallbackBatcher batcher;
    comments.DeleteRange(Start, End, Manual);
}

void CommentCacheSave(JSON Root)
{
    comments.CacheSave(Root);
}

void CommentCacheLoad(JSON Root)
{
    DbCallbackBatcher batcher(true);
    comments.CacheLoad(Root);
    comments.CacheLoad(Root, "auto"); //legacy support
}

bool CommentEnum(COMMENTSINFO* List, size_t* Size)
{
    return comments.Enum(List, Size);
}

void CommentClear(bool Terminating)
{
    DbCallbackBatcher batcher;
    comments.Clear(Terminating);
}

void CommentGetList(std::vector<COMMENTSINFO> & list)
{
    comments.GetList(list);
}

bool CommentGetInfo(duint Address, COMMENTSINFO* info)
{
    return comments.GetInfo(Comments::VaKey(Address), info);
}
