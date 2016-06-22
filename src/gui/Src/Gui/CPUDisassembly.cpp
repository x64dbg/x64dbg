#include "CPUDisassembly.h"
#include "CPUWidget.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QClipboard>
#include "Configuration.h"
#include "Bridge.h"
#include "LineEditDialog.h"
#include "WordEditDialog.h"
#include "HexEditDialog.h"
#include "YaraRuleSelectionDialog.h"
#include "AssembleDialog.h"
#include "StringUtil.h"
#include "Breakpoints.h"
#include "XrefBrowseDialog.h"

CPUDisassembly::CPUDisassembly(CPUWidget* parent) : Disassembly(parent)
{
    // Set specific widget handles
    mGoto = nullptr;
    mParentCPUWindow = parent;

    // Create the action list for the right click context menu
    setupRightClickContextMenu();

    // Connect bridge<->disasm calls
    connect(Bridge::getBridge(), SIGNAL(disassembleAt(dsint, dsint)), this, SLOT(disassembleAt(dsint, dsint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmGet(SELECTIONDATA*)), this, SLOT(selectionGetSlot(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDisasmSet(const SELECTIONDATA*)), this, SLOT(selectionSetSlot(const SELECTIONDATA*)));
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
        copyAddressSlot();
    }
    else
    {
        Disassembly::mousePressEvent(event);
        if(mHighlightingMode) //disable highlighting mode after clicked
        {
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
        toggleInt3BPActionSlot();
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
    foreach(QAction * action, menu->actions()) //check for duplicate action
    if(action->text() == name)
        return;
    QAction* newAction = new QAction(name, this);
    newAction->setFont(QFont("Courier New", 8));
    menu->addAction(newAction);
    if(isFollowInCPU)
        newAction->setObjectName(QString("CPU|") + QString("%1").arg(value, sizeof(dsint) * 2, 16, QChar('0')).toUpper());
    else
        newAction->setObjectName(QString(isReferences ? "REF|" : "DUMP|") + QString("%1").arg(value, sizeof(dsint) * 2, 16, QChar('0')).toUpper());

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
            const DISASM_ARG arg = instr.arg[i];
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
                if(DbgMemIsValidReadPtr(arg.value))
                    addFollowReferenceMenuItem(tr("&Address: ") + segment + QString(arg.mnemonic).toUpper().trimmed(), arg.value, menu, isReferences, isFollowInCPU);
                if(arg.value != arg.constant)
                {
                    QString constant = QString("%1").arg(arg.constant, 1, 16, QChar('0')).toUpper();
                    if(DbgMemIsValidReadPtr(arg.constant))
                        addFollowReferenceMenuItem(tr("&Constant: ") + constant, arg.constant, menu, isReferences, isFollowInCPU);
                }
                if(DbgMemIsValidReadPtr(arg.memvalue))
                    addFollowReferenceMenuItem(tr("&Value: ") + segment + "[" + QString(arg.mnemonic) + "]", arg.memvalue, menu, isReferences, isFollowInCPU);
            }
            else //arg_normal
            {
                if(DbgMemIsValidReadPtr(arg.value))
                    addFollowReferenceMenuItem(QString(arg.mnemonic).toUpper().trimmed(), arg.value, menu, isReferences, isFollowInCPU);
            }
        }
    }
    else //find references
    {
        for(int i = 0; i < instr.argcount; i++)
        {
            const DISASM_ARG arg = instr.arg[i];
            QString constant = QString("%1").arg(arg.constant, 1, 16, QChar('0')).toUpper();
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
    mMenuBuilder->build(&wMenu);
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
    binaryMenu->addAction(makeShortcutAction(tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));
    binaryMenu->addAction(makeShortcutAction(tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));
    binaryMenu->addAction(makeShortcutAction(tr("Fill with &NOPs"), SLOT(binaryFillNopsSlot()), "ActionBinaryFillNops"));
    binaryMenu->addSeparator();
    binaryMenu->addAction(makeShortcutAction(tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));
    binaryMenu->addAction(makeShortcutAction(tr("&Paste"), SLOT(binaryPasteSlot()), "ActionBinaryPaste"));
    binaryMenu->addAction(makeShortcutAction(tr("Paste (&Ignore Size)"), SLOT(binaryPasteIgnoreSizeSlot()), "ActionBinaryPasteIgnoreSize"));
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/binary.png"), tr("&Binary")), binaryMenu);

    MenuBuilder* copyMenu = new MenuBuilder(this);
    copyMenu->addAction(makeShortcutAction(tr("&Selection"), SLOT(copySelectionSlot()), "ActionCopy"));
    copyMenu->addAction(makeAction(tr("Selection (&No Bytes)"), SLOT(copySelectionNoBytesSlot())));
    copyMenu->addAction(makeShortcutAction(tr("&Address"), SLOT(copyAddressSlot()), "ActionCopyAddress"));
    copyMenu->addAction(makeAction(tr("&RVA"), SLOT(copyRvaSlot())));
    copyMenu->addAction(makeAction(tr("Disassembly"), SLOT(copyDisassemblySlot())));
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/copy.png"), tr("&Copy")), copyMenu);

    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/eraser.png"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        dsint start = rvaToVa(getSelectionStart());
        dsint end = rvaToVa(getSelectionEnd());
        return DbgFunctions()->PatchInRange(start, end); //something patched in selected range
    });

    QAction* toggleBreakpointAction = makeShortcutAction(tr("Toggle"), SLOT(toggleInt3BPActionSlot()), "ActionToggleBreakpoint");
    QAction* editSoftwareBreakpointAction = makeAction(tr("Edit"), SLOT(editSoftBpActionSlot()));
    QAction* setHwBreakpointAction = makeAction(tr("Set Hardware on Execution"), SLOT(toggleHwBpActionSlot()));
    QAction* removeHwBreakpointAction = makeAction(tr("Remove Hardware"), SLOT(toggleHwBpActionSlot()));

    QMenu* replaceSlotMenu = makeMenu(tr("Set Hardware on Execution"));
    QAction* replaceSlot0Action = makeMenuAction(replaceSlotMenu, tr("Replace Slot 0 (Free)"), SLOT(setHwBpOnSlot0ActionSlot()));
    QAction* replaceSlot1Action  = makeMenuAction(replaceSlotMenu, tr("Replace Slot 1 (Free)"), SLOT(setHwBpOnSlot1ActionSlot()));
    QAction* replaceSlot2Action  = makeMenuAction(replaceSlotMenu, tr("Replace Slot 2 (Free)"), SLOT(setHwBpOnSlot2ActionSlot()));
    QAction* replaceSlot3Action  = makeMenuAction(replaceSlotMenu, tr("Replace Slot 3 (Free)"), SLOT(setHwBpOnSlot3ActionSlot()));

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/breakpoint.png"), tr("Breakpoint")), [ = ](QMenu * menu)
    {
        BPXTYPE bpType = DbgGetBpxTypeAt(rvaToVa(getInitialSelection()));
        if((bpType & bp_normal) == bp_normal)
            menu->addAction(editSoftwareBreakpointAction);

        menu->addAction(toggleBreakpointAction);

        if((bpType & bp_hardware) == bp_hardware)
        {
            menu->addAction(removeHwBreakpointAction);
        }
        else
        {
            BPMAP bpList;
            DbgGetBpList(bp_hardware, &bpList);

            //get enabled hwbp count
            int enabledCount = bpList.count;
            for(int i = 0; i < bpList.count; i++)
                if(!bpList.bp[i].enabled)
                    enabledCount--;

            if(enabledCount < 4)
            {
                menu->addAction(setHwBreakpointAction);
            }
            else
            {
                REGDUMP wRegDump;
                DbgGetRegDump(&wRegDump);

                for(int i = 0; i < 4; i++)
                {
                    switch(bpList.bp[i].slot)
                    {
                    case 0:
                        replaceSlot0Action->setText("Replace Slot 0 (0x" + QString("%1").arg(bpList.bp[i].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 1:
                        replaceSlot1Action->setText("Replace Slot 1 (0x" + QString("%1").arg(bpList.bp[i].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 2:
                        replaceSlot2Action->setText("Replace Slot 2 (0x" + QString("%1").arg(bpList.bp[i].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    case 3:
                        replaceSlot3Action->setText("Replace Slot 3 (0x" + QString("%1").arg(bpList.bp[i].addr, 8, 16, QChar('0')).toUpper() + ")");
                        break;
                    default:
                        break;
                    }
                }
                menu->addMenu(replaceSlotMenu);
            }
            if(bpList.count)
                BridgeFree(bpList.bp);
        }
        return true;
    });

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/dump.png"), tr("&Follow in Dump")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, false);
        return true;
    });

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/processor-cpu.png"), tr("&Follow in Disassembler")), [this](QMenu * menu)
    {
        setupFollowReferenceMenu(rvaToVa(getInitialSelection()), menu, false, true);
        return menu->actions().length() != 0; //only add this menu if there is something to follow
    });

    mMenuBuilder->addAction(makeAction(QIcon(":/icons/images/source.png"), tr("Open Source File"), SLOT(openSourceSlot())), [this](QMenu*)
    {
        return DbgFunctions()->GetSourceFromAddr(rvaToVa(getInitialSelection()), 0, 0);
    });

    MenuBuilder* decompileMenu = new MenuBuilder(this);
    decompileMenu->addAction(makeShortcutAction(tr("Selection"), SLOT(decompileSelectionSlot()), "ActionDecompileSelection"));
    decompileMenu->addAction(makeShortcutAction(tr("Function"), SLOT(decompileFunctionSlot()), "ActionDecompileFunction"), [this](QMenu*)
    {
        return DbgFunctionGet(rvaToVa(getInitialSelection()), 0, 0);
    });

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/snowman.png"), tr("Decompile")), decompileMenu);

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/help.png"), tr("Help on Symbolic Name")), [this](QMenu * menu)
    {
        QSet<QString> labels;
        if(!getLabelsFromInstruction(rvaToVa(getInitialSelection()), labels))
            return false;
        for(auto label : labels)
            menu->addAction(makeAction(label, SLOT(labelHelpSlot())));
        return true;
    });
    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/helpmnemonic.png"), tr("Help on mnemonic"), SLOT(mnemonicHelpSlot()), "ActionHelpOnMnemonic"));
    QAction* mnemonicBrief = makeShortcutAction(QIcon(":/icons/images/helpbrief.png"), tr("Show mnemonic brief"), SLOT(mnemonicBriefSlot()), "ActionToggleMnemonicBrief");
    mMenuBuilder->addAction(mnemonicBrief, [this, mnemonicBrief](QMenu*)
    {
        if(mShowMnemonicBrief)
            mnemonicBrief->setText(tr("Hide mnemonic brief"));
        else
            mnemonicBrief->setText(tr("Show mnemonic brief"));
        return true;
    });

    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/highlight.png"), tr("&Highlighting mode"), SLOT(enableHighlightingModeSlot()), "ActionHighlightingMode"));
    mMenuBuilder->addSeparator();

    MenuBuilder* labelMenu = new MenuBuilder(this);
    labelMenu->addAction(makeShortcutAction(tr("Label Current Address"), SLOT(setLabelSlot()), "ActionSetLabel"));
    QAction* labelAddress = makeAction(tr("Label"), SLOT(setLabelAddressSlot()));

    labelMenu->addAction(labelAddress, [this, labelAddress](QMenu*)
    {
        BASIC_INSTRUCTION_INFO instr_info;
        DbgDisasmFastAt(rvaToVa(getInitialSelection()), &instr_info);

        duint addr;
        if(instr_info.type & TYPE_MEMORY)
            addr = instr_info.memory.value;
        else if(instr_info.type & TYPE_VALUE || instr_info.type & TYPE_ADDR)
            addr = instr_info.addr;
        else
            return false;

        labelAddress->setText(tr("Label") + " " + ToPtrString(addr));

        return DbgMemIsValidReadPtr(addr);
    });
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/label.png"), tr("Label")), labelMenu);

    QAction* traceRecordDisable = makeAction(tr("Disable"), SLOT(ActionTraceRecordDisableSlot()));
    QAction* traceRecordEnableBit = makeAction(tr("Bit"), SLOT(ActionTraceRecordBitSlot()));
    QAction* traceRecordEnableByte = makeAction(tr("Byte"), SLOT(ActionTraceRecordByteSlot()));
    QAction* traceRecordEnableWord = makeAction(tr("Word"), SLOT(ActionTraceRecordWordSlot()));
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/trace.png"), tr("Trace record")), [ = ](QMenu * menu)
    {
        if(DbgFunctions()->GetTraceRecordType(rvaToVa(getInitialSelection())) == TRACERECORDTYPE::TraceRecordNone)
        {
            menu->addAction(traceRecordEnableBit);
            menu->addAction(traceRecordEnableByte);
            menu->addAction(traceRecordEnableWord);
        }
        else
            menu->addAction(traceRecordDisable);
        return true;
    });

    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/comment.png"), tr("Comment"), SLOT(setCommentSlot()), "ActionSetComment"));
    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/bookmark.png"), tr("Bookmark"), SLOT(setBookmarkSlot()), "ActionToggleBookmark"));
    QAction* toggleFunctionAction = makeShortcutAction(QIcon(":/icons/images/functions.png"), tr("Function"), SLOT(toggleFunctionSlot()), "ActionToggleFunction");
    mMenuBuilder->addAction(toggleFunctionAction, [this, toggleFunctionAction](QMenu*)
    {
        if(!DbgFunctionOverlaps(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd())))
            toggleFunctionAction->setText(tr("Add function"));
        else
            toggleFunctionAction->setText(tr("Delete function"));
        return true;
    });

    mMenuBuilder->addSeparator();


    MenuBuilder* analysisMenu = new MenuBuilder(this);


    analysisMenu->addAction(makeShortcutAction(tr("Analyze single function"), SLOT(analyzeSingleFunctionSlot()), "ActionAnalyzeSingleFunction"));
    analysisMenu->addAction(makeAction(tr("Remove analysis from module"), SLOT(removeAnalysisModuleSlot())));

    analysisMenu->addSeparator();

    analysisMenu->addAction(makeAction(tr("Remove analysis from selection"), SLOT(removeAnalysisSelectionSlot())));

    QMenu* encodeTypeMenu = makeMenu("Treat selection head as");

    QMenu* encodeTypeRangeMenu = makeMenu("Treat selection as");

    std::string strTable[] = {"Command", "Byte", "Word", "Dword", "Fword", "Qword", "Tbyte", "Oword", "",
                              "Float", "Double", "Long Double", "",
                              "ASCII", "UNICODE", "",
                              "MMWord", "XMMWord", "YMMWord"
                             };

    ENCODETYPE enctypeTable[] = {enc_code, enc_byte, enc_word, enc_dword, enc_fword, enc_qword, enc_tbyte, enc_oword, enc_middle,
                                 enc_real4, enc_real8, enc_real10 , enc_middle,
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
            QAction* action = makeAction(tr(strTable[i].c_str()), SLOT(setEncodeTypeRangeSlot()));
            action->setData(enctypeTable[i]);
            encodeTypeRangeMenu->addAction(action);
            action = makeAction(tr(strTable[i].c_str()), SLOT(setEncodeTypeSlot()));
            action->setData(enctypeTable[i]);
            encodeTypeMenu->addAction(action);
        }
    }

    analysisMenu->addMenu(encodeTypeMenu);
    analysisMenu->addMenu(encodeTypeRangeMenu);

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/analyzesinglefunction.png"), tr("Analysis")), analysisMenu);
    mMenuBuilder->addSeparator();


    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/compile.png"), tr("Assemble"), SLOT(assembleSlot()), "ActionAssemble"));


    removeAction(mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/patch.png"), tr("Patches"), SLOT(showPatchesSlot()), "ViewPatches"))); //prevent conflicting shortcut with the MainWindow
    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/yara.png"), tr("&Yara..."), SLOT(yaraSlot()), "ActionYara"));
    mMenuBuilder->addSeparator();

    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/neworigin.png"), tr("Set New Origin Here"), SLOT(setNewOriginHereActionSlot()), "ActionSetNewOriginHere"));

    MenuBuilder* gotoMenu = new MenuBuilder(this);
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/cbp.png"), tr("Origin"), SLOT(gotoOriginSlot()), "ActionGotoOrigin"));
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/previous.png"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return historyHasPrevious();
    });
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/next.png"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return historyHasNext();
    });
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/geolocation-goto.png"), tr("Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/fileoffset.png"), tr("File Offset"), SLOT(gotoFileOffsetSlot()), "ActionGotoFileOffset"), [this](QMenu*)
    {
        char modname[MAX_MODULE_SIZE] = "";
        return DbgGetModuleAt(rvaToVa(getInitialSelection()), modname);
    });
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/top.png"), tr("Start of Page"), SLOT(gotoStartSlot()), "ActionGotoStart"));
    gotoMenu->addAction(makeShortcutAction(QIcon(":/icons/images/bottom.png"), tr("End of Page"), SLOT(gotoEndSlot()), "ActionGotoEnd"));
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/goto.png"), tr("Go to")), gotoMenu);
    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeShortcutAction(QIcon(":/icons/images/xrefs.png"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"), [this](QMenu*)
    {
        return mXrefInfo.refcount > 0;
    });

    MenuBuilder* searchMenu = new MenuBuilder(this);
    MenuBuilder* mSearchRegionMenu = new MenuBuilder(this);
    MenuBuilder* mSearchModuleMenu = new MenuBuilder(this);
    MenuBuilder* mSearchAllMenu = new MenuBuilder(this);

    // Search in Current Region menu
    mFindCommandRegion = makeShortcutAction(tr("C&ommand"), SLOT(findCommandSlot()), "ActionFind");
    mFindConstantRegion = makeAction(tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsRegion = makeAction(tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsRegion = makeAction(QIcon(":/icons/images/call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mSearchRegionMenu->addAction(mFindCommandRegion);
    mSearchRegionMenu->addAction(mFindConstantRegion);
    mSearchRegionMenu->addAction(mFindStringsRegion);
    mSearchRegionMenu->addAction(mFindCallsRegion);
    mSearchRegionMenu->addAction(makeShortcutAction(tr("&Pattern"), SLOT(findPatternSlot()), "ActionFindPattern"));

    // Search in Current Module menu
    mFindCommandModule = makeAction(tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantModule = makeAction(tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsModule = makeAction(tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsModule = makeAction(QIcon(":/icons/images/call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mSearchModuleMenu->addAction(mFindCommandModule);
    mSearchModuleMenu->addAction(mFindConstantModule);
    mSearchModuleMenu->addAction(mFindStringsModule);
    mSearchModuleMenu->addAction(mFindCallsModule);

    // Search in All Modules menu
    mFindCommandAll = makeAction(tr("C&ommand"), SLOT(findCommandSlot()));
    mFindConstantAll = makeAction(tr("&Constant"), SLOT(findConstantSlot()));
    mFindStringsAll = makeAction(tr("&String references"), SLOT(findStringsSlot()));
    mFindCallsAll = makeAction(QIcon(":/icons/images/call.png"), tr("&Intermodular calls"), SLOT(findCallsSlot()));
    mSearchAllMenu->addAction(mFindCommandAll);
    mSearchAllMenu->addAction(mFindConstantAll);
    mSearchAllMenu->addAction(mFindStringsAll);
    mSearchAllMenu->addAction(mFindCallsAll);

    searchMenu->addMenu(makeMenu(tr("Current Region")), mSearchRegionMenu);
    searchMenu->addMenu(makeMenu(tr("Current Module")), mSearchModuleMenu);
    searchMenu->addMenu(makeMenu(tr("All Modules")), mSearchAllMenu);
    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/search-for.png"), tr("&Search for")), searchMenu);

    mReferenceSelectedAddressAction = makeShortcutAction(tr("&Selected Address(es)"), SLOT(findReferencesSlot()), "ActionFindReferencesToSelectedAddress");
    mReferenceSelectedAddressAction->setFont(QFont("Courier New", 8));

    mMenuBuilder->addMenu(makeMenu(QIcon(":/icons/images/find.png"), tr("Find &references to")), [this](QMenu * menu)
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
        menu->addActions(mPluginMenu->actions());
        return true;
    }));
}

void CPUDisassembly::gotoOriginSlot()
{
    if(!DbgIsDebugging())
        return;
    DbgCmdExec("disasm cip");
}


void CPUDisassembly::toggleInt3BPActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_normal) == bp_normal)
    {
        wCmd = "bc " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bp " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
    //emit Disassembly::repainted();
}


void CPUDisassembly::toggleHwBpActionSlot()
{
    duint wVA = rvaToVa(getInitialSelection());
    BPXTYPE wBpType = DbgGetBpxTypeAt(wVA);
    QString wCmd;

    if((wBpType & bp_hardware) == bp_hardware)
    {
        wCmd = "bphwc " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }
    else
    {
        wCmd = "bphws " + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    }

    DbgCmdExec(wCmd.toUtf8().constData());
}


void CPUDisassembly::setHwBpOnSlot0ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 0);
}

void CPUDisassembly::setHwBpOnSlot1ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 1);
}

void CPUDisassembly::setHwBpOnSlot2ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 2);
}

void CPUDisassembly::setHwBpOnSlot3ActionSlot()
{
    setHwBpAt(rvaToVa(getInitialSelection()), 3);
}

void CPUDisassembly::setHwBpAt(duint va, int slot)
{
    int wI = 0;
    int wSlotIndex = -1;
    BPMAP wBPList;
    QString wCmd = "";

    DbgGetBpList(bp_hardware, &wBPList);

    // Find index of slot slot in the list
    for(wI = 0; wI < wBPList.count; wI++)
    {
        if(wBPList.bp[wI].slot == (unsigned short)slot)
        {
            wSlotIndex = wI;
            break;
        }
    }

    if(wSlotIndex < 0) // Slot not used
    {
        wCmd = "bphws " + QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    else // Slot used
    {
        wCmd = "bphwc " + QString("%1").arg((duint)(wBPList.bp[wSlotIndex].addr), sizeof(duint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());

        Sleep(200);

        wCmd = "bphws " + QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(wCmd.toUtf8().constData());
    }
    if(wBPList.count)
        BridgeFree(wBPList.bp);
}

void CPUDisassembly::setNewOriginHereActionSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    QString wCmd = "cip=" + QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(wCmd.toUtf8().constData());
}

void CPUDisassembly::setLabelSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text = QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("DbgSetLabelAt failed!"));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }

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
    else if(instr_info.type & TYPE_VALUE || instr_info.type & TYPE_ADDR)
        addr = instr_info.addr;
    else
        return;

    LineEditDialog mLineEdit(this);
    QString addr_text = ToPtrString(addr);
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt(addr, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle(tr("Add label at ") + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(addr, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("DbgSetLabelAt failed!"));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }

    GuiUpdateAllViews();
}

void CPUDisassembly::setCommentSlot()
{
    if(!DbgIsDebugging())
        return;
    duint wVA = rvaToVa(getInitialSelection());
    LineEditDialog mLineEdit(this);
    QString addr_text = QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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
    if(!DbgSetCommentAt(wVA, mLineEdit.editText.replace('\r', "").replace('\n', "").toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("DbgSetCommentAt failed!"));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
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
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
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
        QString start_text = QString("%1").arg(start, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        QString end_text = QString("%1").arg(end, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        char labeltext[MAX_LABEL_SIZE] = "";
        QString label_text = "";
        if(DbgGetLabelAt(start, SEG_DEFAULT, labeltext))
            label_text = " (" + QString(labeltext) + ")";

        QMessageBox msg(QMessageBox::Question, tr("Define function?"), start_text + "-" + end_text + label_text, QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/compile.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() != QMessageBox::Yes)
            return;
        QString cmd = "functionadd " + start_text + "," + end_text;
        DbgCmdExec(cmd.toUtf8().constData());
    }
    else
    {
        for(duint i = start; i <= end; i++)
        {
            if(DbgFunctionGet(i, &function_start, &function_end))
                break;
        }
        QString start_text = QString("%1").arg(function_start, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        QString end_text = QString("%1").arg(function_end, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        char labeltext[MAX_LABEL_SIZE] = "";
        QString label_text = "";
        if(DbgGetLabelAt(function_start, SEG_DEFAULT, labeltext))
            label_text = " (" + QString(labeltext) + ")";

        QMessageBox msg(QMessageBox::Warning, tr("Delete function?"), start_text + "-" + end_text + label_text, QMessageBox::Ok | QMessageBox::Cancel);
        msg.setDefaultButton(QMessageBox::Cancel);
        msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() != QMessageBox::Ok)
            return;
        QString cmd = "functiondel " + start_text;
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
        QString addr_text = QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();

        Instruction_t instr = this->DisassembleAt(wRVA);

        QString actual_inst = instr.instStr;

        bool assembly_error;
        do
        {
            char error[MAX_ERROR_SIZE] = "";

            assembly_error = false;

            assembleDialog.setSelectedInstrVa(wVA);
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
                msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
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
    if(mGoto->exec() == QDialog::Accepted)
    {
        DbgCmdExec(QString().sprintf("disasm \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
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
    GotoDialog mGotoDialog(this);
    mGotoDialog.fileOffset = true;
    mGotoDialog.modName = QString(modname);
    mGotoDialog.setWindowTitle(tr("Goto File Offset in ") + QString(modname));
    if(mGotoDialog.exec() != QDialog::Accepted)
        return;
    duint value = DbgValFromString(mGotoDialog.expressionText.toUtf8().constData());
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

void CPUDisassembly::gotoXrefSlot()
{
    if(!DbgIsDebugging() || !mXrefInfo.refcount)
        return;
    XrefBrowseDialog xrefDlg(this, getSelectedVa());
    xrefDlg.exec();
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
        QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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
    QString addrStart = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString addrEnd = QString("%1").arg(rvaToVa(getSelectionEnd()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString addrDisasm = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
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

    WordEditDialog wordEdit(this);
    wordEdit.setup("Enter Constant", 0, sizeof(dsint));
    if(wordEdit.exec() != QDialog::Accepted) //cancel pressed
        return;
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString constText = QString("%1").arg(wordEdit.getVal(), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findref " + constText + ", " + addrText + ", 0, %1").arg(refFindType).toUtf8().constData());
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

    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("strref " + addrText + ", 0, %1").arg(refFindType).toUtf8().constData());
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

    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("modcallfind " + addrText + ", 0, 0").toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDisassembly::findPatternSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = QString("%1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern()).toUtf8().constData());
    emit displayReferencesWidget();
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
    hexEdit.setWindowTitle("Edit code at " + QString("%1").arg(rvaToVa(selStart), sizeof(dsint) * 2, 16, QChar('0')).toUpper());
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
    hexEdit.setWindowTitle("Fill code at " + QString("%1").arg(rvaToVa(selStart), sizeof(dsint) * 2, 16, QChar('0')).toUpper());
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
        QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(QString("yara \"%0\",%1").arg(yaraDialog.getSelectedFile()).arg(addrText).toUtf8().constData());
        emit displayReferencesWidget();
    }
}

void CPUDisassembly::copySelectionSlot(bool copyBytes)
{
    QList<Instruction_t> instBuffer;
    prepareDataRange(getSelectionStart(), getSelectionEnd(), &instBuffer);
    QString clipboard = "";
    const int addressLen = getColumnWidth(0) / getCharWidth() - 1;
    const int bytesLen = getColumnWidth(1) / getCharWidth() - 1;
    const int disassemblyLen = getColumnWidth(2) / getCharWidth() - 1;
    for(int i = 0; i < instBuffer.size(); i++)
    {
        if(i)
            clipboard += "\r\n";
        dsint cur_addr = rvaToVa(instBuffer.at(i).rva);
        QString address = getAddrText(cur_addr, 0);
        QString bytes;
        for(int j = 0; j < instBuffer.at(i).dump.size(); j++)
        {
            if(j)
                bytes += " ";
            bytes += QString("%1").arg((unsigned char)(instBuffer.at(i).dump.at(j)), 2, 16, QChar('0')).toUpper();
        }
        QString disassembly;
        for(const auto & token : instBuffer.at(i).tokens.tokens)
            disassembly += token.text;
        QString fullComment;
        QString comment;
        if(GetCommentFormat(cur_addr, comment))
            fullComment = " " + comment;
        clipboard += address.leftJustified(addressLen, QChar(' '), true);
        if(copyBytes)
            clipboard += " | " + bytes.leftJustified(bytesLen, QChar(' '), true);
        clipboard += " | " + disassembly.leftJustified(disassemblyLen, QChar(' '), true) + " |" + fullComment;
    }
    Bridge::CopyToClipboard(clipboard);
}

void CPUDisassembly::copySelectionSlot()
{
    copySelectionSlot(true);
}

void CPUDisassembly::copySelectionNoBytesSlot()
{
    copySelectionSlot(false);
}

void CPUDisassembly::copyAddressSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    Bridge::CopyToClipboard(addrText);
}

void CPUDisassembly::copyRvaSlot()
{
    duint addr = rvaToVa(getInitialSelection());
    duint base = DbgFunctions()->ModBaseFromAddr(addr);
    if(base)
    {
        QString addrText = QString("%1").arg(addr - base, 0, 16, QChar('0')).toUpper();
        Bridge::CopyToClipboard(addrText);
    }
    else
        QMessageBox::warning(this, "Error!", "Selection not in a module...");
}

void CPUDisassembly::copyDisassemblySlot()
{
    QList<Instruction_t> instBuffer;
    prepareDataRange(getSelectionStart(), getSelectionEnd(), &instBuffer);
    QString clipboard = "";
    for(int i = 0; i < instBuffer.size(); i++)
    {
        if(i)
            clipboard += "\r\n";
        for(const auto & token : instBuffer.at(i).tokens.tokens)
            clipboard += token.text;
    }
    Bridge::CopyToClipboard(clipboard);
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

    LineEditDialog mLineEdit(this);
    mLineEdit.enableCheckBox(false);
    //    mLineEdit.setCheckBoxText("Entire &Block");
    //    mLineEdit.setCheckBox(ConfigBool("Disassembler", "FindCommandEntireBlock"));
    mLineEdit.setWindowTitle("Find Command");
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    Config()->setBool("Disassembler", "FindCommandEntireBlock", mLineEdit.bChecked);

    char error[MAX_ERROR_SIZE] = "";
    unsigned char dest[16];
    int asmsize = 0;
    duint va = rvaToVa(getInitialSelection());

    if(!DbgFunctions()->Assemble(va + mMemPage->getSize() / 2, dest, &asmsize, mLineEdit.editText.toUtf8().constData(), error))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to assemble instruction \"" + mLineEdit.editText + "\" (" + error + ")");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QString addr_text = QString("%1").arg(va, sizeof(dsint) * 2, 16, QChar('0')).toUpper();

    dsint size = mMemPage->getSize();
    DbgCmdExec(QString("findasm \"%1\", %2, .%3, %4").arg(mLineEdit.editText).arg(addr_text).arg(size).arg(refFindType).toUtf8().constData());

    emit displayReferencesWidget();
}

void CPUDisassembly::openSourceSlot()
{
    char szSourceFile[MAX_STRING_SIZE] = "";
    int line = 0;
    if(!DbgFunctions()->GetSourceFromAddr(rvaToVa(getInitialSelection()), szSourceFile, &line))
        return;
    Bridge::getBridge()->emitLoadSourceFile(szSourceFile, 0, line);
    emit displaySourceManagerWidget();
}

void CPUDisassembly::decompileSelectionSlot()
{
    dsint addr = rvaToVa(getSelectionStart());
    dsint size = getSelectionSize();
    emit displaySnowmanWidget();
    emit decompileAt(addr, addr + size);
}

void CPUDisassembly::decompileFunctionSlot()
{
    dsint addr = rvaToVa(getInitialSelection());
    duint start;
    duint end;
    if(DbgFunctionGet(addr, &start, &end))
    {
        emit displaySnowmanWidget();
        emit decompileAt(start, end);
    }
}

void CPUDisassembly::displayWarningSlot(QString title, QString text)
{
    QMessageBox msg(QMessageBox::Warning, title, text, QMessageBox::Ok);
    msg.setParent(this, Qt::Dialog);
    msg.setWindowIcon(QIcon(":/icons/images/compile-warning.png"));
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void CPUDisassembly::paintEvent(QPaintEvent* event)
{
    // Hook/hack to update the sidebar at the same time as this widget.
    // Ensures the two widgets are synced and prevents "draw lag"
    auto sidebar = mParentCPUWindow->getSidebarWidget();

    if(sidebar)
        sidebar->repaint();

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

void CPUDisassembly::editSoftBpActionSlot()
{
    Breakpoints::editBP(bp_normal, ToHexString(rvaToVa(getInitialSelection())), this);
}

void CPUDisassembly::ActionTraceRecordBitSlot()
{
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
}

void CPUDisassembly::ActionTraceRecordByteSlot()
{
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
}

void CPUDisassembly::ActionTraceRecordWordSlot()
{
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
}

void CPUDisassembly::ActionTraceRecordDisableSlot()
{
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
    BASIC_INSTRUCTION_INFO disasm;
    DbgDisasmFastAt(rvaToVa(getInitialSelection()), &disasm);
    if(!*disasm.instruction)
        return;
    char* space = strstr(disasm.instruction, " ");
    if(space)
        *space = '\0';
    DbgCmdExecDirect(QString("mnemonichelp %1").arg(disasm.instruction).toUtf8().constData());
    emit displayLogWidget();
}

void CPUDisassembly::analyzeSingleFunctionSlot()
{
    DbgCmdExec(QString("analr %1").arg(ToHexString(rvaToVa(getInitialSelection()))).toUtf8().constData());
}

void CPUDisassembly::removeAnalysisSelectionSlot()
{
    mDisasm->getEncodeMap()->delRange(rvaToVa(getSelectionStart()), getSelectionSize());
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::removeAnalysisModuleSlot()
{
    mDisasm->getEncodeMap()->delSegment(rvaToVa(getSelectionStart()));
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::setEncodeTypeRangeSlot()
{
    QAction* pAction = qobject_cast<QAction*>(sender());
    mDisasm->getEncodeMap()->setDataType(rvaToVa(getSelectionStart()), getSelectionSize(), (ENCODETYPE)pAction->data().toUInt());
    GuiUpdateDisassemblyView();
}

void CPUDisassembly::setEncodeTypeSlot()
{
    QAction* pAction = qobject_cast<QAction*>(sender());
    mDisasm->getEncodeMap()->setDataType(rvaToVa(getSelectionStart()), (ENCODETYPE)pAction->data().toUInt());
    GuiUpdateDisassemblyView();
}
