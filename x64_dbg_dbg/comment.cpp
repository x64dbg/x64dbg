#include "comment.h"
#include "threading.h"
#include "module.h"
#include "debugger.h"
#include "memory.h"

typedef std::map<uint, COMMENTSINFO> CommentsInfo;

static CommentsInfo comments;

bool commentset(uint addr, const char* text, bool manual)
{
    if(!DbgIsDebugging() or !memisvalidreadptr(fdProcessInfo->hProcess, addr) or !text or text[0] == '\1' or strlen(text) >= MAX_COMMENT_SIZE - 1)
        return false;
    if(!*text) //NOTE: delete when there is no text
    {
        commentdel(addr);
        return true;
    }
    COMMENTSINFO comment;
    comment.manual = manual;
    strcpy_s(comment.text, text);
    ModNameFromAddr(addr, comment.mod, true);
    comment.addr = addr - ModBaseFromAddr(addr);
    const uint key = ModHashFromAddr(addr);
    CriticalSectionLocker locker(LockComments);
    if(!comments.insert(std::make_pair(key, comment)).second) //key already present
        comments[key] = comment;
    return true;
}

bool commentget(uint addr, char* text)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockComments);
    const CommentsInfo::iterator found = comments.find(ModHashFromAddr(addr));
    if(found == comments.end()) //not found
        return false;
    strcpy_s(text, MAX_COMMENT_SIZE, found->second.text);
    return true;
}

bool commentdel(uint addr)
{
    if(!DbgIsDebugging())
        return false;
    CriticalSectionLocker locker(LockComments);
    return (comments.erase(ModHashFromAddr(addr)) == 1);
}

void commentdelrange(uint start, uint end)
{
    if(!DbgIsDebugging())
        return;
    bool bDelAll = (start == 0 && end == ~0); //0x00000000-0xFFFFFFFF
    uint modbase = ModBaseFromAddr(start);
    if(modbase != ModBaseFromAddr(end))
        return;
    start -= modbase;
    end -= modbase;
    CriticalSectionLocker locker(LockComments);
    CommentsInfo::iterator i = comments.begin();
    while(i != comments.end())
    {
        if(i->second.manual) //ignore manual
        {
            i++;
            continue;
        }
        if(bDelAll || (i->second.addr >= start && i->second.addr < end))
            comments.erase(i++);
        else
            i++;
    }
}

void commentcachesave(JSON root)
{
    CriticalSectionLocker locker(LockComments);
    const JSON jsoncomments = json_array();
    const JSON jsonautocomments = json_array();
    for(CommentsInfo::iterator i = comments.begin(); i != comments.end(); ++i)
    {
        const COMMENTSINFO curComment = i->second;
        JSON curjsoncomment = json_object();
        json_object_set_new(curjsoncomment, "module", json_string(curComment.mod));
        json_object_set_new(curjsoncomment, "address", json_hex(curComment.addr));
        json_object_set_new(curjsoncomment, "text", json_string(curComment.text));
        if(curComment.manual)
            json_array_append_new(jsoncomments, curjsoncomment);
        else
            json_array_append_new(jsonautocomments, curjsoncomment);
    }
    if(json_array_size(jsoncomments))
        json_object_set(root, "comments", jsoncomments);
    json_decref(jsoncomments);
    if(json_array_size(jsonautocomments))
        json_object_set(root, "autocomments", jsonautocomments);
    json_decref(jsonautocomments);
}

void commentcacheload(JSON root)
{
    CriticalSectionLocker locker(LockComments);
    comments.clear();
    const JSON jsoncomments = json_object_get(root, "comments");
    if(jsoncomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsoncomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curComment.mod, mod);
            else
                *curComment.mod = '\0';
            curComment.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual = true;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curComment.text, text);
            else
                continue; //skip
            const uint key = ModHashFromName(curComment.mod) + curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
    JSON jsonautocomments = json_object_get(root, "autocomments");
    if(jsonautocomments)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautocomments, i, value)
        {
            COMMENTSINFO curComment;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curComment.mod, mod);
            else
                *curComment.mod = '\0';
            curComment.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curComment.manual = false;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curComment.text, text);
            else
                continue; //skip
            const uint key = ModHashFromName(curComment.mod) + curComment.addr;
            comments.insert(std::make_pair(key, curComment));
        }
    }
}

bool commentenum(COMMENTSINFO* commentlist, size_t* cbsize)
{
    if(!DbgIsDebugging())
        return false;
    if(!commentlist && !cbsize)
        return false;
    CriticalSectionLocker locker(LockComments);
    if(!commentlist && cbsize)
    {
        *cbsize = comments.size() * sizeof(COMMENTSINFO);
        return true;
    }
    int j = 0;
    for(CommentsInfo::iterator i = comments.begin(); i != comments.end(); ++i, j++)
    {
        commentlist[j] = i->second;
        commentlist[j].addr += ModBaseFromName(commentlist[j].mod);
    }
    return true;
}

void commentclear()
{
    CriticalSectionLocker locker(LockComments);
    CommentsInfo().swap(comments);
}