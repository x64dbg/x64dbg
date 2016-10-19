#include "CPUInfoBox.h"
#include "Configuration.h"
#include "Bridge.h"

CPUInfoBox::CPUInfoBox(StdTable* parent) : StdTable(parent)
{
    setWindowTitle("InfoBox");
    enableMultiSelection(false);
    setShowHeader(false);
    addColumnAt(0, "", true);
    setRowCount(3);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");
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
    return ((getRowHeight() + 1) * 3);
}

void CPUInfoBox::setInfoLine(int line, QString text)
{
    if(line < 0 || line > 2)
        return;

    setCellContent(line, 0, text);
    reloadData();
}

QString CPUInfoBox::getInfoLine(int line)
{
    if(line < 0 || line > 2)
        return "";

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
    // Set all 3 lines to empty strings
    setRowCount(3);
    setInfoLine(0, "");
    setInfoLine(1, "");
    setInfoLine(2, "");
}

static QString escapeCh(QChar ch)
{
    switch(ch.unicode())
    {
    case '\t':
        return "\\t";
    case '\f':
        return "\\f";
    case '\v':
        return "\\v";
    case '\n':
        return "\\n";
    case '\r':
        return "\\r";
    case '\\':
        return "\\\\";
    case '\"':
        return "\\\"";
    default:
        return QString(1, ch);
    }
}

QString CPUInfoBox::getSymbolicName(dsint addr)
{
    char labelText[MAX_LABEL_SIZE] = "";
    char moduleText[MAX_MODULE_SIZE] = "";
    char string[MAX_STRING_SIZE] = "";
    bool bHasString = DbgGetStringAt(addr, string);
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, labelText);
    bool bHasModule = (DbgGetModuleAt(addr, moduleText) && !QString(labelText).startsWith("JMP.&"));
    QString addrText = DbgMemIsValidReadPtr(addr) ? ToPtrString(addr) : ToHexString(addr);
    QString finalText;
    if(bHasString)
        finalText = addrText + " " + QString(string);
    else if(bHasLabel && bHasModule) //<module.label>
        finalText = QString("<%1.%2>").arg(moduleText).arg(labelText);
    else if(bHasModule) //module.addr
        finalText = QString("%1.%2").arg(moduleText).arg(addrText);
    else if(bHasLabel) //<label>
        finalText = QString("<%1>").arg(labelText);
    else
    {
        finalText = addrText;
        if(addr == (addr & 0xFF))
        {
            QChar c = QChar((char)addr);
            if(c.isPrint() || c.isSpace())
                finalText += QString(" '%1'").arg(escapeCh(c));
        }
        else if(addr == (addr & 0xFFF)) //UNICODE?
        {
            QChar c = QChar((ushort)addr);
            if(c.isPrint() || c.isSpace())
                finalText += QString(" L'%1'").arg(escapeCh(c));
        }
    }
    return finalText;
}

void CPUInfoBox::disasmSelectionChanged(dsint parVA)
{
    curAddr = parVA;
    curRva = -1;
    curOffset = -1;

    if(!DbgIsDebugging() || !DbgMemIsValidReadPtr(parVA))
        return;

    // Rather than using clear() or setInfoLine(), only reset the first two cells to reduce flicker
    setRowCount(3);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");

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
            argMnemonic = !ok ? QString("%1]=[%2").arg(argMnemonic).arg(valText) : valText;
            QString sizeName = "";
            int memsize = basicinfo.memory.size;
            switch(memsize)
            {
            case size_byte:
                sizeName = "byte ";
                break;
            case size_word:
                sizeName = "word ";
                break;
            case size_dword:
                sizeName = "dword ";
                break;
            case size_qword:
                sizeName = "qword ";
                break;
            }

#ifdef _WIN64
            if(arg.segment == SEG_GS)
                sizeName += "gs:";
#else //x32
            if(arg.segment == SEG_FS)
                sizeName += "fs:";
#endif

            if(bUpper)
                sizeName = sizeName.toUpper();

            if(!DbgMemIsValidReadPtr(arg.value))
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=???");
            else
            {
                QString addrText = getSymbolicName(arg.memvalue);
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + addrText);
            }
            j++;
        }
        else
        {
            auto symbolicName = getSymbolicName(arg.value);
            QString mnemonic(arg.mnemonic);
            bool ok;
            mnemonic.toULongLong(&ok, 16);
            if(ok) //skip certain numbers
            {
                if(ToHexString(arg.value) == symbolicName)
                    continue;
                setInfoLine(j, symbolicName);
            }
            else
                setInfoLine(j, mnemonic + "=" + symbolicName);
            j++;
        }
    }
    if(getInfoLine(0) == getInfoLine(1)) //check for duplicate info line
        setInfoLine(1, "");

    // Set last line
    //
    // Format: SECTION:VA MODULE:$RVA :#FILE_OFFSET FUNCTION,Accessed %u times
    QString info;

    // Section
    char section[MAX_SECTION_SIZE * 5];
    if(DbgFunctions()->SectionFromAddr(parVA, section))
        info += QString(section) + ":";

    // VA
    info += ToPtrString(parVA) + " ";

    // Module name, RVA, and file offset
    char mod[MAX_MODULE_SIZE];
    if(DbgFunctions()->ModNameFromAddr(parVA, mod, true))
    {
        dsint modbase = DbgFunctions()->ModBaseFromAddr(parVA);

        // Append modname
        info += mod;

        // Module RVA
        curRva = parVA - modbase;
        if(modbase)
            info += QString(":$%1 ").arg(ToHexString(curRva));

        // File offset
        curOffset = DbgFunctions()->VaToFileOffset(parVA);
        info += QString("#%1 ").arg(ToHexString(curOffset));
    }

    // Function/label name
    char label[MAX_LABEL_SIZE];
    if(DbgGetLabelAt(parVA, SEG_DEFAULT, label))
        info += QString("<%1>").arg(label);
    else
    {
        duint start;
        if(DbgFunctionGet(parVA, &start, nullptr) && DbgGetLabelAt(start, SEG_DEFAULT, label) && start != parVA)
            info += QString("<%1+%2>").arg(label).arg(ToHexString(parVA - start));
    }

    auto tracedCount = DbgFunctions()->GetTraceRecordHitCount(parVA);
    if(tracedCount != 0)
    {
        info += " , " + tr("Accessed %n time(s)", nullptr, tracedCount);
    }

    setInfoLine(2, info);

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

/**
 * @brief CPUInfoBox::addFollowMenuItem Add a follow action to the menu
 * @param menu The menu to which the follow action adds
 * @param name The user-friendly name of the action
 * @param value The VA of the address
 */
void CPUInfoBox::addFollowMenuItem(QMenu* menu, QString name, duint value)
{
    foreach(QAction * action, menu->actions()) //check for duplicate action
    if(action->text() == name)
        return;
    QAction* newAction = new QAction(name, menu);
    newAction->setFont(QFont("Courier New", 8));
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
    foreach(QAction * action, menu->actions()) //check for duplicate action
    if(action->text() == name)
        return;
    QAction* newAction = new QAction(name, menu);
    newAction->setFont(QFont("Courier New", 8));
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
    QMenu wWatchMenu(tr("&Watch"), this);
    setupWatchMenu(&wWatchMenu, curAddr);
    wMenu.addMenu(&wWatchMenu);
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
