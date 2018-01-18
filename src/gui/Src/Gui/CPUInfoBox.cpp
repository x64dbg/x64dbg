#include "CPUInfoBox.h"
#include "Configuration.h"
#include "WordEditDialog.h"
#include "XrefBrowseDialog.h"
#include "Bridge.h"

CPUInfoBox::CPUInfoBox(StdTable* parent) : StdTable(parent)
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
    horizontalScrollBar()->setStyleSheet(ConfigHScrollBarStyle());

    int height = getHeight();
    setMinimumHeight(height);

    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));
    connect(Bridge::getBridge(), SIGNAL(addInfoLine(QString)), this, SLOT(addInfoLine(QString)));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(doubleClickedSlot()));
    curAddr = 0;

    // Deselect any row (visual reasons only)
    setSingleSelection(-1);

    setupContextMenu();
}

void CPUInfoBox::setupContextMenu()
{
    mCopyAddressAction = makeAction(tr("Address"), SLOT(copyAddress()));
    mCopyRvaAction = makeAction(tr("RVA"), SLOT(copyRva()));
    mCopyOffsetAction = makeAction(tr("File Offset"), SLOT(copyOffset()));
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

void CPUInfoBox::disasmSelectionChanged(dsint parVA)
{
    curAddr = parVA;
    curRva = -1;
    curOffset = -1;

    if(!DbgIsDebugging() || !DbgMemIsValidReadPtr(parVA))
        return;

    // Rather than using clear() or setInfoLine(), only reset the first three cells to reduce flicker
    setRowCount(4);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");

    DISASM_INSTR instr;
    memset(&instr, 0, sizeof(instr));
    DbgDisasmAt(parVA, &instr);
    BASIC_INSTRUCTION_INFO basicinfo;
    memset(&basicinfo, 0, sizeof(basicinfo));
    DbgDisasmFastAt(parVA, &basicinfo);

    int start = 0;
    if(basicinfo.branch && !basicinfo.call && (!ConfigBool("Disassembler", "OnlyCipAutoComments") || parVA == DbgValFromString("cip"))) //jump
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
        DISASM_ARG arg = instr.arg[i];
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
            case size_qword:
                sizeName = "qword ptr ";
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
            else if(knownsize)
            {
                QString addrText = getSymbolicNameStr(arg.memvalue);
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + addrText);
            }
            else
            {
                //TODO: properly support XMM constants
                QVector<unsigned char> data;
                data.resize(basicinfo.memory.size);
                memset(data.data(), 0, data.size());
                if(DbgMemRead(arg.value, data.data(), data.size()))
                {
                    QString hex;
                    hex.reserve(data.size() * 3);
                    for(int k = 0; k < data.size(); k++)
                    {
                        if(k)
                            hex.append(' ');
                        hex.append(ToByteString(data[k]));
                    }
                    setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + hex);
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
            QString valText = DbgMemIsValidReadPtr(arg.value) ? ToPtrString(arg.value) : ToHexString(arg.value);
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
            else if(!mnemonic.startsWith("xmm") && //TODO: properly handle display of these registers
                    !mnemonic.startsWith("ymm") &&
                    !mnemonic.startsWith("st"))
            {
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
            return ((A->type < B->type) || (A->addr < B->addr));
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
        curRva = parVA - modbase;
        if(modbase)
            info += QString(":$%1").arg(ToHexString(curRva));

        // File offset
        curOffset = DbgFunctions()->VaToFileOffset(parVA);
        info += QString(" #%1").arg(ToHexString(curOffset));
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
        DbgCmdExec(QString("dump \"%1\"").arg(action->objectName().mid(5)).toUtf8().constData());
    else if(action && action->objectName().startsWith("WATCH|"))
        DbgCmdExec(QString("AddWatch \"[%1]\"").arg(action->objectName().mid(6)).toUtf8().constData());
}

void CPUInfoBox::modifySlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action)
    {
        duint addrVal = 0;
        DbgFunctions()->ValFromString(action->objectName().toUtf8().constData(), &addrVal);
        WordEditDialog wEditDialog(this);
        dsint value = 0;
        DbgMemRead(addrVal, &value, sizeof(dsint));
        wEditDialog.setup(tr("Modify Value"), value, sizeof(dsint));
        if(wEditDialog.exec() != QDialog::Accepted)
            return;
        value = wEditDialog.getVal();
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
    mXrefDlg->setup(curAddr);
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

void CPUInfoBox::setupModifyValueMenu(QMenu* menu, duint wVA)
{
    menu->setIcon(DIcon("modify.png"));

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

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
 * @param wVA The selected VA
 */
void CPUInfoBox::setupFollowMenu(QMenu* menu, duint wVA)
{
    menu->setIcon(DIcon("dump.png"));
    //most basic follow action
    addFollowMenuItem(menu, tr("&Selected Address"), wVA);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

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
 * @param wVA The selected VA
 */
void CPUInfoBox::setupWatchMenu(QMenu* menu, duint wVA)
{
    menu->setIcon(DIcon("animal-dog.png"));
    //most basic follow action
    addWatchMenuItem(menu, tr("&Selected Address"), wVA);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

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

int CPUInfoBox::followInDump(dsint wVA)
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
        DbgCmdExec(QString("dump %1").arg(ToPtrString(wVA)).toUtf8().constData());
        return 0;
    }

    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

    if(instr.type == instr_branch && cellContent.contains("Jump"))
    {
        DbgCmdExec(QString("dump %1").arg(ToPtrString(instr.arg[0].value)).toUtf8().constData());
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
                    DbgCmdExec(QString("dump %1").arg(ToPtrString(arg.value)).toUtf8().constData());
                    return 0;
                }
            }
        }
    }
    return 0;
}

void CPUInfoBox::contextMenuSlot(QPoint pos)
{
    QMenu wMenu(this); //create context menu
    QMenu wFollowMenu(tr("&Follow in Dump"), this);
    setupFollowMenu(&wFollowMenu, curAddr);
    wMenu.addMenu(&wFollowMenu);
    QMenu wModifyValueMenu(tr("&Modify Value"), this);
    setupModifyValueMenu(&wModifyValueMenu, curAddr);
    if(!wModifyValueMenu.isEmpty())
        wMenu.addMenu(&wModifyValueMenu);
    QMenu wWatchMenu(tr("&Watch"), this);
    setupWatchMenu(&wWatchMenu, curAddr);
    wMenu.addMenu(&wWatchMenu);
    if(!getInfoLine(2).isEmpty())
        wMenu.addAction(makeAction(DIcon("xrefs.png"), tr("&Show References"), SLOT(findXReferencesSlot())));
    QMenu wCopyMenu(tr("&Copy"), this);
    setupCopyMenu(&wCopyMenu);
    if(DbgIsDebugging())
    {
        wCopyMenu.addAction(mCopyAddressAction);
        if(curRva != -1)
            wCopyMenu.addAction(mCopyRvaAction);
        if(curOffset != -1)
            wCopyMenu.addAction(mCopyOffsetAction);
    }
    if(wCopyMenu.actions().length())
    {
        wMenu.addSeparator();
        wMenu.addMenu(&wCopyMenu);
    }
    wMenu.exec(mapToGlobal(pos)); //execute context menu
}

void CPUInfoBox::copyAddress()
{
    Bridge::CopyToClipboard(ToPtrString(curAddr));
}

void CPUInfoBox::copyRva()
{
    Bridge::CopyToClipboard(ToHexString(curRva));
}

void CPUInfoBox::copyOffset()
{
    Bridge::CopyToClipboard(ToHexString(curOffset));
}

void CPUInfoBox::doubleClickedSlot()
{
    followInDump(curAddr);
}
