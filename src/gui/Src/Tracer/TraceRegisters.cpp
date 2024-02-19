#include <QMouseEvent>
#include "TraceRegisters.h"
#include "Configuration.h"
#include "EditFloatRegister.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include <QDebug>

TraceRegisters::TraceRegisters(QWidget* parent) : RegistersView(parent)
{
    wCM_CopySIMDRegister = setupAction(DIcon("copy"), tr("Copy floating point value"));
    connect(wCM_CopySIMDRegister, SIGNAL(triggered()), this, SLOT(onCopySIMDRegister()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));

    wCM_SetRegister = setupAction(tr("Set Register value"));
    connect(wCM_SetRegister, SIGNAL(triggered()), this, SLOT(onSetRegister()));
}

void TraceRegisters::setRegisters(REGDUMP* registers)
{
    this->RegistersView::setRegisters(registers);
}

void TraceRegisters::setActive(bool isActive)
{
    this->isActive = isActive;
    this->RegistersView::setRegisters(&this->mRegDumpStruct);
}

void TraceRegisters::displayCustomContextMenuSlot(QPoint pos)
{
    if(!isActive)
        return;
    QMenu menu(this);
    setupSIMDModeMenu();

    if(mSelected != UNKNOWN)
    {
        menu.addAction(wCM_CopyToClipboard);
        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            menu.addAction(wCM_CopyFloatingPointValueToClipboard);
        }
        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            menu.addAction(wCM_CopySIMDRegister);
        }
        if(mLABELDISPLAY.contains(mSelected))
        {
            QString symbol = getRegisterLabel(mSelected);
            if(symbol != "")
                menu.addAction(wCM_CopySymbolToClipboard);
        }
        menu.addAction(wCM_CopyAll);

        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            menu.addMenu(mSwitchSIMDDispMode);
        }

        if(mFPUMMX.contains(mSelected) || mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            if(mFpuMode != 0)
                menu.addAction(mDisplaySTX);
            if(mFpuMode != 1)
                menu.addAction(mDisplayx87rX);
            if(mFpuMode != 2)
                menu.addAction(mDisplayMMX);
        }

        if((!mNoChange.contains(mSelected)) ||
                mSelected == LastError ||
                mSelected == LastStatus ||
                mSelected == CIP)
        {
            menu.addAction(wCM_SetRegister);
        }

        menu.exec(this->mapToGlobal(pos));
    }
    else // Right-click on empty space
    {
        menu.addSeparator();
        menu.addAction(wCM_ChangeFPUView);
        menu.addAction(wCM_CopyAll);
        menu.addMenu(mSwitchSIMDDispMode);
        if(mFpuMode != 0)
            menu.addAction(mDisplaySTX);
        if(mFpuMode != 1)
            menu.addAction(mDisplayx87rX);
        if(mFpuMode != 2)
            menu.addAction(mDisplayMMX);
    }
}

static void showCopyFloatRegister(int bits, QWidget* parent, const QString & title, char* registerData)
{
    EditFloatRegister mEditFloat(bits, parent);
    mEditFloat.setWindowTitle(title);
    mEditFloat.loadData(registerData);
    mEditFloat.show();
    mEditFloat.selectAllText();
    mEditFloat.exec();
}

void TraceRegisters::onCopySIMDRegister()
{
    if(mFPUYMM.contains(mSelected))
        showCopyFloatRegister(256, this, tr("View YMM register"), registerValue(&mRegDumpStruct, mSelected));
    else if(mFPUXMM.contains(mSelected))
        showCopyFloatRegister(128, this, tr("View XMM register"), registerValue(&mRegDumpStruct, mSelected));
    else if(mFPUMMX.contains(mSelected))
        showCopyFloatRegister(64, this, tr("View MMX register"), registerValue(&mRegDumpStruct, mSelected));
}

void TraceRegisters::onSetRegister()
{
    // map x87st0 to x87r0
    REGISTER_NAME reg = mSelected;
    QString regName;
    duint value = *((duint*)registerValue(&mRegDumpStruct, mSelected));
    if(reg >= x87st0 && reg <= x87st7)
        regName = QString().sprintf("st%d", reg - x87st0);
    else
        // map "cax" to "eax" or "rax"
        regName = mRegisterMapping.constFind(reg).value();

    // flags need to '_' infront
    if(mFlags.contains(reg))
    {
        regName = "_" + regName;
        value = (int)(* (bool*) registerValue(&mRegDumpStruct, mSelected));
    }


    // tell everything the compiler
    if(mFPU.contains(reg))
    {
        regName = "_" + regName;
        value = (duint)registerValue(&mRegDumpStruct, mSelected);
    }

    if(mTAGWORD.contains(reg) || reg == MxCsr_RC || reg == x87CW_RC || reg == x87CW_PC || reg == x87SW_TOP)
    {
        value = (* ((const unsigned short*)registerValue(&mRegDumpStruct, mSelected)));
    }

    // we change the value (so highlight it)
    //    mRegisterUpdates.insert(reg);

    qDebug() << "This is the value " << value;
    qDebug() << "This is the string " << regName.toUtf8().constData();

    DbgValToString(regName.toUtf8().constData(), value);

    // force repaint
    //    emit refresh();
}

void TraceRegisters::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(!isActive || event->button() != Qt::LeftButton)
        return;
    // get mouse position
    const int y = (event->y() - yTopSpacing) / (double)mRowHeight;
    const int x = event->x() / (double)mCharWidth;

    // do we find a corresponding register?
    if(!identifyRegister(y, x, 0))
        return;
    if(mSelected == CIP) //double clicked on CIP register: follow in disassembly
        DbgCmdExec(QString("disasm %1").arg(ToPtrString(mRegDumpStruct.regcontext.cip)));
    // double clicked on XMM register: open view XMM register dialog
    else if(mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected) || mFPUMMX.contains(mSelected))
        onCopySIMDRegister();
    // double clicked on GPR: nothing to do (copy?)
}
