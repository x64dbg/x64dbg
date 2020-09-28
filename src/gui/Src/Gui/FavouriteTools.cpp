#include "FavouriteTools.h"
#include "ui_FavouriteTools.h"
#include "Bridge.h"
#include "BrowseDialog.h"
#include "MainWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include "MiscUtil.h"
#include "Configuration.h"
#include <QTableWidget>

FavouriteTools::FavouriteTools(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::FavouriteTools)
{
    ui->setupUi(this);
    //set window flags
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

    setupTools("Tool", ui->listTools);
    setupTools("Script", ui->listScript);

    QStringList tblHeaderCommand;
    tblHeaderCommand << tr("Command") << tr("Shortcut");
    QTableWidget* list = ui->listCommand;
    list->setColumnCount(2);
    list->verticalHeader()->setVisible(false);
    list->setHorizontalHeaderLabels(tblHeaderCommand);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setShowGrid(false);
    list->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    list->verticalHeader()->setDefaultSectionSize(15);
    char buffer[MAX_SETTING_SIZE];
    int i;
    std::vector<QString> allCommand;
    std::vector<QString> allToolShortcut;
    for(i = 1; BridgeSettingGet("Favourite", QString("Command%1").arg(i).toUtf8().constData(), buffer); i++)
    {
        QString command = QString(buffer);
        QString commandShortcut("");
        if(BridgeSettingGet("Favourite", QString("CommandShortcut%1").arg(i).toUtf8().constData(), buffer))
            commandShortcut = QString(buffer);
        allCommand.push_back(command);
        allToolShortcut.push_back(commandShortcut);
    }
    i--;
    if(!allCommand.empty())
    {
        list->setRowCount(i);
        for(int j = 0; j < i; j++)
        {
            list->setItem(j, 0, new QTableWidgetItem(allCommand.at(j)));
            list->setItem(j, 1, new QTableWidgetItem(allToolShortcut.at(j)));
        }
    }
    originalToolsCount = ui->listTools->rowCount();
    originalScriptCount = ui->listScript->rowCount();
    originalCommandCount = ui->listCommand->rowCount();
    ui->listTools->selectRow(0);
    ui->listScript->selectRow(0);
    ui->listCommand->selectRow(0);
    connect(ui->listTools, SIGNAL(itemSelectionChanged()), this, SLOT(onListSelectionChanged()));
    connect(ui->listScript, SIGNAL(itemSelectionChanged()), this, SLOT(onListSelectionChanged()));
    connect(ui->listCommand, SIGNAL(itemSelectionChanged()), this, SLOT(onListSelectionChanged()));
    connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    emit ui->listTools->itemSelectionChanged();
    updateToolsBtnEnabled();

    Config()->setupWindowPos(this);
}

void FavouriteTools::setupTools(QString name, QTableWidget* list)
{
    QStringList tblHeaderTools;
    tblHeaderTools << tr("Path") << tr("Shortcut") << tr("Description");
    list->setColumnCount(3);
    list->verticalHeader()->setVisible(false);
    list->setHorizontalHeaderLabels(tblHeaderTools);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list->setSelectionBehavior(QAbstractItemView::SelectRows);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setShowGrid(false);
    list->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    list->verticalHeader()->setDefaultSectionSize(15);

    char buffer[MAX_SETTING_SIZE];
    int i;
    std::vector<QString> allToolPath;
    std::vector<QString> allToolShortcut;
    std::vector<QString> allToolDescription;
    for(i = 1; BridgeSettingGet("Favourite", (name + QString::number(i)).toUtf8().constData(), buffer); i++)
    {
        QString toolPath = QString(buffer);
        QString toolShortcut("");
        QString toolDescription("");
        if(BridgeSettingGet("Favourite", (name + "Shortcut" + QString::number(i)).toUtf8().constData(), buffer))
            toolShortcut = QString(buffer);
        if(BridgeSettingGet("Favourite", (name + "Description" + QString::number(i)).toUtf8().constData(), buffer))
            toolDescription = QString(buffer);
        allToolPath.push_back(toolPath);
        allToolShortcut.push_back(toolShortcut);
        allToolDescription.push_back(toolDescription);
    }
    i--;
    if(!allToolPath.empty())
    {
        list->setRowCount(i);
        for(int j = 0; j < i; j++)
        {
            list->setItem(j, 0, new QTableWidgetItem(allToolPath.at(j)));
            list->setItem(j, 1, new QTableWidgetItem(allToolShortcut.at(j)));
            list->setItem(j, 2, new QTableWidgetItem(allToolDescription.at(j)));
        }
    }
}

// Events
void FavouriteTools::on_btnAddFavouriteTool_clicked()
{
    QString filename;
    char buffer[MAX_SETTING_SIZE];
    memset(buffer, 0, sizeof(buffer));
    BridgeSettingGet("Favourite", "LastToolPath", buffer);
    BrowseDialog browse(this, tr("Browse tool"), tr("Enter the path of the tool."), tr("Executable Files (*.exe);;All Files (*.*)"), QString(buffer), false);
    if(browse.exec() != QDialog::Accepted || browse.path.length() == 0)
        return;
    filename = browse.path;
    BridgeSettingSet("Favourite", "LastToolPath", filename.toUtf8().constData());
    int rows = ui->listTools->rowCount();
    ui->listTools->setRowCount(rows + 1);
    ui->listTools->setItem(rows, 0, new QTableWidgetItem(filename));
    ui->listTools->setItem(rows, 1, new QTableWidgetItem(QString()));
    ui->listTools->setItem(rows, 2, new QTableWidgetItem(QString()));
    if(rows == 0)
        ui->listTools->selectRow(0);
    updateToolsBtnEnabled();
}

void FavouriteTools::on_btnEditFavouriteTool_clicked()
{
    QTableWidget* table = ui->listTools;
    if(!table->rowCount())
        return;
    QString filename = table->item(table->currentRow(), 0)->text();
    BrowseDialog browse(this, tr("Browse tool"), tr("Enter the path of the tool."), tr("Executable Files (*.exe);;All Files (*.*)"), filename, false);
    if(browse.exec() != QDialog::Accepted)
        return;
    filename = browse.path;
    BridgeSettingSet("Favourite", "LastToolPath", filename.toUtf8().constData());
    table->item(table->currentRow(), 0)->setText(filename);
}

void FavouriteTools::upbutton(QTableWidget* table)
{
    if(!table->rowCount())
        return;
    int currentRow = table->currentRow();
    if(currentRow == 0)
        return;
    for(int i = 0; i < table->columnCount(); i++)
    {
        QString prevContent(table->item(currentRow, i)->text());
        table->item(currentRow, i)->setText(table->item(currentRow - 1, i)->text());
        table->item(currentRow - 1, i)->setText(prevContent);
    }
    table->selectRow(currentRow - 1);
}

void FavouriteTools::downbutton(QTableWidget* table)
{
    if(!table->rowCount())
        return;
    int currentRow = table->currentRow();
    if(currentRow == table->rowCount() - 1)
        return;
    for(int i = 0; i < table->columnCount(); i++)
    {
        QString prevContent(table->item(currentRow, i)->text());
        table->item(currentRow, i)->setText(table->item(currentRow + 1, i)->text());
        table->item(currentRow + 1, i)->setText(prevContent);
    }
    table->selectRow(currentRow + 1);
}

void FavouriteTools::on_btnRemoveFavouriteTool_clicked()
{
    QTableWidget* table = ui->listTools;
    if(!table->rowCount())
        return;
    table->removeRow(table->currentRow());
    updateToolsBtnEnabled();
}

void FavouriteTools::on_btnDescriptionFavouriteTool_clicked()
{
    QTableWidget* table = ui->listTools;
    if(!table->rowCount())
        return;
    QString description = table->item(table->currentRow(), 2)->text();
    if(SimpleInputBox(this, tr("Enter the description"), description, description, tr("This string will appear in the menu.")))
        table->item(table->currentRow(), 2)->setText(description);
}

void FavouriteTools::on_btnUpFavouriteTool_clicked()
{
    upbutton(ui->listTools);
}

void FavouriteTools::on_btnDownFavouriteTool_clicked()
{
    downbutton(ui->listTools);
}

void FavouriteTools::on_btnAddFavouriteScript_clicked()
{
    QString filename;
    char buffer[MAX_SETTING_SIZE];
    memset(buffer, 0, sizeof(buffer));
    BridgeSettingGet("Favourite", "LastScriptPath", buffer);
    filename = QFileDialog::getOpenFileName(this, tr("Select script"), QString(buffer), tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(filename.size() == 0)
        return;
    filename = QDir::toNativeSeparators(filename);
    BridgeSettingSet("Favourite", "LastScriptPath", filename.toUtf8().constData());
    int rows = ui->listScript->rowCount();
    ui->listScript->setRowCount(rows + 1);
    ui->listScript->setItem(rows, 0, new QTableWidgetItem(filename));
    ui->listScript->setItem(rows, 1, new QTableWidgetItem(QString()));
    ui->listScript->setItem(rows, 2, new QTableWidgetItem(QString()));
    if(rows == 0)
        ui->listScript->selectRow(0);
    updateScriptsBtnEnabled();
}

void FavouriteTools::on_btnEditFavouriteScript_clicked()
{
    QTableWidget* table = ui->listScript;
    if(!table->rowCount())
        return;
    QString filename = table->item(table->currentRow(), 0)->text();
    filename = QFileDialog::getOpenFileName(this, tr("Select script"), filename, tr("Script files (*.txt *.scr);;All files (*.*)"));
    if(filename.size() == 0)
        return;
    filename = QDir::toNativeSeparators(filename);
    BridgeSettingSet("Favourite", "LastScriptPath", filename.toUtf8().constData());
    table->item(table->currentRow(), 0)->setText(filename);
}

void FavouriteTools::on_btnRemoveFavouriteScript_clicked()
{
    QTableWidget* table = ui->listScript;
    if(!table->rowCount())
        return;
    table->removeRow(table->currentRow());
    updateScriptsBtnEnabled();
}

void FavouriteTools::on_btnDescriptionFavouriteScript_clicked()
{
    QTableWidget* table = ui->listScript;
    if(!table->rowCount())
        return;
    QString description = table->item(table->currentRow(), 2)->text();
    if(SimpleInputBox(this, tr("Enter the description"), description, description, tr("This string will appear in the menu.")))
        table->item(table->currentRow(), 2)->setText(description);
}

void FavouriteTools::on_btnUpFavouriteScript_clicked()
{
    upbutton(ui->listScript);
}

void FavouriteTools::on_btnDownFavouriteScript_clicked()
{
    downbutton(ui->listScript);
}

void FavouriteTools::on_btnAddFavouriteCommand_clicked()
{
    QString cmd;
    if(SimpleInputBox(this, tr("Enter the command you want to favourite"), "", cmd, tr("Example: bphws csp")))
    {
        int rows = ui->listCommand->rowCount();
        ui->listCommand->setRowCount(rows + 1);
        ui->listCommand->setItem(rows, 0, new QTableWidgetItem(cmd));
        ui->listCommand->setItem(rows, 1, new QTableWidgetItem(QString()));
        if(rows == 0)
            ui->listCommand->selectRow(0);
        updateCommandsBtnEnabled();
    }
}

void FavouriteTools::on_btnEditFavouriteCommand_clicked()
{
    QTableWidget* table = ui->listCommand;
    if(!table->rowCount())
        return;
    QString cmd;
    if(SimpleInputBox(this, tr("Enter a new command"), table->item(table->currentRow(), 0)->text(), cmd, tr("Example: bphws ESP")) && cmd.length())
        table->item(table->currentRow(), 0)->setText(cmd);
}

void FavouriteTools::on_btnRemoveFavouriteCommand_clicked()
{
    QTableWidget* table = ui->listCommand;
    if(!table->rowCount())
        return;
    table->removeRow(table->currentRow());
    updateCommandsBtnEnabled();
}

void FavouriteTools::on_btnUpFavouriteCommand_clicked()
{
    upbutton(ui->listCommand);
}

void FavouriteTools::on_btnDownFavouriteCommand_clicked()
{
    downbutton(ui->listCommand);
}

void FavouriteTools::onListSelectionChanged()
{
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if(table == nullptr)
        return;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if(indexes.count() < 1)
        return;
    ui->shortcutEdit->setErrorState(false);
    ui->shortcutEdit->setText(table->item(table->currentRow(), 1)->text());
}

void FavouriteTools::on_shortcutEdit_askForSave()
{
    QTableWidget* table;
    switch(ui->tabWidget->currentIndex())
    {
    case 0:
        table = ui->listTools;
        break;
    case 1:
        table = ui->listScript;
        break;
    case 2:
        table = ui->listCommand;
        break;
    default:
        return;
    }
    if(!table->rowCount())
        return;
    const QKeySequence newKey = ui->shortcutEdit->getKeysequence();
    if(newKey != currentShortcut)
    {
        bool good = true;
        if(!newKey.isEmpty())
        {
            for(auto i = Config()->Shortcuts.cbegin(); i != Config()->Shortcuts.cend(); ++i)
            {
                if(i.value().Hotkey == newKey) //newkey is trying to override a global shortcut
                {
                    good = false;
                    break;
                }
            }
        }
        if(good)
        {
            QString keyText = "";
            if(!newKey.isEmpty())
                keyText = newKey.toString(QKeySequence::NativeText);
            table->item(table->currentRow(), 1)->setText(keyText);
            ui->shortcutEdit->setErrorState(false);
        }
        else
        {
            ui->shortcutEdit->setErrorState(true);
        }
    }
}

void FavouriteTools::on_btnClearShortcut_clicked()
{
    QTableWidget* table;
    switch(ui->tabWidget->currentIndex())
    {
    case 0:
        table = ui->listTools;
        break;
    case 1:
        table = ui->listScript;
        break;
    case 2:
        table = ui->listCommand;
        break;
    default:
        return;
    }
    if(!table->rowCount())
        return;
    QString emptyString;
    ui->shortcutEdit->setText(emptyString);
    table->item(table->currentRow(), 1)->setText(emptyString);
}

void FavouriteTools::on_btnOK_clicked()
{
    int i;
    for(i = 1; i <= ui->listTools->rowCount(); i++)
    {
        BridgeSettingSet("Favourite", QString("Tool%1").arg(i).toUtf8().constData(), ui->listTools->item(i - 1, 0)->text().toUtf8().constData());
        BridgeSettingSet("Favourite", QString("ToolShortcut%1").arg(i).toUtf8().constData(), ui->listTools->item(i - 1, 1)->text().toUtf8().constData());
        BridgeSettingSet("Favourite", QString("ToolDescription%1").arg(i).toUtf8().constData(), ui->listTools->item(i - 1, 2)->text().toUtf8().constData());
    }
    if(ui->listTools->rowCount() == 0)
    {
        BridgeSettingSet("Favourite", "Tool1", "");
        BridgeSettingSet("Favourite", "ToolShortcut1", "");
        BridgeSettingSet("Favourite", "ToolDescription1", "");
    }
    else
    {
        i = ui->listTools->rowCount() + 1;
        do // Run this at least once to ensure no old tools can be brought to live
        {
            BridgeSettingSet("Favourite", QString("Tool%1").arg(i).toUtf8().constData(), "");
            BridgeSettingSet("Favourite", QString("ToolShortcut%1").arg(i).toUtf8().constData(), "");
            BridgeSettingSet("Favourite", QString("ToolDescription%1").arg(i).toUtf8().constData(), "");
            i++;
        }
        while(i <= originalToolsCount);
    }
    for(int i = 1; i <= ui->listScript->rowCount(); i++)
    {
        BridgeSettingSet("Favourite", QString("Script%1").arg(i).toUtf8().constData(), ui->listScript->item(i - 1, 0)->text().toUtf8().constData());
        BridgeSettingSet("Favourite", QString("ScriptShortcut%1").arg(i).toUtf8().constData(), ui->listScript->item(i - 1, 1)->text().toUtf8().constData());
        BridgeSettingSet("Favourite", QString("ScriptDescription%1").arg(i).toUtf8().constData(), ui->listScript->item(i - 1, 2)->text().toUtf8().constData());
    }
    if(ui->listScript->rowCount() == 0)
    {
        BridgeSettingSet("Favourite", "Script1", "");
        BridgeSettingSet("Favourite", "ScriptShortcut1", "");
        BridgeSettingSet("Favourite", "ScriptDescription1", "");
    }
    else
    {
        i = ui->listScript->rowCount() + 1;
        do
        {
            BridgeSettingSet("Favourite", QString("Script%1").arg(i).toUtf8().constData(), "");
            BridgeSettingSet("Favourite", QString("ScriptShortcut%1").arg(i).toUtf8().constData(), "");
            BridgeSettingSet("Favourite", QString("ScriptDescription%1").arg(i).toUtf8().constData(), "");
            i++;
        }
        while(i <= originalScriptCount);
    }
    for(int i = 1; i <= ui->listCommand->rowCount(); i++)
    {
        BridgeSettingSet("Favourite", QString("Command%1").arg(i).toUtf8().constData(), ui->listCommand->item(i - 1, 0)->text().toUtf8().constData());
        BridgeSettingSet("Favourite", QString("CommandShortcut%1").arg(i).toUtf8().constData(), ui->listCommand->item(i - 1, 1)->text().toUtf8().constData());
    }
    if(ui->listCommand->rowCount() == 0)
    {
        BridgeSettingSet("Favourite", "Command1", "");
        BridgeSettingSet("Favourite", "CommandShortcut1", "");
    }
    else
    {
        i = ui->listCommand->rowCount() + 1;
        do
        {
            BridgeSettingSet("Favourite", QString("Command%1").arg(i).toUtf8().constData(), "");
            BridgeSettingSet("Favourite", QString("CommandShortcut%1").arg(i).toUtf8().constData(), "");
            i++;
        }
        while(i <= originalCommandCount);
    }
    this->done(QDialog::Accepted);
}

void FavouriteTools::tabChanged(int i)
{
    switch(i)
    {
    case 0:
        emit ui->listTools->itemSelectionChanged();
        updateToolsBtnEnabled();
        break;
    case 1:
        emit ui->listScript->itemSelectionChanged();
        updateScriptsBtnEnabled();
        break;
    case 2:
        emit ui->listCommand->itemSelectionChanged();
        updateCommandsBtnEnabled();
        break;
    }
}

FavouriteTools::~FavouriteTools()
{
    Config()->saveWindowPos(this);
    delete ui;
}

void FavouriteTools::updateToolsBtnEnabled()
{
    int rows = ui->listTools->rowCount();
    bool enabled = rows != 0;
    bool updownEnabled = enabled && rows != 1;
    ui->btnRemoveFavouriteTool->setEnabled(enabled);
    ui->btnEditFavouriteTool->setEnabled(enabled);
    ui->btnUpFavouriteTool->setEnabled(updownEnabled);
    ui->btnDownFavouriteTool->setEnabled(updownEnabled);
    ui->btnDescriptionFavouriteTool->setEnabled(enabled);
}

void FavouriteTools::updateScriptsBtnEnabled()
{
    int rows = ui->listScript->rowCount();
    bool enabled = rows != 0;
    bool updownEnabled = enabled && rows != 1;
    ui->btnRemoveFavouriteScript->setEnabled(enabled);
    ui->btnEditFavouriteScript->setEnabled(enabled);
    ui->btnUpFavouriteScript->setEnabled(updownEnabled);
    ui->btnDownFavouriteScript->setEnabled(updownEnabled);
    ui->btnDescriptionFavouriteScript->setEnabled(enabled);

}

void FavouriteTools::updateCommandsBtnEnabled()
{
    int rows = ui->listCommand->rowCount();
    bool enabled = rows != 0;
    bool updownEnabled = enabled && rows != 1;
    ui->btnRemoveFavouriteCommand->setEnabled(enabled);
    ui->btnEditFavouriteCommand->setEnabled(enabled);
    ui->btnUpFavouriteCommand->setEnabled(updownEnabled);
    ui->btnDownFavouriteCommand->setEnabled(updownEnabled);
}
