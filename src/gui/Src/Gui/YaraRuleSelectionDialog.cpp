#include "YaraRuleSelectionDialog.h"
#include "ui_YaraRuleSelectionDialog.h"
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QDirIterator>
#include "Imports.h"

YaraRuleSelectionDialog::YaraRuleSelectionDialog(QWidget* parent, const QString & title) :
    QDialog(parent),
    ui(new Ui::YaraRuleSelectionDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setWindowTitle(title);

    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet("Misc", "YaraRulesDirectory", setting))
    {
        rulesDirectory = QString(setting);
        enumRulesDirectory();
    }
}

YaraRuleSelectionDialog::~YaraRuleSelectionDialog()
{
    delete ui;
}

QString YaraRuleSelectionDialog::getSelectedFile()
{
    return selectedFile;
}

void YaraRuleSelectionDialog::on_buttonDirectory_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Yara Rules Directory..."));
    if(!dir.length())
        return;
    rulesDirectory = QDir::toNativeSeparators(dir);
    BridgeSettingSet("Misc", "YaraRulesDirectory", dir.toUtf8().constData());
    enumRulesDirectory();
}

void YaraRuleSelectionDialog::on_buttonFile_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Select Yara Rule..."), rulesDirectory);
    if(!file.length())
        return;
    selectedFile = QDir::toNativeSeparators(file);
    this->accept();
}

void YaraRuleSelectionDialog::on_buttonSelect_clicked()
{
    if(!ui->listRules->selectedItems().size()) //no selection
        return;
    int selectedIndex = ui->listRules->row(ui->listRules->selectedItems().at(0));
    selectedFile = ruleFiles.at(selectedIndex).first;
    this->accept();
}

void YaraRuleSelectionDialog::enumRulesDirectory()
{
    ruleFiles.clear();
    ui->listRules->clear();
    QDirIterator it(rulesDirectory, QDir::Files, QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        it.next();
        ruleFiles.append(QPair<QString, QString>(QDir::toNativeSeparators(it.filePath()), it.fileName()));
        ui->listRules->addItem(it.fileName());
    }
    ui->listRules->setCurrentRow(0);
}
