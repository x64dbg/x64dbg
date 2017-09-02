#include "ComboBoxDialog.h"
#include "ui_ComboBoxDialog.h"
#include <QLineEdit>
#include <QStringListModel>
#include <QListView>
#include <QCompleter>

ComboBoxDialog::ComboBoxDialog(QWidget* parent) : QDialog(parent), ui(new Ui::ComboBoxDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::MSWindowsFixedSizeDialogHint);
    setModal(true); //modal window
    ui->comboBox->setInsertPolicy(QComboBox::NoInsert);
    ui->comboBox->setEditable(false);
    ui->comboBox->setModel(new QStringListModel(this));
    ui->checkBox->hide();
    bChecked = false;
    ui->label->setVisible(false);
}

ComboBoxDialog::~ComboBoxDialog()
{
    delete ui;
}

void ComboBoxDialog::setEditable(bool editable)
{
    ui->comboBox->setEditable(editable);
    if(editable)
        ui->comboBox->completer()->setCompletionMode(QCompleter::PopupCompletion);
}

void ComboBoxDialog::setItems(const QStringList & items)
{
    ((QStringListModel*)ui->comboBox->model())->setStringList(items);
}

void ComboBoxDialog::setMinimumContentsLength(int characters)
{
    ui->comboBox->setMinimumContentsLength(characters);
    // For performance reasons use this policy on large models.
    ui->comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
}

QString ComboBoxDialog::currentText()
{
    return ui->comboBox->currentText();
}

void ComboBoxDialog::setText(const QString & text)
{
    if(ui->comboBox->isEditable())
    {
        ui->comboBox->setEditText(text);
        ui->comboBox->lineEdit()->selectAll();
    }
    else
    {
        ui->comboBox->setCurrentIndex(ui->comboBox->findText(text));
    }
}

void ComboBoxDialog::setPlaceholderText(const QString & text)
{
    if(ui->comboBox->isEditable())
        ui->comboBox->lineEdit()->setPlaceholderText(text);
}

void ComboBoxDialog::enableCheckBox(bool bEnable)
{
    if(bEnable)
        ui->checkBox->show();
    else
        ui->checkBox->hide();
}

void ComboBoxDialog::setCheckBox(bool bSet)
{
    ui->checkBox->setChecked(bSet);
    bChecked = bSet;
}

void ComboBoxDialog::setCheckBoxText(const QString & text)
{
    ui->checkBox->setText(text);
}

void ComboBoxDialog::on_checkBox_toggled(bool checked)
{
    bChecked = checked;
}
