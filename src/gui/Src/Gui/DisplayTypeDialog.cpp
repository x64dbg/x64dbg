#include "DisplayTypeDialog.h"
#include "ui_DisplayTypeDialog.h"
#include "StringUtil.h"
#include "MiscUtil.h"
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "Configuration.h"

DisplayTypeDialog::DisplayTypeDialog(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::DisplayTypeDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(DIcon("visitstruct"));
    setWindowTitle(tr("Display type"));
    setFocusProxy(ui->typeList->mCurList);

    // Minor UI tweaks
    ui->splitter->setStretchFactor(0, 30);
    ui->splitter->setStretchFactor(1, 70);
    ui->addressEdit->setFont(ConfigFont("Disassembly"));

    // Type list
    ui->typeList->addColumnAt(0, tr("Type Name"), true);
    connect(ui->typeList, &StdSearchListView::selectionChanged, this, &DisplayTypeDialog::updateTypeWidgetSlot);
    updateTypeListSlot();

    loadWindowSettings();
}

DisplayTypeDialog::~DisplayTypeDialog()
{
    saveWindowSettings();
    delete ui;
}

void DisplayTypeDialog::pickType(QWidget* parent, duint defaultAddr)
{
    DisplayTypeDialog dialog(parent);
    if(defaultAddr != 0)
    {
        dialog.ui->addressEdit->setText(ToPtrString(defaultAddr));
    }
    dialog.setFocus();
    if(dialog.exec() == QDialog::Accepted)
    {
        QString selectedType = dialog.getSelectedType();
        if(!selectedType.isEmpty())
        {
            emit Bridge::getBridge()->typeVisit(selectedType, dialog.mCurrentAddress);
        }
    }
}

void DisplayTypeDialog::on_addressEdit_textChanged(const QString & addressText)
{
    if(addressText.isEmpty())
    {
        mCurrentAddress = 0;

        ui->addressLabel->setText(tr("Enter address or expression"));
        ui->addressLabel->setStyleSheet("color: gray;");
    }
    else
    {
        bool validExpression = false;
        mCurrentAddress = DbgEval(addressText.toUtf8().constData(), &validExpression);
        if(validExpression)
        {
            if(DbgMemIsValidReadPtr(mCurrentAddress))
            {
                ui->addressLabel->setText(tr("Address: %1").arg(ToPtrString(mCurrentAddress)));
                ui->addressLabel->setStyleSheet("color: #00DD00;");
            }
            else
            {
                ui->addressLabel->setText(tr("Address: %1 (Invalid memory)").arg(ToPtrString(mCurrentAddress)));
                ui->addressLabel->setStyleSheet("color: red;");
            }
        }
        else
        {
            ui->addressLabel->setText(tr("Invalid address expression"));
            ui->addressLabel->setStyleSheet("color: red;");
        }
    }

    updateTypeWidgetSlot();
}

void DisplayTypeDialog::updateTypeListSlot()
{
    QStringList structs;
    DbgFunctions()->EnumStructs([](const char* name, void* userdata)
    {
        ((QStringList*)userdata)->append(name);
    }, &structs);

    if(structs.isEmpty())
    {
        SimpleErrorBox(this, tr("Error"), tr("No types loaded yet, parse a header first..."));
        return;
    }

    ui->typeList->setRowCount(structs.count());
    for(int i = 0; i < structs.count(); i++)
    {
        ui->typeList->setCellContent(i, ColType, structs[i]);
    }
    ui->typeList->reloadData();

    updateTypeWidgetSlot();
}

void DisplayTypeDialog::updateTypeWidgetSlot()
{
    QString typeName = getSelectedType();
    ui->typeWidget->clearTypes();
    if(!typeName.isEmpty())
    {
        TYPEVISITDATA data = {};
        auto typeNameUtf8 = typeName.toUtf8();
        data.typeName = typeNameUtf8.constData();
        data.addr = mCurrentAddress;
        data.maxPtrDepth = -1;
        data.createLabels = false;
        data.callback = [](void* parent, const TYPEDESCRIPTOR * type, void* userdata) -> void*
        {
            return ((TypeWidget*)userdata)->typeAddNode((QTreeWidgetItem*)parent, type);
        };
        data.userdata = ui->typeWidget;

        DbgTypeVisit(&data);
        ui->typeWidget->updateValuesSlot();
    }
}

QString DisplayTypeDialog::getSelectedType() const
{
    auto selectedIndex = ui->typeList->mCurList->getInitialSelection();
    if(selectedIndex < ui->typeList->mCurList->getRowCount())
    {
        return ui->typeList->mCurList->getCellContent(selectedIndex, ColType);
    }
    return QString();
}

void DisplayTypeDialog::saveWindowSettings()
{
    auto section = metaObject()->className();

    char setting[MAX_SETTING_SIZE] = "";
    BridgeSettingSet(section, "Geometry", saveGeometry().toBase64().data());

    BridgeSettingSet(section, "TypeListSplitter", ui->splitter->saveState().toBase64().data());

    ui->typeWidget->saveWindowSettings(section);
}

void DisplayTypeDialog::loadWindowSettings()
{
    auto section = metaObject()->className();

    char setting[MAX_SETTING_SIZE] = "";
    if(BridgeSettingGet(section, "Geometry", setting))
        restoreGeometry(QByteArray::fromBase64(QByteArray(setting)));

    if(BridgeSettingGet(section, "TypeListSplitter", setting))
        ui->splitter->restoreState(QByteArray::fromBase64(QByteArray(setting)));

    ui->typeWidget->loadWindowSettings(section);
}

