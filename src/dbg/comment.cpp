#include "comment.h"
#include "threading.h"
#include "module.h"
#include "memory.h"

struct CommentSerializer : JSONWrapper<COMMENTSINFO>
{
    bool Save(const COMMENTSINFO & value) override
    {
        setString("module", value.mod);
        setHex("address", value.addr);
        setString("text", value.text);
        setBool("manual", value.manual);
        return true;
    }

    bool Load(COMMENTSINFO & value) override
    {
        value.manual = true;
        getBool("manual", value.manual); //legacy support
        return getString("module", value.mod) &&
               getHex("address", value.addr) &&
               getString("text", value.text);
    }
};

struct Comments : SerializableModuleHashMap<LockComments, COMMENTSINFO, CommentSerializer>
{
    void AdjustValue(COMMENTSINFO & value) const override
    {
        value.addr += ModBaseFromName(value.mod);
    }

protected:
    const char* jsonKey() const override
    {
        return "comments";
    }

    duint makeKey(const COMMENTSINFO & value) const override
    {
        return ModHashFromName(value.mod) + value.addr;
    }
};

static Comments comments;

bool CommentSet(duint Address, const char* Text, bool Manual)
{
    // A valid memory address must be supplied
    if(!MemIsValidReadPtr(Address))
        return false;

    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_COMMENT_SIZE - 1)
        return false;

    // Delete the comment if no text was supplied
    if(Text[0] == '\0')
    {
        CommentDelete(Address);
        return true;
    }

    // Fill out the structure
    COMMENTSINFO comment;
    strcpy_s(comment.text, Text);
    if(!ModNameFromAddr(Address, comment.mod, true))
        *comment.mod = '\0';

    comment.manual = Manual;
    comment.addr = Address - ModBaseFromAddr(Address);

    return comments.Add(comment);
}

bool CommentGet(duint Address, char* Text)
{
    COMMENTSINFO comment;
    if(!comments.Get(Comments::VaKey(Address), comment))
        return false;
    if(comment.manual)
        strcpy_s(Text, MAX_COMMENT_SIZE, comment.text);
    else
        sprintf_s(Text, MAX_COMMENT_SIZE, "\1%s", comment.text);
    return true;
}

bool CommentDelete(duint Address)
{
    return comments.Delete(Comments::VaKey(Address));
}

void CommentDelRange(duint Start, duint End, bool Manual)
{
    comments.DeleteRange(Start, End, [Manual](duint start, duint end, const COMMENTSINFO & value)
    {
        if(Manual ? !value.manual : value.manual)  //ignore non-matching entries
            return false;
        return value.addr >= start && value.addr < end;
    });
}

void CommentCacheSave(JSON Root)
{
    comments.CacheSave(Root);
}

void CommentCacheLoad(JSON Root)
{
    comments.CacheLoad(Root);
    comments.CacheLoad(Root, false, "auto"); //legacy support
}

bool CommentEnum(COMMENTSINFO* List, size_t* Size)
{
    return comments.Enum(List, Size);
}

void CommentClear()
{
    comments.Clear();
}

void CommentGetList(std::vector<COMMENTSINFO> & list)
{
    comments.GetList(list);
}

bool CommentGetInfo(duint Address, COMMENTSINFO* info)
{
    return comments.GetInfo(Comments::VaKey(Address), info);
}
