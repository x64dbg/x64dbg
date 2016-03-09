#include "comment.h"
#include "threading.h"
#include "module.h"
#include "memory.h"

std::unordered_map<duint, COMMENTSINFO> comments;

bool CommentSet(duint Address, const char* Text, bool Manual)
{
    ASSERT_DEBUGGING("Export call");

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
    ModNameFromAddr(Address, comment.mod, true);

    comment.manual = Manual;
    comment.addr = Address - ModBaseFromAddr(Address);

    // Key generated from module hash
    const duint key = ModHashFromAddr(Address);

    EXCLUSIVE_ACQUIRE(LockComments);

    // Insert if possible, otherwise replace
    if(!comments.insert(std::make_pair(key, comment)).second)
        comments[key] = comment;

    return true;
}

bool CommentGet(duint Address, char* Text)
{
    ASSERT_DEBUGGING("Export call");
    SHARED_ACQUIRE(LockComments);

    // Get an existing comment and copy the string buffer
    auto found = comments.find(ModHashFromAddr(Address));

    // Was it found?
    if(found == comments.end())
        return false;

    if(Text)
    {
        if(found->second.manual)   //autocomment
            strcpy_s(Text, MAX_COMMENT_SIZE, found->second.text);
        else
            sprintf_s(Text, MAX_COMMENT_SIZE, "\1%s", found->second.text);
    }

    return true;
}

bool CommentDelete(duint Address)
{
    ASSERT_DEBUGGING("Export call");
    EXCLUSIVE_ACQUIRE(LockComments);

    return (comments.erase(ModHashFromAddr(Address)) > 0);
}

void CommentDelRange(duint Start, duint End, bool Manual)
{
    ASSERT_DEBUGGING("Export call");

    // Are all comments going to be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        CommentClear();
    }
    else
    {
        // Make sure 'Start' and 'End' reference the same module
        duint moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        // Virtual -> relative offset
        Start -= moduleBase;
        End -= moduleBase;

        EXCLUSIVE_ACQUIRE(LockComments);
        for(auto itr = comments.begin(); itr != comments.end();)
        {
            const auto & currentComment = itr->second;
            // Ignore non-matching entries
            if(Manual ? !currentComment.manual : currentComment.manual)
            {
                ++itr;
                continue;
            }

            // [Start, End)
            if(currentComment.addr >= Start && currentComment.addr < End)
                itr = comments.erase(itr);
            else
                ++itr;
        }
    }
}

void CommentCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockComments);

    const JSON jsonComments = json_array();
    const JSON jsonAutoComments = json_array();

    // Build the JSON array
    for(auto & itr : comments)
    {
        JSON currentComment = json_object();

        json_object_set_new(currentComment, "module", json_string(itr.second.mod));
        json_object_set_new(currentComment, "address", json_hex(itr.second.addr));
        json_object_set_new(currentComment, "text", json_string(itr.second.text));

        if(itr.second.manual)
            json_array_append_new(jsonComments, currentComment);
        else
            json_array_append_new(jsonAutoComments, currentComment);
    }

    // Save to the JSON root
    if(json_array_size(jsonComments))
        json_object_set(Root, "comments", jsonComments);

    if(json_array_size(jsonAutoComments))
        json_object_set(Root, "autocomments", jsonAutoComments);

    json_decref(jsonComments);
    json_decref(jsonAutoComments);
}

void CommentCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockComments);

    // Inline lambda to parse each JSON entry
    auto AddComments = [](const JSON Object, bool Manual)
    {
        size_t i;
        JSON value;

        json_array_foreach(Object, i, value)
        {
            COMMENTSINFO commentInfo;
            memset(&commentInfo, 0, sizeof(COMMENTSINFO));

            // Module
            const char* mod = json_string_value(json_object_get(value, "module"));

            if(mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(commentInfo.mod, mod);

            // Address/Manual
            commentInfo.addr = (duint)json_hex_value(json_object_get(value, "address"));
            commentInfo.manual = Manual;

            // String value
            const char* text = json_string_value(json_object_get(value, "text"));

            if(text)
                strcpy_s(commentInfo.text, text);
            else
            {
                // Skip blank comments
                continue;
            }

            const duint key = ModHashFromName(commentInfo.mod) + commentInfo.addr;
            comments.insert(std::make_pair(key, commentInfo));
        }
    };

    // Remove existing entries
    comments.clear();

    const JSON jsonComments = json_object_get(Root, "comments");
    const JSON jsonAutoComments = json_object_get(Root, "autocomments");

    // Load user-set comments
    if(jsonComments)
        AddComments(jsonComments, true);

    // Load auto-set comments
    if(jsonAutoComments)
        AddComments(jsonAutoComments, false);
}

bool CommentEnum(COMMENTSINFO* List, size_t* Size)
{
    ASSERT_DEBUGGING("Command function call");

    // At least 1 parameter must be supplied
    ASSERT_FALSE(!List && !Size);
    SHARED_ACQUIRE(LockComments);

    // Check if the user requested size only
    if(Size)
    {
        *Size = comments.size() * sizeof(COMMENTSINFO);

        if(!List)
            return true;
    }

    // Populate the returned array
    for(auto & itr : comments)
    {
        *List = itr.second;
        List->addr += ModBaseFromName(List->mod);

        List++;
    }

    return true;
}

void CommentClear()
{
    EXCLUSIVE_ACQUIRE(LockComments);
    comments.clear();
}

void CommentGetList(std::vector<COMMENTSINFO> & list)
{
    SHARED_ACQUIRE(LockComments);
    list.clear();
    list.reserve(comments.size());
    for(const auto & itr : comments)
        list.push_back(itr.second);
}

bool CommentGetInfo(duint Address, COMMENTSINFO* info)
{
    SHARED_ACQUIRE(LockComments);

    // Get an existing comment and copy the string buffer
    auto found = comments.find(ModHashFromAddr(Address));

    // Was it found?
    if(found == comments.end())
        return false;

    if(info)
        memcpy(info, &found->second, sizeof(COMMENTSINFO));

    return true;
}
