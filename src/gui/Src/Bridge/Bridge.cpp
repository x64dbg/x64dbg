#include "Bridge.h"
#include <QClipboard>
#include "QBeaEngine.h"
#include "main.h"
#include "Exports.h"

#include "ReferenceManager.h"
#include "SymbolView.h"

/************************************************************************************
                            Global Variables
************************************************************************************/
static Bridge* mBridge;

/************************************************************************************
                            Class Members
************************************************************************************/
Bridge::Bridge(QObject* parent) : QObject(parent)
{
    InitializeCriticalSection(&csBridge);
    for(size_t i = 0; i < BridgeResult::Last; i++)
        resultEvents[i] = CreateEventW(nullptr, true, true, nullptr);
    dwMainThreadId = GetCurrentThreadId();
}

Bridge::~Bridge()
{
    EnterCriticalSection(&csBridge);
    for(size_t i = 0; i < BridgeResult::Last; i++)
        CloseHandle(resultEvents[i]);
    DeleteCriticalSection(&csBridge);
}

void Bridge::CopyToClipboard(const QString & text)
{
    if(!text.length())
        return;
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(text);
    GuiAddStatusBarMessage(tr("The data has been copied to clipboard.\n").toUtf8().constData());
}

void Bridge::CopyToClipboard(const QString & text, const QString & htmlText)
{
    QMimeData* mimeData = new QMimeData();
    mimeData->setData("text/html", htmlText.toUtf8()); // Set text/html data
    mimeData->setData("text/plain", text.toUtf8());  //Set text/plain data
    //Reason not using setText() or setHtml():Don't support storing multiple data in one QMimeData
    QApplication::clipboard()->setMimeData(mimeData); //Copy the QMimeData with text and html data
    GuiAddStatusBarMessage(tr("The data has been copied to clipboard.\n").toUtf8().constData());
}

void Bridge::setResult(BridgeResult::Type type, dsint result)
{
#ifdef DEBUG
    OutputDebugStringA(QString().sprintf("[x64dbg] [%u] Bridge::setResult(%d, %p)\n", GetCurrentThreadId(), type, result).toUtf8().constData());
#endif //DEBUG
    bridgeResults[type] = result;
    SetEvent(resultEvents[type]);
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
                            Helper Functions
************************************************************************************/

void Bridge::emitMenuAddToList(QWidget* parent, QMenu* menu, GUIMENUTYPE hMenu, int hParentMenu)
{
    BridgeResult result(BridgeResult::MenuAddToList);
    emit menuAddMenuToList(parent, menu, hMenu, hParentMenu);
    result.Wait();
}

void Bridge::setDbgStopped()
{
    dbgStopped = true;
}

/************************************************************************************
                            Message processing
************************************************************************************/

void* Bridge::processMessage(GUIMSG type, void* param1, void* param2)
{
    if(dbgStopped) //there can be no more messages if the debugger stopped = IGNORE
        return nullptr;
    switch(type)
    {
    case GUI_DISASSEMBLE_AT:
        mLastCip = (duint)param2;
        emit disassembleAt((dsint)param1, (dsint)param2);
        break;

    case GUI_SET_DEBUG_STATE:
        mIsRunning = DBGSTATE(duint(param1)) == running;
        if(!param2)
            emit dbgStateChanged((DBGSTATE)(dsint)param1);
        break;

    case GUI_ADD_MSG_TO_LOG:
    {
        auto msg = (const char*)param1;
        emit addMsgToLog(QByteArray(msg, int(strlen(msg)) + 1)); //Speed up performance: don't convert to UCS-2 QString
    }
    break;

    case GUI_ADD_MSG_TO_LOG_HTML:
    {
        auto msg = (const char*)param1;
        emit addMsgToLogHtml(QByteArray(msg, int(strlen(msg)) + 1)); //Speed up performance: don't convert to UCS-2 QString
    }
    break;

    case GUI_CLEAR_LOG:
        emit clearLog();
        break;

    case GUI_UPDATE_REGISTER_VIEW:
        emit updateRegisters();
        break;

    case GUI_UPDATE_DISASSEMBLY_VIEW:
        emit updateDisassembly();
        break;

    case GUI_UPDATE_BREAKPOINTS_VIEW:
        emit updateBreakpoints();
        break;

    case GUI_UPDATE_WINDOW_TITLE:
        emit updateWindowTitle(QString((const char*)param1));
        break;

    case GUI_GET_WINDOW_HANDLE:
        return winId;

    case GUI_DUMP_AT:
        emit dumpAt((dsint)param1);
        break;

    case GUI_SCRIPT_ADD:
    {
        BridgeResult result(BridgeResult::ScriptAdd);
        emit scriptAdd((int)param1, (const char**)param2);
        result.Wait();
    }
    break;

    case GUI_SCRIPT_CLEAR:
        emit scriptClear();
        break;

    case GUI_SCRIPT_SETIP:
        emit scriptSetIp((int)param1);
        break;

    case GUI_SCRIPT_ERROR:
    {
        BridgeResult result(BridgeResult::ScriptMessage);
        emit scriptError((int)param1, QString((const char*)param2));
        result.Wait();
    }
    break;

    case GUI_SCRIPT_SETTITLE:
        emit scriptSetTitle(QString((const char*)param1));
        break;

    case GUI_SCRIPT_SETINFOLINE:
        emit scriptSetInfoLine((int)param1, QString((const char*)param2));
        break;

    case GUI_SCRIPT_MESSAGE:
    {
        BridgeResult result(BridgeResult::ScriptMessage);
        emit scriptMessage(QString((const char*)param1));
        result.Wait();
    }
    break;

    case GUI_SCRIPT_MSGYN:
    {
        BridgeResult result(BridgeResult::ScriptMessage);
        emit scriptQuestion(QString((const char*)param1));
        return (void*)result.Wait();
    }
    break;

    case GUI_SCRIPT_ENABLEHIGHLIGHTING:
        emit scriptEnableHighlighting((bool)param1);
        break;

    case GUI_SYMBOL_UPDATE_MODULE_LIST:
        emit updateSymbolList((int)param1, (SYMBOLMODULEINFO*)param2);
        break;

    case GUI_SYMBOL_LOG_ADD:
        emit addMsgToSymbolLog(QString((const char*)param1));
        break;

    case GUI_SYMBOL_LOG_CLEAR:
        emit clearSymbolLog();
        break;

    case GUI_SYMBOL_SET_PROGRESS:
        emit setSymbolProgress((int)param1);
        break;

    case GUI_REF_ADDCOLUMN:
        if(referenceManager->currentReferenceView())
            referenceManager->currentReferenceView()->addColumnAtRef((int)param1, QString((const char*)param2));
        break;

    case GUI_REF_SETROWCOUNT:
    {
        if(referenceManager->currentReferenceView())
            referenceManager->currentReferenceView()->setRowCount((dsint)param1);
    }
    break;

    case GUI_REF_GETROWCOUNT:
        if(referenceManager->currentReferenceView())
            return (void*)referenceManager->currentReferenceView()->stdList()->getRowCount();
        return 0;

    case GUI_REF_SEARCH_GETROWCOUNT:
        if(referenceManager->currentReferenceView())
            return (void*)referenceManager->currentReferenceView()->mCurList->getRowCount();
        return 0;

    case GUI_REF_DELETEALLCOLUMNS:
        GuiReferenceInitialize(tr("References").toUtf8().constData());
        break;

    case GUI_REF_SETCELLCONTENT:
    {
        CELLINFO* info = (CELLINFO*)param1;
        if(referenceManager->currentReferenceView())
            referenceManager->currentReferenceView()->setCellContent(info->row, info->col, QString(info->str));
    }
    break;

    case GUI_REF_GETCELLCONTENT:
    {
        QString content;
        if(referenceManager->currentReferenceView())
            content = referenceManager->currentReferenceView()->stdList()->getCellContent((int)param1, (int)param2);
        auto bytes = content.toUtf8();
        auto data = BridgeAlloc(bytes.size() + 1);
        memcpy(data, bytes.constData(), bytes.size());
        return data;
    }

    case GUI_REF_SEARCH_GETCELLCONTENT:
    {
        QString content;
        if(referenceManager->currentReferenceView())
            content = referenceManager->currentReferenceView()->mCurList->getCellContent((int)param1, (int)param2);
        auto bytes = content.toUtf8();
        auto data = BridgeAlloc(bytes.size() + 1);
        memcpy(data, bytes.constData(), bytes.size());
        return data;
    }

    case GUI_REF_RELOADDATA:
        emit referenceReloadData();
        break;

    case GUI_REF_SETSINGLESELECTION:
        emit referenceSetSingleSelection((int)param1, (bool)param2);
        break;

    case GUI_REF_SETPROGRESS:
        if(referenceManager->currentReferenceView())
        {
            auto newProgress = (int)param1;
            if(referenceManager->currentReferenceView()->progress() != newProgress)
                emit referenceSetProgress(newProgress);
        }
        break;

    case GUI_REF_SETCURRENTTASKPROGRESS:
        if(referenceManager->currentReferenceView())
        {
            auto newProgress = (int)param1;
            if(referenceManager->currentReferenceView()->currentTaskProgress() != newProgress)
                emit referenceSetCurrentTaskProgress((int)param1, QString((const char*)param2));
        }
        break;

    case GUI_REF_SETSEARCHSTARTCOL:
        if(referenceManager->currentReferenceView())
            referenceManager->currentReferenceView()->setSearchStartCol((int)param1);
        break;

    case GUI_REF_INITIALIZE:
    {
        BridgeResult result(BridgeResult::RefInitialize);
        emit referenceInitialize(QString((const char*)param1));
        result.Wait();
    }
    break;

    case GUI_STACK_DUMP_AT:
        emit stackDumpAt((duint)param1, (duint)param2);
        break;

    case GUI_UPDATE_DUMP_VIEW:
        emit updateDump();
        break;

    case GUI_UPDATE_THREAD_VIEW:
        emit updateThreads();
        break;

    case GUI_UPDATE_MEMORY_VIEW:
        emit updateMemory();
        break;

    case GUI_ADD_RECENT_FILE:
        emit addRecentFile(QString((const char*)param1));
        break;

    case GUI_SET_LAST_EXCEPTION:
        emit setLastException((unsigned int)param1);
        break;

    case GUI_GET_DISASSEMBLY:
    {
        duint parVA = (duint)param1;
        char* text = (char*)param2;
        if(!text || !parVA || !DbgIsDebugging())
            return 0;
        byte_t wBuffer[16];
        if(!DbgMemRead(parVA, wBuffer, 16))
            return 0;
        QBeaEngine disasm(int(ConfigUint("Disassembler", "MaxModuleSize")));
        Instruction_t instr = disasm.DisassembleAt(wBuffer, 16, 0, parVA);
        QString finalInstruction;
        for(const auto & curToken : instr.tokens.tokens)
            finalInstruction += curToken.text;
        strncpy_s(text, GUI_MAX_DISASSEMBLY_SIZE, finalInstruction.toUtf8().constData(), _TRUNCATE);
        return (void*)1;
    }
    break;

    case GUI_MENU_ADD:
    {
        BridgeResult result(BridgeResult::MenuAdd);
        emit menuAddMenu((int)param1, QString((const char*)param2));
        return (void*)result.Wait();
    }
    break;

    case GUI_MENU_ADD_ENTRY:
    {
        BridgeResult result(BridgeResult::MenuAddEntry);
        emit menuAddMenuEntry((int)param1, QString((const char*)param2));
        return (void*)result.Wait();
    }
    break;

    case GUI_MENU_ADD_SEPARATOR:
    {
        BridgeResult result(BridgeResult::MenuAddSeparator);
        emit menuAddSeparator((int)param1);
        result.Wait();
    }
    break;

    case GUI_MENU_CLEAR:
    {
        BridgeResult result(BridgeResult::MenuClear);
        emit menuClearMenu((int)param1, false);
        result.Wait();
    }
    break;

    case GUI_MENU_REMOVE:
    {
        BridgeResult result(BridgeResult::MenuRemove);
        emit menuRemoveMenuEntry((int)param1);
        result.Wait();
    }
    break;

    case GUI_SELECTION_GET:
    {
        GUISELECTIONTYPE hWindow = GUISELECTIONTYPE(duint(param1));
        SELECTIONDATA* selection = (SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        BridgeResult result(BridgeResult::SelectionGet);
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
        case GUI_GRAPH:
            emit selectionGraphGet(selection);
            break;
        case GUI_MEMMAP:
            emit selectionMemmapGet(selection);
            break;
        case GUI_SYMMOD:
            emit selectionSymmodGet(selection);
            break;
        default:
            return (void*)false;
        }
        result.Wait();
        if(selection->start > selection->end) //swap start and end
        {
            dsint temp = selection->end;
            selection->end = selection->start;
            selection->start = temp;
        }
        return (void*)true;
    }
    break;

    case GUI_SELECTION_SET:
    {
        GUISELECTIONTYPE hWindow = GUISELECTIONTYPE(duint(param1));
        const SELECTIONDATA* selection = (const SELECTIONDATA*)param2;
        if(!DbgIsDebugging())
            return (void*)false;
        BridgeResult result(BridgeResult::SelectionSet);
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
            return (void*)false;
        }
        return (void*)result.Wait();
    }
    break;

    case GUI_GETLINE_WINDOW:
    {
        QString text = "";
        BridgeResult result(BridgeResult::GetlineWindow);
        emit getStrWindow(QString((const char*)param1), &text);
        if(result.Wait())
        {
            strcpy_s((char*)param2, GUI_MAX_LINE_SIZE, text.toUtf8().constData());
            return (void*)true;
        }
        return (void*)false; //cancel/escape
    }
    break;

    case GUI_AUTOCOMPLETE_ADDCMD:
        emit autoCompleteAddCmd(QString((const char*)param1));
        break;

    case GUI_AUTOCOMPLETE_DELCMD:
        emit autoCompleteDelCmd(QString((const char*)param1));
        break;

    case GUI_AUTOCOMPLETE_CLEARALL:
        emit autoCompleteClearAll();
        break;

    case GUI_ADD_MSG_TO_STATUSBAR:
        emit addMsgToStatusBar(QString((const char*)param1));
        break;

    case GUI_UPDATE_SIDEBAR:
        emit updateSideBar();
        break;

    case GUI_REPAINT_TABLE_VIEW:
        emit repaintTableView();
        break;

    case GUI_UPDATE_PATCHES:
        emit updatePatches();
        break;

    case GUI_UPDATE_CALLSTACK:
        emit updateCallStack();
        break;

    case GUI_UPDATE_SEHCHAIN:
        emit updateSEHChain();
        break;

    case GUI_SYMBOL_REFRESH_CURRENT:
        emit symbolRefreshCurrent();
        break;

    case GUI_LOAD_SOURCE_FILE:
        emit loadSourceFile(QString((const char*)param1), (duint)param2);
        break;

    case GUI_MENU_SET_ICON:
    {
        int hMenu = (int)param1;
        const ICONDATA* icon = (const ICONDATA*)param2;
        BridgeResult result(BridgeResult::MenuSetIcon);
        if(!icon)
            emit setIconMenu(hMenu, QIcon());
        else
        {
            QImage img;
            img.loadFromData((uchar*)icon->data, icon->size);
            QIcon qIcon(QPixmap::fromImage(img));
            emit setIconMenu(hMenu, qIcon);
        }
        result.Wait();
    }
    break;

    case GUI_MENU_SET_ENTRY_ICON:
    {
        int hEntry = (int)param1;
        const ICONDATA* icon = (const ICONDATA*)param2;
        BridgeResult result(BridgeResult::MenuSetEntryIcon);
        if(!icon)
            emit setIconMenuEntry(hEntry, QIcon());
        else
        {
            QImage img;
            img.loadFromData((uchar*)icon->data, icon->size);
            QIcon qIcon(QPixmap::fromImage(img));
            emit setIconMenuEntry(hEntry, qIcon);
        }
        result.Wait();
    }
    break;

    case GUI_MENU_SET_ENTRY_CHECKED:
    {
        BridgeResult result(BridgeResult::MenuSetEntryChecked);
        emit setCheckedMenuEntry(int(param1), bool(param2));
        result.Wait();
    }
    break;

    case GUI_MENU_SET_VISIBLE:
    {
        BridgeResult result(BridgeResult::MenuSetVisible);
        emit setVisibleMenu(int(param1), bool(param2));
        result.Wait();
    }
    break;

    case GUI_MENU_SET_ENTRY_VISIBLE:
    {
        BridgeResult result(BridgeResult::MenuSetEntryVisible);
        emit setVisibleMenuEntry(int(param1), bool(param2));
        result.Wait();
    }
    break;

    case GUI_MENU_SET_NAME:
    {
        BridgeResult result(BridgeResult::MenuSetName);
        emit setNameMenu(int(param1), QString((const char*)param2));
        result.Wait();
    }
    break;

    case GUI_MENU_SET_ENTRY_NAME:
    {
        BridgeResult result(BridgeResult::MenuSetEntryName);
        emit setNameMenuEntry(int(param1), QString((const char*)param2));
        result.Wait();
    }
    break;

    case GUI_SHOW_CPU:
        emit showCpu();
        break;

    case GUI_ADD_QWIDGET_TAB:
        emit addQWidgetTab((QWidget*)param1);
        break;

    case GUI_SHOW_QWIDGET_TAB:
        emit showQWidgetTab((QWidget*)param1);
        break;

    case GUI_CLOSE_QWIDGET_TAB:
        emit closeQWidgetTab((QWidget*)param1);
        break;

    case GUI_EXECUTE_ON_GUI_THREAD:
    {
        if(GetCurrentThreadId() == dwMainThreadId)
            ((GUICALLBACKEX)param1)(param2);
        else
            emit executeOnGuiThread(param1, param2);
    }
    break;

    case GUI_UPDATE_TIME_WASTED_COUNTER:
        emit updateTimeWastedCounter();
        break;

    case GUI_SET_GLOBAL_NOTES:
    {
        QString text = QString((const char*)param1);
        emit setGlobalNotes(text);
    }
    break;

    case GUI_GET_GLOBAL_NOTES:
    {
        BridgeResult result(BridgeResult::GetGlobalNotes);
        emit getGlobalNotes(param1);
        result.Wait();
    }
    break;

    case GUI_SET_DEBUGGEE_NOTES:
    {
        QString text = QString((const char*)param1);
        emit setDebuggeeNotes(text);
    }
    break;

    case GUI_GET_DEBUGGEE_NOTES:
    {
        BridgeResult result(BridgeResult::GetDebuggeeNotes);
        emit getDebuggeeNotes(param1);
        result.Wait();
    }
    break;

    case GUI_DUMP_AT_N:
        emit dumpAtN((duint)param1, (int)param2);
        break;

    case GUI_DISPLAY_WARNING:
    {
        QString title = QString((const char*)param1);
        QString text = QString((const char*)param2);
        emit displayWarning(title, text);
    }
    break;

    case GUI_REGISTER_SCRIPT_LANG:
    {
        BridgeResult result(BridgeResult::RegisterScriptLang);
        emit registerScriptLang((SCRIPTTYPEINFO*)param1);
        result.Wait();
    }
    break;

    case GUI_UNREGISTER_SCRIPT_LANG:
        emit unregisterScriptLang((int)param1);
        break;

    case GUI_UPDATE_ARGUMENT_VIEW:
        emit updateArgumentView();
        break;

    case GUI_FOCUS_VIEW:
    {
        int hWindow = int(param1);
        switch(hWindow)
        {
        case GUI_DISASSEMBLY:
            emit focusDisasm();
            break;
        case GUI_DUMP:
            emit focusDump();
            break;
        case GUI_STACK:
            emit focusStack();
            break;
        case GUI_GRAPH:
            emit focusGraph();
            break;
        case GUI_MEMMAP:
            emit focusMemmap();
            break;
        default:
            break;
        }
    }
    break;

    case GUI_UPDATE_WATCH_VIEW:
        emit updateWatch();
        break;

    case GUI_LOAD_GRAPH:
    {
        BridgeResult result(BridgeResult::LoadGraph);
        emit loadGraph((BridgeCFGraphList*)param1, duint(param2));
        return (void*)result.Wait();
    }
    break;

    case GUI_GRAPH_AT:
    {
        BridgeResult result(BridgeResult::GraphAt);
        emit graphAt(duint(param1));
        return (void*)result.Wait();
    }
    break;

    case GUI_UPDATE_GRAPH_VIEW:
        emit updateGraph();
        break;

    case GUI_SET_LOG_ENABLED:
        emit setLogEnabled(param1 != 0);
        break;

    case GUI_ADD_FAVOURITE_TOOL:
    {
        QString name;
        QString description;
        if(param1 == nullptr)
            return nullptr;
        name = QString((const char*)param1);
        if(param2 != nullptr)
            description = QString((const char*)param2);
        emit addFavouriteItem(0, name, description);
    }
    break;

    case GUI_ADD_FAVOURITE_COMMAND:
    {
        QString name;
        QString shortcut;
        if(param1 == nullptr)
            return nullptr;
        name = QString((const char*)param1);
        if(param2 != nullptr)
            shortcut = QString((const char*)param2);
        emit addFavouriteItem(2, name, shortcut);
    }
    break;

	case GUI_SET_FAVOURITE_COMMAND_SHORTCUT:
	{
		QString name;
		QString shortcut;
		if (param1 == nullptr)
			return nullptr;
		name = QString((const char*)param1);
		if (param2 != nullptr)
			shortcut = QString((const char*)param2);
		emit setFavouriteItemShortcut(2, name, shortcut);
	}
	break;

    case GUI_SET_FAVOURITE_TOOL_SHORTCUT:
    {
        QString name;
        QString shortcut;
        if(param1 == nullptr)
            return nullptr;
        name = QString((const char*)param1);
        if(param2 != nullptr)
            shortcut = QString((const char*)param2);
        emit setFavouriteItemShortcut(0, name, shortcut);
    }
    break;

    case GUI_FOLD_DISASSEMBLY:
        emit foldDisassembly(duint(param1), duint(param2));
        break;

    case GUI_SELECT_IN_MEMORY_MAP:
        emit selectInMemoryMap(duint(param1));
        break;

    case GUI_GET_ACTIVE_VIEW:
    {
        if(param1)
        {
            BridgeResult result(BridgeResult::GetActiveView);
            emit getActiveView((ACTIVEVIEW*)param1);
            result.Wait();
        }
    }
    break;

    case GUI_ADD_INFO_LINE:
    {
        if(param1)
        {
            emit addInfoLine(QString((const char*)param1));
        }
    }
    break;

    case GUI_PROCESS_EVENTS:
        QCoreApplication::processEvents();
        break;

    case GUI_TYPE_ADDNODE:
    {
        BridgeResult result(BridgeResult::TypeAddNode);
        emit typeAddNode(param1, (const TYPEDESCRIPTOR*)param2);
        return (void*)result.Wait();
    }
    break;

    case GUI_TYPE_CLEAR:
    {
        BridgeResult result(BridgeResult::TypeClear);
        emit typeClear();
        result.Wait();
    }
    break;

    case GUI_UPDATE_TYPE_WIDGET:
        emit typeUpdateWidget();
        break;

    case GUI_CLOSE_APPLICATION:
        emit closeApplication();
        break;

    case GUI_FLUSH_LOG:
        emit flushLog();
        break;

    case GUI_MENU_SET_ENTRY_HOTKEY:
    {
        BridgeResult result(BridgeResult::MenuSetEntryHotkey);
        auto params = QString((const char*)param2).split('\1');
        if(params.length() == 2)
        {
            emit setHotkeyMenuEntry(int(param1), params[0], params[1]);
            result.Wait();
        }
    }
    break;

    case GUI_REF_ADDCOMMAND:
    {
        if(param1 == nullptr && param2 == nullptr)
            return nullptr;
        else if(param1 == nullptr)
            emit referenceAddCommand(QString::fromUtf8((const char*)param2), QString::fromUtf8((const char*)param2));
        else
            emit referenceAddCommand(QString::fromUtf8((const char*)param1), QString::fromUtf8((const char*)param2));
    }
    break;

    case GUI_OPEN_TRACE_FILE:
    {
        if(param1 == nullptr)
            return nullptr;
        emit openTraceFile(QString::fromUtf8((const char*)param1));
    }
    break;

    case GUI_UPDATE_TRACE_BROWSER:
        emit updateTraceBrowser();
        break;

    case GUI_INVALIDATE_SYMBOL_SOURCE:
        symbolView->invalidateSymbolSource(duint(param1));
        break;

    case GUI_GET_CURRENT_GRAPH:
    {
        BridgeResult result(BridgeResult::GraphCurrent);
        emit getCurrentGraph((BridgeCFGraphList*)param1);
        result.Wait();
    }
    break;

    case GUI_SHOW_REF:
        emit showReferences();
        break;

    case GUI_SELECT_IN_SYMBOLS_TAB:
        emit symbolSelectModule(duint(param1));
        break;
    }

    return nullptr;
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
    return Bridge::getBridge()->processMessage(type, param1, param2);
}

__declspec(dllexport) const char* _gui_translate_text(const char* source)
{
    if(TLS_TranslatedStringMap)
    {
        QByteArray translatedUtf8 = QCoreApplication::translate("DBG", source).toUtf8();
        // Boom... VS does not support "thread_local"... and cannot use "__declspec(thread)" in a DLL... https://blogs.msdn.microsoft.com/oldnewthing/20101122-00/?p=12233
        // Simulating Thread Local Storage with a map...
        DWORD ThreadId = GetCurrentThreadId();
        TranslatedStringStorage & TranslatedString = (*TLS_TranslatedStringMap)[ThreadId];
        TranslatedString.Data[translatedUtf8.size()] = 0; // Set the string terminator first.
        memcpy(TranslatedString.Data, translatedUtf8.constData(), std::min((size_t)translatedUtf8.size(), sizeof(TranslatedString.Data) - 1)); // Then copy the string safely.
        return TranslatedString.Data; // Don't need to free this memory. But this pointer should be used immediately to reduce race condition.
    }
    else // Translators are not initialized yet.
        return source;
}
