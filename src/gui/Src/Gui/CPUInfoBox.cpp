#include "CPUInfoBox.h"
#include "Configuration.h"
#include "WordEditDialog.h"
#include "XrefBrowseDialog.h"
#include "Bridge.h"
#include "QBeaEngine.h"

CPUInfoBox::CPUInfoBox(QWidget* parent) : StdTable(parent)
{
    setWindowTitle("InfoBox");
    enableMultiSelection(false);
    setShowHeader(false);
    addColumnAt(0, "", true);
    setRowCount(4);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");
    setCellContent(3, 0, "");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    int height = getHeight();
    setMinimumHeight(height);

    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(addInfoLine(QString)), this, SLOT(addInfoLine(QString)));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    mCurAddr = 0;

    // Deselect any row (visual reasons only)
    setSingleSelection(-1);

    int maxModuleSize = (int)ConfigUint("Disassembler", "MaxModuleSize");
    mDisasm = new QBeaEngine(maxModuleSize);

    setupContextMenu();
}

CPUInfoBox::~CPUInfoBox()
{
    delete mDisasm;
}

void CPUInfoBox::setupContextMenu()
{
    mCopyAddressAction = makeAction(tr("Address"), SLOT(copyAddress()));
    mCopyRvaAction = makeAction(tr("RVA"), SLOT(copyRva()));
    mCopyOffsetAction = makeAction(tr("File Offset"), SLOT(copyOffset()));
    mCopyLineAction = makeAction(tr("Copy Line"), SLOT(copyLineSlot()));
    setupShortcuts();
}

int CPUInfoBox::getHeight()
{
    return ((getRowHeight() + 1) * 4);
}

void CPUInfoBox::setInfoLine(int line, QString text)
{
    if(line < 0 || line > 3)
        return;

    setCellContent(line, 0, text);
    reloadData();
}

QString CPUInfoBox::getInfoLine(int line)
{
    if(line < 0 || line > 3)
        return QString();

    return getCellContent(line, 0);
}

void CPUInfoBox::addInfoLine(const QString & infoLine)
{
    auto rowCount = getRowCount();
    setRowCount(rowCount + 1);
    setCellContent(rowCount, 0, infoLine);
    reloadData();
}

void CPUInfoBox::clear()
{
    // Set all 4 lines to empty strings
    setRowCount(4);
    setInfoLine(0, "");
    setInfoLine(1, "");
    setInfoLine(2, "");
    setInfoLine(3, "");
}

QString CPUInfoBox::formatSSEOperand(const QByteArray & data, unsigned char vectorType)
{
    QString hex;
    bool isXMMdecoded = false;
    switch(vectorType)
    {
    case Zydis::VETFloat32:
        if(data.size() == 32)
        {
            hex = composeRegTextYMM(data.constData(), 1);
            isXMMdecoded = true;
        }
        else if(data.size() == 16)
        {
            hex = composeRegTextXMM(data.constData(), 1);
            isXMMdecoded = true;
        }
        else if(data.size() == 4)
        {
            hex = ToFloatString(data.constData());
            isXMMdecoded = true;
        }
        break;
    case Zydis::VETFloat64:
        if(data.size() == 32)
        {
            hex = composeRegTextYMM(data.constData(), 2);
            isXMMdecoded = true;
        }
        else if(data.size() == 16)
        {
            hex = composeRegTextXMM(data.constData(), 2);
            isXMMdecoded = true;
        }
        else if(data.size() == 8)
        {
            hex = ToDoubleString(data.constData());
            isXMMdecoded = true;
        }
        break;
    default:
        isXMMdecoded = false;
        break;
    }
    if(!isXMMdecoded)
    {
        hex.reserve(data.size() * 3);
        for(int k = 0; k < data.size(); k++)
        {
            if(k)
                hex.append(' ');
            hex.append(ToByteString(data[k]));
        }
    }
    return hex;
}

void CPUInfoBox::disasmSelectionChanged(duint parVA)
{
    mCurAddr = parVA;
    mCurRva = -1;
    mCurOffset = -1;

    if(!DbgIsDebugging() || !DbgMemIsValidReadPtr(parVA))
        return;

    // Rather than using clear() or setInfoLine(), only reset the first three cells to reduce flicker
    setRowCount(4);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");

    Instruction_t inst;
    unsigned char instructiondata[MAX_DISASM_BUFFER];
    DbgMemRead(parVA, &instructiondata, MAX_DISASM_BUFFER);
    inst = mDisasm->DisassembleAt(instructiondata, MAX_DISASM_BUFFER, 0, parVA);
    DISASM_INSTR instr; //Fix me: these disasm methods are so messy
    DbgDisasmAt(parVA, &instr);
    BASIC_INSTRUCTION_INFO basicinfo;
    DbgDisasmFastAt(parVA, &basicinfo);

    int start = 0;
    bool commentThis = !ConfigBool("Disassembler", "OnlyCipAutoComments") || parVA == DbgValFromString("cip");
    if(inst.branchType == Instruction_t::Conditional && commentThis) //jump
    {
        bool taken = DbgIsJumpGoingToExecute(parVA);
        if(taken)
            setInfoLine(0, tr("Jump is taken"));
        else
            setInfoLine(0, tr("Jump is not taken"));
        start = 1;
    }

    bool bUpper = ConfigBool("Disassembler", "Uppercase");

    for(int i = 0, j = start; i < instr.argcount && j < 2; i++)
    {
        const DISASM_ARG & arg = instr.arg[i];
        QString argMnemonic = QString(arg.mnemonic);
        if(bUpper)
            argMnemonic = argMnemonic.toUpper();
        if(arg.type == arg_memory)
        {
            bool ok;
            argMnemonic.toULongLong(&ok, 16);
            QString valText = DbgMemIsValidReadPtr(arg.value) ? ToPtrString(arg.value) : ToHexString(arg.value);
            auto valTextSym = getSymbolicNameStr(arg.value);
            if(!valTextSym.contains(valText))
                valText = QString("%1 %2").arg(valText, valTextSym);
            else
                valText = valTextSym;
            argMnemonic = !ok ? QString("%1]=[%2").arg(argMnemonic).arg(valText) : valText;
            QString sizeName = "";
            bool knownsize = true;
            switch(basicinfo.memory.size)
            {
            case size_byte:
                sizeName = "byte ptr ";
                break;
            case size_word:
                sizeName = "word ptr ";
                break;
            case size_dword:
                sizeName = "dword ptr ";
                break;
#ifdef _WIN64
            case size_qword:
                sizeName = "qword ptr ";
                break;
#endif //_WIN64
            case size_xmmword:
                knownsize = false;
                sizeName = "xmmword ptr ";
                break;
            case size_ymmword:
                knownsize = false;
                sizeName = "ymmword ptr ";
                break;
            default:
                knownsize = false;
                break;
            }

            sizeName += [](SEGMENTREG seg)
            {
                switch(seg)
                {
                case SEG_ES:
                    return "es:";
                case SEG_DS:
                    return "ds:";
                case SEG_FS:
                    return "fs:";
                case SEG_GS:
                    return "gs:";
                case SEG_CS:
                    return "cs:";
                case SEG_SS:
                    return "ss:";
                default:
                    return "";
                }
            }(arg.segment);

            if(bUpper)
                sizeName = sizeName.toUpper();

            if(!DbgMemIsValidReadPtr(arg.value))
            {
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=???");
            }
            else if(knownsize && inst.vectorElementType[i] == Zydis::VETDefault) // MOVSD/MOVSS instruction
            {
                QString addrText = getSymbolicNameStr(arg.memvalue);
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + addrText);
            }
            else
            {
                QByteArray data;
                data.resize(basicinfo.memory.size);
                memset(data.data(), 0, data.size());
                if(DbgMemRead(arg.value, data.data(), data.size()))
                {
                    setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + formatSSEOperand(data, inst.vectorElementType[i]));
                }
                else
                {
                    setInfoLine(j, sizeName + "[" + argMnemonic + "]=???");
                }
            }

            j++;
        }
        else
        {
            QString valText;
            auto symbolicName = getSymbolicNameStr(arg.value);
            if(!symbolicName.contains(valText))
                valText = QString("%1 (%2)").arg(symbolicName, valText);
            else
                valText = symbolicName;
            QString mnemonic(arg.mnemonic);
            bool ok;
            mnemonic.toULongLong(&ok, 16);
            if(ok) //skip certain numbers
            {
                if(ToHexString(arg.value) == symbolicName)
                    continue;
                setInfoLine(j, symbolicName);
            }
            else if(!mnemonic.startsWith("xmm") &&
                    !mnemonic.startsWith("ymm") &&
                    !mnemonic.startsWith("zmm") && //TODO: properly handle display of AVX-512 registers
                    !mnemonic.startsWith("k") && //TODO: properly handle display of AVX-512 registers
                    !mnemonic.startsWith("st"))
            {
                setInfoLine(j, mnemonic + "=" + valText);
                j++;
            }
            else if(mnemonic.startsWith("xmm") || mnemonic.startsWith("ymm") || mnemonic.startsWith("st"))
            {
                REGDUMP registers;
                DbgGetRegDumpEx(&registers, sizeof(registers));
                if(mnemonic == "xmm0")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[0], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm1")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[1], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm2")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[2], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm3")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[3], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm4")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[4], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm5")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[5], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm6")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[6], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm7")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[7], 16), inst.vectorElementType[i]);
#ifdef _WIN64
                else if(mnemonic == "xmm8")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[8], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm9")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[9], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm10")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[10], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm11")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[11], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm12")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[12], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm13")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[13], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm14")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[14], 16), inst.vectorElementType[i]);
                else if(mnemonic == "xmm15")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.XmmRegisters[15], 16), inst.vectorElementType[i]);
#endif //_WIN64
                else if(mnemonic == "ymm0")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[0], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm1")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[1], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm2")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[2], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm3")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[3], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm4")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[4], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm5")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[5], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm6")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[6], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm7")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[7], 32), inst.vectorElementType[i]);
#ifdef _WIN64
                else if(mnemonic == "ymm8")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[8], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm9")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[9], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm10")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[10], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm11")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[11], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm12")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[12], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm13")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[13], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm14")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[14], 32), inst.vectorElementType[i]);
                else if(mnemonic == "ymm15")
                    valText = formatSSEOperand(QByteArray((const char*)&registers.regcontext.YmmRegisters[15], 32), inst.vectorElementType[i]);
#endif //_WIN64
                else if(mnemonic == "st0")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[registers.x87StatusWordFields.TOP & 7]);
                else if(mnemonic == "st1")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 1) & 7]);
                else if(mnemonic == "st2")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 2) & 7]);
                else if(mnemonic == "st3")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 3) & 7]);
                else if(mnemonic == "st4")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 4) & 7]);
                else if(mnemonic == "st5")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 5) & 7]);
                else if(mnemonic == "st6")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 6) & 7]);
                else if(mnemonic == "st7")
                    valText = ToLongDoubleString(&registers.x87FPURegisters[(registers.x87StatusWordFields.TOP + 7) & 7]);
                setInfoLine(j, mnemonic + "=" + valText);
                j++;
            }
        }
    }
    if(getInfoLine(0) == getInfoLine(1)) //check for duplicate info line
        setInfoLine(1, "");

    // check references details
    // code extracted from ExtraInfo plugin by torusrxxx
    XREF_INFO xrefInfo;
    xrefInfo.refcount = 0;
    xrefInfo.references = nullptr;

    if(DbgXrefGet(parVA, &xrefInfo) && xrefInfo.refcount > 0)
    {
        QString output;
        std::vector<XREF_RECORD*> data;
        for(duint i = 0; i < xrefInfo.refcount; i++)
            data.push_back(&xrefInfo.references[i]);

        std::sort(data.begin(), data.end(), [](const XREF_RECORD * A, const XREF_RECORD * B)
        {
            if(A->type != B->type)
                return (A->type < B->type);

            return (A->addr < B->addr);
        });

        int t = XREF_NONE;
        duint i;

        for(i = 0; i < xrefInfo.refcount && i < 10; i++)
        {
            if(t != data[i]->type)
            {
                switch(data[i]->type)
                {
                case XREF_JMP:
                    output += tr("Jump from ");
                    break;
                case XREF_CALL:
                    output += tr("Call from ");
                    break;
                default:
                    output += tr("Reference from ");
                    break;
                }

                t = data[i]->type;
            }

            char clabel[MAX_LABEL_SIZE] = "";

            DbgGetLabelAt(data[i]->addr, SEG_DEFAULT, clabel);
            if(*clabel)
                output += QString(clabel);
            else
            {
                duint start;
                if(DbgFunctionGet(data[i]->addr, &start, nullptr) && DbgGetLabelAt(start, SEG_DEFAULT, clabel) && start != data[i]->addr)
                    output += QString("%1+%2").arg(clabel).arg(ToHexString(data[i]->addr - start));
                else
                    output += QString("%1").arg(ToHexString(data[i]->addr));
            }

            if(i != xrefInfo.refcount - 1)
                output += ", ";
        }

        data.clear();
        if(xrefInfo.refcount > 10)
            output += "...";

        setInfoLine(2, output);
    }

    // Set last line
    //
    // Format: SECTION:VA MODULE:$RVA :#FILE_OFFSET FUNCTION, Accessed %u times
    QString info;

    // Section
    char section[MAX_SECTION_SIZE * 5];
    if(DbgFunctions()->SectionFromAddr(parVA, section))
        info += QString(section) + ":";

    // VA
    info += ToPtrString(parVA);

    // Module name, RVA, and file offset
    char mod[MAX_MODULE_SIZE];
    if(DbgFunctions()->ModNameFromAddr(parVA, mod, true))
    {
        dsint modbase = DbgFunctions()->ModBaseFromAddr(parVA);

        // Append modname
        info += " " + QString(mod);

        // Module RVA
        mCurRva = parVA - modbase;
        if(modbase)
            info += QString(":$%1").arg(ToHexString(mCurRva));

        // File offset
        mCurOffset = DbgFunctions()->VaToFileOffset(parVA);
        info += QString(" #%1").arg(ToHexString(mCurOffset));
    }

    // Function/label name
    char label[MAX_LABEL_SIZE];
    if(DbgGetLabelAt(parVA, SEG_DEFAULT, label))
        info += QString(" <%1>").arg(label);
    else
    {
        duint start;
        if(DbgFunctionGet(parVA, &start, nullptr) && DbgGetLabelAt(start, SEG_DEFAULT, label) && start != parVA)
            info += QString(" <%1+%2>").arg(label).arg(ToHexString(parVA - start));
    }

    auto tracedCount = DbgFunctions()->GetTraceRecordHitCount(parVA);
    if(tracedCount != 0)
    {
        info += ", " + tr("Accessed %n time(s)", nullptr, tracedCount);
    }

    setInfoLine(3, info);

    DbgSelChanged(GUI_DISASSEMBLY, parVA);
}

void CPUInfoBox::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        clear();
}

/**
 * @brief CPUInfoBox::followActionSlot Called when follow or watch action is clicked
 */
void CPUInfoBox::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action->objectName().startsWith("DUMP|"))
        DbgCmdExec(QString("dump \"%1\"").arg(action->objectName().mid(5)));
    else if(action && action->objectName().startsWith("WATCH|"))
        DbgCmdExec(QString("AddWatch \"[%1]\"").arg(action->objectName().mid(6)));
}

void CPUInfoBox::modifySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        duint addrVal = 0;
        DbgFunctions()->ValFromString(action->objectName().toUtf8().constData(), &addrVal);
        WordEditDialog editDialog(this);
        dsint value = 0;
        DbgMemRead(addrVal, &value, sizeof(dsint));
        editDialog.setup(tr("Modify Value"), value, sizeof(dsint));
        if(editDialog.exec() != QDialog::Accepted)
            return;
        value = editDialog.getVal();
        DbgMemWrite(addrVal, &value, sizeof(dsint));
        GuiUpdateAllViews();
    }
}

void CPUInfoBox::findXReferencesSlot()
{
    if(!DbgIsDebugging())
        return;
    if(!mXrefDlg)
        mXrefDlg = new XrefBrowseDialog(this);
    mXrefDlg->setup(mCurAddr, [](duint address)
    {
        DbgCmdExec(QString("disasm %1").arg(ToPtrString(address)));
    });
    mXrefDlg->showNormal();
}

void CPUInfoBox::addModifyValueMenuItem(QMenu* menu, QString name, duint value)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, menu);
    menu->addAction(newAction);
    newAction->setObjectName(ToPtrString(value));
    connect(newAction, SIGNAL(triggered()), this, SLOT(modifySlot()));
}

void CPUInfoBox::setupModifyValueMenu(QMenu* menu, duint va)
{
    menu->setIcon(DIcon("modify"));

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(va, &instr);

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            QString segment = "";
#ifdef _WIN64
            if(arg.segment == SEG_GS)
                segment = "gs:";
#else //x32
            if(arg.segment == SEG_FS)
                segment = "fs:";
#endif //_WIN64
            if(DbgMemIsValidReadPtr(arg.value))
                addModifyValueMenuItem(menu, tr("&Address: ") + segment + QString(arg.mnemonic).toUpper().trimmed(), arg.value);
            if(arg.value != arg.constant)
            {
                QString constant = QString("%1").arg(ToHexString(arg.constant));
                if(DbgMemIsValidReadPtr(arg.constant))
                    addModifyValueMenuItem(menu, tr("&Constant: ") + constant, arg.constant);
            }
            if(DbgMemIsValidReadPtr(arg.memvalue))
                addModifyValueMenuItem(menu, tr("&Value: ") + segment + "[" + QString(arg.mnemonic).toUpper().trimmed() + "]", arg.memvalue);
        }
        else
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addModifyValueMenuItem(menu, "&Value: [" + QString(arg.mnemonic).toUpper().trimmed() + "]", arg.value);
        }
    }
}

/**
 * @brief CPUInfoBox::addFollowMenuItem Add a follow action to the menu
 * @param menu The menu to which the follow action adds
 * @param name The user-friendly name of the action
 * @param value The VA of the address
 */
void CPUInfoBox::addFollowMenuItem(QMenu* menu, QString name, duint value)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, menu);
    menu->addAction(newAction);
    newAction->setObjectName(QString("DUMP|") + ToPtrString(value));
    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

/**
 * @brief CPUInfoBox::setupFollowMenu Set up a follow menu.
 * @param menu The menu to create
 * @param va The selected VA
 */
void CPUInfoBox::setupFollowMenu(QMenu* menu, duint va)
{
    menu->setIcon(DIcon("dump"));
    //most basic follow action
    addFollowMenuItem(menu, tr("&Selected Address"), va);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(va, &instr);

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            QString segment = "";
#ifdef _WIN64
            if(arg.segment == SEG_GS)
                segment = "gs:";
#else //x32
            if(arg.segment == SEG_FS)
                segment = "fs:";
#endif //_WIN64
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem(menu, tr("&Address: ") + segment + QString(arg.mnemonic).toUpper().trimmed(), arg.value);
            if(arg.value != arg.constant)
            {
                QString constant = QString("%1").arg(ToHexString(arg.constant));
                if(DbgMemIsValidReadPtr(arg.constant))
                    addFollowMenuItem(menu, tr("&Constant: ") + constant, arg.constant);
            }
            if(DbgMemIsValidReadPtr(arg.memvalue))
                addFollowMenuItem(menu, tr("&Value: ") + segment + "[" + QString(arg.mnemonic) + "]", arg.memvalue);
        }
        else
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem(menu, QString(arg.mnemonic).toUpper().trimmed(), arg.value);
        }
    }
}

/**
 * @brief CPUInfoBox::addFollowMenuItem Add a follow action to the menu
 * @param menu The menu to which the follow action adds
 * @param name The user-friendly name of the action
 * @param value The VA of the address
 */
void CPUInfoBox::addWatchMenuItem(QMenu* menu, QString name, duint value)
{
    foreach(QAction* action, menu->actions()) //check for duplicate action
        if(action->text() == name)
            return;
    QAction* newAction = new QAction(name, menu);
    menu->addAction(newAction);
    newAction->setObjectName(QString("WATCH|") + ToPtrString(value));
    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

/**
 * @brief CPUInfoBox::setupFollowMenu Set up a follow menu.
 * @param menu The menu to create
 * @param va The selected VA
 */
void CPUInfoBox::setupWatchMenu(QMenu* menu, duint va)
{
    menu->setIcon(DIcon("animal-dog"));
    //most basic follow action
    addWatchMenuItem(menu, tr("&Selected Address"), va);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(va, &instr);

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            QString segment = "";
#ifdef _WIN64
            if(arg.segment == SEG_GS)
                segment = "gs:";
#else //x32
            if(arg.segment == SEG_FS)
                segment = "fs:";
#endif //_WIN64
            if(DbgMemIsValidReadPtr(arg.value))
                addWatchMenuItem(menu, tr("&Address: ") + segment + QString(arg.mnemonic).toUpper().trimmed(), arg.value);
            if(arg.value != arg.constant)
            {
                QString constant = QString("%1").arg(ToHexString(arg.constant));
                if(DbgMemIsValidReadPtr(arg.constant))
                    addWatchMenuItem(menu, tr("&Constant: ") + constant, arg.constant);
            }
            if(DbgMemIsValidReadPtr(arg.memvalue))
                addWatchMenuItem(menu, tr("&Value: ") + segment + "[" + QString(arg.mnemonic) + "]", arg.memvalue);
        }
        else
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addWatchMenuItem(menu, QString(arg.mnemonic).toUpper().trimmed(), arg.value);
        }
    }
}

int CPUInfoBox::followInDump(duint va)
{
    // Copy pasta from setupFollowMenu for now
    int tableOffset = getInitialSelection();
    QString cellContent = this->getCellContent(tableOffset, 0);

    // No text in row that was clicked
    if(cellContent.length() == 0)
        return -1;

    // Last line of infoBox => Current Address(EIP) in disassembly
    if(tableOffset == 2)
    {
        DbgCmdExec(QString("dump %1").arg(ToPtrString(va)));
        return 0;
    }

    DISASM_INSTR instr;
    DbgDisasmAt(va, &instr);

    if(instr.type == instr_branch && cellContent.contains("Jump"))
    {
        DbgCmdExec(QString("dump %1").arg(ToPtrString(instr.arg[0].value)));
        return 0;
    }

    // Loop through all instruction arguments
    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            if(DbgMemIsValidReadPtr(arg.value))
            {
                if(cellContent.contains(arg.mnemonic))
                {
                    DbgCmdExec(QString("dump %1").arg(ToPtrString(arg.value)));
                    return 0;
                }
            }
        }
    }
    return 0;
}

void CPUInfoBox::contextMenuSlot(QPoint pos)
{
    QMenu menu(this); //create context menu
    QMenu followMenu(tr("&Follow in Dump"), this);
    setupFollowMenu(&followMenu, mCurAddr);
    menu.addMenu(&followMenu);
    QMenu modifyValueMenu(tr("&Modify Value"), this);
    setupModifyValueMenu(&modifyValueMenu, mCurAddr);
    if(!modifyValueMenu.isEmpty())
        menu.addMenu(&modifyValueMenu);
    QMenu watchMenu(tr("&Watch"), this);
    setupWatchMenu(&watchMenu, mCurAddr);
    menu.addMenu(&watchMenu);
    if(!getInfoLine(2).isEmpty())
        menu.addAction(makeAction(DIcon("xrefs"), tr("&Show References"), SLOT(findXReferencesSlot())));
    QMenu copyMenu(tr("&Copy"), this);
    setupCopyMenu(&copyMenu);
    if(DbgIsDebugging())
    {
        copyMenu.addAction(mCopyAddressAction);
        if(mCurRva != -1)
            copyMenu.addAction(mCopyRvaAction);
        if(mCurOffset != -1)
            copyMenu.addAction(mCopyOffsetAction);
    }
    if(copyMenu.actions().length())
    {
        menu.addSeparator();
        menu.addMenu(&copyMenu);
    }
    menu.exec(mapToGlobal(pos)); //execute context menu
}

void CPUInfoBox::copyAddress()
{
    Bridge::CopyToClipboard(ToPtrString(mCurAddr));
}

void CPUInfoBox::copyRva()
{
    Bridge::CopyToClipboard(ToHexString(mCurRva));
}

void CPUInfoBox::copyOffset()
{
    Bridge::CopyToClipboard(ToHexString(mCurOffset));
}

void CPUInfoBox::doubleClickedSlot()
{
    followInDump(mCurAddr);
}

void CPUInfoBox::setupShortcuts()
{
    mCopyLineAction->setShortcut(ConfigShortcut("ActionCopyLine"));
    addAction(mCopyLineAction);
}
