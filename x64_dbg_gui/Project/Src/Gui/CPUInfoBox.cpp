#include "CPUInfoBox.h"
#include "Configuration.h"
#include "Bridge.h"

CPUInfoBox::CPUInfoBox(StdTable* parent) : StdTable(parent)
{
    enableMultiSelection(false);
    setShowHeader(false);
    addColumnAt(0, "", true);
    setRowCount(3);
    setCellContent(0, 0, "");
    setCellContent(1, 0, "");
    setCellContent(2, 0, "");
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal{border:1px solid grey;background:#f1f1f1;height:10px}QScrollBar::handle:horizontal{background:#aaa;min-width:20px;margin:1px}QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;height:0}");

    int height = getHeight();
    setMaximumHeight(height);
    setMinimumHeight(height);
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
    curAddr = 0;
}

int CPUInfoBox::getHeight()
{
    return ((getRowHeight() + 1) * 3) + 10;
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
        return QString();
    return getCellContent(line, 0);
}

void CPUInfoBox::clear()
{
    setInfoLine(0, "");
    setInfoLine(1, "");
    setInfoLine(2, "");
}

QString CPUInfoBox::getSymbolicName(int_t addr)
{
    char labelText[MAX_LABEL_SIZE] = "";
    char moduleText[MAX_MODULE_SIZE] = "";
    char string[MAX_STRING_SIZE] = "";
    bool bHasString = DbgGetStringAt(addr, string);
    bool bHasLabel = DbgGetLabelAt(addr, SEG_DEFAULT, labelText);
    bool bHasModule = (DbgGetModuleAt(addr, moduleText) && !QString(labelText).startsWith("JMP.&"));
    QString addrText;
    addrText = QString("%1").arg(addr & (uint_t) - 1, 0, 16, QChar('0')).toUpper();
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
            QChar c = QChar::fromLatin1((char)addr);
            if(c.isPrint())
                finalText += QString(" '%1'").arg((char)addr);
        }
        else if(addr == (addr & 0xFFF)) //UNICODE?
        {
            QChar c = QChar::fromLatin1((wchar_t)addr);
            if(c.isPrint())
                finalText += " L'" + QString(c) + "'";
        }
    }
    return finalText;
}

void CPUInfoBox::disasmSelectionChanged(int_t parVA)
{
    curAddr = parVA;
    if(!DbgIsDebugging() || !DbgMemIsValidReadPtr(parVA))
        return;
    clear();

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
            setInfoLine(0, "Jump is taken");
        else
            setInfoLine(0, "Jump is not taken");
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

            if(bUpper)
                sizeName = sizeName.toUpper();

            if(!DbgMemIsValidReadPtr(arg.value))
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=???");
            else
            {
                QString addrText;
                if(memsize == sizeof(int_t))
                    addrText = getSymbolicName(arg.memvalue);
                else
                    addrText = QString("%1").arg(arg.memvalue, memsize * 2, 16, QChar('0')).toUpper();
                setInfoLine(j, sizeName + "[" + argMnemonic + "]=" + addrText);
            }
            j++;
        }
        else
        {
            QString mnemonic(arg.mnemonic);
            bool ok;
            mnemonic.toULongLong(&ok, 16);
            if(ok) //skip numbers
                continue;
            setInfoLine(j, mnemonic + "=" + getSymbolicName(arg.value));
            j++;
        }
    }
    if(getInfoLine(0) == getInfoLine(1)) //check for duplicate info line
        setInfoLine(1, "");
    //set last line
    QString info;
    char mod[MAX_MODULE_SIZE] = "";
    if(DbgFunctions()->ModNameFromAddr(parVA, mod, true))
    {
        int_t modbase = DbgFunctions()->ModBaseFromAddr(parVA);
        if(modbase)
            info = QString(mod) + "[" + QString("%1").arg(parVA - modbase, 0, 16, QChar('0')).toUpper() + "] | ";
        else
            info = QString(mod) + " | ";
    }
    char section[10] = "";
    if(DbgFunctions()->SectionFromAddr(parVA, section))
        info += "\"" + QString(section) + "\":";
    info += QString("%1").arg(parVA, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    char label[MAX_LABEL_SIZE] = "";
    if(DbgGetLabelAt(parVA, SEG_DEFAULT, label))
        info += " <" + QString(label) + ">";
    setInfoLine(2, info);
}

void CPUInfoBox::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
        clear();
}

void CPUInfoBox::followActionSlot()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if(action && action->objectName().startsWith("DUMP|"))
        DbgCmdExec(QString().sprintf("dump \"%s\"", action->objectName().mid(5).toUtf8().constData()).toUtf8().constData());
}

void CPUInfoBox::addFollowMenuItem(QMenu* menu, QString name, int_t value)
{
    foreach(QAction * action, menu->actions()) //check for duplicate action
    if(action->text() == name)
        return;
    QAction* newAction = new QAction(name, this);
    newAction->setFont(QFont("Courier New", 8));
    menu->addAction(newAction);
    newAction->setObjectName(QString("DUMP|") + QString("%1").arg(value, sizeof(int_t) * 2, 16, QChar('0')).toUpper());
    connect(newAction, SIGNAL(triggered()), this, SLOT(followActionSlot()));
}

void CPUInfoBox::setupFollowMenu(QMenu* menu, int_t wVA)
{
    //most basic follow action
    addFollowMenuItem(menu, "&Selection", wVA);

    //add follow actions
    DISASM_INSTR instr;
    DbgDisasmAt(wVA, &instr);

    for(int i = 0; i < instr.argcount; i++)
    {
        const DISASM_ARG arg = instr.arg[i];
        if(arg.type == arg_memory)
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem(menu, "&Address: " + QString(arg.mnemonic).toUpper().trimmed(), arg.value);
            if(arg.value != arg.constant)
            {
                QString constant = QString("%1").arg(arg.constant, 1, 16, QChar('0')).toUpper();
                if(DbgMemIsValidReadPtr(arg.constant))
                    addFollowMenuItem(menu, "&Constant: " + constant, arg.constant);
            }
            if(DbgMemIsValidReadPtr(arg.memvalue))
                addFollowMenuItem(menu, "&Value: [" + QString(arg.mnemonic) + "]", arg.memvalue);
        }
        else
        {
            if(DbgMemIsValidReadPtr(arg.value))
                addFollowMenuItem(menu, QString(arg.mnemonic).toUpper().trimmed(), arg.value);
        }
    }
}

void CPUInfoBox::contextMenuSlot(QPoint pos)
{
    QMenu* wMenu = new QMenu(this); //create context menu
    QMenu* wFollowMenu = new QMenu("&Follow in Dump", this);
    setupFollowMenu(wFollowMenu, curAddr);
    wMenu->addMenu(wFollowMenu);
    QMenu wCopyMenu("&Copy", this);
    setupCopyMenu(&wCopyMenu);
    if(wCopyMenu.actions().length())
    {
        wMenu->addSeparator();
        wMenu->addMenu(&wCopyMenu);
    }
    wMenu->exec(mapToGlobal(pos)); //execute context menu
}
