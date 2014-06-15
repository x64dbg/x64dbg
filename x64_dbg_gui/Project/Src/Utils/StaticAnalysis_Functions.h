#ifndef STATICANALYSIS_FUNCTIONS_H
#define STATICANALYSIS_FUNCTIONS_H

#include "StaticAnalysis_Interface.h"


struct FunctionStructure{
    int_t VA_start;
    int_t VA_end;
    bool valid;
    FunctionStructure(){
        valid=false;
    }
};
class StaticAnalysis_Functions : public StaticAnalysis_Interface
{

    QSet<int_t> ProcStart;

    QList<FunctionStructure> mFunctions;

public:
    StaticAnalysis_Functions(StaticAnalysis *parent);

    FunctionStructure mCurrent;
    bool hasCurrent;

    void clear();
    void see(DISASM* disasm);
    bool think();

    FunctionStructure function(int_t va);
};

#endif // STATICANALYSIS_FUNCTIONS_H
