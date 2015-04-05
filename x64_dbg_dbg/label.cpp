#include "label.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "debugger.h"

std::unordered_map<uint, LABELSINFO> labels;

bool LabelSet(uint Address, const char* Text, bool Manual)
{
    // CHECK: Exported/Command function
    if(!DbgIsDebugging())
        return false;

    // A valid memory address must be supplied
    if(!MemIsValidReadPtr(Address))
        return false;

    // Make sure the string is supplied, within bounds, and not a special delimiter
    if(!Text || Text[0] == '\1' || strlen(Text) >= MAX_LABEL_SIZE - 1)
        return false;

    // Labels cannot be "address" of actual variables
    if(strstr(Text, "&"))
        return false;

    // Delete the label if no text was supplied
    if(Text[0] == '\0')
        return LabelDelete(Address);

    // Fill out the structure data
    LABELSINFO labelInfo;
    labelInfo.manual    = Manual;
    labelInfo.addr      = Address - ModBaseFromAddr(Address);
    strcpy_s(labelInfo.text, Text);
    ModNameFromAddr(Address, labelInfo.mod, true);

    EXCLUSIVE_ACQUIRE(LockLabels);

    // Insert label by key
    const uint key = ModHashFromAddr(Address);

    if(!labels.insert(std::make_pair(ModHashFromAddr(key), labelInfo)).second)
        labels[key] = labelInfo;

    return true;
}

bool LabelFromString(const char* Text, uint* Address)
{
    // CHECK: Future? (Not used)
    if(!DbgIsDebugging())
        return false;

    SHARED_ACQUIRE(LockLabels);

    for(auto & itr : labels)
    {
        // Check if the actual label name matches
        if(strcmp(itr.second.text, Text))
            continue;

        if(Address)
            *Address = itr.second.addr + ModBaseFromName(itr.second.mod);

        // Set status to indicate if label was ever found
        return true;
    }

    return false;
}

bool LabelGet(uint Address, char* Text)
{
    // CHECK: Export function
    if(!DbgIsDebugging())
        return false;

    SHARED_ACQUIRE(LockLabels);

    // Was the label at this address exist?
    auto found = labels.find(ModHashFromAddr(Address));

    if(found == labels.end())
        return false;

    // Copy to user buffer
    if(Text)
        strcpy_s(Text, MAX_LABEL_SIZE, found->second.text);

    return true;
}

bool LabelDelete(uint Address)
{
    // CHECK: Export function
    if(!DbgIsDebugging())
        return false;

    EXCLUSIVE_ACQUIRE(LockLabels);
    return (labels.erase(ModHashFromAddr(Address)) > 0);
}

void LabelDelRange(uint Start, uint End)
{
    // CHECK: Export function
    if(!DbgIsDebugging())
        return;

    // Are all comments going to be deleted?
    // 0x00000000 - 0xFFFFFFFF
    if(Start == 0 && End == ~0)
    {
        EXCLUSIVE_ACQUIRE(LockLabels);
        labels.clear();
    }
    else
    {
        // Make sure 'Start' and 'End' reference the same module
        uint moduleBase = ModBaseFromAddr(Start);

        if(moduleBase != ModBaseFromAddr(End))
            return;

        EXCLUSIVE_ACQUIRE(LockLabels);
        for(auto itr = labels.begin(); itr != labels.end();)
        {
            // Ignore manually set entries
            if(itr->second.manual)
            {
                itr++;
                continue;
            }

            // [Start, End)
            if(itr->second.addr >= Start && itr->second.addr < End)
                itr = labels.erase(itr);
            else
                itr++;
        }
    }
}

void LabelCacheSave(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockLabels);

    // Create the sub-root structures in memory
    const JSON jsonLabels       = json_array();
    const JSON jsonAutoLabels   = json_array();

    // Iterator each label
    for(auto & itr : labels)
    {
        JSON jsonLabel = json_object();
        json_object_set_new(jsonLabel, "module", json_string(itr.second.mod));
        json_object_set_new(jsonLabel, "address", json_hex(itr.second.addr));
        json_object_set_new(jsonLabel, "text", json_string(itr.second.text));

        // Was the label manually added?
        if(itr.second.manual)
            json_array_append_new(jsonLabels, jsonLabel);
        else
            json_array_append_new(jsonAutoLabels, jsonLabel);
    }

    // Apply the object to the global root
    if(json_array_size(jsonLabels))
        json_object_set(Root, "labels", jsonLabels);

    if(json_array_size(jsonAutoLabels))
        json_object_set(Root, "autolabels", jsonAutoLabels);

    json_decref(jsonLabels);
    json_decref(jsonAutoLabels);
}

void LabelCacheLoad(JSON Root)
{
    EXCLUSIVE_ACQUIRE(LockLabels);

    // Inline lambda to parse each JSON entry
    auto AddLabels = [](const JSON Object, bool Manual)
    {
        size_t i;
        JSON value;

        json_array_foreach(Object, i, value)
        {
            LABELSINFO labelInfo;
            memset(&labelInfo, 0, sizeof(LABELSINFO));

            // Module
            const char* mod = json_string_value(json_object_get(value, "module"));

            if(mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(labelInfo.mod, mod);
            else
                labelInfo.mod[0] = '\0';

            // Address/Manual
            labelInfo.addr      = (uint)json_hex_value(json_object_get(value, "address"));
            labelInfo.manual    = Manual;

            // Text string
            const char* text = json_string_value(json_object_get(value, "text"));

            if(text)
                strcpy_s(labelInfo.text, text);
            else
            {
                // Skip empty strings
                continue;
            }

            // Go through the string replacing '&' with spaces
            for(char* ptr = labelInfo.text; ptr[0] != '\0'; ptr++)
            {
                if(ptr[0] == '&')
                    ptr[0] = ' ';
            }

            // Finally insert the data
            const uint key = ModHashFromName(labelInfo.mod) + labelInfo.addr;

            labels.insert(std::make_pair(key, labelInfo));
        }
    };

    // Remove previous data
    labels.clear();

    const JSON jsonLabels       = json_object_get(Root, "labels");
    const JSON jsonAutoLabels   = json_object_get(Root, "autolabels");

    // Load user-set labels
    if(jsonLabels)
        AddLabels(jsonLabels, true);

    // Load auto-set labels
    if(jsonAutoLabels)
        AddLabels(jsonAutoLabels, false);
}

bool LabelEnum(LABELSINFO* List, size_t* Size)
{
    // CHECK: Export function
    if(!DbgIsDebugging())
        return false;

    // At least 1 parameter is required
    if(!List && !Size)
        return false;

    EXCLUSIVE_ACQUIRE(LockLabels);

    // See if the user requested a size
    if(Size)
    {
        *Size = labels.size() * sizeof(LABELSINFO);

        if(!List)
            return true;
    }

    // Fill out the return list while converting the offset
    // to a virtual address
    for(auto & itr : labels)
    {
        *List       = itr.second;
        List->addr  += ModBaseFromName(itr.second.mod);
        List++;
    }

    return true;
}

void LabelClear()
{
    EXCLUSIVE_ACQUIRE(LockLabels);
    labels.clear();
}