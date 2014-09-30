#include "RegistersView.h"
#include <QClipboard>
#include "Configuration.h"
#include "WordEditDialog.h"
#include "LineEditDialog.h"


RegistersView::RegistersView(QWidget* parent) : QScrollArea(parent), mVScrollOffset(0)
{
    // precreate ContextMenu Actions
    wCM_Increment = new QAction(tr("Increment"), this);
    wCM_Increment->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_Increment);
    wCM_Decrement = new QAction(tr("Decrement"), this);
    wCM_Decrement->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_Decrement);
    wCM_Zero = new QAction(tr("Zero"), this);
    wCM_Zero->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_Zero);
    wCM_SetToOne = new QAction(tr("Set to 1"), this);
    wCM_SetToOne->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_SetToOne);
    wCM_Modify = new QAction(tr("Modify Value"), this);
    wCM_Modify->setShortcut(QKeySequence(Qt::Key_Enter));
    wCM_ToggleValue = new QAction(tr("Toggle"), this);
    wCM_ToggleValue->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_ToggleValue);
    wCM_CopyToClipboard = new QAction(tr("Copy Value to Clipboard"), this);
    wCM_CopyToClipboard->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_CopyToClipboard);
    wCM_CopySymbolToClipboard = new QAction(tr("Copy Symbol Value to Clipboard"), this);
    wCM_CopySymbolToClipboard->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_CopySymbolToClipboard);
    wCM_FollowInDisassembly = new QAction(tr("Follow in Disassembler"), this);
    wCM_FollowInDump = new QAction(tr("Follow in Dump"), this);
    wCM_FollowInStack = new QAction("Follow in Stack", this);

    // general purposes register (we allow the user to modify the value)
    mGPR.insert(CAX);
    mGPR.insert(CBX);
    mGPR.insert(CCX);
    mGPR.insert(CDX);
    mGPR.insert(CBP);
    mGPR.insert(CSP);
    mGPR.insert(CSI);
    mGPR.insert(CDI);
    mGPR.insert(R8);
    mGPR.insert(R9);
    mGPR.insert(R10);
    mGPR.insert(R11);
    mGPR.insert(R12);
    mGPR.insert(R13);
    mGPR.insert(R14);
    mGPR.insert(R15);
    mGPR.insert(EFLAGS);

    // flags (we allow the user to toggle them)
    mFlags.insert(CF);
    mFlags.insert(PF);
    mFlags.insert(AF);
    mFlags.insert(ZF);
    mFlags.insert(SF);
    mFlags.insert(TF);
    mFlags.insert(IF);
    mFlags.insert(DF);
    mFlags.insert(OF);

    // FPU x87 and MMX registers
    mFPUx87.insert(x87r0);
    mFPUx87.insert(x87r1);
    mFPUx87.insert(x87r2);
    mFPUx87.insert(x87r3);
    mFPUx87.insert(x87r4);
    mFPUx87.insert(x87r5);
    mFPUx87.insert(x87r6);
    mFPUx87.insert(x87r7);
    mFPUMMX.insert(MM0);
    mFPUMMX.insert(MM1);
    mFPUMMX.insert(MM2);
    mFPUMMX.insert(MM3);
    mFPUMMX.insert(MM4);
    mFPUMMX.insert(MM5);
    mFPUMMX.insert(MM6);
    mFPUMMX.insert(MM7);

    //registers that should not be changed
    mNoChange.insert(GS);
    mNoChange.insert(FS);
    mNoChange.insert(ES);
    mNoChange.insert(DS);
    mNoChange.insert(CS);
    mNoChange.insert(SS);
    mNoChange.insert(DR0);
    mNoChange.insert(DR1);
    mNoChange.insert(DR2);
    mNoChange.insert(DR3);
    mNoChange.insert(DR6);
    mNoChange.insert(DR7);
    mNoChange.insert(CIP);

    // create mapping from internal id to name
    mRegisterMapping.clear();
    mRegisterPlaces.clear();
    int offset = 0;

    /* Register_Position is a struct definition the position
     *
     * (line , start, labelwidth, valuesize )
     */
#ifdef _WIN64
    mRegisterMapping.insert(CAX, "RAX");
    mRegisterPlaces.insert(CAX, Register_Position(0, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBX, "RBX");
    mRegisterPlaces.insert(CBX, Register_Position(1, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CCX, "RCX");
    mRegisterPlaces.insert(CCX, Register_Position(2, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDX, "RDX");
    mRegisterPlaces.insert(CDX, Register_Position(3, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSI, "RSI");
    mRegisterPlaces.insert(CSI, Register_Position(6, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDI, "RDI");
    mRegisterPlaces.insert(CDI, Register_Position(7, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBP, "RBP");
    mRegisterPlaces.insert(CBP, Register_Position(4, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSP, "RSP");
    mRegisterPlaces.insert(CSP, Register_Position(5, 0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(R8, "R8");
    mRegisterPlaces.insert(R8 , Register_Position(9, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R9, "R9");
    mRegisterPlaces.insert(R9 , Register_Position(10, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R10, "R10");
    mRegisterPlaces.insert(R10, Register_Position(11, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R11, "R11");
    mRegisterPlaces.insert(R11, Register_Position(12, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R12, "R12");
    mRegisterPlaces.insert(R12, Register_Position(13, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R13, "R13");
    mRegisterPlaces.insert(R13, Register_Position(14, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R14, "R14");
    mRegisterPlaces.insert(R14, Register_Position(15, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R15, "R15");
    mRegisterPlaces.insert(R15, Register_Position(16, 0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(CIP, "RIP");
    mRegisterPlaces.insert(CIP, Register_Position(18, 0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(EFLAGS, "RFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(20, 0, 9, sizeof(uint_t) * 2));

    offset = 21;
#else
    mRegisterMapping.insert(CAX, "EAX");
    mRegisterPlaces.insert(CAX, Register_Position(0, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBX, "EBX");
    mRegisterPlaces.insert(CBX, Register_Position(1, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CCX, "ECX");
    mRegisterPlaces.insert(CCX, Register_Position(2, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDX, "EDX");
    mRegisterPlaces.insert(CDX, Register_Position(3, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSI, "ESI");
    mRegisterPlaces.insert(CSI, Register_Position(6, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDI, "EDI");
    mRegisterPlaces.insert(CDI, Register_Position(7, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBP, "EBP");
    mRegisterPlaces.insert(CBP, Register_Position(4, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSP, "ESP");
    mRegisterPlaces.insert(CSP, Register_Position(5, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CIP, "EIP");
    mRegisterPlaces.insert(CIP, Register_Position(9, 0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(EFLAGS, "EFLAGS");
    mRegisterPlaces.insert(EFLAGS, Register_Position(11, 0, 9, sizeof(uint_t) * 2));

    offset = 12;
#endif
    mRegisterMapping.insert(ZF, "ZF");
    mRegisterPlaces.insert(ZF, Register_Position(offset + 0, 0, 3, 1));
    mRegisterMapping.insert(OF, "OF");
    mRegisterPlaces.insert(OF, Register_Position(offset + 1, 0, 3, 1));
    mRegisterMapping.insert(CF, "CF");
    mRegisterPlaces.insert(CF, Register_Position(offset + 2, 0, 3, 1));

    mRegisterMapping.insert(PF, "PF");
    mRegisterPlaces.insert(PF, Register_Position(offset + 0, 6, 3, 1));
    mRegisterMapping.insert(SF, "SF");
    mRegisterPlaces.insert(SF, Register_Position(offset + 1, 6, 3, 1));
    mRegisterMapping.insert(TF, "TF");
    mRegisterPlaces.insert(TF, Register_Position(offset + 2, 6, 3, 1));

    mRegisterMapping.insert(AF, "AF");
    mRegisterPlaces.insert(AF, Register_Position(offset + 0, 12, 3, 1));
    mRegisterMapping.insert(DF, "DF");
    mRegisterPlaces.insert(DF, Register_Position(offset + 1, 12, 3, 1));
    mRegisterMapping.insert(IF, "IF");
    mRegisterPlaces.insert(IF, Register_Position(offset + 2, 12, 3, 1));

    offset++;
    mRegisterMapping.insert(GS, "GS");
    mRegisterPlaces.insert(GS, Register_Position(offset + 3, 0, 3, 4));
    mRegisterMapping.insert(ES, "ES");
    mRegisterPlaces.insert(ES, Register_Position(offset + 4, 0, 3, 4));
    mRegisterMapping.insert(CS, "CS");
    mRegisterPlaces.insert(CS, Register_Position(offset + 5, 0, 3, 4));

    mRegisterMapping.insert(FS, "FS");
    mRegisterPlaces.insert(FS, Register_Position(offset + 3, 9, 3, 4));
    mRegisterMapping.insert(DS, "DS");
    mRegisterPlaces.insert(DS, Register_Position(offset + 4, 9, 3, 4));
    mRegisterMapping.insert(SS, "SS");
    mRegisterPlaces.insert(SS, Register_Position(offset + 5, 9, 3, 4));

    offset++;

    mRegisterMapping.insert(x87r0, "x87r0");
    mRegisterPlaces.insert(x87r0, Register_Position(offset + 6, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r1, "x87r1");
    mRegisterPlaces.insert(x87r1, Register_Position(offset + 7, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r2, "x87r2");
    mRegisterPlaces.insert(x87r2, Register_Position(offset + 8, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r3, "x87r3");
    mRegisterPlaces.insert(x87r3, Register_Position(offset + 9, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r4, "x87r4");
    mRegisterPlaces.insert(x87r4, Register_Position(offset + 10, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r5, "x87r5");
    mRegisterPlaces.insert(x87r5, Register_Position(offset + 11, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r6, "x87r6");
    mRegisterPlaces.insert(x87r6, Register_Position(offset + 12, 0, 6, 10 * 2));
    mRegisterMapping.insert(x87r7, "x87r7");
    mRegisterPlaces.insert(x87r7, Register_Position(offset + 13, 0, 6, 10 * 2));

    offset++;

    mRegisterMapping.insert(MM0, "MM0");
    mRegisterPlaces.insert(MM0, Register_Position(offset + 14, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM1, "MM1");
    mRegisterPlaces.insert(MM1, Register_Position(offset + 15, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM2, "MM2");
    mRegisterPlaces.insert(MM2, Register_Position(offset + 16, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM3, "MM3");
    mRegisterPlaces.insert(MM3, Register_Position(offset + 17, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM4, "MM4");
    mRegisterPlaces.insert(MM4, Register_Position(offset + 18, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM5, "MM5");
    mRegisterPlaces.insert(MM5, Register_Position(offset + 19, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM6, "MM6");
    mRegisterPlaces.insert(MM6, Register_Position(offset + 20, 0, 4, 8 * 2));
    mRegisterMapping.insert(MM7, "MM7");
    mRegisterPlaces.insert(MM7, Register_Position(offset + 21, 0, 4, 8 * 2));

    offset++;

    mRegisterMapping.insert(DR0, "DR0");
    mRegisterPlaces.insert(DR0, Register_Position(offset + 22, 0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR1, "DR1");
    mRegisterPlaces.insert(DR1, Register_Position(offset + 23, 0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR2, "DR2");
    mRegisterPlaces.insert(DR2, Register_Position(offset + 24, 0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR3, "DR3");
    mRegisterPlaces.insert(DR3, Register_Position(offset + 25, 0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR6, "DR6");
    mRegisterPlaces.insert(DR6, Register_Position(offset + 26, 0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR7, "DR7");
    mRegisterPlaces.insert(DR7, Register_Position(offset + 27, 0, 4, sizeof(uint_t) * 2));

    fontsUpdatedSlot();
    connect(Config(), SIGNAL(fontsUpdated()), this, SLOT(fontsUpdatedSlot()));

    memset(&wRegDumpStruct, 0, sizeof(REGDUMP));
    memset(&wCipRegDumpStruct, 0, sizeof(REGDUMP));
    mCip = 0;
    mRegisterUpdates.clear();

    mRowsNeeded = offset + 16;
    mRowsNeeded++;
    yTopSpacing = 3; //set top spacing (in pixels)

    // Context Menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    // foreign messages
    connect(Bridge::getBridge(), SIGNAL(updateRegisters()), this, SLOT(updateRegistersSlot()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    // self communication for repainting (maybe some other widgets needs this information, too)
    connect(this, SIGNAL(refresh()), this, SLOT(repaint()));
    // context menu actions
    connect(wCM_Increment, SIGNAL(triggered()), this, SLOT(onIncrementAction()));
    connect(wCM_Decrement, SIGNAL(triggered()), this, SLOT(onDecrementAction()));
    connect(wCM_Zero, SIGNAL(triggered()), this, SLOT(onZeroAction()));
    connect(wCM_SetToOne, SIGNAL(triggered()), this, SLOT(onSetToOneAction()));
    connect(wCM_Modify, SIGNAL(triggered()), this, SLOT(onModifyAction()));
    connect(wCM_ToggleValue, SIGNAL(triggered()), this, SLOT(onToggleValueAction()));
    connect(wCM_CopyToClipboard, SIGNAL(triggered()), this, SLOT(onCopyToClipboardAction()));
    connect(wCM_CopySymbolToClipboard, SIGNAL(triggered()), this, SLOT(onCopySymbolToClipboardAction()));
    connect(wCM_FollowInDisassembly, SIGNAL(triggered()), this, SLOT(onFollowInDisassembly()));
    connect(wCM_FollowInDump, SIGNAL(triggered()), this, SLOT(onFollowInDump()));
    connect(wCM_FollowInStack, SIGNAL(triggered()), this, SLOT(onFollowInStack()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void RegistersView::refreshShortcutsSlot()
{
    wCM_Increment->setShortcut(ConfigShortcut("ActionIncreaseRegister"));
    wCM_Decrement->setShortcut(ConfigShortcut("ActionDecreaseRegister"));
    wCM_Zero->setShortcut(ConfigShortcut("ActionZeroRegister"));
    wCM_SetToOne->setShortcut(ConfigShortcut("ActionSetOneRegister"));
    wCM_ToggleValue->setShortcut(ConfigShortcut("ActionToggleRegisterValue"));
    wCM_CopyToClipboard->setShortcut(ConfigShortcut("ActionCopy"));
    wCM_CopySymbolToClipboard->setShortcut(ConfigShortcut("ActionCopySymbol"));
}

RegistersView::~RegistersView()
{
}

void RegistersView::fontsUpdatedSlot()
{
    setFont(ConfigFont("Registers"));
    int wRowsHeight = QFontMetrics(this->font()).height();
    wRowsHeight = (wRowsHeight * 105) / 100;
    wRowsHeight = (wRowsHeight % 2) == 0 ? wRowsHeight : wRowsHeight + 1;
    mRowHeight = wRowsHeight;
    mCharWidth = QFontMetrics(this->font()).averageCharWidth();
    repaint();
}

/**
 * @brief retrieves the register id from given corrdinates of the viewport
 * @param line
 * @param offset (x-coord)
 * @param resulting register-id
 * @return true if register found
 */
bool RegistersView::identifyRegister(const int line, const int offset, REGISTER_NAME* clickedReg)
{
    // we start by an unknown register id
    if(clickedReg)
        *clickedReg = UNKNOWN;
    bool found_flag = false;
    QMap<REGISTER_NAME, Register_Position>::const_iterator it = mRegisterPlaces.begin();
    // iterate all registers that being displayed
    while(it != mRegisterPlaces.end())
    {
        if((it.value().line == (line - mVScrollOffset))    /* same line ? */
                && ((1 + it.value().start) <= offset)   /* between start ... ? */
                && (offset <= (1 + it.value().start + it.value().labelwidth + it.value().valuesize)) /* ... and end ? */
          )
        {
            // we found a matching register in the viewport
            if(clickedReg)
                *clickedReg = (REGISTER_NAME)it.key();
            found_flag = true;
            break;

        }
        ++it;
    }
    return found_flag;
}

void RegistersView::mousePressEvent(QMouseEvent* event)
{
    if(!DbgIsDebugging())
        return;
    // get mouse position
    const int y = (event->y() - 3) / (double)mRowHeight;
    const int x = event->x() / (double)mCharWidth;

    REGISTER_NAME r;
    // do we find a corresponding register?
    if(identifyRegister(y, x, &r))
    {
        mSelected = r;
        emit refresh();
    }
    else
        mSelected = UNKNOWN;
}

void RegistersView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    if(!DbgIsDebugging() || event->button() != Qt::LeftButton)
        return;
    // get mouse position
    const int y = (event->y() - 3) / (double)mRowHeight;
    const int x = event->x() / (double)mCharWidth;

    // do we find a corresponding register?
    if(!identifyRegister(y, x, 0))
        return;
    // is current register general purposes register or FPU register?
    if(mGPR.contains(mSelected) || mFPUx87.contains(mSelected) || mFPUMMX.contains(mSelected))
    {
        wCM_Modify->trigger();
    }
    else if(mFlags.contains(mSelected))  // is flag ?
        wCM_ToggleValue->trigger();
    else if(mSelected == CIP) //double clicked on CIP register
        DbgCmdExec("disasm cip");
}

void RegistersView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter wPainter(this->viewport());
    wPainter.fillRect(wPainter.viewport(), QBrush(ConfigColor("RegistersBackgroundColor")));

    QMap<REGISTER_NAME, QString>::const_iterator it = mRegisterMapping.begin();
    // iterate all registers
    while(it != mRegisterMapping.end())
    {
        // paint register at given position
        drawRegister(&wPainter, it.key(), registerValue(&wRegDumpStruct, it.key()));
        it++;
    }
}

void RegistersView::keyPressEvent(QKeyEvent* event)
{
    if(!DbgIsDebugging())
        return;
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        wCM_Modify->trigger();
}

QSize RegistersView::sizeHint() const
{
    // 32 character width
    return QSize(32 * mCharWidth , this->viewport()->height());
}

QString RegistersView::getRegisterLabel(REGISTER_NAME register_selected)
{
    char label_text[MAX_LABEL_SIZE] = "";
    char module_text[MAX_MODULE_SIZE] = "";
    char string_text[MAX_STRING_SIZE] = "";

    QString valueText = QString("%1").arg((* ((uint_t*) registerValue(&wRegDumpStruct, register_selected))), mRegisterPlaces[register_selected].valuesize, 16, QChar('0')).toUpper();
    duint register_value = (* ((uint_t*) registerValue(&wRegDumpStruct, register_selected)));
    QString newText = QString("");

    bool hasString = DbgGetStringAt(register_value, string_text);
    bool hasLabel = DbgGetLabelAt(register_value, SEG_DEFAULT, label_text);
    bool hasModule = DbgGetModuleAt(register_value, module_text);

    if(hasString && register_selected != CIP)
    {
        newText = string_text;
    }
    else if(hasLabel && hasModule && register_selected != CIP)
    {
        newText = "<" + QString(module_text) + "." + QString(label_text) + ">";
    }
    else if(hasModule)
    {
        newText = QString(module_text) + "." + valueText;
    }
    else if(hasLabel && register_selected != CIP)
    {
        newText = "<" + QString(label_text) + ">";
    }
    else if(register_selected != CIP)
    {
        bool isCharacter = false;
        if(register_value == (register_value & 0xFF))
        {
            QChar c = QChar((char)register_value);
            if(c.isPrint())
            {
                newText = QString("'%1'").arg((char)register_value);
                isCharacter = IsCharacterRegister(register_selected);
            }
        }
        else if(register_value == (register_value & 0xFFF))  //UNICODE?
        {
            QChar c = QChar((wchar_t)register_value);
            if(c.isPrint())
            {
                newText = "L'" + QString(c) + "'";
                isCharacter = IsCharacterRegister(register_selected);
            }
        }
    }

    return newText;
}

#include <limits>
#include <cmath>
double readFloat80(const uint8_t buffer[10])
{
    /*
     * WE ARE LOSSING 2 BYTES WITH THIS FUNCTION.
     * TODO: CHANGE THIS FOR ONE BETTER.
    */
    //80 bit floating point value according to IEEE-754:
    //1 bit sign, 15 bit exponent, 64 bit mantissa

    const uint16_t SIGNBIT    = 1 << 15;
    const uint16_t EXP_BIAS   = (1 << 14) - 1; // 2^(n-1) - 1 = 16383
    const uint16_t SPECIALEXP = (1 << 15) - 1; // all bits set
    const uint64_t HIGHBIT    = (uint64_t)1 << 63;
    const uint64_t QUIETBIT   = (uint64_t)1 << 62;

    // Extract sign, exponent and mantissa
    uint16_t exponent = *((uint16_t*)&buffer[8]);
    uint64_t mantissa = *((uint64_t*)&buffer[0]);

    double sign = (exponent & SIGNBIT) ? -1.0 : 1.0;
    exponent   &= ~SIGNBIT;

    // Check for undefined values
    if((!exponent && (mantissa & HIGHBIT)) || (exponent && !(mantissa & HIGHBIT)))
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Check for special values (infinity, NaN)
    if(exponent == 0)
    {
        if(mantissa == 0)
        {
            return sign * 0.0;
        }
        else
        {
            // denormalized
        }
    }
    else if(exponent == SPECIALEXP)
    {
        if(!(mantissa & ~HIGHBIT))
        {
            return sign * std::numeric_limits<double>::infinity();
        }
        else
        {
            if(mantissa & QUIETBIT)
            {
                return std::numeric_limits<double>::quiet_NaN();
            }
            else
            {
                return std::numeric_limits<double>::signaling_NaN();
            }
        }
    }

    //value = (-1)^s * (m / 2^63) * 2^(e - 16383)
    double significand = ((double)mantissa / ((uint64_t)1 << 63));
    return sign * ldexp(significand, exponent - EXP_BIAS);
}

void RegistersView::drawRegister(QPainter* p, REGISTER_NAME reg, char* value)
{
    // is the register-id known?
    if(mRegisterMapping.contains(reg))
    {
        uint_t nouint_value;
        bool enable_label_detection = false;
        switch(reg)
        {
        case CAX:
        case CCX:
        case CDX:
        case CBX:
        case CDI:
        case CBP:
        case CSI:
        case CSP:
        case R8:
        case R9:
        case R10:
        case R11:
        case R12:
        case R13:
        case R14:
        case R15:
        case CIP:
        case DR0:
        case DR1:
        case DR2:
        case DR3:
        case DR6:
        case DR7:
            enable_label_detection = true;
            break;

        case CF:
        case PF:
        case AF:
        case ZF:
        case SF:
        case TF:
        case IF:
        case DF:
        case OF:
            nouint_value = * ((bool*) value);
            value = (char*) & nouint_value;
            break;

        case GS:
        case FS:
        case ES:
        case DS:
        case CS:
        case SS:
            nouint_value = * ((unsigned short*) value);
            value = (char*) & nouint_value;
            break;
        }

        // padding to the left is at least one character (looks better)
        int x = mCharWidth * (1 + mRegisterPlaces[reg].start);
        int ySpace = yTopSpacing;
        if(mVScrollOffset != 0)
            ySpace = 0;
        int y = mRowHeight * (mRegisterPlaces[reg].line + mVScrollOffset) + ySpace;

        //draw raster
        /*
        p->save();
        p->setPen(QColor("#FF0000"));
        p->drawLine(0, y, this->viewport()->width(), y);
        p->restore();
        */

        // draw name of value
        int width = mCharWidth * mRegisterMapping[reg].length();
        p->setPen(ConfigColor("RegistersLabelColor"));
        p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, mRegisterMapping[reg]);
        x += (mRegisterPlaces[reg].labelwidth) * mCharWidth;
        //p->drawText(offset,mRowHeight*(mRegisterPlaces[reg].line+1),mRegisterMapping[reg]);

        //set highlighting
        if(DbgIsDebugging() && mRegisterUpdates.contains(reg))
            p->setPen(ConfigColor("RegistersModifiedColor"));
        else
            p->setPen(ConfigColor("RegistersColor"));

        //selection
        if(mSelected == reg)
        {
            p->fillRect(x, y, mRegisterPlaces[reg].valuesize * mCharWidth, mRowHeight, QBrush(ConfigColor("RegistersSelectionColor")));
            //p->fillRect(QRect(x + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line)+2, mRegisterPlaces[reg].valuesize*mCharWidth, mRowHeight), QBrush(ConfigColor("RegistersSelectionColor")));
        }

        QString valueText;
        // draw value
        if(mFPUx87.contains(reg) || mFPUMMX.contains(reg))
        {
            SIZE_T size;
            if(mFPUx87.contains(reg))
                size = 10;
            else
                size = 8;
            valueText = QString(QByteArray(value, size).toHex()).toUpper();
        }
        else
            valueText = QString("%1").arg((* ((uint_t*) value)), mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();

        width = mCharWidth * valueText.length();
        p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, valueText);
        //p->drawText(x + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line+1),QString("%1").arg(value, mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper());

        x += valueText.length() * mCharWidth;

        if((mFPUx87.contains(reg) || mFPUMMX.contains(reg)) && DbgIsDebugging())
        {
            x += 1 * mCharWidth; //1 space
            QString newText;
            if(mFPUx87.contains(reg))
            {
                newText = QString("ST%1 ").arg(((x87FPURegister_t*) registerValue(&wRegDumpStruct, reg))->st_value);
                newText += QString::number(readFloat80(((x87FPURegister_t*) registerValue(&wRegDumpStruct, reg))->data));
            }
            else
            {
                newText = QString::number(* (double*)(((x87FPURegister_t*) registerValue(&wRegDumpStruct, reg))->data));
            }
            width = newText.length() * mCharWidth;
            p->setPen(ConfigColor("RegistersExtraInfoColor"));
            p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);
        }

        // do we have a label ?
        if(enable_label_detection)
        {
            x += 5 * mCharWidth; //5 spaces

            QString newText = getRegisterLabel(reg);

            // are there additional informations?
            if(newText != "")
            {
                width = newText.length() * mCharWidth;
                p->setPen(ConfigColor("RegistersExtraInfoColor"));
                p->drawText(x, y, width, mRowHeight, Qt::AlignVCenter, newText);
                //p->drawText(x,mRowHeight*(mRegisterPlaces[reg].line+1),newText);
            }
        }
    }
}

void RegistersView::updateRegistersSlot()
{
    // read registers
    REGDUMP z;
    memset(&z, 0, sizeof(REGDUMP));
    DbgGetRegDump(&z);
    // update gui
    setRegisters(&z);
}

void RegistersView::displayEditDialog()
{
    if(!mFPUx87.contains(mSelected) && !mFPUMMX.contains(mSelected))
    {
        WordEditDialog wEditDial(this);
        wEditDial.setup(QString("Edit"), (* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))), sizeof(int_t));
        if(wEditDial.exec() == QDialog::Accepted) //OK button clicked
            setRegister(mSelected, wEditDial.getVal());
    }
    else
    {
        /*
        LineEditDialog mLineEdit(this);
        SIZE_T size;
        if (mFPUx87.contains(mSelected))
            size = 10 * 2;
        else if (mFPUMMX.contains(mSelected))
            size = 8 * 2;
        else
            size = sizeof(int_t) * 2;

        mLineEdit.setText(QString("%1").arg((uint_t)registerValue(&wRegDumpStruct, mSelected), size, 16, QChar('0')).toUpper());
        mLineEdit.setWindowTitle("Edit FPU register");
        mLineEdit.setWindowIcon(QIcon(":/icons/images/log.png"));
        mLineEdit.setCursorPosition(0);

        if(mLineEdit.exec() != QDialog::Accepted)
            return; //pressed cancel
        */
    }
}

void RegistersView::onIncrementAction()
{
    if(mGPR.contains(mSelected))
        setRegister(mSelected, (* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))) + 1);
}

void RegistersView::onDecrementAction()
{
    if(mGPR.contains(mSelected))
        setRegister(mSelected, (* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))) - 1);
}

void RegistersView::onZeroAction()
{
    if(!mNoChange.contains(mSelected))
        setRegister(mSelected, 0);
}

void RegistersView::onSetToOneAction()
{
    if(!mNoChange.contains(mSelected))
        setRegister(mSelected, 1);
}

void RegistersView::onModifyAction()
{
    if(mGPR.contains(mSelected) || mFPUx87.contains(mSelected) || mFPUMMX.contains(mSelected))
        displayEditDialog();
}

void RegistersView::onToggleValueAction()
{
    if(mFlags.contains(mSelected))
    {
        int value = (int)(* (bool*) registerValue(&wRegDumpStruct, mSelected));
        setRegister(mSelected, value ^ 1);
    }
    else
    {
        int_t val = (* ((uint_t*) registerValue(&wRegDumpStruct, mSelected)));
        val++;
        val *= -1;
        setRegister(mSelected, val);
    }
}

void RegistersView::onCopyToClipboardAction()
{
    QClipboard* clipboard = QApplication::clipboard();
    QString valueText;
    if(mFPUx87.contains(mSelected) || mFPUMMX.contains(mSelected))
    {
        SIZE_T size;
        char* value;
        if(mFPUx87.contains(mSelected))
        {
            value = (char*)((x87FPURegister_t*) registerValue(&wRegDumpStruct, mSelected))->data;
            size = 10;
        }
        else
        {
            value = (char*) registerValue(&wRegDumpStruct, mSelected);
            size = 8;
        }
        valueText = QString(QByteArray(value, size).toHex()).toUpper();
    }
    else
        valueText = QString("%1").arg((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
    clipboard->setText(valueText);
}

void RegistersView::onCopySymbolToClipboardAction()
{
    if(mGPR.contains(mSelected))
    {
        QClipboard* clipboard = QApplication::clipboard();
        QString symbol = getRegisterLabel(mSelected);
        if(symbol != "")
            clipboard->setText(symbol);
    }
}

void RegistersView::onFollowInDisassembly()
{
    if(mGPR.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("disasm \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onFollowInDump()
{
    if(mGPR.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("dump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::onFollowInStack()
{
    if(mGPR.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("sdump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void RegistersView::displayCustomContextMenuSlot(QPoint pos)
{
    if(!DbgIsDebugging())
        return;
    QMenu wMenu(this);

    if(mSelected != UNKNOWN)
    {
        if(!mNoChange.contains(mSelected) && !mFPUx87.contains(mSelected) && !mFPUMMX.contains(mSelected))
        {
            if((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))) >= 1)
                wMenu.addAction(wCM_Zero);
            if((* ((uint_t*) registerValue(&wRegDumpStruct, mSelected))) == 0)
                wMenu.addAction(wCM_SetToOne);
            wMenu.addAction(wCM_ToggleValue);
        }

        if(mGPR.contains(mSelected) || mFPUx87.contains(mSelected) || mFPUMMX.contains(mSelected))
        {
            wMenu.addAction(wCM_Modify);

            if(mGPR.contains(mSelected))
            {
                wMenu.addAction(wCM_Increment);
                wMenu.addAction(wCM_Decrement);

                uint_t addr = (* ((uint_t*) registerValue(&wRegDumpStruct, mSelected)));
                if(DbgMemIsValidReadPtr(addr))
                {
                    wMenu.addAction(wCM_FollowInDump);
                    wMenu.addAction(wCM_FollowInDisassembly);
                    duint size = 0;
                    duint base = DbgMemFindBaseAddr(DbgValFromString("csp"), &size);
                    if(addr >= base && addr < base + size)
                        wMenu.addAction(wCM_FollowInStack);
                }
            }
        }
        wMenu.addAction(wCM_CopyToClipboard);
        QString symbol = getRegisterLabel(mSelected);
        if(symbol != "")
            wMenu.addAction(wCM_CopySymbolToClipboard);
        wMenu.exec(this->mapToGlobal(pos));
    }
    else
    {
        wMenu.addSeparator();
#ifdef _WIN64
        QAction* wHwbpCsp = wMenu.addAction("HW Break on [RSP]");
#else
        QAction* wHwbpCsp = wMenu.addAction("HW Break on [ESP]");
#endif
        QAction* wAction = wMenu.exec(this->mapToGlobal(pos));

        if(wAction == wHwbpCsp)
            DbgCmdExec("bphws csp,rw");
    }
}

void RegistersView::setRegister(REGISTER_NAME reg, uint_t value)
{
    // is register-id known?
    if(mRegisterMapping.contains(reg))
    {
        // map "cax" to "eax" or "rax"
        QString wRegName = mRegisterMapping.constFind(reg).value();

        // flags need to '!' infront
        if(mFlags.contains(reg))
            wRegName = "!" + wRegName;


        // we change the value (so highlight it)
        mRegisterUpdates.insert(reg);
        // tell everything the compiler
        DbgValToString(wRegName.toUtf8().constData(), value);
        // force repaint
        emit refresh();
    }
}

void RegistersView::debugStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
    {
        updateRegistersSlot();
    }
}

void RegistersView::repaint()
{
    this->viewport()->repaint();
}


uint_t RegistersView::GetUintValue(REGISTER_NAME reg, char* value)
{
    switch(reg)
    {
    case CF:
    case PF:
    case AF:
    case ZF:
    case SF:
    case TF:
    case IF:
    case DF:
    case OF:
        return (uint_t) * ((bool*) value);
        break;

    case GS:
    case FS:
    case ES:
    case DS:
    case CS:
    case SS:
        return (uint_t) * ((unsigned short*) value);
        break;
    }

    return * ((uint_t*) value);
}

char* RegistersView::registerValue(const REGDUMP* regd, const REGISTER_NAME reg)
{
    static int null_value = 0;
    // this is probably the most efficient general method to access the values of the struct

    if(reg == CAX) return (char*) & (regd->titcontext.cax);
    if(reg == CBX) return (char*) & (regd->titcontext.cbx);
    if(reg == CCX) return (char*) & (regd->titcontext.ccx);
    if(reg == CDX) return (char*) & (regd->titcontext.cdx);
    if(reg == CSI) return (char*) & (regd->titcontext.csi);
    if(reg == CDI) return (char*) & (regd->titcontext.cdi);
    if(reg == CBP) return (char*) & (regd->titcontext.cbp);
    if(reg == CSP) return (char*) & (regd->titcontext.csp);

    if(reg == CIP) return (char*) & (regd->titcontext.cip);
    if(reg == EFLAGS) return (char*) & (regd->titcontext.eflags);
#ifdef _WIN64
    if(reg == R8) return (char*) & (regd->titcontext.r8);
    if(reg == R9) return (char*) & (regd->titcontext.r9);
    if(reg == R10) return (char*) & (regd->titcontext.r10);
    if(reg == R11) return (char*) & (regd->titcontext.r11);
    if(reg == R12) return (char*) & (regd->titcontext.r12);
    if(reg == R13) return (char*) & (regd->titcontext.r13);
    if(reg == R14) return (char*) & (regd->titcontext.r14);
    if(reg == R15) return (char*) & (regd->titcontext.r15);
#endif
    // CF,PF,AF,ZF,SF,TF,IF,DF,OF
    if(reg == CF) return (char*) & (regd->flags.c);
    if(reg == PF) return (char*) & (regd->flags.p);
    if(reg == AF) return (char*) & (regd->flags.a);
    if(reg == ZF) return (char*) & (regd->flags.z);
    if(reg == SF) return (char*) & (regd->flags.s);
    if(reg == TF) return (char*) & (regd->flags.t);
    if(reg == IF) return (char*) & (regd->flags.i);
    if(reg == DF) return (char*) & (regd->flags.d);
    if(reg == OF) return (char*) & (regd->flags.o);

    // GS,FS,ES,DS,CS,SS
    if(reg == GS) return (char*) & (regd->titcontext.gs);
    if(reg == FS) return (char*) & (regd->titcontext.fs);
    if(reg == ES) return (char*) & (regd->titcontext.es);
    if(reg == DS) return (char*) & (regd->titcontext.ds);
    if(reg == CS) return (char*) & (regd->titcontext.cs);
    if(reg == SS) return (char*) & (regd->titcontext.ss);

    if(reg == DR0) return (char*) & (regd->titcontext.dr0);
    if(reg == DR1) return (char*) & (regd->titcontext.dr1);
    if(reg == DR2) return (char*) & (regd->titcontext.dr2);
    if(reg == DR3) return (char*) & (regd->titcontext.dr3);
    if(reg == DR6) return (char*) & (regd->titcontext.dr6);
    if(reg == DR7) return (char*) & (regd->titcontext.dr7);

    if(reg == MM0) return (char*) & (regd->titcontext.mmx[0]);
    if(reg == MM1) return (char*) & (regd->titcontext.mmx[1]);
    if(reg == MM2) return (char*) & (regd->titcontext.mmx[2]);
    if(reg == MM3) return (char*) & (regd->titcontext.mmx[3]);
    if(reg == MM4) return (char*) & (regd->titcontext.mmx[4]);
    if(reg == MM5) return (char*) & (regd->titcontext.mmx[5]);
    if(reg == MM6) return (char*) & (regd->titcontext.mmx[6]);
    if(reg == MM7) return (char*) & (regd->titcontext.mmx[7]);

    if(reg == x87r0) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[0]);
    if(reg == x87r1) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[1]);
    if(reg == x87r2) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[2]);
    if(reg == x87r3) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[3]);
    if(reg == x87r4) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[4]);
    if(reg == x87r5) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[5]);
    if(reg == x87r6) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[6]);
    if(reg == x87r7) return (char*) & (regd->titcontext.x87fpu.x87FPURegister[7]);

    return (char*) & null_value;
}

void RegistersView::setRegisters(REGDUMP* reg)
{
    // tests if new-register-value == old-register-value holds
    if(mCip != reg->titcontext.cip) //CIP changed
    {
        wCipRegDumpStruct = wRegDumpStruct;
        mRegisterUpdates.clear();
        mCip = reg->titcontext.cip;
    }

    QMap<REGISTER_NAME, QString>::const_iterator it = mRegisterMapping.begin();
    // iterate all ids (CAX, CBX, ...)
    while(it != mRegisterMapping.end())
    {
        uint_t old_value = GetUintValue((REGISTER_NAME) it.key(), registerValue(reg, it.key()));
        uint_t new_value = GetUintValue((REGISTER_NAME) it.key(), registerValue(&wCipRegDumpStruct, it.key()));
        if(old_value != new_value)
            mRegisterUpdates.insert(it.key());
        else if(mRegisterUpdates.contains(it.key())) //registers are equal
            mRegisterUpdates.remove(it.key());
        it++;
    }

    // now we can save the values
    wRegDumpStruct = (*reg);

    if(mCip != reg->titcontext.cip)
        wCipRegDumpStruct = wRegDumpStruct;

    // force repaint
    emit refresh();

}
