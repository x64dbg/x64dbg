#include "StaticAnalysis_ApiCalls.h"
#include "ApiFingerprints.h"

#define _isPush(disasm)  (QString(disasm->Instruction.Mnemonic).trimmed().toLower() == "push")
#define _isCall(disasm)  ((disasm->Instruction.Opcode == 0xE8) && disasm->Instruction.BranchType && disasm->Instruction.BranchType!=RetType && !(disasm->Argument1.ArgType &REGISTER_TYPE))

StaticAnalysis_ApiCalls::StaticAnalysis_ApiCalls()
{
    clear();
}

bool StaticAnalysis_ApiCalls::hasComment(int_t va){
    return mComments.contains(va);
}

QString StaticAnalysis_ApiCalls::comment(int_t va){
    return mComments.find(va).value();
}


void StaticAnalysis_ApiCalls::clear()
{
    mCurrent.clear();
    mCalls.clear();
    mApiJumps.clear();
    mComments.clear();
}

void StaticAnalysis_ApiCalls::see(DISASM *disasm)
{
    if(_isPush(disasm))
    {
        // there is "push ..." --> remember
        // bugfix!!!!! -1
        mCurrent.arguments.append(disasm->VirtualAddr );
    }
    else if(disasm->Instruction.Opcode == 0xFF)
    {
        // no part of api-call
        mCurrent.clear();

        char labelText[MAX_LABEL_SIZE];
        bool hasLabel = DbgGetLabelAt(disasm->Argument1.Memory.Displacement, SEG_DEFAULT, labelText);

        if(hasLabel){
            // api calls have 0xFF-jumps as target addresses, we need to remember them
            ApiJumpStructure j;
            j.rva = disasm->VirtualAddr;
            j.target = disasm->Argument1.Memory.Displacement;
            j.label = QString(labelText);
            mApiJumps.append(j);
        }


    }
    else if(_isCall(disasm))
    {
        // set missing informations
        mCurrent.rva = disasm->VirtualAddr ;
        mCurrent.target_rva = disasm->Instruction.AddrValue ;
        // copy structure
        CallStructure t = mCurrent;
        mCalls.append(t);
        mCurrent.clear();

    }
    else
    {
        // something else
        mCurrent.clear();
    }
}

bool StaticAnalysis_ApiCalls::think()
{
    //qDebug() << "we found "<<mCalls.count() << "calls ";
    const unsigned int apijumps_count = mApiJumps.count();
    unsigned int numberOfApiCalls=0;

    int_t label_va = 0;
    QList<CallStructure>::iterator call = mCalls.begin();
    // iterate all calls
    while (call != mCalls.end()) {
        bool is_api = false;
        // iterate all 0xFF instructions
        for(int j=0;j<apijumps_count;j++){
            // is call a api call?
            if(call->target_rva == mApiJumps.at(j).rva){
                    // having a label we can look in our database to get the arguments list
                    APIFunction f = ApiFingerprints::instance()->findFunction(mApiJumps.at(j).label);

                    if(!f.invalid){
                        // yeah we know everything about the api-call!
                        QString com = f.ReturnType+" "+f.Name;
                        mComments.insert(call->rva,f.ReturnType+" "+f.Name);

                        int pos=0;
                        // set comments for the arguments
                        for(int i=call->arguments.count() -1; i >=0;i--){
                            QString arg = f.Arguments.at(pos).Type +" "+ f.Arguments.at(pos).Name;
                            mComments.insert(call->arguments.at(i),arg);
                            pos++;

                            if(pos >=f.Arguments.count())
                                break;
                        }


                    }



                is_api = true;
                numberOfApiCalls++;
                break;
            }
        }


        // I don't know if we need non-api-calls (so we remove them)
        if(!is_api){
            call = mCalls.erase(call);
        }else{
            call++;
        }



    }

    // debug output
    qDebug() << "we found "<<numberOfApiCalls << "api calls ";
    QMap<int_t,QString>::const_iterator deb=mComments.begin();
    while(deb != mComments.end()){
        qDebug() << QString("%1").arg(deb.key(),8,16,QChar('0')) << " " << deb.value();
        deb++;
    }


    return true;
}
