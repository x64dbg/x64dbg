#include "exceptiondirectoryanalysis.h"
#include "module.h"
#include "TitanEngine/TitanEngine.h"
#include "memory.h"
#include "console.h"
#include "function.h"
#include "database_cb_batcher.h"

ExceptionDirectoryAnalysis::ExceptionDirectoryAnalysis(duint base, duint size)
    : Analysis(base, size)
{
#ifdef _WIN64
    SHARED_ACQUIRE(LockModules);

    auto modInfo = ModInfoFromAddr(base);
    if(modInfo == nullptr)
        return;

    mModuleBase = modInfo->base;
    mRuntimeFunctions = modInfo->runtimeFunctions;
#endif // _WIN64
}

void ExceptionDirectoryAnalysis::Analyse()
{
#ifdef _WIN64
    for(const auto & Function : mRuntimeFunctions)
    {
        auto funcAddr = mModuleBase + Function.BeginAddress;
        auto funcEnd = mModuleBase + Function.EndAddress - 1;

        // If within limits...
        if(inRange(funcAddr) && inRange(funcEnd) && funcAddr <= funcEnd)
            mFunctions.push_back({ funcAddr, funcEnd });
    };
    dprintf(QT_TRANSLATE_NOOP("DBG", "%u functions discovered!\n"), DWORD(mFunctions.size()));
#else //x32
    dputs(QT_TRANSLATE_NOOP("DBG", "This kind of analysis doesn't work on x32 executables...\n"));
#endif // _WIN64
}

void ExceptionDirectoryAnalysis::SetMarkers()
{
    DbCallbackBatcher batcher;

    FunctionDelRange(mBase, mBase + mSize - 1, false);
    for(const auto & function : mFunctions)
        FunctionAdd(function.first, function.second, false);
}
