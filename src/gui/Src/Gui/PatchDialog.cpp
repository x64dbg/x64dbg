#include "PatchDialog.h"
#include "ui_PatchDialog.h"
#include <QMessageBox>
#include <QIcon>
#include <QFileDialog>
#include <QTextStream>
#include "MiscUtil.h"
#include "StringUtil.h"
#include "Configuration.h"

PatchDialog::PatchDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PatchDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
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
    mIsWorking = false;
    dsint groupStart = getGroupAddress(curPatchList, group);
    GuiUpdateAllViews();
    if(!groupStart)
        return;
    QString color = enabled ? "#00DD00" : "red";
    QString addrText = ToPtrString(groupStart);
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
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
    QString addrText = ToPtrString(getGroupAddress(curPatchList, group));
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setGroup(group);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->showNormal();
    DbgCmdExecDirect(QString("disasm " + addrText));
    DbgCmdExecDirect(QString("dump " + addrText));
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
    QString addrText = ToPtrString(getGroupAddress(curPatchList, group));
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setGroup(group);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->showNormal();
    DbgCmdExecDirect(QString("disasm " + addrText));
    DbgCmdExecDirect(QString("dump " + addrText));
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
        QString addrText = ToPtrString(curPatch.addr);
        QListWidgetItem* item = new QListWidgetItem(QString().sprintf("%d", patchList.at(i).status.group).rightJustified(4, ' ', true) + "|" + addrText + QString().sprintf(":%.2X->%.2X", curPatch.oldbyte, curPatch.newbyte), ui->listPatches);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        Qt::CheckState state = patchList.at(i).status.checked ? Qt::Checked : Qt::Unchecked;
        item->setCheckState(state);
        if(DbgFunctions()->ModRelocationAtAddr(patchList.at(i).patch.addr, nullptr))
        {
            item->setTextColor(ConfigColor("PatchRelocatedByteHighlightColor"));
            item->setToolTip(tr("Byte is located in relocation region"));
        }
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
    int group = mGroupSelector->group();
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = ToPtrString(getGroupAddress(curPatchList, group));
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    GuiUpdateAllViews();
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
    QString addrText = ToPtrString(groupStart);
    DbgCmdExecDirect(QString("disasm " + addrText));
    DbgCmdExecDirect(QString("dump " + addrText));
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
    QString addrText = ToPtrString(getGroupAddress(curPatchList, group));
    QString title = "<font color='" + color + "'><b>" + QString().sprintf("%d:", group) + addrText + "</b></font>";
    mGroupSelector->setGroupTitle(title);
    mGroupSelector->setPreviousEnabled(hasPreviousGroup(curPatchList, group));
    mGroupSelector->setNextEnabled(hasNextGroup(curPatchList, group));
    mGroupSelector->show();
    DbgCmdExecDirect(QString("disasm " + addrText));
    DbgCmdExecDirect(QString("dump " + addrText));
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
        SimpleInfoBox(this, tr("Information"), tr("Nothing to patch!"));
        return;
    }

    if(containsRelocatedBytes(curPatchList) && !showRelocatedBytesWarning())
        return;

    char szModName[MAX_PATH] = "";
    if(!DbgFunctions()->ModPathFromAddr(DbgFunctions()->ModBaseFromName(mod.toUtf8().constData()), szModName, MAX_PATH))
    {
        SimpleErrorBox(this, tr("Error!"), tr("Failed to get module filename..."));
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
        SimpleErrorBox(this, tr("Error!"), tr("Failed to save patched file (%1)").arg(error));
        return;
    }
    SimpleInfoBox(this, tr("Information"), tr("%1/%2 patch(es) applied!").arg(patched).arg(patchList.size()));
}

void PatchDialog::on_btnImport_clicked()
{
    QStringList filenamelist = QFileDialog::getOpenFileNames(this, tr("Open patch"), "", tr("Patch files (*.1337)"));
    int patched = 0;
    bool bBadOriginal = false;
    bool bAlreadyDone = false;

    typedef struct _IMPORTSTATUS
    {
        bool badoriginal;
        bool alreadypatched;
    } IMPORTSTATUS;
    QList<QPair<DBGPATCHINFO, IMPORTSTATUS>> patchList;
    DBGPATCHINFO curPatch;

    for(const auto & filename1 : filenamelist)
    {
        if(!filename1.length())
            continue;
        QString filename = QDir::toNativeSeparators(filename1); //convert to native path format (with backlashes)
        QFile file(filename);
        file.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&file);
        QString patch = in.readAll();
        file.close();
        patch = patch.replace("\r\n", "\n");
        QStringList lines = patch.split("\n", QString::SkipEmptyParts);
        if(!lines.size())
        {
            SimpleErrorBox(this, tr("Error!"), tr("The patch file is empty..."));
            continue;
        }

        dsint modbase = 0;
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
                //File format is error. Don't continue processing this file
                SimpleErrorBox(this, tr("Error!"), tr("Patch file format is incorrect..."));
                break;
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
    }

    //Check if any patch exists
    if(!patchList.size())
    {
        SimpleInfoBox(this, tr("Information"), tr("No patches to apply in the current process."));
        return;
    }

    //Warn if some are already patched
    bool bUndoPatched = false;
    if(bAlreadyDone)
    {
        QMessageBox msg(QMessageBox::Question, tr("Question"), tr("Some patches are already applied.\n\nDo you want to remove these patches?"), QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(DIcon("question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bUndoPatched = true;
    }

    bool bPatchBadOriginals = false;
    if(bBadOriginal)
    {
        QMessageBox msg(QMessageBox::Question, tr("Question"), tr("Some bytes do not match the original in the patch file.\n\nDo you want to apply these patches anyway?"), QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(DIcon("question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bPatchBadOriginals = true;
    }

    //Apply all of the patches
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

    SimpleInfoBox(this, tr("Information"), tr("%1/%2 patch(es) applied!").arg(patched).arg(patchList.size()));
}

void PatchDialog::on_btnExport_clicked()
{
    if(!mPatches.size())
        return;

    if(containsRelocatedBytes() && !showRelocatedBytesWarning())
        return;

    QString filename = QFileDialog::getSaveFileName(this, tr("Save patch"), "", tr("Patch files (*.1337)"));
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)
    if(filename.endsWith(QString(".1337")))
        saveAs1337(filename);
    // TODO: C program source
}

void PatchDialog::saveAs1337(const QString & filename)
{
    QStringList lines;

    int patches = 0;
    for(PatchMap::iterator i = mPatches.begin(); i != mPatches.end(); ++i)
    {
        const PatchInfoList & curPatchList = i.value();
        bool bModPlaced = false;
        dsint modbase = DbgFunctions()->ModBaseFromName(i.key().toUtf8().constData());
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
            QString addrText = ToPtrString(curPatchList.at(j).patch.addr - modbase);
            lines.push_back(addrText + QString().sprintf(":%.2X->%.2X", curPatchList.at(j).patch.oldbyte, curPatchList.at(j).patch.newbyte));
            patches++;
        }
    }

    if(!lines.size())
    {
        SimpleInfoBox(this, tr("Information"), tr("No patches to export."));
        return;
    }

    QFile file(filename);
    file.open(QFile::WriteOnly | QFile::Text);
    QString text = lines.join("\n");
    QByteArray textUtf8 = text.toUtf8();
    file.write(textUtf8.constData(), textUtf8.length());
    file.close();

    SimpleInfoBox(this, tr("Information"), tr("%1 patch(es) exported!").arg(patches));
}

bool PatchDialog::containsRelocatedBytes()
{
    for(PatchMap::iterator i = mPatches.begin(); i != mPatches.end(); ++i)
    {
        const PatchInfoList & curPatchList = i.value();
        if(containsRelocatedBytes(curPatchList))
            return true;
    }
    return false;
}

bool PatchDialog::containsRelocatedBytes(const PatchInfoList & patchList)
{
    for(int i = 0; i < patchList.size(); i++)
    {
        if(patchList.at(i).status.checked && DbgFunctions()->ModRelocationAtAddr(patchList.at(i).patch.addr, nullptr))
            return true;
    }

    return false;
}

bool PatchDialog::showRelocatedBytesWarning()
{
    auto result = QMessageBox::question(this, tr("Patches overlap with relocation regions"), tr("Your patches overlap with relocation regions. This can cause your code to become corrupted when you load the patched executable. Do you want to continue?"));
    return result == QMessageBox::Yes;
}
