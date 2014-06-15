#ifndef STATICANALYSIS_APICALLS_H
#define STATICANALYSIS_APICALLS_H

#include "StaticAnalysis_Interface.h"
#include <QMap>

class StaticAnalysis_ApiCalls : public StaticAnalysis_Interface
{
    struct ApiJumpStructure{
        int_t instruction_va;
        int_t target_va;
        QString label;
    };

    struct CallStructure{
        QList<int_t> arguments;
        int_t instruction_va;
        int_t target_va;
        void clear(){
            arguments.clear();
            instruction_va = 0;
            target_va = 0;
        }
        bool isApiCall;
    };

    QMap<int_t,QString> mComments;   // <rva,comment>

    QList<ApiJumpStructure> mApiJumps;

    // here we store all comments
    QList<CallStructure> mCalls;
    CallStructure mCurrent;

public:
    StaticAnalysis_ApiCalls(StaticAnalysis *parent);
    // clear all extracted informations
    void clear();
    // each derived class will see each instruction only once (in a analysis step)
    void see(DISASM* disasm);
    // this methods process all gathered informations
    bool think();


    QString comment(int_t va);
    bool hasComment(int_t va);
};

#endif // STATICANALYSIS_APICALLS_H
