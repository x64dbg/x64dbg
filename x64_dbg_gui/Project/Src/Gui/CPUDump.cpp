#include "CPUDump.h"
#include "Configuration.h"

CPUDump::CPUDump(QWidget *parent) : HexDump(parent)
{
    hexAsciiSlot();

    connect(Bridge::getBridge(), SIGNAL(dumpAt(int_t)), this, SLOT(printDumpAt(int_t)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpGet(SELECTIONDATA*)), this, SLOT(selectionGet(SELECTIONDATA*)));
    connect(Bridge::getBridge(), SIGNAL(selectionDumpSet(const SELECTIONDATA*)), this, SLOT(selectionSet(const SELECTIONDATA*)));

    setupContextMenu();

    mGoto = 0;
}

void CPUDump::setupContextMenu()
{
    //Label
    mSetLabelAction = new QAction("Set Label", this);
    mSetLabelAction->setShortcutContext(Qt::WidgetShortcut);
    mSetLabelAction->setShortcut(QKeySequence(":"));
    this->addAction(mSetLabelAction);
    connect(mSetLabelAction, SIGNAL(triggered()), this, SLOT(setLabelSlot()));

    //Breakpoint menu
    mBreakpointMenu = new QMenu("&Breakpoint", this);

    //Breakpoint->Hardware, on access
    mHardwareAccessMenu = new QMenu("Hardware, &Access", this);
    mHardwareAccess1 = new QAction("&Byte", this);
    connect(mHardwareAccess1, SIGNAL(triggered()), this, SLOT(hardwareAccess1Slot()));
    mHardwareAccessMenu->addAction(mHardwareAccess1);
    mHardwareAccess2 = new QAction("&Word", this);
    connect(mHardwareAccess2, SIGNAL(triggered()), this, SLOT(hardwareAccess2Slot()));
    mHardwareAccessMenu->addAction(mHardwareAccess2);
    mHardwareAccess4 = new QAction("&Dword", this);
    connect(mHardwareAccess4, SIGNAL(triggered()), this, SLOT(hardwareAccess4Slot()));
    mHardwareAccessMenu->addAction(mHardwareAccess4);
#ifdef _WIN64
    mHardwareAccess8 = new QAction("&Qword", this);
    connect(mHardwareAccess8, SIGNAL(triggered()), this, SLOT(hardwareAccess8Slot()));
    mHardwareAccessMenu->addAction(mHardwareAccess8);
#endif //_WIN64
    mBreakpointMenu->addMenu(mHardwareAccessMenu);

    //Breakpoint->Hardware, on write
    mHardwareWriteMenu = new QMenu("Hardware, &Write", this);
    mHardwareWrite1 = new QAction("&Byte", this);
    connect(mHardwareWrite1, SIGNAL(triggered()), this, SLOT(hardwareWrite1Slot()));
    mHardwareWriteMenu->addAction(mHardwareWrite1);
    mHardwareWrite2 = new QAction("&Word", this);
    connect(mHardwareWrite2, SIGNAL(triggered()), this, SLOT(hardwareWrite2Slot()));
    mHardwareWriteMenu->addAction(mHardwareWrite2);
    mHardwareWrite4 = new QAction("&Dword", this);
    connect(mHardwareWrite4, SIGNAL(triggered()), this, SLOT(hardwareWrite4Slot()));
    mHardwareWriteMenu->addAction(mHardwareWrite4);
#ifdef _WIN64
    mHardwareWrite8 = new QAction("&Qword", this);
    connect(mHardwareWrite8, SIGNAL(triggered()), this, SLOT(hardwareWrite8Slot()));
    mHardwareWriteMenu->addAction(mHardwareWrite8);
#endif //_WIN64
    mBreakpointMenu->addMenu(mHardwareWriteMenu);

    mHardwareExecute = new QAction("Hardware, &Execute", this);
    connect(mHardwareExecute, SIGNAL(triggered()), this, SLOT(hardwareExecuteSlot()));
    mBreakpointMenu->addAction(mHardwareExecute);

    mHardwareRemove = new QAction("Remove &Hardware", this);
    connect(mHardwareRemove, SIGNAL(triggered()), this, SLOT(hardwareRemoveSlot()));
    mBreakpointMenu->addAction(mHardwareRemove);

    //Breakpoint Separator
    mBreakpointMenu->addSeparator();

    //Breakpoint->Memory Access
    mMemoryAccessMenu = new QMenu("Memory, Access", this);
    mMemoryAccessSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryAccessSingleshoot, SIGNAL(triggered()), this, SLOT(memoryAccessSingleshootSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessSingleshoot);
    mMemoryAccessRestore = new QAction("&Restore", this);
    connect(mMemoryAccessRestore, SIGNAL(triggered()), this, SLOT(memoryAccessRestoreSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu("Memory, Write", this);
    mMemoryWriteSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryWriteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryWriteSingleshootSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction("&Restore", this);
    connect(mMemoryWriteRestore, SIGNAL(triggered()), this, SLOT(memoryWriteRestoreSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu("Memory, Execute", this);
    mMemoryExecuteSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryExecuteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction("&Restore", this);
    connect(mMemoryExecuteRestore, SIGNAL(triggered()), this, SLOT(memoryExecuteRestoreSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteRestore);
    mBreakpointMenu->addMenu(mMemoryExecuteMenu);

    //Breakpoint->Remove Memory
    mMemoryRemove = new QAction("Remove &Memory", this);
    connect(mMemoryRemove, SIGNAL(triggered()), this, SLOT(memoryRemoveSlot()));
    mBreakpointMenu->addAction(mMemoryRemove);

    //Goto menu
    mGotoMenu = new QMenu("&Goto", this);
    //Goto->Expression
    mGotoExpression = new QAction("&Expression", this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    mGotoExpression->setShortcut(QKeySequence("ctrl+g"));
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));
    mGotoMenu->addAction(mGotoExpression);

    //Hex menu
    mHexMenu = new QMenu("&Hex", this);
    //Hex->Ascii
    mHexAsciiAction = new QAction("&Ascii", this);
    connect(mHexAsciiAction, SIGNAL(triggered()), this, SLOT(hexAsciiSlot()));
    mHexMenu->addAction(mHexAsciiAction);
    //Hex->Unicode
    mHexUnicodeAction = new QAction("&Unicode", this);
    connect(mHexUnicodeAction, SIGNAL(triggered()), this, SLOT(hexUnicodeSlot()));
    mHexMenu->addAction(mHexUnicodeAction);

    //Text menu
    mTextMenu = new QMenu("&Text", this);
    //Text->Ascii
    mTextAsciiAction = new QAction("&Ascii", this);
    connect(mTextAsciiAction, SIGNAL(triggered()), this, SLOT(textAsciiSlot()));
    mTextMenu->addAction(mTextAsciiAction);
    //Text->Unicode
    mTextUnicodeAction = new QAction("&Unicode", this);
    connect(mTextUnicodeAction, SIGNAL(triggered()), this, SLOT(textUnicodeSlot()));
    mTextMenu->addAction(mTextUnicodeAction);

    //Integer menu
    mIntegerMenu = new QMenu("&Integer", this);
    //Integer->Signed short
    mIntegerSignedShortAction = new QAction("Signed short (16-bit)", this);
    connect(mIntegerSignedShortAction, SIGNAL(triggered()), this, SLOT(integerSignedShortSlot()));
    mIntegerMenu->addAction(mIntegerSignedShortAction);
    //Integer->Signed long
    mIntegerSignedLongAction = new QAction("Signed long (32-bit)", this);
    connect(mIntegerSignedLongAction, SIGNAL(triggered()), this, SLOT(integerSignedLongSlot()));
    mIntegerMenu->addAction(mIntegerSignedLongAction);
#ifdef _WIN64
    //Integer->Signed long long
    mIntegerSignedLongLongAction = new QAction("Signed long long (64-bit)", this);
    connect(mIntegerSignedLongLongAction, SIGNAL(triggered()), this, SLOT(integerSignedLongLongSlot()));
    mIntegerMenu->addAction(mIntegerSignedLongLongAction);
#endif //_WIN64
    //Integer->Unsigned short
    mIntegerUnsignedShortAction = new QAction("Unsigned short (16-bit)", this);
    connect(mIntegerUnsignedShortAction, SIGNAL(triggered()), this, SLOT(integerUnsignedShortSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedShortAction);
    //Integer->Unsigned long
    mIntegerUnsignedLongAction = new QAction("Unsigned long (32-bit)", this);
    connect(mIntegerUnsignedLongAction, SIGNAL(triggered()), this, SLOT(integerUnsignedLongSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedLongAction);
#ifdef _WIN64
    //Integer->Unsigned long long
    mIntegerUnsignedLongLongAction = new QAction("Unsigned long long (64-bit)", this);
    connect(mIntegerUnsignedLongLongAction, SIGNAL(triggered()), this, SLOT(integerUnsignedLongLongSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedLongLongAction);
#endif //_WIN64
    //Integer->Hex short
    mIntegerHexShortAction = new QAction("Hex short (16-bit)", this);
    connect(mIntegerHexShortAction, SIGNAL(triggered()), this, SLOT(integerHexShortSlot()));
    mIntegerMenu->addAction(mIntegerHexShortAction);
    //Integer->Hex long
    mIntegerHexLongAction = new QAction("Hex long (32-bit)", this);
    connect(mIntegerHexLongAction, SIGNAL(triggered()), this, SLOT(integerHexLongSlot()));
    mIntegerMenu->addAction(mIntegerHexLongAction);
#ifdef _WIN64
    //Integer->Hex long long
    mIntegerHexLongLongAction = new QAction("Hex long long (64-bit)", this);
    connect(mIntegerHexLongLongAction, SIGNAL(triggered()), this, SLOT(integerHexLongLongSlot()));
    mIntegerMenu->addAction(mIntegerHexLongLongAction);
#endif //_WIN64

    //Float menu
    mFloatMenu = new QMenu("&Float", this);
    //Float->float
    mFloatFloatAction = new QAction("&Float (32-bit)", this);
    connect(mFloatFloatAction, SIGNAL(triggered()), this, SLOT(floatFloatSlot()));
    mFloatMenu->addAction(mFloatFloatAction);
    //Float->double
    mFloatDoubleAction = new QAction("&Double (64-bit)", this);
    connect(mFloatDoubleAction, SIGNAL(triggered()), this, SLOT(floatDoubleSlot()));
    mFloatMenu->addAction(mFloatDoubleAction);
    //Float->long double
    mFloatLongDoubleAction = new QAction("&Long double (80-bit)", this);
    connect(mFloatLongDoubleAction, SIGNAL(triggered()), this, SLOT(floatLongDoubleSlot()));
    mFloatMenu->addAction(mFloatLongDoubleAction);

    //Address
    mAddressAction = new QAction("&Address", this);
    connect(mAddressAction, SIGNAL(triggered()), this, SLOT(addressSlot()));

    //Disassembly
    mDisassemblyAction = new QAction("&Disassembly", this);
    connect(mDisassemblyAction, SIGNAL(triggered()), this, SLOT(disassemblySlot()));
}

QString CPUDump::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString wStr = "";
    if(!col) //address
    {
        char label[MAX_LABEL_SIZE]="";
        QString addrText="";
        int_t curAddr = rvaToVa((rowBase + rowOffset) * getBytePerRowCount() - mByteOffset);
        addrText = QString("%1").arg(curAddr, sizeof(int_t)*2, 16, QChar('0')).toUpper();
        if(DbgGetLabelAt(curAddr, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE]="";
            if(DbgGetModuleAt(curAddr, module) && !QString(label).startsWith("JMP.&"))
                addrText+=" <"+QString(module)+"."+QString(label)+">";
            else
                addrText+=" <"+QString(label)+">";
        }
        else
            *label=0;
        if(*label) //label
        {
            QColor background=ConfigColor("HexDumpLabelBackgroundColor");
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill bookmark color
            painter->setPen(ConfigColor("HexDumpLabelColor")); //TODO: config
        }
        else
        {
            QColor background=ConfigColor("HexDumpAddressBackgroundColor");
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill bookmark color
            painter->setPen(ConfigColor("HexDumpAddressColor")); //TODO: config
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    else if(col && mDescriptor.at(col - 1).isData == false && mDescriptor.at(col -1).itemCount == 1) //print comments
    {
        uint_t data=0;
        int_t wRva = (rowBase + rowOffset) * getBytePerRowCount() - mByteOffset;
        mMemPage->read((byte_t*)&data, wRva, sizeof(uint_t));
        char label_text[MAX_LABEL_SIZE]="";
        if(DbgGetLabelAt(data, SEG_DEFAULT, label_text))
            wStr=QString(label_text);
    }
    else //data
    {
        wStr = HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    }
    return wStr;
}

void CPUDump::contextMenuEvent(QContextMenuEvent* event)
{
    if(!DbgIsDebugging())
        return;
    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addAction(mSetLabelAction);
    wMenu->addMenu(mBreakpointMenu);
    wMenu->addMenu(mGotoMenu);
    wMenu->addSeparator();
    wMenu->addMenu(mHexMenu);
    wMenu->addMenu(mTextMenu);
    wMenu->addMenu(mIntegerMenu);
    wMenu->addMenu(mFloatMenu);
    wMenu->addAction(mAddressAction);
    wMenu->addAction(mDisassemblyAction);

    int_t selectedAddr = rvaToVa(getInitialSelection());
    if((DbgGetBpxTypeAt(selectedAddr)&bp_hardware)==bp_hardware) //hardware breakpoint set
    {
        mHardwareAccessMenu->menuAction()->setVisible(false);
        mHardwareWriteMenu->menuAction()->setVisible(false);
        mHardwareExecute->setVisible(false);
        mHardwareRemove->setVisible(true);
    }
    else //hardware breakpoint not set
    {
        mHardwareAccessMenu->menuAction()->setVisible(true);
        mHardwareWriteMenu->menuAction()->setVisible(true);
        mHardwareExecute->setVisible(true);
        mHardwareRemove->setVisible(false);
    }
    if((DbgGetBpxTypeAt(selectedAddr)&bp_memory)==bp_memory) //memory breakpoint set
    {
        mMemoryAccessMenu->menuAction()->setVisible(false);
        mMemoryWriteMenu->menuAction()->setVisible(false);
        mMemoryExecuteMenu->menuAction()->setVisible(false);
        mMemoryRemove->setVisible(true);
    }
    else //memory breakpoint not set
    {
        mMemoryAccessMenu->menuAction()->setVisible(true);
        mMemoryWriteMenu->menuAction()->setVisible(true);
        mMemoryExecuteMenu->menuAction()->setVisible(true);
        mMemoryRemove->setVisible(false);
    }

    wMenu->exec(event->globalPos()); //execute context menu
}

void CPUDump::setLabelSlot()
{
    if(!DbgIsDebugging())
        return;

    uint_t wVA = rvaToVa(getSelectionStart());
    LineEditDialog mLineEdit(this);
    QString addr_text=QString("%1").arg(wVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label_text[MAX_COMMENT_SIZE]="";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle("Add label at " + addr_text);
    if(mLineEdit.exec()!=QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetLabelAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
        msg.exec();
    }
    GuiUpdateAllViews();
}

void CPUDump::gotoExpressionSlot()
{
    if(!DbgIsDebugging())
        return;
    if(!mGoto)
        mGoto = new GotoDialog(this);
    mGoto->setWindowTitle("Enter expression to follow in Dump...");
    if(mGoto->exec()==QDialog::Accepted)
    {
        QString cmd;
        DbgCmdExec(cmd.sprintf("dump \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void CPUDump::hexAsciiSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*47, "Hex", false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 16;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*16, "ASCII", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::hexUnicodeSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*47, "Hex", false, wColDesc);

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 8;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*8, "UNICODE", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::textAsciiSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 64;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*64, "ASCII", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::textUnicodeSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 64;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*64, "UNICODE", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedShortSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed short
    wColDesc.itemCount = 8;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8+charwidth*55, "Signed short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed long
    wColDesc.itemCount = 4;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = SignedDecDword;
    appendResetDescriptor(8+charwidth*47, "Signed long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedLongLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed long long
    wColDesc.itemCount = 2;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = SignedDecQword;
    appendResetDescriptor(8+charwidth*41, "Signed long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedShortSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned short
    wColDesc.itemCount = 8;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8+charwidth*47, "Unsigned short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned long
    wColDesc.itemCount = 4;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = UnsignedDecDword;
    appendResetDescriptor(8+charwidth*43, "Unsigned long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned long long
    wColDesc.itemCount = 2;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = UnsignedDecQword;
    appendResetDescriptor(8+charwidth*41, "Unsigned long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexShortSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex short
    wColDesc.itemCount = 8;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = HexWord;
    appendResetDescriptor(8+charwidth*34, "Hex short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex long
    wColDesc.itemCount = 4;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
    appendResetDescriptor(8+charwidth*35, "Hex long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexLongLongSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex long long
    wColDesc.itemCount = 2;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
    appendResetDescriptor(8+charwidth*33, "Hex long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatFloatSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float dword
    wColDesc.itemCount = 4;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = FloatDword;
    appendResetDescriptor(8+charwidth*55, "Float (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatDoubleSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = DoubleQword;
    appendResetDescriptor(8+charwidth*47, "Double (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatLongDoubleSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.data.itemSize = Tword;
    wColDesc.data.twordMode = FloatTword;
    appendResetDescriptor(8+charwidth*59, "Long double (80-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::addressSlot()
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendResetDescriptor(8+charwidth*2*sizeof(uint_t), "Address", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 1;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "Comments", false, wColDesc);

    reloadData();
}

void CPUDump::disassemblySlot()
{
    QMessageBox msg(QMessageBox::Critical, "Error!", "Not yet supported!");
    msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags()&(~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void CPUDump::selectionGet(SELECTIONDATA* selection)
{
    selection->start=rvaToVa(getSelectionStart());
    selection->end=rvaToVa(getSelectionEnd());
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUDump::selectionSet(const SELECTIONDATA* selection)
{
    int_t selMin=mMemPage->getBase();
    int_t selMax=selMin + mMemPage->getSize();
    int_t start=selection->start;
    int_t end=selection->end;
    if(start < selMin || start >= selMax || end < selMin || end >= selMax) //selection out of range
    {
        Bridge::getBridge()->BridgeSetResult(0);
        return;
    }
    setSingleSelection(start - selMin);
    expandSelectionUpTo(end - selMin);
    reloadData();
    Bridge::getBridge()->BridgeSetResult(1);
}

void CPUDump::memoryAccessSingleshootSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 0, r").toUtf8().constData());
}

void CPUDump::memoryAccessRestoreSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 1, r").toUtf8().constData());
}

void CPUDump::memoryWriteSingleshootSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 0, w").toUtf8().constData());
}

void CPUDump::memoryWriteRestoreSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 1, w").toUtf8().constData());
}

void CPUDump::memoryExecuteSingleshootSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 0, x").toUtf8().constData());
}

void CPUDump::memoryExecuteRestoreSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm "+addr_text+", 1, x").toUtf8().constData());
}

void CPUDump::memoryRemoveSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpmc "+addr_text).toUtf8().constData());
}

void CPUDump::hardwareAccess1Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", r, 1").toUtf8().constData());
}

void CPUDump::hardwareAccess2Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", r, 2").toUtf8().constData());
}

void CPUDump::hardwareAccess4Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", r, 4").toUtf8().constData());
}

void CPUDump::hardwareAccess8Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", r, 8").toUtf8().constData());
}

void CPUDump::hardwareWrite1Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", w, 1").toUtf8().constData());
}

void CPUDump::hardwareWrite2Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", w, 2").toUtf8().constData());
}

void CPUDump::hardwareWrite4Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", w, 4").toUtf8().constData());
}

void CPUDump::hardwareWrite8Slot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", w, 8").toUtf8().constData());
}

void CPUDump::hardwareExecuteSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws "+addr_text+", x").toUtf8().constData());
}

void CPUDump::hardwareRemoveSlot()
{
    QString addr_text=QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphwc "+addr_text).toUtf8().constData());
}
