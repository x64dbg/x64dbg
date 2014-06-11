#include "RegistersView.h"



RegistersView::RegistersView(QWidget * parent) : QAbstractScrollArea(parent)
{


    // precreate ContextMenu Actions
    wCM_Increment = new QAction(tr("Increment"),this);
    wCM_Increment->setShortcut(Qt::Key_Plus);
    wCM_Decrement = new QAction(tr("Decrement"),this);
    wCM_Decrement->setShortcut(Qt::Key_Minus);
    wCM_Zero = new QAction(tr("Zero"),this);
    wCM_Zero->setShortcut(Qt::Key_0);
    wCM_SetToOne = new QAction(tr("Set to 1"),this);
    wCM_SetToOne->setShortcut(Qt::Key_1);
    wCM_Modify = new QAction(tr("Modify Value"),this);
    wCM_Modify->setShortcut(Qt::Key_Enter);
    wCM_ToggleValue = new QAction(tr("Toggle"),this);
    wCM_ToggleValue->setShortcut(Qt::Key_Space);
    wCM_CopyToClipboard = new QAction(tr("Copy Value to Clipboard"),this);
    wCM_CopyToClipboard->setShortcut(QKeySequence::Copy);



    // general purposes register (we allow the user to modify the value)
    mGPR.insert(CAX); mGPR.insert(CBX); mGPR.insert(CCX); mGPR.insert(CDX);
    mGPR.insert(CBP); mGPR.insert(CSP);
    mGPR.insert(CSI); mGPR.insert(CDI);
    mGPR.insert(R8);  mGPR.insert(R9);  mGPR.insert(R10);
    mGPR.insert(R11); mGPR.insert(R12); mGPR.insert(R13);
    mGPR.insert(R14); mGPR.insert(R15);
    mGPR.insert(EFLAGS);

    // flags (we allow the user to toggle them)
    mFlags.insert(CF); mFlags.insert(PF); mFlags.insert(AF); mFlags.insert(ZF);
    mFlags.insert(SF); mFlags.insert(TF); mFlags.insert(IF); mFlags.insert(DF);
    mFlags.insert(OF);


    // create mapping from internal id to name
    mRegisterMapping.clear();
    mRegisterPlaces.clear();
    int offset = 0;

    /* Register_Position is a struct definition the position
     *
     * (line , start, labelwidth, valuesize )
     */
#ifdef _WIN64
    mRegisterMapping.insert(CAX,"RAX");         mRegisterPlace.insert(CAX,Register_Position(0,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBX,"RBX");         mRegisterPlace.insert(CBX,Register_Position(1,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CCX,"RCX");         mRegisterPlace.insert(CCX,Register_Position(2,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDX,"RDX");         mRegisterPlace.insert(CDX,Register_Position(3,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSI,"RSI");         mRegisterPlace.insert(CSI,Register_Position(6,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDI,"RDI");         mRegisterPlace.insert(CDI,Register_Position(7,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBP,"RBP");         mRegisterPlace.insert(CBP,Register_Position(4,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSP,"RSP");         mRegisterPlace.insert(CSP,Register_Position(5,0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(R8,"R8");           mRegisterPlace.insert(R8 ,Register_Position(9,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R9,"R9");           mRegisterPlace.insert(R9 ,Register_Position(10,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R10,"R10");         mRegisterPlace.insert(R10,Register_Position(11,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R11,"R11");         mRegisterPlace.insert(R11,Register_Position(12,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R12,"R12");         mRegisterPlace.insert(R12,Register_Position(13,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R13,"R13");         mRegisterPlace.insert(R13,Register_Position(14,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R14,"R14");         mRegisterPlace.insert(R14,Register_Position(15,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(R15,"R15");         mRegisterPlace.insert(R15,Register_Position(16,0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(CIP,"RIP");         mRegisterPlace.insert(CIP,Register_Position(18,0, 6, sizeof(uint_t) * 2));

    mRegisterMapping.insert(EFLAGS,"RFLAGS");   mRegisterPlace.insert(EFLAGS,Register_Position(20,0, 9, sizeof(uint_t) * 2));

    offset = 21;
#else
    mRegisterMapping.insert(CAX,"EAX");         mRegisterPlaces.insert(CAX,Register_Position(0,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBX,"EBX");         mRegisterPlaces.insert(CBX,Register_Position(1,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CCX,"ECX");         mRegisterPlaces.insert(CCX,Register_Position(2,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDX,"EDX");         mRegisterPlaces.insert(CDX,Register_Position(3,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSI,"ESI");         mRegisterPlaces.insert(CSI,Register_Position(6,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CDI,"EDI");         mRegisterPlaces.insert(CDI,Register_Position(7,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CBP,"EBP");         mRegisterPlaces.insert(CBP,Register_Position(4,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CSP,"ESP");         mRegisterPlaces.insert(CSP,Register_Position(5,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(CIP,"EIP");         mRegisterPlaces.insert(CIP,Register_Position(9,0, 6, sizeof(uint_t) * 2));
    mRegisterMapping.insert(EFLAGS,"EFLAGS");   mRegisterPlaces.insert(EFLAGS,Register_Position(11,0, 9, sizeof(uint_t) * 2));

    offset = 12;
#endif
    mRegisterMapping.insert(CF,"CF");           mRegisterPlaces.insert(CF,Register_Position(offset+0,0, 3, 1));
    mRegisterMapping.insert(PF,"PF");           mRegisterPlaces.insert(PF,Register_Position(offset+1,0, 3, 1));
    mRegisterMapping.insert(AF,"AF");           mRegisterPlaces.insert(AF,Register_Position(offset+2,0, 3, 1));
    mRegisterMapping.insert(ZF,"ZF");           mRegisterPlaces.insert(ZF,Register_Position(offset+3,0, 3, 1));
    mRegisterMapping.insert(SF,"SF");           mRegisterPlaces.insert(SF,Register_Position(offset+4,0, 3, 1));

    mRegisterMapping.insert(TF,"TF");           mRegisterPlaces.insert(TF,Register_Position(offset+0, 10, 3,1));
    mRegisterMapping.insert(IF,"IF");           mRegisterPlaces.insert(IF,Register_Position(offset+1, 10, 3,1));
    mRegisterMapping.insert(DF,"DF");           mRegisterPlaces.insert(DF,Register_Position(offset+2, 10, 3,1));
    mRegisterMapping.insert(OF,"OF");           mRegisterPlaces.insert(OF,Register_Position(offset+3, 10, 3,1));

    offset++;
    mRegisterMapping.insert(GS,"GS");           mRegisterPlaces.insert(GS,Register_Position(offset+5,0, 3, 4));
    mRegisterMapping.insert(FS,"FS");           mRegisterPlaces.insert(FS,Register_Position(offset+6,0, 3, 4));
    mRegisterMapping.insert(ES,"ES");           mRegisterPlaces.insert(ES,Register_Position(offset+7,0, 3, 4));
    mRegisterMapping.insert(DS,"DS");           mRegisterPlaces.insert(DS,Register_Position(offset+8,0, 3, 4));
    mRegisterMapping.insert(CS,"CS");           mRegisterPlaces.insert(CS,Register_Position(offset+9,0, 3, 4));
    mRegisterMapping.insert(SS,"SS");           mRegisterPlaces.insert(SS,Register_Position(offset+10,0, 3, 4));

    offset++;
    mRegisterMapping.insert(DR0,"DR0");         mRegisterPlaces.insert(DR0,Register_Position(offset+11,0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR1,"DR1");         mRegisterPlaces.insert(DR1,Register_Position(offset+12,0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR2,"DR2");         mRegisterPlaces.insert(DR2,Register_Position(offset+13,0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR3,"DR3");         mRegisterPlaces.insert(DR3,Register_Position(offset+14,0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR6,"DR6");         mRegisterPlaces.insert(DR6,Register_Position(offset+15,0, 4, sizeof(uint_t) * 2));
    mRegisterMapping.insert(DR7,"DR7");         mRegisterPlaces.insert(DR7,Register_Position(offset+16,0, 4, sizeof(uint_t) * 2));


    // Set background color
    QPalette wPalette;
    wPalette.setColor(QPalette::Window, QColor("#FFFBF0"));
    this->setAutoFillBackground(true);
    this->setPalette(wPalette);

    QFont font("Monospace", 8);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    QAbstractScrollArea::setFont(font);

    int wRowsHeight = QFontMetrics(this->font()).height();
    wRowsHeight = (wRowsHeight * 105) / 100;
    wRowsHeight = (wRowsHeight % 2) == 0 ? wRowsHeight : wRowsHeight + 1;
    mRowHeight = wRowsHeight;
    mCharWidth = QFontMetrics(this->font()).averageCharWidth();

    memset(&wRegDumpStruct, 0, sizeof(REGDUMP));

    // Context Menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    // foreign messages
    connect(Bridge::getBridge(), SIGNAL(updateRegisters()), this, SLOT(updateRegistersSlot()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    // self communication for repainting (maybe some other widgets needs this information, too)
    connect(this,SIGNAL(refresh()),this,SLOT(repaint()));
    // context menu actions
    connect(wCM_Increment,SIGNAL(triggered()),this,SLOT(onIncrementAction()));
    connect(wCM_Decrement,SIGNAL(triggered()),this,SLOT(onDecrementAction()));
    connect(wCM_Zero,SIGNAL(triggered()),this,SLOT(onZeroAction()));
    connect(wCM_SetToOne,SIGNAL(triggered()),this,SLOT(onSetToOneAction()));
    connect(wCM_Modify,SIGNAL(triggered()),this,SLOT(onModifyAction()));
    connect(wCM_ToggleValue,SIGNAL(triggered()),this,SLOT(onToggleValueAction()));
    connect(wCM_CopyToClipboard,SIGNAL(triggered()),this,SLOT(onCopyToClipboardAction()));


}

RegistersView::~RegistersView()
{

}

/**
 * @brief retrieves the register id from given corrdinates of the viewport
 * @param line
 * @param offset (x-coord)
 * @param resulting register-id
 * @return true if register found
 */
bool RegistersView::identifyRegister(const int line, const int offset, REGISTER_NAME *clickedReg){
    // we start by an unknown register id
    *clickedReg = UNKNOWN;
    bool found_flag = false;
    QMap<REGISTER_NAME,Register_Position>::const_iterator it = mRegisterPlaces.begin();
    // iterate all registers that being displayed
    while (it != mRegisterPlaces.end()) {
        if( (it.value().line == line)   /* same line ? */
                && ( (1 + it.value().start) <= offset)  /* between start ... ? */
                && ( offset<= (1+it.value().start+it.value().labelwidth+it.value().valuesize)) /* ... and end ? */
                ){
            // we found a matching register in the viewport
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
    // get mouse position
    const int y = event->y()/(double)mRowHeight;
    const int x = event->x()/(double)mCharWidth;

    REGISTER_NAME r;
    // do we find a corresponding register?
    if(identifyRegister(y,x,&r)){
        mSelected = r;
        emit refresh();
    }

}



void RegistersView::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    // is current register general purposes register ?
    if(mGPR.contains(mSelected)){
        wCM_Modify->trigger();
    }else if(mFlags.contains(mSelected)) // is flag ?
        wCM_ToggleValue->trigger();

}

void RegistersView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter wPainter(this->viewport());

    QMap<REGISTER_NAME,QString>::const_iterator it = mRegisterMapping.begin();
    // iterate all registers
    while (it != mRegisterMapping.end()) {
        // paint register at given position
        drawRegister(&wPainter,it.key(),registerValue(&wRegDumpStruct,it.key()));
        it++;
    }

}

void RegistersView::keyPressEvent(QKeyEvent *event)
{
    if(event->matches(QKeySequence::Copy)){
        wCM_CopyToClipboard->trigger();
        return;
    }


    switch(event->key()) {
    case Qt::Key_0:
        wCM_Zero->trigger();
        break;
    case Qt::Key_1:
        wCM_SetToOne->trigger();
        break;
    case Qt::Key_Plus:
        wCM_Increment->trigger();
        break;
    case Qt::Key_Minus:
        wCM_Decrement->trigger();
        break;
    case Qt::Key_Space:
        wCM_ToggleValue->trigger();
        break;
    case Qt::Key_Enter:
        wCM_Modify->trigger();
        break;

    default:
        break;
    }
}

QSize RegistersView::sizeHint() const
{
    // 32 character width
    return QSize(32*mCharWidth ,this->viewport()->height());
}

void RegistersView::drawRegister(QPainter *p,REGISTER_NAME reg, uint_t value){
    // is the register-id known?
    if(mRegisterMapping.contains(reg)){
        // padding to the left is at least one character (looks better)
        int offset = mCharWidth*(1 + mRegisterPlaces[reg].start);
        // draw name of value
        p->drawText(offset,mRowHeight*(mRegisterPlaces[reg].line+1),mRegisterMapping[reg]);
        p->save();
        if(mRegisterUpdates.contains(reg))
            p->setPen(Qt::red);
        else
            p->setPen(Qt::black);

        if(mSelected == reg){
            p->fillRect(QRect(offset + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line)+2, mRegisterPlaces[reg].valuesize*mCharWidth, mRowHeight), QBrush(QColor("#eee")));
        }
        // draw value
        p->drawText(offset + (mRegisterPlaces[reg].labelwidth)*mCharWidth ,mRowHeight*(mRegisterPlaces[reg].line+1),QString("%1").arg(value, mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper());

        // do we have a label ?
        char label_text[MAX_LABEL_SIZE]="";
        char module_text[MAX_MODULE_SIZE]="";
        bool hasLabel=DbgGetLabelAt(value, SEG_DEFAULT, label_text);
        bool hasModule=DbgGetModuleAt(value, module_text);
        bool isASCII=false;

        offset += mRegisterPlaces[reg].labelwidth*mCharWidth + QString("%1").arg(value, mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper().length()*mCharWidth;
        offset += 5*mCharWidth;
        QString newText = "";
        if(hasLabel && hasModule){
            newText="<"+QString(module_text)+"."+QString(label_text)+">";
        }else if(hasModule){
            newText=QString(module_text)+"."+QString("%1").arg(value, mRegisterPlaces[reg].valuesize, 16, QChar('0')).toUpper();
        }else if(hasLabel ){
            newText="<"+QString(label_text)+">";
        }else{
            // can we interpret the character as ASCII ??
            if(mGPR.contains(reg)){
                if(value == (value & 0xFF)){
                    QChar c = QChar((char)value);
                    if(c.isPrint()){
                        newText=QString("'%1'").arg((char)value);
                        isASCII=true;
                    }
                }
            }
        }
        // are there additional informations?
        if(hasLabel || hasModule || isASCII)
            p->drawText(offset,mRowHeight*(mRegisterPlaces[reg].line+1),newText);

        p->restore();
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
    WordEditDialog wEditDial(this);
    QString wReg = registerValue(&wRegDumpStruct,mSelected);

#ifdef _WIN64
    wEditDial.setup(QString("Edit"),registerValue(&wRegDumpStruct,mSelected), 8);
#else
    wEditDial.setup(QString("Edit"), registerValue(&wRegDumpStruct,mSelected), 4);
#endif

    if(wEditDial.exec() == QDialog::Accepted) //OK button clicked
        setRegister(mSelected, wEditDial.getVal());

}

void RegistersView::onIncrementAction()
{
    if(mGPR.contains(mSelected))
        setRegister(mSelected, registerValue(&wRegDumpStruct,mSelected) + 1);
}

void RegistersView::onDecrementAction()
{
    if(mGPR.contains(mSelected))
        setRegister(mSelected, registerValue(&wRegDumpStruct,mSelected) - 1);
}

void RegistersView::onZeroAction()
{
    setRegister(mSelected, 0);
}

void RegistersView::onSetToOneAction()
{
    setRegister(mSelected, 1);
}

void RegistersView::onModifyAction()
{
    if(mGPR.contains(mSelected))
        displayEditDialog();
}

void RegistersView::onToggleValueAction()
{
    if(mFlags.contains(mSelected))
        setRegister(mSelected, ((int)registerValue(&wRegDumpStruct,mSelected))^1);
    else{
        int_t val = registerValue(&wRegDumpStruct,mSelected);
        val++;
        val *= -1;
        setRegister(mSelected,val);
    }

}

void RegistersView::onCopyToClipboardAction()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(QString("%1").arg(registerValue(&wRegDumpStruct,mSelected), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper());
}

void RegistersView::displayCustomContextMenuSlot(QPoint pos)
{
    QMenu wMenu(this);

    if(mSelected != UNKNOWN)
    {
        if(registerValue(&wRegDumpStruct,mSelected) >= 1)
            wMenu.addAction(wCM_Zero);
        if(registerValue(&wRegDumpStruct,mSelected) == 0)
            wMenu.addAction(wCM_SetToOne);
        wMenu.addAction(wCM_ToggleValue);

        if(mGPR.contains(mSelected))
        {
            wMenu.addAction(wCM_Modify);
            wMenu.addAction(wCM_Increment);
            wMenu.addAction(wCM_Decrement);

        }
        wMenu.addAction(wCM_CopyToClipboard);
        wMenu.exec(this->mapToGlobal(pos));
    }
    else if(DbgIsDebugging())
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
    if(mRegisterMapping.contains(reg)){
        // map "cax" to "eax" or "rax"
        QString wRegName = mRegisterMapping.constFind(reg).value();

        // flags need to '!' infront
        if(mFlags.contains(reg))
            wRegName = "!"+wRegName;


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
    if(state==stopped)
    {
        updateRegistersSlot();
    }
}

void RegistersView::repaint()
{
    this->viewport()->repaint();
}


int_t RegistersView::registerValue(const REGDUMP* regd,const REGISTER_NAME reg){
    // this is probably the most efficient general method to access the values of the struct

    if(reg==CAX) return regd->cax;
    if(reg==CBX) return regd->cbx;
    if(reg==CCX) return regd->ccx;
    if(reg==CDX) return regd->cdx;
    if(reg==CSI) return regd->csi;
    if(reg==CDI) return regd->cdi;
    if(reg==CBP) return regd->cbp;
    if(reg==CSP) return regd->csp;

    if(reg==CIP) return regd->cip;
    if(reg==EFLAGS) return regd->eflags;
#ifdef _WIN64
    if(reg==R8) return regd->r8;
    if(reg==R9) return regd->r9;
    if(reg==R10) return regd->r10;
    if(reg==R11) return regd->r11;
    if(reg==R12) return regd->r12;
    if(reg==R13) return regd->r13;
    if(reg==R14) return regd->r14;
    if(reg==R15) return regd->r15;
#endif
    // CF,PF,AF,ZF,SF,TF,IF,DF,OF
    if(reg==CF) return regd->flags.c;
    if(reg==PF) return regd->flags.p;
    if(reg==AF) return regd->flags.a;
    if(reg==ZF) return regd->flags.z;
    if(reg==SF) return regd->flags.s;
    if(reg==TF) return regd->flags.t;
    if(reg==IF) return regd->flags.i;
    if(reg==DF) return regd->flags.d;
    if(reg==OF) return regd->flags.o;

    // GS,FS,ES,DS,CS,SS
    if(reg==GS) return regd->gs;
    if(reg==FS) return regd->fs;
    if(reg==ES) return regd->es;
    if(reg==DS) return regd->ds;
    if(reg==CS) return regd->cs;
    if(reg==SS) return regd->ss;

    if(reg==DR0) return regd->dr0;
    if(reg==DR1) return regd->dr1;
    if(reg==DR2) return regd->dr2;
    if(reg==DR3) return regd->dr3;
    if(reg==DR6) return regd->dr6;
    if(reg==DR7) return regd->dr7;

    return 0;
}

void RegistersView::setRegisters(REGDUMP* reg)
{
    // tests if new-register-value == old-register-value holds
    mRegisterUpdates.clear();

    QMap<REGISTER_NAME,QString>::const_iterator it = mRegisterMapping.begin();
    // iterate all ids (CAX, CBX, ...)
    while (it != mRegisterMapping.end()) {
        // does a register-value change happens?
        if(registerValue(reg,it.key()) != registerValue(&wRegDumpStruct,it.key()))
            mRegisterUpdates.insert(it.key());
        it++;
    }

    // now we can save the values
    wRegDumpStruct = (*reg);

    // force repaint
    emit refresh();

}
