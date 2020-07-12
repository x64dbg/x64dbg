#include <QListWidget>
#include "MiscUtil.h"
#include "CPUWidget.h"
#include "CPUDisassembly.h"
#include "CPUMultiDump.h"
#include "Configuration.h"
#include "WordEditDialog.h"
#include "LineEditDialog.h"
#include "EditFloatRegister.h"
#include "SelectFields.h"
#include "CPURegistersView.h"
#include "ldconvert.h"

static QAction* setupAction(const QIcon & icon, const QString & text, RegistersView* this_object)
{
    QAction* action = new QAction(icon, text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    return action;
}

static QAction* setupAction(const QString & text, RegistersView* this_object)
{
    QAction* action = new QAction(text, this_object);
    action->setShortcutContext(Qt::WidgetShortcut);
    this_object->addAction(action);
    return action;
}

CPURegistersView::CPURegistersView(CPUWidget* parent) : RegistersView(parent), mParent(parent)
{
    // precreate ContextMenu Actions
    wCM_Increment = setupAction(DIcon("register_inc.png"), tr("Increment"), this);
    wCM_Decrement = setupAction(DIcon("register_dec.png"), tr("Decrement"), this);
    wCM_Zero = setupAction(DIcon("register_zero.png"), tr("Zero"), this);
    wCM_SetToOne = setupAction(DIcon("register_one.png"), tr("Set to 1"), this);
    wCM_Modify = new QAction(DIcon("register_edit.png"), tr("Modify value"), this);
    wCM_Modify->setShortcut(QKeySequence(Qt::Key_Enter));
    wCM_ToggleValue = setupAction(DIcon("register_toggle.png"), tr("Toggle"), this);
    wCM_Undo = setupAction(DIcon("undo.png"), tr("Undo"), this);
    wCM_CopyToClipboard = setupAction(DIcon("copy.png"), tr("Copy value"), this);
    wCM_CopyFloatingPointValueToClipboard = setupAction(DIcon("copy.png"), tr("Copy floating point value"), this);
    wCM_CopySymbolToClipboard = setupAction(DIcon("pdb.png"), tr("Copy Symbol Value"), this);
    wCM_CopyAll = setupAction(DIcon("copy-alt.png"), tr("Copy all registers"), this);
    wCM_FollowInDisassembly = new QAction(DIcon(QString("processor%1.png").arg(ArchValue("32", "64"))), tr("Follow in Disassembler"), this);
    wCM_FollowInDump = new QAction(DIcon("dump.png"), tr("Follow in Dump"), this);
    wCM_FollowInStack = new QAction(DIcon("stack.png"), tr("Follow in Stack"), this);
    wCM_FollowInMemoryMap = new QAction(DIcon("memmap_find_address_page"), tr("Follow in Memory Map"), this);
    wCM_Incrementx87Stack = setupAction(DIcon("arrow-small-down.png"), tr("Increment x87 Stack"), this);
    wCM_Decrementx87Stack = setupAction(DIcon("arrow-small-up.png"), tr("Decrement x87 Stack"), this);
    wCM_ChangeFPUView = new QAction(DIcon("change-view.png"), tr("Change view"), this);
    wCM_IncrementPtrSize = setupAction(DIcon("register_inc.png"), ArchValue(tr("Increase 4"), tr("Increase 8")), this);
    wCM_DecrementPtrSize = setupAction(DIcon("register_dec.png"), ArchValue(tr("Decrease 4"), tr("Decrease 8")), this);
    wCM_Push = setupAction(DIcon("arrow-small-down.png"), tr("Push"), this);
    wCM_Pop = setupAction(DIcon("arrow-small-up.png"), tr("Pop"), this);
    wCM_Highlight = setupAction(DIcon("highlight.png"), tr("Highlight"), this);
    mSwitchSIMDDispMode = new QMenu(tr("Change SIMD Register Display Mode"), this);
    mSwitchSIMDDispMode->setIcon(DIcon("simdmode.png"));
    mDisplaySTX = new QAction(tr("Display ST(x)"), this);
    mDisplayx87rX = new QAction(tr("Display x87rX"), this);
    mDisplayMMX = new QAction(tr("Display MMX"), this);
    SIMDHex = new QAction(tr("Hexadecimal"), mSwitchSIMDDispMode);
    SIMDFloat = new QAction(tr("Float"), mSwitchSIMDDispMode);
    SIMDDouble = new QAction(tr("Double"), mSwitchSIMDDispMode);
    SIMDSWord = new QAction(tr("Signed Word"), mSwitchSIMDDispMode);
    SIMDSDWord = new QAction(tr("Signed Dword"), mSwitchSIMDDispMode);
    SIMDSQWord = new QAction(tr("Signed Qword"), mSwitchSIMDDispMode);
    SIMDUWord = new QAction(tr("Unsigned Word"), mSwitchSIMDDispMode);
    SIMDUDWord = new QAction(tr("Unsigned Dword"), mSwitchSIMDDispMode);
    SIMDUQWord = new QAction(tr("Unsigned Qword"), mSwitchSIMDDispMode);
    SIMDHWord = new QAction(tr("Hexadecimal Word"), mSwitchSIMDDispMode);
    SIMDHDWord = new QAction(tr("Hexadecimal Dword"), mSwitchSIMDDispMode);
    SIMDHQWord = new QAction(tr("Hexadecimal Qword"), mSwitchSIMDDispMode);
    SIMDHex->setData(QVariant(0));
    SIMDFloat->setData(QVariant(1));
    SIMDDouble->setData(QVariant(2));
    SIMDSWord->setData(QVariant(3));
    SIMDUWord->setData(QVariant(6));
    SIMDHWord->setData(QVariant(9));
    SIMDSDWord->setData(QVariant(4));
    SIMDUDWord->setData(QVariant(7));
    SIMDHDWord->setData(QVariant(10));
    SIMDSQWord->setData(QVariant(5));
    SIMDUQWord->setData(QVariant(8));
    SIMDHQWord->setData(QVariant(11));
    mDisplaySTX->setData(QVariant(0));
    mDisplayx87rX->setData(QVariant(1));
    mDisplayMMX->setData(QVariant(2));
    connect(SIMDHex, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDFloat, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDDouble, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHDWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDSQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDUQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(SIMDHQWord, SIGNAL(triggered()), this, SLOT(onSIMDMode()));
    connect(mDisplaySTX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    connect(mDisplayx87rX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    connect(mDisplayMMX, SIGNAL(triggered()), this, SLOT(onFpuMode()));
    SIMDHex->setCheckable(true);
    SIMDFloat->setCheckable(true);
    SIMDDouble->setCheckable(true);
    SIMDSWord->setCheckable(true);
    SIMDUWord->setCheckable(true);
    SIMDHWord->setCheckable(true);
    SIMDSDWord->setCheckable(true);
    SIMDUDWord->setCheckable(true);
    SIMDHDWord->setCheckable(true);
    SIMDSQWord->setCheckable(true);
    SIMDUQWord->setCheckable(true);
    SIMDHQWord->setCheckable(true);
    SIMDHex->setChecked(true);
    SIMDFloat->setChecked(false);
    SIMDDouble->setChecked(false);
    SIMDSWord->setChecked(false);
    SIMDUWord->setChecked(false);
    SIMDHWord->setChecked(false);
    SIMDSDWord->setChecked(false);
    SIMDUDWord->setChecked(false);
    SIMDHDWord->setChecked(false);
    SIMDSQWord->setChecked(false);
    SIMDUQWord->setChecked(false);
    SIMDHQWord->setChecked(false);
    mSwitchSIMDDispMode->addAction(SIMDHex);
    mSwitchSIMDDispMode->addAction(SIMDFloat);
    mSwitchSIMDDispMode->addAction(SIMDDouble);
    mSwitchSIMDDispMode->addAction(SIMDSWord);
    mSwitchSIMDDispMode->addAction(SIMDSDWord);
    mSwitchSIMDDispMode->addAction(SIMDSQWord);
    mSwitchSIMDDispMode->addAction(SIMDUWord);
    mSwitchSIMDDispMode->addAction(SIMDUDWord);
    mSwitchSIMDDispMode->addAction(SIMDUQWord);
    mSwitchSIMDDispMode->addAction(SIMDHWord);
    mSwitchSIMDDispMode->addAction(SIMDHDWord);
    mSwitchSIMDDispMode->addAction(SIMDHQWord);
    // Context Menu
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    // foreign messages
    connect(Bridge::getBridge(), SIGNAL(updateRegisters()), this, SLOT(updateRegistersSlot()));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(displayCustomContextMenuSlot(QPoint)));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(debugStateChangedSlot(DBGSTATE)));
    connect(parent->getDisasmWidget(), SIGNAL(selectionChanged(dsint)), this, SLOT(disasmSelectionChangedSlot(dsint)));
    // context menu actions
    connect(wCM_Increment, SIGNAL(triggered()), this, SLOT(onIncrementAction()));
    connect(wCM_ChangeFPUView, SIGNAL(triggered()), this, SLOT(onChangeFPUViewAction()));
    connect(wCM_Decrement, SIGNAL(triggered()), this, SLOT(onDecrementAction()));
    connect(wCM_Incrementx87Stack, SIGNAL(triggered()), this, SLOT(onIncrementx87StackAction()));
    connect(wCM_Decrementx87Stack, SIGNAL(triggered()), this, SLOT(onDecrementx87StackAction()));
    connect(wCM_Zero, SIGNAL(triggered()), this, SLOT(onZeroAction()));
    connect(wCM_SetToOne, SIGNAL(triggered()), this, SLOT(onSetToOneAction()));
    connect(wCM_Modify, SIGNAL(triggered()), this, SLOT(onModifyAction()));
    connect(wCM_ToggleValue, SIGNAL(triggered()), this, SLOT(onToggleValueAction()));
    connect(wCM_Undo, SIGNAL(triggered()), this, SLOT(onUndoAction()));
    connect(wCM_CopyToClipboard, SIGNAL(triggered()), this, SLOT(onCopyToClipboardAction()));
    connect(wCM_CopyFloatingPointValueToClipboard, SIGNAL(triggered()), this, SLOT(onCopyFloatingPointToClipboardAction()));
    connect(wCM_CopySymbolToClipboard, SIGNAL(triggered()), this, SLOT(onCopySymbolToClipboardAction()));
    connect(wCM_CopyAll, SIGNAL(triggered()), this, SLOT(onCopyAllAction()));
    connect(wCM_FollowInDisassembly, SIGNAL(triggered()), this, SLOT(onFollowInDisassembly()));
    connect(wCM_FollowInDump, SIGNAL(triggered()), this, SLOT(onFollowInDump()));
    connect(wCM_FollowInStack, SIGNAL(triggered()), this, SLOT(onFollowInStack()));
    connect(wCM_FollowInMemoryMap, SIGNAL(triggered()), this, SLOT(onFollowInMemoryMap()));
    connect(wCM_IncrementPtrSize, SIGNAL(triggered()), this, SLOT(onIncrementPtrSize()));
    connect(wCM_DecrementPtrSize, SIGNAL(triggered()), this, SLOT(onDecrementPtrSize()));
    connect(wCM_Push, SIGNAL(triggered()), this, SLOT(onPushAction()));
    connect(wCM_Pop, SIGNAL(triggered()), this, SLOT(onPopAction()));
    connect(wCM_Highlight, SIGNAL(triggered()), this, SLOT(onHighlightSlot()));

    refreshShortcutsSlot();
    connect(Config(), SIGNAL(shortcutsUpdated()), this, SLOT(refreshShortcutsSlot()));
}

void CPURegistersView::refreshShortcutsSlot()
{
    wCM_Increment->setShortcut(ConfigShortcut("ActionIncreaseRegister"));
    wCM_Decrement->setShortcut(ConfigShortcut("ActionDecreaseRegister"));
    wCM_Zero->setShortcut(ConfigShortcut("ActionZeroRegister"));
    wCM_SetToOne->setShortcut(ConfigShortcut("ActionSetOneRegister"));
    wCM_ToggleValue->setShortcut(ConfigShortcut("ActionToggleRegisterValue"));
    wCM_CopyToClipboard->setShortcut(ConfigShortcut("ActionCopy"));
    wCM_CopySymbolToClipboard->setShortcut(ConfigShortcut("ActionCopySymbol"));
    wCM_CopyAll->setShortcut(ConfigShortcut("ActionCopyAllRegisters"));
    wCM_Highlight->setShortcut(ConfigShortcut("ActionHighlightingMode"));
    wCM_IncrementPtrSize->setShortcut(ConfigShortcut("ActionIncreaseRegisterPtrSize"));
    wCM_DecrementPtrSize->setShortcut(ConfigShortcut("ActionDecreaseRegisterPtrSize"));
    wCM_Incrementx87Stack->setShortcut(ConfigShortcut("ActionIncrementx87Stack"));
    wCM_Decrementx87Stack->setShortcut(ConfigShortcut("ActionDecrementx87Stack"));
    wCM_Push->setShortcut(ConfigShortcut("ActionPush"));
    wCM_Pop->setShortcut(ConfigShortcut("ActionPop"));
}

void CPURegistersView::mousePressEvent(QMouseEvent* event)
{
    if(!isActive)
        return;

    if(event->y() < yTopSpacing - mButtonHeight)
    {
        onChangeFPUViewAction();
    }
    else
    {
        // get mouse position
        const int y = (event->y() - yTopSpacing) / (double)mRowHeight;
        const int x = event->x() / (double)mCharWidth;

        REGISTER_NAME r;
        // do we find a corresponding register?
        if(identifyRegister(y, x, &r))
        {
            Disassembly* CPUDisassemblyView = mParent->getDisasmWidget();
            if(CPUDisassemblyView->isHighlightMode())
            {
                if(mGPR.contains(r) && r != REGISTER_NAME::EFLAGS)
                    CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::GeneralRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUMMX.contains(r))
                    CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MmxRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUXMM.contains(r))
                    CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::XmmRegister, mRegisterMapping.constFind(r).value()));
                else if(mFPUYMM.contains(r))
                    CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::YmmRegister, mRegisterMapping.constFind(r).value()));
                else if(mSEGMENTREGISTER.contains(r))
                    CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MemorySegment, mRegisterMapping.constFind(r).value()));
                else
                    mSelected = r;
            }
            else
                mSelected = r;
            emit refresh();
        }
        else
            mSelected = UNKNOWN;
    }
}

void CPURegistersView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if(!isActive || event->button() != Qt::LeftButton)
        return;
    // get mouse position
    const int y = (event->y() - yTopSpacing) / (double)mRowHeight;
    const int x = event->x() / (double)mCharWidth;

    // do we find a corresponding register?
    if(!identifyRegister(y, x, 0))
        return;
    if(mSelected == CIP) //double clicked on CIP register
        DbgCmdExec("disasm cip");
    // is current register general purposes register or FPU register?
    else if(mMODIFYDISPLAY.contains(mSelected))
        wCM_Modify->trigger();
    else if(mBOOLDISPLAY.contains(mSelected)) // is flag ?
        wCM_ToggleValue->trigger();
    else if(mCANSTOREADDRESS.contains(mSelected))
        wCM_FollowInDisassembly->trigger();
}

void CPURegistersView::keyPressEvent(QKeyEvent* event)
{
    if(isActive)
    {
        int key = event->key();
        REGISTER_NAME newRegister = UNKNOWN;

        switch(key)
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            wCM_Modify->trigger();
            break;
        case Qt::Key_Left:
            newRegister = mRegisterRelativePlaces[mSelected].left;
            break;
        case Qt::Key_Right:
            newRegister = mRegisterRelativePlaces[mSelected].right;
            break;
        case Qt::Key_Up:
            newRegister = mRegisterRelativePlaces[mSelected].up;
            break;
        case Qt::Key_Down:
            newRegister = mRegisterRelativePlaces[mSelected].down;
            break;
        }

        if(newRegister != UNKNOWN)
        {
            mSelected = newRegister;
            ensureRegisterVisible(newRegister);
            emit refresh();
        }
    }
    RegistersView::keyPressEvent(event);
}

void CPURegistersView::debugStateChangedSlot(DBGSTATE state)
{
    if(state == stopped)
    {
        updateRegistersSlot();
        isActive = false;
    }
    else
    {
        isActive = true;
    }
}

void CPURegistersView::updateRegistersSlot()
{
    // read registers
    REGDUMP z;
    DbgGetRegDumpEx(&z, sizeof(REGDUMP));
    // update gui
    setRegisters(&z);
}

void CPURegistersView::ModifyFields(const QString & title, STRING_VALUE_TABLE_t* table, SIZE_T size)
{
    SelectFields mSelectFields(this);
    QListWidget* mQListWidget = mSelectFields.GetList();

    QStringList items;
    unsigned int i;

    for(i = 0; i < size; i++)
        items << QApplication::translate("RegistersView_ConstantsOfRegisters", table[i].string) + QString(" (%1)").arg(table[i].value, 0, 16);

    mQListWidget->addItems(items);

    mSelectFields.setWindowTitle(title);
    if(mSelectFields.exec() != QDialog::Accepted)
        return;

    if(mQListWidget->selectedItems().count() != 1)
        return;

    //QListWidgetItem* item = mQListWidget->takeItem(mQListWidget->currentRow());
    QString itemText = mQListWidget->item(mQListWidget->currentRow())->text();

    duint value;

    for(i = 0; i < size; i++)
    {
        if(QApplication::translate("RegistersView_ConstantsOfRegisters", table[i].string) + QString(" (%1)").arg(table[i].value, 0, 16) == itemText)
            break;
    }

    value = table[i].value;

    setRegister(mSelected, (duint)value);
    //delete item;
}

extern STRING_VALUE_TABLE_t MxCsrRCValueStringTable[4];
extern STRING_VALUE_TABLE_t ControlWordRCValueStringTable[4];
extern STRING_VALUE_TABLE_t StatusWordTOPValueStringTable[8];
extern STRING_VALUE_TABLE_t ControlWordPCValueStringTable[4];
extern STRING_VALUE_TABLE_t TagWordValueStringTable[4];

#define MODIFY_FIELDS_DISPLAY(prefix, title, table) ModifyFields(prefix + QChar(' ') + QString(title), (STRING_VALUE_TABLE_t *) & table, SIZE_TABLE(table) )

/**
 * @brief   This function displays the appropriate edit dialog according to selected register
 * @return  Nothing.
 */

void CPURegistersView::displayEditDialog()
{
    if(mFPU.contains(mSelected))
    {
        if(mTAGWORD.contains(mSelected))
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "Tag " + mRegisterMapping.constFind(mSelected).value(), TagWordValueStringTable);
        else if(mSelected == MxCsr_RC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "MxCsr_RC", MxCsrRCValueStringTable);
        else if(mSelected == x87CW_RC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87CW_RC", ControlWordRCValueStringTable);
        else if(mSelected == x87CW_PC)
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87CW_PC", ControlWordPCValueStringTable);
        else if(mSelected == x87SW_TOP)
        {
            MODIFY_FIELDS_DISPLAY(tr("Edit"), "x87SW_TOP", StatusWordTOPValueStringTable);
            // if(mFpuMode == false)
            updateRegistersSlot();
        }
        else if(mFPUYMM.contains(mSelected))
        {
            EditFloatRegister mEditFloat(256, this);
            mEditFloat.setWindowTitle(tr("Edit YMM register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else if(mFPUXMM.contains(mSelected))
        {
            EditFloatRegister mEditFloat(128, this);
            mEditFloat.setWindowTitle(tr("Edit XMM register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else if(mFPUMMX.contains(mSelected))
        {
            EditFloatRegister mEditFloat(64, this);
            mEditFloat.setWindowTitle(tr("Edit MMX register"));
            mEditFloat.loadData(registerValue(&wRegDumpStruct, mSelected));
            mEditFloat.show();
            mEditFloat.selectAllText();
            if(mEditFloat.exec() == QDialog::Accepted)
                setRegister(mSelected, (duint)mEditFloat.getData());
        }
        else
        {
            bool errorinput = false;
            LineEditDialog mLineEdit(this);

            mLineEdit.setText(GetRegStringValueFromValue(mSelected,  registerValue(&wRegDumpStruct, mSelected)));
            mLineEdit.setWindowTitle(tr("Edit FPU register"));
            mLineEdit.setWindowIcon(DIcon("log.png"));
            mLineEdit.setCursorPosition(0);
            auto sizeRegister = int(GetSizeRegister(mSelected));
            if(sizeRegister == 10)
                mLineEdit.setFpuMode();
            mLineEdit.ForceSize(sizeRegister * 2);
            do
            {
                errorinput = false;
                mLineEdit.show();
                mLineEdit.selectAllText();
                if(mLineEdit.exec() != QDialog::Accepted)
                    return; //pressed cancel
                else
                {
                    bool ok = false;
                    duint fpuvalue;

                    if(mUSHORTDISPLAY.contains(mSelected))
                        fpuvalue = (duint) mLineEdit.editText.toUShort(&ok, 16);
                    else if(mDWORDDISPLAY.contains(mSelected))
                        fpuvalue = mLineEdit.editText.toUInt(&ok, 16);
                    else if(mFPUx87_80BITSDISPLAY.contains(mSelected))
                    {
                        QString editTextLower = mLineEdit.editText.toLower();
                        if(sizeRegister == 10 && (mLineEdit.editText.contains(QChar('.')) || editTextLower == "nan" || editTextLower == "inf"
                                                  || editTextLower == "+inf" || editTextLower == "-inf"))
                        {
                            char number[10];
                            str2ld(mLineEdit.editText.toUtf8().constData(), number);
                            setRegister(mSelected, reinterpret_cast<duint>(number));
                            return;
                        }
                        else
                        {
                            QByteArray pArray =  mLineEdit.editText.toLocal8Bit();

                            if(pArray.size() == sizeRegister * 2)
                            {
                                char* pData = (char*) calloc(1, sizeof(char) * sizeRegister);

                                if(pData != NULL)
                                {
                                    ok = true;
                                    char actual_char[3];
                                    for(int i = 0; i < sizeRegister; i++)
                                    {
                                        memset(actual_char, 0, sizeof(actual_char));
                                        memcpy(actual_char, (char*) pArray.data() + (i * 2), 2);
                                        if(! isxdigit(actual_char[0]) || ! isxdigit(actual_char[1]))
                                        {
                                            ok = false;
                                            break;
                                        }
                                        pData[i] = (char)strtol(actual_char, NULL, 16);
                                    }

                                    if(ok)
                                    {
                                        if(!ConfigBool("Gui", "FpuRegistersLittleEndian")) // reverse byte order if it is big-endian
                                        {
                                            pArray = ByteReverse(QByteArray(pData, sizeRegister));
                                            setRegister(mSelected, reinterpret_cast<duint>(pArray.constData()));
                                        }
                                        else
                                            setRegister(mSelected, reinterpret_cast<duint>(pData));
                                    }

                                    free(pData);

                                    if(ok)
                                        return;
                                }
                            }
                        }
                    }
                    if(!ok)
                    {
                        errorinput = true;

                        SimpleWarningBox(this, tr("ERROR CONVERTING TO HEX"), tr("ERROR CONVERTING TO HEX"));
                    }
                    else
                        setRegister(mSelected, fpuvalue);
                }
            }
            while(errorinput);
        }
    }
    else if(mSelected == LastError)
    {
        bool errorinput = false;
        LineEditDialog mLineEdit(this);
        LASTERROR* error = (LASTERROR*)registerValue(&wRegDumpStruct, LastError);
        mLineEdit.setText(QString::number(error->code, 16));
        mLineEdit.setWindowTitle(tr("Set Last Error"));
        mLineEdit.setCursorPosition(0);
        do
        {
            errorinput = true;
            mLineEdit.show();
            mLineEdit.selectAllText();
            if(mLineEdit.exec() != QDialog::Accepted)
                return;
            if(DbgIsValidExpression(mLineEdit.editText.toUtf8().constData()))
                errorinput = false;
        }
        while(errorinput);
        setRegister(LastError, DbgValFromString(mLineEdit.editText.toUtf8().constData()));
    }
    else if(mSelected == LastStatus)
    {
        bool statusinput = false;
        LineEditDialog mLineEdit(this);
        LASTSTATUS* status = (LASTSTATUS*)registerValue(&wRegDumpStruct, LastStatus);
        mLineEdit.setText(QString::number(status->code, 16));
        mLineEdit.setWindowTitle(tr("Set Last Status"));
        mLineEdit.setCursorPosition(0);
        do
        {
            statusinput = true;
            mLineEdit.show();
            mLineEdit.selectAllText();
            if(mLineEdit.exec() != QDialog::Accepted)
                return;
            if(DbgIsValidExpression(mLineEdit.editText.toUtf8().constData()))
                statusinput = false;
        }
        while(statusinput);
        setRegister(LastStatus, DbgValFromString(mLineEdit.editText.toUtf8().constData()));
    }
    else
    {
        WordEditDialog wEditDial(this);
        wEditDial.setup(tr("Edit"), (* ((duint*) registerValue(&wRegDumpStruct, mSelected))), sizeof(dsint));
        if(wEditDial.exec() == QDialog::Accepted) //OK button clicked
            setRegister(mSelected, wEditDial.getVal());
    }
}

void CPURegistersView::CreateDumpNMenu(QMenu* dumpMenu)
{
    QList<QString> names;
    CPUMultiDump* multiDump = mParent->getDumpWidget();
    dumpMenu->setIcon(DIcon("dump.png"));
    int maxDumps = multiDump->getMaxCPUTabs();
    multiDump->getTabNames(names);
    for(int i = 0; i < maxDumps; i++)
    {
        QAction* action = new QAction(names.at(i), this);
        connect(action, SIGNAL(triggered()), this, SLOT(onFollowInDumpN()));
        dumpMenu->addAction(action);
        action->setData(i + 1);
    }
}

void CPURegistersView::onIncrementx87StackAction()
{
    if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        setRegister(x87SW_TOP, ((* ((duint*) registerValue(&wRegDumpStruct, x87SW_TOP))) + 1) % 8);
}

void CPURegistersView::onDecrementx87StackAction()
{
    if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        setRegister(x87SW_TOP, ((* ((duint*) registerValue(&wRegDumpStruct, x87SW_TOP))) - 1) % 8);
}

void CPURegistersView::onIncrementAction()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) + 1);
}

void CPURegistersView::onDecrementAction()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) - 1);
}

void CPURegistersView::onIncrementPtrSize()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) + sizeof(void*));
}

void CPURegistersView::onDecrementPtrSize()
{
    if(mINCREMENTDECREMET.contains(mSelected))
        setRegister(mSelected, (* ((duint*) registerValue(&wRegDumpStruct, mSelected))) - sizeof(void*));
}

void CPURegistersView::onPushAction()
{
    duint csp = (* ((duint*) registerValue(&wRegDumpStruct, CSP))) - sizeof(void*);
    duint regVal = 0;
    regVal = * ((duint*) registerValue(&wRegDumpStruct, mSelected));
    setRegister(CSP, csp);
    DbgMemWrite(csp, (const unsigned char*)&regVal, sizeof(void*));
}

void CPURegistersView::onPopAction()
{
    duint csp = (* ((duint*) registerValue(&wRegDumpStruct, CSP)));
    duint newVal;
    DbgMemRead(csp, (unsigned char*)&newVal, sizeof(void*));
    setRegister(CSP, csp + sizeof(void*));
    setRegister(mSelected, newVal);
}

void CPURegistersView::onZeroAction()
{
    if(mSETONEZEROTOGGLE.contains(mSelected))
    {
        if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            setRegister(mSelected, reinterpret_cast<duint>("\0\0\0\0\0\0\0\0\0")); //9 zeros and 1 terminating zero
        else
            setRegister(mSelected, 0);
    }
}

void CPURegistersView::onSetToOneAction()
{
    if(mSETONEZEROTOGGLE.contains(mSelected))
    {
        if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            setRegister(mSelected, reinterpret_cast<duint>("\0\0\0\0\0\0\0\x80\xFF\x3F"));
        else
            setRegister(mSelected, 1);
    }
}

void CPURegistersView::onModifyAction()
{
    if(mMODIFYDISPLAY.contains(mSelected))
        displayEditDialog();
}

void CPURegistersView::onToggleValueAction()
{
    if(mBOOLDISPLAY.contains(mSelected))
    {
        int value = (int)(* (bool*) registerValue(&wRegDumpStruct, mSelected));
        setRegister(mSelected, value ^ 1);
    }
}

void CPURegistersView::onUndoAction()
{
    if(mUNDODISPLAY.contains(mSelected))
    {
        if(mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected) || mFPUx87_80BITSDISPLAY.contains(mSelected))
            setRegister(mSelected, (duint)registerValue(&wCipRegDumpStruct, mSelected));
        else
            setRegister(mSelected, *(duint*)registerValue(&wCipRegDumpStruct, mSelected));
    }
}

void CPURegistersView::onCopyToClipboardAction()
{
    Bridge::CopyToClipboard(GetRegStringValueFromValue(mSelected, registerValue(&wRegDumpStruct, mSelected)));
}

void CPURegistersView::onCopyFloatingPointToClipboardAction()
{
    Bridge::CopyToClipboard(ToLongDoubleString(((X87FPUREGISTER*) registerValue(&wRegDumpStruct, mSelected))->data));
}

void CPURegistersView::onCopySymbolToClipboardAction()
{
    if(mLABELDISPLAY.contains(mSelected))
    {
        QString symbol = getRegisterLabel(mSelected);
        if(symbol != "")
            Bridge::CopyToClipboard(symbol);
    }
}

void CPURegistersView::onHighlightSlot()
{
    Disassembly* CPUDisassemblyView = mParent->getDisasmWidget();
    if(mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS)
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::GeneralRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mSEGMENTREGISTER.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MemorySegment, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUMMX.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::MmxRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUXMM.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::XmmRegister, mRegisterMapping.constFind(mSelected).value()));
    else if(mFPUYMM.contains(mSelected))
        CPUDisassemblyView->hightlightToken(ZydisTokenizer::SingleToken(ZydisTokenizer::TokenType::YmmRegister, mRegisterMapping.constFind(mSelected).value()));
    CPUDisassemblyView->reloadData();
}

void CPURegistersView::onFollowInDisassembly()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("disasm \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void CPURegistersView::onFollowInDump()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("dump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void CPURegistersView::onFollowInDumpN()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
        {
            QAction* action = qobject_cast<QAction*>(sender());
            int numDump = action->data().toInt();
            DbgCmdExec(QString("dump %1, .%2").arg(addr).arg(numDump).toUtf8().constData());
        }
    }
}

void CPURegistersView::onFollowInStack()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("sdump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void CPURegistersView::onFollowInMemoryMap()
{
    if(mCANSTOREADDRESS.contains(mSelected))
    {
        QString addr = QString("%1").arg((* ((duint*) registerValue(&wRegDumpStruct, mSelected))), mRegisterPlaces[mSelected].valuesize, 16, QChar('0')).toUpper();
        if(DbgMemIsValidReadPtr((* ((duint*) registerValue(&wRegDumpStruct, mSelected)))))
            DbgCmdExec(QString().sprintf("memmapdump \"%s\"", addr.toUtf8().constData()).toUtf8().constData());
    }
}

void CPURegistersView::displayCustomContextMenuSlot(QPoint pos)
{
    if(!isActive)
        return;
    QMenu wMenu(this);
    QMenu* followInDumpNMenu = nullptr;
    const QAction* selectedAction = nullptr;
    switch(ConfigUint("Gui", "SIMDRegistersDisplayMode"))
    {
    case 0:
        selectedAction = SIMDHex;
        break;
    case 1:
        selectedAction = SIMDFloat;
        break;
    case 2:
        selectedAction = SIMDDouble;
        break;
    case 3:
        selectedAction = SIMDSWord;
        break;
    case 6:
        selectedAction = SIMDUWord;
        break;
    case 9:
        selectedAction = SIMDHWord;
        break;
    case 4:
        selectedAction = SIMDSDWord;
        break;
    case 7:
        selectedAction = SIMDUDWord;
        break;
    case 10:
        selectedAction = SIMDHDWord;
        break;
    case 5:
        selectedAction = SIMDSQWord;
        break;
    case 8:
        selectedAction = SIMDUQWord;
        break;
    case 11:
        selectedAction = SIMDHQWord;
        break;
    }
    SIMDHex->setChecked(SIMDHex == selectedAction);
    SIMDFloat->setChecked(SIMDFloat == selectedAction);
    SIMDDouble->setChecked(SIMDDouble == selectedAction);
    SIMDSWord->setChecked(SIMDSWord == selectedAction);
    SIMDUWord->setChecked(SIMDUWord == selectedAction);
    SIMDHWord->setChecked(SIMDHWord == selectedAction);
    SIMDSDWord->setChecked(SIMDSDWord == selectedAction);
    SIMDUDWord->setChecked(SIMDUDWord == selectedAction);
    SIMDHDWord->setChecked(SIMDHDWord == selectedAction);
    SIMDSQWord->setChecked(SIMDSQWord == selectedAction);
    SIMDUQWord->setChecked(SIMDUQWord == selectedAction);
    SIMDHQWord->setChecked(SIMDHQWord == selectedAction);

    if(mSelected != UNKNOWN)
    {
        if(mMODIFYDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_Modify);
        }

        if(mCANSTOREADDRESS.contains(mSelected))
        {
            duint addr = (* ((duint*) registerValue(&wRegDumpStruct, mSelected)));
            if(DbgMemIsValidReadPtr(addr))
            {
                wMenu.addAction(wCM_FollowInDump);
                followInDumpNMenu = new QMenu(tr("Follow in &Dump"), &wMenu);
                CreateDumpNMenu(followInDumpNMenu);
                wMenu.addMenu(followInDumpNMenu);
                wMenu.addAction(wCM_FollowInDisassembly);
                wMenu.addAction(wCM_FollowInMemoryMap);
                duint size = 0;
                duint base = DbgMemFindBaseAddr(DbgValFromString("csp"), &size);
                if(addr >= base && addr < base + size)
                    wMenu.addAction(wCM_FollowInStack);
            }
        }

        wMenu.addAction(wCM_CopyToClipboard);
        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_CopyFloatingPointValueToClipboard);
        }
        wMenu.addAction(wCM_CopyAll);
        if(mLABELDISPLAY.contains(mSelected))
        {
            QString symbol = getRegisterLabel(mSelected);
            if(symbol != "")
                wMenu.addAction(wCM_CopySymbolToClipboard);
        }

        if((mGPR.contains(mSelected) && mSelected != REGISTER_NAME::EFLAGS) || mSEGMENTREGISTER.contains(mSelected) || mFPUMMX.contains(mSelected) || mFPUXMM.contains(mSelected) || mFPUYMM.contains(mSelected))
        {
            wMenu.addAction(wCM_Highlight);
        }

        if(mUNDODISPLAY.contains(mSelected) && CompareRegisters(mSelected, &wRegDumpStruct, &wCipRegDumpStruct) != 0)
        {
            wMenu.addAction(wCM_Undo);
        }

        if(mSETONEZEROTOGGLE.contains(mSelected))
        {
            if(mSelected >= x87r0 && mSelected <= x87r7 || mSelected >= x87st0 && mSelected <= x87st7)
            {
                if(memcmp(registerValue(&wRegDumpStruct, mSelected), "\0\0\0\0\0\0\0\0\0", 10) != 0)
                    wMenu.addAction(wCM_Zero);
                if(memcmp(registerValue(&wRegDumpStruct, mSelected), "\0\0\0\0\0\0\0\x80\xFF\x3F", 10) != 0)
                    wMenu.addAction(wCM_SetToOne);
            }
            else
            {
                if((* ((duint*) registerValue(&wRegDumpStruct, mSelected))) != 0)
                    wMenu.addAction(wCM_Zero);
                if((* ((duint*) registerValue(&wRegDumpStruct, mSelected))) == 0)
                    wMenu.addAction(wCM_SetToOne);
            }
        }

        if(mBOOLDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_ToggleValue);
        }

        if(mFPUx87_80BITSDISPLAY.contains(mSelected))
        {
            wMenu.addAction(wCM_Incrementx87Stack);
            wMenu.addAction(wCM_Decrementx87Stack);
        }

        if(mINCREMENTDECREMET.contains(mSelected))
        {
            wMenu.addAction(wCM_Increment);
            wMenu.addAction(wCM_Decrement);
            wMenu.addAction(wCM_IncrementPtrSize);
            wMenu.addAction(wCM_DecrementPtrSize);
        }

        if(mGPR.contains(mSelected) || mSelected == CIP)
        {
            wMenu.addAction(wCM_Push);
            wMenu.addAction(wCM_Pop);
        }

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
        wMenu.addSeparator();
        QAction* wHwbpCsp = wMenu.addAction(DIcon("breakpoint.png"), tr("Set Hardware Breakpoint on %1").arg(ArchValue("ESP", "RSP")));
        QAction* wAction = wMenu.exec(this->mapToGlobal(pos));

        if(wAction == wHwbpCsp)
            DbgCmdExec("bphws csp,rw");
    }
}

void CPURegistersView::onSIMDMode()
{
    Config()->setUint("Gui", "SIMDRegistersDisplayMode", dynamic_cast<QAction*>(sender())->data().toInt());
    emit refresh();
    GuiUpdateDisassemblyView(); // refresh display mode for data in disassembly
}

void CPURegistersView::onFpuMode()
{
    mFpuMode = (char)(dynamic_cast<QAction*>(sender())->data().toInt());
    InitMappings();
    emit refresh();
}

void CPURegistersView::setRegister(REGISTER_NAME reg, duint value)
{
    // is register-id known?
    if(mRegisterMapping.contains(reg))
    {
        // map x87st0 to x87r0
        QString wRegName;
        if(reg >= x87st0 && reg <= x87st7)
            wRegName = QString().sprintf("st%d", reg - x87st0);
        else
            // map "cax" to "eax" or "rax"
            wRegName = mRegisterMapping.constFind(reg).value();

        // flags need to '_' infront
        if(mFlags.contains(reg))
            wRegName = "_" + wRegName;

        // we change the value (so highlight it)
        mRegisterUpdates.insert(reg);
        // tell everything the compiler
        if(mFPU.contains(reg))
            wRegName = "_" + wRegName;

        DbgValToString(wRegName.toUtf8().constData(), value);

        // force repaint
        emit refresh();
    }
}

void CPURegistersView::disasmSelectionChangedSlot(dsint va)
{
    mHighlightRegs = mParent->getDisasmWidget()->DisassembleAt(va - mParent->getDisasmWidget()->getBase()).regsReferenced;
    emit refresh();
}
