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
#include "YaraRuleSelectionDialog.h"
#include "AssembleDialog.h"
#include "StringUtil.h"
#include "Breakpoints.h"
#include "XrefBrowseDialog.h"
#include "SourceViewerManager.h"
#include "MiscUtil.h"
#include "SnowmanView.h"
#include "MemoryPage.h"
#include "BreakpointMenu.h"
#include "BrowseDialog.h"

CPUDisassembly::CPUDisassembly(CPUWidget* parent) : Disassembly(parent)
{
    setWindowTitle("Disassembly");

    // Set specific widget handles
    mParentCPUWindow = parent;

    // Create the action list for the right click context menu
    setupRightClickContextMenu();

    // Connect bridge<->disasm calls
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint, dsint)), this, SLOT(disassembleAt(dsint, dsint)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
    connect(this, SIGNAL(selectionExpanded()), this, SLOT(selectionUpdatedSlot()));
    connect(Bridge::getBridge(), SIGNAL(displayWarning(QString, QString)), this, SLOT(displayWarningSlot(QString, QString)));
    connect(Bridge::getBridge(), SIGNAL(focusDisasm()), this, SLOT(setFocus()));

    Initialize();
}

void CPUDisassembly::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        if(!DbgIsDebugging())
            return;
        MessageBeep(MB_OK);
        if(event->modifiers() & Qt::ShiftModifier)
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
        mBreakpointMenu->toggleInt3BPActionSlot();
        break;

    // (Disassembly) Assemble dialog
    case 2:
        assembleSlot();
        break;

    // (Comments) Set comment dialog
    case 3:
        setCommentSlot();
        break;

    // Undefined area
    default:
        Disassembly::mouseDoubleClickEvent(event);
        break;
    }
}

void CPUDisassembly::addFollowReferenceMenuItem(QString name, dsint value, QMenu* menu, bool isReferences, bool isFollowInCPU)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, this);
    newAction->setFont(QFont("Courier New", 8));
    menu->addAction(newAction);
    if(isFollowInCPU)
        newAction->setObjectName(QString("CPU|") + ToPtrString(value));
    else
        newAction->setObjectName(QString(isReferences ? "REF|" : "DUMP|") + ToPtrString(value));

    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

void CPUDisassembly::setupFollowReferenceMenu(dsint wVA, QMenu* menu, bool isReferences, bool isFollowInCPU)
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
            addFollowReferenceMenuItem(tr("&Selected Address"), wVA, menu, isReferences, isFollowInCPU);
    }

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

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
                QString constant = ToHexString(arg.constant);
                if(DbgMemIsValidReadPtr(arg.constant))
                    addFollowReferenceMenuItem(tr("&Constant: ") + constant, arg.constant, menu, isReferences, isFollowInCPU);
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
                    addFollowReferenceMenuItem(QString(arg.mnemonic).trimmed(), arg.value, menu, isReferences, isFollowInCPU);
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
    QMenu wMenu(this);
    if(!mHighlightContextMenu)
        mMenuBuilder->build(&wMenu);
    else if(mHighlightToken.text.length())
        mHighlightMenuBuilder->build(&wMenu);
    if(wMenu.actions().length())
        wMenu.exec(event->globalPos());
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
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_edit.png"), tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill.png"), tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill_nop.png"), tr("Fill with &NOPs"), SLOT(binaryFillNopsSlot()), "ActionBinaryFillNops"));
    binaryMenu->addSeparator();
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_copy.png"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste.png"), tr("&Paste"), SLOT(binaryPasteSlot()), "ActionBinaryPaste"), [](QMenu*)
    {
        return QApplication::clipboard()->mimeData()->hasText();
    });
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_paste_ignoresize.png"), tr("Paste (&Ignore Size)"), SLOT(binaryPasteIgnoreSizeSlot()), "ActionBinaryPasteIgnoreSize"), [](QMenu*)
    {
        return QApplication::clipboard()->mimeData()->hasText();
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("binary.png"), tr("&Binary")), binaryMenu);

    MenuBuilder* copyMenu = new MenuBuilder(this);
    copyMenu->addAction(makeShortcutAction(DIcon("copy_selection.png"), tr("&Selection"), SLOT(copySelectionSlot()), "ActionCopy"));
    copyMenu->addAction(makeAction(DIcon("copy_selection.png"), tr("Selection to &File"), SLOT(copySelectionToFileSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes.png"), tr("Selection (&No Bytes)"), SLOT(copySelectionNoBytesSlot())));
    copyMenu->addAction(makeAction(DIcon("copy_selection_no_bytes.png"), tr("Selection to File (No Bytes)"), SLOT(copySelectionToFileNoBytesSlot())));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address.png"), tr("&Address"), SLOT(copyAddressSlot()), "ActionCopyAddress"));
    copyMenu->addAction(makeShortcutAction(DIcon("copy_address.png"), tr("&RVA"), SLOT(copyRvaSlot()), "ActionCopyRva"));
    copyMenu->addAction(makeShortcutAction(DIcon("fileoffset.png"), tr("&File Offset"), SLOT(copyFileOffsetSlot()), "ActionCopyFileOffset"));
    copyMenu->addAction(makeAction(DIcon("copy_disassembly.png"), tr("Disassembly"), SLOT(copyDisassemblySlot())));

    copyMenu->addMenu(makeMenu(DIcon("copy_selection.png"), tr("Symbolic Name")), [this](QMenu * menu)
    {
        QSet<QString> labels;
        if(!getLabelsFromInstruction(rvaToVa(getInitialSelection()), labels))
            return false;
        for(auto label : labels)
            menu->addAction(makeAction(label, SLOT(labelCopySlot())));
        return true;
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("copy.png"), tr("&Copy")), copyMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser.png"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        dsint start = rvaToVa(getSelectionStart());
        dsint end = rvaToVa(getSelectionEnd());
        return DbgFunctions()->PatchInRange(start, end); //something patched in selected range
    });

    mBreakpointMenu = new BreakpointMenu(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getInitialSelection());
    });
    mBreakpointMenu->build(mMenuBuilder);

    mMenuBuilder->addMenu(makeMenu(DIcon("dump.png"), tr("&Follow in Dump")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, false);
        return true;
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("processor-cpu.png"), tr("&Follow in Disassembler")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, true);
        return menu->actions().length() != 0; //only add this menu if there is something to follow
    });

    mMenuBuilder->addAction(makeShortcutAction(DIcon("memmap_find_address_page.png"), tr("Follow in Memory Map"), SLOT(followInMemoryMapSlot()), "ActionFollowMemMap"));

    mMenuBuilder->addAction(makeShortcutAction(DIcon("source.png"), tr("Open Source File"), SLOT(openSourceSlot()), "ActionOpenSourceFile"), [this](QMenu*)
    {
        return DbgFunctions()->GetSourceFromAddr(rvaToVa(getInitialSelection()), 0, 0);
    });

    MenuBuilder* decompileMenu = new MenuBuilder(this);
    decompileMenu->addAction(makeShortcutAction(DIcon("decompile_selection.png"), tr("Selection"), SLOT(decompileSelectionSlot()), "ActionDecompileSelection"));
    decompileMenu->addAction(makeShortcutAction(DIcon("decompile_function.png"), tr("Function"), SLOT(decompileFunctionSlot()), "ActionDecompileFunction"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), 0, 0);
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("snowman.png"), tr("Decompile")), decompileMenu);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("graph.png"), tr("Graph"), SLOT(graphSlot()), "ActionGraph"));

    mMenuBuilder->addMenu(makeMenu(DIcon("help.png"), tr("Help on Symbolic Name")), [this](QMenu * menu)
    {
        QSet<QString> labels;
        if(!getLabelsFromInstruction(rvaToVa(getInitialSelection()), labels))
            return false;
        for(auto label : labels)
            menu->addAction(makeAction(label, SLOT(labelHelpSlot())));
        return true;
    });
    mMenuBuilder->addAction(makeShortcutAction(DIcon("helpmnemonic.png"), tr("Help on mnemonic"), SLOT(mnemonicHelpSlot()), "ActionHelpOnMnemonic"));
    QAction* mnemonicBrief = makeShortcutAction(DIcon("helpbrief.png"), tr("Show mnemonic brief"), SLOT(mnemonicBriefSlot()), "ActionToggleMnemonicBrief");
    mMenuBuilder->addAction(mnemonicBrief, [this, mnemonicBrief](QMenu*)
    {
        if(mShowMnemonicBrief)
            mnemonicBrief->setText(tr("Hide mnemonic brief"));
        else
            mnemonicBrief->setText(tr("Show mnemonic brief"));
        return true;
    });

    mMenuBuilder->addAction(makeShortcutAction(DIcon("highlight.png"), tr("&Highlighting mode"), SLOT(enableHighlightingModeSlot()), "ActionHighlightingMode"));
    QAction* togglePreview = makeShortcutAction(DIcon("branchpreview.png"), tr("Disable Branch Destination Preview"), SLOT(togglePreviewSlot()), "ActionToggleDestinationPreview");
    mMenuBuilder->addAction(togglePreview, [this, togglePreview](QMenu*)
    {
        togglePreview->setText(mPopupEnabled ? tr("Disable Branch Destination Preview") : tr("Enable Branch Destination Preview"));
        return false; //hide this menu from the user, but keep the shortcut working
    });

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
    mMenuBuilder->addMenu(makeMenu(DIcon("label.png"), tr("Label")), labelMenu);

    QAction* traceRecordDisable = makeAction(DIcon("close-all-tabs.png"), tr("Disable"), SLOT(ActionTraceRecordDisableSlot()));
    QAction* traceRecordEnableBit = makeAction(DIcon("bit.png"), tr("Bit"), SLOT(ActionTraceRecordBitSlot()));
    QAction* traceRecordEnableByte = makeAction(DIcon("byte.png"), tr("Byte"), SLOT(ActionTraceRecordByteSlot()));
    QAction* traceRecordEnableWord = makeAction(DIcon("word.png"), tr("Word"), SLOT(ActionTraceRecordWordSlot()));
    QAction* traceRecordToggleRunTrace = makeShortcutAction(tr("Start Run Trace"), SLOT(ActionTraceRecordToggleRunTraceSlot()), "ActionToggleRunTrace");
    mMenuBuilder->addMenu(makeMenu(DIcon("trace.png"), tr("Trace record")), [ = ](QMenu * menu)
    {
        if(DbgFunctions()->GetTraceRecordType(rvaToVa(getInitialSelection())) == TRACERECORDTYPE::TraceRecordNone)
        {
            menu->addAction(traceRecordEnableBit);
            menu->addAction(traceRecordEnableByte);
            menu->addAction(traceRecordEnableWord);
        }
        else
            menu->addAction(traceRecordDisable);
        menu->addSeparator();
        if(DbgValFromString("tr.runtraceenabled()") == 1)
            traceRecordToggleRunTrace->setText(tr("Stop Run Trace"));
        else
            traceRecordToggleRunTrace->setText(tr("Start Run Trace"));
        menu->addAction(traceRecordToggleRunTrace);
        return true;
    });

    mMenuBuilder->addAction(makeShortcutAction(DIcon("comment.png"), tr("Comment"), SLOT(setCommentSlot()), "ActionSetComment"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("bookmark_toggle.png"), tr("Toggle Bookmark"), SLOT(setBookmarkSlot()), "ActionToggleBookmark"));
    mMenuBuilder->addSeparator();

    MenuBuilder* analysisMenu = new MenuBuilder(this);
    QAction* toggleFunctionAction = makeShortcutAction(DIcon("functions.png"), tr("Function"), SLOT(toggleFunctionSlot()), "ActionToggleFunction");
    analysisMenu->addAction(makeShortcutAction(DIcon("analyzemodule.png"), tr("Analyze module"), SLOT(analyzeModuleSlot()), "ActionAnalyzeModule"));
    analysisMenu->addAction(toggleFunctionAction, [this, toggleFunctionAction](QMenu*)
    {
        if(!DbgFunctionOverlaps(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd())))
            toggleFunctionAction->setText(tr("Add function"));
        else
            toggleFunctionAction->setText(tr("Delete function"));
        return true;
    });
    QAction* toggleArgumentAction = makeShortcutAction(DIcon("arguments.png"), tr("Argument"), SLOT(toggleArgumentSlot()), "ActionToggleArgument");
    analysisMenu->addAction(toggleArgumentAction, [this, toggleArgumentAction](QMenu*)
    {
        if(!DbgArgumentOverlaps(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd())))
            toggleArgumentAction->setText(tr("Add argument"));
        else
            toggleArgumentAction->setText(tr("Delete argument"));
        return true;
    });
    analysisMenu->addAction(makeShortcutAction(DIcon("analysis_single_function.png"), tr("Analyze single function"), SLOT(analyzeSingleFunctionSlot()), "ActionAnalyzeSingleFunction"));
    analysisMenu->addSeparator();

    analysisMenu->addAction(makeShortcutAction(DIcon("remove_analysis_from_module.png"), tr("Remove type analysis from module"), SLOT(removeAnalysisModuleSlot()), "ActionRemoveTypeAnalysisFromModule"));
    analysisMenu->addAction(makeShortcutAction(DIcon("remove_analysis_from_selection.png"), tr("Remove type analysis from selection"), SLOT(removeAnalysisSelectionSlot()), "ActionRemoveTypeAnalysisFromSelection"));
    analysisMenu->addSeparator();

    QMenu* encodeTypeMenu = makeMenu(DIcon("treat_selection_head_as.png"), tr("Treat selection &head as"));
    QMenu* encodeTypeRangeMenu = makeMenu(DIcon("treat_from_selection_as.png"), tr("Treat from &selection as"));

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
                icon = DIcon(QString("treat_selection_as_%1.png").arg(iconTable[i]));
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

    mMenuBuilder->addMenu(makeMenu(DIcon("analysis.png"), tr("Analysis")), analysisMenu);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("pdb.png"), tr("Download Symbols for This Module"), SLOT(downloadCurrentSymbolsSlot()), "ActionDownloadSymbol"), [this](QMenu*)
    {
        //only show this action in system modules (generally user modules don't have downloadable symbols)
        return DbgFunctions()->ModGetParty(rvaToVa(getInitialSelection())) == 1;
    });
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeShortcutAction(DIcon("compile.png"), tr("Assemble"), SLOT(assembleSlot()), "ActionAssemble"));
    removeAction(mMenuBuilder->addAction(makeShortcutAction(DIcon("patch.png"), tr("Patches"), SLOT(showPatchesSlot()), "ViewPatches"))); //prevent conflicting shortcut with the MainWindow
    mMenuBuilder->addAction(makeShortcutAction(DIcon("yara.png"), tr("&Yara..."), SLOT(yaraSlot()), "ActionYara"));
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeShortcutAction(DIcon("neworigin.png"), tr("Set New Origin Here"), SLOT(setNewOriginHereActionSlot()), "ActionSetNewOriginHere"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("createthread.png"), tr("Create New Thread Here"), SLOT(createThreadSlot()), "ActionCreateNewThreadHere"));

    MenuBuilder* gotoMenu = new MenuBuilder(this);
    gotoMenu->addAction(makeShortcutAction(DIcon("cbp.png"), tr("Origin"), SLOT(gotoOriginSlot()), "ActionGotoOrigin"));
    gotoMenu->addAction(makeShortcutAction(DIcon("previous.png"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return historyHasPrevious();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next.png"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return historyHasNext();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto.png"), tr("Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    gotoMenu->addAction(makeShortcutAction(DIcon("fileoffset.png"), tr("File Offset"), SLOT(gotoFileOffsetSlot()), "ActionGotoFileOffset"), [this](QMenu*)
    {
        char modname[MAX_MODULE_SIZE] = "";
        return DbgGetModuleAt(rvaToVa(getInitialSelection()), modname);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("top.png"), tr("Start of Page"), SLOT(gotoStartSlot()), "ActionGotoStart"));
    gotoMenu->addAction(makeShortcutAction(DIcon("bottom.png"), tr("End of Page"), SLOT(gotoEndSlot()), "ActionGotoEnd"));
    gotoMenu->addAction(makeShortcutAction(DIcon("functionstart.png"), tr("Start of Function"), SLOT(gotoFunctionStartSlot()), "ActionGotoFunctionStart"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, nullptr);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("functionend.png"), tr("End of Function"), SLOT(gotoFunctionEndSlot()), "ActionGotoFunctionEnd"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, nullptr);
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("prevref.png"), tr("Previous Reference"), SLOT(gotoPreviousReferenceSlot()), "ActionGotoPreviousReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__disasm_refindex > 0 || dis.sel() != refsearch.addr($__disasm_refindex))");
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("nextref.png"), tr("Next Reference"), SLOT(gotoNextReferenceSlot()), "ActionGotoNextReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__disasm_refindex < refsearch.count()|| dis.sel() != refsearch.addr($__disasm_refindex))");
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("goto.png"), tr("Go to")), gotoMenu);
    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs.png"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"), [this](QMenu*)
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

    // Search in Current Region menu
    mFindCommandRegion = makeShortcutAction(DIcon("search_for_command.png"), tr("C&ommand"), SLOT(findCommandSlot()), "ActionFind");
    mFindConstantRegion = makeAction(DIcon("search_for_constant.png"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsRegion = makeAction(DIcon("search_for_string.png"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsRegion = makeAction(DIcon("call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternRegion = makeShortcutAction(DIcon("search_for_pattern.png"), tr("&Pattern"), SLOT(findPatternSlot()), "ActionFindPattern");
    mFindGUIDRegion = makeAction(DIcon("guid.png"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchRegionMenu->addAction(mFindCommandRegion);
    mSearchRegionMenu->addAction(mFindConstantRegion);
    mSearchRegionMenu->addAction(mFindStringsRegion);
    mSearchRegionMenu->addAction(mFindCallsRegion);
    mSearchRegionMenu->addAction(mFindPatternRegion);
    mSearchRegionMenu->addAction(mFindGUIDRegion);

    // Search in Current Module menu
    mFindCommandModule = makeShortcutAction(DIcon("search_for_command.png"), tr("C&ommand"), SLOT(findCommandSlot()), "ActionFindInModule");
    mFindConstantModule = makeAction(DIcon("search_for_constant.png"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsModule = makeAction(DIcon("search_for_string.png"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsModule = makeAction(DIcon("call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternModule = makeShortcutAction(DIcon("search_for_pattern.png"), tr("&Pattern"), SLOT(findPatternSlot()), "ActionFindPatternInModule");
    mFindGUIDModule = makeAction(DIcon("guid.png"), tr("&GUID"), SLOT(findGUIDSlot()));
    mFindNamesModule = makeShortcutAction(DIcon("names.png"), tr("&Names"), SLOT(findNamesSlot()), "ActionFindNamesInModule");
    mSearchModuleMenu->addAction(mFindCommandModule);
    mSearchModuleMenu->addAction(mFindConstantModule);
    mSearchModuleMenu->addAction(mFindStringsModule);
    mSearchModuleMenu->addAction(mFindCallsModule);
    mSearchModuleMenu->addAction(mFindPatternModule);
    mSearchModuleMenu->addAction(mFindGUIDModule);
    mSearchModuleMenu->addAction(mFindNamesModule);

    // Search in Current Function menu
    mFindCommandFunction = makeAction(DIcon("search_for_command.png"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantFunction = makeAction(DIcon("search_for_constant.png"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsFunction = makeAction(DIcon("search_for_string.png"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsFunction = makeAction(DIcon("call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternFunction = makeAction(DIcon("search_for_pattern.png"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDFunction = makeAction(DIcon("guid.png"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchFunctionMenu->addAction(mFindCommandFunction);
    mSearchFunctionMenu->addAction(mFindConstantFunction);
    mSearchFunctionMenu->addAction(mFindStringsFunction);
    mSearchFunctionMenu->addAction(mFindCallsFunction);
    mSearchFunctionMenu->addAction(mFindPatternFunction);
    mSearchFunctionMenu->addAction(mFindGUIDFunction);

    // Search in All Modules menu
    mFindCommandAll = makeAction(DIcon("search_for_command.png"), tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantAll = makeAction(DIcon("search_for_constant.png"), tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsAll = makeAction(DIcon("search_for_string.png"), tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsAll = makeAction(DIcon("call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mFindPatternAll = makeAction(DIcon("search_for_pattern.png"), tr("&Pattern"), SLOT(findPatternSlot()));
    mFindGUIDAll = makeAction(DIcon("guid.png"), tr("&GUID"), SLOT(findGUIDSlot()));
    mSearchAllMenu->addAction(mFindCommandAll);
    mSearchAllMenu->addAction(mFindConstantAll);
    mSearchAllMenu->addAction(mFindStringsAll);
    mSearchAllMenu->addAction(mFindCallsAll);
    mSearchAllMenu->addAction(mFindPatternAll);
    mSearchAllMenu->addAction(mFindGUIDAll);

    searchMenu->addMenu(makeMenu(DIcon("search_current_region.png"), tr("Current Region")), mSearchRegionMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_current_module.png"), tr("Current Module")), mSearchModuleMenu);
    QMenu* searchFunctionMenu = makeMenu(tr("Current Function"));
    searchMenu->addMenu(searchFunctionMenu, mSearchFunctionMenu);
    searchMenu->addMenu(makeMenu(DIcon("search_all_modules.png"), tr("All Modules")), mSearchAllMenu);
    mMenuBuilder->addMenu(makeMenu(DIcon("search-for.png"), tr("&Search for")), searchMenu);

    mReferenceSelectedAddressAction = makeShortcutAction(tr("&Selected Address(es)"), SLOT(findReferencesSlot()), "ActionFindReferencesToSelectedAddress");
    mReferenceSelectedAddressAction->setFont(QFont("Courier New", 8));

    mMenuBuilder->addMenu(makeMenu(DIcon("find.png"), tr("Find &references to")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, true, false);
        return true;
    });

    // Plugins
    mPluginMenu = new QMenu(this);
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_DISASM_MENU);

    mMenuBuilder->addSeparator();
    mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        DbgMenuPrepare(GUI_DISASM_MENU);
        menu->addActions(mPluginMenu->actions());
        return true;
    }));

    // Highlight menu
    mHighlightMenuBuilder = new MenuBuilder(this);

    mHighlightMenuBuilder->addAction(makeAction(DIcon("copy.png"), tr("Copy token &text"), SLOT(copyTokenTextSlot())));
    mHighlightMenuBuilder->addAction(makeAction(DIcon("copy_address.png"), tr("Copy token &value"), SLOT(copyTokenValueSlot())), [this](QMenu*)
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
    DbgCmdExec("disasm cip");
}




void CPUDisassembly::setNewOriginHereActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    if(DbgFunctions()->IsDepEnabled() && !DbgFunctions()->MemIsCodePage(wVA, false))
    {
        QMessageBox msg(QMessageBox::Warning, tr("Current address is not executable"),
                        tr("Setting new origin here may result in crash. Do you really want to continue?"), QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            return;
    }
    QString wCmd = "cip=" + ToPtrString(wVA);
    DbgCmdExec(wCmd.toUtf8().constData());
}

void CPUDisassembly::setLabelSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_LABEL_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != wVA)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(wVA, utf8data.constData()))
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
    QString addr_text = ToPtrString(addr);
    char label_text[MAX_LABEL_SIZE] = "";
    if(DbgGetLabelAt(addr, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
restart:
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QByteArray utf8data = mLineEdit.editText.toUtf8();
    if(!utf8data.isEmpty() && DbgIsValidExpression(utf8data.constData()) && DbgValFromString(utf8data.constData()) != addr)
    {
        QMessageBox msg(QMessageBox::Warning, tr("The label may be in use"),
                        tr("The label \"%1\" may be an existing label or a valid expression. Using such label might have undesired effects. Do you still want to continue?").arg(mLineEdit.editText),
                        QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            goto restart;
    }
    if(!DbgSetLabelAt(addr, utf8data.constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetLabelAt failed!"));


    GuiUpdateAllViews();
}

void CPUDisassembly::setCommentSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    mLineEdit.setTextMaxLength(MAX_COMMENT_SIZE - 2);
    QString addr_text = ToPtrString(wVA);
    char comment_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetCommentAt((duint)wVA, comment_text))
    {
        if(comment_text[0] == '\1') //automatic comment
            mLineEdit.setText(QString(comment_text + 1));
        else
            mLineEdit.setText(QString(comment_text));
    }
    mLineEdit.setWindowTitle(tr("Add comment at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    QString comment = mLineEdit.editText.replace('\r', "").replace('\n', "");
    if(!DbgSetCommentAt(wVA, comment.toUtf8().constData()))
        SimpleErrorBox(this, tr("Error!"), tr("DbgSetCommentAt failed!"));

    static bool easter = isEaster();
    if(easter && comment.toLower() == "oep")
    {
        QFile file(":/icons/images/egg.wav");
        if(file.open(QIODevice::ReadOnly))
        {
            QByteArray egg = file.readAll();
            PlaySoundA(egg.data(), 0, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
        }
    }

    GuiUpdateAllViews();
}

void CPUDisassembly::setBookmarkSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    bool result;
    if(DbgGetBookmarkAt(wVA))
        result = DbgSetBookmarkAt(wVA, false);
    else
        result = DbgSetBookmarkAt(wVA, true);
    if(!result)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("DbgSetBookmarkAt failed!"));
        msg.setWindowIcon(DIcon("compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }

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
        DbgCmdExec(cmd.toUtf8().constData());
    }
    else
    {
        for(duint i = start; i <= end; i++)
        {
            if(DbgFunctionGet(i, &function_start, &function_end))
                break;
        }
        QString cmd = QString("functiondel ") + ToPtrString(function_start);
        DbgCmdExec(cmd.toUtf8().constData());
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
        DbgCmdExec(cmd.toUtf8().constData());
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
        DbgCmdExec(cmd.toUtf8().constData());
    }
}

void CPUDisassembly::assembleSlot()
{
    if(!DbgIsDebugging())
        return;

    AssembleDialog assembleDialog(this);

    do
    {
        dsint wRVA = getInitialSelection();
        duint wVA = rvaToVa(wRVA);
        unfold(wRVA);
        QString addr_text = ToPtrString(wVA);

        Instruction_t instr = this->DisassembleAt(wRVA);

        QString actual_inst = instr.instStr;

        bool assembly_error;
        do
        {
            char error[MAX_ERROR_SIZE] = "";

            assembly_error = false;

            assembleDialog.setSelectedInstrVa(wVA);
            if(ConfigBool("Disassembler", "Uppercase"))
                actual_inst = actual_inst.toUpper().replace(QRegularExpression("0X([0-9A-F]+)"), "0x\\1");
            assembleDialog.setTextEditValue(actual_inst);
            assembleDialog.setWindowTitle(tr("Assemble at %1").arg(addr_text));
            assembleDialog.setFillWithNopsChecked(ConfigBool("Disassembler", "FillNOPs"));
            assembleDialog.setKeepSizeChecked(ConfigBool("Disassembler", "KeepSize"));

            auto exec = assembleDialog.exec();

            Config()->setBool("Disassembler", "FillNOPs", assembleDialog.bFillWithNopsChecked);
            Config()->setBool("Disassembler", "KeepSize", assembleDialog.bKeepSizeChecked);

            if(exec != QDialog::Accepted)
                return;

            //if the instruction its unknown or is the old instruction or empty (easy way to skip from GUI) skipping
            if(assembleDialog.editText == QString("???") || assembleDialog.editText.toLower() == instr.instStr.toLower() || assembleDialog.editText == QString(""))
                break;

            if(!DbgFunctions()->AssembleAtEx(wVA, assembleDialog.editText.toUtf8().constData(), error, assembleDialog.bFillWithNopsChecked))
            {
                QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("Failed to assemble instruction \" %1 \" (%2)").arg(assembleDialog.editText).arg(error));
                msg.setWindowIcon(DIcon("compile-error.png"));
                msg.setParent(this, Qt::Dialog);
                msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
                msg.exec();
                actual_inst = assembleDialog.editText;
                assembly_error = true;
            }
        }
        while(assembly_error);

        //select next instruction after assembling
        setSingleSelection(wRVA);

        dsint botRVA = getTableOffset();
        dsint topRVA = getInstructionRVA(getTableOffset(), getNbrOfLineToPrint() - 1);

        dsint wInstrSize = getInstructionRVA(wRVA, 1) - wRVA - 1;

        expandSelectionUpTo(wRVA + wInstrSize);
        selectNext(false);

        if(getSelectionStart() < botRVA)
            setTableOffset(getSelectionStart());
        else if(getSelectionEnd() >= topRVA)
            setTableOffset(getInstructionRVA(getSelectionEnd(), -getNbrOfLineToPrint() + 2));

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
        DbgCmdExec(QString().sprintf("disasm %p", value).toUtf8().constData());
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
    DbgCmdExec(QString().sprintf("disasm \"%p\"", value).toUtf8().constData());
}

void CPUDisassembly::gotoStartSlot()
{
    duint dest = mMemPage->getBase();
    DbgCmdExec(QString().sprintf("disasm \"%p\"", dest).toUtf8().constData());
}

void CPUDisassembly::gotoEndSlot()
{
    duint dest = mMemPage->getBase() + mMemPage->getSize() - (getViewableRowsCount() * 16);
    DbgCmdExec(QString().sprintf("disasm \"%p\"", dest).toUtf8().constData());
}

void CPUDisassembly::gotoFunctionStartSlot()
{
    duint start;
    if(!DbgFunctionGet(rvaToVa(getInitialSelection()), &start, nullptr))
        return;
    DbgCmdExec(QString("disasm \"%1\"").arg(ToHexString(start)).toUtf8().constData());
}

void CPUDisassembly::gotoFunctionEndSlot()
{
    duint end;
    if(!DbgFunctionGet(rvaToVa(getInitialSelection()), nullptr, &end))
        return;
    DbgCmdExec(QString("disasm \"%1\"").arg(ToHexString(end)).toUtf8().constData());
}

void CPUDisassembly::gotoPreviousReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__disasm_refindex"), addr = DbgEval("refsearch.addr($__disasm_refindex)");
    if(count)
    {
        if(index > 0 && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__disasm_refindex", index - 1);
        DbgCmdExec("disasm refsearch.addr($__disasm_refindex)");
    }
}

void CPUDisassembly::gotoNextReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__disasm_refindex"), addr = DbgEval("refsearch.addr($__disasm_refindex)");
    if(count)
    {
        if(index + 1 < count && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__disasm_refindex", index + 1);
        DbgCmdExec("disasm refsearch.addr($__disasm_refindex)");
    }
}

void CPUDisassembly::gotoXrefSlot()
{
    if(!DbgIsDebugging() || !mXrefInfo.refcount)
        return;
    if(!mXrefDlg)
        mXrefDlg = new XrefBrowseDialog(this);
    mXrefDlg->setup(getSelectedVa());
    mXrefDlg->showNormal();
}

void CPUDisassembly::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    if(action->objectName().startsWith("DUMP|"))
        DbgCmdExec(QString().sprintf("dump \"%s\"", action->objectName().mid(5).toUtf8().constData()).toUtf8().constData());
    else if(action->objectName().startsWith("REF|"))
    {
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        QString value = action->objectName().mid(4);
        DbgCmdExec(QString("findref \"" + value +  "\", " + addrText).toUtf8().constData());
        emit displayReferencesWidget();
    }
    else if(action->objectName().startsWith("CPU|"))
    {
        QString value = action->objectName().mid(4);
        DbgCmdExec(QString("disasm " + value).toUtf8().constData());
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
    DbgCmdExec(QString("findrefrange " + addrStart + ", " + addrEnd + ", " + addrDisasm).toUtf8().constData());
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
    else if(sender() == mFindConstantFunction)
        refFindType = -1;

    WordEditDialog wordEdit(this);
    wordEdit.setup(tr("Enter Constant"), 0, sizeof(dsint));
    if(wordEdit.exec() != QDialog::Accepted) //cancel pressed
        return;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    auto constText = ToHexString(wordEdit.getVal());
    if(refFindType != -1)
        DbgCmdExec(QString("findref %1, %2, 0, %3").arg(constText).arg(addrText).arg(refFindType).toUtf8().constData());
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("findref %1, %2, %3, 0").arg(constText).arg(ToPtrString(start)).arg(ToPtrString(end - start)).toUtf8().constData());
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
    else if(sender() == mFindStringsFunction)
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("strref %1, %2, 0").arg(ToPtrString(start)).arg(ToPtrString(end - start)).toUtf8().constData());
        return;
    }

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    DbgCmdExec(QString("strref %1, 0, %2").arg(addrText).arg(refFindType).toUtf8().constData());
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
    else if(sender() == mFindCallsFunction)
        refFindType = -1;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    if(refFindType != -1)
        DbgCmdExec(QString("modcallfind %1, 0, %2").arg(addrText).arg(refFindType).toUtf8().constData());
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("modcallfind %1, %2, 0").arg(ToPtrString(start)).arg(ToPtrString(end - start)).toUtf8().constData());
    }
    emit displayReferencesWidget();
}

void CPUDisassembly::findPatternSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.isDataCopiable(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;

    dsint addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);

    QString command;
    if(sender() == mFindPatternModule)
    {
        auto base = DbgFunctions()->ModBaseFromAddr(addr);
        if(base)
            command = QString("findallmem %1, %2, %3").arg(ToHexString(base), hexEdit.mHexEdit->pattern(), ToHexString(DbgFunctions()->ModSizeFromAddr(base)));
    }
    if(sender() == mFindPatternFunction)
    {
        duint start, end;
        if(DbgFunctionGet(addr, &start, &end))
            command = QString("findall %1, %2, %3").arg(ToPtrString(start)).arg(hexEdit.mHexEdit->pattern()).arg(ToPtrString(end - start));
        else
            return;
    }
    if(sender() == mFindPatternAll)
        command = QString("findallmem  %1, %2, %3").arg(ToPtrString(addr)).arg(hexEdit.mHexEdit->pattern()).arg("&data&");
    if(!command.length())
        command = QString("findall %1, %2").arg(ToHexString(addr), hexEdit.mHexEdit->pattern());

    DbgCmdExec(command.toUtf8().constData());
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
    else if(sender() == mFindGUIDFunction)
        refFindType = -1;

    auto addrText = ToHexString(rvaToVa(getInitialSelection()));
    if(refFindType == -1)
        DbgCmdExec(QString("findguid %1, 0, %2").arg(addrText).arg(refFindType).toUtf8().constData());
    else
    {
        duint start, end;
        if(DbgFunctionGet(rvaToVa(getInitialSelection()), &start, &end))
            DbgCmdExec(QString("findguid %1, %2, 0").arg(ToPtrString(start)).arg(ToPtrString(end - start)).toUtf8().constData());
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
        Bridge::getBridge()->symbolSelectModule(base);
        emit displaySymbolsWidget();
    }
}

void CPUDisassembly::selectionGetSlot(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(1);
}

void CPUDisassembly::selectionSetSlot(const SELECTIONDATA* selection)
{
    dsint selMin = getBase();
    dsint selMax = selMin + getSize();
    dsint start = selection->start;
    dsint end = selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->setResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->setResult(1);
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
    hexEdit.showKeepSize(false);
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

void CPUDisassembly::yaraSlot()
{
    YaraRuleSelectionDialog yaraDialog(this);
    if(yaraDialog.exec() == QDialog::Accepted)
    {
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        DbgCmdExec(QString("yara \"%0\",%1").arg(yaraDialog.getSelectedFile()).arg(addrText).toUtf8().constData());
        emit displayReferencesWidget();
    }
}

void CPUDisassembly::copySelectionSlot(bool copyBytes)
{
    QString selectionString = "";
    QString selectionHtmlString = "";
    QTextStream stream(&selectionString);
    QTextStream htmlStream(&selectionHtmlString);
    pushSelectionInto(copyBytes, stream, &htmlStream);
    Bridge::CopyToClipboard(selectionString, selectionHtmlString);
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

void CPUDisassembly::pushSelectionInto(bool copyBytes, QTextStream & stream, QTextStream* htmlStream)
{
    const int addressLen = getColumnWidth(0) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(1) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(2) / getCharWidth() - 1;
    if(htmlStream)
        *htmlStream << QString("<table style=\"border-width:0px;border-color:#000000;font-family:%1;font-size:%2px;\">").arg(font().family()).arg(getRowHeight());
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
            stream << "\r\n";
        duint cur_addr = rvaToVa(inst.rva);
        QString address = getAddrText(cur_addr, 0, addressLen > sizeof(duint) * 2 + 1);
        QString bytes;
        if(copyBytes)
        {
            for(int j = 0; j < inst.dump.size(); j++)
            {
                if(j)
                    bytes += " ";
                bytes += ToByteString((unsigned char)(inst.dump.at(j)));
            }
        }
        QString disassembly;
        QString htmlDisassembly;
        if(htmlStream)
        {
            RichTextPainter::List richText;
            if(mHighlightToken.text.length())
                CapstoneTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
            else
                CapstoneTokenizer::TokenToRichText(inst.tokens, richText, 0);
            RichTextPainter::htmlRichText(richText, htmlDisassembly, disassembly);
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
            *htmlStream << QString("<tr><td>%1</td><td>%2</td><td>").arg(address.toHtmlEscaped(), htmlDisassembly);
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

void CPUDisassembly::copyDisassemblySlot()
{
    QString clipboardHtml = QString("<div style=\"font-family: %1; font-size: %2px\">").arg(font().family()).arg(getRowHeight());
    QString clipboard = "";
    prepareDataRange(getSelectionStart(), getSelectionEnd(), [&](int i, const Instruction_t & inst)
    {
        if(i)
        {
            clipboard += "\r\n";
            clipboardHtml += "<br/>";
        }
        RichTextPainter::List richText;
        if(mHighlightToken.text.length())
            CapstoneTokenizer::TokenToRichText(inst.tokens, richText, &mHighlightToken);
        else
            CapstoneTokenizer::TokenToRichText(inst.tokens, richText, 0);
        RichTextPainter::htmlRichText(richText, clipboardHtml, clipboard);
        return true;
    });
    clipboardHtml += QString("</div>");
    Bridge::CopyToClipboard(clipboard, clipboardHtml);
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
    else if(sender() == mFindCommandFunction)
        refFindType = -1;
    else if(sender() == mFindCommandAll)
        refFindType = 2;

    LineEditDialog mLineEdit(this);
    mLineEdit.enableCheckBox(refFindType == 0);
    mLineEdit.setCheckBoxText(tr("Entire &Block"));
    mLineEdit.setCheckBox(ConfigBool("Disassembler", "FindCommandEntireBlock"));
    mLineEdit.setWindowTitle("Find Command");
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    Config()->setBool("Disassembler", "FindCommandEntireBlock", mLineEdit.bChecked);

    char error[MAX_ERROR_SIZE] = "";
    unsigned char dest[16];
    int asmsize = 0;
    duint va = rvaToVa(getInitialSelection());
    if(mLineEdit.bChecked) // entire block
        va = mMemPage->getBase();

    if(!DbgFunctions()->Assemble(mMemPage->getBase() + mMemPage->getSize() / 2, dest, &asmsize, mLineEdit.editText.toUtf8().constData(), error))
    {
        SimpleErrorBox(this, tr("Error!"), tr("Failed to assemble instruction \"") + mLineEdit.editText + "\" (" + error + ")");
        return;
    }

    QString addr_text = ToPtrString(va);

    dsint size = mMemPage->getSize();
    if(refFindType != -1)
        DbgCmdExec(QString("findasm \"%1\", %2, .%3, %4").arg(mLineEdit.editText).arg(addr_text).arg(size).arg(refFindType).toUtf8().constData());
    else
    {
        duint start, end;
        if(DbgFunctionGet(va, &start, &end))
            DbgCmdExec(QString("findasm \"%1\", %2, .%3, 0").arg(mLineEdit.editText).arg(ToPtrString(start)).arg(ToPtrString(end - start)).toUtf8().constData());
    }

    emit displayReferencesWidget();
}

void CPUDisassembly::openSourceSlot()
{
    char szSourceFile[MAX_STRING_SIZE] = "";
    int line = 0;
    if(!DbgFunctions()->GetSourceFromAddr(rvaToVa(getInitialSelection()), szSourceFile, &line))
        return;
    emit Bridge::getBridge()->loadSourceFile(szSourceFile, 0, line);
    emit displaySourceManagerWidget();
}

void CPUDisassembly::decompileSelectionSlot()
{
    dsint addr = rvaToVa(getSelectionStart());
    dsint size = getSelectionSize();
    emit displaySnowmanWidget();
    DecompileAt(Bridge::getBridge()->snowmanView, addr, addr + size);
}

void CPUDisassembly::decompileFunctionSlot()
{
    dsint addr = rvaToVa(getInitialSelection());
    duint start;
    duint end;
    if(DbgFunctionGet(addr, &start, &end))
    {
        BASIC_INSTRUCTION_INFO info;
        DbgDisasmFastAt(end, &info);
        end += info.size - 1;
        emit displaySnowmanWidget();
        DecompileAt(Bridge::getBridge()->snowmanView, start, end);
    }
}

void CPUDisassembly::displayWarningSlot(QString title, QString text)
{
    SimpleWarningBox(this, title, text);
}

void CPUDisassembly::paintEvent(QPaintEvent* event)
{
    // Hook/hack to update the sidebar at the same time as this widget.
    // Ensures the two widgets are synced and prevents "draw lag"
    auto sidebar = mParentCPUWindow->getSidebarWidget();

    if(sidebar)
        sidebar->reload();

    // Signal to render the original content
    Disassembly::paintEvent(event);
}

bool CPUDisassembly::getLabelsFromInstruction(duint addr, QSet<QString> & labels)
{
    BASIC_INSTRUCTION_INFO basicinfo;
    DbgDisasmFastAt(addr, &basicinfo);
    std::vector<duint> values = { addr, basicinfo.addr, basicinfo.value.value, basicinfo.memory.value};
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

    if(fullUrl.startsWith("execute://"))
    {
        QString command = fullUrl.right(fullUrl.length() - 10);
        QProcess::execute(command);
    }
    else
    {
        QDesktopServices::openUrl(QUrl(fullUrl));
    }
}

void CPUDisassembly::ActionTraceRecordBitSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordBitExec)))
        {
            GuiAddLogMessage(tr("Failed to set trace record.\n").toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::ActionTraceRecordByteSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordByteWithExecTypeAndCounter)))
        {
            GuiAddLogMessage(tr("Failed to set trace record.\n").toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::ActionTraceRecordWordSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordWordWithExecTypeAndCounter)))
        {
            GuiAddLogMessage(tr("Failed to set trace record.\n").toUtf8().constData());
            break;
        }
    }
    DbgCmdExec("traceexecute cip");
}

void CPUDisassembly::ActionTraceRecordDisableSlot()
{
    if(!DbgIsDebugging())
        return;
    duint base = mMemPage->getBase();
    duint size = mMemPage->getSize();
    for(duint i = base; i < base + size; i += 4096)
    {
        if(!(DbgFunctions()->SetTraceRecordType(i, TRACERECORDTYPE::TraceRecordNone)))
        {
            GuiAddLogMessage(tr("Failed to set trace record.\n").toUtf8().constData());
            break;
        }
    }
}

void CPUDisassembly::mnemonicBriefSlot()
{
    mShowMnemonicBrief = !mShowMnemonicBrief;
    reloadData();
}

void CPUDisassembly::mnemonicHelpSlot()
{
    unsigned char data[16] = { 0xCC };
    auto addr = rvaToVa(getInitialSelection());
    DbgMemRead(addr, data, sizeof(data));
    Zydis zydis;
    zydis.Disassemble(addr, data);
    DbgCmdExecDirect(QString("mnemonichelp %1").arg(zydis.Mnemonic().c_str()).toUtf8().constData());
    emit displayLogWidget();
}

void CPUDisassembly::analyzeSingleFunctionSlot()
{
    DbgCmdExec(QString("analr %1").arg(ToHexString(rvaToVa(getInitialSelection()))).toUtf8().constData());
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

void CPUDisassembly::graphSlot()
{
    if(DbgCmdExecDirect(QString("graph %1").arg(ToPtrString(rvaToVa(getSelectionStart()))).toUtf8().constData()))
        emit displayGraphWidget();
}

void CPUDisassembly::togglePreviewSlot()
{
    if(mPopupEnabled == true)
        ShowDisassemblyPopup(0, 0, 0);
    mPopupEnabled = !mPopupEnabled;
    BridgeSettingSetUint("Gui", "DisableBranchDestinationPreview", !mPopupEnabled);
}

void CPUDisassembly::analyzeModuleSlot()
{
    DbgCmdExec("cfanal");
    DbgCmdExec("analx");
}

void CPUDisassembly::createThreadSlot()
{
    duint addr = rvaToVa(getSelectionStart());
    if(DbgFunctions()->IsDepEnabled() && !DbgFunctions()->MemIsCodePage(addr, false))
    {
        QMessageBox msg(QMessageBox::Warning, tr("Current address is not executable"),
                        tr("Creating new thread here may result in crash. Do you really want to continue?"), QMessageBox::Yes | QMessageBox::No, this);
        msg.setWindowIcon(DIcon("compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::No)
            return;
    }
    WordEditDialog argWindow(this);
    argWindow.setup(tr("Argument for the new thread"), 0, sizeof(duint));
    if(argWindow.exec() != QDialog::Accepted)
        return;
    DbgCmdExec(QString("createthread %1, %2").arg(ToPtrString(addr)).arg(ToPtrString(argWindow.getVal())).toUtf8().constData());
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
    if(mHighlightToken.type <= CapstoneTokenizer::TokenType::MnemonicUnusual)
        return false;
    duint value = mHighlightToken.value.value;
    if(!mHighlightToken.value.size && !DbgFunctions()->ValFromString(mHighlightToken.text.toUtf8().constData(), &value))
        return false;
    text = ToHexString(value);
    return true;
}

void CPUDisassembly::followInMemoryMapSlot()
{
    DbgCmdExec(QString("memmapdump %1").arg(ToHexString(rvaToVa(getInitialSelection()))).toUtf8().constData());
}

void CPUDisassembly::downloadCurrentSymbolsSlot()
{
    char module[MAX_MODULE_SIZE] = "";
    if(DbgGetModuleAt(rvaToVa(getInitialSelection()), module))
        DbgCmdExec(QString("symdownload \"%0\"").arg(module).toUtf8().constData());
}

void CPUDisassembly::ActionTraceRecordToggleRunTraceSlot()
{
    if(!DbgIsDebugging())
        return;
    if(DbgValFromString("tr.runtraceenabled()") == 1)
        DbgCmdExec("StopRunTrace");
    else
    {
        QString defaultFileName;
        char moduleName[MAX_MODULE_SIZE];
        QDateTime currentTime = QDateTime::currentDateTime();
        duint defaultModule = DbgValFromString("mod.main()");
        if(DbgFunctions()->ModNameFromAddr(defaultModule, moduleName, false))
        {
            defaultFileName = QString::fromUtf8(moduleName);
        }
        defaultFileName += "-" + QLocale(QString(currentLocale)).toString(currentTime.date()) + " " + currentTime.time().toString("hh-mm-ss") + ArchValue(".trace32", ".trace64");
        BrowseDialog browse(this, tr("Select stored file"), tr("Store run trace to the following file"),
                            tr("Run trace files (*.%1);;All files (*.*)").arg(ArchValue("trace32", "trace64")), QCoreApplication::applicationDirPath() + QDir::separator() + "db" + QDir::separator() + defaultFileName, true);
        if(browse.exec() == QDialog::Accepted)
        {
            if(browse.path.contains(QChar('"')) || browse.path.contains(QChar('\'')))
                SimpleErrorBox(this, tr("Error"), tr("File name contains invalid character."));
            else
                DbgCmdExec(QString("StartRunTrace \"%1\"").arg(browse.path).toUtf8().constData());
        }
    }
}
