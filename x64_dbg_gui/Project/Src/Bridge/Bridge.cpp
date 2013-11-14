#include "Bridge.h"

/************************************************************************************
                            Global Variables
************************************************************************************/
Bridge* mBridge;



/************************************************************************************
                            Class Members
************************************************************************************/
Bridge::Bridge(QObject *parent) : QObject(parent)
{
    mData = new QByteArray();

    QFile wFile("AsmCode.bin");

    if(wFile.open(QIODevice::ReadOnly) == false)
         //qDebug() << "File has not been opened.";

    *mData = wFile.readAll();
    //qDebug() << "Size: " << mData->size();

    if(mData->size() == 0)
    {
        //qDebug() << "No Data";
    }
}



void Bridge::readProcessMemory(byte_t* dest, uint_t va, uint_t size)
{
#ifdef BUILD_LIB
    DbgMemRead(va, dest, size);
#else
    stubReadProcessMemory(dest, va, size);
#endif
}

void Bridge::emitDisassembleAtSignal(int_t va, int_t eip)
{
#ifdef BUILD_LIB
    emit disassembleAt(va, eip);
#endif
}

uint_t Bridge::getSize(uint_t va)
{
#ifdef BUILD_LIB
    return DbgMemGetPageSize(va);
#else
    return mData->size();
#endif
}

uint_t Bridge::getBase(uint_t addr)
{
#ifdef BUILD_LIB
    return DbgMemFindBaseAddr(addr,0);
#else
    return 0x00401000;
#endif
}


void Bridge::emitDbgStateChanged(DBGSTATE state)
{
#ifdef BUILD_LIB
    emit dbgStateChanged(state);
#endif
}


void Bridge::emitAddMsgToLog(QString msg)
{
    emit addMsgToLog(msg);
}


void Bridge::emitClearLog()
{
    emit clearLog();
}

void Bridge::emitUpdateRegisters()
{
    emit updateRegisters();
}


bool Bridge::execCmd(const char* cmd)
{
    return DbgCmdExec(cmd);
}

bool Bridge::getMemMapFromDbg(MEMMAP* parMemMap)
{
    return DbgMemMap(parMemMap);
}

bool Bridge::isValidExpression(const char* expression)
{
    return DbgIsValidExpression(expression);
}

bool Bridge::valToString(const char* name, uint_t value)
{
    return DbgValToString(name, value);
}

void Bridge::Free(void* ptr)
{
    BridgeFree(ptr);
}


bool Bridge::getRegDumpFromDbg(REGDUMP* parRegDump)
{
    return DbgGetRegDump(parRegDump);
}

uint_t Bridge::getValFromString(const char* string)
{
    return DbgValFromString(string);
}


/************************************************************************************
                            Static Functions
************************************************************************************/
Bridge* Bridge::getBridge()
{
    return mBridge;
}


void Bridge::initBridge()
{
    mBridge = new Bridge();
}





/************************************************************************************
                            Exported Functions
************************************************************************************/

#ifdef BUILD_LIB

    __declspec(dllexport) int _gui_guiinit(int argc, char *argv[])
    {
        return main(argc, argv);
    }


    __declspec(dllexport) void _gui_disassembleat(duint va, duint eip)
    {
        Bridge::getBridge()->emitDisassembleAtSignal((int_t)va, (int_t)eip);
    }


    __declspec(dllexport) void _gui_setdebugstate(DBGSTATE state)
    {
        Bridge::getBridge()->emitDbgStateChanged(state);
    }


    __declspec(dllexport) void _gui_addlogmessage(const char* msg)
    {
       Bridge::getBridge()->emitAddMsgToLog(QString(msg));
    }


    __declspec(dllexport) void _gui_logclear()
    {
        Bridge::getBridge()->emitClearLog();
    }

    __declspec(dllexport) void _gui_updateregisterview()
    {
        Bridge::getBridge()->emitUpdateRegisters();
    }
#endif


/************************************************************************************
                            Imported Functions (Stub)
************************************************************************************/
#ifndef BUILD_LIB
    void stubReadProcessMemory(byte_t* dest, uint_t va, uint_t size)
    {
        uint_t wI;

        for(wI = 0; wI < size; wI++)
        {
            dest[wI] = Bridge::getBridge()->mData->data()[(va - Bridge::getBridge()->getBase(0)) + wI];
        }
    }
#endif






























