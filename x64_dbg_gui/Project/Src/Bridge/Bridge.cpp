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

void Bridge::CopyToClipboard(const char* text)
{
    HGLOBAL hText;
    char *pText;
    int len=strlen(text);
    if(!len)
        return;

    hText=GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE, len+1);
    pText=(char*)GlobalLock(hText);
    strcpy(pText, text);

    OpenClipboard(0);
    EmptyClipboard();
    if(!SetClipboardData(CF_OEMTEXT, hText))
        MessageBeep(MB_ICONERROR);
    else
        MessageBeep(MB_ICONINFORMATION);
    CloseClipboard();
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

void Bridge::emitScriptAdd(int count, const char** lines)
{
    mBridgeMutex.lock();
    scriptResult=-1;
    emit scriptAdd(count, lines);
    while(scriptResult==-1) //wait for thread completion
        Sleep(100);
    mBridgeMutex.unlock();
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

int Bridge::emitScriptQuestion(QString message)
{
    mBridgeMutex.lock();
    scriptResult=-1;
    emit scriptQuestion(message);
    while(scriptResult==-1) //wait for thread completion
        Sleep(100);
    mBridgeMutex.unlock();
    return scriptResult;
}

void Bridge::emitUpdateSymbolList(int module_count, SYMBOLMODULEINFO* modules)
{
    emit updateSymbolList(module_count, modules);
}

void Bridge::emitAddMsgToSymbolLog(QString msg)
{
    emit addMsgToSymbolLog(msg);
}

void Bridge::emitClearSymbolLog()
{
    emit clearSymbolLog();
}

void Bridge::emitSetSymbolProgress(int progress)
{
    emit setSymbolProgress(progress);
}

void Bridge::emitReferenceAddColumnAt(int width, QString title)
{
    emit referenceAddColumnAt(width, title);
}

void Bridge::emitReferenceSetRowCount(int_t count)
{
    emit referenceSetRowCount(count);
}

void Bridge::emitReferenceDeleteAllColumns()
{
    emit referenceDeleteAllColumns();
}

void Bridge::emitReferenceSetCellContent(int r, int c, QString s)
{
    emit referenceSetCellContent(r, c, s);
}

void Bridge::emitReferenceReloadData()
{
    emit referenceReloadData();
}

void Bridge::emitReferenceSetSingleSelection(int index, bool scroll)
{
    emit referenceSetSingleSelection(index, scroll);
}

void Bridge::emitReferenceSetProgress(int progress)
{
    emit referenceSetProgress(progress);
}

void Bridge::emitStackDumpAt(uint_t va, uint_t csp)
{
    emit stackDumpAt(va, csp);
}

void Bridge::emitUpdateDump()
{
    emit updateDump();
}

void Bridge::emitUpdateThreads()
{
    emit updateThreads();
}

void Bridge::emitAddRecentFile(QString file)
{
    emit addRecentFile(file);
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

    case GUI_SCRIPT_ADD:
    {
        Bridge::getBridge()->emitScriptAdd((int)(int_t)param1, reinterpret_cast<const char**>(param2));
    }
    break;

    case GUI_SCRIPT_CLEAR:
    {
        Bridge::getBridge()->emitScriptClear();
    }
    break;

    case GUI_SCRIPT_SETIP:
    {
        Bridge::getBridge()->emitScriptSetIp((int)(int_t)param1);
    }
    break;

    case GUI_SCRIPT_ERROR:
    {
        Bridge::getBridge()->emitScriptError((int)(int_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_SCRIPT_SETTITLE:
    {
        Bridge::getBridge()->emitScriptSetTitle(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SCRIPT_SETINFOLINE:
    {
        Bridge::getBridge()->emitScriptSetInfoLine((int)(int_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_SCRIPT_MESSAGE:
    {
        Bridge::getBridge()->emitScriptMessage(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SCRIPT_MSGYN:
    {
        return (void*)Bridge::getBridge()->emitScriptQuestion(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SYMBOL_UPDATE_MODULE_LIST:
    {
        Bridge::getBridge()->emitUpdateSymbolList((int)(int_t)param1, (SYMBOLMODULEINFO*)param2);
    }
    break;

    case GUI_SYMBOL_LOG_ADD:
    {
        Bridge::getBridge()->emitAddMsgToSymbolLog(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SYMBOL_LOG_CLEAR:
    {
        Bridge::getBridge()->emitClearSymbolLog();
    }
    break;

    case GUI_SYMBOL_SET_PROGRESS:
    {
        Bridge::getBridge()->emitSetSymbolProgress((int)(int_t)param1);
    }
    break;

    case GUI_REF_ADDCOLUMN:
    {
        Bridge::getBridge()->emitReferenceAddColumnAt((int)(int_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_REF_SETROWCOUNT:
    {
        Bridge::getBridge()->emitReferenceSetRowCount((int)(int_t)param1);
    }
    break;

    case GUI_REF_GETROWCOUNT:
    {
        return (void*)Bridge::getBridge()->referenceView->mList->getRowCount();
    }
    break;

    case GUI_REF_DELETEALLCOLUMNS:
    {
        Bridge::getBridge()->emitReferenceDeleteAllColumns();
    }
    break;

    case GUI_REF_SETCELLCONTENT:
    {
        CELLINFO* info=(CELLINFO*)param1;
        Bridge::getBridge()->emitReferenceSetCellContent(info->row, info->col, QString(info->str));
    }
    break;

    case GUI_REF_GETCELLCONTENT:
    {
        return (void*)Bridge::getBridge()->referenceView->mList->getCellContent((int)(int_t)param1, (int)(int_t)param2).toUtf8().constData();
    }
    break;

    case GUI_REF_RELOADDATA:
    {
        Bridge::getBridge()->emitReferenceReloadData();
    }
    break;

    case GUI_REF_SETSINGLESELECTION:
    {
        Bridge::getBridge()->emitReferenceSetSingleSelection((int)(int_t)param1, (bool)(int_t)param2);
    }
    break;

    case GUI_REF_SETPROGRESS:
    {
        Bridge::getBridge()->emitReferenceSetProgress((int)(int_t)param1);
    }
    break;

    case GUI_STACK_DUMP_AT:
    {
        Bridge::getBridge()->emitStackDumpAt((uint_t)param1, (uint_t)param2);
    }
    break;

    case GUI_UPDATE_DUMP_VIEW:
    {
        Bridge::getBridge()->emitUpdateDump();
    }
    break;

    case GUI_UPDATE_THREAD_VIEW:
    {
        Bridge::getBridge()->emitUpdateThreads();
    }
    break;

    case GUI_ADD_RECENT_FILE:
    {
        Bridge::getBridge()->emitAddRecentFile(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    default:
    {
    }
    break;
    }
    return 0;
}



