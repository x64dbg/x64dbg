#ifndef STATICANALYSIS_H
#define STATICANALYSIS_H

#include <QThread>
#include <QMap>
#include "Bridge.h"
#include "ApiFingerprints.h"
#include "QBeaEngine.h"

// we use QThread here to make sure that the GUI will not be frozen
class StaticAnalysis : public QThread
{
    Q_OBJECT

    // number of instruction in buffer
    static const int mWindowToThePast = 10;

public:
    explicit StaticAnalysis(QWidget *parent = 0);


signals:
    void staticAnalysisCompleted();

public slots:
    void analyze(int_t Base, int_t Size);
protected:
    void run();
    void end();
    bool lastInstruction(const int back, DISASM *InstrANS);
    void saveCurrentInstruction(DISASM Instr);

    void detectApiCalls(DISASM curInstr);
private:
    // current number of disassembled instruction during analysis
    unsigned int mInstructionCounter;
    // circular array as Buffer
    DISASM mTempCircularMemoryArray[mWindowToThePast];
    // we only can analyze if we have a memory buffer and an size the target region
    bool mHaveParameters;
    // flag for possible errors during disassembling
    bool mErrorDuringAnalysis;
    // must-have parameters for disassembling
    int_t mBase;
    int_t mSize;

    // here we store all comments
    QMap<int_t,QString> ApiComments;


};

#endif // STATICANALYSIS_H
