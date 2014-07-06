#include "PatchDialog.h"
#include "ui_PatchDialog.h"

PatchDialog::PatchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PatchDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint);
    setFixedSize(this->size()); //fixed size
    setModal(false); //non-modal window

    connect(Bridge::getBridge(), SIGNAL(updatePatches()), this, SLOT(updatePatches()));

    mPatches = new PatchMap();
}

PatchDialog::~PatchDialog()
{
    delete ui;
}

bool PatchDialog::isPartOfPreviousGroup(PatchInfoList & patchList, int index)
{
    if(!index)
        return true;
    uint addr=patchList.at(index).first.addr;
    uint prevAddr=patchList.at(index-1).first.addr;
    for(int i=1; i<10; i++) //10 bytes in between groups
        if(addr-i==prevAddr)
            return true;
    return false;
}

void PatchDialog::updatePatches()
{
    mIsAdding=true;
    //clear GUI
    ui->listModules->clear();
    ui->listPatches->clear();
    delete mPatches;
    mPatches = new PatchMap();

    //get patches from DBG
    size_t cbsize;
    if(!DbgFunctions()->PatchEnum(0, &cbsize))
        return;
    int numPatches = cbsize/sizeof(DBGPATCHINFO);
    if(!numPatches)
        return;
    DBGPATCHINFO* patches = new DBGPATCHINFO[numPatches];
    memset(patches, 0, numPatches*sizeof(DBGPATCHINFO));
    if(!DbgFunctions()->PatchEnum(patches, 0))
    {
        delete [] patches;
        mIsAdding=false;
        return;
    }

    //fill the patch list
    STATUSINFO defaultStatus;
    defaultStatus.group=0;
    defaultStatus.checked=false;
    for(int i=0; i<numPatches; i++)
    {
        if(!patches[i].addr)
            continue;
        QString mod(patches[i].mod);
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
    for(PatchMap::iterator i=mPatches->begin(); i!=mPatches->end(); ++i)
    {
        qSort(i.value().begin(), i.value().end(), PatchInfoLess);
        PatchInfoList & curPatchList = i.value();
        //group the patched bytes
        for(int j=0,group=0; j<curPatchList.size(); j++)
        {
            if(!isPartOfPreviousGroup(curPatchList, j))
                group++;
            curPatchList[j].second.group=group;
        }
        ui->listModules->addItem(i.key());
    }
    mIsAdding=false;
}

void PatchDialog::on_listModules_itemSelectionChanged()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod(ui->listModules->selectedItems().at(0)->text());
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsAdding=true;
    PatchInfoList patchList = found.value();
    ui->listPatches->clear();
    for(int i=0; i<patchList.size(); i++)
    {
        const DBGPATCHINFO curPatch = patchList.at(i).first;
        QString addrText = QString("%1").arg(curPatch.addr, sizeof(int_t)*2, 16, QChar('0')).toUpper();
        QListWidgetItem* item = new QListWidgetItem(QString().sprintf("%d", patchList.at(i).second.group).rightJustified(4, ' ', true) + "|" + addrText + QString().sprintf(":%.2X->%.2X", curPatch.oldbyte, curPatch.newbyte), ui->listPatches);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    mIsAdding=false;
}

void PatchDialog::on_listPatches_itemChanged(QListWidgetItem *item) //checkbox changed
{
    if(mIsAdding || !ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    bool checked=item->checkState()==Qt::Checked;
    PatchInfoList & curPatchList = found.value();
    PatchPair & patch = curPatchList[ui->listPatches->row(item)];
    if(patch.second.checked == checked) //check state did not change
        return;
    patch.second.checked = checked;
    //check state changed
    if((QApplication::keyboardModifiers() & Qt::ControlModifier) == Qt::ControlModifier)
        return; //control held down -> do not check/uncheck group
    mIsAdding=true;
    //check/uncheck the complete group
    for(int i=0; i<curPatchList.size(); i++)
        if(curPatchList.at(i).second.group==patch.second.group)
        {
            curPatchList[i].second.checked=checked;
            ui->listPatches->item(i)->setCheckState(item->checkState());
        }
    mIsAdding=false;
}

void PatchDialog::on_btnSelectAll_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsAdding=true;
    PatchInfoList & curPatchList = found.value();
    for(int i=0; i<curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Checked);
        curPatchList[i].second.checked=true;
    }
    mIsAdding=false;
}

void PatchDialog::on_btnDeselectAll_clicked()
{
    if(!ui->listModules->selectedItems().size())
        return;
    QString mod = ui->listModules->selectedItems().at(0)->text();
    PatchMap::iterator found = mPatches->find(mod);
    if(found == mPatches->end()) //not found
        return;
    mIsAdding=true;
    PatchInfoList & curPatchList = found.value();
    for(int i=0; i<curPatchList.size(); i++)
    {
        ui->listPatches->item(i)->setCheckState(Qt::Unchecked);
        curPatchList[i].second.checked=false;
    }
    mIsAdding=false;
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
    mIsAdding=true;
    PatchInfoList & curPatchList = found.value();
    int removed=0;
    int total=curPatchList.size();
    for(int i=0; i<total; i++)
    {
        if(curPatchList.at(i).second.checked)
        {
            DbgFunctions()->PatchRestore(curPatchList.at(i).first.addr);
            removed++;
        }
    }
    mIsAdding=false;
    updatePatches();
    if(removed!=total)
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
    int group=patch.second.group;
    int_t groupStart = 0;
    for(int i=0; i<curPatchList.size(); i++)
        if(curPatchList.at(i).second.group==group)
        {
            groupStart = curPatchList.at(i).first.addr;
            break;
        }
    if(!groupStart)
        return;
    QString addrText = QString("%1").arg(groupStart, sizeof(int_t)*2, 16, QChar('0')).toUpper();
    DbgCmdExecDirect(QString("disasm "+addrText).toUtf8().constData());
    DbgCmdExecDirect(QString("dump "+addrText).toUtf8().constData());
}
