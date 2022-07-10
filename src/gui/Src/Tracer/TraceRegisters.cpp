#include <QMouseEvent>
#include "TraceRegisters.h"
#include "Configuration.h"
#include "EditFloatRegister.h"
#include "StringUtil.h"
#include "MiscUtil.h"

TraceRegisters::TraceRegisters(QWidget* parent) : RegistersView(parent)
{
    wCM_CopySIMDRegister = setupAction(DIcon("copy"), tr("Copy floating point value"));
    connect(wCM_CopySIMDRegister, SIGNAL(triggered()), this, SLOT(onCopySIMDRegister()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));
}

void TraceRegisters::setRegisters(REGDUMP* registers)
{
    this->RegistersView::setRegisters(registers);
}

void TraceRegisters::setActive(bool isActive)
{
    this->isActive = isActive;
    this->RegistersView::setRegisters(&this->wRegDumpStruct);
}

void TraceRegisters::displayCustomContextMenuSlot(QPoint pos)
{
    if(!isActive)
        return;
    QMenu wMenu(this);
    setupSIMDModeMenu();

    if(mSelected != UNKNOWN)
    {
        wMenu.addAction(wCM_CopyToClipboard);
        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_CopyFloatingPointValueToClipboard);
        }
        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            wMenu.addAction(wCM_CopySIMDRegister);
        }
        if(mLABELDISPLAY.contains(mSelected))
        {
            QString symbol = getRegisterLabel(mSelected);
            if(symbol != "")
                wMenu.addAction(wCM_CopySymbolToClipboard);
        }
        wMenu.addAction(wCM_CopyAll);

        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            wMenu.addMenu(mSwitchSIMDDispMode);
        }

        if(mFPUMMX.contains(mSelected) || mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            if(mFpuMode != 0)
                wMenu.addAction(mDisplaySTX);
            if(mFpuMode != 1)
                wMenu.addAction(mDisplayx87rX);
            if(mFpuMode != 2)
                wMenu.addAction(mDisplayMMX);
        }

        wMenu.exec(this->mapToGlobal(pos));
    }
    else // Right-click on empty space
    {
        wMenu.addSeparator();
        wMenu.addAction(wCM_ChangeFPUView);
        wMenu.addAction(wCM_CopyAll);
        wMenu.addMenu(mSwitchSIMDDispMode);
        if(mFpuMode != 0)
            wMenu.addAction(mDisplaySTX);
        if(mFpuMode != 1)
            wMenu.addAction(mDisplayx87rX);
        if(mFpuMode != 2)
            wMenu.addAction(mDisplayMMX);
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
        showCopyFloatRegister(256, this, tr("View YMM register"), registerValue(&wRegDumpStruct, mSelected));
    else if(mFPUXMM.contains(mSelected))
        showCopyFloatRegister(128, this, tr("View XMM register"), registerValue(&wRegDumpStruct, mSelected));
    else if(mFPUMMX.contains(mSelected))
        showCopyFloatRegister(64, this, tr("View MMX register"), registerValue(&wRegDumpStruct, mSelected));
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
        DbgCmdExec(QString("disasm %1").arg(ToPtrString(wRegDumpStruct.regcontext.cip)));
    // double clicked on XMM register: open view XMM register dialog
    else if(mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected) || mFPUMMX.contains(mSelected))
        onCopySIMDRegister();
    // double clicked on GPR: nothing to do (copy?)
}
