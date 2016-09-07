#include "PatchDialogGroupSelector.h"
#include "ui_PatchDialogGroupSelector.h"

PatchDialogGroupSelector::PatchDialogGroupSelector(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PatchDialogGroupSelector)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(false); //non-modal window

    mGroup = 0;
}

PatchDialogGroupSelector::~PatchDialogGroupSelector()
{
    delete ui;
}

void PatchDialogGroupSelector::keyPressEvent(QKeyEvent* event)
{
    if(event->modifiers() != Qt::NoModifier)
        return;
    if(event->key() == Qt::Key_Space)
        on_btnToggle_clicked();
    else if(event->key() == Qt::Key_BracketLeft)
        on_btnPrevious_clicked();
    else if(event->key() == Qt::Key_BracketRight)
        on_btnNext_clicked();
}

void PatchDialogGroupSelector::setGroupTitle(const QString & title)
{
    ui->lblTitle->setText(title);
}

void PatchDialogGroupSelector::setPreviousEnabled(bool enable)
{
    ui->btnPrevious->setEnabled(enable);
}

void PatchDialogGroupSelector::setNextEnabled(bool enable)
{
    ui->btnNext->setEnabled(enable);
}

void PatchDialogGroupSelector::setGroup(int group)
{
    mGroup = group;
}

int PatchDialogGroupSelector::group()
{
    return mGroup;
}

void PatchDialogGroupSelector::on_btnToggle_clicked()
{
    emit groupToggle();
}

void PatchDialogGroupSelector::on_btnPrevious_clicked()
{
    if(ui->btnPrevious->isEnabled())
        emit groupPrevious();
}

void PatchDialogGroupSelector::on_btnNext_clicked()
{
    if(ui->btnNext->isEnabled())
        emit groupNext();
}
