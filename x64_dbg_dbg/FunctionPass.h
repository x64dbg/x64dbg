#pragma once

#include <functional>
#include "AnalysisPass.h"
#include "BasicBlock.h"

class FunctionPass : public AnalysisPass
{
public:
    FunctionPass(uint VirtualStart, uint VirtualEnd, BBlockArray & MainBlocks);
    virtual ~FunctionPass();

    virtual const char* GetName() override;
    virtual bool Analyse() override;

private:
    uint m_ModuleStart;

    PVOID m_FunctionInfo;
    ULONG m_FunctionInfoSize;

    void AnalysisWorker(uint Start, uint End, std::vector<FunctionDef>* Blocks);
    void FindFunctionWorkerPrepass(uint Start, uint End, std::vector<FunctionDef>* Blocks);
    void FindFunctionWorker(std::vector<FunctionDef>* Blocks);

    bool ResolveKnownFunctionEnd(FunctionDef* Function);
    bool ResolveFunctionEnd(FunctionDef* Function, BasicBlock* LastBlock);

    void EnumerateFunctionRuntimeEntries64(std::function<bool(PRUNTIME_FUNCTION)> Callback);
};