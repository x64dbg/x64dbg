#include "CPUDump.h"
#include <QMessageBox>
#include <QClipboard>
#include <QFileDialog>
#include "Configuration.h"
#include "Bridge.h"
#include "LineEditDialog.h"
#include "HexEditDialog.h"
#include "YaraRuleSelectionDialog.h"
#include "DataCopyDialog.h"
#include "EntropyDialog.h"
#include "CPUMultiDump.h"
#include <QToolTip>

CPUDump::CPUDump(CPUDisassembly* disas, CPUMultiDump* multiDump, QWidget* parent) : HexDump(parent)
{
    mDisas = disas;
    mCurrentVa = 0;
    mMultiDump = multiDump;

    switch((ViewEnum_t)ConfigUint("HexDump", "DefaultView"))
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
    case ViewIntegerSignedShort:
        integerSignedShortSlot();
        break;
    case ViewIntegerSignedLong:
        integerSignedLongSlot();
        break;
#ifdef _WIN64
    case ViewIntegerSignedLongLong:
        integerSignedLongLongSlot();
        break;
#endif //_WIN64
    case ViewIntegerUnsignedShort:
        integerUnsignedShortSlot();
        break;
    case ViewIntegerUnsignedLong:
        integerUnsignedLongSlot();
        break;
#ifdef _WIN64
    case ViewIntegerUnsignedLongLong:
        integerUnsignedLongLongSlot();
        break;
#endif //_WIN64
    case ViewIntegerHexShort:
        integerHexShortSlot();
        break;
    case ViewIntegerHexLong:
        integerHexLongSlot();
        break;
#ifdef _WIN64
    case ViewIntegerHexLongLong:
        integerHexLongLongSlot();
        break;
#endif //_WIN64
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
        addressSlot();
        break;
    default:
        hexAsciiSlot();
        break;
    }

    connect(this, SIGNAL(selectionUpdated()), this, SLOT(selectionUpdatedSlot()));

    setupContextMenu();

    mGoto = 0;
}

void CPUDump::setupContextMenu()
{
    //Binary menu
    mBinaryMenu = new QMenu("B&inary", this);
    mBinaryMenu->setIcon(QIcon(":/icons/images/binary.png"));

    //Binary->Edit
    mBinaryEditAction = new QAction("&Edit", this);
    mBinaryEditAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryEditAction);
    connect(mBinaryEditAction, SIGNAL(triggered()), this, SLOT(binaryEditSlot()));
    mBinaryMenu->addAction(mBinaryEditAction);

    //Binary->Fill
    mBinaryFillAction = new QAction("&Fill...", this);
    mBinaryFillAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryFillAction);
    connect(mBinaryFillAction, SIGNAL(triggered()), this, SLOT(binaryFillSlot()));
    mBinaryMenu->addAction(mBinaryFillAction);

    //Binary->Separator
    mBinaryMenu->addSeparator();

    //Binary->Copy
    mBinaryCopyAction = new QAction("&Copy", this);
    mBinaryCopyAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryCopyAction);
    connect(mBinaryCopyAction, SIGNAL(triggered()), this, SLOT(binaryCopySlot()));
    mBinaryMenu->addAction(mBinaryCopyAction);

    //Binary->Paste
    mBinaryPasteAction = new QAction("&Paste", this);
    mBinaryPasteAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteAction);
    connect(mBinaryPasteAction, SIGNAL(triggered()), this, SLOT(binaryPasteSlot()));
    mBinaryMenu->addAction(mBinaryPasteAction);

    //Binary->Paste (Ignore Size)
    mBinaryPasteIgnoreSizeAction = new QAction("Paste (&Ignore Size)", this);
    mBinaryPasteIgnoreSizeAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinaryPasteIgnoreSizeAction);
    connect(mBinaryPasteIgnoreSizeAction, SIGNAL(triggered()), this, SLOT(binaryPasteIgnoreSizeSlot()));
    mBinaryMenu->addAction(mBinaryPasteIgnoreSizeAction);

    //Binary->Save To a File
    mBinarySaveToFile = new QAction("Save To a File", this);
    mBinarySaveToFile->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mBinarySaveToFile);
    connect(mBinarySaveToFile, SIGNAL(triggered()), this, SLOT(binarySaveToFileSlot()));
    mBinaryMenu->addAction(mBinarySaveToFile);


    // Restore Selection
    mUndoSelection = new QAction("&Restore selection", this);
    mUndoSelection->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mUndoSelection);
    connect(mUndoSelection, SIGNAL(triggered()), this, SLOT(undoSelectionSlot()));

    // Follow in Stack
    mFollowStack = new QAction("Follow in Stack", this);
    connect(mFollowStack, SIGNAL(triggered()), this, SLOT(followStackSlot()));

    // Follow in Disasm
    mFollowInDisasm = new QAction("Follow in Disassembler", this);
    connect(mFollowInDisasm, SIGNAL(triggered()), this, SLOT(followInDisasmSlot()));

    //Follow DWORD/QWORD
#ifdef _WIN64
    mFollowData = new QAction("&Follow QWORD in Disassembler", this);
#else //x86
    mFollowData = new QAction("&Follow DWORD in Disassembler", this);
#endif //_WIN64
    connect(mFollowData, SIGNAL(triggered()), this, SLOT(followDataSlot()));

    //Follow DWORD/QWORD in Disasm
#ifdef _WIN64
    mFollowDataDump = new QAction("&Follow QWORD in Current Dump", this);
#else //x86
    mFollowDataDump = new QAction("&Follow DWORD in Current Dump", this);
#endif //_WIN64
    connect(mFollowDataDump, SIGNAL(triggered()), this, SLOT(followDataDumpSlot()));

#ifdef _WIN64
    mFollowInDumpMenu = new QMenu("&Follow QWORD in Dump", this);
#else //x86
    mFollowInDumpMenu = new QMenu("&Follow DWORD in Dump", this);
#endif //_WIN64

    int maxDumps = mMultiDump->getMaxCPUTabs();
    for(int i = 0; i < maxDumps; i++)
    {
        QAction* action = new QAction(QString("Dump %1").arg(i + 1), this);
        connect(action, SIGNAL(triggered()), this, SLOT(followInDumpNSlot()));
        mFollowInDumpMenu->addAction(action);
        mFollowInDumpActions.push_back(action);
    }

    //Entropy
    mEntropy = new QAction(QIcon(":/icons/images/entropy.png"), "Entropy...", this);
    connect(mEntropy, SIGNAL(triggered()), this, SLOT(entropySlot()));

    //Label
    mSetLabelAction = new QAction("Set Label", this);
    mSetLabelAction->setShortcutContext(Qt::WidgetShortcut);
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
    mMemoryAccessRestore = new QAction("&Restore on hit", this);
    connect(mMemoryAccessRestore, SIGNAL(triggered()), this, SLOT(memoryAccessRestoreSlot()));
    mMemoryAccessMenu->addAction(mMemoryAccessRestore);
    mBreakpointMenu->addMenu(mMemoryAccessMenu);

    //Breakpoint->Memory Write
    mMemoryWriteMenu = new QMenu("Memory, Write", this);
    mMemoryWriteSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryWriteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryWriteSingleshootSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteSingleshoot);
    mMemoryWriteRestore = new QAction("&Restore on hit", this);
    connect(mMemoryWriteRestore, SIGNAL(triggered()), this, SLOT(memoryWriteRestoreSlot()));
    mMemoryWriteMenu->addAction(mMemoryWriteRestore);
    mBreakpointMenu->addMenu(mMemoryWriteMenu);

    //Breakpoint->Memory Execute
    mMemoryExecuteMenu = new QMenu("Memory, Execute", this);
    mMemoryExecuteSingleshoot = new QAction("&Singleshoot", this);
    connect(mMemoryExecuteSingleshoot, SIGNAL(triggered()), this, SLOT(memoryExecuteSingleshootSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteSingleshoot);
    mMemoryExecuteRestore = new QAction("&Restore on hit", this);
    connect(mMemoryExecuteRestore, SIGNAL(triggered()), this, SLOT(memoryExecuteRestoreSlot()));
    mMemoryExecuteMenu->addAction(mMemoryExecuteRestore);
    mBreakpointMenu->addMenu(mMemoryExecuteMenu);

    //Breakpoint->Remove Memory
    mMemoryRemove = new QAction("Remove &Memory", this);
    connect(mMemoryRemove, SIGNAL(triggered()), this, SLOT(memoryRemoveSlot()));
    mBreakpointMenu->addAction(mMemoryRemove);

    //Find Pattern
    mFindPatternAction = new QAction("&Find Pattern...", this);
    mFindPatternAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mFindPatternAction);
    connect(mFindPatternAction, SIGNAL(triggered()), this, SLOT(findPattern()));

    //Yara
    mYaraAction = new QAction(QIcon(":/icons/images/yara.png"), "&Yara...", this);
    mYaraAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mYaraAction);
    connect(mYaraAction, SIGNAL(triggered()), this, SLOT(yaraSlot()));

    //Data copy
    mDataCopyAction = new QAction(QIcon(":/icons/images/data-copy.png"), "Data copy...", this);
    connect(mDataCopyAction, SIGNAL(triggered()), this, SLOT(dataCopySlot()));

    //Find References
    mFindReferencesAction = new QAction("Find &References", this);
    mFindReferencesAction->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mFindReferencesAction);
    connect(mFindReferencesAction, SIGNAL(triggered()), this, SLOT(findReferencesSlot()));

    //Goto menu
    mGotoMenu = new QMenu("&Goto", this);

    //Goto->Expression
    mGotoExpression = new QAction("&Expression", this);
    mGotoExpression->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoExpression);
    connect(mGotoExpression, SIGNAL(triggered()), this, SLOT(gotoExpressionSlot()));
    mGotoMenu->addAction(mGotoExpression);

    // Goto->File offset
    mGotoFileOffset = new QAction("File Offset", this);
    mGotoFileOffset->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoFileOffset);
    connect(mGotoFileOffset, SIGNAL(triggered()), this, SLOT(gotoFileOffsetSlot()));
    mGotoMenu->addAction(mGotoFileOffset);

    // Goto->Previous
    mGotoPrevious = new QAction("Previous", this);
    mGotoPrevious->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoPrevious);
    connect(mGotoPrevious, SIGNAL(triggered()), this, SLOT(gotoPrevSlot()));
    mGotoMenu->addAction(mGotoPrevious);

    // Goto->Next
    mGotoNext = new QAction("Next", this);
    mGotoNext->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoNext);
    connect(mGotoNext, SIGNAL(triggered()), this, SLOT(gotoNextSlot()));
    mGotoMenu->addAction(mGotoNext);


    // Goto->Start of page
    mGotoStart = new QAction("Start of Page", this);
    mGotoStart->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoStart);
    connect(mGotoStart, SIGNAL(triggered()), this, SLOT(gotoStartSlot()));
    mGotoMenu->addAction(mGotoStart);

    // Goto->End of page
    mGotoEnd = new QAction("End of Page", this);
    mGotoEnd->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mGotoEnd);
    connect(mGotoEnd, SIGNAL(triggered()), this, SLOT(gotoEndSlot()));
    mGotoMenu->addAction(mGotoEnd);

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

    //Plugins
    mPluginMenu = new QMenu(this);
    Bridge::getBridge()->emitMenuAddToList(this, mPluginMenu, GUI_DUMP_MENU);

    //Copy
    mCopyMenu = new QMenu("&Copy", this);
    mCopyMenu->setIcon(QIcon(":/icons/images/copy.png"));

    // Copy -> Address
    mCopyAddress = new QAction("&Address", this);
    connect(mCopyAddress, SIGNAL(triggered()), this, SLOT(copyAddressSlot()));
    mCopyAddress->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(mCopyAddress);
    mCopyMenu->addAction(mCopyAddress);

    // Copy -> RVA
    mCopyRva = new QAction("&RVA", this);
    connect(mCopyRva, SIGNAL(triggered()), this, SLOT(copyRvaSlot()));
    mCopyMenu->addAction(mCopyRva);

    mEncoding = Config()->createMenu_Encoding(this);
    connect(Config(), SIGNAL(encodingUpdated()), this, SLOT(reloadData()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void CPUDump::refreshShortcutsSlot()
{
    mBinaryEditAction->setShortcut(ConfigShortcut("ActionBinaryEdit"));
    mBinaryFillAction->setShortcut(ConfigShortcut("ActionBinaryFill"));
    mBinaryCopyAction->setShortcut(ConfigShortcut("ActionBinaryCopy"));
    mBinaryPasteAction->setShortcut(ConfigShortcut("ActionBinaryPaste"));
    mBinaryPasteIgnoreSizeAction->setShortcut(ConfigShortcut("ActionBinaryPasteIgnoreSize"));
    mUndoSelection->setShortcut(ConfigShortcut("ActionUndoSelection"));
    mSetLabelAction->setShortcut(ConfigShortcut("ActionSetLabel"));
    mFindPatternAction->setShortcut(ConfigShortcut("ActionFindPattern"));
    mFindReferencesAction->setShortcut(ConfigShortcut("ActionFindReferences"));
    mGotoExpression->setShortcut(ConfigShortcut("ActionGotoExpression"));
    mGotoPrevious->setShortcut(ConfigShortcut("ActionGotoPrevious"));
    mGotoNext->setShortcut(ConfigShortcut("ActionGotoNext"));
    mGotoStart->setShortcut(ConfigShortcut("ActionGotoStart"));
    mGotoEnd->setShortcut(ConfigShortcut("ActionGotoEnd"));
    mGotoFileOffset->setShortcut(ConfigShortcut("ActionGotoFileOffset"));
    mYaraAction->setShortcut(ConfigShortcut("ActionYara"));
    mCopyAddress->setShortcut(ConfigShortcut("ActionCopyAddress"));
}

QString CPUDump::paintContent(QPainter* painter, dsint rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    // Reset byte offset when base address is reached
    if(rowBase == 0 && mByteOffset != 0)
        printDumpAt(mMemPage->getBase(), false, false);

    QString wStr = "";
    if(!col) //address
    {
        char label[MAX_LABEL_SIZE] = "";
        QString addrText = "";
        dsint cur_addr = rvaToVa((rowBase + rowOffset) * getBytePerRowCount() - mByteOffset);
        if(mRvaDisplayEnabled) //RVA display
        {
            dsint rva = cur_addr - mRvaDisplayBase;
            if(rva == 0)
            {
#ifdef _WIN64
                addrText = "$ ==>            ";
#else
                addrText = "$ ==>    ";
#endif //_WIN64
            }
            else if(rva > 0)
            {
#ifdef _WIN64
                addrText = "$+" + QString("%1").arg(rva, -15, 16, QChar(' ')).toUpper();
#else
                addrText = "$+" + QString("%1").arg(rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
            }
            else if(rva < 0)
            {
#ifdef _WIN64
                addrText = "$-" + QString("%1").arg(-rva, -15, 16, QChar(' ')).toUpper();
#else
                addrText = "$-" + QString("%1").arg(-rva, -7, 16, QChar(' ')).toUpper();
#endif //_WIN64
            }
        }
        addrText += QString("%1").arg(cur_addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        if(DbgGetLabelAt(cur_addr, SEG_DEFAULT, label)) //has label
        {
            char module[MAX_MODULE_SIZE] = "";
            if(DbgGetModuleAt(cur_addr, module) && !QString(label).startsWith("JMP.&"))
                addrText += " <" + QString(module) + "." + QString(label) + ">";
            else
                addrText += " <" + QString(label) + ">";
        }
        else
            *label = 0;
        if(*label) //label
        {
            QColor background = ConfigColor("HexDumpLabelBackgroundColor");
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill bookmark color
            painter->setPen(ConfigColor("HexDumpLabelColor")); //TODO: config
        }
        else
        {
            QColor background = ConfigColor("HexDumpAddressBackgroundColor");
            if(background.alpha())
                painter->fillRect(QRect(x, y, w, h), QBrush(background)); //fill bookmark color
            painter->setPen(ConfigColor("HexDumpAddressColor")); //TODO: config
        }
        painter->drawText(QRect(x + 4, y , w - 4 , h), Qt::AlignVCenter | Qt::AlignLeft, addrText);
    }
    else if(col && mDescriptor.at(col - 1).isData == false && mDescriptor.at(col - 1).itemCount == 1) //print comments
    {
        duint data = 0;
        dsint wRva = (rowBase + rowOffset) * getBytePerRowCount() - mByteOffset;
        mMemPage->read((byte_t*)&data, wRva, sizeof(duint));
        char modname[MAX_MODULE_SIZE] = "";
        if(!DbgGetModuleAt(data, modname))
            modname[0] = '\0';
        char label_text[MAX_LABEL_SIZE] = "";
        if(DbgGetLabelAt(data, SEG_DEFAULT, label_text))
            wStr = QString(modname) + "." + QString(label_text);
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

    dsint selectedAddr = rvaToVa(getInitialSelection());

    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addMenu(mBinaryMenu);
    wMenu->addMenu(mCopyMenu);
    dsint start = rvaToVa(getSelectionStart());
    dsint end = rvaToVa(getSelectionEnd());
    if(DbgFunctions()->PatchInRange(start, end)) //nothing patched in selected range
        wMenu->addAction(mUndoSelection);
    if(DbgMemIsValidReadPtr(start) && DbgMemFindBaseAddr(start, 0) == DbgMemFindBaseAddr(DbgValFromString("csp"), 0))
        wMenu->addAction(mFollowStack);
    wMenu->addAction(mFollowInDisasm);

    duint ptr = 0;
    DbgMemRead(selectedAddr, (unsigned char*)&ptr, sizeof(duint));
    if(DbgMemIsValidReadPtr(ptr))
    {
        wMenu->addAction(mFollowData);
        wMenu->addAction(mFollowDataDump);
        wMenu->addMenu(mFollowInDumpMenu);
    }

    if(historyHasPrev())
        mGotoPrevious->setVisible(true);
    else
        mGotoPrevious->setVisible(false);

    if(historyHasNext())
        mGotoNext->setVisible(true);
    else
        mGotoNext->setVisible(false);

    wMenu->addAction(mSetLabelAction);
    wMenu->addMenu(mBreakpointMenu);
    wMenu->addAction(mFindPatternAction);
    wMenu->addAction(mFindReferencesAction);
    wMenu->addAction(mYaraAction);
    wMenu->addAction(mDataCopyAction);
    wMenu->addMenu(mGotoMenu);
    wMenu->addAction(mEntropy);
    wMenu->addSeparator();
    wMenu->addMenu(mHexMenu);
    wMenu->addMenu(mTextMenu);
    wMenu->addMenu(mIntegerMenu);
    wMenu->addMenu(mFloatMenu);
    wMenu->addAction(mAddressAction);
    wMenu->addAction(mDisassemblyAction);
    wMenu->addMenu(mEncoding);

    QList<QString> tabNames;
    mMultiDump->getTabNames(tabNames);
    for(int i = 0; i < tabNames.length(); i++)
        mFollowInDumpActions[i]->setText(tabNames[i]);

    if((DbgGetBpxTypeAt(selectedAddr) & bp_hardware) == bp_hardware) //hardware breakpoint set
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
    if((DbgGetBpxTypeAt(selectedAddr) & bp_memory) == bp_memory) //memory breakpoint set
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

    wMenu->addSeparator();
    wMenu->addActions(mPluginMenu->actions());

    wMenu->exec(event->globalPos()); //execute context menu
}

void CPUDump::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
        return;
    switch(getColumnIndexFromX(event->x()))
    {
    case 0: //address
    {
        //very ugly way to calculate the base of the current row (no clue why it works)
        dsint deltaRowBase = getInitialSelection() % getBytePerRowCount() + mByteOffset;
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
        binaryEditSlot();
    }
    break;
    }
}

void CPUDump::mouseMoveEvent(QMouseEvent* event)
{
    dsint ptr = 0, ptrValue = 0;

    // Get mouse pointer relative position
    int x = event->x();
    int y = event->y();

    // Get HexDump own RVA address, then VA in memory
    int rva = getItemStartingAddress(x, y);
    int va = rvaToVa(rva);

    // Read VA
    DbgMemRead(va, (unsigned char*)&ptr, sizeof(dsint));

    // Check if its a pointer
    if(DbgMemIsValidReadPtr(ptr))
    {
        // Get the value at the address pointed by the ptr
        DbgMemRead(ptr, (unsigned char*)&ptrValue, sizeof(dsint));

        // Format text like this : [ptr] = ptrValue
        QString strPtrValue;
#ifdef _WIN64
        strPtrValue.sprintf("[0x%016X] = 0x%016X", ptr, ptrValue);
#else
        strPtrValue.sprintf("[0x%08X] = 0x%08X", ptr, ptrValue);
#endif

        // Show tooltip
        QToolTip::showText(event->globalPos(), strPtrValue);
    }
    // If not a pointer, hide tooltips
    else
    {
        QToolTip::hideText();
    }

    HexDump::mouseMoveEvent(event);
}

void CPUDump::addVaToHistory(dsint parVa)
{
    mVaHistory.push_back(parVa);
    if(mVaHistory.size() > 1)
        mCurrentVa++;
}

bool CPUDump::historyHasPrev()
{
    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return false;
    return true;
}

bool CPUDump::historyHasNext()
{
    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return false;
    return true;
}

void CPUDump::historyPrev()
{
    if(!historyHasPrev())
        return;

    if(!mCurrentVa || !mVaHistory.size()) //we are at the earliest history entry
        return;
    mCurrentVa--;
    printDumpAt(mVaHistory.at(mCurrentVa));
}

void CPUDump::historyNext()
{
    if(!historyHasNext())
        return;

    int size = mVaHistory.size();
    if(!size || mCurrentVa >= mVaHistory.size() - 1) //we are at the newest history entry
        return;
    mCurrentVa++;
    printDumpAt(mVaHistory.at(mCurrentVa));
}

void CPUDump::historyClear()
{
    mCurrentVa = 0;
    mVaHistory.clear();
}

void CPUDump::setLabelSlot()
{
    if(!DbgIsDebugging())
        return;

    duint wVA = rvaToVa(getSelectionStart());
    LineEditDialog mLineEdit(this);
    QString addr_text = QString("%1").arg(wVA, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    char label_text[MAX_COMMENT_SIZE] = "";
    if(DbgGetLabelAt((duint)wVA, SEG_DEFAULT, label_text))
        mLineEdit.setText(QString(label_text));
    mLineEdit.setWindowTitle("Add label at " + addr_text);
    if(mLineEdit.exec() != QDialog::Accepted)
        return;
    if(!DbgSetLabelAt(wVA, mLineEdit.editText.toUtf8().constData()))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "DbgSetLabelAt failed!");
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
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
    if(mGoto->exec() == QDialog::Accepted)
    {
        QString cmd;
        DbgCmdExec(cmd.sprintf("dump \"%s\"", mGoto->expressionText.toUtf8().constData()).toUtf8().constData());
    }
}

void CPUDump::gotoFileOffsetSlot()
{
    if(!DbgIsDebugging())
        return;
    char modname[MAX_MODULE_SIZE] = "";
    if(!DbgFunctions()->ModNameFromAddr(rvaToVa(getInitialSelection()), modname, true))
    {
        QMessageBox::critical(this, "Error!", "Not inside a module...");
        return;
    }
    GotoDialog mGotoDialog(this);
    mGotoDialog.fileOffset = true;
    mGotoDialog.modName = QString(modname);
    mGotoDialog.setWindowTitle("Goto File Offset in " + QString(modname));
    if(mGotoDialog.exec() != QDialog::Accepted)
        return;
    duint value = DbgValFromString(mGotoDialog.expressionText.toUtf8().constData());
    value = DbgFunctions()->FileOffsetToVa(modname, value);
    DbgCmdExec(QString().sprintf("dump \"%p\"", value).toUtf8().constData());
}

void CPUDump::gotoStartSlot()
{
    duint dest = mMemPage->getBase();
    DbgCmdExec(QString().sprintf("dump \"%p\"", dest).toUtf8().constData());
}

void CPUDump::gotoEndSlot()
{
    duint dest = mMemPage->getBase() + mMemPage->getSize() - (getViewableRowsCount() * getBytePerRowCount());
    DbgCmdExec(QString().sprintf("dump \"%p\"", dest).toUtf8().constData());
}

void CPUDump::hexAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, "Hex", false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 16;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 16, "ASCII", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::hexUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewHexUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 16;
    wColDesc.separator = 4;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 47, "Hex", false, wColDesc);

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendDescriptor(8 + charwidth * 16, "UNICODE", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::textAsciiSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextAscii);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, "ASCII", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::textUnicodeSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewTextUnicode);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unicode short
    wColDesc.itemCount = 64;
    wColDesc.separator = 0;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendResetDescriptor(8 + charwidth * 64, "UNICODE", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = SignedDecWord;
    appendResetDescriptor(8 + charwidth * 55, "Signed short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = SignedDecDword;
    appendResetDescriptor(8 + charwidth * 47, "Signed long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerSignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerSignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //signed long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = SignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, "Signed long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedShort);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = UnsignedDecWord;
    appendResetDescriptor(8 + charwidth * 47, "Unsigned short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = UnsignedDecDword;
    appendResetDescriptor(8 + charwidth * 43, "Unsigned long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerUnsignedLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerUnsignedLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //unsigned long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = UnsignedDecQword;
    appendResetDescriptor(8 + charwidth * 41, "Unsigned long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexShortSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexShort);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex short
    wColDesc.itemCount = 8;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Word;
    wColDesc.data.wordMode = HexWord;
    appendResetDescriptor(8 + charwidth * 34, "Hex short (16-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex long
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
    appendResetDescriptor(8 + charwidth * 35, "Hex long (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::integerHexLongLongSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewIntegerHexLongLong);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex long long
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
    appendResetDescriptor(8 + charwidth * 33, "Hex long long (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatFloatSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatFloat);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float dword
    wColDesc.itemCount = 4;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = FloatDword;
    appendResetDescriptor(8 + charwidth * 55, "Float (32-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = DoubleQword;
    appendResetDescriptor(8 + charwidth * 47, "Double (64-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::floatLongDoubleSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewFloatLongDouble);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 2;
    wColDesc.separator = 0;
    wColDesc.data.itemSize = Tword;
    wColDesc.data.twordMode = FloatTword;
    appendResetDescriptor(8 + charwidth * 59, "Long double (80-bit)", false, wColDesc);

    wColDesc.isData = false; //empty column
    wColDesc.itemCount = 0;
    wColDesc.separator = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "", false, wColDesc);

    reloadData();
}

void CPUDump::addressSlot()
{
    Config()->setUint("HexDump", "DefaultView", (duint)ViewAddress);
    int charwidth = getCharWidth();
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

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
    appendResetDescriptor(8 + charwidth * 2 * sizeof(duint), "Address", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 1;
    wColDesc.separator = 0;
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
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void CPUDump::selectionGet(SELECTIONDATA* selection)
{
    selection->start = rvaToVa(getSelectionStart());
    selection->end = rvaToVa(getSelectionEnd());
    Bridge::getBridge()->setResult(1);
}

void CPUDump::selectionSet(const SELECTIONDATA* selection)
{
    dsint selMin = mMemPage->getBase();
    dsint selMax = selMin + mMemPage->getSize();
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

void CPUDump::memoryAccessSingleshootSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 0, r").toUtf8().constData());
}

void CPUDump::memoryAccessRestoreSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 1, r").toUtf8().constData());
}

void CPUDump::memoryWriteSingleshootSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 0, w").toUtf8().constData());
}

void CPUDump::memoryWriteRestoreSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 1, w").toUtf8().constData());
}

void CPUDump::memoryExecuteSingleshootSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 0, x").toUtf8().constData());
}

void CPUDump::memoryExecuteRestoreSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpm " + addr_text + ", 1, x").toUtf8().constData());
}

void CPUDump::memoryRemoveSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bpmc " + addr_text).toUtf8().constData());
}

void CPUDump::hardwareAccess1Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 1").toUtf8().constData());
}

void CPUDump::hardwareAccess2Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 2").toUtf8().constData());
}

void CPUDump::hardwareAccess4Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 4").toUtf8().constData());
}

void CPUDump::hardwareAccess8Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", r, 8").toUtf8().constData());
}

void CPUDump::hardwareWrite1Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 1").toUtf8().constData());
}

void CPUDump::hardwareWrite2Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 2").toUtf8().constData());
}

void CPUDump::hardwareWrite4Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 4").toUtf8().constData());
}

void CPUDump::hardwareWrite8Slot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", w, 8").toUtf8().constData());
}

void CPUDump::hardwareExecuteSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphws " + addr_text + ", x").toUtf8().constData());
}

void CPUDump::hardwareRemoveSlot()
{
    QString addr_text = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("bphwc " + addr_text).toUtf8().constData());
}

void CPUDump::findReferencesSlot()
{
    QString addrStart = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString addrEnd = QString("%1").arg(rvaToVa(getSelectionEnd()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString addrDisasm = QString("%1").arg(mDisas->rvaToVa(mDisas->getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findrefrange " + addrStart + ", " + addrEnd + ", " + addrDisasm).toUtf8().constData());
    emit displayReferencesWidget();
}

void CPUDump::binaryEditSlot()
{
    HexEditDialog hexEdit(this);
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    byte_t* data = new byte_t[selSize];
    mMemPage->read(data, selStart, selSize);
    hexEdit.mHexEdit->setData(QByteArray((const char*)data, selSize));
    delete [] data;
    hexEdit.setWindowTitle("Edit data at " + QString("%1").arg(rvaToVa(selStart), sizeof(dsint) * 2, 16, QChar('0')).toUpper());
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
    hexEdit.setWindowTitle("Fill data at " + QString("%1").arg(rvaToVa(selStart), sizeof(dsint) * 2, 16, QChar('0')).toUpper());
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
    QString fileName = QFileDialog::getSaveFileName(this, "Save to file", QDir::currentPath(), tr("All files (*.*)"));
    if(fileName.length())
    {
        // Get starting selection and selection size, then convert selStart to VA
        dsint rvaSelStart = getSelectionStart();
        dsint selSize = getSelectionEnd() - rvaSelStart + 1;
        dsint vaSelStart = rvaToVa(rvaSelStart);

        // Prepare command
        fileName = QDir::toNativeSeparators(fileName);
        QString cmd = QString("savedata %1,%2,%3").arg(fileName, QString::number(vaSelStart, 16), QString::number(selSize));
        DbgCmdExec(cmd.toUtf8().constData());
    }
}

void CPUDump::findPattern()
{
    HexEditDialog hexEdit(this);
    hexEdit.showEntireBlock(true);
    hexEdit.mHexEdit->setOverwriteMode(false);
    hexEdit.setWindowTitle("Find Pattern...");
    if(hexEdit.exec() != QDialog::Accepted)
        return;
    dsint addr = rvaToVa(getSelectionStart());
    if(hexEdit.entireBlock())
        addr = DbgMemFindBaseAddr(addr, 0);
    QString addrText = QString("%1").arg(addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("findall " + addrText + ", " + hexEdit.mHexEdit->pattern() + ", &data&").toUtf8().constData());
    emit displayReferencesWidget();
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

void CPUDump::followStackSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("sdump " + addrText).toUtf8().constData());
}

void CPUDump::followInDisasmSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("disasm " + addrText).toUtf8().constData());
}

void CPUDump::followDataSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("disasm [%1]").arg(addrText).toUtf8().constData());
}

void CPUDump::followDataDumpSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExec(QString("dump [%1]").arg(addrText).toUtf8().constData());
}

void CPUDump::selectionUpdatedSlot()
{
    QString selStart = QString("%1").arg(rvaToVa(getSelectionStart()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString selEnd = QString("%1").arg(rvaToVa(getSelectionEnd()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString info = "Dump";
    char mod[MAX_MODULE_SIZE] = "";
    if(DbgFunctions()->ModNameFromAddr(rvaToVa(getSelectionStart()), mod, true))
        info = QString(mod) + "";
    GuiAddStatusBarMessage(QString(info + ": " + selStart + " -> " + selEnd + QString().sprintf(" (0x%.8X bytes)\n", getSelectionEnd() - getSelectionStart() + 1)).toUtf8().constData());
}

void CPUDump::yaraSlot()
{
    YaraRuleSelectionDialog yaraDialog(this);
    if(yaraDialog.exec() == QDialog::Accepted)
    {
        QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        DbgCmdExec(QString("yara \"%0\",%1").arg(yaraDialog.getSelectedFile()).arg(addrText).toUtf8().constData());
        emit displayReferencesWidget();
    }
}

void CPUDump::dataCopySlot()
{
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QVector<byte_t> data;
    data.resize(selSize);
    mMemPage->read(data.data(), selStart, selSize);
    DataCopyDialog dataDialog(&data, this);
    dataDialog.exec();
}

void CPUDump::entropySlot()
{
    dsint selStart = getSelectionStart();
    dsint selSize = getSelectionEnd() - selStart + 1;
    QVector<byte_t> data;
    data.resize(selSize);
    mMemPage->read(data.data(), selStart, selSize);

    EntropyDialog entropyDialog(this);
    entropyDialog.setWindowTitle(QString().sprintf("Entropy (Address: %p, Size: %p)", selStart, selSize));
    entropyDialog.show();
    entropyDialog.GraphMemory(data.constData(), data.size());
    entropyDialog.exec();
}

void CPUDump::copyAddressSlot()
{
    QString addrText = QString("%1").arg(rvaToVa(getInitialSelection()), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    Bridge::CopyToClipboard(addrText);
}

void CPUDump::copyRvaSlot()
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

void CPUDump::followInDumpNSlot()
{
    for(int i = 0; i < mFollowInDumpActions.length(); i++)
    {
        if(mFollowInDumpActions[i] == sender())
        {
            DbgCmdExec(QString("dump [%1], %2").arg(ToPtrString(rvaToVa(getSelectionStart()))).arg(i).toUtf8().constData());
        }
    }
}

void CPUDump::gotoNextSlot()
{
    historyNext();
}

void CPUDump::gotoPrevSlot()
{
    historyPrev();
}


