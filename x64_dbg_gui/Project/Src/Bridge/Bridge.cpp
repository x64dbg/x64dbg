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

void Bridge::emitUpdateDisassembly()
{
#ifdef BUILD_LIB
    emit repaintGui();
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
        //Bridge::getBridge()->emitDisassembleAtSignal((int_t)va, (int_t)eip);
        _gui_sendmessage(GUI_DISASSEMBLE_AT, (void*)va, (void*)eip);
    }

    __declspec(dllexport) void _gui_updatedisassemblyview()
    {
        //Bridge::getBridge()->emitUpdateDisassembly();
        _gui_sendmessage(GUI_UPDATE_DISASSEMBLY_VIEW, (void*)0, (void*)0);
    }


    __declspec(dllexport) void _gui_setdebugstate(DBGSTATE state)
    {
        //Bridge::getBridge()->emitDbgStateChanged(state);
        _gui_sendmessage(GUI_SET_DEBUG_STATE, (void*)state, (void*)0);
    }


    __declspec(dllexport) void _gui_addlogmessage(const char* msg)
    {
       //Bridge::getBridge()->emitAddMsgToLog(QString(msg));
       _gui_sendmessage(GUI_ADD_MSG_TO_LOG, (void*)msg, (void*)0);
    }


    __declspec(dllexport) void _gui_logclear()
    {
        //Bridge::getBridge()->emitClearLog();
        _gui_sendmessage(GUI_CLEAR_LOG, (void*)0, (void*)0);
    }

    __declspec(dllexport) void _gui_updateregisterview()
    {
        //Bridge::getBridge()->emitUpdateRegisters();
        _gui_sendmessage(GUI_UPDATE_REGISTER, (void*)0, (void*)0);
    }

    __declspec(dllexport) void _gui_sendmessage(MSGTYPE type, void* param1, void* param2)
    {
        switch(type)
        {
            case GUI_DISASSEMBLE_AT:
            {
                Bridge::getBridge()->emitDisassembleAtSignal((int_t)param1, (int_t)param2);
            }
            break;
            case GUI_SET_DEBUG_STATE:
            {
                Bridge::getBridge()->emitDbgStateChanged(reinterpret_cast<DBGSTATE&>(param1));
            }
            break;
            case GUI_ADD_MSG_TO_LOG:
            {
                Bridge::getBridge()->emitAddMsgToLog(QString(reinterpret_cast<const char*>(param1)));
            }
            break;
            case GUI_CLEAR_LOG:
            {
                Bridge::getBridge()->emitClearLog();
            }
            break;
            case GUI_UPDATE_REGISTER:
            {
                Bridge::getBridge()->emitUpdateRegisters();
            }
            break;
            case GUI_UPDATE_DISASSEMBLY_VIEW:
            {
                Bridge::getBridge()->emitUpdateDisassembly();
            }
            break;
            default:
            {

            }
            break;
        }
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






























