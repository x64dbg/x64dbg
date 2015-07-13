#ifndef _EXCEPTIONDIRECTORYANALYSIS_H
#define _EXCEPTIONDIRECTORYANALYSIS_H

#include "analysis.h"
#include <functional>

class ExceptionDirectoryAnalysis : public Analysis
{
public:
    explicit ExceptionDirectoryAnalysis(uint base, uint size);
    ~ExceptionDirectoryAnalysis();
    void Analyse() override;
    void SetMarkers() override;

private:
    uint _moduleBase;
    uint _functionInfoSize;
    void* _functionInfoData;
    std::vector<std::pair<uint, uint>> _functions;

#ifdef _WIN64
    void EnumerateFunctionRuntimeEntries64(std::function<bool(PRUNTIME_FUNCTION)> Callback);
#endif // _WIN64
};

#endif //_EXCEPTIONDIRECTORYANALYSIS_H