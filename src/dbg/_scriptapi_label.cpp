#include "_scriptapi_label.h"
#include "_scriptapi_module.h"
#include "label.h"

SCRIPT_EXPORT bool Script::Label::Set(duint addr, const char* text, bool manual)
{
    return LabelSet(addr, text, manual);
}

bool Script::Label::Set(const LabelInfo* info)
{
    if(!info)
        return false;
    auto base = Module::BaseFromName(info->mod);
    if(!base)
        return false;
    return Set(base + info->rva, info->text, info->manual);
}

SCRIPT_EXPORT bool Script::Label::FromString(const char* label, duint* addr)
{
    return LabelFromString(label, addr);
}

SCRIPT_EXPORT bool Script::Label::Get(duint addr, char* text)
{
    return LabelGet(addr, text);
}

SCRIPT_EXPORT bool Script::Label::GetInfo(duint addr, LabelInfo* info)
{
    LABELSINFO label;
    if(!LabelGetInfo(addr, &label))
        return false;
    if(info)
    {
        strcpy_s(info->mod, label.mod);
        info->rva = label.addr;
        strcpy_s(info->text, label.text);
        info->manual = label.manual;
    }
    return true;
}

SCRIPT_EXPORT bool Script::Label::Delete(duint addr)
{
    return LabelDelete(addr);
}

SCRIPT_EXPORT void Script::Label::DeleteRange(duint start, duint end)
{
    LabelDelRange(start, end);
}

SCRIPT_EXPORT void Script::Label::Clear()
{
    LabelClear();
}

SCRIPT_EXPORT bool Script::Label::GetList(ListOf(LabelInfo) listInfo)
{
    std::vector<LABELSINFO> labelList;
    LabelGetList(labelList);
    std::vector<LabelInfo> labelScriptList;
    labelScriptList.reserve(labelList.size());
    for(const auto & label : labelList)
    {
        LabelInfo scriptLabel;
        strcpy_s(scriptLabel.mod, label.mod);
        scriptLabel.rva = label.addr;
        strcpy_s(scriptLabel.text, label.text);
        scriptLabel.manual = label.manual;
        labelScriptList.push_back(scriptLabel);
    }
    return List<LabelInfo>::CopyData(listInfo, labelScriptList);
}