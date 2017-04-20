#include "LocalVarsView.h"
#include "capstone_wrapper.h"
#include "CPUMultiDump.h"
#include "MiscUtil.h"
#include "WordEditDialog.h"

LocalVarsView::LocalVarsView(CPUMultiDump* parent) : StdTable(parent)
{
    currentFunc = 0;

    int charWidth = getCharWidth();
    addColumnAt(20 * charWidth, tr("Name"), false);
    addColumnAt(10 * charWidth, tr("Expression"), false); //[EBP + 10]
    addColumnAt(10 * charWidth, tr("Value"), false);
    loadColumnFromConfig("LocalVarsView");

    setupContextMenu();
    connect(Bridge::getBridge(), SIGNAL(updateWatch()), this, SLOT(updateSlot()));
    connect(this, SIGNAL(doubleClickedSignal()), this, SLOT(editSlot()));
}

void LocalVarsView::setupContextMenu()
{
    mMenu = new MenuBuilder(this, [](QMenu*)
    {
        return DbgIsDebugging();
    });
    mMenu->addAction(makeAction(tr("&Rename"), SLOT(renameSlot())));
    MenuBuilder* mBaseRegisters = new MenuBuilder(this);
#ifdef _WIN64
    baseRegisters[0] = new QAction("RAX", this);
    baseRegisters[1] = new QAction("RBX", this);
    baseRegisters[2] = new QAction("RCX", this);
    baseRegisters[3] = new QAction("RDX", this);
    baseRegisters[4] = new QAction("RBP", this);
    baseRegisters[5] = new QAction("RSP", this);
    baseRegisters[6] = new QAction("RSI", this);
    baseRegisters[7] = new QAction("RDI", this);
    baseRegisters[8] = new QAction("R8", this);
    baseRegisters[9] = new QAction("R9", this);
    baseRegisters[10] = new QAction("R10", this);
    baseRegisters[11] = new QAction("R11", this);
    baseRegisters[12] = new QAction("R12", this);
    baseRegisters[13] = new QAction("R13", this);
    baseRegisters[14] = new QAction("R14", this);
    baseRegisters[15] = new QAction("R15", this);
    for(char i = 0; i < 16; i++)
    {
        connect(baseRegisters[i], SIGNAL(triggered()), this, SLOT(baseChangedSlot()));
        baseRegisters[i]->setCheckable(true);
        mBaseRegisters->addAction(baseRegisters[i]);
    }
#else //x86
    baseRegisters[0] = new QAction("EAX", this);
    baseRegisters[1] = new QAction("EBX", this);
    baseRegisters[2] = new QAction("ECX", this);
    baseRegisters[3] = new QAction("EDX", this);
    baseRegisters[4] = new QAction("EBP", this);
    baseRegisters[5] = new QAction("ESP", this);
    baseRegisters[6] = new QAction("ESI", this);
    baseRegisters[7] = new QAction("EDI", this);
    for(char i = 0; i < 8; i++)
    {
        connect(baseRegisters[i], SIGNAL(triggered()), this, SLOT(baseChangedSlot()));
        baseRegisters[i]->setCheckable(true);
        mBaseRegisters->addAction(baseRegisters[i]);
    }
#endif //_WIN64
    baseRegisters[4]->setChecked(true); //CBP
    mMenu->addMenu(makeMenu(tr("Base Register")), mBaseRegisters);
    connect(this, SIGNAL(contextMenuSignal(QPoint)), this, SLOT(contextMenuSlot(QPoint)));
}

void LocalVarsView::contextMenuSlot(const QPoint & pos)
{
    QMenu wMenu(this);
    mMenu->build(&wMenu);
    wMenu.exec(mapToGlobal(pos));
}

void LocalVarsView::baseChangedSlot()
{
    currentFunc = 0;
    updateSlot();
}

void LocalVarsView::updateSlot()
{
    if(!DbgIsDebugging())
    {
        currentFunc = 0;
        setRowCount(0);
        reloadData();
        return;
    }
    REGDUMP z;
    memset(&z, 0, sizeof(REGDUMP));
    DbgGetRegDump(&z);
    duint start, end;

    if(DbgFunctionGet(z.regcontext.cip, &start, &end))
    {
        if(start != this->currentFunc) //needs analyzing
        {
            Capstone dis;
            unsigned char* buffer = new unsigned char[end - start + 16];
            if(!DbgMemRead(start, buffer, end - start + 16)) //failed to read memory for analyzing
            {
                delete buffer;
                setRowCount(0);
                return;
            }
            QSet<dsint> usedOffsets[ArchValue(8, 16)];
            duint address = start;
            while(address < end)
            {
                dis.Disassemble(address, buffer + address - start, end + 16 - address);
                if(dis.IsNop())
                {
                    address += dis.Size();
                    continue;
                }
                for(int i = 0; i < dis.OpCount(); i++)
                {
                    if(dis[i].type != X86_OP_MEM)
                        continue;
                    if(dis[i].mem.base == X86_REG_INVALID) //mov eax, [10000000], global variable
                        continue;
                    if(dis[i].mem.index != X86_REG_INVALID) //mov eax, dword ptr ds:[ebp+ecx*4-10], indexed array
                        continue;
                    const x86_reg registers[] =
                    {
#ifdef _WIN64
                        X86_REG_RAX, X86_REG_RBX, X86_REG_RCX, X86_REG_RDX, X86_REG_RBP, X86_REG_RSP, X86_REG_RSI, X86_REG_RDI, X86_REG_R8, X86_REG_R9, X86_REG_R10, X86_REG_R11, X86_REG_R12, X86_REG_R13, X86_REG_R14, X86_REG_R15
#else //x86
                        X86_REG_EAX, X86_REG_EBX, X86_REG_ECX, X86_REG_EDX, X86_REG_EBP, X86_REG_ESP, X86_REG_ESI, X86_REG_EDI
#endif //_WIN64
                    };
                    for(char j = 0; j < ArchValue(8, 16); j++)
                    {
                        if(!baseRegisters[j]->isChecked())
                            continue;
                        if(dis[i].mem.base == registers[j])
                            usedOffsets[j].insert(dis[i].mem.disp);
                    }
                }
                address += dis.Size();
            }
            delete buffer;
            int rows = 0;
            for(int i = 0; i < ArchValue(8, 16); i++)
                rows += usedOffsets[i].size();
            setRowCount(rows);
            rows = 0;
            for(int i = 0; i < ArchValue(8, 16); i++)
            {
                std::vector<dsint> sorted;
                sorted.reserve(usedOffsets[i].size());
                for(const auto & j : usedOffsets[i])
                    sorted.push_back(j);
                std::sort(sorted.begin(), sorted.end(), std::greater<dsint>());
                for(const auto & j : sorted)
                {
                    QString expr;
                    QString name;
                    if(i == 4) //CBP
                    {
                        if(j < 0)
                        {
                            expr = QString("-0x%1").arg(-j, 0, 16);
                            name = tr("Local%1").arg(-j / sizeof(dsint)); //EBP-C:Local3
                        }
                        else
                        {
                            expr = QString("+0x%1").arg(j, 0, 16);
                            name = tr("Arg%1").arg(j / sizeof(dsint) - 1); //EBP+C:Arg2
                        }
                        expr = QString(ArchValue("[EBP%1]", "[RBP%1]")).arg(expr);
                    }
                    else
                    {
                        if(j < 0)
                        {
                            expr = QString("-0x%1").arg(-j, 0, 16);
                            name = QString("_%1_%2").arg(-j, 0, 16); //ECX-C: _ECX_C
                        }
                        else
                        {
                            expr = QString("+0x%1").arg(j, 0, 16);
                            name = QString("%2_%1").arg(j, 0, 16); //ECX+C: ECX_C
                        }
                        name = name.arg(baseRegisters[i]->text());
                        expr = QString("[%1%2]").arg(baseRegisters[i]->text()).arg(expr);
                    }
                    setCellContent(rows, 0, name);
                    setCellContent(rows, 1, expr);
                    rows++;
                }
            } // Analyze finish
            this->currentFunc = start;
        }
        for(dsint i = 0; i < getRowCount(); i++)
        {
            duint val = 0;
            QByteArray buf = getCellContent(i, 1).toUtf8();
            if(DbgIsValidExpression(buf.constData()))
            {
                val = DbgValFromString(buf.constData());
                setCellContent(i, 2, ToPtrString(val));
            }
            else
                setCellContent(i, 2, tr("[Error]"));
        }
        reloadData();
    }
    else
    {
        currentFunc = 0;
        setRowCount(0);
        reloadData();
    }
}

void LocalVarsView::renameSlot()
{
    QString newName;
    if(getRowCount() > 0)
    {
        QString oldName = getCellContent(getInitialSelection(), 0);
        if(SimpleInputBox(this, tr("Rename local variable \"%1\"").arg(oldName), oldName, newName, oldName))
        {
            setCellContent(getInitialSelection(), 0, newName);
            reloadData();
        }
    }
}

void LocalVarsView::editSlot()
{
    if(getRowCount() <= 0)
        return;
    QString expr = getCellContent(getInitialSelection(), 1);
    if(!(expr.startsWith("[") && expr.endsWith("]")))
        return; //Unable to get address of the expression
    expr.truncate(expr.size() - 1);
    expr = expr.right(expr.size() - 1);
    QByteArray utf8 = expr.toUtf8();
    if(!DbgIsValidExpression(utf8.constData()))
        return;
    duint addr = DbgValFromString(utf8.constData());
    WordEditDialog editor(this);
    duint current = 0;
    if(!DbgMemRead(addr, &current, sizeof(duint)))
        return; //Memory is not accessible
    editor.setup(tr("Edit %1 at %2").arg(getCellContent(getInitialSelection(), 0)).arg(ToPtrString(addr)), current, sizeof(dsint));
    if(editor.exec() == QDialog::Accepted)
    {
        current = editor.getVal();
        DbgMemWrite(addr, &current, sizeof(duint));
        emit updateSlot();
    }
}
