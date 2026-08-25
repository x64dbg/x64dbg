#include <QMouseEvent>
#include "TraceRegisters.h"
#include "TraceWidget.h"
#include "TraceDump.h"
#include "TraceStack.h"
#include "TraceBrowser.h"
#include "Configuration.h"
#include "EditFloatRegister.h"
#include "StringUtil.h"
#include "MiscUtil.h"

TraceRegisters::TraceRegisters(TraceWidget* parent) : RegistersView(parent)
{
    mParent = parent;
    wCM_CopySIMDRegister = setupAction(DIcon("copy"), tr("Copy floating point value"));
    connect(wCM_CopySIMDRegister, SIGNAL(triggered()), this, SLOT(onCopySIMDRegister()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));

    wCM_SetCurrentRegister = setupAction(tr("Set as current value"));
    connect(wCM_SetCurrentRegister, SIGNAL(triggered()), this, SLOT(onSetCurrentRegister()));

    wCM_FollowInDump = new QAction(DIcon("dump"), tr("Follow in Dump"), this);
    connect(wCM_FollowInDump, SIGNAL(triggered()), this, SLOT(onFollowInDump()));

    wCM_FollowInStack = new QAction(DIcon("stack"), tr("Follow in Stack"), this);
    connect(wCM_FollowInStack, SIGNAL(triggered()), this, SLOT(onFollowInStack()));

    wCM_Highlight = setupAction(DIcon("highlight"), tr("Highlight"));
    connect(wCM_Highlight, SIGNAL(triggered()), this, SLOT(onHighlightSlot()));

    delete SIMDAlwaysShowAVX512; // Currently it doesn't support AVX512
    SIMDAlwaysShowAVX512 = nullptr;
    mXMMModeYMMOnly = true;

    wCM_Highlight->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_Highlight);
    wCM_FollowInDump->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_FollowInDump);
    wCM_FollowInStack->setShortcutContext(Qt::WidgetShortcut);
    this->addAction(wCM_FollowInStack);
    refreshShortcutsSlot();
}

void TraceRegisters::setRegisters(REGDUMP* registers, const REGDUMP* previousRegisters)
{
    // TODO: AVX512. TraceFileReader currently exposes REGDUMP, not REGDUMP_AVX512.
    // Prime the diff baseline so the base class compares against the previous trace row
    // instead of the previously displayed live-debugging state.
    mCipRegDumpStruct = expandContext(previousRegisters ? previousRegisters : registers);
    mCip = registers->regcontext.cip;
    this->RegistersView::setRegisters(registers);
}

void TraceRegisters::setActive(bool isActive)
{
    this->isActive = isActive;
    //this->RegistersView::setRegisters(&this->mRegDumpStruct);
    emit refresh();
}

void TraceRegisters::displayCustomContextMenuSlot(QPoint pos)
{
    if(!isActive)
        return;
    QMenu menu(this);
    setupSIMDModeMenu();

    if(mSelected != UNKNOWN)
    {
        if(mCANSTOREADDRESS.contains(mSelected))
        {
            menu.addAction(wCM_FollowInDump);
            if(mSelected == CSP || mSelected == CBP) // TODO: Proper support stack detection
                menu.addAction(wCM_FollowInStack);
        }
        menu.addAction(wCM_CopyToClipboard);
        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            menu.addAction(wCM_CopyFloatingPointValueToClipboard);
        }
        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected))
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
        if((mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS) || mSEGMENTREGISTER.contains(mSelected) || mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUOpmask.contains(mSelected))
        {
            menu.addAction(wCM_Highlight);
        }

        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected))
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

        if(((!mNoChange.contains(mSelected)) ||
                mSelected == LastError ||
                mSelected == LastStatus ||
                mSelected == CIP) && DbgIsDebugging())
        {
            menu.addAction(wCM_SetCurrentRegister);
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
        menu.exec(this->mapToGlobal(pos));
    }
}

void TraceRegisters::refreshShortcutsSlot()
{
    wCM_Highlight->setShortcut(ConfigShortcut("ActionHighlightingMode"));
    wCM_FollowInDump->setShortcut(ConfigShortcut("ActionFollowDwordQwordDump"));
    wCM_FollowInStack->setShortcut(ConfigShortcut("ActionFollowStack"));
    RegistersView::refreshShortcutsSlot();
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
    if(mFPUXMM.contains(mSelected))
        showCopyFloatRegister(GetSizeRegister(mSelected) * 8, this, tr("View XMM register"), registerValue(&mRegDumpStruct, mSelected));
    else if(mFPUMMX.contains(mSelected))
        showCopyFloatRegister(64, this, tr("View MMX register"), registerValue(&mRegDumpStruct, mSelected));
}

void TraceRegisters::onSetCurrentRegister()
{
    // map x87st0 to x87r0
    REGISTER_NAME reg = mSelected;
    QString regName;
    if(reg >= x87st0 && reg <= x87st7)
        regName = QString().sprintf("st%d", reg - x87st0);
    else
        // map "cax" to "eax" or "rax"
        regName = mRegisterMapping.constFind(reg).value();
    if(reg >= XMM0 && reg <= ArchValue(XMM7, XMM31))
    {
        switch(mXMMMode)
        {
        case 1:
            regName[0] = 'Y';
            break;
        case 2:
            regName[0] = 'Z';
            break;
        }
    }

    // flags and MFPU need to '_' infront
    if(mFlags.contains(reg) || mFPU.contains(reg))
        regName = "_" + regName;

    if(mFPUXMM.contains(reg) || mFPUMMX.contains(reg) || mFPUOpmask.contains(reg) || mFPUx87_80BITSDISPLAY.contains(reg))
    {
        DbgValSetBuffer(regName.toUtf8().constData(), registerValue(&mRegDumpStruct, mSelected), GetSizeRegister(mSelected));
        return;
    }

    duint value;
    if(mUINTDISPLAY.contains(reg))
        value = *((const duint*)registerValue(&mRegDumpStruct, mSelected));
    else if(mBOOLDISPLAY.contains(reg))
        value = (duint)(*(const bool*)registerValue(&mRegDumpStruct, mSelected));
    else if(mUSHORTDISPLAY.contains(reg) || mFIELDVALUE.contains(reg))
        value = (duint)(*(const unsigned short*)registerValue(&mRegDumpStruct, mSelected));
    else if(mDWORDDISPLAY.contains(reg))
        value = (duint)(*(const DWORD*)registerValue(&mRegDumpStruct, mSelected));
    else
        value = *((const duint*)registerValue(&mRegDumpStruct, mSelected));

    DbgValSetScalar(regName.toUtf8().constData(), value);

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
        DbgCmdExecAsync(QString("disasm %1").arg(ToPtrString(mRegDumpStruct.regcontext.cip)));
    // double clicked on XMM register: open view XMM register dialog
    else if(mFPUXMM.contains(mSelected) || mFPUMMX.contains(mSelected))
        onCopySIMDRegister();
    // double clicked on GPR: nothing to do (copy?)
}

void TraceRegisters::onFollowInDump()
{
    // First check if the dump is loaded
    if(!mParent->getTraceFile()->getDump()->isEnabled())
        if(!mParent->loadDump()) // Try to load the dump
            return;
    duint value = *((const duint*)registerValue(&mRegDumpStruct, mSelected));
    mParent->getTraceDump()->printDumpAt(value, true, true, true);
}

void TraceRegisters::onFollowInStack()
{
    // First check if the dump is loaded
    if(!mParent->getTraceFile()->getDump()->isEnabled())
        if(!mParent->loadDump())  // Try to load the dump
            return;
    duint value = *((const duint*)registerValue(&mRegDumpStruct, mSelected));
    mParent->getTraceStack()->printDumpAt(value, true, true, true);
}

void TraceRegisters::onHighlightSlot()
{
    TraceBrowser* CPUDisassemblyView = mParent->getTraceBrowser();
    if(mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS)
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::GeneralRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mSEGMENTREGISTER.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MemorySegment, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUMMX.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MmxRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUXMM.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::XmmRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUOpmask.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::ZmmRegister, mRegisterMapping.constFind(mSelected).value()));
    CPUDisassemblyView->reloadData();
}
