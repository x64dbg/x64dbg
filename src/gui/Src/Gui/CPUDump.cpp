#include "CPUDump.h"
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QToolTip>
#include "Configuration.h"
#include "Bridge.h"
#include "HexEditDialog.h"
#include "CPUMultiDump.h"
#include "GotoDialog.h"
#include "CPUDisassembly.h"
#include "CommonActions.h"
#include "WordEditDialog.h"
#include "CodepageSelectionDialog.h"
#include "MiscUtil.h"
#include "BackgroundFlickerThread.h"

CPUDump::CPUDump(CPUMultiDump* multiDump, CPUDisassembly* disassembly, QWidget* parent)
    : HexDump(multiDump->getArchitecture(), parent),
      mMultiDump(multiDump),
      mDisassembly(disassembly)
{
    duint setting;
    if(BridgeSettingGetUint("Gui", "AsciiSeparator", &setting))
        mAsciiSeparator = setting & 0xF;

    setView((ViewEnum_t)ConfigUint("HexDump", "DefaultView"));

    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));
    connect(this, SIGNAL(headerButtonReleased(duint)), this, SLOT(headerButtonReleasedSlot(duint)));

    mPluginMenu = multiDump->mDumpPluginMenu;

    setupContextMenu();
}

void CPUDump::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });

    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getSelectionStart());
    });

    MenuBuilder* binaryMenu = new MenuBuilder(this);
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_edit"), tr("&Edit"), SLOT(binaryEditSlot()), "ActionBinaryEdit"));
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_fill"), tr("&Fill..."), SLOT(binaryFillSlot()), "ActionBinaryFill"));
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
    binaryMenu->addAction(makeShortcutAction(DIcon("binary_save"), tr("Save To a File"), SLOT(binarySaveToFileSlot()), "ActionBinarySave"));
    mMenuBuilder->addMenu(makeMenu(DIcon("binary"), tr("B&inary")), binaryMenu);

    MenuBuilder* copyMenu = new MenuBuilder(this);
    copyMenu->addAction(mCopySelection);
    copyMenu->addAction(mCopyAddress);
    copyMenu->addAction(mCopyRva, [this](QMenu*)
    {
        return DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection())) != 0;
    });
    copyMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("&File Offset"), SLOT(copyFileOffsetSlot()), "ActionCopyFileOffset"), [this](QMenu*)
    {
        return DbgFunctions()->VaToFileOffset(rvaToVa(getInitialSelection())) != 0;
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), copyMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        return DbgFunctions()->PatchInRange(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd()));
    });

    mCommonActions->build(mMenuBuilder, CommonActions::ActionDisasm | CommonActions::ActionMemoryMap | CommonActions::ActionDumpData | CommonActions::ActionDumpN
                          | CommonActions::ActionDisasmData | CommonActions::ActionStackDump | CommonActions::ActionLabel | CommonActions::ActionWatch);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("modify"), tr("&Modify Value"), SLOT(modifyValueSlot()), "ActionModifyValue"), [this](QMenu*)
    {
        auto d = mDescriptor.at(0).data;
        return getSizeOf(d.itemSize) <= sizeof(duint) || (d.itemSize == 4 && d.dwordMode == FloatDword || d.itemSize == 8 && d.qwordMode == DoubleQword);
    });

    MenuBuilder* breakpointMenu = new MenuBuilder(this);
    MenuBuilder* hardwareAccessMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_hardware) == 0;
    });
    MenuBuilder* hardwareWriteMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_hardware) == 0;
    });
    MenuBuilder* memoryAccessMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_memory) == 0;
    });
    MenuBuilder* memoryReadMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_memory) == 0;
    });
    MenuBuilder* memoryWriteMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_memory) == 0;
    });
    MenuBuilder* memoryExecuteMenu = new MenuBuilder(this, [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_memory) == 0;
    });
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_byte"), tr("&Byte"), "bphws $, r, 1"));
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_word"), tr("&Word"), "bphws $, r, 2"));
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_dword"), tr("&Dword"), "bphws $, r, 4"));
#ifdef _WIN64
    hardwareAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_qword"), tr("&Qword"), "bphws $, r, 8"));
#endif //_WIN64
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_byte"), tr("&Byte"), "bphws $, w, 1"));
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_word"), tr("&Word"), "bphws $, w, 2"));
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_dword"), tr("&Dword"), "bphws $, w, 4"));
#ifdef _WIN64
    hardwareWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_qword"), tr("&Qword"), "bphws $, w, 8"));
#endif //_WIN64
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_access"), tr("Hardware, &Access")), hardwareAccessMenu);
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_write"), tr("Hardware, &Write")), hardwareWriteMenu);
    breakpointMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_execute"), tr("Hardware, &Execute"), "bphws $, x"), [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_hardware) == 0;
    });
    breakpointMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_remove"), tr("Remove &Hardware"), "bphwc $"), [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_hardware) != 0;
    });
    breakpointMenu->addSeparator();
    memoryAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, a"));
    memoryAccessMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, a"));
    memoryReadMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, r"));
    memoryReadMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, r"));
    memoryWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, w"));
    memoryWriteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, w"));
    memoryExecuteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_singleshoot"), tr("&Singleshoot"), "bpm $, 0, x"));
    memoryExecuteMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_memory_restore_on_hit"), tr("&Restore on hit"), "bpm $, 1, x"));
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_memory_access"), tr("Memory, Access")), memoryAccessMenu);
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_memory_read"), tr("Memory, Read")), memoryReadMenu);
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_memory_write"), tr("Memory, Write")), memoryWriteMenu);
    breakpointMenu->addMenu(makeMenu(DIcon("breakpoint_memory_execute"), tr("Memory, Execute")), memoryExecuteMenu);
    breakpointMenu->addAction(mCommonActions->makeCommandAction(DIcon("breakpoint_remove"), tr("Remove &Memory"), "bpmc $"), [this](QMenu*)
    {
        return (DbgGetBpxTypeAt(rvaToVa(getSelectionStart())) & bp_memory) != 0;
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("breakpoint"), tr("&Breakpoint")), breakpointMenu);

    mMenuBuilder->addAction(makeShortcutAction(DIcon("search-for"), tr("&Find Pattern..."), SLOT(findPattern()), "ActionFindPattern"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("find"), tr("Find &References"), SLOT(findReferencesSlot()), "ActionFindReferences"));

    mMenuBuilder->addAction(makeShortcutAction(DIcon("sync"), tr("&Sync with expression"), SLOT(syncWithExpressionSlot()), "ActionSync"));
    mMenuBuilder->addAction(makeShortcutAction(DIcon("memmap_alloc_memory"), tr("Allocate Memory"), SLOT(allocMemorySlot()), "ActionAllocateMemory"));

    MenuBuilder* gotoMenu = new MenuBuilder(this);
    gotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("&Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    gotoMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("File Offset"), SLOT(gotoFileOffsetSlot()), "ActionGotoFileOffset"));
    gotoMenu->addAction(makeShortcutAction(DIcon("top"), tr("Start of Page"), SLOT(gotoStartSlot()), "ActionGotoStart"), [this](QMenu*)
    {
        return getSelectionStart() != 0;
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("bottom"), tr("End of Page"), SLOT(gotoEndSlot()), "ActionGotoEnd"));
    gotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("prevref"), tr("Previous Reference"), SLOT(gotoPreviousReferenceSlot()), "ActionGotoPreviousReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__dump_refindex > 0 || dump.sel() != refsearch.addr($__dump_refindex))");
    });
    gotoMenu->addAction(makeShortcutAction(DIcon("nextref"), tr("Next Reference"), SLOT(gotoNextReferenceSlot()), "ActionGotoNextReference"), [](QMenu*)
    {
        return !!DbgEval("refsearch.count() && ($__dump_refindex < refsearch.count() || dump.sel() != refsearch.addr($__dump_refindex))");
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("goto"), tr("&Go to")), gotoMenu);
    mMenuBuilder->addSeparator();

    MenuBuilder* hexMenu = new MenuBuilder(this);
    hexMenu->addAction(makeAction(DIcon("ascii"), tr("&ASCII"), SLOT(hexAsciiSlot())));
    hexMenu->addAction(makeAction(DIcon("ascii-extended"), tr("&Extended ASCII"), SLOT(hexUnicodeSlot())));
    QAction* hexLastCodepage = makeAction(DIcon("codepage"), "?", SLOT(hexLastCodepageSlot()));
    hexMenu->addAction(hexLastCodepage, [hexLastCodepage](QMenu*)
    {
        duint lastCodepage;
        auto allCodecs = QTextCodec::availableCodecs();
        if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
            return false;
        hexLastCodepage->setText(QString::fromLocal8Bit(allCodecs.at(lastCodepage)));
        return true;
    });
    hexMenu->addAction(makeAction(DIcon("codepage"), tr("&Codepage..."), SLOT(hexCodepageSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("hex"), tr("&Hex")), hexMenu);

    MenuBuilder* textMenu = new MenuBuilder(this);
    textMenu->addAction(makeAction(DIcon("ascii"), tr("&ASCII"), SLOT(textAsciiSlot())));
    textMenu->addAction(makeAction(DIcon("ascii-extended"), tr("&Extended ASCII"), SLOT(textUnicodeSlot())));
    QAction* textLastCodepage = makeAction(DIcon("codepage"), "?", SLOT(textLastCodepageSlot()));
    textMenu->addAction(textLastCodepage, [textLastCodepage](QMenu*)
    {
        duint lastCodepage;
        auto allCodecs = QTextCodec::availableCodecs();
        if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
            return false;
        textLastCodepage->setText(QString::fromLocal8Bit(allCodecs.at(lastCodepage)));
        return true;
    });
    textMenu->addAction(makeAction(DIcon("codepage"), tr("&Codepage..."), SLOT(textCodepageSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("strings"), tr("&Text")), textMenu);

    MenuBuilder* integerMenu = new MenuBuilder(this);
    integerMenu->addAction(makeAction(DIcon("byte"), tr("Signed byte (8-bit)"), SLOT(integerSignedByteSlot())));
    integerMenu->addAction(makeAction(DIcon("word"), tr("Signed short (16-bit)"), SLOT(integerSignedShortSlot())));
    integerMenu->addAction(makeAction(DIcon("dword"), tr("Signed long (32-bit)"), SLOT(integerSignedLongSlot())));
    integerMenu->addAction(makeAction(DIcon("qword"), tr("Signed long long (64-bit)"), SLOT(integerSignedLongLongSlot())));
    integerMenu->addAction(makeAction(DIcon("byte"), tr("Unsigned byte (8-bit)"), SLOT(integerUnsignedByteSlot())));
    integerMenu->addAction(makeAction(DIcon("word"), tr("Unsigned short (16-bit)"), SLOT(integerUnsignedShortSlot())));
    integerMenu->addAction(makeAction(DIcon("dword"), tr("Unsigned long (32-bit)"), SLOT(integerUnsignedLongSlot())));
    integerMenu->addAction(makeAction(DIcon("qword"), tr("Unsigned long long (64-bit)"), SLOT(integerUnsignedLongLongSlot())));
    integerMenu->addAction(makeAction(DIcon("word"), tr("Hex short (16-bit)"), SLOT(integerHexShortSlot())));
    integerMenu->addAction(makeAction(DIcon("dword"), tr("Hex long (32-bit)"), SLOT(integerHexLongSlot())));
    integerMenu->addAction(makeAction(DIcon("qword"), tr("Hex long long (64-bit)"), SLOT(integerHexLongLongSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("integer"), tr("&Integer")), integerMenu);

    MenuBuilder* floatMenu = new MenuBuilder(this);
    floatMenu->addAction(makeAction(DIcon("32bit-float"), tr("&Float (32-bit)"), SLOT(floatFloatSlot())));
    floatMenu->addAction(makeAction(DIcon("64bit-float"), tr("&Double (64-bit)"), SLOT(floatDoubleSlot())));
    floatMenu->addAction(makeAction(DIcon("80bit-float"), tr("&Long double (80-bit)"), SLOT(floatLongDoubleSlot())));
    floatMenu->addAction(makeAction(DIcon("word"), tr("&Half float (16-bit)"), SLOT(floatHalfSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("float"), tr("&Float")), floatMenu);

    mMenuBuilder->addAction(makeAction(DIcon("address"), tr("&Address"), SLOT(addressAsciiSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("processor-cpu"), tr("&Disassembly"), SLOT(disassemblySlot())));

    mMenuBuilder->addSeparator();
    mMenuBuilder->addAction(makeAction(DIcon("visitstruct"), tr("Display type"), SLOT(visitSlot())));

    mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    {
        DbgMenuPrepare(GUI_DUMP_MENU);
        menu->addActions(mPluginMenu->actions());
        return true;
    }));

    mMenuBuilder->loadFromConfig();
    updateShortcuts();
}

void CPUDump::getAttention()
{
    BackgroundFlickerThread* thread = new BackgroundFlickerThread(this, mBackgroundColor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}

void CPUDump::getColumnRichText(duint col, duint rva, RichTextPainter::List & richText)
{
    if(col && !mDescriptor.at(col - 1).isData && mDescriptor.at(col - 1).itemCount) //print comments
    {
        RichTextPainter::CustomRichText_t curData;
        curData.flags = RichTextPainter::FlagColor;
        curData.textColor = mTextColor;
        duint data = 0;
        mMemPage->read((byte_t*)&data, rva, sizeof(duint));

        char modname[MAX_MODULE_SIZE] = "";
        if(!DbgGetModuleAt(data, modname))
            modname[0] = '\0';
        char label_text[MAX_LABEL_SIZE] = "";
        char string_text[MAX_STRING_SIZE] = "";
        if(DbgGetLabelAt(data, SEG_DEFAULT, label_text))
            curData.text = QString(modname) + "." + QString(label_text);
        else if(DbgGetStringAt(data, string_text))
            curData.text = string_text;

        if(!curData.text.length()) //stack comments
        {
            auto va = rvaToVa(rva);
            duint stackSize;
            duint csp = DbgValFromString("csp");
            duint stackBase = DbgMemFindBaseAddr(csp, &stackSize);
            STACK_COMMENT comment;
            if(va >= stackBase && va < stackBase + stackSize && DbgStackCommentGet(va, &comment))
            {
                if(va >= csp) //active stack
                {
                    if(*comment.color)
                        curData.textColor = QColor(QString(comment.color));
                }
                else
                    curData.textColor = ConfigColor("StackInactiveTextColor");
                curData.text = comment.comment;
            }
            else if(*modname)
            {
                curData.text = QString("%1.%2").arg(modname, ToPtrString(va));
            }
        }
        if(curData.text.length())
            richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

QString CPUDump::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    // TODO: wtf?
    if(getTableOffset() == 0 && mByteOffset != 0)
        printDumpAt(mMemPage->getBase(), false, false);

    if(!col) //address
    {
        char label[MAX_LABEL_SIZE] = "";
        dsint cur_addr = rvaToVa(row * getBytePerRowCount() - mByteOffset);
        QColor background;
        if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) //label
        {
            background = ConfigColor("HexDumpLabelBackgroundColor");
            painter->setPen(ConfigColor("HexDumpLabelColor")); //TODO: config
        }
        else
        {
            background = ConfigColor("HexDumpAddressBackgroundColor");
            painter->setPen(ConfigColor("HexDumpAddressColor")); //TODO: config
        }
        if(background.alpha())
            painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill background color
        painter->drawText(QRect(x + 4, y, w - 4, h), Qt::AlignVCenter | Qt::AlignLeft, makeAddrText(cur_addr));
        return QString();
    }
    return HexDump::paintContent(painter, row, col, x, y, w, h);
}

void CPUDump::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    mMenuBuilder->build(&menu);
    menu.exec(event->globalPos());
}

void CPUDump::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton || !DbgIsDebugging())
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        //very ugly way to calculate the base of the current row (no clue why it works)
        auto deltaRowBase = getInitialSelection() % getBytePerRowCount() + mByteOffset;
        if(deltaRowBase >= getBytePerRowCount())
            deltaRowBase -= getBytePerRowCount();
        dsint mSelectedVa = rvaToVa(getInitialSelection() - deltaRowBase);
        if(mRvaDisplayEnabled && mSelectedVa == mRvaDisplayBase)
            mRvaDisplayEnabled = false;
        else
        {
            mRvaDisplayEnabled = true;
            mRvaDisplayBase = mSelectedVa;
            mRvaDisplayPageBase = mMemPage->getBase();
        }
        reloadData();
    }
    break;

    default:
    {
        auto d = mDescriptor.at(0).data;
        if(getSizeOf(d.itemSize) <= sizeof(duint) || (d.itemSize == 4 && d.dwordMode == FloatDword || d.itemSize == 8 && d.qwordMode == DoubleQword))
            modifyValueSlot();
        else
            binaryEditSlot();
    }
    break;
    }
}

static QString getTooltipForVa(duint va, int depth)
{
    duint ptr = 0;
    if(!DbgMemRead(va, &ptr, sizeof(duint)))
        return QString();

    QString tooltip;
    /* TODO: if this is enabled, make sure the context menu items also work
    // If the VA is not a valid pointer, try to align it
    if(!DbgMemIsValidReadPtr(ptr))
    {
     va -= va % sizeof(duint);
     DbgMemRead(va, &ptr, sizeof(duint));
    }*/

    // Check if its a pointer
    switch(DbgGetEncodeTypeAt(va, 1))
    {
    // Get information about the pointer type
    case enc_unknown:
    default:
        if(DbgMemIsValidReadPtr(ptr) && depth >= 0)
        {
            tooltip = QString("[%1] = %2").arg(ToPtrString(ptr), getTooltipForVa(ptr, depth - 1));
        }
        // If not a pointer, hide tooltips
        else
        {
            bool isCodePage;
            isCodePage = DbgFunctions()->MemIsCodePage(va, false);
            char disassembly[GUI_MAX_DISASSEMBLY_SIZE];
            if(isCodePage)
            {
                if(GuiGetDisassembly(va, disassembly))
                    tooltip = QString::fromUtf8(disassembly);
                else
                    tooltip = "";
            }
            else
                tooltip = QString("[%1] = %2").arg(ToPtrString(va)).arg(ToPtrString(ptr));
            if(DbgFunctions()->ModGetParty(va) == 1)
                tooltip += " (" + (isCodePage ? CPUDump::tr("System Code") : CPUDump::tr("System Data")) + ")";
            else
                tooltip += " (" + (isCodePage ? CPUDump::tr("User Code") : CPUDump::tr("User Data")) + ")";
        }
        break;
    case enc_code:
        char disassembly[GUI_MAX_DISASSEMBLY_SIZE];
        if(GuiGetDisassembly(va, disassembly))
            tooltip = QString::fromUtf8(disassembly);
        else
            tooltip = "";
        if(DbgFunctions()->ModGetParty(va) == 1)
            tooltip += " (" + CPUDump::tr("System Code") + ")";
        else
            tooltip += " (" + CPUDump::tr("User Code") + ")";
        break;
    case enc_real4:
        tooltip = ToFloatString(&va) + CPUDump::tr(" (Real4)");
        break;
    case enc_real8:
        double numd;
        DbgMemRead(va, &numd, sizeof(double));
        tooltip = ToDoubleString(&numd) + CPUDump::tr(" (Real8)");
        break;
    case enc_byte:
        tooltip = ToByteString(va) + CPUDump::tr(" (BYTE)");
        break;
    case enc_word:
        tooltip = ToWordString(va) + CPUDump::tr(" (WORD)");
        break;
    case enc_dword:
        tooltip = QString("%1").arg((unsigned int)va, 8, 16, QChar('0')).toUpper() + CPUDump::tr(" (DWORD)");
        break;
    case enc_qword:
#ifdef _WIN64
        tooltip = QString("%1").arg((unsigned long long)va, 16, 16, QChar('0')).toUpper() + CPUDump::tr(" (QWORD)");
#else //x86
        unsigned long long qword;
        qword = 0;
        DbgMemRead(va, &qword, 8);
        tooltip = QString("%1").arg((unsigned long long)qword, 16, 16, QChar('0')).toUpper() + CPUDump::tr(" (QWORD)");
#endif //_WIN64
        break;
    case enc_ascii:
    case enc_unicode:
        char str[MAX_STRING_SIZE];
        if(DbgGetStringAt(va, str))
            tooltip = QString::fromUtf8(str) + CPUDump::tr(" (String)");
        else
            tooltip = CPUDump::tr("(Unknown String)");
        break;
    }
    return tooltip;
}

void CPUDump::mouseMoveEvent(QMouseEvent* event)
{
    // Get mouse pointer relative position
    int x = event->x();
    int y = event->y();

    // Get HexDump own RVA address, then VA in memory
    auto va = rvaToVa(getItemStartingAddress(x, y));

    // Read VA
    QToolTip::showText(event->globalPos(), getTooltipForVa(va, 4));

    HexDump::mouseMoveEvent(event);
}

void CPUDump::modifyValueSlot()
{
    dsint addr = getSelectionStart();
    auto d = mDescriptor.at(0).data;
    if(d.itemSize == 4 && d.dwordMode == FloatDword || d.itemSize == 8 && d.qwordMode == DoubleQword)
    {
        auto size = std::min(getSizeOf(mDescriptor.at(0).data.itemSize), sizeof(double));
        if(size == 4)
        {
            float value;
            mMemPage->read(&value, addr, size);
            QString current = QString::number(value);
            QString newvalue;
            if(SimpleInputBox(this, tr("Modify value"), current, newvalue, current))
            {
                bool ok;
                value = newvalue.toFloat(&ok);
                if(ok)
                    mMemPage->write(&value, addr, size);
                else
                    SimpleErrorBox(this, tr("Error"), tr("The input text is not a number!"));
            }
        }
        else if(size == 8)
        {
            double value;
            mMemPage->read(&value, addr, size);
            QString current = QString::number(value);
            QString newvalue;
            if(SimpleInputBox(this, tr("Modify value"), current, newvalue, current))
            {
                bool ok;
                value = newvalue.toDouble(&ok);
                if(ok)
                    mMemPage->write(&value, addr, size);
                else
                    SimpleErrorBox(this, tr("Error"), tr("The input text is not a number!"));
            }
        }
    }
    else
    {
        auto size = std::min(getSizeOf(mDescriptor.at(0).data.itemSize), sizeof(dsint));
        WordEditDialog editDialog(this);
        dsint value = 0;
        mMemPage->read(&value, addr, size);
        editDialog.setup(tr("Modify value"), value, (int)size);
        if(editDialog.exec() != QDialog::Accepted)
            return;
        value = editDialog.getVal();
        mMemPage->write(&value, addr, size);
    }
    GuiUpdateAllViews();
}

void CPUDump::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setWindowTitle(tr("Enter expression to follow in Dump..."));
    mGoto->setInitialExpression(ToPtrString(rvaToVa(getInitialSelection())));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        DbgCmdExec(QString().sprintf("dump %p", value));
    }
}

void CPUDump::gotoFileOffsetSlot()
{
    if(!DbgIsDebugging())
        return;
    char modname[MAX_MODULE_SIZE] = "";
    if(!DbgFunctions()->ModNameFromAddr(rvaToVa(getSelectionStart()), modname, true))
    {
        SimpleErrorBox(this, tr("Error!"), tr("Not inside a module..."));
        return;
    }
    if(!mGotoOffset)
        mGotoOffset = new GotoDialog(this);
    mGotoOffset->fileOffset = true;
    mGotoOffset->modName = QString(modname);
    mGotoOffset->setWindowTitle(tr("Goto File Offset in %1").arg(QString(modname)));
    duint addr = rvaToVa(getInitialSelection());
    duint offset = DbgFunctions()->VaToFileOffset(addr);
    if(offset)
        mGotoOffset->setInitialExpression(ToHexString(offset));
    if(mGotoOffset->exec() != QDialog::Accepted)
        return;
    duint value = DbgValFromString(mGotoOffset->expressionText.toUtf8().constData());
    value = DbgFunctions()->FileOffsetToVa(modname, value);
    DbgCmdExec(QString().sprintf("dump \"%p\"", value));
}

void CPUDump::gotoStartSlot()
{
    duint dest = mMemPage->getBase();
    DbgCmdExec(QString().sprintf("dump \"%p\"", dest));
}

void CPUDump::gotoEndSlot()
{
    duint dest = mMemPage->getBase() + mMemPage->getSize() - (getViewableRowsCount() * getBytePerRowCount());
    DbgCmdExec(QString().sprintf("dump \"%p\"", dest));
}

void CPUDump::gotoPreviousReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__dump_refindex"), addr = DbgEval("refsearch.addr($__dump_refindex)");
    if(count)
    {
        if(index > 0 && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__dump_refindex", index - 1);
        DbgCmdExec("dump refsearch.addr($__dump_refindex)");
        GuiReferenceSetSingleSelection(int(DbgEval("$__dump_refindex")), false);
    }
}

void CPUDump::gotoNextReferenceSlot()
{
    auto count = DbgEval("refsearch.count()"), index = DbgEval("$__dump_refindex"), addr = DbgEval("refsearch.addr($__dump_refindex)");
    if(count)
    {
        if(index + 1 < count && addr == rvaToVa(getInitialSelection()))
            DbgValToString("$__dump_refindex", index + 1);
        DbgCmdExec("dump refsearch.addr($__dump_refindex)");
        GuiReferenceSetSingleSelection(int(DbgEval("$__dump_refindex")), false);
    }
}

void CPUDump::hexAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex byte
    colDesc.itemCount = 16;
    colDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, colDesc);

    colDesc.isData = true; //ascii byte
    colDesc.itemCount = 16;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 16, tr("ASCII"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::hexUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex byte
    colDesc.itemCount = 16;
    colDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, colDesc);

    colDesc.isData = true; //unicode short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    colDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 8, tr("UNICODE"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::hexCodepageSlot()
{
    CodepageSelectionDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    auto codepage = dialog.getSelectedCodepage();

    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex byte
    colDesc.itemCount = 16;
    colDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, colDesc);

    colDesc.isData = true; //text (in code page)
    colDesc.itemCount = 16;
    colDesc.separator = 0;
    colDesc.textEncoding = codepage;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, codepage, false, colDesc);

    reloadData();
}

void CPUDump::hexLastCodepageSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexCodepage);

    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        return;

    colDesc.isData = true; //hex byte
    colDesc.itemCount = 16;
    colDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, colDesc);

    colDesc.isData = true; //text (in code page)
    colDesc.itemCount = 16;
    colDesc.separator = 0;
    colDesc.textEncoding = allCodecs.at(lastCodepage);
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, allCodecs.at(lastCodepage), false, colDesc);

    reloadData();
}

void CPUDump::textLastCodepageSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextCodepage);

    ColumnDescriptor colDesc;
    DataDescriptor dDesc;
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        return;

    colDesc.isData = true; //text (in code page)
    colDesc.itemCount = 64;
    colDesc.separator = 0;
    colDesc.textEncoding = allCodecs.at(lastCodepage);
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendResetDescriptor(0, allCodecs.at(lastCodepage), false, colDesc);

    reloadData();
}

void CPUDump::textAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //ascii byte
    colDesc.itemCount = 64;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, tr("ASCII"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::textUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //unicode short
    colDesc.itemCount = 64;
    colDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    colDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, tr("UNICODE"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::textCodepageSlot()
{
    CodepageSelectionDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    auto codepage = dialog.getSelectedCodepage();

    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //text (in code page)
    colDesc.itemCount = 64;
    colDesc.separator = 0;
    colDesc.textEncoding = codepage;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendResetDescriptor(0, codepage, false, colDesc);

    reloadData();
}

void CPUDump::integerSignedByteSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedByte);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //signed short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    colDesc.data.itemSize = Byte;
    colDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8 + charwidth * 40, tr("Signed byte (8-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerSignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //signed short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    colDesc.data.itemSize = Word;
    colDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8 + charwidth * 55, tr("Signed short (16-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerSignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //signed long
    colDesc.itemCount = 4;
    colDesc.separator = 0;
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = SignedDecDword;
    appendResetDescriptor(8 + charwidth * 47, tr("Signed long (32-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerSignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //signed long long
    colDesc.itemCount = 2;
    colDesc.separator = 0;
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = SignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, tr("Signed long long (64-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerUnsignedByteSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedByte);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //unsigned short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    colDesc.data.itemSize = Byte;
    colDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8 + charwidth * 32, tr("Unsigned byte (8-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerUnsignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //unsigned short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    colDesc.data.itemSize = Word;
    colDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8 + charwidth * 47, tr("Unsigned short (16-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //unsigned long
    colDesc.itemCount = 4;
    colDesc.separator = 0;
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = UnsignedDecDword;
    appendResetDescriptor(8 + charwidth * 43, tr("Unsigned long (32-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //unsigned long long
    colDesc.itemCount = 2;
    colDesc.separator = 0;
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = UnsignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, tr("Unsigned long long (64-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerHexShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexShort);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex short
    colDesc.itemCount = 8;
    colDesc.separator = 0;
    colDesc.data.itemSize = Word;
    colDesc.data.wordMode = HexWord;
    appendResetDescriptor(8 + charwidth * 39, tr("Hex short (16-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerHexLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex long
    colDesc.itemCount = 4;
    colDesc.separator = 0;
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = HexDword;
    appendResetDescriptor(8 + charwidth * 35, tr("Hex long (32-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::integerHexLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //hex long long
    colDesc.itemCount = 2;
    colDesc.separator = 0;
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = HexQword;
    appendResetDescriptor(8 + charwidth * 33, tr("Hex long long (64-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::floatFloatSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatFloat);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //float dword
    colDesc.itemCount = 4;
    colDesc.separator = 0;
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = FloatDword;
    appendResetDescriptor(8 + charwidth * 55, tr("Float (32-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::floatDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //float qword
    colDesc.itemCount = 2;
    colDesc.separator = 0;
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = DoubleQword;
    appendResetDescriptor(8 + charwidth * 47, tr("Double (64-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::floatLongDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatLongDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //float qword
    colDesc.itemCount = 2;
    colDesc.separator = 0;
    colDesc.data.itemSize = Tword;
    colDesc.data.twordMode = FloatTword;
    appendResetDescriptor(8 + charwidth * 59, tr("Long double (80-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::floatHalfSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatHalf);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //float half
    colDesc.itemCount = 4;
    colDesc.separator = 0;
    colDesc.data.itemSize = Word;
    colDesc.data.wordMode = HalfFloatWord;
    appendResetDescriptor(8 + charwidth * 40, tr("Half float (16-bit)"), false, colDesc);

    colDesc.isData = false; //empty column
    colDesc.itemCount = 0;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, "", false, colDesc);

    reloadData();
}

void CPUDump::addressAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewAddressAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //void*
    colDesc.itemCount = 1;
    colDesc.separator = 0;
#ifdef _WIN64
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = HexQword;
#else
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = HexDword;
#endif
    appendResetDescriptor(8 + charwidth * 2 * sizeof(duint), tr("Value"), false, colDesc);

    colDesc.isData = true;
    colDesc.separator = 0;
#ifdef _WIN64
    colDesc.itemCount = 8;
#else
    colDesc.itemCount = 4;
#endif
    colDesc.data.itemSize = Byte;
    colDesc.data.byteMode = AsciiByte;
    colDesc.columnSwitch = [this]()
    {
        this->setView(ViewAddressUnicode);
    };
    appendDescriptor(8 + charwidth * colDesc.itemCount, tr("ASCII"), true, colDesc);

    colDesc.isData = false; //comments
    colDesc.itemCount = 1;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, tr("Comments"), false, colDesc);

    reloadData();
}

void CPUDump::addressUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewAddressUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor colDesc;
    DataDescriptor dDesc;

    colDesc.isData = true; //void*
    colDesc.itemCount = 1;
    colDesc.separator = 0;
#ifdef _WIN64
    colDesc.data.itemSize = Qword;
    colDesc.data.qwordMode = HexQword;
#else
    colDesc.data.itemSize = Dword;
    colDesc.data.dwordMode = HexDword;
#endif
    appendResetDescriptor(8 + charwidth * 2 * sizeof(duint), tr("Value"), false, colDesc);

    colDesc.isData = true;
    colDesc.separator = 0;
#ifdef _WIN64
    colDesc.itemCount = 4;
#else
    colDesc.itemCount = 2;
#endif
    colDesc.data.itemSize = Word;
    colDesc.data.wordMode = UnicodeWord;
    colDesc.columnSwitch = [this]()
    {
        this->setView(ViewAddressAscii);
    };
    appendDescriptor(8 + charwidth * colDesc.itemCount, tr("UNICODE"), true, colDesc);

    colDesc.isData = false; //comments
    colDesc.itemCount = 1;
    colDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    colDesc.data = dDesc;
    appendDescriptor(0, tr("Comments"), false, colDesc);

    reloadData();
}

void CPUDump::disassemblySlot()
{
    SELECTIONDATA selection;
    selectionGet(&selection);
    emit showDisassemblyTab(selection.start, selection.end, rvaToVa(getTableOffsetRva()));
}

void CPUDump::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void CPUDump::selectionSet(const SELECTIONDATA* selection)
{
    dsint selMin = mMemPage->getBase();
    dsint selMax = selMin + mMemPage->getSize();
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

void CPUDump::findReferencesSlot()
{
    QString addrStart = ToPtrString(rvaToVa(getSelectionStart()));
    QString addrEnd = ToPtrString(rvaToVa(getSelectionEnd()));
    QString addrDisasm = ToPtrString(mDisassembly->rvaToVa(mDisassembly->getSelectionStart()));
    DbgCmdExec(QString("findrefrange " + addrStart + ", " + addrEnd + ", " + addrDisasm));
    emit displayReferencesWidget();
}

void CPUDump::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.showKeepSize(true);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle(tr("Edit data at %1").arg(ToPtrString(rvaToVa(selStart))));
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

void CPUDump::binaryFillSlot()
{
    HexEditDialog hexEdit(this);
    hexEdit.mHexEdit->setOverwriteMode(false);
    dsint selStart = getSelectionStart();
    hexEdit.setWindowTitle(tr("Fill data at %1").arg(ToPtrString(rvaToVa(selStart))));
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

void CPUDump::binaryCopySlot()
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

void CPUDump::binaryPasteSlot()
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

void CPUDump::binaryPasteIgnoreSizeSlot()
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

void CPUDump::binarySaveToFileSlot()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to file"), QDir::currentPath(), tr("All files (*.*)"));
    if(fileName.length())
    {
        // Get starting selection and selection size, then convert selStart to VA
        dsint selStart = getSelectionStart();
        dsint selSize = getSelectionEnd() - selStart + 1;

        // Prepare command
        fileName = QDir::toNativeSeparators(fileName);
        QString cmd = QString("savedata \"%1\",%2,%3").arg(fileName, ToHexString(rvaToVa(selStart)), ToHexString(selSize));
        DbgCmdExec(cmd);
    }
}

void CPUDump::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.isDataCopiable(false);
    hexEdit.showStartFromSelection(true, ConfigBool("Gui", "CPUDumpStartFromSelect"));
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    bool startFromSelection = hexEdit.startFromSelection();
    Config()->setBool("Gui", "CPUDumpStartFromSelect", startFromSelection);
    dsint addr = rvaToVa(getSelectionStart());
    if(!startFromSelection)
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = ToPtrString(addr);
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern() + ", &data&"));
    emit displayReferencesWidget();
}

void CPUDump::copyFileOffsetSlot()
{
    duint addr = rvaToVa(getInitialSelection());
    duint offset = DbgFunctions()->VaToFileOffset(addr);
    if(offset)
    {
        QString addrText = ToHexString(offset);
        Bridge::CopyToClipboard(addrText);
    }
    else
        QMessageBox::warning(this, tr("Error!"), tr("Selection not in a file..."));
}

void CPUDump::undoSelectionSlot()
{
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(!DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        return;
    DbgFunctions()->PatchRestoreRange(start, end);
    reloadData();
}

void CPUDump::selectionUpdatedSlot()
{
    QString selStart = ToPtrString(rvaToVa(getSelectionStart()));
    QString selEnd = ToPtrString(rvaToVa(getSelectionEnd()));
    QString info = tr("Dump");
    char mod[MAX_MODULE_SIZE] = "";
    if(DbgFunctions()->ModNameFromAddr(rvaToVa(getSelectionStart()), mod, true))
        info = QString(mod) + "";
    GuiAddStatusBarMessage(QString(info + ": " + selStart + " -> " + selEnd + QString().sprintf(" (0x%.8X bytes)\n", getSelectionEnd() - getSelectionStart() + 1)).toUtf8().constData());
}

void CPUDump::syncWithExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    GotoDialog gotoDialog(this, true);
    gotoDialog.setWindowTitle(tr("Enter expression to sync with..."));
    gotoDialog.setInitialExpression(mSyncAddrExpression);
    if(gotoDialog.exec() != QDialog::Accepted)
        return;
    mSyncAddrExpression = gotoDialog.expressionText;
    updateDumpSlot();
}

void CPUDump::allocMemorySlot()
{
    WordEditDialog mLineEdit(this);
    mLineEdit.setup(tr("Size"), 0x1000, sizeof(duint));
    if(mLineEdit.exec() == QDialog::Accepted)
    {
        duint memsize = mLineEdit.getVal();
        if(memsize == 0) // 1GB
        {
            SimpleWarningBox(this, tr("Warning"), tr("You're trying to allocate a zero-sized buffer just now."));
            return;
        }
        if(memsize > 1024 * 1024 * 1024)
        {
            SimpleErrorBox(this, tr("Error"), tr("The size of buffer you're trying to allocate exceeds 1GB. Please check your expression to ensure nothing is wrong."));
            return;
        }
        DbgCmdExecDirect(QString("alloc %1").arg(ToPtrString(memsize)));
        duint addr = DbgValFromString("$result");
        if(addr != 0)
        {
            DbgCmdExec("Dump $result");
        }
        else
        {
            SimpleErrorBox(this, tr("Error"), tr("Memory allocation failed!"));
            return;
        }
    }
}

void CPUDump::visitSlot()
{
    // Copy pasta from setupFollowMenu for now
    auto vaSelected = rvaToVa(getInitialSelection());

    QStringList structs;
    DbgFunctions()->EnumStructs([](const char* name, void* userdata)
    {
        ((QStringList*)userdata)->append(name);
    }, &structs);

    if(structs.isEmpty())
    {
        SimpleErrorBox(this, tr("Error"), tr("No types loaded yet, parse a header first..."));
        return;
    }

    QString selection;
    if(!SimpleChoiceBox(this, tr("Type to display"), "", structs, selection, true, "", DIcon("struct"), 1) || selection.isEmpty())
        return;
    if(!mGotoType)
        mGotoType = new GotoDialog(this);
    mGotoType->setWindowTitle(tr("Address to display %1 at").arg(selection));

    DbgCmdExec(QString("VisitType %1, %2, 2").arg(selection, ToPtrString(vaSelected)));
}

void CPUDump::setView(ViewEnum_t view)
{
    switch(view)
    {
    case ViewHexAscii:
        hexAsciiSlot();
        break;
    case ViewHexUnicode:
        hexUnicodeSlot();
        break;
    case ViewTextAscii:
        textAsciiSlot();
        break;
    case ViewTextUnicode:
        textUnicodeSlot();
        break;
    case ViewIntegerSignedByte:
        integerSignedByteSlot();
        break;
    case ViewIntegerSignedShort:
        integerSignedShortSlot();
        break;
    case ViewIntegerSignedLong:
        integerSignedLongSlot();
        break;
    case ViewIntegerSignedLongLong:
        integerSignedLongLongSlot();
        break;
    case ViewIntegerUnsignedByte:
        integerUnsignedByteSlot();
        break;
    case ViewIntegerUnsignedShort:
        integerUnsignedShortSlot();
        break;
    case ViewIntegerUnsignedLong:
        integerUnsignedLongSlot();
        break;
    case ViewIntegerUnsignedLongLong:
        integerUnsignedLongLongSlot();
        break;
    case ViewIntegerHexShort:
        integerHexShortSlot();
        break;
    case ViewIntegerHexLong:
        integerHexLongSlot();
        break;
    case ViewIntegerHexLongLong:
        integerHexLongLongSlot();
        break;
    case ViewFloatFloat:
        floatFloatSlot();
        break;
    case ViewFloatDouble:
        floatDoubleSlot();
        break;
    case ViewFloatLongDouble:
        floatLongDoubleSlot();
        break;
    case ViewAddress:
    case ViewAddressAscii:
        addressAsciiSlot();
        break;
    case ViewAddressUnicode:
        addressUnicodeSlot();
        break;
    case ViewHexCodepage:
        hexLastCodepageSlot();
        break;
    case ViewTextCodepage:
        textLastCodepageSlot();
        break;
    default:
        hexAsciiSlot();
        break;
    }
}

void CPUDump::headerButtonReleasedSlot(duint colIndex)
{
    auto callback = mDescriptor[colIndex].columnSwitch;
    if(callback)
        callback();
}
