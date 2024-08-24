#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QDesktopServices>
#include <QClipboard>
#include "CPUDisassembly.h"
#include "main.h"
#include "CPUSideBar.h"
#include "CPUWidget.h"
#include "EncodeMap.h"
#include "CPUMultiDump.h"
#include "Configuration.h"
#include "Bridge.h"
#include "Imports.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"
#include "GotoDialog.h"
#include "HexEditDialog.h"
#include "AssembleDialog.h"
#include "StringUtil.h"
#include "XrefBrowseDialog.h"
#include "SourceViewerManager.h"
#include "MiscUtil.h"
#include "MemoryPage.h"
#include "CommonActions.h"
#include "BrowseDialog.h"
#include "Tracer/TraceBrowser.h"

CPUDisassembly::CPUDisassembly(Architecture* architecture, bool isMain, QWidget* parent)
    : Disassembly(architecture, isMain, parent)
{
    setWindowTitle("Disassembly");

    // Create the action list for the right click context menu
    setupRightClickContextMenu();

    // Connect bridge<->disasm calls
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(duint, duint)), this, SLOT(disassembleAtSlot(duint, duint)));
    if(mIsMain)
    {
        connect(Bridge::getBridge(), SIGNAL(selectionDisasmGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
        connect(Bridge::getBridge(), SIGNAL(selectionDisasmSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
        connect(Bridge::getBridge(), SIGNAL(displayWarning(QString, QString)), this, SLOT(displayWarningSlot(QString, QString)));
    }

    // Connect some internal signals
    connect(this, SIGNAL(selectionExpanded()), this, SLOT(selectionUpdatedSlot()));

    // Load configuration
    mShowMnemonicBrief = ConfigBool("Disassembler", "ShowMnemonicBrief");

    Initialize();
}

void CPUDisassembly::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        MessageBeep(MB_OK);
        if(event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))
            copyRvaSlot();
        else
            copyAddressSlot();
    }
    else
    {
        mHighlightContextMenu = false;
        Disassembly::mousePressEvent(event);
        if(mHighlightingMode) //disable highlighting mode after clicked
        {
            mHighlightContextMenu = true;
            mHighlightingMode = false;
            reloadData();
        }
    }
}

void CPUDisassembly::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        dsint mSelectedVa = rvaToVa(getInitialSelection());
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
            mRvaDisplayEnabled = false;
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = getBase();
        }
        reloadData();
    }
    break;

    // (Opcodes) Set INT3 breakpoint
    case 1:
        mCommonActions->toggleInt3BPActionSlot();
        break;

    // (Disassembly) Assemble dialog
    case 2:
    {
        duint assembleOnDoubleClickInt;
        bool assembleOnDoubleClick = (BridgeSettingGetUint("Disassembler", "AssembleOnDoubleClick", &assembleOnDoubleClickInt) && assembleOnDoubleClickInt);
        if(assembleOnDoubleClick)
        {
            assembleSlot();
        }
        else
        {
            followInstruction(getInitialSelection());
        }
    }
    break;

    // (Comments) Set comment dialog
    case 3:
        mCommonActions->setCommentSlot();
        break;

    // Undefined area
    default:
        Disassembly::mouseDoubleClickEvent(event);
        break;
    }
}

void CPUDisassembly::addFollowReferenceMenuItem(QString name, duint value, QMenu* menu, bool isReferences, bool isFollowInCPU)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, this);
    newAction->setFont(font());
    menu->addAction(newAction);
    if(isFollowInCPU)
        newAction->setObjectName(QString("CPU|") + ToPtrString(value));
    else
        newAction->setObjectName(QString(isReferences ? "REF|" : "DUMP|") + ToPtrString(value));

    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

void CPUDisassembly::setupFollowReferenceMenu(duint va, QMenu* menu, bool isReferences, bool isFollowInCPU)
{
    //remove previous actions
    QList<QAction*> list = menu->actions();
    for(int i = 0; i < list.length(); i++)
        menu->removeAction(list.at(i));

    //most basic follow action
    if(!isFollowInCPU)
    {
        if(isReferences)
            menu->addAction(mReferenceSelectedAddressAction);
        else
            addFollowReferenceMenuItem(tr("&Selected Address"), va, menu, isReferences, isFollowInCPU);
    }

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(va, &instr);

    if(!isReferences) //follow in dump
    {
        for(int i = 0; i < instr.argcount; i++)
        {
            const DISASM_ARG & arg = instr.arg[i];
            if(arg.type == arg_memory)
            {
                QString segment = "";
#ifdef _WIN64
                if(arg.segment == SEG_GS)
                    segment = "gs:";
#else //x32
                if(arg.segment == SEG_FS)
                    segment = "fs:";
#endif //_WIN64
                if(arg.value != arg.constant)
                {
                    if(DbgMemIsValidReadPtr(arg.value))
                        addFollowReferenceMenuItem(tr("&Address: ") + segment + QString(arg.mnemonic).toUpper().trimmed(), arg.value, menu, isReferences, isFollowInCPU);
                }
                if(DbgMemIsValidReadPtr(arg.constant))
                    addFollowReferenceMenuItem(tr("&Constant: ") + getSymbolicName(arg.constant), arg.constant, menu, isReferences, isFollowInCPU);
                if(DbgMemIsValidReadPtr(arg.memvalue))
                {
                    addFollowReferenceMenuItem(tr("&Value: ") + segment + "[" + QString(arg.mnemonic) + "]", arg.memvalue, menu, isReferences, isFollowInCPU);
                    //Check for switch statement
                    if(memcmp(instr.instruction, "jmp ", 4) == 0 && DbgMemIsValidReadPtr(arg.constant)) //todo: extend check for exact form "jmp [reg*4+disp]"
                    {
                        duint* switchTable = new duint[512];
                        memset(switchTable, 0, 512 * sizeof(duint));
                        if(DbgMemRead(arg.constant, switchTable, 512 * sizeof(duint)))
                        {
                            int index;
                            for(index = 0; index < 512; index++)
                                if(!DbgFunctions()->MemIsCodePage(switchTable[index], false))
                                    break;
                            if(index >= 2 && index < 512)
                                for(int index2 = 0; index2 < index; index2++)
                                    addFollowReferenceMenuItem(tr("Jump table%1: ").arg(index2) + ToHexString(switchTable[index2]), switchTable[index2], menu, isReferences, isFollowInCPU);
                        }
                        delete[] switchTable;
                    }
                }

            }
            else //arg_normal
            {
                if(DbgMemIsValidReadPtr(arg.value))
                {
                    QString symbolicName = getSymbolicName(arg.value);
                    QString mnemonic = QString(arg.mnemonic).trimmed();
                    if(mnemonic != ToHexString(arg.value))
                        mnemonic = mnemonic + ": " + symbolicName;
                    else
                        mnemonic = symbolicName;
                    addFollowReferenceMenuItem(mnemonic, arg.value, menu, isReferences, isFollowInCPU);
                }
            }
        }
    }
    else //find references
    {
        for(int i = 0; i < instr.argcount; i++)
        {
            const DISASM_ARG arg = instr.arg[i];
            QString constant = ToHexString(arg.constant);
            if(DbgMemIsValidReadPtr(arg.constant))
                addFollowReferenceMenuItem(tr("Address: ") + constant, arg.constant, menu, isReferences, isFollowInCPU);
            else if(arg.constant)
                addFollowReferenceMenuItem(tr("Constant: ") + constant, arg.constant, menu, isReferences, isFollowInCPU);
        }
    }
}


/************************************************************************************
                            Mouse Management
************************************************************************************/
/**
 * @brief       This method has been reimplemented. It manages the richt click context menu.
 *
 * @param[in]   event       Context menu event
 *
 * @return      Nothing.
 */
void CPUDisassembly::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    if(!mHighlightContextMenu)
        mMenuBuilder->build(&menu);
    else if(mHighlightToken.text.length())
        mHighlightMenuBuilder->build(&menu);
    if(menu.actions().length())
        menu.exec(event->globalPos());
}

/************************************************************************************
                         Context Menu Management
************************************************************************************/
void CPUDisassembly::setupRightClickContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });

    MenuBuilder* binaryMenu = new MenuBuilder(this);
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_edit"), tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill"), tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill_nop"), tr("Fill with &NOPs"), SLOT(binaryFillNopsSlot()), "ActionBinaryFillNops"));
    binaryMenu->addSeparator();
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_copy"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste"), tr("&Paste"), SLOT(binaryPasteSlot()), "ActionBinaryPaste"), [](QMenu*)
    {
        return QApplication::clipboard()->mimeData()->hasText();
    });
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste_ignoresize"), tr("Paste (&Ignore Size)"), SLOT(binaryPasteIgnoreSizeSlot()), "ActionBinaryPasteIgnoreSize"), [](QMenu*)
    {
        return QApplication::clipboard()->mimeData()->hasText();
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("binary"), tr("&Binary")), binaryMenu);

    MenuBuilder* copyMenu = new MenuBuilder(this);
    copyMenu->addAction(makeShortcutAction(DIcon("copy_selection"), tr("&Selection"), SLOT(copySelectionSlot()), "ActionCopy"));
    copyMenu->addAction(makeAction(DIcon("copy_selection"), tr("Selection to &File"), SLOT(copySelectionToFileSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes"), tr("Selection (&No Bytes)"), SLOT(copySelectionNoBytesSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes"), tr("Selection to File (No Bytes)"), SLOT(copySelectionToFileNoBytesSlot())));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address"), tr("&Address"), SLOT(copyAddressSlot()), "ActionCopyAddress"));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address"), tr("&RVA"), SLOT(copyRvaSlot()), "ActionCopyRva"));
    copyMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("&File Offset"), SLOT(copyFileOffsetSlot()), "ActionCopyFileOffset"));
    copyMenu->addAction(makeAction(tr("&Header VA"), SLOT(copyHeaderVaSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_disassembly"), tr("Disassembly"), SLOT(copyDisassemblySlot())));
    copyMenu->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        QSet<QString> labels;
        if(!getLabelsFromInstruction(rvaToVa(getInitialSelection()), labels))
            return false;
        menu->addSeparator();
        for(const auto & label : labels)
            menu->addAction(makeAction(label, SLOT(labelCopySlot())));
        return true;
    }));
    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        dsint start = rvaToVa(getSelectionStart());
        dsint end = rvaToVa(getSelectionEnd());
        return DbgFunctions()->PatchInRange(start, end); //something patched in selected range
    });

    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getInitialSelection());
    });
    mCommonActions->build(mMenuBuilder, CommonActions::ActionBreakpoint);

    mMenuBuilder->addMenu(makeMenu(DIcon("dump"), tr("&Follow in Dump")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, false);
        return true;
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("processor-cpu"), tr("&Follow in Disassembler")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, true);
        return menu->actions().length() != 0; //only add this menu if there is something to follow
    });

    mCommonActions->build(mMenuBuilder, CommonActions::ActionMemoryMap | CommonActions::ActionGraph);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("source"), tr("Open Source File"), SLOT(openSourceSlot()), "ActionOpenSourceFile"), [this](QMenu*)
    {
        return DbgFunctions()->GetSourceFromAddr(rvaToVa(getInitialSelection()), 0, 0);
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("help"), tr("Help on Symbolic Name")), [this](QMenu * menu)
    {
        QSet<QString> labels;
        if(!getLabelsFromInstruction(rvaToVa(getInitialSelection()), labels))
            return false;
        for(auto label : labels)
            menu->addAction(makeAction(label, SLOT(labelHelpSlot())));
        return true;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("helpmnemonic"), tr("Help on mnemonic"), SLOT(mnemonicHelpSlot()), "ActionHelpOnMnemonic"));
    QAction* mnemonicBrief = makeShortcutAction(DIcon("helpbrief"), tr("Show mnemonic brief"), SLOT(mnemonicBriefSlot()), "ActionToggleMnemonicBrief");
    mMenuBuilder->addAction(mnemonicBrief, [this, mnemonicBrief](QMenu*)
    {
        if(mShowMnemonicBrief)
            mnemonicBrief->setText(tr("Hide mnemonic brief"));
        else
            mnemonicBrief->setText(tr("Show mnemonic brief"));
        return true;
    });

    mMenuBuilder->addAction(makeShortcutAction(DIcon("highlight"), tr("&Highlighting mode"), SLOT(enableHighlightingModeSlot()), "ActionHighlightingMode"));
    mMenuBuilder->addAction(makeAction("Edit columns...", SLOT(editColumnDialog())));

    MenuBuilder* labelMenu = new MenuBuilder(this);
    labelMenu->addAction(makeShortcutAction(tr("Label Current Address"), SLOT(setLabelSlot()), "ActionSetLabel"));
    QAction* labelAddress = makeShortcutAction(tr("Label"), SLOT(setLabelAddressSlot()), "ActionSetLabelOperand");

    labelMenu->addAction(labelAddress, [this, labelAddress](QMenu*)
    {
        BASIC_INSTRUCTION_INFO instr_info;
        DbgDisasmFastAt(rvaToVa(getInitialSelection()), &instr_info);

        duint addr;
        if(instr_info.type & TYPE_MEMORY)
            addr = instr_info.memory.value;
        else if(instr_info.type & TYPE_ADDR)
            addr = instr_info.addr;
        else if(instr_info.type & TYPE_VALUE)
            addr = instr_info.value.value;
        else
            return false;

        labelAddress->setText(tr("Label") + " " + ToPtrString(addr));

        return DbgMemIsValidReadPtr(addr);
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("label"), tr("Label")), labelMenu);
    mCommonActions->build(mMenuBuilder, CommonActions::ActionComment | CommonActions::ActionBookmark);

    QAction* traceCoverageDisable = makeAction(DIcon("close-all-tabs"), tr("Disable"), SLOT(traceCoverageDisableSlot()));
    QAction* traceCoverageEnableBit = makeAction(DIcon("bit"), tr("Bit"), SLOT(traceCoverageBitSlot()));
    QAction* traceCoverageEnableByte = makeAction(DIcon("byte"), tr("Byte"), SLOT(traceCoverageByteSlot()));
    QAction* traceCoverageEnableWord = makeAction(DIcon("word"), tr("Word"), SLOT(traceCoverageWordSlot()));
    QAction* traceCoverageToggleTraceRecording = makeShortcutAction(DIcon("control-record"), tr("Start trace recording"), SLOT(traceCoverageToggleTraceRecordingSlot()), "ActionToggleRunTrace");
    mMenuBuilder->addMenu(makeMenu(DIcon("trace"), tr("Trace coverage")), [ = ](QMenu * menu)
    {
        if(DbgFunctions()->GetTraceRecordType(rvaToVa(getInitialSelection())) == TRACERECORDTYPE::TraceRecordNone)
        {
            menu->addAction(traceCoverageEnableBit);
            menu->addAction(traceCoverageEnableByte);
            menu->addAction(traceCoverageEnableWord);
        }
        else
            menu->addAction(traceCoverageDisable);
        menu->addSeparator();
        if(TraceBrowser::isRecording())
        {
            traceCoverageToggleTraceRecording->setText(tr("Stop trace recording"));
            traceCoverageToggleTraceRecording->setIcon(DIcon("control-stop"));
        }
        else
        {
            traceCoverageToggleTraceRecording->setText(tr("Start trace recording"));
            traceCoverageToggleTraceRecording->setIcon(DIcon("control-record"));
        }
        menu->addAction(traceCoverageToggleTraceRecording);
        return true;
    });

    mMenuBuilder->addSeparator();

    MenuBuilder* analysisMenu = new MenuBuilder(this);
    QAction* toggleFunctionAction = makeShortcutAction(DIcon("functions"), tr("Function"), SLOT(toggleFunctionSlot()), "ActionToggleFunction");
    analysisMenu->addAction(makeShortcutAction(DIcon("analyzemodule"), tr("Analyze module"), SLOT(analyzeModuleSlot()), "ActionAnalyzeModule"));
    analysisMenu->addAction(toggleFunctionAction, [this, toggleFunctionAction](QMenu*)
    {
        if(!DbgFunctionOverlaps(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd())))
            toggleFunctionAction->setText(tr("Add function"));
        else
            toggleFunctionAction->setText(tr("Delete function"));
        return true;
    });
    QAction* toggleArgumentAction = makeShortcutAction(DIcon("arguments"), tr("Argument"), SLOT(toggleArgumentSlot()), "ActionToggleArgument");
    analysisMenu->addAction(toggleArgumentAction, [this, toggleArgumentAction](QMenu*)
    {
        if(!DbgArgumentOverlaps(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd())))
            toggleArgumentAction->setText(tr("Add argument"));
        else
            toggleArgumentAction->setText(tr("Delete argument"));
        return true;
    });
    analysisMenu->addAction(makeShortcutAction(tr("Add loop"), SLOT(addLoopSlot()), "ActionAddLoop"));
    analysisMenu->addAction(makeShortcutAction(tr("Delete loop"), SLOT(deleteLoopSlot()), "ActionDeleteLoop"), [this](QMenu*)
    {
        return findDeepestLoopDepth(rvaToVa(getSelectionStart())) >= 0;
    });
    analysisMenu->addAction(makeShortcutAction(DIcon("analysis_single_function"), tr("Analyze single function"), SLOT(analyzeSingleFunctionSlot()), "ActionAnalyzeSingleFunction"));
    analysisMenu->addSeparator();

    analysisMenu->addAction(makeShortcutAction(DIcon("remove_analysis_from_module"), tr("Remove type analysis from module"), SLOT(removeAnalysisModuleSlot()), "ActionRemoveTypeAnalysisFromModule"));
    analysisMenu->addAction(makeShortcutAction(DIcon("remove_analysis_from_selection"), tr("Remove type analysis from selection"), SLOT(removeAnalysisSelectionSlot()), "ActionRemoveTypeAnalysisFromSelection"));
    analysisMenu->addSeparator();

    QMenu* encodeTypeMenu = makeMenu(DIcon("treat_selection_head_as"), tr("Treat selection &head as"));
    QMenu* encodeTypeRangeMenu = makeMenu(DIcon("treat_from_selection_as"), tr("Treat from &selection as"));

    const char* strTable[] = {"Code", "Byte", "Word", "Dword", "Fword", "Qword", "Tbyte", "Oword", nullptr,
                              "Float", "Double", "Long Double", nullptr,
                              "ASCII", "UNICODE", nullptr,
                              "MMWord", "XMMWord", "YMMWord"
                             };

    const char* shortcutTable[] = {"Code", "Byte", "Word", "Dword", "Fword", "Qword", "Tbyte", "Oword", nullptr,
                                   "Float", "Double", "LongDouble", nullptr,
                                   "ASCII", "UNICODE", nullptr,
                                   "MMWord", "XMMWord", "YMMWord"
                                  };

    const char* iconTable[] = {"cmd", "byte", "word", "dword", "fword", "qword", "tbyte", "oword", nullptr,
                               "float", "double", "longdouble", nullptr,
                               "ascii", "unicode", nullptr,
                               "mmword", "xmm", "ymm"
                              };

    ENCODETYPE enctypeTable[] = {enc_code, enc_byte, enc_word, enc_dword, enc_fword, enc_qword, enc_tbyte, enc_oword, enc_middle,
                                 enc_real4, enc_real8, enc_real10, enc_middle,
                                 enc_ascii, enc_unicode, enc_middle,
                                 enc_mmword, enc_xmmword, enc_ymmword
                                };

    int enctypesize = sizeof(enctypeTable) / sizeof(ENCODETYPE);

    for(int i = 0; i < enctypesize; i++)
    {
        if(enctypeTable[i] == enc_middle)
        {
            encodeTypeRangeMenu->addSeparator();
            encodeTypeMenu->addSeparator();
        }
        else
        {
            QAction* action;
            QIcon icon;
            if(iconTable[i])
                icon = DIcon(QString("treat_selection_as_%1").arg(iconTable[i]));
            if(shortcutTable[i])
                action = makeShortcutAction(icon, tr(strTable[i]), SLOT(setEncodeTypeRangeSlot()), QString("ActionTreatSelectionAs%1").arg(shortcutTable[i]).toUtf8().constData());
            else
                action = makeAction(icon, tr(strTable[i]), SLOT(setEncodeTypeRangeSlot()));
            action->setData(enctypeTable[i]);
            encodeTypeRangeMenu->addAction(action);
            if(shortcutTable[i])
                action = makeShortcutAction(icon, tr(strTable[i]), SLOT(setEncodeTypeSlot()), QString("ActionTreatSelectionHeadAs%1").arg(shortcutTable[i]).toUtf8().constData());
            else
                action = makeAction(icon, tr(strTable[i]), SLOT(setEncodeTypeSlot()));
            action->setData(enctypeTable[i]);
            encodeTypeMenu->addAction(action);
        }
    }

    analysisMenu->addMenu(encodeTypeRangeMenu);
    analysisMenu->addMenu(encodeTypeMenu);

    mMenuBuilder->addMenu(makeMenu(DIcon("analysis"), tr("Analysis")), analysisMenu);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("pdb"), tr("Download Symbols for This Module"), SLOT(downloadCurrentSymbolsSlot()), "ActionDownloadSymbol"), [this](QMenu*)
    {
        //only show this action in system modules (generally user modules don't have downloadable symbols)
        return DbgFunctions()->ModGetParty(rvaToVa(getInitialSelection())) == 1;
    });
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeShortcutAction(DIcon("compile"), tr("Assemble"), SLOT(assembleSlot()), "ActionAssemble"));
    removeAction(mMenuBuilder->addAction(makeShortcutAction(DIcon("patch"), tr("Patches"), SLOT(showPatchesSlot()), "ViewPatches"))); //prevent conflicting shortcut with the MainWindow
    mMenuBuilder->addSeparator();

    mCommonActions->build(mMenuBuilder, CommonActions::ActionNewOrigin | CommonActions::ActionNewThread);
    MenuBuilder* gotoMenu = new MenuBuilder(this);
    gotoMenu->addAction(makeShortcutAction(DIcon("cbp"), ArchValue("EIP", "RIP"), SLOT(gotoOriginSlot()), "ActionGotoOrigin"));
    gotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return historyHasPrevious();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return historyHasNext();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    gotoMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("File Offset"), SLOT(gotoFileOffsetSlot()), "ActionGotoFileOffset"), [this](QMenu*)
    {
        char modname[MAX_MODULE_SIZE] = "";
        return DbgGetModuleAt(rvaToVa(getInitialSelection()), modname);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("top"), tr("Start of Page"), SLOT(gotoStartSlot()), "ActionGotoStart"));
    gotoMenu->addAction(makeShortcutAction(DIcon("bottom"), tr("End of Page"), SLOT(gotoEndSlot()), "ActionGotoEnd"));
    gotoMenu->addAction(makeShortcutAction(DIcon("functionstart"), tr("Start of Function"), SLOT(gotoFunctionStartSlot()), "ActionGotoFunctionStart"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, nullptr);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("functionend"), tr("End of Function"), SLOT(gotoFunctionEndSlot()), "ActionGotoFunctionEnd"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, nullptr);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("prevref"), tr("Previous Reference"), SLOT(gotoPreviousReferenceSlot()), "ActionGotoPreviousReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__disasm_refindex > 0 || dis.sel() != refsearch.addr($__disasm_refindex))");
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("nextref"), tr("Next Reference"), SLOT(gotoNextReferenceSlot()), "ActionGotoNextReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__disasm_refindex < refsearch.count()|| dis.sel() != refsearch.addr($__disasm_refindex))");
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("goto"), tr("Go to")), gotoMenu);
    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"), [this](QMenu*)
    {
        return mXrefInfo.refcount > 0;
    });

    MenuBuilder* searchMenu = new MenuBuilder(this);
    MenuBuilder* mSearchRegionMenu = new MenuBuilder(this);
    MenuBuilder* mSearchModuleMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection())) != 0;
    });
    MenuBuilder* mSearchFunctionMenu = new MenuBuilder(this, [this](QMenu*)
    {
        duint start, end;
        return DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end);
    });
    MenuBuilder* mSearchAllMenu = new MenuBuilder(this);
    MenuBuilder* mSearchAllUserMenu = new MenuBuilder(this);
    MenuBuilder* mSearchAllSystemMenu = new MenuBuilder(this);

    // Search in Current Region menu
    mFindCommandRegion = makeShortcutAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()), "ActionFind");
    mFindConstantRegion = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsRegion = makeAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsRegion = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternRegion = makeShortcutAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()), "ActionFindPattern");
    mFindGUIDRegion = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchRegionMenu->addAction(mFindCommandRegion);
    mSearchRegionMenu->addAction(mFindConstantRegion);
    mSearchRegionMenu->addAction(mFindStringsRegion);
    mSearchRegionMenu->addAction(mFindCallsRegion);
    mSearchRegionMenu->addAction(mFindPatternRegion);
    mSearchRegionMenu->addAction(mFindGUIDRegion);

    // Search in Current Module menu
    mFindCommandModule = makeShortcutAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()), "ActionFindInModule");
    mFindConstantModule = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsModule = makeShortcutAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()), "ActionFindStringsModule");
    mFindCallsModule = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternModule = makeShortcutAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()), "ActionFindPatternInModule");
    mFindGUIDModule = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mFindNamesModule = makeShortcutAction(DIcon("names"), tr("&Names"), SLOT(findNamesSlot()), "ActionFindNamesInModule");
    mSearchModuleMenu->addAction(mFindCommandModule);
    mSearchModuleMenu->addAction(mFindConstantModule);
    mSearchModuleMenu->addAction(mFindStringsModule);
    mSearchModuleMenu->addAction(mFindCallsModule);
    mSearchModuleMenu->addAction(mFindPatternModule);
    mSearchModuleMenu->addAction(mFindGUIDModule);
    mSearchModuleMenu->addAction(mFindNamesModule);

    // Search in Current Function menu
    mFindCommandFunction = makeAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantFunction = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsFunction = makeAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsFunction = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternFunction = makeAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDFunction = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchFunctionMenu->addAction(mFindCommandFunction);
    mSearchFunctionMenu->addAction(mFindConstantFunction);
    mSearchFunctionMenu->addAction(mFindStringsFunction);
    mSearchFunctionMenu->addAction(mFindCallsFunction);
    mSearchFunctionMenu->addAction(mFindPatternFunction);
    mSearchFunctionMenu->addAction(mFindGUIDFunction);

    // Search in All User Modules menu
    mFindCommandAllUser = makeAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantAllUser = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsAllUser = makeAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsAllUser = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternAllUser = makeAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDAllUser = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchAllUserMenu->addAction(mFindCommandAllUser);
    mSearchAllUserMenu->addAction(mFindConstantAllUser);
    mSearchAllUserMenu->addAction(mFindStringsAllUser);
    mSearchAllUserMenu->addAction(mFindCallsAllUser);
    mSearchAllUserMenu->addAction(mFindPatternAllUser);
    mSearchAllUserMenu->addAction(mFindGUIDAllUser);

    // Search in All System Modules menu
    mFindCommandAllSystem = makeAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantAllSystem = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsAllSystem = makeAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsAllSystem = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternAllSystem = makeAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDAllSystem = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchAllSystemMenu->addAction(mFindCommandAllSystem);
    mSearchAllSystemMenu->addAction(mFindConstantAllSystem);
    mSearchAllSystemMenu->addAction(mFindStringsAllSystem);
    mSearchAllSystemMenu->addAction(mFindCallsAllSystem);
    mSearchAllSystemMenu->addAction(mFindPatternAllSystem);
    mSearchAllSystemMenu->addAction(mFindGUIDAllSystem);

    // Search in All Modules menu
    mFindCommandAll = makeAction(DIcon("search_for_command"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantAll = makeAction(DIcon("search_for_constant"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsAll = makeAction(DIcon("search_for_string"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsAll = makeAction(DIcon("call"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternAll = makeAction(DIcon("search_for_pattern"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDAll = makeAction(DIcon("guid"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchAllMenu->addAction(mFindCommandAll);
    mSearchAllMenu->addAction(mFindConstantAll);
    mSearchAllMenu->addAction(mFindStringsAll);
    mSearchAllMenu->addAction(mFindCallsAll);
    mSearchAllMenu->addAction(mFindPatternAll);
    mSearchAllMenu->addAction(mFindGUIDAll);

    searchMenu->addMenu(makeMenu(DIcon("search_current_region"), tr("Current Region")), mSearchRegionMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_current_module"), tr("Current Module")), mSearchModuleMenu);
    QMenu* searchFunctionMenu = makeMenu(tr("Current Function"));
    searchMenu->addMenu(searchFunctionMenu, mSearchFunctionMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_all_modules"), tr("All User Modules")), mSearchAllUserMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_all_modules"), tr("All System Modules")), mSearchAllSystemMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_all_modules"), tr("All Modules")), mSearchAllMenu);
    mMenuBuilder->addMenu(makeMenu(DIcon("search-for"), tr("&Search for")), searchMenu);

    mReferenceSelectedAddressAction = makeShortcutAction(tr("&Selected Address(es)"), SLOT(findReferencesSlot()), "ActionFindReferencesToSelectedAddress");

    mMenuBuilder->addMenu(makeMenu(DIcon("find"), tr("Find &references to")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, true, false);
        return true;
    });

    // Plugins
    if(mIsMain)
    {
        mPluginMenu = new QMenu(this);
        Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_DISASM_MENU);
    }

    mMenuBuilder->addSeparator();
    mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        DbgMenuPrepare(GUI_DISASM_MENU);
        if(mIsMain)
        {
            menu->addActions(mPluginMenu->actions());
        }
        return true;
    }));

    // Highlight menu
    mHighlightMenuBuilder = new MenuBuilder(this);

    mHighlightMenuBuilder->addAction(makeAction(DIcon("copy"), tr("Copy token &text"), SLOT(copyTokenTextSlot())));
    mHighlightMenuBuilder->addAction(makeAction(DIcon("copy_address"), tr("Copy token &value"), SLOT(copyTokenValueSlot())), [this](QMenu*)
    {
        QString text;
        if(!getTokenValueText(text))
            return false;
        return text != mHighlightToken.text;
    });

    mMenuBuilder->loadFromConfig();
}

void CPUDisassembly::gotoOriginSlot()
{
    if(!DbgIsDebugging())
        return;
    gotoAddress(DbgValFromString("cip"));
}

void CPUDisassembly::setLabelSlot()
{
    if(!DbgIsDebugging())
        return;
    duint va = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addrText = ToPtrString(va);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)va, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addrText);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != va)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(va, utf8data.constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetLabelAt failed!"));

    GuiUpdateAllViews();
}

void CPUDisassembly::setLabelAddressSlot()
{
    if(!DbgIsDebugging())
        return;
    BASIC_INSTRUCTION_INFO instr_info;
    DbgDisasmFastAt(rvaToVa(getInitialSelection()), &instr_info);

    duint addr;
    if(instr_info.type & TYPE_MEMORY)
        addr = instr_info.memory.value;
    else if(instr_info.type & TYPE_ADDR)
        addr = instr_info.addr;
    else if(instr_info.type & TYPE_VALUE)
        addr = instr_info.value.value;
    else
        return;
    if(!DbgMemIsValidReadPtr(addr))
        return;
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addrText = ToPtrString(addr);
    char label_text[MAX_LABEL_SIZE] = "";
    if(DbgGetLabelAt(addr, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addrText);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != addr)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(addr, utf8data.constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetLabelAt failed!"));


    GuiUpdateAllViews();
}

void CPUDisassembly::toggleFunctionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint start = rvaToVa(getSelectionStart());
    duint end = rvaToVa(getSelectionEnd());
    duint function_start = 0;
    duint function_end = 0;
    if(!DbgFunctionOverlaps(start, end))
    {
        QString cmd = QString("functionadd ") + ToPtrString(start) + "," + ToPtrString(end);
        DbgCmdExec(cmd);
    }
    else
    {
        for(duint i = start; i <= end; i++)
        {
            if(DbgFunctionGet(i, &function_start, &function_end))
                break;
        }
        QString cmd = QString("functiondel ") + ToPtrString(function_start);
        DbgCmdExec(cmd);
    }
}

void CPUDisassembly::toggleArgumentSlot()
{
    if(!DbgIsDebugging())
        return;
    duint start = rvaToVa(getSelectionStart());
    duint end = rvaToVa(getSelectionEnd());
    duint argument_start = 0;
    duint argument_end = 0;
    if(!DbgArgumentOverlaps(start, end))
    {
        QString start_text = ToPtrString(start);
        QString end_text = ToPtrString(end);

        QString cmd = "argumentadd " + start_text + "," + end_text;
        DbgCmdExec(cmd);
    }
    else
    {
        for(duint i = start; i <= end; i++)
        {
            if(DbgArgumentGet(i, &argument_start, &argument_end))
                break;
        }
        QString start_text = ToPtrString(argument_start);

        QString cmd = "argumentdel " + start_text;
        DbgCmdExec(cmd);
    }
}

void CPUDisassembly::addLoopSlot()
{
    if(!DbgIsDebugging())
        return;
    duint start = rvaToVa(getSelectionStart());
    duint end = rvaToVa(getSelectionEnd());
    if(start == end)
        return;
    auto depth = findDeepestLoopDepth(start);
    DbgCmdExec(QString("loopadd %1, %2").arg(ToPtrString(start)).arg(ToPtrString(end)).arg(depth));
}

void CPUDisassembly::deleteLoopSlot()
{
    if(!DbgIsDebugging())
        return;
    duint start = rvaToVa(getSelectionStart());
    auto depth = findDeepestLoopDepth(start);
    if(depth < 0)
        return;
    DbgCmdExec(QString("loopdel %1, .%2").arg(ToPtrString(start)).arg(depth));
}

void CPUDisassembly::assembleSlot()
{
    if(!DbgIsDebugging())
        return;

    AssembleDialog assembleDialog(this);

    do
    {
        dsint rva = getInitialSelection();
        duint va = rvaToVa(rva);
        unfold(rva);
        QString addrText = ToPtrString(va);

        Instruction_t instr = this->DisassembleAt(rva);

        QString actual_inst = instr.instStr;

        bool assembly_error;
        do
        {
            char error[MAX_ERROR_SIZE] = "";

            assembly_error = false;

            assembleDialog.setSelectedInstrVa(va);
            if(ConfigBool("Disassembler", "Uppercase"))
                actual_inst = actual_inst.toUpper().replace(QRegularExpression("0X([0-9A-F]+)"), "0x\\1");
            assembleDialog.setTextEditValue(actual_inst);
            assembleDialog.setWindowTitle(tr("Assemble at %1").arg(addrText));
            assembleDialog.setFillWithNopsChecked(ConfigBool("Disassembler", "FillNOPs"));
            assembleDialog.setKeepSizeChecked(ConfigBool("Disassembler", "KeepSize"));

            auto exec = assembleDialog.exec();

            Config()->setBool("Disassembler", "FillNOPs", assembleDialog.bFillWithNopsChecked);
            Config()->setBool("Disassembler", "KeepSize", assembleDialog.bKeepSizeChecked);

            if(exec != QDialog::Accepted)
                return;

            //sanitize the expression (just simplifying it by removing excess whitespaces)
            auto expression = assembleDialog.editText.simplified();

            //if the instruction its unknown or is the old instruction or empty (easy way to skip from GUI) skipping
            if(expression == QString("???") || expression.toLower() == instr.instStr.toLower() || expression == QString(""))
                break;

            if(!DbgFunctions()->AssembleAtEx(va, expression.toUtf8().constData(), error, assembleDialog.bFillWithNopsChecked))
            {
                QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("Failed to assemble instruction \" %1 \" (%2)").arg(expression).arg(error));
                msg.setWindowIcon(DIcon("compile-error"));
                msg.setParent(this, Qt::Dialog);
                msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
                msg.exec();
                actual_inst = expression;
                assembly_error = true;
            }
        }
        while(assembly_error);

        //select next instruction after assembling
        setSingleSelection(rva);

        auto botRVA = getTableOffset();
        auto topRVA = getInstructionRVA(getTableOffset(), getNbrOfLineToPrint() - 1);

        // TODO: this seems dumb
        auto instrSize = getInstructionRVA(rva, 1) - rva - 1;
        expandSelectionUpTo(rva + instrSize);
        selectNext(false);

        if(getSelectionStart() < botRVA)
            setTableOffset(getSelectionStart());
        else if(getSelectionEnd() >= topRVA)
            setTableOffset(getInstructionRVA(getSelectionEnd(), -(dsint)getNbrOfLineToPrint() + 2));

        //refresh view
        GuiUpdateAllViews();
    }
    while(1);
}

void CPUDisassembly::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setInitialExpression(ToPtrString(rvaToVa(getInitialSelection())));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        gotoAddress(value);
    }
}

void CPUDisassembly::gotoFileOffsetSlot()
{
    if(!DbgIsDebugging())
        return;
    char modname[MAX_MODULE_SIZE] = "";
    if(!DbgFunctions()->ModNameFromAddr(rvaToVa(getInitialSelection()), modname, true))
    {
        QMessageBox::critical(this, tr("Error!"), tr("Not inside a module..."));
        return;
    }
    if(!mGotoOffset)
        mGotoOffset = new GotoDialog(this);
    mGotoOffset->fileOffset = true;
    mGotoOffset->modName = QString(modname);
    mGotoOffset->setWindowTitle(tr("Goto File Offset in ") + QString(modname));
    QString offsetOfSelected;
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int, const Instruction_t & inst)
    {
        duint addr = rvaToVa(inst.rva);
        duint offset = DbgFunctions()->VaToFileOffset(addr);
        if(offset)
            offsetOfSelected = ToHexString(offset);
        return false;
    });
    if(!offsetOfSelected.isEmpty())
        mGotoOffset->setInitialExpression(offsetOfSelected);
    if(mGotoOffset->exec() != QDialog::Accepted)
        return;
    duint value = DbgValFromString(mGotoOffset->expressionText.toUtf8().constData());
    value = DbgFunctions()->FileOffsetToVa(modname, value);
    gotoAddress(value);
}

void CPUDisassembly::gotoStartSlot()
{
    duint dest = mMemPage->getBase();
    gotoAddress(dest);
}

void CPUDisassembly::gotoEndSlot()
{
    duint dest = mMemPage->getBase() + mMemPage->getSize() - (getViewableRowsCount() * 16);
    gotoAddress(dest);
}

void CPUDisassembly::gotoFunctionStartSlot()
{
    duint start;
    if(!DbgFunctionGet(rvaToVa(getInitialSelection()), &start, nullptr))
        return;
    gotoAddress(start);
}

void CPUDisassembly::gotoFunctionEndSlot()
{
    duint end;
    if(!DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, &end))
        return;
    gotoAddress(end);
}

void CPUDisassembly::gotoPreviousReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__disasm_refindex"), addr = DbgEval("refsearch.addr($__disasm_refindex)");
    if(count)
    {
        if(index > 0 && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__disasm_refindex", index - 1);
        gotoAddress(DbgValFromString("refsearch.addr($__disasm_refindex)"));
        GuiReferenceSetSingleSelection(int(DbgEval("$__disasm_refindex")), false);
    }
}

void CPUDisassembly::gotoNextReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__disasm_refindex"), addr = DbgEval("refsearch.addr($__disasm_refindex)");
    if(count)
    {
        if(index + 1 < count && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__disasm_refindex", index + 1);
        gotoAddress(DbgValFromString("refsearch.addr($__disasm_refindex)"));
        GuiReferenceSetSingleSelection(int(DbgEval("$__disasm_refindex")), false);
    }
}

void CPUDisassembly::gotoXrefSlot()
{
    if(!DbgIsDebugging() || !mXrefInfo.refcount)
        return;
    if(!mXrefDlg)
        mXrefDlg = new XrefBrowseDialog(this);
    mXrefDlg->setup(getSelectedVa(), [this](duint addr)
    {
        gotoAddress(addr);
    });
    mXrefDlg->showNormal();
}

void CPUDisassembly::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    if(action->objectName().startsWith("DUMP|"))
        DbgCmdExec(QString().sprintf("dump \"%s\"", action->objectName().mid(5).toUtf8().constData()));
    else if(action->objectName().startsWith("REF|"))
    {
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        QString value = action->objectName().mid(4);
        DbgCmdExec(QString("findref \"" + value +  "\", " + addrText));
        emit displayReferencesWidget();
    }
    else if(action->objectName().startsWith("CPU|"))
    {
        QString value = action->objectName().mid(4);
        gotoAddress(DbgValFromString(value.toUtf8().constData()));
    }
}

void CPUDisassembly::gotoPreviousSlot()
{
    historyPrevious();
}

void CPUDisassembly::gotoNextSlot()
{
    historyNext();
}

void CPUDisassembly::findReferencesSlot()
{
    QString addrStart = ToPtrString(rvaToVa(getSelectionStart()));
    QString addrEnd = ToPtrString(rvaToVa(getSelectionEnd()));
    QString addrDisasm = ToPtrString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("findrefrange " + addrStart + ", " + addrEnd + ", " + addrDisasm));
    emit displayReferencesWidget();
}

void CPUDisassembly::findConstantSlot()
{
    int refFindType = 0;
    if(sender() == mFindConstantRegion)
        refFindType = 0;
    else if(sender() == mFindConstantModule)
        refFindType = 1;
    else if(sender() == mFindConstantAll)
        refFindType = 2;
    else if(sender() == mFindConstantAllUser)
        refFindType = 3;
    else if(sender() == mFindConstantAllSystem)
        refFindType = 4;
    else if(sender() == mFindConstantFunction)
        refFindType = -1;

    WordEditDialog wordEdit(this);
    wordEdit.setup(tr("Enter Constant"), 0, sizeof(dsint));
    if(wordEdit.exec() != QDialog::Accepted) //cancel pressed
        return;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    auto constText = ToHexString(wordEdit.getVal());
    if(refFindType != -1)
        DbgCmdExec(QString("findref %1, %2, 0, %3").arg(constText).arg(addrText).arg(refFindType));
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("findref %1, %2, %3, 0").arg(constText).arg(ToPtrString(start)).arg(ToPtrString(end - start)));
    }
    emit displayReferencesWidget();
}

void CPUDisassembly::findStringsSlot()
{
    int refFindType = 0;
    if(sender() == mFindStringsRegion)
        refFindType = 0;
    else if(sender() == mFindStringsModule)
        refFindType = 1;
    else if(sender() == mFindStringsAll)
        refFindType = 2;
    else if(sender() == mFindStringsAllUser)
        refFindType = 3;
    else if(sender() == mFindStringsAllSystem)
        refFindType = 4;
    else if(sender() == mFindStringsFunction)
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("strref %1, %2, 0").arg(ToPtrString(start)).arg(ToPtrString(end - start)));
        return;
    }

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("strref %1, 0, %2").arg(addrText).arg(refFindType));
    emit displayReferencesWidget();
}


void CPUDisassembly::findCallsSlot()
{
    int refFindType = 0;
    if(sender() == mFindCallsRegion)
        refFindType = 0;
    else if(sender() == mFindCallsModule)
        refFindType = 1;
    else if(sender() == mFindCallsAll)
        refFindType = 2;
    else if(sender() == mFindCallsAllUser)
        refFindType = 3;
    else if(sender() == mFindCallsAllSystem)
        refFindType = 4;
    else if(sender() == mFindCallsFunction)
        refFindType = -1;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    if(refFindType != -1)
        DbgCmdExec(QString("modcallfind %1, 0, %2").arg(addrText).arg(refFindType));
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("modcallfind %1, %2, 0").arg(ToPtrString(start)).arg(ToPtrString(end - start)));
    }
    emit displayReferencesWidget();
}

void CPUDisassembly::findPatternSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.isDataCopiable(false);
    if(sender() == mFindPatternRegion)
        hexEdit.showStartFromSelection(true, ConfigBool("Disassembler", "FindPatternFromSelection"));
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;

    dsint addr = rvaToVa(getSelectionStart());

    QString command;
    if(sender() == mFindPatternRegion)
    {
        bool startFromSelection = hexEdit.startFromSelection();
        Config()->setBool("Disassembler", "FindPatternFromSelection", startFromSelection);
        if(!startFromSelection)
            addr = DbgMemFindBaseAddr(addr, 0);
        command = QString("findall %1, %2").arg(ToHexString(addr), hexEdit.mHexEdit->pattern());
    }
    else if(sender() == mFindPatternModule)
    {
        auto base = DbgFunctions()->ModBaseFromAddr(addr);
        if(base)
            command = QString("findallmem %1, %2, %3").arg(ToHexString(base), hexEdit.mHexEdit->pattern(), ToHexString(DbgFunctions()->ModSizeFromAddr(base)));
        else
            return;
    }
    else if(sender() == mFindPatternFunction)
    {
        duint start, end;
        if(DbgFunctionGet(addr, &start, &end))
            command = QString("findall %1, %2, %3").arg(ToPtrString(start), hexEdit.mHexEdit->pattern(), ToPtrString(end - start));
        else
            return;
    }
    else if(sender() == mFindPatternAll)
    {
        command = QString("findallmem 0, %1, &data&, module").arg(hexEdit.mHexEdit->pattern());
    }
    else if(sender() == mFindPatternAllUser)
    {
        command = QString("findallmem 0, %1, &data&, user").arg(hexEdit.mHexEdit->pattern());
    }
    else if(sender() == mFindPatternAllSystem)
    {
        command = QString("findallmem 0, %1, &data&, system").arg(hexEdit.mHexEdit->pattern());
    }

    if(!command.length())
        throw std::runtime_error("Implementation error in findPatternSlot()");

    DbgCmdExec(command);
    emit displayReferencesWidget();
}

void CPUDisassembly::findGUIDSlot()
{
    int refFindType = 0;
    if(sender() == mFindGUIDRegion)
        refFindType = 0;
    else if(sender() == mFindGUIDModule)
        refFindType = 1;
    else if(sender() == mFindGUIDAll)
        refFindType = 2;
    else if(sender() == mFindGUIDAllUser)
        refFindType = 3;
    else if(sender() == mFindGUIDAllSystem)
        refFindType = 4;
    else if(sender() == mFindGUIDFunction)
        refFindType = -1;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    if(refFindType == -1)
    {
        DbgCmdExec(QString("findguid %1, 0, %2").arg(addrText, refFindType));
    }
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("findguid %1, %2, 0").arg(ToPtrString(start), ToPtrString(end - start)));
    }
    emit displayReferencesWidget();
}

void CPUDisassembly::findNamesSlot()
{
    if(sender() == mFindNamesModule)
    {
        auto base = DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection()));
        if(!base)
            return;

        DbgCmdExec(QString("symfollow %1").arg(ToPtrString(base)));
    }
}

void CPUDisassembly::selectionGetSlot(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void CPUDisassembly::selectionSetSlot(const SELECTIONDATA* selection)
{
    dsint selMin = getBase();
    dsint selMax = selMin + getSize();
    dsint start = selection->start;
    dsint end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->setResult(BridgeResult::SelectionSet, 1);
}

void CPUDisassembly::selectionUpdatedSlot()
{
    QString selStart = ToPtrString(rvaToVa(getSelectionStart()));
    QString selEnd = ToPtrString(rvaToVa(getSelectionEnd()));
    QString info = tr("Disassembly");
    char mod[MAX_MODULE_SIZE] = "";
    if(DbgFunctions()->ModNameFromAddr(rvaToVa(getSelectionStart()), mod, true))
        info = QString(mod) + "";
    GuiAddStatusBarMessage(QString(info + ": " + selStart + " -> " + selEnd + QString().sprintf(" (0x%.8X bytes)\n", getSelectionEnd() - getSelectionStart() + 1)).toUtf8().constData());
}

void CPUDisassembly::enableHighlightingModeSlot()
{
    if(mHighlightingMode)
        mHighlightingMode = false;
    else
        mHighlightingMode = true;
    reloadData();
}

void CPUDisassembly::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showKeepSize(true);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle(tr("Edit code at %1").arg(ToPtrString(rvaToVa(selStart))));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint dataSize = hexEdit.mHexEdit->data().size();
    dsint newSize = selSize > dataSize ? selSize : dataSize;
    data = new byte_t[newSize];
    mMemPage->read(data, selStart, newSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, newSize));
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryFillSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.mHexEdit->setOverwriteMode(false);
    dsint selStart = getSelectionStart();
    hexEdit.setWindowTitle(tr("Fill code at %1").arg(ToPtrString(rvaToVa(selStart))));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    QString pattern = hexEdit.mHexEdit->pattern();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString(pattern));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryFillNopsSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), selSize, sizeof(duint));
    if(mLineEdit.exec() != QDialog::Accepted || !mLineEdit.getVal())
        return;
    selSize = mLineEdit.getVal();
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.mHexEdit->fill(0, QString("90"));
    QByteArray patched(hexEdit.mHexEdit->data());
    mMemPage->write(patched, selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::binaryCopySlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    Bridge::CopyToClipboard(hexEdit.mHexEdit->pattern(true));
}

void CPUDisassembly::binaryPasteSlot()
{
    if(!QApplication::clipboard()->mimeData()->hasText())
        return;
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    if(patched.size() < selSize)
        selSize = patched.size();
    mMemPage->write(patched.constData(), selStart, selSize);
    GuiUpdateAllViews();
}

void CPUDisassembly::undoSelectionSlot()
{
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(!DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        return;
    DbgFunctions()->PatchRestoreRange(start, end);
    reloadData();
}

void CPUDisassembly::binaryPasteIgnoreSizeSlot()
{
    if(!QApplication::clipboard()->mimeData()->hasText())
        return;
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QClipboard* clipboard = QApplication::clipboard();
    hexEdit.mHexEdit->setData(clipboard->text());

    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    QByteArray patched = hexEdit.mHexEdit->applyMaskedData(QByteArray((const char*)data, selSize));
    delete [] data;
    mMemPage->write(patched.constData(), selStart, patched.size());
    GuiUpdateAllViews();
}

void CPUDisassembly::showPatchesSlot()
{
    emit showPatches();
}

void CPUDisassembly::copySelectionSlot(bool copyBytes)
{
    QString selectionString = "";
    QString selectionHtmlString = "";
    QTextStream stream(&selectionString);
    if(getSelectionEnd() - getSelectionStart() < 2048)
    {
        QTextStream htmlStream(&selectionHtmlString);
        pushSelectionInto(copyBytes, stream, &htmlStream);
        Bridge::CopyToClipboard(selectionString, selectionHtmlString);
    }
    else
    {
        pushSelectionInto(copyBytes, stream, nullptr);
        Bridge::CopyToClipboard(selectionString);
    }
}

void CPUDisassembly::copySelectionToFileSlot(bool copyBytes)
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"), "", tr("Text Files (*.txt)"));
    if(fileName != "")
    {
        QFile file(fileName);
        if(!file.open(QIODevice::WriteOnly))
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
            return;
        }

        QTextStream stream(&file);
        pushSelectionInto(copyBytes, stream);
        file.close();
    }
}

void CPUDisassembly::setSideBar(CPUSideBar* sideBar)
{
    mSideBar = sideBar;
}

void CPUDisassembly::pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream)
{
    const int addressLen = getColumnWidth(0) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(1) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(2) / getCharWidth() - 1;
    if(htmlStream)
        *htmlStream << QString("<table style=\"border-width:0px;border-color:#000000;font-family:%1;font-size:%2px;\">").arg(font().family().toHtmlEscaped()).arg(getRowHeight());
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            stream << "\r\n";
        duint cur_addr = rvaToVa(inst.rva);
        QString label;
        QString address = getAddrText(cur_addr, label, addressLen > sizeof(duint) * 2 + 1);
        QString bytes;
        QString bytesHtml;
        if(copyBytes)
            RichTextPainter::htmlRichText(getRichBytes(inst, false), &bytesHtml, bytes);
        QString disassembly;
        QString htmlDisassembly;
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &htmlDisassembly, disassembly);
        }
        else
        {
            for(const auto & token : inst.tokens.tokens)
                disassembly += token.text;
        }
        QString fullComment;
        QString comment;
        bool autocomment;
        if(GetCommentFormat(cur_addr, comment, &autocomment))
            fullComment = " " + comment;
        stream << address.leftJustified(addressLen, QChar(' '), true);
        if(copyBytes)
            stream << " | " + bytes.leftJustified(bytesLen, QChar(' '), true);
        stream << " | " + disassembly.leftJustified(disassemblyLen, QChar(' '), true) + " |" + fullComment;
        if(htmlStream)
        {
            *htmlStream << QString("<tr><td>%1</td><td>%2</td><td>%3</td><td>").arg(address.toHtmlEscaped(), bytesHtml, htmlDisassembly);
            if(!comment.isEmpty())
            {
                if(autocomment)
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mAutoCommentColor.name());
                    if(mAutoCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mAutoCommentBackgroundColor.name());
                }
                else
                {
                    *htmlStream << QString("<span style=\"color:%1").arg(mCommentColor.name());
                    if(mCommentBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mCommentBackgroundColor.name());
                }
                *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
            }
            else
            {
                char label[MAX_LABEL_SIZE];
                if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label))
                {
                    comment = QString::fromUtf8(label);
                    *htmlStream << QString("<span style=\"color:%1").arg(mLabelColor.name());
                    if(mLabelBackgroundColor != Qt::transparent)
                        *htmlStream << QString(";background-color:%2").arg(mLabelBackgroundColor.name());
                    *htmlStream << "\">" << comment.toHtmlEscaped() << "</span></td></tr>";
                }
                else
                    *htmlStream << "</td></tr>";
            }
        }
        return true;
    });
    if(htmlStream)
        *htmlStream << "</table>";
}

void CPUDisassembly::copySelectionSlot()
{
    copySelectionSlot(true);
}

void CPUDisassembly::copySelectionToFileSlot()
{
    copySelectionToFileSlot(true);
}

void CPUDisassembly::copySelectionNoBytesSlot()
{
    copySelectionSlot(false);
}

void CPUDisassembly::copySelectionToFileNoBytesSlot()
{
    copySelectionToFileSlot(false);
}

void CPUDisassembly::copyAddressSlot()
{
    QString clipboard = "";
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            clipboard += "\r\n";
        clipboard += ToPtrString(rvaToVa(inst.rva));
        return true;
    });
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copyRvaSlot()
{
    QString clipboard = "";
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            clipboard += "\r\n";
        duint addr = rvaToVa(inst.rva);
        duint base = DbgFunctions()->ModBaseFromAddr(addr);
        if(base)
            clipboard += ToHexString(addr - base);
        else
        {
            SimpleWarningBox(this, tr("Error!"), tr("Selection not in a module..."));
            return false;
        }
        return true;
    });
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copyFileOffsetSlot()
{
    QString clipboard = "";
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            clipboard += "\r\n";
        duint addr = rvaToVa(inst.rva);
        duint offset = DbgFunctions()->VaToFileOffset(addr);
        if(offset)
            clipboard += ToHexString(offset);
        else
        {
            SimpleErrorBox(this, tr("Error!"), tr("Selection not in a file..."));
            return false;
        }
        return true;
    });
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copyHeaderVaSlot()
{
    QString clipboard = "";
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            clipboard += "\r\n";
        duint addr = rvaToVa(inst.rva);
        duint base = DbgFunctions()->ModBaseFromAddr(addr);
        if(base)
        {
            auto expr = QString("mod.headerva(0x%1)").arg(ToPtrString(addr));
            clipboard += ToPtrString(DbgValFromString(expr.toUtf8().constData()));
        }
        else
        {
            SimpleWarningBox(this, tr("Error!"), tr("Selection not in a module..."));
            return false;
        }
        return true;
    });
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copyDisassemblySlot()
{
    QString clipboard = "";
    if(getSelectionEnd() - getSelectionStart() < 2048)
    {
        QString clipboardHtml = QString("<div style=\"font-family: %1; font-size: %2px\">").arg(font().family()).arg(getRowHeight());
        prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
        {
            if(i)
            {
                clipboard += "\r\n";
                clipboardHtml += "<br/>";
            }
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, &clipboardHtml, clipboard);
            return true;
        });
        clipboardHtml += QString("</div>");
        Bridge::CopyToClipboard(clipboard, clipboardHtml);
    }
    else
    {
        prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
        {
            if(i)
            {
                clipboard += "\r\n";
            }
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                ZydisTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, nullptr, clipboard);
            return true;
        });
        Bridge::CopyToClipboard(clipboard);
    }
}

void CPUDisassembly::labelCopySlot()
{
    QString symbol = ((QAction*)sender())->text();
    Bridge::CopyToClipboard(symbol);
}

void CPUDisassembly::findCommandSlot()
{
    if(!DbgIsDebugging())
        return;

    int refFindType = 0;
    if(sender() == mFindCommandRegion)
        refFindType = 0;
    else if(sender() == mFindCommandModule)
        refFindType = 1;
    else if(sender() == mFindCommandAll)
        refFindType = 2;
    else if(sender() == mFindCommandAllUser)
        refFindType = 3;
    else if(sender() == mFindCommandAllSystem)
        refFindType = 4;
    else if(sender() == mFindCommandFunction)
        refFindType = -1;

    LineEditDialog mLineEdit(this);
    mLineEdit.enableCheckBox(refFindType == 0);
    mLineEdit.setCheckBoxText(tr("Start from &Selection"));
    mLineEdit.setCheckBox(ConfigBool("Disassembler", "FindCommandFromSelection"));
    mLineEdit.setWindowTitle("Find Command");
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    Config()->setBool("Disassembler", "FindCommandFromSelection", mLineEdit.bChecked);

    char error[MAX_ERROR_SIZE] = "";
    unsigned char dest[16];
    int asmsize = 0;
    duint va = rvaToVa(getInitialSelection());
    if(!mLineEdit.bChecked) // start search from selection
        va = mMemPage->getBase();

    if(!DbgFunctions()->Assemble(mMemPage->getBase() + mMemPage->getSize() / 2, dest, &asmsize, mLineEdit.editText.toUtf8().constData(), error))
    {
        SimpleErrorBox(this, tr("Error!"), tr("Failed to assemble instruction \"") + mLineEdit.editText + "\" (" + error + ")");
        return;
    }

    QString addrText = ToPtrString(va);

    dsint size = mMemPage->getSize();
    if(refFindType != -1)
        DbgCmdExec(QString("findasm \"%1\", %2, .%3, %4").arg(mLineEdit.editText).arg(addrText).arg(size).arg(refFindType));
    else
    {
        duint start, end;
        if(DbgFunctionGet(va, &start, &end))
            DbgCmdExec(QString("findasm \"%1\", %2, .%3, 0").arg(mLineEdit.editText).arg(ToPtrString(start)).arg(ToPtrString(end - start)));
    }

    emit displayReferencesWidget();
}

void CPUDisassembly::openSourceSlot()
{
    char szSourceFile[MAX_STRING_SIZE] = "";
    int line = 0;
    auto sel = rvaToVa(getInitialSelection());
    if(!DbgFunctions()->GetSourceFromAddr(sel, szSourceFile, &line))
        return;
    emit Bridge::getBridge()->loadSourceFile(szSourceFile, sel);
    emit displaySourceManagerWidget();
}

void CPUDisassembly::displayWarningSlot(QString title, QString text)
{
    SimpleWarningBox(this, title, text);
}

void CPUDisassembly::paintEvent(QPaintEvent* event)
{
    // Hook/hack to update the sidebar at the same time as this widget.
    // Ensures the two widgets are synced and prevents "draw lag"
    if(mSideBar)
        mSideBar->reload();

    // Signal to render the original content
    Disassembly::paintEvent(event);
}

int CPUDisassembly::findDeepestLoopDepth(duint addr)
{
    for(int depth = 0; ; depth++)
        if(!DbgLoopGet(depth, addr, nullptr, nullptr))
            return depth - 1;
    return -1; // unreachable
}

bool CPUDisassembly::getLabelsFromInstruction(duint addr, QSet<QString> & labels)
{
    BASIC_INSTRUCTION_INFO basicinfo;
    DbgDisasmFastAt(addr, &basicinfo);
    std::vector<duint> values = { addr, basicinfo.addr, basicinfo.value.value, basicinfo.memory.value };
    for(auto value : values)
    {
        char label_[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(value, SEG_DEFAULT, label_))
        {
            //TODO: better cleanup of names
            QString label(label_);
            if(label.endsWith("A") || label.endsWith("W"))
                label = label.left(label.length() - 1);
            if(label.startsWith("&"))
                label = label.right(label.length() - 1);
            labels.insert(label);
        }
    }
    return labels.size() != 0;
}

void CPUDisassembly::labelHelpSlot()
{
    QString topic = ((QAction*)sender())->text();
    char setting[MAX_SETTING_SIZE] = "";
    if(!BridgeSettingGet("Misc", "HelpOnSymbolicNameUrl", setting))
    {
        //"execute://winhlp32.exe -k@topic ..\\win32.hlp";
        strcpy_s(setting, "https://www.google.com/search?q=@topic");
        BridgeSettingSet("Misc", "HelpOnSymbolicNameUrl", setting);
    }
    QString baseUrl(setting);
    QString fullUrl = baseUrl.replace("@topic", topic);

    if(baseUrl.startsWith("execute://"))
    {
        QString command = fullUrl.right(fullUrl.length() - 10);
        QProcess::execute(command);
    }
    else
    {
        QDesktopServices::openUrl(QUrl(fullUrl));
    }
}

void CPUDisassembly::traceCoverageBitSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordBitExec)))
        {
            GuiAddLogMessage(tr("Failed to enable trace coverage for page %1.\n").arg(ToPtrString(i)).toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::traceCoverageByteSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordByteWithExecTypeAndCounter)))
        {
            GuiAddLogMessage(tr("Failed to enable trace coverage for page %1.\n").arg(ToPtrString(i)).toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::traceCoverageWordSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordWordWithExecTypeAndCounter)))
        {
            GuiAddLogMessage(tr("Failed to enable trace coverage for page %1.\n").arg(ToPtrString(i)).toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::traceCoverageDisableSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordNone)))
        {
            GuiAddLogMessage(tr("Failed to disable trace coverage for page %1.\n").arg(ToPtrString(i)).toUtf8().constData());
            break;
        }
    }
}

void CPUDisassembly::mnemonicBriefSlot()
{
    mShowMnemonicBrief = !mShowMnemonicBrief;
    Config()->setBool("Disassembler", "ShowMnemonicBrief", mShowMnemonicBrief);
    reloadData();
}

void CPUDisassembly::mnemonicHelpSlot()
{
    unsigned char data[16] = { 0xCC };
    auto addr = rvaToVa(getInitialSelection());
    DbgMemRead(addr, data, sizeof(data));
    Zydis zydis;
    zydis.Disassemble(addr, data);
    DbgCmdExecDirect(QString("mnemonichelp %1").arg(zydis.Mnemonic().c_str()));
    emit displayLogWidget();
}

void CPUDisassembly::analyzeSingleFunctionSlot()
{
    DbgCmdExec(QString("analr %1").arg(ToHexString(rvaToVa(getInitialSelection()))));
}

void CPUDisassembly::removeAnalysisSelectionSlot()
{
    if(!DbgIsDebugging())
        return;
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), getSelectionSize(), sizeof(duint));
    if(mLineEdit.exec() != QDialog::Accepted || !mLineEdit.getVal())
        return;
    mDisasm->getEncodeMap()->delRange(rvaToVa(getSelectionStart()), mLineEdit.getVal());
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::removeAnalysisModuleSlot()
{
    if(!DbgIsDebugging())
        return;
    mDisasm->getEncodeMap()->delSegment(rvaToVa(getSelectionStart()));
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::setEncodeTypeRangeSlot()
{
    if(!DbgIsDebugging())
        return;
    QAction* pAction = qobject_cast<QAction*>(sender());
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), getSelectionSize(), sizeof(duint));
    if(mLineEdit.exec() != QDialog::Accepted || !mLineEdit.getVal())
        return;
    mDisasm->getEncodeMap()->setDataType(rvaToVa(getSelectionStart()), mLineEdit.getVal(), (ENCODETYPE)pAction->data().toUInt());
    setSingleSelection(getSelectionStart());
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::setEncodeTypeSlot()
{
    if(!DbgIsDebugging())
        return;
    QAction* pAction = qobject_cast<QAction*>(sender());
    mDisasm->getEncodeMap()->setDataType(rvaToVa(getSelectionStart()), (ENCODETYPE)pAction->data().toUInt());
    setSingleSelection(getSelectionStart());
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::analyzeModuleSlot()
{
    DbgCmdExec("cfanal");
    DbgCmdExec("analx");
}

void CPUDisassembly::copyTokenTextSlot()
{
    Bridge::CopyToClipboard(mHighlightToken.text);
}

void CPUDisassembly::copyTokenValueSlot()
{
    QString text;
    if(getTokenValueText(text))
        Bridge::CopyToClipboard(text);
}

bool CPUDisassembly::getTokenValueText(QString & text)
{
    if(mHighlightToken.type <= ZydisTokenizer::TokenType::MnemonicUnusual)
        return false;
    duint value = mHighlightToken.value.value;
    if(!mHighlightToken.value.size && !DbgFunctions()->ValFromString(mHighlightToken.text.toUtf8().constData(), &value))
        return false;
    text = ToHexString(value);
    return true;
}

void CPUDisassembly::downloadCurrentSymbolsSlot()
{
    char module[MAX_MODULE_SIZE] = "";
    if(DbgGetModuleAt(rvaToVa(getInitialSelection()), module))
        DbgCmdExec(QString("symdownload \"%0\"").arg(module));
}

void CPUDisassembly::traceCoverageToggleTraceRecordingSlot()
{
    TraceBrowser::toggleTraceRecording(this);
}
