#ifndef _EXCEPTIONDIRECTORYANALYSIS_H
#define _EXCEPTIONDIRECTORYANALYSIS_H

#include "analysis.h"
#include <functional>

class ExceptionDirectoryAnalysis : public Analysis
{
public:
    explicit ExceptionDirectoryAnalysis(duint base, duint size);
    ~ExceptionDirectoryAnalysis();
    void Analyse() override;
    void SetMarkers() override;

private:
    duint mModuleBase;
    duint mFunctionInfoSize;
    void* mFunctionInfoData;
    std::vector<std::pair<duint, duint>> mFunctions;

#ifdef _WIN64
    void EnumerateFunctionRuntimeEntries64(const std::function<bool(PRUNTIME_FUNCTION)> & Callback) const;
#endif // _WIN64
};

#endif //_EXCEPTIONDIRECTORYANALYSIS_H