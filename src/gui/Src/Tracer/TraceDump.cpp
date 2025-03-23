#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include <QToolTip>
#include "TraceDump.h"
#include "TraceWidget.h"
#include "Configuration.h"
#include "Bridge.h"
#include "HexEditDialog.h"
#include "GotoDialog.h"
#include "TraceBrowser.h"
#include "CommonActions.h"
#include "CodepageSelectionDialog.h"
#include "MiscUtil.h"
//#include "BackgroundFlickerThread.h"
#include "TraceFileSearch.h"

TraceDump::TraceDump(Architecture* architecture, TraceWidget* parent, TraceFileDumpMemoryPage* memoryPage) : mMemoryPage(memoryPage), HexDump(architecture, parent, memoryPage)
{
    mParent = parent;
    setDrawDebugOnly(false);
    //mMultiDump = multiDump;

    duint setting;
    if(BridgeSettingGetUint("Gui", "AsciiSeparator", &setting))
        mAsciiSeparator = setting & 0xF;

    setView((ViewEnum_t)ConfigUint("HexDump", "DefaultView"));

    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));
    connect(this, SIGNAL(headerButtonReleased(duint)), this, SLOT(headerButtonReleasedSlot(duint)));

    //mPluginMenu = multiDump->mDumpPluginMenu;

    setupContextMenu();
}

void TraceDump::setupContextMenu()
{
    mMenuBuilder = new MenuBuilder(this, [this](QMenu*)
    {
        return mMemoryPage->isAvailable();
    });

    mCommonActions = new CommonActions(this, getActionHelperFuncs(), [this]()
    {
        return rvaToVa(getSelectionStart());
    });

    MenuBuilder* wBinaryMenu = new MenuBuilder(this);
    wBinaryMenu->addAction(makeShortcutAction(DIcon("binary_copy"), tr("&Copy"), SLOT(binaryCopySlot()), "ActionBinaryCopy"));
    wBinaryMenu->addAction(makeShortcutAction(DIcon("binary_save"), tr("Save To a File"), SLOT(binarySaveToFileSlot()), "ActionBinarySave"));
    mMenuBuilder->addMenu(makeMenu(DIcon("binary"), tr("B&inary")), wBinaryMenu);

    MenuBuilder* wCopyMenu = new MenuBuilder(this);
    wCopyMenu->addAction(mCopySelection);
    wCopyMenu->addAction(mCopyAddress);
    wCopyMenu->addAction(mCopyRva, [this](QMenu*)
    {
        return DbgFunctions()->ModBaseFromAddr(rvaToVa(getInitialSelection())) != 0;
    });
    wCopyMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("&File Offset"), SLOT(copyFileOffsetSlot()), "ActionCopyFileOffset"), [this](QMenu*)
    {
        return DbgFunctions()->VaToFileOffset(rvaToVa(getInitialSelection())) != 0;
    });

    mMenuBuilder->addMenu(makeMenu(DIcon("copy"), tr("&Copy")), wCopyMenu);

    /*mMenuBuilder->addAction(makeShortcutAction(DIcon("eraser"), tr("&Restore selection"), SLOT(undoSelectionSlot()), "ActionUndoSelection"), [this](QMenu*)
    {
        return DbgFunctions()->PatchInRange(rvaToVa(getSelectionStart()), rvaToVa(getSelectionEnd()));
    });*/

    mCommonActions->build(mMenuBuilder, CommonActions::ActionDisasm | CommonActions::ActionMemoryMap | CommonActions::ActionDumpData | CommonActions::ActionDumpN
                          | CommonActions::ActionDisasmData | CommonActions::ActionStackDump | CommonActions::ActionLabel);
    auto wIsValidReadPtrCallback = [this](QMenu*)
    {
        duint ptr = 0;
        DbgMemRead(rvaToVa(getSelectionStart()), (unsigned char*)&ptr, sizeof(duint));
        return DbgMemIsValidReadPtr(ptr);
    };

    //TODO: Is it necessary to set memory breakpoints here?

    //TODO: find in dump
    mMenuBuilder->addAction(makeShortcutAction(DIcon("search-for"), tr("&Find Pattern..."), SLOT(findPattern()), "ActionFindPattern"));
    //mMenuBuilder->addAction(makeShortcutAction(DIcon("find"), tr("Find &References"), SLOT(findReferencesSlot()), "ActionFindReferences"));

    //TODO: Do we really need to sync with expression here?
    //mMenuBuilder->addAction(makeShortcutAction(DIcon("sync"), tr("&Sync with expression"), SLOT(syncWithExpressionSlot()), "ActionSync"));

    MenuBuilder* wGotoMenu = new MenuBuilder(this);
    wGotoMenu->addAction(makeShortcutAction(DIcon("geolocation-goto"), tr("&Expression"), SLOT(gotoExpressionSlot()), "ActionGotoExpression"));
    //wGotoMenu->addAction(makeShortcutAction(DIcon("fileoffset"), tr("File Offset"), SLOT(gotoFileOffsetSlot()), "ActionGotoFileOffset"));
    //wGotoMenu->addAction(makeShortcutAction(DIcon("top"), tr("Start of Page"), SLOT(gotoStartSlot()), "ActionGotoStart"), [this](QMenu*)
    //{
    //    return getSelectionStart() != 0;
    //});
    //wGotoMenu->addAction(makeShortcutAction(DIcon("bottom"), tr("End of Page"), SLOT(gotoEndSlot()), "ActionGotoEnd"));
    wGotoMenu->addAction(makeShortcutAction(DIcon("previous"), tr("Previous"), SLOT(gotoPreviousSlot()), "ActionGotoPrevious"), [this](QMenu*)
    {
        return mHistory.historyHasPrev();
    });
    wGotoMenu->addAction(makeShortcutAction(DIcon("next"), tr("Next"), SLOT(gotoNextSlot()), "ActionGotoNext"), [this](QMenu*)
    {
        return mHistory.historyHasNext();
    });
    mMenuBuilder->addMenu(makeMenu(DIcon("goto"), tr("&Go to")), wGotoMenu);
    mMenuBuilder->addAction(makeShortcutAction(DIcon("xrefs"), tr("xrefs..."), SLOT(gotoXrefSlot()), "ActionXrefs"));
    mMenuBuilder->addSeparator();

    MenuBuilder* wHexMenu = new MenuBuilder(this);
    wHexMenu->addAction(makeAction(DIcon("ascii"), tr("&ASCII"), SLOT(hexAsciiSlot())));
    wHexMenu->addAction(makeAction(DIcon("ascii-extended"), tr("&Extended ASCII"), SLOT(hexUnicodeSlot())));
    QAction* wHexLastCodepage = makeAction(DIcon("codepage"), "?", SLOT(hexLastCodepageSlot()));
    wHexMenu->addAction(wHexLastCodepage, [wHexLastCodepage](QMenu*)
    {
        duint lastCodepage;
        auto allCodecs = QTextCodec::availableCodecs();
        if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
            return false;
        wHexLastCodepage->setText(QString::fromLocal8Bit(allCodecs.at(lastCodepage)));
        return true;
    });
    wHexMenu->addAction(makeAction(DIcon("codepage"), tr("&Codepage..."), SLOT(hexCodepageSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("hex"), tr("&Hex")), wHexMenu);

    MenuBuilder* wTextMenu = new MenuBuilder(this);
    wTextMenu->addAction(makeAction(DIcon("ascii"), tr("&ASCII"), SLOT(textAsciiSlot())));
    wTextMenu->addAction(makeAction(DIcon("ascii-extended"), tr("&Extended ASCII"), SLOT(textUnicodeSlot())));
    QAction* wTextLastCodepage = makeAction(DIcon("codepage"), "?", SLOT(textLastCodepageSlot()));
    wTextMenu->addAction(wTextLastCodepage, [wTextLastCodepage](QMenu*)
    {
        duint lastCodepage;
        auto allCodecs = QTextCodec::availableCodecs();
        if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
            return false;
        wTextLastCodepage->setText(QString::fromLocal8Bit(allCodecs.at(lastCodepage)));
        return true;
    });
    wTextMenu->addAction(makeAction(DIcon("codepage"), tr("&Codepage..."), SLOT(textCodepageSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("strings"), tr("&Text")), wTextMenu);

    MenuBuilder* wIntegerMenu = new MenuBuilder(this);
    wIntegerMenu->addAction(makeAction(DIcon("byte"), tr("Signed byte (8-bit)"), SLOT(integerSignedByteSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("word"), tr("Signed short (16-bit)"), SLOT(integerSignedShortSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("dword"), tr("Signed long (32-bit)"), SLOT(integerSignedLongSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("qword"), tr("Signed long long (64-bit)"), SLOT(integerSignedLongLongSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("byte"), tr("Unsigned byte (8-bit)"), SLOT(integerUnsignedByteSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("word"), tr("Unsigned short (16-bit)"), SLOT(integerUnsignedShortSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("dword"), tr("Unsigned long (32-bit)"), SLOT(integerUnsignedLongSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("qword"), tr("Unsigned long long (64-bit)"), SLOT(integerUnsignedLongLongSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("word"), tr("Hex short (16-bit)"), SLOT(integerHexShortSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("dword"), tr("Hex long (32-bit)"), SLOT(integerHexLongSlot())));
    wIntegerMenu->addAction(makeAction(DIcon("qword"), tr("Hex long long (64-bit)"), SLOT(integerHexLongLongSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("integer"), tr("&Integer")), wIntegerMenu);

    MenuBuilder* wFloatMenu = new MenuBuilder(this);
    wFloatMenu->addAction(makeAction(DIcon("32bit-float"), tr("&Float (32-bit)"), SLOT(floatFloatSlot())));
    wFloatMenu->addAction(makeAction(DIcon("64bit-float"), tr("&Double (64-bit)"), SLOT(floatDoubleSlot())));
    wFloatMenu->addAction(makeAction(DIcon("80bit-float"), tr("&Long double (80-bit)"), SLOT(floatLongDoubleSlot())));
    wFloatMenu->addAction(makeAction(DIcon("word"), tr("&Half float (16-bit)"), SLOT(floatHalfSlot())));
    mMenuBuilder->addMenu(makeMenu(DIcon("float"), tr("&Float")), wFloatMenu);

    mMenuBuilder->addAction(makeAction(DIcon("address"), tr("&Address"), SLOT(addressAsciiSlot())));
    mMenuBuilder->addAction(makeAction(DIcon("processor-cpu"), tr("&Disassembly"), SLOT(disassemblySlot())))->setEnabled(false); //TODO

    //mMenuBuilder->addSeparator();
    //mMenuBuilder->addBuilder(new MenuBuilder(this, [this](QMenu * menu)
    //{
    //    DbgMenuPrepare(GUI_DUMP_MENU);
    //    menu->addActions(mPluginMenu->actions());
    //    return true;
    //}));

    mMenuBuilder->loadFromConfig();
    disconnect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChanged(DBGSTATE)));
    updateShortcuts();
}

TraceDump::~TraceDump()
{
    mMemPage = nullptr; // Let the stack view delete it instead! If both views are trying to delete, it will crash.
}

void TraceDump::mousePressEvent(QMouseEvent* event)
{
    if(event->buttons() == Qt::MiddleButton) //copy address to clipboard
    {
        //Allow copying while not debugging
        MessageBeep(MB_OK);
        QString addrText = ToPtrString(rvaToVa(getInitialSelection()));
        Bridge::CopyToClipboard(addrText);
        return;
    }
    HexDump::mousePressEvent(event);
}

//void TraceDump::getAttention()
//{
//    BackgroundFlickerThread* thread = new BackgroundFlickerThread(this, mBackgroundColor, this);
//    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
//    thread->start();
//}

void TraceDump::printDumpAt(duint parVA, bool select, bool repaint, bool updateTableOffset)
{
    // TODO: use the latest version from the hexdump, or refactor into a helper function

    // Modified from Hexdump, removed memory page information
    // TODO: get memory range from trace instead
    const duint wSize = 0x1000;  // TODO: Using 4KB pages currently
    auto wBase = mMemoryPage->getBase();
    dsint wRVA = parVA - wBase; //calculate rva
    if(wRVA < 0 || wRVA >= wSize)
    {
        wBase = parVA & ~(wSize - 1);
        mMemoryPage->setAttributes(wBase, wSize);
        wRVA = parVA - wBase; //calculate rva
    }
    auto wBytePerRowCount = getBytePerRowCount(); //get the number of bytes per row
    dsint wRowCount;

    // Byte offset used to be aligned on the given RVA
    mByteOffset = (int)((dsint)wRVA % (dsint)wBytePerRowCount);
    mByteOffset = mByteOffset > 0 ? wBytePerRowCount - mByteOffset : 0;

    // Compute row count
    wRowCount = wSize / wBytePerRowCount;
    wRowCount += mByteOffset > 0 ? 1 : 0;

    //if(mRvaDisplayEnabled && mMemPage->getBase() != mRvaDisplayPageBase)
    //    mRvaDisplayEnabled = false;

    setRowCount(wRowCount); //set the number of rows

    if(updateTableOffset)
    {
        setTableOffset(-1); //make sure the requested address is always first
        setTableOffset((wRVA + mByteOffset) / wBytePerRowCount); //change the displayed offset
    }

    if(select)
    {
        setSingleSelection(wRVA);
        dsint wEndingAddress = wRVA + getSizeOf(mDescriptor.at(0).data.itemSize) - 1;
        expandSelectionUpTo(wEndingAddress);
    }

    if(repaint)
        reloadData();
}

void TraceDump::getColumnRichText(duint col, duint rva, RichTextPainter::List & richText)
{
    if(col && !mDescriptor.at(col - 1).isData && mDescriptor.at(col - 1).itemCount) //print comments
    {
        RichTextPainter::CustomRichText_t curData;
        curData.flags = RichTextPainter::FlagColor;
        curData.textColor = mTextColor;
        duint data = 0;
        mMemPage->read((byte_t*)&data, rva, sizeof(duint));

        char modname[MAX_MODULE_SIZE] = "";
        //TODO
        //if(!DbgGetModuleAt(data, modname))
        //    modname[0] = '\0';
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
        }
        if(curData.text.length())
            richText.push_back(curData);
    }
    else
        HexDump::getColumnRichText(col, rva, richText);
}

QString TraceDump::paintContent(QPainter* painter, duint row, duint col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    //if(rowBase == 0 && mByteOffset != 0)
    //    HexDump::printDumpAt(mMemPage->getBase(), false, false);

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

void TraceDump::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu wMenu(this);
    mMenuBuilder->build(&wMenu);
    wMenu.exec(event->globalPos());
}

void TraceDump::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton || !mMemoryPage->isAvailable())
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
    }
}

void TraceDump::mouseMoveEvent(QMouseEvent* event)
{
    // Get mouse pointer relative position
    int x = event->x();
    int y = event->y();

    // Get HexDump own RVA address, then VA in memory
    auto va = rvaToVa(getItemStartingAddress(x, y));

    // Read VA
    Q_UNUSED(va);
    //QToolTip::showText(event->globalPos(), getTooltipForVa(va, 4)); //TODO: Unsupported

    HexDump::mouseMoveEvent(event);
}

void TraceDump::gotoExpressionSlot()
{
    if(!mMemoryPage->isAvailable())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this, false, true, true);
    mGoto->setWindowTitle(tr("Enter expression to follow in Dump..."));
    mGoto->setInitialExpression(ToPtrString(rvaToVa(getInitialSelection())));
    if(mGoto->exec() == QDialog::Accepted)
    {
        duint value = DbgValFromString(mGoto->expressionText.toUtf8().constData());
        GuiAddLogMessage(ToPtrString(value).toUtf8());
        this->printDumpAt(value, true, true, true);
    }
}

// TODO: Module information need to be read from trace file
//void TraceDump::gotoFileOffsetSlot()
//{
//...
//}

//void TraceDump::gotoStartSlot()
//{
//    duint dest = mMemPage->getBase();
//    this->printDumpAt(dest, true, true, true);
//}

//void TraceDump::gotoEndSlot()
//{
//    duint dest = mMemPage->getBase() + mMemPage->getSize() - (getViewableRowsCount() * getBytePerRowCount());
//    this->printDumpAt(dest, true, true, true);
//}

void TraceDump::gotoXrefSlot()
{
    emit xrefSignal(rvaToVa(getInitialSelection()));
}

void TraceDump::hexAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 16;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 16, tr("ASCII"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::hexUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 8, tr("UNICODE"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::hexCodepageSlot()
{
    CodepageSelectionDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    auto codepage = dialog.getSelectedCodepage();

    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

    wColDesc.isData = true; //text (in code page)
    wColDesc.itemCount = 16;
    wColDesc.separator = 0;
    wColDesc.textEncoding = codepage;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, codepage, false, wColDesc);

    reloadData();
}

void TraceDump::hexLastCodepageSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexCodepage);

    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        return;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = mAsciiSeparator ? mAsciiSeparator : 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, tr("Hex"), false, wColDesc);

    wColDesc.isData = true; //text (in code page)
    wColDesc.itemCount = 16;
    wColDesc.separator = 0;
    wColDesc.textEncoding = allCodecs.at(lastCodepage);
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, allCodecs.at(lastCodepage), false, wColDesc);

    reloadData();
}

void TraceDump::textLastCodepageSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextCodepage);

    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;
    duint lastCodepage;
    auto allCodecs = QTextCodec::availableCodecs();
    if(!BridgeSettingGetUint("Misc", "LastCodepage", &lastCodepage) || lastCodepage >= duint(allCodecs.size()))
        return;

    wColDesc.isData = true; //text (in code page)
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    wColDesc.textEncoding = allCodecs.at(lastCodepage);
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(0, allCodecs.at(lastCodepage), false, wColDesc);

    reloadData();
}

void TraceDump::textAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, tr("ASCII"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::textUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, tr("UNICODE"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::textCodepageSlot()
{
    CodepageSelectionDialog dialog(this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    auto codepage = dialog.getSelectedCodepage();

    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //text (in code page)
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    wColDesc.textEncoding = codepage;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(0, codepage, false, wColDesc);

    reloadData();
}

void TraceDump::integerSignedByteSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedByte);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //signed short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Byte;
    wColDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8 + charwidth * 40, tr("Signed byte (8-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerSignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //signed short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8 + charwidth * 55, tr("Signed short (16-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerSignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //signed long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = SignedDecDword;
    appendResetDescriptor(8 + charwidth * 47, tr("Signed long (32-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerSignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //signed long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = SignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, tr("Signed long long (64-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerUnsignedByteSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedByte);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //unsigned short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Byte;
    wColDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8 + charwidth * 32, tr("Unsigned byte (8-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerUnsignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //unsigned short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8 + charwidth * 47, tr("Unsigned short (16-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerUnsignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //unsigned long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = UnsignedDecDword;
    appendResetDescriptor(8 + charwidth * 43, tr("Unsigned long (32-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerUnsignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //unsigned long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = UnsignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, tr("Unsigned long long (64-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerHexShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexShort);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = HexWord;
    appendResetDescriptor(8 + charwidth * 39, tr("Hex short (16-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerHexLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
    appendResetDescriptor(8 + charwidth * 35, tr("Hex long (32-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::integerHexLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //hex long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
    appendResetDescriptor(8 + charwidth * 33, tr("Hex long long (64-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::floatFloatSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatFloat);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //float dword
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = FloatDword;
    appendResetDescriptor(8 + charwidth * 55, tr("Float (32-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::floatDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = DoubleQword;
    appendResetDescriptor(8 + charwidth * 47, tr("Double (64-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::floatLongDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatLongDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Tword;
    wColDesc.data.twordMode = FloatTword;
    appendResetDescriptor(8 + charwidth * 59, tr("Long double (80-bit)"), false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void TraceDump::floatHalfSlot()
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

void TraceDump::addressAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewAddressAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendResetDescriptor(8 + charwidth * 2 * sizeof(duint), tr("Value"), false, wColDesc);

    wColDesc.isData = true;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.itemCount = 8;
#else
    wColDesc.itemCount = 4;
#endif
    wColDesc.data.itemSize = Byte;
    wColDesc.data.byteMode = AsciiByte;
    wColDesc.columnSwitch = [this]()
    {
        this->setView(ViewAddressUnicode);
    };
    appendDescriptor(8 + charwidth * wColDesc.itemCount, tr("ASCII"), true, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, tr("Comments"), false, wColDesc);

    reloadData();
}

void TraceDump::addressUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewAddressUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor wColDesc;
    DataDescriptor dDesc;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendResetDescriptor(8 + charwidth * 2 * sizeof(duint), tr("Value"), false, wColDesc);

    wColDesc.isData = true;
    wColDesc.separator = 0;
#ifdef _WIN64
    wColDesc.itemCount = 4;
#else
    wColDesc.itemCount = 2;
#endif
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = UnicodeWord;
    wColDesc.columnSwitch = [this]()
    {
        this->setView(ViewAddressAscii);
    };
    appendDescriptor(8 + charwidth * wColDesc.itemCount, tr("UNICODE"), true, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, tr("Comments"), false, wColDesc);

    reloadData();
}

void TraceDump::disassemblySlot()
{
    //SELECTIONDATA selection;
    //selectionGet(&selection);
    //emit showDisassemblyTab(selection.start, selection.end, rvaToVa(getTableOffsetRva()));
}

void TraceDump::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(BridgeResult::SelectionGet, 1);
}

void TraceDump::selectionSet(const SELECTIONDATA* selection)
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

//TODO
//void TraceDump::findReferencesSlot()
//{
//QString addrStart = ToPtrString(rvaToVa(getSelectionStart()));
//QString addrEnd = ToPtrString(rvaToVa(getSelectionEnd()));
//QString addrDisasm = ToPtrString(mDisas->rvaToVa(mDisas->getSelectionStart()));
//DbgCmdExec(QString("findrefrange " + addrStart + ", " + addrEnd + ", " + addrDisasm));
//emit displayReferencesWidget();
//}

void TraceDump::binaryCopySlot()
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

void TraceDump::binarySaveToFileSlot()
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

void TraceDump::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.isDataCopiable(false);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle(tr("Find Pattern..."));
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    if(mParent->loadDumpFully())
    {
        QTime ticks;
        ticks.start();
        auto count = TraceFileSearchMemPattern(mParent->getTraceFile(), hexEdit.mHexEdit->pattern());
        GuiShowReferences();
        GuiAddLogMessage(QCoreApplication::translate("DBG", "%1 occurrence(s) in %2ms\n").arg(count).arg(ticks.elapsed()).toUtf8().constData());
    }
}

void TraceDump::copyFileOffsetSlot()
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

void TraceDump::selectionUpdatedSlot()
{
    QString selStart = ToPtrString(rvaToVa(getSelectionStart()));
    QString selEnd = ToPtrString(rvaToVa(getSelectionEnd()));
    QString info = tr("Dump");
    char mod[MAX_MODULE_SIZE] = "";
    Q_UNUSED(mod);
    //TODO
    //if(DbgFunctions()->ModNameFromAddr(rvaToVa(getSelectionStart()), mod, true))
    //    info = QString(mod) + "";
    GuiAddStatusBarMessage(QString(info + ": " + selStart + " -> " + selEnd + QString().sprintf(" (0x%.8X bytes)\n", getSelectionEnd() - getSelectionStart() + 1)).toUtf8().constData());
}

//void TraceDump::syncWithExpressionSlot()
//{
//if(!mMemoryPage->isAvailable())
//return;
//GotoDialog gotoDialog(this, true);
//gotoDialog.setWindowTitle(tr("Enter expression to sync with..."));
//gotoDialog.setInitialExpression(mSyncAddrExpression);
//if(gotoDialog.exec() != QDialog::Accepted)
//return;
//mSyncAddrExpression = gotoDialog.expressionText;
//updateDumpSlot();
//}

void TraceDump::setView(ViewEnum_t view)
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

void TraceDump::headerButtonReleasedSlot(duint colIndex)
{
    auto callback = mDescriptor[colIndex].columnSwitch;
    if(callback)
        callback();
}
