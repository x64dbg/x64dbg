#include "StaticAnalysis.h"

StaticAnalysis* StaticAnalysis::mPtr = NULL;


StaticAnalysis *StaticAnalysis::instance()
{
    return mPtr;
}

StaticAnalysis::StaticAnalysis(QWidget *parent) :
    QThread(parent), mHaveParameters(false), mInstructionCounter(0), mErrorDuringAnalysis(false), mWorking(false)
{
    connect(Bridge::getBridge(), SIGNAL(analyseCode(int_t,int_t)), this, SLOT(analyze(int_t,int_t)));
    connect(this,SIGNAL(endThread()),this,SLOT(end()));

    apicalls = new StaticAnalysis_ApiCalls();
    mPtr = this;
}



void StaticAnalysis::analyze(const int_t Base,const int_t Size){
    if(!mWorking){
        GuiAddLogMessage(QString("start static analysis\n").toUtf8().constData());
        mBase = Base;
        mSize = Size;
        mWorking=true;
        start();
    }
}

StaticAnalysis_ApiCalls* StaticAnalysis::calls() {
    return apicalls;
}


void StaticAnalysis::run()
{
    int_t addr=mBase;
    int_t size=mSize;
    unsigned char* data = new unsigned char[size];
    if(!DbgMemRead(addr, data, size))
    {
        //ERROR
        GuiScriptError(0, "DbgMemRead failed!"); //TODO: create a better error thing
        return;
    }

    //loop over all instructions
    DISASM disasm;
    memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
    disasm.Archi=64;
#endif // _WIN64
    disasm.EIP=(UIntPtr)data;
    disasm.VirtualAddr=(UInt64)addr;
    int_t i=0;
    while(i<size)
    {
        if(!(i%0x1000)) //progress (optional)
        {
            double percent=(double)i/(double)size;
            //GuiSetProgress((int)(percent*100)); //TODO: reference in statusbar (custom-drawn)
        }
        int len=Disasm(&disasm);
        if(len!=UNKNOWN_OPCODE)
        {
            //TODO: ANALYSE HERE
            apicalls->see(&disasm);


        }
        else
            len=1;

        disasm.EIP+=len;
        disasm.VirtualAddr+=len;
        i+=len;
    }
    //GuiSetProgress(100);
    GuiUpdateAllViews();
    delete data;

    think();
    emit endThread();
}

void StaticAnalysis::think(){
    apicalls->think();
}

void StaticAnalysis::end(){
    mWorking=false;
    if(!mErrorDuringAnalysis){
        GuiAddLogMessage(QString("start static analysis finished\n").toUtf8().constData());
    }
    emit staticAnalysisCompleted();
    qDebug() << "finished";
}
