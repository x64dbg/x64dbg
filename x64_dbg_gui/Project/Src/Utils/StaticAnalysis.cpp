#include "StaticAnalysis.h"

/*
 * maybe we should use another abstract class "Analyzer" to derive a child class "ApiComments"
 * using an interface like "ApiComments::extractInformation(Instruction_t currentInstr);" for better
 * software design
 *
 * but today we don't have any other static analysis reasons
 *
 * THIS IS compileable PSEUDO-CODE !!!
 */

StaticAnalysis::StaticAnalysis(QWidget *parent) :
    QThread(parent), mHaveParameters(false), mInstructionCounter(0), mErrorDuringAnalysis(false)
{
    connect(Bridge::getBridge(), SIGNAL(analyseCode(int_t,int_t)), this, SLOT(analyze(int_t,int_t)));
}



void StaticAnalysis::analyze(const int_t Base,const int_t Size){
    mBase = Base;
    mSize = Size;

    char labelText[MAX_LABEL_SIZE];    // maybe there is a better way, since we allow the user to rename labels
    char moduleText[MAX_MODULE_SIZE];
    bool hasLabel = DbgGetLabelAt(0x00402024, SEG_DEFAULT, labelText);
    bool hasModule = DbgGetModuleAt(0x0040116E, moduleText);

    // we need both to look for informations
    if(hasLabel && hasModule)
        qDebug()<< " has "<< labelText << " and "<<moduleText;
    return;

    start();
}

/**
 * @brief returns the (n-back)-th instruction
 * @param n
 * @return
 */
inline bool StaticAnalysis::lastInstruction(const int back, DISASM* InstrANS){
    // sorry there is no information
    if(mInstructionCounter - back < 0)
        return false;
    // now there are definetly instructions in our instructionsbuffer
    const unsigned int realIndex = (mInstructionCounter - back + mWindowToThePast) % mWindowToThePast;
    InstrANS = &mTempCircularMemoryArray[realIndex];
    return true;
}

/**
 * @brief stores an instruction in the buffer
 * @param Instr
 */
void StaticAnalysis::saveCurrentInstruction(DISASM Instr){
    const unsigned int realIndex = (mInstructionCounter) % mWindowToThePast;
    mTempCircularMemoryArray[realIndex] = Instr;
}

void StaticAnalysis::run()
{

    // will be executed by "StaticAnalysis::start()"

    /* the idea is to disassemble a whole sections in O(number of instructions)
     * and store the last n-Instructions (n=WindowToThePast) using an array of size n
     *
     * !!!HERE SHOULD BE DONE ALL STATIC ANALYSIS!!!
     *
     * if there is an api call --> set comment to it
     * and read from the Histroy the instruction to set there a comment for parameters TokenOrigin
     *
     * IMPORTANT: mWindowToThePast has to be the maximum number of parameters of the functions in the apifingerprints
     *            if not, there will be a missing comments (user just not gets a comment)
     *
     * TODO: use the gathered information in Disassembly.cpp in "Disassembly::paintContent()"
     */

    //thread-safe variables
    int_t addr=mBase;
    int_t size=mSize;



    //allocate+read data
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
            if(disasm.Instruction.BranchType && disasm.Instruction.BranchType!=RetType && !(disasm.Argument1.ArgType &REGISTER_TYPE))
            {

                if(disasm.Instruction.Opcode == 0xE8){
                    detectApiCalls(disasm);
                }

            }


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


    end();
}

void StaticAnalysis::detectApiCalls(DISASM curInstr){

    saveCurrentInstruction(curInstr);
    qDebug() << "analyse "<<curInstr.CompleteInstr;



    DISASM disasm;
    memset(&disasm, 0, sizeof(disasm));
#ifdef _WIN64
    disasm.Archi=64;
#endif // _WIN64
    disasm.EIP=(UIntPtr)data;
    disasm.VirtualAddr=(UInt64)curInstr.Instruction.AddrValue;


    // we check if there is an api call by asking the "Dbg...At()" function what it thinks
    char labelText[MAX_LABEL_SIZE];    // maybe there is a better way, since we allow the user to rename labels
    char moduleText[MAX_MODULE_SIZE];
    bool hasLabel = DbgGetLabelAt(curInstr.VirtualAddr, SEG_DEFAULT, labelText);
    bool hasModule = DbgGetModuleAt(curInstr.VirtualAddr, moduleText);

    // we need both to look for informations
    if(hasLabel && hasModule){

        qDebug() << curInstr.VirtualAddr << " has "<< labelText << " and "<<moduleText;
        return;
        // now we have a chance to find everything
        APIFunction function;
        bool found = ApiFingerprints::instance()->findFunction(QString(moduleText).trimmed().replace(".dll",""),QString(labelText).trimmed(),&function);


    }


}

void StaticAnalysis::end(){
    if(!mErrorDuringAnalysis)
        emit staticAnalysisCompleted();
    qDebug() << "finished";
}
