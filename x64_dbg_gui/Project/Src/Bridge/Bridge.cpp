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

}


/************************************************************************************
                            Exports Binding
************************************************************************************/
void Bridge::emitDisassembleAtSignal(int_t va, int_t eip)
{
    emit disassembleAt(va, eip);
}

void Bridge::emitUpdateDisassembly()
{
    emit repaintGui();
}

void Bridge::emitDbgStateChanged(DBGSTATE state)
{
    emit dbgStateChanged(state);
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

void Bridge::emitUpdateBreakpoints()
{
    emit updateBreakpoints();
}

void Bridge::emitUpdateWindowTitle(QString filename)
{
    emit updateWindowTitle(filename);
}

void Bridge::emitUpdateCPUTitle(QString modname)
{
    emit updateCPUTitle(modname);
}

void Bridge::emitSetInfoLine(int line, QString text)
{
    emit setInfoLine(line, text);
}

void Bridge::emitClearInfoBox()
{
    emit setInfoLine(0, "");
    emit setInfoLine(1, "");
    emit setInfoLine(2, "");
}

void Bridge::emitDumpAt(int_t va)
{
    emit dumpAt(va);
}

void Bridge::emitScriptAddLine(QString text)
{
    emit scriptAddLine(text);
}

void Bridge::emitScriptClear()
{
    emit scriptClear();
}

void Bridge::emitScriptSetIp(int line)
{
    emit scriptSetIp(line);
}

void Bridge::emitScriptError(int line, QString message)
{
    emit scriptError(line, message);
}

void Bridge::emitScriptSetTitle(QString title)
{
    emit scriptSetTitle(title);
}

void Bridge::emitScriptSetInfoLine(int line, QString info)
{
    emit scriptSetInfoLine(line, info);
}

void Bridge::emitScriptMessage(QString message)
{
    emit scriptMessage(message);
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
__declspec(dllexport) int _gui_guiinit(int argc, char *argv[])
{
    return main(argc, argv);
}

__declspec(dllexport) void* _gui_sendmessage(GUIMSG type, void* param1, void* param2)
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

    case GUI_UPDATE_BREAKPOINTS_VIEW:
    {
        Bridge::getBridge()->emitUpdateBreakpoints();
    }
    break;

    case GUI_UPDATE_WINDOW_TITLE:
    {
        Bridge::getBridge()->emitUpdateWindowTitle(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_UPDATE_CPU_TITLE:
    {
        Bridge::getBridge()->emitUpdateCPUTitle(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SET_INFO_LINE:
    {
        Bridge::getBridge()->emitSetInfoLine((int)(int_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_GET_WINDOW_HANDLE:
    {
        return Bridge::getBridge()->winId;
    }
    break;

    case GUI_DUMP_AT:
    {
        Bridge::getBridge()->emitDumpAt((int_t)param1);
    }
    break;

    case GUI_SCRIPT_ADDLINE:
    {
        Bridge::getBridge()->emitScriptAddLine(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SCRIPT_CLEAR:
    {
        Bridge::getBridge()->emitScriptClear();
    }
    break;

    case GUI_SCRIPT_SETIP:
    {
        int_t arg=(int_t)param1;
        Bridge::getBridge()->emitScriptSetIp((int)arg);
    }
    break;

    case GUI_SCRIPT_ERROR:
    {
        int_t arg=(int_t)param1;
        Bridge::getBridge()->emitScriptError((int)arg, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_SCRIPT_SETTITLE:
    {
        Bridge::getBridge()->emitScriptSetTitle(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SCRIPT_SETINFOLINE:
    {
        int_t arg=(int_t)param1;
        Bridge::getBridge()->emitScriptSetInfoLine((int)arg, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_SCRIPT_MESSAGE:
    {
        Bridge::getBridge()->emitScriptMessage(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    default:
    {
    }
    break;
    }
    return 0;
}



