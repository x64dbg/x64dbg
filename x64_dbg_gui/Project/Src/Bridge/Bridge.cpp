#include "Bridge.h"
#include <QClipboard>
#include "QBeaEngine.h"
#include "main.h"
#include "Exports.h"

/************************************************************************************
                            Global Variables
************************************************************************************/
static Bridge* mBridge;

/************************************************************************************
                            Class Members
************************************************************************************/
Bridge::Bridge(QObject* parent) : QObject(parent)
{
    mBridgeMutex = new QMutex();
    winId = 0;
    scriptView = 0;
    referenceManager = 0;
    bridgeResult = 0;
    hasBridgeResult = false;
}

Bridge::~Bridge()
{
    delete mBridgeMutex;
}

void Bridge::CopyToClipboard(const QString & text)
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
}

void Bridge::BridgeSetResult(int_t result)
{
    bridgeResult = result;
    hasBridgeResult = true;
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

void Bridge::emitDumpAt(int_t va)
{
    emit dumpAt(va);
}

void Bridge::emitScriptAdd(int count, const char** lines)
{
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit scriptAdd(count, lines);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
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
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit scriptQuestion(message);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    return bridgeResult;
}

void Bridge::emitScriptEnableHighlighting(bool enable)
{
    emit scriptEnableHighlighting(enable);
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

void Bridge::emitReferenceSetSearchStartCol(int col)
{
    emit referenceSetSearchStartCol(col);
}

void Bridge::emitReferenceInitialize(QString name)
{
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit referenceInitialize(name);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
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

void Bridge::emitUpdateMemory()
{
    emit updateMemory();
}

void Bridge::emitAddRecentFile(QString file)
{
    emit addRecentFile(file);
}

void Bridge::emitSetLastException(unsigned int exceptionCode)
{
    emit setLastException(exceptionCode);
}

int Bridge::emitMenuAddMenu(int hMenu, QString title)
{
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit menuAddMenu(hMenu, title);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    return bridgeResult;
}

int Bridge::emitMenuAddMenuEntry(int hMenu, QString title)
{
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit menuAddMenuEntry(hMenu, title);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    return bridgeResult;
}

void Bridge::emitMenuAddSeparator(int hMenu)
{
    emit menuAddSeparator(hMenu);
}

void Bridge::emitMenuClearMenu(int hMenu)
{
    emit menuClearMenu(hMenu);
}

void Bridge::emitAddMsgToStatusBar(QString msg)
{
    emit addMsgToStatusBar(msg);
}

bool Bridge::emitSelectionGet(int hWindow, SELECTIONDATA* selection)
{
    if(!DbgIsDebugging())
        return false;
    mBridgeMutex->lock();
    hasBridgeResult = false;
    switch(hWindow)
    {
    case GUI_DISASSEMBLY:
        emit selectionDisasmGet(selection);
        break;
    case GUI_DUMP:
        emit selectionDumpGet(selection);
        break;
    case GUI_STACK:
        emit selectionStackGet(selection);
        break;
    default:
        mBridgeMutex->unlock();
        return false;
    }
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    if(selection->start > selection->end) //swap start and end
    {
        int_t temp = selection->end;
        selection->end = selection->start;
        selection->start = temp;
    }
    return true;
}

bool Bridge::emitSelectionSet(int hWindow, const SELECTIONDATA* selection)
{
    if(!DbgIsDebugging())
        return false;
    mBridgeMutex->lock();
    hasBridgeResult = false;
    switch(hWindow)
    {
    case GUI_DISASSEMBLY:
        emit selectionDisasmSet(selection);
        break;
    case GUI_DUMP:
        emit selectionDumpSet(selection);
        break;
    case GUI_STACK:
        emit selectionStackSet(selection);
        break;
    default:
        mBridgeMutex->unlock();
        return false;
    }
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    return bridgeResult;
}

bool Bridge::emitGetStrWindow(const QString title, QString* text)
{
    mBridgeMutex->lock();
    hasBridgeResult = false;
    emit getStrWindow(title, text);
    while(!hasBridgeResult) //wait for thread completion
        Sleep(100);
    mBridgeMutex->unlock();
    return bridgeResult;
}

void Bridge::emitAutoCompleteAddCmd(const QString cmd)
{
    emit autoCompleteAddCmd(cmd);
}

void Bridge::emitAutoCompleteDelCmd(const QString cmd)
{
    emit autoCompleteDelCmd(cmd);
}

void Bridge::emitAutoCompleteClearAll()
{
    emit autoCompleteClearAll();
}

void Bridge::emitUpdateSideBar()
{
    emit updateSideBar();
}

void Bridge::emitRepaintTableView()
{
    emit repaintTableView();
}

void Bridge::emitUpdatePatches()
{
    emit updatePatches();
}

void Bridge::emitUpdateCallStack()
{
    emit updateCallStack();
}

void Bridge::emitSymbolRefreshCurrent()
{
    emit symbolRefreshCurrent();
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
__declspec(dllexport) int _gui_guiinit(int argc, char* argv[])
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
        Bridge::getBridge()->emitDbgStateChanged(reinterpret_cast<DBGSTATE &>(param1));
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

    case GUI_SCRIPT_ENABLEHIGHLIGHTING:
    {
        Bridge::getBridge()->emitScriptEnableHighlighting((bool)(int_t)param1);
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
        return (void*)(void*)Bridge::getBridge()->referenceManager->currentReferenceView()->mList->getRowCount();
    }
    break;

    case GUI_REF_DELETEALLCOLUMNS:
    {
        GuiReferenceInitialize("References");
    }
    break;

    case GUI_REF_SETCELLCONTENT:
    {
        CELLINFO* info = (CELLINFO*)param1;
        Bridge::getBridge()->emitReferenceSetCellContent(info->row, info->col, QString(info->str));
    }
    break;

    case GUI_REF_GETCELLCONTENT:
    {
        return (void*)Bridge::getBridge()->referenceManager->currentReferenceView()->mList->getCellContent((int)(int_t)param1, (int)(int_t)param2).toUtf8().constData();
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

    case GUI_REF_SETSEARCHSTARTCOL:
    {
        Bridge::getBridge()->emitReferenceSetSearchStartCol((int)(int_t)param1);
    }
    break;

    case GUI_REF_INITIALIZE:
    {
        Bridge::getBridge()->emitReferenceInitialize(QString(reinterpret_cast<const char*>(param1)));
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

    case GUI_UPDATE_MEMORY_VIEW:
    {
        Bridge::getBridge()->emitUpdateMemory();
    }
    break;

    case GUI_ADD_RECENT_FILE:
    {
        Bridge::getBridge()->emitAddRecentFile(QString(reinterpret_cast<const char*>(param1)));
    }
    break;

    case GUI_SET_LAST_EXCEPTION:
    {
        Bridge::getBridge()->emitSetLastException((unsigned int)(uint_t)param1);
    }
    break;

    case GUI_GET_DISASSEMBLY:
    {
        uint_t parVA = (uint_t)param1;
        char* text = (char*)param2;
        if(!text || !parVA || !DbgIsDebugging())
            return 0;
        byte_t wBuffer[16];
        if(!DbgMemRead(parVA, wBuffer, 16))
            return 0;
        QBeaEngine disasm;
        Instruction_t instr = disasm.DisassembleAt(wBuffer, 16, 0, 0, parVA);
        BeaTokenizer::TokenizeInstruction(&instr.tokens, &instr.disasm);
        QList<RichTextPainter::CustomRichText_t> richText;
        BeaTokenizer::TokenToRichText(&instr.tokens, &richText, 0);
        QString finalInstruction = "";
        for(int i = 0; i < richText.size(); i++)
            finalInstruction += richText.at(i).text;
        strcpy_s(text, GUI_MAX_DISASSEMBLY_SIZE, finalInstruction.toUtf8().constData());
        return (void*)1;
    }
    break;

    case GUI_MENU_ADD:
    {
        return (void*)(uint_t)Bridge::getBridge()->emitMenuAddMenu((int)(uint_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_MENU_ADD_ENTRY:
    {
        return (void*)(uint_t)Bridge::getBridge()->emitMenuAddMenuEntry((int)(uint_t)param1, QString(reinterpret_cast<const char*>(param2)));
    }
    break;

    case GUI_MENU_ADD_SEPARATOR:
    {
        Bridge::getBridge()->emitMenuAddSeparator((int)(uint_t)param1);
    }
    break;

    case GUI_MENU_CLEAR:
    {
        Bridge::getBridge()->emitMenuClearMenu((int)(uint_t)param1);
    }
    break;

    case GUI_SELECTION_GET:
    {
        return (void*)(int_t)Bridge::getBridge()->emitSelectionGet((int)(uint_t)param1, (SELECTIONDATA*)param2);
    }
    break;

    case GUI_SELECTION_SET:
    {
        return (void*)(int_t)Bridge::getBridge()->emitSelectionSet((int)(uint_t)param1, (const SELECTIONDATA*)param2);
    }
    break;

    case GUI_GETLINE_WINDOW:
    {
        QString text = "";
        if(Bridge::getBridge()->emitGetStrWindow(QString(reinterpret_cast<const char*>(param1)), &text))
        {
            strcpy_s((char*)param2, GUI_MAX_LINE_SIZE, text.toUtf8().constData());
            return (void*)(uint_t)true;
        }
        return (void*)(uint_t)false; //cancel/escape
    }
    break;

    case GUI_AUTOCOMPLETE_ADDCMD:
    {
        Bridge::getBridge()->emitAutoCompleteAddCmd(QString((const char*)param1));
    }
    break;

    case GUI_AUTOCOMPLETE_DELCMD:
    {
        Bridge::getBridge()->emitAutoCompleteDelCmd(QString((const char*)param1));
    }
    break;

    case GUI_AUTOCOMPLETE_CLEARALL:
    {
        Bridge::getBridge()->emitAutoCompleteClearAll();
    }
    break;

    case GUI_ADD_MSG_TO_STATUSBAR:
    {
        Bridge::getBridge()->emitAddMsgToStatusBar(QString((const char*)param1));
    }
    break;

    case GUI_UPDATE_SIDEBAR:
    {
        Bridge::getBridge()->emitUpdateSideBar();
    }
    break;

    case GUI_REPAINT_TABLE_VIEW:
    {
        Bridge::getBridge()->emitRepaintTableView();
    }
    break;

    case GUI_UPDATE_PATCHES:
    {
        Bridge::getBridge()->emitUpdatePatches();
    }
    break;

    case GUI_UPDATE_CALLSTACK:
    {
        Bridge::getBridge()->emitUpdateCallStack();
    }
    break;

    case GUI_SYMBOL_REFRESH_CURRENT:
    {
        Bridge::getBridge()->emitSymbolRefreshCurrent();
    }
    break;

    default:
    {

    }
    break;
    }
    return 0;
}



