#ifndef STATICANALYSIS_H
#define STATICANALYSIS_H

#include <QThread>
#include <QMap>
#include "Bridge.h"
#include "ApiFingerprints.h"
#include "QBeaEngine.h"


#include "StaticAnalysis_ApiCalls.h"
#include "StaticAnalysis_Functions.h"

// we use QThread here to make sure that the GUI will not be frozen
class StaticAnalysis : public QThread
{
    Q_OBJECT

    static StaticAnalysis* mPtr;

    // number of instruction in buffer
    static const int mWindowToThePast = 10;

public:
    explicit StaticAnalysis(QWidget *parent = 0);

    static StaticAnalysis *instance();
    const StaticAnalysis_ApiCalls *calls() ;
    const StaticAnalysis_Functions *functions();

    bool isWorking() const;

signals:
    void staticAnalysisCompleted();
    void endThread();

public slots:
    void analyze(int_t Base, int_t Size);
    void clear();
protected slots:
    void end();

protected:
    void run();
    void see(DISASM *disasm);
    void think();
private:
    // current number of disassembled instruction during analysis
    unsigned int mInstructionCounter;
    // we only can analyze if we have a memory buffer and an size the target region
    bool mHaveParameters;
    // are we working
    bool mWorking;
    // flag for possible errors during disassembling
    bool mErrorDuringAnalysis;
    // must-have parameters for disassembling
    int_t mBase;
    int_t mSize;


    StaticAnalysis_ApiCalls* mApicalls;
    StaticAnalysis_Functions* mFunctions;

};

#endif // STATICANALYSIS_H
