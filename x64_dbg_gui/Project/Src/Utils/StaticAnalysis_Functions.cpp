#include "StaticAnalysis_Functions.h"

#include "StaticAnalysis_Interface.h"
#include "StaticAnalysis.h"
#include <QMap>

#define _isCall(disasm)  ((disasm->Instruction.Opcode == 0xE8) && disasm->Instruction.BranchType && disasm->Instruction.BranchType!=RetType && !(disasm->Argument1.ArgType &REGISTER_TYPE))
#define _isRet(disasm)  ((disasm->Instruction.BranchType && disasm->Instruction.BranchType==RetType ))

/* fot the sake of simplicity we follow the simple idea that after a call there must be a "ret"
 * we do not follow any jumps (so the output can be wrong)
 *
 * a pathological situation would be
 *
 * 0100: call 0123
 * ; ...
 * 0123: xor eax, eax
 * 0124: jz 0126
 * 0125: ret   <-- will be detected
 * 0126: ret   <-- real function end
 *
 * but static analysis at obfuscated code is a tradeoff between speed and smartness
 */

StaticAnalysis_Functions::StaticAnalysis_Functions(StaticAnalysis *parent) : StaticAnalysis_Interface(parent)
{
    clear();
}

FunctionStructure StaticAnalysis_Functions::function(int_t va){
    /* usage
     *
     * FunctionStructure f = StaticAnalysis->functions()->function(some_address);
     * if(f.valid)
     *      cout << "start at" << f.VA_start << " and ends at "<<f.VA_end
     */

    FunctionStructure f;
    if(!mParent->isWorking()){
        for(int i=0;i<mFunctions.count();i++){
            if(mFunctions.at(i).VA_start <= va && va <= mFunctions.at(i).VA_end){
                return mFunctions.at(i);
            }
        }
    }
    return f;
}

void StaticAnalysis_Functions::clear()
{
    ProcStart.clear();
    mFunctions.clear();
    hasCurrent = false;
}

void StaticAnalysis_Functions::see(DISASM *disasm)
{
    // is current address a possible procedure start?
    if(ProcStart.contains(disasm->VirtualAddr)){
        // set current va as current active procedure
        mCurrent.VA_start = disasm->VirtualAddr;
        // ok, let's look for a "ret"
        hasCurrent = true;
    }

    if(_isCall(disasm)){
        // we collect all possible procedure-start-VA
        ProcStart.insert(disasm->Instruction.AddrValue);
    }else if(_isRet(disasm) && hasCurrent){
        // we have an active procedure start, so this "ret" closes the current search
        mCurrent.VA_end = disasm->VirtualAddr;
        // copy
        FunctionStructure f = mCurrent;
        // remember
        mFunctions.append(f);
        // we aren't in any procedure body
        hasCurrent = false;
    }
}

bool StaticAnalysis_Functions::think()
{
    qDebug() << "we found "<<mFunctions.count() << "functions";
    for(int i=0;i<mFunctions.count();i++){
        qDebug() << "start:" << QString("%1").arg(mFunctions.at(i).VA_start,8,16,QChar('0')) << " end: "<<QString("%1").arg(mFunctions.at(i).VA_end,8,16,QChar('0'));
    }
    return true;
}
