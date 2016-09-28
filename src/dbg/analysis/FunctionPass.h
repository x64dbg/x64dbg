#pragma once

#include <functional>
#include "AnalysisPass.h"
#include "BasicBlock.h"

class FunctionPass : public AnalysisPass
{
public:
    FunctionPass(duint VirtualStart, duint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~FunctionPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
    duint m_ModuleStart;

    PVOID m_FunctionInfo;
    ULONG m_FunctionInfoSize;

    void AnalysisWorker(duint Start, duint End, std::vector<FunctionDef>* Blocks);
    void FindFunctionWorkerPrepass(duint Start, duint End, std::vector<FunctionDef>* Blocks);
    void FindFunctionWorker(std::vector<FunctionDef>* Blocks);

    bool ResolveKnownFunctionEnd(FunctionDef* Function);
    bool ResolveFunctionEnd(FunctionDef* Function, BasicBlock* LastBlock);

#ifdef _WIN64
    void EnumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback);
#endif // _WIN64
};