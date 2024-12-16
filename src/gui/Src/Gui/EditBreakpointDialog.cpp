#include "EditBreakpointDialog.h"
#include "ui_EditBreakpointDialog.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include "Configuration.h"
#include "BrowseDialog.h"

EditBreakpointDialog::EditBreakpointDialog(QWidget* parent, const Breakpoints::Data & bp)
    : QDialog(parent),
      ui(new Ui::EditBreakpointDialog),
      mBp(bp)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedHeight(sizeHint().height()); // resizable only horizontally

    ui->labelBreakCondition->setText(QString("<a href=\"https://help.x64dbg.com/en/latest/introduction/ConditionalBreakpoint.html\">%1</a>:").arg(ui->labelBreakCondition->text().replace(":", "")));

    switch(bp.type)
    {
    case bp_dll:
        setWindowTitle(tr("Edit DLL Breakpoint %1").arg(mBp.module));
        break;
    case bp_normal:
        setWindowTitle(tr("Edit Breakpoint %1").arg(getSymbolicName(mBp.addr)));
        break;
    case bp_hardware:
        setWindowTitle(tr("Edit Hardware Breakpoint %1").arg(getSymbolicName(mBp.addr)));
        break;
    case bp_memory:
        setWindowTitle(tr("Edit Memory Breakpoint %1").arg(getSymbolicName(mBp.addr)));
        break;
    case bp_exception:
        setWindowTitle(tr("Edit Exception Breakpoint %1").arg(getSymbolicName(mBp.addr)));
        break;
    default:
        setWindowTitle(tr("Edit Breakpoint %1").arg(getSymbolicName(mBp.addr)));
        break;
    }
    setWindowIcon(DIcon("breakpoint"));
    loadFromBp();

    connect(this, SIGNAL(accepted()), this, SLOT(acceptedSlot()));

    Config()->loadWindowGeometry(this);
}

EditBreakpointDialog::~EditBreakpointDialog()
{
    Config()->saveWindowGeometry(this);
    delete ui;
}

void EditBreakpointDialog::loadFromBp()
{
    ui->editName->setText(mBp.name);
    ui->spinHitCount->setValue(mBp.hitCount);
    ui->editBreakCondition->setText(mBp.breakCondition);
    ui->checkBoxFastResume->setChecked(mBp.fastResume);
    ui->checkBoxSilent->setChecked(mBp.silent);
    ui->checkBoxSingleshoot->setChecked(mBp.singleshoot);
    ui->editLogText->setText(mBp.logText);
    ui->editLogCondition->setText(mBp.logCondition);
    ui->editCommandText->setText(mBp.commandText);
    ui->editCommandCondition->setText(mBp.commandCondition);
    mLogFile = mBp.logFile;
    ui->buttonLogFile->setToolTip(mLogFile);
}

void EditBreakpointDialog::on_editLogText_textEdited(const QString & arg1)
{
    Q_UNUSED(arg1);
    ui->checkBoxSilent->setChecked(true);
}

void EditBreakpointDialog::on_buttonLogFile_clicked()
{
    BrowseDialog browse(
        this,
        tr("Breakpoint log file"),
        tr("Enter the path to the log file."),
        tr("Log Files (*.txt *.log);;All Files (*.*)"),
        mLogFile.isEmpty() ? getDbPath(mainModuleName() + ".log", false) : mLogFile,
        true
    );
    browse.setConfirmOverwrite(false);
    if(browse.exec() == QDialog::Accepted)
        mLogFile = browse.path;
    else
        mLogFile.clear();
    ui->buttonLogFile->setToolTip(mLogFile);
}

void EditBreakpointDialog::acceptedSlot()
{
    mBp.breakCondition = ui->editBreakCondition->text();
    mBp.logText = ui->editLogText->text();
    mBp.logCondition = ui->editLogCondition->text();
    mBp.commandText = ui->editCommandText->text();
    mBp.commandCondition = ui->editCommandCondition->text();
    mBp.name = ui->editName->text();
    mBp.hitCount = ui->spinHitCount->value();
    mBp.singleshoot = ui->checkBoxSingleshoot->isChecked();
    mBp.silent = ui->checkBoxSilent->isChecked();
    mBp.fastResume = ui->checkBoxFastResume->isChecked();
    mBp.logFile = mLogFile;
}
