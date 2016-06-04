#include "PatchDialog.h"
#include "ui_PatchDialog.h"
#include <QMessageBox>
#include <QIcon>
#include <QFileDialog>
#include <QTextStream>
#include <psapi.h>

PatchDialog::PatchDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PatchDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(false); //non-modal window

    connect(Bridge::getBridge(), SIGNAL(updatePatches()), this, SLOT(updatePatches()));
    connect(Bridge::getBridge(), SIGNAL(dbgStateChanged(DBGSTATE)), this, SLOT(dbgStateChanged(DBGSTATE)));

    mGroupSelector = new PatchDialogGroupSelector(parent);
    mGroupSelector->setGroup(0);
    connect(mGroupSelector, SIGNAL(rejected()), this, SLOT(showNormal()));
    connect(mGroupSelector, SIGNAL(rejected()), this, SLOT(setFocus()));
    connect(mGroupSelector, SIGNAL(groupToggle()), this, SLOT(groupToggle()));
    connect(mGroupSelector, SIGNAL(groupPrevious()), this, SLOT(groupPrevious()));
    connect(mGroupSelector, SIGNAL(groupNext()), this, SLOT(groupNext()));

    mIsWorking = false;
}

PatchDialog::~PatchDialog()
{
    delete ui;
}

bool PatchDialog::isPartOfPreviousGroup(const PatchInfoList & patchList, int index)
{
    if(!index)
        return true;
    uint addr = patchList.at(index).patch.addr;
    uint prevAddr = patchList.at(index - 1).patch.addr;
    for(int i = 1; i < 10; i++) //10 bytes in between groups
        if(addr - i == prevAddr)
            return true;
    return false;
}

bool PatchDialog::isGroupEnabled(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).status.group == group && !patchList.at(i).status.checked)
            return false;
    return true;
}

bool PatchDialog::hasPreviousGroup(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).status.group < group)
            return true;
    return false;
}

bool PatchDialog::hasNextGroup(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).status.group > group)
            return true;
    return false;
}

dsint PatchDialog::getGroupAddress(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).status.group == group)
            return patchList.at(i).patch.addr;
    return -1;
}

void PatchDialog::dbgStateChanged(DBGSTATE state)
{
    if(state == stopped)
    {
        mGroupSelector->hide();
        reject();
    }
}

void PatchDialog::updatePatches()
{
    if(this->isVisible())
        mGroupSelector->reject();
    mIsWorking = true;
    //clear GUI
    ui->listModules->clear();
    ui->listPatches->clear();
    mPatches.clear();

    //get patches from DBG
    size_t cbsize;
    if(!DbgFunctions()->PatchEnum || !DbgFunctions()->PatchEnum(0, &cbsize))
        return;
    int numPatches = (int)cbsize / sizeof(DBGPATCHINFO);
    if(!numPatches)
        return;
    DBGPATCHINFO* patches = new DBGPATCHINFO[numPatches];
    memset(patches, 0, numPatches * sizeof(DBGPATCHINFO));
    if(!DbgFunctions()->PatchEnum(patches, 0))
    {
        delete [] patches;
        mIsWorking = false;
        return;
    }

    //fill the patch list
    STATUSINFO defaultStatus;
    defaultStatus.group = 0;
    defaultStatus.checked = true;
    for(int i = 0; i < numPatches; i++)
    {
        if(!patches[i].addr)
            continue;
        if(!*patches[i].mod)
            continue;
        QString mod = patches[i].mod;
        PatchMap::iterator found = mPatches.find(mod);
        if(found != mPatches.end()) //found
            mPatches[mod].append(PatchPair(patches[i], defaultStatus));
        else //not found
        {
            PatchInfoList patchList;
            patchList.append(PatchPair(patches[i], defaultStatus));
            mPatches.insert(mod, patchList);
        }
    }
    delete [] patches;

    //sort the patches by address
    for(PatchMap::iterator i = mPatches.begin(); i != mPatches.end(); ++i)
    {
        qSort(i.value().begin(), i.value().end(), PatchInfoLess);
        PatchInfoList & curPatchList = i.value();
        //group the patched bytes
        for(int j = 0, group = 0; j < curPatchList.size(); j++)
        {
            if(!isPartOfPreviousGroup(curPatchList, j))
                group++;
            curPatchList[j].status.group = group;
            unsigned char byte;
            if(DbgMemRead(curPatchList[j].patch.addr, &byte, sizeof(byte)))
                curPatchList[j].status.checked = byte == curPatchList[j].patch.newbyte;
        }
        ui->listModules->addItem(i.key());
    }

    if(mPatches.size())
        ui->listModules->item(0)->setSelected(true); //select first module

    mIsWorking = false;
}

void PatchDialog::groupToggle()
{
    int group = mGroupSelector->group();
    if(mIsWorking || !ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    bool enabled = !isGroupEnabled(curPatchList, group);
    Qt::CheckState checkState = enabled ? Qt::Checked : Qt::Unchecked;
    mIsWorking = true;
    for(int i = 0; i < curPatchList.size(); i++)
    {
        if(curPatchList.at(i).status.group != group)
            continue;
        ui->listPatches->item(i)->setCheckState(checkState);
        curPatchList[i].status.checked = enabled;
        //change the byte to reflect the change for the user (cypherpunk reported this)
        unsigned char writebyte = curPatchList[i].status.checked ? curPatchList[i].patch.newbyte : curPatchList[i].patch.oldbyte;
        DbgMemWrite(curPatchList[i].patch.addr, &writebyte, sizeof(writebyte));
    }
    GuiUpdateAllViews();
    mIsWorking = false;
    dsint groupStart = getGroupAddress(curPatchList, group);
    if(!groupStart)
        return;
    QString color = enabled ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(groupStart, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::groupPrevious()
{
    int group = mGroupSelector->group();
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!hasPreviousGroup(curPatchList, group))
        return;
    group--;
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setGroup(group);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->showNormal();
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::groupNext()
{
    int group = mGroupSelector->group();
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!hasNextGroup(curPatchList, group))
        return;
    group++;
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setGroup(group);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->showNormal();
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::on_listModules_itemSelectionChanged()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod(ui->listModules->selectedItems().at(0)->text());
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & patchList = found.value();
    ui->listPatches->clear();
    for(int i = 0; i < patchList.size(); i++)
    {
        const DBGPATCHINFO curPatch = patchList.at(i).patch;
        QString addrText = QString("%1").arg(curPatch.addr, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
        QListWidgetItem* item = new QListWidgetItem(QString().sprintf("%d", patchList.at(i).status.group).rightJustified(4, ' ', true) + "|" + addrText + QString().sprintf(":%.2X->%.2X", curPatch.oldbyte, curPatch.newbyte), ui->listPatches);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        Qt::CheckState state = patchList.at(i).status.checked ? Qt::Checked : Qt::Unchecked;
        item->setCheckState(state);
    }
    mIsWorking = false;
}

void PatchDialog::on_listPatches_itemChanged(QListWidgetItem* item) //checkbox changed
{
    if(mIsWorking || !ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    bool checked = item->checkState() == Qt::Checked;
    PatchInfoList & curPatchList = found.value();
    PatchPair & patch = curPatchList[ui->listPatches->row(item)];
    if(patch.status.checked == checked) //check state did not change
        return;
    patch.status.checked = checked;
    //change the byte to reflect the change for the user (cypherpunk reported this)
    unsigned char writebyte = patch.status.checked ? patch.patch.newbyte : patch.patch.oldbyte;
    DbgMemWrite(patch.patch.addr, &writebyte, sizeof(writebyte));
    //check state changed
    if((QApplication::keyboardModifiers() & Qt::ControlModifier) != Qt::ControlModifier)
    {
        mIsWorking = true;
        //check/uncheck the complete group
        for(int i = 0; i < curPatchList.size(); i++)
            if(curPatchList.at(i).status.group == patch.status.group)
            {
                //change the patch state
                curPatchList[i].status.checked = checked;
                ui->listPatches->item(i)->setCheckState(item->checkState());
                //change the byte to reflect the change for the user (cypherpunk reported this)
                unsigned char writebyte = curPatchList[i].status.checked ? curPatchList[i].patch.newbyte : curPatchList[i].patch.oldbyte;
                DbgMemWrite(curPatchList[i].patch.addr, &writebyte, sizeof(writebyte));
            }
        mIsWorking = false;
    }
    GuiUpdateAllViews();
    int group = mGroupSelector->group();
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
}

void PatchDialog::on_btnSelectAll_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    for(int i = 0; i < curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Checked);
        curPatchList[i].status.checked = true;
        //change the byte to reflect the change for the user (cypherpunk reported this)
        DbgMemWrite(curPatchList[i].patch.addr, &curPatchList[i].patch.newbyte, sizeof(unsigned char));
    }
    GuiUpdateAllViews();
    mIsWorking = false;
}

void PatchDialog::on_btnDeselectAll_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    for(int i = 0; i < curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Unchecked);
        curPatchList[i].status.checked = false;
        //change the byte to reflect the change for the user (cypherpunk reported this)
        DbgMemWrite(curPatchList[i].patch.addr, &curPatchList[i].patch.oldbyte, sizeof(unsigned char));
    }
    GuiUpdateAllViews();
    mIsWorking = false;
}

void PatchDialog::on_btnRestoreSelected_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    int selModIdx = ui->listModules->row(ui->listModules->selectedItems().at(0));
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    int removed = 0;
    int total = curPatchList.size();
    for(int i = 0; i < total; i++)
    {
        if(curPatchList.at(i).status.checked)
        {
            DbgFunctions()->PatchRestore(curPatchList.at(i).patch.addr);
            removed++;
        }
    }
    mIsWorking = false;
    updatePatches();
    if(removed != total)
        ui->listModules->setCurrentRow(selModIdx);
    GuiUpdateAllViews();
}

void PatchDialog::on_listPatches_itemSelectionChanged()
{
    if(!ui->listModules->selectedItems().size() || !ui->listPatches->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    PatchPair & patch = curPatchList[ui->listPatches->row(ui->listPatches->selectedItems().at(0))]; //selected item
    dsint groupStart = getGroupAddress(curPatchList, patch.status.group);
    if(!groupStart)
        return;
    QString addrText = QString("%1").arg(groupStart, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::on_btnPickGroups_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!curPatchList.size())
        return;
    this->showMinimized();

    int group = mGroupSelector->group();
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(dsint) * 2, 16, QChar('0')).toUpper();
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->show();
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::on_btnPatchFile_clicked()
{
    //get current module
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches.find(mod);
    if(found == mPatches.end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();

    //get patches to save
    QList<DBGPATCHINFO> patchList;
    for(int i = 0; i < curPatchList.size(); i++)
        if(curPatchList.at(i).status.checked)
            patchList.push_back(curPatchList.at(i).patch);
    if(!curPatchList.size() || !patchList.size())
    {
        QMessageBox msg(QMessageBox::Information, tr("Information"), tr("Nothing to patch!"));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    char szModName[MAX_PATH] = "";
    if(!DbgFunctions()->ModPathFromAddr(DbgFunctions()->ModBaseFromName(mod.toUtf8().constData()), szModName, MAX_PATH))
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("Failed to get module filename..."));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    //open the save file dialog
    int len = (int)strlen(szModName);
    while(szModName[len] != '\\')
        len--;
    char szDirName[MAX_PATH] = "";
    strcpy_s(szDirName, szModName);
    szDirName[len] = '\0';

    QString filename = QFileDialog::getSaveFileName(this, tr("Save file"), szDirName, tr("All files (*.*)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)

    //call patchSave function
    DBGPATCHINFO* dbgPatchList = new DBGPATCHINFO[patchList.size()];
    for(int i = 0; i < patchList.size(); i++)
        dbgPatchList[i] = patchList.at(i);
    char error[MAX_ERROR_SIZE] = "";
    int patched = DbgFunctions()->PatchFile(dbgPatchList, patchList.size(), filename.toUtf8().constData(), error);
    delete [] dbgPatchList;
    if(patched == -1)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("Failed to save patched file (%1)").arg(error));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Information, tr("Information"), tr("%1/%2 patch(es) applied!").arg(patched).arg(patchList.size()));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void PatchDialog::on_btnImport_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Open patch"), "", tr("Patch files (*.1337)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&file);
    QString patch = in.readAll();
    file.close();
    patch = patch.replace("\r\n", "\n");
    QStringList lines = patch.split("\n", QString::SkipEmptyParts);
    if(!lines.size())
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("The patch file is empty..."));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    typedef struct _IMPORTSTATUS
    {
        bool badoriginal;
        bool alreadypatched;
    } IMPORTSTATUS;
    QList<QPair<DBGPATCHINFO, IMPORTSTATUS>> patchList;
    DBGPATCHINFO curPatch;
    dsint modbase = 0;
    bool bBadOriginal = false;
    bool bAlreadyDone = false;
    for(int i = 0; i < lines.size(); i++)
    {
        ULONGLONG rva;
        unsigned int oldbyte;
        unsigned int newbyte;
        QString curLine = lines.at(i);
        if(curLine.startsWith(">")) //module
        {
            strcpy_s(curPatch.mod, curLine.toUtf8().constData() + 1);
            modbase = DbgFunctions()->ModBaseFromName(curPatch.mod);
            continue;
        }
        if(!modbase)
            continue;
        curLine = curLine.replace(" ", "");
        if(sscanf_s(curLine.toUtf8().constData(), "%llX:%X->%X", &rva, &oldbyte, &newbyte) != 3)
        {
            QMessageBox msg(QMessageBox::Critical, tr("Error!"), tr("Patch file format is incorrect..."));
            msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
            msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
            msg.exec();
            return;
        }
        oldbyte &= 0xFF;
        newbyte &= 0xFF;
        curPatch.addr = rva + modbase;
        if(!DbgMemIsValidReadPtr(curPatch.addr))
            continue;
        unsigned char checkbyte = 0;
        DbgMemRead(curPatch.addr, &checkbyte, sizeof(checkbyte));
        IMPORTSTATUS status;
        status.alreadypatched = (checkbyte == newbyte);
        status.badoriginal = (checkbyte != oldbyte);
        if(status.alreadypatched)
            bAlreadyDone = true;
        else if(status.badoriginal)
            bBadOriginal = true;
        curPatch.oldbyte = oldbyte;
        curPatch.newbyte = newbyte;
        patchList.push_back(QPair<DBGPATCHINFO, IMPORTSTATUS>(curPatch, status));
    }

    if(!patchList.size())
    {
        QMessageBox msg(QMessageBox::Information, tr("Information"), tr("No patches to apply in the current process."));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    bool bUndoPatched = false;
    if(bAlreadyDone)
    {
        QMessageBox msg(QMessageBox::Question, tr("Question"), tr("Some patches are already applied.\n\nDo you want to remove these patches?"), QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bUndoPatched = true;
    }

    bool bPatchBadOriginals = false;
    if(bBadOriginal)
    {
        QMessageBox msg(QMessageBox::Question, tr("Question"), tr("Some bytes do not match the original in the patch file.\n\nDo you want to apply these patches anyway?"), QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bPatchBadOriginals = true;
    }

    int patched = 0;
    for(int i = 0; i < patchList.size(); i++)
    {
        if(!bPatchBadOriginals && patchList.at(i).second.badoriginal)
            continue;
        curPatch = patchList.at(i).first;
        if(bUndoPatched && patchList.at(i).second.alreadypatched)
        {
            if(DbgFunctions()->MemPatch(curPatch.addr, &curPatch.oldbyte, 1))
                patched++;
        }
        else
        {
            if(DbgFunctions()->MemPatch(curPatch.addr, &curPatch.newbyte, 1))
                patched++;
        }
    }

    updatePatches();
    GuiUpdateAllViews();

    QMessageBox msg(QMessageBox::Information, tr("Information"), tr("%1/%2 patch(es) applied!").arg(patched).arg(patchList.size()));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void PatchDialog::on_btnExport_clicked()
{
    if(!mPatches.size())
        return;

    QString filename = QFileDialog::getSaveFileName(this, tr("Save patch"), "", tr("Patch files (*.1337);;C program source (*.c)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    if(filename.endsWith(QString(".1337")))
        saveAs1337(filename);
    if(filename.endsWith(QString(".c")))
        saveAsC(filename);
}

void PatchDialog::saveAs1337(const QString & filename)
{
    QStringList lines;

    duint patches = 0;
    for(PatchMap::iterator i = mPatches.begin(); i != mPatches.end(); ++i)
    {
        const PatchInfoList & curPatchList = i.value();
        bool bModPlaced = false;
        duint modbase = DbgFunctions()->ModBaseFromName(i.key().toUtf8().constData());
        if(!modbase)
            continue;
        for(int j = 0; j < curPatchList.size(); j++)
        {
            if(!curPatchList.at(j).status.checked) //skip unchecked patches
                continue;
            if(!bModPlaced)
            {
                lines.push_back(">" + i.key());
                bModPlaced = true;
            }
            QString addrText = QString("%1").arg(curPatchList.at(j).patch.addr - modbase, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            lines.push_back(addrText + QString().sprintf(":%.2X->%.2X", curPatchList.at(j).patch.oldbyte, curPatchList.at(j).patch.newbyte));
            patches++;
        }
    }

    if(!lines.size())
    {
        QMessageBox msg(QMessageBox::Information, tr("Information"), tr("No patches to export."));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QFile file(filename);
    file.open(QFile::WriteOnly | QFile::Text);
    if(file.error() != QFile::NoError)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("Cannot export patches because an error occurs during file I/O"));
        msg.exec();
        return;
    }
    QString text = lines.join("\r\n");
    QByteArray utf8Data = text.toUtf8();
    file.write(utf8Data.constData(), utf8Data.length());
    file.close();

    QMessageBox msg(QMessageBox::Information, tr("Information"), tr("%1 patch(es) exported!").arg(patches));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void PatchDialog::saveAsC(const QString &filename)
{
    QString template_C;
    // Loading template
    template_C = QString(R"xxx(
This template exports patch as C program source.
$TEMPLATE_PREFIX:
/********************************************
 *         x64dbg generated patch           *
 ********************************************/

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <psapi.h>

HANDLE findProcess();
void applyPatch(HANDLE hProcess, const wchar_t* moduleName, char* baseAddress);

int main()
{
    HMODULE hMods[2048];
    DWORD cbNeeded;
    unsigned int i;
    HANDLE hProcess;
    printf("Patch for $exeName\n");
    printf("This patch is generated by x64dbg(http://x64dbg.org) at $date.\n");
    printf("Start patching...\n");
    hProcess = findProcess();
    if(hProcess == NULL)
    {
        printf("Finished, but the target process is not found. No patch is applied.\n");
        return 0;
    }
    printf("Apply $numPatches patches.\n");
    if(EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded) && cbNeeded < sizeof(hMods))
    {
        for(i = 0; i < cbNeeded / sizeof(HMODULE); i++)
        {
            wchar_t modName[MAX_PATH];
            if(GetModuleBaseNameW(hProcess, hMods[i], modName, sizeof(modName) / sizeof(char)))
            {
                applyPatch(hProcess, modName, (char*)hMods[i]);
            }
        }
    }
    else
    {
        printf("Patching failed because EnumProcessModules() failed.");
        return 1;
    }
    printf("Patching complete!\n");
    CloseHandle(hProcess);
    return 0;
}

HANDLE findProcess()
{

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if(!hProcessSnap)
    {
        printf("CreateToolhelp32Snapshot() failed.\n");
        return NULL;
    }
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);
    if(!Process32First(hProcessSnap, &pe32))
    {
        printf("Process32First() failed.\n");
        CloseHandle(hProcessSnap);
        return NULL;
    }
    do
    {
        if(pe32.th32ProcessID == GetCurrentProcessId())
            continue;
        if(pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4) // System process and Idle process have special PID.
            continue;
        if(_wcsicmp(pe32.szExeFile, L"$exeName") == 0)
        {
            printf("Process $exeName has been found. PID = %d\n", pe32.th32ProcessID);
            HANDLE hProcess;
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pe32.th32ProcessID);
            if(hProcess == NULL)
            {
                printf("Cannot open process %d. continue.\n", pe32.th32ProcessID);
                continue;
            }
            return hProcess;
        }
    }while(Process32Next(hProcessSnap, &pe32));
    return NULL;
}

void applyPatch(HANDLE hProcess, const wchar_t* modName, char* baseAddr)
{
    size_t written;
    char buffer;
$MODULE_PREFIX:
    if(_wcsicmp(modName, L"$moduleName") == 0)
    {
        printf("Patching $moduleName at %p\n", baseAddr);
$PATCH:
        buffer = 0x$newByte;
        if(WriteProcessMemory(hProcess, baseAddr + 0x$rva, &buffer, 1, &written) == FALSE)
            printf("Patch $patchIndex/$numPatches at %p failed.\n", baseAddr + 0x$rva);
$MODULE_SUFFIX:
    }
$TEMPLATE_SUFFIX:
}
)xxx");
    // Parse template
    int idx_template = template_C.indexOf("$TEMPLATE_PREFIX:");
    int idx_module = template_C.indexOf("$MODULE_PREFIX:");
    int idx_patch = template_C.indexOf("$PATCH:");
    int idx_module_suffix = template_C.indexOf("$MODULE_SUFFIX:");
    int idx_template_suffix = template_C.indexOf("$TEMPLATE_SUFFIX:");
    if(idx_template < 0 || idx_module < 0 || idx_patch < 0 || idx_module_suffix < 0 || idx_template_suffix < 0)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("Cannot export patches because the template is malformed"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    HANDLE hProcess;
    wchar_t ProcessName[1024];
    hProcess = (HANDLE)DbgValFromString("$hProcess");
    if(GetModuleBaseNameW(hProcess, 0, ProcessName, sizeof(ProcessName) / sizeof(wchar_t)) == 0)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("Cannot export patches because x64dbg cannot get the process name."));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QString text = template_C.mid(idx_template + int(strlen("$TEMPLATE_PREFIX:")), idx_module - idx_template - int(strlen("$TEMPLATE_PREFIX:")));
    QString modulePrefix = template_C.mid(idx_module + int(strlen("$MODULE_PREFIX:")), idx_patch - idx_module - int(strlen("$MODULE_PREFIX:")));
    QString patchText = template_C.mid(idx_patch + int(strlen("$PATCH:")), idx_module_suffix - idx_patch - int(strlen("$PATCH:")));
    QString moduleSuffix = template_C.mid(idx_module_suffix + int(strlen("$MODULE_SUFFIX:")), idx_template_suffix - idx_module_suffix - int(strlen("$MODULE_SUFFIX:")));
    QString templateSuffix = template_C.mid(idx_template_suffix + int(strlen("$TEMPLATE_SUFFIX:")));
    QStringList lines;
    duint patches = 0;
    for(PatchMap::iterator i = mPatches.begin(); i != mPatches.end(); ++i)
    {
        const PatchInfoList & curPatchList = i.value();
        bool bModPlaced = false;
        duint modbase = DbgFunctions()->ModBaseFromName(i.key().toUtf8().constData());
        if(!modbase)
            continue;
        for(int j = 0; j < curPatchList.size(); j++)
        {
            if(!curPatchList.at(j).status.checked) //skip unchecked patches
                continue;
            if(!bModPlaced)
            {
                QString modulePrefix2(modulePrefix);
                modulePrefix2.replace("$moduleName", i.key());
                lines.push_back(modulePrefix2);
                bModPlaced = true;
            }
            patches++;
            QString addrText = QString("%1").arg(curPatchList.at(j).patch.addr - modbase, sizeof(dsint) * 2, 16, QChar('0')).toUpper();
            QString patchText2(patchText);
            QString newByteText(QString().sprintf("%.2X", curPatchList[j].patch.newbyte));
            patchText2.replace("$rva", addrText);
            patchText2.replace("$newByte", newByteText);
            patchText2.replace("$patchIndex", QString::number(patches));
            lines.push_back(patchText2);
        }
        if(bModPlaced)
        {
            QString moduleSuffix2(moduleSuffix);
            moduleSuffix2.replace("$moduleName", i.key());
            lines.push_back(moduleSuffix2);
        }
    }
    if(patches == 0)
    {
        QMessageBox msg(QMessageBox::Information, tr("Information"), tr("No patches to export."));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    text.append(lines.join("\r\n"));
    text.append(templateSuffix);
    text.replace(QString("$exeName"), QString::fromUtf16((const ushort*)ProcessName));
    text.replace(QString("$date"), QDateTime::currentDateTime().toString());
    text.replace(QString("$numPatches"), QString::number(patches));

    QFile file(filename);
    file.open(QFile::WriteOnly | QFile::Text);
    if(file.error() != QFile::NoError)
    {
        QMessageBox msg(QMessageBox::Critical, tr("Error"), tr("Cannot export patches because an error occurs during file I/O"));
        msg.exec();
        return;
    }
    QByteArray utf8Data = text.toUtf8();
    file.write(utf8Data.constData(), utf8Data.length());
    file.close();

    QMessageBox msg(QMessageBox::Information, tr("Information"), tr("%1 patch(es) exported!").arg(patches));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}
