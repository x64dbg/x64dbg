#include "_scriptapi_argument.h"
#include "_scriptapi_module.h"
#include "argument.h"

SCRIPT_EXPORT bool Script::Argument::Add(duint start, duint end, bool manual, duint instructionCount)
{
    return ArgumentAdd(start, end, manual, instructionCount);
}

SCRIPT_EXPORT bool Script::Argument::Add(const ArgumentInfo* info)
{
    if(!info)
        return false;
    auto base = Module::BaseFromName(info->mod);
    if(!base)
        return false;
    return Add(base + info->rvaStart, base + info->rvaEnd, info->manual, info->instructioncount);
}

SCRIPT_EXPORT bool Script::Argument::Get(duint addr, duint* start, duint* end, duint* instructionCount)
{
    return ArgumentGet(addr, start, end, instructionCount);
}

SCRIPT_EXPORT bool Script::Argument::GetInfo(duint addr, ArgumentInfo* info)
{
    ARGUMENTSINFO argument;
    if(!ArgumentGetInfo(addr, argument))
        return false;
    if(info)
    {
        strcpy_s(info->mod, argument.mod().c_str());
        info->rvaStart = argument.start;
        info->rvaEnd = argument.end;
        info->manual = argument.manual;
        info->instructioncount = argument.instructioncount;
    }
    return true;
}

SCRIPT_EXPORT bool Script::Argument::Overlaps(duint start, duint end)
{
    return ArgumentOverlaps(start, end);
}

SCRIPT_EXPORT bool Script::Argument::Delete(duint address)
{
    return ArgumentDelete(address);
}

SCRIPT_EXPORT void Script::Argument::DeleteRange(duint start, duint end, bool deleteManual)
{
    ArgumentDelRange(start, end, deleteManual);
}

SCRIPT_EXPORT void Script::Argument::Clear()
{
    ArgumentClear();
}

SCRIPT_EXPORT bool Script::Argument::GetList(ListOf(ArgumentInfo) list)
{
    std::vector<ARGUMENTSINFO> argumentList;
    ArgumentGetList(argumentList);
    std::vector<ArgumentInfo> argumentScriptList;
    argumentScriptList.reserve(argumentList.size());
    for(const auto & argument : argumentList)
    {
        ArgumentInfo scriptArgument;
        strcpy_s(scriptArgument.mod, argument.mod().c_str());
        scriptArgument.rvaStart = argument.start;
        scriptArgument.rvaEnd = argument.end;
        scriptArgument.manual = argument.manual;
        scriptArgument.instructioncount = argument.instructioncount;
        argumentScriptList.push_back(scriptArgument);
    }
    return BridgeList<ArgumentInfo>::CopyData(list, argumentScriptList);
}