#include <QMouseEvent>
#include "TraceRegisters.h"
#include "Configuration.h"
#include "EditFloatRegister.h"
#include "MiscUtil.h"

TraceRegisters::TraceRegisters(QWidget* parent) : RegistersView(parent)
{
    wCM_CopySSERegister = setupAction(DIcon("copy.png"), tr("Copy floating point value"));
    connect(wCM_CopySSERegister, SIGNAL(triggered()), this, SLOT(onCopySSERegister()));
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
            wMenu.addAction(wCM_CopySSERegister);
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
    else
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

void TraceRegisters::onCopySSERegister()
{
    if(mFPUYMM.contains(mSelected))
    {
        EditFloatRegister mEditFloat(256, this);
        mEditFloat.setWindowTitle(tr("View YMM register"));
        mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
        mEditFloat.show();
        mEditFloat.selectAllText();
        mEditFloat.exec();
    }
    else if(mFPUXMM.contains(mSelected))
    {
        EditFloatRegister mEditFloat(128, this);
        mEditFloat.setWindowTitle(tr("View XMM register"));
        mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
        mEditFloat.show();
        mEditFloat.selectAllText();
        mEditFloat.exec();
    }
    else if(mFPUMMX.contains(mSelected))
    {
        EditFloatRegister mEditFloat(64, this);
        mEditFloat.setWindowTitle(tr("View MMX register"));
        mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
        mEditFloat.show();
        mEditFloat.selectAllText();
        mEditFloat.exec();
    }
}
