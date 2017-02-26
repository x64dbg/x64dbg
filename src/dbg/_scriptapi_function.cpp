#include "_scriptapi_function.h"
#include "_scriptapi_module.h"
#include "function.h"

SCRIPT_EXPORT bool Script::Function::Add(duint start, duint end, bool manual, duint instructionCount)
{
    return FunctionAdd(start, end, manual, instructionCount);
}

SCRIPT_EXPORT bool Script::Function::Add(const FunctionInfo* info)
{
    if(!info)
        return false;
    auto base = Module::BaseFromName(info->mod);
    if(!base)
        return false;
    return Add(base + info->rvaStart, base + info->rvaEnd, info->manual, info->instructioncount);
}

SCRIPT_EXPORT bool Script::Function::Get(duint addr, duint* start, duint* end, duint* instructionCount)
{
    return FunctionGet(addr, start, end, instructionCount);
}

SCRIPT_EXPORT bool Script::Function::GetInfo(duint addr, FunctionInfo* info)
{
    FUNCTIONSINFO function;
    if(!FunctionGetInfo(addr, function))
        return false;
    if(info)
    {
        strcpy_s(info->mod, function.mod().c_str());
        info->rvaStart = function.start;
        info->rvaEnd = function.end;
        info->manual = function.manual;
        info->instructioncount = function.instructioncount;
    }
    return true;
}

SCRIPT_EXPORT bool Script::Function::Overlaps(duint start, duint end)
{
    return FunctionOverlaps(start, end);
}

SCRIPT_EXPORT bool Script::Function::Delete(duint address)
{
    return FunctionDelete(address);
}

SCRIPT_EXPORT void Script::Function::DeleteRange(duint start, duint end, bool deleteManual)
{
    FunctionDelRange(start, end, deleteManual);
}

SCRIPT_EXPORT void Script::Function::DeleteRange(duint start, duint end)
{
    DeleteRange(start, end, false);
}

SCRIPT_EXPORT void Script::Function::Clear()
{
    FunctionClear();
}

SCRIPT_EXPORT bool Script::Function::GetList(ListOf(FunctionInfo) list)
{
    std::vector<FUNCTIONSINFO> functionList;
    FunctionGetList(functionList);
    std::vector<FunctionInfo> functionScriptList;
    functionScriptList.reserve(functionList.size());
    for(const auto & function : functionList)
    {
        FunctionInfo scriptFunction;
        strcpy_s(scriptFunction.mod, function.mod().c_str());
        scriptFunction.rvaStart = function.start;
        scriptFunction.rvaEnd = function.end;
        scriptFunction.manual = function.manual;
        scriptFunction.instructioncount = function.instructioncount;
        functionScriptList.push_back(scriptFunction);
    }
    return BridgeList<FunctionInfo>::CopyData(list, functionScriptList);
}
