#include "SystemBreakpointScriptDialog.h"
#include "ui_SystemBreakpointScriptDialog.h"
#include "Bridge.h"
#include "Configuration.h"
#include <QDirModel>
#include <QFile>
#include <QFileDialog>
#include <QDesktopServices>
#include <QCompleter>
#include <QMessageBox>

SystemBreakpointScriptDialog::SystemBreakpointScriptDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::SystemBreakpointScriptDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);

    auto dirCompleter = [](QLineEdit * lineEdit)
    {
        QCompleter* completer = new QCompleter(lineEdit);
        completer->setModel(new QDirModel(completer));
        lineEdit->setCompleter(completer);
    };
    dirCompleter(ui->lineEditGlobal);
    dirCompleter(ui->lineEditDebuggee);

    {
        char globalChar[MAX_SETTING_SIZE];
        if(BridgeSettingGet("Engine", "InitializeScript", globalChar))
            ui->lineEditGlobal->setText(globalChar);
    }

    if(DbgIsDebugging())
    {
        char moduleName[MAX_MODULE_SIZE];
        if(DbgFunctions()->ModNameFromAddr(DbgValFromString("mod.main()"), moduleName, true))
        {
            ui->groupBoxDebuggee->setTitle(tr("2. System breakpoint script for %1").arg(moduleName));
        }

        ui->lineEditDebuggee->setText(DbgFunctions()->DbgGetDebuggeeInitScript());
    }
    else
    {
        ui->groupBoxDebuggee->setEnabled(false);
    }

    if(ui->lineEditGlobal->text().isEmpty())
        ui->openGlobal->setText(tr("Create"));
    if(ui->lineEditDebuggee->text().isEmpty())
        ui->openDebuggee->setText(tr("Create"));

    Config()->setupWindowPos(this);
}

SystemBreakpointScriptDialog::~SystemBreakpointScriptDialog()
{
    delete ui;
}

void SystemBreakpointScriptDialog::on_pushButtonGlobal_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, ui->groupBoxGlobal->title(), ui->lineEditGlobal->text(), tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!file.isEmpty())
        ui->lineEditGlobal->setText(QDir::toNativeSeparators(file));
    if(ui->lineEditGlobal->text().isEmpty())
        ui->openGlobal->setText(tr("Create"));
    else
        ui->openGlobal->setText(tr("Open"));
}

void SystemBreakpointScriptDialog::on_pushButtonDebuggee_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, ui->groupBoxDebuggee->title(), ui->lineEditDebuggee->text(), tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(!file.isEmpty())
        ui->lineEditDebuggee->setText(QDir::toNativeSeparators(file));
    if(ui->lineEditDebuggee->text().isEmpty())
        ui->openDebuggee->setText(tr("Create"));
    else
        ui->openDebuggee->setText(tr("Open"));
}

void SystemBreakpointScriptDialog::on_openGlobal_clicked()
{
    // First open the script if that is available
    if(!ui->lineEditGlobal->text().isEmpty())
        QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(ui->lineEditGlobal->text())));
    else
    {
        // Ask the user to create a new script
        QMessageBox msgyn(QMessageBox::Question, tr("File not found"), tr("Would you like to create a new script?"), QMessageBox::Yes | QMessageBox::No, this);
        if(msgyn.exec() == QMessageBox::Yes)
        {
            // The new script is at app dir
            QString defaultFileName("autorun.txt");
            defaultFileName = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + defaultFileName);
            // Create it
            if(!QFile::exists(defaultFileName))
            {
                QFile newScript(defaultFileName);
                newScript.open(QIODevice::Append | QIODevice::WriteOnly);
                newScript.close();
            }
            ui->lineEditGlobal->setText(defaultFileName);
            ui->openGlobal->setText(tr("Open"));
            // Open the file
            QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(ui->lineEditGlobal->text())));
        }
    }
}

void SystemBreakpointScriptDialog::on_openDebuggee_clicked()
{
    // First open the script if that is available
    if(!ui->lineEditDebuggee->text().isEmpty())
        QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(ui->lineEditDebuggee->text())));
    else
    {
        // Ask the user to create a new script
        QMessageBox msgyn(QMessageBox::Question, tr("File not found"), tr("Would you like to create a new script?"), QMessageBox::Yes | QMessageBox::No, this);
        if(msgyn.exec() == QMessageBox::Yes)
        {
            // The new script is at db dir
            QString defaultFileName;
            char moduleName[MAX_MODULE_SIZE];
            if(DbgFunctions()->ModNameFromAddr(DbgValFromString("mod.main()"), moduleName, false))
            {
                defaultFileName = QString::fromUtf8(moduleName);
            }
            defaultFileName = defaultFileName + ".autorun.txt";
            defaultFileName = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + QDir::separator() + "db" + QDir::separator() + defaultFileName);
            // Create it
            if(!QFile::exists(defaultFileName))
            {
                QFile newScript(defaultFileName);
                newScript.open(QIODevice::Append | QIODevice::WriteOnly);
                newScript.close();
            }
            ui->lineEditDebuggee->setText(defaultFileName);
            ui->openDebuggee->setText(tr("Open"));
            // Open the file
            QDesktopServices::openUrl(QUrl(QDir::fromNativeSeparators(ui->lineEditDebuggee->text())));
        }
    }
}

void SystemBreakpointScriptDialog::on_SystemBreakpointScriptDialog_accepted()
{
    BridgeSettingSet("Engine", "InitializeScript", ui->lineEditGlobal->text().toUtf8().constData());
    if(ui->groupBoxDebuggee->isEnabled())
        DbgFunctions()->DbgSetDebuggeeInitScript(ui->lineEditDebuggee->text().toUtf8().constData());
}
