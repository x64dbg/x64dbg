#include "TraceXrefBrowseDialog.h"
#include "ui_XrefBrowseDialog.h"
#include "TraceFileReader.h"
#include "TraceFileDump.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "MenuBuilder.h"

TraceXrefBrowseDialog::TraceXrefBrowseDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::XrefBrowseDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowIcon(DIcon("xrefs"));
    setModal(false);
    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    setupContextMenu();
}

QString TraceXrefBrowseDialog::GetFunctionSymbol(duint addr)
{
    QString line;
    char clabel[MAX_LABEL_SIZE] = "";

    DbgGetLabelAt(addr, SEG_DEFAULT, clabel);
    if(*clabel)
        line = QString(clabel);
    else
    {
        duint start;
        if(DbgFunctionGet(addr, &start, nullptr) && DbgGetLabelAt(start, SEG_DEFAULT, clabel) && start != addr)
            line = QString("%1+%2").arg(clabel).arg(ToHexString(addr - start));
        else
            line = QString("%1").arg(ToHexString(addr));
    }

    return line;
}

void TraceXrefBrowseDialog::setup(duint index, duint address, TraceFileReader* traceFile, GotoFunction gotoFunction)
{
    auto dump = traceFile->getDump();
    mXrefInfo.clear();
    mAddress = index;
    mGotoFunction = std::move(gotoFunction);
    mPrevSelectionSize = 0;
    ui->listWidget->clear();
    auto xrefInfo = dump->getReferences(address, address);
    mXrefInfo.reserve(xrefInfo.size());
    for(auto & i : xrefInfo)
    {
        mXrefInfo.emplace_back(TRACE_XREF_RECORD({i, traceFile->Registers(i).regcontext.cip}));
    }

    setWindowTitle(QString(tr("xrefs at <%1>")).arg(GetFunctionSymbol(address)));
    for(size_t i = 0; i < mXrefInfo.size(); i++)
    {
        Zydis zydis;
        QString disasm;
        unsigned char data[16] = { 0xCC };
        int size;
        traceFile->OpCode(mXrefInfo[i].index, data, &size);
        zydis.Disassemble(mXrefInfo[i].addr, data);
        if(zydis.Success())
        {
            disasm = QString::fromStdString(zydis.InstructionText());
        }
        else
        {
            disasm = "???";
        }
        disasm = traceFile->getIndexText(mXrefInfo[i].index) + ": " + disasm;
        ui->listWidget->addItem(disasm);
    }
    ui->listWidget->setCurrentRow(0);
    ui->listWidget->setFocus();
}

void TraceXrefBrowseDialog::setupContextMenu()
{
    mMenu = new MenuBuilder(this);
    auto mCopyMenu = new MenuBuilder(mMenu);
    mCopyMenu->addAction(makeAction(tr("Selected xref"), SLOT(copyThisSlot())));
    mCopyMenu->addAction(makeAction(tr("All xrefs"), SLOT(copyAllSlot())));
    mMenu->addMenu(makeMenu(DIcon("copy"), tr("Copy")), mCopyMenu);
    mMenu->loadFromConfig();
}

void TraceXrefBrowseDialog::changeAddress(duint address)
{
    mGotoFunction(address);
}

TraceXrefBrowseDialog::~TraceXrefBrowseDialog()
{
    delete ui;
}

void TraceXrefBrowseDialog::on_listWidget_itemDoubleClicked(QListWidgetItem*)
{
    accept();
}

void TraceXrefBrowseDialog::on_listWidget_itemSelectionChanged()
{
    if(ui->listWidget->selectedItems().size() != mPrevSelectionSize)
    {
        duint address;
        if(mPrevSelectionSize == 0)
            address = mXrefInfo[ui->listWidget->currentRow()].index;
        else
            address = mAddress;

        changeAddress(address);
    }
    mPrevSelectionSize = ui->listWidget->selectedItems().size();
}

void TraceXrefBrowseDialog::on_listWidget_currentRowChanged(int row)
{
    if(ui->listWidget->selectedItems().size() != 0)
    {
        duint address = mXrefInfo[row].index;
        changeAddress(address);
    }
}

void TraceXrefBrowseDialog::on_XrefBrowseDialog_rejected()
{
    mGotoFunction(mAddress);
}

void TraceXrefBrowseDialog::on_listWidget_itemClicked(QListWidgetItem*)
{
    on_listWidget_currentRowChanged(ui->listWidget->currentRow());
}

void TraceXrefBrowseDialog::on_listWidget_customContextMenuRequested(const QPoint & pos)
{
    QMenu menu(this);
    mMenu->build(&menu);
    menu.exec(ui->listWidget->mapToGlobal(pos));
}

void TraceXrefBrowseDialog::copyThisSlot()
{
    Bridge::CopyToClipboard(ToHexString(mXrefInfo[ui->listWidget->currentRow()].index) + " " + ToPtrString(mXrefInfo[ui->listWidget->currentRow()].addr) + " " + ui->listWidget->selectedItems()[0]->text());
}

void TraceXrefBrowseDialog::copyAllSlot()
{
    QString temp;
    for(int i = 0; i < ui->listWidget->count(); i++)
    {
        temp.append(ToHexString(mXrefInfo[i].index) + " ");
        temp.append(ToPtrString(mXrefInfo[i].addr) + " ");
        temp.append(ui->listWidget->selectedItems()[0]->text());
        temp.append("\r\n");
    }
    Bridge::CopyToClipboard(temp);
}
