#include "CPUDump.h"

CPUDump::CPUDump(QWidget *parent) : HexDump(parent)
{
    int charwidth=QFontMetrics(this->font()).width(QChar(' '));
    ColumnDescriptor_t wColDesc;
    DataDescriptor_t dDesc;

    wColDesc.isData = true; //hex byte
    wColDesc.itemCount = 8;
    dDesc.itemSize = Byte;
    dDesc.byteMode = HexByte;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*23, "Hex", false, wColDesc);

    wColDesc.isData = true; //ascii byte
    wColDesc.itemCount = 8;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(8+charwidth*15, "ASCII", false, wColDesc);

    wColDesc.isData = true; //float qword
    wColDesc.itemCount = 1;
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = DoubleQword;
    appendDescriptor(8+charwidth*23, "Double", false, wColDesc);

    wColDesc.isData = true; //void*
    wColDesc.itemCount = 1;
#ifdef _WIN64
    wColDesc.data.itemSize = Qword;
    wColDesc.data.qwordMode = HexQword;
#else
    wColDesc.data.itemSize = Dword;
    wColDesc.data.dwordMode = HexDword;
#endif
    appendDescriptor(8+charwidth*2*sizeof(uint_t), "void*", false, wColDesc);

    wColDesc.isData = false; //comments
    wColDesc.itemCount = 0;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendDescriptor(0, "Comments", false, wColDesc);

    connect(Bridge::getBridge(), SIGNAL(dumpAt(int_t)), this, SLOT(printDumpAt(int_t)));

    setupContextMenu();
}

void CPUDump::setupContextMenu()
{
    //Hex menu
    mHexMenu = new QMenu("&Hex", this);
    //Hex->Ascii
    mHexAsciiAction = new QAction("&Ascii", this);
    this->addAction(mHexAsciiAction);
    connect(mHexAsciiAction, SIGNAL(triggered()), this, SLOT(hexAsciiSlot()));
    mHexMenu->addAction(mHexAsciiAction);
    //Hex->Unicode
    mHexUnicodeAction = new QAction("&Unicode", this);
    this->addAction(mHexUnicodeAction);
    connect(mHexUnicodeAction, SIGNAL(triggered()), this, SLOT(hexUnicodeSlot()));
    mHexMenu->addAction(mHexUnicodeAction);

    //Text menu
    mTextMenu = new QMenu("&Text", this);
    //Text->Ascii
    mTextAsciiAction = new QAction("&Ascii", this);
    this->addAction(mTextAsciiAction);
    connect(mTextAsciiAction, SIGNAL(triggered()), this, SLOT(textAsciiSlot()));
    mTextMenu->addAction(mTextAsciiAction);
    //Text->Unicode
    mTextUnicodeAction = new QAction("&Unicode", this);
    this->addAction(mTextUnicodeAction);
    connect(mTextUnicodeAction, SIGNAL(triggered()), this, SLOT(textUnicodeSlot()));
    mTextMenu->addAction(mTextUnicodeAction);

    //Integer menu
    mIntegerMenu = new QMenu("&Integer", this);
    //Integer->Signed short
    mIntegerSignedShortAction = new QAction("Signed short (16-bit)", this);
    this->addAction(mIntegerSignedShortAction);
    connect(mIntegerSignedShortAction, SIGNAL(triggered()), this, SLOT(integerSignedShortSlot()));
    mIntegerMenu->addAction(mIntegerSignedShortAction);
    //Integer->Signed long
    mIntegerSignedLongAction = new QAction("Signed long (32-bit)", this);
    this->addAction(mIntegerSignedLongAction);
    connect(mIntegerSignedLongAction, SIGNAL(triggered()), this, SLOT(integerSignedLongSlot()));
    mIntegerMenu->addAction(mIntegerSignedLongAction);
#ifdef _WIN64
    //Integer->Signed long long
    mIntegerSignedLongLongAction = new QAction("Signed long long (64-bit)", this);
    this->addAction(mIntegerSignedLongLongAction);
    connect(mIntegerSignedLongLongAction, SIGNAL(triggered()), this, SLOT(integerSignedLongLongSlot()));
    mIntegerMenu->addAction(mIntegerSignedLongLongAction);
#endif //_WIN64
    //Integer->Unsigned short
    mIntegerUnsignedShortAction = new QAction("Unsigned short (16-bit)", this);
    this->addAction(mIntegerUnsignedShortAction);
    connect(mIntegerUnsignedShortAction, SIGNAL(triggered()), this, SLOT(integerUnsignedShortSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedShortAction);
    //Integer->Unsigned long
    mIntegerUnsignedLongAction = new QAction("Unsigned long (32-bit)", this);
    this->addAction(mIntegerUnsignedLongAction);
    connect(mIntegerUnsignedLongAction, SIGNAL(triggered()), this, SLOT(integerUnsignedLongSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedLongAction);
#ifdef _WIN64
    //Integer->Unsigned long long
    mIntegerUnsignedLongLongAction = new QAction("Unsigned long long (64-bit)", this);
    this->addAction(mIntegerUnsignedLongLongAction);
    connect(mIntegerUnsignedLongLongAction, SIGNAL(triggered()), this, SLOT(integerUnsignedLongLongSlot()));
    mIntegerMenu->addAction(mIntegerUnsignedLongLongAction);
#endif //_WIN64
    //Integer->Hex short
    mIntegerHexShortAction = new QAction("Hex short (16-bit)", this);
    this->addAction(mIntegerHexShortAction);
    connect(mIntegerHexShortAction, SIGNAL(triggered()), this, SLOT(integerHexShortSlot()));
    mIntegerMenu->addAction(mIntegerHexShortAction);
    //Integer->Hex long
    mIntegerHexLongAction = new QAction("Hex long (32-bit)", this);
    this->addAction(mIntegerHexLongAction);
    connect(mIntegerHexLongAction, SIGNAL(triggered()), this, SLOT(integerHexLongSlot()));
    mIntegerMenu->addAction(mIntegerHexLongAction);
#ifdef _WIN64
    //Integer->Hex long long
    mIntegerHexLongLongAction = new QAction("Hex long long (64-bit)", this);
    this->addAction(mIntegerHexLongLongAction);
    connect(mIntegerHexLongLongAction, SIGNAL(triggered()), this, SLOT(integerHexLongLongSlot()));
    mIntegerMenu->addAction(mIntegerHexLongLongAction);
#endif //_WIN64

    //Float menu
    mFloatMenu = new QMenu("&Float", this);
    //Float->float
    mFloatFloatAction = new QAction("&Float (32-bit)", this);
    this->addAction(mFloatFloatAction);
    connect(mFloatFloatAction, SIGNAL(triggered()), this, SLOT(floatFloatSlot()));
    mFloatMenu->addAction(mFloatFloatAction);
    //Float->double
    mFloatDoubleAction = new QAction("&Double (64-bit)", this);
    this->addAction(mFloatDoubleAction);
    connect(mFloatDoubleAction, SIGNAL(triggered()), this, SLOT(floatDoubleSlot()));
    mFloatMenu->addAction(mFloatDoubleAction);
    //Float->long double
    mFloatLongDoubleAction = new QAction("&Long double (80-bit)", this);
    this->addAction(mFloatLongDoubleAction);
    connect(mFloatLongDoubleAction, SIGNAL(triggered()), this, SLOT(floatLongDoubleSlot()));
    mFloatMenu->addAction(mFloatLongDoubleAction);

    //Address
    mAddressAction = new QAction("&Address", this);
    this->addAction(mAddressAction);
    connect(mAddressAction, SIGNAL(triggered()), this, SLOT(addressSlot()));

    //Disassembly
    mDisassemblyAction = new QAction("&Disassembly", this);
    this->addAction(mDisassemblyAction);
    connect(mDisassemblyAction, SIGNAL(triggered()), this, SLOT(disassemblySlot()));
}

QString CPUDump::paintContent(QPainter* painter, int_t rowBase, int rowOffset, int col, int x, int y, int w, int h)
{
    QString wStr = "";
    if(col && mDescriptor.at(col - 1).isData == false && mDescriptor.at(col -1).itemCount == 1) //print comments
    {
        uint_t data=0;
        int_t wRva = (rowBase + rowOffset) * getBytePerRowCount() - mByteOffset;
        mMemPage->readOriginalMemory((byte_t*)&data, wRva, sizeof(uint_t));
        char label_text[MAX_LABEL_SIZE]="";
        if(DbgGetLabelAt(data, SEG_DEFAULT, label_text))
            wStr=QString(label_text);
    }
    else
        wStr = HexDump::paintContent(painter, rowBase, rowOffset, col, x, y, w, h);
    return wStr;
}

void CPUDump::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu* wMenu = new QMenu(this); //create context menu
    wMenu->addMenu(mHexMenu);
    wMenu->addMenu(mTextMenu);
    wMenu->addMenu(mIntegerMenu);
    wMenu->addMenu(mFloatMenu);
    wMenu->addAction(mAddressAction);
    wMenu->addAction(mDisassemblyAction);
    wMenu->exec(event->globalPos()); //execute context menu
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
    appendDescriptor(8+charwidth*31, "ASCII", false, wColDesc);

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
    appendDescriptor(8+charwidth*15, "UNICODE", false, wColDesc);

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
    wColDesc.itemCount = 32;
    dDesc.itemSize = Byte;
    dDesc.byteMode = AsciiByte;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*63, "ASCII", false, wColDesc);

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
    wColDesc.itemCount = 32;
    dDesc.itemSize = Word;
    dDesc.wordMode = UnicodeWord;
    wColDesc.data = dDesc;
    appendResetDescriptor(8+charwidth*63, "UNICODE", false, wColDesc);

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
