#include "label.h"
#include "threading.h"
#include "module.h"
#include "memory.h"
#include "debugger.h"

typedef std::unordered_map<uint, LABELSINFO> LabelsInfo;

static LabelsInfo labels;

bool LabelSet(uint Address, const char* Text, bool Manual)
{
	// CHECK: Exported/Command function
	if (!DbgIsDebugging())
		return false;

	// A valid memory address must be supplied
	if (!MemIsValidReadPtr(Address))
		return false;

	// Make sure the string is supplied, within bounds, and not a special delimiter
	if (!Text || Text[0] == '\1' || strlen(Text) >= MAX_LABEL_SIZE - 1)
		return false;

	// Labels cannot be "address" of actual variables
	if (strstr(Text, "&"))
		return false;

	// Delete the label if no text was supplied
    if(Text[0] == '\0')
        return LabelDelete(Address);

	// Fill out the structure data
    LABELSINFO labelInfo;
    labelInfo.manual	= Manual;
	labelInfo.addr		= Address - ModBaseFromAddr(Address);
	strcpy_s(labelInfo.text, Text);
    ModNameFromAddr(Address, labelInfo.mod, true);

	EXCLUSIVE_ACQUIRE(LockLabels);

	// Insert label by key
	uint key = ModHashFromAddr(Address);

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

	for (auto& itr : labels)
	{
		// Check if the actual label name matches
		if (strcmp(itr.second.text, Text))
			continue;

		if (Address)
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
	if (!DbgIsDebugging())
		return;

	// Are all comments going to be deleted?
	// 0x00000000 - 0xFFFFFFFF
	if (Start == 0 && End == ~0)
	{
		EXCLUSIVE_ACQUIRE(LockLabels);
		labels.clear();
	}
	else
	{
		// Make sure 'Start' and 'End' reference the same module
		uint moduleBase = ModBaseFromAddr(Start);

		if (moduleBase != ModBaseFromAddr(End))
			return;

		EXCLUSIVE_ACQUIRE(LockLabels);
		for (auto itr = labels.begin(); itr != labels.end();)
		{
			// Ignore manually set entries
			if (itr->second.manual)
			{
				itr++;
				continue;
			}

			// [Start, End)
			if (itr->second.addr >= Start && itr->second.addr < End)
				itr = labels.erase(itr);
			else
				itr++;
		}
	}
}

void labelcachesave(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    const JSON jsonlabels = json_array();
    const JSON jsonautolabels = json_array();
    for(LabelsInfo::iterator i = labels.begin(); i != labels.end(); ++i)
    {
        const LABELSINFO curLabel = i->second;
        JSON curjsonlabel = json_object();
        json_object_set_new(curjsonlabel, "module", json_string(curLabel.mod));
        json_object_set_new(curjsonlabel, "address", json_hex(curLabel.addr));
        json_object_set_new(curjsonlabel, "text", json_string(curLabel.text));
        if(curLabel.manual)
            json_array_append_new(jsonlabels, curjsonlabel);
        else
            json_array_append_new(jsonautolabels, curjsonlabel);
    }
    if(json_array_size(jsonlabels))
        json_object_set(root, "labels", jsonlabels);
    json_decref(jsonlabels);
    if(json_array_size(jsonautolabels))
        json_object_set(root, "autolabels", jsonautolabels);
    json_decref(jsonautolabels);
}

void labelcacheload(JSON root)
{
    CriticalSectionLocker locker(LockLabels);
    labels.clear();
    const JSON jsonlabels = json_object_get(root, "labels");
    if(jsonlabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonlabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = true;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curLabel.text, text);
            else
                continue; //skip
            int len = (int)strlen(curLabel.text);
            for(int i = 0; i < len; i++)
                if(curLabel.text[i] == '&')
                    curLabel.text[i] = ' ';
            const uint key = ModHashFromName(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
    JSON jsonautolabels = json_object_get(root, "autolabels");
    if(jsonautolabels)
    {
        size_t i;
        JSON value;
        json_array_foreach(jsonautolabels, i, value)
        {
            LABELSINFO curLabel;
            const char* mod = json_string_value(json_object_get(value, "module"));
            if(mod && *mod && strlen(mod) < MAX_MODULE_SIZE)
                strcpy_s(curLabel.mod, mod);
            else
                *curLabel.mod = '\0';
            curLabel.addr = (uint)json_hex_value(json_object_get(value, "address"));
            curLabel.manual = false;
            const char* text = json_string_value(json_object_get(value, "text"));
            if(text)
                strcpy_s(curLabel.text, text);
            else
                continue; //skip
            const uint key = ModHashFromName(curLabel.mod) + curLabel.addr;
            labels.insert(std::make_pair(key, curLabel));
        }
    }
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
        
		if (!List)
			return true;
    }

	// Fill out the return list while converting the offset
	// to a virtual address
	for (auto& itr : labels)
	{
		*List		= itr.second;
		List->addr	+= ModBaseFromName(itr.second.mod);
		List++;
	}

    return true;
}

void LabelClear()
{
	EXCLUSIVE_ACQUIRE(LockLabels);
	labels.clear();
}