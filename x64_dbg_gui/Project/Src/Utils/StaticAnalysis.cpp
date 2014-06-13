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
}

void StaticAnalysis::setParameters(const int_t Base,const int_t Size)
{
    mBase = Base;
    mSize = Size;
}

/**
 * @brief returns the (n-back)-th instruction
 * @param n
 * @return
 */
inline bool StaticAnalysis::lastInstruction(const int back, Instruction_t* InstrANS){
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
void StaticAnalysis::saveCurrentInstruction(Instruction_t Instr){
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



    /********** dummy variables ****************/
    byte_t* data = NULL;
    uint size = 0;
    uint instrIndex = 0;
    uint origBase = 0;
    uint origInstRVA = 0;
    uint_t currentAddress = 0;

    // WARNING: currently an infinite loop
    while(true/* disassembling of given range is not complete*/){

        // TODO make sure that everything works fine during disassembling, otherwise just set "mErrorDuringAnalysis=true" and stop
        Instruction_t curInstr = DisassembleAt(&data,size,instrIndex,origBase,origInstRVA);
        // store the instruction
        saveCurrentInstruction(curInstr);

        // we check if there is an api call by asking the "Dbg...At()" function what it thinks
        char labelText[MAX_LABEL_SIZE];    // maybe there is a better way, since we allow the user to rename labels
        char moduleText[MAX_MODULE_SIZE];
        bool hasLabel = DbgGetLabelAt(currentAddress,SEG_DEFAULT, labelText);
        bool hasModule = DbgGetModuleAt(currentAddress, moduleText);

        // we need both to look for informations
        if(hasLabel && hasModule){
            // now we have a chance to find everything
            APIFunction function;
            bool found = ApiFingerprints::instance()->findFunction(QString(moduleText).trimmed().replace(".dll",""),QString(labelText).trimmed(),&function);

            if(found){
                // ok we can become excited, since there is an api call and we detect it
                const unsigned int numberOfArguments = function.Arguments.count();
                // prepare a nice comment like "int MessageBoxA(...)"
                QString comment = function.ReturnType + " " + function.Name + "(...)";
                ApiComments.insert(currentAddress,comment);

                // now we try to propagate back the arguments to the last numberOfArguments-th instructions
                // if and only if the instruction is a push instruction
                for(int back=1;back<=numberOfArguments;back++){  // back=1; is correct!
                    Instruction_t rememberInstr;
                    // extract the instruction from the buffer
                    if(lastInstruction(back,&rememberInstr)){
                        // check if the previous instruction (current-back)-th instruction is a "push" instruction
                        // the tokenizer currently only can tell if there is a "push/pop" and cannot distinguish between them
                        if(QString(rememberInstr.disasm.Instruction.Mnemonic).trimmed().toLower() == "Push"){
                            // there is a "push 0xDEADBEEF" , so add a comment
                            comment = function.Arguments.at(back-1);
                            ApiComments.insert(currentAddress,comment);
                        }else{
                            // no push instruction, but an argument left
                            // for several reasons (otherwise unsafe code) we stop here
                            // a possible solution would be a bigger InstructionBuffer
                            break;
                            // TODO in the future: not stopping; maybe there is another instruction between the "push" instructions
                            //                     or there is something like "mov [esp+-0x42], arg"
                        }

                    }else{
                        // we cannot reach the instruction in the buffer (there nothing)
                        break;
                    }
                }
            }
        }
    }
    end();
}

void StaticAnalysis::end(){
    if(!mErrorDuringAnalysis)
        emit staticAnalysisCompleted();
}
