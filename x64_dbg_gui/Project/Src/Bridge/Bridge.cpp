#include "Bridge.h"

/************************************************************************************
                            Global Variables
************************************************************************************/
static Bridge* mBridge;

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

    case GUI_UPDATE_REGISTER_VIEW:
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






























