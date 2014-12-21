#include "PatchDialog.h"
#include "ui_PatchDialog.h"
#include <QMessageBox>
#include <QIcon>
#include <QDesktopServices>
#include <QFileDialog>
#include <QTextStream>

PatchDialog::PatchDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PatchDialog)
{
    ui->setupUi(this);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
#endif
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

    mPatches = new PatchMap();
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
    uint addr = patchList.at(index).first.addr;
    uint prevAddr = patchList.at(index - 1).first.addr;
    for(int i = 1; i < 10; i++) //10 bytes in between groups
        if(addr - i == prevAddr)
            return true;
    return false;
}

bool PatchDialog::isGroupEnabled(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).second.group == group && !patchList.at(i).second.checked)
            return false;
    return true;
}

bool PatchDialog::hasPreviousGroup(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).second.group < group)
            return true;
    return false;
}

bool PatchDialog::hasNextGroup(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).second.group > group)
            return true;
    return false;
}

int_t PatchDialog::getGroupAddress(const PatchInfoList & patchList, int group)
{
    for(int i = 0; i < patchList.size(); i++)
        if(patchList.at(i).second.group == group)
            return patchList.at(i).first.addr;
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
    delete mPatches;
    mPatches = new PatchMap();

    //get patches from DBG
    size_t cbsize;
    if(!DbgFunctions()->PatchEnum(0, &cbsize))
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
        PatchMap::iterator found = mPatches->find(mod);
        if(found != mPatches->end()) //found
            (*mPatches)[mod].append(PatchPair(patches[i], defaultStatus));
        else //not found
        {
            PatchInfoList patchList;
            patchList.append(PatchPair(patches[i], defaultStatus));
            mPatches->insert(mod, patchList);
        }
    }
    delete [] patches;

    //sort the patches by address
    for(PatchMap::iterator i = mPatches->begin(); i != mPatches->end(); ++i)
    {
        qSort(i.value().begin(), i.value().end(), PatchInfoLess);
        PatchInfoList & curPatchList = i.value();
        //group the patched bytes
        for(int j = 0, group = 0; j < curPatchList.size(); j++)
        {
            if(!isPartOfPreviousGroup(curPatchList, j))
                group++;
            curPatchList[j].second.group = group;
        }
        ui->listModules->addItem(i.key());
    }

    if(mPatches->size())
        ui->listModules->item(0)->setSelected(true); //select first module

    mIsWorking = false;
}

void PatchDialog::groupToggle()
{
    int group = mGroupSelector->group();
    if(mIsWorking || !ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    bool enabled = !isGroupEnabled(curPatchList, group);
    Qt::CheckState checkState = enabled ? Qt::Checked : Qt::Unchecked;
    mIsWorking = true;
    for(int i = 0; i < curPatchList.size(); i++)
    {
        if(curPatchList.at(i).second.group != group)
            continue;
        ui->listPatches->item(i)->setCheckState(checkState);
        curPatchList[i].second.checked = enabled;
    }
    mIsWorking = false;
    int_t groupStart = getGroupAddress(curPatchList, group);
    if(!groupStart)
        return;
    QString color = enabled ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(groupStart, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!hasPreviousGroup(curPatchList, group))
        return;
    group--;
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!hasNextGroup(curPatchList, group))
        return;
    group++;
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & patchList = found.value();
    ui->listPatches->clear();
    for(int i = 0; i < patchList.size(); i++)
    {
        const DBGPATCHINFO curPatch = patchList.at(i).first;
        QString addrText = QString("%1").arg(curPatch.addr, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
        QListWidgetItem* item = new QListWidgetItem(QString().sprintf("%d", patchList.at(i).second.group).rightJustified(4, ' ', true) + "|" + addrText + QString().sprintf(":%.2X->%.2X", curPatch.oldbyte, curPatch.newbyte), ui->listPatches);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        Qt::CheckState state = patchList.at(i).second.checked ? Qt::Checked : Qt::Unchecked;
        item->setCheckState(state);
    }
    mIsWorking = false;
}

void PatchDialog::on_listPatches_itemChanged(QListWidgetItem* item) //checkbox changed
{
    if(mIsWorking || !ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    bool checked = item->checkState() == Qt::Checked;
    PatchInfoList & curPatchList = found.value();
    PatchPair & patch = curPatchList[ui->listPatches->row(item)];
    if(patch.second.checked == checked) //check state did not change
        return;
    patch.second.checked = checked;
    //check state changed
    if((QApplication::keyboardModifiers() & Qt::ControlModifier) != Qt::ControlModifier)
    {
        mIsWorking = true;
        //check/uncheck the complete group
        for(int i = 0; i < curPatchList.size(); i++)
            if(curPatchList.at(i).second.group == patch.second.group)
            {
                curPatchList[i].second.checked = checked;
                ui->listPatches->item(i)->setCheckState(item->checkState());
            }
        mIsWorking = false;
    }
    int group = mGroupSelector->group();
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    for(int i = 0; i < curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Checked);
        curPatchList[i].second.checked = true;
    }
    mIsWorking = false;
}

void PatchDialog::on_btnDeselectAll_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    for(int i = 0; i < curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Unchecked);
        curPatchList[i].second.checked = false;
    }
    mIsWorking = false;
}

void PatchDialog::on_btnRestoreSelected_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    int selModIdx = ui->listModules->row(ui->listModules->selectedItems().at(0));
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsWorking = true;
    PatchInfoList & curPatchList = found.value();
    int removed = 0;
    int total = curPatchList.size();
    for(int i = 0; i < total; i++)
    {
        if(curPatchList.at(i).second.checked)
        {
            DbgFunctions()->PatchRestore(curPatchList.at(i).first.addr);
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    PatchPair & patch = curPatchList[ui->listPatches->row(ui->listPatches->selectedItems().at(0))]; //selected item
    int_t groupStart = getGroupAddress(curPatchList, patch.second.group);
    if(!groupStart)
        return;
    QString addrText = QString("%1").arg(groupStart, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
    DbgCmdExecDirect(QString("disasm " + addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump " + addrText).toUtf8().constData());
}

void PatchDialog::on_btnPickGroups_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();
    if(!curPatchList.size())
        return;
    this->showMinimized();

    int group = mGroupSelector->group();
    QString color = isGroupEnabled(curPatchList, group) ? "#00DD00" : "red";
    QString addrText = QString("%1").arg(getGroupAddress(curPatchList, group), sizeof(int_t) * 2, 16, QChar('0')).toUpper();
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
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    PatchInfoList & curPatchList = found.value();

    //get patches to save
    QList<DBGPATCHINFO> patchList;
    for(int i = 0; i < curPatchList.size(); i++)
        if(curPatchList.at(i).second.checked)
            patchList.push_back(curPatchList.at(i).first);
    if(!curPatchList.size() || !patchList.size())
    {
        QMessageBox msg(QMessageBox::Information, "Information", "Nothing to patch!");
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    char szModName[MAX_PATH] = "";
    if(!DbgFunctions()->ModPathFromAddr(DbgFunctions()->ModBaseFromName(mod.toUtf8().constData()), szModName, MAX_PATH))
    {
        QMessageBox msg(QMessageBox::Critical, "Error!", "Failed to get module filename...");
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

    QString filename = QFileDialog::getSaveFileName(this, "Save file", szDirName, "All files (*.*)");
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
        QMessageBox msg(QMessageBox::Critical, "Error!", QString("Failed to save patched file (" + QString(error) + ")"));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }
    QMessageBox msg(QMessageBox::Information, "Information", QString().sprintf("%d/%d patch(es) applied!", patched, patchList.size()));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void PatchDialog::on_btnImport_clicked()
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QString filename = QFileDialog::getOpenFileName(this, tr("Open patch"), QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation), tr("Patch files (*.1337)"));
#else
    QString filename = QFileDialog::getOpenFileName(this, tr("Open patch"), QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0], tr("Patch files (*.1337)"));
#endif
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
        QMessageBox msg(QMessageBox::Critical, "Error!", QString("The patch file is empty..."));
        msg.setWindowIcon(QIcon(":/icons/images/compile-error.png"));
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    typedef struct _IMPORTSTATUS
    {
        bool nomatchoriginal;
        bool matchold;
    } IMPORTSTATUS;
    QList<QPair<DBGPATCHINFO, IMPORTSTATUS>> patchList;
    DBGPATCHINFO curPatch;
    int_t modbase = 0;
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
            QMessageBox msg(QMessageBox::Critical, "Error!", QString("Patch file format is incorrect..."));
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
        if(checkbyte == newbyte)
            bAlreadyDone = true;
        else if(checkbyte != oldbyte)
            bBadOriginal = true;
        curPatch.oldbyte = oldbyte;
        curPatch.newbyte = newbyte;
        IMPORTSTATUS status = {checkbyte != oldbyte && !checkbyte == newbyte, checkbyte == newbyte};
        patchList.push_back(QPair<DBGPATCHINFO, IMPORTSTATUS>(curPatch, status));
    }

    if(!patchList.size())
    {
        QMessageBox msg(QMessageBox::Information, "Information", QString().sprintf("No patches to apply in the current process."));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    bool bUndoPatched = false;
    if(bAlreadyDone)
    {
        QMessageBox msg(QMessageBox::Question, "Question", "Some patches are already applied.\n\nDo you want to remove these patches?", QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bUndoPatched = true;
    }

    bool bPatchBadOriginals = false;
    if(bBadOriginal)
    {
        QMessageBox msg(QMessageBox::Question, "Question", "Some bytes do not match the original in the patch file.\n\nDo you want to apply these patches anyway?", QMessageBox::Yes | QMessageBox::No);
        msg.setWindowIcon(QIcon(":/icons/images/question.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        if(msg.exec() == QMessageBox::Yes)
            bPatchBadOriginals = true;
    }

    int patched = 0;
    for(int i = 0; i < patchList.size(); i++)
    {
        if(!bPatchBadOriginals && patchList.at(i).second.nomatchoriginal)
            continue;
        curPatch = patchList.at(i).first;
        if(bUndoPatched && patchList.at(i).second.matchold)
        {
            GuiAddStatusBarMessage("undo!");
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

    QMessageBox msg(QMessageBox::Information, "Information", QString().sprintf("%d/%d patch(es) applied!", patched, patchList.size()));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}

void PatchDialog::on_btnExport_clicked()
{
    if(!mPatches->size())
        return;

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    QString filename = QFileDialog::getSaveFileName(this, tr("Save patch"), QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation), tr("Patch files (*.1337)"));
#else
    QString filename = QFileDialog::getSaveFileName(this, tr("Save patch"), QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0], tr("Patch files (*.1337)"));
#endif
    if(!filename.length())
        return;
    filename = QDir::toNativeSeparators(filename); //convert to native path format (with backlashes)

    QStringList lines;

    int patches = 0;
    for(PatchMap::iterator i = mPatches->begin(); i != mPatches->end(); ++i)
    {
        const PatchInfoList & curPatchList = i.value();
        bool bModPlaced = false;
        int_t modbase = DbgFunctions()->ModBaseFromName(i.key().toUtf8().constData());
        if(!modbase)
            continue;
        for(int j = 0; j < curPatchList.size(); j++)
        {
            if(!curPatchList.at(j).second.checked) //skip unchecked patches
                continue;
            if(!bModPlaced)
            {
                lines.push_back(">" + i.key());
                bModPlaced = true;
            }
            QString addrText = QString("%1").arg(curPatchList.at(j).first.addr - modbase, sizeof(int_t) * 2, 16, QChar('0')).toUpper();
            lines.push_back(addrText + QString().sprintf(":%.2X->%.2X", curPatchList.at(j).first.oldbyte, curPatchList.at(j).first.newbyte));
            patches++;
        }
    }

    if(!lines.size())
    {
        QMessageBox msg(QMessageBox::Information, "Information", QString().sprintf("No patches to export."));
        msg.setWindowIcon(QIcon(":/icons/images/information.png"));
        msg.setParent(this, Qt::Dialog);
        msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
        msg.exec();
        return;
    }

    QFile file(filename);
    file.open(QFile::WriteOnly | QFile::Text);
    QString text = lines.join("\n");
    file.write(text.toUtf8().constData(), text.length());
    file.close();

    QMessageBox msg(QMessageBox::Information, "Information", QString().sprintf("%d patch(es) exported!", patches));
    msg.setWindowIcon(QIcon(":/icons/images/information.png"));
    msg.setParent(this, Qt::Dialog);
    msg.setWindowFlags(msg.windowFlags() & (~Qt::WindowContextHelpButtonHint));
    msg.exec();
}
