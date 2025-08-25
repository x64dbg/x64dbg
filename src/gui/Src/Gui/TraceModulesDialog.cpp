#include "TraceModulesDialog.h"
#include "ui_TraceModulesDialog.h"
#include "Bridge.h"

TraceModulesDialog::TraceModulesDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::TraceModulesDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true);
}

TraceModulesDialog::~TraceModulesDialog()
{
    delete ui;
}

void TraceModulesDialog::loadModules()
{
    // Get included modules from bridge
    BridgeList<const char*> includedModules;
    Bridge::getBridge()->getTraceIncludes(&includedModules);

    // Get excluded modules from bridge
    BridgeList<const char*> excludedModules;
    Bridge::getBridge()->getTraceExcludes(&excludedModules);

    // Update UI
    QString includedText;
    for(int i = 0; i < includedModules.Count(); i++)
    {
        if(i > 0)
            includedText += "\n";
        includedText += includedModules[i];
    }
    ui->editIncluded->setPlainText(includedText);

    QString excludedText;
    for(int i = 0; i < excludedModules.Count(); i++)
    {
        if(i > 0)
            excludedText += "\n";
        excludedText += excludedModules[i];
    }
    ui->editExcluded->setPlainText(excludedText);
}

void TraceModulesDialog::saveModules()
{
    // Clear existing modules
    Bridge::getBridge()->clearTraceIncludes();
    Bridge::getBridge()->clearTraceExcludes();

    // Get modules from UI
    QStringList includedModules = ui->editIncluded->toPlainText().split('\n', Qt::SkipEmptyParts);
    QStringList excludedModules = ui->editExcluded->toPlainText().split('\n', Qt::SkipEmptyParts);

    // Add included modules
    for(const QString & module : includedModules)
    {
        if(!module.trimmed().isEmpty())
            Bridge::getBridge()->addTraceInclude(module.trimmed());
    }

    // Add excluded modules
    for(const QString & module : excludedModules)
    {
        if(!module.trimmed().isEmpty())
            Bridge::getBridge()->addTraceExclude(module.trimmed());
    }
}

void TraceModulesDialog::on_buttonOk_clicked()
{
    saveModules();
    accept();
}

void TraceModulesDialog::on_buttonCancel_clicked()
{
    reject();
}

void TraceModulesDialog::on_buttonClearIncluded_clicked()
{
    ui->editIncluded->clear();
}

void TraceModulesDialog::on_buttonClearExcluded_clicked()
{
    ui->editExcluded->clear();
}